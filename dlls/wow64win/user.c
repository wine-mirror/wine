/*
 * WoW64 User functions
 *
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ntuser.h"
#include "shellapi.h"
#include "shlobj.h"
#include "wow64win_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wow);

typedef struct
{
    DWORD cbSize;
    DWORD fMask;
    DWORD dwStyle;
    UINT  cyMax;
    ULONG hbrBack;
    DWORD dwContextHelpID;
    ULONG dwMenuData;
} MENUINFO32;

typedef struct
{
    UINT    cbSize;
    UINT    fMask;
    UINT    fType;
    UINT    fState;
    UINT    wID;
    UINT32  hSubMenu;
    UINT32  hbmpChecked;
    UINT32  hbmpUnchecked;
    UINT32  dwItemData;
    UINT32  dwTypeData;
    UINT    cch;
    UINT32  hbmpItem;
} MENUITEMINFOW32;

typedef struct
{
    UINT32  hwnd;
    UINT    message;
    UINT32  wParam;
    UINT32  lParam;
    DWORD   time;
    POINT   pt;
} MSG32;

typedef struct
{
    DWORD dwType;
    DWORD dwSize;
    UINT32 hDevice;
    UINT32 wParam;
} RAWINPUTHEADER32;

typedef struct
{
    USHORT usUsagePage;
    USHORT usUsage;
    DWORD dwFlags;
    UINT32 hwndTarget;
} RAWINPUTDEVICE32;

typedef struct
{
    UINT32 hDevice;
    DWORD dwType;
} RAWINPUTDEVICELIST32;

typedef struct
{
    LONG    dx;
    LONG    dy;
    DWORD   mouseData;
    DWORD   dwFlags;
    DWORD   time;
    ULONG   dwExtraInfo;
} MOUSEINPUT32;

typedef struct
{
    WORD    wVk;
    WORD    wScan;
    DWORD   dwFlags;
    DWORD   time;
    ULONG   dwExtraInfo;
} KEYBDINPUT32;

typedef struct
{
    DWORD type;
    union
    {
        MOUSEINPUT32   mi;
        KEYBDINPUT32   ki;
        HARDWAREINPUT  hi;
    } DUMMYUNIONNAME;
} INPUT32;

typedef struct
{
    int x;
    int y;
    DWORD time;
    ULONG dwExtraInfo;
} MOUSEMOVEPOINT32;

typedef struct
{
    UINT32 hdc;
    BOOL   fErase;
    RECT   rcPaint;
    BOOL   fRestore;
    BOOL   fIncUpdate;
    BYTE   rgbReserved[32];
} PAINTSTRUCT32;

typedef struct
{
    ULONG lpCreateParams;
    ULONG hInstance;
    ULONG hMenu;
    ULONG hwndParent;
    INT   cy;
    INT   cx;
    INT   y;
    INT   x;
    LONG  style;
    ULONG lpszName;
    ULONG lpszClass;
    DWORD dwExStyle;
} CREATESTRUCT32;

typedef struct
{
    ULONG szClass;
    ULONG szTitle;
    ULONG hOwner;
    INT   x;
    INT   y;
    INT   cx;
    INT   cy;
    DWORD style;
    ULONG lParam;
} MDICREATESTRUCT32;

typedef struct
{
    ULONG hmenuIn;
    ULONG hmenuNext;
    ULONG hwndNext;
} MDINEXTMENU32;

typedef struct
{
    LONG  lResult;
    LONG  lParam;
    LONG  wParam;
    DWORD message;
    ULONG hwnd;
} CWPRETSTRUCT32;

typedef struct
{
    ULONG hwnd;
    ULONG hwndInsertAfter;
    INT   x;
    INT   y;
    INT   cx;
    INT   cy;
    UINT  flags;
} WINDOWPOS32;

typedef struct
{
    RECT  rgrc[3];
    ULONG lppos;
} NCCALCSIZE_PARAMS32;

typedef struct
{
    UINT  CtlType;
    UINT  CtlID;
    ULONG hwndItem;
    UINT  itemID1;
    ULONG itemData1;
    UINT  itemID2;
    ULONG itemData2;
    DWORD dwLocaleId;
} COMPAREITEMSTRUCT32;

typedef struct
{
    ULONG dwData;
    DWORD cbData;
    ULONG lpData;
} COPYDATASTRUCT32;

typedef struct
{
    UINT   cbSize;
    INT    iContextType;
    INT    iCtrlId;
    ULONG  hItemHandle;
    DWORD  dwContextId;
    POINT  MousePos;
} HELPINFO32;

typedef struct
{
    UINT  CtlType;
    UINT  CtlID;
    UINT  itemID;
    UINT  itemWidth;
    UINT  itemHeight;
    ULONG itemData;
} MEASUREITEMSTRUCT32;

typedef struct
{
    UINT  CtlType;
    UINT  CtlID;
    UINT  itemID;
    UINT  itemAction;
    UINT  itemState;
    ULONG hwndItem;
    ULONG hDC;
    RECT  rcItem;
    ULONG itemData;
} DRAWITEMSTRUCT32;

typedef struct
{
    DWORD cbSize;
    RECT  rcItem;
    RECT  rcButton;
    DWORD stateButton;
    ULONG hwndCombo;
    ULONG hwndItem;
    ULONG hwndList;
} COMBOBOXINFO32;

typedef struct
{
    ULONG lParam;
    ULONG wParam;
    UINT  message;
    ULONG hwnd;
} CWPSTRUCT32;

typedef struct
{
    POINT pt;
    ULONG hwnd;
    UINT  wHitTestCode;
    ULONG dwExtraInfo;
    DWORD mouseData;
} MOUSEHOOKSTRUCTEX32;

typedef struct
{
    POINT pt;
    DWORD mouseData;
    DWORD flags;
    DWORD time;
    ULONG dwExtraInfo;
} MSLLHOOKSTRUCT32;

typedef struct
{
    DWORD  vkCode;
    DWORD  scanCode;
    DWORD  flags;
    DWORD  time;
    ULONG  dwExtraInfo;
} KBDLLHOOKSTRUCT32;

typedef struct
{
    UINT  message;
    UINT  paramL;
    UINT  paramH;
    DWORD time;
    ULONG hwnd;
} EVENTMSG32;

typedef struct
{
    BOOL  fMouse;
    ULONG hWndActive;
} CBTACTIVATESTRUCT32;

typedef struct
{
    UINT  CtlType;
    UINT  CtlID;
    UINT  itemID;
    ULONG hwndItem;
    ULONG itemData;
} DELETEITEMSTRUCT32;

typedef struct
{
    UINT   cbSize;
    UINT   style;
    ULONG  lpfnWndProc;
    INT    cbClsExtra;
    INT    cbWndExtra;
    ULONG  hInstance;
    ULONG  hIcon;
    ULONG  hCursor;
    ULONG  hbrBackground;
    ULONG  lpszMenuName;
    ULONG  lpszClassName;
    ULONG  hIconSm;
} WNDCLASSEXW32;

struct client_menu_name32
{
    ULONG nameA;
    ULONG nameW;
    ULONG nameUS;
};

struct win_proc_params32
{
    ULONG func;
    ULONG hwnd;
    UINT msg;
    ULONG wparam;
    ULONG lparam;
    BOOL ansi;
    BOOL ansi_dst;
    enum wm_char_mapping mapping;
    ULONG dpi_context;
    ULONG procA;
    ULONG procW;
};

struct win_hook_params32
{
    ULONG proc;
    ULONG handle;
    DWORD pid;
    DWORD tid;
    int id;
    int code;
    ULONG wparam;
    ULONG lparam;
    BOOL prev_unicode;
    BOOL next_unicode;
    WCHAR module[1];
};

struct win_event_hook_params32
{
    DWORD event;
    ULONG hwnd;
    LONG object_id;
    LONG child_id;
    ULONG handle;
    DWORD tid;
    DWORD time;
    ULONG proc;
    WCHAR module[MAX_PATH];
};

struct draw_text_params32
{
    ULONG hdc;
    int count;
    RECT rect;
    UINT flags;
    WCHAR str[1];
};

struct unpack_dde_message_params32
{
    ULONG hwnd;
    UINT message;
    LONG wparam;
    LONG lparam;
    char data[1];
};

static MSG *msg_32to64( MSG *msg, const MSG32 *msg32 )
{
    if (!msg32) return NULL;

    msg->hwnd    = LongToHandle( msg32->hwnd );
    msg->message = msg32->message;
    msg->wParam  = msg32->wParam;
    msg->lParam  = msg32->lParam;
    msg->time    = msg32->time;
    msg->pt      = msg32->pt;
    return msg;
}

static MSG32 *msg_64to32( const MSG *msg64, MSG32 *msg32 )
{
    MSG32 msg;

    if (!msg32) return NULL;

    msg.hwnd    = HandleToLong( msg64->hwnd );
    msg.message = msg64->message;
    msg.wParam  = msg64->wParam;
    msg.lParam  = msg64->lParam;
    msg.time    = msg64->time;
    msg.pt      = msg64->pt;
    memcpy( msg32, &msg, sizeof(msg) );
    return msg32;
}

static struct client_menu_name *client_menu_name_32to64( struct client_menu_name *name,
                                                         const struct client_menu_name32 *name32 )
{
    if (!name32) return NULL;
    name->nameA = UlongToPtr( name32->nameA );
    name->nameW = UlongToPtr( name32->nameW );
    name->nameUS = UlongToPtr( name32->nameUS );
    return name;
}

static struct client_menu_name32 *client_menu_name_64to32( const struct client_menu_name *name64,
                                                           struct client_menu_name32 *name32 )
{
    if (name32)
    {
        struct client_menu_name32 name;
        name.nameA = PtrToUlong( name64->nameA );
        name.nameW = PtrToUlong( name64->nameW );
        name.nameUS = PtrToUlong( name64->nameUS );
        memcpy( name32, &name, sizeof(name) );
    }
    return name32;
}

static void win_proc_params_64to32( const struct win_proc_params *src, struct win_proc_params32 *dst )
{
    struct win_proc_params32 params;

    params.func = PtrToUlong( src->func );
    params.hwnd = HandleToUlong( src->hwnd );
    params.msg = src->msg;
    params.wparam = src->wparam;
    params.lparam = src->lparam;
    params.ansi = src->ansi;
    params.ansi_dst = src->ansi_dst;
    params.mapping = src->mapping;
    params.dpi_context = src->dpi_context;
    params.procA = PtrToUlong( src->procA );
    params.procW = PtrToUlong( src->procW );
    memcpy( dst, &params, sizeof(params) );
}

static void createstruct_32to64( const CREATESTRUCT32 *from, CREATESTRUCTW *to )

{
    to->lpCreateParams = UlongToPtr( from->lpCreateParams );
    to->hInstance      = UlongToPtr( from->hInstance );
    to->hMenu          = LongToHandle( from->hMenu );
    to->hwndParent     = LongToHandle( from->hwndParent );
    to->cy             = from->cy;
    to->cx             = from->cx;
    to->y              = from->y;
    to->x              = from->x;
    to->style          = from->style;
    to->dwExStyle      = from->dwExStyle;
    to->lpszName       = UlongToPtr( from->lpszName );
    to->lpszClass      = UlongToPtr( from->lpszClass );
}

static void createstruct_64to32( const CREATESTRUCTW *from, CREATESTRUCT32 *to )
{
    CREATESTRUCT32 cs;

    cs.lpCreateParams = PtrToUlong( from->lpCreateParams );
    cs.hInstance      = PtrToUlong( from->hInstance );
    cs.hMenu          = HandleToUlong( from->hMenu );
    cs.hwndParent     = HandleToUlong( from->hwndParent );
    cs.cy             = from->cy;
    cs.cx             = from->cx;
    cs.y              = from->y;
    cs.x              = from->x;
    cs.style          = from->style;
    cs.lpszName       = PtrToUlong( from->lpszName );
    cs.lpszClass      = PtrToUlong( from->lpszClass );
    cs.dwExStyle      = from->dwExStyle;
    memcpy( to, &cs, sizeof(cs) );
}

static void winpos_32to64( WINDOWPOS *dst, const WINDOWPOS32 *src )
{
    dst->hwnd            = LongToHandle( src->hwnd );
    dst->hwndInsertAfter = LongToHandle( src->hwndInsertAfter );
    dst->x               = src->x;
    dst->y               = src->y;
    dst->cx              = src->cx;
    dst->cy              = src->cy;
    dst->flags           = src->flags;
}

static void winpos_64to32( const WINDOWPOS *src, WINDOWPOS32 *dst )
{
    WINDOWPOS32 wp;

    wp.hwnd            = HandleToUlong( src->hwnd );
    wp.hwndInsertAfter = HandleToUlong( src->hwndInsertAfter );
    wp.x               = src->x;
    wp.y               = src->y;
    wp.cx              = src->cx;
    wp.cy              = src->cy;
    wp.flags           = src->flags;
    memcpy( dst, &wp, sizeof(wp) );
}

static PAINTSTRUCT *paintstruct_32to64( PAINTSTRUCT *ps, const PAINTSTRUCT32 *ps32 )
{
    if (!ps32) return NULL;
    ps->hdc        = ULongToHandle( ps32->hdc );
    ps->fErase     = ps32->fErase;
    ps->rcPaint    = ps32->rcPaint;
    ps->fRestore   = ps32->fRestore;
    ps->fIncUpdate = ps32->fIncUpdate;
    return ps;
}

static MOUSEHOOKSTRUCTEX32 *mousehookstruct_64to32( const MOUSEHOOKSTRUCTEX *hook64,
                                                    MOUSEHOOKSTRUCTEX32 *hook32 )
{
    MOUSEHOOKSTRUCTEX32 hook;

    if (!hook64) return NULL;

    hook.pt           = hook64->pt;
    hook.hwnd         = HandleToUlong( hook64->hwnd );
    hook.wHitTestCode = hook64->wHitTestCode;
    hook.dwExtraInfo  = hook64->dwExtraInfo;
    hook.mouseData    = hook64->mouseData;
    memcpy( hook32, &hook, sizeof(hook) );
    return hook32;
}

static NTSTATUS dispatch_callback( ULONG id, void *args, ULONG len )
{
    void *ret_ptr;
    ULONG ret_len;
    NTSTATUS status = Wow64KiUserCallbackDispatcher( id, args, len, &ret_ptr, &ret_len );
    return NtCallbackReturn( ret_ptr, ret_len, status );
}

static NTSTATUS WINAPI wow64_NtUserCallEnumDisplayMonitor( void *arg, ULONG size )
{
    struct enum_display_monitor_params *params = arg;
    struct
    {
        ULONG proc;
        ULONG monitor;
        ULONG hdc;
        RECT rect;
        ULONG lparam;
    } params32;

    params32.proc = PtrToUlong( params->proc );
    params32.monitor = HandleToUlong( params->monitor );
    params32.hdc = HandleToUlong( params->hdc );
    params32.rect = params->rect;
    params32.lparam = params->lparam;
    return dispatch_callback( NtUserCallEnumDisplayMonitor, &params32, sizeof(params32) );
}

static NTSTATUS WINAPI wow64_NtUserCallSendAsyncCallback( void *arg, ULONG size )
{
    struct send_async_params *params = arg;
    struct
    {
        ULONG callback;
        ULONG hwnd;
        UINT msg;
        ULONG data;
        ULONG result;
    } params32;

    params32.callback = PtrToUlong( params->callback );
    params32.hwnd = HandleToUlong( params->hwnd );
    params32.msg = params->msg;
    params32.data = params->data;
    params32.result = params->result;
    return dispatch_callback( NtUserCallSendAsyncCallback, &params32, sizeof(params32) );
}

static NTSTATUS WINAPI wow64_NtUserCallWinEventHook( void *arg, ULONG size )
{
    struct win_event_hook_params *params = arg;
    struct win_event_hook_params32 params32;

    params32.event = params->event;
    params32.hwnd = HandleToUlong( params->hwnd );
    params32.object_id = params->object_id;
    params32.child_id = params->child_id;
    params32.handle = HandleToUlong( params->handle );
    params32.tid = params->tid;
    params32.time = params->time;
    params32.proc = PtrToUlong( params->proc );

    size -= FIELD_OFFSET( struct win_event_hook_params, module );
    if (size) memcpy( params32.module, params->module, size );
    return dispatch_callback( NtUserCallWinEventHook, &params32,
                              FIELD_OFFSET( struct win_event_hook_params32, module ) + size);
}

static size_t packed_message_64to32( UINT message, WPARAM wparam,
                                     const void *params64, void *params32, size_t size )
{
    if (!size) return 0;

    switch (message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT32 *cs32 = params32;
            const CREATESTRUCTW *cs64 = params64;

            createstruct_64to32( cs64, cs32 );
            size -= sizeof(*cs64);
            if (size) memmove( cs32 + 1, cs64 + 1, size );
            return sizeof(*cs32) + size;
        }

    case WM_NCCALCSIZE:
        if (wparam)
        {
            NCCALCSIZE_PARAMS32 ncp32;
            const NCCALCSIZE_PARAMS *ncp64 = params64;

            ncp32.rgrc[0] = ncp64->rgrc[0];
            ncp32.rgrc[1] = ncp64->rgrc[1];
            ncp32.rgrc[2] = ncp64->rgrc[2];
            winpos_64to32( (const WINDOWPOS *)(ncp64 + 1),
                           (WINDOWPOS32 *)((const char *)params32 + sizeof(ncp32)) );
            memcpy( params32, &ncp32, sizeof(ncp32) );
            return sizeof(ncp32) + sizeof(WINDOWPOS32);
        }
        break;

    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT32 dis32;
            const DRAWITEMSTRUCT *dis64 = params64;

            dis32.CtlType       = dis64->CtlType;
            dis32.CtlID         = dis64->CtlID;
            dis32.itemID        = dis64->itemID;
            dis32.itemAction    = dis64->itemAction;
            dis32.itemState     = dis64->itemState;
            dis32.hwndItem      = HandleToLong( dis64->hwndItem );
            dis32.hDC           = HandleToUlong( dis64->hDC );
            dis32.itemData      = dis64->itemData;
            dis32.rcItem.left   = dis64->rcItem.left;
            dis32.rcItem.top    = dis64->rcItem.top;
            dis32.rcItem.right  = dis64->rcItem.right;
            dis32.rcItem.bottom = dis64->rcItem.bottom;
            memcpy( params32, &dis32, sizeof(dis32) );
            return sizeof(dis32);
        }

    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT32 mis32;
            const MEASUREITEMSTRUCT *mis64 = params64;

            mis32.CtlType    = mis64->CtlType;
            mis32.CtlID      = mis64->CtlID;
            mis32.itemID     = mis64->itemID;
            mis32.itemWidth  = mis64->itemWidth;
            mis32.itemHeight = mis64->itemHeight;
            mis32.itemData   = mis64->itemData;
            memcpy( params32, &mis32, sizeof(mis32) );
            return sizeof(mis32);
        }

    case WM_DELETEITEM:
        {
            DELETEITEMSTRUCT32 dis32;
            const DELETEITEMSTRUCT *dis64 = params64;

            dis32.CtlType  = dis64->CtlType;
            dis32.CtlID    = dis64->CtlID;
            dis32.itemID   = dis64->itemID;
            dis32.hwndItem = HandleToLong( dis64->hwndItem );
            dis32.itemData = dis64->itemData;
            memcpy( params32, &dis32, sizeof(dis32) );
            return sizeof(dis32);
        }

    case WM_COMPAREITEM:
        {
            COMPAREITEMSTRUCT32 cis32;
            const COMPAREITEMSTRUCT *cis64 = params64;

            cis32.CtlType    = cis64->CtlType;
            cis32.CtlID      = cis64->CtlID;
            cis32.hwndItem   = HandleToLong( cis64->hwndItem );
            cis32.itemID1    = cis64->itemID1;
            cis32.itemData1  = cis64->itemData1;
            cis32.itemID2    = cis64->itemID2;
            cis32.itemData2  = cis64->itemData2;
            cis32.dwLocaleId = cis64->dwLocaleId;
            memcpy( params32, &cis32, sizeof(cis32) );
            return sizeof(cis32);
        }

    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        winpos_64to32( params64, params32 );
        return sizeof(WINDOWPOS32);

    case WM_COPYDATA:
        {
            COPYDATASTRUCT32 cds32;
            const COPYDATASTRUCT *cds64 = params64;

            cds32.dwData = cds64->dwData;
            cds32.cbData = cds64->cbData;
            cds32.lpData = PtrToUlong( cds64->lpData );
            memcpy( params32, &cds32, sizeof(cds32) );
            size -= sizeof(cds32);
            if (size) memmove( (char *)params32 + sizeof(cds32), cds64 + 1, size );
            return sizeof(cds32) + size;
        }
    case WM_HELP:
        {
            HELPINFO32 hi32;
            const HELPINFO *hi64 = params64;

            hi32.cbSize       = sizeof(hi32);
            hi32.iContextType = hi64->iContextType;
            hi32.iCtrlId      = hi64->iCtrlId;
            hi32.hItemHandle  = HandleToLong( hi64->hItemHandle );
            hi32.dwContextId  = hi64->dwContextId;
            hi32.MousePos     = hi64->MousePos;
            memcpy( params32, &hi32, sizeof(hi32) );
            return sizeof(hi32);
        }

    case WM_GETDLGCODE:
        msg_64to32( params64, params32 );
        return sizeof(MSG32);

    case WM_NEXTMENU:
        {
            MDINEXTMENU32 *next32 = params32;
            const MDINEXTMENU *next64 = params64;

            next32->hmenuIn   = HandleToLong( next64->hmenuIn );
            next32->hmenuNext = HandleToLong( next64->hmenuNext );
            next32->hwndNext  = HandleToLong( next64->hwndNext );
            return sizeof(*next32);
        }

    case WM_MDICREATE:
        {
            MDICREATESTRUCT32 mcs32;
            const MDICREATESTRUCTW *mcs64 = params64;

            mcs32.szClass = PtrToUlong( mcs64->szClass );
            mcs32.szTitle = PtrToUlong( mcs64->szTitle );
            mcs32.hOwner  = HandleToLong( mcs64->hOwner );
            mcs32.x       = mcs64->x;
            mcs32.y       = mcs64->y;
            mcs32.cx      = mcs64->cx;
            mcs32.cy      = mcs64->cy;
            mcs32.style   = mcs64->style;
            mcs32.lParam  = mcs64->lParam;
            size -= sizeof(*mcs64);
            if (size) memmove( (char *)params32 + sizeof(mcs32), mcs64 + 1, size );
            memcpy( params32, &mcs32, sizeof(mcs32) );
            return sizeof(mcs32) + size;
        }

    case CB_GETCOMBOBOXINFO:
        {
            COMBOBOXINFO32 ci32;
            const COMBOBOXINFO *ci64 = params64;

            ci32.cbSize      = sizeof(ci32);
            ci32.rcItem      = ci64->rcItem;
            ci32.rcButton    = ci64->rcButton;
            ci32.stateButton = ci64->stateButton;
            ci32.hwndCombo   = HandleToLong( ci64->hwndCombo );
            ci32.hwndItem    = HandleToLong( ci64->hwndItem );
            ci32.hwndList    = HandleToLong( ci64->hwndList );
            memcpy( params32, &ci32, sizeof(ci32) );
            return sizeof(ci32);
        }
    }

    memmove( params32, params64, size );
    return size;
}

static size_t packed_result_32to64( UINT message, WPARAM wparam, const void *params32,
                                    size_t size, void *params64 )
{
    if (!size) return 0;

    switch (message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        if (size >= sizeof(CREATESTRUCT32))
        {
            const CREATESTRUCT32 *cs32 = params32;
            CREATESTRUCTW *cs64 = params64;

            cs64->lpCreateParams = UlongToPtr( cs32->lpCreateParams );
            cs64->hInstance      = UlongToPtr( cs32->hInstance );
            cs64->hMenu          = LongToHandle( cs32->hMenu );
            cs64->hwndParent     = LongToHandle( cs32->hwndParent );
            cs64->cy             = cs32->cy;
            cs64->cx             = cs32->cx;
            cs64->y              = cs32->y;
            cs64->x              = cs32->x;
            cs64->style          = cs32->style;
            cs64->dwExStyle      = cs32->dwExStyle;
            return sizeof(*cs64);
        }
        break;

    case WM_NCCALCSIZE:
        if (wparam)
        {
            const NCCALCSIZE_PARAMS32 *ncp32 = params32;
            NCCALCSIZE_PARAMS *ncp64 = params64;

            ncp64->rgrc[0] = ncp32->rgrc[0];
            ncp64->rgrc[1] = ncp32->rgrc[1];
            ncp64->rgrc[2] = ncp32->rgrc[2];
            winpos_32to64( (WINDOWPOS *)(ncp64 + 1), (const WINDOWPOS32 *)(ncp32 + 1) );
            return sizeof(*ncp64) + sizeof(WINDOWPOS);
        }
        break;

    case WM_MEASUREITEM:
        {
            const MEASUREITEMSTRUCT32 *mis32 = params32;
            MEASUREITEMSTRUCT *mis64 = params64;

            mis64->CtlType    = mis32->CtlType;
            mis64->CtlID      = mis32->CtlID;
            mis64->itemID     = mis32->itemID;
            mis64->itemWidth  = mis32->itemWidth;
            mis64->itemHeight = mis32->itemHeight;
            mis64->itemData   = mis32->itemData;
            return sizeof(*mis64);
        }

    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        winpos_32to64( params64, params32 );
        return sizeof(WINDOWPOS);

    case WM_NEXTMENU:
        {
            const MDINEXTMENU32 *next32 = params32;
            MDINEXTMENU *next64 = params64;

            next64->hmenuIn   = LongToHandle( next32->hmenuIn );
            next64->hmenuNext = LongToHandle( next32->hmenuNext );
            next64->hwndNext  = LongToHandle( next32->hwndNext );
            return sizeof(*next64);
        }

    case CB_GETCOMBOBOXINFO:
        {
            const COMBOBOXINFO32 *ci32 = params32;
            COMBOBOXINFO *ci64 = params64;

            ci64->cbSize      = sizeof(*ci32);
            ci64->rcItem      = ci32->rcItem;
            ci64->rcButton    = ci32->rcButton;
            ci64->stateButton = ci32->stateButton;
            ci64->hwndCombo   = LongToHandle( ci32->hwndCombo );
            ci64->hwndItem    = LongToHandle( ci32->hwndItem );
            ci64->hwndList    = LongToHandle( ci32->hwndList );
            return sizeof(*ci64);
        }

    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
    case WM_GETMINMAXINFO:
    case WM_STYLECHANGING:
    case SBM_SETSCROLLINFO:
    case SBM_GETSCROLLINFO:
    case SBM_GETSCROLLBARINFO:
    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
    case EM_SETRECT:
    case EM_GETRECT:
    case EM_SETRECTNP:
    case LB_GETITEMRECT:
    case CB_GETDROPPEDCONTROLRECT:
    case EM_GETLINE:
    case CB_GETLBTEXT:
    case LB_GETTEXT:
    case LB_GETSELITEMS:
    case WM_SIZING:
    case WM_MOVING:
    case WM_MDIGETACTIVE:
        break;

    default:
        return 0;
    }

    if (size) memcpy( params64, params32, size );
    return size;
}

static NTSTATUS WINAPI wow64_NtUserCallWinProc( void *arg, ULONG size )
{
    struct win_proc_params *params = arg;
    struct win_proc_params32 *params32 = arg;
    size_t lparam_size = 0, offset32 = sizeof(*params32);
    LRESULT result = 0;
    void *ret_ptr;
    ULONG ret_len;
    NTSTATUS status;

    win_proc_params_64to32( params, params32 );
    if (size > sizeof(*params))
    {
        const size_t offset64 = (sizeof(*params) + 15) & ~15;
        offset32 = (offset32 + 15) & ~15;
        lparam_size = packed_message_64to32( params32->msg, params32->wparam,
                                             (char *)params + offset64,
                                             (char *)params32 + offset32,
                                             size - offset64 );
    }

    status = Wow64KiUserCallbackDispatcher( NtUserCallWinProc, params32,
                                            offset32 + lparam_size,
                                            &ret_ptr, &ret_len );

    if (ret_len >= sizeof(LONG))
    {
        LRESULT *result_ptr = arg;
        result = *(LONG *)ret_ptr;
        ret_len = packed_result_32to64( params32->msg, params32->wparam, (LONG *)ret_ptr + 1,
                                        ret_len - sizeof(LONG), result_ptr + 1 );
        *result_ptr = result;
        return NtCallbackReturn( result_ptr, sizeof(*result_ptr) + ret_len, status );
    }

    return NtCallbackReturn( &result, sizeof(result), status );
}

static UINT hook_lparam_64to32( int id, int code, const void *lp, size_t size, void *lp32 )
{
    if (!size) return 0;

    switch (id)
    {
    case WH_SYSMSGFILTER:
    case WH_MSGFILTER:
    case WH_GETMESSAGE:
        msg_64to32( lp, lp32 );
        return sizeof(MSG32);

    case WH_CBT:
        switch (code)
        {
        case HCBT_CREATEWND:
            return packed_message_64to32( WM_CREATE, 0, lp, lp32, size );

        case HCBT_ACTIVATE:
            {
                const CBTACTIVATESTRUCT *cbt = lp;
                CBTACTIVATESTRUCT32 cbt32;
                cbt32.fMouse     = cbt->fMouse;
                cbt32.hWndActive = HandleToUlong( cbt->hWndActive );
                memcpy( lp32, &cbt32, sizeof(cbt32) );
                return sizeof(cbt32);
            }

        case HCBT_CLICKSKIPPED:
            mousehookstruct_64to32( lp, lp32 );
            return sizeof(MOUSEHOOKSTRUCTEX32);
        }
        break;

    case WH_CALLWNDPROC:
        {
            const CWPSTRUCT *cwp = lp;
            CWPSTRUCT32 cwp32;
            cwp32.lParam  = cwp->lParam;
            cwp32.wParam  = cwp->wParam;
            cwp32.message = cwp->message;
            cwp32.hwnd    = HandleToUlong( cwp->hwnd );
            memcpy( lp32, &cwp32, sizeof(cwp32) );
            if (size > sizeof(*cwp))
            {
                const size_t offset64 = (sizeof(*cwp) + 15) & ~15;
                const size_t offset32 = (sizeof(cwp32) + 15) & ~15;
                size = packed_message_64to32( cwp32.message, cwp32.wParam,
                                              (const char *)lp + offset64,
                                              (char *)lp32 + offset32, size - offset64 );
                return offset32 + size;
            }
            return sizeof(cwp32);
        }

    case WH_CALLWNDPROCRET:
        {
            const CWPRETSTRUCT *cwpret = lp;
            CWPRETSTRUCT32 cwpret32;
            cwpret32.lResult = cwpret->lResult;
            cwpret32.lParam  = cwpret->lParam;
            cwpret32.wParam  = cwpret->wParam;
            cwpret32.message = cwpret->message;
            cwpret32.hwnd    = HandleToUlong( cwpret->hwnd );
            memcpy( lp32, &cwpret32, sizeof(cwpret32) );
            if (size > sizeof(*cwpret))
            {
                const size_t offset64 = (sizeof(*cwpret) + 15) & ~15;
                const size_t offset32 = (sizeof(cwpret32) + 15) & ~15;
                size = packed_message_64to32( cwpret32.message, cwpret32.wParam,
                                              (const char *)lp + offset64,
                                              (char *)lp32 + offset32, size - offset64 );
                return offset32 + size;
            }
            return sizeof(cwpret32);
        }

    case WH_MOUSE:
        mousehookstruct_64to32( lp, lp32 );
        return sizeof(MOUSEHOOKSTRUCTEX32);

    case WH_MOUSE_LL:
        {
            const MSLLHOOKSTRUCT *hook = lp;
            MSLLHOOKSTRUCT32 hook32;
            hook32.pt          = hook->pt;
            hook32.mouseData   = hook->mouseData;
            hook32.flags       = hook->flags;
            hook32.time        = hook->time;
            hook32.dwExtraInfo = hook->dwExtraInfo;
            memcpy( lp32, &hook32, sizeof(hook32) );
            return sizeof(hook32);
        }

    case WH_KEYBOARD_LL:
        {
            const KBDLLHOOKSTRUCT *hook = lp;
            KBDLLHOOKSTRUCT32 hook32;
            hook32.vkCode      = hook->vkCode;
            hook32.scanCode    = hook->scanCode;
            hook32.flags       = hook->flags;
            hook32.time        = hook->time;
            hook32.dwExtraInfo = hook->dwExtraInfo;
            memcpy( lp32, &hook32, sizeof(hook32) );
            return sizeof(hook32);
        }

    case WH_JOURNALRECORD:
        {
            const EVENTMSG *event = lp;
            EVENTMSG32 event32;

            event32.message = event->message;
            event32.paramL = event->paramL;
            event32.paramH = event->paramH;
            event32.time = event->time;
            event32.hwnd = HandleToUlong( event->hwnd );
            memcpy( lp32, &event32, sizeof(event32) );
            return sizeof(event32);
        }
    }

    memmove( lp32, lp, size );
    return size;
}

static NTSTATUS WINAPI wow64_NtUserCallWindowsHook( void *arg, ULONG size )
{
    struct win_hook_params *params = arg;
    struct win_hook_params32 params32;
    UINT module_len, size32, offset;
    void *ret_ptr;
    LRESULT *result_ptr = arg;
    ULONG ret_len, ret_size = 0;
    NTSTATUS status;

    module_len = wcslen( params->module );
    size32 = FIELD_OFFSET( struct win_hook_params32, module[module_len + 1] );
    offset = FIELD_OFFSET( struct win_hook_params, module[module_len + 1] );

    params32.proc         = PtrToUlong( params->proc );
    params32.handle       = HandleToUlong( params->handle );
    params32.pid          = params->pid;
    params32.tid          = params->tid;
    params32.id           = params->id;
    params32.code         = params->code;
    params32.wparam       = params->wparam;
    params32.lparam       = params->lparam;
    params32.prev_unicode = params->prev_unicode;
    params32.next_unicode = params->next_unicode;
    memcpy( arg, &params32, FIELD_OFFSET( struct win_hook_params32, module ));
    memmove( ((struct win_hook_params32 *)arg)->module, params->module,
             (module_len + 1) * sizeof(WCHAR) );

    if (size > offset)
    {
        size32 = (size32 + 15) & ~15;
        offset = (offset + 15) & ~15;
        size32 += hook_lparam_64to32( params32.id, params32.code, (char *)params + offset,
                                      size - offset, (char *)arg + size32 );
    }

    status = Wow64KiUserCallbackDispatcher( NtUserCallWindowsHook, arg, size32, &ret_ptr, &ret_len );
    if (status || ret_len < sizeof(LONG)) return status;

    switch (params32.id)
    {
    case WH_SYSMSGFILTER:
    case WH_MSGFILTER:
    case WH_GETMESSAGE:
        if (ret_len == sizeof(MSG32) + sizeof(LONG))
        {
            msg_32to64( (MSG *)(result_ptr + 1), (MSG32 *)((LONG *)ret_ptr + 1) );
            ret_size = sizeof(MSG);
        }
        break;
    }
    *result_ptr = *(LONG *)ret_ptr;
    return NtCallbackReturn( result_ptr, sizeof(*result_ptr) + ret_size, status );
}

static NTSTATUS WINAPI wow64_NtUserCopyImage( void *arg, ULONG size )
{
    struct copy_image_params *params = arg;
    void *ret_ptr;
    ULONG ret_len;
    NTSTATUS status;
    struct
    {
        ULONG hwnd;
        UINT type;
        INT dx;
        INT dy;
        UINT flags;
    } params32;

    params32.hwnd = HandleToUlong( params->hwnd );
    params32.type = params->type;
    params32.dx = params->dx;
    params32.dy = params->dy;
    params32.flags = params->flags;
    status = Wow64KiUserCallbackDispatcher( NtUserCopyImage, &params32, sizeof(params32),
                                            &ret_ptr, &ret_len );
    if (!status && ret_len == sizeof(ULONG))
    {
        HANDLE handle = ULongToHandle( *(ULONG *)ret_ptr );
        return NtCallbackReturn( &handle, sizeof(handle), status );
    }
    return status;
}

static NTSTATUS WINAPI wow64_NtUserDrawNonClientButton( void *arg, ULONG size )
{
    struct draw_non_client_button_params *params = arg;
    struct
    {
        ULONG hwnd;
        ULONG hdc;
        enum NONCLIENT_BUTTON_TYPE type;
        RECT rect;
        BOOL down;
        BOOL grayed;
    } params32;

    params32.hwnd = HandleToUlong( params->hwnd );
    params32.hdc = HandleToUlong( params->hdc );
    params32.type = params->type;
    params32.rect = params->rect;
    params32.down = params->down;
    params32.grayed = params->grayed;
    return dispatch_callback( NtUserDrawNonClientButton, &params32, sizeof(params32) );
}

static NTSTATUS WINAPI wow64_NtUserDrawScrollBar( void *arg, ULONG size )
{
    struct draw_scroll_bar_params *params = arg;
    struct
    {
        ULONG hwnd;
        ULONG hdc;
        INT bar;
        UINT hit_test;
        struct
        {
            ULONG win;
            INT bar;
            INT thumb_pos;
            INT thumb_val;
            BOOL vertical;
            enum SCROLL_HITTEST hit_test;
        } tracking_info;
        BOOL arrows;
        BOOL interior;
        RECT rect;
        UINT enable_flags;
        INT arrow_size;
        INT thumb_pos;
        INT thumb_size;
        BOOL vertical;
    } params32;

    params32.hwnd = HandleToUlong( params->hwnd );
    params32.hdc = HandleToUlong( params->hdc );
    params32.bar = params->bar;
    params32.hit_test = params->hit_test;
    params32.tracking_info.win = HandleToUlong( params->tracking_info.win );
    params32.tracking_info.bar = params->tracking_info.bar;
    params32.tracking_info.thumb_pos = params->tracking_info.thumb_pos;
    params32.tracking_info.thumb_val = params->tracking_info.thumb_val;
    params32.tracking_info.vertical = params->tracking_info.vertical;
    params32.tracking_info.hit_test = params->tracking_info.hit_test;
    params32.arrows = params->arrows;
    params32.interior = params->interior;
    params32.rect = params->rect;
    params32.enable_flags = params->enable_flags;
    params32.arrow_size = params->arrow_size;
    params32.thumb_pos = params->thumb_pos;
    params32.thumb_size = params->thumb_size;
    params32.vertical = params->vertical;
    return dispatch_callback( NtUserDrawScrollBar, &params32, sizeof(params32) );
}

static NTSTATUS WINAPI wow64_NtUserDrawText( void *arg, ULONG size )
{
    struct draw_text_params *params = arg;
    struct draw_text_params32 *params32;
    ULONG offset = offsetof( struct draw_text_params, str ) - offsetof( struct draw_text_params32, str );

    params32 = (struct draw_text_params32 *)((char *)params + offset);
    params32->flags = params->flags;
    params32->rect = params->rect;
    params32->count = params->count;
    params32->hdc = HandleToUlong( params->hdc );
    return dispatch_callback( NtUserDrawText, params32, size - offset );
}

static NTSTATUS WINAPI wow64_NtUserFreeCachedClipboardData( void *arg, ULONG size )
{
    struct free_cached_data_params *params = arg;
    struct
    {
        UINT format;
        ULONG handle;
    } params32;

    params32.format = params->format;
    params32.handle = HandleToUlong( params->handle );
    return dispatch_callback( NtUserFreeCachedClipboardData, &params32, sizeof(params32) );
}

static NTSTATUS WINAPI wow64_NtUserImmProcessKey( void *arg, ULONG size )
{
    struct imm_process_key_params *params = arg;
    struct
    {
        ULONG hwnd;
        ULONG hkl;
        UINT  vkey;
        ULONG key_data;
    } params32;

    params32.hwnd = HandleToUlong( params->hwnd );
    params32.hkl = HandleToUlong( params->hkl );
    params32.vkey = params->vkey;
    params32.key_data = params->key_data;
    return dispatch_callback( NtUserImmProcessKey, &params32, sizeof(params32) );
}

static NTSTATUS WINAPI wow64_NtUserImmTranslateMessage( void *arg, ULONG size )
{
    struct imm_translate_message_params *params = arg;
    struct
    {
        LONG hwnd;
        UINT msg;
        LONG wparam;
        LONG key_data;
    } params32;

    params32.hwnd = HandleToLong( params->hwnd );
    params32.msg = params->msg;
    params32.wparam = params->wparam;
    params32.key_data = params->key_data;
    return dispatch_callback( NtUserImmTranslateMessage, &params32, sizeof(params32) );
}

static NTSTATUS WINAPI wow64_NtUserInitBuiltinClasses( void *arg, ULONG size )
{
    return dispatch_callback( NtUserInitBuiltinClasses, arg, size );
}

static NTSTATUS WINAPI wow64_NtUserLoadDriver( void *arg, ULONG size )
{
    return dispatch_callback( NtUserLoadDriver, arg, size );
}

static NTSTATUS WINAPI wow64_NtUserLoadImage( void *arg, ULONG size )
{
    struct load_image_params *params = arg;
    void *ret_ptr;
    ULONG ret_len;
    NTSTATUS status;
    struct
    {
        ULONG hinst;
        ULONG name;
        UINT type;
        INT dx;
        INT dy;
        UINT flags;
    } params32;

    params32.hinst = PtrToUlong( params->hinst );
    params32.name = PtrToUlong( params->name );
    params32.type = params->type;
    params32.dx = params->dx;
    params32.dy = params->dy;
    params32.flags = params->flags;
    status = Wow64KiUserCallbackDispatcher( NtUserLoadImage, &params32, sizeof(params32),
                                            &ret_ptr, &ret_len );
    if (!status && ret_len == sizeof(ULONG))
    {
        HANDLE handle = ULongToHandle( *(ULONG *)ret_ptr );
        return NtCallbackReturn( &handle, sizeof(handle), status );
    }
    return status;
}

static NTSTATUS WINAPI wow64_NtUserLoadSysMenu( void *arg, ULONG size )
{
    void *ret_ptr;
    ULONG ret_len;
    NTSTATUS status;

    status = Wow64KiUserCallbackDispatcher( NtUserLoadSysMenu, arg, size, &ret_ptr, &ret_len );
    if (!status && ret_len == sizeof(ULONG))
    {
        HMENU menu = ULongToHandle( *(ULONG *)ret_ptr );
        return NtCallbackReturn( &menu, sizeof(menu), status );
    }
    return status;
}

static NTSTATUS WINAPI wow64_NtUserPostDDEMessage( void *arg, ULONG size )
{
    struct post_dde_message_params *params = arg;
    struct
    {
        ULONG hwnd;
        UINT msg;
        LONG wparam;
        LONG lparam;
        DWORD dest_tid;
    } params32;

    params32.hwnd = HandleToUlong( params->hwnd );
    params32.msg = params->msg;
    params32.wparam = params->wparam;
    params32.lparam = params->lparam;
    params32.dest_tid = params->dest_tid;
    return dispatch_callback( NtUserPostDDEMessage, &params32, sizeof(params32) );
}

static NTSTATUS WINAPI wow64_NtUserRenderSynthesizedFormat( void *arg, ULONG size )
{
    return dispatch_callback( NtUserRenderSynthesizedFormat, arg, size );
}

static NTSTATUS WINAPI wow64_NtUserUnpackDDEMessage( void *arg, ULONG size )
{
    struct unpack_dde_message_params *params = arg;
    struct unpack_dde_message_params32 *params32;
    struct unpack_dde_message_result result;
    struct
    {
        LONG wparam;
        LONG lparam;
    } *result32;
    ULONG ret_len;
    NTSTATUS status;

    size -= FIELD_OFFSET( struct unpack_dde_message_params, data );
    if (!(params32 = Wow64AllocateTemp( FIELD_OFFSET( struct unpack_dde_message_params32, data[size] ))))
        return STATUS_NO_MEMORY;

    params32->hwnd = HandleToUlong( params->hwnd );
    params32->message = params->message;
    params32->wparam = params->wparam;
    params32->lparam = params->lparam;
    if (size) memcpy( params32->data, params->data, size );
    size = FIELD_OFFSET( struct unpack_dde_message_params32, data[size] );

    status = Wow64KiUserCallbackDispatcher( NtUserUnpackDDEMessage, params32, size, (void **)&result32, &ret_len );
    if (!status && ret_len == sizeof(*result32))
    {
        result.wparam = result32->wparam;
        result.lparam = result32->lparam;
        return NtCallbackReturn( &result, sizeof(result), status );
    }
    return status;
}

static NTSTATUS WINAPI wow64_NtUserCallDispatchCallback( void *arg, ULONG size )
{
    return dispatch_callback( NtUserCallDispatchCallback, arg, size );
}

static NTSTATUS WINAPI wow64_NtUserDragDropEnter( void *arg, ULONG size )
{
    return dispatch_callback( NtUserDragDropEnter, arg, size );
}

static NTSTATUS WINAPI wow64_NtUserDragDropLeave( void *arg, ULONG size )
{
    return dispatch_callback( NtUserDragDropLeave, arg, size );
}

static NTSTATUS WINAPI wow64_NtUserDragDropDrag( void *arg, ULONG size )
{
    const struct drag_drop_drag_params *params = arg;
    struct
    {
        ULONG hwnd;
        POINT point;
        UINT effect;
    } params32;
    params32.hwnd = HandleToUlong( params->hwnd );
    params32.point = params->point;
    params32.effect = params->effect;
    return dispatch_callback( NtUserDragDropDrag, &params32, sizeof(params32) );
}

static NTSTATUS WINAPI wow64_NtUserDragDropDrop( void *arg, ULONG size )
{
    const struct drag_drop_drop_params *params = arg;
    struct
    {
        ULONG hwnd;
    } params32;
    params32.hwnd = HandleToUlong( params->hwnd );
    return dispatch_callback( NtUserDragDropDrop, &params32, sizeof(params32) );
}

static NTSTATUS WINAPI wow64_NtUserDragDropPost( void *arg, ULONG size )
{
    struct drag_drop_post_params32
    {
        ULONG hwnd;
        UINT drop_size;
        struct drop_files drop;
        BYTE files[];
    };
    const struct drag_drop_post_params *params = arg;
    struct drag_drop_post_params32 *params32;

    size = offsetof(struct drag_drop_post_params32, drop) + params->drop_size;
    if (!(params32 = Wow64AllocateTemp( size ))) return STATUS_NO_MEMORY;
    params32->hwnd = HandleToUlong( params->hwnd );
    params32->drop_size = params->drop_size;
    memcpy( &params32->drop, &params->drop, params->drop_size );
    return dispatch_callback( NtUserDragDropPost, params32, size );
}

ntuser_callback user_callbacks[NtUserCallCount] =
{
#define USER32_CALLBACK_ENTRY(name) wow64_NtUser##name,
    ALL_USER32_CALLBACKS
#undef USER32_CALLBACK_ENTRY
};

NTSTATUS WINAPI wow64_NtUserActivateKeyboardLayout( UINT *args )
{
    HKL layout = get_handle( &args );
    UINT flags = get_ulong( &args );

    return HandleToUlong( NtUserActivateKeyboardLayout( layout, flags ));
}

NTSTATUS WINAPI wow64_NtUserAddClipboardFormatListener( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserAddClipboardFormatListener( hwnd );
}

NTSTATUS WINAPI wow64_NtUserAlterWindowStyle( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT mask = get_ulong( &args );
    UINT style = get_ulong( &args );

    return NtUserAlterWindowStyle( hwnd, mask, style );
}

NTSTATUS WINAPI wow64_NtUserArrangeIconicWindows( UINT *args )
{
    HWND parent = get_handle( &args );

    return NtUserArrangeIconicWindows( parent );
}

NTSTATUS WINAPI wow64_NtUserAssociateInputContext( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HIMC ctx = get_handle( &args );
    ULONG flags = get_ulong( &args );

    return NtUserAssociateInputContext( hwnd, ctx, flags );
}

NTSTATUS WINAPI wow64_NtUserAttachThreadInput( UINT *args )
{
    DWORD from = get_ulong( &args );
    DWORD to = get_ulong( &args );
    BOOL attach = get_ulong( &args );

    return NtUserAttachThreadInput( from, to, attach );
}

NTSTATUS WINAPI wow64_NtUserBeginDeferWindowPos( UINT *args )
{
    INT count = get_ulong( &args );

    return HandleToUlong( NtUserBeginDeferWindowPos( count ));
}

NTSTATUS WINAPI wow64_NtUserBeginPaint( UINT *args )
{
    HWND hwnd = get_handle( &args );
    PAINTSTRUCT32 *ps32 = get_ptr( &args );

    PAINTSTRUCT ps;
    HDC ret;

    ret = NtUserBeginPaint( hwnd, ps32 ? & ps : NULL );
    if (ret && ps32)
    {
        ps32->hdc = HandleToUlong( ps.hdc );
        ps32->fErase  = ps.fErase;
        ps32->rcPaint = ps.rcPaint;
    }
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserBuildHimcList( UINT *args )
{
    ULONG thread_id = get_ulong( &args );
    ULONG count = get_ulong( &args );
    UINT32 *buffer32 = get_ptr( &args );
    UINT *size = get_ptr( &args );

    HIMC *buffer = NULL;
    ULONG i;
    NTSTATUS status;

    if (buffer32 && !(buffer = Wow64AllocateTemp( count * sizeof(*buffer) )))
        return STATUS_NO_MEMORY;

    if ((status = NtUserBuildHimcList( thread_id, count, buffer, size ))) return status;

    for (i = 0; i < *size; i++) buffer32[i] = HandleToUlong( buffer[i] );
    return status;
}

NTSTATUS WINAPI wow64_NtUserBuildHwndList( UINT *args )
{
    HDESK desktop = get_handle( &args );
    HWND hwnd = get_handle( &args );
    BOOL children = get_ulong( &args );
    BOOL non_immersive = get_ulong( &args );
    ULONG thread_id = get_ulong( &args );
    ULONG count = get_ulong( &args );
    UINT32 *buffer32 = get_ptr( &args );
    ULONG *size = get_ptr( &args );

    HWND *buffer;
    ULONG i;
    NTSTATUS status;

    if (!(buffer = Wow64AllocateTemp( count * sizeof(*buffer) ))) return STATUS_NO_MEMORY;

    if ((status = NtUserBuildHwndList( desktop, hwnd, children, non_immersive,
                                       thread_id, count, buffer, size )))
        return status;

    for (i = 0; i < *size; i++) buffer32[i] = HandleToUlong( buffer[i] );
    return status;
}

NTSTATUS WINAPI wow64_NtUserBuildNameList( UINT *args )
{
    HWINSTA handle = get_handle( &args );
    ULONG count = get_ulong( &args );
    struct ntuser_name_list *props = get_ptr( &args );
    ULONG *ret_count = get_ptr( &args );

    return NtUserBuildNameList( handle, count, props, ret_count );
}

NTSTATUS WINAPI wow64_NtUserBuildPropList( UINT *args )
{
    HWND hwnd = get_handle( &args );
    ULONG count = get_ulong( &args );
    struct ntuser_property_list *props = get_ptr( &args );
    ULONG *ret_count = get_ptr( &args );

    return NtUserBuildPropList( hwnd, count, props, ret_count );
}

NTSTATUS WINAPI wow64_NtUserCallHwnd( UINT *args )
{
    HWND hwnd = get_handle( &args );
    DWORD code = get_ulong( &args );

    return NtUserCallHwnd( hwnd, code );
}

NTSTATUS WINAPI wow64_NtUserCallHwndParam( UINT *args )
{
    HWND hwnd = get_handle( &args );
    DWORD_PTR param = get_ulong( &args );
    DWORD code = get_ulong( &args );

    switch (code)
    {
    case NtUserCallHwndParam_GetScrollInfo:
        {
            struct
            {
                int bar;
                ULONG info;
            } *info32 = UlongToPtr( param );
            struct get_scroll_info_params info;

            info.bar = info32->bar;
            info.info = UlongToPtr( info32->info );
            return NtUserCallHwndParam( hwnd, (UINT_PTR)&info, code );
        }

    case NtUserCallHwndParam_GetWindowRects:
        {
            struct
            {
                ULONG rect;
                BOOL client;
                UINT dpi;
            } *params32 = UlongToPtr( param );
            struct get_window_rects_params params;

            params.rect = UlongToPtr( params32->rect );
            params.client = params32->client;
            params.dpi = params32->dpi;
            return NtUserCallHwndParam( hwnd, (UINT_PTR)&params, code );
        }

    case NtUserCallHwndParam_MapWindowPoints:
        {
            struct
            {
                ULONG hwnd_to;
                ULONG points;
                UINT count;
                UINT dpi;
            } *params32 = UlongToPtr( param );
            struct map_window_points_params params;

            params.hwnd_to = LongToHandle( params32->hwnd_to );
            params.points = UlongToPtr( params32->points );
            params.count = params32->count;
            params.dpi = params32->dpi;
            return NtUserCallHwndParam( hwnd, (UINT_PTR)&params, code );
        }

    case NtUserCallHwndParam_SendHardwareInput:
        {
            struct
            {
                UINT flags;
                ULONG input;
                ULONG lparam;
            } *params32 = UlongToPtr( param );
            struct send_hardware_input_params params;

            params.flags = params32->flags;
            params.input = UlongToPtr( params32->input );
            params.lparam = params32->lparam;
            return NtUserCallHwndParam( hwnd, (UINT_PTR)&params, code );
        }

    default:
        return NtUserCallHwndParam( hwnd, param, code );
    }
}

NTSTATUS WINAPI wow64_NtUserCallMsgFilter( UINT *args )
{
    MSG32 *msg32 = get_ptr( &args );
    INT code = get_ulong( &args );
    MSG msg;
    BOOL ret;

    ret = NtUserCallMsgFilter( msg_32to64( &msg, msg32 ), code );
    msg_64to32( &msg, msg32 );
    return ret;
}

NTSTATUS WINAPI wow64_NtUserCallNextHookEx( UINT *args )
{
    HHOOK hhook = get_handle( &args );
    INT code = get_ulong( &args );
    WPARAM wparam = get_ulong( &args );
    LPARAM lparam = get_ulong( &args );

    return NtUserCallNextHookEx( hhook, code, wparam, lparam );
}

NTSTATUS WINAPI wow64_NtUserCallNoParam( UINT *args )
{
    ULONG code = get_ulong( &args );

    return NtUserCallNoParam( code );
}

NTSTATUS WINAPI wow64_NtUserCallOneParam( UINT *args )
{
    ULONG_PTR arg = get_ulong( &args );
    ULONG code = get_ulong( &args );

    return NtUserCallOneParam( arg, code );
}

NTSTATUS WINAPI wow64_NtUserCallTwoParam( UINT *args )
{
    ULONG_PTR arg1 = get_ulong( &args );
    ULONG_PTR arg2 = get_ulong( &args );
    ULONG code = get_ulong( &args );

    switch (code)
    {
    case NtUserCallTwoParam_GetMenuInfo:
        {
            MENUINFO32 *info32 = UlongToPtr( arg2 );
            MENUINFO info;

            if (!info32 || info32->cbSize != sizeof(*info32))
            {
                set_last_error32( ERROR_INVALID_PARAMETER );
                return FALSE;
            }

            info.cbSize = sizeof(info);
            info.fMask = info32->fMask;
            if (!NtUserCallTwoParam( arg1, (UINT_PTR)&info, code )) return FALSE;
            if (info.fMask & MIM_BACKGROUND) info32->hbrBack = HandleToUlong( info.hbrBack );
            if (info.fMask & MIM_HELPID)     info32->dwContextHelpID = info.dwContextHelpID;
            if (info.fMask & MIM_MAXHEIGHT)  info32->cyMax = info.cyMax;
            if (info.fMask & MIM_MENUDATA)   info32->dwMenuData = info.dwMenuData;
            if (info.fMask & MIM_STYLE)      info32->dwStyle = info.dwStyle;
            return TRUE;
        }

    default:
        return NtUserCallTwoParam( arg1, arg2, code );
    }
}

NTSTATUS WINAPI wow64_NtUserChangeClipboardChain( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HWND next = get_handle( &args );

    return NtUserChangeClipboardChain( hwnd, next );
}

NTSTATUS WINAPI wow64_NtUserChangeDisplaySettings( UINT *args )
{
    UNICODE_STRING32 *devname32 = get_ptr( &args );
    DEVMODEW *devmode = get_ptr( &args );
    HWND hwnd = get_handle( &args );
    DWORD flags = get_ulong( &args );
    void *lparam = get_ptr( &args );

    UNICODE_STRING devname;

    return NtUserChangeDisplaySettings( unicode_str_32to64( &devname, devname32 ),
                                        devmode, hwnd, flags, lparam );
}

NTSTATUS WINAPI wow64_NtUserCheckMenuItem( UINT *args )
{
    HMENU handle = get_handle( &args );
    UINT id = get_ulong( &args );
    UINT flags = get_ulong( &args );

    return NtUserCheckMenuItem( handle, id, flags );
}

NTSTATUS WINAPI wow64_NtUserChildWindowFromPointEx( UINT *args )
{
    HWND parent = get_handle( &args );
    LONG x = get_ulong( &args );
    LONG y = get_ulong( &args );
    UINT flags = get_ulong( &args );

    return HandleToUlong( NtUserChildWindowFromPointEx( parent, x, y, flags ));
}

NTSTATUS WINAPI wow64_NtUserClipCursor( UINT *args )
{
    const RECT *rect = get_ptr( &args );

    return NtUserClipCursor( rect );
}

NTSTATUS WINAPI wow64_NtUserCloseClipboard( UINT *args )
{
    return NtUserCloseClipboard();
}

NTSTATUS WINAPI wow64_NtUserCloseDesktop( UINT *args )
{
    HDESK handle = get_handle( &args );

    return NtUserCloseDesktop( handle );
}

NTSTATUS WINAPI wow64_NtUserCloseWindowStation( UINT *args )
{
    HWINSTA handle = get_handle( &args );

    return NtUserCloseWindowStation( handle );
}

NTSTATUS WINAPI wow64_NtUserCopyAcceleratorTable( UINT *args )
{
    HACCEL src = get_handle( &args );
    ACCEL *dst = get_ptr( &args );
    INT count = get_ulong( &args );

    return NtUserCopyAcceleratorTable( src, dst, count );
}

NTSTATUS WINAPI wow64_NtUserCountClipboardFormats( UINT *args )
{
    return NtUserCountClipboardFormats();
}

NTSTATUS WINAPI wow64_NtUserCreateAcceleratorTable( UINT *args )
{
    ACCEL *table = get_ptr( &args );
    INT count = get_ulong( &args );

    return HandleToUlong( NtUserCreateAcceleratorTable( table, count ));
}

NTSTATUS WINAPI wow64_NtUserCreateCaret( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HBITMAP bitmap = get_handle( &args );
    int width = get_ulong( &args );
    int height = get_ulong( &args );

    return NtUserCreateCaret( hwnd, bitmap, width, height );
}

NTSTATUS WINAPI wow64_NtUserCreateDesktopEx( UINT *args )
{
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    UNICODE_STRING32 *device32 = get_ptr( &args );
    DEVMODEW *devmode = get_ptr( &args );
    DWORD flags = get_ulong( &args );
    ACCESS_MASK access = get_ulong( &args );
    ULONG heap_size = get_ulong( &args );

    struct object_attr64 attr;
    UNICODE_STRING device;
    HANDLE ret;

    ret = NtUserCreateDesktopEx( objattr_32to64( &attr, attr32 ),
                                 unicode_str_32to64( &device, device32 ),
                                 devmode, flags, access, heap_size );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserCreateInputContext( UINT *args )
{
    UINT_PTR client_ptr = get_ulong( &args );

    return HandleToUlong( NtUserCreateInputContext( client_ptr ));
}

NTSTATUS WINAPI wow64_NtUserCreateMenu( UINT *args )
{
    return HandleToUlong( NtUserCreateMenu() );
}

NTSTATUS WINAPI wow64_NtUserCreatePopupMenu( UINT *args )
{
    return HandleToUlong( NtUserCreatePopupMenu() );
}

NTSTATUS WINAPI wow64_NtUserCreateWindowEx( UINT *args )
{
    DWORD ex_style = get_ulong( &args );
    UNICODE_STRING32 *class_name32 = get_ptr( &args );
    UNICODE_STRING32 *version32 = get_ptr( &args );
    UNICODE_STRING32 *window_name32 = get_ptr( &args );
    DWORD style = get_ulong( &args );
    int x = get_ulong( &args );
    int y = get_ulong( &args );
    int width = get_ulong( &args );
    int height = get_ulong( &args );
    HWND parent = get_handle( &args );
    HMENU menu = get_handle( &args );
    HINSTANCE instance = get_ptr( &args );
    void *params = get_ptr( &args );
    DWORD flags = get_ulong( &args );
    HINSTANCE client_instance = get_ptr( &args );
    DWORD unk = get_ulong( &args );
    BOOL ansi = get_ulong( &args );

    UNICODE_STRING class_name, version, window_name;
    HWND ret;

    ret = NtUserCreateWindowEx( ex_style,
                                unicode_str_32to64( &class_name, class_name32),
                                unicode_str_32to64( &version, version32 ),
                                unicode_str_32to64( &window_name, window_name32 ),
                                style, x, y, width, height, parent, menu,
                                instance, params, flags, client_instance, unk, ansi );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserCreateWindowStation( UINT *args )
{
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    ULONG arg3 = get_ulong( &args );
    ULONG arg4 = get_ulong( &args );
    ULONG arg5 = get_ulong( &args );
    ULONG arg6 = get_ulong( &args );
    ULONG arg7 = get_ulong( &args );

    struct object_attr64 attr;

    return HandleToUlong( NtUserCreateWindowStation( objattr_32to64( &attr, attr32 ), access,
                                                     arg3, arg4, arg5, arg6, arg7 ));
}

NTSTATUS WINAPI wow64_NtUserDeferWindowPosAndBand( UINT *args )
{
    HDWP hdwp = get_handle( &args );
    HWND hwnd = get_handle( &args );
    HWND after = get_handle( &args );
    INT x = get_ulong( &args );
    INT y = get_ulong( &args );
    INT cx = get_ulong( &args );
    INT cy = get_ulong( &args );
    UINT flags = get_ulong( &args );
    UINT unk1 = get_ulong( &args );
    UINT unk2 = get_ulong( &args );

    HDWP ret = NtUserDeferWindowPosAndBand( hdwp, hwnd, after, x, y, cx, cy, flags, unk1, unk2 );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserDeleteMenu( UINT *args )
{
    HMENU menu = get_handle( &args );
    UINT id = get_ulong( &args );
    UINT flags = get_ulong( &args );

    return NtUserDeleteMenu( menu, id, flags );
}

NTSTATUS WINAPI wow64_NtUserDestroyAcceleratorTable( UINT *args )
{
    HACCEL handle = get_handle( &args );

    return NtUserDestroyAcceleratorTable( handle );
}

NTSTATUS WINAPI wow64_NtUserDestroyCaret( UINT *args )
{
    return NtUserDestroyCaret();
}

NTSTATUS WINAPI wow64_NtUserDestroyCursor( UINT *args )
{
    HCURSOR cursor = get_handle( &args );
    ULONG arg = get_ulong( &args );

    return NtUserDestroyCursor( cursor, arg );
}

NTSTATUS WINAPI wow64_NtUserDestroyInputContext( UINT *args )
{
    HIMC handle = get_handle( &args );

    return NtUserDestroyInputContext( handle );
}

NTSTATUS WINAPI wow64_NtUserDestroyMenu( UINT *args )
{
    HMENU handle = get_handle( &args );

    return NtUserDestroyMenu( handle );
}

NTSTATUS WINAPI wow64_NtUserDestroyWindow( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserDestroyWindow( hwnd );
}

NTSTATUS WINAPI wow64_NtUserDisableThreadIme( UINT *args )
{
    DWORD thread_id = get_ulong( &args );

    return NtUserDisableThreadIme( thread_id );
}

NTSTATUS WINAPI wow64_NtUserDispatchMessage( UINT *args )
{
    const MSG32 *msg32 = get_ptr( &args );
    MSG msg;

    return NtUserDispatchMessage( msg_32to64( &msg, msg32 ));
}

NTSTATUS WINAPI wow64_NtUserDragDetect( UINT *args )
{
    HWND hwnd = get_handle( &args );
    int x = get_ulong( &args );
    int y = get_ulong( &args );

    return NtUserDragDetect( hwnd, x, y );
}

NTSTATUS WINAPI wow64_NtUserDragObject( UINT *args )
{
    HWND parent = get_handle( &args );
    HWND hwnd = get_handle( &args );
    UINT fmt = get_ulong( &args );
    ULONG_PTR data = get_ulong( &args );
    HCURSOR hcursor = get_handle( &args );

    return NtUserDragObject( parent, hwnd, fmt, data, hcursor );
}

NTSTATUS WINAPI wow64_NtUserDrawCaptionTemp( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HDC hdc = get_handle( &args );
    const RECT *rect = get_ptr( &args );
    HFONT font = get_handle( &args );
    HICON icon = get_handle( &args );
    const WCHAR *str = get_ptr( &args );
    UINT flags = get_ulong( &args );

    return NtUserDrawCaptionTemp( hwnd, hdc, rect, font, icon, str, flags );
}

NTSTATUS WINAPI wow64_NtUserDrawIconEx( UINT *args )
{
    HDC hdc = get_handle( &args );
    int x0 = get_ulong( &args );
    int y0 = get_ulong( &args );
    HICON icon = get_handle( &args );
    int width = get_ulong( &args );
    int height = get_ulong( &args );
    UINT istep = get_ulong( &args );
    HBRUSH hbr = get_handle( &args );
    UINT flags = get_ulong( &args );

    return NtUserDrawIconEx( hdc, x0, y0, icon, width, height, istep, hbr, flags );
}

NTSTATUS WINAPI wow64_NtUserDrawMenuBar( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserDrawMenuBar( hwnd );
}

NTSTATUS WINAPI wow64_NtUserDrawMenuBarTemp( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HDC hdc = get_handle( &args );
    RECT *rect = get_ptr( &args );
    HMENU handle = get_handle( &args );
    HFONT font = get_handle( &args );

    return NtUserDrawMenuBarTemp( hwnd, hdc, rect, handle, font );
}

NTSTATUS WINAPI wow64_NtUserEmptyClipboard( UINT *args )
{
    return NtUserEmptyClipboard();
}

NTSTATUS WINAPI wow64_NtUserEnableMenuItem( UINT *args )
{
    HMENU handle = get_handle( &args );
    UINT id = get_ulong( &args );
    UINT flags = get_ulong( &args );

    return NtUserEnableMenuItem( handle, id, flags );
}

NTSTATUS WINAPI wow64_NtUserEnableMouseInPointer( UINT *args )
{
    UINT enable = get_ulong( &args );

    return NtUserEnableMouseInPointer( enable );
}

NTSTATUS WINAPI wow64_NtUserEnableMouseInPointerForThread( UINT *args )
{
    return NtUserEnableMouseInPointerForThread();
}

NTSTATUS WINAPI wow64_NtUserEnableScrollBar( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT bar = get_ulong( &args );
    UINT flags = get_ulong( &args );

    return NtUserEnableScrollBar( hwnd, bar, flags );
}

NTSTATUS WINAPI wow64_NtUserEnableWindow( UINT *args )
{
    HWND hwnd = get_handle( &args );
    BOOL enable = get_ulong( &args );

    return NtUserEnableWindow( hwnd, enable );
}

NTSTATUS WINAPI wow64_NtUserEndDeferWindowPosEx( UINT *args )
{
    HDWP hdwp = get_handle( &args );
    BOOL async = get_ulong( &args );

    return NtUserEndDeferWindowPosEx( hdwp, async );
}

NTSTATUS WINAPI wow64_NtUserEndMenu( UINT *args )
{
    return NtUserEndMenu();
}

NTSTATUS WINAPI wow64_NtUserEndPaint( UINT *args )
{
    HWND hwnd = get_handle( &args );
    const PAINTSTRUCT32 *ps32 = get_ptr( &args );
    PAINTSTRUCT ps;

    return NtUserEndPaint( hwnd, paintstruct_32to64( &ps, ps32 ));
}

NTSTATUS WINAPI wow64_NtUserEnumClipboardFormats( UINT *args )
{
    UINT format = get_ulong( &args );

    return NtUserEnumClipboardFormats( format );
}

NTSTATUS WINAPI wow64_NtUserEnumDisplayDevices( UINT *args )
{
    UNICODE_STRING32 *device32 = get_ptr( &args );
    DWORD index = get_ulong( &args );
    DISPLAY_DEVICEW *info = get_ptr( &args );
    DWORD flags = get_ulong( &args );

    UNICODE_STRING device;

    return NtUserEnumDisplayDevices( unicode_str_32to64( &device, device32 ), index, info, flags );
}

NTSTATUS WINAPI wow64_NtUserEnumDisplayMonitors( UINT *args )
{
    HDC hdc = get_handle( &args );
    RECT *rect = get_ptr( &args );
    MONITORENUMPROC proc = get_ptr( &args );
    LPARAM lp = get_ulong( &args );

    return NtUserEnumDisplayMonitors( hdc, rect, proc, lp );
}

NTSTATUS WINAPI wow64_NtUserEnumDisplaySettings( UINT *args )
{
    UNICODE_STRING32 *device32 = get_ptr( &args );
    DWORD mode = get_ulong( &args );
    DEVMODEW *dev_mode = get_ptr( &args );
    DWORD flags = get_ulong( &args );

    UNICODE_STRING device;

    return NtUserEnumDisplaySettings( unicode_str_32to64( &device, device32 ),
                                      mode, dev_mode, flags );
}

NTSTATUS WINAPI wow64_NtUserExcludeUpdateRgn( UINT *args )
{
    HDC hdc = get_handle( &args );
    HWND hwnd = get_handle( &args );

    return NtUserExcludeUpdateRgn( hdc, hwnd );
}

NTSTATUS WINAPI wow64_NtUserFindExistingCursorIcon( UINT *args )
{
    UNICODE_STRING32 *module32 = get_ptr( &args );
    UNICODE_STRING32 *res_name32 = get_ptr( &args );
    void *desc = get_ptr( &args );

    UNICODE_STRING module;
    UNICODE_STRING res_name;
    HICON ret;

    ret = NtUserFindExistingCursorIcon( unicode_str_32to64( &module, module32 ),
                                        unicode_str_32to64( &res_name, res_name32 ), desc );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserFindWindowEx( UINT *args )
{
    HWND parent = get_handle( &args );
    HWND child = get_handle( &args );
    UNICODE_STRING32 *class32 = get_ptr( &args );
    UNICODE_STRING32 *title32 = get_ptr( &args );
    ULONG unk = get_ulong( &args );

    UNICODE_STRING class, title;
    HWND ret;

    ret = NtUserFindWindowEx( parent, child, unicode_str_32to64( &class, class32 ),
                              unicode_str_32to64( &title, title32 ), unk );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserFlashWindowEx( UINT *args )
{
    struct
    {
        UINT cbSize;
        ULONG hwnd;
        DWORD dwFlags;
        UINT uCount;
        DWORD dwTimeout;
    } *info32 = get_ptr( &args );

    FLASHWINFO info;

    if (!info32)
    {
        set_last_error32( ERROR_NOACCESS );
        return FALSE;
    }

    if (info32->cbSize != sizeof(*info32))
    {
        set_last_error32( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    info.cbSize = sizeof(info);
    info.hwnd = LongToHandle( info32->hwnd );
    info.dwFlags = info32->dwFlags;
    info.uCount = info32->uCount;
    info.dwTimeout = info32->dwTimeout;
    return NtUserFlashWindowEx( &info );
}

NTSTATUS WINAPI wow64_NtUserGetAncestor( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT type = get_ulong( &args );

    return HandleToUlong( NtUserGetAncestor( hwnd, type ));
}

NTSTATUS WINAPI wow64_NtUserGetAsyncKeyState( UINT *args )
{
    INT key = get_ulong( &args );

    return NtUserGetAsyncKeyState( key );
}

NTSTATUS WINAPI wow64_NtUserGetAtomName( UINT *args )
{
    ATOM atom = get_ulong( &args );
    UNICODE_STRING32 *str32 = get_ptr( &args );

    UNICODE_STRING str;

    return NtUserGetAtomName( atom, unicode_str_32to64( &str, str32 ));
}

NTSTATUS WINAPI wow64_NtUserGetCaretBlinkTime( UINT *args )
{
    return NtUserGetCaretBlinkTime();
}

NTSTATUS WINAPI wow64_NtUserGetCaretPos( UINT *args )
{
    POINT *pt = get_ptr( &args );

    return NtUserGetCaretPos( pt );
}

NTSTATUS WINAPI wow64_NtUserGetClassInfoEx( UINT *args )
{
    HINSTANCE instance = get_ptr( &args );
    UNICODE_STRING32 *name32 = get_ptr( &args );
    WNDCLASSEXW32 *wc32 = get_ptr( &args );
    struct client_menu_name32 *client_name32 = get_ptr( &args );
    BOOL ansi = get_ulong( &args );

    struct client_menu_name client_name;
    UNICODE_STRING name;
    WNDCLASSEXW wc;
    ATOM ret;

    wc.cbSize = sizeof(wc);
    if (!(ret = NtUserGetClassInfoEx( instance, unicode_str_32to64( &name, name32 ), &wc,
                                      &client_name, ansi )))
        return 0;

    wc32->style = wc.style;
    wc32->lpfnWndProc = PtrToUlong( wc.lpfnWndProc );
    wc32->cbClsExtra = wc.cbClsExtra;
    wc32->cbWndExtra = wc.cbWndExtra;
    wc32->hInstance = PtrToUlong( wc.hInstance );
    wc32->hIcon = HandleToUlong( wc.hIcon );
    wc32->hCursor = HandleToUlong( wc.hCursor );
    wc32->hbrBackground = HandleToUlong( wc.hbrBackground );
    wc32->lpszMenuName = PtrToUlong( wc.lpszMenuName );
    wc32->lpszClassName = PtrToUlong( wc.lpszClassName );
    wc32->hIconSm = HandleToUlong( wc.hIconSm );
    client_menu_name_64to32( &client_name, client_name32 );
    return ret;
}

NTSTATUS WINAPI wow64_NtUserGetClassName( UINT *args )
{
    HWND hwnd = get_handle( &args );
    BOOL real = get_ulong( &args );
    UNICODE_STRING32 *str32 = get_ptr( &args );

    UNICODE_STRING str;

    return NtUserGetClassName( hwnd, real, unicode_str_32to64( &str, str32 ));
}

NTSTATUS WINAPI wow64_NtUserGetClipCursor( UINT *args )
{
    RECT *rect = get_ptr( &args );

    return NtUserGetClipCursor( rect );
}

NTSTATUS WINAPI wow64_NtUserGetClipboardData( UINT *args )
{
    UINT format = get_ulong( &args );
    struct
    {
        UINT32 data;
        UINT32 size;
        UINT32 data_size;
        UINT   seqno;
        BOOL   data_only;
    } *params32 = get_ptr( &args );

    struct get_clipboard_params params;
    HANDLE ret;

    params.data = UlongToPtr( params32->data );
    params.size = params32->size;
    params.data_size = params32->data_size;
    params.data_only = params32->data_only;

    ret = NtUserGetClipboardData( format, &params );

    params32->size      = params.size;
    params32->data_size = params.data_size;
    params32->seqno     = params.seqno;
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserGetClipboardFormatName( UINT *args )
{
    UINT format = get_ulong( &args );
    WCHAR *buffer = get_ptr( &args );
    INT maxlen = get_ulong( &args );

    return NtUserGetClipboardFormatName( format, buffer, maxlen );
}

NTSTATUS WINAPI wow64_NtUserGetClipboardOwner( UINT *args )
{
    return HandleToUlong( NtUserGetClipboardOwner() );
}

NTSTATUS WINAPI wow64_NtUserGetClipboardSequenceNumber( UINT *args )
{
    return NtUserGetClipboardSequenceNumber();
}

NTSTATUS WINAPI wow64_NtUserGetClipboardViewer( UINT *args )
{
    return HandleToUlong( NtUserGetClipboardViewer() );
}

NTSTATUS WINAPI wow64_NtUserGetCurrentInputMessageSource( UINT *args )
{
    INPUT_MESSAGE_SOURCE *source = get_ptr( &args );

    return NtUserGetCurrentInputMessageSource( source );
}

NTSTATUS WINAPI wow64_NtUserGetCursor( UINT *args )
{
    return HandleToUlong( NtUserGetCursor() );
}

NTSTATUS WINAPI wow64_NtUserGetCursorFrameInfo( UINT *args )
{
    HCURSOR cursor = get_ptr( &args );
    DWORD istep = get_ulong( &args );
    DWORD *rate_jiffies = get_ptr( &args );
    DWORD *num_steps = get_ptr( &args );

    return HandleToUlong( NtUserGetCursorFrameInfo( cursor, istep, rate_jiffies, num_steps ));
}

NTSTATUS WINAPI wow64_NtUserGetCursorInfo( UINT *args )
{
    struct
    {
        DWORD cbSize;
        DWORD flags;
        ULONG hCursor;
        POINT ptScreenPos;
    } *info32 = get_ptr( &args );
    CURSORINFO info;

    if (!info32) return FALSE;
    info.cbSize = sizeof(info);
    if (!NtUserGetCursorInfo( &info )) return FALSE;
    info32->flags = info.flags;
    info32->hCursor = HandleToUlong( info.hCursor );
    info32->ptScreenPos = info.ptScreenPos;
    return TRUE;
}

NTSTATUS WINAPI wow64_NtUserGetCursorPos( UINT *args )
{
    POINT *pt = get_ptr( &args );

    return NtUserGetCursorPos( pt );
}

NTSTATUS WINAPI wow64_NtUserGetDC( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return HandleToUlong( NtUserGetDC( hwnd ));
}

NTSTATUS WINAPI wow64_NtUserGetDCEx( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HRGN clip_rgn = get_handle( &args );
    DWORD flags = get_ulong( &args );

    return HandleToUlong( NtUserGetDCEx( hwnd, clip_rgn, flags ));
}

NTSTATUS WINAPI wow64_NtUserGetDisplayConfigBufferSizes( UINT *args )
{
    UINT32 flags = get_ulong( &args );
    UINT32 *num_path_info = get_ptr( &args );
    UINT32 *num_mode_info = get_ptr( &args );

    return NtUserGetDisplayConfigBufferSizes( flags, num_path_info, num_mode_info );
}

NTSTATUS WINAPI wow64_NtUserGetDoubleClickTime( UINT *args )
{
    return NtUserGetDoubleClickTime();
}

NTSTATUS WINAPI wow64_NtUserGetDpiForMonitor( UINT *args )
{
    HMONITOR monitor = get_handle( &args );
    UINT type = get_ulong( &args );
    UINT *x = get_ptr( &args );
    UINT *y = get_ptr( &args );

    return NtUserGetDpiForMonitor( monitor, type, x, y );
}

NTSTATUS WINAPI wow64_NtUserGetForegroundWindow( UINT *args )
{
    return HandleToUlong( NtUserGetForegroundWindow() );
}

NTSTATUS WINAPI wow64_NtUserGetGUIThreadInfo( UINT *args )
{
    DWORD id = get_ulong( &args );
    struct
    {
        DWORD  cbSize;
        DWORD  flags;
        ULONG  hwndActive;
        ULONG  hwndFocus;
        ULONG  hwndCapture;
        ULONG  hwndMenuOwner;
        ULONG  hwndMoveSize;
        ULONG  hwndCaret;
        RECT   rcCaret;
    } *info32 = get_ptr( &args );
    GUITHREADINFO info;

    if (info32->cbSize != sizeof(*info32))
    {
        set_last_error32( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    info.cbSize = sizeof(info);
    if (!NtUserGetGUIThreadInfo( id, &info )) return FALSE;
    info32->flags         = info.flags;
    info32->hwndActive    = HandleToUlong( info.hwndActive );
    info32->hwndFocus     = HandleToUlong( info.hwndFocus );
    info32->hwndCapture   = HandleToUlong( info.hwndCapture );
    info32->hwndMenuOwner = HandleToUlong( info.hwndMenuOwner );
    info32->hwndMoveSize  = HandleToUlong( info.hwndMoveSize );
    info32->hwndCaret     = HandleToUlong( info.hwndCaret );
    info32->rcCaret       = info.rcCaret;
    return TRUE;
}

NTSTATUS WINAPI wow64_NtUserGetIconInfo( UINT *args )
{
    HICON icon = get_handle( &args );
    struct
    {
	BOOL   fIcon;
	DWORD  xHotspot;
	DWORD  yHotspot;
	UINT32 hbmMask;
	UINT32 hbmColor;
    } *info32 = get_ptr( &args );
    UNICODE_STRING32 *module32 = get_ptr( &args );
    UNICODE_STRING32 *res_name32 = get_ptr( &args );
    DWORD *bpp = get_ptr( &args );
    LONG unk = get_ulong( &args );

    ICONINFO info;
    UNICODE_STRING module, res_name;

    if (!NtUserGetIconInfo( icon, &info, unicode_str_32to64( &module, module32 ),
                            unicode_str_32to64( &res_name, res_name32 ), bpp, unk ))
        return FALSE;

    info32->fIcon    = info.fIcon;
    info32->xHotspot = info.xHotspot;
    info32->yHotspot = info.yHotspot;
    info32->hbmMask  = HandleToUlong( info.hbmMask );
    info32->hbmColor = HandleToUlong( info.hbmColor );
    if (module32)
    {
        module32->Buffer = PtrToUlong( module.Buffer );
        module32->Length = module.Length;
    }
    if (res_name32)
    {
        res_name32->Buffer = PtrToUlong( res_name.Buffer );
        res_name32->Length = res_name.Length;
    }
    return TRUE;
}

NTSTATUS WINAPI wow64_NtUserGetIconSize( UINT *args )
{
    HICON handle = get_handle( &args );
    UINT step = get_ulong( &args );
    LONG *width = get_ptr( &args );
    LONG *height = get_ptr( &args );

    return NtUserGetIconSize( handle, step, width, height );
}

NTSTATUS WINAPI wow64_NtUserGetInternalWindowPos( UINT *args )
{
    HWND hwnd = get_handle( &args );
    RECT *rect = get_ptr( &args );
    POINT *pt = get_ptr( &args );

    return NtUserGetInternalWindowPos( hwnd, rect, pt );
}

NTSTATUS WINAPI wow64_NtUserGetKeyNameText( UINT *args )
{
    LONG lparam = get_ulong( &args );
    WCHAR *buffer = get_ptr( &args );
    INT size = get_ulong( &args );

    return NtUserGetKeyNameText( lparam, buffer, size );
}

NTSTATUS WINAPI wow64_NtUserGetKeyState( UINT *args )
{
    INT vkey = get_ulong( &args );

    return NtUserGetKeyState( vkey );
}

NTSTATUS WINAPI wow64_NtUserGetKeyboardLayout( UINT *args )
{
    DWORD tid = get_ulong( &args );

    return HandleToUlong( NtUserGetKeyboardLayout( tid ));
}

NTSTATUS WINAPI wow64_NtUserGetKeyboardLayoutList( UINT *args )
{
    INT size = get_ulong( &args );
    UINT32 *layouts32 = get_ptr( &args );
    HKL *layouts = NULL;
    UINT ret, i;

    if (layouts32 && size && !(layouts = Wow64AllocateTemp( size * sizeof(*layouts) )))
        return 0;

    ret = NtUserGetKeyboardLayoutList( size, layouts );
    if (layouts)
        for (i = 0; i < ret; i++) layouts32[i] = HandleToUlong( layouts[i] );
    return ret;
}

NTSTATUS WINAPI wow64_NtUserGetKeyboardLayoutName( UINT *args )
{
    WCHAR *name = get_ptr( &args );

    return NtUserGetKeyboardLayoutName( name );
}

NTSTATUS WINAPI wow64_NtUserGetKeyboardState( UINT *args )
{
    BYTE *state = get_ptr( &args );

    return NtUserGetKeyboardState( state );
}

NTSTATUS WINAPI wow64_NtUserGetLayeredWindowAttributes( UINT *args )
{
    HWND hwnd = get_handle( &args );
    COLORREF *key = get_ptr( &args );
    BYTE *alpha = get_ptr( &args );
    DWORD *flags = get_ptr( &args );

    return NtUserGetLayeredWindowAttributes( hwnd, key, alpha, flags );
}

NTSTATUS WINAPI wow64_NtUserGetMenuBarInfo( UINT *args )
{
    HWND hwnd = get_handle( &args );
    LONG id = get_ulong( &args );
    LONG item = get_ulong( &args );
    struct
    {
        DWORD cbSize;
        RECT  rcBar;
        ULONG hMenu;
        ULONG hwndMenu;
        BOOL  fBarFocused:1;
        BOOL  fFocused:1;
    } *info32 = get_ptr( &args );

    MENUBARINFO info;

    if (info32->cbSize != sizeof(*info32))
    {
        set_last_error32( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    info.cbSize = sizeof(info);
    if (!NtUserGetMenuBarInfo( hwnd, id, item, &info )) return FALSE;
    info32->rcBar       = info.rcBar;
    info32->hMenu       = HandleToUlong( info.hMenu );
    info32->hwndMenu    = HandleToUlong( info.hwndMenu );
    info32->fBarFocused = info.fBarFocused;
    info32->fFocused    = info.fFocused;
    return TRUE;
}

NTSTATUS WINAPI wow64_NtUserGetMenuItemRect( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HMENU handle = get_handle( &args );
    UINT item = get_ulong( &args );
    RECT *rect = get_ptr( &args );

    return NtUserGetMenuItemRect( hwnd, handle, item, rect );
}

NTSTATUS WINAPI wow64_NtUserGetMessage( UINT *args )
{
    MSG32 *msg32 = get_ptr( &args );
    HWND hwnd = get_handle( &args );
    UINT first = get_ulong( &args );
    UINT last = get_ulong( &args );
    MSG msg;
    int ret;

    ret = NtUserGetMessage( &msg, hwnd, first, last );
    if (ret != -1) msg_64to32( &msg, msg32 );
    return ret;
}

NTSTATUS WINAPI wow64_NtUserGetMouseMovePointsEx( UINT *args )
{
    UINT size = get_ulong( &args );
    MOUSEMOVEPOINT32 *ptin32 = get_ptr( &args );
    MOUSEMOVEPOINT32 *ptout32 = get_ptr( &args );
    int count = get_ulong( &args );
    DWORD resolution = get_ulong( &args );

    MOUSEMOVEPOINT ptin[64], ptout[64];
    int ret, i;

    if (size != sizeof(MOUSEMOVEPOINT32) || count < 0 || count > ARRAYSIZE( ptin ))
    {
        set_last_error32( ERROR_INVALID_PARAMETER );
        return -1;
    }

    if (!ptin32 || (!ptout32 && count))
    {
        set_last_error32( ERROR_NOACCESS );
        return -1;
    }

    for (i = 0; i < count; i++)
    {
        ptin[i].x = ptin32[i].x;
        ptin[i].y = ptin32[i].y;
        ptin[i].time = ptin32[i].time;
        ptin[i].dwExtraInfo = ptin32[i].dwExtraInfo;
    }

    ret = NtUserGetMouseMovePointsEx( sizeof(MOUSEMOVEPOINT), ptin, ptout, count, resolution );

    for (i = 0; i < ret; i++)
    {
        ptout32[i].x = ptout[i].x;
        ptout32[i].y = ptout[i].y;
        ptout32[i].time = ptout[i].time;
        ptout32[i].dwExtraInfo = ptout[i].dwExtraInfo;
    }

    return ret;
}

NTSTATUS WINAPI wow64_NtUserGetObjectInformation( UINT *args )
{
    HANDLE handle = get_handle( &args );
    INT index = get_ulong( &args );
    void *info = get_ptr( &args );
    DWORD len = get_ulong( &args );
    DWORD *needed = get_ptr( &args );

    return NtUserGetObjectInformation( handle, index, info, len, needed );
}

NTSTATUS WINAPI wow64_NtUserGetOpenClipboardWindow( UINT *args )
{
    return HandleToUlong( NtUserGetOpenClipboardWindow() );
}

NTSTATUS WINAPI wow64_NtUserGetPointerInfoList( UINT *args )
{
    UINT id = get_ulong( &args );
    UINT type = get_ulong( &args );
    UINT unk0 = get_ulong( &args );
    UINT unk1 = get_ulong( &args );
    UINT size = get_ulong( &args );
    void *entry_count = get_ptr( &args );
    void *pointer_count = get_ptr( &args );
    void *pointer_info = get_ptr( &args );

    return NtUserGetPointerInfoList( id, type, unk0, unk1, size, entry_count, pointer_count, pointer_info );
}

NTSTATUS WINAPI wow64_NtUserGetPriorityClipboardFormat( UINT *args )
{
    UINT *list = get_ptr( &args );
    INT count = get_ulong( &args );

    return NtUserGetPriorityClipboardFormat( list, count );
}

NTSTATUS WINAPI wow64_NtUserGetProcessDefaultLayout( UINT *args )
{
    ULONG *layout = get_ptr( &args );

    return NtUserGetProcessDefaultLayout( layout );
}

NTSTATUS WINAPI wow64_NtUserGetProcessDpiAwarenessContext( UINT *args )
{
    HANDLE process = get_handle( &args );

    return NtUserGetProcessDpiAwarenessContext( process );
}

NTSTATUS WINAPI wow64_NtUserGetProcessWindowStation( UINT *args )
{
    return HandleToUlong( NtUserGetProcessWindowStation() );
}

NTSTATUS WINAPI wow64_NtUserGetProp( UINT *args )
{
    HWND hwnd = get_handle( &args );
    const WCHAR *str = get_ptr( &args );

    return HandleToUlong( NtUserGetProp( hwnd, str ));
}

NTSTATUS WINAPI wow64_NtUserGetQueueStatus( UINT *args )
{
    UINT flags = get_ulong( &args );

    return NtUserGetQueueStatus( flags );
}

NTSTATUS WINAPI wow64_NtUserGetRawInputBuffer( UINT *args )
{
    RAWINPUT *data = get_ptr( &args );
    UINT *data_size = get_ptr( &args );
    UINT header_size = get_ulong( &args );

    if (header_size != sizeof(RAWINPUTHEADER32))
    {
        set_last_error32( ERROR_INVALID_PARAMETER );
        return ~0u;
    }

    /* RAWINPUT has different sizes on 32-bit and 64-bit, but no translation is
     * done. The function actually returns different structures depending on
     * whether it's operating under WoW64 or not. */
    return NtUserGetRawInputBuffer( data, data_size, sizeof(RAWINPUTHEADER) );
}

