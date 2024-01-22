/*
 * 16-bit messaging support
 *
 * Copyright 2001 Alexandre Julliard
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

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/winuser16.h"
#include "wownt32.h"
#include "winerror.h"
#include "dde.h"
#include "user_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msg);
WINE_DECLARE_DEBUG_CHANNEL(message);

DWORD USER16_AlertableWait = 0;

struct wow_handlers32 wow_handlers32;

static LRESULT send_message_callback( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                      LRESULT *result, void *arg )
{
    *result = SendMessageA( hwnd, msg, wp, lp );
    return *result;
}

static LRESULT post_message_callback( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                      LRESULT *result, void *arg )
{
    *result = 0;
    return PostMessageA( hwnd, msg, wp, lp );
}

static LRESULT post_thread_message_callback( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                             LRESULT *result, void *arg )
{
    DWORD_PTR tid = (DWORD_PTR)arg;
    *result = 0;
    return PostThreadMessageA( tid, msg, wp, lp );
}

static LRESULT get_message_callback( HWND16 hwnd, UINT16 msg, WPARAM16 wp, LPARAM lp,
                                     LRESULT *result, void *arg )
{
    MSG16 *msg16 = arg;

    msg16->hwnd    = hwnd;
    msg16->message = msg;
    msg16->wParam  = wp;
    msg16->lParam  = lp;
    *result = 0;
    return 0;
}

static LRESULT defdlg_proc_callback( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                     LRESULT *result, void *arg )
{
    *result = DefDlgProcA( hwnd, msg, wp, lp );
    return *result;
}

static LRESULT defwnd_proc_callback( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                     LRESULT *result, void *arg )
{
    return *result = DefWindowProcA( hwnd, msg, wp, lp );
}

static LRESULT call_window_proc_callback( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                          LRESULT *result, void *arg )
{
    WNDPROC proc = arg;
    *result = CallWindowProcA( proc, hwnd, msg, wp, lp );
    return *result;
}


/**********************************************************************
 * Support for window procedure thunks
 */

#include "pshpack1.h"
typedef struct
{
    WORD        popl_eax;        /* popl  %eax (return address) */
    WORD        pushl_func;      /* pushl $proc */
    WNDPROC     proc;
    WORD        pushl_eax;       /* pushl %eax */
    BYTE        ljmp;            /* ljmp relay*/
    FARPROC16   relay;           /* __wine_call_wndproc */
} WINPROC_THUNK;
#include "poppack.h"

#define WINPROC_HANDLE (~0u >> 16)
#define MAX_WINPROCS32 4096
#define MAX_WINPROCS16 1024

static WNDPROC16 winproc16_array[MAX_WINPROCS16];
static unsigned int winproc16_used;

static WINPROC_THUNK *thunk_array;
static UINT thunk_selector;

/* return the window proc index for a given handle, or -1 for an invalid handle
 * indices 0 .. MAX_WINPROCS32-1 are for 32-bit procs,
 * indices MAX_WINPROCS32 .. MAX_WINPROCS32+MAX_WINPROCS16-1 for 16-bit procs */
static int winproc_to_index( WNDPROC16 handle )
{
    unsigned int index;

    if (HIWORD(handle) == thunk_selector)
    {
        index = LOWORD(handle) / sizeof(WINPROC_THUNK);
        /* check alignment */
        if (index * sizeof(WINPROC_THUNK) != LOWORD(handle)) return -1;
        /* check array limits */
        if (index >= MAX_WINPROCS32) return -1;
    }
    else
    {
        index = LOWORD(handle);
        if ((ULONG_PTR)handle >> 16 != WINPROC_HANDLE) return -1;
        /* check array limits */
        if (index >= winproc16_used + MAX_WINPROCS32) return -1;
    }
    return index;
}

/* allocate a 16-bit thunk for an existing window proc */
static WNDPROC16 alloc_win16_thunk( WNDPROC handle )
{
    static FARPROC16 relay;
    WINPROC_THUNK *thunk;
    UINT index = LOWORD( handle );

    if (index >= MAX_WINPROCS32)  /* already a 16-bit proc */
        return winproc16_array[index - MAX_WINPROCS32];

    if (!thunk_array)  /* allocate the array and its selector */
    {
        assert( MAX_WINPROCS16 * sizeof(WINPROC_THUNK) <= 0x10000 );

        if (!(thunk_selector = GlobalAlloc16( GMEM_FIXED | GMEM_ZEROINIT,
                                              MAX_WINPROCS16 * sizeof(WINPROC_THUNK) )))
            return NULL;
        FarSetOwner16( thunk_selector, 0 );
        PrestoChangoSelector16( thunk_selector, thunk_selector );
        thunk_array = GlobalLock16( thunk_selector );
        relay = GetProcAddress16( GetModuleHandle16("user"), "__wine_call_wndproc" );
    }

    thunk = &thunk_array[index];
    thunk->popl_eax     = 0x5866;   /* popl  %eax */
    thunk->pushl_func   = 0x6866;   /* pushl $proc */
    thunk->proc         = handle;
    thunk->pushl_eax    = 0x5066;   /* pushl %eax */
    thunk->ljmp         = 0xea;     /* ljmp   relay*/
    thunk->relay        = relay;
    return (WNDPROC16)MAKESEGPTR( thunk_selector, index * sizeof(WINPROC_THUNK) );
}

/**********************************************************************
 *	     WINPROC_AllocProc16
 */
WNDPROC WINPROC_AllocProc16( WNDPROC16 func )
{
    int index;
    WNDPROC ret;

    if (!func) return NULL;

    /* check if the function is already a win proc */
    if ((index = winproc_to_index( func )) != -1)
        return (WNDPROC)(ULONG_PTR)(index | (WINPROC_HANDLE << 16));

    /* then check if we already have a winproc for that function */
    for (index = 0; index < winproc16_used; index++)
        if (winproc16_array[index] == func) goto done;

    if (winproc16_used >= MAX_WINPROCS16)
    {
        FIXME( "too many winprocs, cannot allocate one for 16-bit %p\n", func );
        return NULL;
    }
    winproc16_array[winproc16_used++] = func;

done:
    ret = (WNDPROC)(ULONG_PTR)((index + MAX_WINPROCS32) | (WINPROC_HANDLE << 16));
    TRACE( "returning %p for %p/16-bit (%d/%d used)\n",
           ret, func, winproc16_used, MAX_WINPROCS16 );
    return ret;
}

/**********************************************************************
 *	     WINPROC_GetProc16
 *
 * Get a window procedure pointer that can be passed to the Windows program.
 */
WNDPROC16 WINPROC_GetProc16( WNDPROC proc, BOOL unicode )
{
    WNDPROC winproc = wow_handlers32.alloc_winproc( proc, unicode );

    if ((ULONG_PTR)winproc >> 16 != WINPROC_HANDLE) return (WNDPROC16)winproc;
    return alloc_win16_thunk( winproc );
}

/* call a 16-bit window procedure */
static LRESULT call_window_proc16( HWND16 hwnd, UINT16 msg, WPARAM16 wParam, LPARAM lParam,
                                   LRESULT *result, void *arg )
{
    DPI_AWARENESS_CONTEXT awareness;
    WNDPROC16 func = arg;
    int index = winproc_to_index( func );
    CONTEXT context;
    size_t size = 0;
    struct
    {
        WORD params[5];
        union
        {
            CREATESTRUCT16 cs16;
            DRAWITEMSTRUCT16 dis16;
            COMPAREITEMSTRUCT16 cis16;
        } u;
    } args;

    if (index >= MAX_WINPROCS32) func = winproc16_array[index - MAX_WINPROCS32];

    /* Window procedures want ax = hInstance, ds = es = ss */

    memset(&context, 0, sizeof(context));
    context.SegDs = context.SegEs = CURRENT_SS;
    if (!(context.Eax = (WORD)GetWindowLongA( HWND_32(hwnd), GWLP_HINSTANCE ))) context.Eax = context.SegDs;
    context.SegCs = SELECTOROF(func);
    context.Eip   = OFFSETOF(func);
    context.Ebp   = CURRENT_SP + FIELD_OFFSET(STACK16FRAME, bp);

    if (lParam)
    {
        /* Some programs (eg. the "Undocumented Windows" examples, JWP) only
           work if structures passed in lParam are placed in the stack/data
           segment. Programmers easily make the mistake of converting lParam
           to a near rather than a far pointer, since Windows apparently
           allows this. We copy the structures to the 16 bit stack; this is
           ugly but makes these programs work. */
        switch (msg)
        {
          case WM_CREATE:
          case WM_NCCREATE:
            size = sizeof(CREATESTRUCT16); break;
          case WM_DRAWITEM:
            size = sizeof(DRAWITEMSTRUCT16); break;
          case WM_COMPAREITEM:
            size = sizeof(COMPAREITEMSTRUCT16); break;
        }
        if (size)
        {
            memcpy( &args.u, MapSL(lParam), size );
            lParam = MAKESEGPTR( CURRENT_SS, CURRENT_SP - size );
        }
    }

    awareness = SetThreadDpiAwarenessContext( GetWindowDpiAwarenessContext( HWND_32(hwnd) ));
    args.params[4] = hwnd;
    args.params[3] = msg;
    args.params[2] = wParam;
    args.params[1] = HIWORD(lParam);
    args.params[0] = LOWORD(lParam);
    WOWCallback16Ex( 0, WCB16_REGS, sizeof(args.params) + size, &args, (DWORD *)&context );
    *result = MAKELONG( LOWORD(context.Eax), LOWORD(context.Edx) );
    SetThreadDpiAwarenessContext( awareness );
    return *result;
}

static LRESULT call_dialog_proc16( HWND16 hwnd, UINT16 msg, WPARAM16 wp, LPARAM lp,
                                   LRESULT *result, void *arg )
{
    LRESULT ret = call_window_proc16( hwnd, msg, wp, lp, result, arg );
    *result = GetWindowLongPtrW( WIN_Handle32(hwnd), DWLP_MSGRESULT );
    return LOWORD(ret);
}

static LRESULT call_window_proc_Ato16( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                       LRESULT *result, void *arg )
{
    return WINPROC_CallProc32ATo16( call_window_proc16, hwnd, msg, wp, lp, result, arg );
}

static LRESULT call_dialog_proc_Ato16( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                       LRESULT *result, void *arg )
{
    return WINPROC_CallProc32ATo16( call_dialog_proc16, hwnd, msg, wp, lp, result, arg );
}



/**********************************************************************
 * Support for Edit word break proc thunks
 */

#define MAX_THUNKS 32

#include <pshpack1.h>
static struct word_break_thunk
{
    BYTE                popl_eax;       /* popl  %eax (return address) */
    BYTE                pushl_proc16;   /* pushl proc16 */
    EDITWORDBREAKPROC16 proc16;
    BYTE                pushl_eax;      /* pushl %eax */
    BYTE                jmp;            /* ljmp call_word_break_proc16 */
    DWORD               callback;
} *word_break_thunks;
#include <poppack.h>

/**********************************************************************
 *           call_word_break_proc16
 */
static INT16 CALLBACK call_word_break_proc16( SEGPTR proc16, LPSTR text, INT index, INT count, INT action )
{
    SEGPTR segptr;
    WORD args[5];
    DWORD result;

    segptr = MapLS( text );
    args[4] = SELECTOROF(segptr);
    args[3] = OFFSETOF(segptr);
    args[2] = index;
    args[1] = count;
    args[0] = action;
    WOWCallback16Ex( proc16, WCB16_PASCAL, sizeof(args), args, &result );
    UnMapLS( segptr );
    return LOWORD(result);
}

/******************************************************************
 *		add_word_break_thunk
 */
static struct word_break_thunk *add_word_break_thunk( EDITWORDBREAKPROC16 proc16 )
{
    struct word_break_thunk *thunk;

