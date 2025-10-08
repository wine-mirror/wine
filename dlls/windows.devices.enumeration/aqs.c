/* Advanced Query Syntax parser
 *
 * Copyright 2025 Vibhav Pant
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
#include <stdarg.h>

#include <windef.h>
#include <devpropdef.h>
#include <devfiltertypes.h>
#include <propvarutil.h>

#include <wine/debug.h>

#include "aqs.h"
#include "aqs.tab.h"

WINE_DEFAULT_DEBUG_CHANNEL( aqs );

static BOOL is_idchar( WCHAR chr )
{
    static const WCHAR legal[] = { 5, '#', '-', '.', '_', '{', '}' };
    int i;

    if (iswdigit( chr ) || iswalpha( chr ) || (chr >= 125 && chr <= 255)) return TRUE;
    for (i = 0; i < ARRAY_SIZE( legal ); i++) if (chr == legal[i]) return TRUE;
    return FALSE;
}

static int keyword_type( const WCHAR *str, unsigned int len )
{
    if (!wcsncmp( str, L"AND", len ))
        return TK_AND;
    if (!wcsncmp( str, L"NOT", len ))
        return TK_NOT;
    if (!wcsncmp( str, L"OR", len ))
        return TK_OR;
    if (!wcsnicmp( str, L"System.StructuredQueryType.Boolean#False", len ))
        return TK_FALSE;
    if (!wcsnicmp( str, L"System.StructuredQueryType.Boolean#True", len ))
        return TK_TRUE;

    return TK_ID;
}

int get_token( const WCHAR *str, enum aqs_tokentype *token )
{
    int i;

    switch (str[0])
    {
    case '\0':
        *token = 0; /* EOF */
        return 0;
    case ' ':
    case '\t':
    case '\r':
    case '\n':
        for (i = 1; iswspace( str[i] ); i++) /*nothing */;
        *token = TK_WHITESPACE;
        return i;
    case '(':
        *token = TK_LEFTPAREN;
        return 1;
    case ')':
        *token = TK_RIGHTPAREN;
        return 1;
    case '[':
        if (str[1] != ']') break; /* illegal */
        *token = TK_NULL;
        return 2;
    case ':':
        *token = TK_COLON;
        return 1;
    case '=':
        *token = TK_EQUAL;
        return 1;
    case L'\u2260': /* ≠, NOT EQUALS TO */
        *token = TK_NOTEQUAL;
        return 1;
    case '-':
        if (iswdigit( str[1] ))
        {
            *token = TK_MINUS;
            return 1;
        }
        *token = TK_NOTEQUAL;
        /* Both -- and - are used as not-equals operators. */
        return str[1] == '-' ? 2 : 1;
    case '<':
        switch (str[1])
        {
        case '=':
            *token = TK_LTE;
            return 2;
        case '>':
            *token = TK_NOTEQUAL;
            return 2;
        default:
            *token = TK_LT;
            return 1;
        }
    case L'\u2264': /* ≤, LESS-THAN OR EQUAL TO */
        *token = TK_LTE;
        return 1;
    case '>':
        if (str[1] == '=')
        {
            *token = TK_GTE;
            return 2;
        }
        *token = TK_GT;
        return 1;
    case L'\u2265': /* ≥, GREATER-THAN OR EQUAL TO */
        *token = TK_GTE;
        return 1;
    case '~':
        *token = TK_TILDE;
        return 1;
    case '!':
        *token = TK_EXCLAM;
        return 1;
    case '$':
        *token = TK_DOLLAR;
        return 1;
    case '\"':
        /* lookup for end double quote, skipping any "" escaped double quotes */
        for (i = 1; str[i]; i++) if (str[i] == '\"' && str[++i] != '\"') break;
        if (i == 1 || str[i - 1] != '\"') break; /* illegal */
        *token = TK_STRING;
        return i;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        *token = TK_INTEGER;
        for (i = 1; iswdigit( str[i] ); i++) /* nothing */;
        return i;
    default:
        if (!is_idchar( str[0] )) break;
        for (i = 1; is_idchar( str[i] ); i++) /* nothing */;
        *token = keyword_type( str, i );
        return i;
    }
    *token = 257; /* UNDEF */
    return 0;
}

UINT aqs_lex( void *p, struct aqs_parser *parser )
{
    enum aqs_tokentype token = -1;
    struct string *str = p;

    do
    {
        parser->idx += parser->len;
        if ((parser->len = get_token( &parser->query[parser->idx], &token )))
        {
            str->data = &parser->query[parser->idx];
            str->len = parser->len;
        }
        parser->all_whitespace &= ((token == TK_WHITESPACE) || (token == 0 /* EOF */));
    } while (token == TK_WHITESPACE);
    return token;
}

