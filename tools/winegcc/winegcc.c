/*
 * MinGW wrapper: makes gcc behave like MinGW.
 *
 * Copyright 2000 Manuel Novoa III
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
 * exaustive list of options that do take parameters (potentially) 
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
 * takes no paramters. This makes "-G num" parsing ambiguous.
 * This option is synonimous to -shared, and as such we will
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
 * Please note tehat the Target Options are relevant to everything:
 *   compiler, linker, asssembler, preprocessor. 
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

static int keep_generated = 0;
static strarray *tmp_files;

struct options 
{
    enum { proc_cc = 0, proc_cpp = 1, proc_pp = 2} processor;
    int use_msvcrt;
    int nostdinc;
    int nostdlib;
    int nodefaultlibs;
    int gui_app;
    int compile_only;
    const char* output_name;
    strarray* lib_names;
    strarray* lib_dirs;
    strarray *linker_args;
    strarray *compiler_args;
    strarray* files;
};

static strarray* build_compile_cmd(struct options* opts);

static int strendswith(const char *str, const char *end)
{
    int l = strlen(str);
    int m = strlen(end);
   
    return l >= m && strcmp(str + l - m, end) == 0; 
}

static void clean_temp_files()
{
    if (!keep_generated)
    {
        int i;
	for (i = 0; i < tmp_files->size; i++)
	    unlink(tmp_files->base[i]);
    }
    strarray_free(tmp_files);
}

static char *get_temp_file(const char *suffix)
{
    char *tmp = strmake("wgcc.XXXXXX%s", suffix);
    int fd = mkstemps( tmp, strlen(suffix) );
    if (fd == -1)
    {
        /* could not create it in current directory, try in /tmp */
        free(tmp);
        tmp = strmake("/tmp/wgcc.XXXXXX%s", suffix);
        fd = mkstemps( tmp, strlen(suffix) );
        if (fd == -1) error( "could not create temp file" );
    }
    close( fd );
    strarray_add(tmp_files, tmp);

    return tmp;
}

static int is_object_file(const char* arg)
{
    return strendswith(arg, ".o") 
	|| strendswith(arg, ".a")
	|| strendswith(arg, ".res");
}

static const char *get_obj_file(const char* file, struct options* opts)
{
    strarray* compargv;
    struct options copts;

    if (is_object_file(file)) return file;
    
    /* make a copy we so don't change any of the initial stuff */
    /* a shallow copy is exactly what we want in this case */
    copts = *opts;
    copts.processor = proc_cc;
    copts.output_name = get_temp_file(".o");
    copts.compile_only = 1;
    copts.files = strarray_alloc();
    strarray_add(copts.files, file);
    compargv = build_compile_cmd(&copts);
    spawn(compargv);
    strarray_free(copts.files);
    strarray_free(compargv);

    return copts.output_name;
}

static const char* get_translator(struct options* args)
{
    const char* cc = proc_cc; /* keep compiler happy */

    switch(args->processor)
    {
	case proc_pp:  cc = "cpp"; break;
	case proc_cc:  cc = "gcc"; break;
	case proc_cpp: cc = "g++"; break;
	default: error("Unknown processor");
    }

    return cc;
}

