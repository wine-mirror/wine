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

#include "wine/test.h"

DEFINE_GUID(control_class,0xdeadbeef,0x29ef,0x4538,0xa5,0xfd,0xb6,0x95,0x73,0xa3,0x62,0xc0);

#define IOCTL_WINETEST_HID_SET_EXPECT    CTL_CODE(FILE_DEVICE_KEYBOARD, 0x800, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_HID_WAIT_EXPECT   CTL_CODE(FILE_DEVICE_KEYBOARD, 0x801, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_HID_SEND_INPUT    CTL_CODE(FILE_DEVICE_KEYBOARD, 0x802, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_HID_SET_CONTEXT   CTL_CODE(FILE_DEVICE_KEYBOARD, 0x803, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_CREATE_DEVICE     CTL_CODE(FILE_DEVICE_KEYBOARD, 0x804, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_REMOVE_DEVICE     CTL_CODE(FILE_DEVICE_KEYBOARD, 0x805, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_HID_WAIT_INPUT    CTL_CODE(FILE_DEVICE_KEYBOARD, 0x806, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

struct hid_expect
{
    DWORD code;
    DWORD ret_length;
    DWORD ret_status;
    BYTE todo;   /* missing on wine */
    BYTE broken_id; /* different or missing (-1) report on some win versions */
    BYTE wine_only;
    BYTE report_id;
    BYTE report_len;
    BYTE report_buf[128];
};

#define EXPECT_QUEUE_BUFFER_SIZE (64 * sizeof(struct hid_expect))

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
    LONG successes;
    LONG failures;
    LONG todo_successes;
    LONG todo_failures;
    LONG skipped;
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
    case IOCTL_WINETEST_HID_WAIT_INPUT: return "IOCTL_WINETEST_HID_WAIT_INPUT";
    case IOCTL_WINETEST_HID_SET_CONTEXT: return "IOCTL_WINETEST_HID_SET_CONTEXT";
    case IOCTL_WINETEST_CREATE_DEVICE: return "IOCTL_WINETEST_CREATE_DEVICE";
    case IOCTL_WINETEST_REMOVE_DEVICE: return "IOCTL_WINETEST_REMOVE_DEVICE";
    default: return "unknown";
    }
}

#ifdef WINE_DRIVER_TEST

static HANDLE okfile;

LONG winetest_successes = 0;
LONG winetest_failures = 0;
LONG winetest_flaky_failures = 0;
LONG winetest_skipped = 0;
LONG winetest_todo_successes = 0;
LONG winetest_todo_failures = 0;
LONG winetest_muted_traces = 0;
LONG winetest_muted_skipped = 0;
LONG winetest_muted_todo_successes = 0;

const char *winetest_platform;
int winetest_platform_is_wine;
int winetest_debug;
int winetest_report_success;
int winetest_color = 0;
int winetest_time = 0;
int winetest_start_time, winetest_last_time;

/* silence todos and skips above this threshold */
int winetest_mute_threshold = 42;

static KSPIN_LOCK tls_data_lock;
static struct winetest_thread_data tls_data_pool[128];
static HANDLE tls_data_thread[ARRAY_SIZE(tls_data_pool)];
static DWORD tls_data_count;

struct winetest_thread_data *winetest_get_thread_data(void)
{
    HANDLE thread = PsGetCurrentThreadId();
    struct winetest_thread_data *data;
    KIRQL irql;
    UINT i;

    KeAcquireSpinLock( &tls_data_lock, &irql );

    for (i = 0; i < tls_data_count; i++) if (tls_data_thread[i] == thread) break;
    data = tls_data_pool + i;

    if (tls_data_thread[i] != thread)
    {
        tls_data_count = min(tls_data_count + 1, ARRAY_SIZE(tls_data_pool));
        tls_data_thread[i] = thread;
        data->str_pos = data->strings;
    }

    KeReleaseSpinLock( &tls_data_lock, irql );

    return data;
}

void winetest_print_lock(void)
{
}

void winetest_print_unlock(void)
{
}

int winetest_vprintf( const char *format, va_list args )
{
    struct winetest_thread_data *data = winetest_get_thread_data();
    IO_STATUS_BLOCK io;
    int len;

    len = vsnprintf( data->str_pos, sizeof(data->strings) - (data->str_pos - data->strings), format, args );
    data->str_pos += len;

    if (len && data->str_pos[-1] == '\n')
    {
        ZwWriteFile( okfile, NULL, NULL, NULL, &io, data->strings,
                     strlen( data->strings ), NULL, NULL );
        data->str_pos = data->strings;
    }

    return len;
}

int winetest_get_time(void)
{
    return 0;
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
    winetest_platform_is_wine = data->running_under_wine;
    winetest_platform = winetest_platform_is_wine ? "wine" : "windows";
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
        winetest_printf( "%04lx:%s: %ld tests executed (%ld marked as todo, 0 as flaky, %ld %s), %ld skipped.\n",
                         (DWORD)(DWORD_PTR)PsGetCurrentProcessId(), test_name,
                         winetest_successes + winetest_failures + winetest_todo_successes + winetest_todo_failures,
                         winetest_todo_successes, winetest_failures + winetest_todo_failures,
                         (winetest_failures + winetest_todo_failures != 1) ? "failures" : "failure", winetest_skipped );
    }

    RtlInitUnicodeString( &string, L"\\BaseNamedObjects\\winetest_dinput_section" );
    /* OBJ_KERNEL_HANDLE is necessary for the file to be accessible from system threads */
    InitializeObjectAttributes( &attr, &string, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, NULL );

    if (!ZwOpenSection( &section, SECTION_MAP_READ | SECTION_MAP_WRITE, &attr ))
    {
        if (!ZwMapViewOfSection( section, NtCurrentProcess(), &addr, 0, 0, NULL, &size, ViewUnmap, 0, PAGE_READWRITE ))
        {
            data = addr;

            InterlockedExchangeAdd( &data->successes, winetest_successes );
            InterlockedExchangeAdd( &data->failures, winetest_failures );
            InterlockedExchangeAdd( &data->todo_successes, winetest_todo_successes );
            InterlockedExchangeAdd( &data->todo_failures, winetest_todo_failures );
            InterlockedExchangeAdd( &data->skipped, winetest_skipped );

            ZwUnmapViewOfSection( NtCurrentProcess(), addr );
        }
        ZwClose( section );
    }

    ZwClose( okfile );
}

#endif /* WINE_DRIVER_TEST */

#endif /* __WINE_DRIVER_HID_H */
