/*
 * Window procedure callbacks
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1996 Alexandre Julliard
 */

#include <stdio.h>
#include "windows.h"
#include "callback.h"
#include "heap.h"
#include "selectors.h"
#include "stackframe.h"
#include "struct32.h"
#include "win.h"
#include "winproc.h"
#include "stddebug.h"
#include "debug.h"

/* Window procedure 16-bit thunk; see BuildSpec16Files() in tools/build.c */
typedef struct
{
    BYTE       popl_eax;             /* popl  %eax (return address) */
    BYTE       pushl_func;           /* pushl $proc */
    WNDPROC32  proc WINE_PACKED;
    BYTE       pushl_eax;            /* pushl %eax */
    WORD       pushw_bp WINE_PACKED; /* pushw %bp */
    BYTE       pushl_thunk;          /* pushl $thunkfrom16 */
    void     (*thunk32)() WINE_PACKED;
    BYTE       lcall;                /* lcall cs:relay */
    void     (*relay)() WINE_PACKED;
    WORD       cs WINE_PACKED;
} WINPROC_THUNK_FROM16;

/* Window procedure 32-bit thunk; see BuildSpec32Files() in tools/build.c */
typedef struct
{
    BYTE       popl_eax;             /* popl  %eax (return address) */
    BYTE       pushl_func;           /* pushl $proc */
    WNDPROC16  proc WINE_PACKED;
    BYTE       pushl_eax;            /* pushl %eax */
    BYTE       pushl_ebp;            /* pushl %ebp */
    BYTE       pushl_name;           /* pushl $name */
    LPCSTR     name WINE_PACKED;
    BYTE       pushl_thunk;          /* pushl $thunkfrom32 */
    void     (*thunk32)() WINE_PACKED;
    BYTE       jmp;                  /* jmp   relay (relative jump)*/
    void     (*relay)() WINE_PACKED;
} WINPROC_THUNK_FROM32;

/* Simple jmp to call 32-bit procedure directly */
typedef struct
{
    BYTE       jmp;                  /* jmp  proc (relative jump) */
    WNDPROC32  proc WINE_PACKED;
} WINPROC_JUMP;

typedef union
{
    WINPROC_THUNK_FROM16  t_from16;
    WINPROC_THUNK_FROM32  t_from32;
} WINPROC_THUNK;

typedef struct tagWINDOWPROC
{
    WINPROC_THUNK         thunk;    /* Thunk */
    WINPROC_JUMP          jmp;      /* Jump */
    struct tagWINDOWPROC *next;     /* Next window proc */
    UINT32                magic;    /* Magic number */
    WINDOWPROCTYPE        type;     /* Function type */
} WINDOWPROC;

#define WINPROC_MAGIC  ('W' | ('P' << 8) | ('R' << 16) | ('C' << 24))

#define WINPROC_THUNKPROC(pproc) \
    (((pproc)->type == WIN_PROC_16) ? \
          (WNDPROC16)((pproc)->thunk.t_from32.proc) : \
          (WNDPROC16)((pproc)->thunk.t_from16.proc))

LRESULT WINPROC_CallProc16To32A( HWND16 hwnd, UINT16 msg,
                                 WPARAM16 wParam, LPARAM lParam,
                                 WNDPROC32 func );
LRESULT WINPROC_CallProc16To32W( HWND16 hwnd, UINT16 msg,
                                 WPARAM16 wParam, LPARAM lParam,
                                 WNDPROC32 func );
static LRESULT WINPROC_CallProc32ATo16( WNDPROC16 func, HWND32 hwnd,
                                        UINT32 msg, WPARAM32 wParam,
                                        LPARAM lParam );
static LRESULT WINPROC_CallProc32WTo16( WNDPROC16 func, HWND32 hwnd,
                                        UINT32 msg, WPARAM32 wParam,
                                        LPARAM lParam );

#ifndef WINELIB
extern void CallFrom16_long_wwwll(void);
extern void CallFrom32_stdcall_5(void);
#else
static void CallFrom16_long_wwwll(void) {}
static void CallFrom32_stdcall_5(void) {}
#endif  /* WINELIB */

static HANDLE32 WinProcHeap;

/**********************************************************************
 *	     WINPROC_Init
 */
BOOL32 WINPROC_Init(void)
{
    WinProcHeap = HeapCreate( HEAP_WINE_SEGPTR | HEAP_WINE_CODESEG, 0, 0 );
    if (!WinProcHeap)
    {
        fprintf( stderr, "Unable to create winproc heap\n" );
        return FALSE;
    }
    return TRUE;
}


/**********************************************************************
 *	     WINPROC_GetPtr
 *
 * Return a pointer to the win proc.
 */
static WINDOWPROC *WINPROC_GetPtr( WNDPROC16 handle )
{
    BYTE *ptr;
    WINDOWPROC *proc;

    /* Check for a linear pointer */

    if (HEAP_IsInsideHeap( WinProcHeap, 0, (LPVOID)handle ))
    {
        ptr = (BYTE *)handle;
        /* First check if it is the jmp address */
        if (*ptr == 0xe9 /* jmp */) ptr -= (int)&((WINDOWPROC *)0)->jmp -
                                           (int)&((WINDOWPROC *)0)->thunk;
        /* Now it must be the thunk address */
        if (*ptr == 0x58 /* popl eax */) ptr -= (int)&((WINDOWPROC *)0)->thunk;
        /* Now we have a pointer to the WINDOWPROC struct */
        if (((WINDOWPROC *)ptr)->magic == WINPROC_MAGIC)
            return (WINDOWPROC *)ptr;
    }

    /* Check for a segmented pointer */

    if (!IsBadReadPtr16((SEGPTR)handle,sizeof(WINDOWPROC)-sizeof(proc->thunk)))
    {
        ptr = (BYTE *)PTR_SEG_TO_LIN(handle);
        if (!HEAP_IsInsideHeap( WinProcHeap, 0, ptr )) return NULL;
        /* It must be the thunk address */
        if (*ptr == 0x58 /* popl eax */) ptr -= (int)&((WINDOWPROC *)0)->thunk;
        /* Now we have a pointer to the WINDOWPROC struct */
        if (((WINDOWPROC *)ptr)->magic == WINPROC_MAGIC)
            return (WINDOWPROC *)ptr;
    }

    return NULL;
}


/**********************************************************************
 *	     WINPROC_AllocWinProc
 *
 * Allocate a new window procedure.
 */
static WINDOWPROC *WINPROC_AllocWinProc( WNDPROC16 func, WINDOWPROCTYPE type )
{
    WINDOWPROC *proc, *oldproc;

    /* Allocate a window procedure */

    if (!(proc = HeapAlloc( WinProcHeap, 0, sizeof(WINDOWPROC) ))) return 0;

    /* Check if the function is already a win proc */

    if ((oldproc = WINPROC_GetPtr( func )))
    {
        *proc = *oldproc;
    }
    else
    {
        switch(type)
        {
        case WIN_PROC_16:
            proc->thunk.t_from32.popl_eax    = 0x58;   /* popl  %eax */
            proc->thunk.t_from32.pushl_func  = 0x68;   /* pushl $proc */
            proc->thunk.t_from32.proc        = func;
            proc->thunk.t_from32.pushl_eax   = 0x50;   /* pushl %eax */
            proc->thunk.t_from32.pushl_ebp   = 0x55;   /* pushl %ebp */
            proc->thunk.t_from32.pushl_name  = 0x68;   /* pushl $name */
            proc->thunk.t_from32.name        = "WINPROC_CallProc32ATo16";
            proc->thunk.t_from32.pushl_thunk = 0x68;   /* pushl $thunkfrom32 */
            proc->thunk.t_from32.thunk32     = (void(*)())WINPROC_CallProc32ATo16;
            proc->thunk.t_from32.jmp         = 0xe9;   /* jmp   relay*/
            proc->thunk.t_from32.relay =  /* relative jump */
                (void (*)())((DWORD)CallFrom32_stdcall_5 -
                                     (DWORD)(&proc->thunk.t_from32.relay + 1));
            break;
        case WIN_PROC_32A:
        case WIN_PROC_32W:
            proc->thunk.t_from16.popl_eax    = 0x58;   /* popl  %eax */
            proc->thunk.t_from16.pushl_func  = 0x68;   /* pushl $proc */
            proc->thunk.t_from16.proc        = (FARPROC32)func;
            proc->thunk.t_from16.pushl_eax   = 0x50;   /* pushl %eax */
            proc->thunk.t_from16.pushw_bp    = 0x5566; /* pushw %bp */
            proc->thunk.t_from16.pushl_thunk = 0x68;   /* pushl $thunkfrom16 */
            proc->thunk.t_from16.thunk32     = (type == WIN_PROC_32A) ?
                                           (void(*)())WINPROC_CallProc16To32A :
                                           (void(*)())WINPROC_CallProc16To32W;
            proc->thunk.t_from16.lcall       = 0x9a;   /* lcall cs:relay */
            proc->thunk.t_from16.relay       = CallFrom16_long_wwwll;
            proc->thunk.t_from16.cs          = WINE_CODE_SELECTOR;
            proc->jmp.jmp  = 0xe9;
            /* Fixup relative jump */
            proc->jmp.proc = (WNDPROC32)((DWORD)func -
                                                 (DWORD)(&proc->jmp.proc + 1));
            break;
        default:
            /* Should not happen */
            break;
        }
        proc->magic = WINPROC_MAGIC;
        proc->type  = type;
    }
    proc->next  = NULL;
    dprintf_win( stddeb, "WINPROC_AllocWinProc(%08x,%d): returning %08x\n",
                 (UINT32)func, type, (UINT32)proc );
    return proc;
}


