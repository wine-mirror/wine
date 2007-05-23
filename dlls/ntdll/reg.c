/*
 * Registry functions
 *
 * Copyright (C) 1999 Juergen Schmied
 * Copyright (C) 2000 Alexandre Julliard
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
 * NOTES:
 * 	HKEY_LOCAL_MACHINE	\\REGISTRY\\MACHINE
 *	HKEY_USERS		\\REGISTRY\\USER
 *	HKEY_CURRENT_CONFIG	\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\HARDWARE PROFILES\\CURRENT
  *	HKEY_CLASSES		\\REGISTRY\\MACHINE\\SOFTWARE\\CLASSES
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/library.h"
#include "ntdll_misc.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(reg);

/* maximum length of a key/value name in bytes (without terminating null) */
#define MAX_NAME_LENGTH ((MAX_PATH-1) * sizeof(WCHAR))

/******************************************************************************
 * NtCreateKey [NTDLL.@]
 * ZwCreateKey [NTDLL.@]
 */
NTSTATUS WINAPI NtCreateKey( PHANDLE retkey, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                             ULONG TitleIndex, const UNICODE_STRING *class, ULONG options,
                             PULONG dispos )
{
    NTSTATUS ret;

    if (!retkey || !attr) return STATUS_ACCESS_VIOLATION;
    if (attr->Length > sizeof(OBJECT_ATTRIBUTES)) return STATUS_INVALID_PARAMETER;
    if (attr->ObjectName->Length > MAX_NAME_LENGTH) return STATUS_BUFFER_OVERFLOW;

    TRACE( "(%p,%s,%s,%x,%x,%p)\n", attr->RootDirectory, debugstr_us(attr->ObjectName),
           debugstr_us(class), options, access, retkey );

    SERVER_START_REQ( create_key )
    {
        req->parent     = attr->RootDirectory;
        req->access     = access;
        req->attributes = attr->Attributes;
        req->options    = options;
        req->modif      = 0;
        req->namelen    = attr->ObjectName->Length;
        wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        if (class) wine_server_add_data( req, class->Buffer, class->Length );
        if (!(ret = wine_server_call( req )))
        {
            *retkey = reply->hkey;
            if (dispos) *dispos = reply->created ? REG_CREATED_NEW_KEY : REG_OPENED_EXISTING_KEY;
        }
    }
    SERVER_END_REQ;
    TRACE("<- %p\n", *retkey);
    return ret;
}

/******************************************************************************
 *  RtlpNtCreateKey [NTDLL.@]
 *
 *  See NtCreateKey.
 */
NTSTATUS WINAPI RtlpNtCreateKey( PHANDLE retkey, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                                 ULONG TitleIndex, const UNICODE_STRING *class, ULONG options,
                                 PULONG dispos )
{
    OBJECT_ATTRIBUTES oa;

    if (attr)
    {
        memcpy( &oa, attr, sizeof oa );
        oa.Attributes &= ~(OBJ_PERMANENT|OBJ_EXCLUSIVE);
        attr = &oa;
    }

    return NtCreateKey(retkey, access, attr, 0, NULL, 0, dispos);
}

/******************************************************************************
 * NtOpenKey [NTDLL.@]
 * ZwOpenKey [NTDLL.@]
 *
 *   OUT	HANDLE			retkey (returns 0 when failure)
 *   IN		ACCESS_MASK		access
 *   IN		POBJECT_ATTRIBUTES 	attr
 */
NTSTATUS WINAPI NtOpenKey( PHANDLE retkey, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    NTSTATUS ret;
    DWORD len = attr->ObjectName->Length;

    TRACE( "(%p,%s,%x,%p)\n", attr->RootDirectory,
           debugstr_us(attr->ObjectName), access, retkey );

    if (len > MAX_NAME_LENGTH) return STATUS_BUFFER_OVERFLOW;
    if (!retkey) return STATUS_INVALID_PARAMETER;

    SERVER_START_REQ( open_key )
    {
        req->parent     = attr->RootDirectory;
        req->access     = access;
        req->attributes = attr->Attributes;
        wine_server_add_data( req, attr->ObjectName->Buffer, len );
        ret = wine_server_call( req );
        *retkey = reply->hkey;
    }
    SERVER_END_REQ;
    TRACE("<- %p\n", *retkey);
    return ret;
}

/******************************************************************************
 * RtlpNtOpenKey [NTDLL.@]
 *
 * See NtOpenKey.
 */
NTSTATUS WINAPI RtlpNtOpenKey( PHANDLE retkey, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr )
{
    if (attr)
        attr->Attributes &= ~(OBJ_PERMANENT|OBJ_EXCLUSIVE);
    return NtOpenKey(retkey, access, attr);
}

/******************************************************************************
 * NtDeleteKey [NTDLL.@]
 * ZwDeleteKey [NTDLL.@]
 */
