/*
 * Window procedure callbacks
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1996 Alexandre Julliard
 */

#include "wine/winbase16.h"
#include "winuser.h"
#include "callback.h"
#include "heap.h"
#include "selectors.h"
#include "struct32.h"
#include "win.h"
#include "winproc.h"
#include "debugtools.h"
#include "spy.h"
#include "commctrl.h"
#include "task.h"
#include "thread.h"

DECLARE_DEBUG_CHANNEL(msg)
DECLARE_DEBUG_CHANNEL(relay)
DECLARE_DEBUG_CHANNEL(win)

/* Window procedure 16-to-32-bit thunk,
 * see BuildSpec16Files() in tools/build.c */

typedef struct
{
    BYTE       popl_eax;             /* popl  %eax (return address) */
    BYTE       pushl_func;           /* pushl $proc */
    WNDPROC  proc WINE_PACKED;
    BYTE       pushl_eax;            /* pushl %eax */
    WORD       pushw_bp WINE_PACKED; /* pushw %bp */
    BYTE       pushl_thunk;          /* pushl $thunkfrom16 */
    void     (*thunk32)() WINE_PACKED;
    BYTE       lcall;                /* lcall cs:relay */
    void     (*relay)() WINE_PACKED; /* WINPROC_CallProc16To32A/W() */
    WORD       cs WINE_PACKED;
} WINPROC_THUNK_FROM16;

/* Window procedure 32-to-16-bit thunk,
 * see BuildSpec32Files() in tools/build.c */

typedef struct
{
    BYTE       popl_eax;             /* popl  %eax (return address) */
    BYTE       pushl_func;           /* pushl $proc */
    WNDPROC16  proc WINE_PACKED;
    BYTE       pushl_eax;            /* pushl %eax */
    BYTE       jmp;                  /* jmp   relay (relative jump)*/
    void     (*relay)() WINE_PACKED; /* WINPROC_CallProc32ATo16() */
} WINPROC_THUNK_FROM32;

/* Simple jmp to call 32-bit procedure directly */
typedef struct
{
    BYTE       jmp;                  /* jmp  proc (relative jump) */
    WNDPROC  proc WINE_PACKED;
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
    UINT                magic;    /* Magic number */
    WINDOWPROCTYPE        type;     /* Function type */
    WINDOWPROCUSER        user;     /* Function user */
} WINDOWPROC;

#define WINPROC_MAGIC  ('W' | ('P' << 8) | ('R' << 16) | ('C' << 24))

#define WINPROC_THUNKPROC(pproc) \
    (((pproc)->type == WIN_PROC_16) ? \
          (WNDPROC16)((pproc)->thunk.t_from32.proc) : \
          (WNDPROC16)((pproc)->thunk.t_from16.proc))

static LRESULT WINAPI WINPROC_CallProc32ATo16( WNDPROC16 func, HWND hwnd,
                                               UINT msg, WPARAM wParam,
                                               LPARAM lParam );
static LRESULT WINAPI WINPROC_CallProc32WTo16( WNDPROC16 func, HWND hwnd,
                                               UINT msg, WPARAM wParam,
                                               LPARAM lParam );
static LRESULT WINPROC_CallProc16To32A( HWND16 hwnd, UINT16 msg,
					WPARAM16 wParam, LPARAM lParam,
					WNDPROC func );
static LRESULT WINPROC_CallProc16To32W( HWND16 hwnd, UINT16 msg,
					WPARAM16 wParam, LPARAM lParam,
					WNDPROC func );

static HANDLE WinProcHeap;


/**********************************************************************
 *	     WINPROC_Init
 */
BOOL WINPROC_Init(void)
{
    WinProcHeap = HeapCreate( HEAP_WINE_SEGPTR | HEAP_WINE_CODESEG, 0, 0 );
    if (!WinProcHeap)
    {
        WARN_(relay)("Unable to create winproc heap\n" );
        return FALSE;
    }
    return TRUE;
}


/**********************************************************************
 *	     WINPROC_CallWndProc32
 *
 * Call a 32-bit WndProc.
 */
static LRESULT WINPROC_CallWndProc( WNDPROC proc, HWND hwnd, UINT msg,
                                      WPARAM wParam, LPARAM lParam )
{
    LRESULT retvalue;
    int iWndsLocks;
    
    TRACE_(relay)("(wndproc=%p,hwnd=%08x,msg=%s,wp=%08x,lp=%08lx)\n",
                   proc, hwnd, SPY_GetMsgName(msg), wParam, lParam );
    /* To avoid any deadlocks, all the locks on the windows structures
       must be suspended before the control is passed to the application */
    iWndsLocks = WIN_SuspendWndsLock();
    retvalue = proc( hwnd, msg, wParam, lParam );
    WIN_RestoreWndsLock(iWndsLocks);
    return retvalue;
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
static WINDOWPROC *WINPROC_AllocWinProc( WNDPROC16 func, WINDOWPROCTYPE type,
                                         WINDOWPROCUSER user )
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
            proc->thunk.t_from32.jmp         = 0xe9;   /* jmp   relay*/
            proc->thunk.t_from32.relay =  /* relative jump */
                (void(*)())((DWORD)WINPROC_CallProc32ATo16 -
                                     (DWORD)(&proc->thunk.t_from32.relay + 1));
            break;
        case WIN_PROC_32A:
        case WIN_PROC_32W:
            proc->thunk.t_from16.popl_eax    = 0x58;   /* popl  %eax */
            proc->thunk.t_from16.pushl_func  = 0x68;   /* pushl $proc */
            proc->thunk.t_from16.proc        = (FARPROC)func;
            proc->thunk.t_from16.pushl_eax   = 0x50;   /* pushl %eax */
            proc->thunk.t_from16.pushw_bp    = 0x5566; /* pushw %bp */
            proc->thunk.t_from16.pushl_thunk = 0x68;   /* pushl $thunkfrom16 */
            proc->thunk.t_from16.thunk32     = (type == WIN_PROC_32A) ?
                                           (void(*)())WINPROC_CallProc16To32A :
                                           (void(*)())WINPROC_CallProc16To32W;
            proc->thunk.t_from16.lcall       = 0x9a;   /* lcall cs:relay */
            proc->thunk.t_from16.relay       = (void*)Callbacks->CallFrom16WndProc;
            GET_CS(proc->thunk.t_from16.cs);
            proc->jmp.jmp  = 0xe9;
            /* Fixup relative jump */
            proc->jmp.proc = (WNDPROC)((DWORD)func -
                                                 (DWORD)(&proc->jmp.proc + 1));
            break;
        default:
            /* Should not happen */
            break;
        }
        proc->magic = WINPROC_MAGIC;
        proc->type  = type;
        proc->user  = user;
    }
    proc->next  = NULL;
    TRACE_(win)("(%08x,%d): returning %08x\n",
                 (UINT)func, type, (UINT)proc );
    return proc;
}


/**********************************************************************
 *	     WINPROC_GetProc
 *
 * Get a window procedure pointer that can be passed to the Windows program.
 */
WNDPROC16 WINPROC_GetProc( HWINDOWPROC proc, WINDOWPROCTYPE type )
{
    if (!proc) return NULL;
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
        else if (type != ((WINDOWPROC *)proc)->type)
            /* Have to return the jmp address if types don't match */
            return (WNDPROC16)&((WINDOWPROC *)proc)->jmp;
        else
            /* Some Win16 programs want to get back the proc they set */
            return (WNDPROC16)((WINDOWPROC *)proc)->thunk.t_from16.proc;
    }
}


/**********************************************************************
 *	     WINPROC_SetProc
 *
 * Set the window procedure for a window or class. There are
 * three tree classes of winproc callbacks:
 *
 * 1) class  -> wp                 	-	not subclassed
 *    class  -> wp -> wp -> wp -> wp 	-	SetClassLong()
 *             /           /
 * 2) window -'           /		-	not subclassed
 *    window -> wp -> wp '           	-	SetWindowLong()
 *
 * 3) timer  -> wp                   	-	SetTimer()
 *
 * Initially, winproc of the window points to the current winproc 
 * thunk of its class. Subclassing prepends a new thunk to the 
 * window winproc chain at the head of the list. Thus, window thunk 
 * list includes class thunks and the latter are preserved when the 
 * window is destroyed.
 *
 */
