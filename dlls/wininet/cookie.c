/*
 * Wininet - cookie handling stuff
 *
 * Copyright 2002 TransGaming Technologies Inc.
 *
 * David Hammerton
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winerror.h"

#include "wine/debug.h"
#include "internet.h"

#define RESPONSE_TIMEOUT        30            /* FROM internet.c */


WINE_DEFAULT_DEBUG_CHANNEL(wininet);

/* FIXME
 *     Cookies are currently memory only.
 *     Cookies are NOT THREAD SAFE
 *     Cookies could use ALOT OF MEMORY. We need some kind of memory management here!
 *     Cookies should care about the expiry time
 */

typedef struct _cookie_domain cookie_domain;
typedef struct _cookie cookie;

struct _cookie
{
    struct _cookie *next;
    struct _cookie *prev;

    struct _cookie_domain *parent;

    LPWSTR lpCookieName;
    LPWSTR lpCookieData;
    time_t expiry; /* FIXME: not used */
};

struct _cookie_domain
{
    struct _cookie_domain *next;
    struct _cookie_domain *prev;

    LPWSTR lpCookieDomain;
    LPWSTR lpCookiePath;
    cookie *cookie_tail;
};

static cookie_domain *cookieDomainTail;

static cookie *COOKIE_addCookie(cookie_domain *domain, LPCWSTR name, LPCWSTR data);
static cookie *COOKIE_findCookie(cookie_domain *domain, LPCWSTR lpszCookieName);
static void COOKIE_deleteCookie(cookie *deadCookie, BOOL deleteDomain);
static cookie_domain *COOKIE_addDomain(LPCWSTR domain, LPCWSTR path);
static cookie_domain *COOKIE_addDomainFromUrl(LPCWSTR lpszUrl);
static cookie_domain *COOKIE_findNextDomain(LPCWSTR lpszCookieDomain, LPCWSTR lpszCookiePath,
                                            cookie_domain *prev_domain, BOOL allow_partial);
static cookie_domain *COOKIE_findNextDomainFromUrl(LPCWSTR lpszUrl, cookie_domain *prev_domain,
                                                   BOOL allow_partial);
static void COOKIE_deleteDomain(cookie_domain *deadDomain);


/* adds a cookie to the domain */
static cookie *COOKIE_addCookie(cookie_domain *domain, LPCWSTR name, LPCWSTR data)
{
    cookie *newCookie = HeapAlloc(GetProcessHeap(), 0, sizeof(cookie));

    newCookie->next = NULL;
    newCookie->prev = NULL;
    newCookie->lpCookieName = NULL;
    newCookie->lpCookieData = NULL;

    if (name)
    {
	newCookie->lpCookieName = HeapAlloc(GetProcessHeap(), 0, (strlenW(name) + 1)*sizeof(WCHAR));
        lstrcpyW(newCookie->lpCookieName, name);
    }
    if (data)
    {
	newCookie->lpCookieData = HeapAlloc(GetProcessHeap(), 0, (strlenW(data) + 1)*sizeof(WCHAR));
        lstrcpyW(newCookie->lpCookieData, data);
    }

    TRACE("added cookie %p (data is %s)\n", newCookie, debugstr_w(data) );

    newCookie->prev = domain->cookie_tail;
    newCookie->parent = domain;
    domain->cookie_tail = newCookie;
    return newCookie;
}


/* finds a cookie in the domain matching the cookie name */
static cookie *COOKIE_findCookie(cookie_domain *domain, LPCWSTR lpszCookieName)
{
    cookie *searchCookie = domain->cookie_tail;
    TRACE("(%p, %s)\n", domain, debugstr_w(lpszCookieName));

    while (searchCookie)
    {
	BOOL candidate = TRUE;
	if (candidate && lpszCookieName)
	{
	    if (candidate && !searchCookie->lpCookieName)
		candidate = FALSE;
	    if (candidate && strcmpW(lpszCookieName, searchCookie->lpCookieName) != 0)
                candidate = FALSE;
	}
	if (candidate)
	    return searchCookie;
        searchCookie = searchCookie->prev;
    }
    return NULL;
}

