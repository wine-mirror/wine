/*
 * Windows hook functions
 *
 * Copyright 1994, 1995 Alexandre Julliard
 *                 1996 Andrew Lewycky
 *
 * Based on investigations by Alex Korobka
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Warning!
 * A HHOOK is a 32-bit handle for compatibility with Windows 3.0 where it was
 * a pointer to the next function. Now it is in fact composed of a USER heap
 * handle in the low 16 bits and of a HOOK_MAGIC value in the high 16 bits.
 */

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winuser16.h"
#include "wine/winbase16.h"
#include "wownt32.h"
#include "hook.h"
#include "win.h"
#include "queue.h"
#include "user.h"
#include "heap.h"
#include "struct32.h"
#include "winproc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(hook);

#include "pshpack1.h"

  /* Hook data (pointed to by a HHOOK) */
typedef struct
{
    HANDLE16   next;               /* 00 Next hook in chain */
    HOOKPROC   proc;               /* 02 Hook procedure (original) */
    INT16      id;                 /* 06 Hook id (WH_xxx) */
    HQUEUE16   ownerQueue;         /* 08 Owner queue (0 for system hook) */
    HMODULE16  ownerModule;        /* 0a Owner module */
    WORD       flags;              /* 0c flags */
} HOOKDATA;

#include "poppack.h"

#define HOOK_MAGIC  ((int)'H' | (int)'K' << 8)  /* 'HK' */
#define HHOOK_32(h) ((HHOOK)(h ? MAKELONG(h, HOOK_MAGIC) : 0))
#define HHOOK_16(h) ((HANDLE16)((HIWORD(h) == HOOK_MAGIC) ? LOWORD(h) : 0))

  /* This should probably reside in USER heap */
static HANDLE16 HOOK_systemHooks[WH_NB_HOOKS] = { 0, };

/* ### start build ### */
extern LONG CALLBACK HOOK_CallTo16_long_wwl(HOOKPROC16,WORD,WORD,LONG);
/* ### stop build ### */


/***********************************************************************
 *           call_hook_16
 */
inline static LRESULT call_hook_16( HOOKPROC16 proc, INT id, INT code, WPARAM wparam, LPARAM lparam )
{
    LRESULT ret = HOOK_CallTo16_long_wwl( proc, code, wparam, lparam );
    /* Grrr. While the hook procedure is supposed to have an LRESULT return
       value even in Win16, it seems that for those hook types where the
       return value is interpreted as BOOL, Windows doesn't actually check
       the HIWORD ...  Some buggy Win16 programs, notably WINFILE, rely on
       that, because they neglect to clear DX ... */
    if (id != WH_JOURNALPLAYBACK) ret = LOWORD( ret );
    return ret;
}


/***********************************************************************
 *           call_hook_16_to_32
 *
 * Convert hook params to 32-bit and call 32-bit hook procedure
 */
