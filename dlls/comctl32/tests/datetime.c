/* Unit test suite for datetime control.
*
* Copyright 2007 Kanit Therdsteerasukdi
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

#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"

#define expect(EXPECTED, GOT) ok((GOT)==(EXPECTED), "Expected %d, got %ld\n", (EXPECTED), (GOT))

#define expect_unsuccess(EXPECTED, GOT) ok((GOT)==(EXPECTED), "Expected %d(unsuccessful), got %ld(successful)\n", (EXPECTED), (GOT))

static HWND create_datetime_control(DWORD style, DWORD exstyle)
{
    HWND hWndDateTime = NULL;

    hWndDateTime = CreateWindowEx(0,
        DATETIMEPICK_CLASS,
        NULL,
        style,
        0,50,300,120,
        NULL,
        NULL,
        NULL,
        NULL);

    return hWndDateTime;
}

static void test_dtm_set_format(HWND hWndDateTime)
{
    LRESULT r;

    todo_wine {
        r = SendMessage(hWndDateTime, DTM_SETFORMAT, 0, (LPARAM)NULL);
        expect(1, r);

        r = SendMessage(hWndDateTime, DTM_SETFORMAT, 0,
            (LPARAM)"'Today is: 'hh':'m':'s dddd MMM dd', 'yyyy");
        expect(1, r);
    }
}

void test_mccolor_types(HWND hWndDateTime, int mccolor_type, const char* mccolor_name)
{
    LRESULT r;
    COLORREF theColor, prevColor;

    theColor=RGB(0,0,0);
    r = SendMessage(hWndDateTime, DTM_SETMCCOLOR, mccolor_type, theColor);
    ok(r != -1, "%s: Set RGB(0,0,0): Expected COLORREF of previous value, got %ld\n", mccolor_name, r);
    prevColor=theColor;
    theColor=RGB(255,255,255);
    r = SendMessage(hWndDateTime, DTM_SETMCCOLOR, mccolor_type, theColor);
    ok(r==prevColor, "%s: Set RGB(255,255,255): Expected COLORREF of previous value, got %ld\n", mccolor_name, r);
    prevColor=theColor;
    theColor=RGB(100,180,220);
    r = SendMessage(hWndDateTime, DTM_SETMCCOLOR, mccolor_type, theColor);
    ok(r==prevColor, "%s: Set RGB(100,180,220): Expected COLORREF of previous value, got %ld\n", mccolor_name, r);
    r = SendMessage(hWndDateTime, DTM_GETMCCOLOR, mccolor_type, 0);
    ok(r==theColor, "%s: GETMCCOLOR: Expected %d, got %ld\n", mccolor_name, theColor, r);
}

static void test_dtm_set_and_get_mccolor(HWND hWndDateTime)
{
    test_mccolor_types(hWndDateTime, MCSC_BACKGROUND, "MCSC_BACKGROUND");
    test_mccolor_types(hWndDateTime, MCSC_MONTHBK, "MCSC_MONTHBK");
    test_mccolor_types(hWndDateTime, MCSC_TEXT, "MCSC_TEXT");
    test_mccolor_types(hWndDateTime, MCSC_TITLEBK, "MCSC_TITLEBK");
    test_mccolor_types(hWndDateTime, MCSC_TITLETEXT, "MCSC_TITLETEXT");
    test_mccolor_types(hWndDateTime, MCSC_TRAILINGTEXT, "MCSC_TRAILINGTEXT");
}

static void test_dtm_set_and_get_mcfont(HWND hWndDateTime)
{
    HFONT hFontOrig, hFontNew;

    hFontOrig = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    SendMessage(hWndDateTime, DTM_SETMCFONT, (WPARAM)hFontOrig, TRUE);
    hFontNew = (HFONT)SendMessage(hWndDateTime, DTM_GETMCFONT, 0, 0);
    ok(hFontOrig == hFontNew, "Expected hFontOrig==hFontNew, hFontOrig=%p, hFontNew=%p\n", hFontOrig, hFontNew);
}

static void test_dtm_get_monthcal(HWND hWndDateTime)
{
    LRESULT r;

    todo_wine {
        r = SendMessage(hWndDateTime, DTM_GETMONTHCAL, 0, 0);
        ok(r == (LPARAM)NULL, "Expected NULL(no child month calendar control), got %ld\n", r);
    }
}

void fill_systime_struct(SYSTEMTIME *st, int year, int month, int dayofweek, int day, int hour, int minute, int second, int milliseconds)
{
    st->wYear = year;
    st->wMonth = month;
    st->wDayOfWeek = dayofweek;
    st->wDay = day;
    st->wHour = hour;
    st->wMinute = minute;
    st->wSecond = second;
    st->wMilliseconds = milliseconds;
}

LPARAM compare_systime_date(SYSTEMTIME *st1, SYSTEMTIME *st2)
{
    return (st1->wYear == st2->wYear)
            && (st1->wMonth == st2->wMonth)
            && (st1->wDayOfWeek == st2->wDayOfWeek)
            && (st1->wDay == st2->wDay);
}

LPARAM compare_systime_time(SYSTEMTIME *st1, SYSTEMTIME *st2)
{
    return (st1->wHour == st2->wHour)
            && (st1->wMinute == st2->wMinute)
            && (st1->wSecond == st2->wSecond)
            && (st1->wMilliseconds == st2->wMilliseconds);
}

LPARAM compare_systime(SYSTEMTIME *st1, SYSTEMTIME *st2)
{
    if(!compare_systime_date(st1, st2))
        return 0;

    return compare_systime_time(st1, st2);
}

#define expect_systime(ST1, ST2) ok(compare_systime((ST1), (ST2))==1, "ST1 != ST2\n")
#define expect_systime_date(ST1, ST2) ok(compare_systime_date((ST1), (ST2))==1, "ST1.date != ST2.date\n")
#define expect_systime_time(ST1, ST2) ok(compare_systime_time((ST1), (ST2))==1, "ST1.time != ST2.time\n")

static void test_dtm_set_and_get_range(HWND hWndDateTime)
{
    LRESULT r;
    SYSTEMTIME st[2];
    SYSTEMTIME getSt[2];

    /* initialize st[0] to lowest possible value */
    fill_systime_struct(&st[0], 1601, 1, 0, 1, 0, 0, 0, 0);
    /* intialize st[1] to all invalid numbers */
    fill_systime_struct(&st[1], 0, 0, 7, 0, 24, 60, 60, 1000);

    r = SendMessage(hWndDateTime, DTM_SETRANGE, GDTR_MIN, (LPARAM)st);
    expect(1, r);
    r = SendMessage(hWndDateTime, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == GDTR_MIN, "Expected %x, not %x(GDTR_MAX) or %x(GDTR_MIN | GDTR_MAX), got %lx\n", GDTR_MIN, GDTR_MAX, GDTR_MIN | GDTR_MAX, r);
    expect_systime(&st[0], &getSt[0]);

    r = SendMessage(hWndDateTime, DTM_SETRANGE, GDTR_MAX, (LPARAM)st);
    expect_unsuccess(0, r);

    /* set st[0] to all invalid numbers */
    fill_systime_struct(&st[0], 0, 0, 7, 0, 24, 60, 60, 1000);
    /* set st[1] to highest possible value */
    fill_systime_struct(&st[1], 30827, 12, 6, 31, 23, 59, 59, 999);

    r = SendMessage(hWndDateTime, DTM_SETRANGE, GDTR_MAX, (LPARAM)st);
    expect(1, r);
    r = SendMessage(hWndDateTime, DTM_GETRANGE, 0, (LPARAM)getSt);
    todo_wine {
        ok(r == GDTR_MAX, "Expected %x, not %x(GDTR_MIN) or %x(GDTR_MIN | GDTR_MAX), got %lx\n", GDTR_MAX, GDTR_MIN, GDTR_MIN | GDTR_MAX, r);
    }
    expect_systime(&st[1], &getSt[1]);

    r = SendMessage(hWndDateTime, DTM_SETRANGE, GDTR_MIN, (LPARAM)st);
    expect_unsuccess(0, r);
    r = SendMessage(hWndDateTime, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    expect_unsuccess(0, r);

    /* set st[0] to highest possible value */
    fill_systime_struct(&st[0], 30827, 12, 6, 31, 23, 59, 59, 999);

    r = SendMessage(hWndDateTime, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    expect(1, r);
    r = SendMessage(hWndDateTime, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == (GDTR_MIN | GDTR_MAX), "Expected %x, not %x(GDTR_MIN) or %x(GDTR_MAX), got %lx\n", (GDTR_MIN | GDTR_MAX), GDTR_MIN, GDTR_MAX, r);
    expect_systime(&st[0], &getSt[0]);
    expect_systime(&st[1], &getSt[1]);

    /* initialize st[0] to lowest possible value */
    fill_systime_struct(&st[0], 1601, 1, 0, 1, 0, 0, 0, 0);
    /* set st[1] to highest possible value */
    fill_systime_struct(&st[1], 30827, 12, 6, 31, 23, 59, 59, 999);

    r = SendMessage(hWndDateTime, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    expect(1, r);
    r = SendMessage(hWndDateTime, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == (GDTR_MIN | GDTR_MAX), "Expected %x, not %x(GDTR_MIN) or %x(GDTR_MAX), got %lx\n", (GDTR_MIN | GDTR_MAX), GDTR_MIN, GDTR_MAX, r);
    expect_systime(&st[0], &getSt[0]);
    expect_systime(&st[1], &getSt[1]);

    /* set st[0] to value higher than minimum */
    fill_systime_struct(&st[0], 1980, 1, 3, 23, 14, 34, 37, 465);
    /* set st[1] to value lower than maximum */
    fill_systime_struct(&st[1], 2007, 3, 2, 31, 23, 59, 59, 999);

    r = SendMessage(hWndDateTime, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    expect(1, r);
    r = SendMessage(hWndDateTime, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == (GDTR_MIN | GDTR_MAX), "Expected %x, not %x(GDTR_MIN) or %x(GDTR_MAX), got %lx\n", (GDTR_MIN | GDTR_MAX), GDTR_MIN, GDTR_MAX, r);
    expect_systime(&st[0], &getSt[0]);
    expect_systime(&st[1], &getSt[1]);
}

/* when max<min for DTM_SETRANGE, Windows seems to swap the min and max values,
although that's undocumented.  However, it doesn't seem to be implemented
correctly, causing some strange side effects */
static void test_dtm_set_range_swap_min_max(HWND hWndDateTime)
{
    LRESULT r;
    SYSTEMTIME st[2];
    SYSTEMTIME getSt[2];
    SYSTEMTIME origSt;

    fill_systime_struct(&st[0], 2007, 2, 4, 15, 2, 2, 2, 2);

    r = SendMessage(hWndDateTime, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st[0]);
    expect(1, r);
    r = SendMessage(hWndDateTime, DTM_GETSYSTEMTIME, 0, (LPARAM)&origSt);
    ok(r == GDT_VALID, "Expected %d, not %d(GDT_NONE) or %d(GDT_ERROR), got %ld\n", GDT_VALID, GDT_NONE, GDT_ERROR, r);
    expect_systime(&st[0], &origSt);

    /* set st[0] to value higher than st[1] */
    fill_systime_struct(&st[0], 2007, 3, 2, 31, 23, 59, 59, 999);
    fill_systime_struct(&st[1], 1980, 1, 3, 23, 14, 34, 37, 465);

    /* since min>max, min and max values should be swapped by DTM_SETRANGE
    automatically */
    r = SendMessage(hWndDateTime, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    expect(1, r);
    r = SendMessage(hWndDateTime, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == (GDTR_MIN | GDTR_MAX), "Expected %x, not %x(GDTR_MIN) or %x(GDTR_MAX), got %lx\n", (GDTR_MIN | GDTR_MAX), GDTR_MIN, GDTR_MAX, r);
    todo_wine {
        expect_systime(&st[0], &getSt[0]);
    }
    todo_wine {
        expect_systime(&st[1], &getSt[1]);
    }

    fill_systime_struct(&st[0], 1980, 1, 3, 23, 14, 34, 37, 465);

    r = SendMessage(hWndDateTime, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st[0]);
    expect(1, r);
    r = SendMessage(hWndDateTime, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt[0]);
    ok(r == GDT_VALID, "Expected %d, not %d(GDT_NONE) or %d(GDT_ERROR), got %ld\n", GDT_VALID, GDT_NONE, GDT_ERROR, r);
    /* the time part seems to not change after swapping the min and max values
    and doing DTM_SETSYSTEMTIME */
    expect_systime_date(&st[0], &getSt[0]);
    todo_wine {
        expect_systime_time(&origSt, &getSt[0]);
    }

    /* set st[0] to value higher than minimum */
    fill_systime_struct(&st[0], 1980, 1, 3, 23, 14, 34, 37, 465);
    /* set st[1] to value lower than maximum */
    fill_systime_struct(&st[1], 2007, 3, 2, 31, 23, 59, 59, 999);

    r = SendMessage(hWndDateTime, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    expect(1, r);
    /* for some reason after we swapped the min and max values before,
    whenever we do a DTM_SETRANGE, the DTM_GETRANGE will return the values
    swapped*/
    r = SendMessage(hWndDateTime, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == (GDTR_MIN | GDTR_MAX), "Expected %x, not %x(GDTR_MIN) or %x(GDTR_MAX), got %lx\n", (GDTR_MIN | GDTR_MAX), GDTR_MIN, GDTR_MAX, r);
    todo_wine {
        expect_systime(&st[0], &getSt[1]);
    }
    todo_wine {
        expect_systime(&st[1], &getSt[0]);
    }

    /* set st[0] to value higher than st[1] */
    fill_systime_struct(&st[0], 2007, 3, 2, 31, 23, 59, 59, 999);
    fill_systime_struct(&st[1], 1980, 1, 3, 23, 14, 34, 37, 465);

    /* set min>max again, so that the return values of DTM_GETRANGE are no
    longer swapped the next time we do a DTM SETRANGE and DTM_GETRANGE*/
    r = SendMessage(hWndDateTime, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    expect(1, r);
    r = SendMessage(hWndDateTime, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == (GDTR_MIN | GDTR_MAX), "Expected %x, not %x(GDTR_MIN) or %x(GDTR_MAX), got %lx\n", (GDTR_MIN | GDTR_MAX), GDTR_MIN, GDTR_MAX, r);
    expect_systime(&st[0], &getSt[1]);
    expect_systime(&st[1], &getSt[0]);

    /* initialize st[0] to lowest possible value */
    fill_systime_struct(&st[0], 1601, 1, 0, 1, 0, 0, 0, 0);
    /* set st[1] to highest possible value */
    fill_systime_struct(&st[1], 30827, 12, 6, 31, 23, 59, 59, 999);

    r = SendMessage(hWndDateTime, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    expect(1, r);
    r = SendMessage(hWndDateTime, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == (GDTR_MIN | GDTR_MAX), "Expected %x, not %x(GDTR_MIN) or %x(GDTR_MAX), got %lx\n", (GDTR_MIN | GDTR_MAX), GDTR_MIN, GDTR_MAX, r);
    expect_systime(&st[0], &getSt[0]);
    expect_systime(&st[1], &getSt[1]);
}

static void test_dtm_set_and_get_system_time(HWND hWndDateTime)
{
    LRESULT r;
    SYSTEMTIME st;
    SYSTEMTIME getSt;

    r = SendMessage(hWndDateTime, DTM_SETSYSTEMTIME, GDT_NONE, (LPARAM)&st);
    expect(1, r);
    r = SendMessage(hWndDateTime, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt);
    ok(r == GDT_NONE, "Expected %d, not %d(GDT_VALID) or %d(GDT_ERROR), got %ld\n", GDT_NONE, GDT_VALID, GDT_ERROR, r);

    /* set st to lowest possible value */
    fill_systime_struct(&st, 1601, 1, 0, 1, 0, 0, 0, 0);

    r = SendMessage(hWndDateTime, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    expect(1, r);

    /* set st to highest possible value */
    fill_systime_struct(&st, 30827, 12, 6, 31, 23, 59, 59, 999);

    r = SendMessage(hWndDateTime, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    expect(1, r);

    /* set st to value between min and max */
    fill_systime_struct(&st, 1980, 1, 3, 23, 14, 34, 37, 465);

    r = SendMessage(hWndDateTime, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    expect(1, r);
    r = SendMessage(hWndDateTime, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt);
    ok(r == GDT_VALID, "Expected %d, not %d(GDT_NONE) or %d(GDT_ERROR), got %ld\n", GDT_VALID, GDT_NONE, GDT_ERROR, r);
    expect_systime(&st, &getSt);

    /* set st to invalid value */
    fill_systime_struct(&st, 0, 0, 7, 0, 24, 60, 60, 1000);

    todo_wine {
        r = SendMessage(hWndDateTime, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
        expect_unsuccess(0, r);
    }
}

static void test_datetime_control(void)
{
    HWND hWndDateTime;

    hWndDateTime = create_datetime_control(DTS_SHOWNONE, 0);

    ok(hWndDateTime != NULL, "Expected non NULL, got %p\n", hWndDateTime);
    if(hWndDateTime!=NULL) {
        test_dtm_set_format(hWndDateTime);
        test_dtm_set_and_get_mccolor(hWndDateTime);
        test_dtm_set_and_get_mcfont(hWndDateTime);
        test_dtm_get_monthcal(hWndDateTime);
        test_dtm_set_and_get_range(hWndDateTime);
        test_dtm_set_range_swap_min_max(hWndDateTime);
        test_dtm_set_and_get_system_time(hWndDateTime);
        }
    else {
        skip("hWndDateTime is NULL\n");
    }

    DestroyWindow(hWndDateTime);
}

START_TEST(datetime)
{
    INITCOMMONCONTROLSEX icex;

    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_DATE_CLASSES;
    InitCommonControlsEx(&icex);

    test_datetime_control();
}