NTSTATUS WINAPI wow64_NtUserGetRawInputData( UINT *args )
{
    HRAWINPUT handle = get_handle( &args );
    UINT command = get_ulong( &args );
    void *data = get_ptr( &args );
    UINT *data_size = get_ptr( &args );
    UINT header_size = get_ulong( &args );

    if (header_size != sizeof(RAWINPUTHEADER32))
    {
        set_last_error32( ERROR_INVALID_PARAMETER );
        return ~0u;
    }

    switch (command)
    {
    case RID_INPUT:
        if (data)
        {
            UINT data_size64, body_size, ret;
            RAWINPUTHEADER32 *data32 = data;
            RAWINPUTHEADER *data64 = NULL;

            data_size64 = *data_size + sizeof(RAWINPUTHEADER);
            if (!(data64 = Wow64AllocateTemp( data_size64 )))
            {
                set_last_error32( STATUS_NO_MEMORY );
                return ~0u;
            }

            ret = NtUserGetRawInputData( handle, command, data64, &data_size64, sizeof(RAWINPUTHEADER) );
            if (ret == ~0u) return ret;

            body_size = ret - sizeof(RAWINPUTHEADER);
            if (*data_size < sizeof(RAWINPUTHEADER32) + body_size)
            {
                set_last_error32( ERROR_INSUFFICIENT_BUFFER );
                return ~0u;
            }

            data32->dwType = data64->dwType;
            data32->dwSize = sizeof(RAWINPUTHEADER32) + body_size;
            data32->hDevice = (UINT_PTR)data64->hDevice;
            data32->wParam = data64->wParam;
            memcpy( data32 + 1, data64 + 1, body_size );
            return sizeof(RAWINPUTHEADER32) + body_size;
        }
        else
        {
            UINT data_size64, ret;

            ret = NtUserGetRawInputData( handle, command, NULL, &data_size64, sizeof(RAWINPUTHEADER) );
            if (ret == ~0u) return ret;
            *data_size = data_size64 - sizeof(RAWINPUTHEADER) + sizeof(RAWINPUTHEADER32);
            return 0;
        }

    case RID_HEADER:
    {
        UINT data_size64 = sizeof(RAWINPUTHEADER);
        RAWINPUTHEADER32 *data32 = data;
        RAWINPUTHEADER data64;
        UINT ret;

        if (!data)
        {
            *data_size = sizeof(RAWINPUTHEADER32);
            return 0;
        }

        if (*data_size < sizeof(RAWINPUTHEADER32))
        {
            set_last_error32( ERROR_INSUFFICIENT_BUFFER );
            return ~0u;
        }

        ret = NtUserGetRawInputData( handle, command, &data64, &data_size64, sizeof(RAWINPUTHEADER) );
        if (ret == ~0u) return ret;
        data32->dwType = data64.dwType;
        data32->dwSize = data64.dwSize - sizeof(RAWINPUTHEADER) + sizeof(RAWINPUTHEADER32);
        data32->hDevice = (UINT_PTR)data64.hDevice;
        data32->wParam = data64.wParam;
        return sizeof(RAWINPUTHEADER32);
    }

    default:
        set_last_error32( ERROR_INVALID_PARAMETER );
        return ~0u;
    }
}

