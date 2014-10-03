#include <libopticon/hash.h>

static uint32_t IHASH = 0;

/** Generic hash-function nicked from grace.
  * \param str The string to hash
  * \return A 32 bit hashed guaranteed not 0.
  */
uint32_t hash_token (const char *str) {
    uint32_t hash = 0;
    uint32_t i    = 0;
    uint32_t s    = 0;
    int sdf;
    
    /* Set up a random intial hash once */
    if (! IHASH)
    {
        sdf = open ("/dev/urandom", O_RDONLY);
        if (sdf<0) sdf = open ("/dev/random", O_RDONLY);
        if (sdf>=0)
        {
            (void) read (sdf, &IHASH, sizeof(IHASH));
            close (sdf);
        }
        if (! IHASH) IHASH = 5138; /* fair dice roll */
    }
    
    hash = IHASH;
    
    const unsigned char* ustr = (const unsigned char*)str;

    for (i = 0; ustr[i]; i++) {
        hash = ((hash << 5) + hash) + (ustr[i] & 0xDF);
    }

    s = hash;
    
    switch (i) { /* note: fallthrough for each case */
        default:
        case 6: hash += (s % 5779) * (ustr[6] & 0xDF);
        case 5: hash += (s % 4643) * (ustr[5] & 0xDF); hash ^= hash << 7;
        case 4: hash += (s %  271) * (ustr[4] & 0xDF);
        case 3: hash += (s % 1607) * (ustr[3] & 0xDF); hash ^= hash << 15;
        case 2: hash += (s % 7649) * (ustr[2] & 0xDF);
        case 1: hash += (s % 6101) * (ustr[1] & 0xDF); hash ^= hash << 25;
        case 0: hash += (s % 1217) * (ustr[0] & 0xDF);
    }
    
    /* zero is inconvenient */
    if (hash == 0) hash = 154004;
    return hash;
}

