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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "build.h"

int UsePIC = 0;
int nb_errors = 0;
int display_warnings = 0;
int native_arch = -1;
int kill_at = 0;
int verbose = 0;
int link_ext_symbols = 0;
int force_pointer_size = 0;
int unwind_tables = 0;
int use_dlltool = 1;
int use_msvcrt = 0;
int safe_seh = 0;
int prefer_native = 0;
int data_only = 0;

struct target target = { 0 };

char *target_alias = NULL;

char *input_file_name = NULL;
char *spec_file_name = NULL;
FILE *output_file = NULL;
const char *output_file_name = NULL;
static int save_temps;
static int fake_module;
static DLLSPEC *main_spec;

static const struct strarray empty_strarray;
struct strarray tools_path = { 0 };
struct strarray as_command = { 0 };
struct strarray cc_command = { 0 };
struct strarray ld_command = { 0 };
struct strarray nm_command = { 0 };
char *cpu_option = NULL;
char *fpu_option = NULL;
char *arch_option = NULL;

static struct strarray res_files;

/* execution mode */
enum exec_mode_values
{
    MODE_NONE,
    MODE_DLL,
    MODE_EXE,
    MODE_DEF,
    MODE_IMPLIB,
    MODE_STATICLIB,
    MODE_BUILTIN,
    MODE_FIXUP_CTORS,
    MODE_RESOURCES
};

static enum exec_mode_values exec_mode = MODE_NONE;


/* set the dll file name from the input file name */
static void set_dll_file_name( const char *name, DLLSPEC *spec )
{
    char *p;

    if (spec->file_name) return;

    name = get_basename( name );
    spec->file_name = xmalloc( strlen(name) + 5 );
    strcpy( spec->file_name, name );
    if ((p = strrchr( spec->file_name, '.' )))
    {
        if (!strcmp( p, ".spec" ) || !strcmp( p, ".def" )) *p = 0;
    }
}

/* set the dll name from the file name */
static void init_dll_name( DLLSPEC *spec )
{
    if (!spec->file_name && output_file_name)
    {
        char *p;
        spec->file_name = xstrdup( output_file_name );
        if ((p = strrchr( spec->file_name, '.' ))) *p = 0;
    }
    if (!spec->dll_name && spec->file_name)  /* set default name from file name */
    {
        char *p;
        spec->dll_name = xstrdup( spec->file_name );
        if ((p = strrchr( spec->dll_name, '.' ))) *p = 0;
    }
    if (spec->dll_name) spec->c_name = make_c_identifier( spec->dll_name );
}

/* set the dll subsystem */
static void set_subsystem( const char *subsystem, DLLSPEC *spec )
{
    char *major, *minor, *str = xstrdup( subsystem );

    if ((major = strchr( str, ':' ))) *major++ = 0;
    if (!strcmp( str, "native" )) spec->subsystem = IMAGE_SUBSYSTEM_NATIVE;
    else if (!strcmp( str, "windows" )) spec->subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
    else if (!strcmp( str, "console" )) spec->subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
    else if (!strcmp( str, "wince" ))   spec->subsystem = IMAGE_SUBSYSTEM_WINDOWS_CE_GUI;
    else if (!strcmp( str, "win16" )) spec->type = SPEC_WIN16;
    else fatal_error( "Invalid subsystem name '%s'\n", subsystem );
    if (major)
    {
        if ((minor = strchr( major, '.' )))
        {
            *minor++ = 0;
            spec->subsystem_minor = atoi( minor );
        }
        spec->subsystem_major = atoi( major );
    }
    free( str );
}

/* set the target CPU and platform */
static void set_target( const char *name )
{
    target_alias = xstrdup( name );

    if (!parse_target( name, &target )) fatal_error( "Unrecognized target '%s'\n", name );
    if (is_pe()) unwind_tables = 1;
}

