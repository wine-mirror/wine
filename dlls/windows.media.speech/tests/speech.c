/*
 * Copyright 2021 Rémi Bernon for CodeWeavers
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
#include <stdarg.h>

#include "corerror.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winstring.h"

#include "initguid.h"
#include "roapi.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Globalization
#include "windows.globalization.h"
#define WIDL_using_Windows_Media_SpeechRecognition
#include "windows.media.speechrecognition.h"
#define WIDL_using_Windows_Media_SpeechSynthesis
#include "windows.media.speechsynthesis.h"

#include "wine/test.h"

#define AsyncStatus_Closed 4

#define SPERR_WINRT_INTERNAL_ERROR 0x800455a0
#define SPERR_WINRT_INCORRECT_FORMAT 0x80131537

#define IHandler_RecognitionResult ITypedEventHandler_SpeechContinuousRecognitionSession_SpeechContinuousRecognitionResultGeneratedEventArgs
#define IHandler_RecognitionResultVtbl ITypedEventHandler_SpeechContinuousRecognitionSession_SpeechContinuousRecognitionResultGeneratedEventArgsVtbl
#define IID_IHandler_RecognitionResult IID_ITypedEventHandler_SpeechContinuousRecognitionSession_SpeechContinuousRecognitionResultGeneratedEventArgs
#define impl_from_IHandler_RecognitionResult impl_from_ITypedEventHandler_SpeechContinuousRecognitionSession_SpeechContinuousRecognitionResultGeneratedEventArgs
#define IHandler_RecognitionResult_iface ITypedEventHandler_SpeechContinuousRecognitionSession_SpeechContinuousRecognitionResultGeneratedEventArgs_iface

#define IHandler_RecognitionCompleted ITypedEventHandler_SpeechContinuousRecognitionSession_SpeechContinuousRecognitionCompletedEventArgs
#define IHandler_RecognitionCompletedVtbl ITypedEventHandler_SpeechContinuousRecognitionSession_SpeechContinuousRecognitionCompletedEventArgsVtbl
#define IID_IHandler_RecognitionCompleted IID_ITypedEventHandler_SpeechContinuousRecognitionSession_SpeechContinuousRecognitionCompletedEventArgs
#define impl_from_IHandler_RecognitionCompleted impl_from_ITypedEventHandler_SpeechContinuousRecognitionSession_SpeechContinuousRecognitionCompletedEventArgs
#define IHandler_RecognitionCompleted_iface ITypedEventHandler_SpeechContinuousRecognitionSession_SpeechContinuousRecognitionCompletedEventArgs_iface

#define IAsyncHandler_IInspectable_iface IAsyncOperationCompletedHandler_IInspectable_iface

HRESULT (WINAPI *pDllGetActivationFactory)(HSTRING, IActivationFactory **);
static BOOL is_win10_1507 = FALSE;
static BOOL is_win10_1709 = FALSE;

static inline LONG get_ref(IUnknown *obj)
{
    IUnknown_AddRef(obj);
    return IUnknown_Release(obj);
}

#define check_refcount(obj, exp) check_refcount_(__LINE__, obj, exp)
static inline void check_refcount_(unsigned int line, void *obj, LONG exp)
{
    LONG ref = get_ref(obj);
    ok_(__FILE__, line)(exp == ref, "Unexpected refcount %lu, expected %lu\n", ref, exp);
}

#define check_interface(obj, iid, exp) check_interface_(__LINE__, obj, iid, exp, FALSE)
#define check_optional_interface(obj, iid, exp) check_interface_(__LINE__, obj, iid, exp, TRUE)
static void check_interface_(unsigned int line, void *obj, const IID *iid, BOOL supported, BOOL optional)
{
    IUnknown *iface = obj;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr || broken(hr == E_NOINTERFACE && optional), "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

struct completed_event_handler
{
    IHandler_RecognitionCompleted IHandler_RecognitionCompleted_iface;
    LONG ref;
};

static inline struct completed_event_handler *impl_from_IHandler_RecognitionCompleted( IHandler_RecognitionCompleted *iface )
{
    return CONTAINING_RECORD(iface, struct completed_event_handler, IHandler_RecognitionCompleted_iface);
}

HRESULT WINAPI completed_event_handler_QueryInterface( IHandler_RecognitionCompleted *iface, REFIID iid, void **out )
{
    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IHandler_RecognitionCompleted))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    trace("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

ULONG WINAPI completed_event_handler_AddRef( IHandler_RecognitionCompleted *iface )
{
    struct completed_event_handler *impl = impl_from_IHandler_RecognitionCompleted(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    return ref;
}

ULONG WINAPI completed_event_handler_Release( IHandler_RecognitionCompleted *iface )
{
    struct completed_event_handler *impl = impl_from_IHandler_RecognitionCompleted(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    return ref;
}

HRESULT WINAPI completed_event_handler_Invoke( IHandler_RecognitionCompleted *iface,
                                               ISpeechContinuousRecognitionSession *sender,
                                               ISpeechContinuousRecognitionCompletedEventArgs *args )
{
    trace("iface %p, sender %p, args %p.\n", iface, sender, args);
    return S_OK;
}

static const struct IHandler_RecognitionCompletedVtbl completed_event_handler_vtbl =
{
    /* IUnknown methods */
    completed_event_handler_QueryInterface,
    completed_event_handler_AddRef,
    completed_event_handler_Release,
    /* ITypedEventHandler<SpeechContinuousRecognitionSession*, SpeechContinuousRecognitionCompletedEventArgs* > methods */
    completed_event_handler_Invoke
};

static HRESULT WINAPI completed_event_handler_create_static( struct completed_event_handler *impl )
{
    impl->IHandler_RecognitionCompleted_iface.lpVtbl = &completed_event_handler_vtbl;
    impl->ref = 1;

    return S_OK;
}

struct recognition_result_handler
{
    IHandler_RecognitionResult IHandler_RecognitionResult_iface;
    LONG ref;
};

static inline struct recognition_result_handler *impl_from_IHandler_RecognitionResult( IHandler_RecognitionResult *iface )
{
    return CONTAINING_RECORD(iface, struct recognition_result_handler, IHandler_RecognitionResult_iface);
}

