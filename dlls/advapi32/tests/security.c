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
}

START_TEST(security)
{
    test_sid();
}
