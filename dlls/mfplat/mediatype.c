/*
 * Copyright 2017 Alistair Leslie-Hughes
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

#define COBJMACROS

#include "mfplat_private.h"
#include <math.h>

#include "dxva.h"
#include "dxva2api.h"
#include "uuids.h"
#include "strmif.h"
#include "initguid.h"
#include "dvdmedia.h"
#include "ks.h"
#include "ksmedia.h"
#include "amvideo.h"
#include "wmcodecdsp.h"
#include "wmsdkidl.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

DEFINE_MEDIATYPE_GUID(MFVideoFormat_RGB1, D3DFMT_A1);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_RGB4, MAKEFOURCC('4','P','x','x'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_ABGR32, D3DFMT_A8B8G8R8);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_ARGB1555, D3DFMT_A1R5G5B5);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_ARGB4444, D3DFMT_A4R4G4B4);
/* SDK MFVideoFormat_A2R10G10B10 uses D3DFMT_A2B10G10R10, let's name it the other way */
DEFINE_MEDIATYPE_GUID(MFVideoFormat_A2B10G10R10, D3DFMT_A2R10G10B10);

DEFINE_MEDIATYPE_GUID(MFVideoFormat_IMC1, MAKEFOURCC('I','M','C','1'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_IMC2, MAKEFOURCC('I','M','C','2'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_IMC3, MAKEFOURCC('I','M','C','3'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_IMC4, MAKEFOURCC('I','M','C','4'));

static const UINT32 default_channel_mask[7] =
{
    0,
    SPEAKER_FRONT_LEFT,
    SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT,
    SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER,
    KSAUDIO_SPEAKER_QUAD,
    KSAUDIO_SPEAKER_QUAD | SPEAKER_FRONT_CENTER,
    KSAUDIO_SPEAKER_5POINT1,
};

static GUID get_am_subtype_for_mf_subtype(GUID subtype)
{
    if (IsEqualGUID(&subtype, &MFVideoFormat_RGB1))
        return MEDIASUBTYPE_RGB1;
    if (IsEqualGUID(&subtype, &MFVideoFormat_RGB4))
        return MEDIASUBTYPE_RGB4;
    if (IsEqualGUID(&subtype, &MFVideoFormat_RGB8))
        return MEDIASUBTYPE_RGB8;
    if (IsEqualGUID(&subtype, &MFVideoFormat_RGB555))
        return MEDIASUBTYPE_RGB555;
    if (IsEqualGUID(&subtype, &MFVideoFormat_RGB565))
        return MEDIASUBTYPE_RGB565;
    if (IsEqualGUID(&subtype, &MFVideoFormat_RGB24))
        return MEDIASUBTYPE_RGB24;
    if (IsEqualGUID(&subtype, &MFVideoFormat_RGB32))
        return MEDIASUBTYPE_RGB32;
    if (IsEqualGUID(&subtype, &MFVideoFormat_ARGB1555))
        return MEDIASUBTYPE_ARGB1555;
    if (IsEqualGUID(&subtype, &MFVideoFormat_ARGB4444))
        return MEDIASUBTYPE_ARGB4444;
    if (IsEqualGUID(&subtype, &MFVideoFormat_ARGB32))
        return MEDIASUBTYPE_ARGB32;
    if (IsEqualGUID(&subtype, &MFVideoFormat_A2B10G10R10))
        return MEDIASUBTYPE_A2R10G10B10;
    if (IsEqualGUID(&subtype, &MFVideoFormat_A2R10G10B10))
        return MEDIASUBTYPE_A2B10G10R10;
    return subtype;
}

struct media_type
{
    struct attributes attributes;
    IMFMediaType IMFMediaType_iface;
    IMFVideoMediaType IMFVideoMediaType_iface;
    IMFAudioMediaType IMFAudioMediaType_iface;
    MFVIDEOFORMAT *video_format;
    WAVEFORMATEX *audio_format;
};

struct stream_desc
{
    struct attributes attributes;
    IMFStreamDescriptor IMFStreamDescriptor_iface;
    IMFMediaTypeHandler IMFMediaTypeHandler_iface;
    DWORD identifier;
    IMFMediaType **media_types;
    unsigned int media_types_count;
    IMFMediaType *current_type;
};

struct presentation_desc_entry
{
    IMFStreamDescriptor *descriptor;
    BOOL selected;
};

struct presentation_desc
{
    struct attributes attributes;
    IMFPresentationDescriptor IMFPresentationDescriptor_iface;
    struct presentation_desc_entry *descriptors;
    unsigned int count;
};

static HRESULT presentation_descriptor_init(struct presentation_desc *object, DWORD count);

static struct media_type *impl_from_IMFMediaType(IMFMediaType *iface)
{
    return CONTAINING_RECORD(iface, struct media_type, IMFMediaType_iface);
}

static struct media_type *impl_from_IMFVideoMediaType(IMFVideoMediaType *iface)
{
    return CONTAINING_RECORD(iface, struct media_type, IMFVideoMediaType_iface);
}

static struct media_type *impl_from_IMFAudioMediaType(IMFAudioMediaType *iface)
{
    return CONTAINING_RECORD(iface, struct media_type, IMFAudioMediaType_iface);
}

static inline struct stream_desc *impl_from_IMFStreamDescriptor(IMFStreamDescriptor *iface)
{
    return CONTAINING_RECORD(iface, struct stream_desc, IMFStreamDescriptor_iface);
}

static struct stream_desc *impl_from_IMFMediaTypeHandler(IMFMediaTypeHandler *iface)
{
    return CONTAINING_RECORD(iface, struct stream_desc, IMFMediaTypeHandler_iface);
}

static struct presentation_desc *impl_from_IMFPresentationDescriptor(IMFPresentationDescriptor *iface)
{
    return CONTAINING_RECORD(iface, struct presentation_desc, IMFPresentationDescriptor_iface);
}

static HRESULT WINAPI mediatype_QueryInterface(IMFMediaType *iface, REFIID riid, void **out)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);
    GUID major = { 0 };

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    attributes_GetGUID(&media_type->attributes, &MF_MT_MAJOR_TYPE, &major);

    if (IsEqualGUID(&major, &MFMediaType_Video) && IsEqualIID(riid, &IID_IMFVideoMediaType))
    {
        *out = &media_type->IMFVideoMediaType_iface;
    }
    else if (IsEqualGUID(&major, &MFMediaType_Audio) && IsEqualIID(riid, &IID_IMFAudioMediaType))
    {
        *out = &media_type->IMFAudioMediaType_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaType) ||
            IsEqualIID(riid, &IID_IMFAttributes) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &media_type->IMFMediaType_iface;
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

static ULONG WINAPI mediatype_AddRef(IMFMediaType *iface)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);
    ULONG refcount = InterlockedIncrement(&media_type->attributes.ref);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI mediatype_Release(IMFMediaType *iface)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);
    ULONG refcount = InterlockedDecrement(&media_type->attributes.ref);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        clear_attributes_object(&media_type->attributes);
        CoTaskMemFree(media_type->video_format);
        CoTaskMemFree(media_type->audio_format);
        free(media_type);
    }

    return refcount;
}

static HRESULT WINAPI mediatype_GetItem(IMFMediaType *iface, REFGUID key, PROPVARIANT *value)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetItem(&media_type->attributes, key, value);
}

static HRESULT WINAPI mediatype_GetItemType(IMFMediaType *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), type);

    return attributes_GetItemType(&media_type->attributes, key, type);
}

static HRESULT WINAPI mediatype_CompareItem(IMFMediaType *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_propvar(value), result);

    return attributes_CompareItem(&media_type->attributes, key, value, result);
}

static HRESULT WINAPI mediatype_Compare(IMFMediaType *iface, IMFAttributes *attrs, MF_ATTRIBUTES_MATCH_TYPE type,
                BOOL *result)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %p, %d, %p.\n", iface, attrs, type, result);

    return attributes_Compare(&media_type->attributes, attrs, type, result);
}

static HRESULT WINAPI mediatype_GetUINT32(IMFMediaType *iface, REFGUID key, UINT32 *value)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT32(&media_type->attributes, key, value);
}

static HRESULT WINAPI mediatype_GetUINT64(IMFMediaType *iface, REFGUID key, UINT64 *value)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT64(&media_type->attributes, key, value);
}

static HRESULT WINAPI mediatype_GetDouble(IMFMediaType *iface, REFGUID key, double *value)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetDouble(&media_type->attributes, key, value);
}

static HRESULT WINAPI mediatype_GetGUID(IMFMediaType *iface, REFGUID key, GUID *value)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetGUID(&media_type->attributes, key, value);
}

static HRESULT WINAPI mediatype_GetStringLength(IMFMediaType *iface, REFGUID key, UINT32 *length)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), length);

    return attributes_GetStringLength(&media_type->attributes, key, length);
}

static HRESULT WINAPI mediatype_GetString(IMFMediaType *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_attr(key), value, size, length);

    return attributes_GetString(&media_type->attributes, key, value, size, length);
}

static HRESULT WINAPI mediatype_GetAllocatedString(IMFMediaType *iface, REFGUID key,
        WCHAR **value, UINT32 *length)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), value, length);

    return attributes_GetAllocatedString(&media_type->attributes, key, value, length);
}

static HRESULT WINAPI mediatype_GetBlobSize(IMFMediaType *iface, REFGUID key, UINT32 *size)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), size);

    return attributes_GetBlobSize(&media_type->attributes, key, size);
}

static HRESULT WINAPI mediatype_GetBlob(IMFMediaType *iface, REFGUID key, UINT8 *buf,
        UINT32 bufsize, UINT32 *blobsize)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_attr(key), buf, bufsize, blobsize);

    return attributes_GetBlob(&media_type->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI mediatype_GetAllocatedBlob(IMFMediaType *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), buf, size);

    return attributes_GetAllocatedBlob(&media_type->attributes, key, buf, size);
}

static HRESULT WINAPI mediatype_GetUnknown(IMFMediaType *iface, REFGUID key, REFIID riid, void **obj)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_guid(riid), obj);

    return attributes_GetUnknown(&media_type->attributes, key, riid, obj);
}

static HRESULT WINAPI mediatype_SetItem(IMFMediaType *iface, REFGUID key, REFPROPVARIANT value)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_propvar(value));

    return attributes_SetItem(&media_type->attributes, key, value);
}

static HRESULT WINAPI mediatype_DeleteItem(IMFMediaType *iface, REFGUID key)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s.\n", iface, debugstr_attr(key));

    return attributes_DeleteItem(&media_type->attributes, key);
}

static HRESULT WINAPI mediatype_DeleteAllItems(IMFMediaType *iface)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p.\n", iface);

    return attributes_DeleteAllItems(&media_type->attributes);
}

static HRESULT WINAPI mediatype_SetUINT32(IMFMediaType *iface, REFGUID key, UINT32 value)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %u.\n", iface, debugstr_attr(key), value);

    return attributes_SetUINT32(&media_type->attributes, key, value);
}

static HRESULT WINAPI mediatype_SetUINT64(IMFMediaType *iface, REFGUID key, UINT64 value)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), wine_dbgstr_longlong(value));

    return attributes_SetUINT64(&media_type->attributes, key, value);
}

static HRESULT WINAPI mediatype_SetDouble(IMFMediaType *iface, REFGUID key, double value)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_attr(key), value);

    return attributes_SetDouble(&media_type->attributes, key, value);
}

static HRESULT WINAPI mediatype_SetGUID(IMFMediaType *iface, REFGUID key, REFGUID value)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_mf_guid(value));

    return attributes_SetGUID(&media_type->attributes, key, value);
}

static HRESULT WINAPI mediatype_SetString(IMFMediaType *iface, REFGUID key, const WCHAR *value)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_w(value));

    return attributes_SetString(&media_type->attributes, key, value);
}

static HRESULT WINAPI mediatype_SetBlob(IMFMediaType *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %p, %u.\n", iface, debugstr_attr(key), buf, size);

    return attributes_SetBlob(&media_type->attributes, key, buf, size);
}

static HRESULT WINAPI mediatype_SetUnknown(IMFMediaType *iface, REFGUID key, IUnknown *unknown)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), unknown);

    return attributes_SetUnknown(&media_type->attributes, key, unknown);
}

static HRESULT WINAPI mediatype_LockStore(IMFMediaType *iface)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p.\n", iface);

    return attributes_LockStore(&media_type->attributes);
}

static HRESULT WINAPI mediatype_UnlockStore(IMFMediaType *iface)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p.\n", iface);

    return attributes_UnlockStore(&media_type->attributes);
}

static HRESULT WINAPI mediatype_GetCount(IMFMediaType *iface, UINT32 *count)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %p.\n", iface, count);

    return attributes_GetCount(&media_type->attributes, count);
}

static HRESULT WINAPI mediatype_GetItemByIndex(IMFMediaType *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return attributes_GetItemByIndex(&media_type->attributes, index, key, value);
}

static HRESULT WINAPI mediatype_CopyAllItems(IMFMediaType *iface, IMFAttributes *dest)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %p.\n", iface, dest);

    return attributes_CopyAllItems(&media_type->attributes, dest);
}

static HRESULT WINAPI mediatype_GetMajorType(IMFMediaType *iface, GUID *guid)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %p.\n", iface, guid);

    return attributes_GetGUID(&media_type->attributes, &MF_MT_MAJOR_TYPE, guid);
}

static HRESULT mediatype_is_compressed(struct media_type *media_type, BOOL *compressed)
{
    UINT32 value;

    if (FAILED(attributes_GetUINT32(&media_type->attributes, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value)))
    {
        value = 0;
    }

    *compressed = !value;

    return S_OK;
}

static HRESULT WINAPI mediatype_IsCompressedFormat(IMFMediaType *iface, BOOL *compressed)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %p.\n", iface, compressed);

    return mediatype_is_compressed(media_type, compressed);
}

static HRESULT media_type_is_equal(struct media_type *media_type, IMFMediaType *type, DWORD *flags)
{
    const DWORD full_equality_flags = MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES |
            MF_MEDIATYPE_EQUAL_FORMAT_DATA | MF_MEDIATYPE_EQUAL_FORMAT_USER_DATA;
    struct comparand
    {
        IMFAttributes *type;
        PROPVARIANT value;
        UINT32 count;
        GUID guid;
        HRESULT hr;
    } left, right, swp;
    unsigned int i;
    BOOL result;

    *flags = 0;

    left.type = &media_type->attributes.IMFAttributes_iface;
    right.type = (IMFAttributes *)type;

    if (FAILED(IMFAttributes_GetGUID(left.type, &MF_MT_MAJOR_TYPE, &left.guid)))
        return E_INVALIDARG;

    if (FAILED(IMFAttributes_GetGUID(right.type, &MF_MT_MAJOR_TYPE, &right.guid)))
        return E_INVALIDARG;

    if (IsEqualGUID(&left.guid, &right.guid))
        *flags |= MF_MEDIATYPE_EQUAL_MAJOR_TYPES;

    /* Subtypes equal or both missing. */
    left.hr = IMFAttributes_GetGUID(left.type, &MF_MT_SUBTYPE, &left.guid);
    right.hr = IMFAttributes_GetGUID(right.type, &MF_MT_SUBTYPE, &right.guid);

    if ((SUCCEEDED(left.hr) && SUCCEEDED(right.hr) && IsEqualGUID(&left.guid, &right.guid)) ||
           (FAILED(left.hr) && FAILED(right.hr)))
    {
        *flags |= MF_MEDIATYPE_EQUAL_FORMAT_TYPES;
    }

    /* Format data */
    IMFAttributes_GetCount(left.type, &left.count);
    IMFAttributes_GetCount(right.type, &right.count);

    if (right.count < left.count)
    {
        swp = left;
        left = right;
        right = swp;
    }

    *flags |= MF_MEDIATYPE_EQUAL_FORMAT_DATA;

    for (i = 0; i < left.count; ++i)
    {
        PROPVARIANT value;
        GUID key;

        if (SUCCEEDED(IMFAttributes_GetItemByIndex(left.type, i, &key, &value)))
        {
            if (IsEqualGUID(&key, &MF_MT_USER_DATA) ||
                    IsEqualGUID(&key, &MF_MT_FRAME_RATE_RANGE_MIN) ||
                    IsEqualGUID(&key, &MF_MT_FRAME_RATE_RANGE_MAX))
            {
                PropVariantClear(&value);
                continue;
            }

            result = FALSE;
            IMFAttributes_CompareItem(right.type, &key, &value, &result);
            PropVariantClear(&value);
            if (!result)
            {
                *flags &= ~MF_MEDIATYPE_EQUAL_FORMAT_DATA;
                break;
            }
        }
    }

    /* User data */
    PropVariantInit(&left.value);
    left.hr = IMFAttributes_GetItem(left.type, &MF_MT_USER_DATA, &left.value);
    PropVariantInit(&right.value);
    right.hr = IMFAttributes_GetItem(right.type, &MF_MT_USER_DATA, &right.value);

    /* Compare user data if both types have it, otherwise simply check if both don't. */
    if (SUCCEEDED(left.hr) && SUCCEEDED(right.hr))
    {
        result = FALSE;
        IMFAttributes_CompareItem(left.type, &MF_MT_USER_DATA, &left.value, &result);
    }
    else
        result = FAILED(left.hr) && FAILED(right.hr);

    PropVariantClear(&left.value);
    PropVariantClear(&right.value);

    if (result)
        *flags |= MF_MEDIATYPE_EQUAL_FORMAT_USER_DATA;

    return *flags == full_equality_flags ? S_OK : S_FALSE;
}

static HRESULT WINAPI mediatype_IsEqual(IMFMediaType *iface, IMFMediaType *type, DWORD *flags)
{
    struct media_type *media_type = impl_from_IMFMediaType(iface);

    TRACE("%p, %p, %p.\n", iface, type, flags);

    return media_type_is_equal(media_type, type, flags);
}

static HRESULT WINAPI mediatype_GetRepresentation(IMFMediaType *iface, GUID guid, void **representation)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(&guid), representation);

    if (IsEqualGUID(&guid, &AM_MEDIA_TYPE_REPRESENTATION))
        return MFCreateAMMediaTypeFromMFMediaType(iface, GUID_NULL, (AM_MEDIA_TYPE **)representation);

    if (IsEqualGUID(&guid, &FORMAT_WaveFormatEx)
            || IsEqualGUID(&guid, &FORMAT_VideoInfo)
            || IsEqualGUID(&guid, &FORMAT_VideoInfo2)
            || IsEqualGUID(&guid, &FORMAT_MFVideoFormat))
        return MFCreateAMMediaTypeFromMFMediaType(iface, guid, (AM_MEDIA_TYPE **)representation);

    FIXME("Format %s not implemented!\n", debugstr_guid(&guid));
    return MF_E_UNSUPPORTED_REPRESENTATION;
}

static HRESULT WINAPI mediatype_FreeRepresentation(IMFMediaType *iface, GUID guid, void *representation)
{
    AM_MEDIA_TYPE *am_type = representation;

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(&guid), representation);

    CoTaskMemFree(am_type->pbFormat);
    CoTaskMemFree(am_type);
    return S_OK;
}

static const IMFMediaTypeVtbl mediatypevtbl =
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

