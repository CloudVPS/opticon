#include <libopticon/import.h>
#include <libopticon/log.h>

/** Changes an array of dictionaries from var data into a meter-style
  * 'dictionary of arrays'
  * \param h The host to write meters to.
  * \param prefix The name of the enveloping array.
  * \param v The first array node.
  */
int dictarray_to_host (host *h, const char *prefix, var *v) {
    char tmpid[16];
    meterid_t mid;
    meter *m;
    
    var *first = v;
    var *vv = first->value.arr.first;
    var *crsr;
    while (vv) {
        const char *curid = vv->id;
        if (strlen (prefix) + strlen (curid) > 10) {
            log_error ("Error parsing meter result, label '%s/%s' too "
                       "long", prefix, curid);
            return 0;
        }
        sprintf (tmpid, "%s/%s", prefix, curid);

        crsr = v;
        metertype_t type;
        
        switch (vv->type) {
            case VAR_INT:
                type = MTYPE_INT;
                break;
            
            case VAR_DOUBLE:
                type = MTYPE_FRAC;
                break;
            
            case VAR_STR:
                type = MTYPE_STR;
                break;
            
            default:
                log_error ("Unsupported subtype %i under '%s'", vv->type, tmpid);
                return 0;
        }

        mid = makeid (tmpid, type, 0);
        m = host_get_meter (h, mid);

        int pos = 0;
        int count = v->parent->value.arr.count;
        meter_setcount (m, count);
        
        while (crsr && (pos < count)) {
            if (type == MTYPE_INT) {
                meter_set_uint (m, pos, var_get_int_forkey (crsr, vv->id));
            }
            else if (type == MTYPE_STR) {
                meter_set_str (m, pos, var_get_str_forkey (crsr, vv->id));
            }
            else {
                meter_set_frac (m, pos, var_get_double_forkey (crsr, vv->id));
            }
            pos++;
            crsr = crsr->next;
        }
        vv = vv->next;
    }
    return 1;
}

/** Convert JSON-structured data into meters. The provided data needs
  * to be restricted to the allowed forms:
  *   key:value
  *   key:[value1,value2]
  *   key:{key1:value1,key2:value2}
  *   key:[{key1:value1,key2:value2},{key1:value3,key2:value4}]
  *   key:[]
  * \param h The host to write meters to
  * \param val The data to parse
  * \return 1 on success, 0 on failure.
  */
int var_to_host (host *h, var *val) {
    var *v = val->value.arr.first;
    int count;
    char firstlevel[16];
    char tmpid[16];
    while (v) {
        if (strlen (v->id) > 11) {
            log_error ("Error parsing meter result, label '%s' too long",
                       v->id);
            return 0;
        }
        meterid_t mid;
        meter *m;
        if (v->type == VAR_DOUBLE) {
            mid = makeid (v->id, MTYPE_FRAC, 0);
            m = host_get_meter (h, mid);
            meter_setcount (m, 0);
            meter_set_frac (m, 0, var_get_double (v));
        }
        else if (v->type == VAR_INT) {
            mid = makeid (v->id, MTYPE_INT, 0);
            m = host_get_meter (h, mid);
            meter_setcount (m, 0);
            meter_set_uint (m, 0, var_get_int (v));
        }
        else if (v->type == VAR_STR) {
            mid = makeid (v->id, MTYPE_STR, 0);
            m = host_get_meter (h, mid);
            meter_setcount (m, 0);
            meter_set_str (m, 0, var_get_str (v));
        }
        else if (v->type == VAR_ARRAY) {
            strcpy (firstlevel, v->id);
            var *vv = v->value.arr.first;
            if (vv) {
                if (vv->type == VAR_ARRAY) {
                    log_error ("Error parsing meter result, nested "
                               "array under '%s' not supported",
                               firstlevel);
                    return 0;
                }
                if (vv->type == VAR_DICT) {
                    dictarray_to_host (h, firstlevel, vv);
                }
                else if (vv->type == VAR_DOUBLE) {
                    count = v->value.arr.count;
                    mid = makeid (v->id, MTYPE_FRAC,0);
                    m = host_get_meter (h, mid);
                    meter_setcount (m, count);
                    for (int i=0; i<count; ++i) {
                        double d = var_get_double_atindex (v, i);
                        meter_set_frac (m, i, d);
                    }
                }
                else if (vv->type == VAR_INT) {
                    count = v->value.arr.count;
                    mid = makeid (v->id, MTYPE_INT,0);
                    m = host_get_meter (h, mid);
                    meter_setcount (m, count);
                    for (int i=0; i<count; ++i) {
                        uint64_t d = (uint64_t) var_get_int_atindex (v, i);
                        meter_set_uint (m, i, d);
                    }
                }
                else if (vv->type == VAR_STR) {
                    count = v->value.arr.count;
                    mid = makeid (v->id, MTYPE_STR,0);
                    m = host_get_meter (h, mid);
                    meter_setcount (m, count);
                    for (int i=0; i<count; ++i) {
                        meter_set_str (m, i, var_get_str_atindex (v,i));
                    }
                }
            }
            else {
                mid = makeid (v->id, MTYPE_INT, 0);
                m = host_get_meter (h, mid);
                meter_set_empty_array (m);
            }
        }
        else if (v->type == VAR_DICT) {
            strcpy (firstlevel, v->id);
            var *vv = v->value.arr.first;
            while (vv) {
                if (strlen (vv->id) + strlen (firstlevel) > 10) {
                    log_error ("Error parsing meter result, label "
                               "'%s/%s' too long", firstlevel, v->id);
                    return 0;
                }
                sprintf (tmpid, "%s/%s", firstlevel, vv->id);
                if (vv->type == VAR_ARRAY) {
                    log_error ("Error parsing meter result, array "
                               "under dict '%s' not supported",
                               firstlevel);
                    return 0;
                }
                if (vv->type == VAR_ARRAY) {
                    log_error ("Error parsing meter result, nested "
                               "dict under '%s' not supported",
                               firstlevel);
                    return 0;
                }
                if (vv->type == VAR_DOUBLE) {
                    mid = makeid (tmpid, MTYPE_FRAC,0);
                    m = host_get_meter (h, mid);
                    meter_setcount (m, 0);
                    meter_set_frac (m, 0, var_get_double (vv));
                }
                else if (vv->type == VAR_INT) {
                    mid = makeid (tmpid, MTYPE_INT,0);
                    m = host_get_meter (h, mid);
                    meter_setcount (m, 0);
                    meter_set_uint (m, 0, var_get_int (vv));
                }
                else if (vv->type == VAR_STR) {
                    mid = makeid (tmpid, MTYPE_STR,0);
                    m = host_get_meter (h, mid);
                    meter_setcount (m, 0);
                    meter_set_str (m, 0, var_get_str (vv));
                }
                vv = vv->next;
            }
        }
        
        v = v->next;
    }
    return 1;
}
