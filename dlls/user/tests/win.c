/*
 * Unit tests for window handling
 *
 * Copyright 2002 Bill Medland
 * Copyright 2002 Alexandre Julliard
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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/test.h"

#ifndef IDC_ARROWA
# define IDC_ARROWA IDC_ARROW
#endif

#ifndef SPI_GETDESKWALLPAPER
#define SPI_GETDESKWALLPAPER 0x0073
#endif

#define LONG_PTR INT_PTR
#define ULONG_PTR UINT_PTR

static HWND (WINAPI *pGetAncestor)(HWND,UINT);

static HWND hwndMain, hwndMain2;

/* check the values returned by the various parent/owner functions on a given window */
static void check_parents( HWND hwnd, HWND ga_parent, HWND gwl_parent, HWND get_parent,
                           HWND gw_owner, HWND ga_root, HWND ga_root_owner )
{
    HWND res;

    if (pGetAncestor)
    {
        res = pGetAncestor( hwnd, GA_PARENT );
        ok( res == ga_parent, "Wrong result for GA_PARENT %p expected %p", res, ga_parent );
    }
    res = (HWND)GetWindowLongW( hwnd, GWL_HWNDPARENT );
    ok( res == gwl_parent, "Wrong result for GWL_HWNDPARENT %p expected %p", res, gwl_parent );
    res = GetParent( hwnd );
    ok( res == get_parent, "Wrong result for GetParent %p expected %p", res, get_parent );
    res = GetWindow( hwnd, GW_OWNER );
    ok( res == gw_owner, "Wrong result for GW_OWNER %p expected %p", res, gw_owner );
    if (pGetAncestor)
    {
        res = pGetAncestor( hwnd, GA_ROOT );
        ok( res == ga_root, "Wrong result for GA_ROOT %p expected %p", res, ga_root );
        res = pGetAncestor( hwnd, GA_ROOTOWNER );
        ok( res == ga_root_owner, "Wrong result for GA_ROOTOWNER %p expected %p", res, ga_root_owner );
    }
}


static HWND create_tool_window( LONG style, HWND parent )
{
    HWND ret = CreateWindowExA(0, "ToolWindowClass", "Tool window 1", style,
                               0, 0, 100, 100, parent, 0, 0, NULL );
    ok( ret != 0, "Creation failed" );
    return ret;
}

