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

/* debug level */
int winetest_debug = 1;

/* current platform */
const char *winetest_platform = "windows";

struct test
{
    const char  *name;
    void       (*func)(void);
};

extern const struct test winetest_testlist[];
static const struct test *current_test; /* test currently being run */
/* FIXME: Access to all the following variables must be protected in a 
 * multithread test. Either via thread local storage or via critical sections
 */
static const char* current_file;        /* file of current check */
static int current_line;                /* line of current check */

static int successes;         /* number of successful tests */
static int failures;          /* number of failures */
static int todo_successes;    /* number of successful tests inside todo block */
static int todo_failures;     /* number of failures inside todo block */
static int todo_level;        /* current todo nesting level */
static int todo_do_loop;

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

    if (todo_level)
    {
        if (condition)
        {
            fprintf( stderr, "%s:%d: Test succeeded inside todo block",
                     current_file, current_line );
            if (msg && msg[0])
            {
                va_start(valist, msg);
                fprintf(stderr,": ");
                vfprintf(stderr, msg, valist);
                va_end(valist);
            }
            fputc( '\n', stderr );
            todo_failures++;
            return 0;
        }
        else todo_successes++;
    }
    else
    {
        if (!condition)
        {
            fprintf( stderr, "%s:%d: Test failed",
                     current_file, current_line );
            if (msg && msg[0])
            {
                va_start(valist, msg);
                fprintf( stderr,": ");
                vfprintf(stderr, msg, valist);
                va_end(valist);
            }
            fputc( '\n', stderr );
            failures++;
            return 0;
        }
        else successes++;
    }
    return 1;
}

winetest_ok_funcptr winetest_set_ok_location( const char* file, int line )
{
    current_file=file;
    current_line=line;
    return &winetest_ok;
}

void winetest_trace( const char *msg, ... )
{
    va_list valist;

    if (winetest_debug > 0)
    {
        va_start(valist, msg);
        vfprintf(stderr, msg, valist);
        va_end(valist);
    }
}

winetest_trace_funcptr winetest_set_trace_location( const char* file, int line )
{
    current_file=file;
    current_line=line;
    return &winetest_trace;
}

void winetest_start_todo( const char* platform )
{
    if (strcmp(winetest_platform,platform)==0)
        todo_level++;
    todo_do_loop=1;
}

int winetest_loop_todo(void)
{
    int do_loop=todo_do_loop;
    todo_do_loop=0;
    return do_loop;
}

void winetest_end_todo( const char* platform )
{
    if (strcmp(winetest_platform,platform)==0)
        todo_level--;
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
        exit(1);
    }
    successes = failures = todo_successes = todo_failures = 0;
    todo_level = 0;
    current_test = test;
    test->func();

    if (winetest_debug)
    {
        fprintf( stderr, "%s: %d tests executed, %d marked as todo, %d %s.\n",
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
    if (!argv[1])
    {
        fprintf( stderr, "Usage: %s test_name\n", argv[0] );
        exit(1);
    }
    exit( run_test(argv[1]) );
}