static LRESULT call_hook_16_to_32( HOOKPROC proc, INT id, INT code, WPARAM wparam, LPARAM lparam,
                                   BOOL unicode )
{
    LRESULT ret = 0;

    switch( id )
    {
    case WH_MSGFILTER:
    case WH_SYSMSGFILTER:
    case WH_JOURNALRECORD:
    {
        MSG16 *msg16 = MapSL(lparam);
        MSG msg32;

        STRUCT32_MSG16to32( msg16, &msg32 );
        ret = proc( code, wparam, (LPARAM)&msg32 );
        break;
    }

    case WH_GETMESSAGE:
    {
        MSG16 *msg16 = MapSL(lparam);
        MSG msg32;

        STRUCT32_MSG16to32( msg16, &msg32 );
        ret = proc( code, wparam, (LPARAM)&msg32 );
        STRUCT32_MSG32to16( &msg32, msg16 );
        break;
    }

    case WH_JOURNALPLAYBACK:
    {
        EVENTMSG16 *em16 = MapSL(lparam);
        EVENTMSG em32;

        em32.message = em16->message;
        em32.paramL  = em16->paramL;
        em32.paramH  = em16->paramH;
        em32.time    = em16->time;
        em32.hwnd    = 0;  /* FIXME */
        ret = proc( code, wparam, (LPARAM)&em32 );
        break;
    }

    case WH_CALLWNDPROC:
    {
        CWPSTRUCT16 *cwp16 = MapSL(lparam);
        CWPSTRUCT cwp32;

        cwp32.hwnd   = WIN_Handle32(cwp16->hwnd);
        cwp32.lParam = cwp16->lParam;

        if (unicode)
            WINPROC_MapMsg16To32W( cwp32.hwnd, cwp16->message, cwp16->wParam,
                                   &cwp32.message, &cwp32.wParam, &cwp32.lParam );
        else
            WINPROC_MapMsg16To32A( cwp32.hwnd, cwp16->message, cwp16->wParam,
                                   &cwp32.message, &cwp32.wParam, &cwp32.lParam );

        ret = proc( code, wparam, (LPARAM)&cwp32 );

        if (unicode)
            WINPROC_UnmapMsg16To32W( cwp32.hwnd, cwp32.message, cwp32.wParam, cwp32.lParam, 0 );
        else
            WINPROC_UnmapMsg16To32A( cwp32.hwnd, cwp32.message, cwp32.wParam, cwp32.lParam, 0 );
        break;
    }

    case WH_CBT:
        switch (code)
        {
        case HCBT_CREATEWND:
            {
                CBT_CREATEWNDA cbtcw32;
                CREATESTRUCTA cs32;
                CBT_CREATEWND16 *cbtcw16 = MapSL(lparam);
                CREATESTRUCT16 *cs16 = MapSL( (SEGPTR)cbtcw16->lpcs );

                cbtcw32.lpcs = &cs32;
                cbtcw32.hwndInsertAfter = WIN_Handle32( cbtcw16->hwndInsertAfter );
                STRUCT32_CREATESTRUCT16to32A( cs16, &cs32 );

                if (unicode)
                {
                    cs32.lpszName = (LPSTR)map_str_16_to_32W( cs16->lpszName );
                    cs32.lpszClass = (LPSTR)map_str_16_to_32W( cs16->lpszClass );
                    ret = proc( code, wparam, (LPARAM)&cbtcw32 );
                    unmap_str_16_to_32W( (LPWSTR)cs32.lpszName );
                    unmap_str_16_to_32W( (LPWSTR)cs32.lpszClass );
                }
                else
                {
                    cs32.lpszName = MapSL( cs16->lpszName );
                    cs32.lpszClass = MapSL( cs16->lpszClass );
                    ret = proc( code, wparam, (LPARAM)&cbtcw32 );
                }
                cbtcw16->hwndInsertAfter = HWND_16( cbtcw32.hwndInsertAfter );
                break;
            }
        case HCBT_ACTIVATE:
            {
                CBTACTIVATESTRUCT16 *cas16 = MapSL(lparam);
                CBTACTIVATESTRUCT cas32;
                cas32.fMouse = cas16->fMouse;
                cas32.hWndActive = WIN_Handle32(cas16->hWndActive);
                ret = proc( code, wparam, (LPARAM)&cas32 );
                break;
            }
        case HCBT_CLICKSKIPPED:
            {
                MOUSEHOOKSTRUCT16 *ms16 = MapSL(lparam);
                MOUSEHOOKSTRUCT ms32;

                ms32.pt.x = ms16->pt.x;
                ms32.pt.y = ms16->pt.y;
                /* wHitTestCode may be negative, so convince compiler to do
                   correct sign extension. Yay. :| */
                ms32.wHitTestCode = (INT)(INT16)ms16->wHitTestCode;
                ms32.dwExtraInfo = ms16->dwExtraInfo;
                ms32.hwnd = WIN_Handle32( ms16->hwnd );
                ret = proc( code, wparam, (LPARAM)&ms32 );
                break;
            }
        case HCBT_MOVESIZE:
            {
                RECT16 *rect16 = MapSL(lparam);
                RECT rect32;

                CONV_RECT16TO32( rect16, &rect32 );
                ret = proc( code, wparam, (LPARAM)&rect32 );
                break;
            }
        }
        break;

    case WH_MOUSE:
    {
        MOUSEHOOKSTRUCT16 *ms16 = MapSL(lparam);
        MOUSEHOOKSTRUCT ms32;

        ms32.pt.x = ms16->pt.x;
        ms32.pt.y = ms16->pt.y;
        /* wHitTestCode may be negative, so convince compiler to do
           correct sign extension. Yay. :| */
        ms32.wHitTestCode = (INT)((INT16)ms16->wHitTestCode);
        ms32.dwExtraInfo = ms16->dwExtraInfo;
        ms32.hwnd = WIN_Handle32(ms16->hwnd);
        ret = proc( code, wparam, (LPARAM)&ms32 );
        break;
    }

    case WH_DEBUG:
    {
        DEBUGHOOKINFO16 *dh16 = MapSL(lparam);
        DEBUGHOOKINFO dh32;

        dh32.idThread = 0;            /* FIXME */
        dh32.idThreadInstaller = 0;   /* FIXME */
        dh32.lParam = dh16->lParam;   /* FIXME Check for sign ext */
        dh32.wParam = dh16->wParam;
        dh32.code   = dh16->code;

        /* do sign extension if it was WH_MSGFILTER */
        if (wparam == 0xffff) wparam = WH_MSGFILTER;
        ret = proc( code, wparam, (LPARAM)&dh32 );
        break;
    }

    case WH_SHELL:
    case WH_KEYBOARD:
        ret = proc( code, wparam, lparam );
        break;

    case WH_HARDWARE:
    case WH_FOREGROUNDIDLE:
    case WH_CALLWNDPROCRET:
    default:
        FIXME("\t[%i] 16to32 translation unimplemented\n", id);
        ret = proc( code, wparam, lparam );
        break;
    }
    return ret;
}


/***********************************************************************
 *           call_hook_32_to_16
 *
 * Convert hook params to 16-bit and call 16-bit hook procedure
 */
