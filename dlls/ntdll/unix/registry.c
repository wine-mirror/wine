/*
 * Registry functions
 *
 * Copyright 1999 Juergen Schmied
 * Copyright 2000 Alexandre Julliard
 * Copyright 2005 Ivan Leo Puoti, Laurent Pinchart
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
 *
 */

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "unix_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(reg);

/* maximum length of a value name in bytes (without terminating null) */
#define MAX_VALUE_LENGTH (16383 * sizeof(WCHAR))


NTSTATUS open_hkcu_key( const char *path, HANDLE *key )
{
    NTSTATUS status;
    char buffer[256];
    WCHAR bufferW[256];
    DWORD_PTR sid_data[(sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE) / sizeof(DWORD_PTR)];
    DWORD i, len = sizeof(sid_data);
    SID *sid;
    UNICODE_STRING name;
    OBJECT_ATTRIBUTES attr;

    status = NtQueryInformationToken( GetCurrentThreadEffectiveToken(), TokenUser, sid_data, len, &len );
    if (status) return status;

    sid = ((TOKEN_USER *)sid_data)->User.Sid;
    len = snprintf( buffer, sizeof(buffer), "\\Registry\\User\\S-%u-%u", sid->Revision,
                   (int)MAKELONG( MAKEWORD( sid->IdentifierAuthority.Value[5], sid->IdentifierAuthority.Value[4] ),
                                  MAKEWORD( sid->IdentifierAuthority.Value[3], sid->IdentifierAuthority.Value[2] )));
    for (i = 0; i < sid->SubAuthorityCount; i++)
        len += snprintf( buffer + len, sizeof(buffer) - len, "-%u", (int)sid->SubAuthority[i] );
    len += snprintf( buffer + len, sizeof(buffer) - len, "\\%s", path );

    ascii_to_unicode( bufferW, buffer, len + 1 );
    init_unicode_string( &name, bufferW );
    InitializeObjectAttributes( &attr, &name, OBJ_CASE_INSENSITIVE, 0, NULL );
    return NtCreateKey( key, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL );
}


