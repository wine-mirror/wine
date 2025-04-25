/*
 * Toolhelp functions
 *
 * Copyright 1996 Marcus Meissner
 * Copyright 2009 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wownt32.h"

#include "wine/winbase16.h"
#include "toolhelp.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(toolhelp);

#pragma pack(push,1)

typedef struct
{
    void     *base;          /* Base address (0 if discarded) */
    DWORD     size;          /* Size in bytes (0 indicates a free block) */
    HGLOBAL16 handle;        /* Handle for this block */
    HGLOBAL16 hOwner;        /* Owner of this block */
    BYTE      lockCount;     /* Count of GlobalFix() calls */
    BYTE      pageLockCount; /* Count of GlobalPageLock() calls */
    BYTE      flags;         /* Allocation flags */
    BYTE      selCount;      /* Number of selectors allocated for this block */
} GLOBALARENA;

#define GLOBAL_MAX_COUNT  8192        /* Max number of allocated blocks */

typedef struct
{
    WORD check;                 /* 00 Heap checking flag */
    WORD freeze;                /* 02 Heap frozen flag */
    WORD items;                 /* 04 Count of items on the heap */
    WORD first;                 /* 06 First item of the heap */
    WORD pad1;                  /* 08 Always 0 */
    WORD last;                  /* 0a Last item of the heap */
    WORD pad2;                  /* 0c Always 0 */
    BYTE ncompact;              /* 0e Compactions counter */
    BYTE dislevel;              /* 0f Discard level */
    DWORD distotal;             /* 10 Total bytes discarded */
    WORD htable;                /* 14 Pointer to handle table */
    WORD hfree;                 /* 16 Pointer to free handle table */
    WORD hdelta;                /* 18 Delta to expand the handle table */
    WORD expand;                /* 1a Pointer to expand function (unused) */
    WORD pstat;                 /* 1c Pointer to status structure (unused) */
    FARPROC16 notify;           /* 1e Pointer to LocalNotify() function */
    WORD lock;                  /* 22 Lock count for the heap */
    WORD extra;                 /* 24 Extra bytes to allocate when expanding */
    WORD minsize;               /* 26 Minimum size of the heap */
    WORD magic;                 /* 28 Magic number */
} LOCALHEAPINFO;

typedef struct
{
/* Arena header */
    WORD prev;          /* Previous arena | arena type */
    WORD next;          /* Next arena */
/* Start of the memory block or free-list info */
    WORD size;          /* Size of the free block */
    WORD free_prev;     /* Previous free block */
    WORD free_next;     /* Next free block */
} LOCALARENA;

#define LOCAL_ARENA_HEADER_SIZE      4
#define LOCAL_ARENA_HEADER( handle) ((handle) - LOCAL_ARENA_HEADER_SIZE)
#define LOCAL_ARENA_PTR(ptr,arena)  ((LOCALARENA *)((char *)(ptr)+(arena)))

#define MOVEABLE_PREFIX sizeof(HLOCAL16)

/* Layout of a handle entry table
 *
 * WORD                     count of entries
 * LOCALHANDLEENTRY[count]  entries
 * WORD                     near ptr to next table
 */
typedef struct
{
    WORD addr;                /* Address of the MOVEABLE block */
    BYTE flags;               /* Flags for this block */
    BYTE lock;                /* Lock count */
} LOCALHANDLEENTRY;


typedef struct
{
    WORD null;        /* Always 0 */
    DWORD old_ss_sp;  /* Stack pointer; used by SwitchTaskTo() */
    WORD heap;        /* Pointer to the local heap information (if any) */
    WORD atomtable;   /* Pointer to the local atom table (if any) */
    WORD stacktop;    /* Top of the stack */
    WORD stackmin;    /* Lowest stack address used so far */
    WORD stackbottom; /* Bottom of the stack */
} INSTANCEDATA;

