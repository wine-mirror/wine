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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
 *          -nostdlib -s  -static  -static-libgcc  -shared  -shared-libgcc
 *          -symbolic -Wl,option  -Xlinker option -u symbol
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

#include <stdio.h>
#include <stdlib.h>
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
    "while true; do\n"
    "  case \"$1\" in\n"
    "    --debugmsg)\n"
    "      debugmsg=\"$1 $2\"\n"
    "      shift; shift;\n"
    "      ;;\n"
    "    *)\n"
    "      break\n"
    "      ;;\n"
    "  esac\n"
    "done\n"
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
    "exec \"$WINELOADER\" $debugmsg -- \"$apppath\" \"$@\"\n"
;

static int keep_generated = 0;
static strarray* tmp_files;

struct options 
{
    enum { proc_cc = 0, proc_cxx = 1, proc_cpp = 2} processor;
    int shared;
    int use_msvcrt;
    int nostdinc;
    int nostdlib;
    int nodefaultlibs;
    int noshortwchar;
    int gui_app;
    int compile_only;
    int wine_mode;
    const char* output_name;
    strarray* prefix;
    strarray* lib_dirs;
    strarray* linker_args;
    strarray* compiler_args;
    strarray* winebuild_args;
    strarray* files;
};

static void clean_temp_files()
{
    int i;

    if (keep_generated) return;

    for (i = 0; i < tmp_files->size; i++)
	unlink(tmp_files->base[i]);
}

char* get_temp_file(const char* prefix, const char* suffix)
{
    char* tmp = strmake("%s-XXXXXX%s", prefix, suffix);
    int fd = mkstemps( tmp, strlen(suffix) );
    if (fd == -1)
    {
        /* could not create it in current directory, try in /tmp */
        free(tmp);
        tmp = strmake("/tmp/%s-XXXXXX%s", prefix, suffix);
        fd = mkstemps( tmp, strlen(suffix) );
        if (fd == -1) error( "could not create temp file" );
    }
    close( fd );
    strarray_add(tmp_files, tmp);

    return tmp;
}

static const strarray* get_translator(struct options* opts)
{
    static strarray* cpp = 0;
    static strarray* cc = 0;
    static strarray* cxx = 0;

    switch(opts->processor)
    {
        case proc_cpp: 
	    if (!cpp) cpp = strarray_fromstring(CPP, " ");
	    return cpp;
        case proc_cc:  
	    if (!cc) cc = strarray_fromstring(CC, " ");
	    return cc;
        case proc_cxx: 
	    if (!cxx) cxx = strarray_fromstring(CXX, " ");
	    return cxx;
    }
    error("Unknown processor");
}

