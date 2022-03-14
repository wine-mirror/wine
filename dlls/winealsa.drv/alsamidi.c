/*
 * MIDI driver for ALSA (unixlib)
 *
 * Copyright 1994       Martin Ayotte
 * Copyright 1998       Luiz Otavio L. Zorzella
 * Copyright 1998, 1999 Eric POUECH
 * Copyright 2003       Christian Costa
 * Copyright 2022       Huw Davies
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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "mmdeviceapi.h"

#include "wine/unixlib.h"

#include "unixlib.h"

static pthread_mutex_t seq_mutex = PTHREAD_MUTEX_INITIALIZER;

static void seq_lock(void)
{
    pthread_mutex_lock(&seq_mutex);
}

static void seq_unlock(void)
{
    pthread_mutex_unlock(&seq_mutex);
}

NTSTATUS midi_seq_lock(void *args)
{
    if (args) seq_lock();
    else seq_unlock();

    return STATUS_SUCCESS;
}
