/* Unit test suite for functions SystemParametersInfo and GetSystemMetrics.
 *
 * Copyright 2002 Andriy Palamarchuk
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

#include "wine/test.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"

#ifndef IDI_APPLICATIONA
# define IDI_APPLICATIONA IDI_APPLICATION
#endif
#ifndef IDC_ARROWA
# define IDC_ARROWA IDC_ARROW
#endif

#ifndef SPI_GETDESKWALLPAPER
# define SPI_GETDESKWALLPAPER 0x0073
#endif

#define eq(received, expected, label, type) \
        ok((received) == (expected), "%s: got " type " instead of " type, (label),(received),(expected))


/* FIXME: should fix the tests to not require this */
static int has_unicode(void) { return 1; }

#define SPI_SETBEEP_REGKEY           "Control Panel\\Sound"
#define SPI_SETBEEP_VALNAME          "Beep"
#define SPI_SETMOUSE_REGKEY             "Control Panel\\Mouse"
#define SPI_SETMOUSE_VALNAME1           "MouseThreshold1"
#define SPI_SETMOUSE_VALNAME2           "MouseThreshold2"
#define SPI_SETMOUSE_VALNAME3           "MouseSpeed"
#define SPI_SETBORDER_REGKEY         "Control Panel\\Desktop"
#define SPI_SETBORDER_VALNAME        "BorderWidth"
#define SPI_SETKEYBOARDSPEED_REGKEY             "Control Panel\\Keyboard"
#define SPI_SETKEYBOARDSPEED_VALNAME            "KeyboardSpeed"
#define SPI_ICONHORIZONTALSPACING_REGKEY        "Control Panel\\Desktop"
#define SPI_ICONHORIZONTALSPACING_VALNAME       "IconSpacing"
#define SPI_SETSCREENSAVETIMEOUT_REGKEY         "Control Panel\\Desktop"
#define SPI_SETSCREENSAVETIMEOUT_VALNAME        "ScreenSaveTimeOut"
#define SPI_SETSCREENSAVEACTIVE_REGKEY          "Control Panel\\Desktop"
#define SPI_SETSCREENSAVEACTIVE_VALNAME         "ScreenSaveActive"
#define SPI_SETGRIDGRANULARITY_REGKEY           "Control Panel\\Desktop"
#define SPI_SETGRIDGRANULARITY_VALNAME          "GridGranularity"
#define SPI_SETKEYBOARDDELAY_REGKEY             "Control Panel\\Keyboard"
#define SPI_SETKEYBOARDDELAY_VALNAME            "KeyboardDelay"
#define SPI_ICONVERTICALSPACING_REGKEY          "Control Panel\\Desktop"
#define SPI_ICONVERTICALSPACING_VALNAME         "IconVerticalSpacing"
#define SPI_SETICONTITLEWRAP_REGKEY             "Control Panel\\Desktop"
#define SPI_SETICONTITLEWRAP_VALNAME            "IconTitleWrap"
#define SPI_SETMENUDROPALIGNMENT_REGKEY         "Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows"
#define SPI_SETMENUDROPALIGNMENT_VALNAME        "MenuDropAlignment"
#define SPI_SETDOUBLECLKWIDTH_REGKEY            "Control Panel\\Mouse"
#define SPI_SETDOUBLECLKWIDTH_VALNAME           "DoubleClickWidth"
#define SPI_SETDOUBLECLKHEIGHT_REGKEY           "Control Panel\\Mouse"
#define SPI_SETDOUBLECLKHEIGHT_VALNAME          "DoubleClickHeight"
#define SPI_SETDOUBLECLICKTIME_REGKEY           "Control Panel\\Mouse"
#define SPI_SETDOUBLECLICKTIME_VALNAME          "DoubleClickSpeed"
#define SPI_SETMOUSEBUTTONSWAP_REGKEY           "Control Panel\\Mouse"
#define SPI_SETMOUSEBUTTONSWAP_VALNAME          "SwapMouseButtons"
#define SPI_SETWORKAREA_REGKEY                  "Control Panel\\Desktop"
#define SPI_SETWORKAREA_VALNAME                 "WINE_WorkArea"
#define SPI_SETSHOWSOUNDS_REGKEY     "Control Panel\\Accessibility\\ShowSounds"
#define SPI_SETSHOWSOUNDS_VALNAME    "On"
#define SPI_SETFASTTASKSWITCH_REGKEY            "Control Panel\\Desktop"
#define SPI_SETFASTTASKSWITCH_VALNAME           "CoolSwitch"
#define SPI_SETDRAGFULLWINDOWS_REGKEY           "Control Panel\\Desktop"
#define SPI_SETDRAGFULLWINDOWS_VALNAME          "DragFullWindows"
#define SPI_SETDESKWALLPAPER_REGKEY		"Control Panel\\Desktop"
#define SPI_SETDESKWALLPAPER_VALNAME		"Wallpaper"
/* FIXME - don't have access to Windows with this action (W95, NT5.0). Set real values */
#define SPI_SETKEYBOARDPREF_REGKEY      "Control Panel\\Desktop"
#define SPI_SETKEYBOARDPREF_VALNAME     "WINE_WorkArea"
#define SPI_SETSCREENREADER_REGKEY      "Control Panel\\Desktop"
#define SPI_SETSCREENREADER_VALNAME     "???"

/* volatile registry branch under CURRENT_USER_REGKEY for temporary values storage */
#define WINE_CURRENT_USER_REGKEY     "Wine"

static HWND ghTestWnd;

static DWORD WINAPI SysParamsThreadFunc( LPVOID lpParam );
static LRESULT CALLBACK SysParamsTestWndProc( HWND hWnd, UINT msg, WPARAM wParam,
                                              LPARAM lParam );
static int change_counter;
static int change_last_param;
static char change_reg_section[MAX_PATH];

