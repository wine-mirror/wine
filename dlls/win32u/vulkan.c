/*
 * Vulkan display driver loading
 *
 * Copyright (c) 2017 Roderick Colenbrander
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

#include "ntgdi_private.h"

/***********************************************************************
 *      __wine_get_vulkan_driver  (win32u.@)
 */
const struct vulkan_funcs * CDECL __wine_get_vulkan_driver( UINT version )
{
    return user_driver->pwine_get_vulkan_driver( version );
}

/***********************************************************************
 *      __wine_direct_unix_call  (win32u.@)
 *
 * Used by winevulkan to do direct calls to Unix lib. We can't use __wine_unix_call
 * in cases that may end up calling display driver.
 */
NTSTATUS WINAPI __wine_direct_unix_call(unixlib_handle_t handle, unsigned int code, void *params)
{
    NTSTATUS (* HOSTPTR * HOSTPTR funcs)( void * HOSTPTR args ) = (void *)(UINT_PTR)handle;
    return funcs[code](params);
}