static LRESULT call_hook_32_to_16( HOOKPROC16 proc, INT id, INT code, WPARAM wparam, LPARAM lparam,
                                   BOOL unicode )
{
    LRESULT ret = 0;

    switch (id)
    {
    case WH_MSGFILTER:
    case WH_SYSMSGFILTER:
    case WH_JOURNALRECORD:
    {
        MSG *msg32 = (MSG *)lparam;
        MSG16 msg16;

        STRUCT32_MSG32to16( msg32, &msg16 );
        lparam = MapLS( &msg16 );
        ret = call_hook_16( proc, id, code, wparam, lparam );
        UnMapLS( lparam );
        break;
    }

    case WH_GETMESSAGE:
    {
        MSG *msg32 = (MSG *)lparam;
        MSG16 msg16;

        STRUCT32_MSG32to16( msg32, &msg16 );
        lparam = MapLS( &msg16 );
        ret = call_hook_16( proc, id, code, wparam, lparam );
        UnMapLS( lparam );
        STRUCT32_MSG16to32( &msg16, msg32 );
        break;
    }

    case WH_JOURNALPLAYBACK:
    {
        EVENTMSG *em32 = (EVENTMSG *)lparam;
        EVENTMSG16 em16;

        em16.message = em32->message;
        em16.paramL  = em32->paramL;
        em16.paramH  = em32->paramH;
        em16.time    = em32->time;
        lparam = MapLS( &em16 );
        ret = call_hook_16( proc, id, code, wparam, lparam );
        UnMapLS( lparam );
        break;
    }

    case WH_CALLWNDPROC:
    {
        CWPSTRUCT *cwp32 = (CWPSTRUCT *)lparam;
        CWPSTRUCT16 cwp16;
        MSGPARAM16 mp16;

        cwp16.hwnd   = HWND_16(cwp32->hwnd);
        cwp16.lParam = cwp32->lParam;

        if (unicode)
            WINPROC_MapMsg32WTo16( cwp32->hwnd, cwp32->message, cwp32->wParam,
                                   &cwp16.message, &cwp16.wParam, &cwp16.lParam );
        else
            WINPROC_MapMsg32ATo16( cwp32->hwnd, cwp32->message, cwp32->wParam,
                                   &cwp16.message, &cwp16.wParam, &cwp16.lParam );

        lparam = MapLS( &cwp16 );
        ret = call_hook_16( proc, id, code, wparam, lparam );
        UnMapLS( lparam );

        mp16.wParam  = cwp16.wParam;
        mp16.lParam  = cwp16.lParam;
        mp16.lResult = 0;
        if (unicode)
            WINPROC_UnmapMsg32WTo16( cwp32->hwnd, cwp32->message, cwp32->wParam,
                                     cwp32->lParam, &mp16 );
        else
            WINPROC_UnmapMsg32ATo16( cwp32->hwnd, cwp32->message, cwp32->wParam,
                                     cwp32->lParam, &mp16 );
        break;
    }

    case WH_CBT:
        switch (code)
        {
        case HCBT_CREATEWND:
            {
                CBT_CREATEWNDA *cbtcw32 = (CBT_CREATEWNDA *)lparam;
                CBT_CREATEWND16 cbtcw16;
                CREATESTRUCT16 cs16;

                STRUCT32_CREATESTRUCT32Ato16( cbtcw32->lpcs, &cs16 );
                cbtcw16.lpcs = (CREATESTRUCT16 *)MapLS( &cs16 );
                cbtcw16.hwndInsertAfter = HWND_16( cbtcw32->hwndInsertAfter );
                lparam = MapLS( &cbtcw16 );

                if (unicode)
                {
                    cs16.lpszName = map_str_32W_to_16( (LPWSTR)cbtcw32->lpcs->lpszName );
                    cs16.lpszClass = map_str_32W_to_16( (LPWSTR)cbtcw32->lpcs->lpszClass );
                    ret = call_hook_16( proc, id, code, wparam, lparam );
                    unmap_str_32W_to_16( cs16.lpszName );
                    unmap_str_32W_to_16( cs16.lpszClass );
                }
                else
                {
                    cs16.lpszName = MapLS( cbtcw32->lpcs->lpszName );
                    cs16.lpszClass = MapLS( cbtcw32->lpcs->lpszClass );
                    ret = call_hook_16( proc, id, code, wparam, lparam );
                    UnMapLS( cs16.lpszName );
                    UnMapLS( cs16.lpszClass );
                }
                cbtcw32->hwndInsertAfter = WIN_Handle32( cbtcw16.hwndInsertAfter );
                UnMapLS( (SEGPTR)cbtcw16.lpcs );
                UnMapLS( lparam );
                break;
            }

        case HCBT_ACTIVATE:
            {
                CBTACTIVATESTRUCT *cas32 = (CBTACTIVATESTRUCT *)lparam;
                CBTACTIVATESTRUCT16 cas16;

                cas16.fMouse     = cas32->fMouse;
                cas16.hWndActive = HWND_16( cas32->hWndActive );

                lparam = MapLS( &cas16 );
                ret = call_hook_16( proc, id, code, wparam, lparam );
                UnMapLS( lparam );
                break;
            }
        case HCBT_CLICKSKIPPED:
            {
                MOUSEHOOKSTRUCT *ms32 = (MOUSEHOOKSTRUCT *)lparam;
                MOUSEHOOKSTRUCT16 ms16;

                ms16.pt.x         = ms32->pt.x;
                ms16.pt.y         = ms32->pt.y;
                ms16.hwnd         = HWND_16( ms32->hwnd );
                ms16.wHitTestCode = ms32->wHitTestCode;
                ms16.dwExtraInfo  = ms32->dwExtraInfo;

                lparam = MapLS( &ms16 );
                ret = call_hook_16( proc, id, code, wparam, lparam );
                UnMapLS( lparam );
                break;
            }
        case HCBT_MOVESIZE:
            {
                RECT *rect32 = (RECT *)lparam;
                RECT16 rect16;

                CONV_RECT32TO16( rect32, &rect16 );
                lparam = MapLS( &rect16 );
                ret = call_hook_16( proc, id, code, wparam, lparam );
                UnMapLS( lparam );
                break;
            }
        }
        break;

    case WH_MOUSE:
    {
        MOUSEHOOKSTRUCT *ms32 = (MOUSEHOOKSTRUCT *)lparam;
        MOUSEHOOKSTRUCT16 ms16;

        ms16.pt.x         = ms32->pt.x;
        ms16.pt.y         = ms32->pt.y;
        ms16.hwnd         = HWND_16( ms32->hwnd );
        ms16.wHitTestCode = ms32->wHitTestCode;
        ms16.dwExtraInfo  = ms32->dwExtraInfo;

        lparam = MapLS( &ms16 );
        ret = call_hook_16( proc, id, code, wparam, lparam );
        UnMapLS( lparam );
        break;
    }

    case WH_DEBUG:
    {
        DEBUGHOOKINFO *dh32 = (DEBUGHOOKINFO *)lparam;
        DEBUGHOOKINFO16 dh16;

        dh16.hModuleHook = 0; /* FIXME */
        dh16.reserved    = 0;
        dh16.lParam      = dh32->lParam;
        dh16.wParam      = dh32->wParam;
        dh16.code        = dh32->code;

        lparam = MapLS( &dh16 );
        ret = call_hook_16( proc, id, code, wparam, lparam );
        UnMapLS( lparam );
        break;
    }

    case WH_SHELL:
    case WH_KEYBOARD:
        ret = call_hook_16( proc, id, code, wparam, lparam );
        break;

    case WH_HARDWARE:
    case WH_FOREGROUNDIDLE:
    case WH_CALLWNDPROCRET:
    default:
        FIXME("\t[%i] 32to16 translation unimplemented\n", id);
        ret = call_hook_16( proc, id, code, wparam, lparam );
        break;
    }
    return ret;
}