static strarray* build_compile_cmd(struct options* opts)
{
    strarray *gcc_argv = strarray_alloc();
    int j;

    strarray_add(gcc_argv, get_translator(opts));

    if (opts->processor != proc_pp)
    {
        strarray_add(gcc_argv, "-fshort-wchar");
        strarray_add(gcc_argv, "-fPIC");
    }
    if (!opts->nostdinc)
    {
        if (opts->use_msvcrt)
        {
            strarray_add(gcc_argv, "-I" INCLUDEDIR "/msvcrt");
            strarray_add(gcc_argv, "-D__MSVCRT__");
        }
        strarray_add(gcc_argv, "-I" INCLUDEDIR "/windows");
    }
    strarray_add(gcc_argv, "-DWIN32");
    strarray_add(gcc_argv, "-D_WIN32");
    strarray_add(gcc_argv, "-D__WIN32");
    strarray_add(gcc_argv, "-D__WIN32__");
    strarray_add(gcc_argv, "-D__WINNT");
    strarray_add(gcc_argv, "-D__WINNT__");

    strarray_add(gcc_argv, "-D__stdcall=__attribute__((__stdcall__))");
    strarray_add(gcc_argv, "-D__cdecl=__attribute__((__cdecl__))");
    strarray_add(gcc_argv, "-D__fastcall=__attribute__((__fastcall__))");
    strarray_add(gcc_argv, "-D_stdcall=__attribute__((__stdcall__))");
    strarray_add(gcc_argv, "-D_cdecl=__attribute__((__cdecl__))");
    strarray_add(gcc_argv, "-D_fastcall=__attribute__((__fastcall__))");
    strarray_add(gcc_argv, "-D__declspec(x)=__declspec_##x");
    strarray_add(gcc_argv, "-D__declspec_align(x)=__attribute__((aligned(x)))");
    strarray_add(gcc_argv, "-D__declspec_allocate(x)=__attribute__((section(x)))");
    strarray_add(gcc_argv, "-D__declspec_deprecated=__attribute__((deprecated))");
    strarray_add(gcc_argv, "-D__declspec_dllimport=__attribute__((dllimport))");
    strarray_add(gcc_argv, "-D__declspec_dllexport=__attribute__((dllexport))");
    strarray_add(gcc_argv, "-D__declspec_naked=__attribute__((naked))");
    strarray_add(gcc_argv, "-D__declspec_noinline=__attribute__((noinline))");
    strarray_add(gcc_argv, "-D__declspec_noreturn=__attribute__((noreturn))");
    strarray_add(gcc_argv, "-D__declspec_nothrow=__attribute__((nothrow))");
    strarray_add(gcc_argv, "-D__declspec_novtable=__attribute__(())"); /* ignore it */
    strarray_add(gcc_argv, "-D__declspec_selectany=__attribute__((weak))");
    strarray_add(gcc_argv, "-D__declspec_thread=__thread");

    /* Wine specific defines */
    strarray_add(gcc_argv, "-D__WINE__");
    strarray_add(gcc_argv, "-DWINE_UNICODE_NATIVE");
    strarray_add(gcc_argv, "-D__int8=char");
    strarray_add(gcc_argv, "-D__int16=short");
    strarray_add(gcc_argv, "-D__int32=int");
    strarray_add(gcc_argv, "-D__int64=long long");

    /* options we handle explicitly */
    if (opts->compile_only)
	strarray_add(gcc_argv, "-c");
    if (opts->output_name)
	strarray_add(gcc_argv, strmake("-o%s", opts->output_name));

    /* the rest of the pass-through parameters */
    for ( j = 0 ; j < opts->compiler_args->size ; j++ ) 
        strarray_add(gcc_argv, opts->compiler_args->base[j]);

    /* last, but not least, the files */
    for ( j = 0; j < opts->files->size; j++ )
	strarray_add(gcc_argv, opts->files->base[j]);

    return gcc_argv;
}


