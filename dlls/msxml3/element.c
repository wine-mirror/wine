/*
 *    DOM Document implementation
 *
 * Copyright 2005 Mike McCormack
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"
#include "msxml6.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct _domelem
{
    DispatchEx dispex;
    IXMLDOMElement IXMLDOMElement_iface;
    LONG refcount;
    struct domnode *node;
} domelem;

static const struct nodemap_funcs domelem_attr_map;

static const tid_t domelem_se_tids[] =
{
    IXMLDOMNode_tid,
    IXMLDOMElement_tid,
    NULL_tid
};

static inline domelem *impl_from_IXMLDOMElement(IXMLDOMElement *iface)
{
    return CONTAINING_RECORD(iface, domelem, IXMLDOMElement_iface);
}

static HRESULT WINAPI domelem_QueryInterface(IXMLDOMElement *iface, REFIID riid, void **obj)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IXMLDOMElement) ||
        IsEqualGUID(riid, &IID_IXMLDOMNode) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = &element->IXMLDOMElement_iface;
    }
    else if (dispex_query_interface(&element->dispex, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (node_query_interface(element->node, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (IsEqualGUID(riid, &IID_ISupportErrorInfo))
    {
        return node_create_supporterrorinfo(domelem_se_tids, obj);
    }
    else
    {
        TRACE("interface %s not implemented\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI domelem_AddRef(IXMLDOMElement *iface)
{
    domelem *element = impl_from_IXMLDOMElement(iface);
    LONG refcount = InterlockedIncrement(&element->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI domelem_Release(IXMLDOMElement *iface)
{
    domelem *element = impl_from_IXMLDOMElement(iface);
    ULONG refcount = InterlockedDecrement(&element->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        domnode_release(element->node);
        free(element);
    }

    return refcount;
}

static HRESULT WINAPI domelem_GetTypeInfoCount(IXMLDOMElement *iface, UINT *count)
{
    domelem *element = impl_from_IXMLDOMElement(iface);
    return IDispatchEx_GetTypeInfoCount(&element->dispex.IDispatchEx_iface, count);
}

static HRESULT WINAPI domelem_GetTypeInfo(IXMLDOMElement *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    domelem *element = impl_from_IXMLDOMElement(iface);
    return IDispatchEx_GetTypeInfo(&element->dispex.IDispatchEx_iface, index, lcid, ti);
}

static HRESULT WINAPI domelem_GetIDsOfNames(IXMLDOMElement *iface, REFIID riid, LPOLESTR* rgszNames,
        UINT cNames, LCID lcid, DISPID* rgDispId)
{
    domelem *element = impl_from_IXMLDOMElement(iface);
    return IDispatchEx_GetIDsOfNames(&element->dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI domelem_Invoke(IXMLDOMElement *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    domelem *element = impl_from_IXMLDOMElement(iface);
    return IDispatchEx_Invoke(&element->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI domelem_get_nodeName(IXMLDOMElement *iface, BSTR *p)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_name(element->node, p);
}

static HRESULT WINAPI domelem_get_nodeValue(IXMLDOMElement *iface, VARIANT *value)
{
    TRACE("%p, %p.\n", iface, value);

    return return_null_var(value);
}

static HRESULT WINAPI domelem_put_nodeValue(IXMLDOMElement *iface, VARIANT value)
{
    TRACE("%p, %s.\n", iface, debugstr_variant(&value));

    return E_FAIL;
}

static HRESULT WINAPI domelem_get_nodeType(IXMLDOMElement *iface, DOMNodeType *type)
{
    TRACE("%p, %p.\n", iface, type);

    *type = NODE_ELEMENT;
    return S_OK;
}

static HRESULT WINAPI domelem_get_parentNode(IXMLDOMElement *iface, IXMLDOMNode **parent)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p.\n", iface, parent);

    return node_get_parent(element->node, parent);
}

static HRESULT WINAPI domelem_get_childNodes(IXMLDOMElement *iface, IXMLDOMNodeList **list)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p.\n", iface, list);

    return node_get_child_nodes(element->node, list);
}

static HRESULT WINAPI domelem_get_firstChild(IXMLDOMElement *iface, IXMLDOMNode **node)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_first_child(element->node, node);
}

static HRESULT WINAPI domelem_get_lastChild(IXMLDOMElement *iface, IXMLDOMNode **node)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_last_child(element->node, node);
}

static HRESULT WINAPI domelem_get_previousSibling(IXMLDOMElement *iface, IXMLDOMNode **node)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_previous_sibling(element->node, node);
}

static HRESULT WINAPI domelem_get_nextSibling(IXMLDOMElement *iface, IXMLDOMNode **node)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_next_sibling(element->node, node);
}

static HRESULT WINAPI domelem_get_attributes(IXMLDOMElement *iface, IXMLDOMNamedNodeMap **map)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p.\n", iface, map);

    return create_nodemap(element->node, &domelem_attr_map, map);
}

static HRESULT WINAPI domelem_insertBefore(IXMLDOMElement *iface,
        IXMLDOMNode *newNode, VARIANT refChild, IXMLDOMNode **old_node)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p, %s, %p.\n", iface, newNode, debugstr_variant(&refChild), old_node);

    return node_insert_before(element->node, newNode, &refChild, old_node);
}

static HRESULT WINAPI domelem_replaceChild(IXMLDOMElement *iface,
        IXMLDOMNode *newNode, IXMLDOMNode *oldNode, IXMLDOMNode **outOldNode)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p, %p, %p.\n", iface, newNode, oldNode, outOldNode);

    return node_replace_child(element->node, newNode, oldNode, outOldNode);
}

static HRESULT WINAPI domelem_removeChild(IXMLDOMElement *iface,
        IXMLDOMNode *child, IXMLDOMNode **oldChild)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p, %p.\n", iface, child, oldChild);

    return node_remove_child(element->node, child, oldChild);
}

static HRESULT WINAPI domelem_appendChild(IXMLDOMElement *iface,
        IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p, %p.\n", iface, child, outChild);

    return node_append_child(element->node, child, outChild);
}

static HRESULT WINAPI domelem_hasChildNodes(IXMLDOMElement *iface, VARIANT_BOOL *ret)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p.\n", iface, ret);

    return node_has_childnodes(element->node, ret);
}

static HRESULT WINAPI domelem_get_ownerDocument(IXMLDOMElement *iface, IXMLDOMDocument **doc)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p.\n", iface, doc);

    return node_get_owner_document(element->node, doc);
}

static HRESULT WINAPI domelem_cloneNode(IXMLDOMElement *iface, VARIANT_BOOL deep, IXMLDOMNode **node)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %d, %p.\n", iface, deep, node);

    return node_clone(element->node, deep, node);
}

static HRESULT WINAPI domelem_get_nodeTypeString(IXMLDOMElement *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"element", p);
}

static HRESULT WINAPI domelem_get_text(IXMLDOMElement *iface, BSTR *p)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_text(element->node, p);
}

static HRESULT WINAPI domelem_put_text(IXMLDOMElement *iface, BSTR p)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(p));

    return node_put_data(element->node, p);
}

static HRESULT WINAPI domelem_get_specified(IXMLDOMElement *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p stub!\n", iface, v);

    *v = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domelem_get_definition(IXMLDOMElement *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p.\n", iface, node);

    return E_NOTIMPL;
}

static inline BYTE hex_to_byte(xmlChar c)
{
    if(c <= '9') return c-'0';
    if(c <= 'F') return c-'A'+10;
    return c-'a'+10;
}

static inline BYTE base64_to_byte(xmlChar c)
{
    if(c == '+') return 62;
    if(c == '/') return 63;
    if(c <= '9') return c-'0'+52;
    if(c <= 'Z') return c-'A';
    return c-'a'+26;
}

static inline HRESULT variant_from_dt(XDR_DT dt, WCHAR *str, VARIANT *v)
{
    VARIANT src;
    HRESULT hr = S_OK;

    VariantInit(&src);

    switch (dt)
    {
    case DT_STRING:
    case DT_NMTOKEN:
    case DT_NMTOKENS:
    case DT_NUMBER:
    case DT_URI:
    case DT_UUID:
        {
            V_VT(v) = VT_BSTR;
            V_BSTR(v) = SysAllocString(str);
            return V_BSTR(v) ? S_OK : E_OUTOFMEMORY;
        }
    case DT_DATE:
    case DT_DATE_TZ:
    case DT_DATETIME:
    case DT_DATETIME_TZ:
    case DT_TIME:
    case DT_TIME_TZ:
        {
            WCHAR *p, *e;
            SYSTEMTIME st;
            DOUBLE date = 0.0;

            st.wYear = 1899;
            st.wMonth = 12;
            st.wDay = 30;
            st.wDayOfWeek = st.wHour = st.wMinute = st.wSecond = st.wMilliseconds = 0;

            V_VT(&src) = VT_BSTR;
            V_BSTR(&src) = SysAllocString(str);

            if(!V_BSTR(&src))
                return E_OUTOFMEMORY;

            p = V_BSTR(&src);
            e = p + SysStringLen(V_BSTR(&src));

            if(p+4<e && *(p+4)=='-') /* parse date (yyyy-mm-dd) */
            {
                st.wYear = wcstol(p, NULL, 10);
                st.wMonth = wcstol(p+5, NULL, 10);
                st.wDay = wcstol(p+8, NULL, 10);
                p += 10;

                if(*p == 'T') p++;
            }

            if(p+2<e && *(p+2)==':') /* parse time (hh:mm:ss.?) */
            {
                st.wHour = wcstol(p, NULL, 10);
                st.wMinute = wcstol(p+3, NULL, 10);
                st.wSecond = wcstol(p+6, NULL, 10);
                p += 8;

                if(*p == '.')
                {
                    p++;
                    while (*p >= '0' && *p <= '9') p++;
                }
            }

            SystemTimeToVariantTime(&st, &date);
            V_VT(v) = VT_DATE;
            V_DATE(v) = date;

            if(*p == '+') /* parse timezone offset (+hh:mm) */
                V_DATE(v) += (DOUBLE)wcstol(p+1, NULL, 10)/24 + (DOUBLE)wcstol(p+4, NULL, 10)/1440;
            else if(*p == '-') /* parse timezone offset (-hh:mm) */
                V_DATE(v) -= (DOUBLE)wcstol(p+1, NULL, 10)/24 + (DOUBLE)wcstol(p+4, NULL, 10)/1440;

            VariantClear(&src);
            return S_OK;
        }
        break;
    case DT_BIN_HEX:
        {
            SAFEARRAYBOUND sab;
            int i, len;

            len = wcslen(str) / 2;
            sab.lLbound = 0;
            sab.cElements = len;

            V_VT(v) = (VT_ARRAY|VT_UI1);
            V_ARRAY(v) = SafeArrayCreate(VT_UI1, 1, &sab);

            if(!V_ARRAY(v))
                return E_OUTOFMEMORY;

            for(i=0; i<len; i++)
                ((BYTE*)V_ARRAY(v)->pvData)[i] = (hex_to_byte(str[2*i])<<4)
                    + hex_to_byte(str[2*i+1]);
            return S_OK;
        }
        break;
    case DT_BIN_BASE64:
        {
            SAFEARRAYBOUND sab;
            WCHAR *c1, *c2;
            int i, len;

            /* remove all formatting chars */
            c1 = c2 = str;
            len = 0;
            while (*c2)
            {
                if ( *c2 == ' '  || *c2 == '\t' ||
                     *c2 == '\n' || *c2 == '\r' )
                {
                    c2++;
                    continue;
                }
                *c1++ = *c2++;
                len++;
            }

            /* skip padding */
            if(str[len-2] == '=') i = 2;
            else if(str[len-1] == '=') i = 1;
            else i = 0;

            sab.lLbound = 0;
            sab.cElements = len/4*3-i;

            V_VT(v) = (VT_ARRAY|VT_UI1);
            V_ARRAY(v) = SafeArrayCreate(VT_UI1, 1, &sab);

            if(!V_ARRAY(v))
                return E_OUTOFMEMORY;

            for(i=0; i<len/4; i++)
            {
                ((BYTE*)V_ARRAY(v)->pvData)[3*i] = (base64_to_byte(str[4*i])<<2)
                    + (base64_to_byte(str[4*i+1])>>4);
                if(3*i+1 < sab.cElements)
                    ((BYTE*)V_ARRAY(v)->pvData)[3*i+1] = (base64_to_byte(str[4*i+1])<<4)
                        + (base64_to_byte(str[4*i+2])>>2);
                if(3*i+2 < sab.cElements)
                    ((BYTE*)V_ARRAY(v)->pvData)[3*i+2] = (base64_to_byte(str[4*i+2])<<6)
                        + base64_to_byte(str[4*i+3]);
            }
            return S_OK;
        }
        break;
    case DT_BOOLEAN:
        V_VT(v) = VT_BOOL;
        break;
    case DT_FIXED_14_4:
        V_VT(v) = VT_CY;
        break;
    case DT_I1:
        V_VT(v) = VT_I1;
        break;
    case DT_I2:
        V_VT(v) = VT_I2;
        break;
    case DT_I4:
    case DT_INT:
        V_VT(v) = VT_I4;
        break;
    case DT_I8:
        V_VT(v) = VT_I8;
        break;
    case DT_R4:
        V_VT(v) = VT_R4;
        break;
    case DT_FLOAT:
    case DT_R8:
        V_VT(v) = VT_R8;
        break;
    case DT_UI1:
        V_VT(v) = VT_UI1;
        break;
    case DT_UI2:
        V_VT(v) = VT_UI2;
        break;
    case DT_UI4:
        V_VT(v) = VT_UI4;
        break;
    case DT_UI8:
        V_VT(v) = VT_UI8;
        break;
    case DT_CHAR:
    case DT_ENTITY:
    case DT_ENTITIES:
    case DT_ENUMERATION:
    case DT_ID:
    case DT_IDREF:
    case DT_IDREFS:
    case DT_NOTATION:
        FIXME("need to handle dt:%s\n", debugstr_dt(dt));
        V_VT(v) = VT_BSTR;
        V_BSTR(v) = SysAllocString(str);
        return V_BSTR(v) ? S_OK : E_OUTOFMEMORY;
    default:
        WARN("unknown type %d\n", dt);
    }

    V_VT(&src) = VT_BSTR;
    V_BSTR(&src) = SysAllocString(str);

    if (!V_BSTR(&src))
        return E_OUTOFMEMORY;

    hr = VariantChangeTypeEx(v, &src,
            MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT),0, V_VT(v));
    VariantClear(&src);
    return hr;
}

