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

static const WCHAR utf16W[] = {'U','T','F','-','1','6',0};
static const WCHAR emptyW[] = {0};
static const WCHAR spaceW[] = {' '};
static const WCHAR quotW[]  = {'\"'};

typedef enum
{
    XmlEncoding_UTF8,
    XmlEncoding_UTF16,
    XmlEncoding_Unknown
} xml_encoding;

typedef enum
{
    OutputBuffer_Native  = 0x001,
    OutputBuffer_Encoded = 0x010,
    OutputBuffer_Both    = 0x100
} output_mode;

typedef enum
{
    MXWriter_BOM = 0,
    MXWriter_DisableEscaping,
    MXWriter_Indent,
    MXWriter_OmitXmlDecl,
    MXWriter_Standalone,
    MXWriter_LastProp
} mxwriter_prop;

typedef struct
{
    char *data;
    unsigned int allocated;
    unsigned int written;
} encoded_buffer;

typedef struct
{
    encoded_buffer utf16;
    encoded_buffer encoded;
    UINT code_page;
} output_buffer;

typedef struct
{
    DispatchEx dispex;
    IMXWriter IMXWriter_iface;
    ISAXContentHandler ISAXContentHandler_iface;
    ISAXLexicalHandler ISAXLexicalHandler_iface;

    LONG ref;
    MSXML_VERSION class_version;

    VARIANT_BOOL props[MXWriter_LastProp];
    BOOL prop_changed;

    BSTR version;

    BSTR encoding; /* exact property value */
    xml_encoding xml_enc;

    /* contains a pending (or not closed yet) element name or NULL if
       we don't have to close */
    BSTR element;

    IStream *dest;
    ULONG dest_written;

    output_buffer *buffer;
} mxwriter;

static xml_encoding parse_encoding_name(const WCHAR *encoding)
{
    static const WCHAR utf8W[]  = {'U','T','F','-','8',0};
    if (!strcmpiW(encoding, utf8W))  return XmlEncoding_UTF8;
    if (!strcmpiW(encoding, utf16W)) return XmlEncoding_UTF16;
    return XmlEncoding_Unknown;
}

static HRESULT init_encoded_buffer(encoded_buffer *buffer)
{
    const int initial_len = 0x2000;
    buffer->data = heap_alloc(initial_len);
    if (!buffer->data) return E_OUTOFMEMORY;

    memset(buffer->data, 0, 4);
    buffer->allocated = initial_len;
    buffer->written = 0;

    return S_OK;
}

static void free_encoded_buffer(encoded_buffer *buffer)
{
    heap_free(buffer->data);
}

