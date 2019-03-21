/*
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
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"

#include "initguid.h"
#include "ole2.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/list.h"

#include "mfplat_private.h"
#include "mfreadwrite.h"
#include "propvarutil.h"
#include "strsafe.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static LONG platform_lock;

struct system_time_source
{
    IMFPresentationTimeSource IMFPresentationTimeSource_iface;
    IMFClockStateSink IMFClockStateSink_iface;
    LONG refcount;
    MFCLOCK_STATE state;
    CRITICAL_SECTION cs;
};

static struct system_time_source *impl_from_IMFPresentationTimeSource(IMFPresentationTimeSource *iface)
{
    return CONTAINING_RECORD(iface, struct system_time_source, IMFPresentationTimeSource_iface);
}

static struct system_time_source *impl_from_IMFClockStateSink(IMFClockStateSink *iface)
{
    return CONTAINING_RECORD(iface, struct system_time_source, IMFClockStateSink_iface);
}

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

    TRACE("%#x, %#x.\n", version, flags);

    if (version != MF_VERSION_XP && version != MF_VERSION_WIN7)
        return MF_E_BAD_STARTUP_VERSION;

    if (InterlockedIncrement(&platform_lock) == 1)
    {
        init_system_queues();
    }

    return S_OK;
}

/***********************************************************************
 *      MFShutdown (mfplat.@)
 */
HRESULT WINAPI MFShutdown(void)
{
    TRACE("\n");

    if (platform_lock <= 0)
        return S_OK;

    if (InterlockedExchangeAdd(&platform_lock, -1) == 1)
    {
        shutdown_system_queues();
    }

    return S_OK;
}

/***********************************************************************
 *      MFLockPlatform (mfplat.@)
 */
HRESULT WINAPI MFLockPlatform(void)
{
    InterlockedIncrement(&platform_lock);

    return S_OK;
}

/***********************************************************************
 *      MFUnlockPlatform (mfplat.@)
 */
HRESULT WINAPI MFUnlockPlatform(void)
{
    if (InterlockedDecrement(&platform_lock) == 0)
    {
        shutdown_system_queues();
    }

    return S_OK;
}

BOOL is_platform_locked(void)
{
    return platform_lock > 0;
}

/***********************************************************************
 *      MFCopyImage (mfplat.@)
 */
HRESULT WINAPI MFCopyImage(BYTE *dest, LONG deststride, const BYTE *src, LONG srcstride, DWORD width, DWORD lines)
{
    TRACE("(%p, %d, %p, %d, %u, %u)\n", dest, deststride, src, srcstride, width, lines);

    while (lines--)
    {
        memcpy(dest, src, width);
        dest += deststride;
        src += srcstride;
    }

    return S_OK;
}

struct guid_def
{
    const GUID *guid;
    const char *name;
};

static int debug_compare_guid(const void *a, const void *b)
{
    const GUID *guid = a;
    const struct guid_def *guid_def = b;
    return memcmp(guid, guid_def->guid, sizeof(*guid));
}

static const char *debugstr_attr(const GUID *guid)
{
    static const struct guid_def guid_defs[] =
    {
#define X(g) { &(g), #g }
        X(MF_READWRITE_MMCSS_CLASS),
        X(MF_SINK_WRITER_ENCODER_CONFIG),
        X(MF_SOURCE_READER_ENABLE_TRANSCODE_ONLY_TRANSFORMS),
        X(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS),
        X(MF_MT_PIXEL_ASPECT_RATIO),
        X(MF_MT_AVG_BITRATE),
        X(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING),
        X(MF_PD_PMPHOST_CONTEXT),
        X(MF_PD_APP_CONTEXT),
        X(MF_PD_TOTAL_FILE_SIZE),
        X(MF_PD_AUDIO_ENCODING_BITRATE),
        X(MF_PD_VIDEO_ENCODING_BITRATE),
        X(MF_PD_MIME_TYPE),
        X(MF_PD_LAST_MODIFIED_TIME),
        X(MF_PD_PLAYBACK_ELEMENT_ID),
        X(MF_MT_ALL_SAMPLES_INDEPENDENT),
        X(MF_PD_PREFERRED_LANGUAGE),
        X(MF_PD_PLAYBACK_BOUNDARY_TIME),
        X(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING),
        X(MF_MT_FRAME_SIZE),
        X(MF_SINK_WRITER_ASYNC_CALLBACK),
        X(MF_MT_FRAME_RATE_RANGE_MAX),
        X(MF_EVENT_SOURCE_TOPOLOGY_CANCELED),
        X(MF_MT_USER_DATA),
        X(MF_EVENT_STREAM_METADATA_SYSTEMID),
        X(MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN),
        X(MF_READWRITE_DISABLE_CONVERTERS),
        X(MFSampleExtension_Token),
        X(MF_SOURCE_READER_D3D11_BIND_FLAGS),
        X(MF_PD_SAMI_STYLELIST),
        X(MF_SD_LANGUAGE),
        X(MF_SD_PROTECTED),
        X(MF_READWRITE_MMCSS_PRIORITY_AUDIO),
        X(MF_BYTESTREAM_ORIGIN_NAME),
        X(MF_BYTESTREAM_CONTENT_TYPE),
        X(MF_BYTESTREAM_DURATION),
        X(MF_SD_SAMI_LANGUAGE),
        X(MF_EVENT_OUTPUT_NODE),
        X(MF_BYTESTREAM_LAST_MODIFIED_TIME),
        X(MF_MT_FRAME_RATE_RANGE_MIN),
        X(MF_BYTESTREAM_IFO_FILE_URI),
        X(MF_EVENT_TOPOLOGY_STATUS),
        X(MF_BYTESTREAM_DLNA_PROFILE_ID),
        X(MF_MT_MAJOR_TYPE),
        X(MF_EVENT_SOURCE_CHARACTERISTICS),
        X(MF_EVENT_SOURCE_CHARACTERISTICS_OLD),
        X(MF_PD_ADAPTIVE_STREAMING),
        X(MFSampleExtension_Timestamp),
        X(MF_MT_SUBTYPE),
        X(MF_SD_MUTUALLY_EXCLUSIVE),
        X(MF_SD_STREAM_NAME),
        X(MF_EVENT_STREAM_METADATA_CONTENT_KEYIDS),
        X(MF_EVENT_STREAM_METADATA_KEYDATA),
        X(MF_SINK_WRITER_D3D_MANAGER),
        X(MF_SOURCE_READER_D3D_MANAGER),
        X(MF_EVENT_SOURCE_FAKE_START),
        X(MF_EVENT_SOURCE_PROJECTSTART),
        X(MF_EVENT_SOURCE_ACTUAL_START),
        X(MF_SOURCE_READER_ASYNC_CALLBACK),
        X(MF_EVENT_SCRUBSAMPLE_TIME),
        X(MF_MT_INTERLACE_MODE),
        X(MF_SOURCE_READER_MEDIASOURCE_CHARACTERISTICS),
        X(MF_EVENT_MFT_INPUT_STREAM_ID),
        X(MF_READWRITE_MMCSS_PRIORITY),
        X(MF_EVENT_START_PRESENTATION_TIME),
        X(MF_EVENT_SESSIONCAPS),
        X(MF_EVENT_PRESENTATION_TIME_OFFSET),
        X(MF_EVENT_SESSIONCAPS_DELTA),
        X(MF_EVENT_START_PRESENTATION_TIME_AT_OUTPUT),
        X(MFSampleExtension_DecodeTimestamp),
        X(MF_SINK_WRITER_DISABLE_THROTTLING),
        X(MF_READWRITE_D3D_OPTIONAL),
        X(MF_READWRITE_MMCSS_CLASS_AUDIO),
        X(MF_SOURCE_READER_DISABLE_CAMERA_PLUGINS),
        X(MF_PD_AUDIO_ISVARIABLEBITRATE),
        X(MF_MT_FRAME_RATE),
        X(MF_SOURCE_READER_MEDIASOURCE_CONFIG),
        X(MF_EVENT_MFT_CONTEXT),
        X(MF_EVENT_DO_THINNING),
        X(MF_SOURCE_READER_DISABLE_DXVA),
#undef X
    };
    struct guid_def *ret = NULL;

    if (guid)
        ret = bsearch(guid, guid_defs, ARRAY_SIZE(guid_defs), sizeof(*guid_defs), debug_compare_guid);

    return ret ? wine_dbg_sprintf("%s", ret->name) : wine_dbgstr_guid(guid);
}

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
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    ULONG refcount = InterlockedDecrement(&attributes->ref);

    TRACE("%p, refcount %d.\n", iface, refcount);

    if (!refcount)
    {
        clear_attributes_object(attributes);
        heap_free(attributes);
    }

    return refcount;
}

static struct attribute *attributes_find_item(struct attributes *attributes, REFGUID key, size_t *index)
{
    size_t i;

    for (i = 0; i < attributes->count; ++i)
    {
        if (IsEqualGUID(key, &attributes->attributes[i].key))
        {
            if (index)
                *index = i;
            return &attributes->attributes[i];
        }
    }

    return NULL;
}

static HRESULT attributes_get_item(struct attributes *attributes, const GUID *key, PROPVARIANT *value)
{
    struct attribute *attribute;
    HRESULT hr;

    EnterCriticalSection(&attributes->cs);

    attribute = attributes_find_item(attributes, key, NULL);
    if (attribute)
    {
        if (attribute->value.vt == value->vt && !(value->vt == VT_UNKNOWN && !attribute->value.u.punkVal))
            hr = PropVariantCopy(value, &attribute->value);
        else
            hr = MF_E_INVALIDTYPE;
    }
    else
        hr = MF_E_ATTRIBUTENOTFOUND;

    LeaveCriticalSection(&attributes->cs);

    return hr;
}

static HRESULT WINAPI mfattributes_GetItem(IMFAttributes *iface, REFGUID key, PROPVARIANT *value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    struct attribute *attribute;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    EnterCriticalSection(&attributes->cs);

    if ((attribute = attributes_find_item(attributes, key, NULL)))
        hr = PropVariantCopy(value, &attribute->value);
    else
        hr = MF_E_ATTRIBUTENOTFOUND;

    LeaveCriticalSection(&attributes->cs);

    return hr;
}

static HRESULT WINAPI mfattributes_GetItemType(IMFAttributes *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    struct attribute *attribute;
    HRESULT hr = S_OK;

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), type);

    EnterCriticalSection(&attributes->cs);

    if ((attribute = attributes_find_item(attributes, key, NULL)))
    {
        *type = attribute->value.vt;
    }
    else
        hr = MF_E_ATTRIBUTENOTFOUND;

    LeaveCriticalSection(&attributes->cs);

    return hr;
}

static HRESULT WINAPI mfattributes_CompareItem(IMFAttributes *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    struct attribute *attribute;

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), value, result);

    *result = FALSE;

    EnterCriticalSection(&attributes->cs);

    if ((attribute = attributes_find_item(attributes, key, NULL)))
    {
        *result = attribute->value.vt == value->vt &&
                !PropVariantCompareEx(&attribute->value, value, PVCU_DEFAULT, PVCF_DEFAULT);
    }

    LeaveCriticalSection(&attributes->cs);

    return S_OK;
}

