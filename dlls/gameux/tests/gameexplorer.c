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
static LONG WINAPI (*_RegGetValueW)(HKEY,LPCWSTR,LPCWSTR,DWORD,LPDWORD,PVOID,LPDWORD);

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
    _RegGetValueW = (LPVOID)GetProcAddress(hAdvapiModule, "RegGetValueW");
    if (!_RegGetValueW) return FALSE;
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
 *  _validateRegistryValue
 *
 * Helper function, verifies single registry value with expected
 *
 * Parameters:
 *  hKey                        [I]     handle to game's key. Key must be opened
 *  keyPath                     [I]     string with path to game's key. Used only
 *                                      to display more useful message on test fail
 *  valueName                   [I]     name of value to check
 *  dwExpectedType              [I]     expected type of value. It should be
 *                                      one of RRF_RT_* flags
 *  lpExpectedContent           [I]     expected content of value. It should be
 *                                      pointer to variable with same type as
 *                                      passed in dwExpectedType
 *
 * Returns:
 *  S_OK                                value exists and contains expected data
 *  S_FALSE                             value exists, but contains other data
 *                                      than expected
 *  E_OUTOFMEMORY                       allocation problem
 *  HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)
 *                                      value does not exist
 *
 * Note: this function returns error codes instead of failing test case, because
 * it is sometimes expected that given value may not exist on some systems, or
 * contain other data.
 */
static HRESULT _validateRegistryValue(
        HKEY hKey,
        LPCWSTR keyPath,
        LPCWSTR valueName,
        DWORD dwExpectedType,
        LPCVOID lpExpectedContent)
{
    HRESULT hr;
    DWORD dwType, dwSize;
    LPVOID lpData = NULL;

    hr = HRESULT_FROM_WIN32(_RegGetValueW(hKey, NULL, valueName, dwExpectedType, &dwType, NULL, &dwSize));
    if(FAILED(hr)) trace("registry value cannot be opened\n"
                "   key: %s\n"
                "   value: %s\n"
                "   expected type: 0x%x\n"
                "   found type: 0x%x\n"
                "   result code: 0x%0x\n",
                wine_dbgstr_w(keyPath),
                wine_dbgstr_w(valueName),
                dwExpectedType,
                dwType,
                hr);

    if(SUCCEEDED(hr))
    {
        lpData = CoTaskMemAlloc(dwSize);
        if(!lpData)
            hr = E_OUTOFMEMORY;
    }

    if(SUCCEEDED(hr))
        hr = HRESULT_FROM_WIN32(_RegGetValueW(hKey, NULL, valueName, dwExpectedType, &dwType, lpData, &dwSize));

    if(SUCCEEDED(hr))
    {
        if(memcmp(lpData, lpExpectedContent, dwSize)==0)
            hr = S_OK;
        else
        {
            if(dwExpectedType == RRF_RT_REG_SZ)
                /* if value type is REG_SZ, display expected and found values */
                trace("not expected content of registry value\n"
                        "   key: %s\n"
                        "   value: %s\n"
                        "   expected REG_SZ content: %s\n"
                        "   found REG_SZ content:    %s\n",
                        wine_dbgstr_w(keyPath),
                        wine_dbgstr_w(valueName),
                        wine_dbgstr_w(lpExpectedContent),
                        wine_dbgstr_w(lpData));
            else
                /* in the other case, do not display content */
                trace("not expected content of registry value\n"
                        "   key: %s\n"
                        "   value: %s\n"
                        "   value type: 0x%x\n",
                        wine_dbgstr_w(keyPath),
                        wine_dbgstr_w(valueName),
                        dwType);

            hr = S_FALSE;
        }
    }

    CoTaskMemFree(lpData);
    return hr;
}
/*******************************************************************************
 *  _validateGameRegistryValues
 *
 * Helper function, verifies values in game's registry key
 *
 * Parameters:
 *  line                        [I]     place of original call. Used only to display
 *                                      more useful message on test fail
 *  hKey                        [I]     handle to game's key. Key must be opened
 *                                      with KEY_READ access permission
 *  keyPath                     [I]     string with path to game's key. Used only
 *                                      to display more useful message on test fail
 *  gameApplicationId           [I]     game application identifier
 *  gameExePath                 [I]     directory where game executable is stored
 *  gameExeName                 [I]     full path to executable, including directory and file name
 */