/* removes a cookie from the list, if its the last cookie we also remove the domain */
static void COOKIE_deleteCookie(cookie *deadCookie, BOOL deleteDomain)
{
    if (deadCookie->lpCookieName)
	HeapFree(GetProcessHeap(), 0, deadCookie->lpCookieName);
    if (deadCookie->lpCookieData)
	HeapFree(GetProcessHeap(), 0, deadCookie->lpCookieData);
    if (deadCookie->prev)
        deadCookie->prev->next = deadCookie->next;
    if (deadCookie->next)
	deadCookie->next->prev = deadCookie->prev;

    if (deadCookie == deadCookie->parent->cookie_tail)
    {
	/* special case: last cookie, lets remove the domain to save memory */
	deadCookie->parent->cookie_tail = deadCookie->prev;
        if (!deadCookie->parent->cookie_tail && deleteDomain)
	    COOKIE_deleteDomain(deadCookie->parent);
    }
}

/* allocates a domain and adds it to the end */
static cookie_domain *COOKIE_addDomain(LPCWSTR domain, LPCWSTR path)
{
    cookie_domain *newDomain = HeapAlloc(GetProcessHeap(), 0, sizeof(cookie_domain));

    newDomain->next = NULL;
    newDomain->prev = NULL;
    newDomain->cookie_tail = NULL;
    newDomain->lpCookieDomain = NULL;
    newDomain->lpCookiePath = NULL;

    if (domain)
    {
	newDomain->lpCookieDomain = HeapAlloc(GetProcessHeap(), 0, (strlenW(domain) + 1)*sizeof(WCHAR));
        strcpyW(newDomain->lpCookieDomain, domain);
    }
    if (path)
    {
	newDomain->lpCookiePath = HeapAlloc(GetProcessHeap(), 0, (strlenW(path) + 1)*sizeof(WCHAR));
        lstrcpyW(newDomain->lpCookiePath, path);
    }

    newDomain->prev = cookieDomainTail;
    cookieDomainTail = newDomain;
    TRACE("Adding domain: %p\n", newDomain);
    return newDomain;
}

static cookie_domain *COOKIE_addDomainFromUrl(LPCWSTR lpszUrl)
{
    WCHAR hostName[2048], path[2048];
    URL_COMPONENTSW UrlComponents;

    UrlComponents.lpszExtraInfo = NULL;
    UrlComponents.lpszPassword = NULL;
    UrlComponents.lpszScheme = NULL;
    UrlComponents.lpszUrlPath = path;
    UrlComponents.lpszUserName = NULL;
    UrlComponents.lpszHostName = hostName;
    UrlComponents.dwHostNameLength = 2048;
    UrlComponents.dwUrlPathLength = 2048;

    InternetCrackUrlW(lpszUrl, 0, 0, &UrlComponents);

    TRACE("Url cracked. Domain: %s, Path: %s.\n", debugstr_w(UrlComponents.lpszHostName),
	  debugstr_w(UrlComponents.lpszUrlPath));

    /* hack for now - FIXME - There seems to be a bug in InternetCrackUrl?? */
    UrlComponents.lpszUrlPath = NULL;

    return COOKIE_addDomain(UrlComponents.lpszHostName, UrlComponents.lpszUrlPath);
}

