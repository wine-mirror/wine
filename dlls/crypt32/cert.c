/*
 * Copyright 2002 Mike McCormack for CodeWeavers
 * Copyright (C) 2004 Juan Lang
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
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wincrypt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

HCERTSTORE WINAPI CertOpenStore(LPCSTR lpszStoreProvider, DWORD dwEncodingType,
 HCRYPTPROV hCryptProv, DWORD dwFlags, const void *pvPara)
{
  FIXME("(%s, %ld, %ld, %ld, %p), stub.\n", debugstr_a(lpszStoreProvider), dwEncodingType, hCryptProv, dwFlags, pvPara);
  return (HCERTSTORE)1;
}

HCERTSTORE WINAPI CertOpenSystemStoreA(HCRYPTPROV hProv,
 LPCSTR szSubSystemProtocol)
{
    FIXME("(%ld, %s), stub\n", hProv, debugstr_a(szSubSystemProtocol));
    return (HCERTSTORE)1;
}

HCERTSTORE WINAPI CertOpenSystemStoreW(HCRYPTPROV hProv,
 LPCWSTR szSubSystemProtocol)
{
    FIXME("(%ld, %s), stub\n", hProv, debugstr_w(szSubSystemProtocol));
    return (HCERTSTORE)1;
}

PCCERT_CONTEXT WINAPI CertEnumCertificatesInStore(HCERTSTORE hCertStore, PCCERT_CONTEXT pPrev)
{
    FIXME("(%p,%p)\n", hCertStore, pPrev);
    return NULL;
}

BOOL WINAPI CertSaveStore(HCERTSTORE hCertStore, DWORD dwMsgAndCertEncodingType,
             DWORD dwSaveAs, DWORD dwSaveTo, void* pvSaveToPara, DWORD dwFlags)
{
    FIXME("(%p,%ld,%ld,%ld,%p,%08lx) stub!\n", hCertStore, 
          dwMsgAndCertEncodingType, dwSaveAs, dwSaveTo, pvSaveToPara, dwFlags);
    return TRUE;
}

PCCRL_CONTEXT WINAPI CertCreateCRLContext( DWORD dwCertEncodingType,
  const BYTE* pbCrlEncoded, DWORD cbCrlEncoded)
{
    FIXME("%08lx %p %08lx\n", dwCertEncodingType, pbCrlEncoded, cbCrlEncoded);
    return NULL;
}

BOOL WINAPI CertCloseStore( HCERTSTORE hCertStore, DWORD dwFlags )
{
    FIXME("%p %08lx\n", hCertStore, dwFlags );
    return TRUE;
}

BOOL WINAPI CertFreeCertificateContext( PCCERT_CONTEXT pCertContext )
{
    FIXME("%p stub\n", pCertContext);
    return TRUE;
}
