/*
 * Generate include file dependencies
 *
 * Copyright 1996, 2013, 2020 Alexandre Julliard
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
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "tools.h"
#include "wine/list.h"

enum incl_type
{
    INCL_NORMAL,           /* #include "foo.h" */
    INCL_SYSTEM,           /* #include <foo.h> */
    INCL_IMPORT,           /* idl import "foo.idl" */
    INCL_IMPORTLIB,        /* idl importlib "foo.tlb" */
    INCL_CPP_QUOTE,        /* idl cpp_quote("#include \"foo.h\"") */
    INCL_CPP_QUOTE_SYSTEM  /* idl cpp_quote("#include <foo.h>") */
};

struct dependency
{
    int                line;          /* source line where this header is included */
    enum incl_type     type;          /* type of include */
    char              *name;          /* header name */
};

struct file
{
    struct list        entry;
    char              *name;          /* full file name relative to cwd */
    void              *args;          /* custom arguments for makefile rule */
    unsigned int       flags;         /* flags (see below) */
    unsigned int       deps_count;    /* files in use */
    unsigned int       deps_size;     /* total allocated size */
    struct dependency *deps;          /* all header dependencies */
};

struct incl_file
{
    struct list        entry;
    struct list        hash_entry;
    struct file       *file;
    char              *name;
    char              *filename;
    char              *basename;      /* base target name for generated files */
    char              *sourcename;    /* source file name for generated headers */
    struct incl_file  *included_by;   /* file that included this one */
    int                included_line; /* line where this file was included */
    enum incl_type     type;          /* type of include */
    unsigned int       arch;          /* architecture for multi-arch files, otherwise 0 */
    unsigned int       use_msvcrt:1;  /* put msvcrt headers in the search path? */
    unsigned int       is_external:1; /* file from external library? */
    struct incl_file  *owner;
    unsigned int       files_count;   /* files in use */
    unsigned int       files_size;    /* total allocated size */
    struct incl_file **files;
    struct strarray    dependencies;  /* file dependencies */
    struct strarray    importlibdeps; /* importlib dependencies */
};

#define FLAG_GENERATED      0x00000001  /* generated file */
#define FLAG_INSTALL        0x00000002  /* file to install */
#define FLAG_TESTDLL        0x00000004  /* file is part of a TESTDLL resource */
#define FLAG_IDL_PROXY      0x00000100  /* generates a proxy (_p.c) file */
#define FLAG_IDL_CLIENT     0x00000200  /* generates a client (_c.c) file */
#define FLAG_IDL_SERVER     0x00000400  /* generates a server (_s.c) file */
#define FLAG_IDL_IDENT      0x00000800  /* generates an ident (_i.c) file */
#define FLAG_IDL_REGISTER   0x00001000  /* generates a registration (_r.res) file */
#define FLAG_IDL_TYPELIB    0x00002000  /* generates a typelib (_l.res) file */
#define FLAG_IDL_REGTYPELIB 0x00004000  /* generates a registered typelib (_t.res) file */
#define FLAG_IDL_HEADER     0x00008000  /* generates a header (.h) file */
#define FLAG_IDL_WINMD      0x00010000  /* generates a metadata (.winmd) file */
#define FLAG_RC_PO          0x00100000  /* rc file contains translations */
#define FLAG_RC_HEADER      0x00200000  /* rc file is a header */
#define FLAG_SFD_FONTS      0x00400000  /* sfd file generated bitmap fonts */
#define FLAG_C_IMPLIB       0x01000000  /* file is part of an import library */
#define FLAG_C_UNIX         0x02000000  /* file is part of a Unix library */
#define FLAG_ARM64EC_X64    0x04000000  /* use x86_64 object on ARM64EC */

static const struct
{
    unsigned int flag;
    const char *ext;
} idl_outputs[] =
{
    { FLAG_IDL_TYPELIB,    "_l.res" },
    { FLAG_IDL_REGTYPELIB, "_t.res" },
    { FLAG_IDL_CLIENT,     "_c.c" },
    { FLAG_IDL_IDENT,      "_i.c" },
    { FLAG_IDL_PROXY,      "_p.c" },
    { FLAG_IDL_SERVER,     "_s.c" },
    { FLAG_IDL_REGISTER,   "_r.res" },
};

#define HASH_SIZE 197

static struct list files[HASH_SIZE];
static struct list global_includes[HASH_SIZE];

enum install_rules { INSTALL_LIB, INSTALL_DEV, INSTALL_TEST, NB_INSTALL_RULES };
static const char *install_targets[NB_INSTALL_RULES] = { "install-lib", "install-dev", "install-test" };
static const char *install_variables[NB_INSTALL_RULES] = { "INSTALL_LIB", "INSTALL_DEV", "INSTALL_TEST" };

#define MAX_ARCHS 6

/* variables common to all makefiles */
static struct strarray archs;
static struct strarray linguas;
static struct strarray dll_flags;
static struct strarray unix_dllflags;
static struct strarray msvcrt_flags;
static struct strarray cpp_flags;
static struct strarray lddll_flags;
static struct strarray libs;
static struct strarray enable_tests;
static struct strarray cmdline_vars;
static struct strarray subdirs;
static struct strarray delay_import_libs;
static struct strarray top_install[NB_INSTALL_RULES];
static const char *root_src_dir;
static const char *tools_dir;
static const char *tools_ext;
static const char *wine64_dir;
static const char *exe_ext;
static const char *fontforge;
static const char *convert;
static const char *flex;
static const char *bison;
static const char *rsvg;
static const char *icotool;
static const char *msgfmt;
static const char *ln_s;
static const char *sed_cmd;
static const char *wayland_scanner;
static const char *sarif_converter;
static const char *compiler_rt;
static const char *buildimage;
static const char *runtest;
static const char *install_sh;
static const char *makedep;
static const char *make_xftmpl;
static const char *sfnt2fon;
static const char *winebuild;
static const char *winegcc;
static const char *widl;
static const char *wrc;
static const char *wmc;
static int so_dll_supported;
static int unix_lib_supported;
/* per-architecture global variables */
static const char *dll_ext[MAX_ARCHS];
static const char *arch_dirs[MAX_ARCHS];
static const char *arch_pe_dirs[MAX_ARCHS];
static const char *arch_install_dirs[MAX_ARCHS];
static const char *strip_progs[MAX_ARCHS];
static const char *delay_load_flags[MAX_ARCHS];
static struct strarray debug_flags[MAX_ARCHS];
static struct strarray target_flags[MAX_ARCHS];
static struct strarray extra_cflags[MAX_ARCHS];
static struct strarray extra_cflags_extlib[MAX_ARCHS];
static struct strarray disabled_dirs[MAX_ARCHS];
static unsigned int native_archs[MAX_ARCHS];
static unsigned int hybrid_archs[MAX_ARCHS];
static struct strarray hybrid_target_flags[MAX_ARCHS];

struct makefile
{
    /* values determined from input makefile */
    struct strarray vars;
    struct strarray include_paths;
    struct strarray include_args;
    struct strarray define_args;
    struct strarray unix_cflags;
    struct strarray programs;
    struct strarray scripts;
    struct strarray imports;
    struct strarray delayimports;
    struct strarray extradllflags;
    struct strarray install[NB_INSTALL_RULES];
    struct strarray extra_targets;
    struct strarray extra_imports;
    struct list     sources;
    struct list     includes;
    const char     *src_dir;
    const char     *obj_dir;
    const char     *parent_dir;
    const char     *module;
    const char     *testdll;
    const char     *extlib;
    const char     *staticlib;
    const char     *importlib;
    const char     *unixlib;
    int             data_only;
    int             is_win16;
    int             is_exe;
    int             disabled[MAX_ARCHS];

    /* values generated at output time */
    struct strarray in_files;
    struct strarray pot_files;
    struct strarray test_files;
    struct strarray sast_files;
    struct strarray clean_files;
    struct strarray distclean_files;
    struct strarray maintainerclean_files;
    struct strarray uninstall_files;
    struct strarray unixobj_files;
    struct strarray font_files;
    struct strarray debug_files;
    struct strarray dlldata_files;
    struct strarray phony_targets;
    struct strarray dependencies;
    struct strarray object_files[MAX_ARCHS];
    struct strarray implib_files[MAX_ARCHS];
    struct strarray ok_files[MAX_ARCHS];
    struct strarray res_files[MAX_ARCHS];
    struct strarray all_targets[MAX_ARCHS];
    struct strarray install_rules[NB_INSTALL_RULES];
};

static struct makefile *top_makefile;
static struct makefile *include_makefile;
static struct makefile **submakes;

static const char separator[] = "### Dependencies";
static const char *output_makefile_name = "Makefile";
static const char *input_makefile_name;
static const char *input_file_name;
static const char *output_file_name;
static const char *temp_file_name;
static char cwd[PATH_MAX];
static int compile_commands_mode;
static int silent_rules;
static int input_line;
static int output_column;
static FILE *output_file;

struct compile_command
{
    struct list      entry;
    const char      *cmd;
    const char      *source;
    const char      *obj;
    struct strarray args;
};

static struct list compile_commands = LIST_INIT( compile_commands );

static const char Usage[] =
    "Usage: makedep [options]\n"
    "Options:\n"
    "   -C          Generate compile_commands.json along with the makefile\n"
    "   -S          Generate Automake-style silent rules\n"
    "   -fxxx       Store output in file 'xxx' (default: Makefile)\n";


static void fatal_error( const char *msg, ... ) __attribute__ ((__format__ (__printf__, 1, 2)));
static void fatal_perror( const char *msg, ... ) __attribute__ ((__format__ (__printf__, 1, 2)));
static void output( const char *format, ... ) __attribute__ ((__format__ (__printf__, 1, 2)));
static char *strmake( const char* fmt, ... ) __attribute__ ((__format__ (__printf__, 1, 2)));

/*******************************************************************
 *         fatal_error
 */
static void fatal_error( const char *msg, ... )
{
    va_list valist;
    va_start( valist, msg );
    if (input_file_name)
    {
        fprintf( stderr, "%s:", input_file_name );
        if (input_line) fprintf( stderr, "%d:", input_line );
        fprintf( stderr, " error: " );
    }
    else fprintf( stderr, "makedep: error: " );
    vfprintf( stderr, msg, valist );
    va_end( valist );
    exit(1);
}


/*******************************************************************
 *         fatal_perror
 */
static void fatal_perror( const char *msg, ... )
{
    va_list valist;
    va_start( valist, msg );
    if (input_file_name)
    {
        fprintf( stderr, "%s:", input_file_name );
        if (input_line) fprintf( stderr, "%d:", input_line );
        fprintf( stderr, " error: " );
    }
    else fprintf( stderr, "makedep: error: " );
    vfprintf( stderr, msg, valist );
    perror( " " );
    va_end( valist );
    exit(1);
}


/*******************************************************************
 *         cleanup_files
 */
static void cleanup_files(void)
{
    if (temp_file_name) unlink( temp_file_name );
    if (output_file_name) unlink( output_file_name );
}


/*******************************************************************
 *         exit_on_signal
 */
static void exit_on_signal( int sig )
{
    exit( 1 );  /* this will call the atexit functions */
}


/*******************************************************************
 *         output
 */
static void output( const char *format, ... )
{
    int ret;
    va_list valist;

    va_start( valist, format );
    ret = vfprintf( output_file, format, valist );
    va_end( valist );
    if (ret < 0) fatal_perror( "output" );
    if (format[0] && format[strlen(format) - 1] == '\n') output_column = 0;
    else output_column += ret;
}


/*******************************************************************
 *         strarray_get_value
 *
 * Find a value in a name/value pair string array.
 */
static const char *strarray_get_value( struct strarray array, const char *name )
{
    int pos, res, min = 0, max = array.count / 2 - 1;

    while (min <= max)
    {
        pos = (min + max) / 2;
        if (!(res = strcmp( array.str[pos * 2], name ))) return array.str[pos * 2 + 1];
        if (res < 0) min = pos + 1;
        else max = pos - 1;
    }
    return NULL;
}


/*******************************************************************
 *         strarray_set_value
 *
 * Define a value in a name/value pair string array.
 */
static void strarray_set_value( struct strarray *array, const char *name, const char *value )
{
    int i, pos, res, min = 0, max = array->count / 2 - 1;

    while (min <= max)
    {
        pos = (min + max) / 2;
        if (!(res = strcmp( array->str[pos * 2], name )))
        {
            /* redefining a variable replaces the previous value */
            array->str[pos * 2 + 1] = value;
            return;
        }
        if (res < 0) min = pos + 1;
        else max = pos - 1;
    }
    strarray_add( array, NULL );
    strarray_add( array, NULL );
    for (i = array->count - 1; i > min * 2 + 1; i--) array->str[i] = array->str[i - 2];
    array->str[min * 2] = name;
    array->str[min * 2 + 1] = value;
}


/*******************************************************************
 *         output_filename
 */
static void output_filename( const char *name )
{
    if (output_column + strlen(name) + 1 > 100)
    {
        output( " \\\n" );
        output( "  " );
    }
    else if (output_column) output( " " );
    output( "%s", name );
}


/*******************************************************************
 *         output_filenames
 */
static void output_filenames( struct strarray array )
{
    unsigned int i;

    for (i = 0; i < array.count; i++) output_filename( array.str[i] );
}


/*******************************************************************
 *         output_rm_filenames
 */
static void output_rm_filenames( struct strarray array, const char *command )
{
    static const unsigned int max_cmdline = 30000;  /* to be on the safe side */
    unsigned int i, len;

    if (!array.count) return;
    output( "\t%s", command );
    for (i = len = 0; i < array.count; i++)
    {
        if (len > max_cmdline)
        {
            output( "\n" );
            output( "\t%s", command );
            len = 0;
        }
        output_filename( array.str[i] );
        len += strlen( array.str[i] ) + 1;
    }
    output( "\n" );
}


/*******************************************************************
 *         get_extension
 */
static char *get_extension( char *filename )
{
    char *ext = strrchr( filename, '.' );
    if (ext && strchr( ext, '/' )) ext = NULL;
    return ext;
}


/*******************************************************************
 *         get_base_name
 */
static const char *get_base_name( const char *name )
{
    char *base;
    if (!strchr( name, '.' )) return name;
    base = xstrdup( name );
    *strrchr( base, '.' ) = 0;
    return base;
}


/*******************************************************************
 *         replace_filename
 */
static char *replace_filename( const char *path, const char *name )
{
    const char *p;
    char *ret;
    size_t len;

    if (!path) return xstrdup( name );
    if (!(p = strrchr( path, '/' ))) return xstrdup( name );
    len = p - path + 1;
    ret = xmalloc( len + strlen( name ) + 1 );
    memcpy( ret, path, len );
    strcpy( ret + len, name );
    return ret;
}


/*******************************************************************
 *         replace_substr
 */
static char *replace_substr( const char *str, const char *start, size_t len, const char *replace )
{
    size_t pos = start - str;
    char *ret = xmalloc( pos + strlen(replace) + strlen(start + len) + 1 );
    memcpy( ret, str, pos );
    strcpy( ret + pos, replace );
    strcat( ret + pos, start + len );
    return ret;
}


/*******************************************************************
 *         get_root_relative_path
 *
 * Get relative path from obj dir to root.
 */
static const char *get_root_relative_path( struct makefile *make )
{
    const char *dir = make->obj_dir;
    char *ret, *p;
    unsigned int dotdots = 0;

    if (!dir) return ".";
    while (*dir)
    {
        dotdots++;
        while (*dir && *dir != '/') dir++;
        while (*dir == '/') dir++;
    }
    assert(dotdots);
    ret = xmalloc( 3 * dotdots );
    for (p = ret; dotdots; dotdots--, p += 3) memcpy( p, "../", 3 );
    p[-1] = 0;  /* remove trailing slash */
    return ret;
}


/*******************************************************************
 *         concat_paths
 */
static char *concat_paths( const char *base, const char *path )
{
    int i, len;
    char *ret;

    if (!base || !base[0]) return xstrdup( path && path[0] ? path : "." );
    if (!path || !path[0]) return xstrdup( base );
    if (path[0] == '/') return xstrdup( path );

    len = strlen( base );
    while (len && base[len - 1] == '/') len--;
    while (len && !strncmp( path, "..", 2 ) && (!path[2] || path[2] == '/'))
    {
        for (i = len; i > 0; i--) if (base[i - 1] == '/') break;
        if (i == len - 2 && !memcmp( base + i, "..", 2 )) break;  /* we can't go up if we already have ".." */
        if (i != len - 1 || base[i] != '.')
        {
            path += 2;
            while (*path == '/') path++;
        }
        /* else ignore "." element */
        while (i > 0 && base[i - 1] == '/') i--;
        len = i;
    }
    if (!len && base[0] != '/') return xstrdup( path[0] ? path : "." );
    ret = xmalloc( len + strlen( path ) + 2 );
    memcpy( ret, base, len );
    ret[len++] = '/';
    strcpy( ret + len, path );
    return ret;
}


/*******************************************************************
 *         escape_cstring
 */
static const char *escape_cstring( const char *str )
{
    char *ret;
    unsigned int i = 0, j = 0;

    if (!strpbrk( str, "\\\"" )) return str;
    ret = xmalloc( 2 * strlen(str) + 1 );
    while (str[i])
    {
        if (str[i] == '\\' || str[i] == '"') ret[j++] = '\\';
        ret[j++] = str[i++];
    }
    ret[j] = 0;
    return ret;
}


/*******************************************************************
 *         is_native_arch_disabled
 *
 * Check if the makefile was disabled for a PE arch that matches the native arch.
 */
static int is_native_arch_disabled( struct makefile *make )
{
    unsigned int arch;

    if (archs.count == 1) return 0;
    if (!so_dll_supported) return 1;

    for (arch = 1; arch < archs.count; arch++)
        if (make->disabled[arch] && !strcmp( archs.str[0], archs.str[arch] ))
            return 1;
    return 0;
}


/*******************************************************************
 *         is_subdir_other_arch
 *
 * Check if the filename is in a subdirectory named from a different arch.
 * Used to avoid building asm files for the wrong platform.
 */
static int is_subdir_other_arch( const char *name, unsigned int arch )
{
    const char *dir, *p = strrchr( name, '/' );

    if (!p || p == name) return 0;
    dir = get_basename( strmake( "%.*s", (int)(p - name), name ));
    if (!strcmp( dir, "arm64" )) dir = "aarch64";
    if (!strcmp( dir, "amd64" )) dir = "x86_64";
    if (native_archs[arch] && !strcmp( dir, archs.str[native_archs[arch]] )) return 0;
    return strcmp( dir, archs.str[arch] );
}


/*******************************************************************
 *         get_link_arch
 */
static int get_link_arch( const struct makefile *make, unsigned int arch, unsigned int *link_arch )
{
    unsigned int hybrid_arch = hybrid_archs[arch];

    if (native_archs[arch]) return 0;
    if (hybrid_arch && make->disabled[hybrid_arch]) hybrid_arch = 0;
    if (make->disabled[arch] && !hybrid_arch) return 0;
    *link_arch = hybrid_arch ? hybrid_arch : arch;
    return 1;
}


/*******************************************************************
 *         is_multiarch
 *
 * Check if arch is one of the PE architectures in multiarch.
 * Also return TRUE for native arch iff there's no PE architecture, not even "none".
 */
static int is_multiarch( unsigned int arch )
{
    return archs.count == 1 || (arch && strcmp( archs.str[arch], "none" ));
}


/*******************************************************************
 *         is_using_msvcrt
 *
 * Check if the files of a makefile use msvcrt by default.
 */
static int is_using_msvcrt( struct makefile *make )
{
    return make->module || make->testdll;
}


/*******************************************************************
 *         arch_module_name
 */
static char *arch_module_name( const char *module, unsigned int arch )
{
    return strmake( "%s%s%s", arch_dirs[arch], module, dll_ext[arch] );
}


/*******************************************************************
 *         arch_make_variable
 */
static char *arch_make_variable( const char *name, unsigned int arch )
{
    return arch ? strmake( "$(%s_%s)", archs.str[arch], name ) : strmake( "$(%s)", name );
}


/*******************************************************************
 *         obj_dir_path
 */
static char *obj_dir_path( const struct makefile *make, const char *path )
{
    return concat_paths( make->obj_dir, path );
}


/*******************************************************************
 *         src_dir_path
 */
static char *src_dir_path( const struct makefile *make, const char *path )
{
    if (make->src_dir) return concat_paths( make->src_dir, path );
    return obj_dir_path( make, path );
}


/*******************************************************************
 *         root_src_dir_path
 */
static char *root_src_dir_path( const char *path )
{
    return concat_paths( root_src_dir, path );
}


