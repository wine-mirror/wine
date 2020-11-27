/*
 * DirectShow capture
 *
 * Copyright (C) 2005 Rolf Kalbermatter
 * Copyright 2005 Maarten Lankhorst
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
#ifndef _QCAP_PRIVATE_H_DEFINED
#define _QCAP_PRIVATE_H_DEFINED

#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "dshow.h"
#include "wine/debug.h"
#include "wine/strmbase.h"

extern HINSTANCE qcap_instance DECLSPEC_HIDDEN;

extern DWORD ObjectRefCount(BOOL increment) DECLSPEC_HIDDEN;

HRESULT audio_record_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT avi_compressor_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT avi_mux_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT capture_graph_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT file_writer_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT smart_tee_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT vfw_capture_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;

struct video_capture_funcs
{
    struct video_capture_device *(CDECL *create)(USHORT index);
    void (CDECL *destroy)(struct video_capture_device *device);
    HRESULT (CDECL *check_format)(struct video_capture_device *device, const AM_MEDIA_TYPE *mt);
    HRESULT (CDECL *set_format)(struct video_capture_device *device, const AM_MEDIA_TYPE *mt);
    void (CDECL *get_format)(struct video_capture_device *device, AM_MEDIA_TYPE *mt, VIDEOINFOHEADER *format);
    HRESULT (CDECL *get_media_type)(struct video_capture_device *device,
            unsigned int index, AM_MEDIA_TYPE *mt, VIDEOINFOHEADER *format);
    void (CDECL *get_caps)(struct video_capture_device *device, LONG index, AM_MEDIA_TYPE *mt,
            VIDEOINFOHEADER *format, VIDEO_STREAM_CONFIG_CAPS *caps);
    LONG (CDECL *get_caps_count)(struct video_capture_device *device);
    HRESULT (CDECL *get_prop_range)(struct video_capture_device *device, VideoProcAmpProperty property,
            LONG *min, LONG *max, LONG *step, LONG *default_value, LONG *flags);
    HRESULT (CDECL *get_prop)(struct video_capture_device *device,
            VideoProcAmpProperty property, LONG *value, LONG *flags);
    HRESULT (CDECL *set_prop)(struct video_capture_device *device,
            VideoProcAmpProperty property, LONG value, LONG flags);
    BOOL (CDECL *read_frame)(struct video_capture_device *device, BYTE *data);
};

#endif