static HRESULT WINAPI domelem_get_nodeTypedValue(IXMLDOMElement *iface, VARIANT *v)
{
    domelem *element = impl_from_IXMLDOMElement(iface);
    HRESULT hr;
    BSTR text;
    XDR_DT dt;

    TRACE("%p, %p.\n", iface, v);

    if (!v)
        return E_INVALIDARG;

    V_VT(v) = VT_NULL;

    dt = node_get_data_type(element->node);
    if (dt == DT_INVALID)
    {
        V_VT(v) = VT_BSTR;
        hr = node_get_text(element->node, &V_BSTR(v));
    }
    else if (SUCCEEDED(hr = node_get_preserved_text(element->node, &text)))
    {
        hr = variant_from_dt(dt, text, v);
        SysFreeString(text);
    }

    return hr;
}

static HRESULT encode_base64(const BYTE *buf, int len, BSTR *ret)
{
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const BYTE *d = buf;
    int bytes, pad_bytes, div;
    DWORD needed;
    WCHAR *ptr;

    bytes = (len*8 + 5)/6;
    pad_bytes = (bytes % 4) ? 4 - (bytes % 4) : 0;

    TRACE("%d, bytes is %d, pad bytes is %d\n", len, bytes, pad_bytes);
    needed = bytes + pad_bytes + 1;

    *ret = SysAllocStringLen(NULL, needed);
    if (!*ret) return E_OUTOFMEMORY;

    /* Three bytes of input give 4 chars of output */
    div = len / 3;

    ptr = *ret;
    while (div > 0)
    {
        /* first char is the first 6 bits of the first byte*/
        *ptr++ = b64[ ( d[0] >> 2) & 0x3f ];
        /* second char is the last 2 bits of the first byte and the first 4
         * bits of the second byte */
        *ptr++ = b64[ ((d[0] << 4) & 0x30) | (d[1] >> 4 & 0x0f)];
        /* third char is the last 4 bits of the second byte and the first 2
         * bits of the third byte */
        *ptr++ = b64[ ((d[1] << 2) & 0x3c) | (d[2] >> 6 & 0x03)];
        /* fourth char is the remaining 6 bits of the third byte */
        *ptr++ = b64[   d[2]       & 0x3f];
        d += 3;
        div--;
    }

    switch (pad_bytes)
    {
        case 1:
            /* first char is the first 6 bits of the first byte*/
            *ptr++ = b64[ ( d[0] >> 2) & 0x3f ];
            /* second char is the last 2 bits of the first byte and the first 4
             * bits of the second byte */
            *ptr++ = b64[ ((d[0] << 4) & 0x30) | (d[1] >> 4 & 0x0f)];
            /* third char is the last 4 bits of the second byte padded with
             * two zeroes */
            *ptr++ = b64[ ((d[1] << 2) & 0x3c) ];
            /* fourth char is a = to indicate one byte of padding */
            *ptr++ = '=';
            break;
        case 2:
            /* first char is the first 6 bits of the first byte*/
            *ptr++ = b64[ ( d[0] >> 2) & 0x3f ];
            /* second char is the last 2 bits of the first byte padded with
             * four zeroes*/
            *ptr++ = b64[ ((d[0] << 4) & 0x30)];
            /* third char is = to indicate padding */
            *ptr++ = '=';
            /* fourth char is = to indicate padding */
            *ptr++ = '=';
            break;
    }

    return S_OK;
}

