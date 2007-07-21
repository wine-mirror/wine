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

static const WCHAR processor_time[] =
    {'%',' ','P','r','o','c','e','s','s','o','r',' ','T','i','m','e',0};
static const WCHAR uptime[] =
    {'S','y','s','t','e','m',' ','U','p',' ','T','i','m','e',0};

static const WCHAR system_uptime[] =
    {'\\','S','y','s','t','e','m','\\','S','y','s','t','e','m',' ','U','p',' ','T','i','m','e',0};
static const WCHAR system_downtime[] = /* does not exist */
    {'\\','S','y','s','t','e','m','\\','S','y','s','t','e','m',' ','D','o','w','n',' ','T','i','m','e',0};
static const WCHAR percentage_processor_time[] =
    {'\\','P','r','o','c','e','s','s','o','r','(','_','T','o','t','a','l',')',
     '\\','%',' ','P','r','o','c','e','s','s','o','r',' ','T','i','m','e',0};

static void test_PdhOpenQueryA( void )
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

    ret = PdhCloseQuery( query );
    ok(ret == PDH_INVALID_HANDLE, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhOpenQueryW( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;

    ret = PdhOpenQueryW( NULL, 0, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhOpenQueryW failed 0x%08x\n", ret);

    ret = PdhOpenQueryW( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryW failed 0x%08x\n", ret);

    ret = PdhCloseQuery( NULL );
    ok(ret == PDH_INVALID_HANDLE, "PdhCloseQuery failed 0x%08x\n", ret);

    ret = PdhCloseQuery( &query );
    ok(ret == PDH_INVALID_HANDLE, "PdhCloseQuery failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == PDH_INVALID_HANDLE, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhAddCounterA( void )
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

static void test_PdhAddCounterW( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;

    ret = PdhOpenQueryW( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryW failed 0x%08x\n", ret);

    ret = PdhAddCounterW( NULL, percentage_processor_time, 0, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddCounterW failed 0x%08x\n", ret);

    ret = PdhAddCounterW( NULL, percentage_processor_time, 0, &counter );
    ok(ret == PDH_INVALID_HANDLE, "PdhAddCounterW failed 0x%08x\n", ret);

    ret = PdhAddCounterW( query, NULL, 0, &counter );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddCounterW failed 0x%08x\n", ret);

    ret = PdhAddCounterW( query, percentage_processor_time, 0, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddCounterW failed 0x%08x\n", ret);

    ret = PdhAddCounterW( query, system_downtime, 0, &counter );
    ok(ret == PDH_CSTATUS_NO_COUNTER, "PdhAddCounterW failed 0x%08x\n", ret);
    ok(!counter, "PdhAddCounterW failed %p\n", counter);

    ret = PdhAddCounterW( query, percentage_processor_time, 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddCounterW failed 0x%08x\n", ret);

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

static void test_PdhGetCounterInfoA( void )
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

static void test_PdhGetCounterInfoW( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;
    PDH_COUNTER_INFO_W info;
    DWORD size;

    ret = PdhOpenQueryW( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryW failed 0x%08x\n", ret);

    ret = PdhAddCounterW( query, percentage_processor_time, 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddCounterW failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoW( NULL, 0, NULL, NULL );
    ok(ret == PDH_INVALID_HANDLE, "PdhGetCounterInfoW failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoW( counter, 0, NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhGetCounterInfoW failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoW( counter, 0, NULL, &info );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhGetCounterInfoW failed 0x%08x\n", ret);

    size = sizeof(info) - 1;
    ret = PdhGetCounterInfoW( counter, 0, &size, NULL );
    ok(ret == PDH_MORE_DATA, "PdhGetCounterInfoW failed 0x%08x\n", ret);

    size = sizeof(info);
    ret = PdhGetCounterInfoW( counter, 0, &size, &info );
    ok(ret == ERROR_SUCCESS, "PdhGetCounterInfoW failed 0x%08x\n", ret);
    ok(size == sizeof(info), "PdhGetCounterInfoW failed %d\n", size);

    ret = PdhGetCounterInfoW( counter, 0, &size, &info );
    ok(ret == ERROR_SUCCESS, "PdhGetCounterInfoW failed 0x%08x\n", ret);
    ok(info.lScale == 0, "lScale %d\n", info.lScale);

    ret = PdhSetCounterScaleFactor( counter, 0 );
    ok(ret == ERROR_SUCCESS, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoW( counter, 0, &size, &info );
    ok(ret == ERROR_SUCCESS, "PdhGetCounterInfoW failed 0x%08x\n", ret);
    ok(info.lScale == 0, "lScale %d\n", info.lScale);

    ret = PdhSetCounterScaleFactor( counter, -5 );
    ok(ret == ERROR_SUCCESS, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoW( counter, 0, &size, &info );
    ok(ret == ERROR_SUCCESS, "PdhGetCounterInfoW failed 0x%08x\n", ret);
    ok(info.lScale == -5, "lScale %d\n", info.lScale);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhLookupPerfIndexByNameA( void )
{
    PDH_STATUS ret;
    DWORD index;

    ret = PdhLookupPerfIndexByNameA( NULL, NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhLookupPerfIndexByNameA failed 0x%08x\n", ret);

    ret = PdhLookupPerfIndexByNameA( NULL, NULL, &index );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhLookupPerfIndexByNameA failed 0x%08x\n", ret);

    ret = PdhLookupPerfIndexByNameA( NULL, "No Counter", &index );
    ok(ret == PDH_STRING_NOT_FOUND, "PdhLookupPerfIndexByNameA failed 0x%08x\n", ret);

    ret = PdhLookupPerfIndexByNameA( NULL, "% Processor Time", NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhLookupPerfIndexByNameA failed 0x%08x\n", ret);

    ret = PdhLookupPerfIndexByNameA( NULL, "% Processor Time", &index );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfIndexByNameA failed 0x%08x\n", ret);
    ok(index == 6, "PdhLookupPerfIndexByNameA failed %d\n", index);

    ret = PdhLookupPerfIndexByNameA( NULL, "System Up Time", &index );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfIndexByNameA failed 0x%08x\n", ret);
    ok(index == 674, "PdhLookupPerfIndexByNameA failed %d\n", index);
}

static void test_PdhLookupPerfIndexByNameW( void )
{
    PDH_STATUS ret;
    DWORD index;

    static const WCHAR no_counter[] = {'N','o',' ','C','o','u','n','t','e','r',0};

    ret = PdhLookupPerfIndexByNameW( NULL, NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhLookupPerfIndexByNameW failed 0x%08x\n", ret);

    ret = PdhLookupPerfIndexByNameW( NULL, NULL, &index );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhLookupPerfIndexByNameW failed 0x%08x\n", ret);

    ret = PdhLookupPerfIndexByNameW( NULL, no_counter, &index );
    ok(ret == PDH_STRING_NOT_FOUND, "PdhLookupPerfIndexByNameW failed 0x%08x\n", ret);

    ret = PdhLookupPerfIndexByNameW( NULL, processor_time, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhLookupPerfIndexByNameW failed 0x%08x\n", ret);

    ret = PdhLookupPerfIndexByNameW( NULL, processor_time, &index );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfIndexByNameW failed 0x%08x\n", ret);
    ok(index == 6, "PdhLookupPerfIndexByNameW failed %d\n", index);

    ret = PdhLookupPerfIndexByNameW( NULL, uptime, &index );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfIndexByNameW failed 0x%08x\n", ret);
    ok(index == 674, "PdhLookupPerfIndexByNameW failed %d\n", index);
}

static void test_PdhLookupPerfNameByIndexA( void )
{
    PDH_STATUS ret;
    char buffer[PDH_MAX_COUNTER_NAME] = "!!";
    DWORD size;

    ret = PdhLookupPerfNameByIndexA( NULL, 0, NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhLookupPerfNameByIndexA failed 0x%08x\n", ret);

    size = 1;
    ret = PdhLookupPerfNameByIndexA( NULL, 0, NULL, &size );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfNameByIndexA failed 0x%08x\n", ret);
    ok(size == 1, "PdhLookupPerfNameByIndexA failed %d\n", size);

    size = sizeof(buffer);
    ret = PdhLookupPerfNameByIndexA( NULL, 0, buffer, &size );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfNameByIndexA failed 0x%08x\n", ret);
    ok(!strcmp(buffer, "!!"), "PdhLookupPerfNameByIndexA failed %s\n", buffer);

    size = 0;
    ret = PdhLookupPerfNameByIndexA( NULL, 6, buffer, &size );
    ok(ret == PDH_MORE_DATA, "PdhLookupPerfNameByIndexA failed 0x%08x\n", ret);
    ok(size == sizeof("% Processor Time"), "PdhLookupPerfNameByIndexA failed %d\n", size);

    size = sizeof(buffer);
    ret = PdhLookupPerfNameByIndexA( NULL, 6, buffer, &size );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfNameByIndexA failed 0x%08x\n", ret);
    ok(!lstrcmpA( buffer, "% Processor Time" ),
       "PdhLookupPerfNameByIndexA failed, got %s expected \'%% Processor Time\'\n", buffer);
    ok(size == sizeof("% Processor Time"), "PdhLookupPerfNameByIndexA failed %d\n", size);

    size = 0;
    ret = PdhLookupPerfNameByIndexA( NULL, 674, NULL, &size );
    ok(ret == PDH_MORE_DATA, "PdhLookupPerfNameByIndexA failed 0x%08x\n", ret);
    ok(size == sizeof("System Up Time"), "PdhLookupPerfNameByIndexA failed %d\n", size);

    size = sizeof(buffer);
    ret = PdhLookupPerfNameByIndexA( NULL, 674, buffer, &size );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfNameByIndexA failed 0x%08x\n", ret);
    ok(!lstrcmpA( buffer, "System Up Time" ),
       "PdhLookupPerfNameByIndexA failed, got %s expected \'System Up Time\'\n", buffer);
    ok(size == sizeof("System Up Time"), "PdhLookupPerfNameByIndexA failed %d\n", size);
}

static void test_PdhLookupPerfNameByIndexW( void )
{
    PDH_STATUS ret;
    WCHAR buffer[PDH_MAX_COUNTER_NAME];
    DWORD size;

    ret = PdhLookupPerfNameByIndexW( NULL, 0, NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhLookupPerfNameByIndexW failed 0x%08x\n", ret);

    size = 1;
    ret = PdhLookupPerfNameByIndexW( NULL, 0, NULL, &size );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfNameByIndexW failed 0x%08x\n", ret);
    ok(size == 1, "PdhLookupPerfNameByIndexW failed %d\n", size);

    size = sizeof(buffer) / sizeof(WCHAR);
    ret = PdhLookupPerfNameByIndexW( NULL, 0, buffer, &size );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfNameByIndexW failed 0x%08x\n", ret);

    size = 0;
    ret = PdhLookupPerfNameByIndexW( NULL, 6, buffer, &size );
    ok(ret == PDH_MORE_DATA, "PdhLookupPerfNameByIndexW failed 0x%08x\n", ret);
    ok(size == sizeof(processor_time) / sizeof(WCHAR), "PdhLookupPerfNameByIndexW failed %d\n", size);

    size = sizeof(buffer) / sizeof(WCHAR);
    ret = PdhLookupPerfNameByIndexW( NULL, 6, buffer, &size );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfNameByIndexW failed 0x%08x\n", ret);
    ok(size == sizeof(processor_time) / sizeof(WCHAR), "PdhLookupPerfNameByIndexW failed %d\n", size);

    size = 0;
    ret = PdhLookupPerfNameByIndexW( NULL, 674, NULL, &size );
    ok(ret == PDH_MORE_DATA, "PdhLookupPerfNameByIndexW failed 0x%08x\n", ret);
    ok(size == sizeof(uptime) / sizeof(WCHAR), "PdhLookupPerfNameByIndexW failed %d\n", size);

    size = sizeof(buffer) / sizeof(WCHAR);
    ret = PdhLookupPerfNameByIndexW( NULL, 674, buffer, &size );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfNameByIndexW failed 0x%08x\n", ret);
    ok(size == sizeof(uptime) / sizeof(WCHAR), "PdhLookupPerfNameByIndexW failed %d\n", size);
}

START_TEST(pdh)
{
    test_PdhOpenQueryA();
    test_PdhOpenQueryW();

    test_PdhAddCounterA();
    test_PdhAddCounterW();

    test_PdhGetFormattedCounterValue();
    test_PdhGetRawCounterValue();
    test_PdhSetCounterScaleFactor();
    test_PdhGetCounterTimeBase();

    test_PdhGetCounterInfoA();
    test_PdhGetCounterInfoW();

    test_PdhLookupPerfIndexByNameA();
    test_PdhLookupPerfIndexByNameW();

    test_PdhLookupPerfNameByIndexA();
    test_PdhLookupPerfNameByIndexW();
}