    if (!word_break_thunks)
    {
        word_break_thunks = VirtualAlloc( NULL, MAX_THUNKS * sizeof(*thunk),
                                          MEM_COMMIT, PAGE_EXECUTE_READWRITE );
        if (!word_break_thunks) return NULL;

        for (thunk = word_break_thunks; thunk < &word_break_thunks[MAX_THUNKS]; thunk++)
        {
            thunk->popl_eax     = 0x58;   /* popl  %eax */
            thunk->pushl_proc16 = 0x68;   /* pushl proc16 */
            thunk->pushl_eax    = 0x50;   /* pushl %eax */
            thunk->jmp          = 0xe9;   /* jmp call_word_break_proc16 */
            thunk->callback     = (char *)call_word_break_proc16 - (char *)(&thunk->callback + 1);
        }
    }
    for (thunk = word_break_thunks; thunk < &word_break_thunks[MAX_THUNKS]; thunk++)
        if (thunk->proc16 == proc16) return thunk;

    for (thunk = word_break_thunks; thunk < &word_break_thunks[MAX_THUNKS]; thunk++)
    {
        if (thunk->proc16) continue;
        thunk->proc16 = proc16;
        return thunk;
    }
    FIXME("Out of word break thunks\n");
    return NULL;
}

/******************************************************************
 *		get_word_break_thunk
 */
static EDITWORDBREAKPROC16 get_word_break_thunk( EDITWORDBREAKPROCA proc )
{
    struct word_break_thunk *thunk = (struct word_break_thunk *)proc;
    if (word_break_thunks && thunk >= word_break_thunks && thunk < &word_break_thunks[MAX_THUNKS])
        return thunk->proc16;
    return NULL;
}


/***********************************************************************
 * Support for 16<->32 message mapping
 */

static inline void *get_buffer( void *static_buffer, size_t size, size_t need )
{
    if (size >= need) return static_buffer;
    return HeapAlloc( GetProcessHeap(), 0, need );
}

static inline void free_buffer( void *static_buffer, void *buffer )
{
    if (buffer != static_buffer) HeapFree( GetProcessHeap(), 0, buffer );
}

static void RECT16to32( const RECT16 *from, RECT *to )
{
    to->left   = from->left;
    to->top    = from->top;
    to->right  = from->right;
    to->bottom = from->bottom;
}

static void RECT32to16( const RECT *from, RECT16 *to )
{
    to->left   = from->left;
    to->top    = from->top;
    to->right  = from->right;
    to->bottom = from->bottom;
}

static void MINMAXINFO32to16( const MINMAXINFO *from, MINMAXINFO16 *to )
{
    to->ptReserved.x     = from->ptReserved.x;
    to->ptReserved.y     = from->ptReserved.y;
    to->ptMaxSize.x      = from->ptMaxSize.x;
    to->ptMaxSize.y      = from->ptMaxSize.y;
    to->ptMaxPosition.x  = from->ptMaxPosition.x;
    to->ptMaxPosition.y  = from->ptMaxPosition.y;
    to->ptMinTrackSize.x = from->ptMinTrackSize.x;
    to->ptMinTrackSize.y = from->ptMinTrackSize.y;
    to->ptMaxTrackSize.x = from->ptMaxTrackSize.x;
    to->ptMaxTrackSize.y = from->ptMaxTrackSize.y;
}

static void MINMAXINFO16to32( const MINMAXINFO16 *from, MINMAXINFO *to )
{
    to->ptReserved.x     = from->ptReserved.x;
    to->ptReserved.y     = from->ptReserved.y;
    to->ptMaxSize.x      = from->ptMaxSize.x;
    to->ptMaxSize.y      = from->ptMaxSize.y;
    to->ptMaxPosition.x  = from->ptMaxPosition.x;
    to->ptMaxPosition.y  = from->ptMaxPosition.y;
    to->ptMinTrackSize.x = from->ptMinTrackSize.x;
    to->ptMinTrackSize.y = from->ptMinTrackSize.y;
    to->ptMaxTrackSize.x = from->ptMaxTrackSize.x;
    to->ptMaxTrackSize.y = from->ptMaxTrackSize.y;
}

static void WINDOWPOS32to16( const WINDOWPOS* from, WINDOWPOS16* to )
{
    to->hwnd            = HWND_16(from->hwnd);
    to->hwndInsertAfter = HWND_16(from->hwndInsertAfter);
    to->x               = from->x;
    to->y               = from->y;
    to->cx              = from->cx;
    to->cy              = from->cy;
    to->flags           = from->flags;
}

static void WINDOWPOS16to32( const WINDOWPOS16* from, WINDOWPOS* to )
{
    to->hwnd            = WIN_Handle32(from->hwnd);
    to->hwndInsertAfter = (from->hwndInsertAfter == (HWND16)-1) ?
                           HWND_TOPMOST : WIN_Handle32(from->hwndInsertAfter);
    to->x               = from->x;
    to->y               = from->y;
    to->cx              = from->cx;
    to->cy              = from->cy;
    to->flags           = from->flags;
}

/* The strings are not copied */
static void CREATESTRUCT32Ato16( const CREATESTRUCTA* from, CREATESTRUCT16* to )
{
    to->lpCreateParams = (SEGPTR)from->lpCreateParams;
    to->hInstance      = HINSTANCE_16(from->hInstance);
    to->hMenu          = HMENU_16(from->hMenu);
    to->hwndParent     = HWND_16(from->hwndParent);
    to->cy             = from->cy;
    to->cx             = from->cx;
    to->y              = from->y;
    to->x              = from->x;
    to->style          = from->style;
    to->dwExStyle      = from->dwExStyle;
}

static void CREATESTRUCT16to32A( const CREATESTRUCT16* from, CREATESTRUCTA *to )

{
    to->lpCreateParams = (LPVOID)from->lpCreateParams;
    to->hInstance      = HINSTANCE_32(from->hInstance);
    to->hMenu          = HMENU_32(from->hMenu);
    to->hwndParent     = WIN_Handle32(from->hwndParent);
    to->cy             = from->cy;
    to->cx             = from->cx;
    to->y              = from->y;
    to->x              = from->x;
    to->style          = from->style;
    to->dwExStyle      = from->dwExStyle;
    to->lpszName       = MapSL(from->lpszName);
    to->lpszClass      = MapSL(from->lpszClass);
}

/* The strings are not copied */
static void MDICREATESTRUCT32Ato16( const MDICREATESTRUCTA* from, MDICREATESTRUCT16* to )
{
    to->hOwner = HINSTANCE_16(from->hOwner);
    to->x      = from->x;
    to->y      = from->y;
    to->cx     = from->cx;
    to->cy     = from->cy;
    to->style  = from->style;
    to->lParam = from->lParam;
}

static void MDICREATESTRUCT16to32A( const MDICREATESTRUCT16* from, MDICREATESTRUCTA *to )
{
    to->hOwner = HINSTANCE_32(from->hOwner);
    to->x      = from->x;
    to->y      = from->y;
    to->cx     = from->cx;
    to->cy     = from->cy;
    to->style  = from->style;
    to->lParam = from->lParam;
    to->szTitle = MapSL(from->szTitle);
    to->szClass = MapSL(from->szClass);
}

static UINT_PTR convert_handle_16_to_32(HANDLE16 src, unsigned int flags)
{
    HANDLE      dst;
    UINT        sz = GlobalSize16(src);
    LPSTR       ptr16, ptr32;

    if (!(dst = GlobalAlloc(flags, sz)))
        return 0;
    ptr16 = GlobalLock16(src);
    ptr32 = GlobalLock(dst);
    if (ptr16 != NULL && ptr32 != NULL) memcpy(ptr32, ptr16, sz);
    GlobalUnlock16(src);
    GlobalUnlock(dst);

    return (UINT_PTR)dst;
}

static HANDLE16 convert_handle_32_to_16(UINT_PTR src, unsigned int flags)
{
    HANDLE16    dst;
    UINT        sz = GlobalSize((HANDLE)src);
    LPSTR       ptr16, ptr32;

    if (!(dst = GlobalAlloc16(flags, sz)))
        return 0;
    ptr32 = GlobalLock((HANDLE)src);
    ptr16 = GlobalLock16(dst);
    if (ptr16 != NULL && ptr32 != NULL) memcpy(ptr16, ptr32, sz);
    GlobalUnlock((HANDLE)src);
    GlobalUnlock16(dst);

    return dst;
}

static BOOL is_old_app( HWND hwnd )
{
    HINSTANCE inst = (HINSTANCE)GetWindowLongPtrW( hwnd, GWLP_HINSTANCE );
    return inst && !((ULONG_PTR)inst >> 16) && (GetExpWinVer16(LOWORD(inst)) & 0xFF00) == 0x0300;
}

static int find_sub_menu( HMENU *hmenu, HMENU16 target )
{
    int i, pos, count = GetMenuItemCount( *hmenu );

    for (i = 0; i < count; i++)
    {
        HMENU sub = GetSubMenu( *hmenu, i );
        if (!sub) continue;
        if (HMENU_16(sub) == target) return i;
        if ((pos = find_sub_menu( &sub, target )) != -1)
        {
            *hmenu = sub;
            return pos;
        }
    }
    return -1;
}

/**********************************************************************
 *	     WINPROC_CallProc16To32A
 */
