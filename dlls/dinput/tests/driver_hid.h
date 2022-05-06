/*
 * ntoskrnl.exe testing framework
 *
 * Copyright 2015 Sebastian Lackner
 * Copyright 2015 Michael Müller
 * Copyright 2015 Christian Costa
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

#ifndef __WINE_DRIVER_HID_H
#define __WINE_DRIVER_HID_H

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winioctl.h"
#include "winternl.h"

#include "ddk/wdm.h"
#include "ddk/hidsdi.h"
#include "ddk/hidport.h"
#include "ddk/hidclass.h"

DEFINE_GUID(control_class,0xdeadbeef,0x29ef,0x4538,0xa5,0xfd,0xb6,0x95,0x73,0xa3,0x62,0xc0);

#define IOCTL_WINETEST_HID_SET_EXPECT    CTL_CODE(FILE_DEVICE_KEYBOARD, 0x800, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_HID_WAIT_EXPECT   CTL_CODE(FILE_DEVICE_KEYBOARD, 0x801, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_HID_SEND_INPUT    CTL_CODE(FILE_DEVICE_KEYBOARD, 0x802, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_HID_SET_CONTEXT   CTL_CODE(FILE_DEVICE_KEYBOARD, 0x803, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_CREATE_DEVICE     CTL_CODE(FILE_DEVICE_KEYBOARD, 0x804, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_REMOVE_DEVICE     CTL_CODE(FILE_DEVICE_KEYBOARD, 0x805, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

struct hid_expect
{
    DWORD code;
    DWORD ret_length;
    DWORD ret_status;
    BYTE todo;   /* missing on wine */
    BYTE broken; /* missing on some win versions */
    BYTE wine_only;
    BYTE report_id;
    BYTE report_len;
    BYTE report_buf[128];
};

struct wait_expect_params
{
    BOOL wait_pending;
};

/* create/remove device */
#define MAX_HID_DESCRIPTOR_LEN 2048

struct hid_device_desc
{
    BOOL is_polled;
    BOOL use_report_id;

    DWORD report_descriptor_len;
    char report_descriptor_buf[MAX_HID_DESCRIPTOR_LEN];

    HIDP_CAPS caps;
    HID_DEVICE_ATTRIBUTES attributes;

    ULONG input_size;
    struct hid_expect input[64];
    ULONG expect_size;
    struct hid_expect expect[64];
    ULONG context_size;
    char context[64];
};

/* kernel/user shared data */
struct winetest_shared_data
{
    int running_under_wine;
    int winetest_report_success;
    int winetest_debug;
    LONG failures;
    LONG todo_failures;
};

