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

#include "config.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
#include "wine/heap.h"

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

    if (!(context = heap_alloc(sizeof(*context))))
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

    heap_free(context);
    return S_OK;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, void *reserved)
{
    TRACE("(%p, %d, %p)\n", inst, reason, reserved);

    switch (reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;    /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(inst);
        break;
    }
    return TRUE;
}
