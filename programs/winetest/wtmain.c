/*
 * Main routine for Wine C unit tests.
 *
 * Copyright 2002 Alexandre Julliard
 * Copyright 2002 Andriy Palamarchuk
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static int successes;         /* number of successful tests */
static int failures;          /* number of failures */
static int todo_successes;    /* number of successful tests inside todo block */
static int todo_failures;     /* number of failures inside todo block */
static int todo_level;        /* current todo nesting level */

/*
 * Checks condition.
 * Parameters:
 *   - condition - condition to check;
 *   - msg test description;
 *   - file - test application source code file name of the check
 *   - line - test application source code file line number of the check
 */
void winetest_ok( int condition, const char *msg, const char *file, int line )
{
    if (todo_level)
    {
        if (condition)
        {
            fprintf( stderr, "%s:%d: Test succeeded inside todo block", file, line );
            if (msg && msg[0]) fprintf( stderr, ": %s", msg );
            fputc( '\n', stderr );
            todo_failures++;
        }
        else todo_successes++;
    }
    else
    {
        if (!condition)
        {
            fprintf( stderr, "%s:%d: Test failed", file, line );
            if (msg && msg[0]) fprintf( stderr, ": %s", msg );
            fputc( '\n', stderr );
            failures++;
        }
        else successes++;
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
