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

typedef enum PSS_CAPTURE_FLAGS
{
    PSS_CAPTURE_NONE                             = 0x00000000,
    PSS_CAPTURE_VA_CLONE                         = 0x00000001,
    PSS_CAPTURE_RESERVED_00000002                = 0x00000002,
    PSS_CAPTURE_HANDLES                          = 0x00000004,
    PSS_CAPTURE_HANDLE_NAME_INFORMATION          = 0x00000008,
    PSS_CAPTURE_HANDLE_BASIC_INFORMATION         = 0x00000010,
    PSS_CAPTURE_HANDLE_TYPE_SPECIFIC_INFORMATION = 0x00000020,
    PSS_CAPTURE_HANDLE_TRACE                     = 0x00000040,
    PSS_CAPTURE_THREADS                          = 0x00000080,
    PSS_CAPTURE_THREAD_CONTEXT                   = 0x00000100,
    PSS_CAPTURE_THREAD_CONTEXT_EXTENDED          = 0x00000200,
    PSS_CAPTURE_RESERVED_00000400                = 0x00000400,
    PSS_CAPTURE_VA_SPACE                         = 0x00000800,
    PSS_CAPTURE_VA_SPACE_SECTION_INFORMAN        = 0x00001000,
    PSS_CAPTURE_IPT_TRACE                        = 0x00002000,
    PSS_CAPTURE_RESERVED_00004000                = 0x00004000,
    PSS_CREATE_BREAKAWAY_OPTIONAL                = 0x04000000,
    PSS_CREATE_BREAKAWAY                         = 0x08000000,
    PSS_CREATE_FORCE_BREAKAWAY                   = 0x10000000,
    PSS_CREATE_USE_VM_ALLOCATIONS                = 0x20000000,
    PSS_CREATE_MEASURE_PERFORMANCE               = 0x40000000,
    PSS_CREATE_RELEASE_SECTION                   = 0x80000000
} PSS_CAPTURE_FLAGS;

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

DWORD WINAPI PssCaptureSnapshot(HANDLE,PSS_CAPTURE_FLAGS,DWORD,HPSS*);
DWORD WINAPI PssFreeSnapshot(HANDLE,HPSS);
DWORD WINAPI PssQuerySnapshot(HPSS,PSS_QUERY_INFORMATION_CLASS,void*,DWORD);

#ifdef __cplusplus
}
#endif

#endif /* _PROCESSSNAPSHOT_H_ */
