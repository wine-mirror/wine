/*
 *    SAX Reader implementation
 *
 * Copyright 2008 Alistair Leslie-Hughes
 * Copyright 2008 Piotr Caban
 * Copyright 2025-2026 Nikolay Sivov
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
#include "wininet.h"
#include "urlmon.h"
#include "winreg.h"
#include "shlwapi.h"
#include "winternl.h"

#include "wine/debug.h"

#include "msxml_private.h"

#include "initguid.h"
#include "saxreader_extensions.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

struct stream_wrapper
{
    ISequentialStream ISequentialStream_iface;
    LONG refcount;

    const BYTE *buffer;
    DWORD size;
    DWORD position;
};

static struct stream_wrapper *impl_from_ISequentialStream(ISequentialStream *iface)
{
    return CONTAINING_RECORD(iface, struct stream_wrapper, ISequentialStream_iface);
}

static HRESULT WINAPI stream_wrapper_QueryInterface(ISequentialStream *iface, REFIID riid, void **out)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_ISequentialStream))
    {
        *out = iface;
        ISequentialStream_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI stream_wrapper_AddRef(ISequentialStream *iface)
{
    struct stream_wrapper *stream = impl_from_ISequentialStream(iface);
    ULONG refcount = InterlockedIncrement(&stream->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI stream_wrapper_Release(ISequentialStream *iface)
{
    struct stream_wrapper *stream = impl_from_ISequentialStream(iface);
    ULONG refcount = InterlockedDecrement(&stream->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
        free(stream);

    return refcount;
}

static HRESULT WINAPI stream_wrapper_Read(ISequentialStream *iface, void *buff, ULONG buff_size, ULONG *read_len)
{
    struct stream_wrapper *stream = impl_from_ISequentialStream(iface);
    DWORD size;

    TRACE("%p, %p, %lu, %p.\n", iface, buff, buff_size, read_len);

    if (stream->position >= stream->size)
    {
        if (read_len)
            *read_len = 0;
        return S_FALSE;
    }

    size = stream->size - stream->position;
    if (buff_size < size)
        size = buff_size;

    memmove(buff, stream->buffer + stream->position, size);
    stream->position += size;

    if (read_len)
        *read_len = size;

    return S_OK;
}

static HRESULT WINAPI stream_wrapper_Write(ISequentialStream *iface, const void *buff, ULONG buff_size, ULONG *written)
{
    TRACE("%p, %p, %lu, %p.\n", iface, buff, buff_size, written);

    return E_NOTIMPL;
}

static const ISequentialStreamVtbl stream_wrapper_vtbl =
{
    stream_wrapper_QueryInterface,
    stream_wrapper_AddRef,
    stream_wrapper_Release,
    stream_wrapper_Read,
    stream_wrapper_Write,
};

HRESULT stream_wrapper_create(const void *buffer, DWORD size, ISequentialStream **ret)
{
    struct stream_wrapper *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ISequentialStream_iface.lpVtbl = &stream_wrapper_vtbl;
    object->refcount = 1;
    object->buffer = buffer;
    object->size = size;

    *ret = &object->ISequentialStream_iface;

    return S_OK;
}

enum attribute_type
{
    ATTR_TYPE_INVALID = 0,
    ATTR_TYPE_CDATA,
    ATTR_TYPE_ID,
    ATTR_TYPE_IDREF,
    ATTR_TYPE_IDREFS,
    ATTR_TYPE_ENTITY,
    ATTR_TYPE_ENTITIES,
    ATTR_TYPE_NMTOKEN,
    ATTR_TYPE_NMTOKENS,
    ATTR_TYPE_NOTATION,
    ATTR_TYPE_ENUMERATION,
};

enum attribute_default_type
{
    ATTR_DEF_TYPE_NONE = 0,
    ATTR_DEF_TYPE_REQUIRED,
    ATTR_DEF_TYPE_IMPLIED,
    ATTR_DEF_TYPE_FIXED,
};

struct attribute
{
    BSTR uri;
    BSTR local;
    BSTR qname;
    BSTR value;
    BSTR prefix;
    bool nsdef;
};

struct enumeration
{
    BSTR *values;
    size_t count;
    size_t capacity;
};

struct attlist_attr
{
    struct parsed_name name;
    enum attribute_type type;
    struct enumeration valuelist;
    enum attribute_default_type valuetype;
    BSTR value;
};

struct attlist_decl
{
    struct list entry;
    BSTR name;
    struct attlist_attr *attributes;
    size_t count;
    size_t capacity;
};

enum element_type
{
    ELEMENT_TYPE_UNDEFINED = 0,
    ELEMENT_TYPE_EMPTY,
    ELEMENT_TYPE_ANY,
    ELEMENT_TYPE_MIXED,
    ELEMENT_TYPE_ELEMENT,
};

enum element_content_type
{
    ELEMENT_CONTENT_PCDATA = 1,
    ELEMENT_CONTENT_ELEMENT,
    ELEMENT_CONTENT_SEQ,
    ELEMENT_CONTENT_OR,
};

enum element_content_occurrence
{
    ELEMENT_CONTENT_ONCE = 1,
    ELEMENT_CONTENT_OPT,
    ELEMENT_CONTENT_MULT,
    ELEMENT_CONTENT_PLUS,
};

struct element_content
{
    BSTR name;
    enum element_content_type type;
    enum element_content_occurrence occurrence;
    struct element_content *left;
    struct element_content *right;
    struct element_content *parent;
};

struct element_decl
{
    BSTR name;
    enum element_type type;
    struct element_content *content;
};


struct entity_part
{
    BSTR value;
    bool reference;
};

struct text_position
{
    int line;
    int column;
};

struct entity_decl
{
    struct list entry;
    BSTR name;
    BSTR sysid;
    BSTR value;
    struct
    {
        struct entity_part *parts;
        size_t count;
        size_t capacity;
    } content;
    bool unparsed;

    struct list input_entry;
    bool visited;
    size_t remaining;
    struct text_position position;
};

enum saxreader_feature
{
    FeatureUnknown               = 0x00000000,
    ExhaustiveErrors             = 0x00000001,
    ExternalGeneralEntities      = 0x00000002,
    ExternalParameterEntities    = 0x00000004,
    ForcedResync                 = 0x00000008,
    NamespacePrefixes            = 0x00000010,
    Namespaces                   = 0x00000020,
    ParameterEntities            = 0x00000040,
    PreserveSystemIndentifiers   = 0x00000080,
    ProhibitDTD                  = 0x00000100,
    SchemaValidation             = 0x00000200,
    ServerHttpRequest            = 0x00000400,
    SuppressValidationfatalError = 0x00000800,
    UseInlineSchema              = 0x00001000,
    UseSchemaLocation            = 0x00002000,
    LexicalHandlerParEntities    = 0x00004000,
    NormalizeLineBreaks          = 0x00008000,
    NormalizeAttributeValues     = 0x00010000,
    FeatureForceDWord            = 0xffffffff,
};

struct saxreader_feature_pair
{
    enum saxreader_feature feature;
    const WCHAR *name;
};

static const struct saxreader_feature_pair saxreader_feature_map[] = {
    { ExhaustiveErrors, L"exhaustive-errors" },
    { ExternalGeneralEntities, L"http://xml.org/sax/features/external-general-entities" },
    { ExternalParameterEntities, L"http://xml.org/sax/features/external-parameter-entities" },
    { LexicalHandlerParEntities, L"http://xml.org/sax/features/lexical-handler/parameter-entities" },
    { NamespacePrefixes, L"http://xml.org/sax/features/namespace-prefixes" },
    { Namespaces, L"http://xml.org/sax/features/namespaces" },
    { NormalizeAttributeValues, L"normalize-attribute-values" },
    { NormalizeLineBreaks, L"normalize-line-breaks" },
    { ProhibitDTD, L"prohibit-dtd" },
    { SchemaValidation, L"schema-validation" },
};

static enum saxreader_feature get_saxreader_feature(const WCHAR *name)
{
    int min, max, n, c;

    min = 0;
    max = ARRAY_SIZE(saxreader_feature_map) - 1;

    while (min <= max)
    {
        n = (min+max)/2;

        c = wcscmp(saxreader_feature_map[n].name, name);
        if (!c)
            return saxreader_feature_map[n].feature;

        if (c > 0)
            max = n-1;
        else
            min = n+1;
    }

    return FeatureUnknown;
}

static const WCHAR empty_str;

typedef struct
{
    BSTR prefix;
    BSTR uri;
} ns;

typedef struct
{
    struct list entry;
    BSTR prefix;
    BSTR local;
    BSTR qname;
    ns *ns; /* namespaces defined in this particular element */
    int ns_count;
} element_entry;

enum saxhandler_type
{
    SAXContentHandler = 0,
    SAXDeclHandler,
    SAXDTDHandler,
    SAXEntityResolver,
    SAXErrorHandler,
    SAXLexicalHandler,
    SAXExtensionHandler,
    SAXHandler_Last
};

struct saxanyhandler_iface
{
    IUnknown *handler;
    IUnknown *vbhandler;
};

struct saxcontenthandler_iface
{
    ISAXContentHandler *handler;
    IVBSAXContentHandler *vbhandler;
};

struct saxextensionhandler_iface
{
    ISAXExtensionHandler *handler;
    IDispatch *vbhandler;
};

struct saxerrorhandler_iface
{
    ISAXErrorHandler *handler;
    IVBSAXErrorHandler *vbhandler;
};

struct saxlexicalhandler_iface
{
    ISAXLexicalHandler *handler;
    IVBSAXLexicalHandler *vbhandler;
};

struct saxdtdhandler_iface
{
    ISAXDTDHandler *handler;
    IVBSAXDTDHandler *vbhandler;
};

struct saxdeclhandler_iface
{
    ISAXDeclHandler *handler;
    IVBSAXDeclHandler *vbhandler;
};

struct saxentityresolver_iface
{
    ISAXEntityResolver *handler;
    IVBSAXEntityResolver *vbhandler;
};

struct saxhandler_iface
{
    union
    {
        struct saxcontenthandler_iface content;
        struct saxentityresolver_iface entityresolver;
        struct saxerrorhandler_iface error;
        struct saxlexicalhandler_iface lexical;
        struct saxdtdhandler_iface dtd;
        struct saxdeclhandler_iface decl;
        struct saxextensionhandler_iface extension;
        struct saxanyhandler_iface anyhandler;
    } u;
};

struct saxreader
{
    DispatchEx dispex;
    IVBSAXXMLReader IVBSAXXMLReader_iface;
    ISAXXMLReader ISAXXMLReader_iface;
    ISAXXMLReaderExtension ISAXXMLReaderExtension_iface;
    LONG refcount;

    struct saxhandler_iface saxhandlers[SAXHandler_Last];
    BOOL isParsing;
    enum saxreader_feature features;
    BSTR xmldecl_version;
    BSTR xmldecl_standalone;
    BSTR xmldecl_encoding;
    int max_xml_size;
    int max_element_depth;
    BSTR empty_bstr;
    MSXML_VERSION version;
};

static HRESULT saxreader_put_handler(struct saxreader *reader, enum saxhandler_type type, void *ptr, bool vb)
{
    struct saxanyhandler_iface *iface = &reader->saxhandlers[type].u.anyhandler;
    IUnknown *unk = (IUnknown *)ptr;

    if (unk)
        IUnknown_AddRef(unk);

    if ((vb && iface->vbhandler) || (!vb && iface->handler))
        IUnknown_Release(vb ? iface->vbhandler : iface->handler);

    if (vb)
        iface->vbhandler = unk;
    else
        iface->handler = unk;

    return S_OK;
}

static HRESULT saxreader_get_handler(const struct saxreader *reader, enum saxhandler_type type, bool vb, void **ret)
{
    const struct saxanyhandler_iface *iface = &reader->saxhandlers[type].u.anyhandler;

    if (!ret) return E_POINTER;

    if ((vb && iface->vbhandler) || (!vb && iface->handler))
    {
        if (vb)
            IUnknown_AddRef(iface->vbhandler);
        else
            IUnknown_AddRef(iface->handler);
    }

    *ret = vb ? iface->vbhandler : iface->handler;

    return S_OK;
}

static struct saxcontenthandler_iface *saxreader_get_contenthandler(struct saxreader *reader)
{
    return &reader->saxhandlers[SAXContentHandler].u.content;
}

static struct saxextensionhandler_iface *saxreader_get_extensionhandler(struct saxreader *reader)
{
    return &reader->saxhandlers[SAXExtensionHandler].u.extension;
}

static struct saxerrorhandler_iface *saxreader_get_errorhandler(struct saxreader *reader)
{
    return &reader->saxhandlers[SAXErrorHandler].u.error;
}

static struct saxlexicalhandler_iface *saxreader_get_lexicalhandler(struct saxreader *reader)
{
    return &reader->saxhandlers[SAXLexicalHandler].u.lexical;
}

static struct saxdtdhandler_iface *saxreader_get_dtdhandler(struct saxreader *reader)
{
    return &reader->saxhandlers[SAXDTDHandler].u.dtd;
}

static struct saxdeclhandler_iface *saxreader_get_declhandler(struct saxreader *reader)
{
    return &reader->saxhandlers[SAXDeclHandler].u.decl;
}

struct encoded_buffer
{
    char *data;
    size_t cur;
    size_t allocated;
    size_t written;
};

static void encoded_buffer_cleanup(struct encoded_buffer *buffer)
{
    free(buffer->data);
    memset(buffer, 0, sizeof(*buffer));
}

enum xmlencoding
{
    XML_ENCODING_UNKNOWN = 0,
    XML_ENCODING_UTF16LE,
    XML_ENCODING_UTF16BE,
    XML_ENCODING_UCS4BE,
    XML_ENCODING_UCS4LE,
    XML_ENCODING_UTF8,
    XML_ENCODING_LAST = XML_ENCODING_UTF8,
    XML_ENCODING_FORCE_DWORD = 0xffffffff,
};

static const char *saxreader_get_encoding_name(enum xmlencoding encoding)
{
    static const char *names[] =
    {
        [XML_ENCODING_UNKNOWN] = "unknown",
        [XML_ENCODING_UTF16LE] = "utf16le",
        [XML_ENCODING_UTF16BE] = "utf16be",
        [XML_ENCODING_UCS4BE] = "ucs4be",
        [XML_ENCODING_UCS4LE] = "ucs4le",
        [XML_ENCODING_UTF8] = "utf8",
    };
    if (encoding > XML_ENCODING_LAST) return "<out-of-bounds>";
    return names[encoding];
}

static bool saxreader_get_encoding_codepage(const WCHAR *name, UINT *codepage)
{
    static const struct
    {
        const WCHAR *name;
        UINT codepage;
    }
    encodings[] =
    {
        { L"shift_jis", 932 },
        { L"shift-jis", 932 },
        { L"gbk", 936 },
        { L"gb2312", 936 },
        { L"us-ascii", 20127 },
        { L"iso8859-1", 28591 },
        { L"iso-8859-1", 28591 },
        { L"iso-8859-2", 28592 },
        { L"iso-8859-3", 28593 },
        { L"iso-8859-4", 28594 },
        { L"iso-8859-5", 28595 },
        { L"iso-8859-6", 28596 },
        { L"iso-8859-7", 28597 },
        { L"iso-8859-8", 28598 },
        { L"iso-8859-9", 28599 },
        { L"windows-1250", 1250 },
        { L"windows-1251", 1251 },
        { L"windows-1252", 1252 },
        { L"windows-1253", 1253 },
        { L"windows-1254", 1254 },
        { L"windows-1255", 1255 },
        { L"windows-1256", 1256 },
        { L"windows-1257", 1257 },
        { L"windows-1258", 1258 },
    };

    for (int i = 0; i < ARRAYSIZE(encodings); ++i)
    {
        if (!wcsicmp(name, encodings[i].name))
        {
            *codepage = encodings[i].codepage;
            return true;
        }
    }

    return false;
}

static bool saxreader_can_ignore_encoding(const WCHAR *encoding)
{
    static const WCHAR *encodings[] =
    {
        L"utf-16", L"utf-8"
    };

    for (int i = 0; i < ARRAYSIZE(encodings); ++i)
    {
        if (!wcsicmp(encoding, encodings[i]))
            return true;
    }

    return false;
}

typedef int (*p_converter)(UINT cp, const char *src, int src_size, WCHAR *buffer, int buffer_length);

static int convert_mbtowc(UINT cp, const char *src, int src_size, WCHAR *buffer, int buffer_length)
{
    return MultiByteToWideChar(cp, 0, src, src_size, buffer, buffer_length);
}

static int convert_utf16be(UINT cp, const char *src, int src_size, WCHAR *buffer, int buffer_length)
{
    size_t size = min(src_size / 2, buffer_length);

    if (!buffer) return src_size / 2;

    for (int i = 0; i < size; ++i)
        buffer[i] = src[i + 1] | src[i];

    return size;
}

static unsigned int saxreader_convert_u32_to_u16(UINT32 ch, WCHAR *out)
{
    if (ch < 0x10000)
    {
        out[0] = ch;
        out[1] = 0;
        out[2] = 0;
        return 1;
    }
    else
    {
        out[0] = ((ch - 0x10000) >> 10) + 0xd800;
        out[1] = ((ch - 0x10000) & 0x3ff) + 0xdc00;
        out[2] = 0;
        return 2;
    }
}

static int convert_ucs4le(UINT cp, const char *src, int src_size, WCHAR *buffer, int buffer_length)
{
    const UINT32 *u = (const UINT32 *)src;
    int size = 0, len;
    WCHAR *p = buffer;

    if (src_size % 4) return 0;

    if (!buffer)
    {
        for (int i = 0; i < src_size / 4; ++i)
        {
            ++size;
            if (u[i] > 0xffff) ++size;
        }

        return size;
    }

    while (buffer_length && src_size)
    {
        if (*u > 0xffff && buffer_length < 2) break;
        len = saxreader_convert_u32_to_u16(*u, buffer);
        buffer_length -= len;
        buffer += len;
        ++u;
    }

    return buffer - p;
}

static int convert_ucs4be(UINT cp, const char *src, int src_size, WCHAR *buffer, int buffer_length)
{
    const UINT32 *u = (const UINT32 *)src;
    int size = 0, len;
    WCHAR *p = buffer;
    UINT32 _u;

    if (src_size % 4) return 0;

    if (!buffer)
    {
        for (int i = 0; i < src_size / 4; ++i)
        {
            ++size;
            if (u[i] & 0xffff) ++size;
        }

        return size;
    }

    while (buffer_length && src_size)
    {
        _u = RtlUlongByteSwap(*u);
        if (_u > 0xffff && buffer_length < 2) break;
        len = saxreader_convert_u32_to_u16(_u, buffer);
        buffer_length -= len;
        buffer += len;
        ++u;
    }

    return buffer - p;
}

static p_converter convert_get_converter(enum xmlencoding encoding)
{
    switch (encoding)
    {
        case XML_ENCODING_UCS4BE:
            return convert_ucs4be;
        case XML_ENCODING_UCS4LE:
            return convert_ucs4le;
        case XML_ENCODING_UTF16BE:
            return convert_utf16be;
        case XML_ENCODING_UTF8:
            return convert_mbtowc;
        case XML_ENCODING_UTF16LE:
        default:
            ;
    }

    return NULL;
}

static UINT convert_get_codepage(enum xmlencoding encoding)
{
    switch (encoding)
    {
        case XML_ENCODING_UTF16LE:
            return 1200;
        case XML_ENCODING_UTF8:
            return CP_UTF8;
        default:
            ;
    }

    return 0;
}

/* Get a rough estimate on raw byte size, to produce output of 'size' WCHARs.
   Estimate could off in either direction, it should never be an issue if requested
   chunk size is always larger than reasonable lookahead window used to detect next markup pattern. */
static ULONG convert_estimate_raw_size(enum xmlencoding encoding, ULONG size)
{
    switch (encoding)
    {
        case XML_ENCODING_UTF16LE:
        case XML_ENCODING_UTF16BE:
            return size * 2;
        case XML_ENCODING_UTF8:
            return size * 3;
        case XML_ENCODING_UCS4BE:
        case XML_ENCODING_UCS4LE:
            return size * 4;
        default:
            return 0;
    }
}

struct input_buffer
{
    struct encoded_buffer utf16;
    struct encoded_buffer raw;
    enum xmlencoding encoding;
    bool switched_encoding;
    p_converter converter;
    UINT code_page;
    struct text_position position;
    size_t consumed;
    size_t raw_size;
    bool last_cr;

    unsigned int chunk_size;
    struct list entities;
};

static bool is_mb_codepage(UINT codepage)
{
    return codepage == 932 || codepage == 936;
}

static size_t convert_get_raw_length(struct input_buffer *buffer)
{
    const struct encoded_buffer *raw = &buffer->raw;
    size_t size = raw->written;

    if (buffer->encoding == XML_ENCODING_UTF8)
    {
        if (buffer->code_page == CP_UTF8)
        {
            /* Incomplete single byte char, look for a start byte of multibyte char. */
            if (raw->data[size-1] & 0x80)
            {
                while (--size && !(raw->data[size] & 0xc0))
                    ;
            }
        }
        else if (is_mb_codepage(buffer->code_page))
        {
            /* Attempt to skip incomplete character */
            if (size > 2
                   && IsDBCSLeadByteEx(buffer->code_page, raw->data[size-1])
                   && !IsDBCSLeadByteEx(buffer->code_page, raw->data[size-2]))
            {
                --size;
            }
        }
    }

    return size - raw->cur;
}

enum saxreader_state
{
    SAX_PARSER_START = 0,
    SAX_PARSER_MISC,
    SAX_PARSER_PI,
    SAX_PARSER_COMMENT,
    SAX_PARSER_START_TAG,
    SAX_PARSER_CONTENT,
    SAX_PARSER_CDATA,
    SAX_PARSER_END_TAG,
    SAX_PARSER_EPILOG,
    SAX_PARSER_XML_DECL,
    SAX_PARSER_EOF,
};

struct string_buffer
{
    WCHAR *data;
    size_t count;
    size_t capacity;
};

struct saxlocator
{
    IVBSAXLocator IVBSAXLocator_iface;
    ISAXLocator ISAXLocator_iface;
    IVBSAXAttributes IVBSAXAttributes_iface;
    ISAXAttributes ISAXAttributes_iface;
    LONG refcount;
    struct saxreader *saxreader;
    HRESULT ret;
    int line;
    int column;
    bool vbInterface;
    struct list elements;
    enum saxreader_state state;
    int depth;

    ISequentialStream *stream;
    bool eos;

    BSTR xmlns_uri;
    BSTR xml_uri;
    BSTR null_uri;
    struct
    {
        struct attribute *entries;
        size_t count;
        size_t capacity;
        size_t *map;
    } attributes;

    struct
    {
        struct list attr_decls;
        struct list entities;
        BSTR typenames[ATTR_TYPE_NMTOKENS + 1];
        BSTR valuenames[ATTR_DEF_TYPE_FIXED + 1];
    } dtd;

    bool collect;
    struct string_buffer scratch;
    struct input_buffer buffer;
    struct text_position start_document_position;
    HRESULT status;
};

static inline struct saxreader *impl_from_IVBSAXXMLReader(IVBSAXXMLReader *iface)
{
    return CONTAINING_RECORD(iface, struct saxreader, IVBSAXXMLReader_iface);
}

static inline struct saxreader *impl_from_ISAXXMLReader(ISAXXMLReader *iface)
{
    return CONTAINING_RECORD(iface, struct saxreader, ISAXXMLReader_iface);
}

static inline struct saxreader *impl_from_ISAXXMLReaderExtension(ISAXXMLReaderExtension *iface)
{
    return CONTAINING_RECORD(iface, struct saxreader, ISAXXMLReaderExtension_iface);
}

static inline struct saxlocator *impl_from_IVBSAXLocator(IVBSAXLocator *iface)
{
    return CONTAINING_RECORD(iface, struct saxlocator, IVBSAXLocator_iface);
}

static inline struct saxlocator *impl_from_ISAXLocator(ISAXLocator *iface)
{
    return CONTAINING_RECORD(iface, struct saxlocator, ISAXLocator_iface);
}

static inline struct saxlocator *impl_from_IVBSAXAttributes(IVBSAXAttributes *iface)
{
    return CONTAINING_RECORD(iface, struct saxlocator, IVBSAXAttributes_iface);
}

static inline struct saxlocator *impl_from_ISAXAttributes(ISAXAttributes *iface)
{
    return CONTAINING_RECORD(iface, struct saxlocator, ISAXAttributes_iface);
}

static inline bool saxreader_has_handler(const struct saxlocator *locator, enum saxhandler_type type)
{
    struct saxanyhandler_iface *iface = &locator->saxreader->saxhandlers[type].u.anyhandler;
    return (locator->vbInterface && iface->vbhandler) || (!locator->vbInterface && iface->handler);
}

static inline HRESULT set_feature_value(struct saxreader *reader, enum saxreader_feature feature, VARIANT_BOOL value)
{
    /* handling of non-VARIANT_* values is version dependent */
    if ((reader->version <  MSXML4) && (value != VARIANT_TRUE))
        value = VARIANT_FALSE;
    if ((reader->version >= MSXML4) && (value != VARIANT_FALSE))
        value = VARIANT_TRUE;

    if (value == VARIANT_TRUE)
        reader->features |=  feature;
    else
        reader->features &= ~feature;

    return S_OK;
}

