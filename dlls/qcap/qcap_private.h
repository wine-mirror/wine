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
#include "wine/heap.h"
#include "wine/strmbase.h"
#include "wine/unicode.h"

extern DWORD ObjectRefCount(BOOL increment) DECLSPEC_HIDDEN;

HRESULT audio_record_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT avi_compressor_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT avi_mux_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT capture_graph_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT file_writer_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT smart_tee_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT vfw_capture_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;

typedef struct _Capture Capture;

Capture *qcap_driver_init(struct strmbase_source *pin, USHORT card) DECLSPEC_HIDDEN;
HRESULT qcap_driver_destroy(Capture *device) DECLSPEC_HIDDEN;
HRESULT qcap_driver_check_format(Capture *device, const AM_MEDIA_TYPE *mt) DECLSPEC_HIDDEN;
HRESULT qcap_driver_set_format(Capture *device, AM_MEDIA_TYPE *mt) DECLSPEC_HIDDEN;
HRESULT qcap_driver_get_caps(Capture *device, LONG index, AM_MEDIA_TYPE **mt,
        VIDEO_STREAM_CONFIG_CAPS *vscc) DECLSPEC_HIDDEN;
LONG qcap_driver_get_caps_count(Capture *device) DECLSPEC_HIDDEN;
HRESULT qcap_driver_get_format(const Capture *device, AM_MEDIA_TYPE **mt) DECLSPEC_HIDDEN;
HRESULT qcap_driver_get_prop_range(Capture *device, VideoProcAmpProperty property,
        LONG *min, LONG *max, LONG *step, LONG *default_value, LONG *flags) DECLSPEC_HIDDEN;
HRESULT qcap_driver_get_prop(Capture *device, VideoProcAmpProperty property, LONG *value, LONG *flags) DECLSPEC_HIDDEN;
HRESULT qcap_driver_set_prop(Capture *device, VideoProcAmpProperty property, LONG value, LONG flags) DECLSPEC_HIDDEN;
void qcap_driver_init_stream(Capture *device) DECLSPEC_HIDDEN;
void qcap_driver_start_stream(Capture *device) DECLSPEC_HIDDEN;
void qcap_driver_stop_stream(Capture *device) DECLSPEC_HIDDEN;
void qcap_driver_cleanup_stream(Capture *device) DECLSPEC_HIDDEN;

#endif
