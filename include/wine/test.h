/*
 * Definitions for Wine C unit tests.
 *
 * Copyright (C) 2002 Alexandre Julliard
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

#ifndef __WINE_WINE_TEST_H
#define __WINE_WINE_TEST_H

#include <stdarg.h>
#include <stdlib.h>
#include <io.h>
#include <windef.h>
#include <winbase.h>
#include <wine/debug.h>

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES  (~0u)
#endif
#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER (~0u)
#endif

/* debug level */
extern int winetest_debug;

/* trace timing information */
extern int winetest_time;

/* running in interactive mode? */
extern int winetest_interactive;

/* report failed flaky tests as failures (BOOL) */
extern int winetest_report_flaky;

/* report successful tests (BOOL) */
extern int winetest_report_success;

/* silence todos and skips above this threshold */
extern int winetest_mute_threshold;

/* current platform */
extern const char *winetest_platform;

extern void winetest_set_location( const char* file, int line );
extern void winetest_subtest( const char* name );
extern void winetest_ignore_exceptions( BOOL ignore );
extern void winetest_start_todo( int is_todo );
extern int winetest_loop_todo(void);
extern void winetest_end_todo(void);
extern void winetest_start_flaky( int is_flaky );
extern int winetest_loop_flaky(void);
extern void winetest_end_flaky(void);
extern int winetest_get_mainargs( char*** pargv );
extern LONG winetest_get_failures(void);
extern void winetest_add_failures( LONG new_failures );
extern void winetest_wait_child_process( HANDLE process );

#ifdef STANDALONE
#define START_TEST(name) \
  static void func_##name(void); \
  const struct test winetest_testlist[] = { { #name, func_##name }, { 0, 0 } }; \
  static void func_##name(void)
#else
#define START_TEST(name) void func_##name(void)
#endif

extern int broken( int condition );
extern int winetest_vok( int condition, const char *msg, va_list ap );
extern void winetest_vskip( const char *msg, va_list ap );

extern void winetest_ok( int condition, const char *msg, ... ) __WINE_PRINTF_ATTR(2,3);
extern void winetest_skip( const char *msg, ... ) __WINE_PRINTF_ATTR(1,2);
extern void winetest_win_skip( const char *msg, ... ) __WINE_PRINTF_ATTR(1,2);
extern void winetest_trace( const char *msg, ... ) __WINE_PRINTF_ATTR(1,2);

extern void winetest_push_context( const char *fmt, ... ) __WINE_PRINTF_ATTR(1, 2);
extern void winetest_pop_context(void);

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

#define subtest  subtest_(__FILE__, __LINE__)
#define ignore_exceptions  ignore_exceptions_(__FILE__, __LINE__)
#define ok       ok_(__FILE__, __LINE__)
#define skip     skip_(__FILE__, __LINE__)
#define win_skip win_skip_(__FILE__, __LINE__)
#define trace    trace_(__FILE__, __LINE__)
#define wait_child_process wait_child_process_(__FILE__, __LINE__)

#define flaky_if(is_flaky) for (winetest_start_flaky(is_flaky); \
                                winetest_loop_flaky(); \
                                winetest_end_flaky())
#define flaky                   flaky_if(TRUE)
#define flaky_wine              flaky_if(!strcmp(winetest_platform, "wine"))
#define flaky_wine_if(is_flaky) flaky_if((is_flaky) && !strcmp(winetest_platform, "wine"))

#define todo_if(is_todo) for (winetest_start_todo(is_todo); \
                              winetest_loop_todo(); \
                              winetest_end_todo())
#define todo_wine               todo_if(!strcmp(winetest_platform, "wine"))
#define todo_wine_if(is_todo)   todo_if((is_todo) && !strcmp(winetest_platform, "wine"))


