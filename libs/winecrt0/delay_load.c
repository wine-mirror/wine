/*
 * Support for delayed imports
 *
 * Copyright 2005 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * In addition to the permissions in the GNU Lesser General Public License,
 * the authors give you unlimited permission to link the compiled version
 * of this file with other programs, and to distribute those programs
 * without any restriction coming from the use of this file.  (The GNU
 * Lesser General Public License restrictions do apply in other respects;
 * for example, they cover modification of the file, and distribution when
 * not linked into another program.)
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
#include "delayloadhandler.h"

WINBASEAPI void *WINAPI DelayLoadFailureHook( LPCSTR name, LPCSTR function );
WINBASEAPI void *WINAPI ResolveDelayLoadedAPI( void* base, const IMAGE_DELAYLOAD_DESCRIPTOR* desc,
                                               PDELAYLOAD_FAILURE_DLL_CALLBACK dllhook,
                                               PDELAYLOAD_FAILURE_SYSTEM_ROUTINE syshook,
                                               IMAGE_THUNK_DATA* addr, ULONG flags );

static inline void *image_base(void)
{
#ifdef __WINE_PE_BUILD
    extern IMAGE_DOS_HEADER __ImageBase;
    return (void *)&__ImageBase;
#else
    extern IMAGE_NT_HEADERS __wine_spec_nt_header;
    return (void *)((__wine_spec_nt_header.OptionalHeader.ImageBase + 0xffff) & ~0xffff);
#endif
}

FARPROC WINAPI __delayLoadHelper2( const IMAGE_DELAYLOAD_DESCRIPTOR *descr, IMAGE_THUNK_DATA *addr )
{
    return ResolveDelayLoadedAPI( image_base(), descr, NULL, DelayLoadFailureHook, addr, 0 );
}
