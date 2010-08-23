/*
 * IGameExplorer and IGameExplorer2 tests
 *
 * Copyright (C) 2010 Mariusz Pluciński
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

#define COBJMACROS

#include "windows.h"
#include "ole2.h"
#include "objsafe.h"
#include "objbase.h"
#include "shlwapi.h"
#include "sddl.h"
#include "shobjidl.h"

#include "initguid.h"
#include "gameux.h"

#include "wine/test.h"

/*******************************************************************************
 * Pointers used instead of direct calls. These procedures are not available on
 * older system, which causes problem while loading test binary.
 */
static BOOL WINAPI (*_ConvertSidToStringSidW)(PSID,LPWSTR*);

/*******************************************************************************
 *_loadDynamicRoutines
 *
 * Helper function, prepares pointers to system procedures which may be not
 * available on older operating systems.
 *
 * Returns:
 *  TRUE                        procedures were loaded successfunnly
 *  FALSE                       procedures were not loaded successfunnly
 */
static BOOL _loadDynamicRoutines(void)
{
    HMODULE hAdvapiModule = GetModuleHandleA( "advapi32.dll" );

    _ConvertSidToStringSidW = (LPVOID)GetProcAddress(hAdvapiModule, "ConvertSidToStringSidW");
    if (!_ConvertSidToStringSidW) return FALSE;
    return TRUE;
}

/*******************************************************************************
 * _buildGameRegistryPath
 *
 * Helper function, builds registry path to key, where game's data are stored
 *
 * Parameters:
 *  installScope                [I]     the scope which was used in AddGame/InstallGame call
 *  gameInstanceId              [I]     game instance GUID
 *  lpRegistryPath              [O]     pointer which will receive address to string
 *                                      containing expected registry path. Path
 *                                      is relative to HKLM registry key. It
 *                                      must be freed by calling CoTaskMemFree
 */
static HRESULT _buildGameRegistryPath(GAME_INSTALL_SCOPE installScope,
        LPCGUID gameInstanceId,
        LPWSTR* lpRegistryPath)
{
    static const WCHAR sGameUxRegistryPath[] = {'S','O','F','T','W','A','R','E','\\',
            'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
            'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','G','a','m','e','U','X',0};
    static const WCHAR sGames[] = {'G','a','m','e','s',0};
    static const WCHAR sBackslash[] = {'\\',0};

    HRESULT hr = S_OK;
    HANDLE hToken = NULL;
    PTOKEN_USER pTokenUser = NULL;
    DWORD dwLength;
    LPWSTR lpSID = NULL;
    WCHAR sInstanceId[40];
    WCHAR sRegistryPath[8192];

    lstrcpyW(sRegistryPath, sGameUxRegistryPath);
    lstrcatW(sRegistryPath, sBackslash);

    if(installScope == GIS_CURRENT_USER)
    {
        /* build registry path containing user's SID */
        if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
            hr = HRESULT_FROM_WIN32(GetLastError());

        if(SUCCEEDED(hr))
        {
            if(!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwLength) &&
                    GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
                hr = HRESULT_FROM_WIN32(GetLastError());

            if(SUCCEEDED(hr))
            {
                pTokenUser = CoTaskMemAlloc(dwLength);
                if(!pTokenUser)
                    hr = E_OUTOFMEMORY;
            }

            if(SUCCEEDED(hr))
                if(!GetTokenInformation(hToken, TokenUser, (LPVOID)pTokenUser, dwLength, &dwLength))
                    hr = HRESULT_FROM_WIN32(GetLastError());

            if(SUCCEEDED(hr))
                if(!_ConvertSidToStringSidW(pTokenUser->User.Sid, &lpSID))
                    hr = HRESULT_FROM_WIN32(GetLastError());

            if(SUCCEEDED(hr))
            {
                lstrcatW(sRegistryPath, lpSID);
                LocalFree(lpSID);
            }

            CoTaskMemFree(pTokenUser);
            CloseHandle(hToken);
        }
    }
    else if(installScope == GIS_ALL_USERS)
        /* build registry path without SID */
        lstrcatW(sRegistryPath, sGames);
    else
        hr = E_INVALIDARG;

    /* put game's instance id on the end of path */
    if(SUCCEEDED(hr))
        hr = (StringFromGUID2(gameInstanceId, sInstanceId, sizeof(sInstanceId)/sizeof(sInstanceId[0])) ? S_OK : E_FAIL);

    if(SUCCEEDED(hr))
    {
        lstrcatW(sRegistryPath, sBackslash);
        lstrcatW(sRegistryPath, sInstanceId);
    }

    if(SUCCEEDED(hr))
    {
        *lpRegistryPath = CoTaskMemAlloc((lstrlenW(sRegistryPath)+1)*sizeof(WCHAR));
        if(!*lpRegistryPath)
            hr = E_OUTOFMEMORY;
    }

    if(SUCCEEDED(hr))
        lstrcpyW(*lpRegistryPath, sRegistryPath);

    return hr;
}
/*******************************************************************************
 *  _validateGameKey
 *
 * Helper function, verifies current state of game's registry key with expected.
 *
 * Parameters:
 *  line                        [I]     place of original call. Used only to display
 *                                      more useful message on test fail
 *  installScope                [I]     the scope which was used in AddGame/InstallGame call
 *  gameInstanceId              [I]     game instance identifier
 *  presenceExpected            [I]     is it expected that game should be currently
 *                                      registered or not. Should be TRUE if checking
 *                                      after using AddGame/InstallGame, and FALSE
 *                                      if checking after RemoveGame/UninstallGame
 */
