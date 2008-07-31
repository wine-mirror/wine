/*
 * Copyright 2008 Juan Lang
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
#include "mssip.h"
#define COBJMACROS
#include "objbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msisip);

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

static GUID mySubject = { 0x000c10f1, 0x0000, 0x0000,
 { 0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46 }};

/***********************************************************************
 *              DllRegisterServer (MSISIP.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    static WCHAR msisip[] = { 'M','S','I','S','I','P','.','D','L','L',0 };
    static WCHAR getSignedDataMsg[] = { 'M','s','i','S','I','P','G','e','t',
     'S','i','g','n','e','d','D','a','t','a','M','s','g',0 };
    static WCHAR putSignedDataMsg[] = { 'M','s','i','S','I','P','P','u','t',
     'S','i','g','n','e','d','D','a','t','a','M','s','g',0 };
    static WCHAR createIndirectData[] = { 'M','s','i','S','I','P',
     'C','r','e','a','t','e','I','n','d','i','r','e','c','t','D','a','t','a',
     0 };
    static WCHAR verifyIndirectData[] = { 'M','s','i','S','I','P',
     'V','e','r','i','f','y','I','n','d','i','r','e','c','t','D','a','t','a',
     0 };
    static WCHAR removeSignedDataMsg[] = { 'M','s','i','S','I','P','R','e','m',
     'o','v','e','S','i','g','n','e','d','D','a','t','a','M','s','g', 0 };
    static WCHAR isMyTypeOfFile[] = { 'M','s','i','S','I','P',
     'I','s','M','y','T','y','p','e','O','f','F','i','l','e',0 };

    SIP_ADD_NEWPROVIDER prov;

    memset(&prov, 0, sizeof(prov));
    prov.cbStruct = sizeof(prov);
    prov.pwszDLLFileName = msisip;
    prov.pgSubject = &mySubject;
    prov.pwszGetFuncName = getSignedDataMsg;
    prov.pwszPutFuncName = putSignedDataMsg;
    prov.pwszCreateFuncName = createIndirectData;
    prov.pwszVerifyFuncName = verifyIndirectData;
    prov.pwszRemoveFuncName = removeSignedDataMsg;
    prov.pwszIsFunctionNameFmt2 = isMyTypeOfFile;
    return CryptSIPAddProvider(&prov) ? S_OK : S_FALSE;
}

/***********************************************************************
 *              DllUnregisterServer (MSISIP.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    CryptSIPRemoveProvider(&mySubject);
    return S_OK;
}

/***********************************************************************
 *              MsiSIPIsMyTypeOfFile (MSISIP.@)
 */
BOOL WINAPI MsiSIPIsMyTypeOfFile(WCHAR *name, GUID *subject)
{
    static const WCHAR msi[] = { '.','m','s','i',0 };
    static const WCHAR msp[] = { '.','m','s','p',0 };
    BOOL ret = FALSE;

    TRACE("(%s, %p)\n", debugstr_w(name), subject);

    if (lstrlenW(name) < lstrlenW(msi))
        return FALSE;
    else if (lstrcmpiW(name + lstrlenW(name) - lstrlenW(msi), msi) &&
     lstrcmpiW(name + lstrlenW(name) - lstrlenW(msp), msp))
        return FALSE;
    else
    {
        IStorage *stg = NULL;
        HRESULT r = StgOpenStorage(name, NULL,
         STGM_DIRECT|STGM_READ|STGM_SHARE_DENY_WRITE, NULL, 0, &stg);

        if (SUCCEEDED(r))
        {
            IStorage_Release(stg);
            *subject = mySubject;
            ret = TRUE;
        }
    }
    return ret;
}
