#include <libopticon/dump.h>
#include <libopticon/ioport.h>
#include <libopticon/ioport_file.h>
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
int write_var_indented (var *v, ioport *into, int _indent) {
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
                ioport_printf (into, ",\n");
            }
            if (indent) ioport_printf (into, "%s", SPC+(128-indent));
            
            if (v->type == VAR_DICT) {
                tstr = dump_escape (crsr->id);
                ioport_printf (into, "\"%s\": ", tstr);
                free (tstr);
            }
            switch (crsr->type) {
                case VAR_NULL:
                    ioport_printf (into, "\"\"");
                    break;
                
                case VAR_INT:
                    ioport_printf (into, "%llu", crsr->value.ival);
                    break;
                    
                case VAR_DOUBLE:
                    ioport_printf (into, "%f", crsr->value.dval);
                    break;
                
                case VAR_STR:
                    tstr = dump_escape (crsr->value.sval);
                    ioport_printf (into, "\"%s\"", tstr);
                    free (tstr);
                    break;
                
                case VAR_DICT:
                    ioport_printf (into, "{\n");
                    if (! write_var_indented (crsr, into, indent+4)) return 0;
                    if (indent) ioport_printf (into, "%s", SPC+(128-indent));
                    ioport_printf (into, "}");
                    break;
                    
                case VAR_ARRAY:
                    ioport_printf (into, "[\n");
                    if (! write_var_indented (crsr, into, indent+4)) return 0;
                    if (indent) ioport_printf (into, "%s", SPC+(128-indent));
                    ioport_printf (into, "]");
                    break;
                    
                default:
                    return 0;
            }
            crsr = crsr->next;
        }
        if (! first) ioport_printf (into, "\n");
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
    int res = 0;
    ioport *io = ioport_create_filewriter (into);
    ioport_write (io, "{\n", 2);
    res = write_var_indented (v, io, 4);
    ioport_write (io, "}\n", 2);
    ioport_close (io);
    return res;
}

int write_var (var *v, ioport *into) {
    int res = 0;
    ioport_write (into, "{\n", 2);
    res = write_var_indented (v, into, 4);
    ioport_write (into, "}\n", 2);
    return res;
}
