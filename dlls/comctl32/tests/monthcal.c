/*
 * comctl32 month calendar unit tests
 *
 * Copyright (C) 2006 Vitaliy Margolen
 * Copyright (C) 2007 Farshad Agah
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"

#include "commctrl.h"

#include "wine/test.h"
#include <assert.h>
#include <windows.h>

#define expect(expected, got) ok(expected == got, "Expected %d, got %d\n", expected, got);

static void test_monthcal(void)
{
    HWND hwnd;
    SYSTEMTIME st[2], st1[2];
    INITCOMMONCONTROLSEX ic = {sizeof(INITCOMMONCONTROLSEX), ICC_DATE_CLASSES};
    int res, month_range;

    InitCommonControlsEx(&ic);
    hwnd = CreateWindowA(MONTHCAL_CLASSA, "MonthCal", WS_POPUP | WS_VISIBLE, CW_USEDEFAULT,
                         0, 300, 300, 0, 0, NULL, NULL);
    ok(hwnd != NULL, "Failed to create MonthCal\n");
    GetSystemTime(&st[0]);
    st[1] = st[0];

    /* Invalid date/time */
    st[0].wYear  = 2000;
    /* Time should not matter */
    st[1].wHour = st[1].wMinute = st[1].wSecond = 70;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set MAX limit\n");
    ok(SendMessage(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == GDTR_MAX, "No limits should be set\n");
    ok(st1[0].wYear != 2000, "Lover limit changed\n");

    st[1].wMonth = 0;
    ok(!SendMessage(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st), "Should have failed to set limits\n");
    ok(SendMessage(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == GDTR_MAX, "No limits should be set\n");
    ok(st1[0].wYear != 2000, "Lover limit changed\n");
    ok(!SendMessage(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Should have failed to set MAX limit\n");
    ok(SendMessage(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == GDTR_MAX, "No limits should be set\n");
    ok(st1[0].wYear != 2000, "Lover limit changed\n");

    GetSystemTime(&st[0]);
    st[0].wDay = 20;
    st[0].wMonth = 5;
    st[1] = st[0];

    month_range = SendMessage(hwnd, MCM_GETMONTHRANGE, GMR_VISIBLE, (LPARAM)st1);
    st[1].wMonth--;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st), "Failed to set both min and max limits\n");
    res = SendMessage(hwnd, MCM_GETMONTHRANGE, GMR_VISIBLE, (LPARAM)st1);
    ok(res == month_range, "Invalid month range (%d)\n", res);
    ok(SendMessage(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == (GDTR_MIN|GDTR_MAX), "Limits should be set\n");

    st[1].wMonth += 2;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st), "Failed to set both min and max limits\n");
    res = SendMessage(hwnd, MCM_GETMONTHRANGE, GMR_VISIBLE, (LPARAM)st1);
    ok(res == month_range, "Invalid month range (%d)\n", res);

    st[1].wYear --;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st), "Failed to set both min and max limits\n");
    st[1].wYear += 1;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st), "Failed to set both min and max limits\n");

    st[1].wMonth -= 3;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set max limit\n");
    ok(SendMessage(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == GDTR_MAX, "Only MAX limit should be set\n");
    st[1].wMonth += 4;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set max limit\n");
    st[1].wYear -= 3;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set max limit\n");
    st[1].wYear += 4;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set max limit\n");
    ok(SendMessage(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == GDTR_MAX, "Only MAX limit should be set\n");

    DestroyWindow(hwnd);
}

static HWND create_monthcal_control(DWORD style)
{
    HWND hwnd;
    static const INITCOMMONCONTROLSEX ic = {sizeof(INITCOMMONCONTROLSEX), ICC_DATE_CLASSES};

    InitCommonControlsEx(&ic);

    hwnd = CreateWindowEx(0,
                    MONTHCAL_CLASS,
                    "",
                    style,
                    0, 300, 300, 0, NULL, NULL, NULL, NULL);

    assert(hwnd);
    return hwnd;
}

static void test_monthcal_color(HWND hwnd)
{
    int res, temp;

    /* Setter and Getters for color*/
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_BACKGROUND, 0);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_BACKGROUND, RGB(0,0,0));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_BACKGROUND, 0);
    expect(RGB(0,0,0), temp);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_BACKGROUND, RGB(255,255,255));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_BACKGROUND, 0);
    expect(RGB(255,255,255), temp);

    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_MONTHBK, 0);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_MONTHBK, RGB(0,0,0));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_MONTHBK, 0);
    expect(RGB(0,0,0), temp);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_MONTHBK, RGB(255,255,255));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_MONTHBK, 0);
    expect(RGB(255,255,255), temp);

    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TEXT, 0);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_TEXT, RGB(0,0,0));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TEXT, 0);
    expect(RGB(0,0,0), temp);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_TEXT, RGB(255,255,255));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TEXT, 0);
    expect(RGB(255,255,255), temp);

    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TITLEBK, 0);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_TITLEBK, RGB(0,0,0));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TITLEBK, 0);
    expect(RGB(0,0,0), temp);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_TITLEBK, RGB(255,255,255));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TITLEBK, 0);
    expect(RGB(255,255,255), temp);

    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TITLETEXT, 0);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_TITLETEXT, RGB(0,0,0));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TITLETEXT, 0);
    expect(RGB(0,0,0), temp);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_TITLETEXT, RGB(255,255,255));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TITLETEXT, 0);
    expect(RGB(255,255,255), temp);

    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TRAILINGTEXT, 0);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_TRAILINGTEXT, RGB(0,0,0));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TRAILINGTEXT, 0);
    expect(RGB(0,0,0), temp);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_TRAILINGTEXT, RGB(255,255,255));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TRAILINGTEXT, 0);
    expect(RGB(255,255,255), temp);
}