static HRESULT WINAPI mfattributes_Compare(IMFAttributes *iface, IMFAttributes *theirs,
        MF_ATTRIBUTES_MATCH_TYPE match_type, BOOL *ret)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    IMFAttributes *smaller, *other;
    MF_ATTRIBUTE_TYPE type;
    HRESULT hr = S_OK;
    UINT32 count;
    BOOL result;
    size_t i;

    TRACE("%p, %p, %d, %p.\n", iface, theirs, match_type, ret);

    if (FAILED(hr = IMFAttributes_GetCount(theirs, &count)))
        return hr;

    EnterCriticalSection(&attributes->cs);

    result = TRUE;

    switch (match_type)
    {
        case MF_ATTRIBUTES_MATCH_OUR_ITEMS:
            for (i = 0; i < attributes->count; ++i)
            {
                if (FAILED(hr = IMFAttributes_CompareItem(theirs, &attributes->attributes[i].key,
                        &attributes->attributes[i].value, &result)))
                    break;
                if (!result)
                    break;
            }
            break;
        case MF_ATTRIBUTES_MATCH_THEIR_ITEMS:
            hr = IMFAttributes_Compare(theirs, iface, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result);
            break;
        case MF_ATTRIBUTES_MATCH_ALL_ITEMS:
            if (count != attributes->count)
            {
                result = FALSE;
                break;
            }
            for (i = 0; i < count; ++i)
            {
                if (FAILED(hr = IMFAttributes_CompareItem(theirs, &attributes->attributes[i].key,
                        &attributes->attributes[i].value, &result)))
                    break;
                if (!result)
                    break;
            }
            break;
        case MF_ATTRIBUTES_MATCH_INTERSECTION:
            for (i = 0; i < attributes->count; ++i)
            {
                if (FAILED(IMFAttributes_GetItemType(theirs, &attributes->attributes[i].key, &type)))
                    continue;

                if (FAILED(hr = IMFAttributes_CompareItem(theirs, &attributes->attributes[i].key,
                        &attributes->attributes[i].value, &result)))
                    break;

                if (!result)
                    break;
            }
            break;
        case MF_ATTRIBUTES_MATCH_SMALLER:
            smaller = attributes->count > count ? theirs : iface;
            other = attributes->count > count ? iface : theirs;
            hr = IMFAttributes_Compare(smaller, other, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result);
            break;
        default:
            WARN("Unknown match type %d.\n", match_type);
            hr = E_INVALIDARG;
    }

    LeaveCriticalSection(&attributes->cs);

    if (SUCCEEDED(hr))
        *ret = result;

    return hr;
}

static HRESULT WINAPI mfattributes_GetUINT32(IMFAttributes *iface, REFGUID key, UINT32 *value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    PROPVARIANT attrval;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    PropVariantInit(&attrval);
    attrval.vt = VT_UI4;
    hr = attributes_get_item(attributes, key, &attrval);
    if (SUCCEEDED(hr))
        *value = attrval.u.ulVal;

    return hr;
}

static HRESULT WINAPI mfattributes_GetUINT64(IMFAttributes *iface, REFGUID key, UINT64 *value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    PROPVARIANT attrval;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    PropVariantInit(&attrval);
    attrval.vt = VT_UI8;
    hr = attributes_get_item(attributes, key, &attrval);
    if (SUCCEEDED(hr))
        *value = attrval.u.uhVal.QuadPart;

    return hr;
}

static HRESULT WINAPI mfattributes_GetDouble(IMFAttributes *iface, REFGUID key, double *value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    PROPVARIANT attrval;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    PropVariantInit(&attrval);
    attrval.vt = VT_R8;
    hr = attributes_get_item(attributes, key, &attrval);
    if (SUCCEEDED(hr))
        *value = attrval.u.dblVal;

    return hr;
}

static HRESULT WINAPI mfattributes_GetGUID(IMFAttributes *iface, REFGUID key, GUID *value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    PROPVARIANT attrval;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    PropVariantInit(&attrval);
    attrval.vt = VT_CLSID;
    hr = attributes_get_item(attributes, key, &attrval);
    if (SUCCEEDED(hr))
        *value = *attrval.u.puuid;

    return hr;
}

static HRESULT WINAPI mfattributes_GetStringLength(IMFAttributes *iface, REFGUID key, UINT32 *length)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    struct attribute *attribute;
    HRESULT hr = S_OK;

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), length);

    EnterCriticalSection(&attributes->cs);

    attribute = attributes_find_item(attributes, key, NULL);
    if (attribute)
    {
        if (attribute->value.vt == MF_ATTRIBUTE_STRING)
            *length = strlenW(attribute->value.u.pwszVal);
        else
            hr = MF_E_INVALIDTYPE;
    }
    else
        hr = MF_E_ATTRIBUTENOTFOUND;

    LeaveCriticalSection(&attributes->cs);

    return hr;
}

static HRESULT WINAPI mfattributes_GetString(IMFAttributes *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    struct attribute *attribute;
    HRESULT hr = S_OK;

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_attr(key), value, size, length);

    EnterCriticalSection(&attributes->cs);

    attribute = attributes_find_item(attributes, key, NULL);
    if (attribute)
    {
        if (attribute->value.vt == MF_ATTRIBUTE_STRING)
        {
            int len = strlenW(attribute->value.u.pwszVal);

            if (length)
                *length = len;

            if (size <= len)
                return STRSAFE_E_INSUFFICIENT_BUFFER;

            memcpy(value, attribute->value.u.pwszVal, (len + 1) * sizeof(WCHAR));
        }
        else
            hr = MF_E_INVALIDTYPE;
    }
    else
        hr = MF_E_ATTRIBUTENOTFOUND;

    LeaveCriticalSection(&attributes->cs);

    return hr;
}

static HRESULT WINAPI mfattributes_GetAllocatedString(IMFAttributes *iface, REFGUID key, WCHAR **value, UINT32 *length)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    PROPVARIANT attrval;
    HRESULT hr;

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), value, length);

    PropVariantInit(&attrval);
    attrval.vt = VT_LPWSTR;
    hr = attributes_get_item(attributes, key, &attrval);
    if (SUCCEEDED(hr))
    {
        *value = attrval.u.pwszVal;
        *length = lstrlenW(*value);
    }

    return hr;
}

static HRESULT WINAPI mfattributes_GetBlobSize(IMFAttributes *iface, REFGUID key, UINT32 *size)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    struct attribute *attribute;
    HRESULT hr = S_OK;

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), size);

    EnterCriticalSection(&attributes->cs);

    attribute = attributes_find_item(attributes, key, NULL);
    if (attribute)
    {
        if (attribute->value.vt == MF_ATTRIBUTE_BLOB)
            *size = attribute->value.u.caub.cElems;
        else
            hr = MF_E_INVALIDTYPE;
    }
    else
        hr = MF_E_ATTRIBUTENOTFOUND;

    LeaveCriticalSection(&attributes->cs);

    return hr;
}

static HRESULT WINAPI mfattributes_GetBlob(IMFAttributes *iface, REFGUID key, UINT8 *buf,
                UINT32 bufsize, UINT32 *blobsize)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    struct attribute *attribute;
    HRESULT hr;

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_attr(key), buf, bufsize, blobsize);

    EnterCriticalSection(&attributes->cs);

    attribute = attributes_find_item(attributes, key, NULL);
    if (attribute)
    {
        if (attribute->value.vt == MF_ATTRIBUTE_BLOB)
        {
            UINT32 size = attribute->value.u.caub.cElems;

            if (bufsize >= size)
                hr = PropVariantToBuffer(&attribute->value, buf, size);
            else
                hr = E_NOT_SUFFICIENT_BUFFER;

            if (blobsize)
                *blobsize = size;
        }
        else
            hr = MF_E_INVALIDTYPE;
    }
    else
        hr = MF_E_ATTRIBUTENOTFOUND;

    LeaveCriticalSection(&attributes->cs);

    return hr;
}

static HRESULT WINAPI mfattributes_GetAllocatedBlob(IMFAttributes *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    PROPVARIANT attrval;
    HRESULT hr;

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), buf, size);

    attrval.vt = VT_VECTOR | VT_UI1;
    hr = attributes_get_item(attributes, key, &attrval);
    if (SUCCEEDED(hr))
    {
        *buf = attrval.u.caub.pElems;
        *size = attrval.u.caub.cElems;
    }

    return hr;
}

static HRESULT WINAPI mfattributes_GetUnknown(IMFAttributes *iface, REFGUID key, REFIID riid, void **ppv)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    PROPVARIANT attrval;
    HRESULT hr;

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_guid(riid), ppv);

    PropVariantInit(&attrval);
    attrval.vt = VT_UNKNOWN;
    hr = attributes_get_item(attributes, key, &attrval);
    if (SUCCEEDED(hr))
        hr = IUnknown_QueryInterface(attrval.u.punkVal, riid, ppv);
    PropVariantClear(&attrval);
    return hr;
}

static HRESULT attributes_set_item(struct attributes *attributes, REFGUID key, REFPROPVARIANT value)
{
    struct attribute *attribute;

    EnterCriticalSection(&attributes->cs);

    attribute = attributes_find_item(attributes, key, NULL);
    if (!attribute)
    {
        if (!mf_array_reserve((void **)&attributes->attributes, &attributes->capacity, attributes->count + 1,
                sizeof(*attributes->attributes)))
        {
            LeaveCriticalSection(&attributes->cs);
            return E_OUTOFMEMORY;
        }
        attributes->attributes[attributes->count].key = *key;
        attribute = &attributes->attributes[attributes->count++];
    }
    else
        PropVariantClear(&attribute->value);

    PropVariantCopy(&attribute->value, value);

    LeaveCriticalSection(&attributes->cs);

    return S_OK;
}

static HRESULT WINAPI mfattributes_SetItem(IMFAttributes *iface, REFGUID key, REFPROPVARIANT value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    PROPVARIANT empty;

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    switch (value->vt)
    {
        case MF_ATTRIBUTE_UINT32:
        case MF_ATTRIBUTE_UINT64:
        case MF_ATTRIBUTE_DOUBLE:
        case MF_ATTRIBUTE_GUID:
        case MF_ATTRIBUTE_STRING:
        case MF_ATTRIBUTE_BLOB:
        case MF_ATTRIBUTE_IUNKNOWN:
            return attributes_set_item(attributes, key, value);
        default:
            PropVariantInit(&empty);
            attributes_set_item(attributes, key, &empty);
            return MF_E_INVALIDTYPE;
    }
}