static LRESULT CALLBACK SysParamsTestWndProc( HWND hWnd, UINT msg, WPARAM wParam,
                                              LPARAM lParam )
{
    switch(msg) {

    case WM_SETTINGCHANGE:
	change_counter++;
	change_last_param = wParam;
        strncpy( change_reg_section, (LPSTR) lParam, sizeof( change_reg_section ));
        break;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

    default:
        return( DefWindowProcA( hWnd, msg, wParam, lParam ) );
    }

    return 0;
}

/*
Performs testing for system parameters messages
params:
 - system parameter id
 - supposed value of the registry key
*/
static void test_change_message( int action, char *reg_section )
{
    ok( 1 == change_counter,
        "Missed a message: change_counter=%d", change_counter );
    change_counter = 0;
    ok( action == change_last_param,
        "Wrong action %d != %d", action, change_last_param );
    change_last_param = 0;
    ok( !strcmp( reg_section, change_reg_section ),
        "Unexpected registry section %s != %s",
        reg_section, change_reg_section );
    strcpy( change_reg_section, "");
}

/*
 * Tests the HKEY_CURRENT_USER subkey value.
 * The value should contain string value.
 *
 * Params:
 * lpsSubKey - subkey name
 * lpsRegName - registry entry name
 * lpsTestValue - value to test
 */
static void _test_reg_key( LPSTR subKey, LPSTR valName, LPSTR testValue, char *file, int line )
{
    CHAR  value[MAX_PATH] = "";
    DWORD valueLen = MAX_PATH;
    DWORD type;
    HKEY hKey;

    RegOpenKeyA( HKEY_CURRENT_USER, subKey, &hKey );
    RegQueryValueExA( hKey, valName, NULL, &type, value, &valueLen );
    RegCloseKey( hKey );
    ok( !strcmp( testValue, value ),
        "Wrong value in registry: subKey=%s, valName=%s, testValue=%s, value=%s",
        subKey, valName, testValue, value );
}

#define test_reg_key( subKey, valName, testValue ) \
    _test_reg_key( subKey, valName, testValue, __FILE__, __LINE__ )