static void _validateGameRegistryKey(int line,
        GAME_INSTALL_SCOPE installScope,
        LPCGUID gameInstanceId,
        BOOL presenceExpected)
{
    HRESULT hr;
    LPWSTR lpRegistryPath = NULL;
    HKEY hKey;

    /* check key presence */
    hr = _buildGameRegistryPath(installScope, gameInstanceId, &lpRegistryPath);

    if(SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegOpenKeyExW(HKEY_LOCAL_MACHINE, lpRegistryPath, 0,
                KEY_READ | KEY_WOW64_64KEY, &hKey));

        if(presenceExpected)
            ok_(__FILE__, line)(hr == S_OK,
                    "problem while trying to open registry key (HKLM): %s, error: 0x%x\n", wine_dbgstr_w(lpRegistryPath), hr);
        else
            ok_(__FILE__, line)(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
                    "other than expected (FILE_NOT_FOUND) error while trying to open registry key (HKLM): %s, error: 0x%x\n", wine_dbgstr_w(lpRegistryPath), hr);
    }

    if(SUCCEEDED(hr))
        RegCloseKey(hKey);

    CoTaskMemFree(lpRegistryPath);
}

/*******************************************************************************
 * Test routines
 */
static void test_create(BOOL* gameExplorerAvailable)
{
    HRESULT hr;

    IGameExplorer* ge = NULL;
    IGameExplorer2* ge2 = NULL;

    /* interface available up from Vista */
    hr = CoCreateInstance( &CLSID_GameExplorer, NULL, CLSCTX_INPROC_SERVER, &IID_IGameExplorer, (LPVOID*)&ge);
    if(ge)
    {
        ok(hr == S_OK, "IGameExplorer creating failed (result false)\n");
        *gameExplorerAvailable = TRUE;
        IGameExplorer_Release(ge);
    }
    else
        win_skip("IGameExplorer cannot be created\n");

    /* interface available up from Win7 */
    hr = CoCreateInstance( &CLSID_GameExplorer, NULL, CLSCTX_INPROC_SERVER, &IID_IGameExplorer2, (LPVOID*)&ge2);
    if(ge2)
    {
        ok( hr == S_OK, "IGameExplorer2 creating failed (result false)\n");
        IGameExplorer2_Release(ge2);
    }
    else
        win_skip("IGameExplorer2 cannot be created\n");
}

