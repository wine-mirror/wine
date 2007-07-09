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

START_TEST(pdh)
{
    test_open_close_query();
    test_add_remove_counter();
    test_PdhGetFormattedCounterValue();
    test_PdhSetCounterScaleFactor();
}
