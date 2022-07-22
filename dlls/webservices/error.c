/*
 * Copyright 2015, 2016 Hans Leidekker for CodeWeavers
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
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "webservices.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "webservices_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(webservices);

static const struct prop_desc error_props[] =
{
    { sizeof(ULONG), TRUE },    /* WS_ERROR_PROPERTY_STRING_COUNT */
    { sizeof(ULONG), FALSE },   /* WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE */
    { sizeof(LANGID), FALSE }   /* WS_ERROR_PROPERTY_LANGID */
};

struct error
{
    ULONG            magic;
    CRITICAL_SECTION cs;
    WS_HEAP         *heap;
    ULONG            prop_count;
    struct prop      prop[ARRAY_SIZE( error_props )];
    ULONG            strs_count;
    ULONG            strs_size; /* Maximum length of the strs array */
    WS_STRING       *strs;
    WS_FAULT        *fault;
    WS_XML_STRING    fault_action;
};

#define ERROR_MAGIC (('E' << 24) | ('R' << 16) | ('R' << 8) | 'O')

static struct error *alloc_error(void)
{
    static const ULONG count = ARRAY_SIZE( error_props );
    struct error *ret;
    ULONG size = sizeof(*ret) + prop_size( error_props, count );

    if (!(ret = calloc( 1, size ))) return NULL;
    if (WsCreateHeap( 1 << 20, 0, NULL, 0, &ret->heap, NULL ) != S_OK)
    {
        free( ret );
        return NULL;
    }

    ret->magic      = ERROR_MAGIC;
    InitializeCriticalSection( &ret->cs );
    ret->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": error.cs");

    prop_init( error_props, count, ret->prop, &ret[1] );
    ret->prop_count = count;

    ret->strs = NULL;
    ret->strs_count = ret->strs_size = 0;

    return ret;
}

static void free_error( struct error *error )
{
    WsFreeHeap( error->heap );
    error->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection( &error->cs );
    free( error );
}

void free_fault_fields( WS_HEAP *heap, WS_FAULT *fault )
{
    WS_FAULT_CODE *code, *prev_code;
    ULONG i;

    code = fault->code;
    while ( code )
    {
        ws_free( heap, code->value.localName.bytes, code->value.localName.length );
        ws_free( heap, code->value.ns.bytes, code->value.ns.length );
        prev_code = code;
        code = code->subCode;
        ws_free( heap, prev_code, sizeof(*prev_code) );
    }

    for (i = 0; i < fault->reasonCount; i++)
    {
        ws_free( heap, fault->reasons[i].text.chars, fault->reasons[i].text.length * sizeof(WCHAR) );
        ws_free( heap, fault->reasons[i].lang.chars, fault->reasons[i].lang.length * sizeof(WCHAR) );
    }
    ws_free( heap, fault->reasons, fault->reasonCount * sizeof(*fault->reasons) );

    ws_free( heap, fault->actor.chars, fault->actor.length * sizeof(WCHAR) );
    ws_free( heap, fault->node.chars, fault->node.length * sizeof(WCHAR) );
    free_xmlbuf((struct xmlbuf *)fault->detail);
}

static void free_fault( WS_HEAP *heap, WS_FAULT *fault )
{
    if (!fault) return;
    free_fault_fields( heap, fault );
    ws_free( heap, fault, sizeof(*fault) );
}

static BOOL copy_xml_string( WS_HEAP *heap, const WS_XML_STRING *src, WS_XML_STRING *dst )
{
    if (!src->length) return TRUE;
    if (!(dst->bytes = ws_alloc( heap, src->length ))) return FALSE;
    memcpy( dst->bytes, src->bytes, src->length );
    dst->length = src->length;
    return TRUE;
}

static BOOL copy_string( WS_HEAP *heap, const WS_STRING *src, WS_STRING *dst )
{
    ULONG size;
    if (!src->length) return TRUE;
    size = src->length * sizeof(WCHAR);
    if (!(dst->chars = ws_alloc( heap, size ))) return FALSE;
    memcpy( dst->chars, src->chars, size );
    dst->length = src->length;
    return TRUE;
}

