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

typedef BOOL (WINAPI *fnConvertSidToStringSidA)( PSID pSid, LPSTR *str );
typedef BOOL (WINAPI *fnConvertSidToStringSidW)( PSID pSid, LPWSTR *str );

fnConvertSidToStringSidW pConvertSidToStringSidW;
fnConvertSidToStringSidA pConvertSidToStringSidA;

void test_sid()
{
    PSID psid;
    LPWSTR str = NULL;
    BOOL r;
    SID_IDENTIFIER_AUTHORITY auth = { {6,7,0x1a,0x15,0x0e,0x1f} };
    WCHAR refstr[] = { 'S','-','1','-','1','A','1','5','6','E','7','F','-',
        '1','2','3','4','5','-','0','-','4','2','9','4','9','6','7','2','9','5',0 };

    HMODULE hmod = GetModuleHandle("advapi32.dll");

    pConvertSidToStringSidW = (fnConvertSidToStringSidW)
                    GetProcAddress( hmod, "ConvertSidToStringSidW" );
    if( !pConvertSidToStringSidW )
        return;
    
    r = AllocateAndInitializeSid( &auth, 3, 12345, 0,-1,0,0,0,0,0,&psid);
    ok( r, "failed to allocate sid\n" );
    r = pConvertSidToStringSidW( psid, &str );
    ok( r, "failed to convert sid\n" );
    ok( !lstrcmpW( str, refstr ), "incorrect sid\n" );
    LocalFree( str );
    FreeSid( psid );
}

void test_trustee()
{
    TRUSTEE trustee;
    PSID psid;
    DWORD r;

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
}

START_TEST(security)
{
    test_sid();
    test_trustee();
}
