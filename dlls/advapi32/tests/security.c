/*
 * Unit tests for security functions
 *
 * Copyright (c) 2004 Mike McCormack
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

#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "aclapi.h"
#include "winnt.h"
#include "winternl.h"
#include "sddl.h"
#include "ntsecapi.h"

#include "wine/test.h"

typedef VOID (WINAPI *fnBuildTrusteeWithSidA)( PTRUSTEEA pTrustee, PSID pSid );
typedef VOID (WINAPI *fnBuildTrusteeWithNameA)( PTRUSTEEA pTrustee, LPSTR pName );
typedef VOID (WINAPI *fnBuildTrusteeWithObjectsAndNameA)( PTRUSTEEA pTrustee,
                                                          POBJECTS_AND_NAME_A pObjName,
                                                          SE_OBJECT_TYPE ObjectType,
                                                          LPSTR ObjectTypeName,
                                                          LPSTR InheritedObjectTypeName,
                                                          LPSTR Name );
typedef VOID (WINAPI *fnBuildTrusteeWithObjectsAndSidA)( PTRUSTEEA pTrustee,
                                                         POBJECTS_AND_SID pObjSid,
                                                         GUID* pObjectGuid,
                                                         GUID* pInheritedObjectGuid,
                                                         PSID pSid );
typedef LPSTR (WINAPI *fnGetTrusteeNameA)( PTRUSTEEA pTrustee );
typedef BOOL (WINAPI *fnConvertSidToStringSidA)( PSID pSid, LPSTR *str );
typedef BOOL (WINAPI *fnConvertStringSidToSidA)( LPCSTR str, PSID pSid );
typedef BOOL (WINAPI *fnGetFileSecurityA)(LPCSTR, SECURITY_INFORMATION,
                                          PSECURITY_DESCRIPTOR, DWORD, LPDWORD);
typedef DWORD (WINAPI *fnRtlAdjustPrivilege)(ULONG,BOOLEAN,BOOLEAN,PBOOLEAN);
typedef BOOL (WINAPI *fnCreateWellKnownSid)(WELL_KNOWN_SID_TYPE,PSID,PSID,DWORD*);

typedef NTSTATUS (WINAPI *fnLsaQueryInformationPolicy)(LSA_HANDLE,POLICY_INFORMATION_CLASS,PVOID*);
typedef NTSTATUS (WINAPI *fnLsaClose)(LSA_HANDLE);
typedef NTSTATUS (WINAPI *fnLsaFreeMemory)(PVOID);
typedef NTSTATUS (WINAPI *fnLsaOpenPolicy)(PLSA_UNICODE_STRING,PLSA_OBJECT_ATTRIBUTES,ACCESS_MASK,PLSA_HANDLE);

static HMODULE hmod;

fnBuildTrusteeWithSidA   pBuildTrusteeWithSidA;
fnBuildTrusteeWithNameA  pBuildTrusteeWithNameA;
fnBuildTrusteeWithObjectsAndNameA pBuildTrusteeWithObjectsAndNameA;
fnBuildTrusteeWithObjectsAndSidA pBuildTrusteeWithObjectsAndSidA;
fnGetTrusteeNameA pGetTrusteeNameA;
fnConvertSidToStringSidA pConvertSidToStringSidA;
fnConvertStringSidToSidA pConvertStringSidToSidA;
fnGetFileSecurityA pGetFileSecurityA;
fnRtlAdjustPrivilege pRtlAdjustPrivilege;
fnCreateWellKnownSid pCreateWellKnownSid;
fnLsaQueryInformationPolicy pLsaQueryInformationPolicy;
fnLsaClose pLsaClose;
fnLsaFreeMemory pLsaFreeMemory;
fnLsaOpenPolicy pLsaOpenPolicy;

struct sidRef
{
    SID_IDENTIFIER_AUTHORITY auth;
    const char *refStr;
};

static void init(void)
{
    hmod = GetModuleHandle("advapi32.dll");
}

static void test_str_sid(const char *str_sid)
{
    PSID psid;
    char *temp;

    if (pConvertStringSidToSidA(str_sid, &psid))
    {
        if (pConvertSidToStringSidA(psid, &temp))
        {
            trace(" %s: %s\n", str_sid, temp);
            LocalFree(temp);
        }
    }
    else
        trace("%s couldn't be converted, returned %ld\n", str_sid, GetLastError());
}

static void test_sid(void)
{
    struct sidRef refs[] = {
     { { {0x00,0x00,0x33,0x44,0x55,0x66} }, "S-1-860116326-1" },
     { { {0x00,0x00,0x01,0x02,0x03,0x04} }, "S-1-16909060-1"  },
     { { {0x00,0x00,0x00,0x01,0x02,0x03} }, "S-1-66051-1"     },
     { { {0x00,0x00,0x00,0x00,0x01,0x02} }, "S-1-258-1"       },
     { { {0x00,0x00,0x00,0x00,0x00,0x02} }, "S-1-2-1"         },
     { { {0x00,0x00,0x00,0x00,0x00,0x0c} }, "S-1-12-1"        },
    };
    const char noSubAuthStr[] = "S-1-5";
    unsigned int i;
    PSID psid = NULL;
    BOOL r;
    LPSTR str = NULL;

    pConvertSidToStringSidA = (fnConvertSidToStringSidA)
                    GetProcAddress( hmod, "ConvertSidToStringSidA" );
    if( !pConvertSidToStringSidA )
        return;
    pConvertStringSidToSidA = (fnConvertStringSidToSidA)
                    GetProcAddress( hmod, "ConvertStringSidToSidA" );
    if( !pConvertStringSidToSidA )
        return;

    r = pConvertStringSidToSidA( NULL, NULL );
    ok( !r, "expected failure with NULL parameters\n" );
    if( GetLastError() == ERROR_CALL_NOT_IMPLEMENTED )
        return;
    ok( GetLastError() == ERROR_INVALID_PARAMETER,
     "expected GetLastError() is ERROR_INVALID_PARAMETER, got %ld\n",
     GetLastError() );

    r = pConvertStringSidToSidA( refs[0].refStr, NULL );
    ok( !r && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected GetLastError() is ERROR_INVALID_PARAMETER, got %ld\n",
     GetLastError() );

    r = pConvertStringSidToSidA( NULL, &str );
    ok( !r && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected GetLastError() is ERROR_INVALID_PARAMETER, got %ld\n",
     GetLastError() );

    r = pConvertStringSidToSidA( noSubAuthStr, &psid );
    ok( !r,
     "expected failure with no sub authorities\n" );
    ok( GetLastError() == ERROR_INVALID_SID,
     "expected GetLastError() is ERROR_INVALID_SID, got %ld\n",
     GetLastError() );

    for( i = 0; i < sizeof(refs) / sizeof(refs[0]); i++ )
    {
        PISID pisid;

        r = AllocateAndInitializeSid( &refs[i].auth, 1,1,0,0,0,0,0,0,0,
         &psid );
        ok( r, "failed to allocate sid\n" );
        r = pConvertSidToStringSidA( psid, &str );
        ok( r, "failed to convert sid\n" );
        if (r)
        {
            ok( !strcmp( str, refs[i].refStr ),
                "incorrect sid, expected %s, got %s\n", refs[i].refStr, str );
            LocalFree( str );
        }
        if( psid )
            FreeSid( psid );

        r = pConvertStringSidToSidA( refs[i].refStr, &psid );
        ok( r, "failed to parse sid string\n" );
        pisid = (PISID)psid;
        ok( pisid &&
         !memcmp( pisid->IdentifierAuthority.Value, refs[i].auth.Value,
         sizeof(refs[i].auth) ),
         "string sid %s didn't parse to expected value\n"
         "(got 0x%04x%08lx, expected 0x%04x%08lx)\n",
         refs[i].refStr,
         MAKEWORD( pisid->IdentifierAuthority.Value[1],
         pisid->IdentifierAuthority.Value[0] ),
         MAKELONG( MAKEWORD( pisid->IdentifierAuthority.Value[5],
         pisid->IdentifierAuthority.Value[4] ),
         MAKEWORD( pisid->IdentifierAuthority.Value[3],
         pisid->IdentifierAuthority.Value[2] ) ),
         MAKEWORD( refs[i].auth.Value[1], refs[i].auth.Value[0] ),
         MAKELONG( MAKEWORD( refs[i].auth.Value[5], refs[i].auth.Value[4] ),
         MAKEWORD( refs[i].auth.Value[3], refs[i].auth.Value[2] ) ) );
        if( psid )
            LocalFree( psid );
    }

    trace("String SIDs:\n");
    test_str_sid("AO");
    test_str_sid("RU");
    test_str_sid("AN");
    test_str_sid("AU");
    test_str_sid("BA");
    test_str_sid("BG");
    test_str_sid("BO");
    test_str_sid("BU");
    test_str_sid("CA");
    test_str_sid("CG");
    test_str_sid("CO");
    test_str_sid("DA");
    test_str_sid("DC");
    test_str_sid("DD");
    test_str_sid("DG");
    test_str_sid("DU");
    test_str_sid("EA");
    test_str_sid("ED");
    test_str_sid("WD");
    test_str_sid("PA");
    test_str_sid("IU");
    test_str_sid("LA");
    test_str_sid("LG");
    test_str_sid("LS");
    test_str_sid("SY");
    test_str_sid("NU");
    test_str_sid("NO");
    test_str_sid("NS");
    test_str_sid("PO");
    test_str_sid("PS");
    test_str_sid("PU");
    test_str_sid("RS");
    test_str_sid("RD");
    test_str_sid("RE");
    test_str_sid("RC");
    test_str_sid("SA");
    test_str_sid("SO");
    test_str_sid("SU");
}

static void test_trustee(void)
{
    GUID ObjectType = {0x12345678, 0x1234, 0x5678, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}};
    GUID InheritedObjectType = {0x23456789, 0x2345, 0x6786, {0x2, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99}};
    GUID ZeroGuid;
    OBJECTS_AND_NAME_ oan;
    OBJECTS_AND_SID oas;
    TRUSTEE trustee;
    PSID psid;
    char szObjectTypeName[] = "ObjectTypeName";
    char szInheritedObjectTypeName[] = "InheritedObjectTypeName";
    char szTrusteeName[] = "szTrusteeName";
    SID_IDENTIFIER_AUTHORITY auth = { {0x11,0x22,0,0,0, 0} };

    memset( &ZeroGuid, 0x00, sizeof (ZeroGuid) );

    pBuildTrusteeWithSidA = (fnBuildTrusteeWithSidA)
                    GetProcAddress( hmod, "BuildTrusteeWithSidA" );
    pBuildTrusteeWithNameA = (fnBuildTrusteeWithNameA)
                    GetProcAddress( hmod, "BuildTrusteeWithNameA" );
    pBuildTrusteeWithObjectsAndNameA = (fnBuildTrusteeWithObjectsAndNameA)
                    GetProcAddress (hmod, "BuildTrusteeWithObjectsAndNameA" );
    pBuildTrusteeWithObjectsAndSidA = (fnBuildTrusteeWithObjectsAndSidA)
                    GetProcAddress (hmod, "BuildTrusteeWithObjectsAndSidA" );
    pGetTrusteeNameA = (fnGetTrusteeNameA)
                    GetProcAddress (hmod, "GetTrusteeNameA" );
    if( !pBuildTrusteeWithSidA || !pBuildTrusteeWithNameA ||
        !pBuildTrusteeWithObjectsAndNameA || !pBuildTrusteeWithObjectsAndSidA ||
        !pGetTrusteeNameA )
        return;

    if ( ! AllocateAndInitializeSid( &auth, 1, 42, 0,0,0,0,0,0,0,&psid ) )
    {
        trace( "failed to init SID\n" );
       return;
    }

    /* test BuildTrusteeWithSidA */
    memset( &trustee, 0xff, sizeof trustee );
    pBuildTrusteeWithSidA( &trustee, psid );

    ok( trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok( trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, 
        "MultipleTrusteeOperation wrong\n");
    ok( trustee.TrusteeForm == TRUSTEE_IS_SID, "TrusteeForm wrong\n");
    ok( trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok( trustee.ptstrName == (LPSTR) psid, "ptstrName wrong\n" );

    /* test BuildTrusteeWithObjectsAndSidA (test 1) */
    memset( &trustee, 0xff, sizeof trustee );
    memset( &oas, 0xff, sizeof(oas) );
    pBuildTrusteeWithObjectsAndSidA(&trustee, &oas, &ObjectType,
                                    &InheritedObjectType, psid);

    ok(trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok(trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, "MultipleTrusteeOperation wrong\n");
    ok(trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_SID, "TrusteeForm wrong\n");
    ok(trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok(trustee.ptstrName == (LPSTR)&oas, "ptstrName wrong\n");
 
    ok(oas.ObjectsPresent == (ACE_OBJECT_TYPE_PRESENT | ACE_INHERITED_OBJECT_TYPE_PRESENT), "ObjectsPresent wrong\n");
    ok(!memcmp(&oas.ObjectTypeGuid, &ObjectType, sizeof(GUID)), "ObjectTypeGuid wrong\n");
    ok(!memcmp(&oas.InheritedObjectTypeGuid, &InheritedObjectType, sizeof(GUID)), "InheritedObjectTypeGuid wrong\n");
    ok(oas.pSid == psid, "pSid wrong\n");

    /* test GetTrusteeNameA */
    ok(pGetTrusteeNameA(&trustee) == (LPSTR)&oas, "GetTrusteeName returned wrong value\n");

    /* test BuildTrusteeWithObjectsAndSidA (test 2) */
    memset( &trustee, 0xff, sizeof trustee );
    memset( &oas, 0xff, sizeof(oas) );
    pBuildTrusteeWithObjectsAndSidA(&trustee, &oas, NULL,
                                    &InheritedObjectType, psid);

    ok(trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok(trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, "MultipleTrusteeOperation wrong\n");
    ok(trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_SID, "TrusteeForm wrong\n");
    ok(trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok(trustee.ptstrName == (LPSTR)&oas, "ptstrName wrong\n");
 
    ok(oas.ObjectsPresent == ACE_INHERITED_OBJECT_TYPE_PRESENT, "ObjectsPresent wrong\n");
    ok(!memcmp(&oas.ObjectTypeGuid, &ZeroGuid, sizeof(GUID)), "ObjectTypeGuid wrong\n");
    ok(!memcmp(&oas.InheritedObjectTypeGuid, &InheritedObjectType, sizeof(GUID)), "InheritedObjectTypeGuid wrong\n");
    ok(oas.pSid == psid, "pSid wrong\n");

    FreeSid( psid );

    /* test BuildTrusteeWithNameA */
    memset( &trustee, 0xff, sizeof trustee );
    pBuildTrusteeWithNameA( &trustee, szTrusteeName );

    ok( trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok( trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, 
        "MultipleTrusteeOperation wrong\n");
    ok( trustee.TrusteeForm == TRUSTEE_IS_NAME, "TrusteeForm wrong\n");
    ok( trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok( trustee.ptstrName == szTrusteeName, "ptstrName wrong\n" );

    /* test BuildTrusteeWithObjectsAndNameA (test 1) */
    memset( &trustee, 0xff, sizeof trustee );
    memset( &oan, 0xff, sizeof(oan) );
    pBuildTrusteeWithObjectsAndNameA(&trustee, &oan, SE_KERNEL_OBJECT, szObjectTypeName,
                                     szInheritedObjectTypeName, szTrusteeName);

    ok(trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok(trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, "MultipleTrusteeOperation wrong\n");
    ok(trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_NAME, "TrusteeForm wrong\n");
    ok(trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok(trustee.ptstrName == (LPTSTR)&oan, "ptstrName wrong\n");
 
    ok(oan.ObjectsPresent == (ACE_OBJECT_TYPE_PRESENT | ACE_INHERITED_OBJECT_TYPE_PRESENT), "ObjectsPresent wrong\n");
    ok(oan.ObjectType == SE_KERNEL_OBJECT, "ObjectType wrong\n");
    ok(oan.InheritedObjectTypeName == szInheritedObjectTypeName, "InheritedObjectTypeName wrong\n");
    ok(oan.ptstrName == szTrusteeName, "szTrusteeName wrong\n");

    /* test GetTrusteeNameA */
    ok(pGetTrusteeNameA(&trustee) == (LPSTR)&oan, "GetTrusteeName returned wrong value\n");

    /* test BuildTrusteeWithObjectsAndNameA (test 2) */
    memset( &trustee, 0xff, sizeof trustee );
    memset( &oan, 0xff, sizeof(oan) );
    pBuildTrusteeWithObjectsAndNameA(&trustee, &oan, SE_KERNEL_OBJECT, NULL,
                                     szInheritedObjectTypeName, szTrusteeName);

    ok(trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok(trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, "MultipleTrusteeOperation wrong\n");
    ok(trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_NAME, "TrusteeForm wrong\n");
    ok(trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok(trustee.ptstrName == (LPSTR)&oan, "ptstrName wrong\n");
 
    ok(oan.ObjectsPresent == ACE_INHERITED_OBJECT_TYPE_PRESENT, "ObjectsPresent wrong\n");
    ok(oan.ObjectType == SE_KERNEL_OBJECT, "ObjectType wrong\n");
    ok(oan.InheritedObjectTypeName == szInheritedObjectTypeName, "InheritedObjectTypeName wrong\n");
    ok(oan.ptstrName == szTrusteeName, "szTrusteeName wrong\n");

    /* test BuildTrusteeWithObjectsAndNameA (test 3) */
    memset( &trustee, 0xff, sizeof trustee );
    memset( &oan, 0xff, sizeof(oan) );
    pBuildTrusteeWithObjectsAndNameA(&trustee, &oan, SE_KERNEL_OBJECT, szObjectTypeName,
                                     NULL, szTrusteeName);

    ok(trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok(trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, "MultipleTrusteeOperation wrong\n");
    ok(trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_NAME, "TrusteeForm wrong\n");
    ok(trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok(trustee.ptstrName == (LPTSTR)&oan, "ptstrName wrong\n");
 
    ok(oan.ObjectsPresent == ACE_OBJECT_TYPE_PRESENT, "ObjectsPresent wrong\n");
    ok(oan.ObjectType == SE_KERNEL_OBJECT, "ObjectType wrong\n");
    ok(oan.InheritedObjectTypeName == NULL, "InheritedObjectTypeName wrong\n");
    ok(oan.ptstrName == szTrusteeName, "szTrusteeName wrong\n");
}
 
/* If the first isn't defined, assume none is */
#ifndef SE_MIN_WELL_KNOWN_PRIVILEGE
#define SE_MIN_WELL_KNOWN_PRIVILEGE       2L
#define SE_CREATE_TOKEN_PRIVILEGE         2L
#define SE_ASSIGNPRIMARYTOKEN_PRIVILEGE   3L
#define SE_LOCK_MEMORY_PRIVILEGE          4L
#define SE_INCREASE_QUOTA_PRIVILEGE       5L
#define SE_MACHINE_ACCOUNT_PRIVILEGE      6L
#define SE_TCB_PRIVILEGE                  7L
#define SE_SECURITY_PRIVILEGE             8L
#define SE_TAKE_OWNERSHIP_PRIVILEGE       9L
#define SE_LOAD_DRIVER_PRIVILEGE         10L
#define SE_SYSTEM_PROFILE_PRIVILEGE      11L
#define SE_SYSTEMTIME_PRIVILEGE          12L
#define SE_PROF_SINGLE_PROCESS_PRIVILEGE 13L
#define SE_INC_BASE_PRIORITY_PRIVILEGE   14L
#define SE_CREATE_PAGEFILE_PRIVILEGE     15L
#define SE_CREATE_PERMANENT_PRIVILEGE    16L
#define SE_BACKUP_PRIVILEGE              17L
#define SE_RESTORE_PRIVILEGE             18L
#define SE_SHUTDOWN_PRIVILEGE            19L
#define SE_DEBUG_PRIVILEGE               20L
#define SE_AUDIT_PRIVILEGE               21L
#define SE_SYSTEM_ENVIRONMENT_PRIVILEGE  22L
#define SE_CHANGE_NOTIFY_PRIVILLEGE      23L
#define SE_REMOTE_SHUTDOWN_PRIVILEGE     24L
#define SE_UNDOCK_PRIVILEGE              25L
#define SE_SYNC_AGENT_PRIVILEGE          26L
#define SE_ENABLE_DELEGATION_PRIVILEGE   27L
#define SE_MANAGE_VOLUME_PRIVILEGE       28L
#define SE_IMPERSONATE_PRIVILEGE         29L
#define SE_CREATE_GLOBAL_PRIVILEGE       30L
#define SE_MAX_WELL_KNOWN_PRIVILEGE      SE_CREATE_GLOBAL_PRIVILEGE
#endif /* ndef SE_MIN_WELL_KNOWN_PRIVILEGE */

static void test_allocateLuid(void)
{
    BOOL (WINAPI *pAllocateLocallyUniqueId)(PLUID);
    LUID luid1, luid2;
    BOOL ret;

    pAllocateLocallyUniqueId = (void*)GetProcAddress(hmod, "AllocateLocallyUniqueId");
    if (!pAllocateLocallyUniqueId) return;

    ret = pAllocateLocallyUniqueId(&luid1);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        return;

    ok(ret,
     "AllocateLocallyUniqueId failed: %ld\n", GetLastError());
    ret = pAllocateLocallyUniqueId(&luid2);
    ok( ret,
     "AllocateLocallyUniqueId failed: %ld\n", GetLastError());
    ok(luid1.LowPart > SE_MAX_WELL_KNOWN_PRIVILEGE || luid1.HighPart != 0,
     "AllocateLocallyUniqueId returned a well-known LUID\n");
    ok(luid1.LowPart != luid2.LowPart || luid1.HighPart != luid2.HighPart,
     "AllocateLocallyUniqueId returned non-unique LUIDs\n");
    ret = pAllocateLocallyUniqueId(NULL);
    ok( !ret && GetLastError() == ERROR_NOACCESS,
     "AllocateLocallyUniqueId(NULL) didn't return ERROR_NOACCESS: %ld\n",
     GetLastError());
}

static void test_lookupPrivilegeName(void)
{
    BOOL (WINAPI *pLookupPrivilegeNameA)(LPCSTR, PLUID, LPSTR, LPDWORD);
    char buf[MAX_PATH]; /* arbitrary, seems long enough */
    DWORD cchName = sizeof(buf);
    LUID luid = { 0, 0 };
    LONG i;
    BOOL ret;

    /* check whether it's available first */
    pLookupPrivilegeNameA = (void*)GetProcAddress(hmod, "LookupPrivilegeNameA");
    if (!pLookupPrivilegeNameA) return;
    luid.LowPart = SE_CREATE_TOKEN_PRIVILEGE;
    ret = pLookupPrivilegeNameA(NULL, &luid, buf, &cchName);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        return;

    /* check with a short buffer */
    cchName = 0;
    luid.LowPart = SE_CREATE_TOKEN_PRIVILEGE;
    ret = pLookupPrivilegeNameA(NULL, &luid, NULL, &cchName);
    ok( !ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
     "LookupPrivilegeNameA didn't fail with ERROR_INSUFFICIENT_BUFFER: %ld\n",
     GetLastError());
    ok(cchName == strlen("SeCreateTokenPrivilege") + 1,
     "LookupPrivilegeNameA returned an incorrect required length for\n"
     "SeCreateTokenPrivilege (got %ld, expected %d)\n", cchName,
     lstrlenA("SeCreateTokenPrivilege") + 1);
    /* check a known value and its returned length on success */
    cchName = sizeof(buf);
    ok(pLookupPrivilegeNameA(NULL, &luid, buf, &cchName) &&
     cchName == strlen("SeCreateTokenPrivilege"),
     "LookupPrivilegeNameA returned an incorrect output length for\n"
     "SeCreateTokenPrivilege (got %ld, expected %d)\n", cchName,
     (int)strlen("SeCreateTokenPrivilege"));
    /* check known values */
    for (i = SE_MIN_WELL_KNOWN_PRIVILEGE; i < SE_MAX_WELL_KNOWN_PRIVILEGE; i++)
    {
        luid.LowPart = i;
        cchName = sizeof(buf);
        ret = pLookupPrivilegeNameA(NULL, &luid, buf, &cchName);
        ok( ret || GetLastError() == ERROR_NO_SUCH_PRIVILEGE,
         "LookupPrivilegeNameA(0.%ld) failed: %ld\n", i, GetLastError());
    }
    /* check a bogus LUID */
    luid.LowPart = 0xdeadbeef;
    cchName = sizeof(buf);
    ret = pLookupPrivilegeNameA(NULL, &luid, buf, &cchName);
    ok( !ret && GetLastError() == ERROR_NO_SUCH_PRIVILEGE,
     "LookupPrivilegeNameA didn't fail with ERROR_NO_SUCH_PRIVILEGE: %ld\n",
     GetLastError());
    /* check on a bogus system */
    luid.LowPart = SE_CREATE_TOKEN_PRIVILEGE;
    cchName = sizeof(buf);
    ret = pLookupPrivilegeNameA("b0gu5.Nam3", &luid, buf, &cchName);
    ok( !ret && GetLastError() == RPC_S_SERVER_UNAVAILABLE,
     "LookupPrivilegeNameA didn't fail with RPC_S_SERVER_UNAVAILABLE: %ld\n",
     GetLastError());
}

struct NameToLUID
{
    const char *name;
    DWORD lowPart;
};

static void test_lookupPrivilegeValue(void)
{
    static const struct NameToLUID privs[] = {
     { "SeCreateTokenPrivilege", SE_CREATE_TOKEN_PRIVILEGE },
     { "SeAssignPrimaryTokenPrivilege", SE_ASSIGNPRIMARYTOKEN_PRIVILEGE },
     { "SeLockMemoryPrivilege", SE_LOCK_MEMORY_PRIVILEGE },
     { "SeIncreaseQuotaPrivilege", SE_INCREASE_QUOTA_PRIVILEGE },
     { "SeMachineAccountPrivilege", SE_MACHINE_ACCOUNT_PRIVILEGE },
     { "SeTcbPrivilege", SE_TCB_PRIVILEGE },
     { "SeSecurityPrivilege", SE_SECURITY_PRIVILEGE },
     { "SeTakeOwnershipPrivilege", SE_TAKE_OWNERSHIP_PRIVILEGE },
     { "SeLoadDriverPrivilege", SE_LOAD_DRIVER_PRIVILEGE },
     { "SeSystemProfilePrivilege", SE_SYSTEM_PROFILE_PRIVILEGE },
     { "SeSystemtimePrivilege", SE_SYSTEMTIME_PRIVILEGE },
     { "SeProfileSingleProcessPrivilege", SE_PROF_SINGLE_PROCESS_PRIVILEGE },
     { "SeIncreaseBasePriorityPrivilege", SE_INC_BASE_PRIORITY_PRIVILEGE },
     { "SeCreatePagefilePrivilege", SE_CREATE_PAGEFILE_PRIVILEGE },
     { "SeCreatePermanentPrivilege", SE_CREATE_PERMANENT_PRIVILEGE },
     { "SeBackupPrivilege", SE_BACKUP_PRIVILEGE },
     { "SeRestorePrivilege", SE_RESTORE_PRIVILEGE },
     { "SeShutdownPrivilege", SE_SHUTDOWN_PRIVILEGE },
     { "SeDebugPrivilege", SE_DEBUG_PRIVILEGE },
     { "SeAuditPrivilege", SE_AUDIT_PRIVILEGE },
     { "SeSystemEnvironmentPrivilege", SE_SYSTEM_ENVIRONMENT_PRIVILEGE },
     { "SeChangeNotifyPrivilege", SE_CHANGE_NOTIFY_PRIVILLEGE },
     { "SeRemoteShutdownPrivilege", SE_REMOTE_SHUTDOWN_PRIVILEGE },
     { "SeUndockPrivilege", SE_UNDOCK_PRIVILEGE },
     { "SeSyncAgentPrivilege", SE_SYNC_AGENT_PRIVILEGE },
     { "SeEnableDelegationPrivilege", SE_ENABLE_DELEGATION_PRIVILEGE },
     { "SeManageVolumePrivilege", SE_MANAGE_VOLUME_PRIVILEGE },
     { "SeImpersonatePrivilege", SE_IMPERSONATE_PRIVILEGE },
     { "SeCreateGlobalPrivilege", SE_CREATE_GLOBAL_PRIVILEGE },
    };
    BOOL (WINAPI *pLookupPrivilegeValueA)(LPCSTR, LPCSTR, PLUID);
    int i;
    LUID luid;
    BOOL ret;

    /* check whether it's available first */
    pLookupPrivilegeValueA = (void*)GetProcAddress(hmod, "LookupPrivilegeValueA");
    if (!pLookupPrivilegeValueA) return;
    ret = pLookupPrivilegeValueA(NULL, "SeCreateTokenPrivilege", &luid);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        return;

    /* check a bogus system name */
    ret = pLookupPrivilegeValueA("b0gu5.Nam3", "SeCreateTokenPrivilege", &luid);
    ok( !ret && GetLastError() == RPC_S_SERVER_UNAVAILABLE,
     "LookupPrivilegeValueA didn't fail with RPC_S_SERVER_UNAVAILABLE: %ld\n",
     GetLastError());
    /* check a NULL string */
    ret = pLookupPrivilegeValueA(NULL, 0, &luid);
    ok( !ret && GetLastError() == ERROR_NO_SUCH_PRIVILEGE,
     "LookupPrivilegeValueA didn't fail with ERROR_NO_SUCH_PRIVILEGE: %ld\n",
     GetLastError());
    /* check a bogus privilege name */
    ret = pLookupPrivilegeValueA(NULL, "SeBogusPrivilege", &luid);
    ok( !ret && GetLastError() == ERROR_NO_SUCH_PRIVILEGE,
     "LookupPrivilegeValueA didn't fail with ERROR_NO_SUCH_PRIVILEGE: %ld\n",
     GetLastError());
    /* check case insensitive */
    ret = pLookupPrivilegeValueA(NULL, "sEcREATEtOKENpRIVILEGE", &luid);
    ok( ret,
     "LookupPrivilegeValueA(NULL, sEcREATEtOKENpRIVILEGE, &luid) failed: %ld\n",
     GetLastError());
    for (i = 0; i < sizeof(privs) / sizeof(privs[0]); i++)
    {
        /* Not all privileges are implemented on all Windows versions, so
         * don't worry if the call fails
         */
        if (pLookupPrivilegeValueA(NULL, privs[i].name, &luid))
        {
            ok(luid.LowPart == privs[i].lowPart,
             "LookupPrivilegeValueA returned an invalid LUID for %s\n",
             privs[i].name);
        }
    }
}

static void test_luid(void)
{
    test_allocateLuid();
    test_lookupPrivilegeName();
    test_lookupPrivilegeValue();
}

static void test_FileSecurity(void)
{
    char directory[MAX_PATH];
    DWORD retval, outSize;
    BOOL result;
    BYTE buffer[0x40];

    pGetFileSecurityA = (fnGetFileSecurityA)
                    GetProcAddress( hmod, "GetFileSecurityA" );
    if( !pGetFileSecurityA )
        return;

    retval = GetTempPathA(sizeof(directory), directory);
    if (!retval) {
        trace("GetTempPathA failed\n");
        return;
    }

    strcpy(directory, "\\Should not exist");

    SetLastError(NO_ERROR);
    result = pGetFileSecurityA( directory,OWNER_SECURITY_INFORMATION,buffer,0x40,&outSize);
    ok(!result, "GetFileSecurityA should fail for not existing directories/files\n"); 
    ok( (GetLastError() == ERROR_FILE_NOT_FOUND ) ||
        (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) , 
        "last error ERROR_FILE_NOT_FOUND / ERROR_CALL_NOT_IMPLEMENTED (98) "
        "expected, got %ld\n", GetLastError());
}

static void test_AccessCheck(void)
{
    PSID EveryoneSid = NULL, AdminSid = NULL, UsersSid = NULL;
    PACL Acl = NULL;
    SECURITY_DESCRIPTOR *SecurityDescriptor = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = { SECURITY_WORLD_SID_AUTHORITY };
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = { SECURITY_NT_AUTHORITY };
    GENERIC_MAPPING Mapping = { KEY_READ, KEY_WRITE, KEY_EXECUTE, KEY_ALL_ACCESS };
    ACCESS_MASK Access;
    BOOL AccessStatus;
    HANDLE Token;
    BOOL ret;
    DWORD PrivSetLen;
    PRIVILEGE_SET *PrivSet;
    BOOL res;
    HMODULE NtDllModule;
    BOOLEAN Enabled;

    NtDllModule = GetModuleHandle("ntdll.dll");

    if (!NtDllModule)
    {
        trace("not running on NT, skipping test\n");
        return;
    }
    pRtlAdjustPrivilege = (fnRtlAdjustPrivilege)
                          GetProcAddress(NtDllModule, "RtlAdjustPrivilege");
    if (!pRtlAdjustPrivilege) return;

    Acl = HeapAlloc(GetProcessHeap(), 0, 256);
    res = InitializeAcl(Acl, 256, ACL_REVISION);
    if(!res && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        trace("ACLs not implemented - skipping tests\n");
        return;
    }
    ok(res, "InitializeAcl failed with error %ld\n", GetLastError());

    res = AllocateAndInitializeSid( &SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &EveryoneSid);
    ok(res, "AllocateAndInitializeSid failed with error %ld\n", GetLastError());

    res = AllocateAndInitializeSid( &SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdminSid);
    ok(res, "AllocateAndInitializeSid failed with error %ld\n", GetLastError());

    res = AllocateAndInitializeSid( &SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_USERS, 0, 0, 0, 0, 0, 0, &UsersSid);
    ok(res, "AllocateAndInitializeSid failed with error %ld\n", GetLastError());

    res = AddAccessAllowedAce(Acl, ACL_REVISION, KEY_READ, EveryoneSid);
    ok(res, "AddAccessAllowedAceEx failed with error %ld\n", GetLastError());

    res = AddAccessAllowedAce(Acl, ACL_REVISION, KEY_ALL_ACCESS, AdminSid);
    ok(res, "AddAccessAllowedAceEx failed with error %ld\n", GetLastError());

    SecurityDescriptor = HeapAlloc(GetProcessHeap(), 0, SECURITY_DESCRIPTOR_MIN_LENGTH);

    res = InitializeSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    ok(res, "InitializeSecurityDescriptor failed with error %ld\n", GetLastError());

    res = SetSecurityDescriptorDacl(SecurityDescriptor, TRUE, Acl, FALSE);
    ok(res, "SetSecurityDescriptorDacl failed with error %ld\n", GetLastError());

    res = SetSecurityDescriptorOwner(SecurityDescriptor, AdminSid, FALSE);
    ok(res, "SetSecurityDescriptorOwner failed with error %ld\n", GetLastError());

    res = SetSecurityDescriptorGroup(SecurityDescriptor, UsersSid, TRUE);
    ok(res, "SetSecurityDescriptorGroup failed with error %ld\n", GetLastError());

    PrivSetLen = FIELD_OFFSET(PRIVILEGE_SET, Privilege[16]);
    PrivSet = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, PrivSetLen);
    PrivSet->PrivilegeCount = 16;

    ImpersonateSelf(SecurityImpersonation);

    pRtlAdjustPrivilege(SE_SECURITY_PRIVILEGE, FALSE, TRUE, &Enabled);

    ret = OpenThreadToken(GetCurrentThread(),
                          TOKEN_QUERY, TRUE, &Token);
    ok(ret, "OpenThreadToken failed with error %ld\n", GetLastError());

    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret, "AccessCheck failed with error %ld\n", GetLastError());
    ok(AccessStatus && (Access == KEY_READ),
        "AccessCheck failed to grant access with error %ld\n",
        GetLastError());

    ret = AccessCheck(SecurityDescriptor, Token, MAXIMUM_ALLOWED, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret, "AccessCheck failed with error %ld\n", GetLastError());
    ok(AccessStatus,
        "AccessCheck failed to grant any access with error %ld\n",
        GetLastError());
    trace("AccessCheck with MAXIMUM_ALLOWED got Access 0x%08lx\n", Access);

    SetLastError(0);
    PrivSet->PrivilegeCount = 16;
    ret = AccessCheck(SecurityDescriptor, Token, ACCESS_SYSTEM_SECURITY, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret && !AccessStatus && GetLastError() == ERROR_PRIVILEGE_NOT_HELD,
        "AccessCheck should have failed with ERROR_PRIVILEGE_NOT_HELD, instead of %ld\n",
        GetLastError());

    ret = pRtlAdjustPrivilege(SE_SECURITY_PRIVILEGE, TRUE, TRUE, &Enabled);
    if (!ret)
    {
        SetLastError(0);
        PrivSet->PrivilegeCount = 16;
        ret = AccessCheck(SecurityDescriptor, Token, ACCESS_SYSTEM_SECURITY, &Mapping,
                          PrivSet, &PrivSetLen, &Access, &AccessStatus);
        ok(ret && AccessStatus && GetLastError() == 0,
            "AccessCheck should have succeeded, error %ld\n",
            GetLastError());
        ok(Access == ACCESS_SYSTEM_SECURITY,
            "Access should be equal to ACCESS_SYSTEM_SECURITY instead of 0x%08lx\n",
            Access);
    }
    else
        trace("Couldn't get SE_SECURITY_PRIVILEGE (0x%08x), skipping ACCESS_SYSTEM_SECURITY test\n",
            ret);

    RevertToSelf();

    if (EveryoneSid)
        FreeSid(EveryoneSid);
    if (AdminSid)
        FreeSid(AdminSid);
    if (UsersSid)
        FreeSid(UsersSid);
    HeapFree(GetProcessHeap(), 0, Acl);
    HeapFree(GetProcessHeap(), 0, SecurityDescriptor);
    HeapFree(GetProcessHeap(), 0, PrivSet);
}

/* test GetTokenInformation for the various attributes */
static void test_token_attr(void)
{
    HANDLE Token;
    DWORD Size;
    TOKEN_PRIVILEGES *Privileges;
    TOKEN_GROUPS *Groups;
    TOKEN_USER *User;
    BOOL ret;
    DWORD i, GLE;
    LPSTR SidString;

    if(!pConvertSidToStringSidA)
        return;

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &Token);
    GLE = GetLastError();
    ok(ret || (GLE == ERROR_CALL_NOT_IMPLEMENTED), 
        "OpenProcessToken failed with error %ld\n", GLE);
    if(!ret && (GLE == ERROR_CALL_NOT_IMPLEMENTED))
    {
        trace("OpenProcessToken() not implemented, skipping test_token_attr()\n");
        return;
    }

    /* groups */
    ret = GetTokenInformation(Token, TokenGroups, NULL, 0, &Size);
    Groups = HeapAlloc(GetProcessHeap(), 0, Size);
    ret = GetTokenInformation(Token, TokenGroups, Groups, Size, &Size);
    ok(ret, "GetTokenInformation(TokenGroups) failed with error %ld\n", GetLastError());
    trace("TokenGroups:\n");
    for (i = 0; i < Groups->GroupCount; i++)
    {
        DWORD NameLength = 255;
        TCHAR Name[255];
        DWORD DomainLength = 255;
        TCHAR Domain[255];
        SID_NAME_USE SidNameUse;
        pConvertSidToStringSidA(Groups->Groups[i].Sid, &SidString);
        Name[0] = '\0';
        Domain[0] = '\0';
        ret = LookupAccountSid(NULL, Groups->Groups[i].Sid, Name, &NameLength, Domain, &DomainLength, &SidNameUse);
        ok(ret, "LookupAccountSid(%s) failed with error %ld\n", SidString, GetLastError());
        trace("\t%s, %s\\%s use: %d attr: 0x%08lx\n", SidString, Domain, Name, SidNameUse, Groups->Groups[i].Attributes);
        LocalFree(SidString);
    }
    HeapFree(GetProcessHeap(), 0, Groups);

    /* user */
    ret = GetTokenInformation(Token, TokenUser, NULL, 0, &Size);
    ok(!ret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "GetTokenInformation(TokenUser) failed with error %ld\n", GetLastError());
    User = HeapAlloc(GetProcessHeap(), 0, Size);
    ret = GetTokenInformation(Token, TokenUser, User, Size, &Size);
    ok(ret,
        "GetTokenInformation(TokenUser) failed with error %ld\n", GetLastError());

    pConvertSidToStringSidA(User->User.Sid, &SidString);
    trace("TokenUser: %s attr: 0x%08lx\n", SidString, User->User.Attributes);
    LocalFree(SidString);

    /* privileges */
    ret = GetTokenInformation(Token, TokenPrivileges, NULL, 0, &Size);
    ok(!ret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "GetTokenInformation(TokenPrivileges) failed with error %ld\n", GetLastError());
    Privileges = HeapAlloc(GetProcessHeap(), 0, Size);
    ret = GetTokenInformation(Token, TokenPrivileges, Privileges, Size, &Size);
    ok(ret,
        "GetTokenInformation(TokenPrivileges) failed with error %ld\n", GetLastError());
    trace("TokenPrivileges:\n");
    for (i = 0; i < Privileges->PrivilegeCount; i++)
    {
        TCHAR Name[256];
        DWORD NameLen = sizeof(Name)/sizeof(Name[0]);
        LookupPrivilegeName(NULL, &Privileges->Privileges[i].Luid, Name, &NameLen);
        trace("\t%s, 0x%lx\n", Name, Privileges->Privileges[i].Attributes);
    }
}

typedef union _MAX_SID
{
    SID sid;
    char max[SECURITY_MAX_SID_SIZE];
} MAX_SID;

static void test_sid_str(PSID * sid)
{
    char *str_sid;
    BOOL ret = pConvertSidToStringSidA(sid, &str_sid);
    ok(ret, "ConvertSidToStringSidA() failed: %ld\n", GetLastError());
    if (ret)
    {
        char account[MAX_PATH], domain[MAX_PATH];
        SID_NAME_USE use;
        DWORD acc_size = MAX_PATH;
        DWORD dom_size = MAX_PATH;
        ret = LookupAccountSid(NULL, sid, account, &acc_size, domain, &dom_size, &use);
        ok(ret || (!ret && (GetLastError() == ERROR_NONE_MAPPED)),
           "LookupAccountSid(%s) failed: %ld\n", str_sid, GetLastError());
        if (ret)
            trace(" %s %s\\%s %d\n", str_sid, domain, account, use);
        else if (GetLastError() == ERROR_NONE_MAPPED)
            trace(" %s Couldn't me mapped\n", str_sid);
        LocalFree(str_sid);
    }
}

static void test_LookupAccountSid(void)
{
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = { SECURITY_NT_AUTHORITY };
    char account[MAX_PATH], domain[MAX_PATH];
    DWORD acc_size, dom_size;
    DWORD real_acc_size, real_dom_size;
    PSID pUsersSid = NULL;
    SID_NAME_USE use;
    BOOL ret;
    DWORD size;
    MAX_SID  max_sid;
    char *str_sid;
    int i;

    /* native windows crashes if account size, domain size, or name use is NULL */

    ret = AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_USERS, 0, 0, 0, 0, 0, 0, &pUsersSid);
    ok(ret || (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED),
       "AllocateAndInitializeSid failed with error %ld\n", GetLastError());

    /* not running on NT so give up */
    if (!ret && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
        return;

    real_acc_size = MAX_PATH;
    real_dom_size = MAX_PATH;
    ret = LookupAccountSidA(NULL, pUsersSid, account, &real_acc_size, domain, &real_dom_size, &use);
    ok(ret, "LookupAccountSid() Expected TRUE, got FALSE\n");

    /* try NULL account */
    acc_size = MAX_PATH;
    dom_size = MAX_PATH;
    ret = LookupAccountSidA(NULL, pUsersSid, NULL, &acc_size, domain, &dom_size, &use);
    ok(ret, "LookupAccountSid() Expected TRUE, got FALSE\n");

    /* try NULL domain */
    acc_size = MAX_PATH;
    dom_size = MAX_PATH;
    ret = LookupAccountSidA(NULL, pUsersSid, account, &acc_size, NULL, &dom_size, &use);
    ok(ret, "LookupAccountSid() Expected TRUE, got FALSE\n");

    /* try a small account buffer */
    acc_size = 1;
    dom_size = MAX_PATH;
    account[0] = 0;
    ret = LookupAccountSidA(NULL, pUsersSid, account, &acc_size, domain, &dom_size, &use);
    ok(!ret, "LookupAccountSid() Expected FALSE got TRUE\n");
    ok(GetLastError() == ERROR_NOT_ENOUGH_MEMORY, "LookupAccountSid() Expected ERROR_NOT_ENOUGH_MEMORY, got %lu\n", GetLastError());

    /* try a 0 sized account buffer */
    acc_size = 0;
    dom_size = MAX_PATH;
    account[0] = 0;
    ret = LookupAccountSidA(NULL, pUsersSid, account, &acc_size, domain, &dom_size, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(acc_size == real_acc_size, "LookupAccountSid() Expected acc_size = %lu, got %lu\n", real_acc_size, acc_size);

    /* try a 0 sized account buffer */
    acc_size = 0;
    dom_size = MAX_PATH;
    ret = LookupAccountSidA(NULL, pUsersSid, NULL, &acc_size, domain, &dom_size, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(acc_size == real_acc_size, "LookupAccountSid() Expected acc_size = %lu, got %lu\n", real_acc_size, acc_size);

    /* try a small domain buffer */
    dom_size = 1;
    acc_size = MAX_PATH;
    account[0] = 0;
    ret = LookupAccountSidA(NULL, pUsersSid, account, &acc_size, domain, &dom_size, &use);
    ok(!ret, "LookupAccountSid() Expected FALSE got TRUE\n");
    ok(GetLastError() == ERROR_NOT_ENOUGH_MEMORY, "LookupAccountSid() Expected ERROR_NOT_ENOUGH_MEMORY, got %lu\n", GetLastError());

    /* try a 0 sized domain buffer */
    dom_size = 0;
    acc_size = MAX_PATH;
    account[0] = 0;
    ret = LookupAccountSidA(NULL, pUsersSid, account, &acc_size, domain, &dom_size, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(dom_size == real_dom_size, "LookupAccountSid() Expected dom_size = %lu, got %lu\n", real_dom_size, dom_size);

    /* try a 0 sized domain buffer */
    dom_size = 0;
    acc_size = MAX_PATH;
    ret = LookupAccountSidA(NULL, pUsersSid, account, &acc_size, NULL, &dom_size, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(acc_size == real_acc_size, "LookupAccountSid() Expected dom_size = %lu, got %lu\n", real_dom_size, dom_size);

    pCreateWellKnownSid = (fnCreateWellKnownSid)GetProcAddress( hmod, "CreateWellKnownSid" );

    if (pCreateWellKnownSid && pConvertSidToStringSidA)
    {
        trace("Well Known SIDs:\n");
        for (i = 0; i <= 60; i++)
        {
            size = SECURITY_MAX_SID_SIZE;
            if (pCreateWellKnownSid(i, NULL, &max_sid.sid, &size))
            {
                if (pConvertSidToStringSidA(&max_sid.sid, &str_sid))
                {
                    acc_size = MAX_PATH;
                    dom_size = MAX_PATH;
                    if (LookupAccountSid(NULL, &max_sid.sid, account, &acc_size, domain, &dom_size, &use))
                        trace(" %d: %s %s\\%s %d\n", i, str_sid, domain, account, use);
                    LocalFree(str_sid);
                }
            }
            else
                trace(" CreateWellKnownSid(%d) failed: %ld\n", i, GetLastError());
        }

        pLsaQueryInformationPolicy = (fnLsaQueryInformationPolicy)GetProcAddress( hmod, "LsaQueryInformationPolicy");
        pLsaOpenPolicy = (fnLsaOpenPolicy)GetProcAddress( hmod, "LsaOpenPolicy");
        pLsaFreeMemory = (fnLsaFreeMemory)GetProcAddress( hmod, "LsaFreeMemory");
        pLsaClose = (fnLsaClose)GetProcAddress( hmod, "LsaClose");

        if (pLsaQueryInformationPolicy && pLsaOpenPolicy && pLsaFreeMemory && pLsaClose)
        {
            NTSTATUS status;
            LSA_HANDLE handle;
            LSA_OBJECT_ATTRIBUTES object_attributes;

            ZeroMemory(&object_attributes, sizeof(object_attributes));
            object_attributes.Length = sizeof(object_attributes);

            status = pLsaOpenPolicy( NULL, &object_attributes, POLICY_ALL_ACCESS, &handle);
            ok(status == STATUS_SUCCESS || status == STATUS_ACCESS_DENIED,
               "LsaOpenPolicy(POLICY_ALL_ACCESS) returned 0x%08lx\n", status);

            /* try a more restricted access mask if necessary */
            if (status == STATUS_ACCESS_DENIED) {
                trace("LsaOpenPolicy(POLICY_ALL_ACCESS) failed, trying POLICY_VIEW_LOCAL_INFORMATION\n");
                status = pLsaOpenPolicy( NULL, &object_attributes, POLICY_VIEW_LOCAL_INFORMATION, &handle);
                ok(status == STATUS_SUCCESS, "LsaOpenPolicy(POLICY_VIEW_LOCAL_INFORMATION) returned 0x%08lx\n", status);
            }

            if (status == STATUS_SUCCESS)
            {
                PPOLICY_ACCOUNT_DOMAIN_INFO info;
                status = pLsaQueryInformationPolicy(handle, PolicyAccountDomainInformation, (PVOID*)&info);
                ok(status == STATUS_SUCCESS, "LsaQueryInformationPolicy() failed, returned 0x%08lx\n", status);
                if (status == STATUS_SUCCESS)
                {
                    ok(info->DomainSid!=0, "LsaQueryInformationPolicy(PolicyAccountDomainInformation) missing SID\n");
                    if (info->DomainSid)
                    {
                        int count = *GetSidSubAuthorityCount(info->DomainSid);
                        CopySid(GetSidLengthRequired(count), &max_sid, info->DomainSid);
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_USER_RID_ADMIN;
                        max_sid.sid.SubAuthorityCount = count + 1;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_USER_RID_GUEST;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_ADMINS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_USERS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_GUESTS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_COMPUTERS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_CONTROLLERS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_CERT_ADMINS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_SCHEMA_ADMINS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_ENTERPRISE_ADMINS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_POLICY_ADMINS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_ALIAS_RID_RAS_SERVERS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = 1000;	/* first user account */
                        test_sid_str((PSID)&max_sid.sid);
                    }

                    pLsaFreeMemory((LPVOID)info);
                }

                status = pLsaClose(handle);
                ok(status == STATUS_SUCCESS, "LsaClose() failed, returned 0x%08lx\n", status);
            }
        }
    }
}

START_TEST(security)
{
    init();
    if (!hmod) return;
    test_sid();
    test_trustee();
    test_luid();
    test_FileSecurity();
    test_AccessCheck();
    test_token_attr();
    test_LookupAccountSid();
}