static HRESULT encode_binhex(const BYTE *buf, int len, BSTR *ret)
{
    static const char byte_to_hex[16] = "0123456789abcdef";
    int i;

    *ret = SysAllocStringLen(NULL, len*2);
    if (!*ret) return E_OUTOFMEMORY;

    for (i = 0; i < len; i++)
    {
        (*ret)[2*i]   = byte_to_hex[buf[i] >> 4];
        (*ret)[2*i+1] = byte_to_hex[0x0f & buf[i]];
    }

    return S_OK;
}

static HRESULT WINAPI domelem_put_nodeTypedValue(IXMLDOMElement *iface, VARIANT value)
{
    domelem *element = impl_from_IXMLDOMElement(iface);
    XDR_DT dt;
    HRESULT hr;

    TRACE("%p, %s.\n", iface, debugstr_variant(&value));

    dt = node_get_data_type(element->node);
    switch (dt)
    {
    /* for untyped node coerce to BSTR and set */
    case DT_INVALID:
    case DT_INT:
        if (V_VT(&value) != VT_BSTR)
        {
            VARIANT content;
            VariantInit(&content);
            hr = VariantChangeType(&content, &value, 0, VT_BSTR);
            if (hr == S_OK)
            {
                hr = node_put_data(element->node, V_BSTR(&content));
                VariantClear(&content);
            }
        }
        else
        {
            hr = node_put_data(element->node, V_BSTR(&value));
        }
        break;
    case DT_BIN_BASE64:
        if (V_VT(&value) == VT_BSTR)
        {
            hr = node_put_data(element->node, V_BSTR(&value));
        }
        else if (V_VT(&value) == (VT_UI1|VT_ARRAY))
        {
            UINT dim = SafeArrayGetDim(V_ARRAY(&value));
            LONG lbound, ubound;
            BSTR encoded;
            BYTE *ptr;
            int len;

            if (dim > 1)
                FIXME("unexpected array dimension count %u\n", dim);

            SafeArrayGetUBound(V_ARRAY(&value), 1, &ubound);
            SafeArrayGetLBound(V_ARRAY(&value), 1, &lbound);

            len = (ubound - lbound + 1)*SafeArrayGetElemsize(V_ARRAY(&value));

            hr = SafeArrayAccessData(V_ARRAY(&value), (void*)&ptr);
            if (FAILED(hr)) return hr;

            hr = encode_base64(ptr, len, &encoded);
            SafeArrayUnaccessData(V_ARRAY(&value));
            if (FAILED(hr)) return hr;

            hr = node_put_data(element->node, encoded);
            SysFreeString(encoded);
        }
        else
        {
            FIXME("unhandled variant type %d for dt:%s\n", V_VT(&value), debugstr_dt(dt));
            return E_NOTIMPL;
        }
        break;
    case DT_BIN_HEX:
        if (V_VT(&value) == (VT_UI1|VT_ARRAY))
        {
            UINT dim = SafeArrayGetDim(V_ARRAY(&value));
            LONG lbound, ubound;
            BSTR encoded;
            BYTE *ptr;
            int len;

            if (dim > 1)
                FIXME("unexpected array dimension count %u\n", dim);

            SafeArrayGetUBound(V_ARRAY(&value), 1, &ubound);
            SafeArrayGetLBound(V_ARRAY(&value), 1, &lbound);

            len = (ubound - lbound + 1)*SafeArrayGetElemsize(V_ARRAY(&value));

            hr = SafeArrayAccessData(V_ARRAY(&value), (void*)&ptr);
            if (FAILED(hr)) return hr;

            hr = encode_binhex(ptr, len, &encoded);
            SafeArrayUnaccessData(V_ARRAY(&value));
            if (FAILED(hr)) return hr;

            hr = node_put_data(element->node, encoded);
            SysFreeString(encoded);
        }
        else
        {
            FIXME("unhandled variant type %d for dt:%s\n", V_VT(&value), debugstr_dt(dt));
            return E_NOTIMPL;
        }
        break;
    default:
        FIXME("not implemented for dt:%s\n", debugstr_dt(dt));
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT WINAPI domelem_get_dataType(IXMLDOMElement *iface, VARIANT *typename)
{
    domelem *element = impl_from_IXMLDOMElement(iface);
    XDR_DT dt;

    TRACE("%p, %p.\n", iface, typename);

    if (!typename)
        return E_INVALIDARG;

    dt = node_get_data_type(element->node);
    switch (dt)
    {
        case DT_BIN_BASE64:
        case DT_BIN_HEX:
        case DT_BOOLEAN:
        case DT_CHAR:
        case DT_DATE:
        case DT_DATE_TZ:
        case DT_DATETIME:
        case DT_DATETIME_TZ:
        case DT_FIXED_14_4:
        case DT_FLOAT:
        case DT_I1:
        case DT_I2:
        case DT_I4:
        case DT_I8:
        case DT_INT:
        case DT_NUMBER:
        case DT_R4:
        case DT_R8:
        case DT_TIME:
        case DT_TIME_TZ:
        case DT_UI1:
        case DT_UI2:
        case DT_UI4:
        case DT_UI8:
        case DT_URI:
        case DT_UUID:
            V_VT(typename) = VT_BSTR;
            V_BSTR(typename) = SysAllocString(dt_to_bstr(dt));

            if (!V_BSTR(typename))
                return E_OUTOFMEMORY;
            break;
        default:
            /* Other types (DTD equivalents) do not return anything here,
             * but the pointer part of the VARIANT is set to NULL */
            V_VT(typename) = VT_NULL;
            V_BSTR(typename) = NULL;
            break;
    }
    return (V_VT(typename) != VT_NULL) ? S_OK : S_FALSE;
}

static HRESULT WINAPI domelem_put_dataType(IXMLDOMElement *iface, BSTR dtName)
{
    domelem *element = impl_from_IXMLDOMElement(iface);
    HRESULT hr = E_FAIL;
    BSTR text;
    XDR_DT dt;

    TRACE("%p, %s.\n", iface, debugstr_w(dtName));

    if (!dtName)
        return E_INVALIDARG;

    dt = bstr_to_dt(dtName, -1);

    if (FAILED(hr = node_get_text(element->node, &text)))
        return hr;

    /* An example of this is. The Text in the node needs to be a 0 or 1 for a boolean type.
       This applies to changing types (string->bool) or setting a new one
     */

    hr = dt_validate(dt, text);
    SysFreeString(text);

    /* Check all supported types. */
    if (hr == S_OK)
    {
        switch (dt)
        {
        case DT_BIN_BASE64:
        case DT_BIN_HEX:
        case DT_BOOLEAN:
        case DT_CHAR:
        case DT_DATE:
        case DT_DATE_TZ:
        case DT_DATETIME:
        case DT_DATETIME_TZ:
        case DT_FIXED_14_4:
        case DT_FLOAT:
        case DT_I1:
        case DT_I2:
        case DT_I4:
        case DT_I8:
        case DT_INT:
        case DT_NMTOKEN:
        case DT_NMTOKENS:
        case DT_NUMBER:
        case DT_R4:
        case DT_R8:
        case DT_STRING:
        case DT_TIME:
        case DT_TIME_TZ:
        case DT_UI1:
        case DT_UI2:
        case DT_UI4:
        case DT_UI8:
        case DT_URI:
        case DT_UUID:
            {
                IXMLDOMNode *attr = NULL;

                if (SUCCEEDED(hr = create_attribute_node(L"dt:dt", L"urn:schemas-microsoft-com:datatypes",
                        element->node->owner, &attr)))
                {
                    if ((text = SysAllocString(dt_to_bstr(dt))))
                    {
                        hr = IXMLDOMNode_put_text(attr, text);
                        SysFreeString(text);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }

                if (SUCCEEDED(hr))
                    hr = node_set_attribute(element->node, attr, NULL);

                if (attr)
                    IXMLDOMNode_Release(attr);
            }
            break;
        default:
            FIXME("need to handle dt:%s\n", debugstr_dt(dt));
            break;
        }
    }

    return hr;
}

static HRESULT WINAPI domelem_get_xml(IXMLDOMElement *iface, BSTR *p)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_xml(element->node, p);
}

static HRESULT WINAPI domelem_transformNode(IXMLDOMElement *iface, IXMLDOMNode *node, BSTR *p)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p, %p.\n", iface, node, p);

    return node_transform_node(element->node, node, p);
}

static HRESULT WINAPI domelem_selectNodes(IXMLDOMElement *iface, BSTR p, IXMLDOMNodeList **list)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), list);

    return node_select_nodes(element->node, p, list);
}

