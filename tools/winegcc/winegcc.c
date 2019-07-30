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
 *          -Xlinker option -u symbol --image-base
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

static int keep_generated = 0;
static strarray* tmp_files;
#ifdef HAVE_SIGSET_T
static sigset_t signal_mask;
#endif

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
    { "macos",   PLATFORM_APPLE },
    { "darwin",  PLATFORM_APPLE },
    { "android", PLATFORM_ANDROID },
    { "solaris", PLATFORM_SOLARIS },
    { "cygwin",  PLATFORM_CYGWIN },
    { "mingw32", PLATFORM_WINDOWS },
    { "windows", PLATFORM_WINDOWS },
    { "winnt",   PLATFORM_WINDOWS }
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
    const char* output_name;
    const char* image_base;
    const char* section_align;
    const char* lib_suffix;
    const char* subsystem;
    const char* prelink;
    strarray* prefix;
    strarray* lib_dirs;
    strarray* linker_args;
    strarray* compiler_args;
    strarray* winebuild_args;
    strarray* files;
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
static enum target_platform build_platform = PLATFORM_WINDOWS;
#else
static enum target_platform build_platform = PLATFORM_UNSPECIFIED;
#endif

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

static char* build_tool_name(struct options *opts, const char* base, const char* deflt)
{
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
        str = xstrdup(deflt);
    return str;
}

static const strarray* get_translator(struct options *opts)
{
    char *str = NULL;
    strarray *ret;

    switch(opts->processor)
    {
    case proc_cpp:
        str = build_tool_name(opts, "cpp", CPP);
        break;
    case proc_cc:
    case proc_as:
        str = build_tool_name(opts, "gcc", CC);
        break;
    case proc_cxx:
        str = build_tool_name(opts, "g++", CXX);
        break;
    default:
        assert(0);
    }
    ret = strarray_fromstring( str, " " );
    free(str);
    if (opts->force_pointer_size)
        strarray_add( ret, strmake("-m%u", 8 * opts->force_pointer_size ));
    return ret;
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
    const strarray *link_tool = get_translator( opts );
    strarray *flags = strarray_alloc();
    unsigned int i;

    strarray_addall( flags, link_tool );
    for (i = 0; i < opts->linker_args->size; i++) strarray_add( flags, opts->linker_args->base[i] );

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
        return flags;

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

    case PLATFORM_WINDOWS:
    case PLATFORM_CYGWIN:
        if (opts->shared || opts->win16_app)
        {
            strarray_add( flags, "-shared" );
            strarray_add( flags, "-Wl,--kill-at" );
        }
        else strarray_add( flags, opts->gui_app ? "-mwindows" : "-mconsole" );

        if (opts->unicode_app) strarray_add( flags, "-municode" );
        if (opts->nodefaultlibs) strarray_add( flags, "-nodefaultlibs" );
        if (opts->nostartfiles) strarray_add( flags, "-nostartfiles" );

        if (opts->subsystem)
        {
            strarray_add( flags, strmake("-Wl,--subsystem,%s", opts->subsystem ));
            if (!strcmp( opts->subsystem, "native" ))
            {
                const char *entry = opts->target_cpu == CPU_x86 ? "_DriverEntry@8" : "DriverEntry";
                strarray_add( flags, strmake( "-Wl,--entry,%s", entry ));
            }
        }

        strarray_add( flags, "-Wl,--nxcompat" );

        if (opts->image_base) strarray_add( flags, strmake("-Wl,--image-base,%s", opts->image_base ));

        if (opts->large_address_aware && opts->target_cpu == CPU_x86)
            strarray_add( flags, "-Wl,--large-address-aware" );

        /* make sure we don't need a libgcc_s dll on Windows */
        strarray_add( flags, "-static-libgcc" );

        return flags;

    default:
        if (opts->image_base)
        {
            if (!try_link( opts->prefix, link_tool, strmake("-Wl,-Ttext-segment=%s", opts->image_base)) )
                strarray_add( flags, strmake("-Wl,-Ttext-segment=%s", opts->image_base) );
            else
                opts->prelink = PRELINK;
        }
        if (!try_link( opts->prefix, link_tool, "-Wl,-z,max-page-size=0x1000"))
            strarray_add( flags, "-Wl,-z,max-page-size=0x1000");
        break;
    }

    /* generic Unix shared library flags */

    strarray_add( flags, "-shared" );
    strarray_add( flags, "-Wl,-Bsymbolic" );
    if (!opts->noshortwchar && opts->target_cpu == CPU_ARM)
        strarray_add( flags, "-Wl,--no-wchar-size-warning" );

    /* Try all options first - this is likely to succeed on modern compilers */
    if (!try_link( opts->prefix, link_tool, "-fPIC -shared -Wl,-Bsymbolic "
                   "-Wl,-z,defs -Wl,-init,__wine_spec_init,-fini,_wine_spec_fini" ))
    {
        strarray_add( flags, "-Wl,-z,defs" );
        strarray_add( flags, "-Wl,-init,__wine_spec_init,-fini,__wine_spec_fini" );
    }
    else /* otherwise figure out which ones are allowed */
    {
        if (!try_link( opts->prefix, link_tool, "-fPIC -shared -Wl,-Bsymbolic -Wl,-z,defs" ))
            strarray_add( flags, "-Wl,-z,defs" );
        if (!try_link( opts->prefix, link_tool, "-fPIC -shared -Wl,-Bsymbolic "
                       "-Wl,-init,__wine_spec_init,-fini,_wine_spec_fini" ))
            strarray_add( flags, "-Wl,-init,__wine_spec_init,-fini,__wine_spec_fini" );
    }

    return flags;
}

