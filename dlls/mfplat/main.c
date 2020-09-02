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
#include <math.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"

#include "initguid.h"
#include "rtworkq.h"
#include "ole2.h"
#include "propsys.h"
#include "dxgi.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "mfplat_private.h"
#include "mfreadwrite.h"
#include "mfmediaengine.h"
#include "propvarutil.h"
#include "strsafe.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static HRESULT heap_strdupW(const WCHAR *str, WCHAR **dest)
{
    HRESULT hr = S_OK;

    if (str)
    {
        unsigned int size;

        size = (lstrlenW(str) + 1) * sizeof(WCHAR);
        *dest = heap_alloc(size);
        if (*dest)
            memcpy(*dest, str, size);
        else
            hr = E_OUTOFMEMORY;
    }
    else
        *dest = NULL;

    return hr;
}

struct local_handler
{
    struct list entry;
    union
    {
        WCHAR *scheme;
        struct
        {
            WCHAR *extension;
            WCHAR *mime;
        } bytestream;
    } u;
    IMFActivate *activate;
};

static CRITICAL_SECTION local_handlers_section = { NULL, -1, 0, 0, 0, 0 };

static struct list local_scheme_handlers = LIST_INIT(local_scheme_handlers);
static struct list local_bytestream_handlers = LIST_INIT(local_bytestream_handlers);

struct mft_registration
{
    struct list entry;
    IClassFactory *factory;
    CLSID clsid;
    GUID category;
    WCHAR *name;
    DWORD flags;
    MFT_REGISTER_TYPE_INFO *input_types;
    UINT32 input_types_count;
    MFT_REGISTER_TYPE_INFO *output_types;
    UINT32 output_types_count;
    BOOL local;
};

static CRITICAL_SECTION local_mfts_section = { NULL, -1, 0, 0, 0, 0 };

static struct list local_mfts = LIST_INIT(local_mfts);

struct transform_activate
{
    struct attributes attributes;
    IMFActivate IMFActivate_iface;
    IClassFactory *factory;
    IMFTransform *transform;
};

struct system_clock
{
    IMFClock IMFClock_iface;
    LONG refcount;
};

struct system_time_source
{
    IMFPresentationTimeSource IMFPresentationTimeSource_iface;
    IMFClockStateSink IMFClockStateSink_iface;
    LONG refcount;
    MFCLOCK_STATE state;
    IMFClock *clock;
    LONGLONG start_offset;
    float rate;
    int i_rate;
    CRITICAL_SECTION cs;
};

static void system_time_source_apply_rate(const struct system_time_source *source, LONGLONG *value)
{
    if (source->i_rate)
        *value *= source->i_rate;
    else
        *value *= source->rate;
}

static struct system_time_source *impl_from_IMFPresentationTimeSource(IMFPresentationTimeSource *iface)
{
    return CONTAINING_RECORD(iface, struct system_time_source, IMFPresentationTimeSource_iface);
}

static struct system_time_source *impl_from_IMFClockStateSink(IMFClockStateSink *iface)
{
    return CONTAINING_RECORD(iface, struct system_time_source, IMFClockStateSink_iface);
}

static struct system_clock *impl_from_IMFClock(IMFClock *iface)
{
    return CONTAINING_RECORD(iface, struct system_clock, IMFClock_iface);
}

static struct transform_activate *impl_from_IMFActivate(IMFActivate *iface)
{
    return CONTAINING_RECORD(iface, struct transform_activate, IMFActivate_iface);
}

static HRESULT WINAPI transform_activate_QueryInterface(IMFActivate *iface, REFIID riid, void **out)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFActivate) ||
            IsEqualIID(riid, &IID_IMFAttributes) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFActivate_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI transform_activate_AddRef(IMFActivate *iface)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);
    ULONG refcount = InterlockedIncrement(&activate->attributes.ref);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI transform_activate_Release(IMFActivate *iface)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);
    ULONG refcount = InterlockedDecrement(&activate->attributes.ref);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        clear_attributes_object(&activate->attributes);
        if (activate->factory)
            IClassFactory_Release(activate->factory);
        if (activate->transform)
            IMFTransform_Release(activate->transform);
        heap_free(activate);
    }

    return refcount;
}

static HRESULT WINAPI transform_activate_GetItem(IMFActivate *iface, REFGUID key, PROPVARIANT *value)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetItem(&activate->attributes, key, value);
}

static HRESULT WINAPI transform_activate_GetItemType(IMFActivate *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), type);

    return attributes_GetItemType(&activate->attributes, key, type);
}

static HRESULT WINAPI transform_activate_CompareItem(IMFActivate *iface, REFGUID key, REFPROPVARIANT value,
        BOOL *result)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_propvar(value), result);

    return attributes_CompareItem(&activate->attributes, key, value, result);
}

static HRESULT WINAPI transform_activate_Compare(IMFActivate *iface, IMFAttributes *theirs,
        MF_ATTRIBUTES_MATCH_TYPE match_type, BOOL *ret)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %p, %d, %p.\n", iface, theirs, match_type, ret);

    return attributes_Compare(&activate->attributes, theirs, match_type, ret);
}

static HRESULT WINAPI transform_activate_GetUINT32(IMFActivate *iface, REFGUID key, UINT32 *value)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT32(&activate->attributes, key, value);
}

static HRESULT WINAPI transform_activate_GetUINT64(IMFActivate *iface, REFGUID key, UINT64 *value)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT64(&activate->attributes, key, value);
}

static HRESULT WINAPI transform_activate_GetDouble(IMFActivate *iface, REFGUID key, double *value)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetDouble(&activate->attributes, key, value);
}

static HRESULT WINAPI transform_activate_GetGUID(IMFActivate *iface, REFGUID key, GUID *value)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetGUID(&activate->attributes, key, value);
}

static HRESULT WINAPI transform_activate_GetStringLength(IMFActivate *iface, REFGUID key, UINT32 *length)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), length);

    return attributes_GetStringLength(&activate->attributes, key, length);
}

static HRESULT WINAPI transform_activate_GetString(IMFActivate *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_attr(key), value, size, length);

    return attributes_GetString(&activate->attributes, key, value, size, length);
}

static HRESULT WINAPI transform_activate_GetAllocatedString(IMFActivate *iface, REFGUID key, WCHAR **value,
        UINT32 *length)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), value, length);

    return attributes_GetAllocatedString(&activate->attributes, key, value, length);
}

static HRESULT WINAPI transform_activate_GetBlobSize(IMFActivate *iface, REFGUID key, UINT32 *size)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), size);

    return attributes_GetBlobSize(&activate->attributes, key, size);
}

static HRESULT WINAPI transform_activate_GetBlob(IMFActivate *iface, REFGUID key, UINT8 *buf, UINT32 bufsize,
        UINT32 *blobsize)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_attr(key), buf, bufsize, blobsize);

    return attributes_GetBlob(&activate->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI transform_activate_GetAllocatedBlob(IMFActivate *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), buf, size);

    return attributes_GetAllocatedBlob(&activate->attributes, key, buf, size);
}

static HRESULT WINAPI transform_activate_GetUnknown(IMFActivate *iface, REFGUID key, REFIID riid, void **out)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_guid(riid), out);

    return attributes_GetUnknown(&activate->attributes, key, riid, out);
}

static HRESULT WINAPI transform_activate_SetItem(IMFActivate *iface, REFGUID key, REFPROPVARIANT value)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_propvar(value));

    return attributes_SetItem(&activate->attributes, key, value);
}

static HRESULT WINAPI transform_activate_DeleteItem(IMFActivate *iface, REFGUID key)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s.\n", iface, debugstr_attr(key));

    return attributes_DeleteItem(&activate->attributes, key);
}

static HRESULT WINAPI transform_activate_DeleteAllItems(IMFActivate *iface)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p.\n", iface);

    return attributes_DeleteAllItems(&activate->attributes);
}

static HRESULT WINAPI transform_activate_SetUINT32(IMFActivate *iface, REFGUID key, UINT32 value)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %u.\n", iface, debugstr_attr(key), value);

    return attributes_SetUINT32(&activate->attributes, key, value);
}

static HRESULT WINAPI transform_activate_SetUINT64(IMFActivate *iface, REFGUID key, UINT64 value)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), wine_dbgstr_longlong(value));

    return attributes_SetUINT64(&activate->attributes, key, value);
}

static HRESULT WINAPI transform_activate_SetDouble(IMFActivate *iface, REFGUID key, double value)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_attr(key), value);

    return attributes_SetDouble(&activate->attributes, key, value);
}

static HRESULT WINAPI transform_activate_SetGUID(IMFActivate *iface, REFGUID key, REFGUID value)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_mf_guid(value));

    return attributes_SetGUID(&activate->attributes, key, value);
}

static HRESULT WINAPI transform_activate_SetString(IMFActivate *iface, REFGUID key, const WCHAR *value)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_w(value));

    return attributes_SetString(&activate->attributes, key, value);
}

static HRESULT WINAPI transform_activate_SetBlob(IMFActivate *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p, %u.\n", iface, debugstr_attr(key), buf, size);

    return attributes_SetBlob(&activate->attributes, key, buf, size);
}

static HRESULT WINAPI transform_activate_SetUnknown(IMFActivate *iface, REFGUID key, IUnknown *unknown)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), unknown);

    return attributes_SetUnknown(&activate->attributes, key, unknown);
}

static HRESULT WINAPI transform_activate_LockStore(IMFActivate *iface)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p.\n", iface);

    return attributes_LockStore(&activate->attributes);
}

static HRESULT WINAPI transform_activate_UnlockStore(IMFActivate *iface)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p.\n", iface);

    return attributes_UnlockStore(&activate->attributes);
}

static HRESULT WINAPI transform_activate_GetCount(IMFActivate *iface, UINT32 *count)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %p.\n", iface, count);

    return attributes_GetCount(&activate->attributes, count);
}

static HRESULT WINAPI transform_activate_GetItemByIndex(IMFActivate *iface, UINT32 index, GUID *key,
        PROPVARIANT *value)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return attributes_GetItemByIndex(&activate->attributes, index, key, value);
}

static HRESULT WINAPI transform_activate_CopyAllItems(IMFActivate *iface, IMFAttributes *dest)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %p.\n", iface, dest);

    return attributes_CopyAllItems(&activate->attributes, dest);
}

static HRESULT WINAPI transform_activate_ActivateObject(IMFActivate *iface, REFIID riid, void **obj)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);
    CLSID clsid;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    EnterCriticalSection(&activate->attributes.cs);

    if (!activate->transform)
    {
        if (activate->factory)
        {
            if (FAILED(hr = IClassFactory_CreateInstance(activate->factory, NULL, &IID_IMFTransform,
                    (void **)&activate->transform)))
            {
                hr = MF_E_INVALIDREQUEST;
            }
        }
        else
        {
            if (SUCCEEDED(hr = attributes_GetGUID(&activate->attributes, &MFT_TRANSFORM_CLSID_Attribute, &clsid)))
            {
                if (FAILED(hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IMFTransform,
                        (void **)&activate->transform)))
                {
                    hr = MF_E_INVALIDREQUEST;
                }
            }
        }
    }

    if (activate->transform)
        hr = IMFTransform_QueryInterface(activate->transform, riid, obj);

    LeaveCriticalSection(&activate->attributes.cs);

    return hr;
}

static HRESULT WINAPI transform_activate_ShutdownObject(IMFActivate *iface)
{
    struct transform_activate *activate = impl_from_IMFActivate(iface);

    TRACE("%p.\n", iface);

    EnterCriticalSection(&activate->attributes.cs);

    if (activate->transform)
    {
        IMFTransform_Release(activate->transform);
        activate->transform = NULL;
    }

    LeaveCriticalSection(&activate->attributes.cs);

    return S_OK;
}

static HRESULT WINAPI transform_activate_DetachObject(IMFActivate *iface)
{
    TRACE("%p.\n", iface);

    return E_NOTIMPL;
}

static const IMFActivateVtbl transform_activate_vtbl =
{
    transform_activate_QueryInterface,
    transform_activate_AddRef,
    transform_activate_Release,
    transform_activate_GetItem,
    transform_activate_GetItemType,
    transform_activate_CompareItem,
    transform_activate_Compare,
    transform_activate_GetUINT32,
    transform_activate_GetUINT64,
    transform_activate_GetDouble,
    transform_activate_GetGUID,
    transform_activate_GetStringLength,
    transform_activate_GetString,
    transform_activate_GetAllocatedString,
    transform_activate_GetBlobSize,
    transform_activate_GetBlob,
    transform_activate_GetAllocatedBlob,
    transform_activate_GetUnknown,
    transform_activate_SetItem,
    transform_activate_DeleteItem,
    transform_activate_DeleteAllItems,
    transform_activate_SetUINT32,
    transform_activate_SetUINT64,
    transform_activate_SetDouble,
    transform_activate_SetGUID,
    transform_activate_SetString,
    transform_activate_SetBlob,
    transform_activate_SetUnknown,
    transform_activate_LockStore,
    transform_activate_UnlockStore,
    transform_activate_GetCount,
    transform_activate_GetItemByIndex,
    transform_activate_CopyAllItems,
    transform_activate_ActivateObject,
    transform_activate_ShutdownObject,
    transform_activate_DetachObject,
};

static HRESULT create_transform_activate(IClassFactory *factory, IMFActivate **activate)
{
    struct transform_activate *object;
    HRESULT hr;

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    if (FAILED(hr = init_attributes_object(&object->attributes, 0)))
    {
        heap_free(object);
        return hr;
    }

    object->IMFActivate_iface.lpVtbl = &transform_activate_vtbl;
    object->factory = factory;
    if (object->factory)
        IClassFactory_AddRef(object->factory);

    *activate = &object->IMFActivate_iface;

    return S_OK;
}

HRESULT WINAPI MFCreateTransformActivate(IMFActivate **activate)
{
    TRACE("%p.\n", activate);

    return create_transform_activate(NULL, activate);
}

static const WCHAR transform_keyW[] = {'M','e','d','i','a','F','o','u','n','d','a','t','i','o','n','\\',
                                 'T','r','a','n','s','f','o','r','m','s',0};
static const WCHAR categories_keyW[] = {'M','e','d','i','a','F','o','u','n','d','a','t','i','o','n','\\',
                                 'T','r','a','n','s','f','o','r','m','s','\\',
                                 'C','a','t','e','g','o','r','i','e','s',0};
static const WCHAR inputtypesW[]  = {'I','n','p','u','t','T','y','p','e','s',0};
static const WCHAR outputtypesW[] = {'O','u','t','p','u','t','T','y','p','e','s',0};
static const WCHAR attributesW[] = {'A','t','t','r','i','b','u','t','e','s',0};
static const WCHAR mftflagsW[] = {'M','F','T','F','l','a','g','s',0};
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
    swprintf(str, 39, szGUIDFmt, guid->Data1, guid->Data2,
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

    if (!s[36]) return TRUE;
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

static HRESULT register_transform(const CLSID *clsid, const WCHAR *name, UINT32 flags,
        UINT32 cinput, const MFT_REGISTER_TYPE_INFO *input_types, UINT32 coutput,
        const MFT_REGISTER_TYPE_INFO *output_types, IMFAttributes *attributes)
{
    static const WCHAR reg_format[] = {'%','s','\\','%','s',0};
    HRESULT hr = S_OK;
    HKEY hclsid = 0;
    WCHAR buffer[64];
    DWORD size, ret;
    WCHAR str[250];
    UINT8 *blob;

    GUIDToString(buffer, clsid);
    swprintf(str, ARRAY_SIZE(str), reg_format, transform_keyW, buffer);

    if ((ret = RegCreateKeyW(HKEY_CLASSES_ROOT, str, &hclsid)))
        hr = HRESULT_FROM_WIN32(ret);

    if (SUCCEEDED(hr))
    {
        size = (lstrlenW(name) + 1) * sizeof(WCHAR);
        if ((ret = RegSetValueExW(hclsid, NULL, 0, REG_SZ, (BYTE *)name, size)))
            hr = HRESULT_FROM_WIN32(ret);
    }

    if (SUCCEEDED(hr) && cinput && input_types)
    {
        size = cinput * sizeof(MFT_REGISTER_TYPE_INFO);
        if ((ret = RegSetValueExW(hclsid, inputtypesW, 0, REG_BINARY, (BYTE *)input_types, size)))
            hr = HRESULT_FROM_WIN32(ret);
    }

    if (SUCCEEDED(hr) && coutput && output_types)
    {
        size = coutput * sizeof(MFT_REGISTER_TYPE_INFO);
        if ((ret = RegSetValueExW(hclsid, outputtypesW, 0, REG_BINARY, (BYTE *)output_types, size)))
            hr = HRESULT_FROM_WIN32(ret);
    }

    if (SUCCEEDED(hr) && attributes)
    {
        if (SUCCEEDED(hr = MFGetAttributesAsBlobSize(attributes, &size)))
        {
            if ((blob = heap_alloc(size)))
            {
                if (SUCCEEDED(hr = MFGetAttributesAsBlob(attributes, blob, size)))
                {
                    if ((ret = RegSetValueExW(hclsid, attributesW, 0, REG_BINARY, blob, size)))
                        hr = HRESULT_FROM_WIN32(ret);
                }
                heap_free(blob);
            }
            else
                hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr) && flags)
    {
        if ((ret = RegSetValueExW(hclsid, mftflagsW, 0, REG_DWORD, (BYTE *)&flags, sizeof(flags))))
            hr = HRESULT_FROM_WIN32(ret);
    }

    RegCloseKey(hclsid);
    return hr;
}

static HRESULT register_category(CLSID *clsid, GUID *category)
{
    static const WCHAR reg_format[] = {'%','s','\\','%','s','\\','%','s',0};
    HKEY htmp1;
    WCHAR guid1[64], guid2[64];
    WCHAR str[350];

    GUIDToString(guid1, category);
    GUIDToString(guid2, clsid);

    swprintf(str, ARRAY_SIZE(str), reg_format, categories_keyW, guid1, guid2);

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

    TRACE("%s, %s, %s, %#x, %u, %p, %u, %p, %p.\n", debugstr_guid(&clsid), debugstr_guid(&category),
            debugstr_w(name), flags, cinput, input_types, coutput, output_types, attributes);

    hr = register_transform(&clsid, name, flags, cinput, input_types, coutput, output_types, attributes);
    if(FAILED(hr))
        ERR("Failed to write register transform\n");

    if (SUCCEEDED(hr))
        hr = register_category(&clsid, &category);

    return hr;
}

static void release_mft_registration(struct mft_registration *mft)
{
    if (mft->factory)
        IClassFactory_Release(mft->factory);
    heap_free(mft->name);
    heap_free(mft->input_types);
    heap_free(mft->output_types);
    heap_free(mft);
}

static HRESULT mft_register_local(IClassFactory *factory, REFCLSID clsid, REFGUID category, LPCWSTR name, UINT32 flags,
        UINT32 input_count, const MFT_REGISTER_TYPE_INFO *input_types, UINT32 output_count,
        const MFT_REGISTER_TYPE_INFO *output_types)
{
    struct mft_registration *mft, *cur, *unreg_mft = NULL;
    HRESULT hr;

    if (!factory && !clsid)
    {
        WARN("Can't register without factory or CLSID.\n");
        return E_FAIL;
    }

    mft = heap_alloc_zero(sizeof(*mft));
    if (!mft)
        return E_OUTOFMEMORY;

    mft->factory = factory;
    if (mft->factory)
        IClassFactory_AddRef(mft->factory);
    if (clsid)
        mft->clsid = *clsid;
    mft->category = *category;
    if (!(flags & (MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_ASYNCMFT | MFT_ENUM_FLAG_HARDWARE)))
        flags |= MFT_ENUM_FLAG_SYNCMFT;
    mft->flags = flags;
    mft->local = TRUE;
    if (FAILED(hr = heap_strdupW(name, &mft->name)))
        goto failed;

    if (input_count && input_types)
    {
        mft->input_types_count = input_count;
        if (!(mft->input_types = heap_calloc(mft->input_types_count, sizeof(*input_types))))
        {
            hr = E_OUTOFMEMORY;
            goto failed;
        }
        memcpy(mft->input_types, input_types, mft->input_types_count * sizeof(*input_types));
    }

    if (output_count && output_types)
    {
        mft->output_types_count = output_count;
        if (!(mft->output_types = heap_calloc(mft->output_types_count, sizeof(*output_types))))
        {
            hr = E_OUTOFMEMORY;
            goto failed;
        }
        memcpy(mft->output_types, output_types, mft->output_types_count * sizeof(*output_types));
    }

    EnterCriticalSection(&local_mfts_section);

    LIST_FOR_EACH_ENTRY(cur, &local_mfts, struct mft_registration, entry)
    {
        if (cur->factory == factory)
        {
            unreg_mft = cur;
            list_remove(&cur->entry);
            break;
        }
    }
    list_add_tail(&local_mfts, &mft->entry);

    LeaveCriticalSection(&local_mfts_section);

    if (unreg_mft)
        release_mft_registration(unreg_mft);

failed:
    if (FAILED(hr))
        release_mft_registration(mft);

    return hr;
}

HRESULT WINAPI MFTRegisterLocal(IClassFactory *factory, REFGUID category, LPCWSTR name, UINT32 flags,
        UINT32 input_count, const MFT_REGISTER_TYPE_INFO *input_types, UINT32 output_count,
        const MFT_REGISTER_TYPE_INFO *output_types)
{
    TRACE("%p, %s, %s, %#x, %u, %p, %u, %p.\n", factory, debugstr_guid(category), debugstr_w(name), flags, input_count,
            input_types, output_count, output_types);

    return mft_register_local(factory, NULL, category, name, flags, input_count, input_types, output_count, output_types);
}

HRESULT WINAPI MFTRegisterLocalByCLSID(REFCLSID clsid, REFGUID category, LPCWSTR name, UINT32 flags,
        UINT32 input_count, const MFT_REGISTER_TYPE_INFO *input_types, UINT32 output_count,
        const MFT_REGISTER_TYPE_INFO *output_types)
{
    TRACE("%s, %s, %s, %#x, %u, %p, %u, %p.\n", debugstr_guid(clsid), debugstr_guid(category), debugstr_w(name), flags,
            input_count, input_types, output_count, output_types);

    return mft_register_local(NULL, clsid, category, name, flags, input_count, input_types, output_count, output_types);
}

static HRESULT mft_unregister_local(IClassFactory *factory, REFCLSID clsid)
{
    struct mft_registration *cur, *cur2;
    BOOL unregister_all = !factory && !clsid;
    struct list unreg;

    list_init(&unreg);

    EnterCriticalSection(&local_mfts_section);

    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &local_mfts, struct mft_registration, entry)
    {
        if (!unregister_all)
        {
            if ((factory && cur->factory == factory) || IsEqualCLSID(&cur->clsid, clsid))
            {
                list_remove(&cur->entry);
                list_add_tail(&unreg, &cur->entry);
                break;
            }
        }
        else
        {
            list_remove(&cur->entry);
            list_add_tail(&unreg, &cur->entry);
        }
    }

    LeaveCriticalSection(&local_mfts_section);

    if (!unregister_all && list_empty(&unreg))
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &unreg, struct mft_registration, entry)
    {
        list_remove(&cur->entry);
        release_mft_registration(cur);
    }

    return S_OK;
}

HRESULT WINAPI MFTUnregisterLocalByCLSID(CLSID clsid)
{
    TRACE("%s.\n", debugstr_guid(&clsid));

    return mft_unregister_local(NULL, &clsid);
}

HRESULT WINAPI MFTUnregisterLocal(IClassFactory *factory)
{
    TRACE("%p.\n", factory);

    return mft_unregister_local(factory, NULL);
}

MFTIME WINAPI MFGetSystemTime(void)
{
    MFTIME mf;

    GetSystemTimeAsFileTime( (FILETIME*)&mf );

    return mf;
}

static BOOL mft_is_type_info_match(struct mft_registration *mft, const GUID *category, UINT32 flags,
        IMFPluginControl *plugin_control, const MFT_REGISTER_TYPE_INFO *input_type,
        const MFT_REGISTER_TYPE_INFO *output_type)
{
    BOOL matching = TRUE;
    DWORD model;
    int i;

    if (!IsEqualGUID(category, &mft->category))
        return FALSE;

    /* Default model is synchronous. */
    model = mft->flags & (MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_ASYNCMFT | MFT_ENUM_FLAG_HARDWARE);
    if (!model)
        model = MFT_ENUM_FLAG_SYNCMFT;
    if (!(model & flags & (MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_ASYNCMFT | MFT_ENUM_FLAG_HARDWARE)))
        return FALSE;

    /* These flags should be explicitly enabled. */
    if (mft->flags & ~flags & (MFT_ENUM_FLAG_FIELDOFUSE | MFT_ENUM_FLAG_TRANSCODE_ONLY))
        return FALSE;

    if (flags & MFT_ENUM_FLAG_SORTANDFILTER && !mft->factory && plugin_control
            && IMFPluginControl_IsDisabled(plugin_control, MF_Plugin_Type_MFT, &mft->clsid) == S_OK)
    {
        return FALSE;
    }

    if (input_type)
    {
        for (i = 0, matching = FALSE; input_type && i < mft->input_types_count; ++i)
        {
            if (!memcmp(&mft->input_types[i], input_type, sizeof(*input_type)))
            {
                matching = TRUE;
                break;
            }
        }
    }

    if (output_type && matching)
    {
        for (i = 0, matching = FALSE; i < mft->output_types_count; ++i)
        {
            if (!memcmp(&mft->output_types[i], output_type, sizeof(*output_type)))
            {
                matching = TRUE;
                break;
            }
        }
    }

    return matching;
}

