/*
 *        Modules & Libraries functions
 */
static char Copyright[] = "Copyright  Martin Ayotte, 1994";

/*
#define DEBUG_MODULE
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "wine.h"
#include "prototypes.h"
#include "windows.h"
#include "dlls.h"
#include "task.h"
#include "toolhelp.h"

extern struct  w_files *wine_files;
extern struct dll_name_table_entry_s dll_builtin_table[];

struct w_files *GetFileInfo(HANDLE);
char *GetDosFileName(char *);

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
	 	printf("GetModuleHandle('%s');\n", lpModuleName);
	else
	 	printf("GetModuleHandle('%x');\n", lpModuleName);

/* 	printf("GetModuleHandle // searching in builtin libraries\n");*/
	for (i = 0; i < N_BUILTINS; i++) {
		if (dll_builtin_table[i].dll_name == NULL) break;
		if (((int) lpModuleName & 0xffff0000) == 0) {
			if (0xFF00 + i == (int) lpModuleName) {
				printf("GetModuleHandle('%s') return %04X \n",
				       lpModuleName, 0xff00 + i);
				return 0xFF00 + i;
				}
			}
		else if (strcasecmp(dll_builtin_table[i].dll_name, dllname) == 0) {
			printf("GetModuleHandle('%x') return %04X \n", 
							lpModuleName, 0xFF00 + i);
			return (0xFF00 + i);
			}
		}

 	printf("GetModuleHandle // searching in loaded modules\n");
	while (w) {
/*		printf("GetModuleHandle // '%x' \n", w->name);  */
		if (((int) lpModuleName & 0xffff0000) == 0) {
			if (w->hinstance == (int) lpModuleName) {
				printf("GetModuleHandle('%x') return %04X \n",
				       lpModuleName, w->hinstance);
				return w->hinstance;
				}
			}
		else if (strcasecmp(w->name, dllname) == 0) {
			printf("GetModuleHandle('%s') return %04X \n", 
							lpModuleName, w->hinstance);
			return w->hinstance;
			}
		w = w->next;
		}
	printf("GetModuleHandle('%x') not found !\n", lpModuleName);
	return 0;
}


/**********************************************************************
 *				GetModuleUsage	[KERNEL.48]
 */
