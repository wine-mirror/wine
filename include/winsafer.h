/*
 * Winsafer definitions
 *
 * Copyright (C) 2009 Nikolay Sivov
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

#ifndef __WINE_WINSAFER_H
#define __WINE_WINSAFER_H

#include <guiddef.h>
#include <wincrypt.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_HANDLE(SAFER_LEVEL_HANDLE);

#define SAFER_TOKEN_NULL_IF_EQUAL   0x00000001
#define SAFER_TOKEN_COMPARE_ONLY    0x00000002
#define SAFER_TOKEN_MAKE_INERT      0x00000004
#define SAFER_TOKEN_WANT_FLAGS      0x00000008

#define SAFER_MAX_HASH_SIZE 64

#pragma pack(push,8)
typedef struct _SAFER_CODE_PROPERTIES_V2 {
    DWORD cbSize;
    DWORD dwCheckFlags;
    LPCWSTR ImagePath;
    HANDLE hImageFileHandle;
    DWORD UrlZoneId;
    BYTE ImageHash[SAFER_MAX_HASH_SIZE];
    DWORD dwImageHashSize;
    LARGE_INTEGER ImageSize;
    ALG_ID HashAlgorithm;
    LPBYTE pByteBlock;
    HWND hWndParent;
    DWORD dwWVTUIChoice;
    LPCWSTR PackageMoniker;
    LPCWSTR PackagePublisher;
    LPCWSTR PackageName;
    ULONG64 PackageVersion;
    BOOL PackageIsFramework;
} SAFER_CODE_PROPERTIES_V2, *PSAFER_CODE_PROPERTIES_V2;

typedef SAFER_CODE_PROPERTIES_V2 SAFER_CODE_PROPERTIES, *PSAFER_CODE_PROPERTIES;
#pragma pack(pop)

#define SAFER_SCOPEID_MACHINE  1
#define SAFER_SCOPEID_USER     2

#define SAFER_LEVELID_DISALLOWED   0x00000
#define SAFER_LEVELID_UNTRUSTED    0x01000
#define SAFER_LEVELID_CONSTRAINED  0x10000
#define SAFER_LEVELID_NORMALUSER   0x20000
#define SAFER_LEVELID_FULLYTRUSTED 0x40000

#define SAFER_LEVEL_OPEN   1

typedef enum _SAFER_POLICY_INFO_CLASS {
    SaferPolicyLevelList = 1,
    SaferPolicyEnableTransparentEnforcement,
    SaferPolicyDefaultLevel,
    SaferPolicyEvaluateUserScope,
    SaferPolicyScopeFlags
} SAFER_POLICY_INFO_CLASS;

typedef enum _SAFER_OBJECT_INFO_CLASS {
    SaferObjectLevelId = 1,
    SaferObjectScopeId,
    SaferObjectFriendlyName,
    SaferObjectDescription,
    SaferObjectBuiltin,
    SaferObjectDisallowed,
    SaferObjectDisableMaxPrivilege,
    SaferObjectInvertDeletedPrivileges,
    SaferObjectDeletedPrivileges,
    SaferObjectDefaultOwner,
    SaferObjectSidsToDisable,
    SaferObjectRestrictedSidsInverted,
    SaferObjectRestrictedSidsAdded,
    SaferObjectAllIdentificationGuids,
    SaferObjectSingleIdentification,
    SaferObjectExtendedError
} SAFER_OBJECT_INFO_CLASS;

WINADVAPI BOOL WINAPI SaferCreateLevel(DWORD,DWORD,DWORD,SAFER_LEVEL_HANDLE*,LPVOID);
WINADVAPI BOOL WINAPI SaferGetPolicyInformation(DWORD,SAFER_POLICY_INFO_CLASS,DWORD,PVOID,PDWORD,LPVOID);
WINADVAPI BOOL WINAPI SaferIdentifyLevel(DWORD,PSAFER_CODE_PROPERTIES,SAFER_LEVEL_HANDLE*,LPVOID);
WINADVAPI BOOL WINAPI SaferSetLevelInformation(SAFER_LEVEL_HANDLE,SAFER_OBJECT_INFO_CLASS,LPVOID,DWORD);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_WINSAFER_H */
