/*
 * MIME OLE Interfaces
 *
 * Copyright 2006 Robert Shearman for CodeWeavers
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

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "mimeole.h"

#include "wine/debug.h"

#include "inetcomm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(inetcomm);

typedef struct MimeMessage
{
    const IMimeMessageVtbl *lpVtbl;

    LONG refs;
} MimeMessage;

static HRESULT WINAPI MimeMessage_QueryInterface(IMimeMessage *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IPersist) ||
        IsEqualIID(riid, &IID_IPersistStreamInit) ||
        IsEqualIID(riid, &IID_IMimeMessageTree) ||
        IsEqualIID(riid, &IID_IMimeMessage))
    {
        *ppv = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    FIXME("no interface for %s\n", debugstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MimeMessage_AddRef(IMimeMessage *iface)
{
    MimeMessage *This = (MimeMessage *)iface;
    TRACE("(%p)->()\n", iface);
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI MimeMessage_Release(IMimeMessage *iface)
{
    MimeMessage *This = (MimeMessage *)iface;
    ULONG refs;

    TRACE("(%p)->()\n", iface);

    refs = InterlockedDecrement(&This->refs);
    if (!refs)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refs;
}

/*** IPersist methods ***/
static HRESULT WINAPI MimeMessage_GetClassID(
    IMimeMessage *iface,
    CLSID *pClassID)
{
    FIXME("(%p)->(%p)\n", iface, pClassID);
    return E_NOTIMPL;
}