static HRESULT WINAPI video_mediatype_QueryInterface(IMFVideoMediaType *iface, REFIID riid, void **out)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);
    return IMFMediaType_QueryInterface(&media_type->IMFMediaType_iface, riid, out);
}

static ULONG WINAPI video_mediatype_AddRef(IMFVideoMediaType *iface)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);
    return IMFMediaType_AddRef(&media_type->IMFMediaType_iface);
}

static ULONG WINAPI video_mediatype_Release(IMFVideoMediaType *iface)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);
    return IMFMediaType_Release(&media_type->IMFMediaType_iface);
}

static HRESULT WINAPI video_mediatype_GetItem(IMFVideoMediaType *iface, REFGUID key, PROPVARIANT *value)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetItem(&media_type->attributes, key, value);
}

static HRESULT WINAPI video_mediatype_GetItemType(IMFVideoMediaType *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), type);

    return attributes_GetItemType(&media_type->attributes, key, type);
}

static HRESULT WINAPI video_mediatype_CompareItem(IMFVideoMediaType *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_propvar(value), result);

    return attributes_CompareItem(&media_type->attributes, key, value, result);
}

static HRESULT WINAPI video_mediatype_Compare(IMFVideoMediaType *iface, IMFAttributes *attrs,
        MF_ATTRIBUTES_MATCH_TYPE type, BOOL *result)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %p, %d, %p.\n", iface, attrs, type, result);

    return attributes_Compare(&media_type->attributes, attrs, type, result);
}

static HRESULT WINAPI video_mediatype_GetUINT32(IMFVideoMediaType *iface, REFGUID key, UINT32 *value)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT32(&media_type->attributes, key, value);
}

static HRESULT WINAPI video_mediatype_GetUINT64(IMFVideoMediaType *iface, REFGUID key, UINT64 *value)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT64(&media_type->attributes, key, value);
}

static HRESULT WINAPI video_mediatype_GetDouble(IMFVideoMediaType *iface, REFGUID key, double *value)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetDouble(&media_type->attributes, key, value);
}

static HRESULT WINAPI video_mediatype_GetGUID(IMFVideoMediaType *iface, REFGUID key, GUID *value)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetGUID(&media_type->attributes, key, value);
}

static HRESULT WINAPI video_mediatype_GetStringLength(IMFVideoMediaType *iface, REFGUID key, UINT32 *length)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), length);

    return attributes_GetStringLength(&media_type->attributes, key, length);
}

static HRESULT WINAPI video_mediatype_GetString(IMFVideoMediaType *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_attr(key), value, size, length);

    return attributes_GetString(&media_type->attributes, key, value, size, length);
}

static HRESULT WINAPI video_mediatype_GetAllocatedString(IMFVideoMediaType *iface, REFGUID key,
        WCHAR **value, UINT32 *length)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), value, length);

    return attributes_GetAllocatedString(&media_type->attributes, key, value, length);
}

static HRESULT WINAPI video_mediatype_GetBlobSize(IMFVideoMediaType *iface, REFGUID key, UINT32 *size)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), size);

    return attributes_GetBlobSize(&media_type->attributes, key, size);
}

static HRESULT WINAPI video_mediatype_GetBlob(IMFVideoMediaType *iface, REFGUID key, UINT8 *buf,
        UINT32 bufsize, UINT32 *blobsize)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_attr(key), buf, bufsize, blobsize);

    return attributes_GetBlob(&media_type->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI video_mediatype_GetAllocatedBlob(IMFVideoMediaType *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), buf, size);

    return attributes_GetAllocatedBlob(&media_type->attributes, key, buf, size);
}

static HRESULT WINAPI video_mediatype_GetUnknown(IMFVideoMediaType *iface, REFGUID key, REFIID riid, void **obj)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_guid(riid), obj);

    return attributes_GetUnknown(&media_type->attributes, key, riid, obj);
}

static HRESULT WINAPI video_mediatype_SetItem(IMFVideoMediaType *iface, REFGUID key, REFPROPVARIANT value)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_propvar(value));

    return attributes_SetItem(&media_type->attributes, key, value);
}

static HRESULT WINAPI video_mediatype_DeleteItem(IMFVideoMediaType *iface, REFGUID key)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s.\n", iface, debugstr_attr(key));

    return attributes_DeleteItem(&media_type->attributes, key);
}

static HRESULT WINAPI video_mediatype_DeleteAllItems(IMFVideoMediaType *iface)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p.\n", iface);

    return attributes_DeleteAllItems(&media_type->attributes);
}

static HRESULT WINAPI video_mediatype_SetUINT32(IMFVideoMediaType *iface, REFGUID key, UINT32 value)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %u.\n", iface, debugstr_attr(key), value);

    return attributes_SetUINT32(&media_type->attributes, key, value);
}

static HRESULT WINAPI video_mediatype_SetUINT64(IMFVideoMediaType *iface, REFGUID key, UINT64 value)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), wine_dbgstr_longlong(value));

    return attributes_SetUINT64(&media_type->attributes, key, value);
}

static HRESULT WINAPI video_mediatype_SetDouble(IMFVideoMediaType *iface, REFGUID key, double value)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_attr(key), value);

    return attributes_SetDouble(&media_type->attributes, key, value);
}

static HRESULT WINAPI video_mediatype_SetGUID(IMFVideoMediaType *iface, REFGUID key, REFGUID value)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_mf_guid(value));

    return attributes_SetGUID(&media_type->attributes, key, value);
}

static HRESULT WINAPI video_mediatype_SetString(IMFVideoMediaType *iface, REFGUID key, const WCHAR *value)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_w(value));

    return attributes_SetString(&media_type->attributes, key, value);
}

static HRESULT WINAPI video_mediatype_SetBlob(IMFVideoMediaType *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %p, %u.\n", iface, debugstr_attr(key), buf, size);

    return attributes_SetBlob(&media_type->attributes, key, buf, size);
}

static HRESULT WINAPI video_mediatype_SetUnknown(IMFVideoMediaType *iface, REFGUID key, IUnknown *unknown)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), unknown);

    return attributes_SetUnknown(&media_type->attributes, key, unknown);
}

static HRESULT WINAPI video_mediatype_LockStore(IMFVideoMediaType *iface)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p.\n", iface);

    return attributes_LockStore(&media_type->attributes);
}

static HRESULT WINAPI video_mediatype_UnlockStore(IMFVideoMediaType *iface)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p.\n", iface);

    return attributes_UnlockStore(&media_type->attributes);
}

static HRESULT WINAPI video_mediatype_GetCount(IMFVideoMediaType *iface, UINT32 *count)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %p.\n", iface, count);

    return attributes_GetCount(&media_type->attributes, count);
}

static HRESULT WINAPI video_mediatype_GetItemByIndex(IMFVideoMediaType *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return attributes_GetItemByIndex(&media_type->attributes, index, key, value);
}

static HRESULT WINAPI video_mediatype_CopyAllItems(IMFVideoMediaType *iface, IMFAttributes *dest)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %p.\n", iface, dest);

    return attributes_CopyAllItems(&media_type->attributes, dest);
}

static HRESULT WINAPI video_mediatype_GetMajorType(IMFVideoMediaType *iface, GUID *guid)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %p.\n", iface, guid);

    return attributes_GetGUID(&media_type->attributes, &MF_MT_MAJOR_TYPE, guid);
}

static HRESULT WINAPI video_mediatype_IsCompressedFormat(IMFVideoMediaType *iface, BOOL *compressed)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %p.\n", iface, compressed);

    return mediatype_is_compressed(media_type, compressed);
}

static HRESULT WINAPI video_mediatype_IsEqual(IMFVideoMediaType *iface, IMFMediaType *type, DWORD *flags)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);

    TRACE("%p, %p, %p.\n", iface, type, flags);

    return media_type_is_equal(media_type, type, flags);
}

static HRESULT WINAPI video_mediatype_GetRepresentation(IMFVideoMediaType *iface, GUID guid, void **representation)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(&guid), representation);

    if (IsEqualGUID(&guid, &FORMAT_WaveFormatEx))
        return MF_E_UNSUPPORTED_REPRESENTATION;

    return mediatype_GetRepresentation((IMFMediaType *)iface, guid, representation);
}

static HRESULT WINAPI video_mediatype_FreeRepresentation(IMFVideoMediaType *iface, GUID guid, void *representation)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(&guid), representation);
    return mediatype_FreeRepresentation((IMFMediaType *)iface, guid, representation);
}

static const MFVIDEOFORMAT * WINAPI video_mediatype_GetVideoFormat(IMFVideoMediaType *iface)
{
    struct media_type *media_type = impl_from_IMFVideoMediaType(iface);
    unsigned int size;
    HRESULT hr;

    TRACE("%p.\n", iface);

    CoTaskMemFree(media_type->video_format);
    media_type->video_format = NULL;
    if (FAILED(hr = MFCreateMFVideoFormatFromMFMediaType(&media_type->IMFMediaType_iface, &media_type->video_format, &size)))
        WARN("Failed to create format description, hr %#lx.\n", hr);

    return media_type->video_format;
}

static HRESULT WINAPI video_mediatype_GetVideoRepresentation(IMFVideoMediaType *iface, GUID representation,
        void **data, LONG stride)
{
    FIXME("%p, %s, %p, %ld.\n", iface, debugstr_guid(&representation), data, stride);

    return E_NOTIMPL;
}

static const IMFVideoMediaTypeVtbl videomediatypevtbl =
{
    video_mediatype_QueryInterface,
    video_mediatype_AddRef,
    video_mediatype_Release,
    video_mediatype_GetItem,
    video_mediatype_GetItemType,
    video_mediatype_CompareItem,
    video_mediatype_Compare,
    video_mediatype_GetUINT32,
    video_mediatype_GetUINT64,
    video_mediatype_GetDouble,
    video_mediatype_GetGUID,
    video_mediatype_GetStringLength,
    video_mediatype_GetString,
    video_mediatype_GetAllocatedString,
    video_mediatype_GetBlobSize,
    video_mediatype_GetBlob,
    video_mediatype_GetAllocatedBlob,
    video_mediatype_GetUnknown,
    video_mediatype_SetItem,
    video_mediatype_DeleteItem,
    video_mediatype_DeleteAllItems,
    video_mediatype_SetUINT32,
    video_mediatype_SetUINT64,
    video_mediatype_SetDouble,
    video_mediatype_SetGUID,
    video_mediatype_SetString,
    video_mediatype_SetBlob,
    video_mediatype_SetUnknown,
    video_mediatype_LockStore,
    video_mediatype_UnlockStore,
    video_mediatype_GetCount,
    video_mediatype_GetItemByIndex,
    video_mediatype_CopyAllItems,
    video_mediatype_GetMajorType,
    video_mediatype_IsCompressedFormat,
    video_mediatype_IsEqual,
    video_mediatype_GetRepresentation,
    video_mediatype_FreeRepresentation,
    video_mediatype_GetVideoFormat,
    video_mediatype_GetVideoRepresentation,
};

static HRESULT WINAPI audio_mediatype_QueryInterface(IMFAudioMediaType *iface, REFIID riid, void **out)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);
    return IMFMediaType_QueryInterface(&media_type->IMFMediaType_iface, riid, out);
}

static ULONG WINAPI audio_mediatype_AddRef(IMFAudioMediaType *iface)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);
    return IMFMediaType_AddRef(&media_type->IMFMediaType_iface);
}

static ULONG WINAPI audio_mediatype_Release(IMFAudioMediaType *iface)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);
    return IMFMediaType_Release(&media_type->IMFMediaType_iface);
}

static HRESULT WINAPI audio_mediatype_GetItem(IMFAudioMediaType *iface, REFGUID key, PROPVARIANT *value)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetItem(&media_type->attributes, key, value);
}

static HRESULT WINAPI audio_mediatype_GetItemType(IMFAudioMediaType *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), type);

    return attributes_GetItemType(&media_type->attributes, key, type);
}

static HRESULT WINAPI audio_mediatype_CompareItem(IMFAudioMediaType *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_propvar(value), result);

    return attributes_CompareItem(&media_type->attributes, key, value, result);
}

static HRESULT WINAPI audio_mediatype_Compare(IMFAudioMediaType *iface, IMFAttributes *attrs,
        MF_ATTRIBUTES_MATCH_TYPE type, BOOL *result)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %p, %d, %p.\n", iface, attrs, type, result);

    return attributes_Compare(&media_type->attributes, attrs, type, result);
}

static HRESULT WINAPI audio_mediatype_GetUINT32(IMFAudioMediaType *iface, REFGUID key, UINT32 *value)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT32(&media_type->attributes, key, value);
}

static HRESULT WINAPI audio_mediatype_GetUINT64(IMFAudioMediaType *iface, REFGUID key, UINT64 *value)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT64(&media_type->attributes, key, value);
}

static HRESULT WINAPI audio_mediatype_GetDouble(IMFAudioMediaType *iface, REFGUID key, double *value)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetDouble(&media_type->attributes, key, value);
}

static HRESULT WINAPI audio_mediatype_GetGUID(IMFAudioMediaType *iface, REFGUID key, GUID *value)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetGUID(&media_type->attributes, key, value);
}

static HRESULT WINAPI audio_mediatype_GetStringLength(IMFAudioMediaType *iface, REFGUID key, UINT32 *length)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), length);

    return attributes_GetStringLength(&media_type->attributes, key, length);
}

static HRESULT WINAPI audio_mediatype_GetString(IMFAudioMediaType *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_attr(key), value, size, length);

    return attributes_GetString(&media_type->attributes, key, value, size, length);
}

static HRESULT WINAPI audio_mediatype_GetAllocatedString(IMFAudioMediaType *iface, REFGUID key,
        WCHAR **value, UINT32 *length)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), value, length);

    return attributes_GetAllocatedString(&media_type->attributes, key, value, length);
}

static HRESULT WINAPI audio_mediatype_GetBlobSize(IMFAudioMediaType *iface, REFGUID key, UINT32 *size)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), size);

    return attributes_GetBlobSize(&media_type->attributes, key, size);
}

static HRESULT WINAPI audio_mediatype_GetBlob(IMFAudioMediaType *iface, REFGUID key, UINT8 *buf,
        UINT32 bufsize, UINT32 *blobsize)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_attr(key), buf, bufsize, blobsize);

    return attributes_GetBlob(&media_type->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI audio_mediatype_GetAllocatedBlob(IMFAudioMediaType *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), buf, size);

    return attributes_GetAllocatedBlob(&media_type->attributes, key, buf, size);
}

static HRESULT WINAPI audio_mediatype_GetUnknown(IMFAudioMediaType *iface, REFGUID key, REFIID riid, void **obj)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_guid(riid), obj);

    return attributes_GetUnknown(&media_type->attributes, key, riid, obj);
}

static HRESULT WINAPI audio_mediatype_SetItem(IMFAudioMediaType *iface, REFGUID key, REFPROPVARIANT value)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_propvar(value));

    return attributes_SetItem(&media_type->attributes, key, value);
}

static HRESULT WINAPI audio_mediatype_DeleteItem(IMFAudioMediaType *iface, REFGUID key)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s.\n", iface, debugstr_attr(key));

    return attributes_DeleteItem(&media_type->attributes, key);
}

static HRESULT WINAPI audio_mediatype_DeleteAllItems(IMFAudioMediaType *iface)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p.\n", iface);

    return attributes_DeleteAllItems(&media_type->attributes);
}

static HRESULT WINAPI audio_mediatype_SetUINT32(IMFAudioMediaType *iface, REFGUID key, UINT32 value)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %u.\n", iface, debugstr_attr(key), value);

    return attributes_SetUINT32(&media_type->attributes, key, value);
}

static HRESULT WINAPI audio_mediatype_SetUINT64(IMFAudioMediaType *iface, REFGUID key, UINT64 value)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), wine_dbgstr_longlong(value));

    return attributes_SetUINT64(&media_type->attributes, key, value);
}

static HRESULT WINAPI audio_mediatype_SetDouble(IMFAudioMediaType *iface, REFGUID key, double value)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_attr(key), value);

    return attributes_SetDouble(&media_type->attributes, key, value);
}

static HRESULT WINAPI audio_mediatype_SetGUID(IMFAudioMediaType *iface, REFGUID key, REFGUID value)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_mf_guid(value));

    return attributes_SetGUID(&media_type->attributes, key, value);
}

static HRESULT WINAPI audio_mediatype_SetString(IMFAudioMediaType *iface, REFGUID key, const WCHAR *value)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_w(value));

    return attributes_SetString(&media_type->attributes, key, value);
}

static HRESULT WINAPI audio_mediatype_SetBlob(IMFAudioMediaType *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %p, %u.\n", iface, debugstr_attr(key), buf, size);

    return attributes_SetBlob(&media_type->attributes, key, buf, size);
}

static HRESULT WINAPI audio_mediatype_SetUnknown(IMFAudioMediaType *iface, REFGUID key, IUnknown *unknown)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), unknown);

    return attributes_SetUnknown(&media_type->attributes, key, unknown);
}

static HRESULT WINAPI audio_mediatype_LockStore(IMFAudioMediaType *iface)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p.\n", iface);

    return attributes_LockStore(&media_type->attributes);
}

static HRESULT WINAPI audio_mediatype_UnlockStore(IMFAudioMediaType *iface)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p.\n", iface);

    return attributes_UnlockStore(&media_type->attributes);
}

static HRESULT WINAPI audio_mediatype_GetCount(IMFAudioMediaType *iface, UINT32 *count)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %p.\n", iface, count);

    return attributes_GetCount(&media_type->attributes, count);
}

static HRESULT WINAPI audio_mediatype_GetItemByIndex(IMFAudioMediaType *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return attributes_GetItemByIndex(&media_type->attributes, index, key, value);
}

static HRESULT WINAPI audio_mediatype_CopyAllItems(IMFAudioMediaType *iface, IMFAttributes *dest)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %p.\n", iface, dest);

    return attributes_CopyAllItems(&media_type->attributes, dest);
}

static HRESULT WINAPI audio_mediatype_GetMajorType(IMFAudioMediaType *iface, GUID *guid)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %p.\n", iface, guid);

    return attributes_GetGUID(&media_type->attributes, &MF_MT_MAJOR_TYPE, guid);
}

static HRESULT WINAPI audio_mediatype_IsCompressedFormat(IMFAudioMediaType *iface, BOOL *compressed)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %p.\n", iface, compressed);

    return mediatype_is_compressed(media_type, compressed);
}

static HRESULT WINAPI audio_mediatype_IsEqual(IMFAudioMediaType *iface, IMFMediaType *type, DWORD *flags)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);

    TRACE("%p, %p, %p.\n", iface, type, flags);

    return media_type_is_equal(media_type, type, flags);
}

static HRESULT WINAPI audio_mediatype_GetRepresentation(IMFAudioMediaType *iface, GUID guid, void **representation)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(&guid), representation);

    if (IsEqualGUID(&guid, &FORMAT_VideoInfo)
            || IsEqualGUID(&guid, &FORMAT_VideoInfo2)
            || IsEqualGUID(&guid, &FORMAT_MFVideoFormat))
        return E_INVALIDARG;

    return mediatype_GetRepresentation((IMFMediaType *)iface, guid, representation);
}

static HRESULT WINAPI audio_mediatype_FreeRepresentation(IMFAudioMediaType *iface, GUID guid, void *representation)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(&guid), representation);
    return mediatype_FreeRepresentation((IMFMediaType *)iface, guid, representation);
}

