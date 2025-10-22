/*
 * Copyright (C) 2023 Mohamad Al-Jaf
 * Copyright (C) 2025 Vibhav Pant
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

#include "cfgmgr32_private.h"
#include "devpkey.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

static int devproperty_compare( const void *p1, const void *p2 )
{
    const DEVPROPCOMPKEY *key1 = &((DEVPROPERTY *)p1)->CompKey;
    const DEVPROPCOMPKEY *key2 = &((DEVPROPERTY *)p2)->CompKey;
    int cmp = memcmp( key1, key2, offsetof( DEVPROPCOMPKEY, LocaleName ));

    if (cmp)
        return cmp;
    if (key1->LocaleName == key2->LocaleName)
        return 0;
    if (!key1->LocaleName)
        return -1;
    if (!key2->LocaleName)
        return 1;
    return wcsicmp( key1->LocaleName, key2->LocaleName );
}

static const char *debugstr_DEVPROP_OPERATOR( DEVPROP_OPERATOR op )
{
    DWORD compare = op & DEVPROP_OPERATOR_MASK_EVAL;
    DWORD list = op & DEVPROP_OPERATOR_MASK_LIST;
    DWORD modifier = op & DEVPROP_OPERATOR_MASK_MODIFIER;
    DWORD logical = op & DEVPROP_OPERATOR_MASK_LOGICAL;
    DWORD array = op & DEVPROP_OPERATOR_MASK_ARRAY;

    return wine_dbg_sprintf( "(%#lx|%#lx|%#lx|%#lx|%#lx)", list, array, modifier, compare, logical );
}


static const char *debugstr_DEVPROP_FILTER_EXPRESSION( const DEVPROP_FILTER_EXPRESSION *filter )
{
    if (!filter) return "(null)";
    return wine_dbg_sprintf( "{%s, {%s, %#lx, %lu, %p}}", debugstr_DEVPROP_OPERATOR( filter->Operator ),
                             debugstr_DEVPROPCOMPKEY( &filter->Property.CompKey ), filter->Property.Type,
                             filter->Property.BufferSize, filter->Property.Buffer );
}

/* Evaluate a filter expression containing comparison operator. */
static BOOL devprop_filter_eval_compare( const DEVPROPERTY *props, UINT props_len, const DEVPROP_FILTER_EXPRESSION *filter )
{
    const DEVPROPERTY *cmp_prop = &filter->Property;
    DEVPROP_OPERATOR op = filter->Operator;
    const DEVPROPERTY *prop = NULL;
    BOOL ret = FALSE;

    TRACE( "(%p, %u, %s)\n", props, props_len, debugstr_DEVPROP_FILTER_EXPRESSION( filter ) );

    switch (filter->Operator & DEVPROP_OPERATOR_MASK_EVAL)
    {
    case DEVPROP_OPERATOR_EXISTS:
        prop = bsearch( &filter->Property.CompKey, props, props_len, sizeof(*props), devproperty_compare );
        ret = prop && prop->Type != DEVPROP_TYPE_EMPTY;
        break;
    case DEVPROP_OPERATOR_EQUALS:
    case DEVPROP_OPERATOR_LESS_THAN:
    case DEVPROP_OPERATOR_GREATER_THAN:
    case DEVPROP_OPERATOR_LESS_THAN_EQUALS:
    case DEVPROP_OPERATOR_GREATER_THAN_EQUALS:
    {
        int cmp = 0;

        prop = bsearch( &filter->Property.CompKey, props, props_len, sizeof(*props), devproperty_compare );
        if (prop && cmp_prop->Type == prop->Type && cmp_prop->BufferSize == prop->BufferSize)
        {
            switch (prop->Type)
            {
            case DEVPROP_TYPE_STRING:
                cmp = op & DEVPROP_OPERATOR_MODIFIER_IGNORE_CASE ? wcsicmp( prop->Buffer, cmp_prop->Buffer )
                                                                 : wcscmp( prop->Buffer, cmp_prop->Buffer );
                TRACE( "%s vs %s -> %u\n", debugstr_w(prop->Buffer), debugstr_w(cmp_prop->Buffer), !!cmp );
                break;
            case DEVPROP_TYPE_GUID:
                /* Any other comparison operator other than DEVPROP_OPERATOR_EQUALS with GUIDs evaluates to false. */
                if (!(op & DEVPROP_OPERATOR_EQUALS)) break;
            default:
                cmp = memcmp( prop->Buffer, cmp_prop->Buffer, prop->BufferSize );
                break;
            }
            if (op & DEVPROP_OPERATOR_EQUALS)
                ret = !cmp;
            else
                ret = (op & DEVPROP_OPERATOR_LESS_THAN) ? cmp < 0 : cmp > 0;
        }
        break;
    }
    case DEVPROP_OPERATOR_BITWISE_AND:
    case DEVPROP_OPERATOR_BITWISE_OR:
    case DEVPROP_OPERATOR_BEGINS_WITH:
    case DEVPROP_OPERATOR_ENDS_WITH:
    case DEVPROP_OPERATOR_CONTAINS:
    default:
        FIXME( "Unsupported operator: %s", debugstr_DEVPROP_OPERATOR( filter->Operator & DEVPROP_OPERATOR_MASK_EVAL ) );
        return FALSE;
    }

    if (op & DEVPROP_OPERATOR_MODIFIER_NOT) ret = !ret;
    TRACE( "-> %u\n", ret );
    return ret;
}

static const DEVPROP_FILTER_EXPRESSION *find_closing_filter( const DEVPROP_FILTER_EXPRESSION *filter, const DEVPROP_FILTER_EXPRESSION *end )
{
    DWORD open = filter->Operator & DEVPROP_OPERATOR_MASK_LOGICAL, close = open + (DEVPROP_OPERATOR_AND_CLOSE - DEVPROP_OPERATOR_AND_OPEN), depth = 0;

    for (const DEVPROP_FILTER_EXPRESSION *closing = filter + 1; closing < end; closing++)
    {
        DWORD logical = closing->Operator & DEVPROP_OPERATOR_MASK_LOGICAL;
        if (logical == close && !depth--) return closing;
        if (logical == open) depth++;
    }

    return NULL;
}

