/*
 * Registry functions
 *
 * Copyright (C) 1999 Juergen Schmied
 * Copyright (C) 2000 Alexandre Julliard
 *
 * NOTES:
 * 	HKEY_LOCAL_MACHINE	\\REGISTRY\\MACHINE
 *	HKEY_USERS		\\REGISTRY\\USER
 *	HKEY_CURRENT_CONFIG	\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\HARDWARE PROFILES\\CURRENT
  *	HKEY_CLASSES		\\REGISTRY\\MACHINE\\SOFTWARE\\CLASSES
 */

#include <string.h>
#include "debugtools.h"
#include "winreg.h"
#include "winerror.h"
#include "wine/unicode.h"
#include "server.h"
#include "ntddk.h"
#include "ntdll_misc.h"

DEFAULT_DEBUG_CHANNEL(reg);

/* maximum length of a key/value name in bytes (without terminating null) */
#define MAX_NAME_LENGTH ((MAX_PATH-1) * sizeof(WCHAR))


/******************************************************************************
 * NtCreateKey [NTDLL.@]
 * ZwCreateKey
 */
NTSTATUS WINAPI NtCreateKey( PHANDLE retkey, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                             ULONG TitleIndex, const UNICODE_STRING *class, ULONG options,
                             PULONG dispos )
{
    NTSTATUS ret;
    DWORD len = attr->ObjectName->Length;

    TRACE( "(0x%x,%s,%s,%lx,%lx,%p)\n", attr->RootDirectory, debugstr_us(attr->ObjectName),
           debugstr_us(class), options, access, retkey );

    if (len > MAX_NAME_LENGTH) return STATUS_BUFFER_OVERFLOW;
    len += sizeof(WCHAR);  /* for storing name length */
    if (class)
    {
        len += class->Length;
        if (len > REQUEST_MAX_VAR_SIZE) return STATUS_BUFFER_OVERFLOW;
    }
    if (!retkey) return STATUS_INVALID_PARAMETER;

    SERVER_START_VAR_REQ( create_key, len )
    {
        WCHAR *data = server_data_ptr(req);

        req->parent  = attr->RootDirectory;
        req->access  = access;
        req->options = options;
        req->modif   = 0;

        *data++ = attr->ObjectName->Length;
        memcpy( data, attr->ObjectName->Buffer, attr->ObjectName->Length );
        if (class) memcpy( (char *)data + attr->ObjectName->Length, class->Buffer, class->Length );
        if (!(ret = SERVER_CALL()))
        {
            *retkey = req->hkey;
            if (dispos) *dispos = req->created ? REG_CREATED_NEW_KEY : REG_OPENED_EXISTING_KEY;
        }
    }
    SERVER_END_VAR_REQ;
    TRACE("<- 0x%04x\n", *retkey);
    return ret;
}


/******************************************************************************
 * NtOpenKey [NTDLL.@]
 * ZwOpenKey
 *   OUT	PHANDLE			retkey (returns 0 when failure)
 *   IN		ACCESS_MASK		access
 *   IN		POBJECT_ATTRIBUTES 	attr 
 */
NTSTATUS WINAPI NtOpenKey( PHANDLE retkey, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    NTSTATUS ret;
    DWORD len = attr->ObjectName->Length;

    TRACE( "(0x%x,%s,%lx,%p)\n", attr->RootDirectory,
           debugstr_us(attr->ObjectName), access, retkey );

    if (len > MAX_NAME_LENGTH) return STATUS_BUFFER_OVERFLOW;
    if (!retkey) return STATUS_INVALID_PARAMETER;

    SERVER_START_VAR_REQ( open_key, len )
    {
        req->parent = attr->RootDirectory;
        req->access = access;
        memcpy( server_data_ptr(req), attr->ObjectName->Buffer, len );
        ret = SERVER_CALL();
        *retkey = req->hkey;
    }
    SERVER_END_VAR_REQ;
    TRACE("<- 0x%04x\n", *retkey);
    return ret;
}


/******************************************************************************
 * NtDeleteKey [NTDLL.@]
 * ZwDeleteKey
 */