static const WAVEFORMATEX * WINAPI audio_mediatype_GetAudioFormat(IMFAudioMediaType *iface)
{
    struct media_type *media_type = impl_from_IMFAudioMediaType(iface);
    unsigned int size;
    HRESULT hr;

    TRACE("%p.\n", iface);

    CoTaskMemFree(media_type->audio_format);
    media_type->audio_format = NULL;
    if (FAILED(hr = MFCreateWaveFormatExFromMFMediaType(&media_type->IMFMediaType_iface, &media_type->audio_format,
            &size, MFWaveFormatExConvertFlag_Normal)))
    {
        WARN("Failed to create wave format description, hr %#lx.\n", hr);
    }

    return media_type->audio_format;
}

static const IMFAudioMediaTypeVtbl audiomediatypevtbl =
{
    audio_mediatype_QueryInterface,
    audio_mediatype_AddRef,
    audio_mediatype_Release,
    audio_mediatype_GetItem,
    audio_mediatype_GetItemType,
    audio_mediatype_CompareItem,
    audio_mediatype_Compare,
    audio_mediatype_GetUINT32,
    audio_mediatype_GetUINT64,
    audio_mediatype_GetDouble,
    audio_mediatype_GetGUID,
    audio_mediatype_GetStringLength,
    audio_mediatype_GetString,
    audio_mediatype_GetAllocatedString,
    audio_mediatype_GetBlobSize,
    audio_mediatype_GetBlob,
    audio_mediatype_GetAllocatedBlob,
    audio_mediatype_GetUnknown,
    audio_mediatype_SetItem,
    audio_mediatype_DeleteItem,
    audio_mediatype_DeleteAllItems,
    audio_mediatype_SetUINT32,
    audio_mediatype_SetUINT64,
    audio_mediatype_SetDouble,
    audio_mediatype_SetGUID,
    audio_mediatype_SetString,
    audio_mediatype_SetBlob,
    audio_mediatype_SetUnknown,
    audio_mediatype_LockStore,
    audio_mediatype_UnlockStore,
    audio_mediatype_GetCount,
    audio_mediatype_GetItemByIndex,
    audio_mediatype_CopyAllItems,
    audio_mediatype_GetMajorType,
    audio_mediatype_IsCompressedFormat,
    audio_mediatype_IsEqual,
    audio_mediatype_GetRepresentation,
    audio_mediatype_FreeRepresentation,
    audio_mediatype_GetAudioFormat,
};

static HRESULT create_media_type(struct media_type **ret)
{
    struct media_type *object;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = init_attributes_object(&object->attributes, 0)))
    {
        free(object);
        return hr;
    }
    object->IMFMediaType_iface.lpVtbl = &mediatypevtbl;
    object->IMFVideoMediaType_iface.lpVtbl = &videomediatypevtbl;
    object->IMFAudioMediaType_iface.lpVtbl = &audiomediatypevtbl;

    *ret = object;

    return S_OK;
}

/***********************************************************************
 *      MFCreateMediaType (mfplat.@)
 */
HRESULT WINAPI MFCreateMediaType(IMFMediaType **media_type)
{
    struct media_type *object;
    HRESULT hr;

    TRACE("%p.\n", media_type);

    if (!media_type)
        return E_INVALIDARG;

    if (FAILED(hr = create_media_type(&object)))
        return hr;

    *media_type = &object->IMFMediaType_iface;

    TRACE("Created media type %p.\n", *media_type);

    return S_OK;
}

static HRESULT WINAPI stream_descriptor_QueryInterface(IMFStreamDescriptor *iface, REFIID riid, void **out)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFStreamDescriptor) ||
            IsEqualIID(riid, &IID_IMFAttributes) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFStreamDescriptor_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI stream_descriptor_AddRef(IMFStreamDescriptor *iface)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);
    ULONG refcount = InterlockedIncrement(&stream_desc->attributes.ref);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI stream_descriptor_Release(IMFStreamDescriptor *iface)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);
    ULONG refcount = InterlockedDecrement(&stream_desc->attributes.ref);
    unsigned int i;

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        for (i = 0; i < stream_desc->media_types_count; ++i)
        {
            if (stream_desc->media_types[i])
                IMFMediaType_Release(stream_desc->media_types[i]);
        }
        free(stream_desc->media_types);
        if (stream_desc->current_type)
            IMFMediaType_Release(stream_desc->current_type);
        clear_attributes_object(&stream_desc->attributes);
        free(stream_desc);
    }

    return refcount;
}

static HRESULT WINAPI stream_descriptor_GetItem(IMFStreamDescriptor *iface, REFGUID key, PROPVARIANT *value)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetItem(&stream_desc->attributes, key, value);
}

static HRESULT WINAPI stream_descriptor_GetItemType(IMFStreamDescriptor *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), type);

    return attributes_GetItemType(&stream_desc->attributes, key, type);
}

static HRESULT WINAPI stream_descriptor_CompareItem(IMFStreamDescriptor *iface, REFGUID key, REFPROPVARIANT value,
        BOOL *result)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_propvar(value), result);

    return attributes_CompareItem(&stream_desc->attributes, key, value, result);
}

static HRESULT WINAPI stream_descriptor_Compare(IMFStreamDescriptor *iface, IMFAttributes *theirs,
        MF_ATTRIBUTES_MATCH_TYPE type, BOOL *result)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %p, %d, %p.\n", iface, theirs, type, result);

    return attributes_Compare(&stream_desc->attributes, theirs, type, result);
}

static HRESULT WINAPI stream_descriptor_GetUINT32(IMFStreamDescriptor *iface, REFGUID key, UINT32 *value)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT32(&stream_desc->attributes, key, value);
}

static HRESULT WINAPI stream_descriptor_GetUINT64(IMFStreamDescriptor *iface, REFGUID key, UINT64 *value)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT64(&stream_desc->attributes, key, value);
}

static HRESULT WINAPI stream_descriptor_GetDouble(IMFStreamDescriptor *iface, REFGUID key, double *value)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetDouble(&stream_desc->attributes, key, value);
}

static HRESULT WINAPI stream_descriptor_GetGUID(IMFStreamDescriptor *iface, REFGUID key, GUID *value)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetGUID(&stream_desc->attributes, key, value);
}

static HRESULT WINAPI stream_descriptor_GetStringLength(IMFStreamDescriptor *iface, REFGUID key, UINT32 *length)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), length);

    return attributes_GetStringLength(&stream_desc->attributes, key, length);
}

static HRESULT WINAPI stream_descriptor_GetString(IMFStreamDescriptor *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_attr(key), value, size, length);

    return attributes_GetString(&stream_desc->attributes, key, value, size, length);
}

static HRESULT WINAPI stream_descriptor_GetAllocatedString(IMFStreamDescriptor *iface, REFGUID key,
        WCHAR **value, UINT32 *length)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), value, length);

    return attributes_GetAllocatedString(&stream_desc->attributes, key, value, length);
}

static HRESULT WINAPI stream_descriptor_GetBlobSize(IMFStreamDescriptor *iface, REFGUID key, UINT32 *size)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), size);

    return attributes_GetBlobSize(&stream_desc->attributes, key, size);
}

static HRESULT WINAPI stream_descriptor_GetBlob(IMFStreamDescriptor *iface, REFGUID key, UINT8 *buf,
        UINT32 bufsize, UINT32 *blobsize)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_attr(key), buf, bufsize, blobsize);

    return attributes_GetBlob(&stream_desc->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI stream_descriptor_GetAllocatedBlob(IMFStreamDescriptor *iface, REFGUID key, UINT8 **buf,
        UINT32 *size)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), buf, size);

    return attributes_GetAllocatedBlob(&stream_desc->attributes, key, buf, size);
}

static HRESULT WINAPI stream_descriptor_GetUnknown(IMFStreamDescriptor *iface, REFGUID key, REFIID riid, void **out)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_guid(riid), out);

    return attributes_GetUnknown(&stream_desc->attributes, key, riid, out);
}

static HRESULT WINAPI stream_descriptor_SetItem(IMFStreamDescriptor *iface, REFGUID key, REFPROPVARIANT value)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_propvar(value));

    return attributes_SetItem(&stream_desc->attributes, key, value);
}

static HRESULT WINAPI stream_descriptor_DeleteItem(IMFStreamDescriptor *iface, REFGUID key)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s.\n", iface, debugstr_attr(key));

    return attributes_DeleteItem(&stream_desc->attributes, key);
}

static HRESULT WINAPI stream_descriptor_DeleteAllItems(IMFStreamDescriptor *iface)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p.\n", iface);

    return attributes_DeleteAllItems(&stream_desc->attributes);
}

static HRESULT WINAPI stream_descriptor_SetUINT32(IMFStreamDescriptor *iface, REFGUID key, UINT32 value)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %u.\n", iface, debugstr_attr(key), value);

    return attributes_SetUINT32(&stream_desc->attributes, key, value);
}

static HRESULT WINAPI stream_descriptor_SetUINT64(IMFStreamDescriptor *iface, REFGUID key, UINT64 value)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), wine_dbgstr_longlong(value));

    return attributes_SetUINT64(&stream_desc->attributes, key, value);
}

static HRESULT WINAPI stream_descriptor_SetDouble(IMFStreamDescriptor *iface, REFGUID key, double value)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_attr(key), value);

    return attributes_SetDouble(&stream_desc->attributes, key, value);
}

static HRESULT WINAPI stream_descriptor_SetGUID(IMFStreamDescriptor *iface, REFGUID key, REFGUID value)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_mf_guid(value));

    return attributes_SetGUID(&stream_desc->attributes, key, value);
}

static HRESULT WINAPI stream_descriptor_SetString(IMFStreamDescriptor *iface, REFGUID key, const WCHAR *value)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_w(value));

    return attributes_SetString(&stream_desc->attributes, key, value);
}

static HRESULT WINAPI stream_descriptor_SetBlob(IMFStreamDescriptor *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %p, %u.\n", iface, debugstr_attr(key), buf, size);

    return attributes_SetBlob(&stream_desc->attributes, key, buf, size);
}

static HRESULT WINAPI stream_descriptor_SetUnknown(IMFStreamDescriptor *iface, REFGUID key, IUnknown *unknown)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), unknown);

    return attributes_SetUnknown(&stream_desc->attributes, key, unknown);
}

static HRESULT WINAPI stream_descriptor_LockStore(IMFStreamDescriptor *iface)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p.\n", iface);

    return attributes_LockStore(&stream_desc->attributes);
}

static HRESULT WINAPI stream_descriptor_UnlockStore(IMFStreamDescriptor *iface)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p.\n", iface);

    return attributes_UnlockStore(&stream_desc->attributes);
}

static HRESULT WINAPI stream_descriptor_GetCount(IMFStreamDescriptor *iface, UINT32 *count)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %p.\n", iface, count);

    return attributes_GetCount(&stream_desc->attributes, count);
}

static HRESULT WINAPI stream_descriptor_GetItemByIndex(IMFStreamDescriptor *iface, UINT32 index, GUID *key,
        PROPVARIANT *value)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return attributes_GetItemByIndex(&stream_desc->attributes, index, key, value);
}

static HRESULT WINAPI stream_descriptor_CopyAllItems(IMFStreamDescriptor *iface, IMFAttributes *dest)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %p.\n", iface, dest);

    return attributes_CopyAllItems(&stream_desc->attributes, dest);
}

static HRESULT WINAPI stream_descriptor_GetStreamIdentifier(IMFStreamDescriptor *iface, DWORD *identifier)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %p.\n", iface, identifier);

    *identifier = stream_desc->identifier;

    return S_OK;
}

static HRESULT WINAPI stream_descriptor_GetMediaTypeHandler(IMFStreamDescriptor *iface, IMFMediaTypeHandler **handler)
{
    struct stream_desc *stream_desc = impl_from_IMFStreamDescriptor(iface);

    TRACE("%p, %p.\n", iface, handler);

    *handler = &stream_desc->IMFMediaTypeHandler_iface;
    IMFMediaTypeHandler_AddRef(*handler);

    return S_OK;
}

static const IMFStreamDescriptorVtbl streamdescriptorvtbl =
{
    stream_descriptor_QueryInterface,
    stream_descriptor_AddRef,
    stream_descriptor_Release,
    stream_descriptor_GetItem,
    stream_descriptor_GetItemType,
    stream_descriptor_CompareItem,
    stream_descriptor_Compare,
    stream_descriptor_GetUINT32,
    stream_descriptor_GetUINT64,
    stream_descriptor_GetDouble,
    stream_descriptor_GetGUID,
    stream_descriptor_GetStringLength,
    stream_descriptor_GetString,
    stream_descriptor_GetAllocatedString,
    stream_descriptor_GetBlobSize,
    stream_descriptor_GetBlob,
    stream_descriptor_GetAllocatedBlob,
    stream_descriptor_GetUnknown,
    stream_descriptor_SetItem,
    stream_descriptor_DeleteItem,
    stream_descriptor_DeleteAllItems,
    stream_descriptor_SetUINT32,
    stream_descriptor_SetUINT64,
    stream_descriptor_SetDouble,
    stream_descriptor_SetGUID,
    stream_descriptor_SetString,
    stream_descriptor_SetBlob,
    stream_descriptor_SetUnknown,
    stream_descriptor_LockStore,
    stream_descriptor_UnlockStore,
    stream_descriptor_GetCount,
    stream_descriptor_GetItemByIndex,
    stream_descriptor_CopyAllItems,
    stream_descriptor_GetStreamIdentifier,
    stream_descriptor_GetMediaTypeHandler
};

static HRESULT WINAPI mediatype_handler_QueryInterface(IMFMediaTypeHandler *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFMediaTypeHandler) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFMediaTypeHandler_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI mediatype_handler_AddRef(IMFMediaTypeHandler *iface)
{
    struct stream_desc *stream_desc = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamDescriptor_AddRef(&stream_desc->IMFStreamDescriptor_iface);
}

static ULONG WINAPI mediatype_handler_Release(IMFMediaTypeHandler *iface)
{
    struct stream_desc *stream_desc = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamDescriptor_Release(&stream_desc->IMFStreamDescriptor_iface);
}

static BOOL stream_descriptor_is_mediatype_supported(IMFMediaType *media_type, IMFMediaType *candidate)
{
    DWORD flags = 0;

    if (FAILED(IMFMediaType_IsEqual(media_type, candidate, &flags)))
        return FALSE;

    return (flags & (MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES)) ==
            (MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES);
}

static HRESULT WINAPI mediatype_handler_IsMediaTypeSupported(IMFMediaTypeHandler *iface, IMFMediaType *in_type,
        IMFMediaType **out_type)
{
    struct stream_desc *stream_desc = impl_from_IMFMediaTypeHandler(iface);
    BOOL supported = FALSE;
    unsigned int i;

    TRACE("%p, %p, %p.\n", iface, in_type, out_type);

    if (!in_type)
        return E_POINTER;

    if (out_type)
        *out_type = NULL;

    EnterCriticalSection(&stream_desc->attributes.cs);

    supported = stream_desc->current_type && stream_descriptor_is_mediatype_supported(stream_desc->current_type, in_type);
    if (!supported)
    {
        for (i = 0; i < stream_desc->media_types_count; ++i)
        {
            if ((supported = stream_descriptor_is_mediatype_supported(stream_desc->media_types[i], in_type)))
                break;
        }
    }

    LeaveCriticalSection(&stream_desc->attributes.cs);

    return supported ? S_OK : MF_E_INVALIDMEDIATYPE;
}

static HRESULT WINAPI mediatype_handler_GetMediaTypeCount(IMFMediaTypeHandler *iface, DWORD *count)
{
    struct stream_desc *stream_desc = impl_from_IMFMediaTypeHandler(iface);

    TRACE("%p, %p.\n", iface, count);

    *count = stream_desc->media_types_count;

    return S_OK;
}

static HRESULT WINAPI mediatype_handler_GetMediaTypeByIndex(IMFMediaTypeHandler *iface, DWORD index,
        IMFMediaType **type)
{
    struct stream_desc *stream_desc = impl_from_IMFMediaTypeHandler(iface);

    TRACE("%p, %lu, %p.\n", iface, index, type);

    if (index >= stream_desc->media_types_count)
        return MF_E_NO_MORE_TYPES;

    if (stream_desc->media_types[index])
    {
        *type = stream_desc->media_types[index];
        IMFMediaType_AddRef(*type);
    }

    return stream_desc->media_types[index] ? S_OK : E_FAIL;
}

static HRESULT WINAPI mediatype_handler_SetCurrentMediaType(IMFMediaTypeHandler *iface, IMFMediaType *type)
{
    struct stream_desc *stream_desc = impl_from_IMFMediaTypeHandler(iface);

    TRACE("%p, %p.\n", iface, type);

    if (!type)
        return E_POINTER;

    EnterCriticalSection(&stream_desc->attributes.cs);
    if (stream_desc->current_type)
        IMFMediaType_Release(stream_desc->current_type);
    stream_desc->current_type = type;
    IMFMediaType_AddRef(stream_desc->current_type);
    LeaveCriticalSection(&stream_desc->attributes.cs);

    return S_OK;
}

static HRESULT WINAPI mediatype_handler_GetCurrentMediaType(IMFMediaTypeHandler *iface, IMFMediaType **type)
{
    struct stream_desc *stream_desc = impl_from_IMFMediaTypeHandler(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, type);

    EnterCriticalSection(&stream_desc->attributes.cs);
    if (stream_desc->current_type)
    {
        *type = stream_desc->current_type;
        IMFMediaType_AddRef(*type);
    }
    else
        hr = MF_E_NOT_INITIALIZED;
    LeaveCriticalSection(&stream_desc->attributes.cs);

    return hr;
}

static HRESULT WINAPI mediatype_handler_GetMajorType(IMFMediaTypeHandler *iface, GUID *type)
{
    struct stream_desc *stream_desc = impl_from_IMFMediaTypeHandler(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, type);

    EnterCriticalSection(&stream_desc->attributes.cs);
    hr = IMFMediaType_GetGUID(stream_desc->current_type ? stream_desc->current_type :
            stream_desc->media_types[0], &MF_MT_MAJOR_TYPE, type);
    LeaveCriticalSection(&stream_desc->attributes.cs);

    return hr;
}

static const IMFMediaTypeHandlerVtbl mediatypehandlervtbl =
{
    mediatype_handler_QueryInterface,
    mediatype_handler_AddRef,
    mediatype_handler_Release,
    mediatype_handler_IsMediaTypeSupported,
    mediatype_handler_GetMediaTypeCount,
    mediatype_handler_GetMediaTypeByIndex,
    mediatype_handler_SetCurrentMediaType,
    mediatype_handler_GetCurrentMediaType,
    mediatype_handler_GetMajorType,
};

/***********************************************************************
 *      MFCreateStreamDescriptor (mfplat.@)
 */