typedef struct _THHOOK
{
    HANDLE16   hGlobalHeap;         /* 00 (handle BURGERMASTER) */
    WORD       pGlobalHeap;         /* 02 (selector BURGERMASTER) */
    HMODULE16  hExeHead;            /* 04 hFirstModule */
    HMODULE16  hExeSweep;           /* 06 (unused) */
    HANDLE16   TopPDB;              /* 08 (handle of KERNEL PDB) */
    HANDLE16   HeadPDB;             /* 0A (first PDB in list) */
    HANDLE16   TopSizePDB;          /* 0C (unused) */
    HTASK16    HeadTDB;             /* 0E hFirstTask */
    HTASK16    CurTDB;              /* 10 hCurrentTask */
    HTASK16    LoadTDB;             /* 12 (unused) */
    HTASK16    LockTDB;             /* 14 hLockedTask */
} THHOOK;

typedef struct _NE_MODULE
{
    WORD      ne_magic;         /* 00 'NE' signature */
    WORD      count;            /* 02 Usage count (ne_ver/ne_rev on disk) */
    WORD      ne_enttab;        /* 04 Near ptr to entry table */
    HMODULE16 next;             /* 06 Selector to next module (ne_cbenttab on disk) */
    WORD      dgroup_entry;     /* 08 Near ptr to segment entry for DGROUP (ne_crc on disk) */
    WORD      fileinfo;         /* 0a Near ptr to file info (OFSTRUCT) (ne_crc on disk) */
    WORD      ne_flags;         /* 0c Module flags */
    WORD      ne_autodata;      /* 0e Logical segment for DGROUP */
    WORD      ne_heap;          /* 10 Initial heap size */
    WORD      ne_stack;         /* 12 Initial stack size */
    DWORD     ne_csip;          /* 14 Initial cs:ip */
    DWORD     ne_sssp;          /* 18 Initial ss:sp */
    WORD      ne_cseg;          /* 1c Number of segments in segment table */
    WORD      ne_cmod;          /* 1e Number of module references */
    WORD      ne_cbnrestab;     /* 20 Size of non-resident names table */
    WORD      ne_segtab;        /* 22 Near ptr to segment table */
    WORD      ne_rsrctab;       /* 24 Near ptr to resource table */
    WORD      ne_restab;        /* 26 Near ptr to resident names table */
    WORD      ne_modtab;        /* 28 Near ptr to module reference table */
    WORD      ne_imptab;        /* 2a Near ptr to imported names table */
    DWORD     ne_nrestab;       /* 2c File offset of non-resident names table */
    WORD      ne_cmovent;       /* 30 Number of moveable entries in entry table*/
    WORD      ne_align;         /* 32 Alignment shift count */
    WORD      ne_cres;          /* 34 # of resource segments */
    BYTE      ne_exetyp;        /* 36 Operating system flags */
    BYTE      ne_flagsothers;   /* 37 Misc. flags */
    HANDLE16  dlls_to_init;     /* 38 List of DLLs to initialize (ne_pretthunks on disk) */
    HANDLE16  nrname_handle;    /* 3a Handle to non-resident name table (ne_psegrefbytes on disk) */
    WORD      ne_swaparea;      /* 3c Min. swap area size */
    WORD      ne_expver;        /* 3e Expected Windows version */
    /* From here, these are extra fields not present in normal Windows */
    HMODULE   module32;         /* PE module handle for Win32 modules */
    HMODULE   owner32;          /* PE module containing this one for 16-bit builtins */
    HMODULE16 self;             /* Handle for this module */
    WORD      self_loading_sel; /* Selector used for self-loading apps. */
    LPVOID    rsrc32_map;       /* HRSRC 16->32 map (for 32-bit modules) */
    LPCVOID   mapping;          /* mapping of the binary file */
    SIZE_T    mapping_size;     /* size of the file mapping */
} NE_MODULE;

#pragma pack(pop)

#define TDB_MAGIC    ('T' | ('D' << 8))

/* FIXME: to make this work, we have to call back all these registered
 * functions from all over the WINE code. Someone with more knowledge than
 * me please do that. -Marcus
 */

static struct notify
{
    HTASK16   htask;
    FARPROC16 lpfnCallback;
    WORD     wFlags;
} *notifys = NULL;

static int nrofnotifys = 0;

static THHOOK *get_thhook(void)
{
    static THHOOK *thhook;

    if (!thhook) thhook = MapSL( (SEGPTR)GetProcAddress16( GetModuleHandle16("KERNEL"), (LPCSTR)332 ));
    return thhook;
}

static GLOBALARENA *get_global_arena(void)
{
    return *(GLOBALARENA **)get_thhook();
}

