/*
 * Main function
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Martin von Loewis
 * Copyright 1995, 1996, 1997 Alexandre Julliard
 * Copyright 1997 Eric Youngdale
 * Copyright 1999 Ulrich Weigand
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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "build.h"

ORDDEF *EntryPoints[MAX_ORDINALS];
ORDDEF *Ordinals[MAX_ORDINALS];
ORDDEF *Names[MAX_ORDINALS];

SPEC_MODE SpecMode = SPEC_MODE_DLL;
SPEC_TYPE SpecType = SPEC_WIN32;

int Base = MAX_ORDINALS;
int Limit = 0;
int DLLHeapSize = 0;
int UsePIC = 0;
int stack_size = 0;
int nb_entry_points = 0;
int nb_names = 0;
int nb_debug_channels = 0;
int nb_lib_paths = 0;
int display_warnings = 0;

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

char *input_file_name = NULL;
const char *output_file_name = NULL;

static FILE *input_file;
static FILE *output_file;
static const char *current_src_dir;
static int nb_res_files;
static char **res_files;

/* execution mode */
static enum
{
    MODE_NONE,
    MODE_SPEC,
    MODE_EXE,
    MODE_GLUE,
    MODE_DEF,
    MODE_DEBUG,
    MODE_RELAY16,
    MODE_RELAY32
} exec_mode = MODE_NONE;

/* set the dll file name from the input file name */
static void set_dll_file_name( const char *name )
{
    char *p;

    if ((p = strrchr( name, '\\' ))) name = p + 1;
    if ((p = strrchr( name, '/' ))) name = p + 1;
    strcpy( DLLFileName, name );
    if ((p = strrchr( DLLFileName, '.' )) && !strcmp( p, ".spec" )) *p = 0;
    if (!strchr( DLLFileName, '.' )) strcat( DLLFileName, ".dll" );
}

/* cleanup on program exit */
static void cleanup(void)
{
    if (output_file_name) unlink( output_file_name );
}


/*******************************************************************
 *         command-line option handling
 */

struct option_descr
{
    const char *name;
    int         has_arg;
    void      (*func)();
    const char *usage;
};

static void do_output( const char *arg );
static void do_usage(void);
static void do_warnings(void);
static void do_f_flags( const char *arg );
static void do_define( const char *arg );
static void do_include( const char *arg );
static void do_k_flags( const char *arg );
static void do_exe_mode( const char *arg );
static void do_module( const char *arg );
static void do_heap( const char *arg );
static void do_name( const char *arg );
static void do_entry( const char *arg );
static void do_spec( const char *arg );
static void do_def( const char *arg );
static void do_exe( const char *arg );
static void do_glue(void);
static void do_relay16(void);
static void do_relay32(void);
static void do_debug(void);
static void do_chdir( const char *arg );
static void do_lib( const char *arg );
static void do_import( const char *arg );
static void do_dimport( const char *arg );
static void do_rsrc( const char *arg );

static const struct option_descr option_table[] =
{
    { "-h",        0, do_usage,   "-h               Display this help message" },
    { "-w",        0, do_warnings,"-w               Turn on warnings" },
    { "-C",        1, do_chdir,   "-C dir           Change directory to <dir> before opening source files" },
    { "-f",        1, do_f_flags, "-f flags         Compiler flags (only -fPIC is supported)" },
    { "-D",        1, do_define,  "-D sym           Ignored for C flags compatibility" },
    { "-I",        1, do_include, "-I dir           Ignored for C flags compatibility" },
    { "-K",        1, do_k_flags, "-K flags         Compiler flags (only -KPIC is supported)" },
    { "-m",        1, do_exe_mode,"-m mode          Set the executable mode (cui|gui|cuiw|guiw)" },
    { "-M",        1, do_module,  "-M module        Set the name of the main (Win32) module for a Win16 dll" },
    { "-L",        1, do_lib,     "-L directory     Look for imports libraries in 'directory'" },
    { "-l",        1, do_import,  "-l lib.dll       Import the specified library" },
    { "-d",        1, do_dimport, "-d lib.dll       Delay-import the specified library" },
    { "-H",        1, do_heap,    "-H size          Set the heap size for a Win16 dll" },
    { "-N",        1, do_name,    "-N dllname       Set the DLL name (default: set from input file name)" },
    { "-e",        1, do_entry,   "-e function      Set the DLL entry point function (default: DllMain)" },
    { "-r",        1, do_rsrc,    "-r rsrc.res      Load resources from rsrc.res" },
    { "-res",      1, do_rsrc,    NULL },  /* for backwards compatibility, will disappear */
    { "-o",        1, do_output,  "-o name          Set the output file name (default: stdout)\n" },
    { "--spec",    1, do_spec,    "--spec file.spec Build a .c file from a spec file" },
    { "--def",     1, do_def,     "--def file.spec  Build a .def file from a spec file" },
    { "--exe",     1, do_exe,     "--exe name       Build a .c file for the named executable" },
    { "--debug",   0, do_debug,   "--debug [files]  Build a .c file containing debug channels declarations" },
    { "--glue",    0, do_glue,    "--glue [files]   Build the 16-bit glue for the source files" },
    { "--relay16", 0, do_relay16, "--relay16        Build the 16-bit relay assembly routines" },
    { "--relay32", 0, do_relay32, "--relay32        Build the 32-bit relay assembly routines" },
    { NULL,       0, NULL,      NULL }
};

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
    const struct option_descr *opt;
    fprintf( stderr, "Usage: winebuild [options]\n\n" );
    fprintf( stderr, "Options:\n" );
    for (opt = option_table; opt->name; opt++)
        if (opt->usage) fprintf( stderr, "   %s\n", opt->usage );

    fprintf( stderr, "\nExactly one of --spec, --def, --exe, --debug, --glue, --relay16 or --relay32 must be specified.\n\n" );
    exit(1);
}

