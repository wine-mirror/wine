/*
 * Registry functions
 *
 * Copyright (C) 1999 Juergen Schmied
 * Copyright (C) 2000 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTES:
 * 	HKEY_LOCAL_MACHINE	\\REGISTRY\\MACHINE
 *	HKEY_USERS		\\REGISTRY\\USER
 *	HKEY_CURRENT_CONFIG	\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\HARDWARE PROFILES\\CURRENT
  *	HKEY_CLASSES		\\REGISTRY\\MACHINE\\SOFTWARE\\CLASSES
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <string.h>

#include "winerror.h"
#include "wine/unicode.h"
#include "wine/library.h"
#include "wine/server.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(reg);

/* maximum length of a key/value name in bytes (without terminating null) */
#define MAX_NAME_LENGTH ((MAX_PATH-1) * sizeof(WCHAR))


/******************************************************************************
 * NtCreateKey [NTDLL.@]
 * ZwCreateKey [NTDLL.@]
 */
NTSTATUS WINAPI NtCreateKey( PHKEY retkey, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                             ULONG TitleIndex, const UNICODE_STRING *class, ULONG options,
                             PULONG dispos )
{
    NTSTATUS ret;

    TRACE( "(0x%p,%s,%s,%lx,%lx,%p)\n", attr->RootDirectory, debugstr_us(attr->ObjectName),
           debugstr_us(class), options, access, retkey );

    if (attr->ObjectName->Length > MAX_NAME_LENGTH) return STATUS_BUFFER_OVERFLOW;
    if (!retkey) return STATUS_INVALID_PARAMETER;

    SERVER_START_REQ( create_key )
    {
        req->parent  = attr->RootDirectory;
        req->access  = access;
        req->options = options;
        req->modif   = 0;
        req->namelen = attr->ObjectName->Length;
        wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        if (class) wine_server_add_data( req, class->Buffer, class->Length );
        if (!(ret = wine_server_call( req )))
        {
            *retkey = reply->hkey;
            if (dispos) *dispos = reply->created ? REG_CREATED_NEW_KEY : REG_OPENED_EXISTING_KEY;
        }
    }
    SERVER_END_REQ;
    TRACE("<- 0x%p\n", *retkey);
    return ret;
}


/******************************************************************************
 * NtOpenKey [NTDLL.@]
 * ZwOpenKey [NTDLL.@]
 *
 *   OUT	PHKEY			retkey (returns 0 when failure)
 *   IN		ACCESS_MASK		access
 *   IN		POBJECT_ATTRIBUTES 	attr
 */
NTSTATUS WINAPI NtOpenKey( PHKEY retkey, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    NTSTATUS ret;
    DWORD len = attr->ObjectName->Length;

    TRACE( "(0x%p,%s,%lx,%p)\n", attr->RootDirectory,
           debugstr_us(attr->ObjectName), access, retkey );

    if (len > MAX_NAME_LENGTH) return STATUS_BUFFER_OVERFLOW;
    if (!retkey) return STATUS_INVALID_PARAMETER;

    SERVER_START_REQ( open_key )
    {
        req->parent = attr->RootDirectory;
        req->access = access;
        wine_server_add_data( req, attr->ObjectName->Buffer, len );
        ret = wine_server_call( req );
        *retkey = reply->hkey;
    }
    SERVER_END_REQ;
    TRACE("<- 0x%p\n", *retkey);
    return ret;
}


/******************************************************************************
 * NtDeleteKey [NTDLL.@]
 * ZwDeleteKey [NTDLL.@]
 */
NTSTATUS WINAPI NtDeleteKey( HKEY hkey )
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
 * NtDeleteValueKey [NTDLL.@]
 * ZwDeleteValueKey [NTDLL.@]
 */
