/*
 * Url functions
 *
 * Copyright 2000 Huw D M Davies for CodeWeavers.
 */

#include <string.h>
#include "windef.h"
#include "winnls.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/unicode.h"
#include "wininet.h"
#include "winreg.h"
#include "shlwapi.h"
#include "debugtools.h"
#include "ordinal.h"

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

static BOOL URL_NeedEscapeA(CHAR ch, DWORD dwFlags)
{

    if (isalnum(ch))
        return FALSE;

    if(dwFlags & URL_ESCAPE_SPACES_ONLY) {
        if(ch == ' ')
	    return TRUE;
	else
	    return FALSE;
    }

    if ((dwFlags & URL_ESCAPE_PERCENT) && (ch == '%'))
	return TRUE;

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

	case '/':
	case '?':
	    if (dwFlags & URL_ESCAPE_SEGMENT_ONLY) return TRUE;
	default:
	    return FALSE;
	}
    }
}

static BOOL URL_NeedEscapeW(WCHAR ch, DWORD dwFlags)
{

    if (iswalnum(ch))
        return FALSE;

    if(dwFlags & URL_ESCAPE_SPACES_ONLY) {
        if(ch == L' ')
	    return TRUE;
	else
	    return FALSE;
    }

    if ((dwFlags & URL_ESCAPE_PERCENT) && (ch == L'%'))
	return TRUE;

    if (ch <= 31 || ch >= 127)
	return TRUE;

    else {
        switch (ch) {
	case L' ':
	case L'<':
	case L'>':
	case L'\"':
	case L'{':
	case L'}':
	case L'|':
	case L'\\':
	case L'^':
	case L']':
	case L'[':
	case L'`':
	case L'&':
	    return TRUE;

	case L'/':
	case L'?':
	    if (dwFlags & URL_ESCAPE_SEGMENT_ONLY) return TRUE;
	default:
	    return FALSE;
	}
    }
}

static BOOL URL_JustLocation(LPCWSTR str)
{
    while(*str && (*str == L'/')) str++;
    if (*str) {
	while (*str && ((*str == L'-') ||
			(*str == L'.') ||
			iswalnum(*str))) str++;
	if (*str == L'/') return FALSE;
    }
    return TRUE;
}


/*************************************************************************
 *        UrlCanonicalizeA     [SHLWAPI.@]
 *
 * Uses the W version to do job.
 */
HRESULT WINAPI UrlCanonicalizeA(LPCSTR pszUrl, LPSTR pszCanonicalized,
	LPDWORD pcchCanonicalized, DWORD dwFlags)
{
    LPWSTR base, canonical;
    DWORD ret, len, len2;

    TRACE("(%s %p %p 0x%08lx) using W version\n",
	  debugstr_a(pszUrl), pszCanonicalized,
	  pcchCanonicalized, dwFlags);

    base = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, 
			      (2*INTERNET_MAX_URL_LENGTH) * sizeof(WCHAR));
    canonical = base + INTERNET_MAX_URL_LENGTH;

    MultiByteToWideChar(0, 0, pszUrl, -1, base, INTERNET_MAX_URL_LENGTH);
    len = INTERNET_MAX_URL_LENGTH;

    ret = UrlCanonicalizeW(base, canonical, &len, dwFlags);
    if (ret != S_OK) {
	HeapFree(GetProcessHeap(), 0, base);
	return ret;
    }

    len2 = WideCharToMultiByte(0, 0, canonical, len, 0, 0, 0, 0);
    if (len2 > *pcchCanonicalized) {
	*pcchCanonicalized = len;
	HeapFree(GetProcessHeap(), 0, base);
	return E_POINTER;
    }
    WideCharToMultiByte(0, 0, canonical, len+1, pszCanonicalized, 
			*pcchCanonicalized, 0, 0);
    *pcchCanonicalized = len2;
    HeapFree(GetProcessHeap(), 0, base);
    return S_OK;
}