/**********************************************************************
 *	     WINPROC_GetProc
 *
 * Get a window procedure pointer that can be passed to the Windows program.
 */
WNDPROC16 WINPROC_GetProc( HWINDOWPROC proc, WINDOWPROCTYPE type )
{
    if (type == WIN_PROC_16)  /* We want a 16:16 address */
    {
        if (((WINDOWPROC *)proc)->type == WIN_PROC_16)
            return ((WINDOWPROC *)proc)->thunk.t_from32.proc;
        else
            return (WNDPROC16)HEAP_GetSegptr( WinProcHeap, 0,
                                              &((WINDOWPROC *)proc)->thunk );
    }
    else  /* We want a 32-bit address */
    {
        if (((WINDOWPROC *)proc)->type == WIN_PROC_16)
            return (WNDPROC16)&((WINDOWPROC *)proc)->thunk;
        else
            return (WNDPROC16)&((WINDOWPROC *)proc)->jmp;
    }
}


/**********************************************************************
 *	     WINPROC_SetProc
 *
 * Set the window procedure for a window or class.
 */
BOOL32 WINPROC_SetProc( HWINDOWPROC *pFirst, WNDPROC16 func,
                        WINDOWPROCTYPE type )
{
    WINDOWPROC *proc, **ppPrev;

    /* Check if function is already in the list */

    ppPrev = (WINDOWPROC **)pFirst;
    proc = WINPROC_GetPtr( func );
    while (*ppPrev)
    {
        if (proc)
        {
            if (*ppPrev == proc) break;
        }
        else
        {
            if (((*ppPrev)->type == type) &&
                (func == WINPROC_THUNKPROC(*ppPrev))) break;
        }
        ppPrev = &(*ppPrev)->next;
    }

    if (*ppPrev)  /* Remove it from the list */
    {
        proc = *ppPrev;
        *ppPrev = proc->next;
    }
    else  /* Allocate a new one */
    {
        if (proc)  /* Was already a win proc */
        {
            type = proc->type;
            func = WINPROC_THUNKPROC(proc);
        }
        proc = WINPROC_AllocWinProc( func, type );
        if (!proc) return FALSE;
    }

    /* Add the win proc at the head of the list */

    dprintf_win( stddeb, "WINPROC_SetProc(%08x,%08x,%d): res=%08x\n",
                 (UINT32)*pFirst, (UINT32)func, type, (UINT32)proc );
    proc->next  = *(WINDOWPROC **)pFirst;
    *(WINDOWPROC **)pFirst = proc;
    return TRUE;
}


/**********************************************************************
 *	     WINPROC_FreeProc
 *
 * Free a list of win procs.
 */
void WINPROC_FreeProc( HWINDOWPROC proc )
{
    while (proc)
    {
        WINDOWPROC *next = ((WINDOWPROC *)proc)->next;
        dprintf_win( stddeb, "WINPROC_FreeProc: freeing %08x\n", (UINT32)proc);
        HeapFree( WinProcHeap, 0, proc );
        proc = next;
    }
}


/**********************************************************************
 *	     WINPROC_GetProcType
 *
 * Return the window procedure type.
 */
WINDOWPROCTYPE WINPROC_GetProcType( HWINDOWPROC proc )
{
    if (!proc ||
        (((WINDOWPROC *)proc)->magic != WINPROC_MAGIC))
        return WIN_PROC_INVALID;
    return ((WINDOWPROC *)proc)->type;
}


/**********************************************************************
 *	     WINPROC_MapMsg32ATo32W
 *
 * Map a message from Ansi to Unicode.
 * Return value is -1 on error, 0 if OK, 1 if an UnmapMsg call is needed.
 */
INT32 WINPROC_MapMsg32ATo32W( UINT32 msg, WPARAM32 wParam, LPARAM *plparam )
{
    switch(msg)
    {
    case WM_GETTEXT:
        {
            LPARAM *ptr = (LPARAM *)HeapAlloc( SystemHeap, 0,
                                     wParam * sizeof(WCHAR) + sizeof(LPARAM) );
            if (!ptr) return -1;
            *ptr++ = *plparam;  /* Store previous lParam */
            *plparam = (LPARAM)ptr;
        }
        return 1;
    case WM_SETTEXT:
        *plparam = (LPARAM)HEAP_strdupAtoW( SystemHeap, 0, (LPCSTR)*plparam );
        return (*plparam ? 1 : -1);
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT32W *cs = (CREATESTRUCT32W *)HeapAlloc( SystemHeap, 0,
                                                                sizeof(*cs) );
            if (!cs) return -1;
            *cs = *(CREATESTRUCT32W *)*plparam;
            if (HIWORD(cs->lpszName))
                cs->lpszName = HEAP_strdupAtoW( SystemHeap, 0,
                                                (LPCSTR)cs->lpszName );
            if (HIWORD(cs->lpszClass))
                cs->lpszClass = HEAP_strdupAtoW( SystemHeap, 0,
                                                 (LPCSTR)cs->lpszClass );
            *plparam = (LPARAM)cs;
        }
        return 1;
    case WM_MDICREATE:
        {
            MDICREATESTRUCT32W *cs =
                (MDICREATESTRUCT32W *)HeapAlloc( SystemHeap, 0, sizeof(*cs) );
            if (!cs) return -1;
            *cs = *(MDICREATESTRUCT32W *)*plparam;
            if (HIWORD(cs->szClass))
                cs->szClass = HEAP_strdupAtoW( SystemHeap, 0,
                                               (LPCSTR)cs->szClass );
            if (HIWORD(cs->szTitle))
                cs->szTitle = HEAP_strdupAtoW( SystemHeap, 0,
                                               (LPCSTR)cs->szTitle );
            *plparam = (LPARAM)cs;
        }
        return 1;
    case WM_ASKCBFORMATNAME:
    case WM_DEVMODECHANGE:
    case WM_MDIACTIVATE:
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    case WM_WININICHANGE:
        fprintf( stderr, "MapMsg32ATo32W: message %04x needs translation\n",
                 msg );
        return -1;
    default:  /* No translation needed */
        return 0;
    }
}


/**********************************************************************
 *	     WINPROC_UnmapMsg32ATo32W
 *
 * Unmap a message that was mapped from Ansi to Unicode.
 */
void WINPROC_UnmapMsg32ATo32W( UINT32 msg, WPARAM32 wParam, LPARAM lParam )
{
    switch(msg)
    {
    case WM_GETTEXT:
        {
            LPARAM *ptr = (LPARAM *)lParam - 1;
            lstrcpynWtoA( (LPSTR)*ptr, (LPWSTR)(ptr + 1), wParam );
            HeapFree( SystemHeap, 0, ptr );
        }
        break;
    case WM_SETTEXT:
        HeapFree( SystemHeap, 0, (void *)lParam );
        break;
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT32W *cs = (CREATESTRUCT32W *)lParam;
            if (HIWORD(cs->lpszName))
                HeapFree( SystemHeap, 0, (LPVOID)cs->lpszName );
            if (HIWORD(cs->lpszClass))
                HeapFree( SystemHeap, 0, (LPVOID)cs->lpszClass );
            HeapFree( SystemHeap, 0, cs );
        }
        break;
    case WM_MDICREATE:
        {
            MDICREATESTRUCT32W *cs = (MDICREATESTRUCT32W *)lParam;
            if (HIWORD(cs->szTitle))
                HeapFree( SystemHeap, 0, (LPVOID)cs->szTitle );
            if (HIWORD(cs->szClass))
                HeapFree( SystemHeap, 0, (LPVOID)cs->szClass );
            HeapFree( SystemHeap, 0, cs );
        }
        break;
    }
}


/**********************************************************************
 *	     WINPROC_MapMsg32WTo32A
 *
 * Map a message from Unicode to Ansi.
 * Return value is -1 on error, 0 if OK, 1 if an UnmapMsg call is needed.
 */