LRESULT WINPROC_CallProc16To32A( winproc_callback_t callback, HWND16 hwnd, UINT16 msg,
                                 WPARAM16 wParam, LPARAM lParam, LRESULT *result, void *arg )
{
    LRESULT ret = 0;
    HWND hwnd32 = WIN_Handle32( hwnd );

    switch(msg)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT16 *cs16 = MapSL(lParam);
            CREATESTRUCTA cs;
            MDICREATESTRUCTA mdi_cs;
            SEGPTR mdi_cs_segptr = 0;

            CREATESTRUCT16to32A( cs16, &cs );
            if (GetWindowLongW(hwnd32, GWL_EXSTYLE) & WS_EX_MDICHILD)
            {
                MDICREATESTRUCT16 *mdi_cs16 = MapSL(cs16->lpCreateParams);
                MDICREATESTRUCT16to32A(mdi_cs16, &mdi_cs);
                cs.lpCreateParams = &mdi_cs;
                mdi_cs_segptr = cs16->lpCreateParams;
            }
            ret = callback( hwnd32, msg, wParam, (LPARAM)&cs, result, arg );
            CREATESTRUCT32Ato16( &cs, cs16 );
            if (mdi_cs_segptr)
            {
                MDICREATESTRUCT32Ato16( &mdi_cs, MapSL( mdi_cs_segptr ) );
                cs16->lpCreateParams = mdi_cs_segptr;
            }
        }
        break;
    case WM_MDICREATE:
        {
            MDICREATESTRUCT16 *cs16 = MapSL(lParam);
            MDICREATESTRUCTA cs;

            MDICREATESTRUCT16to32A( cs16, &cs );
            ret = callback( hwnd32, msg, wParam, (LPARAM)&cs, result, arg );
            MDICREATESTRUCT32Ato16( &cs, cs16 );
        }
        break;
    case WM_MDIACTIVATE:
        if (lParam)
            ret = callback( hwnd32, msg, (WPARAM)WIN_Handle32( HIWORD(lParam) ),
                            (LPARAM)WIN_Handle32( LOWORD(lParam) ), result, arg );
        else /* message sent to MDI client */
            ret = callback( hwnd32, msg, wParam, lParam, result, arg );
        break;
    case WM_MDIGETACTIVE:
        {
            BOOL maximized = FALSE;
            ret = callback( hwnd32, msg, wParam, (LPARAM)&maximized, result, arg );
            *result = MAKELRESULT( LOWORD(*result), maximized );
        }
        break;
    case WM_MDISETMENU:
        ret = callback( hwnd32, wParam ? WM_MDIREFRESHMENU : WM_MDISETMENU,
                        (WPARAM)HMENU_32(LOWORD(lParam)), (LPARAM)HMENU_32(HIWORD(lParam)),
                        result, arg );
        break;
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO16 *mmi16 = MapSL(lParam);
            MINMAXINFO mmi;

            MINMAXINFO16to32( mmi16, &mmi );
            ret = callback( hwnd32, msg, wParam, (LPARAM)&mmi, result, arg );
            MINMAXINFO32to16( &mmi, mmi16 );
        }
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS16 *winpos16 = MapSL(lParam);
            WINDOWPOS winpos;

            WINDOWPOS16to32( winpos16, &winpos );
            ret = callback( hwnd32, msg, wParam, (LPARAM)&winpos, result, arg );
            WINDOWPOS32to16( &winpos, winpos16 );
        }
        break;
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS16 *nc16 = MapSL(lParam);
            NCCALCSIZE_PARAMS nc;
            WINDOWPOS winpos;

            RECT16to32( &nc16->rgrc[0], &nc.rgrc[0] );
            if (wParam)
            {
                RECT16to32( &nc16->rgrc[1], &nc.rgrc[1] );
                RECT16to32( &nc16->rgrc[2], &nc.rgrc[2] );
                WINDOWPOS16to32( MapSL(nc16->lppos), &winpos );
                nc.lppos = &winpos;
            }
            ret = callback( hwnd32, msg, wParam, (LPARAM)&nc, result, arg );
            RECT32to16( &nc.rgrc[0], &nc16->rgrc[0] );
            if (wParam)
            {
                RECT32to16( &nc.rgrc[1], &nc16->rgrc[1] );
                RECT32to16( &nc.rgrc[2], &nc16->rgrc[2] );
                WINDOWPOS32to16( &winpos, MapSL(nc16->lppos) );
            }
        }
        break;
    case WM_COMPAREITEM:
        {
            COMPAREITEMSTRUCT16* cis16 = MapSL(lParam);
            COMPAREITEMSTRUCT cis;
            cis.CtlType    = cis16->CtlType;
            cis.CtlID      = cis16->CtlID;
            cis.hwndItem   = WIN_Handle32( cis16->hwndItem );
            cis.itemID1    = cis16->itemID1;
            cis.itemData1  = cis16->itemData1;
            cis.itemID2    = cis16->itemID2;
            cis.itemData2  = cis16->itemData2;
            cis.dwLocaleId = 0;  /* FIXME */
            ret = callback( hwnd32, msg, wParam, (LPARAM)&cis, result, arg );
        }
        break;
    case WM_DELETEITEM:
        {
            DELETEITEMSTRUCT16* dis16 = MapSL(lParam);
            DELETEITEMSTRUCT dis;
            dis.CtlType  = dis16->CtlType;
            dis.CtlID    = dis16->CtlID;
            dis.hwndItem = WIN_Handle32( dis16->hwndItem );
            dis.itemData = dis16->itemData;
            ret = callback( hwnd32, msg, wParam, (LPARAM)&dis, result, arg );
        }
        break;
    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT16* mis16 = MapSL(lParam);
            MEASUREITEMSTRUCT mis;
            mis.CtlType    = mis16->CtlType;
            mis.CtlID      = mis16->CtlID;
            mis.itemID     = mis16->itemID;
            mis.itemWidth  = mis16->itemWidth;
            mis.itemHeight = mis16->itemHeight;
            mis.itemData   = mis16->itemData;
            ret = callback( hwnd32, msg, wParam, (LPARAM)&mis, result, arg );
            mis16->itemWidth  = (UINT16)mis.itemWidth;
            mis16->itemHeight = (UINT16)mis.itemHeight;
        }
        break;
    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT16* dis16 = MapSL(lParam);
            DRAWITEMSTRUCT dis;
            dis.CtlType       = dis16->CtlType;
            dis.CtlID         = dis16->CtlID;
            dis.itemID        = dis16->itemID;
            dis.itemAction    = dis16->itemAction;
            dis.itemState     = dis16->itemState;
            dis.hwndItem      = (dis.CtlType == ODT_MENU) ? (HWND)HMENU_32(dis16->hwndItem)
                                                          : WIN_Handle32( dis16->hwndItem );
            dis.hDC           = HDC_32(dis16->hDC);
            dis.itemData      = dis16->itemData;
            dis.rcItem.left   = dis16->rcItem.left;
            dis.rcItem.top    = dis16->rcItem.top;
            dis.rcItem.right  = dis16->rcItem.right;
            dis.rcItem.bottom = dis16->rcItem.bottom;
            ret = callback( hwnd32, msg, wParam, (LPARAM)&dis, result, arg );
        }
        break;
    case WM_COPYDATA:
        {
            COPYDATASTRUCT16 *cds16 = MapSL(lParam);
            COPYDATASTRUCT cds;
            cds.dwData = cds16->dwData;
            cds.cbData = cds16->cbData;
            cds.lpData = MapSL(cds16->lpData);
            ret = callback( hwnd32, msg, wParam, (LPARAM)&cds, result, arg );
        }
        break;
    case WM_GETDLGCODE:
        if (lParam)
        {
            MSG16 *msg16 = MapSL(lParam);
            MSG msg32;
            msg32.hwnd    = WIN_Handle32( msg16->hwnd );
            msg32.message = msg16->message;
            msg32.wParam  = msg16->wParam;
            msg32.lParam  = msg16->lParam;
            msg32.time    = msg16->time;
            msg32.pt.x    = msg16->pt.x;
            msg32.pt.y    = msg16->pt.y;
            ret = callback( hwnd32, msg, wParam, (LPARAM)&msg32, result, arg );
        }
        else
            ret = callback( hwnd32, msg, wParam, lParam, result, arg );
        break;
    case WM_NEXTMENU:
        {
            MDINEXTMENU next;
            next.hmenuIn   = (HMENU)lParam;
            next.hmenuNext = 0;
            next.hwndNext  = 0;
            ret = callback( hwnd32, msg, wParam, (LPARAM)&next, result, arg );
            *result = MAKELONG( HMENU_16(next.hmenuNext), HWND_16(next.hwndNext) );
        }
        break;
    case WM_ACTIVATE:
    case WM_CHARTOITEM:
    case WM_COMMAND:
    case WM_VKEYTOITEM:
        ret = callback( hwnd32, msg, MAKEWPARAM( wParam, HIWORD(lParam) ),
                        (LPARAM)WIN_Handle32( LOWORD(lParam) ), result, arg );
        break;
    case WM_HSCROLL:
    case WM_VSCROLL:
        ret = callback( hwnd32, msg, MAKEWPARAM( wParam, LOWORD(lParam) ),
                        (LPARAM)WIN_Handle32( HIWORD(lParam) ), result, arg );
        break;
    case WM_CTLCOLOR:
        if (HIWORD(lParam) <= CTLCOLOR_STATIC)
            ret = callback( hwnd32, WM_CTLCOLORMSGBOX + HIWORD(lParam),
                            (WPARAM)HDC_32(wParam), (LPARAM)WIN_Handle32( LOWORD(lParam) ),
                            result, arg );
        break;
    case WM_GETTEXT:
    case WM_SETTEXT:
    case WM_WININICHANGE:
    case WM_DEVMODECHANGE:
    case WM_ASKCBFORMATNAME:
    case WM_NOTIFY:
        ret = callback( hwnd32, msg, wParam, (LPARAM)MapSL(lParam), result, arg );
        break;
    case WM_MENUCHAR:
        ret = callback( hwnd32, msg, MAKEWPARAM( wParam, LOWORD(lParam) ),
                        (LPARAM)HMENU_32(HIWORD(lParam)), result, arg );
        break;
    case WM_MENUSELECT:
        if((LOWORD(lParam) & MF_POPUP) && (LOWORD(lParam) != 0xFFFF))
        {
            HMENU hmenu = HMENU_32(HIWORD(lParam));
            int pos = find_sub_menu( &hmenu, wParam );
            if (pos == -1) pos = 0;
            wParam = pos;
        }
        ret = callback( hwnd32, msg, MAKEWPARAM( wParam, LOWORD(lParam) ),
                        (LPARAM)HMENU_32(HIWORD(lParam)), result, arg );
        break;
    case WM_PARENTNOTIFY:
        if ((wParam == WM_CREATE) || (wParam == WM_DESTROY))
            ret = callback( hwnd32, msg, MAKEWPARAM( wParam, HIWORD(lParam) ),
                            (LPARAM)WIN_Handle32( LOWORD(lParam) ), result, arg );
        else
            ret = callback( hwnd32, msg, wParam, lParam, result, arg );
        break;
    case WM_ACTIVATEAPP:
        /* We need this when SetActiveWindow sends a Sendmessage16() to
         * a 32-bit window. Might be superfluous with 32-bit interprocess
         * message queues. */
        if (lParam) lParam = HTASK_32(lParam);
        ret = callback( hwnd32, msg, wParam, lParam, result, arg );
        break;
    case WM_DDE_INITIATE:
    case WM_DDE_TERMINATE:
    case WM_DDE_UNADVISE:
    case WM_DDE_REQUEST:
        ret = callback( hwnd32, msg, (WPARAM)WIN_Handle32(wParam), lParam, result, arg );
        break;
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
        {
            HANDLE16 lo16 = LOWORD(lParam);
            UINT_PTR lo32 = 0;
            if (lo16 && !(lo32 = convert_handle_16_to_32(lo16, GMEM_DDESHARE))) break;
            lParam = PackDDElParam( msg, lo32, HIWORD(lParam) );
            ret = callback( hwnd32, msg, (WPARAM)WIN_Handle32(wParam), lParam, result, arg );
        }
        break; /* FIXME don't know how to free allocated memory (handle)  !! */
    case WM_DDE_ACK:
        {
            UINT_PTR lo = LOWORD(lParam);
            UINT_PTR hi = HIWORD(lParam);
            int flag = 0;
            char buf[2];

            if (GlobalGetAtomNameA(hi, buf, 2) > 0) flag |= 1;
            if (GlobalSize16(hi) != 0) flag |= 2;
            switch (flag)
            {
            case 0:
                if (hi)
                {
                    WARN("DDE_ACK: neither atom nor handle!!!\n");
                    hi = 0;
                }
                break;
            case 1:
                break; /* atom, nothing to do */
            case 3:
                WARN("DDE_ACK: %Ix both atom and handle... choosing handle\n", hi);
                /* fall through */
            case 2:
                hi = convert_handle_16_to_32(hi, GMEM_DDESHARE);
                break;
            }
            lParam = PackDDElParam( WM_DDE_ACK, lo, hi );
            ret = callback( hwnd32, msg, (WPARAM)WIN_Handle32(wParam), lParam, result, arg );
        }
        break; /* FIXME don't know how to free allocated memory (handle) !! */
    case WM_DDE_EXECUTE:
        lParam = convert_handle_16_to_32( HIWORD(lParam), GMEM_DDESHARE );
        ret = callback( hwnd32, msg, wParam, lParam, result, arg );
        break; /* FIXME don't know how to free allocated memory (handle) !! */
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
        FIXME_(msg)( "message %04x needs translation\n", msg );
        break;
    case WM_ERASEBKGND:
        ret = callback( hwnd32, msg, (WPARAM)HDC_32(wParam), lParam, result, arg );
        break;
    default:
        ret = callback( hwnd32, msg, wParam, lParam, result, arg );
        break;
    }
    return ret;
}


/**********************************************************************
 *	     WINPROC_CallProc32ATo16
 *
 * Call a 16-bit window procedure, translating the 32-bit args.
 */
