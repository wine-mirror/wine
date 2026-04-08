/*
 * SAXReader/MXWriter tests
 *
 * Copyright 2008 Piotr Caban
 * Copyright 2011 Thomas Mullaly
 * Copyright 2012 Nikolay Sivov
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
#include "msxml2did.h"
#include "ocidl.h"
#include "dispex.h"

#include "wine/test.h"

struct sink
{
    IVBSAXContentHandler IVBSAXContentHandler_iface;
    LONG refcount;
};

static struct sink *impl_from_IVBSAXContentHandler(IVBSAXContentHandler *iface)
{
    return CONTAINING_RECORD(iface, struct sink, IVBSAXContentHandler_iface);
}

static HRESULT WINAPI vb_content_handler_QueryInterface(IVBSAXContentHandler *iface,
        REFIID riid, void **obj)
{
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDispatch)
        || IsEqualGUID(riid, &IID_IVBSAXContentHandler))
    {
        *obj = iface;
        IVBSAXContentHandler_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI vb_content_handler_AddRef(IVBSAXContentHandler *iface)
{
    struct sink *sink = impl_from_IVBSAXContentHandler(iface);
    return InterlockedIncrement(&sink->refcount);
}

static ULONG WINAPI vb_content_handler_Release(IVBSAXContentHandler *iface)
{
    struct sink *sink = impl_from_IVBSAXContentHandler(iface);
    LONG refcount = InterlockedDecrement(&sink->refcount);

    if (!refcount)
        free(sink);

    return refcount;
}

static HRESULT WINAPI vb_content_handler_GetTypeInfoCount(IVBSAXContentHandler *iface, UINT *count)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI vb_content_handler_GetTypeInfo(IVBSAXContentHandler *iface, UINT index,
        LCID lcid, ITypeInfo **ti)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI vb_content_handler_GetIDsOfNames(IVBSAXContentHandler *iface, REFIID riid,
        LPOLESTR *names, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI vb_content_handler_Invoke(IVBSAXContentHandler *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
        UINT *puArgErr)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI vb_content_handler_putref_documentLocator(IVBSAXContentHandler *iface, IVBSAXLocator *locator)
{
    return S_OK;
}

static HRESULT WINAPI vb_content_handler_startDocument(IVBSAXContentHandler *iface)
{
    return S_OK;
}

static HRESULT WINAPI vb_content_handler_endDocument(IVBSAXContentHandler *iface)
{
    return S_OK;
}

static HRESULT WINAPI vb_content_handler_startPrefixMapping(IVBSAXContentHandler *iface, BSTR *prefix, BSTR *uri)
{
    return S_OK;
}

static HRESULT WINAPI vb_content_handler_endPrefixMapping(IVBSAXContentHandler *iface, BSTR *prefix)
{
    return S_OK;
}

static HRESULT WINAPI vb_content_handler_startElement(IVBSAXContentHandler *iface, BSTR *uri, BSTR *localName,
        BSTR *qname, IVBSAXAttributes * oAttributes)
{
    ok(uri != NULL && *uri != NULL, "Unexpected pointer.\n");
    ok(localName != NULL && *localName != NULL, "Unexpected pointer.\n");
    ok(qname != NULL && *qname != NULL, "Unexpected pointer.\n");
    return S_OK;
}

static HRESULT WINAPI vb_content_handler_endElement(IVBSAXContentHandler *iface, BSTR *uri, BSTR *localName,
        BSTR * strQName)
{
    return S_OK;
}

static HRESULT WINAPI vb_content_handler_characters(IVBSAXContentHandler *iface, BSTR * chars)
{
    return S_OK;
}

static HRESULT WINAPI vb_content_handler_ignorableWhitespace(IVBSAXContentHandler *iface, BSTR * chars)
{
    return S_OK;
}

static HRESULT WINAPI vb_content_handler_processingInstruction(IVBSAXContentHandler *iface, BSTR * target, BSTR * data)
{
    return S_OK;
}

static HRESULT WINAPI vb_content_handler_skippedEntity(IVBSAXContentHandler *iface, BSTR * name)
{
    return S_OK;
}

static const IVBSAXContentHandlerVtbl vbcontenthandlervtbl =
{
    vb_content_handler_QueryInterface,
    vb_content_handler_AddRef,
    vb_content_handler_Release,
    vb_content_handler_GetTypeInfoCount,
    vb_content_handler_GetTypeInfo,
    vb_content_handler_GetIDsOfNames,
    vb_content_handler_Invoke,
    vb_content_handler_putref_documentLocator,
    vb_content_handler_startDocument,
    vb_content_handler_endDocument,
    vb_content_handler_startPrefixMapping,
    vb_content_handler_endPrefixMapping,
    vb_content_handler_startElement,
    vb_content_handler_endElement,
    vb_content_handler_characters,
    vb_content_handler_ignorableWhitespace,
    vb_content_handler_processingInstruction,
    vb_content_handler_skippedEntity,
};

static struct sink *create_test_sink(void)
{
    struct sink *sink = calloc(1, sizeof(*sink));

    sink->IVBSAXContentHandler_iface.lpVtbl = &vbcontenthandlervtbl;
    sink->refcount = 1;

    return sink;
}

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__, line)(rc == ref, "expected refcount %ld, got %ld.\n", ref, rc);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static LONG get_refcount(void *iface)
{
    IUnknown *unk = iface;
    LONG ref;

    ref = IUnknown_AddRef(unk);
    IUnknown_Release(unk);
    return ref-1;
}

struct msxmlsupported_data_t
{
    const GUID *clsid;
    const char *name;
    BOOL supported;
};

static BOOL is_clsid_supported(const GUID *clsid, const struct msxmlsupported_data_t *table)
{
    while (table->clsid)
    {
        if (table->clsid == clsid) return table->supported;
        table++;
    }
    return FALSE;
}

static BSTR alloc_str_from_narrow(const char *str)
{
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    BSTR ret = SysAllocStringLen(NULL, len - 1);  /* NUL character added automatically */
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

static BSTR alloced_bstrs[512];
static int alloced_bstrs_count;

static BSTR _bstr_(const char *str)
{
    assert(alloced_bstrs_count < ARRAY_SIZE(alloced_bstrs));
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

static void test_saxstr(const char *file, unsigned line, BSTR str, const WCHAR *expected, BOOL todo, int *failcount)
{
    int len, lenexp, cmp;

    len = SysStringLen(str);

    if (!expected) {
        if (str && todo)
        {
            (*failcount)++;
            todo_wine
            ok_(file, line) (!str, "got %p, expected null str\n", str);
        }
        else
            ok_(file, line) (!str, "got %p, expected null str\n", str);

        if (len && todo)
        {
            (*failcount)++;
            todo_wine
            ok_(file, line) (len == 0, "got len %d, expected 0\n", len);
        }
        else
            ok_(file, line) (len == 0, "got len %d, expected 0\n", len);
        return;
    }

    lenexp = lstrlenW(expected);
    if (lenexp != len && todo)
    {
        (*failcount)++;
        todo_wine
        ok_(file, line) (lenexp == len, "len %d (%s), expected %d (%s)\n", len, wine_dbgstr_wn(str, len),
                lenexp, wine_dbgstr_w(expected));
    }
    else
        ok_(file, line) (lenexp == len, "len %d (%s), expected %d (%s)\n", len, wine_dbgstr_wn(str, len),
                lenexp, wine_dbgstr_w(expected));

    /* exit earlier on length mismatch */
    if (lenexp != len) return;

    cmp = lstrcmpW(str, expected);
    if (cmp && todo)
    {
        (*failcount)++;
        todo_wine
        ok_(file, line) (!cmp, "unexpected str %s, expected %s\n", wine_dbgstr_wn(str, len), wine_dbgstr_w(expected));
    }
    else
        ok_(file, line) (!cmp, "unexpected str %s, expected %s\n", wine_dbgstr_wn(str, len), wine_dbgstr_w(expected));
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
    CH_SKIPPEDENTITY,
    LH_STARTCDATA,
    LH_ENDCDATA,
    LH_COMMENT,
    EH_ERROR,
    EH_FATALERROR,
    EH_IGNORABLEWARNING,
    EVENT_LAST
} CH;

static const char *event_names[EVENT_LAST] = {
    "endtest",
    "putDocumentLocator",
    "startDocument",
    "endDocument",
    "startPrefixMapping",
    "endPrefixMapping",
    "startElement",
    "endElement",
    "characters",
    "ignorableWhitespace",
    "processingInstruction",
    "skippedEntity",
    "startCDATA",
    "endCDATA",
    "comment",
    "error",
    "fatalError",
    "ignorableWarning"
};

struct attribute_entry
{
    const WCHAR *uri;
    const WCHAR *local;
    const WCHAR *qname;
    const WCHAR *value;

    /* used for actual call data only, null for expected call data */
    BSTR uriW;
    BSTR localW;
    BSTR qnameW;
    BSTR valueW;
};

struct call_entry
{
    CH id;
    int line;
    int column;
    HRESULT ret;
    const WCHAR *arg1;
    const WCHAR *arg2;
    const WCHAR *arg3;

    /* allocated once at startElement callback */
    struct attribute_entry *attributes;
    int attr_count;

    /* used for actual call data only, null for expected call data */
    BSTR arg1W;
    BSTR arg2W;
    BSTR arg3W;
};

struct call_sequence
{
    int count;
    int size;
    struct call_entry *sequence;
};

#define CONTENT_HANDLER_INDEX 0
#define NUM_CALL_SEQUENCES    1
static struct call_sequence *sequences[NUM_CALL_SEQUENCES];

static void init_call_entry(ISAXLocator *locator, struct call_entry *call)
{
    memset(call, 0, sizeof(*call));
    ISAXLocator_getLineNumber(locator, &call->line);
    ISAXLocator_getColumnNumber(locator, &call->column);
}

static void add_call(struct call_sequence **seq, int sequence_index,
    const struct call_entry *call)
{
    struct call_sequence *call_seq = seq[sequence_index];

    if (!call_seq->sequence)
    {
        call_seq->size = 10;
        call_seq->sequence = malloc(call_seq->size * sizeof (struct call_entry));
    }

    if (call_seq->count == call_seq->size)
    {
        call_seq->size *= 2;
        call_seq->sequence = realloc(call_seq->sequence, call_seq->size * sizeof (struct call_entry));
    }

    assert(call_seq->sequence);

    call_seq->sequence[call_seq->count].id     = call->id;
    call_seq->sequence[call_seq->count].line   = call->line;
    call_seq->sequence[call_seq->count].column = call->column;
    call_seq->sequence[call_seq->count].arg1W  = call->arg1W;
    call_seq->sequence[call_seq->count].arg2W  = call->arg2W;
    call_seq->sequence[call_seq->count].arg3W  = call->arg3W;
    call_seq->sequence[call_seq->count].ret    = call->ret;
    call_seq->sequence[call_seq->count].attr_count = call->attr_count;
    call_seq->sequence[call_seq->count].attributes = call->attributes;

    call_seq->count++;
}

static inline void flush_sequence(struct call_sequence **seg, int sequence_index)
{
    int i;

    struct call_sequence *call_seq = seg[sequence_index];

    for (i = 0; i < call_seq->count; i++)
    {
        int j;

        for (j = 0; j < call_seq->sequence[i].attr_count; j++)
        {
            SysFreeString(call_seq->sequence[i].attributes[j].uriW);
            SysFreeString(call_seq->sequence[i].attributes[j].localW);
            SysFreeString(call_seq->sequence[i].attributes[j].qnameW);
            SysFreeString(call_seq->sequence[i].attributes[j].valueW);
        }
        free(call_seq->sequence[i].attributes);
        call_seq->sequence[i].attr_count = 0;

        SysFreeString(call_seq->sequence[i].arg1W);
        SysFreeString(call_seq->sequence[i].arg2W);
        SysFreeString(call_seq->sequence[i].arg3W);
    }

    free(call_seq->sequence);
    call_seq->sequence = NULL;
    call_seq->count = call_seq->size = 0;
}

static const char *get_event_name(CH event)
{
    return event_names[event];
}

static void compare_attributes(const struct call_entry *actual, const struct call_entry *expected, const char *context,
    BOOL todo, const char *file, int line, int *failcount)
{
    int i, lenexp = 0;

    /* attribute count is not stored for expected data */
    if (expected->attributes)
    {
        struct attribute_entry *ptr = expected->attributes;
        while (ptr->uri) { lenexp++; ptr++; };
    }

    /* check count first and exit earlier */
    if (actual->attr_count != lenexp && todo)
    {
        (*failcount)++;
        todo_wine
            ok_(file, line) (FALSE, "%s: in event %s expecting attr count %d got %d\n",
                context, get_event_name(actual->id), lenexp, actual->attr_count);
    }
    else
        ok_(file, line) (actual->attr_count == lenexp, "%s: in event %s expecting attr count %d got %d\n",
            context, get_event_name(actual->id), lenexp, actual->attr_count);

    if (actual->attr_count != lenexp) return;

    /* now compare all attributes strings */
    for (i = 0; i < actual->attr_count; i++)
    {
        test_saxstr(file, line, actual->attributes[i].uriW,   expected->attributes[i].uri, todo, failcount);
        test_saxstr(file, line, actual->attributes[i].localW, expected->attributes[i].local, todo, failcount);
        test_saxstr(file, line, actual->attributes[i].qnameW, expected->attributes[i].qname, todo, failcount);
        test_saxstr(file, line, actual->attributes[i].valueW, expected->attributes[i].value, todo, failcount);
    }
}

static void ok_sequence_(struct call_sequence **seq, int sequence_index,
    const struct call_entry *expected, const char *context, BOOL todo,
    const char *file, int line)
{
    struct call_sequence *call_seq = seq[sequence_index];
    static const struct call_entry end_of_sequence = { CH_ENDTEST };
    const struct call_entry *actual, *sequence;
    int failcount = 0;

    add_call(seq, sequence_index, &end_of_sequence);

    sequence = call_seq->sequence;
    actual = sequence;

    while (expected->id != CH_ENDTEST && actual->id != CH_ENDTEST)
    {
        if (expected->id == actual->id)
        {
            if (expected->line != -1)
            {
                /* always test position data */
                if (expected->line != actual->line && todo)
                {
                    todo_wine
                    {
                        failcount++;
                        ok_(file, line) (FALSE,
                            "%s: in event %s expecting line %d got %d\n",
                            context, get_event_name(actual->id), expected->line, actual->line);
                    }
                }
                else
                {
                    ok_(file, line) (expected->line == actual->line,
                        "%s: in event %s expecting line %d got %d\n",
                        context, get_event_name(actual->id), expected->line, actual->line);
                }
            }


            if (expected->column != -1)
            {
                if (expected->column != actual->column && todo)
                {
                    todo_wine
                    {
                        failcount++;
                        ok_(file, line) (FALSE,
                            "%s: in event %s expecting column %d got %d\n",
                            context, get_event_name(actual->id), expected->column, actual->column);
                    }
                }
                else
                {
                    ok_(file, line) (expected->column == actual->column,
                        "%s: in event %s expecting column %d got %d\n",
                        context, get_event_name(actual->id), expected->column, actual->column);
                }
            }

            switch (actual->id)
            {
            case CH_PUTDOCUMENTLOCATOR:
            case CH_STARTDOCUMENT:
            case CH_ENDDOCUMENT:
            case LH_STARTCDATA:
            case LH_ENDCDATA:
                break;
            case CH_STARTPREFIXMAPPING:
                /* prefix, uri */
                test_saxstr(file, line, actual->arg1W, expected->arg1, todo, &failcount);
                test_saxstr(file, line, actual->arg2W, expected->arg2, todo, &failcount);
                break;
            case CH_ENDPREFIXMAPPING:
                /* prefix */
                test_saxstr(file, line, actual->arg1W, expected->arg1, todo, &failcount);
                break;
            case CH_STARTELEMENT:
                /* compare attributes */
                compare_attributes(actual, expected, context, todo, file, line, &failcount);
                /* fallthrough */
            case CH_ENDELEMENT:
                /* uri, localname, qname */
                test_saxstr(file, line, actual->arg1W, expected->arg1, todo, &failcount);
                test_saxstr(file, line, actual->arg2W, expected->arg2, todo, &failcount);
                test_saxstr(file, line, actual->arg3W, expected->arg3, todo, &failcount);
                break;
            case CH_CHARACTERS:
            case CH_IGNORABLEWHITESPACE:
            case LH_COMMENT:
                /* char data */
                test_saxstr(file, line, actual->arg1W, expected->arg1, todo, &failcount);
                break;
            case CH_PROCESSINGINSTRUCTION:
                /* target, data */
                test_saxstr(file, line, actual->arg1W, expected->arg1, todo, &failcount);
                test_saxstr(file, line, actual->arg2W, expected->arg2, todo, &failcount);
                break;
            case CH_SKIPPEDENTITY:
                /* name */
                test_saxstr(file, line, actual->arg1W, expected->arg1, todo, &failcount);
                break;
            case EH_FATALERROR:
                /* test return value only */
                if (expected->ret != actual->ret && todo)
                {
                     failcount++;
                     ok_(file, line) (FALSE,
                         "%s: in event %s expecting ret %#lx got %#lx\n",
                         context, get_event_name(actual->id), expected->ret, actual->ret);
                }
                else
                     ok_(file, line) (expected->ret == actual->ret,
                         "%s: in event %s expecting ret %#lx got %#lx\n",
                         context, get_event_name(actual->id), expected->ret, actual->ret);
                break;
            case EH_ERROR:
            case EH_IGNORABLEWARNING:
            default:
                ok(0, "%s: callback not handled, %s\n", context, get_event_name(actual->id));
            }
            expected++;
            actual++;
        }
        else if (todo)
        {
            failcount++;
            todo_wine
            {
                ok_(file, line) (FALSE, "%s: call %s was expected, but got call %s instead\n",
                    context, get_event_name(expected->id), get_event_name(actual->id));
            }

            flush_sequence(seq, sequence_index);
            return;
        }
        else
        {
            ok_(file, line) (FALSE, "%s: call %s was expected, but got call %s instead\n",
                context, get_event_name(expected->id), get_event_name(actual->id));
            expected++;
            actual++;
        }
    }

    if (todo)
    {
        todo_wine
        {
            if (expected->id != CH_ENDTEST || actual->id != CH_ENDTEST)
            {
                failcount++;
                ok_(file, line) (FALSE, "%s: the call sequence is not complete: expected %s - actual %s\n",
                    context, get_event_name(expected->id), get_event_name(actual->id));
            }
        }
    }
    else if (expected->id != CH_ENDTEST || actual->id != CH_ENDTEST)
    {
        ok_(file, line) (FALSE, "%s: the call sequence is not complete: expected %s - actual %s\n",
            context, get_event_name(expected->id), get_event_name(actual->id));
    }

    if (todo && !failcount) /* succeeded yet marked todo */
    {
        todo_wine
        {
            ok_(file, line)(TRUE, "%s: marked \"todo_wine\" but succeeds\n", context);
        }
    }

    flush_sequence(seq, sequence_index);
}

#define ok_sequence(seq, index, exp, contx, todo) \
        ok_sequence_(seq, index, (exp), (contx), (todo), __FILE__, __LINE__)

static void init_call_sequences(struct call_sequence **seq, int n)
{
    int i;

    for (i = 0; i < n; i++)
        seq[i] = calloc(1, sizeof(**seq));
}

static const WCHAR szSimpleXML[] =

L"<?xml version=\"1.0\" ?>\n"
"<BankAccount>\n"
"   <Number>1234</Number>\n"
"   <Name>Captain Ahab</Name>\n"
"</BankAccount>\n";

static const WCHAR carriage_ret_test[] =

L"<?xml version=\"1.0\"?>\r\n"
"<BankAccount>\r\n\t<Number>1234</Number>\r\n\t"
"<Name>Captain Ahab</Name>\r\n"
"</BankAccount>\r\n";

static const WCHAR szUtf16XML[] = L"<?xml version=\"1.0\" encoding=\"UTF-16\" standalone=\"no\"?>\r\n";

static const CHAR szUtf16BOM[] = {0xff, 0xfe};

static const CHAR szUtf8XML[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\r\n";

static const char utf8xml2[] =
"<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"no\"?>\r\n";

static const char testXML[] =
"<?xml version=\"1.0\" ?>\n"
"<BankAccount>\n"
"   <Number>1234</Number>\n"
"   <Name>Captain Ahab</Name>\n"
"</BankAccount>\n";

static const char test_attributes[] =
"<?xml version=\"1.0\" ?>\n"
"<document xmlns:test=\"prefix_test\" xmlns=\"prefix\" test:arg1=\"arg1\" arg2=\"arg2\" test:ar3=\"arg3\">\n"
"<node1 xmlns:p=\"test\" />"
"</document>\n";

static const char test_cdata_xml[] =
"<?xml version=\"1.0\" ?>"
"<a><![CDATA[Some \r\ntext\n\r\ndata\n\n]]></a>";

static const char test2_cdata_xml[] =
"<?xml version=\"1.0\" ?>"
"<a><![CDATA[\n\r\nSome \r\ntext\n\r\ndata\n\n]]></a>";

static const char test3_cdata_xml[] =
"<?xml version=\"1.0\" ?><a><![CDATA[Some text data]]></a>";

static const char test_pi_xml[] =
"<?xml version=\"1.0\" ?><a><?t some t\rex\r\nt ?></a>";

static const char test_chardata_xml[] =
"<?xml version=\"1.0\" ?><a>\nabc<b>de\nf</b>gh\n</a>";

static const char test_chardata_xml2[] =
"<?xml version=\"1.0\" ?><a>\rabc<b>de\rf</b>gh\r</a>";

static struct call_entry content_handler_test1[] = {
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 2, 14, S_OK, L"", L"BankAccount", L"BankAccount" },
    { CH_CHARACTERS, 2, 14, S_OK, L"\n   " },
    { CH_STARTELEMENT, 3, 12, S_OK, L"", L"Number", L"Number" },
    { CH_CHARACTERS, 3, 12, S_OK, L"1234" },
    { CH_ENDELEMENT, 3, 18, S_OK, L"", L"Number", L"Number" },
    { CH_CHARACTERS, 3, 25, S_OK, L"\n   " },
    { CH_STARTELEMENT, 4, 10, S_OK, L"", L"Name", L"Name" },
    { CH_CHARACTERS, 4, 10, S_OK, L"Captain Ahab" },
    { CH_ENDELEMENT, 4, 24, S_OK, L"", L"Name", L"Name" },
    { CH_CHARACTERS, 4, 29, S_OK, L"\n" },
    { CH_ENDELEMENT, 5, 3, S_OK, L"", L"BankAccount", L"BankAccount" },
    { CH_ENDDOCUMENT, 0, 0, S_OK},
    { CH_ENDTEST }
};

static struct call_entry content_handler_test2[] = {
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 2, 14, S_OK, L"", L"BankAccount", L"BankAccount" },
    { CH_CHARACTERS, 2, 14, S_OK, L"\n" },
    { CH_CHARACTERS, 2, 16, S_OK, L"\t" },
    { CH_STARTELEMENT, 3, 10, S_OK, L"", L"Number", L"Number" },
    { CH_CHARACTERS, 3, 10, S_OK, L"1234" },
    { CH_ENDELEMENT, 3, 16, S_OK, L"", L"Number", L"Number" },
    { CH_CHARACTERS, 3, 23, S_OK, L"\n" },
    { CH_CHARACTERS, 3, 25, S_OK, L"\t" },
    { CH_STARTELEMENT, 4, 8, S_OK, L"", L"Name", L"Name" },
    { CH_CHARACTERS, 4, 8, S_OK, L"Captain Ahab" },
    { CH_ENDELEMENT, 4, 22, S_OK, L"", L"Name", L"Name" },
    { CH_CHARACTERS, 4, 27, S_OK, L"\n" },
    { CH_ENDELEMENT, 5, 3, S_OK, L"", L"BankAccount", L"BankAccount" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct call_entry content_handler_testerror[] = {
    { CH_PUTDOCUMENTLOCATOR, 0, 0, E_FAIL },
    { EH_FATALERROR, 0, 0, E_FAIL },
    { CH_ENDTEST }
};

static struct call_entry content_handler_test_callback_rets[] = {
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_FALSE },
    { CH_STARTDOCUMENT, 0, 0, S_FALSE },
    { EH_FATALERROR, 0, 0, S_FALSE },
    { CH_ENDTEST }
};

static struct attribute_entry ch_attributes1[] = {
    { L"", L"", L"xmlns:test", L"prefix_test" },
    { L"", L"", L"xmlns", L"prefix" },
    { L"prefix_test", L"arg1", L"test:arg1", L"arg1" },
    { L"", L"arg2", L"arg2", L"arg2" },
    { L"prefix_test", L"ar3", L"test:ar3", L"arg3" },
    { NULL }
};

static struct attribute_entry ch_attributes2[] = {
    { L"", L"", L"xmlns:p", L"test" },
    { NULL }
};

static struct call_entry content_handler_test_attributes[] = {
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTPREFIXMAPPING, 2, 96, S_OK, L"test", L"prefix_test" },
    { CH_STARTPREFIXMAPPING, 2, 96, S_OK, L"", L"prefix" },
    { CH_STARTELEMENT, 2, 96, S_OK, L"prefix", L"document", L"document", ch_attributes1 },
    { CH_CHARACTERS, 2, 96, S_OK, L"\n" },
    { CH_STARTPREFIXMAPPING, 3, 25, S_OK, L"p", L"test" },
    { CH_STARTELEMENT, 3, 25, S_OK, L"prefix", L"node1", L"node1", ch_attributes2 },
    { CH_ENDELEMENT, 3, 25, S_OK, L"prefix", L"node1", L"node1" },
    { CH_ENDPREFIXMAPPING, 3, 25, S_OK, L"p" },
    { CH_ENDELEMENT, 3, 27, S_OK, L"prefix", L"document", L"document" },
    { CH_ENDPREFIXMAPPING, 3, 27, S_OK, L"" },
    { CH_ENDPREFIXMAPPING, 3, 27, S_OK, L"test" },
    { CH_ENDDOCUMENT, 0, 0 },
    { CH_ENDTEST }
};