static void do_warnings(void)
{
    display_warnings = 1;
}

static void do_f_flags( const char *arg )
{
    if (!strcmp( arg, "PIC" )) UsePIC = 1;
    /* ignore all other flags */
}

static void do_define( const char *arg )
{
    /* nothing */
}

static void do_include( const char *arg )
{
    /* nothing */
}

static void do_k_flags( const char *arg )
{
    /* Ignored, because cc generates correct code. */
    /* if (!strcmp( arg, "PIC" )) UsePIC = 1; */
    /* ignore all other flags */
}

static void do_heap( const char *arg )
{
    if (!isdigit(arg[0]))
        fatal_error( "Expected number argument with -H option instead of '%s'\n", arg );
    DLLHeapSize = atoi(arg);
    if (DLLHeapSize > 65535) fatal_error( "Invalid heap size %d, maximum is 65535\n", DLLHeapSize );
}

static void do_name( const char *arg )
{
    strncpy( DLLName, arg, sizeof(DLLName) );
    DLLName[sizeof(DLLName) - 1] = 0;
}

static void do_entry( const char *arg )
{
    init_func = xstrdup( arg );
}

static void do_spec( const char *arg )
{
    if (exec_mode != MODE_NONE || !arg[0]) do_usage();
    exec_mode = MODE_SPEC;
    input_file = open_input_file( NULL, arg );
    set_dll_file_name( arg );
}

static void do_def( const char *arg )
{
    if (exec_mode != MODE_NONE || !arg[0]) do_usage();
    exec_mode = MODE_DEF;
    input_file = open_input_file( NULL, arg );
    set_dll_file_name( arg );
}

static void do_exe( const char *arg )
{
    const char *p;

    if (exec_mode != MODE_NONE || !arg[0]) do_usage();
    exec_mode = MODE_EXE;
    if ((p = strrchr( arg, '/' ))) p++;
    else p = arg;
    strcpy( DLLFileName, p );
    if (!strchr( DLLFileName, '.' )) strcat( DLLFileName, ".exe" );
    if (SpecMode == SPEC_MODE_DLL) SpecMode = SPEC_MODE_GUIEXE;
}

static void do_exe_mode( const char *arg )
{
    if (!strcmp( arg, "gui" )) SpecMode = SPEC_MODE_GUIEXE;
    else if (!strcmp( arg, "cui" )) SpecMode = SPEC_MODE_CUIEXE;
    else if (!strcmp( arg, "guiw" )) SpecMode = SPEC_MODE_GUIEXE_UNICODE;
    else if (!strcmp( arg, "cuiw" )) SpecMode = SPEC_MODE_CUIEXE_UNICODE;
    else do_usage();
}

static void do_module( const char *arg )
{
    strcpy( owner_name, arg );
    SpecType = SPEC_WIN16;
}

static void do_glue(void)
{
    if (exec_mode != MODE_NONE) do_usage();
    exec_mode = MODE_GLUE;
}

