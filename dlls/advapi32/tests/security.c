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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "aclapi.h"
#include "winnt.h"

typedef BOOL (WINAPI *fnConvertSidToStringSidA)( PSID pSid, LPSTR *str );
typedef BOOL (WINAPI *fnConvertStringSidToSidA)( LPCSTR str, PSID pSid );

fnConvertSidToStringSidA pConvertSidToStringSidA;
fnConvertStringSidToSidA pConvertStringSidToSidA;

struct sidRef
{
    SID_IDENTIFIER_AUTHORITY auth;
    const char *refStr;
};

void test_sid()
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
    HMODULE hmod = GetModuleHandle("advapi32.dll");
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
        ok( !strcmp( str, refs[i].refStr ),
         "incorrect sid, expected %s, got %s\n", refs[i].refStr, str );
        if( str )
            LocalFree( str );
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
}

void test_trustee()
{
    TRUSTEE trustee;
    PSID psid;
    DWORD r;
    LPSTR str = "2jjj";

    SID_IDENTIFIER_AUTHORITY auth = { {0x11,0x22,0,0,0, 0} };

    r = AllocateAndInitializeSid( &auth, 1, 42, 0,0,0,0,0,0,0,&psid );
    ok( r, "failed to init SID\n" );

    memset( &trustee, 0xff, sizeof trustee );
    BuildTrusteeWithSidA( &trustee, psid );

    ok( trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok( trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, 
        "MultipleTrusteeOperation wrong\n");
    ok( trustee.TrusteeForm == TRUSTEE_IS_SID, "TrusteeForm wrong\n");
    ok( trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok( trustee.ptstrName == (LPSTR) psid, "ptstrName wrong\n" );
    FreeSid( psid );

    /* test BuildTrusteeWithNameA */
    memset( &trustee, 0xff, sizeof trustee );
    BuildTrusteeWithNameA( &trustee, str );

    ok( trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok( trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, 
        "MultipleTrusteeOperation wrong\n");
    ok( trustee.TrusteeForm == TRUSTEE_IS_NAME, "TrusteeForm wrong\n");
    ok( trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok( trustee.ptstrName == str, "ptstrName wrong\n" );
}
 
START_TEST(security)
{
    test_sid();
    test_trustee();
}