/* 'namespaces' is on, 'namespace-prefixes' if off */
static struct attribute_entry ch_attributes_no_prefix[] =
{
    { L"prefix_test", L"arg1", L"test:arg1", L"arg1" },
    { L"", L"arg2", L"arg2", L"arg2" },
    { L"prefix_test", L"ar3", L"test:ar3", L"arg3" },
    { NULL }
};

static struct call_entry content_handler_test_attributes_no_prefix[] = {
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTPREFIXMAPPING, 2, 96, S_OK, L"test", L"prefix_test" },
    { CH_STARTPREFIXMAPPING, 2, 96, S_OK, L"", L"prefix" },
    { CH_STARTELEMENT, 2, 96, S_OK, L"prefix", L"document", L"document", ch_attributes_no_prefix },
    { CH_CHARACTERS, 2, 96, S_OK, L"\n" },
    { CH_STARTPREFIXMAPPING, 3, 25, S_OK, L"p", L"test" },
    { CH_STARTELEMENT, 3, 25, S_OK, L"prefix", L"node1", L"node1", NULL },
    { CH_ENDELEMENT, 3, 25, S_OK, L"prefix", L"node1", L"node1" },
    { CH_ENDPREFIXMAPPING, 3, 25, S_OK, L"p" },
    { CH_ENDELEMENT, 3, 27, S_OK, L"prefix", L"document", L"document" },
    { CH_ENDPREFIXMAPPING, 3, 27, S_OK, L"" },
    { CH_ENDPREFIXMAPPING, 3, 27, S_OK, L"test" },
    { CH_ENDDOCUMENT, 0, 0 },
    { CH_ENDTEST }
};

static struct attribute_entry xmlspace_attrs[] =
{
    { L"http://www.w3.org/XML/1998/namespace", L"space", L"xml:space", L"preserve" },
    { NULL }
};

