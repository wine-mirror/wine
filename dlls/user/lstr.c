/*
 * USER string functions
 *
 * Copyright 1993 Yngvi Sigurjonsson (yngvi@hafro.is)
 * Copyright 1996 Alexandre Julliard
 * Copyright 1996 Marcus Meissner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "winbase.h"
#include "winerror.h"

#include "wine/exception.h"
#include "wine/unicode.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"

#include "excpt.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(resource);

/* filter for page-fault exceptions */
static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ||
        GetExceptionCode() == EXCEPTION_PRIV_INSTRUCTION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

/***********************************************************************
 *           AnsiToOem   (KEYBOARD.5)
 */
INT16 WINAPI AnsiToOem16( LPCSTR s, LPSTR d )
{
    CharToOemA( s, d );
    return -1;
}


/***********************************************************************
 *           OemToAnsi   (KEYBOARD.6)
 */
INT16 WINAPI OemToAnsi16( LPCSTR s, LPSTR d )
{
    OemToCharA( s, d );
    return -1;
}


/***********************************************************************
 *           AnsiToOemBuff   (KEYBOARD.134)
 */
void WINAPI AnsiToOemBuff16( LPCSTR s, LPSTR d, UINT16 len )
{
    if (len != 0) CharToOemBuffA( s, d, len );
}


/***********************************************************************
 *           OemToAnsiBuff   (KEYBOARD.135)
 */
void WINAPI OemToAnsiBuff16( LPCSTR s, LPSTR d, UINT16 len )
{
    if (len != 0) OemToCharBuffA( s, d, len );
}


/***********************************************************************
 *           lstrcmp   (USER.430)
 */
INT16 WINAPI lstrcmp16( LPCSTR str1, LPCSTR str2 )
{
    return (INT16)strcmp( str1, str2 );
}


/***********************************************************************
 *           AnsiUpper   (USER.431)
 */
SEGPTR WINAPI AnsiUpper16( SEGPTR strOrChar )
{
    /* uppercase only one char if strOrChar < 0x10000 */
    if (HIWORD(strOrChar))
    {
        CharUpperA( MapSL(strOrChar) );
        return strOrChar;
    }
    else return toupper((char)strOrChar);
}


/***********************************************************************
 *           AnsiLower   (USER.432)
 */
SEGPTR WINAPI AnsiLower16( SEGPTR strOrChar )
{
    /* lowercase only one char if strOrChar < 0x10000 */
    if (HIWORD(strOrChar))
    {
        CharLowerA( MapSL(strOrChar) );
        return strOrChar;
    }
    else return tolower((char)strOrChar);
}


/***********************************************************************
 *           AnsiUpperBuff   (USER.437)
 */
UINT16 WINAPI AnsiUpperBuff16( LPSTR str, UINT16 len )
{
    CharUpperBuffA( str, len ? len : 65536 );
    return len;
}


/***********************************************************************
 *           AnsiLowerBuff   (USER.438)
 */
UINT16 WINAPI AnsiLowerBuff16( LPSTR str, UINT16 len )
{
    CharLowerBuffA( str, len ? len : 65536 );
    return len;
}


/***********************************************************************
 *           AnsiNext   (USER.472)
 */
SEGPTR WINAPI AnsiNext16(SEGPTR current)
{
    char *ptr = MapSL(current);
    return current + (CharNextA(ptr) - ptr);
}


/***********************************************************************
 *           AnsiPrev   (USER.473)
 */
SEGPTR WINAPI AnsiPrev16( LPCSTR start, SEGPTR current )
{
    char *ptr = MapSL(current);
    return current - (ptr - CharPrevA( start, ptr ));
}


/***********************************************************************
 *           CharNextA   (USER32.@)
 */
LPSTR WINAPI CharNextA( LPCSTR ptr )
{
    if (!*ptr) return (LPSTR)ptr;
    if (IsDBCSLeadByte( ptr[0] ) && ptr[1]) return (LPSTR)(ptr + 2);
    return (LPSTR)(ptr + 1);
}