static void test_SPI_SETBEEP( void )                   /*      2 */
{
    BOOL old_b;
    BOOL b;
    BOOL curr_val;

    trace("testing SPI_{GET,SET}BEEP\n");
    SystemParametersInfoA( SPI_GETBEEP, 0, &old_b, 0 );

    curr_val = TRUE;
    SystemParametersInfoA( SPI_SETBEEP, curr_val, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETBEEP, "" );
    test_reg_key( SPI_SETBEEP_REGKEY,
                  SPI_SETBEEP_VALNAME,
                  curr_val ? "Yes" : "No" );
    SystemParametersInfoA( SPI_GETBEEP, 0, &b, 0 );
    eq( b, curr_val, "SPI_{GET,SET}BEEP", "%d" );
    if (has_unicode())
    {
        SystemParametersInfoW( SPI_GETBEEP, 0, &b, 0 );
        eq( b, curr_val, "SystemParametersInfoW", "%d" );
    }

    /* is a message sent for the second change? */
    SystemParametersInfoA( SPI_SETBEEP, curr_val, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETBEEP, "" );

    curr_val = FALSE;
    if (has_unicode())
        SystemParametersInfoW( SPI_SETBEEP, curr_val, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    else
        SystemParametersInfoA( SPI_SETBEEP, curr_val, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETBEEP, "" );
    test_reg_key( SPI_SETBEEP_REGKEY,
                  SPI_SETBEEP_VALNAME,
                  curr_val ? "Yes" : "No" );
    SystemParametersInfoA( SPI_GETBEEP, 0, &b, 0 );
    eq( b, curr_val, "SPI_{GET,SET}BEEP", "%d" );
    if (has_unicode())
    {
        SystemParametersInfoW( SPI_GETBEEP, 0, &b, 0 );
        eq( b, curr_val, "SystemParametersInfoW", "%d" );
    }
    ok( MessageBeep( MB_OK ), "Return value of MessageBeep when sound is disabled" );

    SystemParametersInfoA( SPI_SETBEEP, old_b, 0, SPIF_UPDATEINIFILE );
}

static char *setmouse_valuenames[3] = {
    SPI_SETMOUSE_VALNAME1,
    SPI_SETMOUSE_VALNAME2,
    SPI_SETMOUSE_VALNAME3
};

/*
 * Runs check for one setting of spi_setmouse.
 */
static void run_spi_setmouse_test( int curr_val[], POINT *req_change, POINT *proj_change,
                                   int nchange )
{
    INT mi[3];
    static int aw_turn = 0;

    char buf[20];
    int i;

    aw_turn++;
    if (has_unicode() && (aw_turn % 2))        /* call unicode version each second call */
        SystemParametersInfoW( SPI_SETMOUSE, 0, curr_val, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    else
        SystemParametersInfoA( SPI_SETMOUSE, 0, curr_val, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETMOUSE, "" );
    for (i = 0; i < 3; i++)
    {
        sprintf( buf, "%d", curr_val[i] );
        test_reg_key( SPI_SETMOUSE_REGKEY, setmouse_valuenames[i], buf );
    }

    SystemParametersInfoA( SPI_GETMOUSE, 0, mi, 0 );
    for (i = 0; i < 3; i++)
    {
        ok(mi[i] == curr_val[i],
           "incorrect value for %d: %d != %d", i, mi[i], curr_val[i]);
    }

    if (has_unicode())
    {
        SystemParametersInfoW( SPI_GETMOUSE, 0, mi, 0 );
        for (i = 0; i < 3; i++)
        {
            ok(mi[i] == curr_val[i],
               "incorrect value for %d: %d != %d", i, mi[i], curr_val[i]);
        }
    }

#if 0  /* FIXME: this always fails for me  - AJ */
    for (i = 0; i < nchange; i++)
    {
        POINT mv;
        mouse_event( MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, 0, 0, 0, 0 );
        mouse_event( MOUSEEVENTF_MOVE, req_change[i].x, req_change[i].y, 0, 0 );
        GetCursorPos( &mv );
        ok( proj_change[i].x == mv.x, "Projected dx and real dx comparison. May fail under high load." );
        ok( proj_change[i].y == mv.y, "Projected dy equals real dy. May fail under high load." );
    }
#endif
}

static void test_SPI_SETMOUSE( void )                  /*      4 */
{
    INT old_mi[3];

    /* win nt default values - 6, 10, 1*/
    INT curr_val[3] = {6, 10, 1};

    /*requested and projected mouse movements */
    POINT req_change[] =   { {6, 6}, { 7, 6}, { 8, 6}, {10, 10}, {11, 10}, {100, 100} };
    POINT proj_change1[] = { {6, 6}, {14, 6}, {16, 6}, {20, 20}, {22, 20}, {200, 200} };
    POINT proj_change2[] = { {6, 6}, {14, 6}, {16, 6}, {20, 20}, {44, 20}, {400, 400} };
    POINT proj_change3[] = { {6, 6}, {14, 6}, {16, 6}, {20, 20}, {22, 20}, {200, 200} };
    POINT proj_change4[] = { {6, 6}, { 7, 6}, { 8, 6}, {10, 10}, {11, 10}, {100, 100} };
    POINT proj_change5[] = { {6, 6}, { 7, 6}, {16, 6}, {20, 20}, {22, 20}, {200, 200} };
    POINT proj_change6[] = { {6, 6}, {28, 6}, {32, 6}, {40, 40}, {44, 40}, {400, 400} };
    POINT proj_change7[] = { {6, 6}, {14, 6}, {32, 6}, {40, 40}, {44, 40}, {400, 400} };
    POINT proj_change8[] = { {6, 6}, {28, 6}, {32, 6}, {40, 40}, {44, 40}, {400, 400} };

    int nchange = sizeof( req_change ) / sizeof( POINT );

    trace("testing SPI_{GET,SET}MOUSE\n");
    SystemParametersInfoA( SPI_GETMOUSE, 0, old_mi, 0 );

    run_spi_setmouse_test( curr_val, req_change, proj_change1, nchange );

    /* acceleration change */
    curr_val[2] = 2;
    run_spi_setmouse_test( curr_val, req_change, proj_change2, nchange );

    /* acceleration change */
    curr_val[2] = 3;
    run_spi_setmouse_test( curr_val, req_change, proj_change3, nchange );

    /* acceleration change */
    curr_val[2] = 0;
    run_spi_setmouse_test( curr_val, req_change, proj_change4, nchange );

    /* threshold change */
    curr_val[2] = 1;
    curr_val[0] = 7;
    run_spi_setmouse_test( curr_val, req_change, proj_change5, nchange );

    /* threshold change */
    curr_val[2] = 2;
    curr_val[0] = 6;
    curr_val[1] = 6;
    run_spi_setmouse_test( curr_val, req_change, proj_change6, nchange );

    /* threshold change */
    curr_val[1] = 7;
    run_spi_setmouse_test( curr_val, req_change, proj_change7, nchange );

    /* threshold change */
    curr_val[1] = 5;
    run_spi_setmouse_test( curr_val, req_change, proj_change8, nchange );

    SystemParametersInfoA( SPI_SETMOUSE, 0, old_mi, SPIF_UPDATEINIFILE );
}

static void test_SPI_SETBORDER( void )                 /*      6 */
{
    UINT old_border;
    UINT curr_val;
    UINT border;
    INT frame;
    char buf[10];

    /* tests one configuration of border settings */
#define TEST_SETBORDER \
    SystemParametersInfoA( SPI_SETBORDER, curr_val, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE ); \
    test_change_message( SPI_SETBORDER, "" ); \
    sprintf( buf, "%d", curr_val ); \
    test_reg_key( SPI_SETBORDER_REGKEY, SPI_SETBORDER_VALNAME, buf ); \
    if (curr_val == 0) curr_val = 1; \
    SystemParametersInfoA( SPI_GETBORDER, 0, &border, 0 ); \
    eq( border, curr_val, "SPI_{GET,SET}BORDER", "%d"); \
    frame = curr_val + GetSystemMetrics( SM_CXDLGFRAME ); \
    eq( frame, GetSystemMetrics( SM_CXFRAME ), "SM_CXFRAME", "%d" ); \
    eq( frame, GetSystemMetrics( SM_CYFRAME ), "SM_CYFRAME", "%d" ); \
    eq( frame, GetSystemMetrics( SM_CXSIZEFRAME ), "SM_CXSIZEFRAME", "%d" ); \
    eq( frame, GetSystemMetrics( SM_CYSIZEFRAME ), "SM_CYSIZEFRAME", "%d" )

    /* These tests hang when XFree86 4.0 for Windows is running (tested on
     *  WinNT, SP2, Cygwin/XFree 4.1.0. Skip the test when XFree86 is
     * running.
     */
    if (!FindWindowA( NULL, "Cygwin/XFree86" ))
    {
        trace("testing SPI_{GET,SET}BORDER\n");
        SystemParametersInfoA( SPI_GETBORDER, 0, &old_border, 0 );

        curr_val = 1;
        TEST_SETBORDER;

        curr_val = 0;
        TEST_SETBORDER;

        curr_val = 7;
        TEST_SETBORDER;

        curr_val = 20;
        TEST_SETBORDER;

        /* This will restore sane values if the test hang previous run. */
        if ( old_border == 7 || old_border == 20 )
            old_border = 1;

        SystemParametersInfoA( SPI_SETBORDER, old_border, 0, SPIF_UPDATEINIFILE );
    }

#undef TEST_SETBORDER
}

static void test_SPI_SETKEYBOARDSPEED( void )          /*     10 */
{
    UINT old_speed;
    UINT curr_val;
    UINT speed;
    char buf[10];

    trace("testing SPI_{GET,SET}KEYBOARDSPEED\n");
    SystemParametersInfoA( SPI_GETKEYBOARDSPEED, 0, &old_speed, 0 );

    curr_val = 0;
    SystemParametersInfoA( SPI_SETKEYBOARDSPEED, curr_val, 0,
                           SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETKEYBOARDSPEED, "" );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_SETKEYBOARDSPEED_REGKEY, SPI_SETKEYBOARDSPEED_VALNAME, buf );

    SystemParametersInfoA( SPI_GETKEYBOARDSPEED, 0, &speed, 0 );
    eq( speed, curr_val, "SPI_{GET,SET}KEYBOARDSPEED", "%d" );

    curr_val = 32;
    SystemParametersInfoA( SPI_SETKEYBOARDSPEED, curr_val, 0,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    curr_val = 31; /* max value */
    test_change_message( SPI_SETKEYBOARDSPEED, "" );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_SETKEYBOARDSPEED_REGKEY, SPI_SETKEYBOARDSPEED_VALNAME, buf );

    SystemParametersInfoA( SPI_GETKEYBOARDSPEED, 0, &speed, 0 );
    eq( speed, curr_val, "SPI_{GET,SET}KEYBOARDSPEED", "%d");

    SystemParametersInfoA( SPI_SETKEYBOARDSPEED, old_speed, 0, SPIF_UPDATEINIFILE );
}

static void test_SPI_ICONHORIZONTALSPACING( void )     /*     13 */
{
    INT old_spacing;
    INT spacing;
    INT curr_val;
    char buf[10];

    trace("testing SPI_ICONHORIZONTALSPACING\n");
    /* default value: 75 */
    SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, 0, &old_spacing, 0 );

    curr_val = 101;
    SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, curr_val, 0,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    test_change_message( SPI_ICONHORIZONTALSPACING, "" );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_ICONHORIZONTALSPACING_REGKEY,
                  SPI_ICONHORIZONTALSPACING_VALNAME, buf );

    SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, 0, &spacing, 0 );
    eq( spacing, curr_val, "ICONHORIZONTALSPACING", "%d");
    eq( GetSystemMetrics( SM_CXICONSPACING ), curr_val, "SM_CXICONSPACING", "%d" );

    curr_val = 10;
    SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, curr_val, 0,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    curr_val = 32;      /*min value*/
    test_change_message( SPI_ICONHORIZONTALSPACING, "" );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_ICONHORIZONTALSPACING_REGKEY,
                  SPI_ICONHORIZONTALSPACING_VALNAME, buf );

    SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, 0, &spacing, 0 );
    eq( spacing, curr_val, "ICONHORIZONTALSPACING", "%d" );
    eq( GetSystemMetrics( SM_CXICONSPACING ), curr_val, "SM_CXICONSPACING", "%d" );

    SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, old_spacing, 0,
                          SPIF_UPDATEINIFILE );
}