/* Return TRUE if the specified filter expressions match the object, FALSE if it doesn't. */
static BOOL devprop_filter_matches_properties( const DEVPROPERTY *props, UINT props_len, DEVPROP_OPERATOR op_outer_logical,
                                               const DEVPROP_FILTER_EXPRESSION *filters, const DEVPROP_FILTER_EXPRESSION *end )
{
    BOOL ret = TRUE;

    TRACE( "(%p, %u, %#x, %p, %p)\n", props, props_len, op_outer_logical, filters, end );

    for (const DEVPROP_FILTER_EXPRESSION *filter = filters; filter < end; filter++)
    {
        DEVPROP_OPERATOR op = filter->Operator;

        if (op == DEVPROP_OPERATOR_NONE) ret = FALSE;
        else if (op & (DEVPROP_OPERATOR_MASK_LIST | DEVPROP_OPERATOR_MASK_ARRAY))
        {
            FIXME( "Unsupported list/array operator: %s\n", debugstr_DEVPROP_OPERATOR( op ) );
            ret = FALSE;
        }
        else if (op & DEVPROP_OPERATOR_MASK_LOGICAL)
        {
            const DEVPROP_FILTER_EXPRESSION *closing = find_closing_filter( filter, end );
            ret = devprop_filter_matches_properties( props, props_len, op & DEVPROP_OPERATOR_MASK_LOGICAL, filter + 1, closing );
            filter = closing;
        }
        else if (op & DEVPROP_OPERATOR_MASK_EVAL)
        {
            ret = devprop_filter_eval_compare( props, props_len, filter );
        }

        /* See if we can short-circuit. */
        switch (op_outer_logical)
        {
        /* {NOT_OPEN, ..., NOT_CLOSE} is the same as {NOT_OPEN, AND_OPEN, ..., AND_CLOSE, NOT_CLOSE}, so we can
         * short circuit here as well. */
        case DEVPROP_OPERATOR_NOT_OPEN:
        case DEVPROP_OPERATOR_AND_OPEN:
            if (!ret) goto done;
            break;
        case DEVPROP_OPERATOR_OR_OPEN:
            if (ret) goto done;
            break;
        default:
            assert( 0 );
            break;
        }
    }

done:
    if (op_outer_logical == DEVPROP_OPERATOR_NOT_OPEN) ret = !ret;
    TRACE( "-> %u\n", ret );
    return ret;
}

static BOOL devprop_type_validate( DEVPROPTYPE type, ULONG buf_size )
{
    static const DWORD type_size[] = {
        0, 0,
        sizeof( BYTE ), sizeof( BYTE ),
        sizeof( INT16 ), sizeof( INT16 ),
        sizeof( INT32 ), sizeof( INT32 ),
        sizeof( INT64 ), sizeof( INT64 ),
        sizeof( FLOAT ), sizeof( DOUBLE ), sizeof( DECIMAL ),
        sizeof( GUID ),
        sizeof( CURRENCY ),
        sizeof( DATE ),
        sizeof( FILETIME ),
        sizeof( DEVPROP_BOOLEAN ),
        [DEVPROP_TYPE_DEVPROPKEY] = sizeof( DEVPROPKEY ),
        [DEVPROP_TYPE_DEVPROPTYPE] = sizeof( DEVPROPTYPE ),
        [DEVPROP_TYPE_ERROR] = sizeof( ULONG ),
        [DEVPROP_TYPE_NTSTATUS] = sizeof( NTSTATUS )
    };
    DWORD mod = type & DEVPROP_MASK_TYPEMOD, size;

    if (mod && mod != DEVPROP_TYPEMOD_ARRAY && mod != DEVPROP_TYPEMOD_LIST)
    {
        FIXME( "Unknown DEVPROPTYPE value: %#lx\n", type );
        return FALSE;
    }

    switch (type & DEVPROP_MASK_TYPE)
    {
    case DEVPROP_TYPE_EMPTY:
    case DEVPROP_TYPE_NULL:
        return !mod;
    case DEVPROP_TYPE_SECURITY_DESCRIPTOR:
    case DEVPROP_TYPE_STRING_INDIRECT:
        return !mod && !!buf_size;

    case DEVPROP_TYPE_STRING:
    case DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING:
        return mod != DEVPROP_TYPEMOD_ARRAY && !!buf_size;
    default:
        /* The only valid modifier for the remaining types is DEVPROP_TYPEMOD_ARRAY */
        if ((type & DEVPROP_MASK_TYPE) > DEVPROP_TYPE_NTSTATUS ||
            (mod && mod != DEVPROP_TYPEMOD_ARRAY))
        {
            FIXME( "Unknown DEVPROPTYPE value: %#lx\n", type );
            return FALSE;
        }
        size = type_size[type & DEVPROP_MASK_TYPE];
    }

    return mod == DEVPROP_TYPEMOD_ARRAY ? buf_size >= size : buf_size == size;
}

