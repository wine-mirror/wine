#ifndef __WINE_TOOLHELP_H
#define __WINE_TOOLHELP_H

#include "windows.h"

#define MAX_DATA	11
#define MAX_MODULE_NAME	9
#define MAX_PATH	255
#define MAX_CLASSNAME	255

#pragma pack(1)

/* Global heap */

typedef struct
{
    DWORD dwSize;
    WORD  wcItems;
    WORD  wcItemsFree;
    WORD  wcItemsLRU;
} GLOBALINFO;

typedef struct
{
    DWORD     dwSize;
    DWORD     dwAddress;
    DWORD     dwBlockSize;
    HGLOBAL16 hBlock;
    WORD      wcLock;
    WORD      wcPageLock;
    WORD      wFlags;
    BOOL16    wHeapPresent;
    HGLOBAL16 hOwner;
    WORD      wType;
    WORD      wData;
    DWORD     dwNext;
    DWORD     dwNextAlt;
} GLOBALENTRY;

  /* GlobalFirst()/GlobalNext() flags */
#define GLOBAL_ALL      0
#define GLOBAL_LRU      1
#define GLOBAL_FREE     2

  /* wType values */
#define GT_UNKNOWN      0
#define GT_DGROUP       1
#define GT_DATA         2
#define GT_CODE         3
#define GT_TASK         4
#define GT_RESOURCE     5
#define GT_MODULE       6
#define GT_FREE         7
#define GT_INTERNAL     8
#define GT_SENTINEL     9
#define GT_BURGERMASTER 10

/* wData values */
#define GD_USERDEFINED      0
#define GD_CURSORCOMPONENT  1
#define GD_BITMAP           2
#define GD_ICONCOMPONENT    3
#define GD_MENU             4
#define GD_DIALOG           5
#define GD_STRING           6
#define GD_FONTDIR          7
#define GD_FONT             8
#define GD_ACCELERATORS     9
#define GD_RCDATA           10
#define GD_ERRTABLE         11
#define GD_CURSOR           12
#define GD_ICON             14
#define GD_NAMETABLE        15
#define GD_MAX_RESOURCE     15

/* wFlags values */
#define GF_PDB_OWNER        0x0100      /* Low byte is KERNEL flags */

BOOL16 GlobalInfo( GLOBALINFO *pInfo );
BOOL16 GlobalFirst( GLOBALENTRY *pGlobal, WORD wFlags );
BOOL16 GlobalNext( GLOBALENTRY *pGlobal, WORD wFlags) ;
BOOL16 GlobalEntryHandle( GLOBALENTRY *pGlobal, HGLOBAL16 hItem );
BOOL16 GlobalEntryModule( GLOBALENTRY *pGlobal, HMODULE16 hModule, WORD wSeg );

/* Local heap */

typedef struct
{
    DWORD   dwSize;
    WORD    wcItems;
} LOCALINFO;

typedef struct
{
    DWORD   dwSize;
    HLOCAL16  hHandle;
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

BOOL16 LocalInfo( LOCALINFO *pLocalInfo, HGLOBAL16 handle );
BOOL16 LocalFirst( LOCALENTRY *pLocalEntry, HGLOBAL16 handle );
BOOL16 LocalNext( LOCALENTRY *pLocalEntry );


/* modules */

typedef struct
{
    DWORD      dwSize;
    char       szModule[MAX_MODULE_NAME + 1];
    HMODULE16  hModule;
    WORD       wcUsage;
    char       szExePath[MAX_PATH + 1];
    HANDLE16   wNext;
} MODULEENTRY, *LPMODULEENTRY;

BOOL16 ModuleFirst(MODULEENTRY *lpModule);
BOOL16 ModuleNext(MODULEENTRY *lpModule);
BOOL16 ModuleFindName(MODULEENTRY *lpModule, LPCSTR lpstrName);
BOOL16 ModuleFindHandle(MODULEENTRY *lpModule, HMODULE16 hModule);

/* tasks */

typedef struct
{
    DWORD        dwSize;
    HTASK16      hTask;
    HTASK16      hTaskParent;
    HINSTANCE16  hInst;
    HMODULE16    hModule;
    WORD         wSS;
    WORD         wSP;
    WORD         wStackTop;
    WORD         wStackMinimum;
    WORD         wStackBottom;
    WORD         wcEvents;
    HGLOBAL16    hQueue;
    char         szModule[MAX_MODULE_NAME + 1];
    WORD         wPSPOffset;
    HANDLE16     hNext;
} TASKENTRY, *LPTASKENTRY;

BOOL16 TaskFirst(LPTASKENTRY lpTask);
BOOL16 TaskNext(LPTASKENTRY lpTask);
BOOL16 TaskFindHandle(LPTASKENTRY lpTask, HTASK16 hTask);
DWORD TaskSetCSIP(HTASK16 hTask, WORD wCS, WORD wIP);
DWORD TaskGetCSIP(HTASK16 hTask);
BOOL16 TaskSwitch(HTASK16 hTask, DWORD dwNewCSIP);

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
    DWORD     dwSize;
    WORD      wUserFreePercent;
    WORD      wGDIFreePercent;
    HGLOBAL16 hUserSegment;
    HGLOBAL16 hGDISegment;
} SYSHEAPINFO;

