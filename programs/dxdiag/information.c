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

struct property_list
{
    const WCHAR *property_name;
    WCHAR **output;
};

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

    HeapFree(GetProcessHeap(), 0, system_info->szTimeEnglish);
    HeapFree(GetProcessHeap(), 0, system_info->szTimeLocalized);
    HeapFree(GetProcessHeap(), 0, system_info->szMachineNameEnglish);
    HeapFree(GetProcessHeap(), 0, system_info->szOSExLongEnglish);
    HeapFree(GetProcessHeap(), 0, system_info->szOSExLocalized);
    HeapFree(GetProcessHeap(), 0, system_info->szLanguagesEnglish);
    HeapFree(GetProcessHeap(), 0, system_info->szLanguagesLocalized);
    HeapFree(GetProcessHeap(), 0, system_info->szSystemManufacturerEnglish);
    HeapFree(GetProcessHeap(), 0, system_info->szSystemModelEnglish);
    HeapFree(GetProcessHeap(), 0, system_info->szBIOSEnglish);
    HeapFree(GetProcessHeap(), 0, system_info->szProcessorEnglish);
    HeapFree(GetProcessHeap(), 0, system_info->szPhysicalMemoryEnglish);
    HeapFree(GetProcessHeap(), 0, system_info->szPageFileEnglish);
    HeapFree(GetProcessHeap(), 0, system_info->szPageFileLocalized);
    HeapFree(GetProcessHeap(), 0, system_info->szWindowsDir);
    HeapFree(GetProcessHeap(), 0, system_info->szDirectXVersionLongEnglish);
    HeapFree(GetProcessHeap(), 0, system_info->szSetupParamEnglish);
    HeapFree(GetProcessHeap(), 0, system_info->szDxDiagVersion);
}

static inline void fill_system_property_list(struct dxdiag_information *dxdiag_info, struct property_list *list)
{
    struct system_information *system_info = &dxdiag_info->system_info;

    list[0].property_name = szTimeEnglish;
    list[0].output = &system_info->szTimeEnglish;
    list[1].property_name = szTimeLocalized;
    list[1].output = &system_info->szTimeLocalized;
    list[2].property_name = szMachineNameEnglish;
    list[2].output = &system_info->szMachineNameEnglish;
    list[3].property_name = szOSExLongEnglish;
    list[3].output = &system_info->szOSExLongEnglish;
    list[4].property_name = szOSExLocalized;
    list[4].output = &system_info->szOSExLocalized;
    list[5].property_name = szLanguagesEnglish;
    list[5].output = &system_info->szLanguagesEnglish;
    list[6].property_name = szLanguagesLocalized;
    list[6].output = &system_info->szLanguagesLocalized;
    list[7].property_name = szSystemManufacturerEnglish;
    list[7].output = &system_info->szSystemManufacturerEnglish;
    list[8].property_name = szSystemModelEnglish;
    list[8].output = &system_info->szSystemModelEnglish;
    list[9].property_name = szBIOSEnglish;
    list[9].output = &system_info->szBIOSEnglish;
    list[10].property_name = szProcessorEnglish;
    list[10].output = &system_info->szProcessorEnglish;
    list[11].property_name = szPhysicalMemoryEnglish;
    list[11].output = &system_info->szPhysicalMemoryEnglish;
    list[12].property_name = szPageFileEnglish;
    list[12].output = &system_info->szPageFileEnglish;
    list[13].property_name = szPageFileLocalized;
    list[13].output = &system_info->szPageFileLocalized;
    list[14].property_name = szWindowsDir;
    list[14].output = &system_info->szWindowsDir;
    list[15].property_name = szDirectXVersionLongEnglish;
    list[15].output = &system_info->szDirectXVersionLongEnglish;
    list[16].property_name = szSetupParamEnglish;
    list[16].output = &system_info->szSetupParamEnglish;
    list[17].property_name = szDxDiagVersion;
    list[17].output = &system_info->szDxDiagVersion;
}

static BOOL fill_system_information(IDxDiagContainer *container, struct dxdiag_information *dxdiag_info)
{
    struct system_information *system_info = &dxdiag_info->system_info;
    size_t i;
    struct property_list property_list[18];

    fill_system_property_list(dxdiag_info, property_list);

    for (i = 0; i < ARRAY_SIZE(property_list); i++)
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

    for (i = 0; i < ARRAY_SIZE(filler_list); i++)
        filler_list[i].free_function(system_info);

    HeapFree(GetProcessHeap(), 0, system_info);
}

struct dxdiag_information *collect_dxdiag_information(BOOL whql_check)
{
    IDxDiagProvider *pddp = NULL;
    IDxDiagContainer *root = NULL;
    struct dxdiag_information *ret = NULL;
    DXDIAG_INIT_PARAMS params = {sizeof(DXDIAG_INIT_PARAMS), DXDIAG_DX9_SDK_VERSION};
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

    params.bAllowWHQLChecks = whql_check;
    hr = IDxDiagProvider_Initialize(pddp, &params);
    if (FAILED(hr))
        goto error;

    hr = IDxDiagProvider_GetRootContainer(pddp, &root);
    if (FAILED(hr))
        goto error;

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret));
    if (!ret)
        goto error;

    for (i = 0; i < ARRAY_SIZE(filler_list); i++)
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