/*******************************************************************
 *         tools_dir_path
 */
static char *tools_dir_path( const char *path )
{
    if (tools_dir) return strmake( "%s/tools/%s", tools_dir, path );
    return strmake( "tools/%s", path );
}


/*******************************************************************
 *         tools_path
 */
static char *tools_path( const char *name )
{
    return strmake( "%s/%s%s", tools_dir_path( name ), name, tools_ext );
}


/*******************************************************************
 *         tools_base_path
 */
static char *tools_base_path( const char *name )
{
    return strmake( "%s%s", tools_dir_path( name ), tools_ext );
}


/*******************************************************************
 *         strarray_addall_path
 */
static void strarray_addall_path( struct strarray *array, const char *dir, struct strarray added )
{
    unsigned int i;

    for (i = 0; i < added.count; i++) strarray_add( array, concat_paths( dir, added.str[i] ));
}


/*******************************************************************
 *         get_line
 */
static char *get_line( FILE *file )
{
    static char *buffer;
    static size_t size;

    if (!size)
    {
        size = 1024;
        buffer = xmalloc( size );
    }
    if (!fgets( buffer, size, file )) return NULL;
    input_line++;

    for (;;)
    {
        char *p = buffer + strlen(buffer);
        /* if line is larger than buffer, resize buffer */
        while (p == buffer + size - 1 && p[-1] != '\n')
        {
            buffer = xrealloc( buffer, size * 2 );
            if (!fgets( buffer + size - 1, size + 1, file )) break;
            p = buffer + strlen(buffer);
            size *= 2;
        }
        if (p > buffer && p[-1] == '\n')
        {
            *(--p) = 0;
            if (p > buffer && p[-1] == '\r') *(--p) = 0;
            if (p > buffer && p[-1] == '\\')
            {
                *(--p) = 0;
                /* line ends in backslash, read continuation line */
                if (!fgets( p, size - (p - buffer), file )) return buffer;
                input_line++;
                continue;
            }
        }
        return buffer;
    }
}


/*******************************************************************
 *         hash_filename
 */
static unsigned int hash_filename( const char *name )
{
    /* FNV-1 hash */
    unsigned int ret = 2166136261u;
    while (*name) ret = (ret * 16777619) ^ *name++;
    return ret % HASH_SIZE;
}


/*******************************************************************
 *         add_file
 */
static struct file *add_file( const char *name )
{
    struct file *file = xmalloc( sizeof(*file) );
    memset( file, 0, sizeof(*file) );
    file->name = xstrdup( name );
    return file;
}


/*******************************************************************
 *         add_dependency
 */
static void add_dependency( struct file *file, const char *name, enum incl_type type )
{
    if (file->deps_count >= file->deps_size)
    {
        file->deps_size *= 2;
        if (file->deps_size < 16) file->deps_size = 16;
        file->deps = xrealloc( file->deps, file->deps_size * sizeof(*file->deps) );
    }
    file->deps[file->deps_count].line = input_line;
    file->deps[file->deps_count].type = type;
    file->deps[file->deps_count].name = xstrdup( name );
    file->deps_count++;
}


/*******************************************************************
 *         find_src_file
 */
static struct incl_file *find_src_file( const struct makefile *make, const char *name )
{
    struct incl_file *file;

    if (make == include_makefile)
    {
        unsigned int hash = hash_filename( name );

        LIST_FOR_EACH_ENTRY( file, &global_includes[hash], struct incl_file, hash_entry )
            if (!strcmp( name, file->name )) return file;
        return NULL;
    }

    LIST_FOR_EACH_ENTRY( file, &make->sources, struct incl_file, entry )
        if (!strcmp( name, file->name )) return file;
    return NULL;
}

/*******************************************************************
 *         find_include_file
 */
static struct incl_file *find_include_file( const struct makefile *make, const char *name )
{
    struct incl_file *file;

    LIST_FOR_EACH_ENTRY( file, &make->includes, struct incl_file, entry )
    {
        const char *filename = file->filename;
        if (!filename) continue;
        if (make->obj_dir && strlen(make->obj_dir) < strlen(filename))
        {
            filename += strlen(make->obj_dir);
            while (*filename == '/') filename++;
        }
        if (!strcmp( name, filename )) return file;
    }
    return NULL;
}

/*******************************************************************
 *         add_include
 *
 * Add an include file if it doesn't already exists.
 */
static struct incl_file *add_include( struct makefile *make, struct incl_file *parent,
                                      const char *name, int line, enum incl_type type )
{
    struct incl_file *include;

    if (parent->files_count >= parent->files_size)
    {
        parent->files_size *= 2;
        if (parent->files_size < 16) parent->files_size = 16;
        parent->files = xrealloc( parent->files, parent->files_size * sizeof(*parent->files) );
    }

    LIST_FOR_EACH_ENTRY( include, &make->includes, struct incl_file, entry )
        if (!parent->use_msvcrt == !include->use_msvcrt && !strcmp( name, include->name ))
            goto found;

    include = xmalloc( sizeof(*include) );
    memset( include, 0, sizeof(*include) );
    include->name = xstrdup(name);
    include->included_by = parent;
    include->included_line = line;
    include->type = type;
    include->use_msvcrt = parent->use_msvcrt;
    list_add_tail( &make->includes, &include->entry );
found:
    parent->files[parent->files_count++] = include;
    return include;
}


/*******************************************************************
 *         add_generated_source
 *
 * Add a generated source file to the list.
 */
static struct incl_file *add_generated_source( struct makefile *make, const char *name,
                                               const char *filename, unsigned int arch )
{
    struct incl_file *file = xmalloc( sizeof(*file) );

    name = strmake( "%s%s", arch_dirs[arch], name );
    memset( file, 0, sizeof(*file) );
    file->file = add_file( name );
    file->arch = arch;
    file->name = xstrdup( name );
    file->basename = xstrdup( filename ? filename : name );
    file->filename = obj_dir_path( make, file->basename );
    file->file->flags = FLAG_GENERATED;
    file->use_msvcrt = is_using_msvcrt( make );
    list_add_tail( &make->sources, &file->entry );
    if (make == include_makefile)
    {
        unsigned int hash = hash_filename( name );
        list_add_tail( &global_includes[hash], &file->hash_entry );
    }
    return file;
}


/*******************************************************************
 *         skip_spaces
 */
static char *skip_spaces( const char *p )
{
    while (*p == ' ' || *p == '\t') p++;
    return (char *)p;
}


/*******************************************************************
 *         parse_include_directive
 */
static void parse_include_directive( struct file *source, char *str )
{
    char quote, *include, *p = skip_spaces( str );

    if (*p != '\"' && *p != '<' ) return;
    quote = *p++;
    if (quote == '<') quote = '>';
    include = p;
    while (*p && (*p != quote)) p++;
    if (!*p) fatal_error( "malformed include directive '%s'\n", str );
    *p = 0;
    add_dependency( source, include, (quote == '>') ? INCL_SYSTEM : INCL_NORMAL );
}


/*******************************************************************
 *         parse_pragma_directive
 */
static void parse_pragma_directive( struct file *source, char *str )
{
    char *flag, *p = str;

    if (*p != ' ' && *p != '\t') return;
    p = strtok( skip_spaces( p ), " \t" );
    if (strcmp( p, "makedep" )) return;

    while ((flag = strtok( NULL, " \t" )))
    {
        if (!strcmp( flag, "depend" ))
        {
            while ((p = strtok( NULL, " \t" ))) add_dependency( source, p, INCL_NORMAL );
            return;
        }
        else if (!strcmp( flag, "install" )) source->flags |= FLAG_INSTALL;
        else if (!strcmp( flag, "testdll" )) source->flags |= FLAG_TESTDLL;

        if (strendswith( source->name, ".idl" ))
        {
            if (!strcmp( flag, "header" )) source->flags |= FLAG_IDL_HEADER;
            else if (!strcmp( flag, "proxy" )) source->flags |= FLAG_IDL_PROXY;
            else if (!strcmp( flag, "client" )) source->flags |= FLAG_IDL_CLIENT;
            else if (!strcmp( flag, "server" )) source->flags |= FLAG_IDL_SERVER;
            else if (!strcmp( flag, "ident" )) source->flags |= FLAG_IDL_IDENT;
            else if (!strcmp( flag, "typelib" )) source->flags |= FLAG_IDL_TYPELIB;
            else if (!strcmp( flag, "register" )) source->flags |= FLAG_IDL_REGISTER;
            else if (!strcmp( flag, "regtypelib" )) source->flags |= FLAG_IDL_REGTYPELIB;
            else if (!strcmp( flag, "winmd" )) source->flags |= FLAG_IDL_WINMD;
        }
        else if (strendswith( source->name, ".rc" ))
        {
            if (!strcmp( flag, "header" )) source->flags |= FLAG_RC_HEADER;
            else if (!strcmp( flag, "po" )) source->flags |= FLAG_RC_PO;
        }
        else if (strendswith( source->name, ".sfd" ))
        {
            if (!strcmp( flag, "font" ))
            {
                struct strarray *array = source->args;

                if (!array)
                {
                    source->args = array = xmalloc( sizeof(*array) );
                    *array = empty_strarray;
                    source->flags |= FLAG_SFD_FONTS;
                }
                strarray_add( array, xstrdup( strtok( NULL, "" )));
                return;
            }
        }
        else
        {
            if (!strcmp( flag, "implib" )) source->flags |= FLAG_C_IMPLIB;
            if (!strcmp( flag, "unix" )) source->flags |= FLAG_C_UNIX;
            if (!strcmp( flag, "arm64ec_x64" )) source->flags |= FLAG_ARM64EC_X64;
        }
    }
}


/*******************************************************************
 *         parse_cpp_directive
 */
static void parse_cpp_directive( struct file *source, char *str )
{
    str = skip_spaces( str );
    if (*str++ != '#') return;
    str = skip_spaces( str );

    if (!strncmp( str, "include", 7 ))
        parse_include_directive( source, str + 7 );
    else if (!strncmp( str, "import", 6 ) && strendswith( source->name, ".m" ))
        parse_include_directive( source, str + 6 );
    else if (!strncmp( str, "pragma", 6 ))
        parse_pragma_directive( source, str + 6 );
}


/*******************************************************************
 *         parse_idl_file
 */
static void parse_idl_file( struct file *source, FILE *file )
{
    char *buffer, *include;

    input_line = 0;

    while ((buffer = get_line( file )))
    {
        char quote;
        char *p = skip_spaces( buffer );

        if (!strncmp( p, "importlib", 9 ))
        {
            p = skip_spaces( p + 9 );
            if (*p++ != '(') continue;
            p = skip_spaces( p );
            if (*p++ != '"') continue;
            include = p;
            while (*p && (*p != '"')) p++;
            if (!*p) fatal_error( "malformed importlib directive\n" );
            *p = 0;
            add_dependency( source, include, INCL_IMPORTLIB );
            continue;
        }

        if (!strncmp( p, "import", 6 ))
        {
            p = skip_spaces( p + 6 );
            if (*p != '"') continue;
            include = ++p;
            while (*p && (*p != '"')) p++;
            if (!*p) fatal_error( "malformed import directive\n" );
            *p = 0;
            add_dependency( source, include, INCL_IMPORT );
            continue;
        }

        /* check for #include inside cpp_quote */
        if (!strncmp( p, "cpp_quote", 9 ))
        {
            p = skip_spaces( p + 9 );
            if (*p++ != '(') continue;
            p = skip_spaces( p );
            if (*p++ != '"') continue;
            if (*p++ != '#') continue;
            p = skip_spaces( p );
            if (strncmp( p, "include", 7 )) continue;
            p = skip_spaces( p + 7 );
            if (*p == '\\' && p[1] == '"')
            {
                p += 2;
                quote = '"';
            }
            else
            {
                if (*p++ != '<' ) continue;
                quote = '>';
            }
            include = p;
            while (*p && (*p != quote)) p++;
            if (!*p || (quote == '"' && p[-1] != '\\'))
                fatal_error( "malformed #include directive inside cpp_quote\n" );
            if (quote == '"') p--;  /* remove backslash */
            *p = 0;
            add_dependency( source, include, (quote == '>') ? INCL_CPP_QUOTE_SYSTEM : INCL_CPP_QUOTE );
            continue;
        }

        parse_cpp_directive( source, p );
    }
}

/*******************************************************************
 *         parse_c_file
 */
static void parse_c_file( struct file *source, FILE *file )
{
    char *buffer;

    input_line = 0;
    while ((buffer = get_line( file )))
    {
        parse_cpp_directive( source, buffer );
    }
}


/*******************************************************************
 *         parse_rc_file
 */
static void parse_rc_file( struct file *source, FILE *file )
{
    char *buffer, *include;

    input_line = 0;
    while ((buffer = get_line( file )))
    {
        char quote;
        char *p = skip_spaces( buffer );

        if (p[0] == '/' && p[1] == '*')  /* check for magic makedep comment */
        {
            p = skip_spaces( p + 2 );
            if (strncmp( p, "@makedep:", 9 )) continue;
            p = skip_spaces( p + 9 );
            quote = '"';
            if (*p == quote)
            {
                include = ++p;
                while (*p && *p != quote) p++;
            }
            else
            {
                include = p;
                while (*p && *p != ' ' && *p != '\t' && *p != '*') p++;
            }
            if (!*p)
                fatal_error( "malformed makedep comment\n" );
            *p = 0;
            add_dependency( source, include, (quote == '>') ? INCL_SYSTEM : INCL_NORMAL );
            continue;
        }

        parse_cpp_directive( source, buffer );
    }
}


/*******************************************************************
 *         parse_in_file
 */
static void parse_in_file( struct file *source, FILE *file )
{
    char *p, *buffer;

    /* make sure it gets rebuilt when the version changes */
    add_dependency( source, "config.h", INCL_SYSTEM );

    if (!strendswith( source->name, ".man.in" )) return;  /* not a man page */

    input_line = 0;
    while ((buffer = get_line( file )))
    {
        if (strncmp( buffer, ".TH", 3 )) continue;
        p = skip_spaces( buffer + 3 );
        if (!(p = strtok( p, " \t" ))) continue;  /* program name */
        if (!(p = strtok( NULL, " \t" ))) continue;  /* man section */
        source->args = xstrdup( p );
        return;
    }
}


/*******************************************************************
 *         parse_sfd_file
 */
static void parse_sfd_file( struct file *source, FILE *file )
{
    char *p, *eol, *buffer;

    input_line = 0;
    while ((buffer = get_line( file )))
    {
        if (strncmp( buffer, "UComments:", 10 )) continue;
        p = buffer + 10;
        while (*p == ' ') p++;
        if (p[0] == '"' && p[1] && buffer[strlen(buffer) - 1] == '"')
        {
            p++;
            buffer[strlen(buffer) - 1] = 0;
        }
        while ((eol = strstr( p, "+AAoA" )))
        {
            *eol = 0;
            p = skip_spaces( p );
            if (*p++ == '#')
            {
                p = skip_spaces( p );
                if (!strncmp( p, "pragma", 6 )) parse_pragma_directive( source, p + 6 );
            }
            p = eol + 5;
        }
        p = skip_spaces( p );
        if (*p++ != '#') return;
        p = skip_spaces( p );
        if (!strncmp( p, "pragma", 6 )) parse_pragma_directive( source, p + 6 );
        return;
    }
}


static const struct
{
    const char *ext;
    void (*parse)( struct file *file, FILE *f );
} parse_functions[] =
{
    { ".c",   parse_c_file },
    { ".h",   parse_c_file },
    { ".inl", parse_c_file },
    { ".l",   parse_c_file },
    { ".m",   parse_c_file },
    { ".rh",  parse_c_file },
    { ".x",   parse_c_file },
    { ".y",   parse_c_file },
    { ".idl", parse_idl_file },
    { ".rc",  parse_rc_file },
    { ".in",  parse_in_file },
    { ".sfd", parse_sfd_file }
};

/*******************************************************************
 *         load_file
 */
static struct file *load_file( const char *name )
{
    struct file *file;
    FILE *f;
    unsigned int i, hash = hash_filename( name );

    LIST_FOR_EACH_ENTRY( file, &files[hash], struct file, entry )
        if (!strcmp( name, file->name )) return file;

    if (!(f = fopen( name, "r" ))) return NULL;

    file = add_file( name );
    list_add_tail( &files[hash], &file->entry );
    input_file_name = file->name;
    input_line = 0;

    for (i = 0; i < ARRAY_SIZE(parse_functions); i++)
    {
        if (!strendswith( name, parse_functions[i].ext )) continue;
        parse_functions[i].parse( file, f );
        break;
    }

    fclose( f );
    input_file_name = NULL;

    return file;
}


/*******************************************************************
 *         open_include_path_file
 *
 * Open a file from a directory on the include path.
 */
static struct file *open_include_path_file( const struct makefile *make, const char *dir,
                                            const char *name, char **filename )
{
    char *src_path = concat_paths( dir, name );
    struct file *ret = load_file( src_path );

    if (ret) *filename = src_path;
    return ret;
}


/*******************************************************************
 *         open_file_same_dir
 *
 * Open a file in the same directory as the parent.
 */
static struct file *open_file_same_dir( const struct incl_file *parent, const char *name, char **filename )
{
    char *src_path = replace_filename( parent->file->name, name );
    struct file *ret = load_file( src_path );

    if (ret) *filename = replace_filename( parent->filename, name );
    return ret;
}


/*******************************************************************
 *         open_same_dir_generated_file
 *
 * Open a generated_file in the same directory as the parent.
 */
static struct file *open_same_dir_generated_file( const struct makefile *make,
                                                  const struct incl_file *parent, struct incl_file *file,
                                                  const char *ext, const char *src_ext )
{
    char *filename;
    struct file *ret = NULL;

    if (strendswith( file->name, ext ) &&
        (ret = open_file_same_dir( parent, replace_extension( file->name, ext, src_ext ), &filename )))
    {
        file->sourcename = filename;
        file->filename = obj_dir_path( make, replace_filename( parent->name, file->name ));
    }
    return ret;
}


/*******************************************************************
 *         open_local_file
 *
 * Open a file in the source directory of the makefile.
 */
static struct file *open_local_file( const struct makefile *make, const char *path, char **filename )
{
    char *src_path = src_dir_path( make, path );
    struct file *ret = load_file( src_path );

    /* if not found, try parent dir */
    if (!ret && make->parent_dir)
    {
        free( src_path );
        path = strmake( "%s/%s", make->parent_dir, path );
        src_path = src_dir_path( make, path );
        ret = load_file( src_path );
    }

    if (ret) *filename = src_path;
    return ret;
}


/*******************************************************************
 *         open_local_generated_file
 *
 * Open a generated file in the directory of the makefile.
 */
static struct file *open_local_generated_file( const struct makefile *make, struct incl_file *file,
                                               const char *ext, const char *src_ext )
{
    struct incl_file *include;

    if (strendswith( file->name, ext ) &&
        (include = find_src_file( make, replace_extension( file->name, ext, src_ext ) )))
    {
        file->sourcename = include->filename;
        file->filename = obj_dir_path( make, file->name );
        return include->file;
    }
    return NULL;
}


/*******************************************************************
 *         open_global_file
 *
 * Open a file in the top-level source directory.
 */
static struct file *open_global_file( const char *path, char **filename )
{
    char *src_path = root_src_dir_path( path );
    struct file *ret = load_file( src_path );

    if (ret) *filename = src_path;
    return ret;
}


/*******************************************************************
 *         open_global_header
 *
 * Open a file in the global include source directory.
 */
static struct file *open_global_header( const char *path, char **filename )
{
    struct incl_file *include = find_src_file( include_makefile, path );

    if (!include) return NULL;
    *filename = include->filename;
    return include->file;
}


/*******************************************************************
 *         open_global_generated_file
 *
 * Open a generated file in the top-level source directory.
 */
static struct file *open_global_generated_file( const struct makefile *make, struct incl_file *file,
                                                const char *ext, const char *src_ext )
{
    struct incl_file *include;

    if (strendswith( file->name, ext ) &&
        (include = find_src_file( include_makefile, replace_extension( file->name, ext, src_ext ) )))
    {
        file->sourcename = include->filename;
        file->filename = strmake( "include/%s", file->name );
        return include->file;
    }
    return NULL;
}


/*******************************************************************
 *         open_src_file
 */
static struct file *open_src_file( const struct makefile *make, struct incl_file *pFile )
{
    struct file *file = open_local_file( make, pFile->name, &pFile->filename );

    if (!file) fatal_perror( "open %s", pFile->name );
    return file;
}


/*******************************************************************
 *         find_importlib_module
 */
static struct makefile *find_importlib_module( const char *name )
{
    unsigned int i, len;