/***********************************************************************
 *           call_hook_32_to_32
 *
 * Convert hook params to/from Unicode and call hook procedure
 */
static LRESULT call_hook_32_to_32( HOOKPROC proc, INT id, INT code, WPARAM wparam, LPARAM lparam,
                                   BOOL to_unicode )
{
    if (id != WH_CBT || code != HCBT_CREATEWND) return proc( code, wparam, lparam );

    if (to_unicode)  /* ASCII to Unicode */
    {
        CBT_CREATEWNDA *cbtcwA = (CBT_CREATEWNDA *)lparam;
        CBT_CREATEWNDW cbtcwW;
        CREATESTRUCTW csW;
        LRESULT ret;

        cbtcwW.lpcs = &csW;
        cbtcwW.hwndInsertAfter = cbtcwA->hwndInsertAfter;
        csW = *(CREATESTRUCTW *)cbtcwA->lpcs;

        if (HIWORD(cbtcwA->lpcs->lpszName))
            csW.lpszName = HEAP_strdupAtoW( GetProcessHeap(), 0, cbtcwA->lpcs->lpszName );
        if (HIWORD(cbtcwA->lpcs->lpszClass))
            csW.lpszClass = HEAP_strdupAtoW( GetProcessHeap(), 0, cbtcwA->lpcs->lpszClass );
        ret = proc( code, wparam, (LPARAM)&cbtcwW );
        cbtcwA->hwndInsertAfter = cbtcwW.hwndInsertAfter;
        if (HIWORD(csW.lpszName)) HeapFree( GetProcessHeap(), 0, (LPWSTR)csW.lpszName );
        if (HIWORD(csW.lpszClass)) HeapFree( GetProcessHeap(), 0, (LPWSTR)csW.lpszClass );
        return ret;
    }
    else  /* Unicode to ASCII */
    {
        CBT_CREATEWNDW *cbtcwW = (CBT_CREATEWNDW *)lparam;
        CBT_CREATEWNDA cbtcwA;
        CREATESTRUCTA csA;
        LRESULT ret;

        cbtcwA.lpcs = &csA;
        cbtcwA.hwndInsertAfter = cbtcwW->hwndInsertAfter;
        csA = *(CREATESTRUCTA *)cbtcwW->lpcs;

        if (HIWORD(cbtcwW->lpcs->lpszName))
            csA.lpszName = HEAP_strdupWtoA( GetProcessHeap(), 0, cbtcwW->lpcs->lpszName );
        if (HIWORD(cbtcwW->lpcs->lpszClass))
            csA.lpszClass = HEAP_strdupWtoA( GetProcessHeap(), 0, cbtcwW->lpcs->lpszClass );
        ret = proc( code, wparam, (LPARAM)&cbtcwA );
        cbtcwW->hwndInsertAfter = cbtcwA.hwndInsertAfter;
        if (HIWORD(csA.lpszName)) HeapFree( GetProcessHeap(), 0, (LPSTR)csA.lpszName );
        if (HIWORD(csA.lpszClass)) HeapFree( GetProcessHeap(), 0, (LPSTR)csA.lpszClass );
        return ret;
    }
}


/***********************************************************************
 *           call_hook
 *
 * Call a hook procedure.
 */
inline static LRESULT call_hook( HOOKDATA *data, INT fromtype, INT code,
                                 WPARAM wparam, LPARAM lparam )
{
    INT type = (data->flags & HOOK_MAPTYPE);
    LRESULT ret;

    /* Suspend window structure locks before calling user code */
    int iWndsLocks = WIN_SuspendWndsLock();

    if (type == HOOK_WIN16)
    {
        if (fromtype == HOOK_WIN16)  /* 16->16 */
            ret = call_hook_16( (HOOKPROC16)data->proc, data->id, code, wparam, lparam );
        else  /* 32->16 */
            ret = call_hook_32_to_16( (HOOKPROC16)data->proc, data->id, code, wparam,
                                      lparam, (type == HOOK_WIN32W) );
    }
    else if (fromtype == HOOK_WIN16)  /* 16->32 */
        ret = call_hook_16_to_32( data->proc, data->id, code, wparam,
                                  lparam, (type == HOOK_WIN32W) );
    else /* 32->32, check unicode */
    {
        if (type == fromtype)
            ret = data->proc( code, wparam, lparam );
        else
            ret = call_hook_32_to_32( data->proc, data->id, code, wparam,
                                      lparam, (type == HOOK_WIN32W) );
    }
    WIN_RestoreWndsLock(iWndsLocks);
    return ret;
}


