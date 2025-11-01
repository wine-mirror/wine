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

#define COBJMACROS

#include <stdint.h>

#include "roapi.h"
#include "winstring.h"
#include "roerrorapi.h"
#define WIDL_using_Windows_Foundation
#include "windows.foundation.h"
#include "wine/debug.h"

#include "cxx.h"
#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(vccorlib);

#ifdef _WIN64
#define EXCEPTION_REF_NAME(name) ".PE$AAV" #name "Exception@Platform@@"
#else
#define EXCEPTION_REF_NAME(name) ".P$AAV" #name "Exception@Platform@@"
#endif

/* Data members of the "Exception" C++/CX class.
 * A back-pointer to this struct is stored just before the IInspectable vtable of an Exception object.
 * TODO: The message fields are likely obtained from IRestrictedErrorInfo::GetErrorDetails, so we
 * should use that once it has been implemented. */
struct exception_inner
{
    /* This only gets set when the exception is thrown. */
    BSTR description;
    /* Likewise, but can also be set by CreateExceptionWithMessage. */
    BSTR restricted_desc;
    void *unknown1;
    void *unknown2;
    HRESULT hr;
    /* Only gets set when the exception is thrown. */
    IRestrictedErrorInfo *error_info;
    const cxx_exception_type *exception_type;
    /* Set to 32 and 64 on 32 and 64-bit platforms respectively, not sure what the purpose is. */
    UINT32 unknown3;
    /* Called before the exception is thrown by _CxxThrowException.
     * This sets the description and error_info fields, and probably originates a WinRT error
     * with RoOriginateError/RoOriginateLanguageException. */
    void (*WINAPI set_exception_info)(struct exception_inner **);
};

struct Exception
{
    IInspectable IInspectable_iface;
    const void *IPrintable_iface;
    const void *IEquatable_iface;
    IClosable IClosable_iface;
    struct exception_inner inner;
    /* Exceptions use the weakref control block for reference counting, even though they don't implement
     * IWeakReferenceSource. */
    struct control_block *control_block;
    /* This is lazily initialized, i.e, only when QueryInterface(IID_IMarshal) gets called. The default value is
     * UINTPTR_MAX/-1 */
    IUnknown *marshal;
};

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

static inline struct Exception *impl_Exception_from_IInspectable(IInspectable *iface)
{
    return CONTAINING_RECORD(iface, struct Exception, IInspectable_iface);
}

