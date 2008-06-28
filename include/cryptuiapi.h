/*
 * Copyright (C) 2008 Juan Lang
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
#ifndef __CRYPTUIAPI_H__
#define __CRYPTUIAPI_H__

#include <wintrust.h>
#include <wincrypt.h>
#include <prsht.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <pshpack8.h>

BOOL WINAPI CryptUIDlgViewContext(DWORD dwContextType, LPVOID pvContext,
 HWND hwnd, LPCWSTR pwszTitle, DWORD dwFlags, LPVOID pvReserved);

/* Values for dwDontUseColumn */
#define CRYPTUI_SELECT_ISSUEDTO_COLUMN     0x00000001
#define CRYPTUI_SELECT_ISSUEDBY_COLUMN     0x00000002
#define CRYPTUI_SELECT_INTENDEDUSE_COLUMN  0x00000004
#define CRYPTUI_SELECT_FRIENDLYNAME_COLUMN 0x00000008
#define CRYPTUI_SELECT_LOCATION_COLUMN     0x00000010
#define CRYPTUI_SELECT_EXPIRATION_COLUMN   0x00000020

PCCERT_CONTEXT WINAPI CryptUIDlgSelectCertificateFromStore(
 HCERTSTORE hCertStore, HWND hwnd, LPCWSTR pwszTitle, LPCWSTR pwszDisplayString,
 DWORD dwDontUseColumn, DWORD dwFlags, LPVOID pvReserved);

#include <poppack.h>

#ifdef __cplusplus
}
#endif

#endif
