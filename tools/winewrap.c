/*
 * Wine wrapper: takes care of internal details for linking.
 *
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
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifndef WINEDLLS
#define WINEDLLS "/usr/local/lib/wine"
#endif

static const char *app_loader_script =
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
    "    apppath=\"$appdir/$appname.exe.so\"\n"
    "else\n"
    "    apppath=\"$appname.exe.so\"\n"
    "fi\n"
    "\n"
    "# determine the WINELOADER\n"
    "if [ ! -x \"$WINELOADER\" ]; then WINELOADER=\"wine\"; fi\n"
    "\n"
    "# and try to start the app\n"
    "exec \"$WINELOADER\" \"$apppath\" \"$@\"\n"
;
	
static char *output_name;
static char **lib_files, **obj_files;
static int nb_lib_files, nb_obj_files;
static int debug = 1;

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

    if ((p = malloc (size)) == NULL) return NULL;
    
    while (1) 
    {
        va_start(ap, fmt);
	n = vsnprintf (p, size, fmt, ap);
	va_end(ap);
        if (n > -1 && n < size) return p;
        size *= 2;
	if ((p = realloc (p, size)) == NULL) return NULL;
    }
}

void spawn(char *const argv[])
{
    int pid, status, i;

    if (debug)
    {	
	for(i = 0; argv[i]; i++) printf("%s ", argv[i]);
	printf("\n");
    }
    
    if ((pid = fork()) == 0) execvp(argv[0], argv);
    else if (wait(&status) > 0)
    {
        if (WEXITSTATUS(status) == 0) return;
        else error("%s failed.", argv[0]);
    }
    perror("Error:");
    exit(1);
}

int is_resource(const char* file)
{
    /* see tools/winebuild/res32.c: check_header for details */
    static const char res_sig[] = { 0,0,0,0, 32,0,0,0, 0xff,0xff, 0,0, 0xff,0xff, 0,0, 0,0,0,0, 0,0, 0,0, 0,0,0,0, 0,0,0,0 };
    char buf[sizeof(res_sig)];
    FILE *fin;
    
    if (!(fin = fopen(file, "r"))) error("Can not open %s", file);
    if (fread(buf, sizeof(buf), 1, fin) != 1) error("Truncated file %s", file);
    fclose(fin);

    return !memcmp(buf, res_sig, sizeof(buf));
}

int main(int argc, char **argv)
{
    char *library = 0, *p;
    int i, j, no_opt = 0, gui_mode = 0;
    char *spec_name, *spec_c_name, *spec_o_name;
    char **spec_args, **comp_args, **link_args;
    FILE *file;
    
    for (i = 1; i < argc; i++)
    {
	if (!no_opt && argv[i][0] == '-')
	{
	    switch (argv[i][1])
	    {
	    case 'o':
		if (argv[i][2]) output_name = strdup(argv[i]+ 2);
		else if (i + 1 < argc) output_name = strdup(argv[++i]);
		else error("The -o switch takes an argument.");
		if ( !(p = strstr(output_name, ".exe")) || *(p+4) )
		    output_name = strmake("%s.exe", output_name);
		break;
	    case 'l':
		if (argv[i][2]) library = argv[i]+ 2;
		else if (i + 1 < argc) library = argv[++i];
		else error("The -l switch takes an argument\n.");
		if (strcmp(library, "winspool") == 0) library = "winspool.drv";
		lib_files = realloc( lib_files, (nb_lib_files+1) * sizeof(*lib_files) );
		lib_files[nb_lib_files++] = strdup(library);
		break;
	    case 'm':
		if (strcmp("-mwindows", argv[i]) == 0) gui_mode = 1;
		else error("Unknown option %s\n", argv[i]);
	    case '-':
		if (argv[i][2]) error("No long option supported.");
		no_opt = 1;
		break;
	    default:
		error("Unknown option -%c", argv[i][1]);
	    }
	    continue;
	}
	
	/* it's a filename, add it to its list */
	obj_files = realloc( obj_files, (nb_obj_files+1) * sizeof(*obj_files) );
	obj_files[nb_obj_files++] = strdup(argv[i]);
    }

    spec_name = tempnam(0, 0);
    spec_c_name = strmake("%s.c", spec_name);
    spec_o_name = strmake("%s.o", spec_name);
    
    /* build winebuild's argument list */
    spec_args = malloc( (nb_lib_files + nb_obj_files + 20) * sizeof (char *) );
    j = 0;
    spec_args[j++] = "winebuild";
    spec_args[j++] = "-fPIC";
    spec_args[j++] = "-o";
    spec_args[j++] = strmake("%s.c", spec_name);
    spec_args[j++] = "--exe";
    spec_args[j++] = output_name;
    spec_args[j++] = "-m";
    spec_args[j++] = gui_mode ? "gui" : "cui";
    for (i = 0; i < nb_obj_files; i++)
	spec_args[j++] = obj_files[i];
    spec_args[j++] = "-L" WINEDLLS;
    for (i = 0; i < nb_lib_files; i++)
	spec_args[j++] = strmake("-l%s", lib_files[i]);
    spec_args[j++] = "-lmsvcrt";
    spec_args[j] = 0;

    /* build gcc's argument list */
    comp_args = malloc (20 * sizeof (char *) );
    j = 0;
    comp_args[j++] = "gcc";
    comp_args[j++] = "-fPIC";
    comp_args[j++] = "-o";
    comp_args[j++] = spec_o_name;
    comp_args[j++] = "-c";
    comp_args[j++] = spec_c_name;
    
    /* build ld's argument list */
    link_args = malloc( (nb_obj_files + 20) * sizeof (char *) );
    j = 0;
    link_args[j++] = "gcc";
    link_args[j++] = "-shared";
    link_args[j++] = "-Wl,-Bsymbolic";
    link_args[j++] = "-lwine";
    link_args[j++] = "-lm";
    link_args[j++] = "-o";
    link_args[j++] = strmake("%s.so", output_name);
    link_args[j++] = spec_o_name;
    for (i = 0; i < nb_obj_files; i++)
	if (!is_resource(obj_files[i])) link_args[j++] = obj_files[i];
    link_args[j] = 0;
    
    /* run winebuild to get the .spec.c file */
    spawn(spec_args);

    /* compile the .spec.c file */
    spawn(comp_args);
    unlink(spec_c_name);
   
    /* run gcc to link */
    spawn(link_args);
    unlink(spec_o_name);

    /* create the loader script */
    output_name[strlen(output_name) - 4] = 0; /* remove the .exe extension */
    if ( !(file = fopen(output_name, "w")) )
	error ("Can not create %s.", output_name);
    fprintf(file, app_loader_script, output_name);
    fclose(file);
    chmod(output_name, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

    return 0;    
} 