static void mft_get_reg_type_info(const WCHAR *clsidW, const WCHAR *typeW, MFT_REGISTER_TYPE_INFO **type,
        UINT32 *count)
{
    HKEY htransform, hfilter;
    DWORD reg_type, size;

    *type = NULL;
    *count = 0;

    if (RegOpenKeyW(HKEY_CLASSES_ROOT, transform_keyW, &htransform))
        return;

    if (RegOpenKeyW(htransform, clsidW, &hfilter))
    {
        RegCloseKey(htransform);
        return;
    }

    if (RegQueryValueExW(hfilter, typeW, NULL, &reg_type, NULL, &size))
        goto out;

    if (reg_type != REG_BINARY)
        goto out;

    if (!size || size % sizeof(**type))
        goto out;

    if (!(*type = heap_alloc(size)))
        goto out;

    *count = size / sizeof(**type);

    if (RegQueryValueExW(hfilter, typeW, NULL, &reg_type, (BYTE *)*type, &size))
    {
        heap_free(*type);
        *type = NULL;
        *count = 0;
    }

out:
    RegCloseKey(hfilter);
    RegCloseKey(htransform);
}

static void mft_get_reg_flags(const WCHAR *clsidW, const WCHAR *nameW, DWORD *flags)
{
    DWORD ret, reg_type, size;
    HKEY hroot, hmft;

    *flags = 0;

    if (RegOpenKeyW(HKEY_CLASSES_ROOT, transform_keyW, &hroot))
        return;

    ret = RegOpenKeyW(hroot, clsidW, &hmft);
    RegCloseKey(hroot);
    if (ret)
        return;

    reg_type = 0;
    if (!RegQueryValueExW(hmft, nameW, NULL, &reg_type, NULL, &size) && reg_type == REG_DWORD)
        RegQueryValueExW(hmft, nameW, NULL, &reg_type, (BYTE *)flags, &size);

    RegCloseKey(hmft);
}

static HRESULT mft_collect_machine_reg(struct list *mfts, const GUID *category, UINT32 flags,
        IMFPluginControl *plugin_control, const MFT_REGISTER_TYPE_INFO *input_type,
        const MFT_REGISTER_TYPE_INFO *output_type)
{
    struct mft_registration mft, *cur;
    HKEY hcategory, hlist;
    WCHAR clsidW[64];
    DWORD ret, size;
    int index = 0;

    if (RegOpenKeyW(HKEY_CLASSES_ROOT, categories_keyW, &hcategory))
        return E_FAIL;

    GUIDToString(clsidW, category);
    ret = RegOpenKeyW(hcategory, clsidW, &hlist);
    RegCloseKey(hcategory);
    if (ret)
        return E_FAIL;

    size = ARRAY_SIZE(clsidW);
    while (!RegEnumKeyExW(hlist, index, clsidW, &size, NULL, NULL, NULL, NULL))
    {
        memset(&mft, 0, sizeof(mft));
        mft.category = *category;
        if (!GUIDFromString(clsidW, &mft.clsid))
            goto next;

        mft_get_reg_flags(clsidW, mftflagsW, &mft.flags);

        if (output_type)
            mft_get_reg_type_info(clsidW, outputtypesW, &mft.output_types, &mft.output_types_count);

        if (input_type)
            mft_get_reg_type_info(clsidW, inputtypesW, &mft.input_types, &mft.input_types_count);

        if (!mft_is_type_info_match(&mft, category, flags, plugin_control, input_type, output_type))
        {
            heap_free(mft.input_types);
            heap_free(mft.output_types);
            goto next;
        }

        cur = heap_alloc(sizeof(*cur));
        /* Reuse allocated type arrays. */
        *cur = mft;
        list_add_tail(mfts, &cur->entry);

    next:
        size = ARRAY_SIZE(clsidW);
        index++;
    }

    return S_OK;
}

static BOOL mft_is_preferred(IMFPluginControl *plugin_control, const CLSID *clsid)
{
    CLSID preferred;
    WCHAR *selector;
    int index = 0;

    while (SUCCEEDED(IMFPluginControl_GetPreferredClsidByIndex(plugin_control, MF_Plugin_Type_MFT, index++, &selector,
            &preferred)))
    {
        CoTaskMemFree(selector);

        if (IsEqualGUID(&preferred, clsid))
            return TRUE;
    }

    return FALSE;
}

static HRESULT mft_enum(GUID category, UINT32 flags, const MFT_REGISTER_TYPE_INFO *input_type,
        const MFT_REGISTER_TYPE_INFO *output_type, IMFAttributes *attributes, IMFActivate ***activate, UINT32 *count)
{
    IMFPluginControl *plugin_control = NULL;
    struct list mfts, mfts_sorted, *result = &mfts;
    struct mft_registration *mft, *mft2;
    unsigned int obj_count;
    HRESULT hr;

    *count = 0;
    *activate = NULL;

    if (!flags)
        flags = MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_LOCALMFT | MFT_ENUM_FLAG_SORTANDFILTER;

    /* Synchronous processing is default. */
    if (!(flags & (MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_ASYNCMFT | MFT_ENUM_FLAG_HARDWARE)))
        flags |= MFT_ENUM_FLAG_SYNCMFT;

    if (FAILED(hr = MFGetPluginControl(&plugin_control)))
    {
        WARN("Failed to get plugin control instance, hr %#x.\n", hr);
        return hr;
    }

    list_init(&mfts);

    /* Collect from registry */
    mft_collect_machine_reg(&mfts, &category, flags, plugin_control, input_type, output_type);

    /* Collect locally registered ones. */
    if (flags & MFT_ENUM_FLAG_LOCALMFT)
    {
        struct mft_registration *local;

        EnterCriticalSection(&local_mfts_section);

        LIST_FOR_EACH_ENTRY(local, &local_mfts, struct mft_registration, entry)
        {
            if (mft_is_type_info_match(local, &category, flags, plugin_control, input_type, output_type))
            {
                mft = heap_alloc_zero(sizeof(*mft));

                mft->clsid = local->clsid;
                mft->factory = local->factory;
                if (mft->factory)
                    IClassFactory_AddRef(mft->factory);
                mft->flags = local->flags;
                mft->local = local->local;

                list_add_tail(&mfts, &mft->entry);
            }
        }

        LeaveCriticalSection(&local_mfts_section);
    }

    list_init(&mfts_sorted);

    if (flags & MFT_ENUM_FLAG_SORTANDFILTER)
    {
        /* Local registrations. */
        LIST_FOR_EACH_ENTRY_SAFE(mft, mft2, &mfts, struct mft_registration, entry)
        {
            if (mft->local)
            {
                list_remove(&mft->entry);
                list_add_tail(&mfts_sorted, &mft->entry);
            }
        }

        /* FIXME: Sort by merit value, for the ones that got it. Currently not handled. */

        /* Preferred transforms. */
        LIST_FOR_EACH_ENTRY_SAFE(mft, mft2, &mfts, struct mft_registration, entry)
        {
            if (!mft->factory && mft_is_preferred(plugin_control, &mft->clsid))
            {
                list_remove(&mft->entry);
                list_add_tail(&mfts_sorted, &mft->entry);
            }
        }

        /* Append the rest. */
        LIST_FOR_EACH_ENTRY_SAFE(mft, mft2, &mfts, struct mft_registration, entry)
        {
            list_remove(&mft->entry);
            list_add_tail(&mfts_sorted, &mft->entry);
        }

        result = &mfts_sorted;
    }

    IMFPluginControl_Release(plugin_control);

    /* Create activation objects from CLSID/IClassFactory. */

    obj_count = list_count(result);

    if (obj_count)
    {
        if (!(*activate = CoTaskMemAlloc(obj_count * sizeof(**activate))))
            hr = E_OUTOFMEMORY;

        obj_count = 0;

        LIST_FOR_EACH_ENTRY_SAFE(mft, mft2, result, struct mft_registration, entry)
        {
            IMFActivate *mft_activate;

            if (*activate)
            {
                if (SUCCEEDED(create_transform_activate(mft->factory, &mft_activate)))
                {
                    (*activate)[obj_count] = mft_activate;

                    if (mft->local)
                    {
                        IMFActivate_SetUINT32(mft_activate, &MFT_PROCESS_LOCAL_Attribute, 1);
                    }
                    else
                    {
                        if (mft->name)
                            IMFActivate_SetString(mft_activate, &MFT_FRIENDLY_NAME_Attribute, mft->name);
                        if (mft->input_types)
                            IMFActivate_SetBlob(mft_activate, &MFT_INPUT_TYPES_Attributes, (const UINT8 *)mft->input_types,
                                    sizeof(*mft->input_types) * mft->input_types_count);
                        if (mft->output_types)
                            IMFActivate_SetBlob(mft_activate, &MFT_OUTPUT_TYPES_Attributes, (const UINT8 *)mft->output_types,
                                    sizeof(*mft->output_types) * mft->output_types_count);
                    }

                    if (!mft->factory)
                        IMFActivate_SetGUID(mft_activate, &MFT_TRANSFORM_CLSID_Attribute, &mft->clsid);

                    IMFActivate_SetUINT32(mft_activate, &MF_TRANSFORM_FLAGS_Attribute, mft->flags);
                    IMFActivate_SetGUID(mft_activate, &MF_TRANSFORM_CATEGORY_Attribute, &mft->category);

                    obj_count++;
                }
            }

            list_remove(&mft->entry);
            release_mft_registration(mft);
        }
    }

    if (!obj_count)
    {
        CoTaskMemFree(*activate);
        *activate = NULL;
    }
    *count = obj_count;

    return hr;
}

/***********************************************************************
 *      MFTEnum (mfplat.@)
 */
HRESULT WINAPI MFTEnum(GUID category, UINT32 flags, MFT_REGISTER_TYPE_INFO *input_type,
        MFT_REGISTER_TYPE_INFO *output_type, IMFAttributes *attributes, CLSID **clsids, UINT32 *count)
{
    struct mft_registration *mft, *mft2;
    unsigned int mft_count;
    struct list mfts;
    HRESULT hr;

    TRACE("%s, %#x, %p, %p, %p, %p, %p.\n", debugstr_guid(&category), flags, input_type, output_type, attributes,
            clsids, count);

    if (!clsids || !count)
        return E_INVALIDARG;

    *count = 0;

    list_init(&mfts);

    if (FAILED(hr = mft_collect_machine_reg(&mfts, &category, MFT_ENUM_FLAG_SYNCMFT, NULL, input_type, output_type)))
        return hr;

    mft_count = list_count(&mfts);

    if (mft_count)
    {
        if (!(*clsids = CoTaskMemAlloc(mft_count * sizeof(**clsids))))
            hr = E_OUTOFMEMORY;

        mft_count = 0;
        LIST_FOR_EACH_ENTRY_SAFE(mft, mft2, &mfts, struct mft_registration, entry)
        {
            if (*clsids)
                (*clsids)[mft_count++] = mft->clsid;
            list_remove(&mft->entry);
            release_mft_registration(mft);
        }
    }

    if (!mft_count)
    {
        CoTaskMemFree(*clsids);
        *clsids = NULL;
    }
    *count = mft_count;

    return hr;
}

/***********************************************************************
 *      MFTEnumEx (mfplat.@)
 */
HRESULT WINAPI MFTEnumEx(GUID category, UINT32 flags, const MFT_REGISTER_TYPE_INFO *input_type,
        const MFT_REGISTER_TYPE_INFO *output_type, IMFActivate ***activate, UINT32 *count)
{
    TRACE("%s, %#x, %p, %p, %p, %p.\n", debugstr_guid(&category), flags, input_type, output_type, activate, count);

    return mft_enum(category, flags, input_type, output_type, NULL, activate, count);
}

/***********************************************************************
 *      MFTEnum2 (mfplat.@)
 */
HRESULT WINAPI MFTEnum2(GUID category, UINT32 flags, const MFT_REGISTER_TYPE_INFO *input_type,
        const MFT_REGISTER_TYPE_INFO *output_type, IMFAttributes *attributes, IMFActivate ***activate, UINT32 *count)
{
    TRACE("%s, %#x, %p, %p, %p, %p, %p.\n", debugstr_guid(&category), flags, input_type, output_type, attributes,
            activate, count);

    if (attributes)
        FIXME("Ignoring attributes.\n");

    return mft_enum(category, flags, input_type, output_type, attributes, activate, count);
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

    RtwqStartup();

    return S_OK;
}

/***********************************************************************
 *      MFShutdown (mfplat.@)
 */
HRESULT WINAPI MFShutdown(void)
{
    TRACE("\n");

    RtwqShutdown();

    return S_OK;
}

/***********************************************************************
 *      MFCopyImage (mfplat.@)
 */