static void test_SPI_SETSCREENSAVETIMEOUT( void )      /*     14 */
{
    UINT old_timeout;
    UINT timeout;
    UINT curr_val;
    char buf[10];

    trace("testing SPI_{GET,SET}SCREENSAVETIMEOUT\n");
    SystemParametersInfoA( SPI_GETSCREENSAVETIMEOUT, 0, &old_timeout, 0 );

    curr_val = 0;
    SystemParametersInfoA( SPI_SETSCREENSAVETIMEOUT, curr_val, 0,
                           SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETSCREENSAVETIMEOUT, "" );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_SETSCREENSAVETIMEOUT_REGKEY,
                  SPI_SETSCREENSAVETIMEOUT_VALNAME, buf );

    SystemParametersInfoA( SPI_GETSCREENSAVETIMEOUT, 0, &timeout, 0 );
    eq( timeout, curr_val, "SPI_{GET,SET}SCREENSAVETIMEOUT", "%d" );

    curr_val = 50000;
    SystemParametersInfoA( SPI_SETSCREENSAVETIMEOUT, curr_val, 0,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETSCREENSAVETIMEOUT, "" );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_SETSCREENSAVETIMEOUT_REGKEY,
                  SPI_SETSCREENSAVETIMEOUT_VALNAME, buf );

    SystemParametersInfoA( SPI_GETSCREENSAVETIMEOUT, 0, &timeout, 0 );
    eq( timeout, curr_val, "SPI_{GET,SET}SCREENSAVETIMEOUT", "%d" );

    SystemParametersInfoA( SPI_SETSCREENSAVETIMEOUT, old_timeout, 0,
                          SPIF_UPDATEINIFILE );
}

static void test_SPI_SETSCREENSAVEACTIVE( void )       /*     17 */
{
    BOOL old_b;
    BOOL b;
    BOOL curr_val;

    trace("testing SPI_{GET,SET}SCREENSAVEACTIVE\n");
    SystemParametersInfoA( SPI_GETSCREENSAVEACTIVE, 0, &old_b, 0 );

    curr_val = TRUE;
    SystemParametersInfoA( SPI_SETSCREENSAVEACTIVE, curr_val, 0,
                           SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETSCREENSAVEACTIVE, "" );
    test_reg_key( SPI_SETSCREENSAVEACTIVE_REGKEY,
                  SPI_SETSCREENSAVEACTIVE_VALNAME,
                  curr_val ? "1" : "0" );

    SystemParametersInfoA( SPI_GETSCREENSAVEACTIVE, 0, &b, 0 );
    eq( b, curr_val, "SPI_{GET,SET}SCREENSAVEACTIVE", "%d" );

    curr_val = FALSE;
    SystemParametersInfoA( SPI_SETSCREENSAVEACTIVE, curr_val, 0,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETSCREENSAVEACTIVE, "" );
    test_reg_key( SPI_SETSCREENSAVEACTIVE_REGKEY,
                  SPI_SETSCREENSAVEACTIVE_VALNAME,
                  curr_val ? "1" : "0" );

    SystemParametersInfoA( SPI_GETSCREENSAVEACTIVE, 0, &b, 0 );
    eq( b, curr_val, "SPI_{GET,SET}SCREENSAVEACTIVE", "%d" );

    SystemParametersInfoA( SPI_SETSCREENSAVEACTIVE, old_b, 0, SPIF_UPDATEINIFILE );
}

static void test_SPI_SETGRIDGRANULARITY( void )        /*     19 */
{
    /* ??? */;
}

