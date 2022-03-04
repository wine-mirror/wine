/*
 * Window classes functions
 *
 * Copyright 1993, 1996, 2003 Alexandre Julliard
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 Juergen Schmied (jsch)
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

#if 0
#pragma makedep unix
#endif

#include <pthread.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "win32u_private.h"
#include "ntuser_private.h"
#include "wine/debug.h"

WINE_DECLARE_DEBUG_CHANNEL(win);

#define MAX_WINPROCS  4096
#define WINPROC_PROC16  ((WINDOWPROC *)1)  /* placeholder for 16-bit window procs */

static WINDOWPROC winproc_array[MAX_WINPROCS];
static UINT winproc_used = NB_BUILTIN_WINPROCS;
static pthread_mutex_t winproc_lock = PTHREAD_MUTEX_INITIALIZER;

/* find an existing winproc for a given function and type */
/* FIXME: probably should do something more clever than a linear search */
static WINDOWPROC *find_winproc( WNDPROC func, BOOL ansi )
{
    unsigned int i;

    for (i = 0; i < NB_BUILTIN_AW_WINPROCS; i++)
    {
        /* match either proc, some apps confuse A and W */
        if (winproc_array[i].procA != func && winproc_array[i].procW != func) continue;
        return &winproc_array[i];
    }
    for (i = NB_BUILTIN_AW_WINPROCS; i < winproc_used; i++)
    {
        if (ansi && winproc_array[i].procA != func) continue;
        if (!ansi && winproc_array[i].procW != func) continue;
        return &winproc_array[i];
    }
    return NULL;
}

/* return the window proc for a given handle, or NULL for an invalid handle,
 * or WINPROC_PROC16 for a handle to a 16-bit proc. */
WINDOWPROC *get_winproc_ptr( WNDPROC handle )
{
    UINT index = LOWORD(handle);
    if ((ULONG_PTR)handle >> 16 != WINPROC_HANDLE) return NULL;
    if (index >= MAX_WINPROCS) return WINPROC_PROC16;
    if (index >= winproc_used) return NULL;
    return &winproc_array[index];
}

/* create a handle for a given window proc */
static inline WNDPROC proc_to_handle( WINDOWPROC *proc )
{
    return (WNDPROC)(ULONG_PTR)((proc - winproc_array) | (WINPROC_HANDLE << 16));
}

/* allocate and initialize a new winproc */
static inline WINDOWPROC *alloc_winproc_ptr( WNDPROC func, BOOL ansi )
{
    WINDOWPROC *proc;

    /* check if the function is already a win proc */
    if (!func) return NULL;
    if ((proc = get_winproc_ptr( func ))) return proc;

    pthread_mutex_lock( &winproc_lock );

    /* check if we already have a winproc for that function */
    if (!(proc = find_winproc( func, ansi )))
    {
        if (winproc_used < MAX_WINPROCS)
        {
            proc = &winproc_array[winproc_used++];
            if (ansi) proc->procA = func;
            else proc->procW = func;
            TRACE_(win)( "allocated %p for %c %p (%d/%d used)\n",
                         proc_to_handle(proc), ansi ? 'A' : 'W', func,
                         winproc_used, MAX_WINPROCS );
        }
        else WARN_(win)( "too many winprocs, cannot allocate one for %p\n", func );
    }
    else TRACE_(win)( "reusing %p for %p\n", proc_to_handle(proc), func );

    pthread_mutex_unlock( &winproc_lock );
    return proc;
}

/**********************************************************************
 *	     alloc_winproc
 *
 * Allocate a window procedure for a window or class.
 *
 * Note that allocated winprocs are never freed; the idea is that even if an app creates a
 * lot of windows, it will usually only have a limited number of window procedures, so the
 * array won't grow too large, and this way we avoid the need to track allocations per window.
 */
WNDPROC alloc_winproc( WNDPROC func, BOOL ansi )
{
    WINDOWPROC *proc;

    if (!(proc = alloc_winproc_ptr( func, ansi ))) return func;
    if (proc == WINPROC_PROC16) return func;
    return proc_to_handle( proc );
}