/* cleanup on program exit */
static void cleanup(void)
{
    if (output_file_name) unlink( output_file_name );
    if (!save_temps) remove_temp_files();
}

/* clean things up when aborting on a signal */
static void exit_on_signal( int sig )
{
    exit(1);  /* this will call atexit functions */
}

/*******************************************************************
 *         command-line option handling
 */
static const char usage_str[] =
"Usage: winebuild [OPTIONS] [FILES]\n\n"
"Options:\n"
"       --as-cmd=AS           Command to use for assembling (default: as)\n"
"   -b, --target=TARGET       Specify target CPU and platform for cross-compiling\n"
"   -B PREFIX                 Look for build tools in the PREFIX directory\n"
"       --cc-cmd=CC           C compiler to use for assembling (default: fall back to --as-cmd)\n"
"       --data-only           Generate a data-only dll (i.e. without any executable code)\n"
"   -d, --delay-lib=LIB       Import the specified library in delayed mode\n"
"   -D SYM                    Ignored for C flags compatibility\n"
"   -e, --entry=FUNC          Set the DLL entry point function (default: DllMain)\n"
"   -E, --export=FILE         Export the symbols defined in the .spec or .def file\n"
"       --external-symbols    Allow linking to external symbols\n"
"   -f FLAGS                  Compiler flags (-fPIC and -fasynchronous-unwind-tables are supported)\n"
"   -F, --filename=DLLFILE    Set the DLL filename (default: from input file name)\n"
"       --fake-module         Create a fake binary module\n"
"   -h, --help                Display this help message\n"
"   -H, --heap=SIZE           Set the heap size for a Win16 dll\n"
"   -I DIR                    Ignored for C flags compatibility\n"
"   -k, --kill-at             Kill stdcall decorations in generated .def files\n"
"   -K, FLAGS                 Compiler flags (only -KPIC is supported)\n"
"       --large-address-aware Support an address space larger than 2Gb\n"
"       --ld-cmd=LD           Command to use for linking (default: ld)\n"
"   -m16, -m32, -m64          Force building 16-bit, 32-bit resp. 64-bit code\n"
"   -M, --main-module=MODULE  Set the name of the main module for a Win16 dll\n"
"       --nm-cmd=NM           Command to use to get undefined symbols (default: nm)\n"
"       --nxcompat=y|n        Set the NX compatibility flag (default: yes)\n"
"   -N, --dll-name=DLLNAME    Set the DLL name (default: from input file name)\n"
"   -o, --output=NAME         Set the output file name (default: stdout)\n"
"       --prefer-native       Set the flag to prefer loading native at run time\n"
"   -r, --res=RSRC.RES        Load resources from RSRC.RES\n"
"       --safeseh             Mark object files as SEH compatible\n"
"       --save-temps          Do not delete the generated intermediate files\n"
"       --subsystem=SUBSYS    Set the subsystem (one of native, windows, console, wince)\n"
"   -u, --undefined=SYMBOL    Add an undefined reference to SYMBOL when linking\n"
"   -v, --verbose             Display the programs invoked\n"
"       --version             Print the version and exit\n"
"   -w, --warnings            Turn on warnings\n"
"       --without-dlltool     Generate import library without using dlltool\n"
"\nMode options:\n"
"       --dll                 Build a library from a .spec file and object files\n"
"       --def                 Build a .def file from a .spec file\n"
"       --exe                 Build an executable from object files\n"
"       --implib              Build an import library\n"
"       --staticlib           Build a static library\n"
"       --resources           Build a .o or .res file for the resource files\n\n"
"       --builtin             Mark a library as a Wine builtin\n"
"       --fixup-ctors         Fixup the constructors data after the module has been built\n"
"The mode options are mutually exclusive; you must specify one and only one.\n\n";