/* check that file is a library for the correct platform */
static int check_platform( struct options *opts, const char *file )
{
    int ret = 0, fd = open( file, O_RDONLY );
    if (fd != -1)
    {
        unsigned char header[16];
        if (read( fd, header, sizeof(header) ) == sizeof(header))
        {
            /* FIXME: only ELF is supported, platform is not checked beyond 32/64 */
            if (!memcmp( header, "\177ELF", 4 ))
            {
                if (header[4] == 2)  /* 64-bit */
                    ret = (opts->target_cpu == CPU_x86_64 || opts->target_cpu == CPU_ARM64);
                else
                    ret = (opts->target_cpu != CPU_x86_64 && opts->target_cpu != CPU_ARM64);
            }
        }
        close( fd );
    }
    return ret;
}

static void make_wine_builtin( const char *file )
{
    static const char wine_magic[32] = "Wine builtin DLL";
    int fd;
    struct
    {
        unsigned short e_magic;
        unsigned short unused[29];
        unsigned int   e_lfanew;
    } header;

    if ((fd = open( file, O_RDWR | O_BINARY )) == -1) error( "Failed to add signature to %s\n", file );

    if (read( fd, &header, sizeof(header) ) == sizeof(header) && !memcmp( &header.e_magic, "MZ", 2 ))
    {
        if (header.e_lfanew < sizeof(header) + sizeof(wine_magic))
            error( "Not enough space (%x) for Wine signature\n", header.e_lfanew );
        write( fd, wine_magic, sizeof(wine_magic) );
    }
    close( fd );
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

static char *get_lib_dir( struct options *opts )
{
    static const char *stdlibpath[] = { LIBDIR, "/usr/lib", "/usr/local/lib", "/lib" };
    static const char libwine[] = "/libwine.so";
    const char *bit_suffix, *other_bit_suffix, *build_multiarch, *target_multiarch;
    unsigned int i;
    size_t build_len, target_len;

    bit_suffix = opts->target_cpu == CPU_x86_64 || opts->target_cpu == CPU_ARM64 ? "64" : "32";
    other_bit_suffix = opts->target_cpu == CPU_x86_64 || opts->target_cpu == CPU_ARM64 ? "32" : "64";
    build_multiarch = get_multiarch_dir( build_cpu );
    target_multiarch = get_multiarch_dir( opts->target_cpu );
    build_len = strlen( build_multiarch );
    target_len = strlen( target_multiarch );

    for (i = 0; i < sizeof(stdlibpath)/sizeof(stdlibpath[0]); i++)
    {
        char *p, *buffer = xmalloc( strlen(stdlibpath[i]) + strlen("/arm-linux-gnueabi") + strlen(libwine) + 1 );
        strcpy( buffer, stdlibpath[i] );
        p = buffer + strlen(buffer);
        while (p > buffer && p[-1] == '/') p--;
        strcpy( p, libwine );
        if (check_platform( opts, buffer )) goto found;
        if (p > buffer + 2 && (!memcmp( p - 2, "32", 2 ) || !memcmp( p - 2, "64", 2 )))
        {
            p -= 2;
            strcpy( p, libwine );
            if (check_platform( opts, buffer )) goto found;
        }
        strcpy( p, bit_suffix );
        strcat( p, libwine );
        if (check_platform( opts, buffer )) goto found;
        strcpy( p, target_multiarch );
        strcat( p, libwine );
        if (check_platform( opts, buffer )) goto found;

        strcpy( buffer, stdlibpath[i] );
        p = buffer + strlen(buffer);
        while (p > buffer && p[-1] == '/') p--;
        strcpy( p, libwine );

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
                if (check_platform( opts, buffer )) goto found;
                memmove( p + build_len, p + target_len, strlen( p + target_len ) + 1 );
                memcpy( p, build_multiarch, build_len );
            }

            if (memcmp( p + 1, "lib", 3 )) continue;
            if (p[4] == '/')
            {
                memmove( p + 6, p + 4, strlen( p + 4 ) + 1 );
                memcpy( p + 4, bit_suffix, 2 );
                if (check_platform( opts, buffer )) goto found;
                memmove( p + 4, p + 6, strlen( p + 6 ) + 1 );
            }
            else if (!memcmp( p + 4, other_bit_suffix, 2 ) && p[6] == '/')
            {
                memcpy( p + 4, bit_suffix, 2 );
                if (check_platform( opts, buffer )) goto found;
                memmove( p + 4, p + 6, strlen( p + 6 ) + 1 );
                if (check_platform( opts, buffer )) goto found;
                memmove( p + 6, p + 4, strlen( p + 4 ) + 1 );
                memcpy( p + 4, other_bit_suffix, 2 );
            }
        }

        free( buffer );
        continue;

    found:
        buffer[strlen(buffer) - strlen(libwine)] = 0;
        return buffer;
    }
    return xstrdup( LIBDIR );
}