HRESULT WINAPI MFCreateStreamDescriptor(DWORD identifier, DWORD count,
        IMFMediaType **types, IMFStreamDescriptor **descriptor)
{
    struct stream_desc *object;
    unsigned int i;
    HRESULT hr;

    TRACE("%ld, %ld, %p, %p.\n", identifier, count, types, descriptor);

    if (!count)
        return E_INVALIDARG;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = init_attributes_object(&object->attributes, 0)))
    {
        free(object);
        return hr;
    }
    object->IMFStreamDescriptor_iface.lpVtbl = &streamdescriptorvtbl;
    object->IMFMediaTypeHandler_iface.lpVtbl = &mediatypehandlervtbl;
    object->identifier = identifier;
    object->media_types = calloc(count, sizeof(*object->media_types));
    if (!object->media_types)
    {
        IMFStreamDescriptor_Release(&object->IMFStreamDescriptor_iface);
        return E_OUTOFMEMORY;
    }
    for (i = 0; i < count; ++i)
    {
        object->media_types[i] = types[i];
        if (object->media_types[i])
            IMFMediaType_AddRef(object->media_types[i]);
    }
    object->media_types_count = count;

    *descriptor = &object->IMFStreamDescriptor_iface;

    return S_OK;
}

static HRESULT WINAPI presentation_descriptor_QueryInterface(IMFPresentationDescriptor *iface, REFIID riid, void **out)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFPresentationDescriptor) ||
            IsEqualIID(riid, &IID_IMFAttributes) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFPresentationDescriptor_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI presentation_descriptor_AddRef(IMFPresentationDescriptor *iface)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);
    ULONG refcount = InterlockedIncrement(&presentation_desc->attributes.ref);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI presentation_descriptor_Release(IMFPresentationDescriptor *iface)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);
    ULONG refcount = InterlockedDecrement(&presentation_desc->attributes.ref);
    unsigned int i;

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        for (i = 0; i < presentation_desc->count; ++i)
        {
            if (presentation_desc->descriptors[i].descriptor)
                IMFStreamDescriptor_Release(presentation_desc->descriptors[i].descriptor);
        }
        clear_attributes_object(&presentation_desc->attributes);
        free(presentation_desc->descriptors);
        free(presentation_desc);
    }

    return refcount;
}

static HRESULT WINAPI presentation_descriptor_GetItem(IMFPresentationDescriptor *iface, REFGUID key,
        PROPVARIANT *value)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetItem(&presentation_desc->attributes, key, value);
}

static HRESULT WINAPI presentation_descriptor_GetItemType(IMFPresentationDescriptor *iface, REFGUID key,
        MF_ATTRIBUTE_TYPE *type)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), type);

    return attributes_GetItemType(&presentation_desc->attributes, key, type);
}

static HRESULT WINAPI presentation_descriptor_CompareItem(IMFPresentationDescriptor *iface, REFGUID key,
        REFPROPVARIANT value, BOOL *result)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_propvar(value), result);

    return attributes_CompareItem(&presentation_desc->attributes, key, value, result);
}

static HRESULT WINAPI presentation_descriptor_Compare(IMFPresentationDescriptor *iface, IMFAttributes *attrs,
        MF_ATTRIBUTES_MATCH_TYPE type, BOOL *result)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %p, %d, %p.\n", iface, attrs, type, result);

    return attributes_Compare(&presentation_desc->attributes, attrs, type, result);
}

static HRESULT WINAPI presentation_descriptor_GetUINT32(IMFPresentationDescriptor *iface, REFGUID key, UINT32 *value)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT32(&presentation_desc->attributes, key, value);
}

static HRESULT WINAPI presentation_descriptor_GetUINT64(IMFPresentationDescriptor *iface, REFGUID key, UINT64 *value)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT64(&presentation_desc->attributes, key, value);
}

static HRESULT WINAPI presentation_descriptor_GetDouble(IMFPresentationDescriptor *iface, REFGUID key, double *value)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetDouble(&presentation_desc->attributes, key, value);
}

static HRESULT WINAPI presentation_descriptor_GetGUID(IMFPresentationDescriptor *iface, REFGUID key, GUID *value)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetGUID(&presentation_desc->attributes, key, value);
}

static HRESULT WINAPI presentation_descriptor_GetStringLength(IMFPresentationDescriptor *iface, REFGUID key,
        UINT32 *length)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), length);

    return attributes_GetStringLength(&presentation_desc->attributes, key, length);
}

static HRESULT WINAPI presentation_descriptor_GetString(IMFPresentationDescriptor *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_attr(key), value, size, length);

    return attributes_GetString(&presentation_desc->attributes, key, value, size, length);
}

static HRESULT WINAPI presentation_descriptor_GetAllocatedString(IMFPresentationDescriptor *iface, REFGUID key,
        WCHAR **value, UINT32 *length)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), value, length);

    return attributes_GetAllocatedString(&presentation_desc->attributes, key, value, length);
}

static HRESULT WINAPI presentation_descriptor_GetBlobSize(IMFPresentationDescriptor *iface, REFGUID key, UINT32 *size)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), size);

    return attributes_GetBlobSize(&presentation_desc->attributes, key, size);
}

static HRESULT WINAPI presentation_descriptor_GetBlob(IMFPresentationDescriptor *iface, REFGUID key, UINT8 *buf,
        UINT32 bufsize, UINT32 *blobsize)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_attr(key), buf, bufsize, blobsize);

    return attributes_GetBlob(&presentation_desc->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI presentation_descriptor_GetAllocatedBlob(IMFPresentationDescriptor *iface, REFGUID key,
        UINT8 **buf, UINT32 *size)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), buf, size);

    return attributes_GetAllocatedBlob(&presentation_desc->attributes, key, buf, size);
}

static HRESULT WINAPI presentation_descriptor_GetUnknown(IMFPresentationDescriptor *iface, REFGUID key,
        REFIID riid, void **out)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_guid(riid), out);

    return attributes_GetUnknown(&presentation_desc->attributes, key, riid, out);
}

static HRESULT WINAPI presentation_descriptor_SetItem(IMFPresentationDescriptor *iface, REFGUID key,
        REFPROPVARIANT value)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_propvar(value));

    return attributes_SetItem(&presentation_desc->attributes, key, value);
}

static HRESULT WINAPI presentation_descriptor_DeleteItem(IMFPresentationDescriptor *iface, REFGUID key)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s.\n", iface, debugstr_attr(key));

    return attributes_DeleteItem(&presentation_desc->attributes, key);
}

static HRESULT WINAPI presentation_descriptor_DeleteAllItems(IMFPresentationDescriptor *iface)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p.\n", iface);

    return attributes_DeleteAllItems(&presentation_desc->attributes);
}

static HRESULT WINAPI presentation_descriptor_SetUINT32(IMFPresentationDescriptor *iface, REFGUID key, UINT32 value)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %u.\n", iface, debugstr_attr(key), value);

    return attributes_SetUINT32(&presentation_desc->attributes, key, value);
}

static HRESULT WINAPI presentation_descriptor_SetUINT64(IMFPresentationDescriptor *iface, REFGUID key, UINT64 value)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), wine_dbgstr_longlong(value));

    return attributes_SetUINT64(&presentation_desc->attributes, key, value);
}

static HRESULT WINAPI presentation_descriptor_SetDouble(IMFPresentationDescriptor *iface, REFGUID key, double value)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_attr(key), value);

    return attributes_SetDouble(&presentation_desc->attributes, key, value);
}

static HRESULT WINAPI presentation_descriptor_SetGUID(IMFPresentationDescriptor *iface, REFGUID key, REFGUID value)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_mf_guid(value));

    return attributes_SetGUID(&presentation_desc->attributes, key, value);
}

static HRESULT WINAPI presentation_descriptor_SetString(IMFPresentationDescriptor *iface, REFGUID key,
        const WCHAR *value)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_w(value));

    return attributes_SetString(&presentation_desc->attributes, key, value);
}

static HRESULT WINAPI presentation_descriptor_SetBlob(IMFPresentationDescriptor *iface, REFGUID key, const UINT8 *buf,
        UINT32 size)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %p, %u.\n", iface, debugstr_attr(key), buf, size);

    return attributes_SetBlob(&presentation_desc->attributes, key, buf, size);
}

static HRESULT WINAPI presentation_descriptor_SetUnknown(IMFPresentationDescriptor *iface, REFGUID key,
        IUnknown *unknown)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), unknown);

    return attributes_SetUnknown(&presentation_desc->attributes, key, unknown);
}

static HRESULT WINAPI presentation_descriptor_LockStore(IMFPresentationDescriptor *iface)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p.\n", iface);

    return attributes_LockStore(&presentation_desc->attributes);
}

static HRESULT WINAPI presentation_descriptor_UnlockStore(IMFPresentationDescriptor *iface)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p.\n", iface);

    return attributes_UnlockStore(&presentation_desc->attributes);
}

static HRESULT WINAPI presentation_descriptor_GetCount(IMFPresentationDescriptor *iface, UINT32 *count)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %p.\n", iface, count);

    return attributes_GetCount(&presentation_desc->attributes, count);
}

static HRESULT WINAPI presentation_descriptor_GetItemByIndex(IMFPresentationDescriptor *iface, UINT32 index, GUID *key,
        PROPVARIANT *value)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return attributes_GetItemByIndex(&presentation_desc->attributes, index, key, value);
}

static HRESULT WINAPI presentation_descriptor_CopyAllItems(IMFPresentationDescriptor *iface, IMFAttributes *dest)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %p.\n", iface, dest);

    return attributes_CopyAllItems(&presentation_desc->attributes, dest);
}

static HRESULT WINAPI presentation_descriptor_GetStreamDescriptorCount(IMFPresentationDescriptor *iface, DWORD *count)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %p.\n", iface, count);

    *count = presentation_desc->count;

    return S_OK;
}

static HRESULT WINAPI presentation_descriptor_GetStreamDescriptorByIndex(IMFPresentationDescriptor *iface, DWORD index,
        BOOL *selected, IMFStreamDescriptor **descriptor)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %lu, %p, %p.\n", iface, index, selected, descriptor);

    if (index >= presentation_desc->count)
        return E_INVALIDARG;

    EnterCriticalSection(&presentation_desc->attributes.cs);
    *selected = presentation_desc->descriptors[index].selected;
    LeaveCriticalSection(&presentation_desc->attributes.cs);

    *descriptor = presentation_desc->descriptors[index].descriptor;
    IMFStreamDescriptor_AddRef(*descriptor);

    return S_OK;
}

static HRESULT WINAPI presentation_descriptor_SelectStream(IMFPresentationDescriptor *iface, DWORD index)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %lu.\n", iface, index);

    if (index >= presentation_desc->count)
        return E_INVALIDARG;

    EnterCriticalSection(&presentation_desc->attributes.cs);
    presentation_desc->descriptors[index].selected = TRUE;
    LeaveCriticalSection(&presentation_desc->attributes.cs);

    return S_OK;
}

static HRESULT WINAPI presentation_descriptor_DeselectStream(IMFPresentationDescriptor *iface, DWORD index)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);

    TRACE("%p, %lu.\n", iface, index);

    if (index >= presentation_desc->count)
        return E_INVALIDARG;

    EnterCriticalSection(&presentation_desc->attributes.cs);
    presentation_desc->descriptors[index].selected = FALSE;
    LeaveCriticalSection(&presentation_desc->attributes.cs);

    return S_OK;
}

static HRESULT WINAPI presentation_descriptor_Clone(IMFPresentationDescriptor *iface,
        IMFPresentationDescriptor **descriptor)
{
    struct presentation_desc *presentation_desc = impl_from_IMFPresentationDescriptor(iface);
    struct presentation_desc *object;
    unsigned int i;

    TRACE("%p, %p.\n", iface, descriptor);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    presentation_descriptor_init(object, presentation_desc->count);

    EnterCriticalSection(&presentation_desc->attributes.cs);

    for (i = 0; i < presentation_desc->count; ++i)
    {
        object->descriptors[i] = presentation_desc->descriptors[i];
        IMFStreamDescriptor_AddRef(object->descriptors[i].descriptor);
    }

    attributes_CopyAllItems(&presentation_desc->attributes, (IMFAttributes *)&object->IMFPresentationDescriptor_iface);

    LeaveCriticalSection(&presentation_desc->attributes.cs);

    *descriptor = &object->IMFPresentationDescriptor_iface;

    return S_OK;
}

static const IMFPresentationDescriptorVtbl presentationdescriptorvtbl =
{
    presentation_descriptor_QueryInterface,
    presentation_descriptor_AddRef,
    presentation_descriptor_Release,
    presentation_descriptor_GetItem,
    presentation_descriptor_GetItemType,
    presentation_descriptor_CompareItem,
    presentation_descriptor_Compare,
    presentation_descriptor_GetUINT32,
    presentation_descriptor_GetUINT64,
    presentation_descriptor_GetDouble,
    presentation_descriptor_GetGUID,
    presentation_descriptor_GetStringLength,
    presentation_descriptor_GetString,
    presentation_descriptor_GetAllocatedString,
    presentation_descriptor_GetBlobSize,
    presentation_descriptor_GetBlob,
    presentation_descriptor_GetAllocatedBlob,
    presentation_descriptor_GetUnknown,
    presentation_descriptor_SetItem,
    presentation_descriptor_DeleteItem,
    presentation_descriptor_DeleteAllItems,
    presentation_descriptor_SetUINT32,
    presentation_descriptor_SetUINT64,
    presentation_descriptor_SetDouble,
    presentation_descriptor_SetGUID,
    presentation_descriptor_SetString,
    presentation_descriptor_SetBlob,
    presentation_descriptor_SetUnknown,
    presentation_descriptor_LockStore,
    presentation_descriptor_UnlockStore,
    presentation_descriptor_GetCount,
    presentation_descriptor_GetItemByIndex,
    presentation_descriptor_CopyAllItems,
    presentation_descriptor_GetStreamDescriptorCount,
    presentation_descriptor_GetStreamDescriptorByIndex,
    presentation_descriptor_SelectStream,
    presentation_descriptor_DeselectStream,
    presentation_descriptor_Clone,
};

static HRESULT presentation_descriptor_init(struct presentation_desc *object, DWORD count)
{
    HRESULT hr;

    if (FAILED(hr = init_attributes_object(&object->attributes, 0)))
        return hr;
    object->IMFPresentationDescriptor_iface.lpVtbl = &presentationdescriptorvtbl;
    if (!(object->descriptors = calloc(count, sizeof(*object->descriptors))))
    {
        IMFPresentationDescriptor_Release(&object->IMFPresentationDescriptor_iface);
        return E_OUTOFMEMORY;
    }
    object->count = count;

    return S_OK;
}

/***********************************************************************
 *      MFCreatePresentationDescriptor (mfplat.@)
 */
HRESULT WINAPI MFCreatePresentationDescriptor(DWORD count, IMFStreamDescriptor **descriptors,
        IMFPresentationDescriptor **out)
{
    struct presentation_desc *object;
    unsigned int i;
    HRESULT hr;

    TRACE("%lu, %p, %p.\n", count, descriptors, out);

    if (!count)
        return E_INVALIDARG;

    for (i = 0; i < count; ++i)
    {
        if (!descriptors[i])
            return E_INVALIDARG;
    }

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = presentation_descriptor_init(object, count)))
    {
        free(object);
        return hr;
    }

    for (i = 0; i < count; ++i)
    {
        object->descriptors[i].descriptor = descriptors[i];
        IMFStreamDescriptor_AddRef(object->descriptors[i].descriptor);
    }

    *out = &object->IMFPresentationDescriptor_iface;

    return S_OK;
}

struct uncompressed_video_format
{
    const GUID *subtype;
    unsigned char bpp;
    unsigned char alignment;
    unsigned char bottom_up;
    unsigned char yuv;
    int compression;
};

static int __cdecl uncompressed_video_format_compare(const void *a, const void *b)
{
    const struct uncompressed_video_format *a_format = a, *b_format = b;
    return memcmp(a_format->subtype, b_format->subtype, sizeof(GUID));
}

static struct uncompressed_video_format video_formats[] =
{
    { &MFVideoFormat_RGB1,          1, 0, 1, 0, BI_RGB },
    { &MFVideoFormat_RGB4,          4, 0, 1, 0, BI_RGB },
    { &MFVideoFormat_RGB24,         24, 3, 1, 0, BI_RGB },
    { &MFVideoFormat_ARGB32,        32, 3, 1, 0, BI_RGB },
    { &MFVideoFormat_RGB32,         32, 3, 1, 0, BI_RGB },
    { &MFVideoFormat_RGB565,        16, 3, 1, 0, BI_BITFIELDS },
    { &MFVideoFormat_RGB555,        16, 3, 1, 0, BI_RGB },
    { &MFVideoFormat_ABGR32,        32, 3, 1, 0, BI_RGB },
    { &MFVideoFormat_A2R10G10B10,   32, 3, 1, 0, -1 },
    { &MFVideoFormat_A2B10G10R10,   32, 3, 1, 0, -1 },
    { &MFVideoFormat_RGB8,          8, 3, 1, 0, BI_RGB },
    { &MFVideoFormat_L8,            8, 3, 1, 0, -1 },
    { &MFVideoFormat_AYUV,          32, 3, 0, 1, -1 },
    { &MFVideoFormat_I420,          12, 0, 0, 1, -1 },
    { &MFVideoFormat_IMC1,          16, 3, 0, 1, -1 },
    { &MFVideoFormat_IMC2,          12, 0, 0, 1, -1 },
    { &MFVideoFormat_IMC3,          16, 3, 0, 1, -1 },
    { &MFVideoFormat_IMC4,          12, 0, 0, 1, -1 },
    { &MFVideoFormat_IYUV,          12, 0, 0, 1, -1 },
    { &MFVideoFormat_NV11,          12, 0, 0, 1, -1 },
    { &MFVideoFormat_NV12,          12, 0, 0, 1, -1 },
    { &MFVideoFormat_D16,           16, 3, 0, 0, -1 },
    { &MFVideoFormat_L16,           16, 3, 0, 0, -1 },
    { &MFVideoFormat_UYVY,          16, 0, 0, 1, -1 },
    { &MFVideoFormat_YUY2,          16, 0, 0, 1, -1 },
    { &MFVideoFormat_YV12,          12, 0, 0, 1, -1 },
    { &MFVideoFormat_YVYU,          16, 0, 0, 1, -1 },
    { &MFVideoFormat_A16B16G16R16F, 64, 3, 1, 0, -1 },
    { &MEDIASUBTYPE_RGB8,           8, 3, 1, 0, BI_RGB },
    { &MEDIASUBTYPE_RGB565,         16, 3, 1, 0, BI_BITFIELDS },
    { &MEDIASUBTYPE_RGB555,         16, 3, 1, 0, BI_RGB },
    { &MEDIASUBTYPE_RGB24,          24, 3, 1, 0, BI_RGB },
    { &MEDIASUBTYPE_RGB32,          32, 3, 1, 0, BI_RGB },
};

static BOOL WINAPI mf_video_formats_init(INIT_ONCE *once, void *param, void **context)
{
    qsort(video_formats, ARRAY_SIZE(video_formats), sizeof(*video_formats), uncompressed_video_format_compare);
    return TRUE;
}

static struct uncompressed_video_format *mf_get_video_format(const GUID *subtype)
{
    static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;
    struct uncompressed_video_format key = {.subtype = subtype};

    InitOnceExecuteOnce(&init_once, mf_video_formats_init, NULL, NULL);

    return bsearch(&key, video_formats, ARRAY_SIZE(video_formats), sizeof(*video_formats),
            uncompressed_video_format_compare);
}