static struct call_entry xmlspaceattr_test[] = {
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 64, S_OK, L"", L"a", L"a", xmlspace_attrs },
    { CH_CHARACTERS, 1, 64, S_OK, L" Some text data " },
    { CH_ENDELEMENT, 1, 82, S_OK, L"", L"a", L"a" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

/* attribute value normalization test */
static const char attribute_normalize[] =
    "<?xml version=\"1.0\" ?>\n"
    "<a attr1=\" \r \n \tattr_value &#65; &#38; &amp;\t \r \n\r\n \n\"/>\n";

static struct attribute_entry attribute_norm_attrs[] =
{
    { L"", L"attr1", L"attr1", L"      attr_value A & &        " },
    { NULL }
};

static struct call_entry attribute_norm[] = {
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 6, 4, S_OK, L"", L"a", L"a", attribute_norm_attrs },
    { CH_ENDELEMENT, 6, 4, S_OK, L"", L"a", L"a" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct call_entry cdata_test[] = {
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 26, S_OK, L"", L"a", L"a" },
    { LH_STARTCDATA, 1, 35, S_OK },
    { CH_CHARACTERS, 1, 35, S_OK, L"Some \n" },
    { CH_CHARACTERS, 1, 42, S_OK, L"text\n\n" },
    { CH_CHARACTERS, 1, 49, S_OK, L"data\n\n" },
    { LH_ENDCDATA, 1, 49, S_OK },
    { CH_ENDELEMENT, 6, 6, S_OK, L"", L"a", L"a" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct call_entry cdata_test2[] = {
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 26, S_OK, L"", L"a", L"a" },
    { LH_STARTCDATA, 1, 35, S_OK },
    { CH_CHARACTERS, 1, 35, S_OK, L"\n\n" },
    { CH_CHARACTERS, 1, 38, S_OK, L"Some \n" },
    { CH_CHARACTERS, 1, 45, S_OK, L"text\n\n" },
    { CH_CHARACTERS, 1, 52, S_OK, L"data\n\n" },
    { LH_ENDCDATA, 1, 52, S_OK },
    { CH_ENDELEMENT, 8, 6, S_OK, L"", L"a", L"a" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct call_entry cdata_test3[] = {
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 26, S_OK, L"", L"a", L"a" },
    { LH_STARTCDATA, 1, 35, S_OK },
    { CH_CHARACTERS, 1, 35, S_OK, L"Some text data" },
    { LH_ENDCDATA, 1, 35, S_OK },
    { CH_ENDELEMENT, 1, 54, S_OK, L"", L"a", L"a" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct call_entry pi_test[] =
{
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 26, S_OK, L"", L"a", L"a" },
    { CH_PROCESSINGINSTRUCTION, 1, 30, S_OK, L"t", L"some t\nex\nt " },
    { CH_ENDELEMENT, 3, 7, S_OK, L"", L"a", L"a" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct call_entry chardata_test[] =
{
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 26, S_OK, L"", L"a", L"a" },
    { CH_CHARACTERS, 1, 26, S_OK, L"\nabc" },
    { CH_STARTELEMENT, 2, 7, S_OK, L"", L"b", L"b" },
    { CH_CHARACTERS, 2, 7, S_OK, L"de\nf" },
    { CH_ENDELEMENT, 3, 4, S_OK, L"", L"b", L"b" },
    { CH_CHARACTERS, 3, 6, S_OK, L"gh\n" },
    { CH_ENDELEMENT, 4, 3, S_OK, L"", L"a", L"a" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct call_entry chardata_test2[] =
{
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 26, S_OK, L"", L"a", L"a" },
    { CH_CHARACTERS, 1, 26, S_OK, L"\n" },
    { CH_CHARACTERS, 2, 1, S_OK, L"abc" },
    { CH_STARTELEMENT, 2, 7, S_OK, L"", L"b", L"b" },
    { CH_CHARACTERS, 2, 7, S_OK, L"de\n" },
    { CH_CHARACTERS, 3, 1, S_OK, L"f" },
    { CH_ENDELEMENT, 3, 4, S_OK, L"", L"b", L"b" },
    { CH_CHARACTERS, 3, 6, S_OK, L"gh\n" },
    { CH_ENDELEMENT, 4, 3, S_OK, L"", L"a", L"a" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct attribute_entry read_test_attrs[] =
{
    { L"", L"attr", L"attr", L"val" },
    { NULL }
};

static struct call_entry read_test_seq[] = {
    { CH_PUTDOCUMENTLOCATOR, -1, 0, S_OK },
    { CH_STARTDOCUMENT, -1, -1, S_OK },
    { CH_STARTELEMENT, -1, -1, S_OK, L"", L"rootelem", L"rootelem" },
    { CH_STARTELEMENT, -1, -1, S_OK, L"", L"elem", L"elem", read_test_attrs },
    { CH_CHARACTERS, -1, -1, S_OK, L"text" },
    { CH_ENDELEMENT, -1, -1, S_OK, L"", L"elem", L"elem" },
    { CH_STARTELEMENT, -1, -1, S_OK, L"", L"elem", L"elem", read_test_attrs },
    { CH_CHARACTERS, -1, -1, S_OK, L"text" },
    { CH_ENDELEMENT, -1, -1, S_OK, L"", L"elem", L"elem" },
    { CH_STARTELEMENT, -1, -1, S_OK, L"", L"elem", L"elem", read_test_attrs },
    { CH_CHARACTERS, -1, -1, S_OK, L"text" },
    { CH_ENDELEMENT, -1, -1, S_OK, L"", L"elem", L"elem" },
    { CH_STARTELEMENT, -1, -1, S_OK, L"", L"elem", L"elem", read_test_attrs },
    { CH_CHARACTERS, -1, -1, S_OK, L"text" },
    { CH_ENDELEMENT, -1, -1, S_OK, L"", L"elem", L"elem" },
    { CH_ENDELEMENT, -1, -1, S_OK, L"", L"rootelem", L"rootelem" },
    { CH_ENDDOCUMENT, -1, -1, S_OK},
    { CH_ENDTEST }
};

static const char xmlspace_attr[] =
    "<?xml version=\"1.0\" encoding=\"UTF-16\"?>"
    "<a xml:space=\"preserve\"> Some text data </a>";

static struct call_entry *expectCall;
static ISAXLocator *locator;
static ISAXXMLReader *g_reader;
int msxml_version;

static void set_expected_seq(struct call_entry *expected)
{
    expectCall = expected;
}

/* to be called once on each tested callback return */
static HRESULT get_expected_ret(void)
{
    HRESULT hr = expectCall->ret;
    if (expectCall->id != CH_ENDTEST) expectCall++;
    return hr;
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
    struct call_entry call;
    IUnknown *unk;
    HRESULT hr;

    locator = pLocator;

    init_call_entry(locator, &call);
    call.id = CH_PUTDOCUMENTLOCATOR;
    add_call(sequences, CONTENT_HANDLER_INDEX, &call);

    hr = ISAXLocator_QueryInterface(pLocator, &IID_IVBSAXLocator, (void**)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

    if (msxml_version >= 6) {
        ISAXAttributes *attr, *attr1;
        IMXAttributes *mxattr;

        EXPECT_REF(pLocator, 1);
        hr = ISAXLocator_QueryInterface(pLocator, &IID_ISAXAttributes, (void**)&attr);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        EXPECT_REF(pLocator, 2);
        hr = ISAXLocator_QueryInterface(pLocator, &IID_ISAXAttributes, (void**)&attr1);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        EXPECT_REF(pLocator, 3);
        ok(attr == attr1, "got %p, %p\n", attr, attr1);

        hr = ISAXAttributes_QueryInterface(attr, &IID_IVBSAXAttributes, (void**)&unk);
        ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

        hr = ISAXLocator_QueryInterface(pLocator, &IID_IVBSAXAttributes, (void**)&unk);
        ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

        hr = ISAXAttributes_QueryInterface(attr, &IID_IMXAttributes, (void**)&mxattr);
        ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

        ISAXAttributes_Release(attr);
        ISAXAttributes_Release(attr1);
    }

    return get_expected_ret();
}

static ISAXAttributes *test_attr_ptr;
static HRESULT WINAPI contentHandler_startDocument(
        ISAXContentHandler* iface)
{
    struct call_entry call;

    init_call_entry(locator, &call);
    call.id = CH_STARTDOCUMENT;
    add_call(sequences, CONTENT_HANDLER_INDEX, &call);

    test_attr_ptr = NULL;

    return get_expected_ret();
}

static HRESULT WINAPI contentHandler_endDocument(
        ISAXContentHandler* iface)
{
    struct call_entry call;

    init_call_entry(locator, &call);
    call.id = CH_ENDDOCUMENT;
    add_call(sequences, CONTENT_HANDLER_INDEX, &call);

    return get_expected_ret();
}

static HRESULT WINAPI contentHandler_startPrefixMapping(
        ISAXContentHandler* iface,
        const WCHAR *prefix, int prefix_len,
        const WCHAR *uri, int uri_len)
{
    struct call_entry call;

    ok(prefix != NULL, "prefix == NULL\n");
    ok(uri != NULL, "uri == NULL\n");

    init_call_entry(locator, &call);
    call.id = CH_STARTPREFIXMAPPING;
    call.arg1W = SysAllocStringLen(prefix, prefix_len);
    call.arg2W = SysAllocStringLen(uri, uri_len);
    add_call(sequences, CONTENT_HANDLER_INDEX, &call);

    return get_expected_ret();
}

static HRESULT WINAPI contentHandler_endPrefixMapping(
        ISAXContentHandler* iface,
        const WCHAR *prefix, int len)
{
    struct call_entry call;

    ok(prefix != NULL, "prefix == NULL\n");

    init_call_entry(locator, &call);
    call.id = CH_ENDPREFIXMAPPING;
    call.arg1W = SysAllocStringLen(prefix, len);
    add_call(sequences, CONTENT_HANDLER_INDEX, &call);

    return get_expected_ret();
}

static HRESULT WINAPI contentHandler_startElement(
        ISAXContentHandler* iface,
        const WCHAR *uri, int uri_len,
        const WCHAR *localname, int local_len,
        const WCHAR *qname, int qname_len,
        ISAXAttributes *saxattr)
{
    struct call_entry call;
    HRESULT hr;
    int len;

    ok(uri != NULL, "uri == NULL\n");
    ok(localname != NULL, "localname == NULL\n");
    ok(qname != NULL, "qname == NULL\n");
    ok(!!saxattr, "Unexpected pointer.\n");

    check_interface(saxattr, &IID_IMXAttributes, FALSE);
    check_interface(saxattr, &IID_IVBSAXAttributes, FALSE);
    check_interface(saxattr, &IID_IVBSAXLocator, FALSE);
    check_interface(saxattr, &IID_IDispatch, FALSE);
    check_interface(saxattr, &IID_IDispatchEx, FALSE);
    check_interface(saxattr, &IID_ISAXAttributes, TRUE);
    check_interface(saxattr, &IID_ISAXXMLReader, FALSE);
    check_interface(saxattr, &IID_IVBSAXXMLReader, FALSE);
    todo_wine
    check_interface(saxattr, &IID_ISAXLocator, FALSE);

    init_call_entry(locator, &call);
    call.id = CH_STARTELEMENT;
    call.arg1W = SysAllocStringLen(uri, uri_len);
    call.arg2W = SysAllocStringLen(localname, local_len);
    call.arg3W = SysAllocStringLen(qname, qname_len);

    if(!test_attr_ptr)
        test_attr_ptr = saxattr;
    ok(test_attr_ptr == saxattr, "Multiple ISAXAttributes instances are used (%p %p)\n", test_attr_ptr, saxattr);

    hr = ISAXAttributes_getValueFromQName(saxattr, NULL, 0, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /* store actual attributes */
    len = 0;
    hr = ISAXAttributes_getLength(saxattr, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    if (len)
    {
        VARIANT_BOOL v;
        int i;

        struct attribute_entry *attr;
        attr = calloc(len, sizeof(*attr));

        v = VARIANT_TRUE;
        hr = ISAXXMLReader_getFeature(g_reader, L"http://xml.org/sax/features/namespaces", &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        for (i = 0; i < len; i++)
        {
            const WCHAR *value;
            int value_len;

            hr = ISAXAttributes_getName(saxattr, i, &uri, &uri_len,
                &localname, &local_len, &qname, &qname_len);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValue(saxattr, i, &value, &value_len);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            /* if 'namespaces' switched off uri and local name contains garbage */
            if (v == VARIANT_FALSE && msxml_version > 0)
            {
                attr[i].uriW   = SysAllocStringLen(NULL, 0);
                attr[i].localW = SysAllocStringLen(NULL, 0);
            }
            else
            {
                attr[i].uriW   = SysAllocStringLen(uri, uri_len);
                attr[i].localW = SysAllocStringLen(localname, local_len);
            }

            attr[i].qnameW = SysAllocStringLen(qname, qname_len);
            attr[i].valueW = SysAllocStringLen(value, value_len);
        }

        call.attributes = attr;
        call.attr_count = len;
    }

    add_call(sequences, CONTENT_HANDLER_INDEX, &call);

    return get_expected_ret();
}

static HRESULT WINAPI contentHandler_endElement(
        ISAXContentHandler* iface,
        const WCHAR *uri, int uri_len,
        const WCHAR *localname, int local_len,
        const WCHAR *qname, int qname_len)
{
    struct call_entry call;

    ok(uri != NULL, "uri == NULL\n");
    ok(localname != NULL, "localname == NULL\n");
    ok(qname != NULL, "qname == NULL\n");

    init_call_entry(locator, &call);
    call.id = CH_ENDELEMENT;
    call.arg1W = SysAllocStringLen(uri, uri_len);
    call.arg2W = SysAllocStringLen(localname, local_len);
    call.arg3W = SysAllocStringLen(qname, qname_len);
    add_call(sequences, CONTENT_HANDLER_INDEX, &call);

    return get_expected_ret();
}

static HRESULT WINAPI contentHandler_characters(
        ISAXContentHandler* iface,
        const WCHAR *chars,
        int len)
{
    struct call_entry call;

    ok(chars != NULL, "chars == NULL\n");

    init_call_entry(locator, &call);
    call.id = CH_CHARACTERS;
    call.arg1W = SysAllocStringLen(chars, len);
    add_call(sequences, CONTENT_HANDLER_INDEX, &call);

    return get_expected_ret();
}

static HRESULT WINAPI contentHandler_ignorableWhitespace(
        ISAXContentHandler* iface,
        const WCHAR *chars, int len)
{
    struct call_entry call;

    ok(chars != NULL, "chars == NULL\n");

    init_call_entry(locator, &call);
    call.id = CH_IGNORABLEWHITESPACE;
    call.arg1W = SysAllocStringLen(chars, len);
    add_call(sequences, CONTENT_HANDLER_INDEX, &call);

    return get_expected_ret();
}

static HRESULT WINAPI contentHandler_processingInstruction(
        ISAXContentHandler* iface,
        const WCHAR *target, int target_len,
        const WCHAR *data, int data_len)
{
    struct call_entry call;

    ok(target != NULL, "target == NULL\n");
    ok(data != NULL, "data == NULL\n");

    init_call_entry(locator, &call);
    call.id = CH_PROCESSINGINSTRUCTION;
    call.arg1W = SysAllocStringLen(target, target_len);
    call.arg2W = SysAllocStringLen(data, data_len);
    add_call(sequences, CONTENT_HANDLER_INDEX, &call);

    return get_expected_ret();
}

static HRESULT WINAPI contentHandler_skippedEntity(
        ISAXContentHandler* iface,
        const WCHAR *name, int len)
{
    struct call_entry call;

    ok(name != NULL, "name == NULL\n");

    init_call_entry(locator, &call);
    call.id = CH_SKIPPEDENTITY;
    call.arg1W = SysAllocStringLen(name, len);
    add_call(sequences, CONTENT_HANDLER_INDEX, &call);

    return get_expected_ret();
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
    ok(0, "unexpected call\n");
    return S_OK;
}

static HRESULT WINAPI isaxerrorHandler_fatalError(
        ISAXErrorHandler* iface,
        ISAXLocator *pLocator,
        const WCHAR *message,
        HRESULT hr)
{
    struct call_entry call;

    init_call_entry(locator, &call);
    call.id  = EH_FATALERROR;
    call.ret = hr;

    add_call(sequences, CONTENT_HANDLER_INDEX, &call);

    get_expected_ret();
    return S_OK;
}

static HRESULT WINAPI isaxerrorHandler_ignorableWarning(
        ISAXErrorHandler* iface,
        ISAXLocator *pLocator,
        const WCHAR *pErrorMessage,
        HRESULT hrErrorCode)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static const ISAXErrorHandlerVtbl errorHandlerVtbl =
{
    isaxerrorHandler_QueryInterface,
    isaxerrorHandler_AddRef,
    isaxerrorHandler_Release,
    isaxerrorHandler_error,
    isaxerrorHandler_fatalError,
    isaxerrorHandler_ignorableWarning
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
    *length = 4;
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
    int index,
    const WCHAR **QName,
    int *QNameLength)
{
    static const WCHAR attrqnamesW[][15] = {L"a:attr1junk",
                                            L"attr2junk",
                                            L"attr3",
                                            L"attr4"};
    static const int attrqnamelen[] = {7, 5, 5, 5};

    ok(index >= 0 && index <= 3, "invalid index received %d\n", index);

    if (index >= 0 && index <= 3)
    {
        *QName = attrqnamesW[index];
        *QNameLength = attrqnamelen[index];
    }
    else
    {
        *QName = NULL;
        *QNameLength = 0;
    }

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

static HRESULT WINAPI isaxattributes_getValue(ISAXAttributes* iface, int index,
    const WCHAR **value, int *nValue)
{
    static const WCHAR attrvaluesW[][10] = {L"a1junk",
                                            L"a2junk",
                                            L"<&\">'",
                                            L"a\rb\nc\r\n"};
    static const int attrvalueslen[] = {2, 2, 5, 7};

    ok(index >= 0 && index <= 3, "invalid index received %d\n", index);

    if (index >= 0 && index <= 3)
    {
        *value = attrvaluesW[index];
        *nValue = attrvalueslen[index];
    }
    else
    {
        *value = NULL;
        *nValue = 0;
    }

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

struct saxlexicalhandler
{
    ISAXLexicalHandler ISAXLexicalHandler_iface;
    LONG ref;

    HRESULT qi_hr; /* ret value for QueryInterface for handler riid */
};

static inline struct saxlexicalhandler *impl_from_ISAXLexicalHandler( ISAXLexicalHandler *iface )
{
    return CONTAINING_RECORD(iface, struct saxlexicalhandler, ISAXLexicalHandler_iface);
}

static HRESULT WINAPI isaxlexical_QueryInterface(ISAXLexicalHandler* iface, REFIID riid, void **out)
{
    struct saxlexicalhandler *handler = impl_from_ISAXLexicalHandler(iface);

    *out = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = iface;
        ok(0, "got unexpected IID_IUnknown query\n");
    }
    else if (IsEqualGUID(riid, &IID_ISAXLexicalHandler))
    {
        if (handler->qi_hr == E_NOINTERFACE) return handler->qi_hr;
        *out = iface;
    }

    if (*out)
        ISAXLexicalHandler_AddRef(iface);
    else
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI isaxlexical_AddRef(ISAXLexicalHandler* iface)
{
    struct saxlexicalhandler *handler = impl_from_ISAXLexicalHandler(iface);
    return InterlockedIncrement(&handler->ref);
}

static ULONG WINAPI isaxlexical_Release(ISAXLexicalHandler* iface)
{
    struct saxlexicalhandler *handler = impl_from_ISAXLexicalHandler(iface);
    return InterlockedDecrement(&handler->ref);
}

static HRESULT WINAPI isaxlexical_startDTD(ISAXLexicalHandler* iface,
    const WCHAR * pName, int nName, const WCHAR * pPublicId,
    int nPublicId, const WCHAR * pSystemId, int nSystemId)
{
    return S_OK;
}

static HRESULT WINAPI isaxlexical_endDTD(ISAXLexicalHandler* iface)
{
    return S_OK;
}

static HRESULT WINAPI isaxlexical_startEntity(ISAXLexicalHandler *iface,
    const WCHAR * pName, int nName)
{
    ok(0, "call not expected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxlexical_endEntity(ISAXLexicalHandler *iface,
    const WCHAR * pName, int nName)
{
    ok(0, "call not expected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxlexical_startCDATA(ISAXLexicalHandler *iface)
{
    struct call_entry call;

    init_call_entry(locator, &call);
    call.id = LH_STARTCDATA;
    add_call(sequences, CONTENT_HANDLER_INDEX, &call);

    return get_expected_ret();
}

static HRESULT WINAPI isaxlexical_endCDATA(ISAXLexicalHandler *iface)
{
    struct call_entry call;

    init_call_entry(locator, &call);
    call.id = LH_ENDCDATA;
    add_call(sequences, CONTENT_HANDLER_INDEX, &call);

    return get_expected_ret();
}

static HRESULT WINAPI isaxlexical_comment(ISAXLexicalHandler *iface,
    const WCHAR *chars, int len)
{
    struct call_entry call;

    init_call_entry(locator, &call);
    call.id = LH_COMMENT;
    call.arg1W = SysAllocStringLen(chars, len);
    add_call(sequences, CONTENT_HANDLER_INDEX, &call);

    return get_expected_ret();
}

static const ISAXLexicalHandlerVtbl SAXLexicalHandlerVtbl =
{
   isaxlexical_QueryInterface,
   isaxlexical_AddRef,
   isaxlexical_Release,
   isaxlexical_startDTD,
   isaxlexical_endDTD,
   isaxlexical_startEntity,
   isaxlexical_endEntity,
   isaxlexical_startCDATA,
   isaxlexical_endCDATA,
   isaxlexical_comment
};

static void init_saxlexicalhandler(struct saxlexicalhandler *handler, HRESULT hr)
{
    handler->ISAXLexicalHandler_iface.lpVtbl = &SAXLexicalHandlerVtbl;
    handler->ref = 1;
    handler->qi_hr = hr;
}

struct saxdeclhandler
{
    ISAXDeclHandler ISAXDeclHandler_iface;
    LONG ref;

    HRESULT qi_hr; /* ret value for QueryInterface for handler riid */
};

static inline struct saxdeclhandler *impl_from_ISAXDeclHandler( ISAXDeclHandler *iface )
{
    return CONTAINING_RECORD(iface, struct saxdeclhandler, ISAXDeclHandler_iface);
}

static HRESULT WINAPI isaxdecl_QueryInterface(ISAXDeclHandler* iface, REFIID riid, void **out)
{
    struct saxdeclhandler *handler = impl_from_ISAXDeclHandler(iface);

    *out = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = iface;
        ok(0, "got unexpected IID_IUnknown query\n");
    }
    else if (IsEqualGUID(riid, &IID_ISAXDeclHandler))
    {
        if (handler->qi_hr == E_NOINTERFACE) return handler->qi_hr;
        *out = iface;
    }

    if (*out)
        ISAXDeclHandler_AddRef(iface);
    else
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI isaxdecl_AddRef(ISAXDeclHandler* iface)
{
    struct saxdeclhandler *handler = impl_from_ISAXDeclHandler(iface);
    return InterlockedIncrement(&handler->ref);
}

static ULONG WINAPI isaxdecl_Release(ISAXDeclHandler* iface)
{
    struct saxdeclhandler *handler = impl_from_ISAXDeclHandler(iface);
    return InterlockedDecrement(&handler->ref);
}

static HRESULT WINAPI isaxdecl_elementDecl(ISAXDeclHandler* iface,
    const WCHAR * pName, int nName, const WCHAR * pModel, int nModel)
{
    return S_OK;
}

static HRESULT WINAPI isaxdecl_attributeDecl(ISAXDeclHandler* iface,
    const WCHAR * pElementName, int nElementName, const WCHAR * pAttributeName,
    int nAttributeName, const WCHAR * pType, int nType, const WCHAR * pValueDefault,
    int nValueDefault, const WCHAR * pValue, int nValue)
{
    return S_OK;
}

static HRESULT WINAPI isaxdecl_internalEntityDecl(ISAXDeclHandler* iface,
    const WCHAR * pName, int nName, const WCHAR * pValue, int nValue)
{
    return S_OK;
}

static HRESULT WINAPI isaxdecl_externalEntityDecl(ISAXDeclHandler* iface,
    const WCHAR * pName, int nName, const WCHAR * pPublicId, int nPublicId,
    const WCHAR * pSystemId, int nSystemId)
{
    return S_OK;
}

static const ISAXDeclHandlerVtbl SAXDeclHandlerVtbl =
{
   isaxdecl_QueryInterface,
   isaxdecl_AddRef,
   isaxdecl_Release,
   isaxdecl_elementDecl,
   isaxdecl_attributeDecl,
   isaxdecl_internalEntityDecl,
   isaxdecl_externalEntityDecl
};

static void init_saxdeclhandler(struct saxdeclhandler *handler, HRESULT hr)
{
    handler->ISAXDeclHandler_iface.lpVtbl = &SAXDeclHandlerVtbl;
    handler->ref = 1;
    handler->qi_hr = hr;
}

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
static unsigned int current_stream_test_index;

static HRESULT WINAPI istream_QueryInterface(IStream *iface, REFIID riid, void **ppvObject)
{
    *ppvObject = NULL;

    ok(!IsEqualGUID(riid, &IID_IPersistStream), "Did not expect QI for IPersistStream\n");

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
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
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
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_Clone(IStream *iface, IStream **ppstm)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI mxstream_Write(IStream *iface, const void *pv, ULONG cb, ULONG *pcbWritten)
{
    BOOL fail = FALSE;

    ok(pv != NULL, "pv == NULL\n");

    if(current_write_test->last) {
        ok(0, "Too many Write calls made on test %d\n", current_stream_test_index);
        return E_FAIL;
    }

    fail = current_write_test->fail_write;

    ok(current_write_test->cb == cb, "Expected %ld, but got %ld on test %d\n",
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

static const IStreamVtbl mxstreamVtbl = {
    istream_QueryInterface,
    istream_AddRef,
    istream_Release,
    istream_Read,
    mxstream_Write,
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

static IStream mxstream = { &mxstreamVtbl };

static int read_cnt;

static HRESULT WINAPI instream_Read(IStream *iface, void *pv, ULONG cb, ULONG *pcbRead)
{
    static const char *ret_str;

    if(!read_cnt)
        ret_str = "<?xml version=\"1.0\" ?>\n<rootelem>";
    else if(read_cnt < 5)
        ret_str = "<elem attr=\"val\">text</elem>";
    else if(read_cnt == 5)
        ret_str = "</rootelem>\n";
    else
        ret_str = "";

    read_cnt++;
    strcpy(pv, ret_str);
    *pcbRead = strlen(ret_str);
    return S_OK;
}

static const IStreamVtbl instreamVtbl = {
    istream_QueryInterface,
    istream_AddRef,
    istream_Release,
    instream_Read,
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

static IStream instream = { &instreamVtbl };

static struct msxmlsupported_data_t reader_support_data[] =
{
    { &CLSID_SAXXMLReader,   "SAXReader"   },
    { &CLSID_SAXXMLReader30, "SAXReader30" },
    { NULL }
};

static struct saxlexicalhandler lexicalhandler;
static struct saxdeclhandler declhandler;

static IStream *create_test_stream(const char *data, int len)
{
     ULARGE_INTEGER size;
     LARGE_INTEGER pos;
     IStream *stream;
     ULONG written;

     if (len == -1) len = strlen(data);
     CreateStreamOnHGlobal(NULL, TRUE, &stream);
     size.QuadPart = len;
     IStream_SetSize(stream, size);
     IStream_Write(stream, data, len, &written);
     pos.QuadPart = 0;
     IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);

     return stream;
}

static void test_saxreader(void)
{
    const struct msxmlsupported_data_t *table = reader_support_data;
    HRESULT hr;
    ISAXXMLReader *reader = NULL;
    VARIANT var;
    ISAXContentHandler *content;
    ISAXErrorHandler *lpErrorHandler;
    SAFEARRAY *sa;
    SAFEARRAYBOUND SADim[1];
    char *ptr = NULL;
    IStream *stream;
    ULONG written;
    HANDLE file;
    static const CHAR testXmlA[] = "test.xml";
    IXMLDOMDocument *doc;
    VARIANT_BOOL v;

    while (table->clsid)
    {
        struct call_entry *test_seq;
        ISAXEntityResolver *resolver;
        IVBSAXXMLReader *vb_reader;
        BSTR str;

        if (!is_clsid_supported(table->clsid, reader_support_data))
        {
            table++;
            continue;
        }

        hr = CoCreateInstance(table->clsid, NULL, CLSCTX_INPROC_SERVER, &IID_ISAXXMLReader, (void**)&reader);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        g_reader = reader;

        check_interface(reader, &IID_IDispatch, TRUE);
        check_interface(reader, &IID_IDispatchEx, TRUE);
        check_interface(reader, &IID_ISAXXMLReader, TRUE);
        check_interface(reader, &IID_IVBSAXXMLReader, TRUE);
        check_interface(reader, &IID_ISAXLocator, FALSE);
        check_interface(reader, &IID_IVBSAXLocator, FALSE);

            msxml_version = 0;

        hr = ISAXXMLReader_getContentHandler(reader, NULL);
        ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

        hr = ISAXXMLReader_getErrorHandler(reader, NULL);
        ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

        hr = ISAXXMLReader_getContentHandler(reader, &content);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(content == NULL, "Expected %p, got %p\n", NULL, content);

        hr = ISAXXMLReader_getErrorHandler(reader, &lpErrorHandler);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(lpErrorHandler == NULL, "Expected %p, got %p\n", NULL, lpErrorHandler);

        hr = ISAXXMLReader_putContentHandler(reader, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXXMLReader_putContentHandler(reader, &contentHandler);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXXMLReader_putErrorHandler(reader, &errorHandler);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXXMLReader_getContentHandler(reader, &content);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(content == &contentHandler, "Expected %p, got %p\n", &contentHandler, content);

        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(szSimpleXML);

        test_seq = content_handler_test1;
        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, "content test 1", FALSE);

        VariantClear(&var);

        SADim[0].lLbound = 0;
        SADim[0].cElements = sizeof(testXML)-1;
        sa = SafeArrayCreate(VT_UI1, 1, SADim);
        SafeArrayAccessData(sa, (void**)&ptr);
        memcpy(ptr, testXML, sizeof(testXML)-1);
        SafeArrayUnaccessData(sa);
        V_VT(&var) = VT_ARRAY|VT_UI1;
        V_ARRAY(&var) = sa;

        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, "content test 1: from safe array", FALSE);

        SafeArrayDestroy(sa);

        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = NULL;
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        V_VT(&var) = VT_DISPATCH;
        V_DISPATCH(&var) = NULL;
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        stream = create_test_stream(testXML, -1);
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown*)stream;

        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, "content test 1: from stream", FALSE);

        IStream_Release(stream);

        stream = create_test_stream(test_attributes, -1);
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown*)stream;

        test_seq = content_handler_test_attributes;
        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, "content test attributes", FALSE);
        IStream_Release(stream);

        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown*)&instream;

        test_seq = read_test_seq;
        read_cnt = 0;
        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(read_cnt == 7, "read_cnt = %d\n", read_cnt);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, "Read call test", FALSE);

        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(carriage_ret_test);

        test_seq = content_handler_test2;
        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, "content test 2", FALSE);

        VariantClear(&var);

        /* from file url */
        file = CreateFileA(testXmlA, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        ok(file != INVALID_HANDLE_VALUE, "Could not create file: %lu\n", GetLastError());
        WriteFile(file, testXML, sizeof(testXML)-1, &written, NULL);
        CloseHandle(file);

        hr = ISAXXMLReader_parseURL(reader, NULL);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = ISAXXMLReader_QueryInterface(reader, &IID_IVBSAXXMLReader, (void **)&vb_reader);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IVBSAXXMLReader_parseURL(vb_reader, NULL);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
        IVBSAXXMLReader_Release(vb_reader);

        test_seq = content_handler_test1;
        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parseURL(reader, L"test.xml");
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, "content test 1: from file url", FALSE);

        /* error handler */
        test_seq = content_handler_testerror;
        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parseURL(reader, L"test.xml");
        ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, "content test error", FALSE);

        /* callback ret values */
        test_seq = content_handler_test_callback_rets;
        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parseURL(reader, L"test.xml");
        ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, "content callback ret values", FALSE);

        DeleteFileA(testXmlA);

        /* parse from IXMLDOMDocument */
        hr = CoCreateInstance(&CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
                &IID_IXMLDOMDocument, (void**)&doc);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        str = SysAllocString(szSimpleXML);
        hr = IXMLDOMDocument_loadXML(doc, str, &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        SysFreeString(str);

        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown*)doc;

        test_seq = content_handler_test2;
        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, "parse from IXMLDOMDocument", FALSE);
        IXMLDOMDocument_Release(doc);

        /* xml:space test */
        test_seq = xmlspaceattr_test;
        set_expected_seq(test_seq);
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = _bstr_(xmlspace_attr);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, "xml:space handling", FALSE);

        /* switch off 'namespaces' feature */
        hr = ISAXXMLReader_putFeature(reader, L"http://xml.org/sax/features/namespaces", VARIANT_FALSE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        stream = create_test_stream(test_attributes, -1);
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown*)stream;

        test_seq = content_handler_test_attributes;
        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, "content test attributes", FALSE);
        IStream_Release(stream);
        hr = ISAXXMLReader_putFeature(reader, L"http://xml.org/sax/features/namespaces", VARIANT_TRUE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* switch off 'namespace-prefixes' feature */
        hr = ISAXXMLReader_putFeature(reader, L"http://xml.org/sax/features/namespace-prefixes", VARIANT_FALSE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        stream = create_test_stream(test_attributes, -1);
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown*)stream;

        test_seq = content_handler_test_attributes_no_prefix;
        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, "content test attributes", FALSE);
        IStream_Release(stream);

        hr = ISAXXMLReader_putFeature(reader, L"http://xml.org/sax/features/namespace-prefixes", VARIANT_TRUE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* attribute normalization */
        stream = create_test_stream(attribute_normalize, -1);
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown*)stream;

        test_seq = attribute_norm;
        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, "attribute value normalization", TRUE);
        IStream_Release(stream);

        resolver = (void*)0xdeadbeef;
        hr = ISAXXMLReader_getEntityResolver(reader, &resolver);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(resolver == NULL, "got %p\n", resolver);

        hr = ISAXXMLReader_putEntityResolver(reader, NULL);
        ok(hr == S_OK || broken(hr == E_FAIL), "Unexpected hr %#lx.\n", hr);

        ISAXXMLReader_Release(reader);
        table++;
    }

    free_bstrs();
}

static void test_saxreader_cdata(void)
{
    const struct msxmlsupported_data_t *table = reader_support_data;
    ISAXXMLReader *reader = NULL;
    char seqname[50];
    IStream *stream;
    VARIANT var;
    HRESULT hr;

    while (table->clsid)
    {
        struct call_entry *test_seq;

        if (!is_clsid_supported(table->clsid, reader_support_data))
        {
            table++;
            continue;
        }

        hr = CoCreateInstance(table->clsid, NULL, CLSCTX_INPROC_SERVER, &IID_ISAXXMLReader, (void **)&reader);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        g_reader = reader;

        msxml_version = 0;

        hr = ISAXXMLReader_putContentHandler(reader, &contentHandler);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXXMLReader_putErrorHandler(reader, &errorHandler);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* CDATA sections */
        init_saxlexicalhandler(&lexicalhandler, S_OK);

        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown *)&lexicalhandler.ISAXLexicalHandler_iface;
        hr = ISAXXMLReader_putProperty(reader, L"http://xml.org/sax/properties/lexical-handler", var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        stream = create_test_stream(test_cdata_xml, -1);
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown *)stream;

        test_seq = cdata_test;
        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        sprintf(seqname, "%s: cdata test", table->name);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, seqname, FALSE);

        IStream_Release(stream);

        /* 2. CDATA sections */
        stream = create_test_stream(test2_cdata_xml, -1);
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown*)stream;

        test_seq = cdata_test2;
        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        sprintf(seqname, "%s: cdata test 2", table->name);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, seqname, FALSE);

        IStream_Release(stream);

        /* 3. CDATA sections */
        stream = create_test_stream(test3_cdata_xml, -1);
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown *)stream;

        test_seq = cdata_test3;
        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        sprintf(seqname, "%s: cdata test 3", table->name);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, seqname, FALSE);

        IStream_Release(stream);

        ISAXXMLReader_Release(reader);
        table++;
    }

    free_bstrs();
}

static void test_saxreader_pi(void)
{
    const struct msxmlsupported_data_t *table = reader_support_data;
    ISAXXMLReader *reader;
    char seqname[50];
    VARIANT var;
    HRESULT hr;

    while (table->clsid)
    {
        struct call_entry *test_seq;

        if (!is_clsid_supported(table->clsid, reader_support_data))
        {
            table++;
            continue;
        }

        hr = CoCreateInstance(table->clsid, NULL, CLSCTX_INPROC_SERVER, &IID_ISAXXMLReader, (void**)&reader);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        g_reader = reader;

        msxml_version = 0;

        hr = ISAXXMLReader_putContentHandler(reader, &contentHandler);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXXMLReader_putErrorHandler(reader, &errorHandler);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* PI */
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown *)create_test_stream(test_pi_xml, -1);

        test_seq = pi_test;
        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        sprintf(seqname, "%s: pi test", table->name);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, seqname, FALSE);
        VariantClear(&var);

        ISAXXMLReader_Release(reader);
        table++;
    }

    free_bstrs();
}

