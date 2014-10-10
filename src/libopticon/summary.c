#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <pthread.h>
#include <libopticon/log.h>
#include <libopticon/summary.h>

/** Allocate a summarydata structure */
summarydata *summarydata_create (void) {
    summarydata *self = (summarydata *) malloc (sizeof (summarydata));
    self->meter = 0;
    self->name.str[0] = 0;
    self->type = SUMMARY_NONE;
    memset (&self->d, 0, sizeof (meterdata));
    self->samplecount = 0;
    return self;
}

/** Free a summarydata structure */
void summarydata_free (summarydata *self) {
    if (self->d.any) free (self->d.any);
    free (self);
}

/** Clear a summarydata structure */
void summarydata_clear (summarydata *self) {
    self->samplecount = 0;
    if (self->d.any) {
        free (self->d.any);
        self->d.any = NULL;
    }
}

/** Reset all sample data back to zero */
void summarydata_clear_sample (summarydata *self) {
    if ((self->meter & MMASK_TYPE) == MTYPE_STR) {
        self->samplecount = 0;
        return;
    }
    summarydata_clear (self);
}

/** Perform an average calculation */
void summarydata_calc_avg (summarydata *self, var *into) {
    if (! (self->samplecount && self->d.any)) {
        switch (self->meter & MMASK_TYPE) {
            case MTYPE_FRAC:
                var_set_double (into, 0.0);
                break;
            
            default:
                var_set_int (into, 0);
                break;
        }
        return;
    }
    switch (self->meter & MMASK_TYPE) {
        case MTYPE_INT:
            var_set_int (into, self->d.u64[0]/(uint64_t)self->samplecount);
            break;
        
        case MTYPE_FRAC:
            var_set_double (into, self->d.frac[0]/(double)self->samplecount);
            break;
        
        default:
            var_set_int (into, 0);
            break;
    }
}

/** Perform a sum calculation */
void summarydata_calc_total (summarydata *self, var *into) {
    if (! self->d.any) {
        switch (self->meter & MMASK_TYPE) {
            case MTYPE_FRAC:
                var_set_double (into, 0.0);
                break;
            
            default:
                var_set_int (into, 0);
                break;
        }
        return;
    }
    switch (self->meter & MMASK_TYPE) {
        case MTYPE_INT:
            var_set_int (into, self->d.u64[0]);
            break;
        
        case MTYPE_FRAC:
            var_set_double (into, self->d.frac[0]);
            break;
            
        default:
            var_set_int (into, 0);
            break;
    }
}

/** Add a sample to a summarydata structure. In case of numerical types,
  * this will tally up the value in the structure's own meterdata, and
  * increase the samplecount. In case of string types, a match will be made
  * against the existing string value in the object's own meterdata, and
  * only update the samplecount on a match.
  * \param Self the summarydata structure
  * \param d The meterdata to process.
  */
void summarydata_add (summarydata *self, meterdata *d, metertype_t tp) {
    log_debug ("(summarydata_add) self->mtype %016llx "
               "tp %016llx\n", self->meter & MMASK_TYPE, tp & MMASK_TYPE);
    switch (self->meter & MMASK_TYPE) {
        case MTYPE_INT:
            if (! self->d.u64) {
                self->d.u64 = (uint64_t *) malloc (sizeof (uint64_t));
                *(self->d.u64) = 0;
            }
            switch (tp & MMASK_TYPE) {
                case MTYPE_INT:
                    log_debug ("(summarydata_add_int) from_int %llu", *(d->u64));
                    *(self->d.u64) += *(d->u64);
                    break;
                
                case MTYPE_FRAC:
                    log_debug ("(summarydata_add_int) from_f %f", *(d->frac));
                    *(self->d.u64) += (uint64_t) *(d->frac);
                    break;
                
                case MTYPE_STR:
                    *(self->d.u64) += strtoull (d->str->str,NULL,10);
                    break;
                
                default:
                    break;
            }
            self->samplecount++;
            break;
        
        case MTYPE_FRAC:
            if (! self->d.frac) {
                self->d.frac = (double *) malloc (sizeof (double));
                *(self->d.frac) = 0.00;
            }
            switch (tp & MMASK_TYPE) {
                case MTYPE_INT:
                    log_debug ("(summarydata_add_f) from_int %llu", *(d->u64));
                    *(self->d.frac) += (double) *(d->u64);
                    break;
                
                case MTYPE_FRAC:
                    log_debug ("(summarydata_add_f) from_f %f", *(d->frac));
                    *(self->d.frac) += *(d->frac);
                    break;
                    
                case MTYPE_STR:
                    *(self->d.frac) += atof (d->str->str);
                    break;
                    
                default:
                    break;
            }
            self->samplecount++;
            break;
        
        case MTYPE_STR:
            if (! self->d.str) {
                /* We really shouldn't be here, but this will make
                 * the whole thing fail softly. */
                self->d.str = (fstring *) malloc (sizeof (fstring));
                self->d.str->str[0] = 0;
            }
            if ((tp & MMASK_TYPE) != MTYPE_STR) break;

            log_debug ("(summarydata_add_str) from_s %f", d->str->str);
            
            if (strcmp (self->d.str->str, d->str->str) == 0) {
                self->samplecount++;
            }
            break;
        
        default:
            break;
    }
}

