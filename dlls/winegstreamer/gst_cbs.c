/*
 * Copyright 2015 Andrew Eikum for CodeWeavers
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

#include "config.h"

#include <gst/gst.h>

#include "objbase.h"

#include "wine/list.h"

#include "gst_cbs.h"

static pthread_key_t wine_gst_key;

void mark_wine_thread(void)
{
    /* set it to non-NULL to indicate that this is a Wine thread */
    pthread_setspecific(wine_gst_key, &wine_gst_key);
}

static BOOL is_wine_thread(void)
{
    return pthread_getspecific(wine_gst_key) != NULL;
}

static pthread_mutex_t cb_list_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cb_list_cond = PTHREAD_COND_INITIALIZER;
static struct list cb_list = LIST_INIT(cb_list);

static void CALLBACK perform_cb(TP_CALLBACK_INSTANCE *instance, void *user)
{
    struct cb_data *cbdata = user;

    if (cbdata->type < GSTDEMUX_MAX)
        perform_cb_gstdemux(cbdata);
    else if (cbdata->type < MEDIA_SOURCE_MAX)
        perform_cb_media_source(cbdata);

    pthread_mutex_lock(&cbdata->lock);
    cbdata->finished = 1;
    pthread_cond_broadcast(&cbdata->cond);
    pthread_mutex_unlock(&cbdata->lock);
}

static DWORD WINAPI dispatch_thread(void *user)
{
    struct cb_data *cbdata;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    pthread_mutex_lock(&cb_list_lock);

    while (1)
    {
        while (list_empty(&cb_list)) pthread_cond_wait(&cb_list_cond, &cb_list_lock);

        cbdata = LIST_ENTRY(list_head(&cb_list), struct cb_data, entry);
        list_remove(&cbdata->entry);
        TrySubmitThreadpoolCallback(&perform_cb, cbdata, NULL);
    }

    pthread_mutex_unlock(&cb_list_lock);

    CoUninitialize();

    return 0;
}

void start_dispatch_thread(void)
{
    pthread_key_create(&wine_gst_key, NULL);
    CloseHandle(CreateThread(NULL, 0, &dispatch_thread, NULL, 0, NULL));
}

/* gstreamer calls our callbacks from threads that Wine did not create. Some
 * callbacks execute code which requires Wine to have created the thread
 * (critical sections, debug logging, dshow client code). Since gstreamer can't
 * provide an API to override its thread creation, we have to intercept all
 * callbacks in code which avoids the Wine thread requirement, and then
 * dispatch those callbacks on a thread that is known to be created by Wine.
 *
 * This thread must not run any code that depends on the Wine TEB!
 */

static void call_cb(struct cb_data *cbdata)
{
    pthread_mutex_init(&cbdata->lock, NULL);
    pthread_cond_init(&cbdata->cond, NULL);
    cbdata->finished = 0;

    if(is_wine_thread()){
        /* The thread which triggered gstreamer to call this callback may
         * already hold a critical section. If so, executing the callback on a
         * worker thread can cause a deadlock. If  we are already on a Wine
         * thread, then there is no need to run this callback on a worker
         * thread anyway, which avoids the deadlock issue. */
        perform_cb(NULL, cbdata);

        pthread_cond_destroy(&cbdata->cond);
        pthread_mutex_destroy(&cbdata->lock);

        return;
    }

    pthread_mutex_lock(&cb_list_lock);

    list_add_tail(&cb_list, &cbdata->entry);
    pthread_cond_broadcast(&cb_list_cond);

    pthread_mutex_lock(&cbdata->lock);

    pthread_mutex_unlock(&cb_list_lock);

    while(!cbdata->finished)
        pthread_cond_wait(&cbdata->cond, &cbdata->lock);

    pthread_mutex_unlock(&cbdata->lock);

    pthread_cond_destroy(&cbdata->cond);
    pthread_mutex_destroy(&cbdata->lock);
}

GstBusSyncReply watch_bus_wrapper(GstBus *bus, GstMessage *msg, gpointer user)
{
    struct cb_data cbdata = { WATCH_BUS };

    cbdata.u.watch_bus_data.bus = bus;
    cbdata.u.watch_bus_data.msg = msg;
    cbdata.u.watch_bus_data.user = user;

    call_cb(&cbdata);

    return cbdata.u.watch_bus_data.ret;
}

