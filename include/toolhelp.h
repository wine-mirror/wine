#ifndef __TOOLHELP_H
#define __TOOLHELP_H

#include "windows.h"

#define MAX_DATA	11
#define MAX_MODULE_NAME	9
#define MAX_PATH	255
#define MAX_CLASSNAME	255

/* Global heap */

WORD GlobalHandleToSel( HANDLE handle );


/* Local heap */

typedef struct
{
    DWORD   dwSize;
    WORD    wcItems;
} LOCALINFO;

typedef struct
{
    DWORD   dwSize;
    HLOCAL  hHandle;
    WORD    wAddress;
    WORD    wSize;
    WORD    wFlags;
    WORD    wcLock;
    WORD    wType;
    WORD    hHeap;
    WORD    wHeapType;
    WORD    wNext;
} LOCALENTRY;

/* wHeapType values */
#define NORMAL_HEAP     0
#define USER_HEAP       1
#define GDI_HEAP        2

/* wFlags values */
#define LF_FIXED        1
#define LF_FREE         2
#define LF_MOVEABLE     4

/* wType values */
#define LT_NORMAL                   0
#define LT_FREE                     0xff
#define LT_GDI_PEN                  1   /* LT_GDI_* is for GDI's heap */
#define LT_GDI_BRUSH                2
#define LT_GDI_FONT                 3
#define LT_GDI_PALETTE              4
#define LT_GDI_BITMAP               5
#define LT_GDI_RGN                  6
#define LT_GDI_DC                   7
#define LT_GDI_DISABLED_DC          8
#define LT_GDI_METADC               9
#define LT_GDI_METAFILE             10
#define LT_GDI_MAX                  LT_GDI_METAFILE
#define LT_USER_CLASS               1   /* LT_USER_* is for USER's heap */
#define LT_USER_WND                 2
#define LT_USER_STRING              3
#define LT_USER_MENU                4
#define LT_USER_CLIP                5
#define LT_USER_CBOX                6
#define LT_USER_PALETTE             7
#define LT_USER_ED                  8
#define LT_USER_BWL                 9
#define LT_USER_OWNERDRAW           10
#define LT_USER_SPB                 11
#define LT_USER_CHECKPOINT          12
#define LT_USER_DCE                 13
#define LT_USER_MWP                 14
#define LT_USER_PROP                15
#define LT_USER_LBIV                16
#define LT_USER_MISC                17
#define LT_USER_ATOMS               18
#define LT_USER_LOCKINPUTSTATE      19
#define LT_USER_HOOKLIST            20
#define LT_USER_USERSEEUSERDOALLOC  21
#define LT_USER_HOTKEYLIST          22
#define LT_USER_POPUPMENU           23
#define LT_USER_HANDLETABLE         32
#define LT_USER_MAX                 LT_USER_HANDLETABLE

BOOL LocalInfo( LOCALINFO *pLocalInfo, HGLOBAL handle );
BOOL LocalFirst( LOCALENTRY *pLocalEntry, HGLOBAL handle );
BOOL LocalNext( LOCALENTRY *pLocalEntry );


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

typedef struct
{
    DWORD   dwSize;
    WORD    wUserFreePercent;
    WORD    wGDIFreePercent;
    HGLOBAL hUserSegment;
    HGLOBAL hGDISegment;
} SYSHEAPINFO;

BOOL MemManInfo(LPMEMMANINFO lpEnhMode);
BOOL SystemHeapInfo( SYSHEAPINFO *pHeapInfo );


/* Window classes */

typedef struct
{
    DWORD     dwSize;
    HMODULE   hInst;              /* This is really an hModule */
    char      szClassName[MAX_CLASSNAME + 1];
    WORD      wNext;
} CLASSENTRY;

BOOL ClassFirst( CLASSENTRY *pClassEntry );
BOOL ClassNext( CLASSENTRY *pClassEntry );


/* Memory read/write */

DWORD MemoryRead( WORD sel, DWORD offset, void *buffer, DWORD count );
DWORD MemoryWrite( WORD sel, DWORD offset, void *buffer, DWORD count );


#endif /* __TOOLHELP_H */