/***********************************************************************
 *           CharNextExA   (USER32.@)
 */
LPSTR WINAPI CharNextExA( WORD codepage, LPCSTR ptr, DWORD flags )
{
    if (!*ptr) return (LPSTR)ptr;
    if (IsDBCSLeadByteEx( codepage, ptr[0] ) && ptr[1]) return (LPSTR)(ptr + 2);
    return (LPSTR)(ptr + 1);
}


/***********************************************************************
 *           CharNextExW   (USER32.@)
 */
LPWSTR WINAPI CharNextExW( WORD codepage, LPCWSTR ptr, DWORD flags )
{
    /* doesn't make sense, there are no codepages for Unicode */
    return NULL;
}


/***********************************************************************
 *           CharNextW   (USER32.@)
 */
LPWSTR WINAPI CharNextW(LPCWSTR x)
{
    if (*x) x++;

    return (LPWSTR)x;
}


/***********************************************************************
 *           CharPrevA   (USER32.@)
 */
LPSTR WINAPI CharPrevA( LPCSTR start, LPCSTR ptr )
{
    while (*start && (start < ptr))
    {
        LPCSTR next = CharNextA( start );
        if (next >= ptr) break;
        start = next;
    }
    return (LPSTR)start;
}


/***********************************************************************
 *           CharPrevExA   (USER32.@)
 */
LPSTR WINAPI CharPrevExA( WORD codepage, LPCSTR start, LPCSTR ptr, DWORD flags )
{
    while (*start && (start < ptr))
    {
        LPCSTR next = CharNextExA( codepage, start, flags );
        if (next > ptr) break;
        start = next;
    }
    return (LPSTR)start;
}


/***********************************************************************
 *           CharPrevExW   (USER32.@)
 */
LPSTR WINAPI CharPrevExW( WORD codepage, LPCWSTR start, LPCWSTR ptr, DWORD flags )
{
    /* doesn't make sense, there are no codepages for Unicode */
    return NULL;
}


/***********************************************************************
 *           CharPrevW   (USER32.@)
 */
LPWSTR WINAPI CharPrevW(LPCWSTR start,LPCWSTR x)
{
    if (x>start) return (LPWSTR)(x-1);
    else return (LPWSTR)x;
}


/***********************************************************************
 *           CharToOemA   (USER32.@)
 */
BOOL WINAPI CharToOemA( LPCSTR s, LPSTR d )
{
    if ( !s || !d ) return TRUE;
    return CharToOemBuffA( s, d, strlen( s ) + 1 );
}


/***********************************************************************
 *           CharToOemBuffA   (USER32.@)
 */
BOOL WINAPI CharToOemBuffA( LPCSTR s, LPSTR d, DWORD len )
{
    WCHAR *bufW;

    bufW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if( bufW )
    {
	MultiByteToWideChar( CP_ACP, 0, s, len, bufW, len );
	WideCharToMultiByte( CP_OEMCP, 0, bufW, len, d, len, NULL, NULL );
	HeapFree( GetProcessHeap(), 0, bufW );
    }
    return TRUE;
}


/***********************************************************************
 *           CharToOemBuffW   (USER32.@)
 */
BOOL WINAPI CharToOemBuffW( LPCWSTR s, LPSTR d, DWORD len )
{
   if ( !s || !d ) return TRUE;
    WideCharToMultiByte( CP_OEMCP, 0, s, len, d, len, NULL, NULL );
    return TRUE;
}


/***********************************************************************
 *           CharToOemW   (USER32.@)
 */
BOOL WINAPI CharToOemW( LPCWSTR s, LPSTR d )
{
    return CharToOemBuffW( s, d, strlenW( s ) + 1 );
}


/***********************************************************************
 *           OemToCharA   (USER32.@)
 */
BOOL WINAPI OemToCharA( LPCSTR s, LPSTR d )
{
    return OemToCharBuffA( s, d, strlen( s ) + 1 );
}


/***********************************************************************
 *           OemToCharBuffA   (USER32.@)
 */
