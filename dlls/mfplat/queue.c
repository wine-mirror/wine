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
    IRtwqAsyncResult *result;
    HRESULT hr;

    TRACE("%#x, %p, %p.\n", queue, callback, state);

    if (FAILED(hr = RtwqCreateAsyncResult(NULL, (IRtwqAsyncCallback *)callback, state, &result)))
        return hr;

    hr = RtwqPutWorkItem(queue, 0, result);

    IRtwqAsyncResult_Release(result);

    return hr;
}

/***********************************************************************
 *      MFPutWorkItem2 (mfplat.@)
 */
HRESULT WINAPI MFPutWorkItem2(DWORD queue, LONG priority, IMFAsyncCallback *callback, IUnknown *state)
{
    IRtwqAsyncResult *result;
    HRESULT hr;

    TRACE("%#x, %d, %p, %p.\n", queue, priority, callback, state);

    if (FAILED(hr = RtwqCreateAsyncResult(NULL, (IRtwqAsyncCallback *)callback, state, &result)))
        return hr;

    hr = RtwqPutWorkItem(queue, priority, result);

    IRtwqAsyncResult_Release(result);

    return hr;
}

/***********************************************************************
 *      MFPutWorkItemEx (mfplat.@)
 */
HRESULT WINAPI MFPutWorkItemEx(DWORD queue, IMFAsyncResult *result)
{
    TRACE("%#x, %p\n", queue, result);

    return RtwqPutWorkItem(queue, 0, (IRtwqAsyncResult *)result);
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
    IRtwqAsyncResult *result;
    HRESULT hr;

    TRACE("%p, %p, %s, %p.\n", callback, state, wine_dbgstr_longlong(timeout), key);

    if (FAILED(hr = RtwqCreateAsyncResult(NULL, (IRtwqAsyncCallback *)callback, state, &result)))
        return hr;

    hr = RtwqScheduleWorkItem(result, timeout, key);

    IRtwqAsyncResult_Release(result);

    return hr;
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
 *      MFGetTimerPeriodicity (mfplat.@)
 */
HRESULT WINAPI MFGetTimerPeriodicity(DWORD *period)
{
    TRACE("%p.\n", period);

    *period = 10;

    return S_OK;
}

/***********************************************************************
 *      MFBeginRegisterWorkQueueWithMMCSS (mfplat.@)
 */
HRESULT WINAPI MFBeginRegisterWorkQueueWithMMCSS(DWORD queue, const WCHAR *usage_class, DWORD taskid,
        IMFAsyncCallback *callback, IUnknown *state)
{
    return RtwqBeginRegisterWorkQueueWithMMCSS(queue, usage_class, taskid, 0,
            (IRtwqAsyncCallback *)callback, state);
}
