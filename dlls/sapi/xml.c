/*
 * Speech API (SAPI) XML parser implementation.
 *
 * Copyright 2025 Shaun Ren for CodeWeavers
 *
 * Based on ntdll/actctx.c
 * Copyright 2004 Jon Griffiths
 * Copyright 2007 Eric Pouech
 * Copyright 2007 Jacek Caban for CodeWeavers
 * Copyright 2007 Alexandre Julliard
 * Copyright 2013 Nikolay Sivov
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

#include <assert.h>

#define COBJMACROS

#include "objbase.h"

#include "sapiddk.h"
#include "sperror.h"

#include "wine/debug.h"

#include "sapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(sapi);


#define MAX_NAMESPACES 64

typedef struct
{
    const WCHAR  *ptr;
    unsigned int  len;
} xmlstr_t;

struct xml_elem
{
    xmlstr_t name;
    xmlstr_t ns;
    int      ns_pos;
};

struct xml_attr
{
    xmlstr_t name;
    xmlstr_t value;
};

struct xml_parser
{
    const WCHAR     *ptr;
    const WCHAR     *end;
    struct xml_attr  namespaces[MAX_NAMESPACES];
    int              ns_pos;
    BOOL             error;
};

static const xmlstr_t empty_xmlstr;

static const WCHAR ssml_ns[] = L"http://www.w3.org/2001/10/synthesis";

static inline const char *debugstr_xmlstr(const xmlstr_t *str)
{
    return debugstr_wn(str->ptr, str->len);
}

static inline BOOL xmlstr_eq(const xmlstr_t* xmlstr, const WCHAR *str)
{
    return !wcsncmp(xmlstr->ptr, str, xmlstr->len) && !str[xmlstr->len];
}

static inline BOOL xml_elem_eq(const struct xml_elem *elem, const WCHAR *ns, const WCHAR *name)
{
    if (!xmlstr_eq(&elem->name, name)) return FALSE;
    return xmlstr_eq(&elem->ns, ns);
}

static BOOL xml_name_eq(const struct xml_elem *elem1, const struct xml_elem *elem2)
{
    return (elem1->name.len == elem2->name.len &&
            elem1->ns.len == elem2->ns.len &&
            !wcsncmp(elem1->name.ptr, elem2->name.ptr, elem1->name.len) &&
            !wcsncmp(elem1->ns.ptr, elem2->ns.ptr, elem1->ns.len));
}

static BOOL set_error(struct xml_parser *parser)
{
    parser->error = TRUE;
    return FALSE;
}

static BOOL is_xmlns_attr(const struct xml_attr *attr)
{
    const int len = wcslen(L"xmlns");
    if (attr->name.len < len) return FALSE;
    if (wcsncmp(attr->name.ptr, L"xmlns", len)) return FALSE;
    return (attr->name.len == len || attr->name.ptr[len] == ':');
}

static void push_xmlns(struct xml_parser *parser, const struct xml_attr *attr)
{
    const int len = wcslen(L"xmlns");
    struct xml_attr *ns;

    if (parser->ns_pos == MAX_NAMESPACES - 1)
    {
        FIXME("Too many namespaces.\n");
        set_error(parser);
        return;
    }
    ns = &parser->namespaces[parser->ns_pos++];
    ns->value = attr->value;
    if (attr->name.len > len)
    {
        ns->name.ptr = attr->name.ptr + len + 1;
        ns->name.len = attr->name.len - len - 1;
    }
    else ns->name = empty_xmlstr;
}

static xmlstr_t find_xmlns(struct xml_parser *parser, const xmlstr_t *name)
{
    int i;

    for (i = parser->ns_pos - 1; i >= 0; i--)
    {
        if (parser->namespaces[i].name.len == name->len &&
            !wcsncmp(parser->namespaces[i].name.ptr, name->ptr, name->len))
            return parser->namespaces[i].value;
    }
    if (parser->ns_pos) WARN("Namespace %s not found.\n", debugstr_xmlstr(name));
    return empty_xmlstr;
}

static void skip_xml_spaces(struct xml_parser *parser)
{
    while (parser->ptr < parser->end && isxmlspace(*parser->ptr))
        parser->ptr++;
}

static BOOL next_xml_attr(struct xml_parser *parser, struct xml_attr *attr, BOOL *end)
{
    const WCHAR *ptr;
    WCHAR quote;

    *end = FALSE;

    if (parser->error) return FALSE;

    skip_xml_spaces(parser);

    if (parser->ptr == parser->end)
        return set_error(parser);

    if (*parser->ptr == '/')
    {
        parser->ptr++;
        if (parser->ptr == parser->end || *parser->ptr != '>')
            return set_error(parser);

        parser->ptr++;
        *end = TRUE;
        return FALSE;
    }

    if (*parser->ptr == '>')
    {
        parser->ptr++;
        return FALSE;
    }

    ptr = parser->ptr;
    while (ptr < parser->end && *ptr != '=' && *ptr != '>' && !isxmlspace(*ptr)) ptr++;

    if (ptr == parser->end)
        return set_error(parser);

    attr->name.ptr = parser->ptr;
    attr->name.len = ptr-parser->ptr;
    parser->ptr = ptr;

    /* skip spaces before '=' */
    while (ptr < parser->end && *ptr != '=' && isxmlspace(*ptr)) ptr++;
    if (ptr == parser->end || *ptr != '=')
        return set_error(parser);

    /* skip '=' itself */
    ptr++;
    if (ptr == parser->end)
        return set_error(parser);

    /* skip spaces after '=' */
    while (ptr < parser->end && *ptr != '"' && *ptr != '\'' && isxmlspace(*ptr)) ptr++;

    if (ptr == parser->end || (*ptr != '"' && *ptr != '\'')) return set_error(parser);

    quote = *ptr++;
    attr->value.ptr = ptr;
    if (ptr == parser->end)
        return set_error(parser);

    while (ptr < parser->end && *ptr != quote) ptr++;
    if (ptr == parser->end)
    {
        parser->ptr = parser->end;
        return set_error(parser);
    }

    attr->value.len = ptr - attr->value.ptr;
    parser->ptr = ptr + 1;
    if (parser->ptr != parser->end) return TRUE;

    return set_error(parser);
}

