/*
 * Copyright 2012 Hans Leidekker for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "wbemcli.h"

#include "wine/debug.h"
#include "wbemprox_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

static HRESULT get_column_index( const struct table *table, const WCHAR *name, UINT *column )
{
    UINT i;
    for (i = 0; i < table->num_cols; i++)
    {
        if (!strcmpiW( table->columns[i].name, name ))
        {
            *column = i;
            return S_OK;
        }
    }
    return WBEM_E_INVALID_QUERY;
}

static UINT get_column_size( const struct table *table, UINT column )
{
    if (table->columns[column].type & CIM_FLAG_ARRAY) return sizeof(void *);

    switch (table->columns[column].type & COL_TYPE_MASK)
    {
    case CIM_SINT16:
    case CIM_UINT16:
        return sizeof(INT16);
    case CIM_SINT32:
    case CIM_UINT32:
        return sizeof(INT32);
    case CIM_DATETIME:
    case CIM_STRING:
        return sizeof(WCHAR *);
    default:
        ERR("unkown column type %u\n", table->columns[column].type & COL_TYPE_MASK);
        break;
    }
    return sizeof(INT32);
}

static UINT get_column_offset( const struct table *table, UINT column )
{
    UINT i, offset = 0;
    for (i = 0; i < column; i++) offset += get_column_size( table, i );
    return offset;
}

static UINT get_row_size( const struct table *table )
{
    return get_column_offset( table, table->num_cols - 1 ) + get_column_size( table, table->num_cols - 1 );
}

static HRESULT get_value( const struct table *table, UINT row, UINT column, INT_PTR *val )
{
    UINT col_offset, row_size;
    const BYTE *ptr;

    col_offset = get_column_offset( table, column );
    row_size = get_row_size( table );
    ptr = table->data + row * row_size + col_offset;

    if (table->columns[column].type & CIM_FLAG_ARRAY)
    {
        *val = (INT_PTR)*(const void **)ptr;
        return S_OK;
    }
    switch (table->columns[column].type & COL_TYPE_MASK)
    {
    case CIM_DATETIME:
    case CIM_STRING:
        *val = (INT_PTR)*(const WCHAR **)ptr;
        break;
    case CIM_SINT16:
        *val = *(const INT16 *)ptr;
        break;
    case CIM_UINT16:
        *val = *(const UINT16 *)ptr;
        break;
    case CIM_SINT32:
        *val = *(const INT32 *)ptr;
        break;
    case CIM_UINT32:
        *val = *(const UINT32 *)ptr;
        break;
    default:
        ERR("invalid column type %u\n", table->columns[column].type & COL_TYPE_MASK);
        *val = 0;
        break;
    }
    return S_OK;
}

static BSTR get_value_bstr( const struct table *table, UINT row, UINT column )
{
    static const WCHAR fmt_signedW[] = {'%','d',0};
    static const WCHAR fmt_unsignedW[] = {'%','u',0};
    static const WCHAR fmt_strW[] = {'\"','%','s','\"',0};
    INT_PTR val;
    BSTR ret;
    WCHAR number[12];
    UINT len;

    if (table->columns[column].type & CIM_FLAG_ARRAY)
    {
        FIXME("array to string conversion not handled\n");
        return NULL;
    }
    if (get_value( table, row, column, &val ) != S_OK) return NULL;

    switch (table->columns[column].type & COL_TYPE_MASK)
    {
    case CIM_DATETIME:
    case CIM_STRING:
        len = strlenW( (const WCHAR *)val ) + 2;
        if (!(ret = SysAllocStringLen( NULL, len ))) return NULL;
        sprintfW( ret, fmt_strW, (const WCHAR *)val );
        return ret;

    case CIM_SINT16:
    case CIM_SINT32:
        sprintfW( number, fmt_signedW, val );
        return SysAllocString( number );

    case CIM_UINT16:
    case CIM_UINT32:
        sprintfW( number, fmt_unsignedW, val );
        return SysAllocString( number );

    default:
        FIXME("unhandled column type %u\n", table->columns[column].type & COL_TYPE_MASK);
        break;
    }
    return NULL;
}

HRESULT create_view( const struct property *proplist, const WCHAR *class,
                     const struct expr *cond, struct view **ret )
{
    struct view *view = heap_alloc( sizeof(struct view) );

    if (!view) return E_OUTOFMEMORY;
    view->proplist = proplist;
    view->table    = get_table( class );
    view->cond     = cond;
    view->result   = NULL;
    view->count    = 0;
    view->index    = 0;
    *ret = view;
    return S_OK;
}

static void clear_table( struct table *table )
{
    UINT i, j, type;

    if (!table->fill || !table->data) return;

    for (i = 0; i < table->num_rows; i++)
    {
        for (j = 0; j < table->num_cols; j++)
        {
            if (!(table->columns[j].type & COL_FLAG_DYNAMIC)) continue;

            type = table->columns[j].type & COL_TYPE_MASK;
            if (type == CIM_STRING || type == CIM_DATETIME || (type & CIM_FLAG_ARRAY))
            {
                void *ptr;
                if (get_value( table, i, j, (INT_PTR *)&ptr ) == S_OK) heap_free( ptr );
            }
        }
    }
    heap_free( table->data );
    table->data = NULL;
}

void destroy_view( struct view *view )
{
    if (view->table) clear_table( view->table );
    heap_free( view->result );
    heap_free( view );
}

static BOOL eval_like( INT_PTR lval, INT_PTR rval )
{
    const WCHAR *p = (const WCHAR *)lval, *q = (const WCHAR *)rval;

    while (*p && *q)
    {
        if (*q == '%')
        {
            while (*q == '%') q++;
            if (!*q) return TRUE;
            while (*p && toupperW( p[1] ) != toupperW( q[1] )) p++;
            if (!*p) return TRUE;
        }
        if (toupperW( *p++ ) != toupperW( *q++ )) return FALSE;
    }
    return TRUE;
}

static HRESULT eval_cond( const struct table *, UINT, const struct expr *, INT_PTR * );

static BOOL eval_binary( const struct table *table, UINT row, const struct complex_expr *expr,
                         INT_PTR *val )
{
    HRESULT lret, rret;
    INT_PTR lval, rval;

    lret = eval_cond( table, row, expr->left, &lval );
    rret = eval_cond( table, row, expr->right, &rval );
    if (lret != S_OK || rret != S_OK) return WBEM_E_INVALID_QUERY;

    switch (expr->op)
    {
    case OP_EQ:
        *val = (lval == rval);
        break;
    case OP_AND:
        *val = (lval && rval);
        break;
    case OP_OR:
        *val = (lval || rval);
        break;
    case OP_GT:
        *val = (lval > rval);
        break;
    case OP_LT:
        *val = (lval < rval);
        break;
    case OP_LE:
        *val = (lval <= rval);
        break;
    case OP_GE:
        *val = (lval >= rval);
        break;
    case OP_NE:
        *val = (lval != rval);
        break;
    case OP_LIKE:
        *val = eval_like( lval, rval );
        break;
    default:
        ERR("unknown operator %u\n", expr->op);
        return WBEM_E_INVALID_QUERY;
    }
    return S_OK;
}

static HRESULT eval_unary( const struct table *table, UINT row, const struct complex_expr *expr,
                           INT_PTR *val )

{
    HRESULT hr;
    UINT column;
    INT_PTR lval;

    hr = get_column_index( table, expr->left->u.propval->name, &column );
    if (hr != S_OK)
        return hr;

    hr = get_value( table, row, column, &lval );
    if (hr != S_OK)
        return hr;

    switch (expr->op)
    {
    case OP_ISNULL:
        *val = !lval;
        break;
    case OP_NOTNULL:
        *val = lval;
        break;
    default:
        ERR("unknown operator %u\n", expr->op);
        return WBEM_E_INVALID_QUERY;
    }
    return S_OK;
}

static HRESULT eval_propval( const struct table *table, UINT row, const struct property *propval,
                             INT_PTR *val )

{
    HRESULT hr;
    UINT column;

    hr = get_column_index( table, propval->name, &column );
    if (hr != S_OK)
        return hr;

    return get_value( table, row, column, val );
}

static HRESULT eval_cond( const struct table *table, UINT row, const struct expr *cond,
                          INT_PTR *val )
{
    if (!cond)
    {
        *val = 1;
        return S_OK;
    }
    switch (cond->type)
    {
    case EXPR_COMPLEX:
        return eval_binary( table, row, &cond->u.expr, val );
    case EXPR_UNARY:
        return eval_unary( table, row, &cond->u.expr, val );
    case EXPR_PROPVAL:
        return eval_propval( table, row, cond->u.propval, val );
    case EXPR_SVAL:
        *val = (INT_PTR)cond->u.sval;
        return S_OK;
    case EXPR_IVAL:
    case EXPR_BVAL:
        *val = cond->u.ival;
        return S_OK;
    default:
        ERR("invalid expression type\n");
        break;
    }
    return WBEM_E_INVALID_QUERY;
}

static HRESULT execute_view( struct view *view )
{
    UINT i, j = 0, len;

    if (!view->table || !view->table->num_rows) return S_OK;

    len = min( view->table->num_rows, 16 );
    if (!(view->result = heap_alloc( len * sizeof(UINT) ))) return E_OUTOFMEMORY;

    for (i = 0; i < view->table->num_rows; i++)
    {
        HRESULT hr;
        INT_PTR val = 0;

        if (j >= len)
        {
            UINT *tmp;
            len *= 2;
            if (!(tmp = heap_realloc( view->result, len * sizeof(UINT) ))) return E_OUTOFMEMORY;
            view->result = tmp;
        }
        if ((hr = eval_cond( view->table, i, view->cond, &val )) != S_OK) return hr;
        if (val) view->result[j++] = i;
    }
    view->count = j;
    return S_OK;
}

static struct query *alloc_query(void)
{
    struct query *query;

    if (!(query = heap_alloc( sizeof(*query) ))) return NULL;
    list_init( &query->mem );
    return query;
}

void free_query( struct query *query )
{
    struct list *mem, *next;

    destroy_view( query->view );
    LIST_FOR_EACH_SAFE( mem, next, &query->mem )
    {
        heap_free( mem );
    }
    heap_free( query );
}

HRESULT exec_query( const WCHAR *str, IEnumWbemClassObject **result )
{
    HRESULT hr;
    struct query *query;

    *result = NULL;
    if (!(query = alloc_query())) return E_OUTOFMEMORY;
    hr = parse_query( str, &query->view, &query->mem );
    if (hr != S_OK) goto done;
    hr = execute_view( query->view );
    if (hr != S_OK) goto done;
    hr = EnumWbemClassObject_create( NULL, query, (void **)result );

done:
    if (hr != S_OK) free_query( query );
    return hr;
}

static BOOL is_selected_prop( const struct view *view, const WCHAR *name )
{
    const struct property *prop = view->proplist;

    if (!prop) return TRUE;
    while (prop)
    {
        if (!strcmpiW( prop->name, name )) return TRUE;
        prop = prop->next;
    }
    return FALSE;
}

static BOOL is_system_prop( const WCHAR *name )
{
    return (name[0] == '_' && name[1] == '_');
}

static BSTR build_servername( const struct view *view )
{
    WCHAR server[MAX_COMPUTERNAME_LENGTH + 1], *p;
    DWORD len = sizeof(server)/sizeof(server[0]);

    if (view->proplist) return NULL;

    if (!(GetComputerNameW( server, &len ))) return NULL;
    for (p = server; *p; p++) *p = toupperW( *p );
    return SysAllocString( server );
}

static BSTR build_classname( const struct view *view )
{
    return SysAllocString( view->table->name );
}

static BSTR build_namespace( const struct view *view )
{
    static const WCHAR cimv2W[] = {'R','O','O','T','\\','C','I','M','V','2',0};

    if (view->proplist) return NULL;
    return SysAllocString( cimv2W );
}

static BSTR build_proplist( const struct view *view, UINT index, UINT count, UINT *len )
{
    static const WCHAR fmtW[] = {'%','s','=','%','s',0};
    UINT i, j, offset, row = view->result[index];
    BSTR *values, ret = NULL;

    if (!(values = heap_alloc( count * sizeof(BSTR) ))) return NULL;

    *len = j = 0;
    for (i = 0; i < view->table->num_cols; i++)
    {
        if (view->table->columns[i].type & COL_FLAG_KEY)
        {
            const WCHAR *name = view->table->columns[i].name;

            values[j] = get_value_bstr( view->table, row, i );
            *len += strlenW( fmtW ) + strlenW( name ) + strlenW( values[j] );
            j++;
        }
    }
    if ((ret = SysAllocStringLen( NULL, *len )))
    {
        offset = j = 0;
        for (i = 0; i < view->table->num_cols; i++)
        {
            if (view->table->columns[i].type & COL_FLAG_KEY)
            {
                const WCHAR *name = view->table->columns[i].name;

                offset += sprintfW( ret + offset, fmtW, name, values[j] );
                if (j < count - 1) ret[offset++] = ',';
                j++;
            }
        }
    }
    for (i = 0; i < count; i++) SysFreeString( values[i] );
    heap_free( values );
    return ret;
}

static UINT count_key_columns( const struct view *view )
{
    UINT i, num_keys = 0;

    for (i = 0; i < view->table->num_cols; i++)
    {
        if (view->table->columns[i].type & COL_FLAG_KEY) num_keys++;
    }
    return num_keys;
}

static BSTR build_relpath( const struct view *view, UINT index, const WCHAR *name )
{
    static const WCHAR fmtW[] = {'%','s','.','%','s',0};
    BSTR class, proplist, ret = NULL;
    UINT num_keys, len;

    if (view->proplist) return NULL;

    if (!(class = build_classname( view ))) return NULL;
    if (!(num_keys = count_key_columns( view ))) return class;
    if (!(proplist = build_proplist( view, index, num_keys, &len ))) goto done;

    len += strlenW( fmtW ) + SysStringLen( class );
    if (!(ret = SysAllocStringLen( NULL, len ))) goto done;
    sprintfW( ret, fmtW, class, proplist );

done:
    SysFreeString( class );
    SysFreeString( proplist );
    return ret;
}

static BSTR build_path( const struct view *view, UINT index, const WCHAR *name )
{
    static const WCHAR fmtW[] = {'\\','\\','%','s','\\','%','s',':','%','s',0};
    BSTR server, namespace = NULL, relpath = NULL, ret = NULL;
    UINT len;

    if (view->proplist) return NULL;

    if (!(server = build_servername( view ))) return NULL;
    if (!(namespace = build_namespace( view ))) goto done;
    if (!(relpath = build_relpath( view, index, name ))) goto done;

    len = strlenW( fmtW ) + SysStringLen( server ) + SysStringLen( namespace ) + SysStringLen( relpath );
    if (!(ret = SysAllocStringLen( NULL, len ))) goto done;
    sprintfW( ret, fmtW, server, namespace, relpath );

done:
    SysFreeString( server );
    SysFreeString( namespace );
    SysFreeString( relpath );
    return ret;
}

static UINT count_selected_props( const struct view *view )
{
    const struct property *prop = view->proplist;
    UINT count;

    if (!prop) return view->table->num_cols;

    count = 1;
    while ((prop = prop->next)) count++;
    return count;
}

static HRESULT get_system_propval( const struct view *view, UINT index, const WCHAR *name,
                                   VARIANT *ret, CIMTYPE *type )
{
    static const WCHAR classW[] = {'_','_','C','L','A','S','S',0};
    static const WCHAR genusW[] = {'_','_','G','E','N','U','S',0};
    static const WCHAR pathW[] = {'_','_','P','A','T','H',0};
    static const WCHAR namespaceW[] = {'_','_','N','A','M','E','S','P','A','C','E',0};
    static const WCHAR propcountW[] = {'_','_','P','R','O','P','E','R','T','Y','_','C','O','U','N','T',0};
    static const WCHAR relpathW[] = {'_','_','R','E','L','P','A','T','H',0};
    static const WCHAR serverW[] = {'_','_','S','E','R','V','E','R',0};

    if (!strcmpiW( name, classW ))
    {
        V_VT( ret ) = VT_BSTR;
        V_BSTR( ret ) = build_classname( view );
        if (type) *type = CIM_STRING;
        return S_OK;
    }
    if (!strcmpiW( name, genusW ))
    {
        V_VT( ret ) = VT_INT;
        V_INT( ret ) = WBEM_GENUS_INSTANCE; /* FIXME */
        if (type) *type = CIM_SINT32;
        return S_OK;
    }
    else if (!strcmpiW( name, namespaceW ))
    {
        V_VT( ret ) = VT_BSTR;
        V_BSTR( ret ) = build_namespace( view );
        if (type) *type = CIM_STRING;
        return S_OK;
    }
    else if (!strcmpiW( name, pathW ))
    {
        V_VT( ret ) = VT_BSTR;
        V_BSTR( ret ) = build_path( view, index, name );
        if (type) *type = CIM_STRING;
        return S_OK;
    }
    if (!strcmpiW( name, propcountW ))
    {
        V_VT( ret ) = VT_INT;
        V_INT( ret ) = count_selected_props( view );
        if (type) *type = CIM_SINT32;
        return S_OK;
    }
    else if (!strcmpiW( name, relpathW ))
    {
        V_VT( ret ) = VT_BSTR;
        V_BSTR( ret ) = build_relpath( view, index, name );
        if (type) *type = CIM_STRING;
        return S_OK;
    }
    else if (!strcmpiW( name, serverW ))
    {
        V_VT( ret ) = VT_BSTR;
        V_BSTR( ret ) = build_servername( view );
        if (type) *type = CIM_STRING;
        return S_OK;
    }
    FIXME("system property %s not implemented\n", debugstr_w(name));
    return WBEM_E_NOT_FOUND;
}

