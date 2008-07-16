/*
 *    SAX Reader implementation
 *
 * Copyright 2008 Alistair Leslie-Hughes
 * Copyright 2008 Piotr Caban
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
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"
#include "msxml2.h"
#include "wininet.h"
#include "urlmon.h"
#include "winreg.h"
#include "shlwapi.h"

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

#include <libxml/SAX2.h>

typedef struct _saxreader
{
    const struct IVBSAXXMLReaderVtbl *lpVtbl;
    const struct ISAXXMLReaderVtbl *lpSAXXMLReaderVtbl;
    LONG ref;
    struct ISAXContentHandler *contentHandler;
    struct ISAXErrorHandler *errorHandler;
    xmlSAXHandler sax;
} saxreader;

typedef struct _saxlocator
{
    const struct ISAXLocatorVtbl *lpSAXLocatorVtbl;
    LONG ref;
    saxreader *saxreader;
    HRESULT ret;
    xmlParserCtxtPtr pParserCtxt;
    int lastLine;
    int lastColumn;
} saxlocator;

static inline saxreader *impl_from_IVBSAXXMLReader( IVBSAXXMLReader *iface )
{
    return (saxreader *)((char*)iface - FIELD_OFFSET(saxreader, lpVtbl));
}

static inline saxreader *impl_from_ISAXXMLReader( ISAXXMLReader *iface )
{
    return (saxreader *)((char*)iface - FIELD_OFFSET(saxreader, lpSAXXMLReaderVtbl));
}

static inline saxlocator *impl_from_ISAXLocator( ISAXLocator *iface )
{
    return (saxlocator *)((char*)iface - FIELD_OFFSET(saxlocator, lpSAXLocatorVtbl));
}

/*** LibXML callbacks ***/
static void libxmlStartDocument(void *ctx)
{
    saxlocator *This = ctx;
    HRESULT hr;

    if(This->saxreader->contentHandler)
    {
        hr = ISAXContentHandler_startDocument(This->saxreader->contentHandler);
        if(FAILED(hr))
        {
            xmlStopParser(This->pParserCtxt);
            This->ret = hr;
        }
    }

    This->lastColumn = xmlSAX2GetColumnNumber(This->pParserCtxt);
    This->lastLine = xmlSAX2GetLineNumber(This->pParserCtxt);
}

static void libxmlEndDocument(void *ctx)
{
    saxlocator *This = ctx;
    HRESULT hr;

    This->lastColumn = 0;
    This->lastLine = 0;

    if(This->saxreader->contentHandler)
    {
        hr = ISAXContentHandler_endDocument(This->saxreader->contentHandler);
        if(FAILED(hr))
        {
            xmlStopParser(This->pParserCtxt);
            This->ret = hr;
        }
    }
}

static void libxmlStartElementNS(
        void *ctx,
        const xmlChar *localname,
        const xmlChar *prefix,
        const xmlChar *URI,
        int nb_namespaces,
        const xmlChar **namespaces,
        int nb_attributes,
        int nb_defaulted,
        const xmlChar **attributes)
{
    BSTR NamespaceUri, LocalName, QName;
    saxlocator *This = ctx;
    HRESULT hr;

    FIXME("Arguments processing not yet implemented.\n");

    This->lastColumn = xmlSAX2GetColumnNumber(This->pParserCtxt)+1;
    This->lastLine = xmlSAX2GetLineNumber(This->pParserCtxt);

    if(This->saxreader->contentHandler)
    {
        NamespaceUri = bstr_from_xmlChar(URI);
        LocalName = bstr_from_xmlChar(localname);
        QName = bstr_from_xmlChar(localname);

        hr = ISAXContentHandler_startElement(
                This->saxreader->contentHandler,
                NamespaceUri, SysStringLen(NamespaceUri),
                LocalName, SysStringLen(LocalName),
                QName, SysStringLen(QName),
                NULL);

        SysFreeString(NamespaceUri);
        SysFreeString(LocalName);
        SysFreeString(QName);

        if(FAILED(hr))
        {
            xmlStopParser(This->pParserCtxt);
            This->ret = hr;
        }
    }
}