static unsigned int mf_get_stride_for_format(const struct uncompressed_video_format *format, unsigned int width)
{
    if (format->bpp < 8) return (width * format->bpp) / 8;
    return (width * (format->bpp / 8) + format->alignment) & ~format->alignment;
}

unsigned int mf_format_get_stride(const GUID *subtype, unsigned int width, BOOL *is_yuv)
{
    struct uncompressed_video_format *format = mf_get_video_format(subtype);

    if (format)
    {
        *is_yuv = format->yuv;
        return mf_get_stride_for_format(format, width);
    }

    return 0;
}

/***********************************************************************
 *      MFGetStrideForBitmapInfoHeader (mfplat.@)
 */
HRESULT WINAPI MFGetStrideForBitmapInfoHeader(DWORD fourcc, DWORD width, LONG *stride)
{
    struct uncompressed_video_format *format;
    GUID subtype;

    TRACE("%s, %lu, %p.\n", mf_debugstr_fourcc(fourcc), width, stride);

    memcpy(&subtype, &MFVideoFormat_Base, sizeof(subtype));
    subtype.Data1 = fourcc;

    if (!(format = mf_get_video_format(&subtype)))
    {
        *stride = 0;
        return MF_E_INVALIDMEDIATYPE;
    }

    *stride = mf_get_stride_for_format(format, width);
    if (format->bottom_up)
        *stride *= -1;

    return S_OK;
}

/***********************************************************************
 *      MFCalculateImageSize (mfplat.@)
 */
HRESULT WINAPI MFCalculateImageSize(REFGUID subtype, UINT32 width, UINT32 height, UINT32 *size)
{
    struct uncompressed_video_format *format;
    unsigned int stride;

    TRACE("%s, %u, %u, %p.\n", debugstr_mf_guid(subtype), width, height, size);

    if (!(format = mf_get_video_format(subtype)))
    {
        *size = 0;
        return E_INVALIDARG;
    }

    switch (subtype->Data1)
    {
        case MAKEFOURCC('I','M','C','2'):
        case MAKEFOURCC('I','M','C','4'):
        case MAKEFOURCC('N','V','1','2'):
        case MAKEFOURCC('Y','V','1','2'):
        case MAKEFOURCC('I','4','2','0'):
        case MAKEFOURCC('I','Y','U','V'):
            /* 2 x 2 block, interleaving UV for half the height */
            *size = ((width + 1) & ~1) * height * 3 / 2;
            break;
        case MAKEFOURCC('N','V','1','1'):
            *size = ((width + 3) & ~3) * height * 3 / 2;
            break;
        case D3DFMT_L8:
        case D3DFMT_L16:
        case D3DFMT_D16:
            *size = width * (format->bpp / 8) * height;
            break;
        default:
            stride = mf_get_stride_for_format(format, width);
            *size = stride * height;
    }

    return S_OK;
}

/***********************************************************************
 *      MFGetPlaneSize (mfplat.@)
 */
HRESULT WINAPI MFGetPlaneSize(DWORD fourcc, DWORD width, DWORD height, DWORD *size)
{
    struct uncompressed_video_format *format;
    unsigned int stride;
    GUID subtype;

    TRACE("%s, %lu, %lu, %p.\n", mf_debugstr_fourcc(fourcc), width, height, size);

    memcpy(&subtype, &MFVideoFormat_Base, sizeof(subtype));
    subtype.Data1 = fourcc;

    if ((format = mf_get_video_format(&subtype)))
        stride = mf_get_stride_for_format(format, width);
    else
        stride = 0;

    switch (fourcc)
    {
        case MAKEFOURCC('I','M','C','2'):
        case MAKEFOURCC('I','M','C','4'):
        case MAKEFOURCC('N','V','1','2'):
        case MAKEFOURCC('Y','V','1','2'):
        case MAKEFOURCC('I','4','2','0'):
        case MAKEFOURCC('I','Y','U','V'):
        case MAKEFOURCC('N','V','1','1'):
            *size = stride * height * 3 / 2;
            break;
        default:
            *size = stride * height;
    }

    return S_OK;
}

/***********************************************************************
 *      MFCompareFullToPartialMediaType (mfplat.@)
 */
BOOL WINAPI MFCompareFullToPartialMediaType(IMFMediaType *full_type, IMFMediaType *partial_type)
{
    BOOL result;
    GUID major;

    TRACE("%p, %p.\n", full_type, partial_type);

    if (FAILED(IMFMediaType_GetMajorType(partial_type, &major)))
        return FALSE;

    if (FAILED(IMFMediaType_Compare(partial_type, (IMFAttributes *)full_type, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result)))
        return FALSE;

    return result;
}

/***********************************************************************
 *      MFWrapMediaType (mfplat.@)
 */
HRESULT WINAPI MFWrapMediaType(IMFMediaType *original, REFGUID major, REFGUID subtype, IMFMediaType **ret)
{
    IMFMediaType *mediatype;
    UINT8 *buffer;
    UINT32 size;
    HRESULT hr;

    TRACE("%p, %s, %s, %p.\n", original, debugstr_guid(major), debugstr_guid(subtype), ret);

    if (FAILED(hr = MFGetAttributesAsBlobSize((IMFAttributes *)original, &size)))
        return hr;

    if (!(buffer = malloc(size)))
        return E_OUTOFMEMORY;

    if (FAILED(hr = MFGetAttributesAsBlob((IMFAttributes *)original, buffer, size)))
        goto failed;

    if (FAILED(hr = MFCreateMediaType(&mediatype)))
        goto failed;

    if (FAILED(hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, major)))
        goto failed;

    if (FAILED(hr = IMFMediaType_SetGUID(mediatype, &MF_MT_SUBTYPE, subtype)))
        goto failed;

    if (FAILED(hr = IMFMediaType_SetBlob(mediatype, &MF_MT_WRAPPED_TYPE, buffer, size)))
        goto failed;

    *ret = mediatype;

failed:
    free(buffer);

    return hr;
}

/***********************************************************************
 *      MFUnwrapMediaType (mfplat.@)
 */
HRESULT WINAPI MFUnwrapMediaType(IMFMediaType *wrapper, IMFMediaType **ret)
{
    IMFMediaType *mediatype;
    UINT8 *buffer;
    UINT32 size;
    HRESULT hr;

    TRACE("%p, %p.\n", wrapper, ret);

    if (FAILED(hr = MFCreateMediaType(&mediatype)))
        return hr;

    if (FAILED(hr = IMFMediaType_GetAllocatedBlob(wrapper, &MF_MT_WRAPPED_TYPE, &buffer, &size)))
    {
        IMFMediaType_Release(mediatype);
        return hr;
    }

    hr = MFInitAttributesFromBlob((IMFAttributes *)mediatype, buffer, size);
    CoTaskMemFree(buffer);
    if (FAILED(hr))
        return hr;

    *ret = mediatype;

    return S_OK;
}

static UINT32 media_type_get_uint32(IMFMediaType *media_type, REFGUID guid)
{
    UINT32 value;
    return SUCCEEDED(IMFMediaType_GetUINT32(media_type, guid, &value)) ? value : 0;
}

/***********************************************************************
 *      MFCreateWaveFormatExFromMFMediaType (mfplat.@)
 */
HRESULT WINAPI MFCreateWaveFormatExFromMFMediaType(IMFMediaType *mediatype, WAVEFORMATEX **ret_format,
        UINT32 *size, UINT32 flags)
{
    UINT32 extra_size = 0, user_size;
    WAVEFORMATEX *format;
    GUID major, subtype, basetype = MFAudioFormat_Base;
    void *user_data;
    HRESULT hr;

    TRACE("%p, %p, %p, %#x.\n", mediatype, ret_format, size, flags);

    if (FAILED(hr = IMFMediaType_GetGUID(mediatype, &MF_MT_MAJOR_TYPE, &major)))
        return hr;

    if (FAILED(hr = IMFMediaType_GetGUID(mediatype, &MF_MT_SUBTYPE, &subtype)))
        return hr;

    if (!IsEqualGUID(&major, &MFMediaType_Audio))
        return E_INVALIDARG;

    if (FAILED(hr = IMFMediaType_GetBlobSize(mediatype, &MF_MT_USER_DATA, &user_size)))
        user_size = 0;

    if (media_type_get_uint32(mediatype, &MF_MT_AUDIO_NUM_CHANNELS) > 2
            && SUCCEEDED(IMFMediaType_GetItem(mediatype, &MF_MT_AUDIO_CHANNEL_MASK, NULL)))
    {
        if (SUCCEEDED(IMFMediaType_GetItem(mediatype, &MF_MT_AUDIO_VALID_BITS_PER_SAMPLE, NULL)))
            flags = MFWaveFormatExConvertFlag_ForceExtensible;
        if (SUCCEEDED(IMFMediaType_GetItem(mediatype, &MF_MT_AUDIO_SAMPLES_PER_BLOCK, NULL)))
            flags = MFWaveFormatExConvertFlag_ForceExtensible;
    }

    basetype.Data1 = subtype.Data1;
    if (subtype.Data1 >> 16 || !IsEqualGUID(&subtype, &basetype))
        flags = MFWaveFormatExConvertFlag_ForceExtensible;

    if (flags == MFWaveFormatExConvertFlag_ForceExtensible)
        extra_size = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(*format);

    *size = sizeof(*format) + user_size + extra_size;
    if (!(format = CoTaskMemAlloc(*size)))
        return E_OUTOFMEMORY;

    memset(format, 0, *size);
    format->wFormatTag = subtype.Data1;
    format->cbSize = user_size + extra_size;
    user_data = format + 1;

    format->nChannels = media_type_get_uint32(mediatype, &MF_MT_AUDIO_NUM_CHANNELS);
    format->nSamplesPerSec = media_type_get_uint32(mediatype, &MF_MT_AUDIO_SAMPLES_PER_SECOND);
    format->nAvgBytesPerSec = media_type_get_uint32(mediatype, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND);
    format->nBlockAlign = media_type_get_uint32(mediatype, &MF_MT_AUDIO_BLOCK_ALIGNMENT);
    format->wBitsPerSample = media_type_get_uint32(mediatype, &MF_MT_AUDIO_BITS_PER_SAMPLE);

    if (flags == MFWaveFormatExConvertFlag_ForceExtensible)
    {
        WAVEFORMATEXTENSIBLE *format_ext = CONTAINING_RECORD(format, WAVEFORMATEXTENSIBLE, Format);

        format_ext->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        format_ext->SubFormat = subtype;
        user_data = format_ext + 1;

        format_ext->Samples.wValidBitsPerSample = media_type_get_uint32(mediatype, &MF_MT_AUDIO_VALID_BITS_PER_SAMPLE);
        format_ext->Samples.wSamplesPerBlock = media_type_get_uint32(mediatype, &MF_MT_AUDIO_SAMPLES_PER_BLOCK);

        if (SUCCEEDED(IMFMediaType_GetItem(mediatype, &MF_MT_AUDIO_CHANNEL_MASK, NULL)))
            format_ext->dwChannelMask = media_type_get_uint32(mediatype, &MF_MT_AUDIO_CHANNEL_MASK);
        else if (format_ext->Format.nChannels < ARRAY_SIZE(default_channel_mask))
            format_ext->dwChannelMask = default_channel_mask[format_ext->Format.nChannels];
    }

    IMFMediaType_GetBlob(mediatype, &MF_MT_USER_DATA, user_data, user_size, NULL);

    *ret_format = format;

    return S_OK;
}

static void mediatype_set_uint32(IMFMediaType *mediatype, const GUID *attr, unsigned int value, HRESULT *hr)
{
    if (SUCCEEDED(*hr))
        *hr = IMFMediaType_SetUINT32(mediatype, attr, value);
}

static void mediatype_set_uint64(IMFMediaType *mediatype, const GUID *attr, unsigned int high, unsigned int low, HRESULT *hr)
{
    if (SUCCEEDED(*hr))
        *hr = IMFMediaType_SetUINT64(mediatype, attr, (UINT64)high << 32 | low);
}

static void mediatype_set_guid(IMFMediaType *mediatype, const GUID *attr, const GUID *value, HRESULT *hr)
{
    if (SUCCEEDED(*hr))
        *hr = IMFMediaType_SetGUID(mediatype, attr, value);
}

static void mediatype_set_blob(IMFMediaType *mediatype, const GUID *attr, const UINT8 *data,
        unsigned int size, HRESULT *hr)
{
    if (SUCCEEDED(*hr))
        *hr = IMFMediaType_SetBlob(mediatype, attr, data, size);
}

/***********************************************************************
 *      MFInitMediaTypeFromWaveFormatEx (mfplat.@)
 */
HRESULT WINAPI MFInitMediaTypeFromWaveFormatEx(IMFMediaType *mediatype, const WAVEFORMATEX *format, UINT32 size)
{
    const WAVEFORMATEXTENSIBLE *wfex = (const WAVEFORMATEXTENSIBLE *)format;
    const void *user_data;
    int user_data_size;
    GUID subtype;
    HRESULT hr;

    TRACE("%p, %p, %u.\n", mediatype, format, size);

    if (!mediatype || !format)
        return E_POINTER;

    if (format->cbSize + sizeof(*format) > size)
        return E_INVALIDARG;

    hr = IMFMediaType_DeleteAllItems(mediatype);

    mediatype_set_guid(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio, &hr);

    if (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        memcpy(&subtype, &wfex->SubFormat, sizeof(subtype));

        if (wfex->dwChannelMask)
            mediatype_set_uint32(mediatype, &MF_MT_AUDIO_CHANNEL_MASK, wfex->dwChannelMask, &hr);

        if (format->wBitsPerSample && wfex->Samples.wValidBitsPerSample)
            mediatype_set_uint32(mediatype, &MF_MT_AUDIO_VALID_BITS_PER_SAMPLE, wfex->Samples.wValidBitsPerSample, &hr);

        user_data_size = format->cbSize - sizeof(WAVEFORMATEXTENSIBLE) + sizeof(WAVEFORMATEX);
        user_data = wfex + 1;
    }
    else
    {
        memcpy(&subtype, &MFAudioFormat_Base, sizeof(subtype));
        subtype.Data1 = format->wFormatTag;

        mediatype_set_uint32(mediatype, &MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1, &hr);
        user_data_size = format->cbSize;
        user_data = format + 1;
    }
    mediatype_set_guid(mediatype, &MF_MT_SUBTYPE, &subtype, &hr);

    if (format->nChannels)
        mediatype_set_uint32(mediatype, &MF_MT_AUDIO_NUM_CHANNELS, format->nChannels, &hr);

    if (format->nSamplesPerSec)
        mediatype_set_uint32(mediatype, &MF_MT_AUDIO_SAMPLES_PER_SECOND, format->nSamplesPerSec, &hr);

    if (format->nAvgBytesPerSec)
        mediatype_set_uint32(mediatype, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, format->nAvgBytesPerSec, &hr);

    if (format->nBlockAlign)
        mediatype_set_uint32(mediatype, &MF_MT_AUDIO_BLOCK_ALIGNMENT, format->nBlockAlign, &hr);

    if (format->wBitsPerSample)
        mediatype_set_uint32(mediatype, &MF_MT_AUDIO_BITS_PER_SAMPLE, format->wBitsPerSample, &hr);

    if (IsEqualGUID(&subtype, &MFAudioFormat_PCM) ||
            IsEqualGUID(&subtype, &MFAudioFormat_Float))
    {
        mediatype_set_uint32(mediatype, &MF_MT_ALL_SAMPLES_INDEPENDENT, 1, &hr);
    }

    if (IsEqualGUID(&subtype, &MFAudioFormat_AAC))
    {
        HEAACWAVEINFO *info = CONTAINING_RECORD(format, HEAACWAVEINFO, wfx);
        if (format->cbSize < sizeof(HEAACWAVEINFO) - sizeof(WAVEFORMATEX))
            return E_INVALIDARG;
        mediatype_set_uint32(mediatype, &MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, info->wAudioProfileLevelIndication, &hr);
        mediatype_set_uint32(mediatype, &MF_MT_AAC_PAYLOAD_TYPE, info->wPayloadType, &hr);
    }

    if (user_data_size > 0)
        mediatype_set_blob(mediatype, &MF_MT_USER_DATA, user_data, user_data_size, &hr);

    return hr;
}

/***********************************************************************
 *      MFCreateVideoMediaTypeFromSubtype (mfplat.@)
 */