NTSTATUS WINAPI NtDeleteValueKey( HKEY hkey, const UNICODE_STRING *name )
{
    NTSTATUS ret;

    TRACE( "(0x%p,%s)\n", hkey, debugstr_us(name) );
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
static NTSTATUS enumerate_key( HKEY handle, int index, KEY_INFORMATION_CLASS info_class,
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
NTSTATUS WINAPI NtEnumerateKey( HKEY handle, ULONG index, KEY_INFORMATION_CLASS info_class,
                                void *info, DWORD length, DWORD *result_len )
{
    /* -1 means query key, so avoid it here */
    if (index == (ULONG)-1) return STATUS_NO_MORE_ENTRIES;
    return enumerate_key( handle, index, info_class, info, length, result_len );
}


/******************************************************************************
 * NtQueryKey [NTDLL.@]
 * ZwQueryKey [NTDLL.@]
 */
NTSTATUS WINAPI NtQueryKey( HKEY handle, KEY_INFORMATION_CLASS info_class,
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
NTSTATUS WINAPI NtEnumerateValueKey( HKEY handle, ULONG index,
                                     KEY_VALUE_INFORMATION_CLASS info_class,
                                     void *info, DWORD length, DWORD *result_len )
{
    NTSTATUS ret;
    void *ptr;
    size_t fixed_size;

    TRACE( "(0x%p,%lu,%d,%p,%ld)\n", handle, index, info_class, info, length );

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
NTSTATUS WINAPI NtQueryValueKey( HKEY handle, const UNICODE_STRING *name,
                                 KEY_VALUE_INFORMATION_CLASS info_class,
                                 void *info, DWORD length, DWORD *result_len )
{
    NTSTATUS ret;
    UCHAR *data_ptr;
    int fixed_size = 0;

    TRACE( "(0x%p,%s,%d,%p,%ld)\n", handle, debugstr_us(name), info_class, info, length );

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
 *  NtFlushKey	[NTDLL.@]
 *  ZwFlushKey  [NTDLL.@]
 */
NTSTATUS WINAPI NtFlushKey(HKEY KeyHandle)
{
	FIXME("(0x%p) stub!\n",
	KeyHandle);
	return 1;
}

/******************************************************************************
 *  NtLoadKey	[NTDLL.@]
 *  ZwLoadKey   [NTDLL.@]
 */
NTSTATUS WINAPI NtLoadKey( const OBJECT_ATTRIBUTES *attr, const OBJECT_ATTRIBUTES *file )
{
    FIXME("stub!\n");
    dump_ObjectAttributes(attr);
    dump_ObjectAttributes(file);
    return STATUS_SUCCESS;
}

/******************************************************************************
 *  NtNotifyChangeKey	[NTDLL.@]
 *  ZwNotifyChangeKey   [NTDLL.@]
 */
NTSTATUS WINAPI NtNotifyChangeKey(
	IN HKEY KeyHandle,
	IN HANDLE Event,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG CompletionFilter,
	IN BOOLEAN Asynchroneous,
	OUT PVOID ChangeBuffer,
	IN ULONG Length,
	IN BOOLEAN WatchSubtree)
{
	FIXME("(0x%p,0x%p,%p,%p,%p,0x%08lx, 0x%08x,%p,0x%08lx,0x%08x) stub!\n",
	KeyHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter,
	Asynchroneous, ChangeBuffer, Length, WatchSubtree);
	return STATUS_SUCCESS;
}

/******************************************************************************
 * NtQueryMultipleValueKey [NTDLL]
 * ZwQueryMultipleValueKey
 */

NTSTATUS WINAPI NtQueryMultipleValueKey(
	HKEY KeyHandle,
	PVALENTW ListOfValuesToQuery,
	ULONG NumberOfItems,
	PVOID MultipleValueInformation,
	ULONG Length,
	PULONG  ReturnLength)
{
	FIXME("(0x%p,%p,0x%08lx,%p,0x%08lx,%p) stub!\n",
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
	IN HKEY Key,
	IN POBJECT_ATTRIBUTES ReplacedObjectAttributes)
{
	FIXME("(0x%p),stub!\n", Key);
	dump_ObjectAttributes(ObjectAttributes);
	dump_ObjectAttributes(ReplacedObjectAttributes);
	return STATUS_SUCCESS;
}
/******************************************************************************
 * NtRestoreKey [NTDLL.@]
 * ZwRestoreKey [NTDLL.@]
 */
NTSTATUS WINAPI NtRestoreKey(
	HKEY KeyHandle,
	HANDLE FileHandle,
	ULONG RestoreFlags)
{
	FIXME("(0x%p,0x%p,0x%08lx) stub\n",
	KeyHandle, FileHandle, RestoreFlags);
	return STATUS_SUCCESS;
}
/******************************************************************************
 * NtSaveKey [NTDLL.@]
 * ZwSaveKey [NTDLL.@]
 */
NTSTATUS WINAPI NtSaveKey(
	IN HKEY KeyHandle,
	IN HANDLE FileHandle)
{
	FIXME("(0x%p,0x%p) stub\n",
	KeyHandle, FileHandle);
	return STATUS_SUCCESS;
}
/******************************************************************************
 * NtSetInformationKey [NTDLL.@]
 * ZwSetInformationKey [NTDLL.@]
 */
NTSTATUS WINAPI NtSetInformationKey(
	IN HKEY KeyHandle,
	IN const int KeyInformationClass,
	IN PVOID KeyInformation,
	IN ULONG KeyInformationLength)
{
	FIXME("(0x%p,0x%08x,%p,0x%08lx) stub\n",
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
NTSTATUS WINAPI NtSetValueKey( HKEY hkey, const UNICODE_STRING *name, ULONG TitleIndex,
                               ULONG type, const void *data, ULONG count )
{
    NTSTATUS ret;

    TRACE( "(0x%p,%s,%ld,%p,%ld)\n", hkey, debugstr_us(name), type, data, count );

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
 * NtUnloadKey [NTDLL.@]
 * ZwUnloadKey [NTDLL.@]
 */
NTSTATUS WINAPI NtUnloadKey(
	IN HKEY KeyHandle)
{
	FIXME("(0x%p) stub\n",
	KeyHandle);
	return STATUS_SUCCESS;
}

/******************************************************************************
 *  RtlFormatCurrentUserKeyPath		[NTDLL.@]
 *
 * NOTE: under NT the user name part of the path is an SID.
 */
NTSTATUS WINAPI RtlFormatCurrentUserKeyPath( IN OUT PUNICODE_STRING KeyPath)
{
    const char *user = wine_get_user_name();
    char *buffer;
    ANSI_STRING AnsiPath;
    NTSTATUS ret;

    if (!(buffer = RtlAllocateHeap( GetProcessHeap(), 0, strlen(user)+16 )))
        return STATUS_NO_MEMORY;

    strcpy( buffer, "\\Registry\\User\\" );
    strcat( buffer, user );
    RtlInitAnsiString( &AnsiPath, buffer );
    ret = RtlAnsiStringToUnicodeString(KeyPath, &AnsiPath, TRUE);
    RtlFreeAnsiString( &AnsiPath );
    return ret;
}

/******************************************************************************
 *  RtlOpenCurrentUser		[NTDLL.@]
 *
 * if we return just HKEY_CURRENT_USER the advapi tries to find a remote
 * registry (odd handle) and fails
 *
 */
DWORD WINAPI RtlOpenCurrentUser(
	IN ACCESS_MASK DesiredAccess, /* [in] */
	OUT PHKEY KeyHandle)	      /* [out] handle of HKEY_CURRENT_USER */
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING ObjectName;
	NTSTATUS ret;

	TRACE("(0x%08lx, %p) stub\n",DesiredAccess, KeyHandle);

	RtlFormatCurrentUserKeyPath(&ObjectName);
	InitializeObjectAttributes(&ObjectAttributes,&ObjectName,OBJ_CASE_INSENSITIVE,0, NULL);
	ret = NtCreateKey(KeyHandle, DesiredAccess, &ObjectAttributes, 0, NULL, 0, NULL);
	RtlFreeUnicodeString(&ObjectName);
	return ret;
}
