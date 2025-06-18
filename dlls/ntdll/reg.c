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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "ntdll_misc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(reg);


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
        oa = *attr;
        oa.Attributes &= ~(OBJ_PERMANENT|OBJ_EXCLUSIVE);
        attr = &oa;
    }

    return NtCreateKey(retkey, access, attr, 0, NULL, 0, dispos);
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
 * RtlpNtMakeTemporaryKey [NTDLL.@]
 *
 *  See NtDeleteKey.
 */
NTSTATUS WINAPI RtlpNtMakeTemporaryKey( HANDLE hkey )
{
    return NtDeleteKey(hkey);
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

  if (out->MaximumLength)
  {
    dwLen = out->MaximumLength + offsetof(KEY_BASIC_INFORMATION, Name);
    info = RtlAllocateHeap( GetProcessHeap(), 0, dwLen );
    if (!info)
      return STATUS_NO_MEMORY;
  }
  else
  {
    dwLen = 0;
    info = NULL;
  }

  ret = NtEnumerateKey( handle, index, KeyBasicInformation, info, dwLen, &dwResultLen );
  dwResultLen -= offsetof(KEY_BASIC_INFORMATION, Name);

  if (ret == STATUS_BUFFER_OVERFLOW)
    out->Length = dwResultLen;
  else if (!ret)
  {
    if (out->MaximumLength < info->NameLength)
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
 * RtlpNtQueryValueKey [NTDLL.@]
 *
 */
NTSTATUS WINAPI RtlpNtQueryValueKey( HANDLE handle, ULONG *result_type, PBYTE dest,
                                     DWORD *result_len, void *unknown )
{
    KEY_VALUE_PARTIAL_INFORMATION *info;
    UNICODE_STRING name;
    NTSTATUS ret;
    DWORD dwResultLen;
    DWORD dwLen = offsetof(KEY_VALUE_PARTIAL_INFORMATION, Data[result_len ? *result_len : 0]);

    info = RtlAllocateHeap( GetProcessHeap(), 0, dwLen );
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
 *  RtlFormatCurrentUserKeyPath		[NTDLL.@]
 *
 */
NTSTATUS WINAPI RtlFormatCurrentUserKeyPath( IN OUT PUNICODE_STRING KeyPath)
{
    static const WCHAR pathW[] = {'\\','R','e','g','i','s','t','r','y','\\','U','s','e','r','\\'};
    char buffer[sizeof(TOKEN_USER) + sizeof(SID) + sizeof(DWORD)*SID_MAX_SUB_AUTHORITIES];
    DWORD len = sizeof(buffer);
    NTSTATUS status;

    status = NtQueryInformationToken(GetCurrentThreadEffectiveToken(), TokenUser, buffer, len, &len);
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
                KeyPath->Buffer = buf;
                KeyPath->Length += sizeof(pathW);
                KeyPath->MaximumLength += sizeof(pathW);
            }
            else
                status = STATUS_NO_MEMORY;
        }
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

	TRACE("(0x%08lx, %p)\n",DesiredAccess, KeyHandle);

        if ((ret = RtlFormatCurrentUserKeyPath(&ObjectName))) return ret;
	InitializeObjectAttributes(&ObjectAttributes,&ObjectName,OBJ_CASE_INSENSITIVE,0, NULL);
	ret = NtCreateKey(KeyHandle, DesiredAccess, &ObjectAttributes, 0, NULL, 0, NULL);
	RtlFreeUnicodeString(&ObjectName);
	return ret;
}


static NTSTATUS RTL_ReportRegistryValue(PKEY_VALUE_FULL_INFORMATION pInfo,
                                        PRTL_QUERY_REGISTRY_TABLE pQuery, PVOID pContext, PVOID pEnvironment)
{
    PUNICODE_STRING str = pQuery->EntryContext;
    ULONG type, len, offset, count, res;
    UNICODE_STRING src, dst;
    WCHAR *data, *wstr;
    LONG *bin;
    NTSTATUS status = STATUS_SUCCESS;

    if (pInfo)
    {
        type = pInfo->Type;
        data = (WCHAR*)((char*)pInfo + pInfo->DataOffset);
        len = pInfo->DataLength;

        /* Ensure that multi-strings from the registry are double-null-terminated */
        if (type == REG_MULTI_SZ)
        {
            while (len < 2 * sizeof(WCHAR) || data[len / sizeof(WCHAR) - 2] || data[len / sizeof(WCHAR) - 1])
            {
                data[len / sizeof(WCHAR)] = 0;
                len += sizeof(WCHAR);
            }
        }
    }
    else
    {
        type = pQuery->DefaultType;
        data = pQuery->DefaultData;
        len = pQuery->DefaultLength;

        if (!data)
            return STATUS_DATA_OVERRUN;

        if (!len)
        {
            switch (type)
            {
            case REG_SZ:
            case REG_EXPAND_SZ:
            case REG_LINK:
                len = (wcslen(data) + 1) * sizeof(WCHAR);
                break;

            case REG_MULTI_SZ:
                wstr = data;
                for (;;)
                {
                    count = wcslen(wstr) + 1;
                    len += count * sizeof(WCHAR);
                    if (!*wstr) break;
                    wstr += count;
                }
                break;
            }
        }
    }

    if (pQuery->Flags & RTL_QUERY_REGISTRY_DIRECT)
    {
        if (pQuery->QueryRoutine)
            return STATUS_INVALID_PARAMETER;

        switch (type)
        {
        case REG_EXPAND_SZ:
            if (!(pQuery->Flags & RTL_QUERY_REGISTRY_NOEXPAND))
            {
                RtlInitUnicodeString(&src, data);
                res = 0;
                dst.MaximumLength = 0;
                RtlExpandEnvironmentStrings_U(pEnvironment, &src, &dst, &res);
                if (str->Buffer == NULL)
                {
                    str->Buffer = RtlAllocateHeap(GetProcessHeap(), 0, res);
                    str->MaximumLength = res;
                }
                else if (str->MaximumLength < res)
                    return STATUS_BUFFER_TOO_SMALL;
                RtlExpandEnvironmentStrings_U(pEnvironment, &src, str, &res);
                str->Length = (res >= sizeof(WCHAR) ? res - sizeof(WCHAR) : res);
                break;
            }
            /* fallthrough */

        case REG_SZ:
        case REG_LINK:
            if (str->Buffer == NULL)
                RtlCreateUnicodeString(str, data);
            else
            {
                if (str->MaximumLength < len)
                    return STATUS_BUFFER_TOO_SMALL;
                memcpy(str->Buffer, data, len);
                str->Length = len - sizeof(WCHAR);
            }
            break;

        case REG_MULTI_SZ:
            if (!(pQuery->Flags & RTL_QUERY_REGISTRY_NOEXPAND))
                return STATUS_INVALID_PARAMETER;

            if (str->Buffer == NULL)
            {
                str->Buffer = RtlAllocateHeap(GetProcessHeap(), 0, len);
                str->MaximumLength = len;
            }
            else if (str->MaximumLength < len)
                return STATUS_BUFFER_TOO_SMALL;
            memcpy(str->Buffer, data, len);
            str->Length = (len >= sizeof(WCHAR) ? len - sizeof(WCHAR) : len);
            break;

        default:
            bin = pQuery->EntryContext;
            if (len <= sizeof(ULONG))
                memcpy(bin, data, len);
            else
            {
                if (bin[0] < 0)
                {
                    if (len <= -bin[0])
                        memcpy(bin, data, len);
                }
                else if (len <= bin[0])
                {
                    bin[0] = len;
                    bin[1] = type;
                    memcpy(bin + 2, data, len);
                }
            }
            break;
        }
    }
    else if (pQuery->QueryRoutine)
    {
        if((pQuery->Flags & RTL_QUERY_REGISTRY_NOEXPAND) ||
           (type != REG_EXPAND_SZ && type != REG_MULTI_SZ))
        {
            status = pQuery->QueryRoutine(pQuery->Name, type, data, len, pContext, pQuery->EntryContext);
        }
        else if (type == REG_EXPAND_SZ)
        {
            RtlInitUnicodeString(&src, data);
            res = 0;
            dst.MaximumLength = 0;
            RtlExpandEnvironmentStrings_U(pEnvironment, &src, &dst, &res);
            dst.Length = 0;
            dst.MaximumLength = res;
            dst.Buffer = RtlAllocateHeap(GetProcessHeap(), 0, res * sizeof(WCHAR));
            RtlExpandEnvironmentStrings_U(pEnvironment, &src, &dst, &res);
            status = pQuery->QueryRoutine(pQuery->Name, REG_SZ, dst.Buffer, res, pContext, pQuery->EntryContext);
            RtlFreeHeap(GetProcessHeap(), 0, dst.Buffer);
        }
        else /* REG_MULTI_SZ */
        {
            for (offset = 0; offset + 2 * sizeof(WCHAR) < len; offset += count)
            {
                wstr = (WCHAR*)((char*)data + offset);
                count = (wcslen(wstr) + 1) * sizeof(WCHAR);
                status = pQuery->QueryRoutine(pQuery->Name, REG_SZ, wstr, count, pContext, pQuery->EntryContext);
                if (status != STATUS_SUCCESS && status != STATUS_BUFFER_TOO_SMALL)
                    return status;
            }
        }
    }
    return status;
}


static NTSTATUS RTL_KeyHandleCreateObject(ULONG RelativeTo, PCWSTR Path, POBJECT_ATTRIBUTES regkey, PUNICODE_STRING str)
{
    PCWSTR base;
    INT len;

    switch (RelativeTo & 0xff)
    {
    case RTL_REGISTRY_ABSOLUTE:
        base = L"";
        break;

    case RTL_REGISTRY_CONTROL:
        base = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\";
        break;

    case RTL_REGISTRY_DEVICEMAP:
        base = L"\\Registry\\Machine\\Hardware\\DeviceMap\\";
        break;

    case RTL_REGISTRY_SERVICES:
        base = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\";
        break;

    case RTL_REGISTRY_USER:
        base = L"\\Registry\\User\\CurrentUser\\";
        break;

    case RTL_REGISTRY_WINDOWS_NT:
        base = L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\";
        break;

    default:
        return STATUS_INVALID_PARAMETER;
    }

    len = (wcslen(base) + wcslen(Path) + 1) * sizeof(WCHAR);
    str->Buffer = RtlAllocateHeap(GetProcessHeap(), 0, len);
    if (str->Buffer == NULL)
        return STATUS_NO_MEMORY;

    wcscpy(str->Buffer, base);
    wcscat(str->Buffer, Path);
    str->Length = len - sizeof(WCHAR);
    str->MaximumLength = len;
    InitializeObjectAttributes(regkey, str, OBJ_CASE_INSENSITIVE, NULL, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS RTL_GetKeyHandle(ULONG RelativeTo, PCWSTR Path, PHANDLE handle)
{
    OBJECT_ATTRIBUTES regkey;
    UNICODE_STRING string;
    NTSTATUS status;

    status = RTL_KeyHandleCreateObject(RelativeTo, Path, &regkey, &string);
    if(status != STATUS_SUCCESS)
	return status;

    status = NtOpenKey(handle, KEY_ALL_ACCESS, &regkey);
    RtlFreeUnicodeString( &string );
    return status;
}

/******************************************************************************
 *              RtlQueryRegistryValues  (NTDLL.@)
 *              RtlQueryRegistryValuesEx  (NTDLL.@)
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

    TRACE("(%ld, %s, %p, %p, %p)\n", RelativeTo, debugstr_w(Path), QueryTable, Context, Environment);

    if(Path == NULL)
        return STATUS_INVALID_PARAMETER;

    /* get a valid handle */
    if (RelativeTo & RTL_REGISTRY_HANDLE)
        topkey = handle = (HANDLE)Path;
    else
    {
        status = RTL_GetKeyHandle(RelativeTo, Path, &topkey);
        if (status != STATUS_SUCCESS) return status;
        handle = topkey;
    }

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
                    status == STATUS_BUFFER_TOO_SMALL ||
                    (status == STATUS_SUCCESS && pInfo->Type == REG_MULTI_SZ && buflen < len + 2 * sizeof(L'\0')))
                {
                    buflen = len + 2 * sizeof(L'\0');
                    RtlFreeHeap(GetProcessHeap(), 0, pInfo);
                    pInfo = RtlAllocateHeap(GetProcessHeap(), 0, buflen);
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
                status == STATUS_BUFFER_TOO_SMALL ||
                (status == STATUS_SUCCESS && pInfo->Type == REG_MULTI_SZ && buflen < len + 2 * sizeof(L'\0')))
            {
                buflen = len + 2 * sizeof(L'\0');
                RtlFreeHeap(GetProcessHeap(), 0, pInfo);
                pInfo = RtlAllocateHeap(GetProcessHeap(), 0, buflen);
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
 * Query multiple registry values with a single call.
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

    TRACE("(%ld, %s)\n", RelativeTo, debugstr_w(Path));

    if(!RelativeTo && (Path == NULL || Path[0] == 0))
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    if(RelativeTo & RTL_REGISTRY_HANDLE)
        return STATUS_SUCCESS;
    if((RelativeTo <= RTL_REGISTRY_USER) && (Path == NULL || Path[0] == 0))
        return STATUS_SUCCESS;

    status = RTL_GetKeyHandle(RelativeTo, Path, &handle);
    if (!status) NtClose(handle);
    if (status == STATUS_INVALID_HANDLE) status = STATUS_OBJECT_NAME_NOT_FOUND;
    return status;
}

/*************************************************************************
 * RtlCreateRegistryKey   [NTDLL.@]
 *
 * Add a key to the registry given by absolute or relative path
 *
 * PARAMS
 *  RelativeTo  [I] Registry path that Path refers to
 *  path        [I] Path to key
 *
 * RETURNS
 *  STATUS_SUCCESS or an appropriate NTSTATUS error code.
 */
NTSTATUS WINAPI RtlCreateRegistryKey(ULONG RelativeTo, PWSTR path)
{
    OBJECT_ATTRIBUTES regkey;
    UNICODE_STRING string;
    HANDLE handle;
    NTSTATUS status;

    RelativeTo &= ~RTL_REGISTRY_OPTIONAL;

    if (!RelativeTo && (path == NULL || path[0] == 0))
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    if (RelativeTo <= RTL_REGISTRY_USER && (path == NULL || path[0] == 0))
        return STATUS_SUCCESS;
    status = RTL_KeyHandleCreateObject(RelativeTo, path, &regkey, &string);
    if(status != STATUS_SUCCESS)
	return status;

    status = NtCreateKey(&handle, KEY_ALL_ACCESS, &regkey, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
    if (handle) NtClose(handle);
    RtlFreeUnicodeString( &string );
    return status;
}

/*************************************************************************
 * RtlDeleteRegistryValue   [NTDLL.@]
 *
 * Query multiple registry values with a single call.
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

    TRACE("(%ld, %s, %s)\n", RelativeTo, debugstr_w(Path), debugstr_w(ValueName));

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

    TRACE( "(%ld, %s, %s) -> %ld: %p [%ld]\n", RelativeTo, debugstr_w(path), debugstr_w(name),
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
