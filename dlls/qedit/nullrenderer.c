/*
 * Null renderer filter
 *
 * Copyright 2004 Christian Costa
 * Copyright 2008 Maarten Lankhorst
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
#include "dshow.h"
#include "wine/debug.h"
#include "wine/strmbase.h"

WINE_DEFAULT_DEBUG_CHANNEL(qedit);

struct null_renderer
{
    struct strmbase_renderer renderer;
    HANDLE run_event;
};

static struct null_renderer *impl_from_strmbase_renderer(struct strmbase_renderer *iface)
{
    return CONTAINING_RECORD(iface, struct null_renderer, renderer);
}

static HRESULT WINAPI NullRenderer_DoRenderSample(struct strmbase_renderer *iface, IMediaSample *sample)
{
    struct null_renderer *filter = impl_from_strmbase_renderer(iface);

    if (filter->renderer.filter.state == State_Paused)
    {
        const HANDLE events[2] = {filter->run_event, filter->renderer.flush_event};

        SetEvent(filter->renderer.state_event);
        LeaveCriticalSection(&filter->renderer.csRenderLock);
        WaitForMultipleObjects(2, events, FALSE, INFINITE);
        EnterCriticalSection(&filter->renderer.csRenderLock);
    }

    return S_OK;
}

static HRESULT WINAPI NullRenderer_CheckMediaType(struct strmbase_renderer *iface, const AM_MEDIA_TYPE *mt)
{
    TRACE("Not a stub!\n");
    return S_OK;
}

static void null_renderer_destroy(struct strmbase_renderer *iface)
{
    struct null_renderer *filter = impl_from_strmbase_renderer(iface);

    CloseHandle(filter->run_event);
    strmbase_renderer_cleanup(&filter->renderer);
    free(filter);
}

static void null_renderer_start_stream(struct strmbase_renderer *iface)
{
    struct null_renderer *filter = impl_from_strmbase_renderer(iface);
    SetEvent(filter->run_event);
}

static void null_renderer_stop_stream(struct strmbase_renderer *iface)
{
    struct null_renderer *filter = impl_from_strmbase_renderer(iface);
    ResetEvent(filter->run_event);
}

static const struct strmbase_renderer_ops renderer_ops =
{
    .pfnCheckMediaType = NullRenderer_CheckMediaType,
    .pfnDoRenderSample = NullRenderer_DoRenderSample,
    .renderer_start_stream = null_renderer_start_stream,
    .renderer_stop_stream = null_renderer_stop_stream,
    .renderer_destroy = null_renderer_destroy,
};

HRESULT null_renderer_create(IUnknown *outer, IUnknown **out)
{
    struct null_renderer *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_renderer_init(&object->renderer, outer, &CLSID_NullRenderer, L"In", &renderer_ops);
    object->run_event = CreateEventW(NULL, TRUE, FALSE, NULL);

    TRACE("Created null renderer %p.\n", object);
    *out = &object->renderer.filter.IUnknown_inner;
    return S_OK;
}
