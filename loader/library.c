/*
 *        Modules & Libraries functions
 */
static char Copyright[] = "Copyright  Martin Ayotte, 1994";

#ifndef WINELIB
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "prototypes.h"
#include "windows.h"
#include "wine.h"
#include "dlls.h"
#include "task.h"

typedef struct {
	LPSTR		ModuleName;
	LPSTR		FileName;
	WORD		Count;
	HANDLE		hModule;
	HINSTANCE	hInst;
	void		*lpPrevModule;
	void		*lpNextModule;
} MODULEENTRY;
typedef MODULEENTRY *LPMODULEENTRY;

static LPMODULEENTRY lpModList = NULL;

extern struct  w_files * wine_files;
#define N_BUILTINS	10
extern struct dll_name_table_entry_s dll_builtin_table[N_BUILTINS];


/**********************************************************************
 *				GetModuleHandle	[KERNEL.47]
 */
HANDLE GetModuleHandle(LPSTR lpModuleName)
{
	register struct w_files *w = wine_files;
	int 	i;
 	printf("GetModuleHandle('%s');\n", lpModuleName);
	while (w) {
/*		printf("GetModuleHandle // '%s' \n", w->name);  */
		if (strcmp(w->name, lpModuleName) == 0) {
			printf("GetModuleHandle('%s') return %04X \n", 
							lpModuleName, w->hinstance);
			return w->hinstance;
			}
		w = w->next;
		}
    for (i = 0; i < N_BUILTINS; i++) {
		if (strcmp(dll_builtin_table[i].dll_name, lpModuleName) == 0) {
			printf("GetModuleHandle('%s') return %04X \n", 
							lpModuleName, 0xFF00 + i);
			return (0xFF00 + i);
			}
		}
	printf("GetModuleHandle('%s') not found !\n", lpModuleName);
	return 0;
}


/**********************************************************************
 *				GetModuleUsage	[KERNEL.48]
 */
int GetModuleUsage(HANDLE hModule)
{
	struct w_files *w;
	printf("GetModuleUsage(%04X);\n", hModule);
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
    printf("GetModuleFileName(%04X, %08X, %d);\n", hModule, lpFileName, nSize);
    if (lpFileName == NULL) return 0;
    w = GetFileInfo(hModule);
    if (w == NULL) return 0;
    if (nSize > strlen(w->name)) nSize = strlen(w->name) + 1;
    strncpy(lpFileName, w->name, nSize);
    printf("GetModuleFileName copied '%s' return %d \n", lpFileName, nSize);
    return nSize - 1;
}


/**********************************************************************
 *				LoadLibrary	[KERNEL.95]
 */
HANDLE LoadLibrary(LPSTR libname)
{
    HANDLE hModule;
	LPMODULEENTRY lpMod = lpModList;
	LPMODULEENTRY lpNewMod;
    printf("LoadLibrary '%s'\n", libname);
	if (lpMod != NULL) {
		while (TRUE) {
			if (strcmp(libname, lpMod->FileName) == 0) {
				lpMod->Count++;
			    printf("LoadLibrary // already loaded hInst=%04X\n", lpMod->hInst);
				return lpMod->hInst;
				}
			if (lpMod->lpNextModule == NULL) break;
			lpMod = lpMod->lpNextModule;
			}
		}
	hModule = GlobalAlloc(GMEM_MOVEABLE, sizeof(MODULEENTRY));
	lpNewMod = (LPMODULEENTRY) GlobalLock(hModule);	
#ifdef DEBUG_LIBRARY
    printf("LoadLibrary // creating new module entry %08X\n", lpNewMod);
#endif
	if (lpNewMod == NULL) return 0;
	if (lpModList == NULL) {
		lpModList = lpNewMod;
		lpNewMod->lpPrevModule = NULL;
		}
	else {
		lpMod->lpNextModule = lpNewMod;
		lpNewMod->lpPrevModule = lpMod;
		}
	lpNewMod->lpNextModule = NULL;
	lpNewMod->hModule = hModule;
	lpNewMod->ModuleName = NULL;
	lpNewMod->FileName = (LPSTR) malloc(strlen(libname));
	if (lpNewMod->FileName != NULL)	strcpy(lpNewMod->FileName, libname);
	lpNewMod->hInst = LoadImage(libname, DLL);
	lpNewMod->Count = 1;
    printf("LoadLibrary returned Library hInst=%04X\n", lpNewMod->hInst);
	GlobalUnlock(hModule);	
    return lpNewMod->hInst;
}