INT32 WINPROC_MapMsg32WTo32A( UINT32 msg, WPARAM32 wParam, LPARAM *plparam )
{
    switch(msg)
    {
    case WM_GETTEXT:
        {
            LPARAM *ptr = (LPARAM *)HeapAlloc( SystemHeap, 0,
                                               wParam + sizeof(LPARAM) );
            if (!ptr) return -1;
            *ptr++ = *plparam;  /* Store previous lParam */
            *plparam = (LPARAM)ptr;
        }
        return 1;
    case WM_SETTEXT:
        *plparam = (LPARAM)HEAP_strdupWtoA( SystemHeap, 0, (LPCWSTR)*plparam );
        return (*plparam ? 1 : -1);
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT32A *cs = (CREATESTRUCT32A *)HeapAlloc( SystemHeap, 0,
                                                                sizeof(*cs) );
            if (!cs) return -1;
            *cs = *(CREATESTRUCT32A *)*plparam;
            if (HIWORD(cs->lpszName))
                cs->lpszName  = HEAP_strdupWtoA( SystemHeap, 0,
                                                 (LPCWSTR)cs->lpszName );
            if (HIWORD(cs->lpszClass))
                cs->lpszClass = HEAP_strdupWtoA( SystemHeap, 0,
                                                 (LPCWSTR)cs->lpszClass);
            *plparam = (LPARAM)cs;
        }
        return 1;
    case WM_MDICREATE:
        {
            MDICREATESTRUCT32A *cs =
                (MDICREATESTRUCT32A *)HeapAlloc( SystemHeap, 0, sizeof(*cs) );
            if (!cs) return -1;
            *cs = *(MDICREATESTRUCT32A *)*plparam;
            if (HIWORD(cs->szTitle))
                cs->szTitle = HEAP_strdupWtoA( SystemHeap, 0,
                                               (LPCWSTR)cs->szTitle );
            if (HIWORD(cs->szClass))
                cs->szClass = HEAP_strdupWtoA( SystemHeap, 0,
                                               (LPCWSTR)cs->szClass );
            *plparam = (LPARAM)cs;
        }
        return 1;
    case WM_ASKCBFORMATNAME:
    case WM_DEVMODECHANGE:
    case WM_MDIACTIVATE:
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    case WM_WININICHANGE:
        fprintf( stderr, "MapMsg32WTo32A: message %04x needs translation\n",
                 msg );
        return -1;
    default:  /* No translation needed */
        return 0;
    }
}


/**********************************************************************
 *	     WINPROC_UnmapMsg32WTo32A
 *
 * Unmap a message that was mapped from Unicode to Ansi.
 */
void WINPROC_UnmapMsg32WTo32A( UINT32 msg, WPARAM32 wParam, LPARAM lParam )
{
    switch(msg)
    {
    case WM_GETTEXT:
        {
            LPARAM *ptr = (LPARAM *)lParam - 1;
            lstrcpynAtoW( (LPWSTR)*ptr, (LPSTR)(ptr + 1), wParam );
            HeapFree( SystemHeap, 0, ptr );
        }
        break;
    case WM_SETTEXT:
        HeapFree( SystemHeap, 0, (void *)lParam );
        break;
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT32A *cs = (CREATESTRUCT32A *)lParam;
            if (HIWORD(cs->lpszName))
                HeapFree( SystemHeap, 0, (LPVOID)cs->lpszName );
            if (HIWORD(cs->lpszClass))
                HeapFree( SystemHeap, 0, (LPVOID)cs->lpszClass );
            HeapFree( SystemHeap, 0, cs );
        }
        break;
    case WM_MDICREATE:
        {
            MDICREATESTRUCT32A *cs = (MDICREATESTRUCT32A *)lParam;
            if (HIWORD(cs->szTitle))
                HeapFree( SystemHeap, 0, (LPVOID)cs->szTitle );
            if (HIWORD(cs->szClass))
                HeapFree( SystemHeap, 0, (LPVOID)cs->szClass );
            HeapFree( SystemHeap, 0, cs );
        }
        break;
    }
}


/**********************************************************************
 *	     WINPROC_MapMsg16To32A
 *
 * Map a message from 16- to 32-bit Ansi.
 * Return value is -1 on error, 0 if OK, 1 if an UnmapMsg call is needed.
 */
INT32 WINPROC_MapMsg16To32A( UINT16 msg16, WPARAM16 wParam16, UINT32 *pmsg32,
                             WPARAM32 *pwparam32, LPARAM *plparam )
{
    *pmsg32 = (UINT32)msg16;
    *pwparam32 = (WPARAM32)wParam16;
    switch(msg16)
    {
    case WM_ACTIVATE:
    case WM_CHARTOITEM:
    case WM_COMMAND:
    case WM_VKEYTOITEM:
        *pwparam32 = MAKEWPARAM( wParam16, HIWORD(*plparam) );
        *plparam   = (LPARAM)(HWND32)LOWORD(*plparam);
        return 0;
    case WM_HSCROLL:
    case WM_VSCROLL:
        *pwparam32 = MAKEWPARAM( wParam16, LOWORD(*plparam) );
        *plparam   = (LPARAM)(HWND32)HIWORD(*plparam);
        return 0;
    case WM_CTLCOLOR:
        *pmsg32    = WM_CTLCOLORMSGBOX + HIWORD(*plparam);
        *pwparam32 = (WPARAM32)(HDC32)wParam16;
        *plparam   = (LPARAM)(HWND32)LOWORD(*plparam);
        return 0;
    case WM_COMPAREITEM:
        {
            COMPAREITEMSTRUCT16* cis16 = (COMPAREITEMSTRUCT16 *)PTR_SEG_TO_LIN(*plparam);
            COMPAREITEMSTRUCT32 *cis = (COMPAREITEMSTRUCT32 *)
                                        HeapAlloc(SystemHeap, 0, sizeof(*cis));
            if (!cis) return -1;
            cis->CtlType    = cis16->CtlType;
            cis->CtlID      = cis16->CtlID;
            cis->hwndItem   = cis16->hwndItem;
            cis->itemID1    = cis16->itemID1;
            cis->itemData1  = cis16->itemData1;
            cis->itemID2    = cis16->itemID2;
            cis->itemData2  = cis16->itemData2;
            cis->dwLocaleId = 0;  /* FIXME */
            *plparam = (LPARAM)cis;
        }
        return 1;
    case WM_DELETEITEM:
        {
            DELETEITEMSTRUCT16* dis16 = (DELETEITEMSTRUCT16 *)PTR_SEG_TO_LIN(*plparam);
            DELETEITEMSTRUCT32 *dis = (DELETEITEMSTRUCT32 *)
                                        HeapAlloc(SystemHeap, 0, sizeof(*dis));
            if (!dis) return -1;
            dis->CtlType  = dis16->CtlType;
            dis->CtlID    = dis16->CtlID;
            dis->hwndItem = dis16->hwndItem;
            dis->itemData = dis16->itemData;
            *plparam = (LPARAM)dis;
        }
        return 1;
    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT16* mis16 = (MEASUREITEMSTRUCT16 *)PTR_SEG_TO_LIN(*plparam);
            MEASUREITEMSTRUCT32 *mis = (MEASUREITEMSTRUCT32 *)
                                        HeapAlloc(SystemHeap, 0,
                                                sizeof(*mis) + sizeof(LPARAM));
            if (!mis) return -1;
            mis->CtlType    = mis16->CtlType;
            mis->CtlID      = mis16->CtlID;
            mis->itemID     = mis16->itemID;
            mis->itemWidth  = mis16->itemWidth;
            mis->itemHeight = mis16->itemHeight;
            mis->itemData   = mis16->itemData;
            *(LPARAM *)(mis + 1) = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)mis;
        }
        return 1;
    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT16* dis16 = (DRAWITEMSTRUCT16 *)PTR_SEG_TO_LIN(*plparam);
            DRAWITEMSTRUCT32 *dis = (DRAWITEMSTRUCT32*)HeapAlloc(SystemHeap, 0,
                                                                 sizeof(*dis));
            if (!dis) return -1;
            dis->CtlType    = dis16->CtlType;
            dis->CtlID      = dis16->CtlID;
            dis->itemID     = dis16->itemID;
            dis->itemAction = dis16->itemAction;
            dis->itemState  = dis16->itemState;
            dis->hwndItem   = dis16->hwndItem;
            dis->hDC        = dis16->hDC;
            dis->itemData   = dis16->itemData;
            CONV_RECT16TO32( &dis16->rcItem, &dis->rcItem );
            *plparam = (LPARAM)dis;
        }
        return 1;
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO32 *mmi = (MINMAXINFO32 *)HeapAlloc( SystemHeap, 0,
                                                sizeof(*mmi) + sizeof(LPARAM));
            if (!mmi) return -1;
            STRUCT32_MINMAXINFO16to32( (MINMAXINFO16*)PTR_SEG_TO_LIN(*plparam),
                                       mmi );
            *(LPARAM *)(mmi + 1) = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)mmi;
        }
        return 1;
    case WM_GETTEXT:
        *plparam = (LPARAM)PTR_SEG_TO_LIN(*plparam);
        return 0;
    case WM_MDICREATE:
        {
            MDICREATESTRUCT16 *cs16 =
                (MDICREATESTRUCT16 *)PTR_SEG_TO_LIN(*plparam);
            MDICREATESTRUCT32A *cs =
                (MDICREATESTRUCT32A *)HeapAlloc( SystemHeap, 0,
                                                sizeof(*cs) + sizeof(LPARAM) );
            if (!cs) return -1;
            STRUCT32_MDICREATESTRUCT16to32A( cs16, cs );
            cs->szTitle = (LPCSTR)PTR_SEG_TO_LIN(cs16->szTitle);
            cs->szClass = (LPCSTR)PTR_SEG_TO_LIN(cs16->szClass);
            *(LPARAM *)(cs + 1) = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)cs;
        }
        return 1;
    case WM_MDISETMENU:
        *pwparam32 = (WPARAM32)(HMENU32)LOWORD(*plparam);
        *plparam   = (LPARAM)(HMENU32)HIWORD(*plparam);
        return 0;
    case WM_MENUCHAR:
    case WM_MENUSELECT:
        *pwparam32 = MAKEWPARAM( wParam16, LOWORD(*plparam) );
        *plparam   = (LPARAM)(HMENU32)HIWORD(*plparam);
        return 0;
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS16 *nc16;
            NCCALCSIZE_PARAMS32 *nc;

            nc = (NCCALCSIZE_PARAMS32 *)HeapAlloc( SystemHeap, 0,
                                                sizeof(*nc) + sizeof(LPARAM) );
            if (!nc) return -1;
            nc16 = (NCCALCSIZE_PARAMS16 *)PTR_SEG_TO_LIN(*plparam);
            CONV_RECT16TO32( &nc16->rgrc[0], &nc->rgrc[0] );
            if (wParam16)
            {
                nc->lppos = (WINDOWPOS32 *)HeapAlloc( SystemHeap, 0,
                                                      sizeof(*nc->lppos) );
                CONV_RECT16TO32( &nc16->rgrc[1], &nc->rgrc[1] );
                CONV_RECT16TO32( &nc16->rgrc[2], &nc->rgrc[2] );
                if (nc->lppos) STRUCT32_WINDOWPOS16to32( (WINDOWPOS16 *)PTR_SEG_TO_LIN(nc16->lppos), nc->lppos );
            }
            *(LPARAM *)(nc + 1) = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)nc;
        }
        return 1;
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT16 *cs16 = (CREATESTRUCT16 *)PTR_SEG_TO_LIN(*plparam);
            CREATESTRUCT32A *cs = (CREATESTRUCT32A *)HeapAlloc( SystemHeap, 0,
                                                sizeof(*cs) + sizeof(LPARAM) );
            if (!cs) return -1;
            STRUCT32_CREATESTRUCT16to32A( cs16, cs );
            cs->lpszName  = (LPCSTR)PTR_SEG_TO_LIN(cs16->lpszName);
            cs->lpszClass = (LPCSTR)PTR_SEG_TO_LIN(cs16->lpszClass);
            *(LPARAM *)(cs + 1) = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)cs;
        }
        return 1;
    case WM_PARENTNOTIFY:
        if ((wParam16 == WM_CREATE) || (wParam16 == WM_DESTROY))
        {
            *pwparam32 = MAKEWPARAM( wParam16, HIWORD(*plparam) );
            *plparam   = (LPARAM)(HWND32)LOWORD(*plparam);
        }
        return 0;
    case WM_SETTEXT:
        *plparam = (LPARAM)PTR_SEG_TO_LIN(*plparam);
        return 0;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS32 *wp = (WINDOWPOS32 *)HeapAlloc( SystemHeap, 0,
                                                sizeof(*wp) + sizeof(LPARAM) );
            if (!wp) return -1;
            STRUCT32_WINDOWPOS16to32( (WINDOWPOS16 *)PTR_SEG_TO_LIN(*plparam),
                                      wp );
            *(LPARAM *)(wp + 1) = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)wp;
        }
        return 1;
    case WM_ASKCBFORMATNAME:
    case WM_DEVMODECHANGE:
    case WM_MDIACTIVATE:
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    case WM_WININICHANGE:
        fprintf( stderr, "MapMsg16To32A: message %04x needs translation\n",
                 msg16 );
        return -1;

    default:  /* No translation needed */
        return 0;
    }
}


