/*
 * XML test
 *
 * Copyright 2008 Piotr Caban
 * Copyright 2011 Thomas Mullaly
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
#include <assert.h>

#include "windows.h"
#include "ole2.h"
#include "msxml2.h"
#include "ocidl.h"

#include "wine/test.h"

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc = IUnknown_AddRef(obj);
    IUnknown_Release(obj);
    ok_(__FILE__,line)(rc-1 == ref, "expected refcount %d, got %d\n", ref, rc-1);
}

static BSTR alloc_str_from_narrow(const char *str)
{
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    BSTR ret = SysAllocStringLen(NULL, len - 1);  /* NUL character added automatically */
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

static BSTR alloced_bstrs[256];
static int alloced_bstrs_count;

static BSTR _bstr_(const char *str)
{
    assert(alloced_bstrs_count < sizeof(alloced_bstrs)/sizeof(alloced_bstrs[0]));
    alloced_bstrs[alloced_bstrs_count] = alloc_str_from_narrow(str);
    return alloced_bstrs[alloced_bstrs_count++];
}

static void free_bstrs(void)
{
    int i;
    for (i = 0; i < alloced_bstrs_count; i++)
        SysFreeString(alloced_bstrs[i]);
    alloced_bstrs_count = 0;
}

typedef enum _CH {
    CH_ENDTEST,
    CH_PUTDOCUMENTLOCATOR,
    CH_STARTDOCUMENT,
    CH_ENDDOCUMENT,
    CH_STARTPREFIXMAPPING,
    CH_ENDPREFIXMAPPING,
    CH_STARTELEMENT,
    CH_ENDELEMENT,
    CH_CHARACTERS,
    CH_IGNORABLEWHITESPACE,
    CH_PROCESSINGINSTRUCTION,
    CH_SKIPPEDENTITY
} CH;

static const WCHAR szSimpleXML[] = {
'<','?','x','m','l',' ','v','e','r','s','i','o','n','=','\"','1','.','0','\"',' ','?','>','\n',
'<','B','a','n','k','A','c','c','o','u','n','t','>','\n',
' ',' ',' ','<','N','u','m','b','e','r','>','1','2','3','4','<','/','N','u','m','b','e','r','>','\n',
' ',' ',' ','<','N','a','m','e','>','C','a','p','t','a','i','n',' ','A','h','a','b','<','/','N','a','m','e','>','\n',
'<','/','B','a','n','k','A','c','c','o','u','n','t','>','\n','\0'
};

static const WCHAR szCarriageRetTest[] = {
'<','?','x','m','l',' ','v','e','r','s','i','o','n','=','"','1','.','0','"','?','>','\r','\n',
'<','B','a','n','k','A','c','c','o','u','n','t','>','\r','\n',
'\t','<','N','u','m','b','e','r','>','1','2','3','4','<','/','N','u','m','b','e','r','>','\r','\n',
'\t','<','N','a','m','e','>','C','a','p','t','a','i','n',' ','A','h','a','b','<','/','N','a','m','e','>','\r','\n',
'<','/','B','a','n','k','A','c','c','o','u','n','t','>','\0'
};

static const WCHAR szUtf16XML[] = {
'<','?','x','m','l',' ','v','e','r','s','i','o','n','=','"','1','.','0','"',' ',
'e','n','c','o','d','i','n','g','=','"','U','T','F','-','1','6','"',' ',
's','t','a','n','d','a','l','o','n','e','=','"','n','o','"','?','>','\r','\n'
};

static const CHAR szUtf16BOM[] = {0xff, 0xfe};

