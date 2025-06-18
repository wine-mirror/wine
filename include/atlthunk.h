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

#ifndef __ATLTHUNK_H__
#define __ATLTHUNK_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AtlThunkData_t AtlThunkData_t;

AtlThunkData_t* WINAPI AtlThunk_AllocateData(void);
WNDPROC WINAPI AtlThunk_DataToCode(AtlThunkData_t*);
void WINAPI AtlThunk_FreeData(AtlThunkData_t*);
void WINAPI AtlThunk_InitData(AtlThunkData_t*,void*,SIZE_T);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __ATLTHUNK_H__ */
