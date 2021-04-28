/*
 * MinGW wrapper: makes gcc behave like MinGW.
 *
 * Copyright 2000 Manuel Novoa III
 * Copyright 2000 Francois Gouget
 * Copyright 2002 Dimitrie O. Paun
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
 *
 * DESCRIPTION
 *
 * all options for gcc start with '-' and are for the most part
 * single options (no parameters as separate argument). 
 * There are of course exceptions to this rule, so here is an 
 * exhaustive list of options that do take parameters (potentially)
 * as a separate argument:
 *
 * Compiler:
 * -x language
 * -o filename
 * -aux-info filename
 *
 * Preprocessor:
 * -D name 
 * -U name
 * -I dir
 * -MF file
 * -MT target
 * -MQ target
 * (all -i.* arg)
 * -include file 
 * -imacros file
 * -idirafter dir
 * -iwithprefix dir
 * -iwithprefixbefore dir
 * -isystem dir
 * -A predicate=answer
 *
 * Linking:
 * -l library
 * -Xlinker option
 * -u symbol
 *
 * Misc:
 * -b machine
 * -V version
 * -G num  (see NOTES below)
 *
 * NOTES
 * There is -G option for compatibility with System V that
 * takes no parameters. This makes "-G num" parsing ambiguous.
 * This option is synonymous to -shared, and as such we will
 * not support it for now.
 *
 * Special interest options 
 *
 *      Assembler Option
 *          -Wa,option
 *
 *      Linker Options
 *          object-file-name  -llibrary -nostartfiles  -nodefaultlibs
 *          -nostdlib -s  -static  -static-libgcc  -static-libstdc++
 *          -shared  -shared-libgcc  -symbolic  -Wl,option
 *          -Xlinker option -u symbol --image-base -fuse-ld
 *
 *      Directory Options
 *          -Bprefix  -Idir  -I-  -Ldir  -specs=file
 *
 *      Target Options
 *          -b machine  -V version
 *
 * Please note that the Target Options are relevant to everything:
 *   compiler, linker, assembler, preprocessor.
 * 
 */ 

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "utils.h"

static const char* app_loader_template =
    "#!/bin/sh\n"
    "\n"
    "appname=\"%s\"\n"
    "# determine the application directory\n"
    "appdir=''\n"
    "case \"$0\" in\n"
    "  */*)\n"
    "    # $0 contains a path, use it\n"
    "    appdir=`dirname \"$0\"`\n"
    "    ;;\n"
    "  *)\n"
    "    # no directory in $0, search in PATH\n"
    "    saved_ifs=$IFS\n"
    "    IFS=:\n"
    "    for d in $PATH\n"
    "    do\n"
    "      IFS=$saved_ifs\n"
    "      if [ -x \"$d/$appname\" ]; then appdir=\"$d\"; break; fi\n"
    "    done\n"
    "    ;;\n"
    "esac\n"
    "\n"
    "# figure out the full app path\n"
    "if [ -n \"$appdir\" ]; then\n"
    "    apppath=\"$appdir/$appname\"\n"
    "    WINEDLLPATH=\"$appdir:$WINEDLLPATH\"\n"
    "    export WINEDLLPATH\n"
    "else\n"
    "    apppath=\"$appname\"\n"
    "fi\n"
    "\n"
    "# determine the WINELOADER\n"
    "if [ ! -x \"$WINELOADER\" ]; then WINELOADER=\"wine\"; fi\n"
    "\n"
    "# and try to start the app\n"
    "exec \"$WINELOADER\" \"$apppath\" \"$@\"\n"
;

static const char *output_file_name;
static const char *output_debug_file;
static const char *output_implib;
static int keep_generated = 0;
static strarray* tmp_files;
#ifdef HAVE_SIGSET_T
static sigset_t signal_mask;
#endif

static const char *bindir;
static const char *libdir;
static const char *includedir;

enum processor { proc_cc, proc_cxx, proc_cpp, proc_as };

static const struct
{
    const char *name;
    enum target_cpu cpu;
} cpu_names[] =
{
    { "i386",    CPU_x86 },
    { "i486",    CPU_x86 },
    { "i586",    CPU_x86 },
    { "i686",    CPU_x86 },
    { "i786",    CPU_x86 },
    { "amd64",   CPU_x86_64 },
    { "x86_64",  CPU_x86_64 },
    { "powerpc", CPU_POWERPC },
    { "arm",     CPU_ARM },
    { "armv5",   CPU_ARM },
    { "armv6",   CPU_ARM },
    { "armv7",   CPU_ARM },
    { "armv7a",  CPU_ARM },
    { "arm64",   CPU_ARM64 },
    { "aarch64", CPU_ARM64 },
};

static const struct
{
    const char *name;
    enum target_platform platform;
} platform_names[] =
{
    { "macos",       PLATFORM_APPLE },
    { "darwin",      PLATFORM_APPLE },
    { "android",     PLATFORM_ANDROID },
    { "solaris",     PLATFORM_SOLARIS },
    { "cygwin",      PLATFORM_CYGWIN },
    { "mingw32",     PLATFORM_MINGW },
    { "windows-gnu", PLATFORM_MINGW },
    { "windows",     PLATFORM_WINDOWS },
    { "winnt",       PLATFORM_MINGW }
};

struct options
{
    enum processor processor;
    enum target_cpu target_cpu;
    enum target_platform target_platform;
    const char *target;
    const char *version;
    int shared;
    int use_msvcrt;
    int nostdinc;
    int nostdlib;
    int nostartfiles;
    int nodefaultlibs;
    int noshortwchar;
    int unix_lib;
    int gui_app;
    int unicode_app;
    int win16_app;
    int compile_only;
    int force_pointer_size;
    int large_address_aware;
    int wine_builtin;
    int unwind_tables;
    int strip;
    int pic;
    const char* wine_objdir;
    const char* winebuild;
    const char* output_name;
    const char* image_base;
    const char* section_align;
    const char* file_align;
    const char* sysroot;
    const char* isysroot;
    const char* lib_suffix;
    const char* subsystem;
    const char* entry_point;
    const char* prelink;
    const char* debug_file;
    const char* out_implib;
    strarray* prefix;
    strarray* lib_dirs;
    strarray* args;
    strarray* linker_args;
    strarray* compiler_args;
    strarray* winebuild_args;
    strarray* files;
    strarray* delayimports;
};

#ifdef __i386__
static const enum target_cpu build_cpu = CPU_x86;
#elif defined(__x86_64__)
static const enum target_cpu build_cpu = CPU_x86_64;
#elif defined(__powerpc__)
static const enum target_cpu build_cpu = CPU_POWERPC;
#elif defined(__arm__)
static const enum target_cpu build_cpu = CPU_ARM;
#elif defined(__aarch64__)
static const enum target_cpu build_cpu = CPU_ARM64;
#else
#error Unsupported CPU
#endif

#ifdef __APPLE__
static enum target_platform build_platform = PLATFORM_APPLE;
#elif defined(__ANDROID__)
static enum target_platform build_platform = PLATFORM_ANDROID;
#elif defined(__sun)
static enum target_platform build_platform = PLATFORM_SOLARIS;
#elif defined(__CYGWIN__)
static enum target_platform build_platform = PLATFORM_CYGWIN;
#elif defined(_WIN32)
static enum target_platform build_platform = PLATFORM_MINGW;
#else
static enum target_platform build_platform = PLATFORM_UNSPECIFIED;
#endif

static void cleanup_output_files(void)
{
    if (output_file_name) unlink( output_file_name );
    if (output_debug_file) unlink( output_debug_file );
    if (output_implib) unlink( output_implib );
}

static void clean_temp_files(void)
{
    unsigned int i;

    if (keep_generated) return;

    for (i = 0; i < tmp_files->size; i++)
	unlink(tmp_files->base[i]);
}

/* clean things up when aborting on a signal */
static void exit_on_signal( int sig )
{
    exit(1);  /* this will call the atexit functions */
}

static char* get_temp_file(const char* prefix, const char* suffix)
{
    int fd;
    char* tmp = strmake("%s-XXXXXX%s", prefix, suffix);

#ifdef HAVE_SIGPROCMASK
    sigset_t old_set;
    /* block signals while manipulating the temp files list */
    sigprocmask( SIG_BLOCK, &signal_mask, &old_set );
#endif
    fd = mkstemps( tmp, strlen(suffix) );
    if (fd == -1)
    {
        /* could not create it in current directory, try in TMPDIR */
        const char* tmpdir;

        free(tmp);
        if (!(tmpdir = getenv("TMPDIR"))) tmpdir = "/tmp";
        tmp = strmake("%s/%s-XXXXXX%s", tmpdir, prefix, suffix);
        fd = mkstemps( tmp, strlen(suffix) );
        if (fd == -1) error( "could not create temp file\n" );
    }
    close( fd );
    strarray_add(tmp_files, tmp);
#ifdef HAVE_SIGPROCMASK
    sigprocmask( SIG_SETMASK, &old_set, NULL );
#endif
    return tmp;
}

enum tool
{
    TOOL_CC,
    TOOL_CXX,
    TOOL_CPP,
    TOOL_LD,
    TOOL_OBJCOPY,
};