static const CHAR szUtf8XML[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\r\n";

static const CHAR szTestXML[] =
"<?xml version=\"1.0\" ?>\n"
"<BankAccount>\n"
"   <Number>1234</Number>\n"
"   <Name>Captain Ahab</Name>\n"
"</BankAccount>\n";

typedef struct _contenthandlercheck {
    CH id;
    int line;
    int column;
    const char *arg1;
    const char *arg2;
    const char *arg3;
} content_handler_test;

static content_handler_test contentHandlerTest1[] = {
    { CH_PUTDOCUMENTLOCATOR, 0, 0 },
    { CH_STARTDOCUMENT, 0, 0 },
    { CH_STARTELEMENT, 2, 14, "", "BankAccount", "BankAccount" },
    { CH_CHARACTERS, 2, 14, "\n   " },
    { CH_STARTELEMENT, 3, 12, "", "Number", "Number" },
    { CH_CHARACTERS, 3, 12, "1234" },
    { CH_ENDELEMENT, 3, 18, "", "Number", "Number" },
    { CH_CHARACTERS, 3, 25, "\n   " },
    { CH_STARTELEMENT, 4, 10, "", "Name", "Name" },
    { CH_CHARACTERS, 4, 10, "Captain Ahab" },
    { CH_ENDELEMENT, 4, 24, "", "Name", "Name" },
    { CH_CHARACTERS, 4, 29, "\n" },
    { CH_ENDELEMENT, 5, 3, "", "BankAccount", "BankAccount" },
    { CH_ENDDOCUMENT, 0, 0 },
    { CH_ENDTEST }
};

static content_handler_test contentHandlerTest2[] = {
    { CH_PUTDOCUMENTLOCATOR, 0, 0 },
    { CH_STARTDOCUMENT, 0, 0 },
    { CH_STARTELEMENT, 2, 14, "", "BankAccount", "BankAccount" },
    { CH_CHARACTERS, 2, 14, "\n" },
    { CH_CHARACTERS, 2, 16, "\t" },
    { CH_STARTELEMENT, 3, 10, "", "Number", "Number" },
    { CH_CHARACTERS, 3, 10, "1234" },
    { CH_ENDELEMENT, 3, 16, "", "Number", "Number" },
    { CH_CHARACTERS, 3, 23, "\n" },
    { CH_CHARACTERS, 3, 25, "\t" },
    { CH_STARTELEMENT, 4, 8, "", "Name", "Name" },
    { CH_CHARACTERS, 4, 8, "Captain Ahab" },
    { CH_ENDELEMENT, 4, 22, "", "Name", "Name" },
    { CH_CHARACTERS, 4, 27, "\n" },
    { CH_ENDELEMENT, 5, 3, "", "BankAccount", "BankAccount" },
    { CH_ENDDOCUMENT, 0, 0 },
    { CH_ENDTEST }
};

static content_handler_test *expectCall;
static ISAXLocator *locator;

static void test_saxstr(unsigned line, const WCHAR *szStr, int nStr, const char *szTest)
{
    WCHAR buf[1024];
    int len;

    if(!szTest) {
        ok_(__FILE__,line) (szStr == NULL, "szStr != NULL\n");
        ok_(__FILE__,line) (nStr == 0, "nStr = %d, expected 0\n", nStr);
        return;
    }

    len = strlen(szTest);
    ok_(__FILE__,line) (len == nStr, "nStr = %d, expected %d (%s)\n", nStr, len, szTest);
    if(len != nStr)
        return;

    MultiByteToWideChar(CP_ACP, 0, szTest, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok_(__FILE__,line) (!memcmp(szStr, buf, len*sizeof(WCHAR)), "unexpected szStr %s, expected %s\n",
                        wine_dbgstr_wn(szStr, nStr), szTest);
}

static BOOL test_expect_call(CH id)
{
    ok(expectCall->id == id, "unexpected call %d, expected %d\n", id, expectCall->id);
    return expectCall->id == id;
}

static void test_locator(unsigned line, int loc_line, int loc_column)
{
    int rcolumn, rline;
    ISAXLocator_getLineNumber(locator, &rline);
    ISAXLocator_getColumnNumber(locator, &rcolumn);

    ok_(__FILE__,line) (rline == loc_line,
            "unexpected line %d, expected %d\n", rline, loc_line);
    ok_(__FILE__,line) (rcolumn == loc_column,
            "unexpected column %d, expected %d\n", rcolumn, loc_column);
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
    if(!test_expect_call(CH_PUTDOCUMENTLOCATOR))
        return E_FAIL;

    locator = pLocator;
    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}

static HRESULT WINAPI contentHandler_startDocument(
        ISAXContentHandler* iface)
{
    if(!test_expect_call(CH_STARTDOCUMENT))
        return E_FAIL;

    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}

static HRESULT WINAPI contentHandler_endDocument(
        ISAXContentHandler* iface)
{
    if(!test_expect_call(CH_ENDDOCUMENT))
        return E_FAIL;

    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}

static HRESULT WINAPI contentHandler_startPrefixMapping(
        ISAXContentHandler* iface,
        const WCHAR *pPrefix,
        int nPrefix,
        const WCHAR *pUri,
        int nUri)
{
    if(!test_expect_call(CH_ENDDOCUMENT))
        return E_FAIL;

    test_saxstr(__LINE__, pPrefix, nPrefix, expectCall->arg1);
    test_saxstr(__LINE__, pUri, nUri, expectCall->arg2);
    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}

static HRESULT WINAPI contentHandler_endPrefixMapping(
        ISAXContentHandler* iface,
        const WCHAR *pPrefix,
        int nPrefix)
{
    if(!test_expect_call(CH_ENDPREFIXMAPPING))
        return E_FAIL;

    test_saxstr(__LINE__, pPrefix, nPrefix, expectCall->arg1);
    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
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
    if(!test_expect_call(CH_STARTELEMENT))
        return E_FAIL;

    test_saxstr(__LINE__, pNamespaceUri, nNamespaceUri, expectCall->arg1);
    test_saxstr(__LINE__, pLocalName, nLocalName, expectCall->arg2);
    test_saxstr(__LINE__, pQName, nQName, expectCall->arg3);
    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
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
    if(!test_expect_call(CH_ENDELEMENT))
        return E_FAIL;

    test_saxstr(__LINE__, pNamespaceUri, nNamespaceUri, expectCall->arg1);
    test_saxstr(__LINE__, pLocalName, nLocalName, expectCall->arg2);
    test_saxstr(__LINE__, pQName, nQName, expectCall->arg3);
    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}

static HRESULT WINAPI contentHandler_characters(
        ISAXContentHandler* iface,
        const WCHAR *pChars,
        int nChars)
{
    if(!test_expect_call(CH_CHARACTERS))
        return E_FAIL;

    test_saxstr(__LINE__, pChars, nChars, expectCall->arg1);
    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}

static HRESULT WINAPI contentHandler_ignorableWhitespace(
        ISAXContentHandler* iface,
        const WCHAR *pChars,
        int nChars)
{
    if(!test_expect_call(CH_IGNORABLEWHITESPACE))
        return E_FAIL;

    test_saxstr(__LINE__, pChars, nChars, expectCall->arg1);
    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}

static HRESULT WINAPI contentHandler_processingInstruction(
        ISAXContentHandler* iface,
        const WCHAR *pTarget,
        int nTarget,
        const WCHAR *pData,
        int nData)
{
    if(!test_expect_call(CH_PROCESSINGINSTRUCTION))
        return E_FAIL;

    test_saxstr(__LINE__, pTarget, nTarget, expectCall->arg1);
    test_saxstr(__LINE__, pData, nData, expectCall->arg2);
    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}

static HRESULT WINAPI contentHandler_skippedEntity(
        ISAXContentHandler* iface,
        const WCHAR *pName,
        int nName)
{
    if(!test_expect_call(CH_SKIPPEDENTITY))
        return E_FAIL;

    test_saxstr(__LINE__, pName, nName, expectCall->arg1);
    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
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

static HRESULT WINAPI isaxerrorHandler_QueryInterface(
        ISAXErrorHandler* iface,
        REFIID riid,
        void **ppvObject)
{
    *ppvObject = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ISAXErrorHandler))
    {
        *ppvObject = iface;
    }
    else
    {
        return E_NOINTERFACE;
    }

    return S_OK;
}

static ULONG WINAPI isaxerrorHandler_AddRef(
        ISAXErrorHandler* iface)
{
    return 2;
}

static ULONG WINAPI isaxerrorHandler_Release(
        ISAXErrorHandler* iface)
{
    return 1;
}

static HRESULT WINAPI isaxerrorHandler_error(
        ISAXErrorHandler* iface,
        ISAXLocator *pLocator,
        const WCHAR *pErrorMessage,
        HRESULT hrErrorCode)
{
    return S_OK;
}

static HRESULT WINAPI isaxerrorHandler_fatalError(
        ISAXErrorHandler* iface,
        ISAXLocator *pLocator,
        const WCHAR *pErrorMessage,
        HRESULT hrErrorCode)
{
    return S_OK;
}

static HRESULT WINAPI isaxerrorHanddler_ignorableWarning(
        ISAXErrorHandler* iface,
        ISAXLocator *pLocator,
        const WCHAR *pErrorMessage,
        HRESULT hrErrorCode)
{
    return S_OK;
}

static const ISAXErrorHandlerVtbl errorHandlerVtbl =
{
    isaxerrorHandler_QueryInterface,
    isaxerrorHandler_AddRef,
    isaxerrorHandler_Release,
    isaxerrorHandler_error,
    isaxerrorHandler_fatalError,
    isaxerrorHanddler_ignorableWarning
};

static ISAXErrorHandler errorHandler = { &errorHandlerVtbl };

static HRESULT WINAPI isaxattributes_QueryInterface(
        ISAXAttributes* iface,
        REFIID riid,
        void **ppvObject)
{
    *ppvObject = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ISAXAttributes))
    {
        *ppvObject = iface;
    }
    else
    {
        return E_NOINTERFACE;
    }

    return S_OK;
}