    for (i = 0; i < subdirs.count; i++)
    {
        if (strncmp( submakes[i]->obj_dir, "dlls/", 5 )) continue;
        len = strlen(submakes[i]->obj_dir);
        if (strncmp( submakes[i]->obj_dir + 5, name, len - 5 )) continue;
        if (!name[len - 5] || !strcmp( name + len - 5, ".dll" )) return submakes[i];
    }
    return NULL;
}


/*******************************************************************
 *         open_include_file
 */
static struct file *open_include_file( const struct makefile *make, struct incl_file *pFile )
{
    struct file *file = NULL;
    unsigned int i, len;

    errno = ENOENT;

    /* check for generated files */
    if ((file = open_local_generated_file( make, pFile, ".tab.h", ".y" ))) return file;
    if ((file = open_local_generated_file( make, pFile, ".h", ".idl" ))) return file;
    if (fontforge && (file = open_local_generated_file( make, pFile, ".ttf", ".sfd" ))) return file;
    if (convert && rsvg && icotool)
    {
        if ((file = open_local_generated_file( make, pFile, ".bmp", ".svg" ))) return file;
        if ((file = open_local_generated_file( make, pFile, ".cur", ".svg" ))) return file;
        if ((file = open_local_generated_file( make, pFile, ".ico", ".svg" ))) return file;
    }
    if ((file = open_local_generated_file( make, pFile, "-client-protocol.h", ".xml" ))) return file;

    /* check for extra targets */
    if (strarray_exists( make->extra_targets, pFile->name ))
    {
        pFile->sourcename = src_dir_path( make, pFile->name );
        pFile->filename = obj_dir_path( make, pFile->name );
        return NULL;
    }

    /* now try in source dir */
    if ((file = open_local_file( make, pFile->name, &pFile->filename ))) return file;

    /* check for global importlib (module dependency) */
    if (pFile->type == INCL_IMPORTLIB && find_importlib_module( pFile->name ))
    {
        pFile->filename = pFile->name;
        return NULL;
    }

    /* check for generated files in global includes */
    if ((file = open_global_generated_file( make, pFile, ".h", ".idl" ))) return file;
    if ((file = open_global_generated_file( make, pFile, ".h", ".h.in" ))) return file;
    if (strendswith( pFile->name, "tmpl.h" ) &&
        (file = open_global_generated_file( make, pFile, ".h", ".x" ))) return file;

    /* check in global includes source dir */
    if ((file = open_global_header( pFile->name, &pFile->filename ))) return file;

    /* check in global msvcrt includes */
    if (pFile->use_msvcrt &&
        (file = open_global_header( strmake( "msvcrt/%s", pFile->name ), &pFile->filename )))
        return file;

    /* now search in include paths */
    for (i = 0; i < make->include_paths.count; i++)
    {
        const char *dir = make->include_paths.str[i];

        if (root_src_dir)
        {
            len = strlen( root_src_dir );
            if (!strncmp( dir, root_src_dir, len ) && (!dir[len] || dir[len] == '/'))
            {
                while (dir[len] == '/') len++;
                file = open_global_file( concat_paths( dir + len, pFile->name ), &pFile->filename );
            }
        }
        else
        {
            if (*dir == '/') continue;
            file = open_include_path_file( make, dir, pFile->name, &pFile->filename );
        }
        if (!file) continue;
        pFile->is_external = 1;
        return file;
    }

    if (pFile->type == INCL_SYSTEM) return NULL;  /* ignore system files we cannot find */

    /* try in src file directory */
    if ((file = open_same_dir_generated_file( make, pFile->included_by, pFile, ".tab.h", ".y" )) ||
        (file = open_same_dir_generated_file( make, pFile->included_by, pFile, ".h", ".idl" )) ||
        (file = open_file_same_dir( pFile->included_by, pFile->name, &pFile->filename )))
    {
        pFile->is_external = pFile->included_by->is_external;
        return file;
    }

    if (make->extlib) return NULL; /* ignore missing files in external libs */

    fprintf( stderr, "%s:%d: error: ", pFile->included_by->file->name, pFile->included_line );
    perror( pFile->name );
    pFile = pFile->included_by;
    while (pFile && pFile->included_by)
    {
        const char *parent = pFile->included_by->sourcename;
        if (!parent) parent = pFile->included_by->file->name;
        fprintf( stderr, "%s:%d: note: %s was first included here\n",
                 parent, pFile->included_line, pFile->name );
        pFile = pFile->included_by;
    }
    exit(1);
}


/*******************************************************************
 *         add_all_includes
 */
static void add_all_includes( struct makefile *make, struct incl_file *parent, struct file *file )
{
    unsigned int i;

    for (i = 0; i < file->deps_count; i++)
    {
        switch (file->deps[i].type)
        {
        case INCL_NORMAL:
        case INCL_IMPORT:
            add_include( make, parent, file->deps[i].name, file->deps[i].line, INCL_NORMAL );
            break;
        case INCL_IMPORTLIB:
            add_include( make, parent, file->deps[i].name, file->deps[i].line, INCL_IMPORTLIB );
            break;
        case INCL_SYSTEM:
            add_include( make, parent, file->deps[i].name, file->deps[i].line, INCL_SYSTEM );
            break;
        case INCL_CPP_QUOTE:
        case INCL_CPP_QUOTE_SYSTEM:
            break;
        }
    }
}


/*******************************************************************
 *         parse_file
 */
static void parse_file( struct makefile *make, struct incl_file *source, int src )
{
    struct file *file = src ? open_src_file( make, source ) : open_include_file( make, source );

    if (!file) return;

    source->file = file;
    source->files_count = 0;
    source->files_size = file->deps_count;
    source->files = xmalloc( source->files_size * sizeof(*source->files) );

    if (strendswith( file->name, ".m" )) file->flags |= FLAG_C_UNIX;
    if (file->flags & FLAG_C_UNIX) source->use_msvcrt = 0;
    else if (file->flags & FLAG_C_IMPLIB) source->use_msvcrt = 1;

    if (source->sourcename)
    {
        if (strendswith( source->sourcename, ".idl" ))
        {
            unsigned int i;

            /* generated .h file always includes these */
            add_include( make, source, "rpc.h", 0, INCL_NORMAL );
            add_include( make, source, "rpcndr.h", 0, INCL_NORMAL );
            for (i = 0; i < file->deps_count; i++)
            {
                switch (file->deps[i].type)
                {
                case INCL_IMPORT:
                    if (strendswith( file->deps[i].name, ".idl" ))
                        add_include( make, source, replace_extension( file->deps[i].name, ".idl", ".h" ),
                                     file->deps[i].line, INCL_NORMAL );
                    else
                        add_include( make, source, file->deps[i].name, file->deps[i].line, INCL_NORMAL );
                    break;
                case INCL_CPP_QUOTE:
                    add_include( make, source, file->deps[i].name, file->deps[i].line, INCL_NORMAL );
                    break;
                case INCL_CPP_QUOTE_SYSTEM:
                    add_include( make, source, file->deps[i].name, file->deps[i].line, INCL_SYSTEM );
                    break;
                case INCL_NORMAL:
                case INCL_SYSTEM:
                case INCL_IMPORTLIB:
                    break;
                }
            }
            return;
        }
        if (strendswith( source->sourcename, ".y" ))
            return;  /* generated .tab.h doesn't include anything */
    }

    add_all_includes( make, source, file );
}


/*******************************************************************
 *         add_src_file
 *
 * Add a source file to the list.
 */
static struct incl_file *add_src_file( struct makefile *make, const char *name )
{
    struct incl_file *file = xmalloc( sizeof(*file) );

    memset( file, 0, sizeof(*file) );
    file->name = xstrdup(name);
    file->use_msvcrt = is_using_msvcrt( make );
    file->is_external = !!make->extlib;
    list_add_tail( &make->sources, &file->entry );
    if (make == include_makefile)
    {
        unsigned int hash = hash_filename( name );
        list_add_tail( &global_includes[hash], &file->hash_entry );
    }
    parse_file( make, file, 1 );
    return file;
}


/*******************************************************************
 *         open_input_makefile
 */
static FILE *open_input_makefile( const struct makefile *make )
{
    FILE *ret;

    if (make->obj_dir)
        input_file_name = root_src_dir_path( obj_dir_path( make, "Makefile.in" ));
    else if (input_makefile_name)
        input_file_name = input_makefile_name;
    else
        input_file_name = output_makefile_name;  /* always use output name for main Makefile */

    input_line = 0;
    if (!(ret = fopen( input_file_name, "r" ))) fatal_perror( "open" );
    return ret;
}


/*******************************************************************
 *         get_make_variable
 */
static const char *get_make_variable( const struct makefile *make, const char *name )
{
    const char *ret;

    if ((ret = strarray_get_value( cmdline_vars, name ))) return ret;
    if ((ret = strarray_get_value( make->vars, name ))) return ret;
    if (top_makefile && (ret = strarray_get_value( top_makefile->vars, name ))) return ret;
    return NULL;
}


/*******************************************************************
 *         get_expanded_make_variable
 */
static char *get_expanded_make_variable( const struct makefile *make, const char *name )
{
    const char *var;
    char *p, *end, *expand, *tmp;

    var = get_make_variable( make, name );
    if (!var) return NULL;

    p = expand = xstrdup( var );
    while ((p = strchr( p, '$' )))
    {
        if (p[1] == '(')
        {
            if (!(end = strchr( p + 2, ')' ))) fatal_error( "syntax error in '%s'\n", expand );
            *end++ = 0;
            if (strchr( p + 2, ':' )) fatal_error( "pattern replacement not supported for '%s'\n", p + 2 );
            var = get_make_variable( make, p + 2 );
            tmp = replace_substr( expand, p, end - p, var ? var : "" );
            /* switch to the new string */
            p = tmp + (p - expand);
            free( expand );
            expand = tmp;
        }
        else if (p[1] == '{')  /* don't expand ${} variables */
        {
            if (!(end = strchr( p + 2, '}' ))) fatal_error( "syntax error in '%s'\n", expand );
            p = end + 1;
        }
        else if (p[1] == '$')
        {
            p += 2;
        }
        else fatal_error( "syntax error in '%s'\n", expand );
    }

    /* consider empty variables undefined */
    p = skip_spaces( expand );
    if (*p) return expand;
    free( expand );
    return NULL;
}


/*******************************************************************
 *         get_expanded_make_var_array
 */
static struct strarray get_expanded_make_var_array( const struct makefile *make, const char *name )
{
    struct strarray ret = empty_strarray;
    char *value, *token;

    if ((value = get_expanded_make_variable( make, name )))
        for (token = strtok( value, " \t" ); token; token = strtok( NULL, " \t" ))
            strarray_add( &ret, token );
    return ret;
}


/*******************************************************************
 *         get_expanded_file_local_var
 */
static struct strarray get_expanded_file_local_var( const struct makefile *make, const char *file,
                                                    const char *name )
{
    char *p, *var = strmake( "%s_%s", file, name );

    for (p = var; *p; p++) if (!isalnum( *p )) *p = '_';
    return get_expanded_make_var_array( make, var );
}


/*******************************************************************
 *         get_expanded_arch_var
 */
static char *get_expanded_arch_var( const struct makefile *make, const char *name, int arch )
{
    return get_expanded_make_variable( make, arch ? strmake( "%s_%s", archs.str[arch], name ) : name );
}


/*******************************************************************
 *         get_expanded_arch_var_array
 */
static struct strarray get_expanded_arch_var_array( const struct makefile *make, const char *name, int arch )
{
    return get_expanded_make_var_array( make, arch ? strmake( "%s_%s", archs.str[arch], name ) : name );
}


/*******************************************************************
 *         set_make_variable
 */
static int set_make_variable( struct strarray *array, const char *assignment )
{
    char *p, *name;

    p = name = xstrdup( assignment );
    while (isalnum(*p) || *p == '_') p++;
    if (name == p) return 0;  /* not a variable */
    if (isspace(*p))
    {
        *p++ = 0;
        p = skip_spaces( p );
    }
    if (*p != '=') return 0;  /* not an assignment */
    *p++ = 0;
    p = skip_spaces( p );

    strarray_set_value( array, name, p );
    return 1;
}


/*******************************************************************
 *         parse_makefile
 */
static struct makefile *parse_makefile( const char *path )
{
    char *buffer;
    FILE *file;
    struct makefile *make = xmalloc( sizeof(*make) );

    memset( make, 0, sizeof(*make) );
    make->obj_dir = path;
    if (root_src_dir) make->src_dir = root_src_dir_path( make->obj_dir );
    if (path && !strcmp( path, "include" )) include_makefile = make;

    file = open_input_makefile( make );
    while ((buffer = get_line( file )))
    {
        if (!strncmp( buffer, separator, strlen(separator) )) break;
        if (*buffer == '\t') continue;  /* command */
        buffer = skip_spaces( buffer );
        if (*buffer == '#') continue;  /* comment */
        set_make_variable( &make->vars, buffer );
    }
    fclose( file );
    input_file_name = NULL;
    return make;
}


/*******************************************************************
 *         add_generated_sources
 */
static void add_generated_sources( struct makefile *make )
{
    unsigned int i, arch;
    struct incl_file *source, *next, *file, *dlldata = NULL;
    struct strarray objs = get_expanded_make_var_array( make, "EXTRA_OBJS" );

    LIST_FOR_EACH_ENTRY_SAFE( source, next, &make->sources, struct incl_file, entry )
    {
        for (arch = 0; arch < archs.count; arch++)
        {
            if (!is_multiarch( arch )) continue;
            if (source->file->flags & FLAG_IDL_CLIENT)
            {
                file = add_generated_source( make, replace_extension( source->name, ".idl", "_c.c" ), NULL, arch );
                add_dependency( file->file, replace_extension( source->name, ".idl", ".h" ), INCL_NORMAL );
                add_all_includes( make, file, file->file );
            }
            if (source->file->flags & FLAG_IDL_SERVER)
            {
                file = add_generated_source( make, replace_extension( source->name, ".idl", "_s.c" ), NULL, arch );
                add_dependency( file->file, "wine/exception.h", INCL_NORMAL );
                add_dependency( file->file, replace_extension( source->name, ".idl", ".h" ), INCL_NORMAL );
                add_all_includes( make, file, file->file );
            }
            if (source->file->flags & FLAG_IDL_IDENT)
            {
                file = add_generated_source( make, replace_extension( source->name, ".idl", "_i.c" ), NULL, arch );
                add_dependency( file->file, "rpc.h", INCL_NORMAL );
                add_dependency( file->file, "rpcndr.h", INCL_NORMAL );
                add_dependency( file->file, "guiddef.h", INCL_NORMAL );
                add_all_includes( make, file, file->file );
            }
            if (source->file->flags & FLAG_IDL_PROXY)
            {
                file = add_generated_source( make, replace_extension( source->name, ".idl", "_p.c" ), NULL, arch );
                add_dependency( file->file, "objbase.h", INCL_NORMAL );
                add_dependency( file->file, "rpcproxy.h", INCL_NORMAL );
                add_dependency( file->file, "wine/exception.h", INCL_NORMAL );
                add_dependency( file->file, replace_extension( source->name, ".idl", ".h" ), INCL_NORMAL );
                add_all_includes( make, file, file->file );
            }
            if (source->file->flags & FLAG_IDL_TYPELIB)
            {
                add_generated_source( make, replace_extension( source->name, ".idl", "_l.res" ), NULL, arch );
            }
            if (source->file->flags & FLAG_IDL_REGTYPELIB)
            {
                add_generated_source( make, replace_extension( source->name, ".idl", "_t.res" ), NULL, arch );
            }
            if (source->file->flags & FLAG_IDL_REGISTER)
            {
                add_generated_source( make, replace_extension( source->name, ".idl", "_r.res" ), NULL, arch );
            }
        }

        /* now the arch-independent files */

        if ((source->file->flags & FLAG_IDL_PROXY) && !dlldata)
        {
            dlldata = add_generated_source( make, "dlldata.o", "dlldata.c", 0 );
            add_dependency( dlldata->file, "objbase.h", INCL_NORMAL );
            add_dependency( dlldata->file, "rpcproxy.h", INCL_NORMAL );
            add_all_includes( make, dlldata, dlldata->file );
        }
        if (source->file->flags & FLAG_IDL_HEADER)
        {
            add_generated_source( make, replace_extension( source->name, ".idl", ".h" ), NULL, 0 );
        }
        if (source->file->flags & FLAG_IDL_WINMD)
        {
            add_generated_source( make, replace_extension( source->name, ".idl", ".winmd" ), NULL, 0 );
        }
        if (!source->file->flags && strendswith( source->name, ".idl" ))
        {
            if (!strncmp( source->name, "wine/", 5 )) continue;
            source->file->flags = FLAG_IDL_HEADER | FLAG_INSTALL;
            add_generated_source( make, replace_extension( source->name, ".idl", ".h" ), NULL, 0 );
        }
        if (strendswith( source->name, ".x" ))
        {
            add_generated_source( make, replace_extension( source->name, ".x", ".h" ), NULL, 0 );
        }
        if (strendswith( source->name, ".y" ))
        {
            file = add_generated_source( make, replace_extension( source->name, ".y", ".tab.c" ), NULL, 0 );
            /* steal the includes list from the source file */
            file->files_count = source->files_count;
            file->files_size = source->files_size;
            file->files = source->files;
            source->files_count = source->files_size = 0;
            source->files = NULL;
        }
        if (strendswith( source->name, ".l" ))
        {
            file = add_generated_source( make, replace_extension( source->name, ".l", ".yy.c" ), NULL, 0 );
            /* steal the includes list from the source file */
            file->files_count = source->files_count;
            file->files_size = source->files_size;
            file->files = source->files;
            source->files_count = source->files_size = 0;
            source->files = NULL;
        }
        if (strendswith( source->name, ".po" ))
        {
            if (!make->disabled[0])
                strarray_add_uniq( &linguas, replace_extension( source->name, ".po", "" ));
        }
        if (strendswith( source->name, ".spec" ))
        {
            char *obj = replace_extension( source->name, ".spec", "" );
            strarray_addall_uniq( &make->extra_imports,
                                  get_expanded_file_local_var( make, obj, "IMPORTS" ));
        }
        if (strendswith( source->name, ".xml" ))
        {
            char *code_name = replace_extension( source->name , ".xml", "-protocol.c" );
            char *header_name = replace_extension( source->name , ".xml", "-client-protocol.h" );

            file = add_generated_source( make, code_name, NULL, 0 );
            file->file->flags |= FLAG_C_UNIX;
            file->use_msvcrt = 0;
            file = add_generated_source( make, header_name, NULL, 0 );
            file->file->flags |= FLAG_C_UNIX;
            file->use_msvcrt = 0;

            free( code_name );
            free( header_name );
        }
    }
    if (make->testdll)
    {
        for (arch = 0; arch < archs.count; arch++)
        {
            if (!is_multiarch( arch )) continue;
            file = add_generated_source( make, "testlist.o", "testlist.c", arch );
            add_dependency( file->file, "wine/test.h", INCL_NORMAL );
            add_all_includes( make, file, file->file );
        }
    }
    for (i = 0; i < objs.count; i++)
    {
        /* default to .c for unknown extra object files */
        if (strendswith( objs.str[i], ".o" ))
        {
            file = add_generated_source( make, objs.str[i], replace_extension( objs.str[i], ".o", ".c" ), 0);
            file->file->flags |= FLAG_C_UNIX;
            file->use_msvcrt = 0;
        }
        else if (strendswith( objs.str[i], ".res" ))
            add_generated_source( make, replace_extension( objs.str[i], ".res", ".rc" ), NULL, 0 );
        else
            add_generated_source( make, objs.str[i], NULL, 0 );
    }
}


/*******************************************************************
 *         create_dir
 */
static void create_dir( const char *dir )
{
    char *p, *path;

    p = path = xstrdup( dir );
    while ((p = strchr( p, '/' )))
    {
        *p = 0;
        if (mkdir( path, 0755 ) == -1 && errno != EEXIST) fatal_perror( "mkdir %s", path );
        *p++ = '/';
        while (*p == '/') p++;
    }
    if (mkdir( path, 0755 ) == -1 && errno != EEXIST) fatal_perror( "mkdir %s", path );
    free( path );
}


/*******************************************************************
 *         create_file_directories
 *
 * Create the base directories of all the files.
 */
static void create_file_directories( const struct makefile *make, struct strarray files )
{
    struct strarray subdirs = empty_strarray;
    unsigned int i;
    char *dir;

    for (i = 0; i < files.count; i++)
    {
        if (!strchr( files.str[i], '/' )) continue;
        dir = obj_dir_path( make, files.str[i] );
        *strrchr( dir, '/' ) = 0;
        strarray_add_uniq( &subdirs, dir );
    }

    for (i = 0; i < subdirs.count; i++) create_dir( subdirs.str[i] );
}


