/*
 * implementation of MSDEVS extensions to string.h
 *
 * Copyright 1999 Corel Corporation  (Albert den Haan)
 */

/* WARNING: The Wine declarations are in tchar.h for now since string.h is 
 * not available to be altered in most development environments.  MSDEVS 5
 * declarse these functions in its own "string.h" */

#include "tchar.h"

#include <ctype.h>
#include <assert.h>

char *_strlwr(char *string) {
    char *cp;

    assert(string != NULL);

    for(cp = string; *cp; cp++) {
	*cp = tolower(*cp);
    }
    return string;
}

char *_strrev(char *string) {
    char *pcFirst, *pcLast;
    assert(string != NULL);

    pcFirst = pcLast = string;
    
    /* find the last character of the string 
     * (i.e. before the assumed nul-character) */
    while(*(pcLast + 1)) {
	pcLast++;
    }

    /* if the following ASSERT fails look for a bad (i.e. not nul-terminated)
     * string */
    assert(pcFirst <= pcLast);

    /* reverse the string */
    while(pcFirst < pcLast) {
	/* swap characters across the middle */
	char cTemp = *pcFirst;
	*pcFirst = *pcLast;
	*pcLast = cTemp;
	/* move towards the middle of the string */
	pcFirst++;
	pcLast--;
    }

    return string;
}

char *_strupr(char *string) {
    char *cp;

    assert(string != NULL);

    for(cp = string; *cp; cp++) {
	*cp = toupper(*cp);
    }
    return string;
}