NTSTATUS WINAPI NtDeleteKey( HANDLE hkey )
{
    NTSTATUS ret;

    TRACE( "(%x)\n", hkey );

    SERVER_START_REQ( delete_key )
    {
        req->hkey = hkey;
        ret = SERVER_CALL();
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 * NtDeleteValueKey [NTDLL.@]
 * ZwDeleteValueKey
 */
NTSTATUS WINAPI NtDeleteValueKey( HANDLE hkey, const UNICODE_STRING *name )
{
    NTSTATUS ret;

    TRACE( "(0x%x,%s)\n", hkey, debugstr_us(name) );
    if (name->Length > MAX_NAME_LENGTH) return STATUS_BUFFER_OVERFLOW;

    SERVER_START_VAR_REQ( delete_key_value, name->Length )
    {
        req->hkey = hkey;
        memcpy( server_data_ptr(req), name->Buffer, name->Length );
        ret = SERVER_CALL();
    }
    SERVER_END_VAR_REQ;
    return ret;
}


/******************************************************************************
 *     fill_key_info
 *
 * Helper function for NtQueryKey and NtEnumerateKey
 */
static NTSTATUS fill_key_info( KEY_INFORMATION_CLASS info_class, void *info, DWORD length,
                               DWORD *result_len, const struct enum_key_request *req )
{
    WCHAR *name_ptr = server_data_ptr(req);
    int name_size = *name_ptr++;
    WCHAR *class_ptr = (WCHAR *)((char *)name_ptr + name_size);
    int class_size = server_data_size(req) - sizeof(WCHAR) - name_size;
    int fixed_size;
    FILETIME modif;

    RtlSecondsSince1970ToTime( req->modif, &modif );

    switch(info_class)
    {
    case KeyBasicInformation:
        {
            KEY_BASIC_INFORMATION keyinfo;
            fixed_size = sizeof(keyinfo) - sizeof(keyinfo.Name);
            keyinfo.LastWriteTime = modif;
            keyinfo.TitleIndex = 0;
            keyinfo.NameLength = name_size;
            memcpy( info, &keyinfo, min( length, fixed_size ) );
            class_size = 0;
        }
        break;
    case KeyFullInformation:
        {
            KEY_FULL_INFORMATION keyinfo;
            fixed_size = sizeof(keyinfo) - sizeof(keyinfo.Class);
            keyinfo.LastWriteTime = modif;
            keyinfo.TitleIndex = 0;
            keyinfo.ClassLength = class_size;
            keyinfo.ClassOffset = keyinfo.ClassLength ? fixed_size : -1;
            keyinfo.SubKeys = req->subkeys;
            keyinfo.MaxNameLen = req->max_subkey;
            keyinfo.MaxClassLen = req->max_class;
            keyinfo.Values = req->values;
            keyinfo.MaxValueNameLen = req->max_value;
            keyinfo.MaxValueDataLen = req->max_data;
            memcpy( info, &keyinfo, min( length, fixed_size ) );
            name_size = 0;
        }
        break;
    case KeyNodeInformation:
        {
            KEY_NODE_INFORMATION keyinfo;
            fixed_size = sizeof(keyinfo) - sizeof(keyinfo.Name);
            keyinfo.LastWriteTime = modif;
            keyinfo.TitleIndex = 0;
            keyinfo.ClassLength = class_size;
            keyinfo.ClassOffset = fixed_size + name_size;
            if (!keyinfo.ClassLength || keyinfo.ClassOffset > length) keyinfo.ClassOffset = -1;
            keyinfo.NameLength = name_size;
            memcpy( info, &keyinfo, min( length, fixed_size ) );
        }
        break;
    default:
        FIXME("Information class not implemented\n");
        return STATUS_INVALID_PARAMETER;
    }

    *result_len = fixed_size + name_size + class_size;
    if (length <= fixed_size) return STATUS_BUFFER_OVERFLOW;
    length -= fixed_size;

    /* copy the name */
    if (name_size)
    {
        memcpy( (char *)info + fixed_size, name_ptr, min(length,name_size) );
        if (length < name_size) return STATUS_BUFFER_OVERFLOW;
        length -= name_size;
    }

    /* copy the class */
    if (class_size)
    {
        memcpy( (char *)info + fixed_size + name_size, class_ptr, min(length,class_size) );
        if (length < class_size) return STATUS_BUFFER_OVERFLOW;
    }
    return STATUS_SUCCESS;
}



/******************************************************************************
 * NtEnumerateKey [NTDLL.@]
 * ZwEnumerateKey
 *
 * NOTES
 *  the name copied into the buffer is NOT 0-terminated 
 */
NTSTATUS WINAPI NtEnumerateKey( HANDLE handle, ULONG index, KEY_INFORMATION_CLASS info_class,
                                void *info, DWORD length, DWORD *result_len )
{
    NTSTATUS ret;

    /* -1 means query key, so avoid it here */
    if (index == (ULONG)-1) return STATUS_NO_MORE_ENTRIES;

    SERVER_START_VAR_REQ( enum_key, REQUEST_MAX_VAR_SIZE )
    {
        req->hkey  = handle;
        req->index = index;
        req->full  = (info_class == KeyFullInformation);
        if (!(ret = SERVER_CALL()))
        {
            ret = fill_key_info( info_class, info, length, result_len, req );
        }
    }
    SERVER_END_VAR_REQ;
    return ret;
}


/******************************************************************************
 * NtQueryKey [NTDLL.@]
 * ZwQueryKey
 */
NTSTATUS WINAPI NtQueryKey( HANDLE handle, KEY_INFORMATION_CLASS info_class,
                            void *info, DWORD length, DWORD *result_len )
{
    NTSTATUS ret;

    SERVER_START_VAR_REQ( enum_key, REQUEST_MAX_VAR_SIZE )
    {
        req->hkey  = handle;
        req->index = -1;
        req->full  = (info_class == KeyFullInformation);
        if (!(ret = SERVER_CALL()))
        {
            ret = fill_key_info( info_class, info, length, result_len, req );
        }
    }
    SERVER_END_VAR_REQ;
    return ret;
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
            length = min( length, sizeof(keyinfo) - sizeof(keyinfo.Name) );
            memcpy( info, &keyinfo, length );
            break;
        }
    case KeyValueFullInformation:
        {
            KEY_VALUE_FULL_INFORMATION keyinfo;
            keyinfo.TitleIndex = 0;
            keyinfo.Type       = type;
            keyinfo.DataOffset = sizeof(keyinfo) - sizeof(keyinfo.Name) + name_len;
            keyinfo.DataLength = data_len;
            keyinfo.NameLength = name_len;
            length = min( length, sizeof(keyinfo) - sizeof(keyinfo.Name) );
            memcpy( info, &keyinfo, length );
            break;
        }
    case KeyValuePartialInformation:
        {
            KEY_VALUE_PARTIAL_INFORMATION keyinfo;
            keyinfo.TitleIndex = 0;
            keyinfo.Type       = type;
            keyinfo.DataLength = data_len;
            length = min( length, sizeof(keyinfo) - sizeof(keyinfo.Data) );
            memcpy( info, &keyinfo, length );
            break;
        }
    default:
        break;
    }
}


