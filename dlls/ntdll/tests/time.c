/*
 * Unit test suite for ntdll time functions
 *
 * Copyright 2004 Rein Klazes
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

#define NONAMELESSUNION
#include "ntdll_test.h"
#include "ddk/wdm.h"

#define TICKSPERSEC        10000000
#define TICKSPERMSEC       10000
#define SECSPERDAY         86400

static VOID (WINAPI *pRtlTimeToTimeFields)( const LARGE_INTEGER *liTime, PTIME_FIELDS TimeFields) ;
static VOID (WINAPI *pRtlTimeFieldsToTime)(  PTIME_FIELDS TimeFields,  PLARGE_INTEGER Time) ;
static NTSTATUS (WINAPI *pNtQueryPerformanceCounter)( LARGE_INTEGER *counter, LARGE_INTEGER *frequency );
static NTSTATUS (WINAPI *pNtQuerySystemInformation)( SYSTEM_INFORMATION_CLASS class,
                                                     void *info, ULONG size, ULONG *ret_size );
static NTSTATUS (WINAPI *pRtlQueryTimeZoneInformation)( RTL_TIME_ZONE_INFORMATION *);
static NTSTATUS (WINAPI *pRtlQueryDynamicTimeZoneInformation)( RTL_DYNAMIC_TIME_ZONE_INFORMATION *);
static BOOL     (WINAPI *pRtlQueryUnbiasedInterruptTime)( ULONGLONG *time );

static const int MonthLengths[2][12] =
{
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static inline BOOL IsLeapYear(int Year)
{
    return Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0);
}

/* start time of the tests */
static TIME_FIELDS tftest = {1889,12,31,23,59,59,0,0};

static void test_pRtlTimeToTimeFields(void)
{
    LARGE_INTEGER litime , liresult;
    TIME_FIELDS tfresult;
    int i=0;
    litime.QuadPart = ((ULONGLONG)0x0144017a << 32) | 0xf0b0a980;
    while( tftest.Year < 2110 ) {
        /* test at the last second of the month */
        pRtlTimeToTimeFields( &litime, &tfresult);
        ok( tfresult.Year == tftest.Year && tfresult.Month == tftest.Month &&
            tfresult.Day == tftest.Day && tfresult.Hour == tftest.Hour && 
            tfresult.Minute == tftest.Minute && tfresult.Second == tftest.Second,
            "#%d expected: %d-%d-%d %d:%d:%d  got:  %d-%d-%d %d:%d:%d\n", ++i,
            tftest.Year, tftest.Month, tftest.Day,
            tftest.Hour, tftest.Minute,tftest.Second,
            tfresult.Year, tfresult.Month, tfresult.Day,
            tfresult.Hour, tfresult.Minute, tfresult.Second);
        /* test the inverse */
        pRtlTimeFieldsToTime( &tfresult, &liresult);
        ok( liresult.QuadPart == litime.QuadPart," TimeFieldsToTime failed on %d-%d-%d %d:%d:%d. Error is %d ticks\n",
            tfresult.Year, tfresult.Month, tfresult.Day,
            tfresult.Hour, tfresult.Minute, tfresult.Second,
            (int) (liresult.QuadPart - litime.QuadPart) );
        /*  one second later is beginning of next month */
        litime.QuadPart +=  TICKSPERSEC ;
        pRtlTimeToTimeFields( &litime, &tfresult);
        ok( tfresult.Year == tftest.Year + (tftest.Month ==12) &&
            tfresult.Month == tftest.Month % 12 + 1 &&
            tfresult.Day == 1 && tfresult.Hour == 0 && 
            tfresult.Minute == 0 && tfresult.Second == 0,
            "#%d expected: %d-%d-%d %d:%d:%d  got:  %d-%d-%d %d:%d:%d\n", ++i,
            tftest.Year + (tftest.Month ==12),
            tftest.Month % 12 + 1, 1, 0, 0, 0,
            tfresult.Year, tfresult.Month, tfresult.Day,
            tfresult.Hour, tfresult.Minute, tfresult.Second);
        /* test the inverse */
        pRtlTimeFieldsToTime( &tfresult, &liresult);
        ok( liresult.QuadPart == litime.QuadPart," TimeFieldsToTime failed on %d-%d-%d %d:%d:%d. Error is %d ticks\n",
            tfresult.Year, tfresult.Month, tfresult.Day,
            tfresult.Hour, tfresult.Minute, tfresult.Second,
            (int) (liresult.QuadPart - litime.QuadPart) );
        /* advance to the end of the month */
        litime.QuadPart -=  TICKSPERSEC ;
        if( tftest.Month == 12) {
            tftest.Month = 1;
            tftest.Year += 1;
        } else 
            tftest.Month += 1;
        tftest.Day = MonthLengths[IsLeapYear(tftest.Year)][tftest.Month - 1];
        litime.QuadPart +=  (LONGLONG) tftest.Day * TICKSPERSEC * SECSPERDAY;
    }
}

