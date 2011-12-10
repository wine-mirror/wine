/*
 *    MXWriter implementation
 *
 * Copyright 2011 Nikolay Sivov for CodeWeavers
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

static const WCHAR utf16W[] = {'U','T','F','-','1','6',0};
static const WCHAR utf8W[]  = {'U','T','F','-','8',0};

static const char crlfA[] = "\r\n";
static const WCHAR emptyW[] = {0};

typedef enum
{
    MXWriter_BOM = 0,
    MXWriter_DisableEscaping,
    MXWriter_Indent,
    MXWriter_OmitXmlDecl,
    MXWriter_Standalone,
    MXWriter_LastProp
} MXWRITER_PROPS;

typedef struct
{
    DispatchEx dispex;
    IMXWriter IMXWriter_iface;
    ISAXContentHandler ISAXContentHandler_iface;

    LONG ref;
    MSXML_VERSION class_version;

    VARIANT_BOOL props[MXWriter_LastProp];
    BOOL prop_changed;
    xmlCharEncoding encoding;
    BSTR version;

    /* contains a pending (or not closed yet) element name or NULL if
       we don't have to close */
    BSTR element;

    IStream *dest;
    ULONG   dest_written;

    int decl_count;   /* practically how many times startDocument was called */
    int decl_written; /* byte length of document prolog */

    xmlOutputBufferPtr buffer;
} mxwriter;

static HRESULT bstr_from_xmlCharEncoding(xmlCharEncoding enc, BSTR *encoding)
{
    const char *encodingA;

    if (enc != XML_CHAR_ENCODING_UTF16LE && enc != XML_CHAR_ENCODING_UTF8) {
        FIXME("Unsupported xmlCharEncoding: %d\n", enc);
        *encoding = NULL;
        return E_NOTIMPL;
    }

    encodingA = xmlGetCharEncodingName(enc);
    if (encodingA) {
        DWORD len = MultiByteToWideChar(CP_ACP, 0, encodingA, -1, NULL, 0);
        *encoding = SysAllocStringLen(NULL, len-1);
        if(*encoding)
            MultiByteToWideChar( CP_ACP, 0, encodingA, -1, *encoding, len);
    } else
        *encoding = SysAllocStringLen(NULL, 0);

    return *encoding ? S_OK : E_OUTOFMEMORY;
}

/* escapes special characters like:
   '<' -> "&lt;"
   '&' -> "&amp;"
   '"' -> "&quot;"
   '>' -> "&gt;"
*/
static WCHAR *get_escaped_string(const WCHAR *str, int *len)
{
    static const WCHAR ltW[]   = {'&','l','t',';'};
    static const WCHAR ampW[]  = {'&','a','m','p',';'};
    static const WCHAR quotW[] = {'&','q','u','o','t',';'};
    static const WCHAR gtW[]   = {'&','g','t',';'};

    const int default_alloc = 100;
    const int grow_thresh = 10;
    int p = *len, conv_len;
    WCHAR *ptr, *ret;

    /* default buffer size to something if length is unknown */
    conv_len = *len == -1 ? default_alloc : max(2**len, default_alloc);
    ptr = ret = heap_alloc(conv_len*sizeof(WCHAR));

    while (*str && p)
    {
        if (ptr - ret > conv_len - grow_thresh)
        {
            int written = ptr - ret;
            conv_len *= 2;
            ptr = ret = heap_realloc(ret, conv_len*sizeof(WCHAR));
            ptr += written;
        }

        switch (*str)
        {
        case '<':
            memcpy(ptr, ltW, sizeof(ltW));
            ptr += sizeof(ltW)/sizeof(WCHAR);
            break;
        case '&':
            memcpy(ptr, ampW, sizeof(ampW));
            ptr += sizeof(ampW)/sizeof(WCHAR);
            break;
        case '"':
            memcpy(ptr, quotW, sizeof(quotW));
            ptr += sizeof(quotW)/sizeof(WCHAR);
            break;
        case '>':
            memcpy(ptr, gtW, sizeof(gtW));
            ptr += sizeof(gtW)/sizeof(WCHAR);
            break;
        default:
            *ptr++ = *str;
            break;
        }

        str++;
        if (*len != -1) p--;
    }

    if (*len != -1) *len = ptr-ret;
    *++ptr = 0;

    return ret;
}