/******************************************************************************
 *  NtEnumerateValueKey	[NTDLL.@]
 *  ZwEnumerateValueKey
 */
NTSTATUS WINAPI NtEnumerateValueKey( HANDLE handle, ULONG index,
                                     KEY_VALUE_INFORMATION_CLASS info_class,
                                     void *info, DWORD length, DWORD *result_len )
{
    NTSTATUS ret;
    char *data_ptr, *name_ptr;
    int fixed_size = 0, name_len = 0, data_len = 0, offset = 0, type = 0, total_len = 0;

    TRACE( "(0x%x,%lu,%d,%p,%ld)\n", handle, index, info_class, info, length );

    /* compute the length we want to retrieve */
    switch(info_class)
    {
    case KeyValueBasicInformation:
        fixed_size = sizeof(KEY_VALUE_BASIC_INFORMATION) - sizeof(WCHAR);
        name_ptr = (char *)info + fixed_size;
        data_ptr = NULL;
        break;
    case KeyValueFullInformation:
        fixed_size = sizeof(KEY_VALUE_FULL_INFORMATION) - sizeof(WCHAR);
        name_ptr = data_ptr = (char *)info + fixed_size;
        break;
    case KeyValuePartialInformation:
        fixed_size = sizeof(KEY_VALUE_PARTIAL_INFORMATION) - sizeof(UCHAR);
        name_ptr = NULL;
        data_ptr = (char *)info + fixed_size;
        break;
    default:
        FIXME( "Information class %d not implemented\n", info_class );
        return STATUS_INVALID_PARAMETER;
    }
    if (length > fixed_size) data_len = length - fixed_size;

    do
    {
        size_t reqlen = data_len + sizeof(WCHAR);
        if (name_ptr && !offset) reqlen += MAX_PATH*sizeof(WCHAR);
        reqlen = min( reqlen, REQUEST_MAX_VAR_SIZE );

        SERVER_START_VAR_REQ( enum_key_value, reqlen )
        {
            req->hkey = handle;
            req->index = index;
            req->offset = offset;

            if (!(ret = SERVER_CALL()))
            {
                size_t size = server_data_size(req) - sizeof(WCHAR);
                WCHAR *name = server_data_ptr(req);
                if (!offset)  /* name is only present on the first request */
                {
                    name_len = *name++;
                    size -= name_len;
                    if (name_ptr)
                    {
                        if (name_len > data_len)  /* overflow */
                        {
                            memcpy( name_ptr, name, data_len );
                            data_len = 0;
                            ret = STATUS_BUFFER_OVERFLOW;
                        }
                        else
                        {
                            memcpy( name_ptr, name, name_len );
                            data_len -= name_len;
                            if (data_ptr) data_ptr += name_len;
                        }
                    }
                    name += name_len / sizeof(WCHAR);
                }
                else name++;  /* skip 0 length */

                if (data_ptr)
                {
                    size = min( size, data_len );
                    memcpy( data_ptr + offset, name, size );
                    offset += size;
                    data_len -= size;
                }
                type = req->type;
                total_len = req->len;
            }
        }
        SERVER_END_VAR_REQ;
        if (ret) return ret;
    } while (data_len && data_ptr && offset < total_len);

    *result_len = total_len + fixed_size + (name_ptr ? name_len : 0);

    if (data_ptr && offset < total_len) ret = STATUS_BUFFER_OVERFLOW;
    if (length < fixed_size) ret = STATUS_BUFFER_OVERFLOW;

    copy_key_value_info( info_class, info, length, type, name_len, total_len );
    return ret;

}