/* find a domain. domain must match if its not NULL. path must match if its not NULL */
static cookie_domain *COOKIE_findNextDomain(LPCWSTR lpszCookieDomain, LPCWSTR lpszCookiePath,
                                            cookie_domain *prev_domain, BOOL allow_partial)
{
    cookie_domain *searchDomain;

    if (prev_domain)
    {
	if(!prev_domain->prev)
	{
	    TRACE("no more domains available, it would seem.\n");
            return NULL;
	}
	searchDomain = prev_domain->prev;
    }
    else searchDomain = cookieDomainTail;

    while (searchDomain)
    {
	BOOL candidate = TRUE;
        TRACE("searching on domain %p\n", searchDomain);
	if (candidate && lpszCookieDomain)
	{
	    if (candidate && !searchDomain->lpCookieDomain)
		candidate = FALSE;
            TRACE("candidate! (%p)\n", searchDomain->lpCookieDomain);
	    TRACE("comparing domain %s with %s\n", 
                  debugstr_w(lpszCookieDomain), 
                  debugstr_w(searchDomain->lpCookieDomain));
	    if (candidate && allow_partial && !strstrW(lpszCookieDomain, searchDomain->lpCookieDomain))
                candidate = FALSE;
	    else if (candidate && !allow_partial &&
		     lstrcmpW(lpszCookieDomain, searchDomain->lpCookieDomain) != 0)
                candidate = FALSE;
 	}
	if (candidate && lpszCookiePath)
	{
            TRACE("comparing paths\n");
	    if (candidate && !searchDomain->lpCookiePath)
                candidate = FALSE;
	    if (candidate && 
                strcmpW(lpszCookiePath, searchDomain->lpCookiePath))
                candidate = FALSE;
	}
	if (candidate)
	{
            TRACE("returning the domain %p\n", searchDomain);
	    return searchDomain;
	}
	searchDomain = searchDomain->prev;
    }
    TRACE("found no domain, returning NULL\n");
    return NULL;
}

static cookie_domain *COOKIE_findNextDomainFromUrl(LPCWSTR lpszUrl, cookie_domain *previous_domain,
                                                   BOOL allow_partial)
{
    WCHAR hostName[2048], path[2048];
    URL_COMPONENTSW UrlComponents;

    UrlComponents.lpszExtraInfo = NULL;
    UrlComponents.lpszPassword = NULL;
    UrlComponents.lpszScheme = NULL;
    UrlComponents.lpszUrlPath = path;
    UrlComponents.lpszUserName = NULL;
    UrlComponents.lpszHostName = hostName;
    UrlComponents.dwHostNameLength = 2048;
    UrlComponents.dwUrlPathLength = 2048;

    InternetCrackUrlW(lpszUrl, 0, 0, &UrlComponents);

    TRACE("Url cracked. Domain: %s, Path: %s.\n",
          debugstr_w(UrlComponents.lpszHostName),
	  debugstr_w(UrlComponents.lpszUrlPath));

    /* hack for now - FIXME - There seems to be a bug in InternetCrackUrl?? */
    UrlComponents.lpszUrlPath = NULL;

    return COOKIE_findNextDomain(UrlComponents.lpszHostName, UrlComponents.lpszUrlPath,
				 previous_domain, allow_partial);
}

/* remove a domain from the list and delete it */
static void COOKIE_deleteDomain(cookie_domain *deadDomain)
{
    while (deadDomain->cookie_tail)
	COOKIE_deleteCookie(deadDomain->cookie_tail, FALSE);
    if (deadDomain->lpCookieDomain)
	HeapFree(GetProcessHeap(), 0, deadDomain->lpCookieDomain);
    if (deadDomain->lpCookiePath)
	HeapFree(GetProcessHeap(), 0, deadDomain->lpCookiePath);
    if (deadDomain->prev)
	deadDomain->prev->next = deadDomain->next;
    if (deadDomain->next)
	deadDomain->next->prev = deadDomain->prev;

    if (cookieDomainTail == deadDomain)
	cookieDomainTail = deadDomain->prev;
    HeapFree(GetProcessHeap(), 0, deadDomain);
}

