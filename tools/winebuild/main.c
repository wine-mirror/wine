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
#include "wine/port.h"

#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

#include "build.h"

int UsePIC = 0;
int nb_errors = 0;
int display_warnings = 0;
int kill_at = 0;
int verbose = 0;
int link_ext_symbols = 0;
int force_pointer_size = 0;
int unwind_tables = 0;
int use_msvcrt = 0;
int unix_lib = 0;
int safe_seh = 0;
int prefer_native = 0;

#ifdef __i386__
enum target_cpu target_cpu = CPU_x86;
#elif defined(__x86_64__)
enum target_cpu target_cpu = CPU_x86_64;
#elif defined(__powerpc__)
enum target_cpu target_cpu = CPU_POWERPC;
#elif defined(__arm__)
enum target_cpu target_cpu = CPU_ARM;
#elif defined(__aarch64__)
enum target_cpu target_cpu = CPU_ARM64;
#else
#error Unsupported CPU
#endif

#ifdef __APPLE__
enum target_platform target_platform = PLATFORM_APPLE;
#elif defined(__linux__)
enum target_platform target_platform = PLATFORM_LINUX;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
enum target_platform target_platform = PLATFORM_FREEBSD;
#elif defined(__sun)
enum target_platform target_platform = PLATFORM_SOLARIS;
#elif defined(_WIN32)
enum target_platform target_platform = PLATFORM_MINGW;
#else
enum target_platform target_platform = PLATFORM_UNSPECIFIED;
#endif

char *target_alias = NULL;

char *input_file_name = NULL;
char *spec_file_name = NULL;
FILE *output_file = NULL;
const char *output_file_name = NULL;
static int fake_module;

struct strarray lib_path = { 0 };
struct strarray tools_path = { 0 };
struct strarray as_command = { 0 };
struct strarray cc_command = { 0 };
struct strarray ld_command = { 0 };
struct strarray nm_command = { 0 };
char *cpu_option = NULL;
char *fpu_option = NULL;
char *arch_option = NULL;
#ifdef __SOFTFP__
const char *float_abi_option = "soft";
#else
const char *float_abi_option = "softfp";
#endif

#ifdef __thumb__
int thumb_mode = 1;
#else
int thumb_mode = 0;
#endif

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

static const struct
{
    const char *name;
    enum target_platform platform;
} platform_names[] =
{
    { "macos",       PLATFORM_APPLE },
    { "darwin",      PLATFORM_APPLE },
    { "linux",       PLATFORM_LINUX },
    { "freebsd",     PLATFORM_FREEBSD },
    { "solaris",     PLATFORM_SOLARIS },
    { "mingw32",     PLATFORM_MINGW },
    { "windows-gnu", PLATFORM_MINGW },
    { "windows",     PLATFORM_WINDOWS },
    { "winnt",       PLATFORM_MINGW }
};