/*************************************************************************
 *        UrlCanonicalizeW     [SHLWAPI.@]
 *
 *
 * MSDN is wrong (at 10/30/01 - go figure). This should support the 
 * following flags:                                      GLA
 *    URL_DONT_ESCAPE_EXTRA_INFO    0x02000000
 *    URL_ESCAPE_SPACES_ONLY        0x04000000
 *    URL_ESCAPE_PERCENT            0x00001000
 *    URL_ESCAPE_UNSAFE             0x10000000
 *    URL_UNESCAPE                  0x10000000
 *    URL_DONT_SIMPLIFY             0x08000000
 *    URL_ESCAPE_SEGMENT_ONLY       0x00002000
 */
HRESULT WINAPI UrlCanonicalizeW(LPCWSTR pszUrl, LPWSTR pszCanonicalized, 
				LPDWORD pcchCanonicalized, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    DWORD EscapeFlags;
    LPWSTR lpszUrlCpy, wk1, wk2, mp, root;
    INT nLen, nByteLen, state;

    TRACE("(%s %p %p 0x%08lx)\n", debugstr_w(pszUrl), pszCanonicalized,
	  pcchCanonicalized, dwFlags);

    nByteLen = (lstrlenW(pszUrl) + 1) * sizeof(WCHAR); /* length in bytes */
    lpszUrlCpy = HeapAlloc(GetProcessHeap(), 0, nByteLen);

    if (dwFlags & URL_DONT_SIMPLIFY)
        memcpy(lpszUrlCpy, pszUrl, nByteLen);
    else {

	/*
	 * state =
	 *         0   initial  1,3
	 *         1   have 2[+] alnum  2,3
	 *         2   have scheme (found :)  4,6,3
	 *         3   failed (no location)
	 *         4   have //  5,3
	 *         5   have 1[+] alnum  6,3
	 *         6   have location (found /) save root location
	 */

	wk1 = (LPWSTR)pszUrl;
	wk2 = lpszUrlCpy;
	state = 0;
	while (*wk1) {
	    switch (state) {
	    case 0:
		if (!iswalnum(*wk1)) {state = 3; break;}
		*wk2++ = *wk1++;
		if (!iswalnum(*wk1)) {state = 3; break;}
		*wk2++ = *wk1++;
		state = 1;
		break;
	    case 1:
		*wk2++ = *wk1;
		if (*wk1++ == L':') state = 2;
		break;
	    case 2:
		if (*wk1 != L'/') {state = 3; break;}
		*wk2++ = *wk1++;
		if (*wk1 != L'/') {state = 6; break;}
		*wk2++ = *wk1++;
		state = 4;
		break;
	    case 3:
		strcpyW(wk2, wk1);
		wk1 += strlenW(wk1);
		wk2 += strlenW(wk2);
		break;
	    case 4:
		if (!iswalnum(*wk1) && (*wk1 != L'-')) {state = 3; break;}
		while(iswalnum(*wk1) || (*wk1 == L'-')) *wk2++ = *wk1++;
		state = 5;
		break;
	    case 5:
		if (*wk1 != L'/') {state = 3; break;}
		*wk2++ = *wk1++;
		state = 6;
		break;
	    case 6:
		/* Now at root location, cannot back up any more. */
		/* "root" will point at the '/' */
		root = wk2-1;
		while (*wk1) {
		    TRACE("wk1=%c\n", (CHAR)*wk1);
		    mp = strchrW(wk1, L'/');
		    if (!mp) {
			strcpyW(wk2, wk1);
			wk1 += strlenW(wk1);
			wk2 += strlenW(wk2);
			continue;
		    }
		    nLen = mp - wk1 + 1;
		    strncpyW(wk2, wk1, nLen);
		    wk2 += nLen;
		    wk1 += nLen;
		    if (*wk1 == L'.') {
			TRACE("found '/.'\n");
			if (*(wk1+1) == L'/') {
			    /* case of /./ -> skip the ./ */
			    wk1 += 2;
			}
			else if (*(wk1+1) == L'.') {
			    /* found /..  look for next / */
			    TRACE("found '/..'\n");
			    if (*(wk1+2) == L'/') {
				/* case /../ -> need to backup wk2 */
				TRACE("found '/../'\n");
				*(wk2-1) = L'\0';  /* set end of string */
				mp = strrchrW(root, L'/');
				if (mp && (mp >= root)) {
				    /* found valid backup point */
				    wk2 = mp + 1;
				    wk1 += 3;
				}
				else {
				    /* did not find point, restore '/' */
				    *(wk2-1) = L'/';
				}
			    }
			}
		    }
		}
		*wk2 = L'\0';
		break;
	    default:
		FIXME("how did we get here - state=%d\n", state);
		return E_INVALIDARG;
	    }
	}
	*wk2 = L'\0';
	TRACE("Simplified, orig <%s>, simple <%s>\n",
	      debugstr_w(pszUrl), debugstr_w(lpszUrlCpy));
    }

    if(dwFlags & URL_UNESCAPE)
        UrlUnescapeW(lpszUrlCpy, NULL, NULL, URL_UNESCAPE_INPLACE);

    if((EscapeFlags = dwFlags & (URL_ESCAPE_UNSAFE | 
                                 URL_ESCAPE_SPACES_ONLY |
                                 URL_ESCAPE_PERCENT |
                                 URL_DONT_ESCAPE_EXTRA_INFO |
				 URL_ESCAPE_SEGMENT_ONLY ))) {
	EscapeFlags &= ~URL_ESCAPE_UNSAFE;
	hr = UrlEscapeW(lpszUrlCpy, pszCanonicalized, pcchCanonicalized,
			EscapeFlags);
    } else { /* No escapping needed, just copy the string */
        nLen = lstrlenW(lpszUrlCpy);
	if(nLen < *pcchCanonicalized)
	    memcpy(pszCanonicalized, lpszUrlCpy, (nLen + 1)*sizeof(WCHAR));
	else {
	    hr = E_POINTER;
	    nLen++;
	}
	*pcchCanonicalized = nLen;
    }

    HeapFree(GetProcessHeap(), 0, lpszUrlCpy);
  
    if (hr == S_OK)
	TRACE("result %s\n", debugstr_w(pszCanonicalized));

    return hr;
}