/**********************************************************************
 *	     WINPROC_UnmapMsg16To32A
 *
 * Unmap a message that was mapped from 16- to 32-bit Ansi.
 */
void WINPROC_UnmapMsg16To32A( UINT32 msg, WPARAM32 wParam, LPARAM lParam )
{
    switch(msg)
    {
    case WM_COMPAREITEM:
    case WM_DELETEITEM:
    case WM_DRAWITEM:
        HeapFree( SystemHeap, 0, (LPVOID)lParam );
        break;
    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT16 *mis16;
            MEASUREITEMSTRUCT32 *mis = (MEASUREITEMSTRUCT32 *)lParam;
            lParam = *(LPARAM *)(mis + 1);
            mis16 = (MEASUREITEMSTRUCT16 *)PTR_SEG_TO_LIN(lParam);
            mis16->itemWidth  = (UINT16)mis->itemWidth;
            mis16->itemHeight = (UINT16)mis->itemHeight;
            HeapFree( SystemHeap, 0, mis );
        }
        break;
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO32 *mmi = (MINMAXINFO32 *)lParam;
            lParam = *(LPARAM *)(mmi + 1);
            STRUCT32_MINMAXINFO32to16( mmi,
                                       (MINMAXINFO16 *)PTR_SEG_TO_LIN(lParam));
            HeapFree( SystemHeap, 0, mmi );
        }
        break;
    case WM_MDICREATE:
        {
            MDICREATESTRUCT32A *cs = (MDICREATESTRUCT32A *)lParam;
            lParam = *(LPARAM *)(cs + 1);
            STRUCT32_MDICREATESTRUCT32Ato16( cs,
                                 (MDICREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam) );
            HeapFree( SystemHeap, 0, cs );
        }
        break;
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS16 *nc16;
            NCCALCSIZE_PARAMS32 *nc = (NCCALCSIZE_PARAMS32 *)lParam;
            lParam = *(LPARAM *)(nc + 1);
            nc16 = (NCCALCSIZE_PARAMS16 *)PTR_SEG_TO_LIN(lParam);
            CONV_RECT32TO16( &nc->rgrc[0], &nc16->rgrc[0] );
            if (wParam)
            {
                CONV_RECT32TO16( &nc->rgrc[1], &nc16->rgrc[1] );
                CONV_RECT32TO16( &nc->rgrc[2], &nc16->rgrc[2] );
                if (nc->lppos)
                {
                    STRUCT32_WINDOWPOS32to16( nc->lppos,
                                   (WINDOWPOS16 *)PTR_SEG_TO_LIN(nc16->lppos));
                    HeapFree( SystemHeap, 0, nc->lppos );
                }
            }
            HeapFree( SystemHeap, 0, nc );
        }
	break;
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT32A *cs = (CREATESTRUCT32A *)lParam;
            lParam = *(LPARAM *)(cs + 1);
            STRUCT32_CREATESTRUCT32Ato16( cs,
                                    (CREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam) );
            HeapFree( SystemHeap, 0, cs );
        }
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS32 *wp = (WINDOWPOS32 *)lParam;
            lParam = *(LPARAM *)(wp + 1);
            STRUCT32_WINDOWPOS32to16(wp,(WINDOWPOS16 *)PTR_SEG_TO_LIN(lParam));
            HeapFree( SystemHeap, 0, wp );
        }
        break;
    }
}


/**********************************************************************
 *	     WINPROC_MapMsg16To32W
 *
 * Map a message from 16- to 32-bit Unicode.
 * Return value is -1 on error, 0 if OK, 1 if an UnmapMsg call is needed.
 */
