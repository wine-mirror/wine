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
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <sys/types.h>
#include "../tools.h"

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
static const char *output;
static int keep_generated = 0;
static int verbose;
const char *temp_dir = NULL;
struct strarray temp_files = { 0 };

static const char *bindir;
static const char *libdir;
static const char *includedir;
static const char *wine_objdir;
static const char *winebuild;
static const char *lib_suffix;
static const char *sysroot;
static const char *isysroot;
static const char *target_alias;
static const char *target_version;
static struct target target;
static struct strarray prefix_dirs;
static struct strarray path_dirs;
static struct strarray lib_path_dirs;

static enum processor { proc_cc, proc_cxx, proc_cpp } processor = proc_cc;

enum file_type { file_na, file_other, file_obj, file_res, file_rc, file_arh, file_dll, file_so, file_spec };

static bool is_pe;
static bool is_static;
static bool is_shared;
static bool is_gui_app;
static bool is_unicode_app;
static bool is_win16_app;
static bool use_msvcrt;
static bool use_pic = true;
static bool use_build_id;
static bool nostdinc;
static bool nostdlib;
static bool nostartfiles;
static bool nodefaultlibs;
static bool noshortwchar;
static bool data_only;
static bool fake_module;
static bool large_address_aware;
static bool wine_builtin;
static bool unwind_tables;
static bool strip;
static bool compile_only;
static bool skip_link;
static bool no_default_config;
static int force_pointer_size;

static const char *image_base;
static const char *section_align;
static const char *file_align;
static const char *subsystem;
static const char *entry_point;
static const char *native_arch;
static struct strarray file_args;
static struct strarray linker_args;
static struct strarray compiler_args;
static struct strarray winebuild_args;
static struct strarray delayimports;


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

static void error(const char* s, ...)
{
    va_list ap;

    va_start(ap, s);
    fprintf(stderr, "winegcc: ");
    vfprintf(stderr, s, ap);
    va_end(ap);
    exit(2);
}

static void create_file(const char* name, int mode, const char* fmt, ...)
{
    va_list ap;
    FILE *file;

    if (verbose) printf("Creating file %s\n", name);
    va_start(ap, fmt);
    if ( !(file = fopen(name, "w")) )
	error("Unable to open %s for writing\n", name);
    vfprintf(file, fmt, ap);
    va_end(ap);
    fclose(file);
    chmod(name, mode);
}

static enum file_type get_file_type(const char* filename)
{
    /* see tools/winebuild/res32.c: check_header for details */
    static const char res_sig[] = { 0,0,0,0, 32,0,0,0, 0xff,0xff, 0,0, 0xff,0xff, 0,0, 0,0,0,0, 0,0, 0,0, 0,0,0,0, 0,0,0,0 };
    static const char elf_sig[4] = "\177ELF";
    static const char ar_sig[8] = "!<arch>\n";
    char buf[sizeof(res_sig)];
    int fd, cnt;

    fd = open( filename, O_RDONLY );
    if (fd == -1) return file_na;
    cnt = read(fd, buf, sizeof(buf));
    close( fd );
    if (cnt == -1) return file_na;

    if (cnt == sizeof(res_sig) && !memcmp(buf, res_sig, sizeof(res_sig))) return file_res;
    if (strendswith(filename, ".o")) return file_obj;
    if (strendswith(filename, ".obj")) return file_obj;
    if (strendswith(filename, ".a")) return file_arh;
    if (strendswith(filename, ".res")) return file_res;
    if (strendswith(filename, ".so")) return file_so;
    if (strendswith(filename, ".dylib")) return file_so;
    if (strendswith(filename, ".def")) return file_spec;
    if (strendswith(filename, ".spec")) return file_spec;
    if (strendswith(filename, ".rc")) return file_rc;
    if (cnt >= sizeof(elf_sig) && !memcmp(buf, elf_sig, sizeof(elf_sig))) return file_so;  /* ELF lib */
    if (cnt >= sizeof(ar_sig) && !memcmp(buf, ar_sig, sizeof(ar_sig))) return file_arh;
    if (cnt >= sizeof(unsigned int) &&
        (*(unsigned int *)buf == 0xfeedface || *(unsigned int *)buf == 0xcefaedfe ||
         *(unsigned int *)buf == 0xfeedfacf || *(unsigned int *)buf == 0xcffaedfe))
        return file_so; /* Mach-O lib */

    return file_other;
}

static char* try_lib_path(const char *dir, const char *arch_dir, const char* library,
                          const char* ext, enum file_type expected_type)
{
    char *fullname;
    enum file_type type;

    /* first try a subdir named from the library we are looking for */
    fullname = strmake("%s/%s%s/lib%s%s", dir, library, arch_dir, library, ext);
    if (verbose > 1) fprintf(stderr, "Try %s...", fullname);
    type = get_file_type(fullname);
    if (verbose > 1) fprintf(stderr, type == expected_type ? "FOUND!\n" : "no\n");
    if (type == expected_type) return fullname;
    free( fullname );

    fullname = strmake("%s/lib%s%s", dir, library, ext);
    if (verbose > 1) fprintf(stderr, "Try %s...", fullname);
    type = get_file_type(fullname);
    if (verbose > 1) fprintf(stderr, type == expected_type ? "FOUND!\n" : "no\n");
    if (type == expected_type) return fullname;
    free( fullname );
    return 0;
}

static enum file_type guess_lib_type(const char* dir, const char* library, char** file)
{
    const char *arch_dir = "";
    const char *suffix = lib_suffix ? lib_suffix : ".a";

    if (!is_pe)
    {
        /* Unix shared object */
        if ((*file = try_lib_path(dir, "", library, ".so", file_so)))
            return file_so;

        /* Mach-O (Darwin/Mac OS X) Dynamic Library behaves mostly like .so */
        if ((*file = try_lib_path(dir, "", library, ".dylib", file_so)))
            return file_so;
    }
    else
    {
        arch_dir = get_arch_dir( target );
        if (!strcmp( suffix, ".a" ))  /* try Mingw-style .dll.a import lib */
        {
            if ((*file = try_lib_path(dir, arch_dir, library, ".dll.a", file_arh)))
                return file_arh;
        }
    }

    /* static archives */
    if ((*file = try_lib_path(dir, arch_dir, library, suffix, file_arh)))
	return file_arh;

    return file_na;
}

static enum file_type get_lib_type(struct strarray path, const char *library, char** file)
{
    unsigned int i;

    for (i = 0; i < path.count; i++)
    {
        enum file_type type = guess_lib_type(path.str[i], library, file);
	if (type != file_na) return type;
    }
    return file_na;
}