BOOL WINAPI OemToCharBuffA( LPCSTR s, LPSTR d, DWORD len )
{
    WCHAR *bufW;

    bufW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if( bufW )
    {
	MultiByteToWideChar( CP_OEMCP, 0, s, len, bufW, len );
	WideCharToMultiByte( CP_ACP, 0, bufW, len, d, len, NULL, NULL );
	HeapFree( GetProcessHeap(), 0, bufW );
    }
    return TRUE;
}


/***********************************************************************
 *           OemToCharBuffW   (USER32.@)
 */
BOOL WINAPI OemToCharBuffW( LPCSTR s, LPWSTR d, DWORD len )
{
    MultiByteToWideChar( CP_OEMCP, 0, s, len, d, len );
    return TRUE;
}


/***********************************************************************
 *           OemToCharW   (USER32.@)
 */
BOOL WINAPI OemToCharW( LPCSTR s, LPWSTR d )
{
    return OemToCharBuffW( s, d, strlen( s ) + 1 );
}


/***********************************************************************
 *           CharLowerA   (USER32.@)
 * FIXME: handle current locale
 */
LPSTR WINAPI CharLowerA(LPSTR x)
{
    if (!HIWORD(x)) return (LPSTR)tolower((char)(int)x);

    __TRY
    {
        LPSTR s = x;
        while (*s)
        {
            *s=tolower(*s);
            s++;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    __ENDTRY
    return x;
}


/***********************************************************************
 *           CharUpperA   (USER32.@)
 * FIXME: handle current locale
 */
LPSTR WINAPI CharUpperA(LPSTR x)
{
    if (!HIWORD(x)) return (LPSTR)toupper((char)(int)x);

    __TRY
    {
        LPSTR s = x;
        while (*s)
        {
            *s=toupper(*s);
            s++;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    __ENDTRY
    return x;
}


/***********************************************************************
 *           CharLowerW   (USER32.@)
 */
LPWSTR WINAPI CharLowerW(LPWSTR x)
{
    if (HIWORD(x)) return strlwrW(x);
    else return (LPWSTR)((UINT)tolowerW(LOWORD(x)));
}


/***********************************************************************
 *           CharUpperW   (USER32.@)
 */
LPWSTR WINAPI CharUpperW(LPWSTR x)
{
    if (HIWORD(x)) return struprW(x);
    else return (LPWSTR)((UINT)toupperW(LOWORD(x)));
}


/***********************************************************************
 *           CharLowerBuffA   (USER32.@)
 */
DWORD WINAPI CharLowerBuffA( LPSTR str, DWORD len )
{
    DWORD lenW;
    WCHAR *strW;
    if (!str) return 0; /* YES */

    lenW = MultiByteToWideChar(CP_ACP, 0, str, len, NULL, 0);
    strW = HeapAlloc(GetProcessHeap(), 0, lenW * sizeof(WCHAR));
    if(strW)
    {
	MultiByteToWideChar(CP_ACP, 0, str, len, strW, lenW);
	CharLowerBuffW(strW, lenW);
	len = WideCharToMultiByte(CP_ACP, 0, strW, lenW, str, len, NULL, NULL);
	HeapFree(GetProcessHeap(), 0, strW);
	return len;
    }
    return 0;
}


/***********************************************************************
 *           CharLowerBuffW   (USER32.@)
 */
DWORD WINAPI CharLowerBuffW( LPWSTR str, DWORD len )
{
    DWORD ret = len;
    if (!str) return 0; /* YES */
    for (; len; len--, str++) *str = tolowerW(*str);
    return ret;
}


/***********************************************************************
 *           CharUpperBuffA   (USER32.@)
 */
DWORD WINAPI CharUpperBuffA( LPSTR str, DWORD len )
{
    DWORD lenW;
    WCHAR *strW;
    if (!str) return 0; /* YES */

    lenW = MultiByteToWideChar(CP_ACP, 0, str, len, NULL, 0);
    strW = HeapAlloc(GetProcessHeap(), 0, lenW * sizeof(WCHAR));
    if(strW)
    {
	MultiByteToWideChar(CP_ACP, 0, str, len, strW, lenW);
	CharUpperBuffW(strW, lenW);
	len = WideCharToMultiByte(CP_ACP, 0, strW, lenW, str, len, NULL, NULL);
	HeapFree(GetProcessHeap(), 0, strW);
	return len;
    }
    return 0;
}


/***********************************************************************
 *           CharUpperBuffW   (USER32.@)
 */
DWORD WINAPI CharUpperBuffW( LPWSTR str, DWORD len )
{
    DWORD ret = len;
    if (!str) return 0; /* YES */
    for (; len; len--, str++) *str = toupperW(*str);
    return ret;
}


/***********************************************************************
 *           IsCharLower    (USER.436)
 *           IsCharLowerA   (USER32.@)
 */
BOOL WINAPI IsCharLowerA(CHAR x)
{
    WCHAR wch;
    MultiByteToWideChar(CP_ACP, 0, &x, 1, &wch, 1);
    return IsCharLowerW(wch);
}


/***********************************************************************
 *           IsCharLowerW   (USER32.@)
 */
BOOL WINAPI IsCharLowerW(WCHAR x)
{
    return (get_char_typeW(x) & C1_LOWER) != 0;
}


/***********************************************************************
 *           IsCharUpper    (USER.435)
 *           IsCharUpperA   (USER32.@)
 */
BOOL WINAPI IsCharUpperA(CHAR x)
{
    WCHAR wch;
    MultiByteToWideChar(CP_ACP, 0, &x, 1, &wch, 1);
    return IsCharUpperW(wch);
}


/***********************************************************************
 *           IsCharUpperW   (USER32.@)
 */
BOOL WINAPI IsCharUpperW(WCHAR x)
{
    return (get_char_typeW(x) & C1_UPPER) != 0;
}


/***********************************************************************
 *           IsCharAlphaNumeric    (USER.434)
 *           IsCharAlphaNumericA   (USER32.@)
 */
BOOL WINAPI IsCharAlphaNumericA(CHAR x)
{
    WCHAR wch;
    MultiByteToWideChar(CP_ACP, 0, &x, 1, &wch, 1);
    return IsCharAlphaNumericW(wch);
}


/***********************************************************************
 *           IsCharAlphaNumericW   (USER32.@)
 */
BOOL WINAPI IsCharAlphaNumericW(WCHAR x)
{
    return (get_char_typeW(x) & (C1_ALPHA|C1_DIGIT)) != 0;
}


/***********************************************************************
 *           IsCharAlpha    (USER.433)
 *           IsCharAlphaA   (USER32.@)
 */
BOOL WINAPI IsCharAlphaA(CHAR x)
{
    WCHAR wch;
    MultiByteToWideChar(CP_ACP, 0, &x, 1, &wch, 1);
    return IsCharAlphaW(wch);
}


/***********************************************************************
 *           IsCharAlphaW   (USER32.@)
 */
BOOL WINAPI IsCharAlphaW(WCHAR x)
{
    return (get_char_typeW(x) & C1_ALPHA) != 0;
}


/***********************************************************************
 *           FormatMessage   (USER.606)
 */
DWORD WINAPI FormatMessage16(
    DWORD   dwFlags,
    SEGPTR lpSource,     /* [in] NOTE: not always a valid pointer */
    WORD   dwMessageId,
    WORD   dwLanguageId,
    LPSTR  lpBuffer,     /* [out] NOTE: *((HLOCAL16*)) for FORMAT_MESSAGE_ALLOCATE_BUFFER*/
    WORD   nSize,
    LPDWORD args         /* [in] NOTE: va_list *args */
) {
#ifdef __i386__
/* This implementation is completely dependant on the format of the va_list on x86 CPUs */
    LPSTR	target,t;
    DWORD	talloced;
    LPSTR	from,f;
    DWORD	width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
    BOOL	eos = FALSE;
    LPSTR       allocstring = NULL;

    TRACE("(0x%lx,%lx,%d,0x%x,%p,%d,%p)\n",
	  dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args);
	if ((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
		&& (dwFlags & FORMAT_MESSAGE_FROM_HMODULE)) return 0;
	if ((dwFlags & FORMAT_MESSAGE_FROM_STRING)
		&&((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
			|| (dwFlags & FORMAT_MESSAGE_FROM_HMODULE))) return 0;

    if (width && width != FORMAT_MESSAGE_MAX_WIDTH_MASK)
        FIXME("line wrapping (%lu) not supported.\n", width);
    from = NULL;
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING)
    {
        char *source = MapSL(lpSource);
        from = HeapAlloc( GetProcessHeap(), 0, strlen(source)+1 );
        strcpy( from, source );
    }
    if (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM) {
        from = HeapAlloc( GetProcessHeap(),0,200 );
	sprintf(from,"Systemmessage, messageid = 0x%08x\n",dwMessageId);
    }
    if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE) {
        INT16	bufsize;
	HINSTANCE16 hinst16 = ((HINSTANCE16)lpSource & 0xffff);

	dwMessageId &= 0xFFFF;
	bufsize=LoadString16(hinst16,dwMessageId,NULL,0);
	if (bufsize) {
	    from = HeapAlloc( GetProcessHeap(), 0, bufsize +1);
	    LoadString16(hinst16,dwMessageId,from,bufsize+1);
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
	while (*f && !eos) {
	    if (*f=='%') {
	        int	insertnr;
		char	*fmtstr,*x,*lastf;
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
			    fmtstr=HeapAlloc(GetProcessHeap(),0,strlen(f)+2);
			    sprintf(fmtstr,"%%%s",f);
			    f+=strlen(f); /*at \0*/
			}
		    }
                    else
                    {
                        if(!args) break;
                        fmtstr=HeapAlloc( GetProcessHeap(), 0, 3 );
                        strcpy( fmtstr, "%s" );
                    }
		    if (args) {
			int	ret;
		        int	sz;
			LPSTR	b = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sz = 100);

			argliststart=args+insertnr-1;

			/* CMF - This makes a BIG assumption about va_list */
			while ((ret = vsnprintf(b, sz, fmtstr, (va_list) argliststart) < 0) || (ret >= sz)) {
			    sz = (ret == -1 ? sz + 100 : ret + 1);
			    b = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, b, sz);
			}
			for (x=b; *x; x++) ADD_TO_T(*x);
                        HeapFree(GetProcessHeap(), 0, b);
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
	       	case '0': /* Just stop processing format string */
		    eos = TRUE;
		    f++;
		    break;
		case 'n': /* 16 bit version just outputs 'n' */
		default:
		    ADD_TO_T(*f++);
		    break;
		}
	    } else { /* '\n' or '\r' gets mapped to "\r\n" */
	        if(*f == '\n' || *f == '\r') {
		    if (width == 0) {
		        ADD_TO_T('\r');
		        ADD_TO_T('\n');
			if(*f++ == '\r' && *f == '\n')
			    f++;
		    }
		} else {
		    ADD_TO_T(*f++);
		}
	    }
	}
	*t='\0';
    }
    talloced = strlen(target)+1;
    if (nSize && talloced<nSize) {
        target = (char*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,nSize);
    }
    TRACE("-- %s\n",debugstr_a(target));
    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
        /* nSize is the MINIMUM size */
        HLOCAL16 h = LocalAlloc16(LPTR,talloced);
        SEGPTR ptr = LocalLock16(h);
	allocstring = MapSL( ptr );
	memcpy( allocstring,target,talloced);
        LocalUnlock16( h );
        *((HLOCAL16*)lpBuffer) = h;
    } else
        lstrcpynA(lpBuffer,target,nSize);
    HeapFree(GetProcessHeap(),0,target);
    if (from) HeapFree(GetProcessHeap(),0,from);
    return (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ?
        strlen(allocstring):
	strlen(lpBuffer);
#else
	return 0;
#endif /* __i386__ */
}
#undef ADD_TO_T
