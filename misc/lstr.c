/*
 * String functions
 *
 * Copyright 1993 Yngvi Sigurjonsson (yngvi@hafro.is)
 * Copyright 1996 Marcus Meissner
 */

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_WCTYPE_H
# include <wctype.h>
#else
# define towlower(c) tolower(c)
# define towupper(c) toupper(c)
# define iswalnum(c) isalnum(c)
# define iswalpha(c) isalpha(c)
# define iswupper(c) isupper(c)
# define iswlower(c) islower(c)
#endif  /* HAVE_WCTYPE_H */


#include "windows.h"
#include "winnt.h"	/* HEAP_ macros */
#include "task.h"
#include "heap.h"
#include "ldt.h"
#include "stackframe.h"
#include "module.h"
#include "debug.h"

/* Funny to divide them between user and kernel. */

/* be careful: always use functions from wctype.h if character > 255 */

/***********************************************************************
 *		IsCharAlpha (USER.433)
 */
BOOL16 WINAPI IsCharAlpha16(CHAR ch)
{
  return isalpha(ch);   /* This is probably not right for NLS */
}

/***********************************************************************
 *		IsCharAlphanumeric (USER.434)
 */
BOOL16 WINAPI IsCharAlphaNumeric16(CHAR ch)
{
    return isalnum(ch);
}

/***********************************************************************
 *		IsCharUpper (USER.435)
 */
BOOL16 WINAPI IsCharUpper16(CHAR ch)
{
  return isupper(ch);
}

/***********************************************************************
 *		IsCharLower (USER.436)
 */
BOOL16 WINAPI IsCharLower16(CHAR ch)
{
  return islower(ch);
}

/***********************************************************************
 *           AnsiUpper16   (USER.431)
 */
SEGPTR WINAPI AnsiUpper16( SEGPTR strOrChar )
{
  /* I am not sure if the locale stuff works with toupper, but then again 
     I am not sure if the Linux libc locale stuffs works at all */

    /* uppercase only one char if strOrChar < 0x10000 */
    if (HIWORD(strOrChar))
    {
        char *s;
        for (s = PTR_SEG_TO_LIN(strOrChar); *s; s++) *s = toupper(*s);
        return strOrChar;
    }
    else return toupper((char)strOrChar);
}


/***********************************************************************
 *           AnsiUpperBuff16   (USER.437)
 */
UINT16 WINAPI AnsiUpperBuff16( LPSTR str, UINT16 len )
{
    UINT32 count = len ? len : 65536;
    for (; count; count--, str++) *str = toupper(*str);
    return len;
}

/***********************************************************************
 *           AnsiLower16   (USER.432)
 */
SEGPTR WINAPI AnsiLower16( SEGPTR strOrChar )
{
  /* I am not sure if the locale stuff works with toupper, but then again 
     I am not sure if the Linux libc locale stuffs works at all */

    /* lowercase only one char if strOrChar < 0x10000 */
    if (HIWORD(strOrChar))
    {
        char *s;
        for (s = PTR_SEG_TO_LIN( strOrChar ); *s; s++) *s = tolower( *s );
        return strOrChar;
    }
    else return tolower((char)strOrChar);
}


/***********************************************************************
 *           AnsiLowerBuff16   (USER.438)
 */
UINT16 WINAPI AnsiLowerBuff16( LPSTR str, UINT16 len )
{
    UINT32 count = len ? len : 65536;
    for (; count; count--, str++) *str = tolower(*str);
    return len;
}


/***********************************************************************
 *           AnsiNext16   (USER.472)
 */
SEGPTR WINAPI AnsiNext16(SEGPTR current)
{
    return (*(char *)PTR_SEG_TO_LIN(current)) ? current + 1 : current;
}


/***********************************************************************
 *           AnsiPrev16   (USER.473)
 */
SEGPTR WINAPI AnsiPrev16( SEGPTR start, SEGPTR current )
{
    return (current == start) ? start : current - 1;
}


/***********************************************************************
 *           OutputDebugString16   (KERNEL.115)
 */