/* creates UTF-8 encoded prolog string with specified or store encoding value */
static int write_prolog_buffer(const mxwriter *This, xmlCharEncoding enc, xmlOutputBufferPtr buffer)
{
    static const char version[] = "<?xml version=\"";
    static const char encoding[] = " encoding=\"";
    static const char standalone[] = " standalone=\"";
    static const char yes[] = "yes\"?>";
    static const char no[] = "no\"?>";
    xmlBufferPtr buf;
    xmlChar *s;
    int use;

    if (enc == XML_CHAR_ENCODING_UTF8)
        buf = buffer->buffer;
    else
        buf = buffer->conv;

    use = buf->use;

    /* version */
    xmlOutputBufferWrite(buffer, sizeof(version)-1, version);
    s = xmlchar_from_wchar(This->version);
    xmlOutputBufferWriteString(buffer, (char*)s);
    heap_free(s);
    xmlOutputBufferWrite(buffer, 1, "\"");

    /* encoding */
    xmlOutputBufferWrite(buffer, sizeof(encoding)-1, encoding);
    xmlOutputBufferWriteString(buffer, xmlGetCharEncodingName(enc));
    xmlOutputBufferWrite(buffer, 1, "\"");

    /* standalone */
    xmlOutputBufferWrite(buffer, sizeof(standalone)-1, standalone);
    if (This->props[MXWriter_Standalone] == VARIANT_TRUE)
        xmlOutputBufferWrite(buffer, sizeof(yes)-1, yes);
    else
        xmlOutputBufferWrite(buffer, sizeof(no)-1, no);

    xmlOutputBufferWrite(buffer, sizeof(crlfA)-1, crlfA);
    xmlOutputBufferFlush(buffer);

    return buf->use - use;
}

/* Attempts to the write data from the mxwriter's buffer to
 * the destination stream (if there is one).
 */
static HRESULT write_data_to_stream(mxwriter *This)
{
    xmlBufferPtr buffer;
    ULONG written = 0;
    HRESULT hr;

    if (!This->dest)
        return S_OK;

    /* The xmlOutputBuffer doesn't copy its contents from its 'buffer' to the
     * 'conv' buffer when UTF8 encoding is used.
     */
    if (This->encoding == XML_CHAR_ENCODING_UTF8)
        buffer = This->buffer->buffer;
    else
        buffer = This->buffer->conv;

    if (This->dest_written > buffer->use) {
        ERR("Failed sanity check! Not sure what to do... (%d > %d)\n", This->dest_written, buffer->use);
        return E_FAIL;
    } else if (This->dest_written == buffer->use && This->encoding != XML_CHAR_ENCODING_UTF8)
        /* Windows seems to make an empty write call when the encoding is UTF-8 and
         * all the data has been written to the stream. It doesn't seem make this call
         * for any other encodings.
         */
        return S_OK;

    /* Write the current content from the output buffer into 'dest'.
     * TODO: Check what Windows does if the IStream doesn't write all of
     *       the data we give it at once.
     */
    hr = IStream_Write(This->dest, buffer->content+This->dest_written,
                         buffer->use-This->dest_written, &written);
    if (FAILED(hr)) {
        WARN("Failed to write data to IStream (0x%08x)\n", hr);
        return hr;
    }

    This->dest_written += written;
    return hr;
}

static void write_output_buffer(const mxwriter *This, const char *data, int len)
{
    xmlOutputBufferWrite(This->buffer, len, data);
}

static void write_output_buffer_str(const mxwriter *This, const char *data)
{
    xmlOutputBufferWriteString(This->buffer, data);
}

/* Newly added element start tag left unclosed cause for empty elements
   we have to close it differently. */
static void close_element_starttag(const mxwriter *This)
{
    static const char gt = '>';
    if (!This->element) return;
    write_output_buffer(This, &gt, 1);
}

static void set_element_name(mxwriter *This, const WCHAR *name, int len)
{
    SysFreeString(This->element);
    This->element = name ? SysAllocStringLen(name, len) : NULL;
}

static inline HRESULT flush_output_buffer(mxwriter *This)
{
    close_element_starttag(This);
    set_element_name(This, NULL, 0);
    xmlOutputBufferFlush(This->buffer);
    return write_data_to_stream(This);
}

/* Resets the mxwriter's output buffer by closing it, then creating a new
 * output buffer using the given encoding.
 */
static inline void reset_output_buffer(mxwriter *This)
{
    xmlOutputBufferClose(This->buffer);
    This->buffer = xmlAllocOutputBuffer(xmlGetCharEncodingHandler(This->encoding));
    This->dest_written = 0;
    This->decl_count = 0;
    This->decl_written = 0;
}

static HRESULT writer_set_property(mxwriter *writer, MXWRITER_PROPS property, VARIANT_BOOL value)
{
    writer->props[property] = value;
    writer->prop_changed = TRUE;
    return S_OK;
}

