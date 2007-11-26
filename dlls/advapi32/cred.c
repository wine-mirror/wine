/*
 * Credential Management APIs
 *
 * Copyright 2007 Robert Shearman for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wincred.h"
#include "winternl.h"

#include "crypt.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cred);

/* the size of the ARC4 key used to encrypt the password data */
#define KEY_SIZE 8

static const WCHAR wszCredentialManagerKey[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\',
    'C','r','e','d','e','n','t','i','a','l',' ','M','a','n','a','g','e','r',0};
static const WCHAR wszEncryptionKeyValue[] = {'E','n','c','r','y','p','t','i','o','n','K','e','y',0};

static const WCHAR wszFlagsValue[] = {'F','l','a','g','s',0};
static const WCHAR wszTypeValue[] = {'T','y','p','e',0};
static const WCHAR wszTargetNameValue[] = {'T','a','r','g','e','t','N','a','m','e',0};
static const WCHAR wszCommentValue[] = {'C','o','m','m','e','n','t',0};
static const WCHAR wszLastWrittenValue[] = {'L','a','s','t','W','r','i','t','t','e','n',0};
static const WCHAR wszPersistValue[] = {'P','e','r','s','i','s','t',0};
static const WCHAR wszTargetAliasValue[] = {'T','a','r','g','e','t','A','l','i','a','s',0};
static const WCHAR wszUserNameValue[] = {'U','s','e','r','N','a','m','e',0};
static const WCHAR wszPasswordValue[] = {'P','a','s','s','w','o','r','d',0};

static DWORD read_credential_blob(HKEY hkey, const BYTE key_data[KEY_SIZE],
                                  LPBYTE credential_blob,
                                  DWORD *credential_blob_size)
{
    DWORD ret;
    DWORD type;

    *credential_blob_size = 0;
    ret = RegQueryValueExW(hkey, wszPasswordValue, 0, &type, NULL, credential_blob_size);
    if (ret != ERROR_SUCCESS)
        return ret;
    else if (type != REG_BINARY)
        return ERROR_REGISTRY_CORRUPT;
    if (credential_blob)
    {
        struct ustring data;
        struct ustring key;

        ret = RegQueryValueExW(hkey, wszPasswordValue, 0, &type, (LPVOID)credential_blob,
                               credential_blob_size);
        if (ret != ERROR_SUCCESS)
            return ret;
        else if (type != REG_BINARY)
            return ERROR_REGISTRY_CORRUPT;

        key.Length = key.MaximumLength = sizeof(key_data);
        key.Buffer = (unsigned char *)key_data;

        data.Length = data.MaximumLength = *credential_blob_size;
        data.Buffer = credential_blob;
        SystemFunction032(&data, &key);
    }
    return ERROR_SUCCESS;
}

