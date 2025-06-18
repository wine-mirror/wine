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
#include <assert.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "win32u_private.h"
#include "ntuser_private.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(class);
WINE_DECLARE_DEBUG_CHANNEL(win);

SYSTEM_BASIC_INFORMATION system_info;

#define MAX_WINPROCS  4096
#define WINPROC_PROC16  ((void *)1)  /* placeholder for 16-bit window procs */

typedef struct tagCLASS
{
    struct list  entry;         /* Entry in class list */
    UINT         style;         /* Class style */
    BOOL         local;         /* Local class? */
    WNDPROC      winproc;       /* Window procedure */
    INT          cbClsExtra;    /* Class extra bytes */
    INT          cbWndExtra;    /* Window extra bytes */
    struct dce  *dce;           /* Opaque pointer to class DCE */
    UINT_PTR     instance;      /* Module that created the task */
    HICON        hIcon;         /* Default icon */
    HICON        hIconSm;       /* Default small icon */
    HICON        hIconSmIntern; /* Internal small icon, derived from hIcon */
    HCURSOR      hCursor;       /* Default cursor */
    HBRUSH       hbrBackground; /* Default background */
    ATOM         atomName;      /* Name of the class */
    WCHAR        name[MAX_ATOM_LEN + 1];
    WCHAR       *basename;      /* Base name for redirected classes, pointer within 'name'. */
    struct client_menu_name menu_name; /* Default menu name */
} CLASS;

/* Built-in class descriptor */
struct builtin_class_descr
{
    const char *name;    /* class name */
    UINT       style;    /* class style */
    INT        extra;     /* window extra bytes */
    ULONG_PTR  cursor;    /* cursor id */
    HBRUSH     brush;     /* brush or system color */
    enum ntuser_client_procs proc;
};

typedef struct tagWINDOWPROC
{
    WNDPROC  procA;    /* ANSI window proc */
    WNDPROC  procW;    /* Unicode window proc */
} WINDOWPROC;

static WINDOWPROC winproc_array[MAX_WINPROCS];
static UINT winproc_used = NTUSER_NB_PROCS;
static pthread_mutex_t winproc_lock = PTHREAD_MUTEX_INITIALIZER;

static struct list class_list = LIST_INIT( class_list );

HINSTANCE user32_module = 0;

/* find an existing winproc for a given function and type */
/* FIXME: probably should do something more clever than a linear search */
static WINDOWPROC *find_winproc( WNDPROC func, BOOL ansi )
{
    unsigned int i;

    for (i = 0; i < NTUSER_NB_PROCS; i++)
    {
        /* match either proc, some apps confuse A and W */
        if (winproc_array[i].procA != func && winproc_array[i].procW != func) continue;
        return &winproc_array[i];
    }
    for ( ; i < winproc_used; i++)
    {
        if (ansi && winproc_array[i].procA != func) continue;
        if (!ansi && winproc_array[i].procW != func) continue;
        return &winproc_array[i];
    }
    return NULL;
}

/* return the window proc for a given handle, or NULL for an invalid handle,
 * or WINPROC_PROC16 for a handle to a 16-bit proc. */
static WINDOWPROC *get_winproc_ptr( WNDPROC handle )
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

/* Get a window procedure pointer that can be passed to the Windows program. */
WNDPROC get_winproc( WNDPROC proc, BOOL ansi )
{
    WINDOWPROC *ptr = get_winproc_ptr( proc );

    if (!ptr || ptr == WINPROC_PROC16) return proc;
    if (ansi)
    {
        if (ptr->procA) return ptr->procA;
        return proc;
    }
    else
    {
        if (ptr->procW) return ptr->procW;
        return proc;
    }
}

/* Return the window procedure type, or the default value if not a winproc handle. */
BOOL is_winproc_unicode( WNDPROC proc, BOOL def_val )
{
    WINDOWPROC *ptr = get_winproc_ptr( proc );

    if (!ptr) return def_val;
    if (ptr == WINPROC_PROC16) return FALSE;  /* 16-bit is always A */
    if (ptr->procA && ptr->procW) return def_val;  /* can be both */
    return ptr->procW != NULL;
}

void get_winproc_params( struct win_proc_params *params, BOOL fixup_ansi_dst )
{
    WINDOWPROC *proc = get_winproc_ptr( params->func );

    if (!proc)
    {
        params->procW = params->procA = NULL;
    }
    else if (proc == WINPROC_PROC16)
    {
        params->procW = params->procA = WINPROC_PROC16;
    }
    else
    {
        params->procA = proc->procA;
        params->procW = proc->procW;

        if (fixup_ansi_dst)
        {
            if (params->ansi)
            {
                if (params->procA) params->ansi_dst = TRUE;
                else if (params->procW) params->ansi_dst = FALSE;
            }
            else
            {
                if (params->procW) params->ansi_dst = FALSE;
                else if (params->procA) params->ansi_dst = TRUE;
            }
        }
    }

    if (!params->procA) params->procA = params->func;
    if (!params->procW) params->procW = params->func;
}

