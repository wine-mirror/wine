/*
 *
 * Copyright 2014 Austin English
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
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"

#include "initguid.h"
#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static const WCHAR transform_keyW[] = {'M','e','d','i','a','F','o','u','n','d','a','t','i','o','n','\\',
                                 'T','r','a','n','s','f','o','r','m','s',0};
static const WCHAR categories_keyW[] = {'M','e','d','i','a','F','o','u','n','d','a','t','i','o','n','\\',
                                 'T','r','a','n','s','f','o','r','m','s','\\',
                                 'C','a','t','e','g','o','r','i','e','s',0};
static const WCHAR inputtypesW[]  = {'I','n','p','u','t','T','y','p','e','s',0};
static const WCHAR outputtypesW[] = {'O','u','t','p','u','t','T','y','p','e','s',0};
static const WCHAR szGUIDFmt[] =
{
    '%','0','8','x','-','%','0','4','x','-','%','0','4','x','-','%','0',
    '2','x','%','0','2','x','-','%','0','2','x','%','0','2','x','%','0','2',
    'x','%','0','2','x','%','0','2','x','%','0','2','x',0
};

static const BYTE guid_conv_table[256] =
{
    0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x00 */
    0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x10 */
    0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x20 */
    0,   1,   2,   3,   4,   5,   6, 7, 8, 9, 0, 0, 0, 0, 0, 0, /* 0x30 */
    0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x40 */
    0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x50 */
    0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf                             /* 0x60 */
};

static WCHAR* GUIDToString(WCHAR *str, REFGUID guid)
{
    sprintfW(str, szGUIDFmt, guid->Data1, guid->Data2,
        guid->Data3, guid->Data4[0], guid->Data4[1],
        guid->Data4[2], guid->Data4[3], guid->Data4[4],
        guid->Data4[5], guid->Data4[6], guid->Data4[7]);

    return str;
}

static inline BOOL is_valid_hex(WCHAR c)
{
    if (!(((c >= '0') && (c <= '9'))  ||
          ((c >= 'a') && (c <= 'f'))  ||
          ((c >= 'A') && (c <= 'F'))))
        return FALSE;
    return TRUE;
}