/* test parent and owner values for various combinations */
static void test_parent_owner(void)
{
    LONG style;
    HWND test, owner, ret;
    HWND desktop = GetDesktopWindow();
    HWND child = create_tool_window( WS_CHILD, hwndMain );

    trace( "main window %p main2 %p desktop %p child %p\n", hwndMain, hwndMain2, desktop, child );

    /* child without parent, should fail */
    test = CreateWindowExA(0, "ToolWindowClass", "Tool window 1",
                           WS_CHILD, 0, 0, 100, 100, 0, 0, 0, NULL );
    ok( !test, "WS_CHILD without parent created" );

    /* desktop window */
    check_parents( desktop, 0, 0, 0, 0, 0, 0 );
    style = GetWindowLongW( desktop, GWL_STYLE );
    ok( !SetWindowLongW( desktop, GWL_STYLE, WS_POPUP ), "Set GWL_STYLE on desktop succeeded" );
    ok( !SetWindowLongW( desktop, GWL_STYLE, 0 ), "Set GWL_STYLE on desktop succeeded" );
    ok( GetWindowLongW( desktop, GWL_STYLE ) == style, "Desktop style changed" );

    /* normal child window */
    test = create_tool_window( WS_CHILD, hwndMain );
    trace( "created child %p\n", test );
    check_parents( test, hwndMain, hwndMain, hwndMain, 0, hwndMain, hwndMain );
    SetWindowLongW( test, GWL_STYLE, 0 );
    check_parents( test, hwndMain, hwndMain, 0, 0, hwndMain, test );
    SetWindowLongW( test, GWL_STYLE, WS_POPUP );
    check_parents( test, hwndMain, hwndMain, 0, 0, hwndMain, test );
    SetWindowLongW( test, GWL_STYLE, WS_POPUP|WS_CHILD );
    check_parents( test, hwndMain, hwndMain, 0, 0, hwndMain, test );
    SetWindowLongW( test, GWL_STYLE, WS_CHILD );
    DestroyWindow( test );

    /* child of desktop */
    test = create_tool_window( WS_CHILD, desktop );
    trace( "created child of desktop %p\n", test );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );
    SetWindowLongW( test, GWL_STYLE, WS_POPUP );
    check_parents( test, desktop, 0, 0, 0, test, test );
    SetWindowLongW( test, GWL_STYLE, 0 );
    check_parents( test, desktop, 0, 0, 0, test, test );
    DestroyWindow( test );

    /* child of child */
    test = create_tool_window( WS_CHILD, child );
    trace( "created child of child %p\n", test );
    check_parents( test, child, child, child, 0, hwndMain, hwndMain );
    SetWindowLongW( test, GWL_STYLE, 0 );
    check_parents( test, child, child, 0, 0, hwndMain, test );
    SetWindowLongW( test, GWL_STYLE, WS_POPUP );
    check_parents( test, child, child, 0, 0, hwndMain, test );
    DestroyWindow( test );

    /* not owned top-level window */
    test = create_tool_window( 0, 0 );
    trace( "created top-level %p\n", test );
    check_parents( test, desktop, 0, 0, 0, test, test );
    SetWindowLongW( test, GWL_STYLE, WS_POPUP );
    check_parents( test, desktop, 0, 0, 0, test, test );
    SetWindowLongW( test, GWL_STYLE, WS_CHILD );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );
    DestroyWindow( test );

    /* owned top-level window */
    test = create_tool_window( 0, hwndMain );
    trace( "created owned top-level %p\n", test );
    check_parents( test, desktop, hwndMain, 0, hwndMain, test, test );
    SetWindowLongW( test, GWL_STYLE, WS_POPUP );
    check_parents( test, desktop, hwndMain, hwndMain, hwndMain, test, hwndMain );
    SetWindowLongW( test, GWL_STYLE, WS_CHILD );
    check_parents( test, desktop, hwndMain, desktop, hwndMain, test, desktop );
    DestroyWindow( test );

    /* not owned popup */
    test = create_tool_window( WS_POPUP, 0 );
    trace( "created popup %p\n", test );
    check_parents( test, desktop, 0, 0, 0, test, test );
    SetWindowLongW( test, GWL_STYLE, WS_CHILD );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );
    SetWindowLongW( test, GWL_STYLE, 0 );
    check_parents( test, desktop, 0, 0, 0, test, test );
    DestroyWindow( test );

    /* owned popup */
    test = create_tool_window( WS_POPUP, hwndMain );
    trace( "created owned popup %p\n", test );
    check_parents( test, desktop, hwndMain, hwndMain, hwndMain, test, hwndMain );
    SetWindowLongW( test, GWL_STYLE, WS_CHILD );
    check_parents( test, desktop, hwndMain, desktop, hwndMain, test, desktop );
    SetWindowLongW( test, GWL_STYLE, 0 );
    check_parents( test, desktop, hwndMain, 0, hwndMain, test, test );
    DestroyWindow( test );

    /* top-level window owned by child (same as owned by top-level) */
    test = create_tool_window( 0, child );
    trace( "created top-level owned by child %p\n", test );
    check_parents( test, desktop, hwndMain, 0, hwndMain, test, test );
    DestroyWindow( test );

    /* popup owned by desktop (same as not owned) */
    test = create_tool_window( WS_POPUP, desktop );
    trace( "created popup owned by desktop %p\n", test );
    check_parents( test, desktop, 0, 0, 0, test, test );
    DestroyWindow( test );

    /* popup owned by child (same as owned by top-level) */
    test = create_tool_window( WS_POPUP, child );
    trace( "created popup owned by child %p\n", test );
    check_parents( test, desktop, hwndMain, hwndMain, hwndMain, test, hwndMain );
    DestroyWindow( test );

    /* not owned popup with WS_CHILD (same as WS_POPUP only) */
    test = create_tool_window( WS_POPUP | WS_CHILD, 0 );
    trace( "created WS_CHILD popup %p\n", test );
    check_parents( test, desktop, 0, 0, 0, test, test );
    DestroyWindow( test );

    /* owned popup with WS_CHILD (same as WS_POPUP only) */
    test = create_tool_window( WS_POPUP | WS_CHILD, hwndMain );
    trace( "created owned WS_CHILD popup %p\n", test );
    check_parents( test, desktop, hwndMain, hwndMain, hwndMain, test, hwndMain );
    DestroyWindow( test );

    /******************** parent changes *************************/
    trace( "testing parent changes\n" );

    /* desktop window */
    check_parents( desktop, 0, 0, 0, 0, 0, 0 );
    ret = (HWND)SetWindowLongW( test, GWL_HWNDPARENT, (LONG_PTR)hwndMain2 );
    ok( !ret, "Set GWL_HWNDPARENT succeeded on desktop" );
    check_parents( desktop, 0, 0, 0, 0, 0, 0 );
    ok( !SetParent( desktop, hwndMain ), "SetParent succeeded on desktop" );
    check_parents( desktop, 0, 0, 0, 0, 0, 0 );

    /* normal child window */
    test = create_tool_window( WS_CHILD, hwndMain );
    trace( "created child %p\n", test );

    ret = (HWND)SetWindowLongW( test, GWL_HWNDPARENT, (LONG_PTR)hwndMain2 );
    ok( ret == hwndMain, "GWL_HWNDPARENT return value %p expected %p", ret, hwndMain );
    check_parents( test, hwndMain2, hwndMain2, hwndMain2, 0, hwndMain2, hwndMain2 );

    ret = (HWND)SetWindowLongW( test, GWL_HWNDPARENT, (LONG_PTR)child );
    ok( ret == hwndMain2, "GWL_HWNDPARENT return value %p expected %p", ret, hwndMain2 );
    check_parents( test, child, child, child, 0, hwndMain, hwndMain );

    ret = (HWND)SetWindowLongW( test, GWL_HWNDPARENT, (LONG_PTR)desktop );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p", ret, child );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );

    /* window is now child of desktop so GWL_HWNDPARENT changes owner from now on */
    ret = (HWND)SetWindowLongW( test, GWL_HWNDPARENT, (LONG_PTR)child );
    ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0", ret );
    check_parents( test, desktop, child, desktop, child, test, desktop );

    ret = (HWND)SetWindowLongW( test, GWL_HWNDPARENT, 0 );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p", ret, child );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );
    DestroyWindow( test );

    /* not owned top-level window */
    test = create_tool_window( 0, 0 );
    trace( "created top-level %p\n", test );

    ret = (HWND)SetWindowLongW( test, GWL_HWNDPARENT, (LONG_PTR)hwndMain2 );
    ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0", ret );
    check_parents( test, desktop, hwndMain2, 0, hwndMain2, test, test );

    ret = (HWND)SetWindowLongW( test, GWL_HWNDPARENT, (LONG_PTR)child );
    ok( ret == hwndMain2, "GWL_HWNDPARENT return value %p expected %p", ret, hwndMain2 );
    check_parents( test, desktop, child, 0, child, test, test );

    ret = (HWND)SetWindowLongW( test, GWL_HWNDPARENT, 0 );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p", ret, child );
    check_parents( test, desktop, 0, 0, 0, test, test );
    DestroyWindow( test );

    /* not owned popup */
    test = create_tool_window( WS_POPUP, 0 );
    trace( "created popup %p\n", test );

    ret = (HWND)SetWindowLongW( test, GWL_HWNDPARENT, (LONG_PTR)hwndMain2 );
    ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0", ret );
    check_parents( test, desktop, hwndMain2, hwndMain2, hwndMain2, test, hwndMain2 );

    ret = (HWND)SetWindowLongW( test, GWL_HWNDPARENT, (LONG_PTR)child );
    ok( ret == hwndMain2, "GWL_HWNDPARENT return value %p expected %p", ret, hwndMain2 );
    check_parents( test, desktop, child, child, child, test, hwndMain );

    ret = (HWND)SetWindowLongW( test, GWL_HWNDPARENT, 0 );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p", ret, child );
    check_parents( test, desktop, 0, 0, 0, test, test );
    DestroyWindow( test );

    /* normal child window */
    test = create_tool_window( WS_CHILD, hwndMain );
    trace( "created child %p\n", test );

    ret = SetParent( test, desktop );
    ok( ret == hwndMain, "SetParent return value %p expected %p", ret, hwndMain );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );

    ret = SetParent( test, child );
    ok( ret == desktop, "SetParent return value %p expected %p", ret, desktop );
    check_parents( test, child, child, child, 0, hwndMain, hwndMain );

    ret = SetParent( test, hwndMain2 );
    ok( ret == child, "SetParent return value %p expected %p", ret, child );
    check_parents( test, hwndMain2, hwndMain2, hwndMain2, 0, hwndMain2, hwndMain2 );
    DestroyWindow( test );

    /* not owned top-level window */
    test = create_tool_window( 0, 0 );
    trace( "created top-level %p\n", test );

    ret = SetParent( test, child );
    ok( ret == desktop, "SetParent return value %p expected %p", ret, desktop );
    check_parents( test, child, child, 0, 0, hwndMain, test );
    DestroyWindow( test );

    /* owned popup */
    test = create_tool_window( WS_POPUP, hwndMain2 );
    trace( "created owned popup %p\n", test );

    ret = SetParent( test, child );
    ok( ret == desktop, "SetParent return value %p expected %p", ret, desktop );
    check_parents( test, child, child, hwndMain2, hwndMain2, hwndMain, hwndMain2 );

    ret = (HWND)SetWindowLongW( test, GWL_HWNDPARENT, (ULONG_PTR)hwndMain );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p", ret, child );
    check_parents( test, hwndMain, hwndMain, hwndMain2, hwndMain2, hwndMain, hwndMain2 );
    DestroyWindow( test );

    /**************** test owner destruction *******************/

    /* owned child popup */
    owner = create_tool_window( 0, 0 );
    test = create_tool_window( WS_POPUP, owner );
    trace( "created owner %p and popup %p\n", owner, test );
    ret = SetParent( test, child );
    ok( ret == desktop, "SetParent return value %p expected %p", ret, desktop );
    check_parents( test, child, child, owner, owner, hwndMain, owner );
    /* window is now child of 'child' but owned by 'owner' */
    DestroyWindow( owner );
    ok( IsWindow(test), "Window %p destroyed by owner destruction", test );
    check_parents( test, child, child, owner, owner, hwndMain, owner );
    ok( !IsWindow(owner), "Owner %p not destroyed", owner );
    DestroyWindow(test);

    /* owned top-level popup */
    owner = create_tool_window( 0, 0 );
    test = create_tool_window( WS_POPUP, owner );
    trace( "created owner %p and popup %p\n", owner, test );
    check_parents( test, desktop, owner, owner, owner, test, owner );
    DestroyWindow( owner );
    ok( !IsWindow(test), "Window %p not destroyed by owner destruction", test );

    /* top-level popup owned by child */
    owner = create_tool_window( WS_CHILD, hwndMain2 );
    test = create_tool_window( WS_POPUP, 0 );
    trace( "created owner %p and popup %p\n", owner, test );
    ret = (HWND)SetWindowLongW( test, GWL_HWNDPARENT, (ULONG_PTR)owner );
    ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0", ret );
    check_parents( test, desktop, owner, owner, owner, test, hwndMain2 );
    DestroyWindow( owner );
    ok( IsWindow(test), "Window %p destroyed by owner destruction", test );
    ok( !IsWindow(owner), "Owner %p not destroyed", owner );
    check_parents( test, desktop, owner, owner, owner, test, owner );
    DestroyWindow(test);

    /* final cleanup */
    DestroyWindow(child);
}