enum long_options_values
{
    LONG_OPT_DLL = 1,
    LONG_OPT_DEF,
    LONG_OPT_EXE,
    LONG_OPT_IMPLIB,
    LONG_OPT_BUILTIN,
    LONG_OPT_ASCMD,
    LONG_OPT_CCCMD,
    LONG_OPT_DATA_ONLY,
    LONG_OPT_EXTERNAL_SYMS,
    LONG_OPT_FAKE_MODULE,
    LONG_OPT_FIXUP_CTORS,
    LONG_OPT_LARGE_ADDRESS_AWARE,
    LONG_OPT_LDCMD,
    LONG_OPT_NMCMD,
    LONG_OPT_NXCOMPAT,
    LONG_OPT_PREFER_NATIVE,
    LONG_OPT_RESOURCES,
    LONG_OPT_SAFE_SEH,
    LONG_OPT_SAVE_TEMPS,
    LONG_OPT_STATICLIB,
    LONG_OPT_SUBSYSTEM,
    LONG_OPT_VERSION,
    LONG_OPT_WITHOUT_DLLTOOL,
};

static const char short_options[] = "B:C:D:E:F:H:I:K:L:M:N:b:d:e:f:hkl:m:o:r:u:vw";

static const struct long_option long_options[] =
{
    /* mode options */
    { "dll",                 0, LONG_OPT_DLL },
    { "def",                 0, LONG_OPT_DEF },
    { "exe",                 0, LONG_OPT_EXE },
    { "implib",              0, LONG_OPT_IMPLIB },
    { "staticlib",           0, LONG_OPT_STATICLIB },
    { "builtin",             0, LONG_OPT_BUILTIN },
    { "resources",           0, LONG_OPT_RESOURCES },
    { "fixup-ctors",         0, LONG_OPT_FIXUP_CTORS },
    /* other long options */
    { "as-cmd",              1, LONG_OPT_ASCMD },
    { "cc-cmd",              1, LONG_OPT_CCCMD },
    { "data-only",           0, LONG_OPT_DATA_ONLY },
    { "external-symbols",    0, LONG_OPT_EXTERNAL_SYMS },
    { "fake-module",         0, LONG_OPT_FAKE_MODULE },
    { "large-address-aware", 0, LONG_OPT_LARGE_ADDRESS_AWARE },
    { "ld-cmd",              1, LONG_OPT_LDCMD },
    { "nm-cmd",              1, LONG_OPT_NMCMD },
    { "nxcompat",            1, LONG_OPT_NXCOMPAT },
    { "prefer-native",       0, LONG_OPT_PREFER_NATIVE },
    { "safeseh",             0, LONG_OPT_SAFE_SEH },
    { "save-temps",          0, LONG_OPT_SAVE_TEMPS },
    { "subsystem",           1, LONG_OPT_SUBSYSTEM },
    { "version",             0, LONG_OPT_VERSION },
    { "without-dlltool",     0, LONG_OPT_WITHOUT_DLLTOOL },
    /* aliases for short options */
    { "target",              1, 'b' },
    { "delay-lib",           1, 'd' },
    { "export",              1, 'E' },
    { "entry",               1, 'e' },
    { "filename",            1, 'F' },
    { "help",                0, 'h' },
    { "heap",                1, 'H' },
    { "kill-at",             0, 'k' },
    { "main-module",         1, 'M' },
    { "dll-name",            1, 'N' },
    { "output",              1, 'o' },
    { "res",                 1, 'r' },
    { "undefined",           1, 'u' },
    { "verbose",             0, 'v' },
    { "warnings",            0, 'w' },
    { NULL }
};

static void usage( int exit_code )
{
    fprintf( stderr, "%s", usage_str );
    exit( exit_code );
}

static void set_exec_mode( enum exec_mode_values mode )
{
    if (exec_mode != MODE_NONE) usage(1);
    exec_mode = mode;
}

