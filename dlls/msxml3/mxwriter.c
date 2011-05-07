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

#ifdef HAVE_LIBXML2

static const char crlfA[] = "\r\n";

typedef enum
{
    MXWriter_BOM = 0,
    MXWriter_DisableEscaping,
    MXWriter_Indent,
    MXWriter_OmitXmlDecl,
    MXWriter_Standalone,
    MXWriter_LastProp
} MXWRITER_PROPS;

typedef struct _mxwriter
{
    IMXWriter IMXWriter_iface;
    ISAXContentHandler ISAXContentHandler_iface;

    LONG ref;

    VARIANT_BOOL props[MXWriter_LastProp];
    BSTR encoding;
    BSTR version;

    IStream *dest;

    xmlOutputBufferPtr buffer;
} mxwriter;

static inline mxwriter *impl_from_IMXWriter(IMXWriter *iface)
{
    return CONTAINING_RECORD(iface, mxwriter, IMXWriter_iface);
}

static inline mxwriter *impl_from_ISAXContentHandler(ISAXContentHandler *iface)
{
    return CONTAINING_RECORD(iface, mxwriter, ISAXContentHandler_iface);
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
    else if ( IsEqualGUID( riid, &IID_ISAXContentHandler ) )
    {
        *obj = &This->ISAXContentHandler_iface;
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
    {
        if (This->dest) IStream_Release(This->dest);
        SysFreeString(This->encoding);
        SysFreeString(This->version);

        xmlOutputBufferClose(This->buffer);
        heap_free(This);
    }

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

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&dest));

    switch (V_VT(&dest))
    {
    case VT_EMPTY:
    {
        if (This->dest) IStream_Release(This->dest);
        This->dest = NULL;
        break;
    }
    case VT_UNKNOWN:
    {
        IStream *stream;
        HRESULT hr;

        hr = IUnknown_QueryInterface(V_UNKNOWN(&dest), &IID_IStream, (void**)&stream);
        if (hr == S_OK)
        {
            if (This->dest) IStream_Release(This->dest);
            This->dest = stream;
            break;
        }

        FIXME("unhandled interface type for VT_UNKNOWN destination\n");
        return E_NOTIMPL;
    }
    default:
        FIXME("unhandled destination type %s\n", debugstr_variant(&dest));
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI mxwriter_get_output(IMXWriter *iface, VARIANT *dest)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%p)\n", This, dest);

    if (!This->dest)
    {
        V_VT(dest)   = VT_BSTR;
        V_BSTR(dest) = bstr_from_xmlChar(This->buffer->buffer->content);

        return S_OK;
    }
    else
        FIXME("not implemented when stream is set up\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_put_encoding(IMXWriter *iface, BSTR encoding)
{
    static const WCHAR utf16W[] = {'U','T','F','-','1','6',0};
    static const WCHAR utf8W[]  = {'U','T','F','-','8',0};
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%s)\n", This, debugstr_w(encoding));

    /* FIXME: filter all supported encodings */
    if (!strcmpW(encoding, utf16W) || !strcmpW(encoding, utf8W))
    {
        SysFreeString(This->encoding);
        This->encoding = SysAllocString(encoding);
        return S_OK;
    }
    else
    {
        FIXME("unsupported encoding %s\n", debugstr_w(encoding));
        return E_INVALIDARG;
    }
}

static HRESULT WINAPI mxwriter_get_encoding(IMXWriter *iface, BSTR *encoding)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%p)\n", This, encoding);

    if (!encoding) return E_POINTER;

    return return_bstr(This->encoding, encoding);
}

static HRESULT WINAPI mxwriter_put_byteOrderMark(IMXWriter *iface, VARIANT_BOOL value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%d)\n", This, value);
    This->props[MXWriter_BOM] = value;

    return S_OK;
}

static HRESULT WINAPI mxwriter_get_byteOrderMark(IMXWriter *iface, VARIANT_BOOL *value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%p)\n", This, value);

    if (!value) return E_POINTER;

    *value = This->props[MXWriter_BOM];

    return S_OK;
}

