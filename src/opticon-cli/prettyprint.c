#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include <libopticon/datatypes.h>
#include <libopticon/util.h>
#include <libopticon/var.h>

#include "prettyprint.h"
#include "api.h"
#include "cmd.h"

/** Display function for host-show section headers */
void print_hdr (const char *hdr) {
    const char *mins = "-----------------------------------------------"
                      "-----------------------------------------------"
                      "-----------------------------------------------";
    
    int minspos = strlen(mins) - 73;
    const char *crsr = hdr;
    printf ("---( ");
    while (*crsr) {
        putc (toupper (*crsr), stdout);
        minspos++;
        crsr++;
    }
    printf (" )");
    printf ("%s", mins + minspos);
    putc ('\n', stdout);
}

/** Display function for host-show data */
void print_value (const char *key, const char *fmt, ...) {
    char val[4096];
    val[0] = 0;
    va_list ap;
    va_start (ap, fmt);
    vsnprintf (val, 4096, fmt, ap);
    va_end (ap);

    const char *dots = "......................";
    int dotspos = strlen(dots) - 18;
    printf ("%s", key);
    dotspos += strlen (key);
    if (dotspos < strlen (dots)) printf ("%s", dots+dotspos);
    printf (": ");
    printf ("%s", val);
    printf ("\n");
}

/** Print out an array as a comma-separated list */
void print_array (const char *key, var *arr) {
    char out[4096];
    out[0] = 0;
    int cnt=0;
    var *crsr = arr->value.arr.first;
    while (crsr) {
        if (cnt) strncat (out, ",", 4096);
        switch (crsr->type) {
            case VAR_INT:
                snprintf (out+strlen(out), 4095-strlen(out), "%llu",
                          var_get_int (crsr));
                break;
            
            case VAR_DOUBLE:
                snprintf (out+strlen(out), 4095-strlen(out), "%.2f",
                          var_get_double (crsr));
                break;
            
            case VAR_STR:
                strncat (out, var_get_str (crsr), 4096);
                break;
            
            default:
                strncat (out, "?", 4096);
                break;
        }
        crsr = crsr->next;
        cnt++;
    }
    
    print_value (key, "%s", out);   
}

/** Print out any non-grid values in a var.
  * \param apires The var
  * \param pfx Path prefix (used for recursion)
  * \param mdef Meter definitions (used for recursion)
  */
void print_values (var *apires, const char *pfx, var *mdef) {
    if (! mdef) mdef = api_get ("/%s/meter", OPTIONS.tenant);
    var *meters = var_get_dict_forkey (mdef, "meter");
    var *crsr = apires->value.arr.first;
    if ( (!pfx) && (crsr) ) {
        print_hdr ("MISC");
    }
    while (crsr) {
        char valbuf[1024];
        const char *name = NULL;
        var *meter;
        if (pfx) {
            sprintf (valbuf, "%s/%s", pfx, crsr->id);
            meter = var_get_dict_forkey (meters, valbuf);
        }
        else meter = var_get_dict_forkey (meters, crsr->id);
        name = var_get_str_forkey (meter, "description");
        if (! name) name = crsr->id;
        switch (crsr->type) {
            case VAR_ARRAY:
                if (crsr->value.arr.first &&
                    crsr->value.arr.first->type != VAR_DICT) {
                    print_array (name, crsr);
                }
                break;
            
            case VAR_STR:
                print_value (name, var_get_str(crsr));
                break;
            
            case VAR_INT:
                sprintf (valbuf, "%llu", var_get_int(crsr));
                print_value (name, valbuf);
                break;
            
            case VAR_DOUBLE:
                sprintf (valbuf, "%.2f", var_get_double(crsr));
                print_value (name, valbuf);
                break;
            
            case VAR_DICT:
                print_values (crsr, crsr->id, mdef);
                break;
                
            default:
                break;
        }
        crsr = crsr->next;
    }
    if (! pfx) var_free (mdef);
}

/** Print out tabular data (like top, df) with headers, bells and
  * whistles.
  * \param arr The array-of-dicts to print
  * \param hdr Array of strings containing column headers. NULL terminated.
  * \param fld Array of strings representing column field ids.
  * \param align Array of columnalign for left/right alignment of columns.
  * \param typ Array of data types.
  * \param wid Array of integers representing column widths.
  * \param suffx Array of suffixes to print after specific column cells.
  * \param div Number to divide a VAR_INT value by prior to printing. The
  *            resulting number will be represented as a float with two
  *            decimals.
  */