/* get the default entry point for a given spec file */
static const char *get_default_entry_point( const DLLSPEC *spec )
{
    if (spec->characteristics & IMAGE_FILE_DLL)
    {
        add_spec_extra_ld_symbol("DllMain");
        return "__wine_spec_dll_entry";
    }
    if (spec->subsystem == IMAGE_SUBSYSTEM_NATIVE) return "DriverEntry";
    if (spec->type == SPEC_WIN16)
    {
        add_spec_extra_ld_symbol("WinMain16");
        return "__wine_spec_exe16_entry";
    }
    else if (spec->unicode_app)
    {
        /* __wine_spec_exe_wentry always calls wmain */
        add_spec_extra_ld_symbol("wmain");
        if (spec->subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI)
            add_spec_extra_ld_symbol("wWinMain");
        return "__wine_spec_exe_wentry";
    }
    else
    {
        /* __wine_spec_exe_entry always calls main */
        add_spec_extra_ld_symbol("main");
        if (spec->subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI)
            add_spec_extra_ld_symbol("WinMain");
        return "__wine_spec_exe_entry";
    }
}

static void option_callback( int optc, char *optarg )
{
    char *p;

    switch (optc)
    {
    case 'B':
        strarray_add( &tools_path, xstrdup( optarg ));
        break;
    case 'D':
        /* ignored */
        break;
    case 'E':
        spec_file_name = xstrdup( optarg );
        set_dll_file_name( optarg, main_spec );
        break;
    case 'F':
        main_spec->file_name = xstrdup( optarg );
        break;
    case 'H':
        if (!isdigit(optarg[0]))
            fatal_error( "Expected number argument with -H option instead of '%s'\n", optarg );
        main_spec->heap_size = atoi(optarg);
        if (main_spec->heap_size > 65535)
            fatal_error( "Invalid heap size %d, maximum is 65535\n", main_spec->heap_size );
        break;
    case 'I':
        /* ignored */
        break;
    case 'K':
        /* ignored, because cc generates correct code. */
        break;
    case 'm':
        if (!strcmp( optarg, "16" )) main_spec->type = SPEC_WIN16;
        else if (!strcmp( optarg, "32" )) force_pointer_size = 4;
        else if (!strcmp( optarg, "64" )) force_pointer_size = 8;
        else if (!strcmp( optarg, "no-cygwin" )) use_msvcrt = 1;
        else if (!strcmp( optarg, "unicode" )) main_spec->unicode_app = 1;
        else if (!strcmp( optarg, "arm64x" )) native_arch = CPU_ARM64;
        else if (!strncmp( optarg, "cpu=", 4 )) cpu_option = xstrdup( optarg + 4 );
        else if (!strncmp( optarg, "fpu=", 4 )) fpu_option = xstrdup( optarg + 4 );
        else if (!strncmp( optarg, "arch=", 5 )) arch_option = xstrdup( optarg + 5 );
        else fatal_error( "Unknown -m option '%s'\n", optarg );
        break;
    case 'M':
        main_spec->main_module = xstrdup( optarg );
        break;
    case 'N':
        main_spec->dll_name = xstrdup( optarg );
        break;
    case 'b':
        set_target( optarg );
        break;
    case 'd':
        add_delayed_import( optarg );
        break;
    case 'e':
        main_spec->init_func = xstrdup( optarg );
        if ((p = strchr( main_spec->init_func, '@' ))) *p = 0;  /* kill stdcall decoration */
        break;
    case 'f':
        if (!strcmp( optarg, "PIC") || !strcmp( optarg, "pic")) UsePIC = 1;
        else if (!strcmp( optarg, "asynchronous-unwind-tables")) unwind_tables = 1;
        else if (!strcmp( optarg, "no-asynchronous-unwind-tables")) unwind_tables = 0;
        /* ignore all other flags */
        break;
    case 'h':
        usage(0);
        break;
    case 'k':
        kill_at = 1;
        break;
    case 'o':
        if (unlink( optarg ) == -1 && errno != ENOENT)
            fatal_error( "Unable to create output file '%s'\n", optarg );
        output_file_name = xstrdup( optarg );
        break;
    case 'r':
        strarray_add( &res_files, xstrdup( optarg ));
        break;
    case 'u':
        add_extra_ld_symbol( optarg );
        break;
    case 'v':
        verbose++;
        break;
    case 'w':
        display_warnings = 1;
        break;
    case LONG_OPT_DLL:
        set_exec_mode( MODE_DLL );
        break;
    case LONG_OPT_DEF:
        set_exec_mode( MODE_DEF );
        break;
    case LONG_OPT_EXE:
        set_exec_mode( MODE_EXE );
        if (!main_spec->subsystem) main_spec->subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
        break;
    case LONG_OPT_IMPLIB:
        set_exec_mode( MODE_IMPLIB );
        break;
    case LONG_OPT_STATICLIB:
        set_exec_mode( MODE_STATICLIB );
        break;
    case LONG_OPT_BUILTIN:
        set_exec_mode( MODE_BUILTIN );
        break;
    case LONG_OPT_FIXUP_CTORS:
        set_exec_mode( MODE_FIXUP_CTORS );
        break;
    case LONG_OPT_ASCMD:
        as_command = strarray_fromstring( optarg, " " );
        break;
    case LONG_OPT_CCCMD:
        cc_command = strarray_fromstring( optarg, " " );
        break;
    case LONG_OPT_DATA_ONLY:
        data_only = 1;
        break;
    case LONG_OPT_FAKE_MODULE:
        fake_module = 1;
        break;
    case LONG_OPT_EXTERNAL_SYMS:
        link_ext_symbols = 1;
        break;
    case LONG_OPT_LARGE_ADDRESS_AWARE:
        main_spec->characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
        break;
    case LONG_OPT_LDCMD:
        ld_command = strarray_fromstring( optarg, " " );
        break;
    case LONG_OPT_NMCMD:
        nm_command = strarray_fromstring( optarg, " " );
        break;
    case LONG_OPT_NXCOMPAT:
        if (optarg[0] == 'n' || optarg[0] == 'N')
            main_spec->dll_characteristics &= ~IMAGE_DLLCHARACTERISTICS_NX_COMPAT;
        break;
    case LONG_OPT_SAFE_SEH:
        safe_seh = 1;
        break;
    case LONG_OPT_PREFER_NATIVE:
        prefer_native = 1;
        main_spec->dll_characteristics |= IMAGE_DLLCHARACTERISTICS_PREFER_NATIVE;
        break;
    case LONG_OPT_RESOURCES:
        set_exec_mode( MODE_RESOURCES );
        break;
    case LONG_OPT_SAVE_TEMPS:
        save_temps = 1;
        break;
    case LONG_OPT_SUBSYSTEM:
        set_subsystem( optarg, main_spec );
        break;
    case LONG_OPT_VERSION:
        printf( "winebuild version " PACKAGE_VERSION "\n" );
        exit(0);
    case LONG_OPT_WITHOUT_DLLTOOL:
        use_dlltool = 0;
        break;
    case '?':
        fprintf( stderr, "winebuild: %s\n\n", optarg );
        usage(1);
        break;
    }
}