static void libxmlEndElementNS(
        void *ctx,
        const xmlChar *localname,
        const xmlChar *prefix,
        const xmlChar *URI)
{
    BSTR NamespaceUri, LocalName, QName;
    saxlocator *This = ctx;
    HRESULT hr;

    This->lastColumn = xmlSAX2GetColumnNumber(This->pParserCtxt);
    This->lastLine = xmlSAX2GetLineNumber(This->pParserCtxt);

    if(This->saxreader->contentHandler)
    {
        NamespaceUri = bstr_from_xmlChar(URI);
        LocalName = bstr_from_xmlChar(localname);
        QName = bstr_from_xmlChar(localname);

        hr = ISAXContentHandler_endElement(
                This->saxreader->contentHandler,
                NamespaceUri, SysStringLen(NamespaceUri),
                LocalName, SysStringLen(LocalName),
                QName, SysStringLen(QName));

        SysFreeString(NamespaceUri);
        SysFreeString(LocalName);
        SysFreeString(QName);

        if(FAILED(hr))
        {
            xmlStopParser(This->pParserCtxt);
            This->ret = hr;
        }
    }
}

static void libxmlCharacters(
        void *ctx,
        const xmlChar *ch,
        int len)
{
    BSTR Chars;
    saxlocator *This = ctx;
    const xmlChar *cur;
    int pos;
    HRESULT hr;

    This->lastColumn = 1;
    This->lastLine = xmlSAX2GetLineNumber(This->pParserCtxt);

    cur = This->pParserCtxt->input->cur;
    if(*cur != '<')
    {
        for(pos=0; pos<len; pos++)
            if(*(cur+pos) == '\n') This->lastLine--;
        cur--;
    }
    else
    {
        for(pos=0; pos<len; pos++)
            if(*(cur-pos-1) == '\n') This->lastLine--;
        cur = cur-len-1;
    }
    for(; *cur!='\n' && cur!=This->pParserCtxt->input->base; cur--)
        This->lastColumn++;

    if(This->saxreader->contentHandler)
    {
        Chars = bstr_from_xmlChar(ch);
        hr = ISAXContentHandler_characters(This->saxreader->contentHandler, Chars, len);
        SysFreeString(Chars);

        if(FAILED(hr))
        {
            xmlStopParser(This->pParserCtxt);
            This->ret = hr;
        }
    }
}