static HRESULT WINAPI mxwriter_put_indent(IMXWriter *iface, VARIANT_BOOL value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%d)\n", This, value);
    This->props[MXWriter_Indent] = value;

    return S_OK;
}

static HRESULT WINAPI mxwriter_get_indent(IMXWriter *iface, VARIANT_BOOL *value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%p)\n", This, value);

    if (!value) return E_POINTER;

    *value = This->props[MXWriter_Indent];

    return S_OK;
}

static HRESULT WINAPI mxwriter_put_standalone(IMXWriter *iface, VARIANT_BOOL value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%d)\n", This, value);
    This->props[MXWriter_Standalone] = value;

    return S_OK;
}

static HRESULT WINAPI mxwriter_get_standalone(IMXWriter *iface, VARIANT_BOOL *value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%p)\n", This, value);

    if (!value) return E_POINTER;

    *value = This->props[MXWriter_Standalone];

    return S_OK;
}

static HRESULT WINAPI mxwriter_put_omitXMLDeclaration(IMXWriter *iface, VARIANT_BOOL value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%d)\n", This, value);
    This->props[MXWriter_OmitXmlDecl] = value;

    return S_OK;
}

static HRESULT WINAPI mxwriter_get_omitXMLDeclaration(IMXWriter *iface, VARIANT_BOOL *value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%p)\n", This, value);

    if (!value) return E_POINTER;

    *value = This->props[MXWriter_OmitXmlDecl];

    return S_OK;
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

    TRACE("(%p)->(%p)\n", This, version);

    if (!version) return E_POINTER;

    return return_bstr(This->version, version);
}

static HRESULT WINAPI mxwriter_put_disableOutputEscaping(IMXWriter *iface, VARIANT_BOOL value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%d)\n", This, value);
    This->props[MXWriter_DisableEscaping] = value;

    return S_OK;
}

static HRESULT WINAPI mxwriter_get_disableOutputEscaping(IMXWriter *iface, VARIANT_BOOL *value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%p)\n", This, value);

    if (!value) return E_POINTER;

    *value = This->props[MXWriter_DisableEscaping];

    return S_OK;
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

/*** ISAXContentHandler ***/
static HRESULT WINAPI mxwriter_saxcontent_QueryInterface(
    ISAXContentHandler *iface,
    REFIID riid,
    void **obj)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    return IMXWriter_QueryInterface(&This->IMXWriter_iface, riid, obj);
}

static ULONG WINAPI mxwriter_saxcontent_AddRef(ISAXContentHandler *iface)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    return IMXWriter_AddRef(&This->IMXWriter_iface);
}

static ULONG WINAPI mxwriter_saxcontent_Release(ISAXContentHandler *iface)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    return IMXWriter_Release(&This->IMXWriter_iface);
}

static HRESULT WINAPI mxwriter_saxcontent_putDocumentLocator(
    ISAXContentHandler *iface,
    ISAXLocator *locator)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    FIXME("(%p)->(%p)\n", This, locator);
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_saxcontent_startDocument(ISAXContentHandler *iface)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    xmlChar *s;

    TRACE("(%p)\n", This);

    if (This->props[MXWriter_OmitXmlDecl] == VARIANT_TRUE) return S_OK;

    /* version */
    xmlOutputBufferWriteString(This->buffer, "<?xml version=\"");
    s = xmlchar_from_wchar(This->version);
    xmlOutputBufferWriteString(This->buffer, (char*)s);
    heap_free(s);
    xmlOutputBufferWriteString(This->buffer, "\"");

    /* encoding */
    xmlOutputBufferWriteString(This->buffer, " encoding=\"");
    s = xmlchar_from_wchar(This->encoding);
    xmlOutputBufferWriteString(This->buffer, (char*)s);
    heap_free(s);
    xmlOutputBufferWriteString(This->buffer, "\"");

    /* standalone */
    xmlOutputBufferWriteString(This->buffer, " standalone=\"");
    if (This->props[MXWriter_Standalone] == VARIANT_TRUE)
        xmlOutputBufferWriteString(This->buffer, "yes\"?>");
    else
        xmlOutputBufferWriteString(This->buffer, "no\"?>");

    xmlOutputBufferWriteString(This->buffer, crlfA);

    return S_OK;
}