/*******************************************************************
 *         output_filenames_obj_dir
 */
static void output_filenames_obj_dir( const struct makefile *make, struct strarray array )
{
    unsigned int i;

    for (i = 0; i < array.count; i++) output_filename( obj_dir_path( make, array.str[i] ));
}


/*******************************************************************
 *         get_dependencies
 */
static void get_dependencies( struct incl_file *file, struct incl_file *source )
{
    unsigned int i;

    if (!file->filename) return;

    if (file != source)
    {
        if (file->owner == source) return;  /* already processed */
        if (file->type == INCL_IMPORTLIB)
        {
            if (!(source->file->flags & (FLAG_IDL_TYPELIB | FLAG_IDL_REGTYPELIB)))
                return;  /* library is imported only when building a typelib */
            strarray_add( &source->importlibdeps, file->filename );
        }
        else strarray_add( &source->dependencies, file->filename );
        file->owner = source;

        /* sanity checks */
        if (!strcmp( file->filename, "include/config.h" ) &&
            file != source->files[0] && !source->is_external)
        {
            input_file_name = source->filename;
            input_line = 0;
            for (i = 0; i < source->file->deps_count; i++)
            {
                if (!strcmp( source->file->deps[i].name, file->name ))
                    input_line = source->file->deps[i].line;
            }
            fatal_error( "%s must be included before other headers\n", file->name );
        }
    }

    for (i = 0; i < file->files_count; i++) get_dependencies( file->files[i], source );
}


/*******************************************************************
 *         get_local_dependencies
 *
 * Get the local dependencies of a given target.
 */
static struct strarray get_local_dependencies( const struct makefile *make, const char *name,
                                               struct strarray targets )
{
    unsigned int i;
    struct strarray deps = get_expanded_file_local_var( make, name, "DEPS" );

    for (i = 0; i < deps.count; i++)
    {
        if (strarray_exists( targets, deps.str[i] ))
            deps.str[i] = obj_dir_path( make, deps.str[i] );
        else
            deps.str[i] = src_dir_path( make, deps.str[i] );
    }
    return deps;
}


/*******************************************************************
 *         get_static_lib
 *
 * Find the makefile that builds the named static library (which may be an import lib).
 */
static struct makefile *get_static_lib( const char *name, unsigned int arch )
{
    unsigned int i;

    for (i = 0; i < subdirs.count; i++)
    {
        if (submakes[i]->importlib && !strcmp( submakes[i]->importlib, name )) return submakes[i];
        if (!submakes[i]->staticlib) continue;
        if (submakes[i]->disabled[arch]) continue;
        if (strncmp( submakes[i]->staticlib, "lib", 3 )) continue;
        if (strncmp( submakes[i]->staticlib + 3, name, strlen(name) )) continue;
        if (strcmp( submakes[i]->staticlib + 3 + strlen(name), ".a" )) continue;
        return submakes[i];
    }
    return NULL;
}


/*******************************************************************
 *         get_native_unix_lib
 */
static const char *get_native_unix_lib( const struct makefile *make, const char *name )
{
    if (!make->unixlib) return NULL;
    if (strncmp( make->unixlib, name, strlen(name) )) return NULL;
    if (make->unixlib[strlen(name)] != '.') return NULL;
    return obj_dir_path( make, make->unixlib );
}


/*******************************************************************
 *         get_parent_makefile
 */
static struct makefile *get_parent_makefile( struct makefile *make )
{
    char *dir, *p;
    unsigned int i;

    if (!make->obj_dir) return NULL;
    dir = xstrdup( make->obj_dir );
    if (!(p = strrchr( dir, '/' ))) return NULL;
    *p = 0;
    for (i = 0; i < subdirs.count; i++)
        if (!strcmp( submakes[i]->obj_dir, dir )) return submakes[i];
    return NULL;
}


/*******************************************************************
 *         needs_delay_lib
 */
static int needs_delay_lib( const struct makefile *make, unsigned int arch )
{
    if (delay_load_flags[arch]) return 0;
    if (!make->importlib) return 0;
    return strarray_exists( delay_import_libs, make->importlib );
}


/*******************************************************************
 *         add_unix_libraries
 */
static struct strarray add_unix_libraries( const struct makefile *make, struct strarray *deps )
{
    struct strarray ret = empty_strarray;
    struct strarray all_libs = empty_strarray;
    unsigned int i, j;

    if (strcmp( make->unixlib, "ntdll.so" )) strarray_add( &all_libs, "-lntdll" );
    strarray_addall( &all_libs, get_expanded_make_var_array( make, "UNIX_LIBS" ));

    for (i = 0; i < all_libs.count; i++)
    {
        const char *lib = NULL;

        if (!strncmp( all_libs.str[i], "-l", 2 ))
        {
            for (j = 0; j < subdirs.count; j++)
            {
                if (make == submakes[j]) continue;
                if ((lib = get_native_unix_lib( submakes[j], all_libs.str[i] + 2 ))) break;
            }
        }
        if (lib)
        {
            strarray_add( deps, lib );
            strarray_add( &ret, lib );
        }
        else strarray_add( &ret, all_libs.str[i] );
    }

    strarray_addall( &ret, libs );
    return ret;
}


/*******************************************************************
 *         is_crt_module
 */
static int is_crt_module( const char *file )
{
    return !strncmp( file, "msvcr", 5 ) || !strncmp( file, "ucrt", 4 ) || !strcmp( file, "crtdll.dll" );
}


/*******************************************************************
 *         get_default_crt
 */
static const char *get_default_crt( const struct makefile *make )
{
    if (make->module && is_crt_module( make->module )) return NULL;  /* don't add crt import to crt dlls */
    return !make->testdll && (!make->staticlib || make->extlib) ? "ucrtbase" : "msvcrt";
}


/*******************************************************************
 *         get_crt_define
 */
static const char *get_crt_define( const struct makefile *make )
{
    const char *crt_dll = NULL;
    unsigned int i, version = 0;

    for (i = 0; i < make->imports.count; i++)
    {
        if (!is_crt_module( make->imports.str[i] )) continue;
        if (crt_dll) fatal_error( "More than one C runtime DLL imported: %s and %s\n",
                                  crt_dll, make->imports.str[i] );
        crt_dll = make->imports.str[i];
    }

    if (!crt_dll)
    {
        if (strarray_exists( make->extradllflags, "-nodefaultlibs" )) return "-D_MSVCR_VER=0";
        if (!(crt_dll = get_default_crt( make ))) crt_dll = make->module;
    }
    if (!strncmp( crt_dll, "ucrt", 4 )) return "-D_UCRT";
    sscanf( crt_dll, "msvcr%u", &version );
    return strmake( "-D_MSVCR_VER=%u", version );
}


/*******************************************************************
 *         get_default_imports
 */
static struct strarray get_default_imports( const struct makefile *make, struct strarray imports,
                                            int nodefaultlibs )
{
    struct strarray ret = empty_strarray;
    const char *crt_dll = get_default_crt( make );
    unsigned int i;

    if (nodefaultlibs)
    {
        for (i = 0; i < imports.count; i++)
            if (!strcmp( imports.str[i], "winecrt0" )) return ret;
        strarray_add( &ret, "winecrt0" );
        if (compiler_rt) strarray_add( &ret, compiler_rt );
        return ret;
    }

    for (i = 0; i < imports.count; i++)
        if (is_crt_module( imports.str[i] ))
            crt_dll = imports.str[i];

    strarray_add( &ret, "winecrt0" );
    if (compiler_rt) strarray_add( &ret, compiler_rt );
    if (crt_dll) strarray_add( &ret, crt_dll );

    if (make->is_win16 && (!make->importlib || strcmp( make->importlib, "kernel" )))
        strarray_add( &ret, "kernel" );

    strarray_add( &ret, "kernel32" );
    strarray_add( &ret, "ntdll" );
    return ret;
}

enum import_type
{
    IMPORT_TYPE_DIRECT,
    IMPORT_TYPE_DELAYED,
    IMPORT_TYPE_DEFAULT,
};

/*******************************************************************
 *         add_import_libs
 */
static struct strarray add_import_libs( const struct makefile *make, struct strarray *deps,
                                        struct strarray imports, enum import_type type, unsigned int arch )
{
    struct strarray ret = empty_strarray;
    unsigned int i, link_arch;

    if (!get_link_arch( make, arch, &link_arch )) return ret;

    for (i = 0; i < imports.count; i++)
    {
        const char *name = imports.str[i];
        struct makefile *submake;

        /* add crt import lib only when adding the default imports libs */
        if (is_crt_module( imports.str[i] ) && type != IMPORT_TYPE_DEFAULT) continue;

        if (name[0] == '-')
        {
            switch (name[1])
            {
            case 'L': strarray_add( &ret, name ); continue;
            case 'l': name += 2; break;
            default: continue;
            }
        }
        else name = get_base_name( name );

        if ((submake = get_static_lib( name, link_arch )))
        {
            const char *ext = (type == IMPORT_TYPE_DELAYED && !delay_load_flags[arch]) ? ".delay.a" : ".a";
            const char *lib = obj_dir_path( submake, strmake( "%slib%s%s", arch_dirs[arch], name, ext ));
            strarray_add_uniq( deps, lib );
            strarray_add( &ret, lib );
        }
        else strarray_add( &ret, strmake( "-l%s", name ));
    }
    return ret;
}


/*******************************************************************
 *         add_install_rule
 */
static void add_install_rule( struct makefile *make, const char *target, unsigned int arch,
                              const char *file, const char *dest )
{
    unsigned int i;

    if (make->disabled[arch]) return;

    for (i = 0; i < NB_INSTALL_RULES; i++)
    {
        if (strarray_exists( make->install[i], target ) ||
            strarray_exists( top_install[i], make->obj_dir ) ||
            strarray_exists( top_install[i], obj_dir_path( make, target )))
        {
            strarray_add( &make->install_rules[i], file );
            strarray_add( &make->install_rules[i], dest );
            break;
        }
    }
}


/*******************************************************************
 *         install_data_file
 */
static void install_data_file( struct makefile *make, const char *target,
                               const char *obj, const char *dir, const char *dst )
{
    if (!dst) dst = get_basename( obj );
    add_install_rule( make, target, 0, obj, strmake( "d%s/%s", dir, dst ));
}


/*******************************************************************
 *         install_data_file_src
 */
static void install_data_file_src( struct makefile *make, const char *target,
                                   const char *src, const char *dir )
{
    add_install_rule( make, target, 0, src, strmake( "D%s/%s", dir, get_basename(src) ));
}


/*******************************************************************
 *         install_header
 */
static void install_header( struct makefile *make, const char *target, const char *obj )
{
    const char *dir, *end;
    bool is_obj = !!obj;

    if (!obj) obj = target;
    if (!strncmp( obj, "wine/", 5 )) dir = "$(includedir)";
    else if (!strncmp( obj, "msvcrt/", 7 )) dir = "$(includedir)/wine";
    else dir = "$(includedir)/wine/windows";
    if ((end = strrchr( obj, '/' ))) dir = strmake( "%s/%.*s", dir, (int)(end - obj), obj );

    if (is_obj)
        install_data_file( make, target, obj, dir, NULL );
    else
        install_data_file_src( make, target, obj, dir );
}


/*******************************************************************
 *         get_source_defines
 */
static struct strarray get_source_defines( struct makefile *make, struct incl_file *source,
                                           const char *obj )
{
    unsigned int i;
    struct strarray ret = empty_strarray;

    strarray_addall( &ret, make->include_args );
    if (source->use_msvcrt)
    {
        strarray_add( &ret, strmake( "-I%s", root_src_dir_path( "include/msvcrt" )));
        for (i = 0; i < make->include_paths.count; i++)
            strarray_add( &ret, strmake( "-I%s", make->include_paths.str[i] ));
        strarray_add( &ret, get_crt_define( make ));
    }
    strarray_addall( &ret, make->define_args );
    strarray_addall( &ret, get_expanded_file_local_var( make, obj, "EXTRADEFS" ));
    return ret;
}


/*******************************************************************
 *         remove_warning_flags
 */
static struct strarray remove_warning_flags( struct strarray flags )
{
    unsigned int i;
    struct strarray ret = empty_strarray;

    for (i = 0; i < flags.count; i++)
        if (strncmp( flags.str[i], "-W", 2 ) || !strncmp( flags.str[i], "-Wno-", 5 ))
            strarray_add( &ret, flags.str[i] );
    return ret;
}


/*******************************************************************
 *         get_debug_files
 */
static void output_debug_files( struct makefile *make, const char *name, unsigned int arch )
{
    unsigned int i;

    for (i = 0; i < debug_flags[arch].count; i++)
    {
        const char *debug_file = NULL;
        const char *flag = debug_flags[arch].str[i];
        if (!strcmp( flag, "pdb" )) debug_file = strmake( "%s.pdb", get_base_name( name ));
        else if (!strncmp( flag, "split", 5 )) debug_file = strmake( "%s.debug", name );
        if (!debug_file) continue;
        strarray_add( &make->debug_files, debug_file );
        output_filename( strmake( "-Wl,--debug-file,%s", obj_dir_path( make, debug_file )));
    }
}


/*******************************************************************
 *         cmd_prefix
 */
static const char *cmd_prefix( const char *cmd )
{
    if (!silent_rules) return "";
    return strmake( "$(quiet_%s)", cmd );
}


/*******************************************************************
 *         output_winegcc_command
 */
static void output_winegcc_command( struct makefile *make, unsigned int arch )
{
    output( "\t%s%s -o $@", cmd_prefix( "CCLD" ), winegcc );
    output_filename( "--wine-objdir ." );
    if (tools_dir)
    {
        output_filename( "--winebuild" );
        output_filename( winebuild );
    }
    output_filenames( target_flags[arch] );
    if (native_archs[arch] && !make->disabled[native_archs[arch]])
        output_filenames( hybrid_target_flags[arch] );
    if (arch) return;
    output_filename( "-mno-cygwin" );
    output_filenames( lddll_flags );
}


/*******************************************************************
 *         output_symlink_rule
 *
 * Output a rule to create a symlink.
 */
static void output_symlink_rule( const char *src_name, const char *link_name, int create_dir )
{
    const char *name = strrchr( link_name, '/' );
    char *dir = NULL;

    if (name)
    {
        dir = xstrdup( link_name );
        dir[name - link_name] = 0;
    }

    output( "\t%s", create_dir ? "" : cmd_prefix( "LN" ));
    if (create_dir && dir && *dir) output( "%s -d %s && ", install_sh, dir );
    output( "rm -f %s && ", link_name );

    /* dest path with a directory needs special handling if ln -s isn't supported */
    if (dir && strcmp( ln_s, "ln -s" ))
        output( "cd %s && %s %s %s\n", *dir ? dir : "/", ln_s, src_name, name + 1 );
    else
        output( "%s %s %s\n", ln_s, src_name, link_name );

    free( dir );
}


/*******************************************************************
 *         output_srcdir_symlink
 *
 * Output rule to create a symlink back to the source directory, for source files
 * that are needed at run-time.
 */
static void output_srcdir_symlink( struct makefile *make, const char *obj )
{
    char *src_file, *dst_file, *src_name;

    if (!make->src_dir) return;
    src_file = src_dir_path( make, obj );
    dst_file = obj_dir_path( make, obj );
    output( "%s: %s\n", dst_file, src_file );

    src_name = src_file;
    if (src_name[0] != '/' && make->obj_dir)
        src_name = concat_paths( get_root_relative_path( make ), src_name );

    output_symlink_rule( src_name, dst_file, 0 );
    strarray_add( &make->all_targets[0], obj );
}


/*******************************************************************
 *         output_install_commands
 */
static void output_install_commands( struct makefile *make, struct strarray files )
{
    unsigned int i, arch;

    for (i = 0; i < files.count; i += 2)
    {
        const char *file = files.str[i];
        const char *dest = strmake( "$(DESTDIR)%s", files.str[i + 1] + 1 );
        char type = *files.str[i + 1];

        switch (type)
        {
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9': /* arch-dependent program */
            arch = type - '0';
            output( "\tSTRIPPROG=%s %s -m 644 $(INSTALL_PROGRAM_FLAGS) %s %s\n",
                    strip_progs[arch], install_sh, obj_dir_path( make, file ), dest );
            output( "\t%s --builtin %s\n", winebuild, dest );
            break;
        case 'd':  /* data file */
            output( "\t%s -m 644 $(INSTALL_DATA_FLAGS) %s %s\n",
                    install_sh, obj_dir_path( make, file ), dest );
            break;
        case 'D':  /* data file in source dir */
            output( "\t%s -m 644 $(INSTALL_DATA_FLAGS) %s %s\n",
                    install_sh, src_dir_path( make, file ), dest );
            break;
        case '0':  /* native arch program file */
        case 'p':  /* program file */
            output( "\tSTRIPPROG=\"$(STRIP)\" %s $(INSTALL_PROGRAM_FLAGS) %s %s\n",
                    install_sh, obj_dir_path( make, file ), dest );
            break;
        case 's':  /* script */
            output( "\t%s $(INSTALL_SCRIPT_FLAGS) %s %s\n",
                    install_sh, obj_dir_path( make, file ), dest );
            break;
        case 'S':  /* script in source dir */
            output( "\t%s $(INSTALL_SCRIPT_FLAGS) %s %s\n",
                    install_sh, src_dir_path( make, file ), dest );
            break;
        case 't':  /* tools file */
            output( "\tSTRIPPROG=\"$(STRIP)\" %s $(INSTALL_PROGRAM_FLAGS) %s %s\n",
                    install_sh, tools_path( file ), dest );
            break;
        case 'y':  /* symlink */
            output_symlink_rule( file, dest, 1 );
            break;
        default:
            assert(0);
        }
        strarray_add( &make->uninstall_files, dest );
    }
}


/*******************************************************************
 *         output_install_rules
 *
 * Rules are stored as a (file,dest) pair of values.
 * The first char of dest indicates the type of install.
 */
static void output_install_rules( struct makefile *make, enum install_rules rules )
{
    unsigned int i;
    struct strarray files = make->install_rules[rules];
    struct strarray targets = empty_strarray;

    if (!files.count) return;

    for (i = 0; i < files.count; i += 2)
    {
        const char *file = files.str[i];
        switch (*files.str[i + 1])
        {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9': /* arch-dependent program */
        case 'd':  /* data file */
        case 'p':  /* program file */
        case 's':  /* script */
            strarray_add_uniq( &targets, obj_dir_path( make, file ));
            break;
        case 't':  /* tools file */
            strarray_add_uniq( &targets, tools_path( file ));
            break;
        }
    }

    output( "%s %s::", obj_dir_path( make, "install" ), obj_dir_path( make, install_targets[rules] ));
    output_filenames( targets );
    output( "\n" );
    output_install_commands( make, files );
    strarray_add_uniq( &make->phony_targets, obj_dir_path( make, "install" ));
    strarray_add_uniq( &make->phony_targets, obj_dir_path( make, install_targets[rules] ));
}


static int cmp_string_length( const char **a, const char **b )
{
    int paths_a = 0, paths_b = 0;
    const char *p;

    for (p = *a; *p; p++) if (*p == '/') paths_a++;
    for (p = *b; *p; p++) if (*p == '/') paths_b++;
    if (paths_b != paths_a) return paths_b - paths_a;
    return strcmp( *a, *b );
}

/*******************************************************************
 *         get_removable_dirs
 *
 * Retrieve a list of directories to try to remove after deleting the files.
 */
static struct strarray get_removable_dirs( struct strarray files )
{
    struct strarray dirs = empty_strarray;
    unsigned int i;

    for (i = 0; i < files.count; i++)
    {
        char *dir = xstrdup( files.str[i] );
        while (strchr( dir, '/' ))
        {
            *strrchr( dir, '/' ) = 0;
            strarray_add_uniq( &dirs, xstrdup(dir) );
        }
    }
    strarray_qsort( &dirs, cmp_string_length );
    return dirs;
}


/*******************************************************************
 *         output_uninstall_rules
 */
