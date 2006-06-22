/* Unit test suite for uxtheme API functions
 *
 * Copyright 2006 Paul Vriens
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
 *
 */

#include <stdarg.h>

#include "windows.h"
#include "uxtheme.h"

#include "wine/test.h"

static HRESULT (WINAPI * pCloseThemeData)(HTHEME);
static HTHEME  (WINAPI * pGetWindowTheme)(HWND);
static BOOL    (WINAPI * pIsAppThemed)(VOID);
static BOOL    (WINAPI * pIsThemeActive)(VOID);
static HTHEME  (WINAPI * pOpenThemeData)(HWND, LPCWSTR);
static HRESULT (WINAPI * pSetWindowTheme)(HWND, LPCWSTR, LPCWSTR);

static HMODULE hUxtheme = 0;

#define UXTHEME_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hUxtheme, #func); \
    if(!p ## func) { \
      trace("GetProcAddress(%s) failed\n", #func); \
      FreeLibrary(hUxtheme); \
      return FALSE; \
    }

static BOOL InitFunctionPtrs(void)
{
    hUxtheme = LoadLibraryA("uxtheme.dll");
    if(!hUxtheme) {
      trace("Could not load uxtheme.dll\n");
      return FALSE;
    }
    if (hUxtheme)
    {
      UXTHEME_GET_PROC(CloseThemeData)
      UXTHEME_GET_PROC(GetWindowTheme)
      UXTHEME_GET_PROC(IsAppThemed)
      UXTHEME_GET_PROC(IsThemeActive)
      UXTHEME_GET_PROC(OpenThemeData)
      UXTHEME_GET_PROC(SetWindowTheme)
    }
    /* The following functions should be available, if not return FALSE. The Vista functions will
     * be checked (at some point in time) within the single tests if needed. All used functions for
     * now are present on WinXP, W2K3 and Wine.
     */
    if (!pCloseThemeData || !pGetWindowTheme ||
        !pIsAppThemed || !pIsThemeActive ||
        !pOpenThemeData || !pSetWindowTheme)
        return FALSE;

    return TRUE;
}

static void test_IsThemed(void)
{
    BOOL bThemeActive;
    BOOL bAppThemed;

    SetLastError(0xdeadbeef);
    bThemeActive = pIsThemeActive();
    trace("Theming is %s\n", (bThemeActive) ? "active" : "inactive");
    todo_wine
        ok( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got 0x%08lx\n",
            GetLastError());

    /* This test is not themed */
    SetLastError(0xdeadbeef);
    bAppThemed = pIsAppThemed();

    if (bThemeActive)
        todo_wine
            ok( bAppThemed == FALSE, "Expected FALSE as this test executable is not (yet) themed.\n");
    else
        /* Although Wine currently returns FALSE, the logic behind it is wrong. It is not a todo_wine though in the testing sense */
        ok( bAppThemed == FALSE, "Expected FALSE as this test executable is not (yet) themed.\n");

    todo_wine
        ok( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got 0x%08lx\n",
            GetLastError());
}

static void test_GetWindowTheme(void)
{
    HTHEME    hTheme;
    HWND      hWnd;
    BOOL    bDestroyed;

    SetLastError(0xdeadbeef);
    hTheme = pGetWindowTheme(NULL);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    todo_wine
        ok( GetLastError() == E_HANDLE,
            "Expected E_HANDLE, got 0x%08lx\n",
            GetLastError());

    /* Only do the bare minumum to get a valid hwnd */
    hWnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,0, 0, 0, NULL);
    if (!hWnd) return;

    SetLastError(0xdeadbeef);
    hTheme = pGetWindowTheme(hWnd);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got 0x%08lx\n",
        GetLastError());

    bDestroyed = DestroyWindow(hWnd);
    if (!bDestroyed)
        trace("Window %p couldn't be destroyed : 0x%08lx\n",
            hWnd, GetLastError());
}

static void test_SetWindowTheme(void)
{
    HRESULT hRes;
    HWND    hWnd;
    BOOL    bDestroyed;

    SetLastError(0xdeadbeef);
    hRes = pSetWindowTheme(NULL, NULL, NULL);
    todo_wine
    {
        ok( hRes == E_HANDLE, "Expected E_HANDLE, got 0x%08lx\n", hRes);
        ok( GetLastError() == 0xdeadbeef,
            "Expected 0xdeadbeef, got 0x%08lx\n",
            GetLastError());
    }

    /* Only do the bare minumum to get a valid hwnd */
    hWnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,0, 0, 0, NULL);
    if (!hWnd) return;

    SetLastError(0xdeadbeef);
    hRes = pSetWindowTheme(hWnd, NULL, NULL);
    ok( hRes == S_OK, "Expected S_OK, got 0x%08lx\n", hRes);
    ok( GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got 0x%08lx\n",
        GetLastError());

    bDestroyed = DestroyWindow(hWnd);
    if (!bDestroyed)
        trace("Window %p couldn't be destroyed : 0x%08lx\n",
            hWnd, GetLastError());
}