static const struct
{
    const char *base;
    const char *llvm_base;
    const char *deflt;
} tool_names[] =
{
    { "gcc",     "clang --driver-mode=gcc", CC },
    { "g++",     "clang --driver-mode=g++", CXX },
    { "cpp",     "clang --driver-mode=cpp", CPP },
    { "ld",      "ld.lld",                  LD },
    { "objcopy", "llvm-objcopy" },
};

static strarray* build_tool_name( struct options *opts, enum tool tool )
{
    const char *base = tool_names[tool].base;
    const char *llvm_base = tool_names[tool].llvm_base;
    const char *deflt = tool_names[tool].deflt;
    const char *path;
    strarray *ret;
    char* str;

    if (opts->target && opts->version)
    {
        str = strmake("%s-%s-%s", opts->target, base, opts->version);
    }
    else if (opts->target)
    {
        str = strmake("%s-%s", opts->target, base);
    }
    else if (opts->version)
    {
        str = strmake("%s-%s", base, opts->version);
    }
    else
        str = xstrdup((deflt && *deflt) ? deflt : base);

    if ((path = find_binary( opts->prefix, str ))) return strarray_fromstring( path, " " );

    if (!opts->version) str = xstrdup( llvm_base );
    else str = strmake( "%s-%s", llvm_base, opts->version );
    path = find_binary( opts->prefix, str );
    if (!path)
    {
        error( "Could not find %s\n", base );
        return NULL;
    }

    ret = strarray_fromstring( path, " " );
    if (!strncmp( llvm_base, "clang", 5 ))
    {
        if (opts->target)
        {
            strarray_add( ret, "-target" );
            strarray_add( ret, opts->target );
        }
        strarray_add( ret, "-Wno-unused-command-line-argument" );
        strarray_add( ret, "-fuse-ld=lld" );
    }
    return ret;
}

static strarray* get_translator(struct options *opts)
{
    enum tool tool;

    switch(opts->processor)
    {
    case proc_cpp:
        tool = TOOL_CPP;
        break;
    case proc_cc:
    case proc_as:
        tool = TOOL_CC;
        break;
    case proc_cxx:
        tool = TOOL_CXX;
        break;
    default:
        assert(0);
    }
    return build_tool_name( opts, tool );
}

static int try_link( const strarray *prefix, const strarray *link_tool, const char *cflags )
{
    const char *in = get_temp_file( "try_link", ".c" );
    const char *out = get_temp_file( "try_link", ".out" );
    const char *err = get_temp_file( "try_link", ".err" );
    strarray *link = strarray_dup( link_tool );
    int sout = -1, serr = -1;
    int ret;

    create_file( in, 0644, "int main(void){return 1;}\n" );

    strarray_add( link, "-o" );
    strarray_add( link, out );
    strarray_addall( link, strarray_fromstring( cflags, " " ) );
    strarray_add( link, in );

    sout = dup( fileno(stdout) );
    freopen( err, "w", stdout );
    serr = dup( fileno(stderr) );
    freopen( err, "w", stderr );
    ret = spawn( prefix, link, 1 );
    if (sout >= 0)
    {
        dup2( sout, fileno(stdout) );
        close( sout );
    }
    if (serr >= 0)
    {
        dup2( serr, fileno(stderr) );
        close( serr );
    }
    strarray_free( link );
    return ret;
}

static strarray *get_link_args( struct options *opts, const char *output_name )
{
    strarray *link_args = get_translator( opts );
    strarray *flags = strarray_alloc();

    strarray_addall( link_args, opts->linker_args );

    if (verbose > 1) strarray_add( flags, "-v" );

    switch (opts->target_platform)
    {
    case PLATFORM_APPLE:
        strarray_add( flags, "-bundle" );
        strarray_add( flags, "-multiply_defined" );
        strarray_add( flags, "suppress" );
        if (opts->target_cpu == CPU_POWERPC)
        {
            strarray_add( flags, "-read_only_relocs" );
            strarray_add( flags, "warning" );
        }
        if (opts->image_base)
        {
            strarray_add( flags, "-image_base" );
            strarray_add( flags, opts->image_base );
        }
        if (opts->strip) strarray_add( flags, "-Wl,-x" );
        strarray_addall( link_args, flags );
        return link_args;

    case PLATFORM_SOLARIS:
        {
            char *mapfile = get_temp_file( output_name, ".map" );
            const char *align = opts->section_align ? opts->section_align : "0x1000";

            create_file( mapfile, 0644, "text = A%s;\ndata = A%s;\n", align, align );
            strarray_add( flags, strmake("-Wl,-M,%s", mapfile) );
            strarray_add( tmp_files, mapfile );
        }
        break;

    case PLATFORM_ANDROID:
        /* the Android loader requires a soname for all libraries */
        strarray_add( flags, strmake( "-Wl,-soname,%s.so", output_name ));
        break;

    case PLATFORM_MINGW:
    case PLATFORM_CYGWIN:
        if (opts->shared || opts->win16_app)
        {
            strarray_add( flags, "-shared" );
            strarray_add( flags, "-Wl,--kill-at" );
        }
        else strarray_add( flags, opts->gui_app ? "-mwindows" : "-mconsole" );

        if (opts->unicode_app) strarray_add( flags, "-municode" );
        if (opts->nodefaultlibs || opts->use_msvcrt) strarray_add( flags, "-nodefaultlibs" );
        if (opts->nostartfiles || opts->use_msvcrt) strarray_add( flags, "-nostartfiles" );
        if (opts->subsystem) strarray_add( flags, strmake("-Wl,--subsystem,%s", opts->subsystem ));

        strarray_add( flags, "-Wl,--nxcompat" );

        if (opts->image_base) strarray_add( flags, strmake("-Wl,--image-base,%s", opts->image_base ));

        if (opts->large_address_aware && opts->target_cpu == CPU_x86)
            strarray_add( flags, "-Wl,--large-address-aware" );

        /* make sure we don't need a libgcc_s dll on Windows */
        strarray_add( flags, "-static-libgcc" );

        if (opts->debug_file && strendswith(opts->debug_file, ".pdb"))
            strarray_add(link_args, strmake("-Wl,-pdb,%s", opts->debug_file));

        if (opts->out_implib)
            strarray_add(link_args, strmake("-Wl,--out-implib,%s", opts->out_implib));

        if (!try_link( opts->prefix, link_args, "-Wl,--file-alignment,0x1000" ))
            strarray_add( link_args, strmake( "-Wl,--file-alignment,%s",
                                              opts->file_align ? opts->file_align : "0x1000" ));
        else if (!try_link( opts->prefix, link_args, "-Wl,-Xlink=-filealign:0x1000" ))
            /* lld from llvm 10 does not support mingw style --file-alignment,
             * but it's possible to use msvc syntax */
            strarray_add( link_args, strmake( "-Wl,-Xlink=-filealign:%s",
                                              opts->file_align ? opts->file_align : "0x1000" ));

        strarray_addall( link_args, flags );
        return link_args;

    case PLATFORM_WINDOWS:
        if (opts->shared || opts->win16_app)
        {
            strarray_add( flags, "-shared" );
            strarray_add( flags, "-Wl,-kill-at" );
        }
        if (opts->unicode_app) strarray_add( flags, "-municode" );
        if (opts->nodefaultlibs || opts->use_msvcrt) strarray_add( flags, "-nodefaultlibs" );
        if (opts->nostartfiles || opts->use_msvcrt) strarray_add( flags, "-nostartfiles" );
        if (opts->image_base) strarray_add( flags, strmake("-Wl,-base:%s", opts->image_base ));
        if (opts->subsystem)
            strarray_add( flags, strmake("-Wl,-subsystem:%s", opts->subsystem ));
        else
            strarray_add( flags, strmake("-Wl,-subsystem:%s", opts->gui_app ? "windows" : "console" ));

        if (opts->debug_file && strendswith(opts->debug_file, ".pdb"))
        {
            strarray_add(link_args, "-Wl,-debug");
            strarray_add(link_args, strmake("-Wl,-pdb:%s", opts->debug_file));
        }
        else if (!opts->strip)
            strarray_add(link_args, "-Wl,-debug:dwarf");

        if (opts->out_implib)
            strarray_add(link_args, strmake("-Wl,-implib:%s", opts->out_implib));

        strarray_add( link_args, strmake( "-Wl,-filealign:%s", opts->file_align ? opts->file_align : "0x1000" ));

        strarray_addall( link_args, flags );
        return link_args;

    default:
        if (opts->image_base)
        {
            if (!try_link( opts->prefix, link_args, strmake("-Wl,-Ttext-segment=%s", opts->image_base)) )
                strarray_add( flags, strmake("-Wl,-Ttext-segment=%s", opts->image_base) );
            else
                opts->prelink = PRELINK;
        }
        if (!try_link( opts->prefix, link_args, "-Wl,-z,max-page-size=0x1000"))
            strarray_add( flags, "-Wl,-z,max-page-size=0x1000");
        break;
    }

    /* generic Unix shared library flags */

    strarray_add( link_args, "-shared" );
    strarray_add( link_args, "-Wl,-Bsymbolic" );
    if (!opts->noshortwchar && opts->target_cpu == CPU_ARM)
        strarray_add( flags, "-Wl,--no-wchar-size-warning" );
    if (!try_link( opts->prefix, link_args, "-Wl,-z,defs" ))
        strarray_add( flags, "-Wl,-z,defs" );

    strarray_addall( link_args, flags );
    return link_args;
}

