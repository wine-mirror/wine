/*
 * Speech API (SAPI) private header file.
 *
 * Copyright (C) 2017 Huw Davies
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

#include "wine/list.h"

struct async_task
{
    struct list entry;
    void (*proc)(struct async_task *);
};

struct async_queue
{
    BOOL init;
    HANDLE wait;
    HANDLE ready;
    HANDLE empty;
    HANDLE cancel;
    struct list tasks;
    CRITICAL_SECTION cs;
};

HRESULT async_start_queue(struct async_queue *queue);
void async_empty_queue(struct async_queue *queue);
void async_cancel_queue(struct async_queue *queue);
HRESULT async_queue_task(struct async_queue *queue, struct async_task *task);
HRESULT async_wait_queue_empty(struct async_queue *queue, DWORD timeout);

HRESULT data_key_create( IUnknown *outer, REFIID iid, void **obj );
HRESULT file_stream_create( IUnknown *outer, REFIID iid, void **obj );
HRESULT resource_manager_create( IUnknown *outer, REFIID iid, void **obj );
HRESULT speech_stream_create( IUnknown *outer, REFIID iid, void **obj );
HRESULT speech_voice_create( IUnknown *outer, REFIID iid, void **obj );
HRESULT mmaudio_out_create( IUnknown *outer, REFIID iid, void **obj );
HRESULT token_category_create( IUnknown *outer, REFIID iid, void **obj );
HRESULT token_enum_create( IUnknown *outer, REFIID iid, void **obj );
HRESULT token_create( IUnknown *outer, REFIID iid, void **obj );