HRESULT WINAPI recognition_result_handler_QueryInterface( IHandler_RecognitionResult *iface, REFIID iid, void **out )
{
    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IHandler_RecognitionResult))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    trace("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

ULONG WINAPI recognition_result_handler_AddRef( IHandler_RecognitionResult *iface )
{
    struct recognition_result_handler *impl = impl_from_IHandler_RecognitionResult(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    return ref;
}

ULONG WINAPI recognition_result_handler_Release( IHandler_RecognitionResult *iface )
{
    struct recognition_result_handler *impl = impl_from_IHandler_RecognitionResult(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    return ref;
}

HRESULT WINAPI recognition_result_handler_Invoke( IHandler_RecognitionResult *iface,
                                                  ISpeechContinuousRecognitionSession *sender,
                                                  ISpeechContinuousRecognitionResultGeneratedEventArgs *args )
{
    trace("iface %p, sender %p, args %p.\n", iface, sender, args);
    return S_OK;
}

static const struct IHandler_RecognitionResultVtbl recognition_result_handler_vtbl =
{
    /* IUnknown methods */
    recognition_result_handler_QueryInterface,
    recognition_result_handler_AddRef,
    recognition_result_handler_Release,
    /* ITypedEventHandler<SpeechContinuousRecognitionSession*, SpeechContinuousRecognitionResultGeneratedEventArgs* > methods */
    recognition_result_handler_Invoke
};

static HRESULT WINAPI recognition_result_handler_create_static( struct recognition_result_handler *impl )
{
    impl->IHandler_RecognitionResult_iface.lpVtbl = &recognition_result_handler_vtbl;
    impl->ref = 1;

    return S_OK;
}

struct async_void_handler
{
    IAsyncActionCompletedHandler IAsyncActionCompletedHandler_iface;
    LONG ref;

    HANDLE event_block;
    HANDLE event_finished;
};

static inline struct async_void_handler *impl_from_IAsyncActionCompletedHandler(IAsyncActionCompletedHandler *iface)
{
    return CONTAINING_RECORD(iface, struct async_void_handler, IAsyncActionCompletedHandler_iface);
}

HRESULT WINAPI async_void_handler_QueryInterface( IAsyncActionCompletedHandler *iface, REFIID iid, void **out )
{
    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IAsyncActionCompletedHandler))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    trace("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

ULONG WINAPI async_void_handler_AddRef( IAsyncActionCompletedHandler *iface )
{
    struct async_void_handler *impl = impl_from_IAsyncActionCompletedHandler(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    return ref;
}

ULONG WINAPI async_void_handler_Release( IAsyncActionCompletedHandler *iface )
{
    struct async_void_handler *impl = impl_from_IAsyncActionCompletedHandler(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    return ref;
}

HRESULT WINAPI async_void_handler_Invoke( IAsyncActionCompletedHandler *iface, IAsyncAction *sender, AsyncStatus status )
{
    struct async_void_handler *impl = impl_from_IAsyncActionCompletedHandler(iface);

    trace("iface %p, sender %p, status %d.\n", iface, sender, status);

    /* Signal finishing of the handler. */
    if (impl->event_finished) SetEvent(impl->event_finished);
    /* Block handler until event is set. */
    if (impl->event_block) WaitForSingleObject(impl->event_block, INFINITE);

    return S_OK;
}

static const struct IAsyncActionCompletedHandlerVtbl async_void_handler_vtbl =
{
    /* IUnknown methods */
    async_void_handler_QueryInterface,
    async_void_handler_AddRef,
    async_void_handler_Release,
    /* IAsyncActionCompletedHandler methods */
    async_void_handler_Invoke
};


static HRESULT WINAPI async_void_handler_create_static( struct async_void_handler *impl )
{
    impl->IAsyncActionCompletedHandler_iface.lpVtbl = &async_void_handler_vtbl;
    impl->ref = 1;

    return S_OK;
}

#define await_async_void(operation, handler) await_async_void_(__LINE__, operation, handler)
static void await_async_void_( unsigned int line, IAsyncAction *action, struct async_void_handler *handler )
{
    HRESULT hr;

    async_void_handler_create_static(handler);
    handler->event_block = NULL;
    handler->event_finished = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok_(__FILE__, line)(!!handler->event_finished, "event_finished wasn't created.\n");

    hr = IAsyncAction_put_Completed(action, &handler->IAsyncActionCompletedHandler_iface);
    ok_(__FILE__, line)(hr == S_OK, "IAsyncAction_put_Completed failed, hr %#lx.\n", hr);
    ok_(__FILE__, line)(!WaitForSingleObject(handler->event_finished , 5000), "Wait for event_finished failed.\n");
    CloseHandle(handler->event_finished);
}

struct async_inspectable_handler
{
    IAsyncOperationCompletedHandler_IInspectable IAsyncHandler_IInspectable_iface;
    const GUID *iid;
    LONG ref;

    HANDLE event_block;
    HANDLE event_finished;
};

static inline struct async_inspectable_handler *impl_from_IAsyncOperationCompletedHandler_IInspectable( IAsyncOperationCompletedHandler_IInspectable *iface )
{
    return CONTAINING_RECORD(iface, struct async_inspectable_handler, IAsyncHandler_IInspectable_iface);
}

HRESULT WINAPI async_inspectable_handler_QueryInterface( IAsyncOperationCompletedHandler_IInspectable *iface,
                                                         REFIID iid,
                                                         void **out )
{
    struct async_inspectable_handler *impl = impl_from_IAsyncOperationCompletedHandler_IInspectable(iface);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, impl->iid))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    trace("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

ULONG WINAPI async_inspectable_handler_AddRef( IAsyncOperationCompletedHandler_IInspectable *iface )
{
    struct async_inspectable_handler *impl = impl_from_IAsyncOperationCompletedHandler_IInspectable(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    return ref;
}

ULONG WINAPI async_inspectable_handler_Release( IAsyncOperationCompletedHandler_IInspectable *iface )
{
    struct async_inspectable_handler *impl = impl_from_IAsyncOperationCompletedHandler_IInspectable(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    return ref;
}

HRESULT WINAPI async_inspectable_handler_Invoke( IAsyncOperationCompletedHandler_IInspectable *iface,
                                                 IAsyncOperation_IInspectable *sender,
                                                 AsyncStatus status )
{
    struct async_inspectable_handler *impl = impl_from_IAsyncOperationCompletedHandler_IInspectable(iface);

    trace("Iface %p, sender %p, status %d.\n", iface, sender, status);

    /* Signal finishing of the handler. */
    if (impl->event_finished) SetEvent(impl->event_finished);
    /* Block handler until event is set. */
    if (impl->event_block) WaitForSingleObject(impl->event_block, INFINITE);

    return S_OK;
}

static const struct IAsyncOperationCompletedHandler_IInspectableVtbl async_inspectable_handler_vtbl =
{
    /* IUnknown methods */
    async_inspectable_handler_QueryInterface,
    async_inspectable_handler_AddRef,
    async_inspectable_handler_Release,
    /* IAsyncOperationCompletedHandler<IInspectable* > methods */
    async_inspectable_handler_Invoke
};

static HRESULT WINAPI async_inspectable_handler_create_static( struct async_inspectable_handler *impl, const GUID *iid )
{
    impl->IAsyncOperationCompletedHandler_IInspectable_iface.lpVtbl = &async_inspectable_handler_vtbl;
    impl->iid = iid;
    impl->ref = 1;

    return S_OK;
}

#define await_async_inspectable(operation, handler, iid) await_async_inspectable_(__LINE__, operation, handler, iid)
static void await_async_inspectable_( unsigned int line, IAsyncOperation_IInspectable *operation, struct async_inspectable_handler *handler, const GUID *handler_iid )
{
    HRESULT hr;

    async_inspectable_handler_create_static(handler, handler_iid);
    handler->event_block = NULL;
    handler->event_finished = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok_(__FILE__, line)(!!handler->event_finished, "event_finished wasn't created.\n");

    hr = IAsyncOperation_IInspectable_put_Completed(operation, &handler->IAsyncHandler_IInspectable_iface);
    ok_(__FILE__, line)(hr == S_OK, "IAsyncOperation_IInspectable_put_Completed failed, hr %#lx.\n", hr);
    ok_(__FILE__, line)(!WaitForSingleObject(handler->event_finished , 5000), "Wait for event_finished failed.\n");
    CloseHandle(handler->event_finished);
}

#define check_async_info(obj, exp_id, exp_status, exp_hr) check_async_info_(__LINE__, obj, exp_id, exp_status, exp_hr)
static void check_async_info_( unsigned int line, IInspectable *async_obj, UINT32 expect_id, AsyncStatus expect_status, HRESULT expect_hr)
{
    IAsyncInfo *async_info;
    AsyncStatus async_status;
    UINT32 async_id;
    HRESULT hr, async_hr;

    hr = IInspectable_QueryInterface(async_obj, &IID_IAsyncInfo, (void **)&async_info);
    ok_(__FILE__, line)(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    async_id = 0xdeadbeef;
    hr = IAsyncInfo_get_Id(async_info, &async_id);
    if (expect_status < 4) ok_(__FILE__, line)(hr == S_OK, "IAsyncInfo_get_Id returned %#lx\n", hr);
    else ok_(__FILE__, line)(hr == E_ILLEGAL_METHOD_CALL, "IAsyncInfo_get_Id returned %#lx\n", hr);
    todo_wine_if(expect_id != 1) ok_(__FILE__, line)(async_id == expect_id, "got async_id %#x\n", async_id);

    async_status = 0xdeadbeef;
    hr = IAsyncInfo_get_Status(async_info, &async_status);
    if (expect_status < 4) ok_(__FILE__, line)(hr == S_OK, "IAsyncInfo_get_Status returned %#lx\n", hr);
    else ok_(__FILE__, line)(hr == E_ILLEGAL_METHOD_CALL, "IAsyncInfo_get_Status returned %#lx\n", hr);
    ok_(__FILE__, line)(async_status == expect_status, "got async_status %#x\n", async_status);

    async_hr = 0xdeadbeef;
    hr = IAsyncInfo_get_ErrorCode(async_info, &async_hr);
    if (expect_status < 4) ok_(__FILE__, line)(hr == S_OK, "IAsyncInfo_get_ErrorCode returned %#lx\n", hr);
    else ok_(__FILE__, line)(hr == E_ILLEGAL_METHOD_CALL, "IAsyncInfo_get_ErrorCode returned %#lx\n", hr);
    if (expect_status < 4) ok_(__FILE__, line)(async_hr == expect_hr, "got async_hr %#lx\n", async_hr);
    else ok_(__FILE__, line)(async_hr == E_ILLEGAL_METHOD_CALL, "got async_hr %#lx\n", async_hr);

    IAsyncInfo_Release(async_info);
}

struct iterator_hstring
{
    IIterator_HSTRING IIterator_HSTRING_iface;
    LONG ref;

    UINT32 index;
    UINT32 size;
    HSTRING *values;
};

static inline struct iterator_hstring *impl_from_IIterator_HSTRING( IIterator_HSTRING *iface )
{
    return CONTAINING_RECORD(iface, struct iterator_hstring, IIterator_HSTRING_iface);
}

static HRESULT WINAPI iterator_hstring_QueryInterface( IIterator_HSTRING *iface, REFIID iid, void **out )
{
    struct iterator_hstring *impl = impl_from_IIterator_HSTRING(iface);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IIterator_HSTRING))
    {
        IInspectable_AddRef((*out = &impl->IIterator_HSTRING_iface));
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI iterator_hstring_AddRef( IIterator_HSTRING *iface )
{
    struct iterator_hstring *impl = impl_from_IIterator_HSTRING(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    return ref;
}

static ULONG WINAPI iterator_hstring_Release( IIterator_HSTRING *iface )
{
    struct iterator_hstring *impl = impl_from_IIterator_HSTRING(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    return ref;
}

static HRESULT WINAPI iterator_hstring_GetIids( IIterator_HSTRING *iface, ULONG *iid_count, IID **iids )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI iterator_hstring_GetRuntimeClassName( IIterator_HSTRING *iface, HSTRING *class_name )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI iterator_hstring_GetTrustLevel( IIterator_HSTRING *iface, TrustLevel *trust_level )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI iterator_hstring_get_Current( IIterator_HSTRING *iface, HSTRING *value )
{
    struct iterator_hstring *impl = impl_from_IIterator_HSTRING(iface);
    HRESULT hr;

    *value = NULL;
    if (impl->index >= impl->size) return E_BOUNDS;

    hr = WindowsDuplicateString(impl->values[impl->index], value);
    return hr;
}

static HRESULT WINAPI iterator_hstring_get_HasCurrent( IIterator_HSTRING *iface, boolean *value )
{
    struct iterator_hstring *impl = impl_from_IIterator_HSTRING(iface);

    *value = impl->index < impl->size;
    return S_OK;
}

static HRESULT WINAPI iterator_hstring_MoveNext( IIterator_HSTRING *iface, boolean *value )
{
    struct iterator_hstring *impl = impl_from_IIterator_HSTRING(iface);

    if (impl->index < impl->size) impl->index++;
    return IIterator_HSTRING_get_HasCurrent(iface, value);
}

static HRESULT WINAPI iterator_hstring_GetMany( IIterator_HSTRING *iface, UINT32 items_size,
                                                HSTRING *items, UINT *count )
{
    return E_NOTIMPL;
}

static const struct IIterator_HSTRINGVtbl iterator_hstring_vtbl =
{
    /* IUnknown methods */
    iterator_hstring_QueryInterface,
    iterator_hstring_AddRef,
    iterator_hstring_Release,
    /* IInspectable methods */
    iterator_hstring_GetIids,
    iterator_hstring_GetRuntimeClassName,
    iterator_hstring_GetTrustLevel,
    /* IIterator<HSTRING> methods */
    iterator_hstring_get_Current,
    iterator_hstring_get_HasCurrent,
    iterator_hstring_MoveNext,
    iterator_hstring_GetMany
};

static HRESULT WINAPI iterator_hstring_create_static( struct iterator_hstring *impl, HSTRING *strings, UINT32 size )
{
    impl->IIterator_HSTRING_iface.lpVtbl = &iterator_hstring_vtbl;
    impl->ref = 1;
    impl->index = 0;
    impl->size = size;
    impl->values = strings;

    return S_OK;
}

struct iterable_hstring
{
    IIterable_HSTRING IIterable_HSTRING_iface;
    LONG ref;

    IIterator_HSTRING *iterator;
};

static inline struct iterable_hstring *impl_from_Iterable_HSTRING( IIterable_HSTRING *iface )
{
    return CONTAINING_RECORD(iface, struct iterable_hstring, IIterable_HSTRING_iface);
}

static HRESULT WINAPI iterable_hstring_QueryInterface( IIterable_HSTRING *iface, REFIID iid, void **out )
{
    struct iterable_hstring *impl = impl_from_Iterable_HSTRING(iface);

    trace("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IIterable_HSTRING))
    {
        IInspectable_AddRef((*out = &impl->IIterable_HSTRING_iface));
        return S_OK;
    }

    trace("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI iterable_hstring_AddRef( IIterable_HSTRING *iface )
{
    struct iterable_hstring *impl = impl_from_Iterable_HSTRING(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    trace("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI iterable_hstring_Release( IIterable_HSTRING *iface )
{
    struct iterable_hstring *impl = impl_from_Iterable_HSTRING(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    trace("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static HRESULT WINAPI iterable_hstring_GetIids( IIterable_HSTRING *iface, ULONG *iid_count, IID **iids )
{
    trace("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI iterable_hstring_GetRuntimeClassName( IIterable_HSTRING *iface, HSTRING *class_name )
{
    trace("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI iterable_hstring_GetTrustLevel( IIterable_HSTRING *iface, TrustLevel *trust_level )
{
    trace("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI iterable_hstring_First( IIterable_HSTRING *iface, IIterator_HSTRING **value )
{
    struct iterable_hstring *impl = impl_from_Iterable_HSTRING(iface);
    struct iterator_hstring *impl_iter = impl_from_IIterator_HSTRING(impl->iterator);

    impl_iter->index = 0;
    IIterator_HSTRING_AddRef((*value = impl->iterator));
    return S_OK;
}

static const struct IIterable_HSTRINGVtbl iterable_hstring_vtbl =
{
    /* IUnknown methods */
    iterable_hstring_QueryInterface,
    iterable_hstring_AddRef,
    iterable_hstring_Release,
    /* IInspectable methods */
    iterable_hstring_GetIids,
    iterable_hstring_GetRuntimeClassName,
    iterable_hstring_GetTrustLevel,
    /* IIterable<HSTRING> methods */
    iterable_hstring_First
};

static HRESULT WINAPI iterable_hstring_create_static( struct iterable_hstring *impl, struct iterator_hstring *iterator )
{
    impl->IIterable_HSTRING_iface.lpVtbl = &iterable_hstring_vtbl;
    impl->ref = 1;
    impl->iterator = &iterator->IIterator_HSTRING_iface;

    return S_OK;
}

static void test_ActivationFactory(void)
{
    static const WCHAR *synthesizer_name = L"Windows.Media.SpeechSynthesis.SpeechSynthesizer";
    static const WCHAR *recognizer_name = L"Windows.Media.SpeechRecognition.SpeechRecognizer";
    static const WCHAR *garbage_name = L"Some.Garbage.Class";
    IActivationFactory *factory = NULL, *factory2 = NULL, *factory3 = NULL, *factory4 = NULL;
    ISpeechRecognizerStatics2 *recognizer_statics2 = NULL;
    HSTRING str, str2, str3;
    HMODULE hdll;
    HRESULT hr;
    ULONG ref;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#lx.\n", hr);

    hr = WindowsCreateString(synthesizer_name, wcslen(synthesizer_name), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx.\n", hr);

    hr = WindowsCreateString(recognizer_name, wcslen(recognizer_name), &str2);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx.\n", hr);

    hr = WindowsCreateString(garbage_name, wcslen(garbage_name), &str3);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx.\n", hr);

    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK, "RoGetActivationFactory failed, hr %#lx.\n", hr);

    check_refcount(factory, 2);

    check_interface(factory, &IID_IInspectable, TRUE);
    check_interface(factory, &IID_IAgileObject, TRUE);
    check_interface(factory, &IID_IInstalledVoicesStatic, TRUE);
    check_interface(factory, &IID_ISpeechRecognizerFactory, FALSE);
    check_interface(factory, &IID_ISpeechRecognizerStatics, FALSE);
    check_interface(factory, &IID_ISpeechRecognizerStatics2, FALSE);

    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory2);
    ok(hr == S_OK, "RoGetActivationFactory failed, hr %#lx.\n", hr);
    ok(factory == factory2, "Factories pointed at factory %p factory2 %p.\n", factory, factory2);
    check_refcount(factory2, 3);

    hr = RoGetActivationFactory(str2, &IID_IActivationFactory, (void **)&factory3);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "Got unexpected hr %#lx.\n", hr);

    if (hr == S_OK) /* Win10+ only */
    {
        check_refcount(factory3, 2);
        ok(factory != factory3, "Factories pointed at factory %p factory3 %p.\n", factory, factory3);

        check_interface(factory3, &IID_IInspectable, TRUE);
        check_interface(factory3, &IID_IAgileObject, TRUE);
        check_interface(factory3, &IID_ISpeechRecognizerFactory, TRUE);
        check_interface(factory3, &IID_ISpeechRecognizerStatics, TRUE);

        hr = IActivationFactory_QueryInterface(factory3, &IID_ISpeechRecognizerStatics2, (void **)&recognizer_statics2);
        ok(hr == S_OK || broken(hr == E_NOINTERFACE), "IActivationFactory_QueryInterface failed, hr %#lx.\n", hr);

        if (hr == S_OK) /* ISpeechRecognizerStatics2 not available in Win10 1507 */
        {
            ref = ISpeechRecognizerStatics2_Release(recognizer_statics2);
            ok(ref == 2, "Got unexpected refcount: %lu.\n", ref);
        }
        else is_win10_1507 = TRUE;

        check_interface(factory3, &IID_IInstalledVoicesStatic, FALSE);

        ref = IActivationFactory_Release(factory3);
        ok(ref == 1, "Got unexpected refcount: %lu.\n", ref);
    }

    hdll = LoadLibraryW(L"windows.media.speech.dll");

    if (hdll)
    {
        pDllGetActivationFactory = (void *)GetProcAddress(hdll, "DllGetActivationFactory");
        ok(!!pDllGetActivationFactory, "DllGetActivationFactory not found.\n");

        hr = pDllGetActivationFactory(str3, &factory4);
        ok((hr == CLASS_E_CLASSNOTAVAILABLE), "Got unexpected hr %#lx.\n", hr);
        FreeLibrary(hdll);
    }

    hr = RoGetActivationFactory(str3, &IID_IActivationFactory, (void **)&factory4);
    ok((hr == REGDB_E_CLASSNOTREG), "RoGetActivationFactory failed, hr %#lx.\n", hr);

    ref = IActivationFactory_Release(factory2);
    ok(ref == 2, "Got unexpected refcount: %lu.\n", ref);

    ref = IActivationFactory_Release(factory);
    ok(ref == 1, "Got unexpected refcount: %lu.\n", ref);

    WindowsDeleteString(str);
    WindowsDeleteString(str2);
    WindowsDeleteString(str3);

    RoUninitialize();
}

static void test_SpeechSynthesizer(void)
{
    static const WCHAR *simple_ssml =
    L"<speak version='1.0' xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='en-US'>"
         "Hello, how are you doing today?"
     "</speak>";
    static const WCHAR *simple_synth_text = L"Hello, how are you doing today?";
    static const WCHAR *speech_synthesizer_name = L"Windows.Media.SpeechSynthesis.SpeechSynthesizer";
    static const WCHAR *speech_synthesizer_name2 = L"windows.media.speechsynthesis.speechsynthesizer";
    static const WCHAR *unknown_class_name = L"Unknown.Class";
    IActivationFactory *factory = NULL, *factory2 = NULL;
    IAsyncOperation_SpeechSynthesisStream *operation_ss_stream = NULL;
    IVectorView_IMediaMarker *media_markers = NULL;
    IVectorView_VoiceInformation *voices = NULL;
    IInstalledVoicesStatic *voices_static = NULL;
    ISpeechSynthesisStream *ss_stream = NULL, *tmp;
    IVoiceInformation *voice;
    IInspectable *inspectable = NULL, *tmp_inspectable = NULL;
    IAgileObject *agile_object = NULL, *tmp_agile_object = NULL;
    ISpeechSynthesizer *synthesizer;
    ISpeechSynthesizer2 *synthesizer2;
    IClosable *closable;
    struct async_inspectable_handler async_inspectable_handler;
    HMODULE hdll;
    HSTRING str, str2;
    HRESULT hr;
    UINT32 size;
    ULONG ref;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#lx\n", hr);

    hr = WindowsCreateString(speech_synthesizer_name, wcslen(speech_synthesizer_name), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx\n", hr);

    hdll = LoadLibraryW(L"windows.media.speech.dll");
    if (hdll)
    {
        pDllGetActivationFactory = (void *)GetProcAddress(hdll, "DllGetActivationFactory");
        ok(!!pDllGetActivationFactory, "DllGetActivationFactory not found.\n");

        hr = WindowsCreateString(unknown_class_name, wcslen(unknown_class_name), &str2);
        ok(hr == S_OK, "WindowsCreateString failed, hr %#lx\n", hr);

        hr = pDllGetActivationFactory(str2, &factory);
        ok(hr == CLASS_E_CLASSNOTAVAILABLE, "Got unexpected hr %#lx.\n", hr);

        WindowsDeleteString(str2);

        hr = WindowsCreateString(speech_synthesizer_name2, wcslen(speech_synthesizer_name2), &str2);
        ok(hr == S_OK, "WindowsCreateString failed, hr %#lx\n", hr);

        hr = pDllGetActivationFactory(str2, &factory2);
        ok(hr == CLASS_E_CLASSNOTAVAILABLE, "Got unexpected hr %#lx.\n", hr);

        WindowsDeleteString(str2);

        hr = pDllGetActivationFactory(str, &factory2);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    }
    else
    {
        win_skip("Failed to load library, err %lu.\n", GetLastError());
    }

    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK, "RoGetActivationFactory failed, hr %#lx\n", hr);

    if (hdll)
    {
        ok(factory == factory2, "Got unexpected factory %p, factory2 %p.\n", factory, factory2);
        IActivationFactory_Release(factory2);
        FreeLibrary(hdll);
    }

    /* Test static Synth ifaces: IActivationFactory, IInstalledVoicesStatic, etc. */
    hr = IActivationFactory_QueryInterface(factory, &IID_IInspectable, (void **)&inspectable);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IInspectable failed, hr %#lx\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IAgileObject, (void **)&agile_object);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IAgileObject failed, hr %#lx\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IInstalledVoicesStatic, (void **)&voices_static);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IInstalledVoicesStatic failed, hr %#lx\n", hr);

    hr = IInstalledVoicesStatic_QueryInterface(voices_static, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "IInstalledVoicesStatic_QueryInterface IID_IInspectable failed, hr %#lx\n", hr);
    ok(tmp_inspectable == inspectable, "IInstalledVoicesStatic_QueryInterface IID_IInspectable returned %p, expected %p\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = IInstalledVoicesStatic_QueryInterface(voices_static, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "IInstalledVoicesStatic_QueryInterface IID_IAgileObject failed, hr %#lx\n", hr);
    ok(tmp_agile_object == agile_object, "IInstalledVoicesStatic_QueryInterface IID_IAgileObject returned %p, expected %p\n", tmp_agile_object, agile_object);
    IAgileObject_Release(tmp_agile_object);

    hr = IInstalledVoicesStatic_get_AllVoices(voices_static, &voices);
    ok(hr == S_OK, "IInstalledVoicesStatic_get_AllVoices failed, hr %#lx\n", hr);

    hr = IVectorView_VoiceInformation_QueryInterface(voices, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "IVectorView_VoiceInformation_QueryInterface voices failed, hr %#lx\n", hr);
    ok(tmp_inspectable != inspectable, "IVectorView_VoiceInformation_QueryInterface voices returned %p, expected %p\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = IVectorView_VoiceInformation_QueryInterface(voices, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == E_NOINTERFACE, "IVectorView_VoiceInformation_QueryInterface voices failed, hr %#lx\n", hr);

    size = 0xdeadbeef;
    hr = IVectorView_VoiceInformation_get_Size(voices, &size);
    ok(hr == S_OK, "IVectorView_VoiceInformation_get_Size voices failed, hr %#lx\n", hr);
    todo_wine ok(size != 0 && size != 0xdeadbeef, "IVectorView_VoiceInformation_get_Size returned %u\n", size);

    voice = (IVoiceInformation *)0xdeadbeef;
    hr = IVectorView_VoiceInformation_GetAt(voices, size, &voice);
    ok(hr == E_BOUNDS, "IVectorView_VoiceInformation_GetAt failed, hr %#lx\n", hr);
    ok(voice == NULL, "IVectorView_VoiceInformation_GetAt returned %p\n", voice);

    hr = IVectorView_VoiceInformation_GetMany(voices, size, 1, &voice, &size);
    ok(hr == S_OK, "IVectorView_VoiceInformation_GetMany failed, hr %#lx\n", hr);
    ok(size == 0, "IVectorView_VoiceInformation_GetMany returned count %u\n", size);

    IVectorView_VoiceInformation_Release(voices);

    hr = IInstalledVoicesStatic_get_DefaultVoice(voices_static, &voice);
    todo_wine ok(hr == S_OK, "IInstalledVoicesStatic_get_DefaultVoice failed, hr %#lx\n", hr);

    if (hr == S_OK)
    {
        IVoiceInformation_get_Description(voice, &str2);
        trace("SpeechSynthesizer default voice %s.\n", debugstr_hstring(str2));

        WindowsDeleteString(str2);
        ref = IVoiceInformation_Release(voice);
        ok(ref == 0, "Got unexpected ref %lu.\n", ref);
    }

    IInstalledVoicesStatic_Release(voices_static);
    IAgileObject_Release(agile_object);
    IInspectable_Release(inspectable);

    /* Test Synthesizer */
    hr = IActivationFactory_QueryInterface(factory, &IID_ISpeechSynthesizer, (void **)&synthesizer);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);

    hr = RoActivateInstance(str, &inspectable);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    WindowsDeleteString(str);

    hr = IInspectable_QueryInterface(inspectable, &IID_ISpeechSynthesizer, (void **)&synthesizer);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Test SynthesizeTextToStreamAsync */
    hr = WindowsCreateString(simple_synth_text, wcslen(simple_synth_text), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx\n", hr);

    hr = ISpeechSynthesizer_SynthesizeTextToStreamAsync(synthesizer, NULL, &operation_ss_stream);
    ok(hr == S_OK, "ISpeechSynthesizer_SynthesizeTextToStreamAsync failed, hr %#lx\n", hr);
    IAsyncOperation_SpeechSynthesisStream_Release(operation_ss_stream);

    hr = ISpeechSynthesizer_SynthesizeTextToStreamAsync(synthesizer, str, &operation_ss_stream);
    ok(hr == S_OK, "ISpeechSynthesizer_SynthesizeTextToStreamAsync failed, hr %#lx\n", hr);

    await_async_inspectable((IAsyncOperation_IInspectable *)operation_ss_stream,
                             &async_inspectable_handler,
                             &IID_IAsyncOperationCompletedHandler_SpeechSynthesisStream);
    check_async_info((IInspectable *)operation_ss_stream, 2, Completed, S_OK);
    check_interface(operation_ss_stream, &IID_IAsyncOperation_SpeechSynthesisStream, TRUE);
    check_interface(operation_ss_stream, &IID_IAgileObject, TRUE);

    hr = IAsyncOperation_SpeechSynthesisStream_GetResults(operation_ss_stream, &ss_stream);
    ok(hr == S_OK, "IAsyncOperation_SpeechSynthesisStream_GetResults failed, hr %#lx\n", hr);

    tmp = (void *)0xdeadbeef;
    hr = IAsyncOperation_SpeechSynthesisStream_GetResults(operation_ss_stream, &tmp);
    ok(hr == S_OK, "IAsyncOperation_SpeechSynthesisStream_GetResults failed, hr %#lx\n", hr);
    todo_wine ok(tmp == NULL, "Got %p.\n", tmp);
    if (tmp && tmp != (void *)0xdeadbeef) ISpeechSynthesisStream_Release(tmp);

    hr = ISpeechSynthesisStream_get_Markers(ss_stream, &media_markers);
    ok(hr == S_OK, "ISpeechSynthesisStream_get_Markers failed, hr %#lx\n", hr);
    check_interface(media_markers, &IID_IVectorView_IMediaMarker, TRUE);
    check_interface(media_markers, &IID_IIterable_IMediaMarker, TRUE);
    check_interface(media_markers, &IID_IAgileObject, TRUE);

    ref = IVectorView_IMediaMarker_Release(media_markers);
    ok(ref == 0, "Got unexpected ref %lu.\n", ref);

    ref = ISpeechSynthesisStream_Release(ss_stream);
    todo_wine ok(ref == 0, "Got unexpected ref %lu.\n", ref);

    IAsyncOperation_SpeechSynthesisStream_Release(operation_ss_stream);

    /* Test SynthesizeSsmlToStreamAsync */
    hr = WindowsCreateString(simple_ssml, wcslen(simple_ssml), &str2);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx\n", hr);

    hr = ISpeechSynthesizer_SynthesizeSsmlToStreamAsync(synthesizer, str2, &operation_ss_stream);
    ok(hr == S_OK, "ISpeechSynthesizer_SynthesizeSsmlToStreamAsync failed, hr %#lx\n", hr);

    await_async_inspectable((IAsyncOperation_IInspectable *)operation_ss_stream,
                             &async_inspectable_handler,
                             &IID_IAsyncOperationCompletedHandler_SpeechSynthesisStream);
    check_async_info((IInspectable *)operation_ss_stream, 3, Completed, S_OK);
    check_interface(operation_ss_stream, &IID_IAsyncOperation_SpeechSynthesisStream, TRUE);
    check_interface(operation_ss_stream, &IID_IAgileObject, TRUE);

    hr = IAsyncOperation_SpeechSynthesisStream_GetResults(operation_ss_stream, &ss_stream);
    ok(hr == S_OK, "IAsyncOperation_SpeechSynthesisStream_GetResults failed, hr %#lx\n", hr);
    check_interface(ss_stream, &IID_ISpeechSynthesisStream, TRUE);
    check_interface(ss_stream, &IID_IAgileObject, TRUE);

    ref = ISpeechSynthesisStream_Release(ss_stream);
    todo_wine ok(ref == 0, "Got unexpected ref %lu.\n", ref);

    IAsyncOperation_SpeechSynthesisStream_Release(operation_ss_stream);

    operation_ss_stream = (void *)0xdeadbeef;
    hr = ISpeechSynthesizer_SynthesizeSsmlToStreamAsync(synthesizer, NULL, &operation_ss_stream);
    /* Broken on Win 8 + 8.1 */
    ok(hr == S_OK || broken(hr == E_INVALIDARG), "ISpeechSynthesizer_SynthesizeSsmlToStreamAsync failed, hr %#lx\n", hr);

    if (hr == S_OK)
    {
        ok(!!operation_ss_stream, "operation_ss_stream had value %p.\n", operation_ss_stream);
        IAsyncOperation_SpeechSynthesisStream_Release(operation_ss_stream);
    }
    else ok(operation_ss_stream == NULL, "operation_ss_stream had value %p.\n", operation_ss_stream);

    operation_ss_stream = (void *)0xdeadbeef;
    hr = ISpeechSynthesizer_SynthesizeSsmlToStreamAsync(synthesizer, str, &operation_ss_stream);
    /* Broken on Win 8 + 8.1 */
    ok(hr == S_OK || broken(hr == SPERR_WINRT_INCORRECT_FORMAT), "ISpeechSynthesizer_SynthesizeSsmlToStreamAsync failed, hr %#lx\n", hr);

    if (hr == S_OK)
    {
        ok(!!operation_ss_stream, "operation_ss_stream had value %p.\n", operation_ss_stream);
        IAsyncOperation_SpeechSynthesisStream_Release(operation_ss_stream);
    }
    else ok(operation_ss_stream == NULL, "operation_ss_stream had value %p.\n", operation_ss_stream);

    WindowsDeleteString(str2);
    WindowsDeleteString(str);

    hr = IInspectable_QueryInterface(inspectable, &IID_IClosable, (void **)&closable);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Test ISpeechSynthesizer2 iface */
    hr = IInspectable_QueryInterface(inspectable, &IID_ISpeechSynthesizer2, (void **)&synthesizer2);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE), "Got unexpected hr %#lx.\n", hr); /* Requires Win10 >= 1703 */

    if (hr == S_OK)
    {
        ISpeechSynthesizerOptions *options;

        hr = ISpeechSynthesizer2_get_Options(synthesizer2, &options);
        todo_wine ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        if (hr == S_OK)
        {
            ISpeechSynthesizerOptions3 *options3;

            check_interface(options, &IID_IAgileObject, TRUE);
            check_optional_interface(options, &IID_ISpeechSynthesizerOptions2, TRUE); /* Requires Win10 >= 1709 */

            hr = ISpeechSynthesizerOptions_QueryInterface(options, &IID_ISpeechSynthesizerOptions3, (void **)&options3);
            ok(hr == S_OK || broken(hr == E_NOINTERFACE), "Got unexpected hr %#lx.\n", hr); /* Requires Win10 >= 1803 */

            if (hr == S_OK)
            {
                ref = ISpeechSynthesizerOptions3_Release(options3);
                ok(ref == 2, "Got unexpected ref %lu.\n", ref);
            }
            else
                is_win10_1709 = TRUE;

            ref = ISpeechSynthesizerOptions_Release(options);
            ok(ref == 1, "Got unexpected ref %lu.\n", ref);
        }

        ref = ISpeechSynthesizer2_Release(synthesizer2);
        ok(ref == 3, "Got unexpected ref %lu.\n", ref);
    }

    ref = IClosable_Release(closable);
    ok(ref == 2, "Got unexpected ref %lu.\n", ref);

    ref = ISpeechSynthesizer_Release(synthesizer);
    ok(ref == 1, "Got unexpected ref %lu.\n", ref);

    ref = IInspectable_Release(inspectable);
    ok(!ref, "Got unexpected ref %lu.\n", ref);

    IActivationFactory_Release(factory);
    RoUninitialize();
}

static void test_VoiceInformation(void)
{
    static const WCHAR *voice_information_name = L"Windows.Media.SpeechSynthesis.VoiceInformation";

    IActivationFactory *factory = NULL;
    HSTRING str;
    HRESULT hr;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#lx\n", hr);

    hr = WindowsCreateString(voice_information_name, wcslen(voice_information_name), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx\n", hr);

    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == REGDB_E_CLASSNOTREG, "RoGetActivationFactory returned unexpected hr %#lx\n", hr);

    WindowsDeleteString(str);

    RoUninitialize();
}

struct put_completed_thread_param
{
    IAsyncOperationCompletedHandler_IInspectable *handler;
    IAsyncOperation_IInspectable *operation;
};

static DWORD WINAPI put_completed_thread(void *arg)
{
    struct put_completed_thread_param *param = arg;
    HRESULT hr;

    hr = IAsyncOperation_IInspectable_put_Completed(param->operation, param->handler);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    return 0;
}

static void test_SpeechRecognizer(void)
{
    static const WCHAR *speech_recognition_name = L"Windows.Media.SpeechRecognition.SpeechRecognizer";
    IAsyncOperationCompletedHandler_SpeechRecognitionCompilationResult *handler = NULL;
    IAsyncOperation_SpeechRecognitionCompilationResult *operation = NULL;
    ISpeechRecognitionCompilationResult *compilation_result = NULL;
    IVector_ISpeechRecognitionConstraint *constraints = NULL;
    ISpeechContinuousRecognitionSession *session = NULL;
    ISpeechRecognizerFactory *sr_factory = NULL;
    ISpeechRecognizerStatics *sr_statics = NULL;
    ISpeechRecognizerStatics2 *sr_statics2 = NULL;
    ISpeechRecognizer *recognizer = NULL;
    ISpeechRecognizer2 *recognizer2 = NULL;
    IActivationFactory *factory = NULL;
    IInspectable *inspectable = NULL;
    ILanguage *language = NULL;
    IAsyncInfo *info = NULL;
    struct put_completed_thread_param put_completed_param;
    struct async_inspectable_handler compilation_handler;
    struct completed_event_handler completed_handler;
    struct recognition_result_handler result_handler;
    SpeechRecognitionResultStatus result_status;
    EventRegistrationToken token = { .value = 0 };
    AsyncStatus async_status;
    HSTRING hstr, hstr_lang;
    HANDLE put_thread;
    LONG ref, old_ref;
    HRESULT hr;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#lx.\n", hr);

    hr = WindowsCreateString(speech_recognition_name, wcslen(speech_recognition_name), &hstr);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx.n", hr);

    hr = RoGetActivationFactory(hstr, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "RoGetActivationFactory failed, hr %#lx.\n", hr);

    if (hr == REGDB_E_CLASSNOTREG) /* Win 8 and 8.1 */
    {
        win_skip("SpeechRecognizer activation factory not available!\n");
        goto done;
    }

    check_interface(factory, &IID_IAgileObject, TRUE);

    hr = IActivationFactory_QueryInterface(factory, &IID_ISpeechRecognizerFactory, (void **)&sr_factory);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_ISpeechRecognizer failed, hr %#lx.\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_ISpeechRecognizerStatics, (void **)&sr_statics);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_ISpeechRecognizerStatics failed, hr %#lx.\n", hr);

    hr = ISpeechRecognizerStatics_get_SystemSpeechLanguage(sr_statics, &language);
    todo_wine ok(hr == S_OK, "ISpeechRecognizerStatics_SystemSpeechLanguage failed, hr %#lx.\n", hr);

    if (hr == S_OK)
    {
        hr = ILanguage_get_LanguageTag(language, &hstr_lang);
        ok(hr == S_OK, "ILanguage_get_LanguageTag failed, hr %#lx.\n", hr);

        trace("SpeechRecognizer default language %s.\n", debugstr_hstring(hstr_lang));

        ILanguage_Release(language);
    }

    ref = ISpeechRecognizerStatics_Release(sr_statics);
    ok(ref == 3, "Got unexpected ref %lu.\n", ref);

    hr = IActivationFactory_QueryInterface(factory, &IID_ISpeechRecognizerStatics2, (void **)&sr_statics2);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE), "IActivationFactory_QueryInterface IID_ISpeechRecognizerStatics2 failed, hr %#lx.\n", hr);

    if (hr == S_OK) /* SpeechRecognizerStatics2 not implemented on Win10 1507 */
    {
        ref = ISpeechRecognizerStatics2_Release(sr_statics2);
        ok(ref == 3, "Got unexpected ref %lu.\n", ref);
    }

    ref = ISpeechRecognizerFactory_Release(sr_factory);
    ok(ref == 2, "Got unexpected ref %lu.\n", ref);

    ref = IActivationFactory_Release(factory);
    ok(ref == 1, "Got unexpected ref %lu.\n", ref);

    hr = RoActivateInstance(hstr, &inspectable);
    ok(hr == S_OK || broken(hr == SPERR_WINRT_INTERNAL_ERROR), "Got unexpected hr %#lx.\n", hr);

    if (hr == S_OK)
    {
        check_refcount(inspectable, 1);

        hr = IInspectable_QueryInterface(inspectable, &IID_ISpeechRecognizer, (void **)&recognizer);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IInspectable_QueryInterface(inspectable, &IID_ISpeechRecognizer2, (void **)&recognizer2);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        check_interface(inspectable, &IID_IClosable, TRUE);

        hr = ISpeechRecognizer2_get_ContinuousRecognitionSession(recognizer2, &session);
        ok(hr == S_OK, "ISpeechRecognizer2_get_ContinuousRecognitionSession failed, hr %#lx.\n", hr);
        check_refcount(session, 2);
        check_refcount(inspectable, 3);

        ref = ISpeechRecognizer2_Release(recognizer2);
        ok(ref == 2, "Got unexpected ref %lu.\n", ref);

        hr = ISpeechContinuousRecognitionSession_add_Completed(session, NULL, &token);
        ok(hr == E_INVALIDARG, "ISpeechContinuousRecognitionSession_add_ResultGenerated failed, hr %#lx.\n", hr);

        token.value = 0xdeadbeef;
        completed_event_handler_create_static(&completed_handler);
        hr = ISpeechContinuousRecognitionSession_add_Completed(session, &completed_handler.IHandler_RecognitionCompleted_iface, &token);
        ok(hr == S_OK, "ISpeechContinuousRecognitionSession_add_ResultGenerated failed, hr %#lx.\n", hr);
        ok(token.value != 0xdeadbeef, "Got unexpected token: %#I64x.\n", token.value);

        hr = ISpeechContinuousRecognitionSession_remove_Completed(session, token);
        ok(hr == S_OK, "ISpeechContinuousRecognitionSession_remove_ResultGenerated failed, hr %#lx.\n", hr);

        hr = ISpeechContinuousRecognitionSession_add_ResultGenerated(session, NULL, &token);
        ok(hr == E_INVALIDARG, "ISpeechContinuousRecognitionSession_add_ResultGenerated failed, hr %#lx.\n", hr);

        token.value = 0xdeadbeef;
        recognition_result_handler_create_static(&result_handler);
        hr = ISpeechContinuousRecognitionSession_add_ResultGenerated(session, &result_handler.IHandler_RecognitionResult_iface, &token);
        ok(hr == S_OK, "ISpeechContinuousRecognitionSession_add_ResultGenerated failed, hr %#lx.\n", hr);
        ok(token.value != 0xdeadbeef, "Got unexpected token: %#I64x.\n", token.value);

        hr = ISpeechContinuousRecognitionSession_remove_ResultGenerated(session, token);
        ok(hr == S_OK, "ISpeechContinuousRecognitionSession_remove_ResultGenerated failed, hr %#lx.\n", hr);

        ref = ISpeechContinuousRecognitionSession_Release(session);
        ok(ref == 1, "Got unexpected ref %lu.\n", ref);

        hr = ISpeechRecognizer_get_Constraints(recognizer, &constraints);
        ok(hr == S_OK, "ISpeechContinuousRecognitionSession_get_Constraints failed, hr %#lx.\n", hr);

        ref = IVector_ISpeechRecognitionConstraint_Release(constraints);
        ok(ref == 1, "Got unexpected ref %lu.\n", ref);

        hr = ISpeechRecognizer_CompileConstraintsAsync(recognizer, &operation);
        ok(hr == S_OK, "ISpeechRecognizer_CompileConstraintsAsync failed, hr %#lx.\n", hr);

        handler = (void*)0xdeadbeef;
        hr = IAsyncOperation_SpeechRecognitionCompilationResult_get_Completed(operation, &handler);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(handler == NULL, "Handler had value %p.\n", handler);

        if (FAILED(hr)) goto skip_operation;

        compilation_result = (void*)0xdeadbeef;
        hr = IAsyncOperation_SpeechRecognitionCompilationResult_GetResults(operation, &compilation_result);
        ok(hr == E_ILLEGAL_METHOD_CALL || hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        if (hr == E_ILLEGAL_METHOD_CALL) /* Sometimes the operation could have already finished here, */
                                         /* if so skip waiting and getting the results a second time. */
        {
            ok(compilation_result == (void*)0xdeadbeef, "Compilation result had value %p.\n", compilation_result);

            await_async_inspectable((IAsyncOperation_IInspectable *)operation,
                                     &compilation_handler,
                                     &IID_IAsyncOperationCompletedHandler_SpeechRecognitionCompilationResult);

            hr = IAsyncOperation_SpeechRecognitionCompilationResult_put_Completed(operation, NULL);
            ok(hr == E_ILLEGAL_DELEGATE_ASSIGNMENT, "Got unexpected hr %#lx.\n", hr);

            hr = IAsyncOperation_SpeechRecognitionCompilationResult_get_Completed(operation, &handler);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            compilation_result = (void*)0xdeadbeef;
            hr = IAsyncOperation_SpeechRecognitionCompilationResult_GetResults(operation, &compilation_result);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        }

        ok(compilation_result != (void*)0xdeadbeef, "Compilation result had value %p.\n", compilation_result);
        check_interface(compilation_result, &IID_IAgileObject, TRUE);

        check_async_info((IInspectable *)operation, 1, Completed, S_OK);

        hr = ISpeechRecognitionCompilationResult_get_Status(compilation_result, &result_status);
        ok(hr == S_OK, "ISpeechRecognitionCompilationResult_get_Status failed, hr %#lx.\n", hr);
        ok(result_status == SpeechRecognitionResultStatus_Success, "Got unexpected status %#x.\n", result_status);

        ref = ISpeechRecognitionCompilationResult_Release(compilation_result);
        todo_wine ok(!ref , "Got unexpected ref %lu.\n", ref);

        compilation_result = (void *)0xdeadbeef;
        hr = IAsyncOperation_SpeechRecognitionCompilationResult_GetResults(operation, &compilation_result);
        todo_wine ok(hr == E_UNEXPECTED, "Got unexpected hr %#lx.\n", hr);
        todo_wine ok(compilation_result == NULL, "Got %p.\n", compilation_result);
        if (compilation_result && compilation_result != (void *)0xdeadbeef) ISpeechRecognitionCompilationResult_Release(compilation_result);

        hr = IAsyncOperation_SpeechRecognitionCompilationResult_QueryInterface(operation, &IID_IAsyncInfo, (void **)&info);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IAsyncInfo_Cancel(info);
        ok(hr == S_OK, "IAsyncInfo_Cancel failed, hr %#lx.\n", hr);
        check_async_info((IInspectable *)operation, 1, Completed, S_OK);

        hr = IAsyncInfo_Close(info);
        ok(hr == S_OK, "IAsyncInfo_Close failed, hr %#lx.\n", hr);
        check_async_info((IInspectable *)operation, 1, AsyncStatus_Closed, S_OK);

        hr = IAsyncInfo_Close(info);
        ok(hr == S_OK, "IAsyncInfo_Close failed, hr %#lx.\n", hr);
        check_async_info((IInspectable *)operation, 1, AsyncStatus_Closed, S_OK);

        IAsyncInfo_Release(info);

        hr = IAsyncOperation_SpeechRecognitionCompilationResult_put_Completed(operation, NULL);
        ok(hr == E_ILLEGAL_METHOD_CALL, "Got unexpected hr %#lx.\n", hr);

        handler = (void*)0xdeadbeef;
        hr = IAsyncOperation_SpeechRecognitionCompilationResult_get_Completed(operation, &handler);
        ok(hr == E_ILLEGAL_METHOD_CALL, "Got unexpected hr %#lx.\n", hr);

        IAsyncOperation_SpeechRecognitionCompilationResult_Release(operation);

        /* Test if AsyncOperation is started immediately. */
        hr = ISpeechRecognizer_CompileConstraintsAsync(recognizer, &operation);
        ok(hr == S_OK, "ISpeechRecognizer_CompileConstraintsAsync failed, hr %#lx.\n", hr);
        check_interface(operation, &IID_IAgileObject, TRUE);

        hr = IAsyncOperation_SpeechRecognitionCompilationResult_QueryInterface(operation, &IID_IAsyncInfo, (void **)&info);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        async_status = 0xdeadbeef;
        hr = IAsyncInfo_get_Status(info, &async_status);
        ok(hr == S_OK, "IAsyncInfo_get_Status failed, hr %#lx.\n", hr);
        ok(async_status != AsyncStatus_Closed, "Status was %#x.\n", async_status);
        IAsyncInfo_Release(info);

        await_async_inspectable((IAsyncOperation_IInspectable *)operation,
                                 &compilation_handler,
                                 &IID_IAsyncOperationCompletedHandler_SpeechRecognitionCompilationResult);
        check_async_info((IInspectable *)operation, 2, Completed, S_OK);

        IAsyncOperation_SpeechRecognitionCompilationResult_Release(operation);

        ref = ISpeechRecognizer_Release(recognizer);
        ok(ref == 1, "Got unexpected ref %lu.\n", ref);

        ref = IInspectable_Release(inspectable);
        ok(!ref, "Got unexpected ref %lu.\n", ref);

        /* Test if AsyncInfo_Close waits for the handler to finish. */
        hr = RoActivateInstance(hstr, &inspectable);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IInspectable_QueryInterface(inspectable, &IID_ISpeechRecognizer, (void **)&recognizer);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        async_inspectable_handler_create_static(&compilation_handler, &IID_IAsyncOperationCompletedHandler_SpeechRecognitionCompilationResult);
        compilation_handler.event_block = CreateEventW(NULL, FALSE, FALSE, NULL);
        compilation_handler.event_finished = CreateEventW(NULL, FALSE, FALSE, NULL);

        ok(!!compilation_handler.event_block, "event_block wasn't created.\n");
        ok(!!compilation_handler.event_finished, "event_finished wasn't created.\n");

        hr = ISpeechRecognizer_CompileConstraintsAsync(recognizer, &operation);
        ok(hr == S_OK, "ISpeechRecognizer_CompileConstraintsAsync failed, hr %#lx.\n", hr);

        put_completed_param.handler = &compilation_handler.IAsyncHandler_IInspectable_iface;
        put_completed_param.operation = (IAsyncOperation_IInspectable *)operation;
        put_thread = CreateThread(NULL, 0, put_completed_thread, &put_completed_param, 0, NULL);

        ok(!WaitForSingleObject(compilation_handler.event_finished, 5000), "Wait for event_finished failed.\n");

        todo_wine ok(compilation_handler.ref == 3, "Got unexpected ref %lu.\n", compilation_handler.ref);

        handler = (void*)0xdeadbeef;
        old_ref = compilation_handler.ref;
        hr = IAsyncOperation_SpeechRecognitionCompilationResult_get_Completed(operation, &handler);
        ok(hr == S_OK, "IAsyncOperation_SpeechRecognitionCompilationResult_get_Completed failed, hr %#lx.\n", hr);
        todo_wine ok(handler == (void *)&compilation_handler.IAsyncHandler_IInspectable_iface, "Handler was %p.\n", handler);

        ref = compilation_handler.ref - old_ref;
        todo_wine ok(ref == 1, "The ref was increased by %lu.\n", ref);
        if (handler) IAsyncOperationCompletedHandler_SpeechRecognitionCompilationResult_Release(handler);

        hr = IAsyncOperation_SpeechRecognitionCompilationResult_QueryInterface(operation, &IID_IAsyncInfo, (void **)&info);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IAsyncInfo_Close(info); /* If IAsyncInfo_Close would wait for the handler to finish, the test would get stuck here. */
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        SetEvent(compilation_handler.event_block);
        ok(!WaitForSingleObject(put_thread, 1000), "Wait for block_thread failed.\n");
        check_async_info((IInspectable *)operation, 1, AsyncStatus_Closed, S_OK);

        CloseHandle(put_thread);
        CloseHandle(compilation_handler.event_block);
        CloseHandle(compilation_handler.event_finished);

        IAsyncInfo_Release(info);

skip_operation:
        IAsyncOperation_SpeechRecognitionCompilationResult_Release(operation);

        ref = ISpeechRecognizer_Release(recognizer);
        ok(ref == 1, "Got unexpected ref %lu.\n", ref);

        ref = IInspectable_Release(inspectable);
        ok(!ref, "Got unexpected ref %lu.\n", ref);
    }
    else if (hr == SPERR_WINRT_INTERNAL_ERROR) /* Not sure when this triggers. Probably if a language pack is not installed. */
    {
        win_skip("Could not init SpeechRecognizer with default language!\n");
    }

done:
    WindowsDeleteString(hstr);

    RoUninitialize();
}

static void test_SpeechRecognitionListConstraint(void)
{
    static const WCHAR *speech_recognition_list_constraint_name = L"Windows.Media.SpeechRecognition.SpeechRecognitionListConstraint";
    static const WCHAR *speech_constraints[] = { L"This is a test.", L"Number 5!", L"What time is it?" };
    static const WCHAR *speech_constraint_tag = L"test_message";
    ISpeechRecognitionListConstraintFactory *listconstraint_factory = NULL;
    ISpeechRecognitionListConstraint *listconstraint = NULL;
    ISpeechRecognitionConstraint *constraint = NULL;
    IVector_HSTRING *hstring_vector = NULL;
    IActivationFactory *factory = NULL;
    IInspectable *inspectable = NULL;
    struct iterator_hstring iterator_hstring;
    struct iterable_hstring iterable_hstring;
    HSTRING commands[3], str, tag, tag_out;
    UINT32 i, vector_size = 0;
    BOOLEAN enabled;
    INT32 str_cmp;
    HRESULT hr;
    LONG ref;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#lx.\n", hr);

    hr = WindowsCreateString(speech_recognition_list_constraint_name, wcslen(speech_recognition_list_constraint_name), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx.\n", hr);

    hr = WindowsCreateString(speech_constraint_tag, wcslen(speech_constraint_tag), &tag);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(commands); i++)
    {
        hr = WindowsCreateString(speech_constraints[i], wcslen(speech_constraints[i]), &commands[i]);
        ok(hr == S_OK, "WindowsCreateString failed, hr %#lx.\n", hr);
    }

    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "RoGetActivationFactory failed, hr %#lx.\n", hr);

    if (hr == REGDB_E_CLASSNOTREG) /* Win 8 and 8.1 */
    {
        win_skip("SpeechRecognitionListConstraint activation factory not available!\n");
        goto done;
    }

    hr = IActivationFactory_ActivateInstance(factory, &inspectable);
    ok(hr == E_NOTIMPL, "IActivationFactory_ActivateInstance failed, hr %#lx.\n", hr);

    check_refcount(factory, 2);
    check_interface(factory, &IID_IInspectable, TRUE);
    check_interface(factory, &IID_IAgileObject, TRUE);

    hr = IActivationFactory_QueryInterface(factory, &IID_ISpeechRecognitionListConstraintFactory, (void **)&listconstraint_factory);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_ISpeechRecognitionListConstraintFactory failed, hr %#lx.\n", hr);

    hr = ISpeechRecognitionListConstraintFactory_Create(listconstraint_factory, NULL, &listconstraint);
    ok(hr == E_POINTER, "ISpeechRecognitionListConstraintFactory_Create failed, hr %#lx.\n", hr);

    hr = ISpeechRecognitionListConstraintFactory_CreateWithTag(listconstraint_factory, NULL, NULL, &listconstraint);
    ok(hr == E_POINTER, "ISpeechRecognitionListConstraintFactory_Create failed, hr %#lx.\n", hr);

    iterator_hstring_create_static(&iterator_hstring, commands, ARRAY_SIZE(commands));
    iterable_hstring_create_static(&iterable_hstring, &iterator_hstring);

    hr = ISpeechRecognitionListConstraintFactory_CreateWithTag(listconstraint_factory, &iterable_hstring.IIterable_HSTRING_iface, NULL, &listconstraint);
    ok(hr == S_OK, "ISpeechRecognitionListConstraintFactory_Create failed, hr %#lx.\n", hr);

    ref = ISpeechRecognitionListConstraint_Release(listconstraint);
    ok(ref == 0, "Got unexpected ref %lu.\n", ref);

    iterator_hstring_create_static(&iterator_hstring, commands, ARRAY_SIZE(commands));
    iterable_hstring_create_static(&iterable_hstring, &iterator_hstring);

    hr = ISpeechRecognitionListConstraintFactory_CreateWithTag(listconstraint_factory, &iterable_hstring.IIterable_HSTRING_iface, tag, &listconstraint);
    ok(hr == S_OK, "ISpeechRecognitionListConstraintFactory_CreateWithTag failed, hr %#lx.\n", hr);

    check_refcount(listconstraint, 1);
    check_interface(listconstraint, &IID_IInspectable, TRUE);
    check_interface(listconstraint, &IID_IAgileObject, TRUE);

    hr = ISpeechRecognitionListConstraint_QueryInterface(listconstraint, &IID_ISpeechRecognitionConstraint, (void **)&constraint);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ISpeechRecognitionListConstraint_get_Commands(listconstraint, &hstring_vector);
    ok(hr == S_OK, "ISpeechRecognitionListConstraint_Commands failed, hr %#lx.\n", hr);

    hr = IVector_HSTRING_get_Size(hstring_vector, &vector_size);
    ok(hr == S_OK, "IVector_HSTRING_get_Size failed, hr %#lx.\n", hr);
    ok(vector_size == ARRAY_SIZE(commands), "Got unexpected vector_size %u.\n", vector_size);

    for (i = 0; i < vector_size; i++)
    {
        HSTRING str;

        hr = IVector_HSTRING_GetAt(hstring_vector, i, &str);
        ok(hr == S_OK, "IVector_HSTRING_GetAt failed, hr %#lx.\n", hr);
        hr = WindowsCompareStringOrdinal(commands[i], str, &str_cmp);
        ok(hr == S_OK, "WindowsCompareStringOrdinal failed, hr %#lx.\n", hr);
        ok(!str_cmp, "Strings not equal.\n");

        WindowsDeleteString(str);
    }

    ref = IVector_HSTRING_Release(hstring_vector);
    ok(ref == 0, "Got unexpected ref %lu.\n", ref);

    hr = ISpeechRecognitionConstraint_get_Tag(constraint, &tag_out);
    todo_wine ok(hr == S_OK, "ISpeechRecognitionConstraint_get_Tag failed, hr %#lx.\n", hr);

    if (FAILED(hr))
        goto skip_tests;

    hr = WindowsCompareStringOrdinal(tag, tag_out, &str_cmp);
    todo_wine ok(hr == S_OK, "WindowsCompareStringOrdinal failed, hr %#lx.\n", hr);
    todo_wine ok(!str_cmp, "Strings not equal.\n");
    hr = WindowsDeleteString(tag_out);
    todo_wine ok(hr == S_OK, "WindowsDeleteString failed, hr %#lx.\n", hr);

skip_tests:
    hr = ISpeechRecognitionConstraint_put_IsEnabled(constraint, TRUE);
    ok(hr == S_OK, "ISpeechRecognitionConstraint_put_IsEnabled failed, hr %#lx.\n", hr);
    hr = ISpeechRecognitionConstraint_get_IsEnabled(constraint, &enabled);
    ok(hr == S_OK, "ISpeechRecognitionConstraint_get_IsEnabled failed, hr %#lx.\n", hr);
    ok(enabled, "ListConstraint didn't get enabled.\n");

    ref = ISpeechRecognitionConstraint_Release(constraint);
    ok(ref == 1, "Got unexpected ref %lu.\n", ref);

    ref = ISpeechRecognitionListConstraint_Release(listconstraint);
    ok(ref == 0, "Got unexpected ref %lu.\n", ref);

    ref = ISpeechRecognitionListConstraintFactory_Release(listconstraint_factory);
    ok(ref == 2, "Got unexpected ref %lu.\n", ref);

    ref = IActivationFactory_Release(factory);
    ok(ref == 1, "Got unexpected ref %lu.\n", ref);

done:
    WindowsDeleteString(str);
    WindowsDeleteString(tag);
    for (i = 0; i < ARRAY_SIZE(commands); i++)
        WindowsDeleteString(commands[i]);

    RoUninitialize();
}

struct action_put_completed_thread_param
{
    IAsyncActionCompletedHandler *handler;
    IAsyncAction *action;
};

static DWORD WINAPI action_put_completed_thread(void *arg)
{
    struct action_put_completed_thread_param *param = arg;
    HRESULT hr;

    hr = IAsyncAction_put_Completed(param->action, param->handler);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    return 0;
}

static void test_Recognition(void)
{
    static const WCHAR *list_constraint_name = L"Windows.Media.SpeechRecognition.SpeechRecognitionListConstraint";
    static const WCHAR *recognizer_name = L"Windows.Media.SpeechRecognition.SpeechRecognizer";
    static const WCHAR *speech_constraint_tag = L"test_message";
    static const WCHAR *speech_constraints[] = { L"This is a test.", L"Number 5!", L"What time is it?" };
    ISpeechRecognitionListConstraintFactory *listconstraint_factory = NULL;
    IAsyncOperation_SpeechRecognitionCompilationResult *operation = NULL;
    IVector_ISpeechRecognitionConstraint *constraints = NULL;
    ISpeechContinuousRecognitionSession *session = NULL;
    ISpeechRecognitionListConstraint *listconstraint = NULL;
    ISpeechRecognitionConstraint *constraint = NULL;
    ISpeechRecognizer *recognizer = NULL;
    ISpeechRecognizer2 *recognizer2 = NULL;
    IAsyncActionCompletedHandler *handler = NULL;
    IAsyncAction *action = NULL, *action2 = NULL;
    IInspectable *inspectable = NULL;
    IAsyncInfo *info = NULL;
    struct async_inspectable_handler compilation_handler;
    struct action_put_completed_thread_param put_param;
    struct recognition_result_handler result_handler;
    struct async_void_handler action_handler;
    struct iterator_hstring iterator_hstring;
    struct iterable_hstring iterable_hstring;
    EventRegistrationToken token = { .value = 0 };
    SpeechRecognizerState recog_state;
    HSTRING commands[3], hstr, tag;
    HANDLE put_thread;
    LONG ref, old_ref;
    HRESULT hr;
    UINT32 i;
    BOOL set;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(commands); i++)
    {
        hr = WindowsCreateString(speech_constraints[i], wcslen(speech_constraints[i]), &commands[i]);
        ok(hr == S_OK, "WindowsCreateString failed, hr %#lx.\n", hr);
    }

    hr = WindowsCreateString(recognizer_name, wcslen(recognizer_name), &hstr);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx.\n", hr);

    hr = RoActivateInstance(hstr, &inspectable);
    ok(hr == S_OK || broken(hr == SPERR_WINRT_INTERNAL_ERROR || hr == REGDB_E_CLASSNOTREG), "Got unexpected hr %#lx.\n", hr);
    WindowsDeleteString(hstr);

    if (FAILED(hr))  /* Win 8 and 8.1 and Win10 without enabled SR. */
    {
        win_skip("SpeechRecognizer cannot be activated!\n");
        goto done;
    }

    hr = WindowsCreateString(list_constraint_name, wcslen(list_constraint_name), &hstr);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx.\n", hr);

    hr = RoGetActivationFactory(hstr, &IID_ISpeechRecognitionListConstraintFactory, (void **)&listconstraint_factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "RoGetActivationFactory failed, hr %#lx.\n", hr);
    WindowsDeleteString(hstr);

    hr = WindowsCreateString(speech_constraint_tag, wcslen(speech_constraint_tag), &tag);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx.\n", hr);

    iterator_hstring_create_static(&iterator_hstring, commands, ARRAY_SIZE(commands));
    iterable_hstring_create_static(&iterable_hstring, &iterator_hstring);
    hr = ISpeechRecognitionListConstraintFactory_CreateWithTag(listconstraint_factory, &iterable_hstring.IIterable_HSTRING_iface, tag, &listconstraint);
    ok(hr == S_OK, "ISpeechRecognitionListConstraintFactory_Create failed, hr %#lx.\n", hr);
    WindowsDeleteString(tag);

    ref = ISpeechRecognitionListConstraintFactory_Release(listconstraint_factory);
    ok(ref == 1, "Got unexpected ref %lu.\n", ref);

    hr = ISpeechRecognitionListConstraint_QueryInterface(listconstraint, &IID_ISpeechRecognitionConstraint, (void **)&constraint);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ISpeechRecognitionConstraint_put_IsEnabled(constraint, TRUE);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IInspectable_QueryInterface(inspectable, &IID_ISpeechRecognizer, (void **)&recognizer);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IInspectable_QueryInterface(inspectable, &IID_ISpeechRecognizer2, (void **)&recognizer2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ISpeechRecognizer2_get_ContinuousRecognitionSession(recognizer2, &session);
    ok(hr == S_OK, "ISpeechRecognizer2_get_ContinuousRecognitionSession failed, hr %#lx.\n", hr);

    hr = ISpeechRecognizer_get_Constraints(recognizer, &constraints);
    ok(hr == S_OK, "ISpeechContinuousRecognitionSession_get_Constraints failed, hr %#lx.\n", hr);

    hr = IVector_ISpeechRecognitionConstraint_Clear(constraints);
    ok(hr == S_OK, "IVector_ISpeechRecognitionConstraint_Clear failed, hr %#lx.\n", hr);

    hr = IVector_ISpeechRecognitionConstraint_Append(constraints, constraint);
    ok(hr == S_OK, "IVector_ISpeechRecognitionConstraint_Append failed, hr %#lx.\n", hr);

    ref = IVector_ISpeechRecognitionConstraint_Release(constraints);
    ok(ref == 1, "Got unexpected ref %lu.\n", ref);

    ref = ISpeechRecognitionConstraint_Release(constraint);
    ok(ref == 2, "Got unexpected ref %lu.\n", ref);

    ref = ISpeechRecognitionListConstraint_Release(listconstraint);
    ok(ref == 1, "Got unexpected ref %lu.\n", ref);

    token.value = 0xdeadbeef;
    recognition_result_handler_create_static(&result_handler);
    hr = ISpeechContinuousRecognitionSession_add_ResultGenerated(session, &result_handler.IHandler_RecognitionResult_iface, &token);
    ok(hr == S_OK, "ISpeechContinuousRecognitionSession_add_ResultGenerated failed, hr %#lx.\n", hr);
    ok(token.value != 0xdeadbeef, "Got unexpected token: %#I64x.\n", token.value);

    recog_state = 0xdeadbeef;
    hr = ISpeechRecognizer2_get_State(recognizer2, &recog_state);
    ok(hr == S_OK, "ISpeechRecognizer2_get_State failed, hr %#lx.\n", hr);
    ok(recog_state == SpeechRecognizerState_Idle, "recog_state was %u.\n", recog_state);

    hr = ISpeechRecognizer_CompileConstraintsAsync(recognizer, &operation);
    ok(hr == S_OK, "ISpeechRecognizer_CompileConstraintsAsync failed, hr %#lx.\n", hr);
    await_async_inspectable((IAsyncOperation_IInspectable *)operation,
                             &compilation_handler,
                             &IID_IAsyncOperationCompletedHandler_SpeechRecognitionCompilationResult);

    IAsyncOperation_IInspectable_Release((IAsyncOperation_IInspectable *)operation);

    hr = ISpeechContinuousRecognitionSession_StartAsync(session, &action);
    ok(hr == S_OK, "ISpeechContinuousRecognitionSession_StartAsync failed, hr %#lx.\n", hr);

    handler = (void *)0xdeadbeef;
    hr = IAsyncAction_get_Completed(action, &handler);
    ok(hr == S_OK, "IAsyncAction_put_Completed failed, hr %#lx.\n", hr);
    ok(handler == NULL, "Handler was %p.\n", handler);

    await_async_void(action, &action_handler);

    action2 = (void *)0xdeadbeef;
    hr = ISpeechContinuousRecognitionSession_StartAsync(session, &action2);
    ok(hr == COR_E_INVALIDOPERATION, "ISpeechContinuousRecognitionSession_StartAsync failed, hr %#lx.\n", hr);
    ok(action2 == NULL, "action2 was %p.\n", action2);

    hr = IAsyncAction_QueryInterface(action, &IID_IAsyncInfo, (void **)&info);
    ok(hr == S_OK, "IAsyncAction_QueryInterface failed, hr %#lx.\n", hr);
    check_async_info((IInspectable *)action, 1, Completed, S_OK);

    hr = IAsyncInfo_Cancel(info);
    ok(hr == S_OK, "IAsyncInfo_Cancel failed, hr %#lx.\n", hr);
    check_async_info((IInspectable *)action, 1, Completed, S_OK);

    hr = IAsyncInfo_Close(info);
    ok(hr == S_OK, "IAsyncInfo_Close failed, hr %#lx.\n", hr);
    check_async_info((IInspectable *)action, 1, AsyncStatus_Closed, S_OK);

    hr = IAsyncInfo_Close(info);
    ok(hr == S_OK, "IAsyncInfo_Close failed, hr %#lx.\n", hr);
    check_async_info((IInspectable *)action, 1, AsyncStatus_Closed, S_OK);

    hr = IAsyncInfo_Cancel(info);
    todo_wine ok(hr == S_OK, "IAsyncInfo_Cancel failed, hr %#lx.\n", hr);
    check_async_info((IInspectable *)action, 1, AsyncStatus_Closed, S_OK);

    IAsyncInfo_Release(info);

    recog_state = 0xdeadbeef;
    hr = ISpeechRecognizer2_get_State(recognizer2, &recog_state);
    ok(hr == S_OK, "ISpeechRecognizer2_get_State failed, hr %#lx.\n", hr);
    ok(recog_state == SpeechRecognizerState_Capturing || broken(recog_state == SpeechRecognizerState_Idle), "recog_state was %u.\n", recog_state);

    /*
     * TODO: Use a loopback device together with prerecorded audio files to test the recognizer's functionality.
     */

    hr = ISpeechContinuousRecognitionSession_PauseAsync(session, &action2);
    ok(hr == S_OK, "ISpeechContinuousRecognitionSession_PauseAsync failed, hr %#lx.\n", hr);
    await_async_void(action2, &action_handler);
    check_async_info((IInspectable *)action2, 3, Completed, S_OK);
    IAsyncAction_Release(action2);

    recog_state = 0xdeadbeef;
    hr = ISpeechRecognizer2_get_State(recognizer2, &recog_state);
    ok(hr == S_OK, "ISpeechRecognizer2_get_State failed, hr %#lx.\n", hr);
    ok(recog_state == SpeechRecognizerState_Paused ||
       broken(recog_state == SpeechRecognizerState_Capturing) /* Broken on Win10 1507 */, "recog_state was %u.\n", recog_state);

    /* Check what happens if we try to pause again, when the session is already paused. */
    hr = ISpeechContinuousRecognitionSession_PauseAsync(session, &action2);
    ok(hr == S_OK, "ISpeechContinuousRecognitionSession_PauseAsync failed, hr %#lx.\n", hr);
    await_async_void(action2, &action_handler);
    check_async_info((IInspectable *)action2, 4, Completed, S_OK);
    IAsyncAction_Release(action2);

    hr = ISpeechContinuousRecognitionSession_Resume(session);
    ok(hr == S_OK, "ISpeechContinuousRecognitionSession_Resume failed, hr %#lx.\n", hr);

    /* Resume when already resumed. */
    hr = ISpeechContinuousRecognitionSession_Resume(session);
    ok(hr == S_OK, "ISpeechContinuousRecognitionSession_Resume failed, hr %#lx.\n", hr);

    recog_state = 0xdeadbeef;
    hr = ISpeechRecognizer2_get_State(recognizer2, &recog_state);
    ok(hr == S_OK, "ISpeechRecognizer2_get_State failed, hr %#lx.\n", hr);
    ok(recog_state == SpeechRecognizerState_Capturing, "recog_state was %u.\n", recog_state);

    hr = ISpeechContinuousRecognitionSession_StopAsync(session, &action2);
    ok(hr == S_OK, "ISpeechContinuousRecognitionSession_StopAsync failed, hr %#lx.\n", hr);

    async_void_handler_create_static(&action_handler);
    action_handler.event_block = CreateEventW(NULL, FALSE, FALSE, NULL);
    action_handler.event_finished = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!action_handler.event_block, "Block event wasn't created.\n");
    ok(!!action_handler.event_finished, "Finished event wasn't created.\n");

    /* Check if IAsyncInfo_Close is non blocking. */
    put_param.handler = &action_handler.IAsyncActionCompletedHandler_iface;
    put_param.action = action2;
    put_thread = CreateThread(NULL, 0, action_put_completed_thread, &put_param, 0, NULL);
    ok(!WaitForSingleObject(action_handler.event_finished , 5000), "Wait for event_finished failed.\n");

    handler = (void *)0xdeadbeef;
    old_ref = action_handler.ref;
    hr = IAsyncAction_get_Completed(action2, &handler);
    ok(hr == S_OK, "IAsyncAction_get_Completed failed, hr %#lx.\n", hr);

    todo_wine ok(handler == &action_handler.IAsyncActionCompletedHandler_iface, "Handler was %p.\n", handler);

    ref = action_handler.ref - old_ref;
    todo_wine ok(ref == 1, "The ref was increased by %lu.\n", ref);
    if (handler) IAsyncActionCompletedHandler_Release(handler);

    hr = IAsyncAction_QueryInterface(action2, &IID_IAsyncInfo, (void **)&info);
    ok(hr == S_OK, "IAsyncAction_QueryInterface failed, hr %#lx.\n", hr);

    hr = IAsyncInfo_Close(info); /* If IAsyncInfo_Close would wait for the handler to finish, the test would get stuck here. */
    ok(hr == S_OK, "IAsyncInfo_Close failed, hr %#lx.\n", hr);
    check_async_info((IInspectable *)action2, 5, AsyncStatus_Closed, S_OK);

    set = SetEvent(action_handler.event_block);
    ok(set == TRUE, "Event 'event_block' wasn't set.\n");
    ok(!WaitForSingleObject(put_thread, 1000), "Wait for put_thread failed.\n");
    IAsyncInfo_Release(info);

    CloseHandle(action_handler.event_finished);
    CloseHandle(action_handler.event_block);
    CloseHandle(put_thread);

    ok(action != action2, "actions were the same!\n");

    IAsyncAction_Release(action2);
    IAsyncAction_Release(action);

    recog_state = 0xdeadbeef;
    hr = ISpeechRecognizer2_get_State(recognizer2, &recog_state);
    ok(hr == S_OK, "ISpeechRecognizer2_get_State failed, hr %#lx.\n", hr);
    ok(recog_state == SpeechRecognizerState_Idle, "recog_state was %u.\n", recog_state);

    /* Try stopping, when already stopped. */
    hr = ISpeechContinuousRecognitionSession_StopAsync(session, &action);
    ok(hr == COR_E_INVALIDOPERATION, "ISpeechContinuousRecognitionSession_StopAsync failed, hr %#lx.\n", hr);
    ok(action == NULL, "action was %p.\n", action);

    /* Test, if Start/StopAsync resets the pause state. */
    hr = ISpeechContinuousRecognitionSession_StartAsync(session, &action);
    ok(hr == S_OK, "ISpeechContinuousRecognitionSession_StartAsync failed, hr %#lx.\n", hr);
    await_async_void(action, &action_handler);
    IAsyncAction_Release(action);

    hr = ISpeechContinuousRecognitionSession_PauseAsync(session, &action);
    ok(hr == S_OK, "ISpeechContinuousRecognitionSession_PauseAsync failed, hr %#lx.\n", hr);
    await_async_void(action, &action_handler);
    IAsyncAction_Release(action);

    recog_state = 0xdeadbeef;
    hr = ISpeechRecognizer2_get_State(recognizer2, &recog_state);
    ok(hr == S_OK, "ISpeechRecognizer2_get_State failed, hr %#lx.\n", hr);
    ok(recog_state == SpeechRecognizerState_Paused ||
       broken(recog_state == SpeechRecognizerState_Capturing) /* Broken on Win10 1507 */, "recog_state was %u.\n", recog_state);

    hr = ISpeechContinuousRecognitionSession_StopAsync(session, &action);
    ok(hr == S_OK, "ISpeechContinuousRecognitionSession_PauseAsync failed, hr %#lx.\n", hr);
    await_async_void(action, &action_handler);
    IAsyncAction_Release(action);

    recog_state = 0xdeadbeef;
    hr = ISpeechRecognizer2_get_State(recognizer2, &recog_state);
    ok(hr == S_OK, "ISpeechRecognizer2_get_State failed, hr %#lx.\n", hr);
    ok(recog_state == SpeechRecognizerState_Idle, "recog_state was %u.\n", recog_state);

    hr = ISpeechContinuousRecognitionSession_StartAsync(session, &action);
    ok(hr == S_OK, "ISpeechContinuousRecognitionSession_PauseAsync failed, hr %#lx.\n", hr);
    await_async_void(action, &action_handler);
    IAsyncAction_Release(action);

    recog_state = 0xdeadbeef;
    hr = ISpeechRecognizer2_get_State(recognizer2, &recog_state);
    ok(hr == S_OK, "ISpeechRecognizer2_get_State failed, hr %#lx.\n", hr);
    ok(recog_state == SpeechRecognizerState_Capturing
       || broken(recog_state == SpeechRecognizerState_Idle) /* Sometimes Windows is a little behind. */,
       "recog_state was %u.\n", recog_state);

    hr = ISpeechContinuousRecognitionSession_StopAsync(session, &action);
    ok(hr == S_OK, "ISpeechContinuousRecognitionSession_PauseAsync failed, hr %#lx.\n", hr);
    await_async_void(action, &action_handler);
    IAsyncAction_Release(action);

    hr = ISpeechContinuousRecognitionSession_remove_ResultGenerated(session, token);
    ok(hr == S_OK, "ISpeechContinuousRecognitionSession_remove_ResultGenerated failed, hr %#lx.\n", hr);

    ISpeechContinuousRecognitionSession_Release(session);
    ISpeechRecognizer2_Release(recognizer2);
    ISpeechRecognizer_Release(recognizer);
    IInspectable_Release(inspectable);

done:
    for (i = 0; i < ARRAY_SIZE(commands); i++)
        WindowsDeleteString(commands[i]);

    RoUninitialize();
}

START_TEST(speech)
{
    test_ActivationFactory();
    test_SpeechSynthesizer();
    test_VoiceInformation();
    test_SpeechRecognizer();
    test_SpeechRecognitionListConstraint();
    test_Recognition();
}