/******************************************************************************
 * NtQueryValueKey [NTDLL.@]
 * ZwQueryValueKey
 *
 * NOTES
 *  the name in the KeyValueInformation is never set
 */
NTSTATUS WINAPI NtQueryValueKey( HANDLE handle, const UNICODE_STRING *name,
                                 KEY_VALUE_INFORMATION_CLASS info_class,
                                 void *info, DWORD length, DWORD *result_len )
{
    NTSTATUS ret;
    char *data_ptr;
    int fixed_size = 0, data_len = 0, offset = 0, type = 0, total_len = 0;

    TRACE( "(0x%x,%s,%d,%p,%ld)\n", handle, debugstr_us(name), info_class, info, length );

    if (name->Length > MAX_NAME_LENGTH) return STATUS_BUFFER_OVERFLOW;

    /* compute the length we want to retrieve */
    switch(info_class)
    {
    case KeyValueBasicInformation:
        fixed_size = sizeof(KEY_VALUE_BASIC_INFORMATION) - sizeof(WCHAR);
        data_ptr = NULL;
        break;
    case KeyValueFullInformation:
        fixed_size = sizeof(KEY_VALUE_FULL_INFORMATION) - sizeof(WCHAR);
        data_ptr = (char *)info + fixed_size;
        break;
    case KeyValuePartialInformation:
        fixed_size = sizeof(KEY_VALUE_PARTIAL_INFORMATION) - sizeof(UCHAR);
        data_ptr = (char *)info + fixed_size;
        break;
    default:
        FIXME( "Information class %d not implemented\n", info_class );
        return STATUS_INVALID_PARAMETER;
    }
    if (data_ptr && length > fixed_size) data_len = length - fixed_size;

    do
    {
        size_t reqlen = min( data_len, REQUEST_MAX_VAR_SIZE );
        reqlen = max( reqlen, name->Length + sizeof(WCHAR) );

        SERVER_START_VAR_REQ( get_key_value, reqlen )
        {
            WCHAR *nameptr = server_data_ptr(req);

            req->hkey = handle;
            req->offset = offset;
            *nameptr++ = name->Length;
            memcpy( nameptr, name->Buffer, name->Length );

            if (!(ret = SERVER_CALL()))
            {
                size_t size = min( server_data_size(req), data_len );
                type = req->type;
                total_len = req->len;
                if (size)
                {
                    memcpy( data_ptr + offset, server_data_ptr(req), size );
                    offset += size;
                    data_len -= size;
                }
            }
        }
        SERVER_END_VAR_REQ;
        if (ret) return ret;
    } while (data_len && offset < total_len);

    *result_len = total_len + fixed_size;

    if (offset < total_len) ret = STATUS_BUFFER_OVERFLOW;
    if (length < fixed_size) ret = STATUS_BUFFER_OVERFLOW;

    copy_key_value_info( info_class, info, length, type, 0, total_len );
    return ret;
}