static HRESULT WINAPI domelem_selectSingleNode(IXMLDOMElement *iface, BSTR p, IXMLDOMNode **node)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), node);

    return node_select_singlenode(element->node, p, node);
}

static HRESULT WINAPI domelem_get_parsed(IXMLDOMElement *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p stub!\n", iface, v);

    *v = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domelem_get_namespaceURI(IXMLDOMElement *iface, BSTR *p)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_namespaceURI(element->node, p);
}

static HRESULT WINAPI domelem_get_prefix(IXMLDOMElement *iface, BSTR *prefix)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p.\n", iface, prefix);

    return node_get_prefix(element->node, prefix);
}

static HRESULT WINAPI domelem_get_baseName(IXMLDOMElement *iface, BSTR *name)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p.\n", iface, name);

    return node_get_base_name(element->node, name);
}

static HRESULT WINAPI domelem_transformNodeToObject(IXMLDOMElement *iface, IXMLDOMNode *node, VARIANT var)
{
    FIXME("%p, %p, %s.\n", iface, node, debugstr_variant(&var));

    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_tagName(IXMLDOMElement *iface, BSTR *p)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_name(element->node, p);
}

static HRESULT WINAPI domelem_getAttribute(IXMLDOMElement *iface, BSTR name, VARIANT *value)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(name), value);

    return node_get_attribute_value(element->node, name, value);
}

