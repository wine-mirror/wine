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
    "#include <stdio.h>\n"
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
    " * e.g. 'hello-wrap.dll' if the application is called 'hello.exe'.\n"
    " */\n"
    "static char* appName     = \"%s\";\n"
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
    "    fprintf(stderr, \"Error: %%s\\n\", msg);\n"
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
    "    HINSTANCE hApp = 0, hMFC = 0, hMain = 0;\n"
    "    void* appMain;\n"
    "    int retcode, i;\n"
    "    const char* libs[] = { %s };\n"
    "\n"
    "    /* Then if this application is MFC based, load the MFC module */\n"
    "    if (mfcModule) {\n"
    "        hMFC = LoadLibrary(mfcModule);\n"
    "        if (!hMFC) error(\"Could not load the MFC module %%s (%%d)\", mfcModule, GetLastError());\n"
    "        /* For MFC apps, WinMain is in the MFC library */\n"
    "        hMain = hMFC;\n"
    "    }\n"
    "\n"
    "    for (i = 0; i < sizeof(libs)/sizeof(libs[0]); i++) {\n"
    "        if (!LoadLibrary(libs[i])) error(\"Could not load %%s (%%d)\", libs[i], GetLastError());\n"
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
    "\n"
    "    return retcode;\n"
    "}\n"
;

static const char *output_name = "a.out";
static strarray *arh_files, *dll_files, *lib_files, *llib_paths, *lib_paths, *obj_files;
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
static char *find_dll(const char *name)
{
    char *fullname;
    int i;

    for (i = 0; i < lib_paths->size; i++)
    {
        if ((fullname = try_dll_path( lib_paths->base[i], name ))) return fullname;
    }
    return try_dll_path( ".", name );
}

/* find a static library */
static char *find_lib(const char *name)
{
    static const char* std_paths[] = { ".", "/usr/lib", "/usr/local/lib" };
    char *fullname;
    int i;
    
    for (i = 0; i < lib_paths->size; i++)
    {
        if ((fullname = try_lib_path( lib_paths->base[i], name ))) return fullname;
    }

    for (i = 0; i < sizeof(std_paths)/sizeof(std_paths[0]); i++)
    {
        if ((fullname = try_lib_path( std_paths[i], name ))) return fullname;
    }

    return 0;
}

static void add_lib_path(const char* path)
{
    strarray_add(lib_paths, strdup(path));
    strarray_add(llib_paths, strmake("-L%s", path));
}

static void add_lib_file(const char* library)
{
    char *lib;
    
    if (find_dll(library))
    {
	strarray_add(dll_files, strmake("-l%s", library));
    }
    else if ((lib = find_lib(library)))
    {
        strarray_add(arh_files, lib);
    }
    else
    {
        strarray_add(lib_files, strmake("-l%s", library));
    }
}

static void create_the_wrapper(char* base_file, char* base_name, char* app_name, int gui_mode)
{
    char *wrp_temp_name, *wspec_name, *wspec_c_name, *wspec_o_name;
    char *wrap_c_name, *wrap_o_name;
    const char *dlls = "";
    strarray *wwrap_args, *wspec_args, *wcomp_args, *wlink_args;
    int i;

    wrp_temp_name = tempnam(0, "wwrp");
    wspec_name = strmake("%s.spec", wrp_temp_name);
    wspec_c_name = strmake("%s.c", wspec_name);
    wspec_o_name = strmake("%s.o", wspec_name);

    wrap_c_name = strmake("%s.c", wrp_temp_name);
    wrap_o_name = strmake("%s.o", wrp_temp_name);

    /* build wrapper compile argument list */
    wwrap_args = strarray_alloc();
    strarray_add(wwrap_args, "gcc");
    strarray_add(wwrap_args, "-fPIC");
    strarray_add(wwrap_args, "-I" INCLUDEDIR "/windows");
    strarray_add(wwrap_args, "-o");
    strarray_add(wwrap_args, wrap_o_name);
    strarray_add(wwrap_args, "-c");
    strarray_add(wwrap_args, wrap_c_name);
    strarray_add(wwrap_args, NULL);

    for (i = dll_files->size - 1; i >= 0; i--)
        dlls = strmake("\"%s\", %s", dll_files->base[i] + 2, dlls);
    create_file(wrap_c_name, wrapper_code, base_name, gui_mode, app_name, dlls);
    spawn(wwrap_args);
    strarray_free(wwrap_args);
    rm_temp_file(wrap_c_name);

    /* build wrapper winebuild's argument list */
    wspec_args = strarray_alloc();
    strarray_add(wspec_args, "winebuild");
    strarray_add(wspec_args, "-o");
    strarray_add(wspec_args, wspec_c_name);
    strarray_add(wspec_args, "--exe");
    strarray_add(wspec_args, strmake("%s.exe", base_name));
    strarray_add(wspec_args, gui_mode ? "-mgui" : "-mcui");
    strarray_add(wspec_args, wrap_o_name);
    strarray_add(wspec_args, "-L" DLLDIR);
    strarray_add(wspec_args, "-lkernel32");
    strarray_add(wspec_args, NULL);

    spawn(wspec_args);
    strarray_free(wspec_args);

    /* build wrapper gcc's argument list */
    wcomp_args = strarray_alloc();
    strarray_add(wcomp_args, "gcc");
    strarray_add(wcomp_args, "-fPIC");
    strarray_add(wcomp_args, "-o");
    strarray_add(wcomp_args, wspec_o_name);
    strarray_add(wcomp_args, "-c");
    strarray_add(wcomp_args, wspec_c_name);
    strarray_add(wcomp_args, NULL);

    spawn(wcomp_args);
    strarray_free(wcomp_args);
    rm_temp_file(wspec_c_name);

    /* build wrapper ld's argument list */
    wlink_args = strarray_alloc();
    strarray_add(wlink_args, "gcc");
    strarray_add(wlink_args, "-shared");
    strarray_add(wlink_args, "-Wl,-Bsymbolic,-z,defs");
    strarray_add(wlink_args, "-lwine");
    strarray_add(wlink_args, "-o");
    strarray_add(wlink_args, strmake("%s.exe.so", base_file));
    strarray_add(wlink_args, wspec_o_name);
    strarray_add(wlink_args, wrap_o_name);
    strarray_add(wlink_args, NULL);

    spawn(wlink_args);
    strarray_free(wlink_args);
    rm_temp_file(wspec_o_name);
    rm_temp_file(wrap_o_name);
}

