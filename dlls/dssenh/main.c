/*
 * Copyright 2008 Maarten Lankhorst
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
#include "wincrypt.h"
#include "objbase.h"
#include "rpcproxy.h"

#include "wine/debug.h"

static HINSTANCE instance;

WINE_DEFAULT_DEBUG_CHANNEL(dssenh);

BOOL WINAPI CPAcquireContext( HCRYPTPROV *ret_prov, LPSTR container, DWORD flags, PVTableProvStruc vtable )
{
    return FALSE;
}

BOOL WINAPI CPReleaseContext( HCRYPTPROV hprov, DWORD flags )
{
    return FALSE;
}

BOOL WINAPI CPGetProvParam( HCRYPTPROV hprov, DWORD param, BYTE *data, DWORD *len, DWORD flags )
{
    return FALSE;
}

BOOL WINAPI CPGenKey( HCRYPTPROV hprov, ALG_ID algid, DWORD flags, HCRYPTKEY *ret_key )
{
    return FALSE;
}

BOOL WINAPI CPDestroyKey( HCRYPTPROV hprov, HCRYPTKEY hkey )
{
    return FALSE;
}

BOOL WINAPI CPImportKey( HCRYPTPROV hprov, const BYTE *data, DWORD len, HCRYPTKEY hpubkey, DWORD flags,
                         HCRYPTKEY *ret_key )
{
    return FALSE;
}

BOOL WINAPI CPExportKey( HCRYPTPROV hprov, HCRYPTKEY hkey, HCRYPTKEY hexpkey, DWORD blobtype, DWORD flags,
                         BYTE *data, DWORD *len )
{
    return FALSE;
}

BOOL WINAPI CPCreateHash( HCRYPTPROV hprov, ALG_ID algid, HCRYPTKEY hkey, DWORD flags, HCRYPTHASH *ret_hash )
{
    return FALSE;
}

BOOL WINAPI CPDestroyHash( HCRYPTPROV hprov, HCRYPTHASH hhash )
{
    return FALSE;
}

BOOL WINAPI CPHashData( HCRYPTPROV hprov, HCRYPTHASH hhash, const BYTE *data, DWORD len, DWORD flags )
{
    return FALSE;
}

BOOL WINAPI CPGetHashParam( HCRYPTPROV hprov, HCRYPTHASH hhash, DWORD param, BYTE *data, DWORD *len, DWORD flags )
{
    return FALSE;
}

BOOL WINAPI CPDeriveKey( HCRYPTPROV hprov, ALG_ID algid, HCRYPTHASH hhash, DWORD flags, HCRYPTKEY *ret_key )
{
    return FALSE;
}

BOOL WINAPI CPSignHash( HCRYPTPROV hprov, HCRYPTHASH hhash, DWORD keyspec, const WCHAR *desc, DWORD flags, BYTE *sig,
                        DWORD *siglen )
{
    return FALSE;
}

BOOL WINAPI CPVerifySignature( HCRYPTPROV hprov, HCRYPTHASH hhash, const BYTE *sig, DWORD siglen, HCRYPTKEY hpubkey,
                               const WCHAR *desc, DWORD flags )
{
    return FALSE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;    /* prefer native version */
    case DLL_PROCESS_ATTACH:
        instance = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
        break;
    }
    return TRUE;
}

/*****************************************************
 *    DllRegisterServer (DSSENH.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( instance );
}

/*****************************************************
 *    DllUnregisterServer (DSSENH.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( instance );
}
