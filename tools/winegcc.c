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
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

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

int main(int argc, char **argv)
{
    char **gcc_argv;
    int i, j;
    int linking = 1, verbose = 0, cpp = 0, use_static_linking = 0;
    int use_stdinc = 1, use_stdlib = 1, use_msvcrt = 0, gui_app = 0;

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
		case 'x':
		    if (strcmp("-xc++", argv[i]) == 0) cpp = 1;
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
	gcc_argv[i++] = BINDIR "/winewrap";
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
		    break;
		case 'l':
		    gcc_argv[i++] = strcmp(argv[j], "-luuid") ? argv[j] : "-lwine_uuid"; 
		    break;
		default:
		    ; /* ignore the rest */
		}
	    }
	    else
		gcc_argv[i++] = argv[j];
	}
	if (use_stdlib && use_msvcrt) gcc_argv[i++] = "-lmsvcrt";
	if (gui_app) gcc_argv[i++] = "-lcomdlg32";
	if (gui_app) gcc_argv[i++] = "-lshell32";
	gcc_argv[i++] = "-ladvapi32";
    }
    else
    {
	gcc_argv[i++] = cpp ? "g++" : "gcc";

	gcc_argv[i++] = "-fshort-wchar";
	gcc_argv[i++] = "-fPIC";
	if (use_stdinc)
	{
	    if (use_msvcrt) gcc_argv[i++] = "-I" INCLUDEDIR "/msvcrt";
	    gcc_argv[i++] = "-I" INCLUDEDIR "/windows";
	}
	gcc_argv[i++] = "-D__WINE__";
	gcc_argv[i++] = "-D__WIN32__";
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
	    else if (strcmp("-s", argv[j]) == 0)
	    	; /* ignore this option */
            else
            	gcc_argv[i++] = argv[j];
    	}
    }

    gcc_argv[i] = NULL;

    if (verbose)
    {
	for (i = 0; gcc_argv[i]; i++) printf("%s ", gcc_argv[i]);
	printf("\n");
    }

    execvp(gcc_argv[0], gcc_argv);

    return 1;
}