HRESULT WINAPI MFCopyImage(BYTE *dest, LONG deststride, const BYTE *src, LONG srcstride, DWORD width, DWORD lines)
{
    TRACE("%p, %d, %p, %d, %u, %u.\n", dest, deststride, src, srcstride, width, lines);

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

static int __cdecl debug_compare_guid(const void *a, const void *b)
{
    const GUID *guid = a;
    const struct guid_def *guid_def = b;
    return memcmp(guid, guid_def->guid, sizeof(*guid));
}

const char *debugstr_attr(const GUID *guid)
{
    static const struct guid_def guid_defs[] =
    {
#define X(g) { &(g), #g }
        X(MF_READWRITE_MMCSS_CLASS),
        X(MF_TOPONODE_MARKIN_HERE),
        X(MF_MT_H264_SUPPORTED_SYNC_FRAME_TYPES),
        X(MF_TOPONODE_MARKOUT_HERE),
        X(MF_TOPONODE_DECODER),
        X(MF_TOPOLOGY_PROJECTSTART),
        X(MF_VIDEO_MAX_MB_PER_SEC),
        X(MF_TOPOLOGY_PROJECTSTOP),
        X(MF_SINK_WRITER_ENCODER_CONFIG),
        X(MF_TOPOLOGY_NO_MARKIN_MARKOUT),
        X(MF_MT_H264_CAPABILITIES),
        X(MF_SOURCE_READER_ENABLE_TRANSCODE_ONLY_TRANSFORMS),
        X(MFT_PREFERRED_ENCODER_PROFILE),
        X(MF_TOPOLOGY_DYNAMIC_CHANGE_NOT_ALLOWED),
        X(MF_MT_MPEG2_PROFILE),
        X(MF_MT_VIDEO_PROFILE),
        X(MF_MT_DV_AAUX_CTRL_PACK_1),
        X(MF_MT_ALPHA_MODE),
        X(MF_MT_MPEG2_TIMECODE),
        X(MF_PMP_SERVER_CONTEXT),
        X(MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE),
        X(MF_MEDIA_ENGINE_TRACK_ID),
        X(MF_MT_CUSTOM_VIDEO_PRIMARIES),
        X(MF_MT_TIMESTAMP_CAN_BE_DTS),
        X(MFT_CODEC_MERIT_Attribute),
        X(MF_TOPOLOGY_PLAYBACK_MAX_DIMS),
        X(MF_LOW_LATENCY),
        X(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS),
        X(MF_MT_MPEG2_FLAGS),
        X(MF_MEDIA_ENGINE_AUDIO_CATEGORY),
        X(MF_MT_PIXEL_ASPECT_RATIO),
        X(MF_TOPOLOGY_ENABLE_XVP_FOR_PLAYBACK),
        X(MFT_CONNECTED_STREAM_ATTRIBUTE),
        X(MF_MT_REALTIME_CONTENT),
        X(MF_MEDIA_ENGINE_CONTENT_PROTECTION_FLAGS),
        X(MF_MT_WRAPPED_TYPE),
        X(MF_MT_DRM_FLAGS),
        X(MF_MT_AVG_BITRATE),
        X(MF_MT_DECODER_USE_MAX_RESOLUTION),
        X(MF_MT_MAX_LUMINANCE_LEVEL),
        X(MFT_CONNECTED_TO_HW_STREAM),
        X(MF_SA_D3D_AWARE),
        X(MF_MT_MAX_KEYFRAME_SPACING),
        X(MFT_TRANSFORM_CLSID_Attribute),
        X(MFT_TRANSFORM_CLSID_Attribute),
        X(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING),
        X(MF_MT_AM_FORMAT_TYPE),
        X(MF_SESSION_APPROX_EVENT_OCCURRENCE_TIME),
        X(MF_MEDIA_ENGINE_SYNCHRONOUS_CLOSE),
        X(MF_MT_H264_MAX_MB_PER_SEC),
        X(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_MAX_BUFFERS),
        X(MF_MT_AUDIO_BLOCK_ALIGNMENT),
        X(MF_PD_PMPHOST_CONTEXT),
        X(MF_PD_APP_CONTEXT),
        X(MF_PD_DURATION),
        X(MF_PD_TOTAL_FILE_SIZE),
        X(MF_PD_AUDIO_ENCODING_BITRATE),
        X(MF_PD_VIDEO_ENCODING_BITRATE),
        X(MFSampleExtension_TargetGlobalLuminance),
        X(MF_PD_MIME_TYPE),
        X(MF_MT_H264_SUPPORTED_SLICE_MODES),
        X(MF_PD_LAST_MODIFIED_TIME),
        X(MF_PD_PLAYBACK_ELEMENT_ID),
        X(MF_MEDIA_ENGINE_BROWSER_COMPATIBILITY_MODE_IE9),
        X(MF_MT_ALL_SAMPLES_INDEPENDENT),
        X(MF_PD_PREFERRED_LANGUAGE),
        X(MF_PD_PLAYBACK_BOUNDARY_TIME),
        X(MF_MEDIA_ENGINE_TELEMETRY_APPLICATION_ID),
        X(MF_ACTIVATE_MFT_LOCKED),
        X(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT),
        X(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING),
        X(MF_MT_FRAME_SIZE),
        X(MF_MT_H264_SIMULCAST_SUPPORT),
        X(MF_SINK_WRITER_ASYNC_CALLBACK),
        X(MF_TOPOLOGY_START_TIME_ON_PRESENTATION_SWITCH),
        X(MFT_DECODER_EXPOSE_OUTPUT_TYPES_IN_NATIVE_ORDER),
        X(MF_TOPONODE_WORKQUEUE_MMCSS_PRIORITY),
        X(MF_MT_FRAME_RATE_RANGE_MAX),
        X(MF_MT_PALETTE),
        X(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_PROVIDER_DEVICE_ID),
        X(MF_TOPOLOGY_STATIC_PLAYBACK_OPTIMIZATIONS),
        X(MF_MEDIA_ENGINE_NEEDKEY_CALLBACK),
        X(MF_MT_GEOMETRIC_APERTURE),
        X(MF_MT_ORIGINAL_WAVE_FORMAT_TAG),
        X(MF_MT_DV_AAUX_SRC_PACK_1),
        X(MF_MEDIA_ENGINE_STREAM_CONTAINS_ALPHA_CHANNEL),
        X(MF_MEDIA_ENGINE_MEDIA_PLAYER_MODE),
        X(MF_MEDIA_ENGINE_EXTENSION),
        X(MF_MT_DEFAULT_STRIDE),
        X(MF_MT_ARBITRARY_FORMAT),
        X(MF_TRANSFORM_CATEGORY_Attribute),
        X(MF_MT_MPEG2_HDCP),
        X(MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND),
        X(MF_MT_SPATIAL_AUDIO_MAX_DYNAMIC_OBJECTS),
        X(MF_MT_DECODER_MAX_DPB_COUNT),
        X(MFSampleExtension_ForwardedDecodeUnits),
        X(MF_MT_DV_AAUX_CTRL_PACK_0),
        X(MF_MT_YUV_MATRIX),
        X(MF_EVENT_SOURCE_TOPOLOGY_CANCELED),
        X(MF_MT_MPEG4_CURRENT_SAMPLE_ENTRY),
        X(MF_MT_MAX_FRAME_AVERAGE_LUMINANCE_LEVEL),
        X(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_ENDPOINT_ID),
        X(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME),
        X(MF_MT_VIDEO_ROTATION),
        X(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_SYMBOLIC_LINK),
        X(MF_MEDIA_ENGINE_BROWSER_COMPATIBILITY_MODE_IE11),
        X(MF_MT_USER_DATA),
        X(MF_ACTIVATE_CUSTOM_VIDEO_MIXER_CLSID),
        X(MF_MT_MIN_MASTERING_LUMINANCE),
        X(MF_ACTIVATE_CUSTOM_VIDEO_MIXER_ACTIVATE),
        X(MF_ACTIVATE_CUSTOM_VIDEO_MIXER_FLAGS),
        X(MF_ACTIVATE_CUSTOM_VIDEO_PRESENTER_CLSID),
        X(MF_EVENT_STREAM_METADATA_SYSTEMID),
        X(MF_ACTIVATE_CUSTOM_VIDEO_PRESENTER_ACTIVATE),
        X(MF_MT_AUDIO_CHANNEL_MASK),
        X(MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN),
        X(MF_READWRITE_DISABLE_CONVERTERS),
        X(MF_MEDIA_ENGINE_BROWSER_COMPATIBILITY_MODE_IE_EDGE),
        X(MF_ACTIVATE_CUSTOM_VIDEO_PRESENTER_FLAGS),
        X(MF_MT_MINIMUM_DISPLAY_APERTURE),
        X(MFSampleExtension_Token),
        X(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_CATEGORY),
        X(MF_MT_AUDIO_VALID_BITS_PER_SAMPLE),
        X(MF_TRANSFORM_ASYNC_UNLOCK),
        X(MF_DISABLE_FRAME_CORRUPTION_INFO),
        X(MF_TOPOLOGY_ENUMERATE_SOURCE_TYPES),
        X(MF_MT_VIDEO_NO_FRAME_ORDERING),
        X(MF_MEDIA_ENGINE_PLAYBACK_VISUAL),
        X(MF_MT_VIDEO_CHROMA_SITING),
        X(MF_AUDIO_RENDERER_ATTRIBUTE_STREAM_CATEGORY),
        X(MFSampleExtension_3DVideo_SampleFormat),
        X(MF_MT_H264_RESOLUTION_SCALING),
        X(MF_MT_VIDEO_LEVEL),
        X(MF_MT_MPEG2_LEVEL),
        X(MF_SAMPLEGRABBERSINK_SAMPLE_TIME_OFFSET),
        X(MF_MT_SAMPLE_SIZE),
        X(MF_MT_AAC_PAYLOAD_TYPE),
        X(MF_TOPOLOGY_PLAYBACK_FRAMERATE),
        X(MF_SOURCE_READER_D3D11_BIND_FLAGS),
        X(MF_MT_AUDIO_FOLDDOWN_MATRIX),
        X(MF_MT_AUDIO_WMADRC_PEAKREF),
        X(MF_MT_AUDIO_WMADRC_PEAKTARGET),
        X(MF_TRANSFORM_FLAGS_Attribute),
        X(MF_MT_H264_SUPPORTED_RATE_CONTROL_MODES),
        X(MF_PD_SAMI_STYLELIST),
        X(MF_MT_AUDIO_WMADRC_AVGREF),
        X(MF_MT_AUDIO_BITS_PER_SAMPLE),
        X(MF_SD_LANGUAGE),
        X(MF_MT_AUDIO_WMADRC_AVGTARGET),
        X(MF_SD_PROTECTED),
        X(MF_SESSION_TOPOLOADER),
        X(MF_SESSION_GLOBAL_TIME),
        X(MF_SESSION_QUALITY_MANAGER),
        X(MF_SESSION_CONTENT_PROTECTION_MANAGER),
        X(MF_MT_MPEG4_SAMPLE_DESCRIPTION),
        X(MF_MT_MPEG_START_TIME_CODE),
        X(MFT_REMUX_MARK_I_PICTURE_AS_CLEAN_POINT),
        X(MFT_REMUX_MARK_I_PICTURE_AS_CLEAN_POINT),
        X(MF_READWRITE_MMCSS_PRIORITY_AUDIO),
        X(MF_MT_H264_MAX_CODEC_CONFIG_DELAY),
        X(MF_MT_DV_AAUX_SRC_PACK_0),
        X(MF_BYTESTREAM_ORIGIN_NAME),
        X(MF_BYTESTREAM_CONTENT_TYPE),
        X(MF_MT_DEPTH_MEASUREMENT),
        X(MF_MEDIA_ENGINE_COMPATIBILITY_MODE_WIN10),
        X(MF_MT_VIDEO_3D_NUM_VIEWS),
        X(MF_BYTESTREAM_DURATION),
        X(MF_SD_SAMI_LANGUAGE),
        X(MF_EVENT_OUTPUT_NODE),
        X(MF_BYTESTREAM_LAST_MODIFIED_TIME),
        X(MFT_ENUM_ADAPTER_LUID),
        X(MF_MT_FRAME_RATE_RANGE_MIN),
        X(MF_BYTESTREAM_IFO_FILE_URI),
        X(MF_EVENT_TOPOLOGY_STATUS),
        X(MF_BYTESTREAM_DLNA_PROFILE_ID),
        X(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_ROLE),
        X(MF_MT_MAJOR_TYPE),
        X(MF_MT_IN_BAND_PARAMETER_SET),
        X(MF_EVENT_SOURCE_CHARACTERISTICS),
        X(MF_EVENT_SOURCE_CHARACTERISTICS_OLD),
        X(MF_SESSION_SERVER_CONTEXT),
        X(MF_MT_VIDEO_3D_FIRST_IS_LEFT),
        X(MFT_DECODER_FINAL_VIDEO_RESOLUTION_HINT),
        X(MF_PD_ADAPTIVE_STREAMING),
        X(MF_MEDIA_ENGINE_SOURCE_RESOLVER_CONFIG_STORE),
        X(MF_MEDIA_ENGINE_COMPATIBILITY_MODE_WWA_EDGE),
        X(MF_MT_H264_SUPPORTED_USAGES),
        X(MFT_PREFERRED_OUTPUTTYPE_Attribute),
        X(MFSampleExtension_Timestamp),
        X(MF_TOPONODE_PRIMARYOUTPUT),
        X(MF_MT_SUBTYPE),
        X(MF_TRANSFORM_ASYNC),
        X(MF_TOPONODE_STREAMID),
        X(MF_MEDIA_ENGINE_PLAYBACK_HWND),
        X(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE),
        X(MF_MT_VIDEO_LIGHTING),
        X(MF_SD_MUTUALLY_EXCLUSIVE),
        X(MF_SD_STREAM_NAME),
        X(MF_MT_DV_VAUX_SRC_PACK),
        X(MF_TOPONODE_RATELESS),
        X(MF_EVENT_STREAM_METADATA_CONTENT_KEYIDS),
        X(MF_TOPONODE_DISABLE_PREROLL),
        X(MF_MT_VIDEO_3D_FORMAT),
        X(MF_EVENT_STREAM_METADATA_KEYDATA),
        X(MF_SOURCE_READER_D3D_MANAGER),
        X(MF_SINK_WRITER_D3D_MANAGER),
        X(MFSampleExtension_3DVideo),
        X(MF_MT_H264_USAGE),
        X(MF_MEDIA_ENGINE_EME_CALLBACK),
        X(MF_EVENT_SOURCE_FAKE_START),
        X(MF_EVENT_SOURCE_PROJECTSTART),
        X(MF_EVENT_SOURCE_ACTUAL_START),
        X(MF_MEDIA_ENGINE_CONTENT_PROTECTION_MANAGER),
        X(MF_MT_AUDIO_SAMPLES_PER_BLOCK),
        X(MFT_ENUM_HARDWARE_URL_Attribute),
        X(MF_SOURCE_READER_ASYNC_CALLBACK),
        X(MF_MT_OUTPUT_BUFFER_NUM),
        X(MFT_ENCODER_SUPPORTS_CONFIG_EVENT),
        X(MF_MT_AUDIO_FLAC_MAX_BLOCK_SIZE),
        X(MFT_FRIENDLY_NAME_Attribute),
        X(MF_MT_FIXED_SIZE_SAMPLES),
        X(MFT_SUPPORT_3DVIDEO),
        X(MFT_SUPPORT_3DVIDEO),
        X(MFT_INPUT_TYPES_Attributes),
        X(MF_MT_H264_LAYOUT_PER_STREAM),
        X(MF_EVENT_SCRUBSAMPLE_TIME),
        X(MF_MT_SPATIAL_AUDIO_MAX_METADATA_ITEMS),
        X(MF_MT_MPEG2_ONE_FRAME_PER_PACKET),
        X(MF_MT_INTERLACE_MODE),
        X(MF_MEDIA_ENGINE_CALLBACK),
        X(MF_MT_VIDEO_RENDERER_EXTENSION_PROFILE),
        X(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_HW_SOURCE),
        X(MF_MT_AUDIO_PREFER_WAVEFORMATEX),
        X(MF_MT_H264_SVC_CAPABILITIES),
        X(MF_TOPONODE_WORKQUEUE_ITEM_PRIORITY),
        X(MF_MT_SPATIAL_AUDIO_OBJECT_METADATA_LENGTH),
        X(MF_MT_SPATIAL_AUDIO_OBJECT_METADATA_FORMAT_ID),
        X(MF_SAMPLEGRABBERSINK_IGNORE_CLOCK),
        X(MF_MT_PAN_SCAN_ENABLED),
        X(MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ID),
        X(MF_MT_DV_VAUX_CTRL_PACK),
        X(MFSampleExtension_ForwardedDecodeUnitType),
        X(MF_MT_AUDIO_AVG_BYTES_PER_SECOND),
        X(MF_SOURCE_READER_MEDIASOURCE_CHARACTERISTICS),
        X(MF_MT_SPATIAL_AUDIO_MIN_METADATA_ITEM_OFFSET_SPACING),
        X(MF_TOPONODE_TRANSFORM_OBJECTID),
        X(MF_DEVSOURCE_ATTRIBUTE_MEDIA_TYPE),
        X(MF_EVENT_MFT_INPUT_STREAM_ID),
        X(MF_MT_SOURCE_CONTENT_HINT),
        X(MFT_ENUM_HARDWARE_VENDOR_ID_Attribute),
        X(MFT_ENUM_TRANSCODE_ONLY_ATTRIBUTE),
        X(MF_READWRITE_MMCSS_PRIORITY),
        X(MF_MT_VIDEO_3D),
        X(MF_EVENT_START_PRESENTATION_TIME),
        X(MF_EVENT_SESSIONCAPS),
        X(MF_EVENT_PRESENTATION_TIME_OFFSET),
        X(MF_MEDIA_ENGINE_AUDIO_ENDPOINT_ROLE),
        X(MF_EVENT_SESSIONCAPS_DELTA),
        X(MF_EVENT_START_PRESENTATION_TIME_AT_OUTPUT),
        X(MFSampleExtension_DecodeTimestamp),
        X(MF_MEDIA_ENGINE_COMPATIBILITY_MODE),
        X(MF_MT_VIDEO_H264_NO_FMOASO),
        X(MF_MT_AVG_BIT_ERROR_RATE),
        X(MF_MT_VIDEO_PRIMARIES),
        X(MF_SINK_WRITER_DISABLE_THROTTLING),
        X(MF_MT_H264_RATE_CONTROL_MODES),
        X(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK),
        X(MF_READWRITE_D3D_OPTIONAL),
        X(MF_MEDIA_ENGINE_DXGI_MANAGER),
        X(MF_READWRITE_MMCSS_CLASS_AUDIO),
        X(MF_MEDIA_ENGINE_COREWINDOW),
        X(MF_SOURCE_READER_DISABLE_CAMERA_PLUGINS),
        X(MF_MT_MPEG4_TRACK_TYPE),
        X(MF_ACTIVATE_VIDEO_WINDOW),
        X(MF_MT_PAN_SCAN_APERTURE),
        X(MF_TOPOLOGY_RESOLUTION_STATUS),
        X(MF_MT_ORIGINAL_4CC),
        X(MF_PD_AUDIO_ISVARIABLEBITRATE),
        X(MF_AUDIO_RENDERER_ATTRIBUTE_FLAGS),
        X(MF_MEDIA_ENGINE_BROWSER_COMPATIBILITY_MODE),
        X(MF_AUDIO_RENDERER_ATTRIBUTE_SESSION_ID),
        X(MF_MT_MPEG2_CONTENT_PACKET),
        X(MFT_PROCESS_LOCAL_Attribute),
        X(MFT_PROCESS_LOCAL_Attribute),
        X(MF_MT_PAD_CONTROL_FLAGS),
        X(MF_MT_VIDEO_NOMINAL_RANGE),
        X(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION),
        X(MF_MT_MPEG_SEQUENCE_HEADER),
        X(MF_MEDIA_ENGINE_OPM_HWND),
        X(MF_MT_AUDIO_SAMPLES_PER_SECOND),
        X(MF_MT_SPATIAL_AUDIO_DATA_PRESENT),
        X(MF_MT_FRAME_RATE),
        X(MF_TOPONODE_FLUSH),
        X(MF_MT_MPEG2_STANDARD),
        X(MF_TOPONODE_DRAIN),
        X(MF_MT_TRANSFER_FUNCTION),
        X(MF_TOPONODE_MEDIASTART),
        X(MF_TOPONODE_MEDIASTOP),
        X(MF_SOURCE_READER_MEDIASOURCE_CONFIG),
        X(MF_TOPONODE_SOURCE),
        X(MF_TOPONODE_PRESENTATION_DESCRIPTOR),
        X(MF_TOPONODE_D3DAWARE),
        X(MF_MT_COMPRESSED),
        X(MF_TOPONODE_STREAM_DESCRIPTOR),
        X(MF_TOPONODE_ERRORCODE),
        X(MF_TOPONODE_SEQUENCE_ELEMENTID),
        X(MF_EVENT_MFT_CONTEXT),
        X(MF_MT_FORWARD_CUSTOM_SEI),
        X(MF_TOPONODE_CONNECT_METHOD),
        X(MFT_OUTPUT_TYPES_Attributes),
        X(MF_MT_IMAGE_LOSS_TOLERANT),
        X(MF_SESSION_REMOTE_SOURCE_MODE),
        X(MF_MT_DEPTH_VALUE_UNIT),
        X(MF_MT_AUDIO_NUM_CHANNELS),
        X(MF_MT_ARBITRARY_HEADER),
        X(MF_TOPOLOGY_DXVA_MODE),
        X(MF_TOPONODE_LOCKED),
        X(MF_TOPONODE_WORKQUEUE_ID),
        X(MF_MEDIA_ENGINE_CONTINUE_ON_CODEC_ERROR),
        X(MF_TOPONODE_WORKQUEUE_MMCSS_CLASS),
        X(MF_TOPONODE_DECRYPTOR),
        X(MF_EVENT_DO_THINNING),
        X(MF_TOPONODE_DISCARDABLE),
        X(MF_TOPOLOGY_HARDWARE_MODE),
        X(MF_SOURCE_READER_DISABLE_DXVA),
        X(MF_MT_FORWARD_CUSTOM_NALU),
        X(MF_MEDIA_ENGINE_BROWSER_COMPATIBILITY_MODE_IE10),
        X(MF_TOPONODE_ERROR_MAJORTYPE),
        X(MF_MT_SECURE),
        X(MFT_FIELDOFUSE_UNLOCK_Attribute),
        X(MF_TOPONODE_ERROR_SUBTYPE),
        X(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE),
        X(MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ROLE),
        X(MF_MT_VIDEO_3D_LEFT_IS_BASE),
        X(MF_TOPONODE_WORKQUEUE_MMCSS_TASKID),
#undef X
    };
    struct guid_def *ret = NULL;

    if (guid)
        ret = bsearch(guid, guid_defs, ARRAY_SIZE(guid_defs), sizeof(*guid_defs), debug_compare_guid);

    return ret ? wine_dbg_sprintf("%s", ret->name) : wine_dbgstr_guid(guid);
}

const char *debugstr_mf_guid(const GUID *guid)
{
    static const struct guid_def guid_defs[] =
    {
#define X(g) { &(g), #g }
        X(MFAudioFormat_ADTS),
        X(MFAudioFormat_PCM),
        X(MFAudioFormat_PCM_HDCP),
        X(MFAudioFormat_Float),
        X(MFAudioFormat_DTS),
        X(MFAudioFormat_DRM),
        X(MFAudioFormat_MSP1),
        X(MFAudioFormat_Vorbis),
        X(MFAudioFormat_AAC),
        X(MFVideoFormat_RGB24),
        X(MFVideoFormat_ARGB32),
        X(MFVideoFormat_RGB32),
        X(MFVideoFormat_RGB565),
        X(MFVideoFormat_RGB555),
        X(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID),
        X(MFVideoFormat_A2R10G10B10),
        X(MFMediaType_Script),
        X(MFMediaType_Image),
        X(MFMediaType_HTML),
        X(MFMediaType_Binary),
        X(MFVideoFormat_MPEG2),
        X(MFMediaType_FileTransfer),
        X(MFVideoFormat_RGB8),
        X(MFAudioFormat_Dolby_AC3),
        X(MFVideoFormat_L8),
        X(MFAudioFormat_LPCM),
        X(MFVideoFormat_420O),
        X(MFVideoFormat_AI44),
        X(MFVideoFormat_AV1),
        X(MFVideoFormat_AYUV),
        X(MFVideoFormat_H263),
        X(MFVideoFormat_H264),
        X(MFVideoFormat_H265),
        X(MFVideoFormat_HEVC),
        X(MFVideoFormat_HEVC_ES),
        X(MFVideoFormat_I420),
        X(MFVideoFormat_IYUV),
        X(MFVideoFormat_M4S2),
        X(MFVideoFormat_MJPG),
        X(MFVideoFormat_MP43),
        X(MFVideoFormat_MP4S),
        X(MFVideoFormat_MP4V),
        X(MFVideoFormat_MPG1),
        X(MFVideoFormat_MSS1),
        X(MFVideoFormat_MSS2),
        X(MFVideoFormat_NV11),
        X(MFVideoFormat_NV12),
        X(MFVideoFormat_ORAW),
        X(MFAudioFormat_Opus),
        X(MFVideoFormat_D16),
        X(MFAudioFormat_MPEG),
        X(MFVideoFormat_P010),
        X(MFVideoFormat_P016),
        X(MFVideoFormat_P210),
        X(MFVideoFormat_P216),
        X(MFVideoFormat_L16),
        X(MFAudioFormat_MP3),
        X(MFVideoFormat_UYVY),
        X(MFVideoFormat_VP10),
        X(MFVideoFormat_VP80),
        X(MFVideoFormat_VP90),
        X(MFVideoFormat_WMV1),
        X(MFVideoFormat_WMV2),
        X(MFVideoFormat_WMV3),
        X(MFVideoFormat_WVC1),
        X(MFVideoFormat_Y210),
        X(MFVideoFormat_Y216),
        X(MFVideoFormat_Y410),
        X(MFVideoFormat_Y416),
        X(MFVideoFormat_Y41P),
        X(MFVideoFormat_Y41T),
        X(MFVideoFormat_Y42T),
        X(MFVideoFormat_YUY2),
        X(MFVideoFormat_YV12),
        X(MFVideoFormat_YVU9),
        X(MFVideoFormat_YVYU),
        X(MFAudioFormat_WMAudioV8),
        X(MFAudioFormat_ALAC),
        X(MFAudioFormat_AMR_NB),
        X(MFMediaType_Audio),
        X(MFAudioFormat_WMAudioV9),
        X(MFAudioFormat_AMR_WB),
        X(MFAudioFormat_WMAudio_Lossless),
        X(MFAudioFormat_AMR_WP),
        X(MFAudioFormat_WMASPDIF),
        X(MFVideoFormat_DV25),
        X(MFVideoFormat_DV50),
        X(MFVideoFormat_DVC),
        X(MFVideoFormat_DVH1),
        X(MFVideoFormat_DVHD),
        X(MFVideoFormat_DVSD),
        X(MFVideoFormat_DVSL),
        X(MFVideoFormat_A16B16G16R16F),
        X(MFVideoFormat_v210),
        X(MFVideoFormat_v216),
        X(MFVideoFormat_v410),
        X(MFMediaType_Video),
        X(MFAudioFormat_AAC_HDCP),
        X(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID),
        X(MFAudioFormat_Dolby_AC3_HDCP),
        X(MFMediaType_Subtitle),
        X(MFMediaType_Stream),
        X(MFAudioFormat_Dolby_AC3_SPDIF),
        X(MFAudioFormat_Float_SpatialObjects),
        X(MFMediaType_SAMI),
        X(MFAudioFormat_ADTS_HDCP),
        X(MFAudioFormat_FLAC),
        X(MFAudioFormat_Dolby_DDPlus),
        X(MFMediaType_MultiplexedFrames),
        X(MFAudioFormat_Base_HDCP),
        X(MFVideoFormat_Base_HDCP),
        X(MFVideoFormat_H264_HDCP),
        X(MFVideoFormat_HEVC_HDCP),
        X(MFMediaType_Default),
        X(MFMediaType_Protected),
        X(MFVideoFormat_H264_ES),
        X(MFMediaType_Perception),
#undef X
    };
    struct guid_def *ret = NULL;

    if (guid)
        ret = bsearch(guid, guid_defs, ARRAY_SIZE(guid_defs), sizeof(*guid_defs), debug_compare_guid);

    return ret ? wine_dbg_sprintf("%s", ret->name) : wine_dbgstr_guid(guid);
}

struct event_id
{
    DWORD id;
    const char *name;
};

static int __cdecl debug_event_id(const void *a, const void *b)
{
    const DWORD *id = a;
    const struct event_id *event_id = b;
    return *id - event_id->id;
}

static const char *debugstr_eventid(DWORD event)
{
    static const struct event_id
    {
        DWORD id;
        const char *name;
    }
    event_ids[] =
    {
#define X(e) { e, #e }
        X(MEUnknown),
        X(MEError),
        X(MEExtendedType),
        X(MENonFatalError),
        X(MESessionUnknown),
        X(MESessionTopologySet),
        X(MESessionTopologiesCleared),
        X(MESessionStarted),
        X(MESessionPaused),
        X(MESessionStopped),
        X(MESessionClosed),
        X(MESessionEnded),
        X(MESessionRateChanged),
        X(MESessionScrubSampleComplete),
        X(MESessionCapabilitiesChanged),
        X(MESessionTopologyStatus),
        X(MESessionNotifyPresentationTime),
        X(MENewPresentation),
        X(MELicenseAcquisitionStart),
        X(MELicenseAcquisitionCompleted),
        X(MEIndividualizationStart),
        X(MEIndividualizationCompleted),
        X(MEEnablerProgress),
        X(MEEnablerCompleted),
        X(MEPolicyError),
        X(MEPolicyReport),
        X(MEBufferingStarted),
        X(MEBufferingStopped),
        X(MEConnectStart),
        X(MEConnectEnd),
        X(MEReconnectStart),
        X(MEReconnectEnd),
        X(MERendererEvent),
        X(MESessionStreamSinkFormatChanged),
        X(MESourceUnknown),
        X(MESourceStarted),
        X(MEStreamStarted),
        X(MESourceSeeked),
        X(MEStreamSeeked),
        X(MENewStream),
        X(MEUpdatedStream),
        X(MESourceStopped),
        X(MEStreamStopped),
        X(MESourcePaused),
        X(MEStreamPaused),
        X(MEEndOfPresentation),
        X(MEEndOfStream),
        X(MEMediaSample),
        X(MEStreamTick),
        X(MEStreamThinMode),
        X(MEStreamFormatChanged),
        X(MESourceRateChanged),
        X(MEEndOfPresentationSegment),
        X(MESourceCharacteristicsChanged),
        X(MESourceRateChangeRequested),
        X(MESourceMetadataChanged),
        X(MESequencerSourceTopologyUpdated),
        X(MESinkUnknown),
        X(MEStreamSinkStarted),
        X(MEStreamSinkStopped),
        X(MEStreamSinkPaused),
        X(MEStreamSinkRateChanged),
        X(MEStreamSinkRequestSample),
        X(MEStreamSinkMarker),
        X(MEStreamSinkPrerolled),
        X(MEStreamSinkScrubSampleComplete),
        X(MEStreamSinkFormatChanged),
        X(MEStreamSinkDeviceChanged),
        X(MEQualityNotify),
        X(MESinkInvalidated),
        X(MEAudioSessionNameChanged),
        X(MEAudioSessionVolumeChanged),
        X(MEAudioSessionDeviceRemoved),
        X(MEAudioSessionServerShutdown),
        X(MEAudioSessionGroupingParamChanged),
        X(MEAudioSessionIconChanged),
        X(MEAudioSessionFormatChanged),
        X(MEAudioSessionDisconnected),
        X(MEAudioSessionExclusiveModeOverride),
        X(METrustUnknown),
        X(MEPolicyChanged),
        X(MEContentProtectionMessage),
        X(MEPolicySet),
        X(MEWMDRMLicenseBackupCompleted),
        X(MEWMDRMLicenseBackupProgress),
        X(MEWMDRMLicenseRestoreCompleted),
        X(MEWMDRMLicenseRestoreProgress),
        X(MEWMDRMLicenseAcquisitionCompleted),
        X(MEWMDRMIndividualizationCompleted),
        X(MEWMDRMIndividualizationProgress),
        X(MEWMDRMProximityCompleted),
        X(MEWMDRMLicenseStoreCleaned),
        X(MEWMDRMRevocationDownloadCompleted),
        X(METransformUnknown),
        X(METransformNeedInput),
        X(METransformHaveOutput),
        X(METransformDrainComplete),
        X(METransformMarker),
        X(METransformInputStreamStateChanged),
        X(MEByteStreamCharacteristicsChanged),
        X(MEVideoCaptureDeviceRemoved),
        X(MEVideoCaptureDevicePreempted),
        X(MEStreamSinkFormatInvalidated),
        X(MEEncodingParameters),
        X(MEContentProtectionMetadata),
        X(MEDeviceThermalStateChanged),
#undef X
    };

    struct event_id *ret = bsearch(&event, event_ids, ARRAY_SIZE(event_ids), sizeof(*event_ids), debug_event_id);
    return ret ? wine_dbg_sprintf("%s", ret->name) : wine_dbg_sprintf("%u", event);
}

static inline struct attributes *impl_from_IMFAttributes(IMFAttributes *iface)
{
    return CONTAINING_RECORD(iface, struct attributes, IMFAttributes_iface);
}

static HRESULT WINAPI mfattributes_QueryInterface(IMFAttributes *iface, REFIID riid, void **out)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFAttributes) ||
            IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFAttributes_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI mfattributes_AddRef(IMFAttributes *iface)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);
    ULONG refcount = InterlockedIncrement(&attributes->ref);

    TRACE("%p, refcount %d.\n", iface, refcount);

    return refcount;
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

HRESULT attributes_GetItem(struct attributes *attributes, REFGUID key, PROPVARIANT *value)
{
    struct attribute *attribute;
    HRESULT hr;

    EnterCriticalSection(&attributes->cs);

    if ((attribute = attributes_find_item(attributes, key, NULL)))
        hr = value ? PropVariantCopy(value, &attribute->value) : S_OK;
    else
        hr = MF_E_ATTRIBUTENOTFOUND;

    LeaveCriticalSection(&attributes->cs);

    return hr;
}

