/*
 * New cryptographic library (ncrypt.dll)
 *
 * Copyright 2016 Alex Henrie
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
#include "ncrypt.h"
#include "ncrypt_internal.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ncrypt);

SECURITY_STATUS WINAPI NCryptCreatePersistedKey(NCRYPT_PROV_HANDLE provider, NCRYPT_KEY_HANDLE *key,
                                                const WCHAR *algid, const WCHAR *name, DWORD keyspec,
                                                DWORD flags)
{
    FIXME("(0x%lx, %p, %s, %s, 0x%08x, 0x%08x): stub\n", provider, key, wine_dbgstr_w(algid),
                                                         wine_dbgstr_w(name), keyspec, flags);
    return NTE_NOT_SUPPORTED;
}

SECURITY_STATUS WINAPI NCryptDecrypt(NCRYPT_KEY_HANDLE key, BYTE *input, DWORD insize, void *padding,
                                     BYTE *output, DWORD outsize, DWORD *result, DWORD flags)
{
    FIXME("(0x%lx, %p, %u, %p, %p, %u, %p, 0x%08x): stub\n", key, input, insize, padding,
                                                             output, outsize, result, flags);
    return NTE_NOT_SUPPORTED;
}

SECURITY_STATUS WINAPI NCryptDeleteKey(NCRYPT_KEY_HANDLE key, DWORD flags)
{
    FIXME("(0x%lx, 0x%08x): stub\n", key, flags);
    return NTE_NOT_SUPPORTED;
}

SECURITY_STATUS WINAPI NCryptEncrypt(NCRYPT_KEY_HANDLE key, BYTE *input, DWORD insize, void *padding,
                                     BYTE *output, DWORD outsize, DWORD *result, DWORD flags)
{
    FIXME("(0x%lx, %p, %u, %p, %p, %u, %p, 0x%08x): stub\n", key, input, insize, padding,
                                                             output, outsize, result, flags);
    return NTE_NOT_SUPPORTED;
}

SECURITY_STATUS WINAPI NCryptEnumAlgorithms(NCRYPT_PROV_HANDLE provider, DWORD alg_ops,
                                            DWORD *alg_count, NCryptAlgorithmName **alg_list,
                                            DWORD flags)
{
    FIXME("(0x%lx, 0x%08x, %p, %p, 0x%08x): stub\n", provider, alg_ops, alg_count, alg_list, flags);
    return NTE_NOT_SUPPORTED;
}

SECURITY_STATUS WINAPI NCryptEnumKeys(NCRYPT_PROV_HANDLE provider, const WCHAR *scope,
                                      NCryptKeyName **key_name, PVOID *enum_state, DWORD flags)
{
    FIXME("(0x%lx, %p, %p, %p, 0x%08x): stub\n", provider, scope, key_name, enum_state, flags);
    return NTE_NOT_SUPPORTED;
}

SECURITY_STATUS WINAPI NCryptFinalizeKey(NCRYPT_KEY_HANDLE key, DWORD flags)
{
    FIXME("(0x%lx, 0x%08x): stub\n", key, flags);
    return NTE_NOT_SUPPORTED;
}

SECURITY_STATUS WINAPI NCryptFreeBuffer(PVOID buf)
{
    FIXME("(%p): stub\n", buf);
    return NTE_NOT_SUPPORTED;
}

SECURITY_STATUS WINAPI NCryptFreeObject(NCRYPT_HANDLE object)
{
    FIXME("(0x%lx): stub\n", object);
    return NTE_NOT_SUPPORTED;
}

SECURITY_STATUS WINAPI NCryptGetProperty(NCRYPT_HANDLE object, const WCHAR *property, PBYTE output,
                                         DWORD outsize, DWORD *result, DWORD flags)
{
    FIXME("(0x%lx, %s, %p, %u, %p, 0x%08x): stub\n", object, wine_dbgstr_w(property), output, outsize,
                                                         result, flags);
    return NTE_NOT_SUPPORTED;
}

static struct object *allocate_object(enum object_type type)
{
    struct object *ret;
    if (!(ret = calloc(1, sizeof(*ret)))) return NULL;
    ret->type = type;
    return ret;
}

SECURITY_STATUS WINAPI NCryptImportKey(NCRYPT_PROV_HANDLE provider, NCRYPT_KEY_HANDLE decrypt_key,
                                       const WCHAR *type, NCryptBufferDesc *params, NCRYPT_KEY_HANDLE *key,
                                       PBYTE data, DWORD datasize, DWORD flags)
{
    FIXME("(0x%lx, 0x%lx, %s, %p, %p, %p, %u, 0x%08x): stub\n", provider, decrypt_key,
                                                                wine_dbgstr_w(type), params,
                                                                key, data, datasize, flags);
    return NTE_NOT_SUPPORTED;
}

SECURITY_STATUS WINAPI NCryptIsAlgSupported(NCRYPT_PROV_HANDLE provider, const WCHAR *algid,
                                            DWORD flags)
{
    FIXME("(0x%lx, %s, 0x%08x): stub\n", provider, wine_dbgstr_w(algid), flags);
    return NTE_NOT_SUPPORTED;
}

BOOL WINAPI NCryptIsKeyHandle(NCRYPT_KEY_HANDLE hKey)
{
    FIXME("(0x%lx): stub\n", hKey);
    return FALSE;
}

SECURITY_STATUS WINAPI NCryptOpenKey(NCRYPT_PROV_HANDLE provider, NCRYPT_KEY_HANDLE *key,
                                     const WCHAR *name, DWORD keyspec, DWORD flags)
{
    FIXME("(0x%lx, %p, %s, 0x%08x, 0x%08x): stub\n", provider, key, wine_dbgstr_w(name), keyspec, flags);
    return NTE_NOT_SUPPORTED;
}

SECURITY_STATUS WINAPI NCryptOpenStorageProvider(NCRYPT_PROV_HANDLE *provider, const WCHAR *name, DWORD flags)
{
    struct object *object;

    FIXME("(%p, %s, %u): stub\n", provider, wine_dbgstr_w(name), flags);

    if (!(object = allocate_object(STORAGE_PROVIDER)))
    {
        ERR("Error allocating memory.\n");
        return NTE_NO_MEMORY;
    }
    *provider = (NCRYPT_PROV_HANDLE)object;
    return ERROR_SUCCESS;
}

SECURITY_STATUS WINAPI NCryptSetProperty(NCRYPT_HANDLE object, const WCHAR *property,
                                         PBYTE input, DWORD insize, DWORD flags)
{
    FIXME("(%lx, %s, %p, %u, 0x%08x): stub\n", object, wine_dbgstr_w(property), input, insize,
                                               flags);
    return NTE_NOT_SUPPORTED;
}