/***********************************************************************
 *           HOOK_GetNextHook
 *
 * Get the next hook of a given hook.
 */
static HHOOK HOOK_GetNextHook( HHOOK hook )
{
    HOOKDATA *data = (HOOKDATA *)USER_HEAP_LIN_ADDR( hook );

    if (!data || !hook) return 0;
    if (data->next) return HHOOK_32(data->next);
    if (!data->ownerQueue) return 0;  /* Already system hook */

    /* Now start enumerating the system hooks */
    return HHOOK_32(HOOK_systemHooks[data->id - WH_MINHOOK]);
}


/***********************************************************************
 *           HOOK_GetHook
 *
 * Get the first hook for a given type.
 */
static HHOOK HOOK_GetHook( INT16 id )
{
    MESSAGEQUEUE *queue;
    HANDLE16 handle = 0;

    if ((queue = QUEUE_Current()) != NULL)
        handle = queue->hooks[id - WH_MINHOOK];
    if (!handle) handle = HOOK_systemHooks[id - WH_MINHOOK];
    return HHOOK_32(handle);
}


/***********************************************************************
 *           HOOK_SetHook
 *
 * Install a given hook.
 */
static HHOOK HOOK_SetHook( INT16 id, LPVOID proc, INT type,
		           HMODULE16 hModule, DWORD dwThreadId )
{
    HOOKDATA *data;
    HANDLE16 handle;
    HQUEUE16 hQueue = 0;

    if ((id < WH_MINHOOK) || (id > WH_MAXHOOK)) return 0;

    TRACE("Setting hook %d: %08x %04x %08lx\n",
                  id, (UINT)proc, hModule, dwThreadId );

    /* Create task queue if none present */
    InitThreadInput16( 0, 0 );

    if (id == WH_JOURNALPLAYBACK) EnableHardwareInput16(FALSE);

    if (dwThreadId)  /* Task-specific hook */
    {
	if ((id == WH_JOURNALRECORD) || (id == WH_JOURNALPLAYBACK) ||
	    (id == WH_SYSMSGFILTER)) return 0;  /* System-only hooks */
        if (!(hQueue = GetThreadQueue16( dwThreadId )))
            return 0;
    }

    /* Create the hook structure */

    if (!(handle = USER_HEAP_ALLOC( sizeof(HOOKDATA) ))) return 0;
    data = (HOOKDATA *) USER_HEAP_LIN_ADDR( handle );
    data->proc        = proc;
    data->id          = id;
    data->ownerQueue  = hQueue;
    data->ownerModule = hModule;
    data->flags       = type;

    /* Insert it in the correct linked list */

    if (hQueue)
    {
        MESSAGEQUEUE *queue = (MESSAGEQUEUE *)QUEUE_Lock( hQueue );
        data->next = queue->hooks[id - WH_MINHOOK];
        queue->hooks[id - WH_MINHOOK] = handle;
        QUEUE_Unlock( queue );
    }
    else
    {
        data->next = HOOK_systemHooks[id - WH_MINHOOK];
        HOOK_systemHooks[id - WH_MINHOOK] = handle;
    }
    TRACE("Setting hook %d: ret=%04x [next=%04x]\n",
			   id, handle, data->next );

    return HHOOK_32(handle);
}


/***********************************************************************
 *           HOOK_RemoveHook
 *
 * Remove a hook from the list.
 */
static BOOL HOOK_RemoveHook( HHOOK hook )
{
    HOOKDATA *data;
    HANDLE16 *prevHandle;

    TRACE("Removing hook %04x\n", hook );

    if (!(data = (HOOKDATA *)USER_HEAP_LIN_ADDR(hook))) return FALSE;
    if (data->flags & HOOK_INUSE)
    {
        /* Mark it for deletion later on */
        WARN("Hook still running, deletion delayed\n" );
        data->proc = (HOOKPROC)0;
        return TRUE;
    }

    if (data->id == WH_JOURNALPLAYBACK) EnableHardwareInput16(TRUE);

    /* Remove it from the linked list */

    if (data->ownerQueue)
    {
        MESSAGEQUEUE *queue = (MESSAGEQUEUE *)QUEUE_Lock( data->ownerQueue );
        if (!queue) return FALSE;
        prevHandle = &queue->hooks[data->id - WH_MINHOOK];
        QUEUE_Unlock( queue );
    }
    else prevHandle = &HOOK_systemHooks[data->id - WH_MINHOOK];

    while (*prevHandle && *prevHandle != HHOOK_16(hook))
        prevHandle = &((HOOKDATA *)USER_HEAP_LIN_ADDR(*prevHandle))->next;

    if (!*prevHandle) return FALSE;
    *prevHandle = data->next;

    USER_HEAP_FREE( hook );
    return TRUE;
}


/***********************************************************************
 *           HOOK_FindValidHook
 */
static HHOOK HOOK_FindValidHook( HHOOK hook )
{
    HOOKDATA *data;

    for (;;)
    {
	if (!(data = (HOOKDATA *)USER_HEAP_LIN_ADDR( hook ))) return 0;
	if (data->proc) return hook;
	hook = HHOOK_32(data->next);
    }
}