static HRESULT WINAPI mfattributes_DeleteItem(IMFAttributes *iface, REFGUID key)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    struct attribute *attribute;
    size_t index = 0;

    TRACE("%p, %s.\n", iface, debugstr_attr(key));

    EnterCriticalSection(&attributes->cs);

    if ((attribute = attributes_find_item(attributes, key, &index)))
    {
        size_t count;

        PropVariantClear(&attribute->value);

        attributes->count--;
        count = attributes->count - index;
        if (count)
            memmove(&attributes->attributes[index], &attributes->attributes[index + 1], count * sizeof(*attributes->attributes));
    }

    LeaveCriticalSection(&attributes->cs);

    return S_OK;
}

static HRESULT WINAPI mfattributes_DeleteAllItems(IMFAttributes *iface)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p.\n", iface);

    EnterCriticalSection(&attributes->cs);

    while (attributes->count)
    {
        PropVariantClear(&attributes->attributes[--attributes->count].value);
    }
    heap_free(attributes->attributes);
    attributes->attributes = NULL;
    attributes->capacity = 0;

    LeaveCriticalSection(&attributes->cs);

    return S_OK;
}

static HRESULT WINAPI mfattributes_SetUINT32(IMFAttributes *iface, REFGUID key, UINT32 value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    PROPVARIANT attrval;

    TRACE("%p, %s, %d.\n", iface, debugstr_attr(key), value);

    attrval.vt = VT_UI4;
    attrval.u.ulVal = value;
    return attributes_set_item(attributes, key, &attrval);
}

static HRESULT WINAPI mfattributes_SetUINT64(IMFAttributes *iface, REFGUID key, UINT64 value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    PROPVARIANT attrval;

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), wine_dbgstr_longlong(value));

    attrval.vt = VT_UI8;
    attrval.u.uhVal.QuadPart = value;
    return attributes_set_item(attributes, key, &attrval);
}

static HRESULT WINAPI mfattributes_SetDouble(IMFAttributes *iface, REFGUID key, double value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    PROPVARIANT attrval;

    TRACE("%p, %s, %f.\n", iface, debugstr_attr(key), value);

    attrval.vt = VT_R8;
    attrval.u.dblVal = value;
    return attributes_set_item(attributes, key, &attrval);
}

static HRESULT WINAPI mfattributes_SetGUID(IMFAttributes *iface, REFGUID key, REFGUID value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    PROPVARIANT attrval;

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_guid(value));

    InitPropVariantFromCLSID(value, &attrval);
    return attributes_set_item(attributes, key, &attrval);
}

static HRESULT WINAPI mfattributes_SetString(IMFAttributes *iface, REFGUID key, const WCHAR *value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    PROPVARIANT attrval;

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_w(value));

    attrval.vt = VT_LPWSTR;
    attrval.u.pwszVal = (WCHAR *)value;
    return attributes_set_item(attributes, key, &attrval);
}

static HRESULT WINAPI mfattributes_SetBlob(IMFAttributes *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    PROPVARIANT attrval;

    TRACE("%p, %s, %p, %u.\n", iface, debugstr_attr(key), buf, size);

    attrval.vt = VT_VECTOR | VT_UI1;
    attrval.u.caub.cElems = size;
    attrval.u.caub.pElems = (UINT8 *)buf;
    return attributes_set_item(attributes, key, &attrval);
}

static HRESULT WINAPI mfattributes_SetUnknown(IMFAttributes *iface, REFGUID key, IUnknown *unknown)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    PROPVARIANT attrval;

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), unknown);

    attrval.vt = VT_UNKNOWN;
    attrval.u.punkVal = unknown;
    return attributes_set_item(attributes, key, &attrval);
}

static HRESULT WINAPI mfattributes_LockStore(IMFAttributes *iface)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p.\n", iface);

    EnterCriticalSection(&attributes->cs);

    return S_OK;
}

static HRESULT WINAPI mfattributes_UnlockStore(IMFAttributes *iface)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p.\n", iface);

    LeaveCriticalSection(&attributes->cs);

    return S_OK;
}

static HRESULT WINAPI mfattributes_GetCount(IMFAttributes *iface, UINT32 *items)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %p.\n", iface, items);

    EnterCriticalSection(&attributes->cs);
    *items = attributes->count;
    LeaveCriticalSection(&attributes->cs);

    return S_OK;
}

static HRESULT WINAPI mfattributes_GetItemByIndex(IMFAttributes *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    EnterCriticalSection(&attributes->cs);

    if (index < attributes->count)
    {
        *key = attributes->attributes[index].key;
        if (value)
            PropVariantCopy(value, &attributes->attributes[index].value);
    }
    else
        hr = E_INVALIDARG;

    LeaveCriticalSection(&attributes->cs);

    return hr;
}

static HRESULT WINAPI mfattributes_CopyAllItems(IMFAttributes *iface, IMFAttributes *dest)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    HRESULT hr = S_OK;
    size_t i;

    TRACE("%p, %p.\n", iface, dest);

    EnterCriticalSection(&attributes->cs);

    IMFAttributes_LockStore(dest);

    IMFAttributes_DeleteAllItems(dest);

    for (i = 0; i < attributes->count; ++i)
    {
        hr = IMFAttributes_SetItem(dest, &attributes->attributes[i].key, &attributes->attributes[i].value);
        if (FAILED(hr))
            break;
    }

    IMFAttributes_UnlockStore(dest);

    LeaveCriticalSection(&attributes->cs);

    return hr;
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

