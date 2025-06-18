/*
 * Copyright (C) 2007 Francois Gouget
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

#ifndef __WINE_USERENV_H
#define __WINE_USERENV_H

#include <wbemcli.h>
#include <profinfo.h>

#ifdef _USERENV_
#define USERENVAPI
#else
#define USERENVAPI DECLSPEC_IMPORT
#endif

#define PT_TEMPORARY    0x00000001
#define PT_ROAMING      0x00000002
#define PT_MANDATORY    0x00000004

typedef enum _GPO_LINK {
    GPLinkUnknown = 0,
    GPLinkMachine,
    GPLinkSite,
    GPLinkDomain,
    GPLinkOrganizationalUnit
} GPO_LINK, *PGPO_LINK;

typedef struct _GROUP_POLICY_OBJECTA {
    DWORD dwOptions;
    DWORD dwVersion;
    LPSTR lpDSPath;
    LPSTR lpFileSysPath;
    LPSTR lpDisplayName;
    CHAR szGPOName[50];
    GPO_LINK GPOLink;
    LPARAM lParam;
    struct _GROUP_POLICY_OBJECTA *pNext;
    struct _GROUP_POLICY_OBJECTA *pPrev;
    LPSTR lpExtensions;
    LPARAM lParam2;
    LPSTR lpLink;
} GROUP_POLICY_OBJECTA, *PGROUP_POLICY_OBJECTA;

typedef struct _GROUP_POLICY_OBJECTW {
    DWORD dwOptions;
    DWORD dwVersion;
    LPWSTR lpDSPath;
    LPWSTR lpFileSysPath;
    LPWSTR lpDisplayName;
    WCHAR szGPOName[50];
    GPO_LINK GPOLink;
    LPARAM lParam;
    struct _GROUP_POLICY_OBJECTW *pNext;
    struct _GROUP_POLICY_OBJECTW *pPrev;
    LPWSTR lpExtensions;
    LPARAM lParam2;
    LPWSTR lpLink;
} GROUP_POLICY_OBJECTW, *PGROUP_POLICY_OBJECTW;

DECL_WINELIB_TYPE_AW(GROUP_POLICY_OBJECT)
DECL_WINELIB_TYPE_AW(PGROUP_POLICY_OBJECT)

#ifdef __cplusplus
extern "C" {
#endif

USERENVAPI BOOL WINAPI CreateEnvironmentBlock(LPVOID*,HANDLE,BOOL);
USERENVAPI BOOL WINAPI DestroyEnvironmentBlock(LPVOID);
USERENVAPI HANDLE WINAPI EnterCriticalPolicySection(BOOL);
USERENVAPI BOOL WINAPI ExpandEnvironmentStringsForUserA(HANDLE,LPCSTR,LPSTR,DWORD);
USERENVAPI BOOL WINAPI ExpandEnvironmentStringsForUserW(HANDLE,LPCWSTR,LPWSTR,DWORD);
#define                ExpandEnvironmentStringsForUser WINELIB_NAME_AW(ExpandEnvironmentStringsForUser)
USERENVAPI DWORD WINAPI GetAppliedGPOListW(DWORD,LPCWSTR,PSID,GUID*,PGROUP_POLICY_OBJECTW*);
USERENVAPI DWORD WINAPI GetAppliedGPOListA(DWORD,LPCSTR,PSID,GUID*,PGROUP_POLICY_OBJECTA*);
#define                 GetAppliedGPOList WINELIB_NAME_AW(GetAppliedGPOList)
USERENVAPI BOOL WINAPI GetUserProfileDirectoryA(HANDLE,LPSTR,LPDWORD);
USERENVAPI BOOL WINAPI GetUserProfileDirectoryW(HANDLE,LPWSTR,LPDWORD);
#define                GetUserProfileDirectory WINELIB_NAME_AW(GetUserProfileDirectory)
USERENVAPI BOOL WINAPI GetProfilesDirectoryA(LPSTR,LPDWORD);
USERENVAPI BOOL WINAPI GetProfilesDirectoryW(LPWSTR,LPDWORD);
#define                GetProfilesDirectory WINELIB_NAME_AW(GetProfilesDirectory)
USERENVAPI BOOL WINAPI GetAllUsersProfileDirectoryA(LPSTR,LPDWORD);
USERENVAPI BOOL WINAPI GetAllUsersProfileDirectoryW(LPWSTR,LPDWORD);
#define                GetAllUsersProfileDirectory WINELIB_NAME_AW(GetAllUsersProfileDirectory)
USERENVAPI BOOL WINAPI GetProfileType(DWORD*);
USERENVAPI BOOL WINAPI LeaveCriticalPolicySection(HANDLE);
USERENVAPI BOOL WINAPI LoadUserProfileA(HANDLE,LPPROFILEINFOA);
USERENVAPI BOOL WINAPI LoadUserProfileW(HANDLE,LPPROFILEINFOW);
#define                LoadUserProfile WINELIB_NAME_AW(LoadUserProfile)
USERENVAPI BOOL WINAPI RegisterGPNotification(HANDLE,BOOL);
USERENVAPI BOOL WINAPI UnloadUserProfile(HANDLE,HANDLE);
USERENVAPI BOOL WINAPI UnregisterGPNotification(HANDLE);

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_USERENV_H */