static const char *get_multiarch_dir( enum target_cpu cpu )
{
   switch(cpu)
   {
   case CPU_x86:     return "/i386-linux-gnu";
   case CPU_x86_64:  return "/x86_64-linux-gnu";
   case CPU_ARM:     return "/arm-linux-gnueabi";
   case CPU_ARM64:   return "/aarch64-linux-gnu";
   case CPU_POWERPC: return "/powerpc-linux-gnu";
   default:
       assert(0);
       return NULL;
   }
}

static const char *get_wine_arch_dir( enum target_cpu target_cpu, enum target_platform target_platform )
{
    const char *cpu;

    switch (target_cpu)
    {
    case CPU_x86:     cpu = "i386";     break;
    case CPU_x86_64:  cpu = "x86_64";   break;
    case CPU_ARM:     cpu = "arm";      break;
    case CPU_ARM64:   cpu = "aarch64";  break;
    default: return "/wine";
    }
    switch (target_platform)
    {
    case PLATFORM_WINDOWS:
    case PLATFORM_CYGWIN:
    case PLATFORM_MINGW:
        return strmake( "/wine/%s-windows", cpu );
    default:
        return strmake( "/wine/%s-unix", cpu );
    }
}

static char *get_lib_dir( struct options *opts )
{
    const char *stdlibpath[] = { libdir, LIBDIR, "/usr/lib", "/usr/local/lib", "/lib" };
    const char *bit_suffix, *other_bit_suffix, *build_multiarch, *target_multiarch, *winecrt0;
    const char *root = opts->sysroot ? opts->sysroot : "";
    unsigned int i;
    struct stat st;
    size_t build_len, target_len;

    bit_suffix = opts->target_cpu == CPU_x86_64 || opts->target_cpu == CPU_ARM64 ? "64" : "32";
    other_bit_suffix = opts->target_cpu == CPU_x86_64 || opts->target_cpu == CPU_ARM64 ? "32" : "64";
    winecrt0 = strmake( "%s/libwinecrt0.a", get_wine_arch_dir( opts->target_cpu, opts->target_platform ));
    build_multiarch = get_multiarch_dir( build_cpu );
    target_multiarch = get_multiarch_dir( opts->target_cpu );
    build_len = strlen( build_multiarch );
    target_len = strlen( target_multiarch );

    for (i = 0; i < ARRAY_SIZE(stdlibpath); i++)
    {
        const char *root = (i && opts->sysroot) ? opts->sysroot : "";
        char *p, *buffer;

        if (!stdlibpath[i]) continue;
        buffer = xmalloc( strlen(root) + strlen(stdlibpath[i]) +
                          strlen("/arm-linux-gnueabi") + strlen(winecrt0) + 1 );
        strcpy( buffer, root );
        strcat( buffer, stdlibpath[i] );
        p = buffer + strlen(buffer);
        while (p > buffer && p[-1] == '/') p--;
        strcpy( p, winecrt0 );
        if (!stat( buffer, &st )) goto found;
        if (p > buffer + 2 && (!memcmp( p - 2, "32", 2 ) || !memcmp( p - 2, "64", 2 )))
        {
            p -= 2;
            strcpy( p, winecrt0 );
            if (!stat( buffer, &st )) goto found;
        }
        strcpy( p, bit_suffix );
        strcat( p, winecrt0 );
        if (!stat( buffer, &st )) goto found;
        strcpy( p, target_multiarch );
        strcat( p, winecrt0 );
        if (!stat( buffer, &st )) goto found;

        strcpy( buffer, root );
        strcat( buffer, stdlibpath[i] );
        p = buffer + strlen(buffer);
        while (p > buffer && p[-1] == '/') p--;
        strcpy( p, winecrt0 );

        /* try to fixup each parent dirs named lib, lib32 or lib64 with target bitness suffix */
        while (p > buffer)
        {
            p--;
            while (p > buffer && *p != '/') p--;
            if (*p != '/') break;

            /* try s/$build_cpu/$target_cpu/ on multiarch */
            if (build_cpu != opts->target_cpu && !memcmp( p, build_multiarch, build_len ) && p[build_len] == '/')
            {
                memmove( p + target_len, p + build_len, strlen( p + build_len ) + 1 );
                memcpy( p, target_multiarch, target_len );
                if (!stat( buffer, &st )) goto found;
                memmove( p + build_len, p + target_len, strlen( p + target_len ) + 1 );
                memcpy( p, build_multiarch, build_len );
            }

            if (memcmp( p + 1, "lib", 3 )) continue;
            if (p[4] == '/')
            {
                memmove( p + 6, p + 4, strlen( p + 4 ) + 1 );
                memcpy( p + 4, bit_suffix, 2 );
                if (!stat( buffer, &st )) goto found;
                memmove( p + 4, p + 6, strlen( p + 6 ) + 1 );
            }
            else if (!memcmp( p + 4, other_bit_suffix, 2 ) && p[6] == '/')
            {
                memcpy( p + 4, bit_suffix, 2 );
                if (!stat( buffer, &st )) goto found;
                memmove( p + 4, p + 6, strlen( p + 6 ) + 1 );
                if (!stat( buffer, &st )) goto found;
                memmove( p + 6, p + 4, strlen( p + 4 ) + 1 );
                memcpy( p + 4, other_bit_suffix, 2 );
            }
        }

        free( buffer );
        continue;

    found:
        buffer[strlen(buffer) - strlen(winecrt0)] = 0;
        return buffer;
    }
    return strmake( "%s%s", root, LIBDIR );
}

static void init_argv0_dir( const char *argv0 )
{
#ifndef _WIN32
    char *p, *dir;

#if defined(__linux__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__)
    dir = realpath( "/proc/self/exe", NULL );
#elif defined (__FreeBSD__) || defined(__DragonFly__)
    dir = realpath( "/proc/curproc/file", NULL );
#else
    dir = realpath( argv0, NULL );
#endif
    if (!dir) return;
    if (!(p = strrchr( dir, '/' ))) return;
    if (p == dir) p++;
    *p = 0;
    bindir = dir;
    includedir = strmake( "%s/%s", dir, BIN_TO_INCLUDEDIR );
    libdir = strmake( "%s/%s", dir, BIN_TO_LIBDIR );
#endif
}