/*** ISAXLocator interface ***/
/*** IUnknown methods ***/
static HRESULT WINAPI isaxlocator_QueryInterface(ISAXLocator* iface, REFIID riid, void **ppvObject)
{
    saxlocator *This = impl_from_ISAXLocator( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    *ppvObject = NULL;

    if ( IsEqualGUID( riid, &IID_IUnknown ) ||
            IsEqualGUID( riid, &IID_ISAXLocator ))
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ISAXLocator_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI isaxlocator_AddRef(ISAXLocator* iface)
{
    saxlocator *This = impl_from_ISAXLocator( iface );
    TRACE("%p\n", This );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI isaxlocator_Release(
        ISAXLocator* iface)
{
    saxlocator *This = impl_from_ISAXLocator( iface );
    LONG ref;

    TRACE("%p\n", This );

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        ISAXXMLReader_Release((ISAXXMLReader*)&This->saxreader->lpSAXXMLReaderVtbl);
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

/*** ISAXLocator methods ***/
static HRESULT WINAPI isaxlocator_getColumnNumber(
        ISAXLocator* iface,
        int *pnColumn)
{
    saxlocator *This = impl_from_ISAXLocator( iface );

    *pnColumn = This->lastColumn;
    return S_OK;
}

static HRESULT WINAPI isaxlocator_getLineNumber(
        ISAXLocator* iface,
        int *pnLine)
{
    saxlocator *This = impl_from_ISAXLocator( iface );

    *pnLine = This->lastLine;
    return S_OK;
}

static HRESULT WINAPI isaxlocator_getPublicId(
        ISAXLocator* iface,
        const WCHAR ** ppwchPublicId)
{
    saxlocator *This = impl_from_ISAXLocator( iface );

    FIXME("(%p)->(%p) stub\n", This, ppwchPublicId);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxlocator_getSystemId(
        ISAXLocator* iface,
        const WCHAR ** ppwchSystemId)
{
    saxlocator *This = impl_from_ISAXLocator( iface );

    FIXME("(%p)->(%p) stub\n", This, ppwchSystemId);
    return E_NOTIMPL;
}

static const struct ISAXLocatorVtbl isaxlocator_vtbl =
{
    isaxlocator_QueryInterface,
    isaxlocator_AddRef,
    isaxlocator_Release,
    isaxlocator_getColumnNumber,
    isaxlocator_getLineNumber,
    isaxlocator_getPublicId,
    isaxlocator_getSystemId
};

static HRESULT SAXLocator_create(saxreader *reader, saxlocator **ppsaxlocator)
{
    saxlocator *locator;

    locator = HeapAlloc( GetProcessHeap(), 0, sizeof (*locator) );
    if( !locator )
        return E_OUTOFMEMORY;

    locator->lpSAXLocatorVtbl = &isaxlocator_vtbl;
    locator->ref = 1;

    locator->saxreader = reader;
    ISAXXMLReader_AddRef((ISAXXMLReader*)&reader->lpSAXXMLReaderVtbl);

    locator->pParserCtxt = NULL;
    locator->lastLine = 0;
    locator->lastColumn = 0;
    locator->ret = S_OK;

    *ppsaxlocator = locator;

    TRACE("returning %p\n", *ppsaxlocator);

    return S_OK;
}

/*** IVBSAXXMLReader interface ***/
/*** IUnknown methods ***/
static HRESULT WINAPI saxxmlreader_QueryInterface(IVBSAXXMLReader* iface, REFIID riid, void **ppvObject)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    *ppvObject = NULL;

    if ( IsEqualGUID( riid, &IID_IUnknown ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IVBSAXXMLReader ))
    {
        *ppvObject = iface;
    }
    else if( IsEqualGUID( riid, &IID_ISAXXMLReader ))
    {
        *ppvObject = (ISAXXMLReader*)&This->lpSAXXMLReaderVtbl;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IVBSAXXMLReader_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI saxxmlreader_AddRef(IVBSAXXMLReader* iface)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    TRACE("%p\n", This );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI saxxmlreader_Release(
    IVBSAXXMLReader* iface)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    LONG ref;

    TRACE("%p\n", This );

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        if(This->contentHandler)
            ISAXContentHandler_Release(This->contentHandler);

        if(This->errorHandler)
            ISAXErrorHandler_Release(This->errorHandler);

        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}
/*** IDispatch ***/
static HRESULT WINAPI saxxmlreader_GetTypeInfoCount( IVBSAXXMLReader *iface, UINT* pctinfo )
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI saxxmlreader_GetTypeInfo(
    IVBSAXXMLReader *iface,
    UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo )
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IVBSAXXMLReader_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI saxxmlreader_GetIDsOfNames(
    IVBSAXXMLReader *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IVBSAXXMLReader_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI saxxmlreader_Invoke(
    IVBSAXXMLReader *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS* pDispParams,
    VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo,
    UINT* puArgErr)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IVBSAXXMLReader_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

/*** IVBSAXXMLReader methods ***/
static HRESULT WINAPI saxxmlreader_getFeature(
    IVBSAXXMLReader* iface,
    const WCHAR *pFeature,
    VARIANT_BOOL *pValue)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%s %p) stub\n", This, debugstr_w(pFeature), pValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_putFeature(
    IVBSAXXMLReader* iface,
    const WCHAR *pFeature,
    VARIANT_BOOL vfValue)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%s %x) stub\n", This, debugstr_w(pFeature), vfValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_getProperty(
    IVBSAXXMLReader* iface,
    const WCHAR *pProp,
    VARIANT *pValue)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%s %p) stub\n", This, debugstr_w(pProp), pValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_putProperty(
    IVBSAXXMLReader* iface,
    const WCHAR *pProp,
    VARIANT value)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%s) stub\n", This, debugstr_w(pProp));
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_getEntityResolver(
    IVBSAXXMLReader* iface,
    IVBSAXEntityResolver **pEntityResolver)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, pEntityResolver);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_putEntityResolver(
    IVBSAXXMLReader* iface,
    IVBSAXEntityResolver *pEntityResolver)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, pEntityResolver);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_getContentHandler(
    IVBSAXXMLReader* iface,
    IVBSAXContentHandler **ppContentHandler)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, ppContentHandler);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_putContentHandler(
    IVBSAXXMLReader* iface,
    IVBSAXContentHandler *contentHandler)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, contentHandler);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_getDTDHandler(
    IVBSAXXMLReader* iface,
    IVBSAXDTDHandler **pDTDHandler)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, pDTDHandler);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_putDTDHandler(
    IVBSAXXMLReader* iface,
    IVBSAXDTDHandler *pDTDHandler)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, pDTDHandler);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_getErrorHandler(
    IVBSAXXMLReader* iface,
    IVBSAXErrorHandler **pErrorHandler)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, pErrorHandler);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_putErrorHandler(
    IVBSAXXMLReader* iface,
    IVBSAXErrorHandler *errorHandler)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, errorHandler);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_getBaseURL(
    IVBSAXXMLReader* iface,
    const WCHAR **pBaseUrl)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, pBaseUrl);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_putBaseURL(
    IVBSAXXMLReader* iface,
    const WCHAR *pBaseUrl)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%s) stub\n", This, debugstr_w(pBaseUrl));
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_getSecureBaseURL(
    IVBSAXXMLReader* iface,
    const WCHAR **pSecureBaseUrl)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, pSecureBaseUrl);
    return E_NOTIMPL;
}