void print_table (var *arr, const char **hdr, const char **fld,
                  columnalign *align, vartype *typ, int *wid,
                  const char **suffx, int *div) {
    char fmt[16];
    char buf[1024];
    
    int col = 0;
    while (hdr[col]) {
        strcpy (fmt, "%");
        if (align[col] == CA_L) strcat (fmt, "-");
        if (wid[col]) sprintf (fmt+strlen(fmt), "%i", wid[col]);
        strcat (fmt, "s ");
        printf (fmt, hdr[col]);
        col++;
    }
    printf ("\n");
    
    var *node = arr->value.arr.first;
    while (node) {
        col = 0;
        while (hdr[col]) {
            switch (typ[col]) {
                case VAR_STR:
                    strncpy (buf, var_get_str_forkey (node, fld[col]),512);
                    buf[512] = 0;
                    break;
                
                case VAR_INT:
                    if (div[col]) {
                        double nval =
                            (double) var_get_int_forkey (node, fld[col]);
                        
                        nval = nval / ((double) div[col]);
                        sprintf (buf, "%.2f", nval);
                    }
                    else {
                        sprintf (buf, "%llu",
                                 var_get_int_forkey (node, fld[col]));
                    }
                    break;
                
                case VAR_DOUBLE:
                    sprintf (buf, "%.2f",
                             var_get_double_forkey (node, fld[col]));
                    break;
                
                default:
                    buf[0] = 0;
                    break;
            }
            strcat (buf, suffx[col]);
            strcpy (fmt, "%");
            if (align[col] == CA_L) strcat (fmt, "-");
            if (wid[col]) sprintf (fmt+strlen(fmt), "%i", wid[col]);
            strcat (fmt, "s ");
            printf (fmt, buf);
            col++;
        }
        printf ("\n");
        node = node->next;
    }
}

/** Prepare data for print_table on a generic table */
void print_generic_table (var *table) {
    int rowcount = var_get_count (table);
    if (rowcount < 1) return;
    var *crsr = table->value.arr.first;
    int count = var_get_count (crsr);
    if (count < 1) return;
    int sz = count+1;
    int slen;
    int i;
    char *c;
    
    char **header = (char **) calloc (sz, sizeof(char *));
    char **field = (char **) calloc (sz, sizeof(char *));
    columnalign *align = (columnalign *) calloc (sz, sizeof(columnalign));
    vartype *type = (vartype *) calloc (sz, sizeof(vartype));
    int *width = (int *) calloc (sz, sizeof (int));
    int *div = (int *) calloc (sz, sizeof (int));
    char **suffix = (char **) calloc (sz, sizeof (char *));
    
    var *node = crsr->value.arr.first;
    for (i=0; node && (i<count); node=node->next, ++i) {
        type[i] = node->type;
        field[i] = (char *) node->id;
        width[i] = strlen (node->id);
        header[i] = strdup (node->id);
        for (c = header[i]; *c; ++c) *c = toupper (*c);
        if (node->type == VAR_DOUBLE || node->type == VAR_INT) {
            align[i] = i ? CA_R : CA_L;
            if (node->type == VAR_DOUBLE) {
                if (width[i] < 8) width[i] = 8;
            }
            else if (width[i] < 10) width[i] = 10;
            
        }
        else {
            align[i] = CA_L;
            if (node->type == VAR_STR) {
                slen = strlen (var_get_str(node)) + 1;
                if (slen > width[i]) width[i] = slen;
            }
        }
        suffix[i] = "";
    }
    
    crsr = crsr->next;
    while (crsr) {
        node = crsr->value.arr.first;
        for (i=0; node && (i<count); node=node->next, ++i) {
            if (node->type == VAR_STR) {
                slen = strlen (var_get_str(node)) +1;
                if (slen > width[i]) width[i] = slen;
            }
        }
    
        crsr = crsr->next;
    }
    
    int twidth = 0;
    for (i=0; i<count; ++i) twidth += width[i];
    if (twidth<40) {
        int add = (40 - twidth) / count;
        if (! add) add = 1;
        for (i=0; i<count; ++i) width[i] += add;
    }
    else if (twidth<60) {
        for (i=0; i<count; ++i) width[i]++;
    }
    
    print_hdr (table->id);
    print_table (table, (const char **) header, (const char **) field,
                 align, type, width, (const char **) suffix, div);
    
    for (i=0; i<count; ++i) free (header[i]);
    free (header);
    free (field);
    free (align);
    free (type);
    free (width);
    free (suffix);
    free (div);
}

/** Print out any tables found in a var dictionary. This will be called 
  * by cmd_get_record() for any table-like value not already printed.
  */
void print_tables (var *apires) {
    var *crsr = apires->value.arr.first;
    while (crsr) {
        if (crsr->type == VAR_ARRAY && var_get_count(crsr)) {
            if (crsr->value.arr.first->type == VAR_DICT) {
                print_generic_table (crsr);
            }
        }
        crsr = crsr->next;
    }
}