static HRESULT WINAPI domelem_setAttribute(IXMLDOMElement *iface, BSTR name, VARIANT value)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_w(name), debugstr_variant(&value));

    return node_set_attribute_value(element->node, name, &value);
}

static HRESULT WINAPI domelem_removeAttribute(IXMLDOMElement *iface, BSTR p)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(p));

    return node_remove_attribute(element->node, p, NULL);
}

static HRESULT WINAPI domelem_getAttributeNode(IXMLDOMElement *iface, BSTR p, IXMLDOMAttribute **node)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), node);

    return node_get_attribute(element->node, p, node);
}

static HRESULT WINAPI domelem_setAttributeNode(IXMLDOMElement *iface, IXMLDOMAttribute *attribute, IXMLDOMAttribute **old)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p, %p\n", iface, attribute, old);

    return node_set_attribute(element->node, (IXMLDOMNode *)attribute, (IXMLDOMNode **)old);
}

static HRESULT WINAPI domelem_removeAttributeNode(IXMLDOMElement *iface,
        IXMLDOMAttribute *attribute, IXMLDOMAttribute **out_attribute)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %p, %p\n", iface, attribute, out_attribute);

    return node_remove_attribute_node(element->node, attribute, out_attribute);
}

static HRESULT WINAPI domelem_getElementsByTagName(IXMLDOMElement *iface, BSTR name, IXMLDOMNodeList **list)
{
    domelem *element = impl_from_IXMLDOMElement(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(name), list);

    return node_get_elements_by_tagname(element->node, name, list);
}