static DWORD registry_read_credential(HKEY hkey, PCREDENTIALW credential,
                                      const BYTE key_data[KEY_SIZE],
                                      char *buffer, DWORD *len)
{
    DWORD type;
    DWORD ret;
    DWORD count;

    ret = RegQueryValueExW(hkey, NULL, 0, &type, NULL, &count);
    if (ret != ERROR_SUCCESS)
        return ret;
    else if (type != REG_SZ)
        return ERROR_REGISTRY_CORRUPT;
    *len += count;
    if (credential)
    {
        credential->TargetName = (LPWSTR)buffer;
        ret = RegQueryValueExW(hkey, NULL, 0, &type, (LPVOID)credential->TargetName,
                               &count);
        if (ret != ERROR_SUCCESS || type != REG_SZ) return ret;
        buffer += count;
    }

    ret = RegQueryValueExW(hkey, wszCommentValue, 0, &type, NULL, &count);
    if (ret != ERROR_FILE_NOT_FOUND && ret != ERROR_SUCCESS)
        return ret;
    else if (type != REG_SZ)
        return ERROR_REGISTRY_CORRUPT;
    *len += count;
    if (credential)
    {
        credential->Comment = (LPWSTR)buffer;
        ret = RegQueryValueExW(hkey, wszCommentValue, 0, &type, (LPVOID)credential->Comment,
                               &count);
        if (ret == ERROR_FILE_NOT_FOUND)
            credential->Comment = NULL;
        else if (ret != ERROR_SUCCESS)
            return ret;
        else if (type != REG_SZ)
            return ERROR_REGISTRY_CORRUPT;
        else
            buffer += count;
    }

    ret = RegQueryValueExW(hkey, wszTargetAliasValue, 0, &type, NULL, &count);
    if (ret != ERROR_FILE_NOT_FOUND && ret != ERROR_SUCCESS)
        return ret;
    else if (type != REG_SZ)
        return ERROR_REGISTRY_CORRUPT;
    *len += count;
    if (credential)
    {
        credential->TargetAlias = (LPWSTR)buffer;
        ret = RegQueryValueExW(hkey, wszTargetAliasValue, 0, &type, (LPVOID)credential->TargetAlias,
                               &count);
        if (ret == ERROR_FILE_NOT_FOUND)
            credential->TargetAlias = NULL;
        else if (ret != ERROR_SUCCESS)
            return ret;
        else if (type != REG_SZ)
            return ERROR_REGISTRY_CORRUPT;
        else
            buffer += count;
    }

    ret = RegQueryValueExW(hkey, wszUserNameValue, 0, &type, NULL, &count);
    if (ret != ERROR_FILE_NOT_FOUND && ret != ERROR_SUCCESS)
        return ret;
    else if (type != REG_SZ)
        return ERROR_REGISTRY_CORRUPT;
    *len += count;
    if (credential)
    {
        credential->UserName = (LPWSTR)buffer;
        ret = RegQueryValueExW(hkey, wszUserNameValue, 0, &type, (LPVOID)credential->UserName,
                               &count);
        if (ret == ERROR_FILE_NOT_FOUND)
        {
            credential->UserName = NULL;
            ret = ERROR_SUCCESS;
        }
        else if (ret != ERROR_SUCCESS)
            return ret;
        else if (type != REG_SZ)
            return ERROR_REGISTRY_CORRUPT;
        else
            buffer += count;
    }

    ret = read_credential_blob(hkey, key_data, NULL, &count);
    if (ret != ERROR_FILE_NOT_FOUND && ret != ERROR_SUCCESS)
        return ret;
    *len += count;
    if (credential)
    {
        credential->CredentialBlob = (LPBYTE)buffer;
        ret = read_credential_blob(hkey, key_data, credential->CredentialBlob, &count);
        if (ret == ERROR_FILE_NOT_FOUND)
        {
            credential->CredentialBlob = NULL;
            ret = ERROR_SUCCESS;
        }
        else if (ret != ERROR_SUCCESS)
            return ret;
        credential->CredentialBlobSize = count;
        buffer += count;
    }

    /* FIXME: Attributes */
    if (credential)
    {
        credential->AttributeCount = 0;
        credential->Attributes = NULL;
    }

    if (!credential) return ERROR_SUCCESS;

    count = sizeof(credential->Flags);
    ret = RegQueryValueExW(hkey, wszFlagsValue, NULL, &type, (LPVOID)&credential->Flags,
                           &count);
    if (ret != ERROR_SUCCESS)
        return ret;
    else if (type != REG_DWORD)
        return ERROR_REGISTRY_CORRUPT;
    count = sizeof(credential->Type);
    ret = RegQueryValueExW(hkey, wszTypeValue, NULL, &type, (LPVOID)&credential->Type,
                           &count);
    if (ret != ERROR_SUCCESS)
        return ret;
    else if (type != REG_DWORD)
        return ERROR_REGISTRY_CORRUPT;

    count = sizeof(credential->LastWritten);
    ret = RegQueryValueExW(hkey, wszLastWrittenValue, NULL, &type, (LPVOID)&credential->LastWritten,
                           &count);
    if (ret != ERROR_SUCCESS)
        return ret;
    else if (type != REG_BINARY)
        return ERROR_REGISTRY_CORRUPT;
    count = sizeof(credential->Persist);
    ret = RegQueryValueExW(hkey, wszPersistValue, NULL, &type, (LPVOID)&credential->Persist,
                           &count);
    if (ret == ERROR_SUCCESS && type != REG_DWORD)
        return ERROR_REGISTRY_CORRUPT;
    return ret;
}

static DWORD write_credential_blob(HKEY hkey, LPCWSTR target_name, DWORD type,
                                   const BYTE key_data[KEY_SIZE],
                                   const BYTE *credential_blob, DWORD credential_blob_size)
{
    LPBYTE encrypted_credential_blob;
    struct ustring data;
    struct ustring key;
    DWORD ret;

    key.Length = key.MaximumLength = sizeof(key_data);
    key.Buffer = (unsigned char *)key_data;

    encrypted_credential_blob = HeapAlloc(GetProcessHeap(), 0, credential_blob_size);
    if (!encrypted_credential_blob) return ERROR_OUTOFMEMORY;

    memcpy(encrypted_credential_blob, credential_blob, credential_blob_size);
    data.Length = data.MaximumLength = credential_blob_size;
    data.Buffer = encrypted_credential_blob;
    SystemFunction032(&data, &key);

    ret = RegSetValueExW(hkey, wszPasswordValue, 0, REG_BINARY, (LPVOID)encrypted_credential_blob, credential_blob_size);
    HeapFree(GetProcessHeap(), 0, encrypted_credential_blob);

    return ret;
}

