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
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
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

static const char *app_gui_spec =
    "@ stdcall WinMain(ptr long ptr long) WinMain\n"
;

static const char *app_cui_spec =
    "@ stdcall main(long ptr ptr) main\n"
;

static const char *wrapper_code =
    "/*\n"
    " * Copyright 2000 Francois Gouget <fgouget@codeweavers.com> for CodeWeavers\n"
    " * Copyright 2002 Dimitrie O. Paun <dpaun@rogers.com>\n"
    " */\n"
    "\n"
    "#include <dlfcn.h>\n"
    "#include <windows.h>\n"
    "\n"
    "\n"
    "/*\n"
    " * Describe the wrapped application\n"
    " */\n"
    "\n"
    "/* The app name */\n"
    "#define APPNAME \"%s\"\n"
    "/**\n"
    " * This is either 0 for a console based application or\n"
    " * 1 for a regular windows application.\n"
    " */\n"
    "#define GUIEXE %d\n"
    "\n"
    "/**\n"
    " * This is the name of the library containing the application,\n"
    " * e.g. 'hello.dll' if the application is called 'hello.exe'.\n"
    " */\n"
    "static char* appName     = APPNAME \".dll\";\n"
    "\n"
    "/**\n"
    " * This is the name of the application's Windows module. If left NULL\n"
    " * then appName is used.\n"
    " */\n"
    "static char* appModule   = NULL;\n"
    "\n"
    "/**\n"
    " * This is the application's entry point. This is usually 'WinMain' for a\n"
    " * gui app and 'main' for a console application.\n"
    " */\n"
    "#if GUIEXE\n"
    "static char* appInit     = \"WinMain\";\n"
    "#else\n"
    "static char* appInit     = \"main\";\n"
    "#endif\n"
    "\n"
    "/**\n"
    " * This is either non-NULL for MFC-based applications and is the name of the\n"
    " * MFC's module. This is the module in which we will take the 'WinMain'\n"
    " * function.\n"
    " */\n"
    "static char* mfcModule   = NULL;\n"
    "\n"
    "\n"
    "void error(const char *format, ...)\n"
    "{\n"
    "    va_list ap;\n"
    "    char msg[4096];\n"
    "\n"
    "    va_start(ap, format);\n"
    "    vsnprintf(msg, sizeof(msg), format, ap);\n"
    "    MessageBox(NULL, msg, \"Error\", MB_OK);\n"
    "    va_end(ap);\n"
    "    exit(1);\n"
    "}\n"
    "\n"
    "\n"
    "#if GUIEXE\n"
    "typedef int WINAPI (*WinMainFunc)(HINSTANCE hInstance, HINSTANCE hPrevInstance,\n"
    "				  PSTR szCmdLine, int iCmdShow);\n"
    "#else\n"
    "typedef int WINAPI (*MainFunc)(int argc, char** argv, char** envp);\n"
    "#endif\n"
    "\n"
    "#if GUIEXE\n"
    "int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,\n"
    "                   PSTR szCmdLine, int iCmdShow)\n"
    "#else\n"
    "int WINAPI main(int argc, char** argv, char** envp)\n"
    "#endif\n"
    "{\n"
    "    void* appLibrary;\n"
    "    HINSTANCE hApp = 0, hMFC = 0, hMain = 0;\n"
    "    void* appMain;\n"
    "    char* libName;\n"
    "    int retcode;\n"
    "\n"
    "    /* Load the application's library */\n"
    "    libName=(char*)malloc(2+strlen(appName)+3+1);\n"
    "    /* FIXME: we should get the wrapper's path and use that as the base for\n"
    "     * the library\n"
    "     */\n"
    "    sprintf(libName,\"./%%s.so\",appName);\n"
    "    appLibrary=dlopen(libName,RTLD_NOW);\n"
    "    if (appLibrary==NULL) {\n"
    "        sprintf(libName,\"%%s.so\",appName);\n"
    "        appLibrary=dlopen(libName,RTLD_NOW);\n"
    "    }\n"
    "    if (!appLibrary) error(\"Could not load the %%s library: %%s\", libName, dlerror());\n"
    "\n"
    "    /* Then if this application is MFC based, load the MFC module */\n"
    "    /* FIXME: I'm not sure this is really necessary */\n"
    "    if (mfcModule) {\n"
    "        hMFC = LoadLibrary(mfcModule);\n"
    "        if (!hMFC) error(\"Could not load the MFC module %%s (%%d)\", mfcModule, GetLastError());\n"
    "        /* MFC is a special case: the WinMain is in the MFC library,\n"
    "         * instead of the application's library.\n"
    "         */\n"
    "        hMain = hMFC;\n"
    "    }\n"
    "\n"
    "    /* Load the application's module */\n"
    "    if (!appModule) appModule = appName;\n"
    "    hApp = LoadLibrary(appModule);\n"
    "    if (!hApp) error(\"Could not load the application's module %%s (%%d)\", appModule, GetLastError());\n"
    "    if (!hMain) hMain = hApp;\n"
    "\n"
    "    /* Get the address of the application's entry point */\n"
    "    appMain = GetProcAddress(hMain, appInit);\n"
    "    if (!appMain) error(\"Could not get the address of %%s (%%d)\", appInit, GetLastError());\n"
    "\n"
    "    /* And finally invoke the application's entry point */\n"
    "#if GUIEXE\n"
    "    retcode = (*((WinMainFunc)appMain))(hApp, hPrevInstance, szCmdLine, iCmdShow);\n"
    "#else\n"
    "    retcode = (*((MainFunc)appMain))(argc, argv, envp);\n"
    "#endif\n"
    "\n"
    "    /* Cleanup and done */\n"
    "    FreeLibrary(hApp);\n"
    "    FreeLibrary(hMFC);\n"
    "    dlclose(appLibrary);\n"
    "    free(libName);\n"
    "\n"
    "    return retcode;\n"
    "}\n"