void existing_new_pad_wrapper(GstElement *bin, GstPad *pad, gpointer user)
{
    struct cb_data cbdata = { EXISTING_NEW_PAD };

    cbdata.u.pad_added_data.element = bin;
    cbdata.u.pad_added_data.pad = pad;
    cbdata.u.pad_added_data.user = user;

    call_cb(&cbdata);
}

gboolean query_function_wrapper(GstPad *pad, GstObject *parent, GstQuery *query)
{
    struct cb_data cbdata = { QUERY_FUNCTION };

    cbdata.u.query_function_data.pad = pad;
    cbdata.u.query_function_data.parent = parent;
    cbdata.u.query_function_data.query = query;

    call_cb(&cbdata);

    return cbdata.u.query_function_data.ret;
}

gboolean activate_mode_wrapper(GstPad *pad, GstObject *parent, GstPadMode mode, gboolean activate)
{
    struct cb_data cbdata = { ACTIVATE_MODE };

    cbdata.u.activate_mode_data.pad = pad;
    cbdata.u.activate_mode_data.parent = parent;
    cbdata.u.activate_mode_data.mode = mode;
    cbdata.u.activate_mode_data.activate = activate;

    call_cb(&cbdata);

    return cbdata.u.activate_mode_data.ret;
}

void no_more_pads_wrapper(GstElement *element, gpointer user)
{
    struct cb_data cbdata = { NO_MORE_PADS };

    cbdata.u.no_more_pads_data.element = element;
    cbdata.u.no_more_pads_data.user = user;

    call_cb(&cbdata);
}

GstFlowReturn request_buffer_src_wrapper(GstPad *pad, GstObject *parent, guint64 ofs, guint len,
        GstBuffer **buf)
{
    struct cb_data cbdata = { REQUEST_BUFFER_SRC };

    cbdata.u.getrange_data.pad = pad;
    cbdata.u.getrange_data.parent = parent;
    cbdata.u.getrange_data.ofs = ofs;
    cbdata.u.getrange_data.len = len;
    cbdata.u.getrange_data.buf = buf;

    call_cb(&cbdata);

    return cbdata.u.getrange_data.ret;
}

gboolean event_src_wrapper(GstPad *pad, GstObject *parent, GstEvent *event)
{
    struct cb_data cbdata = { EVENT_SRC };

    cbdata.u.event_src_data.pad = pad;
    cbdata.u.event_src_data.parent = parent;
    cbdata.u.event_src_data.event = event;

    call_cb(&cbdata);

    return cbdata.u.event_src_data.ret;
}

gboolean event_sink_wrapper(GstPad *pad, GstObject *parent, GstEvent *event)
{
    struct cb_data cbdata = { EVENT_SINK };

    cbdata.u.event_sink_data.pad = pad;
    cbdata.u.event_sink_data.parent = parent;
    cbdata.u.event_sink_data.event = event;

    call_cb(&cbdata);

    return cbdata.u.event_sink_data.ret;
}

GstFlowReturn got_data_sink_wrapper(GstPad *pad, GstObject *parent, GstBuffer *buf)
{
    struct cb_data cbdata = { GOT_DATA_SINK };

    cbdata.u.got_data_sink_data.pad = pad;
    cbdata.u.got_data_sink_data.parent = parent;
    cbdata.u.got_data_sink_data.buf = buf;

    call_cb(&cbdata);

    return cbdata.u.got_data_sink_data.ret;
}

void removed_decoded_pad_wrapper(GstElement *bin, GstPad *pad, gpointer user)
{
    struct cb_data cbdata = { REMOVED_DECODED_PAD };

    cbdata.u.pad_removed_data.element = bin;
    cbdata.u.pad_removed_data.pad = pad;
    cbdata.u.pad_removed_data.user = user;

    call_cb(&cbdata);
}

GstAutoplugSelectResult autoplug_blacklist_wrapper(GstElement *bin, GstPad *pad,
        GstCaps *caps, GstElementFactory *fact, gpointer user)
{
    struct cb_data cbdata = { AUTOPLUG_BLACKLIST };

    cbdata.u.autoplug_blacklist_data.bin = bin;
    cbdata.u.autoplug_blacklist_data.pad = pad;
    cbdata.u.autoplug_blacklist_data.caps = caps;
    cbdata.u.autoplug_blacklist_data.fact = fact;
    cbdata.u.autoplug_blacklist_data.user = user;

    call_cb(&cbdata);

    return cbdata.u.autoplug_blacklist_data.ret;
}

