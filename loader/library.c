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
#include "segmem.h"
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
#include "stddebug.h"
#include "debug.h"
#include "prototypes.h"
#include "library.h"
#include "ne_image.h"
#include "pe_image.h"

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

int IsDLLLoaded(char *name)
{
	struct w_files *wpnt;

	if(FindDLLTable(name))
		return 1;

	for(wpnt = wine_files; wpnt; wpnt = wpnt->next)
		if(strcmp(wpnt->name, name) == 0)
			return 1;

	return 0;
}

void InitDLL(struct w_files *wpnt)
{
	if (wpnt->ne) 
		NE_InitDLL(wpnt);
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
    for (wpnt = wine_files ; wpnt ; wpnt = wpnt->next)
    	if (strcasecmp(wpnt->name, modulename) == 0)
    		return wpnt->hinstance;

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
 *				GetModuleHandle	[KERNEL.47]
 */
HANDLE GetModuleHandle(LPSTR lpModuleName)
{
	register struct w_files *w = wine_files;
	int 	i;
	char dllname[256];

	if ((int) lpModuleName & 0xffff0000)
		ExtractDLLName(lpModuleName, dllname);

	if ((int) lpModuleName & 0xffff0000)
	 	dprintf_module(stddeb,"GetModuleHandle('%s');\n", lpModuleName);
	else
	 	dprintf_module(stddeb,"GetModuleHandle('%p');\n", lpModuleName);

/* 	dprintf_module(stddeb,"GetModuleHandle // searching in builtin libraries\n");*/
	for (i = 0; i < N_BUILTINS; i++) {
		if (dll_builtin_table[i].dll_name == NULL) break;
		if (((int) lpModuleName & 0xffff0000) == 0) {
			if (0xFF00 + i == (int) lpModuleName) {
				dprintf_module(stddeb,"GetModuleHandle('%s') return %04X \n",
				       lpModuleName, 0xff00 + i);
				return 0xFF00 + i;
				}
			}
		else if (strcasecmp(dll_builtin_table[i].dll_name, dllname) == 0) {
			dprintf_module(stddeb,"GetModuleHandle('%p') return %04X \n", 
							lpModuleName, 0xFF00 + i);
			return (0xFF00 + i);
			}
		}

 	dprintf_module(stddeb,"GetModuleHandle // searching in loaded modules\n");
	while (w) {
/*		dprintf_module(stddeb,"GetModuleHandle // '%x' \n", w->name);  */
		if (((int) lpModuleName & 0xffff0000) == 0) {
			if (w->hinstance == (int) lpModuleName) {
				dprintf_module(stddeb,"GetModuleHandle('%p') return %04X \n",
				       lpModuleName, w->hinstance);
				return w->hinstance;
				}
			}
		else if (strcasecmp(w->name, dllname) == 0) {
			dprintf_module(stddeb,"GetModuleHandle('%s') return %04X \n", 
							lpModuleName, w->hinstance);
			return w->hinstance;
			}
		w = w->next;
		}
	printf("GetModuleHandle('%p') not found !\n", lpModuleName);
	return 0;
}


/**********************************************************************
 *				GetModuleUsage	[KERNEL.48]
 */
int GetModuleUsage(HANDLE hModule)
{
	struct w_files *w;

	dprintf_module(stddeb,"GetModuleUsage(%04X);\n", hModule);

	/* built-in dll ? */
	if (IS_BUILTIN_DLL(hModule)) 
		return 2;
		
	w = GetFileInfo(hModule);
/*	return w->Usage; */
	return 1;
}


/**********************************************************************
 *				GetModuleFilename [KERNEL.49]
 */
int GetModuleFileName(HANDLE hModule, LPSTR lpFileName, short nSize)
{
    struct w_files *w;
    LPSTR str;
    char windir[256], temp[256];

    dprintf_module(stddeb,"GetModuleFileName(%04X, %p, %d);\n", hModule, lpFileName, nSize);

    if (lpFileName == NULL) return 0;
    if (nSize < 1) return 0;

    /* built-in dll ? */
    if (IS_BUILTIN_DLL(hModule)) {
	GetWindowsDirectory(windir, sizeof(windir));
	sprintf(temp, "%s\\%s.DLL", windir, dll_builtin_table[hModule & 0x00ff].dll_name);
	ToDos(temp);
	strncpy(lpFileName, temp, nSize);
        dprintf_module(stddeb,"GetModuleFileName copied '%s' (internal dll) return %d \n", lpFileName, nSize);
	return strlen(lpFileName);
    }

    /* check loaded dlls */
    if ((w = GetFileInfo(hModule)) == NULL)
    	return 0;
    str = DOS_GetDosFileName(w->filename);
    if (nSize > strlen(str)) nSize = strlen(str) + 1;
    strncpy(lpFileName, str, nSize);
    dprintf_module(stddeb,"GetModuleFileName copied '%s' return %d \n", lpFileName, nSize);
    return nSize - 1;
}


/**********************************************************************
 *				LoadLibrary	[KERNEL.95]
 */
HANDLE LoadLibrary(LPSTR libname)
{
	HANDLE h;
	
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


/**********************************************************************
 *					GetProcAddress	[KERNEL.50]
 */
FARPROC GetProcAddress(HANDLE hModule, char *proc_name)
{
#ifdef WINELIB
    WINELIB_UNIMP ("GetProcAddress");
#else
    int		sel, addr, ret;
    register struct w_files *w = wine_files;
    int 	ordinal, len;
    char 	* cpnt;
    char	C[128];
    HTASK	hTask;
    LPTASKENTRY lpTask;

    /* built-in dll ? */
    if (IS_BUILTIN_DLL(hModule))
    {
	if ((int) proc_name & 0xffff0000) 
	{
	    dprintf_module(stddeb,"GetProcAddress: builtin %#04X, '%s'\n", 
		   hModule, proc_name);
	    if (GetEntryDLLName(dll_builtin_table[hModule - 0xFF00].dll_name,
				proc_name, &sel, &addr)) 
	    {
		printf("Address not found !\n");
	    }
	}
	else 
	{
	    dprintf_module(stddeb,"GetProcAddress: builtin %#04X, %d\n", 
		   hModule, (int)proc_name);
	    if (GetEntryDLLOrdinal(dll_builtin_table[hModule-0xFF00].dll_name,
				   (int)proc_name & 0x0000FFFF, &sel, &addr)) 
	    {
		printf("Address not found !\n");
	    }
	}
	ret = MAKELONG(addr, sel);
	dprintf_module(stddeb,"GetProcAddress // ret=%08X sel=%04X addr=%04X\n", 
	       ret, sel, addr);
	return (FARPROC)ret;
    }
    if (hModule == 0) 
    {
	hTask = GetCurrentTask();
	dprintf_module(stddeb,"GetProcAddress // GetCurrentTask()=%04X\n", hTask);
	lpTask = (LPTASKENTRY) GlobalLock(hTask);
	if (lpTask == NULL) 
	{
	    printf("GetProcAddress: can't find current module handle !\n");
	    return NULL;
	}
	hModule = lpTask->hInst;
	dprintf_module(stddeb,"GetProcAddress: current module=%04X instance=%04X!\n", 
	       lpTask->hModule, lpTask->hInst);
	GlobalUnlock(hTask);
    }
    while (w && w->hinstance != hModule) 
	w = w->next;
    if (w == NULL) 
	return NULL;
    dprintf_module(stddeb,"GetProcAddress // Module Found ! w->filename='%s'\n", w->filename);
    if ((int)proc_name & 0xFFFF0000) 
    {
	AnsiUpper(proc_name);
	dprintf_module(stddeb,"GetProcAddress: %04X, '%s'\n", hModule, proc_name);
	cpnt = w->ne->nrname_table;
	while(TRUE) 
	{
	    if (((int) cpnt)  - ((int)w->ne->nrname_table) >  
		w->ne->ne_header->nrname_tab_length)  return NULL;
	    len = *cpnt++;
	    strncpy(C, cpnt, len);
	    C[len] = '\0';
	    dprintf_module(stddeb,"pointing Function '%s' ordinal=%d !\n", 
		   C, *((unsigned short *)(cpnt +  len)));
	    if (strncmp(cpnt, proc_name, len) ==  0) 
	    {
		ordinal =  *((unsigned short *)(cpnt +  len));
		break;
	    }
	    cpnt += len + 2;
	}
	if (ordinal == 0) 
	{
	    printf("GetProcAddress // function '%s' not found !\n", proc_name);
	    return NULL;
	}
    }
    else 
    {
	dprintf_module(stddeb,"GetProcAddress: %#04x, %d\n", hModule, (int) proc_name);
	ordinal = (int)proc_name;
    }
    ret = GetEntryPointFromOrdinal(w, ordinal);
    if (ret == -1) 
    {
	printf("GetProcAddress // Function #%d not found !\n", ordinal);
	return NULL;
    }
    addr  = ret & 0xffff;
    sel = (ret >> 16);
    dprintf_module(stddeb,"GetProcAddress // ret=%08X sel=%04X addr=%04X\n", ret, sel, addr);
    return (FARPROC) ret;
#endif /* WINELIB */
}

/* internal dlls */
static void 
FillModStructBuiltIn(MODULEENTRY *lpModule, struct dll_name_table_entry_s *dll)
{
	lpModule->dwSize = dll->dll_table_length * 1024;
	strcpy(lpModule->szModule, dll->dll_name);
	lpModule->hModule = 0xff00 + dll->dll_number;
	lpModule->wcUsage = GetModuleUsage(lpModule->hModule);
	GetModuleFileName(lpModule->hModule, lpModule->szExePath, MAX_PATH + 1);
	lpModule->wNext = 0;
}

/* loaded dlls */
static void 
FillModStructLoaded(MODULEENTRY *lpModule, struct w_files *dll)
{
	lpModule->dwSize = 16384;
	strcpy(lpModule->szModule, dll->name);
	lpModule->hModule = dll->hinstance;
	lpModule->wcUsage = GetModuleUsage(lpModule->hModule);
	GetModuleFileName(lpModule->hModule, lpModule->szExePath, MAX_PATH + 1);
	lpModule->wNext = 0;
}

/**********************************************************************
 *		ModuleFirst [TOOLHELP.59]
 */
BOOL ModuleFirst(MODULEENTRY *lpModule)
{
	dprintf_module(stddeb,"ModuleFirst(%08X)\n", (int) lpModule);
	
	FillModStructBuiltIn(lpModule, &dll_builtin_table[0]);
	return TRUE;
}

/**********************************************************************
 *		ModuleNext [TOOLHELP.60]
 */
BOOL ModuleNext(MODULEENTRY *lpModule)
{
	struct w_files *w;

	dprintf_module(stddeb,"ModuleNext(%08X)\n", (int) lpModule);

	if (IS_BUILTIN_DLL(lpModule->hModule)) {
		/* last built-in ? */
		if ((lpModule->hModule & 0xff) == (N_BUILTINS - 1) ) {
			if (wine_files) {
				FillModStructLoaded(lpModule, wine_files);
				return TRUE;
			} else
				return FALSE;
		}
		FillModStructBuiltIn(lpModule, &dll_builtin_table[(lpModule->hModule & 0xff)+1]);
		return TRUE;
	}
	w = GetFileInfo(lpModule->hModule);
	if (w->next) {
		FillModStructLoaded(lpModule, w->next);
		return TRUE;
	}
	return FALSE;
}

/**********************************************************************
 *		ModuleFindHandle [TOOLHELP.62]
 */
HMODULE ModuleFindHandle(MODULEENTRY *lpModule, HMODULE hModule)
{
	struct w_files *w;

	dprintf_module(stddeb,"ModuleFindHandle(%08X, %04X)\n", (int) lpModule, (int)hModule);

	/* built-in dll ? */
	if (IS_BUILTIN_DLL(hModule)) {
		FillModStructBuiltIn(lpModule, &dll_builtin_table[hModule & 0xff]);
		return hModule;
	}

	/* check loaded dlls */
	if ((w = GetFileInfo(hModule)) == NULL)
	    	return (HMODULE) NULL;
	
	FillModStructLoaded(lpModule, w);
	return w->hinstance;
}

/**********************************************************************
 *		ModuleFindName [TOOLHELP.61]
 */
HMODULE ModuleFindName(MODULEENTRY *lpModule, LPCSTR lpstrName)
{
	return (ModuleFindHandle(lpModule, GetModuleHandle((char*)lpstrName)));
}