/* Grow the strs array to fit an extra element. */
static HRESULT grow_strs_array( struct error *error )
{
    WS_STRING *new_ptr;
    ULONG new_size;

    if (error->strs_count < error->strs_size)
        return S_OK;

    new_size = error->strs_size > 0 ? 2 * error->strs_size : 1;
    if (error->strs_size > 0)
    {
        new_size = 2 * error->strs_size;
        new_ptr = ws_realloc_zero( error->heap, error->strs,
                                   error->strs_size * sizeof(WS_STRING),
                                   new_size * sizeof(WS_STRING) );
    }
    else
    {
        new_size = 1;
        new_ptr = ws_alloc_zero( error->heap, sizeof(WS_STRING) );
    }
    if (!new_ptr)
        return E_OUTOFMEMORY;

    error->strs = new_ptr;
    error->strs_size = new_size;
    return S_OK;
}

static WS_FAULT *dup_fault( WS_HEAP *heap, const WS_FAULT *src )
{
    WS_FAULT *new_fault;
    WS_FAULT_CODE *code, *prev_code, *new_code;
    struct xmlbuf *buf, *new_buf;
    ULONG i;
    BOOL success = FALSE;

    if (!(new_fault = ws_alloc_zero( heap, sizeof(*new_fault) )))
        return NULL;

    prev_code = NULL;
    code = src->code;
    while ( code )
    {
        if (!(new_code = ws_alloc_zero( heap, sizeof(*new_code) )) ||
            !copy_xml_string( heap, &code->value.localName, &new_code->value.localName ) ||
            !copy_xml_string( heap, &code->value.ns, &new_code->value.ns ))
            goto done;

        if (prev_code)
            prev_code->subCode = new_code;
        else
            new_fault->code = new_code;
        prev_code = new_code;
        code = code->subCode;
    }

    if (src->reasonCount > 0)
    {
        if (!(new_fault->reasons = ws_alloc_zero( heap, sizeof(*new_fault->reasons) * src->reasonCount )))
            goto done;
        new_fault->reasonCount = src->reasonCount;
        for (i = 0; i < src->reasonCount; i++)
        {
            if (!copy_string( heap, &src->reasons[i].text, &new_fault->reasons[i].text ) ||
                !copy_string( heap, &src->reasons[i].lang, &new_fault->reasons[i].lang ))
                goto done;
        }
    }

    if (!copy_string( heap, &src->actor, &new_fault->actor ) ||
        !copy_string( heap, &src->node, &new_fault->node ))
        goto done;

    buf = (struct xmlbuf *)src->detail;
    new_buf = NULL;
    if (buf)
    {
        if (!(new_buf = alloc_xmlbuf( heap, buf->bytes.length, buf->encoding,
                                      buf->charset, buf->dict_static, buf->dict )))
            goto done;
        memcpy( new_buf->bytes.bytes, buf->bytes.bytes, buf->bytes.length );
        new_buf->bytes.length = buf->bytes.length;
    }
    new_fault->detail = (WS_XML_BUFFER *)new_buf;

    success = TRUE;
done:
    if (!success)
    {
        free_fault( heap, new_fault );
        return NULL;
    }
    return new_fault;
}

static HRESULT set_fault( struct error *error, const WS_FAULT *value )
{
    static const WCHAR prefix[] = L"The fault reason was: '";
    static const WCHAR postfix[] = L"'.";
    static const ULONG prefix_len = ARRAY_SIZE(prefix) - 1, postfix_len = ARRAY_SIZE(postfix) - 1;
    WS_FAULT *fault;
    WS_STRING *str;
    WCHAR *dst;
    ULONG len;
    HRESULT hr = S_OK;

    if (!(fault = dup_fault( error->heap, value )))
        return E_OUTOFMEMORY;

    /* FIXME: check if reason lang matches error property langid */
    if (fault->reasonCount > 0)
    {
        if ((hr = grow_strs_array( error )) != S_OK) goto done;

        str = &error->strs[error->strs_count];
        len = prefix_len + fault->reasons[0].text.length + postfix_len;
        if (!(str->chars = ws_alloc( error->heap, len * sizeof(WCHAR) )))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        dst = str->chars;
        memcpy( dst, prefix, prefix_len * sizeof(WCHAR) );
        dst += prefix_len;
        memcpy( dst, fault->reasons[0].text.chars, fault->reasons[0].text.length * sizeof(WCHAR) );
        dst += fault->reasons[0].text.length;
        memcpy( dst, postfix, postfix_len * sizeof(WCHAR) );

        str->length = len;
        error->strs_count++;
    }

    free_fault( error->heap, error->fault );
    error->fault = fault;

done:
    if (hr != S_OK)
        free_fault( error->heap, fault );
    return hr;
}

