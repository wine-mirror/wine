/*
 * Copyright 2010 Jacek Caban for CodeWeavers
 * Copyright 2010 Thomas Mullaly
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "urlmon_main.h"
#include "wine/debug.h"

#define NO_SHLWAPI_REG
#include "shlwapi.h"

#define COMBINE_URI_FORCE_FLAG_USE  0x1

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

const WCHAR * WINAPI wine_get_canonicalized_uri(IUri *uri);
HRESULT WINAPI PrivateCoInternetCombineIUri(IUri *pBaseUri, IUri *pRelativeUri, DWORD dwCombineFlags,
                                            IUri **ppCombinedUri, DWORD_PTR dwReserved);
HRESULT WINAPI PrivateCoInternetParseIUri(IUri *pIUri, PARSEACTION ParseAction, DWORD dwFlags,
                                          LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult,
                                          DWORD_PTR dwReserved);

/* List of 3-character top level domain names Windows seems to recognize.
 * There might be more, but, these are the only ones I've found so far.
 */
static const struct {
    WCHAR tld_name[4];
} recognized_tlds[] = {
    {L"com"},
    {L"edu"},
    {L"gov"},
    {L"int"},
    {L"mil"},
    {L"net"},
    {L"org"}
};

static inline BOOL is_ascii(WCHAR c)
{
    return c < 0x80;
}

/* Attempts to parse the domain name from the host.
 *
 * This function also includes the Top-level Domain (TLD) name
 * of the host when it tries to find the domain name. If it finds
 * a valid domain name it will assign 'domain_start' the offset
 * into 'host' where the domain name starts.
 *
 * It's implied that if there is a domain name its range is:
 * [host+domain_start, host+host_len).
 */
void find_domain_name(const WCHAR *host, DWORD host_len,
                             INT *domain_start) {
    const WCHAR *last_tld, *sec_last_tld, *end, *p;

    end = host+host_len-1;

    *domain_start = -1;

    /* There has to be at least enough room for a '.' followed by a
     * 3-character TLD for a domain to even exist in the host name.
     */
    if(host_len < 4)
        return;

    for (last_tld = sec_last_tld = NULL, p = host; p <= end; p++)
    {
        if (*p == '.')
        {
            sec_last_tld = last_tld;
            last_tld = p;
        }
    }
    if(!last_tld)
        /* http://hostname -> has no domain name. */
        return;

    if(!sec_last_tld) {
        /* If the '.' is at the beginning of the host there
         * has to be at least 3 characters in the TLD for it
         * to be valid.
         *  Ex: .com -> .com as the domain name.
         *      .co  -> has no domain name.
         */
        if(last_tld-host == 0) {
            if(end-(last_tld-1) < 3)
                return;
        } else if(last_tld-host == 3) {
            DWORD i;

            /* If there are three characters in front of last_tld and
             * they are on the list of recognized TLDs, then this
             * host doesn't have a domain (since the host only contains
             * a TLD name.
             *  Ex: edu.uk -> has no domain name.
             *      foo.uk -> foo.uk as the domain name.
             */
            for(i = 0; i < ARRAY_SIZE(recognized_tlds); ++i) {
                if(!StrCmpNIW(host, recognized_tlds[i].tld_name, 3))
                    return;
            }
        } else if(last_tld-host < 3)
        {
            /* Anything less than 3 ASCII characters is considered part
             * of the TLD name.
             *  Ex: ak.uk -> Has no domain name.
             */
            for(p = host; p < last_tld; p++) {
                if(!is_ascii(*p))
                    break;
            }

            if(p == last_tld)
                return;
        }

        /* Otherwise the domain name is the whole host name. */
        *domain_start = 0;
    } else if(end+1-last_tld > 3) {
        /* If the last_tld has more than 3 characters, then it's automatically
         * considered the TLD of the domain name.
         *  Ex: www.winehq.org.uk.test -> uk.test as the domain name.
         */
        *domain_start = (sec_last_tld+1)-host;
    } else if(last_tld - (sec_last_tld+1) < 4) {
        DWORD i;
        /* If the sec_last_tld is 3 characters long it HAS to be on the list of
         * recognized to still be considered part of the TLD name, otherwise
         * it's considered the domain name.
         *  Ex: www.google.com.uk -> google.com.uk as the domain name.
         *      www.google.foo.uk -> foo.uk as the domain name.
         */
        if(last_tld - (sec_last_tld+1) == 3) {
            for(i = 0; i < ARRAY_SIZE(recognized_tlds); ++i) {
                if(!StrCmpNIW(sec_last_tld+1, recognized_tlds[i].tld_name, 3)) {
                    for (p = sec_last_tld; p > host; p--) if (p[-1] == '.') break;
                    *domain_start = p - host;
                    TRACE("Found domain name %s\n", debugstr_wn(host+*domain_start,
                                                        (host+host_len)-(host+*domain_start)));
                    return;
                }
            }

            *domain_start = (sec_last_tld+1)-host;
        } else {
            /* Since the sec_last_tld is less than 3 characters it's considered
             * part of the TLD.
             *  Ex: www.google.fo.uk -> google.fo.uk as the domain name.
             */
            for (p = sec_last_tld; p > host; p--) if (p[-1] == '.') break;
            *domain_start = p - host;
        }
    } else {
        /* The second to last TLD has more than 3 characters making it
         * the domain name.
         *  Ex: www.google.test.us -> test.us as the domain name.
         */
        *domain_start = (sec_last_tld+1)-host;
    }

    TRACE("Found domain name %s\n", debugstr_wn(host+*domain_start,
                                        (host+host_len)-(host+*domain_start)));
}

