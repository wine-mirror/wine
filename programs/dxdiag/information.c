/*
 * DxDiag information collection
 *
 * Copyright 2011 Andrew Nguyen
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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dxdiag.h>

#include "wine/debug.h"
#include "wine/unicode.h"

#include "dxdiag_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxdiag);

static BOOL property_to_string(IDxDiagContainer *container, const WCHAR *property, WCHAR **output)
{
    VARIANT var;
    HRESULT hr;
    BOOL ret = FALSE;

    VariantInit(&var);

    hr = IDxDiagContainer_GetProp(container, property, &var);
    if (SUCCEEDED(hr))
    {
        if (V_VT(&var) == VT_BSTR)
        {
            WCHAR *bstr = V_BSTR(&var);

            *output = HeapAlloc(GetProcessHeap(), 0, (strlenW(bstr) + 1) * sizeof(WCHAR));
            if (*output)
            {
                strcpyW(*output, bstr);
                ret = TRUE;
            }
        }
    }

    VariantClear(&var);
    return ret;
}

static void free_system_information(struct dxdiag_information *dxdiag_info)
{
    struct system_information *system_info = &dxdiag_info->system_info;
    size_t i;
    WCHAR *strings[] =
    {
        system_info->szTimeEnglish,
        system_info->szTimeLocalized,
        system_info->szMachineNameEnglish,
        system_info->szOSExLongEnglish,
        system_info->szOSExLocalized,
        system_info->szLanguagesEnglish,
        system_info->szLanguagesLocalized,
        system_info->szSystemManufacturerEnglish,
        system_info->szSystemModelEnglish,
        system_info->szBIOSEnglish,
        system_info->szProcessorEnglish,
        system_info->szPhysicalMemoryEnglish,
        system_info->szPageFileEnglish,
        system_info->szPageFileLocalized,
        system_info->szWindowsDir,
        system_info->szDirectXVersionLongEnglish,
        system_info->szSetupParamEnglish,
        system_info->szDxDiagVersion,
    };

    for (i = 0; i < sizeof(strings)/sizeof(strings[0]); i++)
        HeapFree(GetProcessHeap(), 0, strings[i]);
}

static BOOL fill_system_information(IDxDiagContainer *container, struct dxdiag_information *dxdiag_info)
{
    static const WCHAR szTimeEnglish[] = {'s','z','T','i','m','e','E','n','g','l','i','s','h',0};
    static const WCHAR szTimeLocalized[] = {'s','z','T','i','m','e','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szMachineNameEnglish[] = {'s','z','M','a','c','h','i','n','e','N','a','m','e','E','n','g','l','i','s','h',0};
    static const WCHAR szOSExLongEnglish[] = {'s','z','O','S','E','x','L','o','n','g','E','n','g','l','i','s','h',0};
    static const WCHAR szOSExLocalized[] = {'s','z','O','S','E','x','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szLanguagesEnglish[] = {'s','z','L','a','n','g','u','a','g','e','s','E','n','g','l','i','s','h',0};
    static const WCHAR szLanguagesLocalized[] = {'s','z','L','a','n','g','u','a','g','e','s','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szSystemManufacturerEnglish[] = {'s','z','S','y','s','t','e','m','M','a','n','u','f','a','c','t','u','r','e','r','E','n','g','l','i','s','h',0};
    static const WCHAR szSystemModelEnglish[] = {'s','z','S','y','s','t','e','m','M','o','d','e','l','E','n','g','l','i','s','h',0};
    static const WCHAR szBIOSEnglish[] = {'s','z','B','I','O','S','E','n','g','l','i','s','h',0};
    static const WCHAR szProcessorEnglish[] = {'s','z','P','r','o','c','e','s','s','o','r','E','n','g','l','i','s','h',0};
    static const WCHAR szPhysicalMemoryEnglish[] = {'s','z','P','h','y','s','i','c','a','l','M','e','m','o','r','y','E','n','g','l','i','s','h',0};
    static const WCHAR szPageFileEnglish[] = {'s','z','P','a','g','e','F','i','l','e','E','n','g','l','i','s','h',0};
    static const WCHAR szPageFileLocalized[] = {'s','z','P','a','g','e','F','i','l','e','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szWindowsDir[] = {'s','z','W','i','n','d','o','w','s','D','i','r',0};
    static const WCHAR szDirectXVersionLongEnglish[] = {'s','z','D','i','r','e','c','t','X','V','e','r','s','i','o','n','L','o','n','g','E','n','g','l','i','s','h',0};
    static const WCHAR szSetupParamEnglish[] = {'s','z','S','e','t','u','p','P','a','r','a','m','E','n','g','l','i','s','h',0};
    static const WCHAR szDxDiagVersion[] = {'s','z','D','x','D','i','a','g','V','e','r','s','i','o','n',0};

    struct system_information *system_info = &dxdiag_info->system_info;
    size_t i;
    const struct
    {
        const WCHAR *property_name;
        WCHAR **output;
    } property_list[] =
    {
        {szTimeEnglish, &system_info->szTimeEnglish},
        {szTimeLocalized, &system_info->szTimeLocalized},
        {szMachineNameEnglish, &system_info->szMachineNameEnglish},
        {szOSExLongEnglish, &system_info->szOSExLongEnglish},
        {szOSExLocalized, &system_info->szOSExLocalized},
        {szLanguagesEnglish, &system_info->szLanguagesEnglish},
        {szLanguagesLocalized, &system_info->szLanguagesLocalized},
        {szSystemManufacturerEnglish, &system_info->szSystemManufacturerEnglish},
        {szSystemModelEnglish, &system_info->szSystemModelEnglish},
        {szBIOSEnglish, &system_info->szBIOSEnglish},
        {szProcessorEnglish, &system_info->szProcessorEnglish},
        {szPhysicalMemoryEnglish, &system_info->szPhysicalMemoryEnglish},
        {szPageFileEnglish, &system_info->szPageFileEnglish},
        {szPageFileLocalized, &system_info->szPageFileLocalized},
        {szWindowsDir, &system_info->szWindowsDir},
        {szDirectXVersionLongEnglish, &system_info->szDirectXVersionLongEnglish},
        {szSetupParamEnglish, &system_info->szSetupParamEnglish},
        {szDxDiagVersion, &system_info->szDxDiagVersion},
    };

    for (i = 0; i < sizeof(property_list)/sizeof(property_list[0]); i++)
    {
        if (!property_to_string(container, property_list[i].property_name, property_list[i].output))
        {
            WINE_ERR("Failed to retrieve property %s\n", wine_dbgstr_w(property_list[i].property_name));
            return FALSE;
        }
    }

#ifdef _WIN64
    system_info->win64 = TRUE;
#else
    system_info->win64 = FALSE;
#endif

    return TRUE;
}

static const WCHAR DxDiag_SystemInfo[] = {'D','x','D','i','a','g','_','S','y','s','t','e','m','I','n','f','o',0};

static const struct information_fillers
{
    const WCHAR *child_container_name;
    BOOL (*fill_function)(IDxDiagContainer *, struct dxdiag_information *);
    void (*free_function)(struct dxdiag_information *);
} filler_list[] =
{
    {DxDiag_SystemInfo, fill_system_information, free_system_information},
};

void free_dxdiag_information(struct dxdiag_information *system_info)
{
    size_t i;

    if (!system_info)
        return;

    for (i = 0; i < sizeof(filler_list)/sizeof(filler_list[0]); i++)
        filler_list[i].free_function(system_info);

    HeapFree(GetProcessHeap(), 0, system_info);
}

struct dxdiag_information *collect_dxdiag_information(BOOL whql_check)
{
    IDxDiagProvider *pddp = NULL;
    IDxDiagContainer *root = NULL;
    struct dxdiag_information *ret = NULL;
    DXDIAG_INIT_PARAMS params = {sizeof(DXDIAG_INIT_PARAMS), DXDIAG_DX9_SDK_VERSION, whql_check};
    HRESULT hr;
    size_t i;

    /* Initialize the DxDiag COM instances. */
    hr = CoCreateInstance(&CLSID_DxDiagProvider, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IDxDiagProvider, (void **)&pddp);
    if (FAILED(hr))
    {
        WINE_ERR("IDxDiagProvider instance creation failed with 0x%08x\n", hr);
        goto error;
    }

    hr = IDxDiagProvider_Initialize(pddp, &params);
    if (FAILED(hr))
        goto error;

    hr = IDxDiagProvider_GetRootContainer(pddp, &root);
    if (FAILED(hr))
        goto error;

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret));
    if (!ret)
        goto error;

    for (i = 0; i < sizeof(filler_list)/sizeof(filler_list[0]); i++)
    {
        IDxDiagContainer *child;
        BOOL success;

        hr = IDxDiagContainer_GetChildContainer(root, filler_list[i].child_container_name, &child);
        if (FAILED(hr))
            goto error;

        success = filler_list[i].fill_function(child, ret);
        IDxDiagContainer_Release(child);
        if (!success)
            goto error;
    }

    IDxDiagContainer_Release(root);
    IDxDiagProvider_Release(pddp);
    return ret;

error:
    free_dxdiag_information(ret);
    if (root) IDxDiagContainer_Release(root);
    if (pddp) IDxDiagProvider_Release(pddp);
    return NULL;
}