/** Initialize a summaryinfo structure */
void summaryinfo_init (summaryinfo *self) {
    self->count = 0;
    self->array = NULL;
    pthread_mutex_init (&self->mutex, NULL);
}

/** Clear all summary data */
void summaryinfo_clear (summaryinfo *self) {
    if (self->count) {
        for (int i=0; i<self->count; ++i) {
            summarydata_free (self->array[i]);
        }
        free (self->array);
        self->array = NULL;
        self->count = 0;
    }
}

/** Backend function to add_summary_total and add_summary_avg */
void summaryinfo_add_summary_any (summaryinfo *self, const char *name,
                                  meterid_t mid, summarytype mytype) {
    size_t sz;
    summarydata *newdt = summarydata_create();
    newdt->meter = mid;
    strncpy (newdt->name.str, name, 127);
    newdt->name.str[127] = 0;
    newdt->type = mytype;
    
    if (self->count) {
        self->count++;
        sz = self->count * sizeof (summarydata *);
        self->array = (summarydata **) realloc (self->array, sz);
    }
    else {
        self->count = 1;
        sz = sizeof (summarydata *);
        self->array = (summarydata **) malloc (sz);
    }

    self->array[self->count-1] = newdt;
}

/** Add an averaging summary to the list.
  * \param self The summaryinfo
  * \param name The name of the summary
  * \param mid The meterid to summarize
  */
void summaryinfo_add_summary_avg (summaryinfo *self, const char *name,
                                  meterid_t mid) {
    summaryinfo_add_summary_any (self, name, mid, SUMMARY_AVG);
}

/** Add a summing summary to the list.
  * \param self The summaryinfo
  * \param name The name of the summary
  * \param mid The meterid to summarize
  */
void summaryinfo_add_summary_total (summaryinfo *self, const char *name,
                                    meterid_t mid) {
    summaryinfo_add_summary_any (self, name, mid, SUMMARY_TOTAL);
}

/** Add a counting summary to the list. This is necessarily a string
  * match, that will count the number of matches.
  * \param self The summaryinfo
  * \param name The name of the summary
  * \param mid The meterid to summarize
  * \param match The matchstr
  */
void summaryinfo_add_summary_count (summaryinfo *self, const char *name,
                                    meterid_t _mid, const char *match) {
    /* Our match meter is going to be a string no matter what */
    meterid_t mid = (_mid & MMASK_NAME) | MTYPE_STR;
    size_t sz;
    summarydata *newdt = summarydata_create();
    newdt->meter = mid;
    strncpy (newdt->name.str, name, 127);
    newdt->name.str[127] = 0;
    newdt->type = SUMMARY_COUNT;
    newdt->d.str = (fstring *) malloc (sizeof (fstring));
    strncpy (newdt->d.str->str, match, 127);
    newdt->d.str->str[127] = 0;
    
    if (self->count) {
        self->count++;
        sz = self->count * sizeof (summarydata);
        self->array = (summarydata **) realloc (self->array, sz);
    }
    else {
        self->count = 1;
        sz = sizeof (summarydata);
        self->array = (summarydata **) malloc (sz);
    }

    self->array[self->count-1] = newdt;
}

/** Indicate a beginning of a summary round. This will typically be followed
  * by going over all hosts and feeding the meters to be summarized.
  * This function itself just clears the sample data.
  */
void summaryinfo_start_round (summaryinfo *self) {
    for (int i=0; i<self->count; ++i) {
        summarydata_clear_sample(self->array[i]);
    }
}

/** Feed the summarizer metering data.
  * \param self The summaryinfo
  * \param mid The meter's id and type
  * \param d The meter's data
  */
void summaryinfo_add_meterdata (summaryinfo *self, meterid_t mid, meterdata *d) {
    char midstr[16];
    id2str (mid&MMASK_NAME, midstr);
    log_debug ("(summaryinfo_add_meterdata) mid %s", midstr);
    for (int i=0; i<self->count; ++i) {
        if ((self->array[i]->meter & MMASK_NAME) == (mid & MMASK_NAME)) {
            summarydata_add (self->array[i], d, mid);
        }
    }
}

/** Tally up all summary data and dump it into a var, with value indexed
  * by summary name.
  */
var *summaryinfo_tally_round (summaryinfo *self) {
    var *res = var_alloc();
    
    for (int i=0; i<self->count; ++i) {
        summarydata *crsr = self->array[i];
        var *tlly = var_get_or_make (res, crsr->name.str, VAR_NULL);
        switch (crsr->type) {
            case SUMMARY_AVG:
                summarydata_calc_avg (crsr, tlly);
                break;
            
            case SUMMARY_TOTAL:
                summarydata_calc_total (crsr, tlly);
                break;
            
            case SUMMARY_COUNT:
                var_set_int (tlly, crsr->samplecount);
                break;
            
            default:
                var_set_int (tlly, 0);
                break;
        }
    }
    
    return res;
}
