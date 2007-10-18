/*
 * Copyright (C) 2006 Maarten Lankhorst
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
 */

#include "config.h"
#include "windef.h"
#include "wine/debug.h"
#include "winbase.h"
#include "winnt.h"
#include "wincrypt.h"

WINE_DEFAULT_DEBUG_CHANNEL(cryptnet);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
   TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

   switch (fdwReason) {
      case DLL_WINE_PREATTACH:
         return FALSE;  /* prefer native version */
      case DLL_PROCESS_ATTACH:
         DisableThreadLibraryCalls(hinstDLL);
         break;
      case DLL_PROCESS_DETACH:
         /* Do uninitialisation here */
         break;
      default: break;
   }
   return TRUE;
}

static const WCHAR cryptNet[] = { 'c','r','y','p','t','n','e','t','.',
   'd','l','l',0 };
static const WCHAR ldapProvOpenStore[] = { 'L','d','a','p','P','r','o','v',
   'O','p','e','S','t','o','r','e',0 };

/***********************************************************************
 *    DllRegisterServer (CRYPTNET.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
   TRACE("\n");
   CryptRegisterDefaultOIDFunction(X509_ASN_ENCODING,
    CRYPT_OID_VERIFY_REVOCATION_FUNC, 0, cryptNet);
   CryptRegisterOIDFunction(0, CRYPT_OID_OPEN_STORE_PROV_FUNC, "Ldap",
    cryptNet, "LdapProvOpenStore");
   CryptRegisterOIDFunction(0, CRYPT_OID_OPEN_STORE_PROV_FUNC,
    CERT_STORE_PROV_LDAP_W, cryptNet, "LdapProvOpenStore");
   return S_OK;
}

/***********************************************************************
 *    DllUnregisterServer (CRYPTNET.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
   TRACE("\n");
   CryptUnregisterDefaultOIDFunction(X509_ASN_ENCODING,
    CRYPT_OID_VERIFY_REVOCATION_FUNC, cryptNet);
   CryptUnregisterOIDFunction(0, CRYPT_OID_OPEN_STORE_PROV_FUNC, "Ldap");
   CryptUnregisterOIDFunction(0, CRYPT_OID_OPEN_STORE_PROV_FUNC,
    CERT_STORE_PROV_LDAP_W);
   return S_OK;
}

/***********************************************************************
 *    CryptGetObjectUrl (CRYPTNET.@)
 */
BOOL WINAPI CryptGetObjectUrl(LPCSTR pszUrlOid, LPVOID pvPara, DWORD dwFlags,
 PCRYPT_URL_ARRAY pUrlArray, DWORD *pcbUrlArray, PCRYPT_URL_INFO pUrlInfo,
 DWORD *pcbUrlInfo, LPVOID pvReserved)
{
    FIXME("(%s, %p, %08x, %p, %p, %p, %p, %p): stub\n", debugstr_a(pszUrlOid),
     pvPara, dwFlags, pUrlArray, pcbUrlArray, pUrlInfo, pcbUrlInfo, pvReserved);
    return FALSE;
}

/***********************************************************************
 *    CertDllVerifyRevocation (CRYPTNET.@)
 */
BOOL WINAPI CertDllVerifyRevocation(DWORD dwEncodingType, DWORD dwRevType,
 DWORD cContext, void *rgpvContext[], DWORD dwFlags,
 PCERT_REVOCATION_PARA pRevPara, PCERT_REVOCATION_STATUS pRevStatus)
{
   FIXME("(%08x, %d, %d, %p, %08x, %p, %p): stub\n", dwEncodingType, dwRevType,
    cContext, rgpvContext, dwFlags, pRevPara, pRevStatus);
   return FALSE;
}