HRESULT WINAPI MFCreateVideoMediaTypeFromSubtype(const GUID *subtype, IMFVideoMediaType **media_type)
{
    struct media_type *object;
    HRESULT hr;

    TRACE("%s, %p.\n", debugstr_guid(subtype), media_type);

    if (!media_type)
        return E_INVALIDARG;

    if (FAILED(hr = create_media_type(&object)))
        return hr;

    IMFMediaType_SetGUID(&object->IMFMediaType_iface, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    IMFMediaType_SetGUID(&object->IMFMediaType_iface, &MF_MT_SUBTYPE, subtype);

    *media_type = &object->IMFVideoMediaType_iface;

    return S_OK;
}

/***********************************************************************
 *      MFCreateAudioMediaType (mfplat.@)
 */
HRESULT WINAPI MFCreateAudioMediaType(const WAVEFORMATEX *format, IMFAudioMediaType **media_type)
{
    struct media_type *object;
    HRESULT hr;

    TRACE("%p, %p.\n", format, media_type);

    if (!media_type)
        return E_INVALIDARG;

    if (FAILED(hr = create_media_type(&object)))
        return hr;

    if (FAILED(hr = MFInitMediaTypeFromWaveFormatEx(&object->IMFMediaType_iface, format, sizeof(*format) + format->cbSize)))
    {
        IMFMediaType_Release(&object->IMFMediaType_iface);
        return hr;
    }

    *media_type = &object->IMFAudioMediaType_iface;

    return S_OK;
}

static void media_type_get_ratio(IMFMediaType *media_type, const GUID *attr, DWORD *numerator,
        DWORD *denominator)
{
    UINT64 value;

    if (SUCCEEDED(IMFMediaType_GetUINT64(media_type, attr, &value)))
    {
        *numerator = value >> 32;
        *denominator = value;
    }
}

/***********************************************************************
 *      MFCreateAMMediaTypeFromMFMediaType (mfplat.@)
 */
HRESULT WINAPI MFCreateAMMediaTypeFromMFMediaType(IMFMediaType *media_type, GUID format, AM_MEDIA_TYPE **am_type)
{
    AM_MEDIA_TYPE *mt;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", media_type, debugstr_mf_guid(&format), am_type);

    *am_type = NULL;
    if (!(mt = CoTaskMemAlloc(sizeof(*mt))))
        return E_OUTOFMEMORY;
    if (FAILED(hr = MFInitAMMediaTypeFromMFMediaType(media_type, format, mt)))
    {
        CoTaskMemFree(mt);
        return hr;
    }

    *am_type = mt;
    return hr;
}

/***********************************************************************
 *      MFCreateMFVideoFormatFromMFMediaType (mfplat.@)
 */
HRESULT WINAPI MFCreateMFVideoFormatFromMFMediaType(IMFMediaType *media_type, MFVIDEOFORMAT **video_format, UINT32 *size)
{
    UINT32 palette_size = 0, user_data_size = 0;
    MFVIDEOFORMAT *format;
    INT32 stride;
    GUID guid;

    TRACE("%p, %p, %p.\n", media_type, video_format, size);

    if (SUCCEEDED(IMFMediaType_GetBlobSize(media_type, &MF_MT_PALETTE, &palette_size)))
        *size = offsetof(MFVIDEOFORMAT, surfaceInfo.Palette[palette_size / sizeof(MFPaletteEntry) + 1]);
    else
        *size = sizeof(*format);

    if (SUCCEEDED(IMFMediaType_GetBlobSize(media_type, &MF_MT_USER_DATA, &user_data_size)))
        *size += user_data_size;

    if (!(format = CoTaskMemAlloc(*size)))
        return E_OUTOFMEMORY;

    *video_format = format;

    memset(format, 0, sizeof(*format));
    format->dwSize = *size;

    if (SUCCEEDED(IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid)))
    {
        memcpy(&format->guidFormat, &guid, sizeof(guid));
        format->surfaceInfo.Format = guid.Data1;
    }

    media_type_get_ratio(media_type, &MF_MT_FRAME_SIZE, &format->videoInfo.dwWidth, &format->videoInfo.dwHeight);
    media_type_get_ratio(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &format->videoInfo.PixelAspectRatio.Numerator,
            &format->videoInfo.PixelAspectRatio.Denominator);
    media_type_get_ratio(media_type, &MF_MT_FRAME_RATE, &format->videoInfo.FramesPerSecond.Numerator,
            &format->videoInfo.FramesPerSecond.Denominator);

    format->videoInfo.SourceChromaSubsampling = media_type_get_uint32(media_type, &MF_MT_VIDEO_CHROMA_SITING);
    format->videoInfo.InterlaceMode = media_type_get_uint32(media_type, &MF_MT_INTERLACE_MODE);
    format->videoInfo.TransferFunction = media_type_get_uint32(media_type, &MF_MT_TRANSFER_FUNCTION);
    format->videoInfo.ColorPrimaries = media_type_get_uint32(media_type, &MF_MT_VIDEO_PRIMARIES);
    format->videoInfo.TransferMatrix = media_type_get_uint32(media_type, &MF_MT_YUV_MATRIX);
    format->videoInfo.SourceLighting = media_type_get_uint32(media_type, &MF_MT_VIDEO_LIGHTING);
    format->videoInfo.NominalRange = media_type_get_uint32(media_type, &MF_MT_VIDEO_NOMINAL_RANGE);
    IMFMediaType_GetBlob(media_type, &MF_MT_GEOMETRIC_APERTURE, (UINT8 *)&format->videoInfo.GeometricAperture,
           sizeof(format->videoInfo.GeometricAperture), NULL);
    IMFMediaType_GetBlob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (UINT8 *)&format->videoInfo.MinimumDisplayAperture,
           sizeof(format->videoInfo.MinimumDisplayAperture), NULL);

    /* Video flags. */
    format->videoInfo.VideoFlags |= media_type_get_uint32(media_type, &MF_MT_PAD_CONTROL_FLAGS) & MFVideoFlag_PAD_TO_Mask;
    format->videoInfo.VideoFlags |= (media_type_get_uint32(media_type, &MF_MT_SOURCE_CONTENT_HINT) << 2) & MFVideoFlag_SrcContentHintMask;
    format->videoInfo.VideoFlags |= (media_type_get_uint32(media_type, &MF_MT_DRM_FLAGS) << 5) & (MFVideoFlag_AnalogProtected | MFVideoFlag_DigitallyProtected);
    if (media_type_get_uint32(media_type, &MF_MT_PAN_SCAN_ENABLED))
    {
        format->videoInfo.VideoFlags |= MFVideoFlag_PanScanEnabled;
        IMFMediaType_GetBlob(media_type, &MF_MT_PAN_SCAN_APERTURE, (UINT8 *)&format->videoInfo.PanScanAperture,
               sizeof(format->videoInfo.PanScanAperture), NULL);
    }
    stride = media_type_get_uint32(media_type, &MF_MT_DEFAULT_STRIDE);
    if (stride < 0)
        format->videoInfo.VideoFlags |= MFVideoFlag_BottomUpLinearRep;

    format->compressedInfo.AvgBitrate = media_type_get_uint32(media_type, &MF_MT_AVG_BITRATE);
    format->compressedInfo.AvgBitErrorRate = media_type_get_uint32(media_type, &MF_MT_AVG_BIT_ERROR_RATE);
    format->compressedInfo.MaxKeyFrameSpacing = media_type_get_uint32(media_type, &MF_MT_MAX_KEYFRAME_SPACING);

    /* Palette. */
    if (palette_size)
    {
        format->surfaceInfo.PaletteEntries = palette_size / sizeof(*format->surfaceInfo.Palette);
        IMFMediaType_GetBlob(media_type, &MF_MT_PALETTE, (UINT8 *)format->surfaceInfo.Palette, palette_size, NULL);
    }

    if (user_data_size)
    {
        IMFMediaType_GetBlob(media_type, &MF_MT_USER_DATA, (UINT8 *)format + *size - user_data_size, user_data_size, NULL);
    }

    return S_OK;
}

/***********************************************************************
 *      MFConvertColorInfoToDXVA (mfplat.@)
 */
HRESULT WINAPI MFConvertColorInfoToDXVA(DWORD *dxva_info, const MFVIDEOFORMAT *format)
{
    struct
    {
        UINT SampleFormat           : 8;
        UINT VideoChromaSubsampling : 4;
        UINT NominalRange           : 3;
        UINT VideoTransferMatrix    : 3;
        UINT VideoLighting          : 4;
        UINT VideoPrimaries         : 5;
        UINT VideoTransferFunction  : 5;
    } *dxva_format = (void *)dxva_info;

    TRACE("%p, %p.\n", dxva_info, format);

    if (format->videoInfo.InterlaceMode == MFVideoInterlace_MixedInterlaceOrProgressive)
        dxva_format->SampleFormat = DXVA2_SampleFieldInterleavedEvenFirst;
    else
        dxva_format->SampleFormat = format->videoInfo.InterlaceMode;

    dxva_format->VideoChromaSubsampling = format->videoInfo.SourceChromaSubsampling;
    dxva_format->NominalRange = format->videoInfo.NominalRange;
    dxva_format->VideoTransferMatrix = format->videoInfo.TransferMatrix;
    dxva_format->VideoLighting = format->videoInfo.SourceLighting;
    dxva_format->VideoPrimaries = format->videoInfo.ColorPrimaries;
    dxva_format->VideoTransferFunction = format->videoInfo.TransferFunction;

    return S_OK;
}

struct frame_rate
{
    UINT64 time;
    UINT64 rate;
};

static const struct frame_rate known_rates[] =
{
#define KNOWN_RATE(ft,n,d) { ft, ((UINT64)n << 32) | d }
    KNOWN_RATE(417188, 24000, 1001),
    KNOWN_RATE(416667,    24,    1),
    KNOWN_RATE(400000,    25,    1),
    KNOWN_RATE(333667, 30000, 1001),
    KNOWN_RATE(333333,    30,    1),
    KNOWN_RATE(200000,    50,    1),
    KNOWN_RATE(166833, 60000, 1001),
    KNOWN_RATE(166667,    60,    1),
#undef KNOWN_RATE
};

static const struct frame_rate *known_rate_from_rate(UINT64 rate)
{
    UINT i;
    for (i = 0; i < ARRAY_SIZE(known_rates); i++)
    {
        if (rate == known_rates[i].rate)
            return known_rates + i;
    }
    return NULL;
}

static const struct frame_rate *known_rate_from_time(UINT64 time)
{
    UINT i;
    for (i = 0; i < ARRAY_SIZE(known_rates); i++)
    {
        if (time >= known_rates[i].time - 30
                && time <= known_rates[i].time + 30)
            return known_rates + i;
    }
    return NULL;
}

/***********************************************************************
 *      MFFrameRateToAverageTimePerFrame (mfplat.@)
 */
HRESULT WINAPI MFFrameRateToAverageTimePerFrame(UINT32 numerator, UINT32 denominator, UINT64 *avgframetime)
{
    UINT64 rate = ((UINT64)numerator << 32) | denominator;
    const struct frame_rate *entry;

    TRACE("%u, %u, %p.\n", numerator, denominator, avgframetime);

    if ((entry = known_rate_from_rate(rate)))
        *avgframetime = entry->time;
    else
        *avgframetime = numerator ? denominator * (UINT64)10000000 / numerator : 0;

    return S_OK;
}

static unsigned int get_gcd(unsigned int a, unsigned int b)
{
    unsigned int m;

    while (b)
    {
        m = a % b;
        a = b;
        b = m;
    }

    return a;
}

/***********************************************************************
 *      MFAverageTimePerFrameToFrameRate (mfplat.@)
 */
HRESULT WINAPI MFAverageTimePerFrameToFrameRate(UINT64 avgtime, UINT32 *numerator, UINT32 *denominator)
{
    const struct frame_rate *entry;
    unsigned int gcd;

    TRACE("%s, %p, %p.\n", wine_dbgstr_longlong(avgtime), numerator, denominator);

    if ((entry = known_rate_from_time(avgtime)))
    {
        *numerator = entry->rate >> 32;
        *denominator = entry->rate;
    }
    else if (avgtime)
    {
        if (avgtime > 100000000) avgtime = 100000000;
        gcd = get_gcd(10000000, avgtime);
        *numerator = 10000000 / gcd;
        *denominator = avgtime / gcd;
    }
    else
    {
        *numerator = *denominator = 0;
    }

    return S_OK;
}

/***********************************************************************
 *      MFMapDXGIFormatToDX9Format (mfplat.@)
 */
DWORD WINAPI MFMapDXGIFormatToDX9Format(DXGI_FORMAT dxgi_format)
{
    switch (dxgi_format)
    {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return D3DFMT_A32B32G32R32F;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return D3DFMT_A16B16G16R16F;
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            return D3DFMT_A16B16G16R16;
        case DXGI_FORMAT_R16G16B16A16_SNORM:
            return D3DFMT_Q16W16V16U16;
        case DXGI_FORMAT_R32G32_FLOAT:
            return D3DFMT_G32R32F;
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            return D3DFMT_A2B10G10R10;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            return D3DFMT_Q8W8V8U8;
        case DXGI_FORMAT_R16G16_FLOAT:
            return D3DFMT_G16R16F;
        case DXGI_FORMAT_R16G16_UNORM:
            return D3DFMT_G16R16;
        case DXGI_FORMAT_R16G16_SNORM:
            return D3DFMT_V16U16;
        case DXGI_FORMAT_D32_FLOAT:
            return D3DFMT_D32F_LOCKABLE;
        case DXGI_FORMAT_R32_FLOAT:
            return D3DFMT_R32F;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return D3DFMT_D24S8;
        case DXGI_FORMAT_R8G8_SNORM:
            return D3DFMT_V8U8;
        case DXGI_FORMAT_R16_FLOAT:
            return D3DFMT_R16F;
        case DXGI_FORMAT_D16_UNORM:
            return D3DFMT_D16_LOCKABLE;
        case DXGI_FORMAT_R16_UNORM:
            return D3DFMT_L16;
        case DXGI_FORMAT_R8_UNORM:
            return D3DFMT_L8;
        case DXGI_FORMAT_A8_UNORM:
            return D3DFMT_A8;
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            return D3DFMT_DXT1;
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
            return D3DFMT_DXT2;
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            return D3DFMT_DXT4;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return D3DFMT_A8B8G8R8;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return D3DFMT_A8R8G8B8;
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return D3DFMT_X8R8G8B8;
        case DXGI_FORMAT_AYUV:
            return MAKEFOURCC('A','Y','U','V');
        case DXGI_FORMAT_Y410:
            return MAKEFOURCC('Y','4','1','0');
        case DXGI_FORMAT_Y416:
            return MAKEFOURCC('Y','4','1','6');
        case DXGI_FORMAT_NV12:
            return MAKEFOURCC('N','V','1','2');
        case DXGI_FORMAT_P010:
            return MAKEFOURCC('P','0','1','0');
        case DXGI_FORMAT_P016:
            return MAKEFOURCC('P','0','1','6');
        case DXGI_FORMAT_420_OPAQUE:
            return MAKEFOURCC('4','2','0','O');
        case DXGI_FORMAT_YUY2:
            return D3DFMT_YUY2;
        case DXGI_FORMAT_Y210:
            return MAKEFOURCC('Y','2','1','0');
        case DXGI_FORMAT_Y216:
            return MAKEFOURCC('Y','2','1','6');
        case DXGI_FORMAT_NV11:
            return MAKEFOURCC('N','V','1','1');
        case DXGI_FORMAT_AI44:
            return MAKEFOURCC('A','I','4','4');
        case DXGI_FORMAT_IA44:
            return MAKEFOURCC('I','A','4','4');
        case DXGI_FORMAT_P8:
            return D3DFMT_P8;
        case DXGI_FORMAT_A8P8:
            return D3DFMT_A8P8;
        default:
            return 0;
    }
}

/***********************************************************************
 *      MFMapDX9FormatToDXGIFormat (mfplat.@)
 */
DXGI_FORMAT WINAPI MFMapDX9FormatToDXGIFormat(DWORD format)
{
    switch (format)
    {
        case D3DFMT_A32B32G32R32F:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case D3DFMT_A16B16G16R16F:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case D3DFMT_A16B16G16R16:
            return DXGI_FORMAT_R16G16B16A16_UNORM;
        case D3DFMT_Q16W16V16U16:
            return DXGI_FORMAT_R16G16B16A16_SNORM;
        case D3DFMT_G32R32F:
            return DXGI_FORMAT_R32G32_FLOAT;
        case D3DFMT_A2B10G10R10:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case D3DFMT_Q8W8V8U8:
            return DXGI_FORMAT_R8G8B8A8_SNORM;
        case D3DFMT_G16R16F:
            return DXGI_FORMAT_R16G16_FLOAT;
        case D3DFMT_G16R16:
            return DXGI_FORMAT_R16G16_UNORM;
        case D3DFMT_V16U16:
            return DXGI_FORMAT_R16G16_SNORM;
        case D3DFMT_D32F_LOCKABLE:
            return DXGI_FORMAT_D32_FLOAT;
        case D3DFMT_R32F:
            return DXGI_FORMAT_R32_FLOAT;
        case D3DFMT_D24S8:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case D3DFMT_V8U8:
            return DXGI_FORMAT_R8G8_SNORM;
        case D3DFMT_R16F:
            return DXGI_FORMAT_R16_FLOAT;
        case D3DFMT_L16:
            return DXGI_FORMAT_R16_UNORM;
        case D3DFMT_L8:
            return DXGI_FORMAT_R8_UNORM;
        case D3DFMT_A8:
            return DXGI_FORMAT_A8_UNORM;
        case D3DFMT_DXT1:
            return DXGI_FORMAT_BC1_UNORM;
        case D3DFMT_DXT2:
            return DXGI_FORMAT_BC2_UNORM;
        case D3DFMT_DXT4:
            return DXGI_FORMAT_BC3_UNORM;
        case D3DFMT_A8R8G8B8:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case D3DFMT_X8R8G8B8:
            return DXGI_FORMAT_B8G8R8X8_UNORM;
        case D3DFMT_A8B8G8R8:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case MAKEFOURCC('A','Y','U','V'):
            return DXGI_FORMAT_AYUV;
        case MAKEFOURCC('Y','4','1','0'):
            return DXGI_FORMAT_Y410;
        case MAKEFOURCC('Y','4','1','6'):
            return DXGI_FORMAT_Y416;
        case MAKEFOURCC('N','V','1','2'):
            return DXGI_FORMAT_NV12;
        case MAKEFOURCC('P','0','1','0'):
            return DXGI_FORMAT_P010;
        case MAKEFOURCC('P','0','1','6'):
            return DXGI_FORMAT_P016;
        case MAKEFOURCC('4','2','0','O'):
            return DXGI_FORMAT_420_OPAQUE;
        case D3DFMT_YUY2:
            return DXGI_FORMAT_YUY2;
        case MAKEFOURCC('Y','2','1','0'):
            return DXGI_FORMAT_Y210;
        case MAKEFOURCC('Y','2','1','6'):
            return DXGI_FORMAT_Y216;
        case MAKEFOURCC('N','V','1','1'):
            return DXGI_FORMAT_NV11;
        case MAKEFOURCC('A','I','4','4'):
            return DXGI_FORMAT_AI44;
        case MAKEFOURCC('I','A','4','4'):
            return DXGI_FORMAT_IA44;
        case D3DFMT_P8:
            return DXGI_FORMAT_P8;
        case D3DFMT_A8P8:
            return DXGI_FORMAT_A8P8;
        default:
            return DXGI_FORMAT_UNKNOWN;
    }
}

/***********************************************************************
 *      MFInitVideoFormat_RGB (mfplat.@)
 */
HRESULT WINAPI MFInitVideoFormat_RGB(MFVIDEOFORMAT *format, DWORD width, DWORD height, DWORD d3dformat)
{
    unsigned int transfer_function;

    TRACE("%p, %lu, %lu, %#lx.\n", format, width, height, d3dformat);

    if (!format)
        return E_INVALIDARG;

    if (!d3dformat) d3dformat = D3DFMT_X8R8G8B8;

    switch (d3dformat)
    {
        case D3DFMT_X8R8G8B8:
        case D3DFMT_R8G8B8:
        case D3DFMT_A8R8G8B8:
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A2B10G10R10:
        case D3DFMT_A2R10G10B10:
        case D3DFMT_P8:
            transfer_function = MFVideoTransFunc_sRGB;
            break;
        default:
            transfer_function = MFVideoTransFunc_10;
    }

    memset(format, 0, sizeof(*format));
    format->dwSize = sizeof(*format);
    format->videoInfo.dwWidth = width;
    format->videoInfo.dwHeight = height;
    format->videoInfo.PixelAspectRatio.Numerator = 1;
    format->videoInfo.PixelAspectRatio.Denominator = 1;
    format->videoInfo.InterlaceMode = MFVideoInterlace_Progressive;
    format->videoInfo.TransferFunction = transfer_function;
    format->videoInfo.ColorPrimaries = MFVideoPrimaries_BT709;
    format->videoInfo.SourceLighting = MFVideoLighting_office;
    format->videoInfo.FramesPerSecond.Numerator = 60;
    format->videoInfo.FramesPerSecond.Denominator = 1;
    format->videoInfo.NominalRange = MFNominalRange_Normal;
    format->videoInfo.GeometricAperture.Area.cx = width;
    format->videoInfo.GeometricAperture.Area.cy = height;
    format->videoInfo.MinimumDisplayAperture = format->videoInfo.GeometricAperture;
    memcpy(&format->guidFormat, &MFVideoFormat_Base, sizeof(format->guidFormat));
    format->guidFormat.Data1 = d3dformat;
    format->surfaceInfo.Format = d3dformat;

    return S_OK;
}

static HRESULT mf_get_stride_for_bitmap_info_header(DWORD fourcc, const BITMAPINFOHEADER *bih, LONG *stride)
{
    HRESULT hr;

    if (FAILED(hr = MFGetStrideForBitmapInfoHeader(fourcc, bih->biWidth, stride))) return hr;
    if (bih->biHeight < 0) *stride *= -1;

    return hr;
}