/* load all specified resource files */
static struct strarray load_resources( struct strarray files, DLLSPEC *spec )
{
    struct strarray ret = empty_strarray;
    int i;

    switch (spec->type)
    {
    case SPEC_WIN16:
        for (i = 0; i < res_files.count; i++) load_res16_file( res_files.str[i], spec );
        return files;

    case SPEC_WIN32:
        for (i = 0; i < res_files.count; i++)
        {
            if (!load_res32_file( res_files.str[i], spec ))
                fatal_error( "%s is not a valid Win32 resource file\n", res_files.str[i] );
        }

        /* load any resource file found in the remaining arguments */
        for (i = 0; i < files.count; i++)
        {
            if (!load_res32_file( files.str[i], spec ))
                strarray_add( &ret, files.str[i] ); /* not a resource file, keep it in the list */
        }
        break;
    }
    return ret;
}

static int parse_input_file( DLLSPEC *spec )
{
    FILE *input_file = open_input_file( NULL, spec_file_name );
    int result;

    spec->src_name = xstrdup( input_file_name );
    if (strendswith( spec_file_name, ".def" ))
        result = parse_def_file( input_file, spec );
    else
        result = parse_spec_file( input_file, spec );
    close_input_file( input_file );
    return result;
}

static void check_target(void)
{
    if (is_pe()) return;
    if (target.cpu == CPU_i386 || target.cpu == CPU_x86_64) return;
    fatal_error( "Non-PE builds are not supported on this platform.\n" );
}