HRESULT attributes_GetItemType(struct attributes *attributes, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct attribute *attribute;
    HRESULT hr = S_OK;

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

HRESULT attributes_CompareItem(struct attributes *attributes, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    struct attribute *attribute;

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

HRESULT attributes_Compare(struct attributes *attributes, IMFAttributes *theirs,
        MF_ATTRIBUTES_MATCH_TYPE match_type, BOOL *ret)
{
    IMFAttributes *smaller, *other;
    MF_ATTRIBUTE_TYPE type;
    HRESULT hr = S_OK;
    UINT32 count;
    BOOL result;
    size_t i;

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
            hr = IMFAttributes_Compare(theirs, &attributes->IMFAttributes_iface, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result);
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
            smaller = attributes->count > count ? theirs : &attributes->IMFAttributes_iface;
            other = attributes->count > count ? &attributes->IMFAttributes_iface : theirs;
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

HRESULT attributes_GetUINT32(struct attributes *attributes, REFGUID key, UINT32 *value)
{
    PROPVARIANT attrval;
    HRESULT hr;

    PropVariantInit(&attrval);
    attrval.vt = VT_UI4;
    hr = attributes_get_item(attributes, key, &attrval);
    if (SUCCEEDED(hr))
        *value = attrval.u.ulVal;

    return hr;
}

HRESULT attributes_GetUINT64(struct attributes *attributes, REFGUID key, UINT64 *value)
{
    PROPVARIANT attrval;
    HRESULT hr;

    PropVariantInit(&attrval);
    attrval.vt = VT_UI8;
    hr = attributes_get_item(attributes, key, &attrval);
    if (SUCCEEDED(hr))
        *value = attrval.u.uhVal.QuadPart;

    return hr;
}

HRESULT attributes_GetDouble(struct attributes *attributes, REFGUID key, double *value)
{
    PROPVARIANT attrval;
    HRESULT hr;

    PropVariantInit(&attrval);
    attrval.vt = VT_R8;
    hr = attributes_get_item(attributes, key, &attrval);
    if (SUCCEEDED(hr))
        *value = attrval.u.dblVal;

    return hr;
}

HRESULT attributes_GetGUID(struct attributes *attributes, REFGUID key, GUID *value)
{
    struct attribute *attribute;
    HRESULT hr = S_OK;

    EnterCriticalSection(&attributes->cs);

    attribute = attributes_find_item(attributes, key, NULL);
    if (attribute)
    {
        if (attribute->value.vt == MF_ATTRIBUTE_GUID)
            *value = *attribute->value.u.puuid;
        else
            hr = MF_E_INVALIDTYPE;
    }
    else
        hr = MF_E_ATTRIBUTENOTFOUND;

    LeaveCriticalSection(&attributes->cs);

    return hr;
}

HRESULT attributes_GetStringLength(struct attributes *attributes, REFGUID key, UINT32 *length)
{
    struct attribute *attribute;
    HRESULT hr = S_OK;

    EnterCriticalSection(&attributes->cs);

    attribute = attributes_find_item(attributes, key, NULL);
    if (attribute)
    {
        if (attribute->value.vt == MF_ATTRIBUTE_STRING)
            *length = lstrlenW(attribute->value.u.pwszVal);
        else
            hr = MF_E_INVALIDTYPE;
    }
    else
        hr = MF_E_ATTRIBUTENOTFOUND;

    LeaveCriticalSection(&attributes->cs);

    return hr;
}

HRESULT attributes_GetString(struct attributes *attributes, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct attribute *attribute;
    HRESULT hr = S_OK;

    EnterCriticalSection(&attributes->cs);

    attribute = attributes_find_item(attributes, key, NULL);
    if (attribute)
    {
        if (attribute->value.vt == MF_ATTRIBUTE_STRING)
        {
            int len = lstrlenW(attribute->value.u.pwszVal);

            if (length)
                *length = len;

            if (size <= len)
                hr = STRSAFE_E_INSUFFICIENT_BUFFER;
            else
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

HRESULT attributes_GetAllocatedString(struct attributes *attributes, REFGUID key, WCHAR **value, UINT32 *length)
{
    PROPVARIANT attrval;
    HRESULT hr;

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

HRESULT attributes_GetBlobSize(struct attributes *attributes, REFGUID key, UINT32 *size)
{
    struct attribute *attribute;
    HRESULT hr = S_OK;

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

HRESULT attributes_GetBlob(struct attributes *attributes, REFGUID key, UINT8 *buf, UINT32 bufsize, UINT32 *blobsize)
{
    struct attribute *attribute;
    HRESULT hr;

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

HRESULT attributes_GetAllocatedBlob(struct attributes *attributes, REFGUID key, UINT8 **buf, UINT32 *size)
{
    PROPVARIANT attrval;
    HRESULT hr;

    attrval.vt = VT_VECTOR | VT_UI1;
    hr = attributes_get_item(attributes, key, &attrval);
    if (SUCCEEDED(hr))
    {
        *buf = attrval.u.caub.pElems;
        *size = attrval.u.caub.cElems;
    }

    return hr;
}

HRESULT attributes_GetUnknown(struct attributes *attributes, REFGUID key, REFIID riid, void **out)
{
    PROPVARIANT attrval;
    HRESULT hr;

    PropVariantInit(&attrval);
    attrval.vt = VT_UNKNOWN;
    hr = attributes_get_item(attributes, key, &attrval);
    if (SUCCEEDED(hr))
        hr = IUnknown_QueryInterface(attrval.u.punkVal, riid, out);
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

HRESULT attributes_SetItem(struct attributes *attributes, REFGUID key, REFPROPVARIANT value)
{
    PROPVARIANT empty;

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

HRESULT attributes_DeleteItem(struct attributes *attributes, REFGUID key)
{
    struct attribute *attribute;
    size_t index = 0;

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

HRESULT attributes_DeleteAllItems(struct attributes *attributes)
{
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

HRESULT attributes_SetUINT32(struct attributes *attributes, REFGUID key, UINT32 value)
{
    PROPVARIANT attrval;

    attrval.vt = VT_UI4;
    attrval.u.ulVal = value;
    return attributes_set_item(attributes, key, &attrval);
}

HRESULT attributes_SetUINT64(struct attributes *attributes, REFGUID key, UINT64 value)
{
    PROPVARIANT attrval;

    attrval.vt = VT_UI8;
    attrval.u.uhVal.QuadPart = value;
    return attributes_set_item(attributes, key, &attrval);
}

HRESULT attributes_SetDouble(struct attributes *attributes, REFGUID key, double value)
{
    PROPVARIANT attrval;

    attrval.vt = VT_R8;
    attrval.u.dblVal = value;
    return attributes_set_item(attributes, key, &attrval);
}

HRESULT attributes_SetGUID(struct attributes *attributes, REFGUID key, REFGUID value)
{
    PROPVARIANT attrval;

    attrval.vt = VT_CLSID;
    attrval.u.puuid = (CLSID *)value;
    return attributes_set_item(attributes, key, &attrval);
}

HRESULT attributes_SetString(struct attributes *attributes, REFGUID key, const WCHAR *value)
{
    PROPVARIANT attrval;

    attrval.vt = VT_LPWSTR;
    attrval.u.pwszVal = (WCHAR *)value;
    return attributes_set_item(attributes, key, &attrval);
}

HRESULT attributes_SetBlob(struct attributes *attributes, REFGUID key, const UINT8 *buf, UINT32 size)
{
    PROPVARIANT attrval;

    attrval.vt = VT_VECTOR | VT_UI1;
    attrval.u.caub.cElems = size;
    attrval.u.caub.pElems = (UINT8 *)buf;
    return attributes_set_item(attributes, key, &attrval);
}

HRESULT attributes_SetUnknown(struct attributes *attributes, REFGUID key, IUnknown *unknown)
{
    PROPVARIANT attrval;

    attrval.vt = VT_UNKNOWN;
    attrval.u.punkVal = unknown;
    return attributes_set_item(attributes, key, &attrval);
}

HRESULT attributes_LockStore(struct attributes *attributes)
{
    EnterCriticalSection(&attributes->cs);

    return S_OK;
}

HRESULT attributes_UnlockStore(struct attributes *attributes)
{
    LeaveCriticalSection(&attributes->cs);

    return S_OK;
}

HRESULT attributes_GetCount(struct attributes *attributes, UINT32 *count)
{
    EnterCriticalSection(&attributes->cs);
    *count = attributes->count;
    LeaveCriticalSection(&attributes->cs);

    return S_OK;
}

HRESULT attributes_GetItemByIndex(struct attributes *attributes, UINT32 index, GUID *key, PROPVARIANT *value)
{
    HRESULT hr = S_OK;

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

HRESULT attributes_CopyAllItems(struct attributes *attributes, IMFAttributes *dest)
{
    HRESULT hr = S_OK;
    size_t i;

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

static HRESULT WINAPI mfattributes_GetItem(IMFAttributes *iface, REFGUID key, PROPVARIANT *value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetItem(attributes, key, value);
}

static HRESULT WINAPI mfattributes_GetItemType(IMFAttributes *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), type);

    return attributes_GetItemType(attributes, key, type);
}

static HRESULT WINAPI mfattributes_CompareItem(IMFAttributes *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_propvar(value), result);

    return attributes_CompareItem(attributes, key, value, result);
}

static HRESULT WINAPI mfattributes_Compare(IMFAttributes *iface, IMFAttributes *theirs,
        MF_ATTRIBUTES_MATCH_TYPE match_type, BOOL *ret)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %p, %d, %p.\n", iface, theirs, match_type, ret);

    return attributes_Compare(attributes, theirs, match_type, ret);
}

static HRESULT WINAPI mfattributes_GetUINT32(IMFAttributes *iface, REFGUID key, UINT32 *value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT32(attributes, key, value);
}

static HRESULT WINAPI mfattributes_GetUINT64(IMFAttributes *iface, REFGUID key, UINT64 *value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT64(attributes, key, value);
}

static HRESULT WINAPI mfattributes_GetDouble(IMFAttributes *iface, REFGUID key, double *value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetDouble(attributes, key, value);
}

static HRESULT WINAPI mfattributes_GetGUID(IMFAttributes *iface, REFGUID key, GUID *value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetGUID(attributes, key, value);
}

static HRESULT WINAPI mfattributes_GetStringLength(IMFAttributes *iface, REFGUID key, UINT32 *length)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), length);

    return attributes_GetStringLength(attributes, key, length);
}

static HRESULT WINAPI mfattributes_GetString(IMFAttributes *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_attr(key), value, size, length);

    return attributes_GetString(attributes, key, value, size, length);
}

static HRESULT WINAPI mfattributes_GetAllocatedString(IMFAttributes *iface, REFGUID key, WCHAR **value, UINT32 *length)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), value, length);

    return attributes_GetAllocatedString(attributes, key, value, length);
}

static HRESULT WINAPI mfattributes_GetBlobSize(IMFAttributes *iface, REFGUID key, UINT32 *size)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), size);

    return attributes_GetBlobSize(attributes, key, size);
}

static HRESULT WINAPI mfattributes_GetBlob(IMFAttributes *iface, REFGUID key, UINT8 *buf,
                UINT32 bufsize, UINT32 *blobsize)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_attr(key), buf, bufsize, blobsize);

    return attributes_GetBlob(attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI mfattributes_GetAllocatedBlob(IMFAttributes *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), buf, size);

    return attributes_GetAllocatedBlob(attributes, key, buf, size);
}

static HRESULT WINAPI mfattributes_GetUnknown(IMFAttributes *iface, REFGUID key, REFIID riid, void **out)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_guid(riid), out);

    return attributes_GetUnknown(attributes, key, riid, out);
}

static HRESULT WINAPI mfattributes_SetItem(IMFAttributes *iface, REFGUID key, REFPROPVARIANT value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_propvar(value));

    return attributes_SetItem(attributes, key, value);
}

static HRESULT WINAPI mfattributes_DeleteItem(IMFAttributes *iface, REFGUID key)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s.\n", iface, debugstr_attr(key));

    return attributes_DeleteItem(attributes, key);
}

static HRESULT WINAPI mfattributes_DeleteAllItems(IMFAttributes *iface)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p.\n", iface);

    return attributes_DeleteAllItems(attributes);
}

static HRESULT WINAPI mfattributes_SetUINT32(IMFAttributes *iface, REFGUID key, UINT32 value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %u.\n", iface, debugstr_attr(key), value);

    return attributes_SetUINT32(attributes, key, value);
}

static HRESULT WINAPI mfattributes_SetUINT64(IMFAttributes *iface, REFGUID key, UINT64 value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), wine_dbgstr_longlong(value));

    return attributes_SetUINT64(attributes, key, value);
}

static HRESULT WINAPI mfattributes_SetDouble(IMFAttributes *iface, REFGUID key, double value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_attr(key), value);

    return attributes_SetDouble(attributes, key, value);
}

static HRESULT WINAPI mfattributes_SetGUID(IMFAttributes *iface, REFGUID key, REFGUID value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_mf_guid(value));

    return attributes_SetGUID(attributes, key, value);
}

static HRESULT WINAPI mfattributes_SetString(IMFAttributes *iface, REFGUID key, const WCHAR *value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_w(value));

    return attributes_SetString(attributes, key, value);
}

static HRESULT WINAPI mfattributes_SetBlob(IMFAttributes *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %u.\n", iface, debugstr_attr(key), buf, size);

    return attributes_SetBlob(attributes, key, buf, size);
}

static HRESULT WINAPI mfattributes_SetUnknown(IMFAttributes *iface, REFGUID key, IUnknown *unknown)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), unknown);

    return attributes_SetUnknown(attributes, key, unknown);
}

static HRESULT WINAPI mfattributes_LockStore(IMFAttributes *iface)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p.\n", iface);

    return attributes_LockStore(attributes);
}

static HRESULT WINAPI mfattributes_UnlockStore(IMFAttributes *iface)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p.\n", iface);

    return attributes_UnlockStore(attributes);
}

static HRESULT WINAPI mfattributes_GetCount(IMFAttributes *iface, UINT32 *count)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %p.\n", iface, count);

    return attributes_GetCount(attributes, count);
}

static HRESULT WINAPI mfattributes_GetItemByIndex(IMFAttributes *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return attributes_GetItemByIndex(attributes, index, key, value);
}

static HRESULT WINAPI mfattributes_CopyAllItems(IMFAttributes *iface, IMFAttributes *dest)
{
    struct attributes *attributes = impl_from_IMFAttributes(iface);

    TRACE("%p, %p.\n", iface, dest);

    return attributes_CopyAllItems(attributes, dest);
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
                item.u.subheader.size = (lstrlenW(value.u.pwszVal) + 1) * sizeof(WCHAR);
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

typedef struct bytestream
{
    struct attributes attributes;
    IMFByteStream IMFByteStream_iface;
    IMFGetService IMFGetService_iface;
    IRtwqAsyncCallback read_callback;
    IRtwqAsyncCallback write_callback;
    IStream *stream;
    HANDLE hfile;
    QWORD position;
    DWORD capabilities;
    struct list pending;
    CRITICAL_SECTION cs;
} mfbytestream;

static inline mfbytestream *impl_from_IMFByteStream(IMFByteStream *iface)
{
    return CONTAINING_RECORD(iface, mfbytestream, IMFByteStream_iface);
}

static struct bytestream *impl_bytestream_from_IMFGetService(IMFGetService *iface)
{
    return CONTAINING_RECORD(iface, struct bytestream, IMFGetService_iface);
}

static struct bytestream *impl_from_read_callback_IRtwqAsyncCallback(IRtwqAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct bytestream, read_callback);
}

static struct bytestream *impl_from_write_callback_IRtwqAsyncCallback(IRtwqAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct bytestream, write_callback);
}

enum async_stream_op_type
{
    ASYNC_STREAM_OP_READ,
    ASYNC_STREAM_OP_WRITE,
};

struct async_stream_op
{
    IUnknown IUnknown_iface;
    LONG refcount;
    union
    {
        const BYTE *src;
        BYTE *dest;
    } u;
    QWORD position;
    ULONG requested_length;
    ULONG actual_length;
    IMFAsyncResult *caller;
    struct list entry;
    enum async_stream_op_type type;
};

static struct async_stream_op *impl_async_stream_op_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct async_stream_op, IUnknown_iface);
}

static HRESULT WINAPI async_stream_op_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI async_stream_op_AddRef(IUnknown *iface)
{
    struct async_stream_op *op = impl_async_stream_op_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&op->refcount);

    TRACE("%p, refcount %d.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI async_stream_op_Release(IUnknown *iface)
{
    struct async_stream_op *op = impl_async_stream_op_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&op->refcount);

    TRACE("%p, refcount %d.\n", iface, refcount);

    if (!refcount)
    {
        if (op->caller)
            IMFAsyncResult_Release(op->caller);
        heap_free(op);
    }

    return refcount;
}

static const IUnknownVtbl async_stream_op_vtbl =
{
    async_stream_op_QueryInterface,
    async_stream_op_AddRef,
    async_stream_op_Release,
};

static HRESULT bytestream_create_io_request(struct bytestream *stream, enum async_stream_op_type type,
        const BYTE *data, ULONG size, IMFAsyncCallback *callback, IUnknown *state)
{
    struct async_stream_op *op;
    IRtwqAsyncResult *request;
    HRESULT hr;

    op = heap_alloc(sizeof(*op));
    if (!op)
        return E_OUTOFMEMORY;

    op->IUnknown_iface.lpVtbl = &async_stream_op_vtbl;
    op->refcount = 1;
    op->u.src = data;
    op->position = stream->position;
    op->requested_length = size;
    op->type = type;
    if (FAILED(hr = RtwqCreateAsyncResult((IUnknown *)&stream->IMFByteStream_iface, (IRtwqAsyncCallback *)callback, state,
            (IRtwqAsyncResult **)&op->caller)))
    {
        goto failed;
    }

    if (FAILED(hr = RtwqCreateAsyncResult(&op->IUnknown_iface, type == ASYNC_STREAM_OP_READ ? &stream->read_callback :
            &stream->write_callback, NULL, &request)))
        goto failed;

    RtwqPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, 0, request);
    IRtwqAsyncResult_Release(request);

failed:
    IUnknown_Release(&op->IUnknown_iface);
    return hr;
}

static HRESULT bytestream_complete_io_request(struct bytestream *stream, enum async_stream_op_type type,
        IMFAsyncResult *result, ULONG *actual_length)
{
    struct async_stream_op *op = NULL, *cur;
    HRESULT hr;

    EnterCriticalSection(&stream->cs);
    LIST_FOR_EACH_ENTRY(cur, &stream->pending, struct async_stream_op, entry)
    {
        if (cur->caller == result && cur->type == type)
        {
            op = cur;
            list_remove(&cur->entry);
            break;
        }
    }
    LeaveCriticalSection(&stream->cs);

    if (!op)
        return E_INVALIDARG;

    if (SUCCEEDED(hr = IMFAsyncResult_GetStatus(result)))
        *actual_length = op->actual_length;

    IUnknown_Release(&op->IUnknown_iface);

    return hr;
}

static HRESULT WINAPI bytestream_callback_QueryInterface(IRtwqAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IRtwqAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IRtwqAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI bytestream_read_callback_AddRef(IRtwqAsyncCallback *iface)
{
    struct bytestream *stream = impl_from_read_callback_IRtwqAsyncCallback(iface);
    return IMFByteStream_AddRef(&stream->IMFByteStream_iface);
}

static ULONG WINAPI bytestream_read_callback_Release(IRtwqAsyncCallback *iface)
{
    struct bytestream *stream = impl_from_read_callback_IRtwqAsyncCallback(iface);
    return IMFByteStream_Release(&stream->IMFByteStream_iface);
}

static HRESULT WINAPI bytestream_callback_GetParameters(IRtwqAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static ULONG WINAPI bytestream_write_callback_AddRef(IRtwqAsyncCallback *iface)
{
    struct bytestream *stream = impl_from_write_callback_IRtwqAsyncCallback(iface);
    return IMFByteStream_AddRef(&stream->IMFByteStream_iface);
}

static ULONG WINAPI bytestream_write_callback_Release(IRtwqAsyncCallback *iface)
{
    struct bytestream *stream = impl_from_write_callback_IRtwqAsyncCallback(iface);
    return IMFByteStream_Release(&stream->IMFByteStream_iface);
}

static HRESULT WINAPI bytestream_QueryInterface(IMFByteStream *iface, REFIID riid, void **out)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFByteStream) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &stream->IMFByteStream_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFAttributes))
    {
        *out = &stream->attributes.IMFAttributes_iface;
    }
    else if (stream->IMFGetService_iface.lpVtbl && IsEqualIID(riid, &IID_IMFGetService))
    {
        *out = &stream->IMFGetService_iface;
    }
    else
    {
        WARN("Unsupported %s.\n", debugstr_guid(riid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static ULONG WINAPI bytestream_AddRef(IMFByteStream *iface)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);
    ULONG refcount = InterlockedIncrement(&stream->attributes.ref);

    TRACE("%p, refcount %d.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI bytestream_Release(IMFByteStream *iface)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);
    ULONG refcount = InterlockedDecrement(&stream->attributes.ref);
    struct async_stream_op *cur, *cur2;

    TRACE("%p, refcount %d.\n", iface, refcount);

    if (!refcount)
    {
        clear_attributes_object(&stream->attributes);
        LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &stream->pending, struct async_stream_op, entry)
        {
            list_remove(&cur->entry);
            IUnknown_Release(&cur->IUnknown_iface);
        }
        DeleteCriticalSection(&stream->cs);
        if (stream->stream)
            IStream_Release(stream->stream);
        if (stream->hfile)
            CloseHandle(stream->hfile);
        heap_free(stream);
    }

    return refcount;
}

static HRESULT WINAPI bytestream_stream_GetCapabilities(IMFByteStream *iface, DWORD *capabilities)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);
    STATSTG stat;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, capabilities);

    if (FAILED(hr = IStream_Stat(stream->stream, &stat, STATFLAG_NONAME)))
        return hr;

    *capabilities = MFBYTESTREAM_IS_READABLE | MFBYTESTREAM_IS_SEEKABLE;
    if (stat.grfMode & (STGM_WRITE | STGM_READWRITE))
        *capabilities |= MFBYTESTREAM_IS_WRITABLE;

    return S_OK;
}

static HRESULT WINAPI bytestream_GetCapabilities(IMFByteStream *iface, DWORD *capabilities)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);

    TRACE("%p, %p.\n", iface, capabilities);

    *capabilities = stream->capabilities;

    return S_OK;
}

static HRESULT WINAPI mfbytestream_SetLength(IMFByteStream *iface, QWORD length)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);

    FIXME("%p, %s\n", This, wine_dbgstr_longlong(length));

    return E_NOTIMPL;
}

static HRESULT WINAPI bytestream_file_GetCurrentPosition(IMFByteStream *iface, QWORD *position)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);

    TRACE("%p, %p.\n", iface, position);

    if (!position)
        return E_INVALIDARG;

    *position = stream->position;

    return S_OK;
}

static HRESULT WINAPI bytestream_file_GetLength(IMFByteStream *iface, QWORD *length)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);
    LARGE_INTEGER li;

    TRACE("%p, %p.\n", iface, length);

    if (!length)
        return E_INVALIDARG;

    if (GetFileSizeEx(stream->hfile, &li))
        *length = li.QuadPart;
    else
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

static HRESULT WINAPI bytestream_file_IsEndOfStream(IMFByteStream *iface, BOOL *ret)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);
    LARGE_INTEGER position, length;
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, ret);

    EnterCriticalSection(&stream->cs);

    position.QuadPart = 0;
    if (SetFilePointerEx(stream->hfile, position, &length, FILE_END))
        *ret = stream->position >= length.QuadPart;
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    LeaveCriticalSection(&stream->cs);

    return hr;
}

static HRESULT WINAPI bytestream_file_Read(IMFByteStream *iface, BYTE *buffer, ULONG size, ULONG *read_len)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);
    LARGE_INTEGER position;
    HRESULT hr = S_OK;
    BOOL ret;

    TRACE("%p, %p, %u, %p.\n", iface, buffer, size, read_len);

    EnterCriticalSection(&stream->cs);

    position.QuadPart = stream->position;
    if ((ret = SetFilePointerEx(stream->hfile, position, NULL, FILE_BEGIN)))
    {
        if ((ret = ReadFile(stream->hfile, buffer, size, read_len, NULL)))
            stream->position += *read_len;
    }

    if (!ret)
        hr = HRESULT_FROM_WIN32(GetLastError());

    LeaveCriticalSection(&stream->cs);

    return hr;
}

static HRESULT WINAPI bytestream_BeginRead(IMFByteStream *iface, BYTE *data, ULONG size, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);

    TRACE("%p, %p, %u, %p, %p.\n", iface, data, size, callback, state);

    return bytestream_create_io_request(stream, ASYNC_STREAM_OP_READ, data, size, callback, state);
}

static HRESULT WINAPI bytestream_EndRead(IMFByteStream *iface, IMFAsyncResult *result, ULONG *byte_read)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);

    TRACE("%p, %p, %p.\n", iface, result, byte_read);

    return bytestream_complete_io_request(stream, ASYNC_STREAM_OP_READ, result, byte_read);
}

static HRESULT WINAPI mfbytestream_Write(IMFByteStream *iface, const BYTE *data, ULONG count, ULONG *written)
{
    mfbytestream *This = impl_from_IMFByteStream(iface);

    FIXME("%p, %p, %u, %p\n", This, data, count, written);

    return E_NOTIMPL;
}

static HRESULT WINAPI bytestream_BeginWrite(IMFByteStream *iface, const BYTE *data, ULONG size,
        IMFAsyncCallback *callback, IUnknown *state)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);

    TRACE("%p, %p, %u, %p, %p.\n", iface, data, size, callback, state);

    return bytestream_create_io_request(stream, ASYNC_STREAM_OP_WRITE, data, size, callback, state);
}

static HRESULT WINAPI bytestream_EndWrite(IMFByteStream *iface, IMFAsyncResult *result, ULONG *written)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);

    TRACE("%p, %p, %p.\n", iface, result, written);

    return bytestream_complete_io_request(stream, ASYNC_STREAM_OP_WRITE, result, written);
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

static HRESULT WINAPI bytestream_SetCurrentPosition(IMFByteStream *iface, QWORD position)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);

    TRACE("%p, %s.\n", iface, wine_dbgstr_longlong(position));

    EnterCriticalSection(&stream->cs);
    stream->position = position;
    LeaveCriticalSection(&stream->cs);

    return S_OK;
}

static const IMFByteStreamVtbl bytestream_file_vtbl =
{
    bytestream_QueryInterface,
    bytestream_AddRef,
    bytestream_Release,
    bytestream_GetCapabilities,
    bytestream_file_GetLength,
    mfbytestream_SetLength,
    bytestream_file_GetCurrentPosition,
    bytestream_SetCurrentPosition,
    bytestream_file_IsEndOfStream,
    bytestream_file_Read,
    bytestream_BeginRead,
    bytestream_EndRead,
    mfbytestream_Write,
    bytestream_BeginWrite,
    bytestream_EndWrite,
    mfbytestream_Seek,
    mfbytestream_Flush,
    mfbytestream_Close
};

static HRESULT WINAPI bytestream_stream_GetLength(IMFByteStream *iface, QWORD *length)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);
    STATSTG statstg;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, length);

    if (FAILED(hr = IStream_Stat(stream->stream, &statstg, STATFLAG_NONAME)))
        return hr;

    *length = statstg.cbSize.QuadPart;

    return S_OK;
}

static HRESULT WINAPI bytestream_stream_SetLength(IMFByteStream *iface, QWORD length)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);
    ULARGE_INTEGER size;
    HRESULT hr;

    TRACE("%p, %s.\n", iface, wine_dbgstr_longlong(length));

    EnterCriticalSection(&stream->cs);

    size.QuadPart = length;
    hr = IStream_SetSize(stream->stream, size);

    LeaveCriticalSection(&stream->cs);

    return hr;
}

static HRESULT WINAPI bytestream_stream_GetCurrentPosition(IMFByteStream *iface, QWORD *position)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);

    TRACE("%p, %p.\n", iface, position);

    *position = stream->position;

    return S_OK;
}

static HRESULT WINAPI bytestream_stream_IsEndOfStream(IMFByteStream *iface, BOOL *ret)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);
    STATSTG statstg;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, ret);

    EnterCriticalSection(&stream->cs);

    if (SUCCEEDED(hr = IStream_Stat(stream->stream, &statstg, STATFLAG_NONAME)))
        *ret = stream->position >= statstg.cbSize.QuadPart;

    LeaveCriticalSection(&stream->cs);

    return hr;
}

static HRESULT WINAPI bytestream_stream_Read(IMFByteStream *iface, BYTE *buffer, ULONG size, ULONG *read_len)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);
    LARGE_INTEGER position;
    HRESULT hr;

    TRACE("%p, %p, %u, %p.\n", iface, buffer, size, read_len);

    EnterCriticalSection(&stream->cs);

    position.QuadPart = stream->position;
    if (SUCCEEDED(hr = IStream_Seek(stream->stream, position, STREAM_SEEK_SET, NULL)))
    {
        if (SUCCEEDED(hr = IStream_Read(stream->stream, buffer, size, read_len)))
            stream->position += *read_len;
    }

    LeaveCriticalSection(&stream->cs);

    return hr;
}

static HRESULT WINAPI bytestream_stream_Write(IMFByteStream *iface, const BYTE *buffer, ULONG size, ULONG *written)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);
    LARGE_INTEGER position;
    HRESULT hr;

    TRACE("%p, %p, %u, %p.\n", iface, buffer, size, written);

    EnterCriticalSection(&stream->cs);

    position.QuadPart = stream->position;
    if (SUCCEEDED(hr = IStream_Seek(stream->stream, position, STREAM_SEEK_SET, NULL)))
    {
        if (SUCCEEDED(hr = IStream_Write(stream->stream, buffer, size, written)))
            stream->position += *written;
    }

    LeaveCriticalSection(&stream->cs);

    return hr;
}

