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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "wintrust.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cryptdlg);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
        default:
            break;
    }
    return TRUE;
}

/***********************************************************************
 *		GetFriendlyNameOfCertA (CRYPTDLG.@)
 */
DWORD WINAPI GetFriendlyNameOfCertA(PCCERT_CONTEXT pccert, LPSTR pchBuffer,
                             DWORD cchBuffer)
{
    return CertGetNameStringA(pccert, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL,
     pchBuffer, cchBuffer);
}

/***********************************************************************
 *		GetFriendlyNameOfCertW (CRYPTDLG.@)
 */
DWORD WINAPI GetFriendlyNameOfCertW(PCCERT_CONTEXT pccert, LPWSTR pchBuffer,
                             DWORD cchBuffer)
{
    return CertGetNameStringW(pccert, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL,
     pchBuffer, cchBuffer);
}

/***********************************************************************
 *		CertTrustInit (CRYPTDLG.@)
 */
HRESULT WINAPI CertTrustInit(CRYPT_PROVIDER_DATA *pProvData)
{
    FIXME("(%p)\n", pProvData);
    return E_NOTIMPL;
}

/***********************************************************************
 *		CertTrustCertPolicy (CRYPTDLG.@)
 */
BOOL WINAPI CertTrustCertPolicy(CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner, BOOL fCounterSignerChain, DWORD idxCounterSigner)
{
    FIXME("(%p, %d, %s, %d)\n", pProvData, idxSigner, fCounterSignerChain ? "TRUE" : "FALSE", idxCounterSigner);
    return FALSE;
}

/***********************************************************************
 *		CertTrustCleanup (CRYPTDLG.@)
 */
HRESULT WINAPI CertTrustCleanup(CRYPT_PROVIDER_DATA *pProvData)
{
    FIXME("(%p)\n", pProvData);
    return E_NOTIMPL;
}

/***********************************************************************
 *		CertTrustFinalPolicy (CRYPTDLG.@)
 */
HRESULT WINAPI CertTrustFinalPolicy(CRYPT_PROVIDER_DATA *pProvData)
{
    FIXME("(%p)\n", pProvData);
    return E_NOTIMPL;
}
