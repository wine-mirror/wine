/*
 * ntoskrnl.exe testing framework
 *
 * Copyright 2015 Sebastian Lackner
 * Copyright 2015 Michael Müller
 * Copyright 2015 Christian Costa
 * Copyright 2020 Paul Gofman for CodeWeavers
 * Copyright 2020-2021 Zebediah Figura for CodeWeavers
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
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <winternl.h>

#include <ddk/wdm.h>

#if !defined(__WINE_USE_MSVCRT) || defined(__MINGW32__)
#define __WINE_PRINTF_ATTR(fmt,args) __attribute__((format (printf,fmt,args)))
#else
#define __WINE_PRINTF_ATTR(fmt,args)
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
    const char* current_file;        /* file of current check */
    int current_line;                /* line of current check */
    unsigned int todo_level;         /* current todo nesting level */
    int todo_do_loop;
    char *str_pos;                   /* position in debug buffer */
    char strings[2000];              /* buffer for debug strings */
    char context[8][128];            /* data to print before messages */
    unsigned int context_count;      /* number of context prefixes */
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

    KeAcquireSpinLock(&tls_data_lock, &irql);
    for (data = tls_data_pool; data != tls_data_pool + tls_data_count; ++data)
        if (data->thread == thread) break;
    if (data == tls_data_pool + ARRAY_SIZE(tls_data_pool))
        data = &tls_overflow;
    else if (data == tls_data_pool + tls_data_count)
    {
        data->thread = thread;
        data->str_pos = data->strings;
        tls_data_count++;
    }
    KeReleaseSpinLock(&tls_data_lock, irql);

    return data;
}

static inline void winetest_set_location( const char* file, int line )
{
    struct tls_data *data = get_tls_data();
    data->current_file=strrchr(file,'/');
    if (data->current_file==NULL)
        data->current_file=strrchr(file,'\\');
    if (data->current_file==NULL)
        data->current_file=file;
    else
        data->current_file++;
    data->current_line=line;
}

static inline void kvprintf(const char *format, va_list ap)
{
    struct tls_data *data = get_tls_data();
    IO_STATUS_BLOCK io;
    int len = vsnprintf(data->strings, sizeof(data->strings), format, ap);
    ZwWriteFile(okfile, NULL, NULL, NULL, &io, data->strings, len, NULL, NULL);
}

static inline void WINAPIV kprintf(const char *format, ...) __WINE_PRINTF_ATTR(1,2);
static inline void WINAPIV kprintf(const char *format, ...)
{
    va_list valist;

    va_start(valist, format);
    kvprintf(format, valist);
    va_end(valist);
}

static inline void WINAPIV winetest_printf( const char *msg, ... ) __WINE_PRINTF_ATTR(1,2);
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
    const struct test_data *data;
    SIZE_T size = sizeof(*data);
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string;
    IO_STATUS_BLOCK io;
    void *addr = NULL;
    HANDLE section;
    NTSTATUS ret;

    KeInitializeSpinLock(&tls_data_lock);

    RtlInitUnicodeString(&string, L"\\BaseNamedObjects\\winetest_ntoskrnl_section");
    /* OBJ_KERNEL_HANDLE is necessary for the file to be accessible from system threads */
    InitializeObjectAttributes(&attr, &string, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, NULL);
    if ((ret = ZwOpenSection(&section, SECTION_MAP_READ, &attr)))
        return ret;

    if ((ret = ZwMapViewOfSection(section, NtCurrentProcess(), &addr,
            0, 0, NULL, &size, ViewUnmap, 0, PAGE_READONLY)))
    {
        ZwClose(section);
        return ret;
    }
    data = addr;
    running_under_wine = data->running_under_wine;
    winetest_debug = data->winetest_debug;
    winetest_report_success = data->winetest_report_success;

    ZwUnmapViewOfSection(NtCurrentProcess(), addr);
    ZwClose(section);

    RtlInitUnicodeString(&string, L"\\??\\C:\\windows\\winetest_ntoskrnl_okfile");
    return ZwOpenFile(&okfile, FILE_APPEND_DATA | SYNCHRONIZE, &attr, &io,
            FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_NONALERT);
}

static inline void winetest_cleanup(void)
{
    struct test_data *data;
    SIZE_T size = sizeof(*data);
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string;
    void *addr = NULL;
    HANDLE section;

    if (winetest_debug)
    {
        kprintf("%04lx:ntoskrnl: %ld tests executed (%ld marked as todo, %ld %s), %ld skipped.\n",
                (DWORD)(DWORD_PTR)PsGetCurrentProcessId(), successes + failures + todo_successes + todo_failures,
                todo_successes, failures + todo_failures,
                (failures + todo_failures != 1) ? "failures" : "failure", skipped );
    }

    RtlInitUnicodeString(&string, L"\\BaseNamedObjects\\winetest_ntoskrnl_section");
    /* OBJ_KERNEL_HANDLE is necessary for the file to be accessible from system threads */
    InitializeObjectAttributes(&attr, &string, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, NULL);

    if (!ZwOpenSection(&section, SECTION_MAP_READ | SECTION_MAP_WRITE, &attr))
    {
        if (!ZwMapViewOfSection(section, NtCurrentProcess(), &addr,
                0, 0, NULL, &size, ViewUnmap, 0, PAGE_READWRITE))
        {
            data = addr;

            InterlockedExchangeAdd(&data->failures, failures);
            InterlockedExchangeAdd(&data->todo_failures, todo_failures);

            ZwUnmapViewOfSection(NtCurrentProcess(), addr);
        }
        ZwClose(section);
    }

    ZwClose(okfile);
}