static const GUID * get_mf_subtype_for_am_subtype(const GUID *subtype)
{
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_RGB1))
        return &MFVideoFormat_RGB1;
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_RGB4))
        return &MFVideoFormat_RGB4;
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_RGB8))
        return &MFVideoFormat_RGB8;
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_RGB555))
        return &MFVideoFormat_RGB555;
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_RGB565))
        return &MFVideoFormat_RGB565;
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_RGB24))
        return &MFVideoFormat_RGB24;
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_RGB32))
        return &MFVideoFormat_RGB32;
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_ARGB1555))
        return &MFVideoFormat_ARGB1555;
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_ARGB4444))
        return &MFVideoFormat_ARGB4444;
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_ARGB32))
        return &MFVideoFormat_ARGB32;
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_A2R10G10B10))
        return &MFVideoFormat_A2B10G10R10;
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_A2B10G10R10))
        return &MFVideoFormat_A2R10G10B10;
    return subtype;
}

HRESULT WINAPI MFCreateVideoMediaType(const MFVIDEOFORMAT *format, IMFVideoMediaType **media_type)
{
    struct media_type *object;
    HRESULT hr;

    TRACE("%p, %p.\n", format, media_type);

    if (!media_type)
        return E_INVALIDARG;

    if (FAILED(hr = create_media_type(&object)))
        return hr;

    if (FAILED(hr = MFInitMediaTypeFromMFVideoFormat(&object->IMFMediaType_iface, format, format->dwSize)))
    {
        IMFMediaType_Release(&object->IMFMediaType_iface);
        return hr;
    }

    *media_type = &object->IMFVideoMediaType_iface;

    return hr;
}

/***********************************************************************
 *      MFCreateVideoMediaTypeFromVideoInfoHeader (mfplat.@)
 */
HRESULT WINAPI MFCreateVideoMediaTypeFromVideoInfoHeader(const KS_VIDEOINFOHEADER *vih, DWORD size, DWORD pixel_aspect_ratio_x,
        DWORD pixel_aspect_ratio_y, MFVideoInterlaceMode interlace_mode, QWORD video_flags, const GUID *subtype,
        IMFVideoMediaType **ret)
{
    FIXME("%p, %lu, %lu, %lu, %d, %I64x, %s, %p.\n", vih, size, pixel_aspect_ratio_x, pixel_aspect_ratio_y, interlace_mode,
            video_flags, debugstr_guid(subtype), ret);

    return E_NOTIMPL;
}

/***********************************************************************
 *      MFInitMediaTypeFromMFVideoFormat (mfplat.@)
 */
HRESULT WINAPI MFInitMediaTypeFromMFVideoFormat(IMFMediaType *media_type, const MFVIDEOFORMAT *format, UINT32 size)
{
    UINT32 stride, sample_size, palette_size, user_data_size, value;
    struct uncompressed_video_format *video_format;
    const void *user_data;
    HRESULT hr = S_OK;

    TRACE("%p, %p, %u\n", media_type, format, size);

    if (!format || size < sizeof(*format) || format->dwSize != size)
        return E_INVALIDARG;
    if (size < offsetof(MFVIDEOFORMAT, surfaceInfo.Palette[format->surfaceInfo.PaletteEntries + 1]))
        return E_INVALIDARG;

    mediatype_set_guid(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video, &hr);
    if (!IsEqualGUID(&format->guidFormat, &GUID_NULL))
        mediatype_set_guid(media_type, &MF_MT_SUBTYPE, &format->guidFormat, &hr);
    if ((video_format = mf_get_video_format(&format->guidFormat)))
    {
        mediatype_set_uint32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, 1, &hr);
        mediatype_set_uint32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, 1, &hr);
    }

    if (format->videoInfo.dwWidth && format->videoInfo.dwHeight)
    {
        mediatype_set_uint64(media_type, &MF_MT_FRAME_SIZE, format->videoInfo.dwWidth, format->videoInfo.dwHeight, &hr);

        if (video_format && (stride = mf_get_stride_for_format(video_format, format->videoInfo.dwWidth)))
        {
            if (!video_format->yuv && (format->videoInfo.VideoFlags & MFVideoFlag_BottomUpLinearRep))
                stride = -stride;
            mediatype_set_uint32(media_type, &MF_MT_DEFAULT_STRIDE, stride, &hr);
        }

        if (SUCCEEDED(MFCalculateImageSize(&format->guidFormat, format->videoInfo.dwWidth, format->videoInfo.dwHeight, &sample_size)))
            mediatype_set_uint32(media_type, &MF_MT_SAMPLE_SIZE, sample_size, &hr);
    }

    if (format->videoInfo.PixelAspectRatio.Denominator)
        mediatype_set_uint64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, format->videoInfo.PixelAspectRatio.Numerator,
                format->videoInfo.PixelAspectRatio.Denominator, &hr);
    if (format->videoInfo.SourceChromaSubsampling)
        mediatype_set_uint32(media_type, &MF_MT_VIDEO_CHROMA_SITING, format->videoInfo.SourceChromaSubsampling, &hr);
    if (format->videoInfo.InterlaceMode)
        mediatype_set_uint32(media_type, &MF_MT_INTERLACE_MODE, format->videoInfo.InterlaceMode, &hr);
    if (format->videoInfo.TransferFunction)
        mediatype_set_uint32(media_type, &MF_MT_TRANSFER_FUNCTION, format->videoInfo.TransferFunction, &hr);
    if (format->videoInfo.ColorPrimaries)
        mediatype_set_uint32(media_type, &MF_MT_VIDEO_PRIMARIES, format->videoInfo.ColorPrimaries, &hr);
    if (format->videoInfo.TransferMatrix)
        mediatype_set_uint32(media_type, &MF_MT_YUV_MATRIX, format->videoInfo.TransferMatrix, &hr);
    if (format->videoInfo.SourceLighting)
        mediatype_set_uint32(media_type, &MF_MT_VIDEO_LIGHTING, format->videoInfo.SourceLighting, &hr);
    if (format->videoInfo.FramesPerSecond.Denominator)
        mediatype_set_uint64(media_type, &MF_MT_FRAME_RATE, format->videoInfo.FramesPerSecond.Numerator,
                format->videoInfo.FramesPerSecond.Denominator, &hr);
    if (format->videoInfo.NominalRange)
        mediatype_set_uint32(media_type, &MF_MT_VIDEO_NOMINAL_RANGE, format->videoInfo.NominalRange, &hr);
    if (format->videoInfo.GeometricAperture.Area.cx && format->videoInfo.GeometricAperture.Area.cy)
        mediatype_set_blob(media_type, &MF_MT_GEOMETRIC_APERTURE, (BYTE *)&format->videoInfo.GeometricAperture,
                sizeof(format->videoInfo.GeometricAperture), &hr);
    if (format->videoInfo.MinimumDisplayAperture.Area.cx && format->videoInfo.MinimumDisplayAperture.Area.cy)
        mediatype_set_blob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE *)&format->videoInfo.MinimumDisplayAperture,
                sizeof(format->videoInfo.MinimumDisplayAperture), &hr);
    if (format->videoInfo.PanScanAperture.Area.cx && format->videoInfo.PanScanAperture.Area.cy)
        mediatype_set_blob(media_type, &MF_MT_PAN_SCAN_APERTURE, (BYTE *)&format->videoInfo.PanScanAperture,
                sizeof(format->videoInfo.PanScanAperture), &hr);
    if ((value = !!(format->videoInfo.VideoFlags & MFVideoFlag_PanScanEnabled)))
        mediatype_set_uint32(media_type, &MF_MT_PAN_SCAN_ENABLED, value, &hr);
    if ((value = format->videoInfo.VideoFlags & MFVideoFlag_PAD_TO_Mask))
        mediatype_set_uint32(media_type, &MF_MT_PAD_CONTROL_FLAGS, value, &hr);
    if ((value = format->videoInfo.VideoFlags & MFVideoFlag_SrcContentHintMask))
        mediatype_set_uint32(media_type, &MF_MT_SOURCE_CONTENT_HINT, value >> 2, &hr);
    if ((value = format->videoInfo.VideoFlags & (MFVideoFlag_AnalogProtected | MFVideoFlag_DigitallyProtected)))
        mediatype_set_uint32(media_type, &MF_MT_DRM_FLAGS, value >> 5, &hr);

    if (format->compressedInfo.AvgBitrate)
        mediatype_set_uint32(media_type, &MF_MT_AVG_BITRATE, format->compressedInfo.AvgBitrate, &hr);
    if (format->compressedInfo.AvgBitErrorRate)
        mediatype_set_uint32(media_type, &MF_MT_AVG_BIT_ERROR_RATE, format->compressedInfo.AvgBitErrorRate, &hr);
    if (format->compressedInfo.MaxKeyFrameSpacing)
        mediatype_set_uint32(media_type, &MF_MT_MAX_KEYFRAME_SPACING, format->compressedInfo.MaxKeyFrameSpacing, &hr);

    if (!(palette_size = format->surfaceInfo.PaletteEntries * sizeof(*format->surfaceInfo.Palette)))
        user_data = format + 1;
    else
    {
        mediatype_set_blob(media_type, &MF_MT_PALETTE, (BYTE *)format->surfaceInfo.Palette, palette_size, &hr);
        user_data = &format->surfaceInfo.Palette[format->surfaceInfo.PaletteEntries + 1];
    }

    if ((user_data_size = (BYTE *)format + format->dwSize - (BYTE *)user_data))
        mediatype_set_blob(media_type, &MF_MT_USER_DATA, user_data, user_data_size, &hr);

    return hr;
}

/***********************************************************************
 *      MFInitMediaTypeFromVideoInfoHeader2 (mfplat.@)
 */
HRESULT WINAPI MFInitMediaTypeFromVideoInfoHeader2(IMFMediaType *media_type, const VIDEOINFOHEADER2 *vih, UINT32 size,
        const GUID *subtype)
{
    HRESULT hr = S_OK;
    DWORD height;
    LONG stride;

    TRACE("%p, %p, %u, %s.\n", media_type, vih, size, debugstr_guid(subtype));

    IMFMediaType_DeleteAllItems(media_type);

    if (!subtype)
    {
        switch (vih->bmiHeader.biBitCount)
        {
        case 1: subtype = &MFVideoFormat_RGB1; break;
        case 4: subtype = &MFVideoFormat_RGB4; break;
        case 8: subtype = &MFVideoFormat_RGB8; break;
        case 16: subtype = &MFVideoFormat_RGB555; break;
        case 24: subtype = &MFVideoFormat_RGB24; break;
        case 32: subtype = &MFVideoFormat_RGB32; break;
        default: return E_INVALIDARG;
        }
    }

    height = abs(vih->bmiHeader.biHeight);

    mediatype_set_guid(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video, &hr);
    mediatype_set_guid(media_type, &MF_MT_SUBTYPE, subtype, &hr);
    mediatype_set_uint64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, 1, 1, &hr);
    mediatype_set_uint64(media_type, &MF_MT_FRAME_SIZE, vih->bmiHeader.biWidth, height, &hr);

    if (SUCCEEDED(mf_get_stride_for_bitmap_info_header(subtype->Data1, &vih->bmiHeader, &stride)))
    {
        mediatype_set_uint32(media_type, &MF_MT_DEFAULT_STRIDE, stride, &hr);
        mediatype_set_uint32(media_type, &MF_MT_SAMPLE_SIZE, abs(stride) * height, &hr);
        mediatype_set_uint32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, 1, &hr);
        mediatype_set_uint32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, 1, &hr);
    }

    if (vih->bmiHeader.biSizeImage)
        mediatype_set_uint32(media_type, &MF_MT_SAMPLE_SIZE, vih->bmiHeader.biSizeImage, &hr);

    if (vih->rcSource.left || vih->rcSource.top || vih->rcSource.right || vih->rcSource.bottom)
    {
        MFVideoArea aperture = {{0}};

        aperture.OffsetX.value = vih->rcSource.left;
        aperture.OffsetY.value = vih->rcSource.top;
        aperture.Area.cx = vih->rcSource.right - vih->rcSource.left;
        aperture.Area.cy = vih->rcSource.bottom - vih->rcSource.top;

        mediatype_set_uint32(media_type, &MF_MT_PAN_SCAN_ENABLED, 1, &hr);
        mediatype_set_blob(media_type, &MF_MT_PAN_SCAN_APERTURE, (BYTE *)&aperture, sizeof(aperture), &hr);
        mediatype_set_blob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE *)&aperture, sizeof(aperture), &hr);
    }

    if (SUCCEEDED(hr) && vih->AvgTimePerFrame)
    {
        UINT32 num, den;
        if (SUCCEEDED(hr = MFAverageTimePerFrameToFrameRate(vih->AvgTimePerFrame, &num, &den)))
            mediatype_set_uint64(media_type, &MF_MT_FRAME_RATE, num, den, &hr);
    }

    if (vih->dwControlFlags & AMCONTROL_COLORINFO_PRESENT)
    {
        DXVA_ExtendedFormat *format = (DXVA_ExtendedFormat *)&vih->dwControlFlags;

        if (format->VideoChromaSubsampling)
            mediatype_set_uint32(media_type, &MF_MT_VIDEO_CHROMA_SITING, format->VideoChromaSubsampling, &hr);
        if (format->NominalRange)
            mediatype_set_uint32(media_type, &MF_MT_VIDEO_NOMINAL_RANGE, format->NominalRange, &hr);
        if (format->VideoTransferMatrix)
            mediatype_set_uint32(media_type, &MF_MT_YUV_MATRIX, format->VideoTransferMatrix, &hr);
        if (format->VideoLighting)
            mediatype_set_uint32(media_type, &MF_MT_VIDEO_LIGHTING, format->VideoLighting, &hr);
        if (format->VideoPrimaries)
            mediatype_set_uint32(media_type, &MF_MT_VIDEO_PRIMARIES, format->VideoPrimaries, &hr);
        if (format->VideoTransferFunction)
            mediatype_set_uint32(media_type, &MF_MT_TRANSFER_FUNCTION, format->VideoTransferFunction, &hr);
    }

    if (!(vih->dwInterlaceFlags & AMINTERLACE_IsInterlaced))
        mediatype_set_uint32(media_type, &MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive, &hr);
    else if (vih->dwInterlaceFlags & AMINTERLACE_DisplayModeBobOrWeave)
        mediatype_set_uint32(media_type, &MF_MT_INTERLACE_MODE, MFVideoInterlace_MixedInterlaceOrProgressive, &hr);
    else
        FIXME("dwInterlaceFlags %#lx not implemented\n", vih->dwInterlaceFlags);

    if (size > sizeof(*vih)) mediatype_set_blob(media_type, &MF_MT_USER_DATA, (BYTE *)(vih + 1), size - sizeof(*vih), &hr);
    return hr;
}

/***********************************************************************
 *      MFInitMediaTypeFromVideoInfoHeader (mfplat.@)
 */
HRESULT WINAPI MFInitMediaTypeFromVideoInfoHeader(IMFMediaType *media_type, const VIDEOINFOHEADER *vih, UINT32 size,
        const GUID *subtype)
{
    VIDEOINFOHEADER2 vih2 =
    {
        .rcSource = vih->rcSource,
        .rcTarget = vih->rcTarget,
        .dwBitRate = vih->dwBitRate,
        .dwBitErrorRate = vih->dwBitErrorRate,
        .AvgTimePerFrame = vih->AvgTimePerFrame,
        .bmiHeader = vih->bmiHeader,
    };
    HRESULT hr;

    TRACE("%p, %p, %u, %s.\n", media_type, vih, size, debugstr_guid(subtype));

    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih2, sizeof(vih2), subtype);
    if (size > sizeof(*vih)) mediatype_set_blob(media_type, &MF_MT_USER_DATA, (BYTE *)(vih + 1), size - sizeof(*vih), &hr);
    return hr;
}

/***********************************************************************
 *      MFInitMediaTypeFromMPEG1VideoInfo (mfplat.@)
 */
HRESULT WINAPI MFInitMediaTypeFromMPEG1VideoInfo(IMFMediaType *media_type, const MPEG1VIDEOINFO *vih, UINT32 size,
        const GUID *subtype)
{
    HRESULT hr;

    TRACE("%p, %p, %u, %s.\n", media_type, vih, size, debugstr_guid(subtype));

    if (FAILED(hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih->hdr, sizeof(vih->hdr), subtype)))
        return hr;

    if (vih->dwStartTimeCode)
        mediatype_set_uint32(media_type, &MF_MT_MPEG_START_TIME_CODE, vih->dwStartTimeCode, &hr);
    if (vih->cbSequenceHeader)
        mediatype_set_blob(media_type, &MF_MT_MPEG_SEQUENCE_HEADER, vih->bSequenceHeader, vih->cbSequenceHeader, &hr);

    return hr;
}

/***********************************************************************
 *      MFInitMediaTypeFromMPEG2VideoInfo (mfplat.@)
 */
HRESULT WINAPI MFInitMediaTypeFromMPEG2VideoInfo(IMFMediaType *media_type, const MPEG2VIDEOINFO *vih, UINT32 size,
        const GUID *subtype)
{
    HRESULT hr;

    TRACE("%p, %p, %u, %s.\n", media_type, vih, size, debugstr_guid(subtype));

    if (FAILED(hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih->hdr, sizeof(vih->hdr), subtype)))
        return hr;

    if (vih->dwStartTimeCode)
        mediatype_set_uint32(media_type, &MF_MT_MPEG_START_TIME_CODE, vih->dwStartTimeCode, &hr);
    if (vih->cbSequenceHeader)
        mediatype_set_blob(media_type, &MF_MT_MPEG_SEQUENCE_HEADER, (BYTE *)vih->dwSequenceHeader, vih->cbSequenceHeader, &hr);

    if (vih->dwProfile)
        mediatype_set_uint32(media_type, &MF_MT_MPEG2_PROFILE, vih->dwProfile, &hr);
    if (vih->dwLevel)
        mediatype_set_uint32(media_type, &MF_MT_MPEG2_LEVEL, vih->dwLevel, &hr);
    if (vih->dwFlags)
        mediatype_set_uint32(media_type, &MF_MT_MPEG2_FLAGS, vih->dwFlags, &hr);

    return hr;
}

static HRESULT init_am_media_type_audio_format(AM_MEDIA_TYPE *am_type, IMFMediaType *media_type)
{
    HRESULT hr;

    if (IsEqualGUID(&am_type->formattype, &FORMAT_VideoInfo)
            || IsEqualGUID(&am_type->formattype, &FORMAT_VideoInfo2)
            || IsEqualGUID(&am_type->formattype, &FORMAT_MFVideoFormat))
        return E_INVALIDARG;

    if (IsEqualGUID(&am_type->formattype, &FORMAT_WaveFormatEx)
            || IsEqualGUID(&am_type->formattype, &GUID_NULL))
    {
        UINT32 flags = 0, num_channels = media_type_get_uint32(media_type, &MF_MT_AUDIO_NUM_CHANNELS);

        if (SUCCEEDED(IMFMediaType_GetItem(media_type, &MF_MT_AUDIO_CHANNEL_MASK, NULL))
                || SUCCEEDED(IMFMediaType_GetItem(media_type, &MF_MT_AUDIO_VALID_BITS_PER_SAMPLE, NULL))
                || SUCCEEDED(IMFMediaType_GetItem(media_type, &MF_MT_AUDIO_SAMPLES_PER_BLOCK, NULL))
                || num_channels > 2)
            flags = MFWaveFormatExConvertFlag_ForceExtensible;

        if (FAILED(hr = MFCreateWaveFormatExFromMFMediaType(media_type, (WAVEFORMATEX **)&am_type->pbFormat,
                (UINT32 *)&am_type->cbFormat, flags)))
            return hr;

        am_type->subtype = get_am_subtype_for_mf_subtype(am_type->subtype);
        am_type->formattype = FORMAT_WaveFormatEx;
    }
    else
    {
        WARN("Unknown format %s\n", debugstr_mf_guid(&am_type->formattype));
        am_type->formattype = GUID_NULL;
    }

    return S_OK;
}