NTSTATUS WINAPI NtDeleteKey( HANDLE hkey )
{
    NTSTATUS ret;

    TRACE( "(%p)\n", hkey );

    SERVER_START_REQ( delete_key )
    {
        req->hkey = hkey;
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 * RtlpNtMakeTemporaryKey [NTDLL.@]
 *
 *  See NtDeleteKey.
 */
NTSTATUS WINAPI RtlpNtMakeTemporaryKey( HANDLE hkey )
{
    return NtDeleteKey(hkey);
}

/******************************************************************************
 * NtDeleteValueKey [NTDLL.@]
 * ZwDeleteValueKey [NTDLL.@]
 */
NTSTATUS WINAPI NtDeleteValueKey( HANDLE hkey, const UNICODE_STRING *name )
{
    NTSTATUS ret;

    TRACE( "(%p,%s)\n", hkey, debugstr_us(name) );
    if (name->Length > MAX_NAME_LENGTH) return STATUS_BUFFER_OVERFLOW;

    SERVER_START_REQ( delete_key_value )
    {
        req->hkey = hkey;
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
    NTSTATUS ret;
    void *data_ptr;
    size_t fixed_size;

    switch(info_class)
    {
    case KeyBasicInformation: data_ptr = ((KEY_BASIC_INFORMATION *)info)->Name; break;
    case KeyFullInformation:  data_ptr = ((KEY_FULL_INFORMATION *)info)->Class; break;
    case KeyNodeInformation:  data_ptr = ((KEY_NODE_INFORMATION *)info)->Name;  break;
    default:
        FIXME( "Information class %d not implemented\n", info_class );
        return STATUS_INVALID_PARAMETER;
    }
    fixed_size = (char *)data_ptr - (char *)info;

    SERVER_START_REQ( enum_key )
    {
        req->hkey       = handle;
        req->index      = index;
        req->info_class = info_class;
        if (length > fixed_size) wine_server_set_reply( req, data_ptr, length - fixed_size );
        if (!(ret = wine_server_call( req )))
        {
            LARGE_INTEGER modif;

            RtlSecondsSince1970ToTime( reply->modif, &modif );

            switch(info_class)
            {
            case KeyBasicInformation:
                {
                    KEY_BASIC_INFORMATION keyinfo;
                    fixed_size = (char *)keyinfo.Name - (char *)&keyinfo;
                    keyinfo.LastWriteTime = modif;
                    keyinfo.TitleIndex = 0;
                    keyinfo.NameLength = reply->namelen;
                    memcpy( info, &keyinfo, min( length, fixed_size ) );
                }
                break;
            case KeyFullInformation:
                {
                    KEY_FULL_INFORMATION keyinfo;
                    fixed_size = (char *)keyinfo.Class - (char *)&keyinfo;
                    keyinfo.LastWriteTime = modif;
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
                }
                break;
            case KeyNodeInformation:
                {
                    KEY_NODE_INFORMATION keyinfo;
                    fixed_size = (char *)keyinfo.Name - (char *)&keyinfo;
                    keyinfo.LastWriteTime = modif;
                    keyinfo.TitleIndex = 0;
                    keyinfo.ClassLength = max( 0, wine_server_reply_size(reply) - reply->namelen );
                    keyinfo.ClassOffset = keyinfo.ClassLength ? fixed_size + reply->namelen : -1;
                    keyinfo.NameLength = reply->namelen;
                    memcpy( info, &keyinfo, min( length, fixed_size ) );
                }
                break;
            }
            *result_len = fixed_size + reply->total;
            if (length < *result_len) ret = STATUS_BUFFER_OVERFLOW;
        }
    }
    SERVER_END_REQ;
    return ret;
}



/******************************************************************************
 * NtEnumerateKey [NTDLL.@]
 * ZwEnumerateKey [NTDLL.@]
 *
 * NOTES
 *  the name copied into the buffer is NOT 0-terminated
 */
NTSTATUS WINAPI NtEnumerateKey( HANDLE handle, ULONG index, KEY_INFORMATION_CLASS info_class,
                                void *info, DWORD length, DWORD *result_len )
{
    /* -1 means query key, so avoid it here */
    if (index == (ULONG)-1) return STATUS_NO_MORE_ENTRIES;
    return enumerate_key( handle, index, info_class, info, length, result_len );
}


/******************************************************************************
 * RtlpNtEnumerateSubKey [NTDLL.@]
 *
 */
NTSTATUS WINAPI RtlpNtEnumerateSubKey( HANDLE handle, UNICODE_STRING *out, ULONG index )
{
  KEY_BASIC_INFORMATION *info;
  DWORD dwLen, dwResultLen;
  NTSTATUS ret;

  if (out->Length)
  {
    dwLen = out->Length + sizeof(KEY_BASIC_INFORMATION);
    info = (KEY_BASIC_INFORMATION*)RtlAllocateHeap( GetProcessHeap(), 0, dwLen );
    if (!info)
      return STATUS_NO_MEMORY;
  }
  else
  {
    dwLen = 0;
    info = NULL;
  }

  ret = NtEnumerateKey( handle, index, KeyBasicInformation, info, dwLen, &dwResultLen );
  dwResultLen -= sizeof(KEY_BASIC_INFORMATION);

  if (ret == STATUS_BUFFER_OVERFLOW)
    out->Length = dwResultLen;
  else if (!ret)
  {
    if (out->Length < info->NameLength)
    {
      out->Length = dwResultLen;
      ret = STATUS_BUFFER_OVERFLOW;
    }
    else
    {
      out->Length = info->NameLength;
      memcpy(out->Buffer, info->Name, info->NameLength);
    }
  }

  RtlFreeHeap( GetProcessHeap(), 0, info );
  return ret;
}

/******************************************************************************
 * NtQueryKey [NTDLL.@]
 * ZwQueryKey [NTDLL.@]
 */
NTSTATUS WINAPI NtQueryKey( HANDLE handle, KEY_INFORMATION_CLASS info_class,
                            void *info, DWORD length, DWORD *result_len )
{
    return enumerate_key( handle, -1, info_class, info, length, result_len );
}


/* fill the key value info structure for a specific info class */
static void copy_key_value_info( KEY_VALUE_INFORMATION_CLASS info_class, void *info,
                                 DWORD length, int type, int name_len, int data_len )
{
    switch(info_class)
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
    default:
        break;
    }
}


/******************************************************************************
 *  NtEnumerateValueKey	[NTDLL.@]
 *  ZwEnumerateValueKey [NTDLL.@]
 */
NTSTATUS WINAPI NtEnumerateValueKey( HANDLE handle, ULONG index,
                                     KEY_VALUE_INFORMATION_CLASS info_class,
                                     void *info, DWORD length, DWORD *result_len )
{
    NTSTATUS ret;
    void *ptr;
    size_t fixed_size;

    TRACE( "(%p,%u,%d,%p,%d)\n", handle, index, info_class, info, length );

    /* compute the length we want to retrieve */
    switch(info_class)
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
        req->hkey       = handle;
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
 * NtQueryValueKey [NTDLL.@]
 * ZwQueryValueKey [NTDLL.@]
 *
 * NOTES
 *  the name in the KeyValueInformation is never set
 */
NTSTATUS WINAPI NtQueryValueKey( HANDLE handle, const UNICODE_STRING *name,
                                 KEY_VALUE_INFORMATION_CLASS info_class,
                                 void *info, DWORD length, DWORD *result_len )
{
    NTSTATUS ret;
    UCHAR *data_ptr;
    unsigned int fixed_size = 0;

    TRACE( "(%p,%s,%d,%p,%d)\n", handle, debugstr_us(name), info_class, info, length );

    if (name->Length > MAX_NAME_LENGTH) return STATUS_BUFFER_OVERFLOW;

    /* compute the length we want to retrieve */
    switch(info_class)
    {
    case KeyValueBasicInformation:
        fixed_size = (char *)((KEY_VALUE_BASIC_INFORMATION *)info)->Name - (char *)info;
        data_ptr = NULL;
        break;
    case KeyValueFullInformation:
        data_ptr = (UCHAR *)((KEY_VALUE_FULL_INFORMATION *)info)->Name;
        fixed_size = (char *)data_ptr - (char *)info;
        break;
    case KeyValuePartialInformation:
        data_ptr = ((KEY_VALUE_PARTIAL_INFORMATION *)info)->Data;
        fixed_size = (char *)data_ptr - (char *)info;
        break;
    default:
        FIXME( "Information class %d not implemented\n", info_class );
        return STATUS_INVALID_PARAMETER;
    }

    SERVER_START_REQ( get_key_value )
    {
        req->hkey = handle;
        wine_server_add_data( req, name->Buffer, name->Length );
        if (length > fixed_size) wine_server_set_reply( req, data_ptr, length - fixed_size );
        if (!(ret = wine_server_call( req )))
        {
            copy_key_value_info( info_class, info, length, reply->type,
                                 0, wine_server_reply_size(reply) );
            *result_len = fixed_size + reply->total;
            if (length < *result_len) ret = STATUS_BUFFER_OVERFLOW;
        }
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 * RtlpNtQueryValueKey [NTDLL.@]
 *
 */
NTSTATUS WINAPI RtlpNtQueryValueKey( HANDLE handle, ULONG *result_type, PBYTE dest,
                                     DWORD *result_len )
{
    KEY_VALUE_PARTIAL_INFORMATION *info;
    UNICODE_STRING name;
    NTSTATUS ret;
    DWORD dwResultLen;
    DWORD dwLen = sizeof (KEY_VALUE_PARTIAL_INFORMATION) + (result_len ? *result_len : 0);

    info = (KEY_VALUE_PARTIAL_INFORMATION*)RtlAllocateHeap( GetProcessHeap(), 0, dwLen );
    if (!info)
      return STATUS_NO_MEMORY;

    name.Length = 0;
    ret = NtQueryValueKey( handle, &name, KeyValuePartialInformation, info, dwLen, &dwResultLen );

    if (!ret || ret == STATUS_BUFFER_OVERFLOW)
    {
        if (result_len)
            *result_len = info->DataLength;

        if (result_type)
            *result_type = info->Type;

        if (ret != STATUS_BUFFER_OVERFLOW)
            memcpy( dest, info->Data, info->DataLength );
    }

    RtlFreeHeap( GetProcessHeap(), 0, info );
    return ret;
}

/******************************************************************************
 *  NtFlushKey	[NTDLL.@]
 *  ZwFlushKey  [NTDLL.@]
 */
NTSTATUS WINAPI NtFlushKey(HANDLE key)
{
    NTSTATUS ret;

    TRACE("key=%p\n", key);

    SERVER_START_REQ( flush_key )
    {
	req->hkey = key;
	ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    
    return ret;
}

/******************************************************************************
 *  NtLoadKey	[NTDLL.@]
 *  ZwLoadKey   [NTDLL.@]
 */
NTSTATUS WINAPI NtLoadKey( const OBJECT_ATTRIBUTES *attr, OBJECT_ATTRIBUTES *file )
{
    NTSTATUS ret;
    HANDLE hive;
    IO_STATUS_BLOCK io;

    TRACE("(%p,%p)\n", attr, file);

    ret = NtCreateFile(&hive, GENERIC_READ, file, &io, NULL, FILE_ATTRIBUTE_NORMAL, 0,
                       FILE_OPEN, 0, NULL, 0);
    if (ret) return ret;

    SERVER_START_REQ( load_registry )
    {
        req->hkey = attr->RootDirectory;
        req->file = hive;
        wine_server_add_data(req, attr->ObjectName->Buffer, attr->ObjectName->Length);
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;

    NtClose(hive);
   
    return ret;
}

/******************************************************************************
 *  NtNotifyChangeKey	[NTDLL.@]
 *  ZwNotifyChangeKey   [NTDLL.@]
 */
NTSTATUS WINAPI NtNotifyChangeKey(
	IN HANDLE KeyHandle,
	IN HANDLE Event,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG CompletionFilter,
	IN BOOLEAN Asynchronous,
	OUT PVOID ChangeBuffer,
	IN ULONG Length,
	IN BOOLEAN WatchSubtree)
{
    NTSTATUS ret;

    TRACE("(%p,%p,%p,%p,%p,0x%08x, 0x%08x,%p,0x%08x,0x%08x)\n",
        KeyHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter,
        Asynchronous, ChangeBuffer, Length, WatchSubtree);

    if (ApcRoutine || ApcContext || ChangeBuffer || Length)
        FIXME("Unimplemented optional parameter\n");

    if (!Asynchronous)
    {
        OBJECT_ATTRIBUTES attr;
        InitializeObjectAttributes( &attr, NULL, 0, NULL, NULL );
        ret = NtCreateEvent( &Event, EVENT_ALL_ACCESS, &attr, FALSE, FALSE );
        if (ret != STATUS_SUCCESS)
            return ret;
    }

    SERVER_START_REQ( set_registry_notification )
    {
        req->hkey    = KeyHandle;
        req->event   = Event;
        req->subtree = WatchSubtree;
        req->filter  = CompletionFilter;
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
 
    if (!Asynchronous)
    {
        if (ret == STATUS_SUCCESS)
            NtWaitForSingleObject( Event, FALSE, NULL );
        NtClose( Event );
    }

    return STATUS_SUCCESS;
}

/******************************************************************************
 * NtQueryMultipleValueKey [NTDLL]
 * ZwQueryMultipleValueKey
 */

NTSTATUS WINAPI NtQueryMultipleValueKey(
	HANDLE KeyHandle,
	PKEY_MULTIPLE_VALUE_INFORMATION ListOfValuesToQuery,
	ULONG NumberOfItems,
	PVOID MultipleValueInformation,
	ULONG Length,
	PULONG  ReturnLength)
{
	FIXME("(%p,%p,0x%08x,%p,0x%08x,%p) stub!\n",
	KeyHandle, ListOfValuesToQuery, NumberOfItems, MultipleValueInformation,
	Length,ReturnLength);
	return STATUS_SUCCESS;
}

/******************************************************************************
 * NtReplaceKey [NTDLL.@]
 * ZwReplaceKey [NTDLL.@]
 */
NTSTATUS WINAPI NtReplaceKey(
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE Key,
	IN POBJECT_ATTRIBUTES ReplacedObjectAttributes)
{
	FIXME("(%p),stub!\n", Key);
	dump_ObjectAttributes(ObjectAttributes);
	dump_ObjectAttributes(ReplacedObjectAttributes);
	return STATUS_SUCCESS;
}
/******************************************************************************
 * NtRestoreKey [NTDLL.@]
 * ZwRestoreKey [NTDLL.@]
 */
NTSTATUS WINAPI NtRestoreKey(
	HANDLE KeyHandle,
	HANDLE FileHandle,
	ULONG RestoreFlags)
{
	FIXME("(%p,%p,0x%08x) stub\n",
	KeyHandle, FileHandle, RestoreFlags);
	return STATUS_SUCCESS;
}
/******************************************************************************
 * NtSaveKey [NTDLL.@]
 * ZwSaveKey [NTDLL.@]
 */
NTSTATUS WINAPI NtSaveKey(IN HANDLE KeyHandle, IN HANDLE FileHandle)
{
    NTSTATUS ret;

    TRACE("(%p,%p)\n", KeyHandle, FileHandle);

    SERVER_START_REQ( save_registry )
    {
        req->hkey = KeyHandle;
        req->file = FileHandle;
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;

    return ret;
}
/******************************************************************************
 * NtSetInformationKey [NTDLL.@]
 * ZwSetInformationKey [NTDLL.@]
 */
NTSTATUS WINAPI NtSetInformationKey(
	IN HANDLE KeyHandle,
	IN const int KeyInformationClass,
	IN PVOID KeyInformation,
	IN ULONG KeyInformationLength)
{
	FIXME("(%p,0x%08x,%p,0x%08x) stub\n",
	KeyHandle, KeyInformationClass, KeyInformation, KeyInformationLength);
	return STATUS_SUCCESS;
}


/******************************************************************************
 * NtSetValueKey [NTDLL.@]
 * ZwSetValueKey [NTDLL.@]
 *
 * NOTES
 *   win95 does not care about count for REG_SZ and finds out the len by itself (js)
 *   NT does definitely care (aj)
 */
NTSTATUS WINAPI NtSetValueKey( HANDLE hkey, const UNICODE_STRING *name, ULONG TitleIndex,
                               ULONG type, const void *data, ULONG count )
{
    NTSTATUS ret;

    TRACE( "(%p,%s,%d,%p,%d)\n", hkey, debugstr_us(name), type, data, count );

    if (name->Length > MAX_NAME_LENGTH) return STATUS_BUFFER_OVERFLOW;

    SERVER_START_REQ( set_key_value )
    {
        req->hkey    = hkey;
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
 * RtlpNtSetValueKey [NTDLL.@]
 *
 */
NTSTATUS WINAPI RtlpNtSetValueKey( HANDLE hkey, ULONG type, const void *data,
                                   ULONG count )
{
    UNICODE_STRING name;

    name.Length = 0;
    return NtSetValueKey( hkey, &name, 0, type, data, count );
}

/******************************************************************************
 * NtUnloadKey [NTDLL.@]
 * ZwUnloadKey [NTDLL.@]
 */
NTSTATUS WINAPI NtUnloadKey(IN POBJECT_ATTRIBUTES attr)
{
    NTSTATUS ret;

    TRACE("(%p)\n", attr);

    SERVER_START_REQ( unload_registry )
    {
        req->hkey = attr->RootDirectory;
        ret = wine_server_call(req);
    }
    SERVER_END_REQ;

    return ret;
}

/******************************************************************************
 *  RtlFormatCurrentUserKeyPath		[NTDLL.@]
 *
 */
NTSTATUS WINAPI RtlFormatCurrentUserKeyPath( IN OUT PUNICODE_STRING KeyPath)
{
    static const WCHAR pathW[] = {'\\','R','e','g','i','s','t','r','y','\\','U','s','e','r','\\'};
    HANDLE token;
    NTSTATUS status;

    status = NtOpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &token);
    if (status == STATUS_NO_TOKEN)
        status = NtOpenProcessToken(GetCurrentProcess(), TOKEN_READ, &token);
    if (status == STATUS_SUCCESS)
    {
        char buffer[sizeof(TOKEN_USER) + sizeof(SID) + sizeof(DWORD)*SID_MAX_SUB_AUTHORITIES];
        DWORD len = sizeof(buffer);

        status = NtQueryInformationToken(token, TokenUser, buffer, len, &len);
        if (status == STATUS_SUCCESS)
        {
            KeyPath->MaximumLength = 0;
            status = RtlConvertSidToUnicodeString(KeyPath, ((TOKEN_USER *)buffer)->User.Sid, FALSE);
            if (status == STATUS_BUFFER_OVERFLOW)
            {
                PWCHAR buf = RtlAllocateHeap(GetProcessHeap(), 0,
                                             sizeof(pathW) + KeyPath->Length + sizeof(WCHAR));
                if (buf)
                {
                    memcpy(buf, pathW, sizeof(pathW));
                    KeyPath->MaximumLength = KeyPath->Length + sizeof(WCHAR);
                    KeyPath->Buffer = (PWCHAR)((LPBYTE)buf + sizeof(pathW));
                    status = RtlConvertSidToUnicodeString(KeyPath,
                                                          ((TOKEN_USER *)buffer)->User.Sid, FALSE);
                    KeyPath->Buffer = (PWCHAR)buf;
                    KeyPath->Length += sizeof(pathW);
                    KeyPath->MaximumLength += sizeof(pathW);
                }
                else
                    status = STATUS_NO_MEMORY;
            }
        }
        NtClose(token);
    }
    return status;
}

/******************************************************************************
 *  RtlOpenCurrentUser		[NTDLL.@]
 *
 * NOTES
 *  If we return just HKEY_CURRENT_USER the advapi tries to find a remote
 *  registry (odd handle) and fails.
 */
NTSTATUS WINAPI RtlOpenCurrentUser(
	IN ACCESS_MASK DesiredAccess, /* [in] */
	OUT PHANDLE KeyHandle)	      /* [out] handle of HKEY_CURRENT_USER */
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING ObjectName;
	NTSTATUS ret;

	TRACE("(0x%08x, %p)\n",DesiredAccess, KeyHandle);

        if ((ret = RtlFormatCurrentUserKeyPath(&ObjectName))) return ret;
	InitializeObjectAttributes(&ObjectAttributes,&ObjectName,OBJ_CASE_INSENSITIVE,0, NULL);
	ret = NtCreateKey(KeyHandle, DesiredAccess, &ObjectAttributes, 0, NULL, 0, NULL);
	RtlFreeUnicodeString(&ObjectName);
	return ret;
}


static NTSTATUS RTL_ReportRegistryValue(PKEY_VALUE_FULL_INFORMATION pInfo,
                                        PRTL_QUERY_REGISTRY_TABLE pQuery, PVOID pContext, PVOID pEnvironment)
{
    PUNICODE_STRING str;
    UNICODE_STRING src, dst;
    LONG *bin;
    ULONG offset;
    PWSTR wstr;
    DWORD res;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG len;
    LPWSTR String;
    INT count = 0;

    if (pInfo == NULL)
    {
        if (pQuery->Flags & RTL_QUERY_REGISTRY_DIRECT)
            return STATUS_INVALID_PARAMETER;
        else
        {
            status = pQuery->QueryRoutine(pQuery->Name, pQuery->DefaultType, pQuery->DefaultData,
                                          pQuery->DefaultLength, pContext, pQuery->EntryContext);
        }
        return status;
    }
    len = pInfo->DataLength;

    if (pQuery->Flags & RTL_QUERY_REGISTRY_DIRECT)
    {
        str = (PUNICODE_STRING)pQuery->EntryContext;
 
        switch(pInfo->Type)
        {
        case REG_EXPAND_SZ:
            if (!(pQuery->Flags & RTL_QUERY_REGISTRY_NOEXPAND))
            {
                RtlInitUnicodeString(&src, (WCHAR*)(((CHAR*)pInfo) + pInfo->DataOffset));
                res = 0;
                dst.MaximumLength = 0;
                RtlExpandEnvironmentStrings_U(pEnvironment, &src, &dst, &res);
                dst.Length = 0;
                dst.MaximumLength = res;
                dst.Buffer = RtlAllocateHeap(GetProcessHeap(), 0, res * sizeof(WCHAR));
                RtlExpandEnvironmentStrings_U(pEnvironment, &src, &dst, &res);
                status = pQuery->QueryRoutine(pQuery->Name, pInfo->Type, dst.Buffer,
                                     dst.Length, pContext, pQuery->EntryContext);
                RtlFreeHeap(GetProcessHeap(), 0, dst.Buffer);
            }

        case REG_SZ:
        case REG_LINK:
            if (str->Buffer == NULL)
                RtlCreateUnicodeString(str, (WCHAR*)(((CHAR*)pInfo) + pInfo->DataOffset));
            else
                RtlAppendUnicodeToString(str, (WCHAR*)(((CHAR*)pInfo) + pInfo->DataOffset));
            break;

        case REG_MULTI_SZ:
            if (!(pQuery->Flags & RTL_QUERY_REGISTRY_NOEXPAND))
                return STATUS_INVALID_PARAMETER;

            if (str->Buffer == NULL)
            {
                str->Buffer = RtlAllocateHeap(GetProcessHeap(), 0, len);
                str->MaximumLength = len;
            }
            len = min(len, str->MaximumLength);
            memcpy(str->Buffer, ((CHAR*)pInfo) + pInfo->DataOffset, len);
            str->Length = len;
            break;

        default:
            bin = (LONG*)pQuery->EntryContext;
            if (pInfo->DataLength <= sizeof(ULONG))
                memcpy(bin, ((CHAR*)pInfo) + pInfo->DataOffset,
                    pInfo->DataLength);
            else
            {
                if (bin[0] <= sizeof(ULONG))
                {
                    memcpy(&bin[1], ((CHAR*)pInfo) + pInfo->DataOffset,
                    min(-bin[0], pInfo->DataLength));
                }
                else
                {
                   len = min(bin[0], pInfo->DataLength);
                    bin[1] = len;
                    bin[2] = pInfo->Type;
                    memcpy(&bin[3], ((CHAR*)pInfo) + pInfo->DataOffset, len);
                }
           }
           break;
        }
    }
    else
    {
        if((pQuery->Flags & RTL_QUERY_REGISTRY_NOEXPAND) ||
           (pInfo->Type != REG_EXPAND_SZ && pInfo->Type != REG_MULTI_SZ))
        {
            status = pQuery->QueryRoutine(pInfo->Name, pInfo->Type,
                ((CHAR*)pInfo) + pInfo->DataOffset, pInfo->DataLength,
                pContext, pQuery->EntryContext);
        }
        else if (pInfo->Type == REG_EXPAND_SZ)
        {
            RtlInitUnicodeString(&src, (WCHAR*)(((CHAR*)pInfo) + pInfo->DataOffset));
            res = 0;
            dst.MaximumLength = 0;
            RtlExpandEnvironmentStrings_U(pEnvironment, &src, &dst, &res);
            dst.Length = 0;
            dst.MaximumLength = res;
            dst.Buffer = RtlAllocateHeap(GetProcessHeap(), 0, res * sizeof(WCHAR));
            RtlExpandEnvironmentStrings_U(pEnvironment, &src, &dst, &res);
            status = pQuery->QueryRoutine(pQuery->Name, pInfo->Type, dst.Buffer,
                                          dst.Length, pContext, pQuery->EntryContext);
            RtlFreeHeap(GetProcessHeap(), 0, dst.Buffer);
        }
        else /* REG_MULTI_SZ */
        {
            if(pQuery->Flags & RTL_QUERY_REGISTRY_NOEXPAND)
            {
                for (offset = 0; offset <= pInfo->DataLength; offset += len + sizeof(WCHAR))
                    {
                    wstr = (WCHAR*)(((CHAR*)pInfo) + offset);
                    len = strlenW(wstr) * sizeof(WCHAR);
                    status = pQuery->QueryRoutine(pQuery->Name, pInfo->Type, wstr, len,
                        pContext, pQuery->EntryContext);
                    if(status != STATUS_SUCCESS && status != STATUS_BUFFER_TOO_SMALL)
                        return status;
                    }
            }
            else
            {
                while(count<=pInfo->DataLength)
                {
                    String = (WCHAR*)(((CHAR*)pInfo) + pInfo->DataOffset)+count;
                    count+=strlenW(String)+1;
                    RtlInitUnicodeString(&src, (WCHAR*)(((CHAR*)pInfo) + pInfo->DataOffset));
                    res = 0;
                    dst.MaximumLength = 0;
                    RtlExpandEnvironmentStrings_U(pEnvironment, &src, &dst, &res);
                    dst.Length = 0;
                    dst.MaximumLength = res;
                    dst.Buffer = RtlAllocateHeap(GetProcessHeap(), 0, res * sizeof(WCHAR));
                    RtlExpandEnvironmentStrings_U(pEnvironment, &src, &dst, &res);
                    status = pQuery->QueryRoutine(pQuery->Name, pInfo->Type, dst.Buffer,
                                                  dst.Length, pContext, pQuery->EntryContext);
                    RtlFreeHeap(GetProcessHeap(), 0, dst.Buffer);
                    if(status != STATUS_SUCCESS && status != STATUS_BUFFER_TOO_SMALL)
                        return status;
                }
            }
        }
    }
    return status;
}


static NTSTATUS RTL_GetKeyHandle(ULONG RelativeTo, PCWSTR Path, PHANDLE handle)
{
    UNICODE_STRING KeyString;
    OBJECT_ATTRIBUTES regkey;
    PCWSTR base;
    INT len;
    NTSTATUS status;

    static const WCHAR empty[] = {0};
    static const WCHAR control[] = {'\\','R','e','g','i','s','t','r','y','\\','M','a','c','h','i','n','e',
    '\\','S','y','s','t','e','m','\\','C','u','r','r','e','n','t',' ','C','o','n','t','r','o','l','S','e','t','\\',
    'C','o','n','t','r','o','l','\\',0};

    static const WCHAR devicemap[] = {'\\','R','e','g','i','s','t','r','y','\\','M','a','c','h','i','n','e','\\',
    'H','a','r','d','w','a','r','e','\\','D','e','v','i','c','e','M','a','p','\\',0};

    static const WCHAR services[] = {'\\','R','e','g','i','s','t','r','y','\\','M','a','c','h','i','n','e','\\',
    'S','y','s','t','e','m','\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
    'S','e','r','v','i','c','e','s','\\',0};

    static const WCHAR user[] = {'\\','R','e','g','i','s','t','r','y','\\','U','s','e','r','\\',
    'C','u','r','r','e','n','t','U','s','e','r','\\',0};

    static const WCHAR windows_nt[] = {'\\','R','e','g','i','s','t','r','y','\\','M','a','c','h','i','n','e','\\',
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s',' ','N','T','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',0};

    switch (RelativeTo & 0xff)
    {
    case RTL_REGISTRY_ABSOLUTE:
        base = empty;
        break;

    case RTL_REGISTRY_CONTROL:
        base = control;
        break;

    case RTL_REGISTRY_DEVICEMAP:
        base = devicemap;
        break;

    case RTL_REGISTRY_SERVICES:
        base = services;
        break;

    case RTL_REGISTRY_USER:
        base = user;
        break;

    case RTL_REGISTRY_WINDOWS_NT:
        base = windows_nt;
        break;

    default:
        return STATUS_INVALID_PARAMETER;
    }

    len = (strlenW(base) + strlenW(Path) + 1) * sizeof(WCHAR);
    KeyString.Buffer = RtlAllocateHeap(GetProcessHeap(), 0, len);
    if (KeyString.Buffer == NULL)
        return STATUS_NO_MEMORY;

    strcpyW(KeyString.Buffer, base);
    strcatW(KeyString.Buffer, Path);
    KeyString.Length = len - sizeof(WCHAR);
    KeyString.MaximumLength = len;
    InitializeObjectAttributes(&regkey, &KeyString, OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = NtOpenKey(handle, KEY_ALL_ACCESS, &regkey);
    RtlFreeHeap(GetProcessHeap(), 0, KeyString.Buffer);
    return status;
}

/*************************************************************************
 * RtlQueryRegistryValues   [NTDLL.@]
 *
 * Query multiple registry values with a signle call.
 *
 * PARAMS
 *  RelativeTo  [I] Registry path that Path refers to
 *  Path        [I] Path to key
 *  QueryTable  [I] Table of key values to query
 *  Context     [I] Paremeter to pass to the application defined QueryRoutine function
 *  Environment [I] Optional parameter to use when performing expantion
 *
 * RETURNS
 *  STATUS_SUCCESS or an appropriate NTSTATUS error code.
 */
NTSTATUS WINAPI RtlQueryRegistryValues(IN ULONG RelativeTo, IN PCWSTR Path,
                                       IN PRTL_QUERY_REGISTRY_TABLE QueryTable, IN PVOID Context,
                                       IN PVOID Environment OPTIONAL)
{
    UNICODE_STRING Value;
    HANDLE handle, topkey;
    PKEY_VALUE_FULL_INFORMATION pInfo = NULL;
    ULONG len, buflen = 0;
    NTSTATUS status=STATUS_SUCCESS, ret = STATUS_SUCCESS;
    INT i;

    TRACE("(%d, %s, %p, %p, %p)\n", RelativeTo, debugstr_w(Path), QueryTable, Context, Environment);

    if(Path == NULL)
        return STATUS_INVALID_PARAMETER;

    /* get a valid handle */
    if (RelativeTo & RTL_REGISTRY_HANDLE)
        topkey = handle = (HANDLE)Path;
    else
    {
        status = RTL_GetKeyHandle(RelativeTo, Path, &topkey);
        handle = topkey;
    }
    if(status != STATUS_SUCCESS)
        return status;

    /* Process query table entries */
    for (; QueryTable->QueryRoutine != NULL || QueryTable->Name != NULL; ++QueryTable)
    {
        if (QueryTable->Flags &
            (RTL_QUERY_REGISTRY_SUBKEY | RTL_QUERY_REGISTRY_TOPKEY))
        {
            /* topkey must be kept open just in case we will reuse it later */
            if (handle != topkey)
                NtClose(handle);

            if (QueryTable->Flags & RTL_QUERY_REGISTRY_SUBKEY)
            {
                handle = 0;
                status = RTL_GetKeyHandle(PtrToUlong(QueryTable->Name), Path, &handle);
                if(status != STATUS_SUCCESS)
                {
                    ret = status;
                    goto out;
                }
            }
            else
                handle = topkey;
        }

        if (QueryTable->Flags & RTL_QUERY_REGISTRY_NOVALUE)
        {
            QueryTable->QueryRoutine(QueryTable->Name, REG_NONE, NULL, 0,
                Context, QueryTable->EntryContext);
            continue;
        }

        if (!handle)
        {
            if (QueryTable->Flags & RTL_QUERY_REGISTRY_REQUIRED)
            {
                ret = STATUS_OBJECT_NAME_NOT_FOUND;
                goto out;
            }
            continue;
        }

        if (QueryTable->Name == NULL)
        {
            if (QueryTable->Flags & RTL_QUERY_REGISTRY_DIRECT)
            {
                ret = STATUS_INVALID_PARAMETER;
                goto out;
            }

            /* Report all subkeys */
            for (i = 0;; ++i)
            {
                status = NtEnumerateValueKey(handle, i,
                    KeyValueFullInformation, pInfo, buflen, &len);
                if (status == STATUS_NO_MORE_ENTRIES)
                    break;
                if (status == STATUS_BUFFER_OVERFLOW ||
                    status == STATUS_BUFFER_TOO_SMALL)
                {
                    buflen = len;
                    RtlFreeHeap(GetProcessHeap(), 0, pInfo);
                    pInfo = (KEY_VALUE_FULL_INFORMATION*)RtlAllocateHeap(
                        GetProcessHeap(), 0, buflen);
                    NtEnumerateValueKey(handle, i, KeyValueFullInformation,
                        pInfo, buflen, &len);
                }

                status = RTL_ReportRegistryValue(pInfo, QueryTable, Context, Environment);
                if(status != STATUS_SUCCESS && status != STATUS_BUFFER_TOO_SMALL)
                {
                    ret = status;
                    goto out;
                }
                if (QueryTable->Flags & RTL_QUERY_REGISTRY_DELETE)
                {
                    RtlInitUnicodeString(&Value, pInfo->Name);
                    NtDeleteValueKey(handle, &Value);
                }
            }

            if (i == 0  && (QueryTable->Flags & RTL_QUERY_REGISTRY_REQUIRED))
            {
                ret = STATUS_OBJECT_NAME_NOT_FOUND;
                goto out;
            }
        }
        else
        {
            RtlInitUnicodeString(&Value, QueryTable->Name);
            status = NtQueryValueKey(handle, &Value, KeyValueFullInformation,
                pInfo, buflen, &len);
            if (status == STATUS_BUFFER_OVERFLOW ||
                status == STATUS_BUFFER_TOO_SMALL)
            {
                buflen = len;
                RtlFreeHeap(GetProcessHeap(), 0, pInfo);
                pInfo = (KEY_VALUE_FULL_INFORMATION*)RtlAllocateHeap(
                    GetProcessHeap(), 0, buflen);
                status = NtQueryValueKey(handle, &Value,
                    KeyValueFullInformation, pInfo, buflen, &len);
            }
            if (status != STATUS_SUCCESS)
            {
                if (QueryTable->Flags & RTL_QUERY_REGISTRY_REQUIRED)
                {
                    ret = STATUS_OBJECT_NAME_NOT_FOUND;
                    goto out;
                }
                status = RTL_ReportRegistryValue(NULL, QueryTable, Context, Environment);
                if(status != STATUS_SUCCESS && status != STATUS_BUFFER_TOO_SMALL)
                {
                    ret = status;
                    goto out;
                }
            }
            else
            {
                status = RTL_ReportRegistryValue(pInfo, QueryTable, Context, Environment);
                if(status != STATUS_SUCCESS && status != STATUS_BUFFER_TOO_SMALL)
                {
                    ret = status;
                    goto out;
                }
                if (QueryTable->Flags & RTL_QUERY_REGISTRY_DELETE)
                    NtDeleteValueKey(handle, &Value);
            }
        }
    }

out:
    RtlFreeHeap(GetProcessHeap(), 0, pInfo);
    if (handle != topkey)
        NtClose(handle);
    NtClose(topkey);
    return ret;
}

/*************************************************************************
 * RtlCheckRegistryKey   [NTDLL.@]
 *
 * Query multiple registry values with a signle call.
 *
 * PARAMS
 *  RelativeTo [I] Registry path that Path refers to
 *  Path       [I] Path to key
 *
 * RETURNS
 *  STATUS_SUCCESS if the specified key exists, or an NTSTATUS error code.
 */
NTSTATUS WINAPI RtlCheckRegistryKey(IN ULONG RelativeTo, IN PWSTR Path)
{
    HANDLE handle;
    NTSTATUS status;

    TRACE("(%d, %s)\n", RelativeTo, debugstr_w(Path));

    if((!RelativeTo) && Path == NULL)
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    if(RelativeTo & RTL_REGISTRY_HANDLE)
        return STATUS_SUCCESS;

    status = RTL_GetKeyHandle(RelativeTo, Path, &handle);
    if (handle) NtClose(handle);
    if (status == STATUS_INVALID_HANDLE) status = STATUS_OBJECT_NAME_NOT_FOUND;
    return status;
}

/*************************************************************************
 * RtlDeleteRegistryValue   [NTDLL.@]
 *
 * Query multiple registry values with a signle call.
 *
 * PARAMS
 *  RelativeTo [I] Registry path that Path refers to
 *  Path       [I] Path to key
 *  ValueName  [I] Name of the value to delete
 *
 * RETURNS
 *  STATUS_SUCCESS if the specified key is successfully deleted, or an NTSTATUS error code.
 */
NTSTATUS WINAPI RtlDeleteRegistryValue(IN ULONG RelativeTo, IN PCWSTR Path, IN PCWSTR ValueName)
{
    NTSTATUS status;
    HANDLE handle;
    UNICODE_STRING Value;

    TRACE("(%d, %s, %s)\n", RelativeTo, debugstr_w(Path), debugstr_w(ValueName));

    RtlInitUnicodeString(&Value, ValueName);
    if(RelativeTo == RTL_REGISTRY_HANDLE)
    {
        return NtDeleteValueKey((HANDLE)Path, &Value);
    }
    status = RTL_GetKeyHandle(RelativeTo, Path, &handle);
    if (status) return status;
    status = NtDeleteValueKey(handle, &Value);
    NtClose(handle);
    return status;
}

/*************************************************************************
 * RtlWriteRegistryValue   [NTDLL.@]
 *
 * Sets the registry value with provided data.
 *
 * PARAMS
 *  RelativeTo [I] Registry path that path parameter refers to
 *  path       [I] Path to the key (or handle - see RTL_GetKeyHandle)
 *  name       [I] Name of the registry value to set
 *  type       [I] Type of the registry key to set
 *  data       [I] Pointer to the user data to be set
 *  length     [I] Length of the user data pointed by data
 *
 * RETURNS
 *  STATUS_SUCCESS if the specified key is successfully set,
 *  or an NTSTATUS error code.
 */
NTSTATUS WINAPI RtlWriteRegistryValue( ULONG RelativeTo, PCWSTR path, PCWSTR name,
                                       ULONG type, PVOID data, ULONG length )
{
    HANDLE hkey;
    NTSTATUS status;
    UNICODE_STRING str;

    TRACE( "(%d, %s, %s) -> %d: %p [%d]\n", RelativeTo, debugstr_w(path), debugstr_w(name),
           type, data, length );

    RtlInitUnicodeString( &str, name );

    if (RelativeTo == RTL_REGISTRY_HANDLE)
        return NtSetValueKey( (HANDLE)path, &str, 0, type, data, length );

    status = RTL_GetKeyHandle( RelativeTo, path, &hkey );
    if (status != STATUS_SUCCESS) return status;

    status = NtSetValueKey( hkey, &str, 0, type, data, length );
    NtClose( hkey );

    return status;
}