NTSTATUS WINAPI wow64_NtUserGetRawInputDeviceInfo( UINT *args )
{
    HANDLE handle = get_handle( &args );
    UINT command = get_ulong( &args );
    void *data = get_ptr( &args );
    UINT *data_size = get_ptr( &args );

    return NtUserGetRawInputDeviceInfo( handle, command, data, data_size );
}

NTSTATUS WINAPI wow64_NtUserGetRawInputDeviceList( UINT *args )
{
    RAWINPUTDEVICELIST32 *devices32 = get_ptr( &args );
    UINT *count = get_ptr( &args );
    UINT size = get_ulong( &args );

    if (size != sizeof(RAWINPUTDEVICELIST32))
    {
        set_last_error32( ERROR_INVALID_PARAMETER );
        return ~0u;
    }

    if (devices32)
    {
        RAWINPUTDEVICELIST *devices64;
        unsigned int ret, i;

        if (!(devices64 = Wow64AllocateTemp( (*count) * sizeof(*devices64) )))
        {
            set_last_error32( ERROR_NOT_ENOUGH_MEMORY );
            return ~0u;
        }

        ret = NtUserGetRawInputDeviceList( devices64, count, sizeof(RAWINPUTDEVICELIST) );
        if (ret == ~0u) return ret;

        for (i = 0; i < *count; ++i)
        {
            devices32[i].hDevice = (UINT_PTR)devices64[i].hDevice;
            devices32[i].dwType  = devices64[i].dwType;
        }
        return ret;
    }
    else
    {
        return NtUserGetRawInputDeviceList( NULL, count, sizeof(RAWINPUTDEVICELIST) );
    }
}