static void test_monthcal_currDate(HWND hwnd)
{
    SYSTEMTIME st_original, st_new, st_test;
    int res;

    /* Setter and Getters for current date selected */
    st_original.wYear = 2000;
    st_original.wMonth = 11;
    st_original.wDay = 28;
    st_original.wHour = 11;
    st_original.wMinute = 59;
    st_original.wSecond = 30;
    st_original.wMilliseconds = 0;
    st_original.wDayOfWeek = 0;

    st_new = st_test = st_original;

    /* Should not validate the time */
    res = SendMessage(hwnd, MCM_SETCURSEL, 0, (LPARAM)&st_test);
    expect(1,res);

    /* Overflow matters, check for wDay */
    st_test.wDay += 4;
    res = SendMessage(hwnd, MCM_SETCURSEL, 0, (LPARAM)&st_test);
    todo_wine {expect(0,res);}

    /* correct wDay before checking for wMonth */
    st_test.wDay -= 4;
    expect(st_original.wDay, st_test.wDay);

    /* Overflow matters, check for wMonth */
    st_test.wMonth += 4;
    res = SendMessage(hwnd, MCM_SETCURSEL, 0, (LPARAM)&st_test);
    todo_wine {expect(0,res);}

    /* checking if gets the information right, modify st_new */
    st_new.wYear += 4;
    st_new.wMonth += 4;
    st_new.wDay += 4;
    st_new.wHour += 4;
    st_new.wMinute += 4;
    st_new.wSecond += 4;

    res = SendMessage(hwnd, MCM_GETCURSEL, 0, (LPARAM)&st_new);
    expect(1, res);

    /* st_new change to st_origin, above settings with overflow */
    /* should not change the current settings */
    expect(st_original.wYear, st_new.wYear);
    todo_wine {expect(st_original.wMonth, st_new.wMonth);}
    expect(st_original.wDay, st_new.wDay);
    expect(st_original.wHour, st_new.wHour);
    expect(st_original.wMinute, st_new.wMinute);
    expect(st_original.wSecond, st_new.wSecond);

    /* lparam can not be NULL */
    res = SendMessage(hwnd, MCM_GETCURSEL, 0, (LPARAM) NULL);
    expect(0, res);
}