static inline const char *debugstr_pnp( ULONG code )
{
    switch (code)
    {
    case IRP_MN_START_DEVICE: return "IRP_MN_START_DEVICE";
    case IRP_MN_QUERY_REMOVE_DEVICE: return "IRP_MN_QUERY_REMOVE_DEVICE";
    case IRP_MN_REMOVE_DEVICE: return "IRP_MN_REMOVE_DEVICE";
    case IRP_MN_CANCEL_REMOVE_DEVICE: return "IRP_MN_CANCEL_REMOVE_DEVICE";
    case IRP_MN_STOP_DEVICE: return "IRP_MN_STOP_DEVICE";
    case IRP_MN_QUERY_STOP_DEVICE: return "IRP_MN_QUERY_STOP_DEVICE";
    case IRP_MN_CANCEL_STOP_DEVICE: return "IRP_MN_CANCEL_STOP_DEVICE";
    case IRP_MN_QUERY_DEVICE_RELATIONS: return "IRP_MN_QUERY_DEVICE_RELATIONS";
    case IRP_MN_QUERY_INTERFACE: return "IRP_MN_QUERY_INTERFACE";
    case IRP_MN_QUERY_CAPABILITIES: return "IRP_MN_QUERY_CAPABILITIES";
    case IRP_MN_QUERY_RESOURCES: return "IRP_MN_QUERY_RESOURCES";
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS: return "IRP_MN_QUERY_RESOURCE_REQUIREMENTS";
    case IRP_MN_QUERY_DEVICE_TEXT: return "IRP_MN_QUERY_DEVICE_TEXT";
    case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: return "IRP_MN_FILTER_RESOURCE_REQUIREMENTS";
    case IRP_MN_READ_CONFIG: return "IRP_MN_READ_CONFIG";
    case IRP_MN_WRITE_CONFIG: return "IRP_MN_WRITE_CONFIG";
    case IRP_MN_EJECT: return "IRP_MN_EJECT";
    case IRP_MN_SET_LOCK: return "IRP_MN_SET_LOCK";
    case IRP_MN_QUERY_ID: return "IRP_MN_QUERY_ID";
    case IRP_MN_QUERY_PNP_DEVICE_STATE: return "IRP_MN_QUERY_PNP_DEVICE_STATE";
    case IRP_MN_QUERY_BUS_INFORMATION: return "IRP_MN_QUERY_BUS_INFORMATION";
    case IRP_MN_DEVICE_USAGE_NOTIFICATION: return "IRP_MN_DEVICE_USAGE_NOTIFICATION";
    case IRP_MN_SURPRISE_REMOVAL: return "IRP_MN_SURPRISE_REMOVAL";
    case IRP_MN_QUERY_LEGACY_BUS_INFORMATION: return "IRP_MN_QUERY_LEGACY_BUS_INFORMATION";
    default: return "unknown";
    }
}

static inline const char *debugstr_ioctl( ULONG code )
{
    switch (code)
    {
    case HID_CTL_CODE(0): return "IOCTL_HID_GET_DEVICE_DESCRIPTOR";
    case HID_CTL_CODE(1): return "IOCTL_HID_GET_REPORT_DESCRIPTOR";
    case HID_CTL_CODE(2): return "IOCTL_HID_READ_REPORT";
    case HID_CTL_CODE(3): return "IOCTL_HID_WRITE_REPORT";
    case HID_CTL_CODE(4): return "IOCTL_HID_GET_STRING";
    case HID_CTL_CODE(7): return "IOCTL_HID_ACTIVATE_DEVICE";
    case HID_CTL_CODE(8): return "IOCTL_HID_DEACTIVATE_DEVICE";
    case HID_CTL_CODE(9): return "IOCTL_HID_GET_DEVICE_ATTRIBUTES";
    case HID_CTL_CODE(10): return "IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST";
    case HID_OUT_CTL_CODE(102): return "IOCTL_GET_PHYSICAL_DESCRIPTOR";
    case HID_CTL_CODE(101): return "IOCTL_HID_FLUSH_QUEUE";
    case HID_CTL_CODE(100): return "IOCTL_HID_GET_COLLECTION_DESCRIPTOR";
    case HID_BUFFER_CTL_CODE(106): return "IOCTL_HID_GET_COLLECTION_INFORMATION";
    case HID_OUT_CTL_CODE(100): return "IOCTL_HID_GET_FEATURE";
    case HID_OUT_CTL_CODE(103): return "IOCTL_HID_GET_HARDWARE_ID";
    case HID_OUT_CTL_CODE(120): return "IOCTL_HID_GET_INDEXED_STRING";
    case HID_OUT_CTL_CODE(104): return "IOCTL_HID_GET_INPUT_REPORT";
    case HID_OUT_CTL_CODE(110): return "IOCTL_HID_GET_MANUFACTURER_STRING";
    case HID_BUFFER_CTL_CODE(104): return "IOCTL_GET_NUM_DEVICE_INPUT_BUFFERS";
    case HID_BUFFER_CTL_CODE(102): return "IOCTL_HID_GET_POLL_FREQUENCY_MSEC";
    case HID_OUT_CTL_CODE(111): return "IOCTL_HID_GET_PRODUCT_STRING";
    case HID_OUT_CTL_CODE(112): return "IOCTL_HID_GET_SERIALNUMBER_STRING";
    case HID_IN_CTL_CODE(100): return "IOCTL_HID_SET_FEATURE";
    case HID_BUFFER_CTL_CODE(105): return "IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS";
    case HID_IN_CTL_CODE(101): return "IOCTL_HID_SET_OUTPUT_REPORT";
    case HID_BUFFER_CTL_CODE(103): return "IOCTL_HID_SET_POLL_FREQUENCY_MSEC";
    case HID_BUFFER_CTL_CODE(100): return "IOCTL_HID_GET_DRIVER_CONFIG";
    case HID_BUFFER_CTL_CODE(101): return "IOCTL_HID_SET_DRIVER_CONFIG";
    case HID_OUT_CTL_CODE(121): return "IOCTL_HID_GET_MS_GENRE_DESCRIPTOR";
    case IOCTL_WINETEST_HID_SET_EXPECT: return "IOCTL_WINETEST_HID_SET_EXPECT";
    case IOCTL_WINETEST_HID_WAIT_EXPECT: return "IOCTL_WINETEST_HID_WAIT_EXPECT";
    case IOCTL_WINETEST_HID_SEND_INPUT: return "IOCTL_WINETEST_HID_SEND_INPUT";
    case IOCTL_WINETEST_HID_SET_CONTEXT: return "IOCTL_WINETEST_HID_SET_CONTEXT";
    case IOCTL_WINETEST_CREATE_DEVICE: return "IOCTL_WINETEST_CREATE_DEVICE";
    case IOCTL_WINETEST_REMOVE_DEVICE: return "IOCTL_WINETEST_REMOVE_DEVICE";
    default: return "unknown";
    }
}