NTSTATUS WINAPI wow64_NtUserRealChildWindowFromPoint( UINT *args )
{
    HWND parent = get_handle( &args );
    LONG x = get_ulong( &args );
    LONG y = get_ulong( &args );

    return HandleToUlong( NtUserRealChildWindowFromPoint( parent, x, y ));
}

NTSTATUS WINAPI wow64_NtUserRegisterClassExWOW( UINT *args )
{
    const WNDCLASSEXW32 *wc32 = get_ptr( &args );
    UNICODE_STRING32 *name32 = get_ptr( &args );
    UNICODE_STRING32 *version32 = get_ptr( &args );
    struct client_menu_name32 *client_name32 = get_ptr( &args );
    DWORD fnid = get_ulong( &args );
    DWORD flags = get_ulong( &args );
    DWORD *wow = get_ptr( &args );

    struct client_menu_name client_name;
    UNICODE_STRING name, version;
    WNDCLASSEXW wc;

    if (wc32->cbSize != sizeof(*wc32))
    {
         set_last_error32( ERROR_INVALID_PARAMETER );
         return 0;
    }

    wc.cbSize = sizeof(wc);
    wc.style = wc32->style;
    wc.lpfnWndProc = UlongToPtr( wc32->lpfnWndProc );
    wc.cbClsExtra = wc32->cbClsExtra;
    wc.cbWndExtra = wc32->cbWndExtra;
    wc.hInstance = UlongToPtr( wc32->hInstance );
    wc.hIcon = LongToHandle( wc32->hIcon );
    wc.hCursor = LongToHandle( wc32->hCursor );
    wc.hbrBackground = UlongToHandle( wc32->hbrBackground );
    wc.lpszMenuName = UlongToPtr( wc32->lpszMenuName );
    wc.lpszClassName = UlongToPtr( wc32->lpszClassName );
    wc.hIconSm = LongToHandle( wc32->hIconSm );

    return NtUserRegisterClassExWOW( &wc,
                                     unicode_str_32to64( &name, name32 ),
                                     unicode_str_32to64( &version, version32 ),
                                     client_menu_name_32to64( &client_name, client_name32 ),
                                     fnid, flags, wow );
}