#ifndef ARRAY_SIZE
# define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#ifdef NONAMELESSUNION
# define U(x)  (x).u
# define U1(x) (x).u1
# define U2(x) (x).u2
# define U3(x) (x).u3
# define U4(x) (x).u4
# define U5(x) (x).u5
# define U6(x) (x).u6
# define U7(x) (x).u7
# define U8(x) (x).u8
#else
# define U(x)  (x)
# define U1(x) (x)
# define U2(x) (x)
# define U3(x) (x)
# define U4(x) (x)
# define U5(x) (x)
# define U6(x) (x)
# define U7(x) (x)
# define U8(x) (x)
#endif

#ifdef NONAMELESSSTRUCT
# define S(x)  (x).s
# define S1(x) (x).s1
# define S2(x) (x).s2
# define S3(x) (x).s3
# define S4(x) (x).s4
# define S5(x) (x).s5
#else
# define S(x)  (x)
# define S1(x) (x)
# define S2(x) (x)
# define S3(x) (x)
# define S4(x) (x)
# define S5(x) (x)
#endif


/************************************************************************/
/* Below is the implementation of the various functions, to be included
 * directly into the generated testlist.c file.
 * It is done that way so that the dlls can build the test routines with
 * different includes or flags if needed.
 */

#ifdef STANDALONE

#include <stdio.h>
#include <excpt.h>

struct test
{
    const char *name;
    void (*func)(void);
};

extern const struct test winetest_testlist[];

/* debug level */
int winetest_debug = 1;

/* trace timing information */
int winetest_time = 0;
DWORD winetest_start_time, winetest_last_time;

/* interactive mode? */
int winetest_interactive = 0;

/* current platform */
const char *winetest_platform = "windows";

/* report failed flaky tests as failures (BOOL) */
int winetest_report_flaky = 0;

/* report successful tests (BOOL) */
int winetest_report_success = 0;

/* silence todos and skips above this threshold */
int winetest_mute_threshold = 42;

/* use ANSI escape codes for output coloring */
static int winetest_color;

static HANDLE winetest_mutex;

/* passing arguments around */
static int winetest_argc;
static char** winetest_argv;

static const struct test *current_test; /* test currently being run */

static LONG successes;       /* number of successful tests */
static LONG failures;        /* number of failures */
static LONG flaky_failures;  /* number of failures inside flaky block */
static LONG skipped;         /* number of skipped test chunks */
static LONG todo_successes;  /* number of successful tests inside todo block */
static LONG todo_failures;   /* number of failures inside todo block */
static LONG muted_traces;    /* number of silenced traces */
static LONG muted_skipped;   /* same as skipped but silent */
static LONG muted_todo_successes; /* same as todo_successes but silent */

/* counts how many times a given line printed a message */
static LONG line_counters[16384];

/* The following data must be kept track of on a per-thread basis */
struct tls_data
{
    const char* current_file;        /* file of current check */
    int current_line;                /* line of current check */
    unsigned int flaky_level;        /* current flaky nesting level */
    int flaky_do_loop;
    unsigned int todo_level;         /* current todo nesting level */
    int todo_do_loop;
    char *str_pos;                   /* position in debug buffer */
    char strings[2000];              /* buffer for debug strings */
    char context[8][128];            /* data to print before messages */
    unsigned int context_count;      /* number of context prefixes */
};
static DWORD tls_index;

static struct tls_data *get_tls_data(void)
{
    struct tls_data *data;
    DWORD last_error;

    last_error=GetLastError();
    data=TlsGetValue(tls_index);
    if (!data)
    {
        data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data));
        data->str_pos = data->strings;
        TlsSetValue(tls_index,data);
    }
    SetLastError(last_error);
    return data;
}

static void exit_process( int code )
{
    fflush( stdout );
    ExitProcess( code );
}


void winetest_set_location( const char* file, int line )
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

const char *winetest_elapsed(void)
{
    DWORD now;

    if (!winetest_time) return "";
    winetest_last_time = now = GetTickCount();
    return wine_dbg_sprintf( "%.3f", (now - winetest_start_time) / 1000.0);
}