static void test_monthcal_firstDay(HWND hwnd)
{
    int res, fday, i, temp;
    TCHAR b[128];
    LCID lcid = LOCALE_USER_DEFAULT;

    /* Setter and Getters for first day of week */
    todo_wine {
        /* check for locale first day */
        if(GetLocaleInfo(lcid, LOCALE_IFIRSTDAYOFWEEK, b, 128)){
            fday = atoi(b);
            res = SendMessage(hwnd, MCM_GETFIRSTDAYOFWEEK, 0, 0);
            expect(fday, res);
            res = SendMessage(hwnd, MCM_SETFIRSTDAYOFWEEK, 0, (LPARAM) 0);
            expect(fday, res);
        }else{
            skip("Can not retrive first day of the week\n");
            SendMessage(hwnd, MCM_SETFIRSTDAYOFWEEK, 0, (LPARAM) 0);
        }

        /* check for days of the week*/
        for (i = 1, temp = 0x10000; i < 7; i++, temp++){
            res = SendMessage(hwnd, MCM_GETFIRSTDAYOFWEEK, 0, 0);
            expect(temp, res);
            res = SendMessage(hwnd, MCM_SETFIRSTDAYOFWEEK, 0, (LPARAM) i);
            expect(temp, res);
        }

        /* check for returning to the original first day */
        res = SendMessage(hwnd, MCM_GETFIRSTDAYOFWEEK, 0, 0);
        todo_wine {expect(temp, res);}
    }
}

static void test_monthcal_unicode(HWND hwnd)
{
    int res, temp;

    /* Setter and Getters for Unicode format */

    /* getting the current settings */
    temp = SendMessage(hwnd, MCM_GETUNICODEFORMAT, 0, 0);

    /* setting to 1, should return previous settings */
    res = SendMessage(hwnd, MCM_SETUNICODEFORMAT, 1, 0);
    expect(temp, res);

    /* current setting is 1, so, should return 1 */
    res = SendMessage(hwnd, MCM_GETUNICODEFORMAT, 0, 0);
    todo_wine {expect(1, res);}

    /* setting to 0, should return previous settings */
    res = SendMessage(hwnd, MCM_SETUNICODEFORMAT, 0, 0);
    todo_wine {expect(1, res);}

    /* current setting is 0, so, it should return 0 */
    res = SendMessage(hwnd, MCM_GETUNICODEFORMAT, 0, 0);
    expect(0, res);

    /* should return previous settings */
    res = SendMessage(hwnd, MCM_SETUNICODEFORMAT, 1, 0);
    expect(0, res);
}

static void test_monthcal_HitTest(HWND hwnd)
{
    MCHITTESTINFO mchit;
    int res;

    memset(&mchit, 0, sizeof(MCHITTESTINFO));

    /* Setters for HITTEST */

    /* (0, 0) is the top left of the control and should not be active */
    mchit.cbSize = sizeof(MCHITTESTINFO);
    mchit.pt.x = 0;
    mchit.pt.y = 0;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(0, mchit.pt.x);
    expect(0, mchit.pt.y);
    expect(mchit.uHit, res);
    todo_wine {expect(MCHT_NOWHERE, res);}

    /* (300, 300) is the bottom right of the control and should not be active */
    mchit.pt.x = 300;
    mchit.pt.y = 300;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(300, mchit.pt.x);
    expect(300, mchit.pt.y);
    expect(mchit.uHit, res);
    todo_wine {expect(MCHT_NOWHERE, res);}

    /* (500, 500) is completely out of the control and should not be active */
    mchit.pt.x = 500;
    mchit.pt.y = 500;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(500, mchit.pt.x);
    expect(500, mchit.pt.y);
    expect(mchit.uHit, res);
    todo_wine {expect(MCHT_NOWHERE, res);}

    /* (150, 200) is in active area */
    mchit.pt.x = 150;
    mchit.pt.y = 200;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(150, mchit.pt.x);
    expect(200, mchit.pt.y);
    expect(mchit.uHit, res);
}