static LOCALHEAPINFO *get_local_heap( HANDLE16 ds )
{
    INSTANCEDATA *ptr = MapSL( MAKESEGPTR( ds, 0 ));

    if (!ptr || !ptr->heap) return NULL;
    return (LOCALHEAPINFO*)((char*)ptr + ptr->heap);
}


/***********************************************************************
 *           GlobalHandleToSel   (TOOLHELP.50)
 */
WORD WINAPI GlobalHandleToSel16( HGLOBAL16 handle )
{
    if (!handle) return 0;
    if (!(handle & 7)) return handle - 1;
    return handle | 7;
}


/***********************************************************************
 *           GlobalFirst   (TOOLHELP.51)
 */
BOOL16 WINAPI GlobalFirst16( GLOBALENTRY *pGlobal, WORD wFlags )
{
    if (wFlags == GLOBAL_LRU) return FALSE;
    pGlobal->dwNext = 0;
    return GlobalNext16( pGlobal, wFlags );
}


/***********************************************************************
 *           GlobalNext   (TOOLHELP.52)
 */
BOOL16 WINAPI GlobalNext16( GLOBALENTRY *pGlobal, WORD wFlags)
{
    GLOBALARENA *pGlobalArena = get_global_arena();
    GLOBALARENA *pArena;

    if (pGlobal->dwNext >= GLOBAL_MAX_COUNT) return FALSE;
    pArena = pGlobalArena + pGlobal->dwNext;
    if (wFlags == GLOBAL_FREE)  /* only free blocks */
    {
        int i;
        for (i = pGlobal->dwNext; i < GLOBAL_MAX_COUNT; i++, pArena++)
            if (pArena->size == 0) break;  /* block is free */
        if (i >= GLOBAL_MAX_COUNT) return FALSE;
        pGlobal->dwNext = i;
    }

    pGlobal->dwAddress    = (DWORD_PTR)pArena->base;
    pGlobal->dwBlockSize  = pArena->size;
    pGlobal->hBlock       = pArena->handle;
    pGlobal->wcLock       = pArena->lockCount;
    pGlobal->wcPageLock   = pArena->pageLockCount;
    pGlobal->wFlags       = (GetCurrentPDB16() == pArena->hOwner);
    pGlobal->wHeapPresent = FALSE;
    pGlobal->hOwner       = pArena->hOwner;
    pGlobal->wType        = GT_UNKNOWN;
    pGlobal->wData        = 0;
    pGlobal->dwNext++;
    return TRUE;
}


/***********************************************************************
 *           GlobalInfo   (TOOLHELP.53)
 */
BOOL16 WINAPI GlobalInfo16( GLOBALINFO *pInfo )
{
    GLOBALARENA *pGlobalArena = get_global_arena();
    GLOBALARENA *pArena;
    int i;

    pInfo->wcItems = GLOBAL_MAX_COUNT;
    pInfo->wcItemsFree = 0;
    pInfo->wcItemsLRU = 0;
    for (i = 0, pArena = pGlobalArena; i < GLOBAL_MAX_COUNT; i++, pArena++)
        if (pArena->size == 0) pInfo->wcItemsFree++;
    return TRUE;
}


/***********************************************************************
 *           GlobalEntryHandle   (TOOLHELP.54)
 */
BOOL16 WINAPI GlobalEntryHandle16( GLOBALENTRY *pGlobal, HGLOBAL16 hItem )
{
    GLOBALARENA *pGlobalArena = get_global_arena();
    GLOBALARENA *pArena = pGlobalArena + (hItem >> __AHSHIFT);

    pGlobal->dwAddress    = (DWORD_PTR)pArena->base;
    pGlobal->dwBlockSize  = pArena->size;
    pGlobal->hBlock       = pArena->handle;
    pGlobal->wcLock       = pArena->lockCount;
    pGlobal->wcPageLock   = pArena->pageLockCount;
    pGlobal->wFlags       = (GetCurrentPDB16() == pArena->hOwner);
    pGlobal->wHeapPresent = FALSE;
    pGlobal->hOwner       = pArena->hOwner;
    pGlobal->wType        = GT_UNKNOWN;
    pGlobal->wData        = 0;
    pGlobal->dwNext++;
    return TRUE;
}


/***********************************************************************
 *           GlobalEntryModule   (TOOLHELP.55)
 */
