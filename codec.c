#include <codec.h>
#include <stdlib.h>
#include <util.h>

/** Instantiate a JSON codec */
codec *codec_create_json (void) {
    codec *res = (codec *) malloc (sizeof (codec));
    if (! res) return res;
    res->encode_host = jsoncodec_encode_host;
    res->decode_host = jsoncodec_decode_host;
    return res;
}

/** Instantiate a packet codec */
codec *codec_create_pkt (void) {
    codec *res = (codec *) malloc (sizeof (codec));
    if (! res) return res;
    res->encode_host = pktcodec_encode_host;
    res->decode_host = pktcodec_decode_host;
    return res;
}

/** Destroy a codec */
void codec_release (codec *c) {
    free (c);
}

/** Call on a codec to encode a host's data into an ioport */
int codec_encode_host (codec *c, ioport *io, host *h) {
    return c->encode_host (io, h);
}

/** Call on a codec to load host data from an ioport */
int codec_decode_host (codec *c, ioport *io, host *h) {
    return c->decode_host (io, h);
}

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
            sprintf (buf, "%.2f", m->d.frac[pos]);
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
    meter *mm;
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

/** Write out a meter value */
int pktcodec_write_value (ioport *io, meter *m, uint8_t pos) {
    switch (m->id & MMASK_TYPE) {
        case MTYPE_INT:
            return ioport_write_encint (io, m->d.u64[pos]);
        case MTYPE_FRAC:
            return ioport_write_encfrac (io, m->d.frac[pos]);
        case MTYPE_STR:
            return ioport_write_encstring (io, m->d.str[pos].str);
        default:
            return 0;
    }
}

/** Encode host data in packet format */
int pktcodec_encode_host (ioport *io, host *h) {
    meter *m = h->first;
    meterid_t id;
    uint64_t cnt;
    while (m) {
        cnt = m->count;
        if (cnt > 15) cnt = 15;
        id = m->id | cnt;
        if (cnt == 0) cnt = 1;
        if (! ioport_write_u64 (io, id)) return 0;
        for (uint8_t i=0; i<cnt; ++i) {
            if (! pktcodec_write_value (io, m, i)) return 0;
        }
        m = m->next;
    }
    return 1;
}

/** Decode packet data into a host */
int pktcodec_decode_host (ioport *io, host *h) {
    meterid_t mid;
    uint8_t count;
    char strbuf[128];
    meter *M;
    while (1) {
        mid = ioport_read_u64 (io);
        if (mid == 0) break;
        
        M = host_get_meter (h, mid);
        count = mid & MMASK_COUNT;
        meter_setcount (M, count);
        if (! count) count=1;
        for (uint8_t i=0; i<count; ++i) {
            switch (mid & MMASK_TYPE) {
                case MTYPE_INT:
                    meter_set_uint (M, i, ioport_read_encint (io));
                    break;
                    
                case MTYPE_FRAC:
                    meter_set_frac (M, i, ioport_read_encfrac (io));
                    break;
                
                case MTYPE_STR:
                    ioport_read_encstring (io, strbuf);
                    meter_set_str (M, i, strbuf);
                    break;
                
                default:
                    return 0;
            }
        }
    }
    return 1;
}