HRESULT aqs_parse_query( const WCHAR *str, struct aqs_expr **expr, BOOL *all_whitespace )
{
    struct aqs_parser parser = {0};
    HRESULT hr;
    int ret;

    *expr = NULL;
    if (!*str) return S_OK;

    parser.all_whitespace = TRUE;
    parser.query = str;
    if (FAILED(hr = CoCreateInstance( &CLSID_PropertySystem, NULL, CLSCTX_INPROC_SERVER, &IID_IPropertySystem, (void **)&parser.propsys )))
        return hr;
    aqs_debug = TRACE_ON( aqs );
    ret = aqs_parse( &parser );
    if (!ret && parser.expr)
        *expr = parser.expr;
    else
        hr = FAILED(parser.error) ? parser.error : E_INVALIDARG;

    if (all_whitespace) *all_whitespace = parser.all_whitespace;
    IPropertySystem_Release( parser.propsys );
    return hr;
}

HRESULT get_integer( struct aqs_parser *parser, PROPVARIANT *val )
{
    const WCHAR *str = &parser->query[parser->idx];
    int i, num = 0;

    for (i = 0; i < parser->len; i++)
        num = (str[i] - '0') + num * 10;
    val->vt = VT_UI4;
    val->lVal = num;
    return S_OK;
}

HRESULT get_string( struct aqs_parser *parser, const struct string *p, BOOL id, PROPVARIANT *val )
{
    const WCHAR *str = p->data;
    SIZE_T len = p->len;
    WCHAR *buf;

    if (!id)
    {
        str++;
        len -= 2;
    }
    if (!(buf = CoTaskMemAlloc((len + 1) * sizeof( WCHAR )))) return (parser->error = E_OUTOFMEMORY);
    memcpy( buf, str, len * sizeof( WCHAR ) );
    buf[len] = 0;
    val->vt = VT_LPWSTR;
    val->pwszVal = buf;
    return S_OK;
}

void get_boolean( struct aqs_parser *parser, BOOL b, PROPVARIANT *val )
{
    val->vt = VT_BOOL;
    val->boolVal = b ? VARIANT_TRUE : VARIANT_FALSE;
}

HRESULT join_expr( struct aqs_parser *parser, struct aqs_expr *first, struct aqs_expr *second, struct aqs_expr **ret_expr )
{
    ULONG len = first->len + second->len;
    struct aqs_expr *expr;

    if (!(expr = calloc( 1, offsetof( struct aqs_expr, filters[len] )))) return (parser->error = E_OUTOFMEMORY);

    expr->len = len;
    memcpy( &expr->filters[0], first->filters, sizeof( *expr->filters ) * first->len );
    memcpy( &expr->filters[first->len], second->filters, sizeof( *expr->filters ) * second->len );
    *ret_expr = expr;

    /* Use free instead of free_aqs_expr to reuse the property value buffers in the sub-expressions. */
    free( first );
    free( second );
    return S_OK;
}

HRESULT get_boolean_binary_expr( struct aqs_parser *parser, DEVPROP_OPERATOR op, struct aqs_expr *first, struct aqs_expr *second, struct aqs_expr **ret_expr )
{
    ULONG len = 2 + first->len + second->len;
    struct aqs_expr *expr;

    assert( op == DEVPROP_OPERATOR_AND_OPEN || op == DEVPROP_OPERATOR_OR_OPEN );

    if (!(expr = calloc( 1, offsetof( struct aqs_expr, filters[len] ) ))) return (parser->error = E_OUTOFMEMORY);

    expr->len = len;
    expr->filters[0].Operator = op;
    memcpy( &expr->filters[1], first->filters, sizeof( *expr->filters ) * first->len );
    memcpy( &expr->filters[1 + first->len], second->filters, sizeof( *expr->filters ) * second->len );
    expr->filters[len - 1].Operator = op + (DEVPROP_OPERATOR_AND_CLOSE - DEVPROP_OPERATOR_AND_OPEN);
    *ret_expr = expr;

    free( first );
    free( second );
    return S_OK;
}

