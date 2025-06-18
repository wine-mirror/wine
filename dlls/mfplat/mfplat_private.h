/*
 * Copyright 2019 Nikolay Sivov for CodeWeavers
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

#undef INITGUID
#include <guiddef.h>
#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"
#include "d3d9types.h"

#include "wine/debug.h"

struct attribute
{
    GUID key;
    PROPVARIANT value;
};

struct attributes
{
    IMFAttributes IMFAttributes_iface;
    LONG ref;
    CRITICAL_SECTION cs;
    struct attribute *attributes;
    size_t capacity;
    size_t count;
};

extern HRESULT init_attributes_object(struct attributes *object, UINT32 size);
extern void clear_attributes_object(struct attributes *object);
extern const char *debugstr_attr(const GUID *guid);
extern const char *debugstr_mf_guid(const GUID *guid);

extern HRESULT attributes_GetItem(struct attributes *object, REFGUID key, PROPVARIANT *value);
extern HRESULT attributes_GetItemType(struct attributes *object, REFGUID key, MF_ATTRIBUTE_TYPE *type);
extern HRESULT attributes_CompareItem(struct attributes *object, REFGUID key, REFPROPVARIANT value,
        BOOL *result);
extern HRESULT attributes_Compare(struct attributes *object, IMFAttributes *theirs,
        MF_ATTRIBUTES_MATCH_TYPE match_type, BOOL *ret);
extern HRESULT attributes_GetUINT32(struct attributes *object, REFGUID key, UINT32 *value);
extern HRESULT attributes_GetUINT64(struct attributes *object, REFGUID key, UINT64 *value);
extern HRESULT attributes_GetDouble(struct attributes *object, REFGUID key, double *value);
extern HRESULT attributes_GetGUID(struct attributes *object, REFGUID key, GUID *value);
extern HRESULT attributes_GetStringLength(struct attributes *object, REFGUID key, UINT32 *length);
extern HRESULT attributes_GetString(struct attributes *object, REFGUID key, WCHAR *value, UINT32 size,
        UINT32 *length);
extern HRESULT attributes_GetAllocatedString(struct attributes *object, REFGUID key, WCHAR **value,
        UINT32 *length);
extern HRESULT attributes_GetBlobSize(struct attributes *object, REFGUID key, UINT32 *size);
extern HRESULT attributes_GetBlob(struct attributes *object, REFGUID key, UINT8 *buf, UINT32 bufsize,
        UINT32 *blobsize);
extern HRESULT attributes_GetAllocatedBlob(struct attributes *object, REFGUID key, UINT8 **buf,
        UINT32 *size);
extern HRESULT attributes_GetUnknown(struct attributes *object, REFGUID key, REFIID riid, void **out);
extern HRESULT attributes_SetItem(struct attributes *object, REFGUID key, REFPROPVARIANT value);
extern HRESULT attributes_DeleteItem(struct attributes *object, REFGUID key);
extern HRESULT attributes_DeleteAllItems(struct attributes *object);
extern HRESULT attributes_SetUINT32(struct attributes *object, REFGUID key, UINT32 value);
extern HRESULT attributes_SetUINT64(struct attributes *object, REFGUID key, UINT64 value);
extern HRESULT attributes_SetDouble(struct attributes *object, REFGUID key, double value);
extern HRESULT attributes_SetGUID(struct attributes *object, REFGUID key, REFGUID value);
extern HRESULT attributes_SetString(struct attributes *object, REFGUID key, const WCHAR *value);
extern HRESULT attributes_SetBlob(struct attributes *object, REFGUID key, const UINT8 *buf,
        UINT32 size);
extern HRESULT attributes_SetUnknown(struct attributes *object, REFGUID key, IUnknown *unknown);
extern HRESULT attributes_LockStore(struct attributes *object);
extern HRESULT attributes_UnlockStore(struct attributes *object);
extern HRESULT attributes_GetCount(struct attributes *object, UINT32 *items);
extern HRESULT attributes_GetItemByIndex(struct attributes *object, UINT32 index, GUID *key,
        PROPVARIANT *value);
extern HRESULT attributes_CopyAllItems(struct attributes *object, IMFAttributes *dest);

static inline BOOL mf_array_reserve(void **elements, size_t *capacity, size_t count, size_t size)
{
    size_t new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return TRUE;

    max_capacity = ~(SIZE_T)0 / size;
    if (count > max_capacity)
        return FALSE;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = realloc(*elements, new_capacity * size)))
        return FALSE;

    *elements = new_elements;
    *capacity = new_capacity;

    return TRUE;
}

extern unsigned int mf_format_get_stride(const GUID *subtype, unsigned int width, BOOL *is_yuv);

static inline const char *debugstr_propvar(const PROPVARIANT *v)
{
    if (!v)
        return "(null)";

    switch (v->vt)
    {
        case VT_EMPTY:
            return wine_dbg_sprintf("%p {VT_EMPTY}", v);
        case VT_NULL:
            return wine_dbg_sprintf("%p {VT_NULL}", v);
        case VT_UI4:
            return wine_dbg_sprintf("%p {VT_UI4: %ld}", v, v->ulVal);
        case VT_UI8:
            return wine_dbg_sprintf("%p {VT_UI8: %s}", v, wine_dbgstr_longlong(v->uhVal.QuadPart));
        case VT_I8:
            return wine_dbg_sprintf("%p {VT_I8: %s}", v, wine_dbgstr_longlong(v->hVal.QuadPart));
        case VT_R4:
            return wine_dbg_sprintf("%p {VT_R4: %.8e}", v, v->fltVal);
        case VT_R8:
            return wine_dbg_sprintf("%p {VT_R8: %lf}", v, v->dblVal);
        case VT_CLSID:
            return wine_dbg_sprintf("%p {VT_CLSID: %s}", v, debugstr_mf_guid(v->puuid));
        case VT_LPWSTR:
            return wine_dbg_sprintf("%p {VT_LPWSTR: %s}", v, wine_dbgstr_w(v->pwszVal));
        case VT_VECTOR | VT_UI1:
            return wine_dbg_sprintf("%p {VT_VECTOR|VT_UI1: %p}", v, v->caub.pElems);
        case VT_UNKNOWN:
            return wine_dbg_sprintf("%p {VT_UNKNOWN: %p}", v, v->punkVal);
        default:
            return wine_dbg_sprintf("%p {vt %#x}", v, v->vt);
    }
}

static inline const char *mf_debugstr_fourcc(DWORD format)
{
    static const struct format_name
    {
        unsigned int format;
        const char *name;
    } formats[] =
    {
        { D3DFMT_R8G8B8,        "R8G8B8" },
        { D3DFMT_A8R8G8B8,      "A8R8G8B8" },
        { D3DFMT_X8R8G8B8,      "X8R8G8B8" },
        { D3DFMT_R5G6B5,        "R5G6B5" },
        { D3DFMT_X1R5G5B5,      "X1R5G6B5" },
        { D3DFMT_A2B10G10R10,   "A2B10G10R10" },
        { D3DFMT_P8,            "P8" },
        { D3DFMT_L8,            "L8" },
        { D3DFMT_D16,           "D16" },
        { D3DFMT_L16,           "L16" },
        { D3DFMT_A16B16G16R16F, "A16B16G16R16F" },
    };
    int i;

    if ((format & 0xff) == format)
    {
        for (i = 0; i < ARRAY_SIZE(formats); ++i)
        {
            if (formats[i].format == format)
                return wine_dbg_sprintf("%s", wine_dbgstr_an(formats[i].name, -1));
        }

        return wine_dbg_sprintf("%#lx", format);
    }

    return wine_dbgstr_an((char *)&format, 4);
}

static inline const char *debugstr_time(LONGLONG time)
{
    ULONGLONG abstime = time >= 0 ? time : -time;
    unsigned int i = 0, j = 0;
    char buffer[23], rev[23];

    while (abstime || i <= 8)
    {
        buffer[i++] = '0' + (abstime % 10);
        abstime /= 10;
        if (i == 7) buffer[i++] = '.';
    }
    if (time < 0) buffer[i++] = '-';

    while (i--) rev[j++] = buffer[i];
    while (rev[j-1] == '0' && rev[j-2] != '.') --j;
    rev[j] = 0;

    return wine_dbg_sprintf("%s", rev);
}
