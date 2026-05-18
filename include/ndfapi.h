/*
* Copyright 2026 Alistair Leslie-Hughes
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
#ifndef __NDFAPI_H__
#define __NDFAPI_H__

#include "ndattrib.h"

typedef PVOID NDFHANDLE;

STDAPI NdfCloseIncident(NDFHANDLE handle);
STDAPI NdfCreateConnectivityIncident(NDFHANDLE *handle);
STDAPI NdfCreateWebIncident(LPCWSTR url, NDFHANDLE *handle);
STDAPI NdfCreateWinSockIncident(SOCKET sock, LPCWSTR host, USHORT port, LPCWSTR appId, SID *userId, NDFHANDLE *handle);

#ifdef __cplusplus
STDAPI NdfDiagnoseIncident(NDFHANDLE Handle, ULONG *RootCauseCount, RootCauseInfo **RootCauses, DWORD dwWait=INFINITE, DWORD dwFlags=0);
#else
STDAPI NdfDiagnoseIncident(NDFHANDLE Handle, ULONG *RootCauseCount, RootCauseInfo **RootCauses, DWORD dwWait, DWORD dwFlags);
#endif

STDAPI NdfExecuteDiagnosis(NDFHANDLE handle, HWND hwnd);


#endif