HRESULT get_boolean_not_expr( struct aqs_parser *parser, struct aqs_expr *inner, struct aqs_expr **ret_expr )
{
    ULONG len = 2 + inner->len;
    struct aqs_expr *expr;

    if (!(expr = calloc( 1, offsetof( struct aqs_expr, filters[len] )))) return (parser->error = E_OUTOFMEMORY);

    expr->len = len;
    expr->filters[0].Operator = DEVPROP_OPERATOR_NOT_OPEN;
    memcpy( &expr->filters[1], inner->filters, sizeof( *expr->filters ) * inner->len );
    expr->filters[len - 1].Operator = DEVPROP_OPERATOR_NOT_CLOSE;
    *ret_expr = expr;

    free( inner );
    return S_OK;
}

static HRESULT propval_to_devprop( const PROPVARIANT *comparand_val, const DEVPROPKEY *prop_key, VARTYPE prop_vt, DEVPROPERTY *devprop )
{
    union
    {
        BYTE byte;
        UINT16 int16;
        UINT32 int32;
        UINT64 int64;
        GUID guid;
        DEVPROP_BOOLEAN boolean;
    } devprop_basic_val = {0};
    PROPVARIANT tmp = {0};
    HRESULT hr;

    devprop->CompKey.Key = *prop_key;
    if (comparand_val->vt != VT_EMPTY)
    {
        if (FAILED(hr = PropVariantChangeType( &tmp, comparand_val, 0, prop_vt )))
            return (hr == E_FAIL || hr == E_NOTIMPL) ? E_INVALIDARG : hr;
        switch (prop_vt)
        {
        case VT_CLSID:
            devprop->Type = DEVPROP_TYPE_GUID;
            devprop_basic_val.guid = *tmp.puuid;
            devprop->BufferSize = sizeof( devprop_basic_val.guid );
            break;
        case VT_I1:
        case VT_UI1:
            devprop->Type = prop_vt == VT_I1 ? DEVPROP_TYPE_SBYTE : DEVPROP_TYPE_BYTE;
            devprop_basic_val.byte = tmp.bVal;
            devprop->BufferSize = sizeof( devprop_basic_val.byte );
            break;
        case VT_BOOL:
            devprop->Type = DEVPROP_TYPE_BOOLEAN;
            devprop_basic_val.boolean = tmp.boolVal ? DEVPROP_TRUE : DEVPROP_FALSE;
            devprop->BufferSize = sizeof( devprop_basic_val.boolean );
            break;
        case VT_I2:
        case VT_UI2:
            devprop->Type = prop_vt == VT_I2 ? DEVPROP_TYPE_INT16 : DEVPROP_TYPE_UINT16;
            devprop_basic_val.int16 = tmp.uiVal;
            devprop->BufferSize = sizeof( devprop_basic_val.int16 );
            break;
        case VT_I4:
        case VT_UI4:
            devprop->Type = prop_vt == VT_I4 ? DEVPROP_TYPE_INT32 : DEVPROP_TYPE_UINT32;
            devprop_basic_val.int32 = tmp.ulVal;
            devprop->BufferSize = sizeof( devprop_basic_val.int32 );
            break;
        case VT_I8:
        case VT_UI8:
            devprop->Type = prop_vt == VT_I8 ? DEVPROP_TYPE_INT64 : DEVPROP_TYPE_UINT64;
            devprop_basic_val.int64 = tmp.uhVal.QuadPart;
            devprop->BufferSize = sizeof( devprop_basic_val.int64 );
            break;
        case VT_LPWSTR:
            devprop->Type = DEVPROP_TYPE_STRING;
            devprop->BufferSize = (wcslen( tmp.pwszVal ) + 1) * sizeof( WCHAR );
            break;
        default:
            FIXME( "Unsupported property VARTYPE %d, treating comparand as string.\n", prop_vt );
            PropVariantClear( &tmp );
            if (FAILED(hr = PropVariantChangeType( &tmp, comparand_val, 0, VT_LPWSTR )))
                return (hr == E_FAIL || hr == E_NOTIMPL) ? E_INVALIDARG : hr;
            devprop->Type = DEVPROP_TYPE_STRING;
            devprop->BufferSize = (wcslen( tmp.pwszVal ) + 1) * sizeof( WCHAR );
            break;
        }
    }
    else
    {
        devprop->Type = DEVPROP_TYPE_EMPTY;
        devprop->BufferSize = 0;
    }

    devprop->CompKey.Store = DEVPROP_STORE_SYSTEM;
    devprop->CompKey.LocaleName = NULL;
    devprop->Buffer = NULL;
    if (devprop->BufferSize && !(devprop->Buffer = calloc( 1, devprop->BufferSize )))
    {
        PropVariantClear( &tmp );
        return E_OUTOFMEMORY;
    }
    switch (devprop->Type)
    {
    case DEVPROP_TYPE_STRING:
        wcscpy( devprop->Buffer, tmp.pwszVal );
        break;
    case DEVPROP_TYPE_EMPTY:
        break;
    default:
        memcpy( devprop->Buffer, &devprop_basic_val, devprop->BufferSize );
        break;
    }
    PropVariantClear( &tmp );
    return S_OK;
}

