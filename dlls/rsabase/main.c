/*
 * RSABASE - RSA encryption for Wine
 *
 * Copyright 2004 Mike McCormack for Codeweavers
 * Copyright 2002 Transgaming Technologies
 *
 * David Hammerton
 *
 * (based upon code from dlls/wininet/netconnection.c)
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

 */


#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"

#ifdef HAVE_OPENSSL_SSL_H
#define DSA __ssl_DSA  /* avoid conflict with commctrl.h */
#undef FAR
# include <openssl/rand.h>
#undef FAR
#define FAR do_not_use_this_in_wine
#undef DSA
#endif

#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

#define RSABASE_MAGIC 0x52534100

#ifdef HAVE_OPENSSL_SSL_H

#ifndef SONAME_LIBCRYPTO
#define SONAME_LIBCRYPTO "libcrypto.so"
#endif

static void *libcrypto;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f

/* OpenSSL funtions that we use */
MAKE_FUNCPTR(RAND_bytes);

static BOOL load_libcrypto( void )
{
    libcrypto = wine_dlopen(SONAME_LIBCRYPTO, RTLD_NOW, NULL, 0);
    if (!libcrypto)
    {
        MESSAGE("Couldn't load %s, RSA encryption not available.\n", SONAME_LIBCRYPTO);
        MESSAGE("Install the openssl package if you're have problems.\n");
        return FALSE;
    }

    #define GETFUNC(x) \
    p##x = wine_dlsym(libcrypto, #x, NULL, 0); \
    if (!p##x) \
    { \
        ERR("failed to load symbol %s\n", #x); \
        return FALSE; \
    }

    GETFUNC(RAND_bytes);

    return TRUE;
}

#endif

typedef struct _RSA_CryptProv
{
    DWORD dwMagic;
} RSA_CryptProv;

BOOL WINAPI RSA_CPAcquireContext(HCRYPTPROV *phProv, LPSTR pszContainer,
                   DWORD dwFlags, PVTableProvStruc pVTable)
{
    BOOL ret = FALSE;
    RSA_CryptProv *cp;

    TRACE("%p %s %08lx %p\n", phProv, debugstr_a(pszContainer),
           dwFlags, pVTable);

#ifdef HAVE_OPENSSL_SSL_H

    if( !load_libcrypto() )
        return FALSE;

    cp = HeapAlloc( GetProcessHeap(), 0, sizeof (RSA_CryptProv) );
    if( !cp )
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    cp->dwMagic = RSABASE_MAGIC;

    *phProv = (HCRYPTPROV) cp;
    ret = TRUE;

#endif

    return ret;
}

BOOL WINAPI RSA_CPCreateHash(HCRYPTPROV hProv, ALG_ID Algid, HCRYPTKEY hKey, DWORD dwFlags, HCRYPTHASH *phHash)
{
    FIXME("%08lx %d %08lx %08lx %p\n", hProv, Algid, hKey, dwFlags, phHash);
    return FALSE;
}

BOOL WINAPI RSA_CPDecrypt(HCRYPTPROV hProv, HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final, DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPDeriveKey(HCRYPTPROV hProv, ALG_ID Algid, HCRYPTHASH hBaseData, DWORD dwFlags, HCRYPTKEY *phKey)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPDestroyHash(HCRYPTPROV hProv, HCRYPTHASH hHash)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPDestroyKey(HCRYPTPROV hProv, HCRYPTKEY hKey)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPDuplicateHash(HCRYPTPROV hUID, HCRYPTHASH hHash, DWORD *pdwReserved, DWORD dwFlags, HCRYPTHASH *phHash)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPDuplicateKey(HCRYPTPROV hUID, HCRYPTKEY hKey, DWORD *pdwReserved, DWORD dwFlags, HCRYPTKEY *phKey)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPEncrypt(HCRYPTPROV hProv, HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final, DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen, DWORD dwBufLen)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPExportKey(HCRYPTPROV hProv, HCRYPTKEY hKey, HCRYPTKEY hPubKey, DWORD dwBlobType, DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPGenKey(HCRYPTPROV hProv, ALG_ID Algid, DWORD dwFlags, HCRYPTKEY *phKey)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPGenRandom(HCRYPTPROV hProv, DWORD dwLen, BYTE *pbBuffer)
{
    BOOL ret = FALSE;
    RSA_CryptProv *cp = (RSA_CryptProv*) hProv;

    TRACE("%08lx %ld %p\n", hProv, dwLen, pbBuffer);

    if( cp && ( cp->dwMagic != RSABASE_MAGIC ) )
        return FALSE;

#ifdef HAVE_OPENSSL_SSL_H

    if( !pRAND_bytes)
        return FALSE;
    ret = pRAND_bytes( pbBuffer, dwLen );

#endif

    return ret;
}

BOOL WINAPI RSA_CPGetHashParam(HCRYPTPROV hProv, HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData, DWORD *pdwDataLen, DWORD dwFlags)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPGetKeyParam(HCRYPTPROV hProv, HCRYPTKEY hKey, DWORD dwParam, BYTE *pbData, DWORD *pdwDataLen, DWORD dwFlags)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPGetProvParam(HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData, DWORD *pdwDataLen, DWORD dwFlags)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPGetUserKey(HCRYPTPROV hProv, DWORD dwKeySpec, HCRYPTKEY *phUserKey)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPHashData(HCRYPTPROV hProv, HCRYPTHASH hHash, CONST BYTE *pbData, DWORD dwDataLen, DWORD dwFlags)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPHashSessionKey(HCRYPTPROV hProv, HCRYPTHASH hHash, HCRYPTKEY hKey, DWORD dwFlags)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPImportKey(HCRYPTPROV hProv, CONST BYTE *pbData, DWORD dwDataLen, HCRYPTKEY hPubKey, DWORD dwFlags, HCRYPTKEY *phKey)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPReleaseContext(HCRYPTPROV hProv, DWORD dwFlags)
{
    RSA_CryptProv *cp = (RSA_CryptProv*) hProv;

    TRACE("%08lx %08lx\n", hProv, dwFlags);

    if( cp && ( cp->dwMagic != RSABASE_MAGIC ) )
        return FALSE;

    HeapFree( GetProcessHeap(), 0, cp );

    return TRUE;
}

BOOL WINAPI RSA_CPSetHashParam(HCRYPTPROV hProv, HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData, DWORD dwFlags)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPSetKeyParam(HCRYPTPROV hProv, HCRYPTKEY hKey, DWORD dwParam, BYTE *pbData, DWORD dwFlags)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPSetProvParam(HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData, DWORD dwFlags)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPSignHash(HCRYPTPROV hProv, HCRYPTHASH hHash, DWORD dwKeySpec, LPCWSTR sDescription, DWORD dwFlags, BYTE *pbSignature, DWORD *pdwSigLen)
{
    FIXME("(stub)\n");
    return FALSE;
}

BOOL WINAPI RSA_CPVerifySignature(HCRYPTPROV hProv, HCRYPTHASH hHash, CONST BYTE *pbSignature, DWORD dwSigLen, HCRYPTKEY hPubKey, LPCWSTR sDescription, DWORD dwFlags)
{
    FIXME("(stub)\n");
    return FALSE;
}

/***********************************************************************
 *		DllRegisterServer (RSABASE.@)
 */
HRESULT WINAPI RSABASE_DllRegisterServer()
{
    FIXME("\n");
    return S_OK;
}

/***********************************************************************
 *		DllUnregisterServer (RSABASE.@)
 */
HRESULT WINAPI RSABASE_DllUnregisterServer()
{
    FIXME("\n");
    return S_OK;
}
