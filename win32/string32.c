/*
 * Unicode string management
 *
 * Copyright 1996 Martin von Loewis
 *
 * Conversion between Unicode and ISO-8859-1 is inherently lossy,
 * so the conversion code should be called only if it does not matter
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "windows.h"
#include "string32.h"
#include "xmalloc.h"

LPWSTR STRING32_DupAnsiToUni(LPCSTR src)
{
	LPWSTR dest=xmalloc(2*strlen(src)+2);
	lstrcpyAtoW(dest,src);
	return dest;
}

LPWSTR
STRING32_lstrchrW(LPCWSTR a,WCHAR c) {
	while(*a) {
		if (*a==c)
			return a;
		a++;
	}
	return NULL;
}

LPWSTR
STRING32_strdupW(LPCWSTR a) {
	LPWSTR	b;
	int	len;

	len=sizeof(WCHAR)*(lstrlen32W(a)+1);
	b=(LPWSTR)xmalloc(len);
	memcpy(b,a,len);
	return b;
}
