/*
 * Copyright 2010 Piotr Caban for CodeWeavers
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

#include "config.h"

#include <stdarg.h>
#include <limits.h>

#include "msvcp90.h"

#include "windef.h"
#include "winbase.h"

typedef struct {
    void *mutex;
} mutex;

/* ??0_Mutex@std@@QAE@XZ */
/* ??0_Mutex@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(mutex_ctor, 4)
mutex* __thiscall mutex_ctor(mutex *this)
{
    this->mutex = CreateMutexW(NULL, FALSE, NULL);
    return this;
}

/* ??1_Mutex@std@@QAE@XZ */
/* ??1_Mutex@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(mutex_dtor, 4)
void __thiscall mutex_dtor(mutex *this)
{
    CloseHandle(this->mutex);
}

/* ?_Lock@_Mutex@std@@QAEXXZ */
/* ?_Lock@_Mutex@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(mutex_lock, 4)
void __thiscall mutex_lock(mutex *this)
{
    WaitForSingleObject(this->mutex, INFINITE);
}

/* ?_Unlock@_Mutex@std@@QAEXXZ */
/* ?_Unlock@_Mutex@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(mutex_unlock, 4)
void __thiscall mutex_unlock(mutex *this)
{
    ReleaseMutex(this->mutex);
}

/* ?_Mutex_Lock@_Mutex@std@@CAXPAV12@@Z */
/* ?_Mutex_Lock@_Mutex@std@@CAXPEAV12@@Z */
void CDECL mutex_mutex_lock(mutex *m)
{
    mutex_lock(m);
}

/* ?_Mutex_Unlock@_Mutex@std@@CAXPAV12@@Z */
/* ?_Mutex_Unlock@_Mutex@std@@CAXPEAV12@@Z */
void CDECL mutex_mutex_unlock(mutex *m)
{
    mutex_unlock(m);
}

/* ?_Mutex_ctor@_Mutex@std@@CAXPAV12@@Z */
/* ?_Mutex_ctor@_Mutex@std@@CAXPEAV12@@Z */
void CDECL mutex_mutex_ctor(mutex *m)
{
    mutex_ctor(m);
}

/* ?_Mutex_dtor@_Mutex@std@@CAXPAV12@@Z */
/* ?_Mutex_dtor@_Mutex@std@@CAXPEAV12@@Z */
void CDECL mutex_mutex_dtor(mutex *m)
{
    mutex_dtor(m);
}