static inline void winetest_print_context( const char *msgtype )
{
    struct tls_data *data = get_tls_data();
    unsigned int i;

    winetest_printf( "%s", msgtype );
    for (i = 0; i < data->context_count; ++i)
        kprintf( "%s: ", data->context[i] );
}

static inline LONG winetest_add_line( void )
{
    struct tls_data *data;
    int index, count;

    if (winetest_debug > 1)
        return 0;

    data = get_tls_data();
    index = data->current_line % ARRAY_SIZE(line_counters);
    count = InterlockedIncrement(line_counters + index) - 1;
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
            kvprintf(msg, args);
            InterlockedIncrement(&todo_failures);
            return 0;
        }
        else
        {
            if (!winetest_debug ||
                winetest_add_line() < winetest_mute_threshold)
            {
                if (winetest_debug > 0)
                {
                    winetest_print_context( "Test marked todo: " );
                    kvprintf(msg, args);
                }
                InterlockedIncrement(&todo_successes);
            }
            else
                InterlockedIncrement(&muted_todo_successes);
            return 1;
        }
    }
    else
    {
        if (!condition)
        {
            winetest_print_context( "Test failed: " );
            kvprintf(msg, args);
            InterlockedIncrement(&failures);
            return 0;
        }
        else
        {
            if (winetest_report_success)
                winetest_printf("Test succeeded\n");
            InterlockedIncrement(&successes);
            return 1;
        }
    }
}

static inline void WINAPIV winetest_ok( int condition, const char *msg, ... ) __WINE_PRINTF_ATTR(2,3);
static inline void WINAPIV winetest_ok( int condition, const char *msg, ... )
{
    va_list args;
    va_start(args, msg);
    winetest_vok(condition, msg, args);
    va_end(args);
}

static inline void winetest_vskip( const char *msg, va_list args )
{
    if (winetest_add_line() < winetest_mute_threshold)
    {
        winetest_print_context( "Driver tests skipped: " );
        kvprintf(msg, args);
        InterlockedIncrement(&skipped);
    }
    else
        InterlockedIncrement(&muted_skipped);
}

static inline void WINAPIV winetest_skip( const char *msg, ... ) __WINE_PRINTF_ATTR(1,2);
static inline void WINAPIV winetest_skip( const char *msg, ... )
{
    va_list args;
    va_start(args, msg);
    winetest_vskip(msg, args);
    va_end(args);
}

static inline void WINAPIV winetest_win_skip( const char *msg, ... ) __WINE_PRINTF_ATTR(1,2);
static inline void WINAPIV winetest_win_skip( const char *msg, ... )
{
    va_list args;
    va_start(args, msg);
    if (!running_under_wine)
        winetest_vskip(msg, args);
    else
        winetest_vok(0, msg, args);
    va_end(args);
}

static inline void WINAPIV winetest_trace( const char *msg, ... ) __WINE_PRINTF_ATTR(1,2);
static inline void WINAPIV winetest_trace( const char *msg, ... )
{
    va_list args;

    if (!winetest_debug)
        return;
    if (winetest_add_line() < winetest_mute_threshold)
    {
        winetest_print_context( "" );
        va_start(args, msg);
        kvprintf( msg, args );
        va_end(args);
    }
    else
        InterlockedIncrement(&muted_traces);
}

static inline void winetest_start_todo( int is_todo )
{
    struct tls_data *data = get_tls_data();
    data->todo_level = (data->todo_level << 1) | (is_todo != 0);
    data->todo_do_loop=1;
}

static inline int winetest_loop_todo(void)
{
    struct tls_data *data = get_tls_data();
    int do_loop=data->todo_do_loop;
    data->todo_do_loop=0;
    return do_loop;
}

static inline void winetest_end_todo(void)
{
    struct tls_data *data = get_tls_data();
    data->todo_level >>= 1;
}

static inline void WINAPIV winetest_push_context( const char *fmt, ... ) __WINE_PRINTF_ATTR(1, 2);
static inline void WINAPIV winetest_push_context( const char *fmt, ... )
{
    struct tls_data *data = get_tls_data();
    va_list valist;

    if (data->context_count < ARRAY_SIZE(data->context))
    {
        va_start(valist, fmt);
        vsnprintf( data->context[data->context_count], sizeof(data->context[data->context_count]), fmt, valist );
        va_end(valist);
        data->context[data->context_count][sizeof(data->context[data->context_count]) - 1] = 0;
    }
    ++data->context_count;
}

static inline void winetest_pop_context(void)
{
    struct tls_data *data = get_tls_data();

    if (data->context_count)
        --data->context_count;
}

static inline int broken(int condition)
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