DLGPROC get_dialog_proc( DLGPROC ret, BOOL ansi )
{
    WINDOWPROC *proc;

    if (!(proc = get_winproc_ptr( (WNDPROC)ret ))) return ret;
    if (proc == WINPROC_PROC16) return WINPROC_PROC16;
    return (DLGPROC)(ansi ? proc->procA : proc->procW);
}

static void init_user(void)
{
    NtQuerySystemInformation( SystemBasicInformation, &system_info, sizeof(system_info), NULL );

    shared_session_init();
    gdi_init();
    sysparams_init();
    winstation_init();
    register_desktop_class();
}

/***********************************************************************
 *	     NtUserInitializeClientPfnArrays   (win32u.@)
 */
NTSTATUS WINAPI NtUserInitializeClientPfnArrays( const ntuser_client_func_ptr *client_procsA,
                                                 const ntuser_client_func_ptr *client_procsW,
                                                 const ntuser_client_func_ptr *client_workers,
                                                 HINSTANCE user_module )
{
    static pthread_once_t init_once = PTHREAD_ONCE_INIT;
    UINT i;

    for (i = 0; i < NTUSER_NB_PROCS; i++)
    {
        winproc_array[i].procA = client_procsA[i][0];
        winproc_array[i].procW = client_procsW[i][0];
    }
    user32_module = user_module;

    pthread_once( &init_once, init_user );
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           get_int_atom_value
 */
ATOM get_int_atom_value( UNICODE_STRING *name )
{
    const WCHAR *ptr = name->Buffer;
    const WCHAR *end = ptr + name->Length / sizeof(WCHAR);
    UINT ret = 0;

    if (IS_INTRESOURCE(ptr)) return LOWORD(ptr);

    if (*ptr++ != '#') return 0;
    while (ptr < end)
    {
        if (*ptr < '0' || *ptr > '9') return 0;
        ret = ret * 10 + *ptr++ - '0';
        if (ret > 0xffff) return 0;
    }
    return ret;
}

/***********************************************************************
 *           get_class_ptr
 */
static CLASS *get_class_ptr( HWND hwnd, BOOL write_access )
{
    WND *ptr = get_win_ptr( hwnd );

    if (ptr)
    {
        if (ptr != WND_OTHER_PROCESS && ptr != WND_DESKTOP) return ptr->class;
        if (!write_access) return OBJ_OTHER_PROCESS;

        /* modifying classes in other processes is not allowed */
        if (ptr == WND_DESKTOP || is_window( hwnd ))
        {
            RtlSetLastWin32Error( ERROR_ACCESS_DENIED );
            return NULL;
        }
    }
    RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
    return NULL;
}

/***********************************************************************
 *           release_class_ptr
 */
static void release_class_ptr( CLASS *ptr )
{
    user_unlock();
}

static CLASS *find_class( HINSTANCE module, UNICODE_STRING *name )
{
    ATOM atom = get_int_atom_value( name );
    ULONG_PTR instance = (UINT_PTR)module;
    CLASS *class;
    int is_win16;

    user_lock();
    LIST_FOR_EACH_ENTRY( class, &class_list, CLASS, entry )
    {
        if (atom)
        {
            if (class->atomName != atom) continue;
        }
        else
        {
            if (wcsnicmp( class->name, name->Buffer, name->Length / sizeof(WCHAR) ) ||
                class->name[name->Length / sizeof(WCHAR)]) continue;
        }
        is_win16 = !(class->instance >> 16);
        if (!instance || !class->local || class->instance == instance ||
            (!is_win16 && ((class->instance & ~0xffff) == (instance & ~0xffff))))
        {
            TRACE( "%s %lx -> %p\n", debugstr_us(name), instance, class );
            return class;
        }
    }
    user_unlock();
    return NULL;
}

/***********************************************************************
 *           get_class_winproc
 */
WNDPROC get_class_winproc( CLASS *class )
{
    return class->winproc;
}

/***********************************************************************
 *           get_class_dce
 */
struct dce *get_class_dce( CLASS *class )
{
    return class->dce;
}

/***********************************************************************
 *           set_class_dce
 */
struct dce *set_class_dce( CLASS *class, struct dce *dce )
{
    if (class->dce) return class->dce;  /* already set, don't change it */
    class->dce = dce;
    return dce;
}

/***********************************************************************
 *	     NtUserRegisterClassExWOW   (win32u.@)
 */
ATOM WINAPI NtUserRegisterClassExWOW( const WNDCLASSEXW *wc, UNICODE_STRING *name, UNICODE_STRING *version,
                                      struct client_menu_name *client_menu_name, DWORD fnid,
                                      DWORD flags, DWORD *wow )
{
    const BOOL is_builtin = fnid, ansi = flags;
    HINSTANCE instance;
    HICON sm_icon = 0;
    CLASS *class;
    ATOM atom;
    BOOL ret;

    /* create the desktop window to trigger builtin class registration */
    if (!is_builtin) get_desktop_window();

    if (wc->cbSize != sizeof(*wc) || wc->cbClsExtra < 0 || wc->cbWndExtra < 0 ||
        (!is_builtin && wc->hInstance == user32_module))  /* we can't register a class for user32 */
    {
         RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
         return 0;
    }
    if (!(instance = wc->hInstance)) instance = NtCurrentTeb()->Peb->ImageBaseAddress;

    TRACE( "name=%s hinst=%p style=0x%x clExtr=0x%x winExtr=0x%x\n",
           debugstr_us(name), instance, wc->style, wc->cbClsExtra, wc->cbWndExtra );

    /* Fix the extra bytes value */

    if (wc->cbClsExtra > 40)  /* Extra bytes are limited to 40 in Win32 */
        WARN( "Class extra bytes %d is > 40\n", wc->cbClsExtra);
    if (wc->cbWndExtra > 40)  /* Extra bytes are limited to 40 in Win32 */
        WARN("Win extra bytes %d is > 40\n", wc->cbWndExtra );

    if (!(class = calloc( 1, sizeof(CLASS) + wc->cbClsExtra ))) return 0;

    class->atomName = get_int_atom_value( name );
    class->basename = class->name;
    if (!class->atomName && name)
    {
        memcpy( class->name, name->Buffer, name->Length );
        class->name[name->Length / sizeof(WCHAR)] = 0;
        class->basename += version->Length / sizeof(WCHAR);
    }
    else
    {
        UNICODE_STRING str = { .MaximumLength = sizeof(class->name), .Buffer = class->name };
        NtUserGetAtomName( class->atomName, &str );
    }

    class->style      = wc->style;
    class->local      = !is_builtin && !(wc->style & CS_GLOBALCLASS);
    class->cbWndExtra = wc->cbWndExtra;
    class->cbClsExtra = wc->cbClsExtra;
    class->instance   = (UINT_PTR)instance;

    SERVER_START_REQ( create_class )
    {
        req->local      = class->local;
        req->style      = class->style;
        req->instance   = class->instance;
        req->extra      = class->cbClsExtra;
        req->win_extra  = class->cbWndExtra;
        req->client_ptr = wine_server_client_ptr( class );
        req->atom       = class->atomName;
        req->name_offset = version->Length / sizeof(WCHAR);
        if (!req->atom && name) wine_server_add_data( req, name->Buffer, name->Length );
        ret = !wine_server_call_err( req );
        class->atomName = reply->atom;
    }
    SERVER_END_REQ;
    if (!ret)
    {
        free( class );
        return 0;
    }

    /* Other non-null values must be set by caller */
    if (wc->hIcon && !wc->hIconSm)
        sm_icon = CopyImage( wc->hIcon, IMAGE_ICON,
                             get_system_metrics( SM_CXSMICON ),
                             get_system_metrics( SM_CYSMICON ),
                             LR_COPYFROMRESOURCE );

    user_lock();
    if (class->local) list_add_head( &class_list, &class->entry );
    else list_add_tail( &class_list, &class->entry );

    atom = class->atomName;

    TRACE( "name=%s->%s atom=%04x wndproc=%p hinst=%p bg=%p style=%08x clsExt=%d winExt=%d class=%p\n",
           debugstr_w(wc->lpszClassName), debugstr_us(name), atom, wc->lpfnWndProc, instance,
           wc->hbrBackground, wc->style, wc->cbClsExtra, wc->cbWndExtra, class );

    class->hIcon         = wc->hIcon;
    class->hIconSm       = wc->hIconSm;
    class->hIconSmIntern = sm_icon;
    class->hCursor       = wc->hCursor;
    class->hbrBackground = wc->hbrBackground;
    class->winproc       = alloc_winproc( wc->lpfnWndProc, ansi );
    if (client_menu_name) class->menu_name = *client_menu_name;
    release_class_ptr( class );
    return atom;
}

/***********************************************************************
 *	     NtUserUnregisterClass   (win32u.@)
 */
BOOL WINAPI NtUserUnregisterClass( UNICODE_STRING *name, HINSTANCE instance,
                                   struct client_menu_name *client_menu_name )
{
    CLASS *class = NULL;

    /* create the desktop window to trigger builtin class registration */
    get_desktop_window();

    SERVER_START_REQ( destroy_class )
    {
        req->instance = wine_server_client_ptr( instance );
        if (!(req->atom = get_int_atom_value( name )) && name->Length)
            wine_server_add_data( req, name->Buffer, name->Length );
        if (!wine_server_call_err( req )) class = wine_server_get_ptr( reply->client_ptr );
    }
    SERVER_END_REQ;
    if (!class) return FALSE;

    TRACE( "%p\n", class );

    user_lock();
    if (class->dce) free_dce( class->dce, 0 );
    list_remove( &class->entry );
    if (class->hbrBackground > (HBRUSH)(COLOR_GRADIENTINACTIVECAPTION + 1))
        NtGdiDeleteObjectApp( class->hbrBackground );
    *client_menu_name = class->menu_name;
    NtUserDestroyCursor( class->hIconSmIntern, 0 );
    free( class );
    user_unlock();
    return TRUE;
}

/***********************************************************************
 *	     NtUserGetClassInfo   (win32u.@)
 */
ATOM WINAPI NtUserGetClassInfoEx( HINSTANCE instance, UNICODE_STRING *name, WNDCLASSEXW *wc,
                                  struct client_menu_name *menu_name, BOOL ansi )
{
    static const WCHAR messageW[] = {'M','e','s','s','a','g','e'};
    CLASS *class;
    ATOM atom;

    /* create the desktop window to trigger builtin class registration */
    if (name->Buffer != (const WCHAR *)DESKTOP_CLASS_ATOM &&
        (IS_INTRESOURCE(name->Buffer) || name->Length != sizeof(messageW) ||
         wcsnicmp( name->Buffer, messageW, ARRAYSIZE(messageW) )))
        get_desktop_window();

    if (!(class = find_class( instance, name ))) return 0;

    if (wc)
    {
        wc->style         = class->style;
        wc->lpfnWndProc   = get_winproc( class->winproc, ansi );
        wc->cbClsExtra    = class->cbClsExtra;
        wc->cbWndExtra    = class->cbWndExtra;
        wc->hInstance     = (instance == user32_module) ? 0 : instance;
        wc->hIcon         = class->hIcon;
        wc->hIconSm       = class->hIconSm ? class->hIconSm : class->hIconSmIntern;
        wc->hCursor       = class->hCursor;
        wc->hbrBackground = class->hbrBackground;
        wc->lpszMenuName  = ansi ? (const WCHAR *)class->menu_name.nameA : class->menu_name.nameW;
        wc->lpszClassName = name->Buffer;
    }

    if (menu_name) *menu_name = class->menu_name;
    atom = class->atomName;
    release_class_ptr( class );
    return atom;
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
        RtlSetLastWin32Error( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    size = min( abi->NameLength, name->MaximumLength - sizeof(WCHAR) );
    if (size) memcpy( name->Buffer, abi->Name, size );
    name->Buffer[size / sizeof(WCHAR)] = 0;
    return size / sizeof(WCHAR);
}

/***********************************************************************
 *       NtUserRegisterWindowMessage   (win32u.@)
 */
ATOM WINAPI NtUserRegisterWindowMessage( UNICODE_STRING *name )
{
    RTL_ATOM atom;

    TRACE( "%s\n", debugstr_us(name) );

    if (!name)
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!set_ntstatus( NtAddAtom( name->Buffer, name->Length, &atom ) )) return 0;
    return atom;
}

/***********************************************************************
 *	     NtUserGetClassName   (win32u.@)
 */
INT WINAPI NtUserGetClassName( HWND hwnd, BOOL real, UNICODE_STRING *name )
{
    CLASS *class;
    int ret;

    TRACE( "%p %x %p\n", hwnd, real, name );

    if (name->MaximumLength <= sizeof(WCHAR))
    {
        RtlSetLastWin32Error( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    if (!(class = get_class_ptr( hwnd, FALSE ))) return 0;

    if (class == OBJ_OTHER_PROCESS)
    {
        ATOM atom = 0;

        SERVER_START_REQ( set_class_info )
        {
            req->window = wine_server_user_handle( hwnd );
            req->flags = 0;
            req->extra_offset = -1;
            req->extra_size = 0;
            if (!wine_server_call_err( req ))
                atom = reply->base_atom;
        }
        SERVER_END_REQ;

        return NtUserGetAtomName( atom, name );
    }

    ret = min( name->MaximumLength / sizeof(WCHAR) - 1, lstrlenW(class->basename) );
    if (ret) memcpy( name->Buffer, class->basename, ret * sizeof(WCHAR) );
    name->Buffer[ret] = 0;
    release_class_ptr( class );
    return ret;
}

/* Set class info with the wine server. */
static BOOL set_server_info( HWND hwnd, INT offset, LONG_PTR newval, UINT size )
{
    BOOL ret;

    SERVER_START_REQ( set_class_info )
    {
        req->window = wine_server_user_handle( hwnd );
        req->extra_offset = -1;
        switch(offset)
        {
        case GCW_ATOM:
            req->flags = SET_CLASS_ATOM;
            req->atom = LOWORD(newval);
            break;
        case GCL_STYLE:
            req->flags = SET_CLASS_STYLE;
            req->style = newval;
            break;
        case GCL_CBWNDEXTRA:
            req->flags = SET_CLASS_WINEXTRA;
            req->win_extra = newval;
            break;
        case GCLP_HMODULE:
            req->flags = SET_CLASS_INSTANCE;
            req->instance = wine_server_client_ptr( (void *)newval );
            break;
        default:
            assert( offset >= 0 );
            req->flags = SET_CLASS_EXTRA;
            req->extra_offset = offset;
            req->extra_size = size;
            if ( size == sizeof(LONG) )
            {
                LONG newlong = newval;
                memcpy( &req->extra_value, &newlong, sizeof(LONG) );
            }
            else
                memcpy( &req->extra_value, &newval, sizeof(LONG_PTR) );
            break;
        }
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

static ULONG_PTR set_class_long( HWND hwnd, INT offset, LONG_PTR newval, UINT size, BOOL ansi )
{
    ULONG_PTR retval = 0;
    HICON small_icon = 0;
    CLASS *class;

    if (!(class = get_class_ptr( hwnd, TRUE ))) return 0;

    if (offset >= 0)
    {
        if (set_server_info( hwnd, offset, newval, size ))
        {
            void *ptr = (char *)(class + 1) + offset;
            if ( size == sizeof(LONG) )
            {
                DWORD retdword;
                LONG newlong = newval;
                memcpy( &retdword, ptr, sizeof(DWORD) );
                memcpy( ptr, &newlong, sizeof(LONG) );
                retval = retdword;
            }
            else
            {
                memcpy( &retval, ptr, sizeof(ULONG_PTR) );
                memcpy( ptr, &newval, sizeof(LONG_PTR) );
            }
        }
    }
    else switch(offset)
    {
    case GCLP_MENUNAME:
        {
            struct client_menu_name *menu_name = (void *)newval;
            struct client_menu_name prev = class->menu_name;
            class->menu_name = *menu_name;
            *menu_name = prev;
            retval = 0;  /* Old value is now meaningless anyway */
            break;
        }
    case GCLP_WNDPROC:
        retval = (ULONG_PTR)get_winproc( class->winproc, ansi );
        class->winproc = alloc_winproc( (WNDPROC)newval, ansi );
        break;
    case GCLP_HBRBACKGROUND:
        retval = (ULONG_PTR)class->hbrBackground;
        class->hbrBackground = (HBRUSH)newval;
        break;
    case GCLP_HCURSOR:
        retval = (ULONG_PTR)class->hCursor;
        class->hCursor = (HCURSOR)newval;
        break;
    case GCLP_HICON:
        retval = (ULONG_PTR)class->hIcon;
        if (retval == newval) break;
        if (newval && !class->hIconSm)
        {
            release_class_ptr( class );

            small_icon = CopyImage( (HICON)newval, IMAGE_ICON,
                                    get_system_metrics( SM_CXSMICON ),
                                    get_system_metrics( SM_CYSMICON ),
                                    LR_COPYFROMRESOURCE );

            if (!(class = get_class_ptr( hwnd, TRUE )))
            {
                NtUserDestroyCursor( small_icon, 0 );
                return 0;
            }
            if (retval != HandleToUlong( class->hIcon ) || class->hIconSm)
            {
                /* someone beat us, restart */
                release_class_ptr( class );
                NtUserDestroyCursor( small_icon, 0 );
                return set_class_long( hwnd, offset, newval, size, ansi );
            }
        }
        if (class->hIconSmIntern) NtUserDestroyCursor( class->hIconSmIntern, 0 );
        class->hIcon = (HICON)newval;
        class->hIconSmIntern = small_icon;
        break;
    case GCLP_HICONSM:
        retval = (ULONG_PTR)class->hIconSm;
        if (retval == newval) break;
        if (retval && !newval && class->hIcon)
        {
            HICON icon = class->hIcon;
            release_class_ptr( class );

            small_icon = CopyImage( icon, IMAGE_ICON,
                                    get_system_metrics( SM_CXSMICON ),
                                    get_system_metrics( SM_CYSMICON ),
                                    LR_COPYFROMRESOURCE );

            if (!(class = get_class_ptr( hwnd, TRUE )))
            {
                NtUserDestroyCursor( small_icon, 0 );
                return 0;
            }
            if (class->hIcon != icon || !class->hIconSm)
            {
                /* someone beat us, restart */
                release_class_ptr( class );
                NtUserDestroyCursor( small_icon, 0 );
                return set_class_long( hwnd, offset, newval, size, ansi );
            }
        }
        if (class->hIconSmIntern) NtUserDestroyCursor( class->hIconSmIntern, 0 );
        class->hIconSm = (HICON)newval;
        class->hIconSmIntern = small_icon;
        break;
    case GCL_STYLE:
        if (!set_server_info( hwnd, offset, newval, size )) break;
        retval = class->style;
        class->style = newval;
        break;
    case GCL_CBWNDEXTRA:
        if (!set_server_info( hwnd, offset, newval, size )) break;
        retval = class->cbWndExtra;
        class->cbWndExtra = newval;
        break;
    case GCLP_HMODULE:
        if (!set_server_info( hwnd, offset, newval, size )) break;
        retval = class->instance;
        class->instance = newval;
        break;
    case GCW_ATOM:
        {
            UNICODE_STRING us;
            if (!set_server_info( hwnd, offset, newval, size )) break;
            retval = class->atomName;
            class->atomName = newval;
            us.Buffer = class->name;
            us.MaximumLength = sizeof(class->name);
            NtUserGetAtomName( newval, &us );
        }
        break;
    case GCL_CBCLSEXTRA:  /* cannot change this one */
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        break;
    default:
        RtlSetLastWin32Error( ERROR_INVALID_INDEX );
        break;
    }
    release_class_ptr( class );
    return retval;
}

/***********************************************************************
 *	     NtUserSetClassLong   (win32u.@)
 */
DWORD WINAPI NtUserSetClassLong( HWND hwnd, INT offset, LONG newval, BOOL ansi )
{
    return set_class_long( hwnd, offset, newval, sizeof(LONG), ansi );
}

/***********************************************************************
 *	     NtUserSetClassLongPtr   (win32u.@)
 */
ULONG_PTR WINAPI NtUserSetClassLongPtr( HWND hwnd, INT offset, LONG_PTR newval, BOOL ansi )
{
    return set_class_long( hwnd, offset, newval, sizeof(LONG_PTR), ansi );
}

/***********************************************************************
 *	     NtUserSetClassWord   (win32u.@)
 */
WORD WINAPI NtUserSetClassWord( HWND hwnd, INT offset, WORD newval )
{
    CLASS *class;
    WORD retval = 0;

    if (offset < 0) return NtUserSetClassLong( hwnd, offset, (DWORD)newval, TRUE );

    if (!(class = get_class_ptr( hwnd, TRUE ))) return 0;

    SERVER_START_REQ( set_class_info )
    {
        req->window = wine_server_user_handle( hwnd );
        req->flags = SET_CLASS_EXTRA;
        req->extra_offset = offset;
        req->extra_size = sizeof(newval);
        memcpy( &req->extra_value, &newval, sizeof(newval) );
        if (!wine_server_call_err( req ))
        {
            void *ptr = (char *)(class + 1) + offset;
            memcpy( &retval, ptr, sizeof(retval) );
            memcpy( ptr, &newval, sizeof(newval) );
        }
    }
    SERVER_END_REQ;
    release_class_ptr( class );
    return retval;
}

static ULONG_PTR get_class_long_size( HWND hwnd, INT offset, UINT size, BOOL ansi )
{
    CLASS *class;
    ULONG_PTR retvalue = 0;

    if (!(class = get_class_ptr( hwnd, FALSE ))) return 0;

    if (class == OBJ_OTHER_PROCESS)
    {
        SERVER_START_REQ( set_class_info )
        {
            req->window = wine_server_user_handle( hwnd );
            req->flags = 0;
            req->extra_offset = (offset >= 0) ? offset : -1;
            req->extra_size = (offset >= 0) ? size : 0;
            if (!wine_server_call_err( req ))
            {
                switch(offset)
                {
                case GCLP_HBRBACKGROUND:
                case GCLP_HCURSOR:
                case GCLP_HICON:
                case GCLP_HICONSM:
                case GCLP_WNDPROC:
                case GCLP_MENUNAME:
                    FIXME( "offset %d not supported on other process window %p\n", offset, hwnd );
                    RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
                    break;
                case GCL_STYLE:
                    retvalue = reply->old_style;
                    break;
                case GCL_CBWNDEXTRA:
                    retvalue = reply->old_win_extra;
                    break;
                case GCL_CBCLSEXTRA:
                    retvalue = reply->old_extra;
                    break;
                case GCLP_HMODULE:
                    retvalue = (ULONG_PTR)wine_server_get_ptr( reply->old_instance );
                    break;
                case GCW_ATOM:
                    retvalue = reply->old_atom;
                    break;
                default:
                    if (offset >= 0)
                    {
                        if (size == sizeof(DWORD))
                        {
                            DWORD retdword;
                            memcpy( &retdword, &reply->old_extra_value, sizeof(DWORD) );
                            retvalue = retdword;
                        }
                        else
                            memcpy( &retvalue, &reply->old_extra_value,
                                    sizeof(ULONG_PTR) );
                    }
                    else RtlSetLastWin32Error( ERROR_INVALID_INDEX );
                    break;
                }
            }
        }
        SERVER_END_REQ;
        return retvalue;
    }

    if (offset >= 0)
    {
        if (offset <= class->cbClsExtra - size)
        {
            if (size == sizeof(DWORD))
            {
                DWORD retdword;
                memcpy( &retdword, (char *)(class + 1) + offset, sizeof(DWORD) );
                retvalue = retdword;
            }
            else
                memcpy( &retvalue, (char *)(class + 1) + offset, sizeof(ULONG_PTR) );
        }
        else
            RtlSetLastWin32Error( ERROR_INVALID_INDEX );
        release_class_ptr( class );
        return retvalue;
    }

    switch(offset)
    {
    case GCLP_HBRBACKGROUND:
        retvalue = (ULONG_PTR)class->hbrBackground;
        break;
    case GCLP_HCURSOR:
        retvalue = (ULONG_PTR)class->hCursor;
        break;
    case GCLP_HICON:
        retvalue = (ULONG_PTR)class->hIcon;
        break;
    case GCLP_HICONSM:
        retvalue = (ULONG_PTR)(class->hIconSm ? class->hIconSm : class->hIconSmIntern);
        break;
    case GCL_STYLE:
        retvalue = class->style;
        break;
    case GCL_CBWNDEXTRA:
        retvalue = class->cbWndExtra;
        break;
    case GCL_CBCLSEXTRA:
        retvalue = class->cbClsExtra;
        break;
    case GCLP_HMODULE:
        retvalue = class->instance;
        break;
    case GCLP_WNDPROC:
        retvalue = (ULONG_PTR)get_winproc( class->winproc, ansi );
        break;
    case GCLP_MENUNAME:
        retvalue = ansi ? (ULONG_PTR)class->menu_name.nameA : (ULONG_PTR)class->menu_name.nameW;
        break;
    case GCW_ATOM:
        retvalue = class->atomName;
        break;
    default:
        RtlSetLastWin32Error( ERROR_INVALID_INDEX );
        break;
    }
    release_class_ptr( class );
    return retvalue;
}

DWORD get_class_long( HWND hwnd, INT offset, BOOL ansi )
{
    return get_class_long_size( hwnd, offset, sizeof(DWORD), ansi );
}

ULONG_PTR get_class_long_ptr( HWND hwnd, INT offset, BOOL ansi )
{
    return get_class_long_size( hwnd, offset, sizeof(ULONG_PTR), ansi );
}

WORD get_class_word( HWND hwnd, INT offset )
{
    CLASS *class;
    WORD retvalue = 0;

    if (offset < 0) return get_class_long( hwnd, offset, TRUE );

    if (!(class = get_class_ptr( hwnd, FALSE ))) return 0;

    if (class == OBJ_OTHER_PROCESS)
    {
        SERVER_START_REQ( set_class_info )
        {
            req->window = wine_server_user_handle( hwnd );
            req->flags = 0;
            req->extra_offset = offset;
            req->extra_size = sizeof(retvalue);
            if (!wine_server_call_err( req ))
                memcpy( &retvalue, &reply->old_extra_value, sizeof(retvalue) );
        }
        SERVER_END_REQ;
        return retvalue;
    }

    if (offset <= class->cbClsExtra - sizeof(WORD))
        memcpy( &retvalue, (char *)(class + 1) + offset, sizeof(retvalue) );
    else
        RtlSetLastWin32Error( ERROR_INVALID_INDEX );
    release_class_ptr( class );
    return retvalue;
}

BOOL needs_ime_window( HWND hwnd )
{
    static const WCHAR imeW[] = {'I','M','E',0};
    CLASS *class;
    BOOL ret;

    if (!(class = get_class_ptr( hwnd, FALSE ))) return FALSE;
    ret = !(class->style & CS_IME) && wcscmp( imeW, class->name );
    release_class_ptr( class );
    return ret;
}

static const struct builtin_class_descr desktop_builtin_class =
{
    .name = MAKEINTRESOURCEA(DESKTOP_CLASS_ATOM),
    .style = CS_DBLCLKS,
    .proc = NTUSER_WNDPROC_DESKTOP,
    .brush = (HBRUSH)(COLOR_BACKGROUND + 1),
};

static const struct builtin_class_descr message_builtin_class =
{
    .name = "Message",
    .proc = NTUSER_WNDPROC_MESSAGE,
};

static const struct builtin_class_descr builtin_classes[] =
{
    /* button */
    {
        .name = "Button",
        .style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
        .proc = NTUSER_WNDPROC_BUTTON,
        .extra = sizeof(UINT) + 2 * sizeof(HANDLE),
        .cursor = IDC_ARROW,
    },
    /* combo  */
    {
        .name = "ComboBox",
        .style = CS_PARENTDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
        .proc = NTUSER_WNDPROC_COMBO,
        .extra = sizeof(void *),
        .cursor = IDC_ARROW,
    },
    /* combolbox */
    {
        .name = "ComboLBox",
        .style = CS_DBLCLKS | CS_SAVEBITS,
        .proc = NTUSER_WNDPROC_COMBOLBOX,
        .extra = sizeof(void *),
        .cursor = IDC_ARROW,
    },
    /* dialog */
    {
        .name = MAKEINTRESOURCEA(DIALOG_CLASS_ATOM),
        .style = CS_SAVEBITS | CS_DBLCLKS,
        .proc = NTUSER_WNDPROC_DIALOG,
        .extra = DLGWINDOWEXTRA,
        .cursor = IDC_ARROW,
    },
    /* icon title */
    {
        .name = MAKEINTRESOURCEA(ICONTITLE_CLASS_ATOM),
        .proc = NTUSER_WNDPROC_ICONTITLE,
        .cursor = IDC_ARROW,
    },
    /* IME */
    {
        .name = "IME",
        .proc = NTUSER_WNDPROC_IME,
        .extra = 2 * sizeof(LONG_PTR),
        .cursor = IDC_ARROW,
    },
    /* listbox  */
    {
        .name = "ListBox",
        .style = CS_DBLCLKS,
        .proc = NTUSER_WNDPROC_LISTBOX,
        .extra = sizeof(void *),
        .cursor = IDC_ARROW,
    },
    /* menu */
    {
        .name = MAKEINTRESOURCEA(POPUPMENU_CLASS_ATOM),
        .style = CS_DROPSHADOW | CS_SAVEBITS | CS_DBLCLKS,
        .proc = NTUSER_WNDPROC_MENU,
        .extra = sizeof(HMENU),
        .cursor = IDC_ARROW,
        .brush = (HBRUSH)(COLOR_MENU + 1),
    },
    /* MDIClient */
    {
        .name = "MDIClient",
        .proc = NTUSER_WNDPROC_MDICLIENT,
        .extra = 2 * sizeof(void *),
        .cursor = IDC_ARROW,
        .brush = (HBRUSH)(COLOR_APPWORKSPACE + 1),
    },
    /* scrollbar */
    {
        .name = "ScrollBar",
        .style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
        .proc = NTUSER_WNDPROC_SCROLLBAR,
        .extra = sizeof(struct scroll_bar_win_data),
        .cursor = IDC_ARROW,
    },
    /* static */
    {
        .name = "Static",
        .style = CS_DBLCLKS | CS_PARENTDC,
        .proc = NTUSER_WNDPROC_STATIC,
        .extra = 2 * sizeof(HANDLE),
        .cursor = IDC_ARROW,
    },
};

/***********************************************************************
 *           register_builtin
 *
 * Register a builtin control class.
 * This allows having both ANSI and Unicode winprocs for the same class.
 */
static void register_builtin( const struct builtin_class_descr *descr )
{
    UNICODE_STRING name, version = { .Length = 0 };
    struct client_menu_name menu_name = { 0 };
    WCHAR nameW[64];
    WNDCLASSEXW class = {
        .cbSize = sizeof(class),
        .hInstance = user32_module,
        .style = descr->style,
        .cbWndExtra = descr->extra,
        .hbrBackground = descr->brush,
        .lpfnWndProc = BUILTIN_WINPROC( descr->proc ),
    };

    if (descr->cursor)
        class.hCursor = LoadImageW( 0, (const WCHAR *)descr->cursor, IMAGE_CURSOR,
                                    0, 0, LR_SHARED | LR_DEFAULTSIZE );

    if (IS_INTRESOURCE( descr->name ))
    {
        name.Buffer = (WCHAR *)descr->name;
        name.Length = name.MaximumLength = 0;
    }
    else
    {
        asciiz_to_unicode( nameW, descr->name );
        RtlInitUnicodeString( &name, nameW );
    }

    if (!NtUserRegisterClassExWOW( &class, &name, &version, &menu_name, 1, 0, NULL ) && class.hCursor)
        NtUserDestroyCursor( class.hCursor, 0 );
}

static void register_builtins(void)
{
    ULONG ret_len, i;
    void *ret_ptr;

    /* 64-bit Windows use sizeof(UINT64) for all processes, while 32-bit Windows use 6 for extra
     * bytes size. Civilization II depends on the size being 6, so we use that even in wow64. */
    const struct builtin_class_descr edit_class =
    {
        .name = "Edit",
        .style = CS_DBLCLKS | CS_PARENTDC,
        .proc = NTUSER_WNDPROC_EDIT,
        .extra = sizeof(void *) == 4 || NtCurrentTeb()->WowTebOffset ? 6 : sizeof(UINT64),
        .cursor = IDC_IBEAM,
    };

    for (i = 0; i < ARRAYSIZE(builtin_classes); i++) register_builtin( &builtin_classes[i] );
    register_builtin( &edit_class );
    KeUserModeCallback( NtUserInitBuiltinClasses, NULL, 0, &ret_ptr, &ret_len );
}

/***********************************************************************
 *           register_builtin_classes
 */
void register_builtin_classes(void)
{
    static pthread_once_t init_once = PTHREAD_ONCE_INIT;
    pthread_once( &init_once, register_builtins );
}

/***********************************************************************
 *           register_desktop_class
 */
void register_desktop_class(void)
{
    register_builtin( &desktop_builtin_class );
    register_builtin( &message_builtin_class );
}
