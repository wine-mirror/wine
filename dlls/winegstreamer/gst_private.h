/*
 * GStreamer splitter + decoder, adapted from parser.c
 *
 * Copyright 2010 Maarten Lankhorst for CodeWeavers
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

#ifndef __GST_PRIVATE_INCLUDED__
#define __GST_PRIVATE_INCLUDED__

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "winuser.h"
#include "dshow.h"
#include "strmif.h"
#include "wine/strmbase.h"

#define MEDIATIME_FROM_BYTES(x) ((LONGLONG)(x) * 10000000)

/* enum media */
void dump_AM_MEDIA_TYPE(const AM_MEDIA_TYPE * pmt);

IUnknown * CALLBACK Gstreamer_AudioConvert_create(IUnknown *pUnkOuter, HRESULT *phr);
IUnknown * CALLBACK Gstreamer_Mp3_create(IUnknown *pUnkOuter, HRESULT *phr);
IUnknown * CALLBACK Gstreamer_YUV2RGB_create(IUnknown *pUnkOuter, HRESULT *phr);
IUnknown * CALLBACK Gstreamer_YUV2ARGB_create(IUnknown *pUnkOuter, HRESULT *phr);
IUnknown * CALLBACK Gstreamer_Splitter_create(IUnknown *pUnkOuter, HRESULT *phr);

BOOL init_gstreamer(void) DECLSPEC_HIDDEN;

GstFlowReturn got_data(GstPad *pad, GstObject *parent, GstBuffer *buf) DECLSPEC_HIDDEN;
GstFlowReturn request_buffer(GstPad *pad, guint64 ofs, guint size, GstCaps *caps, GstBuffer **buf) DECLSPEC_HIDDEN;
void Gstreamer_transform_pad_added(GstElement *filter, GstPad *pad, gpointer user) DECLSPEC_HIDDEN;

void start_dispatch_thread(void) DECLSPEC_HIDDEN;

extern const char *media_quark_string DECLSPEC_HIDDEN;

extern HRESULT mfplat_get_class_object(REFCLSID rclsid, REFIID riid, void **obj) DECLSPEC_HIDDEN;
extern HRESULT mfplat_can_unload_now(void) DECLSPEC_HIDDEN;

#endif /* __GST_PRIVATE_INCLUDED__ */