static HRESULT get_code_page(xml_encoding encoding, UINT *cp)
{
    switch (encoding)
    {
    case XmlEncoding_UTF8:
        *cp = CP_UTF8;
        break;
    case XmlEncoding_UTF16:
        *cp = ~0;
        break;
    default:
        FIXME("unsupported encoding %d\n", encoding);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT alloc_output_buffer(xml_encoding encoding, output_buffer **buffer)
{
    output_buffer *ret;
    HRESULT hr;

    ret = heap_alloc(sizeof(*ret));
    if (!ret) return E_OUTOFMEMORY;

    hr = get_code_page(encoding, &ret->code_page);
    if (hr != S_OK) {
        heap_free(ret);
        return hr;
    }

    hr = init_encoded_buffer(&ret->utf16);
    if (hr != S_OK) {
        heap_free(ret);
        return hr;
    }

    if (ret->code_page == CP_UTF8) {
        hr = init_encoded_buffer(&ret->encoded);
        if (hr != S_OK) {
            free_encoded_buffer(&ret->utf16);
            heap_free(ret);
            return hr;
        }
    }
    else
        memset(&ret->encoded, 0, sizeof(ret->encoded));

    *buffer = ret;

    return S_OK;
}

static void free_output_buffer(output_buffer *buffer)
{
    free_encoded_buffer(&buffer->encoded);
    free_encoded_buffer(&buffer->utf16);
    heap_free(buffer);
}

static void grow_buffer(encoded_buffer *buffer, int length)
{
    /* grow if needed, plus 4 bytes to be sure null terminator will fit in */
    if (buffer->allocated < buffer->written + length + 4)
    {
        int grown_size = max(2*buffer->allocated, buffer->allocated + length);
        buffer->data = heap_realloc(buffer->data, grown_size);
        buffer->allocated = grown_size;
    }
}

static HRESULT write_output_buffer_mode(output_buffer *buffer, output_mode mode, const WCHAR *data, int len)
{
    int length;
    char *ptr;

    if (mode & (OutputBuffer_Encoded | OutputBuffer_Both)) {
        if (buffer->code_page == CP_UTF8)
        {
            length = WideCharToMultiByte(buffer->code_page, 0, data, len, NULL, 0, NULL, NULL);
            grow_buffer(&buffer->encoded, length);
            ptr = buffer->encoded.data + buffer->encoded.written;
            length = WideCharToMultiByte(buffer->code_page, 0, data, len, ptr, length, NULL, NULL);
            buffer->encoded.written += len == -1 ? length-1 : length;
        }
    }

    if (mode & (OutputBuffer_Native | OutputBuffer_Both)) {
        /* WCHAR data just copied */
        length = len == -1 ? strlenW(data) : len;
        if (length)
        {
            length *= sizeof(WCHAR);

            grow_buffer(&buffer->utf16, length);
            ptr = buffer->utf16.data + buffer->utf16.written;

            memcpy(ptr, data, length);
            buffer->utf16.written += length;
            ptr += length;
            /* null termination */
            memset(ptr, 0, sizeof(WCHAR));
        }
    }

    return S_OK;
}

static HRESULT write_output_buffer(output_buffer *buffer, const WCHAR *data, int len)
{
    return write_output_buffer_mode(buffer, OutputBuffer_Both, data, len);
}

/* frees buffer data, reallocates with a default lengths */
static void close_output_buffer(mxwriter *This)
{
    heap_free(This->buffer->utf16.data);
    heap_free(This->buffer->encoded.data);
    init_encoded_buffer(&This->buffer->utf16);
    init_encoded_buffer(&This->buffer->encoded);
    get_code_page(This->xml_enc, &This->buffer->code_page);
}

/* escapes special characters like:
   '<' -> "&lt;"
   '&' -> "&amp;"
   '"' -> "&quot;"
   '>' -> "&gt;"
*/
static WCHAR *get_escaped_string(const WCHAR *str, int *len)
{
    static const WCHAR ltW[]    = {'&','l','t',';'};
    static const WCHAR ampW[]   = {'&','a','m','p',';'};
    static const WCHAR equotW[] = {'&','q','u','o','t',';'};
    static const WCHAR gtW[]    = {'&','g','t',';'};

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
            memcpy(ptr, equotW, sizeof(equotW));
            ptr += sizeof(equotW)/sizeof(WCHAR);
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

static void write_prolog_buffer(const mxwriter *This)
{
    static const WCHAR versionW[] = {'<','?','x','m','l',' ','v','e','r','s','i','o','n','=','\"'};
    static const WCHAR encodingW[] = {' ','e','n','c','o','d','i','n','g','=','\"'};
    static const WCHAR standaloneW[] = {' ','s','t','a','n','d','a','l','o','n','e','=','\"'};
    static const WCHAR yesW[] = {'y','e','s','\"','?','>'};
    static const WCHAR noW[] = {'n','o','\"','?','>'};
    static const WCHAR crlfW[] = {'\r','\n'};

    /* version */
    write_output_buffer(This->buffer, versionW, sizeof(versionW)/sizeof(WCHAR));
    write_output_buffer(This->buffer, This->version, -1);
    write_output_buffer(This->buffer, quotW, 1);

    /* encoding */
    write_output_buffer(This->buffer, encodingW, sizeof(encodingW)/sizeof(WCHAR));

    /* always write UTF-16 to WCHAR buffer */
    write_output_buffer_mode(This->buffer, OutputBuffer_Native, utf16W, sizeof(utf16W)/sizeof(WCHAR) - 1);
    write_output_buffer_mode(This->buffer, OutputBuffer_Encoded, This->encoding, -1);
    write_output_buffer(This->buffer, quotW, 1);

    /* standalone */
    write_output_buffer(This->buffer, standaloneW, sizeof(standaloneW)/sizeof(WCHAR));
    if (This->props[MXWriter_Standalone] == VARIANT_TRUE)
        write_output_buffer(This->buffer, yesW, sizeof(yesW)/sizeof(WCHAR));
    else
        write_output_buffer(This->buffer, noW, sizeof(noW)/sizeof(WCHAR));

    write_output_buffer(This->buffer, crlfW, sizeof(crlfW)/sizeof(WCHAR));
}

/* Attempts to the write data from the mxwriter's buffer to
 * the destination stream (if there is one).
 */
static HRESULT write_data_to_stream(mxwriter *This)
{
    encoded_buffer *buffer;
    ULONG written = 0;
    HRESULT hr;

    if (!This->dest)
        return S_OK;

    /* The xmlOutputBuffer doesn't copy its contents from its 'buffer' to the
     * 'conv' buffer when UTF8 encoding is used.
     */
    if (This->xml_enc != XmlEncoding_UTF16)
        buffer = &This->buffer->encoded;
    else
        buffer = &This->buffer->utf16;

    if (This->dest_written > buffer->written) {
        ERR("Failed sanity check! Not sure what to do... (%d > %d)\n", This->dest_written, buffer->written);
        return E_FAIL;
    } else if (This->dest_written == buffer->written && This->xml_enc != XmlEncoding_UTF8)
        /* Windows seems to make an empty write call when the encoding is UTF-8 and
         * all the data has been written to the stream. It doesn't seem make this call
         * for any other encodings.
         */
        return S_OK;

    /* Write the current content from the output buffer into 'dest'.
     * TODO: Check what Windows does if the IStream doesn't write all of
     *       the data we give it at once.
     */
    hr = IStream_Write(This->dest, buffer->data+This->dest_written,
                         buffer->written-This->dest_written, &written);
    if (FAILED(hr)) {
        WARN("Failed to write data to IStream (0x%08x)\n", hr);
        return hr;
    }

    This->dest_written += written;
    return hr;
}

/* Newly added element start tag left unclosed cause for empty elements
   we have to close it differently. */
static void close_element_starttag(const mxwriter *This)
{
    static const WCHAR gtW[] = {'>'};
    if (!This->element) return;
    write_output_buffer(This->buffer, gtW, 1);
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
    return write_data_to_stream(This);
}

/* Resets the mxwriter's output buffer by closing it, then creating a new
 * output buffer using the given encoding.
 */
static inline void reset_output_buffer(mxwriter *This)
{
    close_output_buffer(This);
    This->dest_written = 0;
}

static HRESULT writer_set_property(mxwriter *writer, mxwriter_prop property, VARIANT_BOOL value)
{
    writer->props[property] = value;
    writer->prop_changed = TRUE;
    return S_OK;
}

static HRESULT writer_get_property(const mxwriter *writer, mxwriter_prop property, VARIANT_BOOL *value)
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

static inline mxwriter *impl_from_ISAXLexicalHandler(ISAXLexicalHandler *iface)
{
    return CONTAINING_RECORD(iface, mxwriter, ISAXLexicalHandler_iface);
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
    else if ( IsEqualGUID( riid, &IID_ISAXLexicalHandler ) )
    {
        *obj = &This->ISAXLexicalHandler_iface;
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
        free_output_buffer(This->buffer);

        if (This->dest) IStream_Release(This->dest);
        SysFreeString(This->version);
        SysFreeString(This->encoding);

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
        HRESULT hr = flush_output_buffer(This);
        if (FAILED(hr))
            return hr;

        V_VT(dest)   = VT_BSTR;
        V_BSTR(dest) = SysAllocString((WCHAR*)This->buffer->utf16.data);

        return S_OK;
    }
    else
        FIXME("not implemented when stream is set up\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI mxwriter_put_encoding(IMXWriter *iface, BSTR encoding)
{
    mxwriter *This = impl_from_IMXWriter( iface );
    xml_encoding enc;
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_w(encoding));

    enc = parse_encoding_name(encoding);
    if (enc == XmlEncoding_Unknown)
    {
        FIXME("unsupported encoding %s\n", debugstr_w(encoding));
        return E_INVALIDARG;
    }

    hr = flush_output_buffer(This);
    if (FAILED(hr))
        return hr;

    SysReAllocString(&This->encoding, encoding);
    This->xml_enc = enc;

    TRACE("got encoding %d\n", This->xml_enc);
    reset_output_buffer(This);
    return S_OK;
}

static HRESULT WINAPI mxwriter_get_encoding(IMXWriter *iface, BSTR *encoding)
{
    mxwriter *This = impl_from_IMXWriter( iface );

    TRACE("(%p)->(%p)\n", This, encoding);

    if (!encoding) return E_POINTER;

    *encoding = SysAllocString(This->encoding);
    if (!*encoding) return E_OUTOFMEMORY;

    return S_OK;
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
static HRESULT WINAPI SAXContentHandler_QueryInterface(
    ISAXContentHandler *iface,
    REFIID riid,
    void **obj)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    return IMXWriter_QueryInterface(&This->IMXWriter_iface, riid, obj);
}

static ULONG WINAPI SAXContentHandler_AddRef(ISAXContentHandler *iface)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    return IMXWriter_AddRef(&This->IMXWriter_iface);
}

static ULONG WINAPI SAXContentHandler_Release(ISAXContentHandler *iface)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    return IMXWriter_Release(&This->IMXWriter_iface);
}

static HRESULT WINAPI SAXContentHandler_putDocumentLocator(
    ISAXContentHandler *iface,
    ISAXLocator *locator)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    FIXME("(%p)->(%p)\n", This, locator);
    return E_NOTIMPL;
}

static HRESULT WINAPI SAXContentHandler_startDocument(ISAXContentHandler *iface)
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

    write_prolog_buffer(This);

    if (This->dest && This->xml_enc == XmlEncoding_UTF16) {
        static const char utf16BOM[] = {0xff,0xfe};

        if (This->props[MXWriter_BOM] == VARIANT_TRUE)
            /* Windows passes a NULL pointer as the pcbWritten parameter and
             * ignores any error codes returned from this Write call.
             */
            IStream_Write(This->dest, utf16BOM, sizeof(utf16BOM), NULL);
    }

    return S_OK;
}

