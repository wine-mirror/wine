/*
 * Copyright 2011-2012 Maarten Lankhorst
 * Copyright 2010-2011 Maarten Lankhorst for CodeWeavers
 * Copyright 2011 Andrew Eikum for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>
#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"

#include "unixlib.h"

static pthread_mutex_t pulse_mutex;
static pthread_cond_t pulse_cond = PTHREAD_COND_INITIALIZER;

static void WINAPI pulse_lock(void)
{
    pthread_mutex_lock(&pulse_mutex);
}

static void WINAPI pulse_unlock(void)
{
    pthread_mutex_unlock(&pulse_mutex);
}

static int WINAPI pulse_cond_wait(void)
{
    return pthread_cond_wait(&pulse_cond, &pulse_mutex);
}

static void WINAPI pulse_broadcast(void)
{
    pthread_cond_broadcast(&pulse_cond);
}

static const struct unix_funcs unix_funcs =
{
    pulse_lock,
    pulse_unlock,
    pulse_cond_wait,
    pulse_broadcast,
};

NTSTATUS CDECL __wine_init_unix_lib(HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out)
{
    pthread_mutexattr_t attr;

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);

        if (pthread_mutex_init(&pulse_mutex, &attr) != 0)
            pthread_mutex_init(&pulse_mutex, NULL);

        *(const struct unix_funcs **)ptr_out = &unix_funcs;
        break;
    }

    return STATUS_SUCCESS;
}
