/* Unit tests for the track bar control.
 *
 * Copyright 2007 Keith Stevens
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
*/

#include <assert.h>
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#include "wine/test.h"
#define expect(expected, got) ok(got == expected, "Expected %d, got %d\n", expected, got)

static HWND create_trackbar(DWORD style)
{
    HWND hWndTrack;

    hWndTrack = CreateWindowEx(
      0, TRACKBAR_CLASS,"Trackbar Control", style,
      10, 10, 200, 200, NULL, NULL,NULL ,NULL);

    return hWndTrack;
}

static void test_trackbar_control(void)
{
    HWND hWndTrackbar;
    DWORD style = WS_VISIBLE | TBS_TOOLTIPS | TBS_ENABLESELRANGE | TBS_FIXEDLENGTH | TBS_AUTOTICKS;
    int r;
    HWND hWndLeftBuddy;
    HWND hWndRightBuddy;
    HWND hWndCurrentBuddy;
    HWND hWndTooltip;
    HWND rTest;
    DWORD *rPTics;

    hWndTrackbar = create_trackbar(style);

    ok(hWndTrackbar != NULL, "Expected non NULL value\n");

    if (hWndTrackbar == NULL){
      skip("trackbar control not present?\n");
      return;
    }

    /* TEST OF ALL SETTER and GETTER MESSAGES with required styles turned on*/

    /* test TBM_SETBUDDY */
    hWndLeftBuddy = (HWND) CreateWindowEx(0, STATUSCLASSNAME, NULL, 0,
        0,0,300,20, NULL, NULL, NULL, NULL);
    ok(hWndLeftBuddy != NULL, "Expected non NULL value\n");

    if (hWndLeftBuddy != NULL){
        hWndCurrentBuddy = (HWND) SendMessage(hWndTrackbar, TBM_GETBUDDY, TRUE, 0);
        rTest = (HWND) SendMessage(hWndTrackbar, TBM_SETBUDDY, FALSE, (LPARAM) hWndLeftBuddy);
        ok(rTest == hWndCurrentBuddy, "Expected hWndCurrentBuddy\n");
        rTest = (HWND) SendMessage(hWndTrackbar, TBM_SETBUDDY, FALSE, (LPARAM) hWndLeftBuddy);
        ok(rTest == hWndLeftBuddy, "Expected hWndLeftBuddy\n");
    } else
        skip ("left buddy control not present?\n");

    hWndRightBuddy = (HWND) CreateWindowEx(0, STATUSCLASSNAME, NULL, 0,
        0,0,300,20,NULL,NULL, NULL, NULL);

    ok(hWndRightBuddy != NULL, "expected non NULL value\n");

    if (hWndRightBuddy != NULL){
        hWndCurrentBuddy = (HWND) SendMessage(hWndTrackbar, TBM_GETBUDDY, TRUE, 0);
        rTest = (HWND) SendMessage(hWndTrackbar, TBM_SETBUDDY, TRUE, (LPARAM) hWndRightBuddy);
        ok(rTest == hWndCurrentBuddy, "Expected hWndCurrentBuddy\n");
        rTest = (HWND) SendMessage(hWndTrackbar, TBM_SETBUDDY, TRUE, (LPARAM) hWndRightBuddy);
        ok(rTest == hWndRightBuddy, "Expected hWndRightbuddy\n");
     } else
        skip("Right buddy control not present?\n");

    /* test TBM_GETBUDDY */
    if (hWndLeftBuddy != NULL){
        rTest = (HWND) SendMessage(hWndTrackbar, TBM_GETBUDDY, FALSE, 0);
        ok(rTest == hWndLeftBuddy, "Expected hWndLeftBuddy\n");
        DestroyWindow(hWndLeftBuddy);
    }
    if (hWndRightBuddy != NULL){
        rTest = (HWND) SendMessage(hWndTrackbar, TBM_GETBUDDY, TRUE,0);
        ok(rTest == hWndRightBuddy, "Expected hWndRightBuddy\n");
        DestroyWindow(hWndRightBuddy);
    }

    /* test TBM_SETLINESIZE */
    r = SendMessage(hWndTrackbar, TBM_SETLINESIZE, 0, 10);
    r = SendMessage(hWndTrackbar, TBM_SETLINESIZE, 0, 4);
    expect(10, r);

    /* test TBM_GETLINESIZE */
    r = SendMessage(hWndTrackbar, TBM_GETLINESIZE, 0,0);
    expect(4, r);

    /* test TBM_SETPAGESIZE */
    r = SendMessage(hWndTrackbar, TBM_SETPAGESIZE, 0, 10);
    expect(20, r);
    r = SendMessage(hWndTrackbar, TBM_SETPAGESIZE, 0, -1);
    expect(10, r);

    /* test TBM_GETPAGESIZE */
    todo_wine{
        r = SendMessage(hWndTrackbar, TBM_GETPAGESIZE, 0,0);
        expect(20, r);
    }

    /* test TBM_SETPOS */
    SendMessage(hWndTrackbar, TBM_SETPOS, TRUE, -1);
    r = SendMessage(hWndTrackbar, TBM_GETPOS, 0, 0);
    expect(0, r);
    SendMessage(hWndTrackbar, TBM_SETPOS, TRUE, 5);
    r = SendMessage(hWndTrackbar, TBM_GETPOS, 0,0);
    expect(5, r);
    SendMessage(hWndTrackbar, TBM_SETPOS, TRUE, 1000);
    r = SendMessage(hWndTrackbar, TBM_GETPOS, 0,0);
    expect(100, r);
    SendMessage(hWndTrackbar, TBM_SETPOS, FALSE, 20);
    r = SendMessage(hWndTrackbar, TBM_GETPOS, 0,0);
    expect(20, r);

    /* test TBM_GETPOS */
    r = SendMessage(hWndTrackbar, TBM_GETPOS, 0,0);
    expect(20, r);

    /* test TBM_SETRANGE */
    SendMessage(hWndTrackbar, TBM_SETRANGE, TRUE, MAKELONG(0, 10));
    r = SendMessage(hWndTrackbar, TBM_GETRANGEMAX, 0,0);
    expect(10, r);
    r = SendMessage(hWndTrackbar, TBM_GETRANGEMIN, 0,0);
    expect(0, r);
    SendMessage(hWndTrackbar, TBM_SETRANGE, TRUE, MAKELONG(-1, 1000));
    r = SendMessage(hWndTrackbar, TBM_GETRANGEMAX, 0,0);
    expect(1000, r);
    r = SendMessage(hWndTrackbar, TBM_GETRANGEMIN, 0,0);
    expect(-1, r);
    SendMessage(hWndTrackbar, TBM_SETRANGE, TRUE, MAKELONG(10, 0));
    r = SendMessage(hWndTrackbar, TBM_GETRANGEMAX, 0,0);
    expect(0, r);
    r = SendMessage(hWndTrackbar, TBM_GETRANGEMIN, 0,0);
    expect(10, r);
    SendMessage(hWndTrackbar, TBM_SETRANGE, FALSE, MAKELONG(0,10));
    r = SendMessage(hWndTrackbar, TBM_GETRANGEMAX, 0,0);
    expect(10, r);
    r = SendMessage(hWndTrackbar, TBM_GETRANGEMIN, 0,0);
    expect(0, r);

    /*test TBM_SETRANGEMAX */
    SendMessage(hWndTrackbar, TBM_SETRANGEMAX, TRUE, 10);
    r = SendMessage(hWndTrackbar, TBM_GETRANGEMAX, 0,0);
    expect(10, r);
    SendMessage(hWndTrackbar, TBM_SETRANGEMAX, TRUE, -1);
    r = SendMessage(hWndTrackbar, TBM_GETRANGEMAX, 0,0);
    expect(-1, r);
    SendMessage(hWndTrackbar, TBM_SETRANGEMAX, FALSE, 10);
    r = SendMessage(hWndTrackbar, TBM_GETRANGEMAX, 0,0);
    expect(10, r);

    /* testing TBM_SETRANGEMIN */
    SendMessage(hWndTrackbar, TBM_SETRANGEMIN, TRUE, 0);
    r = SendMessage(hWndTrackbar, TBM_GETRANGEMIN, 0,0);
    expect(0, r);
    SendMessage(hWndTrackbar, TBM_SETRANGEMIN, TRUE, 10);
    r = SendMessage(hWndTrackbar, TBM_GETRANGEMIN, 0,0);
    expect(10, r);
    SendMessage(hWndTrackbar, TBM_SETRANGEMIN, TRUE, -10);
    r = SendMessage(hWndTrackbar, TBM_GETRANGEMIN, 0,0);
    expect(-10, r);
    SendMessage(hWndTrackbar, TBM_SETRANGEMIN, FALSE, 5);
    r = SendMessage(hWndTrackbar, TBM_GETRANGEMIN, 0,0);
    expect(5, r);

    /* test TBM_GETRANGEMAX */
    r = SendMessage(hWndTrackbar, TBM_GETRANGEMAX, 0,0);
    expect(10, r);

    /* test TBM_GETRANGEMIN */
    r = SendMessage(hWndTrackbar, TBM_GETRANGEMIN, 0,0);
    expect(5, r);

    /* test TBM_SETSEL */
    SendMessage(hWndTrackbar, TBM_SETSEL, TRUE, MAKELONG(0,10));
    r = SendMessage(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(10, r);
    r = SendMessage(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(5, r);
    SendMessage(hWndTrackbar, TBM_SETSEL, TRUE, MAKELONG(5, 20));
    r = SendMessage(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(10, r);
    r = SendMessage(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(5, r);
    SendMessage(hWndTrackbar, TBM_SETSEL, FALSE, MAKELONG(5, 10));
    r = SendMessage(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(10, r);
    r = SendMessage(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(5, r);

    /* test TBM_SETSELEND */
    SendMessage(hWndTrackbar, TBM_SETSELEND, TRUE, 10);
    r = SendMessage(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(10, r);
    SendMessage(hWndTrackbar, TBM_SETSELEND, TRUE, 20);
    r = SendMessage(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(10, r);
    SendMessage(hWndTrackbar, TBM_SETSELEND, TRUE, 4);
    r = SendMessage(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(4, r);
    SendMessage(hWndTrackbar, TBM_SETSELEND, FALSE, 2);
    r = SendMessage(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(2, r);

    /* test TBM_GETSELEND */
    r = SendMessage(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(2, r);

    /* testing TBM_SETSELSTART */
    SendMessage(hWndTrackbar, TBM_SETSELSTART, TRUE, 5);
    r = SendMessage(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(5, r);
    SendMessage(hWndTrackbar, TBM_SETSELSTART, TRUE, 0);
    r = SendMessage(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(5, r);
    SendMessage(hWndTrackbar, TBM_SETSELSTART, TRUE, 20);
    r = SendMessage(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(20, r);
    SendMessage(hWndTrackbar, TBM_SETSELSTART, FALSE, 8);
    r = SendMessage(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(8, r);

    /* test TBM_GETSELSTART */
    r = SendMessage(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(8, r);

    /* testing TBM_SETTHUMBLENGTH */
    SendMessage(hWndTrackbar, TBM_SETTHUMBLENGTH, 15, 0);
    r = SendMessage(hWndTrackbar, TBM_GETTHUMBLENGTH, 0,0);
    expect(15, r);
    SendMessage(hWndTrackbar, TBM_SETTHUMBLENGTH, 20, 0);
    r = SendMessage(hWndTrackbar, TBM_GETTHUMBLENGTH, 0,0);
    expect(20, r);

    /* test TBM_GETTHUMBLENGTH */
    r = SendMessage(hWndTrackbar, TBM_GETTHUMBLENGTH, 0,0);
    expect(20, r);

    /* testing TBM_SETTIC */
    /* Set tics at 5 and 10 */
    /* 0 and 20 are out of range and should not be set */
    r = SendMessage(hWndTrackbar, TBM_SETTIC, 0, 0);
    ok(r == FALSE, "Expected FALSE, got %d\n", r);
    todo_wine{
        r = SendMessage(hWndTrackbar, TBM_SETTIC, 0, 5);
        ok(r == TRUE, "Expected TRUE, got %d\n", r);
        r = SendMessage(hWndTrackbar, TBM_SETTIC, 0, 10);
        ok(r == TRUE, "Expected TRUE, got %d\n", r);
    }
    r = SendMessage(hWndTrackbar, TBM_SETTIC, 0, 20);
    ok(r == FALSE, "Expected False, got %d\n", r);

    /* test TBM_SETTICFREQ */
    SendMessage(hWndTrackbar, TBM_SETRANGE, TRUE, MAKELONG(0, 10));
    todo_wine{
        SendMessage(hWndTrackbar, TBM_SETTICFREQ, 2, 0);
        r = SendMessage(hWndTrackbar, TBM_GETNUMTICS, 0,0);
        expect(6, r);
        SendMessage(hWndTrackbar, TBM_SETTICFREQ, 5, 0);
        r = SendMessage(hWndTrackbar, TBM_GETNUMTICS, 0,0);
        expect(3, r);
    }
    SendMessage(hWndTrackbar, TBM_SETTICFREQ, 15, 0);
    r = SendMessage(hWndTrackbar, TBM_GETNUMTICS, 0,0);
    expect(2, r);

    /* test TBM_GETNUMTICS */
    /* since TIC FREQ is 15, there should be only 2 tics now */
    r = SendMessage(hWndTrackbar, TBM_GETNUMTICS, 0,0);
    expect(2, r);

    /* test TBM_GETPTICS */
    todo_wine{
        rPTics = (DWORD *) SendMessage(hWndTrackbar, TBM_GETPTICS, 0,0);
        expect(1, rPTics[0]);
        expect(2, rPTics[1]);
        expect(3, rPTics[2]);
        expect(4, rPTics[3]);
    }

    /* test TBM_GETTIC */
    todo_wine{
        r = SendMessage(hWndTrackbar, TBM_GETTIC, 0,0);
        expect(1, r);
        r = SendMessage(hWndTrackbar, TBM_GETTIC, 4,0);
        expect(5, r);
    }
    r = SendMessage(hWndTrackbar, TBM_GETTIC, 11,0);
    expect(-1, r);

    /* test TBM_GETTICPIC */
    todo_wine{
        r = SendMessage(hWndTrackbar, TBM_GETTICPOS, 0, 0);
        expect(28, r);
        r = SendMessage(hWndTrackbar, TBM_GETTICPOS, 4, 0);
        expect(97, r);
    }

    /* testing TBM_SETTIPSIDE */
    todo_wine{
        r = SendMessage(hWndTrackbar, TBM_SETTIPSIDE, TBTS_TOP, 0);
        expect(0, r);
    }
    r = SendMessage(hWndTrackbar, TBM_SETTIPSIDE, TBTS_LEFT, 0);
    expect(0, r);
    r = SendMessage(hWndTrackbar, TBM_SETTIPSIDE, TBTS_BOTTOM, 0);
    expect(1, r);
    r = SendMessage(hWndTrackbar, TBM_SETTIPSIDE, TBTS_RIGHT, 0);
    expect(2, r);

    /* testing TBM_SETTOOLTIPS */
    hWndTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, 0,
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
      NULL, NULL, NULL, NULL);

    ok(hWndTooltip != NULL, "Expected non NULL value\n");
    if (hWndTooltip != NULL){
        SendMessage(hWndTrackbar, TBM_SETTOOLTIPS, (LPARAM) hWndTooltip, 0);
        rTest = (HWND) SendMessage(hWndTrackbar, TBM_GETTOOLTIPS, 0,0);
        ok(rTest == hWndTooltip, "Expected hWndToolTip, got\n");
        SendMessage(hWndTrackbar, TBM_SETTOOLTIPS, (LPARAM) NULL, 0);
        rTest = (HWND) SendMessage(hWndTrackbar, TBM_GETTOOLTIPS, 0,0);
        ok(rTest == NULL, "Expected NULL\n");
        SendMessage(hWndTrackbar, TBM_SETTOOLTIPS, (LPARAM) hWndTooltip, 5);
        rTest = (HWND) SendMessage(hWndTrackbar, TBM_GETTOOLTIPS, 0,0);
        ok(rTest == hWndTooltip, "Expected hWndTooltip, got\n");
    } else
        skip("tool tip control not present?\n");

    /* test TBM_GETTOOLTIPS */
    rTest = (HWND) SendMessage(hWndTrackbar, TBM_GETTOOLTIPS, 0,0);
    ok(rTest == hWndTooltip, "Expected hWndTooltip\n");

    /* testing TBM_SETUNICODEFORMAT */
    r = SendMessage(hWndTrackbar, TBM_SETUNICODEFORMAT, TRUE, 0);
    ok(r == FALSE, "Expected FALSE, got %d\n",r);
    r = SendMessage(hWndTrackbar, TBM_SETUNICODEFORMAT, FALSE, 0);
    ok(r == TRUE, "Expected TRUE, got %d\n",r);

    /* test TBM_GETUNICODEFORMAT */
    r = SendMessage(hWndTrackbar, TBM_GETUNICODEFORMAT, 0,0);
    ok(r == FALSE, "Expected FALSE, got %d\n",r);

    DestroyWindow(hWndTrackbar);

    /* test getters and setters without styles set */
    hWndTrackbar = create_trackbar(0);

    ok(hWndTrackbar != NULL, "Expected non NULL value\n");

    if (hWndTrackbar == NULL){
      skip("trackbar control not present?\n");
      return;
    }

    /* test TBM_SETSEL  ensure that it is ignored */
    todo_wine{
        SendMessage(hWndTrackbar, TBM_SETSEL, TRUE, MAKELONG(0,10));
        r = SendMessage(hWndTrackbar, TBM_GETSELEND, 0,0);
        expect(0, r);
    }
    r = SendMessage(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(0, r);
    todo_wine{
        SendMessage(hWndTrackbar, TBM_SETSEL, FALSE, MAKELONG(0,10));
        r = SendMessage(hWndTrackbar, TBM_GETSELEND, 0,0);
        expect(0, r);
    }
    r = SendMessage(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(0, r);

    /* test TBM_SETSELEND, ensure that it is ignored */
    SendMessage(hWndTrackbar, TBM_SETSELEND, TRUE, 0);
    r = SendMessage(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(0, r);
    SendMessage(hWndTrackbar, TBM_SETSELEND, FALSE, 0);
    r = SendMessage(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(0, r);

    /* test TBM_SETSELSTART, ensure that it is ignored */
    SendMessage(hWndTrackbar, TBM_SETSELSTART, TRUE, 0);
    r = SendMessage(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(0, r);
    SendMessage(hWndTrackbar, TBM_SETSELSTART, FALSE, 0);
    r = SendMessage(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(0, r);

    DestroyWindow(hWndTrackbar);
}



START_TEST(trackbar)
{
    InitCommonControls();

    test_trackbar_control();
}
