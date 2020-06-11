/*
 * Copyright 2019 Dmitry Timoshkov
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
#define NONAMELESSSTRUCT
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winspool.h"
#include "objbase.h"
#include "prntvpt.h"
#include "initguid.h"
#include "msxml2.h"
#include "wine/heap.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(prntvpt);

struct size
{
    int width;
    int height;
};

struct media
{
    int paper;
    struct size size;
};

struct resolution
{
    int x;
    int y;
};

struct page
{
    struct media media;
    struct resolution resolution;
    int orientation;
    int scaling;
    int color;
};

struct document
{
    int collate;
};

struct job
{
    int nup;
    int copies;
    int input_bin;
};

struct ticket
{
    struct job job;
    struct document document;
    struct page page;
};

static const struct
{
    const WCHAR *name;
    int paper;
} psk_media[] =
{
    { L"psk:ISOA4", DMPAPER_A4 },
};

static int media_to_paper(const WCHAR *name)
{
    int i;

    for (i = 0 ; i < ARRAY_SIZE(psk_media); i++)
        if (!wcscmp(name, psk_media[i].name))
            return psk_media[i].paper;

    FIXME("%s\n", wine_dbgstr_w(name));
    return DMPAPER_A4;
}

static BOOL is_valid_node_name(const WCHAR *name)
{
    static const WCHAR * const psf_names[] = { L"psf:ParameterInit", L"psf:Feature" };
    int i;

    for (i = 0 ; i < ARRAY_SIZE(psf_names); i++)
        if (!wcscmp(name, psf_names[i])) return TRUE;

    return FALSE;
}

static HRESULT verify_ticket(IXMLDOMDocument2 *doc)
{
    IXMLDOMElement *element;
    IXMLDOMNode *node = NULL;
    BSTR str;
    HRESULT hr;

    hr = IXMLDOMDocument2_get_documentElement(doc, &element);
    if (hr != S_OK) return E_PRINTTICKET_FORMAT;

    hr = IXMLDOMElement_get_tagName(element, &str);
    if (hr != S_OK) goto fail;
    if (wcscmp(str, L"psf:PrintTicket") != 0)
        hr = E_FAIL;
    SysFreeString(str);
    if (hr != S_OK) goto fail;

    hr = IXMLDOMElement_get_firstChild(element, &node);
    IXMLDOMElement_Release(element);
    if (hr != S_OK) return S_OK;

    for (;;)
    {
        VARIANT var;
        IXMLDOMNode *next_node;

        hr = IXMLDOMNode_get_nodeName(node, &str);
        if (hr != S_OK) break;
        if (!is_valid_node_name(str))
            hr = E_FAIL;
        SysFreeString(str);
        if (hr != S_OK) break;

        hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void **)&element);
        if (hr != S_OK) break;

        VariantInit(&var);
        hr = IXMLDOMElement_getAttribute(element, (BSTR)L"name", &var);
        IXMLDOMElement_Release(element);
        if (hr != S_OK) break;
        if (V_VT(&var) != VT_BSTR)
            hr = E_FAIL;
        VariantClear(&var);
        if (hr != S_OK) break;

        hr = IXMLDOMNode_get_nextSibling(node, &next_node);
        if (hr != S_OK)
        {
            hr = S_OK;
            break;
        }

        IXMLDOMNode_Release(node);
        node = next_node;
    }

fail:
    if (node) IXMLDOMNode_Release(node);

    return hr != S_OK ? E_PRINTTICKET_FORMAT : S_OK;
}

static HRESULT read_int_value(IXMLDOMNode *node, int *value)
{
    IXMLDOMNode *val;
    HRESULT hr;
    VARIANT var1, var2;

    hr = IXMLDOMNode_selectSingleNode(node, (BSTR)L"./psf:Value[@xsi:type='xsd:integer']", &val);
    if (hr != S_OK) return hr;

    VariantInit(&var1);
    hr = IXMLDOMNode_get_nodeTypedValue(val, &var1);
    if (hr == S_OK)
    {
        VariantInit(&var2);
        hr = VariantChangeTypeEx(&var2, &var1, 0, 0, VT_I4);
        if (hr == S_OK)
            *value = V_I4(&var2);

        VariantClear(&var1);
    }

    IXMLDOMNode_Release(val);
    return hr;
}

static void read_PageMediaSize(IXMLDOMDocument2 *doc, struct ticket *ticket)
{
    IXMLDOMNode *node, *option, *child;
    HRESULT hr;

    hr = IXMLDOMDocument2_selectSingleNode(doc, (BSTR)L"psf:PrintTicket/psf:Feature[@name='psk:PageMediaSize']", &node);
    if (hr != S_OK) return;

    hr = IXMLDOMNode_selectSingleNode(node, (BSTR)L"./psf:Option", &option);
    if (hr == S_OK)
    {
        IXMLDOMElement *element;

        hr = IXMLDOMNode_QueryInterface(option, &IID_IXMLDOMElement, (void **)&element);
        if (hr == S_OK)
        {
            VARIANT var;

            VariantInit(&var);
            hr = IXMLDOMElement_getAttribute(element, (BSTR)L"name", &var);
            if (hr == S_OK && V_VT(&var) == VT_BSTR)
            {
                ticket->page.media.paper = media_to_paper(V_BSTR(&var));
                TRACE("paper: %s => %d\n", wine_dbgstr_w(V_BSTR(&var)), ticket->page.media.paper);
            }
            VariantClear(&var);

            IXMLDOMElement_Release(element);
        }

        hr = IXMLDOMNode_selectSingleNode(option, (BSTR)L"./psf:ScoredProperty[@name='psk:MediaSizeWidth']", &child);
        if (hr == S_OK)
        {
            if (read_int_value(child, &ticket->page.media.size.width) == S_OK)
                TRACE("width: %d\n", ticket->page.media.size.width);
            IXMLDOMNode_Release(child);
        }

        hr = IXMLDOMNode_selectSingleNode(option, (BSTR)L"./psf:ScoredProperty[@name='psk:MediaSizeHeight']", &child);
        if (hr == S_OK)
        {
            if (read_int_value(child, &ticket->page.media.size.height) == S_OK)
                TRACE("height: %d\n", ticket->page.media.size.height);
            IXMLDOMNode_Release(child);
        }

        IXMLDOMNode_Release(option);
    }

    IXMLDOMNode_Release(node);
}

static void set_SelectionNamespaces(IXMLDOMDocument2 *doc)
{
    IStream *stream;
    IXMLDOMElement *element = NULL;
    IXMLDOMNamedNodeMap *map = NULL;
    HRESULT hr;
    LONG count, i;
    HGLOBAL hmem;
    BSTR str;
    VARIANT var;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    if (hr != S_OK) return;

    hr = IXMLDOMDocument2_get_documentElement(doc, &element);
    if (hr != S_OK) goto fail;

    hr = IXMLDOMElement_get_attributes(element, &map);
    if (hr != S_OK) goto fail;

    hr = IXMLDOMNamedNodeMap_get_length(map, &count);
    if (hr != S_OK) goto fail;

    for (i = 0; i < count; i++)
    {
        IXMLDOMNode *node;

        hr = IXMLDOMNamedNodeMap_get_item(map, i, &node);
        if (hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeName(node, &str);
            if (hr == S_OK)
            {
                VariantInit(&var);
                hr = IXMLDOMNode_get_nodeValue(node, &var);
                if (hr == S_OK)
                {
                    if (!wcscmp(str, L"xmlns") || !wcsncmp(str, L"xmlns:", 6))
                    {
                        TRACE("ns[%d]: %s=%s\n", i, wine_dbgstr_w(str), wine_dbgstr_w(V_BSTR(&var)));
                        IStream_Write(stream, str, wcslen(str) * sizeof(WCHAR), NULL);
                        IStream_Write(stream, L"=\"", 2 * sizeof(WCHAR), NULL);
                        IStream_Write(stream, V_BSTR(&var), wcslen(V_BSTR(&var)) * sizeof(WCHAR), NULL);
                        IStream_Write(stream, L"\" ", 2 * sizeof(WCHAR), NULL);
                    }
                    VariantClear(&var);
                }
                SysFreeString(str);
            }
            IXMLDOMNode_Release(node);
        }
    }

    IStream_Write(stream, L"", sizeof(WCHAR), NULL);

    hr = GetHGlobalFromStream(stream, &hmem);
    if (hr != S_OK) goto fail;

    str = GlobalLock(hmem);
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(str);
    IXMLDOMDocument2_setProperty(doc, (BSTR)L"SelectionNamespaces", var);
    SysFreeString(V_BSTR(&var));
    GlobalUnlock(hmem);

fail:
    if (map) IXMLDOMNamedNodeMap_Release(map);
    if (element) IXMLDOMElement_Release(element);
    IStream_Release(stream);
}

static HRESULT parse_ticket(IStream *stream, EPrintTicketScope scope, struct ticket *ticket)
{
    IXMLDOMDocument2 *doc;
    VARIANT src;
    VARIANT_BOOL ret;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DOMDocument30, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IXMLDOMDocument2, (void **)&doc);
    if (hr != S_OK) return hr;

    V_VT(&src) = VT_UNKNOWN;
    V_UNKNOWN(&src) = (IUnknown *)stream;
    hr = IXMLDOMDocument2_load(doc, src, &ret);
    if (hr != S_OK) goto fail;

    hr = verify_ticket(doc);
    if (hr != S_OK) goto fail;

    set_SelectionNamespaces(doc);

    /* PageScope is always added */
    read_PageMediaSize(doc, ticket);