/*************************************************************************
 *        UrlCombineA     [SHLWAPI.@]
 *
 * Uses the W version to do job.
 */
HRESULT WINAPI UrlCombineA(LPCSTR pszBase, LPCSTR pszRelative,
			   LPSTR pszCombined, LPDWORD pcchCombined,
			   DWORD dwFlags)
{
    LPWSTR base, relative, combined;
    DWORD ret, len, len2;

    TRACE("(base %s, Relative %s, Combine size %ld, flags %08lx) using W version\n",
	  debugstr_a(pszBase),debugstr_a(pszRelative),
	  *pcchCombined,dwFlags);

    base = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, 
			      (3*INTERNET_MAX_URL_LENGTH) * sizeof(WCHAR));
    relative = base + INTERNET_MAX_URL_LENGTH;
    combined = relative + INTERNET_MAX_URL_LENGTH;

    MultiByteToWideChar(0, 0, pszBase, -1, base, INTERNET_MAX_URL_LENGTH);
    MultiByteToWideChar(0, 0, pszRelative, -1, relative, INTERNET_MAX_URL_LENGTH);
    len = INTERNET_MAX_URL_LENGTH;

    ret = UrlCombineW(base, relative, combined, &len, dwFlags);
    if (ret != S_OK) {
	HeapFree(GetProcessHeap(), 0, base);
	return ret;
    }

    len2 = WideCharToMultiByte(0, 0, combined, len, 0, 0, 0, 0);
    if (len2 > *pcchCombined) {
	*pcchCombined = len;
	HeapFree(GetProcessHeap(), 0, base);
	return E_POINTER;
    }
    WideCharToMultiByte(0, 0, combined, len+1, pszCombined, *pcchCombined,
			0, 0);
    *pcchCombined = len2;
    HeapFree(GetProcessHeap(), 0, base);
    return S_OK;
}