static HRESULT WINAPI saxxmlreader_putSecureBaseURL(
    IVBSAXXMLReader* iface,
    const WCHAR *secureBaseUrl)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%s) stub\n", This, debugstr_w(secureBaseUrl));
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_parse(
    IVBSAXXMLReader* iface,
    VARIANT varInput)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p) stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_parseURL(
    IVBSAXXMLReader* iface,
    const WCHAR *url)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%s) stub\n", This, debugstr_w(url));
    return E_NOTIMPL;
}

static const struct IVBSAXXMLReaderVtbl saxreader_vtbl =
{
    saxxmlreader_QueryInterface,
    saxxmlreader_AddRef,
    saxxmlreader_Release,
    saxxmlreader_GetTypeInfoCount,
    saxxmlreader_GetTypeInfo,
    saxxmlreader_GetIDsOfNames,
    saxxmlreader_Invoke,
    saxxmlreader_getFeature,
    saxxmlreader_putFeature,
    saxxmlreader_getProperty,
    saxxmlreader_putProperty,
    saxxmlreader_getEntityResolver,
    saxxmlreader_putEntityResolver,
    saxxmlreader_getContentHandler,
    saxxmlreader_putContentHandler,
    saxxmlreader_getDTDHandler,
    saxxmlreader_putDTDHandler,
    saxxmlreader_getErrorHandler,
    saxxmlreader_putErrorHandler,
    saxxmlreader_getBaseURL,
    saxxmlreader_putBaseURL,
    saxxmlreader_getSecureBaseURL,
    saxxmlreader_putSecureBaseURL,
    saxxmlreader_parse,
    saxxmlreader_parseURL
};

/*** ISAXXMLReader interface ***/
/*** IUnknown methods ***/
static HRESULT WINAPI isaxxmlreader_QueryInterface(ISAXXMLReader* iface, REFIID riid, void **ppvObject)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return saxxmlreader_QueryInterface((IVBSAXXMLReader*)&This->lpVtbl, riid, ppvObject);
}

static ULONG WINAPI isaxxmlreader_AddRef(ISAXXMLReader* iface)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return saxxmlreader_AddRef((IVBSAXXMLReader*)&This->lpVtbl);
}