NTSTATUS WINAPI wow64_NtUserGetRegisteredRawInputDevices( UINT *args )
{
    RAWINPUTDEVICE32 *devices32 = get_ptr( &args );
    UINT *count = get_ptr( &args );
    UINT size = get_ulong( &args );

    if (size != sizeof(RAWINPUTDEVICE32))
    {
        set_last_error32( ERROR_INVALID_PARAMETER );
        return ~0u;
    }

    if (devices32)
    {
        RAWINPUTDEVICE *devices64;
        unsigned int ret, i;

        if (!(devices64 = Wow64AllocateTemp( (*count) * sizeof(*devices64) )))
        {
            set_last_error32( ERROR_NOT_ENOUGH_MEMORY );
            return ~0u;
        }

        ret = NtUserGetRegisteredRawInputDevices( devices64, count, sizeof(RAWINPUTDEVICE) );
        if (ret == ~0u) return ret;

        for (i = 0; i < *count; ++i)
        {
            devices32[i].usUsagePage = devices64[i].usUsagePage;
            devices32[i].usUsage     = devices64[i].usUsage;
            devices32[i].dwFlags     = devices64[i].dwFlags;
            devices32[i].hwndTarget  = (ULONG_PTR)devices64[i].hwndTarget;
        }
        return ret;
    }
    else
    {
        return NtUserGetRegisteredRawInputDevices( NULL, count, sizeof(RAWINPUTDEVICE) );
    }
}

NTSTATUS WINAPI wow64_NtUserGetScrollBarInfo( UINT *args )
{
    HWND hwnd = get_handle( &args );
    LONG id = get_ulong( &args );
    SCROLLBARINFO *info = get_ptr( &args );

    return NtUserGetScrollBarInfo( hwnd, id, info );
}

NTSTATUS WINAPI wow64_NtUserGetSystemDpiForProcess( UINT *args )
{
    HANDLE process = get_handle( &args );

    return NtUserGetSystemDpiForProcess( process );
}

NTSTATUS WINAPI wow64_NtUserGetSystemMenu( UINT *args )
{
    HWND hwnd = get_handle( &args );
    BOOL revert = get_ulong( &args );

    return HandleToUlong( NtUserGetSystemMenu( hwnd, revert ));
}

NTSTATUS WINAPI wow64_NtUserGetThreadDesktop( UINT *args )
{
    DWORD thread = get_ulong( &args );

    return HandleToUlong( NtUserGetThreadDesktop( thread ));
}

NTSTATUS WINAPI wow64_NtUserGetThreadState( UINT *args )
{
    USERTHREADSTATECLASS cls = get_ulong( &args );

    return NtUserGetThreadState( cls );
}

NTSTATUS WINAPI wow64_NtUserGetTitleBarInfo( UINT *args )
{
    HWND hwnd = get_handle( &args );
    TITLEBARINFO *info = get_ptr( &args );

    return NtUserGetTitleBarInfo( hwnd, info );
}

NTSTATUS WINAPI wow64_NtUserGetUpdateRect( UINT *args )
{
    HWND hwnd = get_handle( &args );
    RECT *rect = get_ptr( &args );
    BOOL erase = get_ulong( &args );

    return NtUserGetUpdateRect( hwnd, rect, erase );
}

NTSTATUS WINAPI wow64_NtUserGetUpdateRgn( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HRGN hrgn = get_handle( &args );
    BOOL erase = get_ulong( &args );

    return NtUserGetUpdateRgn( hwnd, hrgn, erase );
}

NTSTATUS WINAPI wow64_NtUserGetUpdatedClipboardFormats( UINT *args )
{
    UINT *formats = get_ptr( &args );
    UINT size = get_ulong( &args );
    UINT *out_size = get_ptr( &args );

    return NtUserGetUpdatedClipboardFormats( formats, size, out_size );
}

NTSTATUS WINAPI wow64_NtUserGetWindowContextHelpId( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserGetWindowContextHelpId( hwnd );
}

NTSTATUS WINAPI wow64_NtUserGetWindowDC( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return HandleToUlong( NtUserGetWindowDC( hwnd ));
}

NTSTATUS WINAPI wow64_NtUserGetWindowPlacement( UINT *args )
{
    HWND hwnd = get_handle( &args );
    WINDOWPLACEMENT *placement = get_ptr( &args );

    return NtUserGetWindowPlacement( hwnd, placement );
}

NTSTATUS WINAPI wow64_NtUserGetWindowRgnEx( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HRGN hrgn = get_handle( &args );
    UINT unk = get_ulong( &args );

    return NtUserGetWindowRgnEx( hwnd, hrgn, unk );
}