static HRESULT WINAPI bytestream_stream_Seek(IMFByteStream *iface, MFBYTESTREAM_SEEK_ORIGIN origin, LONGLONG offset,
        DWORD flags, QWORD *current)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %s, %#x, %p.\n", iface, origin, wine_dbgstr_longlong(offset), flags, current);

    EnterCriticalSection(&stream->cs);

    switch (origin)
    {
        case msoBegin:
            stream->position = offset;
            break;
        case msoCurrent:
            stream->position += offset;
            break;
        default:
            WARN("Unknown origin mode %d.\n", origin);
            hr = E_INVALIDARG;
    }

    *current = stream->position;

    LeaveCriticalSection(&stream->cs);

    return hr;
}

static HRESULT WINAPI bytestream_stream_Flush(IMFByteStream *iface)
{
    struct bytestream *stream = impl_from_IMFByteStream(iface);

    TRACE("%p.\n", iface);

    return IStream_Commit(stream->stream, STGC_DEFAULT);
}

static HRESULT WINAPI bytestream_stream_Close(IMFByteStream *iface)
{
    TRACE("%p.\n", iface);

    return S_OK;
}

static const IMFByteStreamVtbl bytestream_stream_vtbl =
{
    bytestream_QueryInterface,
    bytestream_AddRef,
    bytestream_Release,
    bytestream_stream_GetCapabilities,
    bytestream_stream_GetLength,
    bytestream_stream_SetLength,
    bytestream_stream_GetCurrentPosition,
    bytestream_SetCurrentPosition,
    bytestream_stream_IsEndOfStream,
    bytestream_stream_Read,
    bytestream_BeginRead,
    bytestream_EndRead,
    bytestream_stream_Write,
    bytestream_BeginWrite,
    bytestream_EndWrite,
    bytestream_stream_Seek,
    bytestream_stream_Flush,
    bytestream_stream_Close,
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

static HRESULT WINAPI bytestream_stream_read_callback_Invoke(IRtwqAsyncCallback *iface, IRtwqAsyncResult *result)
{
    struct bytestream *stream = impl_from_read_callback_IRtwqAsyncCallback(iface);
    struct async_stream_op *op;
    LARGE_INTEGER position;
    IUnknown *object;
    HRESULT hr;

    if (FAILED(hr = IRtwqAsyncResult_GetObject(result, &object)))
        return hr;

    op = impl_async_stream_op_from_IUnknown(object);

    EnterCriticalSection(&stream->cs);

    position.QuadPart = op->position;
    if (SUCCEEDED(hr = IStream_Seek(stream->stream, position, STREAM_SEEK_SET, NULL)))
    {
        if (SUCCEEDED(hr = IStream_Read(stream->stream, op->u.dest, op->requested_length, &op->actual_length)))
            stream->position += op->actual_length;
    }

    IMFAsyncResult_SetStatus(op->caller, hr);
    list_add_tail(&stream->pending, &op->entry);

    LeaveCriticalSection(&stream->cs);

    MFInvokeCallback(op->caller);

    return S_OK;
}

static HRESULT WINAPI bytestream_stream_write_callback_Invoke(IRtwqAsyncCallback *iface, IRtwqAsyncResult *result)
{
    struct bytestream *stream = impl_from_read_callback_IRtwqAsyncCallback(iface);
    struct async_stream_op *op;
    LARGE_INTEGER position;
    IUnknown *object;
    HRESULT hr;

    if (FAILED(hr = IRtwqAsyncResult_GetObject(result, &object)))
        return hr;

    op = impl_async_stream_op_from_IUnknown(object);

    EnterCriticalSection(&stream->cs);

    position.QuadPart = op->position;
    if (SUCCEEDED(hr = IStream_Seek(stream->stream, position, STREAM_SEEK_SET, NULL)))
    {
        if (SUCCEEDED(hr = IStream_Write(stream->stream, op->u.src, op->requested_length, &op->actual_length)))
            stream->position += op->actual_length;
    }

    IMFAsyncResult_SetStatus(op->caller, hr);
    list_add_tail(&stream->pending, &op->entry);

    LeaveCriticalSection(&stream->cs);

    MFInvokeCallback(op->caller);

    return S_OK;
}

static const IRtwqAsyncCallbackVtbl bytestream_stream_read_callback_vtbl =
{
    bytestream_callback_QueryInterface,
    bytestream_read_callback_AddRef,
    bytestream_read_callback_Release,
    bytestream_callback_GetParameters,
    bytestream_stream_read_callback_Invoke,
};

static const IRtwqAsyncCallbackVtbl bytestream_stream_write_callback_vtbl =
{
    bytestream_callback_QueryInterface,
    bytestream_write_callback_AddRef,
    bytestream_write_callback_Release,
    bytestream_callback_GetParameters,
    bytestream_stream_write_callback_Invoke,
};

/***********************************************************************
 *      MFCreateMFByteStreamOnStream (mfplat.@)
 */
HRESULT WINAPI MFCreateMFByteStreamOnStream(IStream *stream, IMFByteStream **bytestream)
{
    struct bytestream *object;
    LARGE_INTEGER position;
    STATSTG stat;
    HRESULT hr;

    TRACE("%p, %p.\n", stream, bytestream);

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    if (FAILED(hr = init_attributes_object(&object->attributes, 0)))
    {
        heap_free(object);
        return hr;
    }

    object->IMFByteStream_iface.lpVtbl = &bytestream_stream_vtbl;
    object->attributes.IMFAttributes_iface.lpVtbl = &mfbytestream_attributes_vtbl;
    object->read_callback.lpVtbl = &bytestream_stream_read_callback_vtbl;
    object->write_callback.lpVtbl = &bytestream_stream_write_callback_vtbl;
    InitializeCriticalSection(&object->cs);
    list_init(&object->pending);

    object->stream = stream;
    IStream_AddRef(object->stream);
    position.QuadPart = 0;
    IStream_Seek(object->stream, position, STREAM_SEEK_SET, NULL);

    if (SUCCEEDED(IStream_Stat(object->stream, &stat, 0)))
    {
        if (stat.pwcsName)
        {
            IMFAttributes_SetString(&object->attributes.IMFAttributes_iface, &MF_BYTESTREAM_ORIGIN_NAME,
                    stat.pwcsName);
            CoTaskMemFree(stat.pwcsName);
        }
    }

    *bytestream = &object->IMFByteStream_iface;

    return S_OK;
}

static HRESULT WINAPI bytestream_file_read_callback_Invoke(IRtwqAsyncCallback *iface, IRtwqAsyncResult *result)
{
    FIXME("%p, %p.\n", iface, result);

    return E_NOTIMPL;
}

static HRESULT WINAPI bytestream_file_write_callback_Invoke(IRtwqAsyncCallback *iface, IRtwqAsyncResult *result)
{
    FIXME("%p, %p.\n", iface, result);

    return E_NOTIMPL;
}

static const IRtwqAsyncCallbackVtbl bytestream_file_read_callback_vtbl =
{
    bytestream_callback_QueryInterface,
    bytestream_read_callback_AddRef,
    bytestream_read_callback_Release,
    bytestream_callback_GetParameters,
    bytestream_file_read_callback_Invoke,
};

static const IRtwqAsyncCallbackVtbl bytestream_file_write_callback_vtbl =
{
    bytestream_callback_QueryInterface,
    bytestream_write_callback_AddRef,
    bytestream_write_callback_Release,
    bytestream_callback_GetParameters,
    bytestream_file_write_callback_Invoke,
};

static HRESULT WINAPI bytestream_file_getservice_QueryInterface(IMFGetService *iface, REFIID riid, void **obj)
{
    struct bytestream *stream = impl_bytestream_from_IMFGetService(iface);
    return IMFByteStream_QueryInterface(&stream->IMFByteStream_iface, riid, obj);
}

static ULONG WINAPI bytestream_file_getservice_AddRef(IMFGetService *iface)
{
    struct bytestream *stream = impl_bytestream_from_IMFGetService(iface);
    return IMFByteStream_AddRef(&stream->IMFByteStream_iface);
}

static ULONG WINAPI bytestream_file_getservice_Release(IMFGetService *iface)
{
    struct bytestream *stream = impl_bytestream_from_IMFGetService(iface);
    return IMFByteStream_Release(&stream->IMFByteStream_iface);
}

static HRESULT WINAPI bytestream_file_getservice_GetService(IMFGetService *iface, REFGUID service,
        REFIID riid, void **obj)
{
    FIXME("%p, %s, %s, %p.\n", iface, debugstr_guid(service), debugstr_guid(riid), obj);

    return E_NOTIMPL;
}

static const IMFGetServiceVtbl bytestream_file_getservice_vtbl =
{
    bytestream_file_getservice_QueryInterface,
    bytestream_file_getservice_AddRef,
    bytestream_file_getservice_Release,
    bytestream_file_getservice_GetService,
};

/***********************************************************************
 *      MFCreateFile (mfplat.@)
 */
HRESULT WINAPI MFCreateFile(MF_FILE_ACCESSMODE accessmode, MF_FILE_OPENMODE openmode, MF_FILE_FLAGS flags,
                            LPCWSTR url, IMFByteStream **bytestream)
{
    DWORD capabilities = MFBYTESTREAM_IS_SEEKABLE | MFBYTESTREAM_DOES_NOT_USE_NETWORK;
    DWORD filecreation_disposition = 0, fileaccessmode = 0, fileattributes = 0;
    DWORD filesharemode = FILE_SHARE_READ;
    struct bytestream *object;
    FILETIME writetime;
    HANDLE file;
    HRESULT hr;

    TRACE("%d, %d, %#x, %s, %p.\n", accessmode, openmode, flags, debugstr_w(url), bytestream);

    switch (accessmode)
    {
        case MF_ACCESSMODE_READ:
            fileaccessmode = GENERIC_READ;
            capabilities |= MFBYTESTREAM_IS_READABLE;
            break;
        case MF_ACCESSMODE_WRITE:
            fileaccessmode = GENERIC_WRITE;
            capabilities |= MFBYTESTREAM_IS_WRITABLE;
            break;
        case MF_ACCESSMODE_READWRITE:
            fileaccessmode = GENERIC_READ | GENERIC_WRITE;
            capabilities |= (MFBYTESTREAM_IS_READABLE | MFBYTESTREAM_IS_WRITABLE);
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

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
    {
        CloseHandle(file);
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr = init_attributes_object(&object->attributes, 2)))
    {
        CloseHandle(file);
        heap_free(object);
        return hr;
    }
    object->IMFByteStream_iface.lpVtbl = &bytestream_file_vtbl;
    object->attributes.IMFAttributes_iface.lpVtbl = &mfbytestream_attributes_vtbl;
    object->IMFGetService_iface.lpVtbl = &bytestream_file_getservice_vtbl;
    object->read_callback.lpVtbl = &bytestream_file_read_callback_vtbl;
    object->write_callback.lpVtbl = &bytestream_file_write_callback_vtbl;
    InitializeCriticalSection(&object->cs);
    list_init(&object->pending);
    object->capabilities = capabilities;
    object->hfile = file;

    if (GetFileTime(file, NULL, NULL, &writetime))
    {
        IMFAttributes_SetBlob(&object->attributes.IMFAttributes_iface, &MF_BYTESTREAM_LAST_MODIFIED_TIME,
                (const UINT8 *)&writetime, sizeof(writetime));
    }

    IMFAttributes_SetString(&object->attributes.IMFAttributes_iface, &MF_BYTESTREAM_ORIGIN_NAME, url);

    *bytestream = &object->IMFByteStream_iface;

    return S_OK;
}

struct bytestream_wrapper
{
    IMFByteStreamCacheControl IMFByteStreamCacheControl_iface;
    IMFByteStreamBuffering IMFByteStreamBuffering_iface;
    IMFMediaEventGenerator IMFMediaEventGenerator_iface;
    IMFByteStreamTimeSeek IMFByteStreamTimeSeek_iface;
    IMFSampleOutputStream IMFSampleOutputStream_iface;
    IPropertyStore IPropertyStore_iface;
    IMFByteStream IMFByteStream_iface;
    IMFAttributes IMFAttributes_iface;
    LONG refcount;

    IMFByteStreamCacheControl *cache_control;
    IMFByteStreamBuffering *stream_buffering;
    IMFMediaEventGenerator *event_generator;
    IMFByteStreamTimeSeek *time_seek;
    IMFSampleOutputStream *sample_output;
    IPropertyStore *propstore;
    IMFByteStream *stream;
    IMFAttributes *attributes;
    BOOL is_closed;
};

static struct bytestream_wrapper *impl_wrapper_from_IMFByteStream(IMFByteStream *iface)
{
    return CONTAINING_RECORD(iface, struct bytestream_wrapper, IMFByteStream_iface);
}

static struct bytestream_wrapper *impl_wrapper_from_IMFByteStreamCacheControl(IMFByteStreamCacheControl *iface)
{
    return CONTAINING_RECORD(iface, struct bytestream_wrapper, IMFByteStreamCacheControl_iface);
}

static struct bytestream_wrapper *impl_wrapper_from_IMFByteStreamBuffering(IMFByteStreamBuffering *iface)
{
    return CONTAINING_RECORD(iface, struct bytestream_wrapper, IMFByteStreamBuffering_iface);
}

static struct bytestream_wrapper *impl_wrapper_from_IMFMediaEventGenerator(IMFMediaEventGenerator *iface)
{
    return CONTAINING_RECORD(iface, struct bytestream_wrapper, IMFMediaEventGenerator_iface);
}

static struct bytestream_wrapper *impl_wrapper_from_IMFByteStreamTimeSeek(IMFByteStreamTimeSeek *iface)
{
    return CONTAINING_RECORD(iface, struct bytestream_wrapper, IMFByteStreamTimeSeek_iface);
}

static struct bytestream_wrapper *impl_wrapper_from_IMFSampleOutputStream(IMFSampleOutputStream *iface)
{
    return CONTAINING_RECORD(iface, struct bytestream_wrapper, IMFSampleOutputStream_iface);
}

static struct bytestream_wrapper *impl_wrapper_from_IPropertyStore(IPropertyStore *iface)
{
    return CONTAINING_RECORD(iface, struct bytestream_wrapper, IPropertyStore_iface);
}

static struct bytestream_wrapper *impl_wrapper_from_IMFAttributes(IMFAttributes *iface)
{
    return CONTAINING_RECORD(iface, struct bytestream_wrapper, IMFAttributes_iface);
}

static HRESULT WINAPI bytestream_wrapper_QueryInterface(IMFByteStream *iface, REFIID riid, void **out)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStream(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFByteStream) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &wrapper->IMFByteStream_iface;
    }
    else if (wrapper->cache_control && IsEqualIID(riid, &IID_IMFByteStreamCacheControl))
    {
        *out = &wrapper->IMFByteStreamCacheControl_iface;
    }
    else if (wrapper->stream_buffering && IsEqualIID(riid, &IID_IMFByteStreamBuffering))
    {
        *out = &wrapper->IMFByteStreamBuffering_iface;
    }
    else if (wrapper->event_generator && IsEqualIID(riid, &IID_IMFMediaEventGenerator))
    {
        *out = &wrapper->IMFMediaEventGenerator_iface;
    }
    else if (wrapper->time_seek && IsEqualIID(riid, &IID_IMFByteStreamTimeSeek))
    {
        *out = &wrapper->IMFByteStreamTimeSeek_iface;
    }
    else if (wrapper->sample_output && IsEqualIID(riid, &IID_IMFSampleOutputStream))
    {
        *out = &wrapper->IMFSampleOutputStream_iface;
    }
    else if (wrapper->propstore && IsEqualIID(riid, &IID_IPropertyStore))
    {
        *out = &wrapper->IPropertyStore_iface;
    }
    else if (wrapper->attributes && IsEqualIID(riid, &IID_IMFAttributes))
    {
        *out = &wrapper->IMFAttributes_iface;
    }
    else
    {
        WARN("Unsupported %s.\n", debugstr_guid(riid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI bytestream_wrapper_AddRef(IMFByteStream *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStream(iface);
    ULONG refcount = InterlockedIncrement(&wrapper->refcount);

    TRACE("%p, refcount %d.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI bytestream_wrapper_Release(IMFByteStream *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStream(iface);
    ULONG refcount = InterlockedDecrement(&wrapper->refcount);

    TRACE("%p, refcount %d.\n", iface, refcount);

    if (!refcount)
    {
        if (wrapper->cache_control)
            IMFByteStreamCacheControl_Release(wrapper->cache_control);
        if (wrapper->stream_buffering)
            IMFByteStreamBuffering_Release(wrapper->stream_buffering);
        if (wrapper->event_generator)
            IMFMediaEventGenerator_Release(wrapper->event_generator);
        if (wrapper->time_seek)
            IMFByteStreamTimeSeek_Release(wrapper->time_seek);
        if (wrapper->sample_output)
            IMFSampleOutputStream_Release(wrapper->sample_output);
        if (wrapper->propstore)
            IPropertyStore_Release(wrapper->propstore);
        if (wrapper->attributes)
            IMFAttributes_Release(wrapper->attributes);
        IMFByteStream_Release(wrapper->stream);
        heap_free(wrapper);
    }

    return refcount;
}

static HRESULT WINAPI bytestream_wrapper_GetCapabilities(IMFByteStream *iface, DWORD *capabilities)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStream(iface);

    TRACE("%p, %p.\n", iface, capabilities);

    return wrapper->is_closed ? MF_E_INVALIDREQUEST :
            IMFByteStream_GetCapabilities(wrapper->stream, capabilities);
}

static HRESULT WINAPI bytestream_wrapper_GetLength(IMFByteStream *iface, QWORD *length)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStream(iface);

    TRACE("%p, %p.\n", iface, length);

    if (wrapper->is_closed)
        return MF_E_INVALIDREQUEST;

    return wrapper->is_closed ? MF_E_INVALIDREQUEST :
            IMFByteStream_GetLength(wrapper->stream, length);
}

static HRESULT WINAPI bytestream_wrapper_SetLength(IMFByteStream *iface, QWORD length)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStream(iface);

    TRACE("%p, %s.\n", iface, wine_dbgstr_longlong(length));

    if (wrapper->is_closed)
        return MF_E_INVALIDREQUEST;

    return wrapper->is_closed ? MF_E_INVALIDREQUEST :
            IMFByteStream_SetLength(wrapper->stream, length);
}

static HRESULT WINAPI bytestream_wrapper_GetCurrentPosition(IMFByteStream *iface, QWORD *position)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStream(iface);

    TRACE("%p, %p.\n", iface, position);

    if (wrapper->is_closed)
        return MF_E_INVALIDREQUEST;

    return wrapper->is_closed ? MF_E_INVALIDREQUEST :
            IMFByteStream_GetCurrentPosition(wrapper->stream, position);
}

static HRESULT WINAPI bytestream_wrapper_SetCurrentPosition(IMFByteStream *iface, QWORD position)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStream(iface);

    TRACE("%p, %s.\n", iface, wine_dbgstr_longlong(position));

    return wrapper->is_closed ? MF_E_INVALIDREQUEST :
            IMFByteStream_SetCurrentPosition(wrapper->stream, position);
}

static HRESULT WINAPI bytestream_wrapper_IsEndOfStream(IMFByteStream *iface, BOOL *eos)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStream(iface);

    TRACE("%p, %p.\n", iface, eos);

    return wrapper->is_closed ? MF_E_INVALIDREQUEST :
            IMFByteStream_IsEndOfStream(wrapper->stream, eos);
}

static HRESULT WINAPI bytestream_wrapper_Read(IMFByteStream *iface, BYTE *data, ULONG count, ULONG *byte_read)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStream(iface);

    TRACE("%p, %p, %u, %p.\n", iface, data, count, byte_read);

    return wrapper->is_closed ? MF_E_INVALIDREQUEST :
            IMFByteStream_Read(wrapper->stream, data, count, byte_read);
}

static HRESULT WINAPI bytestream_wrapper_BeginRead(IMFByteStream *iface, BYTE *data, ULONG size,
        IMFAsyncCallback *callback, IUnknown *state)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStream(iface);

    TRACE("%p, %p, %u, %p, %p.\n", iface, data, size, callback, state);

    return wrapper->is_closed ? MF_E_INVALIDREQUEST :
            IMFByteStream_BeginRead(wrapper->stream, data, size, callback, state);
}

static HRESULT WINAPI bytestream_wrapper_EndRead(IMFByteStream *iface, IMFAsyncResult *result, ULONG *byte_read)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStream(iface);

    TRACE("%p, %p, %p.\n", iface, result, byte_read);

    return wrapper->is_closed ? MF_E_INVALIDREQUEST :
            IMFByteStream_EndRead(wrapper->stream, result, byte_read);
}

static HRESULT WINAPI bytestream_wrapper_Write(IMFByteStream *iface, const BYTE *data, ULONG count, ULONG *written)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStream(iface);

    TRACE("%p, %p, %u, %p.\n", iface, data, count, written);

    return wrapper->is_closed ? MF_E_INVALIDREQUEST :
            IMFByteStream_Write(wrapper->stream, data, count, written);
}

static HRESULT WINAPI bytestream_wrapper_BeginWrite(IMFByteStream *iface, const BYTE *data, ULONG size,
        IMFAsyncCallback *callback, IUnknown *state)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStream(iface);

    TRACE("%p, %p, %u, %p, %p.\n", iface, data, size, callback, state);

    return wrapper->is_closed ? MF_E_INVALIDREQUEST :
            IMFByteStream_BeginWrite(wrapper->stream, data, size, callback, state);
}

static HRESULT WINAPI bytestream_wrapper_EndWrite(IMFByteStream *iface, IMFAsyncResult *result, ULONG *written)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStream(iface);

    TRACE("%p, %p, %p.\n", iface, result, written);

    return wrapper->is_closed ? MF_E_INVALIDREQUEST :
            IMFByteStream_EndWrite(wrapper->stream, result, written);
}

static HRESULT WINAPI bytestream_wrapper_Seek(IMFByteStream *iface, MFBYTESTREAM_SEEK_ORIGIN seek, LONGLONG offset,
        DWORD flags, QWORD *current)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStream(iface);

    TRACE("%p, %u, %s, %#x, %p.\n", iface, seek, wine_dbgstr_longlong(offset), flags, current);

    return wrapper->is_closed ? MF_E_INVALIDREQUEST :
            IMFByteStream_Seek(wrapper->stream, seek, offset, flags, current);
}

static HRESULT WINAPI bytestream_wrapper_Flush(IMFByteStream *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStream(iface);

    TRACE("%p\n", iface);

    return wrapper->is_closed ? MF_E_INVALIDREQUEST : IMFByteStream_Flush(wrapper->stream);
}

static HRESULT WINAPI bytestream_wrapper_Close(IMFByteStream *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStream(iface);

    TRACE("%p\n", iface);

    wrapper->is_closed = TRUE;

    return S_OK;
}

static const IMFByteStreamVtbl bytestream_wrapper_vtbl =
{
    bytestream_wrapper_QueryInterface,
    bytestream_wrapper_AddRef,
    bytestream_wrapper_Release,
    bytestream_wrapper_GetCapabilities,
    bytestream_wrapper_GetLength,
    bytestream_wrapper_SetLength,
    bytestream_wrapper_GetCurrentPosition,
    bytestream_wrapper_SetCurrentPosition,
    bytestream_wrapper_IsEndOfStream,
    bytestream_wrapper_Read,
    bytestream_wrapper_BeginRead,
    bytestream_wrapper_EndRead,
    bytestream_wrapper_Write,
    bytestream_wrapper_BeginWrite,
    bytestream_wrapper_EndWrite,
    bytestream_wrapper_Seek,
    bytestream_wrapper_Flush,
    bytestream_wrapper_Close,
};

static HRESULT WINAPI bytestream_wrapper_cache_control_QueryInterface(IMFByteStreamCacheControl *iface,
        REFIID riid, void **obj)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStreamCacheControl(iface);
    return IMFByteStream_QueryInterface(&wrapper->IMFByteStream_iface, riid, obj);
}

static ULONG WINAPI bytestream_wrapper_cache_control_AddRef(IMFByteStreamCacheControl *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStreamCacheControl(iface);
    return IMFByteStream_AddRef(&wrapper->IMFByteStream_iface);
}

static ULONG WINAPI bytestream_wrapper_cache_control_Release(IMFByteStreamCacheControl *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStreamCacheControl(iface);
    return IMFByteStream_Release(&wrapper->IMFByteStream_iface);
}

static HRESULT WINAPI bytestream_wrapper_cache_control_StopBackgroundTransfer(IMFByteStreamCacheControl *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStreamCacheControl(iface);

    TRACE("%p.\n", iface);

    return IMFByteStreamCacheControl_StopBackgroundTransfer(wrapper->cache_control);
}

static const IMFByteStreamCacheControlVtbl bytestream_wrapper_cache_control_vtbl =
{
    bytestream_wrapper_cache_control_QueryInterface,
    bytestream_wrapper_cache_control_AddRef,
    bytestream_wrapper_cache_control_Release,
    bytestream_wrapper_cache_control_StopBackgroundTransfer,
};

static HRESULT WINAPI bytestream_wrapper_buffering_QueryInterface(IMFByteStreamBuffering *iface,
        REFIID riid, void **obj)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStreamBuffering(iface);
    return IMFByteStream_QueryInterface(&wrapper->IMFByteStream_iface, riid, obj);
}

static ULONG WINAPI bytestream_wrapper_buffering_AddRef(IMFByteStreamBuffering *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStreamBuffering(iface);
    return IMFByteStream_AddRef(&wrapper->IMFByteStream_iface);
}

static ULONG WINAPI bytestream_wrapper_buffering_Release(IMFByteStreamBuffering *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStreamBuffering(iface);
    return IMFByteStream_Release(&wrapper->IMFByteStream_iface);
}

