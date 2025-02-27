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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <sys/types.h>

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
const char *temp_dir = NULL;
struct strarray temp_files = { 0 };

static const char *bindir;
static const char *libdir;
static const char *includedir;

enum processor { proc_cc, proc_cxx, proc_cpp, proc_as };

struct options
{
    enum processor processor;
    struct target target;
    const char *target_alias;
    const char *version;
    int shared;
    int use_msvcrt;
    int nostdinc;
    int nostdlib;
    int nostartfiles;
    int nodefaultlibs;
    int noshortwchar;
    int data_only;
    int gui_app;
    int unicode_app;
    int win16_app;
    int compile_only;
    int force_pointer_size;
    int large_address_aware;
    int wine_builtin;
    int fake_module;
    int unwind_tables;
    int strip;
    int pic;
    int no_default_config;
    int build_id;
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
    const char* debug_file;
    const char* out_implib;
    const char* native_arch;
    struct strarray prefix;
    struct strarray lib_dirs;
    struct strarray args;
    struct strarray linker_args;
    struct strarray compiler_args;
    struct strarray winebuild_args;
    struct strarray files;
    struct strarray delayimports;
};

static void cleanup_output_files(void)
{
    if (output_file_name) unlink( output_file_name );
    if (output_debug_file) unlink( output_debug_file );
    if (output_implib) unlink( output_implib );
}

static void clean_temp_files(void)
{
    if (!keep_generated) remove_temp_files();
}

/* clean things up when aborting on a signal */
static void exit_on_signal( int sig )
{
    exit(1);  /* this will call the atexit functions */
}