static void test_SPI_SETKEYBOARDDELAY( void )          /*     23 */
{
    UINT old_delay;
    UINT delay;
    UINT curr_val;
    char buf[10];

    trace("testing SPI_{GET,SET}KEYBOARDDELAY\n");
    SystemParametersInfoA( SPI_GETKEYBOARDDELAY, 0, &old_delay, 0 );

    curr_val = 0;
    SystemParametersInfoA( SPI_SETKEYBOARDDELAY, curr_val, 0,
                           SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETKEYBOARDDELAY, "" );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_SETKEYBOARDDELAY_REGKEY,
                  SPI_SETKEYBOARDDELAY_VALNAME, buf );

    SystemParametersInfoA( SPI_GETKEYBOARDDELAY, 0, &delay, 0 );
    eq( delay, curr_val, "SPI_{GET,SET}KEYBOARDDELAY", "%d" );

    curr_val = 3;      /* max value */
    SystemParametersInfoA( SPI_SETKEYBOARDDELAY, curr_val, 0,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETKEYBOARDDELAY, "" );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_SETKEYBOARDDELAY_REGKEY,
                  SPI_SETKEYBOARDDELAY_VALNAME, buf );

    SystemParametersInfoA( SPI_GETKEYBOARDDELAY, 0, &delay, 0 );
    eq( delay, curr_val, "SPI_{GET,SET}KEYBOARDDELAY", "%d" );

    SystemParametersInfoA( SPI_SETKEYBOARDDELAY, old_delay, 0, SPIF_UPDATEINIFILE );
}

static void test_SPI_ICONVERTICALSPACING( void )       /*     24 */
{
    INT old_spacing;
    INT spacing;
    INT curr_val;
    char buf[10];

    trace("testing SPI_ICONVERTICALSPACING\n");
    /* default value: 75 */
    SystemParametersInfoA( SPI_ICONVERTICALSPACING, 0, &old_spacing, 0 );

    curr_val = 101;
    SystemParametersInfoA( SPI_ICONVERTICALSPACING, curr_val, 0,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    test_change_message( SPI_ICONVERTICALSPACING, "" );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_ICONVERTICALSPACING_REGKEY,
                  SPI_ICONVERTICALSPACING_VALNAME, buf );

    SystemParametersInfoA( SPI_ICONVERTICALSPACING, 0, &spacing, 0 );
    eq( spacing, curr_val, "ICONVERTICALSPACING", "%d" );
    eq( GetSystemMetrics( SM_CYICONSPACING ), curr_val, "SM_CYICONSPACING", "%d" );

    curr_val = 10;
    SystemParametersInfoA( SPI_ICONVERTICALSPACING, curr_val, 0,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    curr_val = 32;      /*min value*/
    test_change_message( SPI_ICONVERTICALSPACING, "" );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_ICONVERTICALSPACING_REGKEY,
                  SPI_ICONVERTICALSPACING_VALNAME, buf );

    SystemParametersInfoA( SPI_ICONVERTICALSPACING, 0, &spacing, 0 );
    eq( spacing, curr_val, "ICONVERTICALSPACING", "%d" );
    eq( GetSystemMetrics( SM_CYICONSPACING ), curr_val, "SM_CYICONSPACING", "%d" );

    SystemParametersInfoA( SPI_ICONVERTICALSPACING, old_spacing, 0,
                          SPIF_UPDATEINIFILE );
}

static void test_SPI_SETICONTITLEWRAP( void )          /*     26 */
{
    BOOL old_b;
    BOOL b;
    BOOL curr_val;

    /* These tests hang when XFree86 4.0 for Windows is running (tested on
     * WinNT, SP2, Cygwin/XFree 4.1.0. Skip the test when XFree86 is
     * running.
     */
    if (!FindWindowA( NULL, "Cygwin/XFree86" ))
    {
        trace("testing SPI_{GET,SET}ICONTITLEWRAP\n");
        SystemParametersInfoA( SPI_GETICONTITLEWRAP, 0, &old_b, 0 );

        curr_val = TRUE;
        SystemParametersInfoA( SPI_SETICONTITLEWRAP, curr_val, 0,
                              SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        test_change_message( SPI_SETICONTITLEWRAP, "" );
        test_reg_key( SPI_SETICONTITLEWRAP_REGKEY,
                      SPI_SETICONTITLEWRAP_VALNAME,
                      curr_val ? "1" : "0" );

        SystemParametersInfoA( SPI_GETICONTITLEWRAP, 0, &b, 0 );
        eq( b, curr_val, "SPI_{GET,SET}ICONTITLEWRAP", "%d" );

        curr_val = FALSE;
        SystemParametersInfoA( SPI_SETICONTITLEWRAP, curr_val, 0,
                              SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        test_change_message( SPI_SETICONTITLEWRAP, "" );
        test_reg_key( SPI_SETICONTITLEWRAP_REGKEY,
                      SPI_SETICONTITLEWRAP_VALNAME,
                      curr_val ? "1" : "0" );

        SystemParametersInfoA( SPI_GETICONTITLEWRAP, 0, &b, 0 );
        eq( b, curr_val, "SPI_{GET,SET}ICONTITLEWRAP", "%d" );

        SystemParametersInfoA( SPI_SETICONTITLEWRAP, old_b, 0, SPIF_UPDATEINIFILE );
    }
}

static void test_SPI_SETMENUDROPALIGNMENT( void )      /*     28 */
{
    BOOL old_b;
    BOOL b;
    BOOL curr_val;

    trace("testing SPI_{GET,SET}MENUDROPALIGNMENT\n");
    SystemParametersInfoA( SPI_GETMENUDROPALIGNMENT, 0, &old_b, 0 );

    curr_val = TRUE;
    SystemParametersInfoA( SPI_SETMENUDROPALIGNMENT, curr_val, 0,
                           SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETMENUDROPALIGNMENT, "" );
    test_reg_key( SPI_SETMENUDROPALIGNMENT_REGKEY,
                  SPI_SETMENUDROPALIGNMENT_VALNAME,
                  curr_val ? "1" : "0" );

    SystemParametersInfoA( SPI_GETMENUDROPALIGNMENT, 0, &b, 0 );
    eq( b, curr_val, "SPI_{GET,SET}MENUDROPALIGNMENT", "%d" );
    eq( GetSystemMetrics( SM_MENUDROPALIGNMENT ), curr_val, "SM_MENUDROPALIGNMENT", "%d" );

    curr_val = FALSE;
    SystemParametersInfoA( SPI_SETMENUDROPALIGNMENT, curr_val, 0,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETMENUDROPALIGNMENT, "" );
    test_reg_key( SPI_SETMENUDROPALIGNMENT_REGKEY,
                  SPI_SETMENUDROPALIGNMENT_VALNAME,
                  curr_val ? "1" : "0" );

    SystemParametersInfoA( SPI_GETMENUDROPALIGNMENT, 0, &b, 0 );
    eq( b, curr_val, "SPI_{GET,SET}MENUDROPALIGNMENT", "%d" );
    eq( GetSystemMetrics( SM_MENUDROPALIGNMENT ), curr_val, "SM_MENUDROPALIGNMENT", "%d" );

    SystemParametersInfoA( SPI_SETMENUDROPALIGNMENT, old_b, 0,
                           SPIF_UPDATEINIFILE );
}

static void test_SPI_SETDOUBLECLKWIDTH( void )         /*     29 */
{
    INT old_width;
    INT curr_val;
    char buf[10];

    trace("testing SPI_SETDOUBLECLKWIDTH\n");
    old_width = GetSystemMetrics( SM_CXDOUBLECLK );

    curr_val = 0;
    SystemParametersInfoA( SPI_SETDOUBLECLKWIDTH, curr_val, 0,
                           SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETDOUBLECLKWIDTH, "" );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_SETDOUBLECLKWIDTH_REGKEY,
                  SPI_SETDOUBLECLKWIDTH_VALNAME, buf );
    eq( GetSystemMetrics( SM_CXDOUBLECLK ), curr_val, "SM_CXDOUBLECLK", "%d" );

    curr_val = 10000;
    SystemParametersInfoA( SPI_SETDOUBLECLKWIDTH, curr_val, 0,
                           SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETDOUBLECLKWIDTH, "" );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_SETDOUBLECLKWIDTH_REGKEY,
                  SPI_SETDOUBLECLKWIDTH_VALNAME, buf );
    eq( GetSystemMetrics( SM_CXDOUBLECLK ), curr_val, "SM_CXDOUBLECLK", "%d" );

    SystemParametersInfoA( SPI_SETDOUBLECLKWIDTH, old_width, 0,
                          SPIF_UPDATEINIFILE );
}

