/*
 *        Module & Library functions
static char Copyright[] = "Copyright 1993, 1994 Martin Ayotte, Robert J. Amstadt, Erik Bos";
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "neexe.h"
#include "dlls.h"
#include "if1632.h"
#include "wineopts.h"
#include "arch.h"
#include "options.h"
#include "dos_fs.h"
#include "windows.h"
#include "task.h"
#include "toolhelp.h"
#include "selectors.h"
#include "prototypes.h"
#include "library.h"
#include "ne_image.h"
#include "pe_image.h"
#include "module.h"
#include "stddebug.h"
#include "debug.h"

struct w_files *wine_files = NULL;
static char *DLL_Extensions[] = { "dll", NULL };
static char *EXE_Extensions[] = { "exe", NULL };

#define IS_BUILTIN_DLL(handle) ((handle >> 8) == 0xff) 

/**********************************************************************/

void ExtractDLLName(char *libname, char *temp)
{
    int i;
    
    strcpy(temp, libname);
    if (strchr(temp, '\\') || strchr(temp, '/'))
	for (i = strlen(temp) - 1; i ; i--) 
		if (temp[i] == '\\' || temp[i] == '/') {
			strcpy(temp, temp + i + 1);
			break;
		}
    for (i = strlen(temp) - 1; i ; i--) 
	if (temp[i] == '.') {
		temp[i] = 0;
		break;
	}
}

struct w_files *GetFileInfo(unsigned short instance)
{
    register struct w_files *w = wine_files;

    while (w && w->hinstance != instance)
	w = w->next;
    
    return w;
}
/*
int IsDLLLoaded(char *name)
{
	struct w_files *wpnt;

	if(FindDLLTable(name))
		return 1;

	for(wpnt = wine_files; wpnt; wpnt = wpnt->next)
		if(strcmp(wpnt->name, name) == 0 )
			return 1;

	return 0;
}
*/
void InitDLL(struct w_files *wpnt)
{
	if (wpnt->ne) 
		NE_InitDLL(wpnt->hModule);
	else
		PE_InitDLL(wpnt);
}

void InitializeLoadedDLLs(struct w_files *wpnt)
{
    static flagReadyToRun = 0;
    struct w_files *final_wpnt;

    dprintf_module(stddeb,"InitializeLoadedDLLs(%p)\n", wpnt);

    if (wpnt == NULL)
    {
	flagReadyToRun = 1;
	dprintf_module(stddeb,"Initializing DLLs\n");
    }
    
    if (!flagReadyToRun)
	return;

#if 1
    if (wpnt != NULL)
	dprintf_module(stddeb,"Initializing %s\n", wpnt->name);
#endif

    /*
     * Initialize libraries
     */
    if (!wpnt)
    {
	wpnt = wine_files;
	final_wpnt = NULL;
    }
    else
    {
	final_wpnt = wpnt->next;
    }
    
    for( ; wpnt != final_wpnt; wpnt = wpnt->next)
	InitDLL(wpnt);
}

/**********************************************************************
 *			LoadImage
 * Load one executable into memory
 */
