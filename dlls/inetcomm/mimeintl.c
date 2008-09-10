/*
 * MIME OLE International interface
 *
 * Copyright 2008 Huw Davies for CodeWeavers
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

#define COBJMACROS
#define NONAMELESSUNION

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "objbase.h"
#include "ole2.h"
#include "mimeole.h"

#include "wine/list.h"
#include "wine/debug.h"

#include "inetcomm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(inetcomm);

typedef struct
{
    const IMimeInternationalVtbl *lpVtbl;
    LONG refs;
} internat;

static inline internat *impl_from_IMimeInternational( IMimeInternational *iface )
{
    return (internat *)((char*)iface - FIELD_OFFSET(internat, lpVtbl));
}

static HRESULT WINAPI MimeInternat_QueryInterface( IMimeInternational *iface, REFIID riid, LPVOID *ppobj )
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IMimeInternational))
    {
        IMimeInternational_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }

    FIXME("interface %s not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI MimeInternat_AddRef( IMimeInternational *iface )
{
    internat *This = impl_from_IMimeInternational( iface );
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI MimeInternat_Release( IMimeInternational *iface )
{
    internat *This = impl_from_IMimeInternational( iface );
    ULONG refs;

    refs = InterlockedDecrement(&This->refs);
    if (!refs)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refs;
}

static HRESULT WINAPI MimeInternat_SetDefaultCharset(IMimeInternational *iface, HCHARSET hCharset)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_GetDefaultCharset(IMimeInternational *iface, LPHCHARSET phCharset)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_GetCodePageCharset(IMimeInternational *iface, CODEPAGEID cpiCodePage,
                                                      CHARSETTYPE ctCsetType,
                                                      LPHCHARSET phCharset)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_FindCharset(IMimeInternational *iface, LPCSTR pszCharset,
                                               LPHCHARSET phCharset)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_GetCharsetInfo(IMimeInternational *iface, HCHARSET hCharset,
                                                  LPINETCSETINFO pCsetInfo)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_GetCodePageInfo(IMimeInternational *iface, CODEPAGEID cpiCodePage,
                                                   LPCODEPAGEINFO pCodePageInfo)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_DecodeHeader(IMimeInternational *iface, HCHARSET hCharset,
                                                LPCSTR pszData,
                                                LPPROPVARIANT pDecoded,
                                                LPRFC1522INFO pRfc1522Info)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_EncodeHeader(IMimeInternational *iface, HCHARSET hCharset,
                                                LPPROPVARIANT pData,
                                                LPSTR *ppszEncoded,
                                                LPRFC1522INFO pRfc1522Info)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_ConvertBuffer(IMimeInternational *iface, CODEPAGEID cpiSource,
                                                 CODEPAGEID cpiDest,
                                                 LPBLOB pIn,
                                                 LPBLOB pOut,
                                                 ULONG *pcbRead)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_ConvertString(IMimeInternational *iface, CODEPAGEID cpiSource,
                                                 CODEPAGEID cpiDest,
                                                 LPPROPVARIANT pIn,
                                                 LPPROPVARIANT pOut)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_MLANG_ConvertInetReset(IMimeInternational *iface)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_MLANG_ConvertInetString(IMimeInternational *iface, CODEPAGEID cpiSource,
                                                           CODEPAGEID cpiDest,
                                                           LPCSTR pSource,
                                                           int *pnSizeOfSource,
                                                           LPSTR pDestination,
                                                           int *pnDstSize)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_Rfc1522Decode(IMimeInternational *iface, LPCSTR pszValue,
                                                 LPCSTR pszCharset,
                                                 ULONG cchmax,
                                                 LPSTR *ppszDecoded)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeInternat_Rfc1522Encode(IMimeInternational *iface, LPCSTR pszValue,
                                                 HCHARSET hCharset,
                                                 LPSTR *ppszEncoded)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static IMimeInternationalVtbl mime_internat_vtbl =
{
    MimeInternat_QueryInterface,
    MimeInternat_AddRef,
    MimeInternat_Release,
    MimeInternat_SetDefaultCharset,
    MimeInternat_GetDefaultCharset,
    MimeInternat_GetCodePageCharset,
    MimeInternat_FindCharset,
    MimeInternat_GetCharsetInfo,
    MimeInternat_GetCodePageInfo,
    MimeInternat_DecodeHeader,
    MimeInternat_EncodeHeader,
    MimeInternat_ConvertBuffer,
    MimeInternat_ConvertString,
    MimeInternat_MLANG_ConvertInetReset,
    MimeInternat_MLANG_ConvertInetString,
    MimeInternat_Rfc1522Decode,
    MimeInternat_Rfc1522Encode
};

static internat *global_internat;

HRESULT MimeInternational_Construct(IMimeInternational **internat)
{
    global_internat = HeapAlloc(GetProcessHeap(), 0, sizeof(*global_internat));
    global_internat->lpVtbl = &mime_internat_vtbl;
    global_internat->refs = 0;

    *internat = (IMimeInternational*)&global_internat->lpVtbl;

    IMimeInternational_AddRef(*internat);
    return S_OK;
}

HRESULT WINAPI MimeOleGetInternat(IMimeInternational **internat)
{
    TRACE("(%p)\n", internat);

    *internat = (IMimeInternational *)&global_internat->lpVtbl;
    IMimeInternational_AddRef(*internat);
    return S_OK;
}