static void test_saxreader_characters(void)
{
    const struct msxmlsupported_data_t *table = reader_support_data;
    ISAXXMLReader *reader;
    char seqname[50];
    VARIANT var;
    HRESULT hr;

    while (table->clsid)
    {
        struct call_entry *test_seq;

        if (!is_clsid_supported(table->clsid, reader_support_data))
        {
            table++;
            continue;
        }

        hr = CoCreateInstance(table->clsid, NULL, CLSCTX_INPROC_SERVER, &IID_ISAXXMLReader, (void**)&reader);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        g_reader = reader;

        msxml_version = 0;

        hr = ISAXXMLReader_putContentHandler(reader, &contentHandler);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXXMLReader_putErrorHandler(reader, &errorHandler);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* Character data. */
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown *)create_test_stream(test_chardata_xml, -1);

        test_seq = chardata_test;
        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        sprintf(seqname, "%s: char data test", table->name);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, seqname, FALSE);
        VariantClear(&var);

        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown *)create_test_stream(test_chardata_xml2, -1);

        test_seq = chardata_test2;
        set_expected_seq(test_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        sprintf(seqname, "%s: char data test", table->name);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, test_seq, seqname, FALSE);
        VariantClear(&var);

        ISAXXMLReader_Release(reader);
        table++;
    }

    free_bstrs();
}

