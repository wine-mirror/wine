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

#include "winnt.h"
#include "build.h"

#ifdef __i386__
extern WORD __get_cs(void);
extern WORD __get_ds(void);
__ASM_GLOBAL_FUNC( __get_cs, "movw %cs,%ax\n\tret" );
__ASM_GLOBAL_FUNC( __get_ds, "movw %ds,%ax\n\tret" );
#else
static inline WORD __get_cs(void) { return 0; }
static inline WORD __get_ds(void) { return 0; }
#endif


ORDDEF EntryPoints[MAX_ORDINALS];
ORDDEF *Ordinals[MAX_ORDINALS];
ORDDEF *Names[MAX_ORDINALS];

SPEC_MODE SpecMode = SPEC_MODE_DLL;
int Base = MAX_ORDINALS;
int Limit = 0;
int DLLHeapSize = 0;
int UsePIC = 0;
int nb_entry_points = 0;
int nb_names = 0;
int nb_imports = 0;
int debugging = 1;

char DLLName[80];
char DLLFileName[80];
char DLLInitFunc[80];
char *DLLImports[MAX_IMPORTS];
char rsrc_name[80];

const char *input_file_name;
const char *output_file_name;

unsigned short code_selector;
unsigned short data_selector;

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

static const struct option option_table[] =
{
    { "-fPIC",  0, do_pic,    "-fPIC            Generate PIC code" },
    { "-h",     0, do_usage,  "-h               Display this help message" },
    { "-o",     1, do_output, "-o name          Set the output file name (default: stdout)" },
    { "-spec",  1, do_spec,   "-spec file.spec  Build a .c file from a spec file" },
    { "-glue",  1, do_glue,   "-glue file.c     Build the 16-bit glue for a .c file" },
    { "-relay", 0, do_relay,  "-relay           Build the relay assembly routines" },
    { NULL }
};

static void do_pic(void)
{
    UsePIC = 1;
}

static void do_output( const char *arg )
{
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


/* parse options from the argv array and remove all the recognized ones */
static void parse_options( char *argv[] )
{
    const struct option *opt;
    int i;

    for (i = 1; argv[i]; i++)
    {
        for (opt = option_table; opt->name; opt++)
            if (!strcmp( argv[i], opt->name )) break;

        if (!opt->name)
        {
            fprintf( stderr, "Unrecognized option '%s'\n", argv[i] );
            do_usage();
        }

        if (opt->has_arg && argv[i+1]) opt->func( argv[++i] );
        else opt->func( "" );
    }
}


/*******************************************************************
 *         main
 */
int main(int argc, char **argv)
{
    output_file = stdout;
    parse_options( argv );

    /* Retrieve the selector values; this assumes that we are building
     * the asm files on the platform that will also run them. Probably
     * a safe assumption to make.
     */
    code_selector = __get_cs();
    data_selector = __get_ds();

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