/*** IPersistStreamInit methods ***/
static HRESULT WINAPI MimeMessage_IsDirty(
    IMimeMessage *iface)
{
    FIXME("(%p)->()\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_Load(
    IMimeMessage *iface,
    LPSTREAM pStm){
    FIXME("(%p)->(%p)\n", iface, pStm);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_Save(
    IMimeMessage *iface,
    LPSTREAM pStm,
    BOOL fClearDirty)
{
    FIXME("(%p)->(%p, %s)\n", iface, pStm, fClearDirty ? "TRUE" : "FALSE");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetSizeMax(
    IMimeMessage *iface,
    ULARGE_INTEGER *pcbSize)
{
    FIXME("(%p)->(%p)\n", iface, pcbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_InitNew(
    IMimeMessage *iface)
{
    FIXME("(%p)->()\n", iface);
    return E_NOTIMPL;
}

/*** IMimeMessageTree methods ***/
static HRESULT WINAPI MimeMessage_GetMessageSource(
    IMimeMessage *iface,
    IStream **ppStream,
    DWORD dwFlags)
{
    FIXME("(%p)->(%p, 0x%x)\n", iface, ppStream, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetMessageSize(
    IMimeMessage *iface,
    ULONG *pcbSize,
    DWORD dwFlags)
{
    FIXME("(%p)->(%p, 0x%x)\n", iface, pcbSize, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_LoadOffsetTable(
    IMimeMessage *iface,
    IStream *pStream)
{
    FIXME("(%p)->(%p)\n", iface, pStream);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_SaveOffsetTable(
    IMimeMessage *iface,
    IStream *pStream,
    DWORD dwFlags)
{
    FIXME("(%p)->(%p, 0x%x)\n", iface, pStream, dwFlags);
    return E_NOTIMPL;
}


static HRESULT WINAPI MimeMessage_GetFlags(
    IMimeMessage *iface,
    DWORD *pdwFlags)
{
    FIXME("(%p)->(%p)\n", iface, pdwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_Commit(
    IMimeMessage *iface,
    DWORD dwFlags)
{
    FIXME("(%p)->(0x%x)\n", iface, dwFlags);
    return E_NOTIMPL;
}


static HRESULT WINAPI MimeMessage_HandsOffStorage(
    IMimeMessage *iface)
{
    FIXME("(%p)->()\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_BindToObject(
    IMimeMessage *iface,
    const HBODY hBody,
    REFIID riid,
    void **ppvObject)
{
    FIXME("(%p)->(%p, %s, %p)\n", iface, hBody, debugstr_guid(riid), ppvObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_SaveBody(
    IMimeMessage *iface,
    HBODY hBody,
    DWORD dwFlags,
    IStream *pStream)
{
    FIXME("(%p)->(%p, 0x%x, %p)\n", iface, hBody, dwFlags, pStream);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_InsertBody(
    IMimeMessage *iface,
    BODYLOCATION location,
    HBODY hPivot,
    LPHBODY phBody)
{
    FIXME("(%p)->(%d, %p, %p)\n", iface, location, hPivot, phBody);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetBody(
    IMimeMessage *iface,
    BODYLOCATION location,
    HBODY hPivot,
    LPHBODY phBody)
{
    FIXME("(%p)->(%d, %p, %p)\n", iface, location, hPivot, phBody);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_DeleteBody(
    IMimeMessage *iface,
    HBODY hBody,
    DWORD dwFlags)
{
    FIXME("(%p)->(%p, %08x)\n", iface, hBody, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_MoveBody(
    IMimeMessage *iface,
    HBODY hBody,
    BODYLOCATION location)
{
    FIXME("(%p)->(%d)\n", iface, location);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_CountBodies(
    IMimeMessage *iface,
    HBODY hParent,
    boolean fRecurse,
    ULONG *pcBodies)
{
    FIXME("(%p)->(%p, %s, %p)\n", iface, hParent, fRecurse ? "TRUE" : "FALSE", pcBodies);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_FindFirst(
    IMimeMessage *iface,
    LPFINDBODY pFindBody,
    LPHBODY phBody)
{
    FIXME("(%p)->(%p, %p)\n", iface, pFindBody, phBody);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_FindNext(
    IMimeMessage *iface,
    LPFINDBODY pFindBody,
    LPHBODY phBody)
{
    FIXME("(%p)->(%p, %p)\n", iface, pFindBody, phBody);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_ResolveURL(
    IMimeMessage *iface,
    HBODY hRelated,
    LPCSTR pszBase,
    LPCSTR pszURL,
    DWORD dwFlags,
    LPHBODY phBody)
{
    FIXME("(%p)->(%p, %s, %s, 0x%x, %p)\n", iface, hRelated, pszBase, pszURL, dwFlags, phBody);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_ToMultipart(
    IMimeMessage *iface,
    HBODY hBody,
    LPCSTR pszSubType,
    LPHBODY phMultipart)
{
    FIXME("(%p)->(%p, %s, %p)\n", iface, hBody, pszSubType, phMultipart);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetBodyOffsets(
    IMimeMessage *iface,
    HBODY hBody,
    LPBODYOFFSETS pOffsets)
{
    FIXME("(%p)->(%p, %p)\n", iface, hBody, pOffsets);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetCharset(
    IMimeMessage *iface,
    LPHCHARSET phCharset)
{
    FIXME("(%p)->(%p)\n", iface, phCharset);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_SetCharset(
    IMimeMessage *iface,
    HCHARSET hCharset,
    CSETAPPLYTYPE applytype)
{
    FIXME("(%p)->(%p, %d)\n", iface, hCharset, applytype);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_IsBodyType(
    IMimeMessage *iface,
    HBODY hBody,
    IMSGBODYTYPE bodytype)
{
    FIXME("(%p)->(%p, %d)\n", iface, hBody, bodytype);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_IsContentType(
    IMimeMessage *iface,
    HBODY hBody,
    LPCSTR pszPriType,
    LPCSTR pszSubType)
{
    FIXME("(%p)->(%p, %s, %s)\n", iface, hBody, pszPriType, pszSubType);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_QueryBodyProp(
    IMimeMessage *iface,
    HBODY hBody,
    LPCSTR pszName,
    LPCSTR pszCriteria,
    boolean fSubString,
    boolean fCaseSensitive)
{
    FIXME("(%p)->(%p, %s, %s, %s, %s)\n", iface, hBody, pszName, pszCriteria, fSubString ? "TRUE" : "FALSE", fCaseSensitive ? "TRUE" : "FALSE");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetBodyProp(
    IMimeMessage *iface,
    HBODY hBody,
    LPCSTR pszName,
    DWORD dwFlags,
    LPPROPVARIANT pValue)
{
    FIXME("(%p)->(%p, %s, 0x%x, %p)\n", iface, hBody, pszName, dwFlags, pValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_SetBodyProp(
    IMimeMessage *iface,
    HBODY hBody,
    LPCSTR pszName,
    DWORD dwFlags,
    LPCPROPVARIANT pValue)
{
    FIXME("(%p)->(%p, %s, 0x%x, %p)\n", iface, hBody, pszName, dwFlags, pValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_DeleteBodyProp(
    IMimeMessage *iface,
    HBODY hBody,
    LPCSTR pszName)
{
    FIXME("(%p)->(%p, %s)\n", iface, hBody, pszName);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_SetOption(
    IMimeMessage *iface,
    const TYPEDID oid,
    LPCPROPVARIANT pValue)
{
    FIXME("(%p)->(%d, %p)\n", iface, oid, pValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetOption(
    IMimeMessage *iface,
    const TYPEDID oid,
    LPPROPVARIANT pValue)
{
    FIXME("(%p)->(%d, %p)\n", iface, oid, pValue);
    return E_NOTIMPL;
}

/*** IMimeMessage methods ***/
static HRESULT WINAPI MimeMessage_CreateWebPage(
    IMimeMessage *iface,
    IStream *pRootStm,
    LPWEBPAGEOPTIONS pOptions,
    IMimeMessageCallback *pCallback,
    IMoniker **ppMoniker)
{
    FIXME("(%p)->(%p, %p, %p, %p)\n", iface, pRootStm, pOptions, pCallback, ppMoniker);
    *ppMoniker = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetProp(
    IMimeMessage *iface,
    LPCSTR pszName,
    DWORD dwFlags,
    LPPROPVARIANT pValue)
{
    FIXME("(%p)->(%s, 0x%x, %p)\n", iface, pszName, dwFlags, pValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_SetProp(
    IMimeMessage *iface,
    LPCSTR pszName,
    DWORD dwFlags,
    LPCPROPVARIANT pValue)
{
    FIXME("(%p)->(%s, 0x%x, %p)\n", iface, pszName, dwFlags, pValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_DeleteProp(
    IMimeMessage *iface,
    LPCSTR pszName)
{
    FIXME("(%p)->(%s)\n", iface, pszName);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_QueryProp(
    IMimeMessage *iface,
    LPCSTR pszName,
    LPCSTR pszCriteria,
    boolean fSubString,
    boolean fCaseSensitive)
{
    FIXME("(%p)->(%s, %s, %s, %s)\n", iface, pszName, pszCriteria, fSubString ? "TRUE" : "FALSE", fCaseSensitive ? "TRUE" : "FALSE");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetTextBody(
    IMimeMessage *iface,
    DWORD dwTxtType,
    ENCODINGTYPE ietEncoding,
    IStream **pStream,
    LPHBODY phBody)
{
    FIXME("(%p)->(%d, %d, %p, %p)\n", iface, dwTxtType, ietEncoding, pStream, phBody);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_SetTextBody(
    IMimeMessage *iface,
    DWORD dwTxtType,
    ENCODINGTYPE ietEncoding,
    HBODY hAlternative,
    IStream *pStream,
    LPHBODY phBody)
{
    FIXME("(%p)->(%d, %d, %p, %p, %p)\n", iface, dwTxtType, ietEncoding, hAlternative, pStream, phBody);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_AttachObject(
    IMimeMessage *iface,
    REFIID riid,
    void *pvObject,
    LPHBODY phBody)
{
    FIXME("(%p)->(%s, %p, %p)\n", iface, debugstr_guid(riid), pvObject, phBody);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_AttachFile(
    IMimeMessage *iface,
    LPCSTR pszFilePath,
    IStream *pstmFile,
    LPHBODY phBody)
{
    FIXME("(%p)->(%s, %p, %p)\n", iface, pszFilePath, pstmFile, phBody);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_AttachURL(
    IMimeMessage *iface,
    LPCSTR pszBase,
    LPCSTR pszURL,
    DWORD dwFlags,
    IStream *pstmURL,
    LPSTR *ppszCIDURL,
    LPHBODY phBody)
{
    FIXME("(%p)->(%s, %s, 0x%x, %p, %p, %p)\n", iface, pszBase, pszURL, dwFlags, pstmURL, ppszCIDURL, phBody);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetAttachments(
    IMimeMessage *iface,
    ULONG *pcAttach,
    LPHBODY *pprghAttach)
{
    FIXME("(%p)->(%p, %p)\n", iface, pcAttach, pprghAttach);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetAddressTable(
    IMimeMessage *iface,
    IMimeAddressTable **ppTable)
{
    FIXME("(%p)->(%p)\n", iface, ppTable);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetSender(
    IMimeMessage *iface,
    LPADDRESSPROPS pAddress)
{
    FIXME("(%p)->(%p)\n", iface, pAddress);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetAddressTypes(
    IMimeMessage *iface,
    DWORD dwAdrTypes,
    DWORD dwProps,
    LPADDRESSLIST pList)
{
    FIXME("(%p)->(%d, %d, %p)\n", iface, dwAdrTypes, dwProps, pList);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetAddressFormat(
    IMimeMessage *iface,
    DWORD dwAdrTypes,
    ADDRESSFORMAT format,
    LPSTR *ppszFormat)
{
    FIXME("(%p)->(%d, %d, %p)\n", iface, dwAdrTypes, format, ppszFormat);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_EnumAddressTypes(
    IMimeMessage *iface,
    DWORD dwAdrTypes,
    DWORD dwProps,
    IMimeEnumAddressTypes **ppEnum)
{
    FIXME("(%p)->(%d, %d, %p)\n", iface, dwAdrTypes, dwProps, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_SplitMessage(
    IMimeMessage *iface,
    ULONG cbMaxPart,
    IMimeMessageParts **ppParts)
{
    FIXME("(%p)->(%d, %p)\n", iface, cbMaxPart, ppParts);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetRootMoniker(
    IMimeMessage *iface,
    IMoniker **ppMoniker)
{
    FIXME("(%p)->(%p)\n", iface, ppMoniker);
    return E_NOTIMPL;
}

static const IMimeMessageVtbl MimeMessageVtbl =
{
    MimeMessage_QueryInterface,
    MimeMessage_AddRef,
    MimeMessage_Release,
    MimeMessage_GetClassID,
    MimeMessage_IsDirty,
    MimeMessage_Load,
    MimeMessage_Save,
    MimeMessage_GetSizeMax,
    MimeMessage_InitNew,
    MimeMessage_GetMessageSource,
    MimeMessage_GetMessageSize,
    MimeMessage_LoadOffsetTable,
    MimeMessage_SaveOffsetTable,
    MimeMessage_GetFlags,
    MimeMessage_Commit,
    MimeMessage_HandsOffStorage,
    MimeMessage_BindToObject,
    MimeMessage_SaveBody,
    MimeMessage_InsertBody,
    MimeMessage_GetBody,
    MimeMessage_DeleteBody,
    MimeMessage_MoveBody,
    MimeMessage_CountBodies,
    MimeMessage_FindFirst,
    MimeMessage_FindNext,
    MimeMessage_ResolveURL,
    MimeMessage_ToMultipart,
    MimeMessage_GetBodyOffsets,
    MimeMessage_GetCharset,
    MimeMessage_SetCharset,
    MimeMessage_IsBodyType,
    MimeMessage_IsContentType,
    MimeMessage_QueryBodyProp,
    MimeMessage_GetBodyProp,
    MimeMessage_SetBodyProp,
    MimeMessage_DeleteBodyProp,
    MimeMessage_SetOption,
    MimeMessage_GetOption,
    MimeMessage_CreateWebPage,
    MimeMessage_GetProp,
    MimeMessage_SetProp,
    MimeMessage_DeleteProp,
    MimeMessage_QueryProp,
    MimeMessage_GetTextBody,
    MimeMessage_SetTextBody,
    MimeMessage_AttachObject,
    MimeMessage_AttachFile,
    MimeMessage_AttachURL,
    MimeMessage_GetAttachments,
    MimeMessage_GetAddressTable,
    MimeMessage_GetSender,
    MimeMessage_GetAddressTypes,
    MimeMessage_GetAddressFormat,
    MimeMessage_EnumAddressTypes,
    MimeMessage_SplitMessage,
    MimeMessage_GetRootMoniker,
};

/***********************************************************************
 *              MimeOleCreateMessage (INETCOMM.@)
 */
HRESULT WINAPI MimeOleCreateMessage(IUnknown *pUnkOuter, IMimeMessage **ppMessage)
{
    MimeMessage *This;

    TRACE("(%p, %p)\n", pUnkOuter, ppMessage);

    if (pUnkOuter)
    {
        FIXME("outer unknown not supported yet\n");
        return E_NOTIMPL;
    }

    *ppMessage = NULL;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &MimeMessageVtbl;
    This->refs = 1;

    *ppMessage = (IMimeMessage *)&This->lpVtbl;
    return S_OK;
}

/***********************************************************************
 *              MimeOleSetCompatMode (INETCOMM.@)
 */
HRESULT WINAPI MimeOleSetCompatMode(DWORD dwMode)
{
    FIXME("(0x%x)\n", dwMode);
    return S_OK;
}

/***********************************************************************
 *              MimeOleCreateVirtualStream (INETCOMM.@)
 */
HRESULT WINAPI MimeOleCreateVirtualStream(IStream **ppStream)
{
    HRESULT hr;
    FIXME("(%p)\n", ppStream);

    hr = CreateStreamOnHGlobal(NULL, TRUE, ppStream);
    return hr;
}

typedef struct MimeSecurity
{
    const IMimeSecurityVtbl *lpVtbl;

    LONG refs;
} MimeSecurity;

static HRESULT WINAPI MimeSecurity_QueryInterface(
        IMimeSecurity* iface,
        REFIID riid,
        void** obj)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IMimeSecurity))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    FIXME("no interface for %s\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MimeSecurity_AddRef(
        IMimeSecurity* iface)
{
    MimeSecurity *This = (MimeSecurity *)iface;
    TRACE("(%p)->()\n", iface);
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI MimeSecurity_Release(
        IMimeSecurity* iface)
{
    MimeSecurity *This = (MimeSecurity *)iface;
    ULONG refs;

    TRACE("(%p)->()\n", iface);

    refs = InterlockedDecrement(&This->refs);
    if (!refs)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refs;
}

static HRESULT WINAPI MimeSecurity_InitNew(
        IMimeSecurity* iface)
{
    FIXME("(%p)->(): stub\n", iface);
    return S_OK;
}

static HRESULT WINAPI MimeSecurity_CheckInit(
        IMimeSecurity* iface)
{
    FIXME("(%p)->(): stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_EncodeMessage(
        IMimeSecurity* iface,
        IMimeMessageTree* pTree,
        DWORD dwFlags)
{
    FIXME("(%p)->(%p, %08x): stub\n", iface, pTree, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_EncodeBody(
        IMimeSecurity* iface,
        IMimeMessageTree* pTree,
        HBODY hEncodeRoot,
        DWORD dwFlags)
{
    FIXME("(%p)->(%p, %p, %08x): stub\n", iface, pTree, hEncodeRoot, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_DecodeMessage(
        IMimeSecurity* iface,
        IMimeMessageTree* pTree,
        DWORD dwFlags)
{
    FIXME("(%p)->(%p, %08x): stub\n", iface, pTree, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_DecodeBody(
        IMimeSecurity* iface,
        IMimeMessageTree* pTree,
        HBODY hDecodeRoot,
        DWORD dwFlags)
{
    FIXME("(%p)->(%p, %p, %08x): stub\n", iface, pTree, hDecodeRoot, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_EnumCertificates(
        IMimeSecurity* iface,
        HCAPICERTSTORE hc,
        DWORD dwUsage,
        PCX509CERT pPrev,
        PCX509CERT* ppCert)
{
    FIXME("(%p)->(%p, %08x, %p, %p): stub\n", iface, hc, dwUsage, pPrev, ppCert);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_GetCertificateName(
        IMimeSecurity* iface,
        const PCX509CERT pX509Cert,
        const CERTNAMETYPE cn,
        LPSTR* ppszName)
{
    FIXME("(%p)->(%p, %08x, %p): stub\n", iface, pX509Cert, cn, ppszName);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_GetMessageType(
        IMimeSecurity* iface,
        const HWND hwndParent,
        IMimeBody* pBody,
        DWORD* pdwSecType)
{
    FIXME("(%p)->(%p, %p, %p): stub\n", iface, hwndParent, pBody, pdwSecType);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_GetCertData(
        IMimeSecurity* iface,
        const PCX509CERT pX509Cert,
        const CERTDATAID dataid,
        LPPROPVARIANT pValue)
{
    FIXME("(%p)->(%p, %x, %p): stub\n", iface, pX509Cert, dataid, pValue);
    return E_NOTIMPL;
}


static const IMimeSecurityVtbl MimeSecurityVtbl =
{
    MimeSecurity_QueryInterface,
    MimeSecurity_AddRef,
    MimeSecurity_Release,
    MimeSecurity_InitNew,
    MimeSecurity_CheckInit,
    MimeSecurity_EncodeMessage,
    MimeSecurity_EncodeBody,
    MimeSecurity_DecodeMessage,
    MimeSecurity_DecodeBody,
    MimeSecurity_EnumCertificates,
    MimeSecurity_GetCertificateName,
    MimeSecurity_GetMessageType,
    MimeSecurity_GetCertData
};

/***********************************************************************
 *              MimeOleCreateSecurity (INETCOMM.@)
 */
HRESULT WINAPI MimeOleCreateSecurity(IMimeSecurity **ppSecurity)
{
    MimeSecurity *This;

    TRACE("(%p)\n", ppSecurity);

    *ppSecurity = NULL;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &MimeSecurityVtbl;
    This->refs = 1;

    *ppSecurity = (IMimeSecurity *)&This->lpVtbl;
    return S_OK;
}