HRESULT get_compare_expr( struct aqs_parser *parser, DEVPROP_OPERATOR op, PROPVARIANT *prop_name, PROPVARIANT *val, struct aqs_expr **ret_expr )
{
    IPropertyDescription *prop_desc = NULL;
    DEVPROP_FILTER_EXPRESSION *filter;
    struct aqs_expr *expr;
    VARTYPE type;
    HRESULT hr;

    assert( prop_name->vt == VT_LPWSTR );
    if (!(expr = calloc( 1, offsetof( struct aqs_expr, filters[1] )))) goto fail;
    hr = IPropertySystem_GetPropertyDescriptionByName( parser->propsys, prop_name->pwszVal, &IID_IPropertyDescription, (void **)&prop_desc );
    PropVariantClear( prop_name );
    if (FAILED(hr))
    {
        parser->error = hr == TYPE_E_ELEMENTNOTFOUND ? E_INVALIDARG : hr;
        goto fail;
    }
    expr->len = 1;
    filter = &expr->filters[0];
    if (FAILED(parser->error = IPropertyDescription_GetPropertyKey( prop_desc, (PROPERTYKEY *)&filter->Property.CompKey.Key ))) goto fail;
    if (FAILED(parser->error = IPropertyDescription_GetPropertyType( prop_desc, &type ))) goto fail;
    if (FAILED(parser->error = propval_to_devprop( val, &filter->Property.CompKey.Key, type, &filter->Property ))) goto fail;

    if ((op & DEVPROP_OPERATOR_MASK_EVAL) == DEVPROP_OPERATOR_CONTAINS) FIXME( "Wildcard matching is not supported yet, will compare verbatim.\n" );
    if ((op == DEVPROP_OPERATOR_EQUALS || op == DEVPROP_OPERATOR_NOT_EQUALS) && val->vt == VT_EMPTY)
        filter->Operator = (op == DEVPROP_OPERATOR_EQUALS) ? DEVPROP_OPERATOR_NOT_EXISTS : DEVPROP_OPERATOR_EXISTS;
    else
        filter->Operator = op;

    IPropertyDescription_Release( prop_desc );
    PropVariantClear( val );
    *ret_expr = expr;

    return S_OK;

fail:
    PropVariantClear( val );
    if (prop_desc) IPropertyDescription_Release( prop_desc );
    free( expr );
    return parser->error;
}

static const char *debugstr_DEVPROPKEY( const DEVPROPKEY *key )
{
    if (!key) return "(null)";
    return wine_dbg_sprintf( "{%s,%04lx}", debugstr_guid( &key->fmtid ), key->pid );
}

static const char *debugstr_DEVPROPCOMPKEY( const DEVPROPCOMPKEY *key )
{
    if (!key) return "(null)";
    return wine_dbg_sprintf( "{%s}", debugstr_DEVPROPKEY( &key->Key ) );
}

static const char *debugstr_DEVPROP_FILTER_EXPRESSION( const DEVPROP_FILTER_EXPRESSION *filter )
{
    if (!filter) return "(null)";
    return wine_dbg_sprintf( "{%#x,{%s,%#lx,%lu,%p}}", filter->Operator, debugstr_DEVPROPCOMPKEY( &filter->Property.CompKey ), filter->Property.Type,
                             filter->Property.BufferSize, filter->Property.Buffer );
}

const char *debugstr_expr( const struct aqs_expr *expr )
{
    char buf[200];
    int written = 1;
    ULONG i;

    if (!expr) return "(null)";

    buf[0] = '{';
    for (i = 0; i < expr->len; i++)
    {
        size_t size = sizeof( buf ) - written - 1;
        int len;

        len = snprintf( &buf[written], size, "%s, ", debugstr_DEVPROP_FILTER_EXPRESSION( &expr->filters[i] ) );
        if (len >= size) return wine_dbg_sprintf( "{%lu, %p}", expr->len, expr->filters );
        written += len;
    }
    /* Overwrites the last comma */
    buf[written - 2] = '}';
    buf[written - 1] = '\0';
    return wine_dbg_sprintf( "%s", buf );
}

void free_aqs_expr( struct aqs_expr *expr )
{
    ULONG i;

    if (!expr) return;

    for (i = 0; i < expr->len; i++)
        free( expr->filters[i].Property.Buffer );

    free( expr );
}