NTSTATUS WINAPI wow64_NtUserHideCaret( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserHideCaret( hwnd );
}

NTSTATUS WINAPI wow64_NtUserHiliteMenuItem( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HMENU handle = get_handle( &args );
    UINT item = get_ulong( &args );
    UINT hilite = get_ulong( &args );

    return NtUserHiliteMenuItem( hwnd, handle, item, hilite );
}

NTSTATUS WINAPI wow64_NtUserInitializeClientPfnArrays( UINT *args )
{
    const ntuser_client_func_ptr *procsA = get_ptr( &args );
    const ntuser_client_func_ptr *procsW = get_ptr( &args );
    const ntuser_client_func_ptr *workers = get_ptr( &args );
    HINSTANCE user_module = get_ptr( &args );

    return NtUserInitializeClientPfnArrays( procsA, procsW, workers, user_module );
}

NTSTATUS WINAPI wow64_NtUserInternalGetWindowIcon( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT type = get_ulong( &args );

    return HandleToUlong( NtUserInternalGetWindowIcon( hwnd, type ));
}

NTSTATUS WINAPI wow64_NtUserInternalGetWindowText( UINT *args )
{
    HWND hwnd = get_handle( &args );
    WCHAR *text = get_ptr( &args );
    INT count = get_ulong( &args );

    return NtUserInternalGetWindowText( hwnd, text, count );
}

NTSTATUS WINAPI wow64_NtUserInvalidateRect( UINT *args )
{
    HWND hwnd = get_handle( &args );
    const RECT *rect = get_ptr( &args );
    BOOL erase = get_ulong( &args );

    return NtUserInvalidateRect( hwnd, rect, erase );
}

NTSTATUS WINAPI wow64_NtUserInvalidateRgn( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HRGN hrgn = get_handle( &args );
    BOOL erase = get_ulong( &args );

    return NtUserInvalidateRgn( hwnd, hrgn, erase );
}

NTSTATUS WINAPI wow64_NtUserIsChildWindowDpiMessageEnabled( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserIsChildWindowDpiMessageEnabled( hwnd );
}

NTSTATUS WINAPI wow64_NtUserIsClipboardFormatAvailable( UINT *args )
{
    UINT format = get_ulong( &args );

    return NtUserIsClipboardFormatAvailable( format );
}

NTSTATUS WINAPI wow64_NtUserIsMouseInPointerEnabled( UINT *args )
{
    return NtUserIsMouseInPointerEnabled();
}

NTSTATUS WINAPI wow64_NtUserKillSystemTimer( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT_PTR id = get_ulong( &args );

    return NtUserKillSystemTimer( hwnd, id );
}

NTSTATUS WINAPI wow64_NtUserKillTimer( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT_PTR id = get_ulong( &args );

    return NtUserKillTimer( hwnd, id );
}

NTSTATUS WINAPI wow64_NtUserLockWindowUpdate( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserLockWindowUpdate( hwnd );
}

NTSTATUS WINAPI wow64_NtUserLogicalToPerMonitorDPIPhysicalPoint( UINT *args )
{
    HWND hwnd = get_handle( &args );
    POINT *pt = get_ptr( &args );

    return NtUserLogicalToPerMonitorDPIPhysicalPoint( hwnd, pt );
}

NTSTATUS WINAPI wow64_NtUserMapVirtualKeyEx( UINT *args )
{
    UINT code = get_ulong( &args );
    UINT type = get_ulong( &args );
    HKL layout = get_handle( &args );

    return NtUserMapVirtualKeyEx( code, type, layout );
}

NTSTATUS WINAPI wow64_NtUserMenuItemFromPoint( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HMENU handle = get_handle( &args );
    int x = get_ulong( &args );
    int y = get_ulong( &args );

    return NtUserMenuItemFromPoint( hwnd, handle, x, y );
}

NTSTATUS WINAPI wow64_NtUserMessageBeep( UINT *args )
{
    UINT type = get_ulong( &args );

    return NtUserMessageBeep( type );
}

static LRESULT message_call_32to64( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                    void *result_info, DWORD type, BOOL ansi )
{
    LRESULT ret = 0;

    switch (msg)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        if (lparam)
        {
            CREATESTRUCT32 *cs32 = (void *)lparam;
            CREATESTRUCTW cs;

            createstruct_32to64( cs32, &cs );
            ret = NtUserMessageCall( hwnd, msg, wparam, (LPARAM)&cs, result_info, type, ansi );
            cs32->lpCreateParams = PtrToUlong( cs.lpCreateParams );
            cs32->hInstance      = PtrToUlong( cs.hInstance );
            cs32->hMenu          = HandleToLong( cs.hMenu );
            cs32->hwndParent     = HandleToLong( cs.hwndParent );
            cs32->cy             = cs.cy;
            cs32->cx             = cs.cx;
            cs32->y              = cs.y;
            cs32->x              = cs.x;
            cs32->style          = cs.style;
            cs32->dwExStyle      = cs.dwExStyle;
            return ret;
        }
        return NtUserMessageCall( hwnd, msg, wparam, lparam, result_info, type, ansi );

    case WM_MDICREATE:
        {
            MDICREATESTRUCT32 *cs32 = (void *)lparam;
            MDICREATESTRUCTW cs;

            cs.szClass = UlongToPtr( cs32->szClass );
            cs.szTitle = UlongToPtr( cs32->szTitle );
            cs.hOwner = LongToHandle( cs32->hOwner );
            cs.x = cs32->x;
            cs.y = cs32->y;
            cs.cx = cs32->cx;
            cs.cy = cs32->cy;
            cs.style = cs32->style;
            cs.lParam = cs32->lParam;

            return NtUserMessageCall( hwnd, msg, wparam, (LPARAM)&cs, result_info, type, ansi );
        }

    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS32 *winpos32 = (void *)lparam;
            WINDOWPOS winpos;

            winpos_32to64( &winpos, winpos32 );
            ret = NtUserMessageCall( hwnd, msg, wparam, (LPARAM)&winpos, result_info, type, ansi );
            winpos_64to32( &winpos, winpos32 );
            return ret;
        }

    case WM_NCCALCSIZE:
        if (wparam)
        {
            NCCALCSIZE_PARAMS32 *params32 = (void *)lparam;
            NCCALCSIZE_PARAMS params;
            WINDOWPOS winpos;

            params.rgrc[0] = params32->rgrc[0];
            params.rgrc[1] = params32->rgrc[1];
            params.rgrc[2] = params32->rgrc[2];
            params.lppos = &winpos;
            winpos_32to64( &winpos, UlongToPtr( params32->lppos ));
            ret = NtUserMessageCall( hwnd, msg, wparam, (LPARAM)&params, result_info, type, ansi );
            params32->rgrc[0] = params.rgrc[0];
            params32->rgrc[1] = params.rgrc[1];
            params32->rgrc[2] = params.rgrc[2];
            winpos_64to32( &winpos, UlongToPtr( params32->lppos ));
            return ret;
        }
        return NtUserMessageCall( hwnd, msg, wparam, lparam, result_info, type, ansi );

    case WM_COMPAREITEM:
        {
            COMPAREITEMSTRUCT32 *cis32 = (void *)lparam;
            COMPAREITEMSTRUCT cis;

            cis.CtlType    = cis32->CtlType;
            cis.CtlID      = cis32->CtlID;
            cis.hwndItem   = LongToHandle( cis32->hwndItem );
            cis.itemID1    = cis32->itemID1;
            cis.itemData1  = cis32->itemData1;
            cis.itemID2    = cis32->itemID2;
            cis.itemData2  = cis32->itemData2;
            cis.dwLocaleId = cis32->dwLocaleId;
            return NtUserMessageCall( hwnd, msg, wparam, (LPARAM)&cis, result_info, type, ansi );
        }

    case WM_DELETEITEM:
        {
            DELETEITEMSTRUCT32 *dis32 = (void *)lparam;
            DELETEITEMSTRUCT dis;

            dis.CtlType  = dis32->CtlType;
            dis.CtlID    = dis32->CtlID;
            dis.itemID   = dis32->itemID;
            dis.hwndItem = LongToHandle( dis32->hwndItem );
            dis.itemData = dis32->itemData;
            return NtUserMessageCall( hwnd, msg, wparam, (LPARAM)&dis, result_info, type, ansi );
        }

    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT32 *mis32 = (void *)lparam;
            MEASUREITEMSTRUCT mis;

            mis.CtlType    = mis32->CtlType;
            mis.CtlID      = mis32->CtlID;
            mis.itemID     = mis32->itemID;
            mis.itemWidth  = mis32->itemWidth;
            mis.itemHeight = mis32->itemHeight;
            mis.itemData   = mis32->itemData;
            ret = NtUserMessageCall( hwnd, msg, wparam, (LPARAM)&mis, result_info, type, ansi );
            mis32->CtlType    = mis.CtlType;
            mis32->CtlID      = mis.CtlID;
            mis32->itemID     = mis.itemID;
            mis32->itemWidth  = mis.itemWidth;
            mis32->itemHeight = mis.itemHeight;
            mis32->itemData   = mis.itemData;
            return ret;
        }

    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT32 *dis32 = (void *)lparam;
            DRAWITEMSTRUCT dis;

            dis.CtlType       = dis32->CtlType;
            dis.CtlID         = dis32->CtlID;
            dis.itemID        = dis32->itemID;
            dis.itemAction    = dis32->itemAction;
            dis.itemState     = dis32->itemState;
            dis.hwndItem      = LongToHandle( dis32->hwndItem );
            dis.hDC           = LongToHandle( dis32->hDC );
            dis.itemData      = dis32->itemData;
            dis.rcItem.left   = dis32->rcItem.left;
            dis.rcItem.top    = dis32->rcItem.top;
            dis.rcItem.right  = dis32->rcItem.right;
            dis.rcItem.bottom = dis32->rcItem.bottom;
            return NtUserMessageCall( hwnd, msg, wparam, (LPARAM)&dis, result_info, type, ansi );
        }

    case WM_COPYDATA:
        {
            COPYDATASTRUCT32 *cds32 = (void *)lparam;
            COPYDATASTRUCT cds;

            cds.dwData = cds32->dwData;
            cds.cbData = cds32->cbData;
            cds.lpData = UlongToPtr( cds32->lpData );
            return NtUserMessageCall( hwnd, msg, wparam, (LPARAM)&cds, result_info, type, ansi );
        }

    case WM_HELP:
        {
            HELPINFO32 *hi32 = (void *)lparam;
            HELPINFO hi64;

            hi64.cbSize       = sizeof(hi64);
            hi64.iContextType = hi32->iContextType;
            hi64.iCtrlId      = hi32->iCtrlId;
            hi64.hItemHandle  = LongToHandle( hi32->hItemHandle );
            hi64.dwContextId  = hi32->dwContextId;
            hi64.MousePos     = hi32->MousePos;
            return NtUserMessageCall( hwnd, msg, wparam, (LPARAM)&hi64, result_info, type, ansi );
        }

    case WM_GETDLGCODE:
        if (lparam)
        {
            MSG32 *msg32 = (MSG32 *)lparam;
            MSG msg64;

            return NtUserMessageCall( hwnd, msg, wparam, (LPARAM)msg_32to64( &msg64, msg32 ),
                                      result_info, type, ansi );
        }
        return NtUserMessageCall( hwnd, msg, wparam, lparam, result_info, type, ansi );

    case WM_NEXTMENU:
        {
            MDINEXTMENU32 *next32 = (void *)lparam;
            MDINEXTMENU next;

            next.hmenuIn   = LongToHandle( next32->hmenuIn );
            next.hmenuNext = LongToHandle( next32->hmenuNext );
            next.hwndNext  = LongToHandle( next32->hwndNext );
            ret = NtUserMessageCall( hwnd, msg, wparam, (LPARAM)&next, result_info, type, ansi );
            next32->hmenuIn   = HandleToLong( next.hmenuIn );
            next32->hmenuNext = HandleToLong( next.hmenuNext );
            next32->hwndNext  = HandleToLong( next.hwndNext );
            return ret;
        }

    case WM_PAINTCLIPBOARD:
        {
            PAINTSTRUCT ps;

            paintstruct_32to64( &ps, (PAINTSTRUCT32 *)lparam );
            return NtUserMessageCall( hwnd, msg, wparam, (LPARAM)&ps, result_info, type, ansi );
        }

    case CB_GETCOMBOBOXINFO:
        {
            COMBOBOXINFO32 *ci32 = (COMBOBOXINFO32 *)lparam;
            COMBOBOXINFO ci;

            ci.cbSize      = ci32->cbSize;
            ci.rcItem      = ci32->rcItem;
            ci.rcButton    = ci32->rcButton;
            ci.stateButton = ci32->stateButton;
            ci.hwndCombo   = LongToHandle( ci32->hwndCombo );
            ci.hwndItem    = LongToHandle( ci32->hwndItem );
            ci.hwndList    = LongToHandle( ci32->hwndList );
            ret = NtUserMessageCall( hwnd, msg, wparam, (LPARAM)&ci, result_info, type, ansi );
            ci32->cbSize      = ci.cbSize;
            ci32->rcItem      = ci.rcItem;
            ci32->rcButton    = ci.rcButton;
            ci32->stateButton = ci.stateButton;
            ci32->hwndCombo   = HandleToLong( ci.hwndCombo );
            ci32->hwndItem    = HandleToLong( ci.hwndItem );
            ci32->hwndList    = HandleToLong( ci.hwndList );
            return ret;
        }

    default:
        return NtUserMessageCall( hwnd, msg, wparam, lparam, result_info, type, ansi );
    }
}

NTSTATUS WINAPI wow64_NtUserMessageCall( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT msg = get_ulong( &args );
    LONG wparam = get_ulong( &args );
    LONG lparam = get_ulong( &args );
    void *result_info = get_ptr( &args );
    UINT type = get_ulong ( &args );
    BOOL ansi = get_ulong( &args );

    switch (type)
    {
    case NtUserGetDispatchParams:
    case NtUserCallWindowProc:
        {
            struct win_proc_params32 *params32 = result_info;
            struct win_proc_params params;

            if (type == NtUserCallWindowProc) params.func = UlongToPtr( params32->func );

            if (!NtUserMessageCall( hwnd, msg, wparam, lparam, &params, type, ansi ))
                return FALSE;

            win_proc_params_64to32( &params, params32 );
            return TRUE;
        }

    case NtUserSendMessage:
        {
            struct win_proc_params32 *params32 = result_info;

            if (params32)
            {
                struct win_proc_params params;
                NTSTATUS ret;

                params.hwnd = 0;
                ret = message_call_32to64( hwnd, msg, wparam, lparam, &params, type, ansi );
                if (params.hwnd) win_proc_params_64to32( &params, params32 );
                return ret;
            }

            return message_call_32to64( hwnd, msg, wparam, lparam, result_info, type, ansi );
        }

    case NtUserSendMessageTimeout:
        {
            struct
            {
                UINT flags;
                UINT timeout;
                DWORD result;
            } *params32 = result_info;
            struct send_message_timeout_params params;
            LRESULT ret;

            params.flags = params32->flags;
            params.timeout = params32->timeout;
            ret = message_call_32to64( hwnd, msg, wparam, lparam, &params, type, ansi );
            params32->result = params.result;
            return ret;
        }

    case NtUserSendMessageCallback:
        {
            struct
            {
                ULONG callback;
                ULONG data;
            } *params32 = result_info;
            struct send_message_callback_params params;

            params.callback = UlongToPtr( params32->callback );
            params.data = params32->data;
            return message_call_32to64( hwnd, msg, wparam, lparam, &params, type, ansi );
        }

    case NtUserSpyGetMsgName:
        /* no argument conversion */
        return NtUserMessageCall( hwnd, msg, wparam, lparam, result_info, type, ansi );

    case NtUserImeDriverCall:
        {
            struct
            {
                ULONG himc;
                ULONG state;
                ULONG compstr;
                ULONG key_consumed;
            } *params32 = result_info;
            struct ime_driver_call_params params;
            if (msg == WINE_IME_POST_UPDATE) ERR( "Unexpected WINE_IME_POST_UPDATE message\n" );
            params.himc = UlongToPtr( params32->himc );
            params.state = UlongToPtr( params32->state );
            params.compstr = UlongToPtr( params32->compstr );
            params.key_consumed = UlongToPtr( params32->key_consumed );
            return NtUserMessageCall( hwnd, msg, wparam, lparam, &params, type, ansi );
        }

    case NtUserSystemTrayCall:
        switch (msg)
        {
        case WINE_SYSTRAY_NOTIFY_ICON:
        {
            struct
            {
                DWORD cbSize;
                ULONG hWnd;
                UINT uID;
                UINT uFlags;
                UINT uCallbackMessage;
                ULONG hIcon;
                WCHAR szTip[128];
                DWORD dwState;
                DWORD dwStateMask;
                WCHAR szInfo[256];
                UINT uTimeout;
                WCHAR szInfoTitle[64];
                DWORD dwInfoFlags;
                GUID guidItem;
                ULONG hBalloonIcon;
            } *params32 = result_info;

            NOTIFYICONDATAW params = {.cbSize = sizeof(params)};
            params.hWnd = UlongToHandle( params32->hWnd );
            params.uID = params32->uID;
            params.uFlags = params32->uFlags;
            params.uCallbackMessage = params32->uCallbackMessage;
            params.hIcon = UlongToHandle( params32->hIcon );
            if (params.uFlags & NIF_TIP) wcscpy( params.szTip, params32->szTip );
            params.dwState = params32->dwState;
            params.dwStateMask = params32->dwStateMask;

            if (params.uFlags & NIF_INFO)
            {
                wcscpy( params.szInfoTitle, params32->szInfoTitle );
                wcscpy( params.szInfo, params32->szInfo );
                params.uTimeout = params32->uTimeout;
                params.dwInfoFlags = params32->dwInfoFlags;
            }

            params.guidItem = params32->guidItem;
            params.hBalloonIcon = UlongToHandle( params32->hBalloonIcon );

            return NtUserMessageCall( hwnd, msg, wparam, lparam, &params, type, ansi );
        }

        default:
            return NtUserMessageCall( hwnd, msg, wparam, lparam, result_info, type, ansi );
        }

    case NtUserDragDropCall:
        if (msg == WINE_DRAG_DROP_ENTER)
        {
            ULONG *data32 = result_info;
            NTSTATUS status;
            void *data;

            status = NtUserMessageCall( hwnd, msg, wparam, lparam, &data, type, ansi );
            if (!status) *data32 = HandleToUlong( data );
            return status;
        }
        return NtUserMessageCall( hwnd, msg, wparam, lparam, result_info, type, ansi );

    case NtUserPostDdeCall:
        {
            struct
            {
                ULONG ptr;
                UINT  size;
                DWORD dest_tid;
            } *params32 = result_info;
            struct post_dde_message_call_params params;

            params.ptr = UlongToPtr(params32->ptr);
            params.size = params32->size;
            params.dest_tid = params32->dest_tid;
            return NtUserMessageCall( hwnd, msg, wparam, lparam, &params, type, ansi );
        }
    }

    return message_call_32to64( hwnd, msg, wparam, lparam, result_info, type, ansi );
}

NTSTATUS WINAPI wow64_NtUserMoveWindow( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT x = get_ulong( &args );
    INT y = get_ulong( &args );
    INT cx = get_ulong( &args );
    INT cy = get_ulong( &args );
    BOOL repaint = get_ulong( &args );

    return NtUserMoveWindow( hwnd, x, y, cx, cy, repaint );
}

NTSTATUS WINAPI wow64_NtUserMsgWaitForMultipleObjectsEx( UINT *args )
{
    DWORD count = get_ulong( &args );
    const ULONG *handles32 = get_ptr( &args );
    DWORD timeout = get_ulong( &args );
    DWORD mask = get_ulong( &args );
    DWORD flags = get_ulong( &args );

    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    unsigned int i;

    if (count > ARRAYSIZE(handles))
    {
        set_last_error32( ERROR_INVALID_PARAMETER );
        return WAIT_FAILED;
    }
    for (i = 0; i < count; i++) handles[i] = LongToHandle( handles32[i] );

    return NtUserMsgWaitForMultipleObjectsEx( count, handles, timeout, mask, flags );
}

NTSTATUS WINAPI wow64_NtUserNotifyIMEStatus( UINT *args )
{
    HWND hwnd = get_handle( &args );
    ULONG status = get_ulong( &args );

    NtUserNotifyIMEStatus( hwnd, status );
    return 0;
}

