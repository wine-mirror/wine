/*
 * Main function
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Martin von Loewis
 * Copyright 1995, 1996, 1997 Alexandre Julliard
 * Copyright 1997 Eric Youngdale
 * Copyright 1999 Ulrich Weigand
 */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "config.h"
#include "winnt.h"
#include "build.h"

ORDDEF *EntryPoints[MAX_ORDINALS];
ORDDEF *Ordinals[MAX_ORDINALS];
ORDDEF *Names[MAX_ORDINALS];

SPEC_MODE SpecMode = SPEC_MODE_DLL;
int Base = MAX_ORDINALS;
int Limit = 0;
int DLLHeapSize = 0;
int UsePIC = 0;
int nb_entry_points = 0;
int nb_names = 0;
int nb_debug_channels = 0;
int nb_lib_paths = 0;

/* we only support relay debugging on i386 */
#if defined(__i386__) && !defined(NO_TRACE_MSGS)
int debugging = 1;
#else
int debugging = 0;
#endif

char DLLName[80];
char DLLFileName[80];
char owner_name[80];
char *init_func = NULL;
char **debug_channels = NULL;
char **lib_path = NULL;

const char *input_file_name;
const char *output_file_name;

static FILE *input_file;
static FILE *output_file;

/* execution mode */
static enum { MODE_NONE, MODE_SPEC, MODE_GLUE, MODE_RELAY } exec_mode = MODE_NONE;

/* open the input file */
static void open_input( const char *name )
{
    input_file_name = name;
    if (!(input_file = fopen( name, "r" )))
    {
        fprintf( stderr, "Cannot open input file '%s'\n", name );
        exit(1);
    }
}

/* cleanup on program exit */
static void cleanup(void)
{
    if (output_file_name) unlink( output_file_name );
}


/*******************************************************************
 *         command-line option handling
 */

struct option
{
    const char *name;
    int         has_arg;
    void      (*func)();
    const char *usage;
};

static void do_pic(void);
static void do_output( const char *arg );
static void do_usage(void);
static void do_spec( const char *arg );
static void do_glue( const char *arg );
static void do_relay(void);
static void do_sym( const char *arg );
static void do_lib( const char *arg );

static const struct option option_table[] =
{
    { "-fPIC",  0, do_pic,    "-fPIC            Generate PIC code" },
    { "-h",     0, do_usage,  "-h               Display this help message" },
    { "-L",     1, do_lib,    "-L directory     Look for imports libraries in 'directory'" },
    { "-o",     1, do_output, "-o name          Set the output file name (default: stdout)" },
    { "-sym",   1, do_sym,    "-sym file.o      Read the list of undefined symbols from 'file.o'" },
    { "-spec",  1, do_spec,   "-spec file.spec  Build a .c file from a spec file" },
    { "-glue",  1, do_glue,   "-glue file.c     Build the 16-bit glue for a .c file" },
    { "-relay", 0, do_relay,  "-relay           Build the relay assembly routines" },
    { NULL,     0, NULL,      NULL }
};

static void do_pic(void)
{
    UsePIC = 1;
}

static void do_output( const char *arg )
{
    if ( ( unlink ( arg ) ) == -1 && ( errno != ENOENT ) ) 
    {
        fprintf ( stderr, "Unable to create output file '%s'\n", arg );
        exit (1);
    }
    if (!(output_file = fopen( arg, "w" )))
    {
        fprintf( stderr, "Unable to create output file '%s'\n", arg );
        exit(1);
    }
    output_file_name = arg;
    atexit( cleanup );  /* make sure we remove the output file on exit */
}

static void do_usage(void)
{
    const struct option *opt;
    fprintf( stderr, "Usage: winebuild [options]\n\n" );
    fprintf( stderr, "Options:\n" );
    for (opt = option_table; opt->name; opt++) fprintf( stderr, "   %s\n", opt->usage );
    fprintf( stderr, "\nExactly one of -spec, -glue or -relay must be specified.\n\n" );
    exit(1);
}

static void do_spec( const char *arg )
{
    if (exec_mode != MODE_NONE || !arg[0]) do_usage();
    exec_mode = MODE_SPEC;
    open_input( arg );
}

static void do_glue( const char *arg )
{
    if (exec_mode != MODE_NONE || !arg[0]) do_usage();
    exec_mode = MODE_GLUE;
    open_input( arg );
}

static void do_relay(void)
{
    if (exec_mode != MODE_NONE) do_usage();
    exec_mode = MODE_RELAY;
}

static void do_sym( const char *arg )
{
    extern void read_undef_symbols( const char *name );
    read_undef_symbols( arg );
}

static void do_lib( const char *arg )
{
    lib_path = xrealloc( lib_path, (nb_lib_paths+1) * sizeof(*lib_path) );
    lib_path[nb_lib_paths++] = xstrdup( arg );
}

/* parse options from the argv array and remove all the recognized ones */
static void parse_options( char *argv[] )
{
    const struct option *opt;
    char * const * ptr;
    const char* arg=NULL;

    ptr=argv+1;
    while (*ptr != NULL)
    {
        for (opt = option_table; opt->name; opt++)
        {
            if (opt->has_arg && !strncmp( *ptr, opt->name, strlen(opt->name) ))
            {
                arg=*ptr+strlen(opt->name);
                if (*arg=='\0')
                {
                    ptr++;
                    arg=*ptr;
                }
                break;
            }
            if (!strcmp( *ptr, opt->name ))
            {
                arg=NULL;
                break;
            }
        }

        if (!opt->name)
        {
            fprintf( stderr, "Unrecognized option '%s'\n", *ptr );
            do_usage();
        }

        if (opt->has_arg && arg!=NULL) opt->func( arg );
        else opt->func( "" );
        ptr++;
    }
}


/*******************************************************************
 *         main
 */
int main(int argc, char **argv)
{
    output_file = stdout;
    parse_options( argv );

    switch(exec_mode)
    {
    case MODE_SPEC:
        switch (ParseTopLevel( input_file ))
        {
            case SPEC_WIN16:
                BuildSpec16File( output_file );
                break;
            case SPEC_WIN32:
                BuildSpec32File( output_file );
                break;
            default: assert(0);
        }
        break;
    case MODE_GLUE:
        BuildGlue( output_file, input_file );
        break;
    case MODE_RELAY:
        BuildRelays( output_file );
        break;
    default:
        do_usage();
        break;
    }
    fclose( output_file );
    output_file_name = NULL;
    return 0;
}
