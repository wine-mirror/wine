/*
 * Windows hook functions
 *
 * Copyright 2002 Alexandre Julliard
 * Copyright 2005 Dmitry Timoshkov
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
 *       HCBT_SYSCOMMAND            OK
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

#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "win.h"
#include "user_private.h"
#include "wine/server.h"
#include "wine/asm.h"
#include "wine/debug.h"
#include "winternl.h"

WINE_DEFAULT_DEBUG_CHANNEL(hook);
WINE_DECLARE_DEBUG_CHANNEL(relay);

static struct user_api_hook original_user_api =
{
    USER_DefDlgProc,
    USER_ScrollBarDraw,
    USER_ScrollBarProc,
};
static struct user_api_hook hooked_user_api;
struct user_api_hook *user_api = &original_user_api;

struct hook_info
{
    INT id;
    void *proc;
    void *handle;
    DWORD pid, tid;
    BOOL prev_unicode, next_unicode;
    WCHAR module[MAX_PATH];
};

#define WH_WINEVENT (WH_MAXHOOK+1)

static const char * const hook_names[WH_WINEVENT - WH_MINHOOK + 1] =
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
    "WH_MOUSE_LL",
    "WH_WINEVENT"
};


/***********************************************************************
 *		set_windows_hook
 *
 * Implementation of SetWindowsHookExA and SetWindowsHookExW.
 */
static HHOOK set_windows_hook( INT id, HOOKPROC proc, HINSTANCE inst, DWORD tid, BOOL ansi )
{
    WCHAR module[MAX_PATH];
    UNICODE_STRING str;

    if (!inst)
    {
        RtlInitUnicodeString( &str, NULL );
    }
    else
    {
        size_t len = GetModuleFileNameW( inst, module, ARRAYSIZE(module) );
        if (!len || len >= ARRAYSIZE(module))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        str.Buffer = module;
        str.MaximumLength = str.Length = len * sizeof(WCHAR);
    }

    return NtUserSetWindowsHookEx( inst, &str, tid, id, proc, ansi );
}

#ifdef __i386__
/* Some apps pass a non-stdcall proc to SetWindowsHookExA,
 * so we need a small assembly wrapper to call the proc.
 */
extern LRESULT HOOKPROC_wrapper( HOOKPROC proc,
                                 INT code, WPARAM wParam, LPARAM lParam );
__ASM_GLOBAL_FUNC( HOOKPROC_wrapper,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp,%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %edi\n\t"
                   __ASM_CFI(".cfi_rel_offset %edi,-4\n\t")
                   "pushl %esi\n\t"
                   __ASM_CFI(".cfi_rel_offset %esi,-8\n\t")
                   "pushl %ebx\n\t"
                   __ASM_CFI(".cfi_rel_offset %ebx,-12\n\t")
                   "pushl 20(%ebp)\n\t"
                   "pushl 16(%ebp)\n\t"
                   "pushl 12(%ebp)\n\t"
                   "movl 8(%ebp),%eax\n\t"
                   "call *%eax\n\t"
                   "leal -12(%ebp),%esp\n\t"
                   "popl %ebx\n\t"
                   __ASM_CFI(".cfi_same_value %ebx\n\t")
                   "popl %esi\n\t"
                   __ASM_CFI(".cfi_same_value %esi\n\t")
                   "popl %edi\n\t"
                   __ASM_CFI(".cfi_same_value %edi\n\t")
                   "leave\n\t"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   "ret" )
#else
static inline LRESULT HOOKPROC_wrapper( HOOKPROC proc,
                                 INT code, WPARAM wParam, LPARAM lParam )
{
    return proc( code, wParam, lParam );
}
#endif  /* __i386__ */


/***********************************************************************
 *		call_hook_AtoW
 */
static LRESULT call_hook_AtoW( HOOKPROC proc, INT id, INT code, WPARAM wparam, LPARAM lparam )
{
    LRESULT ret;
    UNICODE_STRING usBuffer;
    if (id != WH_CBT || code != HCBT_CREATEWND)
        ret = HOOKPROC_wrapper( proc, code, wparam, lparam );
    else
    {
        CBT_CREATEWNDA *cbtcwA = (CBT_CREATEWNDA *)lparam;
        CBT_CREATEWNDW cbtcwW;
        CREATESTRUCTW csW;
        LPWSTR nameW = NULL;
        LPWSTR classW = NULL;

        cbtcwW.lpcs = &csW;
        cbtcwW.hwndInsertAfter = cbtcwA->hwndInsertAfter;
        csW = *(CREATESTRUCTW *)cbtcwA->lpcs;

        if (!IS_INTRESOURCE(cbtcwA->lpcs->lpszName))
        {
            RtlCreateUnicodeStringFromAsciiz(&usBuffer,cbtcwA->lpcs->lpszName);
            csW.lpszName = nameW = usBuffer.Buffer;
        }
        if (!IS_INTRESOURCE(cbtcwA->lpcs->lpszClass))
        {
            RtlCreateUnicodeStringFromAsciiz(&usBuffer,cbtcwA->lpcs->lpszClass);
            csW.lpszClass = classW = usBuffer.Buffer;
        }
        ret = HOOKPROC_wrapper( proc, code, wparam, (LPARAM)&cbtcwW );
        cbtcwA->hwndInsertAfter = cbtcwW.hwndInsertAfter;
        HeapFree( GetProcessHeap(), 0, nameW );
        HeapFree( GetProcessHeap(), 0, classW );
    }
    return ret;
}