BOOL16 WINAPI GlobalEntryModule16( GLOBALENTRY *pGlobal, HMODULE16 hModule,
                                 WORD wSeg )
{
    FIXME("(%p, 0x%04x, 0x%04x), stub.\n", pGlobal, hModule, wSeg);
    return FALSE;
}


/***********************************************************************
 *           LocalInfo   (TOOLHELP.56)
 */
BOOL16 WINAPI LocalInfo16( LOCALINFO *pLocalInfo, HGLOBAL16 handle )
{
    LOCALHEAPINFO *pInfo = get_local_heap( SELECTOROF(WOWGlobalLock16(handle)) );
    if (!pInfo) return FALSE;
    pLocalInfo->wcItems = pInfo->items;
    return TRUE;
}


/***********************************************************************
 *           LocalFirst   (TOOLHELP.57)
 */
BOOL16 WINAPI LocalFirst16( LOCALENTRY *pLocalEntry, HGLOBAL16 handle )
{
    WORD ds = GlobalHandleToSel16( handle );
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );
    LOCALHEAPINFO *pInfo = get_local_heap( ds );
    if (!pInfo) return FALSE;

    pLocalEntry->hHandle   = pInfo->first + LOCAL_ARENA_HEADER_SIZE;
    pLocalEntry->wAddress  = pLocalEntry->hHandle;
    pLocalEntry->wFlags    = LF_FIXED;
    pLocalEntry->wcLock    = 0;
    pLocalEntry->wType     = LT_NORMAL;
    pLocalEntry->hHeap     = handle;
    pLocalEntry->wHeapType = NORMAL_HEAP;
    pLocalEntry->wNext     = LOCAL_ARENA_PTR(ptr,pInfo->first)->next;
    pLocalEntry->wSize     = pLocalEntry->wNext - pLocalEntry->hHandle;
    return TRUE;
}


/***********************************************************************
 *           LocalNext   (TOOLHELP.58)
 */
BOOL16 WINAPI LocalNext16( LOCALENTRY *pLocalEntry )
{
    WORD ds = GlobalHandleToSel16( pLocalEntry->hHeap );
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );
    LOCALARENA *pArena;
    WORD table, lhandle;
    LOCALHEAPINFO *pInfo = get_local_heap( ds );

    if (!pInfo) return FALSE;
    if (!pLocalEntry->wNext) return FALSE;
    table = pInfo->htable;
    pArena = LOCAL_ARENA_PTR( ptr, pLocalEntry->wNext );
    pLocalEntry->wAddress  = pLocalEntry->wNext + LOCAL_ARENA_HEADER_SIZE;
    pLocalEntry->wFlags    = (pArena->prev & 3) + 1;
    pLocalEntry->wcLock    = 0;
    /* Find the address in the entry tables */
    lhandle = pLocalEntry->wAddress;
    while (table)
    {
        WORD count = *(WORD *)(ptr + table);
        LOCALHANDLEENTRY *pEntry = (LOCALHANDLEENTRY*)(ptr+table+sizeof(WORD));
        for (; count > 0; count--, pEntry++)
            if (pEntry->addr == lhandle + MOVEABLE_PREFIX)
            {
                lhandle = (HLOCAL16)((char *)pEntry - ptr);
                table = 0;
                pLocalEntry->wAddress  = pEntry->addr;
                pLocalEntry->wFlags    = pEntry->flags;
                pLocalEntry->wcLock    = pEntry->lock;
                break;
            }
        if (table) table = *(WORD *)pEntry;
    }
    pLocalEntry->hHandle   = lhandle;
    pLocalEntry->wType     = LT_NORMAL;
    if (pArena->next != pLocalEntry->wNext)  /* last one? */
        pLocalEntry->wNext = pArena->next;
    else
        pLocalEntry->wNext = 0;
    pLocalEntry->wSize     = pLocalEntry->wNext - pLocalEntry->hHandle;
    return TRUE;
}


/**********************************************************************
 *	    ModuleFirst    (TOOLHELP.59)
 */
BOOL16 WINAPI ModuleFirst16( MODULEENTRY *lpme )
{
    lpme->wNext = get_thhook()->hExeHead;
    return ModuleNext16( lpme );
}


