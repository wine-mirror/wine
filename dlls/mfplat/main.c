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

#include "wine/heap.h"
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

HRESULT WINAPI MFTRegisterLocal(IClassFactory *factory, REFGUID category, LPCWSTR name,
                           UINT32 flags, UINT32 cinput, const MFT_REGISTER_TYPE_INFO *input_types,
                           UINT32 coutput, const MFT_REGISTER_TYPE_INFO* output_types)
{
    FIXME("(%p, %s, %s, %x, %u, %p, %u, %p)\n", factory, debugstr_guid(category), debugstr_w(name),
                                                flags, cinput, input_types, coutput, output_types);

    return S_OK;
}

HRESULT WINAPI MFTUnregisterLocal(IClassFactory *factory)
{
    FIXME("(%p)\n", factory);

    return S_OK;
}

MFTIME WINAPI MFGetSystemTime(void)
{
    MFTIME mf;

    TRACE("()\n");

    GetSystemTimeAsFileTime( (FILETIME*)&mf );

    return mf;
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

HRESULT WINAPI MFCopyImage(BYTE *dest, LONG deststride, const BYTE *src, LONG srcstride, DWORD width, DWORD lines)
{
    FIXME("(%p, %d, %p, %d, %d, %d) stub\n", dest, deststride, src, srcstride, width, lines);
    return E_NOTIMPL;
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

static void init_attribute_object(mfattributes *object, UINT32 size)
{
    object->ref = 1;
    object->IMFAttributes_iface.lpVtbl = &mfattributes_vtbl;
}

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

    init_attribute_object(object, size);
    *attributes = &object->IMFAttributes_iface;

    return S_OK;
}

typedef struct _mfbytestream
{
    mfattributes attributes;
    IMFByteStream IMFByteStream_iface;
} mfbytestream;

static inline mfbytestream *impl_from_IMFByteStream(IMFByteStream *iface)
{
    return CONTAINING_RECORD(iface, mfbytestream, IMFByteStream_iface);
}

static HRESULT WINAPI mfbytestream_QueryInterface(IMFByteStream *iface, REFIID riid, void **out)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), out);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IMFByteStream))
    {
        *out = &This->IMFByteStream_iface;
    }
    else if(IsEqualGUID(riid, &IID_IMFAttributes))
    {
        *out = &This->attributes.IMFAttributes_iface;
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

static ULONG WINAPI mfbytestream_AddRef(IMFByteStream *iface)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);
    ULONG ref = InterlockedIncrement(&This->attributes.ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI mfbytestream_Release(IMFByteStream *iface)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);
    ULONG ref = InterlockedDecrement(&This->attributes.ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI mfbytestream_GetCapabilities(IMFByteStream *iface, DWORD *capabilities)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);

    FIXME("%p, %p\n", This, capabilities);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfbytestream_GetLength(IMFByteStream *iface, QWORD *length)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);

    FIXME("%p, %p\n", This, length);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfbytestream_SetLength(IMFByteStream *iface, QWORD length)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);

    FIXME("%p, %s\n", This, wine_dbgstr_longlong(length));

    return E_NOTIMPL;
}

static HRESULT WINAPI mfbytestream_GetCurrentPosition(IMFByteStream *iface, QWORD *position)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);

    FIXME("%p, %p\n", This, position);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfbytestream_SetCurrentPosition(IMFByteStream *iface, QWORD position)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);

    FIXME("%p, %s\n", This, wine_dbgstr_longlong(position));

    return E_NOTIMPL;
}

static HRESULT WINAPI mfbytestream_IsEndOfStream(IMFByteStream *iface, BOOL *endstream)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);

    FIXME("%p, %p\n", This, endstream);

    if(endstream)
        *endstream = TRUE;

    return S_OK;
}

static HRESULT WINAPI mfbytestream_Read(IMFByteStream *iface, BYTE *data, ULONG count, ULONG *byte_read)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);

    FIXME("%p, %p, %u, %p\n", This, data, count, byte_read);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfbytestream_BeginRead(IMFByteStream *iface, BYTE *data, ULONG count,
                        IMFAsyncCallback *callback, IUnknown *state)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);

    FIXME("%p, %p, %u, %p, %p\n", This, data, count, callback, state);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfbytestream_EndRead(IMFByteStream *iface, IMFAsyncResult *result, ULONG *byte_read)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);

    FIXME("%p, %p, %p\n", This, result, byte_read);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfbytestream_Write(IMFByteStream *iface, const BYTE *data, ULONG count, ULONG *written)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);

    FIXME("%p, %p, %u, %p\n", This, data, count, written);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfbytestream_BeginWrite(IMFByteStream *iface, const BYTE *data, ULONG count,
                        IMFAsyncCallback *callback, IUnknown *state)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);

    FIXME("%p, %p, %u, %p, %p\n", This, data, count, callback, state);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfbytestream_EndWrite(IMFByteStream *iface, IMFAsyncResult *result, ULONG *written)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);

    FIXME("%p, %p, %p\n", This, result, written);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfbytestream_Seek(IMFByteStream *iface, MFBYTESTREAM_SEEK_ORIGIN seek, LONGLONG offset,
                        DWORD flags, QWORD *current)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);

    FIXME("%p, %u, %s, 0x%08x, %p\n", This, seek, wine_dbgstr_longlong(offset), flags, current);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfbytestream_Flush(IMFByteStream *iface)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfbytestream_Close(IMFByteStream *iface)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static const IMFByteStreamVtbl mfbytestream_vtbl =
{
    mfbytestream_QueryInterface,
    mfbytestream_AddRef,
    mfbytestream_Release,
    mfbytestream_GetCapabilities,
    mfbytestream_GetLength,
    mfbytestream_SetLength,
    mfbytestream_GetCurrentPosition,
    mfbytestream_SetCurrentPosition,
    mfbytestream_IsEndOfStream,
    mfbytestream_Read,
    mfbytestream_BeginRead,
    mfbytestream_EndRead,
    mfbytestream_Write,
    mfbytestream_BeginWrite,
    mfbytestream_EndWrite,
    mfbytestream_Seek,
    mfbytestream_Flush,
    mfbytestream_Close
};

static inline mfbytestream *impl_from_IMFByteStream_IMFAttributes(IMFAttributes *iface)
{
    return CONTAINING_RECORD(iface, mfbytestream, attributes.IMFAttributes_iface);
}

static HRESULT WINAPI mfbytestream_attributes_QueryInterface(
    IMFAttributes *iface, REFIID riid, void **out)
{
    mfbytestream *This = impl_from_IMFByteStream_IMFAttributes(iface);
    return IMFByteStream_QueryInterface(&This->IMFByteStream_iface, riid, out);
}

static ULONG WINAPI mfbytestream_attributes_AddRef(IMFAttributes *iface)
{
    mfbytestream *This = impl_from_IMFByteStream_IMFAttributes(iface);
    return IMFByteStream_AddRef(&This->IMFByteStream_iface);
}

static ULONG WINAPI mfbytestream_attributes_Release(IMFAttributes *iface)
{
    mfbytestream *This = impl_from_IMFByteStream_IMFAttributes(iface);
    return IMFByteStream_Release(&This->IMFByteStream_iface);
}

