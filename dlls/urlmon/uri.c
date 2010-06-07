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

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

typedef struct {
    const IUriVtbl  *lpIUriVtbl;
    LONG ref;

    BSTR raw_uri;
} Uri;

typedef struct {
    const IUriBuilderVtbl  *lpIUriBuilderVtbl;
    LONG ref;
} UriBuilder;

typedef struct {
    BSTR            uri;


    BOOL            is_relative;

    const WCHAR     *scheme;
    DWORD           scheme_len;
} parse_data;

static inline BOOL is_alpha(WCHAR val) {
	return ((val >= 'a' && val <= 'z') || (val >= 'A' && val <= 'Z'));
}

static inline BOOL is_num(WCHAR val) {
	return (val >= '0' && val <= '9');
}

/* A URI is implicitly a file path if it begins with
 * a drive letter (eg X:) or starts with "\\" (UNC path).
 */
static inline BOOL is_implicit_file_path(const WCHAR *str) {
    if(is_alpha(str[0]) && str[1] == ':')
        return TRUE;
    else if(str[0] == '\\' && str[1] == '\\')
        return TRUE;

    return FALSE;
}

/* Tries to parse the scheme name of the URI.
 *
 * scheme = ALPHA *(ALPHA | NUM | '+' | '-' | '.') as defined by RFC 3896.
 * NOTE: Windows accepts a number as the first character of a scheme.
 */
static BOOL parse_scheme_name(const WCHAR **ptr, parse_data *data) {
    const WCHAR *start = *ptr;

    data->scheme = NULL;
    data->scheme_len = 0;

    while(**ptr) {
        if(!is_num(**ptr) && !is_alpha(**ptr) && **ptr != '+' &&
           **ptr != '-' && **ptr != '.')
            break;

        (*ptr)++;
    }

    if(*ptr == start)
        return FALSE;

    /* Schemes must end with a ':' */
    if(**ptr != ':') {
        *ptr = start;
        return FALSE;
    }

    data->scheme = start;
    data->scheme_len = *ptr - start;

    ++(*ptr);
    return TRUE;
}

/* Tries to parse (or deduce) the scheme_name of a URI. If it can't
 * parse a scheme from the URI it will try to deduce the scheme_name and scheme_type
 * using the flags specified in 'flags' (if any). Flags that affect how this function
 * operates are the Uri_CREATE_ALLOW_* flags.
 *
 * All parsed/deduced information will be stored in 'data' when the function returns.
 *
 * Returns TRUE if it was able to successfully parse the information.
 */
static BOOL parse_scheme(const WCHAR **ptr, parse_data *data, DWORD flags) {
    static const WCHAR fileW[] = {'f','i','l','e',0};
    static const WCHAR wildcardW[] = {'*',0};

    /* First check to see if the uri could implicitly be a file path. */
    if(is_implicit_file_path(*ptr)) {
        if(flags & Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME) {
            data->scheme = fileW;
            data->scheme_len = lstrlenW(fileW);
            TRACE("(%p %p %x): URI is an implicit file path.\n", ptr, data, flags);
        } else {
            /* Window's does not consider anything that can implicitly be a file
             * path to be a valid URI if the ALLOW_IMPLICIT_FILE_SCHEME flag is not set...
             */
            TRACE("(%p %p %x): URI is implicitly a file path, but, the ALLOW_IMPLICIT_FILE_SCHEME flag wasn't set.\n",
                    ptr, data, flags);
            return FALSE;
        }
    } else if(!parse_scheme_name(ptr, data)) {
        /* No Scheme was found, this means it could be:
         *      a) an implicit Wildcard scheme
         *      b) a relative URI
         *      c) a invalid URI.
         */
        if(flags & Uri_CREATE_ALLOW_IMPLICIT_WILDCARD_SCHEME) {
            data->scheme = wildcardW;
            data->scheme_len = lstrlenW(wildcardW);

            TRACE("(%p %p %x): URI is an implicit wildcard scheme.\n", ptr, data, flags);
        } else if (flags & Uri_CREATE_ALLOW_RELATIVE) {
            data->is_relative = TRUE;
            TRACE("(%p %p %x): URI is relative.\n", ptr, data, flags);
        } else {
            TRACE("(%p %p %x): Malformed URI found. Unable to deduce scheme name.\n", ptr, data, flags);
            return FALSE;
        }
    }

    if(!data->is_relative)
        TRACE("(%p %p %x): Found scheme=%s scheme_len=%d\n", ptr, data, flags,
                debugstr_wn(data->scheme, data->scheme_len), data->scheme_len);

    return TRUE;
}