/******************************************************************************
 *              NtCreateKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateKey( HANDLE *key, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                             ULONG index, const UNICODE_STRING *class, ULONG options, ULONG *dispos )
{
    unsigned int ret;
    data_size_t len;
    struct object_attributes *objattr;

    *key = 0;
    if (attr->Length != sizeof(OBJECT_ATTRIBUTES)) return STATUS_INVALID_PARAMETER;
    if (!attr->ObjectName->Length && !attr->RootDirectory) return STATUS_OBJECT_PATH_SYNTAX_BAD;
    if ((ret = alloc_object_attributes( attr, &objattr, &len ))) return ret;
    objattr->attributes |= OBJ_OPENIF | OBJ_CASE_INSENSITIVE;

    TRACE( "(%p,%s,%s,%x,%x,%p)\n", attr->RootDirectory, debugstr_us(attr->ObjectName),
           debugstr_us(class), (int)options, (int)access, key );

    SERVER_START_REQ( create_key )
    {
        req->access     = access;
        req->options    = options;
        wine_server_add_data( req, objattr, len );
        if (class) wine_server_add_data( req, class->Buffer, class->Length );
        ret = wine_server_call( req );
        *key = wine_server_ptr_handle( reply->hkey );
    }
    SERVER_END_REQ;

    if (ret == STATUS_OBJECT_NAME_EXISTS)
    {
        if (dispos) *dispos = REG_OPENED_EXISTING_KEY;
        ret = STATUS_SUCCESS;
    }
    else if (ret == STATUS_SUCCESS)
    {
        if (dispos) *dispos = REG_CREATED_NEW_KEY;
    }

    TRACE( "<- %p\n", *key );
    free( objattr );
    return ret;
}


/******************************************************************************
 *              NtCreateKeyTransacted  (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateKeyTransacted( HANDLE *key, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                                       ULONG index, const UNICODE_STRING *class, ULONG options,
                                       HANDLE transacted, ULONG *dispos )
{
    FIXME( "(%p,%s,%s,%x,%x,%p,%p)\n", attr->RootDirectory, debugstr_us(attr->ObjectName),
           debugstr_us(class), (int)options, (int)access, transacted, key );
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *              NtOpenKeyEx  (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenKeyEx( HANDLE *key, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr, ULONG options )
{
    unsigned int ret;
    ULONG attributes;

    *key = 0;
    if (attr->Length != sizeof(*attr)) return STATUS_INVALID_PARAMETER;
    if (attr->ObjectName->Length & 1) return STATUS_OBJECT_NAME_INVALID;

    TRACE( "(%p,%s,%x,%p)\n", attr->RootDirectory, debugstr_us(attr->ObjectName), (int)access, key );

    if (options & ~REG_OPTION_OPEN_LINK) FIXME( "options %x not implemented\n", (int)options );

    attributes = attr->Attributes | OBJ_CASE_INSENSITIVE;

    SERVER_START_REQ( open_key )
    {
        req->parent     = wine_server_obj_handle( attr->RootDirectory );
        req->access     = access;
        req->attributes = attributes;
        wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        ret = wine_server_call( req );
        *key = wine_server_ptr_handle( reply->hkey );
    }
    SERVER_END_REQ;
    TRACE("<- %p\n", *key);
    return ret;
}


/******************************************************************************
 *              NtOpenKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenKey( HANDLE *key, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    return NtOpenKeyEx( key, access, attr, 0 );
}


/******************************************************************************
 *              NtOpenKeyTransactedEx  (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenKeyTransactedEx( HANDLE *key, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                                       ULONG options, HANDLE transaction )
{
    FIXME( "(%p %x %p %x %p) semi-stub\n", key, (int)access, attr, (int)options, transaction );
    return NtOpenKeyEx( key, access, attr, options );
}


/******************************************************************************
 *              NtOpenKeyTransacted  (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenKeyTransacted( HANDLE *key, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                                     HANDLE transaction )
{
    return NtOpenKeyTransactedEx( key, access, attr, 0, transaction );
}


/******************************************************************************
 *              NtDeleteKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtDeleteKey( HANDLE key )
{
    unsigned int ret;

    TRACE( "(%p)\n", key );

    SERVER_START_REQ( delete_key )
    {
        req->hkey = wine_server_obj_handle( key );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtRenameKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtRenameKey( HANDLE key, UNICODE_STRING *name )
{
    unsigned int ret;

    TRACE( "(%p %s)\n", key, debugstr_us(name) );

    if (!name) return STATUS_ACCESS_VIOLATION;
    if (!name->Buffer || !name->Length) return STATUS_INVALID_PARAMETER;

    SERVER_START_REQ( rename_key )
    {
        req->hkey = wine_server_obj_handle( key );
        wine_server_add_data( req, name->Buffer, name->Length );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *     enumerate_key
 *
 * Implementation of NtQueryKey and NtEnumerateKey
 */
static NTSTATUS enumerate_key( HANDLE handle, int index, KEY_INFORMATION_CLASS info_class,
                               void *info, DWORD length, DWORD *result_len )