#ifndef __WINE_WINE_TEST_H

#if !defined( __WINE_USE_MSVCRT ) || defined( __MINGW32__ )
#define __WINE_PRINTF_ATTR( fmt, args ) __attribute__((format( printf, fmt, args )))
#else
#define __WINE_PRINTF_ATTR( fmt, args )
#endif

static HANDLE okfile;
static LONG successes;
static LONG failures;
static LONG skipped;
static LONG todo_successes;
static LONG todo_failures;
static LONG muted_traces;
static LONG muted_skipped;
static LONG muted_todo_successes;

static int running_under_wine;
static int winetest_debug;
static int winetest_report_success;

/* silence todos and skips above this threshold */
static int winetest_mute_threshold = 42;

/* counts how many times a given line printed a message */
static LONG line_counters[16384];

/* The following data must be kept track of on a per-thread basis */
struct tls_data
{
    HANDLE thread;
    const char *current_file; /* file of current check */
    int current_line;         /* line of current check */
    unsigned int todo_level;  /* current todo nesting level */
    int todo_do_loop;
    char *str_pos;              /* position in debug buffer */
    char strings[2000];         /* buffer for debug strings */
    char context[8][128];       /* data to print before messages */
    unsigned int context_count; /* number of context prefixes */
};

static KSPIN_LOCK tls_data_lock;
static struct tls_data tls_data_pool[128];
static DWORD tls_data_count;

static inline struct tls_data *get_tls_data(void)
{
    static struct tls_data tls_overflow;
    struct tls_data *data;
    HANDLE thread = PsGetCurrentThreadId();
    KIRQL irql;

    KeAcquireSpinLock( &tls_data_lock, &irql );
    for (data = tls_data_pool; data != tls_data_pool + tls_data_count; ++data)
        if (data->thread == thread) break;
    if (data == tls_data_pool + ARRAY_SIZE(tls_data_pool)) data = &tls_overflow;
    else if (data == tls_data_pool + tls_data_count)
    {
        data->thread = thread;
        data->str_pos = data->strings;
        tls_data_count++;
    }
    KeReleaseSpinLock( &tls_data_lock, irql );

    return data;
}

static inline void winetest_set_location( const char *file, int line )
{
    struct tls_data *data = get_tls_data();
    data->current_file = strrchr( file, '/' );
    if (data->current_file == NULL) data->current_file = strrchr( file, '\\' );
    if (data->current_file == NULL) data->current_file = file;
    else data->current_file++;
    data->current_line = line;
}