/* Parses and validates the components of the specified by data->uri
 * and stores the information it parses into 'data'.
 *
 * Returns TRUE if it successfully parsed the URI. False otherwise.
 */
static BOOL parse_uri(parse_data *data, DWORD flags) {
    const WCHAR *ptr;
    const WCHAR **pptr;

    ptr = data->uri;
    pptr = &ptr;

    TRACE("(%p %x): BEGINNING TO PARSE URI %s.\n", data, flags, debugstr_w(data->uri));

    if(!parse_scheme(pptr, data, flags))
        return FALSE;

    TRACE("(%p %x): FINISHED PARSING URI.\n", data, flags);
    return TRUE;
}

#define URI(x)         ((IUri*)  &(x)->lpIUriVtbl)
#define URIBUILDER(x)  ((IUriBuilder*)  &(x)->lpIUriBuilderVtbl)

#define URI_THIS(iface) DEFINE_THIS(Uri, IUri, iface)

static HRESULT WINAPI Uri_QueryInterface(IUri *iface, REFIID riid, void **ppv)
{
    Uri *This = URI_THIS(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = URI(This);
    }else if(IsEqualGUID(&IID_IUri, riid)) {
        TRACE("(%p)->(IID_IUri %p)\n", This, ppv);
        *ppv = URI(This);
    }else {
        TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI Uri_AddRef(IUri *iface)
{
    Uri *This = URI_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI Uri_Release(IUri *iface)
{
    Uri *This = URI_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        SysFreeString(This->raw_uri);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI Uri_GetPropertyBSTR(IUri *iface, Uri_PROPERTY uriProp, BSTR *pbstrProperty, DWORD dwFlags)
{
    Uri *This = URI_THIS(iface);
    HRESULT hres;
    TRACE("(%p)->(%d %p %x)\n", This, uriProp, pbstrProperty, dwFlags);

    if(!pbstrProperty)
        return E_POINTER;

    if(uriProp > Uri_PROPERTY_STRING_LAST) {
        /* Windows allocates an empty BSTR for invalid Uri_PROPERTY's. */
        *pbstrProperty = SysAllocStringLen(NULL, 0);

        /* It only returns S_FALSE for the ZONE property... */
        if(uriProp == Uri_PROPERTY_ZONE)
            return S_FALSE;
        else
            return S_OK;
    }

    /* Don't have support for flags yet. */
    if(dwFlags) {
        FIXME("(%p)->(%d %p %x)\n", This, uriProp, pbstrProperty, dwFlags);
        return E_NOTIMPL;
    }

    switch(uriProp) {
    case Uri_PROPERTY_RAW_URI:
        *pbstrProperty = SysAllocString(This->raw_uri);
        if(!(*pbstrProperty))
            hres = E_OUTOFMEMORY;
        else
            hres = S_OK;
        break;
    default:
        FIXME("(%p)->(%d %p %x)\n", This, uriProp, pbstrProperty, dwFlags);
        hres = E_NOTIMPL;
    }

    return hres;
}

static HRESULT WINAPI Uri_GetPropertyLength(IUri *iface, Uri_PROPERTY uriProp, DWORD *pcchProperty, DWORD dwFlags)
{
    Uri *This = URI_THIS(iface);
    HRESULT hres;
    TRACE("(%p)->(%d %p %x)\n", This, uriProp, pcchProperty, dwFlags);

    if(!pcchProperty)
        return E_INVALIDARG;

    /* Can only return a length for a property if it's a string. */
    if(uriProp > Uri_PROPERTY_STRING_LAST)
        return E_INVALIDARG;

    /* Don't have support for flags yet. */
    if(dwFlags) {
        FIXME("(%p)->(%d %p %x)\n", This, uriProp, pcchProperty, dwFlags);
        return E_NOTIMPL;
    }

    switch(uriProp) {
    case Uri_PROPERTY_RAW_URI:
        *pcchProperty = SysStringLen(This->raw_uri);
        hres = S_OK;
        break;
    default:
        FIXME("(%p)->(%d %p %x)\n", This, uriProp, pcchProperty, dwFlags);
        hres = E_NOTIMPL;
    }

    return hres;
}

static HRESULT WINAPI Uri_GetPropertyDWORD(IUri *iface, Uri_PROPERTY uriProp, DWORD *pcchProperty, DWORD dwFlags)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%d %p %x)\n", This, uriProp, pcchProperty, dwFlags);

    if(!pcchProperty)
        return E_INVALIDARG;

    /* Microsoft's implementation for the ZONE property of a URI seems to be lacking...
     * From what I can tell, instead of checking which URLZONE the URI belongs to it
     * simply assigns URLZONE_INVALID and returns E_NOTIMPL. This also applies to the GetZone
     * function.
     */
    if(uriProp == Uri_PROPERTY_ZONE) {
        *pcchProperty = URLZONE_INVALID;
        return E_NOTIMPL;
    }

    if(uriProp < Uri_PROPERTY_DWORD_START) {
        *pcchProperty = 0;
        return E_INVALIDARG;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_HasProperty(IUri *iface, Uri_PROPERTY uriProp, BOOL *pfHasProperty)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%d %p)\n", This, uriProp, pfHasProperty);

    if(!pfHasProperty)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetAbsoluteUri(IUri *iface, BSTR *pstrAbsoluteUri)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrAbsoluteUri);

    if(!pstrAbsoluteUri)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetAuthority(IUri *iface, BSTR *pstrAuthority)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrAuthority);

    if(!pstrAuthority)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetDisplayUri(IUri *iface, BSTR *pstrDisplayUri)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrDisplayUri);

    if(!pstrDisplayUri)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetDomain(IUri *iface, BSTR *pstrDomain)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrDomain);

    if(!pstrDomain)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetExtension(IUri *iface, BSTR *pstrExtension)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrExtension);

    if(!pstrExtension)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetFragment(IUri *iface, BSTR *pstrFragment)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrFragment);

    if(!pstrFragment)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetHost(IUri *iface, BSTR *pstrHost)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrHost);

    if(!pstrHost)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetPassword(IUri *iface, BSTR *pstrPassword)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrPassword);

    if(!pstrPassword)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetPath(IUri *iface, BSTR *pstrPath)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrPath);

    if(!pstrPath)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetPathAndQuery(IUri *iface, BSTR *pstrPathAndQuery)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrPathAndQuery);

    if(!pstrPathAndQuery)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetQuery(IUri *iface, BSTR *pstrQuery)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrQuery);

    if(!pstrQuery)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetRawUri(IUri *iface, BSTR *pstrRawUri)
{
    Uri *This = URI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, pstrRawUri);

    /* Just forward the call to GetPropertyBSTR. */
    return Uri_GetPropertyBSTR(iface, Uri_PROPERTY_RAW_URI, pstrRawUri, 0);
}