static const char *find_binary( const char *name )
{
    char *file_name, *args;
    struct strarray dirs = empty_strarray;
    unsigned int i;

    if (strchr( name, '/' )) return name;

    file_name = xstrdup( name );
    if ((args = strchr( file_name, ' ' ))) *args++ = 0;

    strarray_addall( &dirs, prefix_dirs );
    strarray_addall( &dirs, path_dirs );
    for (i = 0; i < dirs.count; i++)
    {
        struct stat st;
        char *prog = strmake( "%s/%s%s", dirs.str[i], file_name, EXEEXT );
        if (stat( prog, &st ) == 0 && S_ISREG( st.st_mode ) && (st.st_mode & 0111))
            return args ? strmake( "%s %s", prog, args ) : prog;
        free( prog );
    }
    return NULL;
}

static int spawn(struct strarray args, int ignore_errors)
{
    int status;
    const char *cmd;

    cmd = args.str[0] = find_binary( args.str[0] );
    if (verbose) strarray_trace( args );

    if ((status = strarray_spawn( args )) && !ignore_errors)
    {
	if (status > 0) error("%s failed\n", cmd);
	else perror("winegcc");
	exit(3);
    }

    return status;
}


struct tool_names
{
    const char *base;
    const char *llvm_base;
    const char *deflt;
};

static const struct tool_names tool_cc      = { "gcc",     "clang --driver-mode=gcc", CC };
static const struct tool_names tool_cxx     = { "g++",     "clang --driver-mode=g++", CXX };
static const struct tool_names tool_cpp     = { "cpp",     "clang --driver-mode=cpp", CPP };
static const struct tool_names tool_ld      = { "ld",      "ld.lld",                  LD };
static const struct tool_names tool_objcopy = { "objcopy", "llvm-objcopy" };

static struct strarray build_tool_name( const char *target_name, struct tool_names tool )
{
    const char *path, *str;
    struct strarray ret;

    if (target_name && target_version)
        str = strmake( "%s-%s-%s", target_name, tool.base, target_version );
    else if (target_name)
        str = strmake( "%s-%s", target_name, tool.base );
    else if (target_version)
        str = strmake("%s-%s", tool.base, target_version);
    else
        str = tool.deflt && *tool.deflt ? tool.deflt : tool.base;

    if ((path = find_binary( str ))) return strarray_fromstring( path, " " );

    if (target_version)
        str = strmake( "%s-%s", tool.llvm_base, target_version );
    else
        str = tool.llvm_base;

    if (!(path = find_binary( str ))) error( "Could not find %s\n", tool.base );

    ret = strarray_fromstring( path, " " );
    if (!strncmp( tool.llvm_base, "clang", 5 ))
    {
        if (target_name)
        {
            strarray_add( &ret, "-target" );
            strarray_add( &ret, target_name );
        }
        strarray_add( &ret, "-Wno-unused-command-line-argument" );
        strarray_add( &ret, "-fuse-ld=lld" );
        if (no_default_config) strarray_add( &ret, "--no-default-config" );
    }
    return ret;
}

static struct strarray get_translator(void)
{
    switch(processor)
    {
    case proc_cpp:
        return build_tool_name( target_alias, tool_cpp );
    case proc_cc:
        return build_tool_name( target_alias, tool_cc );
    case proc_cxx:
        return build_tool_name( target_alias, tool_cxx );
    }
    assert(0);
    return empty_strarray;
}

