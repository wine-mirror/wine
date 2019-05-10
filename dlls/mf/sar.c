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

static HRESULT sar_create_object(IMFAttributes *attributes, void *user_context, IUnknown **obj)
{
    FIXME("%p, %p, %p.\n", attributes, user_context, obj);

    return E_NOTIMPL;
}

static void sar_free_private(void *user_context)
{
}

static const struct activate_funcs sar_activate_funcs =
{
    sar_create_object,
    sar_free_private,
};

/***********************************************************************
 *      MFCreateAudioRendererActivate (mf.@)
 */
HRESULT WINAPI MFCreateAudioRendererActivate(IMFActivate **activate)
{
    TRACE("%p.\n", activate);

    if (!activate)
        return E_POINTER;

    return create_activation_object(NULL, &sar_activate_funcs, activate);
}

/***********************************************************************
 *      MFCreateAudioRenderer (mf.@)
 */
HRESULT WINAPI MFCreateAudioRenderer(IMFAttributes *attributes, IMFMediaSink **sink)
{
    IUnknown *object;
    HRESULT hr;

    TRACE("%p, %p.\n", attributes, sink);

    if (SUCCEEDED(hr = sar_create_object(attributes, NULL, &object)))
    {
        hr = IUnknown_QueryInterface(object, &IID_IMFMediaSink, (void **)sink);
        IUnknown_Release(object);
    }

    return hr;
}