static HRESULT WINAPI Uri_GetSchemeName(IUri *iface, BSTR *pstrSchemeName)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrSchemeName);

    if(!pstrSchemeName)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetUserInfo(IUri *iface, BSTR *pstrUserInfo)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrUserInfo);

    if(!pstrUserInfo)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetUserName(IUri *iface, BSTR *pstrUserName)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrUserName);

    if(!pstrUserName)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetHostType(IUri *iface, DWORD *pdwHostType)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwHostType);

    if(!pdwHostType)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetPort(IUri *iface, DWORD *pdwPort)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwPort);

    if(!pdwPort)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetScheme(IUri *iface, DWORD *pdwScheme)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwScheme);

    if(!pdwScheme)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetZone(IUri *iface, DWORD *pdwZone)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwZone);

    if(!pdwZone)
        return E_INVALIDARG;

    /* Microsoft doesn't seem to have this implemented yet... See
     * the comment in Uri_GetPropertyDWORD for more about this.
     */
    *pdwZone = URLZONE_INVALID;
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetProperties(IUri *iface, DWORD *pdwProperties)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwProperties);

    if(!pdwProperties)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_IsEqual(IUri *iface, IUri *pUri, BOOL *pfEqual)
{
    Uri *This = URI_THIS(iface);
    TRACE("(%p)->(%p %p)\n", This, pUri, pfEqual);

    if(!pfEqual)
        return E_POINTER;

    if(!pUri) {
        *pfEqual = FALSE;

        /* For some reason Windows returns S_OK here... */
        return S_OK;
    }

    FIXME("(%p)->(%p %p)\n", This, pUri, pfEqual);
    return E_NOTIMPL;
}