void WINAPI OutputDebugString16( LPCSTR str )
{
    char *module;
    char *p, *buffer = HeapAlloc( GetProcessHeap(), 0, strlen(str)+2 );
    /* Remove CRs */
    for (p = buffer; *str; str++) if (*str != '\r') *p++ = *str;
    *p = '\0';
    if ((p > buffer) && (p[-1] == '\n')) p[1] = '\0'; /* Remove trailing \n */
    module = MODULE_GetModuleName( GetCurrentTask() );
    TRACE(resource, "%s says '%s'\n",
             module ? module : "???", buffer );
    HeapFree( GetProcessHeap(), 0, buffer );
}


/***********************************************************************
 *           OutputDebugString32A   (KERNEL32
 */
void WINAPI OutputDebugString32A( LPCSTR str )
{
    OutputDebugString16( str );
}



/***********************************************************************
 *           OutputDebugString32W   (KERNEL32
 */
void WINAPI OutputDebugString32W( LPCWSTR str )
{
    LPSTR p = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
    OutputDebugString32A( p );
    HeapFree( GetProcessHeap(), 0, p );
}



/***********************************************************************
 *           CharNext32A   (USER32.29)
 */
LPSTR WINAPI CharNext32A( LPCSTR ptr )
{
    if (!*ptr) return (LPSTR)ptr;
    if (IsDBCSLeadByte32( *ptr )) return (LPSTR)(ptr + 2);
    return (LPSTR)(ptr + 1);
}


/***********************************************************************
 *           CharNextEx32A   (USER32.30)
 */
LPSTR WINAPI CharNextEx32A( WORD codepage, LPCSTR ptr, DWORD flags )
{
    if (!*ptr) return (LPSTR)ptr;
    if (IsDBCSLeadByteEx( codepage, *ptr )) return (LPSTR)(ptr + 2);
    return (LPSTR)(ptr + 1);
}


/***********************************************************************
 *           CharNextExW   (USER32.31)
 */
LPWSTR WINAPI CharNextEx32W(WORD codepage,LPCWSTR x,DWORD flags)
{
    /* FIXME: add DBCS / codepage stuff */
    if (*x) return (LPWSTR)(x+1);
    else return (LPWSTR)x;
}

/***********************************************************************
 *           CharNextW   (USER32.32)
 */
LPWSTR WINAPI CharNext32W(LPCWSTR x)
{
    if (*x) return (LPWSTR)(x+1);
    else return (LPWSTR)x;
}

/***********************************************************************
 *           CharPrev32A   (USER32.33)
 */
LPSTR WINAPI CharPrev32A( LPCSTR start, LPCSTR ptr )
{
    while (*start && (start < ptr))
    {
        LPCSTR next = CharNext32A( start );
        if (next >= ptr) break;
        start = next;
    }
    return (LPSTR)start;
}


/***********************************************************************
 *           CharPrevEx32A   (USER32.34)
 */
LPSTR WINAPI CharPrevEx32A( WORD codepage, LPCSTR start, LPCSTR ptr, DWORD flags )
{
    while (*start && (start < ptr))
    {
        LPCSTR next = CharNextEx32A( codepage, start, flags );
        if (next > ptr) break;
        start = next;
    }
    return (LPSTR)start;
}


/***********************************************************************
 *           CharPrevExW   (USER32.35)
 */
LPWSTR WINAPI CharPrevEx32W(WORD codepage,LPCWSTR start,LPCWSTR x,DWORD flags)
{
    /* FIXME: add DBCS / codepage stuff */
    if (x>start) return (LPWSTR)(x-1);
    else return (LPWSTR)x;
}

/***********************************************************************
 *           CharPrevW   (USER32.36)
 */
LPWSTR WINAPI CharPrev32W(LPCWSTR start,LPCWSTR x)
{
    if (x>start) return (LPWSTR)(x-1);
    else return (LPWSTR)x;
}

/***********************************************************************
 *           CharLowerA   (USER32.25)
 * FIXME: handle current locale
 */
LPSTR WINAPI CharLower32A(LPSTR x)
{
    LPSTR	s;

    if (HIWORD(x))
    {
        s=x;
        while (*s)
        {
            *s=tolower(*s);
            s++;
        }
        return x;
    }
    else return (LPSTR)tolower((char)(int)x);
}