static BOOL RegisterWindowClasses(void)
{
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = DefWindowProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, IDC_ARROWA);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "MainWindowClass";

    if(!RegisterClassA(&cls)) return FALSE;

    cls.style = 0;
    cls.lpfnWndProc = DefWindowProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, IDC_ARROWA);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "ToolWindowClass";

    if(!RegisterClassA(&cls)) return FALSE;

    return TRUE;
}


START_TEST(win)
{
    pGetAncestor = (void *)GetProcAddress( GetModuleHandleA("user32.dll"), "GetAncestor" );

    if (!RegisterWindowClasses()) assert(0);

    hwndMain = CreateWindowExA(/*WS_EX_TOOLWINDOW*/ 0, "MainWindowClass", "Main window",
                               WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                               WS_MAXIMIZEBOX | WS_POPUP,
                               100, 100, 200, 200,
                               0, 0, 0, NULL);
    hwndMain2 = CreateWindowExA(/*WS_EX_TOOLWINDOW*/ 0, "MainWindowClass", "Main window 2",
                                WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                                WS_MAXIMIZEBOX | WS_POPUP,
                                100, 100, 200, 200,
                                0, 0, 0, NULL);
    assert( hwndMain );
    assert( hwndMain2 );

    test_parent_owner();
}