static void test_OpenThemeData(void)
{
    HTHEME    hTheme;
    HWND      hWnd;
    BOOL      bThemeActive;
    BOOL      bDestroyed;

    WCHAR szInvalidClassList[] = {'D','E','A','D','B','E','E','F', 0 };
    WCHAR szButtonClassList[]  = {'B','u','t','t','o','n', 0 };
    WCHAR szButtonClassList2[]  = {'b','U','t','T','o','N', 0 };
    WCHAR szClassList[]        = {'B','u','t','t','o','n',';','L','i','s','t','B','o','x', 0 };

    bThemeActive = pIsThemeActive();

    /* All NULL */
    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeData(NULL, NULL);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    todo_wine
        ok( GetLastError() == E_POINTER,
            "Expected GLE() to be E_POINTER, got 0x%08lx\n",
            GetLastError());

    /* A NULL hWnd and an invalid classlist */
    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeData(NULL, szInvalidClassList);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    todo_wine
        ok( GetLastError() == E_PROP_ID_UNSUPPORTED,
            "Expected GLE() to be E_PROP_ID_UNSUPPORTED, got 0x%08lx\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeData(NULL, szClassList);
    if (bThemeActive)
    {
        ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
        todo_wine
            ok( GetLastError() == ERROR_SUCCESS,
                "Expected ERROR_SUCCESS, got 0x%08lx\n",
                GetLastError());
    }
    else
    {
        ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
        todo_wine
            ok( GetLastError() == E_PROP_ID_UNSUPPORTED,
                "Expected GLE() to be E_PROP_ID_UNSUPPORTED, got 0x%08lx\n",
                GetLastError());
    }

    /* Only do the bare minumum to get a valid hdc */
    hWnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,0, 0, 0, NULL);
    if (!hWnd) return;

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeData(hWnd, NULL);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    todo_wine
        ok( GetLastError() == E_POINTER,
            "Expected GLE() to be E_POINTER, got 0x%08lx\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeData(hWnd, szInvalidClassList);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    todo_wine
        ok( GetLastError() == E_PROP_ID_UNSUPPORTED,
            "Expected GLE() to be E_PROP_ID_UNSUPPORTED, got 0x%08lx\n",
            GetLastError());

    if (!bThemeActive)
    {
        SetLastError(0xdeadbeef);
        hTheme = pOpenThemeData(hWnd, szButtonClassList);
        ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
        todo_wine
            ok( GetLastError() == E_PROP_ID_UNSUPPORTED,
                "Expected GLE() to be E_PROP_ID_UNSUPPORTED, got 0x%08lx\n",
                GetLastError());
        trace("No active theme, skipping rest of OpenThemeData tests\n");
        return;
    }

    /* Only do the next checks if we have an active theme */

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeData(hWnd, szButtonClassList);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    todo_wine
        ok( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got 0x%08lx\n",
            GetLastError());

    /* Test with bUtToN instead of Button */
    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeData(hWnd, szButtonClassList2);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    todo_wine
        ok( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got 0x%08lx\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeData(hWnd, szClassList);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    todo_wine
        ok( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got 0x%08lx\n",
            GetLastError());

    bDestroyed = DestroyWindow(hWnd);
    if (!bDestroyed)
        trace("Window %p couldn't be destroyed : 0x%08lx\n",
            hWnd, GetLastError());
}

static void test_CloseThemeData(void)
{
    HRESULT hRes;

    SetLastError(0xdeadbeef);
    hRes = pCloseThemeData(NULL);
    ok( hRes == E_HANDLE, "Expected E_HANDLE, got 0x%08lx\n", hRes);
    ok( GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got 0x%08lx\n",
        GetLastError());
}

START_TEST(system)
{
    if(!InitFunctionPtrs())
        return;

    /* No real functional tests will be done (yet). The current tests
     * only show input/return behaviour
     */

    /* IsThemeActive and IsAppThemed */
    trace("Starting test_IsThemed()\n");
    test_IsThemed();

    /* GetWindowTheme */
    trace("Starting test_GetWindowTheme()\n");
    test_GetWindowTheme();

    /* SetWindowTheme */
    trace("Starting test_SetWindowTheme()\n");
    test_SetWindowTheme();

    /* OpenThemeData */
    trace("Starting test_OpenThemeData()\n");
    test_OpenThemeData();

    /* CloseThemeData */
    trace("Starting test_CloseThemeData()\n");
    test_CloseThemeData();

    FreeLibrary(hUxtheme);
}