NTSTATUS WINAPI wow64_NtUserNotifyWinEvent( UINT *args )
{
    DWORD event = get_ulong( &args );
    HWND hwnd = get_handle( &args );
    LONG object_id = get_ulong( &args );
    LONG child_id = get_ulong( &args );

    NtUserNotifyWinEvent( event, hwnd, object_id, child_id );
    return 0;
}

NTSTATUS WINAPI wow64_NtUserOpenClipboard( UINT *args )
{
    HWND hwnd = get_handle( &args );
    ULONG unk = get_ulong( &args );

    return NtUserOpenClipboard( hwnd, unk );
}

NTSTATUS WINAPI wow64_NtUserOpenDesktop( UINT *args )
{
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    DWORD flags = get_ulong( &args );
    ACCESS_MASK access = get_ulong( &args );

    struct object_attr64 attr;
    HANDLE ret;

    ret = NtUserOpenDesktop( objattr_32to64( &attr, attr32 ), flags, access );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserOpenInputDesktop( UINT *args )
{
    DWORD flags = get_ulong( &args );
    BOOL inherit = get_ulong( &args );
    ACCESS_MASK access = get_ulong( &args );

    return HandleToUlong( NtUserOpenInputDesktop( flags, inherit, access ));
}

NTSTATUS WINAPI wow64_NtUserOpenWindowStation( UINT *args )
{
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );

    struct object_attr64 attr;

    return HandleToUlong( NtUserOpenWindowStation( objattr_32to64( &attr, attr32 ), access ));
}

NTSTATUS WINAPI wow64_NtUserPeekMessage( UINT *args )
{
    MSG32 *msg32 = get_ptr( &args );
    HWND hwnd = get_handle( &args );
    UINT first = get_ulong( &args );
    UINT last = get_ulong( &args );
    UINT flags = get_ulong( &args );
    MSG msg;

    if (!NtUserPeekMessage( msg32 ? &msg : NULL, hwnd, first, last, flags )) return FALSE;
    msg_64to32( &msg, msg32 );
    return TRUE;
}

NTSTATUS WINAPI wow64_NtUserPerMonitorDPIPhysicalToLogicalPoint( UINT *args )
{
    HWND hwnd = get_handle( &args );
    POINT *pt = get_ptr( &args );

    return NtUserPerMonitorDPIPhysicalToLogicalPoint( hwnd, pt );
}

NTSTATUS WINAPI wow64_NtUserPostMessage( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT msg = get_ulong( &args );
    WPARAM wparam = get_ulong( &args );
    LPARAM lparam = get_ulong( &args );

    return NtUserPostMessage( hwnd, msg, wparam, lparam );
}

NTSTATUS WINAPI wow64_NtUserPostQuitMessage( UINT *args )
{
    INT exit_code = get_ulong( &args );

    return NtUserPostQuitMessage( exit_code );
}

NTSTATUS WINAPI wow64_NtUserPostThreadMessage( UINT *args )
{
    DWORD thread = get_ulong( &args );
    UINT msg = get_ulong( &args );
    WPARAM wparam = get_ulong( &args );
    LPARAM lparam = get_ulong( &args );

    return NtUserPostThreadMessage( thread, msg, wparam, lparam );
}

NTSTATUS WINAPI wow64_NtUserPrintWindow( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HDC hdc = get_handle( &args );
    UINT flags = get_ulong( &args );

    return NtUserPrintWindow( hwnd, hdc, flags );
}

NTSTATUS WINAPI wow64_NtUserQueryDisplayConfig( UINT *args )
{
    UINT32 flags = get_ulong( &args );
    UINT32 *paths_count = get_ptr( &args );
    DISPLAYCONFIG_PATH_INFO *paths = get_ptr( &args );
    UINT32 *modes_count = get_ptr( &args );
    DISPLAYCONFIG_MODE_INFO *modes = get_ptr( &args );
    DISPLAYCONFIG_TOPOLOGY_ID *topology_id = get_ptr( &args );

    return NtUserQueryDisplayConfig( flags, paths_count, paths, modes_count, modes, topology_id );
}

NTSTATUS WINAPI wow64_NtUserQueryInputContext( UINT *args )
{
    HIMC handle = get_handle( &args );
    UINT attr = get_ulong( &args );

    return NtUserQueryInputContext( handle, attr );
}

NTSTATUS WINAPI wow64_NtUserQueryWindow( UINT *args )
{
    HWND hwnd = get_handle( &args );
    WINDOWINFOCLASS cls = get_ulong( &args );

    return HandleToUlong( NtUserQueryWindow( hwnd, cls ));
}

NTSTATUS WINAPI wow64_NtUserRealizePalette( UINT *args )
{
    HDC hdc = get_handle( &args );

    return NtUserRealizePalette( hdc );
}

NTSTATUS WINAPI wow64_NtUserRedrawWindow( UINT *args )
{
    HWND hwnd = get_handle( &args );
    const RECT *rect = get_ptr( &args );
    HRGN hrgn = get_handle( &args );
    UINT flags = get_ulong( &args );

    return NtUserRedrawWindow( hwnd, rect, hrgn, flags );
}

NTSTATUS WINAPI wow64_NtUserRegisterHotKey( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT id = get_ulong( &args );
    UINT modifiers = get_ulong( &args );
    UINT vk = get_ulong( &args );

    return NtUserRegisterHotKey( hwnd, id, modifiers, vk );
}