/******************************************************************************
 *  NtFlushKey	[NTDLL.@]
 *  ZwFlushKey
 */
NTSTATUS WINAPI NtFlushKey(HANDLE KeyHandle)
{
	FIXME("(0x%08x) stub!\n",
	KeyHandle);
	return 1;
}

/******************************************************************************
 *  NtLoadKey	[NTDLL.@]
 *  ZwLoadKey
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
 *  ZwNotifyChangeKey
 */
NTSTATUS WINAPI NtNotifyChangeKey(
	IN HANDLE KeyHandle,
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
	FIXME("(0x%08x,0x%08x,%p,%p,%p,0x%08lx, 0x%08x,%p,0x%08lx,0x%08x) stub!\n",
	KeyHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter,
	Asynchroneous, ChangeBuffer, Length, WatchSubtree);
	return STATUS_SUCCESS;
}

/******************************************************************************
 * NtQueryMultipleValueKey [NTDLL]
 * ZwQueryMultipleValueKey
 */

NTSTATUS WINAPI NtQueryMultipleValueKey(
	HANDLE KeyHandle,
	PVALENTW ListOfValuesToQuery,
	ULONG NumberOfItems,
	PVOID MultipleValueInformation,
	ULONG Length,
	PULONG  ReturnLength)
{
	FIXME("(0x%08x,%p,0x%08lx,%p,0x%08lx,%p) stub!\n",
	KeyHandle, ListOfValuesToQuery, NumberOfItems, MultipleValueInformation,
	Length,ReturnLength);
	return STATUS_SUCCESS;
}

/******************************************************************************
 * NtReplaceKey [NTDLL.@]
 * ZwReplaceKey
 */
NTSTATUS WINAPI NtReplaceKey(
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE Key,
	IN POBJECT_ATTRIBUTES ReplacedObjectAttributes)
{
	FIXME("(0x%08x),stub!\n", Key);
	dump_ObjectAttributes(ObjectAttributes);
	dump_ObjectAttributes(ReplacedObjectAttributes);
	return STATUS_SUCCESS;
}
/******************************************************************************
 * NtRestoreKey [NTDLL.@]
 * ZwRestoreKey
 */
NTSTATUS WINAPI NtRestoreKey(
	HANDLE KeyHandle,
	HANDLE FileHandle,
	ULONG RestoreFlags)
{
	FIXME("(0x%08x,0x%08x,0x%08lx) stub\n",
	KeyHandle, FileHandle, RestoreFlags);
	return STATUS_SUCCESS;
}
/******************************************************************************
 * NtSaveKey [NTDLL.@]
 * ZwSaveKey
 */
NTSTATUS WINAPI NtSaveKey(
	IN HANDLE KeyHandle,
	IN HANDLE FileHandle)
{
	FIXME("(0x%08x,0x%08x) stub\n",
	KeyHandle, FileHandle);
	return STATUS_SUCCESS;
}
/******************************************************************************
 * NtSetInformationKey [NTDLL.@]
 * ZwSetInformationKey
 */