/***********************************************************************
 *           InternetGetCookieW (WININET.@)
 *
 * Retrieve cookie from the specified url
 *
 *  It should be noted that on windows the lpszCookieName parameter is "not implemented".
 *    So it won't be implemented here.
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetGetCookieW(LPCWSTR lpszUrl, LPCWSTR lpszCookieName,
    LPWSTR lpCookieData, LPDWORD lpdwSize)
{
    cookie_domain *cookiesDomain = NULL;
    cookie *thisCookie;
    int cnt = 0, domain_count = 0;
    /* Ok, this is just ODD!. During my tests, it appears M$ like to send out
     * a cookie called 'MtrxTracking' to some urls. Also returns it from InternetGetCookie.
     * I'm not exactly sure what to make of this, so its here for now.
     * It'd be nice to know what exactly is going on, M$ tracking users? Does this need
     * to be unique? Should I generate a random number here? etc.
     */
    WCHAR TrackingString[] = {
        'M','t','r','x','T','r','a','c','k','i','n','g','I','D','=',
        '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5',
        '6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1', 0 };
    WCHAR szps[] = { '%','s',0 };

    TRACE("(%s, %s, %p, %p)\n", debugstr_w(lpszUrl),debugstr_w(lpszCookieName),
	  lpCookieData, lpdwSize);

    if (lpCookieData)
	cnt += snprintfW(lpCookieData + cnt, *lpdwSize - cnt, szps, TrackingString);
    else
	cnt += strlenW(TrackingString);

    while ((cookiesDomain = COOKIE_findNextDomainFromUrl(lpszUrl, cookiesDomain, TRUE)))
    {
        domain_count++;
	TRACE("found domain %p\n", cookiesDomain);

	thisCookie = cookiesDomain->cookie_tail;
	if (lpCookieData == NULL) /* return the size of the buffer required to lpdwSize */
	{
	    while (thisCookie)
	    {
		cnt += 2; /* '; ' */
		cnt += strlenW(thisCookie->lpCookieName);
		cnt += 1; /* = */
		cnt += strlenW(thisCookie->lpCookieData);

		thisCookie = thisCookie->prev;
	    }
	}
	while (thisCookie)
	{
            WCHAR szsc[] = { ';',' ',0 };
            WCHAR szpseq[] = { '%','s','=','%','s',0 };
            cnt += snprintfW(lpCookieData + cnt, *lpdwSize - cnt, szsc);
	    cnt += snprintfW(lpCookieData + cnt, *lpdwSize - cnt, szpseq,
                            thisCookie->lpCookieName,
			    thisCookie->lpCookieData);

	    thisCookie = thisCookie->prev;
	}
    }
    if (lpCookieData == NULL)
    {
	cnt += 1; /* NULL */
	*lpdwSize = cnt;
	TRACE("returning\n");
	return TRUE;
    }

    if (!domain_count)
        return FALSE;

    *lpdwSize = cnt + 1;

    TRACE("Returning %i (from %i domains): %s\n", cnt, domain_count,
           debugstr_w(lpCookieData));

    return (cnt ? TRUE : FALSE);
}


