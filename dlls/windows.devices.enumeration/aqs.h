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

#include <devpropdef.h>
#include <devfiltertypes.h>
#include <wine/list.h>
#include <wine/debug.h>

#include "private.h"

struct string
{
    const WCHAR *data;
    int len;
};

struct aqs_parser
{
    const WCHAR *query;
    int idx;
    int len;
    HRESULT error;
    /* If the parsed query is only whitespaces. Needed for CreateWatcher. */
    BOOL all_whitespace;

    IPropertySystem *propsys;
    struct aqs_expr *expr;
};

struct aqs_expr
{
    ULONG len;
    DEVPROP_FILTER_EXPRESSION filters[1];
};

extern HRESULT aqs_parse_query( const WCHAR *str, struct aqs_expr **expr, BOOL *all_whitespace );

extern UINT aqs_lex( void *val, struct aqs_parser *parser );

extern HRESULT get_integer( struct aqs_parser *parser, PROPVARIANT *val );
extern HRESULT get_string( struct aqs_parser *parser, const struct string *str, BOOL id, PROPVARIANT *val );
extern void get_boolean( struct aqs_parser *parser, BOOL b, PROPVARIANT *val );

extern HRESULT join_expr( struct aqs_parser *parser, struct aqs_expr *first, struct aqs_expr *second, struct aqs_expr **expr );
extern HRESULT get_boolean_binary_expr( struct aqs_parser *parser, DEVPROP_OPERATOR op, struct aqs_expr *first, struct aqs_expr *second, struct aqs_expr **expr );
extern HRESULT get_boolean_not_expr( struct aqs_parser *parser, struct aqs_expr *inner, struct aqs_expr **expr );
extern HRESULT get_compare_expr( struct aqs_parser *parser, DEVPROP_OPERATOR op, PROPVARIANT *prop_name, PROPVARIANT *val, struct aqs_expr **expr );

extern void free_aqs_expr( struct aqs_expr *expr );
extern void free_devprop_filters( DEVPROP_FILTER_EXPRESSION *filters, ULONG filters_len );

extern const char *debugstr_expr( const struct aqs_expr *expr );
static inline const char *debugstr_propvar(const PROPVARIANT *v)
{
    if (!v)
        return "(null)";

    switch (v->vt)
    {
        case VT_EMPTY:
            return wine_dbg_sprintf("%p {VT_EMPTY}", v);
        case VT_NULL:
            return wine_dbg_sprintf("%p {VT_NULL}", v);
        case VT_BOOL:
            return wine_dbg_sprintf("%p {VT_BOOL %d}", v, !!v->boolVal);
        case VT_UI4:
            return wine_dbg_sprintf("%p {VT_UI4: %ld}", v, v->ulVal);
        case VT_UI8:
            return wine_dbg_sprintf("%p {VT_UI8: %s}", v, wine_dbgstr_longlong(v->uhVal.QuadPart));
        case VT_I8:
            return wine_dbg_sprintf("%p {VT_I8: %s}", v, wine_dbgstr_longlong(v->hVal.QuadPart));
        case VT_R4:
            return wine_dbg_sprintf("%p {VT_R4: %.8e}", v, v->fltVal);
        case VT_R8:
            return wine_dbg_sprintf("%p {VT_R8: %lf}", v, v->dblVal);
        case VT_CLSID:
            return wine_dbg_sprintf("%p {VT_CLSID: %s}", v, wine_dbgstr_guid(v->puuid));
        case VT_LPWSTR:
            return wine_dbg_sprintf("%p {VT_LPWSTR: %s}", v, wine_dbgstr_w(v->pwszVal));
        case VT_VECTOR | VT_UI1:
            return wine_dbg_sprintf("%p {VT_VECTOR|VT_UI1: %p}", v, v->caub.pElems);
        case VT_UNKNOWN:
            return wine_dbg_sprintf("%p {VT_UNKNOWN: %p}", v, v->punkVal);
        default:
            return wine_dbg_sprintf("%p {vt %#x}", v, v->vt);
    }
}