static void output_uninstall_rules( struct makefile *make )
{
    static const char *dirs_order[] =
        { "$(includedir)", "$(mandir)", "$(datadir)" };

    struct strarray uninstall_dirs;
    unsigned int i, j;

    if (!make->uninstall_files.count) return;
    output( "uninstall::\n" );
    output_rm_filenames( make->uninstall_files, "rm -f" );
    strarray_add_uniq( &make->phony_targets, "uninstall" );

    if (!subdirs.count) return;
    uninstall_dirs = get_removable_dirs( make->uninstall_files );
    output( "\t-rmdir" );
    for (i = 0; i < ARRAY_SIZE(dirs_order); i++)
    {
        for (j = 0; j < uninstall_dirs.count; j++)
        {
            if (!uninstall_dirs.str[j]) continue;
            if (strncmp( uninstall_dirs.str[j] + strlen("$(DESTDIR)"), dirs_order[i], strlen(dirs_order[i]) ))
                continue;
            output_filename( uninstall_dirs.str[j] );
            uninstall_dirs.str[j] = NULL;
        }
    }
    for (j = 0; j < uninstall_dirs.count; j++)
        if (uninstall_dirs.str[j]) output_filename( uninstall_dirs.str[j] );
    output( "\n" );
}


/*******************************************************************
 *         output_po_files
 */
static void output_po_files( struct makefile *make )
{
    const char *po_dir = src_dir_path( make, "po" );
    unsigned int i;

    if (linguas.count)
    {
        for (i = 0; i < linguas.count; i++)
            output_filename( strmake( "%s/%s.po", po_dir, linguas.str[i] ));
        output( ": %s/wine.pot\n", po_dir );
        output( "\t%smsgmerge --previous -q $@ %s/wine.pot | msgattrib --no-obsolete -o $@.new && mv $@.new $@\n",
                cmd_prefix( "MSG" ), po_dir );
        output( "po/all:" );
        for (i = 0; i < linguas.count; i++)
            output_filename( strmake( "%s/%s.po", po_dir, linguas.str[i] ));
        output( "\n" );
    }
    output( "%s/wine.pot:", po_dir );
    output_filenames( make->pot_files );
    output( "\n" );
    output( "\t%smsgcat -o $@", cmd_prefix( "MSG" ));
    output_filenames( make->pot_files );
    output( "\n" );
    strarray_add( &make->maintainerclean_files, strmake( "%s/wine.pot", po_dir ));
}


/*******************************************************************
 *         output_source_y
 */
static void output_source_y( struct makefile *make, struct incl_file *source, const char *obj )
{
    char *header = strmake( "%s.tab.h", obj );

    if (find_include_file( make, header ))
    {
        output( "%s: %s\n", obj_dir_path( make, header ), source->filename );
        output( "\t%s%s -o %s.tab.$$$$.c --defines=$@ %s && rm -f %s.tab.$$$$.c\n",
                cmd_prefix( "BISON" ), bison, obj_dir_path( make, obj ),
                source->filename, obj_dir_path( make, obj ));
        strarray_add( &make->clean_files, header );
    }
    output( "%s.tab.c: %s\n", obj_dir_path( make, obj ), source->filename );
    output( "\t%s%s -o $@ %s\n", cmd_prefix( "BISON" ), bison, source->filename );
}


/*******************************************************************
 *         output_source_l
 */
static void output_source_l( struct makefile *make, struct incl_file *source, const char *obj )
{
    output( "%s.yy.c: %s\n", obj_dir_path( make, obj ), source->filename );
    output( "\t%s%s -o$@ %s\n", cmd_prefix( "FLEX" ), flex, source->filename );
}


/*******************************************************************
 *         output_source_h
 */
static void output_source_h( struct makefile *make, struct incl_file *source, const char *obj )
{
    if (source->file->flags & FLAG_GENERATED)
        strarray_add( &make->all_targets[0], source->name );
    else if ((source->file->flags & FLAG_INSTALL) || strncmp( source->name, "wine/", 5 ))
        install_header( make, source->name, NULL );
}


/*******************************************************************
 *         output_source_rc
 */
static void output_source_rc( struct makefile *make, struct incl_file *source, const char *obj )
{
    struct strarray defines = get_source_defines( make, source, obj );
    const char *po_dir = NULL, *res_file = strmake( "%s.res", obj );
    unsigned int i, arch;

    if (source->file->flags & FLAG_RC_HEADER) return;
    if (source->file->flags & FLAG_GENERATED) strarray_add( &make->clean_files, source->name );
    if (linguas.count && (source->file->flags & FLAG_RC_PO)) po_dir = "po";
    if (!(source->file->flags & FLAG_TESTDLL))
    {
        for (arch = 0; arch < archs.count; arch++)
            if (!make->disabled[arch]) strarray_add( &make->res_files[arch], res_file );
    }
    else strarray_add( &make->clean_files, res_file );

    if (source->file->flags & FLAG_RC_PO)
    {
        strarray_add( &make->pot_files, strmake( "%s.pot", obj ));
        output( "%s.pot ", obj_dir_path( make, obj ) );
    }
    output( "%s: %s", obj_dir_path( make, res_file ), source->filename );
    output_filename( wrc );
    if (make->src_dir) output_filename( "nls/locale.nls" );
    output_filenames( source->dependencies );
    output( "\n" );
    output( "\t%s%s -u -o $@", cmd_prefix( "WRC" ), wrc );
    if (make->is_win16) output_filename( "-m16" );
    output_filename( "--nostdinc" );
    if (po_dir) output_filename( strmake( "--po-dir=%s", po_dir ));
    output_filenames( defines );
    output_filename( source->filename );
    output( "\n" );
    if (po_dir)
    {
        output( "%s:", obj_dir_path( make, res_file ));
        for (i = 0; i < linguas.count; i++)
            output_filename( strmake( "%s/%s.mo", po_dir, linguas.str[i] ));
        output( "\n" );
    }
}


/*******************************************************************
 *         output_source_mc
 */
static void output_source_mc( struct makefile *make, struct incl_file *source, const char *obj )
{
    unsigned int i, arch;
    char *obj_path = obj_dir_path( make, obj );
    char *res_file = strmake( "%s.res", obj );

    for (arch = 0; arch < archs.count; arch++)
        if (!make->disabled[arch]) strarray_add( &make->res_files[arch], res_file );
    strarray_add( &make->pot_files, strmake( "%s.pot", obj ));
    output( "%s.pot %s.res: %s", obj_path, obj_path, source->filename );
    output_filename( wmc );
    output_filenames( source->dependencies );
    if (make->src_dir) output_filename( "nls/locale.nls" );
    output( "\n" );
    output( "\t%s%s -u -o $@ %s", cmd_prefix( "WMC" ), wmc, source->filename );
    if (linguas.count)
    {
        output_filename( "--po-dir=po" );
        output( "\n" );
        output( "%s.res:", obj_dir_path( make, obj ));
        for (i = 0; i < linguas.count; i++)
            output_filename( strmake( "po/%s.mo", linguas.str[i] ));
    }
    output( "\n" );
}


/*******************************************************************
 *         output_source_res
 */
static void output_source_res( struct makefile *make, struct incl_file *source, const char *obj )
{
    if (make->disabled[source->arch]) return;
    strarray_add( &make->res_files[source->arch], source->name );
}


/*******************************************************************
 *         output_source_idl
 */
static void output_source_idl( struct makefile *make, struct incl_file *source, const char *obj )
{
    struct strarray defines = get_source_defines( make, source, obj );
    struct strarray headers = empty_strarray;
    struct strarray deps = empty_strarray;
    struct strarray multiarch_targets[MAX_ARCHS] = { empty_strarray };
    const char *dest;
    unsigned int i, arch;

    if (find_include_file( make, strmake( "%s.h", obj ))) source->file->flags |= FLAG_IDL_HEADER;
    if (!source->file->flags) return;

    if (source->file->flags & FLAG_IDL_PROXY) strarray_add( &make->dlldata_files, source->name );
    if (source->file->flags & FLAG_INSTALL)
    {
        install_header( make, source->name, NULL );
        if (source->file->flags & FLAG_IDL_HEADER)
            install_header( make, source->name, strmake( "%s.h", obj ));
    }
    if (source->file->flags & FLAG_IDL_HEADER)
    {
        dest = strmake( "%s.h", obj );
        strarray_add( &headers, dest );
        if (!find_src_file( make, dest )) strarray_add( &make->clean_files, dest );
    }
    if (source->file->flags & FLAG_IDL_WINMD)
    {
        dest = strmake( "%s.winmd", obj );
        strarray_add( &headers, dest );
        strarray_add( &make->clean_files, dest );
    }

    for (i = 0; i < ARRAY_SIZE(idl_outputs); i++)
    {
        if (!(source->file->flags & idl_outputs[i].flag)) continue;
        for (arch = 0; arch < archs.count; arch++)
        {
            if (!is_multiarch( arch )) continue;
            if (make->disabled[arch]) continue;
            dest = strmake( "%s%s%s", arch_dirs[arch], obj, idl_outputs[i].ext );
            if (!find_src_file( make, dest )) strarray_add( &make->clean_files, dest );
            strarray_add( &multiarch_targets[arch], dest );
        }
    }

    for (arch = 0; arch < archs.count; arch++)
    {
        struct strarray arch_deps = empty_strarray;

        if (!arch) strarray_addall( &arch_deps, headers );
        strarray_addall( &arch_deps, multiarch_targets[arch] );
        if (!arch_deps.count) continue;
        output_filenames_obj_dir( make, arch_deps );
        output( ":\n" );
        output( "\t%s%s -o $@", cmd_prefix( "WIDL" ), widl );
        output_filenames( target_flags[arch] );
        output_filename( "--nostdinc" );
        output_filename( "-Ldlls/\\*" );
        output_filenames( defines );
        output_filenames( get_expanded_make_var_array( make, "EXTRAIDLFLAGS" ));
        output_filenames( get_expanded_file_local_var( make, obj, "EXTRAIDLFLAGS" ));
        if (arch) output_filenames( get_expanded_arch_var_array( make, "EXTRAIDLFLAGS", arch ));
        output_filename( source->filename );
        output( "\n" );
        strarray_addall( &deps, arch_deps );
    }

    if (deps.count)
    {
        output_filenames_obj_dir( make, deps );
        output( ":" );
        output_filename( widl );
        output_filename( source->filename );
        output_filenames( source->dependencies );
        output( "\n" );
    }

    if (source->importlibdeps.count)
    {
        for (arch = 0; arch < archs.count; arch++)
        {
            if (!multiarch_targets[arch].count) continue;
            output_filenames_obj_dir( make, multiarch_targets[arch] );
            output( ":" );
            for (i = 0; i < source->importlibdeps.count; i++)
            {
                int native_arch = native_archs[arch] ? native_archs[arch] : arch;
                struct makefile *submake = find_importlib_module( source->importlibdeps.str[i] );
                const char *module = strmake( "%s/%s", arch_pe_dirs[native_arch], submake->module );
                output_filename( obj_dir_path( submake, module ));
            }
            output( "\n" );
        }
    }
}


/*******************************************************************
 *         output_source_x
 */
static void output_source_x( struct makefile *make, struct incl_file *source, const char *obj )
{
    output( "%s.h: %s %s\n", obj_dir_path( make, obj ), make_xftmpl, source->filename );
    output( "\t%s%s -H -o $@ %s\n", cmd_prefix( "GEN" ), make_xftmpl, source->filename );
    if (source->file->flags & FLAG_INSTALL)
    {
        install_header( make, source->name, NULL );
        install_header( make, source->name, strmake( "%s.h", obj ));
    }
}


/*******************************************************************
 *         output_source_sfd
 */
static void output_source_sfd( struct makefile *make, struct incl_file *source, const char *obj )
{
    unsigned int i;
    char *ttf_obj = strmake( "%s.ttf", obj );
    char *ttf_file = src_dir_path( make, ttf_obj );

    if (fontforge && !make->src_dir)
    {
        output( "%s: %s\n", ttf_file, source->filename );
        output( "\t%s%s -script %s %s $@\n", cmd_prefix( "GEN" ),
                fontforge, root_src_dir_path( "fonts/genttf.ff" ), source->filename );
        if (!(source->file->flags & FLAG_SFD_FONTS)) strarray_add( &make->font_files, ttf_obj );
        strarray_add( &make->maintainerclean_files, ttf_obj );
    }
    if (source->file->flags & FLAG_INSTALL)
    {
        install_data_file_src( make, source->name, ttf_obj, "$(datadir)/wine/fonts" );
        output_srcdir_symlink( make, ttf_obj );
    }

    if (source->file->flags & FLAG_SFD_FONTS)
    {
        struct strarray *array = source->file->args;

        for (i = 0; i < array->count; i++)
        {
            char *font = strtok( xstrdup(array->str[i]), " \t" );
            char *args = strtok( NULL, "" );

            strarray_add( &make->all_targets[0], xstrdup( font ));
            output( "%s: %s %s\n", obj_dir_path( make, font ), sfnt2fon, ttf_file );
            output( "\t%s%s -q -o $@ %s %s\n", cmd_prefix( "GEN" ), sfnt2fon, ttf_file, args );
            install_data_file( make, source->name, font, "$(datadir)/wine/fonts", NULL );
        }
    }
}


/*******************************************************************
 *         output_source_svg
 */
static void output_source_svg( struct makefile *make, struct incl_file *source, const char *obj )
{
    static const char * const images[] = { "bmp", "cur", "ico", NULL };
    unsigned int i;

    if (convert && rsvg && icotool)
    {
        for (i = 0; images[i]; i++)
            if (find_include_file( make, strmake( "%s.%s", obj, images[i] ))) break;

        if (images[i])
        {
            output( "%s.%s: %s\n", src_dir_path( make, obj ), images[i], source->filename );
            output( "\t%sCONVERT=\"%s\" ICOTOOL=\"%s\" RSVG=\"%s\" %s %s $@\n",
                    cmd_prefix( "GEN" ), convert, icotool, rsvg, buildimage, source->filename );
            strarray_add( &make->maintainerclean_files, strmake( "%s.%s", obj, images[i] ));
        }
    }
}


/*******************************************************************
 *         output_source_nls
 */
static void output_source_nls( struct makefile *make, struct incl_file *source, const char *obj )
{
    install_data_file_src( make, source->name, source->name, "$(datadir)/wine/nls" );
    output_srcdir_symlink( make, strmake( "%s.nls", obj ));
}


/*******************************************************************
 *         output_source_desktop
 */
static void output_source_desktop( struct makefile *make, struct incl_file *source, const char *obj )
{
    install_data_file_src( make, source->name, source->name, "$(datadir)/applications" );
}


/*******************************************************************
 *         output_source_po
 */
static void output_source_po( struct makefile *make, struct incl_file *source, const char *obj )
{
    output( "%s.mo: %s\n", obj_dir_path( make, obj ), source->filename );
    output( "\t%s%s -o $@ %s\n", cmd_prefix( "MSG" ), msgfmt, source->filename );
    strarray_add( &make->all_targets[0], strmake( "%s.mo", obj ));
}


/*******************************************************************
 *         output_source_in
 */
static void output_source_in( struct makefile *make, struct incl_file *source, const char *obj )
{
    unsigned int i;

    if (make == include_makefile) return;  /* ignore generated includes */
    if (strendswith( obj, ".man" ) && source->file->args)
    {
        struct strarray symlinks;
        char *dir, *dest = replace_extension( obj, ".man", "" );
        char *lang = strchr( dest, '.' );
        char *section = source->file->args;
        if (lang)
        {
            *lang++ = 0;
            dir = strmake( "$(mandir)/%s/man%s", lang, section );
        }
        else dir = strmake( "$(mandir)/man%s", section );
        install_data_file( make, dest, obj, dir, strmake( "%s.%s", dest, section ));
        symlinks = get_expanded_file_local_var( make, dest, "SYMLINKS" );
        for (i = 0; i < symlinks.count; i++)
            add_install_rule( make, symlinks.str[i], 0, strmake( "%s.%s", dest, section ),
                              strmake( "y%s/%s.%s", dir, symlinks.str[i], section ));
        free( dest );
        free( dir );
    }
    strarray_add( &make->in_files, obj );
    strarray_add( &make->all_targets[0], obj );
    output( "%s: %s\n", obj_dir_path( make, obj ), source->filename );
    output( "\t%s%s %s >$@ || (rm -f $@ && false)\n", cmd_prefix( "SED" ), sed_cmd, source->filename );
    output( "%s:", obj_dir_path( make, obj ));
    output_filenames( source->dependencies );
    output( "\n" );
    install_data_file( make, obj, obj, "$(datadir)/wine", NULL );
}


/*******************************************************************
 *         output_source_spec
 */
static void output_source_spec( struct makefile *make, struct incl_file *source, const char *obj )
{
    /* nothing to do */
}


/*******************************************************************
 *         output_source_testdll
 */
static void output_source_testdll( struct makefile *make, struct incl_file *source, const char *obj )
{
    struct strarray imports = get_expanded_file_local_var( make, obj, "IMPORTS" );
    struct strarray dll_flags = empty_strarray;
    struct strarray default_imports = empty_strarray;
    struct strarray all_libs, dep_libs;
    const char *dll_name, *obj_name, *res_name, *output_rsrc, *output_file, *ext = ".dll";
    struct incl_file *spec_file = find_src_file( make, strmake( "%s.spec", obj ));
    unsigned int arch, link_arch;

    if (!imports.count) imports = make->imports;
    strarray_addall( &dll_flags, make->extradllflags );
    strarray_addall( &dll_flags, get_expanded_file_local_var( make, obj, "EXTRADLLFLAGS" ));
    default_imports = get_default_imports( make, imports, strarray_exists( dll_flags, "-nodefaultlibs" ));
    if (strarray_exists( dll_flags, "-mconsole" )) ext = ".exe";

    for (arch = 0; arch < archs.count; arch++)
    {
        const char *hybrid_obj_name = NULL;

        if (!is_multiarch( arch ) || !get_link_arch( make, arch, &link_arch)) continue;

        all_libs = dep_libs = empty_strarray;
        strarray_addall( &all_libs, add_import_libs( make, &dep_libs, imports, IMPORT_TYPE_DIRECT, arch ) );
        strarray_addall( &all_libs, add_import_libs( make, &dep_libs, default_imports, IMPORT_TYPE_DEFAULT, arch ) );
        if (!arch) strarray_addall( &all_libs, libs );
        dll_name = arch_module_name( strmake( "%s%s", obj, ext ), arch );
        obj_name = obj_dir_path( make, strmake( "%s%s.o", arch_dirs[arch], obj ));
        if (link_arch != arch)
            hybrid_obj_name = obj_dir_path( make, strmake( "%s%s.o", arch_dirs[link_arch], obj ));
        output_file = obj_dir_path( make, dll_name );
        output_rsrc = strmake( "%s.res", dll_name );

        if (!find_src_file( make, strmake( "%s.rc", obj ) )) res_name = NULL;
        else res_name = obj_dir_path( make, strmake( "%s.res", obj ) );

        strarray_add( &make->clean_files, dll_name );
        strarray_add( &make->res_files[arch], output_rsrc );
        output( "%s:", obj_dir_path( make, output_rsrc ));
        output_filename( output_file );
        output_filename( wrc );
        output( "\n" );
        output( "\t%secho \"%s%s TESTDLL \\\"%s\\\"\" | %s -u -o $@\n",
                cmd_prefix( "WRC" ), obj, ext, output_file, wrc );

        output( "%s:", output_file );
        if (spec_file) output_filename( spec_file->filename );
        output_filename( obj_name );
        if (hybrid_obj_name) output_filename( hybrid_obj_name );
        if (res_name) output_filename( res_name );
        output_filenames( dep_libs );
        output_filename( winebuild );
        output_filename( winegcc );
        output( "\n" );
        output_winegcc_command( make, link_arch );
        output_filename( "-s" );
        output_filenames( dll_flags );
        if (link_arch) output_filenames( get_expanded_arch_var_array( make, "EXTRADLLFLAGS", link_arch ));
        if (!strcmp( ext, ".dll" )) output_filename( "-shared" );
        if (spec_file) output_filename( spec_file->filename );
        output_filename( obj_name );
        if (hybrid_obj_name) output_filename( hybrid_obj_name );
        if (res_name) output_filename( res_name );
        output_debug_files( make, dll_name, link_arch );
        output_filenames( all_libs );
        output_filename( arch_make_variable( "LDFLAGS", link_arch ));
        output( "\n" );
    }
}


/*******************************************************************
 *         output_source_xml
 */
static void output_source_xml( struct makefile *make, struct incl_file *source, const char *obj )
{
    if (wayland_scanner)
    {
        output( "%s-protocol.c: %s\n", obj_dir_path( make, obj ), source->filename );
        output( "\t%s%s private-code %s $@\n", cmd_prefix( "GEN" ), wayland_scanner, source->filename );
        output( "%s-client-protocol.h: %s\n", obj_dir_path( make, obj ), source->filename );
        output( "\t%s%s client-header %s $@\n", cmd_prefix( "GEN" ), wayland_scanner, source->filename );
    }
}


/*******************************************************************
 *         output_source_winmd
 */
