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

static char *get_obj_file(char **argv, int n)
{
    char *tmpobj;
    strarray* compargv;
    int j;

    if (strendswith(argv[n], ".o")) return argv[n];
    if (strendswith(argv[n], ".a")) return argv[n];
    if (strendswith(argv[n], ".res")) return argv[n];
    
    tmpobj = get_temp_file(".o");

    compargv = strarray_alloc();
    strarray_add(compargv,"winegcc");
    strarray_add(compargv, "-c");
    strarray_add(compargv, "-o");
    strarray_add(compargv, tmpobj);
    for (j = 1; j <= n; j++)
	if (argv[j]) strarray_add(compargv, argv[j]);
    strarray_add(compargv, NULL);
    
    spawn(compargv);
    strarray_free(compargv);

    return tmpobj;
}


int main(int argc, char **argv)
{
    strarray *gcc_argv;
    int i, j;
    int linking = 1, cpp = 0, preprocessor = 0, use_static_linking = 0;
    int use_stdinc = 1, use_stdlib = 1, use_msvcrt = 0, gui_app = 0;

    tmp_files = strarray_alloc();
    atexit(clean_temp_files);
    
    if (strendswith(argv[0], "winecpp")) preprocessor = 1;
    else if (strendswith(argv[0], "++")) cpp = 1;
    
    for ( i = 1 ; i < argc ; i++ ) 
    {
        if (argv[i][0] == '-')  /* option */
	{
            switch (argv[i][1]) 
	    {
                case 'c':        /* compile or assemble */
                case 'S':        /* generate assembler code */
                case 'E':        /* preprocess only */
                    if (argv[i][2] == 0) linking = 0;
                    break;
                case 'M':        /* map file generation */
                    linking = 0;
                    break;
		case 'm':
		    if (strcmp("-mno-cygwin", argv[i]) == 0)
			use_msvcrt = 1;
		    else if (strcmp("-mwindows", argv[i]) == 0)
			gui_app = 1;
		    else if (strcmp("-mconsole", argv[i]) == 0)
			gui_app = 0;
		    break;
                case 'n':
                    if (strcmp("-nostdinc", argv[i]) == 0)
                        use_stdinc = 0;
                    else if (strcmp("-nodefaultlibs", argv[i]) == 0)
                        use_stdlib = 0;
                    else if (strcmp("-nostdlib", argv[i]) == 0)
                        use_stdlib = 0;
                    break;
                case 's':
                    if (strcmp("-static", argv[i]) == 0) use_static_linking = 1;
                    break;
                case 'v':        /* verbose */
                    if (argv[i][2] == 0) verbose = 1;
                    break;
                case 'W':
                    if (strncmp("-Wl,", argv[i], 4) == 0)
		    {
                        if (strstr(argv[i], "-static"))
                            use_static_linking = 1;
                    }
                    break;
                case '-':
                    if (strcmp("-static", argv[i]+1) == 0)
                        use_static_linking = 1;
                    break;
            }
        } 
    }

    if (preprocessor) linking = 0;
    if (use_static_linking) error("Static linking is not supported.");

    gcc_argv = strarray_alloc();

    if (linking)
    {
	int has_input_files = 0;

	strarray *copy_argv;

	/* we need a copy in case we decide to pass args straight to gcc
	 * and we erase some of the original parameters as we go along
	 */
	copy_argv = strarray_alloc();
	strarray_add(copy_argv, cpp ? "g++" : "gcc");
	for( j = 1; j < argc ; j++ )
	    strarray_add(copy_argv, argv[j]);

	strarray_add(gcc_argv, "winewrap");
	if (gui_app) strarray_add(gcc_argv, "-mgui");

	if (cpp) strarray_add(gcc_argv, "-C");

    	for ( j = 1 ; j < argc ; j++ ) 
    	{
	    if ( argv[j][0] == '-' )
	    {
		switch (argv[j][1])
		{
		case 'L':
		case 'o':
		    strarray_add(gcc_argv, argv[j]);
		    if (!argv[j][2] && j + 1 < argc)
		    {
			argv[j] = 0;
			strarray_add(gcc_argv, argv[++j]);
		    }
		    argv[j] = 0;
		    break;
		case 'l':
		    strarray_add(gcc_argv, argv[j]);
		    argv[j] = 0;
		    break;
		default:
		    ; /* ignore the rest */
		}
	    }
	    else
	    {
		strarray_add(gcc_argv, get_obj_file(argv, j));
		argv[j] = 0;
		has_input_files = 1;
	    }
	}

	if (has_input_files)
	{
	    if (use_stdlib && use_msvcrt) strarray_add(gcc_argv, "-lmsvcrt");
	    if (gui_app) strarray_add(gcc_argv, "-lcomdlg32");
	    strarray_add(gcc_argv, "-ladvapi32");
	    strarray_add(gcc_argv, "-lshell32");
	}
	else
	{
	    /* if we have nothing to process, just forward stuff to gcc */
	    strarray_free(gcc_argv);
	    gcc_argv = copy_argv;
	}
    }
    else
    {
        strarray_add(gcc_argv, preprocessor ? "cpp" : cpp ? "g++" : "gcc");

        if (!preprocessor)
        {
            strarray_add(gcc_argv, "-fshort-wchar");
            strarray_add(gcc_argv, "-fPIC");
        }
        if (use_stdinc)
        {
            if (use_msvcrt)
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

    	for ( j = 1 ; j < argc ; j++ ) 
    	{
	    if (strcmp("-mno-cygwin", argv[j]) == 0)
	    	; /* ignore this option */
	    else if (strcmp("-mwindows", argv[j]) == 0)
	    	; /* ignore this option */
	    else if (strcmp("-mconsole", argv[j]) == 0)
	    	; /* ignore this option */
	    else if (strcmp("-mthreads", argv[j]) == 0)
	    	; /* ignore this option */
	    else if (strncmp("-Wl,", argv[j], 4) == 0)
		; /* do not pass linking options to compiler */
	    else if (strcmp("-s", argv[j]) == 0)
	    	; /* ignore this option */
            else
            	strarray_add(gcc_argv, argv[j]);
    	}
    }

    strarray_add(gcc_argv, NULL);

    spawn(gcc_argv);

    strarray_free(gcc_argv);

    return 0;
}