INT32 WINPROC_MapMsg16To32W( UINT16 msg16, WPARAM16 wParam16, UINT32 *pmsg32,
                             WPARAM32 *pwparam32, LPARAM *plparam )
{
    switch(msg16)
    {
    case WM_GETTEXT:
    case WM_SETTEXT:
        *plparam = (LPARAM)PTR_SEG_TO_LIN(*plparam);
        return WINPROC_MapMsg32ATo32W( *pmsg32, *pwparam32, plparam );
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT16 *cs16 = (CREATESTRUCT16 *)PTR_SEG_TO_LIN(*plparam);
            CREATESTRUCT32W *cs = (CREATESTRUCT32W *)HeapAlloc( SystemHeap, 0,
                                                sizeof(*cs) + sizeof(LPARAM) );
            if (!cs) return -1;
            STRUCT32_CREATESTRUCT16to32A( cs16, (CREATESTRUCT32A *)cs );
            cs->lpszName  = (LPCWSTR)PTR_SEG_TO_LIN(cs16->lpszName);
            cs->lpszClass = (LPCWSTR)PTR_SEG_TO_LIN(cs16->lpszClass);
            if (HIWORD(cs->lpszName))
                cs->lpszName = HEAP_strdupAtoW( SystemHeap, 0,
                                                (LPCSTR)cs->lpszName );
            if (HIWORD(cs->lpszClass))
                cs->lpszClass = HEAP_strdupAtoW( SystemHeap, 0,
                                                 (LPCSTR)cs->lpszClass );
            *(LPARAM *)(cs + 1) = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)cs;
        }
        return 1;
    case WM_MDICREATE:
        {
            MDICREATESTRUCT16 *cs16 =
                (MDICREATESTRUCT16 *)PTR_SEG_TO_LIN(*plparam);
            MDICREATESTRUCT32W *cs =
                (MDICREATESTRUCT32W *)HeapAlloc( SystemHeap, 0,
                                                sizeof(*cs) + sizeof(LPARAM) );
            if (!cs) return -1;
            STRUCT32_MDICREATESTRUCT16to32A( cs16, (MDICREATESTRUCT32A *)cs );
            cs->szTitle = (LPCWSTR)PTR_SEG_TO_LIN(cs16->szTitle);
            cs->szClass = (LPCWSTR)PTR_SEG_TO_LIN(cs16->szClass);
            if (HIWORD(cs->szTitle))
                cs->szTitle = HEAP_strdupAtoW( SystemHeap, 0,
                                               (LPCSTR)cs->szTitle );
            if (HIWORD(cs->szClass))
                cs->szClass = HEAP_strdupAtoW( SystemHeap, 0,
                                               (LPCSTR)cs->szClass );
            *(LPARAM *)(cs + 1) = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)cs;
        }
        return 1;
    default:  /* No Unicode translation needed */
        return WINPROC_MapMsg16To32A( msg16, wParam16, pmsg32,
                                      pwparam32, plparam );
    }
}


/**********************************************************************
 *	     WINPROC_UnmapMsg16To32W
 *
 * Unmap a message that was mapped from 16- to 32-bit Unicode.
 */
void WINPROC_UnmapMsg16To32W( UINT32 msg, WPARAM32 wParam, LPARAM lParam )
{
    switch(msg)
    {
    case WM_GETTEXT:
    case WM_SETTEXT:
        WINPROC_UnmapMsg32ATo32W( msg, wParam, lParam );
        break;
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT32W *cs = (CREATESTRUCT32W *)lParam;
            lParam = *(LPARAM *)(cs + 1);
            STRUCT32_CREATESTRUCT32Ato16( (CREATESTRUCT32A *)cs,
                                    (CREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam) );
            if (HIWORD(cs->lpszName))
                HeapFree( SystemHeap, 0, (LPVOID)cs->lpszName );
            if (HIWORD(cs->lpszClass))
                HeapFree( SystemHeap, 0, (LPVOID)cs->lpszClass );
            HeapFree( SystemHeap, 0, cs );
        }
        break;
    case WM_MDICREATE:
        {
            MDICREATESTRUCT32W *cs = (MDICREATESTRUCT32W *)lParam;
            lParam = *(LPARAM *)(cs + 1);
            STRUCT32_MDICREATESTRUCT32Ato16( (MDICREATESTRUCT32A *)cs,
                                 (MDICREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam) );
            if (HIWORD(cs->szTitle))
                HeapFree( SystemHeap, 0, (LPVOID)cs->szTitle );
            if (HIWORD(cs->szClass))
                HeapFree( SystemHeap, 0, (LPVOID)cs->szClass );
            HeapFree( SystemHeap, 0, cs );
        }
        break;
    default:
        WINPROC_UnmapMsg16To32A( msg, wParam, lParam );
        break;
    }
}


/**********************************************************************
 *	     WINPROC_MapMsg32ATo16
 *
 * Map a message from 32-bit Ansi to 16-bit.
 * Return value is -1 on error, 0 if OK, 1 if an UnmapMsg call is needed.
 */
