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
#include <libxml/parserInternals.h>

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
    WCHAR *publicId;
    WCHAR *systemId;
    xmlChar *lastCur;
    int line;
    int column;
} saxlocator;

typedef struct _saxattributes
{
    const struct ISAXAttributesVtbl *lpSAXAttributesVtbl;
    LONG ref;
    int nb_attributes;
    BSTR *szLocalname;
    BSTR *szPrefix;
    BSTR *szURI;
    BSTR *szValue;
    BSTR *szQName;
} saxattributes;

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

static inline saxattributes *impl_from_ISAXAttributes( ISAXAttributes *iface )
{
    return (saxattributes *)((char*)iface - FIELD_OFFSET(saxattributes, lpSAXAttributesVtbl));
}


BSTR bstr_from_xmlCharN(const xmlChar *buf, int len)
{
    DWORD dLen;
    LPWSTR str;
    BSTR bstr;

    if (!buf)
        return NULL;

    dLen = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buf, len, NULL, 0);
    if(len != -1) dLen++;
    str = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, dLen * sizeof (WCHAR));
    if (!str)
        return NULL;
    MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buf, len, str, dLen);
    if(len != -1) str[dLen-1] = '\0';
    bstr = SysAllocString(str);
    HeapFree(GetProcessHeap(), 0, str);

    return bstr;
}

static void format_error_message_from_id(saxlocator *This, HRESULT hr)
{
    xmlStopParser(This->pParserCtxt);
    This->ret = hr;

    if(This->saxreader->errorHandler)
    {
        WCHAR msg[1024];
        if(!FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL, hr, 0, msg, sizeof(msg), NULL))
        {
            FIXME("MSXML errors not yet supported.\n");
            msg[0] = '\0';
        }

        ISAXErrorHandler_fatalError(This->saxreader->errorHandler,
                (ISAXLocator*)&This->lpSAXLocatorVtbl, msg, hr);
    }
}

static void update_position(saxlocator *This, xmlChar *end)
{
    if(This->lastCur == NULL)
    {
        This->lastCur = (xmlChar*)This->pParserCtxt->input->base;
        This->line = 1;
        This->column = 1;
    }
    else if(This->lastCur < This->pParserCtxt->input->base)
    {
        This->lastCur = (xmlChar*)This->pParserCtxt->input->base;
        This->line = 1;
        This->column = 1;
    }

    if(!end) end = (xmlChar*)This->pParserCtxt->input->cur;

    while(This->lastCur < end)
    {
        if(*(This->lastCur) == '\n')
        {
            This->line++;
            This->column = 1;
        }
        else if(*(This->lastCur) == '\r' && (This->lastCur==This->pParserCtxt->input->end || *(This->lastCur+1)!='\n'))
        {
            This->line++;
            This->column = 1;
        }
        else This->column++;

        This->lastCur++;
    }
}

/*** ISAXAttributes interface ***/
/*** IUnknown methods ***/
static HRESULT WINAPI isaxattributes_QueryInterface(
        ISAXAttributes* iface,
        REFIID riid,
        void **ppvObject)
{
    saxattributes *This = impl_from_ISAXAttributes(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_ISAXAttributes))
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ISAXAttributes_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI isaxattributes_AddRef(ISAXAttributes* iface)
{
    saxattributes *This = impl_from_ISAXAttributes(iface);
    TRACE("%p\n", This);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI isaxattributes_Release(ISAXAttributes* iface)
{
    saxattributes *This = impl_from_ISAXAttributes(iface);
    LONG ref;

    TRACE("%p\n", This);

    ref = InterlockedDecrement(&This->ref);
    if (ref==0)
    {
        int index;
        for(index=0; index<This->nb_attributes; index++)
        {
            SysFreeString(This->szLocalname[index]);
            SysFreeString(This->szPrefix[index]);
            SysFreeString(This->szURI[index]);
            SysFreeString(This->szValue[index]);
        }

        HeapFree(GetProcessHeap(), 0, This->szLocalname);
        HeapFree(GetProcessHeap(), 0, This->szPrefix);
        HeapFree(GetProcessHeap(), 0, This->szURI);
        HeapFree(GetProcessHeap(), 0, This->szValue);

        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** ISAXAttributes methods ***/
static HRESULT WINAPI isaxattributes_getLength(
        ISAXAttributes* iface,
        int *length)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );

    *length = This->nb_attributes;
    TRACE("Length set to %d\n", *length);
    return S_OK;
}

static HRESULT WINAPI isaxattributes_getURI(
        ISAXAttributes* iface,
        int nIndex,
        const WCHAR **pUrl,
        int *pUriSize)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );

    FIXME("(%p)->(%d) stub\n", This, nIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getLocalName(
        ISAXAttributes* iface,
        int nIndex,
        const WCHAR **pLocalName,
        int *pLocalNameLength)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );
    TRACE("(%p)->(%d)\n", This, nIndex);

    if(nIndex >= This->nb_attributes) return E_INVALIDARG;

    *pLocalNameLength = SysStringLen(This->szLocalname[nIndex]);
    *pLocalName = This->szLocalname[nIndex];

    return S_OK;
}