BOOL WINPROC_SetProc( HWINDOWPROC *pFirst, WNDPROC16 func,
                        WINDOWPROCTYPE type, WINDOWPROCUSER user )
{
    BOOL bRecycle = FALSE;
    WINDOWPROC *proc, **ppPrev;

    /* Check if function is already in the list */

    ppPrev = (WINDOWPROC **)pFirst;
    proc = WINPROC_GetPtr( func );
    while (*ppPrev)
    {
        if (proc)
        {
            if (*ppPrev == proc)
            {
                if ((*ppPrev)->user != user)
		{
		    /* terminal thunk is being restored */

		    WINPROC_FreeProc( *pFirst, (*ppPrev)->user );
		    *(WINDOWPROC **)pFirst = *ppPrev;
		    return TRUE;
		}
		bRecycle = TRUE;
		break;
	    }
        }
        else
        {
            if (((*ppPrev)->type == type) &&
                (func == WINPROC_THUNKPROC(*ppPrev)))
            {
                bRecycle = TRUE;
                break;
            }
        }
            
        /* WPF_CLASS thunk terminates window thunk list */
        if ((*ppPrev)->user != user) break;
        ppPrev = &(*ppPrev)->next;
    }

    if (bRecycle)
    {
        /* Extract this thunk from the list */
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
        proc = WINPROC_AllocWinProc( func, type, user );
        if (!proc) return FALSE;
    }

    /* Add the win proc at the head of the list */

    TRACE_(win)("(%08x,%08x,%d): res=%08x\n",
                 (UINT)*pFirst, (UINT)func, type, (UINT)proc );
    proc->next  = *(WINDOWPROC **)pFirst;
    *(WINDOWPROC **)pFirst = proc;
    return TRUE;
}


/**********************************************************************
 *	     WINPROC_FreeProc
 *
 * Free a list of win procs.
 */