/***********************************************************************
 *           CoInternetCombineIUri (urlmon.@)
 */
HRESULT WINAPI CoInternetCombineIUri(IUri *pBaseUri, IUri *pRelativeUri, DWORD dwCombineFlags,
                                     IUri **ppCombinedUri, DWORD_PTR dwReserved)
{
    HRESULT hr;
    IInternetProtocolInfo *info;
    const WCHAR *relative_canon_uri, *base_canon_uri;

    TRACE("(%p %p %lx %p %Ix)\n", pBaseUri, pRelativeUri, dwCombineFlags, ppCombinedUri, dwReserved);

    if(!ppCombinedUri)
        return E_INVALIDARG;

    if(!pBaseUri || !pRelativeUri) {
        *ppCombinedUri = NULL;
        return E_INVALIDARG;
    }

    relative_canon_uri = wine_get_canonicalized_uri(pRelativeUri);
    base_canon_uri = wine_get_canonicalized_uri(pBaseUri);
    if(!relative_canon_uri || !base_canon_uri) {
        *ppCombinedUri = NULL;
        FIXME("(%p %p %lx %p %Ix) Unknown IUri types not supported yet.\n",
            pBaseUri, pRelativeUri, dwCombineFlags, ppCombinedUri, dwReserved);
        return E_NOTIMPL;
    }

    info = get_protocol_info(base_canon_uri);
    if(info) {
        WCHAR result[INTERNET_MAX_URL_LENGTH+1];
        DWORD result_len = 0;

        hr = IInternetProtocolInfo_CombineUrl(info, base_canon_uri, relative_canon_uri, dwCombineFlags,
                                              result, INTERNET_MAX_URL_LENGTH+1, &result_len, 0);
        IInternetProtocolInfo_Release(info);
        if(SUCCEEDED(hr)) {
            hr = CreateUri(result, Uri_CREATE_ALLOW_RELATIVE, 0, ppCombinedUri);
            if(SUCCEEDED(hr))
                return hr;
        }
    }

    return PrivateCoInternetCombineIUri(pBaseUri, pRelativeUri, dwCombineFlags, ppCombinedUri, 0);
}

/***********************************************************************
 *           CoInternetCombineUrlEx (urlmon.@)
 */
