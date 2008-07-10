/*
 * XML test
 *
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
#define CONST_VTABLE

#include <stdio.h>
#include "windows.h"
#include "ole2.h"
#include "msxml2.h"
#include "ocidl.h"

#include "wine/test.h"

static const WCHAR szSimpleXML[] = {
'<','?','x','m','l',' ','v','e','r','s','i','o','n','=','\"','1','.','0','\"',' ','?','>','\n',
'<','B','a','n','k','A','c','c','o','u','n','t','>','\n',
' ',' ',' ','<','N','u','m','b','e','r','>','1','2','3','4','<','/','N','u','m','b','e','r','>','\n',
' ',' ',' ','<','N','a','m','e','>','C','a','p','t','a','i','n',' ','A','h','a','b','<','/','N','a','m','e','>','\n',
'<','/','B','a','n','k','A','c','c','o','u','n','t','>','\n','\0'
};

typedef struct _contenthandler
{
    const struct ISAXContentHandlerVtbl *lpContentHandlerVtbl;
} contenthandler;

static inline contenthandler *impl_from_ISAXContentHandler(ISAXContentHandler *iface)
{
    return (contenthandler *)((char*)iface - FIELD_OFFSET(contenthandler, lpContentHandlerVtbl));
}

static HRESULT WINAPI contentHandler_QueryInterface(
        ISAXContentHandler* iface,
        REFIID riid,
        void **ppvObject)
{
    *ppvObject = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ISAXContentHandler))
    {
        *ppvObject = iface;
    }
    else
    {
        return E_NOINTERFACE;
    }

    return S_OK;
}

static ULONG WINAPI contentHandler_AddRef(
        ISAXContentHandler* iface)
{
    return 2;
}

static ULONG WINAPI contentHandler_Release(
        ISAXContentHandler* iface)
{
    return 1;
}

static HRESULT WINAPI contentHandler_putDocumentLocator(
        ISAXContentHandler* iface,
        ISAXLocator *pLocator)
{
    return S_OK;
}

static HRESULT WINAPI contentHandler_startDocument(
        ISAXContentHandler* iface)
{
    return S_OK;
}

static HRESULT WINAPI contentHandler_endDocument(
        ISAXContentHandler* iface)
{
    return S_OK;
}

static HRESULT WINAPI contentHandler_startPrefixMapping(
        ISAXContentHandler* iface,
        const WCHAR *pPrefix,
        int nPrefix,
        const WCHAR *pUri,
        int nUri)
{
    return S_OK;
}

static HRESULT WINAPI contentHandler_endPrefixMapping(
        ISAXContentHandler* iface,
        const WCHAR *pPrefix,
        int nPrefix)
{
    return S_OK;
}

static HRESULT WINAPI contentHandler_startElement(
        ISAXContentHandler* iface,
        const WCHAR *pNamespaceUri,
        int nNamespaceUri,
        const WCHAR *pLocalName,
        int nLocalName,
        const WCHAR *pQName,
        int nQName,
        ISAXAttributes *pAttr)
{
    return S_OK;
}

static HRESULT WINAPI contentHandler_endElement(
        ISAXContentHandler* iface,
        const WCHAR *pNamespaceUri,
        int nNamespaceUri,
        const WCHAR *pLocalName,
        int nLocalName,
        const WCHAR *pQName,
        int nQName)
{
    return S_OK;
}

static HRESULT WINAPI contentHandler_characters(
        ISAXContentHandler* iface,
        const WCHAR *pChars,
        int nChars)
{
    return S_OK;
}

static HRESULT WINAPI contentHandler_ignorableWhitespace(
        ISAXContentHandler* iface,
        const WCHAR *pChars,
        int nChars)
{
    return S_OK;
}

static HRESULT WINAPI contentHandler_processingInstruction(
        ISAXContentHandler* iface,
        const WCHAR *pTarget,
        int nTarget,
        const WCHAR *pData,
        int nData)
{
    return S_OK;
}

static HRESULT WINAPI contentHandler_skippedEntity(
        ISAXContentHandler* iface,
        const WCHAR *pName,
        int nName)
{
    return S_OK;
}


static const ISAXContentHandlerVtbl contentHandlerVtbl =
{
    contentHandler_QueryInterface,
    contentHandler_AddRef,
    contentHandler_Release,
    contentHandler_putDocumentLocator,
    contentHandler_startDocument,
    contentHandler_endDocument,
    contentHandler_startPrefixMapping,
    contentHandler_endPrefixMapping,
    contentHandler_startElement,
    contentHandler_endElement,
    contentHandler_characters,
    contentHandler_ignorableWhitespace,
    contentHandler_processingInstruction,
    contentHandler_skippedEntity
};

static ISAXContentHandler contentHandler = { &contentHandlerVtbl };


static void test_saxreader(void)
{
    HRESULT hr;
    ISAXXMLReader *reader = NULL;
    VARIANT var;
    ISAXContentHandler *lpContentHandler;

    hr = CoCreateInstance(&CLSID_SAXXMLReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_ISAXXMLReader, (LPVOID*)&reader);

    if(FAILED(hr))
    {
        skip("Failed to create SAXXMLReader instance\n");
        return;
    }

    hr = ISAXXMLReader_getContentHandler(reader, &lpContentHandler);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(lpContentHandler == NULL, "Expected %p, got %p\n", NULL, lpContentHandler);

    hr = ISAXXMLReader_putContentHandler(reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = ISAXXMLReader_putContentHandler(reader, &contentHandler);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = ISAXXMLReader_getContentHandler(reader, &lpContentHandler);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(lpContentHandler == &contentHandler, "Expected %p, got %p\n", &contentHandler, lpContentHandler);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(szSimpleXML);

    hr = ISAXXMLReader_parse(reader, var);
    todo_wine {
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    }

    ISAXXMLReader_Release(reader);
}

START_TEST(saxreader)
{
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "failed to init com\n");

    test_saxreader();

    CoUninitialize();
}
