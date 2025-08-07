/*
 *    Copyright (c) 2000 Lionel Ulmer
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
#ifndef __WINE_OPENGL32_UNIX_PRIVATE_H
#define __WINE_OPENGL32_UNIX_PRIVATE_H

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wingdi.h"
#include "ntgdi.h"

#include "wine/opengl_driver.h"
#include "unix_thunks.h"

struct registry_entry
{
    const char *name;      /* name of the extension */
    const char *extension; /* name of the GL/WGL extension */
    size_t offset;         /* offset in the opengl_funcs table */
};

extern const struct registry_entry extension_registry[];
extern const int extension_registry_size;

extern struct opengl_funcs null_opengl_funcs;

static inline const struct opengl_funcs *get_dc_funcs( HDC hdc )
{
    const struct opengl_funcs *funcs = __wine_get_wgl_driver( hdc, WINE_OPENGL_DRIVER_VERSION, &null_opengl_funcs );
    if (!funcs) RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
    return funcs;
}

#ifdef _WIN64

static inline void *copy_wow64_ptr32s( UINT_PTR address, ULONG count )
{
    ULONG *ptrs = (ULONG *)address;
    void **tmp;

    if (!ptrs || !(tmp = calloc( count, sizeof(*tmp) ))) return NULL;
    while (count--) tmp[count] = ULongToPtr(ptrs[count]);
    return tmp;
}

static inline TEB *get_teb64( ULONG teb32 )
{
    TEB32 *teb32_ptr = ULongToPtr( teb32 );
    return (TEB *)((char *)teb32_ptr + teb32_ptr->WowTebOffset);
}

extern NTSTATUS return_wow64_string( const void *str, PTR32 *wow64_str );

#endif

extern pthread_mutex_t wgl_lock;

extern NTSTATUS process_attach( void *args );
extern NTSTATUS thread_attach( void *args );
extern NTSTATUS process_detach( void *args );
extern NTSTATUS get_pixel_formats( void *args );
extern void set_context_attribute( TEB *teb, GLenum name, const void *value, size_t size );

#endif /* __WINE_OPENGL32_UNIX_PRIVATE_H */
