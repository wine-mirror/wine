/*
 * Tests for pdh.dll (Performance Data Helper)
 *
 * Copyright 2007 Hans Leidekker
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

#include <stdio.h>

#include "windows.h"

#include "pdh.h"
#include "pdhmsg.h"

#include "wine/test.h"

static void test_open_close_query( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;

    ret = PdhOpenQueryA( NULL, 0, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhOpenQueryA failed 0x%08x\n", ret);

    ret = PdhOpenQueryA( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryA failed 0x%08x\n", ret);

    ret = PdhCloseQuery( NULL );
    ok(ret == PDH_INVALID_HANDLE, "PdhCloseQuery failed 0x%08x\n", ret);

    ret = PdhCloseQuery( &query );
    ok(ret == PDH_INVALID_HANDLE, "PdhCloseQuery failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_add_remove_counter( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;

    ret = PdhOpenQueryA( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( NULL, "\\System\\System Up Time", 0, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( NULL, "\\System\\System Up Time", 0, &counter );
    ok(ret == PDH_INVALID_HANDLE, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( query, NULL, 0, &counter );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( query, "\\System\\System Up Time", 0, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( query, "\\System\\System Down Time", 0, &counter );
    ok(ret == PDH_CSTATUS_NO_COUNTER, "PdhAddCounterA failed 0x%08x\n", ret);
    ok(!counter, "PdhAddCounterA failed %p\n", counter);

    ret = PdhAddCounterA( query, "\\System\\System Up Time", 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhCollectQueryData( NULL );
    ok(ret == PDH_INVALID_HANDLE, "PdhCollectQueryData failed 0x%08x\n", ret);

    ret = PdhCollectQueryData( counter );
    ok(ret == PDH_INVALID_HANDLE, "PdhCollectQueryData failed 0x%08x\n", ret);

    ret = PdhCollectQueryData( query );
    ok(ret == ERROR_SUCCESS, "PdhCollectQueryData failed 0x%08x\n", ret);

    ret = PdhRemoveCounter( NULL );
    ok(ret == PDH_INVALID_HANDLE, "PdhRemoveCounter failed 0x%08x\n", ret);

    ret = PdhRemoveCounter( counter );
    ok(ret == ERROR_SUCCESS, "PdhRemoveCounter failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhGetFormattedCounterValue( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;
    PDH_FMT_COUNTERVALUE value;

    ret = PdhOpenQueryA( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( query, "\\System\\System Up Time", 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhGetFormattedCounterValue( NULL, PDH_FMT_LARGE, NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhGetFormattedCounterValue failed 0x%08x\n", ret);

    ret = PdhGetFormattedCounterValue( NULL, PDH_FMT_LARGE, NULL, &value );
    ok(ret == PDH_INVALID_HANDLE, "PdhGetFormattedCounterValue failed 0x%08x\n", ret);

    ret = PdhGetFormattedCounterValue( counter, PDH_FMT_LARGE, NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhGetFormattedCounterValue failed 0x%08x\n", ret);

    ret = PdhGetFormattedCounterValue( counter, PDH_FMT_LARGE, NULL, &value );
    ok(ret == ERROR_SUCCESS, "PdhGetFormattedCounterValue failed 0x%08x\n", ret);

    ret = PdhCollectQueryData( query );
    ok(ret == ERROR_SUCCESS, "PdhCollectQueryData failed 0x%08x\n", ret);

    ret = PdhGetFormattedCounterValue( counter, PDH_FMT_LARGE, NULL, &value );
    ok(ret == ERROR_SUCCESS, "PdhGetFormattedCounterValue failed 0x%08x\n", ret);

    ret = PdhGetFormattedCounterValue( counter, PDH_FMT_LARGE | PDH_FMT_NOSCALE, NULL, &value );
    ok(ret == ERROR_SUCCESS, "PdhGetFormattedCounterValue failed 0x%08x\n", ret);

    ret = PdhGetFormattedCounterValue( counter, PDH_FMT_LARGE | PDH_FMT_NOCAP100, NULL, &value );
    ok(ret == ERROR_SUCCESS, "PdhGetFormattedCounterValue failed 0x%08x\n", ret);

    ret = PdhGetFormattedCounterValue( counter, PDH_FMT_LARGE | PDH_FMT_1000, NULL, &value );
    ok(ret == ERROR_SUCCESS, "PdhGetFormattedCounterValue failed 0x%08x\n", ret);

    ret = PdhSetCounterScaleFactor( counter, 2 );
    ok(ret == ERROR_SUCCESS, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhGetFormattedCounterValue( counter, PDH_FMT_LARGE, NULL, &value );
    ok(ret == ERROR_SUCCESS, "PdhGetFormattedCounterValue failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhGetRawCounterValue( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;
    PDH_RAW_COUNTER value;

    ret = PdhOpenQueryA( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( query, "\\System\\System Up Time", 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhGetRawCounterValue( NULL, NULL, &value );
    ok(ret == PDH_INVALID_HANDLE, "PdhGetRawCounterValue failed 0x%08x\n", ret);

    ret = PdhGetRawCounterValue( counter, NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhGetRawCounterValue failed 0x%08x\n", ret);

    ret = PdhGetRawCounterValue( counter, NULL, &value );
    ok(ret == ERROR_SUCCESS, "PdhGetRawCounterValue failed 0x%08x\n", ret);
    ok(value.CStatus == ERROR_SUCCESS, "expected ERROR_SUCCESS got %x", value.CStatus);

    ret = PdhCollectQueryData( query );
    ok(ret == ERROR_SUCCESS, "PdhCollectQueryData failed 0x%08x\n", ret);

    ret = PdhGetRawCounterValue( counter, NULL, &value );
    ok(ret == ERROR_SUCCESS, "PdhGetRawCounterValue failed 0x%08x\n", ret);
    ok(value.CStatus == ERROR_SUCCESS, "expected ERROR_SUCCESS got %x", value.CStatus);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhSetCounterScaleFactor( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;

    ret = PdhOpenQueryA( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( query, "\\System\\System Up Time", 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhSetCounterScaleFactor( NULL, 8 );
    ok(ret == PDH_INVALID_HANDLE, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhSetCounterScaleFactor( NULL, 1 );
    ok(ret == PDH_INVALID_HANDLE, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhSetCounterScaleFactor( counter, 8 );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhSetCounterScaleFactor( counter, -8 );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhSetCounterScaleFactor( counter, 7 );
    ok(ret == ERROR_SUCCESS, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhSetCounterScaleFactor( counter, 0 );
    ok(ret == ERROR_SUCCESS, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhGetCounterTimeBase( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;
    LONGLONG base;

    ret = PdhOpenQueryA( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( query, "\\System\\System Up Time", 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhGetCounterTimeBase( NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhGetCounterTimeBase failed 0x%08x\n", ret);

    ret = PdhGetCounterTimeBase( NULL, &base );
    ok(ret == PDH_INVALID_HANDLE, "PdhGetCounterTimeBase failed 0x%08x\n", ret);

    ret = PdhGetCounterTimeBase( counter, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhGetCounterTimeBase failed 0x%08x\n", ret);

    ret = PdhGetCounterTimeBase( counter, &base );
    ok(ret == ERROR_SUCCESS, "PdhGetCounterTimeBase failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhGetCounterInfo( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;
    PDH_COUNTER_INFO_A info;
    DWORD size;

    ret = PdhOpenQueryA( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( query, "\\System\\System Up Time", 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoA( NULL, 0, NULL, NULL );
    ok(ret == PDH_INVALID_HANDLE, "PdhGetCounterInfoA failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoA( counter, 0, NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhGetCounterInfoA failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoA( counter, 0, NULL, &info );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhGetCounterInfoA failed 0x%08x\n", ret);

    size = sizeof(info) - 1;
    ret = PdhGetCounterInfoA( counter, 0, &size, NULL );
    ok(ret == PDH_MORE_DATA, "PdhGetCounterInfoA failed 0x%08x\n", ret);

    size = sizeof(info);
    ret = PdhGetCounterInfoA( counter, 0, &size, &info );
    ok(ret == ERROR_SUCCESS, "PdhGetCounterInfoA failed 0x%08x\n", ret);
    ok(size == sizeof(info), "PdhGetCounterInfoA failed %d\n", size);

    ret = PdhGetCounterInfoA( counter, 0, &size, &info );
    ok(ret == ERROR_SUCCESS, "PdhGetCounterInfoA failed 0x%08x\n", ret);
    ok(info.lScale == 0, "lScale %d\n", info.lScale);

    ret = PdhSetCounterScaleFactor( counter, 0 );
    ok(ret == ERROR_SUCCESS, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoA( counter, 0, &size, &info );
    ok(ret == ERROR_SUCCESS, "PdhGetCounterInfoA failed 0x%08x\n", ret);
    ok(info.lScale == 0, "lScale %d\n", info.lScale);

    ret = PdhSetCounterScaleFactor( counter, -5 );
    ok(ret == ERROR_SUCCESS, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoA( counter, 0, &size, &info );
    ok(ret == ERROR_SUCCESS, "PdhGetCounterInfoA failed 0x%08x\n", ret);
    ok(info.lScale == -5, "lScale %d\n", info.lScale);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

START_TEST(pdh)
{
    test_open_close_query();
    test_add_remove_counter();
    test_PdhGetFormattedCounterValue();
    test_PdhGetRawCounterValue();
    test_PdhSetCounterScaleFactor();
    test_PdhGetCounterTimeBase();
    test_PdhGetCounterInfo();
}