static HRESULT WINAPI bytestream_wrapper_buffering_SetBufferingParams(IMFByteStreamBuffering *iface,
        MFBYTESTREAM_BUFFERING_PARAMS *params)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStreamBuffering(iface);

    TRACE("%p, %p.\n", iface, params);

    return IMFByteStreamBuffering_SetBufferingParams(wrapper->stream_buffering, params);
}

static HRESULT WINAPI bytestream_wrapper_buffering_EnableBuffering(IMFByteStreamBuffering *iface,
        BOOL enable)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStreamBuffering(iface);

    TRACE("%p, %d.\n", iface, enable);

    return IMFByteStreamBuffering_EnableBuffering(wrapper->stream_buffering, enable);
}

static HRESULT WINAPI bytestream_wrapper_buffering_StopBuffering(IMFByteStreamBuffering *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStreamBuffering(iface);

    TRACE("%p.\n", iface);

    return IMFByteStreamBuffering_StopBuffering(wrapper->stream_buffering);
}

static const IMFByteStreamBufferingVtbl bytestream_wrapper_buffering_vtbl =
{
    bytestream_wrapper_buffering_QueryInterface,
    bytestream_wrapper_buffering_AddRef,
    bytestream_wrapper_buffering_Release,
    bytestream_wrapper_buffering_SetBufferingParams,
    bytestream_wrapper_buffering_EnableBuffering,
    bytestream_wrapper_buffering_StopBuffering,
};

static HRESULT WINAPI bytestream_wrapper_timeseek_QueryInterface(IMFByteStreamTimeSeek *iface,
        REFIID riid, void **obj)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStreamTimeSeek(iface);
    return IMFByteStream_QueryInterface(&wrapper->IMFByteStream_iface, riid, obj);
}

static ULONG WINAPI bytestream_wrapper_timeseek_AddRef(IMFByteStreamTimeSeek *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStreamTimeSeek(iface);
    return IMFByteStream_AddRef(&wrapper->IMFByteStream_iface);
}

static ULONG WINAPI bytestream_wrapper_timeseek_Release(IMFByteStreamTimeSeek *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStreamTimeSeek(iface);
    return IMFByteStream_Release(&wrapper->IMFByteStream_iface);
}

static HRESULT WINAPI bytestream_wrapper_timeseek_IsTimeSeekSupported(IMFByteStreamTimeSeek *iface, BOOL *result)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStreamTimeSeek(iface);

    TRACE("%p, %p.\n", iface, result);

    return IMFByteStreamTimeSeek_IsTimeSeekSupported(wrapper->time_seek, result);
}

static HRESULT WINAPI bytestream_wrapper_timeseek_TimeSeek(IMFByteStreamTimeSeek *iface, QWORD position)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStreamTimeSeek(iface);

    TRACE("%p, %s.\n", iface, wine_dbgstr_longlong(position));

    return IMFByteStreamTimeSeek_TimeSeek(wrapper->time_seek, position);
}

static HRESULT WINAPI bytestream_wrapper_timeseek_GetTimeSeekResult(IMFByteStreamTimeSeek *iface, QWORD *start_time,
        QWORD *stop_time, QWORD *duration)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFByteStreamTimeSeek(iface);

    TRACE("%p, %p, %p, %p.\n", iface, start_time, stop_time, duration);

    return IMFByteStreamTimeSeek_GetTimeSeekResult(wrapper->time_seek, start_time, stop_time, duration);
}

static const IMFByteStreamTimeSeekVtbl bytestream_wrapper_timeseek_vtbl =
{
    bytestream_wrapper_timeseek_QueryInterface,
    bytestream_wrapper_timeseek_AddRef,
    bytestream_wrapper_timeseek_Release,
    bytestream_wrapper_timeseek_IsTimeSeekSupported,
    bytestream_wrapper_timeseek_TimeSeek,
    bytestream_wrapper_timeseek_GetTimeSeekResult,
};

static HRESULT WINAPI bytestream_wrapper_events_QueryInterface(IMFMediaEventGenerator *iface, REFIID riid, void **obj)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFMediaEventGenerator(iface);
    return IMFByteStream_QueryInterface(&wrapper->IMFByteStream_iface, riid, obj);
}

static ULONG WINAPI bytestream_wrapper_events_AddRef(IMFMediaEventGenerator *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFMediaEventGenerator(iface);
    return IMFByteStream_AddRef(&wrapper->IMFByteStream_iface);
}

static ULONG WINAPI bytestream_wrapper_events_Release(IMFMediaEventGenerator *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFMediaEventGenerator(iface);
    return IMFByteStream_Release(&wrapper->IMFByteStream_iface);
}

static HRESULT WINAPI bytestream_wrapper_events_GetEvent(IMFMediaEventGenerator *iface, DWORD flags, IMFMediaEvent **event)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %#x, %p.\n", iface, flags, event);

    return IMFMediaEventGenerator_GetEvent(wrapper->event_generator, flags, event);
}

static HRESULT WINAPI bytestream_wrapper_events_BeginGetEvent(IMFMediaEventGenerator *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    return IMFMediaEventGenerator_BeginGetEvent(wrapper->event_generator, callback, state);
}

static HRESULT WINAPI bytestream_wrapper_events_EndGetEvent(IMFMediaEventGenerator *iface, IMFAsyncResult *result, IMFMediaEvent **event)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    return IMFMediaEventGenerator_EndGetEvent(wrapper->event_generator, result, event);
}

static HRESULT WINAPI bytestream_wrapper_events_QueueEvent(IMFMediaEventGenerator *iface, MediaEventType type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %d, %s, %#x, %s.\n", iface, type, debugstr_guid(ext_type), hr, debugstr_propvar(value));

    return IMFMediaEventGenerator_QueueEvent(wrapper->event_generator, type, ext_type, hr, value);
}

static const IMFMediaEventGeneratorVtbl bytestream_wrapper_events_vtbl =
{
    bytestream_wrapper_events_QueryInterface,
    bytestream_wrapper_events_AddRef,
    bytestream_wrapper_events_Release,
    bytestream_wrapper_events_GetEvent,
    bytestream_wrapper_events_BeginGetEvent,
    bytestream_wrapper_events_EndGetEvent,
    bytestream_wrapper_events_QueueEvent,
};

static HRESULT WINAPI bytestream_wrapper_sample_output_QueryInterface(IMFSampleOutputStream *iface, REFIID riid, void **obj)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFSampleOutputStream(iface);
    return IMFByteStream_QueryInterface(&wrapper->IMFByteStream_iface, riid, obj);
}

static ULONG WINAPI bytestream_wrapper_sample_output_AddRef(IMFSampleOutputStream *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFSampleOutputStream(iface);
    return IMFByteStream_AddRef(&wrapper->IMFByteStream_iface);
}

static ULONG WINAPI bytestream_wrapper_sample_output_Release(IMFSampleOutputStream *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFSampleOutputStream(iface);
    return IMFByteStream_Release(&wrapper->IMFByteStream_iface);
}

static HRESULT WINAPI bytestream_wrapper_sample_output_BeginWriteSample(IMFSampleOutputStream *iface, IMFSample *sample,
        IMFAsyncCallback *callback, IUnknown *state)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFSampleOutputStream(iface);

    TRACE("%p, %p, %p, %p.\n", iface, sample, callback, state);

    return IMFSampleOutputStream_BeginWriteSample(wrapper->sample_output, sample, callback, state);
}

static HRESULT WINAPI bytestream_wrapper_sample_output_EndWriteSample(IMFSampleOutputStream *iface, IMFAsyncResult *result)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFSampleOutputStream(iface);

    TRACE("%p, %p.\n", iface, result);

    return IMFSampleOutputStream_EndWriteSample(wrapper->sample_output, result);
}

static HRESULT WINAPI bytestream_wrapper_sample_output_Close(IMFSampleOutputStream *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFSampleOutputStream(iface);

    TRACE("%p.\n", iface);

    return IMFSampleOutputStream_Close(wrapper->sample_output);
}

static const IMFSampleOutputStreamVtbl bytestream_wrapper_sample_output_vtbl =
{
    bytestream_wrapper_sample_output_QueryInterface,
    bytestream_wrapper_sample_output_AddRef,
    bytestream_wrapper_sample_output_Release,
    bytestream_wrapper_sample_output_BeginWriteSample,
    bytestream_wrapper_sample_output_EndWriteSample,
    bytestream_wrapper_sample_output_Close,
};

static HRESULT WINAPI bytestream_wrapper_propstore_QueryInterface(IPropertyStore *iface, REFIID riid, void **obj)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IPropertyStore(iface);
    return IMFByteStream_QueryInterface(&wrapper->IMFByteStream_iface, riid, obj);
}

static ULONG WINAPI bytestream_wrapper_propstore_AddRef(IPropertyStore *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IPropertyStore(iface);
    return IMFByteStream_AddRef(&wrapper->IMFByteStream_iface);
}

static ULONG WINAPI bytestream_wrapper_propstore_Release(IPropertyStore *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IPropertyStore(iface);
    return IMFByteStream_Release(&wrapper->IMFByteStream_iface);
}

static HRESULT WINAPI bytestream_wrapper_propstore_GetCount(IPropertyStore *iface, DWORD *count)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IPropertyStore(iface);

    TRACE("%p, %p.\n", iface, count);

    return IPropertyStore_GetCount(wrapper->propstore, count);
}

static HRESULT WINAPI bytestream_wrapper_propstore_GetAt(IPropertyStore *iface, DWORD prop, PROPERTYKEY *key)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IPropertyStore(iface);

    TRACE("%p, %u, %p.\n", iface, prop, key);

    return IPropertyStore_GetAt(wrapper->propstore, prop, key);
}

static HRESULT WINAPI bytestream_wrapper_propstore_GetValue(IPropertyStore *iface, REFPROPERTYKEY key, PROPVARIANT *value)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IPropertyStore(iface);

    TRACE("%p, %p, %p.\n", iface, key, value);

    return IPropertyStore_GetValue(wrapper->propstore, key, value);
}

static HRESULT WINAPI bytestream_wrapper_propstore_SetValue(IPropertyStore *iface, REFPROPERTYKEY key,
        const PROPVARIANT *value)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IPropertyStore(iface);

    TRACE("%p, %p, %s.\n", iface, key, debugstr_propvar(value));

    return IPropertyStore_SetValue(wrapper->propstore, key, value);
}

static HRESULT WINAPI bytestream_wrapper_propstore_Commit(IPropertyStore *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IPropertyStore(iface);

    TRACE("%p.\n", iface);

    return IPropertyStore_Commit(wrapper->propstore);
}

static const IPropertyStoreVtbl bytestream_wrapper_propstore_vtbl =
{
    bytestream_wrapper_propstore_QueryInterface,
    bytestream_wrapper_propstore_AddRef,
    bytestream_wrapper_propstore_Release,
    bytestream_wrapper_propstore_GetCount,
    bytestream_wrapper_propstore_GetAt,
    bytestream_wrapper_propstore_GetValue,
    bytestream_wrapper_propstore_SetValue,
    bytestream_wrapper_propstore_Commit,
};

static HRESULT WINAPI bytestream_wrapper_attributes_QueryInterface(IMFAttributes *iface, REFIID riid, void **obj)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);
    return IMFByteStream_QueryInterface(&wrapper->IMFByteStream_iface, riid, obj);
}

static ULONG WINAPI bytestream_wrapper_attributes_AddRef(IMFAttributes *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);
    return IMFByteStream_AddRef(&wrapper->IMFByteStream_iface);
}

static ULONG WINAPI bytestream_wrapper_attributes_Release(IMFAttributes *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);
    return IMFByteStream_Release(&wrapper->IMFByteStream_iface);
}

static HRESULT WINAPI bytestream_wrapper_attributes_GetItem(IMFAttributes *iface, REFGUID key, PROPVARIANT *value)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return IMFAttributes_GetItem(wrapper->attributes, key, value);
}

static HRESULT WINAPI bytestream_wrapper_attributes_GetItemType(IMFAttributes *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), type);

    return IMFAttributes_GetItemType(wrapper->attributes, key, type);
}

static HRESULT WINAPI bytestream_wrapper_attributes_CompareItem(IMFAttributes *iface, REFGUID key,
        REFPROPVARIANT value, BOOL *result)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_propvar(value), result);

    return IMFAttributes_CompareItem(wrapper->attributes, key, value, result);
}

static HRESULT WINAPI bytestream_wrapper_attributes_Compare(IMFAttributes *iface, IMFAttributes *theirs,
        MF_ATTRIBUTES_MATCH_TYPE match_type, BOOL *ret)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %p, %d, %p.\n", iface, theirs, match_type, ret);

    return IMFAttributes_Compare(wrapper->attributes, theirs, match_type, ret);
}

static HRESULT WINAPI bytestream_wrapper_attributes_GetUINT32(IMFAttributes *iface, REFGUID key, UINT32 *value)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return IMFAttributes_GetUINT32(wrapper->attributes, key, value);
}

static HRESULT WINAPI bytestream_wrapper_attributes_GetUINT64(IMFAttributes *iface, REFGUID key, UINT64 *value)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return IMFAttributes_GetUINT64(wrapper->attributes, key, value);
}

static HRESULT WINAPI bytestream_wrapper_attributes_GetDouble(IMFAttributes *iface, REFGUID key, double *value)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return IMFAttributes_GetDouble(wrapper->attributes, key, value);
}

static HRESULT WINAPI bytestream_wrapper_attributes_GetGUID(IMFAttributes *iface, REFGUID key, GUID *value)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return IMFAttributes_GetGUID(wrapper->attributes, key, value);
}

static HRESULT WINAPI bytestream_wrapper_attributes_GetStringLength(IMFAttributes *iface, REFGUID key, UINT32 *length)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), length);

    return IMFAttributes_GetStringLength(wrapper->attributes, key, length);
}

static HRESULT WINAPI bytestream_wrapper_attributes_GetString(IMFAttributes *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_attr(key), value, size, length);

    return IMFAttributes_GetString(wrapper->attributes, key, value, size, length);
}

static HRESULT WINAPI bytestream_wrapper_attributes_GetAllocatedString(IMFAttributes *iface, REFGUID key, WCHAR **value, UINT32 *length)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), value, length);

    return IMFAttributes_GetAllocatedString(wrapper->attributes, key, value, length);
}

static HRESULT WINAPI bytestream_wrapper_attributes_GetBlobSize(IMFAttributes *iface, REFGUID key, UINT32 *size)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), size);

    return IMFAttributes_GetBlobSize(wrapper->attributes, key, size);
}

static HRESULT WINAPI bytestream_wrapper_attributes_GetBlob(IMFAttributes *iface, REFGUID key, UINT8 *buf,
                UINT32 bufsize, UINT32 *blobsize)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_attr(key), buf, bufsize, blobsize);

    return IMFAttributes_GetBlob(wrapper->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI bytestream_wrapper_attributes_GetAllocatedBlob(IMFAttributes *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), buf, size);

    return IMFAttributes_GetAllocatedBlob(wrapper->attributes, key, buf, size);
}

static HRESULT WINAPI bytestream_wrapper_attributes_GetUnknown(IMFAttributes *iface, REFGUID key, REFIID riid, void **obj)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_guid(riid), obj);

    return IMFAttributes_GetUnknown(wrapper->attributes, key, riid, obj);
}

static HRESULT WINAPI bytestream_wrapper_attributes_SetItem(IMFAttributes *iface, REFGUID key, REFPROPVARIANT value)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_propvar(value));

    return IMFAttributes_SetItem(wrapper->attributes, key, value);
}

static HRESULT WINAPI bytestream_wrapper_attributes_DeleteItem(IMFAttributes *iface, REFGUID key)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s.\n", iface, debugstr_attr(key));

    return IMFAttributes_DeleteItem(wrapper->attributes, key);
}

static HRESULT WINAPI bytestream_wrapper_attributes_DeleteAllItems(IMFAttributes *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_DeleteAllItems(wrapper->attributes);
}

static HRESULT WINAPI bytestream_wrapper_attributes_SetUINT32(IMFAttributes *iface, REFGUID key, UINT32 value)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %d.\n", iface, debugstr_attr(key), value);

    return IMFAttributes_SetUINT32(wrapper->attributes, key, value);
}

static HRESULT WINAPI bytestream_wrapper_attributes_SetUINT64(IMFAttributes *iface, REFGUID key, UINT64 value)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), wine_dbgstr_longlong(value));

    return IMFAttributes_SetUINT64(wrapper->attributes, key, value);
}

static HRESULT WINAPI bytestream_wrapper_attributes_SetDouble(IMFAttributes *iface, REFGUID key, double value)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_attr(key), value);

    return IMFAttributes_SetDouble(wrapper->attributes, key, value);
}

static HRESULT WINAPI bytestream_wrapper_attributes_SetGUID(IMFAttributes *iface, REFGUID key, REFGUID value)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_mf_guid(value));

    return IMFAttributes_SetGUID(wrapper->attributes, key, value);
}

static HRESULT WINAPI bytestream_wrapper_attributes_SetString(IMFAttributes *iface, REFGUID key, const WCHAR *value)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_w(value));

    return IMFAttributes_SetString(wrapper->attributes, key, value);
}

static HRESULT WINAPI bytestream_wrapper_attributes_SetBlob(IMFAttributes *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %u.\n", iface, debugstr_attr(key), buf, size);

    return IMFAttributes_SetBlob(wrapper->attributes, key, buf, size);
}

static HRESULT WINAPI bytestream_wrapper_attributes_SetUnknown(IMFAttributes *iface, REFGUID key, IUnknown *unknown)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), unknown);

    return IMFAttributes_SetUnknown(wrapper->attributes, key, unknown);
}

static HRESULT WINAPI bytestream_wrapper_attributes_LockStore(IMFAttributes *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_LockStore(wrapper->attributes);
}

static HRESULT WINAPI bytestream_wrapper_attributes_UnlockStore(IMFAttributes *iface)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_UnlockStore(wrapper->attributes);
}

static HRESULT WINAPI bytestream_wrapper_attributes_GetCount(IMFAttributes *iface, UINT32 *count)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %p.\n", iface, count);

    return IMFAttributes_GetCount(wrapper->attributes, count);
}

static HRESULT WINAPI bytestream_wrapper_attributes_GetItemByIndex(IMFAttributes *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return IMFAttributes_GetItemByIndex(wrapper->attributes, index, key, value);
}

static HRESULT WINAPI bytestream_wrapper_attributes_CopyAllItems(IMFAttributes *iface, IMFAttributes *dest)
{
    struct bytestream_wrapper *wrapper = impl_wrapper_from_IMFAttributes(iface);

    TRACE("%p, %p.\n", iface, dest);

    return IMFAttributes_CopyAllItems(wrapper->attributes, dest);
}

static const IMFAttributesVtbl bytestream_wrapper_attributes_vtbl =
{
    bytestream_wrapper_attributes_QueryInterface,
    bytestream_wrapper_attributes_AddRef,
    bytestream_wrapper_attributes_Release,
    bytestream_wrapper_attributes_GetItem,
    bytestream_wrapper_attributes_GetItemType,
    bytestream_wrapper_attributes_CompareItem,
    bytestream_wrapper_attributes_Compare,
    bytestream_wrapper_attributes_GetUINT32,
    bytestream_wrapper_attributes_GetUINT64,
    bytestream_wrapper_attributes_GetDouble,
    bytestream_wrapper_attributes_GetGUID,
    bytestream_wrapper_attributes_GetStringLength,
    bytestream_wrapper_attributes_GetString,
    bytestream_wrapper_attributes_GetAllocatedString,
    bytestream_wrapper_attributes_GetBlobSize,
    bytestream_wrapper_attributes_GetBlob,
    bytestream_wrapper_attributes_GetAllocatedBlob,
    bytestream_wrapper_attributes_GetUnknown,
    bytestream_wrapper_attributes_SetItem,
    bytestream_wrapper_attributes_DeleteItem,
    bytestream_wrapper_attributes_DeleteAllItems,
    bytestream_wrapper_attributes_SetUINT32,
    bytestream_wrapper_attributes_SetUINT64,
    bytestream_wrapper_attributes_SetDouble,
    bytestream_wrapper_attributes_SetGUID,
    bytestream_wrapper_attributes_SetString,
    bytestream_wrapper_attributes_SetBlob,
    bytestream_wrapper_attributes_SetUnknown,
    bytestream_wrapper_attributes_LockStore,
    bytestream_wrapper_attributes_UnlockStore,
    bytestream_wrapper_attributes_GetCount,
    bytestream_wrapper_attributes_GetItemByIndex,
    bytestream_wrapper_attributes_CopyAllItems
};

/***********************************************************************
 *      MFCreateMFByteStreamWrapper (mfplat.@)
 */
HRESULT WINAPI MFCreateMFByteStreamWrapper(IMFByteStream *stream, IMFByteStream **wrapper)
{
    struct bytestream_wrapper *object;

    TRACE("%p, %p.\n", stream, wrapper);

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFByteStreamCacheControl_iface.lpVtbl = &bytestream_wrapper_cache_control_vtbl;
    object->IMFByteStreamBuffering_iface.lpVtbl = &bytestream_wrapper_buffering_vtbl;
    object->IMFMediaEventGenerator_iface.lpVtbl = &bytestream_wrapper_events_vtbl;
    object->IMFByteStreamTimeSeek_iface.lpVtbl = &bytestream_wrapper_timeseek_vtbl;
    object->IMFSampleOutputStream_iface.lpVtbl = &bytestream_wrapper_sample_output_vtbl;
    object->IMFByteStream_iface.lpVtbl = &bytestream_wrapper_vtbl;
    object->IPropertyStore_iface.lpVtbl = &bytestream_wrapper_propstore_vtbl;
    object->IMFAttributes_iface.lpVtbl = &bytestream_wrapper_attributes_vtbl;

    IMFByteStream_QueryInterface(stream, &IID_IMFByteStreamCacheControl, (void **)&object->cache_control);
    IMFByteStream_QueryInterface(stream, &IID_IMFByteStreamBuffering, (void **)&object->stream_buffering);
    IMFByteStream_QueryInterface(stream, &IID_IMFMediaEventGenerator, (void **)&object->event_generator);
    IMFByteStream_QueryInterface(stream, &IID_IMFByteStreamTimeSeek, (void **)&object->time_seek);
    IMFByteStream_QueryInterface(stream, &IID_IMFSampleOutputStream, (void **)&object->sample_output);
    IMFByteStream_QueryInterface(stream, &IID_IPropertyStore, (void **)&object->propstore);
    IMFByteStream_QueryInterface(stream, &IID_IMFAttributes, (void **)&object->attributes);

    object->stream = stream;
    IMFByteStream_AddRef(object->stream);

    object->refcount = 1;

    *wrapper = &object->IMFByteStream_iface;

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
    IRtwqAsyncResult *inner_result;
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

struct source_resolver
{
    IMFSourceResolver IMFSourceResolver_iface;
    LONG refcount;
    IRtwqAsyncCallback stream_callback;
    IRtwqAsyncCallback url_callback;
    CRITICAL_SECTION cs;
    struct list pending;
};

static struct source_resolver *impl_from_IMFSourceResolver(IMFSourceResolver *iface)
{
    return CONTAINING_RECORD(iface, struct source_resolver, IMFSourceResolver_iface);
}

static struct source_resolver *impl_from_stream_IRtwqAsyncCallback(IRtwqAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct source_resolver, stream_callback);
}

static struct source_resolver *impl_from_url_IRtwqAsyncCallback(IRtwqAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct source_resolver, url_callback);
}