void unknown_type_wrapper(GstElement *bin, GstPad *pad, GstCaps *caps, gpointer user)
{
    struct cb_data cbdata = { UNKNOWN_TYPE };

    cbdata.u.unknown_type_data.bin = bin;
    cbdata.u.unknown_type_data.pad = pad;
    cbdata.u.unknown_type_data.caps = caps;
    cbdata.u.unknown_type_data.user = user;

    call_cb(&cbdata);
}

gboolean query_sink_wrapper(GstPad *pad, GstObject *parent, GstQuery *query)
{
    struct cb_data cbdata = { QUERY_SINK };

    cbdata.u.query_sink_data.pad = pad;
    cbdata.u.query_sink_data.parent = parent;
    cbdata.u.query_sink_data.query = query;

    call_cb(&cbdata);

    return cbdata.u.query_sink_data.ret;
}

GstFlowReturn bytestream_wrapper_pull_wrapper(GstPad *pad, GstObject *parent, guint64 ofs, guint len,
        GstBuffer **buf)
{
    struct cb_data cbdata = { BYTESTREAM_WRAPPER_PULL };

    cbdata.u.getrange_data.pad = pad;
    cbdata.u.getrange_data.parent = parent;
    cbdata.u.getrange_data.ofs = ofs;
    cbdata.u.getrange_data.len = len;
    cbdata.u.getrange_data.buf = buf;

    call_cb(&cbdata);

    return cbdata.u.getrange_data.ret;
}

gboolean bytestream_query_wrapper(GstPad *pad, GstObject *parent, GstQuery *query)
{
    struct cb_data cbdata = { BYTESTREAM_QUERY };

    cbdata.u.query_function_data.pad = pad;
    cbdata.u.query_function_data.parent = parent;
    cbdata.u.query_function_data.query = query;

    call_cb(&cbdata);

    return cbdata.u.query_function_data.ret;
}

gboolean bytestream_pad_mode_activate_wrapper(GstPad *pad, GstObject *parent, GstPadMode mode, gboolean activate)
{
    struct cb_data cbdata = { BYTESTREAM_PAD_MODE_ACTIVATE };

    cbdata.u.activate_mode_data.pad = pad;
    cbdata.u.activate_mode_data.parent = parent;
    cbdata.u.activate_mode_data.mode = mode;
    cbdata.u.activate_mode_data.activate = activate;

    call_cb(&cbdata);

    return cbdata.u.activate_mode_data.ret;
}

gboolean bytestream_pad_event_process_wrapper(GstPad *pad, GstObject *parent, GstEvent *event)
{
    struct cb_data cbdata = { BYTESTREAM_PAD_EVENT_PROCESS };

    cbdata.u.event_src_data.pad = pad;
    cbdata.u.event_src_data.parent = parent;
    cbdata.u.event_src_data.event = event;

    call_cb(&cbdata);

    return cbdata.u.event_src_data.ret;
}

GstBusSyncReply mf_src_bus_watch_wrapper(GstBus *bus, GstMessage *message, gpointer user)
{
    struct cb_data cbdata = { MF_SRC_BUS_WATCH };

    cbdata.u.watch_bus_data.bus = bus;
    cbdata.u.watch_bus_data.msg = message;
    cbdata.u.watch_bus_data.user = user;

    call_cb(&cbdata);

    return cbdata.u.watch_bus_data.ret;
}

void mf_src_stream_added_wrapper(GstElement *bin, GstPad *pad, gpointer user)
{
    struct cb_data cbdata = { MF_SRC_STREAM_ADDED };

    cbdata.u.pad_added_data.element = bin;
    cbdata.u.pad_added_data.pad = pad;
    cbdata.u.pad_added_data.user = user;

    call_cb(&cbdata);
}

void mf_src_stream_removed_wrapper(GstElement *element, GstPad *pad, gpointer user)
{
    struct cb_data cbdata = { MF_SRC_STREAM_REMOVED };

    cbdata.u.pad_removed_data.element = element;
    cbdata.u.pad_removed_data.pad = pad;
    cbdata.u.pad_removed_data.user = user;

    call_cb(&cbdata);
}

void mf_src_no_more_pads_wrapper(GstElement *element, gpointer user)
{
    struct cb_data cbdata = { MF_SRC_NO_MORE_PADS };

    cbdata.u.no_more_pads_data.element = element;
    cbdata.u.no_more_pads_data.user = user;

    call_cb(&cbdata);
}
