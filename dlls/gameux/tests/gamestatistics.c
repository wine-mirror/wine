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
#include "shlobj.h"

#include "gameux.h"
#include "wine/test.h"

/*******************************************************************************
 * utilities
 */
static IGameExplorer *ge = NULL;
static WCHAR sExeName[MAX_PATH] = {0};
static GUID gameInstanceId;
static HRESULT WINAPI (*pSHGetFolderPathW)(HWND,int,HANDLE,DWORD,LPWSTR);
/*******************************************************************************
 *_loadDynamicRoutines
 *
 * Helper function, prepares pointers to system procedures which may be not
 * available on older operating systems.
 *
 * Returns:
 *  TRUE                        procedures were loaded successfully
 *  FALSE                       procedures were not loaded successfully
 */
static BOOL _loadDynamicRoutines(void)
{
    HMODULE hModule = LoadLibraryA( "shell32.dll" );

    pSHGetFolderPathW = (LPVOID)GetProcAddress(hModule, "SHGetFolderPathW");
    if (!pSHGetFolderPathW) return FALSE;
    return TRUE;
}
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
 * _buildStatisticsFilePath
 * Creates path to file containing statistics of game with given id.
 *
 * Parameters:
 *  guidApplicationId                       [I]     application id of game
 *  lpStatisticsFile                        [O]     pointer where address of
 *                                                  string with path will be
 *                                                  stored. Path must be deallocated
 *                                                  using CoTaskMemFree(...)
 */
static HRESULT _buildStatisticsFilePath(LPCGUID guidApplicationId, LPWSTR *lpStatisticsFile)
{
    static const WCHAR sBackslash[] = {'\\',0};
    static const WCHAR sStatisticsDir[] = {'\\','M','i','c','r','o','s','o','f','t',
            '\\','W','i','n','d','o','w','s','\\','G','a','m','e','E','x','p',
            'l','o','r','e','r','\\','G','a','m','e','S','t','a','t','i','s',
            't','i','c','s','\\',0};
    static const WCHAR sDotGamestats[] = {'.','g','a','m','e','s','t','a','t','s',0};
    HRESULT hr;
    WCHAR sGuid[49], sPath[MAX_PATH];

    hr = pSHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, sPath);

    if(SUCCEEDED(hr))
        hr = (StringFromGUID2(guidApplicationId, sGuid, sizeof(sGuid) / sizeof(sGuid[0])) != 0 ? S_OK : E_FAIL);

    if(SUCCEEDED(hr))
    {
        lstrcatW(sPath, sStatisticsDir);
        lstrcatW(sPath, sGuid);
        lstrcatW(sPath, sBackslash);
        lstrcatW(sPath, sGuid);
        lstrcatW(sPath, sDotGamestats);

        *lpStatisticsFile = CoTaskMemAlloc((lstrlenW(sPath)+1)*sizeof(WCHAR));
        if(!*lpStatisticsFile) hr = E_OUTOFMEMORY;
    }

    if(SUCCEEDED(hr))
        lstrcpyW(*lpStatisticsFile, sPath);

    return hr;
}
/*******************************************************************************
 * _isFileExist
 * Checks if given file exists
 *
 * Parameters:
 *  lpFile                          [I]     path to file
 *
 * Result:
 *  TRUE        file exists
 *  FALSE       file does not exist
 */