INT32 WINPROC_MapMsg32ATo16( UINT32 msg32, WPARAM32 wParam32, UINT16 *pmsg16,
                             WPARAM16 *pwparam16, LPARAM *plparam )
{
    *pmsg16 = (UINT16)msg32;
    *pwparam16 = (WPARAM16)LOWORD(wParam32);
    switch(msg32)
    {
    case BM_GETCHECK32:
    case BM_SETCHECK32:
    case BM_GETSTATE32:
    case BM_SETSTATE32:
    case BM_SETSTYLE32:
        *pmsg16 = (UINT16)msg32 + (BM_GETCHECK16 - BM_GETCHECK32);
        return 0;
    case LB_CARETOFF32:
    case LB_CARETON32:
    case LB_DELETESTRING32:
    case LB_GETANCHORINDEX32:
    case LB_GETCARETINDEX32:
    case LB_GETCOUNT32:
    case LB_GETCURSEL32:
    case LB_GETHORIZONTALEXTENT32:
    case LB_GETITEMDATA32:
    case LB_GETITEMHEIGHT32:
    case LB_GETSEL32:
    case LB_GETSELCOUNT32:
    case LB_GETTEXTLEN32:
    case LB_GETTOPINDEX32:
    case LB_RESETCONTENT32:
    case LB_SELITEMRANGE32:
    case LB_SELITEMRANGEEX32:
    case LB_SETANCHORINDEX32:
    case LB_SETCARETINDEX32:
    case LB_SETCOLUMNWIDTH32:
    case LB_SETCURSEL32:
    case LB_SETHORIZONTALEXTENT32:
    case LB_SETITEMDATA32:
    case LB_SETITEMHEIGHT32:
    case LB_SETSEL32:
    case LB_SETTOPINDEX32:
        *pmsg16 = (UINT16)msg32 + (LB_ADDSTRING16 - LB_ADDSTRING32);
        return 0;
    case LB_ADDSTRING32:
    case LB_FINDSTRING32:
    case LB_FINDSTRINGEXACT32:
    case LB_INSERTSTRING32:
    case LB_SELECTSTRING32:
    case LB_DIR32:
    case LB_ADDFILE32:
/*    case LB_GETTEXT32: FIXME */
        {
            LPSTR str = SEGPTR_STRDUP( (LPSTR)*plparam );
            if (!str) return -1;
            *plparam = (LPARAM)SEGPTR_GET(str);
        }
        *pmsg16 = (UINT16)msg32 + (LB_ADDSTRING16 - LB_ADDSTRING32);
        return 1;
    case LB_GETITEMRECT32:
        {
            RECT16 *rect;
            rect = (RECT16 *)SEGPTR_ALLOC( sizeof(RECT16) + sizeof(LPARAM) );
            if (!rect) return -1;
            *(LPARAM *)(rect + 1) = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)SEGPTR_GET(rect);
        }
        return 1;
    case LB_GETSELITEMS32:
        {
            LPINT16 items;
            *pwparam16 = (WPARAM16)MIN( wParam32, 0x7f80 ); /* Must be < 64K */
            if (!(items = SEGPTR_ALLOC( *pwparam16 * sizeof(INT16)
                                        + sizeof(LPARAM)))) return -1;
            *((LPARAM *)items)++ = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)SEGPTR_GET(items);
        }
        return 1;
    case LB_SETTABSTOPS32:
        if (wParam32)
        {
            INT32 i;
            LPINT16 stops;
            *pwparam16 = (WPARAM16)MIN( wParam32, 0x7f80 ); /* Must be < 64K */
            if (!(stops = SEGPTR_ALLOC( *pwparam16 * sizeof(INT16)
                                        + sizeof(LPARAM)))) return -1;
            for (i = 0; i < *pwparam16; i++) stops[i] = *((LPINT32)*plparam+i);
            *plparam = (LPARAM)SEGPTR_GET(stops);
            return 1;
        }
        *pmsg16 = LB_SETTABSTOPS16;
        return 0;
    case WM_ACTIVATE:
    case WM_CHARTOITEM:
    case WM_COMMAND:
    case WM_VKEYTOITEM:
        *plparam = MAKELPARAM( (HWND16)*plparam, HIWORD(wParam32) );
        return 0;
    case WM_HSCROLL:
    case WM_VSCROLL:
        *plparam = MAKELPARAM( HIWORD(wParam32), (HWND16)*plparam );
        return 0;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        *pmsg16  = WM_CTLCOLOR;
        *plparam = MAKELPARAM( (HWND16)*plparam,
                               (WORD)msg32 - WM_CTLCOLORMSGBOX );
        return 0;
    case WM_COMPAREITEM:
        {
            COMPAREITEMSTRUCT32 *cis32 = (COMPAREITEMSTRUCT32 *)*plparam;
            COMPAREITEMSTRUCT16 *cis = SEGPTR_NEW(COMPAREITEMSTRUCT16);
            if (!cis) return -1;
            cis->CtlType    = (UINT16)cis32->CtlType;
            cis->CtlID      = (UINT16)cis32->CtlID;
            cis->hwndItem   = (HWND16)cis32->hwndItem;
            cis->itemID1    = (UINT16)cis32->itemID1;
            cis->itemData1  = cis32->itemData1;
            cis->itemID2    = (UINT16)cis32->itemID2;
            cis->itemData2  = cis32->itemData2;
            *plparam = (LPARAM)SEGPTR_GET(cis);
        }
        return 1;
    case WM_DELETEITEM:
        {
            DELETEITEMSTRUCT32 *dis32 = (DELETEITEMSTRUCT32 *)*plparam;
            DELETEITEMSTRUCT16 *dis = SEGPTR_NEW(DELETEITEMSTRUCT16);
            if (!dis) return -1;
            dis->CtlType  = (UINT16)dis32->CtlType;
            dis->CtlID    = (UINT16)dis32->CtlID;
            dis->itemID   = (UINT16)dis32->itemID;
            dis->hwndItem = (HWND16)dis32->hwndItem;
            dis->itemData = dis32->itemData;
            *plparam = (LPARAM)SEGPTR_GET(dis);
        }
        return 1;
    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT32 *dis32 = (DRAWITEMSTRUCT32 *)*plparam;
            DRAWITEMSTRUCT16 *dis = SEGPTR_NEW(DRAWITEMSTRUCT16);
            if (!dis) return -1;
            dis->CtlType    = (UINT16)dis32->CtlType;
            dis->CtlID      = (UINT16)dis32->CtlID;
            dis->itemID     = (UINT16)dis32->itemID;
            dis->itemAction = (UINT16)dis32->itemAction;
            dis->itemState  = (UINT16)dis32->itemState;
            dis->hwndItem   = (HWND16)dis32->hwndItem;
            dis->hDC        = (HDC16)dis32->hDC;
            dis->itemData   = dis32->itemData;
            CONV_RECT32TO16( &dis32->rcItem, &dis->rcItem );
            *plparam = (LPARAM)SEGPTR_GET(dis);
        }
        return 1;
    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT32 *mis32 = (MEASUREITEMSTRUCT32 *)*plparam;
            MEASUREITEMSTRUCT16 *mis = (MEASUREITEMSTRUCT16 *)
                                     SEGPTR_ALLOC(sizeof(*mis)+sizeof(LPARAM));
            if (!mis) return -1;
            mis->CtlType    = (UINT16)mis32->CtlType;
            mis->CtlID      = (UINT16)mis32->CtlID;
            mis->itemID     = (UINT16)mis32->itemID;
            mis->itemWidth  = (UINT16)mis32->itemWidth;
            mis->itemHeight = (UINT16)mis32->itemHeight;
            mis->itemData   = mis32->itemData;
            *(LPARAM *)(mis + 1) = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)SEGPTR_GET(mis);
        }
        return 1;
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO16 *mmi = (MINMAXINFO16 *)SEGPTR_ALLOC( sizeof(*mmi) +
                                                              sizeof(LPARAM) );
            if (!mmi) return -1;
            STRUCT32_MINMAXINFO32to16( (MINMAXINFO32 *)*plparam, mmi );
            *(LPARAM *)(mmi + 1) = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)SEGPTR_GET(mmi);
        }
        return 1;
    case WM_GETTEXT:
        {
            LPSTR str;
            *pwparam16 = (WPARAM16)MIN( wParam32, 0xff80 ); /* Must be < 64K */
            if (!(str = SEGPTR_ALLOC(*pwparam16 + sizeof(LPARAM)))) return -1;
            *((LPARAM *)str)++ = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)SEGPTR_GET(str);
        }
        return 1;
    case WM_MDICREATE:
        {
            MDICREATESTRUCT16 *cs;
            MDICREATESTRUCT32A *cs32 = (MDICREATESTRUCT32A *)*plparam;
            LPSTR name, cls;

            if (!(cs = SEGPTR_NEW(MDICREATESTRUCT16))) return -1;
            STRUCT32_MDICREATESTRUCT32Ato16( cs32, cs );
            name = SEGPTR_STRDUP( cs32->szTitle );
            cls  = SEGPTR_STRDUP( cs32->szClass );
            cs->szTitle = SEGPTR_GET(name);
            cs->szClass = SEGPTR_GET(cls);
            *plparam = (LPARAM)SEGPTR_GET(cs);
        }
        return 1;
    case WM_MDISETMENU:
        *pwparam16 = TRUE;  /* FIXME? */
        *plparam   = MAKELPARAM( (HMENU16)LOWORD(wParam32),
                                 (HMENU16)LOWORD(*plparam) );
        return 0;
    case WM_MENUCHAR:
    case WM_MENUSELECT:
        *plparam = MAKELPARAM( HIWORD(wParam32), (HMENU16)*plparam );
        return 0;
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS32 *nc32 = (NCCALCSIZE_PARAMS32 *)*plparam;
            NCCALCSIZE_PARAMS16 *nc = (NCCALCSIZE_PARAMS16 *)SEGPTR_ALLOC( sizeof(*nc) + sizeof(LPARAM) );
            if (!nc) return -1;

            CONV_RECT32TO16( &nc32->rgrc[0], &nc->rgrc[0] );
            if (wParam32)
            {
                WINDOWPOS16 *wp;
                CONV_RECT32TO16( &nc32->rgrc[1], &nc->rgrc[1] );
                CONV_RECT32TO16( &nc32->rgrc[2], &nc->rgrc[2] );
                if (!(wp = SEGPTR_NEW(WINDOWPOS16)))
                {
                    SEGPTR_FREE(nc);
                    return -1;
                }
                STRUCT32_WINDOWPOS32to16( nc32->lppos, wp );
                nc->lppos = SEGPTR_GET(wp);
            }
            *(LPARAM *)(nc + 1) = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)SEGPTR_GET(nc);
        }
        return 1;
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT16 *cs;
            CREATESTRUCT32A *cs32 = (CREATESTRUCT32A *)*plparam;
            LPSTR name, cls;

            if (!(cs = SEGPTR_NEW(CREATESTRUCT16))) return -1;
            STRUCT32_CREATESTRUCT32Ato16( cs32, cs );
            name = SEGPTR_STRDUP( cs32->lpszName );
            cls  = SEGPTR_STRDUP( cs32->lpszClass );
            cs->lpszName  = SEGPTR_GET(name);
            cs->lpszClass = SEGPTR_GET(cls);
            *plparam = (LPARAM)SEGPTR_GET(cs);
        }
        return 1;
    case WM_PARENTNOTIFY:
        if ((LOWORD(wParam32)==WM_CREATE) || (LOWORD(wParam32)==WM_DESTROY))
            *plparam = MAKELPARAM( (HWND16)*plparam, HIWORD(wParam32));
        /* else nothing to do */
        return 0;
    case WM_SETTEXT:
        {
            LPSTR str = SEGPTR_STRDUP( (LPSTR)*plparam );
            if (!str) return -1;
            *plparam = (LPARAM)SEGPTR_GET(str);
        }
        return 1;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS16 *wp = (WINDOWPOS16 *)SEGPTR_ALLOC( sizeof(*wp) +
                                                           sizeof(LPARAM) );
            if (!wp) return -1;
            STRUCT32_WINDOWPOS32to16( (WINDOWPOS32 *)*plparam, wp );
            *(LPARAM *)(wp + 1) = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)SEGPTR_GET(wp);
        }
        return 1;
    case WM_ASKCBFORMATNAME:
    case WM_DEVMODECHANGE:
    case WM_MDIACTIVATE:
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    case WM_WININICHANGE:
        fprintf( stderr, "MapMsg32ATo16: message %04x needs translation\n",
                 msg32 );
        return -1;

    default:  /* No translation needed */
        return 0;
    }
}


/**********************************************************************
 *	     WINPROC_UnmapMsg32ATo16
 *
 * Unmap a message that was mapped from 32-bit Ansi to 16-bit.
 */
