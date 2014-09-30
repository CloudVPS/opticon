#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <libopticon/uuid.h>

/** Compare two UUIDs.
  * \param first First UUID.
  * \param second Second UUID.
  * \return 0 if they differ, 1 if they're the same.
  */
int uuidcmp (uuid first, uuid second) {
    return (first.msb == second.msb && first.lsb == second.lsb);
}

/** Turn a UUID string into a UUID value.
  * \param str The input string
  * \return The UUID value.
  */
uuid mkuuid (const char *str) {
    uuid res = {0,0};
    if (! str) return res;
    const char *crsr = str;
    char c;
    int out = 0;
    uint64_t rout = 0;
    int respos = 0;
    int majpos = 1;
    if (! crsr) return res;
    
    while (*crsr && (majpos>=0)) {
        c = *crsr;
        out = -1;
        if ((c>='0')&&(c<='9')) out = c-'0';
        else if ((c>='a')&&(c<='f')) out = (c-'a')+10;
        else if ((c>='A')&&(c<='F')) out = (c-'A')+10;
        if (out >= 0) {
            rout = out;
            rout = rout << (4*(15-respos));
            if (majpos) res.msb |= rout;
            else res.lsb |= rout;
            respos++;
            if (respos>15) {
                respos = 0;
                majpos--;
            }
        }
        crsr++;
    }
    return res;
}

/** Generate a random uuid */
uuid uuidgen (void) {
    uuid res = {0,0};
    int fdevr = open ("/dev/random",O_RDONLY);
    read (fdevr, &res, sizeof(res));
    close (fdevr);
    return res;
}

/** Generate a nil uuid */
uuid uuidnil (void) {
    uuid res = {0,0};
    return res;
}

/** Check whether a uuid is nil */
int uuidvalid (uuid u) {
    return (u.msb && u.lsb);
}

