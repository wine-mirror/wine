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
#include <sys/stat.h>

static char **tmp_files;
static int nb_tmp_files;
static int verbose = 0;
static int keep_generated = 0;

void error(const char *s, ...)
{
    va_list ap;
    
    va_start(ap, s);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(2);
}

char *strmake(const char *fmt, ...) 
{
    int n, size = 100;
    char *p;
    va_list ap;

    if ((p = malloc (size)) == NULL)
	error("Can not malloc %d bytes.", size);
    
    while (1) 
    {
        va_start(ap, fmt);
	n = vsnprintf (p, size, fmt, ap);
	va_end(ap);
        if (n > -1 && n < size) return p;
        size *= 2;
	if ((p = realloc (p, size)) == NULL)
	    error("Can not realloc %d bytes.", size);
    }
}

void spawn(char *const argv[])
{
    int i, status;
    
    if (verbose)
    {	
	for(i = 0; argv[i]; i++) printf("%s ", argv[i]);
	printf("\n");
    }
    if (!(status = spawnvp( _P_WAIT, argv[0], argv))) return;
    
    if (status > 0) error("%s failed.", argv[0]);
    else perror("Error:");
    exit(3);
}

int strendswith(const char *str, const char *end)
{
    int l = strlen(str);
    int m = strlen(end);
   
    return l >= m && strcmp(str + l - m, end) == 0; 
}

void clean_temp_files()
{
    int i;
    
    if (keep_generated) return;

    for (i = 0; i < nb_tmp_files; i++)
	unlink(tmp_files[i]);
}

char *get_temp_file(const char *suffix)
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
    tmp_files = realloc( tmp_files, (nb_tmp_files+1) * sizeof(*tmp_files) );
    tmp_files[nb_tmp_files++] = tmp;

    return tmp;
}

char *get_obj_file(char **argv, int n)
{
    char *tmpobj, **compargv;
    int i, j;

    if (strendswith(argv[n], ".o")) return argv[n];
    if (strendswith(argv[n], ".a")) return argv[n];
    if (strendswith(argv[n], ".res")) return argv[n];
    
    tmpobj = get_temp_file(".o");
    compargv = malloc(sizeof(char*) * (n + 10));
    i = 0;
    compargv[i++] = "winegcc";
    compargv[i++] = "-c";
    compargv[i++] = "-o";
    compargv[i++] = tmpobj;
    for (j = 1; j <= n; j++)
	if (argv[j]) compargv[i++] = argv[j];
    compargv[i] = 0;
    
    spawn(compargv);

    return tmpobj;
}