/***********************************************************************
 *           CharLowerBuffA   (USER32.26)
 * FIXME: handle current locale
 */
DWORD WINAPI CharLowerBuff32A(LPSTR x,DWORD buflen)
{
    DWORD done=0;

    if (!x) return 0; /* YES */
    while (*x && (buflen--))
    {
        *x=tolower(*x);
        x++;
        done++;
    }
    return done;
}

/***********************************************************************
 *           CharLowerBuffW   (USER32.27)
 * FIXME: handle current locale
 */
DWORD WINAPI CharLowerBuff32W(LPWSTR x,DWORD buflen)
{
    DWORD done=0;

    if (!x) return 0; /* YES */
    while (*x && (buflen--))
    {
        *x=towlower(*x);
        x++;
        done++;
    }
    return done;
}

/***********************************************************************
 *           CharLowerW   (USER32.28)
 * FIXME: handle current locale
 */
LPWSTR WINAPI CharLower32W(LPWSTR x)
{
    if (HIWORD(x))
    {
        LPWSTR s = x;
        while (*s)
        {
            *s=towlower(*s);
            s++;
        }
        return x;
    }
    else return (LPWSTR)towlower(LOWORD(x));
}

/***********************************************************************
 *           CharUpper32A   (USER32.41)
 * FIXME: handle current locale
 */
LPSTR WINAPI CharUpper32A(LPSTR x)
{
    if (HIWORD(x))
    {
        LPSTR s = x;
        while (*s)
        {
            *s=toupper(*s);
            s++;
        }
        return x;
    }
    return (LPSTR)toupper((char)(int)x);
}

/***********************************************************************
 *           CharUpperBuffA   (USER32.42)
 * FIXME: handle current locale
 */
DWORD WINAPI CharUpperBuff32A(LPSTR x,DWORD buflen)
{
    DWORD done=0;

    if (!x) return 0; /* YES */
    while (*x && (buflen--))
    {
        *x=toupper(*x);
        x++;
        done++;
    }
    return done;
}

/***********************************************************************
 *           CharUpperBuffW   (USER32.43)
 * FIXME: handle current locale
 */
DWORD WINAPI CharUpperBuff32W(LPWSTR x,DWORD buflen)
{
    DWORD done=0;

    if (!x) return 0; /* YES */
    while (*x && (buflen--))
    {
        *x=towupper(*x);
        x++;
        done++;
    }
    return done;
}

/***********************************************************************
 *           CharUpperW   (USER32.44)
 * FIXME: handle current locale
 */
LPWSTR WINAPI CharUpper32W(LPWSTR x)
{
    if (HIWORD(x))
    {
        LPWSTR s = x;
        while (*s)
        {
            *s=towupper(*s);
            s++;
        }
        return x;
    }
    else return (LPWSTR)towupper(LOWORD(x));
}

/***********************************************************************
 *           IsCharAlphaA   (USER32.331)
 * FIXME: handle current locale
 */
BOOL32 WINAPI IsCharAlpha32A(CHAR x)
{
    return isalpha(x);
}

/***********************************************************************
 *           IsCharAlphaNumericA   (USER32.332)
 * FIXME: handle current locale
 */
BOOL32 WINAPI IsCharAlphaNumeric32A(CHAR x)
{
    return isalnum(x);
}

/***********************************************************************
 *           IsCharAlphaNumericW   (USER32.333)
 * FIXME: handle current locale
 */
BOOL32 WINAPI IsCharAlphaNumeric32W(WCHAR x)
{
    return iswalnum(x);
}

/***********************************************************************
 *           IsCharAlphaW   (USER32.334)
 * FIXME: handle current locale
 */
BOOL32 WINAPI IsCharAlpha32W(WCHAR x)
{
    return iswalpha(x);
}

/***********************************************************************
 *           IsCharLower32A   (USER32.335)
 * FIXME: handle current locale
 */
BOOL32 WINAPI IsCharLower32A(CHAR x)
{
    return islower(x);
}

/***********************************************************************
 *           IsCharLower32W   (USER32.336)
 * FIXME: handle current locale
 */
