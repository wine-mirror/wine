/*
 * Generic Implementation of COutputQueue
 *
 * Copyright 2011 Aric Stewart, CodeWeavers
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

#include "dshow.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/list.h"
#include "wine/strmbase.h"
#include "uuids.h"
#include "vfwmsgs.h"
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(strmbase);

typedef struct tagQueuedEvent {
    struct list entry;

    IMediaSample *pSample;
} QueuedEvent;

static DWORD WINAPI OutputQueue_InitialThreadProc(LPVOID data)
{
    OutputQueue *This = (OutputQueue *)data;
    return This->pFuncsTable->pfnThreadProc(This);
}

static void OutputQueue_FreeSamples(OutputQueue *pOutputQueue)
{
    struct list *cursor, *cursor2;
    LIST_FOR_EACH_SAFE(cursor, cursor2, pOutputQueue->SampleList)
    {
        QueuedEvent *qev = LIST_ENTRY(cursor, QueuedEvent, entry);
        list_remove(cursor);
        HeapFree(GetProcessHeap(),0,qev);
    }
}

HRESULT WINAPI OutputQueue_Construct(
    BaseOutputPin *pInputPin,
    BOOL bAuto,
    BOOL bQueue,
    LONG lBatchSize,
    BOOL bBatchExact,
    DWORD dwPriority,
    const OutputQueueFuncTable* pFuncsTable,
    OutputQueue **ppOutputQueue )

{
    HRESULT hr = S_OK;
    BOOL threaded = FALSE;
    DWORD tid;

    OutputQueue *This;

    if (!pInputPin || !pFuncsTable || !ppOutputQueue)
        return E_INVALIDARG;

    *ppOutputQueue = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(OutputQueue));
    if (!*ppOutputQueue)
        return E_OUTOFMEMORY;

    This = *ppOutputQueue;
    This->pFuncsTable = pFuncsTable;
    This->lBatchSize = lBatchSize;
    This->bBatchExact = bBatchExact;
    InitializeCriticalSection(&This->csQueue);
    This->csQueue.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": OutputQueue.csQueue");
    This->SampleList = HeapAlloc(GetProcessHeap(),0,sizeof(struct list));
    if (!This->SampleList)
    {
        OutputQueue_Destroy(This);
        *ppOutputQueue = NULL;
        return E_OUTOFMEMORY;
    }
    list_init(This->SampleList);

    This->pInputPin = pInputPin;
    IPin_AddRef((IPin*)pInputPin);

    EnterCriticalSection(&This->csQueue);
    if (bAuto && pInputPin->pMemInputPin)
        threaded = IMemInputPin_ReceiveCanBlock(pInputPin->pMemInputPin);
    else
        threaded = bQueue;

    if (threaded)
    {
        This->hThread = CreateThread(NULL, 0, OutputQueue_InitialThreadProc, This, 0, &tid);
        if (This->hThread)
        {
            SetThreadPriority(This->hThread, dwPriority);
            This->hProcessQueue = CreateEventW(NULL, 0, 0, NULL);
        }
    }
    LeaveCriticalSection(&This->csQueue);

    return hr;
}

HRESULT WINAPI OutputQueue_Destroy(OutputQueue *pOutputQueue)
{
    EnterCriticalSection(&pOutputQueue->csQueue);
    OutputQueue_FreeSamples(pOutputQueue);
    pOutputQueue->bTerminate = TRUE;
    SetEvent(pOutputQueue->hProcessQueue);
    LeaveCriticalSection(&pOutputQueue->csQueue);

    DeleteCriticalSection(&pOutputQueue->csQueue);
    CloseHandle(pOutputQueue->hProcessQueue);

    if (pOutputQueue->SampleList)
        HeapFree(GetProcessHeap(),0,pOutputQueue->SampleList);

    IPin_Release((IPin*)pOutputQueue->pInputPin);
    HeapFree(GetProcessHeap(),0,pOutputQueue);
    return S_OK;
}

HRESULT WINAPI OutputQueue_ReceiveMultiple(OutputQueue *pOutputQueue, IMediaSample **ppSamples, LONG nSamples, LONG *nSamplesProcessed)
{
    HRESULT hr = S_OK;
    int i;

    if (!pOutputQueue->pInputPin->pin.pConnectedTo || !pOutputQueue->pInputPin->pMemInputPin)
        return VFW_E_NOT_CONNECTED;

    if (!pOutputQueue->hThread)
    {
        IMemInputPin_AddRef(pOutputQueue->pInputPin->pMemInputPin);
        hr = IMemInputPin_ReceiveMultiple(pOutputQueue->pInputPin->pMemInputPin,ppSamples, nSamples, nSamplesProcessed);
        IMemInputPin_Release(pOutputQueue->pInputPin->pMemInputPin);
    }
    else
    {
        EnterCriticalSection(&pOutputQueue->csQueue);
        *nSamplesProcessed = 0;

        for (i = 0; i < nSamples; i++)
        {
            QueuedEvent *qev = HeapAlloc(GetProcessHeap(),0,sizeof(QueuedEvent));
            if (!qev)
            {
                ERR("Out of Memory\n");
                hr = E_OUTOFMEMORY;
                break;
            }
            qev->pSample = ppSamples[i];
            IMediaSample_AddRef(ppSamples[i]);
            list_add_tail(pOutputQueue->SampleList, &qev->entry);
            (*nSamplesProcessed)++;
        }
        LeaveCriticalSection(&pOutputQueue->csQueue);

        if (!pOutputQueue->bBatchExact || list_count(pOutputQueue->SampleList) >= pOutputQueue->lBatchSize)
            SetEvent(pOutputQueue->hProcessQueue);
    }
    return hr;
}

HRESULT WINAPI OutputQueue_Receive(OutputQueue *pOutputQueue, IMediaSample *pSample)
{
    LONG processed;
    return OutputQueue_ReceiveMultiple(pOutputQueue,&pSample,1,&processed);
}

DWORD WINAPI OutputQueueImpl_ThreadProc(OutputQueue *pOutputQueue)
{
    do
    {
        EnterCriticalSection(&pOutputQueue->csQueue);
        if (list_count(pOutputQueue->SampleList) > 0 &&
             (!pOutputQueue->bBatchExact ||
              list_count(pOutputQueue->SampleList) >= pOutputQueue->lBatchSize))
        {
            IMediaSample **ppSamples;
            LONG nSamples;
            LONG nSamplesProcessed;
            struct list *cursor, *cursor2;
            int i = 0;

            nSamples = list_count(pOutputQueue->SampleList);
            ppSamples = HeapAlloc(GetProcessHeap(),0,sizeof(IMediaSample*) * nSamples);
            LIST_FOR_EACH_SAFE(cursor, cursor2, pOutputQueue->SampleList)
            {
                QueuedEvent *qev = LIST_ENTRY(cursor, QueuedEvent, entry);
                list_remove(cursor);
                ppSamples[i++] = qev->pSample;
                HeapFree(GetProcessHeap(),0,qev);
            }

            if (pOutputQueue->pInputPin->pin.pConnectedTo && pOutputQueue->pInputPin->pMemInputPin)
            {
                IMemInputPin_AddRef(pOutputQueue->pInputPin->pMemInputPin);
                IMemInputPin_ReceiveMultiple(pOutputQueue->pInputPin->pMemInputPin, ppSamples, nSamples, &nSamplesProcessed);
                IMemInputPin_Release(pOutputQueue->pInputPin->pMemInputPin);
            }
            for (i = 0; i < nSamples; i++)
                IUnknown_Release(ppSamples[i]);
            HeapFree(GetProcessHeap(),0,ppSamples);
        }
        LeaveCriticalSection(&pOutputQueue->csQueue);
        WaitForSingleObject(pOutputQueue->hProcessQueue, INFINITE);
    }
    while (!pOutputQueue->bTerminate);
    return S_OK;
}