static HRESULT writer_get_property(const mxwriter *writer, MXWRITER_PROPS property, VARIANT_BOOL *value)
{
    if (!value) return E_POINTER;
    *value = writer->props[property];
    return S_OK;
}

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

    *obj = NULL;

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
    else if (dispex_query_interface(&This->dispex, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
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
        /* Windows flushes the buffer when the interface is destroyed. */
        flush_output_buffer(This);

        if (This->dest) IStream_Release(This->dest);
        SysFreeString(This->version);

        xmlOutputBufferClose(This->buffer);
        SysFreeString(This->element);
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI mxwriter_GetTypeInfoCount(IMXWriter *iface, UINT* pctinfo)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI mxwriter_GetTypeInfo(
    IMXWriter *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    mxwriter *This = impl_from_IMXWriter( iface );
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface,
        iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI mxwriter_GetIDsOfNames(
    IMXWriter *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    mxwriter *This = impl_from_IMXWriter( iface );
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI mxwriter_Invoke(
    IMXWriter *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    mxwriter *This = impl_from_IMXWriter( iface );
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI mxwriter_put_output(IMXWriter *iface, VARIANT dest)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&dest));

    hr = flush_output_buffer(This);
    if (FAILED(hr))
        return hr;

    switch (V_VT(&dest))
    {
    case VT_EMPTY:
    {
        if (This->dest) IStream_Release(This->dest);
        This->dest = NULL;
        reset_output_buffer(This);
        break;
    }
    case VT_UNKNOWN:
    {
        IStream *stream;

        hr = IUnknown_QueryInterface(V_UNKNOWN(&dest), &IID_IStream, (void**)&stream);
        if (hr == S_OK)
        {
            /* Recreate the output buffer to make sure it's using the correct encoding. */
            reset_output_buffer(This);

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
        xmlOutputBufferPtr prolog;
        BSTR output, body = NULL;
        xmlBufferPtr buffer;
        WCHAR *ptr;
        HRESULT hr;
        int i;

        hr = flush_output_buffer(This);
        if (FAILED(hr))
            return hr;

        if (This->decl_count)
        {
            prolog = xmlAllocOutputBuffer(xmlGetCharEncodingHandler(xmlParseCharEncoding("UTF-16")));
            write_prolog_buffer(This, xmlParseCharEncoding("UTF-16"), prolog);
        }
        else
            prolog = NULL;

        /* optimize some paticular cases */
        /* 1. no prolog and UTF-8 buffer */
        if (This->encoding == XML_CHAR_ENCODING_UTF8 && !prolog)
        {
            V_VT(dest)   = VT_BSTR;
            V_BSTR(dest) = bstr_from_xmlChar(This->buffer->buffer->content);
            return S_OK;
        }

        /* 2. no prolog and UTF-16 buffer */
        if (!prolog)
        {
            V_VT(dest)   = VT_BSTR;
            V_BSTR(dest) = SysAllocStringLen((const WCHAR*)This->buffer->conv->content,
                This->buffer->conv->use/sizeof(WCHAR));
            return S_OK;
        }

        V_BSTR(dest) = NULL;

        if (This->encoding == XML_CHAR_ENCODING_UTF8)
        {
            buffer = This->buffer->buffer;
            body = bstr_from_xmlChar(buffer->content+This->decl_written);
            ptr = output = SysAllocStringByteLen(NULL, prolog->conv->use*This->decl_count +
                SysStringByteLen(body));
        }
        else
        {
            buffer = This->buffer->conv;
            ptr = output = SysAllocStringByteLen(NULL, prolog->conv->use*This->decl_count +
                buffer->use - This->decl_written);
        }

        /* write prolog part */
        i = This->decl_count;
        while (i--)
        {
            memcpy(ptr, prolog->conv->content, prolog->conv->use);
            ptr += prolog->conv->use/sizeof(WCHAR);
        }
        xmlOutputBufferClose(prolog);

        /* write main part */
        if (body)
        {
            memcpy(ptr, body, SysStringByteLen(body));
            SysFreeString(body);
        }
        else
            memcpy(ptr, buffer->content + This->decl_written, buffer->use-This->decl_written);

        V_VT(dest)   = VT_BSTR;
        V_BSTR(dest) = output;

        return S_OK;
    }
    else
        FIXME("not implemented when stream is set up\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_put_encoding(IMXWriter *iface, BSTR encoding)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%s)\n", This, debugstr_w(encoding));

    /* FIXME: filter all supported encodings */
    if (!strcmpW(encoding, utf16W) || !strcmpW(encoding, utf8W))
    {
        HRESULT hr;
        LPSTR enc;

        hr = flush_output_buffer(This);
        if (FAILED(hr))
            return hr;

        enc = heap_strdupWtoA(encoding);
        if (!enc)
            return E_OUTOFMEMORY;

        This->encoding = xmlParseCharEncoding(enc);
        heap_free(enc);

        TRACE("got encoding %d\n", This->encoding);
        reset_output_buffer(This);
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

    return bstr_from_xmlCharEncoding(This->encoding, encoding);
}

static HRESULT WINAPI mxwriter_put_byteOrderMark(IMXWriter *iface, VARIANT_BOOL value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%d)\n", This, value);
    return writer_set_property(This, MXWriter_BOM, value);
}

static HRESULT WINAPI mxwriter_get_byteOrderMark(IMXWriter *iface, VARIANT_BOOL *value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%p)\n", This, value);
    return writer_get_property(This, MXWriter_BOM, value);
}

static HRESULT WINAPI mxwriter_put_indent(IMXWriter *iface, VARIANT_BOOL value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%d)\n", This, value);
    return writer_set_property(This, MXWriter_Indent, value);
}

static HRESULT WINAPI mxwriter_get_indent(IMXWriter *iface, VARIANT_BOOL *value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%p)\n", This, value);
    return writer_get_property(This, MXWriter_Indent, value);
}

static HRESULT WINAPI mxwriter_put_standalone(IMXWriter *iface, VARIANT_BOOL value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%d)\n", This, value);
    return writer_set_property(This, MXWriter_Standalone, value);
}