static void test_SPI_SETDOUBLECLKHEIGHT( void )        /*     30 */
{
    INT old_height;
    INT curr_val;
    char buf[10];

    trace("testing SPI_SETDOUBLECLKHEIGHT\n");
    old_height = GetSystemMetrics( SM_CYDOUBLECLK );

    curr_val = 0;
    SystemParametersInfoA( SPI_SETDOUBLECLKHEIGHT, curr_val, 0,
                           SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETDOUBLECLKHEIGHT, "" );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_SETDOUBLECLKHEIGHT_REGKEY,
                  SPI_SETDOUBLECLKHEIGHT_VALNAME, buf );
    eq( GetSystemMetrics( SM_CYDOUBLECLK ), curr_val, "SM_CYDOUBLECLK", "%d" );

    curr_val = 10000;
    SystemParametersInfoA( SPI_SETDOUBLECLKHEIGHT, curr_val, 0,
                           SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETDOUBLECLKHEIGHT, "" );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_SETDOUBLECLKHEIGHT_REGKEY,
                  SPI_SETDOUBLECLKHEIGHT_VALNAME, buf );
    eq( GetSystemMetrics( SM_CYDOUBLECLK ), curr_val, "SM_CYDOUBLECLK", "%d" );

    SystemParametersInfoA( SPI_SETDOUBLECLKHEIGHT, old_height, 0,
                          SPIF_UPDATEINIFILE );
}

static void test_SPI_SETDOUBLECLICKTIME( void )        /*     32 */
{
    UINT curr_val;
    UINT saved_val;
    UINT old_time;
    char buf[10];

    trace("testing SPI_SETDOUBLECLICKTIME\n");
    old_time = GetDoubleClickTime();

    curr_val = 0;
    SystemParametersInfoA( SPI_SETDOUBLECLICKTIME, curr_val, 0,
                           SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETDOUBLECLICKTIME, "" );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_SETDOUBLECLICKTIME_REGKEY,
                  SPI_SETDOUBLECLICKTIME_VALNAME, buf );
    curr_val = 500; /* used value for 0 */
    eq( GetDoubleClickTime(), curr_val, "GetDoubleClickTime", "%d" );

    curr_val = 1000;
    SystemParametersInfoA( SPI_SETDOUBLECLICKTIME, curr_val, 0,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETDOUBLECLICKTIME, "" );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_SETDOUBLECLICKTIME_REGKEY,
                  SPI_SETDOUBLECLICKTIME_VALNAME, buf );
    eq( GetDoubleClickTime(), curr_val, "GetDoubleClickTime", "%d" );

    saved_val = curr_val;

    curr_val = 0;
    SetDoubleClickTime( curr_val );
    sprintf( buf, "%d", saved_val );
    test_reg_key( SPI_SETDOUBLECLICKTIME_REGKEY,
                  SPI_SETDOUBLECLICKTIME_VALNAME, buf );
    curr_val = 500; /* used value for 0 */
    eq( GetDoubleClickTime(), curr_val, "GetDoubleClickTime", "%d" );

    curr_val = 1000;
    SetDoubleClickTime( curr_val );
    sprintf( buf, "%d", saved_val );
    test_reg_key( SPI_SETDOUBLECLICKTIME_REGKEY,
                  SPI_SETDOUBLECLICKTIME_VALNAME, buf );
    eq( GetDoubleClickTime(), curr_val, "GetDoubleClickTime", "%d" );

    SystemParametersInfoA(SPI_SETDOUBLECLICKTIME, old_time, 0, SPIF_UPDATEINIFILE);
}