/***********************************************************************
 *           InternetGetCookieA (WININET.@)
 *
 * Retrieve cookie from the specified url
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetGetCookieA(LPCSTR lpszUrl, LPCSTR lpszCookieName,
    LPSTR lpCookieData, LPDWORD lpdwSize)
{
    DWORD len;
    LPWSTR szCookieData = NULL, szUrl = NULL, szCookieName = NULL;
    BOOL r;

    TRACE("(%s,%s,%p)\n", debugstr_a(lpszUrl), debugstr_a(lpszCookieName),
        lpCookieData);

    if( lpszUrl )
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpszUrl, -1, NULL, 0 );
        szUrl = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpszUrl, -1, szUrl, len );
    }

    if( lpszCookieName )
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpszCookieName, -1, NULL, 0 );
        szCookieName = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpszCookieName, -1, szCookieName, len );
    }

    r = InternetGetCookieW( szUrl, szCookieName, NULL, &len );
    if( r )
    {
        szCookieData = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if( !szCookieData )
            return FALSE;

        r = InternetGetCookieW( szUrl, szCookieName, szCookieData, &len );

        *lpdwSize = WideCharToMultiByte( CP_ACP, 0, szCookieData, len,
                                lpCookieData, *lpdwSize, NULL, NULL );
    }

    if( szCookieData )
        HeapFree( GetProcessHeap(), 0, szCookieData );
    if( szCookieName )
        HeapFree( GetProcessHeap(), 0, szCookieName );
    if( szUrl )
        HeapFree( GetProcessHeap(), 0, szUrl );

    return r;
}


/***********************************************************************
 *           InternetSetCookieW (WININET.@)
 *
 * Sets cookie for the specified url
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetSetCookieW(LPCWSTR lpszUrl, LPCWSTR lpszCookieName,
    LPCWSTR lpCookieData)
{
    cookie *thisCookie;
    cookie_domain *thisCookieDomain;

    TRACE("(%s,%s,%s)\n", debugstr_w(lpszUrl),
        debugstr_w(lpszCookieName), debugstr_w(lpCookieData));

    if (!lpCookieData || !strlenW(lpCookieData))
    {
        TRACE("no cookie data, not adding\n");
	return FALSE;
    }
    if (!lpszCookieName)
    {
	/* some apps (or is it us??) try to add a cookie with no cookie name, but
         * the cookie data in the form of name=data. */
	/* FIXME, probably a bug here, for now I don't care */
	WCHAR *ourCookieName, *ourCookieData;
	int ourCookieNameSize;
        BOOL ret;
	if (!(ourCookieData = strchrW(lpCookieData, '=')))
	{
            TRACE("something terribly wrong with cookie data %s\n", 
                   debugstr_w(ourCookieData));
	    return FALSE;
	}
	ourCookieNameSize = ourCookieData - lpCookieData;
	ourCookieData += 1;
	ourCookieName = HeapAlloc(GetProcessHeap(), 0, 
                              (ourCookieNameSize + 1)*sizeof(WCHAR));
	strncpyW(ourCookieName, ourCookieData, ourCookieNameSize);
	ourCookieName[ourCookieNameSize] = '\0';
	TRACE("setting (hacked) cookie of %s, %s\n",
               debugstr_w(ourCookieName), debugstr_w(ourCookieData));
        ret = InternetSetCookieW(lpszUrl, ourCookieName, ourCookieData);
	HeapFree(GetProcessHeap(), 0, ourCookieName);
        return ret;
    }

    if (!(thisCookieDomain = COOKIE_findNextDomainFromUrl(lpszUrl, NULL, FALSE)))
        thisCookieDomain = COOKIE_addDomainFromUrl(lpszUrl);

    if ((thisCookie = COOKIE_findCookie(thisCookieDomain, lpszCookieName)))
	COOKIE_deleteCookie(thisCookie, FALSE);

    thisCookie = COOKIE_addCookie(thisCookieDomain, lpszCookieName, lpCookieData);
    return TRUE;
}


/***********************************************************************
 *           InternetSetCookieA (WININET.@)
 *
 * Sets cookie for the specified url
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetSetCookieA(LPCSTR lpszUrl, LPCSTR lpszCookieName,
    LPCSTR lpCookieData)
{
    DWORD len;
    LPWSTR szCookieData = NULL, szUrl = NULL, szCookieName = NULL;
    BOOL r;

    TRACE("(%s,%s,%s)\n", debugstr_a(lpszUrl),
        debugstr_a(lpszCookieName), debugstr_a(lpCookieData));

    if( lpszUrl )
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpszUrl, -1, NULL, 0 );
        szUrl = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpszUrl, -1, szUrl, len );
    }

    if( lpszCookieName )
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpszCookieName, -1, NULL, 0 );
        szCookieName = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpszCookieName, -1, szCookieName, len );
    }

    if( lpCookieData )
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpCookieData, -1, NULL, 0 );
        szCookieData = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpCookieData, -1, szCookieData, len );
    }

    r = InternetSetCookieW( szUrl, szCookieName, szCookieData );

    if( szCookieData )
        HeapFree( GetProcessHeap(), 0, szCookieData );
    if( szCookieName )
        HeapFree( GetProcessHeap(), 0, szCookieName );
    if( szUrl )
        HeapFree( GetProcessHeap(), 0, szUrl );

    return r;
}