static HRESULT WINAPI Exception_QueryInterface(IInspectable *iface, const GUID *iid, void **out)
{
    struct Exception *impl = impl_Exception_from_IInspectable(iface);

    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject))
    {
        IInspectable_AddRef((*out = &impl->IInspectable_iface));
        return S_OK;
    }
    if (IsEqualGUID(iid, &IID_IClosable))
    {
        IClosable_AddRef((*out = &impl->IClosable_iface));
        return S_OK;
    }
    if (IsEqualGUID(iid, &IID_IMarshal))
    {
        IUnknown *marshal, *old;
        HRESULT hr;

        if ((ULONG_PTR)impl->marshal != UINTPTR_MAX)
            return IUnknown_QueryInterface(impl->marshal, iid, out);
        /* Try initializing impl->marshal. */
        if (FAILED(hr = CoCreateFreeThreadedMarshaler((IUnknown *)&impl->IInspectable_iface, &marshal)))
            return hr;
        old = InterlockedCompareExchangePointer((void *)&impl->marshal, marshal, (void *)UINTPTR_MAX);
        if ((ULONG_PTR)old != UINTPTR_MAX) /* Someone else has already set impl->marshal, use it. */
            IUnknown_Release(marshal);
        return IUnknown_QueryInterface(impl->marshal, iid, out);
    }

    ERR("%s not implemented, returning E_NOTINTERFACE.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI Exception_AddRef(IInspectable *iface)
{
    struct Exception *impl = impl_Exception_from_IInspectable(iface);
    TRACE("(%p)\n", iface);
    return InterlockedIncrement(&impl->control_block->ref_strong);
}

static ULONG WINAPI Exception_Release(IInspectable *iface)
{
    struct Exception *impl = impl_Exception_from_IInspectable(iface);
    ULONG ref = InterlockedDecrement(&impl->control_block->ref_strong);

    TRACE("(%p)\n", iface);

    if (!ref)
    {
        SysFreeString(impl->inner.description);
        SysFreeString(impl->inner.restricted_desc);
        if ((ULONG_PTR)impl->marshal != UINTPTR_MAX)
            IUnknown_Release((IUnknown *)impl->marshal);
        /* Because Exception objects are never allocated inline, we don't need to use ReleaseTarget. */
        IWeakReference_Release(&impl->control_block->IWeakReference_iface);
        FreeException(impl);
    }
    return ref;
}

static HRESULT WINAPI Exception_GetRuntimeClassName(IInspectable *iface, HSTRING *class_name)
{
    struct Exception *impl = impl_Exception_from_IInspectable(iface);
    const WCHAR *buf;

    TRACE("(%p, %p)\n", iface, class_name);

#define WINRT_EXCEPTION(name, hr) case (hr): buf = L"Platform." #name "Exception"; break;
    switch (impl->inner.hr)
    {
    WINRT_EXCEPTIONS
#undef WINRT_EXCEPTION
    default:
        buf = L"Platform.COMException";
        break;
    }
    return WindowsCreateString(buf, wcslen(buf), class_name);
}

static HRESULT WINAPI Exception_GetIids(IInspectable *iface, ULONG *count, IID **iids)
{
    TRACE("(%p, %p, %p)\n", iface, count, iids);

    *count = 0;
    *iids = NULL;
    return S_OK;
}

static HRESULT WINAPI Exception_GetTrustLevel(IInspectable *iface, TrustLevel *level)
{
    FIXME("(%p, %p)\n", iface, level);
    return E_NOTIMPL;
}

DEFINE_RTTI_DATA(Exception_ref, 0, EXCEPTION_REF_NAME());
typedef struct Exception *Exception_ref;
DEFINE_CXX_TYPE(Exception_ref);

DEFINE_RTTI_DATA(Exception, 0, "?.AVException@Platform@@");
COM_VTABLE_RTTI_START(IInspectable, Exception)
COM_VTABLE_ENTRY(Exception_QueryInterface)
COM_VTABLE_ENTRY(Exception_AddRef)
COM_VTABLE_ENTRY(Exception_Release)
COM_VTABLE_ENTRY(Exception_GetIids)
COM_VTABLE_ENTRY(Exception_GetRuntimeClassName)
COM_VTABLE_ENTRY(Exception_GetTrustLevel)
COM_VTABLE_RTTI_END;

DEFINE_IINSPECTABLE_(Exception_Closable, IClosable, struct Exception,
        impl_Exception_from_IClosable, IClosable_iface, &impl->IInspectable_iface);

static HRESULT WINAPI Exception_Closable_Close(IClosable *iface)
{
    FIXME("(%p)\n", iface);
    return E_NOTIMPL;
}

DEFINE_RTTI_DATA(Exception_Closable, offsetof(struct Exception, IClosable_iface),
        "?.AVException@Platform@@")
COM_VTABLE_RTTI_START(IClosable, Exception_Closable)
COM_VTABLE_ENTRY(Exception_Closable_QueryInterface)
COM_VTABLE_ENTRY(Exception_Closable_AddRef)
COM_VTABLE_ENTRY(Exception_Closable_Release)
COM_VTABLE_ENTRY(Exception_Closable_GetIids)
COM_VTABLE_ENTRY(Exception_Closable_GetRuntimeClassName)
COM_VTABLE_ENTRY(Exception_Closable_GetTrustLevel)
COM_VTABLE_ENTRY(Exception_Closable_Close)
COM_VTABLE_RTTI_END;

static void WINAPI set_exception_info(struct exception_inner **inner)
{
    struct exception_inner *info = *inner;
    WCHAR buf[256];

    FIXME("(%p): semi-stub!\n", inner);

    if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, info->hr, 0, buf, ARRAY_SIZE(buf), NULL))
        info->description = SysAllocString(buf);
    if (!info->restricted_desc)
        info->restricted_desc = SysAllocString(info->description);
    info->error_info = NULL;
}

struct Exception *__cdecl Exception_ctor(struct Exception *this, HRESULT hr)
{
    struct exception_alloc *base = CONTAINING_RECORD(this, struct exception_alloc, data);

    TRACE("(%p, %#lx)\n", this, hr);

    this->IInspectable_iface.lpVtbl = &Exception_vtable.vtable;
    this->IClosable_iface.lpVtbl = &Exception_Closable_vtable.vtable;
    this->IEquatable_iface = this->IPrintable_iface = NULL;

    memset(&this->inner, 0, sizeof(this->inner));
    base->exception_inner = &this->inner;
    this->inner.exception_type = &Exception_ref_exception_type;
    this->inner.hr = hr;
    this->inner.unknown3 = sizeof(void *) == 8 ? 64 : 32;
    this->inner.set_exception_info = set_exception_info;

    this->marshal = (IUnknown *)UINTPTR_MAX;

    if (SUCCEEDED(hr))
        __abi_WinRTraiseInvalidArgumentException();
    return this;
}

struct Exception *__cdecl Exception_hstring_ctor(struct Exception *this, HRESULT hr, HSTRING msg)
{
    const WCHAR *buf;
    BOOL has_null;
    UINT32 len;

    TRACE("(%p, %#lx, %s)\n", this, hr, debugstr_hstring(msg));

    Exception_ctor(this, hr);
    if (WindowsIsStringEmpty(msg)) return this;

    /* Native throws InvalidArgumentException if msg has an embedded NUL byte. */
    if (FAILED(hr = WindowsStringHasEmbeddedNull(msg, &has_null)))
        __abi_WinRTraiseCOMException(hr);
    else if (has_null)
        __abi_WinRTraiseInvalidArgumentException();

    buf = WindowsGetStringRawBuffer(msg, &len);
    if (len && !(this->inner.restricted_desc = SysAllocStringLen(buf, len)))
        __abi_WinRTraiseOutOfMemoryException();
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

void init_exception(void *base)
{
    INIT_RTTI(Exception_ref, base);
    INIT_CXX_TYPE(Exception_ref, base);
    INIT_RTTI(Exception, base);
    INIT_RTTI(Exception_Closable, base);
}