{
    unsigned int ret;
    void *data_ptr;
    size_t fixed_size;

    switch (info_class)
    {
    case KeyBasicInformation:  data_ptr = ((KEY_BASIC_INFORMATION *)info)->Name; break;
    case KeyFullInformation:   data_ptr = ((KEY_FULL_INFORMATION *)info)->Class; break;
    case KeyNodeInformation:   data_ptr = ((KEY_NODE_INFORMATION *)info)->Name;  break;
    case KeyNameInformation:   data_ptr = ((KEY_NAME_INFORMATION *)info)->Name;  break;
    case KeyCachedInformation: data_ptr = ((KEY_CACHED_INFORMATION *)info)+1;    break;
    default:
        FIXME( "Information class %d not implemented\n", info_class );
        return STATUS_INVALID_PARAMETER;
    }
    fixed_size = (char *)data_ptr - (char *)info;

    SERVER_START_REQ( enum_key )
    {
        req->hkey       = wine_server_obj_handle( handle );
        req->index      = index;
        req->info_class = info_class;
        if (length > fixed_size) wine_server_set_reply( req, data_ptr, length - fixed_size );
        if (!(ret = wine_server_call( req )))
        {
            switch (info_class)
            {
            case KeyBasicInformation:
            {
                KEY_BASIC_INFORMATION keyinfo;
                keyinfo.LastWriteTime.QuadPart = reply->modif;
                keyinfo.TitleIndex = 0;
                keyinfo.NameLength = reply->namelen;
                memcpy( info, &keyinfo, min( length, fixed_size ) );
            break;
            }

            case KeyFullInformation:
            {
                KEY_FULL_INFORMATION keyinfo;
                keyinfo.LastWriteTime.QuadPart = reply->modif;
                keyinfo.TitleIndex = 0;
                keyinfo.ClassLength = wine_server_reply_size(reply);
                keyinfo.ClassOffset = keyinfo.ClassLength ? fixed_size : -1;
                keyinfo.SubKeys = reply->subkeys;
                keyinfo.MaxNameLen = reply->max_subkey;
                keyinfo.MaxClassLen = reply->max_class;
                keyinfo.Values = reply->values;
                keyinfo.MaxValueNameLen = reply->max_value;
                keyinfo.MaxValueDataLen = reply->max_data;
                memcpy( info, &keyinfo, min( length, fixed_size ) );
                break;
            }

            case KeyNodeInformation:
            {
                KEY_NODE_INFORMATION keyinfo;
                keyinfo.LastWriteTime.QuadPart = reply->modif;
                keyinfo.TitleIndex = 0;
                if (reply->namelen < wine_server_reply_size(reply))
                {
                    keyinfo.ClassLength = wine_server_reply_size(reply) - reply->namelen;
                    keyinfo.ClassOffset = fixed_size + reply->namelen;
                }
                else
                {
                    keyinfo.ClassLength = 0;
                    keyinfo.ClassOffset = -1;
                }
                keyinfo.NameLength = reply->namelen;
                memcpy( info, &keyinfo, min( length, fixed_size ) );
                break;
            }

            case KeyNameInformation:
            {
                KEY_NAME_INFORMATION keyinfo;
                keyinfo.NameLength = reply->namelen;
                memcpy( info, &keyinfo, min( length, fixed_size ) );
                break;
            }

            case KeyCachedInformation:
            {
                KEY_CACHED_INFORMATION keyinfo;
                keyinfo.LastWriteTime.QuadPart = reply->modif;
                keyinfo.TitleIndex = 0;
                keyinfo.SubKeys = reply->subkeys;
                keyinfo.MaxNameLen = reply->max_subkey;
                keyinfo.Values = reply->values;
                keyinfo.MaxValueNameLen = reply->max_value;
                keyinfo.MaxValueDataLen = reply->max_data;
                keyinfo.NameLength = reply->namelen;
                memcpy( info, &keyinfo, min( length, fixed_size ) );
                break;
            }

            default:
                break;
            }
            *result_len = fixed_size + reply->total;
            if (length < fixed_size) ret = STATUS_BUFFER_TOO_SMALL;
            else if (length < *result_len) ret = STATUS_BUFFER_OVERFLOW;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtEnumerateKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtEnumerateKey( HANDLE handle, ULONG index, KEY_INFORMATION_CLASS info_class,
                                void *info, DWORD length, DWORD *result_len )
{
    /* -1 means query key, so avoid it here */
    if (index == (ULONG)-1) return STATUS_NO_MORE_ENTRIES;
    return enumerate_key( handle, index, info_class, info, length, result_len );
}


/******************************************************************************
 *              NtQueryKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryKey( HANDLE handle, KEY_INFORMATION_CLASS info_class,
                            void *info, DWORD length, DWORD *result_len )
{
    return enumerate_key( handle, -1, info_class, info, length, result_len );
}


/******************************************************************************
 *              NtSetInformationKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetInformationKey( HANDLE key, int class, void *info, ULONG length )
{
    FIXME( "(%p,0x%08x,%p,0x%08x) stub\n", key, class, info, (int)length );
    return STATUS_SUCCESS;
}


/* fill the key value info structure for a specific info class */
static void copy_key_value_info( KEY_VALUE_INFORMATION_CLASS info_class, void *info,
                                 DWORD length, int type, int name_len, int data_len )
{
    switch (info_class)
    {
    case KeyValueBasicInformation:
    {
        KEY_VALUE_BASIC_INFORMATION keyinfo;
        keyinfo.TitleIndex = 0;
        keyinfo.Type       = type;
        keyinfo.NameLength = name_len;
        length = min( length, (char *)keyinfo.Name - (char *)&keyinfo );
        memcpy( info, &keyinfo, length );
        break;
    }

    case KeyValueFullInformation:
    {
        KEY_VALUE_FULL_INFORMATION keyinfo;
        keyinfo.TitleIndex = 0;
        keyinfo.Type       = type;
        keyinfo.DataOffset = (char *)keyinfo.Name - (char *)&keyinfo + name_len;
        keyinfo.DataLength = data_len;
        keyinfo.NameLength = name_len;
        length = min( length, (char *)keyinfo.Name - (char *)&keyinfo );
        memcpy( info, &keyinfo, length );
        break;
    }

    case KeyValuePartialInformation:
    {
        KEY_VALUE_PARTIAL_INFORMATION keyinfo;
        keyinfo.TitleIndex = 0;
        keyinfo.Type       = type;
        keyinfo.DataLength = data_len;
        length = min( length, (char *)keyinfo.Data - (char *)&keyinfo );
        memcpy( info, &keyinfo, length );
        break;
    }

    case KeyValuePartialInformationAlign64:
    {
        KEY_VALUE_PARTIAL_INFORMATION_ALIGN64 keyinfo;
        keyinfo.Type       = type;
        keyinfo.DataLength = data_len;
        length = min( length, (char *)keyinfo.Data - (char *)&keyinfo );
        memcpy( info, &keyinfo, length );
        break;
    }

    default:
        break;
    }
}


/******************************************************************************
 *              NtEnumerateValueKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtEnumerateValueKey( HANDLE handle, ULONG index, KEY_VALUE_INFORMATION_CLASS info_class,
                                     void *info, DWORD length, DWORD *result_len )
{
    unsigned int ret;
    void *ptr;
    size_t fixed_size;

    TRACE( "(%p,%u,%d,%p,%d)\n", handle, (int)index, info_class, info, (int)length );

    /* compute the length we want to retrieve */
    switch (info_class)
    {
    case KeyValueBasicInformation:   ptr = ((KEY_VALUE_BASIC_INFORMATION *)info)->Name; break;
    case KeyValueFullInformation:    ptr = ((KEY_VALUE_FULL_INFORMATION *)info)->Name; break;
    case KeyValuePartialInformation: ptr = ((KEY_VALUE_PARTIAL_INFORMATION *)info)->Data; break;
    default:
        FIXME( "Information class %d not implemented\n", info_class );
        return STATUS_INVALID_PARAMETER;
    }
    fixed_size = (char *)ptr - (char *)info;

    SERVER_START_REQ( enum_key_value )
    {
        req->hkey       = wine_server_obj_handle( handle );
        req->index      = index;
        req->info_class = info_class;
        if (length > fixed_size) wine_server_set_reply( req, ptr, length - fixed_size );
        if (!(ret = wine_server_call( req )))
        {
            copy_key_value_info( info_class, info, length, reply->type, reply->namelen,
                                 wine_server_reply_size(reply) - reply->namelen );
            *result_len = fixed_size + reply->total;
            if (length < *result_len) ret = STATUS_BUFFER_OVERFLOW;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtQueryValueKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryValueKey( HANDLE handle, const UNICODE_STRING *name,
                                 KEY_VALUE_INFORMATION_CLASS info_class,
                                 void *info, DWORD length, DWORD *result_len )
{
    unsigned int ret;
    UCHAR *data_ptr;
    unsigned int fixed_size, min_size;

    TRACE( "(%p,%s,%d,%p,%d)\n", handle, debugstr_us(name), info_class, info, (int)length );

    if (name->Length > MAX_VALUE_LENGTH) return STATUS_OBJECT_NAME_NOT_FOUND;

    /* compute the length we want to retrieve */
    switch(info_class)
    {
    case KeyValueBasicInformation:
    {
        KEY_VALUE_BASIC_INFORMATION *basic_info = info;
        min_size = FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name);
        fixed_size = min_size + name->Length;
        if (min_size < length)
            memcpy(basic_info->Name, name->Buffer, min(length - min_size, name->Length));
        data_ptr = NULL;
        break;
    }

    case KeyValueFullInformation:
    {
        KEY_VALUE_FULL_INFORMATION *full_info = info;
        min_size = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name);
        fixed_size = min_size + name->Length;
        if (min_size < length)
            memcpy(full_info->Name, name->Buffer, min(length - min_size, name->Length));
        data_ptr = (UCHAR *)full_info->Name + name->Length;
        break;
    }

    case KeyValuePartialInformation:
        min_size = fixed_size = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
        data_ptr = ((KEY_VALUE_PARTIAL_INFORMATION *)info)->Data;
        break;

    case KeyValuePartialInformationAlign64:
        min_size = fixed_size = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, Data);
        data_ptr = ((KEY_VALUE_PARTIAL_INFORMATION_ALIGN64 *)info)->Data;
        break;

    default:
        FIXME( "Information class %d not implemented\n", info_class );
        return STATUS_INVALID_PARAMETER;
    }

    SERVER_START_REQ( get_key_value )
    {
        req->hkey = wine_server_obj_handle( handle );
        wine_server_add_data( req, name->Buffer, name->Length );
        if (length > fixed_size && data_ptr) wine_server_set_reply( req, data_ptr, length - fixed_size );
        if (!(ret = wine_server_call( req )))
        {
            copy_key_value_info( info_class, info, length, reply->type,
                                 name->Length, reply->total );
            *result_len = fixed_size + (info_class == KeyValueBasicInformation ? 0 : reply->total);
            if (length < min_size) ret = STATUS_BUFFER_TOO_SMALL;
            else if (length < *result_len) ret = STATUS_BUFFER_OVERFLOW;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtQueryMultipleValueKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryMultipleValueKey( HANDLE key, KEY_MULTIPLE_VALUE_INFORMATION *info,
                                         ULONG count, void *buffer, ULONG length, ULONG *retlen )
{
    FIXME( "(%p,%p,0x%08x,%p,0x%08x,%p) stub!\n", key, info, (int)count, buffer, (int)length, retlen );
    return STATUS_SUCCESS;
}


/******************************************************************************
 *              NtSetValueKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetValueKey( HANDLE key, const UNICODE_STRING *name, ULONG index,
                               ULONG type, const void *data, ULONG count )
{
    unsigned int ret;

    TRACE( "(%p,%s,%d,%p,%d)\n", key, debugstr_us(name), (int)type, data, (int)count );

    if (name->Length > MAX_VALUE_LENGTH) return STATUS_INVALID_PARAMETER;

    SERVER_START_REQ( set_key_value )
    {
        req->hkey    = wine_server_obj_handle( key );
        req->type    = type;
        req->namelen = name->Length;
        wine_server_add_data( req, name->Buffer, name->Length );
        wine_server_add_data( req, data, count );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtDeleteValueKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtDeleteValueKey( HANDLE key, const UNICODE_STRING *name )
{
    unsigned int ret;

    TRACE( "(%p,%s)\n", key, debugstr_us(name) );

    if (name->Length > MAX_VALUE_LENGTH) return STATUS_OBJECT_NAME_NOT_FOUND;

    SERVER_START_REQ( delete_key_value )
    {
        req->hkey = wine_server_obj_handle( key );
        wine_server_add_data( req, name->Buffer, name->Length );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtNotifyChangeMultipleKeys  (NTDLL.@)
 */
NTSTATUS WINAPI NtNotifyChangeMultipleKeys( HANDLE key, ULONG count, OBJECT_ATTRIBUTES *attr,
                                            HANDLE event, PIO_APC_ROUTINE apc, void *apc_context,
                                            IO_STATUS_BLOCK *io, ULONG filter, BOOLEAN subtree,
                                            void *buffer, ULONG length, BOOLEAN async )
{
    unsigned int ret;

    TRACE( "(%p,%u,%p,%p,%p,%p,%p,0x%08x, 0x%08x,%p,0x%08x,0x%08x)\n",
           key, (int)count, attr, event, apc, apc_context, io,
           (int)filter, async, buffer, (int)length, subtree );

    if (count || attr || apc || apc_context || buffer || length)
        FIXME( "Unimplemented optional parameter\n" );

    if (!async)
    {
        OBJECT_ATTRIBUTES attr;
        InitializeObjectAttributes( &attr, NULL, 0, NULL, NULL );
        ret = NtCreateEvent( &event, EVENT_ALL_ACCESS, &attr, SynchronizationEvent, FALSE );
        if (ret) return ret;
    }

    SERVER_START_REQ( set_registry_notification )
    {
        req->hkey    = wine_server_obj_handle( key );
        req->event   = wine_server_obj_handle( event );
        req->subtree = subtree;
        req->filter  = filter;
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;

    if (!async)
    {
        if (ret == STATUS_PENDING) ret = NtWaitForSingleObject( event, FALSE, NULL );
        NtClose( event );
    }
    return ret;
}


/******************************************************************************
 *              NtNotifyChangeKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtNotifyChangeKey( HANDLE key, HANDLE event, PIO_APC_ROUTINE apc, void *apc_context,
                                   IO_STATUS_BLOCK *io, ULONG filter, BOOLEAN subtree,
                                   void *buffer, ULONG length, BOOLEAN async )
{
    return NtNotifyChangeMultipleKeys( key, 0, NULL, event, apc, apc_context,
                                       io, filter, subtree, buffer, length, async );
}


/******************************************************************************
 *              NtFlushKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtFlushKey( HANDLE key )
{
    unsigned int ret;

    TRACE( "key=%p\n", key );

    SERVER_START_REQ( flush_key )
    {
	req->hkey = wine_server_obj_handle( key );
	ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtLoadKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtLoadKey( const OBJECT_ATTRIBUTES *attr, OBJECT_ATTRIBUTES *file )
{
    TRACE( "(%p,%p)\n", attr, file );
    return NtLoadKeyEx( attr, file, 0, 0, 0, 0, NULL, NULL );
}

/******************************************************************************
 *              NtLoadKey2  (NTDLL.@)
 */
NTSTATUS WINAPI NtLoadKey2( const OBJECT_ATTRIBUTES *attr, OBJECT_ATTRIBUTES *file, ULONG flags )
{
    return NtLoadKeyEx( attr, file, flags, 0, 0, 0, NULL, NULL );
}

/******************************************************************************
 *              NtLoadKeyEx  (NTDLL.@)
 */
NTSTATUS WINAPI NtLoadKeyEx( const OBJECT_ATTRIBUTES *attr, OBJECT_ATTRIBUTES *file, ULONG flags, HANDLE trustkey,
                             HANDLE event, ACCESS_MASK access, HANDLE *roothandle, IO_STATUS_BLOCK *iostatus )
{
    unsigned int ret;
    HANDLE key;
    data_size_t len;
    struct object_attributes *objattr;
    char *unix_name;
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES new_attr = *file;

    TRACE( "(%p,%p,0x%x,%p,%p,0x%x,%p,%p)\n",
           attr, file, (int)flags, trustkey, event, (int)access, roothandle, iostatus );

    if (flags) FIXME( "flags %x not handled\n", (int)flags );
    if (trustkey) FIXME("trustkey parameter not supported\n");
    if (event) FIXME("event parameter not supported\n");
    if (access) FIXME("access parameter not supported\n");
    if (roothandle) FIXME("roothandle is not filled\n");
    if (iostatus) FIXME("iostatus is not filled\n");

    get_redirect( &new_attr, &nt_name );
    if (!(ret = nt_to_unix_file_name( &new_attr, &unix_name, FILE_OPEN )))
    {
        ret = open_unix_file( &key, unix_name, GENERIC_READ | SYNCHRONIZE,
                              &new_attr, 0, 0, FILE_OPEN, 0, NULL, 0 );
        free( unix_name );
    }
    free( nt_name.Buffer );

    if (ret) return ret;

    if ((ret = alloc_object_attributes( attr, &objattr, &len ))) return ret;
    objattr->attributes |= OBJ_OPENIF | OBJ_CASE_INSENSITIVE;

    SERVER_START_REQ( load_registry )
    {
        req->file = wine_server_obj_handle( key );
        wine_server_add_data( req, objattr, len );
        ret = wine_server_call( req );
        if (ret == STATUS_OBJECT_NAME_EXISTS) ret = STATUS_SUCCESS;
    }
    SERVER_END_REQ;

    NtClose( key );
    free( objattr );
    return ret;
}

/******************************************************************************
 *              NtUnloadKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtUnloadKey( OBJECT_ATTRIBUTES *attr )
{
    unsigned int ret;

    TRACE( "(%p)\n", attr );

    if (!attr || !attr->ObjectName) return STATUS_ACCESS_VIOLATION;
    if (attr->Length != sizeof(*attr)) return STATUS_INVALID_PARAMETER;
    if (attr->ObjectName->Length & 1) return STATUS_OBJECT_NAME_INVALID;

    SERVER_START_REQ( unload_registry )
    {
        req->parent     = wine_server_obj_handle( attr->RootDirectory );
        req->attributes = attr->Attributes;
        wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        ret = wine_server_call(req);
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtSaveKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtSaveKey( HANDLE key, HANDLE file )
{
    unsigned int ret;

    TRACE( "(%p,%p)\n", key, file );

    SERVER_START_REQ( save_registry )
    {
        req->hkey = wine_server_obj_handle( key );
        req->file = wine_server_obj_handle( file );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtRestoreKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtRestoreKey( HANDLE key, HANDLE file, ULONG flags )
{
    FIXME( "(%p,%p,0x%08x) stub\n", key, file, (int)flags );
    return STATUS_SUCCESS;
}


/******************************************************************************
 *              NtReplaceKey  (NTDLL.@)
 */
NTSTATUS WINAPI NtReplaceKey( OBJECT_ATTRIBUTES *attr, HANDLE key, OBJECT_ATTRIBUTES *replace )
{
    FIXME( "(%s,%p,%s),stub!\n", debugstr_us(attr->ObjectName), key, debugstr_us(replace->ObjectName) );
    return STATUS_SUCCESS;
}

/******************************************************************************
 *              NtQueryLicenseValue  (NTDLL.@)
 *
 * NOTES
 *  On Windows all license properties are stored in a single key, but
 *  unless there is some app which explicitly depends on that, there is
 *  no good reason to reproduce that.
 */
NTSTATUS WINAPI NtQueryLicenseValue( const UNICODE_STRING *name, ULONG *type,
                                     void *data, ULONG length, ULONG *retlen )
{
    static const WCHAR nameW[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                  'M','a','c','h','i','n','e','\\',
                                  'S','o','f','t','w','a','r','e','\\',
                                  'W','i','n','e','\\','L','i','c','e','n','s','e',
                                  'I','n','f','o','r','m','a','t','i','o','n',0};
    UNICODE_STRING keyW = RTL_CONSTANT_STRING( nameW );
    KEY_VALUE_PARTIAL_INFORMATION *info;
    NTSTATUS status = STATUS_OBJECT_NAME_NOT_FOUND;
    DWORD info_length, count;
    OBJECT_ATTRIBUTES attr;
    HANDLE key;

    if (!name || !name->Buffer || !name->Length || !retlen) return STATUS_INVALID_PARAMETER;

    info_length = FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data ) + length;
    if (!(info = malloc( info_length ))) return STATUS_NO_MEMORY;

    InitializeObjectAttributes( &attr, &keyW, 0, 0, NULL );

    /* @@ Wine registry key: HKLM\Software\Wine\LicenseInformation */
    if (!NtOpenKey( &key, KEY_READ, &attr ))
    {
        status = NtQueryValueKey( key, name, KeyValuePartialInformation, info, info_length, &count );
        if (!status || status == STATUS_BUFFER_OVERFLOW)
        {
            if (type) *type = info->Type;
            *retlen = info->DataLength;
            if (status == STATUS_BUFFER_OVERFLOW)
                status = STATUS_BUFFER_TOO_SMALL;
            else
                memcpy( data, info->Data, info->DataLength );
        }
        NtClose( key );
    }

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        FIXME( "License key %s not found\n", debugstr_w(name->Buffer) );

    free( info );
    return status;
}