static HRESULT resolver_handler_end_create(struct source_resolver *resolver, enum resolved_object_origin origin,
        IRtwqAsyncResult *result)
{
    IRtwqAsyncResult *inner_result = (IRtwqAsyncResult *)IRtwqAsyncResult_GetStateNoAddRef(result);
    RTWQASYNCRESULT *data = (RTWQASYNCRESULT *)inner_result;
    struct resolver_queued_result *queued_result;
    union
    {
        IUnknown *handler;
        IMFByteStreamHandler *stream_handler;
        IMFSchemeHandler *scheme_handler;
    } handler;

    if (!(queued_result = heap_alloc_zero(sizeof(*queued_result))))
        return E_OUTOFMEMORY;

    IRtwqAsyncResult_GetObject(inner_result, &handler.handler);

    switch (origin)
    {
        case OBJECT_FROM_BYTESTREAM:
            queued_result->hr = IMFByteStreamHandler_EndCreateObject(handler.stream_handler, (IMFAsyncResult *)result,
                    &queued_result->obj_type, &queued_result->object);
            break;
        case OBJECT_FROM_URL:
            queued_result->hr = IMFSchemeHandler_EndCreateObject(handler.scheme_handler, (IMFAsyncResult *)result,
                    &queued_result->obj_type, &queued_result->object);
            break;
        default:
            queued_result->hr = E_FAIL;
    }

    IUnknown_Release(handler.handler);

    if (data->hEvent)
    {
        queued_result->inner_result = inner_result;
        IRtwqAsyncResult_AddRef(queued_result->inner_result);
    }

    /* Push resolved object type and created object, so we don't have to guess on End*() call. */
    EnterCriticalSection(&resolver->cs);
    list_add_tail(&resolver->pending, &queued_result->entry);
    LeaveCriticalSection(&resolver->cs);

    if (data->hEvent)
        SetEvent(data->hEvent);
    else
    {
        IUnknown *caller_state = IRtwqAsyncResult_GetStateNoAddRef(inner_result);
        IRtwqAsyncResult *caller_result;

        if (SUCCEEDED(RtwqCreateAsyncResult(queued_result->object, data->pCallback, caller_state, &caller_result)))
        {
            RtwqInvokeCallback(caller_result);
            IRtwqAsyncResult_Release(caller_result);
        }
    }

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

static HRESULT WINAPI source_resolver_callback_QueryInterface(IRtwqAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IRtwqAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IRtwqAsyncCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI source_resolver_callback_stream_AddRef(IRtwqAsyncCallback *iface)
{
    struct source_resolver *resolver = impl_from_stream_IRtwqAsyncCallback(iface);
    return IMFSourceResolver_AddRef(&resolver->IMFSourceResolver_iface);
}

static ULONG WINAPI source_resolver_callback_stream_Release(IRtwqAsyncCallback *iface)
{
    struct source_resolver *resolver = impl_from_stream_IRtwqAsyncCallback(iface);
    return IMFSourceResolver_Release(&resolver->IMFSourceResolver_iface);
}

static HRESULT WINAPI source_resolver_callback_GetParameters(IRtwqAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI source_resolver_callback_stream_Invoke(IRtwqAsyncCallback *iface, IRtwqAsyncResult *result)
{
    struct source_resolver *resolver = impl_from_stream_IRtwqAsyncCallback(iface);

    return resolver_handler_end_create(resolver, OBJECT_FROM_BYTESTREAM, result);
}

static const IRtwqAsyncCallbackVtbl source_resolver_callback_stream_vtbl =
{
    source_resolver_callback_QueryInterface,
    source_resolver_callback_stream_AddRef,
    source_resolver_callback_stream_Release,
    source_resolver_callback_GetParameters,
    source_resolver_callback_stream_Invoke,
};

static ULONG WINAPI source_resolver_callback_url_AddRef(IRtwqAsyncCallback *iface)
{
    struct source_resolver *resolver = impl_from_url_IRtwqAsyncCallback(iface);
    return IMFSourceResolver_AddRef(&resolver->IMFSourceResolver_iface);
}

static ULONG WINAPI source_resolver_callback_url_Release(IRtwqAsyncCallback *iface)
{
    struct source_resolver *resolver = impl_from_url_IRtwqAsyncCallback(iface);
    return IMFSourceResolver_Release(&resolver->IMFSourceResolver_iface);
}

static HRESULT WINAPI source_resolver_callback_url_Invoke(IRtwqAsyncCallback *iface, IRtwqAsyncResult *result)
{
    struct source_resolver *resolver = impl_from_url_IRtwqAsyncCallback(iface);

    return resolver_handler_end_create(resolver, OBJECT_FROM_URL, result);
}

static const IRtwqAsyncCallbackVtbl source_resolver_callback_url_vtbl =
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
    HRESULT hr = E_FAIL;
    unsigned int i, j;
    UINT32 length;

    *handler = NULL;

    /* MIME type */
    if (SUCCEEDED(IMFByteStream_QueryInterface(stream, &IID_IMFAttributes, (void **)&attributes)))
    {
        IMFAttributes_GetAllocatedString(attributes, &MF_BYTESTREAM_CONTENT_TYPE, &mimeW, &length);
        IMFAttributes_Release(attributes);
    }

    /* Extension */
    url_ext = url ? wcsrchr(url, '.') : NULL;

    if (!url_ext && !mimeW)
    {
        CoTaskMemFree(mimeW);
        return MF_E_UNSUPPORTED_BYTESTREAM_TYPE;
    }

    if (!(flags & MF_RESOLUTION_DISABLE_LOCAL_PLUGINS))
    {
        struct local_handler *local_handler;

        EnterCriticalSection(&local_handlers_section);

        LIST_FOR_EACH_ENTRY(local_handler, &local_bytestream_handlers, struct local_handler, entry)
        {
            if ((mimeW && !lstrcmpiW(mimeW, local_handler->u.bytestream.mime))
                    || (url_ext && !lstrcmpiW(url_ext, local_handler->u.bytestream.extension)))
            {
                if (SUCCEEDED(hr = IMFActivate_ActivateObject(local_handler->activate, &IID_IMFByteStreamHandler,
                        (void **)handler)))
                    break;
            }
        }

        LeaveCriticalSection(&local_handlers_section);

        if (*handler)
            return hr;
    }

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

static HRESULT resolver_create_scheme_handler(const WCHAR *scheme, DWORD flags, IMFSchemeHandler **handler)
{
    static const char schemehandlerspath[] = "Software\\Microsoft\\Windows Media Foundation\\SchemeHandlers";
    static const HKEY hkey_roots[2] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
    HRESULT hr = MF_E_UNSUPPORTED_SCHEME;
    unsigned int i;

    TRACE("%s, %#x, %p.\n", debugstr_w(scheme), flags, handler);

    *handler = NULL;

    if (!(flags & MF_RESOLUTION_DISABLE_LOCAL_PLUGINS))
    {
        struct local_handler *local_handler;

        EnterCriticalSection(&local_handlers_section);

        LIST_FOR_EACH_ENTRY(local_handler, &local_scheme_handlers, struct local_handler, entry)
        {
            if (!lstrcmpiW(scheme, local_handler->u.scheme))
            {
                if (SUCCEEDED(hr = IMFActivate_ActivateObject(local_handler->activate, &IID_IMFSchemeHandler,
                        (void **)handler)))
                    break;
            }
        }

        LeaveCriticalSection(&local_handlers_section);

        if (*handler)
            return hr;
    }

    for (i = 0; i < ARRAY_SIZE(hkey_roots); ++i)
    {
        HKEY hkey, hkey_handler;

        hr = MF_E_UNSUPPORTED_SCHEME;

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

    return hr;
}

static HRESULT resolver_get_scheme_handler(const WCHAR *url, DWORD flags, IMFSchemeHandler **handler)
{
    static const WCHAR fileschemeW[] = {'f','i','l','e',':',0};
    const WCHAR *ptr = url;
    unsigned int len;
    WCHAR *scheme;
    HRESULT hr;

    /* RFC 3986: scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." ) */
    while (*ptr)
    {
        WCHAR ch = towlower(*ptr);

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

    /* Schemes must end with a ':', if not found try "file:" */
    if (ptr == url || *ptr != ':')
    {
        url = fileschemeW;
        ptr = fileschemeW + ARRAY_SIZE(fileschemeW) - 1;
    }

    len = ptr - url;
    scheme = heap_alloc((len + 1) * sizeof(WCHAR));
    if (!scheme)
        return E_OUTOFMEMORY;

    memcpy(scheme, url, len * sizeof(WCHAR));
    scheme[len] = 0;

    hr = resolver_create_scheme_handler(scheme, flags, handler);
    if (FAILED(hr) && url != fileschemeW)
        hr = resolver_create_scheme_handler(fileschemeW, flags, handler);

    heap_free(scheme);

    return hr;
}

static HRESULT resolver_end_create_object(struct source_resolver *resolver, enum resolved_object_origin origin,
        IRtwqAsyncResult *result, MF_OBJECT_TYPE *obj_type, IUnknown **out)
{
    struct resolver_queued_result *queued_result = NULL, *iter;
    IUnknown *object;
    HRESULT hr;

    if (FAILED(hr = IRtwqAsyncResult_GetObject(result, &object)))
        return hr;

    EnterCriticalSection(&resolver->cs);

    LIST_FOR_EACH_ENTRY(iter, &resolver->pending, struct resolver_queued_result, entry)
    {
        if (iter->inner_result == result || (iter->object == object && iter->origin == origin))
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
        if (queued_result->inner_result)
            IRtwqAsyncResult_Release(queued_result->inner_result);
        heap_free(queued_result);
    }
    else
        hr = E_UNEXPECTED;

    return hr;
}

static HRESULT WINAPI source_resolver_QueryInterface(IMFSourceResolver *iface, REFIID riid, void **obj)
{
    struct source_resolver *resolver = impl_from_IMFSourceResolver(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFSourceResolver) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &resolver->IMFSourceResolver_iface;
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
    IRtwqAsyncResult *result;
    RTWQASYNCRESULT *data;
    HRESULT hr;

    TRACE("%p, %s, %#x, %p, %p, %p.\n", iface, debugstr_w(url), flags, props, obj_type, object);

    if (!url || !obj_type || !object)
        return E_POINTER;

    if (FAILED(hr = resolver_get_scheme_handler(url, flags, &handler)))
        return hr;

    hr = RtwqCreateAsyncResult((IUnknown *)handler, NULL, NULL, &result);
    IMFSchemeHandler_Release(handler);
    if (FAILED(hr))
        return hr;

    data = (RTWQASYNCRESULT *)result;
    data->hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    hr = IMFSchemeHandler_BeginCreateObject(handler, url, flags, props, NULL, (IMFAsyncCallback *)&resolver->url_callback,
            (IUnknown *)result);
    if (FAILED(hr))
    {
        IRtwqAsyncResult_Release(result);
        return hr;
    }

    WaitForSingleObject(data->hEvent, INFINITE);

    hr = resolver_end_create_object(resolver, OBJECT_FROM_URL, result, obj_type, object);
    IRtwqAsyncResult_Release(result);

    return hr;
}

static HRESULT WINAPI source_resolver_CreateObjectFromByteStream(IMFSourceResolver *iface, IMFByteStream *stream,
    const WCHAR *url, DWORD flags, IPropertyStore *props, MF_OBJECT_TYPE *obj_type, IUnknown **object)
{
    struct source_resolver *resolver = impl_from_IMFSourceResolver(iface);
    IMFByteStreamHandler *handler;
    IRtwqAsyncResult *result;
    RTWQASYNCRESULT *data;
    HRESULT hr;

    TRACE("%p, %p, %s, %#x, %p, %p, %p.\n", iface, stream, debugstr_w(url), flags, props, obj_type, object);

    if (!stream || !obj_type || !object)
        return E_POINTER;

    if (FAILED(hr = resolver_get_bytestream_handler(stream, url, flags, &handler)))
        return MF_E_UNSUPPORTED_BYTESTREAM_TYPE;

    hr = RtwqCreateAsyncResult((IUnknown *)handler, NULL, NULL, &result);
    IMFByteStreamHandler_Release(handler);
    if (FAILED(hr))
        return hr;

    data = (RTWQASYNCRESULT *)result;
    data->hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    hr = IMFByteStreamHandler_BeginCreateObject(handler, stream, url, flags, props, NULL,
            (IMFAsyncCallback *)&resolver->stream_callback, (IUnknown *)result);
    if (FAILED(hr))
    {
        IRtwqAsyncResult_Release(result);
        return hr;
    }

    WaitForSingleObject(data->hEvent, INFINITE);

    hr = resolver_end_create_object(resolver, OBJECT_FROM_BYTESTREAM, result, obj_type, object);
    IRtwqAsyncResult_Release(result);

    return hr;
}

static HRESULT WINAPI source_resolver_BeginCreateObjectFromURL(IMFSourceResolver *iface, const WCHAR *url,
        DWORD flags, IPropertyStore *props, IUnknown **cancel_cookie, IMFAsyncCallback *callback, IUnknown *state)
{
    struct source_resolver *resolver = impl_from_IMFSourceResolver(iface);
    IMFSchemeHandler *handler;
    IUnknown *inner_cookie = NULL;
    IRtwqAsyncResult *result;
    HRESULT hr;

    TRACE("%p, %s, %#x, %p, %p, %p, %p.\n", iface, debugstr_w(url), flags, props, cancel_cookie, callback, state);

    if (FAILED(hr = resolver_get_scheme_handler(url, flags, &handler)))
        return hr;

    if (cancel_cookie)
        *cancel_cookie = NULL;

    hr = RtwqCreateAsyncResult((IUnknown *)handler, (IRtwqAsyncCallback *)callback, state, &result);
    IMFSchemeHandler_Release(handler);
    if (FAILED(hr))
        return hr;

    hr = IMFSchemeHandler_BeginCreateObject(handler, url, flags, props, cancel_cookie ? &inner_cookie : NULL,
            (IMFAsyncCallback *)&resolver->url_callback, (IUnknown *)result);

    if (SUCCEEDED(hr) && inner_cookie)
        resolver_create_cancel_object((IUnknown *)handler, OBJECT_FROM_URL, inner_cookie, cancel_cookie);

    IRtwqAsyncResult_Release(result);

    return hr;
}

static HRESULT WINAPI source_resolver_EndCreateObjectFromURL(IMFSourceResolver *iface, IMFAsyncResult *result,
        MF_OBJECT_TYPE *obj_type, IUnknown **object)
{
    struct source_resolver *resolver = impl_from_IMFSourceResolver(iface);

    TRACE("%p, %p, %p, %p.\n", iface, result, obj_type, object);

    return resolver_end_create_object(resolver, OBJECT_FROM_URL, (IRtwqAsyncResult *)result, obj_type, object);
}

static HRESULT WINAPI source_resolver_BeginCreateObjectFromByteStream(IMFSourceResolver *iface, IMFByteStream *stream,
        const WCHAR *url, DWORD flags, IPropertyStore *props, IUnknown **cancel_cookie, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct source_resolver *resolver = impl_from_IMFSourceResolver(iface);
    IMFByteStreamHandler *handler;
    IUnknown *inner_cookie = NULL;
    IRtwqAsyncResult *result;
    HRESULT hr;

    TRACE("%p, %p, %s, %#x, %p, %p, %p, %p.\n", iface, stream, debugstr_w(url), flags, props, cancel_cookie,
            callback, state);

    if (FAILED(hr = resolver_get_bytestream_handler(stream, url, flags, &handler)))
        return hr;

    if (cancel_cookie)
        *cancel_cookie = NULL;

    hr = RtwqCreateAsyncResult((IUnknown *)handler, (IRtwqAsyncCallback *)callback, state, &result);
    IMFByteStreamHandler_Release(handler);
    if (FAILED(hr))
        return hr;

    hr = IMFByteStreamHandler_BeginCreateObject(handler, stream, url, flags, props,
            cancel_cookie ? &inner_cookie : NULL, (IMFAsyncCallback *)&resolver->stream_callback, (IUnknown *)result);

    /* Cancel object wraps underlying handler cancel cookie with context necessary to call CancelObjectCreate(). */
    if (SUCCEEDED(hr) && inner_cookie)
        resolver_create_cancel_object((IUnknown *)handler, OBJECT_FROM_BYTESTREAM, inner_cookie, cancel_cookie);

    IRtwqAsyncResult_Release(result);

    return hr;
}

static HRESULT WINAPI source_resolver_EndCreateObjectFromByteStream(IMFSourceResolver *iface, IMFAsyncResult *result,
        MF_OBJECT_TYPE *obj_type, IUnknown **object)
{
    struct source_resolver *resolver = impl_from_IMFSourceResolver(iface);

    TRACE("%p, %p, %p, %p.\n", iface, result, obj_type, object);

    return resolver_end_create_object(resolver, OBJECT_FROM_BYTESTREAM, (IRtwqAsyncResult *)result, obj_type, object);
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

struct media_event
{
    struct attributes attributes;
    IMFMediaEvent IMFMediaEvent_iface;

    MediaEventType type;
    GUID extended_type;
    HRESULT status;
    PROPVARIANT value;
};

static inline struct media_event *impl_from_IMFMediaEvent(IMFMediaEvent *iface)
{
    return CONTAINING_RECORD(iface, struct media_event, IMFMediaEvent_iface);
}

static HRESULT WINAPI mfmediaevent_QueryInterface(IMFMediaEvent *iface, REFIID riid, void **out)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IMFAttributes) ||
       IsEqualGUID(riid, &IID_IMFMediaEvent))
    {
        *out = &event->IMFMediaEvent_iface;
    }
    else
    {
        FIXME("%s, %p.\n", debugstr_guid(riid), out);
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static ULONG WINAPI mfmediaevent_AddRef(IMFMediaEvent *iface)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);
    ULONG refcount = InterlockedIncrement(&event->attributes.ref);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI mfmediaevent_Release(IMFMediaEvent *iface)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);
    ULONG refcount = InterlockedDecrement(&event->attributes.ref);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        clear_attributes_object(&event->attributes);
        PropVariantClear(&event->value);
        heap_free(event);
    }

    return refcount;
}

static HRESULT WINAPI mfmediaevent_GetItem(IMFMediaEvent *iface, REFGUID key, PROPVARIANT *value)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetItem(&event->attributes, key, value);
}

static HRESULT WINAPI mfmediaevent_GetItemType(IMFMediaEvent *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), type);

    return attributes_GetItemType(&event->attributes, key, type);
}

static HRESULT WINAPI mfmediaevent_CompareItem(IMFMediaEvent *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_propvar(value), result);

    return attributes_CompareItem(&event->attributes, key, value, result);
}

static HRESULT WINAPI mfmediaevent_Compare(IMFMediaEvent *iface, IMFAttributes *attrs, MF_ATTRIBUTES_MATCH_TYPE type,
        BOOL *result)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %p, %d, %p.\n", iface, attrs, type, result);

    return attributes_Compare(&event->attributes, attrs, type, result);
}

static HRESULT WINAPI mfmediaevent_GetUINT32(IMFMediaEvent *iface, REFGUID key, UINT32 *value)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT32(&event->attributes, key, value);
}

static HRESULT WINAPI mfmediaevent_GetUINT64(IMFMediaEvent *iface, REFGUID key, UINT64 *value)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT64(&event->attributes, key, value);
}

static HRESULT WINAPI mfmediaevent_GetDouble(IMFMediaEvent *iface, REFGUID key, double *value)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetDouble(&event->attributes, key, value);
}

static HRESULT WINAPI mfmediaevent_GetGUID(IMFMediaEvent *iface, REFGUID key, GUID *value)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetGUID(&event->attributes, key, value);
}

static HRESULT WINAPI mfmediaevent_GetStringLength(IMFMediaEvent *iface, REFGUID key, UINT32 *length)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), length);

    return attributes_GetStringLength(&event->attributes, key, length);
}

static HRESULT WINAPI mfmediaevent_GetString(IMFMediaEvent *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_attr(key), value, size, length);

    return attributes_GetString(&event->attributes, key, value, size, length);
}

static HRESULT WINAPI mfmediaevent_GetAllocatedString(IMFMediaEvent *iface, REFGUID key,
        WCHAR **value, UINT32 *length)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), value, length);

    return attributes_GetAllocatedString(&event->attributes, key, value, length);
}

static HRESULT WINAPI mfmediaevent_GetBlobSize(IMFMediaEvent *iface, REFGUID key, UINT32 *size)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), size);

    return attributes_GetBlobSize(&event->attributes, key, size);
}

static HRESULT WINAPI mfmediaevent_GetBlob(IMFMediaEvent *iface, REFGUID key, UINT8 *buf,
        UINT32 bufsize, UINT32 *blobsize)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_attr(key), buf, bufsize, blobsize);

    return attributes_GetBlob(&event->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI mfmediaevent_GetAllocatedBlob(IMFMediaEvent *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), buf, size);

    return attributes_GetAllocatedBlob(&event->attributes, key, buf, size);
}

static HRESULT WINAPI mfmediaevent_GetUnknown(IMFMediaEvent *iface, REFGUID key, REFIID riid, void **out)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_guid(riid), out);

    return attributes_GetUnknown(&event->attributes, key, riid, out);
}

static HRESULT WINAPI mfmediaevent_SetItem(IMFMediaEvent *iface, REFGUID key, REFPROPVARIANT value)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_propvar(value));

    return attributes_SetItem(&event->attributes, key, value);
}

static HRESULT WINAPI mfmediaevent_DeleteItem(IMFMediaEvent *iface, REFGUID key)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s.\n", iface, debugstr_attr(key));

    return attributes_DeleteItem(&event->attributes, key);
}

static HRESULT WINAPI mfmediaevent_DeleteAllItems(IMFMediaEvent *iface)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p.\n", iface);

    return attributes_DeleteAllItems(&event->attributes);
}

static HRESULT WINAPI mfmediaevent_SetUINT32(IMFMediaEvent *iface, REFGUID key, UINT32 value)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %u.\n", iface, debugstr_attr(key), value);

    return attributes_SetUINT32(&event->attributes, key, value);
}

static HRESULT WINAPI mfmediaevent_SetUINT64(IMFMediaEvent *iface, REFGUID key, UINT64 value)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), wine_dbgstr_longlong(value));

    return attributes_SetUINT64(&event->attributes, key, value);
}

static HRESULT WINAPI mfmediaevent_SetDouble(IMFMediaEvent *iface, REFGUID key, double value)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_attr(key), value);

    return attributes_SetDouble(&event->attributes, key, value);
}

static HRESULT WINAPI mfmediaevent_SetGUID(IMFMediaEvent *iface, REFGUID key, REFGUID value)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_mf_guid(value));

    return attributes_SetGUID(&event->attributes, key, value);
}

static HRESULT WINAPI mfmediaevent_SetString(IMFMediaEvent *iface, REFGUID key, const WCHAR *value)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_w(value));

    return attributes_SetString(&event->attributes, key, value);
}

static HRESULT WINAPI mfmediaevent_SetBlob(IMFMediaEvent *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %p, %u.\n", iface, debugstr_attr(key), buf, size);

    return attributes_SetBlob(&event->attributes, key, buf, size);
}

static HRESULT WINAPI mfmediaevent_SetUnknown(IMFMediaEvent *iface, REFGUID key, IUnknown *unknown)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), unknown);

    return attributes_SetUnknown(&event->attributes, key, unknown);
}

static HRESULT WINAPI mfmediaevent_LockStore(IMFMediaEvent *iface)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p.\n", iface);

    return attributes_LockStore(&event->attributes);
}

static HRESULT WINAPI mfmediaevent_UnlockStore(IMFMediaEvent *iface)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p.\n", iface);

    return attributes_UnlockStore(&event->attributes);
}

static HRESULT WINAPI mfmediaevent_GetCount(IMFMediaEvent *iface, UINT32 *count)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %p.\n", iface, count);

    return attributes_GetCount(&event->attributes, count);
}

static HRESULT WINAPI mfmediaevent_GetItemByIndex(IMFMediaEvent *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return attributes_GetItemByIndex(&event->attributes, index, key, value);
}

static HRESULT WINAPI mfmediaevent_CopyAllItems(IMFMediaEvent *iface, IMFAttributes *dest)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %p.\n", iface, dest);

    return attributes_CopyAllItems(&event->attributes, dest);
}

static HRESULT WINAPI mfmediaevent_GetType(IMFMediaEvent *iface, MediaEventType *type)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %p.\n", iface, type);

    *type = event->type;

    return S_OK;
}

static HRESULT WINAPI mfmediaevent_GetExtendedType(IMFMediaEvent *iface, GUID *extended_type)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %p.\n", iface, extended_type);

    *extended_type = event->extended_type;

    return S_OK;
}

static HRESULT WINAPI mfmediaevent_GetStatus(IMFMediaEvent *iface, HRESULT *status)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %p.\n", iface, status);

    *status = event->status;

    return S_OK;
}