NTSTATUS WINAPI NtSetInformationKey(
	IN HANDLE KeyHandle,
	IN const int KeyInformationClass,
	IN PVOID KeyInformation,
	IN ULONG KeyInformationLength)
{
	FIXME("(0x%08x,0x%08x,%p,0x%08lx) stub\n",
	KeyHandle, KeyInformationClass, KeyInformation, KeyInformationLength);
	return STATUS_SUCCESS;
}


/******************************************************************************
 * NtSetValueKey [NTDLL.@]
 * ZwSetValueKey
 *
 * NOTES
 *   win95 does not care about count for REG_SZ and finds out the len by itself (js) 
 *   NT does definitely care (aj)
 */
NTSTATUS WINAPI NtSetValueKey( HANDLE hkey, const UNICODE_STRING *name, ULONG TitleIndex,
                               ULONG type, const void *data, ULONG count )
{
    NTSTATUS ret;
    ULONG namelen, pos;

    TRACE( "(0x%x,%s,%ld,%p,%ld)\n", hkey, debugstr_us(name), type, data, count );

    if (name->Length > MAX_NAME_LENGTH) return STATUS_BUFFER_OVERFLOW;

    namelen = name->Length + sizeof(WCHAR);  /* for storing length */
    pos = 0;

    do
    {
        ULONG len = count - pos;
        if (len > REQUEST_MAX_VAR_SIZE - namelen) len = REQUEST_MAX_VAR_SIZE - namelen;

        SERVER_START_VAR_REQ( set_key_value, namelen + len )
        {
            WCHAR *name_ptr = server_data_ptr(req);

            req->hkey   = hkey;
            req->type   = type;
            req->total  = count;
            req->offset = pos;
            *name_ptr++ = name->Length;
            memcpy( name_ptr, name->Buffer, name->Length );
            memcpy( (char *)name_ptr + name->Length, (char *)data + pos, len );
            pos += len;
            ret = SERVER_CALL();
        }
        SERVER_END_VAR_REQ;
    } while (!ret && pos < count);
    return ret;
}

/******************************************************************************
 * NtUnloadKey [NTDLL.@]
 * ZwUnloadKey
 */
NTSTATUS WINAPI NtUnloadKey(
	IN HANDLE KeyHandle)
{
	FIXME("(0x%08x) stub\n",
	KeyHandle);
	return STATUS_SUCCESS;
}

/******************************************************************************
 *  RtlFormatCurrentUserKeyPath		[NTDLL.@]
 */
NTSTATUS WINAPI RtlFormatCurrentUserKeyPath(
	IN OUT PUNICODE_STRING KeyPath)
{
/*	LPSTR Path = "\\REGISTRY\\USER\\S-1-5-21-0000000000-000000000-0000000000-500";*/
	LPSTR Path = "\\REGISTRY\\USER\\.DEFAULT";
	ANSI_STRING AnsiPath;

	FIXME("(%p) stub\n",KeyPath);
	RtlInitAnsiString(&AnsiPath, Path);
	return RtlAnsiStringToUnicodeString(KeyPath, &AnsiPath, TRUE);
}

/******************************************************************************
 *  RtlOpenCurrentUser		[NTDLL.@]
 *
 * if we return just HKEY_CURRENT_USER the advapi try's to find a remote
 * registry (odd handle) and fails
 *
 */
DWORD WINAPI RtlOpenCurrentUser(
	IN ACCESS_MASK DesiredAccess, /* [in] */
	OUT PHANDLE KeyHandle)	      /* [out] handle of HKEY_CURRENT_USER */
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING ObjectName;
	NTSTATUS ret;

	TRACE("(0x%08lx, %p) stub\n",DesiredAccess, KeyHandle);

	RtlFormatCurrentUserKeyPath(&ObjectName);
	InitializeObjectAttributes(&ObjectAttributes,&ObjectName,OBJ_CASE_INSENSITIVE,0, NULL);
	ret = NtOpenKey(KeyHandle, DesiredAccess, &ObjectAttributes);
	RtlFreeUnicodeString(&ObjectName);
	return ret;
}