static void test_SPI_SETMOUSEBUTTONSWAP( void )        /*     33 */
{
    BOOL old_b;
    BOOL curr_val;

    trace("testing SPI_SETMOUSEBUTTONSWAP\n");
    old_b = GetSystemMetrics( SM_SWAPBUTTON );

    curr_val = TRUE;
    SystemParametersInfoA( SPI_SETMOUSEBUTTONSWAP, curr_val, 0,
                           SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETMOUSEBUTTONSWAP, "" );
    test_reg_key( SPI_SETMOUSEBUTTONSWAP_REGKEY,
                  SPI_SETMOUSEBUTTONSWAP_VALNAME,
                  curr_val ? "1" : "0" );
    eq( GetSystemMetrics( SM_SWAPBUTTON ), curr_val, "SM_SWAPBUTTON", "%d" );

    curr_val = FALSE;
    SystemParametersInfoA( SPI_SETMOUSEBUTTONSWAP, curr_val, 0,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETMOUSEBUTTONSWAP, "" );
    test_reg_key( SPI_SETMOUSEBUTTONSWAP_REGKEY,
                  SPI_SETMOUSEBUTTONSWAP_VALNAME,
                  curr_val ? "1" : "0" );
    eq( GetSystemMetrics( SM_SWAPBUTTON ), curr_val, "SM_SWAPBUTTON", "%d" );

    SystemParametersInfoA( SPI_SETMOUSEBUTTONSWAP, old_b, 0,
                           SPIF_UPDATEINIFILE );
}

static void test_SPI_SETFASTTASKSWITCH( void )         /*     36 */
{
    /* Temporarily commented out.
    BOOL old_b;
    BOOL b;
    BOOL res;
    BOOL curr_val;

    SystemParametersInfoA( SPI_GETFASTTASKSWITCH, 0, &old_b, 0 );

    curr_val = TRUE;
    res = SystemParametersInfoA( SPI_SETFASTTASKSWITCH, curr_val, 0,
                                 SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    ok( !res, "Must not set" );

    SystemParametersInfoA( SPI_GETFASTTASKSWITCH, 0, &b, 0 );
    ok( b, "retrieved value is the same as set" );

    curr_val = FALSE;
    res = SystemParametersInfoA( SPI_SETFASTTASKSWITCH, curr_val, 0,
                                 SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    ok( !res, "Must not set" );

    SystemParametersInfoA( SPI_GETFASTTASKSWITCH, 0, &b, 0 );

    ok( b, "retrieved value is the same as set" );

    SystemParametersInfoA( SPI_SETFASTTASKSWITCH, old_b, 0,
                           SPIF_UPDATEINIFILE );
    */
}

static void test_SPI_SETDRAGFULLWINDOWS( void )        /*     37 */
{
    BOOL old_b;
    BOOL b;
    BOOL curr_val;

    trace("testing SPI_{GET,SET}DRAGFULLWINDOWS\n");
    SystemParametersInfoA( SPI_GETDRAGFULLWINDOWS, 0, &old_b, 0 );

    curr_val = TRUE;
    SystemParametersInfoA( SPI_SETDRAGFULLWINDOWS, curr_val, 0,
                           SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETDRAGFULLWINDOWS, "" );
    test_reg_key( SPI_SETDRAGFULLWINDOWS_REGKEY,
                  SPI_SETDRAGFULLWINDOWS_VALNAME,
                  curr_val ? "1" : "0" );

    SystemParametersInfoA( SPI_GETDRAGFULLWINDOWS, 0, &b, 0 );
    eq( b, curr_val, "SPI_{GET,SET}DRAGFULLWINDOWS", "%d" );

    curr_val = FALSE;
    SystemParametersInfoA( SPI_SETDRAGFULLWINDOWS, curr_val, 0,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETDRAGFULLWINDOWS, "" );
    test_reg_key( SPI_SETDRAGFULLWINDOWS_REGKEY,
                  SPI_SETDRAGFULLWINDOWS_VALNAME,
                  curr_val ? "1" : "0" );

    SystemParametersInfoA( SPI_GETDRAGFULLWINDOWS, 0, &b, 0 );
    eq( b, curr_val, "SPI_{GET,SET}DRAGFULLWINDOWS", "%d" );

    SystemParametersInfoA( SPI_SETDRAGFULLWINDOWS, old_b, 0, SPIF_UPDATEINIFILE );
}

static void test_SPI_SETWORKAREA( void )               /*     47 */
{
    /* default - 0,0,1152, 864 */
    RECT old_area;
    RECT area;
    RECT curr_val;

    trace("testing SPI_{GET,SET}WORKAREA\n");
    SystemParametersInfoA(SPI_GETWORKAREA, 0, &old_area, 0);

    curr_val.left = 1;
    curr_val.top = 1;
    curr_val.right = 800;
    curr_val.bottom = 600;
    SystemParametersInfoA( SPI_SETWORKAREA, 0, &curr_val,
                           SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETWORKAREA, "" );
    SystemParametersInfoA( SPI_GETWORKAREA, 0, &area, 0 );
    eq( area.left,   curr_val.left,   "left",   "%ld" );
    eq( area.top,    curr_val.top,    "top",    "%ld" );
    eq( area.right,  curr_val.right,  "right",  "%ld" );
    eq( area.bottom, curr_val.bottom, "bottom", "%ld" );

    curr_val.left = 2;
    curr_val.top = 2;
    curr_val.right = 801;
    curr_val.bottom = 601;
    SystemParametersInfoA( SPI_SETWORKAREA, 0, &curr_val,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETWORKAREA, "" );
    SystemParametersInfoA( SPI_GETWORKAREA, 0, &area, 0 );
    eq( area.left,   curr_val.left,   "left",   "%ld" );
    eq( area.top,    curr_val.top,    "top",    "%ld" );
    eq( area.right,  curr_val.right,  "right",  "%ld" );
    eq( area.bottom, curr_val.bottom, "bottom", "%ld" );

    SystemParametersInfoA(SPI_SETWORKAREA, 0, &old_area, SPIF_UPDATEINIFILE);
}

