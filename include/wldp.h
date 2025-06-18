/*
 * Copyright 2023 Louis Lenders
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

#ifndef WLDP_H
#define WLDP_H

#define WLDP_LOCKDOWN_UNDEFINED             0x00000000
#define WLDP_LOCKDOWN_DEFINED_FLAG          0x80000000
#define WLDP_LOCKDOWN_CONFIG_CI_FLAG        0x00000001
#define WLDP_LOCKDOWN_CONFIG_CI_AUDIT_FLAG  0x00000002
#define WLDP_LOCKDOWN_UMCIENFORCE_FLAG      0x00000004
#define WLDP_LOCKDOWN_AUDIT_FLAG            0x00000008
#define WLDP_LOCKDOWN_EXCLUSION_FLAG        0x00000010

typedef enum WLDP_HOST_ID
{
  WLDP_HOST_ID_UNKNOWN,
  WLDP_HOST_ID_GLOBAL,
  WLDP_HOST_ID_VBA,
  WLDP_HOST_ID_WSH,
  WLDP_HOST_ID_POWERSHELL,
  WLDP_HOST_ID_IE,
  WLDP_HOST_ID_MSI,
  WLDP_HOST_ID_ALL,
  WLDP_HOST_ID_MAX
} WLDP_HOST_ID;

typedef enum _WLDP_WINDOWS_LOCKDOWN_MODE
{
  WLDP_WINDOWS_LOCKDOWN_MODE_UNLOCKED,
  WLDP_WINDOWS_LOCKDOWN_MODE_TRIAL,
  WLDP_WINDOWS_LOCKDOWN_MODE_LOCKED,
  WLDP_WINDOWS_LOCKDOWN_MODE_MAX
} WLDP_WINDOWS_LOCKDOWN_MODE, *PWLDP_WINDOWS_LOCKDOWN_MODE;

typedef struct WLDP_HOST_INFORMATION
{
  DWORD dwRevision;
  WLDP_HOST_ID dwHostId;
  const WCHAR* szSource;
  HANDLE hSource;
} WLDP_HOST_INFORMATION, *PWLDP_HOST_INFORMATION;

HRESULT WINAPI WldpGetLockdownPolicy(WLDP_HOST_INFORMATION*,DWORD*,DWORD);
HRESULT WINAPI WldpIsDynamicCodePolicyEnabled(BOOL*);
HRESULT WINAPI WldpQueryWindowsLockdownMode(WLDP_WINDOWS_LOCKDOWN_MODE*);

#endif