static int is_pe_target( const struct options *opts )
{
    switch (opts->target.platform)
    {
    case PLATFORM_MINGW:
    case PLATFORM_CYGWIN:
    case PLATFORM_WINDOWS:
        return 1;
    default:
        return 0;
    }
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

static struct strarray build_tool_name( struct options *opts, const char *target, enum tool tool )
{
    const char *base = tool_names[tool].base;
    const char *llvm_base = tool_names[tool].llvm_base;
    const char *deflt = tool_names[tool].deflt;
    const char *path;
    struct strarray ret = empty_strarray;
    char* str;

    if (target && opts->version)
    {
        str = strmake( "%s-%s-%s", target, base, opts->version );
    }
    else if (target)
    {
        str = strmake( "%s-%s", target, base );
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
        return ret;
    }

    ret = strarray_fromstring( path, " " );
    if (!strncmp( llvm_base, "clang", 5 ))
    {
        if (target)
        {
            strarray_add( &ret, "-target" );
            strarray_add( &ret, target );
        }
        strarray_add( &ret, "-Wno-unused-command-line-argument" );
        strarray_add( &ret, "-fuse-ld=lld" );
        if (opts->no_default_config) strarray_add( &ret, "--no-default-config" );
    }
    return ret;
}

static struct strarray get_translator(struct options *opts)
{
    switch(opts->processor)
    {
    case proc_cpp:
        return build_tool_name( opts, opts->target_alias, TOOL_CPP );
    case proc_cc:
    case proc_as:
        return build_tool_name( opts, opts->target_alias, TOOL_CC );
    case proc_cxx:
        return build_tool_name( opts, opts->target_alias, TOOL_CXX );
    }
    assert(0);
    return empty_strarray;
}

static int try_link( struct strarray prefix, struct strarray link_tool, const char *cflags )
{
    const char *in = make_temp_file( "try_link", ".c" );
    const char *out = make_temp_file( "try_link", ".out" );
    const char *err = make_temp_file( "try_link", ".err" );
    struct strarray link = empty_strarray;
    int sout = -1, serr = -1;
    int ret;

    create_file( in, 0644, "int main(void){return 1;}\n" );

    strarray_addall( &link, link_tool );
    strarray_add( &link, "-o" );
    strarray_add( &link, out );
    strarray_addall( &link, strarray_fromstring( cflags, " " ) );
    strarray_add( &link, in );

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
    return ret;
}

static struct strarray get_link_args( struct options *opts, const char *output_name )
{
    struct strarray link_args = get_translator( opts );
    struct strarray flags = empty_strarray;

    strarray_addall( &link_args, opts->linker_args );

    if (verbose > 1) strarray_add( &flags, "-v" );

    switch (opts->target.platform)
    {
    case PLATFORM_APPLE:
        strarray_add( &flags, "-bundle" );
        strarray_add( &flags, "-multiply_defined" );
        strarray_add( &flags, "suppress" );
        if (opts->image_base)
        {
            strarray_add( &flags, "-image_base" );
            strarray_add( &flags, opts->image_base );
        }
        if (opts->strip) strarray_add( &flags, "-Wl,-x" );
        strarray_addall( &link_args, flags );
        return link_args;

    case PLATFORM_SOLARIS:
        {
            char *mapfile = make_temp_file( output_name, ".map" );
            const char *align = opts->section_align ? opts->section_align : "0x1000";

            create_file( mapfile, 0644, "text = A%s;\ndata = A%s;\n", align, align );
            strarray_add( &flags, strmake("-Wl,-M,%s", mapfile) );
        }
        break;

    case PLATFORM_ANDROID:
        /* the Android loader requires a soname for all libraries */
        strarray_add( &flags, strmake( "-Wl,-soname,%s.so", output_name ));
        break;

    case PLATFORM_MINGW:
    case PLATFORM_CYGWIN:
        if (opts->shared || opts->win16_app)
        {
            strarray_add( &flags, "-shared" );
            strarray_add( &flags, "-Wl,--kill-at" );
        }
        else strarray_add( &flags, opts->gui_app ? "-mwindows" : "-mconsole" );

        if (opts->unicode_app) strarray_add( &flags, "-municode" );
        if (opts->nodefaultlibs || opts->use_msvcrt) strarray_add( &flags, "-nodefaultlibs" );
        if (opts->nostartfiles || opts->use_msvcrt) strarray_add( &flags, "-nostartfiles" );
        if (opts->subsystem) strarray_add( &flags, strmake("-Wl,--subsystem,%s", opts->subsystem ));

        strarray_add( &flags, "-Wl,--exclude-all-symbols" );
        strarray_add( &flags, "-Wl,--nxcompat" );
        strarray_add( &flags, "-Wl,--dynamicbase" );
        strarray_add( &flags, "-Wl,--disable-auto-image-base" );

        if (opts->image_base) strarray_add( &flags, strmake("-Wl,--image-base,%s", opts->image_base ));

        if (opts->large_address_aware && opts->target.cpu == CPU_i386)
            strarray_add( &flags, "-Wl,--large-address-aware" );

        /* make sure we don't need a libgcc_s dll on Windows */
        if (!opts->nodefaultlibs && !opts->use_msvcrt)
            strarray_add( &flags, "-static-libgcc" );

        if (opts->debug_file && strendswith(opts->debug_file, ".pdb"))
            strarray_add(&link_args, strmake("-Wl,--pdb=%s", opts->debug_file));

        if (opts->build_id)
            strarray_add( &link_args, "-Wl,--build-id");

        if (opts->out_implib)
            strarray_add(&link_args, strmake("-Wl,--out-implib,%s", opts->out_implib));

        if (!try_link( opts->prefix, link_args, "-Wl,--file-alignment,0x1000" ))
            strarray_add( &link_args, strmake( "-Wl,--file-alignment,%s",
                                              opts->file_align ? opts->file_align : "0x1000" ));
        else if (!try_link( opts->prefix, link_args, "-Wl,-Xlink=-filealign:0x1000" ))
            /* lld from llvm 10 does not support mingw style --file-alignment,
             * but it's possible to use msvc syntax */
            strarray_add( &link_args, strmake( "-Wl,-Xlink=-filealign:%s",
                                              opts->file_align ? opts->file_align : "0x1000" ));

        strarray_addall( &link_args, flags );
        return link_args;

    case PLATFORM_WINDOWS:
        if (opts->shared || opts->win16_app)
        {
            strarray_add( &flags, "-shared" );
            strarray_add( &flags, "-Wl,-kill-at" );
        }
        if (opts->unicode_app) strarray_add( &flags, "-municode" );
        if (opts->nodefaultlibs || opts->use_msvcrt) strarray_add( &flags, "-nodefaultlibs" );
        if (opts->nostartfiles) strarray_add( &flags, "-nostartfiles" );
        if (opts->use_msvcrt) strarray_add( &flags, "-nostdlib" );
        if (opts->image_base) strarray_add( &flags, strmake("-Wl,-base:%s", opts->image_base ));
        if (opts->subsystem)
            strarray_add( &flags, strmake("-Wl,-subsystem:%s", opts->subsystem ));
        else
            strarray_add( &flags, strmake("-Wl,-subsystem:%s", opts->gui_app ? "windows" : "console" ));

        if (opts->debug_file && strendswith(opts->debug_file, ".pdb"))
        {
            strarray_add(&link_args, "-Wl,-debug");
            strarray_add(&link_args, strmake("-Wl,-pdb:%s", opts->debug_file));
        }
        else if (!opts->strip)
            strarray_add(&link_args, "-Wl,-debug:dwarf");

        if (opts->build_id)
            strarray_add( &link_args, "-Wl,-build-id");

        if (opts->out_implib)
            strarray_add(&link_args, strmake("-Wl,-implib:%s", opts->out_implib));
        else
            strarray_add(&link_args, strmake("-Wl,-implib:%s", make_temp_file( output_name, ".lib" )));

        strarray_add( &link_args, strmake( "-Wl,-filealign:%s", opts->file_align ? opts->file_align : "0x1000" ));

        strarray_addall( &link_args, flags );
        return link_args;

    default:
        if (opts->image_base)
        {
            if (!try_link( opts->prefix, link_args, strmake("-Wl,-Ttext-segment=%s", opts->image_base)) )
                strarray_add( &flags, strmake("-Wl,-Ttext-segment=%s", opts->image_base) );
        }
        if (!try_link( opts->prefix, link_args, "-Wl,-z,max-page-size=0x1000"))
            strarray_add( &flags, "-Wl,-z,max-page-size=0x1000");
        break;
    }

    if (opts->build_id)
        strarray_add( &link_args, "-Wl,--build-id");

    /* generic Unix shared library flags */

    strarray_add( &link_args, "-shared" );
    strarray_add( &link_args, "-Wl,-Bsymbolic" );
    if (!opts->noshortwchar && opts->target.cpu == CPU_ARM)
        strarray_add( &flags, "-Wl,--no-wchar-size-warning" );
    if (!try_link( opts->prefix, link_args, "-Wl,-z,defs" ))
        strarray_add( &flags, "-Wl,-z,defs" );

    strarray_addall( &link_args, flags );
    return link_args;
}

static const char *get_multiarch_dir( struct target target )
{
   switch (target.cpu)
   {
   case CPU_i386:    return "/i386-linux-gnu";
   case CPU_x86_64:  return "/x86_64-linux-gnu";
   case CPU_ARM:     return "/arm-linux-gnueabi";
   case CPU_ARM64:   return "/aarch64-linux-gnu";
   default:
       assert(0);
   }
   return NULL;
}

static char *get_lib_dir( struct options *opts )
{
    const char *stdlibpath[] = { libdir, LIBDIR, "/usr/lib", "/usr/local/lib", "/lib" };
    const char *bit_suffix, *other_bit_suffix, *build_multiarch, *target_multiarch, *winecrt0;
    const char *root = opts->sysroot ? opts->sysroot : "";
    unsigned int i;
    struct stat st;
    size_t build_len, target_len;

    bit_suffix = get_target_ptr_size( opts->target ) == 8 ? "64" : "32";
    other_bit_suffix = get_target_ptr_size( opts->target ) == 8 ? "32" : "64";
    winecrt0 = strmake( "/wine%s/libwinecrt0.a", get_arch_dir( opts->target ));
    build_multiarch = get_multiarch_dir( get_default_target() );
    target_multiarch = get_multiarch_dir( opts->target );
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
            if (get_default_target().cpu != opts->target.cpu &&
                !memcmp( p, build_multiarch, build_len ) && p[build_len] == '/')
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

static void compile(struct options* opts, const char* lang)
{
    struct strarray comp_args = get_translator(opts);
    unsigned int i, j;
    int gcc_defs = 0;
    struct strarray gcc;
    struct strarray gpp;

    if (opts->force_pointer_size)
        strarray_add( &comp_args, strmake("-m%u", 8 * opts->force_pointer_size ) );
    switch(opts->processor)
    {
	case proc_cpp: gcc_defs = 1; break;
	case proc_as:  gcc_defs = 0; break;
	/* Note: if the C compiler is gcc we assume the C++ compiler is too */
	/* mixing different C and C++ compilers isn't supported in configure anyway */
	case proc_cc:
	case proc_cxx:
            gcc = build_tool_name( opts, opts->target_alias, TOOL_CC );
            gpp = build_tool_name( opts, opts->target_alias, TOOL_CXX );
            for ( j = 0; !gcc_defs && j < comp_args.count; j++ )
            {
                const char *cc = comp_args.str[j];

                for (i = 0; !gcc_defs && i < gcc.count; i++)
                    gcc_defs = gcc.str[i][0] != '-' && strendswith(cc, gcc.str[i]);
                for (i = 0; !gcc_defs && i < gpp.count; i++)
                    gcc_defs = gpp.str[i][0] != '-' && strendswith(cc, gpp.str[i]);
            }
            break;
    }

    if (opts->target.platform == PLATFORM_WINDOWS ||
        opts->target.platform == PLATFORM_CYGWIN ||
        opts->target.platform == PLATFORM_MINGW)
        goto no_compat_defines;

    if (opts->processor != proc_cpp)
    {
	if (gcc_defs && !opts->wine_objdir && !opts->noshortwchar)
	{
            strarray_add(&comp_args, "-fshort-wchar");
            strarray_add(&comp_args, "-DWINE_UNICODE_NATIVE");
	}
        strarray_add(&comp_args, "-D_REENTRANT");
        if (opts->pic)
            strarray_add(&comp_args, "-fPIC");
        else
            strarray_add(&comp_args, "-fno-PIC");
    }

    if (get_target_ptr_size( opts->target ) == 8)
    {
        strarray_add(&comp_args, "-DWIN64");
        strarray_add(&comp_args, "-D_WIN64");
        strarray_add(&comp_args, "-D__WIN64");
        strarray_add(&comp_args, "-D__WIN64__");
    }

    strarray_add(&comp_args, "-DWIN32");
    strarray_add(&comp_args, "-D_WIN32");
    strarray_add(&comp_args, "-D__WIN32");
    strarray_add(&comp_args, "-D__WIN32__");
    strarray_add(&comp_args, "-D__WINNT");
    strarray_add(&comp_args, "-D__WINNT__");

    if (gcc_defs)
    {
        switch (opts->target.cpu)
        {
        case CPU_x86_64:
        case CPU_ARM64:
            strarray_add(&comp_args, "-D__stdcall=__attribute__((ms_abi))");
            strarray_add(&comp_args, "-D__cdecl=__stdcall");
            strarray_add(&comp_args, "-D__fastcall=__stdcall");
            break;
        case CPU_i386:
            strarray_add(&comp_args, "-D__stdcall=__attribute__((__stdcall__)) __attribute__((__force_align_arg_pointer__))");
            strarray_add(&comp_args, "-D__cdecl=__attribute__((__cdecl__)) __attribute__((__force_align_arg_pointer__))");
            strarray_add(&comp_args, "-D__fastcall=__attribute__((__fastcall__))");
            break;
        case CPU_ARM:
            strarray_add(&comp_args, "-D__stdcall=__attribute__((pcs(\"aapcs-vfp\")))");
            strarray_add(&comp_args, "-D__cdecl=__stdcall");
            strarray_add(&comp_args, "-D__fastcall=__stdcall");
            break;
        case CPU_ARM64EC:
            break;
        }
        strarray_add(&comp_args, "-D_stdcall=__stdcall");
        strarray_add(&comp_args, "-D_cdecl=__cdecl");
        strarray_add(&comp_args, "-D_fastcall=__fastcall");
	strarray_add(&comp_args, "-D__declspec(x)=__declspec_##x");
	strarray_add(&comp_args, "-D__declspec_align(x)=__attribute__((aligned(x)))");
	strarray_add(&comp_args, "-D__declspec_allocate(x)=__attribute__((section(x)))");
	strarray_add(&comp_args, "-D__declspec_deprecated=__attribute__((deprecated))");
	strarray_add(&comp_args, "-D__declspec_dllimport=__attribute__((dllimport))");
	strarray_add(&comp_args, "-D__declspec_dllexport=__attribute__((dllexport))");
	strarray_add(&comp_args, "-D__declspec_naked=__attribute__((naked))");
	strarray_add(&comp_args, "-D__declspec_noinline=__attribute__((noinline))");
	strarray_add(&comp_args, "-D__declspec_noreturn=__attribute__((noreturn))");
	strarray_add(&comp_args, "-D__declspec_nothrow=__attribute__((nothrow))");
	strarray_add(&comp_args, "-D__declspec_novtable=__attribute__(())"); /* ignore it */
	strarray_add(&comp_args, "-D__declspec_selectany=__attribute__((weak))");
	strarray_add(&comp_args, "-D__declspec_thread=__thread");
    }

    strarray_add(&comp_args, "-D__int8=char");
    strarray_add(&comp_args, "-D__int16=short");
    strarray_add(&comp_args, "-D__int32=int");
    if (get_target_ptr_size( opts->target ) == 8)
        strarray_add(&comp_args, "-D__int64=long");
    else
        strarray_add(&comp_args, "-D__int64=long long");

no_compat_defines:
    strarray_add(&comp_args, "-D__WINE__");

    /* options we handle explicitly */
    if (opts->compile_only)
	strarray_add(&comp_args, "-c");
    if (opts->output_name)
    {
	strarray_add(&comp_args, "-o");
	strarray_add(&comp_args, opts->output_name);
    }

    /* the rest of the pass-through parameters */
    strarray_addall(&comp_args, opts->compiler_args);

    /* the language option, if any */
    if (lang && strcmp(lang, "-xnone"))
	strarray_add(&comp_args, lang);

    /* last, but not least, the files */
    for ( j = 0; j < opts->files.count; j++ )
    {
	if (opts->files.str[j][0] != '-' || !opts->files.str[j][1]) /* not an option or bare '-' (i.e. stdin) */
	    strarray_add(&comp_args, opts->files.str[j]);
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
            if (includedir) strarray_add( &comp_args, strmake( "%s%s/wine/msvcrt", isystem, includedir ));
            for (j = 0; j < ARRAY_SIZE(incl_dirs); j++)
            {
                if (j && !strcmp( incl_dirs[0], incl_dirs[j] )) continue;
                strarray_add(&comp_args, strmake( "%s%s%s/wine/msvcrt", isystem, root, incl_dirs[j] ));
            }
            strarray_add(&comp_args, "-D__MSVCRT__");
        }
        if (includedir)
        {
            strarray_add( &comp_args, strmake( "%s%s/wine/windows", isystem, includedir ));
            strarray_add( &comp_args, strmake( "%s%s", idirafter, includedir ));
        }
        for (j = 0; j < ARRAY_SIZE(incl_dirs); j++)
        {
            if (j && !strcmp( incl_dirs[0], incl_dirs[j] )) continue;
            strarray_add(&comp_args, strmake( "%s%s%s/wine/windows", isystem, root, incl_dirs[j] ));
            strarray_add(&comp_args, strmake( "%s%s%s", idirafter, root, incl_dirs[j] ));
        }
    }
    else if (opts->wine_objdir)
        strarray_add(&comp_args, strmake("-I%s/include", opts->wine_objdir) );

    spawn(opts->prefix, comp_args, 0);
}

static const char* compile_to_object(struct options* opts, const char* file, const char* lang)
{
    struct options copts;

    /* make a copy so we don't change any of the initial stuff */
    /* a shallow copy is exactly what we want in this case */
    copts = *opts;
    copts.output_name = make_temp_file(get_basename_noext(file), ".o");
    copts.compile_only = 1;
    copts.files = empty_strarray;
    strarray_add(&copts.files, file);
    compile(&copts, lang);
    return copts.output_name;
}

/* return the initial set of options needed to run winebuild */
static struct strarray get_winebuild_args( struct options *opts, const char *target )
{
    const char* winebuild = getenv("WINEBUILD");
    const char *binary = NULL;
    struct strarray spec_args = empty_strarray;
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
    strarray_add( &spec_args, binary );
    if (verbose) strarray_add( &spec_args, "-v" );
    if (keep_generated) strarray_add( &spec_args, "--save-temps" );
    if (target)
    {
        strarray_add( &spec_args, "--target" );
        strarray_add( &spec_args, target );
    }
    if (opts->force_pointer_size)
        strarray_add(&spec_args, strmake("-m%u", 8 * opts->force_pointer_size ));
    for (i = 0; i < opts->prefix.count; i++)
        strarray_add( &spec_args, strmake( "-B%s", opts->prefix.str[i] ));
    strarray_addall( &spec_args, opts->winebuild_args );
    return spec_args;
}

static void fixup_constructors( struct options *opts, const char *file )
{
    struct strarray args = get_winebuild_args( opts, opts->target_alias );

    strarray_add( &args, "--fixup-ctors" );
    strarray_add( &args, file );
    spawn( opts->prefix, args, 0 );
}

static void make_wine_builtin( struct options *opts, const char *file )
{
    struct strarray args = get_winebuild_args( opts, opts->target_alias );

    strarray_add( &args, "--builtin" );
    strarray_add( &args, file );
    spawn( opts->prefix, args, 0 );
}

/* check if there is a static lib associated to a given dll */
static char *find_static_lib( const char *dll )
{
    char *lib = strmake("%s.a", dll);
    if (get_file_type(lib) == file_arh) return lib;
    free( lib );
    return NULL;
}

static const char *find_libgcc(struct strarray prefix, struct strarray link_tool)
{
    const char *out = make_temp_file( "find_libgcc", ".out" );
    const char *err = make_temp_file( "find_libgcc", ".err" );
    struct strarray link = empty_strarray;
    int sout = -1, serr = -1, i;
    char *libgcc, *p;
    struct stat st;
    size_t cnt;
    int ret;

    for (i = 0; i < link_tool.count; i++)
	if (strcmp(link_tool.str[i], "--no-default-config" ))
            strarray_add( &link, link_tool.str[i] );

    strarray_add( &link, "-print-libgcc-file-name" );

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
static void add_library( struct options *opts, struct strarray lib_dirs,
                         struct strarray *files, const char *library )
{
    char *static_lib, *fullname = 0;

    switch(get_lib_type(opts->target, lib_dirs, library, "lib", opts->lib_suffix, &fullname))
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

/* run winebuild to generate the .spec.o file */
static void build_spec_obj( struct options *opts, const char *spec_file, const char *output_file,
                            const char *target, struct strarray files, struct strarray resources,
                            const char *entry_point, struct strarray *spec_objs )
{
    unsigned int i;
    int is_pe = is_pe_target( opts );
    struct strarray spec_args = get_winebuild_args( opts, target );
    struct strarray tool;
    const char *spec_o_name, *output_name;

    /* get the filename from the path */
    output_name = get_basename( output_file );

    tool = build_tool_name( opts, target, TOOL_CC );
    strarray_add( &spec_args, strmake( "--cc-cmd=%s", strarray_tostring( tool, " " )));
    if (!is_pe)
    {
        tool = build_tool_name( opts, target, TOOL_LD );
        strarray_add( &spec_args, strmake( "--ld-cmd=%s", strarray_tostring( tool, " " )));
    }

    spec_o_name = make_temp_file(output_name, ".spec.o");
    if (!is_pe)
    {
        if (opts->pic) strarray_add(&spec_args, "-fPIC");
        if (opts->use_msvcrt) strarray_add(&spec_args, "-mno-cygwin");
        if (opts->unwind_tables) strarray_add( &spec_args, "-fasynchronous-unwind-tables" );
    }
    strarray_add(&spec_args, opts->shared ? "--dll" : "--exe");
    if (opts->fake_module)
    {
        strarray_add(&spec_args, "--fake-module");
        strarray_add(&spec_args, "-o");
        strarray_add(&spec_args, output_file);
    }
    else
    {
        strarray_add(&spec_args, "-o");
        strarray_add(&spec_args, spec_o_name);
    }
    if (spec_file)
    {
        strarray_add(&spec_args, "-E");
        strarray_add(&spec_args, spec_file);
    }
    else
    {
        strarray_add(&spec_args, "-F");
        strarray_add(&spec_args, output_name);
    }

    if (!opts->shared)
    {
        strarray_add(&spec_args, "--subsystem");
        strarray_add(&spec_args, opts->gui_app ? "windows" : "console");
        if (opts->large_address_aware) strarray_add( &spec_args, "--large-address-aware" );
    }

    if (opts->target.platform == PLATFORM_WINDOWS && opts->target.cpu == CPU_i386)
        strarray_add(&spec_args, "--safeseh");

    if (entry_point)
    {
        strarray_add(&spec_args, "--entry");
        strarray_add(&spec_args, entry_point);
    }

    if (opts->subsystem)
    {
        strarray_add(&spec_args, "--subsystem");
        strarray_add(&spec_args, opts->subsystem);
    }

    if (!is_pe)
    {
        for (i = 0; i < opts->delayimports.count; i++)
            strarray_add(&spec_args, strmake("-d%s", opts->delayimports.str[i]));
    }

    strarray_addall( &spec_args, resources );

    /* add other files */
    strarray_add(&spec_args, "--");
    for (i = 0; i < files.count; i++)
    {
	switch(files.str[i][1])
	{
	    case 'd':
	    case 'a':
	    case 'o':
		strarray_add(&spec_args, files.str[i] + 2);
		break;
	}
    }

    spawn(opts->prefix, spec_args, 0);
    strarray_add( spec_objs, spec_o_name );
}

/* run winebuild to generate a data-only library */
static void build_data_lib( struct options *opts, const char *spec_file, const char *output_file, struct strarray files )
{
    unsigned int i;
    struct strarray spec_args = get_winebuild_args( opts, opts->target_alias );

    strarray_add(&spec_args, opts->shared ? "--dll" : "--exe");
    strarray_add(&spec_args, "-o");
    strarray_add(&spec_args, output_file);
    if (spec_file)
    {
        strarray_add(&spec_args, "-E");
        strarray_add(&spec_args, spec_file);
    }

    /* add resource files */
    for (i = 0; i < files.count; i++)
	if (files.str[i][1] == 'r') strarray_add(&spec_args, files.str[i]);

    spawn(opts->prefix, spec_args, 0);
}

static void build(struct options* opts)
{
    struct strarray resources = empty_strarray;
    struct strarray spec_objs = empty_strarray;
    struct strarray lib_dirs = empty_strarray;
    struct strarray files = empty_strarray;
    struct strarray link_args;
    char *output_file, *output_path;
    const char *output_name, *spec_file, *lang;
    const char *libgcc = NULL;
    int generate_app_loader = 1;
    const char *crt_lib = NULL, *entry_point = NULL;
    int is_pe = is_pe_target( opts );
    unsigned int i, j;

    /* NOTE: for the files array we'll use the following convention:
     *    -axxx:  xxx is an archive (.a)
     *    -dxxx:  xxx is a DLL (.def)
     *    -lxxx:  xxx is an unsorted library
     *    -oxxx:  xxx is an object (.o)
     *    -rxxx:  xxx is a resource (.res)
     *    -sxxx:  xxx is a shared lib (.so)
     *    -xlll:  lll is the language (c, c++, etc.)
     */

    output_file = xstrdup( opts->output_name ? opts->output_name : "a.out" );

    /* 'winegcc -o app xxx.exe.so' only creates the load script */
    if (opts->files.count == 1 && strendswith(opts->files.str[0], ".exe.so"))
    {
	create_file(output_file, 0755, app_loader_template, opts->files.str[0]);
	return;
    }

    /* generate app loader only for .exe */
    if (opts->shared || is_pe || strendswith(output_file, ".so"))
	generate_app_loader = 0;

    if (strendswith(output_file, ".fake")) opts->fake_module = 1;

    /* normalize the filename a bit: strip .so, ensure it has proper ext */
    if (!strchr(get_basename( output_file ), '.'))
        output_file = strmake("%s.%s", output_file, opts->shared ? "dll" : "exe");
    else if (strendswith(output_file, ".so"))
	output_file[strlen(output_file) - 3] = 0;
    output_path = is_pe ? output_file : strmake( "%s.so", output_file );

    /* get the filename from the path */
    output_name = get_basename( output_file );

    /* prepare the linking path */
    if (!opts->wine_objdir)
    {
        char *lib_dir = get_lib_dir( opts );
        strarray_addall( &lib_dirs, opts->lib_dirs );
        strarray_add( &lib_dirs, strmake( "%s/wine%s", lib_dir, get_arch_dir( opts->target )));
        strarray_add( &lib_dirs, lib_dir );
    }
    else
    {
        strarray_add(&lib_dirs, strmake("%s/dlls", opts->wine_objdir));
        strarray_addall(&lib_dirs, opts->lib_dirs);
    }

    /* mark the files with their appropriate type */
    spec_file = lang = 0;
    for ( j = 0; j < opts->files.count; j++ )
    {
	const char* file = opts->files.str[j];
	if (file[0] != '-')
	{
	    switch(get_file_type(file))
	    {
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
		    strarray_add(&files, strmake("-r%s", file));
		    break;
		case file_obj:
		    strarray_add(&files, strmake("-o%s", file));
		    break;
		case file_arh:
                    if (opts->use_msvcrt)
                    {
                        char *p = get_basename( file );
                        if (!strncmp(p, "libmsvcr", 8) || !strncmp(p, "libucrt", 7)) crt_lib = file;
                    }
                    strarray_add(&files, strmake("-a%s", file));
                    break;
		case file_so:
		    strarray_add(&files, strmake("-s%s", file));
		    break;
	    	case file_na:
		    error("File does not exist: %s\n", file);
		    break;
	        default:
		    file = compile_to_object(opts, file, lang);
		    strarray_add(&files, strmake("-o%s", file));
		    break;
	    }
	}
	else if (file[1] == 'l')
            add_library(opts, lib_dirs, &files, file + 2 );
	else if (file[1] == 'x')
	    lang = file;
        else if(file[1] == 'W')
            strarray_add(&files, file);
    }

    /* add the default libraries, if needed */

    if (!opts->wine_objdir && !opts->nodefaultlibs)
    {
        if (opts->gui_app)
	{
	    add_library(opts, lib_dirs, &files, "shell32");
	    add_library(opts, lib_dirs, &files, "comdlg32");
	    add_library(opts, lib_dirs, &files, "gdi32");
	}
        add_library(opts, lib_dirs, &files, "advapi32");
        add_library(opts, lib_dirs, &files, "user32");
        add_library(opts, lib_dirs, &files, "winecrt0");
        if (opts->target.platform == PLATFORM_WINDOWS)
            add_library(opts, lib_dirs, &files, "compiler-rt");
        if (opts->use_msvcrt)
        {
            if (!crt_lib)
            {
                if (strncmp( output_name, "msvcr", 5 ) &&
                    strncmp( output_name, "ucrt", 4 ) &&
                    strcmp( output_name, "crtdll.dll" ))
                    add_library(opts, lib_dirs, &files, "ucrtbase");
            }
            else strarray_add(&files, strmake("-a%s", crt_lib));
        }
        if (opts->win16_app) add_library(opts, lib_dirs, &files, "kernel");
        add_library(opts, lib_dirs, &files, "kernel32");
        add_library(opts, lib_dirs, &files, "ntdll");
    }

    /* set default entry point, if needed */
    if (!opts->entry_point)
    {
        if (opts->subsystem && !strcmp( opts->subsystem, "native" ))
            entry_point = (is_pe && opts->target.cpu == CPU_i386) ? "DriverEntry@8" : "DriverEntry";
        else if (opts->use_msvcrt && !opts->shared && !opts->win16_app)
            entry_point = opts->unicode_app ? "wmainCRTStartup" : "mainCRTStartup";
    }
    else entry_point = opts->entry_point;

    /* run winebuild to generate the .spec.o file */
    if (opts->data_only)
    {
        build_data_lib( opts, spec_file, output_file, files );
        return;
    }

    for (i = 0; i < files.count; i++)
	if (files.str[i][1] == 'r') strarray_add( &resources, files.str[i] );

    build_spec_obj( opts, spec_file, output_file, opts->target_alias, files, resources,
                    entry_point, &spec_objs );
    if (opts->native_arch)
    {
        const char *suffix = strchr( opts->target_alias, '-' );
        if (!suffix) suffix = "";
        build_spec_obj( opts, spec_file, output_file, strmake( "%s%s", opts->native_arch, suffix ),
                        files, empty_strarray, entry_point, &spec_objs );
    }

    if (opts->fake_module) return;  /* nothing else to do */

    /* link everything together now */
    link_args = get_link_args( opts, output_name );

    if (opts->nodefaultlibs || opts->use_msvcrt)
    {
        switch (opts->target.platform)
        {
        case PLATFORM_MINGW:
        case PLATFORM_CYGWIN:
            libgcc = find_libgcc( opts->prefix, link_args );
            if (!libgcc) libgcc = "-lgcc";
            break;
        default:
            break;
        }
    }

    strarray_add(&link_args, "-o");
    strarray_add(&link_args, output_path);

    for ( j = 0; j < lib_dirs.count; j++ )
	strarray_add(&link_args, strmake("-L%s", lib_dirs.str[j]));

    if (is_pe && opts->use_msvcrt && !entry_point && (opts->shared || opts->win16_app))
        entry_point = opts->target.cpu == CPU_i386 ? "DllMainCRTStartup@12" : "DllMainCRTStartup";

    if (is_pe && entry_point)
    {
        if (opts->target.platform == PLATFORM_WINDOWS)
            strarray_add(&link_args, strmake("-Wl,-entry:%s", entry_point));
        else
            strarray_add(&link_args, strmake("-Wl,--entry,%s%s",
                                            is_pe && opts->target.cpu == CPU_i386 ? "_" : "",
                                            entry_point));
    }

    strarray_addall( &link_args, spec_objs );

    if (is_pe)
    {
        for (j = 0; j < opts->delayimports.count; j++)
        {
            if (opts->target.platform == PLATFORM_WINDOWS)
                strarray_add(&link_args, strmake("-Wl,-delayload:%s", opts->delayimports.str[j]));
            else
                strarray_add(&link_args, strmake("-Wl,-delayload,%s",opts->delayimports.str[j]));
        }
    }

    for ( j = 0; j < files.count; j++ )
    {
	const char* name = files.str[j] + 2;
	switch(files.str[j][1])
	{
	    case 'l':
		strarray_add(&link_args, strmake("-l%s", name));
		break;
	    case 's':
	    case 'o':
		strarray_add(&link_args, name);
		break;
	    case 'a':
                if (!opts->use_msvcrt && !opts->lib_suffix && strchr(name, '/'))
                {
                    /* turn the path back into -Ldir -lfoo options
                     * this makes sure that we use the specified libs even
                     * when mingw adds its own import libs to the link */
                    const char *p = get_basename( name );

                    if (is_pe)
                    {
                        if (!strncmp( p, "lib", 3 ) && strcmp( p, "libmsvcrt.a" ))
                        {
                            strarray_add(&link_args, strmake("-L%s", get_dirname(name) ));
                            strarray_add(&link_args, strmake("-l%s", get_basename_noext( p + 3 )));
                            break;
                        }
                    }
                    else
                    {
                        /* don't link to ntdll or ntoskrnl in non-msvcrt mode
                         * since they export CRT functions */
                        if (!strcmp( p, "libntdll.a" )) break;
                        if (!strcmp( p, "libntoskrnl.a" )) break;
                    }
                }
		strarray_add(&link_args, name);
		break;
	    case 'W':
		strarray_add(&link_args, files.str[j]);
		break;
	}
    }

    if (!opts->nostdlib && !is_pe)
    {
	strarray_add(&link_args, "-ldl");
	strarray_add(&link_args, "-lm");
	strarray_add(&link_args, "-lc");
    }

    if (libgcc) strarray_add(&link_args, libgcc);

    output_file_name = output_path;
    output_debug_file = opts->debug_file;
    output_implib = opts->out_implib;
    atexit( cleanup_output_files );

    spawn(opts->prefix, link_args, 0);

    if (opts->debug_file && !strendswith(opts->debug_file, ".pdb"))
    {
        struct strarray tool, objcopy = build_tool_name(opts, opts->target_alias, TOOL_OBJCOPY);

        tool = empty_strarray;
        strarray_addall( &tool, objcopy );
        strarray_add(&tool, "--only-keep-debug");
        strarray_add(&tool, output_path);
        strarray_add(&tool, opts->debug_file);
        spawn(opts->prefix, tool, 1);

        tool = empty_strarray;
        strarray_addall( &tool, objcopy );
        strarray_add(&tool, "--strip-debug");
        strarray_add(&tool, output_path);
        spawn(opts->prefix, tool, 1);

        tool = empty_strarray;
        strarray_addall( &tool, objcopy );
        strarray_add(&tool, "--add-gnu-debuglink");
        strarray_add(&tool, opts->debug_file);
        strarray_add(&tool, output_path);
        spawn(opts->prefix, tool, 0);
    }

    if (opts->out_implib && !is_pe)
    {
        struct strarray tool, implib_args;

        if (!spec_file)
            error("--out-implib requires a .spec or .def file\n");

        implib_args = get_winebuild_args( opts, opts->target_alias );
        tool = build_tool_name( opts, opts->target_alias, TOOL_CC );
        strarray_add( &implib_args, strmake( "--cc-cmd=%s", strarray_tostring( tool, " " )));
        tool = build_tool_name( opts, opts->target_alias, TOOL_LD );
        strarray_add( &implib_args, strmake( "--ld-cmd=%s", strarray_tostring( tool, " " )));

        strarray_add(&implib_args, "--implib");
        strarray_add(&implib_args, "-o");
        strarray_add(&implib_args, opts->out_implib);
        strarray_add(&implib_args, "--export");
        strarray_add(&implib_args, spec_file);

        spawn(opts->prefix, implib_args, 0);
    }

    if (!is_pe) fixup_constructors( opts, output_path );
    else if (opts->wine_builtin) make_wine_builtin( opts, output_path );

    /* create the loader script */
    if (generate_app_loader)
        create_file(output_file, 0755, app_loader_template, strmake("%s.so", output_name));
}


static void forward( struct options *opts )
{
    struct strarray args = get_translator(opts);

    strarray_addall(&args, opts->compiler_args);
    strarray_addall(&args, opts->linker_args);
    spawn(opts->prefix, args, 0);
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
    opts->target_alias = xstrdup( target );
    if (!parse_target( target, &opts->target )) error( "Invalid target specification '%s'\n", target );
}

static int is_option( struct options *opts, int i, const char *option, const char **option_arg )
{
    if (!strcmp( opts->args.str[i], option ))
    {
        if (opts->args.count == i) error( "option %s requires an argument\n", opts->args.str[i] );
        *option_arg = opts->args.str[i + 1];
        return 1;
    }
    if (!strncmp( opts->args.str[i], option, strlen(option) ) && opts->args.str[i][strlen(option)] == '=')
    {
        *option_arg = opts->args.str[i] + strlen(option) + 1;
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

    init_signals( exit_on_signal );
    bindir = get_bindir( argv[0] );
    libdir = get_libdir( bindir );
    includedir = get_includedir( bindir );

    /* setup tmp file removal at exit */
    atexit(clean_temp_files);

    /* initialize options */
    memset(&opts, 0, sizeof(opts));
    opts.target = init_argv0_target( argv[0] );
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
            strarray_add( &opts.args, argv[i] );
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
            strarray_add( &opts.args, opt );
        }
    }

    /* parse options */
    for (i = 0; i < opts.args.count; i++)
    {
        if (opts.args.str[i][0] == '-' && opts.args.str[i][1])  /* option, except '-' alone is stdin, which is a file */
	{
	    /* determine if this switch is followed by a separate argument */
	    next_is_arg = 0;
	    option_arg = 0;
	    switch(opts.args.str[i][1])
	    {
		case 'x': case 'o': case 'D': case 'U':
		case 'I': case 'A': case 'l': case 'u':
		case 'b': case 'V': case 'G': case 'L':
		case 'B': case 'R': case 'z':
		    if (opts.args.str[i][2]) option_arg = &opts.args.str[i][2];
		    else next_is_arg = 1;
		    break;
		case 'i':
		    next_is_arg = 1;
		    break;
		case 'a':
		    if (strcmp("-aux-info", opts.args.str[i]) == 0)
			next_is_arg = 1;
		    if (strcmp("-arch", opts.args.str[i]) == 0)
			next_is_arg = 1;
		    break;
		case 'X':
		    if (strcmp("-Xlinker", opts.args.str[i]) == 0)
			next_is_arg = 1;
		    break;
		case 'M':
		    c = opts.args.str[i][2];
		    if (c == 'F' || c == 'T' || c == 'Q')
		    {
			if (opts.args.str[i][3]) option_arg = &opts.args.str[i][3];
			else next_is_arg = 1;
		    }
		    break;
		case 'f':
		    if (strcmp("-framework", opts.args.str[i]) == 0)
			next_is_arg = 1;
		    break;
                case 't':
                    next_is_arg = strcmp("-target", opts.args.str[i]) == 0;
                    break;
		case '-':
		    next_is_arg = (strcmp("--param", opts.args.str[i]) == 0 ||
                                   strcmp("--sysroot", opts.args.str[i]) == 0 ||
                                   strcmp("--target", opts.args.str[i]) == 0 ||
                                   strcmp("--wine-objdir", opts.args.str[i]) == 0 ||
                                   strcmp("--winebuild", opts.args.str[i]) == 0 ||
                                   strcmp("--lib-suffix", opts.args.str[i]) == 0);
		    break;
	    }
	    if (next_is_arg)
            {
                if (i + 1 >= opts.args.count) error("option -%c requires an argument\n", opts.args.str[i][1]);
                option_arg = opts.args.str[i+1];
            }

	    /* determine what options go 'as is' to the linker & the compiler */
	    raw_linker_arg = is_linker_arg(opts.args.str[i]);
	    raw_compiler_arg = !raw_linker_arg;
            raw_winebuild_arg = 0;

	    /* do a bit of semantic analysis */
            switch (opts.args.str[i][1])
	    {
		case 'B':
		    str = xstrdup(option_arg);
		    if (strendswith(str, "/")) str[strlen(str) - 1] = 0;
                    strarray_add(&opts.prefix, str);
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
		    if (opts.args.str[i][2] == 0) opts.compile_only = 1;
		    /* fall through */
                case 'S':        /* generate assembler code */
                case 'E':        /* preprocess only */
                    if (opts.args.str[i][2] == 0) linking = 0;
                    break;
		case 'f':
		    if (strcmp("-fno-short-wchar", opts.args.str[i]) == 0)
                        opts.noshortwchar = 1;
		    else if (!strcmp("-fasynchronous-unwind-tables", opts.args.str[i]))
                        opts.unwind_tables = 1;
		    else if (!strcmp("-fno-asynchronous-unwind-tables", opts.args.str[i]))
                        opts.unwind_tables = 0;
		    else if (!strcmp("-fms-hotpatch", opts.args.str[i]))
                        raw_linker_arg = 1;
                    else if (!strcmp("-fPIC", opts.args.str[i]) || !strcmp("-fpic", opts.args.str[i]))
                        opts.pic = 1;
                    else if (!strcmp("-fno-PIC", opts.args.str[i]) || !strcmp("-fno-pic", opts.args.str[i]))
                        opts.pic = 0;
		    break;
                case 'i':
                    if (!strcmp( "-isysroot", opts.args.str[i] )) opts.isysroot = opts.args.str[i + 1];
                    break;
		case 'l':
		    strarray_add(&opts.files, strmake("-l%s", option_arg));
                    raw_compiler_arg = 0;
		    break;
		case 'L':
		    strarray_add(&opts.lib_dirs, option_arg);
                    raw_compiler_arg = 0;
		    break;
                case 'M':        /* map file generation */
                    linking = 0;
                    break;
		case 'm':
		    if (strcmp("-mno-cygwin", opts.args.str[i]) == 0)
                    {
			opts.use_msvcrt = 1;
                        raw_compiler_arg = 0;
                    }
		    if (strcmp("-mcygwin", opts.args.str[i]) == 0)
                    {
			opts.use_msvcrt = 0;
                        raw_compiler_arg = 0;
                    }
		    else if (strcmp("-mwindows", opts.args.str[i]) == 0)
                    {
			opts.gui_app = 1;
                        raw_compiler_arg = 0;
                    }
		    else if (strcmp("-mconsole", opts.args.str[i]) == 0)
                    {
			opts.gui_app = 0;
                        raw_compiler_arg = 0;
                    }
		    else if (strcmp("-municode", opts.args.str[i]) == 0)
                    {
			opts.unicode_app = 1;
                        raw_compiler_arg = 0;
                        raw_winebuild_arg = 1;
                    }
		    else if (strcmp("-mthreads", opts.args.str[i]) == 0)
                    {
                        raw_compiler_arg = 0;
                    }
		    else if (strcmp("-m16", opts.args.str[i]) == 0)
                    {
			opts.win16_app = 1;
                        raw_compiler_arg = 0;
                        raw_winebuild_arg = 1;
                    }
		    else if (strcmp("-m32", opts.args.str[i]) == 0)
                    {
                        set_target_ptr_size( &opts.target, 4 );
                        opts.force_pointer_size = 4;
			raw_linker_arg = 1;
                    }
		    else if (strcmp("-m64", opts.args.str[i]) == 0)
                    {
                        set_target_ptr_size( &opts.target, 8 );
                        opts.force_pointer_size = 8;
			raw_linker_arg = 1;
                    }
                    else if (!strcmp("-marm", opts.args.str[i] ) || !strcmp("-mthumb", opts.args.str[i] ))
                    {
			raw_linker_arg = 1;
                    }
                    else if (!strcmp("-marm64x", opts.args.str[i] ))
                    {
                        raw_linker_arg = 1;
                        opts.native_arch = "aarch64";
                    }
                    else if (!strncmp("-mcpu=", opts.args.str[i], 6) ||
                             !strncmp("-mfpu=", opts.args.str[i], 6) ||
                             !strncmp("-march=", opts.args.str[i], 7))
                        raw_winebuild_arg = 1;
		    break;
                case 'n':
                    if (strcmp("-nostdinc", opts.args.str[i]) == 0)
                        opts.nostdinc = 1;
                    else if (strcmp("-nodefaultlibs", opts.args.str[i]) == 0)
                        opts.nodefaultlibs = 1;
                    else if (strcmp("-nostdlib", opts.args.str[i]) == 0)
                        opts.nostdlib = 1;
                    else if (strcmp("-nostartfiles", opts.args.str[i]) == 0)
                        opts.nostartfiles = 1;
                    break;
		case 'o':
		    opts.output_name = option_arg;
                    raw_compiler_arg = 0;
		    break;
                case 'p':
                    if (strcmp("-pthread", opts.args.str[i]) == 0)
                    {
                        raw_compiler_arg = 1;
                        raw_linker_arg = 1;
                    }
                    break;
                case 's':
                    if (strcmp("-static", opts.args.str[i]) == 0)
			linking = -1;
		    else if(strcmp("-save-temps", opts.args.str[i]) == 0)
			keep_generated = 1;
                    else if (strncmp("-specs=", opts.args.str[i], 7) == 0)
                        raw_linker_arg = 1;
		    else if(strcmp("-shared", opts.args.str[i]) == 0)
		    {
			opts.shared = 1;
                        raw_compiler_arg = raw_linker_arg = 0;
		    }
                    else if (strcmp("-s", opts.args.str[i]) == 0 && opts.target.platform == PLATFORM_APPLE)
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
                    if (opts.args.str[i][2] == 0) verbose++;
                    break;
                case 'W':
                    if (strncmp("-Wl,", opts.args.str[i], 4) == 0)
		    {
                        unsigned int j;
                        struct strarray Wl = strarray_fromstring(opts.args.str[i] + 4, ",");
                        for (j = 0; j < Wl.count; j++)
                        {
                            if (!strcmp(Wl.str[j], "--image-base") && j < Wl.count - 1)
                            {
                                opts.image_base = xstrdup( Wl.str[++j] );
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "--section-alignment") && j < Wl.count - 1)
                            {
                                opts.section_align = xstrdup( Wl.str[++j] );
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "--file-alignment") && j < Wl.count - 1)
                            {
                                opts.file_align = xstrdup( Wl.str[++j] );
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "--large-address-aware"))
                            {
                                opts.large_address_aware = 1;
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "--wine-builtin"))
                            {
                                opts.wine_builtin = 1;
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "--subsystem") && j < Wl.count - 1)
                            {
                                opts.subsystem = xstrdup( Wl.str[++j] );
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "--entry") && j < Wl.count - 1)
                            {
                                opts.entry_point = xstrdup( Wl.str[++j] );
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "-delayload") && j < Wl.count - 1)
                            {
                                strarray_add( &opts.delayimports, Wl.str[++j] );
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "--debug-file") && j < Wl.count - 1)
                            {
                                opts.debug_file = xstrdup( Wl.str[++j] );
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "--whole-archive") ||
                                !strcmp(Wl.str[j], "--no-whole-archive") ||
                                !strcmp(Wl.str[j], "--start-group") ||
                                !strcmp(Wl.str[j], "--end-group"))
                            {
                                strarray_add( &opts.files, strmake( "-Wl,%s", Wl.str[j] ));
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "--out-implib"))
                            {
                                opts.out_implib = xstrdup( Wl.str[++j] );
                                continue;
                            }
                            if (!strcmp( Wl.str[j], "--build-id" ))
                            {
                                opts.build_id = 1;
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "-static")) linking = -1;
                            strarray_add(&opts.linker_args, strmake("-Wl,%s",Wl.str[j]));
                        }
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
		    else if (strncmp("-Wb,", opts.args.str[i], 4) == 0)
		    {
                        unsigned int j;
                        struct strarray Wb = strarray_fromstring(opts.args.str[i] + 4, ",");
                        for (j = 0; j < Wb.count; j++)
                        {
                            if (!strcmp(Wb.str[j], "--data-only")) opts.data_only = 1;
                            if (!strcmp(Wb.str[j], "--fake-module")) opts.fake_module = 1;
                            else strarray_add( &opts.winebuild_args, Wb.str[j] );
                        }
                        raw_compiler_arg = raw_linker_arg = 0;
		    }
                    break;
		case 'x':
		    lang = strmake("-x%s", option_arg);
		    strarray_add(&opts.files, lang);
		    /* we'll pass these flags ourselves, explicitly */
                    raw_compiler_arg = raw_linker_arg = 0;
		    break;
                case '-':
                    if (strcmp("-static", opts.args.str[i]+1) == 0)
                        linking = -1;
                    else if (!strcmp( "-no-default-config", opts.args.str[i] + 1 ))
                    {
                        opts.no_default_config = 1;
                        raw_compiler_arg = raw_linker_arg = 1;
                    }
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
		strarray_add( &opts.linker_args, opts.args.str[i] );
		if (next_is_arg && (i + 1 < opts.args.count))
		    strarray_add( &opts.linker_args, opts.args.str[i + 1] );
	    }
	    if (raw_compiler_arg)
	    {
		strarray_add( &opts.compiler_args, opts.args.str[i] );
		if (next_is_arg && (i + 1 < opts.args.count))
		    strarray_add( &opts.compiler_args, opts.args.str[i + 1] );
	    }
            if (raw_winebuild_arg)
            {
                strarray_add( &opts.winebuild_args, opts.args.str[i] );
		if (next_is_arg && (i + 1 < opts.args.count))
		    strarray_add( &opts.winebuild_args, opts.args.str[i + 1] );
            }

	    /* skip the next token if it's an argument */
	    if (next_is_arg) i++;
        }
	else
	{
	    strarray_add( &opts.files, opts.args.str[i] );
	}
    }

    if (opts.force_pointer_size) set_target_ptr_size( &opts.target, opts.force_pointer_size );
    if (opts.processor == proc_cpp) linking = 0;
    if (linking == -1) error("Static linking is not supported\n");

    if (is_pe_target( &opts )) opts.use_msvcrt = 1;

    if (opts.files.count == 0 && !opts.fake_module) forward(&opts);
    else if (linking) build(&opts);
    else compile(&opts, lang);

    output_file_name = NULL;
    output_debug_file = NULL;
    output_implib = NULL;
    return 0;
}
