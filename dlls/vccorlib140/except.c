/*
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

#include "roapi.h"
#include "winstring.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vccorlib);

void *__cdecl CreateExceptionWithMessage(HRESULT hr, HSTRING msg)
{
    FIXME("(%#lx, %s): stub!\n", hr, debugstr_hstring(msg));
    return NULL;
}

void *__cdecl CreateException(HRESULT hr)
{
    FIXME("(%#lx): stub!\n", hr);
    return NULL;
}

void WINAPI __abi_WinRTraiseCOMException(HRESULT hr)
{
    FIXME("(%#lx): stub!\n", hr);
}

#define WINRT_EXCEPTIONS                                     \
    WINRT_EXCEPTION(AccessDenied, E_ACCESSDENIED)            \
    WINRT_EXCEPTION(ChangedState, E_CHANGED_STATE)           \
    WINRT_EXCEPTION(ClassNotRegistered, REGDB_E_CLASSNOTREG) \
    WINRT_EXCEPTION(Disconnected, RPC_E_DISCONNECTED)        \
    WINRT_EXCEPTION(Failure, E_FAIL)                         \
    WINRT_EXCEPTION(InvalidArgument, E_INVALIDARG)           \
    WINRT_EXCEPTION(InvalidCast, E_NOINTERFACE)              \
    WINRT_EXCEPTION(NotImplemented, E_NOTIMPL)               \
    WINRT_EXCEPTION(NullReference, E_POINTER)                \
    WINRT_EXCEPTION(ObjectDisposed, RO_E_CLOSED)             \
    WINRT_EXCEPTION(OperationCanceled, E_ABORT)              \
    WINRT_EXCEPTION(OutOfBounds, E_BOUNDS)                   \
    WINRT_EXCEPTION(OutOfMemory, E_OUTOFMEMORY)              \
    WINRT_EXCEPTION(WrongThread, RPC_E_WRONG_THREAD)

#define WINRT_EXCEPTION(name, hr)                                                  \
    void WINAPI __abi_WinRTraise##name##Exception(void)                            \
    {                                                                              \
        FIXME("(): stub!\n");                                                      \
    }                                                                              \
    void *__cdecl name##Exception_ctor(void *this)                      \
    {                                                                              \
        FIXME("(%p): stub!\n", this);                                              \
        return this;                                                               \
    }                                                                              \
    void *__cdecl name##Exception_hstring_ctor(void *this, HSTRING msg) \
    {                                                                              \
        FIXME("(%p, %s): stub!\n", this, debugstr_hstring(msg));                   \
        return this;                                                               \
    }

WINRT_EXCEPTIONS
#undef WINRT_EXCEPTION

void *__cdecl Exception_ctor(void *this, HRESULT hr)
{
    FIXME("(%p, %#lx): stub!\n", this, hr);
    return this;
}

void *__cdecl Exception_hstring_ctor(void *this, HRESULT hr, HSTRING msg)
{
    FIXME("(%p, %#lx, %s): stub!\n", this, hr, debugstr_hstring(msg));
    return this;
}

void *__cdecl COMException_ctor(void *this, HRESULT hr)
{
    FIXME("(%p, %#lx): stub!\n", this, hr);
    return this;
}

void *__cdecl COMException_hstring_ctor(void *this, HRESULT hr, HSTRING msg)
{
    FIXME("(%p, %#lx, %s): stub!\n", this, hr, debugstr_hstring(msg));
    return this;
}

HSTRING __cdecl Exception_get_Message(void *excp)
{
    FIXME("(%p): stub!\n", excp);
    return NULL;
}