static void _validateGameRegistryValues(int line,
        HKEY hKey,
        LPCWSTR keyPath,
        LPCGUID gameApplicationId,
        LPCWSTR gameExePath,
        LPCWSTR gameExeName)
{
    static const WCHAR sApplicationId[] = {'A','p','p','l','i','c','a','t','i','o','n','I','d',0};
    static const WCHAR sConfigApplicationPath[] = {'C','o','n','f','i','g','A','p','p','l','i','c','a','t','i','o','n','P','a','t','h',0};
    static const WCHAR sConfigGDFBinaryPath[] = {'C','o','n','f','i','g','G','D','F','B','i','n','a','r','y','P','a','t','h',0};
    static const WCHAR sDescription[] = {'D','e','s','c','r','i','p','t','i','o','n',0};
    static const WCHAR sExampleGame[] = {'E','x','a','m','p','l','e',' ','G','a','m','e',0};
    static const WCHAR sGameDescription[] = {'G','a','m','e',' ','D','e','s','c','r','i','p','t','i','o','n',0};
    static const WCHAR sTitle[] = {'T','i','t','l','e',0};

    HRESULT hr;
    WCHAR sGameApplicationId[40];

    hr = (StringFromGUID2(gameApplicationId, sGameApplicationId, sizeof(sGameApplicationId)/sizeof(sGameApplicationId[0])) ? S_OK : E_FAIL);
    ok_(__FILE__, line)(hr == S_OK, "cannot convert game application id to string\n");

    /* these values exist up from Vista */
    hr = _validateRegistryValue(hKey, keyPath,   sApplicationId,            RRF_RT_REG_SZ,      sGameApplicationId);
    todo_wine ok_(__FILE__, line)(hr==S_OK, "failed while checking registry value (error 0x%x)\n", hr);
    hr = _validateRegistryValue(hKey, keyPath,   sConfigApplicationPath,    RRF_RT_REG_SZ,      gameExePath);
    todo_wine ok_(__FILE__, line)(hr==S_OK, "failed while checking registry value (error 0x%x)\n", hr);
    hr = _validateRegistryValue(hKey, keyPath,   sConfigGDFBinaryPath,      RRF_RT_REG_SZ,      gameExeName);
    todo_wine ok_(__FILE__, line)(hr==S_OK, "failed while checking registry value (error 0x%x)\n", hr);
    hr = _validateRegistryValue(hKey, keyPath,   sTitle,                    RRF_RT_REG_SZ,      sExampleGame);
    todo_wine ok_(__FILE__, line)(hr==S_OK, "failed while checking registry value (error 0x%x)\n", hr);

    /* this value exists up from Win7 */
    hr = _validateRegistryValue(hKey, keyPath,   sDescription,              RRF_RT_REG_SZ,      sGameDescription);
    todo_wine ok_(__FILE__, line)(hr==S_OK || broken(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)), "failed while checking registry value (error 0x%x)\n", hr);
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
 *  gameApplicationId           [I]     game application identifier
 *  gameExePath                 [I]     directory where game executable is stored
 *  gameExeName                 [I]     full path to executable, including directory and file name
 *  presenceExpected            [I]     is it expected that game should be currently
 *                                      registered or not. Should be TRUE if checking
 *                                      after using AddGame/InstallGame, and FALSE
 *                                      if checking after RemoveGame/UninstallGame
 */
static void _validateGameRegistryKey(int line,
        GAME_INSTALL_SCOPE installScope,
        LPCGUID gameInstanceId,
        LPCGUID gameApplicationId,
        LPCWSTR gameExePath,
        LPCWSTR gameExeName,
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
    {
        if(presenceExpected)
            /* if the key exists and we expected it, let's verify it's content */
            _validateGameRegistryValues(line, hKey, lpRegistryPath, gameApplicationId, gameExePath, gameExeName);

        RegCloseKey(hKey);
    }

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
    static const GUID applicationId = { 0x17A6558E, 0x60BE, 0x4078,
        { 0xB6, 0x6F, 0x9C, 0x3A, 0xDA, 0x2A, 0x32, 0xE6 }};

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
                todo_wine _validateGameRegistryKey(__LINE__, GIS_CURRENT_USER, &guid, &applicationId, sExePath, sExeName, TRUE);

                hr = IGameExplorer_RemoveGame(ge, guid);
                todo_wine ok(SUCCEEDED(hr), "IGameExplorer::RemoveGame failed (error 0x%08x)\n", hr);
            }

            _validateGameRegistryKey(__LINE__, GIS_CURRENT_USER, &guid, &applicationId, sExePath, sExeName, FALSE);


            /* try to register game with empty guid */
            memcpy(&guid, &GUID_NULL, sizeof (guid));

            hr = IGameExplorer_AddGame(ge, bstrExeName, bstrExePath, GIS_CURRENT_USER, &guid);
            todo_wine ok(SUCCEEDED(hr), "IGameExplorer::AddGame failed (error 0x%08x)\n", hr);
            todo_wine ok(memcmp(&guid, &GUID_NULL, sizeof (guid)) != 0, "AddGame did not modify GUID\n");

            if(SUCCEEDED(hr))
            {
                todo_wine _validateGameRegistryKey(__LINE__, GIS_CURRENT_USER, &guid, &applicationId, sExePath, sExeName, TRUE);

                hr = IGameExplorer_RemoveGame(ge, guid);
                todo_wine ok(SUCCEEDED(hr), "IGameExplorer::RemoveGame failed (error 0x%08x)\n", hr);
            }

            _validateGameRegistryKey(__LINE__, GIS_CURRENT_USER, &guid, &applicationId, sExePath, sExeName, FALSE);
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