static DWORD registry_write_credential(HKEY hkey, const CREDENTIALW *credential,
                                       const BYTE key_data[KEY_SIZE], BOOL preserve_blob)
{
    DWORD ret;
    FILETIME LastWritten;

    GetSystemTimeAsFileTime(&LastWritten);

    ret = RegSetValueExW(hkey, wszFlagsValue, 0, REG_DWORD, (LPVOID)&credential->Flags,
                         sizeof(credential->Flags));
    if (ret != ERROR_SUCCESS) return ret;
    ret = RegSetValueExW(hkey, wszTypeValue, 0, REG_DWORD, (LPVOID)&credential->Type,
                         sizeof(credential->Type));
    if (ret != ERROR_SUCCESS) return ret;
    ret = RegSetValueExW(hkey, NULL, 0, REG_SZ, (LPVOID)credential->TargetName,
                         sizeof(WCHAR)*(strlenW(credential->TargetName)+1));
    if (ret != ERROR_SUCCESS) return ret;
    if (credential->Comment)
    {
        ret = RegSetValueExW(hkey, wszCommentValue, 0, REG_SZ, (LPVOID)credential->Comment,
                             sizeof(WCHAR)*(strlenW(credential->Comment)+1));
        if (ret != ERROR_SUCCESS) return ret;
    }
    ret = RegSetValueExW(hkey, wszLastWrittenValue, 0, REG_BINARY, (LPVOID)&LastWritten,
                         sizeof(LastWritten));
    if (ret != ERROR_SUCCESS) return ret;
    ret = RegSetValueExW(hkey, wszPersistValue, 0, REG_DWORD, (LPVOID)&credential->Persist,
                         sizeof(credential->Persist));
    if (ret != ERROR_SUCCESS) return ret;
    /* FIXME: Attributes */
    if (credential->TargetAlias)
    {
        ret = RegSetValueExW(hkey, wszTargetAliasValue, 0, REG_SZ, (LPVOID)credential->TargetAlias,
                             sizeof(WCHAR)*(strlenW(credential->TargetAlias)+1));
        if (ret != ERROR_SUCCESS) return ret;
    }
    if (credential->UserName)
    {
        ret = RegSetValueExW(hkey, wszUserNameValue, 0, REG_SZ, (LPVOID)credential->UserName,
                             sizeof(WCHAR)*(strlenW(credential->UserName)+1));
        if (ret != ERROR_SUCCESS) return ret;
    }
    if (!preserve_blob)
    {
        ret = write_credential_blob(hkey, credential->TargetName, credential->Type,
                                    key_data, credential->CredentialBlob,
                                    credential->CredentialBlobSize);
    }
    return ret;
}

static DWORD open_cred_mgr_key(HKEY *hkey, BOOL open_for_write)
{
    return RegCreateKeyExW(HKEY_CURRENT_USER, wszCredentialManagerKey, 0,
                           NULL, REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE, NULL, hkey, NULL);
}

static DWORD get_cred_mgr_encryption_key(HKEY hkeyMgr, BYTE key_data[KEY_SIZE])
{
    static const BYTE my_key_data[KEY_SIZE] = { 0 };
    DWORD type;
    DWORD count;
    FILETIME ft;
    ULONG seed;
    ULONG value;
    DWORD ret;

    memcpy(key_data, my_key_data, KEY_SIZE);

    count = KEY_SIZE;
    ret = RegQueryValueExW(hkeyMgr, wszEncryptionKeyValue, NULL, &type, (LPVOID)key_data,
                           &count);
    if (ret == ERROR_SUCCESS)
    {
        if (type != REG_BINARY)
            return ERROR_REGISTRY_CORRUPT;
        else
            return ERROR_SUCCESS;
    }
    if (ret != ERROR_FILE_NOT_FOUND)
        return ret;

    GetSystemTimeAsFileTime(&ft);
    seed = ft.dwLowDateTime;
    value = RtlUniform(&seed);
    *(DWORD *)key_data = value;
    seed = ft.dwHighDateTime;
    value = RtlUniform(&seed);
    *(DWORD *)(key_data + 4) = value;

    return RegSetValueExW(hkeyMgr, wszEncryptionKeyValue, 0, REG_BINARY,
                          (LPVOID)key_data, KEY_SIZE);
}

static LPWSTR get_key_name_for_target(LPCWSTR target_name, DWORD type)
{
    static const WCHAR wszGenericPrefix[] = {'G','e','n','e','r','i','c',':',' ',0};
    static const WCHAR wszDomPasswdPrefix[] = {'D','o','m','P','a','s','s','w','d',':',' ',0};
    INT len;
    LPCWSTR prefix = NULL;
    LPWSTR key_name, p;

    len = strlenW(target_name);
    if (type == CRED_TYPE_GENERIC)
    {
        prefix = wszGenericPrefix;
        len += sizeof(wszGenericPrefix)/sizeof(wszGenericPrefix[0]);
    }
    else
    {
        prefix = wszDomPasswdPrefix;
        len += sizeof(wszDomPasswdPrefix)/sizeof(wszDomPasswdPrefix[0]);
    }

    key_name = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!key_name) return NULL;

    strcpyW(key_name, prefix);
    strcatW(key_name, target_name);

    for (p = key_name; *p; p++)
        if (*p == '\\') *p = '_';

    return key_name;
}

