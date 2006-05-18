/*
 * Copyright (C) 2004 Mike McCormack
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

#ifndef __WINE_ACLAPI_H
#define __WINE_ACLAPI_H

#ifndef __WINESRC__
# include <windows.h>
#endif
#include <accctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

DWORD WINAPI GetExplicitEntriesFromAclA( PACL, PULONG, PEXPLICIT_ACCESS_A* );
DWORD WINAPI GetExplicitEntriesFromAclW( PACL, PULONG, PEXPLICIT_ACCESS_W* );
#define     GetExplicitEntriesFromAcl WINELIB_NAME_AW(GetExplicitEntriesFromAcl)

DWORD WINAPI GetSecurityInfo( HANDLE, SE_OBJECT_TYPE, SECURITY_INFORMATION,
        PSID*, PSID*, PACL*, PACL*, PSECURITY_DESCRIPTOR*);

DWORD WINAPI GetSecurityInfoExA(HANDLE, SE_OBJECT_TYPE, SECURITY_INFORMATION,
        LPCSTR, LPCSTR, PACTRL_ACCESSA*, PACTRL_AUDITA*, LPSTR*, LPSTR*);
DWORD WINAPI GetSecurityInfoExW(HANDLE, SE_OBJECT_TYPE, SECURITY_INFORMATION,
        LPCWSTR, LPCWSTR, PACTRL_ACCESSW*, PACTRL_AUDITW*, LPWSTR*, LPWSTR*);
#define     GetSecurityInfoEx WINELIB_NAME_AW(GetSecurityInfoEx)

DWORD WINAPI GetNamedSecurityInfoA(LPSTR, SE_OBJECT_TYPE, SECURITY_INFORMATION,
        PSID*, PSID*, PACL*, PACL*, PSECURITY_DESCRIPTOR*);
DWORD WINAPI GetNamedSecurityInfoW(LPWSTR, SE_OBJECT_TYPE, SECURITY_INFORMATION,
        PSID*, PSID*, PACL*, PACL*, PSECURITY_DESCRIPTOR*);
#define     GetNamedSecurityInfo WINELIB_NAME_AW(GetNamedSecurityInfo)

DWORD WINAPI SetNamedSecurityInfoA(LPSTR, SE_OBJECT_TYPE,
         SECURITY_INFORMATION, PSID, PSID, PACL, PACL);
DWORD WINAPI SetNamedSecurityInfoW(LPWSTR, SE_OBJECT_TYPE,
         SECURITY_INFORMATION, PSID, PSID, PACL, PACL);
#define     SetNamedSecurityInfo WINELIB_NAME_AW(SetNamedSecurityInfo)

DWORD WINAPI SetEntriesInAclA( ULONG, PEXPLICIT_ACCESSA, PACL, PACL*);
DWORD WINAPI SetEntriesInAclW( ULONG, PEXPLICIT_ACCESSW, PACL, PACL*);
#define     SetEntriesInAcl WINELIB_NAME_AW(SetEntriesInAcl)

DWORD WINAPI SetSecurityInfo(HANDLE, SE_OBJECT_TYPE, SECURITY_INFORMATION,
                             PSID, PSID, PACL, PACL);

TRUSTEE_FORM WINAPI GetTrusteeFormA( PTRUSTEEA );
TRUSTEE_FORM WINAPI GetTrusteeFormW( PTRUSTEEW );
#define     GetTrusteeForm WINELIB_NAME_AW(GetTrusteeForm)

LPSTR WINAPI GetTrusteeNameA( PTRUSTEEA );
LPWSTR WINAPI GetTrusteeNameW( PTRUSTEEW );
#define     GetTrusteeName WINELIB_NAME_AW(GetTrusteeName)

TRUSTEE_TYPE WINAPI GetTrusteeTypeA( PTRUSTEEA );
TRUSTEE_TYPE WINAPI GetTrusteeTypeW( PTRUSTEEW );
#define     GetTrusteeType WINELIB_NAME_AW(GetTrusteeType)

DWORD WINAPI BuildSecurityDescriptorA( PTRUSTEEA, PTRUSTEEA, ULONG, PEXPLICIT_ACCESS_A,
                                       ULONG, PEXPLICIT_ACCESS_A,
                                       PSECURITY_DESCRIPTOR, PULONG,
                                       PSECURITY_DESCRIPTOR* );
DWORD WINAPI BuildSecurityDescriptorW( PTRUSTEEW, PTRUSTEEW, ULONG, PEXPLICIT_ACCESS_W,
                                       ULONG, PEXPLICIT_ACCESS_W,
                                       PSECURITY_DESCRIPTOR, PULONG,
                                       PSECURITY_DESCRIPTOR* );
#define     BuildSecurityDescriptor WINELIB_NAME_AW(BuildSecurityDescriptor)

void WINAPI BuildTrusteeWithNameA( PTRUSTEEA, LPSTR );
void WINAPI BuildTrusteeWithNameW( PTRUSTEEW, LPWSTR );
#define     BuildTrusteeWithName WINELIB_NAME_AW(BuildTrusteeWithName)

VOID WINAPI BuildTrusteeWithObjectsAndNameA(PTRUSTEEA, POBJECTS_AND_NAME_A,
                                            SE_OBJECT_TYPE, LPSTR, LPSTR, LPSTR);
VOID WINAPI BuildTrusteeWithObjectsAndNameW(PTRUSTEEW, POBJECTS_AND_NAME_W,
                                            SE_OBJECT_TYPE, LPWSTR, LPWSTR, LPWSTR);
#define     BuildTrusteeWithObjectsAndName WINELIB_NAME_AW(BuildTrusteeWithObjectsAndName)

VOID WINAPI BuildTrusteeWithObjectsAndSidA(PTRUSTEEA, POBJECTS_AND_SID, GUID*, GUID*, PSID);
VOID WINAPI BuildTrusteeWithObjectsAndSidW(PTRUSTEEW, POBJECTS_AND_SID, GUID*, GUID*, PSID);
#define     BuildTrusteeWithObjectsAndSid WINELIB_NAME_AW(BuildTrusteeWithObjectsAndSid)

VOID WINAPI BuildExplicitAccessWithNameA(PEXPLICIT_ACCESSA, LPSTR, DWORD, ACCESS_MODE, DWORD);
VOID WINAPI BuildExplicitAccessWithNameW(PEXPLICIT_ACCESSW, LPWSTR, DWORD, ACCESS_MODE, DWORD);
#define     BuildExplicitAccessWithName WINELIB_NAME_AW(BuildExplicitAccessWithName)

VOID WINAPI BuildTrusteeWithSidA(PTRUSTEEA pTrustee, PSID pSid);
VOID WINAPI BuildTrusteeWithSidW(PTRUSTEEW pTrustee, PSID pSid);
#define     BuildTrusteeWithSid WINELIB_NAME_AW(BuildTrusteeWithSid)

DWORD WINAPI GetEffectiveRightsFromAclA(PACL,PTRUSTEE_A,PACCESS_MASK);
DWORD WINAPI GetEffectiveRightsFromAclW(PACL,PTRUSTEE_W,PACCESS_MASK);
#define      GetEffectiveRightsFromAcl WINELIB_NAME_AW(GetEffectiveRightsFromAcl)

#ifdef __cplusplus
}
#endif

#endif /* __WINE_ACLAPI_H */