static BOOL devprop_filters_validate( const DEVPROP_FILTER_EXPRESSION *filters, const DEVPROP_FILTER_EXPRESSION *end )
{
    const DEVPROP_FILTER_EXPRESSION *closing;

    if (filters == end) return FALSE;

    for (const DEVPROP_FILTER_EXPRESSION *filter = filters; filter < end; filter++)
    {
        const DEVPROPERTY *prop = &filter->Property;
        DEVPROP_OPERATOR op = filter->Operator;
        DWORD compare = op & DEVPROP_OPERATOR_MASK_EVAL;
        DWORD list = op & DEVPROP_OPERATOR_MASK_LIST;
        DWORD modifier = op & DEVPROP_OPERATOR_MASK_MODIFIER;
        DWORD logical = op & DEVPROP_OPERATOR_MASK_LOGICAL;
        DWORD array = op & DEVPROP_OPERATOR_MASK_ARRAY;

        if ((compare && compare > DEVPROP_OPERATOR_CONTAINS)
            || (logical && (op & DEVPROP_OPERATOR_MASK_NOT_LOGICAL))
            || (array && (op != DEVPROP_OPERATOR_ARRAY_CONTAINS))
            || !!prop->Buffer != !!prop->BufferSize)
        {
            FIXME( "Unknown operator: %#x\n", op );
            return FALSE;
        }
        if (!op) continue;
        if (compare && compare != DEVPROP_OPERATOR_EXISTS
            && !devprop_type_validate( prop->Type, prop->BufferSize ))
            return FALSE;

        switch (modifier)
        {
        case DEVPROP_OPERATOR_NONE:
        case DEVPROP_OPERATOR_MODIFIER_NOT:
        case DEVPROP_OPERATOR_MODIFIER_IGNORE_CASE:
            break;
        default:
            return FALSE;
        }

        switch (list)
        {
        case DEVPROP_OPERATOR_NONE:
        case DEVPROP_OPERATOR_LIST_CONTAINS:
        case DEVPROP_OPERATOR_LIST_ELEMENT_BEGINS_WITH:
        case DEVPROP_OPERATOR_LIST_ELEMENT_ENDS_WITH:
        case DEVPROP_OPERATOR_LIST_ELEMENT_CONTAINS:
            break;
        default:
            return FALSE;
        }

        switch (logical)
        {
        case DEVPROP_OPERATOR_NONE:
            break;
        case DEVPROP_OPERATOR_AND_OPEN:
        case DEVPROP_OPERATOR_OR_OPEN:
        case DEVPROP_OPERATOR_NOT_OPEN:
            if (!(closing = find_closing_filter( filter, end ))) return FALSE;
            if (!devprop_filters_validate( filter + 1, closing )) return FALSE;
            filter = closing;
            break;
        default:
            return FALSE;
        }
    }

    return TRUE;
}

static LSTATUS copy_device_interface_property( HKEY hkey, const struct device_interface *iface, DEVPROPERTY *property )
{
    struct property prop;
    LSTATUS err;

    init_property( &prop, &property->CompKey.Key, &property->Type, property->Buffer, &property->BufferSize );
    if (!(err = query_device_interface_property( hkey, iface, &prop ))) return ERROR_SUCCESS;
    if (err && err != ERROR_MORE_DATA) return ERROR_SUCCESS;

    if (!(prop.buffer = malloc( property->BufferSize ))) return E_OUTOFMEMORY;
    if ((err = query_device_interface_property( hkey, iface, &prop ))) free( prop.buffer );
    else property->Buffer = prop.buffer;

    return err;
}

static LSTATUS copy_device_interface_properties( HKEY hkey, const struct device_interface *iface, const DEVPROPCOMPKEY *keys, ULONG keys_len,
                                                 DEVPROPERTY *properties, ULONG *props_len )
{
    const DEVPROPCOMPKEY *key, *end;
    DEVPROPERTY *prop = properties;
    LSTATUS err = ERROR_SUCCESS;

    for (key = keys, end = keys + keys_len; !err && key < end; key++, prop++)
    {
        prop->CompKey = *key;
        err = copy_device_interface_property( hkey, iface, prop );
    }
    *props_len = prop - properties;

    return err;
}

static LSTATUS get_property_compare_keys( const DEVPROPKEY *keys, ULONG keys_len, const DEVPROPCOMPKEY **comp_keys, ULONG *comp_keys_len )
{
    if (!(*comp_keys_len = keys_len)) return ERROR_SUCCESS;
    if (!(*comp_keys = calloc( keys_len, sizeof(**comp_keys) ))) return ERROR_OUTOFMEMORY;
    for (UINT i = 0; i < keys_len; ++i) ((DEVPROPCOMPKEY *)*comp_keys)[i].Key = keys[i];
    return ERROR_SUCCESS;
}

static LSTATUS copy_device_interface_property_keys( HKEY hkey, const struct device_interface *iface, const DEVPROPCOMPKEY **comp_keys, ULONG *comp_keys_len )
{
    DEVPROPKEY *tmp, *keys = NULL;
    LSTATUS err;

    for (;;)
    {
        if (!(err = enum_device_interface_property_keys( hkey, iface, keys, comp_keys_len ))) break;
        if (err != ERROR_MORE_DATA) return err;
        if (!(tmp = realloc( keys, *comp_keys_len * sizeof(*keys) ))) return ERROR_OUTOFMEMORY;
        keys = tmp;
    }

    get_property_compare_keys( keys, *comp_keys_len, comp_keys, comp_keys_len );
    free( keys );
    return ERROR_SUCCESS;
}

typedef LSTATUS (*enum_device_object_cb)( DEV_OBJECT_TYPE type, const WCHAR *id, ULONG *props_len, DEVPROPERTY **props, void *context );

static UINT select_property( const DEVPROPCOMPKEY *key, DEVPROPERTY *props, DEVPROPERTY *select_end, DEVPROPERTY *props_end )
{
    for (DEVPROPERTY *prop = select_end; prop < props_end; prop++)
    {
        if (IsEqualDevPropCompKey( prop->CompKey, *key ))
        {
            DEVPROPERTY tmp = *select_end;
            *select_end = *prop;
            *prop = tmp;
            return 1;
        }
    }
    return 0;
}

static void select_properties( const DEVPROPCOMPKEY *keys, ULONG keys_len, DEVPROPERTY **props, ULONG *props_len )
{
    DEVPROPERTY *select_end = *props, *props_end = *props + *props_len;

    for (UINT i = 0; i < keys_len; i++) select_end += select_property( keys + i, *props, select_end, props_end );
    *props_len = select_end - *props;

    while (select_end < props_end) free( (select_end++)->Buffer );
    if (!*props_len)
    {
        free( *props );
        *props = NULL;
    }
}

struct enum_dev_object_params
{
    DEV_OBJECT_TYPE                  type;
    const DEVPROPCOMPKEY            *props;
    ULONG                            props_len;
    BOOL                             all_props;
    const DEVPROP_FILTER_EXPRESSION *filters;
    const DEVPROP_FILTER_EXPRESSION *filters_end;
    enum_device_object_cb            callback;
    void                            *context;
};

