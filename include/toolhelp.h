#ifndef __TOOLHELP_H
#define __TOOLHELP_H

#include "windows.h"

DECLARE_HANDLE(HMODULE);
DECLARE_HANDLE(HGLOBAL);

#define MAX_DATA	11
#define MAX_MODULE_NAME	9
#define MAX_PATH	255
#define MAX_CLASSNAME	255

/* modules */

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

/* tasks */

typedef struct tagTASKENTRY {
	DWORD dwSize;
	HTASK hTask;
	HTASK hTaskParent;
	HINSTANCE hInst;
	HMODULE hModule;
	WORD wSS;
	WORD wSP;
	WORD wStackTop;
	WORD wStackMinimum;
	WORD wStackBottom;
	WORD wcEvents;
	HGLOBAL hQueue;
	char szModule[MAX_MODULE_NAME + 1];
	WORD wPSPOffset;
	HANDLE hNext;
} TASKENTRY;
typedef TASKENTRY *LPTASKENTRY;

BOOL	TaskFirst(LPTASKENTRY lpTask);
BOOL	TaskNext(LPTASKENTRY lpTask);
BOOL	TaskFindHandle(LPTASKENTRY lpTask, HTASK hTask);
DWORD	TaskSetCSIP(HTASK hTask, WORD wCS, WORD wIP);
DWORD	TaskGetCSIP(HTASK hTask);
BOOL	TaskSwitch(HTASK hTask, DWORD dwNewCSIP);

/* mem info */

typedef struct tagMEMMANINFO {
	DWORD dwSize;
	DWORD dwLargestFreeBlock;
	DWORD dwMaxPagesAvailable;
	DWORD dwMaxPagesLockable;
	DWORD dwTotalLinearSpace;
	DWORD dwTotalUnlockedPages;
	DWORD dwFreePages;
	DWORD dwTotalPages;
	DWORD dwFreeLinearSpace;
	DWORD dwSwapFilePages;
	WORD wPageSize;
} MEMMANINFO;
typedef MEMMANINFO *LPMEMMANINFO;

typedef struct tagSYSHEAPINFO {
	DWORD dwSize;
	WORD wUserFreePercent;
	WORD wGDIFreePercent;
	HGLOBAL hUserSegment;
	HGLOBAL hGDISegment;
} SYSHEAPINFO;
typedef SYSHEAPINFO *LPSYSHEAPINFO;

BOOL MemManInfo(LPMEMMANINFO lpEnhMode);
BOOL SystemHeapInfo(LPSYSHEAPINFO lpSysHeap);

#endif /* __TOOLHELP_H */