static HRESULT WINAPI SAXContentHandler_endDocument(ISAXContentHandler *iface)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    TRACE("(%p)\n", This);
    This->prop_changed = FALSE;
    return flush_output_buffer(This);
}

static HRESULT WINAPI SAXContentHandler_startPrefixMapping(
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

static HRESULT WINAPI SAXContentHandler_endPrefixMapping(
    ISAXContentHandler *iface,
    const WCHAR *prefix,
    int nprefix)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    FIXME("(%p)->(%s)\n", This, debugstr_wn(prefix, nprefix));
    return E_NOTIMPL;
}

static HRESULT WINAPI SAXContentHandler_startElement(
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
    static const WCHAR ltW[] = {'<'};

    TRACE("(%p)->(%s %s %s %p)\n", This, debugstr_wn(namespaceUri, nnamespaceUri),
        debugstr_wn(local_name, nlocal_name), debugstr_wn(QName, nQName), attr);

    if ((!namespaceUri || !local_name || !QName) && This->class_version != MSXML6)
        return E_INVALIDARG;

    close_element_starttag(This);
    set_element_name(This, QName ? QName  : emptyW,
                           QName ? nQName : 0);

    write_output_buffer(This->buffer, ltW, 1);
    write_output_buffer(This->buffer, QName, nQName);

    if (attr)
    {
        HRESULT hr;
        INT length;
        INT i;

        hr = ISAXAttributes_getLength(attr, &length);
        if (FAILED(hr)) return hr;

        for (i = 0; i < length; i++)
        {
            static const WCHAR eqqW[] = {'=','\"'};
            const WCHAR *str;
            WCHAR *escaped;
            INT len = 0;

            hr = ISAXAttributes_getQName(attr, i, &str, &len);
            if (FAILED(hr)) return hr;

            /* space separator in front of every attribute */
            write_output_buffer(This->buffer, spaceW, 1);
            write_output_buffer(This->buffer, str, len);

            write_output_buffer(This->buffer, eqqW, 2);

            len = 0;
            hr = ISAXAttributes_getValue(attr, i, &str, &len);
            if (FAILED(hr)) return hr;

            escaped = get_escaped_string(str, &len);
            write_output_buffer(This->buffer, escaped, len);
            heap_free(escaped);

            write_output_buffer(This->buffer, quotW, 1);
        }
    }

    return S_OK;
}