static inline void kvprintf( const char *format, va_list ap )
{
    struct tls_data *data = get_tls_data();
    IO_STATUS_BLOCK io;
    int len = vsnprintf( data->str_pos, sizeof(data->strings) - (data->str_pos - data->strings), format, ap );
    data->str_pos += len;

    if (len && data->str_pos[-1] == '\n')
    {
        ZwWriteFile( okfile, NULL, NULL, NULL, &io, data->strings,
                     strlen( data->strings ), NULL, NULL );
        data->str_pos = data->strings;
    }
}

static inline void WINAPIV kprintf( const char *format, ... ) __WINE_PRINTF_ATTR( 1, 2 );
static inline void WINAPIV kprintf( const char *format, ... )
{
    va_list valist;

    va_start( valist, format );
    kvprintf( format, valist );
    va_end( valist );
}

static inline void WINAPIV winetest_printf( const char *msg, ... ) __WINE_PRINTF_ATTR( 1, 2 );
static inline void WINAPIV winetest_printf( const char *msg, ... )
{
    struct tls_data *data = get_tls_data();
    va_list valist;

    kprintf( "%s:%d: ", data->current_file, data->current_line );
    va_start( valist, msg );
    kvprintf( msg, valist );
    va_end( valist );
}

static inline NTSTATUS winetest_init(void)
{
    const struct winetest_shared_data *data;
    SIZE_T size = sizeof(*data);
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string;
    IO_STATUS_BLOCK io;
    void *addr = NULL;
    HANDLE section;
    NTSTATUS ret;

    KeInitializeSpinLock( &tls_data_lock );

    RtlInitUnicodeString( &string, L"\\BaseNamedObjects\\winetest_dinput_section" );
    /* OBJ_KERNEL_HANDLE is necessary for the file to be accessible from system threads */
    InitializeObjectAttributes( &attr, &string, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, NULL );
    if ((ret = ZwOpenSection( &section, SECTION_MAP_READ, &attr ))) return ret;

    if ((ret = ZwMapViewOfSection( section, NtCurrentProcess(), &addr, 0, 0, NULL, &size, ViewUnmap, 0, PAGE_READONLY )))
    {
        ZwClose( section );
        return ret;
    }
    data = addr;
    running_under_wine = data->running_under_wine;
    winetest_debug = data->winetest_debug;
    winetest_report_success = data->winetest_report_success;

    ZwUnmapViewOfSection( NtCurrentProcess(), addr );
    ZwClose( section );

    RtlInitUnicodeString( &string, L"\\??\\C:\\windows\\winetest_dinput_okfile" );
    return ZwOpenFile( &okfile, FILE_APPEND_DATA | SYNCHRONIZE, &attr, &io,
                       FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_NONALERT );
}

#define winetest_cleanup() winetest_cleanup_( __FILE__ )
static inline void winetest_cleanup_( const char *file )
{
    char test_name[MAX_PATH], *tmp;
    struct winetest_shared_data *data;
    SIZE_T size = sizeof(*data);
    const char *source_file;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string;
    void *addr = NULL;
    HANDLE section;

    source_file = strrchr( file, '/' );
    if (!source_file) source_file = strrchr( file, '\\' );
    if (!source_file) source_file = file;
    else source_file++;

    strcpy( test_name, source_file );
    if ((tmp = strrchr( test_name, '.' ))) *tmp = 0;

    if (winetest_debug)
    {
        kprintf( "%04lx:%s: %ld tests executed (%ld marked as todo, %ld %s), %ld skipped.\n",
                 (DWORD)(DWORD_PTR)PsGetCurrentProcessId(), test_name,
                 successes + failures + todo_successes + todo_failures, todo_successes, failures + todo_failures,
                 (failures + todo_failures != 1) ? "failures" : "failure", skipped );
    }

    RtlInitUnicodeString( &string, L"\\BaseNamedObjects\\winetest_dinput_section" );
    /* OBJ_KERNEL_HANDLE is necessary for the file to be accessible from system threads */
    InitializeObjectAttributes( &attr, &string, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, NULL );

    if (!ZwOpenSection( &section, SECTION_MAP_READ | SECTION_MAP_WRITE, &attr ))
    {
        if (!ZwMapViewOfSection( section, NtCurrentProcess(), &addr, 0, 0, NULL, &size, ViewUnmap, 0, PAGE_READWRITE ))
        {
            data = addr;

            InterlockedExchangeAdd( &data->failures, failures );
            InterlockedExchangeAdd( &data->todo_failures, todo_failures );

            ZwUnmapViewOfSection( NtCurrentProcess(), addr );
        }
        ZwClose( section );
    }

    ZwClose( okfile );
}