static inline HRESULT get_feature_value(const struct saxreader *reader, enum saxreader_feature feature, VARIANT_BOOL *value)
{
    *value = reader->features & feature ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static void saxreader_set_error(struct saxlocator *locator, HRESULT status)
{
    if (locator->status == S_OK)
        locator->status = status;
}

static BSTR saxreader_alloc_string(struct saxlocator *locator, const WCHAR *str)
{
    BSTR ret;

    if (!str)
        return NULL;

    if (!(ret = SysAllocString(str)))
        saxreader_set_error(locator, E_OUTOFMEMORY);

    return ret;
}

static BSTR saxreader_alloc_stringlen(struct saxlocator *locator, const WCHAR *str, UINT len)
{
    BSTR ret;

    if (!(ret = SysAllocStringLen(str, len)))
        saxreader_set_error(locator, E_OUTOFMEMORY);

    return ret;
}

static void saxreader_ensure_dtd_typenames(struct saxlocator *locator)
{
    if (locator->dtd.typenames[ATTR_TYPE_CDATA])
        return;

    locator->dtd.typenames[ATTR_TYPE_CDATA] = saxreader_alloc_string(locator, L"CDATA");
    locator->dtd.typenames[ATTR_TYPE_ID] = saxreader_alloc_string(locator, L"ID");
    locator->dtd.typenames[ATTR_TYPE_IDREF] = saxreader_alloc_string(locator, L"IDREF");
    locator->dtd.typenames[ATTR_TYPE_IDREFS] = saxreader_alloc_string(locator, L"IDREFS");
    locator->dtd.typenames[ATTR_TYPE_ENTITY] = saxreader_alloc_string(locator, L"ENTITY");
    locator->dtd.typenames[ATTR_TYPE_ENTITIES] = saxreader_alloc_string(locator, L"ENTITIES");
    locator->dtd.typenames[ATTR_TYPE_NMTOKEN] = saxreader_alloc_string(locator, L"NMTOKEN");
    locator->dtd.typenames[ATTR_TYPE_NMTOKENS] = saxreader_alloc_string(locator, L"NMTOKENS");

    locator->dtd.valuenames[ATTR_DEF_TYPE_REQUIRED] = saxreader_alloc_string(locator, L"#REQUIRED");
    locator->dtd.valuenames[ATTR_DEF_TYPE_IMPLIED] = saxreader_alloc_string(locator, L"#IMPLIED");
    locator->dtd.valuenames[ATTR_DEF_TYPE_FIXED] = saxreader_alloc_string(locator, L"#FIXED");
}

static bool is_namespaces_enabled(const struct saxreader *reader)
{
    return (reader->version < MSXML4) || (reader->features & Namespaces);
}

static void free_element_entry(element_entry *element)
{
    int i;

    for (i=0; i<element->ns_count;i++)
    {
        SysFreeString(element->ns[i].prefix);
        SysFreeString(element->ns[i].uri);
    }

    SysFreeString(element->prefix);
    SysFreeString(element->local);
    SysFreeString(element->qname);

    free(element->ns);
    free(element);
}

static HRESULT WINAPI ivbsaxattributes_QueryInterface(IVBSAXAttributes *iface, REFIID riid, void **obj)
{
    struct saxlocator *locator = impl_from_IVBSAXAttributes(iface);
    return IVBSAXLocator_QueryInterface(&locator->IVBSAXLocator_iface, riid, obj);
}

static ULONG WINAPI ivbsaxattributes_AddRef(IVBSAXAttributes* iface)
{
    struct saxlocator *locator = impl_from_IVBSAXAttributes(iface);
    return IVBSAXLocator_AddRef(&locator->IVBSAXLocator_iface);
}

static ULONG WINAPI ivbsaxattributes_Release(IVBSAXAttributes* iface)
{
    struct saxlocator *locator = impl_from_IVBSAXAttributes(iface);
    return IVBSAXLocator_Release(&locator->IVBSAXLocator_iface);
}

static HRESULT WINAPI ivbsaxattributes_GetTypeInfoCount(IVBSAXAttributes *iface, UINT *count)
{
    TRACE("%p, %p.\n", iface, count);

    *count = 1;

    return S_OK;
}

static HRESULT WINAPI ivbsaxattributes_GetTypeInfo(IVBSAXAttributes *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    TRACE("%p, %u, %lx, %p.\n", iface, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IVBSAXAttributes_tid, ppTInfo);
}

static HRESULT WINAPI ivbsaxattributes_GetIDsOfNames(IVBSAXAttributes *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %s, %p, %u, %lx %p.\n", iface, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IVBSAXAttributes_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI ivbsaxattributes_Invoke(IVBSAXAttributes *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD flags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *ei, UINT *arg_err)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %ld, %s, %lx, %d, %p, %p, %p, %p.\n", iface, dispIdMember, debugstr_guid(riid),
          lcid, flags, pDispParams, pVarResult, ei, arg_err);

    hr = get_typeinfo(IVBSAXAttributes_tid, &typeinfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, flags, pDispParams, pVarResult, ei, arg_err);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI ivbsaxattributes_get_length(IVBSAXAttributes *iface, int *length)
{
    struct saxlocator *locator = impl_from_IVBSAXAttributes(iface);

    TRACE("%p, %p.\n", iface, length);

    *length = locator->attributes.count;

    return S_OK;
}

static const struct attribute *saxlocator_get_attribute(const struct saxlocator *locator, int index)
{
    if (index >= locator->attributes.count || index < 0) return NULL;
    if (locator->attributes.map)
        return &locator->attributes.entries[locator->attributes.map[index]];
    return &locator->attributes.entries[index];
}

static const struct attribute *saxlocator_get_attribute_by_name(const struct saxlocator *locator,
        const WCHAR *uri, int uri_length, const WCHAR *local, int local_length, int *index)
{
    const struct attribute *attr;

    for (int i = 0; i < locator->attributes.count; ++i)
    {
        attr = saxlocator_get_attribute(locator, i);

        if (uri_length != SysStringLen(attr->uri) || local_length != SysStringLen(attr->local))
            continue;

        if (uri_length && memcmp(uri, attr->uri, sizeof(WCHAR) * uri_length))
            continue;

        if (local_length && memcmp(local, attr->local, sizeof(WCHAR) * local_length))
            continue;

        *index = i;
        return attr;
    }

    return NULL;
}

static const struct attribute *saxlocator_get_attribute_by_qname(const struct saxlocator *locator, const WCHAR *name,
        int length, int *index)
{
    const struct attribute *attr;

    for (int i = 0; i < locator->attributes.count; ++i)
    {
        attr = saxlocator_get_attribute(locator, i);

        if (length != SysStringLen(attr->qname)) continue;
        if (memcmp(name, attr->qname, sizeof(WCHAR) * length)) continue;

        *index = i;
        return attr;
    }

    return NULL;
}

static HRESULT WINAPI ivbsaxattributes_getURI(IVBSAXAttributes *iface, int index, BSTR *uri)
{
    struct saxlocator *locator = impl_from_IVBSAXAttributes(iface);
    const struct attribute *attr;

    TRACE("%p, %d, %p.\n", iface, index, uri);

    if (!uri)
        return E_POINTER;

    *uri = NULL;

    if (!(attr = saxlocator_get_attribute(locator, index)))
        return E_INVALIDARG;

    return return_bstrn(attr->uri, SysStringLen(attr->uri), uri);
}

static HRESULT WINAPI ivbsaxattributes_getLocalName(IVBSAXAttributes *iface, int index, BSTR *name)
{
    struct saxlocator *locator = impl_from_IVBSAXAttributes(iface);
    const struct attribute *attr;

    TRACE("%p, %d, %p.\n", iface, index, name);

    if (!name)
        return E_POINTER;

    *name = NULL;

    if (!(attr = saxlocator_get_attribute(locator, index)))
        return E_INVALIDARG;

    return return_bstrn(attr->local, SysStringLen(attr->local), name);
}

static HRESULT WINAPI ivbsaxattributes_getQName(IVBSAXAttributes *iface, int index, BSTR *name)
{
    struct saxlocator *locator = impl_from_IVBSAXAttributes(iface);
    const struct attribute *attr;

    TRACE("%p, %d, %p.\n", iface, index, name);

    if (!name)
        return E_POINTER;

    *name = NULL;

    if (!(attr = saxlocator_get_attribute(locator, index)))
        return E_INVALIDARG;

    return return_bstrn(attr->qname, SysStringLen(attr->qname), name);
}

static HRESULT WINAPI ivbsaxattributes_getIndexFromName(IVBSAXAttributes *iface, BSTR uri,
        BSTR name, int *index)
{
    struct saxlocator *locator = impl_from_IVBSAXAttributes(iface);
    const struct attribute *attr;

    attr = saxlocator_get_attribute_by_name(locator, uri, SysStringLen(uri), name, SysStringLen(name), index);

    return attr ? S_OK : E_INVALIDARG;
}

static HRESULT WINAPI ivbsaxattributes_getIndexFromQName(IVBSAXAttributes *iface, BSTR name, int *index)
{
    struct saxlocator *locator = impl_from_IVBSAXAttributes(iface);
    const struct attribute *attr;

    if (!name || !index) return E_POINTER;

    if (!(attr = saxlocator_get_attribute_by_qname(locator, name, SysStringLen(name), index)))
        return E_INVALIDARG;

    return S_OK;
}

static HRESULT WINAPI ivbsaxattributes_getType(IVBSAXAttributes *iface, int index, BSTR *type)
{
    FIXME("%p, %d, %p stub\n", iface, index, type);

    if (!type)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI ivbsaxattributes_getTypeFromName(IVBSAXAttributes *iface, BSTR uri, BSTR name, BSTR *type)
{
    FIXME("%p, %s, %s, %p stub\n", iface, debugstr_w(uri), debugstr_w(name), type);

    if (!type)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI ivbsaxattributes_getTypeFromQName(IVBSAXAttributes *iface, BSTR name, BSTR *type)
{
    FIXME("%p, %s, %p stub\n", iface, debugstr_w(name), type);

    if (!type)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI ivbsaxattributes_getValue(IVBSAXAttributes *iface, int index, BSTR *value)
{
    struct saxlocator *locator = impl_from_IVBSAXAttributes(iface);
    const struct attribute *attr;

    TRACE("%p, %d, %p.\n", iface, index, value);

    if (!value)
        return E_POINTER;

    *value = NULL;

    if (!(attr = saxlocator_get_attribute(locator, index)))
        return E_INVALIDARG;

    return return_bstrn(attr->value, SysStringLen(attr->value), value);
}

static HRESULT WINAPI ivbsaxattributes_getValueFromName(IVBSAXAttributes *iface, BSTR uri, BSTR name, BSTR *value)
{
    struct saxlocator *locator = impl_from_IVBSAXAttributes(iface);
    const struct attribute *attr;
    int index;

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_w(uri), debugstr_w(name), value);

    if (!value)
        return E_POINTER;

    *value = NULL;

    if (!(attr = saxlocator_get_attribute_by_name(locator, uri, SysStringLen(uri), name, SysStringLen(name), &index)))
        return E_INVALIDARG;

    return return_bstrn(attr->value, SysStringLen(attr->value), value);
}

static HRESULT WINAPI ivbsaxattributes_getValueFromQName(IVBSAXAttributes *iface, BSTR name, BSTR *value)
{
    struct saxlocator *locator = impl_from_IVBSAXAttributes(iface);
    const struct attribute *attr;
    int index;

    TRACE("%p, %s, %p.\n", iface, debugstr_w(name), value);

    if (!value)
        return E_POINTER;

    *value = NULL;

    if (!(attr = saxlocator_get_attribute_by_qname(locator, name, SysStringLen(name), &index)))
        return E_INVALIDARG;

    return return_bstrn(attr->value, SysStringLen(attr->value), value);
}

static const struct IVBSAXAttributesVtbl ivbsaxattributes_vtbl =
{
    ivbsaxattributes_QueryInterface,
    ivbsaxattributes_AddRef,
    ivbsaxattributes_Release,
    ivbsaxattributes_GetTypeInfoCount,
    ivbsaxattributes_GetTypeInfo,
    ivbsaxattributes_GetIDsOfNames,
    ivbsaxattributes_Invoke,
    ivbsaxattributes_get_length,
    ivbsaxattributes_getURI,
    ivbsaxattributes_getLocalName,
    ivbsaxattributes_getQName,
    ivbsaxattributes_getIndexFromName,
    ivbsaxattributes_getIndexFromQName,
    ivbsaxattributes_getType,
    ivbsaxattributes_getTypeFromName,
    ivbsaxattributes_getTypeFromQName,
    ivbsaxattributes_getValue,
    ivbsaxattributes_getValueFromName,
    ivbsaxattributes_getValueFromQName
};

static HRESULT WINAPI isaxattributes_QueryInterface(ISAXAttributes *iface, REFIID riid, void **obj)
{
    struct saxlocator *locator = impl_from_ISAXAttributes(iface);
    return ISAXLocator_QueryInterface(&locator->ISAXLocator_iface, riid, obj);
}

static ULONG WINAPI isaxattributes_AddRef(ISAXAttributes *iface)
{
    struct saxlocator *locator = impl_from_ISAXAttributes(iface);
    return ISAXLocator_AddRef(&locator->ISAXLocator_iface);
}

static ULONG WINAPI isaxattributes_Release(ISAXAttributes *iface)
{
    struct saxlocator *locator = impl_from_ISAXAttributes(iface);
    return ISAXLocator_Release(&locator->ISAXLocator_iface);
}

/*** ISAXAttributes methods ***/
static HRESULT WINAPI isaxattributes_getLength(ISAXAttributes *iface, int *length)
{
    struct saxlocator *locator = impl_from_ISAXAttributes(iface);

    TRACE("%p, %p.\n", iface, length);

    *length = locator->attributes.count;

    return S_OK;
}

static HRESULT saxlocator_return_string(BSTR str, const WCHAR **p, int *length)
{
    if (!p || !length)
        return E_POINTER;

    *length = SysStringLen(str);
    *p = str;

    TRACE("%s:%d\n", debugstr_w(str), *length);

    return S_OK;
}

static HRESULT WINAPI isaxattributes_getURI(ISAXAttributes *iface, int index,
        const WCHAR **url, int *length)
{
    struct saxlocator *locator = impl_from_ISAXAttributes(iface);
    const struct attribute *attr;

    TRACE("%p, %d, %p, %p.\n", iface, index, url, length);

    if (!(attr = saxlocator_get_attribute(locator, index))) return E_INVALIDARG;
    return saxlocator_return_string(attr->uri, url, length);
}

static HRESULT WINAPI isaxattributes_getLocalName(ISAXAttributes *iface, int index,
        const WCHAR **name, int *length)
{
    struct saxlocator *locator = impl_from_ISAXAttributes(iface);
    const struct attribute *attr;

    TRACE("%p, %d, %p, %p.\n", iface, index, name, length);

    if (!(attr = saxlocator_get_attribute(locator, index))) return E_INVALIDARG;
    return saxlocator_return_string(attr->local, name, length);
}

static HRESULT WINAPI isaxattributes_getQName(ISAXAttributes *iface, int index,
        const WCHAR **name, int *length)
{
    struct saxlocator *locator = impl_from_ISAXAttributes(iface);
    const struct attribute *attr;

    TRACE("%p, %d, %p, %p.\n", iface, index, name, length);

    if (!(attr = saxlocator_get_attribute(locator, index))) return E_INVALIDARG;
    return saxlocator_return_string(attr->qname, name, length);
}

static HRESULT WINAPI isaxattributes_getName(ISAXAttributes *iface, int index,
        const WCHAR **uri, int *uri_length, const WCHAR **local, int *local_length,
        const WCHAR **qname, int *qname_length)
{
    struct saxlocator *locator = impl_from_ISAXAttributes(iface);
    const struct attribute *attr;

    TRACE("%p, %d, %p, %p, %p, %p, %p, %p.\n", iface, index, uri, uri_length, local, local_length,
            qname, qname_length);

    if (!(attr = saxlocator_get_attribute(locator, index))) return E_INVALIDARG;
    if (!uri || !uri_length || !local || !local_length || !qname || !qname_length) return E_POINTER;

    saxlocator_return_string(attr->uri, uri, uri_length);
    saxlocator_return_string(attr->local, local, local_length);
    saxlocator_return_string(attr->qname, qname, qname_length);

    return S_OK;
}

static HRESULT WINAPI isaxattributes_getIndexFromName(ISAXAttributes *iface, const WCHAR *uri,
        int uri_length, const WCHAR *local, int local_length, int *index)
{
    struct saxlocator *locator = impl_from_ISAXAttributes(iface);
    const struct attribute *attr;

    TRACE("%p, %s, %d, %s, %d, %p.\n", iface, debugstr_w(uri), uri_length, debugstr_w(local), local_length, index);

    if (!uri || !local || !index)
        return E_POINTER;

    attr = saxlocator_get_attribute_by_name(locator, uri, uri_length, local, local_length, index);

    return attr ? S_OK : E_INVALIDARG;
}

static HRESULT WINAPI isaxattributes_getIndexFromQName(ISAXAttributes *iface, const WCHAR *name, int length, int *index)
{
    struct saxlocator *locator = impl_from_ISAXAttributes(iface);
    const struct attribute *attr;

    TRACE("%p, %s, %d, %p.\n", iface, debugstr_w(name), length, index);

    if (!name || !index) return E_POINTER;
    if (!length) return E_INVALIDARG;

    attr = saxlocator_get_attribute_by_qname(locator, name, length, index);

    return attr ? S_OK : E_INVALIDARG;
}

static HRESULT WINAPI isaxattributes_getType(ISAXAttributes *iface, int index, const WCHAR **type,
        int *length)
{
    FIXME("%p, %d, %p, %p stub\n", iface, index, type, length);

    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getTypeFromName(ISAXAttributes *iface, const WCHAR *uri,
        int uri_length, const WCHAR *name, int name_length, const WCHAR **type, int *type_length)
{
    FIXME("%p, %s, %d, %s, %d, %p, %p stub\n", iface, debugstr_w(uri), uri_length, debugstr_w(name),
            name_length, type, type_length);

    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getTypeFromQName(ISAXAttributes *iface, const WCHAR *name,
        int name_length, const WCHAR **type, int *type_length)
{
    FIXME("%p, %s, %d, %p, %p stub\n", iface, debugstr_w(name), name_length, type, type_length);

    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getValue(ISAXAttributes *iface, int index, const WCHAR **value,
        int *length)
{
    struct saxlocator *locator = impl_from_ISAXAttributes(iface);
    const struct attribute *attr;

    TRACE("%p, %d, %p, %p.\n", iface, index, value, length);

    if (!(attr = saxlocator_get_attribute(locator, index))) return E_INVALIDARG;
    return saxlocator_return_string(attr->value, value, length);
}

static HRESULT WINAPI isaxattributes_getValueFromName(ISAXAttributes *iface, const WCHAR *uri,
        int uri_length, const WCHAR *name, int name_length, const WCHAR **value, int *length)
{
    struct saxlocator *locator = impl_from_ISAXAttributes(iface);
    const struct attribute *attr;
    int index;

    TRACE("%p, %s, %d, %s, %d, %p, %p.\n", iface, debugstr_w(uri), uri_length,
            debugstr_w(name), name_length, value, length);

    if (!uri || !name)
        return E_POINTER;

    attr = saxlocator_get_attribute_by_name(locator, uri, uri_length, name, name_length, &index);
    if (attr)
        return saxlocator_return_string(attr->value, value, length);

    return E_INVALIDARG;
}

static HRESULT WINAPI isaxattributes_getValueFromQName(ISAXAttributes *iface,
        const WCHAR *name, int name_length, const WCHAR **value, int *length)
{
    struct saxlocator *locator = impl_from_ISAXAttributes(iface);
    const struct attribute *attr;
    int index;

    TRACE("%p, %s, %d, %p, %p.\n", iface, debugstr_w(name), name_length, value, length);

    if (!name)
        return E_POINTER;

    attr = saxlocator_get_attribute_by_qname(locator, name, name_length, &index);
    if (attr)
        return saxlocator_return_string(attr->value, value, length);

    return E_INVALIDARG;
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

static void saxreader_free_bstr(BSTR *str)
{
    if (str)
    {
        SysFreeString(*str);
        *str = NULL;
    }
}

static void saxreader_clear_attributes(struct saxlocator *locator)
{
    for (size_t i = 0; i < locator->attributes.count; ++i)
    {
        struct attribute *attr = &locator->attributes.entries[i];

        saxreader_free_bstr(&attr->local);
        saxreader_free_bstr(&attr->value);
        saxreader_free_bstr(&attr->qname);
        saxreader_free_bstr(&attr->prefix);
    }

    free(locator->attributes.map);
    locator->attributes.map = NULL;
    locator->attributes.count = 0;
}

static HRESULT WINAPI ivbsaxlocator_QueryInterface(IVBSAXLocator *iface, REFIID riid, void **obj)
{
    struct saxlocator *locator = impl_from_IVBSAXLocator(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_IDispatch) ||
            IsEqualGUID(riid, &IID_IVBSAXLocator))
    {
        *obj = iface;
    }
    else if (IsEqualGUID(riid, &IID_IVBSAXAttributes))
    {
        *obj = &locator->IVBSAXAttributes_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IVBSAXLocator_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI ivbsaxlocator_AddRef(IVBSAXLocator *iface)
{
    struct saxlocator *locator = impl_from_IVBSAXLocator(iface);
    return ISAXLocator_AddRef(&locator->ISAXLocator_iface);
}

static ULONG WINAPI ivbsaxlocator_Release(IVBSAXLocator *iface)
{
    struct saxlocator *locator = impl_from_IVBSAXLocator(iface);
    return ISAXLocator_Release(&locator->ISAXLocator_iface);
}

static HRESULT WINAPI ivbsaxlocator_GetTypeInfoCount(IVBSAXLocator *iface, UINT *count)
{
    TRACE("%p, %p.\n", iface, count);

    *count = 1;

    return S_OK;
}

static HRESULT WINAPI ivbsaxlocator_GetTypeInfo(IVBSAXLocator *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    TRACE("%p, %u, %lx, %p.\n", iface, index, lcid, ti);

    return get_typeinfo(IVBSAXLocator_tid, ti);
}

static HRESULT WINAPI ivbsaxlocator_GetIDsOfNames(IVBSAXLocator *iface, REFIID riid, LPOLESTR *names,
        UINT count, LCID lcid, DISPID *dispid)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %s, %p, %u, %lx, %p.\n", iface, debugstr_guid(riid), names, count, lcid, dispid);

    if (!names || !count || !dispid)
        return E_INVALIDARG;

    hr = get_typeinfo(IVBSAXLocator_tid, &typeinfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, dispid);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI ivbsaxlocator_Invoke(IVBSAXLocator *iface, DISPID dispid, REFIID riid,
        LCID lcid, WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *ei, UINT *argerr)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %ld, %s, %lx, %d, %p, %p, %p, %p.\n", iface, dispid, debugstr_guid(riid),
          lcid, flags, params, result, ei, argerr);

    hr = get_typeinfo(IVBSAXLocator_tid, &typeinfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispid, flags, params, result, ei, argerr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI ivbsaxlocator_get_columnNumber(IVBSAXLocator *iface, int *column)
{
    struct saxlocator *locator = impl_from_IVBSAXLocator(iface);
    return ISAXLocator_getColumnNumber(&locator->ISAXLocator_iface, column);
}

static HRESULT WINAPI ivbsaxlocator_get_lineNumber(IVBSAXLocator *iface, int *line)
{
    struct saxlocator *locator = impl_from_IVBSAXLocator(iface);
    return ISAXLocator_getLineNumber(&locator->ISAXLocator_iface, line);
}

static HRESULT WINAPI ivbsaxlocator_get_publicId(IVBSAXLocator *iface, BSTR *ret)
{
    FIXME("%p, %p stub.\n", iface, ret);

    if (!ret)
        return E_POINTER;

    *ret = NULL;

    return S_OK;
}

static HRESULT WINAPI ivbsaxlocator_get_systemId(IVBSAXLocator *iface, BSTR *ret)
{
    FIXME("%p, %p.\n", iface, ret);

    if (!ret)
        return E_POINTER;

    *ret = NULL;

    return S_OK;
}

static const struct IVBSAXLocatorVtbl VBSAXLocatorVtbl =
{
    ivbsaxlocator_QueryInterface,
    ivbsaxlocator_AddRef,
    ivbsaxlocator_Release,
    ivbsaxlocator_GetTypeInfoCount,
    ivbsaxlocator_GetTypeInfo,
    ivbsaxlocator_GetIDsOfNames,
    ivbsaxlocator_Invoke,
    ivbsaxlocator_get_columnNumber,
    ivbsaxlocator_get_lineNumber,
    ivbsaxlocator_get_publicId,
    ivbsaxlocator_get_systemId
};

static HRESULT WINAPI isaxlocator_QueryInterface(ISAXLocator *iface, REFIID riid, void **obj)
{
    struct saxlocator *locator = impl_from_ISAXLocator(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_ISAXLocator))
    {
        *obj = iface;
    }
    else if (IsEqualGUID(riid, &IID_ISAXAttributes))
    {
        *obj = &locator->ISAXAttributes_iface;
    }
    else
    {
        WARN("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ISAXLocator_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI isaxlocator_AddRef(ISAXLocator* iface)
{
    struct saxlocator *locator = impl_from_ISAXLocator(iface);
    ULONG refcount = InterlockedIncrement(&locator->refcount);
    TRACE("%p, refcount %lu.\n", iface, refcount);
    return refcount;
}

static void saxreader_clear_dtd(struct saxlocator *locator)
{
    for (int i = 0; i < ARRAYSIZE(locator->dtd.typenames); ++i)
        SysFreeString(locator->dtd.typenames[i]);
    for (int i = 0; i < ARRAYSIZE(locator->dtd.valuenames); ++i)
        SysFreeString(locator->dtd.valuenames[i]);
}

static ULONG WINAPI isaxlocator_Release(ISAXLocator *iface)
{
    struct saxlocator *locator = impl_from_ISAXLocator(iface);
    ULONG refcount = InterlockedDecrement(&locator->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
    {
        element_entry *element, *element2;

        SysFreeString(locator->xmlns_uri);
        SysFreeString(locator->xml_uri);
        SysFreeString(locator->null_uri);

        saxreader_clear_attributes(locator);
        free(locator->attributes.entries);

        saxreader_clear_dtd(locator);

        /* element stack */
        LIST_FOR_EACH_ENTRY_SAFE(element, element2, &locator->elements, element_entry, entry)
        {
            list_remove(&element->entry);
            free_element_entry(element);
        }

        encoded_buffer_cleanup(&locator->buffer.utf16);
        encoded_buffer_cleanup(&locator->buffer.raw);

        if (locator->stream)
            ISequentialStream_Release(locator->stream);
        ISAXXMLReader_Release(&locator->saxreader->ISAXXMLReader_iface);
        free(locator);
    }

    return refcount;
}

/*** ISAXLocator methods ***/
static HRESULT WINAPI isaxlocator_getColumnNumber(ISAXLocator *iface, int *column)
{
    struct saxlocator *locator = impl_from_ISAXLocator(iface);

    *column = locator->column;
    return S_OK;
}

static HRESULT WINAPI isaxlocator_getLineNumber(ISAXLocator *iface, int *line)
{
    struct saxlocator *locator = impl_from_ISAXLocator(iface);

    *line = locator->line;
    return S_OK;
}

static HRESULT WINAPI isaxlocator_getPublicId(ISAXLocator *iface, const WCHAR **value)
{
    FIXME("%p, %p stub\n", iface, value);

    *value = NULL;
    return S_OK;
}

static HRESULT WINAPI isaxlocator_getSystemId(ISAXLocator *iface, const WCHAR **value)
{
    FIXME("%p, %p stub\n", iface, value);

    *value = NULL;
    return S_OK;
}

static const struct ISAXLocatorVtbl SAXLocatorVtbl =
{
    isaxlocator_QueryInterface,
    isaxlocator_AddRef,
    isaxlocator_Release,
    isaxlocator_getColumnNumber,
    isaxlocator_getLineNumber,
    isaxlocator_getPublicId,
    isaxlocator_getSystemId
};

static HRESULT saxlocator_create(struct saxreader *reader, ISequentialStream *stream,
        bool vbInterface, struct saxlocator **obj)
{
    struct saxlocator *locator;
    HRESULT hr;

    *obj = NULL;

    if (!(locator = calloc(1, sizeof(*locator))))
        return E_OUTOFMEMORY;

    locator->IVBSAXLocator_iface.lpVtbl = &VBSAXLocatorVtbl;
    locator->ISAXLocator_iface.lpVtbl = &SAXLocatorVtbl;
    locator->IVBSAXAttributes_iface.lpVtbl = &ivbsaxattributes_vtbl;
    locator->ISAXAttributes_iface.lpVtbl = &isaxattributes_vtbl;
    locator->refcount = 1;
    locator->vbInterface = vbInterface;

    locator->saxreader = reader;
    ISAXXMLReader_AddRef(&reader->ISAXXMLReader_iface);

    locator->stream = stream;
    ISequentialStream_AddRef(locator->stream);

    locator->line = reader->version < MSXML4 ? 0 : 1;
    locator->column = 0;
    locator->buffer.position.line = 1;
    locator->buffer.position.column = 1;
    locator->buffer.chunk_size = 4096;
    list_init(&locator->buffer.entities);

    locator->ret = S_OK;

    locator->xml_uri = saxreader_alloc_string(locator, L"http://www.w3.org/XML/1998/namespace");
    locator->null_uri = saxreader_alloc_stringlen(locator, NULL, 0);
    if (locator->saxreader->version >= MSXML6)
        locator->xmlns_uri = saxreader_alloc_string(locator, L"http://www.w3.org/2000/xmlns/");
    else
        locator->xmlns_uri = saxreader_alloc_stringlen(locator, NULL, 0);

    list_init(&locator->elements);
    list_init(&locator->dtd.attr_decls);
    list_init(&locator->dtd.entities);

    if (FAILED(locator->status))
    {
        hr = locator->status;
        ISAXLocator_Release(&locator->ISAXLocator_iface);
        return hr;
    }

    *obj = locator;

    TRACE("returning %p\n", *obj);

    return S_OK;
}

static bool saxreader_array_reserve(struct saxlocator *locator, void **elements, size_t *capacity, size_t count, size_t size)
{
    bool ret;

    if (!(ret = array_reserve(elements, capacity, count, size)))
        saxreader_set_error(locator, E_OUTOFMEMORY);

    return ret;
}

static bool saxreader_reserve_buffer(struct saxlocator *locator, struct encoded_buffer *buffer, size_t size)
{
    return saxreader_array_reserve(locator, (void **)&buffer->data, &buffer->allocated,
            buffer->written + size, sizeof(*buffer->data));
}

static BSTR saxreader_string_to_bstr(struct saxlocator *locator, struct string_buffer *buffer)
{
    BSTR str = saxreader_alloc_stringlen(locator, buffer->data, buffer->count);
    free(buffer->data);
    memset(buffer, 0, sizeof(*buffer));
    return str;
}

static bool saxreader_limit_xml_size(struct saxlocator *locator, ULONG read)
{
    if (locator->saxreader && locator->saxreader->max_xml_size > 0)
    {
        locator->buffer.raw_size += read;
        if (locator->buffer.raw_size > locator->saxreader->max_xml_size * 1024)
        {
            saxreader_set_error(locator, E_FAIL);
            locator->eos = true;
            return false;
        }
    }

    return true;
}

static bool saxreader_stream_read(struct saxlocator *locator, void *buffer, ULONG size, ULONG *read)
{
    HRESULT hr;

    if (locator->eos)
        return false;

    if (FAILED(hr = ISequentialStream_Read(locator->stream, buffer, size, read)))
    {
        saxreader_set_error(locator, hr);
        locator->eos = true;
        return false;
    }

    if (!saxreader_limit_xml_size(locator, *read))
        return false;

    locator->eos = *read == 0;
    return true;
}

static void saxreader_convert_input(struct saxlocator *locator, bool switch_encoding)
{
    struct encoded_buffer *utf16 = &locator->buffer.utf16;
    struct encoded_buffer *raw = &locator->buffer.raw;
    ULONG read = 0, size;

    /* First conversion attempt with a new codepage. */

    if (switch_encoding)
    {
        size = raw->cur;
        raw->cur = 0;
    }
    else
    {
        size = convert_get_raw_length(&locator->buffer);
    }

    read = locator->buffer.converter(locator->buffer.code_page, raw->data + raw->cur, size, NULL, 0);

    if (saxreader_reserve_buffer(locator, utf16, (read + 1) * sizeof(WCHAR)))
    {
        locator->buffer.converter(locator->buffer.code_page, raw->data + raw->cur, size,
                (WCHAR *)(utf16->data + utf16->written), read);

        utf16->written += read * sizeof(WCHAR);
        *(WCHAR *)&utf16->data[utf16->written] = 0;

        /* Discard processed raw data */
        if (locator->buffer.switched_encoding)
        {
            if (size < raw->written)
                memmove(raw->data, raw->data + size, raw->written - size);
            raw->written -= size;
        }
        else
        {
            raw->cur += size;
        }
    }
}

static void saxreader_more(struct saxlocator *locator)
{
    struct encoded_buffer *utf16 = &locator->buffer.utf16;
    struct encoded_buffer *raw = &locator->buffer.raw;
    ULONG read = 0, size;

    if (locator->buffer.encoding == XML_ENCODING_UTF16LE)
    {
        ULONG needed = locator->buffer.chunk_size * sizeof(WCHAR);

        if (!saxreader_reserve_buffer(locator, utf16, needed + sizeof(WCHAR)))
            return;

        if (!saxreader_stream_read(locator, utf16->data + utf16->written, needed, &read))
            return;
        utf16->written += read;
        *(WCHAR *)&utf16->data[utf16->written] = 0;

        return;
    }

    size = convert_estimate_raw_size(locator->buffer.encoding, locator->buffer.chunk_size);
    if (!saxreader_reserve_buffer(locator, raw, size))
        return;

    if (!saxreader_stream_read(locator, raw->data + raw->written, size, &read))
        return;
    raw->written += read;

    /* Convert complete part of the input */
    saxreader_convert_input(locator, false);
}

static void saxreader_shrink(struct saxlocator *locator)
{
    struct encoded_buffer *buffer = &locator->buffer.utf16;
    size_t size;

    if (buffer->cur > 3 * locator->buffer.chunk_size / 8)
    {
        size = buffer->written - buffer->cur * sizeof(WCHAR);
        if (size)
            memmove(buffer->data, (WCHAR *)buffer->data + buffer->cur, size);

        buffer->written = size;
        buffer->cur = 0;
    }
}

static WCHAR *saxreader_get_ptr(struct saxlocator *locator)
{
    struct encoded_buffer *buffer = &locator->buffer.utf16;
    WCHAR *ptr = (WCHAR *)buffer->data + buffer->cur;
    if (!*ptr) saxreader_more(locator);
    return (WCHAR *)buffer->data + buffer->cur;
}

static WCHAR saxreader_get_char(struct saxlocator *locator, int pos)
{
    struct encoded_buffer *buffer = &locator->buffer.utf16;
    if (buffer->written - buffer->cur < pos * sizeof(WCHAR)) return 0;
    return *(WCHAR *)(buffer->data + buffer->cur + pos * sizeof(WCHAR));
}

static WCHAR *saxreader_get_ptr_noread(struct saxlocator *locator)
{
    struct encoded_buffer *buffer = &locator->buffer.utf16;
    return (WCHAR *)buffer->data + buffer->cur;
}

static void saxreader_update_position(struct saxlocator *locator, WCHAR ch)
{
    struct input_buffer *input = &locator->buffer;

    /* Treat \r\n -> \n */
    if (ch == '\r' || (!input->last_cr && ch == '\n'))
    {
        ++input->position.line;
        input->position.column = 1;
    }
    else if (!(input->last_cr && ch == '\n'))
    {
        ++input->position.column;
    }
    input->last_cr = ch == '\r';
}

static void saxlocator_end_entity(struct saxlocator *locator, const struct text_position *position, BSTR name);

static bool saxreader_pop_entity(struct saxlocator *locator)
{
    struct input_buffer *input = &locator->buffer;
    struct entity_decl *entity;

    if (list_empty(&input->entities)) return true;

    entity = LIST_ENTRY(list_head(&input->entities), struct entity_decl, input_entry);

    if (entity->remaining)
    {
        --entity->remaining;
        return false;
    }

    entity->visited = false;
    list_remove(&entity->input_entry);

    saxlocator_end_entity(locator, &entity->position, entity->name);

    return list_empty(&input->entities);
}

static void saxreader_string_append(struct saxlocator *locator, struct string_buffer *buffer,
        const WCHAR *str, UINT len)
{
    if (!saxreader_array_reserve(locator, (void **)&buffer->data, &buffer->capacity, buffer->count + len, sizeof(*str)))
    {
        saxreader_set_error(locator, E_OUTOFMEMORY);
        return;
    }

    memcpy(buffer->data + buffer->count, str, len * sizeof(WCHAR));
    buffer->count += len;
}

static void saxreader_skip(struct saxlocator *locator, int n)
{
    struct encoded_buffer *buffer = &locator->buffer.utf16;
    struct input_buffer *input = &locator->buffer;
    WCHAR ch;

    while (locator->status == S_OK && (ch = *saxreader_get_ptr(locator)) && n--)
    {
        if (locator->collect)
            saxreader_string_append(locator, &locator->scratch, &ch, 1);

        if (saxreader_pop_entity(locator))
            saxreader_update_position(locator, ch);

        ++buffer->cur;
        ++input->consumed;
    }
}

static bool saxreader_peek(struct saxlocator *locator, const WCHAR *str, size_t count)
{
    struct encoded_buffer *buffer = &locator->buffer.utf16;
    WCHAR *ptr = (WCHAR *)buffer->data + buffer->cur;

    if (FAILED(locator->status))
        return false;

    if ((buffer->written - buffer->cur) < count)
        return false;

    return !memcmp(ptr, str, count * sizeof(WCHAR));
}

static bool saxreader_cmp(struct saxlocator *locator, const WCHAR *str)
{
    const WCHAR *ptr;
    size_t i = 0;

    if (FAILED(locator->status))
        return false;

    ptr = saxreader_get_ptr(locator);
    while (str[i])
    {
        if (!ptr[i])
        {
            saxreader_more(locator);
            ptr = saxreader_get_ptr(locator);
        }
        if (str[i] != ptr[i])
            return false;
        i++;
    }
    saxreader_skip(locator, i);
    return true;
}

static void saxreader_fatal_error(struct saxlocator *locator)
{
    struct saxerrorhandler_iface *handler = saxreader_get_errorhandler(locator->saxreader);

    if (saxreader_has_handler(locator, SAXErrorHandler))
    {
        WCHAR msg[1024];

        if (!FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE, MSXML_hInstance, locator->status, 0,
                msg, ARRAY_SIZE(msg), NULL))
        {
            *msg = 0;
        }

        if (locator->vbInterface)
        {
            BSTR s = SysAllocString(msg);
            IVBSAXErrorHandler_fatalError(handler->vbhandler, &locator->IVBSAXLocator_iface, &s, locator->status);
            SysFreeString(s);
        }
        else
        {
            ISAXErrorHandler_fatalError(handler->handler, &locator->ISAXLocator_iface, msg, locator->status);
        }
    }
}

struct ns
{
    BSTR prefix;
    BSTR uri;
};

struct element
{
    struct list entry;
    struct parsed_name name;
    BSTR uri;

    struct
    {
        struct ns *entries;
        size_t count;
        size_t capacity;
    } ns;
};

static HRESULT saxlocator_callback_result(struct saxlocator *locator, HRESULT hr)
{
    if (locator->saxreader->version >= MSXML4) return FAILED(hr) ? hr : S_OK;
    return hr;
}

static void saxlocator_put_document_locator(struct saxlocator *locator)
{
    struct saxcontenthandler_iface *handler = saxreader_get_contenthandler(locator->saxreader);
    HRESULT hr = S_OK;

    if (locator->status != S_OK)
        return;

    locator->start_document_position = locator->buffer.position;
    if (locator->start_document_position.column)
        --locator->start_document_position.column;

    if (saxreader_has_handler(locator, SAXContentHandler))
    {
        if (locator->vbInterface)
            hr = IVBSAXContentHandler_putref_documentLocator(handler->vbhandler, &locator->IVBSAXLocator_iface);
        else
            hr = ISAXContentHandler_putDocumentLocator(handler->handler, &locator->ISAXLocator_iface);
    }

    if (FAILED(hr))
        locator->status = hr;
}

static void saxlocator_xmldecl(struct saxlocator *locator)
{
    struct saxextensionhandler_iface *handler = saxreader_get_extensionhandler(locator->saxreader);
    HRESULT hr = S_OK;

    if (locator->status != S_OK)
        return;

    if (!locator->saxreader->xmldecl_version)
        return;

    if (saxreader_has_handler(locator, SAXExtensionHandler))
    {
        if (!locator->vbInterface)
            hr = ISAXExtensionHandler_xmldecl(handler->handler,
                    locator->saxreader->xmldecl_version,
                    locator->saxreader->xmldecl_encoding,
                    locator->saxreader->xmldecl_standalone);
    }

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxlocator_start_document(struct saxlocator *locator)
{
    struct saxcontenthandler_iface *handler = saxreader_get_contenthandler(locator->saxreader);
    HRESULT hr = S_OK;

    if (locator->status != S_OK)
        return;

    if (locator->saxreader->version >= MSXML4)
    {
        locator->line = locator->start_document_position.line;
        locator->column = locator->start_document_position.column;
    }
    else
    {
        locator->line = 0;
        locator->column = 0;
    }

    if (saxreader_has_handler(locator, SAXContentHandler))
    {
        if (locator->vbInterface)
            hr = IVBSAXContentHandler_startDocument(handler->vbhandler);
        else
            hr = ISAXContentHandler_startDocument(handler->handler);
    }

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxlocator_end_document(struct saxlocator *locator)
{
    struct saxcontenthandler_iface *handler = saxreader_get_contenthandler(locator->saxreader);
    HRESULT hr = S_OK;

    if (locator->status != S_OK)
        return;

    if (locator->saxreader->version >= MSXML4)
    {
        locator->line = locator->buffer.position.line;
        locator->column = locator->buffer.position.column - 1;
    }
    else
    {
        locator->line = 0;
        locator->column = 0;
    }

    if (saxreader_has_handler(locator, SAXContentHandler))
    {
        if (locator->vbInterface)
            hr = IVBSAXContentHandler_endDocument(handler->vbhandler);
        else
            hr = ISAXContentHandler_endDocument(handler->handler);
    }

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxlocator_start_element(struct saxlocator *locator, const struct text_position *position,
        struct element *element)
{
    struct saxcontenthandler_iface *handler = saxreader_get_contenthandler(locator->saxreader);
    int max_depth = locator->saxreader->max_element_depth;
    MSXML_VERSION version = locator->saxreader->version;
    BSTR uri = NULL, local = NULL;
    HRESULT hr;

    if (locator->status != S_OK)
        return;

    ++locator->depth;
    if (max_depth && locator->depth > max_depth)
        return saxreader_set_error(locator, version < MSXML6 ? E_ABORT : E_SAX_MAX_ELEMENT_DEPTH);

    if (!saxreader_has_handler(locator, SAXContentHandler))
        return;

    locator->line = position->line;
    locator->column = position->column;
    /* Point to the closing '>' */
    if (version >= MSXML4)
        --locator->column;

    if (is_namespaces_enabled(locator->saxreader))
    {
        for (size_t i = 0; i < element->ns.count && saxreader_has_handler(locator, SAXContentHandler); ++i)
        {
            struct ns *ns = &element->ns.entries[i];

            if (locator->vbInterface)
                hr = IVBSAXContentHandler_startPrefixMapping(
                        handler->vbhandler, &ns->prefix, &ns->uri);
            else
                hr = ISAXContentHandler_startPrefixMapping(
                        handler->handler,
                        ns->prefix,
                        SysStringLen(ns->prefix),
                        ns->uri,
                        SysStringLen(ns->uri));

            if (FAILED((hr = saxlocator_callback_result(locator, hr))))
            {
                locator->status = hr;
                return;
            }
        }

        uri = element->uri;
        local = element->name.local;
    }

    if (locator->vbInterface)
    {
        if (!uri) uri = locator->saxreader->empty_bstr;
        hr = IVBSAXContentHandler_startElement(handler->vbhandler, &uri, &local, &element->name.qname,
                &locator->IVBSAXAttributes_iface);
    }
    else
    {
        hr = ISAXContentHandler_startElement(handler->handler, uri ? uri : &empty_str, SysStringLen(uri),
                local ? local : &empty_str, SysStringLen(local), element->name.qname, SysStringLen(element->name.qname),
                &locator->ISAXAttributes_iface);
    }

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxlocator_end_element(struct saxlocator *locator, const struct text_position *position, struct element *element)
{
    struct saxcontenthandler_iface *handler = saxreader_get_contenthandler(locator->saxreader);
    BSTR uri = NULL, local = NULL;
    HRESULT hr;

    if (locator->status != S_OK)
        return;

    --locator->depth;
    locator->line = position->line;
    /* Point to the closing '>' */
    if (locator->saxreader->version >= MSXML4)
        locator->column = locator->buffer.position.column - 1;
    else
        locator->column = position->column;

    if (saxreader_has_handler(locator, SAXContentHandler))
    {
        if (is_namespaces_enabled(locator->saxreader))
        {
            uri = element->uri;
            local = element->name.local;
        }

        if (locator->vbInterface)
        {
            if (!uri) uri = locator->saxreader->empty_bstr;
            hr = IVBSAXContentHandler_endElement(handler->vbhandler, &uri, &local, &element->name.qname);
        }
        else
        {
            hr = ISAXContentHandler_endElement(handler->handler, uri ? uri : &empty_str, SysStringLen(uri),
                    local ? local : &empty_str, SysStringLen(local), element->name.qname, SysStringLen(element->name.qname));
        }

        locator->status = saxlocator_callback_result(locator, hr);
    }

    if (locator->status != S_OK)
        return;

    if (is_namespaces_enabled(locator->saxreader))
    {
        for (size_t i = 0; i < element->ns.count && saxreader_has_handler(locator, SAXContentHandler); ++i)
        {
            size_t index = locator->saxreader->version >= MSXML4 ? i : element->ns.count - 1 - i;
            struct ns *ns = &element->ns.entries[index];

            if (locator->vbInterface)
                hr = IVBSAXContentHandler_endPrefixMapping(handler->vbhandler, &ns->prefix);
            else
                hr = ISAXContentHandler_endPrefixMapping(handler->handler, ns->prefix, SysStringLen(ns->prefix));

            locator->status = saxlocator_callback_result(locator, hr);
            if (locator->status != S_OK)
                break;
        }
    }
}

static void saxlocator_start_cdata(struct saxlocator *locator, const struct text_position *position)
{
    struct saxlexicalhandler_iface *lexical = saxreader_get_lexicalhandler(locator->saxreader);
    HRESULT hr = S_OK;

    if (locator->status != S_OK)
        return;

    locator->line = position->line;
    locator->column = position->column;
    /* Point to the closing '[' */
    if (locator->saxreader->version >= MSXML4)
        --locator->column;

    if (saxreader_has_handler(locator, SAXLexicalHandler))
    {
       if (locator->vbInterface)
           hr = IVBSAXLexicalHandler_startCDATA(lexical->vbhandler);
       else
           hr = ISAXLexicalHandler_startCDATA(lexical->handler);
    }

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxlocator_end_cdata(struct saxlocator *locator, const struct text_position *position)
{
    struct saxlexicalhandler_iface *lexical = saxreader_get_lexicalhandler(locator->saxreader);
    HRESULT hr = S_OK;

    if (locator->status != S_OK)
        return;

    if (locator->saxreader->version >= MSXML4)
    {
        locator->line = locator->buffer.position.line;
        locator->column = locator->buffer.position.column - 1;
    }
    else
    {
        locator->line = position->line;
        locator->column = position->column;
    }

    if (saxreader_has_handler(locator, SAXLexicalHandler))
    {
       if (locator->vbInterface)
           hr = IVBSAXLexicalHandler_endCDATA(lexical->vbhandler);
       else
           hr = ISAXLexicalHandler_endCDATA(lexical->handler);
    }

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxlocator_characters(struct saxlocator *locator, const struct text_position *position, BSTR chars)
{
    struct saxcontenthandler_iface *content = saxreader_get_contenthandler(locator->saxreader);
    HRESULT hr;

    if (locator->status != S_OK || !*chars)
        return;

    locator->line = position->line;
    locator->column = position->column;

    if (!saxreader_has_handler(locator, SAXContentHandler))
        return;

    if (locator->vbInterface)
        hr = IVBSAXContentHandler_characters(content->vbhandler, &chars);
    else
        hr = ISAXContentHandler_characters(content->handler, chars, SysStringLen(chars));

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxlocator_comment(struct saxlocator *locator, const struct text_position *position, BSTR chars)
{
    struct saxlexicalhandler_iface *lexical = saxreader_get_lexicalhandler(locator->saxreader);
    HRESULT hr;

    if (locator->status != S_OK)
        return;

    locator->line = position->line;
    locator->column = position->column;

    if (!saxreader_has_handler(locator, SAXLexicalHandler))
        return;

    if (locator->vbInterface)
        hr = IVBSAXLexicalHandler_comment(lexical->vbhandler, &chars);
    else
        hr = ISAXLexicalHandler_comment(lexical->handler, chars, SysStringLen(chars));

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxlocator_pi(struct saxlocator *locator, const struct text_position *position, BSTR target, BSTR data)
{
    struct saxcontenthandler_iface *handler = saxreader_get_contenthandler(locator->saxreader);
    HRESULT hr;

    if (locator->status != S_OK)
        return;

    /* Point to the closing '>' */
    if (locator->saxreader->version >= MSXML4)
    {
        locator->line = locator->buffer.position.line;
        locator->column = locator->buffer.position.column - 1;
    }
    else
    {
        locator->line = position->line;
        locator->column = position->column;
    }

    if (!saxreader_has_handler(locator, SAXContentHandler))
        return;

    if (locator->vbInterface)
        hr = IVBSAXContentHandler_processingInstruction(handler->vbhandler, &target, &data);
    else
        hr = ISAXContentHandler_processingInstruction(handler->handler, target, SysStringLen(target),
                data, SysStringLen(data));

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxlocator_skipped_entity(struct saxlocator *locator, const struct text_position *position, BSTR name)
{
    struct saxcontenthandler_iface *handler = saxreader_get_contenthandler(locator->saxreader);
    HRESULT hr;

    if (locator->status != S_OK)
        return;

    locator->line = position->line;
    locator->column = position->column;

    if (!saxreader_has_handler(locator, SAXContentHandler))
        return;

    if (locator->vbInterface)
        hr = IVBSAXContentHandler_skippedEntity(handler->vbhandler, &name);
    else
        hr = ISAXContentHandler_skippedEntity(handler->handler, name, SysStringLen(name));

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxlocator_notationdecl(struct saxlocator *locator, const struct text_position *position,
        BSTR name, BSTR pubid, BSTR sysid)
{
    struct saxdtdhandler_iface *handler = saxreader_get_dtdhandler(locator->saxreader);
    HRESULT hr;

    if (locator->status != S_OK)
        return;

    locator->line = position->line;
    locator->column = position->column;

    if (!saxreader_has_handler(locator, SAXDTDHandler))
        return;

    if (locator->vbInterface)
        hr = IVBSAXDTDHandler_notationDecl(handler->vbhandler, &name, &pubid, &sysid);
    else
        hr = ISAXDTDHandler_notationDecl(handler->handler, name, SysStringLen(name),
                pubid, SysStringLen(pubid), sysid, SysStringLen(sysid));

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxlocator_unparsed_entitydecl(struct saxlocator *locator, BSTR name, BSTR pubid, BSTR sysid, BSTR notation)
{
    struct saxdtdhandler_iface *handler = saxreader_get_dtdhandler(locator->saxreader);
    HRESULT hr;

    if (locator->status != S_OK)
        return;

    locator->line = locator->buffer.position.line;
    locator->column = locator->buffer.position.column;

    if (!saxreader_has_handler(locator, SAXDTDHandler))
        return;

    if (locator->vbInterface)
        hr = IVBSAXDTDHandler_unparsedEntityDecl(handler->vbhandler, &name, &pubid, &sysid, &notation);
    else
        hr = ISAXDTDHandler_unparsedEntityDecl(handler->handler, name, SysStringLen(name),
                pubid, SysStringLen(pubid), sysid, SysStringLen(sysid), notation, SysStringLen(notation));

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxlocator_external_entitydecl(struct saxlocator *locator, BSTR name, BSTR pubid, BSTR sysid)
{
    struct saxdeclhandler_iface *handler = saxreader_get_declhandler(locator->saxreader);
    HRESULT hr;

    if (locator->status != S_OK)
        return;

    locator->line = locator->buffer.position.line;
    locator->column = locator->buffer.position.column;

    if (!saxreader_has_handler(locator, SAXDeclHandler))
        return;

    if (locator->vbInterface)
        hr = IVBSAXDeclHandler_externalEntityDecl(handler->vbhandler, &name, &pubid, &sysid);
    else
        hr = ISAXDeclHandler_externalEntityDecl(handler->handler, name, SysStringLen(name),
                pubid, SysStringLen(pubid), sysid, SysStringLen(sysid));

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxreader_element_decl_occurence_stringify(struct saxlocator *locator, struct string_buffer *buffer,
        struct element_content *c)
{
    if (c->occurrence == ELEMENT_CONTENT_OPT)
        saxreader_string_append(locator, buffer, L"?", 1);
    else if (c->occurrence == ELEMENT_CONTENT_MULT)
        saxreader_string_append(locator, buffer, L"*", 1);
    else if (c->occurrence == ELEMENT_CONTENT_PLUS)
        saxreader_string_append(locator, buffer, L"+", 1);
}

static void saxreader_element_decl_content_stringify(struct saxlocator *locator, struct string_buffer *buffer,
        struct element_decl *decl)
{
    struct element_content *cur, *content;

    saxreader_string_append(locator, buffer, L"(", 1);
    cur = content = decl->content;

    do
    {
        if (!cur) return;

        switch (cur->type)
        {
            case ELEMENT_CONTENT_PCDATA:
                saxreader_string_append(locator, buffer, L"#PCDATA", 7);
                break;
            case ELEMENT_CONTENT_ELEMENT:
                saxreader_string_append(locator, buffer, cur->name, SysStringLen(cur->name));
                break;
            case ELEMENT_CONTENT_SEQ:
            case ELEMENT_CONTENT_OR:
                if ((cur != decl->content) && cur->parent &&
                        ((cur->type != cur->parent->type) || (cur->occurrence != ELEMENT_CONTENT_ONCE)))
                {
                    saxreader_string_append(locator, buffer, L"(", 1);
                }
                cur = cur->left;
                continue;
            default:
                ;
        }

        while (cur != content)
        {
            struct element_content *parent = cur->parent;

            if (!parent) return;

            if (((cur->type == ELEMENT_CONTENT_OR) ||
                 (cur->type == ELEMENT_CONTENT_SEQ)) &&
                ((cur->type != parent->type) ||
                 (cur->occurrence != ELEMENT_CONTENT_ONCE)))
            {
                saxreader_string_append(locator, buffer, L")", 1);
            }
            saxreader_element_decl_occurence_stringify(locator, buffer, cur);

            if (cur == parent->left)
            {
                if (parent->type == ELEMENT_CONTENT_SEQ)
                    saxreader_string_append(locator, buffer, L",", 1);
                else if (parent->type == ELEMENT_CONTENT_OR)
                    saxreader_string_append(locator, buffer, L"|", 1);

                cur = parent->right;
                break;
            }

            cur = parent;
        }
    }
    while (cur != content);

    saxreader_string_append(locator, buffer, L")", 1);
    saxreader_element_decl_occurence_stringify(locator, buffer, content);
}

static BSTR saxreader_element_decl_model_stringify(struct saxlocator *locator, struct element_decl *decl)
{
    struct string_buffer buffer = { 0 };

    if (decl->type == ELEMENT_TYPE_EMPTY)
        saxreader_string_append(locator, &buffer, L"EMPTY", 5);
    else if (decl->type == ELEMENT_TYPE_ANY)
        saxreader_string_append(locator, &buffer, L"ANY", 3);
    else
        saxreader_element_decl_content_stringify(locator, &buffer, decl);

    return saxreader_string_to_bstr(locator, &buffer);
}

static void saxlocator_elementdecl(struct saxlocator *locator, struct element_decl *decl)
{
    struct saxdeclhandler_iface *handler = saxreader_get_declhandler(locator->saxreader);
    HRESULT hr;
    BSTR model;

    if (locator->status != S_OK)
        return;

    locator->line = locator->buffer.position.line;
    locator->column = locator->buffer.position.column;

    if (!saxreader_has_handler(locator, SAXDeclHandler))
        return;

    model = saxreader_element_decl_model_stringify(locator, decl);

    if (locator->vbInterface)
        hr = IVBSAXDeclHandler_elementDecl(handler->vbhandler, &decl->name, &model);
    else
        hr = ISAXDeclHandler_elementDecl(handler->handler, decl->name, SysStringLen(decl->name), model, SysStringLen(model));

    SysFreeString(model);

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxlocator_internal_entitydecl(struct saxlocator *locator, BSTR name, BSTR value)
{
    struct saxdeclhandler_iface *handler = saxreader_get_declhandler(locator->saxreader);
    HRESULT hr;

    if (locator->status != S_OK)
        return;

    locator->line = locator->buffer.position.line;
    locator->column = locator->buffer.position.column;

    if (!saxreader_has_handler(locator, SAXDeclHandler))
        return;

    if (locator->vbInterface)
        hr = IVBSAXDeclHandler_internalEntityDecl(handler->vbhandler, &name, &value);
    else
        hr = ISAXDeclHandler_internalEntityDecl(handler->handler, name, SysStringLen(name),
                value, SysStringLen(value));

    locator->status = saxlocator_callback_result(locator, hr);
}

static BSTR saxreader_attribute_type_stringify(struct saxlocator *locator, struct attlist_attr *attr)
{
    struct string_buffer buffer = { 0 };
    bool separate = false;

    if (attr->type == ATTR_TYPE_NOTATION)
        saxreader_string_append(locator, &buffer, L"NOTATION ", 9);

    saxreader_string_append(locator, &buffer, L"(", 1);
    for (size_t i = 0; i < attr->valuelist.count; ++i)
    {
        if (separate)
            saxreader_string_append(locator, &buffer, L"|", 1);
        saxreader_string_append(locator, &buffer, attr->valuelist.values[i], SysStringLen(attr->valuelist.values[i]));
        separate = true;
    }
    saxreader_string_append(locator, &buffer, L")", 1);

    return saxreader_string_to_bstr(locator, &buffer);
}

static void saxlocator_attribute_decl(struct saxlocator *locator, struct attlist_decl *decl)
{
    struct saxdeclhandler_iface *handler = saxreader_get_declhandler(locator->saxreader);
    BSTR typename, tofree, valuetype;
    HRESULT hr;

    if (locator->status != S_OK)
        return;

    locator->line = locator->buffer.position.line;
    locator->column = locator->buffer.position.column;

    if (!saxreader_has_handler(locator, SAXDeclHandler))
        return;

    saxreader_ensure_dtd_typenames(locator);
    if (locator->status != S_OK)
        return;

    for (size_t i = 0; i < decl->count && locator->status == S_OK; ++i)
    {
        tofree = NULL;

        if (decl->attributes[i].type == ATTR_TYPE_NOTATION
                || decl->attributes[i].type == ATTR_TYPE_ENUMERATION)
        {
            typename = tofree = saxreader_attribute_type_stringify(locator, &decl->attributes[i]);
        }
        else
        {
            typename = locator->dtd.typenames[decl->attributes[i].type];
        }
        valuetype = locator->dtd.valuenames[decl->attributes[i].valuetype];

        if (locator->vbInterface)
            hr = IVBSAXDeclHandler_attributeDecl(handler->vbhandler, &decl->name,
                    &decl->attributes[i].name.qname, &typename, &valuetype, &decl->attributes[i].value);
        else
            hr = ISAXDeclHandler_attributeDecl(handler->handler, decl->name, SysStringLen(decl->name),
                    decl->attributes[i].name.qname, SysStringLen(decl->attributes[i].name.qname),
                    typename, SysStringLen(typename), valuetype, SysStringLen(valuetype),
                    decl->attributes[i].value, SysStringLen(decl->attributes[i].value));

        SysFreeString(tofree);
        locator->status = saxlocator_callback_result(locator, hr);
    }
}

static void saxlocator_startdtd(struct saxlocator *locator, BSTR name, BSTR pubid, BSTR sysid)
{
    struct saxlexicalhandler_iface *lexical = saxreader_get_lexicalhandler(locator->saxreader);
    HRESULT hr;

    if (locator->status != S_OK)
        return;

    locator->line = locator->buffer.position.line;
    locator->column = locator->buffer.position.column;

    if (!saxreader_has_handler(locator, SAXLexicalHandler))
        return;

    if (locator->vbInterface)
        hr = IVBSAXLexicalHandler_startDTD(lexical->vbhandler, &name, &pubid, &sysid);
    else
        hr = ISAXLexicalHandler_startDTD(lexical->handler, name, SysStringLen(name),
                pubid, SysStringLen(pubid), sysid, SysStringLen(sysid));

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxlocator_enddtd(struct saxlocator *locator)
{
    struct saxlexicalhandler_iface *lexical = saxreader_get_lexicalhandler(locator->saxreader);
    HRESULT hr;

    if (locator->status != S_OK)
        return;

    locator->line = locator->buffer.position.line;
    locator->column = locator->buffer.position.column;

    if (!saxreader_has_handler(locator, SAXLexicalHandler))
        return;

    if (locator->vbInterface)
        hr = IVBSAXLexicalHandler_endDTD(lexical->vbhandler);
    else
        hr = ISAXLexicalHandler_endDTD(lexical->handler);

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxlocator_extension_dtd(struct saxlocator *locator)
{
    struct saxextensionhandler_iface *handler = saxreader_get_extensionhandler(locator->saxreader);
    HRESULT hr = S_OK;
    BSTR str;

    if (locator->status != S_OK)
        return;

    str = saxreader_string_to_bstr(locator, &locator->scratch);
    if (saxreader_has_handler(locator, SAXExtensionHandler))
    {
        if (!locator->vbInterface)
            hr = ISAXExtensionHandler_dtd(handler->handler, str);
    }
    SysFreeString(str);

    locator->collect = false;

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxlocator_start_entity(struct saxlocator *locator, const struct text_position *position, BSTR name)
{
    struct saxlexicalhandler_iface *lexical = saxreader_get_lexicalhandler(locator->saxreader);
    HRESULT hr;

    if (locator->status != S_OK)
        return;

    locator->line = position->line;
    locator->column = position->column;

    if (!saxreader_has_handler(locator, SAXLexicalHandler))
        return;

    if (locator->vbInterface)
        hr = IVBSAXLexicalHandler_startEntity(lexical->vbhandler, &name);
    else
        hr = ISAXLexicalHandler_startEntity(lexical->handler, name, SysStringLen(name));

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxlocator_end_entity(struct saxlocator *locator, const struct text_position *position, BSTR name)
{
    struct saxlexicalhandler_iface *lexical = saxreader_get_lexicalhandler(locator->saxreader);
    HRESULT hr;

    if (locator->status != S_OK)
        return;

    locator->line = position->line;
    locator->column = position->column;

    if (!saxreader_has_handler(locator, SAXLexicalHandler))
        return;

    if (locator->vbInterface)
        hr = IVBSAXLexicalHandler_endEntity(lexical->vbhandler, &name);
    else
        hr = ISAXLexicalHandler_endEntity(lexical->handler, name, SysStringLen(name));

    locator->status = saxlocator_callback_result(locator, hr);
}

static void saxreader_string_append_bstr(struct saxlocator *locator, struct string_buffer *buffer, BSTR *str)
{
    saxreader_string_append(locator, buffer, *str, SysStringLen(*str));
    SysFreeString(*str);
    *str = NULL;
}

/* [3] S ::= (#x20 | #x9 | #xD | #xA)+ */
static bool saxreader_is_space(WCHAR ch)
{
    return xml_is_space(ch);
}

/* [3] S ::= (#x20 | #x9 | #xD | #xA)+ */
static bool saxreader_skipspaces(struct saxlocator *locator)
{
    size_t length = 0;
    const WCHAR *ptr;

    if (locator->status != S_OK)
        return false;

    ptr = saxreader_get_ptr(locator);
    while (saxreader_is_space(*ptr))
    {
        saxreader_skip(locator, 1);
        ptr = saxreader_get_ptr(locator);
        ++length;
    }

    return length;
}

static bool saxreader_skip_required_spaces(struct saxlocator *locator)
{
    bool ret;

    if (!(ret = saxreader_skipspaces(locator)))
        saxreader_set_error(locator, E_SAX_MISSINGWHITESPACE);

    return ret;
}

static WCHAR saxreader_is_quote(struct saxlocator *locator)
{
    const WCHAR *ptr;

    if (locator->status != S_OK)
        return 0;

    ptr = saxreader_get_ptr(locator);
    if (*ptr == '\'' || *ptr == '"') return *ptr;
    return 0;
}

/* [24] VersionInfo ::= S 'version' Eq ("'" VersionNum "'" | '"' VersionNum '"') */
/* [26] VersionNum ::= '1.' [0-9]+ */
static void saxreader_parse_versioninfo(struct saxlocator *locator, const struct parsed_name *name, BSTR value)
{
    if (wcscmp(name->qname, L"version"))
        return saxreader_set_error(locator, E_SAX_UNEXPECTED_ATTRIBUTE);

    if (wcscmp(value, L"1.0"))
        return saxreader_set_error(locator, E_SAX_INVALID_VERSION);

    SysFreeString(locator->saxreader->xmldecl_version);
    locator->saxreader->xmldecl_version = saxreader_alloc_string(locator, L"1.0");
}

static void saxreader_switch_encoding(struct saxlocator *locator, UINT codepage)
{
    /* We'll keep mbtowc converter. */
    locator->buffer.code_page = codepage;

    /* Convert entire buffer again, nothing should've been discarded yet.
       The utf16 buffer cursor is kept intact. */
    locator->buffer.utf16.written = 0;
    locator->buffer.switched_encoding = true;
    saxreader_convert_input(locator, true);
}

/* [80] EncodingDecl ::= S 'encoding' Eq ('"' EncName '"' | "'" EncName "'" ) */
static void saxreader_parse_encdecl(struct saxlocator *locator, const struct parsed_name *name, BSTR value)
{
    UINT codepage;

    if (wcscmp(name->qname, L"encoding"))
        return saxreader_set_error(locator, E_SAX_UNEXPECTED_ATTRIBUTE);

    locator->saxreader->xmldecl_encoding = saxreader_alloc_string(locator, value);
    TRACE("encoding name %s\n", debugstr_w(locator->saxreader->xmldecl_encoding));

    /* Switch encoding only in UTF-8 -> mbtowc-able direction. */

    if (locator->buffer.encoding == XML_ENCODING_UTF8)
    {
        if (saxreader_get_encoding_codepage(locator->saxreader->xmldecl_encoding, &codepage))
            saxreader_switch_encoding(locator, codepage);
        else if (!saxreader_can_ignore_encoding(locator->saxreader->xmldecl_encoding))
        {
            FIXME("Unsupported encoding %s.\n", debugstr_w(locator->saxreader->xmldecl_encoding));
            saxreader_set_error(locator, E_SAX_INVALIDENCODING);
        }
    }
}

/* [32] SDDecl ::= S 'standalone' Eq (("'" ('yes' | 'no') "'") | ('"' ('yes' | 'no') '"')) */
static void saxreader_parse_sddecl(struct saxlocator *locator, const struct parsed_name *name, BSTR value)
{
    if (wcscmp(name->qname, L"standalone"))
        return saxreader_set_error(locator, E_SAX_UNEXPECTED_ATTRIBUTE);

    if (!wcscmp(value, L"yes") || !wcscmp(value, L"no"))
        locator->saxreader->xmldecl_standalone = saxreader_alloc_string(locator, value);
    else
        saxreader_set_error(locator, E_SAX_INVALID_STANDALONE);
}

/*
static bool saxreader_parse_xmldecl(struct saxlocator *locator)
{
    WCHAR ch;

    saxreader_more(locator);

    ch = saxreader_get_char(locator, 5);
    if (!saxreader_peek(locator, L"<?xml", 5) || !saxreader_is_space(ch))
        return false;
    saxreader_skip(locator, 5);
    saxreader_skipspaces(locator);

    saxreader_parse_versioninfo(locator);

    if (saxreader_cmp(locator, L"?>"))
        return true;

    saxreader_parse_encdecl(locator);

    if (locator->saxreader->xmldecl_encoding)
    {
        if (saxreader_cmp(locator, L"?>"))
            return true;
        saxreader_skip_required_spaces(locator);
    }

    saxreader_skipspaces(locator);
    saxreader_parse_sddecl(locator);
    saxreader_skipspaces(locator);

    if (!saxreader_cmp(locator, L"?>"))
        saxreader_set_error(locator, E_SAX_BAD_XMLDECL);

    return true;
}
*/
static BSTR saxreader_parse_xmldecl_attribute(struct saxlocator *locator, struct parsed_name *name);

/* [85] BaseChar ::= ... */
static bool saxreader_is_basechar(WCHAR ch)
{
    return (ch >= 0x41 && ch <= 0x5a)
            || (ch >= 0x61 && ch <= 0x7a) || (ch >= 0xc0 && ch <= 0xd6)
            || (ch >= 0xd8 && ch <= 0xf6) || (ch >= 0xf8 && ch <= 0x131)
            || (ch >= 0x134 && ch <= 0x13e) || (ch >= 0x141 && ch <= 0x148)
            || (ch >= 0x14a && ch <= 0x17e) || (ch >= 0x180 && ch <= 0x1c3)
            || (ch >= 0x1cd && ch <= 0x1f0) || (ch >= 0x1f4 && ch <= 0x1f5)
            || (ch >= 0x1fa && ch <= 0x217) || (ch >= 0x250 && ch <= 0x2a8)
            || (ch >= 0x2bb && ch <= 0x2c1) || ch == 0x386 || (ch >= 0x388 && ch <= 0x38a)
            || ch == 0x38c || (ch >= 0x38e && ch <= 0x3a1)
            || (ch >= 0x3a3 && ch <= 0x3ce) || (ch >= 0x3d0 && ch <= 0x3d6)
            || ch == 0x3da || ch == 0x3dc || ch == 0x3de || ch == 0x3e0
            || (ch >= 0x3e2 && ch <= 0x3f3) || (ch >= 0x401 && ch <= 0x40c)
            || (ch >= 0x40e && ch <= 0x44f) || (ch >= 0x451 && ch <= 0x45c)
            || (ch >= 0x45e && ch <= 0x481) || (ch >= 0x490 && ch <= 0x4c4)
            || (ch >= 0x4c7 && ch <= 0x4c8) || (ch >= 0x4cb && ch <= 0x4cc)
            || (ch >= 0x4d0 && ch <= 0x4eb) || (ch >= 0x4ee && ch <= 0x4f5)
            || (ch >= 0x4f8 && ch <= 0x4f9) || (ch >= 0x531 && ch <= 0x556)
            || ch == 0x559 || (ch >= 0x561 && ch <= 0x586)
            || (ch >= 0x5d0 && ch <= 0x5ea) || (ch >= 0x5f0 && ch <= 0x5f2)
            || (ch >= 0x621 && ch <= 0x63a) || (ch >= 0x641 && ch <= 0x64a)
            || (ch >= 0x671 && ch <= 0x6b7) || (ch >= 0x6ba && ch <= 0x6be)
            || (ch >= 0x6c0 && ch <= 0x6ce) || (ch >= 0x6d0 && ch <= 0x6d3)
            || ch == 0x6d5 || (ch >= 0x6e5 && ch <= 0x6e6)
            || (ch >= 0x905 && ch <= 0x939) || ch == 0x93d || (ch >= 0x958 && ch <= 0x961)
            || (ch >= 0x985 && ch <= 0x98c) || (ch >= 0x98f && ch <= 0x990)
            || (ch >= 0x993 && ch <= 0x9a8) || (ch >= 0x9aa && ch <= 0x9b0)
            || ch == 0x9b2 || (ch >= 0x9b6 && ch <= 0x9b9)
            || (ch >= 0x9dc && ch <= 0x9dd) || (ch >= 0x9df && ch <= 0x9e1)
            || (ch >= 0x9f0 && ch <= 0x9f1) || (ch >= 0xa05 && ch <= 0xa0a)
            || (ch >= 0xa0f && ch <= 0xa10) || (ch >= 0xa13 && ch <= 0xa28)
            || (ch >= 0xa2a && ch <= 0xa30) || (ch >= 0xa32 && ch <= 0xa33)
            || (ch >= 0xa35 && ch <= 0xa36) || (ch >= 0xa38 && ch <= 0xa39)
            || (ch >= 0xa59 && ch <= 0xa5c)
            || ch == 0xa5e || (ch >= 0xa72 && ch <= 0xa74) || (ch >= 0xa85 && ch <= 0xa8b)
            || ch == 0xa8d || (ch >= 0xa8f && ch <= 0xa91)
            || (ch >= 0xa93 && ch <= 0xaa8) || (ch >= 0xaaa && ch <= 0xab0)
            || (ch >= 0xab2 && ch <= 0xab3) || (ch >= 0xab5 && ch <= 0xab9)
            || ch == 0xabd || ch == 0xae0 || (ch >= 0xb05 && ch <= 0xb0c)
            || (ch >= 0xb0f && ch <= 0xb10) || (ch >= 0xb13 && ch <= 0xb28)
            || (ch >= 0xb2a && ch <= 0xb30) || (ch >= 0xb32 && ch <= 0xb33)
            || (ch >= 0xb36 && ch <= 0xb39) || ch == 0xb3d || (ch >= 0xb5c && ch <= 0xb5d)
            || (ch >= 0xb5f && ch <= 0xb61) || (ch >= 0xb85 && ch <= 0xb8a)
            || (ch >= 0xb8e && ch <= 0xb90) || (ch >= 0xb92 && ch <= 0xb95)
            || (ch >= 0xb99 && ch <= 0xb9a) || ch == 0xb9c || (ch >= 0xb9e && ch <= 0xb9f)
            || (ch >= 0xba3 && ch <= 0xba4) || (ch >= 0xba8 && ch <= 0xbaa)
            || (ch >= 0xbae && ch <= 0xbb5) || (ch >= 0xbb7 && ch <= 0xbb9)
            || (ch >= 0xc05 && ch <= 0xc0c) || (ch >= 0xc0e && ch <= 0xc10)
            || (ch >= 0xc12 && ch <= 0xc28) || (ch >= 0xc2a && ch <= 0xc33)
            || (ch >= 0xc35 && ch <= 0xc39) || (ch >= 0xc60 && ch <= 0xc61)
            || (ch >= 0xc85 && ch <= 0xc8c) || (ch >= 0xc8e && ch <= 0xc90)
            || (ch >= 0xc92 && ch <= 0xca8) || (ch >= 0xcaa && ch <= 0xcb3)
            || (ch >= 0xcb5 && ch <= 0xcb9) || ch == 0xcde || (ch >= 0xce0 && ch <= 0xce1)
            || (ch >= 0xd05 && ch <= 0xd0c) || (ch >= 0xd0e && ch <= 0xd10)
            || (ch >= 0xd12 && ch <= 0xd28) || (ch >= 0xd2a && ch <= 0xd39)
            || (ch >= 0xd60 && ch <= 0xd61) || (ch >= 0xe01 && ch <= 0xe2e)
            || ch == 0xe30 || (ch >= 0xe32 && ch <= 0xe33)
            || (ch >= 0xe40 && ch <= 0xe45) || (ch >= 0xe81 && ch <= 0xe82)
            || ch == 0xe84 || (ch >= 0xe87 && ch <= 0xe88)
            || ch == 0xe8a || ch == 0xe8d || (ch >= 0xe94 && ch <= 0xe97)
            || (ch >= 0xe99 && ch <= 0xe9f) || (ch >= 0xea1 && ch <= 0xea3)
            || ch == 0xea5 || ch == 0xea7 || (ch >= 0xeaa && ch <= 0xeab)
            || (ch >= 0xead && ch <= 0xeae) || ch == 0xeb0 || (ch >= 0xeb2 && ch <= 0xeb3)
            || ch == 0xebd || (ch >= 0xec0 && ch <= 0xec4)
            || (ch >= 0xf40 && ch <= 0xf47) || (ch >= 0xf49 && ch <= 0xf69)
            || (ch >= 0x10a0 && ch <= 0x10c5) || (ch >= 0x10d0 && ch <= 0x10f6)
            || ch == 0x1100 || (ch >= 0x1102 && ch <= 0x1103)
            || (ch >= 0x1105 && ch <= 0x1107) || ch == 0x1109 || (ch >= 0x110b && ch <= 0x110c)
            || (ch >= 0x110e && ch <= 0x1112) || ch == 0x113c || ch == 0x113e || ch == 0x1140
            || ch == 0x114c || ch == 0x114e || ch == 0x1150
            || (ch >= 0x1154 && ch <= 0x1155) || ch == 0x1159 || (ch >= 0x115f && ch <= 0x1161)
            || ch == 0x1163 || ch == 0x1165 || ch == 0x1167
            || ch == 0x1169 || (ch >= 0x116d && ch <= 0x116e)
            || (ch >= 0x1172 && ch <= 0x1173) || ch == 0x1175 || ch == 0x119e || ch == 0x11a8
            || ch == 0x11ab || (ch >= 0x11ae && ch <= 0x11af)
            || (ch >= 0x11b7 && ch <= 0x11b8) || ch == 0x11ba || (ch >= 0x11bc && ch <= 0x11c2)
            || ch == 0x11eb || ch == 0x11f0 || ch == 0x11f9
            || (ch >= 0x1e00 && ch <= 0x1e9b) || (ch >= 0x1ea0 && ch <= 0x1ef9)
            || (ch >= 0x1f00 && ch <= 0x1f15) || (ch >= 0x1f18 && ch <= 0x1f1d)
            || (ch >= 0x1f20 && ch <= 0x1f45) || (ch >= 0x1f48 && ch <= 0x1f4d)
            || (ch >= 0x1f50 && ch <= 0x1f57) || ch == 0x1f59 || ch == 0x1f5b || ch == 0x1f5d
            || (ch >= 0x1f5f && ch <= 0x1f7d) || (ch >= 0x1f80 && ch <= 0x1fb4)
            || (ch >= 0x1fb6 && ch <= 0x1fbc) || ch == 0x1fbe || (ch >= 0x1fc2 && ch <= 0x1fc4)
            || (ch >= 0x1fc6 && ch <= 0x1fcc) || (ch >= 0x1fd0 && ch <= 0x1fd3)
            || (ch >= 0x1fd6 && ch <= 0x1fdb) || (ch >= 0x1fe0 && ch <= 0x1fec)
            || (ch >= 0x1ff2 && ch <= 0x1ff4) || (ch >= 0x1ff6 && ch <= 0x1ffc)
            || ch == 0x2126 || (ch >= 0x212a && ch <= 0x212b)
            || ch == 0x212e || (ch >= 0x2180 && ch <= 0x2182)
            || (ch >= 0x3041 && ch <= 0x3094) || (ch >= 0x30a1 && ch <= 0x30fa)
            || (ch >= 0x3105 && ch <= 0x312c) || (ch >= 0xac00 && ch <= 0xd7a3);
}

/* [86] Ideographic ::= [#x4E00-#x9FA5] | #x3007 | [#x3021-#x3029] */
static bool saxreader_is_ideographic(WCHAR ch)
{
    return (ch >= 0x4e00 && ch <= 0x9fa5) || ch == 0x3007
            || (ch >= 0x3021 && ch <= 0x3029);
}

/* [84] Letter ::= BaseChar | Ideographic */
static bool saxreader_is_letter(WCHAR ch)
{
    return saxreader_is_basechar(ch) || saxreader_is_ideographic(ch);
}

/* [87] CombiningChar ::= ... */
static bool saxreader_is_combiningchar(WCHAR ch)
{
    return (ch >= 0x300 && ch <= 0x345) || (ch >= 0x360 && ch <= 0x361)
            || (ch >= 0x483 && ch <= 0x486) || (ch >= 0x591 && ch <= 0x5a1) || ch == 0x670
            || (ch >= 0x6d6 && ch <= 0x6e4) || (ch >= 0x6e7 && ch <= 0x6e8)
            || (ch >= 0x6ea && ch <= 0x6ed) || (ch >= 0x901 && ch <= 0x903) || ch == 0x93c
            || (ch >= 0x93e && ch <= 0x94d) || (ch >= 0x951 && ch <= 0x954)
            || (ch >= 0x962 && ch <= 0x963) || (ch >= 0x981 && ch <= 0x983) || ch == 0x9bc
            || (ch >= 0x9be && ch <= 0x9bf) || (ch >= 0x9c0 && ch <= 0x9c4)
            || (ch >= 0x9c7 && ch <= 0x9c8) || (ch >= 0x9cb && ch <= 0x9cd) || ch == 0x9d7
            || (ch >= 0x9e2 && ch <= 0x9e3) || ch == 0xa02
            || ch == 0xa3c || (ch >= 0xa3e && ch <= 0xa42)
            || (ch >= 0xa47 && ch <= 0xa48) || (ch >= 0xa4b && ch <= 0xa4d)
            || (ch >= 0xa70 && ch <= 0xa71) || (ch >= 0xa81 && ch <= 0xa83)
            || ch == 0xabc || (ch >= 0xabe && ch <= 0xac5)
            || (ch >= 0xac7 && ch <= 0xac9) || (ch >= 0xacb && ch <= 0xacd)
            || (ch >= 0xb01 && ch <= 0xb03) || ch == 0xb3c || (ch >= 0xb3e && ch <= 0xb43)
            || (ch >= 0xb47 && ch <= 0xb48) || (ch >= 0xb4b && ch <= 0xb4d)
            || (ch >= 0xb56 && ch <= 0xb57) || (ch >= 0xb82 && ch <= 0xb83)
            || (ch >= 0xbbe && ch <= 0xbc2) || (ch >= 0xbc6 && ch <= 0xbc8)
            || (ch >= 0xbca && ch <= 0xbcd)
            || ch == 0xbd7 || (ch >= 0xc01 && ch <= 0xc03)
            || (ch >= 0xc3e && ch <= 0xc44) || (ch >= 0xc46 && ch <= 0xc48)
            || (ch >= 0xc4a && ch <= 0xc4d) || (ch >= 0xc55 && ch <= 0xc56)
            || (ch >= 0xc82 && ch <= 0xc83) || (ch >= 0xcbe && ch <= 0xcc4)
            || (ch >= 0xcc6 && ch <= 0xcc8) || (ch >= 0xcca && ch <= 0xccd)
            || (ch >= 0xcd5 && ch <= 0xcd6) || (ch >= 0xd02 && ch <= 0xd03)
            || (ch >= 0xd3e && ch <= 0xd43) || (ch >= 0xd46 && ch <= 0xd48)
            || (ch >= 0xd4a && ch <= 0xd4d) || ch == 0xd57 || ch == 0xe31
            || (ch >= 0xe34 && ch <= 0xe3a) || (ch >= 0xe47 && ch <= 0xe4e)
            || ch == 0xeb1 || (ch >= 0xeb4 && ch <= 0xeb9)
            || (ch >= 0xebb && ch <= 0xebc) || (ch >= 0xec8 && ch <= 0xecd)
            || (ch >= 0xf18 && ch <= 0xf19) || ch == 0xf35 || ch == 0xf37 || ch == 0xf39
            || (ch >= 0xf3e && ch <= 0xf3f) || (ch >= 0xf71 && ch <= 0xf84)
            || (ch >= 0xf86 && ch <= 0xf8b) || (ch >= 0xf90 && ch <= 0xf95)
            || ch == 0xf97 || (ch >= 0xf99 && ch <= 0xfad)
            || (ch >= 0xfb1 && ch <= 0xfb7) || ch == 0xfb9 || (ch >= 0x20d0 && ch <= 0x20dc)
            || ch == 0x20e1 || (ch >= 0x302a && ch <= 0x302f)
            || ch == 0x3099 || ch == 0x309a;
}

/* [88] Digit ::= [#x0030-#x0039] | [#x0660-#x0669] | [#x06F0-#x06F9] | [#x0966-#x096F] |
        [#x09E6-#x09EF] | [#x0A66-#x0A6F] | [#x0AE6-#x0AEF] | [#x0B66-#x0B6F] |
        [#x0BE7-#x0BEF] | [#x0C66-#x0C6F] | [#x0CE6-#x0CEF] | [#x0D66-#x0D6F] |
        [#x0E50-#x0E59] | [#x0ED0-#x0ED9] | [#x0F20-#x0F29] */
static bool saxreader_is_digit(WCHAR ch)
{
    return (ch >= 0x30 && ch <= 0x39) || (ch >= 0x660 && ch <= 0x669)
            || (ch >= 0x6f0 && ch <= 0x6f9) || (ch >= 0x966 && ch <= 0x96f)
            || (ch >= 0x9e6 && ch <= 0x9ef) || (ch >= 0xa66 && ch <= 0xa6f)
            || (ch >= 0xae6 && ch <= 0xaef) || (ch >= 0xb66 && ch <= 0xb6f)
            || (ch >= 0xbe7 && ch <= 0xbef) || (ch >= 0xc66 && ch <= 0xc6f)
            || (ch >= 0xce6 && ch <= 0xcef) || (ch >= 0xd66 && ch <= 0xd6f)
            || (ch >= 0xe50 && ch <= 0xe59) || (ch >= 0xed0 && ch <= 0xed9)
            || (ch >= 0xf20 && ch <= 0xf29);
}

/* [89] Extender ::= #x00B7 | #x02D0 | #x02D1 | #x0387 | #x0640 | #x0E46 | #x0EC6 | #x3005 |
        [#x3031-#x3035] | [#x309D-#x309E] | [#x30FC-#x30FE] */
static bool saxreader_is_extender(WCHAR ch)
{
    return ch == 0xb7 || ch == 0x2d0 || ch == 0x2d1 || ch == 0x387 || ch == 0x640 || ch == 0xe46
            || ch == 0xec6 || ch == 0x3005 || (ch >= 0x3031 && ch <= 0x3035)
            || (ch >= 0x309d && ch <= 0x309e) || (ch >= 0x30fc && ch <= 0x30fe);
}

/* [4] NameChar ::= Letter | Digit | '.' | '-' | '_' | ':' | CombiningChar | Extender */
static bool saxreader_is_namechar(WCHAR ch)
{
    return saxreader_is_letter(ch)
            || saxreader_is_digit(ch)
            || ch == '.' || ch == '-' || ch == '_' || ch == ':'
            || saxreader_is_combiningchar(ch)
            || saxreader_is_extender(ch);
}

/* [5] NCNameChar ::= NameChar - ':' */
static bool saxreader_is_ncnamechar(WCHAR ch)
{
    return saxreader_is_namechar(ch) && ch != ':';
}

/* [6] NCNameStartChar ::= Letter | '_' */
static bool saxreader_is_ncname_startchar(WCHAR ch)
{
    return saxreader_is_letter(ch) || ch == '_';
}

static void *saxreader_calloc(struct saxlocator *locator, size_t count, size_t size)
{
    void *ret;

    if (!(ret = calloc(count, size)))
        saxreader_set_error(locator, E_OUTOFMEMORY);

    return ret;
}

/* [7] Nmtoken ::= (NameChar)+ */
static void saxreader_parse_nmtoken(struct saxlocator *locator, BSTR *token)
{
    WCHAR ch = *saxreader_get_ptr(locator), *ptr;
    UINT len = 0;

    *token = NULL;

    while (saxreader_is_namechar(ch))
    {
        saxreader_skip(locator, 1);
        ch = *saxreader_get_ptr(locator);
        ++len;
    }

    if (len)
    {
        ptr = saxreader_get_ptr(locator);
        *token = saxreader_alloc_stringlen(locator, ptr - len, len);
    }
    else
    {
        saxreader_set_error(locator, E_SAX_BADNAMECHAR);
    }
}

/* [5] Name ::= (Letter | '_' | ':') (NameChar)*

   Disallow leading ':', and optionally check for it, without stopping. */
static void saxreader_parse_name_opt(struct saxlocator *locator, bool no_colons, BSTR *name)
{
    WCHAR ch = *saxreader_get_ptr(locator), *ptr;
    UINT len, col_count = 0;

    *name = NULL;

    if (!saxreader_is_letter(ch) && ch != '_')
    {
        saxreader_set_error(locator, E_SAX_BADSTARTNAMECHAR);
        return;
    }

    len = 0;
    while (saxreader_is_namechar(ch))
    {
        if (ch == ':') ++col_count;
        saxreader_skip(locator, 1);
        ch = *saxreader_get_ptr(locator);
        ++len;
    }

    if (no_colons && col_count)
    {
        saxreader_set_error(locator, E_SAX_CONTAINSCOLON);
    }
    else if (col_count > 1)
    {
        saxreader_set_error(locator, E_SAX_MULTIPLE_COLONS);
    }
    else if (len)
    {
        ptr = saxreader_get_ptr(locator);
        *name = saxreader_alloc_stringlen(locator, ptr - len, len);
    }
    else
    {
        saxreader_set_error(locator, E_SAX_BADNAMECHAR);
    }
}

static void saxreader_parse_name(struct saxlocator *locator, BSTR *name)
{
    saxreader_parse_name_opt(locator, false, name);
}

/* Parse the name, then abort if it contains a colon.

   Used for entities, PIs, notations and most of attribute values */
static void saxreader_parse_name_strict(struct saxlocator *locator, BSTR *name)
{
    saxreader_parse_name_opt(locator, true, name);
}

/* [4 NS] NCName ::= NCNameStartChar NCNameChar* */
static BSTR saxreader_parse_ncname(struct saxlocator *locator)
{
    struct string_buffer buffer = { 0 };
    WCHAR ch;

    if (locator->status != S_OK)
        return NULL;

    ch = *saxreader_get_ptr(locator);
    if (!saxreader_is_ncname_startchar(ch))
    {
        saxreader_set_error(locator, E_SAX_BADSTARTNAMECHAR);
        return NULL;
    }

    do
    {
        saxreader_string_append(locator, &buffer, &ch, 1);
        saxreader_skip(locator, 1);
        ch = *saxreader_get_ptr(locator);
    }
    while (saxreader_is_ncnamechar(ch) && locator->status == S_OK);

    saxreader_shrink(locator);

    return saxreader_string_to_bstr(locator, &buffer);
}

/* [7 NS]  QName ::= PrefixedName | UnprefixedName
   [8 NS]  PrefixedName ::= Prefix ':' LocalPart
   [9 NS]  UnprefixedName ::= LocalPart
   [10 NS] Prefix ::= NCName */
static void saxreader_parse_qname(struct saxlocator *locator, struct parsed_name *name)
{
    struct string_buffer buffer = { 0 };
    BSTR ncname;

    memset(name, 0, sizeof(*name));

    ncname = saxreader_parse_ncname(locator);

    if (saxreader_cmp(locator, L":"))
    {
        name->prefix = ncname;
        name->local = saxreader_parse_ncname(locator);
        if (*saxreader_get_ptr(locator) == ':')
            saxreader_set_error(locator, E_SAX_MULTIPLE_COLONS);
    }
    else
    {
        name->local = ncname;
    }

    if (name->prefix)
    {
        saxreader_string_append(locator, &buffer, name->prefix, SysStringLen(name->prefix));
        saxreader_string_append(locator, &buffer, L":", 1);
    }
    saxreader_string_append(locator, &buffer, name->local, SysStringLen(name->local));
    name->qname = saxreader_string_to_bstr(locator, &buffer);
}

static const WCHAR * validate_ncname(const WCHAR *name)
{
    const WCHAR *p = name;

    if (!saxreader_is_ncname_startchar(*p++))
        return NULL;

    while (*p && *p != ':')
    {
        if (!saxreader_is_ncnamechar(*p)) return NULL;
        ++p;
    }

    return p;
}

bool parser_is_valid_qualified_name(const WCHAR *name)
{
    const WCHAR *p;

    p = validate_ncname(name);
    if (!p)
        return false;

    if (*p == ':')
        p = validate_ncname(++p);

    return p && !*p;
}

static struct element *saxreader_new_element(struct saxlocator *locator, struct parsed_name *name)
{
    struct element *element;

    if (!(element = saxreader_calloc(locator, 1, sizeof(*element))))
        return NULL;
    list_add_head(&locator->elements, &element->entry);

    element->name = *name;
    saxreader_clear_attributes(locator);

    return element;
}

static void saxreader_free_name(struct parsed_name *name)
{
    SysFreeString(name->prefix);
    SysFreeString(name->local);
    SysFreeString(name->qname);
    memset(name, 0, sizeof(*name));
}

static void saxreader_free_element(struct element *element)
{
    saxreader_free_name(&element->name);
    free(element);
}

static bool bstr_equal(BSTR s1, BSTR s2)
{
    if (SysStringLen(s1) != SysStringLen(s2)) return false;
    return !memcmp(s1, s2, SysStringLen(s1) * sizeof(WCHAR));
}

static bool bstr_startswith(BSTR s, const WCHAR *prefix, UINT length)
{
    if (SysStringLen(s) < length) return false;
    return !memcmp(s, prefix, length * sizeof(WCHAR));
}

static const struct ns *saxreader_get_namespace(struct saxlocator *locator, BSTR prefix)
{
    struct element *element;

    LIST_FOR_EACH_ENTRY(element, &locator->elements, struct element, entry)
    {
        for (size_t i = 0; i < element->ns.count; ++i)
        {
            if (bstr_equal(element->ns.entries[i].prefix, prefix))
                return &element->ns.entries[i];
        }
    }

    return NULL;
}

static void saxreader_add_namespace(struct saxlocator *locator, struct element *element, BSTR prefix, BSTR uri)
{
    struct ns *ns;

    if (bstr_startswith(prefix, L"xml", 3))
        return saxreader_set_error(locator, E_SAX_RESERVEDNAMESPACE);

    if (!saxreader_array_reserve(locator, (void **)&element->ns.entries, &element->ns.capacity,
            element->ns.count + 1, sizeof(*element->ns.entries)))
    {
        return;
    }

    ns = &element->ns.entries[element->ns.count++];
    ns->prefix = saxreader_alloc_string(locator, prefix);
    ns->uri = saxreader_alloc_string(locator, uri);
}

static bool saxreader_has_attribute(struct saxlocator *locator, BSTR name)
{
    for (size_t i = 0; i < locator->attributes.count; ++i)
    {
        if (bstr_equal(locator->attributes.entries[i].qname, name))
            return true;
    }

    return false;
}

static void saxreader_add_attribute(struct saxlocator *locator, const struct parsed_name *name, BSTR value, bool nsdef)
{
    struct attribute *attr;

    if (nsdef && !(locator->saxreader->features & NamespacePrefixes))
        return;

    if (saxreader_has_attribute(locator, name->qname))
        return saxreader_set_error(locator, E_SAX_DUPLICATEATTRIBUTE);

    if (!saxreader_array_reserve(locator, (void **)&locator->attributes.entries, &locator->attributes.capacity,
            locator->attributes.count + 1, sizeof(*locator->attributes.entries)))
    {
        return;
    }

    attr = &locator->attributes.entries[locator->attributes.count++];
    attr->prefix = name->prefix ? saxreader_alloc_string(locator, name->prefix) : NULL;
    attr->local = saxreader_alloc_string(locator, nsdef ? L"" : name->local);
    attr->qname = saxreader_alloc_string(locator, name->qname);
    attr->value = saxreader_alloc_string(locator, value);
    attr->nsdef = nsdef;
}

static void saxreader_add_default_attributes(struct saxlocator *locator, struct element *element)
{
    struct attlist_decl *decl;

    LIST_FOR_EACH_ENTRY(decl, &locator->dtd.attr_decls, struct attlist_decl, entry)
    {
        if (bstr_equal(decl->name, element->name.qname))
        {
            for (size_t i = 0; i < decl->count; ++i)
            {
                const struct attlist_attr *attr = &decl->attributes[i];

                if (!attr->value)
                    continue;

                if (!saxreader_has_attribute(locator, attr->name.qname))
                    saxreader_add_attribute(locator, &attr->name, attr->value, false);
            }
        }
    }
}

static void saxreader_set_element_uri(struct saxlocator *locator, struct element *element)
{
    struct attribute *attr;
    const struct ns *ns;

    if (element->name.prefix)
    {
        if (!(ns = saxreader_get_namespace(locator, element->name.prefix)))
            return saxreader_set_error(locator, E_SAX_UNDECLAREDPREFIX);
        element->uri = ns->uri;
    }
    else if ((ns = saxreader_get_namespace(locator, locator->saxreader->empty_bstr)))
    {
        element->uri = ns->uri;
    }

    for (size_t i = 0; i < locator->attributes.count; ++i)
    {
        attr = &locator->attributes.entries[i];

        if (attr->nsdef)
        {
            attr->uri = locator->xmlns_uri;
        }
        else if (!attr->prefix)
        {
            attr->uri = locator->null_uri;
        }
        else if (!wcscmp(attr->prefix, L"xml"))
        {
            attr->uri = locator->xml_uri;
        }
        else if (!(ns = saxreader_get_namespace(locator, attr->prefix)))
        {
            return saxreader_set_error(locator, E_SAX_UNDECLAREDPREFIX);
        }
        else
        {
            attr->uri = ns->uri;
        }
        saxreader_free_bstr(&attr->prefix);
    }
}

/* [2] Char ::= #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF] */
static bool saxreader_is_char(UINT32 ch)
{
    return (ch == 0x9) || (ch == 0xa) || (ch == 0xd) ||
           (ch >= 0x20 && ch <= 0xd7ff) ||
           (ch >= 0xe000 && ch <= 0xfffd) ||
           (ch >= 0x10000 && ch <= 0x10ffff);
}

/* [66] CharRef ::= '&#' [0-9]+ ';' | '&#x' [0-9a-fA-F]+ ';' */
static void saxreader_parse_charref(struct saxlocator *locator, struct string_buffer *buffer,
        struct text_position *position)
{
    WCHAR u16[3], *ptr;
    unsigned int len;
    UINT32 ch = 0;

    /* Skip &# */
    saxreader_skip(locator, 2);
    *position = locator->buffer.position;

    ptr = saxreader_get_ptr(locator);

    /* hex char or decimal */
    if (*ptr == 'x')
    {
        saxreader_skip(locator, 1);
        ptr = saxreader_get_ptr(locator);

        while (*ptr != ';')
        {
            if ((*ptr >= '0' && *ptr <= '9'))
                ch = ch*16 + *ptr - '0';
            else if ((*ptr >= 'a' && *ptr <= 'f'))
                ch = ch*16 + *ptr - 'a' + 10;
            else if ((*ptr >= 'A' && *ptr <= 'F'))
                ch = ch*16 + *ptr - 'A' + 10;
            else
                return saxreader_set_error(locator, E_SAX_BADCHARINENTREF);

            if (ch > 0x10ffff)
                return saxreader_set_error(locator, E_SAX_INVALID_UNICODE);

            saxreader_skip(locator, 1);
            ptr = saxreader_get_ptr(locator);
        }
    }
    else
    {
        while (*ptr != ';')
        {
            if (!(*ptr >= '0' && *ptr <= '9'))
                return saxreader_set_error(locator, E_SAX_BADCHARINENTREF);

            ch = ch*10 + *ptr - '0';
            saxreader_skip(locator, 1);
            ptr = saxreader_get_ptr(locator);

            if (ch > 0x10ffff)
                return saxreader_set_error(locator, E_SAX_INVALID_UNICODE);
        }
    }
    /* For the closing ';' */
    saxreader_skip(locator, 1);

    if (!saxreader_is_char(ch))
        return saxreader_set_error(locator, E_SAX_INVALID_UNICODE);

    len = saxreader_convert_u32_to_u16(ch, u16);
    saxreader_string_append(locator, buffer, u16, len);
}

static struct entity_decl *saxreader_get_entity(struct saxlocator *locator, BSTR name)
{
    struct entity_decl *entity;

    if (!name)
        return NULL;

    LIST_FOR_EACH_ENTRY(entity, &locator->dtd.entities, struct entity_decl, entry)
    {
        if (bstr_equal(entity->name, name))
            return entity;
    }

    return NULL;
}

static const WCHAR *saxreader_get_predefined_entity(BSTR name)
{
    if (!wcscmp(name, L"amp")) return L"&";
    if (!wcscmp(name, L"lt")) return L"<";
    if (!wcscmp(name, L"gt")) return L">";
    if (!wcscmp(name, L"apos")) return L"'";
    if (!wcscmp(name, L"quot")) return L"\"";
    return NULL;
}

static void saxreader_stringify_entity(struct saxlocator *locator, struct string_buffer *buffer, BSTR name)
{
    struct entity_decl *entity;
    const WCHAR *value;

    if ((value = saxreader_get_predefined_entity(name)))
        return saxreader_string_append(locator, buffer, value, 1);

    if (!(entity = saxreader_get_entity(locator, name)))
        return saxreader_set_error(locator, E_SAX_UNDEFINEDREF);

    if (entity->visited)
        return saxreader_set_error(locator, E_SAX_INFINITEREFLOOP);

    for (size_t i = 0; i < entity->content.count && locator->status == S_OK; ++i)
    {
        const struct entity_part *part = &entity->content.parts[i];

        if (part->reference)
        {
            entity->visited = true;
            saxreader_stringify_entity(locator, buffer, part->value);
            entity->visited = false;
        }
        else
        {
            saxreader_string_append(locator, buffer, part->value, SysStringLen(part->value));
        }
    }
}

/* [10] AttValue ::= '"' ([^<&"] | Reference)* '"'
        |  "'" ([^<&'] | Reference)* "'" */
static BSTR saxreader_parse_attvalue(struct saxlocator *locator)
{
    struct string_buffer buffer = { 0 };
    struct text_position position;
    WCHAR quote[2] = { 0 }, ch;
    bool normalized;
    BSTR name;

    if (!(quote[0] = saxreader_is_quote(locator)))
    {
        saxreader_set_error(locator, E_SAX_MISSINGQUOTE);
        return NULL;
    }
    saxreader_skip(locator, 1);

    /* All references are resolved */

    while (locator->status == S_OK)
    {
        if (saxreader_cmp(locator, quote))
            break;

        if (saxreader_cmp(locator, L"<"))
        {
            saxreader_set_error(locator, E_SAX_BADCHARINSTRING);
            break;
        }

        if (saxreader_peek(locator, L"&#", 2))
        {
            saxreader_parse_charref(locator, &buffer, &position);
        }
        else if (saxreader_peek(locator, L"&", 1))
        {
            /* Skip '&' */
            saxreader_skip(locator, 1);

            saxreader_parse_name(locator, &name);
            if (saxreader_cmp(locator, L";"))
                saxreader_stringify_entity(locator, &buffer, name);
            else
                saxreader_set_error(locator, E_SAX_INVALID_UNICODE);
            SysFreeString(name);
        }
        else
        {
            normalized = false;
            if (locator->saxreader->features & NormalizeAttributeValues)
            {
                if (saxreader_cmp(locator, L"\r\n")
                        || saxreader_cmp(locator, L"\r")
                        || saxreader_cmp(locator, L"\n")
                        || saxreader_cmp(locator, L"\t"))
                {
                    saxreader_string_append(locator, &buffer, L" ", 1);
                    normalized = true;
                }
            }
            else
            {
                /* Potentially could happen on read */
                if (saxreader_cmp(locator, L"\r\n"))
                {
                    saxreader_string_append(locator, &buffer, L"\n", 1);
                    normalized = true;
                }
            }

            if (!normalized)
            {
                ch = *saxreader_get_ptr_noread(locator);
                if (!ch)
                {
                    saxreader_set_error(locator, E_SAX_BADCHARINSTRING);
                    break;
                }
                saxreader_string_append(locator, &buffer, &ch, 1);
                saxreader_skip(locator, 1);
            }
        }
    }

    if (locator->status != S_OK) return NULL;

    return saxreader_string_to_bstr(locator, &buffer);
}

static bool saxreader_add_namespace_attribute(struct saxlocator *locator, struct element *element,
        struct parsed_name *name, BSTR value)
{
    if (name->prefix && !wcscmp(name->prefix, L"xmlns"))
        saxreader_add_namespace(locator, element, name->local, value);
    else if (!name->prefix && name->local && !wcscmp(name->local, L"xmlns"))
        saxreader_add_namespace(locator, element, locator->saxreader->empty_bstr, value);
    else
        return false;

    return true;
}

/* [15 NS] Attribute ::= NSAttName Eq AttValue | QName Eq AttValue */
static void saxreader_parse_attribute(struct saxlocator *locator, struct element *element)
{
    struct parsed_name name;
    BSTR value;
    bool ns;

    saxreader_parse_qname(locator, &name);
    saxreader_skipspaces(locator);
    if (!saxreader_cmp(locator, L"="))
        saxreader_set_error(locator, E_SAX_MISSINGEQUALS);
    saxreader_skipspaces(locator);
    value = saxreader_parse_attvalue(locator);

    ns = saxreader_add_namespace_attribute(locator, element, &name, value);
    saxreader_add_attribute(locator, &name, value, ns);

    saxreader_free_name(&name);
    SysFreeString(value);
}

static BSTR saxreader_parse_xmldecl_attribute(struct saxlocator *locator, struct parsed_name *name)
{
    BSTR value;

    saxreader_parse_qname(locator, name);
    saxreader_skipspaces(locator);
    if (!saxreader_cmp(locator, L"="))
        saxreader_set_error(locator, E_SAX_MISSINGEQUALS);
    saxreader_skipspaces(locator);
    value = saxreader_parse_attvalue(locator);

    return value;
}

/* [23] XMLDecl ::= '<?xml' VersionInfo EncodingDecl? SDDecl? S? '?>' */
static void saxreader_parse_xmldecl(struct saxlocator *locator)
{
    struct parsed_name name;
    bool done = false;
    BSTR value;
    WCHAR ch;

    saxreader_more(locator);

    ch = saxreader_get_char(locator, 5);
    if (!saxreader_peek(locator, L"<?xml", 5) || !saxreader_is_space(ch))
        return;
    saxreader_skip(locator, 5);
    saxreader_skipspaces(locator);

    value = saxreader_parse_xmldecl_attribute(locator, &name);
    saxreader_parse_versioninfo(locator, &name, value);
    saxreader_free_name(&name);
    SysFreeString(value);

    saxreader_skipspaces(locator);
    if (saxreader_cmp(locator, L"?>"))
        return;

    while (!done && locator->status == S_OK)
    {
        value = saxreader_parse_xmldecl_attribute(locator, &name);

        if (!wcscmp(name.qname, L"encoding"))
        {
            saxreader_parse_encdecl(locator, &name, value);
        }
        else if (!wcscmp(name.qname, L"standalone"))
        {
            saxreader_parse_sddecl(locator, &name, value);
            done = true;
        }
        else
        {
            saxreader_set_error(locator, E_SAX_UNEXPECTED_ATTRIBUTE);
            continue;
        }

        saxreader_free_name(&name);
        SysFreeString(value);

        saxreader_skipspaces(locator);
        if (saxreader_cmp(locator, L"?>"))
            return;
    }

    saxreader_skipspaces(locator);
    if (!saxreader_cmp(locator, L"?>"))
        saxreader_set_error(locator, E_SAX_BAD_XMLDECL);
}

static void saxreader_reorder_attributes(struct saxlocator *locator)
{
    size_t count, i, idx;

    if (locator->saxreader->version < MSXML4)
        return;
    if (!(locator->saxreader->features & Namespaces))
        return;

    count = locator->attributes.count;
    if (!(locator->attributes.map = saxreader_calloc(locator, count, sizeof(*locator->attributes.map))))
        return;

    /* Regular attributes first, then namespace definitions. */

    for (i = 0, idx = 0; i < count; ++i)
    {
        if (locator->attributes.entries[i].nsdef) continue;
        locator->attributes.map[idx++] = i;
    }

    for (i = 0; i < count; ++i)
    {
        if (!locator->attributes.entries[i].nsdef) continue;
        locator->attributes.map[idx++] = i;
    }
}

/* [12 NS] STag ::= '<' QName (S Attribute)* S? '>'
   [14 NS] EmptyElemTag ::= '<' QName (S Attribute)* S? '/>' */
static void saxreader_parse_starttag(struct saxlocator *locator)
{
    struct text_position position = { 0 };
    bool empty_element = false;
    struct element *element;
    struct parsed_name name;

    if (!saxreader_cmp(locator, L"<"))
        return saxreader_set_error(locator, E_SAX_INVALIDATROOTLEVEL);

    saxreader_parse_qname(locator, &name);

    if (bstr_startswith(name.prefix, L"xml", 3))
    {
        saxreader_free_name(&name);
        return saxreader_set_error(locator, E_SAX_RESERVEDNAMESPACE);
    }

    if (!(element = saxreader_new_element(locator, &name)))
    {
        saxreader_free_name(&name);
        return;
    }

    while (locator->status == S_OK)
    {
        saxreader_skipspaces(locator);

        if (saxreader_cmp(locator, L"/>"))
        {
            empty_element = true;
            position = locator->buffer.position;
            break;
        }

        if (saxreader_cmp(locator, L">"))
        {
            position = locator->buffer.position;
            break;
        }

        saxreader_parse_attribute(locator, element);
    }

    /* Append default attributes from DTD. */
    saxreader_add_default_attributes(locator, element);

    saxreader_reorder_attributes(locator);

    /* The 'uri' string pointers are reused from the namespace stacks.
       Namespace definitions can appear after attributes that are using them,
       so we have to reiterate after all attributes were parsed. */
    saxreader_set_element_uri(locator, element);

    saxlocator_start_element(locator, &position, element);
    if (empty_element)
    {
        saxlocator_end_element(locator, &position, element);
        list_remove(&element->entry);
        saxreader_free_element(element);
    }
}

/* [13 NS] ETag ::= '</' QName S? '>' */
static void saxreader_parse_endtag(struct saxlocator *locator)
{
    struct text_position position;
    struct element *element;
    struct parsed_name name;
    HRESULT hr = S_OK;

    /* Skip '</' */
    saxreader_skip(locator, 2);
    position = locator->buffer.position;

    saxreader_parse_qname(locator, &name);
    saxreader_skipspaces(locator);

    if (!saxreader_cmp(locator, L">"))
        hr = E_SAX_UNCLOSEDENDTAG;
    else if (list_empty(&locator->elements))
        hr = E_SAX_UNEXPECTEDENDTAG;

    if (SUCCEEDED(hr))
    {
        element = LIST_ENTRY(list_head(&locator->elements), struct element, entry);

        if (!wcscmp(element->name.qname, name.qname))
            saxlocator_end_element(locator, &position, element);
        else
            hr = E_SAX_ENDTAGMISMATCH;

        list_remove(&element->entry);
        saxreader_free_element(element);
    }

    saxreader_free_name(&name);
    saxreader_set_error(locator, hr);
}

static bool saxreader_is_xml_pi(BSTR name)
{
    if (SysStringLen(name) != 3) return false;
    return (name[0] == 'x' || name[0] == 'X')
            && (name[1] == 'm' || name[1] == 'M')
            && (name[2] == 'l' || name[2] == 'L');
}

/* [16] PI ::= '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
   [17] PITarget ::= Name - (('X' | 'x') ('M' | 'm') ('L' | 'l')) */
static void saxreader_parse_pi(struct saxlocator *locator)
{
    struct string_buffer buffer = { 0 };
    struct text_position position;
    BSTR target, chars;
    WCHAR ch;

    /* Skip '<?' */
    saxreader_skip(locator, 2);
    saxreader_parse_name_strict(locator, &target);

    if (saxreader_is_xml_pi(target))
    {
        SysFreeString(target);
        return saxreader_set_error(locator, E_SAX_PIDECLSYNTAX);
    }

    saxreader_skipspaces(locator);
    position = locator->buffer.position;

    ch = *saxreader_get_ptr_noread(locator);
    while (ch && locator->status == S_OK)
    {
        if (saxreader_cmp(locator, L"?>"))
        {
            chars = saxreader_string_to_bstr(locator, &buffer);

            saxlocator_pi(locator, &position, target, chars);

            SysFreeString(target);
            SysFreeString(chars);

            return;
        }

        if (locator->saxreader->features & NormalizeLineBreaks
                && (saxreader_cmp(locator, L"\r\n") || saxreader_cmp(locator, L"\r")))
        {
            saxreader_string_append(locator, &buffer, L"\n", 1);
        }
        else
        {
            saxreader_string_append(locator, &buffer, &ch, 1);
            saxreader_skip(locator, 1);
        }
        ch = *saxreader_get_ptr_noread(locator);
    }

    SysFreeString(target);
    saxreader_set_error(locator, E_SAX_UNCLOSEDPI);
}

/* [15] Comment ::= '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->' */
static void saxreader_parse_comment(struct saxlocator *locator)
{
    struct string_buffer buffer = { 0 };
    struct text_position position;
    BSTR chars;
    WCHAR ch;

    /* Skip <!-- */
    saxreader_skip(locator, 4);
    position = locator->buffer.position;

    ch = *saxreader_get_ptr_noread(locator);
    while (ch && locator->status == S_OK)
    {
        if (saxreader_cmp(locator, L"-->"))
        {
            chars = saxreader_string_to_bstr(locator, &buffer);
            saxlocator_comment(locator, &position, chars);
            SysFreeString(chars);
            return;
        }
        else if (saxreader_cmp(locator, L"--"))
        {
            saxreader_set_error(locator, E_SAX_COMMENTSYNTAX);
            break;
        }
        else
        {
            if (locator->saxreader->features & NormalizeLineBreaks
                    && (saxreader_cmp(locator, L"\r\n") || saxreader_cmp(locator, L"\r")))
            {
                saxreader_string_append(locator, &buffer, L"\n", 1);
            }
            else
            {
                saxreader_string_append(locator, &buffer, &ch, 1);
                saxreader_skip(locator, 1);
            }
        }

        ch = *saxreader_get_ptr_noread(locator);
    }

    free(buffer.data);
    saxreader_set_error(locator, E_SAX_UNEXPECTED_EOF);
}

static bool saxreader_predefined_entity(struct saxlocator *locator, const struct text_position *position, BSTR name)
{
    const WCHAR *value;
    BSTR str;

    if (locator->status != S_OK)
        return false;

    if (!(value = saxreader_get_predefined_entity(name)))
        return false;

    str = SysAllocString(value);
    saxlocator_characters(locator, position, str);
    SysFreeString(str);

    return true;
}

static void saxreader_push_entity(struct saxlocator *locator, const struct text_position *position,
        struct entity_decl *entity)
{
    struct encoded_buffer *buffer = &locator->buffer.utf16;
    ULONG needed = SysStringLen(entity->value);
    void *dst, *src;
    size_t size;

    if (entity->visited)
        return saxreader_set_error(locator, E_SAX_INFINITEREFLOOP);

    /* Squeeze entity value at current input position. Empty entities are valid. */
    if (needed)
    {
        if (!saxreader_array_reserve(locator, (void **)&buffer->data, &buffer->allocated,
                buffer->written + needed * sizeof(WCHAR), sizeof(*buffer->data)))
        {
            return saxreader_set_error(locator, E_OUTOFMEMORY);
        }

        if (buffer->written > buffer->cur * sizeof(WCHAR))
        {
            dst = (WCHAR *)buffer->data + buffer->cur + needed;
            src = (WCHAR *)buffer->data + buffer->cur;
            size = buffer->written - buffer->cur * sizeof(WCHAR);
            memmove(dst, src, size);
        }

        dst = (WCHAR *)buffer->data + buffer->cur;
        src = entity->value;
        size = needed * sizeof(WCHAR);
        memcpy(dst, src, size);

        buffer->written += needed * sizeof(WCHAR);
    }

    entity->visited = true;
    entity->remaining = needed;
    entity->position = *position;

    /* Recursive referencing is explicitly detected, making sure that entities could only be linked once. */
    list_add_head(&locator->buffer.entities, &entity->input_entry);

    saxlocator_start_entity(locator, &entity->position, entity->name);
}

/* [66] CharRef ::= '&#' [0-9]+ ';' | '&#x' [0-9a-fA-F]+ ';'
   [67] Reference ::= EntityRef | CharRef
   [68] EntityRef ::= '&' Name ';' */
static void saxreader_parse_reference(struct saxlocator *locator)
{
    struct string_buffer buffer = { 0 };
    struct text_position position;
    struct entity_decl *entity;
    BSTR str;

    if (saxreader_peek(locator, L"&#", 2))
    {
        saxreader_parse_charref(locator, &buffer, &position);
        str = saxreader_string_to_bstr(locator, &buffer);
        saxlocator_characters(locator, &position, str);
    }
    else
    {
        /* Skip '&' */
        saxreader_skip(locator, 1);
        position = locator->buffer.position;

        saxreader_parse_name(locator, &str);
        if (!saxreader_cmp(locator, L";"))
            saxreader_set_error(locator, E_SAX_MISSINGSEMICOLON);

        if (!saxreader_predefined_entity(locator, &position, str))
        {
            if ((entity = saxreader_get_entity(locator, str)))
            {
                if (entity->unparsed)
                    saxreader_set_error(locator, E_SAX_UNPARSEDENTITYREF);
                else if (*entity->value)
                    saxreader_push_entity(locator, &position, entity);
                else
                    saxlocator_skipped_entity(locator, &position, entity->name);
            }
            else
            {
                saxlocator_skipped_entity(locator, &position, str);
            }
        }
    }
    SysFreeString(str);
}

struct chardata_context
{
    struct string_buffer buffer;
    struct text_position position;
    size_t count;
    WCHAR ch;
};

static void saxreader_parse_characters(struct saxlocator *locator, struct chardata_context *ctxt)
{
    BSTR chars;

    /* Normalize line breaks */
    if (ctxt->ch == '\r')
    {
        bool move_position = false;

        saxreader_skip(locator, 1);
        ctxt->ch = *saxreader_get_ptr_noread(locator);
        if (ctxt->ch == '\n')
            saxreader_skip(locator, 1);
        else if (ctxt->ch != '\r')
            move_position = true;

        saxreader_string_append(locator, &ctxt->buffer, L"\n", 1);
        chars = saxreader_string_to_bstr(locator, &ctxt->buffer);

        saxlocator_characters(locator, &ctxt->position, chars);
        SysFreeString(chars);

        saxreader_shrink(locator);

        if (move_position)
            ctxt->position = locator->buffer.position;
        else
            ctxt->position.column += (locator->buffer.consumed - ctxt->count);

        ctxt->count = locator->buffer.consumed;
    }
    else
    {
        saxreader_string_append(locator, &ctxt->buffer, &ctxt->ch, 1);
        saxreader_skip(locator, 1);
    }

    ctxt->ch = *saxreader_get_ptr(locator);
}

static void saxreader_parse_characters_newparser(struct saxlocator *locator, struct chardata_context *ctxt)
{
    BSTR chars;

    /* Normalize line breaks */
    if (ctxt->ch == '\r')
    {
        bool move_position = false;

        saxreader_skip(locator, 1);
        ctxt->ch = *saxreader_get_ptr_noread(locator);
        if (ctxt->ch == '\n')
            saxreader_skip(locator, 1);
        else if (ctxt->ch != '\r')
            move_position = true;

        /* Flush current block first, then new line separately */
        chars = saxreader_string_to_bstr(locator, &ctxt->buffer);
        saxlocator_characters(locator, &ctxt->position, chars);
        SysFreeString(chars);

        saxreader_shrink(locator);

        ctxt->position = locator->buffer.position;
        ctxt->position.column = 0;
        saxreader_string_append(locator, &ctxt->buffer, L"\n", 1);
        chars = saxreader_string_to_bstr(locator, &ctxt->buffer);
        saxlocator_characters(locator, &ctxt->position, chars);
        SysFreeString(chars);

        if (move_position)
            ctxt->position = locator->buffer.position;
        else
            ctxt->position.column += (locator->buffer.consumed - ctxt->count);

        ctxt->count = locator->buffer.consumed;
    }
    else
    {
        saxreader_string_append(locator, &ctxt->buffer, &ctxt->ch, 1);
        saxreader_skip(locator, 1);
    }

    ctxt->ch = *saxreader_get_ptr(locator);
}

/* [14] CharData ::= [^<&]* - ([^<&]* ']]>' [^<&]*) */
static void saxreader_parse_chardata(struct saxlocator *locator)
{
    struct chardata_context context = { 0 };
    BSTR chars;

    context.position = locator->buffer.position;
    context.count = locator->buffer.consumed;
    context.ch = *saxreader_get_ptr(locator);

    while (context.ch)
    {
        if (saxreader_cmp(locator, L"]]>"))
        {
            saxreader_set_error(locator, E_SAX_INVALID_CDATACLOSINGTAG);
            break;
        }

        if (context.ch == '&' || context.ch == '<')
        {
            if (context.buffer.count)
            {
                if (locator->saxreader->version >= MSXML4)
                {
                    context.position.line = locator->buffer.position.line;
                    context.position.column = locator->buffer.position.column;
                }

                chars = saxreader_string_to_bstr(locator, &context.buffer);
                saxlocator_characters(locator, &context.position, chars);
                SysFreeString(chars);
            }
            else
            {
                free(context.buffer.data);
            }
            return;
        }

        if (locator->saxreader->features & NormalizeLineBreaks)
        {
            if (locator->saxreader->version >= MSXML4)
                saxreader_parse_characters_newparser(locator, &context);
            else
                saxreader_parse_characters(locator, &context);
        }
        else
        {
            saxreader_string_append(locator, &context.buffer, &context.ch, 1);
            saxreader_skip(locator, 1);
            context.ch = *saxreader_get_ptr(locator);
        }
    }

    free(context.buffer.data);
    saxreader_set_error(locator, E_SAX_UNEXPECTED_EOF);
}

/* [18] CDSect ::= CDStart CData CDEnd
   [19] CDStart ::= '<![CDATA['
   [20] CData ::= (Char* - (Char* ']]>' Char*))
   [21] CDEnd ::= ']]>' */
static void saxreader_parse_cdata(struct saxlocator *locator)
{
    struct chardata_context context = { 0 };
    BSTR chars;

    /* Skip <![CDATA[ */
    saxreader_skip(locator, 9);

    context.position = locator->buffer.position;
    context.count = locator->buffer.consumed;
    context.ch = *saxreader_get_ptr(locator);

    saxlocator_start_cdata(locator, &context.position);

    while (context.ch && locator->status == S_OK)
    {
        if (saxreader_cmp(locator, L"]]>"))
        {
            /* Point to closing '>' */
            if (locator->saxreader->version >= MSXML4)
            {
                context.position.line = locator->buffer.position.line;
                context.position.column = locator->buffer.position.column - 1;
            }

            chars = saxreader_string_to_bstr(locator, &context.buffer);
            saxlocator_characters(locator, &context.position, chars);
            SysFreeString(chars);

            return saxlocator_end_cdata(locator, &context.position);
        }

        if (locator->saxreader->features & NormalizeLineBreaks)
        {
            if (locator->saxreader->version >= MSXML4)
                saxreader_parse_characters_newparser(locator, &context);
            else
                saxreader_parse_characters(locator, &context);
        }
        else
        {
            saxreader_string_append(locator, &context.buffer, &context.ch, 1);
            saxreader_skip(locator, 1);
            context.ch = *saxreader_get_ptr(locator);
        }
    }

    free(context.buffer.data);
    saxreader_set_error(locator, E_SAX_UNCLOSEDCDATA);
}

static struct element_content *saxreader_new_element_content(struct saxlocator *locator,
        BSTR *name, enum element_content_type type)
{
    struct element_content *ret;

    if (locator->status != S_OK)
    {
        saxreader_free_bstr(name);
        return NULL;
    }

    if (!(ret = calloc(1, sizeof(*ret))))
    {
        saxreader_free_bstr(name);
        saxreader_set_error(locator, E_OUTOFMEMORY);
        return NULL;
    }

    if (name)
    {
        ret->name = *name;
        *name = NULL;
    }
    ret->type = type;
    ret->occurrence = ELEMENT_CONTENT_ONCE;

    return ret;
}

static struct element_content * saxreader_free_element_content(struct element_content *c)
{
    int depth = 0;

    while (true)
    {
        struct element_content *parent;

        while (c->left || c->right)
        {
            c = c->left ? c->left : c->right;
            ++depth;
        }

        SysFreeString(c->name);
        parent = c->parent;
        if (depth == 0 || !parent)
        {
            free(c);
            break;
        }
        if (c == parent->left)
            parent->left = NULL;
        else
            parent->right = NULL;
        free(c);

        if (parent->right)
        {
            c = parent->right;
        }
        else
        {
            --depth;
            c = parent;
        }
    }

    return NULL;
}

static void saxreader_link_element_content_part(struct element_content *parent, struct element_content *child, bool left)
{
    if (left)
        parent->left = child;
    else
        parent->right = child;

    if (child)
        child->parent = parent;
}

/* [51] Mixed ::= '(' S? '#PCDATA' (S? '|' S? Name)* S? ')*' | '(' S? '#PCDATA' S? ')' */
static enum element_type saxreader_parse_mixed_contentspec(struct saxlocator *locator, struct element_content **model)
{
    struct element_content *ret = NULL, *cur = NULL, *n, *left;
    BSTR name = NULL;
    WCHAR ch;

    *model = NULL;

    saxreader_skipspaces(locator);
    if (saxreader_cmp(locator, L")"))
    {
        ret = saxreader_new_element_content(locator, NULL, ELEMENT_CONTENT_PCDATA);
        if (saxreader_cmp(locator, L"*"))
            ret->occurrence = ELEMENT_CONTENT_MULT;
    }
    else
    {
        ch = *saxreader_get_ptr_noread(locator);
        if (ch == '(' || ch == '|')
            ret = cur = saxreader_new_element_content(locator, NULL, ELEMENT_CONTENT_PCDATA);
        while (ch == '|' && locator->status == S_OK)
        {
            saxreader_skip(locator, 1);
            if (name)
            {
                n = saxreader_new_element_content(locator, NULL, ELEMENT_CONTENT_OR);
                left = saxreader_new_element_content(locator, &name, ELEMENT_CONTENT_ELEMENT);
                saxreader_link_element_content_part(n, left, true);

                saxreader_link_element_content_part(cur, n, false);
                cur = n;
            }
            else
            {
                ret = saxreader_new_element_content(locator, NULL, ELEMENT_CONTENT_OR);
                saxreader_link_element_content_part(ret, cur, true);
                cur = ret;
            }
            saxreader_skipspaces(locator);
            saxreader_parse_name(locator, &name);
            saxreader_skipspaces(locator);
            ch = *saxreader_get_ptr_noread(locator);
        }

        if (!saxreader_cmp(locator, L")"))
            saxreader_set_error(locator, E_SAX_BADCHARINMIXEDMODEL);
        else if (!saxreader_cmp(locator, L"*"))
            saxreader_set_error(locator, E_SAX_MISSING_STAR);
        if (name)
        {
            cur->right = saxreader_new_element_content(locator, &name, ELEMENT_CONTENT_ELEMENT);
            if (cur->right) cur->right->parent = cur;
        }
        if (ret)
            ret->occurrence = ELEMENT_CONTENT_MULT;
    }

    if (locator->status == S_OK)
    {
        *model = ret;
        return ELEMENT_TYPE_MIXED;
    }

    return ELEMENT_TYPE_UNDEFINED;
}

static struct element_content * saxreader_parse_children_contentspec(struct saxlocator *locator);

static struct element_content * saxreader_parse_child_contentspec(struct saxlocator *locator)
{
    struct element_content *ret;
    BSTR name;

    saxreader_skipspaces(locator);
    if (saxreader_cmp(locator, L"("))
    {
        ret = saxreader_parse_children_contentspec(locator);
        saxreader_skipspaces(locator);
    }
    else
    {
        saxreader_parse_name(locator, &name);
        ret = saxreader_new_element_content(locator, &name, ELEMENT_CONTENT_ELEMENT);
        if (!ret)
            return NULL;
        if (saxreader_cmp(locator, L"?"))
            ret->occurrence = ELEMENT_CONTENT_OPT;
        else if (saxreader_cmp(locator, L"*"))
            ret->occurrence = ELEMENT_CONTENT_MULT;
        else if (saxreader_cmp(locator, L"+"))
            ret->occurrence = ELEMENT_CONTENT_PLUS;
        else
            ret->occurrence = ELEMENT_CONTENT_ONCE;
    }
    saxreader_skipspaces(locator);

    return ret;
}

static struct element_content * saxreader_free_element_content_pair(struct element_content *c1, struct element_content *c2)
{
    if (c1 && c1 != c2)
        saxreader_free_element_content(c1);
    if (c2)
        saxreader_free_element_content(c2);
    return NULL;
}

static struct element_content * saxreader_parse_children_contentspec(struct saxlocator *locator)
{
    struct element_content *ret = NULL, *cur = NULL, *last = NULL, *op = NULL;
    WCHAR ch, sep = 0;

    /* TODO: limit recursion depth */

    if (locator->status != S_OK)
        return NULL;

    cur = ret = saxreader_parse_child_contentspec(locator);
    if (!cur)
        return NULL;

    ch = *saxreader_get_ptr(locator);
    while (ch != ')' && locator->status == S_OK)
    {
        if (ch == ',')
        {
            /* Check for "Name | Name , Name" */
            if (sep == 0) sep = ch;
            else if (sep != ch)
            {
                ret = saxreader_free_element_content_pair(last, ret);
                saxreader_set_error(locator, E_SAX_INVALID_MODEL);
                break;
            }

            saxreader_skip(locator, 1);
            op = saxreader_new_element_content(locator, NULL, ELEMENT_CONTENT_SEQ);
            if (!op)
            {
                ret = saxreader_free_element_content_pair(last, ret);
                break;
            }
            if (!last)
            {
                saxreader_link_element_content_part(op, ret, true);
                ret = cur = op;
            }
            else
            {
                saxreader_link_element_content_part(cur, op, false);
                saxreader_link_element_content_part(op, last, true);
                cur = op;
                last = NULL;
            }
        }
        else if (ch == '|')
        {
            /* Check for "Name , Name | Name" */
            if (sep == 0) sep = ch;
            else if (sep != ch)
            {
                ret = saxreader_free_element_content_pair(last, ret);
                saxreader_set_error(locator, E_SAX_INVALID_MODEL);
                break;
            }
            saxreader_skip(locator, 1);

            op = saxreader_new_element_content(locator, NULL, ELEMENT_CONTENT_OR);
            if (!op)
            {
                ret = saxreader_free_element_content_pair(last, ret);
                break;
            }
            if (!last)
            {
                saxreader_link_element_content_part(op, ret, true);
                ret = cur = op;
            }
            else
            {
                saxreader_link_element_content_part(cur, op, false);
                saxreader_link_element_content_part(op, last, true);
                cur = op;
                last = NULL;
            }
        }
        else
        {
            ret = saxreader_free_element_content_pair(last, ret);
            saxreader_set_error(locator, E_SAX_INVALID_MODEL);
            break;
        }

        if (!(last = saxreader_parse_child_contentspec(locator)))
        {
            ret = saxreader_free_element_content(ret);
            break;
        }

        ch = *saxreader_get_ptr(locator);
    }

    if (!ret)
        return NULL;

    /* Closing ')' */
    saxreader_skip(locator, 1);

    if (cur && last)
        saxreader_link_element_content_part(cur, last, false);

    if (saxreader_cmp(locator, L"?"))
    {
        if (ret->occurrence == ELEMENT_CONTENT_PLUS
                || ret->occurrence == ELEMENT_CONTENT_MULT)
        {
            ret->occurrence = ELEMENT_CONTENT_MULT;
        }
        else
        {
            ret->occurrence = ELEMENT_CONTENT_OPT;
        }
    }
    else if (saxreader_cmp(locator, L"*"))
    {
        ret->occurrence = ELEMENT_CONTENT_MULT;
    }
    else if (saxreader_cmp(locator, L"+"))
    {
        if (ret->occurrence == ELEMENT_CONTENT_PLUS
                || ret->occurrence == ELEMENT_CONTENT_MULT)
        {
            ret->occurrence = ELEMENT_CONTENT_MULT;
        }
        else
        {
            ret->occurrence = ELEMENT_CONTENT_PLUS;
        }
    }

    return ret;
}

/* [46] contentspec ::= 'EMPTY' | 'ANY' | Mixed | children
   [51] Mixed ::= '(' S? '#PCDATA' (S? '|' S? Name)* S? ')*' | '(' S? '#PCDATA' S? ')' */
static enum element_type saxreader_parse_contentspec(struct saxlocator *locator, struct element_content **content)
{
    *content = NULL;

    if (saxreader_cmp(locator, L"EMPTY"))
    {
        return ELEMENT_TYPE_EMPTY;
    }
    else if (saxreader_cmp(locator, L"ANY"))
    {
        return ELEMENT_TYPE_ANY;
    }
    else if (saxreader_cmp(locator, L"("))
    {
        saxreader_skipspaces(locator);

        if (saxreader_cmp(locator, L"#PCDATA"))
            return saxreader_parse_mixed_contentspec(locator, content);
        else
        {
            *content = saxreader_parse_children_contentspec(locator);
            return ELEMENT_TYPE_ELEMENT;
        }
    }
    else
    {
        saxreader_set_error(locator, E_SAX_INVALID_MODEL);
        return ELEMENT_TYPE_UNDEFINED;
    }
}

/* [45] elementdecl ::= '<!ELEMENT' S Name S contentspec S? '>' */
static void saxreader_parse_elementdecl(struct saxlocator *locator)
{
    struct element_decl decl;

    /* Skip <!ELEMENT */
    saxreader_skip(locator, 9);
    saxreader_skip_required_spaces(locator);
    saxreader_parse_name(locator, &decl.name);
    saxreader_skip_required_spaces(locator);
    decl.type = saxreader_parse_contentspec(locator, &decl.content);
    saxreader_skipspaces(locator);
    if (saxreader_cmp(locator, L">"))
        saxlocator_elementdecl(locator, &decl);
    else
        saxreader_set_error(locator, E_SAX_EXPECTINGTAGEND);

    SysFreeString(decl.name);
}

/* [13] PubidChar ::= #x20 | #xD | #xA | [a-zA-Z0-9] | [-'()+,./:=?;!*#@$_%] */
static bool saxreader_is_pubidchar(WCHAR ch)
{
    return (ch == 0x20) || (ch == 0xd) || (ch == 0xa) ||
           (ch >= 'a' && ch <= 'z') ||
           (ch >= 'A' && ch <= 'Z') ||
           (ch >= '0' && ch <= '9') ||
           (ch >= '-' && ch <= ';') || /* '()*+,-./:; */
           (ch == '=') || (ch == '?') ||
           (ch == '@') || (ch == '!') ||
           (ch == '#') || (ch == '$') || (ch == '%') ||
           (ch == '_');
}

/* [12] PubidLiteral ::= '"' PubidChar* '"' | "'" (PubidChar - "'")* "'"
   [13] PubidChar ::= #x20 | #xD | #xA | [a-zA-Z0-9] | [-'()+,./:=?;!*#@$_%] */
static BSTR saxreader_parse_pubidliteral(struct saxlocator *locator)
{
    struct string_buffer buffer = { 0 };
    WCHAR quote, ch;
    BSTR str;

    if (!(quote = saxreader_is_quote(locator)))
    {
        saxreader_set_error(locator, E_SAX_MISSINGQUOTE);
        return NULL;
    }

    saxreader_skip(locator, 1);
    saxreader_more(locator);

    ch = *saxreader_get_ptr_noread(locator);
    while (saxreader_is_pubidchar(ch) && ch != quote)
    {
        saxreader_string_append(locator, &buffer, &ch, 1);
        saxreader_skip(locator, 1);
        ch = *saxreader_get_ptr_noread(locator);
    }

    if (ch == quote)
    {
        str = saxreader_string_to_bstr(locator, &buffer);
        /* Skip closing quote */
        saxreader_skip(locator, 1);
        return str;
    }

    free(buffer.data);
    saxreader_set_error(locator, E_SAX_EXPECTINGCLOSEQUOTE);
    return NULL;
}

/* [11] SystemLiteral ::= ('"' [^"]* '"') | ("'" [^']* "'") */
static BSTR saxreader_parse_systemliteral(struct saxlocator *locator)
{
    struct string_buffer buffer = { 0 };
    WCHAR quote, ch;

    if (!(quote = saxreader_is_quote(locator)))
    {
        saxreader_set_error(locator, E_SAX_MISSINGQUOTE);
        return NULL;
    }

    saxreader_skip(locator, 1);
    saxreader_more(locator);

    ch = *saxreader_get_ptr_noread(locator);
    while (ch)
    {
        if (ch == quote)
        {
            saxreader_skip(locator, 1);
            return saxreader_string_to_bstr(locator, &buffer);
        }
        saxreader_string_append(locator, &buffer, &ch, 1);
        saxreader_skip(locator, 1);
        ch = *saxreader_get_ptr_noread(locator);
    }

    free(buffer.data);
    saxreader_set_error(locator, E_SAX_EXPECTINGCLOSEQUOTE);
    return NULL;
}

/* [75] ExternalID ::= 'SYSTEM' S SystemLiteral
                     | 'PUBLIC' S PubidLiteral S SystemLiteral
   [83] PublicID ::= 'PUBLIC' S PubidLiteral */
static void saxreader_parse_externalid(struct saxlocator *locator, BSTR *systemid, BSTR *publicid, bool is_notation)
{
    *systemid = *publicid = NULL;

    if (saxreader_cmp(locator, L"SYSTEM"))
    {
        if (!saxreader_skip_required_spaces(locator))
            return;

        *systemid = saxreader_parse_systemliteral(locator);
    }
    else if (saxreader_cmp(locator, L"PUBLIC"))
    {
        if (!saxreader_skip_required_spaces(locator))
            return;

        *publicid = saxreader_parse_pubidliteral(locator);

        if (is_notation)
        {
            if (!saxreader_skipspaces(locator))
                return;
            if (!saxreader_is_quote(locator))
                return;
        }
        else
        {
            if (!saxreader_skip_required_spaces(locator))
            {
                SysFreeString(*publicid);
                *publicid = NULL;
                return;
            }
        }

        *systemid = saxreader_parse_systemliteral(locator);
    }
}

/* Read EntityRef without much validation */
static void saxreader_parse_entityvalue_entityref(struct saxlocator *locator, struct string_buffer *buffer)
{
    WCHAR ch;

    saxreader_string_append(locator, buffer, L"&", 1);
    saxreader_skip(locator, 1);

    /* Check for empty name */
    ch = *saxreader_get_ptr_noread(locator);
    if (!saxreader_is_namechar(ch))
        saxreader_set_error(locator, E_SAX_BADCHARINENTREF);

    while (saxreader_is_namechar(ch))
    {
        saxreader_string_append(locator, buffer, &ch, 1);
        saxreader_skip(locator, 1);
        ch = *saxreader_get_ptr_noread(locator);
    }

    if (saxreader_cmp(locator, L";"))
        saxreader_string_append(locator, buffer, L";", 1);
    else
        saxreader_set_error(locator, E_SAX_BADCHARINENTREF);
}

static void saxreader_entity_add_part(struct saxlocator *locator, struct entity_decl *decl,
        struct string_buffer *buffer, bool reference)
{
    struct entity_part *part;

    if (!buffer->count)
        return;

    if (!saxreader_array_reserve(locator, (void **)&decl->content.parts, &decl->content.capacity,
            decl->content.count + 1, sizeof(*decl->content.parts)))
    {
        return;
    }
    part = &decl->content.parts[decl->content.count++];

    part->value = saxreader_string_to_bstr(locator, buffer);
    part->reference = reference;
}

/* [9] EntityValue ::= '"' ([^%&"] | PEReference | Reference)* '"'
        |  "'" ([^%&'] | PEReference | Reference)* "'" */
static void saxreader_parse_entityvalue(struct saxlocator *locator, struct entity_decl *decl)
{
    struct string_buffer buffer = { 0 }, part = { 0 };
    struct text_position position;
    WCHAR quote, ch;
    size_t count;

    if (!(quote = saxreader_is_quote(locator)))
    {
        saxreader_set_error(locator, E_SAX_MISSINGQUOTE);
        return;
    }

    saxreader_skip(locator, 1);

    /* Only character references are resolved. Entity references are parsed
       lazily, checking for valid name characters but not for a valid name.
       Referenced entities do not have to be already declared, and are not expanded
       if they are declared. */

    saxreader_more(locator);
    ch = *saxreader_get_ptr_noread(locator);
    while (ch != quote)
    {
        if (saxreader_peek(locator, L"&#", 2))
        {
            count = buffer.count;
            saxreader_parse_charref(locator, &buffer, &position);
            saxreader_string_append(locator, &part, buffer.data + count, buffer.count - count);
        }
        else if (saxreader_peek(locator, L"&", 1))
        {
            saxreader_entity_add_part(locator, decl, &part, false);

            count = buffer.count;
            saxreader_parse_entityvalue_entityref(locator, &buffer);
            /* Skip markup in &name; */
            if (buffer.count - count > 2)
                saxreader_string_append(locator, &part, buffer.data + count + 1, buffer.count - count - 2);

            saxreader_entity_add_part(locator, decl, &part, true);
        }
        else
        {
            saxreader_string_append(locator, &buffer, &ch, 1);
            saxreader_string_append(locator, &part, &ch, 1);
            saxreader_skip(locator, 1);
        }
        ch = *saxreader_get_ptr_noread(locator);
    }

    if (ch == quote)
    {
        saxreader_entity_add_part(locator, decl, &part, false);
        decl->value = saxreader_string_to_bstr(locator, &buffer);

        /* Skip closing quote */
        saxreader_skip(locator, 1);
        return;
    }

    free(part.data);
    free(buffer.data);
    saxreader_set_error(locator, E_SAX_EXPECTINGCLOSEQUOTE);
}

static void saxreader_release_entity(struct entity_decl *entity)
{
    SysFreeString(entity->name);
    SysFreeString(entity->sysid);
    SysFreeString(entity->value);
    free(entity);
}

/* [70] EntityDecl ::= GEDecl | PEDecl
   [71] GEDecl ::= '<!ENTITY' S Name S EntityDef S? '>'
   [72] PEDecl ::= '<!ENTITY' S '%' S Name S PEDef S? '>'
   [73] EntityDef ::= EntityValue | (ExternalID NDataDecl?)
   [74] PEDef ::= EntityValue | ExternalID
   [76] NDataDecl ::= S 'NDATA' S Name */
static void saxreader_parse_entitydecl(struct saxlocator *locator)
{
    BSTR notation = NULL, pubid = NULL;
    struct string_buffer buffer = { 0 };
    struct entity_decl *decl;
    bool pe, undeclared;

    /* Skip <!ENTITY */
    saxreader_skip(locator, 8);
    saxreader_skip_required_spaces(locator);

    if ((pe = saxreader_cmp(locator, L"%")))
        saxreader_skip_required_spaces(locator);

    if (!(decl = saxreader_calloc(locator, 1, sizeof(*decl))))
        return;

    saxreader_parse_name_strict(locator, &decl->name);
    saxreader_skip_required_spaces(locator);

    if (pe)
    {
        saxreader_string_append(locator, &buffer, L"%", 1);
        saxreader_string_append_bstr(locator, &buffer, &decl->name);
        decl->name = saxreader_string_to_bstr(locator, &buffer);
        undeclared = saxreader_get_entity(locator, decl->name) == NULL;

        if (saxreader_is_quote(locator))
        {
            saxreader_parse_entityvalue(locator, decl);
            saxreader_skipspaces(locator);
            if (saxreader_cmp(locator, L">"))
            {
                if (undeclared)
                    saxlocator_internal_entitydecl(locator, decl->name, decl->value);
            }
            else
            {
                saxreader_set_error(locator, E_SAX_EXPECTINGTAGEND);
            }
        }
        else
        {
            saxreader_parse_externalid(locator, &decl->sysid, &pubid, false);
            saxreader_skipspaces(locator);
            if (saxreader_cmp(locator, L">"))
            {
                if (undeclared)
                    saxlocator_external_entitydecl(locator, decl->name, pubid, decl->sysid);
            }
            else
            {
                saxreader_set_error(locator, E_SAX_EXPECTINGTAGEND);
            }
        }
    }
    else
    {
        undeclared = saxreader_get_entity(locator, decl->name) == NULL;

        if (saxreader_is_quote(locator))
        {
            saxreader_parse_entityvalue(locator, decl);
            saxreader_skipspaces(locator);
            if (saxreader_cmp(locator, L">"))
            {
                if (undeclared)
                    saxlocator_internal_entitydecl(locator, decl->name, decl->value);
            }
            else
            {
                saxreader_set_error(locator, E_SAX_EXPECTINGTAGEND);
            }
        }
        else
        {
            saxreader_parse_externalid(locator, &decl->sysid, &pubid, false);

            saxreader_more(locator);
            if (!saxreader_peek(locator, L">", 1))
                saxreader_skip_required_spaces(locator);

            if (saxreader_cmp(locator, L"NDATA"))
            {
                decl->unparsed = true;
                saxreader_skip_required_spaces(locator);
                saxreader_parse_name(locator, &notation);
                saxreader_skipspaces(locator);
                if (saxreader_cmp(locator, L">"))
                {
                    if (undeclared)
                        saxlocator_unparsed_entitydecl(locator, decl->name, pubid, decl->sysid, notation);
                }
                else
                {
                    saxreader_set_error(locator, E_SAX_EXPECTINGTAGEND);
                }
            }
            else
            {
                saxreader_skipspaces(locator);
                if (saxreader_cmp(locator, L">"))
                {
                    if (undeclared)
                        saxlocator_external_entitydecl(locator, decl->name, pubid, decl->sysid);
                }
                else
                {
                    saxreader_set_error(locator, E_SAX_EXPECTINGTAGEND);
                }
            }
        }
    }

    if (undeclared)
        list_add_tail(&locator->dtd.entities, &decl->entry);
    else
        saxreader_release_entity(decl);

    SysFreeString(pubid);
    SysFreeString(notation);
}

static void saxreader_discard_string(BSTR *str)
{
    SysFreeString(*str);
    *str = NULL;
}

static void saxreader_append_enum_item(struct saxlocator *locator, struct enumeration *list,
        BSTR *value)
{
    if (locator->status != S_OK)
        return saxreader_discard_string(value);

    if (!saxreader_array_reserve(locator, (void **)&list->values, &list->capacity, list->count + 1,
            sizeof(*list->values)))
    {
        return;
    }

    list->values[list->count++] = *value;
    *value = NULL;
}

static void saxreader_clear_enumeration(struct enumeration *list)
{
    for (size_t i = 0; i < list->count; ++i)
        SysFreeString(list->values[i]);
    free(list->values);
}

/* [58] NotationType ::= 'NOTATION' S '(' S? Name (S? '|' S? Name)* S? ')' */
static enum attribute_type saxreader_parse_notation_type(struct saxlocator *locator,
        struct enumeration *list)
{
    bool done = false;
    BSTR name;

    /* Skip NOTATION */
    saxreader_skip(locator, 8);
    saxreader_skip_required_spaces(locator);
    if (!saxreader_cmp(locator, L"("))
    {
        saxreader_set_error(locator, E_SAX_MISSING_PAREN);
        return ATTR_TYPE_INVALID;
    }

    while (locator->status == S_OK && !done)
    {
        saxreader_skipspaces(locator);
        saxreader_parse_name_strict(locator, &name);
        saxreader_append_enum_item(locator, list, &name);
        saxreader_skipspaces(locator);

        if (saxreader_cmp(locator, L")"))
            done = true;
        else if (!saxreader_cmp(locator, L"|"))
            saxreader_set_error(locator, E_SAX_BADCHARINENUMERATION);
    }

    if (locator->status == S_OK)
        return ATTR_TYPE_NOTATION;

    saxreader_clear_enumeration(list);
    return ATTR_TYPE_INVALID;
}

/* [59] Enumeration ::= '(' S? Nmtoken (S? '|' S? Nmtoken)* S? ')' */
static enum attribute_type saxreader_parse_enumeration_type(struct saxlocator *locator,
        struct enumeration *list)
{
    bool done = false;
    BSTR token;

    /* Skip '(' */
    saxreader_skip(locator, 1);

    while (locator->status == S_OK && !done)
    {
        saxreader_skipspaces(locator);
        saxreader_parse_nmtoken(locator, &token);
        saxreader_append_enum_item(locator, list, &token);
        saxreader_skipspaces(locator);

        if (saxreader_cmp(locator, L")"))
            done = true;
        else if (!saxreader_cmp(locator, L"|"))
            saxreader_set_error(locator, E_SAX_BADCHARINENUMERATION);
    }

    if (locator->status == S_OK)
        return ATTR_TYPE_ENUMERATION;

    saxreader_clear_enumeration(list);
    return ATTR_TYPE_INVALID;
}

/* [54] AttType ::= StringType | TokenizedType | EnumeratedType
   [55] StringType ::= 'CDATA'
   [56] TokenizedType ::= 'ID' | 'IDREF' | 'IDREFS' | 'ENTITY'
           | 'ENTITIES' | 'NMTOKEN' | 'NMTOKENS'
   [57] EnumeratedType ::= NotationType | Enumeration */
static enum attribute_type saxreader_parse_atttype(struct saxlocator *locator,
        struct enumeration *list)
{
    if (saxreader_cmp(locator, L"CDATA"))
        return ATTR_TYPE_CDATA;
    if (saxreader_cmp(locator, L"IDREFS"))
        return ATTR_TYPE_IDREFS;
    if (saxreader_cmp(locator, L"IDREF"))
        return ATTR_TYPE_IDREF;
    if (saxreader_cmp(locator, L"ID"))
        return ATTR_TYPE_ID;
    if (saxreader_cmp(locator, L"ENTITY"))
        return ATTR_TYPE_ENTITY;
    if (saxreader_cmp(locator, L"ENTITIES"))
        return ATTR_TYPE_ENTITIES;
    if (saxreader_cmp(locator, L"NMTOKENS"))
        return ATTR_TYPE_NMTOKENS;
    if (saxreader_cmp(locator, L"NMTOKEN"))
        return ATTR_TYPE_NMTOKEN;

    if (saxreader_peek(locator, L"NOTATION", 8))
        return saxreader_parse_notation_type(locator, list);
    else if (saxreader_peek(locator, L"(", 1))
        return saxreader_parse_enumeration_type(locator, list);

    saxreader_set_error(locator, E_SAX_INVALID_TYPE);
    return ATTR_TYPE_INVALID;
}

/* [60] DefaultDecl ::= '#REQUIRED' | '#IMPLIED' | (('#FIXED' S)? AttValue) */
static enum attribute_default_type saxreader_parse_defaultdecl(struct saxlocator *locator, BSTR *value)
{
    enum attribute_default_type ret = ATTR_DEF_TYPE_NONE;

    *value = NULL;

    if (saxreader_cmp(locator, L"#REQUIRED"))
        return ATTR_DEF_TYPE_REQUIRED;
    if (saxreader_cmp(locator, L"#IMPLIED"))
        return ATTR_DEF_TYPE_IMPLIED;

    if (saxreader_cmp(locator, L"#FIXED"))
    {
        ret = ATTR_DEF_TYPE_FIXED;
        saxreader_skip_required_spaces(locator);
    }
    *value = saxreader_parse_attvalue(locator);

    return ret;
}

/* [52] AttlistDecl ::= '<!ATTLIST' S Name AttDef* S? '>'
   [53] AttDef ::= S Name S AttType S DefaultDecl */
static void saxreader_parse_attlistdecl(struct saxlocator *locator)
{
    struct attlist_decl *decl;

    if (!(decl = saxreader_calloc(locator, 1, sizeof(*decl))))
        return;
    list_add_tail(&locator->dtd.attr_decls, &decl->entry);

    /* Skip <!ATTLIST */
    saxreader_skip(locator, 9);
    saxreader_skip_required_spaces(locator);
    saxreader_parse_name(locator, &decl->name);
    saxreader_skipspaces(locator);

    while (locator->status == S_OK)
    {
        struct attlist_attr *attr;

        if (saxreader_cmp(locator, L">"))
            break;

        if (!saxreader_array_reserve(locator, (void **)&decl->attributes, &decl->capacity,
                decl->count + 1, sizeof(*decl->attributes)))
        {
            break;
        }
        attr = &decl->attributes[decl->count++];
        memset(attr, 0, sizeof(*attr));

        saxreader_parse_qname(locator, &attr->name);
        saxreader_skip_required_spaces(locator);
        attr->type = saxreader_parse_atttype(locator, &attr->valuelist);
        saxreader_skip_required_spaces(locator);
        attr->valuetype = saxreader_parse_defaultdecl(locator, &attr->value);

        if (saxreader_cmp(locator, L">"))
            break;

        saxreader_skip_required_spaces(locator);
    }

    saxlocator_attribute_decl(locator, decl);
}

/* [82] NotationDecl ::= '<!NOTATION' S Name S (ExternalID | PublicID) S? '>' */
static void saxreader_parse_notationdecl(struct saxlocator *locator)
{
    BSTR name = NULL, sysid = NULL, pubid = NULL;

    /* Skip <!NOTATION */
    saxreader_skip(locator, 10);
    saxreader_skip_required_spaces(locator);
    saxreader_parse_name_strict(locator, &name);
    saxreader_shrink(locator);

    saxreader_skip_required_spaces(locator);
    saxreader_parse_externalid(locator, &sysid, &pubid, true);
    saxreader_skipspaces(locator);

    if (!saxreader_cmp(locator, L">"))
        saxreader_set_error(locator, E_SAX_EXPECTINGTAGEND);

    saxlocator_notationdecl(locator, &locator->buffer.position, name, pubid, sysid);

    SysFreeString(name);
    SysFreeString(sysid);
    SysFreeString(pubid);
}

/* [29] markupdecl ::= elementdecl | AttlistDecl | EntityDecl | NotationDecl | PI | Comment */
static void saxreader_parse_markupdecl(struct saxlocator *locator)
{
    saxreader_more(locator);
    if (saxreader_peek(locator, L"<!--", 4))
        saxreader_parse_comment(locator);
    else if (saxreader_peek(locator, L"<!", 2))
    {
        if (saxreader_peek(locator, L"<!ELEMENT", 9))
            saxreader_parse_elementdecl(locator);
        else if (saxreader_peek(locator, L"<!ENTITY", 8))
            saxreader_parse_entitydecl(locator);
        else if (saxreader_peek(locator, L"<!ATTLIST", 9))
            saxreader_parse_attlistdecl(locator);
        else if (saxreader_peek(locator, L"<!NOTATION", 10))
            saxreader_parse_notationdecl(locator);
        else
            saxreader_set_error(locator, E_SAX_BADDECLNAME);
    }
    else if (saxreader_peek(locator, L"<?", 2))
        saxreader_parse_pi(locator);
    else
        saxreader_set_error(locator, E_SAX_BADCHARINDTD);
}

/* [69] PEReference ::= '%' Name ';' */
static void saxreader_parse_pe_reference(struct saxlocator *locator)
{
    struct string_buffer buffer = { 0 };
    struct text_position position;
    struct entity_decl *entity;
    BSTR name;

    /* Skip '%' */
    saxreader_skip(locator, 1);
    position = locator->buffer.position;

    saxreader_parse_name(locator, &name);
    if (!saxreader_cmp(locator, L";"))
        saxreader_set_error(locator, E_SAX_MISSINGSEMICOLON);

    saxreader_string_append(locator, &buffer, L"%", 1);
    saxreader_string_append_bstr(locator, &buffer, &name);
    name = saxreader_string_to_bstr(locator, &buffer);

    if ((entity = saxreader_get_entity(locator, name)))
        saxreader_push_entity(locator, &locator->buffer.position, entity);
    else
        saxlocator_skipped_entity(locator, &position, name);
}

/* [28a] DeclSep ::= PEReference | S
   [28b] intSubset ::= (markupdecl | DeclSep)*
   [29]  markupdecl ::= elementdecl | AttlistDecl | EntityDecl | NotationDecl | PI | Comment */
static void saxreader_parse_intsubset(struct saxlocator *locator)
{
    WCHAR *ptr;

    /* Opening '[' bracket already skipped */
    saxreader_skipspaces(locator);

    while (locator->status == S_OK)
    {
        if (saxreader_cmp(locator, L"]"))
            return;

        // TODO: conditional blocks should be supported too

        if (saxreader_peek(locator, L"<!", 2) || saxreader_peek(locator, L"<?", 2))
            saxreader_parse_markupdecl(locator);
        else if (saxreader_peek(locator, L"%", 1))
            saxreader_parse_pe_reference(locator);
        else
            saxreader_set_error(locator, E_SAX_BADCHARINDTD);

        saxreader_skipspaces(locator);
        saxreader_shrink(locator);
        ptr = saxreader_get_ptr(locator);
        if (!*ptr)
            saxreader_set_error(locator, E_SAX_UNEXPECTED_EOF);
    }
}

/* [28] doctypedecl ::= '<!DOCTYPE' S Name (S ExternalID)? S? ('[' intSubset ']' S?)? '>' */
static void saxreader_parse_doctype(struct saxlocator *locator)
{
    BSTR name, sysid, pubid;

    if (locator->saxreader->features & ProhibitDTD)
        return saxreader_set_error(locator, E_SAX_INVALIDATROOTLEVEL);

    if (saxreader_has_handler(locator, SAXExtensionHandler))
        locator->collect = true;

    /* Skip <!DOCTYPE */
    saxreader_skip(locator, 9);
    saxreader_skip_required_spaces(locator);
    saxreader_parse_name(locator, &name);

    saxreader_skipspaces(locator);
    saxreader_parse_externalid(locator, &sysid, &pubid, false);

    saxreader_skipspaces(locator);
    saxreader_shrink(locator);

    if (saxreader_cmp(locator, L"["))
    {
        saxlocator_startdtd(locator, name, pubid, sysid);
        saxreader_parse_intsubset(locator);
        saxreader_skipspaces(locator);
        if (!saxreader_cmp(locator, L">"))
            saxreader_set_error(locator, E_SAX_EXPECTINGTAGEND);
    }
    else if (saxreader_cmp(locator, L">"))
    {
        saxlocator_startdtd(locator, name, pubid, sysid);
    }

    if (sysid)
        FIXME("External subset is not supported.\n");

    saxlocator_extension_dtd(locator);
    saxlocator_enddtd(locator);

    SysFreeString(sysid);
    SysFreeString(pubid);
}

static enum xmlencoding saxreader_match_encoding(const char *data, size_t size, size_t *bom)
{
    const BYTE *b = (const BYTE *)data;

    *bom = 0;

    if (size < 4)
        return XML_ENCODING_UNKNOWN;

    if (b[0] == 0 && b[1] == 0 && b[2] == 0 && b[3] == '<')
        return XML_ENCODING_UCS4BE;
    if (b[0] == '<' && b[1] == 0 && b[2] == 0 && b[3] == 0)
        return XML_ENCODING_UCS4LE;
    if (b[0] == '<' && b[1] == 0 && b[2] == '?' && b[3] == 0)
        return XML_ENCODING_UTF16LE;
    if (b[0] == '<' && b[1] == 0 && b[2] && b[2] != '?')
        return XML_ENCODING_UTF16LE;
    if (b[0] == 0 && b[1] == '<' && b[2] == 0 && b[3] == '?')
        return XML_ENCODING_UTF16BE;
    if (b[0] == '<' && b[1] == '?' && b[2] == 'x' && b[3] == 'm')
        return XML_ENCODING_UTF8;
    if (b[0] == '<' && b[1] && b[1] != '?')
        return XML_ENCODING_UTF8;

    if (b[0] == 0xef && b[1] == 0xbb && b[2] == 0xbf)
    {
        *bom = 3;
        return XML_ENCODING_UTF8;
    }

    if (b[0] == 0xfe && b[1] == 0xff)
    {
        *bom = 2;
        return XML_ENCODING_UTF16BE;
    }

    if (b[0] == 0xff && b[1] == 0xfe)
    {
        *bom = 2;
        return XML_ENCODING_UTF16LE;
    }

    return XML_ENCODING_UNKNOWN;
}

static void saxreader_detect_encoding(struct saxlocator *locator, bool force_utf16)
{
    struct encoded_buffer *utf16 = &locator->buffer.utf16;
    struct encoded_buffer *raw = &locator->buffer.raw;
    enum xmlencoding encoding;
    size_t bom = 0, size;
    ULONG read = 0;
    HRESULT hr;

    /* In principle there is no need to read as much to detect encoding,
       but experiments show that application never gets smaller input buffer. */

    if (!saxreader_reserve_buffer(locator, raw, locator->buffer.chunk_size))
        return;

    if (FAILED(hr = ISequentialStream_Read(locator->stream, raw->data, locator->buffer.chunk_size, &read)))
        return saxreader_set_error(locator, hr);

    if (!saxreader_limit_xml_size(locator, read))
        return;

    if (!read)
        return saxreader_set_error(locator, E_SAX_MISSINGROOT);

    raw->written = read;

    encoding = force_utf16 ? XML_ENCODING_UTF16LE : saxreader_match_encoding(raw->data, read, &bom);

    if (encoding == XML_ENCODING_UNKNOWN)
    {
        WARN("Failed to detect document encoding.\n");
        return saxreader_set_error(locator, E_SAX_INVALIDENCODING);
    }

    locator->buffer.encoding = encoding;
    locator->buffer.converter = convert_get_converter(encoding);
    locator->buffer.code_page = convert_get_codepage(encoding);

    TRACE("detected encoding %s\n", saxreader_get_encoding_name(encoding));

    size = read - bom;

    if (encoding == XML_ENCODING_UTF16LE)
    {
        if (!saxreader_reserve_buffer(locator, utf16, size + sizeof(WCHAR)))
            return;

        memcpy(utf16->data + utf16->written, raw->data + bom, size);
        utf16->written += size;
        *(WCHAR *)&utf16->data[utf16->written] = 0;

        encoded_buffer_cleanup(raw);
    }
    else if (bom)
    {
        memmove(raw->data, raw->data + bom, size);
        raw->written -= bom;
    }
}

static HRESULT saxreader_parse_stream(struct saxreader *reader, ISequentialStream *stream,
        bool force_utf16, bool vbInterface)
{
    struct saxlocator *locator;
    HRESULT hr;

    if (FAILED(hr = saxlocator_create(reader, stream, vbInterface, &locator)))
        return hr;

    while (locator->state != SAX_PARSER_EOF)
    {
        if (locator->status != S_OK)
        {
            saxreader_fatal_error(locator);
            break;
        }

        switch (locator->state)
        {
            case SAX_PARSER_START:
                saxreader_detect_encoding(locator, force_utf16);
                locator->state = SAX_PARSER_XML_DECL;
                break;
            case SAX_PARSER_XML_DECL:
                saxreader_parse_xmldecl(locator);
                locator->buffer.switched_encoding = true;
                saxlocator_put_document_locator(locator);
                saxlocator_xmldecl(locator);
                saxlocator_start_document(locator);
                locator->state = SAX_PARSER_MISC;
                break;
            case SAX_PARSER_MISC:
            case SAX_PARSER_EPILOG:
                saxreader_skipspaces(locator);
                saxreader_more(locator);

                if (saxreader_peek(locator, L"<", 1))
                {
                    if (saxreader_peek(locator, L"<?", 2))
                    {
                        saxreader_parse_pi(locator);
                        break;
                    }
                    else if (saxreader_peek(locator, L"<!--", 4))
                    {
                        saxreader_parse_comment(locator);
                        break;
                    }
                    else if (locator->state == SAX_PARSER_MISC)
                    {
                        if (saxreader_peek(locator, L"<!DOCTYPE", 9))
                        {
                            saxreader_parse_doctype(locator);
                            break;
                        }
                    }
                }

                if (locator->state == SAX_PARSER_EPILOG)
                {
                    locator->state = SAX_PARSER_EOF;
                    saxlocator_end_document(locator);
                }
                else
                {
                    locator->state = SAX_PARSER_START_TAG;
                }
                break;
            case SAX_PARSER_START_TAG:
                saxreader_parse_starttag(locator);
                if (list_empty(&locator->elements))
                    locator->state = SAX_PARSER_EPILOG;
                else
                    locator->state = SAX_PARSER_CONTENT;
                break;
            case SAX_PARSER_END_TAG:
                saxreader_parse_endtag(locator);
                if (list_empty(&locator->elements))
                    locator->state = SAX_PARSER_EPILOG;
                else
                    locator->state = SAX_PARSER_CONTENT;
                break;
            case SAX_PARSER_CONTENT:
                saxreader_more(locator);

                if (saxreader_peek(locator, L"<", 1))
                {
                    if (saxreader_peek(locator, L"</", 2))
                    {
                        locator->state = SAX_PARSER_END_TAG;
                    }
                    else if (saxreader_peek(locator, L"<?", 2))
                    {
                        saxreader_parse_pi(locator);
                        locator->state = SAX_PARSER_CONTENT;
                    }
                    else if (saxreader_peek(locator, L"<!--", 4))
                    {
                        saxreader_parse_comment(locator);
                        locator->state = SAX_PARSER_CONTENT;
                    }
                    else if (saxreader_peek(locator, L"<![CDATA[", 9))
                    {
                        locator->state = SAX_PARSER_CDATA;
                    }
                    else
                    {
                        locator->state = SAX_PARSER_START_TAG;
                    }
                }
                else if (saxreader_peek(locator, L"&", 1))
                {
                    saxreader_parse_reference(locator);
                }
                else
                {
                    saxreader_parse_chardata(locator);
                }
                break;
            case SAX_PARSER_CDATA:
                saxreader_parse_cdata(locator);
                locator->state = SAX_PARSER_CONTENT;
                break;
            default:
                locator->state = SAX_PARSER_EOF;
        }
    }

    hr = locator->status;
    ISAXLocator_Release(&locator->ISAXLocator_iface);

    return hr;
}

static HRESULT saxreader_parse(struct saxreader *reader, VARIANT input, bool vbInterface)
{
    ISequentialStream *stream;
    HRESULT hr;

    TRACE("%p, %s.\n", reader, debugstr_variant(&input));

    switch (V_VT(&input))
    {
        case VT_BSTR:
        case VT_BSTR|VT_BYREF:
        {
            BSTR str = V_ISBYREF(&input) ? *V_BSTRREF(&input) : V_BSTR(&input);

            if (FAILED(hr = stream_wrapper_create((const void *)str, SysStringByteLen(str), &stream)))
                return hr;
            hr = saxreader_parse_stream(reader, stream, true, vbInterface);
            ISequentialStream_Release(stream);
            break;
        }
        case VT_ARRAY|VT_UI1:
        {
            LONG lBound, uBound;
            ULONG size;
            void *data;

            if (FAILED(hr = SafeArrayGetLBound(V_ARRAY(&input), 1, &lBound))) return hr;
            if (FAILED(hr = SafeArrayGetUBound(V_ARRAY(&input), 1, &uBound))) return hr;
            if (FAILED(hr = SafeArrayAccessData(V_ARRAY(&input), &data))) return hr;

            size = (uBound - lBound + 1) * SafeArrayGetElemsize(V_ARRAY(&input));
            if (SUCCEEDED(hr = stream_wrapper_create(data, size, &stream)))
            {
                hr = saxreader_parse_stream(reader, stream, false, vbInterface);
                ISequentialStream_Release(stream);
            }
            SafeArrayUnaccessData(V_ARRAY(&input));
            break;
        }
        case VT_UNKNOWN:
        case VT_DISPATCH:
        {
            ISequentialStream *stream = NULL;
            IXMLDOMDocument *xmlDoc;

            if (!V_UNKNOWN(&input))
                return E_INVALIDARG;

            if (IUnknown_QueryInterface(V_UNKNOWN(&input), &IID_IXMLDOMDocument, (void**)&xmlDoc) == S_OK)
            {
                BSTR bstrData;

                IXMLDOMDocument_get_xml(xmlDoc, &bstrData);
                stream_wrapper_create(bstrData, SysStringByteLen(bstrData), &stream);
                hr = saxreader_parse_stream(reader, stream, false, vbInterface);
                ISequentialStream_Release(stream);
                IXMLDOMDocument_Release(xmlDoc);
                SysFreeString(bstrData);
                break;
            }

            /* try base interface first */
            IUnknown_QueryInterface(V_UNKNOWN(&input), &IID_ISequentialStream, (void **)&stream);
            if (!stream)
                /* this should never happen if IStream is implemented properly, but just in case */
                IUnknown_QueryInterface(V_UNKNOWN(&input), &IID_IStream, (void **)&stream);

            if (stream)
            {
                hr = saxreader_parse_stream(reader, stream, false, vbInterface);
                ISequentialStream_Release(stream);
            }
            else
            {
                WARN("IUnknown* input doesn't support any of expected interfaces\n");
                hr = E_INVALIDARG;
            }

            break;
        }
        default:
            WARN("input type %d is not supported\n", V_VT(&input));
            hr = E_INVALIDARG;
    }

    return hr;
}

static HRESULT internal_vbonDataAvailable(void *obj, char *ptr, DWORD len)
{
    struct saxreader *reader = obj;
    ISequentialStream *stream;
    HRESULT hr;

    if (SUCCEEDED(hr = stream_wrapper_create(ptr, len, &stream)))
    {
        hr = saxreader_parse_stream(reader, stream, false, true);
        ISequentialStream_Release(stream);
    }
    return hr;
}

static HRESULT internal_onDataAvailable(void *obj, char *ptr, DWORD len)
{
    struct saxreader *reader = obj;
    ISequentialStream *stream;
    HRESULT hr;

    if (SUCCEEDED(hr = stream_wrapper_create(ptr, len, &stream)))
    {
        hr = saxreader_parse_stream(reader, stream, false, false);
        ISequentialStream_Release(stream);
    }
    return hr;
}

static HRESULT saxreader_parse_url(struct saxreader *reader, const WCHAR *url, bool vbInterface)
{
    IMoniker *mon;
    bsc_t *bsc;
    HRESULT hr;

    TRACE("%p, %s.\n", reader, debugstr_w(url));

    if (!url && reader->version < MSXML4)
        return E_INVALIDARG;

    hr = create_moniker_from_url(url, &mon);
    if (FAILED(hr))
        return hr;

    if (vbInterface) hr = bind_url(mon, internal_vbonDataAvailable, reader, &bsc);
    else hr = bind_url(mon, internal_onDataAvailable, reader, &bsc);
    IMoniker_Release(mon);

    if (FAILED(hr))
        return hr;

    return detach_bsc(bsc);
}

static HRESULT saxreader_put_handler_from_variant(struct saxreader *reader, enum saxhandler_type type,
        const VARIANT *v, bool vb)
{
    const IID *riid;

    if (V_VT(v) == VT_EMPTY)
        return saxreader_put_handler(reader, type, NULL, vb);

    switch (type)
    {
    case SAXDeclHandler:
        riid = vb ? &IID_IVBSAXDeclHandler : &IID_ISAXDeclHandler;
        break;
    case SAXLexicalHandler:
        riid = vb ? &IID_IVBSAXLexicalHandler : &IID_ISAXLexicalHandler;
        break;
    case SAXExtensionHandler:
        riid = &IID_ISAXExtensionHandler;
        if (vb) return E_UNEXPECTED;
        break;
    default:
        ERR("wrong handler type %d\n", type);
        return E_FAIL;
    }

    switch (V_VT(v))
    {
    case VT_DISPATCH:
    case VT_UNKNOWN:
    {
        IUnknown *handler = NULL;

        if (V_UNKNOWN(v))
        {
            HRESULT hr = IUnknown_QueryInterface(V_UNKNOWN(v), riid, (void**)&handler);
            if (FAILED(hr)) return hr;
        }

        saxreader_put_handler(reader, type, handler, vb);
        if (handler) IUnknown_Release(handler);
        break;
    }
    default:
        ERR("value type %d not supported\n", V_VT(v));
        return E_INVALIDARG;
    }

    return S_OK;
}

HRESULT variant_get_int_property(const VARIANT *v, int *ret)
{
    VARIANT dest;

    VariantInit(&dest);
    if (FAILED(VariantChangeType(&dest, v, 0, VT_I4)))
        return E_FAIL;

    *ret = V_I4(&dest);
    return S_OK;
}

static HRESULT saxreader_put_property(struct saxreader *reader, const WCHAR *prop, VARIANT value, bool vbInterface)
{
    VARIANT *v;

    TRACE("%p, %s, %s.\n", reader, debugstr_w(prop), debugstr_variant(&value));

    if (reader->isParsing) return E_FAIL;

    v = V_VT(&value) == (VT_VARIANT|VT_BYREF) ? V_VARIANTREF(&value) : &value;

    if (!wcscmp(prop, L"http://xml.org/sax/properties/declaration-handler"))
        return saxreader_put_handler_from_variant(reader, SAXDeclHandler, v, vbInterface);

    if (!wcscmp(prop, L"http://xml.org/sax/properties/lexical-handler"))
        return saxreader_put_handler_from_variant(reader, SAXLexicalHandler, v, vbInterface);

    if (!wcscmp(prop, L"http://winehq.org/sax/properties/extension-handler"))
        return saxreader_put_handler_from_variant(reader, SAXExtensionHandler, v, vbInterface);

    if (!wcscmp(prop, L"max-xml-size"))
    {
        int size;

        if (FAILED(variant_get_int_property(&value, &size)))
            return E_FAIL;

        if (size < 0)
            return E_INVALIDARG;

        if (reader->version >= MSXML4 && size > 4194304)
            return E_INVALIDARG;
        if (reader->version < MSXML4 && size >= 4194304)
            return E_INVALIDARG;

        reader->max_xml_size = size;
        return S_OK;
    }

    if (!wcscmp(prop, L"max-element-depth"))
    {
        int depth;

        if (FAILED(variant_get_int_property(&value, &depth)))
            return E_FAIL;

        if (depth < 0)
            return E_INVALIDARG;

        reader->max_element_depth = depth;
        return S_OK;
    }

    FIXME("(%p)->(%s:%s): unsupported property\n", reader, debugstr_w(prop), debugstr_variant(v));

    if (!wcscmp(prop, L"charset"))
        return E_NOTIMPL;

    if (!wcscmp(prop, L"http://xml.org/sax/properties/dom-node"))
        return E_FAIL;

    if (!wcscmp(prop, L"input-source"))
        return E_NOTIMPL;

    if (!wcscmp(prop, L"schema-declaration-handler"))
        return E_NOTIMPL;

    if (!wcscmp(prop, L"xmldecl-encoding"))
        return E_FAIL;

    if (!wcscmp(prop, L"xmldecl-standalone"))
        return E_FAIL;

    if (!wcscmp(L"xmldecl-version", prop))
        return E_FAIL;

    return E_INVALIDARG;
}

static HRESULT saxreader_get_property(const struct saxreader *reader, const WCHAR *prop, VARIANT *value, bool vb)
{
    if (!value) return E_POINTER;

    if (!wcscmp(prop, L"http://xml.org/sax/properties/lexical-handler"))
    {
        V_VT(value) = vb ? VT_DISPATCH : VT_UNKNOWN;
        saxreader_get_handler(reader, SAXLexicalHandler, vb, (void**)&V_UNKNOWN(value));
        return S_OK;
    }

    if (!wcscmp(prop, L"http://xml.org/sax/properties/declaration-handler"))
    {
        V_VT(value) = vb ? VT_DISPATCH : VT_UNKNOWN;
        saxreader_get_handler(reader, SAXDeclHandler, vb, (void**)&V_UNKNOWN(value));
        return S_OK;
    }

    if (!wcscmp(prop, L"xmldecl-version"))
    {
        V_VT(value) = VT_BSTR;
        return return_bstr(reader->xmldecl_version, &V_BSTR(value));
    }

    if (!wcscmp(prop, L"xmldecl-encoding"))
    {
        V_VT(value) = VT_BSTR;
        return return_bstr(reader->xmldecl_encoding, &V_BSTR(value));
    }

    if (!wcscmp(prop, L"xmldecl-standalone"))
    {
        V_VT(value) = VT_BSTR;
        return return_bstr(reader->xmldecl_encoding, &V_BSTR(value));
    }

    if (!wcscmp(prop, L"max-xml-size"))
    {
        V_VT(value) = VT_I4;
        V_I4(value) = reader->max_xml_size;
        return S_OK;
    }

    if (!wcscmp(prop, L"max-element-depth"))
    {
        V_VT(value) = VT_I4;
        V_I4(value) = reader->max_element_depth;
        return S_OK;
    }

    FIXME("(%p)->(%s) unsupported property\n", reader, debugstr_w(prop));

    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_QueryInterface(IVBSAXXMLReader *iface, REFIID riid, void **obj)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IVBSAXXMLReader))
    {
        *obj = iface;
    }
    else if (IsEqualGUID(riid, &IID_ISAXXMLReader))
    {
        *obj = &reader->ISAXXMLReader_iface;
    }
    else if (IsEqualGUID(riid, &IID_ISAXXMLReaderExtension))
    {
        *obj = &reader->ISAXXMLReaderExtension_iface;
    }
    else if (dispex_query_interface(&reader->dispex, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IVBSAXXMLReader_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI saxxmlreader_AddRef(IVBSAXXMLReader *iface)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    LONG refcount = InterlockedIncrement(&reader->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI saxxmlreader_Release(IVBSAXXMLReader *iface)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    LONG refcount = InterlockedDecrement(&reader->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        for (int i = 0; i < ARRAYSIZE(reader->saxhandlers); ++i)
        {
            struct saxanyhandler_iface *saxiface = &reader->saxhandlers[i].u.anyhandler;

            if (saxiface->handler)
                IUnknown_Release(saxiface->handler);

            if (saxiface->vbhandler)
                IUnknown_Release(saxiface->vbhandler);
        }

        SysFreeString(reader->xmldecl_version);
        SysFreeString(reader->empty_bstr);

        free(reader);
    }

    return refcount;
}

static HRESULT WINAPI saxxmlreader_GetTypeInfoCount(IVBSAXXMLReader *iface, UINT *count)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    return IDispatchEx_GetTypeInfoCount(&reader->dispex.IDispatchEx_iface, count);
}

static HRESULT WINAPI saxxmlreader_GetTypeInfo(IVBSAXXMLReader *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    return IDispatchEx_GetTypeInfo(&reader->dispex.IDispatchEx_iface, index, lcid, ti);
}

static HRESULT WINAPI saxxmlreader_GetIDsOfNames(IVBSAXXMLReader *iface, REFIID riid, LPOLESTR *names,
        UINT count, LCID lcid, DISPID *dispid)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    return IDispatchEx_GetIDsOfNames(&reader->dispex.IDispatchEx_iface, riid, names, count, lcid, dispid);
}

static HRESULT WINAPI saxxmlreader_Invoke(IVBSAXXMLReader *iface, DISPID dispid, REFIID riid,
        LCID lcid, WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *ei,
        UINT *argerr)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    return IDispatchEx_Invoke(&reader->dispex.IDispatchEx_iface, dispid, riid, lcid, flags, params, result, ei, argerr);
}

static HRESULT WINAPI saxxmlreader_getFeature(IVBSAXXMLReader *iface, BSTR name, VARIANT_BOOL *value)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    return ISAXXMLReader_getFeature(&reader->ISAXXMLReader_iface, name, value);
}

static HRESULT WINAPI saxxmlreader_putFeature(IVBSAXXMLReader *iface, BSTR name, VARIANT_BOOL value)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    return ISAXXMLReader_putFeature(&reader->ISAXXMLReader_iface, name, value);
}

static HRESULT WINAPI saxxmlreader_getProperty(IVBSAXXMLReader *iface, BSTR name, VARIANT *value)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(name), value);

    return saxreader_get_property(reader, name, value, true);
}

static HRESULT WINAPI saxxmlreader_putProperty(IVBSAXXMLReader *iface, BSTR name, VARIANT value)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_w(name), debugstr_variant(&value));

    return saxreader_put_property(reader, name, value, true);
}

static HRESULT WINAPI saxxmlreader_get_entityResolver(IVBSAXXMLReader *iface, IVBSAXEntityResolver **resolver)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    return saxreader_get_handler(reader, SAXEntityResolver, true, (void **)resolver);
}

static HRESULT WINAPI saxxmlreader_putref_entityResolver(IVBSAXXMLReader *iface, IVBSAXEntityResolver *resolver)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    return saxreader_put_handler(reader, SAXEntityResolver, resolver, true);
}

static HRESULT WINAPI saxxmlreader_get_contentHandler(IVBSAXXMLReader *iface, IVBSAXContentHandler **handler)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    return saxreader_get_handler(reader, SAXContentHandler, true, (void **)handler);
}

static HRESULT WINAPI saxxmlreader_putref_contentHandler(IVBSAXXMLReader *iface, IVBSAXContentHandler *handler)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    return saxreader_put_handler(reader, SAXContentHandler, handler, true);
}

static HRESULT WINAPI saxxmlreader_get_dtdHandler(IVBSAXXMLReader *iface, IVBSAXDTDHandler **handler)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    return saxreader_get_handler(reader, SAXDTDHandler, true, (void **)handler);
}

static HRESULT WINAPI saxxmlreader_putref_dtdHandler(IVBSAXXMLReader *iface, IVBSAXDTDHandler *handler)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    return saxreader_put_handler(reader, SAXDTDHandler, handler, true);
}

static HRESULT WINAPI saxxmlreader_get_errorHandler(IVBSAXXMLReader *iface, IVBSAXErrorHandler **handler)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    return saxreader_get_handler(reader, SAXErrorHandler, true, (void **)handler);
}

static HRESULT WINAPI saxxmlreader_putref_errorHandler(IVBSAXXMLReader *iface, IVBSAXErrorHandler *handler)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    return saxreader_put_handler(reader, SAXErrorHandler, handler, true);
}

static HRESULT WINAPI saxxmlreader_get_baseURL(IVBSAXXMLReader *iface, BSTR *url)
{
    FIXME("%p, %p stub\n", iface, url);

    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_put_baseURL(IVBSAXXMLReader *iface, BSTR url)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    return ISAXXMLReader_putBaseURL(&reader->ISAXXMLReader_iface, url);
}

static HRESULT WINAPI saxxmlreader_get_secureBaseURL(IVBSAXXMLReader *iface, BSTR *url)
{
    FIXME("%p, %p stub\n", iface, url);

    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_put_secureBaseURL(IVBSAXXMLReader *iface, BSTR url)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    return ISAXXMLReader_putSecureBaseURL(&reader->ISAXXMLReader_iface, url);
}

static HRESULT WINAPI saxxmlreader_parse(IVBSAXXMLReader *iface, VARIANT input)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    return saxreader_parse(reader, input, true);
}

static HRESULT WINAPI saxxmlreader_parseURL(IVBSAXXMLReader *iface, BSTR url)
{
    struct saxreader *reader = impl_from_IVBSAXXMLReader(iface);
    return saxreader_parse_url(reader, url, true);
}

static const struct IVBSAXXMLReaderVtbl vbsaxxmlreadervtbl =
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
    saxxmlreader_get_entityResolver,
    saxxmlreader_putref_entityResolver,
    saxxmlreader_get_contentHandler,
    saxxmlreader_putref_contentHandler,
    saxxmlreader_get_dtdHandler,
    saxxmlreader_putref_dtdHandler,
    saxxmlreader_get_errorHandler,
    saxxmlreader_putref_errorHandler,
    saxxmlreader_get_baseURL,
    saxxmlreader_put_baseURL,
    saxxmlreader_get_secureBaseURL,
    saxxmlreader_put_secureBaseURL,
    saxxmlreader_parse,
    saxxmlreader_parseURL
};

static HRESULT WINAPI isaxxmlreader_QueryInterface(ISAXXMLReader *iface, REFIID riid, void **obj)
{
    struct saxreader *reader = impl_from_ISAXXMLReader(iface);
    return IVBSAXXMLReader_QueryInterface(&reader->IVBSAXXMLReader_iface, riid, obj);
}

static ULONG WINAPI isaxxmlreader_AddRef(ISAXXMLReader *iface)
{
    struct saxreader *reader = impl_from_ISAXXMLReader(iface);
    return IVBSAXXMLReader_AddRef(&reader->IVBSAXXMLReader_iface);
}

static ULONG WINAPI isaxxmlreader_Release(ISAXXMLReader *iface)
{
    struct saxreader *reader = impl_from_ISAXXMLReader(iface);
    return IVBSAXXMLReader_Release(&reader->IVBSAXXMLReader_iface);
}

static HRESULT WINAPI isaxxmlreader_getFeature(ISAXXMLReader *iface, const WCHAR *name, VARIANT_BOOL *value)
{
    struct saxreader *reader = impl_from_ISAXXMLReader(iface);
    enum saxreader_feature feature;

    TRACE("%p, %s, %p.\n", iface, debugstr_w(name), value);

    feature = get_saxreader_feature(name);

    if (reader->version < MSXML4 && (feature == ExhaustiveErrors || feature == SchemaValidation))
        return E_INVALIDARG;
    if (feature == NormalizeLineBreaks && reader->version >= MSXML4)
        return E_INVALIDARG;

    if (feature == Namespaces ||
            feature == NamespacePrefixes ||
            feature == ExhaustiveErrors ||
            feature == SchemaValidation ||
            feature == NormalizeLineBreaks)
        return get_feature_value(reader, feature, value);

    FIXME("%p, %s, %p stub\n", iface, debugstr_w(name), value);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_putFeature(ISAXXMLReader *iface, const WCHAR *name, VARIANT_BOOL value)
{
    struct saxreader *reader = impl_from_ISAXXMLReader(iface);
    enum saxreader_feature feature;

    TRACE("%p, %s, %#x.\n", iface, debugstr_w(name), value);

    feature = get_saxreader_feature(name);

    if (feature == NormalizeLineBreaks && reader->version >= MSXML4)
        return E_INVALIDARG;

    /* accepted cases */
    if ((feature == ExhaustiveErrors && value == VARIANT_FALSE) ||
        (feature == SchemaValidation && value == VARIANT_FALSE) ||
         feature == Namespaces ||
         feature == NamespacePrefixes ||
         feature == NormalizeLineBreaks ||
         feature == NormalizeAttributeValues ||
         feature == ProhibitDTD)
    {
        return set_feature_value(reader, feature, value);
    }

    if (feature == LexicalHandlerParEntities ||
            feature == ExternalGeneralEntities ||
            feature == ExternalParameterEntities)
    {
        FIXME("%p, %s, %#x stub\n", iface, debugstr_w(name), value);
        return set_feature_value(reader, feature, value);
    }

    FIXME("%p, %s, %#x stub\n", iface, debugstr_w(name), value);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_getProperty(ISAXXMLReader *iface, const WCHAR *name, VARIANT *value)
{
    struct saxreader *reader = impl_from_ISAXXMLReader(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(name), value);

    return saxreader_get_property(reader, name, value, false);
}

static HRESULT WINAPI isaxxmlreader_putProperty(ISAXXMLReader *iface, const WCHAR *name, VARIANT value)
{
    struct saxreader *reader = impl_from_ISAXXMLReader(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_w(name), debugstr_variant(&value));

    return saxreader_put_property(reader, name, value, false);
}

static HRESULT WINAPI isaxxmlreader_getEntityResolver(ISAXXMLReader *iface, ISAXEntityResolver **resolver)
{
    struct saxreader *reader = impl_from_ISAXXMLReader(iface);
    return saxreader_get_handler(reader, SAXEntityResolver, false, (void **)resolver);
}

static HRESULT WINAPI isaxxmlreader_putEntityResolver(ISAXXMLReader *iface, ISAXEntityResolver *resolver)
{
    struct saxreader *reader = impl_from_ISAXXMLReader(iface);
    return saxreader_put_handler(reader, SAXEntityResolver, resolver, false);
}

static HRESULT WINAPI isaxxmlreader_getContentHandler(ISAXXMLReader *iface, ISAXContentHandler **handler)
{
    struct saxreader *reader = impl_from_ISAXXMLReader(iface);
    return saxreader_get_handler(reader, SAXContentHandler, false, (void **)handler);
}

static HRESULT WINAPI isaxxmlreader_putContentHandler(ISAXXMLReader *iface, ISAXContentHandler *handler)
{
    struct saxreader *reader = impl_from_ISAXXMLReader(iface);
    return saxreader_put_handler(reader, SAXContentHandler, handler, false);
}

static HRESULT WINAPI isaxxmlreader_getDTDHandler(ISAXXMLReader *iface, ISAXDTDHandler **handler)
{
    struct saxreader *reader = impl_from_ISAXXMLReader(iface);
    return saxreader_get_handler(reader, SAXDTDHandler, false, (void **)handler);
}

static HRESULT WINAPI isaxxmlreader_putDTDHandler(ISAXXMLReader *iface, ISAXDTDHandler *handler)
{
    struct saxreader *reader = impl_from_ISAXXMLReader(iface);
    return saxreader_put_handler(reader, SAXDTDHandler, handler, false);
}

static HRESULT WINAPI isaxxmlreader_getErrorHandler(ISAXXMLReader *iface, ISAXErrorHandler **handler)
{
    struct saxreader *reader = impl_from_ISAXXMLReader(iface);
    return saxreader_get_handler(reader, SAXErrorHandler, false, (void **)handler);
}

static HRESULT WINAPI isaxxmlreader_putErrorHandler(ISAXXMLReader *iface, ISAXErrorHandler *handler)
{
    struct saxreader *reader = impl_from_ISAXXMLReader(iface);
    return saxreader_put_handler(reader, SAXErrorHandler, handler, false);
}

static HRESULT WINAPI isaxxmlreader_getBaseURL(ISAXXMLReader *iface, const WCHAR **url)
{
    FIXME("%p, %p stub\n", iface, url);

    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_putBaseURL(ISAXXMLReader *iface, const WCHAR *url)
{
    FIXME("%p, %s stub\n", iface, debugstr_w(url));

    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_getSecureBaseURL(ISAXXMLReader *iface, const WCHAR **url)
{
    FIXME("%p, %p stub\n", iface, url);

    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_putSecureBaseURL(ISAXXMLReader *iface, const WCHAR *url)
{
    FIXME("%p, %s stub\n", iface, debugstr_w(url));

    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_parse(ISAXXMLReader *iface, VARIANT input)
{
    struct saxreader *reader = impl_from_ISAXXMLReader(iface);
    return saxreader_parse(reader, input, false);
}

static HRESULT WINAPI isaxxmlreader_parseURL(ISAXXMLReader *iface, const WCHAR *url)
{
    struct saxreader *reader = impl_from_ISAXXMLReader(iface);
    return saxreader_parse_url(reader, url, false);
}

static const struct ISAXXMLReaderVtbl saxxmlreadervtbl =
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

static HRESULT WINAPI saxxmlreader_extension_QueryInterface(ISAXXMLReaderExtension *iface, REFIID riid, void **obj)
{
    struct saxreader *reader = impl_from_ISAXXMLReaderExtension(iface);
    return IVBSAXXMLReader_QueryInterface(&reader->IVBSAXXMLReader_iface, riid, obj);
}

static ULONG WINAPI saxxmlreader_extension_AddRef(ISAXXMLReaderExtension *iface)
{
    struct saxreader *reader = impl_from_ISAXXMLReaderExtension(iface);
    return IVBSAXXMLReader_AddRef(&reader->IVBSAXXMLReader_iface);
}

static ULONG WINAPI saxxmlreader_extension_Release(ISAXXMLReaderExtension *iface)
{
    struct saxreader *reader = impl_from_ISAXXMLReaderExtension(iface);
    return IVBSAXXMLReader_Release(&reader->IVBSAXXMLReader_iface);
}

static HRESULT WINAPI saxxmlreader_extension_parseUTF16(ISAXXMLReaderExtension *iface, ISequentialStream *stream)
{
    struct saxreader *reader = impl_from_ISAXXMLReaderExtension(iface);
    return saxreader_parse_stream(reader, stream, true, false);
}

static const struct ISAXXMLReaderExtensionVtbl saxxmlreaderextensionvtbl =
{
    saxxmlreader_extension_QueryInterface,
    saxxmlreader_extension_AddRef,
    saxxmlreader_extension_Release,
    saxxmlreader_extension_parseUTF16,
};

static const tid_t saxreader_iface_tids[] =
{
    IVBSAXXMLReader_tid,
    0
};

static dispex_static_data_t saxreader_dispex =
{
    NULL,
    IVBSAXXMLReader_tid,
    NULL,
    saxreader_iface_tids
};

static HRESULT saxreader_create(MSXML_VERSION version, struct saxreader **reader)
{
    struct saxreader *object;

    *reader = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IVBSAXXMLReader_iface.lpVtbl = &vbsaxxmlreadervtbl;
    object->ISAXXMLReader_iface.lpVtbl = &saxxmlreadervtbl;
    object->ISAXXMLReaderExtension_iface.lpVtbl = &saxxmlreaderextensionvtbl;
    object->refcount = 1;
    object->features = Namespaces | NamespacePrefixes | NormalizeLineBreaks | NormalizeAttributeValues;
    object->version = version;
    object->empty_bstr = SysAllocString(L"");
    object->max_element_depth = version > MSXML3 ? 256 : 5000;

    init_dispex(&object->dispex, (IUnknown *)&object->IVBSAXXMLReader_iface, &saxreader_dispex);

    *reader = object;

    return S_OK;
}

HRESULT SAXXMLReader_create(MSXML_VERSION version, void **obj)
{
    struct saxreader *reader;
    HRESULT hr;

    TRACE("%p\n", obj);

    *obj = NULL;

    if (SUCCEEDED(hr = saxreader_create(version, &reader)))
        *obj = &reader->IVBSAXXMLReader_iface;

    TRACE("returning iface %p\n", *obj);

    return hr;
}

/* Fixed size buffer variant, to be used without markup. */
static void saxreader_parse_xmldecl_body(struct saxlocator *locator)
{
    struct parsed_name name;
    bool done = false;
    BSTR value;

    saxreader_skipspaces(locator);

    value = saxreader_parse_xmldecl_attribute(locator, &name);
    saxreader_parse_versioninfo(locator, &name, value);
    saxreader_free_name(&name);
    SysFreeString(value);

    saxreader_skipspaces(locator);
    if (locator->eos)
        return;

    while (!done && locator->status == S_OK)
    {
        value = saxreader_parse_xmldecl_attribute(locator, &name);

        if (!wcscmp(name.qname, L"encoding"))
        {
            saxreader_parse_encdecl(locator, &name, value);
        }
        else if (!wcscmp(name.qname, L"standalone"))
        {
            saxreader_parse_sddecl(locator, &name, value);
            done = true;
        }
        else
        {
            saxreader_set_error(locator, E_SAX_UNEXPECTED_ATTRIBUTE);
        }

        saxreader_free_name(&name);
        SysFreeString(value);

        if (locator->status != S_OK)
            continue;

        saxreader_skipspaces(locator);
        if (locator->eos)
            return;
    }

    saxreader_skipspaces(locator);
    if (!locator->eos)
        saxreader_set_error(locator, E_SAX_BAD_XMLDECL);
}

HRESULT parse_xml_decl_body(const WCHAR *text, BSTR *version, BSTR *encoding, BSTR *standalone)
{
    struct saxlocator *locator;
    ISequentialStream *stream;
    struct saxreader *reader;
    HRESULT hr;

    *version = *encoding = *standalone = NULL;

    if (!text)
        return E_SAX_XMLDECLSYNTAX;

    if (FAILED(hr = stream_wrapper_create(text, wcslen(text) * sizeof(WCHAR), &stream)))
        return hr;

    hr = saxreader_create(MSXML3, &reader);
    if (SUCCEEDED(hr))
    {
        hr = saxlocator_create(reader, stream, false, &locator);
        ISAXXMLReader_Release(&reader->ISAXXMLReader_iface);
    }

    if (SUCCEEDED(hr))
    {
        saxreader_detect_encoding(locator, true);

        saxreader_parse_xmldecl_body(locator);

        *version = saxreader_alloc_string(locator, locator->saxreader->xmldecl_version);
        *encoding = saxreader_alloc_string(locator, locator->saxreader->xmldecl_encoding);
        *standalone = saxreader_alloc_string(locator, locator->saxreader->xmldecl_standalone);

        hr = locator->status;

        ISAXLocator_Release(&locator->ISAXLocator_iface);
    }

    ISequentialStream_Release(stream);

    return hr;
}

static void saxlocator_standalone_init(struct saxlocator *locator, ISequentialStream *stream)
{
    locator->stream = stream;
    locator->buffer.encoding = XML_ENCODING_UTF16LE;
    locator->buffer.code_page = convert_get_codepage(XML_ENCODING_UTF16LE);
    locator->buffer.chunk_size = 4096;
    list_init(&locator->buffer.entities);
    saxreader_more(locator);
}

HRESULT parse_qualified_name(const WCHAR *src, struct parsed_name *name)
{
    struct saxlocator locator = { 0 };
    ISequentialStream *stream;
    HRESULT hr;

    if (FAILED(hr = stream_wrapper_create(src, wcslen(src) * sizeof(WCHAR), &stream)))
        return hr;
    saxlocator_standalone_init(&locator, stream);

    saxreader_parse_qname(&locator, name);

    encoded_buffer_cleanup(&locator.buffer.utf16);
    ISequentialStream_Release(stream);

    return locator.status;
}

void parsed_name_cleanup(struct parsed_name *name)
{
    saxreader_free_name(name);
}
