/*
 * Speech API (SAPI) async helper implementation.
 *
 * Copyright 2023 Shaun Ren for CodeWeavers
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
#include "objbase.h"

#include "wine/list.h"
#include "wine/debug.h"

#include "sapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(sapi);

static struct async_task *async_dequeue_task(struct async_queue *queue)
{
    struct async_task *task = NULL;
    struct list *head;

    EnterCriticalSection(&queue->cs);
    if ((head = list_head(&queue->tasks)))
    {
        task = LIST_ENTRY(head, struct async_task, entry);
        list_remove(head);
    }
    LeaveCriticalSection(&queue->cs);

    return task;
}

void async_empty_queue(struct async_queue *queue)
{
    struct async_task *task, *next;

    if (!queue->init) return;

    EnterCriticalSection(&queue->cs);
    LIST_FOR_EACH_ENTRY_SAFE(task, next, &queue->tasks, struct async_task, entry)
    {
        list_remove(&task->entry);
        free(task);
    }
    LeaveCriticalSection(&queue->cs);

    SetEvent(queue->empty);
}

static void CALLBACK async_worker(TP_CALLBACK_INSTANCE *instance, void *ctx)
{
    struct async_queue *queue = ctx;
    HANDLE handles[2] = { queue->cancel, queue->wait };
    DWORD ret;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    SetEvent(queue->ready);

    for (;;)
    {
        ret = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
        if (ret == WAIT_OBJECT_0)
            goto cancel;
        else if (ret == WAIT_OBJECT_0 + 1)
        {
            struct async_task *task;

            while ((task = async_dequeue_task(queue)))
            {
                ResetEvent(queue->empty);
                task->proc(task);
                free(task);
                if (WaitForSingleObject(queue->cancel, 0) == WAIT_OBJECT_0)
                    goto cancel;
            }

            SetEvent(queue->empty);
        }
        else
            ERR("WaitForMultipleObjects failed: %#lx.\n", ret);
    }

cancel:
    async_empty_queue(queue);
    CoUninitialize();
    TRACE("cancelled.\n");
    SetEvent(queue->ready);
}

HRESULT async_start_queue(struct async_queue *queue)
{
    HRESULT hr;

    if (queue->init)
        return S_OK;

    InitializeCriticalSection(&queue->cs);
    list_init(&queue->tasks);

    if (!(queue->wait = CreateEventW(NULL, FALSE, FALSE, NULL)) ||
        !(queue->ready = CreateEventW(NULL, FALSE, FALSE, NULL)) ||
        !(queue->cancel = CreateEventW(NULL, FALSE, FALSE, NULL)) ||
        !(queue->empty = CreateEventW(NULL, TRUE, TRUE, NULL)))
        goto fail;

    queue->init = TRUE;

    if (!TrySubmitThreadpoolCallback(async_worker, queue, NULL))
        goto fail;

    WaitForSingleObject(queue->ready, INFINITE);
    return S_OK;

fail:
    hr = HRESULT_FROM_WIN32(GetLastError());
    DeleteCriticalSection(&queue->cs);
    if (queue->wait)   CloseHandle(queue->wait);
    if (queue->ready)  CloseHandle(queue->ready);
    if (queue->cancel) CloseHandle(queue->cancel);
    if (queue->empty)  CloseHandle(queue->empty);
    memset(queue, 0, sizeof(*queue));
    return hr;
}

void async_cancel_queue(struct async_queue *queue)
{
    if (!queue->init) return;

    SetEvent(queue->cancel);
    WaitForSingleObject(queue->ready, INFINITE);

    DeleteCriticalSection(&queue->cs);
    CloseHandle(queue->wait);
    CloseHandle(queue->ready);
    CloseHandle(queue->cancel);
    CloseHandle(queue->empty);

    memset(queue, 0, sizeof(*queue));
}

HRESULT async_queue_task(struct async_queue *queue, struct async_task *task)
{
    HRESULT hr;

    if (FAILED(hr = async_start_queue(queue)))
        return hr;

    EnterCriticalSection(&queue->cs);
    list_add_tail(&queue->tasks, &task->entry);
    LeaveCriticalSection(&queue->cs);

    ResetEvent(queue->empty);
    SetEvent(queue->wait);

    return S_OK;
}

HRESULT async_wait_queue_empty(struct async_queue *queue, DWORD timeout)
{
    if (!queue->init) return WAIT_OBJECT_0;
    return WaitForSingleObject(queue->empty, timeout);
}