/* set the dll file name from the input file name */
static void set_dll_file_name( const char *name, DLLSPEC *spec )
{
    char *p;

    if (spec->file_name) return;

    if ((p = strrchr( name, '\\' ))) name = p + 1;
    if ((p = strrchr( name, '/' ))) name = p + 1;
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
static void set_target( const char *target )
{
    unsigned int i;
    char *p, *spec = xstrdup( target );

    /* target specification is in the form CPU-MANUFACTURER-OS or CPU-MANUFACTURER-KERNEL-OS */

    target_alias = xstrdup( target );

    /* get the CPU part */

    if ((p = strchr( spec, '-' )))
    {
        int cpu;

        *p++ = 0;
        cpu = get_cpu_from_name( spec );
        if (cpu == -1) fatal_error( "Unrecognized CPU '%s'\n", spec );
        target_cpu = cpu;
    }
    else if (!strcmp( spec, "mingw32" ))
    {
        target_cpu = CPU_x86;
        p = spec;
    }
    else
        fatal_error( "Invalid target specification '%s'\n", target );

    /* get the OS part */

    target_platform = PLATFORM_UNSPECIFIED;  /* default value */
    for (;;)
    {
        for (i = 0; i < ARRAY_SIZE(platform_names); i++)
        {
            if (!strncmp( platform_names[i].name, p, strlen(platform_names[i].name) ))
            {
                target_platform = platform_names[i].platform;
                break;
            }
        }
        if (target_platform != PLATFORM_UNSPECIFIED || !(p = strchr( p, '-' ))) break;
        p++;
    }

    free( spec );

    if (target_cpu == CPU_ARM && is_pe()) thumb_mode = 1;
}

/* cleanup on program exit */
static void cleanup(void)
{
    if (output_file_name) unlink( output_file_name );
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
"   -l, --library=LIB         Import the specified library\n"
"   -L, --library-path=DIR    Look for imports libraries in DIR\n"
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
    LONG_OPT_VERSION
};

static const char short_options[] = "B:C:D:E:F:H:I:K:L:M:N:b:d:e:f:hkl:m:o:r:u:vw";

static const struct option long_options[] =
{
    { "dll",           0, 0, LONG_OPT_DLL },
    { "def",           0, 0, LONG_OPT_DEF },
    { "exe",           0, 0, LONG_OPT_EXE },
    { "implib",        0, 0, LONG_OPT_IMPLIB },
    { "staticlib",     0, 0, LONG_OPT_STATICLIB },
    { "builtin",       0, 0, LONG_OPT_BUILTIN },
    { "as-cmd",        1, 0, LONG_OPT_ASCMD },
    { "cc-cmd",        1, 0, LONG_OPT_CCCMD },
    { "external-symbols", 0, 0, LONG_OPT_EXTERNAL_SYMS },
    { "fake-module",   0, 0, LONG_OPT_FAKE_MODULE },
    { "fixup-ctors",   0, 0, LONG_OPT_FIXUP_CTORS },
    { "large-address-aware", 0, 0, LONG_OPT_LARGE_ADDRESS_AWARE },
    { "ld-cmd",        1, 0, LONG_OPT_LDCMD },
    { "nm-cmd",        1, 0, LONG_OPT_NMCMD },
    { "nxcompat",      1, 0, LONG_OPT_NXCOMPAT },
    { "prefer-native", 0, 0, LONG_OPT_PREFER_NATIVE },
    { "resources",     0, 0, LONG_OPT_RESOURCES },
    { "safeseh",       0, 0, LONG_OPT_SAFE_SEH },
    { "save-temps",    0, 0, LONG_OPT_SAVE_TEMPS },
    { "subsystem",     1, 0, LONG_OPT_SUBSYSTEM },
    { "version",       0, 0, LONG_OPT_VERSION },
    /* aliases for short options */
    { "target",        1, 0, 'b' },
    { "delay-lib",     1, 0, 'd' },
    { "export",        1, 0, 'E' },
    { "entry",         1, 0, 'e' },
    { "filename",      1, 0, 'F' },
    { "help",          0, 0, 'h' },
    { "heap",          1, 0, 'H' },
    { "kill-at",       0, 0, 'k' },
    { "library",       1, 0, 'l' },
    { "library-path",  1, 0, 'L' },
    { "main-module",   1, 0, 'M' },
    { "dll-name",      1, 0, 'N' },
    { "output",        1, 0, 'o' },
    { "res",           1, 0, 'r' },
    { "undefined",     1, 0, 'u' },
    { "verbose",       0, 0, 'v' },
    { "warnings",      0, 0, 'w' },
    { NULL,            0, 0, 0 }
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
    if (spec->characteristics & IMAGE_FILE_DLL) return "DllMain";
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

/* parse options from the argv array and remove all the recognized ones */
static char **parse_options( int argc, char **argv, DLLSPEC *spec )
{
    char *p;
    int optc;
    int save_temps = 0;

    while ((optc = getopt_long( argc, argv, short_options, long_options, NULL )) != -1)
    {
        switch(optc)
        {
        case 'B':
            strarray_add( &tools_path, xstrdup( optarg ), NULL );
            break;
        case 'D':
            /* ignored */
            break;
        case 'E':
            spec_file_name = xstrdup( optarg );
            set_dll_file_name( optarg, spec );
            break;
        case 'F':
            spec->file_name = xstrdup( optarg );
            break;
        case 'H':
            if (!isdigit(optarg[0]))
                fatal_error( "Expected number argument with -H option instead of '%s'\n", optarg );
            spec->heap_size = atoi(optarg);
            if (spec->heap_size > 65535)
                fatal_error( "Invalid heap size %d, maximum is 65535\n", spec->heap_size );
            break;
        case 'I':
            /* ignored */
            break;
        case 'K':
            /* ignored, because cc generates correct code. */
            break;
        case 'L':
            strarray_add( &lib_path, xstrdup( optarg ), NULL );
            break;
        case 'm':
            if (!strcmp( optarg, "16" )) spec->type = SPEC_WIN16;
            else if (!strcmp( optarg, "32" )) force_pointer_size = 4;
            else if (!strcmp( optarg, "64" )) force_pointer_size = 8;
            else if (!strcmp( optarg, "arm" )) thumb_mode = 0;
            else if (!strcmp( optarg, "thumb" )) thumb_mode = 1;
            else if (!strcmp( optarg, "no-cygwin" )) use_msvcrt = 1;
            else if (!strcmp( optarg, "unix" )) unix_lib = 1;
            else if (!strcmp( optarg, "unicode" )) spec->unicode_app = 1;
            else if (!strncmp( optarg, "cpu=", 4 )) cpu_option = xstrdup( optarg + 4 );
            else if (!strncmp( optarg, "fpu=", 4 )) fpu_option = xstrdup( optarg + 4 );
            else if (!strncmp( optarg, "arch=", 5 )) arch_option = xstrdup( optarg + 5 );
            else if (!strncmp( optarg, "float-abi=", 10 )) float_abi_option = xstrdup( optarg + 10 );
            else fatal_error( "Unknown -m option '%s'\n", optarg );
            break;
        case 'M':
            spec->main_module = xstrdup( optarg );
            break;
        case 'N':
            spec->dll_name = xstrdup( optarg );
            break;
        case 'b':
            set_target( optarg );
            break;
        case 'd':
            add_delayed_import( optarg );
            break;
        case 'e':
            spec->init_func = xstrdup( optarg );
            if ((p = strchr( spec->init_func, '@' ))) *p = 0;  /* kill stdcall decoration */
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
        case 'l':
            add_import_dll( optarg, NULL );
            break;
        case 'o':
            if (unlink( optarg ) == -1 && errno != ENOENT)
                fatal_error( "Unable to create output file '%s'\n", optarg );
            output_file_name = xstrdup( optarg );
            break;
        case 'r':
            strarray_add( &res_files, xstrdup( optarg ), NULL );
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
            if (!spec->subsystem) spec->subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
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
        case LONG_OPT_FAKE_MODULE:
            fake_module = 1;
            break;
        case LONG_OPT_EXTERNAL_SYMS:
            link_ext_symbols = 1;
            break;
        case LONG_OPT_LARGE_ADDRESS_AWARE:
            spec->characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
            break;
        case LONG_OPT_LDCMD:
            ld_command = strarray_fromstring( optarg, " " );
            break;
        case LONG_OPT_NMCMD:
            nm_command = strarray_fromstring( optarg, " " );
            break;
        case LONG_OPT_NXCOMPAT:
            if (optarg[0] == 'n' || optarg[0] == 'N')
                spec->dll_characteristics &= ~IMAGE_DLLCHARACTERISTICS_NX_COMPAT;
            break;
        case LONG_OPT_SAFE_SEH:
            safe_seh = 1;
            break;
        case LONG_OPT_PREFER_NATIVE:
            prefer_native = 1;
            spec->dll_characteristics |= IMAGE_DLLCHARACTERISTICS_PREFER_NATIVE;
            break;
        case LONG_OPT_RESOURCES:
            set_exec_mode( MODE_RESOURCES );
            break;
        case LONG_OPT_SAVE_TEMPS:
            save_temps = 1;
            break;
        case LONG_OPT_SUBSYSTEM:
            set_subsystem( optarg, spec );
            break;
        case LONG_OPT_VERSION:
            printf( "winebuild version " PACKAGE_VERSION "\n" );
            exit(0);
        case '?':
            usage(1);
            break;
        }
    }

    if (!save_temps) atexit( cleanup_tmp_files );

    if (spec->file_name && !strchr( spec->file_name, '.' ))
        strcat( spec->file_name, exec_mode == MODE_EXE ? ".exe" : ".dll" );
    init_dll_name( spec );

    switch (target_cpu)
    {
    case CPU_x86:
        if (force_pointer_size == 8) target_cpu = CPU_x86_64;
        break;
    case CPU_x86_64:
        if (force_pointer_size == 4) target_cpu = CPU_x86;
        break;
    default:
        if (force_pointer_size == 8)
            fatal_error( "Cannot build 64-bit code for this CPU\n" );
        break;
    }

    return &argv[optind];
}


/* load all specified resource files */
static void load_resources( char *argv[], DLLSPEC *spec )
{
    int i;
    char **ptr, **last;

    switch (spec->type)
    {
    case SPEC_WIN16:
        for (i = 0; i < res_files.count; i++) load_res16_file( res_files.str[i], spec );
        break;

    case SPEC_WIN32:
        for (i = 0; i < res_files.count; i++)
        {
            if (!load_res32_file( res_files.str[i], spec ))
                fatal_error( "%s is not a valid Win32 resource file\n", res_files.str[i] );
        }

        /* load any resource file found in the remaining arguments */
        for (ptr = last = argv; *ptr; ptr++)
        {
            if (!load_res32_file( *ptr, spec ))
                *last++ = *ptr; /* not a resource file, keep it in the list */
        }
        *last = NULL;
        break;
    }
}

/* add input files that look like import libs to the import list */
static void load_import_libs( char *argv[] )
{
    char **ptr, **last;

    for (ptr = last = argv; *ptr; ptr++)
    {
        if (strendswith( *ptr, ".def" ))
            add_import_dll( NULL, *ptr );
        else
            *last++ = *ptr; /* not an import dll, keep it in the list */
    }
    *last = NULL;
}

static int parse_input_file( DLLSPEC *spec )
{
    FILE *input_file = open_input_file( NULL, spec_file_name );
    char *extension = strrchr( spec_file_name, '.' );
    int result;

    spec->src_name = xstrdup( input_file_name );
    if (extension && !strcmp( extension, ".def" ))
        result = parse_def_file( input_file, spec );
    else
        result = parse_spec_file( input_file, spec );
    close_input_file( input_file );
    return result;
}


/*******************************************************************
 *         main
 */
int main(int argc, char **argv)
{
    DLLSPEC *spec = alloc_dll_spec();

#ifdef SIGHUP
    signal( SIGHUP, exit_on_signal );
#endif
    signal( SIGTERM, exit_on_signal );
    signal( SIGINT, exit_on_signal );

    argv = parse_options( argc, argv, spec );
    atexit( cleanup );  /* make sure we remove the output file on exit */

    switch(exec_mode)
    {
    case MODE_DLL:
        if (spec->subsystem != IMAGE_SUBSYSTEM_NATIVE)
            spec->characteristics |= IMAGE_FILE_DLL;
        /* fall through */
    case MODE_EXE:
        load_resources( argv, spec );
        if (spec_file_name && !parse_input_file( spec )) break;
        if (!spec->init_func && !unix_lib) spec->init_func = xstrdup( get_default_entry_point( spec ));

        if (fake_module)
        {
            if (spec->type == SPEC_WIN16) output_fake_module16( spec );
            else output_fake_module( spec );
            break;
        }
        if (!is_pe())
        {
            load_import_libs( argv );
            read_undef_symbols( spec, argv );
            resolve_imports( spec );
        }
        if (spec->type == SPEC_WIN16) output_spec16_file( spec );
        else output_spec32_file( spec );
        break;
    case MODE_DEF:
        if (argv[0]) fatal_error( "file argument '%s' not allowed in this mode\n", argv[0] );
        if (!spec_file_name) fatal_error( "missing .spec file\n" );
        if (!parse_input_file( spec )) break;
        open_output_file();
        output_def_file( spec, 0 );
        close_output_file();
        break;
    case MODE_IMPLIB:
        if (!spec_file_name) fatal_error( "missing .spec file\n" );
        if (!parse_input_file( spec )) break;
        output_static_lib( spec, argv );
        break;
    case MODE_STATICLIB:
        output_static_lib( NULL, argv );
        break;
    case MODE_BUILTIN:
        if (!argv[0]) fatal_error( "missing file argument for --builtin option\n" );
        make_builtin_files( argv );
        break;
    case MODE_FIXUP_CTORS:
        if (!argv[0]) fatal_error( "missing file argument for --fixup-ctors option\n" );
        fixup_constructors( argv );
        break;
    case MODE_RESOURCES:
        load_resources( argv, spec );
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