HRESULT get_propval( const struct view *view, UINT index, const WCHAR *name, VARIANT *ret, CIMTYPE *type )
{
    HRESULT hr;
    UINT column, row = view->result[index];
    INT_PTR val;

    if (is_system_prop( name )) return get_system_propval( view, index, name, ret, type );
    if (!is_selected_prop( view, name )) return WBEM_E_NOT_FOUND;

    hr = get_column_index( view->table, name, &column );
    if (hr != S_OK) return WBEM_E_NOT_FOUND;

    hr = get_value( view->table, row, column, &val );
    if (hr != S_OK) return hr;

    switch (view->table->columns[column].type & COL_TYPE_MASK)
    {
    case CIM_STRING:
    case CIM_DATETIME:
        V_VT( ret ) = VT_BSTR;
        V_BSTR( ret ) = SysAllocString( (const WCHAR *)val );
        break;
    case CIM_SINT16:
        V_VT( ret ) = VT_I2;
        V_I2( ret ) = val;
        break;
    case CIM_UINT16:
        V_VT( ret ) = VT_UI2;
        V_UI2( ret ) = val;
        break;
    case CIM_SINT32:
        V_VT( ret ) = VT_I4;
        V_I4( ret ) = val;
        break;
    case CIM_UINT32:
        V_VT( ret ) = VT_UI4;
        V_UI4( ret ) = val;
        break;
    default:
        ERR("unhandled column type %u\n", view->table->columns[column].type);
        return WBEM_E_FAILED;
    }
    if (type) *type = view->table->columns[column].type & COL_TYPE_MASK;
    return S_OK;
}

HRESULT get_properties( const struct view *view, SAFEARRAY **props )
{
    SAFEARRAY *sa;
    BSTR str;
    LONG i;

    if (!(sa = SafeArrayCreateVector( VT_BSTR, 0, view->table->num_cols ))) return E_OUTOFMEMORY;

    for (i = 0; i < view->table->num_cols; i++)
    {
        str = SysAllocString( view->table->columns[i].name );
        if (!str || SafeArrayPutElement( sa, &i, str ) != S_OK)
        {
            SysFreeString( str );
            SafeArrayDestroy( sa );
            return E_OUTOFMEMORY;
        }
    }
    *props = sa;
    return S_OK;
}