static inline void winetest_print_context( const char *msgtype )
{
    struct tls_data *data = get_tls_data();
    unsigned int i;

    winetest_printf( "%s", msgtype );
    for (i = 0; i < data->context_count; ++i) kprintf( "%s: ", data->context[i] );
}

static inline LONG winetest_add_line(void)
{
    struct tls_data *data;
    int index, count;

    if (winetest_debug > 1) return 0;

    data = get_tls_data();
    index = data->current_line % ARRAY_SIZE(line_counters);
    count = InterlockedIncrement( line_counters + index ) - 1;
    if (count == winetest_mute_threshold)
        winetest_printf( "Line has been silenced after %d occurrences\n", winetest_mute_threshold );

    return count;
}

static inline int winetest_vok( int condition, const char *msg, va_list args )
{
    struct tls_data *data = get_tls_data();

    if (data->todo_level)
    {
        if (condition)
        {
            winetest_print_context( "Test succeeded inside todo block: " );
            kvprintf( msg, args );
            InterlockedIncrement( &todo_failures );
            return 0;
        }
        else
        {
            if (!winetest_debug || winetest_add_line() < winetest_mute_threshold)
            {
                if (winetest_debug > 0)
                {
                    winetest_print_context( "Test marked todo: " );
                    kvprintf( msg, args );
                }
                InterlockedIncrement( &todo_successes );
            }
            else InterlockedIncrement( &muted_todo_successes );
            return 1;
        }
    }
    else
    {
        if (!condition)
        {
            winetest_print_context( "Test failed: " );
            kvprintf( msg, args );
            InterlockedIncrement( &failures );
            return 0;
        }
        else
        {
            if (winetest_report_success) winetest_printf( "Test succeeded\n" );
            InterlockedIncrement( &successes );
            return 1;
        }
    }
}

static inline void WINAPIV winetest_ok( int condition, const char *msg, ... ) __WINE_PRINTF_ATTR( 2, 3 );
static inline void WINAPIV winetest_ok( int condition, const char *msg, ... )
{
    va_list args;
    va_start( args, msg );
    winetest_vok( condition, msg, args );
    va_end( args );
}

static inline void winetest_vskip( const char *msg, va_list args )
{
    if (winetest_add_line() < winetest_mute_threshold)
    {
        winetest_print_context( "Driver tests skipped: " );
        kvprintf( msg, args );
        InterlockedIncrement( &skipped );
    }
    else InterlockedIncrement( &muted_skipped );
}

static inline void WINAPIV winetest_skip( const char *msg, ... ) __WINE_PRINTF_ATTR( 1, 2 );
static inline void WINAPIV winetest_skip( const char *msg, ... )
{
    va_list args;
    va_start( args, msg );
    winetest_vskip( msg, args );
    va_end( args );
}

static inline void WINAPIV winetest_win_skip( const char *msg, ... ) __WINE_PRINTF_ATTR( 1, 2 );
static inline void WINAPIV winetest_win_skip( const char *msg, ... )
{
    va_list args;
    va_start( args, msg );
    if (!running_under_wine) winetest_vskip( msg, args );
    else winetest_vok( 0, msg, args );
    va_end( args );
}