static void compile(struct options* opts, const char* lang)
{
    strarray* comp_args = strarray_alloc();
    unsigned int i, j;
    int gcc_defs = 0;
    strarray* gcc;
    strarray* gpp;

    strarray_addall(comp_args, get_translator(opts));
    switch(opts->processor)
    {
	case proc_cpp: gcc_defs = 1; break;
	case proc_as:  gcc_defs = 0; break;
	/* Note: if the C compiler is gcc we assume the C++ compiler is too */
	/* mixing different C and C++ compilers isn't supported in configure anyway */
	case proc_cc:
	case proc_cxx:
            gcc = strarray_fromstring(build_tool_name(opts, "gcc", CC), " ");
            gpp = strarray_fromstring(build_tool_name(opts, "g++", CXX), " ");
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

    if (opts->target_platform == PLATFORM_WINDOWS || opts->target_platform == PLATFORM_CYGWIN)
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
            strarray_add(comp_args, "-D__stdcall=__attribute__((ms_abi))");
            strarray_add(comp_args, "-D__cdecl=__attribute__((ms_abi))");
            strarray_add(comp_args, "-D_stdcall=__attribute__((ms_abi))");
            strarray_add(comp_args, "-D_cdecl=__attribute__((ms_abi))");
            strarray_add(comp_args, "-D__fastcall=__attribute__((ms_abi))");
            strarray_add(comp_args, "-D_fastcall=__attribute__((ms_abi))");
            break;
        case CPU_x86:
            strarray_add(comp_args, "-D__stdcall=__attribute__((__stdcall__)) __attribute__((__force_align_arg_pointer__))");
            strarray_add(comp_args, "-D__cdecl=__attribute__((__cdecl__)) __attribute__((__force_align_arg_pointer__))");
            strarray_add(comp_args, "-D_stdcall=__attribute__((__stdcall__)) __attribute__((__force_align_arg_pointer__))");
            strarray_add(comp_args, "-D_cdecl=__attribute__((__cdecl__)) __attribute__((__force_align_arg_pointer__))");
            strarray_add(comp_args, "-D__fastcall=__attribute__((__fastcall__))");
            strarray_add(comp_args, "-D_fastcall=__attribute__((__fastcall__))");
            break;
        case CPU_ARM:
        case CPU_ARM64:
        case CPU_POWERPC:
            strarray_add(comp_args, "-D__stdcall=");
            strarray_add(comp_args, "-D__cdecl=");
            strarray_add(comp_args, "-D_stdcall=");
            strarray_add(comp_args, "-D_cdecl=");
            strarray_add(comp_args, "-D__fastcall=");
            strarray_add(comp_args, "-D_fastcall=");
            break;
        }
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
	if (opts->files->base[j][0] != '-')
	    strarray_add(comp_args, opts->files->base[j]);
    }

    /* standard includes come last in the include search path */
    if (!opts->wine_objdir && !opts->nostdinc)
    {
        if (opts->use_msvcrt)
        {
            strarray_add(comp_args, gcc_defs ? "-isystem" INCLUDEDIR "/wine/msvcrt" : "-I" INCLUDEDIR "/wine/msvcrt" );
            strarray_add(comp_args, "-D__MSVCRT__");
        }
        strarray_add(comp_args, "-I" INCLUDEDIR );
        strarray_add(comp_args, gcc_defs ? "-isystem" INCLUDEDIR "/wine/windows" : "-I" INCLUDEDIR "/wine/windows" );
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
    strarray *spec_args = strarray_alloc();

    if (!winebuild) winebuild = "winebuild";
    strarray_add( spec_args, winebuild );
    if (verbose) strarray_add( spec_args, "-v" );
    if (keep_generated) strarray_add( spec_args, "--save-temps" );
    if (opts->target)
    {
        strarray_add( spec_args, "--target" );
        strarray_add( spec_args, opts->target );
    }
    if (!opts->use_msvcrt) strarray_add( spec_args, "-munix" );
    if (opts->unwind_tables) strarray_add( spec_args, "-fasynchronous-unwind-tables" );
    else strarray_add( spec_args, "-fno-asynchronous-unwind-tables" );
    return spec_args;
}

