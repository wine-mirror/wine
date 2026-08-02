/*
 * Copyright (C) 2026 Alistair Leslie-Hughes
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
#ifndef __FLTUSER_H__
#define __FLTUSER_H__

#include <fltuserstructures.h>

#ifdef __cplusplus
extern "C" {
#endif


HRESULT WINAPI FilterConnectCommunicationPort(LPCWSTR, DWORD, LPVOID, DWORD, LPSECURITY_ATTRIBUTES, HANDLE *);
HRESULT WINAPI FilterLoad(LPCWSTR);
HRESULT WINAPI FilterFindClose(HANDLE);
HRESULT WINAPI FilterFindFirst(DWORD, LPVOID, DWORD, LPDWORD, LPHANDLE);
HRESULT WINAPI FilterUnload(LPCWSTR);

#ifdef __cplusplus
}
#endif


#endif
