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

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "wine/debug.h"
#include "wine/list.h"

#include "mfplat_private.h"
#include "rtworkq.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

/***********************************************************************
 *      MFAllocateWorkQueue (mfplat.@)
 */
HRESULT WINAPI MFAllocateWorkQueue(DWORD *queue)
{
    TRACE("%p.\n", queue);

    return RtwqAllocateWorkQueue(RTWQ_STANDARD_WORKQUEUE, queue);
}

/***********************************************************************
 *      MFPutWorkItem (mfplat.@)
 */
HRESULT WINAPI MFPutWorkItem(DWORD queue, IMFAsyncCallback *callback, IUnknown *state)
{
    IMFAsyncResult *result;
    HRESULT hr;

    TRACE("%#x, %p, %p.\n", queue, callback, state);

    if (FAILED(hr = MFCreateAsyncResult(NULL, callback, state, &result)))
        return hr;

    hr = MFPutWorkItemEx2(queue, 0, result);

    IMFAsyncResult_Release(result);

    return hr;
}

/***********************************************************************
 *      MFPutWorkItem2 (mfplat.@)
 */
HRESULT WINAPI MFPutWorkItem2(DWORD queue, LONG priority, IMFAsyncCallback *callback, IUnknown *state)
{
    IMFAsyncResult *result;
    HRESULT hr;

    TRACE("%#x, %d, %p, %p.\n", queue, priority, callback, state);

    if (FAILED(hr = MFCreateAsyncResult(NULL, callback, state, &result)))
        return hr;

    hr = MFPutWorkItemEx2(queue, priority, result);

    IMFAsyncResult_Release(result);

    return hr;
}

/***********************************************************************
 *      MFPutWorkItemEx (mfplat.@)
 */
HRESULT WINAPI MFPutWorkItemEx(DWORD queue, IMFAsyncResult *result)
{
    TRACE("%#x, %p\n", queue, result);

    return MFPutWorkItemEx2(queue, 0, result);
}

/***********************************************************************
 *      MFPutWorkItemEx2 (mfplat.@)
 */
HRESULT WINAPI MFPutWorkItemEx2(DWORD queue, LONG priority, IMFAsyncResult *result)
{
    TRACE("%#x, %d, %p\n", queue, priority, result);

    return RtwqPutWorkItem(queue, priority, (IRtwqAsyncResult *)result);
}

/***********************************************************************
 *      MFScheduleWorkItem (mfplat.@)
 */
HRESULT WINAPI MFScheduleWorkItem(IMFAsyncCallback *callback, IUnknown *state, INT64 timeout, MFWORKITEM_KEY *key)
{
    IMFAsyncResult *result;
    HRESULT hr;

    TRACE("%p, %p, %s, %p.\n", callback, state, wine_dbgstr_longlong(timeout), key);

    if (FAILED(hr = MFCreateAsyncResult(NULL, callback, state, &result)))
        return hr;

    hr = MFScheduleWorkItemEx(result, timeout, key);

    IMFAsyncResult_Release(result);

    return hr;
}

/***********************************************************************
 *      MFScheduleWorkItemEx (mfplat.@)
 */
HRESULT WINAPI MFScheduleWorkItemEx(IMFAsyncResult *result, INT64 timeout, MFWORKITEM_KEY *key)
{
    TRACE("%p, %s, %p.\n", result, wine_dbgstr_longlong(timeout), key);

    return RtwqScheduleWorkItem((IRtwqAsyncResult *)result, timeout, key);
}

/***********************************************************************
 *      MFInvokeCallback (mfplat.@)
 */
HRESULT WINAPI MFInvokeCallback(IMFAsyncResult *result)
{
    TRACE("%p.\n", result);

    return RtwqInvokeCallback((IRtwqAsyncResult *)result);
}

/***********************************************************************
 *      MFCreateAsyncResult (mfplat.@)
 */
HRESULT WINAPI MFCreateAsyncResult(IUnknown *object, IMFAsyncCallback *callback, IUnknown *state, IMFAsyncResult **out)
{
    TRACE("%p, %p, %p, %p.\n", object, callback, state, out);

    return RtwqCreateAsyncResult(object, (IRtwqAsyncCallback *)callback, state, (IRtwqAsyncResult **)out);
}

/***********************************************************************
 *      MFGetTimerPeriodicity (mfplat.@)
 */
HRESULT WINAPI MFGetTimerPeriodicity(DWORD *period)
{
    TRACE("%p.\n", period);

    *period = 10;

    return S_OK;
}