int main(int argc, char **argv)
{
    char **gcc_argv;
    int i, j;
    int linking = 1, cpp = 0, use_static_linking = 0;
    int use_stdinc = 1, use_stdlib = 1, use_msvcrt = 0, gui_app = 0;

    atexit(clean_temp_files);
    
    if (strendswith(argv[0], "++")) cpp = 1;
    
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
		case 'V':
		    printf("winegcc v0.3\n");
		    exit(0);
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

    if (use_static_linking) error("Static linking is not supported.");

    gcc_argv = malloc(sizeof(char*) * (argc + 20));

    i = 0;
    if (linking)
    {
	int has_output_name = 0;

	gcc_argv[i++] = "winewrap";
	if (gui_app) gcc_argv[i++] = "-mgui";

	if (cpp) gcc_argv[i++] = "-C";	
    	for ( j = 1 ; j < argc ; j++ ) 
    	{
	    if ( argv[j][0] == '-' )
	    {
		switch (argv[j][1])
		{
		case 'L':
		case 'o':
		    gcc_argv[i++] = argv[j];
		    argv[j] = 0;
		    if (!gcc_argv[i-1][2] && j + 1 < argc)
		    {
			gcc_argv[i++] = argv[++j];
			argv[j] = 0;
		    }
		    has_output_name = 1;
		    break;
		case 'l':
		    gcc_argv[i++] = strcmp(argv[j], "-luuid") ? argv[j] : "-lwine_uuid"; 
		    argv[j] = 0;
		    break;
		default:
		    ; /* ignore the rest */
		}
	    }
	    else
	    {
		gcc_argv[i++] = get_obj_file(argv, j);
		argv[j] = 0;
	    }

	    /* Support the a.out default name, to appease configure */
	    if (!has_output_name)
	    {
		gcc_argv[i++] = "-o";
		gcc_argv[i++] = "a.out";
	    }
	}
	if (use_stdlib && use_msvcrt) gcc_argv[i++] = "-lmsvcrt";
	if (gui_app) gcc_argv[i++] = "-lcomdlg32";
	gcc_argv[i++] = "-ladvapi32";
	gcc_argv[i++] = "-lshell32";
    }
    else
    {
	gcc_argv[i++] = cpp ? "g++" : "gcc";

	gcc_argv[i++] = "-fshort-wchar";
	gcc_argv[i++] = "-fPIC";
	if (use_stdinc)
	{
	    if (use_msvcrt)
	    {
		gcc_argv[i++] = "-I" INCLUDEDIR "/msvcrt";
	    	gcc_argv[i++] = "-D__MSVCRT__";
	    }
	    gcc_argv[i++] = "-I" INCLUDEDIR "/windows";
	}
	gcc_argv[i++] = "-DWIN32";
	gcc_argv[i++] = "-D_WIN32";
	gcc_argv[i++] = "-D__WIN32";
	gcc_argv[i++] = "-D__WIN32__";
	gcc_argv[i++] = "-D__WINNT";
	gcc_argv[i++] = "-D__WINNT__";

	gcc_argv[i++] = "-D__stdcall=__attribute__((__stdcall__))";
	gcc_argv[i++] = "-D__cdecl=__attribute__((__cdecl__))";
	gcc_argv[i++] = "-D__fastcall=__attribute__((__fastcall__))";
	gcc_argv[i++] = "-D_stdcall=__attribute__((__stdcall__))";
	gcc_argv[i++] = "-D_cdecl=__attribute__((__cdecl__))";
	gcc_argv[i++] = "-D_fastcall=__attribute__((__fastcall__))";
	gcc_argv[i++] = "-D__declspec(x)=__declspec_##x";
        gcc_argv[i++] = "-D__declspec_align(x)=__attribute__((aligned(x)))";
        gcc_argv[i++] = "-D__declspec_allocate(x)=__attribute__((section(x)))";
        gcc_argv[i++] = "-D__declspec_deprecated=__attribute__((deprecated))";
        gcc_argv[i++] = "-D__declspec_dllimport=__attribute__((dllimport))";
        gcc_argv[i++] = "-D__declspec_dllexport=__attribute__((dllexport))";
        gcc_argv[i++] = "-D__declspec_naked=__attribute__((naked))";
        gcc_argv[i++] = "-D__declspec_noinline=__attribute__((noinline))";
        gcc_argv[i++] = "-D__declspec_noreturn=__attribute__((noreturn))";
        gcc_argv[i++] = "-D__declspec_nothrow=__attribute__((nothrow))";
        gcc_argv[i++] = "-D__declspec_novtable=__attribute__(())"; /* ignore it */
        gcc_argv[i++] = "-D__declspec_selectany=__attribute__((weak))";
        gcc_argv[i++] = "-D__declspec_thread=__thread";
    
	/* Wine specific defines */
	gcc_argv[i++] = "-D__WINE__";
	gcc_argv[i++] = "-DWINE_UNICODE_NATIVE";
	gcc_argv[i++] = "-D__int8=char";
	gcc_argv[i++] = "-D__int16=short";
	gcc_argv[i++] = "-D__int32=int";
	gcc_argv[i++] = "-D__int64=long long";

    	for ( j = 1 ; j < argc ; j++ ) 
    	{
	    if (strcmp("-mno-cygwin", argv[j]) == 0)
	    	; /* ignore this option */
	    else if (strcmp("-mwindows", argv[j]) == 0)
	    	; /* ignore this option */
	    else if (strcmp("-mthreads", argv[j]) == 0)
	    	; /* ignore this option */
	    else if (strncmp("-Wl,", argv[j], 4) == 0)
		; /* do not pass linking options to compiler */
	    else if (strcmp("-s", argv[j]) == 0)
	    	; /* ignore this option */
            else
            	gcc_argv[i++] = argv[j];
    	}
    }

    gcc_argv[i] = NULL;

    spawn(gcc_argv);

    return 0;
}