void WINPROC_FreeProc( HWINDOWPROC proc, WINDOWPROCUSER user )
{
    while (proc)
    {
        WINDOWPROC *next = ((WINDOWPROC *)proc)->next;
        if (((WINDOWPROC *)proc)->user != user) break;
        TRACE_(win)("freeing %08x\n", (UINT)proc);
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
 *	     WINPROC_TestCBForStr
 *
 * Return TRUE if the lparam is a string
 */
BOOL WINPROC_TestCBForStr ( HWND hwnd )
{
    BOOL retvalue;
    WND * wnd = WIN_FindWndPtr(hwnd);
    retvalue = ( !(LOWORD(wnd->dwStyle) & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) ||
	      (LOWORD(wnd->dwStyle) & CBS_HASSTRINGS) );
    WIN_ReleaseWndPtr(wnd);
    return retvalue;
}
/**********************************************************************
 *	     WINPROC_TestLBForStr
 *
 * Return TRUE if the lparam is a string
 */
BOOL WINPROC_TestLBForStr ( HWND hwnd )
{
    BOOL retvalue;
    WND * wnd = WIN_FindWndPtr(hwnd); 
    retvalue = ( !(LOWORD(wnd->dwStyle) & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) ||
	    (LOWORD(wnd->dwStyle) & LBS_HASSTRINGS) );
    WIN_ReleaseWndPtr(wnd);
    return retvalue;

}
/**********************************************************************
 *	     WINPROC_MapMsg32ATo32W
 *
 * Map a message from Ansi to Unicode.
 * Return value is -1 on error, 0 if OK, 1 if an UnmapMsg call is needed.
 *
 * FIXME:
 *  WM_CHAR, WM_CHARTOITEM, WM_DEADCHAR, WM_MENUCHAR, WM_SYSCHAR, WM_SYSDEADCHAR
 *
 * FIXME:
 *  WM_GETTEXT/WM_SETTEXT and static control with SS_ICON style:
 *  the first four bytes are the handle of the icon 
 *  when the WM_SETTEXT message has been used to set the icon
 */
INT WINPROC_MapMsg32ATo32W( HWND hwnd, UINT msg, WPARAM wParam, LPARAM *plparam )
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
    /* lparam is string (0-terminated) */
    case WM_SETTEXT:
    case WM_WININICHANGE:
    case CB_DIR:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
    case LB_DIR:
    case LB_ADDFILE:
    case LB_FINDSTRING:
    case LB_SELECTSTRING:
    case EM_REPLACESEL:
        *plparam = (LPARAM)HEAP_strdupAtoW( SystemHeap, 0, (LPCSTR)*plparam );
        return (*plparam ? 1 : -1);

    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCTW *cs = (CREATESTRUCTW *)HeapAlloc( SystemHeap, 0,
                                                                sizeof(*cs) );
            if (!cs) return -1;
            *cs = *(CREATESTRUCTW *)*plparam;
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
            MDICREATESTRUCTW *cs =
                (MDICREATESTRUCTW *)HeapAlloc( SystemHeap, 0, sizeof(*cs) );
            if (!cs) return -1;
            *cs = *(MDICREATESTRUCTW *)*plparam;
            if (HIWORD(cs->szClass))
                cs->szClass = HEAP_strdupAtoW( SystemHeap, 0,
                                               (LPCSTR)cs->szClass );
            if (HIWORD(cs->szTitle))
                cs->szTitle = HEAP_strdupAtoW( SystemHeap, 0,
                                               (LPCSTR)cs->szTitle );
            *plparam = (LPARAM)cs;
        }
        return 1;

/* Listbox */
    case LB_ADDSTRING:
    case LB_INSERTSTRING:
	if ( WINPROC_TestLBForStr( hwnd ))
          *plparam = (LPARAM)HEAP_strdupAtoW( SystemHeap, 0, (LPCSTR)*plparam );
        return (*plparam ? 1 : -1);

    case LB_GETTEXT:		    /* fixme: fixed sized buffer */
        { if ( WINPROC_TestLBForStr( hwnd ))
	  { LPARAM *ptr = (LPARAM *)HeapAlloc( SystemHeap, 0, 256 * sizeof(WCHAR) + sizeof(LPARAM) );
            if (!ptr) return -1;
            *ptr++ = *plparam;  /* Store previous lParam */
            *plparam = (LPARAM)ptr;
	  }
        }
        return 1;

/* Combobox */
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
	if ( WINPROC_TestCBForStr( hwnd ))
          *plparam = (LPARAM)HEAP_strdupAtoW( SystemHeap, 0, (LPCSTR)*plparam );
        return (*plparam ? 1 : -1);

    case CB_GETLBTEXT:    /* fixme: fixed sized buffer */
        { if ( WINPROC_TestCBForStr( hwnd ))
          { LPARAM *ptr = (LPARAM *)HeapAlloc( SystemHeap, 0, 256 * sizeof(WCHAR) + sizeof(LPARAM) );
            if (!ptr) return -1;
            *ptr++ = *plparam;  /* Store previous lParam */
            *plparam = (LPARAM)ptr;
	  }
        }
        return 1;

/* Multiline edit */
    case EM_GETLINE:
        { WORD len = (WORD)*plparam;
	  LPARAM *ptr = (LPARAM *) HEAP_xalloc( SystemHeap, 0, sizeof(LPARAM) + sizeof (WORD) + len*sizeof(WCHAR) );
          if (!ptr) return -1;
          *ptr++ = *plparam;  /* Store previous lParam */
	  *((WORD *) ptr) = len;   /* Store the length */
          *plparam = (LPARAM)ptr;
	}
        return 1;

    case WM_ASKCBFORMATNAME:
    case WM_DEVMODECHANGE:
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    case EM_SETPASSWORDCHAR:
        FIXME_(msg)("message %s (0x%x) needs translation, please report\n", SPY_GetMsgName(msg), msg );
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
void WINPROC_UnmapMsg32ATo32W( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch(msg)
    {
    case WM_GETTEXT:
        {
            LPARAM *ptr = (LPARAM *)lParam - 1;
            lstrcpynWtoA( (LPSTR)*ptr, (LPWSTR)lParam, wParam );
            HeapFree( SystemHeap, 0, ptr );
        }
        break;

    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCTW *cs = (CREATESTRUCTW *)lParam;
            if (HIWORD(cs->lpszName))
                HeapFree( SystemHeap, 0, (LPVOID)cs->lpszName );
            if (HIWORD(cs->lpszClass))
                HeapFree( SystemHeap, 0, (LPVOID)cs->lpszClass );
            HeapFree( SystemHeap, 0, cs );
        }
        break;

    case WM_MDICREATE:
        {
            MDICREATESTRUCTW *cs = (MDICREATESTRUCTW *)lParam;
            if (HIWORD(cs->szTitle))
                HeapFree( SystemHeap, 0, (LPVOID)cs->szTitle );
            if (HIWORD(cs->szClass))
                HeapFree( SystemHeap, 0, (LPVOID)cs->szClass );
            HeapFree( SystemHeap, 0, cs );
        }
        break;

    case WM_SETTEXT:
    case WM_WININICHANGE:
    case CB_DIR:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
    case LB_DIR:
    case LB_ADDFILE:
    case LB_FINDSTRING:
    case LB_SELECTSTRING:
    case EM_REPLACESEL:
        HeapFree( SystemHeap, 0, (void *)lParam );
        break;

/* Listbox */
    case LB_ADDSTRING:
    case LB_INSERTSTRING:
	if ( WINPROC_TestLBForStr( hwnd ))
          HeapFree( SystemHeap, 0, (void *)lParam );
        break;

    case LB_GETTEXT:
        { if ( WINPROC_TestLBForStr( hwnd ))
          { LPARAM *ptr = (LPARAM *)lParam - 1;
	    lstrcpyWtoA( (LPSTR)*ptr, (LPWSTR)(lParam) );
            HeapFree( SystemHeap, 0, ptr );
	  }
        }
        break;

/* Combobox */
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
	if ( WINPROC_TestCBForStr( hwnd ))
          HeapFree( SystemHeap, 0, (void *)lParam );
        break;

    case CB_GETLBTEXT:
        { if ( WINPROC_TestCBForStr( hwnd ))
	  { LPARAM *ptr = (LPARAM *)lParam - 1;
            lstrcpyWtoA( (LPSTR)*ptr, (LPWSTR)(lParam) );
            HeapFree( SystemHeap, 0, ptr );
	  }
        }
        break;

/* Multiline edit */
    case EM_GETLINE:
        { LPARAM * ptr = (LPARAM *)lParam - 1;  /* get the old lParam */
	  WORD len = *(WORD *) lParam;
          lstrcpynWtoA( (LPSTR)*ptr , (LPWSTR)lParam, len );
          HeapFree( SystemHeap, 0, ptr );
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
INT WINPROC_MapMsg32WTo32A( HWND hwnd, UINT msg, WPARAM wParam, LPARAM *plparam )
{   switch(msg)
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
    case WM_WININICHANGE:
    case CB_DIR:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
    case LB_DIR:
    case LB_ADDFILE:
    case LB_FINDSTRING:
    case LB_SELECTSTRING:
    case EM_REPLACESEL:
        *plparam = (LPARAM)HEAP_strdupWtoA( SystemHeap, 0, (LPCWSTR)*plparam );
        return (*plparam ? 1 : -1);

    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCTA *cs = (CREATESTRUCTA *)HeapAlloc( SystemHeap, 0,
                                                                sizeof(*cs) );
            if (!cs) return -1;
            *cs = *(CREATESTRUCTA *)*plparam;
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
            MDICREATESTRUCTA *cs =
                (MDICREATESTRUCTA *)HeapAlloc( SystemHeap, 0, sizeof(*cs) );
            if (!cs) return -1;
            *cs = *(MDICREATESTRUCTA *)*plparam;
            if (HIWORD(cs->szTitle))
                cs->szTitle = HEAP_strdupWtoA( SystemHeap, 0,
                                               (LPCWSTR)cs->szTitle );
            if (HIWORD(cs->szClass))
                cs->szClass = HEAP_strdupWtoA( SystemHeap, 0,
                                               (LPCWSTR)cs->szClass );
            *plparam = (LPARAM)cs;
        }
        return 1;

/* Listbox */
    case LB_ADDSTRING:
    case LB_INSERTSTRING:
	if ( WINPROC_TestLBForStr( hwnd ))
          *plparam = (LPARAM)HEAP_strdupWtoA( SystemHeap, 0, (LPCWSTR)*plparam );
        return (*plparam ? 1 : -1);

    case LB_GETTEXT:			/* fixme: fixed sized buffer */
        { if ( WINPROC_TestLBForStr( hwnd ))
	  { LPARAM *ptr = (LPARAM *)HeapAlloc( SystemHeap, 0, 256 + sizeof(LPARAM) );
            if (!ptr) return -1;
            *ptr++ = *plparam;  /* Store previous lParam */
            *plparam = (LPARAM)ptr;
	  }
        }
        return 1;

/* Combobox */
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
	if ( WINPROC_TestCBForStr( hwnd ))
          *plparam = (LPARAM)HEAP_strdupWtoA( SystemHeap, 0, (LPCWSTR)*plparam );
        return (*plparam ? 1 : -1);

    case CB_GETLBTEXT:		/* fixme: fixed sized buffer */
        { if ( WINPROC_TestCBForStr( hwnd ))
	  { LPARAM *ptr = (LPARAM *)HeapAlloc( SystemHeap, 0, 256 + sizeof(LPARAM) );
            if (!ptr) return -1;
            *ptr++ = *plparam;  /* Store previous lParam */
            *plparam = (LPARAM)ptr;
	  }
        }
        return 1;

/* Multiline edit */
    case EM_GETLINE:
        { WORD len = (WORD)*plparam;
	  LPARAM *ptr = (LPARAM *) HEAP_xalloc( SystemHeap, 0, sizeof(LPARAM) + sizeof (WORD) + len*sizeof(CHAR) );
          if (!ptr) return -1;
          *ptr++ = *plparam;  /* Store previous lParam */
	  *((WORD *) ptr) = len;   /* Store the length */
          *plparam = (LPARAM)ptr;
	}
        return 1;

    case WM_ASKCBFORMATNAME:
    case WM_DEVMODECHANGE:
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    case EM_SETPASSWORDCHAR:
        FIXME_(msg)("message %s (%04x) needs translation, please report\n",SPY_GetMsgName(msg),msg );
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
void WINPROC_UnmapMsg32WTo32A( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch(msg)
    {
    case WM_GETTEXT:
        {
            LPARAM *ptr = (LPARAM *)lParam - 1;
            lstrcpynAtoW( (LPWSTR)*ptr, (LPSTR)lParam, wParam );
            HeapFree( SystemHeap, 0, ptr );
        }
        break;

    case WM_SETTEXT:
    case WM_WININICHANGE:
    case CB_DIR:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
    case LB_DIR:
    case LB_ADDFILE:
    case LB_FINDSTRING:
    case LB_SELECTSTRING:
    case EM_REPLACESEL:
        HeapFree( SystemHeap, 0, (void *)lParam );
        break;

    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCTA *cs = (CREATESTRUCTA *)lParam;
            if (HIWORD(cs->lpszName))
                HeapFree( SystemHeap, 0, (LPVOID)cs->lpszName );
            if (HIWORD(cs->lpszClass))
                HeapFree( SystemHeap, 0, (LPVOID)cs->lpszClass );
            HeapFree( SystemHeap, 0, cs );
        }
        break;

    case WM_MDICREATE:
        {
            MDICREATESTRUCTA *cs = (MDICREATESTRUCTA *)lParam;
            if (HIWORD(cs->szTitle))
                HeapFree( SystemHeap, 0, (LPVOID)cs->szTitle );
            if (HIWORD(cs->szClass))
                HeapFree( SystemHeap, 0, (LPVOID)cs->szClass );
            HeapFree( SystemHeap, 0, cs );
        }
        break;

/* Listbox */
    case LB_ADDSTRING:
    case LB_INSERTSTRING:
	if ( WINPROC_TestLBForStr( hwnd ))
          HeapFree( SystemHeap, 0, (void *)lParam );
        break;

    case LB_GETTEXT:
        { if ( WINPROC_TestLBForStr( hwnd ))
          { LPARAM *ptr = (LPARAM *)lParam - 1;
            lstrcpyAtoW( (LPWSTR)*ptr, (LPSTR)(lParam) );
            HeapFree( SystemHeap, 0, ptr );
	  }
        }
        break;

/* Combobox */
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
	if ( WINPROC_TestCBForStr( hwnd ))
          HeapFree( SystemHeap, 0, (void *)lParam );
        break;

    case CB_GETLBTEXT:
        { if ( WINPROC_TestCBForStr( hwnd ))
          { LPARAM *ptr = (LPARAM *)lParam - 1;
            lstrcpyAtoW( (LPWSTR)*ptr, (LPSTR)(lParam) );
            HeapFree( SystemHeap, 0, ptr );
	  }
        }
        break;

/* Multiline edit */
    case EM_GETLINE:
        { LPARAM * ptr = (LPARAM *)lParam - 1;  /* get the old lparam */
	  WORD len = *(WORD *)ptr;
          lstrcpynAtoW( (LPWSTR) *ptr, (LPSTR)lParam, len );
          HeapFree( SystemHeap, 0, ptr );
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
INT WINPROC_MapMsg16To32A( UINT16 msg16, WPARAM16 wParam16, UINT *pmsg32,
                             WPARAM *pwparam32, LPARAM *plparam )
{
    *pmsg32 = (UINT)msg16;
    *pwparam32 = (WPARAM)wParam16;
    switch(msg16)
    {
    case WM_ACTIVATE:
    case WM_CHARTOITEM:
    case WM_COMMAND:
    case WM_VKEYTOITEM:
        *pwparam32 = MAKEWPARAM( wParam16, HIWORD(*plparam) );
        *plparam   = (LPARAM)(HWND)LOWORD(*plparam);
        return 0;
    case WM_HSCROLL:
    case WM_VSCROLL:
        *pwparam32 = MAKEWPARAM( wParam16, LOWORD(*plparam) );
        *plparam   = (LPARAM)(HWND)HIWORD(*plparam);
        return 0;
    case WM_CTLCOLOR:
    	if ( HIWORD(*plparam) > CTLCOLOR_STATIC ) return -1;
        *pmsg32    = WM_CTLCOLORMSGBOX + HIWORD(*plparam);
        *pwparam32 = (WPARAM)(HDC)wParam16;
        *plparam   = (LPARAM)(HWND)LOWORD(*plparam);
        return 0;
    case WM_COMPAREITEM:
        {
            COMPAREITEMSTRUCT16* cis16 = (COMPAREITEMSTRUCT16 *)PTR_SEG_TO_LIN(*plparam);
            COMPAREITEMSTRUCT *cis = (COMPAREITEMSTRUCT *)
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
            DELETEITEMSTRUCT *dis = (DELETEITEMSTRUCT *)
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
            MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)
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
            DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT*)HeapAlloc(SystemHeap, 0,
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
            MINMAXINFO *mmi = (MINMAXINFO *)HeapAlloc( SystemHeap, 0,
                                                sizeof(*mmi) + sizeof(LPARAM));
            if (!mmi) return -1;
            STRUCT32_MINMAXINFO16to32( (MINMAXINFO16*)PTR_SEG_TO_LIN(*plparam),
                                       mmi );
            *(LPARAM *)(mmi + 1) = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)mmi;
        }
        return 1;
    case WM_GETTEXT:
    case WM_SETTEXT:
        *plparam = (LPARAM)PTR_SEG_TO_LIN(*plparam);
        return 0;
    case WM_MDICREATE:
        {
            MDICREATESTRUCT16 *cs16 =
                (MDICREATESTRUCT16 *)PTR_SEG_TO_LIN(*plparam);
            MDICREATESTRUCTA *cs =
                (MDICREATESTRUCTA *)HeapAlloc( SystemHeap, 0,
                                                sizeof(*cs) + sizeof(LPARAM) );
            if (!cs) return -1;
            STRUCT32_MDICREATESTRUCT16to32A( cs16, cs );
            cs->szTitle = (LPCSTR)PTR_SEG_TO_LIN(cs16->szTitle);
            cs->szClass = (LPCSTR)PTR_SEG_TO_LIN(cs16->szClass);
            *(LPARAM *)(cs + 1) = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)cs;
        }
        return 1;
    case WM_MDIGETACTIVE:
        *plparam = (LPARAM)HeapAlloc( SystemHeap, 0, sizeof(BOOL) );
        *(BOOL*)(*plparam) = 0;
        return 1;
    case WM_MDISETMENU:
        if(wParam16==TRUE)
           *pmsg32=WM_MDIREFRESHMENU;
        *pwparam32 = (WPARAM)(HMENU)LOWORD(*plparam);
        *plparam   = (LPARAM)(HMENU)HIWORD(*plparam);
        return 0;
    case WM_MENUCHAR:
    case WM_MENUSELECT:
        *pwparam32 = MAKEWPARAM( wParam16, LOWORD(*plparam) );
        *plparam   = (LPARAM)(HMENU)HIWORD(*plparam);
        return 0;
    case WM_MDIACTIVATE:
	if( *plparam )
	{
	    *pwparam32 = (WPARAM)(HWND)HIWORD(*plparam);
	    *plparam   = (LPARAM)(HWND)LOWORD(*plparam);
	}
	else /* message sent to MDI client */
	    *pwparam32 = wParam16;
        return 0;
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS16 *nc16;
            NCCALCSIZE_PARAMS *nc;

            nc = (NCCALCSIZE_PARAMS *)HeapAlloc( SystemHeap, 0,
                                                sizeof(*nc) + sizeof(LPARAM) );
            if (!nc) return -1;
            nc16 = (NCCALCSIZE_PARAMS16 *)PTR_SEG_TO_LIN(*plparam);
            CONV_RECT16TO32( &nc16->rgrc[0], &nc->rgrc[0] );
            if (wParam16)
            {
                nc->lppos = (WINDOWPOS *)HeapAlloc( SystemHeap, 0,
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
            CREATESTRUCTA *cs = (CREATESTRUCTA *)HeapAlloc( SystemHeap, 0,
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
            *plparam   = (LPARAM)(HWND)LOWORD(*plparam);
        }
        return 0;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS *wp = (WINDOWPOS *)HeapAlloc( SystemHeap, 0,
                                                sizeof(*wp) + sizeof(LPARAM) );
            if (!wp) return -1;
            STRUCT32_WINDOWPOS16to32( (WINDOWPOS16 *)PTR_SEG_TO_LIN(*plparam),
                                      wp );
            *(LPARAM *)(wp + 1) = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)wp;
        }
        return 1;
    case WM_GETDLGCODE:
        if (*plparam)
        {
            LPMSG16 msg16 = (LPMSG16)PTR_SEG_TO_LIN(*plparam);
            LPMSG msg32 = (LPMSG)HeapAlloc( SystemHeap, 0, sizeof(MSG) );

            if (!msg32) return -1;
            msg32->hwnd = msg16->hwnd;
            msg32->lParam = msg16->lParam;
            msg32->time = msg16->time;
            CONV_POINT16TO32(&msg16->pt,&msg32->pt);
            /* this is right, right? */
            if (WINPROC_MapMsg16To32A(msg16->message,msg16->wParam,
                                     &msg32->message,&msg32->wParam,
                                     &msg32->lParam)<0) {
                HeapFree( SystemHeap, 0, msg32 );
                return -1;
            }
            *plparam = (LPARAM)msg32;
            return 1;
        }
        else return 0;
    case WM_NOTIFY:
    	*plparam = (LPARAM)PTR_SEG_TO_LIN(*plparam);
    	return 1;
    case WM_ACTIVATEAPP:
    	if (*plparam)
	{ /* We need this when SetActiveWindow sends a Sendmessage16() to
	     a 32bit window. Might be superflous with 32bit interprocess
	     message queues.
	  */
	  HTASK16 htask = (HTASK16) *plparam;
	  DWORD idThread = (DWORD)((TDB*)GlobalLock16(htask))->teb->tid;
	  *plparam = (LPARAM) idThread;
	}
	return 1;
    case WM_ASKCBFORMATNAME:
    case WM_DEVMODECHANGE:
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    case WM_WININICHANGE:
        FIXME_(msg)("message %04x needs translation\n",msg16 );
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
LRESULT WINPROC_UnmapMsg16To32A( HWND16 hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                 LRESULT result )
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
            MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lParam;
            lParam = *(LPARAM *)(mis + 1);
            mis16 = (MEASUREITEMSTRUCT16 *)PTR_SEG_TO_LIN(lParam);
            mis16->itemWidth  = (UINT16)mis->itemWidth;
            mis16->itemHeight = (UINT16)mis->itemHeight;
            HeapFree( SystemHeap, 0, mis );
        }
        break;
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO *mmi = (MINMAXINFO *)lParam;
            lParam = *(LPARAM *)(mmi + 1);
            STRUCT32_MINMAXINFO32to16( mmi,
                                       (MINMAXINFO16 *)PTR_SEG_TO_LIN(lParam));
            HeapFree( SystemHeap, 0, mmi );
        }
        break;
    case WM_MDICREATE:
        {
            MDICREATESTRUCTA *cs = (MDICREATESTRUCTA *)lParam;
            lParam = *(LPARAM *)(cs + 1);
            STRUCT32_MDICREATESTRUCT32Ato16( cs,
                                 (MDICREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam) );
            HeapFree( SystemHeap, 0, cs );
        }
        break;
    case WM_MDIGETACTIVE:
        result = MAKELONG( LOWORD(result), (BOOL16)(*(BOOL *)lParam) );
        HeapFree( SystemHeap, 0, (BOOL *)lParam );
        break;
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS16 *nc16;
            NCCALCSIZE_PARAMS *nc = (NCCALCSIZE_PARAMS *)lParam;
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
            CREATESTRUCTA *cs = (CREATESTRUCTA *)lParam;
            lParam = *(LPARAM *)(cs + 1);
            STRUCT32_CREATESTRUCT32Ato16( cs,
                                    (CREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam) );
            HeapFree( SystemHeap, 0, cs );
        }
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS *wp = (WINDOWPOS *)lParam;
            lParam = *(LPARAM *)(wp + 1);
            STRUCT32_WINDOWPOS32to16(wp,(WINDOWPOS16 *)PTR_SEG_TO_LIN(lParam));
            HeapFree( SystemHeap, 0, wp );
        }
        break;
    case WM_GETDLGCODE:
        if (lParam)
        {
            LPMSG msg32 = (LPMSG)lParam;

            WINPROC_UnmapMsg16To32A( hwnd, msg32->message, msg32->wParam, msg32->lParam,
                                     result);
            HeapFree( SystemHeap, 0, msg32 );
        }
        break;
    }
    return result;
}