HRESULT init_attributes_object(struct attributes *object, UINT32 size)
{
    object->IMFAttributes_iface.lpVtbl = &mfattributes_vtbl;
    object->ref = 1;
    InitializeCriticalSection(&object->cs);

    object->attributes = NULL;
    object->count = 0;
    object->capacity = 0;
    if (!mf_array_reserve((void **)&object->attributes, &object->capacity, size,
                          sizeof(*object->attributes)))
    {
        DeleteCriticalSection(&object->cs);
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

void clear_attributes_object(struct attributes *object)
{
    size_t i;

    for (i = 0; i < object->count; i++)
        PropVariantClear(&object->attributes[i].value);
    heap_free(object->attributes);

    DeleteCriticalSection(&object->cs);
}

/***********************************************************************
 *      MFCreateAttributes (mfplat.@)
 */
HRESULT WINAPI MFCreateAttributes(IMFAttributes **attributes, UINT32 size)
{
    struct attributes *object;
    HRESULT hr;

    TRACE("%p, %d\n", attributes, size);

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    if (FAILED(hr = init_attributes_object(object, size)))
    {
        heap_free(object);
        return hr;
    }
    *attributes = &object->IMFAttributes_iface;

    return S_OK;
}

#define ATTRIBUTES_STORE_MAGIC 0x494d4641 /* IMFA */

struct attributes_store_header
{
    DWORD magic;
    UINT32 count;
};

struct attributes_store_item
{
    GUID key;
    QWORD type;
    union
    {
        double f;
        UINT32 i32;
        UINT64 i64;
        struct
        {
            DWORD size;
            DWORD offset;
        } subheader;
    } u;
};

/***********************************************************************
 *      MFGetAttributesAsBlobSize (mfplat.@)
 */
HRESULT WINAPI MFGetAttributesAsBlobSize(IMFAttributes *attributes, UINT32 *size)
{
    unsigned int i, count, length;
    HRESULT hr;
    GUID key;

    TRACE("%p, %p.\n", attributes, size);

    IMFAttributes_LockStore(attributes);

    hr = IMFAttributes_GetCount(attributes, &count);

    *size = sizeof(struct attributes_store_header);

    for (i = 0; i < count; ++i)
    {
        MF_ATTRIBUTE_TYPE type;

        hr = IMFAttributes_GetItemByIndex(attributes, i, &key, NULL);
        if (FAILED(hr))
            break;

        *size += sizeof(struct attributes_store_item);

        IMFAttributes_GetItemType(attributes, &key, &type);

        switch (type)
        {
            case MF_ATTRIBUTE_GUID:
                *size += sizeof(GUID);
                break;
            case MF_ATTRIBUTE_STRING:
                IMFAttributes_GetStringLength(attributes, &key, &length);
                *size += (length + 1) * sizeof(WCHAR);
                break;
            case MF_ATTRIBUTE_BLOB:
                IMFAttributes_GetBlobSize(attributes, &key, &length);
                *size += length;
                break;
            case MF_ATTRIBUTE_UINT32:
            case MF_ATTRIBUTE_UINT64:
            case MF_ATTRIBUTE_DOUBLE:
            case MF_ATTRIBUTE_IUNKNOWN:
            default:
                ;
        }
    }

    IMFAttributes_UnlockStore(attributes);

    return hr;
}

struct attr_serialize_context
{
    UINT8 *buffer;
    UINT8 *ptr;
    UINT32 size;
};

static void attributes_serialize_write(struct attr_serialize_context *context, const void *value, unsigned int size)
{
    memcpy(context->ptr, value, size);
    context->ptr += size;
}

static BOOL attributes_serialize_write_item(struct attr_serialize_context *context, struct attributes_store_item *item,
        const void *value)
{
    switch (item->type)
    {
        case MF_ATTRIBUTE_UINT32:
        case MF_ATTRIBUTE_UINT64:
        case MF_ATTRIBUTE_DOUBLE:
            attributes_serialize_write(context, item, sizeof(*item));
            break;
        case MF_ATTRIBUTE_GUID:
        case MF_ATTRIBUTE_STRING:
        case MF_ATTRIBUTE_BLOB:
            item->u.subheader.offset = context->size - item->u.subheader.size;
            attributes_serialize_write(context, item, sizeof(*item));
            memcpy(context->buffer + item->u.subheader.offset, value, item->u.subheader.size);
            context->size -= item->u.subheader.size;
            break;
        default:
            return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *      MFGetAttributesAsBlob (mfplat.@)
 */
HRESULT WINAPI MFGetAttributesAsBlob(IMFAttributes *attributes, UINT8 *buffer, UINT size)
{
    struct attributes_store_header header;
    struct attr_serialize_context context;
    unsigned int required_size, i;
    PROPVARIANT value;
    UINT32 count;
    HRESULT hr;

    TRACE("%p, %p, %u.\n", attributes, buffer, size);

    if (FAILED(hr = MFGetAttributesAsBlobSize(attributes, &required_size)))
        return hr;

    if (required_size > size)
        return MF_E_BUFFERTOOSMALL;

    context.buffer = buffer;
    context.ptr = buffer;
    context.size = required_size;

    IMFAttributes_LockStore(attributes);

    header.magic = ATTRIBUTES_STORE_MAGIC;
    header.count = 0; /* Will be updated later */
    IMFAttributes_GetCount(attributes, &count);

    attributes_serialize_write(&context, &header, sizeof(header));

    for (i = 0; i < count; ++i)
    {
        struct attributes_store_item item;
        const void *data = NULL;

        hr = IMFAttributes_GetItemByIndex(attributes, i, &item.key, &value);
        if (FAILED(hr))
            break;

        item.type = value.vt;

        switch (value.vt)
        {
            case MF_ATTRIBUTE_UINT32:
            case MF_ATTRIBUTE_UINT64:
                item.u.i64 = value.u.uhVal.QuadPart;
                break;
            case MF_ATTRIBUTE_DOUBLE:
                item.u.f = value.u.dblVal;
                break;
            case MF_ATTRIBUTE_GUID:
                item.u.subheader.size = sizeof(*value.u.puuid);
                data = value.u.puuid;
                break;
            case MF_ATTRIBUTE_STRING:
                item.u.subheader.size = (strlenW(value.u.pwszVal) + 1) * sizeof(WCHAR);
                data = value.u.pwszVal;
                break;
            case MF_ATTRIBUTE_BLOB:
                item.u.subheader.size = value.u.caub.cElems;
                data = value.u.caub.pElems;
                break;
            case MF_ATTRIBUTE_IUNKNOWN:
                break;
            default:
                WARN("Unknown attribute type %#x.\n", value.vt);
        }

        if (attributes_serialize_write_item(&context, &item, data))
            header.count++;

        PropVariantClear(&value);
    }

    memcpy(context.buffer, &header, sizeof(header));

    IMFAttributes_UnlockStore(attributes);

    return S_OK;
}

static HRESULT attributes_deserialize_read(struct attr_serialize_context *context, void *value, unsigned int size)
{
    if (context->size < (context->ptr - context->buffer) + size)
        return E_INVALIDARG;

    memcpy(value, context->ptr, size);
    context->ptr += size;

    return S_OK;
}

/***********************************************************************
 *      MFInitAttributesFromBlob (mfplat.@)
 */
HRESULT WINAPI MFInitAttributesFromBlob(IMFAttributes *dest, const UINT8 *buffer, UINT size)
{
    struct attr_serialize_context context;
    struct attributes_store_header header;
    struct attributes_store_item item;
    IMFAttributes *attributes;
    unsigned int i;
    HRESULT hr;

    TRACE("%p, %p, %u.\n", dest, buffer, size);

    context.buffer = (UINT8 *)buffer;
    context.ptr = (UINT8 *)buffer;
    context.size = size;

    /* Validate buffer structure. */
    if (FAILED(hr = attributes_deserialize_read(&context, &header, sizeof(header))))
        return hr;

    if (header.magic != ATTRIBUTES_STORE_MAGIC)
        return E_UNEXPECTED;

    if (FAILED(hr = MFCreateAttributes(&attributes, header.count)))
        return hr;

    for (i = 0; i < header.count; ++i)
    {
        if (FAILED(hr = attributes_deserialize_read(&context, &item, sizeof(item))))
            break;

        hr = E_UNEXPECTED;

        switch (item.type)
        {
            case MF_ATTRIBUTE_UINT32:
                hr = IMFAttributes_SetUINT32(attributes, &item.key, item.u.i32);
                break;
            case MF_ATTRIBUTE_UINT64:
                hr = IMFAttributes_SetUINT64(attributes, &item.key, item.u.i64);
                break;
            case MF_ATTRIBUTE_DOUBLE:
                hr = IMFAttributes_SetDouble(attributes, &item.key, item.u.f);
                break;
            case MF_ATTRIBUTE_GUID:
                if (item.u.subheader.size == sizeof(GUID) &&
                        item.u.subheader.offset + item.u.subheader.size <= context.size)
                {
                    hr = IMFAttributes_SetGUID(attributes, &item.key,
                            (const GUID *)(context.buffer + item.u.subheader.offset));
                }
                break;
            case MF_ATTRIBUTE_STRING:
                if (item.u.subheader.size >= sizeof(WCHAR) &&
                        item.u.subheader.offset + item.u.subheader.size <= context.size)
                {
                    hr = IMFAttributes_SetString(attributes, &item.key,
                            (const WCHAR *)(context.buffer + item.u.subheader.offset));
                }
                break;
            case MF_ATTRIBUTE_BLOB:
                if (item.u.subheader.size > 0 && item.u.subheader.offset + item.u.subheader.size <= context.size)
                {
                    hr = IMFAttributes_SetBlob(attributes, &item.key, context.buffer + item.u.subheader.offset,
                            item.u.subheader.size);
                }
                break;
            default:
                ;
        }

        if (FAILED(hr))
            break;
    }

    if (SUCCEEDED(hr))
    {
        IMFAttributes_DeleteAllItems(dest);
        hr = IMFAttributes_CopyAllItems(attributes, dest);
    }

    IMFAttributes_Release(attributes);

    return hr;
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
        clear_attributes_object(&This->attributes);
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
    HRESULT hr;

    TRACE("(%p, %p): stub\n", stream, bytestream);

    object = heap_alloc( sizeof(*object) );
    if(!object)
        return E_OUTOFMEMORY;

    if (FAILED(hr = init_attributes_object(&object->attributes, 0)))
    {
        heap_free(object);
        return hr;
    }
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
    HRESULT hr;

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

    if (FAILED(hr = init_attributes_object(&object->attributes, 0)))
    {
        heap_free(object);
        return hr;
    }
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
        clear_attributes_object(&This->attributes);
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

    init_attributes_object(&object->attributes, 0);
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

enum resolved_object_origin
{
    OBJECT_FROM_BYTESTREAM,
    OBJECT_FROM_URL,
};

struct resolver_queued_result
{
    struct list entry;
    IUnknown *object;
    MF_OBJECT_TYPE obj_type;
    HRESULT hr;
    enum resolved_object_origin origin;
};

struct resolver_cancel_object
{
    IUnknown IUnknown_iface;
    LONG refcount;
    union
    {
        IUnknown *handler;
        IMFByteStreamHandler *stream_handler;
        IMFSchemeHandler *scheme_handler;
    } u;
    IUnknown *cancel_cookie;
    enum resolved_object_origin origin;
};

typedef struct source_resolver
{
    IMFSourceResolver IMFSourceResolver_iface;
    LONG refcount;
    IMFAsyncCallback stream_callback;
    IMFAsyncCallback url_callback;
    CRITICAL_SECTION cs;
    struct list pending;
} mfsourceresolver;

static struct source_resolver *impl_from_IMFSourceResolver(IMFSourceResolver *iface)
{
    return CONTAINING_RECORD(iface, struct source_resolver, IMFSourceResolver_iface);
}

static struct source_resolver *impl_from_stream_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct source_resolver, stream_callback);
}

static struct source_resolver *impl_from_url_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct source_resolver, url_callback);
}

static HRESULT resolver_handler_end_create(struct source_resolver *resolver, enum resolved_object_origin origin,
        IMFAsyncResult *result)
{
    IMFAsyncResult *inner_result = (IMFAsyncResult *)IMFAsyncResult_GetStateNoAddRef(result);
    struct resolver_queued_result *queued_result;
    union
    {
        IUnknown *handler;
        IMFByteStreamHandler *stream_handler;
        IMFSchemeHandler *scheme_handler;
    } handler;

    queued_result = heap_alloc(sizeof(*queued_result));

    IMFAsyncResult_GetObject(inner_result, &handler.handler);

    switch (origin)
    {
        case OBJECT_FROM_BYTESTREAM:
            queued_result->hr = IMFByteStreamHandler_EndCreateObject(handler.stream_handler, result,
                    &queued_result->obj_type, &queued_result->object);
            break;
        case OBJECT_FROM_URL:
            queued_result->hr = IMFSchemeHandler_EndCreateObject(handler.scheme_handler, result,
                    &queued_result->obj_type, &queued_result->object);
            break;
        default:
            queued_result->hr = E_FAIL;
    }

    IUnknown_Release(handler.handler);

    if (SUCCEEDED(queued_result->hr))
    {
        MFASYNCRESULT *data = (MFASYNCRESULT *)inner_result;

        /* Push resolved object type and created object, so we don't have to guess on End*() call. */
        EnterCriticalSection(&resolver->cs);
        list_add_tail(&resolver->pending, &queued_result->entry);
        LeaveCriticalSection(&resolver->cs);

        if (data->hEvent)
            SetEvent(data->hEvent);
        else
        {
            IUnknown *caller_state = IMFAsyncResult_GetStateNoAddRef(inner_result);
            IMFAsyncResult *caller_result;

            if (SUCCEEDED(MFCreateAsyncResult(queued_result->object, data->pCallback, caller_state, &caller_result)))
            {
                MFInvokeCallback(caller_result);
                IMFAsyncResult_Release(caller_result);
            }
        }
    }
    else
        heap_free(queued_result);

    return S_OK;
}

static struct resolver_cancel_object *impl_cancel_obj_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct resolver_cancel_object, IUnknown_iface);
}

static HRESULT WINAPI resolver_cancel_object_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI resolver_cancel_object_AddRef(IUnknown *iface)
{
    struct resolver_cancel_object *object = impl_cancel_obj_from_IUnknown(iface);
    return InterlockedIncrement(&object->refcount);
}

static ULONG WINAPI resolver_cancel_object_Release(IUnknown *iface)
{
    struct resolver_cancel_object *object = impl_cancel_obj_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&object->refcount);

    if (!refcount)
    {
        if (object->cancel_cookie)
            IUnknown_Release(object->cancel_cookie);
        IUnknown_Release(object->u.handler);
        heap_free(object);
    }

    return refcount;
}

static const IUnknownVtbl resolver_cancel_object_vtbl =
{
    resolver_cancel_object_QueryInterface,
    resolver_cancel_object_AddRef,
    resolver_cancel_object_Release,
};

static struct resolver_cancel_object *unsafe_impl_cancel_obj_from_IUnknown(IUnknown *iface)
{
    if (!iface)
        return NULL;

    return (iface->lpVtbl == &resolver_cancel_object_vtbl) ?
            CONTAINING_RECORD(iface, struct resolver_cancel_object, IUnknown_iface) : NULL;
}

static HRESULT resolver_create_cancel_object(IUnknown *handler, enum resolved_object_origin origin,
        IUnknown *cancel_cookie, IUnknown **cancel_object)
{
    struct resolver_cancel_object *object;

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IUnknown_iface.lpVtbl = &resolver_cancel_object_vtbl;
    object->refcount = 1;
    object->u.handler = handler;
    IUnknown_AddRef(object->u.handler);
    object->cancel_cookie = cancel_cookie;
    IUnknown_AddRef(object->cancel_cookie);
    object->origin = origin;

    *cancel_object = &object->IUnknown_iface;

    return S_OK;
}

static HRESULT WINAPI source_resolver_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI source_resolver_callback_stream_AddRef(IMFAsyncCallback *iface)
{
    struct source_resolver *resolver = impl_from_stream_IMFAsyncCallback(iface);
    return IMFSourceResolver_AddRef(&resolver->IMFSourceResolver_iface);
}

static ULONG WINAPI source_resolver_callback_stream_Release(IMFAsyncCallback *iface)
{
    struct source_resolver *resolver = impl_from_stream_IMFAsyncCallback(iface);
    return IMFSourceResolver_Release(&resolver->IMFSourceResolver_iface);
}

