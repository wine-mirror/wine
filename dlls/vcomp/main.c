/*
 *
 * vcomp implementation
 *
 * Copyright 2011 Austin English
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

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vcomp);

static int     vcomp_max_threads;
static int     vcomp_num_threads;

int CDECL omp_get_dynamic(void)
{
    TRACE("stub\n");
    return 0;
}

int CDECL omp_get_max_threads(void)
{
    TRACE("()\n");
    return vcomp_max_threads;
}

int CDECL omp_get_nested(void)
{
    TRACE("stub\n");
    return 0;
}

int CDECL omp_get_num_procs(void)
{
    TRACE("stub\n");
    return 1;
}

int CDECL omp_get_num_threads(void)
{
    TRACE("stub\n");
    return 1;
}

int CDECL omp_get_thread_num(void)
{
    TRACE("stub\n");
    return 0;
}

/* Time in seconds since "some time in the past" */
double CDECL omp_get_wtime(void)
{
    return GetTickCount() / 1000.0;
}

void CDECL omp_set_dynamic(int val)
{
    TRACE("(%d): stub\n", val);
}

void CDECL omp_set_nested(int nested)
{
    TRACE("(%d): stub\n", nested);
}

void CDECL omp_set_num_threads(int num_threads)
{
    TRACE("(%d)\n", num_threads);
    if (num_threads >= 1)
        vcomp_num_threads = num_threads;
}

void CDECL _vcomp_barrier(void)
{
    TRACE("stub\n");
}

void CDECL _vcomp_set_num_threads(int num_threads)
{
    TRACE("(%d): stub\n", num_threads);
}

int CDECL _vcomp_single_begin(int flags)
{
    TRACE("(%x): stub\n", flags);
    return TRUE;
}

void CDECL _vcomp_single_end(void)
{
    TRACE("stub\n");
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    TRACE("(%p, %d, %p)\n", instance, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */

        case DLL_PROCESS_ATTACH:
        {
            SYSTEM_INFO sysinfo;
            DisableThreadLibraryCalls(instance);

            GetSystemInfo(&sysinfo);
            vcomp_max_threads = sysinfo.dwNumberOfProcessors;
            vcomp_num_threads = sysinfo.dwNumberOfProcessors;
            break;
        }
    }

    return TRUE;
}