static const char color_reset[] = "\x1b[0m";
static const char color_dark_red[] = "\x1b[31m";
static const char color_dark_purple[] = "\x1b[35m";
static const char color_green[] = "\x1b[32m";
static const char color_yellow[] = "\x1b[33m";
static const char color_blue[] = "\x1b[34m";
static const char color_bright_red[] = "\x1b[1;91m";
static const char color_bright_purple[] = "\x1b[1;95m";

static void winetest_printf( const char *msg, ... ) __WINE_PRINTF_ATTR(1,2);
static void winetest_printf( const char *msg, ... )
{
    struct tls_data *data = get_tls_data();
    va_list valist;

    printf( "%s:%d:%s ", data->current_file, data->current_line, winetest_elapsed() );
    va_start( valist, msg );
    vprintf( msg, valist );
    va_end( valist );
}
static void winetest_print_context( const char *msgtype )
{
    struct tls_data *data = get_tls_data();
    unsigned int i;

    winetest_printf( "%s", msgtype );
    for (i = 0; i < data->context_count; ++i)
        printf( "%s: ", data->context[i] );
}

static void winetest_print_lock(void)
{
    UINT ret;

    if (!winetest_mutex) return;
    ret = WaitForSingleObject(winetest_mutex, 30000);
    if (ret != WAIT_OBJECT_0 && ret != WAIT_ABANDONED)
    {
        winetest_printf("could not get the print lock: %u\n", ret);
        winetest_mutex = 0;
    }
}

static void winetest_print_unlock(void)
{
    if (winetest_mutex) ReleaseMutex(winetest_mutex);
}

void winetest_subtest( const char* name )
{
    winetest_print_lock();
    winetest_printf( "Subtest %s\n", name );
    winetest_print_unlock();
}

void winetest_ignore_exceptions( BOOL ignore )
{
    winetest_print_lock();
    winetest_printf( "IgnoreExceptions=%d\n", ignore ? 1 : 0 );
    winetest_print_unlock();
}

int broken( int condition )
{
    return (strcmp(winetest_platform, "windows") == 0) && condition;
}

static LONG winetest_add_line( void )
{
    struct tls_data *data;
    int index, count;

    if (winetest_debug > 1)
        return 0;

    data = get_tls_data();
    index = data->current_line % ARRAY_SIZE(line_counters);
    count = InterlockedIncrement(line_counters + index) - 1;
    if (count == winetest_mute_threshold)
    {
        winetest_print_lock();
        winetest_printf( "Line has been silenced after %d occurrences\n", winetest_mute_threshold );
        winetest_print_unlock();
    }

    return count;
}

/*
 * Checks condition.
 * Parameters:
 *   - condition - condition to check;
 *   - msg test description;
 *   - file - test application source code file name of the check
 *   - line - test application source code file line number of the check
 * Return:
 *   0 if condition does not have the expected value, 1 otherwise
 */