BOOL32 WINAPI IsCharLower32W(WCHAR x)
{
    return iswlower(x);
}

/***********************************************************************
 *           IsCharUpper32A   (USER32.337)
 * FIXME: handle current locale
 */
BOOL32 WINAPI IsCharUpper32A(CHAR x)
{
    return isupper(x);
}

/***********************************************************************
 *           IsCharUpper32W   (USER32.338)
 * FIXME: handle current locale
 */
BOOL32 WINAPI IsCharUpper32W(WCHAR x)
{
    return iswupper(x);
}

/***********************************************************************
 *           FormatMessage32A   (KERNEL32.138)
 * FIXME: missing wrap,FROM_SYSTEM message-loading,
 */
DWORD WINAPI FormatMessage32A(
	DWORD	dwFlags,
	LPCVOID	lpSource,
	DWORD	dwMessageId,
	DWORD	dwLanguageId,
	LPSTR	lpBuffer,
	DWORD	nSize,
	LPDWORD	args /* va_list *args */
) {
	LPSTR	target,t;
	DWORD	talloced;
	LPSTR	from,f;
	DWORD	width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
	DWORD	nolinefeed = 0;

	TRACE(resource, "(0x%lx,%p,%ld,0x%lx,%p,%ld,%p)\n",
		     dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args);
	if (width) 
		FIXME(resource,"line wrapping not supported.\n");
	from = NULL;
	if (dwFlags & FORMAT_MESSAGE_FROM_STRING)
		from = HEAP_strdupA( GetProcessHeap(), 0, (LPSTR)lpSource);
	if (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM) {
		from = HeapAlloc( GetProcessHeap(),0,200 );
		sprintf(from,"Systemmessage, messageid = 0x%08lx\n",dwMessageId);
	}
	if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE) {
		INT32	bufsize;

		dwMessageId &= 0xFFFF;
		bufsize=LoadMessage32A((HMODULE32)lpSource,dwMessageId,dwLanguageId,NULL,100);
		if (bufsize) {
			from = HeapAlloc( GetProcessHeap(), 0, bufsize + 1 );
			LoadMessage32A((HMODULE32)lpSource,dwMessageId,dwLanguageId,from,bufsize+1);
		}
	}
	target	= HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 100);
	t	= target;
	talloced= 100;

