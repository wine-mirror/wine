/*
 * Url functions
 *
 * Copyright 2000 Huw D M Davies for CodeWeavers.
 */

#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "shlwapi.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(shell);

static BOOL URL_NeedEscape(CHAR ch, DWORD dwFlags)
{

    if (isalnum(ch))
        return FALSE;

    if(dwFlags & URL_ESCAPE_SPACES_ONLY) {
        if(ch == ' ')
	    return TRUE;
	else
	    return FALSE;
    }

    if (ch <= 31 || ch >= 127)
	return TRUE; 

    else {
        switch (ch) {
	case ' ':
	case '<':
	case '>':
	case '\"':
	case '{':
	case '}':
	case '|':
	case '\\':
	case '^':
	case ']':
	case '[':
	case '`':
	case '&':
	    return TRUE;

	default:
	    return FALSE;
	}
    }
}

/*************************************************************************
 *        UrlCanonicalizeA     [SHLWAPI]
 */
HRESULT WINAPI UrlCanonicalizeA(LPCSTR pszUrl, LPSTR pszCanonicalized, 
	LPDWORD pcchCanonicalized, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    LPSTR lpszUrlCpy;
    INT nLen;

    TRACE("(%s %p %p 0x%08lx)\n", debugstr_a(pszUrl), pszCanonicalized,
	  pcchCanonicalized, dwFlags);

    nLen = strlen(pszUrl);
    lpszUrlCpy = HeapAlloc(GetProcessHeap(), 0, nLen + 1);
       
    if (dwFlags & URL_DONT_SIMPLIFY)
        memcpy(lpszUrlCpy, pszUrl, nLen + 1);
    else {
        FIXME("Simplify path\n");
        memcpy(lpszUrlCpy, pszUrl, nLen + 1);
    }

    if(dwFlags & URL_UNESCAPE)
        UrlUnescapeA(lpszUrlCpy, NULL, NULL, URL_UNESCAPE_INPLACE);

    if(dwFlags & (URL_ESCAPE_UNSAFE | URL_ESCAPE_SPACES_ONLY)) {
        DWORD EscapeFlags = dwFlags & (URL_ESCAPE_SPACES_ONLY
				       /* | URL_ESCAPE_PERCENT */);
	hr = UrlEscapeA(lpszUrlCpy, pszCanonicalized, pcchCanonicalized,
			EscapeFlags);
    } else { /* No escapping needed, just copy the string */
        nLen = strlen(lpszUrlCpy);
	if(nLen < *pcchCanonicalized)
	    memcpy(pszCanonicalized, lpszUrlCpy, nLen + 1);
	else {
	    hr = E_POINTER;
	    nLen++;
	}
	*pcchCanonicalized = nLen;
    }

    HeapFree(GetProcessHeap(), 0, lpszUrlCpy);
  
    return hr;
}

/*************************************************************************
 *        UrlCanonicalizeW     [SHLWAPI]
 */
HRESULT WINAPI UrlCanonicalizeW(LPCWSTR pszUrl, LPWSTR pszCanonicalized, 
				LPDWORD pcchCanonicalized, DWORD dwFlags)
{
    FIXME("(%s %p %p 0x%08lx): stub\n",debugstr_w(pszUrl),
	  pszCanonicalized, pcchCanonicalized, dwFlags);
    return E_NOTIMPL;
}

/*************************************************************************
 *      UrlEscapeA	[SHLWAPI]
 *
 * Converts unsafe characters into their escape sequences.
 *
 * The converted string is returned in pszEscaped if the buffer size
 * (which should be supplied in pcchEscaped) is large enough, in this
 * case the function returns S_OK and pcchEscaped contains the length
 * of the escaped string.  If the buffer is not large enough the
 * function returns E_POINTER and pcchEscaped contains the required
 * buffer size (including room for the '\0').
 *
 * By default the function stops converting at the first '?' or
 * '#'. [MSDN says differently].  If URL_ESCAPE_SPACE_ONLY flag is set
 * then only spaces are converted, but the conversion continues past a
 * '?' or '#'.
 *
 * BUGS:
 *
 * None of the URL_ define values are documented, so they were
 * determined by trial and error.  MSDN mentions URL_ESCAPE_PERCENT
 * but I can't find a value that does this under win2000.
 * URL_DONT_ESCAPE_EXTRA_INFO appears to be the default which is what
 * we assume here.  URL_ESCAPE_SEGMENT_ONLY is not implemented
 * (value??).  A value of 0x2000 for dwFlags seems to escape
 * '/'s too - this is neither documented on MSDN nor implemented here.
 * For character values that are converted see URL_NeedEscape.
 */