static HRESULT WINAPI source_resolver_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI source_resolver_callback_stream_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct source_resolver *resolver = impl_from_stream_IMFAsyncCallback(iface);

    return resolver_handler_end_create(resolver, OBJECT_FROM_BYTESTREAM, result);
}

static const IMFAsyncCallbackVtbl source_resolver_callback_stream_vtbl =
{
    source_resolver_callback_QueryInterface,
    source_resolver_callback_stream_AddRef,
    source_resolver_callback_stream_Release,
    source_resolver_callback_GetParameters,
    source_resolver_callback_stream_Invoke,
};

static ULONG WINAPI source_resolver_callback_url_AddRef(IMFAsyncCallback *iface)
{
    struct source_resolver *resolver = impl_from_url_IMFAsyncCallback(iface);
    return IMFSourceResolver_AddRef(&resolver->IMFSourceResolver_iface);
}

static ULONG WINAPI source_resolver_callback_url_Release(IMFAsyncCallback *iface)
{
    struct source_resolver *resolver = impl_from_url_IMFAsyncCallback(iface);
    return IMFSourceResolver_Release(&resolver->IMFSourceResolver_iface);
}

static HRESULT WINAPI source_resolver_callback_url_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct source_resolver *resolver = impl_from_url_IMFAsyncCallback(iface);

    return resolver_handler_end_create(resolver, OBJECT_FROM_URL, result);
}

static const IMFAsyncCallbackVtbl source_resolver_callback_url_vtbl =
{
    source_resolver_callback_QueryInterface,
    source_resolver_callback_url_AddRef,
    source_resolver_callback_url_Release,
    source_resolver_callback_GetParameters,
    source_resolver_callback_url_Invoke,
};

static HRESULT resolver_create_registered_handler(HKEY hkey, REFIID riid, void **handler)
{
    unsigned int j = 0, name_length, type;
    HRESULT hr = E_FAIL;
    WCHAR clsidW[39];
    CLSID clsid;

    name_length = ARRAY_SIZE(clsidW);
    while (!RegEnumValueW(hkey, j++, clsidW, &name_length, NULL, &type, NULL, NULL))
    {
        if (type == REG_SZ)
        {
            if (SUCCEEDED(CLSIDFromString(clsidW, &clsid)))
            {
                hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, riid, handler);
                if (SUCCEEDED(hr))
                    break;
            }
        }

        name_length = ARRAY_SIZE(clsidW);
    }

    return hr;
}

static HRESULT resolver_get_bytestream_handler(IMFByteStream *stream, const WCHAR *url, DWORD flags,
        IMFByteStreamHandler **handler)
{
    static const char streamhandlerspath[] = "Software\\Microsoft\\Windows Media Foundation\\ByteStreamHandlers";
    static const HKEY hkey_roots[2] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
    IMFAttributes *attributes;
    const WCHAR *url_ext;
    WCHAR *mimeW = NULL;
    unsigned int i, j;
    UINT32 length;
    HRESULT hr;

    *handler = NULL;

    /* MIME type */
    if (SUCCEEDED(IMFByteStream_QueryInterface(stream, &IID_IMFAttributes, (void **)&attributes)))
    {
        IMFAttributes_GetAllocatedString(attributes, &MF_BYTESTREAM_CONTENT_TYPE, &mimeW, &length);
        IMFAttributes_Release(attributes);
    }

    /* Extension */
    url_ext = url ? strrchrW(url, '.') : NULL;

    if (!url_ext && !mimeW)
    {
        CoTaskMemFree(mimeW);
        return MF_E_UNSUPPORTED_BYTESTREAM_TYPE;
    }

    /* FIXME: check local handlers first */

    for (i = 0, hr = E_FAIL; i < ARRAY_SIZE(hkey_roots); ++i)
    {
        const WCHAR *namesW[2] = { mimeW, url_ext };
        HKEY hkey, hkey_handler;

        if (RegOpenKeyA(hkey_roots[i], streamhandlerspath, &hkey))
            continue;

        for (j = 0; j < ARRAY_SIZE(namesW); ++j)
        {
            if (!namesW[j])
                continue;

            if (!RegOpenKeyW(hkey, namesW[j], &hkey_handler))
            {
                hr = resolver_create_registered_handler(hkey_handler, &IID_IMFByteStreamHandler, (void **)handler);
                RegCloseKey(hkey_handler);
            }

            if (SUCCEEDED(hr))
                break;
        }

        RegCloseKey(hkey);

        if (SUCCEEDED(hr))
            break;
    }

    CoTaskMemFree(mimeW);
    return hr;
}

static HRESULT resolver_get_scheme_handler(const WCHAR *url, DWORD flags, IMFSchemeHandler **handler)
{
    static const char schemehandlerspath[] = "Software\\Microsoft\\Windows Media Foundation\\SchemeHandlers";
    static const HKEY hkey_roots[2] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
    const WCHAR *ptr = url;
    unsigned int len, i;
    WCHAR *scheme;
    HRESULT hr;

    /* RFC 3986: scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." ) */
    while (*ptr)
    {
        WCHAR ch = tolowerW(*ptr);

        if (*ptr == '*' && ptr == url)
        {
             ptr++;
             break;
        }
        else if (!(*ptr >= '0' && *ptr <= '9') &&
                !(ch >= 'a' && ch <= 'z') &&
                *ptr != '+' && *ptr != '-' && *ptr != '.')
        {
            break;
        }

        ptr++;
    }

    /* Schemes must end with a ':' */
    if (ptr == url || *ptr != ':')
        return MF_E_UNSUPPORTED_SCHEME;

    len = ptr - url;
    scheme = heap_alloc((len + 1) * sizeof(WCHAR));
    if (!scheme)
        return E_OUTOFMEMORY;

    memcpy(scheme, url, len * sizeof(WCHAR));
    scheme[len] = 0;

    /* FIXME: check local handlers first */

    for (i = 0, hr = E_FAIL; i < ARRAY_SIZE(hkey_roots); ++i)
    {
        HKEY hkey, hkey_handler;

        if (RegOpenKeyA(hkey_roots[i], schemehandlerspath, &hkey))
            continue;

        if (!RegOpenKeyW(hkey, scheme, &hkey_handler))
        {
            hr = resolver_create_registered_handler(hkey_handler, &IID_IMFSchemeHandler, (void **)handler);
            RegCloseKey(hkey_handler);
        }

        RegCloseKey(hkey);

        if (SUCCEEDED(hr))
            break;
    }

    heap_free(scheme);

    return hr;
}

static HRESULT resolver_end_create_object(struct source_resolver *resolver, enum resolved_object_origin origin,
        IMFAsyncResult *result, MF_OBJECT_TYPE *obj_type, IUnknown **out)
{
    struct resolver_queued_result *queued_result = NULL, *iter;
    IUnknown *object;
    HRESULT hr;

    if (FAILED(hr = IMFAsyncResult_GetObject(result, &object)))
        return hr;

    EnterCriticalSection(&resolver->cs);

    LIST_FOR_EACH_ENTRY(iter, &resolver->pending, struct resolver_queued_result, entry)
    {
        if (iter->object == object && iter->origin == origin)
        {
            list_remove(&iter->entry);
            queued_result = iter;
            break;
        }
    }

    LeaveCriticalSection(&resolver->cs);

    IUnknown_Release(object);

    if (queued_result)
    {
        *out = queued_result->object;
        *obj_type = queued_result->obj_type;
        hr = queued_result->hr;
        heap_free(queued_result);
    }
    else
        hr = E_UNEXPECTED;

    return hr;
}

static HRESULT WINAPI source_resolver_QueryInterface(IMFSourceResolver *iface, REFIID riid, void **obj)
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