static BOOL credential_matches_filter(HKEY hkeyCred, LPCWSTR filter)
{
    LPWSTR target_name;
    DWORD ret;
    DWORD type;
    DWORD count;
    LPCWSTR p;

    if (!filter) return TRUE;

    ret = RegQueryValueExW(hkeyCred, NULL, 0, &type, NULL, &count);
    if (ret != ERROR_SUCCESS)
        return FALSE;
    else if (type != REG_SZ)
        return FALSE;

    target_name = HeapAlloc(GetProcessHeap(), 0, count);
    if (!target_name)
        return FALSE;
    ret = RegQueryValueExW(hkeyCred, NULL, 0, &type, (LPVOID)target_name, &count);
    if (ret != ERROR_SUCCESS || type != REG_SZ)
    {
        HeapFree(GetProcessHeap(), 0, target_name);
        return FALSE;
    }

    TRACE("comparing filter %s to target name %s\n", debugstr_w(filter),
          debugstr_w(target_name));

    p = strchrW(filter, '*');
    ret = CompareStringW(GetThreadLocale(), 0, filter,
                         (p && !p[1] ? p - filter : -1), target_name,
                         (p && !p[1] ? p - filter : -1)) == CSTR_EQUAL;

    HeapFree(GetProcessHeap(), 0, target_name);
    return ret;
}

static DWORD registry_enumerate_credentials(HKEY hkeyMgr, LPCWSTR filter,
                                            LPWSTR target_name,
                                            DWORD target_name_len, BYTE key_data[KEY_SIZE],
                                            PCREDENTIALW *credentials, char *buffer,
                                            DWORD *len, DWORD *count)
{
    DWORD i;
    DWORD ret;
    for (i = 0;; i++)
    {
        HKEY hkeyCred;
        ret = RegEnumKeyW(hkeyMgr, i, target_name, target_name_len+1);
        if (ret == ERROR_NO_MORE_ITEMS)
        {
            ret = ERROR_SUCCESS;
            break;
        }
        else if (ret != ERROR_SUCCESS)
        {
            ret = ERROR_SUCCESS;
            continue;
        }
        TRACE("target_name = %s\n", debugstr_w(target_name));
        ret = RegOpenKeyExW(hkeyMgr, target_name, 0, KEY_QUERY_VALUE, &hkeyCred);
        if (ret != ERROR_SUCCESS)
        {
            ret = ERROR_SUCCESS;
            continue;
        }
        if (!credential_matches_filter(hkeyCred, filter))
        {
            RegCloseKey(hkeyCred);
            continue;
        }
        if (buffer)
        {
            *len = sizeof(CREDENTIALW);
            credentials[*count] = (PCREDENTIALW)buffer;
        }
        else
            *len += sizeof(CREDENTIALW);
        ret = registry_read_credential(hkeyCred, buffer ? credentials[*count] : NULL,
                                       key_data, buffer ? buffer + sizeof(CREDENTIALW) : NULL,
                                       len);
        RegCloseKey(hkeyCred);
        if (ret != ERROR_SUCCESS) break;
        if (buffer) buffer += *len;
        (*count)++;
    }
    return ret;
}

static void convert_PCREDENTIALW_to_PCREDENTIALA(const CREDENTIALW *CredentialW, PCREDENTIALA CredentialA, DWORD *len)
{
    char *buffer = (char *)CredentialA + sizeof(CREDENTIALA);
    INT string_len;

    *len += sizeof(CREDENTIALA);
    if (!CredentialA)
    {
        if (CredentialW->TargetName) *len += WideCharToMultiByte(CP_ACP, 0, CredentialW->TargetName, -1, NULL, 0, NULL, NULL);
        if (CredentialW->Comment) *len += WideCharToMultiByte(CP_ACP, 0, CredentialW->Comment, -1, NULL, 0, NULL, NULL);
        *len += CredentialW->CredentialBlobSize;
        if (CredentialW->TargetAlias) *len += WideCharToMultiByte(CP_ACP, 0, CredentialW->TargetAlias, -1, NULL, 0, NULL, NULL);
        if (CredentialW->UserName) *len += WideCharToMultiByte(CP_ACP, 0, CredentialW->UserName, -1, NULL, 0, NULL, NULL);

        return;
    }

    CredentialA->Flags = CredentialW->Flags;
    CredentialA->Type = CredentialW->Type;
    if (CredentialW->TargetName)
    {
        CredentialA->TargetName = (LPSTR)buffer;
        string_len = WideCharToMultiByte(CP_ACP, 0, CredentialW->TargetName, -1, CredentialA->TargetName, -1, NULL, NULL);
        buffer += string_len;
        *len += string_len;
    }
    else
        CredentialA->TargetName = NULL;
    if (CredentialW->Comment)
    {
        CredentialA->Comment = (LPSTR)buffer;
        string_len = WideCharToMultiByte(CP_ACP, 0, CredentialW->Comment, -1, CredentialA->Comment, -1, NULL, NULL);
        buffer += string_len;
        *len += string_len;
    }
    else
        CredentialA->Comment = NULL;
    CredentialA->LastWritten = CredentialW->LastWritten;
    CredentialA->CredentialBlobSize = CredentialW->CredentialBlobSize;
    if (CredentialW->CredentialBlobSize)
    {
        CredentialA->CredentialBlob =(LPBYTE)buffer;
        memcpy(CredentialA->CredentialBlob, CredentialW->CredentialBlob,
               CredentialW->CredentialBlobSize);
        buffer += CredentialW->CredentialBlobSize;
        *len += CredentialW->CredentialBlobSize;
    }
    else
        CredentialA->CredentialBlob = NULL;
    CredentialA->Persist = CredentialW->Persist;
    CredentialA->AttributeCount = 0;
    CredentialA->Attributes = NULL; /* FIXME */
    if (CredentialW->TargetAlias)
    {
        CredentialA->TargetAlias = (LPSTR)buffer;
        string_len = WideCharToMultiByte(CP_ACP, 0, CredentialW->TargetAlias, -1, CredentialA->TargetAlias, -1, NULL, NULL);
        buffer += string_len;
        *len += string_len;
    }
    else
        CredentialA->TargetAlias = NULL;
    if (CredentialW->UserName)
    {
        CredentialA->UserName = (LPSTR)buffer;
        string_len = WideCharToMultiByte(CP_ACP, 0, CredentialW->UserName, -1, CredentialA->UserName, -1, NULL, NULL);
        buffer += string_len;
        *len += string_len;
    }
    else
        CredentialA->UserName = NULL;
}