/***********************************************************************
 *           HOOK_CallHook
 *
 * Call a hook procedure.
 */
static LRESULT HOOK_CallHook( HHOOK hook, INT fromtype, INT code,
                              WPARAM wParam, LPARAM lParam )
{
    MESSAGEQUEUE *queue;
    HANDLE16 prevHandle;
    HOOKDATA *data = (HOOKDATA *)USER_HEAP_LIN_ADDR( hook );
    LRESULT ret;

    if (!(queue = QUEUE_Current())) return 0;
    prevHandle = queue->hCurHook;
    queue->hCurHook = HHOOK_16(hook);

    TRACE("Calling hook %04x: %d %08x %08lx\n", hook, code, wParam, lParam );

    data->flags |= HOOK_INUSE;
    ret = call_hook( data, fromtype, code, wParam, lParam );
    data->flags &= ~HOOK_INUSE;

    TRACE("Ret hook %04x = %08lx\n", hook, ret );

    queue->hCurHook = prevHandle;
    if (!data->proc) HOOK_RemoveHook( hook );
    return ret;
}

/***********************************************************************
 *           Exported Functions & APIs
 */

/***********************************************************************
 *           HOOK_IsHooked
 *
 * Replacement for calling HOOK_GetHook from other modules.
 */
BOOL HOOK_IsHooked( INT16 id )
{
    return HOOK_GetHook( id ) != 0;
}


/***********************************************************************
 *           HOOK_CallHooks16
 *
 * Call a hook chain.
 */
LRESULT HOOK_CallHooks16( INT16 id, INT16 code, WPARAM16 wParam,
                          LPARAM lParam )
{
    HHOOK hook;

    if (!(hook = HOOK_GetHook( id ))) return 0;
    if (!(hook = HOOK_FindValidHook(hook))) return 0;
    return HOOK_CallHook( hook, HOOK_WIN16, code, wParam, lParam );
}

/***********************************************************************
 *           HOOK_CallHooksA
 *
 * Call a hook chain.
 */
LRESULT HOOK_CallHooksA( INT id, INT code, WPARAM wParam,
                           LPARAM lParam )
{
    HHOOK hook;

    if (!(hook = HOOK_GetHook( id ))) return 0;
    if (!(hook = HOOK_FindValidHook(hook))) return 0;
    return HOOK_CallHook( hook, HOOK_WIN32A, code, wParam, lParam );
}

/***********************************************************************
 *           HOOK_CallHooksW
 *
 * Call a hook chain.
 */
LRESULT HOOK_CallHooksW( INT id, INT code, WPARAM wParam,
                           LPARAM lParam )
{
    HHOOK hook;

    if (!(hook = HOOK_GetHook( id ))) return 0;
    if (!(hook = HOOK_FindValidHook(hook))) return 0;
    return HOOK_CallHook( hook, HOOK_WIN32W, code, wParam,
			  lParam );
}


/***********************************************************************
 *	     HOOK_FreeModuleHooks
 */
void HOOK_FreeModuleHooks( HMODULE16 hModule )
{
 /* remove all system hooks registered by this module */

  HOOKDATA*     hptr;
  HANDLE16      handle, next;
  int           id;

  for( id = WH_MINHOOK; id <= WH_MAXHOOK; id++ )
    {
       handle = HOOK_systemHooks[id - WH_MINHOOK];
       while( handle )
          if( (hptr = (HOOKDATA *)USER_HEAP_LIN_ADDR(handle)) )
	    {
	      next = hptr->next;
	      if( hptr->ownerModule == hModule )
                {
                  hptr->flags &= HOOK_MAPTYPE;
                  HOOK_RemoveHook(HHOOK_32(handle));
                }
	      handle = next;
	    }
	  else handle = 0;
    }
}

/***********************************************************************
 *	     HOOK_FreeQueueHooks
 */
void HOOK_FreeQueueHooks(void)
{
  /* remove all hooks registered by the current queue */

  HOOKDATA*	hptr = NULL;
  HHOOK 	hook, next;
  int 		id;

  for( id = WH_MINHOOK; id <= WH_MAXHOOK; id++ )
    {
       hook = HOOK_GetHook( id );
       while( hook )
	{
	  next = HOOK_GetNextHook(hook);

	  hptr = (HOOKDATA *)USER_HEAP_LIN_ADDR( hook );
	  if( hptr && hptr->ownerQueue )
	    {
	      hptr->flags &= HOOK_MAPTYPE;
	      HOOK_RemoveHook(hook);
	    }
	  hook = next;
	}
    }
}


/***********************************************************************
 *		SetWindowsHook (USER.121)
 */
FARPROC16 WINAPI SetWindowsHook16( INT16 id, HOOKPROC16 proc )
{
    HINSTANCE16 hInst = FarGetOwner16( HIWORD(proc) );

    /* WH_MSGFILTER is the only task-specific hook for SetWindowsHook() */
    HTASK16 hTask = (id == WH_MSGFILTER) ? GetCurrentTask() : 0;

    return (FARPROC16)SetWindowsHookEx16( id, proc, hInst, hTask );
}

/***********************************************************************
 *		SetWindowsHookA (USER32.@)
 */
HHOOK WINAPI SetWindowsHookA( INT id, HOOKPROC proc )
{
    return SetWindowsHookExA( id, proc, 0, GetCurrentThreadId() );
}

/***********************************************************************
 *		SetWindowsHookW (USER32.@)
 */
HHOOK WINAPI SetWindowsHookW( INT id, HOOKPROC proc )
{
    return SetWindowsHookExW( id, proc, 0, GetCurrentThreadId() );
}


