#ifndef _WORDLIST_H
#define _WORDLIST_H 1

typedef struct wordlist_s {
    int       argc;
    char    **argv;
} wordlist;

int          wordcount (const char *);
wordlist    *wordlist_create (void);
wordlist    *wordlist_make (const char *);
void         wordlist_add (wordlist *, const char *);
void         wordlist_free (wordlist *);

#endif
