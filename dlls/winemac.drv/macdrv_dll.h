/*
 * MAC driver definitions
 *
 * Copyright 2022 Jacek Caban for CodeWeavers
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

#ifndef __WINE_MACDRV_DLL_H
#define __WINE_MACDRV_DLL_H

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"
#include "unixlib.h"

extern NTSTATUS WINAPI macdrv_dnd_query_drag(void *arg, ULONG size);
extern NTSTATUS WINAPI macdrv_dnd_query_drop(void *arg, ULONG size);
extern NTSTATUS WINAPI macdrv_dnd_query_exited(void *arg, ULONG size);

extern HMODULE macdrv_module;

#endif /* __WINE_MACDRV_DLL_H */