/**********************************************************************
 *	    ModuleNext    (TOOLHELP.60)
 */
BOOL16 WINAPI ModuleNext16( MODULEENTRY *lpme )
{
    NE_MODULE *pModule;
    char *name;

    if (!lpme->wNext) return FALSE;
    if (!(pModule = GlobalLock16( GetExePtr(lpme->wNext) ))) return FALSE;
    name = (char *)pModule + pModule->ne_restab;
    memcpy( lpme->szModule, name + 1, min(*name, MAX_MODULE_NAME) );
    lpme->szModule[min(*name, MAX_MODULE_NAME)] = '\0';
    lpme->hModule = lpme->wNext;
    lpme->wcUsage = pModule->count;
    name = ((OFSTRUCT *)((char*)pModule + pModule->fileinfo))->szPathName;
    lstrcpynA( lpme->szExePath, name, sizeof(lpme->szExePath) );
    lpme->wNext = pModule->next;
    return TRUE;
}


/**********************************************************************
 *	    ModuleFindName    (TOOLHELP.61)
 */
BOOL16 WINAPI ModuleFindName16( MODULEENTRY *lpme, LPCSTR name )
{
    lpme->wNext = GetModuleHandle16( name );
    return ModuleNext16( lpme );
}


/**********************************************************************
 *	    ModuleFindHandle    (TOOLHELP.62)
 */
BOOL16 WINAPI ModuleFindHandle16( MODULEENTRY *lpme, HMODULE16 hModule )
{
    hModule = GetExePtr( hModule );
    lpme->wNext = hModule;
    return ModuleNext16( lpme );
}


/***********************************************************************
 *           TaskFirst   (TOOLHELP.63)
 */
BOOL16 WINAPI TaskFirst16( TASKENTRY *lpte )
{
    lpte->hNext = get_thhook()->HeadTDB;
    return TaskNext16( lpte );
}


/***********************************************************************
 *           TaskNext   (TOOLHELP.64)
 */
BOOL16 WINAPI TaskNext16( TASKENTRY *lpte )
{
    TDB *pTask;
    INSTANCEDATA *pInstData;

    TRACE_(toolhelp)("(%p): task=%04x\n", lpte, lpte->hNext );
    if (!lpte->hNext) return FALSE;

    /* make sure that task and hInstance are valid (skip initial Wine task !) */
    while (1) {
        pTask = GlobalLock16( lpte->hNext );
        if (!pTask || pTask->magic != TDB_MAGIC) return FALSE;
        if (pTask->hInstance)
            break;
        lpte->hNext = pTask->hNext;
    }
    pInstData = MapSL( MAKESEGPTR( GlobalHandleToSel16(pTask->hInstance), 0 ) );
    lpte->hTask         = lpte->hNext;
    lpte->hTaskParent   = pTask->hParent;
    lpte->hInst         = pTask->hInstance;
    lpte->hModule       = pTask->hModule;
    lpte->wSS           = SELECTOROF( pTask->teb->SystemReserved1[0] );
    lpte->wSP           = OFFSETOF( pTask->teb->SystemReserved1[0] );
    lpte->wStackTop     = pInstData->stacktop;
    lpte->wStackMinimum = pInstData->stackmin;
    lpte->wStackBottom  = pInstData->stackbottom;
    lpte->wcEvents      = pTask->nEvents;
    lpte->hQueue        = pTask->hQueue;
    lstrcpynA( lpte->szModule, pTask->module_name, sizeof(lpte->szModule) );
    lpte->wPSPOffset    = 0x100;  /*??*/
    lpte->hNext         = pTask->hNext;
    return TRUE;
}


/***********************************************************************
 *           TaskFindHandle   (TOOLHELP.65)
 */
BOOL16 WINAPI TaskFindHandle16( TASKENTRY *lpte, HTASK16 hTask )
{
    lpte->hNext = hTask;
    return TaskNext16( lpte );
}


/***********************************************************************
 *           MemManInfo   (TOOLHELP.72)
 */