static void read_xml_elem(struct xml_parser *parser, struct xml_elem *elem)
{
    const WCHAR *ptr = parser->ptr;

    elem->ns = empty_xmlstr;
    elem->name.ptr = ptr;
    while (ptr < parser->end && !isxmlspace(*ptr) && *ptr != '>' && *ptr != '/')
    {
        if (*ptr == ':')
        {
            elem->ns.ptr = elem->name.ptr;
            elem->ns.len = ptr - elem->ns.ptr;
            elem->name.ptr = ptr + 1;
        }
        ptr++;
    }
    elem->name.len = ptr - elem->name.ptr;
    parser->ptr = ptr;
}

static inline BOOL is_special_xml_markup(const struct xml_elem *elem)
{
    return *elem->name.ptr == '!' || *elem->name.ptr == '?';
}

static BOOL skip_special_xml_markup(struct xml_parser *parser, struct xml_elem *elem)
{
    const WCHAR *ptr;

    if (parser->error) return FALSE;

    if (elem->name.len > 1 && elem->name.ptr[0] == '!' && elem->name.ptr[1] == '[')
    {
        /* <![...]]> */
        for (ptr = parser->ptr; ptr < parser->end - 3; ptr++)
        {
            if (ptr[0] == ']' && ptr[1] == ']' && ptr[2] == '>')
            {
                parser->ptr = ptr + 3;
                return TRUE;
            }
        }
    }
    else if (xmlstr_eq(&elem->name, L"!--"))
    {
        /* <!--...--> */
        for (ptr = parser->ptr; ptr < parser->end - 2; ptr++)
        {
            if (ptr[0] == '-' && ptr[1] == '-' && ptr[2] == '>')
            {
                parser->ptr = ptr + 3;
                return TRUE;
            }
        }
    }
    else if (*elem->name.ptr == '!')
    {
        /* <!...> */
        for (ptr = parser->ptr; ptr < parser->end; ptr++)
        {
            if (*ptr == '>')
            {
                parser->ptr = ptr + 1;
                return TRUE;
            }
        }
    }
    else if (*elem->name.ptr == '?')
    {
        /* <?...?> */
        for (ptr = parser->ptr; ptr < parser->end - 1; ptr++)
        {
            if (ptr[0] == '?' && ptr[1] == '>')
            {
                parser->ptr = ptr + 2;
                return TRUE;
            }
        }
    }

    return FALSE;
}