void WINPROC_UnmapMsg32ATo16( UINT32 msg, WPARAM16 wParam, LPARAM lParam )
{
    switch(msg)
    {
    case LB_ADDFILE32:
    case LB_ADDSTRING32:
    case LB_DIR32:
    case LB_FINDSTRING32:
    case LB_FINDSTRINGEXACT32:
    case LB_INSERTSTRING32:
    case LB_SELECTSTRING32:
    case LB_SETTABSTOPS32:
    case WM_COMPAREITEM:
    case WM_DELETEITEM:
    case WM_DRAWITEM:
    case WM_SETTEXT:
        SEGPTR_FREE( PTR_SEG_TO_LIN(lParam) );
        break;
    case LB_GETITEMRECT32:
        {
            RECT16 *rect = (RECT16 *)PTR_SEG_TO_LIN(lParam);
            lParam = *(LPARAM *)(rect + 1);
            CONV_RECT16TO32( rect, (RECT32 *)lParam );
            SEGPTR_FREE( rect );
        }
        break;
    case LB_GETSELITEMS32:
        {
            INT32 i;
            LPINT16 items = (LPINT16)PTR_SEG_TO_LIN(lParam);
            lParam = *((LPARAM *)items - 1);
            for (i = 0; i < wParam; i++) *((LPINT32)lParam + i) = items[i];
            SEGPTR_FREE( (LPARAM *)items - 1 );
        }
        break;
    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT16 *mis = (MEASUREITEMSTRUCT16 *)PTR_SEG_TO_LIN(lParam);
            MEASUREITEMSTRUCT32 *mis32 = *(MEASUREITEMSTRUCT32 **)(mis + 1);
            mis32->itemWidth  = mis->itemWidth;
            mis32->itemHeight = mis->itemHeight;
            SEGPTR_FREE(mis);
        }
        break;
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO16 *mmi = (MINMAXINFO16 *)PTR_SEG_TO_LIN(lParam);
            lParam = *(LPARAM *)(mmi + 1);
            STRUCT32_MINMAXINFO16to32( mmi, (MINMAXINFO32 *)lParam );
            SEGPTR_FREE(mmi);
        }
        break;
    case WM_GETTEXT:
        {
            LPSTR str = (LPSTR)PTR_SEG_TO_LIN(lParam);
            lParam = *((LPARAM *)str - 1);
            lstrcpyn32A( (LPSTR)lParam, str, wParam );
            SEGPTR_FREE( (LPARAM *)str - 1 );
        }
        break;
    case WM_MDICREATE:
        {
            MDICREATESTRUCT16 *cs = (MDICREATESTRUCT16*)PTR_SEG_TO_LIN(lParam);
            SEGPTR_FREE( PTR_SEG_TO_LIN(cs->szTitle) );
            SEGPTR_FREE( PTR_SEG_TO_LIN(cs->szClass) );
            SEGPTR_FREE( cs );
        }
        break;
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS32 *nc32;
            NCCALCSIZE_PARAMS16 *nc = (NCCALCSIZE_PARAMS16 *)PTR_SEG_TO_LIN(lParam);
            lParam = *(LPARAM *)(nc + 1);
            nc32 = (NCCALCSIZE_PARAMS32 *)lParam;
            CONV_RECT16TO32( &nc->rgrc[0], &nc32->rgrc[0] );
            if (wParam)
            {
                CONV_RECT16TO32( &nc->rgrc[1], &nc32->rgrc[1] );
                CONV_RECT16TO32( &nc->rgrc[2], &nc32->rgrc[2] );
                STRUCT32_WINDOWPOS16to32( (WINDOWPOS16 *)PTR_SEG_TO_LIN(nc->lppos),
                                          nc32->lppos );
                SEGPTR_FREE( PTR_SEG_TO_LIN(nc->lppos) );
            }
            SEGPTR_FREE(nc);
        }
        break;
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT16 *cs = (CREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam);
            SEGPTR_FREE( PTR_SEG_TO_LIN(cs->lpszName) );
            SEGPTR_FREE( PTR_SEG_TO_LIN(cs->lpszClass) );
            SEGPTR_FREE( cs );
        }
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS16 *wp = (WINDOWPOS16 *)PTR_SEG_TO_LIN(lParam);
            lParam = *(LPARAM *)(wp + 1);
            STRUCT32_WINDOWPOS16to32( wp, (WINDOWPOS32 *)lParam );
            SEGPTR_FREE(wp);
        }
        break;
    }
}


/**********************************************************************
 *	     WINPROC_MapMsg32WTo16
 *
 * Map a message from 32-bit Unicode to 16-bit.
 * Return value is -1 on error, 0 if OK, 1 if an UnmapMsg call is needed.
 */
INT32 WINPROC_MapMsg32WTo16( UINT32 msg32, WPARAM32 wParam32, UINT16 *pmsg16,
                             WPARAM16 *pwparam16, LPARAM *plparam )
{
    switch(msg32)
    {
    case LB_ADDSTRING32:
    case LB_FINDSTRING32:
    case LB_FINDSTRINGEXACT32:
    case LB_INSERTSTRING32:
    case LB_SELECTSTRING32:
    case LB_DIR32:
    case LB_ADDFILE32:
        {
            LPSTR str = SEGPTR_STRDUP_WtoA( (LPWSTR)*plparam );
            if (!str) return -1;
            *pmsg16 = (UINT16)msg32 + (LB_ADDSTRING16 - LB_ADDSTRING32);
            *pwparam16 = (WPARAM16)LOWORD(wParam32);
            *plparam   = (LPARAM)SEGPTR_GET(str);
        }
        return 1;
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT16 *cs;
            CREATESTRUCT32W *cs32 = (CREATESTRUCT32W *)*plparam;
            LPSTR name, cls;

            if (!(cs = SEGPTR_NEW(CREATESTRUCT16))) return -1;
            STRUCT32_CREATESTRUCT32Ato16( (CREATESTRUCT32A *)cs32, cs );
            name = SEGPTR_STRDUP_WtoA( cs32->lpszName );
            cls  = SEGPTR_STRDUP_WtoA( cs32->lpszClass );
            cs->lpszName  = SEGPTR_GET(name);
            cs->lpszClass = SEGPTR_GET(cls);
            *pmsg16    = (UINT16)msg32;
            *pwparam16 = (WPARAM16)LOWORD(wParam32);
            *plparam   = (LPARAM)SEGPTR_GET(cs);
        }
        return 1;
    case WM_MDICREATE:
        {
            MDICREATESTRUCT16 *cs;
            MDICREATESTRUCT32W *cs32 = (MDICREATESTRUCT32W *)*plparam;
            LPSTR name, cls;

            if (!(cs = SEGPTR_NEW(MDICREATESTRUCT16))) return -1;
            STRUCT32_MDICREATESTRUCT32Ato16( (MDICREATESTRUCT32A *)cs32, cs );
            name = SEGPTR_STRDUP_WtoA( cs32->szTitle );
            cls  = SEGPTR_STRDUP_WtoA( cs32->szClass );
            cs->szTitle = SEGPTR_GET(name);
            cs->szClass = SEGPTR_GET(cls);
            *pmsg16    = (UINT16)msg32;
            *pwparam16 = (WPARAM16)LOWORD(wParam32);
            *plparam   = (LPARAM)SEGPTR_GET(cs);
        }
        return 1;
    case WM_SETTEXT:
        {
            LPSTR str = SEGPTR_STRDUP_WtoA( (LPWSTR)*plparam );
            if (!str) return -1;
            *pmsg16    = (UINT16)msg32;
            *pwparam16 = (WPARAM16)LOWORD(wParam32);
            *plparam   = (LPARAM)SEGPTR_GET(str);
        }
        return 1;
    default:  /* No Unicode translation needed */
        return WINPROC_MapMsg32ATo16( msg32, wParam32, pmsg16,
                                      pwparam16, plparam );
    }
}


/**********************************************************************
 *	     WINPROC_UnmapMsg32WTo16
 *
 * Unmap a message that was mapped from 32-bit Unicode to 16-bit.
 */
void WINPROC_UnmapMsg32WTo16( UINT32 msg, WPARAM16 wParam, LPARAM lParam )
{
    switch(msg)
    {
    case WM_GETTEXT:
        {
            LPSTR str = (LPSTR)PTR_SEG_TO_LIN(lParam);
            lParam = *((LPARAM *)str - 1);
            lstrcpyAtoW( (LPWSTR)lParam, str );
            SEGPTR_FREE( (LPARAM *)str - 1 );
        }
        break;
    default:
        WINPROC_UnmapMsg32ATo16( msg, wParam, lParam );
        break;
    }
}


/**********************************************************************
 *	     WINPROC_CallProc32ATo32W
 *
 * Call a window procedure, translating args from Ansi to Unicode.
 */
static LRESULT WINPROC_CallProc32ATo32W( WNDPROC32 func, HWND32 hwnd,
                                         UINT32 msg, WPARAM32 wParam,
                                         LPARAM lParam )
{
    LRESULT result;

    if (WINPROC_MapMsg32ATo32W( msg, wParam, &lParam ) == -1) return 0;
    result = CallWndProc32( func, hwnd, msg, wParam, lParam );
    WINPROC_UnmapMsg32ATo32W( msg, wParam, lParam );
    return result;
}


/**********************************************************************
 *	     WINPROC_CallProc32WTo32A
 *
 * Call a window procedure, translating args from Unicode to Ansi.
 */
static LRESULT WINPROC_CallProc32WTo32A( WNDPROC32 func, HWND32 hwnd,
                                         UINT32 msg, WPARAM32 wParam,
                                         LPARAM lParam )
{
    LRESULT result;

    if (WINPROC_MapMsg32WTo32A( msg, wParam, &lParam ) == -1) return 0;
    result = CallWndProc32( func, hwnd, msg, wParam, lParam );
    WINPROC_UnmapMsg32WTo32A( msg, wParam, lParam );
    return result;
}


/**********************************************************************
 *	     WINPROC_CallProc16To32A
 *
 * Call a 32-bit window procedure, translating the 16-bit args.
 */
