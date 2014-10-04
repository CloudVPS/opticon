#include <libopticon/codec_pkt.h>
#include <libopticon/util.h>
#include <stdlib.h>

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
        id = m->id | cnt;
        if (cnt == 0) cnt = 1;
        if (! ioport_write_u64 (io, id)) return 0;
        if (cnt < SZ_EMPTY_VAL) {
            for (uint8_t i=0; i<cnt; ++i) {
                if (! pktcodec_write_value (io, m, i)) return 0;
            }
        }
        m = m->next;
    }
    return 1;
}

/** Decode packet data into a host */
int pktcodec_decode_host (ioport *io, host *h) {
    meterid_t mid;
    meterid_t pfx;
    meterid_t lastpfx = 0ULL;
    uint8_t count;
    char strbuf[128];
    meter *M;
    while (1) {
        mid = ioport_read_u64 (io);
        if (mid == 0) break;
        
        /* SZ_EMPTY_ARRAY should go about and find children and
         * empty those instead, if they exist.
         */
        count = mid & MMASK_COUNT;
        if (count == SZ_EMPTY_ARRAY) {
            meter *crsr = host_find_prefix (h, mid, NULL);
            if (crsr) {
                while (crsr) {
                    id2str (crsr->id, strbuf);
                    log_debug ("(decode) emptying %s", strbuf);
                    meter_set_empty_array (crsr);
                    crsr = host_find_prefix (h, mid, crsr);
                }
                continue;
            }
        }
        
        /* If the id has a prefix, get rid of a possible node that
         * has the prefix for its name.
         */
        pfx = idgetprefix (mid);
        if (pfx && (pfx != lastpfx)) {
            lastpfx = pfx;
            meter *crsr = host_find_meter_name (h, pfx);
            while (crsr) {
                id2str (crsr->id, strbuf);
                log_debug ("(decode) replacing %s with subtree", strbuf);
                host_delete_meter (h, crsr);
                crsr = host_find_meter_name (h, pfx);
            }
        }
        
        M = host_get_meter (h, mid);
        meter_setcount (M, count);
        if (! count) count=1;
        if (count >= SZ_EMPTY_VAL) continue;
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

/** Instantiate a packet codec */
codec *codec_create_pkt (void) {
    codec *res = (codec *) malloc (sizeof (codec));
    if (! res) return res;
    res->encode_host = pktcodec_encode_host;
    res->decode_host = pktcodec_decode_host;
    return res;
}