HINSTANCE LoadImage(char *module, int filetype, int change_dir)
{
    HINSTANCE handle;
    struct w_files *wpnt, *wpnt1;
    char buffer[256], header[2], modulename[64], *fullname;

    ExtractDLLName(module, modulename);
    dprintf_module(stddeb,"LoadImage [%s]\n", module);
    /* built-in one ? */
    if (FindDLLTable(modulename)) {
	return GetModuleHandle(modulename);
    }
    
    /* already loaded ? */
    for (wpnt = wine_files ; wpnt ; wpnt = wpnt->next)  {
      if (strcasecmp(wpnt->name, modulename) == 0 && filetype == wpnt->type) {
	return wpnt->hinstance;
      }
    }

    /*
     * search file
     */
    fullname = DOS_FindFile(buffer, sizeof(buffer), module, 
			(filetype == EXE ? EXE_Extensions : DLL_Extensions), 
			WindowsPath);
    if (fullname == NULL)
    {
    	fprintf(stderr, "LoadImage: I can't find %s.dll | %s.exe !\n",
		module, module);
	return 2;
    }

    fullname = DOS_GetDosFileName(fullname);
    
    dprintf_module(stddeb,"LoadImage: loading %s (%s)\n           [%s]\n", 
	    module, buffer, fullname);

    if (change_dir && fullname)
    {
	char dirname[256];
	char *p;

	strcpy(dirname, fullname);
	p = strrchr(dirname, '\\');
	*p = '\0';

	DOS_SetDefaultDrive(dirname[0] - 'A');
	DOS_ChangeDir(dirname[0] - 'A', dirname + 2);
    }

    /* First allocate a spot to store the info we collect, and add it to
     * our linked list if we could load the file.
     */

    wpnt = (struct w_files *) malloc(sizeof(struct w_files));

    /*
     * Open file for reading.
     */
    wpnt->fd = open(buffer, O_RDONLY);
    if (wpnt->fd < 0)
	return 2;

    /* 
     * Establish header pointers.
     */
    wpnt->filename = strdup(buffer);
    wpnt->name = strdup(modulename);
    wpnt->type = filetype;
    wpnt->initialised = FALSE;

    /* read mz header */
    wpnt->mz_header = (struct mz_header_s *) malloc(sizeof(struct mz_header_s));;
    lseek(wpnt->fd, 0, SEEK_SET);
    if (read(wpnt->fd, wpnt->mz_header, sizeof(struct mz_header_s)) !=
	sizeof(struct mz_header_s))
    {
	fprintf(stderr, "Unable to read MZ header from file '%s'\n", buffer);
        exit(1);
    }

      /* This field is ignored according to "Windows Internals", p.242 */
#if 0
    if (wpnt->mz_header->must_be_0x40 != 0x40)
	myerror("This is not a Windows program");
#endif

    /* read first two bytes to determine filetype */
    lseek(wpnt->fd, wpnt->mz_header->ne_offset, SEEK_SET);
    read(wpnt->fd, &header, sizeof(header));

    handle = 0;

    /* 
     * Stick this file into the list of loaded files so we don't try to reload
     * it again if another module references this module.  Do this before
     * calling NE_LoadImage because we might get back here before NE_loadImage
     * returns.
     */
    if(wine_files == NULL)
       wine_files = wpnt;
    else {
	 wpnt1 = wine_files;
	 while(wpnt1->next)
	    wpnt1 = wpnt1->next;
	 wpnt1->next = wpnt;
    }
    wpnt->next = NULL;

    if (header[0] == 'N' && header[1] == 'E')
    	handle = NE_LoadImage(wpnt);
    if (header[0] == 'P' && header[1] == 'E')
        handle = PE_LoadImage(wpnt);
    wpnt->hinstance = handle;

    if (handle > 32) {
	return handle;
    } else {
	fprintf(stderr, "wine: (%s) unknown fileformat !\n", wpnt->filename);

	/* Remove this module from the list of loaded modules */
	if (wine_files == wpnt)
	    wine_files = NULL;
	else
	    wpnt1->next = NULL;
	close(wpnt->fd);
	free(wpnt->filename);
	free(wpnt->name);
	free(wpnt);

	return 14;
    }
}

/**********************************************************************
 *				LoadLibrary	[KERNEL.95]
 */
HANDLE LoadLibrary(LPSTR libname)
{
	HANDLE h;
	
        dprintf_module(stddeb,"LoadLibrary: (%08x) %s\n",(int)libname,libname);

	if ((h = LoadImage(libname, DLL, 0)) < 32)
		return h;

	if (!IS_BUILTIN_DLL(h))
		InitDLL(GetFileInfo(h));
	
	return h;
}

/**********************************************************************
 *				FreeLibrary	[KERNEL.96]
 */
void FreeLibrary(HANDLE hLib)
{
	dprintf_module(stddeb,"FreeLibrary(%04X);\n", hLib);

	/* built-in dll ? */
	if (IS_BUILTIN_DLL(hLib) || hLib == 0 || hLib == hSysRes) 
	    	return;

/*
	while (lpMod != NULL) {
		if (lpMod->hInst == hLib) {
			if (lpMod->Count == 1) {
				wpnt = GetFileInfo(hLib);
				if (wpnt->ne)
					NE_UnloadImage(wpnt);
				else
					PE_UnloadImage(wpnt);
				if (hLib != (HANDLE)NULL) GlobalFree(hLib);
				if (lpMod->ModuleName != NULL) free(lpMod->ModuleName);
				if (lpMod->FileName != NULL) free(lpMod->FileName);
				GlobalFree(lpMod->hModule);
				dprintf_module(stddeb,"FreeLibrary // freed !\n");
				return;
				}
			lpMod->Count--;
			dprintf_module(stddeb,"FreeLibrary // Count decremented !\n");
			return;
			}
		lpMod = lpMod->lpNextModule;
		}
*/
}


/***********************************************************************
 *           GetProcAddress   (KERNEL.50)
 */
FARPROC GetProcAddress( HANDLE hModule, SEGPTR name )
{
    WORD ordinal;
    SEGPTR ret;

    if (!hModule) hModule = GetCurrentTask();
    hModule = GetExePtr( hModule );

    if (HIWORD(name) != 0)
    {
        ordinal = MODULE_GetOrdinal( hModule, (LPSTR)PTR_SEG_TO_LIN(name) );
        dprintf_module( stddeb, "GetProcAddress: %04x '%s'\n",
                        hModule, (LPSTR)PTR_SEG_TO_LIN(name) );
    }
    else
    {
        ordinal = LOWORD(name);
        dprintf_module( stddeb, "GetProcAddress: %04x %04x\n",
                        hModule, ordinal );
    }
    if (!ordinal) return (FARPROC)0;

    ret = MODULE_GetEntryPoint( hModule, ordinal );

    dprintf_module( stddeb, "GetProcAddress: returning %08lx\n", ret );
    return (FARPROC)ret;
}
