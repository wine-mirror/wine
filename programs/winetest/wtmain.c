/*
 * Main routine for Wine C unit tests.
 *
 * Copyright 2002 Alexandre Julliard
 * Copyright 2002 Andriy Palamarchuk
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wine/test.h"
#include "winbase.h"

/* debug level */
int winetest_debug = 1;

/* current platform */
const char *winetest_platform = "windows";

/* report successful tests (BOOL) */
int winetest_report_success = 0;

struct test
{
    const char  *name;
    void       (*func)(void);
};

extern const struct test winetest_testlist[];
static const struct test *current_test; /* test currently being run */

static LONG successes;       /* number of successful tests */
static LONG failures;        /* number of failures */
static LONG todo_successes;  /* number of successful tests inside todo block */
static LONG todo_failures;   /* number of failures inside todo block */

/* The following data must be kept track of on a per-thread basis */
typedef struct
{
    const char* current_file;        /* file of current check */
    int current_line;                /* line of current check */
    int todo_level;                  /* current todo nesting level */
    int todo_do_loop;
} tls_data;
static DWORD tls_index;

static tls_data* get_tls_data(void)
{
    tls_data* data;
    DWORD last_error;

    last_error=GetLastError();
    data=TlsGetValue(tls_index);
    if (!data)
    {
        data=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(tls_data));
        TlsSetValue(tls_index,data);
    }
    SetLastError(last_error);
    return data;
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
int winetest_ok( int condition, const char *msg, ... )
{
    va_list valist;
    tls_data* data=get_tls_data();

    if (data->todo_level)
    {
        if (condition)
        {
            fprintf( stderr, "%s:%d: Test succeeded inside todo block",
                     data->current_file, data->current_line );
            if (msg && msg[0])
            {
                va_start(valist, msg);
                fprintf(stderr,": ");
                vfprintf(stderr, msg, valist);
                va_end(valist);
            }
            fputc( '\n', stderr );
            InterlockedIncrement(&todo_failures);
            return 0;
        }
        else InterlockedIncrement(&todo_successes);
    }
    else
    {
        if (!condition)
        {
            fprintf( stderr, "%s:%d: Test failed",
                     data->current_file, data->current_line );
            if (msg && msg[0])
            {
                va_start(valist, msg);
                fprintf( stderr,": ");
                vfprintf(stderr, msg, valist);
                va_end(valist);
            }
            fputc( '\n', stderr );
            InterlockedIncrement(&failures);
            return 0;
        }
        else
        {
            if( winetest_report_success)
                fprintf( stderr, "%s:%d: Test succeeded\n",
                         data->current_file, data->current_line);
            InterlockedIncrement(&successes);
        }
    }
    return 1;
}

winetest_ok_funcptr winetest_set_ok_location( const char* file, int line )
{
    tls_data* data=get_tls_data();
    data->current_file=file;
    data->current_line=line;
    return &winetest_ok;
}

void winetest_trace( const char *msg, ... )
{
    va_list valist;
    tls_data* data=get_tls_data();

    if (winetest_debug > 0)
    {
        fprintf( stderr, "%s:%d:", data->current_file, data->current_line );
        va_start(valist, msg);
        vfprintf(stderr, msg, valist);
        va_end(valist);
    }
}

winetest_trace_funcptr winetest_set_trace_location( const char* file, int line )
{
    tls_data* data=get_tls_data();
    data->current_file=file;
    data->current_line=line;
    return &winetest_trace;
}

void winetest_start_todo( const char* platform )
{
    tls_data* data=get_tls_data();
    if (strcmp(winetest_platform,platform)==0)
        data->todo_level++;
    data->todo_do_loop=1;
}

int winetest_loop_todo(void)
{
    tls_data* data=get_tls_data();
    int do_loop=data->todo_do_loop;
    data->todo_do_loop=0;
    return do_loop;
}

void winetest_end_todo( const char* platform )
{
    if (strcmp(winetest_platform,platform)==0)
    {
        tls_data* data=get_tls_data();
        data->todo_level--;
    }
}

/* Find a test by name */
static const struct test *find_test( const char *name )
{
    const struct test *test;
    const char *p;
    int len;

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


/* Run a named test, and return exit status */
static int run_test( const char *name )
{
    const struct test *test;
    int status;

    if (!(test = find_test( name )))
    {
        fprintf( stderr, "Fatal: test '%s' does not exist.\n", name );
        ExitProcess(1);
    }
    successes = failures = todo_successes = todo_failures = 0;
    tls_index=TlsAlloc();
    current_test = test;
    test->func();

    if (winetest_debug)
    {
        fprintf( stderr, "%s: %ld tests executed, %ld marked as todo, %ld %s.\n",
                 name, successes + failures + todo_successes + todo_failures,
                 todo_successes, failures + todo_failures,
                 (failures + todo_failures != 1) ? "failures" : "failure" );
    }
    status = (failures + todo_failures < 255) ? failures + todo_failures : 255;
    return status;
}


/* main function */
int main( int argc, char **argv )
{
    char *p;

    if ((p = getenv( "WINETEST_PLATFORM" ))) winetest_platform = p;
    if ((p = getenv( "WINETEST_DEBUG" ))) winetest_debug = atoi(p);
    if ((p = getenv( "WINETEST_REPORT_SUCCESS"))) winetest_report_success = \
                atoi(p);
    if (!argv[1])
    {
        fprintf( stderr, "Usage: %s test_name\n", argv[0] );
        ExitProcess(1);
    }
    ExitProcess( run_test(argv[1]) );
}
