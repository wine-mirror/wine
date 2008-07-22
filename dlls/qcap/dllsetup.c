/*
 * DirectX DLL registration and unregistration
 *
 * Copyright (C) 2005 Rolf Kalbermatter
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
#include <assert.h>

#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "winreg.h"
#include "objbase.h"
#include "uuids.h"

#include "dllsetup.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

/*
 * defines and constants
 */
#define MAX_KEY_LEN  260

static WCHAR const clsid_keyname[6] =
{'C','L','S','I','D',0 };
static WCHAR const ips32_keyname[15] =
{'I','n','P','r','o','c','S','e','r','v','e','r','3','2',0};
static WCHAR const tmodel_keyname[15] =
{'T','h','r','e','a','d','i','n','g','M','o','d','e','l',0};
static WCHAR const tmodel_both[] =
{'B','o','t','h',0};

/*
 * SetupRegisterClass()
 */
static HRESULT SetupRegisterClass(HKEY clsid, LPCWSTR szCLSID,
                                  LPCWSTR szDescription,
                                  LPCWSTR szFileName,
                                  LPCWSTR szServerType,
                                  LPCWSTR szThreadingModel)
{
    HKEY hkey, hsubkey = NULL;
    LONG ret = RegCreateKeyW(clsid, szCLSID, &hkey);
    if (ERROR_SUCCESS != ret)
        return HRESULT_FROM_WIN32(ret);

    /* set description string */
    ret = RegSetValueW(hkey, NULL, REG_SZ, szDescription,
                       sizeof(WCHAR) * (lstrlenW(szDescription) + 1));
    if (ERROR_SUCCESS != ret)
        goto err_out;

    /* create CLSID\\{"CLSID"}\\"ServerType" key, using key to CLSID\\{"CLSID"}
       passed back by last call to RegCreateKeyW(). */
    ret = RegCreateKeyW(hkey,  szServerType, &hsubkey);
    if (ERROR_SUCCESS != ret)
        goto err_out;

    /* set server path */
    ret = RegSetValueW(hsubkey, NULL, REG_SZ, szFileName,
                       sizeof(WCHAR) * (lstrlenW(szFileName) + 1));
    if (ERROR_SUCCESS != ret)
        goto err_out;

    /* set threading model */
    ret = RegSetValueExW(hsubkey, tmodel_keyname, 0L, REG_SZ,
                         (const BYTE*)szThreadingModel, 
                         sizeof(WCHAR) * (lstrlenW(szThreadingModel) + 1));
err_out:
    if (hsubkey)
        RegCloseKey(hsubkey);
    RegCloseKey(hkey);
    return HRESULT_FROM_WIN32(ret);
}

/*
 * RegisterAllClasses()
 */
static HRESULT SetupRegisterAllClasses(const CFactoryTemplate * pList, int num,
                                       LPCWSTR szFileName, BOOL bRegister)
{
    HRESULT hr = NOERROR;
    HKEY hkey;
    OLECHAR szCLSID[CHARS_IN_GUID];
    LONG i, ret = RegCreateKeyW(HKEY_CLASSES_ROOT, clsid_keyname, &hkey);
    if (ERROR_SUCCESS != ret)
        return HRESULT_FROM_WIN32(ret);

    for (i = 0; i < num; i++, pList++)
    {
        /* (un)register CLSID and InprocServer32 */
        hr = StringFromGUID2(pList->m_ClsID, szCLSID, CHARS_IN_GUID);
        if (SUCCEEDED(hr))
        {
            if (bRegister )
                hr = SetupRegisterClass(hkey, szCLSID,
                                        pList->m_Name, szFileName,
                                        ips32_keyname, tmodel_both);
            else
                hr = RegDeleteTreeW(hkey, szCLSID);
        }
    }
    RegCloseKey(hkey);
    return hr;
}


/****************************************************************************
 * SetupRegisterServers
 *
 * This function is table driven using the static members of the
 * CFactoryTemplate class defined in the Dll.
 *
 * It registers the Dll as the InprocServer32 for all the classes in
 * CFactoryTemplate
 *
 ****************************************************************************/
HRESULT SetupRegisterServers(const CFactoryTemplate * pList, int num,
                             BOOL bRegister)
{
    static const WCHAR szFileName[] = {'q','c','a','p','.','d','l','l',0};
    HRESULT hr = NOERROR;
    IFilterMapper2 *pIFM2 = NULL;

    /* first register all server classes, just to make sure */
    if (bRegister)
        hr = SetupRegisterAllClasses(pList, num, szFileName, TRUE );

    /* next, register/unregister all filters */
    if (SUCCEEDED(hr))
    {
        hr = CoInitialize(NULL);

        TRACE("Getting IFilterMapper2\r\n");
        hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
                              &IID_IFilterMapper2, (void **)&pIFM2);

        if (SUCCEEDED(hr))
        {
            int i;
            
            /* scan through array of CFactoryTemplates registering all filters */
            for (i = 0; i < num; i++, pList++)
            {
                if (pList->m_pAMovieSetup_Filter.dwVersion)
                {
                    hr = IFilterMapper2_RegisterFilter(pIFM2, pList->m_ClsID, pList->m_Name, NULL, &CLSID_LegacyAmFilterCategory, NULL, &pList->m_pAMovieSetup_Filter);
                }

                /* check final error for this pass and break loop if we failed */
                if (FAILED(hr))
                    break;
            }

            /* release interface */
            IFilterMapper2_Release(pIFM2);
        }

        /* and clear up */
        CoFreeUnusedLibraries();
        CoUninitialize();
    }

    /* if unregistering, unregister all OLE servers */
    if (SUCCEEDED(hr) && !bRegister)
        hr = SetupRegisterAllClasses(pList, num, szFileName, FALSE);
    return hr;
}

/****************************************************************************
 * SetupInitializeServers
 *
 * This function is table driven using the static members of the
 * CFactoryTemplate class defined in the Dll.
 *
 * It calls the initialize function for any class in CFactoryTemplate with
 * one defined.
 *
 ****************************************************************************/
void SetupInitializeServers(const CFactoryTemplate * pList, int num,
                            BOOL bLoading)
{
    int i;

    for (i = 0; i < num; i++, pList++)
    {
        if (pList->m_lpfnInit)
            pList->m_lpfnInit(bLoading, pList->m_ClsID);
    }
}