BOOL16 WINAPI MemManInfo16( MEMMANINFO *info )
{
    SYSTEM_BASIC_INFORMATION sbi;
    MEMORYSTATUS status;

    /*
     * Not unsurprisingly although the documentation says you
     * _must_ provide the size in the dwSize field, this function
     * (under Windows) always fills the structure and returns true.
     */
    NtQuerySystemInformation( SystemBasicInformation, &sbi, sizeof(sbi), NULL );
    GlobalMemoryStatus( &status );
    info->wPageSize            = sbi.PageSize;
    info->dwLargestFreeBlock   = status.dwAvailVirtual;
    info->dwMaxPagesAvailable  = info->dwLargestFreeBlock / info->wPageSize;
    info->dwMaxPagesLockable   = info->dwMaxPagesAvailable;
    info->dwTotalLinearSpace   = status.dwTotalVirtual / info->wPageSize;
    info->dwTotalUnlockedPages = info->dwTotalLinearSpace;
    info->dwFreePages          = info->dwMaxPagesAvailable;
    info->dwTotalPages         = info->dwTotalLinearSpace;
    info->dwFreeLinearSpace    = info->dwMaxPagesAvailable;
    info->dwSwapFilePages      = status.dwTotalPageFile / info->wPageSize;
    return TRUE;
}


/***********************************************************************
 *		NotifyRegister (TOOLHELP.73)
 */
BOOL16 WINAPI NotifyRegister16( HTASK16 htask, FARPROC16 lpfnCallback,
                              WORD wFlags )
{
    int	i;

    FIXME("(%x,%lx,%x), semi-stub.\n",
                      htask, (DWORD)lpfnCallback, wFlags );
    if (!htask) htask = GetCurrentTask();
    for (i=0;i<nrofnotifys;i++)
        if (notifys[i].htask==htask)
            break;
    if (i==nrofnotifys) {
        if (notifys==NULL)
            notifys=HeapAlloc( GetProcessHeap(), 0,
                                               sizeof(struct notify) );
        else
            notifys=HeapReAlloc( GetProcessHeap(), 0, notifys,
                                        sizeof(struct notify)*(nrofnotifys+1));
        if (!notifys) return FALSE;
        nrofnotifys++;
    }
    notifys[i].htask=htask;
    notifys[i].lpfnCallback=lpfnCallback;
    notifys[i].wFlags=wFlags;
    return TRUE;
}

/***********************************************************************
 *		NotifyUnregister (TOOLHELP.74)
 */
BOOL16 WINAPI NotifyUnregister16( HTASK16 htask )
{
    int	i;

    FIXME("(%x), semi-stub.\n", htask );
    if (!htask) htask = GetCurrentTask();
    for (i=nrofnotifys;i--;)
        if (notifys[i].htask==htask)
            break;
    if (i==-1)
        return FALSE;
    memcpy(notifys+i,notifys+(i+1),sizeof(struct notify)*(nrofnotifys-i-1));
    notifys=HeapReAlloc( GetProcessHeap(), 0, notifys,
                                        (nrofnotifys-1)*sizeof(struct notify));
    nrofnotifys--;
    return TRUE;
}

/***********************************************************************
 *		StackTraceCSIPFirst (TOOLHELP.67)
 */
BOOL16 WINAPI StackTraceCSIPFirst16(STACKTRACEENTRY *ste, WORD wSS, WORD wCS, WORD wIP, WORD wBP)
{
    FIXME("(%p, ss %04x, cs %04x, ip %04x, bp %04x): stub.\n", ste, wSS, wCS, wIP, wBP);
    return TRUE;
}

/***********************************************************************
 *		StackTraceFirst (TOOLHELP.66)
 */
BOOL16 WINAPI StackTraceFirst16(STACKTRACEENTRY *ste, HTASK16 Task)
{
    FIXME("(%p, %04x), stub.\n", ste, Task);
    return TRUE;
}

/***********************************************************************
 *		StackTraceNext (TOOLHELP.68)
 */
BOOL16 WINAPI StackTraceNext16(STACKTRACEENTRY *ste)
{
    FIXME("(%p), stub.\n", ste);
    return TRUE;
}

/***********************************************************************
 *		InterruptRegister (TOOLHELP.75)
 */
BOOL16 WINAPI InterruptRegister16( HTASK16 task, FARPROC callback )
{
    FIXME("(%04x, %p), stub.\n", task, callback);
    return TRUE;
}

/***********************************************************************
 *		InterruptUnRegister (TOOLHELP.76)
 */