static HRESULT set_action( struct error *error, const WS_XML_STRING *value )
{
    BYTE *buf;

    if (value->length == 0)
    {
        ws_free( error->heap, error->fault_action.bytes, error->fault_action.length );
        memset( &error->fault_action, 0, sizeof(error->fault_action) );
        return S_OK;
    }

    if (!(buf = ws_alloc( error->heap, value->length )))
        return E_OUTOFMEMORY;

    memcpy( buf, value->bytes, value->length );
    ws_free( error->heap, error->fault_action.bytes, error->fault_action.length );
    error->fault_action.bytes = buf;
    error->fault_action.length = value->length;

    return S_OK;
}

/**************************************************************************
 *          WsCreateError		[webservices.@]
 */
HRESULT WINAPI WsCreateError( const WS_ERROR_PROPERTY *properties, ULONG count, WS_ERROR **handle )
{
    struct error *error;
    LANGID langid = GetUserDefaultUILanguage();
    HRESULT hr;
    ULONG i;

    TRACE( "%p %lu %p\n", properties, count, handle );

    if (!handle) return E_INVALIDARG;
    if (!(error = alloc_error())) return E_OUTOFMEMORY;

    prop_set( error->prop, error->prop_count, WS_ERROR_PROPERTY_LANGID, &langid, sizeof(langid) );
    for (i = 0; i < count; i++)
    {
        if (properties[i].id == WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE)
        {
            free_error( error );
            return E_INVALIDARG;
        }
        hr = prop_set( error->prop, error->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK)
        {
            free_error( error );
            return hr;
        }
    }

    TRACE( "created %p\n", error );
    *handle = (WS_ERROR *)error;
    return S_OK;
}

static void reset_error( struct error *error )
{
    ULONG code = 0;

    prop_set( error->prop, error->prop_count, WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE, &code, sizeof(code) );

    error->strs = NULL;
    error->strs_count = error->strs_size = 0;
    error->fault = NULL;
    memset( &error->fault_action, 0, sizeof(error->fault_action) );

    WsResetHeap( error->heap, NULL );
}

/**************************************************************************
 *          WsFreeError		[webservices.@]
 */
void WINAPI WsFreeError( WS_ERROR *handle )
{
    struct error *error = (struct error *)handle;

    TRACE( "%p\n", handle );

    if (!error) return;

    EnterCriticalSection( &error->cs );

    if (error->magic != ERROR_MAGIC)
    {
        LeaveCriticalSection( &error->cs );
        return;
    }

    reset_error( error );
    error->magic = 0;

    LeaveCriticalSection( &error->cs );
    free_error( error );
}

/**************************************************************************
 *          WsResetError	[webservices.@]
 */