static inline void WINAPIV winetest_trace( const char *msg, ... ) __WINE_PRINTF_ATTR( 1, 2 );
static inline void WINAPIV winetest_trace( const char *msg, ... )
{
    va_list args;

    if (!winetest_debug) return;
    if (winetest_add_line() < winetest_mute_threshold)
    {
        winetest_print_context( "" );
        va_start( args, msg );
        kvprintf( msg, args );
        va_end( args );
    }
    else InterlockedIncrement( &muted_traces );
}

static inline void winetest_start_todo( int is_todo )
{
    struct tls_data *data = get_tls_data();
    data->todo_level = (data->todo_level << 1) | (is_todo != 0);
    data->todo_do_loop = 1;
}

static inline int winetest_loop_todo(void)
{
    struct tls_data *data = get_tls_data();
    int do_loop = data->todo_do_loop;
    data->todo_do_loop = 0;
    return do_loop;
}

static inline void winetest_end_todo(void)
{
    struct tls_data *data = get_tls_data();
    data->todo_level >>= 1;
}

static inline void WINAPIV winetest_push_context( const char *fmt, ... ) __WINE_PRINTF_ATTR( 1, 2 );
static inline void WINAPIV winetest_push_context( const char *fmt, ... )
{
    struct tls_data *data = get_tls_data();
    va_list valist;

    if (data->context_count < ARRAY_SIZE(data->context))
    {
        va_start( valist, fmt );
        vsnprintf( data->context[data->context_count], sizeof(data->context[data->context_count]), fmt, valist );
        va_end( valist );
        data->context[data->context_count][sizeof(data->context[data->context_count]) - 1] = 0;
    }
    ++data->context_count;
}

static inline void winetest_pop_context(void)
{
    struct tls_data *data = get_tls_data();

    if (data->context_count) --data->context_count;
}

static inline int broken( int condition )
{
    return !running_under_wine && condition;
}

#ifdef WINETEST_NO_LINE_NUMBERS
# define subtest_(file, line)  (winetest_set_location(file, 0), 0) ? (void)0 : winetest_subtest
# define ignore_exceptions_(file, line)  (winetest_set_location(file, 0), 0) ? (void)0 : winetest_ignore_exceptions
# define ok_(file, line)       (winetest_set_location(file, 0), 0) ? (void)0 : winetest_ok
# define skip_(file, line)     (winetest_set_location(file, 0), 0) ? (void)0 : winetest_skip
# define win_skip_(file, line) (winetest_set_location(file, 0), 0) ? (void)0 : winetest_win_skip
# define trace_(file, line)    (winetest_set_location(file, 0), 0) ? (void)0 : winetest_trace
# define wait_child_process_(file, line) (winetest_set_location(file, 0), 0) ? (void)0 : winetest_wait_child_process
#else
# define subtest_(file, line)  (winetest_set_location(file, line), 0) ? (void)0 : winetest_subtest
# define ignore_exceptions_(file, line)  (winetest_set_location(file, line), 0) ? (void)0 : winetest_ignore_exceptions
# define ok_(file, line)       (winetest_set_location(file, line), 0) ? (void)0 : winetest_ok
# define skip_(file, line)     (winetest_set_location(file, line), 0) ? (void)0 : winetest_skip
# define win_skip_(file, line) (winetest_set_location(file, line), 0) ? (void)0 : winetest_win_skip
# define trace_(file, line)    (winetest_set_location(file, line), 0) ? (void)0 : winetest_trace
# define wait_child_process_(file, line) (winetest_set_location(file, line), 0) ? (void)0 : winetest_wait_child_process
#endif

#define ok       ok_(__FILE__, __LINE__)
#define skip     skip_(__FILE__, __LINE__)
#define trace    trace_(__FILE__, __LINE__)
#define win_skip win_skip_(__FILE__, __LINE__)

#define todo_if(is_todo) for (winetest_start_todo(is_todo); \
                              winetest_loop_todo(); \
                              winetest_end_todo())
#define todo_wine               todo_if(running_under_wine)
#define todo_wine_if(is_todo)   todo_if((is_todo) && running_under_wine)

#endif /* __WINE_WINE_TEST_H */

#endif /* __WINE_DRIVER_HID_H */