/***********************************************************************
 *		SetWindowsHookEx (USER.291)
 */
HHOOK WINAPI SetWindowsHookEx16( INT16 id, HOOKPROC16 proc, HINSTANCE16 hInst,
                                 HTASK16 hTask )
{
    if (id == WH_DEBUG)
    {
	FIXME("WH_DEBUG is broken in 16-bit Windows.\n");
	return 0;
    }
    return HOOK_SetHook( id, proc, HOOK_WIN16, GetExePtr(hInst), (DWORD)hTask );
}

/***********************************************************************
 *		SetWindowsHookExA (USER32.@)
 */
HHOOK WINAPI SetWindowsHookExA( INT id, HOOKPROC proc, HINSTANCE hInst,
                                  DWORD dwThreadId )
{
    return HOOK_SetHook( id, proc, HOOK_WIN32A, MapHModuleLS(hInst), dwThreadId );
}

/***********************************************************************
 *		SetWindowsHookExW (USER32.@)
 */
HHOOK WINAPI SetWindowsHookExW( INT id, HOOKPROC proc, HINSTANCE hInst,
                                  DWORD dwThreadId )
{
    return HOOK_SetHook( id, proc, HOOK_WIN32W, MapHModuleLS(hInst), dwThreadId );
}


/***********************************************************************
 *		UnhookWindowsHook (USER.234)
 */
BOOL16 WINAPI UnhookWindowsHook16( INT16 id, HOOKPROC16 proc )
{
    return UnhookWindowsHook( id, (HOOKPROC)proc );
}

/***********************************************************************
 *		UnhookWindowsHook (USER32.@)
 */
BOOL WINAPI UnhookWindowsHook( INT id, HOOKPROC proc )
{
    HHOOK hook = HOOK_GetHook( id );

    TRACE("%d %08lx\n", id, (DWORD)proc );

    while (hook)
    {
        HOOKDATA *data = (HOOKDATA *)USER_HEAP_LIN_ADDR( hook );
        if (data->proc == proc) break;
        hook = HOOK_GetNextHook( hook );
    }
    if (!hook) return FALSE;
    return HOOK_RemoveHook( hook );
}


/***********************************************************************
 *		UnhookWindowsHookEx (USER.292)
 */
BOOL16 WINAPI UnhookWindowsHookEx16( HHOOK hhook )
{
    return HOOK_RemoveHook(hhook);
}

/***********************************************************************
 *		UnhookWindowsHookEx (USER32.@)
 */
BOOL WINAPI UnhookWindowsHookEx( HHOOK hhook )
{
    return HOOK_RemoveHook(hhook);
}


/***********************************************************************
 *		CallNextHookEx (USER.293)
 *
 * I wouldn't have separated this into 16 and 32 bit versions, but I
 * need a way to figure out if I need to do a mapping or not.
 */
LRESULT WINAPI CallNextHookEx16( HHOOK hhook, INT16 code, WPARAM16 wParam,
                                 LPARAM lParam )
{
    HHOOK next;

    if (!(next = HOOK_GetNextHook(hhook))) return 0;

    return HOOK_CallHook( next, HOOK_WIN16, code, wParam, lParam );
}


/***********************************************************************
 *		CallNextHookEx (USER32.@)
 *
 * There aren't ANSI and UNICODE versions of this.
 */
LRESULT WINAPI CallNextHookEx( HHOOK hhook, INT code, WPARAM wParam,
                                 LPARAM lParam )
{
    HHOOK next;
    INT fromtype;	/* figure out Ansi/Unicode */
    HOOKDATA *oldhook;

    if (!(next = HOOK_GetNextHook(hhook))) return 0;

    oldhook = (HOOKDATA *)USER_HEAP_LIN_ADDR( hhook );
    fromtype = oldhook->flags & HOOK_MAPTYPE;

    if (fromtype == HOOK_WIN16)
      ERR("called from 16bit hook!\n");

    return HOOK_CallHook( next, fromtype, code, wParam, lParam );
}


/***********************************************************************
 *		DefHookProc (USER.235)
 */
LRESULT WINAPI DefHookProc16( INT16 code, WPARAM16 wParam, LPARAM lParam,
                              HHOOK *hhook )
{
    /* Note: the *hhook parameter is never used, since we rely on the
     * current hook value from the task queue to find the next hook. */
    MESSAGEQUEUE *queue;

    if (!(queue = QUEUE_Current())) return 0;
    return CallNextHookEx16(HHOOK_32(queue->hCurHook), code, wParam, lParam);
}


/***********************************************************************
 *		CallMsgFilter (USER.123)
 */
BOOL16 WINAPI CallMsgFilter16( SEGPTR msg, INT16 code )
{
    if (HOOK_CallHooks16( WH_SYSMSGFILTER, code, 0, (LPARAM)msg )) return TRUE;
    return HOOK_CallHooks16( WH_MSGFILTER, code, 0, (LPARAM)msg );
}


/***********************************************************************
 *		CallMsgFilter32 (USER.823)
 */