static HRESULT WINAPI isaxattributes_getQName(
        ISAXAttributes* iface,
        int nIndex,
        const WCHAR **pQName,
        int *pQNameLength)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );
    TRACE("(%p)->(%d)\n", This, nIndex);

    if(nIndex >= This->nb_attributes) return E_INVALIDARG;

    *pQNameLength = SysStringLen(This->szQName[nIndex]);
    *pQName = This->szQName[nIndex];

    return S_OK;
}

static HRESULT WINAPI isaxattributes_getName(
        ISAXAttributes* iface,
        int nIndex,
        const WCHAR **pUri,
        int *pUriLength,
        const WCHAR **pLocalName,
        int *pLocalNameSize,
        const WCHAR **pQName,
        int *pQNameLength)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );

    FIXME("(%p)->(%d) stub\n", This, nIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getIndexFromName(
        ISAXAttributes* iface,
        const WCHAR *pUri,
        int cUriLength,
        const WCHAR *pLocalName,
        int cocalNameLength,
        int *index)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );

    FIXME("(%p)->(%s, %d, %s, %d) stub\n", This, debugstr_w(pUri), cUriLength,
            debugstr_w(pLocalName), cocalNameLength);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getIndexFromQName(
        ISAXAttributes* iface,
        const WCHAR *pQName,
        int nQNameLength,
        int *index)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );

    FIXME("(%p)->(%s, %d) stub\n", This, debugstr_w(pQName), nQNameLength);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getType(
        ISAXAttributes* iface,
        int nIndex,
        const WCHAR **pType,
        int *pTypeLength)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );

    FIXME("(%p)->(%d) stub\n", This, nIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getTypeFromName(
        ISAXAttributes* iface,
        const WCHAR *pUri,
        int nUri,
        const WCHAR *pLocalName,
        int nLocalName,
        const WCHAR **pType,
        int *nType)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );

    FIXME("(%p)->(%s, %d, %s, %d) stub\n", This, debugstr_w(pUri), nUri,
            debugstr_w(pLocalName), nLocalName);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getTypeFromQName(
        ISAXAttributes* iface,
        const WCHAR *pQName,
        int nQName,
        const WCHAR **pType,
        int *nType)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );

    FIXME("(%p)->(%s, %d) stub\n", This, debugstr_w(pQName), nQName);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getValue(
        ISAXAttributes* iface,
        int nIndex,
        const WCHAR **pValue,
        int *nValue)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );
    TRACE("(%p)->(%d)\n", This, nIndex);

    if(nIndex >= This->nb_attributes) return E_INVALIDARG;

    *nValue = SysStringLen(This->szValue[nIndex]);
    *pValue = This->szValue[nIndex];

    return S_OK;
}

static HRESULT WINAPI isaxattributes_getValueFromName(
        ISAXAttributes* iface,
        const WCHAR *pUri,
        int nUri,
        const WCHAR *pLocalName,
        int nLocalName,
        const WCHAR **pValue,
        int *nValue)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );

    FIXME("(%p)->(%s, %d, %s, %d) stub\n", This, debugstr_w(pUri), nUri,
            debugstr_w(pLocalName), nLocalName);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getValueFromQName(
        ISAXAttributes* iface,
        const WCHAR *pQName,
        int nQName,
        const WCHAR **pValue,
        int *nValue)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );

    FIXME("(%p)->(%s, %d) stub\n", This, debugstr_w(pQName), nQName);
    return E_NOTIMPL;
}