/*************************************************************************
 *        UrlCombineW     [SHLWAPI.@]
 */
HRESULT WINAPI UrlCombineW(LPCWSTR pszBase, LPCWSTR pszRelative,
			   LPWSTR pszCombined, LPDWORD pcchCombined,
			   DWORD dwFlags)
{
    UNKNOWN_SHLWAPI_2 base, relative;
    DWORD myflags, sizeloc = 0;
    DWORD len, res1, res2, process_case = 0;
    LPWSTR work, preliminary, mbase, mrelative;
    WCHAR myfilestr[] = {'f','i','l','e',':','/','/','/','\0'};
    WCHAR single_slash[] = {'/','\0'};
    HRESULT ret;

    TRACE("(base %s, Relative %s, Combine size %ld, flags %08lx)\n",
	  debugstr_w(pszBase),debugstr_w(pszRelative),
	  *pcchCombined,dwFlags);

    base.size = 24;
    relative.size = 24;

    /* Get space for duplicates of the input and the output */
    preliminary = HeapAlloc(GetProcessHeap(), 0, (3*INTERNET_MAX_URL_LENGTH) *
			    sizeof(WCHAR));
    mbase = preliminary + INTERNET_MAX_URL_LENGTH;
    mrelative = mbase + INTERNET_MAX_URL_LENGTH;
    *preliminary = L'\0';

    /* Canonicalize the base input prior to looking for the scheme */
    myflags = dwFlags & (URL_DONT_SIMPLIFY | URL_UNESCAPE);
    len = INTERNET_MAX_URL_LENGTH;
    ret = UrlCanonicalizeW(pszBase, mbase, &len, myflags);

    /* Canonicalize the relative input prior to looking for the scheme */
    len = INTERNET_MAX_URL_LENGTH;
    ret = UrlCanonicalizeW(pszRelative, mrelative, &len, myflags);

    /* See if the base has a scheme */
    res1 = SHLWAPI_2(mbase, &base);
    if (res1) {
	/* if pszBase has no scheme, then return pszRelative */
	TRACE("no scheme detected in Base\n");
	process_case = 1;
    }
    else do {

	/* get size of location field (if it exists) */
	work = (LPWSTR)base.ap2;
	sizeloc = 0;
	if (*work++ == L'/') {
	    if (*work++ == L'/') {
		/* At this point have start of location and
		 * it ends at next '/' or end of string.
		 */
		while(*work && (*work != L'/')) work++;
		sizeloc = work - base.ap2;
	    }
	}

	/* Change .sizep2 to not have the last leaf in it,
	 * Note: we need to start after the location (if it exists)
	 */
	work = strrchrW((base.ap2+sizeloc), L'/');
	if (work) {
	    len = work - base.ap2 + 1;
	    base.sizep2 = len;
	}
	/*
	 * At this point:
	 *    .ap2      points to location (starting with '//')
	 *    .sizep2   length of location (above) and rest less the last
	 *              leaf (if any)
	 *    sizeloc   length of location (above) up to but not including 
	 *              the last '/'
	 */

	res2 = SHLWAPI_2(mrelative, &relative);
	if (res2) {
	    /* no scheme in pszRelative */
	    TRACE("no scheme detected in Relative\n");
	    relative.ap2 = mrelative;  /* case 3,4,5 depends on this */
	    relative.sizep2 = strlenW(mrelative);
	    if (*pszRelative  == L':') {
		/* case that is either left alone or uses pszBase */
		if (dwFlags & URL_PLUGGABLE_PROTOCOL) {
		    process_case = 5;
		    break;
		}
		process_case = 1;
		break;
	    }
	    if (isalnum(*mrelative) && (*(mrelative + 1) == L':')) {
		/* case that becomes "file:///" */
		strcpyW(preliminary, myfilestr);
		process_case = 1;
		break;
	    }
	    if ((*mrelative == L'/') && (*(mrelative+1) == L'/')) {
		/* pszRelative has location and rest */
		process_case = 3;
		break;
	    }
	    if (*mrelative == L'/') {
		/* case where pszRelative is root to location */
		process_case = 4;
		break;
	    }
	    process_case = (*base.ap2 == L'/') ? 5 : 3;
	    break;
	}

	/* handle cases where pszRelative has scheme */
	if ((base.sizep1 == relative.sizep1) && 
	    (strncmpW(base.ap1, relative.ap1, base.sizep1) == 0)) {

	    /* since the schemes are the same */
	    if ((*relative.ap2 == L'/') && (*(relative.ap2+1) == L'/')) {
		/* case where pszRelative replaces location and following */
		process_case = 3;
		break;
	    }
	    if (*relative.ap2 == L'/') {
		/* case where pszRelative is root to location */
		process_case = 4;
		break;
	    }
	    /* case where scheme is followed by document path */
	    process_case = 5;
	    break;
	}
	if ((*relative.ap2 == L'/') && (*(relative.ap2+1) == L'/')) {
	    /* case where pszRelative replaces scheme, location,
	     * and following and handles PLUGGABLE
	     */
	    process_case = 2;
	    break;
	}
	process_case = 1;
	break;
    } while(FALSE); /* a litte trick to allow easy exit from nested if's */


    ret = S_OK;
    switch (process_case) {

    case 1:  /*
	      * Return pszRelative appended to what ever is in pszCombined,
	      * (which may the string "file:///"
	      */
	len = strlenW(mrelative) + strlenW(preliminary);
	if (len+1 > *pcchCombined) {
	    *pcchCombined = len;
	    ret = E_POINTER;
	    break;
	} 
	strcatW(preliminary, mrelative);
	break;

    case 2:  /*
	      * Same as case 1, but if URL_PLUGGABLE_PROTOCOL was specified
	      * and pszRelative starts with "//", then append a "/"
	      */
	len = strlenW(mrelative) + 1;
	if (len+1 > *pcchCombined) {
	    *pcchCombined = len;
	    ret = E_POINTER;
	    break;
	} 
	strcpyW(preliminary, mrelative);
	if (!(dwFlags & URL_PLUGGABLE_PROTOCOL) &&
	    URL_JustLocation(relative.ap2))
	    strcatW(preliminary, single_slash);
	break;

    case 3:  /*
	      * Return the pszBase scheme with pszRelative. Basicly
	      * keeps the scheme and replaces the domain and following.
	      */
	len = base.sizep1 + 1 + relative.sizep2 + 1;
	if (len+1 > *pcchCombined) {
	    *pcchCombined = len;
	    ret = E_POINTER;
	    break;
	} 
	strncpyW(preliminary, base.ap1, base.sizep1 + 1);
	work = preliminary + base.sizep1 + 1;
	strcpyW(work, relative.ap2);
	if (!(dwFlags & URL_PLUGGABLE_PROTOCOL) &&
	    URL_JustLocation(relative.ap2))
	    strcatW(work, single_slash);
	break;

    case 4:  /*
	      * Return the pszBase scheme and location but everything
	      * after the location is pszRelative. (Replace document
	      * from root on.)
	      */
	len = base.sizep1 + 1 + sizeloc + relative.sizep2 + 1;
	if (len+1 > *pcchCombined) {
	    *pcchCombined = len;
	    ret = E_POINTER;
	    break;
	} 
	strncpyW(preliminary, base.ap1, base.sizep1+1+sizeloc);
	work = preliminary + base.sizep1 + 1 + sizeloc;
	if (dwFlags & URL_PLUGGABLE_PROTOCOL)
	    *(work++) = L'/';
	strcpyW(work, relative.ap2);
	break;

    case 5:  /*
	      * Return the pszBase without its document (if any) and 
	      * append pszRelative after its scheme.
	      */
	len = base.sizep1 + 1 + base.sizep2 + relative.sizep2;
	if (len+1 > *pcchCombined) {
	    *pcchCombined = len;
	    ret = E_POINTER;
	    break;
	} 
	strncpyW(preliminary, base.ap1, base.sizep1+1+base.sizep2);
	work = preliminary + base.sizep1+1+base.sizep2 - 1;
	if (*work++ != L'/')
	    *(work++) = L'/';
	strcpyW(work, relative.ap2);
	break;

    default:
	FIXME("How did we get here????? process_case=%ld\n", process_case);
	ret = E_INVALIDARG;
    }

    if (ret == S_OK) {
	/*
	 * Now that the combining is done, process the escape options if 
	 * necessary, otherwise just copy the string.
	 */
	myflags = dwFlags & (URL_ESCAPE_PERCENT |
			     URL_ESCAPE_SPACES_ONLY |
                             URL_DONT_ESCAPE_EXTRA_INFO |
			     URL_ESCAPE_SEGMENT_ONLY);
	if (myflags)
	    ret = UrlEscapeW(preliminary, pszCombined,
			     pcchCombined, myflags);
	else {
	    len = (strlenW(preliminary) + 1) * sizeof(WCHAR);
	    memcpy(pszCombined, preliminary, len);
	    *pcchCombined = strlenW(preliminary);
	}
	TRACE("return-%ld len=%ld, %s\n",
	      process_case, *pcchCombined, debugstr_w(pszCombined));
    }
    HeapFree(GetProcessHeap(), 0, preliminary);
    return ret;
}