HRESULT WINAPI UrlEscapeA(
	LPCSTR pszUrl,
	LPSTR pszEscaped,
	LPDWORD pcchEscaped,
	DWORD dwFlags)
{
    LPCSTR src;
    DWORD needed = 0, ret;
    BOOL stop_escapping = FALSE;
    char next[3], *dst = pszEscaped;
    char hex[] = "0123456789ABCDEF";
    INT len;

    TRACE("(%s %p %p 0x%08lx)\n", debugstr_a(pszUrl), pszEscaped,
	  pcchEscaped, dwFlags);

    if(dwFlags & ~URL_ESCAPE_SPACES_ONLY)
        FIXME("Unimplemented flags: %08lx\n", dwFlags);

    for(src = pszUrl; *src; src++) {
        if(!(dwFlags & URL_ESCAPE_SPACES_ONLY) &&
	   (*src == '#' || *src == '?'))
	    stop_escapping = TRUE;

	if(URL_NeedEscape(*src, dwFlags) && stop_escapping == FALSE) {
	    next[0] = '%';
	    next[1] = hex[(*src >> 4) & 0xf];
	    next[2] = hex[*src & 0xf];
	    len = 3;
	} else {
	    next[0] = *src;
	    len = 1;
	}

	if(needed + len <= *pcchEscaped) {
	    memcpy(dst, next, len);
	    dst += len;
	}
	needed += len;
    }

    if(needed < *pcchEscaped) {
        *dst = '\0';
	ret = S_OK;
    } else {
        needed++; /* add one for the '\0' */
	ret = E_POINTER;
    }
    *pcchEscaped = needed;
    return ret;
}	

/*************************************************************************
 *      UrlEscapeW	[SHLWAPI]
 */
HRESULT WINAPI UrlEscapeW(
	LPCWSTR pszUrl,
	LPWSTR pszEscaped,
	LPDWORD pcchEscaped,
	DWORD dwFlags)
{
    FIXME("(%s %p %p 0x%08lx): stub\n",debugstr_w(pszUrl),
	  pszEscaped, pcchEscaped, dwFlags);
    return E_NOTIMPL;
}	


/*************************************************************************
 *      UrlUnescapeA	[SHLWAPI]
 *
 * Converts escape sequences back to ordinary characters.
 * 
 * If URL_ESCAPE_INPLACE is set in dwFlags then pszUnescaped and
 * pcchUnescaped are ignored and the converted string is returned in
 * pszUrl, otherwise the string is returned in pszUnescaped.
 * pcchUnescaped should contain the size of pszUnescaped on calling
 * and will contain the length the the returned string on return if
 * the buffer is big enough else it will contain the buffer size
 * required (including room for the '\0').  The function returns S_OK
 * on success or E_POINTER if the buffer is not large enough.  If the
 * URL_DONT_ESCAPE_EXTRA_INFO flag is set then the conversion stops at
 * the first occurrence of either '?' or '#'.
 *
 */
HRESULT WINAPI UrlUnescapeA(
	LPCSTR pszUrl,
	LPSTR pszUnescaped,
	LPDWORD pcchUnescaped,
	DWORD dwFlags)
{
    char *dst, next;
    LPCSTR src;
    HRESULT ret;
    DWORD needed;
    BOOL stop_unescapping = FALSE;

    TRACE("(%s, %p, %p, %08lx): stub\n", debugstr_a(pszUrl), pszUnescaped,
	  pcchUnescaped, dwFlags);

    if(dwFlags & URL_UNESCAPE_INPLACE)
        dst = (char*)pszUrl;
    else
        dst = pszUnescaped;

    for(src = pszUrl, needed = 0; *src; src++, needed++) {
        if(dwFlags & URL_DONT_UNESCAPE_EXTRA_INFO &&
	   (*src == '#' || *src == '?')) {
	    stop_unescapping = TRUE;
	    next = *src;
	} else if(*src == '%' && isxdigit(*(src + 1)) && isxdigit(*(src + 2))
		  && stop_unescapping == FALSE) {
	    INT ih;
	    char buf[3];
	    memcpy(buf, src + 1, 2);
	    buf[2] = '\0';
	    ih = strtol(buf, NULL, 16);
	    next = (CHAR) ih;
	    src += 2; /* Advance to end of escape */
	} else
	    next = *src;

	if(dwFlags & URL_UNESCAPE_INPLACE || needed < *pcchUnescaped)
	    *dst++ = next;
    }

    if(dwFlags & URL_UNESCAPE_INPLACE || needed < *pcchUnescaped) {
        *dst = '\0';
	ret = S_OK;
    } else {
        needed++; /* add one for the '\0' */
	ret = E_POINTER;
    }
    if(!(dwFlags & URL_UNESCAPE_INPLACE))
        *pcchUnescaped = needed;

    return ret;
}

/*************************************************************************
 *      UrlUnescapeW	[SHLWAPI]
 */
HRESULT WINAPI UrlUnescapeW(
	LPCWSTR pszUrl,
	LPWSTR pszUnescaped,
	LPDWORD pcchUnescaped,
	DWORD dwFlags)
{
    FIXME("(%s, %p, %p, %08lx): stub\n", debugstr_w(pszUrl), pszUnescaped,
	  pcchUnescaped, dwFlags);
    return E_NOTIMPL;
}
