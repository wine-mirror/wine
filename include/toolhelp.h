#ifndef __TOOLHELP_H
#define __TOOLHELP_H

#include "windows.h"

DECLARE_HANDLE(HMODULE);

#define MAX_MODULE_NAME 9
#define MAX_PATH 255

typedef struct {
    DWORD dwSize;
    char szModule[MAX_MODULE_NAME + 1];
    HMODULE hModule;
    WORD wcUsage;
    char szExePath[MAX_PATH + 1];
    WORD wNext;
} MODULEENTRY;
typedef MODULEENTRY *LPMODULEENTRY;

BOOL	ModuleFirst(MODULEENTRY *lpModule);
BOOL	ModuleNext(MODULEENTRY *lpModule);
HMODULE ModuleFindName(MODULEENTRY *lpModule, LPCSTR lpstrName);
HMODULE ModuleFindHandle(MODULEENTRY *lpModule, HMODULE hModule);

#endif /* __TOOLHELP_H */