static int try_link( struct strarray link_tool, const char *cflags )
{
    const char *in = make_temp_file( "try_link", ".c" );
    const char *out = make_temp_file( "try_link", ".out" );
    const char *err = make_temp_file( "try_link", ".err" );
    struct strarray link = empty_strarray;
    int sout = -1, serr = -1;
    int ret;

    create_file( in, 0644, "int %s(void){return 1;}\n", is_pe ? "mainCRTStartup" : "main");

    strarray_addall( &link, link_tool );
    strarray_add( &link, "-o" );
    strarray_add( &link, out );
    strarray_addall( &link, strarray_fromstring( cflags, " " ) );
    strarray_add( &link, in );

    sout = dup( fileno(stdout) );
    freopen( err, "w", stdout );
    serr = dup( fileno(stderr) );
    freopen( err, "w", stderr );
    ret = spawn( link, 1 );
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

static struct strarray get_link_args( const char *output_name )
{
    struct strarray link_args = get_translator();
    struct strarray flags = empty_strarray;

    strarray_addall( &link_args, linker_args );

    if (verbose > 1) strarray_add( &flags, "-v" );

    switch (target.platform)
    {
    case PLATFORM_APPLE:
        strarray_add( &flags, "-bundle" );
        strarray_add( &flags, "-multiply_defined" );
        strarray_add( &flags, "suppress" );
        if (image_base)
        {
            strarray_add( &flags, "-image_base" );
            strarray_add( &flags, image_base );
        }
        /* On Mac, change -s into -Wl,-x. ld's -s switch is deprecated,
         * and it doesn't work on Tiger with MH_BUNDLEs anyway */
        if (strip) strarray_add( &flags, "-Wl,-x" );
        strarray_addall( &link_args, flags );
        return link_args;

    case PLATFORM_SOLARIS:
        {
            char *mapfile = make_temp_file( output_name, ".map" );
            create_file( mapfile, 0644, "text = A%s;\ndata = A%s;\n", section_align, section_align );
            strarray_add( &flags, strmake("-Wl,-M,%s", mapfile) );
        }
        break;

    case PLATFORM_ANDROID:
        /* the Android loader requires a soname for all libraries */
        strarray_add( &flags, strmake( "-Wl,-soname,%s.so", output_name ));
        break;

    case PLATFORM_MINGW:
    case PLATFORM_CYGWIN:
        strarray_add( &link_args, "-nodefaultlibs" );
        strarray_add( &link_args, "-nostartfiles" );

        if (is_shared || is_win16_app)
        {
            strarray_add( &flags, "-shared" );
            strarray_add( &flags, "-Wl,--kill-at" );
        }
        else strarray_add( &flags, is_gui_app ? "-mwindows" : "-mconsole" );

        if (is_unicode_app) strarray_add( &flags, "-municode" );
        if (subsystem) strarray_add( &flags, strmake("-Wl,--subsystem,%s", subsystem ));

        strarray_add( &flags, "-Wl,--exclude-all-symbols" );
        strarray_add( &flags, "-Wl,--nxcompat" );
        strarray_add( &flags, "-Wl,--dynamicbase" );
        strarray_add( &flags, "-Wl,--disable-auto-image-base" );

        if (image_base) strarray_add( &flags, strmake("-Wl,--image-base,%s", image_base ));

        if (large_address_aware && target.cpu == CPU_i386)
            strarray_add( &flags, "-Wl,--large-address-aware" );

        if (entry_point)
            strarray_add( &flags, strmake( "-Wl,--entry,%s%s", target.cpu == CPU_i386 ? "_" : "", entry_point ));

        if (output_debug_file && strendswith(output_debug_file, ".pdb"))
            strarray_add(&link_args, strmake("-Wl,--pdb=%s", output_debug_file));

        if (use_build_id)
            strarray_add( &link_args, "-Wl,--build-id");

        if (output_implib)
            strarray_add(&link_args, strmake("-Wl,--out-implib,%s", output_implib));

        if (strip) strarray_add( &link_args, "-s" );

        if (!try_link( link_args, "-Wl,--file-alignment,0x1000" ))
            strarray_add( &link_args, strmake( "-Wl,--file-alignment,%s", file_align ));
        else if (!try_link( link_args, "-Wl,-Xlink=-filealign:0x1000" ))
            /* lld from llvm 10 does not support mingw style --file-alignment,
             * but it's possible to use msvc syntax */
            strarray_add( &link_args, strmake( "-Wl,-Xlink=-filealign:%s", file_align ));

        strarray_addall( &link_args, flags );
        return link_args;

    case PLATFORM_WINDOWS:
        strarray_add( &link_args, "-nodefaultlibs" );
        strarray_add( &link_args, "-nostdlib" );

        if (is_shared || is_win16_app)
        {
            strarray_add( &flags, "-shared" );
            strarray_add( &flags, "-Wl,-kill-at" );
        }
        if (is_unicode_app) strarray_add( &flags, "-municode" );
        if (nostartfiles) strarray_add( &flags, "-nostartfiles" );
        if (image_base) strarray_add( &flags, strmake("-Wl,-base:%s", image_base ));
        if (entry_point) strarray_add( &flags, strmake( "-Wl,-entry:%s", entry_point ));

        if (subsystem)
            strarray_add( &flags, strmake("-Wl,-subsystem:%s", subsystem ));
        else
            strarray_add( &flags, strmake("-Wl,-subsystem:%s", is_gui_app ? "windows" : "console" ));

        if (output_debug_file && strendswith(output_debug_file, ".pdb"))
        {
            strarray_add(&link_args, "-Wl,-debug");
            strarray_add(&link_args, strmake("-Wl,-pdb:%s", output_debug_file));
        }
        else if (strip)
            strarray_add( &link_args, "-s" );
        else
            strarray_add(&link_args, "-Wl,-debug:dwarf");

        if (use_build_id)
            strarray_add( &link_args, "-Wl,-build-id");

        if (output_implib)
            strarray_add(&link_args, strmake("-Wl,-implib:%s", output_implib));
        else
            strarray_add(&link_args, strmake("-Wl,-implib:%s", make_temp_file( output_name, ".lib" )));

        strarray_add( &link_args, strmake( "-Wl,-filealign:%s", file_align ));

        strarray_addall( &link_args, flags );
        return link_args;

    default:
        if (image_base)
        {
            if (!try_link( link_args, strmake("-Wl,-Ttext-segment=%s", image_base)) )
                strarray_add( &flags, strmake("-Wl,-Ttext-segment=%s", image_base) );
        }
        if (!try_link( link_args, "-Wl,-z,max-page-size=0x1000"))
            strarray_add( &flags, "-Wl,-z,max-page-size=0x1000");
        break;
    }

    if (use_build_id)
        strarray_add( &link_args, "-Wl,--build-id");

    /* generic Unix shared library flags */

    if (strip) strarray_add( &link_args, "-s" );
    strarray_add( &link_args, "-shared" );
    strarray_add( &link_args, "-Wl,-Bsymbolic" );
    if (!noshortwchar && target.cpu == CPU_ARM)
        strarray_add( &flags, "-Wl,--no-wchar-size-warning" );
    if (!try_link( link_args, "-Wl,-z,defs" ))
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

static char *get_lib_dir(void)
{
    const char *stdlibpath[] = { libdir, LIBDIR, "/usr/lib", "/usr/local/lib", "/lib" };
    const char *bit_suffix, *other_bit_suffix, *build_multiarch, *target_multiarch, *winecrt0;
    const char *root = sysroot ? sysroot : "";
    unsigned int i;
    struct stat st;
    size_t build_len, target_len;

    bit_suffix = get_target_ptr_size( target ) == 8 ? "64" : "32";
    other_bit_suffix = get_target_ptr_size( target ) == 8 ? "32" : "64";
    winecrt0 = strmake( "/wine%s/libwinecrt0.a", get_arch_dir( target ));
    build_multiarch = get_multiarch_dir( get_default_target() );
    target_multiarch = get_multiarch_dir( target );
    build_len = strlen( build_multiarch );
    target_len = strlen( target_multiarch );

    for (i = 0; i < ARRAY_SIZE(stdlibpath); i++)
    {
        const char *root = (i && sysroot) ? sysroot : "";
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
            if (get_default_target().cpu != target.cpu &&
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

/* add compatibility defines to make non-PE platforms more Windows-like */
static struct strarray get_compat_defines( int gcc_defs )
{
    struct strarray args = empty_strarray;

    if (processor != proc_cpp)
    {
	if (gcc_defs && !wine_objdir && !noshortwchar)
	{
            strarray_add(&args, "-fshort-wchar");
            strarray_add(&args, "-DWINE_UNICODE_NATIVE");
	}
        strarray_add(&args, "-D_REENTRANT");
        if (use_pic)
            strarray_add(&args, "-fPIC");
        else
            strarray_add(&args, "-fno-PIC");
    }

    if (get_target_ptr_size( target ) == 8)
    {
        strarray_add(&args, "-DWIN64");
        strarray_add(&args, "-D_WIN64");
        strarray_add(&args, "-D__WIN64");
        strarray_add(&args, "-D__WIN64__");
    }

    strarray_add(&args, "-DWIN32");
    strarray_add(&args, "-D_WIN32");
    strarray_add(&args, "-D__WIN32");
    strarray_add(&args, "-D__WIN32__");
    strarray_add(&args, "-D__WINNT");
    strarray_add(&args, "-D__WINNT__");

    if (gcc_defs)
    {
        switch (target.cpu)
        {
        case CPU_x86_64:
        case CPU_ARM64:
            strarray_add(&args, "-D__stdcall=__attribute__((ms_abi))");
            strarray_add(&args, "-D__cdecl=__stdcall");
            strarray_add(&args, "-D__fastcall=__stdcall");
            break;
        case CPU_i386:
            strarray_add(&args, "-D__stdcall=__attribute__((__stdcall__)) __attribute__((__force_align_arg_pointer__))");
            strarray_add(&args, "-D__cdecl=__attribute__((__cdecl__)) __attribute__((__force_align_arg_pointer__))");
            strarray_add(&args, "-D__fastcall=__attribute__((__fastcall__))");
            break;
        case CPU_ARM:
            strarray_add(&args, "-D__stdcall=__attribute__((pcs(\"aapcs-vfp\")))");
            strarray_add(&args, "-D__cdecl=__stdcall");
            strarray_add(&args, "-D__fastcall=__stdcall");
            break;
        case CPU_ARM64EC:
            break;
        }
        strarray_add(&args, "-D_stdcall=__stdcall");
        strarray_add(&args, "-D_cdecl=__cdecl");
        strarray_add(&args, "-D_fastcall=__fastcall");
	strarray_add(&args, "-D__declspec(x)=__declspec_##x");
	strarray_add(&args, "-D__declspec_align(x)=__attribute__((aligned(x)))");
	strarray_add(&args, "-D__declspec_allocate(x)=__attribute__((section(x)))");
	strarray_add(&args, "-D__declspec_deprecated=__attribute__((deprecated))");
	strarray_add(&args, "-D__declspec_dllimport=__attribute__((dllimport))");
	strarray_add(&args, "-D__declspec_dllexport=__attribute__((dllexport))");
	strarray_add(&args, "-D__declspec_naked=__attribute__((naked))");
	strarray_add(&args, "-D__declspec_noinline=__attribute__((noinline))");
	strarray_add(&args, "-D__declspec_noreturn=__attribute__((noreturn))");
	strarray_add(&args, "-D__declspec_nothrow=__attribute__((nothrow))");
	strarray_add(&args, "-D__declspec_novtable=__attribute__(())"); /* ignore it */
	strarray_add(&args, "-D__declspec_selectany=__attribute__((weak))");
	strarray_add(&args, "-D__declspec_thread=__thread");
    }

    strarray_add(&args, "-D__int8=char");
    strarray_add(&args, "-D__int16=short");
    strarray_add(&args, "-D__int32=int");
    if (get_target_ptr_size( target ) == 8)
        strarray_add(&args, "-D__int64=long");
    else
        strarray_add(&args, "-D__int64=long long");

    return args;
}

static void compile( struct strarray files, const char *output_name, int compile_only )
{
    struct strarray comp_args = get_translator();
    unsigned int i, j;
    int gcc_defs = 0;
    struct strarray gcc;
    struct strarray gpp;

    if (force_pointer_size)
        strarray_add( &comp_args, strmake("-m%u", 8 * force_pointer_size ) );
    switch(processor)
    {
	case proc_cpp: gcc_defs = 1; break;
	/* Note: if the C compiler is gcc we assume the C++ compiler is too */
	/* mixing different C and C++ compilers isn't supported in configure anyway */
	case proc_cc:
	case proc_cxx:
            gcc = build_tool_name( target_alias, tool_cc );
            gpp = build_tool_name( target_alias, tool_cxx );
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

    if (!is_pe) strarray_addall( &comp_args, get_compat_defines( gcc_defs ));

    strarray_add(&comp_args, "-D__WINE__");

    /* options we handle explicitly */
    if (compile_only)
	strarray_add(&comp_args, "-c");
    if (output_name)
    {
	strarray_add(&comp_args, "-o");
	strarray_add(&comp_args, output_name);
    }

    if (verbose > 1) strarray_add( &comp_args, "-v" );

    /* the rest of the pass-through parameters */
    strarray_addall(&comp_args, compiler_args);

    /* last, but not least, the files */
    for ( j = 0; j < files.count; j++ )
    {
        if (files.str[j][0] == '-')
        {
            /* keep -x and bare '-' (i.e. stdin) options */
            if (files.str[j][1] && files.str[j][1] != 'x') continue;
        }
	strarray_add(&comp_args, files.str[j]);
    }

    /* standard includes come last in the include search path */
    if (!wine_objdir && !nostdinc)
    {
        const char *incl_dirs[] = { INCLUDEDIR, "/usr/include", "/usr/local/include" };
        const char *root = isysroot ? isysroot : sysroot ? sysroot : "";
        const char *isystem = gcc_defs ? "-isystem" : "-I";
        const char *idirafter = gcc_defs ? "-idirafter" : "-I";

        if (use_msvcrt)
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
            if (!use_msvcrt) strarray_add( &comp_args, strmake( "%s%s", idirafter, includedir ));
        }
        for (j = 0; j < ARRAY_SIZE(incl_dirs); j++)
        {
            if (j && !strcmp( incl_dirs[0], incl_dirs[j] )) continue;
            strarray_add(&comp_args, strmake( "%s%s%s/wine/windows", isystem, root, incl_dirs[j] ));
            if (!use_msvcrt) strarray_add(&comp_args, strmake( "%s%s%s", idirafter, root, incl_dirs[j] ));
        }
    }
    else if (wine_objdir)
        strarray_add(&comp_args, strmake("-I%s/include", wine_objdir) );

    spawn(comp_args, 0);
}

static const char* compile_to_object(const char* file, const char* lang)
{
    char *output_name = make_temp_file(get_basename_noext(file), ".o");
    struct strarray files = empty_strarray;

    if (lang) strarray_add(&files, lang);
    strarray_add(&files, file);
    compile(files, output_name, 1);
    return output_name;
}

/* return the initial set of options needed to run winebuild */
static struct strarray get_winebuild_args( const char *target )
{
    const char *binary;
    struct strarray spec_args = empty_strarray;
    unsigned int i;

    if (!(binary = find_binary( winebuild ))) error( "Could not find winebuild\n" );
    strarray_add( &spec_args, binary );
    if (verbose) strarray_add( &spec_args, "-v" );
    if (keep_generated) strarray_add( &spec_args, "--save-temps" );
    if (target)
    {
        strarray_add( &spec_args, "--target" );
        strarray_add( &spec_args, target );
    }
    if (force_pointer_size)
        strarray_add(&spec_args, strmake("-m%u", 8 * force_pointer_size ));
    for (i = 0; i < prefix_dirs.count; i++)
        strarray_add( &spec_args, strmake( "-B%s", prefix_dirs.str[i] ));
    strarray_addall( &spec_args, winebuild_args );
    return spec_args;
}

static void fixup_constructors( const char *file )
{
    struct strarray args = get_winebuild_args( target_alias );

    strarray_add( &args, "--fixup-ctors" );
    strarray_add( &args, file );
    spawn( args, 0 );
}

static void make_wine_builtin( const char *file )
{
    struct strarray args = get_winebuild_args( target_alias );

    strarray_add( &args, "--builtin" );
    strarray_add( &args, file );
    spawn( args, 0 );
}

/* check if there is a static lib associated to a given dll */
static char *find_static_lib( const char *dll )
{
    char *lib = strmake("%s.a", dll);
    if (get_file_type(lib) == file_arh) return lib;
    free( lib );
    return NULL;
}

static const char *find_libgcc(void)
{
    const char *out = make_temp_file( "find_libgcc", ".out" );
    const char *err = make_temp_file( "find_libgcc", ".err" );
    struct strarray link = get_translator();
    int sout = -1, serr = -1, i;
    char *libgcc, *p;
    struct stat st;
    size_t cnt;
    int ret;

    for (i = 0; i < linker_args.count; i++)
	if (strcmp(linker_args.str[i], "--no-default-config" ))
            strarray_add( &link, linker_args.str[i] );

    strarray_add( &link, "-print-libgcc-file-name" );

    sout = dup( fileno(stdout) );
    freopen( out, "w", stdout );
    serr = dup( fileno(stderr) );
    freopen( err, "w", stderr );
    ret = spawn( link, 1 );
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
static void add_library( struct strarray lib_dirs, struct strarray *files, const char *library )
{
    char *static_lib, *fullname = 0;

    switch(get_lib_type(lib_dirs, library, &fullname))
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
static void build_spec_obj( const char *spec_file, const char *output_file,
                            const char *target_name, struct strarray files,
                            struct strarray resources, struct strarray *spec_objs )
{
    unsigned int i;
    struct strarray spec_args = get_winebuild_args( target_name );
    struct strarray tool;
    const char *spec_o_name, *output_name;

    /* get the filename from the path */
    output_name = get_basename( output_file );

    tool = build_tool_name( target_name, tool_cc );
    strarray_add( &spec_args, strmake( "--cc-cmd=%s", strarray_tostring( tool, " " )));
    if (!is_pe)
    {
        tool = build_tool_name( target_name, tool_ld );
        strarray_add( &spec_args, strmake( "--ld-cmd=%s", strarray_tostring( tool, " " )));
    }

    spec_o_name = make_temp_file(output_name, ".spec.o");
    if (!is_pe)
    {
        if (use_pic) strarray_add(&spec_args, "-fPIC");
        if (use_msvcrt) strarray_add(&spec_args, "-mno-cygwin");
        if (unwind_tables) strarray_add( &spec_args, "-fasynchronous-unwind-tables" );
    }
    strarray_add(&spec_args, is_shared ? "--dll" : "--exe");
    if (fake_module)
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

    if (!is_shared)
    {
        strarray_add(&spec_args, "--subsystem");
        strarray_add(&spec_args, is_gui_app ? "windows" : "console");
        if (large_address_aware) strarray_add( &spec_args, "--large-address-aware" );
    }

    if (target.platform == PLATFORM_WINDOWS && target.cpu == CPU_i386)
        strarray_add(&spec_args, "--safeseh");

    if (entry_point)
    {
        strarray_add(&spec_args, "--entry");
        strarray_add(&spec_args, entry_point);
    }

    if (subsystem)
    {
        strarray_add(&spec_args, "--subsystem");
        strarray_add(&spec_args, subsystem);
    }

    if (!is_pe)
    {
        for (i = 0; i < delayimports.count; i++)
            strarray_add(&spec_args, strmake("-d%s", delayimports.str[i]));
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

    spawn(spec_args, 0);
    strarray_add( spec_objs, spec_o_name );
}

/* run winebuild to generate a data-only library */
static void build_data_lib( const char *spec_file, const char *output_file, struct strarray files )
{
    unsigned int i;
    struct strarray spec_args = get_winebuild_args( target_alias );

    strarray_add(&spec_args, is_shared ? "--dll" : "--exe");
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

    spawn(spec_args, 0);
}

static void build(struct strarray input_files, const char *output)
{
    struct strarray resources = empty_strarray;
    struct strarray spec_objs = empty_strarray;
    struct strarray lib_dirs = empty_strarray;
    struct strarray files = empty_strarray;
    struct strarray link_args;
    char *output_file;
    const char *output_name, *spec_file, *lang;
    const char *libgcc = NULL;
    int generate_app_loader = 1;
    const char *crt_lib = NULL;
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

    output_file = xstrdup( output ? output : "a.out" );

    /* 'winegcc -o app xxx.exe.so' only creates the load script */
    if (input_files.count == 1 && strendswith(input_files.str[0], ".exe.so"))
    {
	create_file(output_file, 0755, app_loader_template, input_files.str[0]);
	return;
    }

    if (is_static) error("Static linking is not supported\n");

    /* generate app loader only for .exe */
    if (is_shared || is_pe || strendswith(output_file, ".so"))
	generate_app_loader = 0;

    /* normalize the filename a bit: strip .so, ensure it has proper ext */
    if (!strchr(get_basename( output_file ), '.'))
        output_file = strmake("%s.%s", output_file, is_shared ? "dll" : "exe");
    else if (strendswith(output_file, ".so"))
	output_file[strlen(output_file) - 3] = 0;
    output_file_name = is_pe ? output_file : strmake( "%s.so", output_file );

    /* get the filename from the path */
    output_name = get_basename( output_file );

    /* prepare the linking path */
    if (!wine_objdir)
    {
        char *lib_dir = get_lib_dir();
        strarray_addall( &lib_dirs, lib_path_dirs );
        strarray_add( &lib_dirs, strmake( "%s/wine%s", lib_dir, get_arch_dir( target )));
        strarray_add( &lib_dirs, lib_dir );
    }
    else
    {
        strarray_add(&lib_dirs, strmake("%s/dlls", wine_objdir));
        strarray_addall(&lib_dirs, lib_path_dirs);
    }

    /* mark the files with their appropriate type */
    spec_file = lang = 0;
    for ( j = 0; j < input_files.count; j++ )
    {
	const char* file = input_files.str[j];
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
                    if (use_msvcrt)
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
		    file = compile_to_object(file, lang);
		    strarray_add(&files, strmake("-o%s", file));
		    break;
	    }
	}
	else if (file[1] == 'l')
            add_library( lib_dirs, &files, file + 2 );
	else if (file[1] == 'x')
	    lang = file;
        else if(file[1] == 'W')
            strarray_add(&files, file);
    }

    /* add the default libraries, if needed */

    if (!wine_objdir && !nodefaultlibs)
    {
        if (is_gui_app)
	{
	    add_library(lib_dirs, &files, "shell32");
	    add_library(lib_dirs, &files, "comdlg32");
	    add_library(lib_dirs, &files, "gdi32");
	}
        add_library(lib_dirs, &files, "advapi32");
        add_library(lib_dirs, &files, "user32");
        add_library(lib_dirs, &files, "winecrt0");
        if (target.platform == PLATFORM_WINDOWS)
            add_library(lib_dirs, &files, "compiler-rt");
        if (use_msvcrt)
        {
            if (!crt_lib)
            {
                if (strncmp( output_name, "msvcr", 5 ) &&
                    strncmp( output_name, "ucrt", 4 ) &&
                    strcmp( output_name, "crtdll.dll" ))
                    add_library(lib_dirs, &files, "ucrtbase");
            }
            else strarray_add(&files, strmake("-a%s", crt_lib));
        }
        if (is_win16_app) add_library(lib_dirs, &files, "kernel");
        add_library(lib_dirs, &files, "kernel32");
        add_library(lib_dirs, &files, "ntdll");
    }

    /* set default entry point, if needed */
    if (!entry_point)
    {
        if (subsystem && !strcmp( subsystem, "native" ))
            entry_point = (is_pe && target.cpu == CPU_i386) ? "DriverEntry@8" : "DriverEntry";
        else if (use_msvcrt && !is_shared && !is_win16_app)
            entry_point = is_unicode_app ? "wmainCRTStartup" : "mainCRTStartup";
    }

    /* run winebuild to generate the .spec.o file */
    if (data_only)
    {
        build_data_lib( spec_file, output_file, files );
        return;
    }

    for (i = 0; i < files.count; i++)
	if (files.str[i][1] == 'r') strarray_add( &resources, files.str[i] );

    build_spec_obj( spec_file, output_file, target_alias, files, resources, &spec_objs );
    if (native_arch)
    {
        const char *suffix = strchr( target_alias, '-' );
        if (!suffix) suffix = "";
        build_spec_obj( spec_file, output_file, strmake( "%s%s", native_arch, suffix ),
                        files, empty_strarray, &spec_objs );
    }

    if (fake_module) return;  /* nothing else to do */

    if (is_pe && !entry_point && (is_shared || is_win16_app))
        entry_point = target.cpu == CPU_i386 ? "DllMainCRTStartup@12" : "DllMainCRTStartup";

    /* link everything together now */
    link_args = get_link_args( output_name );

    switch (target.platform)
    {
    case PLATFORM_MINGW:
    case PLATFORM_CYGWIN:
        libgcc = find_libgcc();
        if (!libgcc) libgcc = "-lgcc";
        break;
    default:
        break;
    }

    strarray_add(&link_args, "-o");
    strarray_add(&link_args, output_file_name);

    for ( j = 0; j < lib_dirs.count; j++ )
	strarray_add(&link_args, strmake("-L%s", lib_dirs.str[j]));

    strarray_addall( &link_args, spec_objs );

    if (is_pe)
    {
        for (j = 0; j < delayimports.count; j++)
        {
            if (target.platform == PLATFORM_WINDOWS)
                strarray_add(&link_args, strmake("-Wl,-delayload:%s", delayimports.str[j]));
            else
                strarray_add(&link_args, strmake("-Wl,-delayload,%s",delayimports.str[j]));
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
                if (!use_msvcrt && !lib_suffix && strchr(name, '/'))
                {
                    const char *p = get_basename( name );

                    /* don't link to ntdll or ntoskrnl in non-msvcrt mode
                     * since they export CRT functions */
                    if (!strcmp( p, "libntdll.a" )) break;
                    if (!strcmp( p, "libntoskrnl.a" )) break;
                }
		strarray_add(&link_args, name);
		break;
	    case 'W':
		strarray_add(&link_args, files.str[j]);
		break;
	}
    }

    if (!nostdlib && !is_pe)
    {
	strarray_add(&link_args, "-ldl");
	strarray_add(&link_args, "-lm");
	strarray_add(&link_args, "-lc");
    }

    if (libgcc) strarray_add(&link_args, libgcc);

    atexit( cleanup_output_files );

    spawn(link_args, 0);

    if (output_debug_file && !strendswith(output_debug_file, ".pdb"))
    {
        struct strarray tool, objcopy = build_tool_name(target_alias, tool_objcopy);

        tool = empty_strarray;
        strarray_addall( &tool, objcopy );
        strarray_add(&tool, "--only-keep-debug");
        strarray_add(&tool, output_file_name);
        strarray_add(&tool, output_debug_file);
        spawn(tool, 1);

        tool = empty_strarray;
        strarray_addall( &tool, objcopy );
        strarray_add(&tool, "--strip-debug");
        strarray_add(&tool, output_file_name);
        spawn(tool, 1);

        tool = empty_strarray;
        strarray_addall( &tool, objcopy );
        strarray_add(&tool, "--add-gnu-debuglink");
        strarray_add(&tool, output_debug_file);
        strarray_add(&tool, output_file_name);
        spawn(tool, 0);
    }

    if (output_implib && !is_pe)
    {
        struct strarray tool, implib_args;

        if (!spec_file)
            error("--out-implib requires a .spec or .def file\n");

        implib_args = get_winebuild_args( target_alias );
        tool = build_tool_name( target_alias, tool_cc );
        strarray_add( &implib_args, strmake( "--cc-cmd=%s", strarray_tostring( tool, " " )));
        tool = build_tool_name( target_alias, tool_ld );
        strarray_add( &implib_args, strmake( "--ld-cmd=%s", strarray_tostring( tool, " " )));

        strarray_add(&implib_args, "--implib");
        strarray_add(&implib_args, "-o");
        strarray_add(&implib_args, output_implib);
        strarray_add(&implib_args, "--export");
        strarray_add(&implib_args, spec_file);

        spawn(implib_args, 0);
    }

    if (!is_pe) fixup_constructors( output_file_name );
    else if (wine_builtin) make_wine_builtin( output_file_name );

    /* create the loader script */
    if (generate_app_loader)
        create_file(output_file, 0755, app_loader_template, strmake("%s.so", output_name));
}


static void forward(void)
{
    struct strarray args = get_translator();

    strarray_addall(&args, compiler_args);
    strarray_addall(&args, linker_args);
    spawn(args, 0);
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

static int is_option( struct strarray args, int i, const char *option, const char **option_arg )
{
    if (!strcmp( args.str[i], option ))
    {
        if (args.count == i) error( "option %s requires an argument\n", args.str[i] );
        *option_arg = args.str[i + 1];
        return 1;
    }
    if (!strncmp( args.str[i], option, strlen(option) ) && args.str[i][strlen(option)] == '=')
    {
        *option_arg = args.str[i] + strlen(option) + 1;
        return 1;
    }
    return 0;
}

static struct strarray read_args_from_file( const char *name )
{
    struct strarray args = empty_strarray;
    char *input_buffer = NULL, *iter, *end, *opt, *out;
    struct stat st;
    int fd;

    if ((fd = open( name, O_RDONLY | O_BINARY )) == -1) error( "Cannot open %s\n", name );
    fstat( fd, &st );
    if (st.st_size)
    {
        input_buffer = xmalloc( st.st_size + 1 );
        if (read( fd, input_buffer, st.st_size ) != st.st_size) error( "Cannot read %s\n", name );
    }
    close( fd );
    end = input_buffer + st.st_size;
    for (iter = input_buffer; iter < end; iter++)
    {
        char quote = 0;
        while (iter < end && isspace(*iter)) iter++;
        if (iter == end) break;
        opt = out = iter;
        while (iter < end && (quote || !isspace(*iter)))
        {
            if (*iter == quote)
            {
                iter++;
                quote = 0;
            }
            else if (*iter == '\'' || *iter == '"') quote = *iter++;
            else
            {
                if (*iter == '\\' && iter + 1 < end) iter++;
                *out++ = *iter++;
            }
        }
        *out = 0;
        strarray_add( &args, opt );
    }
    return args;
}

int main(int argc, char **argv)
{
    int i, c, next_is_arg = 0;
    int raw_compiler_arg, raw_linker_arg, raw_winebuild_arg;
    struct strarray args = empty_strarray;
    const char* option_arg;
    char* str;

    init_signals( exit_on_signal );
    bindir = get_bindir( argv[0] );
    libdir = get_libdir( bindir );
    includedir = get_includedir( bindir );
    target = init_argv0_target( argv[0] );
    path_dirs = strarray_frompath( getenv( "PATH" ));

    /* setup tmp file removal at exit */
    atexit(clean_temp_files);

    /* determine the processor type */
    if (strendswith(argv[0], "winecpp")) processor = proc_cpp;
    else if (strendswith(argv[0], "++")) processor = proc_cxx;

    for (i = 1; i < argc; i++)
        if (argv[i][0] == '@')
            strarray_addall( &args, read_args_from_file( argv[i] + 1 ));
        else
            strarray_add( &args, argv[i] );

    /* parse options */
    for (i = 0; i < args.count; i++)
    {
        if (args.str[i][0] == '-' && args.str[i][1])  /* option, except '-' alone is stdin, which is a file */
	{
	    /* determine if this switch is followed by a separate argument */
	    next_is_arg = 0;
	    option_arg = 0;
	    switch(args.str[i][1])
	    {
		case 'x': case 'o': case 'D': case 'U':
		case 'I': case 'A': case 'l': case 'u':
		case 'b': case 'V': case 'G': case 'L':
		case 'B': case 'R': case 'z':
		    if (args.str[i][2]) option_arg = &args.str[i][2];
		    else next_is_arg = 1;
		    break;
		case 'i':
		    next_is_arg = 1;
		    break;
		case 'a':
		    if (strcmp("-aux-info", args.str[i]) == 0)
			next_is_arg = 1;
		    if (strcmp("-arch", args.str[i]) == 0)
			next_is_arg = 1;
		    break;
		case 'X':
		    if (strcmp("-Xlinker", args.str[i]) == 0)
			next_is_arg = 1;
		    break;
		case 'M':
		    c = args.str[i][2];
		    if (c == 'F' || c == 'T' || c == 'Q')
		    {
			if (args.str[i][3]) option_arg = &args.str[i][3];
			else next_is_arg = 1;
		    }
		    break;
		case 'f':
		    if (strcmp("-framework", args.str[i]) == 0)
			next_is_arg = 1;
		    break;
                case 't':
                    next_is_arg = strcmp("-target", args.str[i]) == 0;
                    break;
		case '-':
		    next_is_arg = (strcmp("--param", args.str[i]) == 0 ||
                                   strcmp("--sysroot", args.str[i]) == 0 ||
                                   strcmp("--target", args.str[i]) == 0 ||
                                   strcmp("--wine-objdir", args.str[i]) == 0 ||
                                   strcmp("--winebuild", args.str[i]) == 0 ||
                                   strcmp("--lib-suffix", args.str[i]) == 0);
		    break;
	    }
	    if (next_is_arg)
            {
                if (i + 1 >= args.count) error("option -%c requires an argument\n", args.str[i][1]);
                option_arg = args.str[i+1];
            }

	    /* determine what options go 'as is' to the linker & the compiler */
	    raw_linker_arg = is_linker_arg(args.str[i]);
	    raw_compiler_arg = !raw_linker_arg;
            raw_winebuild_arg = 0;

	    /* do a bit of semantic analysis */
            switch (args.str[i][1])
	    {
		case 'B':
		    str = xstrdup(option_arg);
		    if (strendswith(str, "/")) str[strlen(str) - 1] = 0;
                    strarray_add(&prefix_dirs, str);
                    raw_linker_arg = 1;
		    break;
                case 'b':
                    target_alias = option_arg;
                    raw_compiler_arg = 0;
                    break;
                case 'V':
                    target_version = option_arg;
                    raw_compiler_arg = 0;
                    break;
                case 'c':        /* compile or assemble */
                    raw_compiler_arg = 0;
		    if (args.str[i][2] == 0) compile_only = true;
                    break;
                case 'S':        /* generate assembler code */
                case 'E':        /* preprocess only */
                    if (args.str[i][2] == 0) skip_link = true;
                    break;
		case 'f':
		    if (strcmp("-fno-short-wchar", args.str[i]) == 0)
                        noshortwchar = true;
		    else if (!strcmp("-fasynchronous-unwind-tables", args.str[i]))
                        unwind_tables = true;
		    else if (!strcmp("-fno-asynchronous-unwind-tables", args.str[i]))
                        unwind_tables = false;
		    else if (!strcmp("-fms-hotpatch", args.str[i]))
                        raw_linker_arg = 1;
                    else if (!strcmp("-fPIC", args.str[i]) || !strcmp("-fpic", args.str[i]))
                        use_pic = true;
                    else if (!strcmp("-fno-PIC", args.str[i]) || !strcmp("-fno-pic", args.str[i]))
                        use_pic = false;
		    break;
                case 'i':
                    if (!strcmp( "-isysroot", args.str[i] )) isysroot = args.str[i + 1];
                    break;
		case 'l':
		    strarray_add(&file_args, strmake("-l%s", option_arg));
                    raw_compiler_arg = 0;
		    break;
		case 'L':
		    strarray_add(&lib_path_dirs, option_arg);
                    raw_compiler_arg = 0;
		    break;
                case 'M':        /* map file generation */
                    skip_link = true;
                    break;
		case 'm':
		    if (strcmp("-mno-cygwin", args.str[i]) == 0)
                    {
			use_msvcrt = true;
                        raw_compiler_arg = 0;
                    }
		    if (strcmp("-mcygwin", args.str[i]) == 0)
                    {
			use_msvcrt = false;
                        raw_compiler_arg = 0;
                    }
		    else if (strcmp("-mwindows", args.str[i]) == 0)
                    {
			is_gui_app = true;
                        raw_compiler_arg = 0;
                    }
		    else if (strcmp("-mconsole", args.str[i]) == 0)
                    {
			is_gui_app = false;
                        raw_compiler_arg = 0;
                    }
		    else if (strcmp("-municode", args.str[i]) == 0)
                    {
			is_unicode_app = true;
                        raw_compiler_arg = 0;
                        raw_winebuild_arg = 1;
                    }
		    else if (strcmp("-mthreads", args.str[i]) == 0)
                    {
                        raw_compiler_arg = 0;
                    }
		    else if (strcmp("-m16", args.str[i]) == 0)
                    {
			is_win16_app = true;
                        raw_compiler_arg = 0;
                        raw_winebuild_arg = 1;
                    }
		    else if (strcmp("-m32", args.str[i]) == 0)
                    {
                        force_pointer_size = 4;
			raw_linker_arg = 1;
                    }
		    else if (strcmp("-m64", args.str[i]) == 0)
                    {
                        force_pointer_size = 8;
			raw_linker_arg = 1;
                    }
                    else if (!strcmp("-marm", args.str[i] ) || !strcmp("-mthumb", args.str[i] ))
                    {
			raw_linker_arg = 1;
                    }
                    else if (!strcmp("-marm64x", args.str[i] ))
                    {
                        raw_linker_arg = 1;
                        native_arch = "aarch64";
                    }
                    else if (!strncmp("-mcpu=", args.str[i], 6) ||
                             !strncmp("-mfpu=", args.str[i], 6) ||
                             !strncmp("-march=", args.str[i], 7))
                        raw_winebuild_arg = 1;
		    break;
                case 'n':
                    if (strcmp("-nostdinc", args.str[i]) == 0)
                        nostdinc = true;
                    else if (strcmp("-nodefaultlibs", args.str[i]) == 0)
                        nodefaultlibs = true;
                    else if (strcmp("-nostdlib", args.str[i]) == 0)
                        nostdlib = true;
                    else if (strcmp("-nostartfiles", args.str[i]) == 0)
                        nostartfiles = true;
                    break;
		case 'o':
		    output = option_arg;
                    raw_compiler_arg = 0;
		    break;
                case 'p':
                    if (strcmp("-pthread", args.str[i]) == 0)
                    {
                        raw_compiler_arg = 1;
                        raw_linker_arg = 1;
                    }
                    break;
                case 's':
                    if (strcmp("-static", args.str[i]) == 0)
			is_static = true;
		    else if(strcmp("-save-temps", args.str[i]) == 0)
			keep_generated = 1;
                    else if (strncmp("-specs=", args.str[i], 7) == 0)
                        raw_linker_arg = 1;
		    else if(strcmp("-shared", args.str[i]) == 0)
		    {
			is_shared = true;
                        raw_compiler_arg = raw_linker_arg = 0;
		    }
                    else if (strcmp("-s", args.str[i]) == 0)
                    {
                        strip = true;
                        raw_linker_arg = 0;
                    }
                    break;
                case 't':
                    if (is_option( args, i, "-target", &option_arg ))
                    {
                        target_alias = option_arg;
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
                    break;
                case 'v':
                    if (args.str[i][2] == 0)
                    {
                        verbose++;
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
                    break;
                case 'W':
                    if (strncmp("-Wl,", args.str[i], 4) == 0)
		    {
                        unsigned int j;
                        struct strarray Wl = strarray_fromstring(args.str[i] + 4, ",");
                        for (j = 0; j < Wl.count; j++)
                        {
                            if (!strcmp(Wl.str[j], "--image-base") && j < Wl.count - 1)
                            {
                                image_base = xstrdup( Wl.str[++j] );
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "--section-alignment") && j < Wl.count - 1)
                            {
                                section_align = xstrdup( Wl.str[++j] );
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "--file-alignment") && j < Wl.count - 1)
                            {
                                file_align = xstrdup( Wl.str[++j] );
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "--large-address-aware"))
                            {
                                large_address_aware = true;
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "--wine-builtin"))
                            {
                                wine_builtin = true;
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "--subsystem") && j < Wl.count - 1)
                            {
                                subsystem = xstrdup( Wl.str[++j] );
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "--entry") && j < Wl.count - 1)
                            {
                                entry_point = xstrdup( Wl.str[++j] );
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "-delayload") && j < Wl.count - 1)
                            {
                                strarray_add( &delayimports, Wl.str[++j] );
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "--debug-file") && j < Wl.count - 1)
                            {
                                output_debug_file = xstrdup( Wl.str[++j] );
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "--whole-archive") ||
                                !strcmp(Wl.str[j], "--no-whole-archive") ||
                                !strcmp(Wl.str[j], "--start-group") ||
                                !strcmp(Wl.str[j], "--end-group"))
                            {
                                strarray_add( &file_args, strmake( "-Wl,%s", Wl.str[j] ));
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "--out-implib"))
                            {
                                output_implib = xstrdup( Wl.str[++j] );
                                continue;
                            }
                            if (!strcmp( Wl.str[j], "--build-id" ))
                            {
                                use_build_id = true;
                                continue;
                            }
                            if (!strcmp(Wl.str[j], "-static")) is_static = true;
                            strarray_add(&linker_args, strmake("-Wl,%s",Wl.str[j]));
                        }
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
		    else if (strncmp("-Wb,", args.str[i], 4) == 0)
		    {
                        unsigned int j;
                        struct strarray Wb = strarray_fromstring(args.str[i] + 4, ",");
                        for (j = 0; j < Wb.count; j++)
                        {
                            if (!strcmp(Wb.str[j], "--data-only")) data_only = true;
                            if (!strcmp(Wb.str[j], "--fake-module")) fake_module = true;
                            else strarray_add( &winebuild_args, Wb.str[j] );
                        }
                        raw_compiler_arg = raw_linker_arg = 0;
		    }
                    break;
		case 'x':
		    strarray_add(&file_args, args.str[i]);
		    /* we'll pass these flags ourselves, explicitly */
                    raw_compiler_arg = raw_linker_arg = 0;
		    break;
                case '-':
                    if (strcmp("-static", args.str[i]+1) == 0)
                        is_static = true;
                    else if (!strcmp( "-no-default-config", args.str[i] + 1 ))
                    {
                        no_default_config = true;
                        raw_compiler_arg = raw_linker_arg = 1;
                    }
                    else if (is_option( args, i, "--sysroot", &option_arg ))
                    {
                        sysroot = option_arg;
                        raw_linker_arg = 1;
                    }
                    else if (is_option( args, i, "--target", &option_arg ))
                    {
                        target_alias = option_arg;
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
                    else if (is_option( args, i, "--wine-objdir", &option_arg ))
                    {
                        wine_objdir = option_arg;
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
                    else if (is_option( args, i, "--winebuild", &option_arg ))
                    {
                        winebuild = option_arg;
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
                    else if (is_option( args, i, "--lib-suffix", &option_arg ))
                    {
                        lib_suffix = option_arg;
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
                    break;
            }

	    /* put the arg into the appropriate bucket */
	    if (raw_linker_arg)
	    {
		strarray_add( &linker_args, args.str[i] );
		if (next_is_arg && (i + 1 < args.count))
		    strarray_add( &linker_args, args.str[i + 1] );
	    }
	    if (raw_compiler_arg)
	    {
		strarray_add( &compiler_args, args.str[i] );
		if (next_is_arg && (i + 1 < args.count))
		    strarray_add( &compiler_args, args.str[i + 1] );
	    }
            if (raw_winebuild_arg)
            {
                strarray_add( &winebuild_args, args.str[i] );
		if (next_is_arg && (i + 1 < args.count))
		    strarray_add( &winebuild_args, args.str[i + 1] );
            }

	    /* skip the next token if it's an argument */
	    if (next_is_arg) i++;
        }
	else
	{
	    strarray_add( &file_args, args.str[i] );
	}
    }

    if (target_alias && !parse_target( target_alias, &target ))
        error( "Invalid target specification '%s'\n", target_alias );
    if (force_pointer_size) set_target_ptr_size( &target, force_pointer_size );

    if (processor == proc_cpp) skip_link = true;

    is_pe = is_pe_target( target );
    if (is_pe) use_msvcrt = true;
    if (output && strendswith( output, ".fake" )) fake_module = true;

    if (!section_align) section_align = "0x1000";
    if (!file_align) file_align = section_align;

    if (!winebuild)
    {
        if (wine_objdir) winebuild = strmake( "%s/tools/winebuild/winebuild%s", wine_objdir, EXEEXT );
        else if (!(winebuild = getenv( "WINEBUILD" )))
        {
            if (bindir) winebuild = strmake( "%s/winebuild%s", bindir, EXEEXT );
            else winebuild = "winebuild";
        }
    }
    if (file_args.count == 0 && !fake_module) forward();
    else if (!skip_link && !compile_only) build(file_args, output);
    else compile(file_args, output, compile_only);

    output_file_name = NULL;
    output_debug_file = NULL;
    output_implib = NULL;
    return 0;
}