NTSTATUS WINAPI wow64_NtUserRegisterRawInputDevices( UINT *args )
{
    const RAWINPUTDEVICE32 *devices32 = get_ptr( &args );
    UINT count = get_ulong( &args );
    UINT size = get_ulong( &args );

    RAWINPUTDEVICE *devices64;
    unsigned int i;

    if (size != sizeof(RAWINPUTDEVICE32))
    {
        set_last_error32( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!(devices64 = Wow64AllocateTemp( count * sizeof(*devices64) )))
    {
        set_last_error32( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    for (i = 0; i < count; ++i)
    {
        devices64[i].usUsagePage = devices32[i].usUsagePage;
        devices64[i].usUsage = devices32[i].usUsage;
        devices64[i].dwFlags = devices32[i].dwFlags;
        devices64[i].hwndTarget = UlongToPtr( devices32[i].hwndTarget );
    }

    return NtUserRegisterRawInputDevices( devices64, count, sizeof(*devices64) );
}

NTSTATUS WINAPI wow64_NtUserRegisterTouchPadCapable( UINT *args )
{
    UINT capable = get_ulong( &args );

    return NtUserRegisterTouchPadCapable( capable );
}

NTSTATUS WINAPI wow64_NtUserReleaseCapture( UINT *args )
{
    return NtUserReleaseCapture();
}

NTSTATUS WINAPI wow64_NtUserReleaseDC( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HDC hdc = get_handle( &args );

    return NtUserReleaseDC( hwnd, hdc );
}

NTSTATUS WINAPI wow64_NtUserRemoveClipboardFormatListener( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserRemoveClipboardFormatListener( hwnd );
}

NTSTATUS WINAPI wow64_NtUserRegisterWindowMessage( UINT *args )
{
    UNICODE_STRING32 *name32 = get_ptr( &args );
    UNICODE_STRING name;

    return NtUserRegisterWindowMessage( unicode_str_32to64( &name, name32 ));
}

NTSTATUS WINAPI wow64_NtUserRemoveMenu( UINT *args )
{
    HMENU handle = get_handle( &args );
    UINT id = get_ulong( &args );
    UINT flags = get_ulong( &args );

    return NtUserRemoveMenu( handle, id, flags );
}

NTSTATUS WINAPI wow64_NtUserRemoveProp( UINT *args )
{
    HWND hwnd = get_handle( &args );
    const WCHAR *str = get_ptr( &args );

    return HandleToUlong( NtUserRemoveProp( hwnd, str ));
}

NTSTATUS WINAPI wow64_NtUserReplyMessage( UINT *args )
{
    LRESULT result = get_ulong( &args );

    return NtUserReplyMessage( result );
}

NTSTATUS WINAPI wow64_NtUserScheduleDispatchNotification( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserScheduleDispatchNotification( hwnd );
}

NTSTATUS WINAPI wow64_NtUserScrollDC( UINT *args )
{
    HDC hdc = get_handle( &args );
    INT dx = get_ulong( &args );
    INT dy = get_ulong( &args );
    const RECT *scroll = get_ptr( &args );
    const RECT *clip = get_ptr( &args );
    HRGN ret_update_rgn = get_handle( &args );
    RECT *update_rect = get_ptr( &args );

    return NtUserScrollDC( hdc, dx, dy, scroll, clip, ret_update_rgn, update_rect );
}

NTSTATUS WINAPI wow64_NtUserScrollWindowEx( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT dx = get_ulong( &args );
    INT dy = get_ulong( &args );
    const RECT *rect = get_ptr( &args );
    const RECT *clip_rect = get_ptr( &args );
    HRGN update_rgn = get_handle( &args );
    RECT *update_rect = get_ptr( &args );
    UINT flags = get_ulong( &args );

    return NtUserScrollWindowEx( hwnd, dx, dy, rect, clip_rect, update_rgn, update_rect, flags );
}

NTSTATUS WINAPI wow64_NtUserSelectPalette( UINT *args )
{
    HDC hdc = get_handle( &args );
    HPALETTE hpal = get_handle( &args );
    WORD bkg = get_ulong( &args );

    return HandleToUlong( NtUserSelectPalette( hdc, hpal, bkg ));
}

NTSTATUS WINAPI wow64_NtUserSendInput( UINT *args )
{
    UINT count = get_ulong( &args );
    INPUT32 *inputs32 = get_ptr( &args );
    int size = get_ulong( &args );

    INPUT *inputs = NULL;
    unsigned int i;

    if (size != sizeof(*inputs32) || !count)
    {
        set_last_error32( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (!inputs32)
    {
        set_last_error32( ERROR_NOACCESS );
        return 0;
    }

    if (count && !(inputs = Wow64AllocateTemp( count * sizeof(*inputs) )))
        return 0;

    for (i = 0; i < count; i++)
    {
        inputs[i].type = inputs32[i].type;
        switch (inputs[i].type)
        {
        case INPUT_MOUSE:
            inputs[i].mi.dx          = inputs32[i].mi.dx;
            inputs[i].mi.dy          = inputs32[i].mi.dy;
            inputs[i].mi.mouseData   = inputs32[i].mi.mouseData;
            inputs[i].mi.dwFlags     = inputs32[i].mi.dwFlags;
            inputs[i].mi.time        = inputs32[i].mi.time;
            inputs[i].mi.dwExtraInfo = inputs32[i].mi.dwExtraInfo;
            break;
        case INPUT_KEYBOARD:
            inputs[i].ki.wVk         = inputs32[i].ki.wVk;
            inputs[i].ki.wScan       = inputs32[i].ki.wScan;
            inputs[i].ki.dwFlags     = inputs32[i].ki.dwFlags;
            inputs[i].ki.time        = inputs32[i].ki.time;
            inputs[i].ki.dwExtraInfo = inputs32[i].ki.dwExtraInfo;
            break;
        case INPUT_HARDWARE:
            inputs[i].hi = inputs32[i].hi;
            break;
        }
    }

    return NtUserSendInput( count, inputs, sizeof(*inputs) );
}

NTSTATUS WINAPI wow64_NtUserSetActiveWindow( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return HandleToUlong( NtUserSetActiveWindow( hwnd ));
}

NTSTATUS WINAPI wow64_NtUserSetAdditionalForegroundBoostProcesses( UINT *args )
{
    HWND hwnd = get_handle( &args );
    DWORD count = get_ulong( &args );
    ULONG *handles32 = get_ptr( &args );

    HANDLE handles[32];
    unsigned int i;

    if (count > ARRAYSIZE(handles))
    {
        set_last_error32( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    for (i = 0; i < count; i++) handles[i] = LongToHandle( handles32[i] );

    return NtUserSetAdditionalForegroundBoostProcesses( hwnd, count, handles );
}

NTSTATUS WINAPI wow64_NtUserSetCapture( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return HandleToUlong( NtUserSetCapture( hwnd ));
}

NTSTATUS WINAPI wow64_NtUserSetCaretBlinkTime( UINT *args )
{
    unsigned int time = get_ulong( &args );

    return NtUserSetCaretBlinkTime( time );
}

NTSTATUS WINAPI wow64_NtUserSetCaretPos( UINT *args )
{
    INT x = get_ulong( &args );
    INT y = get_ulong( &args );

    return NtUserSetCaretPos( x, y );
}

NTSTATUS WINAPI wow64_NtUserSetClassLong( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT offset = get_ulong( &args );
    LONG newval = get_ulong( &args );
    BOOL ansi = get_ulong( &args );

    return NtUserSetClassLong( hwnd, offset, newval, ansi );
}

NTSTATUS WINAPI wow64_NtUserSetClassLongPtr( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT offset = get_ulong( &args );
    LONG_PTR newval = get_ulong( &args );
    BOOL ansi = get_ulong( &args );

    if (offset == GCLP_MENUNAME)
    {
        struct client_menu_name menu_name;
        struct client_menu_name32 *menu_name32 = UlongToPtr( newval );
        NtUserSetClassLongPtr( hwnd, offset,
                               (UINT_PTR)client_menu_name_32to64( &menu_name, menu_name32 ), ansi );
        client_menu_name_64to32( &menu_name, menu_name32 );
        return 0;
    }

    return NtUserSetClassLongPtr( hwnd, offset, newval, ansi );
}

NTSTATUS WINAPI wow64_NtUserSetClassWord( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT offset = get_ulong( &args );
    WORD newval = get_ulong( &args );

    return NtUserSetClassWord( hwnd, offset, newval );
}

NTSTATUS WINAPI wow64_NtUserSetClipboardData( UINT *args )
{
    UINT format = get_ulong( &args );
    HANDLE handle = get_handle( &args );
    struct
    {
        UINT32 data;
        UINT32 size;
        BOOL   cache_only;
        UINT   seqno;
    } *params32 = get_ptr( &args );

    struct set_clipboard_params params;
    params.data       = UlongToPtr( params32->data );
    params.size       = params32->size;
    params.cache_only = params32->cache_only;
    params.seqno      = params32->seqno;

    return NtUserSetClipboardData( format, handle, &params );
}

NTSTATUS WINAPI wow64_NtUserSetClipboardViewer( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return HandleToUlong( NtUserSetClipboardViewer( hwnd ));
}

NTSTATUS WINAPI wow64_NtUserSetCursor( UINT *args )
{
    HCURSOR cursor = get_handle( &args );

    return HandleToUlong( NtUserSetCursor( cursor ));
}

NTSTATUS WINAPI wow64_NtUserSetCursorIconData( UINT *args )
{
    HCURSOR cursor = get_handle( &args );
    UNICODE_STRING32 *module32 = get_ptr( &args );
    UNICODE_STRING32 *res_name32 = get_ptr( &args );
    struct
    {
        UINT  flags;
        UINT  num_steps;
        UINT  num_frames;
        UINT  delay;
        ULONG frames;
        ULONG frame_seq;
        ULONG frame_rates;
        ULONG rsrc;
    } *desc32 = get_ptr( &args );
    struct
    {
        UINT  width;
        UINT  height;
        ULONG color;
        ULONG alpha;
        ULONG mask;
        POINT hotspot;
    } *frames32 = UlongToPtr( desc32->frames );

    UNICODE_STRING module, res_name;
    struct cursoricon_desc desc;
    UINT i, num_frames;

    num_frames = max( desc32->num_frames, 1 );
    if (!(desc.frames = Wow64AllocateTemp( num_frames * sizeof(*desc.frames) ))) return FALSE;
    desc.flags = desc32->flags;
    desc.num_steps = desc32->num_steps;
    desc.num_frames = desc32->num_frames;
    desc.delay = desc32->delay;
    desc.frame_seq = UlongToPtr( desc32->frame_seq );
    desc.frame_rates = UlongToPtr( desc32->frame_rates );
    desc.rsrc = UlongToPtr( desc32->rsrc );

    for (i = 0; i < num_frames; i++)
    {
        desc.frames[i].width = frames32[i].width;
        desc.frames[i].height = frames32[i].height;
        desc.frames[i].color = UlongToHandle( frames32[i].color );
        desc.frames[i].alpha = UlongToHandle( frames32[i].alpha );
        desc.frames[i].mask = UlongToHandle( frames32[i].mask );
        desc.frames[i].hotspot = frames32[i].hotspot;
    }

    return NtUserSetCursorIconData( cursor, unicode_str_32to64( &module, module32 ),
                                    unicode_str_32to64( &res_name, res_name32), &desc );
}

NTSTATUS WINAPI wow64_NtUserSetCursorPos( UINT *args )
{
    INT x = get_ulong( &args );
    INT y = get_ulong( &args );

    return NtUserSetCursorPos( x, y );
}

NTSTATUS WINAPI wow64_NtUserSetFocus( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return HandleToUlong( NtUserSetFocus( hwnd ));
}

NTSTATUS WINAPI wow64_NtUserSetForegroundWindow( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserSetForegroundWindow( hwnd );
}

NTSTATUS WINAPI wow64_NtUserSetInternalWindowPos( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT cmd = get_ulong( &args );
    RECT *rect = get_ptr( &args );
    POINT *pt = get_ptr( &args );

    NtUserSetInternalWindowPos( hwnd, cmd, rect, pt );
    return 0;
}

NTSTATUS WINAPI wow64_NtUserSetKeyboardState( UINT *args )
{
    BYTE *state = get_ptr( &args );

    return NtUserSetKeyboardState( state );
}

NTSTATUS WINAPI wow64_NtUserSetLayeredWindowAttributes( UINT *args )
{
    HWND hwnd = get_handle( &args );
    COLORREF key = get_ulong( &args );
    BYTE alpha = get_ulong( &args );
    DWORD flags = get_ulong( &args );

    return NtUserSetLayeredWindowAttributes( hwnd, key, alpha, flags );
}

NTSTATUS WINAPI wow64_NtUserSetMenu( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HMENU menu = get_handle( &args );

    return NtUserSetMenu( hwnd, menu );
}

NTSTATUS WINAPI wow64_NtUserSetMenuContextHelpId( UINT *args )
{
    HMENU menu = get_handle( &args );
    DWORD id = get_ulong( &args );

    return NtUserSetMenuContextHelpId( menu, id );
}

NTSTATUS WINAPI wow64_NtUserSetMenuDefaultItem( UINT *args )
{
    HMENU handle = get_handle( &args );
    UINT item = get_ulong( &args );
    UINT bypos = get_ulong( &args );

    return NtUserSetMenuDefaultItem( handle, item, bypos );
}

NTSTATUS WINAPI wow64_NtUserSetObjectInformation( UINT *args )
{
    HANDLE handle = get_handle( &args );
    INT index = get_ulong( &args );
    void *info = get_ptr( &args );
    DWORD len = get_ulong( &args );

    return NtUserSetObjectInformation( handle, index, info, len );
}

NTSTATUS WINAPI wow64_NtUserSetParent( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HWND parent = get_handle( &args );

    return HandleToUlong( NtUserSetParent( hwnd, parent ));
}

NTSTATUS WINAPI wow64_NtUserSetProcessDefaultLayout( UINT *args )
{
    UINT layout = get_ulong( &args );

    return NtUserSetProcessDefaultLayout( layout );
}

NTSTATUS WINAPI wow64_NtUserSetProcessDpiAwarenessContext( UINT *args )
{
    ULONG awareness = get_ulong( &args );
    ULONG unknown = get_ulong( &args );

    return NtUserSetProcessDpiAwarenessContext( awareness, unknown );
}

NTSTATUS WINAPI wow64_NtUserSetProcessWindowStation( UINT *args )
{
    HWINSTA handle = get_handle( &args );

    return NtUserSetProcessWindowStation( handle );
}

NTSTATUS WINAPI wow64_NtUserSetProgmanWindow( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return HandleToUlong( NtUserSetProgmanWindow( hwnd ));
}

NTSTATUS WINAPI wow64_NtUserSetProp( UINT *args )
{
    HWND hwnd = get_handle( &args );
    const WCHAR *str = get_ptr( &args );
    HANDLE handle = get_handle( &args );

    return NtUserSetProp( hwnd, str, handle );
}

NTSTATUS WINAPI wow64_NtUserSetScrollInfo( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT bar = get_ulong( &args );
    const SCROLLINFO *info = get_ptr( &args );
    BOOL redraw = get_ulong( &args );

    return NtUserSetScrollInfo( hwnd, bar, info, redraw );
}

NTSTATUS WINAPI wow64_NtUserSetShellWindowEx( UINT *args )
{
    HWND shell = get_handle( &args );
    HWND list_view = get_handle( &args );

    return NtUserSetShellWindowEx( shell, list_view );
}

NTSTATUS WINAPI wow64_NtUserSetSysColors( UINT *args )
{
    INT count = get_ulong( &args );
    const INT *colors = get_ptr( &args );
    const COLORREF *values = get_ptr( &args );

    return NtUserSetSysColors( count, colors, values );
}

NTSTATUS WINAPI wow64_NtUserSetSystemMenu( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HMENU menu = get_handle( &args );

    return NtUserSetSystemMenu( hwnd, menu );
}

NTSTATUS WINAPI wow64_NtUserSetSystemTimer( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT_PTR id = get_ulong( &args );
    UINT timeout = get_ulong( &args );

    return NtUserSetSystemTimer( hwnd, id, timeout );
}

NTSTATUS WINAPI wow64_NtUserSetTaskmanWindow( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return HandleToUlong( NtUserSetTaskmanWindow( hwnd ));
}

NTSTATUS WINAPI wow64_NtUserSetThreadDesktop( UINT *args )
{
    HDESK handle = get_handle( &args );

    return NtUserSetThreadDesktop( handle );
}

NTSTATUS WINAPI wow64_NtUserSetTimer( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT_PTR id = get_ulong( &args );
    UINT timeout = get_ulong( &args );
    TIMERPROC proc = get_ptr( &args );
    ULONG tolerance = get_ulong( &args );

    return NtUserSetTimer( hwnd, id, timeout, proc, tolerance );
}

NTSTATUS WINAPI wow64_NtUserSetWinEventHook( UINT *args )
{
    DWORD event_min = get_ulong( &args );
    DWORD event_max = get_ulong( &args );
    HMODULE inst = get_ptr( &args );
    UNICODE_STRING32 *module32 = get_ptr( &args );
    WINEVENTPROC proc = get_ptr(&args );
    DWORD pid = get_ulong( &args );
    DWORD tid = get_ulong( &args );
    DWORD flags = get_ulong( &args );
    UNICODE_STRING module;
    HWINEVENTHOOK ret;

    ret = NtUserSetWinEventHook( event_min, event_max, inst,
                                 unicode_str_32to64( &module, module32 ),
                                 proc, pid, tid, flags );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserSetWindowContextHelpId( UINT *args )
{
    HWND hwnd = get_handle( &args );
    DWORD id = get_ulong( &args );

    return NtUserSetWindowContextHelpId( hwnd, id );
}

NTSTATUS WINAPI wow64_NtUserSetWindowLong( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT offset = get_ulong( &args );
    LONG newval = get_ulong( &args );
    BOOL ansi = get_ulong( &args );

    switch (offset)
    {
    case GWLP_HINSTANCE:
    case GWLP_WNDPROC:
        return NtUserSetWindowLongPtr( hwnd, offset, (ULONG)newval, ansi );
    }

    return NtUserSetWindowLong( hwnd, offset, newval, ansi );
}

NTSTATUS WINAPI wow64_NtUserSetWindowLongPtr( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT offset = get_ulong( &args );
    LONG_PTR newval = get_ulong( &args );
    BOOL ansi = get_ulong( &args );

    return NtUserSetWindowLongPtr( hwnd, offset, newval, ansi );
}

NTSTATUS WINAPI wow64_NtUserSetWindowPlacement( UINT *args )
{
    HWND hwnd = get_handle( &args );
    const WINDOWPLACEMENT *wpl = get_ptr( &args );

    return NtUserSetWindowPlacement( hwnd, wpl );
}

NTSTATUS WINAPI wow64_NtUserSetWindowPos( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HWND after = get_handle( &args );
    INT x = get_ulong( &args );
    INT y = get_ulong( &args );
    INT cx = get_ulong( &args );
    INT cy = get_ulong( &args );
    UINT flags = get_ulong( &args );

    return NtUserSetWindowPos( hwnd, after, x, y, cx, cy, flags );
}

NTSTATUS WINAPI wow64_NtUserSetWindowRgn( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HRGN hrgn = get_handle( &args );
    BOOL redraw = get_ulong( &args );

    return NtUserSetWindowRgn( hwnd, hrgn, redraw );
}

NTSTATUS WINAPI wow64_NtUserSetWindowWord( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT offset = get_ulong( &args );
    WORD newval = get_ulong( &args );

    return NtUserSetWindowWord( hwnd, offset, newval );
}

NTSTATUS WINAPI wow64_NtUserSetWindowsHookEx( UINT *args )
{
    HINSTANCE inst = get_ptr( &args );
    UNICODE_STRING32 *module32 = get_ptr( &args );
    DWORD tid = get_ulong( &args );
    INT id = get_ulong( &args );
    HOOKPROC proc = get_ptr( &args );
    BOOL ansi = get_ulong( &args );
    UNICODE_STRING module;
    HHOOK ret;

    ret = NtUserSetWindowsHookEx( inst, unicode_str_32to64( &module, module32 ),
                                  tid, id, proc, ansi );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserShowCaret( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserShowCaret( hwnd );
}

NTSTATUS WINAPI wow64_NtUserShowCursor( UINT *args )
{
    BOOL show = get_ulong( &args );

    return NtUserShowCursor( show );
}

NTSTATUS WINAPI wow64_NtUserShowOwnedPopups( UINT *args )
{
    HWND owner = get_handle( &args );
    BOOL show = get_ulong( &args );

    return NtUserShowOwnedPopups( owner, show );
}

NTSTATUS WINAPI wow64_NtUserShowScrollBar( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT bar = get_ulong( &args );
    BOOL show = get_ulong( &args );

    return NtUserShowScrollBar( hwnd, bar, show );
}

NTSTATUS WINAPI wow64_NtUserShowWindow( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT cmd = get_ulong( &args );

    return NtUserShowWindow( hwnd, cmd );
}

NTSTATUS WINAPI wow64_NtUserShowWindowAsync( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT cmd = get_ulong( &args );

    return NtUserShowWindowAsync( hwnd, cmd );
}

NTSTATUS WINAPI wow64_NtUserSwitchDesktop( UINT *args )
{
    HDESK handle = get_handle( &args );

    return NtUserSwitchDesktop( handle );
}

NTSTATUS WINAPI wow64_NtUserSystemParametersInfo( UINT *args )
{
    UINT action = get_ulong( &args );
    UINT val = get_ulong( &args );
    void *ptr = get_ptr( &args );
    UINT winini = get_ulong( &args );

    switch (action)
    {
    case SPI_GETSERIALKEYS:
        if (ptr)
        {
            struct
            {
                UINT  cbSize;
                DWORD dwFlags;
                ULONG lpszActivePort;
                ULONG lpszPort;
                UINT  iBaudRate;
                UINT  iPortState;
                UINT  iActive;
            } *keys32 = ptr;
            SERIALKEYSW keys;

            if (keys32->cbSize != sizeof(*keys32)) return FALSE;
            keys.cbSize = sizeof(keys);
            if (!NtUserSystemParametersInfo( action, val, &keys, winini )) return FALSE;
            keys32->dwFlags = keys.dwFlags;
            keys32->lpszActivePort = PtrToUlong( keys.lpszActivePort );
            keys32->lpszPort = PtrToUlong( keys.lpszPort );
            keys32->iBaudRate = keys.iBaudRate;
            keys32->iPortState = keys.iPortState;
            keys32->iActive = keys.iActive;
            return TRUE;
        }
        break;

    case SPI_GETSOUNDSENTRY:
        if (ptr)
        {
            struct
            {
                UINT  cbSize;
                DWORD dwFlags;
                DWORD iFSTextEffect;
                DWORD iFSTextEffectMSec;
                DWORD iFSTextEffectColorBits;
                DWORD iFSGrafEffect;
                DWORD iFSGrafEffectMSec;
                DWORD iFSGrafEffectColor;
                DWORD iWindowsEffect;
                DWORD iWindowsEffectMSec;
                ULONG lpszWindowsEffectDLL;
                DWORD iWindowsEffectOrdinal;
            } *entry32 = ptr;
            SOUNDSENTRYW entry;

            if (entry32->cbSize != sizeof(*entry32)) return FALSE;
            entry.cbSize = sizeof(entry);
            if (!NtUserSystemParametersInfo( action, val, &entry, winini )) return FALSE;
            entry32->dwFlags = entry.dwFlags;
            entry32->iFSTextEffect = entry.iFSTextEffect;
            entry32->iFSTextEffectMSec = entry.iFSTextEffectMSec;
            entry32->iFSTextEffectColorBits = entry.iFSTextEffectColorBits;
            entry32->iFSGrafEffect = entry.iFSGrafEffect;
            entry32->iFSGrafEffectMSec = entry.iFSGrafEffectMSec;
            entry32->iFSGrafEffectColor = entry.iFSGrafEffectColor;
            entry32->iWindowsEffect = entry.iWindowsEffect;
            entry32->iWindowsEffectMSec = entry.iWindowsEffectMSec;
            entry32->lpszWindowsEffectDLL = PtrToUlong( entry.lpszWindowsEffectDLL );
            entry32->iWindowsEffectOrdinal = entry.iWindowsEffectOrdinal;
            return TRUE;
        }
        break;

    case SPI_GETHIGHCONTRAST:
        if (ptr)
        {
            struct
            {
                UINT  cbSize;
                DWORD dwFlags;
                ULONG lpszDefaultScheme;
            } *info32 = ptr;
            HIGHCONTRASTW info;

            if (info32->cbSize != sizeof(*info32)) return FALSE;
            info.cbSize = sizeof(info);
            if (!NtUserSystemParametersInfo( action, val, &info, winini )) return FALSE;
            info32->dwFlags = info.dwFlags;
            info32->lpszDefaultScheme = PtrToUlong( info.lpszDefaultScheme );
            return TRUE;
        }
        break;
    }

    return NtUserSystemParametersInfo( action, val, ptr, winini );
}

NTSTATUS WINAPI wow64_NtUserSystemParametersInfoForDpi( UINT *args )
{
    UINT action = get_ulong( &args );
    UINT val = get_ulong( &args );
    void *ptr = get_ptr( &args );
    UINT winini = get_ulong( &args );
    UINT dpi = get_ulong( &args );

    return NtUserSystemParametersInfoForDpi( action, val, ptr, winini, dpi );
}

NTSTATUS WINAPI wow64_NtUserThunkedMenuInfo( UINT *args )
{
    HMENU menu = get_handle( &args );
    MENUINFO32 *info32 = get_ptr( &args );
    MENUINFO info;

    if (info32)
    {
        info.cbSize = sizeof(info);
        info.fMask = info32->fMask;
        info.dwStyle = info32->dwStyle;
        info.cyMax = info32->cyMax;
        info.hbrBack = UlongToHandle( info32->hbrBack );
        info.dwContextHelpID = info32->dwContextHelpID;
        info.dwMenuData = info32->dwMenuData;
    }

    return NtUserThunkedMenuInfo( menu, info32 ? &info : NULL );
}

NTSTATUS WINAPI wow64_NtUserThunkedMenuItemInfo( UINT *args )
{
    HMENU handle = get_handle( &args );
    UINT pos = get_ulong( &args );
    UINT flags = get_ulong( &args );
    UINT method = get_ulong( &args );
    MENUITEMINFOW32 *info32 = get_ptr( &args );
    UNICODE_STRING32 *str32 = get_ptr( &args );
    MENUITEMINFOW info = { sizeof(info) }, *info_ptr;
    UNICODE_STRING str;
    UINT ret;

    if (info32)
    {
        info.cbSize = sizeof(info);
        info.fMask = info32->fMask;
        switch (method)
        {
        case NtUserSetMenuItemInfo:
        case NtUserInsertMenuItem:
            info.fType = info32->fType;
            info.fState = info32->fState;
            info.wID = info32->wID;
            info.hSubMenu = LongToHandle( info32->hSubMenu );
            info.hbmpChecked = UlongToHandle( info32->hbmpChecked );
            info.hbmpUnchecked = UlongToHandle( info32->hbmpUnchecked );
            info.dwItemData = info32->dwItemData;
            info.dwTypeData = UlongToPtr( info32->dwTypeData );
            info.cch = info32->cch;
            info.hbmpItem = UlongToHandle( info32->hbmpItem );
            break;
        case NtUserCheckMenuRadioItem:
            info.cch = info32->cch;
            break;
        case NtUserGetMenuItemInfoA:
        case NtUserGetMenuItemInfoW:
            info.dwTypeData = UlongToPtr( info32->dwTypeData );
            info.cch = info32->cch;
            break;
        }
        info_ptr = &info;
    }
    else info_ptr = NULL;

    ret = NtUserThunkedMenuItemInfo( handle, pos, flags, method, info_ptr,
                                     unicode_str_32to64( &str, str32 ));

    if (info_ptr)
    {
        switch (method)
        {
        case NtUserGetMenuItemInfoA:
        case NtUserGetMenuItemInfoW:
            if (info.fMask & (MIIM_TYPE | MIIM_STRING | MIIM_FTYPE))
                info32->fType = info.fType;
            if (info.fMask & (MIIM_TYPE | MIIM_BITMAP))
                info32->hbmpItem = HandleToUlong( info.hbmpItem );
            if (info.fMask & (MIIM_TYPE | MIIM_STRING))
            {
                info32->dwTypeData = (UINT_PTR)info.dwTypeData;
                info32->cch = info.cch;
            }
            if (info.fMask & MIIM_STATE) info32->fState = info.fState;
            if (info.fMask & MIIM_ID) info32->wID = info.wID;
            info32->hSubMenu = HandleToUlong( info.hSubMenu );
            if (info.fMask & MIIM_CHECKMARKS)
            {
                info32->hbmpChecked = HandleToUlong( info.hbmpChecked );
                info32->hbmpUnchecked = HandleToUlong( info.hbmpUnchecked );
            }
            if (info.fMask & MIIM_DATA) info32->dwItemData = info.dwItemData;
            break;
        }
    }

    return ret;
}

NTSTATUS WINAPI wow64_NtUserToUnicodeEx( UINT *args )
{
    UINT virt = get_ulong( &args );
    UINT scan = get_ulong( &args );
    const BYTE *state = get_ptr( &args );
    WCHAR *str = get_ptr( &args );
    int size = get_ulong( &args );
    UINT flags = get_ulong( &args );
    HKL layout = get_handle( &args );

    return NtUserToUnicodeEx( virt, scan, state, str, size, flags, layout );
}

NTSTATUS WINAPI wow64_NtUserTrackMouseEvent( UINT *args )
{
    struct
    {
        DWORD  cbSize;
        DWORD  dwFlags;
        UINT32 hwndTrack;
        DWORD  dwHoverTime;
    } *info32 = get_ptr( &args );
    TRACKMOUSEEVENT info;
    BOOL ret;

    if (info32->cbSize != sizeof(*info32))
    {
        set_last_error32( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    info.cbSize      = sizeof(info);
    info.dwFlags     = info32->dwFlags;
    info.hwndTrack   = LongToHandle( info32->hwndTrack );
    info.dwHoverTime = info32->dwHoverTime;
    ret = NtUserTrackMouseEvent( &info );
    info32->dwFlags     = info.dwFlags;
    info32->hwndTrack   = HandleToUlong( info.hwndTrack );
    info32->dwHoverTime = info.dwHoverTime;
    return ret;
}

NTSTATUS WINAPI wow64_NtUserTrackPopupMenuEx( UINT *args )
{
    HMENU handle = get_handle( &args );
    UINT flags = get_ulong( &args );
    int x = get_ulong( &args );
    int y = get_ulong( &args );
    HWND hwnd = get_handle( &args );
    TPMPARAMS *params = get_ptr( &args );

    return NtUserTrackPopupMenuEx( handle, flags, x, y, hwnd, params );
}

NTSTATUS WINAPI wow64_NtUserTranslateAccelerator( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HACCEL accel = get_handle( &args );
    MSG32 *msg32 = get_ptr( &args );

    MSG msg;

    return NtUserTranslateAccelerator( hwnd, accel, msg_32to64( &msg, msg32 ));
}

NTSTATUS WINAPI wow64_NtUserTranslateMessage( UINT *args )
{
    const MSG32 *msg32 = get_ptr( &args );
    UINT flags = get_ulong( &args );
    MSG msg;

    return NtUserTranslateMessage( msg_32to64( &msg, msg32 ), flags );
}

NTSTATUS WINAPI wow64_NtUserUnhookWinEvent( UINT *args )
{
    HWINEVENTHOOK handle = get_handle( &args );

    return NtUserUnhookWinEvent( handle );
}

NTSTATUS WINAPI wow64_NtUserUnhookWindowsHook( UINT *args )
{
    INT id = get_ulong( &args );
    HOOKPROC proc = get_ptr( &args );

    return NtUserUnhookWindowsHook( id, proc );
}

NTSTATUS WINAPI wow64_NtUserUnhookWindowsHookEx( UINT *args )
{
    HHOOK handle = get_handle( &args );

    return NtUserUnhookWindowsHookEx( handle );
}

NTSTATUS WINAPI wow64_NtUserUnregisterClass( UINT *args )
{
    UNICODE_STRING32 *name32 = get_ptr( &args );
    HINSTANCE instance = get_ptr( &args );
    struct client_menu_name32 *menu_name32 = get_ptr( &args );

    UNICODE_STRING name;
    struct client_menu_name menu_name;
    BOOL ret;

    ret = NtUserUnregisterClass( unicode_str_32to64( &name, name32 ), instance, &menu_name );
    if (ret) client_menu_name_64to32( &menu_name, menu_name32 );
    return ret;
}

NTSTATUS WINAPI wow64_NtUserUnregisterHotKey( UINT *args )
{
    HWND hwnd = get_handle( &args );
    int id = get_ulong( &args );

    return NtUserUnregisterHotKey( hwnd, id );
}

NTSTATUS WINAPI wow64_NtUserUpdateInputContext( UINT *args )
{
    HIMC handle = get_handle( &args );
    UINT attr = get_ulong( &args );
    UINT_PTR value = get_ulong( &args );

    return NtUserUpdateInputContext( handle, attr, value );
}

NTSTATUS WINAPI wow64_NtUserUpdateLayeredWindow( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HDC hdc_dst = get_handle( &args );
    const POINT *pts_dst = get_ptr( &args );
    const SIZE *size = get_ptr( &args );
    HDC hdc_src = get_handle( &args );
    const POINT *pts_src = get_ptr( &args );
    COLORREF key = get_ulong( &args );
    const BLENDFUNCTION *blend = get_ptr( &args );
    DWORD flags = get_ulong( &args );
    const RECT *dirty = get_ptr( &args );

    return NtUserUpdateLayeredWindow( hwnd, hdc_dst, pts_dst, size, hdc_src, pts_src,
                                      key, blend, flags, dirty );
}

NTSTATUS WINAPI wow64_NtUserValidateRect( UINT *args )
{
    HWND hwnd = get_handle( &args );
    const RECT *rect = get_ptr( &args );

    return NtUserValidateRect( hwnd, rect );
}

NTSTATUS WINAPI wow64_NtUserValidateRgn( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HRGN hrgn = get_handle( &args );

    return NtUserValidateRgn( hwnd, hrgn );
}

NTSTATUS WINAPI wow64_NtUserVkKeyScanEx( UINT *args )
{
    WCHAR chr = get_ulong( &args );
    HKL layout = get_handle( &args );

    return NtUserVkKeyScanEx( chr, layout );
}

NTSTATUS WINAPI wow64_NtUserWaitForInputIdle( UINT *args )
{
    HANDLE process = get_handle( &args );
    DWORD timeout = get_ulong( &args );
    BOOL wow = get_ulong( &args );

    return NtUserWaitForInputIdle( process, timeout, wow );
}

NTSTATUS WINAPI wow64_NtUserWaitMessage( UINT *args )
{
    return NtUserWaitMessage();
}

NTSTATUS WINAPI wow64_NtUserWindowFromDC( UINT *args )
{
    HDC hdc = get_handle( &args );

    return HandleToUlong( NtUserWindowFromDC( hdc ));
}

NTSTATUS WINAPI wow64_NtUserWindowFromPoint( UINT *args )
{
    LONG x = get_ulong( &args );
    LONG y = get_ulong( &args );

    return HandleToUlong( NtUserWindowFromPoint( x, y ));
}

NTSTATUS WINAPI wow64_NtUserDisplayConfigGetDeviceInfo( UINT *args )
{
    DISPLAYCONFIG_DEVICE_INFO_HEADER *packet = get_ptr( &args );

    return NtUserDisplayConfigGetDeviceInfo( packet );
}