static void test_SPI_SETSHOWSOUNDS( void )             /*     57 */
{
    BOOL old_b;
    BOOL b;
    BOOL curr_val;

    trace("testing SPI_{GET,SET}SHOWSOUNDS\n");
    SystemParametersInfoA( SPI_GETSHOWSOUNDS, 0, &old_b, 0 );

    curr_val = TRUE;
    SystemParametersInfoA( SPI_SETSHOWSOUNDS, curr_val, 0,
                           SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETSHOWSOUNDS, "" );
    test_reg_key( SPI_SETSHOWSOUNDS_REGKEY,
                  SPI_SETSHOWSOUNDS_VALNAME,
                  curr_val ? "1" : "0" );

    SystemParametersInfoA( SPI_GETSHOWSOUNDS, 0, &b, 0 );
    eq( b, curr_val, "SPI_GETSHOWSOUNDS", "%d" );
    eq( GetSystemMetrics( SM_SHOWSOUNDS ), curr_val, "SM_SHOWSOUNDS", "%d" );

    curr_val = FALSE;
    SystemParametersInfoA( SPI_SETSHOWSOUNDS, curr_val, 0,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    test_change_message( SPI_SETSHOWSOUNDS, "" );
    test_reg_key( SPI_SETSHOWSOUNDS_REGKEY,
                  SPI_SETSHOWSOUNDS_VALNAME,
                  curr_val ? "1" : "0" );

    SystemParametersInfoA( SPI_GETSHOWSOUNDS, 0, &b, 0 );
    eq( b, curr_val, "SPI_GETSHOWSOUNDS", "%d" );
    eq( GetSystemMetrics( SM_SHOWSOUNDS ), curr_val, "SM_SHOWSOUNDS", "%d" );

    SystemParametersInfoA( SPI_SETSHOWSOUNDS, old_b, 0, SPIF_UPDATEINIFILE );
}

static void test_SPI_SETKEYBOARDPREF( void )           /*     69 */
{
    /* TODO!!! - don't have version of Windows which has this */
}

static void test_SPI_SETSCREENREADER( void )           /*     71 */
{
    /* TODO!!! - don't have version of Windows which has this */
}

static void test_SPI_SETWALLPAPER( void )              /*   115 */
{
    char oldval[260];
    char newval[260];
    trace("testing SPI_{GET,SET}DESKWALLPAPER\n");

    SystemParametersInfoA(SPI_GETDESKWALLPAPER, 260, oldval, 0);
    test_reg_key(SPI_SETDESKWALLPAPER_REGKEY, SPI_SETDESKWALLPAPER_VALNAME, oldval);

    strcpy(newval, "");
    SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, newval, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    test_change_message(SPI_SETDESKWALLPAPER, "");

    SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, oldval, SPIF_UPDATEINIFILE);
}

/*
 * Registry entries for the system parameters.
 * Names are created by 'SET' flags names.
 * We assume that corresponding 'GET' entries use the same registry keys.
 */
static DWORD WINAPI SysParamsThreadFunc( LPVOID lpParam )
{
    test_SPI_SETBEEP();                         /*      1 */
    test_SPI_SETMOUSE();                        /*      4 */
    test_SPI_SETBORDER();                       /*      6 */
    test_SPI_SETKEYBOARDSPEED();                /*     10 */
    test_SPI_ICONHORIZONTALSPACING();           /*     13 */
    test_SPI_SETSCREENSAVETIMEOUT();            /*     14 */
    test_SPI_SETSCREENSAVEACTIVE();             /*     17 */
    test_SPI_SETGRIDGRANULARITY();              /*     19 */
    test_SPI_SETKEYBOARDDELAY();                /*     23 */
    test_SPI_ICONVERTICALSPACING();             /*     24 */
    test_SPI_SETICONTITLEWRAP();                /*     26 */
    test_SPI_SETMENUDROPALIGNMENT();            /*     28 */
    test_SPI_SETDOUBLECLKWIDTH();               /*     29 */
    test_SPI_SETDOUBLECLKHEIGHT();              /*     30 */
    test_SPI_SETDOUBLECLICKTIME();              /*     32 */
    test_SPI_SETMOUSEBUTTONSWAP();              /*     33 */
    test_SPI_SETFASTTASKSWITCH();               /*     36 */
    test_SPI_SETDRAGFULLWINDOWS();              /*     37 */
    test_SPI_SETWORKAREA();                     /*     47 */
    test_SPI_SETSHOWSOUNDS();                   /*     57 */
    test_SPI_SETKEYBOARDPREF();                 /*     69 */
    test_SPI_SETSCREENREADER();                 /*     71 */
    test_SPI_SETWALLPAPER();			/*    115 */
    SendMessageA( ghTestWnd, WM_DESTROY, 0, 0 );
    return 0;
}

START_TEST(sysparams)
{
    WNDCLASSA wc;
    MSG msg;
    HANDLE hThread;
    DWORD dwThreadId;
    HANDLE hInstance = GetModuleHandleA( NULL );

    change_counter = 0;
    change_last_param = 0;
    strcpy( change_reg_section, "" );

    wc.lpszClassName = "SysParamsTestClass";
    wc.lpfnWndProc = SysParamsTestWndProc;
    wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconA( 0, IDI_APPLICATIONA );
    wc.hCursor = LoadCursorA( 0, IDC_ARROWA );
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1 );
    wc.lpszMenuName = 0;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    RegisterClassA( &wc );

    ghTestWnd = CreateWindowA( "SysParamsTestClass", "Test System Parameters Application",
                               WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, 0, 0, hInstance, NULL );

    hThread = CreateThread( NULL, 0, SysParamsThreadFunc, 0, 0, &dwThreadId );
    assert( hThread );
    CloseHandle( hThread );

    while( GetMessageA( &msg, 0, 0, 0 )) {
        TranslateMessage( &msg );
        DispatchMessageA( &msg );
    }
}