static const IMFAttributesVtbl mfbytestream_attributes_vtbl =
{
    mfbytestream_attributes_QueryInterface,
    mfbytestream_attributes_AddRef,
    mfbytestream_attributes_Release,
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

HRESULT WINAPI MFCreateMFByteStreamOnStream(IStream *stream, IMFByteStream **bytestream)
{
    mfbytestream *object;

    TRACE("(%p, %p): stub\n", stream, bytestream);

    object = heap_alloc( sizeof(*object) );
    if(!object)
        return E_OUTOFMEMORY;

    init_attribute_object(&object->attributes, 0);
    object->IMFByteStream_iface.lpVtbl = &mfbytestream_vtbl;
    object->attributes.IMFAttributes_iface.lpVtbl = &mfbytestream_attributes_vtbl;

    *bytestream = &object->IMFByteStream_iface;

    return S_OK;
}

HRESULT WINAPI MFCreateFile(MF_FILE_ACCESSMODE accessmode, MF_FILE_OPENMODE openmode, MF_FILE_FLAGS flags,
                            LPCWSTR url, IMFByteStream **bytestream)
{
    mfbytestream *object;
    DWORD fileaccessmode = 0;
    DWORD filesharemode = FILE_SHARE_READ;
    DWORD filecreation_disposition = 0;
    DWORD fileattributes = 0;
    HANDLE file;

    FIXME("(%d, %d, %d, %s, %p): stub\n", accessmode, openmode, flags, debugstr_w(url), bytestream);

    switch (accessmode)
    {
        case MF_ACCESSMODE_READ:
            fileaccessmode = GENERIC_READ;
            break;
        case MF_ACCESSMODE_WRITE:
            fileaccessmode = GENERIC_WRITE;
            break;
        case MF_ACCESSMODE_READWRITE:
            fileaccessmode = GENERIC_READ | GENERIC_WRITE;
            break;
    }

    switch (openmode)
    {
        case MF_OPENMODE_FAIL_IF_NOT_EXIST:
            filecreation_disposition = OPEN_EXISTING;
            break;
        case MF_OPENMODE_FAIL_IF_EXIST:
            filecreation_disposition = CREATE_NEW;
            break;
        case MF_OPENMODE_RESET_IF_EXIST:
            filecreation_disposition = TRUNCATE_EXISTING;
            break;
        case MF_OPENMODE_APPEND_IF_EXIST:
            filecreation_disposition = OPEN_ALWAYS;
            fileaccessmode |= FILE_APPEND_DATA;
            break;
        case MF_OPENMODE_DELETE_IF_EXIST:
            filecreation_disposition = CREATE_ALWAYS;
            break;
    }

    if (flags & MF_FILEFLAGS_NOBUFFERING)
        fileattributes |= FILE_FLAG_NO_BUFFERING;

    /* Open HANDLE to file */
    file = CreateFileW(url, fileaccessmode, filesharemode, NULL,
                       filecreation_disposition, fileattributes, 0);

    if(file == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    /* Close the file again, since we don't do anything with it yet */
    CloseHandle(file);

    object = heap_alloc( sizeof(*object) );
    if(!object)
        return E_OUTOFMEMORY;

    init_attribute_object(&object->attributes, 0);
    object->IMFByteStream_iface.lpVtbl = &mfbytestream_vtbl;
    object->attributes.IMFAttributes_iface.lpVtbl = &mfbytestream_attributes_vtbl;

    *bytestream = &object->IMFByteStream_iface;

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

typedef struct _mfpresentationdescriptor
{
    mfattributes attributes;
    IMFPresentationDescriptor IMFPresentationDescriptor_iface;
} mfpresentationdescriptor;

static inline mfpresentationdescriptor *impl_from_IMFPresentationDescriptor(IMFPresentationDescriptor *iface)
{
    return CONTAINING_RECORD(iface, mfpresentationdescriptor, IMFPresentationDescriptor_iface);
}

static HRESULT WINAPI mfpresentationdescriptor_QueryInterface(IMFPresentationDescriptor *iface, REFIID riid, void **out)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), out);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IMFAttributes) ||
       IsEqualGUID(riid, &IID_IMFPresentationDescriptor))
    {
        *out = &This->IMFPresentationDescriptor_iface;
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

static ULONG WINAPI mfpresentationdescriptor_AddRef(IMFPresentationDescriptor *iface)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    ULONG ref = InterlockedIncrement(&This->attributes.ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI mfpresentationdescriptor_Release(IMFPresentationDescriptor *iface)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    ULONG ref = InterlockedDecrement(&This->attributes.ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI mfpresentationdescriptor_GetItem(IMFPresentationDescriptor *iface, REFGUID key, PROPVARIANT *value)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_GetItem(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfpresentationdescriptor_GetItemType(IMFPresentationDescriptor *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_GetItemType(&This->attributes.IMFAttributes_iface, key, type);
}

static HRESULT WINAPI mfpresentationdescriptor_CompareItem(IMFPresentationDescriptor *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_CompareItem(&This->attributes.IMFAttributes_iface, key, value, result);
}

static HRESULT WINAPI mfpresentationdescriptor_Compare(IMFPresentationDescriptor *iface, IMFAttributes *attrs, MF_ATTRIBUTES_MATCH_TYPE type,
                BOOL *result)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_Compare(&This->attributes.IMFAttributes_iface, attrs, type, result);
}

static HRESULT WINAPI mfpresentationdescriptor_GetUINT32(IMFPresentationDescriptor *iface, REFGUID key, UINT32 *value)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_GetUINT32(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfpresentationdescriptor_GetUINT64(IMFPresentationDescriptor *iface, REFGUID key, UINT64 *value)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_GetUINT64(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfpresentationdescriptor_GetDouble(IMFPresentationDescriptor *iface, REFGUID key, double *value)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_GetDouble(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfpresentationdescriptor_GetGUID(IMFPresentationDescriptor *iface, REFGUID key, GUID *value)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_GetGUID(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfpresentationdescriptor_GetStringLength(IMFPresentationDescriptor *iface, REFGUID key, UINT32 *length)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_GetStringLength(&This->attributes.IMFAttributes_iface, key, length);
}

static HRESULT WINAPI mfpresentationdescriptor_GetString(IMFPresentationDescriptor *iface, REFGUID key, WCHAR *value,
                UINT32 size, UINT32 *length)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_GetString(&This->attributes.IMFAttributes_iface, key, value, size, length);
}

static HRESULT WINAPI mfpresentationdescriptor_GetAllocatedString(IMFPresentationDescriptor *iface, REFGUID key,
                WCHAR **value, UINT32 *length)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_GetAllocatedString(&This->attributes.IMFAttributes_iface, key, value, length);
}

static HRESULT WINAPI mfpresentationdescriptor_GetBlobSize(IMFPresentationDescriptor *iface, REFGUID key, UINT32 *size)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_GetBlobSize(&This->attributes.IMFAttributes_iface, key, size);
}

static HRESULT WINAPI mfpresentationdescriptor_GetBlob(IMFPresentationDescriptor *iface, REFGUID key, UINT8 *buf,
                UINT32 bufsize, UINT32 *blobsize)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_GetBlob(&This->attributes.IMFAttributes_iface, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI mfpresentationdescriptor_GetAllocatedBlob(IMFPresentationDescriptor *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_GetAllocatedBlob(&This->attributes.IMFAttributes_iface, key, buf, size);
}

static HRESULT WINAPI mfpresentationdescriptor_GetUnknown(IMFPresentationDescriptor *iface, REFGUID key, REFIID riid, void **ppv)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_GetUnknown(&This->attributes.IMFAttributes_iface, key, riid, ppv);
}

static HRESULT WINAPI mfpresentationdescriptor_SetItem(IMFPresentationDescriptor *iface, REFGUID key, REFPROPVARIANT value)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_SetItem(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfpresentationdescriptor_DeleteItem(IMFPresentationDescriptor *iface, REFGUID key)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_DeleteItem(&This->attributes.IMFAttributes_iface, key);
}

static HRESULT WINAPI mfpresentationdescriptor_DeleteAllItems(IMFPresentationDescriptor *iface)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_DeleteAllItems(&This->attributes.IMFAttributes_iface);
}

static HRESULT WINAPI mfpresentationdescriptor_SetUINT32(IMFPresentationDescriptor *iface, REFGUID key, UINT32 value)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_SetUINT32(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfpresentationdescriptor_SetUINT64(IMFPresentationDescriptor *iface, REFGUID key, UINT64 value)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_SetUINT64(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfpresentationdescriptor_SetDouble(IMFPresentationDescriptor *iface, REFGUID key, double value)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_SetDouble(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfpresentationdescriptor_SetGUID(IMFPresentationDescriptor *iface, REFGUID key, REFGUID value)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_SetGUID(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfpresentationdescriptor_SetString(IMFPresentationDescriptor *iface, REFGUID key, const WCHAR *value)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_SetString(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfpresentationdescriptor_SetBlob(IMFPresentationDescriptor *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_SetBlob(&This->attributes.IMFAttributes_iface, key, buf, size);
}

static HRESULT WINAPI mfpresentationdescriptor_SetUnknown(IMFPresentationDescriptor *iface, REFGUID key, IUnknown *unknown)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_SetUnknown(&This->attributes.IMFAttributes_iface, key, unknown);
}

static HRESULT WINAPI mfpresentationdescriptor_LockStore(IMFPresentationDescriptor *iface)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_LockStore(&This->attributes.IMFAttributes_iface);
}

static HRESULT WINAPI mfpresentationdescriptor_UnlockStore(IMFPresentationDescriptor *iface)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_UnlockStore(&This->attributes.IMFAttributes_iface);
}

static HRESULT WINAPI mfpresentationdescriptor_GetCount(IMFPresentationDescriptor *iface, UINT32 *items)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_GetCount(&This->attributes.IMFAttributes_iface, items);
}

static HRESULT WINAPI mfpresentationdescriptor_GetItemByIndex(IMFPresentationDescriptor *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);
    return IMFAttributes_GetItemByIndex(&This->attributes.IMFAttributes_iface, index, key, value);
}

static HRESULT WINAPI mfpresentationdescriptor_CopyAllItems(IMFPresentationDescriptor *iface, IMFAttributes *dest)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);

    FIXME("%p, %p\n", This, dest);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfpresentationdescriptor_GetStreamDescriptorCount(IMFPresentationDescriptor *iface, DWORD *descriptor_count)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);

    FIXME("%p, %p\n", This, descriptor_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfpresentationdescriptor_GetStreamDescriptorByIndex(IMFPresentationDescriptor *iface, DWORD index,
                                                                          BOOL *selected, IMFStreamDescriptor **descriptor)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);

    FIXME("%p, %#x, %p, %p\n", This, index, selected, descriptor);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfpresentationdescriptor_SelectStream(IMFPresentationDescriptor *iface, DWORD index)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);

    FIXME("%p, %#x\n", This, index);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfpresentationdescriptor_DeselectStream(IMFPresentationDescriptor *iface, DWORD index)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);

    FIXME("%p, %#x\n", This, index);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfpresentationdescriptor_Clone(IMFPresentationDescriptor *iface, IMFPresentationDescriptor **descriptor)
{
    mfpresentationdescriptor *This = impl_from_IMFPresentationDescriptor(iface);

    FIXME("%p, %p\n", This, descriptor);

    return E_NOTIMPL;
}

