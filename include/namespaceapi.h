/*
 * Copyright (C) 2021 Mohamad Al-Jaf
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

#ifndef _NAMESPACEAPI_H_
#define _NAMESPACEAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#define PRIVATE_NAMESPACE_FLAG_DESTROY 0x00000001

WINBASEAPI BOOL    WINAPI AddSIDToBoundaryDescriptor(HANDLE*,PSID);
WINBASEAPI BOOLEAN WINAPI ClosePrivateNamespace(HANDLE,ULONG);
WINBASEAPI HANDLE  WINAPI CreateBoundaryDescriptorW(LPCWSTR,ULONG);
WINBASEAPI HANDLE  WINAPI CreatePrivateNamespaceW(LPSECURITY_ATTRIBUTES,LPVOID,LPCWSTR);
WINBASEAPI void    WINAPI DeleteBoundaryDescriptor(HANDLE);
WINBASEAPI HANDLE  WINAPI OpenPrivateNamespaceW(LPVOID,LPCWSTR);

#ifdef __cplusplus
}
#endif

#endif /* _NAMESPACEAPI_H_ */