/* check if there is a static lib associated to a given dll */
static char *find_static_lib( const char *dll )
{
    char *lib = strmake("%s.a", dll);
    if (get_file_type(lib) == file_arh) return lib;
    free( lib );
    return NULL;
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
    strarray *spec_args, *link_args;
    char *output_file, *output_path;
    const char *spec_o_name;
    const char *output_name, *spec_file, *lang;
    int generate_app_loader = 1;
    int fake_module = 0;
    int is_pe = (opts->target_platform == PLATFORM_WINDOWS || opts->target_platform == PLATFORM_CYGWIN);
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
    if (strendswith(output_file, ".so")) 
	output_file[strlen(output_file) - 3] = 0;
    if ((output_name = strrchr(output_file, '/'))) output_name++;
    else output_name = output_file;
    if (!strchr(output_name, '.'))
        output_file = strmake("%s.%s", output_file, opts->shared ? "dll" : "exe");
    output_path = is_pe ? output_file : strmake( "%s.so", output_file );

    /* get the filename from the path */
    if ((output_name = strrchr(output_file, '/'))) output_name++;
    else output_name = output_file;

    /* prepare the linking path */
    if (!opts->wine_objdir)
    {
        char *lib_dir = get_lib_dir( opts );
        lib_dirs = strarray_dup(opts->lib_dirs);
        strarray_add( lib_dirs, strmake( "%s/wine", lib_dir ));
        strarray_add( lib_dirs, lib_dir );
    }
    else
    {
        lib_dirs = strarray_alloc();
        strarray_add(lib_dirs, strmake("%s/dlls", opts->wine_objdir));
        strarray_add(lib_dirs, strmake("%s/libs/wine", opts->wine_objdir));
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

    if (!opts->nodefaultlibs)
    {
        if (opts->use_msvcrt) add_library(opts, lib_dirs, files, "msvcrt");
        add_library(opts, lib_dirs, files, "winecrt0");
        if (opts->win16_app) add_library(opts, lib_dirs, files, "kernel");
        add_library(opts, lib_dirs, files, "kernel32");
        add_library(opts, lib_dirs, files, "ntdll");
    }
    if (!opts->nostdlib && !is_pe) add_library(opts, lib_dirs, files, "wine");

    /* run winebuild to generate the .spec.o file */
    spec_args = get_winebuild_args( opts );
    strarray_add( spec_args, strmake( "--cc-cmd=%s", build_tool_name( opts, "gcc", CC )));
    strarray_add( spec_args, strmake( "--ld-cmd=%s", build_tool_name( opts, "ld", LD )));

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
    if (opts->win16_app) strarray_add(spec_args, "-m16");

    if (!opts->shared)
    {
        strarray_add(spec_args, "-F");
        strarray_add(spec_args, output_name);
        strarray_add(spec_args, "--subsystem");
        strarray_add(spec_args, opts->gui_app ? "windows" : "console");
        if (opts->unicode_app)
        {
            strarray_add(spec_args, "--entry");
            strarray_add(spec_args, "__wine_spec_exe_wentry");
        }
        if (opts->large_address_aware) strarray_add( spec_args, "--large-address-aware" );
    }

    if (opts->subsystem)
    {
        strarray_add(spec_args, "--subsystem");
        strarray_add(spec_args, opts->subsystem);
    }

    for ( j = 0; j < lib_dirs->size; j++ )
	strarray_add(spec_args, strmake("-L%s", lib_dirs->base[j]));

    for ( j = 0 ; j < opts->winebuild_args->size ; j++ )
        strarray_add(spec_args, opts->winebuild_args->base[j]);

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

    strarray_add(link_args, "-o");
    strarray_add(link_args, output_path);

    for ( j = 0; j < lib_dirs->size; j++ )
	strarray_add(link_args, strmake("-L%s", lib_dirs->base[j]));

    strarray_add(link_args, spec_o_name);

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
                if (is_pe && !opts->lib_suffix && strchr(name, '/'))
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
	}
    }

    if (!opts->nostdlib && !is_pe)
    {
	strarray_add(link_args, "-lm");
	strarray_add(link_args, "-lc");
    }

    if (opts->nodefaultlibs && is_pe) strarray_add( link_args, "-lgcc" );

    spawn(opts->prefix, link_args, 0);
    strarray_free (link_args);

    if (is_pe && opts->wine_builtin) make_wine_builtin( output_path );

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

    /* create the loader script */
    if (generate_app_loader)
        create_file(output_file, 0755, app_loader_template, strmake("%s.so", output_name));
}


