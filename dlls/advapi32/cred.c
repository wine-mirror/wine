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
#include <time.h>
#include <limits.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wincred.h"
#include "winternl.h"
#include "winioctl.h"
#define WINE_MOUNTMGR_EXTENSIONS
#include "ddk/mountmgr.h"

#include "crypt.h"

#include "wine/debug.h"

#include "advapi32_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(cred);

/* the size of the ARC4 key used to encrypt the password data */
#define KEY_SIZE 8

static DWORD read_credential_blob(HKEY hkey, const BYTE key_data[KEY_SIZE],
                                  LPBYTE credential_blob,
                                  DWORD *credential_blob_size)
{
    DWORD ret;
    DWORD type;

    *credential_blob_size = 0;
    ret = RegQueryValueExW(hkey, L"Password", 0, &type, NULL, credential_blob_size);
    if (ret != ERROR_SUCCESS)
        return ret;
    else if (type != REG_BINARY)
        return ERROR_REGISTRY_CORRUPT;
    if (credential_blob)
    {
        struct ustring data;
        struct ustring key;

        ret = RegQueryValueExW(hkey, L"Password", 0, &type, credential_blob,
                               credential_blob_size);
        if (ret != ERROR_SUCCESS)
            return ret;
        else if (type != REG_BINARY)
            return ERROR_REGISTRY_CORRUPT;

        key.Length = key.MaximumLength = KEY_SIZE;
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

    ret = RegQueryValueExW(hkey, L"Comment", 0, &type, NULL, &count);
    if (ret != ERROR_FILE_NOT_FOUND && ret != ERROR_SUCCESS)
        return ret;
    else if (type != REG_SZ)
        return ERROR_REGISTRY_CORRUPT;
    *len += count;
    if (credential)
    {
        credential->Comment = (LPWSTR)buffer;
        ret = RegQueryValueExW(hkey, L"Comment", 0, &type, (LPVOID)credential->Comment,
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

    ret = RegQueryValueExW(hkey, L"TargetAlias", 0, &type, NULL, &count);
    if (ret != ERROR_FILE_NOT_FOUND && ret != ERROR_SUCCESS)
        return ret;
    else if (type != REG_SZ)
        return ERROR_REGISTRY_CORRUPT;
    *len += count;
    if (credential)
    {
        credential->TargetAlias = (LPWSTR)buffer;
        ret = RegQueryValueExW(hkey, L"TargetAlias", 0, &type, (LPVOID)credential->TargetAlias,
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

    ret = RegQueryValueExW(hkey, L"UserName", 0, &type, NULL, &count);
    if (ret != ERROR_FILE_NOT_FOUND && ret != ERROR_SUCCESS)
        return ret;
    else if (type != REG_SZ)
        return ERROR_REGISTRY_CORRUPT;
    *len += count;
    if (credential)
    {
        credential->UserName = (LPWSTR)buffer;
        ret = RegQueryValueExW(hkey, L"UserName", 0, &type, (LPVOID)credential->UserName,
                               &count);
        if (ret == ERROR_FILE_NOT_FOUND)
            credential->UserName = NULL;
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
            credential->CredentialBlob = NULL;
        else if (ret != ERROR_SUCCESS)
            return ret;
        credential->CredentialBlobSize = count;
    }

    /* FIXME: Attributes */
    if (credential)
    {
        credential->AttributeCount = 0;
        credential->Attributes = NULL;
    }

    if (!credential) return ERROR_SUCCESS;

    count = sizeof(credential->Flags);
    ret = RegQueryValueExW(hkey, L"Flags", NULL, &type, (LPVOID)&credential->Flags,
                           &count);
    if (ret != ERROR_SUCCESS)
        return ret;
    else if (type != REG_DWORD)
        return ERROR_REGISTRY_CORRUPT;
    count = sizeof(credential->Type);
    ret = RegQueryValueExW(hkey, L"Type", NULL, &type, (LPVOID)&credential->Type,
                           &count);
    if (ret != ERROR_SUCCESS)
        return ret;
    else if (type != REG_DWORD)
        return ERROR_REGISTRY_CORRUPT;

    count = sizeof(credential->LastWritten);
    ret = RegQueryValueExW(hkey, L"LastWritten", NULL, &type, (LPVOID)&credential->LastWritten,
                           &count);
    if (ret != ERROR_SUCCESS)
        return ret;
    else if (type != REG_BINARY)
        return ERROR_REGISTRY_CORRUPT;
    count = sizeof(credential->Persist);
    ret = RegQueryValueExW(hkey, L"Persist", NULL, &type, (LPVOID)&credential->Persist,
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

    key.Length = key.MaximumLength = KEY_SIZE;
    key.Buffer = (unsigned char *)key_data;

    encrypted_credential_blob = malloc(credential_blob_size);
    if (!encrypted_credential_blob) return ERROR_OUTOFMEMORY;

    memcpy(encrypted_credential_blob, credential_blob, credential_blob_size);
    data.Length = data.MaximumLength = credential_blob_size;
    data.Buffer = encrypted_credential_blob;
    SystemFunction032(&data, &key);

    ret = RegSetValueExW(hkey, L"Password", 0, REG_BINARY, encrypted_credential_blob, credential_blob_size);
    free(encrypted_credential_blob);

    return ret;
}

static DWORD registry_write_credential(HKEY hkey, const CREDENTIALW *credential,
                                       const BYTE key_data[KEY_SIZE], BOOL preserve_blob)
{
    DWORD ret;
    FILETIME LastWritten;

    GetSystemTimeAsFileTime(&LastWritten);

    ret = RegSetValueExW(hkey, L"Flags", 0, REG_DWORD, (const BYTE*)&credential->Flags,
                         sizeof(credential->Flags));
    if (ret != ERROR_SUCCESS) return ret;
    ret = RegSetValueExW(hkey, L"Type", 0, REG_DWORD, (const BYTE*)&credential->Type,
                         sizeof(credential->Type));
    if (ret != ERROR_SUCCESS) return ret;
    ret = RegSetValueExW(hkey, NULL, 0, REG_SZ, (LPVOID)credential->TargetName,
                         sizeof(WCHAR)*(lstrlenW(credential->TargetName)+1));
    if (ret != ERROR_SUCCESS) return ret;
    if (credential->Comment)
    {
        ret = RegSetValueExW(hkey, L"Comment", 0, REG_SZ, (LPVOID)credential->Comment,
                             sizeof(WCHAR)*(lstrlenW(credential->Comment)+1));
        if (ret != ERROR_SUCCESS) return ret;
    }
    ret = RegSetValueExW(hkey, L"LastWritten", 0, REG_BINARY, (LPVOID)&LastWritten,
                         sizeof(LastWritten));
    if (ret != ERROR_SUCCESS) return ret;
    ret = RegSetValueExW(hkey, L"Persist", 0, REG_DWORD, (const BYTE*)&credential->Persist,
                         sizeof(credential->Persist));
    if (ret != ERROR_SUCCESS) return ret;
    /* FIXME: Attributes */
    if (credential->TargetAlias)
    {
        ret = RegSetValueExW(hkey, L"TargetAlias", 0, REG_SZ, (LPVOID)credential->TargetAlias,
                             sizeof(WCHAR)*(lstrlenW(credential->TargetAlias)+1));
        if (ret != ERROR_SUCCESS) return ret;
    }
    if (credential->UserName)
    {
        ret = RegSetValueExW(hkey, L"UserName", 0, REG_SZ, (LPVOID)credential->UserName,
                             sizeof(WCHAR)*(lstrlenW(credential->UserName)+1));
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

static DWORD host_write_credential( const CREDENTIALW *credential, BOOL preserve_blob )
{
    struct mountmgr_credential *cred;
    HANDLE mgr;
    DWORD size;
    WCHAR *ptr;
    BOOL ret;

    if (credential->Flags)
        FIXME( "flags 0x%lx not written\n", credential->Flags );
    if (credential->Type != CRED_TYPE_DOMAIN_PASSWORD)
        FIXME( "credential type of %ld not supported\n", credential->Type );
    if (credential->Persist != CRED_PERSIST_LOCAL_MACHINE)
        FIXME( "persist value of %ld not supported\n", credential->Persist );
    if (credential->AttributeCount)
        FIXME( "custom attributes not supported\n" );

    if (credential->CredentialBlobSize % sizeof(WCHAR)) return ERROR_NOT_SUPPORTED;

    mgr = CreateFileW( MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                       OPEN_EXISTING, 0, 0 );
    if (mgr == INVALID_HANDLE_VALUE) return GetLastError();

    size = sizeof(*cred) + (lstrlenW( credential->TargetName ) + lstrlenW( credential->UserName ) + 2) * sizeof(WCHAR);
    size += credential->CredentialBlobSize;
    if (credential->Comment) size += (lstrlenW( credential->Comment ) + 1) * sizeof(WCHAR);
    if (!(cred = malloc( size )))
    {
        CloseHandle( mgr );
        return ERROR_OUTOFMEMORY;
    }
    ptr = (WCHAR *)(cred + 1);

    cred->targetname_offset = sizeof(*cred);
    cred->targetname_size   = (lstrlenW( credential->TargetName ) + 1) * sizeof(WCHAR);
    lstrcpyW( ptr, credential->TargetName );
    ptr += cred->targetname_size / sizeof(WCHAR);

    cred->username_offset = cred->targetname_offset + cred->targetname_size;
    cred->username_size   = (lstrlenW( credential->UserName ) + 1) * sizeof(WCHAR);
    lstrcpyW( ptr, credential->UserName );
    ptr += cred->username_size / sizeof(WCHAR);

    cred->blob_offset = cred->username_offset + cred->username_size;
    if (credential->CredentialBlob)
    {
        cred->blob_size = credential->CredentialBlobSize;
        memcpy( ptr, credential->CredentialBlob, credential->CredentialBlobSize );
        ptr += cred->blob_size / sizeof(WCHAR);
    }
    else cred->blob_size = 0;
    cred->blob_preserve = preserve_blob;

    cred->comment_offset = cred->blob_offset + cred->blob_size;
    if (credential->Comment)
    {
        cred->comment_size = (lstrlenW( credential->Comment ) + 1) * sizeof(WCHAR);
        lstrcpyW( ptr, credential->Comment );
    }
    else cred->comment_size = 0;

    ret = DeviceIoControl( mgr, IOCTL_MOUNTMGR_WRITE_CREDENTIAL, cred, size, NULL, 0, NULL, NULL );
    free( cred );
    CloseHandle( mgr );

    return ret ? ERROR_SUCCESS : GetLastError();
}

static DWORD open_cred_mgr_key(HKEY *hkey, BOOL open_for_write)
{
    return RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Wine\\Credential Manager", 0,
                           NULL, REG_OPTION_NON_VOLATILE,
                           KEY_READ | (open_for_write ? KEY_WRITE : 0), NULL, hkey, NULL);
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
    ret = RegQueryValueExW(hkeyMgr, L"EncryptionKey", NULL, &type, key_data,
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

    ret = RegSetValueExW(hkeyMgr, L"EncryptionKey", 0, REG_BINARY,
                         key_data, KEY_SIZE);
    if (ret == ERROR_ACCESS_DENIED)
    {
        ret = open_cred_mgr_key(&hkeyMgr, TRUE);
        if (ret == ERROR_SUCCESS)
        {
            ret = RegSetValueExW(hkeyMgr, L"EncryptionKey", 0, REG_BINARY,
                                 key_data, KEY_SIZE);
            RegCloseKey(hkeyMgr);
        }
    }
    return ret;
}

static LPWSTR get_key_name_for_target(LPCWSTR target_name, DWORD type)
{
    INT len;
    LPCWSTR prefix = NULL;
    LPWSTR key_name, p;

    len = lstrlenW(target_name);
    if (type == CRED_TYPE_GENERIC)
    {
        prefix = L"Generic: ";
        len += ARRAY_SIZE(L"Generic: ");
    }
    else
    {
        prefix = L"DomPasswd: ";
        len += ARRAY_SIZE(L"DomPasswd: ");
    }

    key_name = malloc(len * sizeof(WCHAR));
    if (!key_name) return NULL;

    lstrcpyW(key_name, prefix);
    lstrcatW(key_name, target_name);

    for (p = key_name; *p; p++)
        if (*p == '\\') *p = '_';

    return key_name;
}

static BOOL registry_credential_matches_filter(HKEY hkeyCred, LPCWSTR filter)
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

    target_name = malloc(count);
    if (!target_name)
        return FALSE;
    ret = RegQueryValueExW(hkeyCred, NULL, 0, &type, (LPVOID)target_name, &count);
    if (ret != ERROR_SUCCESS || type != REG_SZ)
    {
        free(target_name);
        return FALSE;
    }

    TRACE("comparing filter %s to target name %s\n", debugstr_w(filter),
          debugstr_w(target_name));

    p = wcschr(filter, '*');
    ret = CompareStringW(GetThreadLocale(), NORM_IGNORECASE, filter,
                         (p && !p[1] ? p - filter : -1), target_name,
                         (p && !p[1] ? p - filter : -1)) == CSTR_EQUAL;

    free(target_name);
    return ret;
}

static DWORD registry_enumerate_credentials(HKEY hkeyMgr, LPCWSTR filter,
                                            LPWSTR target_name,
                                            DWORD target_name_len, const BYTE key_data[KEY_SIZE],
                                            PCREDENTIALW *credentials, char **buffer,
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
            continue;
        TRACE("target_name = %s\n", debugstr_w(target_name));
        ret = RegOpenKeyExW(hkeyMgr, target_name, 0, KEY_QUERY_VALUE, &hkeyCred);
        if (ret != ERROR_SUCCESS)
            continue;
        if (!registry_credential_matches_filter(hkeyCred, filter))
        {
            RegCloseKey(hkeyCred);
            continue;
        }
        if (buffer)
        {
            *len = sizeof(CREDENTIALW);
            credentials[*count] = (PCREDENTIALW)*buffer;
        }
        else
            *len += sizeof(CREDENTIALW);
        ret = registry_read_credential(hkeyCred, buffer ? credentials[*count] : NULL,
                                       key_data, buffer ? *buffer + sizeof(CREDENTIALW) : NULL,
                                       len);
        RegCloseKey(hkeyCred);
        if (ret != ERROR_SUCCESS) break;
        if (buffer) *buffer += *len;
        (*count)++;
    }
    return ret;
}

/******************************************************************************
 * convert_PCREDENTIALW_to_PCREDENTIALA [internal]
 *
 * convert a Credential struct from UNICODE to ANSI and return the needed size in Bytes
 *
 */

static INT convert_PCREDENTIALW_to_PCREDENTIALA(const CREDENTIALW *CredentialW, PCREDENTIALA CredentialA, DWORD len)
{
    char *buffer;
    INT string_len;
    INT needed = sizeof(CREDENTIALA);

    if (!CredentialA)
    {
        if (CredentialW->TargetName)
            needed += WideCharToMultiByte(CP_ACP, 0, CredentialW->TargetName, -1, NULL, 0, NULL, NULL);
        if (CredentialW->Comment)
            needed += WideCharToMultiByte(CP_ACP, 0, CredentialW->Comment, -1, NULL, 0, NULL, NULL);
        needed += CredentialW->CredentialBlobSize;
        if (CredentialW->TargetAlias)
            needed += WideCharToMultiByte(CP_ACP, 0, CredentialW->TargetAlias, -1, NULL, 0, NULL, NULL);
        if (CredentialW->UserName)
            needed += WideCharToMultiByte(CP_ACP, 0, CredentialW->UserName, -1, NULL, 0, NULL, NULL);

        return needed;
    }


    buffer = (char *)CredentialA + sizeof(CREDENTIALA);
    len -= sizeof(CREDENTIALA);
    CredentialA->Flags = CredentialW->Flags;
    CredentialA->Type = CredentialW->Type;

    if (CredentialW->TargetName)
    {
        CredentialA->TargetName = buffer;
        string_len = WideCharToMultiByte(CP_ACP, 0, CredentialW->TargetName, -1, buffer, len, NULL, NULL);
        buffer += string_len;
        needed += string_len;
        len -= string_len;
    }
    else
        CredentialA->TargetName = NULL;
    if (CredentialW->Comment)
    {
        CredentialA->Comment = buffer;
        string_len = WideCharToMultiByte(CP_ACP, 0, CredentialW->Comment, -1, buffer, len, NULL, NULL);
        buffer += string_len;
        needed += string_len;
        len -= string_len;
    }
    else
        CredentialA->Comment = NULL;
    CredentialA->LastWritten = CredentialW->LastWritten;
    CredentialA->CredentialBlobSize = CredentialW->CredentialBlobSize;
    if (CredentialW->CredentialBlobSize && (CredentialW->CredentialBlobSize <= len))
    {
        CredentialA->CredentialBlob =(LPBYTE)buffer;
        memcpy(CredentialA->CredentialBlob, CredentialW->CredentialBlob,
               CredentialW->CredentialBlobSize);
        buffer += CredentialW->CredentialBlobSize;
        needed += CredentialW->CredentialBlobSize;
        len -= CredentialW->CredentialBlobSize;
    }
    else
        CredentialA->CredentialBlob = NULL;
    CredentialA->Persist = CredentialW->Persist;
    CredentialA->AttributeCount = 0;
    CredentialA->Attributes = NULL; /* FIXME */
    if (CredentialW->TargetAlias)
    {
        CredentialA->TargetAlias = buffer;
        string_len = WideCharToMultiByte(CP_ACP, 0, CredentialW->TargetAlias, -1, buffer, len, NULL, NULL);
        buffer += string_len;
        needed += string_len;
        len -= string_len;
    }
    else
        CredentialA->TargetAlias = NULL;
    if (CredentialW->UserName)
    {
        CredentialA->UserName = buffer;
        string_len = WideCharToMultiByte(CP_ACP, 0, CredentialW->UserName, -1, buffer, len, NULL, NULL);
        needed += string_len;
    }
    else
        CredentialA->UserName = NULL;

    return needed;
}

/******************************************************************************
 * convert_PCREDENTIALA_to_PCREDENTIALW [internal]
 *
 * convert a Credential struct from ANSI to UNICODE and return the needed size in Bytes
 *
 */
static INT convert_PCREDENTIALA_to_PCREDENTIALW(const CREDENTIALA *CredentialA, PCREDENTIALW CredentialW, INT len)
{
    char *buffer;
    INT string_len;
    INT needed = sizeof(CREDENTIALW);

    if (!CredentialW)
    {
        if (CredentialA->TargetName)
            needed += sizeof(WCHAR) * MultiByteToWideChar(CP_ACP, 0, CredentialA->TargetName, -1, NULL, 0);
        if (CredentialA->Comment)
            needed += sizeof(WCHAR) * MultiByteToWideChar(CP_ACP, 0, CredentialA->Comment, -1, NULL, 0);
        needed += CredentialA->CredentialBlobSize;
        if (CredentialA->TargetAlias)
            needed += sizeof(WCHAR) * MultiByteToWideChar(CP_ACP, 0, CredentialA->TargetAlias, -1, NULL, 0);
        if (CredentialA->UserName)
            needed += sizeof(WCHAR) * MultiByteToWideChar(CP_ACP, 0, CredentialA->UserName, -1, NULL, 0);

        return needed;
    }

    buffer = (char *)CredentialW + sizeof(CREDENTIALW);
    len -= sizeof(CREDENTIALW);
    CredentialW->Flags = CredentialA->Flags;
    CredentialW->Type = CredentialA->Type;
    if (CredentialA->TargetName)
    {
        CredentialW->TargetName = (LPWSTR)buffer;
        string_len = MultiByteToWideChar(CP_ACP, 0, CredentialA->TargetName, -1, CredentialW->TargetName, len / sizeof(WCHAR));
        buffer += sizeof(WCHAR) * string_len;
        needed += sizeof(WCHAR) * string_len;
        len -= sizeof(WCHAR) * string_len;
    }
    else
        CredentialW->TargetName = NULL;
    if (CredentialA->Comment)
    {
        CredentialW->Comment = (LPWSTR)buffer;
        string_len = MultiByteToWideChar(CP_ACP, 0, CredentialA->Comment, -1, CredentialW->Comment, len / sizeof(WCHAR));
        buffer += sizeof(WCHAR) * string_len;
        needed += sizeof(WCHAR) * string_len;
        len -= sizeof(WCHAR) * string_len;
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
        needed += CredentialA->CredentialBlobSize;
        len -= CredentialA->CredentialBlobSize;
    }
    else
        CredentialW->CredentialBlob = NULL;
    CredentialW->Persist = CredentialA->Persist;
    CredentialW->AttributeCount = 0;
    CredentialW->Attributes = NULL; /* FIXME */
    if (CredentialA->TargetAlias)
    {
        CredentialW->TargetAlias = (LPWSTR)buffer;
        string_len = MultiByteToWideChar(CP_ACP, 0, CredentialA->TargetAlias, -1, CredentialW->TargetAlias, len / sizeof(WCHAR));
        buffer += sizeof(WCHAR) * string_len;
        needed += sizeof(WCHAR) * string_len;
        len -= sizeof(WCHAR) * string_len;
    }
    else
        CredentialW->TargetAlias = NULL;
    if (CredentialA->UserName)
    {
        CredentialW->UserName = (LPWSTR)buffer;
        string_len = MultiByteToWideChar(CP_ACP, 0, CredentialA->UserName, -1, CredentialW->UserName, len / sizeof(WCHAR));
        needed += sizeof(WCHAR) * string_len;
    }
    else
        CredentialW->UserName = NULL;

    return needed;
}

/******************************************************************************
 * CredDeleteA [ADVAPI32.@]
 */
BOOL WINAPI CredDeleteA(LPCSTR TargetName, DWORD Type, DWORD Flags)
{
    LPWSTR TargetNameW;
    DWORD len;
    BOOL ret;

    TRACE("(%s, %ld, 0x%lx)\n", debugstr_a(TargetName), Type, Flags);

    if (!TargetName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    len = MultiByteToWideChar(CP_ACP, 0, TargetName, -1, NULL, 0);
    TargetNameW = malloc(len * sizeof(WCHAR));
    if (!TargetNameW)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    MultiByteToWideChar(CP_ACP, 0, TargetName, -1, TargetNameW, len);

    ret = CredDeleteW(TargetNameW, Type, Flags);

    free(TargetNameW);

    return ret;
}

static DWORD host_delete_credential( const WCHAR *targetname )
{
    struct mountmgr_credential *cred;
    DWORD size, name_size = (lstrlenW( targetname ) + 1) * sizeof(WCHAR);
    HANDLE mgr;
    WCHAR *ptr;
    BOOL ret;

    mgr = CreateFileW( MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                       OPEN_EXISTING, 0, 0 );
    if (mgr == INVALID_HANDLE_VALUE) return GetLastError();

    size = sizeof(*cred) + name_size;
    if (!(cred = malloc( size )))
    {
        CloseHandle( mgr );
        return ERROR_OUTOFMEMORY;
    }
    cred->targetname_offset = sizeof(*cred);
    cred->targetname_size   = name_size;
    ptr = (WCHAR *)(cred + 1);
    lstrcpyW( ptr, targetname );

    ret = DeviceIoControl( mgr, IOCTL_MOUNTMGR_DELETE_CREDENTIAL, cred, size, NULL, 0, NULL, NULL );
    free( cred );
    CloseHandle( mgr );

    return ret ? ERROR_SUCCESS : GetLastError();
}

/******************************************************************************
 * CredDeleteW [ADVAPI32.@]
 */
BOOL WINAPI CredDeleteW(LPCWSTR TargetName, DWORD Type, DWORD Flags)
{
    HKEY hkeyMgr;
    DWORD ret;
    LPWSTR key_name;

    TRACE("(%s, %ld, 0x%lx)\n", debugstr_w(TargetName), Type, Flags);

    if (!TargetName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (Type != CRED_TYPE_GENERIC && Type != CRED_TYPE_DOMAIN_PASSWORD)
    {
        FIXME("unhandled type %ld\n", Type);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (Flags)
    {
        FIXME("unhandled flags 0x%lx\n", Flags);
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    if (Type == CRED_TYPE_DOMAIN_PASSWORD)
    {
        ret = host_delete_credential(TargetName);
        if (ret == ERROR_SUCCESS)
            return TRUE;
    }

    ret = open_cred_mgr_key(&hkeyMgr, TRUE);
    if (ret != ERROR_SUCCESS)
    {
        WARN("couldn't open/create manager key, error %ld\n", ret);
        SetLastError(ERROR_NO_SUCH_LOGON_SESSION);
        return FALSE;
    }

    key_name = get_key_name_for_target(TargetName, Type);
    ret = RegDeleteKeyW(hkeyMgr, key_name);
    free(key_name);
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
    INT len;
    INT needed;
    char *buffer;

    TRACE("(%s, 0x%lx, %p, %p)\n", debugstr_a(Filter), Flags, Count, Credentials);

    if (Filter)
    {
        len = MultiByteToWideChar(CP_ACP, 0, Filter, -1, NULL, 0);
        FilterW = malloc(len * sizeof(WCHAR));
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
        free(FilterW);
        return FALSE;
    }
    free(FilterW);

    len = *Count * sizeof(PCREDENTIALA);
    for (i = 0; i < *Count; i++)
        len += convert_PCREDENTIALW_to_PCREDENTIALA(CredentialsW[i], NULL, 0);

    *Credentials = malloc(len);
    if (!*Credentials)
    {
        CredFree(CredentialsW);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    buffer = (char *)&(*Credentials)[*Count];
    len -= *Count * sizeof(PCREDENTIALA);
    for (i = 0; i < *Count; i++)
    {
        (*Credentials)[i] = (PCREDENTIALA)buffer;
        needed = convert_PCREDENTIALW_to_PCREDENTIALA(CredentialsW[i], (*Credentials)[i], len);
        buffer += needed;
        len -= needed;
    }

    CredFree(CredentialsW);

    return TRUE;
}

#define CRED_LIST_COUNT 16
#define CRED_DATA_SIZE  2048
static DWORD host_enumerate_credentials( const WCHAR *filter, CREDENTIALW **credentials, char *buf, DWORD *len, DWORD *count )
{
    struct mountmgr_credential_list *list, *tmp;
    DWORD i, j, ret, size, filter_size, offset = 0;
    HANDLE mgr;
    WCHAR *ptr;

    if (filter) filter_size = (lstrlenW( filter ) + 1) * sizeof(WCHAR);
    else
    {
        filter = L"";
        filter_size = sizeof(L"");
    }

    mgr = CreateFileW( MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
    if (mgr == INVALID_HANDLE_VALUE) return GetLastError();

    size = FIELD_OFFSET( struct mountmgr_credential_list, creds[CRED_LIST_COUNT] ) + filter_size + CRED_DATA_SIZE;
    if (!(list = malloc( size )))
    {
        CloseHandle( mgr );
        return ERROR_OUTOFMEMORY;
    }

    for (;;)
    {
        list->filter_offset = sizeof(*list);
        list->filter_size   = filter_size;
        ptr = (WCHAR *)((char *)list + list->filter_offset);
        lstrcpyW( ptr, filter );

        if (DeviceIoControl( mgr, IOCTL_MOUNTMGR_ENUMERATE_CREDENTIALS, list, size, list, size, NULL, NULL )) break;
        if ((ret = GetLastError()) != ERROR_MORE_DATA) goto done;

        size = list->size + filter_size;
        if (!(tmp = realloc( list, size )))
        {
            ret = ERROR_OUTOFMEMORY;
            goto done;
        }
        list = tmp;
    }

    for (i = 0, j = *count; i < list->count; i++)
    {
        CREDENTIALW *cred = (CREDENTIALW *)(buf + offset);

        offset += sizeof(*cred) + list->creds[i].targetname_size + list->creds[i].comment_size + list->creds[i].blob_size +
                  list->creds[i].username_size;
        if (!buf) continue;

        ptr = (WCHAR *)(cred + 1);
        cred->Flags      = 0;
        cred->Type       = CRED_TYPE_DOMAIN_PASSWORD;
        cred->TargetName = ptr;
        memcpy( cred->TargetName, (char *)&list->creds[i] + list->creds[i].targetname_offset, list->creds[i].targetname_size );
        ptr += list->creds[i].targetname_size / sizeof(WCHAR);
        if (list->creds[i].comment_size)
        {
            cred->Comment = ptr;
            memcpy( cred->Comment, (char *)&list->creds[i] + list->creds[i].comment_offset, list->creds[i].comment_size );
            ptr += list->creds[i].comment_size / sizeof(WCHAR);
        }
        else cred->Comment = NULL;
        cred->LastWritten = list->creds[i].last_written;
        if (list->creds[i].blob_size)
        {
            cred->CredentialBlobSize = list->creds[i].blob_size;
            cred->CredentialBlob     = (BYTE *)ptr;
            memcpy( cred->CredentialBlob, (char *)&list->creds[i] + list->creds[i].blob_offset, list->creds[i].blob_size );
            ptr += list->creds[i].blob_size / sizeof(WCHAR);
        }
        else
        {
            cred->CredentialBlobSize = 0;
            cred->CredentialBlob     = NULL;
        }
        cred->Persist        = CRED_PERSIST_LOCAL_MACHINE;
        cred->AttributeCount = 0;
        cred->Attributes     = NULL;
        cred->TargetAlias    = NULL;
        if (list->creds[i].username_size)
        {
            cred->UserName = ptr;
            memcpy( cred->UserName, (char *)&list->creds[i] + list->creds[i].username_offset, list->creds[i].username_size );
        }
        else cred->UserName = NULL;
        if (credentials) credentials[j++] = cred;
    }

    *len += offset;
    *count += list->count;
    ret = ERROR_SUCCESS;

done:
    free( list );
    CloseHandle( mgr );
    return ret;
}

/******************************************************************************
 * CredEnumerateW [ADVAPI32.@]
 */
BOOL WINAPI CredEnumerateW(LPCWSTR Filter, DWORD Flags, DWORD *Count, PCREDENTIALW **Credentials)
{
    HKEY hkeyMgr;
    DWORD ret;
    LPWSTR target_name;
    DWORD target_name_len;
    DWORD len;
    char *buffer;
    BYTE key_data[KEY_SIZE];

    TRACE("(%s, 0x%lx, %p, %p)\n", debugstr_w(Filter), Flags, Count, Credentials);

    if (Flags)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    ret = open_cred_mgr_key(&hkeyMgr, FALSE);
    if (ret != ERROR_SUCCESS)
    {
        WARN("couldn't open/create manager key, error %ld\n", ret);
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

    target_name = malloc((target_name_len + 1) * sizeof(WCHAR));
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
    if (ret == ERROR_SUCCESS)
    {
        ret = host_enumerate_credentials(Filter, NULL, NULL, &len, Count);
        if (ret == ERROR_NOT_SUPPORTED) ret = ERROR_SUCCESS;
    }
    if (ret == ERROR_SUCCESS && *Count == 0)
        ret = ERROR_NOT_FOUND;
    if (ret != ERROR_SUCCESS)
    {
        free(target_name);
        RegCloseKey(hkeyMgr);
        SetLastError(ret);
        return FALSE;
    }
    len += *Count * sizeof(PCREDENTIALW);

    if (ret == ERROR_SUCCESS)
    {
        buffer = malloc(len);
        *Credentials = (PCREDENTIALW *)buffer;
        if (buffer)
        {
            buffer += *Count * sizeof(PCREDENTIALW);
            *Count = 0;
            ret = registry_enumerate_credentials(hkeyMgr, Filter, target_name, target_name_len, key_data,
                                                 *Credentials, &buffer, &len, Count);
            if (ret == ERROR_SUCCESS)
            {
                ret = host_enumerate_credentials(Filter, *Credentials, buffer, &len, Count);
                if (ret == ERROR_NOT_SUPPORTED) ret = ERROR_SUCCESS;
            }
        }
        else ret = ERROR_OUTOFMEMORY;
    }

    free(target_name);
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
    free(Buffer);
}

/******************************************************************************
 * CredReadA [ADVAPI32.@]
 */
BOOL WINAPI CredReadA(LPCSTR TargetName, DWORD Type, DWORD Flags, PCREDENTIALA *Credential)
{
    LPWSTR TargetNameW;
    PCREDENTIALW CredentialW;
    INT len;

    TRACE("(%s, %ld, 0x%lx, %p)\n", debugstr_a(TargetName), Type, Flags, Credential);

    if (!TargetName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    len = MultiByteToWideChar(CP_ACP, 0, TargetName, -1, NULL, 0);
    TargetNameW = malloc(len * sizeof(WCHAR));
    if (!TargetNameW)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    MultiByteToWideChar(CP_ACP, 0, TargetName, -1, TargetNameW, len);

    if (!CredReadW(TargetNameW, Type, Flags, &CredentialW))
    {
        free(TargetNameW);
        return FALSE;
    }
    free(TargetNameW);

    len = convert_PCREDENTIALW_to_PCREDENTIALA(CredentialW, NULL, 0);
    *Credential = malloc(len);
    if (!*Credential)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    convert_PCREDENTIALW_to_PCREDENTIALA(CredentialW, *Credential, len);

    CredFree(CredentialW);

    return TRUE;
}

static DWORD host_read_credential( const WCHAR *targetname, CREDENTIALW **ret_credential )
{
    struct mountmgr_credential *cred_in, *cred_out = NULL, *tmp;
    DWORD err = ERROR_OUTOFMEMORY, size_in, size_out, size_name = (lstrlenW( targetname ) + 1) * sizeof(WCHAR);
    HANDLE mgr;
    WCHAR *ptr;
    BOOL ret;

    mgr = CreateFileW( MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                       OPEN_EXISTING, 0, 0 );
    if (mgr == INVALID_HANDLE_VALUE) return GetLastError();

    size_in = sizeof(*cred_in) + size_name;
    if (!(cred_in = malloc( size_in )))
    {
        CloseHandle( mgr );
        return ERROR_OUTOFMEMORY;
    }
    cred_in->targetname_offset = sizeof(*cred_in);
    cred_in->targetname_size   = size_name;
    ptr = (WCHAR *)(cred_in + 1);
    lstrcpyW( ptr, targetname );

    size_out = 256;
    if (!(cred_out = malloc( size_out ))) goto done;

    for (;;)
    {
        ret = DeviceIoControl( mgr, IOCTL_MOUNTMGR_READ_CREDENTIAL, cred_in, size_in, cred_out, size_out, NULL, NULL );
        if (ret || (err = GetLastError()) != ERROR_MORE_DATA) break;
        size_out *= 2;
        if (!(tmp = realloc( cred_out, size_out ))) goto done;
        cred_out = tmp;
    }

    if (ret)
    {
        CREDENTIALW *credential;
        DWORD size = sizeof(*credential) + cred_out->targetname_size + cred_out->username_size + cred_out->comment_size +
                     cred_out->blob_size;

        if (!(credential = calloc( 1, size )))
        {
            err = ERROR_OUTOFMEMORY;
            goto done;
        }
        ptr = (WCHAR *)(credential + 1);

        credential->Type = CRED_TYPE_DOMAIN_PASSWORD;
        memcpy( ptr, (char *)cred_out + cred_out->targetname_offset, cred_out->targetname_size );
        credential->TargetName = ptr;
        ptr += lstrlenW( ptr ) + 1;
        if (cred_out->comment_size)
        {
            memcpy( ptr, (char *)cred_out + cred_out->comment_offset, cred_out->comment_size );
            credential->Comment = ptr;
            ptr += lstrlenW( ptr ) + 1;
        }
        credential->LastWritten = cred_out->last_written;
        if ((credential->CredentialBlobSize = cred_out->blob_size))
        {
            memcpy( ptr, (char *)cred_out + cred_out->blob_offset, cred_out->blob_size );
            credential->CredentialBlob = (BYTE *)ptr;
            ptr += cred_out->blob_size / sizeof(WCHAR);
        }
        credential->Persist = CRED_PERSIST_LOCAL_MACHINE;
        memcpy( ptr, (char *)cred_out + cred_out->username_offset, cred_out->username_size );
        credential->UserName = ptr;

        *ret_credential = credential;
        err = ERROR_SUCCESS;
    }

done:
    free( cred_in );
    free( cred_out );
    CloseHandle( mgr );
    return err;
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

    TRACE("(%s, %ld, 0x%lx, %p)\n", debugstr_w(TargetName), Type, Flags, Credential);

    if (!TargetName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (Type != CRED_TYPE_GENERIC && Type != CRED_TYPE_DOMAIN_PASSWORD)
    {
        FIXME("unhandled type %ld\n", Type);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (Flags)
    {
        FIXME("unhandled flags 0x%lx\n", Flags);
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    if (Type == CRED_TYPE_DOMAIN_PASSWORD)
    {
        ret = host_read_credential( TargetName, Credential );
        if (ret != ERROR_SUCCESS && ret != ERROR_NOT_SUPPORTED)
        {
            SetLastError(ret);
            return FALSE;
        }
        if (ret == ERROR_SUCCESS) return TRUE;
    }

    ret = open_cred_mgr_key(&hkeyMgr, FALSE);
    if (ret != ERROR_SUCCESS)
    {
        WARN("couldn't open/create manager key, error %ld\n", ret);
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
    free(key_name);
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
        *Credential = malloc(len);
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
 * CredReadDomainCredentialsA [ADVAPI32.@]
 */
BOOL WINAPI CredReadDomainCredentialsA(PCREDENTIAL_TARGET_INFORMATIONA TargetInformation,
                                       DWORD Flags, DWORD *Size, PCREDENTIALA **Credentials)
{
    PCREDENTIAL_TARGET_INFORMATIONW TargetInformationW;
    INT len;
    DWORD i;
    WCHAR *buffer, *end;
    BOOL ret;
    PCREDENTIALW* CredentialsW;

    TRACE("(%p, 0x%lx, %p, %p)\n", TargetInformation, Flags, Size, Credentials);

    /* follow Windows behavior - do not test for NULL, initialize early */
    *Size = 0;
    *Credentials = NULL;

    if (!TargetInformation)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    len = sizeof(*TargetInformationW);
    if (TargetInformation->TargetName)
        len += MultiByteToWideChar(CP_ACP, 0, TargetInformation->TargetName, -1, NULL, 0) * sizeof(WCHAR);
    if (TargetInformation->NetbiosServerName)
        len += MultiByteToWideChar(CP_ACP, 0, TargetInformation->NetbiosServerName, -1, NULL, 0) * sizeof(WCHAR);
    if (TargetInformation->DnsServerName)
        len += MultiByteToWideChar(CP_ACP, 0, TargetInformation->DnsServerName, -1, NULL, 0) * sizeof(WCHAR);
    if (TargetInformation->NetbiosDomainName)
        len += MultiByteToWideChar(CP_ACP, 0, TargetInformation->NetbiosDomainName, -1, NULL, 0) * sizeof(WCHAR);
    if (TargetInformation->DnsDomainName)
        len += MultiByteToWideChar(CP_ACP, 0, TargetInformation->DnsDomainName, -1, NULL, 0) * sizeof(WCHAR);
    if (TargetInformation->DnsTreeName)
        len += MultiByteToWideChar(CP_ACP, 0, TargetInformation->DnsTreeName, -1, NULL, 0) * sizeof(WCHAR);
    if (TargetInformation->PackageName)
        len += MultiByteToWideChar(CP_ACP, 0, TargetInformation->PackageName, -1, NULL, 0) * sizeof(WCHAR);

    TargetInformationW = malloc(len);
    if (!TargetInformationW)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    buffer = (WCHAR*)(TargetInformationW + 1);
    end = (WCHAR *)((char *)TargetInformationW + len);

    if (TargetInformation->TargetName)
    {
        TargetInformationW->TargetName = buffer;
        buffer += MultiByteToWideChar(CP_ACP, 0, TargetInformation->TargetName, -1,
                                      TargetInformationW->TargetName, end - buffer);
    } else
        TargetInformationW->TargetName = NULL;

    if (TargetInformation->NetbiosServerName)
    {
        TargetInformationW->NetbiosServerName = buffer;
        buffer += MultiByteToWideChar(CP_ACP, 0, TargetInformation->NetbiosServerName, -1,
                                      TargetInformationW->NetbiosServerName, end - buffer);
    } else
        TargetInformationW->NetbiosServerName = NULL;

    if (TargetInformation->DnsServerName)
    {
        TargetInformationW->DnsServerName = buffer;
        buffer += MultiByteToWideChar(CP_ACP, 0, TargetInformation->DnsServerName, -1,
                                      TargetInformationW->DnsServerName, end - buffer);
    } else
        TargetInformationW->DnsServerName = NULL;

    if (TargetInformation->NetbiosDomainName)
    {
        TargetInformationW->NetbiosDomainName = buffer;
        buffer += MultiByteToWideChar(CP_ACP, 0, TargetInformation->NetbiosDomainName, -1,
                                      TargetInformationW->NetbiosDomainName, end - buffer);
    } else
        TargetInformationW->NetbiosDomainName = NULL;

    if (TargetInformation->DnsDomainName)
    {
        TargetInformationW->DnsDomainName = buffer;
        buffer += MultiByteToWideChar(CP_ACP, 0, TargetInformation->DnsDomainName, -1,
                                      TargetInformationW->DnsDomainName, end - buffer);
    } else
        TargetInformationW->DnsDomainName = NULL;

    if (TargetInformation->DnsTreeName)
    {
        TargetInformationW->DnsTreeName = buffer;
        buffer += MultiByteToWideChar(CP_ACP, 0, TargetInformation->DnsTreeName, -1,
                                      TargetInformationW->DnsTreeName, end - buffer);
    } else
        TargetInformationW->DnsTreeName = NULL;

    if (TargetInformation->PackageName)
    {
        TargetInformationW->PackageName = buffer;
        MultiByteToWideChar(CP_ACP, 0, TargetInformation->PackageName, -1,
                            TargetInformationW->PackageName, end - buffer);
    } else
        TargetInformationW->PackageName = NULL;

    TargetInformationW->Flags = TargetInformation->Flags;
    TargetInformationW->CredTypeCount = TargetInformation->CredTypeCount;
    TargetInformationW->CredTypes = TargetInformation->CredTypes;

    ret = CredReadDomainCredentialsW(TargetInformationW, Flags, Size, &CredentialsW);

    free(TargetInformationW);

    if (ret)
    {
        char *buf;
        INT needed;

        len = *Size * sizeof(PCREDENTIALA);
        for (i = 0; i < *Size; i++)
            len += convert_PCREDENTIALW_to_PCREDENTIALA(CredentialsW[i], NULL, 0);

        *Credentials = malloc(len);
        if (!*Credentials)
        {
            CredFree(CredentialsW);
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        buf = (char *)&(*Credentials)[*Size];
        len -= *Size * sizeof(PCREDENTIALA);
        for (i = 0; i < *Size; i++)
        {
            (*Credentials)[i] = (PCREDENTIALA)buf;
            needed = convert_PCREDENTIALW_to_PCREDENTIALA(CredentialsW[i], (*Credentials)[i], len);
            buf += needed;
            len -= needed;
        }

        CredFree(CredentialsW);
    }
    return ret;
}

/******************************************************************************
 * CredReadDomainCredentialsW [ADVAPI32.@]
 */
BOOL WINAPI CredReadDomainCredentialsW(PCREDENTIAL_TARGET_INFORMATIONW TargetInformation, DWORD Flags,
                                       DWORD *Size, PCREDENTIALW **Credentials)
{
    FIXME("(%p, 0x%lx, %p, %p) stub\n", TargetInformation, Flags, Size, Credentials);

    /* follow Windows behavior - do not test for NULL, initialize early */
    *Size = 0;
    *Credentials = NULL;
    if (!TargetInformation)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SetLastError(ERROR_NOT_FOUND);
    return FALSE;
}

/******************************************************************************
 * CredWriteA [ADVAPI32.@]
 */
BOOL WINAPI CredWriteA(PCREDENTIALA Credential, DWORD Flags)
{
    BOOL ret;
    INT len;
    PCREDENTIALW CredentialW;

    TRACE("(%p, 0x%lx)\n", Credential, Flags);

    if (!Credential || !Credential->TargetName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    len = convert_PCREDENTIALA_to_PCREDENTIALW(Credential, NULL, 0);
    CredentialW = malloc(len);
    if (!CredentialW)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    convert_PCREDENTIALA_to_PCREDENTIALW(Credential, CredentialW, len);

    ret = CredWriteW(CredentialW, Flags);

    free(CredentialW);

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

    TRACE("(%p, 0x%lx)\n", Credential, Flags);

    if (!Credential || !Credential->TargetName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (Flags & ~CRED_PRESERVE_CREDENTIAL_BLOB)
    {
        FIXME("unhandled flags 0x%lx\n", Flags);
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    if (Credential->Type != CRED_TYPE_GENERIC && Credential->Type != CRED_TYPE_DOMAIN_PASSWORD)
    {
        FIXME("unhandled type %ld\n", Credential->Type);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    TRACE("Credential->Flags = 0x%08lx\n", Credential->Flags);
    TRACE("Credential->Type = %lu\n", Credential->Type);
    TRACE("Credential->TargetName = %s\n", debugstr_w(Credential->TargetName));
    TRACE("Credential->Comment = %s\n", debugstr_w(Credential->Comment));
    TRACE("Credential->Persist = %lu\n", Credential->Persist);
    TRACE("Credential->TargetAlias = %s\n", debugstr_w(Credential->TargetAlias));
    TRACE("Credential->UserName = %s\n", debugstr_w(Credential->UserName));

    if (Credential->Type == CRED_TYPE_DOMAIN_PASSWORD)
    {
        if (!Credential->UserName ||
            (Credential->Persist == CRED_PERSIST_ENTERPRISE &&
            (!wcschr(Credential->UserName, '\\') && !wcschr(Credential->UserName, '@'))))
        {
            ERR("bad username %s\n", debugstr_w(Credential->UserName));
            SetLastError(ERROR_BAD_USERNAME);
            return FALSE;
        }
    }

    if (!Credential->AttributeCount &&
        Credential->Type == CRED_TYPE_DOMAIN_PASSWORD &&
        (Credential->Persist == CRED_PERSIST_LOCAL_MACHINE || Credential->Persist == CRED_PERSIST_ENTERPRISE))
    {
        ret = host_write_credential(Credential, Flags & CRED_PRESERVE_CREDENTIAL_BLOB);
        if (ret != ERROR_SUCCESS && ret != ERROR_NOT_SUPPORTED)
        {
            SetLastError(ret);
            return FALSE;
        }
        if (ret == ERROR_SUCCESS) return TRUE;
    }

    ret = open_cred_mgr_key(&hkeyMgr, FALSE);
    if (ret != ERROR_SUCCESS)
    {
        WARN("couldn't open/create manager key, error %ld\n", ret);
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
    ret = RegCreateKeyExW(hkeyMgr, key_name, 0, NULL,
                          Credential->Persist == CRED_PERSIST_SESSION ? REG_OPTION_VOLATILE : REG_OPTION_NON_VOLATILE,
                          KEY_READ|KEY_WRITE, NULL, &hkeyCred, NULL);
    free(key_name);
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

/******************************************************************************
 * CredGetSessionTypes [ADVAPI32.@]
 */
WINADVAPI BOOL WINAPI CredGetSessionTypes(DWORD persistCount, LPDWORD persists)
{
    TRACE("(%lu, %p)\n", persistCount, persists);

    memset(persists, CRED_PERSIST_NONE, persistCount*sizeof(*persists));
    if (CRED_TYPE_GENERIC < persistCount)
    {
        persists[CRED_TYPE_GENERIC] = CRED_PERSIST_ENTERPRISE;

        if (CRED_TYPE_DOMAIN_PASSWORD < persistCount)
        {
            persists[CRED_TYPE_DOMAIN_PASSWORD] = CRED_PERSIST_ENTERPRISE;
        }
    }
    return TRUE;
}

/******************************************************************************
 * CredMarshalCredentialA [ADVAPI32.@]
 */
BOOL WINAPI CredMarshalCredentialA( CRED_MARSHAL_TYPE type, PVOID cred, LPSTR *out )
{
    BOOL ret;
    WCHAR *outW;

    TRACE("%u, %p, %p\n", type, cred, out);

    if ((ret = CredMarshalCredentialW( type, cred, &outW )))
    {
        int len = WideCharToMultiByte( CP_ACP, 0, outW, -1, NULL, 0, NULL, NULL );
        if (!(*out = malloc( len )))
        {
            free( outW );
            return FALSE;
        }
        WideCharToMultiByte( CP_ACP, 0, outW, -1, *out, len, NULL, NULL );
        free( outW );
    }
    return ret;
}

static UINT cred_encode( const char *bin, unsigned int len, WCHAR *cred )
{
    static const char enc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789#-";
    UINT n = 0, x;

    while (len > 0)
    {
        cred[n++] = enc[bin[0] & 0x3f];
        x = (bin[0] & 0xc0) >> 6;
        if (len == 1)
        {
            cred[n++] = enc[x];
            break;
        }
        cred[n++] = enc[((bin[1] & 0xf) << 2) | x];
        x = (bin[1] & 0xf0) >> 4;
        if (len == 2)
        {
            cred[n++] = enc[x];
            break;
        }
        cred[n++] = enc[((bin[2] & 0x3) << 4) | x];
        cred[n++] = enc[(bin[2] & 0xfc) >> 2];
        bin += 3;
        len -= 3;
    }
    return n;
}

/******************************************************************************
 * CredMarshalCredentialW [ADVAPI32.@]
 */
BOOL WINAPI CredMarshalCredentialW( CRED_MARSHAL_TYPE type, PVOID cred, LPWSTR *out )
{
    CERT_CREDENTIAL_INFO *cert = cred;
    USERNAME_TARGET_CREDENTIAL_INFO *target = cred;
    DWORD len, size;
    WCHAR *p;

    TRACE("%u, %p, %p\n", type, cred, out);

    if (!cred || (type == CertCredential && cert->cbSize < sizeof(*cert)) ||
        (type != CertCredential && type != UsernameTargetCredential && type != BinaryBlobCredential) ||
        (type == UsernameTargetCredential && (!target->UserName || !target->UserName[0])))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    switch (type)
    {
    case CertCredential:
    {
        size = (sizeof(cert->rgbHashOfCert) + 2) * 4 / 3;
        if (!(p = malloc( (size + 4) * sizeof(WCHAR) ))) return FALSE;
        p[0] = '@';
        p[1] = '@';
        p[2] = 'A' + type;
        len = cred_encode( (const char *)cert->rgbHashOfCert, sizeof(cert->rgbHashOfCert), p + 3 );
        p[len + 3] = 0;
        break;
    }
    case UsernameTargetCredential:
    {
        len = lstrlenW( target->UserName );
        size = (sizeof(DWORD) + len * sizeof(WCHAR) + 2) * 4 / 3;
        if (!(p = malloc( (size + 4) * sizeof(WCHAR) ))) return FALSE;
        p[0] = '@';
        p[1] = '@';
        p[2] = 'A' + type;
        size = len * sizeof(WCHAR);
        len = cred_encode( (const char *)&size, sizeof(DWORD), p + 3 );
        len += cred_encode( (const char *)target->UserName, size, p + 3 + len );
        p[len + 3] = 0;
        break;
    }
    case BinaryBlobCredential:
        FIXME("BinaryBlobCredential not implemented\n");
        return FALSE;
    default:
        return FALSE;
    }
    *out = p;
    return TRUE;
}

/******************************************************************************
 * CredUnmarshalCredentialA [ADVAPI32.@]
 */
BOOL WINAPI CredUnmarshalCredentialA( LPCSTR cred, PCRED_MARSHAL_TYPE type, PVOID *out )
{
    BOOL ret;
    WCHAR *credW = NULL;

    TRACE("%s, %p, %p\n", debugstr_a(cred), type, out);

    if (cred)
    {
        int len = MultiByteToWideChar( CP_ACP, 0, cred, -1, NULL, 0 );
        if (!(credW = malloc( len * sizeof(WCHAR) ))) return FALSE;
        MultiByteToWideChar( CP_ACP, 0, cred, -1, credW, len );
    }
    ret = CredUnmarshalCredentialW( credW, type, out );
    free( credW );
    return ret;
}

static inline char char_decode( WCHAR c )
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '#') return 62;
    if (c == '-') return 63;
    return 64;
}

static BOOL cred_decode( const WCHAR *cred, unsigned int len, char *buf )
{
    unsigned int i = 0;
    char c0, c1, c2, c3;
    const WCHAR *p = cred;

    while (len >= 4)
    {
        if ((c0 = char_decode( p[0] )) > 63) return FALSE;
        if ((c1 = char_decode( p[1] )) > 63) return FALSE;
        if ((c2 = char_decode( p[2] )) > 63) return FALSE;
        if ((c3 = char_decode( p[3] )) > 63) return FALSE;

        buf[i + 0] = (c1 << 6) | c0;
        buf[i + 1] = (c2 << 4) | (c1 >> 2);
        buf[i + 2] = (c3 << 2) | (c2 >> 4);
        len -= 4;
        i += 3;
        p += 4;
    }
    if (len == 3)
    {
        if ((c0 = char_decode( p[0] )) > 63) return FALSE;
        if ((c1 = char_decode( p[1] )) > 63) return FALSE;
        if ((c2 = char_decode( p[2] )) > 63) return FALSE;

        buf[i + 0] = (c1 << 6) | c0;
        buf[i + 1] = (c2 << 4) | (c1 >> 2);
    }
    else if (len == 2)
    {
        if ((c0 = char_decode( p[0] )) > 63) return FALSE;
        if ((c1 = char_decode( p[1] )) > 63) return FALSE;

        buf[i + 0] = (c1 << 6) | c0;
    }
    else if (len == 1)
    {
        return FALSE;
    }
    return TRUE;
}

/******************************************************************************
 * CredUnmarshalCredentialW [ADVAPI32.@]
 */
BOOL WINAPI CredUnmarshalCredentialW( LPCWSTR cred, PCRED_MARSHAL_TYPE type, PVOID *out )
{
    unsigned int len, buflen;

    TRACE("%s, %p, %p\n", debugstr_w(cred), type, out);

    if (!cred || cred[0] != '@' || cred[1] != '@' ||
        char_decode( cred[2] ) > 63)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    len = lstrlenW( cred + 3 );
    *type = char_decode( cred[2] );
    switch (*type)
    {
    case CertCredential:
    {
        char hash[CERT_HASH_LENGTH];
        CERT_CREDENTIAL_INFO *cert;

        if (len != 27 || !cred_decode( cred + 3, len, hash ))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        if (!(cert = malloc( sizeof(*cert) ))) return FALSE;
        memcpy( cert->rgbHashOfCert, hash, sizeof(cert->rgbHashOfCert) );
        cert->cbSize = sizeof(*cert);
        *out = cert;
        break;
    }
    case UsernameTargetCredential:
    {
        USERNAME_TARGET_CREDENTIAL_INFO *target;
        DWORD size;

        if (len < 9 || !cred_decode( cred + 3, 6, (char *)&size ) ||
            size % sizeof(WCHAR) || len - 6 != (size * 4 + 2) / 3)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        buflen = sizeof(*target) + size + sizeof(WCHAR);
        if (!(target = malloc( buflen ))) return FALSE;
        if (!cred_decode( cred + 9, len - 6, (char *)(target + 1) ))
        {
            free( target );
            return FALSE;
        }
        target->UserName = (WCHAR *)(target + 1);
        target->UserName[size / sizeof(WCHAR)] = 0;
        *out = target;
        break;
    }
    case BinaryBlobCredential:
        FIXME("BinaryBlobCredential not implemented\n");
        return FALSE;
    default:
        WARN("unhandled type %u\n", *type);
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return TRUE;
}

/******************************************************************************
 * CredIsMarshaledCredentialW [ADVAPI32.@]
 *
 * Check, if the name parameter is a marshaled credential, hash or binary blob
 *
 * PARAMS
 *  name    the name to check
 *
 * RETURNS
 *  TRUE:  the name parameter is a marshaled credential, hash or binary blob
 *  FALSE: the name is a plain username
 */
BOOL WINAPI CredIsMarshaledCredentialW(LPCWSTR name)
{
    TRACE("(%s)\n", debugstr_w(name));

    if (name && name[0] == '@' && name[1] == '@' && name[2] > 'A' && name[3])
    {
        char hash[CERT_HASH_LENGTH];
        int len = lstrlenW(name + 3 );
        DWORD size;

        if ((name[2] - 'A') == CertCredential && (len == 27) && cred_decode(name + 3, len, hash))
            return TRUE;

        if (((name[2] - 'A') == UsernameTargetCredential) &&
            (len >= 9) && cred_decode(name + 3, 6, (char *)&size) && size)
            return TRUE;

        if ((name[2] - 'A') == BinaryBlobCredential)
            FIXME("BinaryBlobCredential not checked\n");

        if ((name[2] - 'A') > BinaryBlobCredential)
            TRACE("unknown type: %d\n", (name[2] - 'A'));
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

/******************************************************************************
 * CredIsMarshaledCredentialA [ADVAPI32.@]
 *
 * See CredIsMarshaledCredentialW
 *
 */
BOOL WINAPI CredIsMarshaledCredentialA(LPCSTR name)
{
    LPWSTR nameW = NULL;
    BOOL res;
    int len;

    TRACE("(%s)\n", debugstr_a(name));

    if (name)
    {
        len = MultiByteToWideChar(CP_ACP, 0, name, -1, NULL, 0);
        nameW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, name, -1, nameW, len);
    }

    res = CredIsMarshaledCredentialW(nameW);
    free(nameW);
    return res;
}