static LSTATUS enum_dev_objects_device_interface( HKEY hkey, const void *object, const WCHAR *name, UINT name_len, void *context )
{
    struct enum_dev_object_params *params = context;
    ULONG keys_len = params->props_len, properties_len = 0;
    const struct device_interface *iface = object;
    const DEVPROPCOMPKEY *keys = params->props;
    DEVPROPERTY *properties = NULL;
    LSTATUS err = ERROR_SUCCESS;
    BOOL matches;

    TRACE( "hkey %p object %s name %s\n", hkey, debugstr_device_interface(iface), debugstr_w(name) );

    /* If we're also filtering objects, get all properties for this object. Once the filters have been
     * evaluated, free properties that have not been requested, and set cPropertyCount to comp_keys_len.  */
    if (params->all_props || params->filters) err = copy_device_interface_property_keys( hkey, iface, &keys, &keys_len );
    if (!err)
    {
        if (keys_len && !(properties = calloc( keys_len, sizeof(*properties) ))) err = ERROR_OUTOFMEMORY;
        else err = copy_device_interface_properties( hkey, iface, keys, keys_len, properties, &properties_len );
    }

    if (!err)
    {
        /* Sort properties by DEVPROPCOMPKEY for faster filter evaluation. */
        if (params->filters) qsort( properties, properties_len, sizeof(*properties), devproperty_compare );

        /* By default, the evaluation is performed by AND-ing all individual filter expressions. */
        matches = devprop_filter_matches_properties( properties, properties_len, DEVPROP_OPERATOR_AND_OPEN, params->filters, params->filters_end );

        /* Shrink properties to only the desired ones, unless DevQueryFlagAllProperties is set. */
        if (!params->all_props) select_properties( params->props, params->props_len, &properties, &properties_len );

        if (matches) err = params->callback( params->type, name, &properties_len, &properties, params->context );
    }

    DevFreeObjectProperties( properties_len, properties );
    if (params->all_props || params->filters) free( (void *)keys );

    return err;
}

static LSTATUS enum_dev_objects( DEV_OBJECT_TYPE type, const DEVPROPCOMPKEY *props, ULONG props_len, BOOL all_props,
                                 const DEVPROP_FILTER_EXPRESSION *filters, const DEVPROP_FILTER_EXPRESSION *filters_end,
                                 enum_device_object_cb callback, void *context )
{
    struct enum_dev_object_params params =
    {
        .type = type,
        .props = props,
        .props_len = props_len,
        .all_props = all_props,
        .filters = filters,
        .filters_end = filters_end,
        .callback = callback,
        .context = context,
    };

    switch (type)
    {
    case DevObjectTypeDeviceInterface:
    case DevObjectTypeDeviceInterfaceDisplay:
        return enum_device_interfaces( TRUE, enum_dev_objects_device_interface, &params );

    case DevObjectTypeDeviceContainer:
    case DevObjectTypeDevice:
    case DevObjectTypeDeviceInterfaceClass:
    case DevObjectTypeAEP:
    case DevObjectTypeAEPContainer:
    case DevObjectTypeDeviceInstallerClass:
    case DevObjectTypeDeviceContainerDisplay:
    case DevObjectTypeAEPService:
    case DevObjectTypeDevicePanel:
    case DevObjectTypeAEPProtocol:
        FIXME("Unsupported DEV_OJBECT_TYPE: %d\n", type );
    default:
        return ERROR_SUCCESS;
    }
}

struct objects_list
{
    DEV_OBJECT *objects;
    ULONG len;
};

static LSTATUS dev_objects_append( DEV_OBJECT_TYPE type, const WCHAR *id, ULONG *props_len, DEVPROPERTY **props, void *context )
{
    struct objects_list *list = context;
    DEV_OBJECT *tmp, *obj;

    if (!(tmp = realloc( list->objects, (list->len + 1) * sizeof(*list->objects) ))) return ERROR_OUTOFMEMORY;
    list->objects = tmp;

    obj = list->objects + list->len;
    if (!(obj->pszObjectId = wcsdup( id ))) return ERROR_OUTOFMEMORY;
    obj->ObjectType = type;
    obj->cPropertyCount = *props_len;
    obj->pProperties = *props;
    *props_len = 0;
    *props = NULL;
    list->len++;

    return ERROR_SUCCESS;
}

HRESULT WINAPI DevGetObjects( DEV_OBJECT_TYPE type, ULONG flags, ULONG props_len, const DEVPROPCOMPKEY *props,
                              ULONG filters_len, const DEVPROP_FILTER_EXPRESSION *filters, ULONG *objs_len,
                              const DEV_OBJECT **objs )
{
    TRACE( "(%d, %#lx, %lu, %p, %lu, %p, %p, %p)\n", type, flags, props_len, props, filters_len, filters, objs_len, objs );
    return DevGetObjectsEx( type, flags, props_len, props, filters_len, filters, 0, NULL, objs_len, objs );
}

HRESULT WINAPI DevGetObjectsEx( DEV_OBJECT_TYPE type, ULONG flags, ULONG props_len, const DEVPROPCOMPKEY *props,
                                ULONG filters_len, const DEVPROP_FILTER_EXPRESSION *filters, ULONG params_len,
                                const DEV_QUERY_PARAMETER *params, ULONG *objs_len, const DEV_OBJECT **objs )
{
    ULONG valid_flags = DevQueryFlagAllProperties | DevQueryFlagLocalize;
    struct objects_list objects = {0};
    LSTATUS err;

    TRACE( "(%d, %#lx, %lu, %p, %lu, %p, %lu, %p, %p, %p)\n", type, flags, props_len, props, filters_len, filters,
           params_len, params, objs_len, objs );

    if (!!props_len != !!props || !!filters_len != !!filters || !!params_len != !!params || (flags & ~valid_flags)
        || (props_len && (flags & DevQueryFlagAllProperties))
        || (filters && !devprop_filters_validate( filters, filters + filters_len )))
        return E_INVALIDARG;
    if (params)
        FIXME( "Query parameters are not supported!\n" );

    *objs = NULL;
    *objs_len = 0;
    if ((err = enum_dev_objects( type, props, props_len, !!(flags & DevQueryFlagAllProperties), filters, filters + filters_len,
                                 dev_objects_append, &objects )))
    {
        DevFreeObjects( objects.len, objects.objects );
        return HRESULT_FROM_WIN32(err);
    }

    *objs = objects.objects;
    *objs_len = objects.len;
    return S_OK;
}