static BOOL GUIDFromString(LPCWSTR s, GUID *id)
{
    int i;

    /* in form XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX */

    id->Data1 = 0;
    for (i = 0; i < 8; i++)
    {
        if (!is_valid_hex(s[i])) return FALSE;
        id->Data1 = (id->Data1 << 4) | guid_conv_table[s[i]];
    }
    if (s[8]!='-') return FALSE;

    id->Data2 = 0;
    for (i = 9; i < 13; i++)
    {
        if (!is_valid_hex(s[i])) return FALSE;
        id->Data2 = (id->Data2 << 4) | guid_conv_table[s[i]];
    }
    if (s[13]!='-') return FALSE;

    id->Data3 = 0;
    for (i = 14; i < 18; i++)
    {
        if (!is_valid_hex(s[i])) return FALSE;
        id->Data3 = (id->Data3 << 4) | guid_conv_table[s[i]];
    }
    if (s[18]!='-') return FALSE;

    for (i = 19; i < 36; i+=2)
    {
        if (i == 23)
        {
            if (s[i]!='-') return FALSE;
            i++;
        }
        if (!is_valid_hex(s[i]) || !is_valid_hex(s[i+1])) return FALSE;
        id->Data4[(i-19)/2] = guid_conv_table[s[i]] << 4 | guid_conv_table[s[i+1]];
    }

    if (!s[37]) return TRUE;
    return FALSE;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
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

static HRESULT register_transform(CLSID *clsid, WCHAR *name,
                                  UINT32 cinput, MFT_REGISTER_TYPE_INFO *input_types,
                                  UINT32 coutput, MFT_REGISTER_TYPE_INFO *output_types)
{
    static const WCHAR reg_format[] = {'%','s','\\','%','s',0};
    HKEY hclsid = 0;
    WCHAR buffer[64];
    DWORD size;
    WCHAR str[250];

    GUIDToString(buffer, clsid);
    sprintfW(str, reg_format, transform_keyW, buffer);

    if (RegCreateKeyW(HKEY_CLASSES_ROOT, str, &hclsid))
        return E_FAIL;

    size = (strlenW(name) + 1) * sizeof(WCHAR);
    if (RegSetValueExW(hclsid, NULL, 0, REG_SZ, (BYTE *)name, size))
        goto err;

    if (cinput && input_types)
    {
        size = cinput * sizeof(MFT_REGISTER_TYPE_INFO);
        if (RegSetValueExW(hclsid, inputtypesW, 0, REG_BINARY, (BYTE *)input_types, size))
            goto err;
    }

    if (coutput && output_types)
    {
        size = coutput * sizeof(MFT_REGISTER_TYPE_INFO);
        if (RegSetValueExW(hclsid, outputtypesW, 0, REG_BINARY, (BYTE *)output_types, size))
            goto err;
    }

    RegCloseKey(hclsid);
    return S_OK;

err:
    RegCloseKey(hclsid);
    return E_FAIL;
}

static HRESULT register_category(CLSID *clsid, GUID *category)
{
    static const WCHAR reg_format[] = {'%','s','\\','%','s','\\','%','s',0};
    HKEY htmp1;
    WCHAR guid1[64], guid2[64];
    WCHAR str[350];

    GUIDToString(guid1, category);
    GUIDToString(guid2, clsid);

    sprintfW(str, reg_format, categories_keyW, guid1, guid2);

    if (RegCreateKeyW(HKEY_CLASSES_ROOT, str, &htmp1))
        return E_FAIL;

    RegCloseKey(htmp1);
    return S_OK;
}

/***********************************************************************
 *      MFTRegister (mfplat.@)
 */
HRESULT WINAPI MFTRegister(CLSID clsid, GUID category, LPWSTR name, UINT32 flags, UINT32 cinput,
                           MFT_REGISTER_TYPE_INFO *input_types, UINT32 coutput,
                           MFT_REGISTER_TYPE_INFO *output_types, IMFAttributes *attributes)
{
    HRESULT hr;

    TRACE("(%s, %s, %s, %x, %u, %p, %u, %p, %p)\n", debugstr_guid(&clsid), debugstr_guid(&category),
                                                    debugstr_w(name), flags, cinput, input_types,
                                                    coutput, output_types, attributes);

    if (attributes)
        FIXME("attributes not yet supported.\n");

    if (flags)
        FIXME("flags not yet supported.\n");

    hr = register_transform(&clsid, name, cinput, input_types, coutput, output_types);
    if(FAILED(hr))
        ERR("Failed to write register transform\n");

    if (SUCCEEDED(hr))
        hr = register_category(&clsid, &category);

    return hr;
}

static BOOL match_type(const WCHAR *clsid_str, const WCHAR *type_str, MFT_REGISTER_TYPE_INFO *type)
{
    HKEY htransform, hfilter;
    DWORD reg_type, size;
    LONG ret = FALSE;
    MFT_REGISTER_TYPE_INFO *info = NULL;
    int i;

    if (RegOpenKeyW(HKEY_CLASSES_ROOT, transform_keyW, &htransform))
        return FALSE;

    if (RegOpenKeyW(htransform, clsid_str, &hfilter))
    {
        RegCloseKey(htransform);
        return FALSE;
    }

    if (RegQueryValueExW(hfilter, type_str, NULL, &reg_type, NULL, &size) != ERROR_SUCCESS)
        goto out;

    if (reg_type != REG_BINARY)
        goto out;

    if (!size || size % (sizeof(MFT_REGISTER_TYPE_INFO)) != 0)
        goto out;

    info = HeapAlloc(GetProcessHeap(), 0, size);
    if (!info)
        goto out;

    if (RegQueryValueExW(hfilter, type_str, NULL, &reg_type, (LPBYTE)info, &size) != ERROR_SUCCESS)
        goto out;

    for (i = 0; i < size / sizeof(MFT_REGISTER_TYPE_INFO); i++)
    {
        if (IsEqualGUID(&info[i].guidMajorType, &type->guidMajorType) &&
            IsEqualGUID(&info[i].guidSubtype,   &type->guidSubtype))
        {
            ret = TRUE;
            break;
        }
    }

out:
    HeapFree(GetProcessHeap(), 0, info);
    RegCloseKey(hfilter);
    RegCloseKey(htransform);
    return ret;
}

/***********************************************************************
 *      MFTEnum (mfplat.@)
 */
HRESULT WINAPI MFTEnum(GUID category, UINT32 flags, MFT_REGISTER_TYPE_INFO *input_type,
                       MFT_REGISTER_TYPE_INFO *output_type, IMFAttributes *attributes,
                       CLSID **pclsids, UINT32 *pcount)
{
    WCHAR buffer[64], clsid_str[MAX_PATH] = {0};
    HKEY hcategory, hlist;
    DWORD index = 0;
    DWORD size = MAX_PATH;
    CLSID *clsids = NULL;
    UINT32 count = 0;
    LONG ret;

    TRACE("(%s, %x, %p, %p, %p, %p, %p)\n", debugstr_guid(&category), flags, input_type,
                                            output_type, attributes, pclsids, pcount);

    if (!pclsids || !pcount)
        return E_INVALIDARG;

    if (RegOpenKeyW(HKEY_CLASSES_ROOT, categories_keyW, &hcategory))
        return E_FAIL;

    GUIDToString(buffer, &category);

    ret = RegOpenKeyW(hcategory, buffer, &hlist);
    RegCloseKey(hcategory);
    if (ret) return E_FAIL;

    while (RegEnumKeyExW(hlist, index, clsid_str, &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        GUID clsid;
        void *tmp;

        if (!GUIDFromString(clsid_str, &clsid))
            goto next;

        if (output_type && !match_type(clsid_str, outputtypesW, output_type))
            goto next;

        if (input_type && !match_type(clsid_str, inputtypesW, input_type))
            goto next;

        tmp = CoTaskMemRealloc(clsids, (count + 1) * sizeof(GUID));
        if (!tmp)
        {
            CoTaskMemFree(clsids);
            RegCloseKey(hlist);
            return E_OUTOFMEMORY;
        }

        clsids = tmp;
        clsids[count++] = clsid;

    next:
        size = MAX_PATH;
        index++;
    }

    *pclsids = clsids;
    *pcount = count;

    RegCloseKey(hlist);
    return S_OK;
}

/***********************************************************************
 *      MFTEnumEx (mfplat.@)
 */
HRESULT WINAPI MFTEnumEx(GUID category, UINT32 flags, const MFT_REGISTER_TYPE_INFO *input_type,
                         const MFT_REGISTER_TYPE_INFO *output_type, IMFActivate ***activate,
                         UINT32 *pcount)
{
    FIXME("(%s, %x, %p, %p, %p, %p): stub\n", debugstr_guid(&category), flags, input_type,
                                              output_type, activate, pcount);

    *pcount = 0;
    return S_OK;
}

/***********************************************************************
 *      MFTUnregister (mfplat.@)
 */
HRESULT WINAPI MFTUnregister(CLSID clsid)
{
    WCHAR buffer[64], category[MAX_PATH];
    HKEY htransform, hcategory, htmp;
    DWORD size = MAX_PATH;
    DWORD index = 0;

    TRACE("(%s)\n", debugstr_guid(&clsid));

    GUIDToString(buffer, &clsid);

    if (!RegOpenKeyW(HKEY_CLASSES_ROOT, transform_keyW, &htransform))
    {
        RegDeleteKeyW(htransform, buffer);
        RegCloseKey(htransform);
    }

    if (!RegOpenKeyW(HKEY_CLASSES_ROOT, categories_keyW, &hcategory))
    {
        while (RegEnumKeyExW(hcategory, index, category, &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            if (!RegOpenKeyW(hcategory, category, &htmp))
            {
                RegDeleteKeyW(htmp, buffer);
                RegCloseKey(htmp);
            }
            size = MAX_PATH;
            index++;
        }
        RegCloseKey(hcategory);
    }

    return S_OK;
}

/***********************************************************************
 *      MFStartup (mfplat.@)
 */
HRESULT WINAPI MFStartup(ULONG version, DWORD flags)
{
#define MF_VERSION_XP   MAKELONG( MF_API_VERSION, 1 )
#define MF_VERSION_WIN7 MAKELONG( MF_API_VERSION, 2 )

    FIXME("(%u, %u): stub\n", version, flags);

    if(version != MF_VERSION_XP && version != MF_VERSION_WIN7)
        return MF_E_BAD_STARTUP_VERSION;

    return S_OK;
}

/***********************************************************************
 *      MFShutdown (mfplat.@)
 */
HRESULT WINAPI MFShutdown(void)
{
    FIXME("(): stub\n");
    return S_OK;
}

static HRESULT WINAPI MFPluginControl_QueryInterface(IMFPluginControl *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(IID_IUnknown %p)\n", ppv);
        *ppv = iface;
    }else if(IsEqualGUID(riid, &IID_IMFPluginControl)) {
        TRACE("(IID_IMFPluginControl %p)\n", ppv);
        *ppv = iface;
    }else {
        FIXME("(%s %p)\n", debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MFPluginControl_AddRef(IMFPluginControl *iface)
{
    TRACE("\n");
    return 2;
}

static ULONG WINAPI MFPluginControl_Release(IMFPluginControl *iface)
{
    TRACE("\n");
    return 1;
}

static HRESULT WINAPI MFPluginControl_GetPreferredClsid(IMFPluginControl *iface, DWORD plugin_type,
        const WCHAR *selector, CLSID *clsid)
{
    FIXME("(%d %s %p)\n", plugin_type, debugstr_w(selector), clsid);
    return E_NOTIMPL;
}

static HRESULT WINAPI MFPluginControl_GetPreferredClsidByIndex(IMFPluginControl *iface, DWORD plugin_type,
        DWORD index, WCHAR **selector, CLSID *clsid)
{
    FIXME("(%d %d %p %p)\n", plugin_type, index, selector, clsid);
    return E_NOTIMPL;
}

static HRESULT WINAPI MFPluginControl_SetPreferredClsid(IMFPluginControl *iface, DWORD plugin_type,
        const WCHAR *selector, const CLSID *clsid)
{
    FIXME("(%d %s %s)\n", plugin_type, debugstr_w(selector), debugstr_guid(clsid));
    return E_NOTIMPL;
}

static HRESULT WINAPI MFPluginControl_IsDisabled(IMFPluginControl *iface, DWORD plugin_type, REFCLSID clsid)
{
    FIXME("(%d %s)\n", plugin_type, debugstr_guid(clsid));
    return E_NOTIMPL;
}

static HRESULT WINAPI MFPluginControl_GetDisabledByIndex(IMFPluginControl *iface, DWORD plugin_type, DWORD index, CLSID *clsid)
{
    FIXME("(%d %d %p)\n", plugin_type, index, clsid);
    return E_NOTIMPL;
}

static HRESULT WINAPI MFPluginControl_SetDisabled(IMFPluginControl *iface, DWORD plugin_type, REFCLSID clsid, BOOL disabled)
{
    FIXME("(%d %s %x)\n", plugin_type, debugstr_guid(clsid), disabled);
    return E_NOTIMPL;
}

static const IMFPluginControlVtbl MFPluginControlVtbl = {
    MFPluginControl_QueryInterface,
    MFPluginControl_AddRef,
    MFPluginControl_Release,
    MFPluginControl_GetPreferredClsid,
    MFPluginControl_GetPreferredClsidByIndex,
    MFPluginControl_SetPreferredClsid,
    MFPluginControl_IsDisabled,
    MFPluginControl_GetDisabledByIndex,
    MFPluginControl_SetDisabled
};

static IMFPluginControl plugin_control = { &MFPluginControlVtbl };

/***********************************************************************
 *      MFGetPluginControl (mfplat.@)
 */
HRESULT WINAPI MFGetPluginControl(IMFPluginControl **ret)
{
    TRACE("(%p)\n", ret);

    *ret = &plugin_control;
    return S_OK;
}

typedef struct _mfattributes
{
    IMFAttributes IMFAttributes_iface;
    LONG ref;
} mfattributes;

static inline mfattributes *impl_from_IMFAttributes(IMFAttributes *iface)
{
    return CONTAINING_RECORD(iface, mfattributes, IMFAttributes_iface);
}

static HRESULT WINAPI mfattributes_QueryInterface(IMFAttributes *iface, REFIID riid, void **out)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), out);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IMFAttributes))
    {
        *out = &This->IMFAttributes_iface;
    }
    else
    {
        FIXME("(%s, %p)\n", debugstr_guid(riid), out);
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static ULONG WINAPI mfattributes_AddRef(IMFAttributes *iface)
{
    mfattributes *This = impl_from_IMFAttributes(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI mfattributes_Release(IMFAttributes *iface)
{
    mfattributes *This = impl_from_IMFAttributes(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI mfattributes_GetItem(IMFAttributes *iface, REFGUID key, PROPVARIANT *value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetItemType(IMFAttributes *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), type);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_CompareItem(IMFAttributes *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p, %p\n", This, debugstr_guid(key), value, result);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_Compare(IMFAttributes *iface, IMFAttributes *theirs, MF_ATTRIBUTES_MATCH_TYPE type,
                BOOL *result)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %p, %d, %p\n", This, theirs, type, result);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetUINT32(IMFAttributes *iface, REFGUID key, UINT32 *value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetUINT64(IMFAttributes *iface, REFGUID key, UINT64 *value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetDouble(IMFAttributes *iface, REFGUID key, double *value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetGUID(IMFAttributes *iface, REFGUID key, GUID *value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetStringLength(IMFAttributes *iface, REFGUID key, UINT32 *length)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), length);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetString(IMFAttributes *iface, REFGUID key, WCHAR *value,
                UINT32 size, UINT32 *length)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p, %d, %p\n", This, debugstr_guid(key), value, size, length);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetAllocatedString(IMFAttributes *iface, REFGUID key,
                                      WCHAR **value, UINT32 *length)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p, %p\n", This, debugstr_guid(key), value, length);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetBlobSize(IMFAttributes *iface, REFGUID key, UINT32 *size)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), size);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetBlob(IMFAttributes *iface, REFGUID key, UINT8 *buf,
                UINT32 bufsize, UINT32 *blobsize)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p, %d, %p\n", This, debugstr_guid(key), buf, bufsize, blobsize);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetAllocatedBlob(IMFAttributes *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p, %p\n", This, debugstr_guid(key), buf, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetUnknown(IMFAttributes *iface, REFGUID key, REFIID riid, void **ppv)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %s, %p\n", This, debugstr_guid(key), debugstr_guid(riid), ppv);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_SetItem(IMFAttributes *iface, REFGUID key, REFPROPVARIANT Value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), Value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_DeleteItem(IMFAttributes *iface, REFGUID key)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s\n", This, debugstr_guid(key));

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_DeleteAllItems(IMFAttributes *iface)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_SetUINT32(IMFAttributes *iface, REFGUID key, UINT32 value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %d\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_SetUINT64(IMFAttributes *iface, REFGUID key, UINT64 value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %s\n", This, debugstr_guid(key), wine_dbgstr_longlong(value));

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_SetDouble(IMFAttributes *iface, REFGUID key, double value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %f\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_SetGUID(IMFAttributes *iface, REFGUID key, REFGUID value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %s\n", This, debugstr_guid(key), debugstr_guid(value));

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_SetString(IMFAttributes *iface, REFGUID key, const WCHAR *value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %s\n", This, debugstr_guid(key), debugstr_w(value));

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_SetBlob(IMFAttributes *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p, %d\n", This, debugstr_guid(key), buf, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_SetUnknown(IMFAttributes *iface, REFGUID key, IUnknown *unknown)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), unknown);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_LockStore(IMFAttributes *iface)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_UnlockStore(IMFAttributes *iface)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetCount(IMFAttributes *iface, UINT32 *items)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %p\n", This, items);

    if(items)
        *items = 0;

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetItemByIndex(IMFAttributes *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %d, %p, %p\n", This, index, key, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_CopyAllItems(IMFAttributes *iface, IMFAttributes *dest)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %p\n", This, dest);

    return E_NOTIMPL;
}

static const IMFAttributesVtbl mfattributes_vtbl =
{
    mfattributes_QueryInterface,
    mfattributes_AddRef,
    mfattributes_Release,
    mfattributes_GetItem,
    mfattributes_GetItemType,
    mfattributes_CompareItem,
    mfattributes_Compare,
    mfattributes_GetUINT32,
    mfattributes_GetUINT64,
    mfattributes_GetDouble,
    mfattributes_GetGUID,
    mfattributes_GetStringLength,
    mfattributes_GetString,
    mfattributes_GetAllocatedString,
    mfattributes_GetBlobSize,
    mfattributes_GetBlob,
    mfattributes_GetAllocatedBlob,
    mfattributes_GetUnknown,
    mfattributes_SetItem,
    mfattributes_DeleteItem,
    mfattributes_DeleteAllItems,
    mfattributes_SetUINT32,
    mfattributes_SetUINT64,
    mfattributes_SetDouble,
    mfattributes_SetGUID,
    mfattributes_SetString,
    mfattributes_SetBlob,
    mfattributes_SetUnknown,
    mfattributes_LockStore,
    mfattributes_UnlockStore,
    mfattributes_GetCount,
    mfattributes_GetItemByIndex,
    mfattributes_CopyAllItems
};

/***********************************************************************
 *      MFCreateAttributes (mfplat.@)
 */
HRESULT WINAPI MFCreateAttributes(IMFAttributes **attributes, UINT32 size)
{
    mfattributes *object;

    TRACE("%p, %d\n", attributes, size);

    object = HeapAlloc( GetProcessHeap(), 0, sizeof(*object) );
    if(!object)
        return E_OUTOFMEMORY;

    object->ref = 1;
    object->IMFAttributes_iface.lpVtbl = &mfattributes_vtbl;

    *attributes = &object->IMFAttributes_iface;
    return S_OK;
}

typedef struct _mfsourceresolver
{
    IMFSourceResolver IMFSourceResolver_iface;
    LONG ref;
} mfsourceresolver;

static inline mfsourceresolver *impl_from_IMFSourceResolver(IMFSourceResolver *iface)
{
    return CONTAINING_RECORD(iface, mfsourceresolver, IMFSourceResolver_iface);
}

static HRESULT WINAPI mfsourceresolver_QueryInterface(IMFSourceResolver *iface, REFIID riid, void **obj)
{
    mfsourceresolver *This = impl_from_IMFSourceResolver(iface);

    TRACE("(%p->(%s, %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFSourceResolver) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &This->IMFSourceResolver_iface;
    }
    else
    {
        *obj = NULL;
        FIXME("unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI mfsourceresolver_AddRef(IMFSourceResolver *iface)
{
    mfsourceresolver *This = impl_from_IMFSourceResolver(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(%u)\n", This, ref);

    return ref;
}

static ULONG WINAPI mfsourceresolver_Release(IMFSourceResolver *iface)
{
    mfsourceresolver *This = impl_from_IMFSourceResolver(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%u)\n", This, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI mfsourceresolver_CreateObjectFromURL(IMFSourceResolver *iface, const WCHAR *url,
    DWORD flags, IPropertyStore *props, MF_OBJECT_TYPE *obj_type, IUnknown **object)
{
    mfsourceresolver *This = impl_from_IMFSourceResolver(iface);

    FIXME("(%p)->(%s, %#x, %p, %p, %p): stub\n", This, debugstr_w(url), flags, props, obj_type, object);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsourceresolver_CreateObjectFromByteStream(IMFSourceResolver *iface, IMFByteStream *stream,
    const WCHAR *url, DWORD flags, IPropertyStore *props, MF_OBJECT_TYPE *obj_type, IUnknown **object)
{
    mfsourceresolver *This = impl_from_IMFSourceResolver(iface);

    FIXME("(%p)->(%p, %s, %#x, %p, %p, %p): stub\n", This, stream, debugstr_w(url), flags, props, obj_type, object);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsourceresolver_BeginCreateObjectFromURL(IMFSourceResolver *iface, const WCHAR *url,
    DWORD flags, IPropertyStore *props, IUnknown **cancel_cookie, IMFAsyncCallback *callback, IUnknown *unk_state)
{
    mfsourceresolver *This = impl_from_IMFSourceResolver(iface);

    FIXME("(%p)->(%s, %#x, %p, %p, %p, %p): stub\n", This, debugstr_w(url), flags, props, cancel_cookie,
        callback, unk_state);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsourceresolver_EndCreateObjectFromURL(IMFSourceResolver *iface, IMFAsyncResult *result,
    MF_OBJECT_TYPE *obj_type, IUnknown **object)
{
    mfsourceresolver *This = impl_from_IMFSourceResolver(iface);

    FIXME("(%p)->(%p, %p, %p): stub\n", This, result, obj_type, object);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsourceresolver_BeginCreateObjectFromByteStream(IMFSourceResolver *iface, IMFByteStream *stream,
    const WCHAR *url, DWORD flags, IPropertyStore *props, IUnknown **cancel_cookie, IMFAsyncCallback *callback,
    IUnknown *unk_state)
{
    mfsourceresolver *This = impl_from_IMFSourceResolver(iface);

    FIXME("(%p)->(%p, %s, %#x, %p, %p, %p, %p): stub\n", This, stream, debugstr_w(url), flags, props, cancel_cookie,
        callback, unk_state);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsourceresolver_EndCreateObjectFromByteStream(IMFSourceResolver *iface, IMFAsyncResult *result,
    MF_OBJECT_TYPE *obj_type, IUnknown **object)
{
    mfsourceresolver *This = impl_from_IMFSourceResolver(iface);

    FIXME("(%p)->(%p, %p, %p): stub\n", This, result, obj_type, object);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsourceresolver_CancelObjectCreation(IMFSourceResolver *iface, IUnknown *cancel_cookie)
{
    mfsourceresolver *This = impl_from_IMFSourceResolver(iface);

    FIXME("(%p)->(%p): stub\n", This, cancel_cookie);

    return E_NOTIMPL;
}

static const IMFSourceResolverVtbl mfsourceresolvervtbl =
{
   mfsourceresolver_QueryInterface,
   mfsourceresolver_AddRef,
   mfsourceresolver_Release,
   mfsourceresolver_CreateObjectFromURL,
   mfsourceresolver_CreateObjectFromByteStream,
   mfsourceresolver_BeginCreateObjectFromURL,
   mfsourceresolver_EndCreateObjectFromURL,
   mfsourceresolver_BeginCreateObjectFromByteStream,
   mfsourceresolver_EndCreateObjectFromByteStream,
   mfsourceresolver_CancelObjectCreation,
};

/***********************************************************************
 *      MFCreateSourceResolver (mfplat.@)
 */
HRESULT WINAPI MFCreateSourceResolver(IMFSourceResolver **resolver)
{
    mfsourceresolver *object;

    TRACE("%p\n", resolver);

    if (!resolver)
        return E_POINTER;

    object = HeapAlloc( GetProcessHeap(), 0, sizeof(*object) );
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFSourceResolver_iface.lpVtbl = &mfsourceresolvervtbl;
    object->ref = 1;

    *resolver = &object->IMFSourceResolver_iface;
    return S_OK;
}

typedef struct _mfmediatype
{
    IMFMediaType IMFMediaType_iface;
    LONG ref;
} mfmediatype;

static inline mfmediatype *impl_from_IMFMediaType(IMFMediaType *iface)
{
    return CONTAINING_RECORD(iface, mfmediatype, IMFMediaType_iface);
}

static HRESULT WINAPI mediatype_QueryInterface(IMFMediaType *iface, REFIID riid, void **object)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), object);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IMFAttributes) ||
       IsEqualGUID(riid, &IID_IMFMediaType))
    {
        *object = &This->IMFMediaType_iface;
    }
    else
    {
        FIXME("(%s, %p)\n", debugstr_guid(riid), object);
        *object = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*object);
    return S_OK;
}

static ULONG WINAPI mediatype_AddRef(IMFMediaType *iface)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI mediatype_Release(IMFMediaType *iface)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI mediatype_GetItem(IMFMediaType *iface, REFGUID key, PROPVARIANT *value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_GetItemType(IMFMediaType *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), type);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_CompareItem(IMFMediaType *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %p, %p\n", This, debugstr_guid(key), value, result);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_Compare(IMFMediaType *iface, IMFAttributes *attrs, MF_ATTRIBUTES_MATCH_TYPE type,
                BOOL *result)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %p, %d, %p\n", This, attrs, type, result);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_GetUINT32(IMFMediaType *iface, REFGUID key, UINT32 *value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_GetUINT64(IMFMediaType *iface, REFGUID key, UINT64 *value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_GetDouble(IMFMediaType *iface, REFGUID key, double *value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_GetGUID(IMFMediaType *iface, REFGUID key, GUID *value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_GetStringLength(IMFMediaType *iface, REFGUID key, UINT32 *length)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), length);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_GetString(IMFMediaType *iface, REFGUID key, WCHAR *value,
                UINT32 size, UINT32 *length)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %p, %d, %p\n", This, debugstr_guid(key), value, size, length);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_GetAllocatedString(IMFMediaType *iface, REFGUID key,
                WCHAR **value, UINT32 *length)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %p, %p\n", This, debugstr_guid(key), value, length);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_GetBlobSize(IMFMediaType *iface, REFGUID key, UINT32 *size)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), size);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_GetBlob(IMFMediaType *iface, REFGUID key, UINT8 *buf,
                UINT32 bufsize, UINT32 *blobsize)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %p, %d, %p\n", This, debugstr_guid(key), buf, bufsize, blobsize);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_GetAllocatedBlob(IMFMediaType *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %p, %p\n", This, debugstr_guid(key), buf, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_GetUnknown(IMFMediaType *iface, REFGUID key, REFIID riid, void **ppv)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %s, %p\n", This, debugstr_guid(key), debugstr_guid(riid), ppv);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_SetItem(IMFMediaType *iface, REFGUID key, REFPROPVARIANT value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_DeleteItem(IMFMediaType *iface, REFGUID key)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s\n", This, debugstr_guid(key));

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_DeleteAllItems(IMFMediaType *iface)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_SetUINT32(IMFMediaType *iface, REFGUID key, UINT32 value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %d\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_SetUINT64(IMFMediaType *iface, REFGUID key, UINT64 value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %s\n", This, debugstr_guid(key), wine_dbgstr_longlong(value));

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_SetDouble(IMFMediaType *iface, REFGUID key, double value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %f\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_SetGUID(IMFMediaType *iface, REFGUID key, REFGUID value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %s\n", This, debugstr_guid(key), debugstr_guid(value));

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_SetString(IMFMediaType *iface, REFGUID key, const WCHAR *value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %s\n", This, debugstr_guid(key), debugstr_w(value));

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_SetBlob(IMFMediaType *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %p, %d\n", This, debugstr_guid(key), buf, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_SetUnknown(IMFMediaType *iface, REFGUID key, IUnknown *unknown)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), unknown);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_LockStore(IMFMediaType *iface)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_UnlockStore(IMFMediaType *iface)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_GetCount(IMFMediaType *iface, UINT32 *items)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %p\n", This, items);

    if(items)
        *items = 0;

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_GetItemByIndex(IMFMediaType *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %d, %p, %p\n", This, index, key, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_CopyAllItems(IMFMediaType *iface, IMFAttributes *dest)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %p\n", This, dest);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_GetMajorType(IMFMediaType *iface, GUID *guid)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %p\n", This, guid);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_IsCompressedFormat(IMFMediaType *iface, BOOL *compressed)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %p\n", This, compressed);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_IsEqual(IMFMediaType *iface, IMFMediaType *type, DWORD *flags)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %p, %p\n", This, type, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_GetRepresentation(IMFMediaType *iface, GUID guid, void **representation)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(&guid), representation);

    return E_NOTIMPL;
}

static HRESULT WINAPI mediatype_FreeRepresentation(IMFMediaType *iface, GUID guid, void *representation)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(&guid), representation);

    return E_NOTIMPL;
}

static const IMFMediaTypeVtbl mediatype_vtbl =
{
    mediatype_QueryInterface,
    mediatype_AddRef,
    mediatype_Release,
    mediatype_GetItem,
    mediatype_GetItemType,
    mediatype_CompareItem,
    mediatype_Compare,
    mediatype_GetUINT32,
    mediatype_GetUINT64,
    mediatype_GetDouble,
    mediatype_GetGUID,
    mediatype_GetStringLength,
    mediatype_GetString,
    mediatype_GetAllocatedString,
    mediatype_GetBlobSize,
    mediatype_GetBlob,
    mediatype_GetAllocatedBlob,
    mediatype_GetUnknown,
    mediatype_SetItem,
    mediatype_DeleteItem,
    mediatype_DeleteAllItems,
    mediatype_SetUINT32,
    mediatype_SetUINT64,
    mediatype_SetDouble,
    mediatype_SetGUID,
    mediatype_SetString,
    mediatype_SetBlob,
    mediatype_SetUnknown,
    mediatype_LockStore,
    mediatype_UnlockStore,
    mediatype_GetCount,
    mediatype_GetItemByIndex,
    mediatype_CopyAllItems,
    mediatype_GetMajorType,
    mediatype_IsCompressedFormat,
    mediatype_IsEqual,
    mediatype_GetRepresentation,
    mediatype_FreeRepresentation
};

/***********************************************************************
 *      MFCreateMediaType (mfplat.@)
 */
HRESULT WINAPI MFCreateMediaType(IMFMediaType **type)
{
    mfmediatype *object;

    TRACE("%p\n", type);

    if(!type)
        return E_INVALIDARG;

    object = HeapAlloc( GetProcessHeap(), 0, sizeof(*object) );
    if(!object)
        return E_OUTOFMEMORY;

    object->ref = 1;
    object->IMFMediaType_iface.lpVtbl = &mediatype_vtbl;

    *type = &object->IMFMediaType_iface;
    return S_OK;
}

typedef struct _mfeventqueue
{
    IMFMediaEventQueue IMFMediaEventQueue_iface;
    LONG ref;
} mfeventqueue;

static inline mfeventqueue *impl_from_IMFMediaEventQueue(IMFMediaEventQueue *iface)
{
    return CONTAINING_RECORD(iface, mfeventqueue, IMFMediaEventQueue_iface);
}

static HRESULT WINAPI mfeventqueue_QueryInterface(IMFMediaEventQueue *iface, REFIID riid, void **out)
{
    mfeventqueue *This = impl_from_IMFMediaEventQueue(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), out);

    if(IsEqualGUID(riid, &IID_IUnknown)      ||
       IsEqualGUID(riid, &IID_IMFMediaEventQueue))
    {
        *out = &This->IMFMediaEventQueue_iface;
    }
    else
    {
        FIXME("(%s, %p)\n", debugstr_guid(riid), out);
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static ULONG WINAPI mfeventqueue_AddRef(IMFMediaEventQueue *iface)
{
    mfeventqueue *This = impl_from_IMFMediaEventQueue(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI mfeventqueue_Release(IMFMediaEventQueue *iface)
{
    mfeventqueue *This = impl_from_IMFMediaEventQueue(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI mfeventqueue_GetEvent(IMFMediaEventQueue *iface, DWORD flags, IMFMediaEvent **event)
{
    mfeventqueue *This = impl_from_IMFMediaEventQueue(iface);

    FIXME("%p, %p\n", This, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfeventqueue_BeginGetEvent(IMFMediaEventQueue *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    mfeventqueue *This = impl_from_IMFMediaEventQueue(iface);

    FIXME("%p, %p, %p\n", This, callback, state);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfeventqueue_EndGetEvent(IMFMediaEventQueue *iface, IMFAsyncResult *result, IMFMediaEvent **event)
{
    mfeventqueue *This = impl_from_IMFMediaEventQueue(iface);

    FIXME("%p, %p, %p\n", This, result, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfeventqueue_QueueEvent(IMFMediaEventQueue *iface, IMFMediaEvent *event)
{
    mfeventqueue *This = impl_from_IMFMediaEventQueue(iface);

    FIXME("%p, %p\n", This, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfeventqueue_QueueEventParamVar(IMFMediaEventQueue *iface, MediaEventType met,
        REFGUID type, HRESULT status, const PROPVARIANT *value)
{
    mfeventqueue *This = impl_from_IMFMediaEventQueue(iface);

    FIXME("%p, %d, %s, 0x%08x, %p\n", This, met, debugstr_guid(type), status, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfeventqueue_QueueEventParamUnk(IMFMediaEventQueue *iface, MediaEventType met, REFGUID type,
        HRESULT status, IUnknown *unk)
{
    mfeventqueue *This = impl_from_IMFMediaEventQueue(iface);

    FIXME("%p, %d, %s, 0x%08x, %p\n", This, met, debugstr_guid(type), status, unk);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfeventqueue_Shutdown(IMFMediaEventQueue *iface)
{
    mfeventqueue *This = impl_from_IMFMediaEventQueue(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static const IMFMediaEventQueueVtbl mfeventqueue_vtbl =
{
    mfeventqueue_QueryInterface,
    mfeventqueue_AddRef,
    mfeventqueue_Release,
    mfeventqueue_GetEvent,
    mfeventqueue_BeginGetEvent,
    mfeventqueue_EndGetEvent,
    mfeventqueue_QueueEvent,
    mfeventqueue_QueueEventParamVar,
    mfeventqueue_QueueEventParamUnk,
    mfeventqueue_Shutdown
};

/***********************************************************************
 *      MFCreateEventQueue (mfplat.@)
 */
HRESULT WINAPI MFCreateEventQueue(IMFMediaEventQueue **queue)
{
    mfeventqueue *object;

    TRACE("%p\n", queue);

    object = HeapAlloc( GetProcessHeap(), 0, sizeof(*object) );
    if(!object)
        return E_OUTOFMEMORY;

    object->ref = 1;
    object->IMFMediaEventQueue_iface.lpVtbl = &mfeventqueue_vtbl;

    *queue = &object->IMFMediaEventQueue_iface;

    return S_OK;
}

typedef struct _mfdescriptor
{
    IMFStreamDescriptor IMFStreamDescriptor_iface;
    LONG ref;
} mfdescriptor;

static inline mfdescriptor *impl_from_IMFStreamDescriptor(IMFStreamDescriptor *iface)
{
    return CONTAINING_RECORD(iface, mfdescriptor, IMFStreamDescriptor_iface);
}

static HRESULT WINAPI mfdescriptor_QueryInterface(IMFStreamDescriptor *iface, REFIID riid, void **out)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), out);

    if(IsEqualGUID(riid, &IID_IUnknown)      ||
       IsEqualGUID(riid, &IID_IMFAttributes) ||
       IsEqualGUID(riid, &IID_IMFStreamDescriptor))
    {
        *out = &This->IMFStreamDescriptor_iface;
    }
    else
    {
        FIXME("(%s, %p)\n", debugstr_guid(riid), out);
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static ULONG WINAPI mfdescriptor_AddRef(IMFStreamDescriptor *iface)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI mfdescriptor_Release(IMFStreamDescriptor *iface)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI mfdescriptor_GetItem(IMFStreamDescriptor *iface, REFGUID key, PROPVARIANT *value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_GetItemType(IMFStreamDescriptor *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), type);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_CompareItem(IMFStreamDescriptor *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %p, %p\n", This, debugstr_guid(key), value, result);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_Compare(IMFStreamDescriptor *iface, IMFAttributes *theirs, MF_ATTRIBUTES_MATCH_TYPE type,
                BOOL *result)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %p, %d, %p\n", This, theirs, type, result);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_GetUINT32(IMFStreamDescriptor *iface, REFGUID key, UINT32 *value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_GetUINT64(IMFStreamDescriptor *iface, REFGUID key, UINT64 *value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_GetDouble(IMFStreamDescriptor *iface, REFGUID key, double *value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_GetGUID(IMFStreamDescriptor *iface, REFGUID key, GUID *value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_GetStringLength(IMFStreamDescriptor *iface, REFGUID key, UINT32 *length)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), length);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_GetString(IMFStreamDescriptor *iface, REFGUID key, WCHAR *value,
                UINT32 size, UINT32 *length)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %p, %d, %p\n", This, debugstr_guid(key), value, size, length);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_GetAllocatedString(IMFStreamDescriptor *iface, REFGUID key,
                                      WCHAR **value, UINT32 *length)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %p, %p\n", This, debugstr_guid(key), value, length);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_GetBlobSize(IMFStreamDescriptor *iface, REFGUID key, UINT32 *size)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), size);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_GetBlob(IMFStreamDescriptor *iface, REFGUID key, UINT8 *buf,
                UINT32 bufsize, UINT32 *blobsize)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %p, %d, %p\n", This, debugstr_guid(key), buf, bufsize, blobsize);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_GetAllocatedBlob(IMFStreamDescriptor *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %p, %p\n", This, debugstr_guid(key), buf, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_GetUnknown(IMFStreamDescriptor *iface, REFGUID key, REFIID riid, void **ppv)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %s, %p\n", This, debugstr_guid(key), debugstr_guid(riid), ppv);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_SetItem(IMFStreamDescriptor *iface, REFGUID key, REFPROPVARIANT Value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), Value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_DeleteItem(IMFStreamDescriptor *iface, REFGUID key)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s\n", This, debugstr_guid(key));

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_DeleteAllItems(IMFStreamDescriptor *iface)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_SetUINT32(IMFStreamDescriptor *iface, REFGUID key, UINT32 value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %d\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_SetUINT64(IMFStreamDescriptor *iface, REFGUID key, UINT64 value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %s\n", This, debugstr_guid(key), wine_dbgstr_longlong(value));

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_SetDouble(IMFStreamDescriptor *iface, REFGUID key, double value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %f\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_SetGUID(IMFStreamDescriptor *iface, REFGUID key, REFGUID value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %s\n", This, debugstr_guid(key), debugstr_guid(value));

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_SetString(IMFStreamDescriptor *iface, REFGUID key, const WCHAR *value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %s\n", This, debugstr_guid(key), debugstr_w(value));

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_SetBlob(IMFStreamDescriptor *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %p, %d\n", This, debugstr_guid(key), buf, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_SetUnknown(IMFStreamDescriptor *iface, REFGUID key, IUnknown *unknown)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), unknown);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_LockStore(IMFStreamDescriptor *iface)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_UnlockStore(IMFStreamDescriptor *iface)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_GetCount(IMFStreamDescriptor *iface, UINT32 *items)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %p\n", This, items);

    if(items)
        *items = 0;

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_GetItemByIndex(IMFStreamDescriptor *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %d, %p, %p\n", This, index, key, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_CopyAllItems(IMFStreamDescriptor *iface, IMFAttributes *dest)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %p\n", This, dest);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_GetStreamIdentifier(IMFStreamDescriptor *iface, DWORD *identifier)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %p\n", This, identifier);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfdescriptor_GetMediaTypeHandler(IMFStreamDescriptor *iface, IMFMediaTypeHandler **handler)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);

    FIXME("%p, %p\n", This, handler);

    return E_NOTIMPL;
}

static const IMFStreamDescriptorVtbl mfdescriptor_vtbl =
{
    mfdescriptor_QueryInterface,
    mfdescriptor_AddRef,
    mfdescriptor_Release,
    mfdescriptor_GetItem,
    mfdescriptor_GetItemType,
    mfdescriptor_CompareItem,
    mfdescriptor_Compare,
    mfdescriptor_GetUINT32,
    mfdescriptor_GetUINT64,
    mfdescriptor_GetDouble,
    mfdescriptor_GetGUID,
    mfdescriptor_GetStringLength,
    mfdescriptor_GetString,
    mfdescriptor_GetAllocatedString,
    mfdescriptor_GetBlobSize,
    mfdescriptor_GetBlob,
    mfdescriptor_GetAllocatedBlob,
    mfdescriptor_GetUnknown,
    mfdescriptor_SetItem,
    mfdescriptor_DeleteItem,
    mfdescriptor_DeleteAllItems,
    mfdescriptor_SetUINT32,
    mfdescriptor_SetUINT64,
    mfdescriptor_SetDouble,
    mfdescriptor_SetGUID,
    mfdescriptor_SetString,
    mfdescriptor_SetBlob,
    mfdescriptor_SetUnknown,
    mfdescriptor_LockStore,
    mfdescriptor_UnlockStore,
    mfdescriptor_GetCount,
    mfdescriptor_GetItemByIndex,
    mfdescriptor_CopyAllItems,
    mfdescriptor_GetStreamIdentifier,
    mfdescriptor_GetMediaTypeHandler
};

HRESULT WINAPI MFCreateStreamDescriptor(DWORD identifier, DWORD count,
        IMFMediaType **types, IMFStreamDescriptor **descriptor)
{
    mfdescriptor *object;

    TRACE("%d, %d, %p, %p\n", identifier, count, types, descriptor);

    object = HeapAlloc( GetProcessHeap(), 0, sizeof(*object) );
    if(!object)
        return E_OUTOFMEMORY;

    object->ref = 1;
    object->IMFStreamDescriptor_iface.lpVtbl = &mfdescriptor_vtbl;

    *descriptor = &object->IMFStreamDescriptor_iface;
    return S_OK;
}