LRESULT WINPROC_CallProc32ATo16( winproc_callback16_t callback, HWND hwnd, UINT msg,
                                 WPARAM wParam, LPARAM lParam, LRESULT *result, void *arg )
{
    LRESULT ret = 0;

    switch(msg)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCTA *cs32 = (CREATESTRUCTA *)lParam;
            CREATESTRUCT16 cs;
            MDICREATESTRUCT16 mdi_cs16;
            BOOL mdi_child = (GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_MDICHILD);

            CREATESTRUCT32Ato16( cs32, &cs );
            cs.lpszName  = MapLS( (void *)cs32->lpszName );
            cs.lpszClass = MapLS( (void *)cs32->lpszClass );

            if (mdi_child)
            {
                MDICREATESTRUCTA *mdi_cs = cs32->lpCreateParams;
                MDICREATESTRUCT32Ato16( mdi_cs, &mdi_cs16 );
                mdi_cs16.szTitle = MapLS( (void *)mdi_cs->szTitle );
                mdi_cs16.szClass = MapLS( (void *)mdi_cs->szClass );
                cs.lpCreateParams = MapLS( &mdi_cs16 );
            }
            lParam = MapLS( &cs );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
            UnMapLS( cs.lpszName );
            UnMapLS( cs.lpszClass );
            if (mdi_child)
            {
                UnMapLS( cs.lpCreateParams );
                UnMapLS( mdi_cs16.szTitle );
                UnMapLS( mdi_cs16.szClass );
            }
        }
        break;
    case WM_MDICREATE:
        {
            MDICREATESTRUCTA *cs32 = (MDICREATESTRUCTA *)lParam;
            MDICREATESTRUCT16 cs;

            MDICREATESTRUCT32Ato16( cs32, &cs );
            cs.szTitle = MapLS( (void *)cs32->szTitle );
            cs.szClass = MapLS( (void *)cs32->szClass );
            lParam = MapLS( &cs );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
            UnMapLS( cs.szTitle );
            UnMapLS( cs.szClass );
        }
        break;
    case WM_MDIACTIVATE:
        if (GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_MDICHILD)
            ret = callback( HWND_16(hwnd), msg, ((HWND)lParam == hwnd),
                            MAKELPARAM( LOWORD(lParam), LOWORD(wParam) ), result, arg );
        else
            ret = callback( HWND_16(hwnd), msg, HWND_16( wParam ), 0, result, arg );
        break;
    case WM_MDIGETACTIVE:
        ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
        if (lParam) *(BOOL *)lParam = (BOOL16)HIWORD(*result);
        *result = (LRESULT)WIN_Handle32( LOWORD(*result) );
        break;
    case WM_MDISETMENU:
        ret = callback( HWND_16(hwnd), msg, (lParam == 0),
                        MAKELPARAM( LOWORD(wParam), LOWORD(lParam) ), result, arg );
        break;
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO *mmi32 = (MINMAXINFO *)lParam;
            MINMAXINFO16 mmi;

            MINMAXINFO32to16( mmi32, &mmi );
            lParam = MapLS( &mmi );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
            MINMAXINFO16to32( &mmi, mmi32 );
        }
        break;
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS *nc32 = (NCCALCSIZE_PARAMS *)lParam;
            NCCALCSIZE_PARAMS16 nc;
            WINDOWPOS16 winpos;

            RECT32to16( &nc32->rgrc[0], &nc.rgrc[0] );
            if (wParam)
            {
                RECT32to16( &nc32->rgrc[1], &nc.rgrc[1] );
                RECT32to16( &nc32->rgrc[2], &nc.rgrc[2] );
                WINDOWPOS32to16( nc32->lppos, &winpos );
                nc.lppos = MapLS( &winpos );
            }
            lParam = MapLS( &nc );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
            RECT16to32( &nc.rgrc[0], &nc32->rgrc[0] );
            if (wParam)
            {
                RECT16to32( &nc.rgrc[1], &nc32->rgrc[1] );
                RECT16to32( &nc.rgrc[2], &nc32->rgrc[2] );
                WINDOWPOS16to32( &winpos, nc32->lppos );
                UnMapLS( nc.lppos );
            }
        }
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS *winpos32 = (WINDOWPOS *)lParam;
            WINDOWPOS16 winpos;

            WINDOWPOS32to16( winpos32, &winpos );
            lParam = MapLS( &winpos );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
            WINDOWPOS16to32( &winpos, winpos32 );
        }
        break;
    case WM_COMPAREITEM:
        {
            COMPAREITEMSTRUCT *cis32 = (COMPAREITEMSTRUCT *)lParam;
            COMPAREITEMSTRUCT16 cis;
            cis.CtlType    = cis32->CtlType;
            cis.CtlID      = cis32->CtlID;
            cis.hwndItem   = HWND_16( cis32->hwndItem );
            cis.itemID1    = cis32->itemID1;
            cis.itemData1  = cis32->itemData1;
            cis.itemID2    = cis32->itemID2;
            cis.itemData2  = cis32->itemData2;
            lParam = MapLS( &cis );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
        }
        break;
    case WM_DELETEITEM:
        {
            DELETEITEMSTRUCT *dis32 = (DELETEITEMSTRUCT *)lParam;
            DELETEITEMSTRUCT16 dis;
            dis.CtlType  = dis32->CtlType;
            dis.CtlID    = dis32->CtlID;
            dis.itemID   = dis32->itemID;
            dis.hwndItem = (dis.CtlType == ODT_MENU) ? (HWND16)LOWORD(dis32->hwndItem)
                                                     : HWND_16( dis32->hwndItem );
            dis.itemData = dis32->itemData;
            lParam = MapLS( &dis );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
        }
        break;
    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT *dis32 = (DRAWITEMSTRUCT *)lParam;
            DRAWITEMSTRUCT16 dis;
            dis.CtlType       = dis32->CtlType;
            dis.CtlID         = dis32->CtlID;
            dis.itemID        = dis32->itemID;
            dis.itemAction    = dis32->itemAction;
            dis.itemState     = dis32->itemState;
            dis.hwndItem      = HWND_16( dis32->hwndItem );
            dis.hDC           = HDC_16(dis32->hDC);
            dis.itemData      = dis32->itemData;
            dis.rcItem.left   = dis32->rcItem.left;
            dis.rcItem.top    = dis32->rcItem.top;
            dis.rcItem.right  = dis32->rcItem.right;
            dis.rcItem.bottom = dis32->rcItem.bottom;
            lParam = MapLS( &dis );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
        }
        break;
    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT *mis32 = (MEASUREITEMSTRUCT *)lParam;
            MEASUREITEMSTRUCT16 mis;
            mis.CtlType    = mis32->CtlType;
            mis.CtlID      = mis32->CtlID;
            mis.itemID     = mis32->itemID;
            mis.itemWidth  = mis32->itemWidth;
            mis.itemHeight = mis32->itemHeight;
            mis.itemData   = mis32->itemData;
            lParam = MapLS( &mis );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
            mis32->itemWidth  = mis.itemWidth;
            mis32->itemHeight = mis.itemHeight;
        }
        break;
    case WM_COPYDATA:
        {
            COPYDATASTRUCT *cds32 = (COPYDATASTRUCT *)lParam;
            COPYDATASTRUCT16 cds;

            cds.dwData = cds32->dwData;
            cds.cbData = cds32->cbData;
            cds.lpData = MapLS( cds32->lpData );
            lParam = MapLS( &cds );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
            UnMapLS( cds.lpData );
        }
        break;
    case WM_GETDLGCODE:
        if (lParam)
        {
            MSG *msg32 = (MSG *)lParam;
            MSG16 msg16;

            msg16.hwnd    = HWND_16( msg32->hwnd );
            msg16.message = msg32->message;
            msg16.wParam  = msg32->wParam;
            msg16.lParam  = msg32->lParam;
            msg16.time    = msg32->time;
            msg16.pt.x    = msg32->pt.x;
            msg16.pt.y    = msg32->pt.y;
            lParam = MapLS( &msg16 );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
        }
        else
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
        break;
    case WM_NEXTMENU:
        {
            MDINEXTMENU *next = (MDINEXTMENU *)lParam;
            ret = callback( HWND_16(hwnd), msg, wParam, (LPARAM)next->hmenuIn, result, arg );
            next->hmenuNext = HMENU_32( LOWORD(*result) );
            next->hwndNext  = WIN_Handle32( HIWORD(*result) );
            *result = 0;
        }
        break;
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        wParam = min( wParam, 0xff80 ); /* Must be < 64K */
        /* fall through */
    case WM_NOTIFY:
    case WM_SETTEXT:
    case WM_WININICHANGE:
    case WM_DEVMODECHANGE:
        lParam = MapLS( (void *)lParam );
        ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
        UnMapLS( lParam );
        break;
    case WM_ACTIVATE:
    case WM_CHARTOITEM:
    case WM_COMMAND:
    case WM_VKEYTOITEM:
        ret = callback( HWND_16(hwnd), msg, wParam, MAKELPARAM( (HWND16)lParam, HIWORD(wParam) ),
                        result, arg );
        break;
    case WM_HSCROLL:
    case WM_VSCROLL:
        ret = callback( HWND_16(hwnd), msg, wParam, MAKELPARAM( HIWORD(wParam), (HWND16)lParam ),
                        result, arg );
        break;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        ret = callback( HWND_16(hwnd), WM_CTLCOLOR, wParam,
                        MAKELPARAM( (HWND16)lParam, msg - WM_CTLCOLORMSGBOX ), result, arg );
        break;
    case WM_MENUSELECT:
        if(HIWORD(wParam) & MF_POPUP)
        {
            HMENU hmenu;
            if ((HIWORD(wParam) != 0xffff) || lParam)
            {
                if ((hmenu = GetSubMenu( (HMENU)lParam, LOWORD(wParam) )))
                {
                    ret = callback( HWND_16(hwnd), msg, HMENU_16(hmenu),
                                    MAKELPARAM( HIWORD(wParam), (HMENU16)lParam ), result, arg );
                    break;
                }
            }
        }
        /* fall through */
    case WM_MENUCHAR:
        ret = callback( HWND_16(hwnd), msg, wParam,
                        MAKELPARAM( HIWORD(wParam), (HMENU16)lParam ), result, arg );
        break;
    case WM_PARENTNOTIFY:
        if ((LOWORD(wParam) == WM_CREATE) || (LOWORD(wParam) == WM_DESTROY))
            ret = callback( HWND_16(hwnd), msg, wParam,
                            MAKELPARAM( (HWND16)lParam, HIWORD(wParam) ), result, arg );
        else
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
        break;
    case WM_ACTIVATEAPP:
        ret = callback( HWND_16(hwnd), msg, wParam, HTASK_16( lParam ), result, arg );
        break;
    case WM_PAINT:
        if (IsIconic( hwnd ) && GetClassLongPtrW( hwnd, GCLP_HICON ))
            ret = callback( HWND_16(hwnd), WM_PAINTICON, 1, lParam, result, arg );
        else
            ret = callback( HWND_16(hwnd), WM_PAINT, wParam, lParam, result, arg );
        break;
    case WM_ERASEBKGND:
        if (IsIconic( hwnd ) && GetClassLongPtrW( hwnd, GCLP_HICON )) msg = WM_ICONERASEBKGND;
        ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
        break;
    case WM_DDE_INITIATE:
    case WM_DDE_TERMINATE:
    case WM_DDE_UNADVISE:
    case WM_DDE_REQUEST:
        ret = callback( HWND_16(hwnd), msg, HWND_16(wParam), lParam, result, arg );
        break;
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
        {
            UINT_PTR lo32, hi;
            HANDLE16 lo16 = 0;

            UnpackDDElParam( msg, lParam, &lo32, &hi );
            if (lo32 && !(lo16 = convert_handle_32_to_16(lo32, GMEM_DDESHARE))) break;
            ret = callback( HWND_16(hwnd), msg, HWND_16(wParam),
                            MAKELPARAM(lo16, hi), result, arg );
        }
        break; /* FIXME don't know how to free allocated memory (handle)  !! */
    case WM_DDE_ACK:
        {
            UINT_PTR lo, hi;
            int flag = 0;
            char buf[2];

            UnpackDDElParam( msg, lParam, &lo, &hi );

            if (GlobalGetAtomNameA((ATOM)hi, buf, sizeof(buf)) > 0) flag |= 1;
            if (GlobalSize((HANDLE)hi) != 0) flag |= 2;
            switch (flag)
            {
            case 0:
                if (hi)
                {
                    WARN("DDE_ACK: neither atom nor handle!!!\n");
                    hi = 0;
                }
                break;
            case 1:
                break; /* atom, nothing to do */
            case 3:
                WARN("DDE_ACK: %Ix both atom and handle... choosing handle\n", hi);
                /* fall through */
            case 2:
                hi = convert_handle_32_to_16(hi, GMEM_DDESHARE);
                break;
            }
            ret = callback( HWND_16(hwnd), msg, HWND_16(wParam),
                            MAKELPARAM(lo, hi), result, arg );
        }
        break; /* FIXME don't know how to free allocated memory (handle) !! */
    case WM_DDE_EXECUTE:
        lParam = MAKELPARAM( 0, convert_handle_32_to_16( lParam, GMEM_DDESHARE ));
        ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
        break; /* FIXME don't know how to free allocated memory (handle) !! */
    case WM_TIMER:
        if (wParam & SYSTEM_TIMER_FLAG)
            msg = WM_SYSTIMER;
        ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
        break;
    case SBM_SETRANGE:
        ret = callback( HWND_16(hwnd), SBM_SETRANGE16, 0, MAKELPARAM(wParam, lParam), result, arg );
        break;
    case SBM_GETRANGE:
        ret = callback( HWND_16(hwnd), SBM_GETRANGE16, wParam, lParam, result, arg );
        *(LPINT)wParam = LOWORD(*result);
        *(LPINT)lParam = HIWORD(*result);
        break;
    case BM_GETCHECK:
    case BM_SETCHECK:
    case BM_GETSTATE:
    case BM_SETSTATE:
    case BM_SETSTYLE:
        ret = callback( HWND_16(hwnd), msg + BM_GETCHECK16 - BM_GETCHECK, wParam, lParam, result, arg );
        break;
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
        ret = callback( HWND_16(hwnd), msg + EM_GETSEL16 - EM_GETSEL, wParam, lParam, result, arg );
        break;
    case EM_SETSEL:
        ret = callback( HWND_16(hwnd), EM_SETSEL16, 0, MAKELPARAM( wParam, lParam ), result, arg );
        break;
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
        ret = callback( HWND_16(hwnd), msg + LB_ADDSTRING16 - LB_ADDSTRING, wParam, lParam, result, arg );
        break;
    case LB_ADDSTRING:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_INSERTSTRING:
    case LB_SELECTSTRING:
    case LB_GETTEXT:
    case LB_DIR:
    case LB_ADDFILE:
        lParam = MapLS( (LPSTR)lParam );
        ret = callback( HWND_16(hwnd), msg + LB_ADDSTRING16 - LB_ADDSTRING, wParam, lParam, result, arg );
        UnMapLS( lParam );
        break;
    case LB_GETSELITEMS:
        {
            INT *items32 = (INT *)lParam;
            INT16 *items, buffer[512];
            unsigned int i;

            wParam = min( wParam, 0x7f80 ); /* Must be < 64K */
            if (!(items = get_buffer( buffer, sizeof(buffer), wParam * sizeof(INT16) ))) break;
            lParam = MapLS( items );
            ret = callback( HWND_16(hwnd), LB_GETSELITEMS16, wParam, lParam, result, arg );
            UnMapLS( lParam );
            for (i = 0; i < wParam; i++) items32[i] = items[i];
            free_buffer( buffer, items );
        }
        break;
    case LB_SETTABSTOPS:
        if (wParam)
        {
            INT *stops32 = (INT *)lParam;
            INT16 *stops, buffer[512];
            unsigned int i;

            wParam = min( wParam, 0x7f80 ); /* Must be < 64K */
            if (!(stops = get_buffer( buffer, sizeof(buffer), wParam * sizeof(INT16) ))) break;
            for (i = 0; i < wParam; i++) stops[i] = stops32[i];
            lParam = MapLS( stops );
            ret = callback( HWND_16(hwnd), LB_SETTABSTOPS16, wParam, lParam, result, arg );
            UnMapLS( lParam );
            free_buffer( buffer, stops );
        }
        else ret = callback( HWND_16(hwnd), LB_SETTABSTOPS16, wParam, lParam, result, arg );
        break;
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
        ret = callback( HWND_16(hwnd), msg + CB_GETEDITSEL16 - CB_GETEDITSEL, wParam, lParam, result, arg );
        break;
    case CB_GETEDITSEL:
        ret = callback( HWND_16(hwnd), CB_GETEDITSEL16, wParam, lParam, result, arg );
        if (wParam) *((PUINT)(wParam)) = LOWORD(*result);
        if (lParam) *((PUINT)(lParam)) = HIWORD(*result);  /* FIXME: subtract 1? */
        break;
    case CB_ADDSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_INSERTSTRING:
    case CB_SELECTSTRING:
    case CB_DIR:
    case CB_GETLBTEXT:
        lParam = MapLS( (LPSTR)lParam );
        ret = callback( HWND_16(hwnd), msg + CB_GETEDITSEL16 - CB_GETEDITSEL, wParam, lParam, result, arg );
        UnMapLS( lParam );
        break;
    case LB_GETITEMRECT:
    case CB_GETDROPPEDCONTROLRECT:
        {
            RECT *r32 = (RECT *)lParam;
            RECT16 rect;
            lParam = MapLS( &rect );
            ret = callback( HWND_16(hwnd),
                            (msg == LB_GETITEMRECT) ? LB_GETITEMRECT16 : CB_GETDROPPEDCONTROLRECT16,
                            wParam, lParam, result, arg );
            UnMapLS( lParam );
            RECT16to32( &rect, r32 );
        }
        break;
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
        FIXME_(msg)( "message %04x needs translation\n", msg );
        break;
    /* the following messages should not be sent to 16-bit apps */
    case WM_SIZING:
    case WM_MOVING:
    case WM_CAPTURECHANGED:
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
        break;
    default:
        ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
        break;
    }
    return ret;
}