#define ADD_TO_T(c) \
	*t++=c;\
	if (t-target == talloced) {\
		target	= (char*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,talloced*2);\
		t	= target+talloced;\
		talloced*=2;\
	}

	if (from) {
		f=from;
		while (*f) {
			if (*f=='%') {
				int	insertnr;
				char	*fmtstr,*sprintfbuf,*x,*lastf;
				DWORD	*argliststart;

				fmtstr = NULL;
				lastf = f;
				f++;
				if (!*f) {
					ADD_TO_T('%');
					continue;
				}
				switch (*f) {
				case '1':case '2':case '3':case '4':case '5':
				case '6':case '7':case '8':case '9':
					insertnr=*f-'0';
					switch (f[1]) {
					case '0':case '1':case '2':case '3':
					case '4':case '5':case '6':case '7':
					case '8':case '9':
						f++;
						insertnr=insertnr*10+*f-'0';
						f++;
						break;
					default:
						f++;
						break;
					}
					if (*f=='!') {
						f++;
						if (NULL!=(x=strchr(f,'!'))) {
							*x='\0';
							fmtstr=HeapAlloc(GetProcessHeap(),0,strlen(f)+2);
							sprintf(fmtstr,"%%%s",f);
							f=x+1;
						} else {
							fmtstr=HeapAlloc(GetProcessHeap(),0,strlen(f));
							sprintf(fmtstr,"%%%s",f);
							f+=strlen(f); /*at \0*/
						}
					} else
					        if(!args) 
						  break;
					else
						fmtstr=HEAP_strdupA(GetProcessHeap(),0,"%s");
					if (args) {
						if (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)
							argliststart=args+insertnr-1;
						else
                                                    argliststart=(*(DWORD**)args)+insertnr-1;

						if (fmtstr[strlen(fmtstr)-1]=='s')
							sprintfbuf=HeapAlloc(GetProcessHeap(),0,strlen((LPSTR)argliststart[0])+1);
						else
							sprintfbuf=HeapAlloc(GetProcessHeap(),0,100);

						/* CMF - This makes a BIG assumption about va_list */
						wvsprintf32A(sprintfbuf, fmtstr, (va_list) argliststart);
						x=sprintfbuf;
						while (*x) {
							ADD_TO_T(*x++);
						}
						HeapFree(GetProcessHeap(),0,sprintfbuf);
					} else {
						/* NULL args - copy formatstr 
						 * (probably wrong)
						 */
						while ((lastf<f)&&(*lastf)) {
							ADD_TO_T(*lastf++);
						}
					}
					HeapFree(GetProcessHeap(),0,fmtstr);
					break;
				case 'n':
					/* FIXME: perhaps add \r too? */
					ADD_TO_T('\n');
					f++;
					break;
				case '0':
					nolinefeed=1;
					f++;
					break;
				default:ADD_TO_T(*f++)
					break;

				}
			} else {
				ADD_TO_T(*f++)
			}
		}
		*t='\0';
	}
	if (nolinefeed) {
	    /* remove linefeed */
	    if(t>target && t[-1]=='\n') {
		*--t=0;
		if(t>target && t[-1]=='\r')
		    *--t=0;
	    }
	} else {
	    /* add linefeed */
	    if(t==target || t[-1]!='\n')
		ADD_TO_T('\n'); /* FIXME: perhaps add \r too? */
	}
	talloced = strlen(target)+1;
	if (nSize && talloced<nSize) {
		target = (char*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,nSize);
	}
	if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
		/* nSize is the MINIMUM size */
		*((LPVOID*)lpBuffer) = (LPVOID)LocalAlloc32(GMEM_ZEROINIT,talloced);
		memcpy(*(LPSTR*)lpBuffer,target,talloced);
	} else
		strncpy(lpBuffer,target,nSize);
	HeapFree(GetProcessHeap(),0,target);
	if (from) HeapFree(GetProcessHeap(),0,from);
	return (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ? 
			strlen(*(LPSTR*)lpBuffer):
			strlen(lpBuffer);
}
#undef ADD_TO_T


/***********************************************************************
 *           FormatMessage32W   (KERNEL32.138)
 */
DWORD WINAPI FormatMessage32W(
	DWORD	dwFlags,
	LPCVOID	lpSource,
	DWORD	dwMessageId,
	DWORD	dwLanguageId,
	LPWSTR	lpBuffer,
	DWORD	nSize,
	LPDWORD	args /* va_list *args */
) {
	LPSTR	target,t;
	DWORD	talloced;
	LPSTR	from,f;
	DWORD	width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
	DWORD	nolinefeed = 0;

	TRACE(resource, "(0x%lx,%p,%ld,0x%lx,%p,%ld,%p)\n",
		     dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args);
	if (width) 
		FIXME(resource,"line wrapping not supported.\n");
	from = NULL;
	if (dwFlags & FORMAT_MESSAGE_FROM_STRING)
		from = HEAP_strdupWtoA(GetProcessHeap(),0,(LPWSTR)lpSource);
	if (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM) {
		/* gather information from system message tables ... */
		from = HeapAlloc( GetProcessHeap(),0,200 );
		sprintf(from,"Systemmessage, messageid = 0x%08lx\n",dwMessageId);
	}
	if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE) {
		INT32	bufsize;

		dwMessageId &= 0xFFFF;
		bufsize=LoadMessage32A((HMODULE32)lpSource,dwMessageId,dwLanguageId,NULL,100);
		if (bufsize)
                {
                    from = HeapAlloc( GetProcessHeap(), 0, bufsize + 1 );
                    LoadMessage32A((HMODULE32)lpSource,dwMessageId,dwLanguageId,from,bufsize+1);
		}
	}
	target	= HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 100 );
	t	= target;
	talloced= 100;