/**********************************************************************
 *				FreeLibrary	[KERNEL.96]
 */
void FreeLibrary(HANDLE hLib)
{
	LPMODULEENTRY lpMod = lpModList;
    printf("FreeLibrary(%04X);\n", hLib);
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
}


/**********************************************************************
 *					GetProcAddress	[KERNEL.50]
 */
FARPROC GetProcAddress(HANDLE hModule, char *proc_name)
{
	WORD 	wOrdin;
	int		sel, addr, ret;
    register struct w_files *w = wine_files;
	int 	ordinal, len;
	char 	* cpnt;
	char	C[128];
	HTASK	hTask;
	LPTASKENTRY lpTask;
	if (hModule >= 0xF000) {
		if ((int) proc_name & 0xffff0000) {
			printf("GetProcAddress: builtin %#04X, '%s'\n", 
										hModule, proc_name);
/*			wOrdin = FindOrdinalFromName(struct dll_table_entry_s *dll_table, proc_name); */
			}
		else {
			printf("GetProcAddress: builtin %#04X, %d\n", 
								hModule, (int)proc_name);
			}
		return NULL;
		}
	if (hModule == 0) {
		hTask = GetCurrentTask();
		printf("GetProcAddress // GetCurrentTask()=%04X\n", hTask);
		lpTask = (LPTASKENTRY) GlobalLock(hTask);
		if (lpTask == NULL) {
			printf("GetProcAddress: can't find current module handle !\n");
			return NULL;
			}
		hModule = lpTask->hInst;
		printf("GetProcAddress: current module=%04X instance=%04X!\n", 
			lpTask->hModule, lpTask->hInst);
		GlobalUnlock(hTask);
		}
	while (w && w->hinstance != hModule) w = w->next;
	if (w == NULL) return NULL;
	printf("GetProcAddress // Module Found ! w->filename='%s'\n", w->filename);
	if ((int)proc_name & 0xFFFF0000) {
		AnsiUpper(proc_name);
		printf("GetProcAddress: %04X, '%s'\n", hModule, proc_name);
		cpnt = w->nrname_table;
		while(TRUE) {
			if (((int) cpnt)  - ((int)w->nrname_table) >  
			   w->ne_header->nrname_tab_length)  return NULL;
			len = *cpnt++;
			strncpy(C, cpnt, len);
			C[len] = '\0';
#ifdef DEBUG_MODULE
			printf("pointing Function '%s' !\n", C);
#endif
			if (strncmp(cpnt, proc_name, len) ==  0) {
				ordinal =  *((unsigned short *)  (cpnt +  len));
				break;
				}
			cpnt += len + 2;
			};
		if (ordinal == 0) {
			printf("GetProcAddress // function '%s' not found !\n", proc_name);
			return NULL;
			}
		}
	else {
		printf("GetProcAddress: %#04x, %d\n", hModule, (int) proc_name);
		ordinal = (int)proc_name;
		}
	ret = GetEntryPointFromOrdinal(w, ordinal);
	if (ret == -1) {
		printf("GetProcAddress // Function #%d not found !\n", ordinal);
		return NULL;
		}
	addr  = ret & 0xffff;
	sel = (ret >> 16);
	printf("GetProcAddress // ret=%08X sel=%04X addr=%04X\n", ret, sel, addr);
	return (FARPROC) ret;
}

#endif /* ifndef WINELIB */