static void compile(struct options* opts, const char* lang)
{
    strarray* comp_args = strarray_alloc();
    unsigned int i, j;
    int gcc_defs = 0;
    strarray* gcc;
    strarray* gpp;

    strarray_addall(comp_args, get_translator(opts));
    if (opts->force_pointer_size)
        strarray_add( comp_args, strmake("-m%u", 8 * opts->force_pointer_size ) );
    switch(opts->processor)
    {
	case proc_cpp: gcc_defs = 1; break;
	case proc_as:  gcc_defs = 0; break;
	/* Note: if the C compiler is gcc we assume the C++ compiler is too */
	/* mixing different C and C++ compilers isn't supported in configure anyway */
	case proc_cc:
	case proc_cxx:
            gcc = build_tool_name(opts, TOOL_CC);
            gpp = build_tool_name(opts, TOOL_CXX);
            for ( j = 0; !gcc_defs && j < comp_args->size; j++ )
            {
                const char *cc = comp_args->base[j];

                for (i = 0; !gcc_defs && i < gcc->size; i++)
                    gcc_defs = gcc->base[i][0] != '-' && strendswith(cc, gcc->base[i]);
                for (i = 0; !gcc_defs && i < gpp->size; i++)
                    gcc_defs = gpp->base[i][0] != '-' && strendswith(cc, gpp->base[i]);
            }
            strarray_free(gcc);
            strarray_free(gpp);
            break;
    }

    if (opts->target_platform == PLATFORM_WINDOWS || opts->target_platform == PLATFORM_CYGWIN || opts->target_platform == PLATFORM_MINGW)
        goto no_compat_defines;

    if (opts->processor != proc_cpp)
    {
	if (gcc_defs && !opts->wine_objdir && !opts->noshortwchar)
	{
            strarray_add(comp_args, "-fshort-wchar");
            strarray_add(comp_args, "-DWINE_UNICODE_NATIVE");
	}
        strarray_add(comp_args, "-D_REENTRANT");
        if (opts->pic)
            strarray_add(comp_args, "-fPIC");
        else
            strarray_add(comp_args, "-fno-PIC");
    }

    if (opts->target_cpu == CPU_x86_64 || opts->target_cpu == CPU_ARM64)
    {
        strarray_add(comp_args, "-DWIN64");
        strarray_add(comp_args, "-D_WIN64");
        strarray_add(comp_args, "-D__WIN64");
        strarray_add(comp_args, "-D__WIN64__");
    }

    strarray_add(comp_args, "-DWIN32");
    strarray_add(comp_args, "-D_WIN32");
    strarray_add(comp_args, "-D__WIN32");
    strarray_add(comp_args, "-D__WIN32__");
    strarray_add(comp_args, "-D__WINNT");
    strarray_add(comp_args, "-D__WINNT__");

    if (gcc_defs)
    {
        switch (opts->target_cpu)
        {
        case CPU_x86_64:
        case CPU_ARM64:
            strarray_add(comp_args, "-D__stdcall=__attribute__((ms_abi))");
            strarray_add(comp_args, "-D__cdecl=__stdcall");
            strarray_add(comp_args, "-D__fastcall=__stdcall");
            break;
        case CPU_x86:
            strarray_add(comp_args, "-D__stdcall=__attribute__((__stdcall__)) __attribute__((__force_align_arg_pointer__))");
            strarray_add(comp_args, "-D__cdecl=__attribute__((__cdecl__)) __attribute__((__force_align_arg_pointer__))");
            strarray_add(comp_args, "-D__fastcall=__attribute__((__fastcall__))");
            break;
        case CPU_ARM:
            strarray_add(comp_args, "-D__stdcall=__attribute__((pcs(\"aapcs-vfp\")))");
            strarray_add(comp_args, "-D__cdecl=__stdcall");
            strarray_add(comp_args, "-D__fastcall=__stdcall");
            break;
        case CPU_POWERPC:
            strarray_add(comp_args, "-D__stdcall=");
            strarray_add(comp_args, "-D__cdecl=");
            strarray_add(comp_args, "-D__fastcall=");
            break;
        }
        strarray_add(comp_args, "-D_stdcall=__stdcall");
        strarray_add(comp_args, "-D_cdecl=__cdecl");
        strarray_add(comp_args, "-D_fastcall=__fastcall");
	strarray_add(comp_args, "-D__declspec(x)=__declspec_##x");
	strarray_add(comp_args, "-D__declspec_align(x)=__attribute__((aligned(x)))");
	strarray_add(comp_args, "-D__declspec_allocate(x)=__attribute__((section(x)))");
	strarray_add(comp_args, "-D__declspec_deprecated=__attribute__((deprecated))");
	strarray_add(comp_args, "-D__declspec_dllimport=__attribute__((dllimport))");
	strarray_add(comp_args, "-D__declspec_dllexport=__attribute__((dllexport))");
	strarray_add(comp_args, "-D__declspec_naked=__attribute__((naked))");
	strarray_add(comp_args, "-D__declspec_noinline=__attribute__((noinline))");
	strarray_add(comp_args, "-D__declspec_noreturn=__attribute__((noreturn))");
	strarray_add(comp_args, "-D__declspec_nothrow=__attribute__((nothrow))");
	strarray_add(comp_args, "-D__declspec_novtable=__attribute__(())"); /* ignore it */
	strarray_add(comp_args, "-D__declspec_selectany=__attribute__((weak))");
	strarray_add(comp_args, "-D__declspec_thread=__thread");
    }

    strarray_add(comp_args, "-D__int8=char");
    strarray_add(comp_args, "-D__int16=short");
    strarray_add(comp_args, "-D__int32=int");
    if (opts->target_cpu == CPU_x86_64 || opts->target_cpu == CPU_ARM64)
        strarray_add(comp_args, "-D__int64=long");
    else
        strarray_add(comp_args, "-D__int64=long long");

no_compat_defines:
    strarray_add(comp_args, "-D__WINE__");

    /* options we handle explicitly */
    if (opts->compile_only)
	strarray_add(comp_args, "-c");
    if (opts->output_name)
    {
	strarray_add(comp_args, "-o");
	strarray_add(comp_args, opts->output_name);
    }

    /* the rest of the pass-through parameters */
    for ( j = 0 ; j < opts->compiler_args->size ; j++ ) 
        strarray_add(comp_args, opts->compiler_args->base[j]);

    /* the language option, if any */
    if (lang && strcmp(lang, "-xnone"))
	strarray_add(comp_args, lang);

    /* last, but not least, the files */
    for ( j = 0; j < opts->files->size; j++ )
    {
	if (opts->files->base[j][0] != '-' || !opts->files->base[j][1]) /* not an option or bare '-' (i.e. stdin) */
	    strarray_add(comp_args, opts->files->base[j]);
    }

    /* standard includes come last in the include search path */
    if (!opts->wine_objdir && !opts->nostdinc)
    {
        const char *incl_dirs[] = { INCLUDEDIR, "/usr/include", "/usr/local/include" };
        const char *root = opts->isysroot ? opts->isysroot : opts->sysroot ? opts->sysroot : "";
        const char *isystem = gcc_defs ? "-isystem" : "-I";
        const char *idirafter = gcc_defs ? "-idirafter" : "-I";

        if (opts->use_msvcrt)
        {
            if (includedir) strarray_add( comp_args, strmake( "%s%s/wine/msvcrt", isystem, includedir ));
            for (j = 0; j < ARRAY_SIZE(incl_dirs); j++)
            {
                if (j && !strcmp( incl_dirs[0], incl_dirs[j] )) continue;
                strarray_add(comp_args, strmake( "%s%s%s/wine/msvcrt", isystem, root, incl_dirs[j] ));
            }
            strarray_add(comp_args, "-D__MSVCRT__");
        }
        if (includedir)
        {
            strarray_add( comp_args, strmake( "%s%s/wine/windows", isystem, includedir ));
            strarray_add( comp_args, strmake( "%s%s", idirafter, includedir ));
        }
        for (j = 0; j < ARRAY_SIZE(incl_dirs); j++)
        {
            if (j && !strcmp( incl_dirs[0], incl_dirs[j] )) continue;
            strarray_add(comp_args, strmake( "%s%s%s/wine/windows", isystem, root, incl_dirs[j] ));
            strarray_add(comp_args, strmake( "%s%s%s", idirafter, root, incl_dirs[j] ));
        }
    }
    else if (opts->wine_objdir)
        strarray_add(comp_args, strmake("-I%s/include", opts->wine_objdir) );

    spawn(opts->prefix, comp_args, 0);
    strarray_free(comp_args);
}

static const char* compile_to_object(struct options* opts, const char* file, const char* lang)
{
    struct options copts;
    char* base_name;

    /* make a copy so we don't change any of the initial stuff */
    /* a shallow copy is exactly what we want in this case */
    base_name = get_basename(file);
    copts = *opts;
    copts.output_name = get_temp_file(base_name, ".o");
    copts.compile_only = 1;
    copts.files = strarray_alloc();
    strarray_add(copts.files, file);
    compile(&copts, lang);
    strarray_free(copts.files);
    free(base_name);

    return copts.output_name;
}

/* return the initial set of options needed to run winebuild */
static strarray *get_winebuild_args(struct options *opts)
{
    const char* winebuild = getenv("WINEBUILD");
    const char *binary = NULL;
    strarray *spec_args = strarray_alloc();
    unsigned int i;

    if (opts->winebuild)
        binary = opts->winebuild;
    else if (opts->wine_objdir)
        binary = strmake( "%s/tools/winebuild/winebuild%s", opts->wine_objdir, EXEEXT );
    else if (winebuild)
        binary = find_binary( opts->prefix, winebuild );
    else if (bindir)
        binary = strmake( "%s/winebuild%s", bindir, EXEEXT );
    else
        binary = find_binary( opts->prefix, "winebuild" );
    if (!binary) error( "Could not find winebuild\n" );
    strarray_add( spec_args, binary );
    if (verbose) strarray_add( spec_args, "-v" );
    if (keep_generated) strarray_add( spec_args, "--save-temps" );
    if (opts->target)
    {
        strarray_add( spec_args, "--target" );
        strarray_add( spec_args, opts->target );
    }
    if (opts->prefix)
    {
        for (i = 0; i < opts->prefix->size; i++)
            strarray_add( spec_args, strmake( "-B%s", opts->prefix->base[i] ));
    }
    strarray_addall( spec_args, opts->winebuild_args );
    if (opts->unwind_tables) strarray_add( spec_args, "-fasynchronous-unwind-tables" );
    else strarray_add( spec_args, "-fno-asynchronous-unwind-tables" );
    return spec_args;
}

static void fixup_constructors( struct options *opts, const char *file )
{
    strarray *args = get_winebuild_args( opts );

    strarray_add( args, "--fixup-ctors" );
    strarray_add( args, file );
    spawn( opts->prefix, args, 0 );
    strarray_free( args );
}

static void make_wine_builtin( struct options *opts, const char *file )
{
    strarray *args = get_winebuild_args( opts );

    strarray_add( args, "--builtin" );
    strarray_add( args, file );
    spawn( opts->prefix, args, 0 );
    strarray_free( args );
}

/* check if there is a static lib associated to a given dll */
static char *find_static_lib( const char *dll )
{
    char *lib = strmake("%s.a", dll);
    if (get_file_type(lib) == file_arh) return lib;
    free( lib );
    return NULL;
}

static const char *find_libgcc(const strarray *prefix, const strarray *link_tool)
{
    const char *out = get_temp_file( "find_libgcc", ".out" );
    const char *err = get_temp_file( "find_libgcc", ".err" );
    strarray *link = strarray_dup( link_tool );
    int sout = -1, serr = -1;
    char *libgcc, *p;
    struct stat st;
    size_t cnt;
    int ret;

    strarray_add( link, "-print-libgcc-file-name" );

    sout = dup( fileno(stdout) );
    freopen( out, "w", stdout );
    serr = dup( fileno(stderr) );
    freopen( err, "w", stderr );
    ret = spawn( prefix, link, 1 );
    if (sout >= 0)
    {
        dup2( sout, fileno(stdout) );
        close( sout );
    }
    if (serr >= 0)
    {
        dup2( serr, fileno(stderr) );
        close( serr );
    }
    strarray_free( link );

    if (ret || stat(out, &st) || !st.st_size) return NULL;

    libgcc = xmalloc(st.st_size + 1);
    sout = open(out, O_RDONLY);
    if (sout == -1) return NULL;
    cnt = read(sout, libgcc, st.st_size);
    close(sout);
    libgcc[cnt] = 0;
    if ((p = strchr(libgcc, '\n'))) *p = 0;
    return libgcc;
}