fail:
    IXMLDOMDocument2_Release(doc);
    return hr;
}

static void ticket_to_devmode(const struct ticket *ticket, DEVMODEW *dm)
{
    memset(dm, 0, sizeof(*dm));

    dm->dmSize = sizeof(*dm);
    dm->dmFields = DM_ORIENTATION | DM_PAPERSIZE | DM_PAPERLENGTH | DM_PAPERWIDTH | DM_SCALE |
                   DM_COPIES | DM_COLOR | DM_PRINTQUALITY | DM_YRESOLUTION | DM_COLLATE;
    dm->u1.s1.dmOrientation = ticket->page.orientation;
    dm->u1.s1.dmPaperSize = ticket->page.media.paper;
    dm->u1.s1.dmPaperWidth = ticket->page.media.size.width / 100;
    dm->u1.s1.dmPaperLength = ticket->page.media.size.height / 100;
    dm->u1.s1.dmScale = ticket->page.scaling;
    dm->u1.s1.dmCopies = ticket->job.copies;
    dm->dmColor = ticket->page.color;
    dm->u1.s1.dmPrintQuality = ticket->page.resolution.x;
    dm->dmYResolution = ticket->page.resolution.y;
    dm->dmCollate = ticket->document.collate;
}

static void initialize_ticket(struct ticket *ticket)
{
    ticket->job.nup = 0;
    ticket->job.copies = 1;
    ticket->job.input_bin = DMBIN_AUTO;
    ticket->document.collate = DMCOLLATE_FALSE;
    ticket->page.media.paper = DMPAPER_A4;
    ticket->page.media.size.width = 210000;
    ticket->page.media.size.height = 297000;
    ticket->page.resolution.x = 600;
    ticket->page.resolution.y = 600;
    ticket->page.orientation = DMORIENT_PORTRAIT;
    ticket->page.scaling = 100;
    ticket->page.color = DMCOLOR_MONOCHROME;
}

HRESULT WINAPI PTConvertPrintTicketToDevMode(HPTPROVIDER provider, IStream *stream, EDefaultDevmodeType type,
                                             EPrintTicketScope scope, ULONG *size, PDEVMODEW *dm, BSTR *error)
{
    HRESULT hr;
    struct ticket ticket;

    TRACE("%p,%p,%d,%d,%p,%p,%p\n", provider, stream, type, scope, size, dm, error);

    if (!provider || !stream || !size || !dm)
        return E_INVALIDARG;

    initialize_ticket(&ticket);

    hr = parse_ticket(stream, scope, &ticket);
    if (hr != S_OK) return hr;

    *dm = heap_alloc(sizeof(**dm));
    if (!dm) return E_OUTOFMEMORY;

    ticket_to_devmode(&ticket, *dm);
    *size = sizeof(**dm);

    return S_OK;
}