#undef URI_THIS

static const IUriVtbl UriVtbl = {
    Uri_QueryInterface,
    Uri_AddRef,
    Uri_Release,
    Uri_GetPropertyBSTR,
    Uri_GetPropertyLength,
    Uri_GetPropertyDWORD,
    Uri_HasProperty,
    Uri_GetAbsoluteUri,
    Uri_GetAuthority,
    Uri_GetDisplayUri,
    Uri_GetDomain,
    Uri_GetExtension,
    Uri_GetFragment,
    Uri_GetHost,
    Uri_GetPassword,
    Uri_GetPath,
    Uri_GetPathAndQuery,
    Uri_GetQuery,
    Uri_GetRawUri,
    Uri_GetSchemeName,
    Uri_GetUserInfo,
    Uri_GetUserName,
    Uri_GetHostType,
    Uri_GetPort,
    Uri_GetScheme,
    Uri_GetZone,
    Uri_GetProperties,
    Uri_IsEqual
};

/***********************************************************************
 *           CreateUri (urlmon.@)
 */
HRESULT WINAPI CreateUri(LPCWSTR pwzURI, DWORD dwFlags, DWORD_PTR dwReserved, IUri **ppURI)
{
    Uri *ret;
    parse_data data;

    TRACE("(%s %x %x %p)\n", debugstr_w(pwzURI), dwFlags, (DWORD)dwReserved, ppURI);

    if(!ppURI)
        return E_INVALIDARG;

    if(!pwzURI) {
        *ppURI = NULL;
        return E_INVALIDARG;
    }

    ret = heap_alloc(sizeof(Uri));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->lpIUriVtbl = &UriVtbl;
    ret->ref = 1;

    /* Create a copy of pwzURI and store it as the raw_uri. */
    ret->raw_uri = SysAllocString(pwzURI);
    if(!ret->raw_uri) {
        heap_free(ret);
        return E_OUTOFMEMORY;
    }

    memset(&data, 0, sizeof(parse_data));
    data.uri = ret->raw_uri;

    if(!parse_uri(&data, dwFlags)) {
        /* Encountered an unsupported or invalid URI */
        SysFreeString(ret->raw_uri);
        heap_free(ret);
        *ppURI = NULL;
        return E_INVALIDARG;
    }

    *ppURI = URI(ret);
    return S_OK;
}

#define URIBUILDER_THIS(iface) DEFINE_THIS(UriBuilder, IUriBuilder, iface)

