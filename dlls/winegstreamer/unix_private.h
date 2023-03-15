/*
 * winegstreamer Unix library interface
 *
 * Copyright 2020-2021 Zebediah Figura for CodeWeavers
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

#ifndef __WINE_WINEGSTREAMER_UNIX_PRIVATE_H
#define __WINE_WINEGSTREAMER_UNIX_PRIVATE_H

#include "unixlib.h"

#include <gst/gst.h>

/* unixlib.c */

GST_DEBUG_CATEGORY_EXTERN(wine) DECLSPEC_HIDDEN;
#define GST_CAT_DEFAULT wine

extern NTSTATUS wg_init_gstreamer(void *args) DECLSPEC_HIDDEN;

extern GstStreamType stream_type_from_caps(GstCaps *caps) DECLSPEC_HIDDEN;
extern GstElement *create_element(const char *name, const char *plugin_set) DECLSPEC_HIDDEN;
extern GstElement *find_element(GstElementFactoryListType type, GstCaps *src_caps, GstCaps *sink_caps) DECLSPEC_HIDDEN;
extern bool append_element(GstElement *container, GstElement *element, GstElement **first, GstElement **last) DECLSPEC_HIDDEN;
extern bool link_src_to_element(GstPad *src_pad, GstElement *element) DECLSPEC_HIDDEN;
extern bool link_element_to_sink(GstElement *element, GstPad *sink_pad) DECLSPEC_HIDDEN;

/* wg_format.c */

extern void wg_format_from_caps(struct wg_format *format, const GstCaps *caps) DECLSPEC_HIDDEN;
extern bool wg_format_compare(const struct wg_format *a, const struct wg_format *b) DECLSPEC_HIDDEN;
extern GstCaps *wg_format_to_caps(const struct wg_format *format) DECLSPEC_HIDDEN;

/* wg_transform.c */

extern NTSTATUS wg_transform_create(void *args) DECLSPEC_HIDDEN;
extern NTSTATUS wg_transform_destroy(void *args) DECLSPEC_HIDDEN;
extern NTSTATUS wg_transform_set_output_format(void *args) DECLSPEC_HIDDEN;
extern NTSTATUS wg_transform_push_data(void *args) DECLSPEC_HIDDEN;
extern NTSTATUS wg_transform_read_data(void *args) DECLSPEC_HIDDEN;
extern NTSTATUS wg_transform_get_status(void *args) DECLSPEC_HIDDEN;

/* wg_allocator.c */

/* wg_allocator_release_sample can be used to release any sample that was requested. */
typedef struct wg_sample *(*wg_allocator_request_sample_cb)(gsize size, void *context);
extern GstAllocator *wg_allocator_create(wg_allocator_request_sample_cb request_sample,
        void *request_sample_context) DECLSPEC_HIDDEN;
extern void wg_allocator_destroy(GstAllocator *allocator) DECLSPEC_HIDDEN;
extern void wg_allocator_release_sample(GstAllocator *allocator, struct wg_sample *sample,
        bool discard_data) DECLSPEC_HIDDEN;

#endif /* __WINE_WINEGSTREAMER_UNIX_PRIVATE_H */
