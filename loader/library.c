#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "prototypes.h"
#include "windows.h"
#include "wine.h"
#include "dlls.h"

extern struct  w_files * wine_files;


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
    printf("GetModuleHandle('%s');\n", lpModuleName);
    while (w) {
	printf("GetModuleHandle // '%s' \n", w->name); 
	if (strcmp(w->name, lpModuleName) == 0) {
	    printf("GetModuleHandle('%s') return %04X \n", 
		lpModuleName, w->hinstance);
	    return w->hinstance;
	    }
	w = w->next;
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
    HANDLE hRet;
    printf("LoadLibrary '%s'\n", libname);
    hRet = LoadImage(libname, DLL);
    printf("after LoadLibrary hRet=%04X\n", hRet);
    return hRet;
}


/**********************************************************************
 *				FreeLibrary	[KERNEL.96]
 */
void FreeLibrary(HANDLE hLib)
{
    printf("FreeLibrary(%04X);\n", hLib);
    if (hLib != (HANDLE)NULL) GlobalFree(hLib);
}