int GetModuleUsage(HANDLE hModule)
{
	struct w_files *w;

	printf("GetModuleUsage(%04X);\n", hModule);

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

    printf("GetModuleFileName(%04X, %08X, %d);\n", hModule, lpFileName, nSize);

    if (lpFileName == NULL) return 0;
    if (nSize < 1) return 0;

    /* built-in dll ? */
    if (IS_BUILTIN_DLL(hModule)) {
	GetWindowsDirectory(windir, sizeof(windir));
	sprintf(temp, "%s\\%s.DLL", windir, dll_builtin_table[hModule & 0x00ff].dll_name);
	ToDos(temp);
	strncpy(lpFileName, temp, nSize);
        printf("GetModuleFileName copied '%s' (internal dll) return %d \n", lpFileName, nSize);
	return strlen(lpFileName);
    }

    /* check loaded dlls */
    if ((w = GetFileInfo(hModule)) == NULL)
    	return 0;
    str = GetDosFileName(w->filename);
    if (nSize > strlen(str)) nSize = strlen(str) + 1;
    strncpy(lpFileName, str, nSize);
    printf("GetModuleFileName copied '%s' return %d \n", lpFileName, nSize);
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
    printf("FreeLibrary(%04X);\n", hLib);

	/* built-in dll ? */
	if (IS_BUILTIN_DLL(hLib)) 
	    	return;

/*
	while (lpMod != NULL) {
		if (lpMod->hInst == hLib) {
			if (lpMod->Count == 1) {
				if (hLib != (HANDLE)NULL) GlobalFree(hLib);
				if (lpMod->ModuleName != NULL) free(lpMod->ModuleName);
				if (lpMod->FileName != NULL) free(lpMod->FileName);
				GlobalFree(lpMod->hModule);
				printf("FreeLibrary // freed !\n");
				return;
				}
			lpMod->Count--;
			printf("FreeLibrary // Count decremented !\n");
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
    int		i, sel, addr, ret;
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
	    printf("GetProcAddress: builtin %#04X, '%s'\n", 
		   hModule, proc_name);
	    if (GetEntryDLLName(dll_builtin_table[hModule - 0xFF00].dll_name,
				proc_name, &sel, &addr)) 
	    {
		printf("Address not found !\n");
	    }
	}
	else 
	{
	    printf("GetProcAddress: builtin %#04X, %d\n", 
		   hModule, (int)proc_name);
	    if (GetEntryDLLOrdinal(dll_builtin_table[hModule-0xFF00].dll_name,
				   (int)proc_name & 0x0000FFFF, &sel, &addr)) 
	    {
		printf("Address not found !\n");
	    }
	}
	ret = MAKELONG(addr, sel);
	printf("GetProcAddress // ret=%08X sel=%04X addr=%04X\n", 
	       ret, sel, addr);
	return (FARPROC)ret;
    }
    if (hModule == 0) 
    {
	hTask = GetCurrentTask();
	printf("GetProcAddress // GetCurrentTask()=%04X\n", hTask);
	lpTask = (LPTASKENTRY) GlobalLock(hTask);
	if (lpTask == NULL) 
	{
	    printf("GetProcAddress: can't find current module handle !\n");
	    return NULL;
	}
	hModule = lpTask->hInst;
	printf("GetProcAddress: current module=%04X instance=%04X!\n", 
	       lpTask->hModule, lpTask->hInst);
	GlobalUnlock(hTask);
    }
    while (w && w->hinstance != hModule) 
	w = w->next;
    if (w == NULL) 
	return NULL;
    printf("GetProcAddress // Module Found ! w->filename='%s'\n", w->filename);
    if ((int)proc_name & 0xFFFF0000) 
    {
	AnsiUpper(proc_name);
	printf("GetProcAddress: %04X, '%s'\n", hModule, proc_name);
	cpnt = w->nrname_table;
	while(TRUE) 
	{
	    if (((int) cpnt)  - ((int)w->nrname_table) >  
		w->ne_header->nrname_tab_length)  return NULL;
	    len = *cpnt++;
	    strncpy(C, cpnt, len);
	    C[len] = '\0';
#ifdef DEBUG_MODULE
	    printf("pointing Function '%s' ordinal=%d !\n", 
		   C, *((unsigned short *)(cpnt +  len)));
#endif
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
	printf("GetProcAddress: %#04x, %d\n", hModule, (int) proc_name);
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
    printf("GetProcAddress // ret=%08X sel=%04X addr=%04X\n", ret, sel, addr);
    return (FARPROC) ret;
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
 *		ModuleFirst [TOOHELP.59]
 */
BOOL ModuleFirst(MODULEENTRY *lpModule)
{
	printf("ModuleFirst(%08X)\n", lpModule);
	
	FillModStructBuiltIn(lpModule, &dll_builtin_table[0]);
	return TRUE;
}

/**********************************************************************
 *		ModuleNext [TOOHELP.60]
 */
BOOL ModuleNext(MODULEENTRY *lpModule)
{
	struct w_files *w;

	printf("ModuleNext(%08X)\n", lpModule);

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
 *		ModuleFindHandle [TOOHELP.62]
 */
HMODULE ModuleFindHandle(MODULEENTRY *lpModule, HMODULE hModule)
{
	struct w_files *w;

	printf("ModuleFindHandle(%08X, %04X)\n", lpModule, hModule);

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
 *		ModuleFindName [TOOHELP.61]
 */
HMODULE ModuleFindName(MODULEENTRY *lpModule, LPCSTR lpstrName)
{
	return (ModuleFindHandle(lpModule, GetModuleHandle((char*)lpstrName)));
}