/**********************************************************************
 *	     WINPROC_MapMsg16To32W
 *
 * Map a message from 16- to 32-bit Unicode.
 * Return value is -1 on error, 0 if OK, 1 if an UnmapMsg call is needed.
 */
INT WINPROC_MapMsg16To32W( HWND16 hwnd, UINT16 msg16, WPARAM16 wParam16, UINT *pmsg32,
                             WPARAM *pwparam32, LPARAM *plparam )
{
    switch(msg16)
    {
    case WM_GETTEXT:
    case WM_SETTEXT:
        *plparam = (LPARAM)PTR_SEG_TO_LIN(*plparam);
        return WINPROC_MapMsg32ATo32W( hwnd, *pmsg32, *pwparam32, plparam );
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT16 *cs16 = (CREATESTRUCT16 *)PTR_SEG_TO_LIN(*plparam);
            CREATESTRUCTW *cs = (CREATESTRUCTW *)HeapAlloc( SystemHeap, 0,
                                                sizeof(*cs) + sizeof(LPARAM) );
            if (!cs) return -1;
            STRUCT32_CREATESTRUCT16to32A( cs16, (CREATESTRUCTA *)cs );
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
            MDICREATESTRUCTW *cs =
                (MDICREATESTRUCTW *)HeapAlloc( SystemHeap, 0,
                                                sizeof(*cs) + sizeof(LPARAM) );
            if (!cs) return -1;
            STRUCT32_MDICREATESTRUCT16to32A( cs16, (MDICREATESTRUCTA *)cs );
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
    case WM_GETDLGCODE:
        if (*plparam)
        {
            LPMSG16 msg16 = (LPMSG16)PTR_SEG_TO_LIN(*plparam);
            LPMSG msg32 = (LPMSG)HeapAlloc( SystemHeap, 0, sizeof(MSG) );

            if (!msg32) return -1;
            msg32->hwnd = msg16->hwnd;
            msg32->lParam = msg16->lParam;
            msg32->time = msg16->time;
            CONV_POINT16TO32(&msg16->pt,&msg32->pt);
            /* this is right, right? */
            if (WINPROC_MapMsg16To32W(hwnd, msg16->message,msg16->wParam,
                                     &msg32->message,&msg32->wParam,
                                     &msg32->lParam)<0) {
                HeapFree( SystemHeap, 0, msg32 );
                return -1;
            }
            *plparam = (LPARAM)msg32;
            return 1;
        }
        else return 0;
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
LRESULT WINPROC_UnmapMsg16To32W( HWND16 hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                 LRESULT result )
{
    switch(msg)
    {
    case WM_GETTEXT:
    case WM_SETTEXT:
        WINPROC_UnmapMsg32ATo32W( hwnd, msg, wParam, lParam );
        break;
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCTW *cs = (CREATESTRUCTW *)lParam;
            lParam = *(LPARAM *)(cs + 1);
            STRUCT32_CREATESTRUCT32Ato16( (CREATESTRUCTA *)cs,
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
            MDICREATESTRUCTW *cs = (MDICREATESTRUCTW *)lParam;
            lParam = *(LPARAM *)(cs + 1);
            STRUCT32_MDICREATESTRUCT32Ato16( (MDICREATESTRUCTA *)cs,
                                 (MDICREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam) );
            if (HIWORD(cs->szTitle))
                HeapFree( SystemHeap, 0, (LPVOID)cs->szTitle );
            if (HIWORD(cs->szClass))
                HeapFree( SystemHeap, 0, (LPVOID)cs->szClass );
            HeapFree( SystemHeap, 0, cs );
        }
        break;
    case WM_GETDLGCODE:
        if (lParam)
        {
            LPMSG msg32 = (LPMSG)lParam;

            WINPROC_UnmapMsg16To32W( hwnd, msg32->message, msg32->wParam, msg32->lParam,
                                     result);
            HeapFree( SystemHeap, 0, msg32 );
        }
        break;
    default:
        return WINPROC_UnmapMsg16To32A( hwnd, msg, wParam, lParam, result );
    }
    return result;
}


/**********************************************************************
 *	     WINPROC_MapMsg32ATo16
 *
 * Map a message from 32-bit Ansi to 16-bit.
 * Return value is -1 on error, 0 if OK, 1 if an UnmapMsg call is needed.
 */
INT WINPROC_MapMsg32ATo16( HWND hwnd, UINT msg32, WPARAM wParam32,
                             UINT16 *pmsg16, WPARAM16 *pwparam16,
                             LPARAM *plparam )
{
    *pmsg16 = (UINT16)msg32;
    *pwparam16 = (WPARAM16)LOWORD(wParam32);
    switch(msg32)
    {
    case BM_GETCHECK:
    case BM_SETCHECK:
    case BM_GETSTATE:
    case BM_SETSTATE:
    case BM_SETSTYLE:
        *pmsg16 = (UINT16)msg32 + (BM_GETCHECK16 - BM_GETCHECK);
        return 0;

    case EM_GETSEL:
    case EM_GETRECT:
    case EM_SETRECT:
    case EM_SETRECTNP:
    case EM_SCROLL:
    case EM_LINESCROLL:
    case EM_SCROLLCARET:
    case EM_GETMODIFY:
    case EM_SETMODIFY:
    case EM_GETLINECOUNT:
    case EM_LINEINDEX:
    case EM_SETHANDLE:
    case EM_GETHANDLE:
    case EM_GETTHUMB:
    case EM_LINELENGTH:
    case EM_REPLACESEL:
    case EM_GETLINE:
    case EM_LIMITTEXT:
    case EM_CANUNDO:
    case EM_UNDO:
    case EM_FMTLINES:
    case EM_LINEFROMCHAR:
    case EM_SETTABSTOPS:
    case EM_SETPASSWORDCHAR:
    case EM_EMPTYUNDOBUFFER:
    case EM_GETFIRSTVISIBLELINE:
    case EM_SETREADONLY:
    case EM_SETWORDBREAKPROC:
    case EM_GETWORDBREAKPROC:
    case EM_GETPASSWORDCHAR:
        *pmsg16 = (UINT16)msg32 + (EM_GETSEL16 - EM_GETSEL);
        return 0;

    case LB_CARETOFF:
    case LB_CARETON:
    case LB_DELETESTRING:
    case LB_GETANCHORINDEX:
    case LB_GETCARETINDEX:
    case LB_GETCOUNT:
    case LB_GETCURSEL:
    case LB_GETHORIZONTALEXTENT:
    case LB_GETITEMDATA:
    case LB_GETITEMHEIGHT:
    case LB_GETSEL:
    case LB_GETSELCOUNT:
    case LB_GETTEXTLEN:
    case LB_GETTOPINDEX:
    case LB_RESETCONTENT:
    case LB_SELITEMRANGE:
    case LB_SELITEMRANGEEX:
    case LB_SETANCHORINDEX:
    case LB_SETCARETINDEX:
    case LB_SETCOLUMNWIDTH:
    case LB_SETCURSEL:
    case LB_SETHORIZONTALEXTENT:
    case LB_SETITEMDATA:
    case LB_SETITEMHEIGHT:
    case LB_SETSEL:
    case LB_SETTOPINDEX:
        *pmsg16 = (UINT16)msg32 + (LB_ADDSTRING16 - LB_ADDSTRING);
        return 0;
    case CB_DELETESTRING:
    case CB_GETCOUNT:
    case CB_GETLBTEXTLEN:
    case CB_LIMITTEXT:
    case CB_RESETCONTENT:
    case CB_SETEDITSEL:
    case CB_GETCURSEL:
    case CB_SETCURSEL:
    case CB_SHOWDROPDOWN:
    case CB_SETITEMDATA:
    case CB_SETITEMHEIGHT:
    case CB_GETITEMHEIGHT:
    case CB_SETEXTENDEDUI:
    case CB_GETEXTENDEDUI:
    case CB_GETDROPPEDSTATE:
	*pmsg16 = (UINT16)msg32 + (CB_GETEDITSEL16 - CB_GETEDITSEL);
	return 0;
    case CB_GETEDITSEL:
	*pmsg16 = CB_GETEDITSEL16;
	return 1;

    case LB_ADDSTRING:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_INSERTSTRING:
    case LB_SELECTSTRING:
    case LB_DIR:
    case LB_ADDFILE:
        {
            LPSTR str = SEGPTR_STRDUP( (LPSTR)*plparam );
            if (!str) return -1;
            *plparam = (LPARAM)SEGPTR_GET(str);
        }
        *pmsg16 = (UINT16)msg32 + (LB_ADDSTRING16 - LB_ADDSTRING);
        return 1;

    case CB_ADDSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_INSERTSTRING:
    case CB_SELECTSTRING:
    case CB_DIR:
	{
	    LPSTR str = SEGPTR_STRDUP( (LPSTR)*plparam );
	    if (!str) return -1;
	    *plparam = (LPARAM)SEGPTR_GET(str);
	}
	*pmsg16 = (UINT16)msg32 + (CB_GETEDITSEL16 - CB_GETEDITSEL);
	return 1;

    case LB_GETITEMRECT:
        {
            RECT16 *rect;
            rect = (RECT16 *)SEGPTR_ALLOC( sizeof(RECT16) + sizeof(LPARAM) );
            if (!rect) return -1;
            *(LPARAM *)(rect + 1) = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)SEGPTR_GET(rect);
        }
	*pmsg16 = LB_GETITEMRECT16;
        return 1;
    case LB_GETSELITEMS:
        {
            LPINT16 items;
            *pwparam16 = (WPARAM16)MIN( wParam32, 0x7f80 ); /* Must be < 64K */
            if (!(items = SEGPTR_ALLOC( *pwparam16 * sizeof(INT16)
                                        + sizeof(LPARAM)))) return -1;
            *((LPARAM *)items)++ = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)SEGPTR_GET(items);
        }
	*pmsg16 = LB_GETSELITEMS16;
        return 1;
    case LB_SETTABSTOPS:
        if (wParam32)
        {
            INT i;
            LPINT16 stops;
            *pwparam16 = (WPARAM16)MIN( wParam32, 0x7f80 ); /* Must be < 64K */
            if (!(stops = SEGPTR_ALLOC( *pwparam16 * sizeof(INT16)
                                        + sizeof(LPARAM)))) return -1;
            for (i = 0; i < *pwparam16; i++) stops[i] = *((LPINT)*plparam+i);
            *plparam = (LPARAM)SEGPTR_GET(stops);
            return 1;
        }
        *pmsg16 = LB_SETTABSTOPS16;
        return 0;

    case CB_GETDROPPEDCONTROLRECT:
        {
	    RECT16 *rect;
	    rect = (RECT16 *)SEGPTR_ALLOC( sizeof(RECT16) + sizeof(LPARAM) );
	    if (!rect) return -1;
	    *(LPARAM *)(rect + 1) = *plparam;  /* Store the previous lParam */
	    *plparam = (LPARAM)SEGPTR_GET(rect);
        }
	*pmsg16 = CB_GETDROPPEDCONTROLRECT16;
        return 1;

    case LB_GETTEXT:
	*plparam = (LPARAM)MapLS( (LPVOID)(*plparam) );
	*pmsg16 = LB_GETTEXT16;
	return 1;

    case CB_GETLBTEXT:
	*plparam = (LPARAM)MapLS( (LPVOID)(*plparam) );
	*pmsg16 = CB_GETLBTEXT16;
	return 1;

    case EM_SETSEL:
	*pwparam16 = 0;
	*plparam = MAKELONG( (INT16)(INT)wParam32, (INT16)*plparam );
	*pmsg16 = EM_SETSEL16;
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
            COMPAREITEMSTRUCT *cis32 = (COMPAREITEMSTRUCT *)*plparam;
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
            DELETEITEMSTRUCT *dis32 = (DELETEITEMSTRUCT *)*plparam;
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
            DRAWITEMSTRUCT *dis32 = (DRAWITEMSTRUCT *)*plparam;
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
            MEASUREITEMSTRUCT *mis32 = (MEASUREITEMSTRUCT *)*plparam;
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
            STRUCT32_MINMAXINFO32to16( (MINMAXINFO *)*plparam, mmi );
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
            MDICREATESTRUCTA *cs32 = (MDICREATESTRUCTA *)*plparam;
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
    case WM_MDIGETACTIVE:
        return 1;
    case WM_MDISETMENU:
        *plparam   = MAKELPARAM( (HMENU16)LOWORD(wParam32),
                                 (HMENU16)LOWORD(*plparam) );
        *pwparam16 = (*plparam == 0);
        return 0;
    case WM_MENUCHAR:
    case WM_MENUSELECT:
        *plparam = MAKELPARAM( HIWORD(wParam32), (HMENU16)*plparam );
        return 0;
    case WM_MDIACTIVATE:
	{
            WND *tempWnd = WIN_FindWndPtr(hwnd);
            if( WIDGETS_IsControl(tempWnd, BIC32_MDICLIENT) )
            {
	    *pwparam16 = (HWND)wParam32;
	    *plparam = 0;
	}
	else
	{
	    *pwparam16 = ((HWND)*plparam == hwnd);
	    *plparam = MAKELPARAM( (HWND16)LOWORD(*plparam),
				   (HWND16)LOWORD(wParam32) );
	}
            WIN_ReleaseWndPtr(tempWnd);
        }
        return 0;
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS *nc32 = (NCCALCSIZE_PARAMS *)*plparam;
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
            CREATESTRUCTA *cs32 = (CREATESTRUCTA *)*plparam;
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
    case WM_NOTIFY:
	*plparam = MapLS( (NMHDR *)*plparam ); /* NMHDR is already 32-bit */
	return 1;
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
            STRUCT32_WINDOWPOS32to16( (WINDOWPOS *)*plparam, wp );
            *(LPARAM *)(wp + 1) = *plparam;  /* Store the previous lParam */
            *plparam = (LPARAM)SEGPTR_GET(wp);
        }
        return 1;
    case WM_GETDLGCODE:
         if (*plparam) {
            LPMSG msg32 = (LPMSG) *plparam;
            LPMSG16 msg16 = (LPMSG16) SEGPTR_NEW( MSG16 );

            if (!msg16) return -1;
            msg16->hwnd = msg32->hwnd;
            msg16->lParam = msg32->lParam;
            msg16->time = msg32->time;
            CONV_POINT32TO16(&msg32->pt,&msg16->pt);
            /* this is right, right? */
            if (WINPROC_MapMsg32ATo16(msg32->hwnd,msg32->message,msg32->wParam,
                         &msg16->message,&msg16->wParam, &msg16->lParam)<0) {
                SEGPTR_FREE( msg16 );
                return -1;
            }
            *plparam = (LPARAM)SEGPTR_GET(msg16);
            return 1;
        }
        return 0;

    case WM_ACTIVATEAPP:
	if (*plparam) {
	*plparam = (LPARAM)THREAD_IdToTEB((DWORD) *plparam)->htask16;
	}
	return 1;
    case WM_ASKCBFORMATNAME:
    case WM_DEVMODECHANGE:
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    case WM_WININICHANGE:
        FIXME_(msg)("message %04x needs translation\n", msg32 );
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
void WINPROC_UnmapMsg32ATo16( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                              MSGPARAM16* p16 ) 
{
    switch(msg)
    {
    case LB_ADDFILE:
    case LB_ADDSTRING:
    case LB_DIR:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_INSERTSTRING:
    case LB_SELECTSTRING:
    case LB_SETTABSTOPS:
    case CB_ADDSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_INSERTSTRING:
    case CB_SELECTSTRING:
    case CB_DIR:
    case WM_COMPAREITEM:
    case WM_DELETEITEM:
    case WM_DRAWITEM:
    case WM_SETTEXT:
        SEGPTR_FREE( PTR_SEG_TO_LIN(p16->lParam) );
        break;

    case CB_GETDROPPEDCONTROLRECT:
    case LB_GETITEMRECT:
        {
            RECT16 *rect = (RECT16 *)PTR_SEG_TO_LIN(p16->lParam);
            p16->lParam = *(LPARAM *)(rect + 1);
            CONV_RECT16TO32( rect, (RECT *)(p16->lParam));
            SEGPTR_FREE( rect );
        }
        break;
    case LB_GETSELITEMS:
        {
            INT i;
            LPINT16 items = (LPINT16)PTR_SEG_TO_LIN(lParam);
            p16->lParam = *((LPARAM *)items - 1);
            for (i = 0; i < p16->wParam; i++) *((LPINT)(p16->lParam) + i) = items[i];
            SEGPTR_FREE( (LPARAM *)items - 1 );
        }
        break;

    case CB_GETEDITSEL:
	if( wParam )
	    *((LPUINT)(wParam)) = LOWORD(p16->lResult);
	if( lParam )
	    *((LPUINT)(lParam)) = HIWORD(p16->lResult);	/* FIXME: substract 1? */
	break;

    case LB_GETTEXT:
    case CB_GETLBTEXT:
	UnMapLS( (SEGPTR)(p16->lParam) );
	break;

    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT16 *mis = (MEASUREITEMSTRUCT16 *)PTR_SEG_TO_LIN(p16->lParam);
            MEASUREITEMSTRUCT *mis32 = *(MEASUREITEMSTRUCT **)(mis + 1);
            mis32->itemWidth  = mis->itemWidth;
            mis32->itemHeight = mis->itemHeight;
            SEGPTR_FREE(mis);
        }
        break;
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO16 *mmi = (MINMAXINFO16 *)PTR_SEG_TO_LIN(p16->lParam);
            p16->lParam = *(LPARAM *)(mmi + 1);
            STRUCT32_MINMAXINFO16to32( mmi, (MINMAXINFO *)(p16->lParam) );
            SEGPTR_FREE(mmi);
        }
        break;
    case WM_GETTEXT:
        {
            LPSTR str = (LPSTR)PTR_SEG_TO_LIN(p16->lParam);
            p16->lParam = *((LPARAM *)str - 1);
            lstrcpynA( (LPSTR)(p16->lParam), str, p16->wParam );
            SEGPTR_FREE( (LPARAM *)str - 1 );
        }
        break;
    case WM_MDICREATE:
        {
            MDICREATESTRUCT16 *cs = (MDICREATESTRUCT16*)PTR_SEG_TO_LIN(p16->lParam);
            SEGPTR_FREE( PTR_SEG_TO_LIN(cs->szTitle) );
            SEGPTR_FREE( PTR_SEG_TO_LIN(cs->szClass) );
            SEGPTR_FREE( cs );
        }
        break;
    case WM_MDIGETACTIVE:
        if (lParam) *(BOOL *)lParam = (BOOL16)HIWORD(p16->lResult);
        p16->lResult = (HWND)LOWORD(p16->lResult);
        break;
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS *nc32;
            NCCALCSIZE_PARAMS16 *nc = (NCCALCSIZE_PARAMS16 *)PTR_SEG_TO_LIN(p16->lParam);
            p16->lParam = *(LPARAM *)(nc + 1);
            nc32 = (NCCALCSIZE_PARAMS *)(p16->lParam);
            CONV_RECT16TO32( &nc->rgrc[0], &nc32->rgrc[0] );
            if (p16->wParam)
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
            CREATESTRUCT16 *cs = (CREATESTRUCT16 *)PTR_SEG_TO_LIN(p16->lParam);
            SEGPTR_FREE( PTR_SEG_TO_LIN(cs->lpszName) );
            SEGPTR_FREE( PTR_SEG_TO_LIN(cs->lpszClass) );
            SEGPTR_FREE( cs );
        }
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS16 *wp = (WINDOWPOS16 *)PTR_SEG_TO_LIN(p16->lParam);
            p16->lParam = *(LPARAM *)(wp + 1);
            STRUCT32_WINDOWPOS16to32( wp, (WINDOWPOS *)p16->lParam );
            SEGPTR_FREE(wp);
        }
        break;
    case WM_NOTIFY:
	UnMapLS(p16->lParam);
	break;
    case WM_GETDLGCODE:
        if (p16->lParam)
        {
            LPMSG16 msg16 = (LPMSG16)PTR_SEG_TO_LIN(p16->lParam);
            MSGPARAM16 msgp16;
            msgp16.wParam=msg16->wParam;
            msgp16.lParam=msg16->lParam;
            WINPROC_UnmapMsg32ATo16(((LPMSG)lParam)->hwnd, ((LPMSG)lParam)->message,
                    ((LPMSG)lParam)->wParam, ((LPMSG)lParam)->lParam,
                    &msgp16 );
            SEGPTR_FREE(msg16);
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
INT WINPROC_MapMsg32WTo16( HWND hwnd, UINT msg32, WPARAM wParam32,
                             UINT16 *pmsg16, WPARAM16 *pwparam16,
                             LPARAM *plparam )
{
    switch(msg32)
    {
    case LB_ADDSTRING:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_INSERTSTRING:
    case LB_SELECTSTRING:
    case LB_DIR:
    case LB_ADDFILE:
        {
            LPSTR str = SEGPTR_STRDUP_WtoA( (LPWSTR)*plparam );
            if (!str) return -1;
            *pwparam16 = (WPARAM16)LOWORD(wParam32);
            *plparam   = (LPARAM)SEGPTR_GET(str);
        }
	*pmsg16 = (UINT16)msg32 + (LB_ADDSTRING16 - LB_ADDSTRING);
        return 1;

    case CB_ADDSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_INSERTSTRING:
    case CB_SELECTSTRING:
    case CB_DIR:
        {
            LPSTR str = SEGPTR_STRDUP_WtoA( (LPWSTR)*plparam );
            if (!str) return -1;
            *pwparam16 = (WPARAM16)LOWORD(wParam32);
            *plparam   = (LPARAM)SEGPTR_GET(str);
        }
	*pmsg16 = (UINT16)msg32 + (CB_ADDSTRING16 - CB_ADDSTRING);
        return 1;

    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT16 *cs;
            CREATESTRUCTW *cs32 = (CREATESTRUCTW *)*plparam;
            LPSTR name, cls;

            if (!(cs = SEGPTR_NEW(CREATESTRUCT16))) return -1;
            STRUCT32_CREATESTRUCT32Ato16( (CREATESTRUCTA *)cs32, cs );
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
            MDICREATESTRUCTW *cs32 = (MDICREATESTRUCTW *)*plparam;
            LPSTR name, cls;

            if (!(cs = SEGPTR_NEW(MDICREATESTRUCT16))) return -1;
            STRUCT32_MDICREATESTRUCT32Ato16( (MDICREATESTRUCTA *)cs32, cs );
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
        return WINPROC_MapMsg32ATo16( hwnd, msg32, wParam32, pmsg16,
                                      pwparam16, plparam );
    }
}


/**********************************************************************
 *	     WINPROC_UnmapMsg32WTo16
 *
 * Unmap a message that was mapped from 32-bit Unicode to 16-bit.
 */
void WINPROC_UnmapMsg32WTo16( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                              MSGPARAM16* p16 )
{
    switch(msg)
    {
    case WM_GETTEXT:
        {
            LPSTR str = (LPSTR)PTR_SEG_TO_LIN(p16->lParam);
            p16->lParam = *((LPARAM *)str - 1);
            lstrcpyAtoW( (LPWSTR)(p16->lParam), str );
            SEGPTR_FREE( (LPARAM *)str - 1 );
        }
        break;
    default:
        WINPROC_UnmapMsg32ATo16( hwnd, msg, wParam, lParam, p16 );
        break;
    }
}


/**********************************************************************
 *	     WINPROC_CallProc32ATo32W
 *
 * Call a window procedure, translating args from Ansi to Unicode.
 */
static LRESULT WINPROC_CallProc32ATo32W( WNDPROC func, HWND hwnd,
                                         UINT msg, WPARAM wParam,
                                         LPARAM lParam )
{
    LRESULT result;

    if (WINPROC_MapMsg32ATo32W( hwnd, msg, wParam, &lParam ) == -1) return 0;
    result = WINPROC_CallWndProc( func, hwnd, msg, wParam, lParam );
    WINPROC_UnmapMsg32ATo32W( hwnd, msg, wParam, lParam );
    return result;
}


/**********************************************************************
 *	     WINPROC_CallProc32WTo32A
 *
 * Call a window procedure, translating args from Unicode to Ansi.
 */
static LRESULT WINPROC_CallProc32WTo32A( WNDPROC func, HWND hwnd,
                                         UINT msg, WPARAM wParam,
                                         LPARAM lParam )
{
    LRESULT result;

    if (WINPROC_MapMsg32WTo32A( hwnd, msg, wParam, &lParam ) == -1) return 0;
    result = WINPROC_CallWndProc( func, hwnd, msg, wParam, lParam );
    WINPROC_UnmapMsg32WTo32A( hwnd, msg, wParam, lParam );
    return result;
}


/**********************************************************************
 *	     WINPROC_CallProc16To32A
 *
 * Call a 32-bit window procedure, translating the 16-bit args.
 */
LRESULT WINPROC_CallProc16To32A( HWND16 hwnd, UINT16 msg,
                                 WPARAM16 wParam, LPARAM lParam,
                                 WNDPROC func )
{
    LRESULT result;
    UINT msg32;
    WPARAM wParam32;

    if (WINPROC_MapMsg16To32A( msg, wParam, &msg32, &wParam32, &lParam ) == -1)
        return 0;
    result = WINPROC_CallWndProc( func, hwnd, msg32, wParam32, lParam );
    return WINPROC_UnmapMsg16To32A( hwnd, msg32, wParam32, lParam, result );
}


/**********************************************************************
 *	     WINPROC_CallProc16To32W
 *
 * Call a 32-bit window procedure, translating the 16-bit args.
 */
LRESULT WINPROC_CallProc16To32W( HWND16 hwnd, UINT16 msg,
                                 WPARAM16 wParam, LPARAM lParam,
                                 WNDPROC func )
{
    LRESULT result;
    UINT msg32;
    WPARAM wParam32;

    if (WINPROC_MapMsg16To32W( hwnd, msg, wParam, &msg32, &wParam32, &lParam ) == -1)
        return 0;

    result = WINPROC_CallWndProc( func, hwnd, msg32, wParam32, lParam );
    
    return WINPROC_UnmapMsg16To32W( hwnd, msg32, wParam32, lParam, result );
}


/**********************************************************************
 *	     WINPROC_CallProc32ATo16
 *
 * Call a 16-bit window procedure, translating the 32-bit args.
 */
static LRESULT WINAPI WINPROC_CallProc32ATo16( WNDPROC16 func, HWND hwnd,
                                               UINT msg, WPARAM wParam,
                                               LPARAM lParam )
{
    UINT16 msg16;
    MSGPARAM16 mp16;

    mp16.lParam = lParam;
    if (WINPROC_MapMsg32ATo16( hwnd, msg, wParam, 
                               &msg16, &mp16.wParam, &mp16.lParam ) == -1)
        return 0;
    mp16.lResult = Callbacks->CallWndProc( func, hwnd, msg16,
                                           mp16.wParam, mp16.lParam );
    WINPROC_UnmapMsg32ATo16( hwnd, msg, wParam, lParam, &mp16 );
    return mp16.lResult;
}


/**********************************************************************
 *	     WINPROC_CallProc32WTo16
 *
 * Call a 16-bit window procedure, translating the 32-bit args.
 */
static LRESULT WINAPI WINPROC_CallProc32WTo16( WNDPROC16 func, HWND hwnd,
                                               UINT msg, WPARAM wParam,
                                               LPARAM lParam )
{
    UINT16 msg16;
    MSGPARAM16 mp16;

    mp16.lParam = lParam;
    if (WINPROC_MapMsg32WTo16( hwnd, msg, wParam, &msg16, &mp16.wParam,
                               &mp16.lParam ) == -1)
        return 0;
    mp16.lResult = Callbacks->CallWndProc( func, hwnd, msg16,
                                           mp16.wParam, mp16.lParam );
    WINPROC_UnmapMsg32WTo16( hwnd, msg, wParam, lParam, &mp16 );
    return mp16.lResult;
}


/**********************************************************************
 *	     CallWindowProc16    (USER.122)
 */
LRESULT WINAPI CallWindowProc16( WNDPROC16 func, HWND16 hwnd, UINT16 msg,
                                 WPARAM16 wParam, LPARAM lParam )
{
    WINDOWPROC *proc = WINPROC_GetPtr( func );

    if (!proc)
        return Callbacks->CallWndProc( func, hwnd, msg, wParam, lParam );

#if testing
    func = WINPROC_GetProc( (HWINDOWPROC)proc, WIN_PROC_16 );
    return Callbacks->CallWndProc( func, hwnd, msg, wParam, lParam );
#endif
    
    switch(proc->type)
    {
    case WIN_PROC_16:
        if (!proc->thunk.t_from32.proc) return 0;
        return Callbacks->CallWndProc( proc->thunk.t_from32.proc,
                                       hwnd, msg, wParam, lParam );
    case WIN_PROC_32A:
        if (!proc->thunk.t_from16.proc) return 0;
        return WINPROC_CallProc16To32A( hwnd, msg, wParam, lParam,
                                        proc->thunk.t_from16.proc );
    case WIN_PROC_32W:
        if (!proc->thunk.t_from16.proc) return 0;
        return WINPROC_CallProc16To32W( hwnd, msg, wParam, lParam,
                                        proc->thunk.t_from16.proc );
    default:
        WARN_(relay)("Invalid proc %p\n", proc );
        return 0;
    }
}


/**********************************************************************
 *	     CallWindowProc32A    (USER32.18) 
 *
 * The CallWindowProc() function invokes the windows procedure _func_,
 * with _hwnd_ as the target window, the message specified by _msg_, and
 * the message parameters _wParam_ and _lParam_.
 *
 * Some kinds of argument conversion may be done, I'm not sure what.
 *
 * CallWindowProc() may be used for windows subclassing. Use
 * SetWindowLong() to set a new windows procedure for windows of the
 * subclass, and handle subclassed messages in the new windows
 * procedure. The new windows procedure may then use CallWindowProc()
 * with _func_ set to the parent class's windows procedure to dispatch
 * the message to the superclass.
 *
 * RETURNS
 *
 *    The return value is message dependent.
 *
 * CONFORMANCE
 *
 *   ECMA-234, Win32 
 */
LRESULT WINAPI CallWindowProcA( 
    WNDPROC func, /* window procedure */
    HWND hwnd, /* target window */
    UINT msg,  /* message */
    WPARAM wParam, /* message dependent parameter */
    LPARAM lParam    /* message dependent parameter */
) {
    WINDOWPROC *proc = WINPROC_GetPtr( (WNDPROC16)func );

    if (!proc) return WINPROC_CallWndProc( func, hwnd, msg, wParam, lParam );

#if testing
    func = WINPROC_GetProc( (HWINDOWPROC)proc, WIN_PROC_32A );
    return WINPROC_CallWndProc( func, hwnd, msg, wParam, lParam );
#endif

    switch(proc->type)
    {
    case WIN_PROC_16:
        if (!proc->thunk.t_from32.proc) return 0;
        return WINPROC_CallProc32ATo16( proc->thunk.t_from32.proc,
                                        hwnd, msg, wParam, lParam );
    case WIN_PROC_32A:
        if (!proc->thunk.t_from16.proc) return 0;
        return WINPROC_CallWndProc( proc->thunk.t_from16.proc,
                                      hwnd, msg, wParam, lParam );
    case WIN_PROC_32W:
        if (!proc->thunk.t_from16.proc) return 0;
        return WINPROC_CallProc32ATo32W( proc->thunk.t_from16.proc,
                                         hwnd, msg, wParam, lParam );
    default:
        WARN_(relay)("Invalid proc %p\n", proc );
        return 0;
    }
}


/**********************************************************************
 *	     CallWindowProc32W    (USER32.19)
 */
LRESULT WINAPI CallWindowProcW( WNDPROC func, HWND hwnd, UINT msg,
                                  WPARAM wParam, LPARAM lParam )
{
    WINDOWPROC *proc = WINPROC_GetPtr( (WNDPROC16)func );

    if (!proc) return WINPROC_CallWndProc( func, hwnd, msg, wParam, lParam );

#if testing
    func = WINPROC_GetProc( (HWINDOWPROC)proc, WIN_PROC_32W );
    return WINPROC_CallWndProc( func, hwnd, msg, wParam, lParam );
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
        return WINPROC_CallWndProc( proc->thunk.t_from16.proc,
                                      hwnd, msg, wParam, lParam );
    default:
        WARN_(relay)("Invalid proc %p\n", proc );
        return 0;
    }
}
