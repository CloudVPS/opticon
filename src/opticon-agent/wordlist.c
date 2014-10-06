#include "wordlist.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define isspace(q) ((q==' ')||(q=='\t'))

inline char *findspace (char *src) {
	register char *t1;
	register char *t2;
	
	t1 = strchr (src, ' ');
	t2 = strchr (src, '\t');
	if (t1 && t2) {
		if (t1<t2) t2 = NULL;
		else t1 = NULL;
	}
	
	if (t1) return t1;
	return t2;
}

int wordcount (const char *string) {
	int ln;
	int cnt;
	int i;
	
	cnt = 1;
	ln = strlen (string);
	i = 0;
	
	while (i < ln) {
		if (isspace(string[i])) {
			while (isspace(string[i])) ++i;
			if (string[i]) ++cnt;
		}
		++i;
	}
	
	return cnt;
}

wordlist *wordlist_create (void) {
	wordlist *res;
	
	res = (wordlist *) malloc (sizeof (wordlist));
	res->argc = 0;
	res->argv = NULL;
	return res;
}

void wordlist_add (wordlist *arg, const char *elm) {
	if (arg->argc) {
		arg->argv = (char **) realloc (arg->argv, (arg->argc + 1) * sizeof (char *));
		arg->argv[arg->argc++] = strdup (elm);
	}
	else {
		arg->argc = 1;
		arg->argv = (char **) malloc (sizeof (char *));
		arg->argv[0] = strdup (elm);
	}
}

wordlist *wordlist_make (const char *string) {
	wordlist    *result;
	char		 *rightbound;
	char		 *word;
	char		 *crsr;
	int			  count;
	int			  pos;
	
	crsr = (char *) string;
	while ((*crsr == ' ')||(*crsr == '\t')) ++crsr;
	
	result = (wordlist *) malloc (sizeof (wordlist));
	count = wordcount (crsr);
	result->argc = count;
	result->argv = (char **) malloc (count * sizeof (char *));
	
	pos = 0;
	
	while ((rightbound = findspace (crsr))) {
		word = (char *) malloc ((rightbound-crsr+3) * sizeof (char));
		memcpy (word, crsr, rightbound-crsr);
		word[rightbound-crsr] = 0;
		result->argv[pos++] = word;
		crsr = rightbound;
		while (isspace(*crsr)) ++crsr;
	}
	if (*crsr) {
		word = strdup (crsr);
		result->argv[pos++] = word;
	}
	
	return result;
}

void wordlist_free (wordlist *lst) {
	int i;
	for (i=0;i<lst->argc;++i) free (lst->argv[i]);
	free (lst->argv);
	free (lst);
}