static void forward(int argc, char **argv, struct options* opts)
{
    strarray* args = strarray_alloc();
    int j;

    strarray_addall(args, get_translator(opts));

    for( j = 1; j < argc; j++ ) 
	strarray_add(args, argv[j]);

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
	case 'l':
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
    }

    for (j = 0; j < sizeof(link_switches)/sizeof(link_switches[0]); j++)
	if (strcmp(link_switches[j], arg) == 0) return 1;

    return 0;
}

/*
 *      Target Options
 *          -b machine  -V version
 */
static int is_target_arg(const char* arg)
{
    return arg[1] == 'b' || arg[1] == 'V';
}


/*
 *      Directory Options
 *          -Bprefix  -Idir  -I-  -Ldir  -specs=file
 */
static int is_directory_arg(const char* arg)
{
    return arg[1] == 'B' || arg[1] == 'L' || arg[1] == 'I' || strncmp("-specs=", arg, 7) == 0;
}

/*
 *      MinGW Options
 *	    -mno-cygwin -mwindows -mconsole -mthreads -municode
 */ 
static int is_mingw_arg(const char* arg)
{
    static const char* mingw_switches[] = 
    {
        "-mno-cygwin", "-mwindows", "-mconsole", "-mthreads", "-municode"
    };
    unsigned int j;

    for (j = 0; j < sizeof(mingw_switches)/sizeof(mingw_switches[0]); j++)
	if (strcmp(mingw_switches[j], arg) == 0) return 1;

    return 0;
}

