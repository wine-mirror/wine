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
#include "string32.h"
#include "xmalloc.h"

int
STRING32_UniLen(LPCWSTR s)
{
	int i;
	for(i=0;*s;s++)
		i++;
	return i;
}

void STRING32_UniToAnsi(LPSTR dest,LPCWSTR src)
{
	static int have_warned=0;
	while(*src)
	{
		if(*src>255 && !have_warned)
		{
			fprintf(stderr,"Cannot deal with non-ANSI characters\n");
			have_warned=1;
		}
		*dest++=*src++;
	}
	/* copy the terminator */
	*dest = *src;
}

/* FIXME: we need to use unsigned char here, for if 
 *        we got chars with the 7th bit set, we will get
 *	  negative integers -> wrong unicode values
 */
void 
STRING32_AnsiToUni(LPWSTR dest,LPCSTR src) {
	unsigned char	*usrc;

	usrc=(unsigned char*)src;
	while(*usrc)
		*dest++=*usrc++;
	*dest = *usrc;
}

LPSTR STRING32_DupUniToAnsi(LPCWSTR src)
{
	LPSTR dest=xmalloc(STRING32_UniLen(src)+1);
	STRING32_UniToAnsi(dest,src);
	return dest;
}

LPWSTR STRING32_DupAnsiToUni(LPCSTR src)
{
	LPWSTR dest=xmalloc(2*strlen(src)+2);
	STRING32_AnsiToUni(dest,src);
	return dest;
}

DWORD STRING32_lstrlenW(LPCWSTR str)
{
	int len;
	for(len=0;*str;str++)
		len++;
	return len;
}

/* not an API function */

WCHAR STRING32_tolowerW(WCHAR c)
{
	/* FIXME: Unicode */
	return tolower(c);
}

int STRING32_lstrcmpniW(LPCWSTR a,LPCWSTR b,DWORD len)
{
	while(len--)
	{
		WCHAR c1,c2;
		c1 = STRING32_tolowerW(*a);
		c2 = STRING32_tolowerW(*b);
		if(c1<c2)return -1;
		if(c1>c2)return 1;
		if(c1==0 && c2==0)return 0;
		if(c1==0)return -1;
		if(c2==0)return 1;
		a++;
		b++;
	}
	return 0;
}

int
STRING32_lstrcmpW(LPCWSTR a,LPCWSTR b) {
	WCHAR	diff;

	while(*a && *b) {
		diff=*a-*b;
		if (diff) return diff;
		a++;
		b++;
	}
	if (*a) return *a;
	if (*b) return -*b;
	return 0;
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

	len=sizeof(WCHAR)*(STRING32_UniLen(a)+1);
	b=(LPWSTR)xmalloc(len);
	memcpy(b,a,len);
	return b;
}
