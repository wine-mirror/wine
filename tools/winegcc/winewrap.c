/*
 * Wine wrapper: takes care of internal details for linking.
 *
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
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#include "utils.h"

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
    "while true; do\n"
    "  case \"$1\" in\n"
    "    --debugmsg)\n"
    "      debugmsg=\"$1 $2\"\n"
    "      shift; shift;\n"
    "      ;;\n"
    "    --dll)\n"
    "      dll=\"$1 $2\"\n"
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
    "    apppath=\"$appdir/$appname.exe.so\"\n"
    "    WINEDLLPATH=\"$appdir:$WINEDLLPATH\"\n"
    "    export WINEDLLPATH\n"
    "else\n"
    "    apppath=\"$appname.exe.so\"\n"
    "fi\n"
    "\n"
    "# determine the WINELOADER\n"
    "if [ ! -x \"$WINELOADER\" ]; then WINELOADER=\"wine\"; fi\n"
    "\n"
    "# and try to start the app\n"
    "exec \"$WINELOADER\" $debugmsg $dll -- \"$apppath\" \"$@\"\n"
;

static const char *output_name = "a.out";
static strarray *so_files, *arh_files, *dll_files, *llib_paths, *lib_paths, *obj_files;
static int keep_generated = 0;

static void rm_temp_file(const char *file)
{
    if (!keep_generated) unlink(file);
}

static void create_file(const char *name, const char *fmt, ...)
{
    va_list ap;
    FILE *file;

    if (verbose) printf("Creating file %s\n", name);
    va_start(ap, fmt);
    if ( !(file = fopen(name, "w")) )
	error ("Can not create %s.", name);
    vfprintf(file, fmt, ap);
    va_end(ap);
    fclose(file);
}

static int is_resource(const char* file)
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

static void add_lib_path(const char* path)
{
    strarray_add(lib_paths, strdup(path));
    strarray_add(llib_paths, strmake("-L%s", path));
}

/* open the file for a given name, in a specified path, with the given extension */
static char *try_path( const char *path, const char *name, const char *ext )
{
    char *fullname;
    int fd;

    fullname = strmake("%s/lib%s.%s", path, name, ext);
    if (verbose > 1) fprintf(stderr, "Try %s...", fullname);

    /* check if the file exists */
    if ((fd = open( fullname, O_RDONLY )) != -1)
    {
        close( fd );
	if (verbose > 1) fprintf(stderr, "FOUND!\n");
        return fullname;
    }
    free( fullname );
    if (verbose > 1) fprintf(stderr, "\n");
    return NULL;
}

static int identify_lib_file(const char *path, const char* library)
{
    char *lib;

    if (try_path(path, library, "so"))
    {
	/* Unix shared object */
        strarray_add(so_files, strmake("-l%s", library));
    }
    else if (try_path(path, library, "def"))
    {
	/* Windows DLL */
	strarray_add(dll_files, strmake("-l%s", library));
    }
    else if ((lib = try_path(path, library, "a")))
    {
	/* winebuild needs the full path for .a files */
        strarray_add(arh_files, lib);
    }
    else
    {
	/* failed to find the library */
	return 0;
    }
    return 1;
}
 
static void identify_lib_files(strarray *lib_files)
{
    static const char* std_paths[] = { "/usr/lib", "/usr/local/lib" };
    int i, j;

    for (i = 0; i < lib_files->size; i++)
    {
        for (j = 0; j < lib_paths->size; j++)
        {
            if (identify_lib_file(lib_paths->base[i], lib_files->base[i])) 
		break;
	}
	if (j < lib_paths->size) continue;
	for (j = 0; j < sizeof(std_paths)/sizeof(std_paths[0]); j++)
	{
            if (identify_lib_file(std_paths[i], lib_files->base[i])) 
		break;
	}
	if (j < sizeof(std_paths)/sizeof(std_paths[0])) continue;
	fprintf(stderr, "cannot find %s.{so,def,a} in library search path\n", 
		lib_files->base[i]);
    }
}

