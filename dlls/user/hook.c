/*
 * Windows hook functions
 *
 * Copyright 2002 Alexandre Julliard
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
 *
 * NOTES:
 *   Status of the various hooks:
 *     WH_MSGFILTER                 OK
 *     WH_JOURNALRECORD             Partially implemented
 *     WH_JOURNALPLAYBACK           Partially implemented
 *     WH_KEYBOARD                  OK
 *     WH_GETMESSAGE	            OK (FIXME: A/W mapping?)
 *     WH_CALLWNDPROC	            OK (FIXME: A/W mapping?)
 *     WH_CBT
 *       HCBT_MOVESIZE              OK
 *       HCBT_MINMAX                OK
 *       HCBT_QS                    OK
 *       HCBT_CREATEWND             OK
 *       HCBT_DESTROYWND            OK
 *       HCBT_ACTIVATE              OK
 *       HCBT_CLICKSKIPPED          OK
 *       HCBT_KEYSKIPPED            OK
 *       HCBT_SYSCOMMAND            Not implemented
 *       HCBT_SETFOCUS              OK
 *     WH_SYSMSGFILTER	            OK
 *     WH_MOUSE	                    OK
 *     WH_HARDWARE	            Not supported in Win32
 *     WH_DEBUG	                    Not implemented
 *     WH_SHELL
 *       HSHELL_WINDOWCREATED       OK
 *       HSHELL_WINDOWDESTROYED     OK
 *       HSHELL_ACTIVATESHELLWINDOW Not implemented
 *       HSHELL_WINDOWACTIVATED     Not implemented
 *       HSHELL_GETMINRECT          Not implemented
 *       HSHELL_REDRAW              Not implemented
 *       HSHELL_TASKMAN             Not implemented
 *       HSHELL_LANGUAGE            Not implemented
 *       HSHELL_SYSMENU             Not implemented
 *       HSHELL_ENDTASK             Not implemented
 *       HSHELL_ACCESSIBILITYSTATE  Not implemented
 *       HSHELL_APPCOMMAND          Not implemented
 *       HSHELL_WINDOWREPLACED      Not implemented
 *       HSHELL_WINDOWREPLACING     Not implemented
 *     WH_FOREGROUNDIDLE            Not implemented
 *     WH_CALLWNDPROCRET            OK (FIXME: A/W mapping?)
 *     WH_KEYBOARD_LL               Implemented but should use SendMessage instead
 *     WH_MOUSE_LL                  Implemented but should use SendMessage instead
 */

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "heap.h"
#include "message.h"
#include "win.h"
#include "wine/server.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "winternl.h"

WINE_DEFAULT_DEBUG_CHANNEL(hook);
WINE_DECLARE_DEBUG_CHANNEL(relay);

static const char * const hook_names[WH_MAXHOOK - WH_MINHOOK + 1] =
{
    "WH_MSGFILTER",
    "WH_JOURNALRECORD",
    "WH_JOURNALPLAYBACK",
    "WH_KEYBOARD",
    "WH_GETMESSAGE",
    "WH_CALLWNDPROC",
    "WH_CBT",
    "WH_SYSMSGFILTER",
    "WH_MOUSE",
    "WH_HARDWARE",
    "WH_DEBUG",
    "WH_SHELL",
    "WH_FOREGROUNDIDLE",
    "WH_CALLWNDPROCRET",
    "WH_KEYBOARD_LL",
    "WH_MOUSE_LL"
};


/***********************************************************************
 *		set_windows_hook
 *
 * Implementation of SetWindowsHookExA and SetWindowsHookExW.
 */
