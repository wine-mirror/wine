/*
 * WS2SPI.H -- definitions to be used with the WinSock service provider.
 *
 * Copyright (C) 2001 Patrik Stridvall
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _WINSOCK2SPI_
#define _WINSOCK2SPI_

#ifndef _WINSOCK2API_
#include <winsock2.h>
#endif /* !defined(_WINSOCK2API_) */

#include <pshpack4.h>

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

typedef BOOL (WINAPI *LPWPUPOSTMESSAGE)(HWND,UINT,WPARAM,LPARAM);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#include <poppack.h>

#endif /* !defined(_WINSOCK2SPI_) */
