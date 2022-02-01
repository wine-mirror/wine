/*
 * Copyright 2022 Esme Povirk
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wine/debug.h"
#include "wine/heap.h"

#include "diasymreader_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(diasymreader);

typedef struct SymWriter {
    ISymUnmanagedWriter5 iface;
    LONG ref;
} SymWriter;

static inline SymWriter *impl_from_ISymUnmanagedWriter5(ISymUnmanagedWriter5 *iface)
{
    return CONTAINING_RECORD(iface, SymWriter, iface);
}

static HRESULT WINAPI SymWriter_QueryInterface(ISymUnmanagedWriter5 *iface, REFIID iid,
    void **ppv)
{
    SymWriter *This = impl_from_ISymUnmanagedWriter5(iface);

    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_ISymUnmanagedWriter, iid) ||
        IsEqualIID(&IID_ISymUnmanagedWriter2, iid) ||
        IsEqualIID(&IID_ISymUnmanagedWriter3, iid) ||
        IsEqualIID(&IID_ISymUnmanagedWriter4, iid) ||
        IsEqualIID(&IID_ISymUnmanagedWriter5, iid))
    {
        *ppv = &This->iface;
    }
    else
    {
        WARN("unknown interface %s\n", debugstr_guid(iid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI SymWriter_AddRef(ISymUnmanagedWriter5 *iface)
{
    SymWriter *This = impl_from_ISymUnmanagedWriter5(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI SymWriter_Release(ISymUnmanagedWriter5 *iface)
{
    SymWriter *This = impl_from_ISymUnmanagedWriter5(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI SymWriter_DefineDocument(ISymUnmanagedWriter5 *iface, const WCHAR *url,
    const GUID *language, const GUID *languageVendor, const GUID *documentType,
    ISymUnmanagedDocumentWriter** pRetVal)
{
    FIXME("(%p,%s,%s,%s,%s,%p)\n", iface, debugstr_w(url), debugstr_guid(language),
        debugstr_guid(languageVendor), debugstr_guid(documentType), pRetVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_SetUserEntryPoint(ISymUnmanagedWriter5 *iface, mdMethodDef entryMethod)
{
    FIXME("(%p,0x%x)\n", iface, entryMethod);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_OpenMethod(ISymUnmanagedWriter5 *iface, mdMethodDef method)
{
    FIXME("(%p,0x%x)\n", iface, method);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_CloseMethod(ISymUnmanagedWriter5 *iface)
{
    FIXME("(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_OpenScope(ISymUnmanagedWriter5 *iface, ULONG32 startOffset,
    ULONG32 *pRetVal)
{
    FIXME("(%p,%u,%p)\n", iface, startOffset, pRetVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_CloseScope(ISymUnmanagedWriter5 *iface, ULONG32 endOffset)
{
    FIXME("(%p,%u)\n", iface, endOffset);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_SetScopeRange(ISymUnmanagedWriter5 *iface, ULONG32 scopeID, ULONG32 startOffset,
    ULONG32 endOffset)
{
    FIXME("(%p,%u,%u,%u)\n", iface, scopeID, startOffset, endOffset);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_DefineLocalVariable(ISymUnmanagedWriter5 *iface, const WCHAR *name,
    ULONG32 attributes, ULONG32 cSig, unsigned char signature[], ULONG32 addrKind,
    ULONG32 addr1, ULONG32 addr2, ULONG32 addr3, ULONG32 startOffset, ULONG32 endOffset)
{
    FIXME("(%p,%s,0x%x,%u,%u)\n", iface, debugstr_w(name), attributes, cSig, addrKind);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_DefineParameter(ISymUnmanagedWriter5 *iface, const WCHAR *name,
    ULONG32 attributes, ULONG32 sequence, ULONG32 addrKind,
    ULONG32 addr1, ULONG32 addr2, ULONG32 addr3)
{
    FIXME("(%p,%s,0x%x,%u,%u)\n", iface, debugstr_w(name), attributes, sequence, addrKind);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_DefineField(ISymUnmanagedWriter5 *iface, mdTypeDef parent,
    const WCHAR *name, ULONG32 attributes, ULONG32 cSig, unsigned char signature[], ULONG32 addrKind,
    ULONG32 addr1, ULONG32 addr2, ULONG32 addr3)
{
    FIXME("(%p,0x%x,%s,0x%x,%u,%u)\n", iface, parent, debugstr_w(name), attributes, cSig, addrKind);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_DefineGlobalVariable(ISymUnmanagedWriter5 *iface, const WCHAR *name,
    ULONG32 attributes, ULONG32 cSig, unsigned char signature[], ULONG32 addrKind,
    ULONG32 addr1, ULONG32 addr2, ULONG32 addr3)
{
    FIXME("(%p,%s,0x%x,%u,%u)\n", iface, debugstr_w(name), attributes, cSig, addrKind);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_Close(ISymUnmanagedWriter5 *iface)
{
    FIXME("(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_SetSymAttributes(ISymUnmanagedWriter5 *iface, mdToken parent,
    const WCHAR *name, ULONG32 cData, unsigned char data[])
{
    FIXME("(%p,0x%x,%s,%u)\n", iface, parent, debugstr_w(name), cData);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_OpenNamespace(ISymUnmanagedWriter5 *iface, const WCHAR *name)
{
    FIXME("(%p,%s)\n", iface, debugstr_w(name));
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_CloseNamespace(ISymUnmanagedWriter5 *iface)
{
    FIXME("(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_UsingNamespace(ISymUnmanagedWriter5 *iface, const WCHAR *fullName)
{
    FIXME("(%p,%s)\n", iface, debugstr_w(fullName));
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_SetMethodSourceRange(ISymUnmanagedWriter5 *iface, ISymUnmanagedDocumentWriter *startDoc,
    ULONG32 startLine, ULONG32 startColumn, ISymUnmanagedDocumentWriter *endDoc, ULONG32 endLine, ULONG32 endColumn)
{
    FIXME("(%p,%p,%u,%u,%p,%u,%u)\n", iface, startDoc, startLine, startColumn, endDoc, endLine, endColumn);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_Initialize(ISymUnmanagedWriter5 *iface, IUnknown *emitter, const WCHAR *filename,
    IStream *pIStream, BOOL fFullBuild)
{
    FIXME("(%p,%p,%s,%p,%u)\n", iface, emitter, debugstr_w(filename), pIStream, fFullBuild);
    return S_OK;
}

static HRESULT WINAPI SymWriter_GetDebugInfo(ISymUnmanagedWriter5 *iface, IMAGE_DEBUG_DIRECTORY *pIDD, DWORD cData,
    DWORD *pcData, BYTE data[])
{
    FIXME("(%p,%p,%lu,%p,%p)\n", iface, pIDD, cData, pcData, data);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_DefineSequencePoints(ISymUnmanagedWriter5 *iface, ISymUnmanagedDocumentWriter *document,
    ULONG32 spCount, ULONG32 offsets[], ULONG32 lines[], ULONG32 columns[], ULONG32 endLines[], ULONG32 endColumns[])
{
    FIXME("(%p,%p,%u)\n", iface, document, spCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_RemapToken(ISymUnmanagedWriter5 *iface, mdToken oldToken, mdToken newToken)
{
    FIXME("(%p,0x%x,0x%x)\n", iface, oldToken, newToken);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_Initialize2(ISymUnmanagedWriter5 *iface, IUnknown *emitter, const WCHAR *tempFilename,
    IStream *pIStream, BOOL fFullBuild, const WCHAR *finalFilename)
{
    FIXME("(%p,%p,%s,%p,%u,%s)\n", iface, emitter, debugstr_w(tempFilename), pIStream, fFullBuild, debugstr_w(finalFilename));
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_DefineConstant(ISymUnmanagedWriter5 *iface, const WCHAR *name, VARIANT value, ULONG32 cSig,
    unsigned char signature[])
{
    FIXME("(%p,%s,%s,%u,%p)\n", iface, debugstr_w(name), debugstr_variant(&value), cSig, signature);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_Abort(ISymUnmanagedWriter5 *iface)
{
    FIXME("(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_DefineLocalVariable2(ISymUnmanagedWriter5 *iface, const WCHAR *name, ULONG32 attributes,
    mdSignature sigToken, ULONG32 addrKind, ULONG32 addr1, ULONG32 addr2, ULONG32 addr3,
    ULONG32 startOffset, ULONG32 endOffset)
{
    FIXME("(%p,%s,0x%x,0x%x,%u)\n", iface, debugstr_w(name), attributes, sigToken, addrKind);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_DefineGlobalVariable2(ISymUnmanagedWriter5 *iface, const WCHAR *name, ULONG32 attributes,
    mdSignature sigToken, ULONG32 addrKind, ULONG32 addr1, ULONG32 addr2, ULONG32 addr3)
{
    FIXME("(%p,%s,0x%x,0x%x,%u)\n", iface, debugstr_w(name), attributes, sigToken, addrKind);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_DefineConstant2(ISymUnmanagedWriter5 *iface, const WCHAR *name, VARIANT value, mdSignature sigToken)
{
    FIXME("(%p,%s,%s,0x%x)\n", iface, debugstr_w(name), debugstr_variant(&value), sigToken);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_OpenMethod2(ISymUnmanagedWriter5 *iface, mdMethodDef method, ULONG32 isect, ULONG32 offset)
{
    FIXME("(%p,0x%x,%u,%u)\n", iface, method, isect, offset);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_Commit(ISymUnmanagedWriter5 *iface)
{
    FIXME("(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_GetDebugInfoWithPadding(ISymUnmanagedWriter5 *iface, IMAGE_DEBUG_DIRECTORY *pIDD, DWORD cbData,
    DWORD* pcData, BYTE data[])
{
    FIXME("(%p,%p,%lu,%p,%p)\n", iface, pIDD, cbData, pcData, data);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_OpenMapTokensToSourceSpans(ISymUnmanagedWriter5 *iface)
{
    FIXME("(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_CloseMapTokensToSourceSpans(ISymUnmanagedWriter5 *iface)
{
    FIXME("(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI SymWriter_MapTokenToSourceSpan(ISymUnmanagedWriter5 *iface, mdToken token, ISymUnmanagedDocumentWriter* document,
                                 ULONG32 line, ULONG32 column, ULONG32 endLine, ULONG32 endColumn)
{
    FIXME("(%p,%x,%p,%u,%u,%u,%u)\n", iface, token, document, line, column, endLine, endColumn);
    return E_NOTIMPL;
}


static const ISymUnmanagedWriter5Vtbl SymWriter_Vtbl = {
    SymWriter_QueryInterface,
    SymWriter_AddRef,
    SymWriter_Release,
    SymWriter_DefineDocument,
    SymWriter_SetUserEntryPoint,
    SymWriter_OpenMethod,
    SymWriter_CloseMethod,
    SymWriter_OpenScope,
    SymWriter_CloseScope,
    SymWriter_SetScopeRange,
    SymWriter_DefineLocalVariable,
    SymWriter_DefineParameter,
    SymWriter_DefineField,
    SymWriter_DefineGlobalVariable,
    SymWriter_Close,
    SymWriter_SetSymAttributes,
    SymWriter_OpenNamespace,
    SymWriter_CloseNamespace,
    SymWriter_UsingNamespace,
    SymWriter_SetMethodSourceRange,
    SymWriter_Initialize,
    SymWriter_GetDebugInfo,
    SymWriter_DefineSequencePoints,
    SymWriter_RemapToken,
    SymWriter_Initialize2,
    SymWriter_DefineConstant,
    SymWriter_Abort,
    SymWriter_DefineLocalVariable2,
    SymWriter_DefineGlobalVariable2,
    SymWriter_DefineConstant2,
    SymWriter_OpenMethod2,
    SymWriter_Commit,
    SymWriter_GetDebugInfoWithPadding,
    SymWriter_OpenMapTokensToSourceSpans,
    SymWriter_CloseMapTokensToSourceSpans,
    SymWriter_MapTokenToSourceSpan
};

HRESULT SymWriter_CreateInstance(REFIID iid, void **ppv)
{
    SymWriter *This;
    HRESULT hr;

    This = heap_alloc(sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->iface.lpVtbl = &SymWriter_Vtbl;
    This->ref = 1;

    hr = IUnknown_QueryInterface(&This->iface, iid, ppv);

    ISymUnmanagedWriter5_Release(&This->iface);

    return hr;
}
