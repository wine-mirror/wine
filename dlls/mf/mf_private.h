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

#include "mferror.h"
#include "mfidl.h"
#include "mfapi.h"

#include "wine/debug.h"

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

struct activate_funcs
{
    HRESULT (*create_object)(IMFAttributes *attributes, void *context, IUnknown **object);
    void (*shutdown_object)(void *context, IUnknown *object);
    void (*free_private)(void *context);
};

HRESULT create_activation_object(void *context, const struct activate_funcs *funcs, IMFActivate **ret);

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
            return wine_dbg_sprintf("%p {VT_CLSID: %s}", v, wine_dbgstr_guid(v->puuid));
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

extern HRESULT file_scheme_handler_construct(REFIID riid, void **obj);
extern HRESULT urlmon_scheme_handler_construct(REFIID riid, void **obj);

extern BOOL mf_is_sample_copier_transform(IMFTransform *transform);
extern BOOL mf_is_sar_sink(IMFMediaSink *sink);
extern HRESULT topology_node_get_object(IMFTopologyNode *node, REFIID riid, void **obj);
extern HRESULT topology_node_get_type_handler(IMFTopologyNode *node, DWORD stream, BOOL output, IMFMediaTypeHandler **handler);
extern HRESULT topology_node_init_media_type(IMFTopologyNode *node, DWORD stream, BOOL output, IMFMediaType **type);
