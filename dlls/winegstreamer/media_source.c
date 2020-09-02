/* GStreamer Media Source
 *
 * Copyright 2020 Derek Lesho
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

#include "gst_private.h"

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "mfapi.h"
#include "mferror.h"
#include "mfidl.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct winegstreamer_stream_handler_result
{
    struct list entry;
    IMFAsyncResult *result;
    MF_OBJECT_TYPE obj_type;
    IUnknown *object;
};

struct winegstreamer_stream_handler
{
    IMFByteStreamHandler IMFByteStreamHandler_iface;
    IMFAsyncCallback IMFAsyncCallback_iface;
    LONG refcount;
    struct list results;
    CRITICAL_SECTION cs;
};

static struct winegstreamer_stream_handler *impl_from_IMFByteStreamHandler(IMFByteStreamHandler *iface)
{
    return CONTAINING_RECORD(iface, struct winegstreamer_stream_handler, IMFByteStreamHandler_iface);
}

static struct winegstreamer_stream_handler *impl_from_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct winegstreamer_stream_handler, IMFAsyncCallback_iface);
}

static HRESULT WINAPI winegstreamer_stream_handler_QueryInterface(IMFByteStreamHandler *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFByteStreamHandler) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFByteStreamHandler_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI winegstreamer_stream_handler_AddRef(IMFByteStreamHandler *iface)
{
    struct winegstreamer_stream_handler *handler = impl_from_IMFByteStreamHandler(iface);
    ULONG refcount = InterlockedIncrement(&handler->refcount);

    TRACE("%p, refcount %u.\n", handler, refcount);

    return refcount;
}

static ULONG WINAPI winegstreamer_stream_handler_Release(IMFByteStreamHandler *iface)
{
    struct winegstreamer_stream_handler *handler = impl_from_IMFByteStreamHandler(iface);
    ULONG refcount = InterlockedDecrement(&handler->refcount);
    struct winegstreamer_stream_handler_result *result, *next;

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        LIST_FOR_EACH_ENTRY_SAFE(result, next, &handler->results, struct winegstreamer_stream_handler_result, entry)
        {
            list_remove(&result->entry);
            IMFAsyncResult_Release(result->result);
            if (result->object)
                IUnknown_Release(result->object);
            heap_free(result);
        }
        DeleteCriticalSection(&handler->cs);
        heap_free(handler);
    }

    return refcount;
}

struct create_object_context
{
    IUnknown IUnknown_iface;
    LONG refcount;

    IPropertyStore *props;
    IMFByteStream *stream;
    WCHAR *url;
    DWORD flags;
};

static struct create_object_context *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct create_object_context, IUnknown_iface);
}

static HRESULT WINAPI create_object_context_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI create_object_context_AddRef(IUnknown *iface)
{
    struct create_object_context *context = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&context->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI create_object_context_Release(IUnknown *iface)
{
    struct create_object_context *context = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&context->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (context->props)
            IPropertyStore_Release(context->props);
        if (context->stream)
            IMFByteStream_Release(context->stream);
        if (context->url)
            heap_free(context->url);
        heap_free(context);
    }

    return refcount;
}

static const IUnknownVtbl create_object_context_vtbl =
{
    create_object_context_QueryInterface,
    create_object_context_AddRef,
    create_object_context_Release,
};

static WCHAR *heap_strdupW(const WCHAR *str)
{
    WCHAR *ret = NULL;

    if (str)
    {
        unsigned int size;

        size = (lstrlenW(str) + 1) * sizeof(WCHAR);
        ret = heap_alloc(size);
        if (ret)
            memcpy(ret, str, size);
    }

    return ret;
}

static HRESULT WINAPI winegstreamer_stream_handler_BeginCreateObject(IMFByteStreamHandler *iface, IMFByteStream *stream, const WCHAR *url, DWORD flags,
        IPropertyStore *props, IUnknown **cancel_cookie, IMFAsyncCallback *callback, IUnknown *state)
{
    struct winegstreamer_stream_handler *this = impl_from_IMFByteStreamHandler(iface);
    struct create_object_context *context;
    IMFAsyncResult *caller, *item;
    HRESULT hr;

    TRACE("%p, %s, %#x, %p, %p, %p, %p.\n", iface, debugstr_w(url), flags, props, cancel_cookie, callback, state);

    if (cancel_cookie)
        *cancel_cookie = NULL;

    if (FAILED(hr = MFCreateAsyncResult(NULL, callback, state, &caller)))
        return hr;

    context = heap_alloc(sizeof(*context));
    if (!context)
    {
        IMFAsyncResult_Release(caller);
        return E_OUTOFMEMORY;
    }

    context->IUnknown_iface.lpVtbl = &create_object_context_vtbl;
    context->refcount = 1;
    context->props = props;
    if (context->props)
        IPropertyStore_AddRef(context->props);
    context->flags = flags;
    context->stream = stream;
    if (context->stream)
        IMFByteStream_AddRef(context->stream);
    if (url)
        context->url = heap_strdupW(url);
    if (!context->stream)
    {
        IMFAsyncResult_Release(caller);
        IUnknown_Release(&context->IUnknown_iface);
        return E_OUTOFMEMORY;
    }

    hr = MFCreateAsyncResult(&context->IUnknown_iface, &this->IMFAsyncCallback_iface, (IUnknown *)caller, &item);
    IUnknown_Release(&context->IUnknown_iface);
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr = MFPutWorkItemEx(MFASYNC_CALLBACK_QUEUE_IO, item)))
        {
            if (cancel_cookie)
            {
                *cancel_cookie = (IUnknown *)caller;
                IUnknown_AddRef(*cancel_cookie);
            }
        }

        IMFAsyncResult_Release(item);
    }
    IMFAsyncResult_Release(caller);

    return hr;
}

static HRESULT WINAPI winegstreamer_stream_handler_EndCreateObject(IMFByteStreamHandler *iface, IMFAsyncResult *result,
        MF_OBJECT_TYPE *obj_type, IUnknown **object)
{
    struct winegstreamer_stream_handler *this = impl_from_IMFByteStreamHandler(iface);
    struct winegstreamer_stream_handler_result *found = NULL, *cur;
    HRESULT hr;

    TRACE("%p, %p, %p, %p.\n", iface, result, obj_type, object);

    EnterCriticalSection(&this->cs);

    LIST_FOR_EACH_ENTRY(cur, &this->results, struct winegstreamer_stream_handler_result, entry)
    {
        if (result == cur->result)
        {
            list_remove(&cur->entry);
            found = cur;
            break;
        }
    }

    LeaveCriticalSection(&this->cs);

    if (found)
    {
        *obj_type = found->obj_type;
        *object = found->object;
        hr = IMFAsyncResult_GetStatus(found->result);
        IMFAsyncResult_Release(found->result);
        heap_free(found);
    }
    else
    {
        *obj_type = MF_OBJECT_INVALID;
        *object = NULL;
        hr = MF_E_UNEXPECTED;
    }

    return hr;
}

static HRESULT WINAPI winegstreamer_stream_handler_CancelObjectCreation(IMFByteStreamHandler *iface, IUnknown *cancel_cookie)
{
    struct winegstreamer_stream_handler *this = impl_from_IMFByteStreamHandler(iface);
    struct winegstreamer_stream_handler_result *found = NULL, *cur;

    TRACE("%p, %p.\n", iface, cancel_cookie);

    EnterCriticalSection(&this->cs);

    LIST_FOR_EACH_ENTRY(cur, &this->results, struct winegstreamer_stream_handler_result, entry)
    {
        if (cancel_cookie == (IUnknown *)cur->result)
        {
            list_remove(&cur->entry);
            found = cur;
            break;
        }
    }

    LeaveCriticalSection(&this->cs);

    if (found)
    {
        IMFAsyncResult_Release(found->result);
        if (found->object)
            IUnknown_Release(found->object);
        heap_free(found);
    }

    return found ? S_OK : MF_E_UNEXPECTED;
}

static HRESULT WINAPI winegstreamer_stream_handler_GetMaxNumberOfBytesRequiredForResolution(IMFByteStreamHandler *iface, QWORD *bytes)
{
    FIXME("stub (%p %p)\n", iface, bytes);
    return E_NOTIMPL;
}

static const IMFByteStreamHandlerVtbl winegstreamer_stream_handler_vtbl =
{
    winegstreamer_stream_handler_QueryInterface,
    winegstreamer_stream_handler_AddRef,
    winegstreamer_stream_handler_Release,
    winegstreamer_stream_handler_BeginCreateObject,
    winegstreamer_stream_handler_EndCreateObject,
    winegstreamer_stream_handler_CancelObjectCreation,
    winegstreamer_stream_handler_GetMaxNumberOfBytesRequiredForResolution,
};

static HRESULT WINAPI winegstreamer_stream_handler_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI winegstreamer_stream_handler_callback_AddRef(IMFAsyncCallback *iface)
{
    struct winegstreamer_stream_handler *handler = impl_from_IMFAsyncCallback(iface);
    return IMFByteStreamHandler_AddRef(&handler->IMFByteStreamHandler_iface);
}

static ULONG WINAPI winegstreamer_stream_handler_callback_Release(IMFAsyncCallback *iface)
{
    struct winegstreamer_stream_handler *handler = impl_from_IMFAsyncCallback(iface);
    return IMFByteStreamHandler_Release(&handler->IMFByteStreamHandler_iface);
}

static HRESULT WINAPI winegstreamer_stream_handler_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT winegstreamer_stream_handler_create_object(struct winegstreamer_stream_handler *This, WCHAR *url, IMFByteStream *stream, DWORD flags,
                                            IPropertyStore *props, IUnknown **out_object, MF_OBJECT_TYPE *out_obj_type)
{
    FIXME("(%p %s %p %u %p %p %p)\n", This, debugstr_w(url), stream, flags, props, out_object, out_obj_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI winegstreamer_stream_handler_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct winegstreamer_stream_handler *handler = impl_from_IMFAsyncCallback(iface);
    struct winegstreamer_stream_handler_result *handler_result;
    MF_OBJECT_TYPE obj_type = MF_OBJECT_INVALID;
    IUnknown *object = NULL, *context_object;
    struct create_object_context *context;
    IMFAsyncResult *caller;
    HRESULT hr;

    caller = (IMFAsyncResult *)IMFAsyncResult_GetStateNoAddRef(result);

    if (FAILED(hr = IMFAsyncResult_GetObject(result, &context_object)))
    {
        WARN("Expected context set for callee result.\n");
        return hr;
    }

    context = impl_from_IUnknown(context_object);

    hr = winegstreamer_stream_handler_create_object(handler, context->url, context->stream, context->flags, context->props, &object, &obj_type);

    handler_result = heap_alloc(sizeof(*handler_result));
    if (handler_result)
    {
        handler_result->result = caller;
        IMFAsyncResult_AddRef(handler_result->result);
        handler_result->obj_type = obj_type;
        handler_result->object = object;

        EnterCriticalSection(&handler->cs);
        list_add_tail(&handler->results, &handler_result->entry);
        LeaveCriticalSection(&handler->cs);
    }
    else
    {
        if (object)
            IUnknown_Release(object);
        hr = E_OUTOFMEMORY;
    }

    IUnknown_Release(&context->IUnknown_iface);

    IMFAsyncResult_SetStatus(caller, hr);
    MFInvokeCallback(caller);

    return S_OK;
}

static const IMFAsyncCallbackVtbl winegstreamer_stream_handler_callback_vtbl =
{
    winegstreamer_stream_handler_callback_QueryInterface,
    winegstreamer_stream_handler_callback_AddRef,
    winegstreamer_stream_handler_callback_Release,
    winegstreamer_stream_handler_callback_GetParameters,
    winegstreamer_stream_handler_callback_Invoke,
};

HRESULT winegstreamer_stream_handler_create(REFIID riid, void **obj)
{
    struct winegstreamer_stream_handler *this;
    HRESULT hr;

    TRACE("%s, %p.\n", debugstr_guid(riid), obj);

    this = heap_alloc_zero(sizeof(*this));
    if (!this)
        return E_OUTOFMEMORY;

    list_init(&this->results);
    InitializeCriticalSection(&this->cs);

    this->IMFByteStreamHandler_iface.lpVtbl = &winegstreamer_stream_handler_vtbl;
    this->IMFAsyncCallback_iface.lpVtbl = &winegstreamer_stream_handler_callback_vtbl;
    this->refcount = 1;

    hr = IMFByteStreamHandler_QueryInterface(&this->IMFByteStreamHandler_iface, riid, obj);
    IMFByteStreamHandler_Release(&this->IMFByteStreamHandler_iface);

    return hr;
}