/***********************************************************************
 *		SendMessage  (USER.111)
 */
LRESULT WINAPI SendMessage16( HWND16 hwnd16, UINT16 msg, WPARAM16 wparam, LPARAM lparam )
{
    LRESULT result;
    HWND hwnd = WIN_Handle32( hwnd16 );

    if (hwnd != HWND_BROADCAST &&
        GetWindowThreadProcessId( hwnd, NULL ) == GetCurrentThreadId())
    {
        /* call 16-bit window proc directly */
        WNDPROC16 winproc;

        /* first the WH_CALLWNDPROC hook */
        call_WH_CALLWNDPROC_hook( hwnd16, msg, wparam, lparam );

        if (!(winproc = (WNDPROC16)GetWindowLong16( hwnd16, GWLP_WNDPROC ))) return 0;

        TRACE_(message)("(0x%04x) [%04x] wp=%04x lp=%08Ix\n", hwnd16, msg, wparam, lparam );
        result = CallWindowProc16( winproc, hwnd16, msg, wparam, lparam );
        TRACE_(message)("(0x%04x) [%04x] wp=%04x lp=%08Ix returned %08Ix\n",
                        hwnd16, msg, wparam, lparam, result );
    }
    else  /* map to 32-bit unicode for inter-thread/process message */
    {
        WINPROC_CallProc16To32A( send_message_callback, hwnd16, msg, wparam, lparam, &result, NULL );
    }
    return result;
}


/***********************************************************************
 *		PostMessage  (USER.110)
 */
BOOL16 WINAPI PostMessage16( HWND16 hwnd, UINT16 msg, WPARAM16 wparam, LPARAM lparam )
{
    LRESULT unused;
    return WINPROC_CallProc16To32A( post_message_callback, hwnd, msg, wparam, lparam, &unused, NULL );
}


/***********************************************************************
 *		PostAppMessage (USER.116)
 */
BOOL16 WINAPI PostAppMessage16( HTASK16 hTask, UINT16 msg, WPARAM16 wparam, LPARAM lparam )
{
    LRESULT unused;
    DWORD_PTR tid = HTASK_32( hTask );

    if (!tid) return FALSE;
    return WINPROC_CallProc16To32A( post_thread_message_callback, 0, msg, wparam, lparam,
                                    &unused, (void *)tid );
}


/**********************************************************************
 *		CallWindowProc (USER.122)
 */
LRESULT WINAPI CallWindowProc16( WNDPROC16 func, HWND16 hwnd, UINT16 msg,
                                 WPARAM16 wParam, LPARAM lParam )
{
    int index = winproc_to_index( func );
    LRESULT result;

    if (!func) return 0;

    if (index == -1 || index >= MAX_WINPROCS32)
        call_window_proc16( hwnd, msg, wParam, lParam, &result, func );
    else
    {
        WNDPROC proc = (WNDPROC)func;
        if (thunk_array && thunk_array[index].proc) proc = thunk_array[index].proc;
        WINPROC_CallProc16To32A( call_window_proc_callback, hwnd, msg, wParam, lParam, &result, proc );
    }
    return result;
}


/**********************************************************************
 *	     __wine_call_wndproc   (USER.1010)
 */
LRESULT WINAPI __wine_call_wndproc( HWND16 hwnd, UINT16 msg, WPARAM16 wParam, LPARAM lParam, WNDPROC proc )
{
    LRESULT result;
    WINPROC_CallProc16To32A( call_window_proc_callback, hwnd, msg, wParam, lParam, &result, proc );
    return result;
}


/***********************************************************************
 *		InSendMessage  (USER.192)
 */
BOOL16 WINAPI InSendMessage16(void)
{
    return InSendMessage();
}


/***********************************************************************
 *		ReplyMessage  (USER.115)
 */
void WINAPI ReplyMessage16( LRESULT result )
{
    ReplyMessage( result );
}


/***********************************************************************
 *		PeekMessage32 (USER.819)
 */
BOOL16 WINAPI PeekMessage32_16( MSG32_16 *msg16, HWND16 hwnd16,
                                UINT16 first, UINT16 last, UINT16 flags,
                                BOOL16 wHaveParamHigh )
{
    MSG msg;
    LRESULT unused;
    HWND hwnd = WIN_Handle32( hwnd16 );

    if(USER16_AlertableWait)
        MsgWaitForMultipleObjectsEx( 0, NULL, 0, 0, MWMO_ALERTABLE );
    if (!PeekMessageA( &msg, hwnd, first, last, flags )) return FALSE;

    msg16->msg.time    = msg.time;
    msg16->msg.pt.x    = (INT16)msg.pt.x;
    msg16->msg.pt.y    = (INT16)msg.pt.y;
    if (wHaveParamHigh) msg16->wParamHigh = HIWORD(msg.wParam);
    WINPROC_CallProc32ATo16( get_message_callback, msg.hwnd, msg.message, msg.wParam, msg.lParam,
                             &unused, &msg16->msg );
    return TRUE;
}


/***********************************************************************
 *		DefWindowProc (USER.107)
 */
LRESULT WINAPI DefWindowProc16( HWND16 hwnd16, UINT16 msg, WPARAM16 wParam, LPARAM lParam )
{
    LRESULT result;
    WINPROC_CallProc16To32A( defwnd_proc_callback, hwnd16, msg, wParam, lParam, &result, 0 );
    return result;
}


/***********************************************************************
 *              DefDlgProc (USER.308)
 */
LRESULT WINAPI DefDlgProc16( HWND16 hwnd, UINT16 msg, WPARAM16 wParam, LPARAM lParam )
{
    LRESULT result;
    WINPROC_CallProc16To32A( defdlg_proc_callback, hwnd, msg, wParam, lParam, &result, 0 );
    return result;
}


/***********************************************************************
 *		PeekMessage  (USER.109)
 */