/*******************************************************************
 *         main
 */
int main(int argc, char **argv)
{
    struct strarray files;
    DLLSPEC *spec = main_spec = alloc_dll_spec();

    init_signals( exit_on_signal );
    target = init_argv0_target( argv[0] );
    if (target.platform == PLATFORM_CYGWIN) target.platform = PLATFORM_MINGW;
    if (is_pe()) unwind_tables = 1;

    files = parse_options( argc, argv, short_options, long_options, 0, option_callback );

    atexit( cleanup );  /* make sure we remove the output file on exit */

    if (spec->file_name && !strchr( spec->file_name, '.' ))
        strcat( spec->file_name, exec_mode == MODE_EXE ? ".exe" : ".dll" );
    init_dll_name( spec );

    if (force_pointer_size) set_target_ptr_size( &target, force_pointer_size );

    switch(exec_mode)
    {
    case MODE_DLL:
        if (spec->subsystem != IMAGE_SUBSYSTEM_NATIVE)
            spec->characteristics |= IMAGE_FILE_DLL;
        /* fall through */
    case MODE_EXE:
        if (get_ptr_size() == 4)
        {
            spec->characteristics |= IMAGE_FILE_32BIT_MACHINE;
        }
        else
        {
            spec->characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
            spec->dll_characteristics |= IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA;
        }

        check_target();
        files = load_resources( files, spec );
        if (spec_file_name && !parse_input_file( spec )) break;
        if (!spec->init_func) spec->init_func = xstrdup( get_default_entry_point( spec ));

        if (fake_module)
        {
            output_fake_module( spec );
            break;
        }
        if (data_only)
        {
            output_data_module( spec );
            break;
        }
        if (!is_pe())
        {
            read_undef_symbols( spec, files );
            resolve_imports( spec );
        }
        if (spec->type == SPEC_WIN16) output_spec16_file( spec );
        else output_spec32_file( spec );
        break;
    case MODE_DEF:
        if (files.count) fatal_error( "file argument '%s' not allowed in this mode\n", files.str[0] );
        if (!spec_file_name) fatal_error( "missing .spec file\n" );
        if (!parse_input_file( spec )) break;
        open_output_file();
        output_def_file( spec, &spec->exports, 0 );
        close_output_file();
        break;
    case MODE_IMPLIB:
        check_target();
        if (!spec_file_name) fatal_error( "missing .spec file\n" );
        if (!parse_input_file( spec )) break;
        output_import_lib( spec, files );
        break;
    case MODE_STATICLIB:
        output_static_lib( output_file_name, files, 1 );
        break;
    case MODE_BUILTIN:
        if (!files.count) fatal_error( "missing file argument for --builtin option\n" );
        make_builtin_files( files );
        break;
    case MODE_FIXUP_CTORS:
        if (!files.count) fatal_error( "missing file argument for --fixup-ctors option\n" );
        fixup_constructors( files );
        break;
    case MODE_RESOURCES:
        files = load_resources( files, spec );
        output_res_o_file( spec );
        break;
    default:
        usage(1);
        break;
    }
    if (nb_errors) exit(1);
    output_file_name = NULL;
    return 0;
}
