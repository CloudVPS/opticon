#include <libopticonf/dump.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

static const char *SPC = "                                "
                         "                                "
                         "                                "
                         "                                ";

/** Escape quoted string content with backslash */
char *dump_escape (const char *str) {
    char *res = (char *) malloc (2*strlen(str)+1);
    const char *icrsr = str;
    char *ocrsr = res;
    while (*icrsr) {
        if (*icrsr == '\\' || *icrsr == '\"') {
            *ocrsr++ = '\\';
        }
        *ocrsr++ = *icrsr++;
    }
    *ocrsr = 0;
    return res;
}

/** Recursive implementation function for dump_var().
  * \param v The (dict) var at the current level to print out.
  * \param into The file to write to.
  * \param _indent The desired indentation level.
  * \return 1 on success, 0 on failure.
  */
int dump_var2 (var *v, FILE *into, int _indent) {
    char *tstr;
    int first=1;
    int indent = _indent;
    if (indent>128) indent = 128;
    var *crsr;
    if (v->type == VAR_DICT || v->type == VAR_ARRAY) {
        crsr = v->value.arr.first;
        while (crsr) {
            if (first) first=0;
            else {
                fprintf (into, ",\n");
            }
            if (indent) fprintf (into, "%s", SPC+(128-indent));
            
            if (v->type == VAR_DICT) {
                tstr = dump_escape (crsr->id);
                fprintf (into, "\"%s\": ", tstr);
                free (tstr);
            }
            switch (crsr->type) {
                case VAR_NULL:
                    fprintf (into, "\"\"");
                    break;
                
                case VAR_INT:
                    fprintf (into, "%i", crsr->value.ival);
                    break;
                    
                case VAR_DOUBLE:
                    fprintf (into, "%f", crsr->value.dval);
                    break;
                
                case VAR_STR:
                    tstr = dump_escape (crsr->value.sval);
                    fprintf (into, "\"%s\"", tstr);
                    free (tstr);
                    break;
                
                case VAR_DICT:
                    fprintf (into, "{\n");
                    if (! dump_var2 (crsr, into, indent+4)) return 0;
                    if (indent) fprintf (into, "%s", SPC+(128-indent));
                    fprintf (into, "}");
                    break;
                    
                case VAR_ARRAY:
                    fprintf (into, "[\n");
                    if (! dump_var2 (crsr, into, indent+4)) return 0;
                    if (indent) fprintf (into, "%s", SPC+(128-indent));
                    fprintf (into, "]");
                    break;
                    
                default:
                    return 0;
            }
            crsr = crsr->next;
        }
        if (! first) fprintf (into, "\n");
        return 1;
    }
    return 0;
}

/** Dump a variable tree in JSON format to a file (descriptor).
  * \param v The tree to write out.
  * \param into The file to write to.
  * \return 1 on success, 0 on failure.
  */
int dump_var (var *v, FILE *into) {
    return dump_var2 (v, into, 0);
}