;

static char *output_name;
static char **arh_files,  **dll_files,  **lib_files,  **lib_paths,  **obj_files;
static int nb_arh_files, nb_dll_files, nb_lib_files, nb_lib_paths, nb_obj_files;
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

void rm_temp_file(const char *file)
{
    if (!keep_generated) unlink(file);
}

void create_file(const char *name, const char *fmt, ...)
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

void spawn(char *const argv[])
{
    int pid, status, wret, i;

    if (verbose)
    {	
	for(i = 0; argv[i]; i++) printf("%s ", argv[i]);
	printf("\n");
    }
    
    if ((pid = fork()) == 0) execvp(argv[0], argv);
    else if (pid > 0)
    {
	while (pid != (wret = waitpid(pid, &status, 0)))
	    if (wret == -1 && errno != EINTR) break;
	
        if (pid == wret && WIFEXITED(status) && WEXITSTATUS(status) == 0) return;
        error("%s failed.", argv[0]);
    }
    perror("Error:");
    exit(3);
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

/* open the .def library for a given dll in a specified path */
static char *try_dll_path( const char *path, const char *name )
{
    return try_path(path, name, "def");
}

/* open the .a library for a given lib in a specified path */
static char *try_lib_path( const char *path, const char *name )
{
    return try_path(path, name, "a");
}

/* open the .def library for a given dll */
static char *open_dll(const char *name)
{
    char *fullname;
    int i;

    for (i = 0; i < nb_lib_paths; i++)
    {
        if ((fullname = try_dll_path( lib_paths[i], name ))) return fullname;
    }
    return try_dll_path( ".", name );
}

/* find a static library */
static char *find_lib(const char *name)
{
    static const char* std_paths[] = { ".", "/usr/lib", "/usr/local/lib" };
    char *fullname;
    int i;
    
    for (i = 0; i < nb_lib_paths; i++)
    {
        if ((fullname = try_lib_path( lib_paths[i], name ))) return fullname;
    }

    for (i = 0; i < sizeof(std_paths)/sizeof(std_paths[0]); i++)
    {
        if ((fullname = try_lib_path( std_paths[i], name ))) return fullname;
    }

    return 0;
}

void add_lib_path(const char* path)
{
    lib_paths = realloc( lib_paths, (nb_lib_paths+1) * sizeof(*lib_paths) );
    lib_paths[nb_lib_paths++] = strdup(path);
    dll_files = realloc( dll_files, (nb_dll_files+1) * sizeof(*dll_files) );
    dll_files[nb_dll_files++] = strmake("-L%s", path);
    lib_files = realloc( lib_files, (nb_lib_files+1) * sizeof(*lib_files) );
    lib_files[nb_lib_files++] = strmake("-L%s", path);
}

void add_lib_file(const char* library)
{
    char *lib;
    
    if (open_dll(library))
    {
        dll_files = realloc( dll_files, (nb_dll_files+1) * sizeof(*dll_files) );
        dll_files[nb_dll_files++] = strmake("-l%s", library);
    }
    else if ((lib = find_lib(library)))
    {
        arh_files = realloc( arh_files, (nb_arh_files+1) * sizeof(*arh_files) );
        arh_files[nb_arh_files++] = lib;
    }
    else
    {
        lib_files = realloc( lib_files, (nb_lib_files+1) * sizeof(*lib_files) );
        lib_files[nb_lib_files++] = strmake("-l%s", library);
    }
}

int main(int argc, char **argv)
{
    char *library = 0, *path = 0;
    int i, j, len, cpp = 0, no_opt = 0, gui_mode = 0, create_wrapper = -1;
    char *base_name, *base_file, *app_temp_name, *wrp_temp_name;
    char *spec_name, *spec_c_name, *spec_o_name;
    char *wspec_name, *wspec_c_name, *wspec_o_name;
    char *wrap_c_name, *wrap_o_name;
    char **spec_args, **comp_args, **link_args;
    char **wwrap_args, **wspec_args, **wcomp_args, **wlink_args;
   
    /* include the standard DLL path first */
    add_lib_path(WINEDLLS);
	
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
		add_lib_file(library);
		break;
	    case 'm':
		if (strcmp("-mwindows", argv[i]) == 0) gui_mode = 1;
		else if (strcmp("-mgui", argv[i]) == 0) gui_mode = 1;
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
	    case 'w':
		create_wrapper = 1;
		break;
	    case 'W':
		create_wrapper = 0;
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
	obj_files = realloc( obj_files, (nb_obj_files+1) * sizeof(*obj_files) );
	obj_files[nb_obj_files++] = strdup(argv[i]);
    }

    /* create wrapper only in C++ by default */
    if (create_wrapper == -1) create_wrapper = cpp;
    
    /* link in by default the big three */
    add_lib_file("user32");
    add_lib_file("gdi32");
    add_lib_file("kernel32");

    app_temp_name = tempnam(0, "wapp");
    wrp_temp_name = tempnam(0, "wwrp");
   
    /* get base filename by removing the .exe extension, if present */ 
    base_file = strdup(output_name);
    len = strlen(base_file);
    if (len > 4 && strcmp(base_file + len - 4, ".exe") == 0)
	base_file[len - 4 ] = 0; /* remove the .exe extension */

    /* we need to get rid of the directory part to get the base name */
    if ( (base_name = strrchr(base_file, '/')) ) base_name++;
    else base_name = base_file;
    
    spec_name = strmake("%s.spec", app_temp_name);
    spec_c_name = strmake("%s.c", spec_name);
    spec_o_name = strmake("%s.o", spec_name);

    wspec_name = strmake("%s.spec", wrp_temp_name);
    wspec_c_name = strmake("%s.c", wspec_name);
    wspec_o_name = strmake("%s.o", wspec_name);

    wrap_c_name = strmake("%s.c", wrp_temp_name);
    wrap_o_name = strmake("%s.o", wrp_temp_name);

    /* build winebuild's argument list */
    spec_args = malloc( (nb_arh_files + nb_dll_files + nb_obj_files + 20) * sizeof (char *) );
    j = 0;
    spec_args[j++] = BINDIR "/winebuild";
    spec_args[j++] = "-o";
    spec_args[j++] = spec_c_name;
    if (create_wrapper)
    {
	spec_args[j++] = "-F";
	spec_args[j++] = strmake("%s.dll", base_name);
	spec_args[j++] = "--spec";
	spec_args[j++] = spec_name;
    }
    else
    {
	spec_args[j++] = "--exe";
	spec_args[j++] = strmake("%s.exe", base_name);
        spec_args[j++] = gui_mode ? "-mgui" : "-mcui";
    }
    for (i = 0; i < nb_dll_files; i++)
	spec_args[j++] = dll_files[i];
    for (i = 0; i < nb_obj_files; i++)
	spec_args[j++] = obj_files[i];
    for (i = 0; i < nb_arh_files; i++)
	spec_args[j++] = arh_files[i];
    spec_args[j] = 0;

    /* build gcc's argument list */
    comp_args = malloc ( 20 * sizeof (char *) );
    j = 0;
    comp_args[j++] = "gcc";
    comp_args[j++] = "-fPIC";
    comp_args[j++] = "-o";
    comp_args[j++] = spec_o_name;
    comp_args[j++] = "-c";
    comp_args[j++] = spec_c_name;
    comp_args[j] = 0;
    
    /* build ld's argument list */
    link_args = malloc( (nb_arh_files + nb_obj_files + nb_lib_files + 20) * sizeof (char *) );
    j = 0;
    link_args[j++] = cpp ? "g++" : "gcc";
    link_args[j++] = "-shared";
    link_args[j++] = "-Wl,-Bsymbolic,-z,defs";
    link_args[j++] = "-lwine";
    link_args[j++] = "-lm";
    for (i = 0; i < nb_lib_files; i++)
	link_args[j++] = lib_files[i];
    link_args[j++] = "-o";
    link_args[j++] = strmake("%s.%s.so", base_file, create_wrapper ? "dll" : "exe");
    link_args[j++] = spec_o_name;
    for (i = 0; i < nb_obj_files; i++)
	if (!is_resource(obj_files[i])) link_args[j++] = obj_files[i];
    for (i = 0; i < nb_arh_files; i++)
	link_args[j++] = arh_files[i];
    link_args[j] = 0;
  
    /* build wrapper compile argument list */
    wwrap_args = malloc ( 20 * sizeof (char *) );
    j = 0;
    wwrap_args[j++] = "gcc";
    wwrap_args[j++] = "-fPIC";
    wwrap_args[j++] = "-I" INCLUDEDIR "/windows";
    wwrap_args[j++] = "-o";
    wwrap_args[j++] = wrap_o_name;
    wwrap_args[j++] = "-c";
    wwrap_args[j++] = wrap_c_name;
    wwrap_args[j] = 0;
     
    /* build wrapper winebuild's argument list */
    wspec_args = malloc( (nb_dll_files + 20) * sizeof (char *) );
    j = 0;
    wspec_args[j++] = BINDIR "/winebuild";
    wspec_args[j++] = "-o";
    wspec_args[j++] = wspec_c_name;
    wspec_args[j++] = "--exe";
    wspec_args[j++] = strmake("%s.exe", base_name);
    wspec_args[j++] = gui_mode ? "-mgui" : "-mcui";
    wspec_args[j++] = wrap_o_name;
    for (i = 0; i < nb_dll_files; i++)
	wspec_args[j++] = dll_files[i];
    wspec_args[j] = 0;

    /* build wrapper gcc's argument list */
    wcomp_args = malloc ( 20 * sizeof (char *) );
    j = 0;
    wcomp_args[j++] = "gcc";
    wcomp_args[j++] = "-fPIC";
    wcomp_args[j++] = "-o";
    wcomp_args[j++] = wspec_o_name;
    wcomp_args[j++] = "-c";
    wcomp_args[j++] = wspec_c_name;
    wcomp_args[j] = 0;
    
    /* build wrapper ld's argument list */
    wlink_args = malloc( 20 * sizeof (char *) );
    j = 0;
    wlink_args[j++] = cpp ? "g++" : "gcc";
    wlink_args[j++] = "-shared";
    wlink_args[j++] = "-Wl,-Bsymbolic,-z,defs";
    wlink_args[j++] = "-lwine";
    wlink_args[j++] = "-ldl";
    wlink_args[j++] = "-o";
    wlink_args[j++] = strmake("%s.exe.so", base_file);
    wlink_args[j++] = wspec_o_name;
    wlink_args[j++] = wrap_o_name;
    wlink_args[j] = 0;
    
    /* run winebuild to get the .spec.c file */
    if (create_wrapper)
    {
	create_file(spec_name, gui_mode ? app_gui_spec : app_cui_spec);
        spawn(spec_args);
        rm_temp_file(spec_name);
	spawn(comp_args);
	rm_temp_file(spec_c_name);
	spawn(link_args);
	rm_temp_file(spec_o_name);

	create_file(wrap_c_name, wrapper_code, base_name, gui_mode);
	spawn(wwrap_args);
	rm_temp_file(wrap_c_name);
	spawn(wspec_args);
	spawn(wcomp_args);
	rm_temp_file(wspec_c_name);
	spawn(wlink_args);
	rm_temp_file(wspec_o_name);
	rm_temp_file(wrap_o_name);
    }
    else
    {
	spawn(spec_args);
	spawn(comp_args);
	rm_temp_file(spec_c_name);
	spawn(link_args);
	rm_temp_file(spec_o_name);
    }

    /* create the loader script */
    create_file(base_file, app_loader_script, base_name);
    chmod(base_file, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

    return 0;    
} 