static void output_source_winmd( struct makefile *make, struct incl_file *source, const char *obj )
{
    if (source->file->flags & FLAG_GENERATED) strarray_add( &make->all_targets[0], source->name );
}


/*******************************************************************
 *         output_source_one_arch
 */
static void output_source_one_arch( struct makefile *make, struct incl_file *source, const char *obj,
                                    struct strarray defines, struct strarray *targets,
                                    unsigned int arch )
{
    const char *obj_name, *var_cc, *var_cflags;
    struct compile_command *cmd;
    struct strarray cflags = empty_strarray;

    if (make->disabled[arch] && !(source->file->flags & FLAG_C_IMPLIB)) return;

    if (arch)
    {
        if (source->file->flags & FLAG_C_UNIX) return;
        if (!is_multiarch( arch )) return;
        if (!is_using_msvcrt( make ) && !make->staticlib && !(source->file->flags & FLAG_C_IMPLIB)) return;
    }
    else if (source->file->flags & FLAG_C_UNIX)
    {
        if (!unix_lib_supported) return;
    }
    else if (archs.count > 1 && is_using_msvcrt( make ))
    {
        if (!so_dll_supported) return;
        if (!(source->file->flags & FLAG_C_IMPLIB) && (!make->staticlib || make->extlib)) return;
    }

    if (strendswith( source->name, ".S" ) && is_subdir_other_arch( source->name, arch )) return;

    obj_name = strmake( "%s%s.o", source->arch ? "" : arch_dirs[arch], obj );
    strarray_add( targets, obj_name );

    if (source->file->flags & FLAG_C_UNIX)
        strarray_add( &make->unixobj_files, obj_name );
    else if (source->file->flags & FLAG_C_IMPLIB)
        strarray_add( &make->implib_files[arch], obj_name );
    else if (!(source->file->flags & FLAG_TESTDLL))
        strarray_add( &make->object_files[arch], obj_name );
    else
        strarray_add( &make->clean_files, obj_name );

    if (!source->use_msvcrt) strarray_addall( &cflags, make->unix_cflags );
    if ((source->file->flags & FLAG_ARM64EC_X64) && !strcmp( archs.str[arch], "arm64ec" ))
    {
        var_cc     = "$(x86_64_CC)";
        var_cflags = "$(x86_64_CFLAGS)";
        strarray_add( &cflags, "-D__arm64ec_x64__" );
        strarray_addall( &cflags, get_expanded_make_var_array( top_makefile, "x86_64_EXTRACFLAGS" ));
    }
    else
    {
        var_cc     = arch_make_variable( "CC", arch );
        var_cflags = arch_make_variable( "CFLAGS", arch );
        strarray_addall( &cflags, make->extlib ? extra_cflags_extlib[arch] : extra_cflags[arch] );
    }

    if (!arch)
    {
        if (source->file->flags & FLAG_C_UNIX)
        {
            strarray_addall( &cflags, unix_dllflags );
        }
        else if (make->module || make->testdll)
        {
            strarray_addall( &cflags, dll_flags );
            if (source->use_msvcrt) strarray_addall( &cflags, msvcrt_flags );
            if (!unix_lib_supported &&
                ((make->module && is_crt_module( make->module )) ||
                 (make->testdll && is_crt_module( make->testdll ))))
                strarray_add( &cflags, "-fno-builtin" );
        }
        strarray_addall( &cflags, cpp_flags );
    }
    else
    {
        if ((make->module && is_crt_module( make->module )) ||
            (make->testdll && is_crt_module( make->testdll )))
            strarray_add( &cflags, "-fno-builtin" );
    }

    output( "%s: %s\n", obj_dir_path( make, obj_name ), source->filename );
    output( "\t%s%s -c -o $@ %s", cmd_prefix( "CC" ), var_cc, source->filename );
    output_filenames( defines );
    output_filenames( cflags );
    output_filename( var_cflags );
    output( "\n" );

    if (make->testdll && strendswith( source->name, ".c" ) &&
        !(source->file->flags & (FLAG_GENERATED | FLAG_TESTDLL)))
    {
        const char *ok_file, *test_exe;

        ok_file = strmake( "%s%s.ok", arch_dirs[arch], obj );
        test_exe = replace_extension( make->testdll, ".dll", "_test.exe" );
        strarray_add( &make->ok_files[arch], ok_file );
        output( "%s:\n", obj_dir_path( make, ok_file ));
        output( "\t%s%s $(RUNTESTFLAGS) -T . -M %s -p %s %s && touch $@\n",
                cmd_prefix( "TEST" ), runtest, make->testdll,
                obj_dir_path( make, arch_module_name( test_exe, arch )), obj );
    }

    if (source->file->flags & FLAG_GENERATED) return;

    /* static analysis rules */

    if (sarif_converter && make->module && !make->extlib)
    {
        const char *sast_name = strmake( "%s%s.sarif", source->arch ? "" : arch_dirs[arch], obj );
        output( "%s: %s\n", obj_dir_path( make, sast_name ), source->filename );
        output( "\t%s%s -o $@ %s", cmd_prefix( "SAST" ), var_cc, source->filename );
        output_filenames( defines );
        output_filenames( cflags );
        output_filename( "--analyze" );
        output_filename( "-Xclang" );
        output_filename( "-analyzer-output=sarif" );
        output_filename( "$(SASTFLAGS)" );
        output( "\n" );
        strarray_add( &make->sast_files, sast_name );
        strarray_add( targets, sast_name );
    }

    /* compile commands */

    cmd = xmalloc( sizeof(*cmd) );
    cmd->source = source->filename;
    cmd->obj = obj_dir_path( make, obj_name );
    cmd->args = empty_strarray;
    strarray_addall( &cmd->args, defines );
    strarray_addall( &cmd->args, cflags );

    if ((source->file->flags & FLAG_ARM64EC_X64) && !strcmp( archs.str[arch], "arm64ec" ))
    {
        char *cflags = get_expanded_make_variable( make, "x86_64_CFLAGS" );
        cmd->cmd = get_expanded_make_variable( make, "x86_64_CC" );
        if (cflags) strarray_add( &cmd->args, cflags );
    }
    else
    {
        char *cflags = get_expanded_arch_var( make, "CFLAGS", arch );
        cmd->cmd = get_expanded_arch_var( make, "CC", arch );
        if (cflags) strarray_add( &cmd->args, cflags );
    }
    list_add_tail( &compile_commands, &cmd->entry );
}


/*******************************************************************
 *         output_source_default
 */
static void output_source_default( struct makefile *make, struct incl_file *source, const char *obj )
{
    struct strarray defines = get_source_defines( make, source, obj );
    struct strarray targets = empty_strarray;
    unsigned int arch;

    for (arch = 0; arch < archs.count; arch++)
        if (!source->arch || source->arch == arch)
            output_source_one_arch( make, source, obj, defines, &targets, arch );

    if (source->file->flags & FLAG_GENERATED)
    {
        if (!make->testdll || !strendswith( source->filename, "testlist.c" ))
            strarray_add( &make->clean_files, source->basename );
    }
    else if (source->file->flags & FLAG_TESTDLL)
    {
        output_source_testdll( make, source, obj );
    }
    else
    {
        if (make->testdll && strendswith( source->name, ".c" ))
            strarray_add( &make->test_files, obj );
    }

    if (targets.count && source->dependencies.count)
    {
        output_filenames_obj_dir( make, targets );
        output( ":" );
        output_filenames( source->dependencies );
        output( "\n" );
    }
}


/* dispatch table to output rules for a single source file */
static const struct
{
    const char *ext;
    void (*fn)( struct makefile *make, struct incl_file *source, const char *obj );
} output_source_funcs[] =
{
    { "y", output_source_y },
    { "l", output_source_l },
    { "h", output_source_h },
    { "rh", output_source_h },
    { "inl", output_source_h },
    { "rc", output_source_rc },
    { "mc", output_source_mc },
    { "res", output_source_res },
    { "idl", output_source_idl },
    { "sfd", output_source_sfd },
    { "svg", output_source_svg },
    { "nls", output_source_nls },
    { "desktop", output_source_desktop },
    { "po", output_source_po },
    { "in", output_source_in },
    { "x", output_source_x },
    { "spec", output_source_spec },
    { "xml", output_source_xml },
    { "winmd", output_source_winmd },
    { NULL, output_source_default }
};


/*******************************************************************
 *         output_fake_module
 */
static void output_fake_module( struct makefile *make, const char *spec_file )
{
    unsigned int arch = 0;  /* fake modules are always native */
    const char *name = strmake( "%s/%s", arch_pe_dirs[arch], make->module );

    if (make->disabled[arch]) return;

    strarray_add( &make->all_targets[arch], name );
    install_data_file( make, make->module, name, strmake( "$(libdir)/wine/%s", arch_pe_dirs[arch] ), NULL );

    output( "%s:", obj_dir_path( make, name ));
    if (spec_file) output_filename( spec_file );
    output_filenames_obj_dir( make, make->res_files[arch] );
    output_filename( winebuild );
    output_filename( winegcc );
    output( "\n" );
    output_winegcc_command( make, arch );
    output_filename( "-Wb,--fake-module" );
    if (!make->is_exe) output_filename( "-shared" );
    if (spec_file) output_filename( spec_file );
    output_filenames( make->extradllflags );
    output_filenames_obj_dir( make, make->res_files[arch] );
    output( "\n" );
}


/*******************************************************************
 *         output_module
 */
static void output_module( struct makefile *make, unsigned int arch )
{
    struct strarray default_imports = empty_strarray;
    struct strarray all_libs = empty_strarray;
    struct strarray dep_libs = empty_strarray;
    struct strarray imports = make->imports;
    const char *module_name;
    char *spec_file = NULL;
    unsigned int i, link_arch;

    if (!make->is_exe)
    {
        if (make->data_only || strendswith( make->module, ".drv" ) ||
            strarray_exists( make->extradllflags, "-Wl,--subsystem,native" ))
        {
            /* spec file is optional */
            struct incl_file *spec = find_src_file( make, replace_extension( make->module, ".dll", ".spec" ));
            if (spec) spec_file = spec->filename;
        }
        else spec_file = src_dir_path( make, replace_extension( make->module, ".dll", ".spec" ));
    }

    if (!make->data_only)
    {
        if (!get_link_arch( make, arch, &link_arch )) return;

        module_name = arch_module_name( make->module, arch );
        default_imports = get_default_imports( make, imports,
                                               strarray_exists( make->extradllflags, "-nodefaultlibs" ));

        strarray_addall( &all_libs, add_import_libs( make, &dep_libs, imports, IMPORT_TYPE_DIRECT, arch ));
        strarray_addall( &all_libs, add_import_libs( make, &dep_libs, make->delayimports, IMPORT_TYPE_DELAYED, arch ));
        strarray_addall( &all_libs, add_import_libs( make, &dep_libs, default_imports, IMPORT_TYPE_DEFAULT, arch ) );
        if (!arch) strarray_addall( &all_libs, libs );

        if (delay_load_flags[arch])
        {
            for (i = 0; i < make->delayimports.count; i++)
            {
                struct makefile *import = get_static_lib( make->delayimports.str[i], arch );
                if (import) strarray_add( &all_libs, strmake( "%s%s", delay_load_flags[arch], import->module ));
            }
        }
    }
    else
    {
        if (native_archs[arch]) return;
        if (make->disabled[arch]) return;
        link_arch = arch;

        module_name = strmake( "%s/%s", arch_pe_dirs[arch], make->module );
    }

    strarray_add( &make->all_targets[link_arch], module_name );
    if (make->data_only)
        install_data_file( make, make->module, module_name,
                           strmake( "$(libdir)/wine/%s", arch_pe_dirs[arch] ), NULL );
    else
        add_install_rule( make, make->module, link_arch, module_name,
                          strmake( "%c%s/%s%s", '0' + arch, arch_install_dirs[arch], make->module,
                                   dll_ext[arch] ));

    output( "%s:", obj_dir_path( make, module_name ));
    if (spec_file) output_filename( spec_file );
    output_filenames_obj_dir( make, make->object_files[arch] );
    if (link_arch != arch) output_filenames_obj_dir( make, make->object_files[link_arch] );
    output_filenames_obj_dir( make, make->res_files[arch] );
    output_filenames( dep_libs );
    output_filename( winebuild );
    output_filename( winegcc );
    output( "\n" );
    output_winegcc_command( make, link_arch );
    if (arch) output_filename( "-Wl,--wine-builtin" );
    if (!make->is_exe) output_filename( "-shared" );
    if (spec_file) output_filename( spec_file );
    output_filenames( make->extradllflags );
    if (link_arch) output_filenames( get_expanded_arch_var_array( make, "EXTRADLLFLAGS", link_arch ));
    output_filenames_obj_dir( make, make->object_files[arch] );
    if (link_arch != arch) output_filenames_obj_dir( make, make->object_files[link_arch] );
    output_filenames_obj_dir( make, make->res_files[arch] );
    output_debug_files( make, module_name, link_arch );
    output_filenames( all_libs );
    output_filename( arch_make_variable( "LDFLAGS", link_arch ));
    output( "\n" );

    if (!make->data_only && !arch && unix_lib_supported) output_fake_module( make, spec_file );
}


/*******************************************************************
 *         output_import_lib
 */
static void output_import_lib( struct makefile *make, unsigned int arch )
{
    char *spec_file = src_dir_path( make, replace_extension( make->module, ".dll", ".spec" ));
    const char *name = strmake( "%slib%s.a", arch_dirs[arch], make->importlib );
    unsigned int hybrid_arch = hybrid_archs[arch];

    if (native_archs[arch]) return;

    strarray_add( &make->clean_files, name );
    if (needs_delay_lib( make, arch ))
    {
        const char *delay_name = replace_extension( name, ".a", ".delay.a" );
        strarray_add( &make->clean_files, delay_name );
        output( "%s ", obj_dir_path( make, delay_name ));
    }
    output( "%s: %s %s", obj_dir_path( make, name ), winebuild, spec_file );
    output_filenames_obj_dir( make, make->implib_files[arch] );
    if (hybrid_arch) output_filenames_obj_dir( make, make->implib_files[hybrid_arch] );
    output( "\n" );
    output( "\t%s%s -w --implib -o $@", cmd_prefix( "BUILD" ), winebuild );
    if (!delay_load_flags[arch]) output_filename( "--without-dlltool" );
    output_filenames( target_flags[hybrid_arch ? hybrid_arch : arch] );
    if (make->is_win16) output_filename( "-m16" );
    if (hybrid_arch) output_filenames( hybrid_target_flags[hybrid_arch] );
    output_filename( "--export" );
    output_filename( spec_file );
    output_filenames_obj_dir( make, make->implib_files[arch] );
    if (hybrid_arch) output_filenames_obj_dir( make, make->implib_files[hybrid_arch] );
    output( "\n" );

    if (make->disabled[arch]) return;
    install_data_file( make, make->importlib, name, arch_install_dirs[arch], NULL );
}


/*******************************************************************
 *         output_unix_lib
 */
static void output_unix_lib( struct makefile *make )
{
    struct strarray unix_deps = empty_strarray;
    struct strarray unix_libs = add_unix_libraries( make, &unix_deps );
    unsigned int arch = 0;  /* unix libs are always native */

    if (make->disabled[arch]) return;

    strarray_add( &make->all_targets[arch], make->unixlib );
    add_install_rule( make, make->module, arch, make->unixlib,
                      strmake( "p%s/%s", arch_install_dirs[arch], make->unixlib ));
    output( "%s:", obj_dir_path( make, make->unixlib ));
    output_filenames_obj_dir( make, make->unixobj_files );
    output_filenames( unix_deps );
    output( "\n" );
    output( "\t%s$(CC) -o $@", cmd_prefix( "CCLD" ));
    output_filenames( get_expanded_make_var_array( make, "UNIXLDFLAGS" ));
    output_filenames_obj_dir( make, make->unixobj_files );
    output_filenames( unix_libs );
    output_filename( "$(LDFLAGS)" );
    output( "\n" );
}


/*******************************************************************
 *         output_static_lib
 */
static void output_static_lib( struct makefile *make, unsigned int arch )
{
    const char *name = strmake( "%s%s", arch_dirs[arch], make->staticlib );
    unsigned int hybrid_arch = hybrid_archs[arch];

    if (make->disabled[arch]) return;
    if (native_archs[arch]) return;
    if (arch && !is_multiarch( arch )) return;

    strarray_add( &make->clean_files, name );
    output( "%s: %s", obj_dir_path( make, name ), winebuild );
    output_filenames_obj_dir( make, make->object_files[arch] );
    if (hybrid_arch) output_filenames_obj_dir( make, make->object_files[hybrid_arch] );
    if (!arch) output_filenames_obj_dir( make, make->unixobj_files );
    output( "\n" );
    output( "\t%s%s -w --staticlib -o $@", cmd_prefix( "BUILD" ), winebuild );
    output_filenames( target_flags[hybrid_arch ? hybrid_arch : arch] );
    output_filenames_obj_dir( make, make->object_files[arch] );
    if (hybrid_arch) output_filenames_obj_dir( make, make->object_files[hybrid_arch] );
    if (!arch) output_filenames_obj_dir( make, make->unixobj_files );
    output( "\n" );
    install_data_file( make, make->staticlib, name, arch_install_dirs[arch], NULL );
}


/*******************************************************************
 *         output_test_module
 */
static void output_test_module( struct makefile *make, unsigned int arch )
{
    char *basemodule = replace_extension( make->testdll, ".dll", "" );
    char *stripped = arch_module_name( strmake( "%s_test-stripped.exe", basemodule ), arch );
    char *testmodule = arch_module_name( strmake( "%s_test.exe", basemodule ), arch );
    struct strarray default_imports = get_default_imports( make, make->imports, 0 );
    struct strarray dep_libs = empty_strarray;
    struct strarray all_libs = empty_strarray;
    struct makefile *parent = get_parent_makefile( make );
    unsigned int link_arch;

    if (!get_link_arch( make, arch, &link_arch )) return;

    strarray_addall( &all_libs, add_import_libs( make, &dep_libs, make->imports, IMPORT_TYPE_DIRECT, arch ) );
    strarray_addall( &all_libs, add_import_libs( make, &dep_libs, default_imports, IMPORT_TYPE_DEFAULT, arch ) );

    strarray_add( &make->all_targets[arch], testmodule );
    strarray_add( &make->clean_files, stripped );
    output( "%s:\n", obj_dir_path( make, testmodule ));
    output_winegcc_command( make, link_arch );
    output_filenames( make->extradllflags );
    output_filenames_obj_dir( make, make->object_files[arch] );
    if (link_arch != arch) output_filenames_obj_dir( make, make->object_files[link_arch] );
    output_filenames_obj_dir( make, make->res_files[arch] );
    output_debug_files( make, testmodule, arch );
    output_filenames( all_libs );
    output_filename( arch_make_variable( "LDFLAGS", link_arch ));
    output( "\n" );
    output( "%s:\n", obj_dir_path( make, stripped ));
    output_winegcc_command( make, link_arch );
    output_filename( "-s" );
    output_filename( strmake( "-Wb,-F,%s_test.exe", basemodule ));
    output_filenames( make->extradllflags );
    output_filenames_obj_dir( make, make->object_files[arch] );
    if (link_arch != arch) output_filenames_obj_dir( make, make->object_files[link_arch] );
    output_filenames_obj_dir( make, make->res_files[arch] );
    output_filenames( all_libs );
    output_filename( arch_make_variable( "LDFLAGS", link_arch ));
    output( "\n" );
    output( "%s %s:", obj_dir_path( make, testmodule ), obj_dir_path( make, stripped ));
    output_filenames_obj_dir( make, make->object_files[arch] );
    if (link_arch != arch) output_filenames_obj_dir( make, make->object_files[link_arch] );
    output_filenames_obj_dir( make, make->res_files[arch] );
    output_filenames( dep_libs );
    output_filename( winebuild );
    output_filename( winegcc );
    output( "\n" );

    output( "programs/winetest/%s%s_test.res: %s\n", arch_dirs[arch], basemodule,
            obj_dir_path( make, stripped ));
    output( "\t%secho \"%s_test.exe TESTRES \\\"%s\\\"\" | %s -u -o $@\n", cmd_prefix( "WRC" ),
            basemodule, obj_dir_path( make, stripped ), wrc );

    if (make->disabled[arch] || (parent && parent->disabled[arch]))
    {
        make->ok_files[arch] = empty_strarray;
        return;
    }
    output_filenames_obj_dir( make, make->ok_files[arch] );
    output( ": %s", obj_dir_path( make, testmodule ));
    if (parent)
    {
        char *parent_module = arch_module_name( make->testdll, arch );
        output_filename( obj_dir_path( parent, parent_module ));
        if (parent->unixlib) output_filename( obj_dir_path( parent, parent->unixlib ));
    }
    output( "\n" );
    output( "%s %s:", obj_dir_path( make, "check" ), obj_dir_path( make, "test" ));
    output_filenames_obj_dir( make, make->ok_files[arch] );
    output( "\n" );
    strarray_add_uniq( &make->phony_targets, obj_dir_path( make, "check" ));
    strarray_add_uniq( &make->phony_targets, obj_dir_path( make, "test" ));
    output( "%s::\n", obj_dir_path( make, "testclean" ));
    output( "\trm -f" );
    output_filenames_obj_dir( make, make->ok_files[arch] );
    output( "\n" );
    strarray_addall( &make->clean_files, make->ok_files[arch] );
    strarray_add_uniq( &make->phony_targets, obj_dir_path( make, "testclean" ));
}