static HRESULT WINAPI SAXContentHandler_endElement(
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
        static const WCHAR closeW[] = {'/','>'};

        write_output_buffer(This->buffer, closeW, 2);
    }
    else
    {
        static const WCHAR closetagW[] = {'<','/'};
        static const WCHAR gtW[] = {'>'};

        write_output_buffer(This->buffer, closetagW, 2);
        write_output_buffer(This->buffer, QName, nQName);
        write_output_buffer(This->buffer, gtW, 1);
    }

    set_element_name(This, NULL, 0);

    return S_OK;
}

static HRESULT WINAPI SAXContentHandler_characters(
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
        write_output_buffer(This->buffer, chars, nchars);

    return S_OK;
}

static HRESULT WINAPI SAXContentHandler_ignorableWhitespace(
    ISAXContentHandler *iface,
    const WCHAR *chars,
    int nchars)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    FIXME("(%p)->(%s)\n", This, debugstr_wn(chars, nchars));
    return E_NOTIMPL;
}

static HRESULT WINAPI SAXContentHandler_processingInstruction(
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

static HRESULT WINAPI SAXContentHandler_skippedEntity(
    ISAXContentHandler *iface,
    const WCHAR *name,
    int nname)
{
    mxwriter *This = impl_from_ISAXContentHandler( iface );
    FIXME("(%p)->(%s)\n", This, debugstr_wn(name, nname));
    return E_NOTIMPL;
}

static const struct ISAXContentHandlerVtbl SAXContentHandlerVtbl =
{
    SAXContentHandler_QueryInterface,
    SAXContentHandler_AddRef,
    SAXContentHandler_Release,
    SAXContentHandler_putDocumentLocator,
    SAXContentHandler_startDocument,
    SAXContentHandler_endDocument,
    SAXContentHandler_startPrefixMapping,
    SAXContentHandler_endPrefixMapping,
    SAXContentHandler_startElement,
    SAXContentHandler_endElement,
    SAXContentHandler_characters,
    SAXContentHandler_ignorableWhitespace,
    SAXContentHandler_processingInstruction,
    SAXContentHandler_skippedEntity
};

/*** ISAXLexicalHandler ***/
static HRESULT WINAPI SAXLexicalHandler_QueryInterface(ISAXLexicalHandler *iface,
    REFIID riid, void **obj)
{
    mxwriter *This = impl_from_ISAXLexicalHandler( iface );
    return IMXWriter_QueryInterface(&This->IMXWriter_iface, riid, obj);
}

static ULONG WINAPI SAXLexicalHandler_AddRef(ISAXLexicalHandler *iface)
{
    mxwriter *This = impl_from_ISAXLexicalHandler( iface );
    return IMXWriter_AddRef(&This->IMXWriter_iface);
}

static ULONG WINAPI SAXLexicalHandler_Release(ISAXLexicalHandler *iface)
{
    mxwriter *This = impl_from_ISAXLexicalHandler( iface );
    return IMXWriter_Release(&This->IMXWriter_iface);
}

static HRESULT WINAPI SAXLexicalHandler_startDTD(ISAXLexicalHandler *iface,
    const WCHAR *name, int name_len, const WCHAR *publicId, int publicId_len,
    const WCHAR *systemId, int systemId_len)
{
    static const WCHAR doctypeW[] = {'<','!','D','O','C','T','Y','P','E',' '};
    static const WCHAR openintW[] = {'[','\r','\n'};

    mxwriter *This = impl_from_ISAXLexicalHandler( iface );

    TRACE("(%p)->(%s %s %s)\n", This, debugstr_wn(name, name_len), debugstr_wn(publicId, publicId_len),
        debugstr_wn(systemId, systemId_len));

    if (!name) return E_INVALIDARG;

    write_output_buffer(This->buffer, doctypeW, sizeof(doctypeW)/sizeof(WCHAR));

    if (*name)
    {
        write_output_buffer(This->buffer, name, name_len);
        write_output_buffer(This->buffer, spaceW, 1);
    }

    if (publicId)
    {
        static const WCHAR publicW[] = {'P','U','B','L','I','C',' '};

        write_output_buffer(This->buffer, publicW, sizeof(publicW)/sizeof(WCHAR));
        write_output_buffer(This->buffer, quotW, 1);
        write_output_buffer(This->buffer, publicId, publicId_len);
        write_output_buffer(This->buffer, quotW, 1);

        if (!systemId) return E_INVALIDARG;

        if (*publicId)
            write_output_buffer(This->buffer, spaceW, 1);

        write_output_buffer(This->buffer, quotW, 1);
        write_output_buffer(This->buffer, systemId, systemId_len);
        write_output_buffer(This->buffer, quotW, 1);

        if (*systemId)
            write_output_buffer(This->buffer, spaceW, 1);
    }
    else if (systemId)
    {
        static const WCHAR systemW[] = {'S','Y','S','T','E','M',' '};

        write_output_buffer(This->buffer, systemW, sizeof(systemW)/sizeof(WCHAR));
        write_output_buffer(This->buffer, quotW, 1);
        write_output_buffer(This->buffer, systemId, systemId_len);
        write_output_buffer(This->buffer, quotW, 1);
        if (*systemId)
            write_output_buffer(This->buffer, spaceW, 1);
    }

    write_output_buffer(This->buffer, openintW, sizeof(openintW)/sizeof(WCHAR));

    return S_OK;
}

static HRESULT WINAPI SAXLexicalHandler_endDTD(ISAXLexicalHandler *iface)
{
    mxwriter *This = impl_from_ISAXLexicalHandler( iface );
    static const WCHAR closedtdW[] = {']','>','\r','\n'};

    TRACE("(%p)\n", This);

    write_output_buffer(This->buffer, closedtdW, sizeof(closedtdW)/sizeof(WCHAR));

    return S_OK;
}

static HRESULT WINAPI SAXLexicalHandler_startEntity(ISAXLexicalHandler *iface, const WCHAR *name, int len)
{
    mxwriter *This = impl_from_ISAXLexicalHandler( iface );
    FIXME("(%p)->(%s): stub\n", This, debugstr_wn(name, len));
    return E_NOTIMPL;
}

static HRESULT WINAPI SAXLexicalHandler_endEntity(ISAXLexicalHandler *iface, const WCHAR *name, int len)
{
    mxwriter *This = impl_from_ISAXLexicalHandler( iface );
    FIXME("(%p)->(%s): stub\n", This, debugstr_wn(name, len));
    return E_NOTIMPL;
}

static HRESULT WINAPI SAXLexicalHandler_startCDATA(ISAXLexicalHandler *iface)
{
    static const WCHAR scdataW[] = {'<','!','[','C','D','A','T','A','['};
    mxwriter *This = impl_from_ISAXLexicalHandler( iface );

    TRACE("(%p)\n", This);

    write_output_buffer(This->buffer, scdataW, sizeof(scdataW)/sizeof(WCHAR));

    return S_OK;
}

static HRESULT WINAPI SAXLexicalHandler_endCDATA(ISAXLexicalHandler *iface)
{
    mxwriter *This = impl_from_ISAXLexicalHandler( iface );
    static const WCHAR ecdataW[] = {']',']','>'};

    TRACE("(%p)\n", This);

    write_output_buffer(This->buffer, ecdataW, sizeof(ecdataW)/sizeof(WCHAR));

    return S_OK;
}

static HRESULT WINAPI SAXLexicalHandler_comment(ISAXLexicalHandler *iface, const WCHAR *chars, int nchars)
{
    mxwriter *This = impl_from_ISAXLexicalHandler( iface );
    static const WCHAR copenW[] = {'<','!','-','-'};
    static const WCHAR ccloseW[] = {'-','-','>','\r','\n'};

    TRACE("(%p)->(%s:%d)\n", This, debugstr_wn(chars, nchars), nchars);

    if (!chars) return E_INVALIDARG;

    close_element_starttag(This);

    write_output_buffer(This->buffer, copenW, sizeof(copenW)/sizeof(WCHAR));
    if (nchars)
        write_output_buffer(This->buffer, chars, nchars);
    write_output_buffer(This->buffer, ccloseW, sizeof(ccloseW)/sizeof(WCHAR));

    return S_OK;
}

static const struct ISAXLexicalHandlerVtbl SAXLexicalHandlerVtbl =
{
    SAXLexicalHandler_QueryInterface,
    SAXLexicalHandler_AddRef,
    SAXLexicalHandler_Release,
    SAXLexicalHandler_startDTD,
    SAXLexicalHandler_endDTD,
    SAXLexicalHandler_startEntity,
    SAXLexicalHandler_endEntity,
    SAXLexicalHandler_startCDATA,
    SAXLexicalHandler_endCDATA,
    SAXLexicalHandler_comment
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
    HRESULT hr;

    TRACE("(%p, %p)\n", outer, ppObj);

    if (outer) FIXME("support aggregation, outer\n");

    This = heap_alloc( sizeof (*This) );
    if(!This)
        return E_OUTOFMEMORY;

    This->IMXWriter_iface.lpVtbl = &MXWriterVtbl;
    This->ISAXContentHandler_iface.lpVtbl = &SAXContentHandlerVtbl;
    This->ISAXLexicalHandler_iface.lpVtbl = &SAXLexicalHandlerVtbl;
    This->ref = 1;
    This->class_version = version;

    This->props[MXWriter_BOM] = VARIANT_TRUE;
    This->props[MXWriter_DisableEscaping] = VARIANT_FALSE;
    This->props[MXWriter_Indent] = VARIANT_FALSE;
    This->props[MXWriter_OmitXmlDecl] = VARIANT_FALSE;
    This->props[MXWriter_Standalone] = VARIANT_FALSE;
    This->prop_changed = FALSE;
    This->encoding = SysAllocString(utf16W);
    This->version  = SysAllocString(version10W);
    This->xml_enc  = XmlEncoding_UTF16;

    This->element = NULL;

    This->dest = NULL;
    This->dest_written = 0;

    hr = alloc_output_buffer(This->xml_enc, &This->buffer);
    if (hr != S_OK) {
        SysFreeString(This->encoding);
        SysFreeString(This->version);
        heap_free(This);
        return hr;
    }

    init_dispex(&This->dispex, (IUnknown*)&This->IMXWriter_iface, &mxwriter_dispex);

    *ppObj = &This->IMXWriter_iface;

    TRACE("returning iface %p\n", *ppObj);

    return S_OK;
}