static void convert_PCREDENTIALA_to_PCREDENTIALW(const CREDENTIALA *CredentialA, PCREDENTIALW CredentialW, DWORD *len)
{
    char *buffer = (char *)CredentialW + sizeof(CREDENTIALW);
    INT string_len;

    *len += sizeof(CREDENTIALW);
    if (!CredentialW)
    {
        if (CredentialA->TargetName) *len += sizeof(WCHAR) * MultiByteToWideChar(CP_ACP, 0, CredentialA->TargetName, -1, NULL, 0);
        if (CredentialA->Comment) *len += sizeof(WCHAR) * MultiByteToWideChar(CP_ACP, 0, CredentialA->Comment, -1, NULL, 0);
        *len += CredentialA->CredentialBlobSize;
        if (CredentialA->TargetAlias) *len += sizeof(WCHAR) * MultiByteToWideChar(CP_ACP, 0, CredentialA->TargetAlias, -1, NULL, 0);
        if (CredentialA->UserName) *len += sizeof(WCHAR) * MultiByteToWideChar(CP_ACP, 0, CredentialA->UserName, -1, NULL, 0);

        return;
    }

    CredentialW->Flags = CredentialA->Flags;
    CredentialW->Type = CredentialA->Type;
    if (CredentialA->TargetName)
    {
        CredentialW->TargetName = (LPWSTR)buffer;
        string_len = MultiByteToWideChar(CP_ACP, 0, CredentialA->TargetName, -1, CredentialW->TargetName, -1);
        buffer += sizeof(WCHAR) * string_len;
        *len += sizeof(WCHAR) * string_len;
    }
    else
        CredentialW->TargetName = NULL;
    if (CredentialA->Comment)
    {
        CredentialW->Comment = (LPWSTR)buffer;
        string_len = MultiByteToWideChar(CP_ACP, 0, CredentialA->Comment, -1, CredentialW->Comment, -1);
        buffer += sizeof(WCHAR) * string_len;
        *len += sizeof(WCHAR) * string_len;
    }
    else
        CredentialW->Comment = NULL;
    CredentialW->LastWritten = CredentialA->LastWritten;
    CredentialW->CredentialBlobSize = CredentialA->CredentialBlobSize;
    if (CredentialA->CredentialBlobSize)
    {
        CredentialW->CredentialBlob =(LPBYTE)buffer;
        memcpy(CredentialW->CredentialBlob, CredentialA->CredentialBlob,
               CredentialA->CredentialBlobSize);
        buffer += CredentialA->CredentialBlobSize;
        *len += CredentialA->CredentialBlobSize;
    }
    else
        CredentialW->CredentialBlob = NULL;
    CredentialW->Persist = CredentialA->Persist;
    CredentialW->AttributeCount = 0;
    CredentialW->Attributes = NULL; /* FIXME */
    if (CredentialA->TargetAlias)
    {
        CredentialW->TargetAlias = (LPWSTR)buffer;
        string_len = MultiByteToWideChar(CP_ACP, 0, CredentialA->TargetAlias, -1, CredentialW->TargetAlias, -1);
        buffer += sizeof(WCHAR) * string_len;
        *len += sizeof(WCHAR) * string_len;
    }
    else
        CredentialW->TargetAlias = NULL;
    if (CredentialA->UserName)
    {
        CredentialW->UserName = (LPWSTR)buffer;
        string_len = MultiByteToWideChar(CP_ACP, 0, CredentialA->UserName, -1, CredentialW->UserName, -1);
        buffer += sizeof(WCHAR) * string_len;
        *len += sizeof(WCHAR) * string_len;
    }
    else
        CredentialW->UserName = NULL;
}