/* add specified library to the list of files */
static void add_library( struct options *opts, strarray *lib_dirs, strarray *files, const char *library )
{
    char *static_lib, *fullname = 0;

    switch(get_lib_type(opts->target_platform, lib_dirs, library, opts->lib_suffix, &fullname))
    {
    case file_arh:
        strarray_add(files, strmake("-a%s", fullname));
        break;
    case file_dll:
        strarray_add(files, strmake("-d%s", fullname));
        if ((static_lib = find_static_lib(fullname)))
        {
            strarray_add(files, strmake("-a%s",static_lib));
            free(static_lib);
        }
        break;
    case file_so:
    default:
        /* keep it anyway, the linker may know what to do with it */
        strarray_add(files, strmake("-l%s", library));
        break;
    }
    free(fullname);
}

static void build(struct options* opts)
{
    strarray *lib_dirs, *files;
    strarray *spec_args, *link_args, *implib_args, *tool;
    char *output_file, *output_path;
    const char *spec_o_name, *libgcc = NULL;
    const char *output_name, *spec_file, *lang;
    int generate_app_loader = 1;
    const char *crt_lib = NULL, *entry_point = NULL;
    int fake_module = 0;
    int is_pe = (opts->target_platform == PLATFORM_WINDOWS || opts->target_platform == PLATFORM_CYGWIN || opts->target_platform == PLATFORM_MINGW);
    unsigned int j;

    /* NOTE: for the files array we'll use the following convention:
     *    -axxx:  xxx is an archive (.a)
     *    -dxxx:  xxx is a DLL (.def)
     *    -lxxx:  xxx is an unsorted library
     *    -oxxx:  xxx is an object (.o)
     *    -rxxx:  xxx is a resource (.res)
     *    -sxxx:  xxx is a shared lib (.so)
     *    -xlll:  lll is the language (c, c++, etc.)
     */

    output_file = strdup( opts->output_name ? opts->output_name : "a.out" );

    /* 'winegcc -o app xxx.exe.so' only creates the load script */
    if (opts->files->size == 1 && strendswith(opts->files->base[0], ".exe.so"))
    {
	create_file(output_file, 0755, app_loader_template, opts->files->base[0]);
	return;
    }

    /* generate app loader only for .exe */
    if (opts->shared || is_pe || strendswith(output_file, ".so"))
	generate_app_loader = 0;

    if (strendswith(output_file, ".fake")) fake_module = 1;

    /* normalize the filename a bit: strip .so, ensure it has proper ext */
    if ((output_name = strrchr(output_file, '/'))) output_name++;
    else output_name = output_file;
    if (!strchr(output_name, '.'))
        output_file = strmake("%s.%s", output_file, opts->shared ? "dll" : "exe");
    else if (strendswith(output_file, ".so"))
	output_file[strlen(output_file) - 3] = 0;
    output_path = is_pe ? output_file : strmake( "%s.so", output_file );

    /* get the filename from the path */
    if ((output_name = strrchr(output_file, '/'))) output_name++;
    else output_name = output_file;

    /* prepare the linking path */
    if (!opts->wine_objdir)
    {
        char *lib_dir = get_lib_dir( opts );
        lib_dirs = strarray_dup(opts->lib_dirs);
        strarray_add( lib_dirs, strmake( "%s%s", lib_dir,
                                         get_wine_arch_dir( opts->target_cpu, opts->target_platform )));
        strarray_add( lib_dirs, lib_dir );
    }
    else
    {
        lib_dirs = strarray_alloc();
        strarray_add(lib_dirs, strmake("%s/dlls", opts->wine_objdir));
        strarray_addall(lib_dirs, opts->lib_dirs);
    }

    /* mark the files with their appropriate type */
    spec_file = lang = 0;
    files = strarray_alloc();
    for ( j = 0; j < opts->files->size; j++ )
    {
	const char* file = opts->files->base[j];
	if (file[0] != '-')
	{
	    switch(get_file_type(file))
	    {
		case file_def:
		case file_spec:
		    if (spec_file)
			error("Only one spec file can be specified\n");
		    spec_file = file;
		    break;
		case file_rc:
		    /* FIXME: invoke wrc to build it */
		    error("Can't compile .rc file at the moment: %s\n", file);
	            break;
	    	case file_res:
		    strarray_add(files, strmake("-r%s", file));
		    break;
		case file_obj:
		    strarray_add(files, strmake("-o%s", file));
		    break;
		case file_arh:
                    if (opts->use_msvcrt)
                    {
                        const char *p = strrchr(file, '/');
                        if (p) p++;
                        else p = file;
                        if (!strncmp(p, "libmsvcr", 8) || !strncmp(p, "libucrt", 7)) crt_lib = file;
                    }
                    strarray_add(files, strmake("-a%s", file));
                    break;
		case file_so:
		    strarray_add(files, strmake("-s%s", file));
		    break;
	    	case file_na:
		    error("File does not exist: %s\n", file);
		    break;
	        default:
		    file = compile_to_object(opts, file, lang);
		    strarray_add(files, strmake("-o%s", file));
		    break;
	    }
	}
	else if (file[1] == 'l')
            add_library(opts, lib_dirs, files, file + 2 );
	else if (file[1] == 'x')
	    lang = file;
        else if(file[1] == 'W')
            strarray_add(files, file);
    }

    /* add the default libraries, if needed */

    if (!opts->wine_objdir && !opts->nodefaultlibs)
    {
        if (opts->gui_app)
	{
	    add_library(opts, lib_dirs, files, "shell32");
	    add_library(opts, lib_dirs, files, "comdlg32");
	    add_library(opts, lib_dirs, files, "gdi32");
	}
        add_library(opts, lib_dirs, files, "advapi32");
        add_library(opts, lib_dirs, files, "user32");
    }

    if (!opts->nodefaultlibs && !opts->unix_lib)
    {
        add_library(opts, lib_dirs, files, "winecrt0");
        if (opts->use_msvcrt)
        {
            if (!crt_lib)
            {
                if (strncmp( output_name, "msvcr", 5 ) &&
                    strncmp( output_name, "ucrt", 4 ) &&
                    strcmp( output_name, "crtdll.dll" ))
                    add_library(opts, lib_dirs, files, "ucrtbase");
            }
            else strarray_add(files, strmake("-a%s", crt_lib));
        }
        if (opts->win16_app) add_library(opts, lib_dirs, files, "kernel");
        add_library(opts, lib_dirs, files, "kernel32");
        add_library(opts, lib_dirs, files, "ntdll");
    }

    /* set default entry point, if needed */
    if (!opts->entry_point)
    {
        if (opts->subsystem && !strcmp( opts->subsystem, "native" ))
            entry_point = (is_pe && opts->target_cpu == CPU_x86) ? "DriverEntry@8" : "DriverEntry";
        else if (opts->use_msvcrt && !opts->shared && !opts->win16_app)
            entry_point = opts->unicode_app ? "wmainCRTStartup" : "mainCRTStartup";
    }
    else entry_point = opts->entry_point;

    /* run winebuild to generate the .spec.o file */
    spec_args = get_winebuild_args( opts );
    if ((tool = build_tool_name( opts, TOOL_CC ))) strarray_add( spec_args, strmake( "--cc-cmd=%s", strarray_tostring( tool, " " )));
    if (!is_pe && (tool = build_tool_name( opts, TOOL_LD ))) strarray_add( spec_args, strmake( "--ld-cmd=%s", strarray_tostring( tool, " " )));

    spec_o_name = get_temp_file(output_name, ".spec.o");
    if (opts->force_pointer_size)
        strarray_add(spec_args, strmake("-m%u", 8 * opts->force_pointer_size ));
    strarray_add(spec_args, "-D_REENTRANT");
    if (opts->pic && !is_pe) strarray_add(spec_args, "-fPIC");
    strarray_add(spec_args, opts->shared ? "--dll" : "--exe");
    if (fake_module)
    {
        strarray_add(spec_args, "--fake-module");
        strarray_add(spec_args, "-o");
        strarray_add(spec_args, output_file);
    }
    else
    {
        strarray_add(spec_args, "-o");
        strarray_add(spec_args, spec_o_name);
    }
    if (spec_file)
    {
        strarray_add(spec_args, "-E");
        strarray_add(spec_args, spec_file);
    }

    if (!opts->shared)
    {
        strarray_add(spec_args, "-F");
        strarray_add(spec_args, output_name);
        strarray_add(spec_args, "--subsystem");
        strarray_add(spec_args, opts->gui_app ? "windows" : "console");
        if (opts->large_address_aware) strarray_add( spec_args, "--large-address-aware" );
    }

    if (opts->target_platform == PLATFORM_WINDOWS) strarray_add(spec_args, "--safeseh");

    if (entry_point)
    {
        strarray_add(spec_args, "--entry");
        strarray_add(spec_args, entry_point);
    }

    if (opts->subsystem)
    {
        strarray_add(spec_args, "--subsystem");
        strarray_add(spec_args, opts->subsystem);
    }

    for ( j = 0; j < lib_dirs->size; j++ )
	strarray_add(spec_args, strmake("-L%s", lib_dirs->base[j]));

    if (!is_pe)
    {
        for (j = 0; j < opts->delayimports->size; j++)
            strarray_add(spec_args, strmake("-d%s", opts->delayimports->base[j]));
    }

    /* add resource files */
    for ( j = 0; j < files->size; j++ )
	if (files->base[j][1] == 'r') strarray_add(spec_args, files->base[j]);

    /* add other files */
    strarray_add(spec_args, "--");
    for ( j = 0; j < files->size; j++ )
    {
	switch(files->base[j][1])
	{
	    case 'd':
	    case 'a':
	    case 'o':
		strarray_add(spec_args, files->base[j] + 2);
		break;
	}
    }

    spawn(opts->prefix, spec_args, 0);
    strarray_free (spec_args);
    if (fake_module) return;  /* nothing else to do */

    /* link everything together now */
    link_args = get_link_args( opts, output_name );

    if ((opts->nodefaultlibs || opts->use_msvcrt) && opts->target_platform == PLATFORM_MINGW)
    {
        libgcc = find_libgcc(opts->prefix, link_args);
        if (!libgcc) libgcc = "-lgcc";
    }

    strarray_add(link_args, "-o");
    strarray_add(link_args, output_path);

    for ( j = 0; j < lib_dirs->size; j++ )
	strarray_add(link_args, strmake("-L%s", lib_dirs->base[j]));

    if (is_pe && opts->use_msvcrt && !entry_point && (opts->shared || opts->win16_app))
        entry_point = opts->target_cpu == CPU_x86 ? "DllMainCRTStartup@12" : "DllMainCRTStartup";

    if (is_pe && entry_point)
    {
        if (opts->target_platform == PLATFORM_WINDOWS)
            strarray_add(link_args, strmake("-Wl,-entry:%s", entry_point));
        else
            strarray_add(link_args, strmake("-Wl,--entry,%s%s",
                                            is_pe && opts->target_cpu == CPU_x86 ? "_" : "",
                                            entry_point));
    }

    strarray_add(link_args, spec_o_name);

    if (is_pe)
    {
        for (j = 0; j < opts->delayimports->size; j++)
        {
            if (opts->target_platform == PLATFORM_WINDOWS)
                strarray_add(link_args, strmake("-Wl,-delayload:%s", opts->delayimports->base[j]));
            else
                strarray_add(link_args, strmake("-Wl,-delayload,%s",opts->delayimports->base[j]));
        }
    }

    for ( j = 0; j < files->size; j++ )
    {
	const char* name = files->base[j] + 2;
	switch(files->base[j][1])
	{
	    case 'l':
		strarray_add(link_args, strmake("-l%s", name));
		break;
	    case 's':
	    case 'o':
		strarray_add(link_args, name);
		break;
	    case 'a':
                if (is_pe && !opts->use_msvcrt && !opts->lib_suffix && strchr(name, '/'))
                {
                    /* turn the path back into -Ldir -lfoo options
                     * this makes sure that we use the specified libs even
                     * when mingw adds its own import libs to the link */
                    char *lib = xstrdup( name );
                    char *p = strrchr( lib, '/' );

                    *p++ = 0;
                    if (!strncmp( p, "lib", 3 ) && strcmp( p, "libmsvcrt.a" ))
                    {
                        char *ext = strrchr( p, '.' );

                        if (ext) *ext = 0;
                        p += 3;
                        strarray_add(link_args, strmake("-L%s", lib ));
                        strarray_add(link_args, strmake("-l%s", p ));
                        free( lib );
                        break;
                    }
                    free( lib );
                }
		strarray_add(link_args, name);
		break;
	    case 'W':
		strarray_add(link_args, files->base[j]);
		break;
	}
    }

    if (!opts->nostdlib && !is_pe)
    {
	strarray_add(link_args, "-lm");
	strarray_add(link_args, "-lc");
    }

    if (libgcc) strarray_add(link_args, libgcc);

    output_file_name = output_path;
    output_debug_file = opts->debug_file;
    output_implib = opts->out_implib;
    atexit( cleanup_output_files );

    spawn(opts->prefix, link_args, 0);
    strarray_free (link_args);

    if (opts->debug_file && !strendswith(opts->debug_file, ".pdb"))
    {
        strarray *tool, *objcopy = build_tool_name(opts, TOOL_OBJCOPY);

        tool = strarray_dup(objcopy);
        strarray_add(tool, "--only-keep-debug");
        strarray_add(tool, output_path);
        strarray_add(tool, opts->debug_file);
        spawn(opts->prefix, tool, 1);
        strarray_free(tool);

        tool = strarray_dup(objcopy);
        strarray_add(tool, "--strip-debug");
        strarray_add(tool, output_path);
        spawn(opts->prefix, tool, 1);
        strarray_free(tool);

        tool = objcopy;
        strarray_add(tool, "--add-gnu-debuglink");
        strarray_add(tool, opts->debug_file);
        strarray_add(tool, output_path);
        spawn(opts->prefix, tool, 0);
        strarray_free(tool);
    }

    if (opts->out_implib && !is_pe)
    {
        if (!spec_file)
            error("--out-implib requires a .spec or .def file\n");

        implib_args = get_winebuild_args( opts );
        if ((tool = build_tool_name( opts, TOOL_CC ))) strarray_add( implib_args, strmake( "--cc-cmd=%s", strarray_tostring( tool, " " )));
        if ((tool = build_tool_name( opts, TOOL_LD ))) strarray_add( implib_args, strmake( "--ld-cmd=%s", strarray_tostring( tool, " " )));

        strarray_add(implib_args, "--implib");
        strarray_add(implib_args, "-o");
        strarray_add(implib_args, opts->out_implib);
        strarray_add(implib_args, "--export");
        strarray_add(implib_args, spec_file);

        spawn(opts->prefix, implib_args, 0);
        strarray_free (implib_args);
    }

    /* set the base address with prelink if linker support is not present */
    if (opts->prelink && !opts->target)
    {
        if (opts->prelink[0] && strcmp(opts->prelink,"false"))
        {
            strarray *prelink_args = strarray_alloc();
            strarray_add(prelink_args, opts->prelink);
            strarray_add(prelink_args, "--reloc-only");
            strarray_add(prelink_args, opts->image_base);
            strarray_add(prelink_args, output_path);
            spawn(opts->prefix, prelink_args, 1);
            strarray_free(prelink_args);
        }
    }

    if (!is_pe) fixup_constructors( opts, output_path );
    else if (opts->wine_builtin) make_wine_builtin( opts, output_path );

    /* create the loader script */
    if (generate_app_loader)
        create_file(output_file, 0755, app_loader_template, strmake("%s.so", output_name));
}