BOOL16 WINAPI PeekMessage16( MSG16 *msg, HWND16 hwnd,
                             UINT16 first, UINT16 last, UINT16 flags )
{
    return PeekMessage32_16( (MSG32_16 *)msg, hwnd, first, last, flags, FALSE );
}


/***********************************************************************
 *		GetMessage32  (USER.820)
 */
BOOL16 WINAPI GetMessage32_16( MSG32_16 *msg16, HWND16 hwnd16, UINT16 first,
                               UINT16 last, BOOL16 wHaveParamHigh )
{
    MSG msg;
    LRESULT unused;
    HWND hwnd = WIN_Handle32( hwnd16 );

    if(USER16_AlertableWait)
        MsgWaitForMultipleObjectsEx( 0, NULL, INFINITE, 0, MWMO_ALERTABLE );
    GetMessageA( &msg, hwnd, first, last );
    msg16->msg.time    = msg.time;
    msg16->msg.pt.x    = (INT16)msg.pt.x;
    msg16->msg.pt.y    = (INT16)msg.pt.y;
    if (wHaveParamHigh) msg16->wParamHigh = HIWORD(msg.wParam);
    WINPROC_CallProc32ATo16( get_message_callback, msg.hwnd, msg.message, msg.wParam, msg.lParam,
                             &unused, &msg16->msg );

    TRACE( "message %04x, hwnd %p, filter(%04x - %04x)\n",
           msg16->msg.message, hwnd, first, last );

    return msg16->msg.message != WM_QUIT;
}


/***********************************************************************
 *		GetMessage  (USER.108)
 */
BOOL16 WINAPI GetMessage16( MSG16 *msg, HWND16 hwnd, UINT16 first, UINT16 last )
{
    return GetMessage32_16( (MSG32_16 *)msg, hwnd, first, last, FALSE );
}


/***********************************************************************
 *		TranslateMessage32 (USER.821)
 */
BOOL16 WINAPI TranslateMessage32_16( const MSG32_16 *msg, BOOL16 wHaveParamHigh )
{
    MSG msg32;

    msg32.hwnd    = WIN_Handle32( msg->msg.hwnd );
    msg32.message = msg->msg.message;
    msg32.wParam  = MAKEWPARAM( msg->msg.wParam, wHaveParamHigh ? msg->wParamHigh : 0 );
    msg32.lParam  = msg->msg.lParam;
    return TranslateMessage( &msg32 );
}


/***********************************************************************
 *		TranslateMessage (USER.113)
 */
BOOL16 WINAPI TranslateMessage16( const MSG16 *msg )
{
    return TranslateMessage32_16( (const MSG32_16 *)msg, FALSE );
}


/***********************************************************************
 *		DispatchMessage (USER.114)
 */
LONG WINAPI DispatchMessage16( const MSG16* msg )
{
    WNDPROC16 winproc;
    LRESULT retval;

      /* Process timer messages */
    if ((msg->message == WM_TIMER) || (msg->message == WM_SYSTIMER))
    {
        if (msg->lParam)
            return CallWindowProc16( (WNDPROC16)msg->lParam, msg->hwnd,
                                     msg->message, msg->wParam, GetTickCount() );
    }

    if (!(winproc = (WNDPROC16)GetWindowLong16( msg->hwnd, GWLP_WNDPROC )))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    TRACE_(message)("(0x%04x) [%04x] wp=%04x lp=%08Ix\n", msg->hwnd, msg->message, msg->wParam, msg->lParam );
    retval = CallWindowProc16( winproc, msg->hwnd, msg->message, msg->wParam, msg->lParam );
    TRACE_(message)("(0x%04x) [%04x] wp=%04x lp=%08Ix returned %08Ix\n",
                    msg->hwnd, msg->message, msg->wParam, msg->lParam, retval );
    return retval;
}


/***********************************************************************
 *		DispatchMessage32 (USER.822)
 */
LONG WINAPI DispatchMessage32_16( const MSG32_16 *msg16, BOOL16 wHaveParamHigh )
{
    if (wHaveParamHigh == FALSE)
        return DispatchMessage16( &msg16->msg );
    else
    {
        MSG msg;

        msg.hwnd    = WIN_Handle32( msg16->msg.hwnd );
        msg.message = msg16->msg.message;
        msg.wParam  = MAKEWPARAM( msg16->msg.wParam, msg16->wParamHigh );
        msg.lParam  = msg16->msg.lParam;
        msg.time    = msg16->msg.time;
        msg.pt.x    = msg16->msg.pt.x;
        msg.pt.y    = msg16->msg.pt.y;
        return DispatchMessageA( &msg );
    }
}


/***********************************************************************
 *		IsDialogMessage (USER.90)
 */
BOOL16 WINAPI IsDialogMessage16( HWND16 hwndDlg, MSG16 *msg16 )
{
    MSG msg;
    HWND hwndDlg32;

    msg.hwnd  = WIN_Handle32(msg16->hwnd);
    hwndDlg32 = WIN_Handle32(hwndDlg);

    switch(msg16->message)
    {
    case WM_KEYDOWN:
    case WM_CHAR:
    case WM_SYSCHAR:
        msg.message = msg16->message;
        msg.wParam  = msg16->wParam;
        msg.lParam  = msg16->lParam;
        msg.time    = msg16->time;
        msg.pt.x    = msg16->pt.x;
        msg.pt.y    = msg16->pt.y;
        return IsDialogMessageA( hwndDlg32, &msg );
    }

    if ((hwndDlg32 != msg.hwnd) && !IsChild( hwndDlg32, msg.hwnd )) return FALSE;
    TranslateMessage16( msg16 );
    DispatchMessage16( msg16 );
    return TRUE;
}


/***********************************************************************
 *		MsgWaitForMultipleObjects  (USER.640)
 */
DWORD WINAPI MsgWaitForMultipleObjects16( DWORD count, const HANDLE *handles,
                                          BOOL wait_all, DWORD timeout, DWORD mask )
{
    return MsgWaitForMultipleObjectsEx( count, handles, timeout, mask,
                                        wait_all ? MWMO_WAITALL : 0 );
}


/**********************************************************************
 *		SetDoubleClickTime (USER.20)
 */
void WINAPI SetDoubleClickTime16( UINT16 interval )
{
    SetDoubleClickTime( interval );
}


/**********************************************************************
 *		GetDoubleClickTime (USER.21)
 */
UINT16 WINAPI GetDoubleClickTime16(void)
{
    return GetDoubleClickTime();
}


/***********************************************************************
 *		PostQuitMessage (USER.6)
 */
void WINAPI PostQuitMessage16( INT16 exitCode )
{
    PostQuitMessage( exitCode );
}


/**********************************************************************
 *		GetKeyState (USER.106)
 */
INT16 WINAPI GetKeyState16(INT16 vkey)
{
    return GetKeyState(vkey);
}


/**********************************************************************
 *		GetKeyboardState (USER.222)
 */
BOOL WINAPI GetKeyboardState16( LPBYTE state )
{
    return GetKeyboardState( state );
}


/**********************************************************************
 *		SetKeyboardState (USER.223)
 */
BOOL WINAPI SetKeyboardState16( LPBYTE state )
{
    return SetKeyboardState( state );
}


/***********************************************************************
 *		SetMessageQueue (USER.266)
 */
BOOL16 WINAPI SetMessageQueue16( INT16 size )
{
    return SetMessageQueue( size );
}


/***********************************************************************
 *		UserYield (USER.332)
 */
void WINAPI UserYield16(void)
{
    MSG msg;
    PeekMessageW( &msg, 0, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE );
}


/***********************************************************************
 *		GetQueueStatus (USER.334)
 */
DWORD WINAPI GetQueueStatus16( UINT16 flags )
{
    return GetQueueStatus( flags );
}


/***********************************************************************
 *		GetInputState (USER.335)
 */
BOOL16 WINAPI GetInputState16(void)
{
    return GetInputState();
}


/**********************************************************************
 *           TranslateAccelerator      (USER.178)
 */
INT16 WINAPI TranslateAccelerator16( HWND16 hwnd, HACCEL16 hAccel, LPMSG16 msg )
{
    MSG msg32;

    if (!msg) return 0;
    msg32.message = msg->message;
    /* msg32.hwnd not used */
    msg32.wParam  = msg->wParam;
    msg32.lParam  = msg->lParam;
    return TranslateAcceleratorW( WIN_Handle32(hwnd), HACCEL_32(hAccel), &msg32 );
}


/**********************************************************************
 *		TranslateMDISysAccel (USER.451)
 */
BOOL16 WINAPI TranslateMDISysAccel16( HWND16 hwndClient, LPMSG16 msg )
{
    if (msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN)
    {
        MSG msg32;
        msg32.hwnd    = WIN_Handle32(msg->hwnd);
        msg32.message = msg->message;
        msg32.wParam  = msg->wParam;
        msg32.lParam  = msg->lParam;
        /* MDICLIENTINFO is still the same for win32 and win16 ... */
        return TranslateMDISysAccel( WIN_Handle32(hwndClient), &msg32 );
    }
    return 0;
}


/***********************************************************************
 *           button_proc16
 */
static LRESULT button_proc16( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, BOOL unicode )
{
    static const UINT msg16_offset = BM_GETCHECK16 - BM_GETCHECK;

    switch (msg)
    {
    case BM_GETCHECK16:
    case BM_SETCHECK16:
    case BM_GETSTATE16:
    case BM_SETSTATE16:
    case BM_SETSTYLE16:
        return wow_handlers32.button_proc( hwnd, msg - msg16_offset, wParam, lParam, FALSE );
    default:
        return wow_handlers32.button_proc( hwnd, msg, wParam, lParam, unicode );
    }
}


/***********************************************************************
 *           combo_proc16
 */
static LRESULT combo_proc16( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, BOOL unicode )
{
    static const UINT msg16_offset = CB_GETEDITSEL16 - CB_GETEDITSEL;

    switch (msg)
    {
    case CB_INSERTSTRING16:
    case CB_SELECTSTRING16:
    case CB_FINDSTRING16:
    case CB_FINDSTRINGEXACT16:
        wParam = (INT)(INT16)wParam;
        /* fall through */
    case CB_ADDSTRING16:
    {
        DWORD style = GetWindowLongW( hwnd, GWL_STYLE );
        if ((style & CBS_HASSTRINGS) || !(style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)))
           lParam = (LPARAM)MapSL(lParam);
        msg -= msg16_offset;
        break;
    }
    case CB_SETITEMHEIGHT16:
    case CB_GETITEMHEIGHT16:
    case CB_SETCURSEL16:
    case CB_GETLBTEXTLEN16:
    case CB_GETITEMDATA16:
    case CB_SETITEMDATA16:
        wParam = (INT)(INT16)wParam;	/* signed integer */
        msg -= msg16_offset;
        break;
    case CB_GETDROPPEDCONTROLRECT16:
        lParam = (LPARAM)MapSL(lParam);
        if (lParam)
        {
            RECT r;
            RECT16 *r16 = (RECT16 *)lParam;
            wow_handlers32.combo_proc( hwnd, CB_GETDROPPEDCONTROLRECT, wParam, (LPARAM)&r, FALSE );
            r16->left   = r.left;
            r16->top    = r.top;
            r16->right  = r.right;
            r16->bottom = r.bottom;
        }
        return CB_OKAY;
    case CB_DIR16:
        if (wParam & DDL_DRIVES) wParam |= DDL_EXCLUSIVE;
        lParam = (LPARAM)MapSL(lParam);
        msg -= msg16_offset;
        break;
    case CB_GETLBTEXT16:
        wParam = (INT)(INT16)wParam;
        lParam = (LPARAM)MapSL(lParam);
        msg -= msg16_offset;
        break;
    case CB_GETEDITSEL16:
        wParam = lParam = 0;   /* just in case */
        msg -= msg16_offset;
        break;
    case CB_LIMITTEXT16:
    case CB_SETEDITSEL16:
    case CB_DELETESTRING16:
    case CB_RESETCONTENT16:
    case CB_GETDROPPEDSTATE16:
    case CB_SHOWDROPDOWN16:
    case CB_GETCOUNT16:
    case CB_GETCURSEL16:
    case CB_SETEXTENDEDUI16:
    case CB_GETEXTENDEDUI16:
        msg -= msg16_offset;
        break;
    default:
        return wow_handlers32.combo_proc( hwnd, msg, wParam, lParam, unicode );
    }
    return wow_handlers32.combo_proc( hwnd, msg, wParam, lParam, FALSE );
}

