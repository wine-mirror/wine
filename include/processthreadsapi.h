/*
 * Copyright 2021 Brendan Shanks for CodeWeavers
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

#ifndef _PROCESSTHREADSAPI_H
#define _PROCESSTHREADSAPI_H

#ifdef __cplusplus
extern "C" {
#endif

WINBASEAPI HRESULT WINAPI GetThreadDescription(HANDLE,PWSTR *);
WINBASEAPI HRESULT WINAPI SetThreadDescription(HANDLE,PCWSTR);

#ifdef __cplusplus
}
#endif

#endif  /* _PROCESSTHREADSAPI_H */