static void forward( struct options *opts )
{
    strarray* args = strarray_alloc();

    strarray_addall(args, get_translator(opts));
    strarray_addall(args, opts->compiler_args);
    strarray_addall(args, opts->linker_args);
    spawn(opts->prefix, args, 0);
    strarray_free (args);
}

static int is_linker_arg(const char* arg)
{
    static const char* link_switches[] = 
    {
	"-nostdlib", "-s", "-static", "-static-libgcc", "-static-libstdc++",
	"-shared", "-shared-libgcc", "-symbolic", "-framework", "--coverage",
	"-fprofile-generate", "-fprofile-use"
    };
    unsigned int j;

    switch (arg[1]) 
    {
	case 'R':
	case 'z':
	case 'u':
	    return 1;
        case 'W':
            if (strncmp("-Wl,", arg, 4) == 0) return 1;
	    break;
	case 'X':
	    if (strcmp("-Xlinker", arg) == 0) return 1;
	    break;
	case 'a':
	    if (strcmp("-arch", arg) == 0) return 1;
	    break;
	case 'f':
	    if (strncmp("-fuse-ld=", arg, 9) == 0) return 1;
	    break;
        case 'r':
            if (strncmp("-rtlib=", arg, 7) == 0) return 1;
            break;
    }

    for (j = 0; j < ARRAY_SIZE(link_switches); j++)
	if (strcmp(link_switches[j], arg) == 0) return 1;

    return 0;
}

static void parse_target_option( struct options *opts, const char *target )
{
    char *p, *spec = xstrdup( target );
    unsigned int i;

    /* target specification is in the form CPU-MANUFACTURER-OS or CPU-MANUFACTURER-KERNEL-OS */

    /* get the CPU part */

    if ((p = strchr( spec, '-' )))
    {
        *p++ = 0;
        for (i = 0; i < ARRAY_SIZE(cpu_names); i++)
        {
            if (!strcmp( cpu_names[i].name, spec ))
            {
                opts->target_cpu = cpu_names[i].cpu;
                break;
            }
        }
        if (i == ARRAY_SIZE(cpu_names))
            error( "Unrecognized CPU '%s'\n", spec );
    }
    else if (!strcmp( spec, "mingw32" ))
    {
        opts->target_cpu = CPU_x86;
        p = spec;
    }
    else
        error( "Invalid target specification '%s'\n", target );

    /* get the OS part */

    opts->target_platform = PLATFORM_UNSPECIFIED;  /* default value */
    for (;;)
    {
        for (i = 0; i < ARRAY_SIZE(platform_names); i++)
        {
            if (!strncmp( platform_names[i].name, p, strlen(platform_names[i].name) ))
            {
                opts->target_platform = platform_names[i].platform;
                break;
            }
        }
        if (opts->target_platform != PLATFORM_UNSPECIFIED || !(p = strchr( p, '-' ))) break;
        p++;
    }

    free( spec );
    opts->target = xstrdup( target );
}