static const IMFPresentationDescriptorVtbl mfpresentationdescriptor_vtbl =
{
    mfpresentationdescriptor_QueryInterface,
    mfpresentationdescriptor_AddRef,
    mfpresentationdescriptor_Release,
    mfpresentationdescriptor_GetItem,
    mfpresentationdescriptor_GetItemType,
    mfpresentationdescriptor_CompareItem,
    mfpresentationdescriptor_Compare,
    mfpresentationdescriptor_GetUINT32,
    mfpresentationdescriptor_GetUINT64,
    mfpresentationdescriptor_GetDouble,
    mfpresentationdescriptor_GetGUID,
    mfpresentationdescriptor_GetStringLength,
    mfpresentationdescriptor_GetString,
    mfpresentationdescriptor_GetAllocatedString,
    mfpresentationdescriptor_GetBlobSize,
    mfpresentationdescriptor_GetBlob,
    mfpresentationdescriptor_GetAllocatedBlob,
    mfpresentationdescriptor_GetUnknown,
    mfpresentationdescriptor_SetItem,
    mfpresentationdescriptor_DeleteItem,
    mfpresentationdescriptor_DeleteAllItems,
    mfpresentationdescriptor_SetUINT32,
    mfpresentationdescriptor_SetUINT64,
    mfpresentationdescriptor_SetDouble,
    mfpresentationdescriptor_SetGUID,
    mfpresentationdescriptor_SetString,
    mfpresentationdescriptor_SetBlob,
    mfpresentationdescriptor_SetUnknown,
    mfpresentationdescriptor_LockStore,
    mfpresentationdescriptor_UnlockStore,
    mfpresentationdescriptor_GetCount,
    mfpresentationdescriptor_GetItemByIndex,
    mfpresentationdescriptor_CopyAllItems,
    mfpresentationdescriptor_GetStreamDescriptorCount,
    mfpresentationdescriptor_GetStreamDescriptorByIndex,
    mfpresentationdescriptor_SelectStream,
    mfpresentationdescriptor_DeselectStream,
    mfpresentationdescriptor_Clone,
};

typedef struct _mfsource
{
    IMFMediaSource IMFMediaSource_iface;
    LONG ref;
} mfsource;

static inline mfsource *impl_from_IMFMediaSource(IMFMediaSource *iface)
{
    return CONTAINING_RECORD(iface, mfsource, IMFMediaSource_iface);
}