LRESULT WINPROC_CallProc16To32A( HWND16 hwnd, UINT16 msg,
                                 WPARAM16 wParam, LPARAM lParam,
                                 WNDPROC32 func )
{
    LRESULT result;
    UINT32 msg32;
    WPARAM32 wParam32;

    if (WINPROC_MapMsg16To32A( msg, wParam, &msg32, &wParam32, &lParam ) == -1)
        return 0;
    result = CallWndProc32( func, hwnd, msg32, wParam32, lParam );
    WINPROC_UnmapMsg16To32A( msg32, wParam32, lParam );
    return result;
}


/**********************************************************************
 *	     WINPROC_CallProc16To32W
 *
 * Call a 32-bit window procedure, translating the 16-bit args.
 */
LRESULT WINPROC_CallProc16To32W( HWND16 hwnd, UINT16 msg,
                                 WPARAM16 wParam, LPARAM lParam,
                                 WNDPROC32 func )
{
    LRESULT result;
    UINT32 msg32;
    WPARAM32 wParam32;

    if (WINPROC_MapMsg16To32W( msg, wParam, &msg32, &wParam32, &lParam ) == -1)
        return 0;
    result = CallWndProc32( func, hwnd, msg32, wParam32, lParam );
    WINPROC_UnmapMsg16To32W( msg32, wParam32, lParam );
    return result;
}


/**********************************************************************
 *	     WINPROC_CallProc32ATo16
 *
 * Call a 16-bit window procedure, translating the 32-bit args.
 */
static LRESULT WINPROC_CallProc32ATo16( WNDPROC16 func, HWND32 hwnd,
                                        UINT32 msg, WPARAM32 wParam,
                                        LPARAM lParam )
{
    LRESULT result;
    UINT16 msg16;
    WPARAM16 wParam16;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    WORD ds = CURRENT_DS;

    if (WINPROC_MapMsg32ATo16( msg, wParam, &msg16, &wParam16, &lParam ) == -1)
        return 0;
    if (wndPtr) CURRENT_DS = wndPtr->hInstance;
    result = CallWndProc16( func, hwnd, msg16, wParam16, lParam );
    CURRENT_DS = ds;
    WINPROC_UnmapMsg32ATo16( msg, wParam16, lParam );
    return result;
}


/**********************************************************************
 *	     WINPROC_CallProc32WTo16
 *
 * Call a 16-bit window procedure, translating the 32-bit args.
 */
static LRESULT WINPROC_CallProc32WTo16( WNDPROC16 func, HWND32 hwnd,
                                        UINT32 msg, WPARAM32 wParam,
                                        LPARAM lParam )
{
    LRESULT result;
    UINT16 msg16;
    WPARAM16 wParam16;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    WORD ds = CURRENT_DS;

    if (WINPROC_MapMsg32WTo16( msg, wParam, &msg16, &wParam16, &lParam ) == -1)
        return 0;
    if (wndPtr) CURRENT_DS = wndPtr->hInstance;
    result = CallWndProc16( func, hwnd, msg16, wParam16, lParam );
    CURRENT_DS = ds;
    WINPROC_UnmapMsg32WTo16( msg, wParam16, lParam );
    return result;
}


/**********************************************************************
 *	     CallWindowProc16    (USER.122)
 */
LRESULT CallWindowProc16( WNDPROC16 func, HWND16 hwnd, UINT16 msg,
                          WPARAM16 wParam, LPARAM lParam )
{
    LRESULT result;
    WND *wndPtr;
    WINDOWPROC *proc = WINPROC_GetPtr( func );
    WORD ds = CURRENT_DS;

    if (!proc)
    {
        wndPtr = WIN_FindWndPtr( hwnd );
        if (wndPtr) CURRENT_DS = wndPtr->hInstance;
        result = CallWndProc16( (FARPROC16)func, hwnd, msg, wParam, lParam );
        CURRENT_DS = ds;
        return result;
    }
#if testing
    wndPtr = WIN_FindWndPtr( hwnd );
    if (wndPtr) CURRENT_DS = wndPtr->hInstance;
    result = CallWndProc16( WINPROC_GetProc( (HWINDOWPROC)proc, WIN_PROC_16),
                            hwnd, msg, wParam, lParam );
    CURRENT_DS = ds;
    return result;
#endif
    
    switch(proc->type)
    {
    case WIN_PROC_16:
        if (!proc->thunk.t_from32.proc) return 0;
        wndPtr = WIN_FindWndPtr( hwnd );
        if (wndPtr) CURRENT_DS = wndPtr->hInstance;
#ifndef WINELIB
        if ((msg == WM_CREATE) || (msg == WM_NCCREATE))
        {
            CREATESTRUCT16 *cs = (CREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam);
            /* Build the CREATESTRUCT on the 16-bit stack. */
            /* This is really ugly, but some programs (notably the */
            /* "Undocumented Windows" examples) want it that way.  */
            result = CallWndProcNCCREATE16( proc->thunk.t_from32.proc,
                    cs->dwExStyle, cs->lpszClass, cs->lpszName, cs->style,
                    cs->x, cs->y, cs->cx, cs->cy, cs->hwndParent, cs->hMenu,
                    cs->hInstance, (LONG)cs->lpCreateParams, hwnd, msg, wParam,
                    MAKELONG( IF1632_Saved16_sp-sizeof(CREATESTRUCT16),
                              IF1632_Saved16_ss ) );
            CURRENT_DS = ds;
            return result;
        }
#endif
        result = CallWndProc16( proc->thunk.t_from32.proc,
                                hwnd, msg, wParam, lParam );
        CURRENT_DS = ds;
        return result;

    case WIN_PROC_32A:
        if (!proc->thunk.t_from16.proc) return 0;
        return WINPROC_CallProc16To32A( hwnd, msg, wParam, lParam,
                                        proc->thunk.t_from16.proc );
    case WIN_PROC_32W:
        if (!proc->thunk.t_from16.proc) return 0;
        return WINPROC_CallProc16To32W( hwnd, msg, wParam, lParam,
                                        proc->thunk.t_from16.proc );
    default:
        fprintf( stderr, "CallWindowProc16: invalid proc %p\n", proc );
        return 0;
    }
}


/**********************************************************************
 *	     CallWindowProc32A    (USER32.17)
 */
LRESULT CallWindowProc32A( WNDPROC32 func, HWND32 hwnd, UINT32 msg,
                           WPARAM32 wParam, LPARAM lParam )
{
    WINDOWPROC *proc = WINPROC_GetPtr( (WNDPROC16)func );

    if (!proc) return CallWndProc32( func, hwnd, msg, wParam, lParam );

#if testing
    func = WINPROC_GetProc( (HWINDOWPROC)proc, WIN_PROC_32A );
    return CallWndProc32( func, hwnd, msg, wParam, lParam );
#endif

    switch(proc->type)
    {
    case WIN_PROC_16:
        if (!proc->thunk.t_from32.proc) return 0;
        return WINPROC_CallProc32ATo16( proc->thunk.t_from32.proc,
                                        hwnd, msg, wParam, lParam );
    case WIN_PROC_32A:
        if (!proc->thunk.t_from16.proc) return 0;
        return CallWndProc32( proc->thunk.t_from16.proc,
                              hwnd, msg, wParam, lParam );
    case WIN_PROC_32W:
        if (!proc->thunk.t_from16.proc) return 0;
        return WINPROC_CallProc32ATo32W( proc->thunk.t_from16.proc,
                                         hwnd, msg, wParam, lParam );
    default:
        fprintf( stderr, "CallWindowProc32A: invalid proc %p\n", proc );
        return 0;
    }
}


/**********************************************************************
 *	     CallWindowProc32W    (USER32.18)
 */
LRESULT CallWindowProc32W( WNDPROC32 func, HWND32 hwnd, UINT32 msg,
                           WPARAM32 wParam, LPARAM lParam )
{
    WINDOWPROC *proc = WINPROC_GetPtr( (WNDPROC16)func );

    if (!proc) return CallWndProc32( func, hwnd, msg, wParam, lParam );

#if testing
    func = WINPROC_GetProc( (HWINDOWPROC)proc, WIN_PROC_32W );
    return CallWndProc32( func, hwnd, msg, wParam, lParam );
#endif

    switch(proc->type)
    {
    case WIN_PROC_16:
        if (!proc->thunk.t_from32.proc) return 0;
        return WINPROC_CallProc32WTo16( proc->thunk.t_from32.proc,
                                        hwnd, msg, wParam, lParam );
    case WIN_PROC_32A:
        if (!proc->thunk.t_from16.proc) return 0;
        return WINPROC_CallProc32WTo32A( proc->thunk.t_from16.proc,
                                         hwnd, msg, wParam, lParam );
    case WIN_PROC_32W:
        if (!proc->thunk.t_from16.proc) return 0;
        return CallWndProc32( proc->thunk.t_from16.proc,
                              hwnd, msg, wParam, lParam );
    default:
        fprintf( stderr, "CallWindowProc32W: invalid proc %p\n", proc );
        return 0;
    }
}
