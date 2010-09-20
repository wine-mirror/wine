/*
 * IGameStatisticsMgr tests
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

#include "shlwapi.h"
#include "oleauto.h"

#include "gameux.h"
#include "wine/test.h"

/*******************************************************************************
 * utilities
 */
static IGameExplorer *ge = NULL;
static WCHAR sExeName[MAX_PATH] = {0};
static GUID gameInstanceId;
/*******************************************************************************
 * _registerGame
 * Registers test suite executable as game in Games Explorer. Required to test
 * game statistics.
 */
static HRESULT _registerGame(void) {
    HRESULT hr;
    WCHAR sExePath[MAX_PATH];
    BSTR bstrExeName, bstrExePath;
    DWORD dwExeNameLen;

    /* prepare path to binary */
    dwExeNameLen = GetModuleFileNameW(NULL, sExeName, sizeof (sExeName) / sizeof (sExeName[0]));
    hr = (dwExeNameLen!= 0 ? S_OK : E_FAIL);
    lstrcpynW(sExePath, sExeName, StrRChrW(sExeName, NULL, '\\') - sExeName + 1);

    bstrExeName = SysAllocString(sExeName);
    if(!bstrExeName) hr = E_OUTOFMEMORY;

    bstrExePath = SysAllocString(sExePath);
    if(!bstrExePath) hr = E_OUTOFMEMORY;

    if(SUCCEEDED(hr))
    {
        gameInstanceId = GUID_NULL;
        hr = CoCreateInstance(&CLSID_GameExplorer, NULL, CLSCTX_INPROC_SERVER,
                              &IID_IGameExplorer, (LPVOID*)&ge);
    }

    if(SUCCEEDED(hr))
        hr = IGameExplorer_AddGame(ge, bstrExeName, bstrExePath,
                                   GIS_CURRENT_USER, &gameInstanceId);

    if(FAILED(hr) && ge)
    {
        IGameExplorer_Release(ge);
        ge = NULL;
    }

    SysFreeString(bstrExeName);
    SysFreeString(bstrExePath);
    return hr;
}
/*******************************************************************************
 * _unregisterGame
 * Unregisters test suite from Games Explorer.
 */
static HRESULT _unregisterGame(void) {
    HRESULT hr;

    if(!ge) return E_FAIL;

    hr = IGameExplorer_RemoveGame(ge, gameInstanceId);

    IGameExplorer_Release(ge);
    ge = NULL;

    return hr;
}
/*******************************************************************************
 * test routines
 */
static void test_create(BOOL* gameStatisticsAvailable)
{
    HRESULT hr;

    IGameStatisticsMgr* gsm = NULL;
    *gameStatisticsAvailable = FALSE;

    /* interface available up from Win7 */
    hr = CoCreateInstance( &CLSID_GameStatistics, NULL, CLSCTX_INPROC_SERVER, &IID_IGameStatisticsMgr, (LPVOID*)&gsm);
    if(gsm)
    {
        ok( hr == S_OK, "IGameStatisticsMgr creating failed (result: 0x%08x)\n", hr);
        *gameStatisticsAvailable = TRUE;
        IGameStatisticsMgr_Release(gsm);
    }
    else
        win_skip("IGameStatisticsMgr cannot be created\n");
}
static void test_gamestatisticsmgr( void )
{
    HRESULT hr;
    DWORD dwOpenResult;

    IGameStatisticsMgr* gsm = NULL;
    IGameStatistics* gs = NULL;

    hr = CoCreateInstance( &CLSID_GameStatistics, NULL, CLSCTX_INPROC_SERVER, &IID_IGameStatisticsMgr, (LPVOID*)&gsm);
    ok(hr == S_OK, "IGameStatisticsMgr creating failed (result false)\n");

    /* test trying to create interface IGameStatistics using GetGameStatistics method */
    hr = IGameStatisticsMgr_GetGameStatistics(gsm, sExeName, GAMESTATS_OPEN_OPENORCREATE, &dwOpenResult, &gs);
    todo_wine ok(SUCCEEDED(hr), "GetGameStatistics returned error: 0x%x\n", hr);
    todo_wine ok(gs!=NULL, "GetGameStatistics did not return valid interface pointer\n");
    if(gs)
    {
        hr = IGameStatistics_Release(gs);
        ok(SUCCEEDED(hr), "releasing IGameStatistics returned error: 0x%x\n", hr);

        /* test of removing game statistics from underlying storage */
        hr = IGameStatisticsMgr_RemoveGameStatistics(gsm, sExeName);
        todo_wine ok(SUCCEEDED(hr), "cannot remove game statistics, error: 0x%x\n", hr);
    }

    hr = IGameStatisticsMgr_Release(gsm);
    ok(SUCCEEDED(hr), "releasing IGameStatisticsMgr returned error: 0x%x\n", hr);
}

START_TEST(gamestatistics)
{
    HRESULT hr;
    BOOL gameStatisticsAvailable;

    hr = CoInitialize( NULL );
    ok( hr == S_OK, "failed to init COM\n");

    test_create(&gameStatisticsAvailable);

    if(gameStatisticsAvailable)
    {
        hr = _registerGame();
        ok( hr == S_OK, "cannot register game in Game Explorer (error: 0x%x)\n", hr);

        test_gamestatisticsmgr();

        hr = _unregisterGame();
        ok( hr == S_OK, "cannot unregister game from Game Explorer (error: 0x%x)\n", hr);
    }

    CoUninitialize();
}