BOOL16 WINAPI CallMsgFilter32_16( SEGPTR msg16_32, INT16 code, BOOL16 wHaveParamHigh )
{
    MSG32_16 *lpmsg16_32 = MapSL(msg16_32);

    if (wHaveParamHigh == FALSE)
    {
        lpmsg16_32->wParamHigh = 0;
        /* WARNING: msg16_32->msg has to be the first variable in the struct */
        return CallMsgFilter16(msg16_32, code);
    }
    else
    {
        MSG msg32;
        BOOL16 ret;

        msg32.hwnd      = WIN_Handle32( lpmsg16_32->msg.hwnd );
        msg32.message   = lpmsg16_32->msg.message;
        msg32.wParam    = MAKELONG(lpmsg16_32->msg.wParam, lpmsg16_32->wParamHigh);
        msg32.lParam    = lpmsg16_32->msg.lParam;
        msg32.time      = lpmsg16_32->msg.time;
        msg32.pt.x      = lpmsg16_32->msg.pt.x;
        msg32.pt.y      = lpmsg16_32->msg.pt.y;

        ret = (BOOL16)CallMsgFilterA(&msg32, (INT)code);

        lpmsg16_32->msg.hwnd    = HWND_16( msg32.hwnd );
        lpmsg16_32->msg.message = msg32.message;
        lpmsg16_32->msg.wParam  = LOWORD(msg32.wParam);
        lpmsg16_32->msg.lParam  = msg32.lParam;
        lpmsg16_32->msg.time    = msg32.time;
        lpmsg16_32->msg.pt.x    = msg32.pt.x;
        lpmsg16_32->msg.pt.y    = msg32.pt.y;
        lpmsg16_32->wParamHigh  = HIWORD(msg32.wParam);

        return ret;
    }
}


/***********************************************************************
 *		CallMsgFilterA (USER32.@)
 *
 * FIXME: There are ANSI and UNICODE versions of this, plus an unspecified
 * version, plus USER (the 16bit one) has a CallMsgFilter32 function.
 */
BOOL WINAPI CallMsgFilterA( LPMSG msg, INT code )
{
    if (HOOK_CallHooksA( WH_SYSMSGFILTER, code, 0, (LPARAM)msg ))
      return TRUE;
    return HOOK_CallHooksA( WH_MSGFILTER, code, 0, (LPARAM)msg );
}


/***********************************************************************
 *		CallMsgFilterW (USER32.@)
 */
BOOL WINAPI CallMsgFilterW( LPMSG msg, INT code )
{
    if (HOOK_CallHooksW( WH_SYSMSGFILTER, code, 0, (LPARAM)msg ))
      return TRUE;
    return HOOK_CallHooksW( WH_MSGFILTER, code, 0, (LPARAM)msg );
}


/***********************************************************************
 *           SetWinEventHook                            [USER32.@]
 *
 * Set up an event hook for a set of events.
 *
 * PARAMS
 *  dwMin     [I] Lowest event handled by pfnProc
 *  dwMax     [I] Highest event handled by pfnProc
 *  hModule   [I] DLL containing pfnProc
 *  pfnProc   [I] Callback event hook function
 *  dwProcess [I] Process to get events from, or 0 for all processes
 *  dwThread  [I] Thread to get events from, or 0 for all threads
 *  dwFlags   [I] Flags indicating the status of pfnProc
 *
 * RETURNS
 *  Success: A handle representing the hook.
 *  Failure: A NULL handle.
 *
 * BUGS
 *  Not implemented.
 */
HWINEVENTHOOK WINAPI SetWinEventHook(DWORD dwMin, DWORD dwMax, HMODULE hModule,
                                     WINEVENTPROC pfnProc, DWORD dwProcess,
                                     DWORD dwThread, DWORD dwFlags)
{
  FIXME("(%ld,%ld,0x%08x,%p,%ld,%ld,0x%08lx)-stub!\n", dwMin, dwMax, hModule,
        pfnProc, dwProcess, dwThread, dwFlags);

  return (HWINEVENTHOOK)0;
}

/***********************************************************************
 *           UnhookWinEvent                             [USER32.@]
 *
 * Remove an event hook for a set of events.
 *
 * PARAMS
 *  hEventHook [I] Event hook to remove
 *
 * RETURNS
 *  Success: TRUE. The event hook has been removed.
 *  Failure: FALSE, if hEventHook is invalid.
 *
 * BUGS
 *  Not implemented.
 */
BOOL WINAPI UnhookWinEvent(HWINEVENTHOOK hEventHook)
{
  FIXME("(%p)-stub!\n", (void*)hEventHook);

  if (!hEventHook)
    return FALSE;

  return TRUE;
}

/***********************************************************************
 *           NotifyWinEvent                             [USER32.@]
 *
 * Inform the OS that an event has occurred.
 *
 * PARAMS
 *  dwEvent  [I] Id of the event
 *  hWnd     [I] Window holding the object that created the event
 *  nId      [I] Type of object that created the event
 *  nChildId [I] Child object of nId, or CHILDID_SELF.
 *
 * RETURNS
 *  Nothing.
 *
 * BUGS
 *  Not implemented.
 */
VOID WINAPI NotifyWinEvent(DWORD dwEvent, HWND hWnd, LONG nId, LONG nChildId)
{
  FIXME("(%ld,0x%08x,%ld,%ld)-stub!\n", dwEvent, hWnd, nId, nChildId);
}

/***********************************************************************
 *           IsWinEventHookInstalled                       [USER32.@]
 *
 * Determine if an event hook is installed for an event.
 *
 * PARAMS
 *  dwEvent  [I] Id of the event
 *
 * RETURNS
 *  TRUE,  If there are any hooks installed for the event.
 *  FALSE, Otherwise.
 *
 * BUGS
 *  Not implemented.
 */
BOOL WINAPI IsWinEventHookInstalled(DWORD dwEvent)
{
  FIXME("(%ld)-stub!\n", dwEvent);
  return TRUE;
}