void WINAPI DevFreeObjects( ULONG objs_len, const DEV_OBJECT *objs )
{
    DEV_OBJECT *objects = (DEV_OBJECT *)objs;
    ULONG i;

    TRACE( "(%lu, %p)\n", objs_len, objs );

    for (i = 0; i < objs_len; i++)
    {
        DevFreeObjectProperties( objects[i].cPropertyCount, objects[i].pProperties );
        free( (void *)objects[i].pszObjectId );
    }
    free( objects );
    return;
}

struct device_iface_path
{
    struct rb_entry entry;
    WCHAR *path;
};

static int device_iface_path_cmp( const void *key, const struct rb_entry *entry )
{
    const struct device_iface_path *iface = RB_ENTRY_VALUE( entry, struct device_iface_path, entry );
    return wcsicmp( key, iface->path );
}

struct device_query_context
{
    LONG ref;
    DEV_OBJECT_TYPE type;
    ULONG flags;
    ULONG prop_keys_len;
    DEVPROPCOMPKEY *prop_keys;
    BOOL filters;

    CRITICAL_SECTION cs;
    PDEV_QUERY_RESULT_CALLBACK callback;
    void *user_data;
    DEV_QUERY_STATE state;
    HANDLE closed;
    struct rb_tree known_ifaces;
    HCMNOTIFICATION notify;
};

static LSTATUS device_query_context_add_object( DEV_OBJECT_TYPE type, const WCHAR *id, ULONG *props_len, DEVPROPERTY **props, void *data )
{
    DEV_QUERY_RESULT_ACTION_DATA action_data =
    {
        .Action = DevQueryResultAdd,
        .Data.DeviceObject =
        {
            .ObjectType = type,
            .pszObjectId = id,
            .cPropertyCount = *props_len,
            .pProperties = *props,
        },
    };
    struct device_query_context *ctx = data;
    struct device_iface_path *iface_entry = NULL;
    LSTATUS err = ERROR_SUCCESS;

    TRACE( "(%s, %p)\n", debugstr_w( id ), data );

    ctx->callback( (HDEVQUERY)ctx, ctx->user_data, &action_data );

    EnterCriticalSection( &ctx->cs );
    if (ctx->state == DevQueryStateClosed)
        err = ERROR_CANCELLED;
    else if (type == DevObjectTypeDeviceInterface || type == DevObjectTypeDeviceInterfaceDisplay)
    {
        if (!(iface_entry = calloc( 1, sizeof( *iface_entry ) )) || !(iface_entry->path = wcsdup( id )))
        {
            if (iface_entry) free( iface_entry->path );
            free( iface_entry );
            err = ERROR_OUTOFMEMORY;
        }
        else if (rb_put( &ctx->known_ifaces, iface_entry->path, &iface_entry->entry ))
        {
            free( iface_entry->path );
            free( iface_entry );
        }
    }
    LeaveCriticalSection( &ctx->cs );

    return err;
}

