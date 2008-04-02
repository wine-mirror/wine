/* DirectPlay Conformance Tests
 *
 * Copyright 2007 - Alessandro Pignotti
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

#include "wine/test.h"
#define INITGUID
#include <dplay.h>

static BOOL validSP = FALSE; /*This global variable is needed until wine has a working service provider
                               implementation*/

static BOOL CALLBACK EnumConnectionsCallback(LPCGUID lpguidSP, LPVOID lpConnection,
    DWORD dwConnectionSize, LPCDPNAME lpName, DWORD dwFlags, LPVOID lpContext)
{
    HRESULT hr;

    if(IsEqualGUID(lpguidSP,&DPSPGUID_TCPIP))
    {
        /*I'm forcing the TCP/IP Service provider*/
        hr = IDirectPlayX_InitializeConnection((LPDIRECTPLAY4) lpContext, lpConnection, 0);
        if( SUCCEEDED( hr ))
            validSP = TRUE;
        return FALSE;
    }
    return TRUE;
}

static void test_session_guid(LPDIRECTPLAY4 pDP)
{
    GUID appGuid;
    GUID zeroGuid;
    DPSESSIONDESC2 sessionDesc;
    LPDPSESSIONDESC2 newSession;
    DWORD sessionSize;
    static char name[] = "DPlay conformance test";

    CoCreateGuid( &appGuid );
    IDirectPlayX_EnumConnections(pDP, &appGuid, EnumConnectionsCallback, pDP, 0);

    if( !validSP )
    {
        skip("Tests will not work without a valid service provider, skipping.\n");
        return;
    }

    memset(&sessionDesc, 0, sizeof( DPSESSIONDESC2 ));
    memset(&zeroGuid, 0, 16);

    sessionDesc.dwSize = sizeof( DPSESSIONDESC2 );
    memcpy(&sessionDesc.guidApplication, &appGuid, 16);
    sessionDesc.dwFlags = DPSESSION_CLIENTSERVER;
    U1(sessionDesc).lpszSessionNameA = name;
    sessionDesc.dwMaxPlayers = 10;
    sessionDesc.dwCurrentPlayers = 0;
    IDirectPlayX_Open(pDP, &sessionDesc, DPOPEN_CREATE);

    /* I read the sessiondesc from directplay in a fresh memory location,
       because directplay does not touch the original struct, but saves
       internally a version with the session guid set*/

    IDirectPlayX_GetSessionDesc(pDP, NULL, &sessionSize);
    newSession = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sessionSize);
    IDirectPlayX_GetSessionDesc(pDP, newSession, &sessionSize);
    todo_wine ok( !IsEqualGUID(&newSession->guidInstance, &zeroGuid), "Session guid not initialized\n");
    HeapFree(GetProcessHeap(), 0, newSession);
}

START_TEST(dplayx)
{
    LPDIRECTPLAY4 pDP;

    CoInitialize( NULL );
    CoCreateInstance(&CLSID_DirectPlay, NULL, CLSCTX_ALL, &IID_IDirectPlay4A, (VOID**)&pDP);

    test_session_guid( pDP );

    IDirectPlayX_Release( pDP );
    CoUninitialize();
}
