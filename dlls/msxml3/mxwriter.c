/*
 *    MXWriter implementation
 *
 * Copyright 2011 Nikolay Sivov for CodeWeaversы
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
#include "config.h"

#include <stdarg.h>
#ifdef HAVE_LIBXML2
# include <libxml/parser.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "ole2.h"

#include "msxml6.h"

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct _mxwriter
{
    IMXWriter IMXWriter_iface;
    LONG ref;
} mxwriter;

static inline mxwriter *impl_from_IMXWriter(IMXWriter *iface)
{
    return CONTAINING_RECORD(iface, mxwriter, IMXWriter_iface);
}

static HRESULT WINAPI mxwriter_QueryInterface(IMXWriter *iface, REFIID riid, void **obj)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if ( IsEqualGUID( riid, &IID_IMXWriter ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *obj = &This->IMXWriter_iface;
    }
    else
    {
        ERR("interface %s not implemented\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IMXWriter_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI mxwriter_AddRef(IMXWriter *iface)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    return ref;
}

static ULONG WINAPI mxwriter_Release(IMXWriter *iface)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI mxwriter_GetTypeInfoCount(IMXWriter *iface, UINT* pctinfo)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI mxwriter_GetTypeInfo(
    IMXWriter *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IMXWriter_tid, ppTInfo);
}

static HRESULT WINAPI mxwriter_GetIDsOfNames(
    IMXWriter *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    mxwriter *This = impl_from_IMXWriter( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IMXWriter_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI mxwriter_Invoke(
    IMXWriter *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    mxwriter *This = impl_from_IMXWriter( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IMXWriter_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &This->IMXWriter_iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI mxwriter_put_output(IMXWriter *iface, VARIANT dest)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&dest));
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_get_output(IMXWriter *iface, VARIANT *dest)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    FIXME("(%p)->(%p)\n", This, dest);
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_put_encoding(IMXWriter *iface, BSTR encoding)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    FIXME("(%p)->(%s)\n", This, debugstr_w(encoding));
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_get_encoding(IMXWriter *iface, BSTR *encoding)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    FIXME("(%p)->(%p)\n", This, encoding);
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_put_byteOrderMark(IMXWriter *iface, VARIANT_BOOL writeBOM)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    FIXME("(%p)->(%d)\n", This, writeBOM);
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_get_byteOrderMark(IMXWriter *iface, VARIANT_BOOL *writeBOM)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    FIXME("(%p)->(%p)\n", This, writeBOM);
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_put_indent(IMXWriter *iface, VARIANT_BOOL indent)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    FIXME("(%p)->(%d)\n", This, indent);
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_get_indent(IMXWriter *iface, VARIANT_BOOL *indent)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    FIXME("(%p)->(%p)\n", This, indent);
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_put_standalone(IMXWriter *iface, VARIANT_BOOL value)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    FIXME("(%p)->(%d)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_get_standalone(IMXWriter *iface, VARIANT_BOOL *value)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_put_omitXMLDeclaration(IMXWriter *iface, VARIANT_BOOL value)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    FIXME("(%p)->(%d)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_get_omitXMLDeclaration(IMXWriter *iface, VARIANT_BOOL *value)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_put_version(IMXWriter *iface, BSTR version)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    FIXME("(%p)->(%s)\n", This, debugstr_w(version));
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_get_version(IMXWriter *iface, BSTR *version)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    FIXME("(%p)->(%p)\n", This, version);
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_put_disableOutputEscaping(IMXWriter *iface, VARIANT_BOOL value)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    FIXME("(%p)->(%d)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_get_disableOutputEscaping(IMXWriter *iface, VARIANT_BOOL *value)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_flush(IMXWriter *iface)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const struct IMXWriterVtbl mxwriter_vtbl =
{
    mxwriter_QueryInterface,
    mxwriter_AddRef,
    mxwriter_Release,
    mxwriter_GetTypeInfoCount,
    mxwriter_GetTypeInfo,
    mxwriter_GetIDsOfNames,
    mxwriter_Invoke,
    mxwriter_put_output,
    mxwriter_get_output,
    mxwriter_put_encoding,
    mxwriter_get_encoding,
    mxwriter_put_byteOrderMark,
    mxwriter_get_byteOrderMark,
    mxwriter_put_indent,
    mxwriter_get_indent,
    mxwriter_put_standalone,
    mxwriter_get_standalone,
    mxwriter_put_omitXMLDeclaration,
    mxwriter_get_omitXMLDeclaration,
    mxwriter_put_version,
    mxwriter_get_version,
    mxwriter_put_disableOutputEscaping,
    mxwriter_get_disableOutputEscaping,
    mxwriter_flush
};

HRESULT MXWriter_create(IUnknown *pUnkOuter, void **ppObj)
{
    mxwriter *This;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    if (pUnkOuter) FIXME("support aggregation, outer\n");

    This = heap_alloc( sizeof (*This) );
    if(!This)
        return E_OUTOFMEMORY;

    This->IMXWriter_iface.lpVtbl = &mxwriter_vtbl;
    This->ref = 1;

    *ppObj = &This->IMXWriter_iface;

    TRACE("returning iface %p\n", *ppObj);

    return S_OK;
}