static void parse_target_option( struct options *opts, const char *target )
{
    char *p, *platform, *spec = xstrdup( target );
    unsigned int i;

    /* target specification is in the form CPU-MANUFACTURER-OS or CPU-MANUFACTURER-KERNEL-OS */

    /* get the CPU part */

    if ((p = strchr( spec, '-' )))
    {
        *p++ = 0;
        for (i = 0; i < sizeof(cpu_names)/sizeof(cpu_names[0]); i++)
        {
            if (!strcmp( cpu_names[i].name, spec ))
            {
                opts->target_cpu = cpu_names[i].cpu;
                break;
            }
        }
        if (i == sizeof(cpu_names)/sizeof(cpu_names[0]))
            error( "Unrecognized CPU '%s'\n", spec );
        platform = p;
        if ((p = strrchr( p, '-' ))) platform = p + 1;
    }
    else if (!strcmp( spec, "mingw32" ))
    {
        opts->target_cpu = CPU_x86;
        platform = spec;
    }
    else
        error( "Invalid target specification '%s'\n", target );

    /* get the OS part */

    opts->target_platform = PLATFORM_UNSPECIFIED;  /* default value */
    for (i = 0; i < sizeof(platform_names)/sizeof(platform_names[0]); i++)
    {
        if (!strncmp( platform_names[i].name, platform, strlen(platform_names[i].name) ))
        {
            opts->target_platform = platform_names[i].platform;
            break;
        }
    }

    free( spec );
    opts->target = xstrdup( target );
}