static const struct ISAXAttributesVtbl isaxattributes_vtbl =
{
    isaxattributes_QueryInterface,
    isaxattributes_AddRef,
    isaxattributes_Release,
    isaxattributes_getLength,
    isaxattributes_getURI,
    isaxattributes_getLocalName,
    isaxattributes_getQName,
    isaxattributes_getName,
    isaxattributes_getIndexFromName,
    isaxattributes_getIndexFromQName,
    isaxattributes_getType,
    isaxattributes_getTypeFromName,
    isaxattributes_getTypeFromQName,
    isaxattributes_getValue,
    isaxattributes_getValueFromName,
    isaxattributes_getValueFromQName
};

static HRESULT SAXAttributes_create(saxattributes **attr,
        int nb_attributes, const xmlChar **xmlAttributes)
{
    saxattributes *attributes;
    int index;

    attributes = HeapAlloc(GetProcessHeap(), 0, sizeof(*attributes));
    if(!attributes)
        return E_OUTOFMEMORY;

    attributes->lpSAXAttributesVtbl = &isaxattributes_vtbl;
    attributes->ref = 1;

    attributes->nb_attributes = nb_attributes;

    attributes->szLocalname =
        HeapAlloc(GetProcessHeap(), 0, sizeof(BSTR)*nb_attributes);
    attributes->szPrefix =
        HeapAlloc(GetProcessHeap(), 0, sizeof(BSTR)*nb_attributes);
    attributes->szURI =
        HeapAlloc(GetProcessHeap(), 0, sizeof(BSTR)*nb_attributes);
    attributes->szValue =
        HeapAlloc(GetProcessHeap(), 0, sizeof(BSTR)*nb_attributes);
    attributes->szQName =
        HeapAlloc(GetProcessHeap(), 0, sizeof(BSTR)*nb_attributes);

    if(!attributes->szLocalname || !attributes->szPrefix
            || !attributes->szURI || !attributes->szValue
            || !attributes->szQName)
    {
        if(attributes->szLocalname)
            HeapFree(GetProcessHeap(), 0, attributes->szLocalname);
        if(attributes->szPrefix)
            HeapFree(GetProcessHeap(), 0, attributes->szPrefix);
        if(attributes->szURI)
            HeapFree(GetProcessHeap(), 0, attributes->szURI);
        if(attributes->szValue)
            HeapFree(GetProcessHeap(), 0, attributes->szValue);
        if(attributes->szQName)
            HeapFree(GetProcessHeap(), 0, attributes->szQName);
        return E_FAIL;
    }

    for(index=0; index<nb_attributes; index++)
    {
        int len1, len2;

        attributes->szLocalname[index] =
            bstr_from_xmlChar(xmlAttributes[index*5]);
        attributes->szPrefix[index] =
            bstr_from_xmlChar(xmlAttributes[index*5+1]);
        attributes->szURI[index] =
            bstr_from_xmlChar(xmlAttributes[index*5+2]);
        attributes->szValue[index] =
            bstr_from_xmlCharN(xmlAttributes[index*5+3],
                    xmlAttributes[index*5+4]-xmlAttributes[index*5+3]);

        len1 = SysStringLen(attributes->szPrefix[index]);
        len2 = SysStringLen(attributes->szLocalname[index]);
        attributes->szQName[index] = SysAllocStringLen(NULL, len1+len2);
        memcpy(attributes->szQName[index], attributes->szPrefix[index],
                len1*sizeof(WCHAR));
        memcpy(attributes->szQName[index]+len1,
                attributes->szLocalname[index], len2*sizeof(WCHAR));
        attributes->szQName[index][len1+len2] = '\0';
    }

    *attr = attributes;

    TRACE("returning %p\n", *attr);

    return S_OK;
}

/*** LibXML callbacks ***/
static void libxmlStartDocument(void *ctx)
{
    saxlocator *This = ctx;
    HRESULT hr;

    if(This->saxreader->contentHandler)
    {
        hr = ISAXContentHandler_startDocument(This->saxreader->contentHandler);
        if(hr != S_OK)
            format_error_message_from_id(This, hr);
    }

    update_position(This, NULL);
}

