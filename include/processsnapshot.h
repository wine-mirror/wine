/*
 * Copyright (C) 2025 Louis Lenders
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

#ifndef _PROCESSSNAPSHOT_H_
#define _PROCESSSNAPSHOT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PSS_QUERY_INFORMATION_CLASS
{
    PSS_QUERY_PROCESS_INFORMATION         = 0,
    PSS_QUERY_VA_CLONE_INFORMATION        = 1,
    PSS_QUERY_AUXILIARY_PAGES_INFORMATION = 2,
    PSS_QUERY_VA_SPACE_INFORMATION        = 3,
    PSS_QUERY_HANDLE_INFORMATION          = 4,
    PSS_QUERY_THREAD_INFORMATION          = 5,
    PSS_QUERY_HANDLE_TRACE_INFORMATION    = 6,
    PSS_QUERY_PERFORMANCE_COUNTERS        = 7
} PSS_QUERY_INFORMATION_CLASS;

DECLARE_HANDLE(HPSS);

DWORD WINAPI PssQuerySnapshot(HPSS,PSS_QUERY_INFORMATION_CLASS,void*,DWORD);

#ifdef __cplusplus
}
#endif

#endif /* _PROCESSSNAPSHOT_H_ */