static int is_option( struct options *opts, int i, const char *option, const char **option_arg )
{
    if (!strcmp( opts->args->base[i], option ))
    {
        if (opts->args->size == i) error( "option %s requires an argument\n", opts->args->base[i] );
        *option_arg = opts->args->base[i + 1];
        return 1;
    }
    if (!strncmp( opts->args->base[i], option, strlen(option) ) && opts->args->base[i][strlen(option)] == '=')
    {
        *option_arg = opts->args->base[i] + strlen(option) + 1;
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int i, c, next_is_arg = 0, linking = 1;
    int raw_compiler_arg, raw_linker_arg, raw_winebuild_arg;
    const char* option_arg;
    struct options opts;
    char* lang = 0;
    char* str;

#ifdef SIGHUP
    signal( SIGHUP, exit_on_signal );
#endif
    signal( SIGTERM, exit_on_signal );
    signal( SIGINT, exit_on_signal );
#ifdef HAVE_SIGADDSET
    sigemptyset( &signal_mask );
    sigaddset( &signal_mask, SIGHUP );
    sigaddset( &signal_mask, SIGTERM );
    sigaddset( &signal_mask, SIGINT );
#endif
    init_argv0_dir( argv[0] );

    /* setup tmp file removal at exit */
    tmp_files = strarray_alloc();
    atexit(clean_temp_files);

    /* initialize options */
    memset(&opts, 0, sizeof(opts));
    opts.target_cpu = build_cpu;
    opts.target_platform = build_platform;
    opts.lib_dirs = strarray_alloc();
    opts.files = strarray_alloc();
    opts.linker_args = strarray_alloc();
    opts.compiler_args = strarray_alloc();
    opts.winebuild_args = strarray_alloc();
    opts.delayimports = strarray_alloc();
    opts.args = strarray_alloc();
    opts.pic = 1;

    /* determine the processor type */
    if (strendswith(argv[0], "winecpp")) opts.processor = proc_cpp;
    else if (strendswith(argv[0], "++")) opts.processor = proc_cxx;
    
    for (i = 1; i < argc; i++)
    {
        char *input_buffer = NULL, *iter, *opt, *out;
        struct stat st;
        int fd;

        if (argv[i][0] != '@' || (fd = open( argv[i] + 1, O_RDONLY | O_BINARY )) == -1)
        {
            strarray_add( opts.args, argv[i] );
            continue;
        }
        if ((fstat( fd, &st ) == -1)) error( "Cannot stat %s\n", argv[i] + 1 );
        if (st.st_size)
        {
            input_buffer = xmalloc( st.st_size + 1 );
            if (read( fd, input_buffer, st.st_size ) != st.st_size) error( "Cannot read %s\n", argv[i] + 1 );
        }
        close( fd );
        for (iter = input_buffer; iter < input_buffer + st.st_size; iter++)
        {
            char quote = 0;
            while (iter < input_buffer + st.st_size && isspace(*iter)) iter++;
            if (iter == input_buffer + st.st_size) break;
            opt = out = iter;
            while (iter < input_buffer + st.st_size && (quote || !isspace(*iter)))
            {
                if (*iter == quote)
                {
                    iter++;
                    quote = 0;
                }
                else if (*iter == '\'' || *iter == '"') quote = *iter++;
                else
                {
                    if (*iter == '\\' && iter + 1 < input_buffer + st.st_size) iter++;
                    *out++ = *iter++;
                }
            }
            *out = 0;
            strarray_add( opts.args, opt );
        }
    }

    /* parse options */
    for (i = 0; i < opts.args->size; i++)
    {
        if (opts.args->base[i][0] == '-' && opts.args->base[i][1])  /* option, except '-' alone is stdin, which is a file */
	{
	    /* determine if this switch is followed by a separate argument */
	    next_is_arg = 0;
	    option_arg = 0;
	    switch(opts.args->base[i][1])
	    {
		case 'x': case 'o': case 'D': case 'U':
		case 'I': case 'A': case 'l': case 'u':
		case 'b': case 'V': case 'G': case 'L':
		case 'B': case 'R': case 'z':
		    if (opts.args->base[i][2]) option_arg = &opts.args->base[i][2];
		    else next_is_arg = 1;
		    break;
		case 'i':
		    next_is_arg = 1;
		    break;
		case 'a':
		    if (strcmp("-aux-info", opts.args->base[i]) == 0)
			next_is_arg = 1;
		    if (strcmp("-arch", opts.args->base[i]) == 0)
			next_is_arg = 1;
		    break;
		case 'X':
		    if (strcmp("-Xlinker", opts.args->base[i]) == 0)
			next_is_arg = 1;
		    break;
		case 'M':
		    c = opts.args->base[i][2];
		    if (c == 'F' || c == 'T' || c == 'Q')
		    {
			if (opts.args->base[i][3]) option_arg = &opts.args->base[i][3];
			else next_is_arg = 1;
		    }
		    break;
		case 'f':
		    if (strcmp("-framework", opts.args->base[i]) == 0)
			next_is_arg = 1;
		    break;
                case 't':
                    next_is_arg = strcmp("-target", opts.args->base[i]) == 0;
                    break;
		case '-':
		    next_is_arg = (strcmp("--param", opts.args->base[i]) == 0 ||
                                   strcmp("--sysroot", opts.args->base[i]) == 0 ||
                                   strcmp("--target", opts.args->base[i]) == 0 ||
                                   strcmp("--wine-objdir", opts.args->base[i]) == 0 ||
                                   strcmp("--winebuild", opts.args->base[i]) == 0 ||
                                   strcmp("--lib-suffix", opts.args->base[i]) == 0);
		    break;
	    }
	    if (next_is_arg)
            {
                if (i + 1 >= opts.args->size) error("option -%c requires an argument\n", opts.args->base[i][1]);
                option_arg = opts.args->base[i+1];
            }

	    /* determine what options go 'as is' to the linker & the compiler */
	    raw_linker_arg = is_linker_arg(opts.args->base[i]);
	    raw_compiler_arg = !raw_linker_arg;
            raw_winebuild_arg = 0;

	    /* do a bit of semantic analysis */
            switch (opts.args->base[i][1])
	    {
		case 'B':
		    str = strdup(option_arg);
		    if (strendswith(str, "/")) str[strlen(str) - 1] = 0;
                    if (!opts.prefix) opts.prefix = strarray_alloc();
                    strarray_add(opts.prefix, str);
                    raw_linker_arg = 1;
		    break;
                case 'b':
                    parse_target_option( &opts, option_arg );
                    raw_compiler_arg = 0;
                    break;
                case 'V':
                    opts.version = xstrdup( option_arg );
                    raw_compiler_arg = 0;
                    break;
                case 'c':        /* compile or assemble */
                    raw_compiler_arg = 0;
		    if (opts.args->base[i][2] == 0) opts.compile_only = 1;
		    /* fall through */
                case 'S':        /* generate assembler code */
                case 'E':        /* preprocess only */
                    if (opts.args->base[i][2] == 0) linking = 0;
                    break;
		case 'f':
		    if (strcmp("-fno-short-wchar", opts.args->base[i]) == 0)
                        opts.noshortwchar = 1;
		    else if (!strcmp("-fasynchronous-unwind-tables", opts.args->base[i]))
                        opts.unwind_tables = 1;
		    else if (!strcmp("-fno-asynchronous-unwind-tables", opts.args->base[i]))
                        opts.unwind_tables = 0;
                    else if (!strcmp("-fPIC", opts.args->base[i]) || !strcmp("-fpic", opts.args->base[i]))
                        opts.pic = 1;
                    else if (!strcmp("-fno-PIC", opts.args->base[i]) || !strcmp("-fno-pic", opts.args->base[i]))
                        opts.pic = 0;
		    break;
                case 'i':
                    if (!strcmp( "-isysroot", opts.args->base[i] )) opts.isysroot = opts.args->base[i + 1];
                    break;
		case 'l':
		    strarray_add(opts.files, strmake("-l%s", option_arg));
                    raw_compiler_arg = 0;
		    break;
		case 'L':
		    strarray_add(opts.lib_dirs, option_arg);
                    raw_compiler_arg = 0;
		    break;
                case 'M':        /* map file generation */
                    linking = 0;
                    break;
		case 'm':
		    if (strcmp("-mno-cygwin", opts.args->base[i]) == 0)
                    {
			opts.use_msvcrt = 1;
                        raw_compiler_arg = 0;
                        raw_winebuild_arg = 1;
                    }
		    else if (strcmp("-mwindows", opts.args->base[i]) == 0)
                    {
			opts.gui_app = 1;
                        raw_compiler_arg = 0;
                    }
		    else if (strcmp("-mconsole", opts.args->base[i]) == 0)
                    {
			opts.gui_app = 0;
                        raw_compiler_arg = 0;
                    }
		    else if (strcmp("-municode", opts.args->base[i]) == 0)
                    {
			opts.unicode_app = 1;
                        raw_compiler_arg = 0;
                        raw_winebuild_arg = 1;
                    }
		    else if (strcmp("-mthreads", opts.args->base[i]) == 0)
                    {
                        raw_compiler_arg = 0;
                    }
		    else if (strcmp("-munix", opts.args->base[i]) == 0)
                    {
			opts.unix_lib = 1;
                        raw_compiler_arg = 0;
                        raw_winebuild_arg = 1;
                    }
		    else if (strcmp("-m16", opts.args->base[i]) == 0)
                    {
			opts.win16_app = 1;
                        raw_compiler_arg = 0;
                        raw_winebuild_arg = 1;
                    }
		    else if (strcmp("-m32", opts.args->base[i]) == 0)
                    {
                        if (opts.target_cpu == CPU_x86_64)
                            opts.target_cpu = CPU_x86;
                        else if (opts.target_cpu == CPU_ARM64)
                            opts.target_cpu = CPU_ARM;
                        opts.force_pointer_size = 4;
			raw_linker_arg = 1;
                    }
		    else if (strcmp("-m64", opts.args->base[i]) == 0)
                    {
                        if (opts.target_cpu == CPU_x86)
                            opts.target_cpu = CPU_x86_64;
                        else if (opts.target_cpu == CPU_ARM)
                            opts.target_cpu = CPU_ARM64;
                        opts.force_pointer_size = 8;
			raw_linker_arg = 1;
                    }
                    else if (!strcmp("-marm", opts.args->base[i] ) || !strcmp("-mthumb", opts.args->base[i] ))
                    {
			raw_linker_arg = 1;
                        raw_winebuild_arg = 1;
                    }
                    else if (!strncmp("-mcpu=", opts.args->base[i], 6) ||
                             !strncmp("-mfpu=", opts.args->base[i], 6) ||
                             !strncmp("-march=", opts.args->base[i], 7) ||
                             !strncmp("-mfloat-abi=", opts.args->base[i], 12))
                        raw_winebuild_arg = 1;
		    break;
                case 'n':
                    if (strcmp("-nostdinc", opts.args->base[i]) == 0)
                        opts.nostdinc = 1;
                    else if (strcmp("-nodefaultlibs", opts.args->base[i]) == 0)
                        opts.nodefaultlibs = 1;
                    else if (strcmp("-nostdlib", opts.args->base[i]) == 0)
                        opts.nostdlib = 1;
                    else if (strcmp("-nostartfiles", opts.args->base[i]) == 0)
                        opts.nostartfiles = 1;
                    break;
		case 'o':
		    opts.output_name = option_arg;
                    raw_compiler_arg = 0;
		    break;
                case 's':
                    if (strcmp("-static", opts.args->base[i]) == 0)
			linking = -1;
		    else if(strcmp("-save-temps", opts.args->base[i]) == 0)
			keep_generated = 1;
                    else if (strncmp("-specs=", opts.args->base[i], 7) == 0)
                        raw_linker_arg = 1;
		    else if(strcmp("-shared", opts.args->base[i]) == 0)
		    {
			opts.shared = 1;
                        raw_compiler_arg = raw_linker_arg = 0;
		    }
                    else if (strcmp("-s", opts.args->base[i]) == 0 && opts.target_platform == PLATFORM_APPLE)
                    {
                        /* On Mac, change -s into -Wl,-x. ld's -s switch
                         * is deprecated, and it doesn't work on Tiger with
                         * MH_BUNDLEs anyway
                         */
                        opts.strip = 1;
                        raw_linker_arg = 0;
                    }
                    break;
                case 't':
                    if (is_option( &opts, i, "-target", &option_arg ))
                    {
                        parse_target_option( &opts, option_arg );
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
                    break;
                case 'v':
                    if (opts.args->base[i][2] == 0) verbose++;
                    break;
                case 'W':
                    if (strncmp("-Wl,", opts.args->base[i], 4) == 0)
		    {
                        unsigned int j;
                        strarray* Wl = strarray_fromstring(opts.args->base[i] + 4, ",");
                        for (j = 0; j < Wl->size; j++)
                        {
                            if (!strcmp(Wl->base[j], "--image-base") && j < Wl->size - 1)
                            {
                                opts.image_base = strdup( Wl->base[++j] );
                                continue;
                            }
                            if (!strcmp(Wl->base[j], "--section-alignment") && j < Wl->size - 1)
                            {
                                opts.section_align = strdup( Wl->base[++j] );
                                continue;
                            }
                            if (!strcmp(Wl->base[j], "--file-alignment") && j < Wl->size - 1)
                            {
                                opts.file_align = strdup( Wl->base[++j] );
                                continue;
                            }
                            if (!strcmp(Wl->base[j], "--large-address-aware"))
                            {
                                opts.large_address_aware = 1;
                                continue;
                            }
                            if (!strcmp(Wl->base[j], "--wine-builtin"))
                            {
                                opts.wine_builtin = 1;
                                continue;
                            }
                            if (!strcmp(Wl->base[j], "--subsystem") && j < Wl->size - 1)
                            {
                                opts.subsystem = strdup( Wl->base[++j] );
                                continue;
                            }
                            if (!strcmp(Wl->base[j], "--entry") && j < Wl->size - 1)
                            {
                                opts.entry_point = strdup( Wl->base[++j] );
                                continue;
                            }
                            if (!strcmp(Wl->base[j], "-delayload") && j < Wl->size - 1)
                            {
                                strarray_add( opts.delayimports, Wl->base[++j] );
                                continue;
                            }
                            if (!strcmp(Wl->base[j], "--debug-file") && j < Wl->size - 1)
                            {
                                opts.debug_file = strdup( Wl->base[++j] );
                                continue;
                            }
                            if (!strcmp(Wl->base[j], "--whole-archive") || !strcmp(Wl->base[j], "--no-whole-archive"))
                            {
                                strarray_add( opts.files, strmake( "-Wl,%s", Wl->base[j] ));
                                continue;
                            }
                            if (!strcmp(Wl->base[j], "--out-implib"))
                            {
                                opts.out_implib = strdup( Wl->base[++j] );
                                continue;
                            }
                            if (!strcmp(Wl->base[j], "-static")) linking = -1;
                            strarray_add(opts.linker_args, strmake("-Wl,%s",Wl->base[j]));
                        }
                        strarray_free(Wl);
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
		    else if (strncmp("-Wb,", opts.args->base[i], 4) == 0)
		    {
			strarray* Wb = strarray_fromstring(opts.args->base[i] + 4, ",");
			strarray_addall(opts.winebuild_args, Wb);
			strarray_free(Wb);
                        /* don't pass it to the compiler, it generates errors */
                        raw_compiler_arg = raw_linker_arg = 0;
		    }
                    break;
		case 'x':
		    lang = strmake("-x%s", option_arg);
		    strarray_add(opts.files, lang);
		    /* we'll pass these flags ourselves, explicitly */
                    raw_compiler_arg = raw_linker_arg = 0;
		    break;
                case '-':
                    if (strcmp("-static", opts.args->base[i]+1) == 0)
                        linking = -1;
                    else if (is_option( &opts, i, "--sysroot", &option_arg ))
                    {
                        opts.sysroot = option_arg;
                        raw_linker_arg = 1;
                    }
                    else if (is_option( &opts, i, "--target", &option_arg ))
                    {
                        parse_target_option( &opts, option_arg );
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
                    else if (is_option( &opts, i, "--wine-objdir", &option_arg ))
                    {
                        opts.wine_objdir = option_arg;
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
                    else if (is_option( &opts, i, "--winebuild", &option_arg ))
                    {
                        opts.winebuild = option_arg;
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
                    else if (is_option( &opts, i, "--lib-suffix", &option_arg ))
                    {
                        opts.lib_suffix = option_arg;
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
                    break;
            }

	    /* put the arg into the appropriate bucket */
	    if (raw_linker_arg) 
	    {
		strarray_add( opts.linker_args, opts.args->base[i] );
		if (next_is_arg && (i + 1 < opts.args->size))
		    strarray_add( opts.linker_args, opts.args->base[i + 1] );
	    }
	    if (raw_compiler_arg)
	    {
		strarray_add( opts.compiler_args, opts.args->base[i] );
		if (next_is_arg && (i + 1 < opts.args->size))
		    strarray_add( opts.compiler_args, opts.args->base[i + 1] );
	    }
            if (raw_winebuild_arg)
            {
                strarray_add( opts.winebuild_args, opts.args->base[i] );
		if (next_is_arg && (i + 1 < opts.args->size))
		    strarray_add( opts.winebuild_args, opts.args->base[i + 1] );
            }

	    /* skip the next token if it's an argument */
	    if (next_is_arg) i++;
        }
	else
	{
	    strarray_add( opts.files, opts.args->base[i] );
	} 
    }

    if (opts.processor == proc_cpp) linking = 0;
    if (linking == -1) error("Static linking is not supported\n");

    if (opts.files->size == 0) forward(&opts);
    else if (linking) build(&opts);
    else compile(&opts, lang);

    output_file_name = NULL;
    output_debug_file = NULL;
    output_implib = NULL;
    return 0;
}