/***********************************************************************
 *	     NtUserInitializeClientPfnArrays   (win32u.@)
 */
NTSTATUS WINAPI NtUserInitializeClientPfnArrays( const struct user_client_procs *client_procsA,
                                                 const struct user_client_procs *client_procsW,
                                                 const void *client_workers, HINSTANCE user_module )
{
    winproc_array[WINPROC_BUTTON].procA = client_procsA->pButtonWndProc;
    winproc_array[WINPROC_BUTTON].procW = client_procsW->pButtonWndProc;
    winproc_array[WINPROC_COMBO].procA = client_procsA->pComboWndProc;
    winproc_array[WINPROC_COMBO].procW = client_procsW->pComboWndProc;
    winproc_array[WINPROC_DEFWND].procA = client_procsA->pDefWindowProc;
    winproc_array[WINPROC_DEFWND].procW = client_procsW->pDefWindowProc;
    winproc_array[WINPROC_DIALOG].procA = client_procsA->pDefDlgProc;
    winproc_array[WINPROC_DIALOG].procW = client_procsW->pDefDlgProc;
    winproc_array[WINPROC_EDIT].procA = client_procsA->pEditWndProc;
    winproc_array[WINPROC_EDIT].procW = client_procsW->pEditWndProc;
    winproc_array[WINPROC_LISTBOX].procA = client_procsA->pListBoxWndProc;
    winproc_array[WINPROC_LISTBOX].procW = client_procsW->pListBoxWndProc;
    winproc_array[WINPROC_MDICLIENT].procA = client_procsA->pMDIClientWndProc;
    winproc_array[WINPROC_MDICLIENT].procW = client_procsW->pMDIClientWndProc;
    winproc_array[WINPROC_SCROLLBAR].procA = client_procsA->pScrollBarWndProc;
    winproc_array[WINPROC_SCROLLBAR].procW = client_procsW->pScrollBarWndProc;
    winproc_array[WINPROC_STATIC].procA = client_procsA->pStaticWndProc;
    winproc_array[WINPROC_STATIC].procW = client_procsW->pStaticWndProc;
    winproc_array[WINPROC_IME].procA = client_procsA->pImeWndProc;
    winproc_array[WINPROC_IME].procW = client_procsW->pImeWndProc;
    winproc_array[WINPROC_DESKTOP].procA = client_procsA->pDesktopWndProc;
    winproc_array[WINPROC_DESKTOP].procW = client_procsW->pDesktopWndProc;
    winproc_array[WINPROC_ICONTITLE].procA = client_procsA->pIconTitleWndProc;
    winproc_array[WINPROC_ICONTITLE].procW = client_procsW->pIconTitleWndProc;
    winproc_array[WINPROC_MENU].procA = client_procsA->pPopupMenuWndProc;
    winproc_array[WINPROC_MENU].procW = client_procsW->pPopupMenuWndProc;
    winproc_array[WINPROC_MESSAGE].procA = client_procsA->pMessageWndProc;
    winproc_array[WINPROC_MESSAGE].procW = client_procsW->pMessageWndProc;

    return STATUS_SUCCESS;
}

/***********************************************************************
 *	     NtUserGetAtomName   (win32u.@)
 */
ULONG WINAPI NtUserGetAtomName( ATOM atom, UNICODE_STRING *name )
{
    char buf[sizeof(ATOM_BASIC_INFORMATION) + MAX_ATOM_LEN * sizeof(WCHAR)];
    ATOM_BASIC_INFORMATION *abi = (ATOM_BASIC_INFORMATION *)buf;
    UINT size;

    if (!set_ntstatus( NtQueryInformationAtom( atom, AtomBasicInformation,
                                               buf, sizeof(buf), NULL )))
        return 0;

    if (name->MaximumLength < sizeof(WCHAR))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    size = min( abi->NameLength, name->MaximumLength - sizeof(WCHAR) );
    if (size) memcpy( name->Buffer, abi->Name, size );
    name->Buffer[size / sizeof(WCHAR)] = 0;
    return size / sizeof(WCHAR);
}