int winetest_vok( int condition, const char *msg, va_list args )
{
    struct tls_data *data = get_tls_data();

    if (data->todo_level)
    {
        if (condition)
        {
            winetest_print_lock();
            if (data->flaky_level)
            {
                if (winetest_color) printf( color_dark_purple );
                winetest_print_context( "Test succeeded inside flaky todo block: " );
                vprintf(msg, args);
                InterlockedIncrement(&flaky_failures);
            }
            else
            {
                if (winetest_color) printf( color_dark_red );
                winetest_print_context( "Test succeeded inside todo block: " );
                vprintf(msg, args);
                InterlockedIncrement(&todo_failures);
            }
            if (winetest_color) printf( color_reset );
            winetest_print_unlock();
            return 0;
        }
        else
        {
            if (!winetest_debug ||
                winetest_add_line() < winetest_mute_threshold)
            {
                if (winetest_debug > 0)
                {
                    winetest_print_lock();
                    if (winetest_color) printf( color_yellow );
                    winetest_print_context( "Test marked todo: " );
                    vprintf(msg, args);
                    if (winetest_color) printf( color_reset );
                    winetest_print_unlock();
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
            winetest_print_lock();
            if (data->flaky_level)
            {
                if (winetest_color) printf( color_bright_purple );
                winetest_print_context( "Test marked flaky: " );
                vprintf(msg, args);
                InterlockedIncrement(&flaky_failures);
            }
            else
            {
                if (winetest_color) printf( color_bright_red );
                winetest_print_context( "Test failed: " );
                vprintf(msg, args);
                InterlockedIncrement(&failures);
            }
            if (winetest_color) printf( color_reset );
            winetest_print_unlock();
            return 0;
        }
        else
        {
            if (winetest_report_success ||
                (winetest_time && GetTickCount() >= winetest_last_time + 1000))
            {
                winetest_print_lock();
                if (winetest_color) printf( color_green );
                winetest_printf("Test succeeded\n");
                if (winetest_color) printf( color_reset );
                winetest_print_unlock();
            }
            InterlockedIncrement(&successes);
            return 1;
        }
    }
}

void winetest_ok( int condition, const char *msg, ... )
{
    va_list valist;

    va_start(valist, msg);
    winetest_vok(condition, msg, valist);
    va_end(valist);
}

void winetest_trace( const char *msg, ... )
{
    va_list valist;

    if (!winetest_debug)
        return;
    if (winetest_add_line() < winetest_mute_threshold)
    {
        winetest_print_lock();
        winetest_print_context( "" );
        va_start(valist, msg);
        vprintf( msg, valist );
        va_end(valist);
        winetest_print_unlock();
    }
    else
        InterlockedIncrement(&muted_traces);
}

void winetest_vskip( const char *msg, va_list args )
{
    if (winetest_add_line() < winetest_mute_threshold)
    {
        winetest_print_lock();
        if (winetest_color) printf( color_blue );
        winetest_print_context( "Tests skipped: " );
        vprintf(msg, args);
        if (winetest_color) printf( color_reset );
        winetest_print_unlock();
        InterlockedIncrement(&skipped);
    }
    else
        InterlockedIncrement(&muted_skipped);
}

void winetest_skip( const char *msg, ... )
{
    va_list valist;
    va_start(valist, msg);
    winetest_vskip(msg, valist);
    va_end(valist);
}

void winetest_win_skip( const char *msg, ... )
{
    va_list valist;
    va_start(valist, msg);
    if (strcmp(winetest_platform, "windows") == 0)
        winetest_vskip(msg, valist);
    else
        winetest_vok(0, msg, valist);
    va_end(valist);
}

void winetest_start_flaky( int is_flaky )
{
    struct tls_data *data = get_tls_data();
    data->flaky_level = (data->flaky_level << 1) | (is_flaky != 0);
    data->flaky_do_loop = 1;
}

int winetest_loop_flaky(void)
{
    struct tls_data *data = get_tls_data();
    int do_flaky = data->flaky_do_loop;
    data->flaky_do_loop = 0;
    return do_flaky;
}

void winetest_end_flaky(void)
{
    struct tls_data *data = get_tls_data();
    data->flaky_level >>= 1;
}

void winetest_start_todo( int is_todo )
{
    struct tls_data *data = get_tls_data();
    data->todo_level = (data->todo_level << 1) | (is_todo != 0);
    data->todo_do_loop=1;
}

int winetest_loop_todo(void)
{
    struct tls_data *data = get_tls_data();
    int do_loop=data->todo_do_loop;
    data->todo_do_loop=0;
    return do_loop;
}

void winetest_end_todo(void)
{
    struct tls_data *data = get_tls_data();
    data->todo_level >>= 1;
}

void winetest_push_context( const char *fmt, ... )
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

void winetest_pop_context(void)
{
    struct tls_data *data = get_tls_data();

    if (data->context_count)
        --data->context_count;
}

int winetest_get_mainargs( char*** pargv )
{
    *pargv = winetest_argv;
    return winetest_argc;
}

LONG winetest_get_failures(void)
{
    return failures;
}

void winetest_add_failures( LONG new_failures )
{
    while (new_failures-- > 0)
        InterlockedIncrement( &failures );
}

void winetest_wait_child_process( HANDLE process )
{
    DWORD ret;

    winetest_ok( process != NULL, "No child process handle (CreateProcess failed?)\n" );
    if (!process) return;

    ret = WaitForSingleObject( process, 30000 );
    if (ret == WAIT_TIMEOUT)
        winetest_ok( 0, "Timed out waiting for the child process\n" );
    else if (ret != WAIT_OBJECT_0)
        winetest_ok( 0, "Could not wait for the child process: %d le=%u\n",
                     (UINT)ret, (UINT)GetLastError() );
    else
    {
        DWORD exit_code;
        GetExitCodeProcess( process, &exit_code );
        if (exit_code > 255)
        {
            DWORD pid = GetProcessId( process );
            winetest_print_lock();
            if (winetest_color) printf( color_bright_red );
            winetest_printf( "unhandled exception %08x in child process %04x\n", (UINT)exit_code, (UINT)pid );
            if (winetest_color) printf( color_reset );
            winetest_print_unlock();
            InterlockedIncrement( &failures );
        }
        else if (exit_code)
        {
            winetest_print_lock();
            winetest_printf( "%u failures in child process\n", (UINT)exit_code );
            winetest_print_unlock();
            while (exit_code-- > 0)
                InterlockedIncrement(&failures);
        }
    }
}

/* Find a test by name */
static const struct test *find_test( const char *name )
{
    const struct test *test;
    const char *p;
    size_t len;

    if ((p = strrchr( name, '/' ))) name = p + 1;
    if ((p = strrchr( name, '\\' ))) name = p + 1;
    len = strlen(name);
    if (len > 2 && !strcmp( name + len - 2, ".c" )) len -= 2;

    for (test = winetest_testlist; test->name; test++)
    {
        if (!strncmp( test->name, name, len ) && !test->name[len]) break;
    }
    return test->name ? test : NULL;
}


/* Display list of valid tests */
static void list_tests(void)
{
    const struct test *test;

    printf( "Valid test names:\n" );
    for (test = winetest_testlist; test->name; test++)
        printf( "    %s\n", test->name );
}


/* Run a named test, and return exit status */
static int run_test( const char *name )
{
    const struct test *test;
    int status;

    if (!(test = find_test( name )))
    {
        printf( "Fatal: test '%s' does not exist.\n", name );
        exit_process(1);
    }
    tls_index=TlsAlloc();
    current_test = test;
    test->func();

    if (winetest_debug)
    {
        winetest_print_lock();
        if (muted_todo_successes || muted_skipped || muted_traces)
            printf( "%04x:%s:%s Silenced %d todos, %d skips and %d traces.\n",
                    (UINT)GetCurrentProcessId(), test->name, winetest_elapsed(),
                    (UINT)muted_todo_successes, (UINT)muted_skipped, (UINT)muted_traces);
        printf( "%04x:%s:%s %d tests executed (%d marked as todo, %d as flaky, %d %s), %d skipped.\n",
                (UINT)GetCurrentProcessId(), test->name, winetest_elapsed(),
                (UINT)(successes + failures + flaky_failures + todo_successes + todo_failures),
                (UINT)todo_successes, (UINT)flaky_failures, (UINT)(failures + todo_failures),
                (failures + todo_failures != 1) ? "failures" : "failure",
                (UINT)skipped );
        winetest_print_unlock();
    }
    status = failures + todo_failures;
    if (winetest_report_flaky) status += flaky_failures;
    if (status > 255) status = 255;
    return status;
}


/* Display usage and exit */
static void usage( const char *argv0 )
{
    printf( "Usage: %s test_name\n\n", argv0 );
    list_tests();
    exit_process(1);
}

/* trap unhandled exceptions */
static LONG CALLBACK exc_filter( EXCEPTION_POINTERS *ptrs )
{
    struct tls_data *data = get_tls_data();

    winetest_print_lock();
    if (data->current_file)
        printf( "%s:%d: this is the last test seen before the exception\n",
                data->current_file, data->current_line );
    if (winetest_color) printf( color_bright_red );
    printf( "%04x:%s:%s unhandled exception %08x at %p\n",
            (UINT)GetCurrentProcessId(), current_test->name, winetest_elapsed(),
            (UINT)ptrs->ExceptionRecord->ExceptionCode, ptrs->ExceptionRecord->ExceptionAddress );
    if (winetest_color) printf( color_reset );
    fflush( stdout );
    winetest_print_unlock();
    return EXCEPTION_EXECUTE_HANDLER;
}

/* check if we're running under wine */
static BOOL running_under_wine(void)
{
    HMODULE module = GetModuleHandleA( "ntdll.dll" );
    if (!module) return FALSE;
    return (GetProcAddress( module, "wine_server_call" ) != NULL);
}

#ifdef __GNUC__
void _fpreset(void) {} /* override the mingw fpu init code */
#endif

/* main function */
int main( int argc, char **argv )
{
    char p[128];

    setvbuf (stdout, NULL, _IONBF, 0);
    winetest_mutex = CreateMutexA(NULL, FALSE, "winetest_print_mutex");

    winetest_argc = argc;
    winetest_argv = argv;

    if (GetEnvironmentVariableA( "WINETEST_PLATFORM", p, sizeof(p) ))
        winetest_platform = strdup(p);
    else if (running_under_wine())
        winetest_platform = "wine";

    if (GetEnvironmentVariableA( "WINETEST_COLOR", p, sizeof(p) ))
    {
        BOOL automode = !strcmp(p, "auto");
        winetest_color = automode ? isatty( fileno( stdout ) ) : atoi(p);
        /* enable ANSI support for Windows console */
        if (winetest_color)
        {
            HANDLE hOutput = (HANDLE)_get_osfhandle( fileno( stdout ) );
            DWORD mode;
            if (GetConsoleMode( hOutput, &mode ) &&
                !SetConsoleMode( hOutput, mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING ) &&
                automode)
                winetest_color = 0;
        }
    }
    if (GetEnvironmentVariableA( "WINETEST_DEBUG", p, sizeof(p) )) winetest_debug = atoi(p);
    if (GetEnvironmentVariableA( "WINETEST_INTERACTIVE", p, sizeof(p) )) winetest_interactive = atoi(p);
    if (GetEnvironmentVariableA( "WINETEST_REPORT_FLAKY", p, sizeof(p) )) winetest_report_flaky = atoi(p);
    if (GetEnvironmentVariableA( "WINETEST_REPORT_SUCCESS", p, sizeof(p) )) winetest_report_success = atoi(p);
    if (GetEnvironmentVariableA( "WINETEST_TIME", p, sizeof(p) )) winetest_time = atoi(p);
    winetest_last_time = winetest_start_time = GetTickCount();

    if (!strcmp( winetest_platform, "windows" )) SetUnhandledExceptionFilter( exc_filter );
    if (!winetest_interactive) SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX );

    if (!argv[1])
    {
        if (winetest_testlist[0].name && !winetest_testlist[1].name)  /* only one test */
            return run_test( winetest_testlist[0].name );
        usage( argv[0] );
    }
    if (!strcmp( argv[1], "--list" ))
    {
        list_tests();
        return 0;
    }
    return run_test(argv[1]);
}

#endif  /* STANDALONE */

#endif  /* __WINE_WINE_TEST_H */
