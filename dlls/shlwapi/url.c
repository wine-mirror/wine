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

static const unsigned char HashDataLookup[256] = {
 0x01, 0x0E, 0x6E, 0x19, 0x61, 0xAE, 0x84, 0x77, 0x8A, 0xAA, 0x7D, 0x76, 0x1B,
 0xE9, 0x8C, 0x33, 0x57, 0xC5, 0xB1, 0x6B, 0xEA, 0xA9, 0x38, 0x44, 0x1E, 0x07,
 0xAD, 0x49, 0xBC, 0x28, 0x24, 0x41, 0x31, 0xD5, 0x68, 0xBE, 0x39, 0xD3, 0x94,
 0xDF, 0x30, 0x73, 0x0F, 0x02, 0x43, 0xBA, 0xD2, 0x1C, 0x0C, 0xB5, 0x67, 0x46,
 0x16, 0x3A, 0x4B, 0x4E, 0xB7, 0xA7, 0xEE, 0x9D, 0x7C, 0x93, 0xAC, 0x90, 0xB0,
 0xA1, 0x8D, 0x56, 0x3C, 0x42, 0x80, 0x53, 0x9C, 0xF1, 0x4F, 0x2E, 0xA8, 0xC6,
 0x29, 0xFE, 0xB2, 0x55, 0xFD, 0xED, 0xFA, 0x9A, 0x85, 0x58, 0x23, 0xCE, 0x5F,
 0x74, 0xFC, 0xC0, 0x36, 0xDD, 0x66, 0xDA, 0xFF, 0xF0, 0x52, 0x6A, 0x9E, 0xC9,
 0x3D, 0x03, 0x59, 0x09, 0x2A, 0x9B, 0x9F, 0x5D, 0xA6, 0x50, 0x32, 0x22, 0xAF,
 0xC3, 0x64, 0x63, 0x1A, 0x96, 0x10, 0x91, 0x04, 0x21, 0x08, 0xBD, 0x79, 0x40,
 0x4D, 0x48, 0xD0, 0xF5, 0x82, 0x7A, 0x8F, 0x37, 0x69, 0x86, 0x1D, 0xA4, 0xB9,
 0xC2, 0xC1, 0xEF, 0x65, 0xF2, 0x05, 0xAB, 0x7E, 0x0B, 0x4A, 0x3B, 0x89, 0xE4,
 0x6C, 0xBF, 0xE8, 0x8B, 0x06, 0x18, 0x51, 0x14, 0x7F, 0x11, 0x5B, 0x5C, 0xFB,
 0x97, 0xE1, 0xCF, 0x15, 0x62, 0x71, 0x70, 0x54, 0xE2, 0x12, 0xD6, 0xC7, 0xBB,
 0x0D, 0x20, 0x5E, 0xDC, 0xE0, 0xD4, 0xF7, 0xCC, 0xC4, 0x2B, 0xF9, 0xEC, 0x2D,
 0xF4, 0x6F, 0xB6, 0x99, 0x88, 0x81, 0x5A, 0xD9, 0xCA, 0x13, 0xA5, 0xE7, 0x47,
 0xE6, 0x8E, 0x60, 0xE3, 0x3E, 0xB3, 0xF6, 0x72, 0xA2, 0x35, 0xA0, 0xD7, 0xCD,
 0xB4, 0x2F, 0x6D, 0x2C, 0x26, 0x1F, 0x95, 0x87, 0x00, 0xD8, 0x34, 0x3F, 0x17,
 0x25, 0x45, 0x27, 0x75, 0x92, 0xB8, 0xA3, 0xC8, 0xDE, 0xEB, 0xF8, 0xF3, 0xDB,
 0x0A, 0x98, 0x83, 0x7B, 0xE5, 0xCB, 0x4C, 0x78, 0xD1 };

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

/*************************************************************************
 *      HashData	[SHLWAPI]
 *
 * Hash an input block into a variable sized digest.
 */
BOOL WINAPI HashData(const unsigned char *lpSrc, INT nSrcLen,
                     unsigned char *lpDest, INT nDestLen)
{
  INT srcCount = nSrcLen - 1, destCount = nDestLen - 1;

  if (IsBadReadPtr(lpSrc, nSrcLen) ||
      IsBadWritePtr(lpDest, nDestLen))
    return FALSE;

  while (destCount >= 0)
  {
    lpDest[destCount] = (destCount & 0xff);
    destCount--;
  }

  while (srcCount >= 0)
  {
    destCount = nDestLen - 1;
    while (destCount >= 0)
    {
      lpDest[destCount] = HashDataLookup[lpSrc[srcCount] ^ lpDest[destCount]];
      destCount--;
    }
    srcCount--;
  }
  return TRUE;
}

/*************************************************************************
 *      UrlHashA	[SHLWAPI]
 *
 * Hash an ASCII URL.
 */
HRESULT WINAPI UrlHashA(LPCSTR pszUrl, unsigned char *lpDest, INT nDestLen)
{
  if (IsBadStringPtrA(pszUrl, -1) || IsBadWritePtr(lpDest, nDestLen))
    return E_INVALIDARG;

  HashData(pszUrl, strlen(pszUrl), lpDest, nDestLen);
  return NOERROR;
}