static HRESULT WINAPI domelem_normalize(IXMLDOMElement *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static const struct IXMLDOMElementVtbl domelem_vtbl =
{
    domelem_QueryInterface,
    domelem_AddRef,
    domelem_Release,
    domelem_GetTypeInfoCount,
    domelem_GetTypeInfo,
    domelem_GetIDsOfNames,
    domelem_Invoke,
    domelem_get_nodeName,
    domelem_get_nodeValue,
    domelem_put_nodeValue,
    domelem_get_nodeType,
    domelem_get_parentNode,
    domelem_get_childNodes,
    domelem_get_firstChild,
    domelem_get_lastChild,
    domelem_get_previousSibling,
    domelem_get_nextSibling,
    domelem_get_attributes,
    domelem_insertBefore,
    domelem_replaceChild,
    domelem_removeChild,
    domelem_appendChild,
    domelem_hasChildNodes,
    domelem_get_ownerDocument,
    domelem_cloneNode,
    domelem_get_nodeTypeString,
    domelem_get_text,
    domelem_put_text,
    domelem_get_specified,
    domelem_get_definition,
    domelem_get_nodeTypedValue,
    domelem_put_nodeTypedValue,
    domelem_get_dataType,
    domelem_put_dataType,
    domelem_get_xml,
    domelem_transformNode,
    domelem_selectNodes,
    domelem_selectSingleNode,
    domelem_get_parsed,
    domelem_get_namespaceURI,
    domelem_get_prefix,
    domelem_get_baseName,
    domelem_transformNodeToObject,
    domelem_get_tagName,
    domelem_getAttribute,
    domelem_setAttribute,
    domelem_removeAttribute,
    domelem_getAttributeNode,
    domelem_setAttributeNode,
    domelem_removeAttributeNode,
    domelem_getElementsByTagName,
    domelem_normalize,
};

static HRESULT domelem_get_qualified_item(const struct domnode *node, BSTR name, BSTR uri,
    IXMLDOMNode **item)
{
    TRACE("%p, %s, %s, %p.\n", node, debugstr_w(name), debugstr_w(uri), item);

    if (!name || !item)
        return E_INVALIDARG;

    return node_get_qualified_attribute(node, name, uri, item);
}

static HRESULT domelem_get_named_item(const struct domnode *node, BSTR name, IXMLDOMNode **item)
{
    TRACE("%p, %s, %p.\n", node, debugstr_w(name), item);

    if (!name || !item)
        return E_INVALIDARG;

    return node_get_attribute(node, name, (IXMLDOMAttribute **)item);
}

static HRESULT domelem_set_named_item(struct domnode *node, IXMLDOMNode *newItem, IXMLDOMNode **namedItem)
{
    TRACE("%p, %p, %p.\n", node, newItem, namedItem );

    return node_set_attribute(node, newItem, namedItem);
}

static HRESULT domelem_remove_qualified_item(struct domnode *node, BSTR name, BSTR uri, IXMLDOMNode **item)
{
    TRACE("%p, %s, %s, %p.\n", node, debugstr_w(name), debugstr_w(uri), item);

    if (!name) return E_INVALIDARG;

    return node_remove_qualified_attribute(node, name, uri, item);
}

static HRESULT domelem_remove_named_item(struct domnode *node, BSTR name, IXMLDOMNode **item)
{
    TRACE("%p, %s, %p.\n", node, debugstr_w(name), item);

    return node_remove_attribute(node, name, item);
}

static HRESULT domelem_get_item(struct domnode *node, LONG index, IXMLDOMNode **item)
{
    TRACE("%p, %ld, %p.\n", node, index, item);

    return node_get_attribute_by_index(node, index, item);
}

static HRESULT domelem_get_length(struct domnode *node, LONG *length)
{
    TRACE("%p, %p.\n", node, length);

    if (!length)
        return E_INVALIDARG;

    *length = list_count(&node->attributes);
    return S_OK;
}

static HRESULT domelem_next_node(const struct domnode *node, LONG *iter, IXMLDOMNode **nextNode)
{
    struct domnode *curr = NULL, *n;
    LONG count = *iter;

    TRACE("%p, %ld, %p.\n", node, *iter, nextNode);

    *nextNode = NULL;

    LIST_FOR_EACH_ENTRY(n, &node->attributes, struct domnode, entry)
    {
        curr = n;
        if (!count--) break;
        curr = NULL;
    }

    if (!curr)
        return S_FALSE;

    (*iter)++;
    return create_node(curr, nextNode);
}

static const struct nodemap_funcs domelem_attr_map =
{
    .get_named_item = domelem_get_named_item,
    .set_named_item = domelem_set_named_item,
    .remove_named_item = domelem_remove_named_item,
    .get_item = domelem_get_item,
    .get_length = domelem_get_length,
    .get_qualified_item = domelem_get_qualified_item,
    .remove_qualified_item = domelem_remove_qualified_item,
    .next_node = domelem_next_node,
};

static const tid_t domelem_iface_tids[] =
{
    IXMLDOMElement_tid,
    0
};

static dispex_static_data_t domelem_dispex =
{
    NULL,
    IXMLDOMElement_tid,
    NULL,
    domelem_iface_tids
};

HRESULT create_element(struct domnode *node, IUnknown **obj)
{
    domelem *object;

    *obj = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IXMLDOMElement_iface.lpVtbl = &domelem_vtbl;
    object->refcount = 1;
    object->node = domnode_addref(node);

    init_dispex(&object->dispex, (IUnknown *)&object->IXMLDOMElement_iface, &domelem_dispex);

    *obj = (IUnknown *)&object->IXMLDOMElement_iface;

    return S_OK;
}