static HRESULT WINAPI mxwriter_saxcontent_endDocument(ISAXContentHandler *iface)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_saxcontent_startPrefixMapping(
    ISAXContentHandler *iface,
    const WCHAR *prefix,
    int nprefix,
    const WCHAR *uri,
    int nuri)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    FIXME("(%p)->(%s %s)\n", This, debugstr_wn(prefix, nprefix), debugstr_wn(uri, nuri));
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_saxcontent_endPrefixMapping(
    ISAXContentHandler *iface,
    const WCHAR *prefix,
    int nprefix)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    FIXME("(%p)->(%s)\n", This, debugstr_wn(prefix, nprefix));
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_saxcontent_startElement(
    ISAXContentHandler *iface,
    const WCHAR *namespaceUri,
    int nnamespaceUri,
    const WCHAR *local_name,
    int nlocal_name,
    const WCHAR *QName,
    int nQName,
    ISAXAttributes *attr)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    xmlChar *s;

    TRACE("(%p)->(%s %s %s %p)\n", This, debugstr_wn(namespaceUri, nnamespaceUri),
        debugstr_wn(local_name, nlocal_name), debugstr_wn(QName, nQName), attr);

    if (!namespaceUri || !local_name || !QName) return E_INVALIDARG;

    xmlOutputBufferWriteString(This->buffer, "<");
    s = xmlchar_from_wchar(QName);
    xmlOutputBufferWriteString(This->buffer, (char*)s);
    heap_free(s);

    if (attr)
    {
        HRESULT hr;
        INT length;
        INT i;

        hr = ISAXAttributes_getLength(attr, &length);
        if (FAILED(hr)) return hr;

        if (length) xmlOutputBufferWriteString(This->buffer, " ");

        for (i = 0; i < length; i++)
        {
            const WCHAR *str;
            INT len;

            hr = ISAXAttributes_getQName(attr, i, &str, &len);
            if (FAILED(hr)) return hr;

            s = xmlchar_from_wchar(str);
            xmlOutputBufferWriteString(This->buffer, (char*)s);
            heap_free(s);

            xmlOutputBufferWriteString(This->buffer, "=\"");

            hr = ISAXAttributes_getValue(attr, i, &str, &len);
            if (FAILED(hr)) return hr;

            s = xmlchar_from_wchar(str);
            xmlOutputBufferWriteString(This->buffer, (char*)s);
            heap_free(s);

            xmlOutputBufferWriteString(This->buffer, "\"");
        }
    }

    xmlOutputBufferWriteString(This->buffer, ">");

    return S_OK;
}

static HRESULT WINAPI mxwriter_saxcontent_endElement(
    ISAXContentHandler *iface,
    const WCHAR *namespaceUri,
    int nnamespaceUri,
    const WCHAR * local_name,
    int nlocal_name,
    const WCHAR *QName,
    int nQName)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    xmlChar *s;

    TRACE("(%p)->(%s %s %s)\n", This, debugstr_wn(namespaceUri, nnamespaceUri),
        debugstr_wn(local_name, nlocal_name), debugstr_wn(QName, nQName));

    if (!namespaceUri || !local_name || !QName) return E_INVALIDARG;

    xmlOutputBufferWriteString(This->buffer, "</");
    s = xmlchar_from_wchar(QName);
    xmlOutputBufferWriteString(This->buffer, (char*)s);
    heap_free(s);
    xmlOutputBufferWriteString(This->buffer, ">");

    return S_OK;
}