static HRESULT WINAPI UriBuilder_QueryInterface(IUriBuilder *iface, REFIID riid, void **ppv)
{
    UriBuilder *This = URIBUILDER_THIS(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = URIBUILDER(This);
    }else if(IsEqualGUID(&IID_IUriBuilder, riid)) {
        TRACE("(%p)->(IID_IUri %p)\n", This, ppv);
        *ppv = URIBUILDER(This);
    }else {
        TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI UriBuilder_AddRef(IUriBuilder *iface)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI UriBuilder_Release(IUriBuilder *iface)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI UriBuilder_CreateUriSimple(IUriBuilder *iface,
                                                 DWORD        dwAllowEncodingPropertyMask,
                                                 DWORD_PTR    dwReserved,
                                                 IUri       **ppIUri)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dwAllowEncodingPropertyMask, (DWORD)dwReserved, ppIUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_CreateUri(IUriBuilder *iface,
                                           DWORD        dwCreateFlags,
                                           DWORD        dwAllowEncodingPropertyMask,
                                           DWORD_PTR    dwReserved,
                                           IUri       **ppIUri)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(0x%08x %d %d %p)\n", This, dwCreateFlags, dwAllowEncodingPropertyMask, (DWORD)dwReserved, ppIUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_CreateUriWithFlags(IUriBuilder *iface,
                                         DWORD        dwCreateFlags,
                                         DWORD        dwUriBuilderFlags,
                                         DWORD        dwAllowEncodingPropertyMask,
                                         DWORD_PTR    dwReserved,
                                         IUri       **ppIUri)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(0x%08x 0x%08x %d %d %p)\n", This, dwCreateFlags, dwUriBuilderFlags,
        dwAllowEncodingPropertyMask, (DWORD)dwReserved, ppIUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI  UriBuilder_GetIUri(IUriBuilder *iface, IUri **ppIUri)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppIUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetIUri(IUriBuilder *iface, IUri *pIUri)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pIUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetFragment(IUriBuilder *iface, DWORD *pcchFragment, LPCWSTR *ppwzFragment)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchFragment, ppwzFragment);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetHost(IUriBuilder *iface, DWORD *pcchHost, LPCWSTR *ppwzHost)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchHost, ppwzHost);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetPassword(IUriBuilder *iface, DWORD *pcchPassword, LPCWSTR *ppwzPassword)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchPassword, ppwzPassword);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetPath(IUriBuilder *iface, DWORD *pcchPath, LPCWSTR *ppwzPath)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchPath, ppwzPath);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetPort(IUriBuilder *iface, BOOL *pfHasPort, DWORD *pdwPort)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pfHasPort, pdwPort);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetQuery(IUriBuilder *iface, DWORD *pcchQuery, LPCWSTR *ppwzQuery)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchQuery, ppwzQuery);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetSchemeName(IUriBuilder *iface, DWORD *pcchSchemeName, LPCWSTR *ppwzSchemeName)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchSchemeName, ppwzSchemeName);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetUserName(IUriBuilder *iface, DWORD *pcchUserName, LPCWSTR *ppwzUserName)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchUserName, ppwzUserName);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetFragment(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetHost(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetPassword(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetPath(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetPort(IUriBuilder *iface, BOOL fHasPort, DWORD dwNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%d %d)\n", This, fHasPort, dwNewValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetQuery(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetSchemeName(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetUserName(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_RemoveProperties(IUriBuilder *iface, DWORD dwPropertyMask)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(0x%08x)\n", This, dwPropertyMask);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_HasBeenModified(IUriBuilder *iface, BOOL *pfModified)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pfModified);
    return E_NOTIMPL;
}

#undef URIBUILDER_THIS

static const IUriBuilderVtbl UriBuilderVtbl = {
    UriBuilder_QueryInterface,
    UriBuilder_AddRef,
    UriBuilder_Release,
    UriBuilder_CreateUriSimple,
    UriBuilder_CreateUri,
    UriBuilder_CreateUriWithFlags,
    UriBuilder_GetIUri,
    UriBuilder_SetIUri,
    UriBuilder_GetFragment,
    UriBuilder_GetHost,
    UriBuilder_GetPassword,
    UriBuilder_GetPath,
    UriBuilder_GetPort,
    UriBuilder_GetQuery,
    UriBuilder_GetSchemeName,
    UriBuilder_GetUserName,
    UriBuilder_SetFragment,
    UriBuilder_SetHost,
    UriBuilder_SetPassword,
    UriBuilder_SetPath,
    UriBuilder_SetPort,
    UriBuilder_SetQuery,
    UriBuilder_SetSchemeName,
    UriBuilder_SetUserName,
    UriBuilder_RemoveProperties,
    UriBuilder_HasBeenModified,
};

/***********************************************************************
 *           CreateIUriBuilder (urlmon.@)
 */
HRESULT WINAPI CreateIUriBuilder(IUri *pIUri, DWORD dwFlags, DWORD_PTR dwReserved, IUriBuilder **ppIUriBuilder)
{
    UriBuilder *ret;

    TRACE("(%p %x %x %p)\n", pIUri, dwFlags, (DWORD)dwReserved, ppIUriBuilder);

    ret = heap_alloc(sizeof(UriBuilder));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->lpIUriBuilderVtbl = &UriBuilderVtbl;
    ret->ref = 1;

    *ppIUriBuilder = URIBUILDER(ret);
    return S_OK;
}