HRESULT WINAPI CoInternetCombineUrlEx(IUri *pBaseUri, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags,
                                      IUri **ppCombinedUri, DWORD_PTR dwReserved)
{
    IUri *relative;
    const WCHAR *base_canon_uri;
    HRESULT hr;
    IInternetProtocolInfo *info;

    TRACE("(%p %s %lx %p %Ix)\n", pBaseUri, debugstr_w(pwzRelativeUrl), dwCombineFlags,
        ppCombinedUri, dwReserved);

    if(!ppCombinedUri)
        return E_POINTER;

    if(!pwzRelativeUrl) {
        *ppCombinedUri = NULL;
        return E_UNEXPECTED;
    }

    if(!pBaseUri) {
        *ppCombinedUri = NULL;
        return E_INVALIDARG;
    }

    base_canon_uri = wine_get_canonicalized_uri(pBaseUri);
    if(!base_canon_uri) {
        *ppCombinedUri = NULL;
        FIXME("(%p %s %lx %p %Ix) Unknown IUri's not supported yet.\n", pBaseUri, debugstr_w(pwzRelativeUrl),
            dwCombineFlags, ppCombinedUri, dwReserved);
        return E_NOTIMPL;
    }

    info = get_protocol_info(base_canon_uri);
    if(info) {
        WCHAR result[INTERNET_MAX_URL_LENGTH+1];
        DWORD result_len = 0;

        hr = IInternetProtocolInfo_CombineUrl(info, base_canon_uri, pwzRelativeUrl, dwCombineFlags,
                                              result, INTERNET_MAX_URL_LENGTH+1, &result_len, 0);
        IInternetProtocolInfo_Release(info);
        if(SUCCEEDED(hr)) {
            hr = CreateUri(result, Uri_CREATE_ALLOW_RELATIVE, 0, ppCombinedUri);
            if(SUCCEEDED(hr))
                return hr;
        }
    }

    hr = CreateUri(pwzRelativeUrl, Uri_CREATE_ALLOW_RELATIVE|Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME, 0, &relative);
    if(FAILED(hr)) {
        *ppCombinedUri = NULL;
        return hr;
    }

    hr = PrivateCoInternetCombineIUri(pBaseUri, relative, dwCombineFlags, ppCombinedUri, COMBINE_URI_FORCE_FLAG_USE);

    IUri_Release(relative);
    return hr;
}

/***********************************************************************
 *           CoInternetParseIUri (urlmon.@)
 */
HRESULT WINAPI CoInternetParseIUri(IUri *pIUri, PARSEACTION ParseAction, DWORD dwFlags,
                                   LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult,
                                   DWORD_PTR dwReserved)
{
    HRESULT hr;
    const WCHAR *canon_uri;
    IInternetProtocolInfo *info;

    TRACE("(%p %d %lx %p %ld %p %Ix)\n", pIUri, ParseAction, dwFlags, pwzResult,
        cchResult, pcchResult, dwReserved);

    if(!pcchResult)
        return E_POINTER;

    if(!pwzResult || !pIUri) {
        *pcchResult = 0;
        return E_INVALIDARG;
    }

    if(!(canon_uri = wine_get_canonicalized_uri(pIUri))) {
        *pcchResult = 0;
        FIXME("(%p %d %lx %p %ld %p %Ix) Unknown IUri's not supported for this action.\n",
            pIUri, ParseAction, dwFlags, pwzResult, cchResult, pcchResult, dwReserved);
        return E_NOTIMPL;
    }

    info = get_protocol_info(canon_uri);
    if(info) {
        hr = IInternetProtocolInfo_ParseUrl(info, canon_uri, ParseAction, dwFlags,
                                            pwzResult, cchResult, pcchResult, 0);
        IInternetProtocolInfo_Release(info);
        if(SUCCEEDED(hr)) return hr;
    }

    return PrivateCoInternetParseIUri(pIUri, ParseAction, dwFlags, pwzResult, cchResult, pcchResult, dwReserved);
}
