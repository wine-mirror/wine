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

typedef struct module_table_entry
{
	HINSTANCE	hInst;
	LPSTR		name;
	WORD		count;
} MODULEENTRY;

extern struct  w_files * wine_files;

#define N_BUILTINS	10

extern struct dll_name_table_entry_s dll_builtin_table[N_BUILTINS];

/**********************************************************************
 *				GetCurrentTask	[KERNEL.36]
 */
HTASK GetCurrentTask()
{
    int pid = getpid();
    printf("GetCurrentTask() returned %d !\n", pid);
    return pid;
}


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
/*    return w->Usage; */
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
    printf("LoadLibrary '%s'\n", libname);
    hModule = LoadImage(libname, DLL);
    printf("LoadLibrary returned hModule=%04X\n", hModule);
    return hModule;
}


/**********************************************************************
 *				FreeLibrary	[KERNEL.96]
 */
void FreeLibrary(HANDLE hLib)
{
    printf("FreeLibrary(%04X);\n", hLib);
    if (hLib != (HANDLE)NULL) GlobalFree(hLib);
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
	if (hModule == 0) {
		printf("GetProcAddress: Bad Module handle=%#04X\n", hModule);
		return NULL;
		}
	if (hModule >= 0xF000) {
		if ((int) proc_name & 0xffff0000) {
			printf("GetProcAddress: builtin %#04x, '%s'\n", hModule, proc_name);
/*			wOrdin = FindOrdinalFromName(struct dll_table_entry_s *dll_table, proc_name); */
			}
		else {
			printf("GetProcAddress: builtin %#04x, %d\n", hModule, (int) proc_name);
			}
		return NULL;
		}
	while (w && w->hinstance != hModule) w = w->next;
	printf("GetProcAddress // Module Found ! w->filename='%s'\n", w->filename);
	if (w == NULL) return NULL;
	if ((int) proc_name & 0xffff0000) {
		AnsiUpper(proc_name);
		printf("GetProcAddress: %#04x, '%s'\n", hModule, proc_name);
		cpnt = w->nrname_table;
		while(TRUE) {
			if (((int) cpnt)  - ((int)w->nrname_table) >  
			   w->ne_header->nrname_tab_length)  return NULL;
			len = *cpnt++;
			strncpy(C, cpnt, len);
			C[len] = '\0';
			printf("pointing Function '%s' !\n", C);
			if (strncmp(cpnt, proc_name, len) ==  0) break;
			cpnt += len + 2;
			};
		ordinal =  *((unsigned short *)  (cpnt +  len));
		}
	else {
		printf("GetProcAddress: %#04x, %d\n", hModule, (int) proc_name);
		ordinal = (int)proc_name;
		}
	ret = GetEntryPointFromOrdinal(w, ordinal);
	if (ret == -1) {
		printf("GetProcAddress // Function not found !\n");
		return NULL;
		}
	addr  = ret & 0xffff;
	sel = (ret >> 16);
	printf("GetProcAddress // ret=%08X sel=%04X addr=%04X\n", ret, sel, addr);
	return ret;
}

#endif /* ifndef WINELIB */