/*************************************************************************
 *      UrlEscapeA	[SHLWAPI.@]
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
 *  Have now implemented the following flags:
 *     URL_ESCAPE_SPACES_ONLY
 *     URL_DONT_ESCAPE_EXTRA_INFO
 *     URL_ESCAPE_SEGMENT_ONLY
 *     URL_ESCAPE_PERCENT
 *  Initial testing seems to indicate that this is now working like
 *  native shlwapi version 5. Note that these functions did not work
 *  well (or at all) in shlwapi version 4.
 *
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

    if(dwFlags & ~(URL_ESCAPE_SPACES_ONLY |
		   URL_ESCAPE_SEGMENT_ONLY |
		   URL_DONT_ESCAPE_EXTRA_INFO |
		   URL_ESCAPE_PERCENT))
        FIXME("Unimplemented flags: %08lx\n", dwFlags);

    /* fix up flags */
    if (dwFlags & URL_ESCAPE_SPACES_ONLY)
	/* if SPACES_ONLY specified, reset the other controls */
	dwFlags &= ~(URL_DONT_ESCAPE_EXTRA_INFO |
		     URL_ESCAPE_PERCENT |
		     URL_ESCAPE_SEGMENT_ONLY);

    else
	/* if SPACES_ONLY *not* specified the assume DONT_ESCAPE_EXTRA_INFO */
	dwFlags |= URL_DONT_ESCAPE_EXTRA_INFO;

    for(src = pszUrl; *src; src++) {
        if(!(dwFlags & URL_ESCAPE_SEGMENT_ONLY) &&
	   (dwFlags & URL_DONT_ESCAPE_EXTRA_INFO) &&
	   (*src == '#' || *src == '?'))
	    stop_escapping = TRUE;

	if(URL_NeedEscapeA(*src, dwFlags) && stop_escapping == FALSE) {
	    /* TRACE("escaping %c\n", *src); */
	    next[0] = '%';
	    next[1] = hex[(*src >> 4) & 0xf];
	    next[2] = hex[*src & 0xf];
	    len = 3;
	} else {
	    /* TRACE("passing %c\n", *src); */
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
 *      UrlEscapeW	[SHLWAPI.@]
 *
 * See UrlEscapeA for list of assumptions, bugs, and FIXMEs
 */
HRESULT WINAPI UrlEscapeW(
	LPCWSTR pszUrl,
	LPWSTR pszEscaped,
	LPDWORD pcchEscaped,
	DWORD dwFlags)
{
    LPCWSTR src;
    DWORD needed = 0, ret;
    BOOL stop_escapping = FALSE;
    WCHAR next[5], *dst = pszEscaped;
    CHAR hex[] = "0123456789ABCDEF";
    INT len;

    TRACE("(%s %p %p 0x%08lx)\n", debugstr_w(pszUrl), pszEscaped,
	  pcchEscaped, dwFlags);

    if(dwFlags & ~(URL_ESCAPE_SPACES_ONLY |
		   URL_ESCAPE_SEGMENT_ONLY |
		   URL_DONT_ESCAPE_EXTRA_INFO |
		   URL_ESCAPE_PERCENT))
        FIXME("Unimplemented flags: %08lx\n", dwFlags);

    /* fix up flags */
    if (dwFlags & URL_ESCAPE_SPACES_ONLY)
	/* if SPACES_ONLY specified, reset the other controls */
	dwFlags &= ~(URL_DONT_ESCAPE_EXTRA_INFO |
		     URL_ESCAPE_PERCENT |
		     URL_ESCAPE_SEGMENT_ONLY);

    else
	/* if SPACES_ONLY *not* specified the assume DONT_ESCAPE_EXTRA_INFO */
	dwFlags |= URL_DONT_ESCAPE_EXTRA_INFO;

    for(src = pszUrl; *src; src++) {
	/*
	 * if(!(dwFlags & URL_ESCAPE_SPACES_ONLY) &&
	 *   (*src == L'#' || *src == L'?'))
	 *    stop_escapping = TRUE;
	 */
        if(!(dwFlags & URL_ESCAPE_SEGMENT_ONLY) &&
	   (dwFlags & URL_DONT_ESCAPE_EXTRA_INFO) &&
	   (*src == L'#' || *src == L'?'))
	    stop_escapping = TRUE;

	if(URL_NeedEscapeW(*src, dwFlags) && stop_escapping == FALSE) {
	    /* TRACE("escaping %c\n", *src); */
	    next[0] = L'%';
	    /*
	     * I would have assumed that the W form would escape
	     * the character with 4 hex digits (or even 8),
	     * however, experiments show that native shlwapi escapes
	     * with only 2 hex digits.
	     *   next[1] = hex[(*src >> 12) & 0xf];
	     *   next[2] = hex[(*src >> 8) & 0xf];
	     *   next[3] = hex[(*src >> 4) & 0xf];
	     *   next[4] = hex[*src & 0xf];
	     *   len = 5;
	     */
	    next[1] = hex[(*src >> 4) & 0xf];
	    next[2] = hex[*src & 0xf];
	    len = 3;
	} else {
	    /* TRACE("passing %c\n", *src); */
	    next[0] = *src;
	    len = 1;
	}

	if(needed + len <= *pcchEscaped) {
	    memcpy(dst, next, len*sizeof(WCHAR));
	    dst += len;
	}
	needed += len;
    }

    if(needed < *pcchEscaped) {
        *dst = L'\0';
	ret = S_OK;
    } else {
        needed++; /* add one for the '\0' */
	ret = E_POINTER;
    }
    *pcchEscaped = needed;
    return ret;
}


/*************************************************************************
 *      UrlUnescapeA	[SHLWAPI.@]
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

    TRACE("(%s, %p, %p, 0x%08lx)\n", debugstr_a(pszUrl), pszUnescaped,
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

    if (ret == S_OK) {
	TRACE("result %s\n", (dwFlags & URL_UNESCAPE_INPLACE) ? 
	      debugstr_a(pszUrl) : debugstr_a(pszUnescaped));
    }

    return ret;
}

/*************************************************************************
 *      UrlUnescapeW	[SHLWAPI.@]
 *
 * See UrlUnescapeA for list of assumptions, bugs, and FIXMEs
 */
HRESULT WINAPI UrlUnescapeW(
	LPCWSTR pszUrl,
	LPWSTR pszUnescaped,
	LPDWORD pcchUnescaped,
	DWORD dwFlags)
{
    WCHAR *dst, next;
    LPCWSTR src;
    HRESULT ret;
    DWORD needed;
    BOOL stop_unescapping = FALSE;

    TRACE("(%s, %p, %p, 0x%08lx)\n", debugstr_w(pszUrl), pszUnescaped,
	  pcchUnescaped, dwFlags);

    if(dwFlags & URL_UNESCAPE_INPLACE)
        dst = (WCHAR*)pszUrl;
    else
        dst = pszUnescaped;

    for(src = pszUrl, needed = 0; *src; src++, needed++) {
        if(dwFlags & URL_DONT_UNESCAPE_EXTRA_INFO &&
	   (*src == L'#' || *src == L'?')) {
	    stop_unescapping = TRUE;
	    next = *src;
	} else if(*src == L'%' && iswxdigit(*(src + 1)) && iswxdigit(*(src + 2))
		  && stop_unescapping == FALSE) {
	    INT ih;
	    WCHAR buf[3];
	    memcpy(buf, src + 1, 2*sizeof(WCHAR));
	    buf[2] = L'\0';
	    ih = wcstol(buf, NULL, 16);
	    next = (WCHAR) ih;
	    src += 2; /* Advance to end of escape */
	} else
	    next = *src;

	if(dwFlags & URL_UNESCAPE_INPLACE || needed < *pcchUnescaped)
	    *dst++ = next;
    }

    if(dwFlags & URL_UNESCAPE_INPLACE || needed < *pcchUnescaped) {
        *dst = L'\0';
	ret = S_OK;
    } else {
        needed++; /* add one for the '\0' */
	ret = E_POINTER;
    }
    if(!(dwFlags & URL_UNESCAPE_INPLACE))
        *pcchUnescaped = needed;

    if (ret == S_OK) {
	TRACE("result %s\n", (dwFlags & URL_UNESCAPE_INPLACE) ? 
	      debugstr_w(pszUrl) : debugstr_w(pszUnescaped));
    }

    return ret;
}

/*************************************************************************
 *      UrlGetLocationA 	[SHLWAPI.@]
 */
LPCSTR WINAPI UrlGetLocationA(
	LPCSTR pszUrl)
{
    FIXME("(%s): stub\n", debugstr_a(pszUrl));
    return 0;
}

/*************************************************************************
 *      UrlGetLocationW 	[SHLWAPI.@]
 */
LPCWSTR WINAPI UrlGetLocationW(
	LPCWSTR pszUrl)
{
    FIXME("(%s): stub\n", debugstr_w(pszUrl));
    return 0;
}

/*************************************************************************
 *      HashData	[SHLWAPI.@]
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
 *      UrlHashA	[SHLWAPI.@]
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

/*************************************************************************
 *      UrlApplySchemeW	[SHLWAPI.@]
 */
HRESULT WINAPI UrlApplySchemeW(LPCWSTR pszIn, LPWSTR pszOut, LPDWORD pcchOut, DWORD dwFlags)
{
    HRESULT err = NOERROR;
    FIXME("(%s %p %p %08lx): stub !\n", debugstr_w(pszIn), pszOut, pcchOut, dwFlags);
    strcpyW(pszOut, pszIn);
    *pcchOut = (err != E_POINTER) ? strlenW(pszOut) : 0;
    return err;
}