static ULONG WINAPI isaxattributes_AddRef(ISAXAttributes* iface)
{
    return 2;
}

static ULONG WINAPI isaxattributes_Release(ISAXAttributes* iface)
{
    return 1;
}

static HRESULT WINAPI isaxattributes_getLength(ISAXAttributes* iface, int *length)
{
    *length = 2;
    return S_OK;
}

static HRESULT WINAPI isaxattributes_getURI(
    ISAXAttributes* iface,
    int nIndex,
    const WCHAR **pUrl,
    int *pUriSize)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getLocalName(
    ISAXAttributes* iface,
    int nIndex,
    const WCHAR **pLocalName,
    int *pLocalNameLength)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getQName(
    ISAXAttributes* iface,
    int nIndex,
    const WCHAR **pQName,
    int *pQNameLength)
{
    static const WCHAR attr1W[] = {'a',':','a','t','t','r','1',0};
    static const WCHAR attr2W[] = {'a','t','t','r','2',0};

    ok(nIndex == 0 || nIndex == 1, "invalid index received %d\n", nIndex);

    *pQName = (nIndex == 0) ? attr1W : attr2W;
    *pQNameLength = lstrlenW(*pQName);

    return S_OK;
}

static HRESULT WINAPI isaxattributes_getName(
    ISAXAttributes* iface,
    int nIndex,
    const WCHAR **pUri,
    int * pUriLength,
    const WCHAR ** pLocalName,
    int * pLocalNameSize,
    const WCHAR ** pQName,
    int * pQNameLength)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getIndexFromName(
    ISAXAttributes* iface,
    const WCHAR * pUri,
    int cUriLength,
    const WCHAR * pLocalName,
    int cocalNameLength,
    int * index)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getIndexFromQName(
    ISAXAttributes* iface,
    const WCHAR * pQName,
    int nQNameLength,
    int * index)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getType(
    ISAXAttributes* iface,
    int nIndex,
    const WCHAR ** pType,
    int * pTypeLength)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getTypeFromName(
    ISAXAttributes* iface,
    const WCHAR * pUri,
    int nUri,
    const WCHAR * pLocalName,
    int nLocalName,
    const WCHAR ** pType,
    int * nType)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getTypeFromQName(
    ISAXAttributes* iface,
    const WCHAR * pQName,
    int nQName,
    const WCHAR ** pType,
    int * nType)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getValue(
    ISAXAttributes* iface,
    int nIndex,
    const WCHAR ** pValue,
    int * nValue)
{
    static const WCHAR attrval1W[] = {'a','1',0};
    static const WCHAR attrval2W[] = {'a','2',0};

    ok(nIndex == 0 || nIndex == 1, "invalid index received %d\n", nIndex);

    *pValue = (nIndex == 0) ? attrval1W : attrval2W;
    *nValue = lstrlenW(*pValue);

    return S_OK;
}

static HRESULT WINAPI isaxattributes_getValueFromName(
    ISAXAttributes* iface,
    const WCHAR * pUri,
    int nUri,
    const WCHAR * pLocalName,
    int nLocalName,
    const WCHAR ** pValue,
    int * nValue)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getValueFromQName(
    ISAXAttributes* iface,
    const WCHAR * pQName,
    int nQName,
    const WCHAR ** pValue,
    int * nValue)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const ISAXAttributesVtbl SAXAttributesVtbl =
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

