/*
 * Helpers used for WinRT event dispatch.
 *
 * Copyright 2025 Vibhav Pant
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
#include "eventtoken.h"
#include "inspectable.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vccorlib);

void *__cdecl Delegate_ctor(void *this)
{
    FIXME("(%p): stub!\n", this);
    return NULL;
}

void WINAPI EventSourceInitialize(IInspectable **source)
{
    FIXME("(%p): stub!\n", source);
}

void WINAPI EventSourceUninitialize(IInspectable **source)
{
    FIXME("(%p): stub!\n", source);
}

EventRegistrationToken WINAPI EventSourceAdd(IInspectable **source, void *lock, IUnknown *delegate)
{
    EventRegistrationToken token = {0};
    FIXME("(%p, %p, %p): stub!\n", source, lock, delegate);
    return token;
}

void WINAPI EventSourceRemove(IInspectable **source, void *lock, EventRegistrationToken token)
{
    FIXME("(%p, %p, {%I64u}): stub!\n", source, lock, token.value);
}

IInspectable *WINAPI EventSourceGetTargetArray(IInspectable *source, void *lock)
{
    FIXME("(%p, %p): stub!\n", source, lock);
    return NULL;
}

void *WINAPI EventSourceGetTargetArrayEvent(void *source, ULONG idx, const GUID *iid, EventRegistrationToken *token)
{
    FIXME("(%p, %lu, %s, %p): stub!\n", source, idx, debugstr_guid(iid), token);
    return NULL;
}

ULONG WINAPI EventSourceGetTargetArraySize(void *source)
{
    FIXME("(%p): stub!\n", source);
    return 0;
}