#define ADD_TO_T(c) \
	*t++=c;\
	if (t-target == talloced) {\
		target	= (char*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,talloced*2);\
		t	= target+talloced;\
		talloced*=2;\
	}

	if (from) {
		f=from;
		while (*f) {
			if (*f=='%') {
				int	insertnr;
				char	*fmtstr,*sprintfbuf,*x;
				DWORD	*argliststart;

				fmtstr = NULL;
				f++;
				if (!*f) {
					ADD_TO_T('%');
					continue;
				}
				switch (*f) {
				case '1':case '2':case '3':case '4':case '5':
				case '6':case '7':case '8':case '9':
					insertnr=*f-'0';
					switch (f[1]) {
					case '0':case '1':case '2':case '3':
					case '4':case '5':case '6':case '7':
					case '8':case '9':
						f++;
						insertnr=insertnr*10+*f-'0';
						f++;
						break;
					default:
						f++;
						break;
					}
					if (*f=='!') {
						f++;
						if (NULL!=(x=strchr(f,'!')))
                                                {
                                                    *x='\0';
                                                    fmtstr=HeapAlloc( GetProcessHeap(), 0, strlen(f)+2);
							sprintf(fmtstr,"%%%s",f);
							f=x+1;
						} else {
							fmtstr=HeapAlloc(GetProcessHeap(),0,strlen(f));
							sprintf(fmtstr,"%%%s",f);
							f+=strlen(f); /*at \0*/
						}
					} else
					        if(!args)
						  break;
					else
						fmtstr=HEAP_strdupA( GetProcessHeap(),0,"%s");
					if (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)
						argliststart=args+insertnr-1;
					else
						argliststart=(*(DWORD**)args)+insertnr-1;

					if (fmtstr[strlen(fmtstr)-1]=='s') {
						DWORD	xarr[3];

						xarr[0]=(DWORD)HEAP_strdupWtoA(GetProcessHeap(),0,(LPWSTR)(*(argliststart+0)));
						/* possible invalid pointers */
						xarr[1]=*(argliststart+1);
						xarr[2]=*(argliststart+2);
						sprintfbuf=HeapAlloc(GetProcessHeap(),0,lstrlen32W((LPWSTR)argliststart[0])*2+1);

						/* CMF - This makes a BIG assumption about va_list */
						vsprintf(sprintfbuf, fmtstr, (va_list) xarr);
					} else {
						sprintfbuf=HeapAlloc(GetProcessHeap(),0,100);

						/* CMF - This makes a BIG assumption about va_list */
						wvsprintf32A(sprintfbuf, fmtstr, (va_list) argliststart);
					}
					x=sprintfbuf;
					while (*x) {
						ADD_TO_T(*x++);
					}
					HeapFree(GetProcessHeap(),0,sprintfbuf);
					HeapFree(GetProcessHeap(),0,fmtstr);
					break;
				case 'n':
					/* FIXME: perhaps add \r too? */
					ADD_TO_T('\n');
					f++;
					break;
				case '0':
					nolinefeed=1;
					f++;
					break;
				default:ADD_TO_T(*f++)
					break;

				}
			} else {
				ADD_TO_T(*f++)
			}
		}
		*t='\0';
	}
	if (nolinefeed) {
	    /* remove linefeed */
	    if(t>target && t[-1]=='\n') {
		*--t=0;
		if(t>target && t[-1]=='\r')
		    *--t=0;
	    }
	} else {
	    /* add linefeed */
	    if(t==target || t[-1]!='\n')
		ADD_TO_T('\n'); /* FIXME: perhaps add \r too? */
	}
	talloced = strlen(target)+1;
	if (nSize && talloced<nSize)
		target = (char*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,nSize);
	if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
		/* nSize is the MINIMUM size */
		*((LPVOID*)lpBuffer) = (LPVOID)LocalAlloc32(GMEM_ZEROINIT,talloced*2+2);
		lstrcpynAtoW(*(LPWSTR*)lpBuffer,target,talloced);
	} else
		lstrcpynAtoW(lpBuffer,target,nSize);
	HeapFree(GetProcessHeap(),0,target);
	if (from) HeapFree(GetProcessHeap(),0,from);
	return (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ? 
			lstrlen32W(*(LPWSTR*)lpBuffer):
			lstrlen32W(lpBuffer);
}
#undef ADD_TO_T
