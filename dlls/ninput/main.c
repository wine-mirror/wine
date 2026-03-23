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
    BOOL ui_feedback;
    INTERACTION_CONFIGURATION_FLAGS config_flags[INTERACTION_ID_CROSS_SLIDE];
};

static struct interaction_context *context_from_handle(HINTERACTIONCONTEXT handle)
{
    return (struct interaction_context *)handle;
}

HRESULT WINAPI BufferPointerPacketsInteractionContext(HINTERACTIONCONTEXT context,
        UINT32 entries_count, const POINTER_INFO *pointer_info)
{
    FIXME("context %p, entries_count %u, pointer_info %p: stub!\n", context, entries_count, pointer_info);

    if (!context)
        return E_HANDLE;
    if (!entries_count)
        return E_INVALIDARG;
    if (!pointer_info)
        return E_POINTER;

    return S_OK;
}

HRESULT WINAPI CreateInteractionContext(HINTERACTIONCONTEXT *handle)
{
    struct interaction_context *context;

    TRACE("handle %p.\n", handle);

    if (!handle)
        return E_POINTER;

    if (!(context = calloc(1, sizeof(*context))))
        return E_OUTOFMEMORY;

    context->filter_pointers = TRUE;
    context->ui_feedback = TRUE;

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
            FIXME("Unhandled property %#x.\n", property);
            *value = 0;
            return E_NOTIMPL;

        case INTERACTION_CONTEXT_PROPERTY_INTERACTION_UI_FEEDBACK:
            *value = context->ui_feedback;
            return S_OK;

        case INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS:
            *value = context->filter_pointers;
            return S_OK;

        default:
            WARN("Invalid property %#x.\n", property);
            return E_INVALIDARG;
    }
}

HRESULT WINAPI GetStateInteractionContext(HINTERACTIONCONTEXT context,
        const POINTER_INFO *pointer_info, INTERACTION_STATE *state)
{
    FIXME("context %p, pointer_info %p, state %p: stub!.\n", context, pointer_info, state);

    if (!context)
        return E_HANDLE;
    if (!state)
        return E_POINTER;

    *state = INTERACTION_STATE_IDLE;
    return S_OK;
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
            FIXME("Unhandled property %#x.\n", property);
            return E_NOTIMPL;

        case INTERACTION_CONTEXT_PROPERTY_INTERACTION_UI_FEEDBACK:
            if (value != FALSE && value != TRUE)
                return E_INVALIDARG;
            context->ui_feedback = value;
            return S_OK;

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

    FIXME("context %p, count %u, configuration %p: semi-stub!.\n", context, count, configuration);

    if (!context)
        return E_HANDLE;
    if (!count)
        return E_INVALIDARG;
    if (!configuration)
        return E_POINTER;

    for (unsigned int i = 0; i < count; i++)
    {
        if (configuration[i].interactionId == INTERACTION_ID_NONE ||
                configuration[i].interactionId > INTERACTION_ID_CROSS_SLIDE)
            return E_INVALIDARG;
    }

    for (unsigned int i = 0; i < count; i++)
    {
        unsigned int index = configuration[i].interactionId - 1;
        context->config_flags[index] = configuration[i].enable;
    }

    return S_OK;
}

HRESULT WINAPI GetInteractionConfigurationInteractionContext(HINTERACTIONCONTEXT handle,
        UINT32 count, INTERACTION_CONTEXT_CONFIGURATION *configuration)
{
    struct interaction_context *context = context_from_handle(handle);

    FIXME("context %p, count %u, configuration %p: semi-stub!\n", context, count, configuration);

    if (!context)
        return E_HANDLE;
    if (!count)
        return S_OK;
    if (!configuration)
        return E_POINTER;

    for (unsigned int i = 0; i < count; i++)
    {
        if (configuration[i].interactionId == INTERACTION_ID_NONE ||
                configuration[i].interactionId > INTERACTION_ID_CROSS_SLIDE)
            return E_INVALIDARG;
    }

    for (unsigned int i = 0; i < count; i++)
    {
        unsigned int index = configuration[i].interactionId - 1;
        configuration[i].enable = context->config_flags[index];
    }

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

HRESULT WINAPI ProcessBufferedPacketsInteractionContext(HINTERACTIONCONTEXT context)
{
    FIXME("context %p: stub!\n", context);

    if (!context)
        return E_HANDLE;

    return S_OK;
}

HRESULT WINAPI ProcessInertiaInteractionContext(HINTERACTIONCONTEXT context)
{
    FIXME("context %p: stub!\n", context);
    return E_NOTIMPL;
}

/* Undocumented function at ordinal 2502 */
HRESULT WINAPI NINPUT_2502(HINTERACTIONCONTEXT *context, void *p1, void *p2)
{
    FIXME("context %p p1 %p p2 %p: stub!\n", context, p1, p2);

    if (!context)
        return E_HANDLE;

    return S_OK;
}
