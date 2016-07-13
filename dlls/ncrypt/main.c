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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ncrypt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ncrypt);

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    TRACE("(%p, %u, %p)\n", instance, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
}

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

SECURITY_STATUS WINAPI NCryptEncrypt(NCRYPT_KEY_HANDLE key, BYTE *input, DWORD insize, void *padding,
                                     BYTE *output, DWORD outsize, DWORD *result, DWORD flags)
{
    FIXME("(0x%lx, %p, %u, %p, %p, %u, %p, 0x%08x): stub\n", key, input, insize, padding,
                                                             output, outsize, result, flags);
    return NTE_NOT_SUPPORTED;
}

SECURITY_STATUS WINAPI NCryptFinalizeKey(NCRYPT_KEY_HANDLE key, DWORD flags)
{
    FIXME("(0x%lx, 0x%08x): stub\n", key, flags);
    return NTE_NOT_SUPPORTED;
}

SECURITY_STATUS WINAPI NCryptFreeObject(NCRYPT_HANDLE object)
{
    FIXME("(0x%lx): stub\n", object);
    return NTE_NOT_SUPPORTED;
}

SECURITY_STATUS WINAPI NCryptOpenKey(NCRYPT_PROV_HANDLE provider, NCRYPT_KEY_HANDLE *key,
                                     const WCHAR *name, DWORD keyspec, DWORD flags)
{
    FIXME("(0x%lx, %p, %s, 0x%08x, 0x%08x): stub\n", provider, key, wine_dbgstr_w(name), keyspec, flags);
    return NTE_NOT_SUPPORTED;
}

SECURITY_STATUS WINAPI NCryptOpenStorageProvider(NCRYPT_PROV_HANDLE *provider, const WCHAR *name, DWORD flags)
{
    FIXME("(%p, %s, %u): stub\n", provider, wine_dbgstr_w(name), flags);
    return NTE_NOT_SUPPORTED;
}