int main(int argc, char **argv)
{
    int i, c, next_is_arg = 0, linking = 1;
    int raw_compiler_arg, raw_linker_arg;
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
    opts.pic = 1;

    /* determine the processor type */
    if (strendswith(argv[0], "winecpp")) opts.processor = proc_cpp;
    else if (strendswith(argv[0], "++")) opts.processor = proc_cxx;
    
    /* parse options */
    for ( i = 1 ; i < argc ; i++ ) 
    {
        if (argv[i][0] == '-')  /* option */
	{
	    /* determine if this switch is followed by a separate argument */
	    next_is_arg = 0;
	    option_arg = 0;
	    switch(argv[i][1])
	    {
		case 'x': case 'o': case 'D': case 'U':
		case 'I': case 'A': case 'l': case 'u':
		case 'b': case 'V': case 'G': case 'L':
		case 'B': case 'R': case 'z':
		    if (argv[i][2]) option_arg = &argv[i][2];
		    else next_is_arg = 1;
		    break;
		case 'i':
		    next_is_arg = 1;
		    break;
		case 'a':
		    if (strcmp("-aux-info", argv[i]) == 0)
			next_is_arg = 1;
		    if (strcmp("-arch", argv[i]) == 0)
			next_is_arg = 1;
		    break;
		case 'X':
		    if (strcmp("-Xlinker", argv[i]) == 0)
			next_is_arg = 1;
		    break;
		case 'M':
		    c = argv[i][2];
		    if (c == 'F' || c == 'T' || c == 'Q')
		    {
			if (argv[i][3]) option_arg = &argv[i][3];
			else next_is_arg = 1;
		    }
		    break;
		case 'f':
		    if (strcmp("-framework", argv[i]) == 0)
			next_is_arg = 1;
		    break;
		case '-':
		    if (strcmp("--param", argv[i]) == 0)
			next_is_arg = 1;
		    break;
	    }
	    if (next_is_arg)
            {
                if (i + 1 >= argc) error("option -%c requires an argument\n", argv[i][1]);
                option_arg = argv[i+1];
            }

	    /* determine what options go 'as is' to the linker & the compiler */
	    raw_compiler_arg = raw_linker_arg = 0;
	    if (is_linker_arg(argv[i])) 
	    {
		raw_linker_arg = 1;
	    }
	    else 
	    {
		if (is_directory_arg(argv[i]) || is_target_arg(argv[i]))
		    raw_linker_arg = 1;
		raw_compiler_arg = !is_mingw_arg(argv[i]);
	    }

	    /* these things we handle explicitly so we don't pass them 'as is' */
	    if (argv[i][1] == 'l' || argv[i][1] == 'I' || argv[i][1] == 'L')
		raw_linker_arg = 0;
	    if (argv[i][1] == 'c' || argv[i][1] == 'L')
		raw_compiler_arg = 0;
	    if (argv[i][1] == 'o' || argv[i][1] == 'b' || argv[i][1] == 'V')
		raw_compiler_arg = raw_linker_arg = 0;

	    /* do a bit of semantic analysis */
            switch (argv[i][1]) 
	    {
		case 'B':
		    str = strdup(option_arg);
		    if (strendswith(str, "/")) str[strlen(str) - 1] = 0;
		    if (strendswith(str, "/tools/winebuild"))
                    {
                        char *objdir = strdup(str);
                        objdir[strlen(objdir) - sizeof("/tools/winebuild") + 1] = 0;
                        opts.wine_objdir = objdir;
                        /* don't pass it to the compiler, this generates warnings */
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
                    else if (!strcmp(str, "tools/winebuild"))
                    {
                        opts.wine_objdir = ".";
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
                    if (!opts.prefix) opts.prefix = strarray_alloc();
                    strarray_add(opts.prefix, str);
		    break;
                case 'b':
                    parse_target_option( &opts, option_arg );
                    break;
                case 'V':
                    opts.version = xstrdup( option_arg );
                    break;
                case 'c':        /* compile or assemble */
		    if (argv[i][2] == 0) opts.compile_only = 1;
		    /* fall through */
                case 'S':        /* generate assembler code */
                case 'E':        /* preprocess only */
                    if (argv[i][2] == 0) linking = 0;
                    break;
		case 'f':
		    if (strcmp("-fno-short-wchar", argv[i]) == 0)
                        opts.noshortwchar = 1;
		    else if (!strcmp("-fasynchronous-unwind-tables", argv[i]))
                        opts.unwind_tables = 1;
		    else if (!strcmp("-fno-asynchronous-unwind-tables", argv[i]))
                        opts.unwind_tables = 0;
                    else if (!strcmp("-fPIC", argv[i]) || !strcmp("-fpic", argv[i]))
                        opts.pic = 1;
                    else if (!strcmp("-fno-PIC", argv[i]) || !strcmp("-fno-pic", argv[i]))
                        opts.pic = 0;
		    break;
		case 'l':
		    strarray_add(opts.files, strmake("-l%s", option_arg));
		    break;
		case 'L':
		    strarray_add(opts.lib_dirs, option_arg);
		    break;
                case 'M':        /* map file generation */
                    linking = 0;
                    break;
		case 'm':
		    if (strcmp("-mno-cygwin", argv[i]) == 0)
			opts.use_msvcrt = 1;
		    else if (strcmp("-mwindows", argv[i]) == 0)
			opts.gui_app = 1;
		    else if (strcmp("-mconsole", argv[i]) == 0)
			opts.gui_app = 0;
		    else if (strcmp("-municode", argv[i]) == 0)
			opts.unicode_app = 1;
		    else if (strcmp("-m16", argv[i]) == 0)
			opts.win16_app = 1;
		    else if (strcmp("-m32", argv[i]) == 0)
                    {
                        if (opts.target_cpu == CPU_x86_64)
                            opts.target_cpu = CPU_x86;
                        else if (opts.target_cpu == CPU_ARM64)
                            opts.target_cpu = CPU_ARM;
                        opts.force_pointer_size = 4;
			raw_linker_arg = 1;
                    }
		    else if (strcmp("-m64", argv[i]) == 0)
                    {
                        if (opts.target_cpu == CPU_x86)
                            opts.target_cpu = CPU_x86_64;
                        else if (opts.target_cpu == CPU_ARM)
                            opts.target_cpu = CPU_ARM64;
                        opts.force_pointer_size = 8;
			raw_linker_arg = 1;
                    }
                    else if (!strcmp("-marm", argv[i] ) || !strcmp("-mthumb", argv[i] ))
                    {
                        strarray_add(opts.winebuild_args, argv[i]);
			raw_linker_arg = 1;
                    }
                    else if (!strncmp("-mcpu=", argv[i], 6) ||
                             !strncmp("-mfpu=", argv[i], 6) ||
                             !strncmp("-march=", argv[i], 7) ||
                             !strncmp("-mfloat-abi=", argv[i], 12))
                        strarray_add(opts.winebuild_args, argv[i]);
		    break;
                case 'n':
                    if (strcmp("-nostdinc", argv[i]) == 0)
                        opts.nostdinc = 1;
                    else if (strcmp("-nodefaultlibs", argv[i]) == 0)
                        opts.nodefaultlibs = 1;
                    else if (strcmp("-nostdlib", argv[i]) == 0)
                        opts.nostdlib = 1;
                    else if (strcmp("-nostartfiles", argv[i]) == 0)
                        opts.nostartfiles = 1;
                    break;
		case 'o':
		    opts.output_name = option_arg;
		    break;
                case 's':
                    if (strcmp("-static", argv[i]) == 0) 
			linking = -1;
		    else if(strcmp("-save-temps", argv[i]) == 0)
			keep_generated = 1;
		    else if(strcmp("-shared", argv[i]) == 0)
		    {
			opts.shared = 1;
                        raw_compiler_arg = raw_linker_arg = 0;
		    }
                    else if (strcmp("-s", argv[i]) == 0 && opts.target_platform == PLATFORM_APPLE)
                    {
                        /* On Mac, change -s into -Wl,-x. ld's -s switch
                         * is deprecated, and it doesn't work on Tiger with
                         * MH_BUNDLEs anyway
                         */
                        opts.strip = 1;
                        raw_linker_arg = 0;
                    }
                    break;
                case 'v':
                    if (argv[i][2] == 0) verbose++;
                    break;
                case 'W':
                    if (strncmp("-Wl,", argv[i], 4) == 0)
		    {
                        unsigned int j;
                        strarray* Wl = strarray_fromstring(argv[i] + 4, ",");
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
                            if (!strcmp(Wl->base[j], "-static")) linking = -1;
                            strarray_add(opts.linker_args, strmake("-Wl,%s",Wl->base[j]));
                        }
                        strarray_free(Wl);
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
		    else if (strncmp("-Wb,", argv[i], 4) == 0)
		    {
			strarray* Wb = strarray_fromstring(argv[i] + 4, ",");
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
                    if (strcmp("-static", argv[i]+1) == 0)
                        linking = -1;
                    else if (!strncmp("--sysroot", argv[i], 9) && opts.wine_objdir)
                    {
                        if (argv[i][9] == '=') opts.wine_objdir = argv[i] + 10;
                        else opts.wine_objdir = argv[++i];
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
                    else if (!strncmp("--lib-suffix", argv[i], 12) && opts.wine_objdir)
                    {
                        if (argv[i][12] == '=') opts.lib_suffix = argv[i] + 13;
                        else opts.lib_suffix = argv[++i];
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
                    break;
            }

	    /* put the arg into the appropriate bucket */
	    if (raw_linker_arg) 
	    {
		strarray_add(opts.linker_args, argv[i]);
		if (next_is_arg && (i + 1 < argc)) 
		    strarray_add(opts.linker_args, argv[i + 1]);
	    }
	    if (raw_compiler_arg)
	    {
		strarray_add(opts.compiler_args, argv[i]);
		if (next_is_arg && (i + 1 < argc))
		    strarray_add(opts.compiler_args, argv[i + 1]);
	    }

	    /* skip the next token if it's an argument */
	    if (next_is_arg) i++;
        }
	else
	{
	    strarray_add(opts.files, argv[i]);
	} 
    }

    if (opts.processor == proc_cpp) linking = 0;
    if (linking == -1) error("Static linking is not supported\n");

    if (opts.files->size == 0) forward(argc, argv, &opts);
    else if (linking) build(&opts);
    else compile(&opts, lang);

    return 0;
}