static HHOOK set_windows_hook( INT id, HOOKPROC proc, HINSTANCE inst, DWORD tid, BOOL unicode )
{
    HHOOK handle = 0;
    WCHAR module[MAX_PATH];

    if (tid)  /* thread-local hook */
    {
        if (id == WH_JOURNALRECORD ||
            id == WH_JOURNALPLAYBACK ||
            id == WH_KEYBOARD_LL ||
            id == WH_MOUSE_LL ||
            id == WH_SYSMSGFILTER)
        {
            /* these can only be global */
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        inst = 0;
    }
    else  /* system-global hook */
    {
        if (!inst || !GetModuleFileNameW( inst, module, MAX_PATH ))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
    }

    SERVER_START_REQ( set_hook )
    {
        req->id      = id;
        req->tid     = tid;
        req->unicode = unicode;
        if (inst) /* make proc relative to the module base */
        {
            req->proc = (void *)((char *)proc - (char *)inst);
            wine_server_add_data( req, module, strlenW(module) * sizeof(WCHAR) );
        }
        else req->proc = proc;

        if (!wine_server_call_err( req )) handle = reply->handle;
    }
    SERVER_END_REQ;

    TRACE( "%s %p %lx -> %p\n", hook_names[id-WH_MINHOOK], proc, tid, handle );
    return handle;
}


/***********************************************************************
 *		call_hook_AtoW
 */
static LRESULT call_hook_AtoW( HOOKPROC proc, INT id, INT code, WPARAM wparam, LPARAM lparam )
{
    LRESULT ret;
    UNICODE_STRING usBuffer;
    if (id != WH_CBT || code != HCBT_CREATEWND) ret = proc( code, wparam, lparam );
    else
    {
        CBT_CREATEWNDA *cbtcwA = (CBT_CREATEWNDA *)lparam;
        CBT_CREATEWNDW cbtcwW;
        CREATESTRUCTW csW;

        cbtcwW.lpcs = &csW;
        cbtcwW.hwndInsertAfter = cbtcwA->hwndInsertAfter;
        csW = *(CREATESTRUCTW *)cbtcwA->lpcs;

        if (HIWORD(cbtcwA->lpcs->lpszName))
        {
            RtlCreateUnicodeStringFromAsciiz(&usBuffer,cbtcwA->lpcs->lpszName);
            csW.lpszName = usBuffer.Buffer;
        }
        if (HIWORD(cbtcwA->lpcs->lpszClass))
        {
            RtlCreateUnicodeStringFromAsciiz(&usBuffer,cbtcwA->lpcs->lpszName);
            csW.lpszClass = usBuffer.Buffer;
        }
        ret = proc( code, wparam, (LPARAM)&cbtcwW );
        cbtcwA->hwndInsertAfter = cbtcwW.hwndInsertAfter;
        if (HIWORD(csW.lpszName)) HeapFree( GetProcessHeap(), 0, (LPWSTR)csW.lpszName );
        if (HIWORD(csW.lpszClass)) HeapFree( GetProcessHeap(), 0, (LPWSTR)csW.lpszClass );
    }
    return ret;
}


/***********************************************************************
 *		call_hook_WtoA
 */
static LRESULT call_hook_WtoA( HOOKPROC proc, INT id, INT code, WPARAM wparam, LPARAM lparam )
{
    LRESULT ret;

    if (id != WH_CBT || code != HCBT_CREATEWND) ret = proc( code, wparam, lparam );
    else
    {
        CBT_CREATEWNDW *cbtcwW = (CBT_CREATEWNDW *)lparam;
        CBT_CREATEWNDA cbtcwA;
        CREATESTRUCTA csA;

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
    }
    return ret;
}


/***********************************************************************
 *		call_hook
 */
static LRESULT call_hook( HOOKPROC proc, INT id, INT code, WPARAM wparam, LPARAM lparam,
                          BOOL prev_unicode, BOOL next_unicode )
{
    LRESULT ret;

    if (TRACE_ON(relay))
        DPRINTF( "%08lx:Call hook proc %p (id=%s,code=%x,wp=%08x,lp=%08lx)\n",
                 GetCurrentThreadId(), proc, hook_names[id-WH_MINHOOK], code, wparam, lparam );

    if (!prev_unicode == !next_unicode) ret = proc( code, wparam, lparam );
    else if (prev_unicode) ret = call_hook_WtoA( proc, id, code, wparam, lparam );
    else ret = call_hook_AtoW( proc, id, code, wparam, lparam );

    if (TRACE_ON(relay))
        DPRINTF( "%08lx:Ret  hook proc %p (id=%s,code=%x,wp=%08x,lp=%08lx) retval=%08lx\n",
                 GetCurrentThreadId(), proc, hook_names[id-WH_MINHOOK], code, wparam, lparam, ret );

    return ret;
}


/***********************************************************************
 *		get_hook_proc
 *
 * Retrieve the hook procedure real value for a module-relative proc
 */
static HOOKPROC get_hook_proc( HOOKPROC proc, const WCHAR *module )
{
    HMODULE mod;

    if (!(mod = GetModuleHandleW(module)))
    {
        TRACE( "loading %s\n", debugstr_w(module) );
        /* FIXME: the library will never be freed */
        if (!(mod = LoadLibraryW(module))) return NULL;
    }
    return (HOOKPROC)((char *)mod + (ULONG_PTR)proc);
}


/***********************************************************************
 *		HOOK_CallHooks
 */
LRESULT HOOK_CallHooks( INT id, INT code, WPARAM wparam, LPARAM lparam, BOOL unicode )
{
    MESSAGEQUEUE *queue = QUEUE_Current();
    HOOKPROC proc = NULL;
    HHOOK prev;
    WCHAR module[MAX_PATH];
    BOOL unicode_hook = FALSE;
    LRESULT ret = 0;

    if (!queue) return 0;
    prev = queue->hook;
    SERVER_START_REQ( start_hook_chain )
    {
        req->id = id;
        wine_server_set_reply( req, module, sizeof(module)-sizeof(WCHAR) );
        if (!wine_server_call_err( req ))
        {
            module[wine_server_reply_size(req) / sizeof(WCHAR)] = 0;
            queue->hook = reply->handle;
            proc = reply->proc;
            unicode_hook = reply->unicode;
        }
    }
    SERVER_END_REQ;

    if (proc)
    {
        TRACE( "calling hook %p %s code %x wp %x lp %lx module %s\n",
               proc, hook_names[id-WH_MINHOOK], code, wparam, lparam, debugstr_w(module) );

        if (!module[0] || (proc = get_hook_proc( proc, module )) != NULL)
        {
            int locks = WIN_SuspendWndsLock();
            ret = call_hook( proc, id, code, wparam, lparam, unicode, unicode_hook );
            WIN_RestoreWndsLock( locks );
        }

        SERVER_START_REQ( finish_hook_chain )
        {
            req->id = id;
            wine_server_call( req );
        }
        SERVER_END_REQ;
    }

    queue->hook = prev;
    return ret;
}


/***********************************************************************
 *           HOOK_IsHooked
 */
BOOL HOOK_IsHooked( INT id )
{
    return TRUE;  /* FIXME */
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
 *		SetWindowsHookExA (USER32.@)
 */
HHOOK WINAPI SetWindowsHookExA( INT id, HOOKPROC proc, HINSTANCE inst, DWORD tid )
{
    return set_windows_hook( id, proc, inst, tid, FALSE );
}

/***********************************************************************
 *		SetWindowsHookExW (USER32.@)
 */
HHOOK WINAPI SetWindowsHookExW( INT id, HOOKPROC proc, HINSTANCE inst, DWORD tid )
{
    return set_windows_hook( id, proc, inst, tid, TRUE );
}


/***********************************************************************
 *		UnhookWindowsHook (USER32.@)
 */
BOOL WINAPI UnhookWindowsHook( INT id, HOOKPROC proc )
{
    BOOL ret;

    TRACE( "%s %p\n", hook_names[id-WH_MINHOOK], proc );

    SERVER_START_REQ( remove_hook )
    {
        req->handle = 0;
        req->id   = id;
        req->proc = proc;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}



/***********************************************************************
 *		UnhookWindowsHookEx (USER32.@)
 */
BOOL WINAPI UnhookWindowsHookEx( HHOOK hhook )
{
    BOOL ret;

    TRACE( "%p\n", hhook );

    SERVER_START_REQ( remove_hook )
    {
        req->handle = hhook;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *		CallNextHookEx (USER32.@)
 */
LRESULT WINAPI CallNextHookEx( HHOOK hhook, INT code, WPARAM wparam, LPARAM lparam )
{
    MESSAGEQUEUE *queue = QUEUE_Current();
    HHOOK prev;
    WCHAR module[MAX_PATH];
    HOOKPROC proc = NULL;
    INT id = 0;
    BOOL prev_unicode = FALSE, next_unicode = FALSE;
    LRESULT ret = 0;

    if (!queue) return 0;
    prev = queue->hook;
    SERVER_START_REQ( get_next_hook )
    {
        req->handle = prev;
        wine_server_set_reply( req, module, sizeof(module)-sizeof(WCHAR) );
        if (!wine_server_call_err( req ))
        {
            module[wine_server_reply_size(req) / sizeof(WCHAR)] = 0;
            queue->hook  = reply->next;
            id           = reply->id;
            proc         = reply->proc;
            prev_unicode = reply->prev_unicode;
            next_unicode = reply->next_unicode;
        }
    }
    SERVER_END_REQ;

    if (proc)
    {
        TRACE( "calling hook %p %s code %x wp %x lp %lx module %s\n",
               proc, hook_names[id-WH_MINHOOK], code, wparam, lparam, debugstr_w(module) );

        if (!module[0] || (proc = get_hook_proc( proc, module )) != NULL)
            return call_hook( proc, id, code, wparam, lparam, prev_unicode, next_unicode );
    }
    queue->hook = prev;
    return ret;
}


/***********************************************************************
 *		CallMsgFilterA (USER32.@)
 */
BOOL WINAPI CallMsgFilterA( LPMSG msg, INT code )
{
    if (HOOK_CallHooks( WH_SYSMSGFILTER, code, 0, (LPARAM)msg, FALSE )) return TRUE;
    return HOOK_CallHooks( WH_MSGFILTER, code, 0, (LPARAM)msg, FALSE );
}


/***********************************************************************
 *		CallMsgFilterW (USER32.@)
 */
BOOL WINAPI CallMsgFilterW( LPMSG msg, INT code )
{
    if (HOOK_CallHooks( WH_SYSMSGFILTER, code, 0, (LPARAM)msg, TRUE )) return TRUE;
    return HOOK_CallHooks( WH_MSGFILTER, code, 0, (LPARAM)msg, TRUE );
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
    FIXME("(%ld,%ld,%p,%p,%ld,%ld,0x%08lx)-stub!\n", dwMin, dwMax, hModule,
          pfnProc, dwProcess, dwThread, dwFlags);
    return 0;
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
    FIXME("(%p)-stub!\n", hEventHook);

    return (hEventHook != 0);
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
void WINAPI NotifyWinEvent(DWORD dwEvent, HWND hWnd, LONG nId, LONG nChildId)
{
    FIXME("(%ld,%p,%ld,%ld)-stub!\n", dwEvent, hWnd, nId, nChildId);
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