/***********************************************************************
 *		call_hook_WtoA
 */
static LRESULT call_hook_WtoA( HOOKPROC proc, INT id, INT code, WPARAM wparam, LPARAM lparam )
{
    LRESULT ret;

    if (id != WH_CBT || code != HCBT_CREATEWND)
        ret = HOOKPROC_wrapper( proc, code, wparam, lparam );
    else
    {
        CBT_CREATEWNDW *cbtcwW = (CBT_CREATEWNDW *)lparam;
        CBT_CREATEWNDA cbtcwA;
        CREATESTRUCTA csA;
        int len;
        LPSTR nameA = NULL;
        LPSTR classA = NULL;

        cbtcwA.lpcs = &csA;
        cbtcwA.hwndInsertAfter = cbtcwW->hwndInsertAfter;
        csA = *(CREATESTRUCTA *)cbtcwW->lpcs;

        if (!IS_INTRESOURCE(cbtcwW->lpcs->lpszName)) {
            len = WideCharToMultiByte( CP_ACP, 0, cbtcwW->lpcs->lpszName, -1, NULL, 0, NULL, NULL );
            nameA = HeapAlloc( GetProcessHeap(), 0, len*sizeof(CHAR) );
            WideCharToMultiByte( CP_ACP, 0, cbtcwW->lpcs->lpszName, -1, nameA, len, NULL, NULL );
            csA.lpszName = nameA;
        }

        if (!IS_INTRESOURCE(cbtcwW->lpcs->lpszClass)) {
            len = WideCharToMultiByte( CP_ACP, 0, cbtcwW->lpcs->lpszClass, -1, NULL, 0, NULL, NULL );
            classA = HeapAlloc( GetProcessHeap(), 0, len*sizeof(CHAR) );
            WideCharToMultiByte( CP_ACP, 0, cbtcwW->lpcs->lpszClass, -1, classA, len, NULL, NULL );
            csA.lpszClass = classA;
        }

        ret = HOOKPROC_wrapper( proc, code, wparam, (LPARAM)&cbtcwA );
        cbtcwW->hwndInsertAfter = cbtcwA.hwndInsertAfter;
        HeapFree( GetProcessHeap(), 0, nameA );
        HeapFree( GetProcessHeap(), 0, classA );
    }
    return ret;
}


/***********************************************************************
 *		call_hook_proc
 */
static LRESULT call_hook_proc( HOOKPROC proc, INT id, INT code, WPARAM wparam, LPARAM lparam,
                               BOOL prev_unicode, BOOL next_unicode )
{
    LRESULT ret;

    TRACE_(relay)( "\1Call hook proc %p (id=%s,code=%x,wp=%08lx,lp=%08lx)\n",
                   proc, hook_names[id-WH_MINHOOK], code, wparam, lparam );

    if (!prev_unicode == !next_unicode) ret = proc( code, wparam, lparam );
    else if (prev_unicode) ret = call_hook_WtoA( proc, id, code, wparam, lparam );
    else ret = call_hook_AtoW( proc, id, code, wparam, lparam );

    TRACE_(relay)( "\1Ret  hook proc %p (id=%s,code=%x,wp=%08lx,lp=%08lx) retval=%08lx\n",
                   proc, hook_names[id-WH_MINHOOK], code, wparam, lparam, ret );

    return ret;
}


/***********************************************************************
 *		get_hook_proc
 *
 * Retrieve the hook procedure real value for a module-relative proc
 */
void *get_hook_proc( void *proc, const WCHAR *module, HMODULE *free_module )
{
    HMODULE mod;

    GetModuleHandleExW( 0, module, &mod );
    *free_module = mod;
    if (!mod)
    {
        TRACE( "loading %s\n", debugstr_w(module) );
        /* FIXME: the library will never be freed */
        if (!(mod = LoadLibraryExW(module, NULL, LOAD_WITH_ALTERED_SEARCH_PATH))) return NULL;
    }
    return (char *)mod + (ULONG_PTR)proc;
}


/***********************************************************************
 *		HOOK_CallHooks
 */