static void compile(struct options* opts)
{
    strarray* comp_args = strarray_alloc();
    int j, gcc_defs = 0;

    switch(opts->processor)
    {
	case proc_cpp:  gcc_defs = 1; break;
#ifdef __GNUC__
	/* Note: if the C compiler is gcc we assume the C++ compiler is too */
	/* mixing different C and C++ compilers isn't supported in configure anyway */
	case proc_cc:  gcc_defs = 1; break;
	case proc_cxx: gcc_defs = 1; break;
#else
	case proc_cc:  gcc_defs = 0; break;
	case proc_cxx: gcc_defs = 0; break;
#endif
    }
    strarray_addall(comp_args, get_translator(opts));

    if (opts->processor != proc_cpp)
    {
#ifdef CC_FLAG_SHORT_WCHAR
	if (!opts->wine_mode && !opts->noshortwchar)
	{
            strarray_add(comp_args, CC_FLAG_SHORT_WCHAR);
            strarray_add(comp_args, "-DWINE_UNICODE_NATIVE");
	}
#endif
        strarray_addall(comp_args, strarray_fromstring(DLLFLAGS, " "));
    }
    if (!opts->wine_mode && !opts->nostdinc)
    {
        if (opts->use_msvcrt)
        {
            strarray_add(comp_args, "-I" INCLUDEDIR "/msvcrt");
            strarray_add(comp_args, "-D__MSVCRT__");
        }
        strarray_add(comp_args, "-I" INCLUDEDIR "/windows");
    }
    strarray_add(comp_args, "-DWIN32");
    strarray_add(comp_args, "-D_WIN32");
    strarray_add(comp_args, "-D__WIN32");
    strarray_add(comp_args, "-D__WIN32__");
    strarray_add(comp_args, "-D__WINNT");
    strarray_add(comp_args, "-D__WINNT__");

    if (gcc_defs)
    {
	strarray_add(comp_args, "-D__stdcall=__attribute__((__stdcall__))");
	strarray_add(comp_args, "-D__cdecl=__attribute__((__cdecl__))");
	strarray_add(comp_args, "-D__fastcall=__attribute__((__fastcall__))");
	strarray_add(comp_args, "-D_stdcall=__attribute__((__stdcall__))");
	strarray_add(comp_args, "-D_cdecl=__attribute__((__cdecl__))");
	strarray_add(comp_args, "-D_fastcall=__attribute__((__fastcall__))");
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

    /* Wine specific defines */
    strarray_add(comp_args, "-D__WINE__");
    strarray_add(comp_args, "-D__int8=char");
    strarray_add(comp_args, "-D__int16=short");
    /* FIXME: what about 64-bit platforms? */
    strarray_add(comp_args, "-D__int32=int");
#ifdef HAVE_LONG_LONG
    strarray_add(comp_args, "-D__int64=long long");
#endif

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

    /* last, but not least, the files */
    for ( j = 0; j < opts->files->size; j++ )
	strarray_add(comp_args, opts->files->base[j]);

    spawn(opts->prefix, comp_args);
}

static const char* compile_to_object(struct options* opts, const char* file)
{
    struct options copts;
    char* base_name;

    /* make a copy we so don't change any of the initial stuff */
    /* a shallow copy is exactly what we want in this case */
    base_name = get_basename(file);
    copts = *opts;
    copts.processor = proc_cc;
    copts.output_name = get_temp_file(base_name, ".o");
    copts.compile_only = 1;
    copts.files = strarray_alloc();
    strarray_add(copts.files, file);
    compile(&copts);
    strarray_free(copts.files);
    free(base_name);

    return copts.output_name;
}

static void build(struct options* opts)
{
    static const char *stdlibpath[] = { DLLDIR, LIBDIR, "/usr/lib", "/usr/local/lib", "/lib" };
    strarray *lib_dirs, *files;
    strarray *spec_args, *comp_args, *link_args;
    char *output_file;
    const char *spec_c_name, *spec_o_name;
    const char *output_name, *spec_file;
    const char* winebuild = getenv("WINEBUILD");
    int generate_app_loader = 1;
    int j;

    /* NOTE: for the files array we'll use the following convention:
     *    -axxx:  xxx is an archive (.a)
     *    -dxxx:  xxx is a DLL (.def)
     *    -lxxx:  xxx is an unsorted library
     *    -oxxx:  xxx is an object (.o)
     *    -rxxx:  xxx is a resource (.res)
     *    -sxxx:  xxx is a shared lib (.so)
     */

    if (!winebuild) winebuild = "winebuild";

    output_file = strdup( opts->output_name ? opts->output_name : "a.out" );

    /* 'winegcc -o app xxx.exe.so' only creates the load script */
    if (opts->files->size == 1 && strendswith(opts->files->base[0], ".exe.so"))
    {
	create_file(output_file, 0755, app_loader_template, opts->files->base[0]);
	return;
    }

    /* generate app loader only for .exe */
    if (opts->shared || strendswith(output_file, ".exe.so"))
	generate_app_loader = 0;

    /* normalize the filename a bit: strip .so, ensure it has proper ext */
    if (strendswith(output_file, ".so")) 
	output_file[strlen(output_file) - 3] = 0;
    if (opts->shared)
    {
	if ((output_name = strrchr(output_file, '/'))) output_name++;
	else output_name = output_file;
	if (!strchr(output_name, '.'))
	    output_file = strmake("%s.dll", output_file);
    }
    else if (!strendswith(output_file, ".exe"))
	output_file = strmake("%s.exe", output_file);

    /* get the filename by the path, if present */
    if ((output_name = strrchr(output_file, '/'))) output_name++;
    else output_name = output_file;

    /* prepare the linking path */
    lib_dirs = strarray_dup(opts->lib_dirs);
    if (!opts->wine_mode)
    {
	for ( j = 0; j < sizeof(stdlibpath)/sizeof(stdlibpath[0]); j++ )
	    strarray_add(lib_dirs, stdlibpath[j]);
    }

    /* mark the files with their appropriate type */
    spec_file = 0;
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
		    if (!opts->shared) 
		        error("Spec file %s not supported in non-shared mode", file);
		    if (spec_file)
			error("Only one spec file can be specified in shared mode");
		    spec_file = file;
		    break;
		case file_rc:
		    /* FIXME: invoke wrc to build it */
		    error("Can't compile .rc file at the moment: %s", file);
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
		    error("File does not exist: %s", file);
		    break;
	        default:
		    file = compile_to_object(opts, file);
		    strarray_add(files, strmake("-o%s", file));
		    break;
	    }
	}
	else if (file[1] == 'l')
	{
	    char* fullname = 0;
	    switch(get_lib_type(lib_dirs, file + 2, &fullname))
	    {
	    	case file_arh:
		    strarray_add(files, strmake("-a%s", fullname));
		    break;
	        case file_dll:
		    strarray_add(files, strmake("-d%s", file + 2));
		    break;
	        case file_so:
		    strarray_add(files, strmake("-s%s", file + 2));
		    break;
	        default:
                    /* keep it anyway, the linker may know what to do with it */
                    strarray_add(files, file);
                    break;
	    }
	    free(fullname);
	}
    }
    if (opts->shared && !spec_file)
	error("A spec file is currently needed in shared mode");

    /* add the default libraries, if needed */
    if (!opts->nostdlib) 
    {
        if (opts->use_msvcrt) strarray_add(files, "-dmsvcrt");
    }

    if (!opts->wine_mode && !opts->nodefaultlibs) 
    {
        if (opts->gui_app) 
	{
            strarray_add(files, "-dshell32");
	    strarray_add(files, "-dcomdlg32");
	    strarray_add(files, "-dgdi32");
	}
        strarray_add(files, "-dadvapi32");
        strarray_add(files, "-duser32");
        strarray_add(files, "-dkernel32");
    }

    /* run winebuild to generate the .spec.c file */
    spec_args = strarray_alloc();
    spec_c_name = get_temp_file(output_name, ".spec.c");
    strarray_add(spec_args, winebuild);
    strarray_add(spec_args, "-o");
    strarray_add(spec_args, spec_c_name);
    if (opts->shared)
    {
        strarray_add(spec_args, "--dll");
        strarray_add(spec_args, spec_file);
    }
    else
    {
        strarray_add(spec_args, "--exe");
        strarray_add(spec_args, output_name);
        strarray_add(spec_args, "--subsystem");
        strarray_add(spec_args, opts->gui_app ? "windows" : "console");
    }

    for ( j = 0; j < lib_dirs->size; j++ )
	strarray_add(spec_args, strmake("-L%s", lib_dirs->base[j]));

    for ( j = 0 ; j < opts->winebuild_args->size ; j++ )
        strarray_add(spec_args, opts->winebuild_args->base[j]);

    for ( j = 0; j < files->size; j++ )
    {
	const char* name = files->base[j] + 2;
	switch(files->base[j][1])
	{
	    case 'd':
		strarray_add(spec_args, strmake("-l%s", name));
		break;
	    case 'r':
		strarray_add(spec_args, files->base[j]);
		break;
	    case 'a':
	    case 'o':
		strarray_add(spec_args, name);
		break;
	}
    }

    spawn(opts->prefix, spec_args);

    /* compile the .spec.c file into a .spec.o file */
    comp_args = strarray_alloc();
    spec_o_name = compile_to_object(opts, spec_c_name);
    
    /* link everything together now */
    link_args = strarray_alloc();
    strarray_addall(link_args, get_translator(opts));
    strarray_addall(link_args, strarray_fromstring(LDDLLFLAGS, " "));

    strarray_add(link_args, "-o");
    strarray_add(link_args, strmake("%s.so", output_file));

    for ( j = 0 ; j < opts->linker_args->size ; j++ ) 
        strarray_add(link_args, opts->linker_args->base[j]);

    for ( j = 0; j < lib_dirs->size; j++ )
	strarray_add(link_args, strmake("-L%s", lib_dirs->base[j]));

    strarray_add(link_args, spec_o_name);

    for ( j = 0; j < files->size; j++ )
    {
	const char* name = files->base[j] + 2;
	switch(files->base[j][1])
	{
	    case 'l':
	    case 's':
		strarray_add(link_args, strmake("-l%s", name));
		break;
	    case 'a':
	    case 'o':
		strarray_add(link_args, name);
		break;
	}
    }

    if (!opts->nostdlib) 
    {
	strarray_add(link_args, "-lwine");
	strarray_add(link_args, "-lm");
	strarray_add(link_args, "-lc");
    }

    spawn(opts->prefix, link_args);

    /* create the loader script */
    if (generate_app_loader)
    {
        if (strendswith(output_file, ".exe")) output_file[strlen(output_file) - 4] = 0;
        create_file(output_file, 0755, app_loader_template, strmake("%s.so", output_name));
    }
}