static void init_video_info_header2(VIDEOINFOHEADER2 *vih, const GUID *subtype, IMFMediaType *media_type)
{
    struct uncompressed_video_format *video_format = mf_get_video_format(subtype);
    DXVA_ExtendedFormat *format = (DXVA_ExtendedFormat *)&vih->dwControlFlags;
    UINT32 image_size, width, height, value;
    UINT64 frame_size, frame_rate;

    vih->bmiHeader.biSize = sizeof(vih->bmiHeader);
    vih->bmiHeader.biPlanes = 1;
    vih->bmiHeader.biBitCount = video_format ? video_format->bpp : 0;

    if (video_format && video_format->compression != -1)
        vih->bmiHeader.biCompression = video_format->compression;
    else
        vih->bmiHeader.biCompression = subtype->Data1;

    vih->dwBitRate = media_type_get_uint32(media_type, &MF_MT_AVG_BITRATE);
    vih->dwBitErrorRate = media_type_get_uint32(media_type, &MF_MT_AVG_BIT_ERROR_RATE);
    if (SUCCEEDED(IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_RATE, &frame_rate)) && (frame_rate >> 32))
        vih->AvgTimePerFrame = round(10000000. * (UINT32)frame_rate / (frame_rate >> 32));
    vih->bmiHeader.biSizeImage = media_type_get_uint32(media_type, &MF_MT_SAMPLE_SIZE);

    if (SUCCEEDED(IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &frame_size)))
    {
        BOOL bottom_up = vih->bmiHeader.biCompression == BI_RGB || vih->bmiHeader.biCompression == BI_BITFIELDS;
        INT32 stride;

        width = frame_size >> 32;
        if (!(stride = media_type_get_uint32(media_type, &MF_MT_DEFAULT_STRIDE)))
            stride = width * (bottom_up ? -1 : 1);
        else if (video_format)
            stride /= video_format->bpp / 8;
        height = (UINT32)frame_size;

        vih->bmiHeader.biWidth = abs(stride);
        vih->bmiHeader.biHeight = height * (bottom_up && stride >= 0 ? -1 : 1);

        if (SUCCEEDED(MFCalculateImageSize(subtype, abs(stride), height, &image_size)))
            vih->bmiHeader.biSizeImage = image_size;

        if (vih->bmiHeader.biWidth > width)
        {
            vih->rcSource.right = vih->rcTarget.right = width;
            vih->rcSource.bottom = vih->rcTarget.bottom = height;
        }
    }

    format->VideoChromaSubsampling = media_type_get_uint32(media_type, &MF_MT_VIDEO_CHROMA_SITING);
    format->NominalRange = media_type_get_uint32(media_type, &MF_MT_VIDEO_NOMINAL_RANGE);
    format->VideoTransferMatrix = media_type_get_uint32(media_type, &MF_MT_YUV_MATRIX);
    format->VideoLighting = media_type_get_uint32(media_type, &MF_MT_VIDEO_LIGHTING);
    format->VideoPrimaries = media_type_get_uint32(media_type, &MF_MT_VIDEO_PRIMARIES);
    format->VideoTransferFunction = media_type_get_uint32(media_type, &MF_MT_TRANSFER_FUNCTION);

    if (format->VideoChromaSubsampling || format->NominalRange || format->VideoTransferMatrix
            || format->VideoLighting || format->VideoPrimaries || format->VideoTransferFunction)
        format->SampleFormat = AMCONTROL_COLORINFO_PRESENT;

    switch ((value = media_type_get_uint32(media_type, &MF_MT_INTERLACE_MODE)))
    {
    case MFVideoInterlace_Unknown:
    case MFVideoInterlace_Progressive:
        break;
    case MFVideoInterlace_MixedInterlaceOrProgressive:
        vih->dwInterlaceFlags = AMINTERLACE_DisplayModeBobOrWeave | AMINTERLACE_IsInterlaced;
        break;
    default:
        FIXME("MF_MT_INTERLACE_MODE %u not implemented!\n", value);
        vih->dwInterlaceFlags = AMINTERLACE_IsInterlaced;
        break;
    }
}

static void init_video_info_header(VIDEOINFOHEADER *vih, const GUID *subtype, IMFMediaType *media_type)
{
    VIDEOINFOHEADER2 vih2 = {{0}};

    init_video_info_header2(&vih2, subtype, media_type);

    vih->rcSource = vih2.rcSource;
    vih->rcTarget = vih2.rcTarget;
    vih->dwBitRate = vih2.dwBitRate;
    vih->dwBitErrorRate = vih2.dwBitErrorRate;
    vih->AvgTimePerFrame = vih2.AvgTimePerFrame;
    vih->bmiHeader = vih2.bmiHeader;
}

static UINT32 get_am_media_type_video_format_size(const GUID *format_type, IMFMediaType *media_type)
{
    if (IsEqualGUID(format_type, &FORMAT_VideoInfo))
    {
        UINT32 size = sizeof(VIDEOINFOHEADER), user_size;
        if (SUCCEEDED(IMFMediaType_GetBlobSize(media_type, &MF_MT_USER_DATA, &user_size)))
            size += user_size;
        return size;
    }

    if (IsEqualGUID(format_type, &FORMAT_VideoInfo2))
    {
        UINT32 size = sizeof(VIDEOINFOHEADER2), user_size;
        if (SUCCEEDED(IMFMediaType_GetBlobSize(media_type, &MF_MT_USER_DATA, &user_size)))
            size += user_size;
        return size;
    }

    if (IsEqualGUID(format_type, &FORMAT_MPEGVideo))
    {
        UINT32 size = sizeof(MPEG1VIDEOINFO), sequence_size;
        if (SUCCEEDED(IMFMediaType_GetBlobSize(media_type, &MF_MT_MPEG_SEQUENCE_HEADER, &sequence_size)))
            size += sequence_size;
        return size;
    }

    if (IsEqualGUID(format_type, &FORMAT_MPEG2Video))
    {
        UINT32 size = sizeof(MPEG2VIDEOINFO), sequence_size;
        if (SUCCEEDED(IMFMediaType_GetBlobSize(media_type, &MF_MT_MPEG_SEQUENCE_HEADER, &sequence_size)))
            size += sequence_size;
        return size;
    }

    return 0;
}

static HRESULT init_am_media_type_video_format(AM_MEDIA_TYPE *am_type, IMFMediaType *media_type)
{
    struct uncompressed_video_format *video_format = mf_get_video_format(&am_type->subtype);
    HRESULT hr;

    if (IsEqualGUID(&am_type->formattype, &FORMAT_WaveFormatEx))
        return E_INVALIDARG;

    if (IsEqualGUID(&am_type->formattype, &FORMAT_MFVideoFormat))
        return MFCreateMFVideoFormatFromMFMediaType(media_type, (MFVIDEOFORMAT **)&am_type->pbFormat,
                (UINT32 *)&am_type->cbFormat);

    if (IsEqualGUID(&am_type->formattype, &GUID_NULL))
    {
        if (IsEqualGUID(&am_type->subtype, &MEDIASUBTYPE_MPEG1Payload)
                || IsEqualGUID(&am_type->subtype, &MEDIASUBTYPE_MPEG1Packet))
            am_type->formattype = FORMAT_MPEGVideo;
        else if (IsEqualGUID(&am_type->subtype, &MEDIASUBTYPE_MPEG2_VIDEO))
            am_type->formattype = FORMAT_MPEG2Video;
        else
            am_type->formattype = FORMAT_VideoInfo;
    }

    am_type->cbFormat = get_am_media_type_video_format_size(&am_type->formattype, media_type);
    if (!(am_type->pbFormat = CoTaskMemAlloc(am_type->cbFormat)))
        return E_OUTOFMEMORY;
    memset(am_type->pbFormat, 0, am_type->cbFormat);

    if (IsEqualGUID(&am_type->formattype, &FORMAT_VideoInfo))
    {
        VIDEOINFOHEADER *format = (VIDEOINFOHEADER *)am_type->pbFormat;
        init_video_info_header(format, &am_type->subtype, media_type);

        if (am_type->cbFormat > sizeof(*format) && FAILED(hr = IMFMediaType_GetBlob(media_type, &MF_MT_USER_DATA,
                (BYTE *)(format + 1), am_type->cbFormat - sizeof(*format), NULL)))
            return hr;
        format->bmiHeader.biSize += am_type->cbFormat - sizeof(*format);

        am_type->subtype = get_am_subtype_for_mf_subtype(am_type->subtype);
        am_type->bFixedSizeSamples = !!video_format;
        am_type->bTemporalCompression = !video_format;
    }
    else if (IsEqualGUID(&am_type->formattype, &FORMAT_VideoInfo2))
    {
        VIDEOINFOHEADER2 *format = (VIDEOINFOHEADER2 *)am_type->pbFormat;
        init_video_info_header2(format, &am_type->subtype, media_type);

        if (am_type->cbFormat > sizeof(*format) && FAILED(hr = IMFMediaType_GetBlob(media_type, &MF_MT_USER_DATA,
                (BYTE *)(format + 1), am_type->cbFormat - sizeof(*format), NULL)))
            return hr;
        format->bmiHeader.biSize += am_type->cbFormat - sizeof(*format);

        am_type->subtype = get_am_subtype_for_mf_subtype(am_type->subtype);
        am_type->bFixedSizeSamples = !!video_format;
        am_type->bTemporalCompression = !video_format;
    }
    else if (IsEqualGUID(&am_type->formattype, &FORMAT_MPEGVideo))
    {
        MPEG1VIDEOINFO *format = (MPEG1VIDEOINFO *)am_type->pbFormat;

        init_video_info_header(&format->hdr, &am_type->subtype, media_type);
        format->hdr.bmiHeader.biSize = 0;

        format->dwStartTimeCode = media_type_get_uint32(media_type, &MF_MT_MPEG_START_TIME_CODE);

        if (am_type->cbFormat > sizeof(*format) && FAILED(hr = IMFMediaType_GetBlob(media_type, &MF_MT_MPEG_SEQUENCE_HEADER,
                format->bSequenceHeader, am_type->cbFormat - sizeof(*format), NULL)))
            return hr;
        format->cbSequenceHeader = am_type->cbFormat - sizeof(*format);

        am_type->subtype = get_am_subtype_for_mf_subtype(am_type->subtype);
        am_type->bFixedSizeSamples = !!video_format;
        am_type->bTemporalCompression = !video_format;
    }
    else if (IsEqualGUID(&am_type->formattype, &FORMAT_MPEG2Video))
    {
        MPEG2VIDEOINFO *format = (MPEG2VIDEOINFO *)am_type->pbFormat;

        init_video_info_header2(&format->hdr, &am_type->subtype, media_type);

        format->dwStartTimeCode = media_type_get_uint32(media_type, &MF_MT_MPEG_START_TIME_CODE);
        format->dwProfile = media_type_get_uint32(media_type, &MF_MT_MPEG2_PROFILE);
        format->dwLevel = media_type_get_uint32(media_type, &MF_MT_MPEG2_LEVEL);
        format->dwFlags = media_type_get_uint32(media_type, &MF_MT_MPEG2_FLAGS);

        if (am_type->cbFormat > sizeof(*format) && FAILED(hr = IMFMediaType_GetBlob(media_type, &MF_MT_MPEG_SEQUENCE_HEADER,
                (BYTE *)format->dwSequenceHeader, am_type->cbFormat - sizeof(*format), NULL)))
            return hr;
        format->cbSequenceHeader = am_type->cbFormat - sizeof(*format);

        am_type->subtype = get_am_subtype_for_mf_subtype(am_type->subtype);
        am_type->bFixedSizeSamples = !!video_format;
        am_type->bTemporalCompression = !video_format;
    }
    else
    {
        WARN("Unknown format %s\n", debugstr_mf_guid(&am_type->formattype));
        am_type->formattype = GUID_NULL;
    }

    return S_OK;
}

/***********************************************************************
 *      MFInitAMMediaTypeFromMFMediaType (mfplat.@)
 */
HRESULT WINAPI MFInitAMMediaTypeFromMFMediaType(IMFMediaType *media_type, GUID format, AM_MEDIA_TYPE *am_type)
{
    HRESULT hr;

    TRACE("%p, %s, %p.\n", media_type, debugstr_mf_guid(&format), am_type);

    memset(am_type, 0, sizeof(*am_type));
    am_type->formattype = format;

    if (FAILED(hr = IMFMediaType_GetMajorType(media_type, &am_type->majortype))
            || FAILED(hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &am_type->subtype)))
        goto done;

    am_type->bTemporalCompression = !media_type_get_uint32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT);
    am_type->bFixedSizeSamples = media_type_get_uint32(media_type, &MF_MT_FIXED_SIZE_SAMPLES);
    am_type->lSampleSize = media_type_get_uint32(media_type, &MF_MT_SAMPLE_SIZE);

    if (IsEqualGUID(&am_type->majortype, &MFMediaType_Audio))
        hr = init_am_media_type_audio_format(am_type, media_type);
    else if (IsEqualGUID(&am_type->majortype, &MFMediaType_Video))
        hr = init_am_media_type_video_format(am_type, media_type);
    else
    {
        FIXME("Not implemented!\n");
        hr = E_NOTIMPL;
    }

done:
    if (FAILED(hr))
    {
        CoTaskMemFree(am_type->pbFormat);
        am_type->pbFormat = NULL;
        am_type->cbFormat = 0;
    }

    return hr;
}

/***********************************************************************
 *      MFInitMediaTypeFromAMMediaType (mfplat.@)
 */
HRESULT WINAPI MFInitMediaTypeFromAMMediaType(IMFMediaType *media_type, const AM_MEDIA_TYPE *am_type)
{
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", media_type, am_type);

    IMFMediaType_DeleteAllItems(media_type);

    if (IsEqualGUID(&am_type->majortype, &MEDIATYPE_Video))
    {
        const GUID *subtype = get_mf_subtype_for_am_subtype(&am_type->subtype);

        if (am_type->cbFormat && !am_type->pbFormat)
            hr = E_INVALIDARG;
        else if (IsEqualGUID(&am_type->formattype, &FORMAT_VideoInfo)
                && am_type->cbFormat >= sizeof(VIDEOINFOHEADER))
            hr = MFInitMediaTypeFromVideoInfoHeader(media_type, (VIDEOINFOHEADER *)am_type->pbFormat, am_type->cbFormat, subtype);
        else if (IsEqualGUID(&am_type->formattype, &FORMAT_VideoInfo2)
                && am_type->cbFormat >= sizeof(VIDEOINFOHEADER2))
            hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, (VIDEOINFOHEADER2 *)am_type->pbFormat, am_type->cbFormat, subtype);
        else if (IsEqualGUID(&am_type->formattype, &FORMAT_MPEGVideo)
                && am_type->cbFormat >= sizeof(MPEG1VIDEOINFO))
            hr = MFInitMediaTypeFromMPEG1VideoInfo(media_type, (MPEG1VIDEOINFO *)am_type->pbFormat, am_type->cbFormat, subtype);
        else if (IsEqualGUID(&am_type->formattype, &FORMAT_MPEG2Video)
                && am_type->cbFormat >= sizeof(MPEG2VIDEOINFO))
            hr = MFInitMediaTypeFromMPEG2VideoInfo(media_type, (MPEG2VIDEOINFO *)am_type->pbFormat, am_type->cbFormat, subtype);
        else
        {
            FIXME("Unsupported format type %s / size %ld.\n", debugstr_guid(&am_type->formattype), am_type->cbFormat);
            return E_NOTIMPL;
        }
    }
    else if (IsEqualGUID(&am_type->majortype, &MEDIATYPE_Audio))
    {
        if (am_type->cbFormat && !am_type->pbFormat)
            hr = E_INVALIDARG;
        else if (IsEqualGUID(&am_type->formattype, &FORMAT_WaveFormatEx)
                && am_type->cbFormat >= sizeof(WAVEFORMATEX))
            hr = MFInitMediaTypeFromWaveFormatEx(media_type, (WAVEFORMATEX *)am_type->pbFormat, am_type->cbFormat);
        else if (IsEqualGUID(&am_type->formattype, &GUID_NULL))
        {
            mediatype_set_guid(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio, &hr);
            mediatype_set_guid(media_type, &MF_MT_SUBTYPE, &am_type->subtype, &hr);
            mediatype_set_guid(media_type, &MF_MT_AM_FORMAT_TYPE, &GUID_NULL, &hr);
            mediatype_set_uint32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, 1, &hr);
        }
        else
        {
            FIXME("Unsupported format type %s / size %ld.\n", debugstr_guid(&am_type->formattype), am_type->cbFormat);
            return E_NOTIMPL;
        }
    }
    else
    {
        FIXME("Unsupported major type %s.\n", debugstr_guid(&am_type->majortype));
        return E_NOTIMPL;
    }

    if (!am_type->bTemporalCompression && FAILED(IMFMediaType_GetItem(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, NULL)))
        mediatype_set_uint32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, 1, &hr);
    if (am_type->bFixedSizeSamples && FAILED(IMFMediaType_GetItem(media_type, &MF_MT_FIXED_SIZE_SAMPLES, NULL)))
        mediatype_set_uint32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, 1, &hr);
    if (am_type->lSampleSize && FAILED(IMFMediaType_GetItem(media_type, &MF_MT_SAMPLE_SIZE, NULL)))
        mediatype_set_uint32(media_type, &MF_MT_SAMPLE_SIZE, am_type->lSampleSize, &hr);

    return hr;
}

/***********************************************************************
 *      MFCreateMediaTypeFromRepresentation (mfplat.@)
 */
HRESULT WINAPI MFCreateMediaTypeFromRepresentation(GUID guid_representation, void *representation,
        IMFMediaType **media_type)
{
    HRESULT hr;

    TRACE("%s, %p, %p\n", debugstr_guid(&guid_representation), representation, media_type);

    if (!IsEqualGUID(&guid_representation, &AM_MEDIA_TYPE_REPRESENTATION))
        return MF_E_UNSUPPORTED_REPRESENTATION;
    if (!representation || !media_type)
        return E_INVALIDARG;

    if (FAILED(hr = MFCreateMediaType(media_type)))
        return hr;
    if (FAILED(hr = MFInitMediaTypeFromAMMediaType(*media_type, representation)))
    {
        IMFMediaType_Release(*media_type);
        *media_type = NULL;
    }

    return hr;
}
