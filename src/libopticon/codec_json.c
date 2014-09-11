#include <libopticon/codec_json.h>
#include <libopticon/util.h>
#include <stdlib.h>

/** Write out a meter value in JSON value format.
  * \param type The metertype to read and encode
  * \param m The meter
  * \param pos The array position within the meter
  * \param into The ioport to use
  */
void jsoncodec_dump_val (metertype_t type, meter *m, int pos, ioport *into) {
    char buf[1024];
    switch (type) {
        case MTYPE_INT:
            sprintf (buf, "%llu", m->d.u64[pos]);
            break;
        
        case MTYPE_FRAC:
            sprintf (buf, "%.3f", m->d.frac[pos]);
            break;
        
        case MTYPE_STR:
            sprintf (buf, "\"%s\"", m->d.str[pos].str);
            break;
            
        default:
            buf[0] = '\0';
            break;
    }
    ioport_write (into, buf, strlen (buf));
}

/** Dumps a meter with a path element */
void jsoncodec_dump_pathval (meter *m, int pos, ioport *into, int ind) {
    meter *mm = m;
    char buf[1024];
    if (ind) {
        ioport_write (into, "    {", 5);
    }
    else {
        ioport_write (into, "{", 1);
    }
    while (mm) {
        nodeid2str (mm->id & MMASK_NAME, buf);
        ioport_write (into, "\"", 1);
        ioport_write (into, buf, strlen(buf));
        ioport_write (into, "\":",2);
        jsoncodec_dump_val (mm->id & MMASK_TYPE, mm, pos, into);
        mm = meter_next_sibling (mm);
        if (mm) ioport_write (into, ",", 1);
    }
    ioport_write (into, "}", 1);
}

/** Write out a host's state as JSON data.
  * \param h The host object
  * \param into ioport to use
  */
int jsoncodec_encode_host (ioport *into, host *h) {
    char buffer[256];
    uint64_t pathbuffer[128];
    int paths = 0;
    uint64_t pathmask = 0;
    meter *m = h->first;
    int i;
    int first=1;
    int dobrk = 0;
    while (m) {
        pathmask = idhaspath (m->id);
        if (pathmask) {
            dobrk = 0;
            for (i=0; i<paths; ++i) {
                if (pathbuffer[i] == (m->id & pathmask)) {
                    dobrk = 1;
                    break;
                }
            }
            if (dobrk) {
                m = m->next;
                continue;
            }
            if (paths>127) return 1;
            pathbuffer[paths++] = (m->id & pathmask);
        }
        
        if (first) first=0;
        else ioport_write (into, ",\n", 2);
        ioport_write (into, "\"", 1);

        if (pathmask) {
            id2str (m->id & pathmask, buffer);
            ioport_write (into, buffer, strlen(buffer));
            ioport_write (into, "\":", 2);
            if (m->count > 0) {
                ioport_write (into, "[\n", 2);
            }
            
            for (i=0; (i==0)||(i<m->count); ++i) {
                if (i) ioport_write (into, ",\n", 2);
                jsoncodec_dump_pathval (m,i,into,(m->count ? 1 : 0));
            }
            
            if (m->count > 0) {
                ioport_write (into, "\n]", 2);
            }
        }
        else {
            id2str (m->id, buffer);
            ioport_write (into, buffer, strlen(buffer));
            ioport_write (into, "\":", 2);
            if (m->count > 0) {
                ioport_write (into, "[", 1);
            }
            for (i=0; (i==0)||(i<m->count); ++i) {
                jsoncodec_dump_val (m->id & MMASK_TYPE, m, i, into);
                if ((i+1)<m->count) ioport_write (into, ",",1);
            }
            if (m->count > 0) {
                ioport_write (into, "]", 1);
            }
        }
        m=m->next;
    }
    ioport_write (into,"\n",1);
    return 0;
}

/** Decode host data from JSON (not implemented) */
int jsoncodec_decode_host (ioport *io, host *h) {
    fprintf (stderr, "%% JSON decoding not supported\n");
    return 0;
}

/** Instantiate a JSON codec */
codec *codec_create_json (void) {
    codec *res = (codec *) malloc (sizeof (codec));
    if (! res) return res;
    res->encode_host = jsoncodec_encode_host;
    res->decode_host = jsoncodec_decode_host;
    return res;
}