static ULONG WINAPI isaxxmlreader_Release(ISAXXMLReader* iface)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return saxxmlreader_Release((IVBSAXXMLReader*)&This->lpVtbl);
}

/*** ISAXXMLReader methods ***/
static HRESULT WINAPI isaxxmlreader_getFeature(
        ISAXXMLReader* iface,
        const WCHAR *pFeature,
        VARIANT_BOOL *pValue)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    FIXME("(%p)->(%s %p) stub\n", This, debugstr_w(pFeature), pValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_putFeature(
        ISAXXMLReader* iface,
        const WCHAR *pFeature,
        VARIANT_BOOL vfValue)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    FIXME("(%p)->(%s %x) stub\n", This, debugstr_w(pFeature), vfValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_getProperty(
        ISAXXMLReader* iface,
        const WCHAR *pProp,
        VARIANT *pValue)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    FIXME("(%p)->(%s %p) stub\n", This, debugstr_w(pProp), pValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_putProperty(
        ISAXXMLReader* iface,
        const WCHAR *pProp,
        VARIANT value)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    FIXME("(%p)->(%s) stub\n", This, debugstr_w(pProp));
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_getEntityResolver(
        ISAXXMLReader* iface,
        ISAXEntityResolver **ppEntityResolver)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, ppEntityResolver);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_putEntityResolver(
        ISAXXMLReader* iface,
        ISAXEntityResolver *pEntityResolver)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, pEntityResolver);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_getContentHandler(
        ISAXXMLReader* iface,
        ISAXContentHandler **pContentHandler)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    TRACE("(%p)->(%p)\n", This, pContentHandler);
    if(pContentHandler == NULL)
        return E_POINTER;
    if(This->contentHandler)
        ISAXContentHandler_AddRef(This->contentHandler);
    *pContentHandler = This->contentHandler;

    return S_OK;
}

static HRESULT WINAPI isaxxmlreader_putContentHandler(
        ISAXXMLReader* iface,
        ISAXContentHandler *contentHandler)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    TRACE("(%p)->(%p)\n", This, contentHandler);
    if(contentHandler)
        ISAXContentHandler_AddRef(contentHandler);
    if(This->contentHandler)
        ISAXContentHandler_Release(This->contentHandler);
    This->contentHandler = contentHandler;

    return S_OK;
}

static HRESULT WINAPI isaxxmlreader_getDTDHandler(
        ISAXXMLReader* iface,
        ISAXDTDHandler **pDTDHandler)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, pDTDHandler);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_putDTDHandler(
        ISAXXMLReader* iface,
        ISAXDTDHandler *pDTDHandler)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, pDTDHandler);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_getErrorHandler(
        ISAXXMLReader* iface,
        ISAXErrorHandler **pErrorHandler)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    TRACE("(%p)->(%p)\n", This, pErrorHandler);
    if(pErrorHandler == NULL)
        return E_POINTER;
    if(This->errorHandler)
        ISAXErrorHandler_AddRef(This->errorHandler);
    *pErrorHandler = This->errorHandler;

    return S_OK;
}

static HRESULT WINAPI isaxxmlreader_putErrorHandler(
        ISAXXMLReader* iface,
        ISAXErrorHandler *errorHandler)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    TRACE("(%p)->(%p)\n", This, errorHandler);
    if(errorHandler)
        ISAXErrorHandler_AddRef(errorHandler);
    if(This->errorHandler)
        ISAXErrorHandler_Release(This->errorHandler);
    This->errorHandler = errorHandler;

    return S_OK;
}