static void libxmlEndDocument(void *ctx)
{
    saxlocator *This = ctx;
    HRESULT hr;

    This->column = 0;
    This->line = 0;

    if(This->ret != S_OK) return;

    if(This->saxreader->contentHandler)
    {
        hr = ISAXContentHandler_endDocument(This->saxreader->contentHandler);
        if(hr != S_OK)
            format_error_message_from_id(This, hr);
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
    saxattributes *attr;

    update_position(This, (xmlChar*)This->pParserCtxt->input->cur+1);

    if(This->saxreader->contentHandler)
    {
        NamespaceUri = bstr_from_xmlChar(URI);
        LocalName = bstr_from_xmlChar(localname);
        QName = bstr_from_xmlChar(localname);

        hr = SAXAttributes_create(&attr, nb_attributes, attributes);
        if(hr == S_OK)
        {
            hr = ISAXContentHandler_startElement(
                    This->saxreader->contentHandler,
                    NamespaceUri, SysStringLen(NamespaceUri),
                    LocalName, SysStringLen(LocalName),
                    QName, SysStringLen(QName),
                    (ISAXAttributes*)&attr->lpSAXAttributesVtbl);

            ISAXAttributes_Release((ISAXAttributes*)&attr->lpSAXAttributesVtbl);
        }

        SysFreeString(NamespaceUri);
        SysFreeString(LocalName);
        SysFreeString(QName);

        if(hr != S_OK)
            format_error_message_from_id(This, hr);
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
    xmlChar *end;

    end = This->lastCur;
    while(*end != '<' && *(end+1) != '/') end++;
    update_position(This, end+2);

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

        if(hr != S_OK)
            format_error_message_from_id(This, hr);
    }
}

static void libxmlCharacters(
        void *ctx,
        const xmlChar *ch,
        int len)
{
    BSTR Chars;
    saxlocator *This = ctx;
    HRESULT hr;
    xmlChar *end;
    xmlChar *lastCurCopy;
    xmlChar *chEnd;
    int columnCopy;
    int lineCopy;

    if(*(This->lastCur-1) != '>' && *(This->lastCur-1) != '/') return;

    if(*(This->lastCur-1) != '>')
    {
        end = (xmlChar*)This->pParserCtxt->input->cur-len;
        while(*(end-1) != '>') end--;
        update_position(This, end);
    }

    chEnd = This->lastCur+len;
    while(*chEnd != '<') chEnd++;

    Chars = bstr_from_xmlChar(ch);

    lastCurCopy = This->lastCur;
    columnCopy = This->column;
    lineCopy = This->line;
    end = This->lastCur;

    if(This->saxreader->contentHandler)
    {
        while(This->lastCur < chEnd)
        {
            end = This->lastCur;
            while(end < chEnd-1)
            {
                if(*end == '\r') break;
                end++;
            }

            Chars = bstr_from_xmlChar(This->lastCur);

            if(*end == '\r' && *(end+1) == '\n')
            {
                memmove((WCHAR*)Chars+(end-This->lastCur),
                        (WCHAR*)Chars+(end-This->lastCur)+1,
                        (SysStringLen(Chars)-(end-This->lastCur))*sizeof(WCHAR));
                SysReAllocStringLen(&Chars, Chars, SysStringLen(Chars)-1);
            }
            else if(*end == '\r') Chars[end-This->lastCur] = '\n';

            hr = ISAXContentHandler_characters(This->saxreader->contentHandler, Chars, end-This->lastCur+1);
            SysFreeString(Chars);
            if(hr != S_OK)
            {
                format_error_message_from_id(This, hr);
                return;
            }

            if(*(end+1) == '\n') end++;
            if(end < chEnd) end++;

            This->column += end-This->lastCur;
            This->lastCur = end;
        }

        This->lastCur = lastCurCopy;
        This->column = columnCopy;
        This->line = lineCopy;
        update_position(This, chEnd);
    }
}

static void libxmlSetDocumentLocator(
        void *ctx,
        xmlSAXLocatorPtr loc)
{
    saxlocator *This = ctx;
    HRESULT hr;

    hr = ISAXContentHandler_putDocumentLocator(This->saxreader->contentHandler,
            (ISAXLocator*)&This->lpSAXLocatorVtbl);

    if(FAILED(hr))
        format_error_message_from_id(This, hr);
}

void libxmlFatalError(void *ctx, const char *msg, ...)
{
    saxlocator *This = ctx;
    char message[1024];
    WCHAR *wszError;
    DWORD len;
    va_list args;

    if(!This->saxreader->errorHandler)
    {
        xmlStopParser(This->pParserCtxt);
        This->ret = E_FAIL;
        return;
    }

    FIXME("Error handling is not compatible.\n");

    va_start(args, msg);
    vsprintf(message, msg, args);
    va_end(args);

    len = MultiByteToWideChar(CP_ACP, 0, message, -1, NULL, 0);
    wszError = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*len);
    MultiByteToWideChar(CP_ACP, 0, message, -1, (LPWSTR)wszError, len);

    ISAXErrorHandler_fatalError(This->saxreader->errorHandler,
            (ISAXLocator*)&This->lpSAXLocatorVtbl, wszError, E_FAIL);

    HeapFree(GetProcessHeap(), 0, wszError);

    xmlStopParser(This->pParserCtxt);
    This->ret = E_FAIL;
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
        if(This->publicId)
            SysFreeString(This->publicId);
        if(This->systemId)
            SysFreeString(This->systemId);

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

    *pnColumn = This->column;
    return S_OK;
}