static void do_debug(void)
{
    if (exec_mode != MODE_NONE) do_usage();
    exec_mode = MODE_DEBUG;
}

static void do_chdir( const char *arg )
{
    current_src_dir = arg;
}

static void do_relay16(void)
{
    if (exec_mode != MODE_NONE) do_usage();
    exec_mode = MODE_RELAY16;
}

static void do_relay32(void)
{
    if (exec_mode != MODE_NONE) do_usage();
    exec_mode = MODE_RELAY32;
}

static void do_lib( const char *arg )
{
    lib_path = xrealloc( lib_path, (nb_lib_paths+1) * sizeof(*lib_path) );
    lib_path[nb_lib_paths++] = xstrdup( arg );
}

static void do_import( const char *arg )
{
    add_import_dll( arg, 0 );
}

static void do_dimport( const char *arg )
{
    add_import_dll( arg, 1 );
}

static void do_rsrc( const char *arg )
{
    res_files = xrealloc( res_files, (nb_res_files+1) * sizeof(*res_files) );
    res_files[nb_res_files++] = xstrdup( arg );
}

/* parse options from the argv array and remove all the recognized ones */
static void parse_options( char *argv[] )
{
    const struct option_descr *opt;
    char **ptr, **last;
    const char* arg=NULL;

    for (ptr = last = argv + 1; *ptr; ptr++)
    {
        /* first check the exact option name */
        for (opt = option_table; opt->name; opt++)
        {
            if (!strcmp( *ptr, opt->name ) ||
                /* for long option check without the first dash too */
                (opt->name[1] == '-' && !strcmp( *ptr, opt->name+1 )))
            {
                if (opt->has_arg) arg = *(++ptr);
                else arg = NULL;
                break;
            }
        }

        /* now check for option name concatenated with argument */
        if (!opt->name)
        {
            for (opt = option_table; opt->name; opt++)
            {
                if (opt->has_arg && !strncmp( *ptr, opt->name, strlen(opt->name) ))
                {
                    arg = *ptr + strlen(opt->name);
                    break;
                }
            }
        }

        if (opt->name)
        {
            if (opt->has_arg && arg != NULL) opt->func( arg );
            else opt->func( "" );
        }
        else  /* keep this argument */
        {
            if (last != ptr) *last = *ptr;
            last++;
        }
    }
    *last = NULL;
}


/* load all specified resource files */
static void load_resources(void)
{
    int i;

    switch (SpecType)
    {
    case SPEC_WIN16:
        for (i = 0; i < nb_res_files; i++) load_res16_file( res_files[i] );
        break;
    case SPEC_WIN32:
        for (i = 0; i < nb_res_files; i++) load_res32_file( res_files[i] );
        break;
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
        load_resources();
        ParseTopLevel( input_file );
        switch (SpecType)
        {
            case SPEC_WIN16:
                if (argv[1])
                    fatal_error( "file argument '%s' not allowed in this mode\n", argv[1] );
                BuildSpec16File( output_file );
                break;
            case SPEC_WIN32:
                read_undef_symbols( argv + 1 );
                BuildSpec32File( output_file );
                break;
            default: assert(0);
        }
        break;
    case MODE_EXE:
        if (SpecType == SPEC_WIN16) fatal_error( "Cannot build 16-bit exe files\n" );
        load_resources();
        read_undef_symbols( argv + 1 );
        BuildSpec32File( output_file );
        break;
    case MODE_DEF:
        if (argv[1]) fatal_error( "file argument '%s' not allowed in this mode\n", argv[1] );
        if (SpecType == SPEC_WIN16) fatal_error( "Cannot yet build .def file for 16-bit dlls\n" );
        load_resources();
        ParseTopLevel( input_file );
        BuildDef32File( output_file );
        break;
    case MODE_DEBUG:
        BuildDebugFile( output_file, current_src_dir, argv + 1 );
        break;
    case MODE_GLUE:
        BuildGlue( output_file, current_src_dir, argv + 1 );
        break;
    case MODE_RELAY16:
        if (argv[1]) fatal_error( "file argument '%s' not allowed in this mode\n", argv[1] );
        BuildRelays16( output_file );
        break;
    case MODE_RELAY32:
        if (argv[1]) fatal_error( "file argument '%s' not allowed in this mode\n", argv[1] );
        BuildRelays32( output_file );
        break;
    default:
        do_usage();
        break;
    }
    if (output_file_name)
    {
        fclose( output_file );
        output_file_name = NULL;
    }
    return 0;
}