/*******************************************************************
 *         output_programs
 */
static void output_programs( struct makefile *make )
{
    unsigned int i, j;
    unsigned int arch = 0;  /* programs are always native */

    for (i = 0; i < make->programs.count; i++)
    {
        const char *install_dir;
        char *program = strmake( "%s%s", make->programs.str[i], exe_ext );
        struct strarray deps = get_local_dependencies( make, make->programs.str[i], make->in_files );
        struct strarray all_libs = get_expanded_file_local_var( make, make->programs.str[i], "LDFLAGS" );
        struct strarray objs     = get_expanded_file_local_var( make, make->programs.str[i], "OBJS" );
        struct strarray symlinks = get_expanded_file_local_var( make, make->programs.str[i], "SYMLINKS" );

        if (!objs.count) objs = make->object_files[arch];
        if (!strarray_exists( all_libs, "-nodefaultlibs" ))
        {
            strarray_addall( &all_libs, get_expanded_make_var_array( make, "UNIX_LIBS" ));
            strarray_addall( &all_libs, libs );
        }

        output( "%s:", obj_dir_path( make, program ) );
        output_filenames_obj_dir( make, objs );
        output_filenames( deps );
        output( "\n" );
        output( "\t%s$(CC) -o $@", cmd_prefix( "CCLD" ));
        output_filenames_obj_dir( make, objs );
        output_filenames( all_libs );
        output_filename( "$(LDFLAGS)" );
        output( "\n" );
        strarray_add( &make->all_targets[arch], program );

        for (j = 0; j < symlinks.count; j++)
        {
            output( "%s: %s\n", obj_dir_path( make, symlinks.str[j] ), obj_dir_path( make, program ));
            output_symlink_rule( program, obj_dir_path( make, symlinks.str[j] ), 0 );
        }
        strarray_addall( &make->all_targets[arch], symlinks );

        install_dir = !strcmp( make->obj_dir, "loader" ) ? arch_install_dirs[arch] : "$(bindir)";
        add_install_rule( make, make->programs.str[i], arch, program, strmake( "p%s/%s", install_dir, program ));
        for (j = 0; j < symlinks.count; j++)
            add_install_rule( make, symlinks.str[j], arch, program,
                              strmake( "y$(bindir)/%s%s", symlinks.str[j], exe_ext ));
    }
}


/*******************************************************************
 *         output_subdirs
 */
static void output_subdirs( struct makefile *make )
{
    struct strarray all_targets = empty_strarray;
    struct strarray makefile_deps = empty_strarray;
    struct strarray clean_files = empty_strarray;
    struct strarray sast_files = empty_strarray;
    struct strarray testclean_files = empty_strarray;
    struct strarray distclean_files = empty_strarray;
    struct strarray distclean_dirs = empty_strarray;
    struct strarray dependencies = empty_strarray;
    struct strarray install_deps[NB_INSTALL_RULES] = { empty_strarray };
    struct strarray tooldeps_deps = empty_strarray;
    struct strarray buildtest_deps = empty_strarray;
    unsigned int i, j, arch;

    strarray_addall( &clean_files, make->clean_files );
    strarray_addall( &distclean_files, make->distclean_files );
    for (arch = 0; arch < archs.count; arch++) strarray_addall( &all_targets, make->all_targets[arch] );
    for (i = 0; i < subdirs.count; i++)
    {
        struct strarray subclean = empty_strarray;
        strarray_addall( &subclean, get_removable_dirs( submakes[i]->clean_files ));
        strarray_addall( &subclean, get_removable_dirs( submakes[i]->distclean_files ));
        strarray_add( &makefile_deps, src_dir_path( submakes[i], "Makefile.in" ));
        strarray_addall_uniq( &make->phony_targets, submakes[i]->phony_targets );
        strarray_addall_uniq( &make->uninstall_files, submakes[i]->uninstall_files );
        strarray_addall_uniq( &dependencies, submakes[i]->dependencies );
        strarray_addall_path( &clean_files, submakes[i]->obj_dir, submakes[i]->clean_files );
        strarray_addall_path( &sast_files, submakes[i]->obj_dir, submakes[i]->sast_files );
        strarray_addall_path( &distclean_files, submakes[i]->obj_dir, submakes[i]->distclean_files );
        strarray_addall_path( &distclean_dirs, submakes[i]->obj_dir, subclean );
        strarray_addall_path( &make->maintainerclean_files, submakes[i]->obj_dir, submakes[i]->maintainerclean_files );
        strarray_addall_path( &make->pot_files, submakes[i]->obj_dir, submakes[i]->pot_files );

        for (arch = 0; arch < archs.count; arch++)
        {
            if (submakes[i]->disabled[arch]) continue;
            strarray_addall_path( &all_targets, submakes[i]->obj_dir, submakes[i]->all_targets[arch] );
            strarray_addall_path( &testclean_files, submakes[i]->obj_dir, submakes[i]->ok_files[arch] );
        }
        if (submakes[i]->disabled[0]) continue;

        strarray_addall_path( &all_targets, submakes[i]->obj_dir, submakes[i]->font_files );
        if (!strcmp( submakes[i]->obj_dir, "tools" ) || !strncmp( submakes[i]->obj_dir, "tools/", 6 ))
            strarray_add( &tooldeps_deps, obj_dir_path( submakes[i], "all" ));
        if (submakes[i]->testdll)
            strarray_add( &buildtest_deps, obj_dir_path( submakes[i], "all" ));
        for (j = 0; j < NB_INSTALL_RULES; j++)
            if (submakes[i]->install_rules[j].count)
                strarray_add( &install_deps[j], obj_dir_path( submakes[i], install_targets[j] ));
    }
    strarray_addall( &dependencies, makefile_deps );
    output( "all:" );
    output_filenames( all_targets );
    output( "\n" );
    output( "Makefile:" );
    output_filenames( makefile_deps );
    output( "\n" );
    output_filenames( dependencies );
    output( ":\n" );
    for (j = 0; j < NB_INSTALL_RULES; j++)
    {
        if (!install_deps[j].count) continue;
        if (strcmp( install_targets[j], "install-test" ))
        {
            output( "install " );
            strarray_add_uniq( &make->phony_targets, "install" );
        }
        output( "%s::", install_targets[j] );
        output_filenames( install_deps[j] );
        output( "\n" );
        strarray_add_uniq( &make->phony_targets, install_targets[j] );
    }
    output_uninstall_rules( make );
    if (buildtest_deps.count)
    {
        output( "buildtests:" );
        output_filenames( buildtest_deps );
        output( "\n" );
        strarray_add_uniq( &make->phony_targets, "buildtests" );
    }
    output( "check test:" );
    output_filenames( testclean_files );
    output( "\n" );
    strarray_add_uniq( &make->phony_targets, "check" );
    strarray_add_uniq( &make->phony_targets, "test" );

    if (sarif_converter)
    {
        if (strcmp( sarif_converter, "false" ))
        {
            output( "gl-code-quality-report.json:\n" );
            output( "\t%s%s -t codequality -r", cmd_prefix( "SAST" ), sarif_converter );
            output_filename( root_src_dir_path("") );
            output_filenames( sast_files );
            output_filename( "$@" );
            output( "\n" );
            strarray_add( &clean_files, "gl-code-quality-report.json" );
            output( "gl-code-quality-report.json " );
        }
        output( "sast:" );
        output_filenames( sast_files );
        output( "\n" );
        strarray_add_uniq( &make->phony_targets, "sast" );
    }

    if (get_expanded_make_variable( make, "GETTEXTPO_LIBS" )) output_po_files( make );

    output( "clean::\n");
    output_rm_filenames( clean_files, "rm -f" );
    output( "testclean::\n");
    output_rm_filenames( testclean_files, "rm -f" );
    output( "distclean:: clean\n");
    output_rm_filenames( distclean_files, "rm -f" );
    output_rm_filenames( distclean_dirs, "-rmdir 2>/dev/null" );
    output( "maintainer-clean::\n");
    output_rm_filenames( make->maintainerclean_files, "rm -f" );
    strarray_add_uniq( &make->phony_targets, "distclean" );
    strarray_add_uniq( &make->phony_targets, "testclean" );
    strarray_add_uniq( &make->phony_targets, "maintainer-clean" );

    if (tooldeps_deps.count)
    {
        output( "__tooldeps__:" );
        output_filenames( tooldeps_deps );
        output( "\n" );
        strarray_add_uniq( &make->phony_targets, "__tooldeps__" );
    }

    if (make->phony_targets.count)
    {
        output( ".PHONY:" );
        output_filenames( make->phony_targets );
        output( "\n" );
    }
}


/*******************************************************************
 *         output_sources
 */
static void output_sources( struct makefile *make )
{
    struct strarray all_targets = empty_strarray;
    struct incl_file *source;
    unsigned int i, j, arch;

    strarray_add_uniq( &make->phony_targets, "all" );

    LIST_FOR_EACH_ENTRY( source, &make->sources, struct incl_file, entry )
    {
        char *obj = xstrdup( source->name );
        char *ext = get_extension( obj );

        if (!ext) fatal_error( "unsupported file type %s\n", source->name );
        *ext++ = 0;

        for (j = 0; output_source_funcs[j].ext; j++)
            if (!strcmp( ext, output_source_funcs[j].ext )) break;

        output_source_funcs[j].fn( make, source, obj );
        strarray_addall_uniq( &make->dependencies, source->dependencies );
    }

    /* special case for winetest: add resource files from other test dirs */
    if (make->obj_dir && !strcmp( make->obj_dir, "programs/winetest" ))
    {
        for (arch = 0; arch < archs.count; arch++)
        {
            if (!is_multiarch( arch )) continue;
            for (i = 0; i < subdirs.count; i++)
            {
                if (!submakes[i]->testdll) continue;
                if (submakes[i]->disabled[arch]) continue;
                if (enable_tests.count && !strarray_exists( enable_tests, submakes[i]->testdll )) continue;
                strarray_add( &make->res_files[arch],
                              strmake( "%s%s", arch_dirs[arch],
                                       replace_extension( submakes[i]->testdll, ".dll", "_test.res" )));
            }
        }
    }

    if (make->dlldata_files.count)
    {
        output( "%s: %s %s\n", obj_dir_path( make, "dlldata.c" ),
                widl, src_dir_path( make, "Makefile.in" ));
        output( "\t%s%s --dlldata-only -o $@", cmd_prefix( "WIDL" ), widl );
        output_filenames( make->dlldata_files );
        output( "\n" );
    }

    if (make->staticlib)
    {
        for (arch = 0; arch < archs.count; arch++)
            if (is_multiarch( arch ) || (so_dll_supported && !make->extlib))
                output_static_lib( make, arch );
    }
    else if (make->module)
    {
        for (arch = 0; arch < archs.count; arch++)
        {
            if (is_multiarch( arch )) output_module( make, arch );
            if (make->importlib && (is_multiarch( arch ) || (!arch && !is_native_arch_disabled( make ))))
                output_import_lib( make, arch );
        }
        if (make->unixlib) output_unix_lib( make );
        if (make->is_exe && !make->is_win16 && unix_lib_supported && strendswith( make->module, ".exe" ))
        {
            char *binary = replace_extension( make->module, ".exe", "" );
            add_install_rule( make, binary, 0, "wine",
                              strmake( "%c$(bindir)/%s", strcmp( ln_s, "ln -s" ) ? 't' : 'y', binary ));
        }
    }
    else if (make->testdll)
    {
        for (arch = 0; arch < archs.count; arch++)
            if (is_multiarch( arch )) output_test_module( make, arch );
    }
    else if (make->programs.count) output_programs( make );

    for (i = 0; i < make->scripts.count; i++)
        add_install_rule( make, make->scripts.str[i], 0, make->scripts.str[i],
                          strmake( "S$(bindir)/%s", make->scripts.str[i] ));

    for (i = 0; i < make->extra_targets.count; i++)
        if (strarray_exists( make->dependencies, obj_dir_path( make, make->extra_targets.str[i] )))
            strarray_add( &make->clean_files, make->extra_targets.str[i] );
        else
            strarray_add( &make->all_targets[0], make->extra_targets.str[i] );

    if (!make->src_dir) strarray_add( &make->distclean_files, ".gitignore" );
    strarray_add( &make->distclean_files, "Makefile" );
    if (make->testdll) strarray_add( &make->distclean_files, "testlist.c" );

    for (arch = 0; arch < archs.count; arch++)
    {
        strarray_addall_uniq( &make->clean_files, make->object_files[arch] );
        strarray_addall_uniq( &make->clean_files, make->implib_files[arch] );
        strarray_addall_uniq( &make->clean_files, make->res_files[arch] );
        strarray_addall_uniq( &make->clean_files, make->all_targets[arch] );
    }
    strarray_addall( &make->clean_files, make->unixobj_files );
    strarray_addall( &make->clean_files, make->pot_files );
    strarray_addall( &make->clean_files, make->debug_files );
    strarray_addall( &make->clean_files, make->sast_files );

    if (make == top_makefile)
    {
        output_subdirs( make );
        return;
    }

    if (!strcmp( make->obj_dir, "po" )) strarray_add( &make->distclean_files, "LINGUAS" );

    for (arch = 0; arch < archs.count; arch++) strarray_addall( &all_targets, make->all_targets[arch] );
    strarray_addall( &all_targets, make->font_files );
    if (all_targets.count)
    {
        output( "%s:", obj_dir_path( make, "all" ));
        output_filenames_obj_dir( make, all_targets );
        output( "\n" );
        strarray_add_uniq( &make->phony_targets, obj_dir_path( make, "all" ));
    }
    for (i = 0; i < NB_INSTALL_RULES; i++) output_install_rules( make, i );

    if (make->clean_files.count)
    {
        output( "%s::\n", obj_dir_path( make, "clean" ));
        output( "\trm -f" );
        output_filenames_obj_dir( make, make->clean_files );
        output( "\n" );
        strarray_add( &make->phony_targets, obj_dir_path( make, "clean" ));
    }
}


/*******************************************************************
 *         create_temp_file
 */
static FILE *create_temp_file( const char *orig )
{
    char *name = xmalloc( strlen(orig) + 13 );
    unsigned int i, id = getpid();
    int fd;
    FILE *ret = NULL;

    for (i = 0; i < 100; i++)
    {
        snprintf( name, strlen(orig) + 13, "%s.tmp%08x", orig, id );
        if ((fd = open( name, O_RDWR | O_CREAT | O_EXCL, 0666 )) != -1)
        {
            ret = fdopen( fd, "w" );
            break;
        }
        if (errno != EEXIST) break;
        id += 7777;
    }
    if (!ret) fatal_error( "failed to create output file for '%s'\n", orig );
    temp_file_name = name;
    return ret;
}


/*******************************************************************
 *         rename_temp_file
 */
static void rename_temp_file( const char *dest )
{
    int ret = rename( temp_file_name, dest );
    if (ret == -1 && errno == EEXIST)
    {
        /* rename doesn't overwrite on windows */
        unlink( dest );
        ret = rename( temp_file_name, dest );
    }
    if (ret == -1) fatal_error( "failed to rename output file to '%s'\n", dest );
    temp_file_name = NULL;
}


/*******************************************************************
 *         are_files_identical
 */
static int are_files_identical( FILE *file1, FILE *file2 )
{
    for (;;)
    {
        char buffer1[8192], buffer2[8192];
        int size1 = fread( buffer1, 1, sizeof(buffer1), file1 );
        int size2 = fread( buffer2, 1, sizeof(buffer2), file2 );
        if (size1 != size2) return 0;
        if (!size1) return feof( file1 ) && feof( file2 );
        if (memcmp( buffer1, buffer2, size1 )) return 0;
    }
}


/*******************************************************************
 *         rename_temp_file_if_changed
 */
static void rename_temp_file_if_changed( const char *dest )
{
    FILE *file1, *file2;
    int do_rename = 1;

    if ((file1 = fopen( dest, "r" )))
    {
        if ((file2 = fopen( temp_file_name, "r" )))
        {
            do_rename = !are_files_identical( file1, file2 );
            fclose( file2 );
        }
        fclose( file1 );
    }
    if (!do_rename)
    {
        unlink( temp_file_name );
        temp_file_name = NULL;
    }
    else rename_temp_file( dest );
}


/*******************************************************************
 *         output_linguas
 */
static void output_linguas( const struct makefile *make )
{
    const char *dest = obj_dir_path( make, "LINGUAS" );
    struct incl_file *source;

    output_file = create_temp_file( dest );

    output( "# Automatically generated by make depend; DO NOT EDIT!!\n" );
    LIST_FOR_EACH_ENTRY( source, &make->sources, struct incl_file, entry )
        if (strendswith( source->name, ".po" ))
            output( "%s\n", replace_extension( source->name, ".po", "" ));

    if (fclose( output_file )) fatal_perror( "write" );
    output_file = NULL;
    rename_temp_file_if_changed( dest );
}


/*******************************************************************
 *         output_compile_commands
 */
static void output_compile_commands( const char *dest )
{
    struct compile_command *cmd;
    unsigned int i;
    const char *dir;

    output_file = create_temp_file( dest );
    dir = escape_cstring( cwd );

    output( "[\n" );
    LIST_FOR_EACH_ENTRY( cmd, &compile_commands, struct compile_command, entry )
    {
        output( "  {\n" );
        output( "    \"command\": \"%s -c -o %s %s", cmd->cmd, cmd->obj, cmd->source );
        for (i = 0; i < cmd->args.count; i++) output( " %s", escape_cstring( cmd->args.str[i] ));
        output( "\",\n" );
        output( "    \"file\": \"%s\",\n", cmd->source );
        output( "    \"output\": \"%s\",\n", cmd->obj );
        output( "    \"directory\": \"%s\"\n", dir );
        output( "  }%s\n", list_next( &compile_commands, &cmd->entry ) ? "," : "" );
    }
    output( "]\n" );

    if (fclose( output_file )) fatal_perror( "write" );
    output_file = NULL;
    rename_temp_file( dest );
}


/*******************************************************************
 *         output_testlist
 */
static void output_testlist( const struct makefile *make )
{
    const char *dest = obj_dir_path( make, "testlist.c" );
    unsigned int i;

    output_file = create_temp_file( dest );

    output( "/* Automatically generated by make depend; DO NOT EDIT!! */\n\n" );
    output( "#define WIN32_LEAN_AND_MEAN\n" );
    output( "#include <windows.h>\n\n" );
    output( "#define STANDALONE\n" );
    output( "#include \"wine/test.h\"\n\n" );

    for (i = 0; i < make->test_files.count; i++)
        output( "extern void func_%s(void);\n", make->test_files.str[i] );
    output( "\n" );
    output( "const struct test winetest_testlist[] =\n" );
    output( "{\n" );
    for (i = 0; i < make->test_files.count; i++)
        output( "    { \"%s\", func_%s },\n", make->test_files.str[i], make->test_files.str[i] );
    output( "    { 0, 0 }\n" );
    output( "};\n" );

    if (fclose( output_file )) fatal_perror( "write" );
    output_file = NULL;
    rename_temp_file_if_changed( dest );
}


/*******************************************************************
 *         output_gitignore
 */
static void output_gitignore( const char *dest, struct strarray files )
{
    unsigned int i;

    output_file = create_temp_file( dest );

    output( "# Automatically generated by make depend; DO NOT EDIT!!\n" );
    for (i = 0; i < files.count; i++)
    {
        if (!strchr( files.str[i], '/' )) output( "/" );
        output( "%s\n", files.str[i] );
    }

    if (fclose( output_file )) fatal_perror( "write" );
    output_file = NULL;
    rename_temp_file( dest );
}


/*******************************************************************
 *         output_stub_makefile
 */