static void test_add_remove_game(void)
{
    static const GUID defaultGUID = {0x01234567, 0x89AB, 0xCDEF,
        { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF}};

    HRESULT hr;

    IGameExplorer* ge = NULL;
    WCHAR sExeName[MAX_PATH];
    WCHAR sExePath[MAX_PATH];
    BSTR bstrExeName = NULL, bstrExePath = NULL;
    DWORD dwExeNameLen;
    GUID guid;

    hr = CoCreateInstance(&CLSID_GameExplorer, NULL, CLSCTX_INPROC_SERVER, &IID_IGameExplorer, (LPVOID*) & ge);
    ok(ge != NULL, "cannot create coclass IGameExplorer\n");
    ok(hr == S_OK, "cannot create coclass IGameExplorer\n");

    if(ge)
    {
        /* prepare path to binary */
        dwExeNameLen = GetModuleFileNameW(NULL, sExeName, sizeof (sExeName) / sizeof (sExeName[0]));
        ok(dwExeNameLen != 0, "GetModuleFileNameW returned invalid value\n");
        lstrcpynW(sExePath, sExeName, StrRChrW(sExeName, NULL, '\\') - sExeName + 1);
        bstrExeName = SysAllocString(sExeName);
        ok(bstrExeName != NULL, "cannot allocate string for exe name\n");
        bstrExePath = SysAllocString(sExePath);
        ok(bstrExePath != NULL, "cannot allocate string for exe path\n");

        if(bstrExeName && bstrExePath)
        {
            trace("prepared EXE name: %s\n", wine_dbgstr_w(bstrExeName));
            trace("prepared EXE path: %s\n", wine_dbgstr_w(bstrExePath));


            /* try to register game with provided guid */
            memcpy(&guid, &defaultGUID, sizeof (guid));

            hr = IGameExplorer_AddGame(ge, bstrExeName, bstrExePath, GIS_CURRENT_USER, &guid);
            todo_wine ok(SUCCEEDED(hr), "IGameExplorer::AddGame failed (error 0x%08x)\n", hr);
            ok(memcmp(&guid, &defaultGUID, sizeof (guid)) == 0, "AddGame unexpectedly modified GUID\n");

            if(SUCCEEDED(hr))
            {
                todo_wine _validateGameRegistryKey(__LINE__, GIS_CURRENT_USER, &guid, TRUE);

                hr = IGameExplorer_RemoveGame(ge, guid);
                todo_wine ok(SUCCEEDED(hr), "IGameExplorer::RemoveGame failed (error 0x%08x)\n", hr);
            }

            _validateGameRegistryKey(__LINE__, GIS_CURRENT_USER, &guid, FALSE);


            /* try to register game with empty guid */
            memcpy(&guid, &GUID_NULL, sizeof (guid));

            hr = IGameExplorer_AddGame(ge, bstrExeName, bstrExePath, GIS_CURRENT_USER, &guid);
            todo_wine ok(SUCCEEDED(hr), "IGameExplorer::AddGame failed (error 0x%08x)\n", hr);
            todo_wine ok(memcmp(&guid, &GUID_NULL, sizeof (guid)) != 0, "AddGame did not modify GUID\n");

            if(SUCCEEDED(hr))
            {
                todo_wine _validateGameRegistryKey(__LINE__, GIS_CURRENT_USER, &guid, TRUE);

                hr = IGameExplorer_RemoveGame(ge, guid);
                todo_wine ok(SUCCEEDED(hr), "IGameExplorer::RemoveGame failed (error 0x%08x)\n", hr);
            }

            _validateGameRegistryKey(__LINE__, GIS_CURRENT_USER, &guid, FALSE);
        }

        /* free allocated resources */
        SysFreeString(bstrExePath);
        SysFreeString(bstrExeName);

        IGameExplorer_Release(ge);
    }
}

START_TEST(gameexplorer)
{
    HRESULT r;
    BOOL gameExplorerAvailable = FALSE;

    if(_loadDynamicRoutines())
    {
        r = CoInitialize( NULL );
        ok( r == S_OK, "failed to init COM\n");

        test_create(&gameExplorerAvailable);

        if(gameExplorerAvailable)
            test_add_remove_game();
    }
    else
        /* this is not a failure, because both procedures loaded by address
         * are always available on systems which has gameux.dll */
        win_skip("too old system, cannot load required dynamic procedures\n");


    CoUninitialize();
}
