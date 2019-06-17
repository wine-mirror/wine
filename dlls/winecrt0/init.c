/*
 * Initialization code for spec files
 *
 * Copyright 2005 Alexandre Julliard
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
#include "windef.h"
#include "winbase.h"
#include "wine/library.h"
#include "crt0_private.h"

enum init_state __wine_spec_init_state = NO_INIT_DONE;

extern const IMAGE_NT_HEADERS __wine_spec_nt_header;
extern const char __wine_spec_file_name[];

void DECLSPEC_HIDDEN __wine_spec_init(void)
{
    __wine_spec_init_state = DLL_REGISTERED;
    __wine_dll_register( &__wine_spec_nt_header, __wine_spec_file_name );
}

void DECLSPEC_HIDDEN __wine_spec_init_ctor(void)
{
    if (__wine_spec_init_state == NO_INIT_DONE) __wine_spec_init();
    __wine_spec_init_state = CONSTRUCTORS_DONE;
}