static HRESULT WINAPI mxwriter_get_standalone(IMXWriter *iface, VARIANT_BOOL *value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%p)\n", This, value);
    return writer_get_property(This, MXWriter_Standalone, value);
}

static HRESULT WINAPI mxwriter_put_omitXMLDeclaration(IMXWriter *iface, VARIANT_BOOL value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%d)\n", This, value);
    return writer_set_property(This, MXWriter_OmitXmlDecl, value);
}

static HRESULT WINAPI mxwriter_get_omitXMLDeclaration(IMXWriter *iface, VARIANT_BOOL *value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%p)\n", This, value);
    return writer_get_property(This, MXWriter_OmitXmlDecl, value);
}

static HRESULT WINAPI mxwriter_put_version(IMXWriter *iface, BSTR version)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%s)\n", This, debugstr_w(version));

    if (!version) return E_INVALIDARG;

    SysFreeString(This->version);
    This->version = SysAllocString(version);

    return S_OK;
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
    return writer_set_property(This, MXWriter_DisableEscaping, value);
}

static HRESULT WINAPI mxwriter_get_disableOutputEscaping(IMXWriter *iface, VARIANT_BOOL *value)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%p)\n", This, value);
    return writer_get_property(This, MXWriter_DisableEscaping, value);
}

static HRESULT WINAPI mxwriter_flush(IMXWriter *iface)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    TRACE("(%p)\n", This);
    return flush_output_buffer(This);
}

static const struct IMXWriterVtbl MXWriterVtbl =
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

    TRACE("(%p)\n", This);

    /* If properties have been changed since the last "endDocument" call
     * we need to reset the output buffer. If we don't the output buffer
     * could end up with multiple XML documents in it, plus this seems to
     * be how Windows works.
     */
    if (This->prop_changed) {
        reset_output_buffer(This);
        This->prop_changed = FALSE;
    }

    if (This->props[MXWriter_OmitXmlDecl] == VARIANT_TRUE) return S_OK;

    This->decl_count++;
    This->decl_written += write_prolog_buffer(This, This->encoding, This->buffer);

    if (This->dest && This->encoding == XML_CHAR_ENCODING_UTF16LE) {
        static const CHAR utf16BOM[] = {0xff,0xfe};

        if (This->props[MXWriter_BOM] == VARIANT_TRUE)
            /* Windows passes a NULL pointer as the pcbWritten parameter and
             * ignores any error codes returned from this Write call.
             */
            IStream_Write(This->dest, utf16BOM, sizeof(utf16BOM), NULL);
    }

    return S_OK;
}