static HRESULT WINAPI isaxxmlreader_getBaseURL(
        ISAXXMLReader* iface,
        const WCHAR **pBaseUrl)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, pBaseUrl);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_putBaseURL(
        ISAXXMLReader* iface,
        const WCHAR *pBaseUrl)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    FIXME("(%p)->(%s) stub\n", This, debugstr_w(pBaseUrl));
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_getSecureBaseURL(
        ISAXXMLReader* iface,
        const WCHAR **pSecureBaseUrl)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, pSecureBaseUrl);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_putSecureBaseURL(
        ISAXXMLReader* iface,
        const WCHAR *secureBaseUrl)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    FIXME("(%p)->(%s) stub\n", This, debugstr_w(secureBaseUrl));
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_parse(
        ISAXXMLReader* iface,
        VARIANT varInput)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    saxlocator *locator;
    xmlChar *data = NULL;
    HRESULT hr;

    FIXME("(%p) semi-stub\n", This);

    hr = SAXLocator_create(This, &locator);
    if(FAILED(hr))
        return E_FAIL;

    hr = S_OK;
    switch(V_VT(&varInput))
    {
        case VT_BSTR:
            locator->pParserCtxt = xmlNewParserCtxt();
            if(!locator->pParserCtxt)
            {
                hr = E_FAIL;
                break;
            }
            data = xmlChar_from_wchar(V_BSTR(&varInput));
            xmlSetupParserForBuffer(locator->pParserCtxt, data, NULL);

            locator->pParserCtxt->sax = &locator->saxreader->sax;
            locator->pParserCtxt->userData = locator;

            if(xmlParseDocument(locator->pParserCtxt)) hr = E_FAIL;
            else hr = locator->ret;
            break;
        default:
            hr = E_NOTIMPL;
    }

    if(locator->pParserCtxt)
    {
        locator->pParserCtxt->sax = NULL;
        xmlFreeParserCtxt(locator->pParserCtxt);
        locator->pParserCtxt = NULL;
    }
    if(data) HeapFree(GetProcessHeap(), 0, data);
    ISAXLocator_Release((ISAXLocator*)&locator->lpSAXLocatorVtbl);
    return hr;
}

static HRESULT WINAPI isaxxmlreader_parseURL(
        ISAXXMLReader* iface,
        const WCHAR *url)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    FIXME("(%p)->(%s) stub\n", This, debugstr_w(url));
    return E_NOTIMPL;
}

static const struct ISAXXMLReaderVtbl isaxreader_vtbl =
{
    isaxxmlreader_QueryInterface,
    isaxxmlreader_AddRef,
    isaxxmlreader_Release,
    isaxxmlreader_getFeature,
    isaxxmlreader_putFeature,
    isaxxmlreader_getProperty,
    isaxxmlreader_putProperty,
    isaxxmlreader_getEntityResolver,
    isaxxmlreader_putEntityResolver,
    isaxxmlreader_getContentHandler,
    isaxxmlreader_putContentHandler,
    isaxxmlreader_getDTDHandler,
    isaxxmlreader_putDTDHandler,
    isaxxmlreader_getErrorHandler,
    isaxxmlreader_putErrorHandler,
    isaxxmlreader_getBaseURL,
    isaxxmlreader_putBaseURL,
    isaxxmlreader_getSecureBaseURL,
    isaxxmlreader_putSecureBaseURL,
    isaxxmlreader_parse,
    isaxxmlreader_parseURL
};

HRESULT SAXXMLReader_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    saxreader *reader;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    reader = HeapAlloc( GetProcessHeap(), 0, sizeof (*reader) );
    if( !reader )
        return E_OUTOFMEMORY;

    reader->lpVtbl = &saxreader_vtbl;
    reader->lpSAXXMLReaderVtbl = &isaxreader_vtbl;
    reader->ref = 1;
    reader->contentHandler = NULL;
    reader->errorHandler = NULL;

    memset(&reader->sax, 0, sizeof(xmlSAXHandler));
    reader->sax.initialized = XML_SAX2_MAGIC;
    reader->sax.startDocument = libxmlStartDocument;
    reader->sax.endDocument = libxmlEndDocument;
    reader->sax.startElementNs = libxmlStartElementNS;
    reader->sax.endElementNs = libxmlEndElementNS;
    reader->sax.characters = libxmlCharacters;

    *ppObj = &reader->lpVtbl;

    TRACE("returning iface %p\n", *ppObj);

    return S_OK;
}

#else

HRESULT SAXXMLReader_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    MESSAGE("This program tried to use a SAX XML Reader object, but\n"
            "libxml2 support was not present at compile time.\n");
    return E_NOTIMPL;
}

#endif