static HRESULT WINAPI mfmediaevent_GetValue(IMFMediaEvent *iface, PROPVARIANT *value)
{
    struct media_event *event = impl_from_IMFMediaEvent(iface);

    TRACE("%p, %p.\n", iface, value);

    PropVariantCopy(value, &event->value);

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
HRESULT WINAPI MFCreateMediaEvent(MediaEventType type, REFGUID extended_type, HRESULT status, const PROPVARIANT *value,
        IMFMediaEvent **event)
{
    struct media_event *object;
    HRESULT hr;

    TRACE("%s, %s, %#x, %s, %p.\n", debugstr_eventid(type), debugstr_guid(extended_type), status,
            debugstr_propvar(value), event);

    object = heap_alloc(sizeof(*object));
    if (!object)
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

    TRACE("Created event %p.\n", *event);

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
    BOOL notified;
    IRtwqAsyncResult *subscriber;
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
    if (list_empty(&queue->events) || !queue->subscriber || queue->notified)
        return;

    queue->notified = TRUE;
    RtwqPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, 0, queue->subscriber);
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
            hr = IRtwqAsyncResult_GetStateNoAddRef(queue->subscriber) == state ?
                    MF_S_MULTIPLE_BEGIN : MF_E_MULTIPLE_BEGIN;
        else
            hr = MF_E_MULTIPLE_SUBSCRIBERS;
    }
    else
    {
        hr = RtwqCreateAsyncResult(NULL, (IRtwqAsyncCallback *)callback, state, &queue->subscriber);
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
    else if (queue->subscriber == (IRtwqAsyncResult *)result)
    {
        *event = queue_pop_event(queue);
        if (queue->subscriber)
            IRtwqAsyncResult_Release(queue->subscriber);
        queue->subscriber = NULL;
        queue->notified = FALSE;
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

    TRACE("%p, %s, %s, %#x, %s\n", iface, debugstr_eventid(event_type), debugstr_guid(extended_type), status,
            debugstr_propvar(value));

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

    TRACE("%p, %s, %s, %#x, %p.\n", iface, debugstr_eventid(event_type), debugstr_guid(extended_type), status, unk);

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

static HRESULT WINAPI system_clock_QueryInterface(IMFClock *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFClock) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFClock_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI system_clock_AddRef(IMFClock *iface)
{
    struct system_clock *clock = impl_from_IMFClock(iface);
    ULONG refcount = InterlockedIncrement(&clock->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI system_clock_Release(IMFClock *iface)
{
    struct system_clock *clock = impl_from_IMFClock(iface);
    ULONG refcount = InterlockedDecrement(&clock->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
        heap_free(clock);

    return refcount;
}

static HRESULT WINAPI system_clock_GetClockCharacteristics(IMFClock *iface, DWORD *flags)
{
    TRACE("%p, %p.\n", iface, flags);

    *flags = MFCLOCK_CHARACTERISTICS_FLAG_FREQUENCY_10MHZ | MFCLOCK_CHARACTERISTICS_FLAG_ALWAYS_RUNNING |
            MFCLOCK_CHARACTERISTICS_FLAG_IS_SYSTEM_CLOCK;

    return S_OK;
}

static HRESULT WINAPI system_clock_GetCorrelatedTime(IMFClock *iface, DWORD reserved, LONGLONG *clock_time,
        MFTIME *system_time)
{
    TRACE("%p, %#x, %p, %p.\n", iface, reserved, clock_time, system_time);

    *clock_time = *system_time = MFGetSystemTime();

    return S_OK;
}

static HRESULT WINAPI system_clock_GetContinuityKey(IMFClock *iface, DWORD *key)
{
    TRACE("%p, %p.\n", iface, key);

    *key = 0;

    return S_OK;
}

static HRESULT WINAPI system_clock_GetState(IMFClock *iface, DWORD reserved, MFCLOCK_STATE *state)
{
    TRACE("%p, %#x, %p.\n", iface, reserved, state);

    *state = MFCLOCK_STATE_RUNNING;

    return S_OK;
}

static HRESULT WINAPI system_clock_GetProperties(IMFClock *iface, MFCLOCK_PROPERTIES *props)
{
    TRACE("%p, %p.\n", iface, props);

    if (!props)
        return E_POINTER;

    memset(props, 0, sizeof(*props));
    props->qwClockFrequency = MFCLOCK_FREQUENCY_HNS;
    props->dwClockTolerance = MFCLOCK_TOLERANCE_UNKNOWN;
    props->dwClockJitter = 1;

    return S_OK;
}

static const IMFClockVtbl system_clock_vtbl =
{
    system_clock_QueryInterface,
    system_clock_AddRef,
    system_clock_Release,
    system_clock_GetClockCharacteristics,
    system_clock_GetCorrelatedTime,
    system_clock_GetContinuityKey,
    system_clock_GetState,
    system_clock_GetProperties,
};

static HRESULT create_system_clock(IMFClock **clock)
{
    struct system_clock *object;

    if (!(object = heap_alloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFClock_iface.lpVtbl = &system_clock_vtbl;
    object->refcount = 1;

    *clock = &object->IMFClock_iface;

    return S_OK;
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
        if (source->clock)
            IMFClock_Release(source->clock);
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
    struct system_time_source *source = impl_from_IMFPresentationTimeSource(iface);
    HRESULT hr;

    TRACE("%p, %#x, %p, %p.\n", iface, reserved, clock_time, system_time);

    EnterCriticalSection(&source->cs);
    if (SUCCEEDED(hr = IMFClock_GetCorrelatedTime(source->clock, 0, clock_time, system_time)))
    {
        if (source->state == MFCLOCK_STATE_RUNNING)
        {
            system_time_source_apply_rate(source, clock_time);
            *clock_time += source->start_offset;
        }
        else
            *clock_time = source->start_offset;
    }
    LeaveCriticalSection(&source->cs);

    return hr;
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
    TRACE("%p, %p.\n", iface, props);

    if (!props)
        return E_POINTER;

    memset(props, 0, sizeof(*props));
    props->qwClockFrequency = MFCLOCK_FREQUENCY_HNS;
    props->dwClockTolerance = MFCLOCK_TOLERANCE_UNKNOWN;
    props->dwClockJitter = 1;

    return S_OK;
}

static HRESULT WINAPI system_time_source_GetUnderlyingClock(IMFPresentationTimeSource *iface, IMFClock **clock)
{
    struct system_time_source *source = impl_from_IMFPresentationTimeSource(iface);

    TRACE("%p, %p.\n", iface, clock);

    *clock = source->clock;
    IMFClock_AddRef(*clock);

    return S_OK;
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
    MFCLOCK_STATE state;
    HRESULT hr;

    TRACE("%p, %s, %s.\n", iface, debugstr_time(system_time), debugstr_time(start_offset));

    EnterCriticalSection(&source->cs);
    state = source->state;
    if (SUCCEEDED(hr = system_time_source_change_state(source, CLOCK_CMD_START)))
    {
        system_time_source_apply_rate(source, &system_time);
        if (start_offset == PRESENTATION_CURRENT_POSITION)
        {
            switch (state)
            {
                case MFCLOCK_STATE_RUNNING:
                    break;
                case MFCLOCK_STATE_PAUSED:
                    source->start_offset -= system_time;
                    break;
                default:
                    source->start_offset = -system_time;
                    break;
                    ;
            }
        }
        else
        {
            source->start_offset = -system_time + start_offset;
        }
    }
    LeaveCriticalSection(&source->cs);

    return hr;
}

static HRESULT WINAPI system_time_source_sink_OnClockStop(IMFClockStateSink *iface, MFTIME system_time)
{
    struct system_time_source *source = impl_from_IMFClockStateSink(iface);
    HRESULT hr;

    TRACE("%p, %s.\n", iface, debugstr_time(system_time));

    EnterCriticalSection(&source->cs);
    if (SUCCEEDED(hr = system_time_source_change_state(source, CLOCK_CMD_STOP)))
        source->start_offset = 0;
    LeaveCriticalSection(&source->cs);

    return hr;
}

static HRESULT WINAPI system_time_source_sink_OnClockPause(IMFClockStateSink *iface, MFTIME system_time)
{
    struct system_time_source *source = impl_from_IMFClockStateSink(iface);
    HRESULT hr;

    TRACE("%p, %s.\n", iface, debugstr_time(system_time));

    EnterCriticalSection(&source->cs);
    if (SUCCEEDED(hr = system_time_source_change_state(source, CLOCK_CMD_PAUSE)))
    {
        system_time_source_apply_rate(source, &system_time);
        source->start_offset += system_time;
    }
    LeaveCriticalSection(&source->cs);

    return hr;
}

static HRESULT WINAPI system_time_source_sink_OnClockRestart(IMFClockStateSink *iface, MFTIME system_time)
{
    struct system_time_source *source = impl_from_IMFClockStateSink(iface);
    HRESULT hr;

    TRACE("%p, %s.\n", iface, debugstr_time(system_time));

    EnterCriticalSection(&source->cs);
    if (SUCCEEDED(hr = system_time_source_change_state(source, CLOCK_CMD_RESTART)))
    {
        system_time_source_apply_rate(source, &system_time);
        source->start_offset -= system_time;
    }
    LeaveCriticalSection(&source->cs);

    return hr;
}

static HRESULT WINAPI system_time_source_sink_OnClockSetRate(IMFClockStateSink *iface, MFTIME system_time, float rate)
{
    struct system_time_source *source = impl_from_IMFClockStateSink(iface);
    double intpart;

    TRACE("%p, %s, %f.\n", iface, debugstr_time(system_time), rate);

    if (rate == 0.0f)
        return MF_E_UNSUPPORTED_RATE;

    modf(rate, &intpart);

    EnterCriticalSection(&source->cs);
    source->rate = rate;
    source->i_rate = rate == intpart ? rate : 0;
    LeaveCriticalSection(&source->cs);

    return S_OK;
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
    HRESULT hr;

    TRACE("%p.\n", time_source);

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFPresentationTimeSource_iface.lpVtbl = &systemtimesourcevtbl;
    object->IMFClockStateSink_iface.lpVtbl = &systemtimesourcesinkvtbl;
    object->refcount = 1;
    object->rate = 1.0f;
    object->i_rate = 1;
    InitializeCriticalSection(&object->cs);

    if (FAILED(hr = create_system_clock(&object->clock)))
    {
        IMFPresentationTimeSource_Release(&object->IMFPresentationTimeSource_iface);
        return hr;
    }

    *time_source = &object->IMFPresentationTimeSource_iface;

    return S_OK;
}

struct async_create_file
{
    IRtwqAsyncCallback IRtwqAsyncCallback_iface;
    LONG refcount;
    MF_FILE_ACCESSMODE access_mode;
    MF_FILE_OPENMODE open_mode;
    MF_FILE_FLAGS flags;
    WCHAR *path;
};

struct async_create_file_result
{
    struct list entry;
    IRtwqAsyncResult *result;
    IMFByteStream *stream;
};

static struct list async_create_file_results = LIST_INIT(async_create_file_results);
static CRITICAL_SECTION async_create_file_cs = { NULL, -1, 0, 0, 0, 0 };

static struct async_create_file *impl_from_create_file_IRtwqAsyncCallback(IRtwqAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct async_create_file, IRtwqAsyncCallback_iface);
}

static HRESULT WINAPI async_create_file_callback_QueryInterface(IRtwqAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IRtwqAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IRtwqAsyncCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI async_create_file_callback_AddRef(IRtwqAsyncCallback *iface)
{
    struct async_create_file *async = impl_from_create_file_IRtwqAsyncCallback(iface);
    ULONG refcount = InterlockedIncrement(&async->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI async_create_file_callback_Release(IRtwqAsyncCallback *iface)
{
    struct async_create_file *async = impl_from_create_file_IRtwqAsyncCallback(iface);
    ULONG refcount = InterlockedDecrement(&async->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        heap_free(async->path);
        heap_free(async);
    }

    return refcount;
}

static HRESULT WINAPI async_create_file_callback_GetParameters(IRtwqAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI async_create_file_callback_Invoke(IRtwqAsyncCallback *iface, IRtwqAsyncResult *result)
{
    struct async_create_file *async = impl_from_create_file_IRtwqAsyncCallback(iface);
    IRtwqAsyncResult *caller;
    IMFByteStream *stream;
    HRESULT hr;

    caller = (IRtwqAsyncResult *)IRtwqAsyncResult_GetStateNoAddRef(result);

    hr = MFCreateFile(async->access_mode, async->open_mode, async->flags, async->path, &stream);
    if (SUCCEEDED(hr))
    {
        struct async_create_file_result *result_item;

        result_item = heap_alloc(sizeof(*result_item));
        if (result_item)
        {
            result_item->result = caller;
            IRtwqAsyncResult_AddRef(caller);
            result_item->stream = stream;
            IMFByteStream_AddRef(stream);

            EnterCriticalSection(&async_create_file_cs);
            list_add_tail(&async_create_file_results, &result_item->entry);
            LeaveCriticalSection(&async_create_file_cs);
        }

        IMFByteStream_Release(stream);
    }
    else
        IRtwqAsyncResult_SetStatus(caller, hr);

    RtwqInvokeCallback(caller);

    return S_OK;
}

static const IRtwqAsyncCallbackVtbl async_create_file_callback_vtbl =
{
    async_create_file_callback_QueryInterface,
    async_create_file_callback_AddRef,
    async_create_file_callback_Release,
    async_create_file_callback_GetParameters,
    async_create_file_callback_Invoke,
};

/***********************************************************************
 *      MFBeginCreateFile (mfplat.@)
 */
HRESULT WINAPI MFBeginCreateFile(MF_FILE_ACCESSMODE access_mode, MF_FILE_OPENMODE open_mode, MF_FILE_FLAGS flags,
        const WCHAR *path, IMFAsyncCallback *callback, IUnknown *state, IUnknown **cancel_cookie)
{
    struct async_create_file *async = NULL;
    IRtwqAsyncResult *caller, *item = NULL;
    HRESULT hr;

    TRACE("%#x, %#x, %#x, %s, %p, %p, %p.\n", access_mode, open_mode, flags, debugstr_w(path), callback, state,
            cancel_cookie);

    if (cancel_cookie)
        *cancel_cookie = NULL;

    if (FAILED(hr = RtwqCreateAsyncResult(NULL, (IRtwqAsyncCallback *)callback, state, &caller)))
        return hr;

    async = heap_alloc(sizeof(*async));
    if (!async)
    {
        hr = E_OUTOFMEMORY;
        goto failed;
    }

    async->IRtwqAsyncCallback_iface.lpVtbl = &async_create_file_callback_vtbl;
    async->refcount = 1;
    async->access_mode = access_mode;
    async->open_mode = open_mode;
    async->flags = flags;
    if (FAILED(hr = heap_strdupW(path, &async->path)))
        goto failed;

    hr = RtwqCreateAsyncResult(NULL, &async->IRtwqAsyncCallback_iface, (IUnknown *)caller, &item);
    if (FAILED(hr))
        goto failed;

    if (cancel_cookie)
    {
        *cancel_cookie = (IUnknown *)caller;
        IUnknown_AddRef(*cancel_cookie);
    }

    hr = RtwqInvokeCallback(item);

failed:
    if (async)
        IRtwqAsyncCallback_Release(&async->IRtwqAsyncCallback_iface);
    if (item)
        IRtwqAsyncResult_Release(item);
    if (caller)
        IRtwqAsyncResult_Release(caller);

    return hr;
}

static HRESULT async_create_file_pull_result(IUnknown *unk, IMFByteStream **stream)
{
    struct async_create_file_result *item;
    HRESULT hr = MF_E_UNEXPECTED;
    IRtwqAsyncResult *result;

    *stream = NULL;

    if (FAILED(IUnknown_QueryInterface(unk, &IID_IRtwqAsyncResult, (void **)&result)))
        return hr;

    EnterCriticalSection(&async_create_file_cs);

    LIST_FOR_EACH_ENTRY(item, &async_create_file_results, struct async_create_file_result, entry)
    {
        if (result == item->result)
        {
            *stream = item->stream;
            IRtwqAsyncResult_Release(item->result);
            list_remove(&item->entry);
            heap_free(item);
            break;
        }
    }

    LeaveCriticalSection(&async_create_file_cs);

    if (*stream)
        hr = IRtwqAsyncResult_GetStatus(result);

    IRtwqAsyncResult_Release(result);

    return hr;
}

/***********************************************************************
 *      MFEndCreateFile (mfplat.@)
 */
HRESULT WINAPI MFEndCreateFile(IMFAsyncResult *result, IMFByteStream **stream)
{
    TRACE("%p, %p.\n", result, stream);

    return async_create_file_pull_result((IUnknown *)result, stream);
}

/***********************************************************************
 *      MFCancelCreateFile (mfplat.@)
 */
HRESULT WINAPI MFCancelCreateFile(IUnknown *cancel_cookie)
{
    IMFByteStream *stream = NULL;
    HRESULT hr;

    TRACE("%p.\n", cancel_cookie);

    hr = async_create_file_pull_result(cancel_cookie, &stream);

    if (stream)
        IMFByteStream_Release(stream);

    return hr;
}

/***********************************************************************
 *      MFRegisterLocalSchemeHandler (mfplat.@)
 */
HRESULT WINAPI MFRegisterLocalSchemeHandler(const WCHAR *scheme, IMFActivate *activate)
{
    struct local_handler *handler;
    HRESULT hr;

    TRACE("%s, %p.\n", debugstr_w(scheme), activate);

    if (!scheme || !activate)
        return E_INVALIDARG;

    if (!(handler = heap_alloc(sizeof(*handler))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = heap_strdupW(scheme, &handler->u.scheme)))
    {
        heap_free(handler);
        return hr;
    }
    handler->activate = activate;
    IMFActivate_AddRef(handler->activate);

    EnterCriticalSection(&local_handlers_section);
    list_add_head(&local_scheme_handlers, &handler->entry);
    LeaveCriticalSection(&local_handlers_section);

    return S_OK;
}

/***********************************************************************
 *      MFRegisterLocalByteStreamHandler (mfplat.@)
 */
HRESULT WINAPI MFRegisterLocalByteStreamHandler(const WCHAR *extension, const WCHAR *mime, IMFActivate *activate)
{
    struct local_handler *handler;
    HRESULT hr;

    TRACE("%s, %s, %p.\n", debugstr_w(extension), debugstr_w(mime), activate);

    if ((!extension && !mime) || !activate)
        return E_INVALIDARG;

    if (!(handler = heap_alloc_zero(sizeof(*handler))))
        return E_OUTOFMEMORY;

    hr = heap_strdupW(extension, &handler->u.bytestream.extension);
    if (SUCCEEDED(hr))
        hr = heap_strdupW(mime, &handler->u.bytestream.mime);

    if (FAILED(hr))
        goto failed;

    EnterCriticalSection(&local_handlers_section);
    list_add_head(&local_bytestream_handlers, &handler->entry);
    LeaveCriticalSection(&local_handlers_section);

    return hr;

failed:
    heap_free(handler->u.bytestream.extension);
    heap_free(handler->u.bytestream.mime);
    heap_free(handler);

    return hr;
}

struct property_store
{
    IPropertyStore IPropertyStore_iface;
    LONG refcount;
    CRITICAL_SECTION cs;
    size_t count, capacity;
    struct
    {
        PROPERTYKEY key;
        PROPVARIANT value;
    } *values;
};

static struct property_store *impl_from_IPropertyStore(IPropertyStore *iface)
{
    return CONTAINING_RECORD(iface, struct property_store, IPropertyStore_iface);
}

static HRESULT WINAPI property_store_QueryInterface(IPropertyStore *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IPropertyStore) || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IPropertyStore_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI property_store_AddRef(IPropertyStore *iface)
{
    struct property_store *store = impl_from_IPropertyStore(iface);
    ULONG refcount = InterlockedIncrement(&store->refcount);

    TRACE("%p, refcount %d.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI property_store_Release(IPropertyStore *iface)
{
    struct property_store *store = impl_from_IPropertyStore(iface);
    ULONG refcount = InterlockedDecrement(&store->refcount);

    TRACE("%p, refcount %d.\n", iface, refcount);

    if (!refcount)
    {
        DeleteCriticalSection(&store->cs);
        heap_free(store->values);
        heap_free(store);
    }

    return refcount;
}

static HRESULT WINAPI property_store_GetCount(IPropertyStore *iface, DWORD *count)
{
    struct property_store *store = impl_from_IPropertyStore(iface);

    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_INVALIDARG;

    EnterCriticalSection(&store->cs);
    *count = store->count;
    LeaveCriticalSection(&store->cs);
    return S_OK;
}

static HRESULT WINAPI property_store_GetAt(IPropertyStore *iface, DWORD index, PROPERTYKEY *key)
{
    struct property_store *store = impl_from_IPropertyStore(iface);

    TRACE("%p, %u, %p.\n", iface, index, key);

    EnterCriticalSection(&store->cs);

    if (index >= store->count)
    {
        LeaveCriticalSection(&store->cs);
        return E_INVALIDARG;
    }

    *key = store->values[index].key;

    LeaveCriticalSection(&store->cs);
    return S_OK;
}

static HRESULT WINAPI property_store_GetValue(IPropertyStore *iface, REFPROPERTYKEY key, PROPVARIANT *value)
{
    struct property_store *store = impl_from_IPropertyStore(iface);
    unsigned int i;

    TRACE("%p, %p, %p.\n", iface, key, value);

    if (!value)
        return E_INVALIDARG;

    if (!key)
        return S_FALSE;

    EnterCriticalSection(&store->cs);

    for (i = 0; i < store->count; ++i)
    {
        if (!memcmp(key, &store->values[i].key, sizeof(PROPERTYKEY)))
        {
            PropVariantCopy(value, &store->values[i].value);
            LeaveCriticalSection(&store->cs);
            return S_OK;
        }
    }

    LeaveCriticalSection(&store->cs);
    return S_FALSE;
}

static HRESULT WINAPI property_store_SetValue(IPropertyStore *iface, REFPROPERTYKEY key, REFPROPVARIANT value)
{
    struct property_store *store = impl_from_IPropertyStore(iface);
    unsigned int i;

    TRACE("%p, %p, %p.\n", iface, key, value);

    EnterCriticalSection(&store->cs);

    for (i = 0; i < store->count; ++i)
    {
        if (!memcmp(key, &store->values[i].key, sizeof(PROPERTYKEY)))
        {
            PropVariantCopy(&store->values[i].value, value);
            LeaveCriticalSection(&store->cs);
            return S_OK;
        }
    }

    if (!mf_array_reserve((void **)&store->values, &store->capacity, store->count + 1, sizeof(*store->values)))
    {
        LeaveCriticalSection(&store->cs);
        return E_OUTOFMEMORY;
    }

    store->values[store->count].key = *key;
    PropVariantCopy(&store->values[store->count].value, value);
    ++store->count;

    LeaveCriticalSection(&store->cs);
    return S_OK;
}

static HRESULT WINAPI property_store_Commit(IPropertyStore *iface)
{
    TRACE("%p.\n", iface);

    return E_NOTIMPL;
}

static const IPropertyStoreVtbl property_store_vtbl =
{
    property_store_QueryInterface,
    property_store_AddRef,
    property_store_Release,
    property_store_GetCount,
    property_store_GetAt,
    property_store_GetValue,
    property_store_SetValue,
    property_store_Commit,
};

/***********************************************************************
 *      CreatePropertyStore (mfplat.@)
 */
HRESULT WINAPI CreatePropertyStore(IPropertyStore **store)
{
    struct property_store *object;

    TRACE("%p.\n", store);

    if (!store)
        return E_INVALIDARG;

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IPropertyStore_iface.lpVtbl = &property_store_vtbl;
    object->refcount = 1;
    InitializeCriticalSection(&object->cs);

    TRACE("Created store %p.\n", object);
    *store = &object->IPropertyStore_iface;

    return S_OK;
}

struct dxgi_device_manager
{
    IMFDXGIDeviceManager IMFDXGIDeviceManager_iface;
    LONG refcount;
    UINT token;
    IDXGIDevice *device;
};

static struct dxgi_device_manager *impl_from_IMFDXGIDeviceManager(IMFDXGIDeviceManager *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_device_manager, IMFDXGIDeviceManager_iface);
}

static HRESULT WINAPI dxgi_device_manager_QueryInterface(IMFDXGIDeviceManager *iface, REFIID riid, void **obj)
{
    TRACE("(%p, %s, %p).\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFDXGIDeviceManager) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFDXGIDeviceManager_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dxgi_device_manager_AddRef(IMFDXGIDeviceManager *iface)
{
    struct dxgi_device_manager *manager = impl_from_IMFDXGIDeviceManager(iface);
    ULONG refcount = InterlockedIncrement(&manager->refcount);

    TRACE("(%p) ref=%u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI dxgi_device_manager_Release(IMFDXGIDeviceManager *iface)
{
    struct dxgi_device_manager *manager = impl_from_IMFDXGIDeviceManager(iface);
    ULONG refcount = InterlockedDecrement(&manager->refcount);

    TRACE("(%p) ref=%u.\n", iface, refcount);

    if (!refcount)
    {
        if (manager->device)
            IDXGIDevice_Release(manager->device);
        heap_free(manager);
    }

    return refcount;
}

static HRESULT WINAPI dxgi_device_manager_CloseDeviceHandle(IMFDXGIDeviceManager *iface, HANDLE device)
{
    FIXME("(%p, %p): stub.\n", iface, device);

    return E_NOTIMPL;
}

static HRESULT WINAPI dxgi_device_manager_GetVideoService(IMFDXGIDeviceManager *iface, HANDLE device,
                                                          REFIID riid, void **service)
{
    FIXME("(%p, %p, %s, %p): stub.\n", iface, device, debugstr_guid(riid), service);

    return E_NOTIMPL;
}

static HRESULT WINAPI dxgi_device_manager_LockDevice(IMFDXGIDeviceManager *iface, HANDLE device,
                                                     REFIID riid, void **ppv, BOOL block)
{
    FIXME("(%p, %p, %s, %p, %d): stub.\n", iface, device, wine_dbgstr_guid(riid), ppv, block);

    return E_NOTIMPL;
}

static HRESULT WINAPI dxgi_device_manager_OpenDeviceHandle(IMFDXGIDeviceManager *iface, HANDLE *device)
{
    FIXME("(%p, %p): stub.\n", iface, device);

    return E_NOTIMPL;
}

static HRESULT WINAPI dxgi_device_manager_ResetDevice(IMFDXGIDeviceManager *iface, IUnknown *device, UINT token)
{
    struct dxgi_device_manager *manager = impl_from_IMFDXGIDeviceManager(iface);
    IDXGIDevice *dxgi_device;
    HRESULT hr;

    TRACE("(%p, %p, %u).\n", iface, device, token);

    if (!device || token != manager->token)
        return E_INVALIDARG;

    hr = IUnknown_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
    if (SUCCEEDED(hr))
    {
        if (manager->device)
            IDXGIDevice_Release(manager->device);
        manager->device = dxgi_device;
    }
    else
        hr = E_INVALIDARG;

    return hr;
}

static HRESULT WINAPI dxgi_device_manager_TestDevice(IMFDXGIDeviceManager *iface, HANDLE device)
{
    FIXME("(%p, %p): stub.\n", iface, device);

    return E_NOTIMPL;
}

static HRESULT WINAPI dxgi_device_manager_UnlockDevice(IMFDXGIDeviceManager *iface, HANDLE device, BOOL state)
{
    FIXME("(%p, %p, %d): stub.\n", iface, device, state);

    return E_NOTIMPL;
}

static const IMFDXGIDeviceManagerVtbl dxgi_device_manager_vtbl =
{
    dxgi_device_manager_QueryInterface,
    dxgi_device_manager_AddRef,
    dxgi_device_manager_Release,
    dxgi_device_manager_CloseDeviceHandle,
    dxgi_device_manager_GetVideoService,
    dxgi_device_manager_LockDevice,
    dxgi_device_manager_OpenDeviceHandle,
    dxgi_device_manager_ResetDevice,
    dxgi_device_manager_TestDevice,
    dxgi_device_manager_UnlockDevice,
};

HRESULT WINAPI MFCreateDXGIDeviceManager(UINT *token, IMFDXGIDeviceManager **manager)
{
    struct dxgi_device_manager *object;

    TRACE("(%p, %p).\n", token, manager);

    if (!token || !manager)
        return E_POINTER;

    object = heap_alloc(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFDXGIDeviceManager_iface.lpVtbl = &dxgi_device_manager_vtbl;
    object->refcount = 1;
    object->token = GetTickCount();
    object->device = NULL;

    TRACE("Created device manager: %p, token: %u.\n", object, object->token);

    *token = object->token;
    *manager = &object->IMFDXGIDeviceManager_iface;

    return S_OK;
}