BOOL16 MemManInfo(LPMEMMANINFO lpEnhMode);
BOOL16 SystemHeapInfo( SYSHEAPINFO *pHeapInfo );

/* timer info */

typedef struct tagTIMERINFO {
	DWORD dwSize;
	DWORD dwmsSinceStart;
	DWORD dwmsThisVM;
} TIMERINFO;

BOOL16 TimerCount( TIMERINFO *pTimerInfo );

/* Window classes */

typedef struct
{
    DWORD     dwSize;
    HMODULE16 hInst;              /* This is really an hModule */
    char      szClassName[MAX_CLASSNAME + 1];
    HANDLE16  wNext;
} CLASSENTRY;

BOOL16 ClassFirst( CLASSENTRY *pClassEntry );
BOOL16 ClassNext( CLASSENTRY *pClassEntry );


/* Memory read/write */

DWORD MemoryRead( WORD sel, DWORD offset, void *buffer, DWORD count );
DWORD MemoryWrite( WORD sel, DWORD offset, void *buffer, DWORD count );

/* flags to NotifyRegister() */
#define NF_NORMAL	0	/* everything except taskswitches, debugerrors,
				 * debugstrings
				 */
#define NF_TASKSWITCH	1	/* get taskswitch information */
#define NF_RIP		2	/* get debugerrors of system */

BOOL16 NotifyRegister(HTASK16 htask,FARPROC16 lpfnCallback,WORD wFlags);

#define NFY_UNKNOWN	0
#define NFY_LOADSEG	1
/* DATA is a pointer to following struct: */
typedef struct {
	DWORD	dwSize;
	WORD	wSelector;
	WORD	wSegNum;
	WORD	wType;		/* bit 0 set if this is a code segment */
	WORD	wcInstance;	/* only valid for data segment */
} NFYLOADSEG;
/* called when freeing a segment. LOWORD(dwData) is the freed selector */
#define NFY_FREESEG	2

/* called when loading/starting a DLL */
#define NFY_STARTDLL	3
typedef struct {
    DWORD      dwSize;
    HMODULE16  hModule;
    WORD       wCS;
    WORD       wIP;
} NFYSTARTDLL;

/* called when starting a task. dwData is CS:IP */
#define NFY_STARTTASK	4

/* called when a task terminates. dwData is the return code */
#define NFY_EXITTASK	5

/* called when module is removed. LOWORD(dwData) is the handle */
#define NFY_DELMODULE	6

/* RIP? debugevent */
#define NFY_RIP		7
typedef struct {
	DWORD	dwSize;
	WORD	wIP;
	WORD	wCS;
	WORD	wSS;
	WORD	wBP;
	WORD	wExitCode;
} NFYRIP;

/* called before (after?) switching to a task
 * no data, callback should call GetCurrentTask
 */
#define	NFY_TASKIN	8

/* called before(after?) switching from a task
 * no data, callback should call GetCurrentTask
*/
#define NFY_TASKOUT	9

/* returns ASCII input value, dwData not set */
#define NFY_INCHAR	10

/* output debugstring (pointed to by dwData) */
#define NFY_OUTSTRING	11

/* log errors */
#define NFY_LOGERROR	12
typedef struct {
	DWORD	dwSize;
	UINT16	wErrCode;
	VOID   *lpInfo; /* depends on wErrCode */
} NFYLOGERROR;

/* called for parameter errors? */
#define NFY_LOGPARAMERROR	13
typedef struct {
    DWORD       dwSize;
    UINT16      wErrCode;
    FARPROC16   lpfnErrorAddr;
    void      **lpBadParam;
} NFYLOGPARAMERROR;

#pragma pack(4)

#endif /* __WINE_TOOLHELP_H */