static ISAXAttributes saxattributes = { &SAXAttributesVtbl };

typedef struct mxwriter_write_test_t {
    BOOL        last;
    const BYTE  *data;
    DWORD       cb;
    BOOL        null_written;
    BOOL        fail_write;
} mxwriter_write_test;

typedef struct mxwriter_stream_test_t {
    VARIANT_BOOL        bom;
    const char          *encoding;
    mxwriter_write_test expected_writes[4];
} mxwriter_stream_test;

static const mxwriter_write_test *current_write_test;
static DWORD current_stream_test_index;

static HRESULT WINAPI istream_QueryInterface(IStream *iface, REFIID riid, void **ppvObject)
{
    *ppvObject = NULL;

    if(IsEqualGUID(riid, &IID_IStream) || IsEqualGUID(riid, &IID_IUnknown))
        *ppvObject = iface;
    else
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI istream_AddRef(IStream *iface)
{
    return 2;
}

static ULONG WINAPI istream_Release(IStream *iface)
{
    return 1;
}

static HRESULT WINAPI istream_Read(IStream *iface, void *pv, ULONG cb, ULONG *pcbRead)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_Write(IStream *iface, const void *pv, ULONG cb, ULONG *pcbWritten)
{
    BOOL fail = FALSE;

    ok(pv != NULL, "pv == NULL\n");

    if(current_write_test->last) {
        ok(0, "Too many Write calls made on test %d\n", current_stream_test_index);
        return E_FAIL;
    }

    fail = current_write_test->fail_write;

    ok(current_write_test->cb == cb, "Expected %d, but got %d on test %d\n",
        current_write_test->cb, cb, current_stream_test_index);

    if(!pcbWritten)
        ok(current_write_test->null_written, "pcbWritten was NULL on test %d\n", current_stream_test_index);
    else
        ok(!memcmp(current_write_test->data, pv, cb), "Unexpected data on test %d\n", current_stream_test_index);

    ++current_write_test;

    if(pcbWritten)
        *pcbWritten = cb;

    return fail ? E_FAIL : S_OK;
}

static HRESULT WINAPI istream_Seek(IStream *iface, LARGE_INTEGER dlibMove, DWORD dwOrigin,
        ULARGE_INTEGER *plibNewPosition)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_SetSize(IStream *iface, ULARGE_INTEGER libNewSize)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_CopyTo(IStream *iface, IStream *pstm, ULARGE_INTEGER cb,
        ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *plibWritten)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_Commit(IStream *iface, DWORD grfCommitFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_Revert(IStream *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_LockRegion(IStream *iface, ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb, DWORD dwLockType)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_UnlockRegion(IStream *iface, ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb, DWORD dwLockType)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_Stat(IStream *iface, STATSTG *pstatstg, DWORD grfStatFlag)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_Clone(IStream *iface, IStream **ppstm)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IStreamVtbl StreamVtbl = {
    istream_QueryInterface,
    istream_AddRef,
    istream_Release,
    istream_Read,
    istream_Write,
    istream_Seek,
    istream_SetSize,
    istream_CopyTo,
    istream_Commit,
    istream_Revert,
    istream_LockRegion,
    istream_UnlockRegion,
    istream_Stat,
    istream_Clone
};

static IStream mxstream = { &StreamVtbl };

static void test_saxreader(void)
{
    HRESULT hr;
    ISAXXMLReader *reader = NULL;
    VARIANT var;
    ISAXContentHandler *lpContentHandler;
    ISAXErrorHandler *lpErrorHandler;
    SAFEARRAY *pSA;
    SAFEARRAYBOUND SADim[1];
    char *pSAData = NULL;
    IStream *iStream;
    ULARGE_INTEGER liSize;
    LARGE_INTEGER liPos;
    ULONG bytesWritten;
    HANDLE file;
    static const CHAR testXmlA[] = "test.xml";
    static const WCHAR testXmlW[] = {'t','e','s','t','.','x','m','l',0};
    IXMLDOMDocument *domDocument;
    BSTR bstrData;
    VARIANT_BOOL vBool;

    hr = CoCreateInstance(&CLSID_SAXXMLReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_ISAXXMLReader, (LPVOID*)&reader);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = ISAXXMLReader_getContentHandler(reader, NULL);
    ok(hr == E_POINTER, "Expected E_POINTER, got %08x\n", hr);

    hr = ISAXXMLReader_getErrorHandler(reader, NULL);
    ok(hr == E_POINTER, "Expected E_POINTER, got %08x\n", hr);

    hr = ISAXXMLReader_getContentHandler(reader, &lpContentHandler);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(lpContentHandler == NULL, "Expected %p, got %p\n", NULL, lpContentHandler);

    hr = ISAXXMLReader_getErrorHandler(reader, &lpErrorHandler);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(lpErrorHandler == NULL, "Expected %p, got %p\n", NULL, lpErrorHandler);

    hr = ISAXXMLReader_putContentHandler(reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = ISAXXMLReader_putContentHandler(reader, &contentHandler);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = ISAXXMLReader_putErrorHandler(reader, &errorHandler);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = ISAXXMLReader_getContentHandler(reader, &lpContentHandler);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(lpContentHandler == &contentHandler, "Expected %p, got %p\n", &contentHandler, lpContentHandler);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(szSimpleXML);

    expectCall = contentHandlerTest1;
    hr = ISAXXMLReader_parse(reader, var);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    test_expect_call(CH_ENDTEST);

    VariantClear(&var);

    SADim[0].lLbound= 0;
    SADim[0].cElements= sizeof(szTestXML)-1;
    pSA = SafeArrayCreate(VT_UI1, 1, SADim);
    SafeArrayAccessData(pSA, (void**)&pSAData);
    memcpy(pSAData, szTestXML, sizeof(szTestXML)-1);
    SafeArrayUnaccessData(pSA);
    V_VT(&var) = VT_ARRAY|VT_UI1;
    V_ARRAY(&var) = pSA;

    expectCall = contentHandlerTest1;
    hr = ISAXXMLReader_parse(reader, var);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    test_expect_call(CH_ENDTEST);

    SafeArrayDestroy(pSA);

    CreateStreamOnHGlobal(NULL, TRUE, &iStream);
    liSize.QuadPart = strlen(szTestXML);
    IStream_SetSize(iStream, liSize);
    IStream_Write(iStream, szTestXML, strlen(szTestXML), &bytesWritten);
    liPos.QuadPart = 0;
    IStream_Seek(iStream, liPos, STREAM_SEEK_SET, NULL);
    V_VT(&var) = VT_UNKNOWN|VT_DISPATCH;
    V_UNKNOWN(&var) = (IUnknown*)iStream;

    expectCall = contentHandlerTest1;
    hr = ISAXXMLReader_parse(reader, var);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    test_expect_call(CH_ENDTEST);

    IStream_Release(iStream);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(szCarriageRetTest);

    expectCall = contentHandlerTest2;
    hr = ISAXXMLReader_parse(reader, var);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    test_expect_call(CH_ENDTEST);

    VariantClear(&var);

    file = CreateFileA(testXmlA, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Could not create file: %u\n", GetLastError());
    WriteFile(file, szTestXML, sizeof(szTestXML)-1, &bytesWritten, NULL);
    CloseHandle(file);

    expectCall = contentHandlerTest1;
    hr = ISAXXMLReader_parseURL(reader, testXmlW);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    test_expect_call(CH_ENDTEST);

    DeleteFileA(testXmlA);

    hr = CoCreateInstance(&CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
            &IID_IXMLDOMDocument, (LPVOID*)&domDocument);
    if(FAILED(hr))
    {
        skip("Failed to create DOMDocument instance\n");
        return;
    }
    bstrData = SysAllocString(szSimpleXML);
    hr = IXMLDOMDocument_loadXML(domDocument, bstrData, &vBool);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = (IUnknown*)domDocument;

    expectCall = contentHandlerTest2;
    hr = ISAXXMLReader_parse(reader, var);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    test_expect_call(CH_ENDTEST);
    IXMLDOMDocument_Release(domDocument);

    ISAXXMLReader_Release(reader);
    SysFreeString(bstrData);
}

/* UTF-8 data with UTF-8 BOM and UTF-16 in prolog */
static const CHAR UTF8BOMTest[] =
"\xEF\xBB\xBF<?xml version = \"1.0\" encoding = \"UTF-16\"?>\n"
"<a></a>\n";

struct enc_test_entry_t {
    const GUID *guid;
    const char *clsid;
    const char *data;
    HRESULT hr;
    int todo;
};

static const struct enc_test_entry_t encoding_test_data[] = {
    { &CLSID_SAXXMLReader,   "CLSID_SAXXMLReader",   UTF8BOMTest, 0xc00ce56f, 1 },
    { &CLSID_SAXXMLReader30, "CLSID_SAXXMLReader30", UTF8BOMTest, 0xc00ce56f, 1 },
    { &CLSID_SAXXMLReader40, "CLSID_SAXXMLReader40", UTF8BOMTest, S_OK, 0 },
    { &CLSID_SAXXMLReader60, "CLSID_SAXXMLReader60", UTF8BOMTest, S_OK, 0 },
    { 0 }
};

static void test_encoding(void)
{
    const struct enc_test_entry_t *entry = encoding_test_data;
    static const WCHAR testXmlW[] = {'t','e','s','t','.','x','m','l',0};
    static const CHAR testXmlA[] = "test.xml";
    ISAXXMLReader *reader;
    DWORD written;
    HANDLE file;
    HRESULT hr;

    while (entry->guid)
    {
        hr = CoCreateInstance(entry->guid, NULL, CLSCTX_INPROC_SERVER, &IID_ISAXXMLReader, (void**)&reader);
        if (hr != S_OK)
        {
            win_skip("can't create %s instance\n", entry->clsid);
            entry++;
            continue;
        }

        file = CreateFileA(testXmlA, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        ok(file != INVALID_HANDLE_VALUE, "Could not create file: %u\n", GetLastError());
        WriteFile(file, UTF8BOMTest, sizeof(UTF8BOMTest)-1, &written, NULL);
        CloseHandle(file);

        hr = ISAXXMLReader_parseURL(reader, testXmlW);
        if (entry->todo)
            todo_wine ok(hr == entry->hr, "Expected 0x%08x, got 0x%08x. CLSID %s\n", entry->hr, hr, entry->clsid);
        else
            ok(hr == entry->hr, "Expected 0x%08x, got 0x%08x. CLSID %s\n", entry->hr, hr, entry->clsid);

        DeleteFileA(testXmlA);
        ISAXXMLReader_Release(reader);

        entry++;
    }
}

static void test_mxwriter_contenthandler(void)
{
    ISAXContentHandler *handler;
    IMXWriter *writer, *writer2;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    EXPECT_REF(writer, 1);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&handler);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    EXPECT_REF(writer, 2);
    EXPECT_REF(handler, 2);

    hr = ISAXContentHandler_QueryInterface(handler, &IID_IMXWriter, (void**)&writer2);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(writer2 == writer, "got %p, expected %p\n", writer2, writer);
    EXPECT_REF(writer, 3);
    EXPECT_REF(writer2, 3);
    IMXWriter_Release(writer2);

    ISAXContentHandler_Release(handler);
    IMXWriter_Release(writer);
}

static void test_mxwriter_properties(void)
{
    static const WCHAR utf16W[] = {'U','T','F','-','1','6',0};
    static const WCHAR emptyW[] = {0};
    static const WCHAR testW[] = {'t','e','s','t',0};
    IMXWriter *writer;
    VARIANT_BOOL b;
    HRESULT hr;
    BSTR str, str2;

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IMXWriter_get_disableOutputEscaping(writer, NULL);
    ok(hr == E_POINTER, "got %08x\n", hr);

    b = VARIANT_TRUE;
    hr = IMXWriter_get_disableOutputEscaping(writer, &b);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(b == VARIANT_FALSE, "got %d\n", b);

    hr = IMXWriter_get_byteOrderMark(writer, NULL);
    ok(hr == E_POINTER, "got %08x\n", hr);

    b = VARIANT_FALSE;
    hr = IMXWriter_get_byteOrderMark(writer, &b);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    hr = IMXWriter_get_indent(writer, NULL);
    ok(hr == E_POINTER, "got %08x\n", hr);

    b = VARIANT_TRUE;
    hr = IMXWriter_get_indent(writer, &b);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(b == VARIANT_FALSE, "got %d\n", b);

    hr = IMXWriter_get_omitXMLDeclaration(writer, NULL);
    ok(hr == E_POINTER, "got %08x\n", hr);

    b = VARIANT_TRUE;
    hr = IMXWriter_get_omitXMLDeclaration(writer, &b);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(b == VARIANT_FALSE, "got %d\n", b);

    hr = IMXWriter_get_standalone(writer, NULL);
    ok(hr == E_POINTER, "got %08x\n", hr);

    b = VARIANT_TRUE;
    hr = IMXWriter_get_standalone(writer, &b);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(b == VARIANT_FALSE, "got %d\n", b);

    /* set and check */
    hr = IMXWriter_put_standalone(writer, VARIANT_TRUE);
    ok(hr == S_OK, "got %08x\n", hr);

    b = VARIANT_FALSE;
    hr = IMXWriter_get_standalone(writer, &b);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    hr = IMXWriter_get_encoding(writer, NULL);
    ok(hr == E_POINTER, "got %08x\n", hr);

    /* UTF-16 is a default setting apparently */
    str = (void*)0xdeadbeef;
    hr = IMXWriter_get_encoding(writer, &str);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(lstrcmpW(str, utf16W) == 0, "expected empty string, got %s\n", wine_dbgstr_w(str));

    str2 = (void*)0xdeadbeef;
    hr = IMXWriter_get_encoding(writer, &str2);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(str != str2, "expected newly allocated, got same %p\n", str);

    SysFreeString(str2);
    SysFreeString(str);

    /* put empty string */
    str = SysAllocString(emptyW);
    hr = IMXWriter_put_encoding(writer, str);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);
    SysFreeString(str);

    str = (void*)0xdeadbeef;
    hr = IMXWriter_get_encoding(writer, &str);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(lstrcmpW(str, utf16W) == 0, "expected empty string, got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* invalid encoding name */
    str = SysAllocString(testW);
    hr = IMXWriter_put_encoding(writer, str);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);
    SysFreeString(str);

    hr = IMXWriter_get_version(writer, NULL);
    ok(hr == E_POINTER, "got %08x\n", hr);
    /* default version is 'surprisingly' 1.0 */
    hr = IMXWriter_get_version(writer, &str);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(!lstrcmpW(str, _bstr_("1.0")), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IMXWriter_Release(writer);
    free_bstrs();
}

static void test_mxwriter_flush(void)
{
    ISAXContentHandler *content;
    IMXWriter *writer;
    LARGE_INTEGER pos;
    ULARGE_INTEGER pos2;
    IStream *stream;
    VARIANT dest;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    EXPECT_REF(stream, 1);

    /* detach when nothing was attached */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* attach stream */
    V_VT(&dest) = VT_UNKNOWN;
    V_UNKNOWN(&dest) = (IUnknown*)stream;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    todo_wine EXPECT_REF(stream, 3);

    /* detach setting VT_EMPTY destination */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    EXPECT_REF(stream, 1);

    V_VT(&dest) = VT_UNKNOWN;
    V_UNKNOWN(&dest) = (IUnknown*)stream;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* flush() doesn't detach a stream */
    hr = IMXWriter_flush(writer);
todo_wine {
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    EXPECT_REF(stream, 3);
}

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(pos2.QuadPart == 0, "expected stream beginning\n");

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "got %08x\n", hr);

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    todo_wine ok(pos2.QuadPart != 0, "expected stream beginning\n");

    /* already started */
    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ISAXContentHandler_endDocument(content);
    todo_wine ok(hr == S_OK, "got %08x\n", hr);

    /* flushed on endDocument() */
    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    todo_wine ok(pos2.QuadPart != 0, "expected stream position moved\n");

    ISAXContentHandler_Release(content);
    IStream_Release(stream);
    IMXWriter_Release(writer);
}

static void test_mxwriter_startenddocument(void)
{
    ISAXContentHandler *content;
    IMXWriter *writer;
    VARIANT dest;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ISAXContentHandler_endDocument(content);
    todo_wine ok(hr == S_OK, "got %08x\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(_bstr_("<?xml version=\"1.0\" encoding=\"UTF-16\" standalone=\"no\"?>\r\n"), V_BSTR(&dest)),
        "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* now try another startDocument */
    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "got %08x\n", hr);
    /* and get duplicated prolog */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(_bstr_("<?xml version=\"1.0\" encoding=\"UTF-16\" standalone=\"no\"?>\r\n"
                        "<?xml version=\"1.0\" encoding=\"UTF-16\" standalone=\"no\"?>\r\n"), V_BSTR(&dest)),
        "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    /* now with omitted declaration */
    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ISAXContentHandler_endDocument(content);
    todo_wine ok(hr == S_OK, "got %08x\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(_bstr_(""), V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    free_bstrs();
}

static void test_mxwriter_startendelement(void)
{
    static const char winehqA[] = "http://winehq.org";
    ISAXContentHandler *content;
    IMXWriter *writer;
    VARIANT dest;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "got %08x\n", hr);

    /* qualified name without defined namespace */
    hr = ISAXContentHandler_startElement(content, NULL, 0, NULL, 0, _bstr_("a:b"), 3, NULL);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);

    hr = ISAXContentHandler_startElement(content, NULL, 0, _bstr_("b"), 1, _bstr_("a:b"), 3, NULL);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);

    /* only local name is an error too */
    hr = ISAXContentHandler_startElement(content, NULL, 0, _bstr_("b"), 1, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);

    /* only local name is an error too */
    hr = ISAXContentHandler_startElement(content, _bstr_(""), 0, _bstr_("b"), 1, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);

    /* all string pointers should be not null */
    hr = ISAXContentHandler_startElement(content, _bstr_(""), 0, _bstr_("b"), 1, _bstr_(""), 0, NULL);
    ok(hr == S_OK, "got %08x\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(_bstr_("<>"), V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXContentHandler_startElement(content, _bstr_(""), 0, _bstr_(""), 0, _bstr_("b"), 1, NULL);
    ok(hr == S_OK, "got %08x\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(_bstr_("<><b>"), V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXContentHandler_endElement(content, NULL, 0, NULL, 0, _bstr_("a:b"), 3);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);

    hr = ISAXContentHandler_endElement(content, NULL, 0, _bstr_("b"), 1, _bstr_("a:b"), 3);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);

    /* only local name is an error too */
    hr = ISAXContentHandler_endElement(content, NULL, 0, _bstr_("b"), 1, NULL, 0);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);

    hr = ISAXContentHandler_endElement(content, _bstr_(""), 0, _bstr_(""), 0, _bstr_("b"), 1);
    ok(hr == S_OK, "got %08x\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(_bstr_("<><b></b>"), V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* some with namespace URI */
    hr = ISAXContentHandler_startElement(content, _bstr_(winehqA), sizeof(winehqA), _bstr_(""), 0, _bstr_("nspace:c"), 8, NULL);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ISAXContentHandler_endElement(content, _bstr_(winehqA), sizeof(winehqA), _bstr_(""), 0, _bstr_("nspace:c"), 8);
    ok(hr == S_OK, "got %08x\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine ok(!lstrcmpW(_bstr_("<><b></b><nspace:c/>"), V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* try to end element that wasn't open */
    hr = ISAXContentHandler_endElement(content, _bstr_(""), 0, _bstr_(""), 0, _bstr_("a"), 1);
    ok(hr == S_OK, "got %08x\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine ok(!lstrcmpW(_bstr_("<><b></b><nspace:c/></a>"), V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* try with attributes */
    hr = ISAXContentHandler_startElement(content, _bstr_(""), 0, _bstr_(""), 0, _bstr_("b"), 1, &saxattributes);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ISAXContentHandler_endDocument(content);
    todo_wine ok(hr == S_OK, "got %08x\n", hr);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    free_bstrs();
}

static void test_mxwriter_characters(void)
{
    static const WCHAR chardataW[] = {'T','E','S','T','C','H','A','R','D','A','T','A',' ','.',0};
    ISAXContentHandler *content;
    IMXWriter *writer;
    VARIANT dest;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ISAXContentHandler_characters(content, NULL, 0);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);

    hr = ISAXContentHandler_characters(content, chardataW, 0);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ISAXContentHandler_characters(content, chardataW, sizeof(chardataW)/sizeof(WCHAR) - 1);
    ok(hr == S_OK, "got %08x\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(_bstr_("TESTCHARDATA ."), V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXContentHandler_endDocument(content);
    todo_wine ok(hr == S_OK, "got %08x\n", hr);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    free_bstrs();
}

static const mxwriter_stream_test mxwriter_stream_tests[] = {
    {
        VARIANT_TRUE,"UTF-16",
        {
            {FALSE,(const BYTE*)szUtf16BOM,sizeof(szUtf16BOM),TRUE},
            {FALSE,(const BYTE*)szUtf16XML,sizeof(szUtf16XML)},
            {TRUE}
        }
    },
    {
        VARIANT_FALSE,"UTF-16",
        {
            {FALSE,(const BYTE*)szUtf16XML,sizeof(szUtf16XML)},
            {TRUE}
        }
    },
    {
        VARIANT_TRUE,"UTF-8",
        {
            {FALSE,(const BYTE*)szUtf8XML,sizeof(szUtf8XML)-1},
            /* For some reason Windows makes an empty write call when UTF-8 encoding is used
             * and the writer is released.
             */
            {FALSE,NULL,0},
            {TRUE}
        }
    },
    {
        VARIANT_TRUE,"UTF-16",
        {
            {FALSE,(const BYTE*)szUtf16BOM,sizeof(szUtf16BOM),TRUE},
            {FALSE,(const BYTE*)szUtf16XML,sizeof(szUtf16XML)},
            {TRUE}
        }
    },
    {
        VARIANT_TRUE,"UTF-16",
        {
            {FALSE,(const BYTE*)szUtf16BOM,sizeof(szUtf16BOM),TRUE,TRUE},
            {FALSE,(const BYTE*)szUtf16XML,sizeof(szUtf16XML)},
            {TRUE}
        }
    }
};

static void test_mxwriter_stream(void)
{
    IMXWriter *writer;
    ISAXContentHandler *content;
    HRESULT hr;
    VARIANT dest;
    IStream *stream;
    LARGE_INTEGER pos;
    ULARGE_INTEGER pos2;
    DWORD test_count = sizeof(mxwriter_stream_tests)/sizeof(mxwriter_stream_tests[0]);

    for(current_stream_test_index = 0; current_stream_test_index < test_count; ++current_stream_test_index) {
        const mxwriter_stream_test *test = mxwriter_stream_tests+current_stream_test_index;

        hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
                &IID_IMXWriter, (void**)&writer);
        ok(hr == S_OK, "CoCreateInstance failed: %08x\n", hr);

        hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
        ok(hr == S_OK, "QueryInterface(ISAXContentHandler) failed: %08x\n", hr);

        hr = IMXWriter_put_encoding(writer, _bstr_(test->encoding));
        ok(hr == S_OK, "put_encoding failed with %08x on test %d\n", hr, current_stream_test_index);

        V_VT(&dest) = VT_UNKNOWN;
        V_UNKNOWN(&dest) = (IUnknown*)&mxstream;
        hr = IMXWriter_put_output(writer, dest);
        ok(hr == S_OK, "put_output failed with %08x on test %d\n", hr, current_stream_test_index);
        VariantClear(&dest);

        hr = IMXWriter_put_byteOrderMark(writer, test->bom);
        ok(hr == S_OK, "put_byteOrderMark failed with %08x on test %d\n", hr, current_stream_test_index);

        current_write_test = test->expected_writes;

        hr = ISAXContentHandler_startDocument(content);
        ok(hr == S_OK, "startDocument failed with %08x on test %d\n", hr, current_stream_test_index);

        hr = ISAXContentHandler_endDocument(content);
        todo_wine ok(hr == S_OK, "endDocument failed with %08x on test %d\n", hr, current_stream_test_index);

        ISAXContentHandler_Release(content);
        IMXWriter_Release(writer);

        todo_wine ok(current_write_test->last, "The last %d write calls on test %d were missed\n",
            current_write_test-test->expected_writes, current_stream_test_index);
    }

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "CoCreateInstance failed: %08x\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "CreateStreamOnHGlobal failed: %08x\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "QueryInterface(ISAXContentHandler) failed: %08x\n", hr);

    hr = IMXWriter_put_encoding(writer, _bstr_("UTF-8"));
    ok(hr == S_OK, "put_encoding failed: %08x\n", hr);

    V_VT(&dest) = VT_UNKNOWN;
    V_UNKNOWN(&dest) = (IUnknown*)stream;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "put_output failed: %08x\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "startDocument failed: %08x\n", hr);

    /* Setting output of the mxwriter causes the current output to be flushed,
     * and the writer to start over.
     */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "put_output failed: %08x\n", hr);

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Seek failed: %08x\n", hr);
    todo_wine ok(pos2.QuadPart != 0, "expected stream position moved\n");

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "startDocument failed: %08x\n", hr);

    hr = ISAXContentHandler_endDocument(content);
    todo_wine ok(hr == S_OK, "endDocument failed: %08x\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "get_output failed: %08x\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "Expected VT_BSTR, got %d\n", V_VT(&dest));
    todo_wine ok(!lstrcmpW(_bstr_("<?xml version=\"1.0\" encoding=\"UTF-16\" standalone=\"no\"?>\r\n"), V_BSTR(&dest)),
            "Got wrong content: %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    free_bstrs();
}