static HRESULT WINAPI mxwriter_saxcontent_endDocument(ISAXContentHandler *iface)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    TRACE("(%p)\n", This);
    This->prop_changed = FALSE;
    return flush_output_buffer(This);
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

    if ((!namespaceUri || !local_name || !QName) && This->class_version != MSXML6)
        return E_INVALIDARG;

    close_element_starttag(This);
    set_element_name(This, QName ? QName  : emptyW,
                           QName ? nQName : 0);

    write_output_buffer(This, "<", 1);
    s = xmlchar_from_wcharn(QName, nQName);
    write_output_buffer_str(This, (char*)s);
    heap_free(s);

    if (attr)
    {
        HRESULT hr;
        INT length;
        INT i;

        hr = ISAXAttributes_getLength(attr, &length);
        if (FAILED(hr)) return hr;

        for (i = 0; i < length; i++)
        {
            const WCHAR *str;
            WCHAR *escaped;
            INT len = 0;

            hr = ISAXAttributes_getQName(attr, i, &str, &len);
            if (FAILED(hr)) return hr;

            /* space separator in front of every attribute */
            write_output_buffer(This, " ", 1);

            s = xmlchar_from_wcharn(str, len);
            write_output_buffer_str(This, (char*)s);
            heap_free(s);

            write_output_buffer(This, "=\"", 2);

            len = 0;
            hr = ISAXAttributes_getValue(attr, i, &str, &len);
            if (FAILED(hr)) return hr;

            escaped = get_escaped_string(str, &len);
            s = xmlchar_from_wcharn(escaped, len);
            write_output_buffer_str(This, (char*)s);
            heap_free(escaped);
            heap_free(s);

            write_output_buffer(This, "\"", 1);
        }
    }

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

    TRACE("(%p)->(%s:%d %s:%d %s:%d)\n", This, debugstr_wn(namespaceUri, nnamespaceUri), nnamespaceUri,
        debugstr_wn(local_name, nlocal_name), nlocal_name, debugstr_wn(QName, nQName), nQName);

    if ((!namespaceUri || !local_name || !QName) && This->class_version != MSXML6)
        return E_INVALIDARG;

    if (This->element && QName && !strncmpW(This->element, QName, nQName))
    {
        write_output_buffer(This, "/>", 2);
    }
    else
    {
        xmlChar *s = xmlchar_from_wcharn(QName, nQName);

        write_output_buffer(This, "</", 2);
        write_output_buffer_str(This, (char*)s);
        write_output_buffer(This, ">", 1);

        heap_free(s);
    }

    set_element_name(This, NULL, 0);

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

    close_element_starttag(This);
    set_element_name(This, NULL, 0);

    if (nchars)
    {
        xmlChar *s = xmlchar_from_wcharn(chars, nchars);
        write_output_buffer_str(This, (char*)s);
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

static const tid_t mxwriter_iface_tids[] = {
    IMXWriter_tid,
    0
};

static dispex_static_data_t mxwriter_dispex = {
    NULL,
    IMXWriter_tid,
    NULL,
    mxwriter_iface_tids
};

HRESULT MXWriter_create(MSXML_VERSION version, IUnknown *outer, void **ppObj)
{
    static const WCHAR version10W[] = {'1','.','0',0};
    mxwriter *This;

    TRACE("(%p, %p)\n", outer, ppObj);

    if (outer) FIXME("support aggregation, outer\n");

    This = heap_alloc( sizeof (*This) );
    if(!This)
        return E_OUTOFMEMORY;

    This->IMXWriter_iface.lpVtbl = &MXWriterVtbl;
    This->ISAXContentHandler_iface.lpVtbl = &mxwriter_saxcontent_vtbl;
    This->ref = 1;
    This->class_version = version;

    This->props[MXWriter_BOM] = VARIANT_TRUE;
    This->props[MXWriter_DisableEscaping] = VARIANT_FALSE;
    This->props[MXWriter_Indent] = VARIANT_FALSE;
    This->props[MXWriter_OmitXmlDecl] = VARIANT_FALSE;
    This->props[MXWriter_Standalone] = VARIANT_FALSE;
    This->prop_changed = FALSE;
    This->encoding = xmlParseCharEncoding("UTF-16");
    This->version  = SysAllocString(version10W);

    This->element = NULL;

    This->decl_count = 0;
    This->decl_written = 0;

    This->dest = NULL;
    This->dest_written = 0;

    This->buffer = xmlAllocOutputBuffer(xmlGetCharEncodingHandler(This->encoding));

    init_dispex(&This->dispex, (IUnknown*)&This->IMXWriter_iface, &mxwriter_dispex);

    *ppObj = &This->IMXWriter_iface;

    TRACE("returning iface %p\n", *ppObj);

    return S_OK;
}

#else

HRESULT MXWriter_create(MSXML_VERSION version, IUnknown *outer, void **obj)
{
    MESSAGE("This program tried to use a MXXMLWriter object, but\n"
            "libxml2 support was not present at compile time.\n");
    return E_NOTIMPL;
}

#endif /* HAVE_LIBXML2 */
