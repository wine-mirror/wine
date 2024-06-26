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

#include "mfmediaengine.h"

struct video_frame_sink;

HRESULT create_video_frame_sink(IMFMediaType *media_type, IUnknown *device_manager, IMFAsyncCallback *events_callback,
    struct video_frame_sink **sink);
HRESULT video_frame_sink_query_iface(struct video_frame_sink *object, REFIID riid, void **obj);
ULONG video_frame_sink_release(struct video_frame_sink *sink);
int video_frame_sink_get_sample(struct video_frame_sink *sink, IMFSample **sample);
HRESULT video_frame_sink_get_pts(struct video_frame_sink *sink, MFTIME clocktime, LONGLONG *pts);
void video_frame_sink_notify_end_of_presentation_segment(struct video_frame_sink *sink);
