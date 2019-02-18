/*
 * Copyright 2019 Jacek Caban for CodeWeavers
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

#include "windows.h"
#include "atlthunk.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(atlthunk);

AtlThunkData_t* WINAPI AtlThunk_AllocateData(void)
{
    FIXME("\n");
    return NULL;
}

WNDPROC WINAPI AtlThunk_DataToCode(AtlThunkData_t *thunk)
{
    FIXME("(%p)\n", thunk);
    return NULL;
}

void WINAPI AtlThunk_FreeData(AtlThunkData_t *thunk)
{
    FIXME("(%p)\n", thunk);
}

void WINAPI AtlThunk_InitData(AtlThunkData_t *thunk, void *proc, SIZE_T arg)
{
    FIXME("(%p %p %lx)\n", thunk, proc, arg);
}