static BOOL _isFileExists(LPCWSTR lpFile)
{
    HANDLE hFile = CreateFileW(lpFile, GENERIC_READ, 0, NULL,
                               OPEN_EXISTING, 0, NULL);
    if(hFile == INVALID_HANDLE_VALUE) return FALSE;
    CloseHandle(hFile);
    return TRUE;
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
    static const GUID guidApplicationId = { 0x17A6558E, 0x60BE, 0x4078, { 0xB6, 0x6F, 0x9C, 0x3A, 0xDA, 0x2A, 0x32, 0xE6 } };
    static const WCHAR sCategory0[] = {'C','a','t','e','g','o','r','y','0',0};
    static const WCHAR sCategory1[] = {'C','a','t','e','g','o','r','y','1',0};
    static const WCHAR sCategory2[] = {'C','a','t','e','g','o','r','y','2',0};
    static const WCHAR sCategory0a[] = {'C','a','t','e','g','o','r','y','0','a',0};
    static const WCHAR sStatistic00[] = {'S','t','a','t','i','s','t','i','c','0','0',0};
    static const WCHAR sStatistic01[] = {'S','t','a','t','i','s','t','i','c','0','1',0};
    static const WCHAR sStatistic10[] = {'S','t','a','t','i','s','t','i','c','1','0',0};
    static const WCHAR sStatistic11[] = {'S','t','a','t','i','s','t','i','c','1','1',0};
    static const WCHAR sStatistic20[] = {'S','t','a','t','i','s','t','i','c','2','0',0};
    static const WCHAR sStatistic21[] = {'S','t','a','t','i','s','t','i','c','2','1',0};
    static const WCHAR sValue00[] = {'V','a','l','u','e','0','0',0};
    static const WCHAR sValue01[] = {'V','a','l','u','e','0','1',0};
    static const WCHAR sValue10[] = {'V','a','l','u','e','1','0',0};
    static const WCHAR sValue11[] = {'V','a','l','u','e','1','1',0};
    static const WCHAR sValue20[] = {'V','a','l','u','e','2','0',0};
    static const WCHAR sValue21[] = {'V','a','l','u','e','2','1',0};

    HRESULT hr;
    DWORD dwOpenResult;
    LPWSTR lpStatisticsFile = NULL;
    LPWSTR lpName = NULL, lpValue = NULL, sTooLongString = NULL;
    UINT uMaxCategoryLength = 0, uMaxNameLength = 0, uMaxValueLength = 0;
    WORD wMaxStatsPerCategory = 0, wMaxCategories = 0;

    IGameStatisticsMgr* gsm = NULL;
    IGameStatistics* gs;

    hr = CoCreateInstance( &CLSID_GameStatistics, NULL, CLSCTX_INPROC_SERVER, &IID_IGameStatisticsMgr, (LPVOID*)&gsm);
    ok(hr == S_OK, "IGameStatisticsMgr creating failed (result false)\n");

    /* test trying to create interface IGameStatistics using GetGameStatistics method */

    /* this should fail, cause statistics doesn't yet exists */
    gs = (void *)0xdeadbeef;
    hr = IGameStatisticsMgr_GetGameStatistics(gsm, sExeName, GAMESTATS_OPEN_OPENONLY, &dwOpenResult, &gs);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "GetGameStatistics returned unexpected value: 0x%08x\n", hr);
    ok(gs == NULL, "Expected output pointer to be NULL, got %p\n", gs);

    /* now, allow to create */
    hr = IGameStatisticsMgr_GetGameStatistics(gsm, sExeName, GAMESTATS_OPEN_OPENORCREATE, &dwOpenResult, &gs);
    ok(SUCCEEDED(hr), "GetGameStatistics returned error: 0x%x\n", hr);
    ok(gs!=NULL, "GetGameStatistics did not return valid interface pointer\n");
    if(gs)
    {
        /* test of limit values returned from interface */
        hr = IGameStatistics_GetMaxCategoryLength(gs, &uMaxCategoryLength);
        ok(hr==S_OK, "getting maximum length of category failed\n");
        ok(uMaxCategoryLength==60, "getting maximum length of category returned invalid value: %d\n", uMaxCategoryLength);

        hr = IGameStatistics_GetMaxNameLength(gs, &uMaxNameLength);
        ok(hr==S_OK, "getting maximum name length failed\n");
        ok(uMaxNameLength==30, "getting maximum name length returned invalid value: %d\n", uMaxNameLength);

        hr = IGameStatistics_GetMaxValueLength(gs, &uMaxValueLength);
        ok(hr==S_OK, "getting maximum value length failed\n");
        ok(uMaxValueLength==30, "getting maximum value length returned invalid value: %d\n", uMaxValueLength);

        hr = IGameStatistics_GetMaxCategories(gs, &wMaxCategories);
        ok(hr==S_OK, "getting maximum number of categories failed\n");
        ok(wMaxCategories==10, "getting maximum number of categories returned invalid value: %d\n", wMaxCategories);

        hr = IGameStatistics_GetMaxStatsPerCategory(gs, &wMaxStatsPerCategory);
        ok(hr==S_OK, "getting maximum number of statistics per category failed\n");
        ok(wMaxStatsPerCategory==10, "getting maximum number of statistics per category returned invalid value: %d\n", wMaxStatsPerCategory);

        /* create name of statistics file */
        hr = _buildStatisticsFilePath(&guidApplicationId, &lpStatisticsFile);
        ok(SUCCEEDED(hr), "cannot build path to game statistics (error 0x%x)\n", hr);
        trace("statistics file path: %s\n", wine_dbgstr_w(lpStatisticsFile));
        ok(_isFileExists(lpStatisticsFile) == FALSE, "statistics file %s already exists\n", wine_dbgstr_w(lpStatisticsFile));

        /* write sample statistics */
        hr = IGameStatistics_SetCategoryTitle(gs, wMaxCategories, NULL);
        ok(hr==E_INVALIDARG, "setting category title invalid value: 0x%x\n", hr);

        hr = IGameStatistics_SetCategoryTitle(gs, wMaxCategories, sCategory0);
        ok(hr==E_INVALIDARG, "setting category title invalid value: 0x%x\n", hr);

        /* check what happen if string is too long */
        sTooLongString = CoTaskMemAlloc(sizeof(WCHAR)*(uMaxCategoryLength+2));
        memset(sTooLongString, 'a', sizeof(WCHAR)*(uMaxCategoryLength+1));
        sTooLongString[uMaxCategoryLength+1]=0;

        /* when string is too long, Windows returns S_FALSE, but saves string (stripped to expected number of characters) */
        hr = IGameStatistics_SetCategoryTitle(gs, 0, sTooLongString);
        ok(hr==S_FALSE, "setting category title invalid result: 0x%x\n", hr);
        CoTaskMemFree(sTooLongString);

        ok(IGameStatistics_SetCategoryTitle(gs, 0, sCategory0)==S_OK, "setting category title failed: %s\n", wine_dbgstr_w(sCategory0));
        ok(IGameStatistics_SetCategoryTitle(gs, 1, sCategory1)==S_OK, "setting category title failed: %s\n", wine_dbgstr_w(sCategory1));
        ok(IGameStatistics_SetCategoryTitle(gs, 2, sCategory2)==S_OK, "setting category title failed: %s\n", wine_dbgstr_w(sCategory1));

        /* check what happen if any string is NULL */
        hr = IGameStatistics_SetStatistic(gs, 0, 0, NULL, sValue00);
        ok(hr == S_FALSE, "setting statistic returned unexpected value: 0x%x)\n", hr);

        hr = IGameStatistics_SetStatistic(gs, 0, 0, sStatistic00, NULL);
        ok(hr == S_OK, "setting statistic returned unexpected value: 0x%x)\n", hr);

        /* check what happen if any string is too long */
        sTooLongString = CoTaskMemAlloc(sizeof(WCHAR)*(uMaxNameLength+2));
        memset(sTooLongString, 'a', sizeof(WCHAR)*(uMaxNameLength+1));
        sTooLongString[uMaxNameLength+1]=0;
        hr = IGameStatistics_SetStatistic(gs, 0, 0, sTooLongString, sValue00);
        ok(hr == S_FALSE, "setting statistic returned unexpected value: 0x%x)\n", hr);
        CoTaskMemFree(sTooLongString);

        sTooLongString = CoTaskMemAlloc(sizeof(WCHAR)*(uMaxValueLength+2));
        memset(sTooLongString, 'a', sizeof(WCHAR)*(uMaxValueLength+1));
        sTooLongString[uMaxValueLength+1]=0;
        hr = IGameStatistics_SetStatistic(gs, 0, 0, sStatistic00, sTooLongString);
        ok(hr == S_FALSE, "setting statistic returned unexpected value: 0x%x)\n", hr);
        CoTaskMemFree(sTooLongString);

        /* check what happen on too big index of category or statistic */
        hr = IGameStatistics_SetStatistic(gs, wMaxCategories, 0, sStatistic00, sValue00);
        ok(hr == E_INVALIDARG, "setting statistic returned unexpected value: 0x%x)\n", hr);

        hr = IGameStatistics_SetStatistic(gs, 0, wMaxStatsPerCategory, sStatistic00, sValue00);
        ok(hr == E_INVALIDARG, "setting statistic returned unexpected value: 0x%x)\n", hr);

        ok(IGameStatistics_SetStatistic(gs, 0, 0, sStatistic00, sValue00)==S_OK, "setting statistic failed: name=%s, value=%s\n", wine_dbgstr_w(sStatistic00), wine_dbgstr_w(sValue00));
        ok(IGameStatistics_SetStatistic(gs, 0, 1, sStatistic01, sValue01)==S_OK, "setting statistic failed: name=%s, value=%s\n", wine_dbgstr_w(sStatistic01), wine_dbgstr_w(sValue01));
        ok(IGameStatistics_SetStatistic(gs, 1, 0, sStatistic10, sValue10)==S_OK, "setting statistic failed: name=%s, value=%s\n", wine_dbgstr_w(sStatistic10), wine_dbgstr_w(sValue10));
        ok(IGameStatistics_SetStatistic(gs, 1, 1, sStatistic11, sValue11)==S_OK, "setting statistic failed: name=%s, value=%s\n", wine_dbgstr_w(sStatistic11), wine_dbgstr_w(sValue11));
        ok(IGameStatistics_SetStatistic(gs, 2, 0, sStatistic20, sValue20)==S_OK, "setting statistic failed: name=%s, value=%s\n", wine_dbgstr_w(sStatistic20), wine_dbgstr_w(sValue20));
        ok(IGameStatistics_SetStatistic(gs, 2, 1, sStatistic21, sValue21)==S_OK, "setting statistic failed: name=%s, value=%s\n", wine_dbgstr_w(sStatistic21), wine_dbgstr_w(sValue21));

        ok(_isFileExists(lpStatisticsFile) == FALSE, "statistics file %s already exists\n", wine_dbgstr_w(lpStatisticsFile));

        ok(IGameStatistics_Save(gs, FALSE)==S_OK, "statistic saving failed\n");

        ok(_isFileExists(lpStatisticsFile) == TRUE, "statistics file %s does not exists\n", wine_dbgstr_w(lpStatisticsFile));

        /* this value should not be stored in storage, we need it only to test is it not saved */
        ok(IGameStatistics_SetCategoryTitle(gs, 0, sCategory0a)==S_OK, "setting category title failed: %s\n", wine_dbgstr_w(sCategory0a));

        hr = IGameStatistics_Release(gs);
        ok(SUCCEEDED(hr), "releasing IGameStatistics returned error: 0x%08x\n", hr);

        /* try to read written statistics */
        hr = IGameStatisticsMgr_GetGameStatistics(gsm, sExeName, GAMESTATS_OPEN_OPENORCREATE, &dwOpenResult, &gs);
        ok(SUCCEEDED(hr), "GetGameStatistics returned error: 0x%08x\n", hr);
        ok(dwOpenResult == GAMESTATS_OPEN_OPENED, "GetGameStatistics returned invalid open result: 0x%x\n", dwOpenResult);
        ok(gs!=NULL, "GetGameStatistics did not return valid interface pointer\n");

        /* verify values with these which we stored before*/
        hr = IGameStatistics_GetCategoryTitle(gs, 0, &lpName);
        ok(hr == S_OK, "getting category title failed\n");
        ok(lstrcmpW(lpName, sCategory0)==0, "getting category title returned invalid string (%s)\n", wine_dbgstr_w(lpName));
        CoTaskMemFree(lpName);

        hr = IGameStatistics_GetCategoryTitle(gs, 1, &lpName);
        ok(hr == S_OK, "getting category title failed\n");
        ok(lstrcmpW(lpName, sCategory1)==0, "getting category title returned invalid string (%s)\n", wine_dbgstr_w(lpName));
        CoTaskMemFree(lpName);

        hr = IGameStatistics_GetCategoryTitle(gs, 2, &lpName);
        ok(hr == S_OK, "getting category title failed\n");
        ok(lstrcmpW(lpName, sCategory2)==0, "getting category title returned invalid string (%s)\n", wine_dbgstr_w(lpName));
        CoTaskMemFree(lpName);

        /* check result if category doesn't exists */
        hr = IGameStatistics_GetCategoryTitle(gs, 3, &lpName);
        ok(hr == S_OK, "getting category title failed\n");
        ok(lpName == NULL, "getting category title failed\n");
        CoTaskMemFree(lpName);

        hr = IGameStatistics_GetStatistic(gs, 0, 0, &lpName, &lpValue);
        ok(hr == S_OK, "getting statistic failed\n");
        ok(lstrcmpW(lpName, sStatistic00)==0, "getting statistic returned invalid name\n");
        ok(lstrcmpW(lpValue, sValue00)==0, "getting statistic returned invalid value\n");
        CoTaskMemFree(lpName);
        CoTaskMemFree(lpValue);

        hr = IGameStatistics_GetStatistic(gs, 0, 1, &lpName, &lpValue);
        ok(hr == S_OK, "getting statistic failed\n");
        ok(lstrcmpW(lpName, sStatistic01)==0, "getting statistic returned invalid name\n");
        ok(lstrcmpW(lpValue, sValue01)==0, "getting statistic returned invalid value\n");
        CoTaskMemFree(lpName);
        CoTaskMemFree(lpValue);

        hr = IGameStatistics_GetStatistic(gs, 1, 0, &lpName, &lpValue);
        ok(hr == S_OK, "getting statistic failed\n");
        ok(lstrcmpW(lpName, sStatistic10)==0, "getting statistic returned invalid name\n");
        ok(lstrcmpW(lpValue, sValue10)==0, "getting statistic returned invalid value\n");
        CoTaskMemFree(lpName);
        CoTaskMemFree(lpValue);

        hr = IGameStatistics_GetStatistic(gs, 1, 1, &lpName, &lpValue);
        ok(hr == S_OK, "getting statistic failed\n");
        ok(lstrcmpW(lpName, sStatistic11)==0, "getting statistic returned invalid name\n");
        ok(lstrcmpW(lpValue, sValue11)==0, "getting statistic returned invalid value\n");
        CoTaskMemFree(lpName);
        CoTaskMemFree(lpValue);

        hr = IGameStatistics_GetStatistic(gs, 2, 0, &lpName, &lpValue);
        ok(hr == S_OK, "getting statistic failed\n");
        ok(lstrcmpW(lpName, sStatistic20)==0, "getting statistic returned invalid name\n");
        ok(lstrcmpW(lpValue, sValue20)==0, "getting statistic returned invalid value\n");
        CoTaskMemFree(lpName);
        CoTaskMemFree(lpValue);

        hr = IGameStatistics_GetStatistic(gs, 2, 1, &lpName, &lpValue);
        ok(hr == S_OK, "getting statistic failed\n");
        ok(lstrcmpW(lpName, sStatistic21)==0, "getting statistic returned invalid name\n");
        ok(lstrcmpW(lpValue, sValue21)==0, "getting statistic returned invalid value\n");
        CoTaskMemFree(lpName);
        CoTaskMemFree(lpValue);

        hr = IGameStatistics_Release(gs);
        ok(SUCCEEDED(hr), "releasing IGameStatistics returned error: 0x%x\n", hr);

        /* test of removing game statistics from underlying storage */
        ok(_isFileExists(lpStatisticsFile) == TRUE, "statistics file %s does not exists\n", wine_dbgstr_w(lpStatisticsFile));
        hr = IGameStatisticsMgr_RemoveGameStatistics(gsm, sExeName);
        ok(SUCCEEDED(hr), "cannot remove game statistics, error: 0x%x\n", hr);
        ok(_isFileExists(lpStatisticsFile) == FALSE, "statistics file %s still exists\n", wine_dbgstr_w(lpStatisticsFile));
    }

    hr = IGameStatisticsMgr_Release(gsm);
    ok(SUCCEEDED(hr), "releasing IGameStatisticsMgr returned error: 0x%x\n", hr);

    CoTaskMemFree(lpStatisticsFile);
}

START_TEST(gamestatistics)
{
    HRESULT hr;
    BOOL gameStatisticsAvailable;

    if(_loadDynamicRoutines())
    {
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
    else
        /* this is not a failure, because a procedure loaded by address
         * is always available on systems which has gameux.dll */
        win_skip("too old system, cannot load required dynamic procedures\n");
}