static strarray* build_link_cmd(struct options* opts)
{
    strarray *gcc_argv = strarray_alloc();
    int j;

    strarray_add(gcc_argv, "winewrap");
    if (opts->gui_app) strarray_add(gcc_argv, "-mgui");

    if (opts->processor == proc_cpp) strarray_add(gcc_argv, "-C");

    if (opts->output_name)
        strarray_add(gcc_argv, strmake("-o%s", opts->output_name));
    else
        strarray_add(gcc_argv, "-oa.out");

    for ( j = 0 ; j < opts->linker_args->size ; j++ ) 
        strarray_add(gcc_argv, opts->linker_args->base[j]);

    for ( j = 0; j < opts->lib_dirs->size; j++ )
	strarray_add(gcc_argv, strmake("-L%s", opts->lib_dirs->base[j]));

    for ( j = 0; j < opts->lib_names->size; j++ )
	strarray_add(gcc_argv, strmake("-l%s", opts->lib_names->base[j]));

    if (!opts->nostdlib) 
    {
        if (opts->use_msvcrt) strarray_add(gcc_argv, "-lmsvcrt");
    }

    if (!opts->nodefaultlibs) 
    {
        if (opts->gui_app) strarray_add(gcc_argv, "-lcomdlg32");
        strarray_add(gcc_argv, "-ladvapi32");
        strarray_add(gcc_argv, "-lshell32");
    }

    for ( j = 0; j < opts->files->size; j++ )
	strarray_add(gcc_argv, get_obj_file(opts->files->base[j], opts));

    return gcc_argv;
}


static strarray* build_forward_cmd(int argc, char **argv, struct options* opts)
{
    strarray *gcc_argv = strarray_alloc();
    int j;

    strarray_add(gcc_argv, get_translator(opts));

    for( j = 1; j < argc; j++ ) 
	strarray_add(gcc_argv, argv[j]);

    return gcc_argv;
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
    strarray *gcc_argv;

    /* setup tmp file removal at exit */
    tmp_files = strarray_alloc();
    atexit(clean_temp_files);
    
    /* initialize options */
    memset(&opts, 0, sizeof(opts));
    opts.lib_names = strarray_alloc();
    opts.lib_dirs = strarray_alloc();
    opts.files = strarray_alloc();
    opts.linker_args = strarray_alloc();
    opts.compiler_args = strarray_alloc();

    /* determine the processor type */
    if (strendswith(argv[0], "winecpp")) opts.processor = proc_pp;
    else if (strendswith(argv[0], "++")) opts.processor = proc_cpp;
    
    /* parse options */
    for ( i = 1 ; i < argc ; i++ ) 
    {
        if (argv[i][0] == '-')  /* option */
	{
	    /* determine if tihs switch is followed by a separate argument */
	    option_arg = 0;
	    switch(argv[i][1])
	    {
		case 'x': case 'o': case 'D': case 'U':
		case 'I': case 'A': case 'l': case 'u':
		case 'b': case 'V': case 'G':
		    if (argv[i][2]) option_arg = argv[i] + 2;
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
			if (argv[i][3]) option_arg = argv[i] + 3;
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

	    /* do a bit of semantic analysis */
            switch (argv[i][1]) 
	    {
                case 'c':        /* compile or assemble */
		    if (argv[i][2] == 0) opts.compile_only = 1;
		    /* fall through */
                case 'S':        /* generate assembler code */
                case 'E':        /* preprocess only */
                    if (argv[i][2] == 0) linking = 0;
                    break;
		case 'l':
		    strarray_add(opts.lib_names, option_arg);
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
                    break;
                case 'v':
                    if (argv[i][2] == 0) verbose = 1;
                    break;
                case 'W':
                    if (strncmp("-Wl,", argv[i], 4) == 0)
		    {
                        if (strstr(argv[i], "-static"))
                            linking = -1;
                    }
                    break;
                case '-':
                    if (strcmp("-static", argv[i]+1) == 0)
                        linking = -1;
                    break;
            }

	    /* skip the next token if it's an argument */
	    if (next_is_arg) i++;
        }
	else
	{
	    strarray_add(opts.files, argv[i]);
	} 
    }

    if (opts.processor == proc_pp) linking = 0;
    if (linking == -1) error("Static linking is not supported.");

    if (opts.files->size == 0)
	gcc_argv = build_forward_cmd(argc, argv, &opts);
    else if (linking)
	gcc_argv = build_link_cmd(&opts);
    else
	gcc_argv = build_compile_cmd(&opts);
    spawn(gcc_argv);

    return 0;
}