/******************************************************************************
 * CredDeleteA [ADVAPI32.@]
 */
BOOL WINAPI CredDeleteA(LPCSTR TargetName, DWORD Type, DWORD Flags)
{
    LPWSTR TargetNameW;
    DWORD len;
    BOOL ret;

    TRACE("(%s, %d, 0x%x)\n", debugstr_a(TargetName), Type, Flags);

    if (!TargetName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    len = MultiByteToWideChar(CP_ACP, 0, TargetName, -1, NULL, 0);
    TargetNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!TargetNameW)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    MultiByteToWideChar(CP_ACP, 0, TargetName, -1, TargetNameW, len);

    ret = CredDeleteW(TargetNameW, Type, Flags);

    HeapFree(GetProcessHeap(), 0, TargetNameW);

    return ret;
}

/******************************************************************************
 * CredDeleteW [ADVAPI32.@]
 */
BOOL WINAPI CredDeleteW(LPCWSTR TargetName, DWORD Type, DWORD Flags)
{
    HKEY hkeyMgr;
    DWORD ret;
    LPWSTR key_name;

    TRACE("(%s, %d, 0x%x)\n", debugstr_w(TargetName), Type, Flags);

    if (!TargetName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (Type != CRED_TYPE_GENERIC && Type != CRED_TYPE_DOMAIN_PASSWORD)
    {
        FIXME("unhandled type %d\n", Type);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (Flags)
    {
        FIXME("unhandled flags 0x%x\n", Flags);
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    ret = open_cred_mgr_key(&hkeyMgr, TRUE);
    if (ret != ERROR_SUCCESS)
    {
        WARN("couldn't open/create manager key, error %d\n", ret);
        SetLastError(ERROR_NO_SUCH_LOGON_SESSION);
        return FALSE;
    }

    key_name = get_key_name_for_target(TargetName, Type);
    ret = RegDeleteKeyW(hkeyMgr, key_name);
    HeapFree(GetProcessHeap(), 0, key_name);
    RegCloseKey(hkeyMgr);
    if (ret != ERROR_SUCCESS)
    {
        SetLastError(ERROR_NOT_FOUND);
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * CredEnumerateA [ADVAPI32.@]
 */
BOOL WINAPI CredEnumerateA(LPCSTR Filter, DWORD Flags, DWORD *Count,
                           PCREDENTIALA **Credentials)
{
    LPWSTR FilterW;
    PCREDENTIALW *CredentialsW;
    DWORD i;
    DWORD len;
    char *buffer;

    TRACE("(%s, 0x%x, %p, %p)\n", debugstr_a(Filter), Flags, Count, Credentials);

    if (Filter)
    {
        len = MultiByteToWideChar(CP_ACP, 0, Filter, -1, NULL, 0);
        FilterW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!FilterW)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
        MultiByteToWideChar(CP_ACP, 0, Filter, -1, FilterW, len);
    }
    else
        FilterW = NULL;

    if (!CredEnumerateW(FilterW, Flags, Count, &CredentialsW))
    {
        HeapFree(GetProcessHeap(), 0, FilterW);
        return FALSE;
    }
    HeapFree(GetProcessHeap(), 0, FilterW);

    len = *Count * sizeof(PCREDENTIALA);
    for (i = 0; i < *Count; i++)
        convert_PCREDENTIALW_to_PCREDENTIALA(CredentialsW[i], NULL, &len);

    *Credentials = HeapAlloc(GetProcessHeap(), 0, len);
    if (!*Credentials)
    {
        CredFree(CredentialsW);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    buffer = (char *)&(*Credentials)[*Count];
    for (i = 0; i < *Count; i++)
    {
        len = 0;
        (*Credentials)[i] = (PCREDENTIALA)buffer;
        convert_PCREDENTIALW_to_PCREDENTIALA(CredentialsW[i], (*Credentials)[i], &len);
        buffer += len;
    }

    CredFree(CredentialsW);

    return TRUE;
}

/******************************************************************************
 * CredEnumerateW [ADVAPI32.@]
 */
BOOL WINAPI CredEnumerateW(LPCWSTR Filter, DWORD Flags, DWORD *Count,
                           PCREDENTIALW **Credentials)
{
    HKEY hkeyMgr;
    DWORD ret;
    LPWSTR target_name;
    DWORD target_name_len;
    DWORD len;
    char *buffer;
    BYTE key_data[KEY_SIZE];

    TRACE("(%s, 0x%x, %p, %p)\n", debugstr_w(Filter), Flags, Count, Credentials);

    if (Flags)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    ret = open_cred_mgr_key(&hkeyMgr, FALSE);
    if (ret != ERROR_SUCCESS)
    {
        WARN("couldn't open/create manager key, error %d\n", ret);
        SetLastError(ERROR_NO_SUCH_LOGON_SESSION);
        return FALSE;
    }

    ret = get_cred_mgr_encryption_key(hkeyMgr, key_data);
    if (ret != ERROR_SUCCESS)
    {
        RegCloseKey(hkeyMgr);
        SetLastError(ret);
        return FALSE;
    }

    ret = RegQueryInfoKeyW(hkeyMgr, NULL, NULL, NULL, NULL, &target_name_len, NULL, NULL, NULL, NULL, NULL, NULL);
    if (ret != ERROR_SUCCESS)
    {
        RegCloseKey(hkeyMgr);
        SetLastError(ret);
        return FALSE;
    }

    target_name = HeapAlloc(GetProcessHeap(), 0, (target_name_len+1)*sizeof(WCHAR));
    if (!target_name)
    {
        RegCloseKey(hkeyMgr);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    *Count = 0;
    len = 0;
    ret = registry_enumerate_credentials(hkeyMgr, Filter, target_name, target_name_len,
                                         key_data, NULL, NULL, &len, Count);
    if (ret == ERROR_SUCCESS && *Count == 0)
        ret = ERROR_NOT_FOUND;
    if (ret != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, target_name);
        RegCloseKey(hkeyMgr);
        SetLastError(ret);
        return FALSE;
    }
    len += *Count + sizeof(PCREDENTIALW);

    if (ret == ERROR_SUCCESS)
    {
        buffer = HeapAlloc(GetProcessHeap(), 0, len);
        *Credentials = (PCREDENTIALW *)buffer;
        if (buffer)
        {
            buffer += *Count * sizeof(PCREDENTIALW);
            *Count = 0;
            ret = registry_enumerate_credentials(hkeyMgr, Filter, target_name,
                                                 target_name_len, key_data,
                                                 *Credentials, buffer, &len,
                                                 Count);
        }
        else
            ret = ERROR_OUTOFMEMORY;
    }

    HeapFree(GetProcessHeap(), 0, target_name);
    RegCloseKey(hkeyMgr);

    if (ret != ERROR_SUCCESS)
    {
        SetLastError(ret);
        return FALSE;
    }
    return TRUE;
}

/******************************************************************************
 * CredFree [ADVAPI32.@]
 */
VOID WINAPI CredFree(PVOID Buffer)
{
    HeapFree(GetProcessHeap(), 0, Buffer);
}

/******************************************************************************
 * CredReadA [ADVAPI32.@]
 */
BOOL WINAPI CredReadA(LPCSTR TargetName, DWORD Type, DWORD Flags, PCREDENTIALA *Credential)
{
    LPWSTR TargetNameW;
    PCREDENTIALW CredentialW;
    DWORD len;

    TRACE("(%s, %d, 0x%x, %p)\n", debugstr_a(TargetName), Type, Flags, Credential);

    if (!TargetName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    len = MultiByteToWideChar(CP_ACP, 0, TargetName, -1, NULL, 0);
    TargetNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!TargetNameW)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    MultiByteToWideChar(CP_ACP, 0, TargetName, -1, TargetNameW, len);

    if (!CredReadW(TargetNameW, Type, Flags, &CredentialW))
    {
        HeapFree(GetProcessHeap(), 0, TargetNameW);
        return FALSE;
    }
    HeapFree(GetProcessHeap(), 0, TargetNameW);

    len = 0;
    convert_PCREDENTIALW_to_PCREDENTIALA(CredentialW, NULL, &len);
    *Credential = HeapAlloc(GetProcessHeap(), 0, len);
    if (!*Credential)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    len = 0;
    convert_PCREDENTIALW_to_PCREDENTIALA(CredentialW, *Credential, &len);

    CredFree(CredentialW);

    return TRUE;
}

/******************************************************************************
 * CredReadW [ADVAPI32.@]
 */
BOOL WINAPI CredReadW(LPCWSTR TargetName, DWORD Type, DWORD Flags, PCREDENTIALW *Credential)
{
    HKEY hkeyMgr;
    HKEY hkeyCred;
    DWORD ret;
    LPWSTR key_name;
    DWORD len;
    BYTE key_data[KEY_SIZE];

    TRACE("(%s, %d, 0x%x, %p)\n", debugstr_w(TargetName), Type, Flags, Credential);

    if (!TargetName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (Type != CRED_TYPE_GENERIC && Type != CRED_TYPE_DOMAIN_PASSWORD)
    {
        FIXME("unhandled type %d\n", Type);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (Flags)
    {
        FIXME("unhandled flags 0x%x\n", Flags);
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    ret = open_cred_mgr_key(&hkeyMgr, FALSE);
    if (ret != ERROR_SUCCESS)
    {
        WARN("couldn't open/create manager key, error %d\n", ret);
        SetLastError(ERROR_NO_SUCH_LOGON_SESSION);
        return FALSE;
    }

    ret = get_cred_mgr_encryption_key(hkeyMgr, key_data);
    if (ret != ERROR_SUCCESS)
    {
        RegCloseKey(hkeyMgr);
        SetLastError(ret);
        return FALSE;
    }

    key_name = get_key_name_for_target(TargetName, Type);
    ret = RegOpenKeyExW(hkeyMgr, key_name, 0, KEY_QUERY_VALUE, &hkeyCred);
    HeapFree(GetProcessHeap(), 0, key_name);
    if (ret != ERROR_SUCCESS)
    {
        TRACE("credentials for target name %s not found\n", debugstr_w(TargetName));
        SetLastError(ERROR_NOT_FOUND);
        return FALSE;
    }

    len = sizeof(**Credential);
    ret = registry_read_credential(hkeyCred, NULL, key_data, NULL, &len);
    if (ret == ERROR_SUCCESS)
    {
        *Credential = HeapAlloc(GetProcessHeap(), 0, len);
        if (*Credential)
        {
            len = sizeof(**Credential);
            ret = registry_read_credential(hkeyCred, *Credential, key_data,
                                           (char *)(*Credential + 1), &len);
        }
        else
            ret = ERROR_OUTOFMEMORY;
    }

    RegCloseKey(hkeyCred);
    RegCloseKey(hkeyMgr);

    if (ret != ERROR_SUCCESS)
    {
        SetLastError(ret);
        return FALSE;
    }
    return TRUE;
}

/******************************************************************************
 * CredWriteA [ADVAPI32.@]
 */
BOOL WINAPI CredWriteA(PCREDENTIALA Credential, DWORD Flags)
{
    BOOL ret;
    DWORD len;
    PCREDENTIALW CredentialW;

    TRACE("(%p, 0x%x)\n", Credential, Flags);

    if (!Credential || !Credential->TargetName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    len = 0;
    convert_PCREDENTIALA_to_PCREDENTIALW(Credential, NULL, &len);
    CredentialW = HeapAlloc(GetProcessHeap(), 0, len);
    if (!CredentialW)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    len = 0;
    convert_PCREDENTIALA_to_PCREDENTIALW(Credential, CredentialW, &len);

    ret = CredWriteW(CredentialW, Flags);

    HeapFree(GetProcessHeap(), 0, CredentialW);

    return ret;
}

/******************************************************************************
 * CredWriteW [ADVAPI32.@]
 */
BOOL WINAPI CredWriteW(PCREDENTIALW Credential, DWORD Flags)
{
    HKEY hkeyMgr;
    HKEY hkeyCred;
    DWORD ret;
    LPWSTR key_name;
    BYTE key_data[KEY_SIZE];

    TRACE("(%p, 0x%x)\n", Credential, Flags);

    if (!Credential || !Credential->TargetName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (Flags & ~CRED_PRESERVE_CREDENTIAL_BLOB)
    {
        FIXME("unhandled flags 0x%x\n", Flags);
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    if (Credential->Type != CRED_TYPE_GENERIC && Credential->Type != CRED_TYPE_DOMAIN_PASSWORD)
    {
        FIXME("unhandled type %d\n", Credential->Type);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    TRACE("Credential->TargetName = %s\n", debugstr_w(Credential->TargetName));
    TRACE("Credential->UserName = %s\n", debugstr_w(Credential->UserName));

    if (Credential->Type == CRED_TYPE_DOMAIN_PASSWORD)
    {
        if (!Credential->UserName ||
            (!strchrW(Credential->UserName, '\\') && !strchrW(Credential->UserName, '@')))
        {
            ERR("bad username %s\n", debugstr_w(Credential->UserName));
            SetLastError(ERROR_BAD_USERNAME);
            return FALSE;
        }
    }

    ret = open_cred_mgr_key(&hkeyMgr, FALSE);
    if (ret != ERROR_SUCCESS)
    {
        WARN("couldn't open/create manager key, error %d\n", ret);
        SetLastError(ERROR_NO_SUCH_LOGON_SESSION);
        return FALSE;
    }

    ret = get_cred_mgr_encryption_key(hkeyMgr, key_data);
    if (ret != ERROR_SUCCESS)
    {
        RegCloseKey(hkeyMgr);
        SetLastError(ret);
        return FALSE;
    }

    key_name = get_key_name_for_target(Credential->TargetName, Credential->Type);
    ret = RegCreateKeyExW(hkeyMgr, key_name, 0, NULL, REG_OPTION_VOLATILE,
                          KEY_READ|KEY_WRITE, NULL, &hkeyCred, NULL);
    HeapFree(GetProcessHeap(), 0, key_name);
    if (ret != ERROR_SUCCESS)
    {
        TRACE("credentials for target name %s not found\n",
              debugstr_w(Credential->TargetName));
        SetLastError(ERROR_NOT_FOUND);
        return FALSE;
    }

    ret = registry_write_credential(hkeyCred, Credential, key_data,
                                    Flags & CRED_PRESERVE_CREDENTIAL_BLOB);

    RegCloseKey(hkeyCred);
    RegCloseKey(hkeyMgr);

    if (ret != ERROR_SUCCESS)
    {
        SetLastError(ret);
        return FALSE;
    }
    return TRUE;
}