static void test_NtQueryPerformanceCounter(void)
{
    LARGE_INTEGER counter, frequency;
    NTSTATUS status;

    status = pNtQueryPerformanceCounter(NULL, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %08x\n", status);
    status = pNtQueryPerformanceCounter(NULL, &frequency);
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %08x\n", status);
    status = pNtQueryPerformanceCounter(&counter, (void *)0xdeadbee0);
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %08x\n", status);
    status = pNtQueryPerformanceCounter((void *)0xdeadbee0, &frequency);
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %08x\n", status);

    status = pNtQueryPerformanceCounter(&counter, NULL);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08x\n", status);
    status = pNtQueryPerformanceCounter(&counter, &frequency);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08x\n", status);
}

static void test_RtlQueryTimeZoneInformation(void)
{
    RTL_DYNAMIC_TIME_ZONE_INFORMATION tzinfo, tzinfo2;
    NTSTATUS status;
    ULONG len;

    /* test RtlQueryTimeZoneInformation returns an indirect string,
       e.g. @tzres.dll,-32 (Vista or later) */
    if (!pRtlQueryTimeZoneInformation || !pRtlQueryDynamicTimeZoneInformation)
    {
        win_skip("Time zone name tests require Vista or later\n");
        return;
    }

    memset(&tzinfo, 0xcc, sizeof(tzinfo));
    status = pRtlQueryDynamicTimeZoneInformation(&tzinfo);
    ok(status == STATUS_SUCCESS,
       "RtlQueryDynamicTimeZoneInformation failed, got %08x\n", status);
    ok(tzinfo.StandardName[0] == '@',
       "standard time zone name isn't an indirect string, got %s\n",
       wine_dbgstr_w(tzinfo.StandardName));
    ok(tzinfo.DaylightName[0] == '@',
       "daylight time zone name isn't an indirect string, got %s\n",
       wine_dbgstr_w(tzinfo.DaylightName));

    memset(&tzinfo2, 0xcc, sizeof(tzinfo2));
    status = pNtQuerySystemInformation( SystemDynamicTimeZoneInformation, &tzinfo2, sizeof(tzinfo2), &len );
    ok( !status, "NtQuerySystemInformation failed %x\n", status );
    ok( len == sizeof(tzinfo2), "wrong len %u\n", len );
    ok( !memcmp( &tzinfo, &tzinfo2, sizeof(tzinfo2) ), "tz data is different\n" );

    memset(&tzinfo, 0xcc, sizeof(tzinfo));
    status = pRtlQueryTimeZoneInformation((RTL_TIME_ZONE_INFORMATION *)&tzinfo);
    ok(status == STATUS_SUCCESS,
       "RtlQueryTimeZoneInformation failed, got %08x\n", status);
    ok(tzinfo.StandardName[0] == '@',
       "standard time zone name isn't an indirect string, got %s\n",
       wine_dbgstr_w(tzinfo.StandardName));
    ok(tzinfo.DaylightName[0] == '@',
       "daylight time zone name isn't an indirect string, got %s\n",
       wine_dbgstr_w(tzinfo.DaylightName));

    memset(&tzinfo, 0xcc, sizeof(tzinfo));
    status = pRtlQueryTimeZoneInformation((RTL_TIME_ZONE_INFORMATION *)&tzinfo);
    ok(status == STATUS_SUCCESS,
       "RtlQueryTimeZoneInformation failed, got %08x\n", status);
    ok(tzinfo.StandardName[0] == '@',
       "standard time zone name isn't an indirect string, got %s\n",
       wine_dbgstr_w(tzinfo.StandardName));
    ok(tzinfo.DaylightName[0] == '@',
       "daylight time zone name isn't an indirect string, got %s\n",
       wine_dbgstr_w(tzinfo.DaylightName));

    memset(&tzinfo2, 0xcc, sizeof(tzinfo2));
    status = pNtQuerySystemInformation( SystemTimeZoneInformation, &tzinfo2,
                                        sizeof(RTL_TIME_ZONE_INFORMATION), &len );
    ok( !status, "NtQuerySystemInformation failed %x\n", status );
    ok( len == sizeof(RTL_TIME_ZONE_INFORMATION), "wrong len %u\n", len );
    ok( !memcmp( &tzinfo, &tzinfo2, sizeof(RTL_TIME_ZONE_INFORMATION) ), "tz data is different\n" );
}

static ULONGLONG read_ksystem_time(volatile KSYSTEM_TIME *time)
{
    ULONGLONG high, low;
    do
    {
        high = time->High1Time;
        low = time->LowPart;
    }
    while (high != time->High2Time);
    return high << 32 | low;
}

static void test_user_shared_data_time(void)
{
    KSHARED_USER_DATA *user_shared_data = (void *)0x7ffe0000;
    ULONGLONG t1, t2, t3;
    int i = 0;

    i = 0;
    do
    {
        t1 = GetTickCount();
        if (user_shared_data->NtMajorVersion <= 5 && user_shared_data->NtMinorVersion <= 1)
            t2 = (*(volatile ULONG*)&user_shared_data->TickCountLowDeprecated * (ULONG64)user_shared_data->TickCountMultiplier) >> 24;
        else
            t2 = (read_ksystem_time(&user_shared_data->u.TickCount) * user_shared_data->TickCountMultiplier) >> 24;
        t3 = GetTickCount();
    } while(t3 < t1 && i++ < 1); /* allow for wrap, but only once */

    ok(t1 <= t2, "USD TickCount / GetTickCount are out of order: %s %s\n",
       wine_dbgstr_longlong(t1), wine_dbgstr_longlong(t2));
    ok(t2 <= t3, "USD TickCount / GetTickCount are out of order: %s %s\n",
       wine_dbgstr_longlong(t2), wine_dbgstr_longlong(t3));

    i = 0;
    do
    {
        LARGE_INTEGER system_time;
        NtQuerySystemTime(&system_time);
        t1 = system_time.QuadPart;
        t2 = read_ksystem_time(&user_shared_data->SystemTime);
        NtQuerySystemTime(&system_time);
        t3 = system_time.QuadPart;
    } while(t3 < t1 && i++ < 1); /* allow for wrap, but only once */

    /* FIXME: not always in order, but should be close */
    todo_wine_if(t1 > t2 && t1 - t2 < 50 * TICKSPERMSEC)
    ok(t1 <= t2, "USD SystemTime / NtQuerySystemTime are out of order %s %s\n",
       wine_dbgstr_longlong(t1), wine_dbgstr_longlong(t2));
    ok(t2 <= t3, "USD SystemTime / NtQuerySystemTime are out of order %s %s\n",
       wine_dbgstr_longlong(t2), wine_dbgstr_longlong(t3));

    if (!pRtlQueryUnbiasedInterruptTime)
        win_skip("skipping RtlQueryUnbiasedInterruptTime tests\n");
    else
    {
        i = 0;
        do
        {
            pRtlQueryUnbiasedInterruptTime(&t1);
            t2 = read_ksystem_time(&user_shared_data->InterruptTime);
            pRtlQueryUnbiasedInterruptTime(&t3);
        } while(t3 < t1 && i++ < 1); /* allow for wrap, but only once */

        ok(t1 <= t2, "USD InterruptTime / RtlQueryUnbiasedInterruptTime are out of order %s %s\n",
           wine_dbgstr_longlong(t1), wine_dbgstr_longlong(t2));
        ok(t2 <= t3 || broken(t2 == t3 + 82410089070) /* w864 has some weird offset on testbot */,
           "USD InterruptTime / RtlQueryUnbiasedInterruptTime are out of order %s %s\n",
           wine_dbgstr_longlong(t2), wine_dbgstr_longlong(t3));
    }
}

START_TEST(time)
{
    HMODULE mod = GetModuleHandleA("ntdll.dll");
    pRtlTimeToTimeFields = (void *)GetProcAddress(mod,"RtlTimeToTimeFields");
    pRtlTimeFieldsToTime = (void *)GetProcAddress(mod,"RtlTimeFieldsToTime");
    pNtQueryPerformanceCounter = (void *)GetProcAddress(mod, "NtQueryPerformanceCounter");
    pNtQuerySystemInformation = (void *)GetProcAddress(mod, "NtQuerySystemInformation");
    pRtlQueryTimeZoneInformation =
        (void *)GetProcAddress(mod, "RtlQueryTimeZoneInformation");
    pRtlQueryDynamicTimeZoneInformation =
        (void *)GetProcAddress(mod, "RtlQueryDynamicTimeZoneInformation");
    pRtlQueryUnbiasedInterruptTime = (void *)GetProcAddress(mod, "RtlQueryUnbiasedInterruptTime");

    if (pRtlTimeToTimeFields && pRtlTimeFieldsToTime)
        test_pRtlTimeToTimeFields();
    else
        win_skip("Required time conversion functions are not available\n");
    test_NtQueryPerformanceCounter();
    test_RtlQueryTimeZoneInformation();
    test_user_shared_data_time();
}
