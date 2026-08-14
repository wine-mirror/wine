/*
 * Copyright (C) the Wine project
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

#ifndef __WINE_XTASKQUEUE_H
#define __WINE_XTASKQUEUE_H

#ifdef __cplusplus
extern "C" {

enum class XTaskQueueDispatchMode : UINT32
{
    Manual,
    ThreadPool,
    SerializedThreadPool,
    Immediate,
};

enum class XTaskQueuePort : UINT32
{
    Work,
    Completion,
};

#elif defined(__WINESRC__)

typedef enum XTaskQueueDispatchMode
{
    XTaskQueueDispatchMode_Manual,
    XTaskQueueDispatchMode_ThreadPool,
    XTaskQueueDispatchMode_SerializedThreadPool,
    XTaskQueueDispatchMode_Immediate,
} XTaskQueueDispatchMode;

typedef enum XTaskQueuePort
{
    XTaskQueuePort_Work,
    XTaskQueuePort_Completion,
} XTaskQueuePort;

#endif

typedef struct XTaskQueueObject *XTaskQueueHandle;
typedef struct XTaskQueuePortObject *XTaskQueuePortHandle;

typedef struct XTaskQueueRegistrationToken XTaskQueueRegistrationToken;

typedef void __stdcall XTaskQueueCallback( void *context, BOOLEAN canceled );
typedef void __stdcall XTaskQueueMonitorCallback( void *context, XTaskQueueHandle queue, XTaskQueuePort port );
typedef void __stdcall XTaskQueueTerminatedCallback( void *context );

struct XTaskQueueRegistrationToken
{
    UINT64 token;
};

#ifdef __cplusplus
}
#endif

#endif