static void test_monthcal_today(HWND hwnd)
{
    SYSTEMTIME st_test, st_new;
    int res;

    /* Setter and Getters for "today" information */

    /* check for overflow, should be ok */
    st_test.wDay = 38;
    st_test.wMonth = 38;

    st_new.wDay = 27;
    st_new.wMonth = 27;

    SendMessage(hwnd, MCM_SETTODAY, 0, (LPARAM)&st_test);

    res = SendMessage(hwnd, MCM_GETTODAY, 0, (LPARAM)&st_new);
    expect(1, res);

    /* st_test should not change */
    expect(38, st_test.wDay);
    expect(38, st_test.wMonth);

    /* st_new should change, overflow does not matter */
    expect(38, st_new.wDay);
    expect(38, st_new.wMonth);

    /* check for zero, should be ok*/
    st_test.wDay = 0;
    st_test.wMonth = 0;

    SendMessage(hwnd, MCM_SETTODAY, 0, (LPARAM)&st_test);

    res = SendMessage(hwnd, MCM_GETTODAY, 0, (LPARAM)&st_new);
    expect(1, res);

    /* st_test should not change */
    expect(0, st_test.wDay);
    expect(0, st_test.wMonth);

    /* st_new should change to zero*/
    expect(0, st_new.wDay);
    expect(0, st_new.wMonth);
}

static void test_monthcal_scroll(HWND hwnd)
{
    int res;

    /* Setter and Getters for scroll rate */
    res = SendMessage(hwnd, MCM_SETMONTHDELTA, 2, 0);
    expect(0, res);

    res = SendMessage(hwnd, MCM_SETMONTHDELTA, 3, 0);
    expect(2, res);
    res = SendMessage(hwnd, MCM_GETMONTHDELTA, 0, 0);
    expect(3, res);

    res = SendMessage(hwnd, MCM_SETMONTHDELTA, 12, 0);
    expect(3, res);
    res = SendMessage(hwnd, MCM_GETMONTHDELTA, 0, 0);
    expect(12, res);

    res = SendMessage(hwnd, MCM_SETMONTHDELTA, 15, 0);
    expect(12, res);
    res = SendMessage(hwnd, MCM_GETMONTHDELTA, 0, 0);
    expect(15, res);

    res = SendMessage(hwnd, MCM_SETMONTHDELTA, -5, 0);
    expect(15, res);
    res = SendMessage(hwnd, MCM_GETMONTHDELTA, 0, 0);
    expect(-5, res);
}

static void test_monthcal_MaxSelDay(HWND hwnd)
{
    int res;

    /* Setter and Getters for max selected days */
    res = SendMessage(hwnd, MCM_SETMAXSELCOUNT, 5, 0);
    expect(1, res);
    res = SendMessage(hwnd, MCM_GETMAXSELCOUNT, 0, 0);
    expect(5, res);

    res = SendMessage(hwnd, MCM_SETMAXSELCOUNT, 15, 0);
    expect(1, res);
    res = SendMessage(hwnd, MCM_GETMAXSELCOUNT, 0, 0);
    expect(15, res);

    res = SendMessage(hwnd, MCM_SETMAXSELCOUNT, -1, 0);
    todo_wine {expect(0, res);}
    res = SendMessage(hwnd, MCM_GETMAXSELCOUNT, 0, 0);
    todo_wine {expect(15, res);}
}


START_TEST(monthcal)
{
    HWND hwnd;
    test_monthcal();

    hwnd = create_monthcal_control(0);
    test_monthcal_color(hwnd);
    test_monthcal_currDate(hwnd);
    test_monthcal_firstDay(hwnd);
    test_monthcal_unicode(hwnd);
    test_monthcal_HitTest(hwnd);
    test_monthcal_today(hwnd);
    test_monthcal_scroll(hwnd);

    DestroyWindow(hwnd);
    hwnd = create_monthcal_control(MCS_MULTISELECT);

    test_monthcal_MaxSelDay(hwnd);

    DestroyWindow(hwnd);
}