static void test_saxreader_properties(void)
{
    static const struct saxreader_props_test_t
    {
        const WCHAR *prop_name;
        IUnknown *iface;
    }
    props_test_data[] =
    {
        { L"http://xml.org/sax/properties/lexical-handler", (IUnknown *)&lexicalhandler.ISAXLexicalHandler_iface },
        { L"http://xml.org/sax/properties/declaration-handler", (IUnknown *)&declhandler.ISAXDeclHandler_iface },
        { 0 }
    };

    const struct saxreader_props_test_t *ptr = props_test_data;
    IVBSAXXMLReader *vb_reader;
    ISAXXMLReader *reader;
    HRESULT hr;
    VARIANT v;

    hr = CoCreateInstance(&CLSID_SAXXMLReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_ISAXXMLReader, (void**)&reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXXMLReader_getProperty(reader, L"http://xml.org/sax/properties/lexical-handler", NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    while (ptr->prop_name)
    {
        VARIANT varref;
        LONG ref;

        init_saxlexicalhandler(&lexicalhandler, S_OK);
        init_saxdeclhandler(&declhandler, S_OK);

        V_VT(&v) = VT_EMPTY;
        V_UNKNOWN(&v) = (IUnknown*)0xdeadbeef;
        hr = ISAXXMLReader_getProperty(reader, ptr->prop_name, &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(V_VT(&v) == VT_UNKNOWN, "got %d\n", V_VT(&v));
        ok(V_UNKNOWN(&v) == NULL, "got %p\n", V_UNKNOWN(&v));

        /* VT_UNKNOWN */
        V_VT(&v) = VT_UNKNOWN;
        V_UNKNOWN(&v) = ptr->iface;
        ref = get_refcount(ptr->iface);
        hr = ISAXXMLReader_putProperty(reader, ptr->prop_name, v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(ref < get_refcount(ptr->iface), "Unexpected refcount.\n");

        /* VT_DISPATCH */
        V_VT(&v) = VT_DISPATCH;
        V_UNKNOWN(&v) = ptr->iface;
        ref = get_refcount(ptr->iface);
        hr = ISAXXMLReader_putProperty(reader, ptr->prop_name, v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(ref == get_refcount(ptr->iface), "got wrong refcount %ld, expected %ld\n", get_refcount(ptr->iface), ref);

        /* VT_VARIANT|VT_BYREF with VT_UNKNOWN in referenced variant */
        V_VT(&varref) = VT_UNKNOWN;
        V_UNKNOWN(&varref) = ptr->iface;

        V_VT(&v) = VT_VARIANT|VT_BYREF;
        V_VARIANTREF(&v) = &varref;
        ref = get_refcount(ptr->iface);
        hr = ISAXXMLReader_putProperty(reader, ptr->prop_name, v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(ref == get_refcount(ptr->iface), "got wrong refcount %ld, expected %ld\n", get_refcount(ptr->iface), ref);

        /* VT_VARIANT|VT_BYREF with VT_DISPATCH in referenced variant */
        V_VT(&varref) = VT_DISPATCH;
        V_UNKNOWN(&varref) = ptr->iface;

        V_VT(&v) = VT_VARIANT|VT_BYREF;
        V_VARIANTREF(&v) = &varref;
        ref = get_refcount(ptr->iface);
        hr = ISAXXMLReader_putProperty(reader, ptr->prop_name, v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(ref == get_refcount(ptr->iface), "got wrong refcount %ld, expected %ld\n", get_refcount(ptr->iface), ref);

        V_VT(&v) = VT_EMPTY;
        V_UNKNOWN(&v) = (IUnknown*)0xdeadbeef;

        ref = get_refcount(ptr->iface);
        hr = ISAXXMLReader_getProperty(reader, ptr->prop_name, &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(V_VT(&v) == VT_UNKNOWN, "got %d\n", V_VT(&v));
        ok(V_UNKNOWN(&v) == ptr->iface, "got %p\n", V_UNKNOWN(&v));
        ok(ref < get_refcount(ptr->iface), "expected inreased refcount\n");
        VariantClear(&v);

        V_VT(&v) = VT_EMPTY;
        V_UNKNOWN(&v) = (IUnknown*)0xdeadbeef;
        hr = ISAXXMLReader_putProperty(reader, ptr->prop_name, v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        V_VT(&v) = VT_EMPTY;
        V_UNKNOWN(&v) = (IUnknown*)0xdeadbeef;
        hr = ISAXXMLReader_getProperty(reader, ptr->prop_name, &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(V_VT(&v) == VT_UNKNOWN, "got %d\n", V_VT(&v));
        ok(V_UNKNOWN(&v) == NULL, "got %p\n", V_UNKNOWN(&v));

        V_VT(&v) = VT_UNKNOWN;
        V_UNKNOWN(&v) = ptr->iface;
        hr = ISAXXMLReader_putProperty(reader, ptr->prop_name, v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* only VT_EMPTY seems to be valid to reset property */
        V_VT(&v) = VT_I4;
        V_UNKNOWN(&v) = (IUnknown*)0xdeadbeef;
        hr = ISAXXMLReader_putProperty(reader, ptr->prop_name, v);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        V_VT(&v) = VT_EMPTY;
        V_UNKNOWN(&v) = (IUnknown*)0xdeadbeef;
        hr = ISAXXMLReader_getProperty(reader, ptr->prop_name, &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(V_VT(&v) == VT_UNKNOWN, "got %d\n", V_VT(&v));
        ok(V_UNKNOWN(&v) == ptr->iface, "got %p\n", V_UNKNOWN(&v));
        VariantClear(&v);

        V_VT(&v) = VT_UNKNOWN;
        V_UNKNOWN(&v) = NULL;
        hr = ISAXXMLReader_putProperty(reader, ptr->prop_name, v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        V_VT(&v) = VT_EMPTY;
        V_UNKNOWN(&v) = (IUnknown*)0xdeadbeef;
        hr = ISAXXMLReader_getProperty(reader, ptr->prop_name, &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(V_VT(&v) == VT_UNKNOWN, "got %d\n", V_VT(&v));
        ok(V_UNKNOWN(&v) == NULL, "got %p\n", V_UNKNOWN(&v));

        /* block QueryInterface on handler riid */
        V_VT(&v) = VT_UNKNOWN;
        V_UNKNOWN(&v) = ptr->iface;
        hr = ISAXXMLReader_putProperty(reader, ptr->prop_name, v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        init_saxlexicalhandler(&lexicalhandler, E_NOINTERFACE);
        init_saxdeclhandler(&declhandler, E_NOINTERFACE);

        V_VT(&v) = VT_UNKNOWN;
        V_UNKNOWN(&v) = ptr->iface;
        EXPECT_REF(ptr->iface, 1);
        ref = get_refcount(ptr->iface);
        hr = ISAXXMLReader_putProperty(reader, ptr->prop_name, v);
        ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);
        EXPECT_REF(ptr->iface, 1);

        V_VT(&v) = VT_EMPTY;
        V_UNKNOWN(&v) = (IUnknown*)0xdeadbeef;
        hr = ISAXXMLReader_getProperty(reader, ptr->prop_name, &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(V_VT(&v) == VT_UNKNOWN, "got %d\n", V_VT(&v));
        ok(V_UNKNOWN(&v) != NULL, "got %p\n", V_UNKNOWN(&v));

        ptr++;
        free_bstrs();
    }

    ISAXXMLReader_Release(reader);

    /* Getters with VB interface */
    hr = CoCreateInstance(&CLSID_SAXXMLReader, NULL, CLSCTX_INPROC_SERVER, &IID_IVBSAXXMLReader, (void **)&vb_reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&v) = VT_EMPTY;
    V_DISPATCH(&v) = (IDispatch *)0xdeadbeef;
    hr = IVBSAXXMLReader_getProperty(vb_reader, _bstr_("http://xml.org/sax/properties/lexical-handler"), &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_DISPATCH, "Unexpected type %d.\n", V_VT(&v));
    ok(!V_DISPATCH(&v), "Unexpected value %p.\n", V_UNKNOWN(&v));

    V_VT(&v) = VT_EMPTY;
    V_UNKNOWN(&v) = (IUnknown*)0xdeadbeef;
    hr = IVBSAXXMLReader_getProperty(vb_reader, _bstr_("http://xml.org/sax/properties/declaration-handler"), &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_DISPATCH, "Unexpected type %d.\n", V_VT(&v));
    ok(!V_DISPATCH(&v), "Unexpected value %p.\n", V_UNKNOWN(&v));

    IVBSAXXMLReader_Release(vb_reader);

    free_bstrs();
}

static void test_saxreader_max_xml_size(void)
{
    static const char test_text[] =
        "aaaaaaaaaa"
        "aaaaaaaaaa"
        "aaaaaaaaaa"
        "aaaaaaaaaa"
        "aaaaaaaaaa"
        "aaaaaaaaaa"
        "aaaaaaaaaa"
        "aaaaaaaaaa"
        "aaaaaaaaaa"
        "aaaaaaaaaa";
    const struct msxmlsupported_data_t *table = reader_support_data;
    ISAXXMLReader *reader;
    LARGE_INTEGER pos;
    IStream *stream;
    DWORD written;
    VARIANT var;
    HRESULT hr;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IStream_Write(stream, "<a>", 3, &written);
    for (int i = 0; i < 20; ++i)
        IStream_Write(stream, test_text, sizeof(test_text)-1, &written);
    IStream_Write(stream, "</a>", 4, &written);
    pos.QuadPart = 0;
    IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);

    while (table->clsid)
    {
        if (!is_clsid_supported(table->clsid, reader_support_data))
        {
            table++;
            continue;
        }

        winetest_push_context("%s", table->name);

        hr = CoCreateInstance(table->clsid, NULL, CLSCTX_INPROC_SERVER, &IID_ISAXXMLReader, (void **)&reader);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        V_VT(&var) = VT_EMPTY;
        V_I4(&var) = 123;
        hr = ISAXXMLReader_getProperty(reader, L"max-xml-size", &var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(V_VT(&var) == VT_I4, "Unexpected type %d.\n", V_VT(&var));
        ok(!V_I4(&var), "Unexpected value %ld.\n", V_I4(&var));

        V_VT(&var) = VT_R4;
        V_R4(&var) = 10.0;
        hr = ISAXXMLReader_putProperty(reader, L"max-xml-size", var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        V_VT(&var) = VT_EMPTY;
        V_I4(&var) = 0;
        hr = ISAXXMLReader_getProperty(reader, L"max-xml-size", &var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(V_VT(&var) == VT_I4, "Unexpected type %d.\n", V_VT(&var));
        ok(V_I4(&var) == 10, "Unexpected value %ld.\n", V_I4(&var));

        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = _bstr_("abc");
        hr = ISAXXMLReader_putProperty(reader, L"max-xml-size", var);
        ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

        V_VT(&var) = VT_EMPTY;
        V_I4(&var) = 0;
        hr = ISAXXMLReader_getProperty(reader, L"max-xml-size", &var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(V_VT(&var) == VT_I4, "Unexpected type %d.\n", V_VT(&var));
        ok(V_I4(&var) == 10, "Unexpected value %ld.\n", V_I4(&var));

        V_VT(&var) = VT_I4;
        V_I4(&var) = -123;
        hr = ISAXXMLReader_putProperty(reader, L"max-xml-size", var);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        V_VT(&var) = VT_I4;
        V_I4(&var) = 4194303;
        hr = ISAXXMLReader_putProperty(reader, L"max-xml-size", var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        V_VT(&var) = VT_I4;
        V_I4(&var) = 4194304;
        hr = ISAXXMLReader_putProperty(reader, L"max-xml-size", var);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        V_VT(&var) = VT_I4;
        V_I4(&var) = 4194305;
        hr = ISAXXMLReader_putProperty(reader, L"max-xml-size", var);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        /* Limit to 1K */
        V_VT(&var) = VT_I4;
        V_I4(&var) = 1;
        hr = ISAXXMLReader_putProperty(reader, L"max-xml-size", var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        pos.QuadPart = 0;
        IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);

        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown *)stream;
        hr = ISAXXMLReader_parse(reader, var);
        ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

        V_VT(&var) = VT_I4;
        V_I4(&var) = 3;
        hr = ISAXXMLReader_putProperty(reader, L"max-xml-size", var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        pos.QuadPart = 0;
        IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);

        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown *)stream;
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        ISAXXMLReader_Release(reader);
        table++;

        winetest_pop_context();
    }

    free_bstrs();
    IStream_Release(stream);
}

static struct call_entry normalize_line_breaks_text_seq[] =
{
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 4, S_OK, L"", L"e", L"e" },
    { CH_CHARACTERS, 1, 4, S_OK, L"a\n" },
    { CH_CHARACTERS, 2, 1, S_OK, L"b" },
    { CH_ENDELEMENT, 2, 4, S_OK, L"", L"e", L"e" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct call_entry normalize_line_breaks_off_text_seq[] =
{
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 4, S_OK, L"", L"e", L"e" },
    { CH_CHARACTERS, 1, 4, S_OK, L"a\rb" },
    { CH_ENDELEMENT, 2, 4, S_OK, L"", L"e", L"e" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct attribute_entry normalize_line_breaks_attrs[] =
{
    { L"", L"attr", L"attr", L"a b c d" },
    { NULL }
};

static struct call_entry normalize_line_breaks_attr_seq[] =
{
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 3, 5, S_OK, L"", L"e", L"e", normalize_line_breaks_attrs },
    { CH_ENDELEMENT, 3, 7, S_OK, L"", L"e", L"e" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct call_entry normalize_line_breaks_comment_seq[] =
{
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 4, S_OK, L"", L"e", L"e" },
    { LH_COMMENT, 1, 8, S_OK, L" ab\n" },
    { LH_COMMENT, 2, 1, S_OK, L"c " },
    { CH_ENDELEMENT, 2, 8, S_OK, L"", L"e", L"e" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct call_entry normalize_line_breaks_off_comment_seq[] =
{
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 4, S_OK, L"", L"e", L"e" },
    { LH_COMMENT, 1, 8, S_OK, L" ab\rc " },
    { CH_ENDELEMENT, 2, 8, S_OK, L"", L"e", L"e" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct call_entry normalize_line_breaks_cdata_seq[] =
{
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 4, S_OK, L"", L"e", L"e" },
    { LH_STARTCDATA, 1, 13, S_OK },
    { CH_CHARACTERS, 1, 13, S_OK, L" ab\n" },
    { CH_CHARACTERS, 2, 1, S_OK, L"c " },
    { LH_ENDCDATA, 2, 1, S_OK },
    { CH_ENDELEMENT, 2, 8, S_OK, L"", L"e", L"e" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct call_entry normalize_line_breaks_off_cdata_seq[] =
{
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 4, S_OK, L"", L"e", L"e" },
    { LH_STARTCDATA, 1, 13, S_OK },
    { CH_CHARACTERS, 1, 13, S_OK, L" ab\rc " },
    { LH_ENDCDATA, 1, 13, S_OK },
    { CH_ENDELEMENT, 2, 8, S_OK, L"", L"e", L"e" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct call_entry normalize_line_breaks_pi_seq[] =
{
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 4, S_OK, L"", L"e", L"e" },
    { CH_PROCESSINGINSTRUCTION, 1, 9, S_OK, L"pi", L"ab\nc " },
    { CH_ENDELEMENT, 2, 7, S_OK, L"", L"e", L"e" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct call_entry normalize_line_breaks_off_pi_seq[] =
{
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 4, S_OK, L"", L"e", L"e" },
    { CH_PROCESSINGINSTRUCTION, 1, 9, S_OK, L"pi", L"ab\rc \r\nd " },
    { CH_ENDELEMENT, 3, 7, S_OK, L"", L"e", L"e" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static void test_saxreader_normalize_line_breaks(void)
{
    static struct msxmlsupported_data_t classes[] =
    {
        { &CLSID_SAXXMLReader,   "SAXReader"   },
        { &CLSID_SAXXMLReader30, "SAXReader30" },
        { NULL }
    };
    const struct msxmlsupported_data_t *table = classes;
    ISAXXMLReader *reader;
    VARIANT_BOOL v;
    VARIANT var;
    HRESULT hr;

    while (table->clsid)
    {
        if (!is_clsid_supported(table->clsid, reader_support_data))
        {
            table++;
            continue;
        }

        winetest_push_context("%s", table->name);

        hr = CoCreateInstance(table->clsid, NULL, CLSCTX_INPROC_SERVER, &IID_ISAXXMLReader, (void **)&reader);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        g_reader = reader;

        v = VARIANT_FALSE;
        hr = ISAXXMLReader_getFeature(reader, L"normalize-line-breaks", &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(v == VARIANT_TRUE, "Unexpected value %d.\n", v);

        hr = ISAXXMLReader_putContentHandler(reader, &contentHandler);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown *)&lexicalhandler.ISAXLexicalHandler_iface;
        init_saxlexicalhandler(&lexicalhandler, S_OK);
        hr = ISAXXMLReader_putProperty(reader, L"http://xml.org/sax/properties/lexical-handler", var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* Text content */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = _bstr_("<e>a\rb</e>");
        set_expected_seq(normalize_line_breaks_text_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, normalize_line_breaks_text_seq, "Normalize line breaks: text", FALSE);

        /* Attribute values */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = _bstr_("<e attr=\"a\rb\nc\r\nd\" ></e>");
        set_expected_seq(normalize_line_breaks_attr_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, normalize_line_breaks_attr_seq, "Normalize line breaks: attribute", TRUE);

        /* Comments */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = _bstr_("<e><!-- ab\rc --></e>");
        set_expected_seq(normalize_line_breaks_comment_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, normalize_line_breaks_comment_seq, "Normalize line breaks: comment", TRUE);

        /* CDATA */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = _bstr_("<e><![CDATA[ ab\rc ]]></e>");
        set_expected_seq(normalize_line_breaks_cdata_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, normalize_line_breaks_cdata_seq, "Normalize line breaks: cdata", FALSE);

        /* PI */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = _bstr_("<e><?pi ab\rc ?></e>");
        set_expected_seq(normalize_line_breaks_pi_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, normalize_line_breaks_pi_seq, "Normalize line breaks: PI", FALSE);

        /* Disable normalization */
        hr = ISAXXMLReader_putFeature(reader, L"normalize-line-breaks", VARIANT_FALSE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* Text content */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = _bstr_("<e>a\rb</e>");
        set_expected_seq(normalize_line_breaks_off_text_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, normalize_line_breaks_off_text_seq, "Normalize line breaks (off): text", FALSE);

        /* Attribute values */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = _bstr_("<e attr=\"a\rb\nc\r\nd\" ></e>");
        set_expected_seq(normalize_line_breaks_attr_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, normalize_line_breaks_attr_seq, "Normalize line breaks (off): attribute", TRUE);

        /* Comments */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = _bstr_("<e><!-- ab\rc --></e>");
        set_expected_seq(normalize_line_breaks_off_comment_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, normalize_line_breaks_off_comment_seq, "Normalize line breaks (off): comment", FALSE);

        /* CDATA */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = _bstr_("<e><![CDATA[ ab\rc ]]></e>");
        set_expected_seq(normalize_line_breaks_off_cdata_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, normalize_line_breaks_off_cdata_seq, "Normalize line breaks (off): cdata", FALSE);

        /* PI */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = _bstr_("<e><?pi ab\rc \r\nd ?></e>");
        set_expected_seq(normalize_line_breaks_off_pi_seq);
        hr = ISAXXMLReader_parse(reader, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, CONTENT_HANDLER_INDEX, normalize_line_breaks_off_pi_seq, "Normalize line breaks (off): PI", FALSE);

        ISAXXMLReader_Release(reader);
        table++;

        winetest_pop_context();
    }

    free_bstrs();
}

static void test_saxreader_exhaustive_errors(void)
{
    static const GUID *classes[] = { &CLSID_SAXXMLReader, &CLSID_SAXXMLReader30 };
    ISAXXMLReader *reader;
    VARIANT_BOOL v;
    HRESULT hr;

    for (int i = 0; i < ARRAYSIZE(classes); ++i)
    {
        hr = CoCreateInstance(classes[i], NULL, CLSCTX_INPROC_SERVER, &IID_ISAXXMLReader, (void **)&reader);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        v = 123;
        hr = ISAXXMLReader_getFeature(reader, L"exhaustive-errors", &v);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
        ok(v == 123, "Unexpected value %d.\n", v);

        hr = ISAXXMLReader_putFeature(reader, L"exhaustive-errors", VARIANT_TRUE);
        todo_wine
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = ISAXXMLReader_putFeature(reader, L"exhaustive-errors", VARIANT_FALSE);
        todo_wine
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        ISAXXMLReader_Release(reader);
    }
}

struct feature_ns_entry_t {
    const GUID *guid;
    const char *clsid;
    VARIANT_BOOL value;
    VARIANT_BOOL value2; /* feature value after feature set to 0xc */
};

static const struct feature_ns_entry_t feature_ns_entry_data[] = {
    { &CLSID_SAXXMLReader,   "CLSID_SAXXMLReader",   VARIANT_TRUE, VARIANT_FALSE },
    { &CLSID_SAXXMLReader30, "CLSID_SAXXMLReader30", VARIANT_TRUE, VARIANT_FALSE },
    { 0 }
};

static void test_saxreader_features(void)
{
    static const WCHAR *feature_names[] =
    {
        L"http://xml.org/sax/features/namespaces",
        L"http://xml.org/sax/features/namespace-prefixes",
        NULL
    };

    const struct feature_ns_entry_t *entry = feature_ns_entry_data;
    ISAXXMLReader *reader;

    while (entry->guid)
    {
        VARIANT_BOOL value;
        const WCHAR **name;
        HRESULT hr;

        hr = CoCreateInstance(entry->guid, NULL, CLSCTX_INPROC_SERVER, &IID_ISAXXMLReader, (void**)&reader);
        if (hr != S_OK)
        {
            win_skip("can't create %s instance\n", entry->clsid);
            entry++;
            continue;
        }

        value = 123;
        hr = ISAXXMLReader_getFeature(reader, L"schema-validation", &value);
        ok(hr == E_INVALIDARG, "Failed to get feature value, hr %#lx.\n", hr);
        ok(value == 123, "Unexpected value %d.\n", value);

        name = feature_names;
        while (*name)
        {
            value = 0xc;
            hr = ISAXXMLReader_getFeature(reader, *name, &value);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(entry->value == value, "%s: got wrong default value %x, expected %x\n", entry->clsid, value, entry->value);

            value = 0xc;
            hr = ISAXXMLReader_putFeature(reader, *name, value);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            value = 0xd;
            hr = ISAXXMLReader_getFeature(reader, *name, &value);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(entry->value2 == value, "%s: got wrong value %x, expected %x\n", entry->clsid, value, entry->value2);

            hr = ISAXXMLReader_putFeature(reader, *name, VARIANT_FALSE);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            value = 0xd;
            hr = ISAXXMLReader_getFeature(reader, *name, &value);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(value == VARIANT_FALSE, "%s: got wrong value %x, expected VARIANT_FALSE\n", entry->clsid, value);

            hr = ISAXXMLReader_putFeature(reader, *name, VARIANT_TRUE);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            value = 0xd;
            hr = ISAXXMLReader_getFeature(reader, *name, &value);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(value == VARIANT_TRUE, "%s: got wrong value %x, expected VARIANT_TRUE\n", entry->clsid, value);

            name++;
        }

        ISAXXMLReader_Release(reader);

        entry++;
    }
}

/* UTF-8 data with UTF-8 BOM and UTF-16 in prolog */
static const CHAR UTF8BOMTest[] =
"\xEF\xBB\xBF<?xml version = \"1.0\" encoding = \"UTF-16\"?>\n"
"<a></a>\n";

struct enc_test_entry_t
{
    const GUID *guid;
    HRESULT hr;
    BOOL todo;
};

static const struct enc_test_entry_t encoding_test_data[] =
{
    { &CLSID_SAXXMLReader,   0xc00ce56f, TRUE },
    { &CLSID_SAXXMLReader30, 0xc00ce56f, TRUE },
    { 0 }
};

static void create_test_file(const char *name, const void *data, DWORD size)
{
    DWORD written;
    HANDLE file;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Could not create file: %lu\n", GetLastError());
    WriteFile(file, data, size, &written, NULL);
    CloseHandle(file);
}

static const WCHAR _80[] = {0x0080,0};
static struct call_entry xml_iso_8859_1_seq[] =
{
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 48, S_OK, L"", L"a", L"a" },
    { CH_CHARACTERS, 1, 48, S_OK, _80 },
    { CH_ENDELEMENT, 1, 51, S_OK, L"", L"a", L"a" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct call_entry xml_win1252_seq[] =
{
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 50, S_OK, L"", L"a", L"a" },
    { CH_CHARACTERS, 1, 50, S_OK, L"\u00c1\u20ac" },
    { CH_ENDELEMENT, 1, 54, S_OK, L"", L"a", L"a" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct call_entry xml_win1253_seq[] =
{
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 50, S_OK, L"", L"a", L"a" },
    { CH_CHARACTERS, 1, 50, S_OK, L"\u0391" },
    { CH_ENDELEMENT, 1, 53, S_OK, L"", L"a", L"a" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static struct call_entry xml_us_ascii_seq[] =
{
    { CH_PUTDOCUMENTLOCATOR, 0, 0, S_OK },
    { CH_STARTDOCUMENT, 0, 0, S_OK },
    { CH_STARTELEMENT, 1, 46, S_OK, L"", L"a", L"a" },
    { CH_CHARACTERS, 1, 46, S_OK, L"A" },
    { CH_ENDELEMENT, 1, 49, S_OK, L"", L"a", L"a" },
    { CH_ENDDOCUMENT, 0, 0, S_OK },
    { CH_ENDTEST }
};

static void test_saxreader_encoding(void)
{
    static const DWORD ucs4_le_test[] =
    {
        '<','a','>','t','e','x','t','<','/','a','>',
    };

    static const char xml_win936_test[] =
        "<?xml version=\"1.0\" encoding=\"windows-936\" ?><a>text</a>";

    static const char xml_win1252_test[] =
        "<?xml version=\"1.0\" encoding=\"windows-1252\" ?><a>" "\xc1" "\x80" "</a>";

    static const char xml_win1253_test[] =
        "<?xml version=\"1.0\" encoding=\"windows-1253\" ?><a>" "\xc1" "</a>";

    static const char xml_us_ascii_test[] =
        "<?xml version=\"1.0\" encoding=\"us-ascii\" ?><a>" "\xc1" "</a>";

    static const char xml_iso_8859_1_test[] =
        "<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?><a>" "\x80" "</a>";

    const struct enc_test_entry_t *entry = encoding_test_data;
    static const CHAR testXmlA[] = "test.xml";
    DWORD ucs4_be_test[ARRAYSIZE(ucs4_le_test)];
    ISAXXMLReader *reader;
    HRESULT hr;

    for (int i = 0; i < ARRAYSIZE(ucs4_le_test); ++i)
        ucs4_be_test[i] = (ucs4_le_test[i] & 0xff) << 24;

    while (entry->guid)
    {
        VARIANT input;

        hr = CoCreateInstance(entry->guid, NULL, CLSCTX_INPROC_SERVER, &IID_ISAXXMLReader, (void**)&reader);
        if (hr != S_OK)
        {
            entry++;
            continue;
        }

        /* UTF-8 with BOM */
        create_test_file(testXmlA, UTF8BOMTest, sizeof(UTF8BOMTest)-1);

        hr = ISAXXMLReader_parseURL(reader, L"test.xml");
        todo_wine_if(entry->todo)
            ok(hr == entry->hr, "Unexpected hr %#lx, class %s.\n", hr, wine_dbgstr_guid(entry->guid));

        DeleteFileA(testXmlA);

        /* UCS-4 LE */
        create_test_file(testXmlA, ucs4_le_test, sizeof(ucs4_le_test));
        hr = ISAXXMLReader_parseURL(reader, L"test.xml");
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        DeleteFileA(testXmlA);

        /* UCS-4 BE */
        create_test_file(testXmlA, ucs4_be_test, sizeof(ucs4_be_test));
        hr = ISAXXMLReader_parseURL(reader, L"test.xml");
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        DeleteFileA(testXmlA);

        /* try BSTR input with no BOM or '<?xml' instruction */
        V_VT(&input) = VT_BSTR;
        V_BSTR(&input) = _bstr_("<element></element>");
        hr = ISAXXMLReader_parse(reader, input);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        ISAXXMLReader_Release(reader);

        free_bstrs();
        entry++;
    }

    hr = CoCreateInstance(&CLSID_SAXXMLReader30, NULL, CLSCTX_INPROC_SERVER, &IID_ISAXXMLReader, (void **)&reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXXMLReader_putContentHandler(reader, &contentHandler);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    create_test_file(testXmlA, xml_win936_test, sizeof(xml_win936_test) - 1);
    hr = ISAXXMLReader_parseURL(reader, L"test.xml");
    todo_wine
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);
    flush_sequence(sequences, CONTENT_HANDLER_INDEX);
    DeleteFileA(testXmlA);

    create_test_file(testXmlA, xml_win1252_test, sizeof(xml_win1252_test) - 1);
    set_expected_seq(xml_win1252_seq);
    hr = ISAXXMLReader_parseURL(reader, L"test.xml");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, CONTENT_HANDLER_INDEX, xml_win1252_seq, "Content test with windows-1252", FALSE);
    DeleteFileA(testXmlA);

    create_test_file(testXmlA, xml_win1253_test, sizeof(xml_win1253_test) - 1);
    set_expected_seq(xml_win1253_seq);
    hr = ISAXXMLReader_parseURL(reader, L"test.xml");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, CONTENT_HANDLER_INDEX, xml_win1253_seq, "Content test with windows-1253", FALSE);
    DeleteFileA(testXmlA);

    create_test_file(testXmlA, xml_us_ascii_test, sizeof(xml_us_ascii_test) - 1);
    set_expected_seq(xml_us_ascii_seq);
    hr = ISAXXMLReader_parseURL(reader, L"test.xml");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, CONTENT_HANDLER_INDEX, xml_us_ascii_seq, "Content test with us-ascii", FALSE);
    DeleteFileA(testXmlA);

    create_test_file(testXmlA, xml_iso_8859_1_test, sizeof(xml_iso_8859_1_test) - 1);
    set_expected_seq(xml_iso_8859_1_seq);
    hr = ISAXXMLReader_parseURL(reader, L"test.xml");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, CONTENT_HANDLER_INDEX, xml_iso_8859_1_seq, "Content test with iso-8859-1", FALSE);
    DeleteFileA(testXmlA);

    ISAXXMLReader_Release(reader);
}

static void test_mxwriter_handlers(void)
{
    IMXWriter *writer;
    HRESULT hr;
    int i;

    static REFIID riids[] =
    {
        &IID_ISAXContentHandler,
        &IID_ISAXLexicalHandler,
        &IID_ISAXDeclHandler,
        &IID_ISAXDTDHandler,
        &IID_ISAXErrorHandler,
        &IID_IVBSAXDeclHandler,
        &IID_IVBSAXLexicalHandler,
        &IID_IVBSAXContentHandler,
        &IID_IVBSAXDTDHandler,
        &IID_IVBSAXErrorHandler
    };

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(writer, 1);

    for (i = 0; i < ARRAY_SIZE(riids); i++)
    {
        IUnknown *handler;
        IMXWriter *writer2;

        /* handler from IMXWriter */
        hr = IMXWriter_QueryInterface(writer, riids[i], (void**)&handler);
        ok(hr == S_OK, "%s, unexpected hr %#lx.\n", wine_dbgstr_guid(riids[i]), hr);
        EXPECT_REF(writer, 2);
        EXPECT_REF(handler, 2);

        /* IMXWriter from a handler */
        hr = IUnknown_QueryInterface(handler, &IID_IMXWriter, (void**)&writer2);
        ok(hr == S_OK, "%s, unexpected hr %#lx.\n", wine_dbgstr_guid(riids[i]), hr);
        ok(writer2 == writer, "got %p, expected %p\n", writer2, writer);
        EXPECT_REF(writer, 3);
        IMXWriter_Release(writer2);
        IUnknown_Release(handler);
    }

    IMXWriter_Release(writer);
}

static struct msxmlsupported_data_t mxwriter_support_data[] =
{
    { &CLSID_MXXMLWriter,   "MXXMLWriter"   },
    { &CLSID_MXXMLWriter30, "MXXMLWriter30" },
    { NULL }
};

static struct msxmlsupported_data_t mxattributes_support_data[] =
{
    { &CLSID_SAXAttributes,   "SAXAttributes"   },
    { &CLSID_SAXAttributes30, "SAXAttributes30" },
    { NULL }
};

struct mxwriter_props_t
{
    const GUID *clsid;
    VARIANT_BOOL bom;
    VARIANT_BOOL disable_escape;
    VARIANT_BOOL indent;
    VARIANT_BOOL omitdecl;
    VARIANT_BOOL standalone;
    const WCHAR *encoding;
};

static const struct mxwriter_props_t mxwriter_default_props[] =
{
    { &CLSID_MXXMLWriter,   VARIANT_TRUE, VARIANT_FALSE, VARIANT_FALSE, VARIANT_FALSE, VARIANT_FALSE, L"UTF-16" },
    { &CLSID_MXXMLWriter30, VARIANT_TRUE, VARIANT_FALSE, VARIANT_FALSE, VARIANT_FALSE, VARIANT_FALSE, L"UTF-16" },
    { NULL }
};

static void test_mxwriter_default_properties(const struct mxwriter_props_t *table)
{
    int i = 0;

    while (table->clsid)
    {
        IMXWriter *writer;
        VARIANT_BOOL b;
        BSTR encoding;
        HRESULT hr;

        if (!is_clsid_supported(table->clsid, mxwriter_support_data))
        {
            table++;
            i++;
            continue;
        }

        hr = CoCreateInstance(table->clsid, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        b = !table->bom;
        hr = IMXWriter_get_byteOrderMark(writer, &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(table->bom == b, "test %d: got BOM %d, expected %d\n", i, b, table->bom);

        b = !table->disable_escape;
        hr = IMXWriter_get_disableOutputEscaping(writer, &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(table->disable_escape == b, "test %d: got disable escape %d, expected %d\n", i, b,
           table->disable_escape);

        b = !table->indent;
        hr = IMXWriter_get_indent(writer, &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(table->indent == b, "test %d: got indent %d, expected %d\n", i, b, table->indent);

        b = !table->omitdecl;
        hr = IMXWriter_get_omitXMLDeclaration(writer, &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(table->omitdecl == b, "test %d: got omitdecl %d, expected %d\n", i, b, table->omitdecl);

        b = !table->standalone;
        hr = IMXWriter_get_standalone(writer, &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(table->standalone == b, "test %d: got standalone %d, expected %d\n", i, b, table->standalone);

        hr = IMXWriter_get_encoding(writer, &encoding);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(encoding, table->encoding), "test %d: got encoding %s, expected %s\n",
            i, wine_dbgstr_w(encoding), wine_dbgstr_w(table->encoding));
        SysFreeString(encoding);

        IMXWriter_Release(writer);

        table++;
        i++;
    }
}

static void test_mxwriter_properties(void)
{
    ISAXContentHandler *content;
    IMXWriter *writer;
    VARIANT_BOOL b;
    HRESULT hr;
    BSTR str, str2;
    VARIANT dest;

    test_mxwriter_default_properties(mxwriter_default_props);

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_get_disableOutputEscaping(writer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_get_byteOrderMark(writer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_get_indent(writer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_get_omitXMLDeclaration(writer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_get_standalone(writer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /* set and check */
    hr = IMXWriter_put_standalone(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    b = VARIANT_FALSE;
    hr = IMXWriter_get_standalone(writer, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    hr = IMXWriter_get_encoding(writer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /* UTF-16 is a default setting apparently */
    str = (void*)0xdeadbeef;
    hr = IMXWriter_get_encoding(writer, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"UTF-16"), "Unexpected string %s.\n", wine_dbgstr_w(str));

    str2 = (void*)0xdeadbeef;
    hr = IMXWriter_get_encoding(writer, &str2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(str != str2, "expected newly allocated, got same %p\n", str);

    SysFreeString(str2);
    SysFreeString(str);

    /* put empty string */
    str = SysAllocString(L"");
    hr = IMXWriter_put_encoding(writer, str);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    SysFreeString(str);

    str = (void*)0xdeadbeef;
    hr = IMXWriter_get_encoding(writer, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"UTF-16"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* invalid encoding name */
    str = SysAllocString(L"test");
    hr = IMXWriter_put_encoding(writer, str);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    SysFreeString(str);

    /* test case sensitivity */
    hr = IMXWriter_put_encoding(writer, _bstr_("utf-8"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    str = (void*)0xdeadbeef;
    hr = IMXWriter_get_encoding(writer, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"utf-8"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hr = IMXWriter_put_encoding(writer, _bstr_("uTf-16"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    str = (void*)0xdeadbeef;
    hr = IMXWriter_get_encoding(writer, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"uTf-16"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* how it affects document creation */
    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<?xml version=\"1.0\" encoding=\"UTF-16\" standalone=\"yes\"?>\r\n",
        V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);
    ISAXContentHandler_Release(content);

    hr = IMXWriter_get_version(writer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    /* default version is 'surprisingly' 1.0 */
    hr = IMXWriter_get_version(writer, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"1.0"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* store version string as is */
    hr = IMXWriter_put_version(writer, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_version(writer, _bstr_("1.0"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_version(writer, _bstr_(""));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMXWriter_get_version(writer, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L""), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hr = IMXWriter_put_version(writer, _bstr_("a.b"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMXWriter_get_version(writer, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"a.b"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hr = IMXWriter_put_version(writer, _bstr_("2.0"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMXWriter_get_version(writer, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"2.0"), "got %s\n", wine_dbgstr_w(str));
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
    char *buff;
    LONG ref;
    int len;

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(stream, 1);

    /* detach when nothing was attached */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* attach stream */
    V_VT(&dest) = VT_UNKNOWN;
    V_UNKNOWN(&dest) = (IUnknown*)stream;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine EXPECT_REF(stream, 3);

    /* detach setting VT_EMPTY destination */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(stream, 1);

    V_VT(&dest) = VT_UNKNOWN;
    V_UNKNOWN(&dest) = (IUnknown*)stream;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* flush() doesn't detach a stream */
    hr = IMXWriter_flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine EXPECT_REF(stream, 3);

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pos2.QuadPart == 0, "expected stream beginning\n");

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pos2.QuadPart != 0, "expected stream beginning\n");

    /* already started */
    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* flushed on endDocument() */
    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pos2.QuadPart != 0, "expected stream position moved\n");

    IStream_Release(stream);

    /* auto-flush feature */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(stream, 1);

    V_VT(&dest) = VT_UNKNOWN;
    V_UNKNOWN(&dest) = (IUnknown*)stream;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_byteOrderMark(writer, VARIANT_FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, L"a", -1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* internal buffer is flushed automatically on certain threshold */
    pos.QuadPart = 0;
    pos2.QuadPart = 1;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pos2.QuadPart == 0, "expected stream beginning\n");

    len = 2048;
    buff = malloc(len + 1);
    memset(buff, 'A', len);
    buff[len] = 0;
    hr = ISAXContentHandler_characters(content, _bstr_(buff), len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    pos.QuadPart = 0;
    pos2.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pos2.QuadPart != 0, "unexpected stream beginning\n");

    hr = IMXWriter_get_output(writer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    ref = get_refcount(stream);
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_UNKNOWN, "got vt type %d\n", V_VT(&dest));
    ok(V_UNKNOWN(&dest) == (IUnknown*)stream, "got pointer %p\n", V_UNKNOWN(&dest));
    ok(ref+1 == get_refcount(stream), "expected increased refcount\n");
    VariantClear(&dest);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IStream_Release(stream);

    /* test char count lower than threshold */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(stream, 1);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, L"a", -1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    pos.QuadPart = 0;
    pos2.QuadPart = 1;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pos2.QuadPart == 0, "expected stream beginning\n");

    memset(buff, 'A', len);
    buff[len] = 0;
    hr = ISAXContentHandler_characters(content, _bstr_(buff), len - 8);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    pos.QuadPart = 0;
    pos2.QuadPart = 1;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pos2.QuadPart == 0, "expected stream beginning\n");

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* test auto-flush function when stream is not set */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, L"a", -1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(buff, 'A', len);
    buff[len] = 0;
    hr = ISAXContentHandler_characters(content, _bstr_(buff), len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    len += strlen("<a>");
    ok(SysStringLen(V_BSTR(&dest)) == len, "got len=%d, expected %d\n", SysStringLen(V_BSTR(&dest)), len);
    VariantClear(&dest);

    free(buff);
    ISAXContentHandler_Release(content);
    IStream_Release(stream);
    IMXWriter_Release(writer);
    free_bstrs();
}

static void test_mxwriter_startenddocument(void)
{
    ISAXContentHandler *content;
    IMXWriter *writer;
    VARIANT dest;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<?xml version=\"1.0\" encoding=\"UTF-16\" standalone=\"no\"?>\r\n", V_BSTR(&dest)),
        "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* now try another startDocument */
    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* and get duplicated prolog */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<?xml version=\"1.0\" encoding=\"UTF-16\" standalone=\"no\"?>\r\n"
                  "<?xml version=\"1.0\" encoding=\"UTF-16\" standalone=\"no\"?>\r\n", V_BSTR(&dest)),
        "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    /* now with omitted declaration */
    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    free_bstrs();
}

enum startendtype
{
    StartElement    = 0x001,
    EndElement      = 0x010,
    StartEndElement = 0x011,
    DisableEscaping = 0x100
};

struct writer_startendelement_t {
    const GUID *clsid;
    enum startendtype type;
    const WCHAR *uri;
    const WCHAR *local_name;
    const WCHAR *qname;
    const WCHAR *output;
    HRESULT hr;
    ISAXAttributes *attr;
};

static const WCHAR startelement_xml[] = L"<uri:local a:attr1=\"a1\" attr2=\"a2\" attr3=\"&lt;&amp;&quot;&gt;\'\" attr4=\"a\r\nb\r\nc\r\n\">";
static const WCHAR startendelement_xml[] = L"<uri:local a:attr1=\"a1\" attr2=\"a2\" attr3=\"&lt;&amp;&quot;&gt;\'\" attr4=\"a\r\nb\r\nc\r\n\"/>";
static const WCHAR startendelement_noescape_xml[] = L"<uri:local a:attr1=\"a1\" attr2=\"a2\" attr3=\"<&\">\'\" attr4=\"a\r\nb\r\nc\r\n\"/>";

static const struct writer_startendelement_t writer_startendelement[] =
{
    { &CLSID_MXXMLWriter,   StartElement, NULL, NULL, NULL, NULL, E_INVALIDARG },
    { &CLSID_MXXMLWriter30, StartElement, NULL, NULL, NULL, NULL, E_INVALIDARG },

    { &CLSID_MXXMLWriter,   StartElement, L"uri", NULL, NULL, NULL, E_INVALIDARG },
    { &CLSID_MXXMLWriter30, StartElement, L"uri", NULL, NULL, NULL, E_INVALIDARG },

    { &CLSID_MXXMLWriter,   StartElement, NULL, L"local", NULL, NULL, E_INVALIDARG },
    { &CLSID_MXXMLWriter30, StartElement, NULL, L"local", NULL, NULL, E_INVALIDARG },

    { &CLSID_MXXMLWriter,   StartElement, NULL, NULL, L"qname", NULL, E_INVALIDARG },
    { &CLSID_MXXMLWriter30, StartElement, NULL, NULL, L"qname", NULL, E_INVALIDARG },

    { &CLSID_MXXMLWriter,   StartElement, L"uri", L"local", L"qname", L"<qname>", S_OK },
    { &CLSID_MXXMLWriter30, StartElement, L"uri", L"local", L"qname", L"<qname>", S_OK },

    { &CLSID_MXXMLWriter,   StartElement, L"uri", L"local", NULL, NULL, E_INVALIDARG },
    { &CLSID_MXXMLWriter30, StartElement, L"uri", L"local", NULL, NULL, E_INVALIDARG },

    { &CLSID_MXXMLWriter,   StartElement, L"uri", L"local", L"uri:local", L"<uri:local>", S_OK },
    { &CLSID_MXXMLWriter30, StartElement, L"uri", L"local", L"uri:local", L"<uri:local>", S_OK },

    { &CLSID_MXXMLWriter,   StartElement, L"uri", L"local", L"uri:local2", L"<uri:local2>", S_OK },
    { &CLSID_MXXMLWriter30, StartElement, L"uri", L"local", L"uri:local2", L"<uri:local2>", S_OK },

    /* endElement tests */
    { &CLSID_MXXMLWriter,   EndElement, NULL, NULL, NULL, NULL, E_INVALIDARG },
    { &CLSID_MXXMLWriter30, EndElement, NULL, NULL, NULL, NULL, E_INVALIDARG },
    { &CLSID_MXXMLWriter,   EndElement, L"uri", NULL, NULL, NULL, E_INVALIDARG },
    { &CLSID_MXXMLWriter30, EndElement, L"uri", NULL, NULL, NULL, E_INVALIDARG },
    { &CLSID_MXXMLWriter,   EndElement, NULL, L"local", NULL, NULL, E_INVALIDARG },
    { &CLSID_MXXMLWriter30, EndElement, NULL, L"local", NULL, NULL, E_INVALIDARG },
    { &CLSID_MXXMLWriter,   EndElement, NULL, NULL, L"qname", NULL, E_INVALIDARG },
    { &CLSID_MXXMLWriter30, EndElement, NULL, NULL, L"qname", NULL, E_INVALIDARG },
    { &CLSID_MXXMLWriter,   EndElement, L"uri", L"local", L"qname", L"</qname>", S_OK },
    { &CLSID_MXXMLWriter30, EndElement, L"uri", L"local", L"qname", L"</qname>", S_OK },
    { &CLSID_MXXMLWriter,   EndElement, L"uri", L"local", NULL, NULL, E_INVALIDARG },
    { &CLSID_MXXMLWriter30, EndElement, L"uri", L"local", NULL, NULL, E_INVALIDARG },
    { &CLSID_MXXMLWriter,   EndElement, L"uri", L"local", L"uri:local", L"</uri:local>", S_OK },
    { &CLSID_MXXMLWriter30, EndElement, L"uri", L"local", L"uri:local", L"</uri:local>", S_OK },
    { &CLSID_MXXMLWriter,   EndElement, L"uri", L"local", L"uri:local2", L"</uri:local2>", S_OK },
    { &CLSID_MXXMLWriter30, EndElement, L"uri", L"local", L"uri:local2", L"</uri:local2>", S_OK },

    /* with attributes */
    { &CLSID_MXXMLWriter,   StartElement, L"uri", L"local", L"uri:local", startelement_xml, S_OK, &saxattributes },
    { &CLSID_MXXMLWriter30, StartElement, L"uri", L"local", L"uri:local", startelement_xml, S_OK, &saxattributes },
    /* empty elements */
    { &CLSID_MXXMLWriter,   StartEndElement, L"uri", L"local", L"uri:local", startendelement_xml, S_OK, &saxattributes },
    { &CLSID_MXXMLWriter30, StartEndElement, L"uri", L"local", L"uri:local", startendelement_xml, S_OK, &saxattributes },
    { &CLSID_MXXMLWriter,   StartEndElement, L"", L"", L"", L"</>", S_OK },
    { &CLSID_MXXMLWriter30, StartEndElement, L"", L"", L"", L"</>", S_OK },

    /* with disabled output escaping */
    { &CLSID_MXXMLWriter,   StartEndElement | DisableEscaping, L"uri", L"local", L"uri:local", startendelement_noescape_xml, S_OK, &saxattributes },
    { &CLSID_MXXMLWriter30, StartEndElement | DisableEscaping, L"uri", L"local", L"uri:local", startendelement_noescape_xml, S_OK, &saxattributes },

    { NULL }
};

static void get_class_support_data(struct msxmlsupported_data_t *table, REFIID riid)
{
    while (table->clsid)
    {
        IUnknown *unk;
        HRESULT hr;

        hr = CoCreateInstance(table->clsid, NULL, CLSCTX_INPROC_SERVER, riid, (void**)&unk);
        if (hr == S_OK) IUnknown_Release(unk);

        table->supported = hr == S_OK;
        if (hr != S_OK) win_skip("class %s not supported\n", table->name);

        table++;
    }
}

static void test_mxwriter_startendelement_batch(const struct writer_startendelement_t *table)
{
    int i = 0;

    while (table->clsid)
    {
        ISAXContentHandler *content;
        IMXWriter *writer;
        HRESULT hr;

        if (!is_clsid_supported(table->clsid, mxwriter_support_data))
        {
            table++;
            i++;
            continue;
        }

        hr = CoCreateInstance(table->clsid, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXContentHandler_startDocument(content);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        if (table->type & DisableEscaping)
        {
            hr = IMXWriter_put_disableOutputEscaping(writer, VARIANT_TRUE);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        }

        if (table->type & StartElement)
        {
            hr = ISAXContentHandler_startElement(content, table->uri, table->uri ? lstrlenW(table->uri) : 0,
                table->local_name, table->local_name ? lstrlenW(table->local_name) : 0, table->qname,
                table->qname ? lstrlenW(table->qname) : 0, table->attr);
            ok(hr == table->hr, "test %d: got %#lx, expected %#lx\n", i, hr, table->hr);
        }

        if (table->type & EndElement)
        {
            hr = ISAXContentHandler_endElement(content, table->uri, table->uri ? lstrlenW(table->uri) : 0,
                table->local_name, table->local_name ? lstrlenW(table->local_name) : 0, table->qname,
                table->qname ? lstrlenW(table->qname) : 0);
            ok(hr == table->hr, "test %d: got %#lx, expected %#lx\n", i, hr, table->hr);
        }

        /* test output */
        if (hr == S_OK)
        {
            VARIANT dest;

            V_VT(&dest) = VT_EMPTY;
            hr = IMXWriter_get_output(writer, &dest);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
            ok(!lstrcmpW(table->output, V_BSTR(&dest)),
                "test %d: got wrong content %s, expected %s\n", i, wine_dbgstr_w(V_BSTR(&dest)), wine_dbgstr_w(table->output));
            VariantClear(&dest);
        }

        ISAXContentHandler_Release(content);
        IMXWriter_Release(writer);

        table++;
        i++;
    }

    free_bstrs();
}

/* point of these test is to start/end element with different names and name lengths */
struct writer_startendelement2_t {
    const GUID *clsid;
    const WCHAR *qnamestart;
    int qnamestart_len;
    const WCHAR *qnameend;
    int qnameend_len;
    const WCHAR *output;
    HRESULT hr;
};

static const struct writer_startendelement2_t writer_startendelement2[] = {
    { &CLSID_MXXMLWriter,   L"a", -1, L"b", -1, L"<a/>", S_OK },
    { &CLSID_MXXMLWriter30, L"a", -1, L"b", -1, L"<a/>", S_OK },

    { &CLSID_MXXMLWriter,   L"a", 1, L"b", 1, L"<a/>", S_OK },
    { &CLSID_MXXMLWriter30, L"a", 1, L"b", 1, L"<a/>", S_OK },
    { NULL }
};

static void test_mxwriter_startendelement_batch2(const struct writer_startendelement2_t *table)
{
    int i = 0;

    while (table->clsid)
    {
        ISAXContentHandler *content;
        IMXWriter *writer;
        HRESULT hr;

        if (!is_clsid_supported(table->clsid, mxwriter_support_data))
        {
            table++;
            i++;
            continue;
        }

        hr = CoCreateInstance(table->clsid, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXContentHandler_startDocument(content);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, table->qnamestart, table->qnamestart_len, NULL);
        ok(hr == table->hr, "test %d: got %#lx, expected %#lx\n", i, hr, table->hr);

        hr = ISAXContentHandler_endElement(content, L"", 0, L"", 0, table->qnameend, table->qnameend_len);
        ok(hr == table->hr, "test %d: got %#lx, expected %#lx\n", i, hr, table->hr);

        /* test output */
        if (hr == S_OK)
        {
            VARIANT dest;

            V_VT(&dest) = VT_EMPTY;
            hr = IMXWriter_get_output(writer, &dest);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
            ok(!lstrcmpW(table->output, V_BSTR(&dest)),
                "test %d: got wrong content %s, expected %s\n", i, wine_dbgstr_w(V_BSTR(&dest)), wine_dbgstr_w(table->output));
            VariantClear(&dest);
        }

        ISAXContentHandler_Release(content);
        IMXWriter_Release(writer);

        table++;
        i++;

        free_bstrs();
    }
}


static void test_mxwriter_startendelement(void)
{
    ISAXContentHandler *content;
    IVBSAXContentHandler *vb_content;
    IMXWriter *writer;
    VARIANT dest;
    BSTR bstr_null = NULL, bstr_empty, bstr_a, bstr_b, bstr_ab;
    HRESULT hr;

    test_mxwriter_startendelement_batch(writer_startendelement);
    test_mxwriter_startendelement_batch2(writer_startendelement2);

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_IVBSAXContentHandler, (void**)&vb_content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXContentHandler_startDocument(vb_content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    bstr_empty = SysAllocString(L"");
    bstr_a = SysAllocString(L"a");
    bstr_b = SysAllocString(L"b");
    bstr_ab = SysAllocString(L"a:b");

    hr = IVBSAXContentHandler_startElement(vb_content, &bstr_null, &bstr_empty, &bstr_b, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXContentHandler_startElement(vb_content, &bstr_empty, &bstr_b, &bstr_empty, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = IVBSAXContentHandler_startElement(vb_content, &bstr_empty, &bstr_empty, &bstr_b, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<><b>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = IVBSAXContentHandler_endElement(vb_content, &bstr_null, &bstr_null, &bstr_b);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXContentHandler_endElement(vb_content, &bstr_null, &bstr_a, &bstr_b);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXContentHandler_endElement(vb_content, &bstr_a, &bstr_b, &bstr_null);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXContentHandler_endElement(vb_content, &bstr_empty, &bstr_null, &bstr_b);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXContentHandler_endElement(vb_content, &bstr_empty, &bstr_b, &bstr_null);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXContentHandler_endElement(vb_content, &bstr_empty, &bstr_empty, &bstr_b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<><b></b>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    SysFreeString(bstr_empty);
    SysFreeString(bstr_a);
    SysFreeString(bstr_b);
    SysFreeString(bstr_ab);

    hr = IVBSAXContentHandler_endDocument(vb_content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IVBSAXContentHandler_Release(vb_content);
    IMXWriter_Release(writer);

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* all string pointers should be not null */
    hr = ISAXContentHandler_startElement(content, L"", 0, L"b", 1, L"", 0, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, L"b", 1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<><b>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXContentHandler_endElement(content, NULL, 0, NULL, 0, L"a:b", 3);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endElement(content, NULL, 0, L"b", 1, L"a:b", 3);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* only local name is an error too */
    hr = ISAXContentHandler_endElement(content, NULL, 0, L"b", 1, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endElement(content, L"", 0, L"", 0, L"b", 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<><b></b>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, L"abcdef", 3, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<abc>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMXWriter_flush(writer);

    hr = ISAXContentHandler_endElement(content, L"", 0, L"", 0, L"abdcdef", 3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<abc></abd>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* length -1 */
    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, L"a", -1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<a>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);
    free_bstrs();
}

struct writer_characters_t {
    const GUID *clsid;
    const WCHAR *data;
    const WCHAR *output;
};

static const struct writer_characters_t writer_characters[] =
{
    { &CLSID_MXXMLWriter,   L"< > & \" \'", L"&lt; &gt; &amp; \" \'" },
    { &CLSID_MXXMLWriter30, L"< > & \" \'", L"&lt; &gt; &amp; \" \'" },
    { NULL }
};

static void test_mxwriter_characters(void)
{
    static const WCHAR embedded_nullbytes[] = L"a\0b\0\0\0c";
    const struct writer_characters_t *table = writer_characters;
    IVBSAXContentHandler *vb_content;
    ISAXContentHandler *content;
    IMXWriter *writer;
    VARIANT dest;
    BSTR str;
    HRESULT hr;
    int i = 0;

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_IVBSAXContentHandler, (void**)&vb_content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_characters(content, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_characters(content, L"TESTCHARDATA .", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    str = _bstr_("VbChars");
    hr = IVBSAXContentHandler_characters(vb_content, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_characters(content, L"TESTCHARDATA .", 14);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"VbCharsTESTCHARDATA .", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ISAXContentHandler_Release(content);
    IVBSAXContentHandler_Release(vb_content);
    IMXWriter_Release(writer);

    /* try empty characters data to see if element is closed */
    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, L"a", 1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_characters(content, L"TESTCHARDATA .", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endElement(content, L"", 0, L"", 0, L"a", 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<a></a>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    /* test embedded null bytes */
    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_characters(content, embedded_nullbytes, ARRAY_SIZE(embedded_nullbytes));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(SysStringLen(V_BSTR(&dest)) == ARRAY_SIZE(embedded_nullbytes), "unexpected len %d\n", SysStringLen(V_BSTR(&dest)));
    ok(!memcmp(V_BSTR(&dest), embedded_nullbytes, ARRAY_SIZE(embedded_nullbytes)),
       "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_IVBSAXContentHandler, (void**)&vb_content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXContentHandler_startDocument(vb_content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    str = SysAllocStringLen(embedded_nullbytes, ARRAY_SIZE(embedded_nullbytes));
    hr = IVBSAXContentHandler_characters(vb_content, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    SysFreeString(str);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(SysStringLen(V_BSTR(&dest)) == ARRAY_SIZE(embedded_nullbytes), "unexpected len %d\n", SysStringLen(V_BSTR(&dest)));
    ok(!memcmp(V_BSTR(&dest), embedded_nullbytes, ARRAY_SIZE(embedded_nullbytes)),
       "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    IVBSAXContentHandler_Release(vb_content);
    IMXWriter_Release(writer);

    /* Newlines in consecutive calls */
    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void **)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_characters(content, L"ab\r", 3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_characters(content, L"\ncd", 3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "Unexpected type %d.\n", V_VT(&dest));
    ok(!lstrcmpW(L"ab\r\n\r\ncd", V_BSTR(&dest)), "Unexpected content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_characters(content, L"\nab\rc\r\n", 7);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "Unexpected type %d.\n", V_VT(&dest));
    ok(!lstrcmpW(L"\r\nab\r\nc\r\n", V_BSTR(&dest)), "Unexpected content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    while (table->clsid)
    {
        if (!is_clsid_supported(table->clsid, mxwriter_support_data))
        {
            table++;
            i++;
            continue;
        }

        hr = CoCreateInstance(table->clsid, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXContentHandler_startDocument(content);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXContentHandler_characters(content, table->data, lstrlenW(table->data));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* test output */
        if (hr == S_OK)
        {
            V_VT(&dest) = VT_EMPTY;
            hr = IMXWriter_get_output(writer, &dest);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
            ok(!lstrcmpW(table->output, V_BSTR(&dest)),
                "test %d: got wrong content %s, expected %s\n", i, wine_dbgstr_w(V_BSTR(&dest)), wine_dbgstr_w(table->output));
            VariantClear(&dest);
        }

        /* with disabled escaping */
        V_VT(&dest) = VT_EMPTY;
        hr = IMXWriter_put_output(writer, dest);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXWriter_put_disableOutputEscaping(writer, VARIANT_TRUE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXContentHandler_characters(content, table->data, lstrlenW(table->data));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* test output */
        if (hr == S_OK)
        {
            V_VT(&dest) = VT_EMPTY;
            hr = IMXWriter_get_output(writer, &dest);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
            ok(!lstrcmpW(table->data, V_BSTR(&dest)),
                "test %d: got wrong content %s, expected %s\n", i, wine_dbgstr_w(V_BSTR(&dest)), wine_dbgstr_w(table->data));
            VariantClear(&dest);
        }

        ISAXContentHandler_Release(content);
        IMXWriter_Release(writer);

        table++;
        i++;
    }

    free_bstrs();
}

static const mxwriter_stream_test mxwriter_stream_tests[] = {
    {
        VARIANT_TRUE,"UTF-16",
        {
            {FALSE,(const BYTE*)szUtf16BOM,sizeof(szUtf16BOM),TRUE},
            {FALSE,(const BYTE*)szUtf16XML,sizeof(szUtf16XML)-sizeof(WCHAR)},
            {TRUE}
        }
    },
    {
        VARIANT_FALSE,"UTF-16",
        {
            {FALSE,(const BYTE*)szUtf16XML,sizeof(szUtf16XML)-sizeof(WCHAR)},
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
        VARIANT_TRUE,"utf-8",
        {
            {FALSE,(const BYTE*)utf8xml2,sizeof(utf8xml2)-1},
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
            {FALSE,(const BYTE*)szUtf16XML,sizeof(szUtf16XML)-sizeof(WCHAR)},
            {TRUE}
        }
    },
    {
        VARIANT_TRUE,"UTF-16",
        {
            {FALSE,(const BYTE*)szUtf16BOM,sizeof(szUtf16BOM),TRUE,TRUE},
            {FALSE,(const BYTE*)szUtf16XML,sizeof(szUtf16XML)-sizeof(WCHAR)},
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
    DWORD test_count = ARRAY_SIZE(mxwriter_stream_tests);

    for(current_stream_test_index = 0; current_stream_test_index < test_count; ++current_stream_test_index) {
        const mxwriter_stream_test *test = mxwriter_stream_tests+current_stream_test_index;

        hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
                &IID_IMXWriter, (void**)&writer);
        ok(hr == S_OK, "Unexpected hr %#lx on test %d\n", hr, current_stream_test_index);

        hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
        ok(hr == S_OK, "Unexpected hr %#lx on test %d\n", hr, current_stream_test_index);

        hr = IMXWriter_put_encoding(writer, _bstr_(test->encoding));
        ok(hr == S_OK, "Unexpected hr %#lx on test %d\n", hr, current_stream_test_index);

        V_VT(&dest) = VT_UNKNOWN;
        V_UNKNOWN(&dest) = (IUnknown*)&mxstream;
        hr = IMXWriter_put_output(writer, dest);
        ok(hr == S_OK, "Unexpected hr %#lx on test %d\n", hr, current_stream_test_index);

        hr = IMXWriter_put_byteOrderMark(writer, test->bom);
        ok(hr == S_OK, "Unexpected hr %#lx on test %d\n", hr, current_stream_test_index);

        current_write_test = test->expected_writes;

        hr = ISAXContentHandler_startDocument(content);
        ok(hr == S_OK, "Unexpected hr %#lx on test %d\n", hr, current_stream_test_index);

        hr = ISAXContentHandler_endDocument(content);
        ok(hr == S_OK, "Unexpected hr %#lx on test %d\n", hr, current_stream_test_index);

        ISAXContentHandler_Release(content);
        IMXWriter_Release(writer);

        ok(current_write_test->last, "The last %d write calls on test %d were missed\n",
            (int)(current_write_test-test->expected_writes), current_stream_test_index);
    }

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_encoding(writer, _bstr_("UTF-8"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_UNKNOWN;
    V_UNKNOWN(&dest) = (IUnknown*)stream;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Setting output of the mxwriter causes the current output to be flushed,
     * and the writer to start over.
     */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pos2.QuadPart != 0, "expected stream position moved\n");

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "Expected VT_BSTR, got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<?xml version=\"1.0\" encoding=\"UTF-16\" standalone=\"no\"?>\r\n", V_BSTR(&dest)),
            "Got wrong content: %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* test when BOM is written to output stream */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_UNKNOWN;
    V_UNKNOWN(&dest) = (IUnknown*)stream;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_byteOrderMark(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_encoding(writer, _bstr_("UTF-16"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    pos.QuadPart = 0;
    pos2.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pos2.QuadPart == 2, "got wrong position\n");

    IStream_Release(stream);
    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    free_bstrs();
}

static const char *encoding_names[] = {
    "iso-8859-1",
    "iso-8859-2",
    "iso-8859-3",
    "iso-8859-4",
    "iso-8859-5",
    "iso-8859-7",
    "iso-8859-9",
    "iso-8859-13",
    "iso-8859-15",
    NULL
};

static void test_mxwriter_encoding(void)
{
    ISAXContentHandler *content;
    IMXWriter *writer;
    IStream *stream;
    const char *enc;
    VARIANT dest;
    HRESULT hr;
    HGLOBAL g;
    char *ptr;
    BSTR s;
    int i;

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_encoding(writer, _bstr_("UTF-8"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* The content is always re-encoded to UTF-16 when the output is
     * retrieved as a BSTR.
     */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "Expected VT_BSTR, got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<?xml version=\"1.0\" encoding=\"UTF-16\" standalone=\"no\"?>\r\n", V_BSTR(&dest)),
            "got wrong content: %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* switch encoding when something is written already */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_UNKNOWN;
    V_UNKNOWN(&dest) = (IUnknown*)stream;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_encoding(writer, _bstr_("UTF-8"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* write empty element */
    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, L"a", 1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endElement(content, L"", 0, L"", 0, L"a", 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* switch */
    hr = IMXWriter_put_encoding(writer, _bstr_("UTF-16"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = GetHGlobalFromStream(stream, &g);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ptr = GlobalLock(g);
    ok(!strncmp(ptr, "<a/>", 4), "got %c%c%c%c\n", ptr[0],ptr[1],ptr[2],ptr[3]);
    GlobalUnlock(g);

    /* so output is unaffected, encoding name is stored however */
    hr = IMXWriter_get_encoding(writer, &s);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(s, L"UTF-16"), "got %s\n", wine_dbgstr_w(s));
    SysFreeString(s);

    IStream_Release(stream);

    i = 0;
    enc = encoding_names[i];
    while (enc)
    {
        char expectedA[200];

        hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        V_VT(&dest) = VT_UNKNOWN;
        V_UNKNOWN(&dest) = (IUnknown*)stream;
        hr = IMXWriter_put_output(writer, dest);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXWriter_put_encoding(writer, _bstr_(enc));
        ok(hr == S_OK || broken(hr != S_OK) /* old win versions do not support certain encodings */,
            "%s: encoding not accepted\n", enc);
        if (hr != S_OK)
        {
            enc = encoding_names[++i];
            IStream_Release(stream);
            continue;
        }

        hr = ISAXContentHandler_startDocument(content);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXContentHandler_endDocument(content);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXWriter_flush(writer);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* prepare expected string */
        *expectedA = 0;
        strcat(expectedA, "<?xml version=\"1.0\" encoding=\"");
        strcat(expectedA, enc);
        strcat(expectedA, "\" standalone=\"no\"?>\r\n");

        hr = GetHGlobalFromStream(stream, &g);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        ptr = GlobalLock(g);
        ok(!strncmp(ptr, expectedA, strlen(expectedA)), "%s: got %s, expected %.50s\n", enc, ptr, expectedA);
        GlobalUnlock(g);

        V_VT(&dest) = VT_EMPTY;
        hr = IMXWriter_put_output(writer, dest);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        IStream_Release(stream);

        enc = encoding_names[++i];
    }

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    free_bstrs();
}

static void test_obj_dispex(IUnknown *obj)
{
    DISPID dispid = DISPID_SAX_XMLREADER_GETFEATURE;
    IDispatchEx *dispex;
    IUnknown *unk;
    DWORD props;
    UINT ticnt;
    HRESULT hr;
    BSTR name;
    DISPID did;

    hr = IUnknown_QueryInterface(obj, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr)) return;

    ticnt = 0;
    hr = IDispatchEx_GetTypeInfoCount(dispex, &ticnt);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ticnt == 1, "ticnt=%u\n", ticnt);

    name = SysAllocString(L"*");
    hr = IDispatchEx_DeleteMemberByName(dispex, name, fdexNameCaseSensitive);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    SysFreeString(name);

    hr = IDispatchEx_DeleteMemberByDispID(dispex, dispid);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    props = 0;
    hr = IDispatchEx_GetMemberProperties(dispex, dispid, grfdexPropCanAll, &props);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    ok(props == 0, "Unexpected value %ld.\n", props);

    hr = IDispatchEx_GetMemberName(dispex, dispid, &name);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr)) SysFreeString(name);

    hr = IDispatchEx_GetNextDispID(dispex, fdexEnumDefault, DISPID_SAX_XMLREADER_GETFEATURE, &dispid);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    unk = (IUnknown*)0xdeadbeef;
    hr = IDispatchEx_GetNameSpaceParent(dispex, &unk);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    ok(unk == (IUnknown*)0xdeadbeef, "got %p\n", unk);

    name = SysAllocString(L"testprop");
    hr = IDispatchEx_GetDispID(dispex, name, fdexNameEnsure, &did);
    ok(hr == DISP_E_UNKNOWNNAME, "Unexpected hr %#lx.\n", hr);
    SysFreeString(name);

    IDispatchEx_Release(dispex);
}

static void test_saxreader_dispex(void)
{
    IVBSAXXMLReader *vbreader;
    ISAXXMLReader *reader;
    DISPPARAMS dispparams;
    DISPID dispid;
    IUnknown *unk;
    VARIANT arg;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SAXXMLReader, NULL, CLSCTX_INPROC_SERVER,
                &IID_ISAXXMLReader, (void**)&reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(reader, &IID_ISAXXMLReader, TRUE);
    check_interface(reader, &IID_IVBSAXXMLReader, TRUE);
    check_interface(reader, &IID_IDispatch, TRUE);
    check_interface(reader, &IID_IDispatchEx, TRUE);

    hr = ISAXXMLReader_QueryInterface(reader, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    test_obj_dispex(unk);
    IUnknown_Release(unk);

    hr = ISAXXMLReader_QueryInterface(reader, &IID_IVBSAXXMLReader, (void**)&vbreader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IVBSAXXMLReader_QueryInterface(vbreader, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    test_obj_dispex(unk);
    IUnknown_Release(unk);

    dispid = DISPID_PROPERTYPUT;
    dispparams.cArgs = 1;
    dispparams.cNamedArgs = 1;
    dispparams.rgdispidNamedArgs = &dispid;
    dispparams.rgvarg = &arg;

    V_VT(&arg) = VT_DISPATCH;
    V_DISPATCH(&arg) = NULL;

    /* propputref is callable as PROPERTYPUT and PROPERTYPUTREF */
    hr = IVBSAXXMLReader_Invoke(vbreader,
        DISPID_SAX_XMLREADER_CONTENTHANDLER,
       &IID_NULL,
        0,
        DISPATCH_PROPERTYPUT,
       &dispparams,
        NULL,
        NULL,
        NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXXMLReader_Invoke(vbreader,
        DISPID_SAX_XMLREADER_CONTENTHANDLER,
       &IID_NULL,
        0,
        DISPATCH_PROPERTYPUTREF,
       &dispparams,
        NULL,
        NULL,
        NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IVBSAXXMLReader_Release(vbreader);
    ISAXXMLReader_Release(reader);
}

static void test_mxwriter_dispex(void)
{
    IDispatchEx *dispex;
    IMXWriter *writer;
    IUnknown *unk;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDispatchEx_QueryInterface(dispex, &IID_IUnknown, (void**)&unk);
    test_obj_dispex(unk);
    IUnknown_Release(unk);
    IDispatchEx_Release(dispex);
    IMXWriter_Release(writer);
}

static void test_mxwriter_comment(void)
{
    IVBSAXLexicalHandler *vblexical;
    ISAXContentHandler *content;
    ISAXLexicalHandler *lexical;
    IMXWriter *writer;
    VARIANT dest;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXLexicalHandler, (void**)&lexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_IVBSAXLexicalHandler, (void**)&vblexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_comment(lexical, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXLexicalHandler_comment(vblexical, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_comment(lexical, L"comment", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<!---->\r\n", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXLexicalHandler_comment(lexical, L"comment", 7);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<!---->\r\n<!--comment-->\r\n", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* As an element child */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, _bstr_("a"), -1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = ISAXLexicalHandler_comment(lexical, L"comment", 7);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = ISAXContentHandler_endElement(content, L"", 0, L"", 0, _bstr_("a"), -1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "Unexpected type %d.\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<a><!--comment--></a>", V_BSTR(&dest)), "Unexpected content %s.\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    ISAXLexicalHandler_Release(lexical);
    IVBSAXLexicalHandler_Release(vblexical);
    IMXWriter_Release(writer);
    free_bstrs();
}

static void test_mxwriter_cdata(void)
{
    IVBSAXLexicalHandler *vblexical;
    ISAXContentHandler *content;
    ISAXLexicalHandler *lexical;
    IMXWriter *writer;
    VARIANT dest;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXLexicalHandler, (void**)&lexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_IVBSAXLexicalHandler, (void**)&vblexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_startCDATA(lexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<![CDATA[", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = IVBSAXLexicalHandler_startCDATA(vblexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* all these are escaped for text nodes */
    hr = ISAXContentHandler_characters(content, L"< > & \"", 7);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_characters(content, L"\na\rb\r\n", 6);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_endCDATA(lexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<![CDATA[<![CDATA[< > & \"\r\na\r\nb\r\n]]>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    ISAXLexicalHandler_Release(lexical);
    IVBSAXLexicalHandler_Release(vblexical);
    IMXWriter_Release(writer);
    free_bstrs();
}

static void test_mxwriter_pi(void)
{
    ISAXContentHandler *content;
    IMXWriter *writer;
    VARIANT dest;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_processingInstruction(content, NULL, 0, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_processingInstruction(content, L"target", 0, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_processingInstruction(content, L"target", 6, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<?\?>\r\n<?target?>\r\n", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXContentHandler_processingInstruction(content, L"target", 4, L"data", 4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<?\?>\r\n<?target?>\r\n<?targ data?>\r\n", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_processingInstruction(content, L"target", 6, L"data", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<?target?>\r\n", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);


    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);
}

static void test_mxwriter_ignorablespaces(void)
{
    ISAXContentHandler *content;
    IMXWriter *writer;
    VARIANT dest;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_ignorableWhitespace(content, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_ignorableWhitespace(content, L"data", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_ignorableWhitespace(content, L"data", 4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_ignorableWhitespace(content, L"data", 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"datad", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);
}

static void test_mxwriter_dtd(void)
{
    IVBSAXLexicalHandler *vblexical;
    ISAXContentHandler *content;
    ISAXLexicalHandler *lexical;
    IVBSAXDeclHandler *vbdecl;
    ISAXDeclHandler *decl;
    ISAXDTDHandler *dtd;
    IMXWriter *writer;
    VARIANT dest;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXLexicalHandler, (void**)&lexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXDeclHandler, (void**)&decl);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_IVBSAXDeclHandler, (void**)&vbdecl);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_IVBSAXLexicalHandler, (void**)&vblexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_startDTD(lexical, NULL, 0, NULL, 0, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXLexicalHandler_startDTD(vblexical, NULL, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_startDTD(lexical, NULL, 0, L"pub", 3, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_startDTD(lexical, NULL, 0, NULL, 0, L"sys", 3);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_startDTD(lexical, NULL, 0, L"pub", 3, L"sys", 3);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_startDTD(lexical, L"name", 4, NULL, 0, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<!DOCTYPE name [\r\n", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* system id is required if public is present */
    hr = ISAXLexicalHandler_startDTD(lexical, L"name", 4, L"pub", 3, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_startDTD(lexical, L"name", 4, L"pub", 3, L"sys", 3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<!DOCTYPE name [\r\n<!DOCTYPE name PUBLIC \"pub\""
        "<!DOCTYPE name PUBLIC \"pub\" \"sys\" [\r\n", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXLexicalHandler_endDTD(lexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXLexicalHandler_endDTD(vblexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<!DOCTYPE name [\r\n<!DOCTYPE name PUBLIC \"pub\""
         "<!DOCTYPE name PUBLIC \"pub\" \"sys\" [\r\n]>\r\n]>\r\n",
        V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* element declaration */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_elementDecl(decl, NULL, 0, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXDeclHandler_elementDecl(vbdecl, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_elementDecl(decl, L"name", 4, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_elementDecl(decl, L"name", 4, L"content", 7);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<!ELEMENT name content>\r\n",
        V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_elementDecl(decl, L"name", 4, L"content", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<!ELEMENT name >\r\n",
        V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* attribute declaration */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_attributeDecl(decl, L"element", 7, L"attribute", 9, L"CDATA", 5,
            L"#REQUIRED", 9, L"value", 5);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<!ATTLIST element attribute CDATA #REQUIRED \"value\">\r\n",
        V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXDeclHandler_attributeDecl(decl, L"element", 7, L"attribute2", 10, L"CDATA", 5,
            L"#REQUIRED", 9, L"value2", 6);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_attributeDecl(decl, L"element2", 8, L"attribute3", 10, L"CDATA", 5,
            L"#REQUIRED", 9, L"value3", 6);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<!ATTLIST element attribute CDATA #REQUIRED \"value\">\r\n"
                        "<!ATTLIST element attribute2 CDATA #REQUIRED \"value2\">\r\n"
                        "<!ATTLIST element2 attribute3 CDATA #REQUIRED \"value3\">\r\n",
        V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* internal entities */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_internalEntityDecl(decl, NULL, 0, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXDeclHandler_internalEntityDecl(vbdecl, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_internalEntityDecl(decl, L"name", -1, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_internalEntityDecl(decl, L"name", 4, L"value", 5);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<!ENTITY name \"value\">\r\n", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* external entities */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_externalEntityDecl(decl, NULL, 0, NULL, 0, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXDeclHandler_externalEntityDecl(vbdecl, NULL, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_externalEntityDecl(decl, L"name", 0, NULL, 0, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_externalEntityDecl(decl, L"name", -1, NULL, 0, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_externalEntityDecl(decl, L"name", 4, L"pubid", 5, L"sysid", 5);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_externalEntityDecl(decl, L"name", 4, NULL, 0, L"sysid", 5);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_externalEntityDecl(decl, L"name", 4, L"pubid", 5, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<!ENTITY name PUBLIC \"pubid\" \"sysid\">\r\n<!ENTITY name SYSTEM \"sysid\">\r\n",
        V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));

    VariantClear(&dest);

    /* notation declaration */
    hr = IMXWriter_QueryInterface(writer, &IID_ISAXDTDHandler, (void**)&dtd);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDTDHandler_notationDecl(dtd, NULL, 0, NULL, 0, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDTDHandler_notationDecl(dtd, L"name", 4, NULL, 0, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDTDHandler_notationDecl(dtd, L"name", 4, L"pubid", 5, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDTDHandler_notationDecl(dtd, L"name", 4, L"pubid", 5, L"sysid", 5);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDTDHandler_notationDecl(dtd, L"name", 4, NULL, 0, L"sysid", 5);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(
        L"<!NOTATION name"
        "<!NOTATION name PUBLIC \"pubid\">\r\n"
        "<!NOTATION name PUBLIC \"pubid\" \"sysid\">\r\n"
        "<!NOTATION name SYSTEM \"sysid\">\r\n",
        V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));

    VariantClear(&dest);

    ISAXDTDHandler_Release(dtd);

    ISAXContentHandler_Release(content);
    ISAXLexicalHandler_Release(lexical);
    IVBSAXLexicalHandler_Release(vblexical);
    IVBSAXDeclHandler_Release(vbdecl);
    ISAXDeclHandler_Release(decl);
    IMXWriter_Release(writer);
    free_bstrs();
}

typedef struct {
    const CLSID *clsid;
    const char *uri;
    const char *local;
    const char *qname;
    const char *type;
    const char *value;
    HRESULT hr;
} addattribute_test_t;

static const addattribute_test_t addattribute_data[] = {
    { &CLSID_SAXAttributes,   NULL, NULL, "ns:qname", NULL, "value", E_INVALIDARG },
    { &CLSID_SAXAttributes30, NULL, NULL, "ns:qname", NULL, "value", E_INVALIDARG },

    { &CLSID_SAXAttributes,   NULL, "qname", "ns:qname", NULL, "value", E_INVALIDARG },
    { &CLSID_SAXAttributes30, NULL, "qname", "ns:qname", NULL, "value", E_INVALIDARG },

    { &CLSID_SAXAttributes,   "uri", "qname", "ns:qname", NULL, "value", E_INVALIDARG },
    { &CLSID_SAXAttributes30, "uri", "qname", "ns:qname", NULL, "value", E_INVALIDARG },

    { &CLSID_SAXAttributes,   "uri", "qname", "ns:qname", "type", "value", S_OK },
    { &CLSID_SAXAttributes30, "uri", "qname", "ns:qname", "type", "value", S_OK },

    { NULL }
};

static void test_mxattr_addAttribute(void)
{
    const addattribute_test_t *table = addattribute_data;
    int i = 0;

    while (table->clsid)
    {
        ISAXAttributes *saxattr;
        IMXAttributes *mxattr;
        const WCHAR *value;
        int len, index;
        HRESULT hr;

        if (!is_clsid_supported(table->clsid, mxattributes_support_data))
        {
            table++;
            i++;
            continue;
        }

        hr = CoCreateInstance(table->clsid, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXAttributes, (void**)&mxattr);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXAttributes_QueryInterface(mxattr, &IID_ISAXAttributes, (void**)&saxattr);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXAttributes_getLength(saxattr, NULL);
        ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

        len = -1;
        hr = ISAXAttributes_getLength(saxattr, &len);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(len == 0, "got %d\n", len);

        hr = ISAXAttributes_getValue(saxattr, 0, &value, &len);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = ISAXAttributes_getValue(saxattr, 0, NULL, &len);
        ok(hr == E_POINTER /* win8 */ || hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = ISAXAttributes_getValue(saxattr, 0, &value, NULL);
        ok(hr == E_POINTER /* win8 */ || hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = ISAXAttributes_getValue(saxattr, 0, NULL, NULL);
        ok(hr == E_POINTER /* win8 */ || hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = ISAXAttributes_getType(saxattr, 0, &value, &len);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = ISAXAttributes_getType(saxattr, 0, NULL, &len);
        ok(hr == E_POINTER /* win8 */ || hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = ISAXAttributes_getType(saxattr, 0, &value, NULL);
        ok(hr == E_POINTER /* win8 */ || hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = ISAXAttributes_getType(saxattr, 0, NULL, NULL);
        ok(hr == E_POINTER /* win8 */ || hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = IMXAttributes_addAttribute(mxattr, _bstr_(table->uri), _bstr_(table->local),
            _bstr_(table->qname), _bstr_(table->type), _bstr_(table->value));
        ok(hr == table->hr, "%d: got %#lx, expected %#lx.\n", i, hr, table->hr);

        if (hr == S_OK)
        {
            hr = ISAXAttributes_getValue(saxattr, 0, NULL, &len);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValue(saxattr, 0, &value, NULL);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValue(saxattr, 0, NULL, NULL);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getType(saxattr, 0, NULL, &len);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getType(saxattr, 0, &value, NULL);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getType(saxattr, 0, NULL, NULL);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            len = -1;
            hr = ISAXAttributes_getValue(saxattr, 0, &value, &len);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(!lstrcmpW(_bstr_(table->value), value), "%d: got %s, expected %s\n", i, wine_dbgstr_w(value),
                table->value);
            ok(lstrlenW(value) == len, "%d: got wrong value length %d\n", i, len);

            len = -1;
            value = (void*)0xdeadbeef;
            hr = ISAXAttributes_getType(saxattr, 0, &value, &len);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            if (table->type)
            {
                ok(!lstrcmpW(_bstr_(table->type), value), "%d: got %s, expected %s\n", i, wine_dbgstr_w(value),
                    table->type);
                ok(lstrlenW(value) == len, "%d: got wrong type value length %d\n", i, len);
            }
            else
            {
                ok(*value == 0, "%d: got type value %s\n", i, wine_dbgstr_w(value));
                ok(len == 0, "%d: got wrong type value length %d\n", i, len);
            }

            hr = ISAXAttributes_getIndexFromQName(saxattr, NULL, 0, NULL);
            if (IsEqualGUID(table->clsid, &CLSID_SAXAttributes) ||
                IsEqualGUID(table->clsid, &CLSID_SAXAttributes30))
            {
                ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
            }
            else
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getIndexFromQName(saxattr, NULL, 0, &index);
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

            index = -1;
            hr = ISAXAttributes_getIndexFromQName(saxattr, L"nonexistent", 11, &index);
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
            ok(index == -1, "%d: got wrong index %d\n", i, index);

            index = -1;
            hr = ISAXAttributes_getIndexFromQName(saxattr, _bstr_(table->qname), 0, &index);
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
            ok(index == -1, "%d: got wrong index %d\n", i, index);

            index = -1;
            hr = ISAXAttributes_getIndexFromQName(saxattr, _bstr_(table->qname), strlen(table->qname), &index);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(index == 0, "%d: got wrong index %d\n", i, index);

            index = -1;
            hr = ISAXAttributes_getIndexFromQName(saxattr, _bstr_(table->qname), strlen(table->qname)-1, &index);
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
            ok(index == -1, "%d: got wrong index %d\n", i, index);

            hr = ISAXAttributes_getValueFromQName(saxattr, NULL, 0, NULL, NULL);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValueFromQName(saxattr, _bstr_(table->qname), 0, NULL, NULL);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValueFromQName(saxattr, _bstr_(table->qname), 0, &value, NULL);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            /* versions 4 and 6 crash */
            hr = ISAXAttributes_getValueFromQName(saxattr, _bstr_(table->qname), strlen(table->qname), NULL, NULL);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValueFromQName(saxattr, _bstr_(table->qname), strlen(table->qname), NULL, &len);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValueFromName(saxattr, NULL, 0, NULL, 0, NULL, NULL);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValueFromName(saxattr, _bstr_(table->uri), 0, NULL, 0, NULL, NULL);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValueFromName(saxattr, _bstr_(table->uri), 0, NULL, 0, &value, NULL);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValueFromName(saxattr, _bstr_(table->uri), 0, _bstr_(table->local), 0, &value, NULL);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValueFromName(saxattr, _bstr_(table->uri), 0, _bstr_(table->local), 0, NULL, &len);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValueFromName(saxattr, _bstr_(table->uri), strlen(table->uri), _bstr_(table->local),
                strlen(table->local), NULL, NULL);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValueFromQName(saxattr, _bstr_(table->qname), strlen(table->qname), &value, &len);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(!lstrcmpW(_bstr_(table->value), value), "%d: got %s, expected %s\n", i, wine_dbgstr_w(value),
                table->value);
            ok(lstrlenW(value) == len, "%d: got wrong value length %d\n", i, len);

            if (table->uri) {
                hr = ISAXAttributes_getValueFromName(saxattr, _bstr_(table->uri), strlen(table->uri),
                    _bstr_(table->local), strlen(table->local), &value, &len);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
                ok(!lstrcmpW(_bstr_(table->value), value), "%d: got %s, expected %s\n", i, wine_dbgstr_w(value),
                    table->value);
                ok(lstrlenW(value) == len, "%d: got wrong value length %d\n", i, len);
            }
        }

        len = -1;
        hr = ISAXAttributes_getLength(saxattr, &len);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (table->hr == S_OK)
            ok(len == 1, "%d: got %d length, expected 1\n", i, len);
        else
            ok(len == 0, "%d: got %d length, expected 0\n", i, len);

        ISAXAttributes_Release(saxattr);
        IMXAttributes_Release(mxattr);

        table++;
        i++;
    }

    free_bstrs();
}

static void test_mxattr_clear(void)
{
    ISAXAttributes *saxattr;
    IMXAttributes *mxattr;
    const WCHAR *ptr;
    HRESULT hr;
    int len;

    hr = CoCreateInstance(&CLSID_SAXAttributes, NULL, CLSCTX_INPROC_SERVER,
        &IID_IMXAttributes, (void**)&mxattr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXAttributes_QueryInterface(mxattr, &IID_ISAXAttributes, (void**)&saxattr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXAttributes_getQName(saxattr, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXAttributes_getQName(saxattr, 0, &ptr, &len);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMXAttributes_clear(mxattr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXAttributes_addAttribute(mxattr, _bstr_("uri"), _bstr_("local"),
        _bstr_("qname"), _bstr_("type"), _bstr_("value"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = -1;
    hr = ISAXAttributes_getLength(saxattr, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 1, "got %d\n", len);

    len = -1;
    hr = ISAXAttributes_getQName(saxattr, 0, NULL, &len);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(len == -1, "got %d\n", len);

    ptr = (void*)0xdeadbeef;
    hr = ISAXAttributes_getQName(saxattr, 0, &ptr, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(ptr == (void*)0xdeadbeef, "got %p\n", ptr);

    len = 0;
    hr = ISAXAttributes_getQName(saxattr, 0, &ptr, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 5, "got %d\n", len);
    ok(!lstrcmpW(ptr, L"qname"), "got %s\n", wine_dbgstr_w(ptr));

    hr = IMXAttributes_clear(mxattr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = -1;
    hr = ISAXAttributes_getLength(saxattr, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 0, "got %d\n", len);

    len = -1;
    ptr = (void*)0xdeadbeef;
    hr = ISAXAttributes_getQName(saxattr, 0, &ptr, &len);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(len == -1, "got %d\n", len);
    ok(ptr == (void*)0xdeadbeef, "got %p\n", ptr);

    IMXAttributes_Release(mxattr);
    ISAXAttributes_Release(saxattr);
    free_bstrs();
}

static void test_mxattr_dispex(void)
{
    IMXAttributes *mxattr;
    IDispatchEx *dispex;
    IUnknown *unk;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SAXAttributes, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXAttributes, (void**)&mxattr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXAttributes_QueryInterface(mxattr, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDispatchEx_QueryInterface(dispex, &IID_IUnknown, (void**)&unk);
    test_obj_dispex(unk);
    IUnknown_Release(unk);
    IDispatchEx_Release(dispex);

    IMXAttributes_Release(mxattr);
}

static void test_mxattr_qi(void)
{
    static const GUID *classes[] = { &CLSID_SAXAttributes, &CLSID_SAXAttributes30 };
    IMXAttributes *mxattr;
    HRESULT hr;

    for (int i = 0; i < ARRAYSIZE(classes); ++i)
    {
        hr = CoCreateInstance(classes[i], NULL, CLSCTX_INPROC_SERVER, &IID_IMXAttributes, (void **)&mxattr);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        check_interface(mxattr, &IID_IMXAttributes, TRUE);
        check_interface(mxattr, &IID_ISAXAttributes, TRUE);
        check_interface(mxattr, &IID_IVBSAXAttributes, TRUE);
        check_interface(mxattr, &IID_IDispatch, TRUE);
        check_interface(mxattr, &IID_IDispatchEx, TRUE);

        IMXAttributes_Release(mxattr);
    }
}

static struct msxmlsupported_data_t saxattr_support_data[] =
{
    { &CLSID_SAXAttributes,   "SAXAttributes"   },
    { &CLSID_SAXAttributes30, "SAXAttributes30" },
    { NULL }
};

static void test_mxattr_localname(void)
{
    const struct msxmlsupported_data_t *table = saxattr_support_data;

    while (table->clsid)
    {
        ISAXAttributes *saxattr;
        IMXAttributes *mxattr;
        HRESULT hr;
        int index;

        if (!is_clsid_supported(table->clsid, mxattributes_support_data))
        {
            table++;
            continue;
        }

        hr = CoCreateInstance(table->clsid, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMXAttributes, (void**)&mxattr);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXAttributes_QueryInterface(mxattr, &IID_ISAXAttributes, (void**)&saxattr);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXAttributes_getIndexFromName(saxattr, NULL, 0, NULL, 0, &index);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        /* add some ambiguous attribute names */
        hr = IMXAttributes_addAttribute(mxattr, _bstr_("uri"), _bstr_("localname"),
            _bstr_("a:localname"), _bstr_(""), _bstr_("value"));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMXAttributes_addAttribute(mxattr, _bstr_("uri"), _bstr_("localname"),
            _bstr_("b:localname"), _bstr_(""), _bstr_("value"));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        index = -1;
        hr = ISAXAttributes_getIndexFromName(saxattr, L"uri", 3, L"localname", 9, &index);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(index == 0, "%s: got index %d\n", table->name, index);

        index = -1;
        hr = ISAXAttributes_getIndexFromName(saxattr, L"uri1", 4, L"localname", 9, &index);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
        ok(index == -1, "%s: got index %d\n", table->name, index);

        index = -1;
        hr = ISAXAttributes_getIndexFromName(saxattr, L"uri", 3, L"localname1", 10, &index);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
        ok(index == -1, "%s: got index %d\n", table->name, index);

        if (IsEqualGUID(table->clsid, &CLSID_SAXAttributes) ||
            IsEqualGUID(table->clsid, &CLSID_SAXAttributes30))
        {
            hr = ISAXAttributes_getIndexFromName(saxattr, NULL, 0, NULL, 0, NULL);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getIndexFromName(saxattr, L"uri", 3, L"localname1", 10, NULL);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
        }
        else
        {
            hr = ISAXAttributes_getIndexFromName(saxattr, NULL, 0, NULL, 0, NULL);
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getIndexFromName(saxattr, L"uri", 3, L"localname1", 10, NULL);
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
        }

        hr = ISAXAttributes_getIndexFromName(saxattr, L"uri", 3, NULL, 0, &index);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = ISAXAttributes_getIndexFromName(saxattr, NULL, 0, L"localname1", 10, &index);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        table++;

        ISAXAttributes_Release(saxattr);
        IMXAttributes_Release(mxattr);
        free_bstrs();
    }
}

static void test_mxwriter_indent(void)
{
    ISAXContentHandler *content;
    IMXWriter *writer;
    VARIANT dest;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_indent(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, L"a", -1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_characters(content, L"", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, L"b", -1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, L"c", -1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endElement(content, L"", 0, L"", 0, L"c", -1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endElement(content, L"", 0, L"", 0, L"b", -1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endElement(content, L"", 0, L"", 0, L"a", -1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<?xml version=\"1.0\" encoding=\"UTF-16\" standalone=\"no\"?>\r\n<a><b>\r\n\t\t<c/>\r\n\t</b>\r\n</a>", V_BSTR(&dest)),
        "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    free_bstrs();
}

static void test_saxreader_vb_content_handler(void)
{
    IVBSAXXMLReader *reader;
    struct sink *sink;
    VARIANT var;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SAXXMLReader, NULL, CLSCTX_INPROC_SERVER, &IID_IVBSAXXMLReader, (void **)&reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    sink = create_test_sink();

    hr = IVBSAXXMLReader_putref_contentHandler(reader, &sink->IVBSAXContentHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = _bstr_("<a>text</a>");
    hr = IVBSAXXMLReader_parse(reader, var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IVBSAXContentHandler_Release(&sink->IVBSAXContentHandler_iface);
    IVBSAXXMLReader_Release(reader);

    free_bstrs();
}

static void test_mxwriter_from_reader(void)
{
    IVBSAXContentHandler *content_handler;
    IVBSAXXMLReader *reader;
    IMXWriter *writer;
    VARIANT var;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SAXXMLReader, NULL, CLSCTX_INPROC_SERVER, &IID_IVBSAXXMLReader, (void **)&reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_MXXMLWriter, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_IVBSAXContentHandler, (void **)&content_handler);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXXMLReader_putref_contentHandler(reader, content_handler);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = _bstr_("<a>text</a>");
    hr = IVBSAXXMLReader_parse(reader, var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    VariantClear(&var);

    V_VT(&var) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&var) == VT_BSTR, "Unexpected output type %d.\n", V_VT(&var));
    ok(!lstrcmpW(L"<a>text</a>", V_BSTR(&var)), "Unexpected content %s.\n", wine_dbgstr_w(V_BSTR(&var)));
    VariantClear(&var);

    IMXWriter_Release(writer);
    IVBSAXXMLReader_Release(reader);
    IVBSAXContentHandler_Release(content_handler);
}

static const WCHAR xml_dtd[] =
    L"<?xml version=\"1.0\" ?>\n"
    "<!DOCTYPE dummy:td-name PUBLIC \"dtd-pub-id\" \"dtd-sys-id\" [\n"
    "<!NOTATION notation0 PUBLIC \"pub-id\" \"sys-id\" >\n"
    "<!ENTITY ent1 SYSTEM \"sys-id\" NDATA notation1 >\n"
    "<!ENTITY % ent1 \"sys-id\" >\n"
    "<!ENTITY % ent2 SYSTEM \"sys-id-2\" >\n"
    "<!ELEMENT div1 (head, (p | list | note)*, div2*)>\n"
    "<!ELEMENT div2 (head, (p | list | note)*, div2*)>\n"
    "<!ELEMENT div3 ( #PCDATA )* >\n"
    "<!ELEMENT div4 ( #PCDATA | name | name)* >\n"
    "<!ELEMENT div5 ( #PCDATA | name)*>\n"
    "<!ELEMENT div6 (a | b* | c?)* >\n"
    "<!ENTITY % ent1 SYSTEM \"sys-id\" >\n"
    "<!ENTITY ent1.1 SYSTEM \"sys-id2\" >\n"
    "<!ENTITY ent2 \"inte&#39;rnal &someent; ent2\" >\n"
    "<!ENTITY ent3 \"t_e_x_t&ent4;\" >\n"
    "<!ENTITY ent4 \"w\" >\n"
    "<!ENTITY ent5 \"w2\" >\n"
    "<!ENTITY % ent5 \"<!ENTITY ent11 \'&ent3;\'>\" >\n"
    "<!ENTITY % ent10 \"<!ELEMENT div3 (head) >\" >\n"
    "<!ENTITY % ent13 \"test\" >\n"
    "<!ENTITY % ent14 \"\" >\n"
    "<!ENTITY ent6 \"a&ent8;\" >\n"
    "<!ENTITY ent7 \"some&:;\" >\n"
    "<!ENTITY ent8 \"ab&ent9;cd&ent4;ef\" >\n"
    "<!ENTITY ent9 \"&ent4;\" >\n"
    "<!ENTITY apos \"blah-apos\" >\n"
    "<!ENTITY quot \"blah-quot\" >\n"
    "<!ENTITY entA \"a\" >\n"
    "<!ENTITY ent11 \"<a/>\" >\n"
    "<!ENTITY ent12 \"\" >\n"
    "<!ATTLIST termdef2 >\n"
    "<!ATTLIST termdef\n"
    "          id      ID      #REQUIRED\n"
    "          name    CDATA   #IMPLIED\n"
    "          na:me2   CDATA   #FIXED \"default&#x20;value\"\n"
    "          name3   CDATA #FIXED \"a-&#x20;-&ent4;b\" >\n"
    "<!ATTLIST termdef\n"
    "          id2     ID      #IMPLIED\n"
    "          id      ID      #IMPLIED\n"
    "          type    (bullets|ordered|:glo-ssary) \"ordered\"\n"
    "          n    NOTATION  (name1 | name2) \"ordered\"\n"
    "          n2    NOTATION  (name3) \"ordered\"\n"
    "          id3     ID      #IMPLIED>\n"
    "<!ATTLIST BankAccount\n"
    "          ns:d CDATA \"defval\">\n"
    "<!ATTLIST BankAccount\n"
    "          ns:s CDATA \"defval2\">\n"
    "]>\n"
    "<a></a>";

static void test_saxreader_dtd(void)
{
    ISAXXMLReader *reader;
    VARIANT var;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SAXXMLReader, NULL, CLSCTX_INPROC_SERVER, &IID_ISAXXMLReader, (void **)&reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    init_saxdeclhandler(&declhandler, S_OK);
    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = (IUnknown *)&declhandler.ISAXDeclHandler_iface;
    hr = ISAXXMLReader_putProperty(reader, L"http://xml.org/sax/properties/declaration-handler", var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    init_saxlexicalhandler(&lexicalhandler, S_OK);
    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = (IUnknown *)&lexicalhandler.ISAXLexicalHandler_iface;
    hr = ISAXXMLReader_putProperty(reader, L"http://xml.org/sax/properties/lexical-handler", var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXXMLReader_putContentHandler(reader, &contentHandler);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(xml_dtd);

    hr = ISAXXMLReader_parse(reader, var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(L"<?xml version=\"1.0\" encoding=\"value\" standalone=\"yes\"?><a>text</a>");

    hr = ISAXXMLReader_parse(reader, var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    VariantClear(&var);

    ISAXXMLReader_Release(reader);
}

static void test_saxreader_parse_input(void)
{
    ISAXXMLReader *reader;
    VARIANT var;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SAXXMLReader, NULL, CLSCTX_INPROC_SERVER, &IID_ISAXXMLReader, (void **)&reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocStringByteLen("<a>text</a>", 11);

    hr = ISAXXMLReader_parse(reader, var);
    todo_wine
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    VariantClear(&var);

    ISAXXMLReader_Release(reader);
}

START_TEST(saxreader)
{
    ISAXXMLReader *reader;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "failed to init com\n");

    hr = CoCreateInstance(&CLSID_SAXXMLReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_ISAXXMLReader, (void**)&reader);

    if(FAILED(hr))
    {
        win_skip("Failed to create SAXXMLReader instance\n");
        CoUninitialize();
        return;
    }
    ISAXXMLReader_Release(reader);

    init_call_sequences(sequences, NUM_CALL_SEQUENCES);

    get_class_support_data(reader_support_data, &IID_ISAXXMLReader);

    test_saxreader();
    test_saxreader_properties();
    test_saxreader_max_xml_size();
    test_saxreader_normalize_line_breaks();
    test_saxreader_exhaustive_errors();
    test_saxreader_features();
    test_saxreader_encoding();
    test_saxreader_dispex();
    test_saxreader_vb_content_handler();
    test_saxreader_cdata();
    test_saxreader_pi();
    test_saxreader_characters();
    test_saxreader_dtd();
    test_saxreader_parse_input();

    /* MXXMLWriter tests */
    get_class_support_data(mxwriter_support_data, &IID_IMXWriter);
    if (is_clsid_supported(&CLSID_MXXMLWriter, mxwriter_support_data))
    {
        test_mxwriter_handlers();
        test_mxwriter_startenddocument();
        test_mxwriter_startendelement();
        test_mxwriter_characters();
        test_mxwriter_comment();
        test_mxwriter_cdata();
        test_mxwriter_pi();
        test_mxwriter_ignorablespaces();
        test_mxwriter_dtd();
        test_mxwriter_properties();
        test_mxwriter_flush();
        test_mxwriter_stream();
        test_mxwriter_encoding();
        test_mxwriter_dispex();
        test_mxwriter_indent();
        test_mxwriter_from_reader();
    }
    else
        win_skip("MXXMLWriter not supported\n");

    /* SAXAttributes tests */
    get_class_support_data(mxattributes_support_data, &IID_IMXAttributes);
    if (is_clsid_supported(&CLSID_SAXAttributes, mxattributes_support_data))
    {
        test_mxattr_qi();
        test_mxattr_addAttribute();
        test_mxattr_clear();
        test_mxattr_localname();
        test_mxattr_dispex();
    }
    else
        win_skip("SAXAttributes not supported\n");

    CoUninitialize();
}