static BOOL next_xml_elem(struct xml_parser *parser, struct xml_elem *elem, const struct xml_elem *parent)
{
    const WCHAR *ptr;
    struct xml_attr attr;
    BOOL end = FALSE;

    parser->ns_pos = parent->ns_pos;  /* restore namespace stack to parent state */

    if (parser->error) return FALSE;

    skip_xml_spaces(parser);

    if (parser->ptr == parser->end || *parser->ptr != '<')
        return set_error(parser);
    parser->ptr++;

    /* check for element terminating the parent element */
    if (parser->ptr < parser->end && *parser->ptr == '/')
    {
        parser->ptr++;
        read_xml_elem(parser, elem);
        elem->ns = find_xmlns(parser, &elem->ns);
        if (!xml_name_eq(elem, parent))
        {
            ERR("Wrong closing element %s for %s.\n",
                 debugstr_xmlstr(&elem->name), debugstr_xmlstr(&parent->name));
            return set_error(parser);
        }
        skip_xml_spaces(parser);
        if (parser->ptr == parser->end || *parser->ptr++ != '>')
            return set_error(parser);
        return FALSE;
    }

    read_xml_elem(parser, elem);
    if (!elem->name.len)
        return set_error(parser);

    if (!is_special_xml_markup(elem))
    {
        /* parse namespace attributes */
        ptr = parser->ptr;
        while (next_xml_attr(parser, &attr, &end))
        {
            if (is_xmlns_attr(&attr)) push_xmlns(parser, &attr);
        }
        parser->ptr = ptr;
        elem->ns = find_xmlns(parser, &elem->ns);
        elem->ns_pos = parser->ns_pos;
    }

    if (parser->ptr != parser->end) return TRUE;
    else return set_error(parser);
}

static HRESULT parse_ssml_speak_elem(struct xml_parser *parser, SPVSTATE *state)
{
    return E_NOTIMPL;
}

static HRESULT parse_ssml_contents(const WCHAR *contents, const WCHAR *end, SPVSTATE *state, SPVTEXTFRAG **frag_list)
{
    struct xml_parser parser = {0};
    struct xml_elem parent = {0};
    struct xml_elem elem;
    HRESULT hr;

    parser.ptr = contents;
    parser.end = end;

    /* Default SSML namespace. */
    parser.namespaces[0].name = empty_xmlstr;
    parser.namespaces[0].value.ptr = ssml_ns;
    parser.namespaces[0].value.len = wcslen(ssml_ns);
    parser.ns_pos = 1;

    parent.ns = parser.namespaces[0].value;
    parent.ns_pos = 1;

    for (;;)
    {
        if (!next_xml_elem(&parser, &elem, &parent))
            return SPERR_UNSUPPORTED_FORMAT;

        if (!is_special_xml_markup(&elem))
            break;
        if (!skip_special_xml_markup(&parser, &elem))
            return SPERR_UNSUPPORTED_FORMAT;
    }

    if (!xml_elem_eq(&elem, ssml_ns, L"speak"))
        return SPERR_UNSUPPORTED_FORMAT;
    if (FAILED(hr = parse_ssml_speak_elem(&parser, state)))
        return hr;

    if (next_xml_elem(&parser, &elem, &parent))
    {
        ERR("Unexpected element %s after <speak>.\n", debugstr_xmlstr(&elem.name));
        return SPERR_UNSUPPORTED_FORMAT;
    }

    if (parser.error)
        return SPERR_UNSUPPORTED_FORMAT;

    return S_OK;
}

HRESULT parse_sapi_xml(const WCHAR *contents, DWORD parse_flag, BOOL persist, SPVSTATE *global_state,
        SPVTEXTFRAG **frag_list)
{
    SPVSTATE state = *global_state;
    const WCHAR *end;
    HRESULT hr;

    TRACE("(%p, %#lx, %d, %p, %p).\n", contents, parse_flag, persist, global_state, frag_list);

    if (parse_flag == SPF_PARSE_SAPI)
    {
        FIXME("SAPI XML parsing is not implemented.\n");
        return E_NOTIMPL;
    }

    assert(!state.pPhoneIds);
    assert(!state.Context.pCategory && !state.Context.pBefore && !state.Context.pAfter);

    end = contents + wcslen(contents);

    if (FAILED(hr = parse_ssml_contents(contents, end, &state, frag_list)))
        return hr;

    if (persist)
    {
        assert(!state.pPhoneIds);
        assert(!state.Context.pCategory && !state.Context.pBefore && !state.Context.pAfter);

        *global_state = state;
    }

    return S_OK;
}