/*********************************************************************
 * edit_lock_buffer (internal)
 *
 * A 16 bit application might send an EM_GETHANDLE message and expect a HLOCAL16
 * (16 bit SEG:OFF handler). From that moment on we have to keep using this
 * 16 bit memory handler, because it is supposed to be valid at all times after
 * EM_GETHANDLE.
 * We create a HLOCAL16 buffer in edit_get_handle and copy the text from the
 * HLOCAL buffer, when needed
 *
 */

#define GWW_HANDLE16 sizeof(void*)

static void edit_lock_buffer( HWND hwnd )
{
    HLOCAL16 hloc16 = GetWindowWord( hwnd, GWW_HANDLE16 );
    HANDLE16 oldDS;
    HLOCAL hloc32;
    UINT size;

    if (!hloc16) return;
    if (!(hloc32 = (HLOCAL)wow_handlers32.edit_proc( hwnd, EM_GETHANDLE, 0, 0, FALSE ))) return;

    oldDS = CURRENT_DS;
    CURRENT_DS = GetWindowLongPtrW( hwnd, GWLP_HINSTANCE );
    size = LocalSize16(hloc16);
    if (LocalReAlloc( hloc32, size, LMEM_MOVEABLE ))
    {
        char *text = MapSL( LocalLock16( hloc16 ));
        char *dest = LocalLock( hloc32 );
        memcpy( dest, text, size );
        LocalUnlock( hloc32 );
        LocalUnlock16( hloc16 );
    }
    CURRENT_DS = oldDS;
}

static void edit_unlock_buffer( HWND hwnd )
{
    HLOCAL16 hloc16 = GetWindowWord( hwnd, GWW_HANDLE16 );
    HANDLE16 oldDS;
    HLOCAL hloc32;
    UINT size;

    if (!hloc16) return;
    if (!(hloc32 = (HLOCAL)wow_handlers32.edit_proc( hwnd, EM_GETHANDLE, 0, 0, FALSE ))) return;
    size = LocalSize( hloc32 );

    oldDS = CURRENT_DS;
    CURRENT_DS = GetWindowLongPtrW( hwnd, GWLP_HINSTANCE );
    if (LocalReAlloc16( hloc16, size, LMEM_MOVEABLE ))
    {
        char *text = LocalLock( hloc32 );
        char *dest = MapSL( LocalLock16( hloc16 ));
        memcpy( dest, text, size );
        LocalUnlock( hloc32 );
        LocalUnlock16( hloc16 );
    }
    CURRENT_DS = oldDS;
}

static HLOCAL16 edit_get_handle( HWND hwnd )
{
    CHAR *textA;
    UINT alloc_size;
    HLOCAL hloc;
    HANDLE16 oldDS;
    HLOCAL16 hloc16 = GetWindowWord( hwnd, GWW_HANDLE16 );

    if (hloc16) return hloc16;

    if (!(hloc = (HLOCAL)wow_handlers32.edit_proc( hwnd, EM_GETHANDLE, 0, 0, FALSE ))) return 0;
    alloc_size = LocalSize( hloc );

    oldDS = CURRENT_DS;
    CURRENT_DS = GetWindowLongPtrW( hwnd, GWLP_HINSTANCE );

    if (!LocalHeapSize16())
    {
        if (!LocalInit16(CURRENT_DS, 0, GlobalSize16(CURRENT_DS)))
        {
            ERR("could not initialize local heap\n");
            goto done;
        }
    }

    if (!(hloc16 = LocalAlloc16(LMEM_MOVEABLE | LMEM_ZEROINIT, alloc_size)))
    {
        ERR("could not allocate new 16 bit buffer\n");
        goto done;
    }

    if (!(textA = MapSL(LocalLock16( hloc16))))
    {
        ERR("could not lock new 16 bit buffer\n");
        LocalFree16(hloc16);
        hloc16 = 0;
        goto done;
    }
    memcpy( textA, LocalLock( hloc ), alloc_size );
    LocalUnlock( hloc );
    LocalUnlock16( hloc16 );
    SetWindowWord( hwnd, GWW_HANDLE16, hloc16 );

done:
    CURRENT_DS = oldDS;
    return hloc16;
}

static void edit_set_handle( HWND hwnd, HLOCAL16 hloc16 )
{
    HINSTANCE16 hInstance = GetWindowLongPtrW( hwnd, GWLP_HINSTANCE );
    HANDLE16 oldDS = CURRENT_DS;
    HLOCAL hloc32;
    INT count;
    CHAR *text;

    if (!(GetWindowLongW( hwnd, GWL_STYLE ) & ES_MULTILINE)) return;
    if (!hloc16) return;

    CURRENT_DS = hInstance;
    count = LocalSize16(hloc16);
    text = MapSL(LocalLock16(hloc16));
    if ((hloc32 = LocalAlloc(LMEM_MOVEABLE, count)))
    {
        memcpy( LocalLock(hloc32), text, count );
        LocalUnlock(hloc32);
        LocalUnlock16(hloc16);
        SetWindowWord( hwnd, GWW_HANDLE16, hloc16 );
    }
    CURRENT_DS = oldDS;
    if (hloc32) wow_handlers32.edit_proc( hwnd, EM_SETHANDLE, (WPARAM)hloc32, 0, FALSE );
}

static void edit_destroy_handle( HWND hwnd )
{
    HLOCAL16 hloc16 = GetWindowWord( hwnd, GWW_HANDLE16 );
    if (hloc16)
    {
        HANDLE16 oldDS = CURRENT_DS;

        CURRENT_DS = GetWindowLongPtrW( hwnd, GWLP_HINSTANCE );
        while (LocalUnlock16(hloc16)) ;
        LocalFree16(hloc16);
        CURRENT_DS = oldDS;
        SetWindowWord( hwnd, GWW_HANDLE16, 0 );
    }
}

/*********************************************************************
 *	edit_proc16
 */
static LRESULT edit_proc16( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, BOOL unicode )
{
    static const UINT msg16_offset = EM_GETSEL16 - EM_GETSEL;
    LRESULT result = 0;

    edit_lock_buffer( hwnd );
    switch (msg)
    {
    case EM_SCROLL16:
    case EM_SCROLLCARET16:
    case EM_GETMODIFY16:
    case EM_SETMODIFY16:
    case EM_GETLINECOUNT16:
    case EM_GETTHUMB16:
    case EM_LINELENGTH16:
    case EM_LIMITTEXT16:
    case EM_CANUNDO16:
    case EM_UNDO16:
    case EM_FMTLINES16:
    case EM_LINEFROMCHAR16:
    case EM_SETPASSWORDCHAR16:
    case EM_EMPTYUNDOBUFFER16:
    case EM_SETREADONLY16:
    case EM_GETPASSWORDCHAR16:
	/* these messages missing from specs */
    case WM_USER+15:
    case WM_USER+16:
    case WM_USER+19:
    case WM_USER+26:
        result = wow_handlers32.edit_proc( hwnd, msg - msg16_offset, wParam, lParam, FALSE );
        break;
    case EM_GETSEL16:
        result = wow_handlers32.edit_proc( hwnd, msg - msg16_offset, 0, 0, FALSE );
        break;
    case EM_REPLACESEL16:
    case EM_GETLINE16:
        result = wow_handlers32.edit_proc( hwnd, msg - msg16_offset, wParam, (LPARAM)MapSL(lParam), FALSE );
        break;
    case EM_LINESCROLL16:
        result = wow_handlers32.edit_proc( hwnd, msg - msg16_offset, (INT)(SHORT)HIWORD(lParam),
                                           (INT)(SHORT)LOWORD(lParam), FALSE );
        break;
    case EM_LINEINDEX16:
        if ((INT16)wParam == -1) wParam = -1;
        result = wow_handlers32.edit_proc( hwnd, msg - msg16_offset, wParam, lParam, FALSE );
        break;
    case EM_SETSEL16:
        if ((short)LOWORD(lParam) == -1)
        {
            wParam = -1;
            lParam = 0;
        }
        else
        {
            wParam = LOWORD(lParam);
            lParam = HIWORD(lParam);
        }
        result = wow_handlers32.edit_proc( hwnd, msg - msg16_offset, wParam, lParam, FALSE );
        break;
    case EM_GETRECT16:
        if (lParam)
        {
            RECT rect;
            RECT16 *r16 = MapSL(lParam);
            wow_handlers32.edit_proc( hwnd, msg - msg16_offset, wParam, (LPARAM)&rect, FALSE );
            r16->left   = rect.left;
            r16->top    = rect.top;
            r16->right  = rect.right;
            r16->bottom = rect.bottom;
        }
        break;
    case EM_SETRECT16:
    case EM_SETRECTNP16:
        if (lParam)
        {
            RECT rect;
            RECT16 *r16 = MapSL(lParam);
            rect.left   = r16->left;
            rect.top    = r16->top;
            rect.right  = r16->right;
            rect.bottom = r16->bottom;
            wow_handlers32.edit_proc( hwnd, msg - msg16_offset, wParam, (LPARAM)&rect, FALSE );
        }
        break;
    case EM_SETHANDLE16:
        edit_set_handle( hwnd, (HLOCAL16)wParam );
        break;
    case EM_GETHANDLE16:
        result = edit_get_handle( hwnd );
        break;
    case EM_SETTABSTOPS16:
    {
        INT16 *tabs16 = MapSL(lParam);
        INT i, count = wParam, *tabs = NULL;
        if (count > 0)
        {
            if (!(tabs = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*tabs) ))) return 0;
            for (i = 0; i < count; i++) tabs[i] = tabs16[i];
        }
        result = wow_handlers32.edit_proc( hwnd, msg - msg16_offset, count, (LPARAM)tabs, FALSE );
        HeapFree( GetProcessHeap(), 0, tabs );
        break;
    }
    case EM_GETFIRSTVISIBLELINE16:
        if (!(GetWindowLongW( hwnd, GWL_STYLE ) & ES_MULTILINE)) break;
        result = wow_handlers32.edit_proc( hwnd, msg - msg16_offset, wParam, lParam, FALSE );
        break;
    case EM_SETWORDBREAKPROC16:
    {
        struct word_break_thunk *thunk = add_word_break_thunk( (EDITWORDBREAKPROC16)lParam );
        result = wow_handlers32.edit_proc( hwnd, EM_SETWORDBREAKPROC, wParam, (LPARAM)thunk, FALSE );
        break;
    }
    case EM_GETWORDBREAKPROC16:
        result = wow_handlers32.edit_proc( hwnd, EM_GETWORDBREAKPROC, wParam, lParam, FALSE );
        result = (LRESULT)get_word_break_thunk( (EDITWORDBREAKPROCA)result );
        break;
    case WM_NCDESTROY:
        edit_destroy_handle( hwnd );
        return wow_handlers32.edit_proc( hwnd, msg, wParam, lParam, unicode );  /* no unlock on destroy */
    case WM_HSCROLL:
    case WM_VSCROLL:
        if (LOWORD(wParam) == EM_GETTHUMB16 || LOWORD(wParam) == EM_LINESCROLL16) wParam -= msg16_offset;
        result = wow_handlers32.edit_proc( hwnd, msg, wParam, lParam, unicode );
        break;
    default:
        result = wow_handlers32.edit_proc( hwnd, msg, wParam, lParam, unicode );
        break;
    }
    edit_unlock_buffer( hwnd );
    return result;
}


/***********************************************************************
 *           listbox_proc16
 */