static HRESULT WINAPI mfsource_QueryInterface(IMFMediaSource *iface, REFIID riid, void **out)
{
    mfsource *This = impl_from_IMFMediaSource(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFMediaSource) ||
        IsEqualIID(riid, &IID_IMFMediaEventGenerator) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &This->IMFMediaSource_iface;
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

static ULONG WINAPI mfsource_AddRef(IMFMediaSource *iface)
{
    mfsource *This = impl_from_IMFMediaSource(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI mfsource_Release(IMFMediaSource *iface)
{
    mfsource *This = impl_from_IMFMediaSource(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI mfsource_GetEvent(IMFMediaSource *iface, DWORD flags, IMFMediaEvent **event)
{
    mfsource *This = impl_from_IMFMediaSource(iface);

    FIXME("(%p)->(%#x, %p)\n", This, flags, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsource_BeginGetEvent(IMFMediaSource *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    mfsource *This = impl_from_IMFMediaSource(iface);

    FIXME("(%p)->(%p, %p)\n", This, callback, state);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsource_EndGetEvent(IMFMediaSource *iface, IMFAsyncResult *result, IMFMediaEvent **event)
{
    mfsource *This = impl_from_IMFMediaSource(iface);

    FIXME("(%p)->(%p, %p)\n", This, result, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsource_QueueEvent(IMFMediaSource *iface, MediaEventType event_type, REFGUID ext_type,
        HRESULT hr, const PROPVARIANT *value)
{
    mfsource *This = impl_from_IMFMediaSource(iface);

    FIXME("(%p)->(%d, %s, %#x, %p)\n", This, event_type, debugstr_guid(ext_type), hr, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsource_GetCharacteristics(IMFMediaSource *iface, DWORD *characteristics)
{
    mfsource *This = impl_from_IMFMediaSource(iface);

    FIXME("(%p)->(%p): stub\n", This, characteristics);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsource_CreatePresentationDescriptor(IMFMediaSource *iface, IMFPresentationDescriptor **descriptor)
{
    mfsource *This = impl_from_IMFMediaSource(iface);

    mfpresentationdescriptor *object;

    FIXME("(%p)->(%p): stub\n", This, descriptor);

    object = HeapAlloc( GetProcessHeap(), 0, sizeof(*object) );
    if (!object)
        return E_OUTOFMEMORY;

    init_attribute_object(&object->attributes, 0);
    object->IMFPresentationDescriptor_iface.lpVtbl = &mfpresentationdescriptor_vtbl;

    *descriptor = &object->IMFPresentationDescriptor_iface;
    return S_OK;
}

static HRESULT WINAPI mfsource_Start(IMFMediaSource *iface, IMFPresentationDescriptor *descriptor,
                                     const GUID *time_format, const PROPVARIANT *start_position)
{
    mfsource *This = impl_from_IMFMediaSource(iface);

    FIXME("(%p)->(%p, %p, %p): stub\n", This, descriptor, time_format, start_position);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsource_Stop(IMFMediaSource *iface)
{
    mfsource *This = impl_from_IMFMediaSource(iface);

    FIXME("(%p): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsource_Pause(IMFMediaSource *iface)
{
    mfsource *This = impl_from_IMFMediaSource(iface);

    FIXME("(%p): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsource_Shutdown(IMFMediaSource *iface)
{
    mfsource *This = impl_from_IMFMediaSource(iface);

    FIXME("(%p): stub\n", This);

    return S_OK;
}

static const IMFMediaSourceVtbl mfsourcevtbl =
{
    mfsource_QueryInterface,
    mfsource_AddRef,
    mfsource_Release,
    mfsource_GetEvent,
    mfsource_BeginGetEvent,
    mfsource_EndGetEvent,
    mfsource_QueueEvent,
    mfsource_GetCharacteristics,
    mfsource_CreatePresentationDescriptor,
    mfsource_Start,
    mfsource_Stop,
    mfsource_Pause,
    mfsource_Shutdown,
};

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

    if (!stream || !obj_type || !object)
        return E_POINTER;

    if (flags & MF_RESOLUTION_MEDIASOURCE)
    {
        mfsource *new_object;

        new_object = HeapAlloc( GetProcessHeap(), 0, sizeof(*new_object) );
        if (!new_object)
            return E_OUTOFMEMORY;

        new_object->IMFMediaSource_iface.lpVtbl = &mfsourcevtbl;
        new_object->ref = 1;

        *object = (IUnknown *)&new_object->IMFMediaSource_iface;
        *obj_type = MF_OBJECT_MEDIASOURCE;
        return S_OK;
    }

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
    mfattributes attributes;
    IMFMediaType IMFMediaType_iface;
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
    ULONG ref = InterlockedIncrement(&This->attributes.ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI mediatype_Release(IMFMediaType *iface)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    ULONG ref = InterlockedDecrement(&This->attributes.ref);

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
    return IMFAttributes_GetItem(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mediatype_GetItemType(IMFMediaType *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_GetItemType(&This->attributes.IMFAttributes_iface, key, type);
}

static HRESULT WINAPI mediatype_CompareItem(IMFMediaType *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_CompareItem(&This->attributes.IMFAttributes_iface, key, value, result);
}

static HRESULT WINAPI mediatype_Compare(IMFMediaType *iface, IMFAttributes *attrs, MF_ATTRIBUTES_MATCH_TYPE type,
                BOOL *result)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_Compare(&This->attributes.IMFAttributes_iface, attrs, type, result);
}

static HRESULT WINAPI mediatype_GetUINT32(IMFMediaType *iface, REFGUID key, UINT32 *value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_GetUINT32(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mediatype_GetUINT64(IMFMediaType *iface, REFGUID key, UINT64 *value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_GetUINT64(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mediatype_GetDouble(IMFMediaType *iface, REFGUID key, double *value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_GetDouble(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mediatype_GetGUID(IMFMediaType *iface, REFGUID key, GUID *value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_GetGUID(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mediatype_GetStringLength(IMFMediaType *iface, REFGUID key, UINT32 *length)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_GetStringLength(&This->attributes.IMFAttributes_iface, key, length);
}

static HRESULT WINAPI mediatype_GetString(IMFMediaType *iface, REFGUID key, WCHAR *value,
                UINT32 size, UINT32 *length)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_GetString(&This->attributes.IMFAttributes_iface, key, value, size, length);
}

static HRESULT WINAPI mediatype_GetAllocatedString(IMFMediaType *iface, REFGUID key,
                WCHAR **value, UINT32 *length)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_GetAllocatedString(&This->attributes.IMFAttributes_iface, key, value, length);
}

static HRESULT WINAPI mediatype_GetBlobSize(IMFMediaType *iface, REFGUID key, UINT32 *size)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_GetBlobSize(&This->attributes.IMFAttributes_iface, key, size);
}

static HRESULT WINAPI mediatype_GetBlob(IMFMediaType *iface, REFGUID key, UINT8 *buf,
                UINT32 bufsize, UINT32 *blobsize)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_GetBlob(&This->attributes.IMFAttributes_iface, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI mediatype_GetAllocatedBlob(IMFMediaType *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_GetAllocatedBlob(&This->attributes.IMFAttributes_iface, key, buf, size);
}

static HRESULT WINAPI mediatype_GetUnknown(IMFMediaType *iface, REFGUID key, REFIID riid, void **ppv)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_GetUnknown(&This->attributes.IMFAttributes_iface, key, riid, ppv);
}

static HRESULT WINAPI mediatype_SetItem(IMFMediaType *iface, REFGUID key, REFPROPVARIANT value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_SetItem(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mediatype_DeleteItem(IMFMediaType *iface, REFGUID key)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_DeleteItem(&This->attributes.IMFAttributes_iface, key);
}

static HRESULT WINAPI mediatype_DeleteAllItems(IMFMediaType *iface)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_DeleteAllItems(&This->attributes.IMFAttributes_iface);
}

static HRESULT WINAPI mediatype_SetUINT32(IMFMediaType *iface, REFGUID key, UINT32 value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_SetUINT32(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mediatype_SetUINT64(IMFMediaType *iface, REFGUID key, UINT64 value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_SetUINT64(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mediatype_SetDouble(IMFMediaType *iface, REFGUID key, double value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_SetDouble(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mediatype_SetGUID(IMFMediaType *iface, REFGUID key, REFGUID value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_SetGUID(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mediatype_SetString(IMFMediaType *iface, REFGUID key, const WCHAR *value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_SetString(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mediatype_SetBlob(IMFMediaType *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_SetBlob(&This->attributes.IMFAttributes_iface, key, buf, size);
}

static HRESULT WINAPI mediatype_SetUnknown(IMFMediaType *iface, REFGUID key, IUnknown *unknown)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_SetUnknown(&This->attributes.IMFAttributes_iface, key, unknown);
}

static HRESULT WINAPI mediatype_LockStore(IMFMediaType *iface)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_LockStore(&This->attributes.IMFAttributes_iface);
}

static HRESULT WINAPI mediatype_UnlockStore(IMFMediaType *iface)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_UnlockStore(&This->attributes.IMFAttributes_iface);
}

static HRESULT WINAPI mediatype_GetCount(IMFMediaType *iface, UINT32 *items)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_GetCount(&This->attributes.IMFAttributes_iface, items);
}

static HRESULT WINAPI mediatype_GetItemByIndex(IMFMediaType *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    mfmediatype *This = impl_from_IMFMediaType(iface);
    return IMFAttributes_GetItemByIndex(&This->attributes.IMFAttributes_iface, index, key, value);
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

    object = heap_alloc( sizeof(*object) );
    if(!object)
        return E_OUTOFMEMORY;

    init_attribute_object(&object->attributes, 0);
    object->IMFMediaType_iface.lpVtbl = &mediatype_vtbl;

    *type = &object->IMFMediaType_iface;

    return S_OK;
}

typedef struct _mfmediaevent
{
    mfattributes attributes;
    IMFMediaEvent IMFMediaEvent_iface;

    MediaEventType type;
    GUID extended_type;
    HRESULT status;
    PROPVARIANT value;
} mfmediaevent;

static inline mfmediaevent *impl_from_IMFMediaEvent(IMFMediaEvent *iface)
{
    return CONTAINING_RECORD(iface, mfmediaevent, IMFMediaEvent_iface);
}

static HRESULT WINAPI mfmediaevent_QueryInterface(IMFMediaEvent *iface, REFIID riid, void **out)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), out);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IMFAttributes) ||
       IsEqualGUID(riid, &IID_IMFMediaEvent))
    {
        *out = &This->IMFMediaEvent_iface;
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

static ULONG WINAPI mfmediaevent_AddRef(IMFMediaEvent *iface)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    ULONG ref = InterlockedIncrement(&This->attributes.ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI mfmediaevent_Release(IMFMediaEvent *iface)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    ULONG ref = InterlockedDecrement(&This->attributes.ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI mfmediaevent_GetItem(IMFMediaEvent *iface, REFGUID key, PROPVARIANT *value)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_GetItem(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfmediaevent_GetItemType(IMFMediaEvent *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_GetItemType(&This->attributes.IMFAttributes_iface, key, type);
}

static HRESULT WINAPI mfmediaevent_CompareItem(IMFMediaEvent *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_CompareItem(&This->attributes.IMFAttributes_iface, key, value, result);
}

static HRESULT WINAPI mfmediaevent_Compare(IMFMediaEvent *iface, IMFAttributes *attrs, MF_ATTRIBUTES_MATCH_TYPE type,
                BOOL *result)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_Compare(&This->attributes.IMFAttributes_iface, attrs, type, result);
}

static HRESULT WINAPI mfmediaevent_GetUINT32(IMFMediaEvent *iface, REFGUID key, UINT32 *value)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_GetUINT32(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfmediaevent_GetUINT64(IMFMediaEvent *iface, REFGUID key, UINT64 *value)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_GetUINT64(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfmediaevent_GetDouble(IMFMediaEvent *iface, REFGUID key, double *value)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_GetDouble(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfmediaevent_GetGUID(IMFMediaEvent *iface, REFGUID key, GUID *value)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_GetGUID(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfmediaevent_GetStringLength(IMFMediaEvent *iface, REFGUID key, UINT32 *length)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_GetStringLength(&This->attributes.IMFAttributes_iface, key, length);
}

static HRESULT WINAPI mfmediaevent_GetString(IMFMediaEvent *iface, REFGUID key, WCHAR *value,
                UINT32 size, UINT32 *length)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_GetString(&This->attributes.IMFAttributes_iface, key, value, size, length);
}

static HRESULT WINAPI mfmediaevent_GetAllocatedString(IMFMediaEvent *iface, REFGUID key,
                WCHAR **value, UINT32 *length)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_GetAllocatedString(&This->attributes.IMFAttributes_iface, key, value, length);
}

static HRESULT WINAPI mfmediaevent_GetBlobSize(IMFMediaEvent *iface, REFGUID key, UINT32 *size)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_GetBlobSize(&This->attributes.IMFAttributes_iface, key, size);
}

static HRESULT WINAPI mfmediaevent_GetBlob(IMFMediaEvent *iface, REFGUID key, UINT8 *buf,
                UINT32 bufsize, UINT32 *blobsize)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_GetBlob(&This->attributes.IMFAttributes_iface, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI mfmediaevent_GetAllocatedBlob(IMFMediaEvent *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_GetAllocatedBlob(&This->attributes.IMFAttributes_iface, key, buf, size);
}

static HRESULT WINAPI mfmediaevent_GetUnknown(IMFMediaEvent *iface, REFGUID key, REFIID riid, void **ppv)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_GetUnknown(&This->attributes.IMFAttributes_iface, key, riid, ppv);
}

static HRESULT WINAPI mfmediaevent_SetItem(IMFMediaEvent *iface, REFGUID key, REFPROPVARIANT value)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_SetItem(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfmediaevent_DeleteItem(IMFMediaEvent *iface, REFGUID key)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_DeleteItem(&This->attributes.IMFAttributes_iface, key);
}

static HRESULT WINAPI mfmediaevent_DeleteAllItems(IMFMediaEvent *iface)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_DeleteAllItems(&This->attributes.IMFAttributes_iface);
}

static HRESULT WINAPI mfmediaevent_SetUINT32(IMFMediaEvent *iface, REFGUID key, UINT32 value)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_SetUINT32(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfmediaevent_SetUINT64(IMFMediaEvent *iface, REFGUID key, UINT64 value)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_SetUINT64(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfmediaevent_SetDouble(IMFMediaEvent *iface, REFGUID key, double value)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_SetDouble(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfmediaevent_SetGUID(IMFMediaEvent *iface, REFGUID key, REFGUID value)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_SetGUID(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfmediaevent_SetString(IMFMediaEvent *iface, REFGUID key, const WCHAR *value)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_SetString(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfmediaevent_SetBlob(IMFMediaEvent *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_SetBlob(&This->attributes.IMFAttributes_iface, key, buf, size);
}

static HRESULT WINAPI mfmediaevent_SetUnknown(IMFMediaEvent *iface, REFGUID key, IUnknown *unknown)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_SetUnknown(&This->attributes.IMFAttributes_iface, key, unknown);
}

static HRESULT WINAPI mfmediaevent_LockStore(IMFMediaEvent *iface)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_LockStore(&This->attributes.IMFAttributes_iface);
}

static HRESULT WINAPI mfmediaevent_UnlockStore(IMFMediaEvent *iface)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_UnlockStore(&This->attributes.IMFAttributes_iface);
}

static HRESULT WINAPI mfmediaevent_GetCount(IMFMediaEvent *iface, UINT32 *items)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_GetCount(&This->attributes.IMFAttributes_iface, items);
}

static HRESULT WINAPI mfmediaevent_GetItemByIndex(IMFMediaEvent *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_GetItemByIndex(&This->attributes.IMFAttributes_iface, index, key, value);
}

static HRESULT WINAPI mfmediaevent_CopyAllItems(IMFMediaEvent *iface, IMFAttributes *dest)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);

    FIXME("%p, %p\n", This, dest);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfmediaevent_GetType(IMFMediaEvent *iface, MediaEventType *type)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %p\n", This, type);

    *type = This->type;

    return S_OK;
}

static HRESULT WINAPI mfmediaevent_GetExtendedType(IMFMediaEvent *iface, GUID *extended_type)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %p\n", This, extended_type);

    *extended_type = This->extended_type;

    return S_OK;
}

static HRESULT WINAPI mfmediaevent_GetStatus(IMFMediaEvent *iface, HRESULT *status)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %p\n", This, status);

    *status = This->status;

    return S_OK;
}

static HRESULT WINAPI mfmediaevent_GetValue(IMFMediaEvent *iface, PROPVARIANT *value)
{
    mfmediaevent *This = impl_from_IMFMediaEvent(iface);

    PropVariantCopy(value, &This->value);

    return S_OK;
}

static const IMFMediaEventVtbl mfmediaevent_vtbl =
{
    mfmediaevent_QueryInterface,
    mfmediaevent_AddRef,
    mfmediaevent_Release,
    mfmediaevent_GetItem,
    mfmediaevent_GetItemType,
    mfmediaevent_CompareItem,
    mfmediaevent_Compare,
    mfmediaevent_GetUINT32,
    mfmediaevent_GetUINT64,
    mfmediaevent_GetDouble,
    mfmediaevent_GetGUID,
    mfmediaevent_GetStringLength,
    mfmediaevent_GetString,
    mfmediaevent_GetAllocatedString,
    mfmediaevent_GetBlobSize,
    mfmediaevent_GetBlob,
    mfmediaevent_GetAllocatedBlob,
    mfmediaevent_GetUnknown,
    mfmediaevent_SetItem,
    mfmediaevent_DeleteItem,
    mfmediaevent_DeleteAllItems,
    mfmediaevent_SetUINT32,
    mfmediaevent_SetUINT64,
    mfmediaevent_SetDouble,
    mfmediaevent_SetGUID,
    mfmediaevent_SetString,
    mfmediaevent_SetBlob,
    mfmediaevent_SetUnknown,
    mfmediaevent_LockStore,
    mfmediaevent_UnlockStore,
    mfmediaevent_GetCount,
    mfmediaevent_GetItemByIndex,
    mfmediaevent_CopyAllItems,
    mfmediaevent_GetType,
    mfmediaevent_GetExtendedType,
    mfmediaevent_GetStatus,
    mfmediaevent_GetValue,
};

HRESULT WINAPI MFCreateMediaEvent(MediaEventType type, REFGUID extended_type, HRESULT status,
                                  const PROPVARIANT *value, IMFMediaEvent **event)
{
    mfmediaevent *object;

    TRACE("%#x, %s, %08x, %p, %p\n", type, debugstr_guid(extended_type), status, value, event);

    object = HeapAlloc( GetProcessHeap(), 0, sizeof(*object) );
    if(!object)
        return E_OUTOFMEMORY;

    init_attribute_object(&object->attributes, 0);
    object->IMFMediaEvent_iface.lpVtbl = &mfmediaevent_vtbl;

    object->type = type;
    object->extended_type = *extended_type;
    object->status = status;

    PropVariantInit(&object->value);
    if (value)
        PropVariantCopy(&object->value, value);

    *event = &object->IMFMediaEvent_iface;

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
    mfattributes attributes;
    IMFStreamDescriptor IMFStreamDescriptor_iface;
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
    ULONG ref = InterlockedIncrement(&This->attributes.ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI mfdescriptor_Release(IMFStreamDescriptor *iface)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    ULONG ref = InterlockedDecrement(&This->attributes.ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI mfdescriptor_GetItem(IMFStreamDescriptor *iface, REFGUID key, PROPVARIANT *value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_GetItem(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfdescriptor_GetItemType(IMFStreamDescriptor *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_GetItemType(&This->attributes.IMFAttributes_iface, key, type);
}

static HRESULT WINAPI mfdescriptor_CompareItem(IMFStreamDescriptor *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_CompareItem(&This->attributes.IMFAttributes_iface, key, value, result);
}

static HRESULT WINAPI mfdescriptor_Compare(IMFStreamDescriptor *iface, IMFAttributes *theirs, MF_ATTRIBUTES_MATCH_TYPE type,
                BOOL *result)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_Compare(&This->attributes.IMFAttributes_iface, theirs, type, result);
}

static HRESULT WINAPI mfdescriptor_GetUINT32(IMFStreamDescriptor *iface, REFGUID key, UINT32 *value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_GetUINT32(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfdescriptor_GetUINT64(IMFStreamDescriptor *iface, REFGUID key, UINT64 *value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_GetUINT64(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfdescriptor_GetDouble(IMFStreamDescriptor *iface, REFGUID key, double *value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_GetDouble(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfdescriptor_GetGUID(IMFStreamDescriptor *iface, REFGUID key, GUID *value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_GetGUID(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfdescriptor_GetStringLength(IMFStreamDescriptor *iface, REFGUID key, UINT32 *length)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_GetStringLength(&This->attributes.IMFAttributes_iface, key, length);
}

static HRESULT WINAPI mfdescriptor_GetString(IMFStreamDescriptor *iface, REFGUID key, WCHAR *value,
                UINT32 size, UINT32 *length)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_GetString(&This->attributes.IMFAttributes_iface, key, value, size, length);
}

static HRESULT WINAPI mfdescriptor_GetAllocatedString(IMFStreamDescriptor *iface, REFGUID key,
                                      WCHAR **value, UINT32 *length)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_GetAllocatedString(&This->attributes.IMFAttributes_iface, key, value, length);
}

static HRESULT WINAPI mfdescriptor_GetBlobSize(IMFStreamDescriptor *iface, REFGUID key, UINT32 *size)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_GetBlobSize(&This->attributes.IMFAttributes_iface, key, size);
}

static HRESULT WINAPI mfdescriptor_GetBlob(IMFStreamDescriptor *iface, REFGUID key, UINT8 *buf,
                UINT32 bufsize, UINT32 *blobsize)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_GetBlob(&This->attributes.IMFAttributes_iface, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI mfdescriptor_GetAllocatedBlob(IMFStreamDescriptor *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_GetAllocatedBlob(&This->attributes.IMFAttributes_iface, key, buf, size);
}

static HRESULT WINAPI mfdescriptor_GetUnknown(IMFStreamDescriptor *iface, REFGUID key, REFIID riid, void **ppv)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_GetUnknown(&This->attributes.IMFAttributes_iface, key, riid, ppv);
}

static HRESULT WINAPI mfdescriptor_SetItem(IMFStreamDescriptor *iface, REFGUID key, REFPROPVARIANT value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_SetItem(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfdescriptor_DeleteItem(IMFStreamDescriptor *iface, REFGUID key)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_DeleteItem(&This->attributes.IMFAttributes_iface, key);
}

static HRESULT WINAPI mfdescriptor_DeleteAllItems(IMFStreamDescriptor *iface)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_DeleteAllItems(&This->attributes.IMFAttributes_iface);
}

static HRESULT WINAPI mfdescriptor_SetUINT32(IMFStreamDescriptor *iface, REFGUID key, UINT32 value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_SetUINT32(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfdescriptor_SetUINT64(IMFStreamDescriptor *iface, REFGUID key, UINT64 value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_SetUINT64(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfdescriptor_SetDouble(IMFStreamDescriptor *iface, REFGUID key, double value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_SetDouble(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfdescriptor_SetGUID(IMFStreamDescriptor *iface, REFGUID key, REFGUID value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_SetGUID(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfdescriptor_SetString(IMFStreamDescriptor *iface, REFGUID key, const WCHAR *value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_SetString(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfdescriptor_SetBlob(IMFStreamDescriptor *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_SetBlob(&This->attributes.IMFAttributes_iface, key, buf, size);
}

static HRESULT WINAPI mfdescriptor_SetUnknown(IMFStreamDescriptor *iface, REFGUID key, IUnknown *unknown)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_SetUnknown(&This->attributes.IMFAttributes_iface, key, unknown);
}

static HRESULT WINAPI mfdescriptor_LockStore(IMFStreamDescriptor *iface)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_LockStore(&This->attributes.IMFAttributes_iface);
}

static HRESULT WINAPI mfdescriptor_UnlockStore(IMFStreamDescriptor *iface)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_UnlockStore(&This->attributes.IMFAttributes_iface);
}

static HRESULT WINAPI mfdescriptor_GetCount(IMFStreamDescriptor *iface, UINT32 *items)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_GetCount(&This->attributes.IMFAttributes_iface, items);
}

static HRESULT WINAPI mfdescriptor_GetItemByIndex(IMFStreamDescriptor *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    mfdescriptor *This = impl_from_IMFStreamDescriptor(iface);
    return IMFAttributes_GetItemByIndex(&This->attributes.IMFAttributes_iface, index, key, value);
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

    object = heap_alloc(sizeof(*object));
    if(!object)
        return E_OUTOFMEMORY;

    init_attribute_object(&object->attributes, 0);
    object->IMFStreamDescriptor_iface.lpVtbl = &mfdescriptor_vtbl;
    *descriptor = &object->IMFStreamDescriptor_iface;

    return S_OK;
}


typedef struct _mfbuffer
{
    IMFMediaBuffer IMFMediaBuffer_iface;
    LONG ref;

    BYTE *buffer;
    DWORD max_length;
    DWORD current;
} mfbuffer;

static inline mfbuffer *impl_from_IMFMediaBuffer(IMFMediaBuffer *iface)
{
    return CONTAINING_RECORD(iface, mfbuffer, IMFMediaBuffer_iface);
}

static HRESULT WINAPI mfbuffer_QueryInterface(IMFMediaBuffer *iface, REFIID riid, void **out)
{
    mfbuffer *This = impl_from_IMFMediaBuffer(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), out);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IMFMediaBuffer))
    {
        *out = &This->IMFMediaBuffer_iface;
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

static ULONG WINAPI mfbuffer_AddRef(IMFMediaBuffer *iface)
{
    mfbuffer *This = impl_from_IMFMediaBuffer(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI mfbuffer_Release(IMFMediaBuffer *iface)
{
    mfbuffer *This = impl_from_IMFMediaBuffer(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        heap_free(This->buffer);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI mfbuffer_Lock(IMFMediaBuffer *iface, BYTE **buffer, DWORD *max, DWORD *current)
{
    mfbuffer *This = impl_from_IMFMediaBuffer(iface);

    TRACE("%p, %p %p, %p\n", This, buffer, max, current);

    if(!buffer)
        return E_INVALIDARG;

    *buffer = This->buffer;
    if(max)
        *max = This->max_length;
    if(current)
        *current = This->current;

    return S_OK;
}

static HRESULT WINAPI mfbuffer_Unlock(IMFMediaBuffer *iface)
{
    mfbuffer *This = impl_from_IMFMediaBuffer(iface);

    TRACE("%p\n", This);

    return S_OK;
}

static HRESULT WINAPI mfbuffer_GetCurrentLength(IMFMediaBuffer *iface, DWORD *current)
{
    mfbuffer *This = impl_from_IMFMediaBuffer(iface);

    TRACE("%p\n", This);

    if(!current)
        return E_INVALIDARG;

    *current = This->current;

    return S_OK;
}

static HRESULT WINAPI mfbuffer_SetCurrentLength(IMFMediaBuffer *iface, DWORD current)
{
    mfbuffer *This = impl_from_IMFMediaBuffer(iface);

    TRACE("%p, %u\n", This, current);

    if(current > This->max_length)
        return E_INVALIDARG;

    This->current = current;

    return S_OK;
}

static HRESULT WINAPI mfbuffer_GetMaxLength(IMFMediaBuffer *iface, DWORD *max)
{
    mfbuffer *This = impl_from_IMFMediaBuffer(iface);

    TRACE("%p, %p\n", This, max);

    if(!max)
        return E_INVALIDARG;

    *max = This->max_length;

    return S_OK;
}

static const IMFMediaBufferVtbl mfbuffer_vtbl =
{
    mfbuffer_QueryInterface,
    mfbuffer_AddRef,
    mfbuffer_Release,
    mfbuffer_Lock,
    mfbuffer_Unlock,
    mfbuffer_GetCurrentLength,
    mfbuffer_SetCurrentLength,
    mfbuffer_GetMaxLength
};

HRESULT WINAPI MFCreateMemoryBuffer(DWORD max_length, IMFMediaBuffer **buffer)
{
    mfbuffer *object;
    BYTE *bytes;

    TRACE("%u, %p\n", max_length, buffer);

    if(!buffer)
        return E_INVALIDARG;

    object = heap_alloc( sizeof(*object) );
    if(!object)
        return E_OUTOFMEMORY;

    bytes = heap_alloc( max_length );
    if(!bytes)
    {
        heap_free(object);
        return E_OUTOFMEMORY;
    }

    object->ref = 1;
    object->max_length = max_length;
    object->current = 0;
    object->buffer = bytes;
    object->IMFMediaBuffer_iface.lpVtbl = &mfbuffer_vtbl;
    *buffer = &object->IMFMediaBuffer_iface;

    return S_OK;
}

typedef struct _mfsample
{
    mfattributes attributes;
    IMFSample IMFSample_iface;
} mfsample;

static inline mfsample *impl_from_IMFSample(IMFSample *iface)
{
    return CONTAINING_RECORD(iface, mfsample, IMFSample_iface);
}

static HRESULT WINAPI mfsample_QueryInterface(IMFSample *iface, REFIID riid, void **out)
{
    mfsample *This = impl_from_IMFSample(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), out);

    if(IsEqualGUID(riid, &IID_IUnknown)      ||
       IsEqualGUID(riid, &IID_IMFAttributes) ||
       IsEqualGUID(riid, &IID_IMFSample))
    {
        *out = &This->IMFSample_iface;
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

static ULONG WINAPI mfsample_AddRef(IMFSample *iface)
{
    mfsample *This = impl_from_IMFSample(iface);
    ULONG ref = InterlockedIncrement(&This->attributes.ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI mfsample_Release(IMFSample *iface)
{
    mfsample *This = impl_from_IMFSample(iface);
    ULONG ref = InterlockedDecrement(&This->attributes.ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI mfsample_GetItem(IMFSample *iface, REFGUID key, PROPVARIANT *value)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_GetItem(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfsample_GetItemType(IMFSample *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_GetItemType(&This->attributes.IMFAttributes_iface, key, type);
}

static HRESULT WINAPI mfsample_CompareItem(IMFSample *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_CompareItem(&This->attributes.IMFAttributes_iface, key, value, result);
}

static HRESULT WINAPI mfsample_Compare(IMFSample *iface, IMFAttributes *theirs, MF_ATTRIBUTES_MATCH_TYPE type,
                BOOL *result)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_Compare(&This->attributes.IMFAttributes_iface, theirs, type, result);
}

static HRESULT WINAPI mfsample_GetUINT32(IMFSample *iface, REFGUID key, UINT32 *value)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_GetUINT32(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfsample_GetUINT64(IMFSample *iface, REFGUID key, UINT64 *value)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_GetUINT64(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfsample_GetDouble(IMFSample *iface, REFGUID key, double *value)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_GetDouble(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfsample_GetGUID(IMFSample *iface, REFGUID key, GUID *value)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_GetGUID(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfsample_GetStringLength(IMFSample *iface, REFGUID key, UINT32 *length)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_GetStringLength(&This->attributes.IMFAttributes_iface, key, length);
}

static HRESULT WINAPI mfsample_GetString(IMFSample *iface, REFGUID key, WCHAR *value,
                UINT32 size, UINT32 *length)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_GetString(&This->attributes.IMFAttributes_iface, key, value, size, length);
}

static HRESULT WINAPI mfsample_GetAllocatedString(IMFSample *iface, REFGUID key,
                                      WCHAR **value, UINT32 *length)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_GetAllocatedString(&This->attributes.IMFAttributes_iface, key, value, length);
}

static HRESULT WINAPI mfsample_GetBlobSize(IMFSample *iface, REFGUID key, UINT32 *size)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_GetBlobSize(&This->attributes.IMFAttributes_iface, key, size);
}

static HRESULT WINAPI mfsample_GetBlob(IMFSample *iface, REFGUID key, UINT8 *buf,
                UINT32 bufsize, UINT32 *blobsize)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_GetBlob(&This->attributes.IMFAttributes_iface, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI mfsample_GetAllocatedBlob(IMFSample *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_GetAllocatedBlob(&This->attributes.IMFAttributes_iface, key, buf, size);
}

static HRESULT WINAPI mfsample_GetUnknown(IMFSample *iface, REFGUID key, REFIID riid, void **ppv)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_GetUnknown(&This->attributes.IMFAttributes_iface, key, riid, ppv);
}

static HRESULT WINAPI mfsample_SetItem(IMFSample *iface, REFGUID key, REFPROPVARIANT value)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_SetItem(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfsample_DeleteItem(IMFSample *iface, REFGUID key)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_DeleteItem(&This->attributes.IMFAttributes_iface, key);
}

static HRESULT WINAPI mfsample_DeleteAllItems(IMFSample *iface)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_DeleteAllItems(&This->attributes.IMFAttributes_iface);
}

static HRESULT WINAPI mfsample_SetUINT32(IMFSample *iface, REFGUID key, UINT32 value)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_SetUINT32(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfsample_SetUINT64(IMFSample *iface, REFGUID key, UINT64 value)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_SetUINT64(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfsample_SetDouble(IMFSample *iface, REFGUID key, double value)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_SetDouble(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfsample_SetGUID(IMFSample *iface, REFGUID key, REFGUID value)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_SetGUID(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfsample_SetString(IMFSample *iface, REFGUID key, const WCHAR *value)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_SetString(&This->attributes.IMFAttributes_iface, key, value);
}

static HRESULT WINAPI mfsample_SetBlob(IMFSample *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_SetBlob(&This->attributes.IMFAttributes_iface, key, buf, size);
}

static HRESULT WINAPI mfsample_SetUnknown(IMFSample *iface, REFGUID key, IUnknown *unknown)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_SetUnknown(&This->attributes.IMFAttributes_iface, key, unknown);
}

static HRESULT WINAPI mfsample_LockStore(IMFSample *iface)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_LockStore(&This->attributes.IMFAttributes_iface);
}

static HRESULT WINAPI mfsample_UnlockStore(IMFSample *iface)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_UnlockStore(&This->attributes.IMFAttributes_iface);
}

static HRESULT WINAPI mfsample_GetCount(IMFSample *iface, UINT32 *items)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_GetCount(&This->attributes.IMFAttributes_iface, items);
}

static HRESULT WINAPI mfsample_GetItemByIndex(IMFSample *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    mfsample *This = impl_from_IMFSample(iface);
    return IMFAttributes_GetItemByIndex(&This->attributes.IMFAttributes_iface, index, key, value);
}

static HRESULT WINAPI mfsample_CopyAllItems(IMFSample *iface, IMFAttributes *dest)
{
    mfsample *This = impl_from_IMFSample(iface);

    FIXME("%p, %p\n", This, dest);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsample_GetSampleFlags(IMFSample *iface, DWORD *flags)
{
    mfsample *This = impl_from_IMFSample(iface);

    FIXME("%p, %p\n", This, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsample_SetSampleFlags(IMFSample *iface, DWORD flags)
{
    mfsample *This = impl_from_IMFSample(iface);

    FIXME("%p, %x\n", This, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsample_GetSampleTime(IMFSample *iface, LONGLONG *sampletime)
{
    mfsample *This = impl_from_IMFSample(iface);

    FIXME("%p, %p\n", This, sampletime);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsample_SetSampleTime(IMFSample *iface, LONGLONG sampletime)
{
    mfsample *This = impl_from_IMFSample(iface);

    FIXME("%p, %s\n", This, wine_dbgstr_longlong(sampletime));

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsample_GetSampleDuration(IMFSample *iface, LONGLONG *duration)
{
    mfsample *This = impl_from_IMFSample(iface);

    FIXME("%p, %p\n", This, duration);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsample_SetSampleDuration(IMFSample *iface, LONGLONG duration)
{
    mfsample *This = impl_from_IMFSample(iface);

    FIXME("%p, %s\n", This, wine_dbgstr_longlong(duration));

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsample_GetBufferCount(IMFSample *iface, DWORD *count)
{
    mfsample *This = impl_from_IMFSample(iface);

    FIXME("%p, %p\n", This, count);

    if(*count)
        *count = 0;

    return S_OK;
}

static HRESULT WINAPI mfsample_GetBufferByIndex(IMFSample *iface, DWORD index, IMFMediaBuffer **buffer)
{
    mfsample *This = impl_from_IMFSample(iface);

    FIXME("%p, %u, %p\n", This, index, buffer);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsample_ConvertToContiguousBuffer(IMFSample *iface, IMFMediaBuffer **buffer)
{
    mfsample *This = impl_from_IMFSample(iface);

    FIXME("%p, %p\n", This, buffer);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsample_AddBuffer(IMFSample *iface, IMFMediaBuffer *buffer)
{
    mfsample *This = impl_from_IMFSample(iface);

    FIXME("%p, %p\n", This, buffer);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsample_RemoveBufferByIndex(IMFSample *iface, DWORD index)
{
    mfsample *This = impl_from_IMFSample(iface);

    FIXME("%p, %u\n", This, index);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsample_RemoveAllBuffers(IMFSample *iface)
{
    mfsample *This = impl_from_IMFSample(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsample_GetTotalLength(IMFSample *iface, DWORD *length)
{
    mfsample *This = impl_from_IMFSample(iface);

    FIXME("%p, %p\n", This, length);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsample_CopyToBuffer(IMFSample *iface, IMFMediaBuffer *buffer)
{
    mfsample *This = impl_from_IMFSample(iface);

    FIXME("%p, %p\n", This, buffer);

    return E_NOTIMPL;
}

static const IMFSampleVtbl mfsample_vtbl =
{
    mfsample_QueryInterface,
    mfsample_AddRef,
    mfsample_Release,
    mfsample_GetItem,
    mfsample_GetItemType,
    mfsample_CompareItem,
    mfsample_Compare,
    mfsample_GetUINT32,
    mfsample_GetUINT64,
    mfsample_GetDouble,
    mfsample_GetGUID,
    mfsample_GetStringLength,
    mfsample_GetString,
    mfsample_GetAllocatedString,
    mfsample_GetBlobSize,
    mfsample_GetBlob,
    mfsample_GetAllocatedBlob,
    mfsample_GetUnknown,
    mfsample_SetItem,
    mfsample_DeleteItem,
    mfsample_DeleteAllItems,
    mfsample_SetUINT32,
    mfsample_SetUINT64,
    mfsample_SetDouble,
    mfsample_SetGUID,
    mfsample_SetString,
    mfsample_SetBlob,
    mfsample_SetUnknown,
    mfsample_LockStore,
    mfsample_UnlockStore,
    mfsample_GetCount,
    mfsample_GetItemByIndex,
    mfsample_CopyAllItems,
    mfsample_GetSampleFlags,
    mfsample_SetSampleFlags,
    mfsample_GetSampleTime,
    mfsample_SetSampleTime,
    mfsample_GetSampleDuration,
    mfsample_SetSampleDuration,
    mfsample_GetBufferCount,
    mfsample_GetBufferByIndex,
    mfsample_ConvertToContiguousBuffer,
    mfsample_AddBuffer,
    mfsample_RemoveBufferByIndex,
    mfsample_RemoveAllBuffers,
    mfsample_GetTotalLength,
    mfsample_CopyToBuffer
};

HRESULT WINAPI MFCreateSample(IMFSample **sample)
{
    mfsample *object;

    TRACE("%p\n", sample);

    object = heap_alloc(sizeof(*object));
    if(!object)
        return E_OUTOFMEMORY;

    init_attribute_object(&object->attributes, 0);
    object->IMFSample_iface.lpVtbl = &mfsample_vtbl;
    *sample = &object->IMFSample_iface;

    return S_OK;
}