static void forward(int argc, char **argv, struct options* opts)
{
    strarray* args = strarray_alloc();
    int j;

    strarray_addall(args, get_translator(opts));

    for( j = 1; j < argc; j++ ) 
	strarray_add(args, argv[j]);

    spawn(opts->prefix, args);
}

/*
 *      Linker Options
 *          object-file-name  -llibrary -nostartfiles  -nodefaultlibs
 *          -nostdlib -s  -static  -static-libgcc  -shared  -shared-libgcc
 *          -symbolic -Wl,option  -Xlinker option -u symbol
 */
static int is_linker_arg(const char* arg)
{
    static const char* link_switches[] = 
    {
	"-nostartfiles", "-nodefaultlibs", "-nostdlib", "-s", 
	"-static", "-static-libgcc", "-shared", "-shared-libgcc", "-symbolic"
    };
    int j;

    switch (arg[1]) 
    {
	case 'l': 
	case 'u':
	    return 1;
        case 'W':
            if (strncmp("-Wl,", arg, 4) == 0) return 1;
	    break;
	case 'X':
	    if (strcmp("-Xlinker", arg) == 0) return 1;
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
    return arg[1] == 'b' || arg[2] == 'V';
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
 *	    -mno-cygwin -mwindows -mconsole -mthreads
 */ 
static int is_mingw_arg(const char* arg)
{
    static const char* mingw_switches[] = 
    {
	"-mno-cygwin", "-mwindows", "-mconsole", "-mthreads"
    };
    int j;

    for (j = 0; j < sizeof(mingw_switches)/sizeof(mingw_switches[0]); j++)
	if (strcmp(mingw_switches[j], arg) == 0) return 1;

    return 0;
}

int main(int argc, char **argv)
{
    int i, c, next_is_arg = 0, linking = 1;
    int raw_compiler_arg, raw_linker_arg;
    const char* option_arg;
    struct options opts;
    char* str;

    /* setup tmp file removal at exit */
    tmp_files = strarray_alloc();
    atexit(clean_temp_files);
    
    /* initialize options */
    memset(&opts, 0, sizeof(opts));
    opts.lib_dirs = strarray_alloc();
    opts.files = strarray_alloc();
    opts.linker_args = strarray_alloc();
    opts.compiler_args = strarray_alloc();
    opts.winebuild_args = strarray_alloc();

    /* determine the processor type */
    if (strendswith(argv[0], "winecpp")) opts.processor = proc_cpp;
    else if (strendswith(argv[0], "++")) opts.processor = proc_cxx;
    
    /* parse options */
    for ( i = 1 ; i < argc ; i++ ) 
    {
        if (argv[i][0] == '-')  /* option */
	{
	    /* determine if tihs switch is followed by a separate argument */
	    next_is_arg = 0;
	    option_arg = 0;
	    switch(argv[i][1])
	    {
		case 'x': case 'o': case 'D': case 'U':
		case 'I': case 'A': case 'l': case 'u':
		case 'b': case 'V': case 'G': case 'L':
		case 'B':
		    if (argv[i][2]) option_arg = &argv[i][2];
		    else next_is_arg = 1;
		    break;
		case 'i':
		    next_is_arg = 1;
		    break;
		case 'a':
		    if (strcmp("-aux-info", argv[i]) == 0)
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
	    }
	    if (next_is_arg) option_arg = argv[i+1];

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
	    if (argv[i][1] == 'o')
		raw_compiler_arg = raw_linker_arg = 0;

	    /* do a bit of semantic analysis */
            switch (argv[i][1]) 
	    {
		case 'B':
		    str = strdup(option_arg);
		    if (strendswith(str, "/tools/winebuild"))
                    {
                        opts.wine_mode = 1;
                        /* don't pass it to the compiler, this generates warnings */
                        raw_compiler_arg = raw_linker_arg = 0;
                    }
		    if (strendswith(str, "/")) str[strlen(str) - 1] = 0;
                    if (!opts.prefix) opts.prefix = strarray_alloc();
                    strarray_add(opts.prefix, str);
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
		    break;
                case 'n':
                    if (strcmp("-nostdinc", argv[i]) == 0)
                        opts.nostdinc = 1;
                    else if (strcmp("-nodefaultlibs", argv[i]) == 0)
                        opts.nodefaultlibs = 1;
                    else if (strcmp("-nostdlib", argv[i]) == 0)
                        opts.nostdlib = 1;
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
                    break;
                case 'v':
                    if (argv[i][2] == 0) verbose++;
                    break;
                case 'W':
                    if (strncmp("-Wl,", argv[i], 4) == 0)
		    {
                        if (strstr(argv[i], "-static"))
                            linking = -1;
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
                case '-':
                    if (strcmp("-static", argv[i]+1) == 0)
                        linking = -1;
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
    if (linking == -1) error("Static linking is not supported.");

    if (opts.files->size == 0) forward(argc, argv, &opts);
    else if (linking) build(&opts);
    else compile(&opts);

    return 0;
}