static HRESULT device_query_context_create( struct device_query_context **query, DEV_OBJECT_TYPE type, ULONG flags,
                                            ULONG props_len, const DEVPROPCOMPKEY *props,
                                            PDEV_QUERY_RESULT_CALLBACK callback, void *user_data )
{
    struct device_query_context *ctx;
    ULONG i;

    if (!(ctx = calloc( 1, sizeof( *ctx ))))
        return E_OUTOFMEMORY;
    ctx->ref = 1;
    if (!(flags & DevQueryFlagAsyncClose))
    {
        ctx->closed = CreateEventW( NULL, FALSE, FALSE, NULL );
        if (ctx->closed == INVALID_HANDLE_VALUE)
        {
            free( ctx );
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    ctx->prop_keys_len = props_len;
    if (props_len && !(ctx->prop_keys = calloc( props_len, sizeof( *props ) )))
    {
        if (ctx->closed) CloseHandle( ctx->closed );
        free( ctx );
        return E_OUTOFMEMORY;
    }
    for (i = 0; i < props_len; i++)
    {
        ctx->prop_keys[i].Key = props[i].Key;
        ctx->prop_keys[i].Store = props[i].Store;
    }
    InitializeCriticalSectionEx( &ctx->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    ctx->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": device_query_context.cs");
    ctx->type = type;
    ctx->flags = flags;
    ctx->callback = callback;
    ctx->user_data = user_data;
    ctx->state = DevQueryStateInitialized;
    rb_init( &ctx->known_ifaces, device_iface_path_cmp );

    *query = ctx;
    return S_OK;
}

static void device_query_context_addref( struct device_query_context *ctx )
{
    InterlockedIncrement( &ctx->ref );
}

static void device_iface_path_free( struct rb_entry *entry, void *data )
{
    struct device_iface_path *path = RB_ENTRY_VALUE( entry, struct device_iface_path, entry );
    free( path->path );
    free( path );
}

static void device_query_context_release( struct device_query_context *ctx )
{
    if (!InterlockedDecrement( &ctx->ref ))
    {
        free( ctx->prop_keys );
        ctx->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection( &ctx->cs );
        if (ctx->closed) CloseHandle( ctx->closed );
        rb_destroy( &ctx->known_ifaces, device_iface_path_free, NULL );
        free( ctx );
    }
}

static void device_query_context_notify_state_change( struct device_query_context *ctx, DEV_QUERY_STATE state )
{
    DEV_QUERY_RESULT_ACTION_DATA action_data = {0};

    action_data.Action = DevQueryResultStateChange;
    action_data.Data.State = state;
    ctx->callback( (HDEVQUERY)ctx, ctx->user_data, &action_data );
}

static void CALLBACK device_query_context_notify_enum_completed_async( TP_CALLBACK_INSTANCE *instance, void *data )
{
    device_query_context_notify_state_change( data, DevQueryStateEnumCompleted );
    device_query_context_release( data );
}

static void CALLBACK device_query_context_notify_closed_async( TP_CALLBACK_INSTANCE *instance, void *data )
{
    device_query_context_notify_state_change( data, DevQueryStateClosed );
    device_query_context_release( data );
}

static void CALLBACK device_query_context_notify_aborted_async( TP_CALLBACK_INSTANCE *instance, void *data )
{
    device_query_context_notify_state_change( data, DevQueryStateAborted );
    device_query_context_release( data );
}

struct devquery_notify_data
{
    DEV_QUERY_RESULT_ACTION_DATA action_data;
    struct device_query_context *ctx;
};

static void CALLBACK device_query_notify_dev_async( TP_CALLBACK_INSTANCE *instance, void *notify_data )
{
    struct devquery_notify_data *data = notify_data;

    data->ctx->callback( (HDEVQUERY)data->ctx, data->ctx->user_data, &data->action_data );
    if (data->action_data.Action != DevQueryResultStateChange)
        DevFreeObjectProperties( data->action_data.Data.DeviceObject.cPropertyCount,
                                 data->action_data.Data.DeviceObject.pProperties );
    device_query_context_release( data->ctx );
    free( data );
}

static const char *debugstr_CM_NOTIFY_ACTION( CM_NOTIFY_ACTION action )
{
    static const char *str[] = {
        "CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL",
        "CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL",
        "CM_NOTIFY_ACTION_DEVICEQUERYREMOVE",
        "CM_NOTIFY_ACTION_DEVICEQUERYREMOVEFAILED",
        "CM_NOTIFY_ACTION_DEVICEREMOVEPENDING",
        "CM_NOTIFY_ACTION_DEVICEREMOVECOMPLETE",
        "CM_NOTIFY_ACTION_DEVICECUSTOMEVENT",
        "CM_NOTIFY_ACTION_DEVICEINSTANCEENUMERATED",
        "CM_NOTIFY_ACTION_DEVICEINSTANCESTARTED",
        "CM_NOTIFY_ACTION_DEVICEINSTANCEREMOVED",
    };

    return action < ARRAY_SIZE( str ) ? str[action] : wine_dbg_sprintf( "(unknown %d)", action );
}

static const char *debugstr_CM_NOTIFY_EVENT_DATA( const CM_NOTIFY_EVENT_DATA *event )
{
    if (!event) return wine_dbg_sprintf( "(null)" );
    switch (event->FilterType)
    {
    case CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE:
        return wine_dbg_sprintf( "{CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE %lu {{%s %s}}}", event->Reserved,
                                 debugstr_guid( &event->u.DeviceInterface.ClassGuid ),
                                 debugstr_w( event->u.DeviceInterface.SymbolicLink ) );
    case CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE:
        if (event->u.DeviceHandle.NameOffset == -1)
        {
            return wine_dbg_sprintf( "{CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE %lu {{%s -1 %lu %p}}}", event->Reserved,
                                     debugstr_guid( &event->u.DeviceHandle.EventGuid ),
                                     event->u.DeviceHandle.DataSize, event->u.DeviceHandle.Data );
        }
        return wine_dbg_sprintf( "{CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE %lu {{%s %ld %lu %p %s}}}", event->Reserved,
                                 debugstr_guid( &event->u.DeviceHandle.EventGuid ), event->u.DeviceHandle.NameOffset,
                                 event->u.DeviceHandle.DataSize, event->u.DeviceHandle.Data,
                                 debugstr_w( (WCHAR *)&event->u.DeviceHandle.Data[event->u.DeviceHandle.NameOffset] ) );
    case CM_NOTIFY_FILTER_TYPE_DEVICEINSTANCE:
        return wine_dbg_sprintf( "{CM_NOTIFY_FILTER_TYPE_DEVICEINSTANCE %lu %s}", event->Reserved,
                                 debugstr_w( event->u.DeviceInstance.InstanceId ) );
    default:
        return wine_dbg_sprintf( "{(unknown %d) %lu}", event->FilterType, event->Reserved );
    }
}

static DWORD CALLBACK device_query_context_cm_notify_callback( HCMNOTIFICATION notify, void *data,
                                                               CM_NOTIFY_ACTION action,
                                                               CM_NOTIFY_EVENT_DATA *event, DWORD event_size )
{
    struct device_query_context *ctx = data;
    const ULONG prop_flags = ctx->flags & (DevQueryFlagAllProperties | DevQueryFlagLocalize);
    DEV_QUERY_RESULT_ACTION_DATA *action_data;
    struct devquery_notify_data *notify_data;
    struct device_iface_path *iface_entry;
    const WCHAR *iface_path;
    struct rb_entry *entry;
    HRESULT hr = S_OK;

    TRACE( "(%p, %p, %s, %s, %lu)\n", notify, data, debugstr_CM_NOTIFY_ACTION( action ),
           debugstr_CM_NOTIFY_EVENT_DATA( event ), event_size );

    if (action != CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL && action != CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL)
    {
        FIXME( "Unexpected CM_NOTIFY_ACTION value: %d\n", action );
        return 0;
    }

    iface_path = event->u.DeviceInterface.SymbolicLink;
    EnterCriticalSection( &ctx->cs );
    if (ctx->state == DevQueryStateClosed || ctx->state == DevQueryStateAborted)
    {
        LeaveCriticalSection( &ctx->cs );
        return 0;
    }

    if (!(notify_data = calloc( 1, sizeof ( *notify_data ) )))
        goto abort;
    action_data = &notify_data->action_data;
    notify_data->ctx = ctx;
    action_data->Data.DeviceObject.ObjectType = ctx->type;

    /* Interface removal and arrival for objects that have already been enumerated. */
    if ((entry = rb_get( &ctx->known_ifaces, iface_path )))
    {
        const DEVPROP_BOOLEAN enabled = action == CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL ? DEVPROP_TRUE : DEVPROP_FALSE;
        DEVPROPERTY *prop;

        if (!(prop = calloc( 1, sizeof( *prop ))))
            goto abort;
        if (!(prop->Buffer = calloc( 1, sizeof( enabled ) )))
        {
            free( prop );
            goto abort;
        }
        prop->CompKey.Key = DEVPKEY_DeviceInterface_Enabled;
        prop->Type = DEVPROP_TYPE_BOOLEAN;
        prop->BufferSize = sizeof( enabled );
        memcpy( prop->Buffer, &enabled, sizeof( enabled ) );
        iface_entry =  RB_ENTRY_VALUE( entry, struct device_iface_path, entry );
        action_data->Action = DevQueryResultUpdate;
        action_data->Data.DeviceObject.cPropertyCount = 1;
        action_data->Data.DeviceObject.pProperties = prop;
    }
    else
    {
        if (!(iface_entry = calloc( 1, sizeof( *iface_entry ) )))
            goto abort;
        if (!(iface_entry->path = wcsdup( iface_path )))
        {
            free( iface_entry );
            goto abort;
        }
        rb_put( &ctx->known_ifaces, iface_path, &iface_entry->entry );
        hr = DevGetObjectProperties( ctx->type, iface_path, prop_flags, ctx->prop_keys_len, ctx->prop_keys,
                                     &action_data->Data.DeviceObject.cPropertyCount,
                                     &action_data->Data.DeviceObject.pProperties );
        if (FAILED( hr ))
            goto abort;
        action_data->Action = DevQueryResultAdd;
    }

    action_data->Data.DeviceObject.pszObjectId = iface_entry->path;
    device_query_context_addref( ctx );
    if (!TrySubmitThreadpoolCallback( device_query_notify_dev_async, notify_data, NULL ))
        device_query_context_release( ctx );
    LeaveCriticalSection( &ctx->cs );
    return 0;
abort:
    free( notify_data );
    ctx->state = DevQueryStateAborted;
    device_query_context_addref( ctx );
    if (!TrySubmitThreadpoolCallback( device_query_context_notify_aborted_async, ctx, NULL ))
        device_query_context_release( ctx );
    LeaveCriticalSection( &ctx->cs );
    return 0;
}

static void CALLBACK device_query_enum_objects_async( TP_CALLBACK_INSTANCE *instance, void *data )
{
    struct device_query_context *ctx = data;
    LSTATUS err = ERROR_SUCCESS;
    BOOL success;

    if (!ctx->filters)
        err = enum_dev_objects( ctx->type, ctx->prop_keys, ctx->prop_keys_len, !!(ctx->flags & DevQueryFlagAllProperties),
                               0, NULL, device_query_context_add_object, ctx );

    EnterCriticalSection( &ctx->cs );
    if (ctx->state == DevQueryStateClosed)
        err = ERROR_CANCELLED;

    switch (err)
    {
    case ERROR_SUCCESS:
        ctx->state = DevQueryStateEnumCompleted;
        success = TrySubmitThreadpoolCallback( device_query_context_notify_enum_completed_async, ctx, NULL );
        if (ctx->filters || !(ctx->flags & DevQueryFlagUpdateResults))
            break;
        switch (ctx->type)
        {
        case DevObjectTypeDeviceInterface:
        case DevObjectTypeDeviceInterfaceDisplay:
        {
            CM_NOTIFY_FILTER filter = { 0 };
            CONFIGRET ret;

            filter.cbSize = sizeof( filter );
            filter.Flags = CM_NOTIFY_FILTER_FLAG_ALL_INTERFACE_CLASSES;
            filter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
            device_query_context_addref( ctx );
            ret = CM_Register_Notification( &filter, ctx, device_query_context_cm_notify_callback, &ctx->notify );
            if (ret)
            {
                ERR( "CM_Register_Notification failed: %lu\n", ret );
                device_query_context_release( ctx );
                goto abort;
            }
            break;
        }
        default:
            FIXME( "Device updates not supported for object type %d\n", ctx->type );
            break;
        }
        break;
    case ERROR_CANCELLED:
        if (!(ctx->flags & DevQueryFlagAsyncClose))
        {
            LeaveCriticalSection( &ctx->cs );
            SetEvent( ctx->closed );
            device_query_context_release( ctx );
            return;
        }
        success = TrySubmitThreadpoolCallback( device_query_context_notify_closed_async, ctx, NULL );
        break;
    default:
        goto abort;
    }

    LeaveCriticalSection( &ctx->cs );
    if (!success)
        device_query_context_release( ctx );
    return;
abort:
    ctx->state = DevQueryStateAborted;
    success = TrySubmitThreadpoolCallback( device_query_context_notify_aborted_async, ctx, NULL );
    LeaveCriticalSection( &ctx->cs );
    if (!success)
        device_query_context_release( ctx );
}

HRESULT WINAPI DevCreateObjectQuery( DEV_OBJECT_TYPE type, ULONG flags, ULONG props_len, const DEVPROPCOMPKEY *props,
                                     ULONG filters_len, const DEVPROP_FILTER_EXPRESSION *filters,
                                     PDEV_QUERY_RESULT_CALLBACK callback, void *user_data, HDEVQUERY *devquery )
{
    TRACE( "(%d, %#lx, %lu, %p, %lu, %p, %p, %p, %p)\n", type, flags, props_len, props, filters_len, filters, callback,
           user_data, devquery );
    return DevCreateObjectQueryEx( type, flags, props_len, props, filters_len, filters, 0, NULL, callback, user_data,
                                   devquery );
}

HRESULT WINAPI DevCreateObjectQueryEx( DEV_OBJECT_TYPE type, ULONG flags, ULONG props_len, const DEVPROPCOMPKEY *props,
                                       ULONG filters_len, const DEVPROP_FILTER_EXPRESSION *filters, ULONG params_len,
                                       const DEV_QUERY_PARAMETER *params, PDEV_QUERY_RESULT_CALLBACK callback,
                                       void *user_data, HDEVQUERY *devquery )
{
    ULONG valid_flags = DevQueryFlagUpdateResults | DevQueryFlagAllProperties | DevQueryFlagLocalize | DevQueryFlagAsyncClose;
    struct device_query_context *ctx = NULL;
    HRESULT hr;

    TRACE( "(%d, %#lx, %lu, %p, %lu, %p, %lu, %p, %p, %p, %p)\n", type, flags, props_len, props, filters_len,
           filters, params_len, params, callback, user_data, devquery );

    if (!!props_len != !!props || !!filters_len != !!filters || !!params_len != !!params || (flags & ~valid_flags) || !callback
        || (props_len && (flags & DevQueryFlagAllProperties))
        || (filters && !devprop_filters_validate( filters, filters + filters_len )))
        return E_INVALIDARG;
    if (filters)
        FIXME( "Query filters are not supported!\n" );
    if (params)
        FIXME( "Query parameters are not supported!\n" );

    hr = device_query_context_create( &ctx, type, flags, props_len, props, callback, user_data );
    if (FAILED( hr ))
        return hr;

    ctx->filters = !!filters;
    device_query_context_addref( ctx );
    if (!TrySubmitThreadpoolCallback( device_query_enum_objects_async, ctx, NULL ))
        hr = HRESULT_FROM_WIN32( GetLastError() );
    if (FAILED( hr ))
    {
        device_query_context_release( ctx );
        ctx = NULL;
    }

    *devquery = (HDEVQUERY)ctx;
    return hr;
}

void WINAPI DevCloseObjectQuery( HDEVQUERY query )
{
    struct device_query_context *ctx = (struct device_query_context *)query;
    BOOL async = ctx->flags & DevQueryFlagAsyncClose;
    DEV_QUERY_STATE old;

    TRACE( "(%p)\n", query );

    if (!query)
        return;

    EnterCriticalSection( &ctx->cs );
    old = ctx->state;
    ctx->state = DevQueryStateClosed;

    if (ctx->notify)
    {
        CM_Unregister_Notification( ctx->notify );
        device_query_context_release( ctx ); /* Reference held by CM_Register_Notification. */
    }
    if (async && old == DevQueryStateEnumCompleted)
    {
        /* No asynchronous operation involving this query is active, so we need to notify DevQueryStateClosed. */
        BOOL success = TrySubmitThreadpoolCallback( device_query_context_notify_closed_async, ctx, NULL );
        LeaveCriticalSection( &ctx->cs );
        if (success)
            return;
    }
    else if (!async && old == DevQueryStateInitialized)
    {
        LeaveCriticalSection( &ctx->cs );
        /* Wait for the active async operation to end. */
        WaitForSingleObject( ctx->closed, INFINITE );
    }
    else
        LeaveCriticalSection( &ctx->cs );

    device_query_context_release( ctx );
    return;
}

HRESULT WINAPI DevGetObjectProperties( DEV_OBJECT_TYPE type, const WCHAR *id, ULONG flags, ULONG props_len,
                                       const DEVPROPCOMPKEY *props, ULONG *buf_len, const DEVPROPERTY **buf )
{
    TRACE( "(%d, %s, %#lx, %lu, %p, %p, %p)\n", type, debugstr_w( id ), flags, props_len, props, buf_len, buf );
    return DevGetObjectPropertiesEx( type, id, flags, props_len, props, 0, NULL, buf_len, buf );
}

HRESULT WINAPI DevGetObjectPropertiesEx( DEV_OBJECT_TYPE type, const WCHAR *id, ULONG flags, ULONG props_len,
                                         const DEVPROPCOMPKEY *props, ULONG params_len,
                                         const DEV_QUERY_PARAMETER *params, ULONG *buf_len, const DEVPROPERTY **buf )
{
    ULONG valid_flags = DevQueryFlagAllProperties | DevQueryFlagLocalize;
    BOOL all_props = flags & DevQueryFlagAllProperties;
    LSTATUS err = ERROR_SUCCESS;

    TRACE( "(%d, %s, %#lx, %lu, %p, %lu, %p, %p, %p)\n", type, debugstr_w( id ), flags, props_len, props,
           params_len, params, buf_len, buf );

    if (flags & ~valid_flags)
        return E_INVALIDARG;
    if (type == DevObjectTypeUnknown || type > DevObjectTypeAEPProtocol)
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    if (!id || !!props_len != !!props || !!params_len != !!params
        || (props_len && (flags & DevQueryFlagAllProperties)))
        return E_INVALIDARG;

    switch (type)
    {
    case DevObjectTypeDeviceInterface:
    case DevObjectTypeDeviceInterfaceDisplay:
    {
        ULONG properties_len = 0, keys_len = props_len;
        const DEVPROPCOMPKEY *keys = props;
        DEVPROPERTY *properties = NULL;
        struct device_interface iface;
        HKEY hkey;

        if ((err = init_device_interface( &iface, id ))) break;
        if ((err = open_device_interface_key( &iface, KEY_ALL_ACCESS, TRUE, &hkey ))) break;

        if (all_props) err = copy_device_interface_property_keys( hkey, &iface, &keys, &keys_len );
        if (!err)
        {
            if ((properties_len = keys_len) && !(properties = calloc( keys_len, sizeof(*properties) ))) err = ERROR_OUTOFMEMORY;
            else err = copy_device_interface_properties( hkey, &iface, keys, keys_len, properties, &properties_len );
        }

        *buf = properties;
        *buf_len = properties_len;

        if (keys != props) free( (void *)keys );
        RegCloseKey( hkey );
        break;
    }
    default:
        FIXME( "Unsupported DEV_OBJECT_TYPE: %d\n", type );
        err = ERROR_FILE_NOT_FOUND;
        break;
    }

    if (err) return HRESULT_FROM_WIN32(err);
    return S_OK;
}

const DEVPROPERTY *WINAPI DevFindProperty( const DEVPROPKEY *key, DEVPROPSTORE store, const WCHAR *locale,
                                           ULONG props_len, const DEVPROPERTY *props )
{
    DEVPROPCOMPKEY comp_key;
    ULONG i;

    TRACE( "(%s, %d, %s, %lu, %p)\n", debugstr_DEVPROPKEY( key ), store, debugstr_w( locale ), props_len, props );

    /* Windows does not validate parameters here. */
    comp_key.Key = *key;
    comp_key.Store = store;
    comp_key.LocaleName = locale;
    for (i = 0; i < props_len; i++)
    {
        if (IsEqualDevPropCompKey( comp_key, props[i].CompKey ))
            return &props[i];
    }
    return NULL;
}

void WINAPI DevFreeObjectProperties( ULONG len, const DEVPROPERTY *props )
{
    DEVPROPERTY *properties = (DEVPROPERTY *)props;
    ULONG i;

    TRACE( "(%lu, %p)\n", len, props );

    for (i = 0; i < len; i++)
        free( properties[i].Buffer );
    free( properties );
}