static void output_stub_makefile( struct makefile *make )
{
    struct strarray targets = empty_strarray;
    const char *make_var = strarray_get_value( top_makefile->vars, "MAKE" );
    unsigned int i, arch;

    for (arch = 0; arch < archs.count; arch++)
        if (make->all_targets[arch].count) strarray_add_uniq( &targets, "all" );

    for (i = 0; i < NB_INSTALL_RULES; i++)
    {
        if (!make->install_rules[i].count) continue;
        strarray_add_uniq( &targets, "install" );
        strarray_add( &targets, install_targets[i] );
    }
    if (make->clean_files.count) strarray_add( &targets, "clean" );
    if (make->test_files.count)
    {
        strarray_add( &targets, "check" );
        strarray_add( &targets, "test" );
        strarray_add( &targets, "testclean" );
    }

    if (!targets.count && !make->clean_files.count) return;

    output_file_name = obj_dir_path( make, "Makefile" );
    output_file = create_temp_file( output_file_name );

    output( "# Auto-generated stub makefile; all rules forward to the top-level makefile\n\n" );

    if (make_var) output( "MAKE = %s\n\n", make_var );

    output( "all:\n" );
    output_filenames( targets );
    output_filenames( make->clean_files );
    output( ":\n" );
    output( "\t@cd %s && $(MAKE) %s/$@\n", get_root_relative_path( make ), make->obj_dir );
    output( ".PHONY:" );
    output_filenames( targets );
    output( "\n" );

    fclose( output_file );
    output_file = NULL;
    rename_temp_file( output_file_name );
}


/*******************************************************************
 *         output_silent_rules
 */
static void output_silent_rules(void)
{
    static const char *cmds[] =
    {
        "BISON",
        "BUILD",
        "CC",
        "CCLD",
        "FLEX",
        "GEN",
        "LN",
        "MSG",
        "SAST",
        "SED",
        "TEST",
        "WIDL",
        "WMC",
        "WRC"
    };
    unsigned int i;

    output( "V = 0\n" );
    for (i = 0; i < ARRAY_SIZE(cmds); i++)
    {
        output( "quiet_%s = $(quiet_%s_$(V))\n", cmds[i], cmds[i] );
        output( "quiet_%s_0 = @echo \"  %-5s \" $@;\n", cmds[i], cmds[i] );
        output( "quiet_%s_1 =\n", cmds[i] );
    }
}


/*******************************************************************
 *         output_top_makefile
 */
static void output_top_makefile( struct makefile *make )
{
    char buffer[1024];
    FILE *src_file;
    unsigned int i;
    int found = 0;

    output_file_name = obj_dir_path( make, output_makefile_name );
    output_file = create_temp_file( output_file_name );

    /* copy the contents of the source makefile */
    src_file = open_input_makefile( make );
    while (fgets( buffer, sizeof(buffer), src_file ) && !found)
    {
        if (fwrite( buffer, 1, strlen(buffer), output_file ) != strlen(buffer)) fatal_perror( "write" );
        found = !strncmp( buffer, separator, strlen(separator) );
    }
    if (fclose( src_file )) fatal_perror( "close" );
    input_file_name = NULL;

    if (!found) output( "\n%s (everything below this line is auto-generated; DO NOT EDIT!!)\n", separator );

    if (silent_rules) output_silent_rules();

    /* add special targets for makefile and dependencies */

    output( ".INIT: Makefile\n" );
    output( ".PRECIOUS: Makefile\n" );
    output( ".MAKEFILEDEPS:\n" );
    output( ".SUFFIXES:\n" );
    output( "Makefile: config.status %s\n", makedep );
    output( "\t@./config.status Makefile\n" );
    output( "config.status: %s\n", root_src_dir_path( "configure" ));
    output( "\t@./config.status --recheck\n" );
    strarray_add( &make->distclean_files, "config.status" );
    output( "include/config.h: include/stamp-h\n" );
    output( "include/stamp-h: %s config.status\n", root_src_dir_path( "include/config.h.in" ));
    output( "\t@./config.status include/config.h include/stamp-h\n" );
    strarray_add( &make->distclean_files, "include/config.h" );
    strarray_add( &make->distclean_files, "include/stamp-h" );
    output( "depend: %s\n", makedep );
    output( "\t%s%s%s\n", makedep,
            compile_commands_mode ? " -C" : "",
            silent_rules ? " -S" : "" );
    strarray_add( &make->phony_targets, "depend" );

    if (!strarray_exists( disabled_dirs[0], "tools/wine" ))
    {
        const char *loader = "tools/wine/wine";
        if (!strarray_exists( subdirs, "tools/wine" )) loader = tools_path( "wine" );
        output( "wine: %s\n", loader );
        output( "\t%srm -f $@ && %s %s $@\n", cmd_prefix( "LN" ), ln_s, loader );
        strarray_add( &make->all_targets[0], "wine" );
    }

    if (wine64_dir)
    {
        output( "loader-wow64:\n" );
	output( "\t%srm -f $@ && %s %s/loader $@\n", cmd_prefix( "LN" ), ln_s, wine64_dir );
        output( "%s/loader-wow64:\n", wine64_dir );
	output( "\t%srm -f $@ && %s %s/loader $@\n", cmd_prefix( "LN" ), ln_s, cwd );
        output( "all: %s/loader-wow64\n", wine64_dir );
        strarray_add( &make->all_targets[0], "loader-wow64" );
    }
    else strarray_add( &make->clean_files, "loader-wow64" );

    if (compile_commands_mode) strarray_add( &make->distclean_files, "compile_commands.json" );
    strarray_addall( &make->distclean_files, get_expanded_make_var_array( make, "CONFIGURE_TARGETS" ));
    if (!make->src_dir)
    {
        strarray_add( &make->maintainerclean_files, "configure" );
        strarray_add( &make->maintainerclean_files, "include/config.h.in" );
    }

    for (i = 0; i < subdirs.count; i++) output_sources( submakes[i] );
    output_sources( make );

    fclose( output_file );
    output_file = NULL;
    rename_temp_file( output_file_name );
}


/*******************************************************************
 *         output_dependencies
 */
static void output_dependencies( struct makefile *make )
{
    struct strarray ignore_files = empty_strarray;

    if (make->obj_dir) create_dir( make->obj_dir );

    if (make == top_makefile) output_top_makefile( make );
    else output_stub_makefile( make );

    strarray_addall( &ignore_files, make->distclean_files );
    strarray_addall( &ignore_files, make->clean_files );
    if (make->testdll) output_testlist( make );
    if (make->obj_dir && !strcmp( make->obj_dir, "po" )) output_linguas( make );
    if (!make->src_dir) output_gitignore( obj_dir_path( make, ".gitignore" ), ignore_files );

    create_file_directories( make, ignore_files );

    output_file_name = NULL;
}


/*******************************************************************
 *         load_sources
 */
static void load_sources( struct makefile *make )
{
    unsigned int i, arch;
    struct strarray value;
    struct incl_file *file;

    strarray_set_value( &make->vars, "top_srcdir", root_src_dir_path( "" ));
    strarray_set_value( &make->vars, "srcdir", src_dir_path( make, "" ));

    make->parent_dir    = get_expanded_make_variable( make, "PARENTSRC" );
    make->module        = get_expanded_make_variable( make, "MODULE" );
    make->testdll       = get_expanded_make_variable( make, "TESTDLL" );
    make->staticlib     = get_expanded_make_variable( make, "STATICLIB" );
    make->importlib     = get_expanded_make_variable( make, "IMPORTLIB" );
    make->extlib        = get_expanded_make_variable( make, "EXTLIB" );
    if (unix_lib_supported) make->unixlib = get_expanded_make_variable( make, "UNIXLIB" );

    make->programs      = get_expanded_make_var_array( make, "PROGRAMS" );
    make->scripts       = get_expanded_make_var_array( make, "SCRIPTS" );
    make->imports       = get_expanded_make_var_array( make, "IMPORTS" );
    make->delayimports  = get_expanded_make_var_array( make, "DELAYIMPORTS" );
    make->extradllflags = get_expanded_make_var_array( make, "EXTRADLLFLAGS" );
    make->extra_targets = get_expanded_make_var_array( make, "EXTRA_TARGETS" );
    for (i = 0; i < NB_INSTALL_RULES; i++)
        make->install[i] = get_expanded_make_var_array( make, install_variables[i] );

    if (make->extlib) make->staticlib = make->extlib;
    if (make->staticlib) make->module = make->staticlib;

    if (make->obj_dir)
    {
        make->disabled[0] = strarray_exists( disabled_dirs[0], make->obj_dir );
        for (arch = 1; arch < archs.count; arch++)
            make->disabled[arch] = make->disabled[0] || strarray_exists( disabled_dirs[arch], make->obj_dir );
    }
    make->is_win16   = strarray_exists( make->extradllflags, "-m16" );
    make->data_only  = strarray_exists( make->extradllflags, "-Wb,--data-only" );
    make->is_exe     = strarray_exists( make->extradllflags, "-mconsole" ) ||
                       strarray_exists( make->extradllflags, "-mwindows" );

    if (make->module)
    {
        /* add default install rules if nothing was specified */
        for (i = 0; i < NB_INSTALL_RULES; i++) if (make->install[i].count) break;
        if (i == NB_INSTALL_RULES && !make->extlib)
        {
            if (make->importlib) strarray_add( &make->install[INSTALL_DEV], make->importlib );
            if (make->staticlib) strarray_add( &make->install[INSTALL_DEV], make->staticlib );
            else strarray_add( &make->install[INSTALL_LIB], make->module );
        }
    }

    make->include_paths = empty_strarray;
    make->include_args = empty_strarray;
    make->define_args = empty_strarray;
    make->unix_cflags = empty_strarray;
    if (!make->extlib) strarray_add( &make->define_args, "-D__WINESRC__" );
    strarray_add( &make->unix_cflags, "-DWINE_UNIX_LIB" );

    value = get_expanded_make_var_array( make, "EXTRAINCL" );
    for (i = 0; i < value.count; i++)
    {
        if (!strncmp( value.str[i], "-I", 2 ))
        {
            const char *dir = value.str[i] + 2;
            if (!strncmp( dir, "./", 2 ))
            {
                dir += 2;
                while (*dir == '/') dir++;
            }
            strarray_add_uniq( &make->include_paths, dir );
        }
        else if (!strncmp( value.str[i], "-D", 2 ) || !strncmp( value.str[i], "-U", 2 ))
            strarray_add_uniq( &make->define_args, value.str[i] );
    }
    strarray_addall( &make->define_args, get_expanded_make_var_array( make, "EXTRADEFS" ));
    strarray_addall_uniq( &make->unix_cflags, get_expanded_make_var_array( make, "UNIX_CFLAGS" ));

    strarray_add( &make->include_args, strmake( "-I%s", obj_dir_path( make, "" )));
    if (make->src_dir)
        strarray_add( &make->include_args, strmake( "-I%s", make->src_dir ));
    if (make->parent_dir)
        strarray_add( &make->include_args, strmake( "-I%s", src_dir_path( make, make->parent_dir )));
    strarray_add( &make->include_args, "-Iinclude" );
    if (root_src_dir) strarray_add( &make->include_args, strmake( "-I%s", root_src_dir_path( "include" )));

    list_init( &make->sources );
    list_init( &make->includes );

    value = get_expanded_make_var_array( make, "SOURCES" );
    for (i = 0; i < value.count; i++) add_src_file( make, value.str[i] );

    add_generated_sources( make );

    LIST_FOR_EACH_ENTRY( file, &make->includes, struct incl_file, entry ) parse_file( make, file, 0 );
    LIST_FOR_EACH_ENTRY( file, &make->sources, struct incl_file, entry ) get_dependencies( file, file );

    for (i = 0; i < make->delayimports.count; i++)
        strarray_add_uniq( &delay_import_libs, get_base_name( make->delayimports.str[i] ));
}


/*******************************************************************
 *         parse_makeflags
 */
static void parse_makeflags( const char *flags )
{
    const char *p = flags;
    char *var, *buffer = xmalloc( strlen(flags) + 1 );

    while (*p)
    {
        p = skip_spaces( p );
        var = buffer;
        while (*p && !isspace(*p))
        {
            if (*p == '\\' && p[1]) p++;
            *var++ = *p++;
        }
        *var = 0;
        if (var > buffer) set_make_variable( &cmdline_vars, buffer );
    }
}


/*******************************************************************
 *         parse_option
 */
static int parse_option( const char *opt )
{
    if (opt[0] != '-')
    {
        if (strchr( opt, '=' )) return set_make_variable( &cmdline_vars, opt );
        return 0;
    }
    switch(opt[1])
    {
    case 'f':
        if (opt[2]) output_makefile_name = opt + 2;
        break;
    case 'i':
        if (opt[2]) input_makefile_name = opt + 2;
        break;
    case 'C':
        compile_commands_mode = 1;
        break;
    case 'S':
        silent_rules = 1;
        break;
    default:
        fprintf( stderr, "Unknown option '%s'\n%s", opt, Usage );
        exit(1);
    }
    return 1;
}


/*******************************************************************
 *         find_pe_arch
 */
static unsigned int find_pe_arch( const char *arch )
{
    unsigned int i;
    for (i = 1; i < archs.count; i++) if (!strcmp( archs.str[i], arch )) return i;
    return 0;
}


/*******************************************************************
 *         main
 */
int main( int argc, char *argv[] )
{
    const char *makeflags = getenv( "MAKEFLAGS" );
    const char *target;
    unsigned int i, j, arch, ec_arch;

    if (makeflags) parse_makeflags( makeflags );

    i = 1;
    while (i < argc)
    {
        if (parse_option( argv[i] ))
        {
            for (j = i; j < argc; j++) argv[j] = argv[j+1];
            argc--;
        }
        else i++;
    }

    if (argc > 1)
    {
        fprintf( stderr, "Unknown argument '%s'\n%s", argv[1], Usage );
        exit(1);
    }

    atexit( cleanup_files );
    init_signals( exit_on_signal );
    getcwd( cwd, sizeof(cwd) );

    for (i = 0; i < HASH_SIZE; i++) list_init( &files[i] );
    for (i = 0; i < HASH_SIZE; i++) list_init( &global_includes[i] );

    top_makefile = parse_makefile( NULL );

    target_flags[0]    = get_expanded_make_var_array( top_makefile, "TARGETFLAGS" );
    msvcrt_flags       = get_expanded_make_var_array( top_makefile, "MSVCRTFLAGS" );
    dll_flags          = get_expanded_make_var_array( top_makefile, "DLLFLAGS" );
    unix_dllflags      = get_expanded_make_var_array( top_makefile, "UNIXDLLFLAGS" );
    cpp_flags          = get_expanded_make_var_array( top_makefile, "CPPFLAGS" );
    lddll_flags        = get_expanded_make_var_array( top_makefile, "LDDLLFLAGS" );
    libs               = get_expanded_make_var_array( top_makefile, "LIBS" );
    enable_tests       = get_expanded_make_var_array( top_makefile, "ENABLE_TESTS" );
    for (i = 0; i < NB_INSTALL_RULES; i++)
        top_install[i] = get_expanded_make_var_array( top_makefile, strmake( "TOP_%s", install_variables[i] ));

    root_src_dir       = get_expanded_make_variable( top_makefile, "srcdir" );
    tools_dir          = get_expanded_make_variable( top_makefile, "toolsdir" );
    tools_ext          = get_expanded_make_variable( top_makefile, "toolsext" );
    wine64_dir         = get_expanded_make_variable( top_makefile, "wine64dir" );
    exe_ext            = get_expanded_make_variable( top_makefile, "EXEEXT" );
    dll_ext[0]         = get_expanded_make_variable( top_makefile, "DLLEXT" );
    fontforge          = get_expanded_make_variable( top_makefile, "FONTFORGE" );
    convert            = get_expanded_make_variable( top_makefile, "CONVERT" );
    flex               = get_expanded_make_variable( top_makefile, "FLEX" );
    bison              = get_expanded_make_variable( top_makefile, "BISON" );
    rsvg               = get_expanded_make_variable( top_makefile, "RSVG" );
    icotool            = get_expanded_make_variable( top_makefile, "ICOTOOL" );
    msgfmt             = get_expanded_make_variable( top_makefile, "MSGFMT" );
    sed_cmd            = get_expanded_make_variable( top_makefile, "SED_CMD" );
    ln_s               = get_expanded_make_variable( top_makefile, "LN_S" );
    wayland_scanner    = get_expanded_make_variable( top_makefile, "WAYLAND_SCANNER" );
    sarif_converter    = get_expanded_make_variable( top_makefile, "SARIF_CONVERTER" );
    compiler_rt        = get_expanded_make_variable( top_makefile, "COMPILER_RT_PE_LIBS" );

    if (root_src_dir && !strcmp( root_src_dir, "." )) root_src_dir = NULL;
    if (tools_dir && !strcmp( tools_dir, "." )) tools_dir = NULL;
    if (!exe_ext) exe_ext = "";
    if (!dll_ext[0]) dll_ext[0] = "";
    if (!tools_ext) tools_ext = "";

    buildimage  = root_src_dir_path( "tools/buildimage" );
    runtest     = root_src_dir_path( "tools/runtest" );
    install_sh  = root_src_dir_path( "tools/install-sh" );
    makedep     = tools_base_path( "makedep" );
    make_xftmpl = tools_base_path( "make_xftmpl" );
    sfnt2fon    = tools_path( "sfnt2fon" );
    winebuild   = tools_path( "winebuild" );
    winegcc     = tools_path( "winegcc" );
    widl        = tools_path( "widl" );
    wrc         = tools_path( "wrc" );
    wmc         = tools_path( "wmc" );

    unix_lib_supported = !!strcmp( exe_ext, ".exe" );
    so_dll_supported = !!dll_ext[0][0];  /* non-empty dll ext means supported */

    strarray_add( &archs, get_expanded_make_variable( top_makefile, "HOST_ARCH" ));
    strarray_addall( &archs, get_expanded_make_var_array( top_makefile, "PE_ARCHS" ));

    /* check for ARM64X setup */
    if ((ec_arch = find_pe_arch( "arm64ec" )) && (arch = find_pe_arch( "aarch64" )))
    {
        native_archs[ec_arch] = arch;
        hybrid_archs[arch] = ec_arch;
        strarray_add( &hybrid_target_flags[ec_arch], "-marm64x" );
    }

    arch_dirs[0] = "";
    arch_install_dirs[0] = unix_lib_supported ? strmake( "$(libdir)/wine/%s-unix", archs.str[0] ) : "$(libdir)/wine";
    strip_progs[0] = "\"$(STRIP)\"";

    for (arch = 1; arch < archs.count; arch++)
    {
        target = get_expanded_arch_var( top_makefile, "TARGET", arch );
        strarray_add( &target_flags[arch], "-b" );
        strarray_add( &target_flags[arch], target );
        arch_dirs[arch] = strmake( "%s-windows/", archs.str[arch] );
        arch_install_dirs[arch] = strmake( "$(libdir)/wine/%s-windows", archs.str[arch] );
        strip_progs[arch] = get_expanded_arch_var( top_makefile, "STRIP", arch );
        dll_ext[arch] = "";
    }

    for (arch = 0; arch < archs.count; arch++)
    {
        arch_pe_dirs[arch] = strmake( "%s-windows", archs.str[arch] );
        extra_cflags[arch] = get_expanded_arch_var_array( top_makefile, "EXTRACFLAGS", arch );
        extra_cflags_extlib[arch] = remove_warning_flags( extra_cflags[arch] );
        disabled_dirs[arch] = get_expanded_arch_var_array( top_makefile, "DISABLED_SUBDIRS", arch );
        if (!is_multiarch( arch )) continue;
        delay_load_flags[arch] = get_expanded_arch_var( top_makefile, "DELAYLOADFLAG", arch );
        debug_flags[arch] = get_expanded_arch_var_array( top_makefile, "DEBUG", arch );
    }

    if (unix_lib_supported)
    {
        delay_load_flags[0] = "-Wl,-delayload,";
        debug_flags[0].count = 0;
    }

    top_makefile->src_dir = root_src_dir;
    subdirs = get_expanded_make_var_array( top_makefile, "SUBDIRS" );
    submakes = xmalloc( subdirs.count * sizeof(*submakes) );

    for (i = 0; i < subdirs.count; i++) submakes[i] = parse_makefile( subdirs.str[i] );

    load_sources( top_makefile );
    load_sources( include_makefile );
    for (i = 0; i < subdirs.count; i++)
        if (submakes[i] != include_makefile) load_sources( submakes[i] );

    output_dependencies( top_makefile );
    for (i = 0; i < subdirs.count; i++) output_dependencies( submakes[i] );

    if (compile_commands_mode) output_compile_commands( "compile_commands.json" );

    return 0;
}