int main(int argc, char **argv)
{
    char *library = 0, *path = 0;
    int i, len, cpp = 0, no_opt = 0, gui_mode = 0, create_wrapper = -1;
    char *base_name, *base_file, *base_path, *app_temp_name, *app_name = 0;
    char *spec_name, *spec_c_name, *spec_o_name;
    strarray *spec_args, *comp_args, *link_args;

    arh_files = strarray_alloc();
    dll_files = strarray_alloc();
    lib_files = strarray_alloc();
    lib_paths = strarray_alloc();
    obj_files = strarray_alloc();
    llib_paths = strarray_alloc();

    /* include the standard DLL path first */
    add_lib_path(DLLDIR);

    for (i = 1; i < argc; i++)
    {
	if (!no_opt && argv[i][0] == '-')
	{
	    switch (argv[i][1])
	    {
	    case 'a':
		if (argv[i][2]) app_name = strdup(argv[i]+ 2);
		else if (i + 1 < argc) app_name = strdup(argv[++i]);
		else error("The -a switch takes an argument.");
		break;
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
	strarray_add(obj_files, strdup(argv[i]));
    }

    /* create wrapper only in C++ by default */
    if (create_wrapper == -1) create_wrapper = cpp;
    
    /* link in by default the big three */
    if (gui_mode) add_lib_file("gdi32");
    add_lib_file("user32");
    add_lib_file("kernel32");

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
    
    /* create default name for the wrapper */
    if (!app_name) app_name =  strmake("%s-wrap.dll", base_name);

    spec_name = strmake("%s.spec", app_temp_name);
    spec_c_name = strmake("%s.c", spec_name);
    spec_o_name = strmake("%s.o", spec_name);

    /* build winebuild's argument list */
    spec_args = strarray_alloc();
    strarray_add(spec_args, "winebuild");
    strarray_add(spec_args, "-o");
    strarray_add(spec_args, spec_c_name);
    if (create_wrapper)
    {
	create_file(spec_name, gui_mode ? app_gui_spec : app_cui_spec);
	strarray_add(spec_args, "-F");
	strarray_add(spec_args, app_name);
	strarray_add(spec_args, "--spec");
	strarray_add(spec_args, spec_name);
    }
    else
    {
	strarray_add(spec_args, "--exe");
	strarray_add(spec_args, strmake("%s.exe", base_name));
        strarray_add(spec_args, gui_mode ? "-mgui" : "-mcui");
    }
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

    if (create_wrapper)
        rm_temp_file(spec_name);

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
    strarray_add(link_args, "-Wl,-Bsymbolic,-z,defs");

    for (i = 0; i < llib_paths->size; i++)
	strarray_add(link_args, llib_paths->base[i]);
    strarray_add(link_args, "-lwine");
    strarray_add(link_args, "-lm");
    for (i = 0; i < lib_files->size; i++)
	strarray_add(link_args, lib_files->base[i]);

    strarray_add(link_args, "-o");
    if (create_wrapper)
	strarray_add(link_args, strmake("%s/%s.so", base_path, app_name));
    else
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

    if (create_wrapper)
	create_the_wrapper(base_file, base_name, app_name, gui_mode );

    /* create the loader script */
    create_file(base_file, app_loader_script, base_name);
    chmod(base_file, 0755);

    return 0;    
} 
