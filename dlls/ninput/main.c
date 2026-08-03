/*
 * Copyright 2018 Andrey Gusev
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

#include "interactioncontext.h"

WINE_DEFAULT_DEBUG_CHANNEL(ninput);

struct interaction_context
{
    BOOL filter_pointers;
};

static struct interaction_context *context_from_handle(HINTERACTIONCONTEXT handle)
{
    return (struct interaction_context *)handle;
}

HRESULT WINAPI CreateInteractionContext(HINTERACTIONCONTEXT *handle)
{
    struct interaction_context *context;

    TRACE("handle %p.\n", handle);

    if (!handle)
        return E_POINTER;

    if (!(context = malloc(sizeof(*context))))
        return E_OUTOFMEMORY;

    context->filter_pointers = TRUE;

    TRACE("Created context %p.\n", context);

    *handle = (HINTERACTIONCONTEXT)context;

    return S_OK;
}

HRESULT WINAPI DestroyInteractionContext(HINTERACTIONCONTEXT handle)
{
    struct interaction_context *context = context_from_handle(handle);

    TRACE("context %p.\n", context);

    if (!context)
        return E_HANDLE;

    free(context);
    return S_OK;
}

HRESULT WINAPI GetPropertyInteractionContext(HINTERACTIONCONTEXT handle,
        INTERACTION_CONTEXT_PROPERTY property, UINT32 *value)
{
    struct interaction_context *context = context_from_handle(handle);

    TRACE("context %p, property %#x, value %p.\n", context, property, value);

    if (!context)
        return E_HANDLE;
    if (!value)
        return E_POINTER;

    switch (property)
    {
        case INTERACTION_CONTEXT_PROPERTY_MEASUREMENT_UNITS:
        case INTERACTION_CONTEXT_PROPERTY_INTERACTION_UI_FEEDBACK:
            FIXME("Unhandled property %#x.\n", property);
            *value = 0;
            return E_NOTIMPL;

        case INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS:
            *value = context->filter_pointers;
            return S_OK;

        default:
            WARN("Invalid property %#x.\n", property);
            return E_INVALIDARG;
    }
}

HRESULT WINAPI SetPropertyInteractionContext(HINTERACTIONCONTEXT handle,
        INTERACTION_CONTEXT_PROPERTY property, UINT32 value)
{
    struct interaction_context *context = context_from_handle(handle);

    TRACE("context %p, property %#x, value %#x.\n", context, property, value);

    if (!context)
        return E_HANDLE;

    switch (property)
    {
        case INTERACTION_CONTEXT_PROPERTY_MEASUREMENT_UNITS:
        case INTERACTION_CONTEXT_PROPERTY_INTERACTION_UI_FEEDBACK:
            FIXME("Unhandled property %#x.\n", property);
            return E_NOTIMPL;

        case INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS:
            if (value != FALSE && value != TRUE)
                return E_INVALIDARG;
            context->filter_pointers = value;
            return S_OK;

        default:
            WARN("Invalid property %#x.\n", property);
            return E_INVALIDARG;
    }
}

HRESULT WINAPI SetInteractionConfigurationInteractionContext(HINTERACTIONCONTEXT handle,
        UINT32 count, const INTERACTION_CONTEXT_CONFIGURATION *configuration)
{
    struct interaction_context *context = context_from_handle(handle);

    FIXME("context %p, count %u, configuration %p: stub!.\n", context, count, configuration);

    if (!context)
        return E_HANDLE;
    if (!count)
        return E_INVALIDARG;
    if (!configuration)
        return E_POINTER;

    return S_OK;
}

HRESULT WINAPI RegisterOutputCallbackInteractionContext(HINTERACTIONCONTEXT handle,
        INTERACTION_CONTEXT_OUTPUT_CALLBACK callback, void *data)
{
    struct interaction_context *context = context_from_handle(handle);

    FIXME("context %p, callback %p, data %p: stub!.\n", context, callback, data);

    if (!context)
        return E_HANDLE;

    return S_OK;
}

HRESULT WINAPI ProcessInertiaInteractionContext(HINTERACTIONCONTEXT context)
{
    FIXME("context %p: stub!\n", context);
    return E_NOTIMPL;
}