static HRESULT WINAPI mxwriter_saxcontent_characters(
    ISAXContentHandler *iface,
    const WCHAR *chars,
    int nchars)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );

    TRACE("(%p)->(%s:%d)\n", This, debugstr_wn(chars, nchars), nchars);

    if (!chars) return E_INVALIDARG;

    if (nchars)
    {
        xmlChar *s = xmlchar_from_wcharn(chars, nchars);
        xmlOutputBufferWriteString(This->buffer, (char*)s);
        heap_free(s);
    }

    return S_OK;
}

static HRESULT WINAPI mxwriter_saxcontent_ignorableWhitespace(
    ISAXContentHandler *iface,
    const WCHAR *chars,
    int nchars)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    FIXME("(%p)->(%s)\n", This, debugstr_wn(chars, nchars));
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_saxcontent_processingInstruction(
    ISAXContentHandler *iface,
    const WCHAR *target,
    int ntarget,
    const WCHAR *data,
    int ndata)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    FIXME("(%p)->(%s %s)\n", This, debugstr_wn(target, ntarget), debugstr_wn(data, ndata));
    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_saxcontent_skippedEntity(
    ISAXContentHandler *iface,
    const WCHAR *name,
    int nname)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    FIXME("(%p)->(%s)\n", This, debugstr_wn(name, nname));
    return E_NOTIMPL;
}

static const struct ISAXContentHandlerVtbl mxwriter_saxcontent_vtbl =
{
    mxwriter_saxcontent_QueryInterface,
    mxwriter_saxcontent_AddRef,
    mxwriter_saxcontent_Release,
    mxwriter_saxcontent_putDocumentLocator,
    mxwriter_saxcontent_startDocument,
    mxwriter_saxcontent_endDocument,
    mxwriter_saxcontent_startPrefixMapping,
    mxwriter_saxcontent_endPrefixMapping,
    mxwriter_saxcontent_startElement,
    mxwriter_saxcontent_endElement,
    mxwriter_saxcontent_characters,
    mxwriter_saxcontent_ignorableWhitespace,
    mxwriter_saxcontent_processingInstruction,
    mxwriter_saxcontent_skippedEntity
};

HRESULT MXWriter_create(IUnknown *pUnkOuter, void **ppObj)
{
    static const WCHAR utf16W[] = {'U','T','F','-','1','6',0};
    static const WCHAR version10W[] = {'1','.','0',0};
    mxwriter *This;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    if (pUnkOuter) FIXME("support aggregation, outer\n");

    This = heap_alloc( sizeof (*This) );
    if(!This)
        return E_OUTOFMEMORY;

    This->IMXWriter_iface.lpVtbl = &mxwriter_vtbl;
    This->ISAXContentHandler_iface.lpVtbl = &mxwriter_saxcontent_vtbl;
    This->ref = 1;

    This->props[MXWriter_BOM] = VARIANT_TRUE;
    This->props[MXWriter_DisableEscaping] = VARIANT_FALSE;
    This->props[MXWriter_Indent] = VARIANT_FALSE;
    This->props[MXWriter_OmitXmlDecl] = VARIANT_FALSE;
    This->props[MXWriter_Standalone] = VARIANT_FALSE;
    This->encoding   = SysAllocString(utf16W);
    This->version    = SysAllocString(version10W);

    This->dest = NULL;

    /* set up a buffer, default encoding is UTF-16 */
    This->buffer = xmlAllocOutputBuffer(xmlFindCharEncodingHandler("UTF-16"));

    *ppObj = &This->IMXWriter_iface;

    TRACE("returning iface %p\n", *ppObj);

    return S_OK;
}

#else

HRESULT MXWriter_create(IUnknown *pUnkOuter, void **obj)
{
    MESSAGE("This program tried to use a MXXMLWriter object, but\n"
            "libxml2 support was not present at compile time.\n");
    return E_NOTIMPL;
}

#endif /* HAVE_LIBXML2 */