LRESULT HOOK_CallHooks( INT id, INT code, WPARAM wparam, LPARAM lparam, BOOL unicode )
{
    struct win_hook_params params;
    params.id = id;
    params.code = code;
    params.wparam = wparam;
    params.lparam = lparam;
    params.next_unicode = unicode;
    return NtUserCallOneParam( (UINT_PTR)&params, NtUserCallHooks );
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
    return set_windows_hook( id, proc, inst, tid, TRUE );
}

/***********************************************************************
 *		SetWindowsHookExW (USER32.@)
 */
HHOOK WINAPI SetWindowsHookExW( INT id, HOOKPROC proc, HINSTANCE inst, DWORD tid )
{
    return set_windows_hook( id, proc, inst, tid, FALSE );
}


/***********************************************************************
 *		UnhookWindowsHook (USER32.@)
 */
BOOL WINAPI UnhookWindowsHook( INT id, HOOKPROC proc )
{
    return NtUserCallTwoParam( id, (UINT_PTR)proc, NtUserUnhookWindowsHook );
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
 *  event_min [I] Lowest event handled by pfnProc
 *  event_max [I] Highest event handled by pfnProc
 *  inst      [I] DLL containing pfnProc
 *  proc      [I] Callback event hook function
 *  pid       [I] Process to get events from, or 0 for all processes
 *  tid       [I] Thread to get events from, or 0 for all threads
 *  flags     [I] Flags indicating the status of pfnProc
 *
 * RETURNS
 *  Success: A handle representing the hook.
 *  Failure: A NULL handle.
 */
HWINEVENTHOOK WINAPI SetWinEventHook(DWORD event_min, DWORD event_max,
                                     HMODULE inst, WINEVENTPROC proc,
                                     DWORD pid, DWORD tid, DWORD flags)
{
    WCHAR module[MAX_PATH];
    UNICODE_STRING str;
    DWORD len = 0;

    TRACE("%d,%d,%p,%p,%08x,%04x,%08x\n", event_min, event_max, inst,
          proc, pid, tid, flags);

    if (inst && (!(len = GetModuleFileNameW( inst, module, MAX_PATH )) || len >= MAX_PATH))
    {
        inst = 0;
        len = 0;
    }
    str.Buffer = module;
    str.Length = str.MaximumLength = len * sizeof(WCHAR);
    return NtUserSetWinEventHook( event_min, event_max, inst, &str, proc, pid, tid, flags );
}

BOOL WINAPI User32CallWinEventHook( const struct win_event_hook_params *params, ULONG size )
{
    WINEVENTPROC proc = params->proc;
    HMODULE free_module = 0;

    if (params->module[0] && !(proc = get_hook_proc( proc, params->module, &free_module ))) return FALSE;

    TRACE_(relay)( "\1Call winevent hook proc %p (hhook=%p,event=%x,hwnd=%p,object_id=%x,child_id=%x,tid=%04x,time=%x)\n",
                   proc, params->handle, params->event, params->hwnd, params->object_id,
                   params->child_id, params->tid, params->time );

    proc( params->handle, params->event, params->hwnd, params->object_id, params->child_id,
          params->tid, params->time );

    TRACE_(relay)( "\1Ret  winevent hook proc %p (hhook=%p,event=%x,hwnd=%p,object_id=%x,child_id=%x,tid=%04x,time=%x)\n",
                   proc, params->handle, params->event, params->hwnd, params->object_id,
                   params->child_id, params->tid, params->time );

    if (free_module) FreeLibrary( free_module );
    return TRUE;
}

BOOL WINAPI User32CallWindowsHook( const struct win_hook_params *params, ULONG size )
{
    HOOKPROC proc = params->proc;
    HMODULE free_module = 0;
    LRESULT ret;

    if (params->module[0] && !(proc = get_hook_proc( proc, params->module, &free_module ))) return FALSE;

    ret = call_hook_proc( proc, params->id, params->code, params->wparam, params->lparam,
                          params->prev_unicode, params->next_unicode );

    if (free_module) FreeLibrary( free_module );
    return ret;
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
    /* FIXME: Needed by Office 2007 installer */
    WARN("(%d)-stub!\n", dwEvent);
    return TRUE;
}

/* Undocumented RegisterUserApiHook() */
BOOL WINAPI RegisterUserApiHook(const struct user_api_hook *new, struct user_api_hook *old)
{
    if (!new)
        return FALSE;

    USER_Lock();
    hooked_user_api = *new;
    user_api = &hooked_user_api;
    if (old)
        *old = original_user_api;
    USER_Unlock();
    return TRUE;
}

/* Undocumented UnregisterUserApiHook() */
void WINAPI UnregisterUserApiHook(void)
{
    InterlockedExchangePointer((void **)&user_api, &original_user_api);
}
