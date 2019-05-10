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

#define COBJMACROS

#include "mfidl.h"
#include "mf_private.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct sample_grabber_activate_context
{
    IMFMediaType *media_type;
    IMFSampleGrabberSinkCallback *callback;
};

static void sample_grabber_free_private(void *user_context)
{
    struct sample_grabber_activate_context *context = user_context;
    IMFMediaType_Release(context->media_type);
    IMFSampleGrabberSinkCallback_Release(context->callback);
    heap_free(context);
}

static HRESULT sample_grabber_create_object(IMFAttributes *attributes, void *user_context, IUnknown **obj)
{
    FIXME("%p, %p, %p.\n", attributes, user_context, obj);

    return E_NOTIMPL;
}

static const struct activate_funcs sample_grabber_activate_funcs =
{
    sample_grabber_create_object,
    sample_grabber_free_private,
};

/***********************************************************************
 *      MFCreateSampleGrabberSinkActivate (mf.@)
 */
HRESULT WINAPI MFCreateSampleGrabberSinkActivate(IMFMediaType *media_type, IMFSampleGrabberSinkCallback *callback,
        IMFActivate **activate)
{
    struct sample_grabber_activate_context *context;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", media_type, callback, activate);

    if (!media_type || !callback || !activate)
        return E_POINTER;

    context = heap_alloc_zero(sizeof(*context));
    if (!context)
        return E_OUTOFMEMORY;

    context->media_type = media_type;
    IMFMediaType_AddRef(context->media_type);
    context->callback = callback;
    IMFSampleGrabberSinkCallback_AddRef(context->callback);

    if (FAILED(hr = create_activation_object(context, &sample_grabber_activate_funcs, activate)))
        sample_grabber_free_private(context);

    return hr;
}