int main(int argc, char **argv)
{
    char *library = 0, *path = 0;
    int i, len, cpp = 0, no_opt = 0, gui_mode = 0;
    char *base_name, *base_file, *base_path, *app_temp_name;
    char *spec_name, *spec_c_name, *spec_o_name;
    strarray *spec_args, *comp_args, *link_args;
    strarray *lib_files;

    so_files = strarray_alloc();
    arh_files = strarray_alloc();
    dll_files = strarray_alloc();
    lib_files = strarray_alloc();
    lib_paths = strarray_alloc();
    obj_files = strarray_alloc();
    llib_paths = strarray_alloc();

    for (i = 1; i < argc; i++)
    {
	if (!no_opt && argv[i][0] == '-')
	{
	    switch (argv[i][1])
	    {
	    case 'k':
		keep_generated = 1;
		break;
	    case 'o':
		if (argv[i][2]) output_name = strdup(argv[i]+ 2);
		else if (i + 1 < argc) output_name = strdup(argv[++i]);
		else error("The -o switch takes an argument.");
		break;
	    case 'L':
		if (argv[i][2]) path = argv[i] + 2;
		else if (i + 1 < argc) path = argv[++i];
		else error("The -L switch takes an argument\n.");
		add_lib_path(path);
		break;
	    case 'l':
		if (argv[i][2]) library = argv[i] + 2;
		else if (i + 1 < argc) library = argv[++i];
		else error("The -l switch takes an argument\n.");
		strarray_add(lib_files, library);
		break;
	    case 'm':
		if (strcmp("-mgui", argv[i]) == 0) gui_mode = 1;
		else error("Unknown option %s\n", argv[i]);
		break;
            case 'v':        /* verbose */
                if (argv[i][2] == 0) verbose++;
                break;
	    case 'V':
		printf("winewrap v0.40\n");
		exit(0);
		break;
	    case 'C':
		cpp = 1;
		break;
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
	strarray_add(obj_files, strdup(argv[i]));
    }

    /* include the standard library (for eg libwine.so) and DLL paths last */
    add_lib_path(DLLDIR);
    add_lib_path(LIBDIR);

    /* link in by default the big three */
    if (gui_mode)
	strarray_add(lib_files, "gdi32");
    strarray_add(lib_files, "user32");
    strarray_add(lib_files, "kernel32");

    /* Sort out the libraries into .def's and .a's */
    identify_lib_files(lib_files);
    strarray_free(lib_files);

    app_temp_name = tempnam(0, "wapp");
    /* get base filename by removing the .exe extension, if present */ 
    base_file = strdup(output_name);
    len = strlen(base_file);
    if (len > 4 && strcmp(base_file + len - 4, ".exe") == 0)
	base_file[len - 4 ] = 0; /* remove the .exe extension */

    /* we need to get rid of the directory part to get the base name */
    if ( (base_name = strrchr(base_file, '/')) ) 
    {
	base_path = strdup(base_file);
	*strrchr(base_path, '/') = 0;
	base_name++;
    }
    else 
    {
	base_path = strdup(".");
	base_name = base_file;
    }
    
    spec_name = strmake("%s.spec", app_temp_name);
    spec_c_name = strmake("%s.c", spec_name);
    spec_o_name = strmake("%s.o", spec_name);

    /* build winebuild's argument list */
    spec_args = strarray_alloc();
    strarray_add(spec_args, "winebuild");
    strarray_add(spec_args, "-o");
    strarray_add(spec_args, spec_c_name);
    strarray_add(spec_args, "--exe");
    strarray_add(spec_args, strmake("%s.exe", base_name));
    strarray_add(spec_args, gui_mode ? "-mgui" : "-mcui");
    for (i = 0; i < llib_paths->size; i++)
	strarray_add(spec_args, llib_paths->base[i]);
    for (i = 0; i < dll_files->size; i++)
	strarray_add(spec_args, dll_files->base[i]);
    for (i = 0; i < obj_files->size; i++)
	strarray_add(spec_args, obj_files->base[i]);
    for (i = 0; i < arh_files->size; i++)
	strarray_add(spec_args, arh_files->base[i]);
    strarray_add(spec_args, NULL);

    /* run winebuild to get the .spec.c file */
    spawn(spec_args);
    strarray_free(spec_args);

    /* build gcc's argument list */
    comp_args = strarray_alloc();
    strarray_add(comp_args, "gcc");
    strarray_add(comp_args, "-fPIC");
    strarray_add(comp_args, "-o");
    strarray_add(comp_args, spec_o_name);
    strarray_add(comp_args, "-c");
    strarray_add(comp_args, spec_c_name);
    strarray_add(comp_args, NULL);

    spawn(comp_args);
    strarray_free(comp_args);
    rm_temp_file(spec_c_name);
    
    /* build ld's argument list */
    link_args = strarray_alloc();
    strarray_add(link_args, cpp ? "g++" : "gcc");
    strarray_add(link_args, "-shared");
#ifdef HAVE_LINKER_INIT_FINI
    strarray_add(link_args, "-Wl,-Bsymbolic,-z,defs,-init,__wine_spec_init,-fini,__wine_spec_fini");
#else
    strarray_add(link_args, "-Wl,-Bsymbolic,-z,defs");
#endif

    for (i = 0; i < llib_paths->size; i++)
	strarray_add(link_args, llib_paths->base[i]);
    strarray_add(link_args, "-lwine");
    strarray_add(link_args, "-lm");
    for (i = 0; i < so_files->size; i++)
	strarray_add(link_args, so_files->base[i]);

    strarray_add(link_args, "-o");
    strarray_add(link_args, strmake("%s.exe.so", base_file));
    strarray_add(link_args, spec_o_name);

    for (i = 0; i < obj_files->size; i++)
	if (!is_resource(obj_files->base[i]))
	    strarray_add(link_args, obj_files->base[i]);
    for (i = 0; i < arh_files->size; i++)
	strarray_add(link_args, arh_files->base[i]);
    strarray_add(link_args, NULL);
  
    spawn(link_args);
    strarray_free(link_args);
    rm_temp_file(spec_o_name);

    /* create the loader script */
    create_file(base_file, app_loader_script, base_name);
    chmod(base_file, 0755);

    return 0;    
} 