HRESULT WINAPI WsResetError( WS_ERROR *handle )
{
    struct error *error = (struct error *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p\n", handle );

    if (!error) return E_INVALIDARG;

    EnterCriticalSection( &error->cs );

    if (error->magic != ERROR_MAGIC)
    {
        LeaveCriticalSection( &error->cs );
        return E_INVALIDARG;
    }

    reset_error( error );

    LeaveCriticalSection( &error->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsGetErrorProperty		[webservices.@]
 */
HRESULT WINAPI WsGetErrorProperty( WS_ERROR *handle, WS_ERROR_PROPERTY_ID id, void *buf,
                                   ULONG size )
{
    struct error *error = (struct error *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %u %p %lu\n", handle, id, buf, size );

    if (!error || !buf) return E_INVALIDARG;

    EnterCriticalSection( &error->cs );

    if (error->magic != ERROR_MAGIC)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (id == WS_ERROR_PROPERTY_STRING_COUNT)
    {
        if (size == sizeof(ULONG))
            *(ULONG *)buf = error->strs_count;
        else
            hr = E_INVALIDARG;
    }
    else
        hr = prop_get( error->prop, error->prop_count, id, buf, size );

done:
    LeaveCriticalSection( &error->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsGetErrorString		[webservices.@]
 */
HRESULT WINAPI WsGetErrorString( WS_ERROR *handle, ULONG index, WS_STRING *str )
{
    struct error *error = (struct error *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %lu %p\n", handle, index, str );

    if (!error || !str) return E_INVALIDARG;

    EnterCriticalSection( &error->cs );

    if (error->magic != ERROR_MAGIC)
    {
        hr = E_INVALIDARG;
        goto done;
    }
    if (index >= error->strs_count)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    /* The strings are indexed from most recently added to least recently added. */
    memcpy( str, &error->strs[error->strs_count - 1 - index], sizeof(WS_STRING) );

done:
    LeaveCriticalSection( &error->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsSetErrorProperty		[webservices.@]
 */
HRESULT WINAPI WsSetErrorProperty( WS_ERROR *handle, WS_ERROR_PROPERTY_ID id, const void *value,
                                   ULONG size )
{
    struct error *error = (struct error *)handle;
    HRESULT hr;

    TRACE( "%p %u %p %lu\n", handle, id, value, size );

    if (!error) return E_INVALIDARG;

    EnterCriticalSection( &error->cs );

    if (error->magic != ERROR_MAGIC)
    {
        LeaveCriticalSection( &error->cs );
        return E_INVALIDARG;
    }

    if (id == WS_ERROR_PROPERTY_LANGID) hr = WS_E_INVALID_OPERATION;
    else hr = prop_set( error->prop, error->prop_count, id, value, size );

    LeaveCriticalSection( &error->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsAddErrorString		[webservices.@]
 */
HRESULT WINAPI WsAddErrorString( WS_ERROR *handle, const WS_STRING *str )
{
    struct error *error = (struct error *)handle;
    WCHAR *chars;
    HRESULT hr;

    TRACE( "%p %p\n", handle, str );

    if (!error || !str) return E_INVALIDARG;

    EnterCriticalSection( &error->cs );

    if (error->magic != ERROR_MAGIC)
    {
        hr = E_INVALIDARG;
        goto done;
    }
    if (!(chars = ws_alloc( error->heap, str->length * sizeof(*chars) )))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    if ( (hr = grow_strs_array( error )) != S_OK )
    {
        ws_free( error->heap, chars, str->length * sizeof(*chars) );
        goto done;
    }

    memcpy( chars, str->chars, str->length * sizeof(*chars) );

    error->strs[error->strs_count].chars = chars;
    error->strs[error->strs_count].length = str->length;
    error->strs_count++;

done:
    LeaveCriticalSection( &error->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsGetFaultErrorDetail		[webservices.@]
 */
HRESULT WINAPI WsGetFaultErrorDetail( WS_ERROR *handle, const WS_FAULT_DETAIL_DESCRIPTION *desc,
                                      WS_READ_OPTION option, WS_HEAP *heap, void *value, ULONG size )
{
    static const WS_XML_STRING detail = {6, (BYTE *)"detail"};
    struct error *error = (struct error *)handle;
    WS_XML_READER *reader = NULL;
    const WS_XML_NODE *node;
    const WS_XML_ELEMENT_NODE *elem;
    BOOL nil = FALSE;
    HRESULT hr = S_OK;

    TRACE( "%p %p %u %p %p %lu\n", handle, desc, option, heap, value, size );

    if (!error || !desc || !value) return E_INVALIDARG;
    if ((option == WS_READ_REQUIRED_POINTER ||
         option == WS_READ_OPTIONAL_POINTER ||
         option == WS_READ_NILLABLE_POINTER) && size != sizeof(void *))
        return E_INVALIDARG;

    EnterCriticalSection( &error->cs );

    if (error->magic != ERROR_MAGIC)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (!error->fault || !error->fault->detail)
    {
        nil = TRUE;
        goto done;
    }
    if ((hr = WsCreateReader( NULL, 0, &reader, NULL )) != S_OK) goto done;
    if ((hr = WsSetInputToBuffer( reader, error->fault->detail, NULL, 0, NULL )) != S_OK) goto done;

    if ((hr = WsReadNode( reader, NULL )) != S_OK) goto done;
    if ((hr = WsGetReaderNode( reader, &node, NULL )) != S_OK) goto done;
    elem = (const WS_XML_ELEMENT_NODE *)node;
    if (!(node->nodeType == WS_XML_NODE_TYPE_ELEMENT &&
          WsXmlStringEquals( elem->localName, &detail, NULL ) == S_OK))
    {
        hr = WS_E_INVALID_FORMAT;
        goto done;
    }

    if (desc->action && error->fault_action.length &&
        WsXmlStringEquals( desc->action, &error->fault_action, NULL ) != S_OK)
    {
        nil = TRUE;
        goto done;
    }

    if ((hr = WsReadNode( reader, NULL )) != S_OK) goto done;
    if ((hr = WsReadElement( reader, desc->detailElementDescription,
                             option, heap, value, size, handle )) != S_OK)
        goto done;

done:
    LeaveCriticalSection( &error->cs );
    WsFreeReader( reader );

    if ((hr != S_OK || nil) && (option == WS_READ_OPTIONAL_POINTER || option == WS_READ_NILLABLE_POINTER))
        *(void **)value = NULL;
    if (nil && !(option == WS_READ_OPTIONAL_POINTER || option == WS_READ_NILLABLE_POINTER))
        hr = WS_E_INVALID_FORMAT;

    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsGetFaultErrorProperty		[webservices.@]
 */
HRESULT WINAPI WsGetFaultErrorProperty( WS_ERROR *handle, WS_FAULT_ERROR_PROPERTY_ID id,
                                        void *buf, ULONG size )
{
    struct error *error = (struct error *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %u %p %lu\n", handle, id, buf, size );

    if (!error || !buf) return E_INVALIDARG;
    if (id > WS_FAULT_ERROR_PROPERTY_HEADER) return E_INVALIDARG;
    else if (id == WS_FAULT_ERROR_PROPERTY_HEADER)
    {
        FIXME( "WS_FAULT_ERROR_PROPERTY_HEADER not supported\n" );
        return E_NOTIMPL;
    }

    EnterCriticalSection( &error->cs );

    if (error->magic != ERROR_MAGIC)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (id == WS_FAULT_ERROR_PROPERTY_FAULT && size == sizeof(WS_FAULT *))
        *(WS_FAULT **)buf = error->fault;
    else if (id == WS_FAULT_ERROR_PROPERTY_ACTION && size == sizeof(WS_XML_STRING))
        memcpy( buf, &error->fault_action, sizeof(WS_XML_STRING) );
    else
        hr = E_INVALIDARG;

done:
    LeaveCriticalSection( &error->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsSetFaultErrorProperty		[webservices.@]
 */
HRESULT WINAPI WsSetFaultErrorProperty( WS_ERROR *handle, WS_FAULT_ERROR_PROPERTY_ID id,
                                        const void *value, ULONG size )
{
    struct error *error = (struct error *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %u %p %lu\n", handle, id, value, size );

    if (!error || !value) return E_INVALIDARG;
    if (id > WS_FAULT_ERROR_PROPERTY_HEADER) return E_INVALIDARG;
    else if (id == WS_FAULT_ERROR_PROPERTY_HEADER)
    {
        FIXME( "WS_FAULT_ERROR_PROPERTY_HEADER not supported\n" );
        return E_NOTIMPL;
    }

    EnterCriticalSection( &error->cs );

    if (error->magic != ERROR_MAGIC)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (id == WS_FAULT_ERROR_PROPERTY_FAULT && size == sizeof(WS_FAULT))
        hr = set_fault( error, value );
    else if (id == WS_FAULT_ERROR_PROPERTY_ACTION && size == sizeof(WS_XML_STRING))
        hr = set_action( error, value );
    else
        hr = E_INVALIDARG;

done:
    LeaveCriticalSection( &error->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}