static void test_mxwriter_encoding(void)
{
    IMXWriter *writer;
    ISAXContentHandler *content;
    HRESULT hr;
    VARIANT dest;

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "CoCreateInstance failed: %08x\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "QueryInterface(ISAXContentHandler) failed: %08x\n", hr);

    hr = IMXWriter_put_encoding(writer, _bstr_("UTF-8"));
    ok(hr == S_OK, "put_encoding failed: %08x\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "startDocument failed: %08x\n", hr);

    hr = ISAXContentHandler_endDocument(content);
    todo_wine ok(hr == S_OK, "endDocument failed: %08x\n", hr);

    /* The content is always re-encoded to UTF-16 when the output is
     * retrieved as a BSTR.
     */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "get_output failed: %08x\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "Expected VT_BSTR, got %d\n", V_VT(&dest));
    todo_wine ok(!lstrcmpW(_bstr_("<?xml version=\"1.0\" encoding=\"UTF-16\" standalone=\"no\"?>\r\n"), V_BSTR(&dest)),
            "got wrong content: %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    free_bstrs();
}

START_TEST(saxreader)
{
    ISAXXMLReader *reader;
    IMXWriter *writer;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "failed to init com\n");

    hr = CoCreateInstance(&CLSID_SAXXMLReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_ISAXXMLReader, (void**)&reader);

    if(FAILED(hr))
    {
        skip("Failed to create SAXXMLReader instance\n");
        CoUninitialize();
        return;
    }
    ISAXXMLReader_Release(reader);

    test_saxreader();
    test_encoding();

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    if (hr == S_OK)
    {
        IMXWriter_Release(writer);

        test_mxwriter_contenthandler();
        test_mxwriter_startenddocument();
        test_mxwriter_startendelement();
        test_mxwriter_characters();
        test_mxwriter_properties();
        test_mxwriter_flush();
        test_mxwriter_stream();
        test_mxwriter_encoding();
    }
    else
        win_skip("MXXMLWriter not supported\n");

    CoUninitialize();
}