BOOL16 WINAPI InterruptUnRegister16( HTASK16 task )
{
    FIXME("(%04x), stub.\n", task);
    return TRUE;
}

/***********************************************************************
 *           TerminateApp   (TOOLHELP.77)
 *
 * See "Undocumented Windows".
 */
void WINAPI TerminateApp16(HTASK16 hTask, WORD wFlags)
{
    if (hTask && hTask != GetCurrentTask())
    {
        FIXME("cannot terminate task %x\n", hTask);
        return;
    }

#if 0  /* FIXME */
    /* check undocumented flag */
    if (!(wFlags & 0x8000))
        TASK_CallTaskSignalProc( USIG16_TERMINATION, hTask );
#endif

    /* UndocWin says to call int 0x21/0x4c exit=0xff here,
       but let's just call ExitThread */
    ExitThread(0xff);
}

/***********************************************************************
 *           MemoryRead   (TOOLHELP.78)
 */
DWORD WINAPI MemoryRead16( WORD sel, DWORD offset, void *buffer, DWORD count )
{
    char *base = (char *)GetSelectorBase( sel );
    DWORD limit = GetSelectorLimit16( sel );

    if (offset > limit) return 0;
    if (offset + count > limit + 1) count = limit + 1 - offset;
    memcpy( buffer, base + offset, count );
    return count;
}


/***********************************************************************
 *           MemoryWrite   (TOOLHELP.79)
 */
DWORD WINAPI MemoryWrite16( WORD sel, DWORD offset, void *buffer, DWORD count )
{
    char *base = (char *)GetSelectorBase( sel );
    DWORD limit = GetSelectorLimit16( sel );

    if (offset > limit) return 0;
    if (offset + count > limit) count = limit + 1 - offset;
    memcpy( base + offset, buffer, count );
    return count;
}

/***********************************************************************
 *           TimerCount   (TOOLHELP.80)
 */
BOOL16 WINAPI TimerCount16( TIMERINFO *pTimerInfo )
{
    /* FIXME
     * In standard mode, dwmsSinceStart = dwmsThisVM
     *
     * I tested this, under Windows in enhanced mode, and
     * if you never switch VM (ie start/stop DOS) these
     * values should be the same as well.
     *
     * Also, Wine should adjust for the hardware timer
     * to reduce the amount of error to ~1ms.
     * I can't be bothered, can you?
     */
    pTimerInfo->dwmsSinceStart = pTimerInfo->dwmsThisVM = GetTickCount();
    return TRUE;
}

/***********************************************************************
 *           SystemHeapInfo   (TOOLHELP.71)
 */
BOOL16 WINAPI SystemHeapInfo16( SYSHEAPINFO *pHeapInfo )
{
    HANDLE16 oldDS = CURRENT_DS;
    WORD user = LoadLibrary16( "USER.EXE" );
    WORD gdi = LoadLibrary16( "GDI.EXE" );

    CURRENT_DS = user;
    pHeapInfo->wUserFreePercent = (int)LocalCountFree16() * 100 / LocalHeapSize16();
    CURRENT_DS = gdi;
    pHeapInfo->wGDIFreePercent  = (int)LocalCountFree16() * 100 / LocalHeapSize16();
    CURRENT_DS = oldDS;
    pHeapInfo->hUserSegment = user;
    pHeapInfo->hGDISegment  = gdi;
    FreeLibrary16( user );
    FreeLibrary16( gdi );
    return TRUE;
}

/***********************************************************************
 *           Local32Info   (TOOLHELP.84)
 */
BOOL16 WINAPI Local32Info16( LOCAL32INFO *pLocal32Info, HGLOBAL16 handle )
{
    FIXME( "Call Local32Info16 in kernel\n" );
    return FALSE;
}

/***********************************************************************
 *           Local32First   (TOOLHELP.85)
 */
BOOL16 WINAPI Local32First16( LOCAL32ENTRY *pLocal32Entry, HGLOBAL16 handle )
{
    FIXME( "Call Local32First16 in kernel\n" );
    return FALSE;
}

/***********************************************************************
 *           Local32Next   (TOOLHELP.86)
 */
BOOL16 WINAPI Local32Next16( LOCAL32ENTRY *pLocal32Entry )
{
    FIXME( "Call Local32Next16 in kernel\n" );
    return FALSE;
}