static HRESULT WINAPI isaxlocator_getLineNumber(
        ISAXLocator* iface,
        int *pnLine)
{
    saxlocator *This = impl_from_ISAXLocator( iface );

    *pnLine = This->line;
    return S_OK;
}

static HRESULT WINAPI isaxlocator_getPublicId(
        ISAXLocator* iface,
        const WCHAR ** ppwchPublicId)
{
    BSTR publicId;
    saxlocator *This = impl_from_ISAXLocator( iface );

    if(This->publicId) SysFreeString(This->publicId);

    publicId = bstr_from_xmlChar(xmlSAX2GetPublicId(This->pParserCtxt));
    if(SysStringLen(publicId))
        This->publicId = (WCHAR*)&publicId;
    else
    {
        SysFreeString(publicId);
        This->publicId = NULL;
    }

    *ppwchPublicId = This->publicId;
    return S_OK;
}

static HRESULT WINAPI isaxlocator_getSystemId(
        ISAXLocator* iface,
        const WCHAR ** ppwchSystemId)
{
    BSTR systemId;
    saxlocator *This = impl_from_ISAXLocator( iface );

    if(This->systemId) SysFreeString(This->systemId);

    systemId = bstr_from_xmlChar(xmlSAX2GetSystemId(This->pParserCtxt));
    if(SysStringLen(systemId))
        This->systemId = (WCHAR*)&systemId;
    else
    {
        SysFreeString(systemId);
        This->systemId = NULL;
    }

    *ppwchSystemId = This->systemId;
    return S_OK;
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
    locator->publicId = NULL;
    locator->systemId = NULL;
    locator->lastCur = NULL;
    locator->line = 0;
    locator->column = 0;
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

static HRESULT parse_buffer(saxreader *This, const char *buffer, int size)
{
    saxlocator *locator;
    HRESULT hr;

    hr = SAXLocator_create(This, &locator);
    if(FAILED(hr))
        return E_FAIL;

    locator->pParserCtxt = xmlCreateMemoryParserCtxt(buffer, size);
    if(!locator->pParserCtxt)
    {
        ISAXLocator_Release((ISAXLocator*)&locator->lpSAXLocatorVtbl);
        return E_FAIL;
    }

    locator->pParserCtxt->sax = &locator->saxreader->sax;
    locator->pParserCtxt->userData = locator;

    if(xmlParseDocument(locator->pParserCtxt)) hr = E_FAIL;
    else hr = locator->ret;

    if(locator->pParserCtxt)
    {
        locator->pParserCtxt->sax = NULL;
        xmlFreeParserCtxt(locator->pParserCtxt);
        locator->pParserCtxt = NULL;
    }

    ISAXLocator_Release((ISAXLocator*)&locator->lpSAXLocatorVtbl);
    return S_OK;
}

static HRESULT WINAPI isaxxmlreader_parse(
        ISAXXMLReader* iface,
        VARIANT varInput)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    HRESULT hr;

    TRACE("(%p)\n", This);

    hr = S_OK;
    switch(V_VT(&varInput))
    {
        case VT_BSTR:
            hr = parse_buffer(This, (const char*)V_BSTR(&varInput),
                    SysStringByteLen(V_BSTR(&varInput)));
            break;
        case VT_ARRAY|VT_UI1: {
            void *pSAData;
            LONG lBound, uBound;
            ULONG dataRead;

            hr = SafeArrayGetLBound(V_ARRAY(&varInput), 1, &lBound);
            if(hr != S_OK) break;
            hr = SafeArrayGetUBound(V_ARRAY(&varInput), 1, &uBound);
            if(hr != S_OK) break;
            dataRead = (uBound-lBound)*SafeArrayGetElemsize(V_ARRAY(&varInput));
            hr = SafeArrayAccessData(V_ARRAY(&varInput), (void**)&pSAData);
            if(hr != S_OK) break;
            hr = parse_buffer(This, pSAData, dataRead);
            SafeArrayUnaccessData(V_ARRAY(&varInput));
            break;
        }
        case VT_UNKNOWN:
        case VT_DISPATCH: {
            IPersistStream *persistStream;
            IStream *stream = NULL;
            IXMLDOMDocument *xmlDoc;

            if(IUnknown_QueryInterface(V_UNKNOWN(&varInput),
                        &IID_IPersistStream, (void**)&persistStream) == S_OK)
            {
                hr = IPersistStream_Save(persistStream, stream, TRUE);
                IPersistStream_Release(persistStream);
                if(hr != S_OK) break;
            }
            if(stream || IUnknown_QueryInterface(V_UNKNOWN(&varInput),
                        &IID_IStream, (void**)&stream) == S_OK)
            {
                STATSTG dataInfo;
                ULONG dataRead;
                char *data;

                while(1)
                {
                    hr = IStream_Stat(stream, &dataInfo, STATFLAG_NONAME);
                    if(hr == E_PENDING) continue;
                    break;
                }
                data = HeapAlloc(GetProcessHeap(), 0,
                        dataInfo.cbSize.QuadPart);
                while(1)
                {
                    hr = IStream_Read(stream, data,
                            dataInfo.cbSize.QuadPart, &dataRead);
                    if(hr == E_PENDING) continue;
                    break;
                }
                hr = parse_buffer(This, data, dataInfo.cbSize.QuadPart);
                HeapFree(GetProcessHeap(), 0, data);
                IStream_Release(stream);
                break;
            }
            if(IUnknown_QueryInterface(V_UNKNOWN(&varInput),
                                       &IID_IXMLDOMDocument, (void**)&xmlDoc) == S_OK)
            {
                BSTR bstrData;

                IXMLDOMDocument_get_xml(xmlDoc, &bstrData);
                hr = parse_buffer(This, (const char*)bstrData, SysStringByteLen(bstrData));
                IXMLDOMDocument_Release(xmlDoc);
                hr = E_NOTIMPL;
                break;
            }
        }
        default:
            WARN("vt %d not implemented\n", V_VT(&varInput));
            hr = E_INVALIDARG;
    }

    return hr;
}

static HRESULT saxreader_onDataAvailable(void *obj, char *ptr, DWORD len)
{
    saxreader *This = obj;
    HRESULT hr;

    hr = parse_buffer(This, ptr, len);

    return hr;
}

static HRESULT WINAPI isaxxmlreader_parseURL(
        ISAXXMLReader* iface,
        const WCHAR *url)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    bsc_t *bsc;
    HRESULT hr;

    TRACE("(%p)->(%s) stub\n", This, debugstr_w(url));

    hr = bind_url(url, saxreader_onDataAvailable, This, &bsc);
    if(FAILED(hr))
        return hr;

    detach_bsc(bsc);

    return S_OK;
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
    reader->sax.setDocumentLocator = libxmlSetDocumentLocator;
    reader->sax.error = libxmlFatalError;
    reader->sax.fatalError = libxmlFatalError;

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