static ULONG WINAPI source_resolver_AddRef(IMFSourceResolver *iface)
{
    struct source_resolver *resolver = impl_from_IMFSourceResolver(iface);
    ULONG refcount = InterlockedIncrement(&resolver->refcount);

    TRACE("%p, refcount %d.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI source_resolver_Release(IMFSourceResolver *iface)
{
    struct source_resolver *resolver = impl_from_IMFSourceResolver(iface);
    ULONG refcount = InterlockedDecrement(&resolver->refcount);
    struct resolver_queued_result *result, *result2;

    TRACE("%p, refcount %d.\n", iface, refcount);

    if (!refcount)
    {
        LIST_FOR_EACH_ENTRY_SAFE(result, result2, &resolver->pending, struct resolver_queued_result, entry)
        {
            if (result->object)
                IUnknown_Release(result->object);
            list_remove(&result->entry);
            heap_free(result);
        }
        DeleteCriticalSection(&resolver->cs);
        heap_free(resolver);
    }

    return refcount;
}

static HRESULT WINAPI source_resolver_CreateObjectFromURL(IMFSourceResolver *iface, const WCHAR *url,
        DWORD flags, IPropertyStore *props, MF_OBJECT_TYPE *obj_type, IUnknown **object)
{
    struct source_resolver *resolver = impl_from_IMFSourceResolver(iface);
    IMFSchemeHandler *handler;
    IMFAsyncResult *result;
    MFASYNCRESULT *data;
    HRESULT hr;

    TRACE("%p, %s, %#x, %p, %p, %p\n", iface, debugstr_w(url), flags, props, obj_type, object);

    if (!url || !obj_type || !object)
        return E_POINTER;

    if (FAILED(hr = resolver_get_scheme_handler(url, flags, &handler)))
        return hr;

    hr = MFCreateAsyncResult((IUnknown *)handler, NULL, NULL, &result);
    IMFSchemeHandler_Release(handler);
    if (FAILED(hr))
        return hr;

    data = (MFASYNCRESULT *)result;
    data->hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    hr = IMFSchemeHandler_BeginCreateObject(handler, url, flags, props, NULL, &resolver->stream_callback,
            (IUnknown *)result);
    if (FAILED(hr))
    {
        IMFAsyncResult_Release(result);
        return hr;
    }

    WaitForSingleObject(data->hEvent, INFINITE);

    hr = resolver_end_create_object(resolver, OBJECT_FROM_URL, result, obj_type, object);
    IMFAsyncResult_Release(result);

    return hr;
}

static HRESULT WINAPI source_resolver_CreateObjectFromByteStream(IMFSourceResolver *iface, IMFByteStream *stream,
    const WCHAR *url, DWORD flags, IPropertyStore *props, MF_OBJECT_TYPE *obj_type, IUnknown **object)
{
    struct source_resolver *resolver = impl_from_IMFSourceResolver(iface);
    IMFByteStreamHandler *handler;
    IMFAsyncResult *result;
    MFASYNCRESULT *data;
    HRESULT hr;

    TRACE("%p, %p, %s, %#x, %p, %p, %p.\n", iface, stream, debugstr_w(url), flags, props, obj_type, object);

    if (!stream || !obj_type || !object)
        return E_POINTER;

    if (FAILED(hr = resolver_get_bytestream_handler(stream, url, flags, &handler)))
        goto fallback;

    hr = MFCreateAsyncResult((IUnknown *)handler, NULL, NULL, &result);
    IMFByteStreamHandler_Release(handler);
    if (FAILED(hr))
        return hr;

    data = (MFASYNCRESULT *)result;
    data->hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    hr = IMFByteStreamHandler_BeginCreateObject(handler, stream, url, flags, props, NULL, &resolver->stream_callback,
            (IUnknown *)result);
    if (FAILED(hr))
    {
        IMFAsyncResult_Release(result);
        return hr;
    }

    WaitForSingleObject(data->hEvent, INFINITE);

    hr = resolver_end_create_object(resolver, OBJECT_FROM_BYTESTREAM, result, obj_type, object);
    IMFAsyncResult_Release(result);

    /* TODO: following stub is left intentionally until real source plugins are implemented.  */
    if (SUCCEEDED(hr))
        return hr;

fallback:
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

static HRESULT WINAPI source_resolver_BeginCreateObjectFromURL(IMFSourceResolver *iface, const WCHAR *url,
        DWORD flags, IPropertyStore *props, IUnknown **cancel_cookie, IMFAsyncCallback *callback, IUnknown *state)
{
    struct source_resolver *resolver = impl_from_IMFSourceResolver(iface);
    IMFSchemeHandler *handler;
    IUnknown *inner_cookie = NULL;
    IMFAsyncResult *result;
    HRESULT hr;

    TRACE("%p, %s, %#x, %p, %p, %p, %p.\n", iface, debugstr_w(url), flags, props, cancel_cookie, callback, state);

    if (FAILED(hr = resolver_get_scheme_handler(url, flags, &handler)))
        return hr;

    if (cancel_cookie)
        *cancel_cookie = NULL;

    hr = MFCreateAsyncResult((IUnknown *)handler, callback, state, &result);
    IMFSchemeHandler_Release(handler);
    if (FAILED(hr))
        return hr;

    hr = IMFSchemeHandler_BeginCreateObject(handler, url, flags, props, cancel_cookie ? &inner_cookie : NULL,
            &resolver->url_callback, (IUnknown *)result);

    if (SUCCEEDED(hr) && inner_cookie)
        resolver_create_cancel_object((IUnknown *)handler, OBJECT_FROM_URL, inner_cookie, cancel_cookie);

    IMFAsyncResult_Release(result);

    return hr;
}

static HRESULT WINAPI source_resolver_EndCreateObjectFromURL(IMFSourceResolver *iface, IMFAsyncResult *result,
        MF_OBJECT_TYPE *obj_type, IUnknown **object)
{
    struct source_resolver *resolver = impl_from_IMFSourceResolver(iface);

    TRACE("%p, %p, %p, %p.\n", iface, result, obj_type, object);

    return resolver_end_create_object(resolver, OBJECT_FROM_URL, result, obj_type, object);
}

static HRESULT WINAPI source_resolver_BeginCreateObjectFromByteStream(IMFSourceResolver *iface, IMFByteStream *stream,
        const WCHAR *url, DWORD flags, IPropertyStore *props, IUnknown **cancel_cookie, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct source_resolver *resolver = impl_from_IMFSourceResolver(iface);
    IMFByteStreamHandler *handler;
    IUnknown *inner_cookie = NULL;
    IMFAsyncResult *result;
    HRESULT hr;

    TRACE("%p, %p, %s, %#x, %p, %p, %p, %p.\n", iface, stream, debugstr_w(url), flags, props, cancel_cookie,
            callback, state);

    if (FAILED(hr = resolver_get_bytestream_handler(stream, url, flags, &handler)))
        return hr;

    if (cancel_cookie)
        *cancel_cookie = NULL;

    hr = MFCreateAsyncResult((IUnknown *)handler, callback, state, &result);
    IMFByteStreamHandler_Release(handler);
    if (FAILED(hr))
        return hr;

    hr = IMFByteStreamHandler_BeginCreateObject(handler, stream, url, flags, props,
            cancel_cookie ? &inner_cookie : NULL, &resolver->stream_callback, (IUnknown *)result);

    /* Cancel object wraps underlying handler cancel cookie with context necessary to call CancelObjectCreate(). */
    if (SUCCEEDED(hr) && inner_cookie)
        resolver_create_cancel_object((IUnknown *)handler, OBJECT_FROM_BYTESTREAM, inner_cookie, cancel_cookie);

    IMFAsyncResult_Release(result);

    return hr;
}

static HRESULT WINAPI source_resolver_EndCreateObjectFromByteStream(IMFSourceResolver *iface, IMFAsyncResult *result,
        MF_OBJECT_TYPE *obj_type, IUnknown **object)
{
    struct source_resolver *resolver = impl_from_IMFSourceResolver(iface);

    TRACE("%p, %p, %p, %p.\n", iface, result, obj_type, object);

    return resolver_end_create_object(resolver, OBJECT_FROM_BYTESTREAM, result, obj_type, object);
}

static HRESULT WINAPI source_resolver_CancelObjectCreation(IMFSourceResolver *iface, IUnknown *cancel_cookie)
{
    struct resolver_cancel_object *object;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, cancel_cookie);

    if (!(object = unsafe_impl_cancel_obj_from_IUnknown(cancel_cookie)))
        return E_UNEXPECTED;

    switch (object->origin)
    {
        case OBJECT_FROM_BYTESTREAM:
            hr = IMFByteStreamHandler_CancelObjectCreation(object->u.stream_handler, object->cancel_cookie);
            break;
        case OBJECT_FROM_URL:
            hr = IMFSchemeHandler_CancelObjectCreation(object->u.scheme_handler, object->cancel_cookie);
            break;
        default:
            hr = E_UNEXPECTED;
    }

    return hr;
}

static const IMFSourceResolverVtbl mfsourceresolvervtbl =
{
    source_resolver_QueryInterface,
    source_resolver_AddRef,
    source_resolver_Release,
    source_resolver_CreateObjectFromURL,
    source_resolver_CreateObjectFromByteStream,
    source_resolver_BeginCreateObjectFromURL,
    source_resolver_EndCreateObjectFromURL,
    source_resolver_BeginCreateObjectFromByteStream,
    source_resolver_EndCreateObjectFromByteStream,
    source_resolver_CancelObjectCreation,
};

/***********************************************************************
 *      MFCreateSourceResolver (mfplat.@)
 */
HRESULT WINAPI MFCreateSourceResolver(IMFSourceResolver **resolver)
{
    struct source_resolver *object;

    TRACE("%p\n", resolver);

    if (!resolver)
        return E_POINTER;

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFSourceResolver_iface.lpVtbl = &mfsourceresolvervtbl;
    object->stream_callback.lpVtbl = &source_resolver_callback_stream_vtbl;
    object->url_callback.lpVtbl = &source_resolver_callback_url_vtbl;
    object->refcount = 1;
    list_init(&object->pending);
    InitializeCriticalSection(&object->cs);

    *resolver = &object->IMFSourceResolver_iface;

    return S_OK;
}

typedef struct media_event
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
    struct media_event *event = impl_from_IMFMediaEvent(iface);
    ULONG ref = InterlockedDecrement(&event->attributes.ref);

    TRACE("(%p) ref=%u\n", iface, ref);

    if (!ref)
    {
        clear_attributes_object(&event->attributes);
        PropVariantClear(&event->value);
        heap_free(event);
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
    struct media_event *event = impl_from_IMFMediaEvent(iface);
    return IMFAttributes_CopyAllItems(&event->attributes.IMFAttributes_iface, dest);
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

/***********************************************************************
 *      MFCreateMediaEvent (mfplat.@)
 */
HRESULT WINAPI MFCreateMediaEvent(MediaEventType type, REFGUID extended_type, HRESULT status,
                                  const PROPVARIANT *value, IMFMediaEvent **event)
{
    mfmediaevent *object;
    HRESULT hr;

    TRACE("%#x, %s, %08x, %p, %p\n", type, debugstr_guid(extended_type), status, value, event);

    object = HeapAlloc( GetProcessHeap(), 0, sizeof(*object) );
    if(!object)
        return E_OUTOFMEMORY;

    if (FAILED(hr = init_attributes_object(&object->attributes, 0)))
    {
        heap_free(object);
        return hr;
    }
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

struct event_queue
{
    IMFMediaEventQueue IMFMediaEventQueue_iface;
    LONG refcount;

    CRITICAL_SECTION cs;
    CONDITION_VARIABLE update_event;
    struct list events;
    BOOL is_shut_down;
    IMFAsyncResult *subscriber;
};

struct queued_event
{
    struct list entry;
    IMFMediaEvent *event;
};

static inline struct event_queue *impl_from_IMFMediaEventQueue(IMFMediaEventQueue *iface)
{
    return CONTAINING_RECORD(iface, struct event_queue, IMFMediaEventQueue_iface);
}

static IMFMediaEvent *queue_pop_event(struct event_queue *queue)
{
    struct list *head = list_head(&queue->events);
    struct queued_event *queued_event;
    IMFMediaEvent *event;

    if (!head)
        return NULL;

    queued_event = LIST_ENTRY(head, struct queued_event, entry);
    event = queued_event->event;
    list_remove(&queued_event->entry);
    heap_free(queued_event);
    return event;
}

static void event_queue_cleanup(struct event_queue *queue)
{
    IMFMediaEvent *event;

    while ((event = queue_pop_event(queue)))
        IMFMediaEvent_Release(event);
}

static HRESULT WINAPI eventqueue_QueryInterface(IMFMediaEventQueue *iface, REFIID riid, void **out)
{
    struct event_queue *queue = impl_from_IMFMediaEventQueue(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFMediaEventQueue) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &queue->IMFMediaEventQueue_iface;
        IMFMediaEventQueue_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI eventqueue_AddRef(IMFMediaEventQueue *iface)
{
    struct event_queue *queue = impl_from_IMFMediaEventQueue(iface);
    ULONG refcount = InterlockedIncrement(&queue->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI eventqueue_Release(IMFMediaEventQueue *iface)
{
    struct event_queue *queue = impl_from_IMFMediaEventQueue(iface);
    ULONG refcount = InterlockedDecrement(&queue->refcount);

    TRACE("%p, refcount %u.\n", queue, refcount);

    if (!refcount)
    {
        event_queue_cleanup(queue);
        DeleteCriticalSection(&queue->cs);
        heap_free(queue);
    }

    return refcount;
}

static HRESULT WINAPI eventqueue_GetEvent(IMFMediaEventQueue *iface, DWORD flags, IMFMediaEvent **event)
{
    struct event_queue *queue = impl_from_IMFMediaEventQueue(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, event);

    EnterCriticalSection(&queue->cs);

    if (queue->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else if (queue->subscriber)
        hr = MF_E_MULTIPLE_SUBSCRIBERS;
    else
    {
        if (flags & MF_EVENT_FLAG_NO_WAIT)
        {
            if (!(*event = queue_pop_event(queue)))
                hr = MF_E_NO_EVENTS_AVAILABLE;
        }
        else
        {
            while (list_empty(&queue->events) && !queue->is_shut_down)
            {
                SleepConditionVariableCS(&queue->update_event, &queue->cs, INFINITE);
            }
            *event = queue_pop_event(queue);
            if (queue->is_shut_down)
                hr = MF_E_SHUTDOWN;
        }
    }

    LeaveCriticalSection(&queue->cs);

    return hr;
}

static void queue_notify_subscriber(struct event_queue *queue)
{
    if (list_empty(&queue->events) || !queue->subscriber)
        return;

    MFPutWorkItemEx(MFASYNC_CALLBACK_QUEUE_STANDARD, queue->subscriber);
}

static HRESULT WINAPI eventqueue_BeginGetEvent(IMFMediaEventQueue *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    struct event_queue *queue = impl_from_IMFMediaEventQueue(iface);
    MFASYNCRESULT *result_data = (MFASYNCRESULT *)queue->subscriber;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", iface, callback, state);

    if (!callback)
        return E_INVALIDARG;

    EnterCriticalSection(&queue->cs);

    if (queue->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else if (result_data)
    {
        if (result_data->pCallback == callback)
            hr = IMFAsyncResult_GetStateNoAddRef(queue->subscriber) == state ?
                    MF_S_MULTIPLE_BEGIN : MF_E_MULTIPLE_BEGIN;
        else
            hr = MF_E_MULTIPLE_SUBSCRIBERS;
    }
    else
    {
        hr = MFCreateAsyncResult(NULL, callback, state, &queue->subscriber);
        if (SUCCEEDED(hr))
            queue_notify_subscriber(queue);
    }

    LeaveCriticalSection(&queue->cs);

    return hr;
}

static HRESULT WINAPI eventqueue_EndGetEvent(IMFMediaEventQueue *iface, IMFAsyncResult *result, IMFMediaEvent **event)
{
    struct event_queue *queue = impl_from_IMFMediaEventQueue(iface);
    HRESULT hr = E_FAIL;

    TRACE("%p, %p, %p.\n", iface, result, event);

    EnterCriticalSection(&queue->cs);

    if (queue->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else if (queue->subscriber == result)
    {
        *event = queue_pop_event(queue);
        if (queue->subscriber)
            IMFAsyncResult_Release(queue->subscriber);
        queue->subscriber = NULL;
        hr = *event ? S_OK : E_FAIL;
    }

    LeaveCriticalSection(&queue->cs);

    return hr;
}

static HRESULT eventqueue_queue_event(struct event_queue *queue, IMFMediaEvent *event)
{
    struct queued_event *queued_event;
    HRESULT hr = S_OK;

    queued_event = heap_alloc(sizeof(*queued_event));
    if (!queued_event)
        return E_OUTOFMEMORY;

    queued_event->event = event;

    EnterCriticalSection(&queue->cs);

    if (queue->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else
    {
        IMFMediaEvent_AddRef(queued_event->event);
        list_add_tail(&queue->events, &queued_event->entry);
        queue_notify_subscriber(queue);
    }

    LeaveCriticalSection(&queue->cs);

    if (FAILED(hr))
        heap_free(queued_event);

    WakeAllConditionVariable(&queue->update_event);

    return hr;
}

static HRESULT WINAPI eventqueue_QueueEvent(IMFMediaEventQueue *iface, IMFMediaEvent *event)
{
    struct event_queue *queue = impl_from_IMFMediaEventQueue(iface);

    TRACE("%p, %p.\n", iface, event);

    return eventqueue_queue_event(queue, event);
}

static HRESULT WINAPI eventqueue_QueueEventParamVar(IMFMediaEventQueue *iface, MediaEventType event_type,
        REFGUID extended_type, HRESULT status, const PROPVARIANT *value)
{
    struct event_queue *queue = impl_from_IMFMediaEventQueue(iface);
    IMFMediaEvent *event;
    HRESULT hr;

    TRACE("%p, %d, %s, %#x, %p\n", iface, event_type, debugstr_guid(extended_type), status, value);

    if (FAILED(hr = MFCreateMediaEvent(event_type, extended_type, status, value, &event)))
        return hr;

    hr = eventqueue_queue_event(queue, event);
    IMFMediaEvent_Release(event);
    return hr;
}

static HRESULT WINAPI eventqueue_QueueEventParamUnk(IMFMediaEventQueue *iface, MediaEventType event_type,
        REFGUID extended_type, HRESULT status, IUnknown *unk)
{
    struct event_queue *queue = impl_from_IMFMediaEventQueue(iface);
    IMFMediaEvent *event;
    PROPVARIANT value;
    HRESULT hr;

    TRACE("%p, %d, %s, %#x, %p.\n", iface, event_type, debugstr_guid(extended_type), status, unk);

    value.vt = VT_UNKNOWN;
    value.u.punkVal = unk;

    if (FAILED(hr = MFCreateMediaEvent(event_type, extended_type, status, &value, &event)))
        return hr;

    hr = eventqueue_queue_event(queue, event);
    IMFMediaEvent_Release(event);
    return hr;
}

static HRESULT WINAPI eventqueue_Shutdown(IMFMediaEventQueue *iface)
{
    struct event_queue *queue = impl_from_IMFMediaEventQueue(iface);

    TRACE("%p\n", queue);

    EnterCriticalSection(&queue->cs);

    if (!queue->is_shut_down)
    {
        event_queue_cleanup(queue);
        queue->is_shut_down = TRUE;
    }

    LeaveCriticalSection(&queue->cs);

    WakeAllConditionVariable(&queue->update_event);

    return S_OK;
}

static const IMFMediaEventQueueVtbl eventqueuevtbl =
{
    eventqueue_QueryInterface,
    eventqueue_AddRef,
    eventqueue_Release,
    eventqueue_GetEvent,
    eventqueue_BeginGetEvent,
    eventqueue_EndGetEvent,
    eventqueue_QueueEvent,
    eventqueue_QueueEventParamVar,
    eventqueue_QueueEventParamUnk,
    eventqueue_Shutdown
};

/***********************************************************************
 *      MFCreateEventQueue (mfplat.@)
 */
HRESULT WINAPI MFCreateEventQueue(IMFMediaEventQueue **queue)
{
    struct event_queue *object;

    TRACE("%p\n", queue);

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFMediaEventQueue_iface.lpVtbl = &eventqueuevtbl;
    object->refcount = 1;
    list_init(&object->events);
    InitializeCriticalSection(&object->cs);
    InitializeConditionVariable(&object->update_event);

    *queue = &object->IMFMediaEventQueue_iface;

    return S_OK;
}

struct collection
{
    IMFCollection IMFCollection_iface;
    LONG refcount;
    IUnknown **elements;
    size_t capacity;
    size_t count;
};

static struct collection *impl_from_IMFCollection(IMFCollection *iface)
{
    return CONTAINING_RECORD(iface, struct collection, IMFCollection_iface);
}

static void collection_clear(struct collection *collection)
{
    size_t i;

    for (i = 0; i < collection->count; ++i)
    {
        if (collection->elements[i])
            IUnknown_Release(collection->elements[i]);
    }

    heap_free(collection->elements);
    collection->elements = NULL;
    collection->count = 0;
    collection->capacity = 0;
}

static HRESULT WINAPI collection_QueryInterface(IMFCollection *iface, REFIID riid, void **out)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFCollection) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFCollection_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI collection_AddRef(IMFCollection *iface)
{
    struct collection *collection = impl_from_IMFCollection(iface);
    ULONG refcount = InterlockedIncrement(&collection->refcount);

    TRACE("%p, %d.\n", collection, refcount);

    return refcount;
}

static ULONG WINAPI collection_Release(IMFCollection *iface)
{
    struct collection *collection = impl_from_IMFCollection(iface);
    ULONG refcount = InterlockedDecrement(&collection->refcount);

    TRACE("%p, %d.\n", collection, refcount);

    if (!refcount)
    {
        collection_clear(collection);
        heap_free(collection->elements);
        heap_free(collection);
    }

    return refcount;
}

static HRESULT WINAPI collection_GetElementCount(IMFCollection *iface, DWORD *count)
{
    struct collection *collection = impl_from_IMFCollection(iface);

    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_POINTER;

    *count = collection->count;

    return S_OK;
}

static HRESULT WINAPI collection_GetElement(IMFCollection *iface, DWORD idx, IUnknown **element)
{
    struct collection *collection = impl_from_IMFCollection(iface);

    TRACE("%p, %u, %p.\n", iface, idx, element);

    if (!element)
        return E_POINTER;

    if (idx >= collection->count)
        return E_INVALIDARG;

    *element = collection->elements[idx];
    if (*element)
        IUnknown_AddRef(*element);

    return *element ? S_OK : E_UNEXPECTED;
}

static HRESULT WINAPI collection_AddElement(IMFCollection *iface, IUnknown *element)
{
    struct collection *collection = impl_from_IMFCollection(iface);

    TRACE("%p, %p.\n", iface, element);

    if (!mf_array_reserve((void **)&collection->elements, &collection->capacity, collection->count + 1,
            sizeof(*collection->elements)))
        return E_OUTOFMEMORY;

    collection->elements[collection->count++] = element;
    if (element)
        IUnknown_AddRef(element);

    return S_OK;
}

static HRESULT WINAPI collection_RemoveElement(IMFCollection *iface, DWORD idx, IUnknown **element)
{
    struct collection *collection = impl_from_IMFCollection(iface);
    size_t count;

    TRACE("%p, %u, %p.\n", iface, idx, element);

    if (!element)
        return E_POINTER;

    if (idx >= collection->count)
        return E_INVALIDARG;

    *element = collection->elements[idx];

    count = collection->count - idx - 1;
    if (count)
        memmove(&collection->elements[idx], &collection->elements[idx + 1], count * sizeof(*collection->elements));
    collection->count--;

    return S_OK;
}

static HRESULT WINAPI collection_InsertElementAt(IMFCollection *iface, DWORD idx, IUnknown *element)
{
    struct collection *collection = impl_from_IMFCollection(iface);
    size_t i;

    TRACE("%p, %u, %p.\n", iface, idx, element);

    if (!mf_array_reserve((void **)&collection->elements, &collection->capacity, idx + 1,
            sizeof(*collection->elements)))
        return E_OUTOFMEMORY;

    if (idx < collection->count)
    {
        memmove(&collection->elements[idx + 1], &collection->elements[idx],
            (collection->count - idx) * sizeof(*collection->elements));
        collection->count++;
    }
    else
    {
        for (i = collection->count; i < idx; ++i)
            collection->elements[i] = NULL;
        collection->count = idx + 1;
    }

    collection->elements[idx] = element;
    if (collection->elements[idx])
        IUnknown_AddRef(collection->elements[idx]);

    return S_OK;
}

static HRESULT WINAPI collection_RemoveAllElements(IMFCollection *iface)
{
    struct collection *collection = impl_from_IMFCollection(iface);

    TRACE("%p.\n", iface);

    collection_clear(collection);

    return S_OK;
}

static const IMFCollectionVtbl mfcollectionvtbl =
{
    collection_QueryInterface,
    collection_AddRef,
    collection_Release,
    collection_GetElementCount,
    collection_GetElement,
    collection_AddElement,
    collection_RemoveElement,
    collection_InsertElementAt,
    collection_RemoveAllElements,
};

/***********************************************************************
 *      MFCreateCollection (mfplat.@)
 */
HRESULT WINAPI MFCreateCollection(IMFCollection **collection)
{
    struct collection *object;

    TRACE("%p\n", collection);

    if (!collection)
        return E_POINTER;

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFCollection_iface.lpVtbl = &mfcollectionvtbl;
    object->refcount = 1;

    *collection = &object->IMFCollection_iface;

    return S_OK;
}

/***********************************************************************
 *      MFHeapAlloc (mfplat.@)
 */
void *WINAPI MFHeapAlloc(SIZE_T size, ULONG flags, char *file, int line, EAllocationType type)
{
    TRACE("%lu, %#x, %s, %d, %#x.\n", size, flags, debugstr_a(file), line, type);
    return HeapAlloc(GetProcessHeap(), flags, size);
}

/***********************************************************************
 *      MFHeapFree (mfplat.@)
 */
void WINAPI MFHeapFree(void *p)
{
    TRACE("%p\n", p);
    HeapFree(GetProcessHeap(), 0, p);
}

/***********************************************************************
 *      MFCreateMFByteStreamOnStreamEx (mfplat.@)
 */
HRESULT WINAPI MFCreateMFByteStreamOnStreamEx(IUnknown *stream, IMFByteStream **bytestream)
{
    FIXME("(%p, %p): stub\n", stream, bytestream);

    return E_NOTIMPL;
}

static HRESULT WINAPI system_time_source_QueryInterface(IMFPresentationTimeSource *iface, REFIID riid, void **obj)
{
    struct system_time_source *source = impl_from_IMFPresentationTimeSource(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFPresentationTimeSource) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &source->IMFPresentationTimeSource_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFClockStateSink))
    {
        *obj = &source->IMFClockStateSink_iface;
    }
    else
    {
        WARN("Unsupported %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI system_time_source_AddRef(IMFPresentationTimeSource *iface)
{
    struct system_time_source *source = impl_from_IMFPresentationTimeSource(iface);
    ULONG refcount = InterlockedIncrement(&source->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI system_time_source_Release(IMFPresentationTimeSource *iface)
{
    struct system_time_source *source = impl_from_IMFPresentationTimeSource(iface);
    ULONG refcount = InterlockedDecrement(&source->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        DeleteCriticalSection(&source->cs);
        heap_free(source);
    }

    return refcount;
}

static HRESULT WINAPI system_time_source_GetClockCharacteristics(IMFPresentationTimeSource *iface, DWORD *flags)
{
    TRACE("%p, %p.\n", iface, flags);

    *flags = MFCLOCK_CHARACTERISTICS_FLAG_FREQUENCY_10MHZ | MFCLOCK_CHARACTERISTICS_FLAG_IS_SYSTEM_CLOCK;

    return S_OK;
}

static HRESULT WINAPI system_time_source_GetCorrelatedTime(IMFPresentationTimeSource *iface, DWORD reserved,
        LONGLONG *clock_time, MFTIME *system_time)
{
    FIXME("%p, %#x, %p, %p.\n", iface, reserved, clock_time, system_time);

    return E_NOTIMPL;
}


static HRESULT WINAPI system_time_source_GetContinuityKey(IMFPresentationTimeSource *iface, DWORD *key)
{
    TRACE("%p, %p.\n", iface, key);

    *key = 0;

    return S_OK;
}

static HRESULT WINAPI system_time_source_GetState(IMFPresentationTimeSource *iface, DWORD reserved,
        MFCLOCK_STATE *state)
{
    struct system_time_source *source = impl_from_IMFPresentationTimeSource(iface);

    TRACE("%p, %#x, %p.\n", iface, reserved, state);

    EnterCriticalSection(&source->cs);
    *state = source->state;
    LeaveCriticalSection(&source->cs);

    return S_OK;
}

static HRESULT WINAPI system_time_source_GetProperties(IMFPresentationTimeSource *iface, MFCLOCK_PROPERTIES *props)
{
    FIXME("%p, %p.\n", iface, props);

    return E_NOTIMPL;
}

static HRESULT WINAPI system_time_source_GetUnderlyingClock(IMFPresentationTimeSource *iface, IMFClock **clock)
{
    FIXME("%p, %p.\n", iface, clock);

    return E_NOTIMPL;
}

static const IMFPresentationTimeSourceVtbl systemtimesourcevtbl =
{
    system_time_source_QueryInterface,
    system_time_source_AddRef,
    system_time_source_Release,
    system_time_source_GetClockCharacteristics,
    system_time_source_GetCorrelatedTime,
    system_time_source_GetContinuityKey,
    system_time_source_GetState,
    system_time_source_GetProperties,
    system_time_source_GetUnderlyingClock,
};

static HRESULT WINAPI system_time_source_sink_QueryInterface(IMFClockStateSink *iface, REFIID riid, void **out)
{
    struct system_time_source *source = impl_from_IMFClockStateSink(iface);
    return IMFPresentationTimeSource_QueryInterface(&source->IMFPresentationTimeSource_iface, riid, out);
}

static ULONG WINAPI system_time_source_sink_AddRef(IMFClockStateSink *iface)
{
    struct system_time_source *source = impl_from_IMFClockStateSink(iface);
    return IMFPresentationTimeSource_AddRef(&source->IMFPresentationTimeSource_iface);
}

static ULONG WINAPI system_time_source_sink_Release(IMFClockStateSink *iface)
{
    struct system_time_source *source = impl_from_IMFClockStateSink(iface);
    return IMFPresentationTimeSource_Release(&source->IMFPresentationTimeSource_iface);
}

enum clock_command
{
    CLOCK_CMD_START = 0,
    CLOCK_CMD_STOP,
    CLOCK_CMD_PAUSE,
    CLOCK_CMD_RESTART,
    CLOCK_CMD_MAX,
};

static HRESULT system_time_source_change_state(struct system_time_source *source, enum clock_command command)
{
    static const BYTE state_change_is_allowed[MFCLOCK_STATE_PAUSED+1][CLOCK_CMD_MAX] =
    {   /*              S  S* P  R */
        /* INVALID */ { 1, 0, 1, 0 },
        /* RUNNING */ { 1, 1, 1, 0 },
        /* STOPPED */ { 1, 1, 0, 0 },
        /* PAUSED  */ { 1, 1, 0, 1 },
    };
    static const MFCLOCK_STATE states[CLOCK_CMD_MAX] =
    {
        /* CLOCK_CMD_START   */ MFCLOCK_STATE_RUNNING,
        /* CLOCK_CMD_STOP    */ MFCLOCK_STATE_STOPPED,
        /* CLOCK_CMD_PAUSE   */ MFCLOCK_STATE_PAUSED,
        /* CLOCK_CMD_RESTART */ MFCLOCK_STATE_RUNNING,
    };

    /* Special case that go against usual state change vs return value behavior. */
    if (source->state == MFCLOCK_STATE_INVALID && command == CLOCK_CMD_STOP)
        return S_OK;

    if (!state_change_is_allowed[source->state][command])
        return MF_E_INVALIDREQUEST;

    source->state = states[command];

    return S_OK;
}

static HRESULT WINAPI system_time_source_sink_OnClockStart(IMFClockStateSink *iface, MFTIME system_time,
        LONGLONG start_offset)
{
    struct system_time_source *source = impl_from_IMFClockStateSink(iface);
    HRESULT hr;

    TRACE("%p, %s, %s.\n", iface, wine_dbgstr_longlong(system_time), wine_dbgstr_longlong(start_offset));

    EnterCriticalSection(&source->cs);
    hr = system_time_source_change_state(source, CLOCK_CMD_START);
    LeaveCriticalSection(&source->cs);

    /* FIXME: update timestamps */

    return hr;
}

static HRESULT WINAPI system_time_source_sink_OnClockStop(IMFClockStateSink *iface, MFTIME system_time)
{
    struct system_time_source *source = impl_from_IMFClockStateSink(iface);
    HRESULT hr;

    TRACE("%p, %s.\n", iface, wine_dbgstr_longlong(system_time));

    EnterCriticalSection(&source->cs);
    hr = system_time_source_change_state(source, CLOCK_CMD_STOP);
    LeaveCriticalSection(&source->cs);

    /* FIXME: update timestamps */

    return hr;
}

static HRESULT WINAPI system_time_source_sink_OnClockPause(IMFClockStateSink *iface, MFTIME system_time)
{
    struct system_time_source *source = impl_from_IMFClockStateSink(iface);
    HRESULT hr;

    TRACE("%p, %s.\n", iface, wine_dbgstr_longlong(system_time));

    EnterCriticalSection(&source->cs);
    hr = system_time_source_change_state(source, CLOCK_CMD_PAUSE);
    LeaveCriticalSection(&source->cs);

    /* FIXME: update timestamps */

    return hr;
}

static HRESULT WINAPI system_time_source_sink_OnClockRestart(IMFClockStateSink *iface, MFTIME system_time)
{
    struct system_time_source *source = impl_from_IMFClockStateSink(iface);
    HRESULT hr;

    TRACE("%p, %s.\n", iface, wine_dbgstr_longlong(system_time));

    EnterCriticalSection(&source->cs);
    hr = system_time_source_change_state(source, CLOCK_CMD_RESTART);
    LeaveCriticalSection(&source->cs);

    /* FIXME: update timestamps */

    return hr;
}

static HRESULT WINAPI system_time_source_sink_OnClockSetRate(IMFClockStateSink *iface, MFTIME system_time, float rate)
{
    FIXME("%p, %s, %f.\n", iface, wine_dbgstr_longlong(system_time), rate);

    return E_NOTIMPL;
}

static const IMFClockStateSinkVtbl systemtimesourcesinkvtbl =
{
    system_time_source_sink_QueryInterface,
    system_time_source_sink_AddRef,
    system_time_source_sink_Release,
    system_time_source_sink_OnClockStart,
    system_time_source_sink_OnClockStop,
    system_time_source_sink_OnClockPause,
    system_time_source_sink_OnClockRestart,
    system_time_source_sink_OnClockSetRate,
};

/***********************************************************************
 *      MFCreateSystemTimeSource (mfplat.@)
 */
HRESULT WINAPI MFCreateSystemTimeSource(IMFPresentationTimeSource **time_source)
{
    struct system_time_source *object;

    TRACE("%p.\n", time_source);

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFPresentationTimeSource_iface.lpVtbl = &systemtimesourcevtbl;
    object->IMFClockStateSink_iface.lpVtbl = &systemtimesourcesinkvtbl;
    object->refcount = 1;
    InitializeCriticalSection(&object->cs);

    *time_source = &object->IMFPresentationTimeSource_iface;

    return S_OK;
}