static LRESULT listbox_proc16( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, BOOL unicode )
{
    static const UINT msg16_offset = LB_ADDSTRING16 - LB_ADDSTRING;
    LRESULT ret;

    switch (msg)
    {
    case WM_SIZE:
        if (is_old_app( hwnd ))
        {
            DWORD style = GetWindowLongW( hwnd, GWL_STYLE );
            int height, remaining, item_height;
            RECT rect;

            /* give a margin for error to old 16 bits programs - if we need
               less than the height of the nonclient area, round to the
               *next* number of items */

            if (!(style & LBS_NOINTEGRALHEIGHT) && !(style & LBS_OWNERDRAWVARIABLE))
            {
                GetClientRect( hwnd, &rect );
                height = rect.bottom - rect.top;
                item_height = wow_handlers32.listbox_proc( hwnd, LB_GETITEMHEIGHT, 0, 0, FALSE );
                remaining = item_height ? (height % item_height) : 0;
                if ((height > item_height) && remaining)
                {
                    GetWindowRect( hwnd, &rect );
                    if ((item_height - remaining) <= rect.bottom - rect.top - height)
                        remaining = remaining - item_height;
                    TRACE( "[%p]: changing height %d -> %d\n", hwnd, height, height - remaining );
                    SetWindowPos( hwnd, 0, 0, 0, rect.right - rect.left,
                                  rect.bottom - rect.top - remaining,
                                  SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE );
                    return 0;
                }
            }
        }
        return wow_handlers32.listbox_proc( hwnd, msg, wParam, lParam, unicode );

    case LB_RESETCONTENT16:
    case LB_DELETESTRING16:
    case LB_GETITEMDATA16:
    case LB_SETITEMDATA16:
    case LB_GETCOUNT16:
    case LB_GETTEXTLEN16:
    case LB_GETCURSEL16:
    case LB_GETTOPINDEX16:
    case LB_GETITEMHEIGHT16:
    case LB_SETCARETINDEX16:
    case LB_GETCARETINDEX16:
    case LB_SETTOPINDEX16:
    case LB_SETCOLUMNWIDTH16:
    case LB_GETSELCOUNT16:
    case LB_SELITEMRANGE16:
    case LB_SELITEMRANGEEX16:
    case LB_GETHORIZONTALEXTENT16:
    case LB_SETHORIZONTALEXTENT16:
    case LB_GETANCHORINDEX16:
    case LB_CARETON16:
    case LB_CARETOFF16:
        msg -= msg16_offset;
        break;
    case LB_GETSEL16:
    case LB_SETSEL16:
    case LB_SETCURSEL16:
    case LB_SETANCHORINDEX16:
        wParam = (INT)(INT16)wParam;
        msg -= msg16_offset;
        break;
    case LB_INSERTSTRING16:
    case LB_FINDSTRING16:
    case LB_FINDSTRINGEXACT16:
    case LB_SELECTSTRING16:
        wParam = (INT)(INT16)wParam;
        /* fall through */
    case LB_ADDSTRING16:
    case LB_ADDFILE16:
    {
        DWORD style = GetWindowLongW( hwnd, GWL_STYLE );
        if ((style & LBS_HASSTRINGS) || !(style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)))
            lParam = (LPARAM)MapSL(lParam);
        msg -= msg16_offset;
        break;
    }
    case LB_GETTEXT16:
        lParam = (LPARAM)MapSL(lParam);
        msg -= msg16_offset;
        break;
    case LB_SETITEMHEIGHT16:
        lParam = LOWORD(lParam);
        msg -= msg16_offset;
        break;
    case LB_GETITEMRECT16:
        {
            RECT rect;
            RECT16 *r16 = MapSL(lParam);
            ret = wow_handlers32.listbox_proc( hwnd, LB_GETITEMRECT, (INT16)wParam, (LPARAM)&rect, FALSE );
            r16->left   = rect.left;
            r16->top    = rect.top;
            r16->right  = rect.right;
            r16->bottom = rect.bottom;
            return ret;
        }
    case LB_GETSELITEMS16:
    {
        INT16 *array16 = MapSL( lParam );
        INT i, count = (INT16)wParam, *array;
        if (!(array = HeapAlloc( GetProcessHeap(), 0, wParam * sizeof(*array) ))) return LB_ERRSPACE;
        ret = wow_handlers32.listbox_proc( hwnd, LB_GETSELITEMS, count, (LPARAM)array, FALSE );
        for (i = 0; i < ret; i++) array16[i] = array[i];
        HeapFree( GetProcessHeap(), 0, array );
        return ret;
    }
    case LB_DIR16:
        /* according to Win16 docs, DDL_DRIVES should make DDL_EXCLUSIVE
         * be set automatically (this is different in Win32) */
        if (wParam & DDL_DRIVES) wParam |= DDL_EXCLUSIVE;
        lParam = (LPARAM)MapSL(lParam);
        msg -= msg16_offset;
        break;
    case LB_SETTABSTOPS16:
    {
        INT i, count, *tabs = NULL;
        INT16 *tabs16 = MapSL( lParam );

        if ((count = (INT16)wParam) > 0)
        {
            if (!(tabs = HeapAlloc( GetProcessHeap(), 0, wParam * sizeof(*tabs) ))) return LB_ERRSPACE;
            for (i = 0; i < count; i++) tabs[i] = tabs16[i] << 1; /* FIXME */
        }
        ret = wow_handlers32.listbox_proc( hwnd, LB_SETTABSTOPS, count, (LPARAM)tabs, FALSE );
        HeapFree( GetProcessHeap(), 0, tabs );
        return ret;
    }
    default:
        return wow_handlers32.listbox_proc( hwnd, msg, wParam, lParam, unicode );
    }
    return wow_handlers32.listbox_proc( hwnd, msg, wParam, lParam, FALSE );
}


/***********************************************************************
 *           mdiclient_proc16
 */
static LRESULT mdiclient_proc16( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, BOOL unicode )
{
    if (msg == WM_CREATE)
    {
        LPCREATESTRUCTA cs = (LPCREATESTRUCTA)lParam;
        HINSTANCE instance = (HINSTANCE)GetWindowLongPtrW( hwnd, GWLP_HINSTANCE );
        BOOL is_win32 = !instance || ((ULONG_PTR)instance >> 16);

	/* Translation layer doesn't know what's in the cs->lpCreateParams
	 * so we have to keep track of what environment we're in. */
	if (!is_win32)
	{
            void *orig = cs->lpCreateParams;
            LRESULT ret;
            CLIENTCREATESTRUCT ccs;
            CLIENTCREATESTRUCT16 *ccs16 = MapSL( PtrToUlong( orig ));

            ccs.hWindowMenu  = HMENU_32(ccs16->hWindowMenu);
            ccs.idFirstChild = ccs16->idFirstChild;
            cs->lpCreateParams = &ccs;
            ret = wow_handlers32.mdiclient_proc( hwnd, msg, wParam, lParam, unicode );
            cs->lpCreateParams = orig;
            return ret;
	}
    }
    return wow_handlers32.mdiclient_proc( hwnd, msg, wParam, lParam, unicode );
}


/***********************************************************************
 *           scrollbar_proc16
 */
static LRESULT scrollbar_proc16( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, BOOL unicode )
{
    static const UINT msg16_offset = SBM_SETPOS16 - SBM_SETPOS;

    switch (msg)
    {
    case SBM_SETPOS16:
    case SBM_GETPOS16:
    case SBM_ENABLE_ARROWS16:
        msg -= msg16_offset;
        break;
    case SBM_SETRANGE16:
        msg = wParam ? SBM_SETRANGEREDRAW : SBM_SETRANGE;
        wParam = LOWORD(lParam);
        lParam = HIWORD(lParam);
        break;
    case SBM_GETRANGE16:
    {
        INT min, max;
        wow_handlers32.scrollbar_proc( hwnd, SBM_GETRANGE, (WPARAM)&min, (LPARAM)&max, FALSE );
        return MAKELRESULT(min, max);
    }
    default:
        return wow_handlers32.scrollbar_proc( hwnd, msg, wParam, lParam, unicode );
    }
    return wow_handlers32.scrollbar_proc( hwnd, msg, wParam, lParam, FALSE );
}


/***********************************************************************
 *           static_proc16
 */
static LRESULT static_proc16( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, BOOL unicode )
{
    switch (msg)
    {
    case WM_NCCREATE:
    {
        CREATESTRUCTA *cs = (CREATESTRUCTA *)lParam;
        LRESULT ret = wow_handlers32.static_proc( hwnd, msg, wParam, lParam, unicode );

        if (!ret) return 0;
        if (((ULONG_PTR)cs->hInstance >> 16)) return ret;  /* 32-bit instance, nothing to do */
        switch (cs->style & SS_TYPEMASK)
        {
        case SS_ICON:
            {
                HICON16 icon = LoadIcon16( HINSTANCE_16(cs->hInstance), cs->lpszName );
                if (!icon) icon = LoadCursor16( HINSTANCE_16(cs->hInstance), cs->lpszName );
                if (icon) wow_handlers32.static_proc( hwnd, STM_SETIMAGE, IMAGE_ICON,
                                                      (LPARAM)get_icon_32(icon), FALSE );
                break;
            }
        case SS_BITMAP:
            {
                HBITMAP16 bitmap = LoadBitmap16( HINSTANCE_16(cs->hInstance), cs->lpszName );
                if (bitmap) wow_handlers32.static_proc( hwnd, STM_SETIMAGE, IMAGE_BITMAP,
                                                        (LPARAM)HBITMAP_32(bitmap), FALSE );
                break;
            }
        }
        return ret;
    }
    case STM_SETICON16:
        wParam = (WPARAM)get_icon_32( (HICON16)wParam );
        return wow_handlers32.static_proc( hwnd, STM_SETICON, wParam, lParam, FALSE );
    case STM_GETICON16:
        return get_icon_16( (HICON)wow_handlers32.static_proc( hwnd, STM_GETICON, wParam, lParam, FALSE ));
    default:
        return wow_handlers32.static_proc( hwnd, msg, wParam, lParam, unicode );
    }
}


/***********************************************************************
 *           create_window16
 */
HWND create_window16( CREATESTRUCTW *cs, LPCWSTR className, HINSTANCE instance, BOOL unicode )
{
    /* map to module handle */
    if (instance && !((ULONG_PTR)instance >> 16))
        instance = HINSTANCE_32( GetExePtr( HINSTANCE_16(instance) ));

    return wow_handlers32.create_window( cs, className, instance, unicode );
}


static NTSTATUS WINAPI User16CallFreeIcon( void *args, ULONG size )
{
    ULONG *param = args;
    GlobalFree16( LOWORD(*param) );
    return STATUS_SUCCESS;
}


static NTSTATUS WINAPI User16ThunkLock( void *args, ULONG size )
{
    DWORD *param = args;
    if (size != sizeof(DWORD))
    {
        DWORD lock;
        ReleaseThunkLock( &lock );
        return NtCallbackReturn( &lock, sizeof(lock), STATUS_SUCCESS );
    }
    RestoreThunkLock( *param );
    return STATUS_SUCCESS;
}


void register_wow_handlers(void)
{
    KERNEL_CALLBACK_PROC *callback_table = NtCurrentTeb()->Peb->KernelCallbackTable;
    static const struct wow_handlers16 handlers16 =
    {
        button_proc16,
        combo_proc16,
        edit_proc16,
        listbox_proc16,
        mdiclient_proc16,
        scrollbar_proc16,
        static_proc16,
        create_window16,
        call_window_proc_Ato16,
        call_dialog_proc_Ato16,
    };

    callback_table[NtUserCallFreeIcon] = User16CallFreeIcon;
    callback_table[NtUserThunkLock]    = User16ThunkLock;

    NtUserEnableThunkLock( TRUE );

    UserRegisterWowHandlers( &handlers16, &wow_handlers32 );
}
