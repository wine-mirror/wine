/*
 * Window messaging support
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "winnls.h"
#include "dde.h"
#include "wine/unicode.h"
#include "wine/server.h"
#include "message.h"
#include "user.h"
#include "win.h"
#include "winproc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msg);

#define WM_NCMOUSEFIRST WM_NCMOUSEMOVE
#define WM_NCMOUSELAST  WM_NCMBUTTONDBLCLK

#define MAX_PACK_COUNT 4

/* description of the data fields that need to be packed along with a sent message */
struct packed_message
{
    int         count;
    const void *data[MAX_PACK_COUNT];
    size_t      size[MAX_PACK_COUNT];
};

/* info about the message currently being received by the current thread */
struct received_message_info
{
    enum message_type type;
    MSG               msg;
    UINT              flags;  /* InSendMessageEx return flags */
};

/* structure to group all parameters for sent messages of the various kinds */
struct send_message_info
{
    enum message_type type;
    HWND              hwnd;
    UINT              msg;
    WPARAM            wparam;
    LPARAM            lparam;
    UINT              flags;      /* flags for SendMessageTimeout */
    UINT              timeout;    /* timeout for SendMessageTimeout */
    SENDASYNCPROC     callback;   /* callback function for SendMessageCallback */
    ULONG_PTR         data;       /* callback data */
};


/* flag for messages that contain pointers */
/* 32 messages per entry, messages 0..31 map to bits 0..31 */

#define SET(msg) (1 << ((msg) & 31))

static const unsigned int message_pointer_flags[] =
{
    /* 0x00 - 0x1f */
    SET(WM_CREATE) | SET(WM_SETTEXT) | SET(WM_GETTEXT) |
    SET(WM_WININICHANGE) | SET(WM_DEVMODECHANGE),
    /* 0x20 - 0x3f */
    SET(WM_GETMINMAXINFO) | SET(WM_DRAWITEM) | SET(WM_MEASUREITEM) | SET(WM_DELETEITEM) |
    SET(WM_COMPAREITEM),
    /* 0x40 - 0x5f */
    SET(WM_WINDOWPOSCHANGING) | SET(WM_WINDOWPOSCHANGED) | SET(WM_COPYDATA) |
    SET(WM_NOTIFY) | SET(WM_HELP),
    /* 0x60 - 0x7f */
    SET(WM_STYLECHANGING) | SET(WM_STYLECHANGED),
    /* 0x80 - 0x9f */
    SET(WM_NCCREATE) | SET(WM_NCCALCSIZE) | SET(WM_GETDLGCODE),
    /* 0xa0 - 0xbf */
    SET(EM_GETSEL) | SET(EM_GETRECT) | SET(EM_SETRECT) | SET(EM_SETRECTNP),
    /* 0xc0 - 0xdf */
    SET(EM_REPLACESEL) | SET(EM_GETLINE) | SET(EM_SETTABSTOPS),
    /* 0xe0 - 0xff */
    SET(SBM_GETRANGE) | SET(SBM_SETSCROLLINFO) | SET(SBM_GETSCROLLINFO),
    /* 0x100 - 0x11f */
    0,
    /* 0x120 - 0x13f */
    0,
    /* 0x140 - 0x15f */
    SET(CB_GETEDITSEL) | SET(CB_ADDSTRING) | SET(CB_DIR) | SET(CB_GETLBTEXT) |
    SET(CB_INSERTSTRING) | SET(CB_FINDSTRING) | SET(CB_SELECTSTRING) |
    SET(CB_GETDROPPEDCONTROLRECT) | SET(CB_FINDSTRINGEXACT),
    /* 0x160 - 0x17f */
    0,
    /* 0x180 - 0x19f */
    SET(LB_ADDSTRING) | SET(LB_INSERTSTRING) | SET(LB_GETTEXT) | SET(LB_SELECTSTRING) |
    SET(LB_DIR) | SET(LB_FINDSTRING) |
    SET(LB_GETSELITEMS) | SET(LB_SETTABSTOPS) | SET(LB_ADDFILE) | SET(LB_GETITEMRECT),
    /* 0x1a0 - 0x1bf */
    SET(LB_FINDSTRINGEXACT),
    /* 0x1c0 - 0x1df */
    0,
    /* 0x1e0 - 0x1ff */
    0,
    /* 0x200 - 0x21f */
    SET(WM_NEXTMENU) | SET(WM_SIZING) | SET(WM_MOVING) | SET(WM_DEVICECHANGE),
    /* 0x220 - 0x23f */
    SET(WM_MDICREATE) | SET(WM_MDIGETACTIVE) | SET(WM_DROPOBJECT) |
    SET(WM_QUERYDROPOBJECT) | SET(WM_DRAGLOOP) | SET(WM_DRAGSELECT) | SET(WM_DRAGMOVE),
    /* 0x240 - 0x25f */
    0,
    /* 0x260 - 0x27f */
    0,
    /* 0x280 - 0x29f */
    0,
    /* 0x2a0 - 0x2bf */
    0,
    /* 0x2c0 - 0x2df */
    0,
    /* 0x2e0 - 0x2ff */
    0,
    /* 0x300 - 0x31f */
    SET(WM_ASKCBFORMATNAME)
};

/* flags for messages that contain Unicode strings */
static const unsigned int message_unicode_flags[] =
{
    /* 0x00 - 0x1f */
    SET(WM_CREATE) | SET(WM_SETTEXT) | SET(WM_GETTEXT) | SET(WM_GETTEXTLENGTH) |
    SET(WM_WININICHANGE) | SET(WM_DEVMODECHANGE),
    /* 0x20 - 0x3f */
    SET(WM_CHARTOITEM),
    /* 0x40 - 0x5f */
    0,
    /* 0x60 - 0x7f */
    0,
    /* 0x80 - 0x9f */
    SET(WM_NCCREATE),
    /* 0xa0 - 0xbf */
    0,
    /* 0xc0 - 0xdf */
    SET(EM_REPLACESEL) | SET(EM_GETLINE) | SET(EM_SETPASSWORDCHAR),
    /* 0xe0 - 0xff */
    0,
    /* 0x100 - 0x11f */
    SET(WM_CHAR) | SET(WM_DEADCHAR) | SET(WM_SYSCHAR) | SET(WM_SYSDEADCHAR),
    /* 0x120 - 0x13f */
    SET(WM_MENUCHAR),
    /* 0x140 - 0x15f */
    SET(CB_ADDSTRING) | SET(CB_DIR) | SET(CB_GETLBTEXT) | SET(CB_GETLBTEXTLEN) |
    SET(CB_INSERTSTRING) | SET(CB_FINDSTRING) | SET(CB_SELECTSTRING) | SET(CB_FINDSTRINGEXACT),
    /* 0x160 - 0x17f */
    0,
    /* 0x180 - 0x19f */
    SET(LB_ADDSTRING) | SET(LB_INSERTSTRING) | SET(LB_GETTEXT) | SET(LB_GETTEXTLEN) |
    SET(LB_SELECTSTRING) | SET(LB_DIR) | SET(LB_FINDSTRING) | SET(LB_ADDFILE),
    /* 0x1a0 - 0x1bf */
    SET(LB_FINDSTRINGEXACT),
    /* 0x1c0 - 0x1df */
    0,
    /* 0x1e0 - 0x1ff */
    0,
    /* 0x200 - 0x21f */
    0,
    /* 0x220 - 0x23f */
    SET(WM_MDICREATE),
    /* 0x240 - 0x25f */
    0,
    /* 0x260 - 0x27f */
    0,
    /* 0x280 - 0x29f */
    SET(WM_IME_CHAR),
    /* 0x2a0 - 0x2bf */
    0,
    /* 0x2c0 - 0x2df */
    0,
    /* 0x2e0 - 0x2ff */
    0,
    /* 0x300 - 0x31f */
    SET(WM_PAINTCLIPBOARD) | SET(WM_SIZECLIPBOARD) | SET(WM_ASKCBFORMATNAME)
};

/* check whether a given message type includes pointers */
inline static int is_pointer_message( UINT message )
{
    if (message >= 8*sizeof(message_pointer_flags)) return FALSE;
    return (message_pointer_flags[message / 32] & SET(message)) != 0;
}

/* check whether a given message type contains Unicode (or ASCII) chars */
inline static int is_unicode_message( UINT message )
{
    if (message >= 8*sizeof(message_unicode_flags)) return FALSE;
    return (message_unicode_flags[message / 32] & SET(message)) != 0;
}

#undef SET

/* add a data field to a packed message */
inline static void push_data( struct packed_message *data, const void *ptr, size_t size )
{
    data->data[data->count] = ptr;
    data->size[data->count] = size;
    data->count++;
}

/* add a string to a packed message */
inline static void push_string( struct packed_message *data, LPCWSTR str )
{
    push_data( data, str, (strlenW(str) + 1) * sizeof(WCHAR) );
}

/* retrieve a pointer to data from a packed message and increment the buffer pointer */
inline static void *get_data( void **buffer, size_t size )
{
    void *ret = *buffer;
    *buffer = (char *)*buffer + size;
    return ret;
}

/* make sure that the buffer contains a valid null-terminated Unicode string */
inline static BOOL check_string( LPCWSTR str, size_t size )
{
    for (size /= sizeof(WCHAR); size; size--, str++)
        if (!*str) return TRUE;
    return FALSE;
}

/* make sure that there is space for 'size' bytes in buffer, growing it if needed */
inline static void *get_buffer_space( void **buffer, size_t size )
{
    void *ret;
    if (!(ret = HeapReAlloc( GetProcessHeap(), 0, *buffer, size )))
        HeapFree( GetProcessHeap(), 0, *buffer );
    *buffer = ret;
    return ret;
}

/* retrieve a string pointer from packed data */
inline static LPWSTR get_string( void **buffer )
{
    return get_data( buffer, (strlenW( (LPWSTR)*buffer ) + 1) * sizeof(WCHAR) );
}

/* check whether a combobox expects strings or ids in CB_ADDSTRING/CB_INSERTSTRING */
inline static BOOL combobox_has_strings( HWND hwnd )
{
    DWORD style = GetWindowLongA( hwnd, GWL_STYLE );
    return (!(style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) || (style & CBS_HASSTRINGS));
}

/* check whether a listbox expects strings or ids in LB_ADDSTRING/LB_INSERTSTRING */
inline static BOOL listbox_has_strings( HWND hwnd )
{
    DWORD style = GetWindowLongA( hwnd, GWL_STYLE );
    return (!(style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) || (style & LBS_HASSTRINGS));
}

/* check if hwnd is a broadcast magic handle */
inline static BOOL is_broadcast( HWND hwnd )
{
    return (hwnd == HWND_BROADCAST || hwnd == HWND_TOPMOST);
}


/***********************************************************************
 *		broadcast_message_callback
 *
 * Helper callback for broadcasting messages.
 */
static BOOL CALLBACK broadcast_message_callback( HWND hwnd, LPARAM lparam )
{
    struct send_message_info *info = (struct send_message_info *)lparam;
    if (!(GetWindowLongW( hwnd, GWL_STYLE ) & (WS_POPUP|WS_CAPTION))) return TRUE;
    switch(info->type)
    {
    case MSG_UNICODE:
        SendMessageTimeoutW( hwnd, info->msg, info->wparam, info->lparam,
                             info->flags, info->timeout, NULL );
        break;
    case MSG_ASCII:
        SendMessageTimeoutA( hwnd, info->msg, info->wparam, info->lparam,
                             info->flags, info->timeout, NULL );
        break;
    case MSG_NOTIFY:
        SendNotifyMessageW( hwnd, info->msg, info->wparam, info->lparam );
        break;
    case MSG_CALLBACK:
        SendMessageCallbackW( hwnd, info->msg, info->wparam, info->lparam,
                              info->callback, info->data );
        break;
    case MSG_POSTED:
        PostMessageW( hwnd, info->msg, info->wparam, info->lparam );
        break;
    default:
        ERR( "bad type %d\n", info->type );
        break;
    }
    return TRUE;
}


/***********************************************************************
 *		map_wparam_AtoW
 *
 * Convert the wparam of an ASCII message to Unicode.
 */
static WPARAM map_wparam_AtoW( UINT message, WPARAM wparam )
{
    switch(message)
    {
    case WM_CHARTOITEM:
    case EM_SETPASSWORDCHAR:
    case WM_CHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case WM_MENUCHAR:
        {
            char ch = LOWORD(wparam);
            WCHAR wch;
            MultiByteToWideChar(CP_ACP, 0, &ch, 1, &wch, 1);
            wparam = MAKEWPARAM( wch, HIWORD(wparam) );
        }
        break;
    case WM_IME_CHAR:
        {
            char ch[2];
            WCHAR wch;
            ch[0] = (wparam >> 8);
            ch[1] = (wparam & 0xff);
            MultiByteToWideChar(CP_ACP, 0, ch, 2, &wch, 1);
            wparam = MAKEWPARAM( wch, HIWORD(wparam) );
        }
        break;
    }
    return wparam;
}


/***********************************************************************
 *		map_wparam_WtoA
 *
 * Convert the wparam of a Unicode message to ASCII.
 */
static WPARAM map_wparam_WtoA( UINT message, WPARAM wparam )
{
    switch(message)
    {
    case WM_CHARTOITEM:
    case EM_SETPASSWORDCHAR:
    case WM_CHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case WM_MENUCHAR:
        {
            WCHAR wch = LOWORD(wparam);
            BYTE ch;
            WideCharToMultiByte( CP_ACP, 0, &wch, 1, &ch, 1, NULL, NULL );
            wparam = MAKEWPARAM( ch, HIWORD(wparam) );
        }
        break;
    case WM_IME_CHAR:
        {
            WCHAR wch = LOWORD(wparam);
            BYTE ch[2];

            ch[1] = 0;
            WideCharToMultiByte( CP_ACP, 0, &wch, 1, ch, 2, NULL, NULL );
            wparam = MAKEWPARAM( (ch[0] << 8) | ch[1], HIWORD(wparam) );
        }
        break;
    }
    return wparam;
}


/***********************************************************************
 *		pack_message
 *
 * Pack a message for sending to another process.
 * Return the size of the data we expect in the message reply.
 * Set data->count to -1 if there is an error.
 */
static size_t pack_message( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
                            struct packed_message *data )
{
    data->count = 0;
    switch(message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
    {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)lparam;
        push_data( data, cs, sizeof(*cs) );
        if (HIWORD(cs->lpszName)) push_string( data, cs->lpszName );
        if (HIWORD(cs->lpszClass)) push_string( data, cs->lpszClass );
        return sizeof(*cs);
    }
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        return wparam * sizeof(WCHAR);
    case WM_WININICHANGE:
        if (lparam) push_string(data, (LPWSTR)lparam );
        return 0;
    case WM_SETTEXT:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
        push_string( data, (LPWSTR)lparam );
        return 0;
    case WM_GETMINMAXINFO:
        push_data( data, (MINMAXINFO *)lparam, sizeof(MINMAXINFO) );
        return sizeof(MINMAXINFO);
    case WM_DRAWITEM:
        push_data( data, (DRAWITEMSTRUCT *)lparam, sizeof(DRAWITEMSTRUCT) );
        return 0;
    case WM_MEASUREITEM:
        push_data( data, (MEASUREITEMSTRUCT *)lparam, sizeof(MEASUREITEMSTRUCT) );
        return sizeof(MEASUREITEMSTRUCT);
    case WM_DELETEITEM:
        push_data( data, (DELETEITEMSTRUCT *)lparam, sizeof(DELETEITEMSTRUCT) );
        return 0;
    case WM_COMPAREITEM:
        push_data( data, (COMPAREITEMSTRUCT *)lparam, sizeof(COMPAREITEMSTRUCT) );
        return 0;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        push_data( data, (WINDOWPOS *)lparam, sizeof(WINDOWPOS) );
        return sizeof(WINDOWPOS);
    case WM_COPYDATA:
    {
        COPYDATASTRUCT *cp = (COPYDATASTRUCT *)lparam;
        push_data( data, cp, sizeof(*cp) );
        if (cp->lpData) push_data( data, cp->lpData, cp->cbData );
        return 0;
    }
    case WM_NOTIFY:
        /* WM_NOTIFY cannot be sent across processes (MSDN) */
        data->count = -1;
        return 0;
    case WM_HELP:
        push_data( data, (HELPINFO *)lparam, sizeof(HELPINFO) );
        return 0;
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
        push_data( data, (STYLESTRUCT *)lparam, sizeof(STYLESTRUCT) );
        return 0;
    case WM_NCCALCSIZE:
        if (!wparam)
        {
            push_data( data, (RECT *)lparam, sizeof(RECT) );
            return sizeof(RECT);
        }
        else
        {
            NCCALCSIZE_PARAMS *nc = (NCCALCSIZE_PARAMS *)lparam;
            push_data( data, nc, sizeof(*nc) );
            push_data( data, nc->lppos, sizeof(*nc->lppos) );
            return sizeof(*nc) + sizeof(*nc->lppos);
        }
    case WM_GETDLGCODE:
        if (lparam) push_data( data, (MSG *)lparam, sizeof(MSG) );
        return sizeof(MSG);
    case SBM_SETSCROLLINFO:
        push_data( data, (SCROLLINFO *)lparam, sizeof(SCROLLINFO) );
        return 0;
    case SBM_GETSCROLLINFO:
        push_data( data, (SCROLLINFO *)lparam, sizeof(SCROLLINFO) );
        return sizeof(SCROLLINFO);
    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
    {
        size_t size = 0;
        if (wparam) size += sizeof(DWORD);
        if (lparam) size += sizeof(DWORD);
        return size;
    }
    case EM_GETRECT:
    case LB_GETITEMRECT:
    case CB_GETDROPPEDCONTROLRECT:
        return sizeof(RECT);
    case EM_SETRECT:
    case EM_SETRECTNP:
        push_data( data, (RECT *)lparam, sizeof(RECT) );
        return 0;
    case EM_GETLINE:
    {
        WORD *pw = (WORD *)lparam;
        push_data( data, pw, sizeof(*pw) );
        return *pw * sizeof(WCHAR);
    }
    case EM_SETTABSTOPS:
    case LB_SETTABSTOPS:
        if (wparam) push_data( data, (UINT *)lparam, sizeof(UINT) * wparam );
        return 0;
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
        if (combobox_has_strings( hwnd )) push_string( data, (LPWSTR)lparam );
        return 0;
    case CB_GETLBTEXT:
        if (!combobox_has_strings( hwnd )) return sizeof(ULONG_PTR);
        return (SendMessageW( hwnd, CB_GETLBTEXTLEN, wparam, 0 ) + 1) * sizeof(WCHAR);
    case LB_ADDSTRING:
    case LB_INSERTSTRING:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_SELECTSTRING:
        if (listbox_has_strings( hwnd )) push_string( data, (LPWSTR)lparam );
        return 0;
    case LB_GETTEXT:
        if (!listbox_has_strings( hwnd )) return sizeof(ULONG_PTR);
        return (SendMessageW( hwnd, LB_GETTEXTLEN, wparam, 0 ) + 1) * sizeof(WCHAR);
    case LB_GETSELITEMS:
        return wparam * sizeof(UINT);
    case WM_NEXTMENU:
        push_data( data, (MDINEXTMENU *)lparam, sizeof(MDINEXTMENU) );
        return sizeof(MDINEXTMENU);
    case WM_SIZING:
    case WM_MOVING:
        push_data( data, (RECT *)lparam, sizeof(RECT) );
        return sizeof(RECT);
    case WM_MDICREATE:
    {
        MDICREATESTRUCTW *cs = (MDICREATESTRUCTW *)lparam;
        push_data( data, cs, sizeof(*cs) );
        if (HIWORD(cs->szTitle)) push_string( data, cs->szTitle );
        if (HIWORD(cs->szClass)) push_string( data, cs->szClass );
        return sizeof(*cs);
    }
    case WM_MDIGETACTIVE:
        if (lparam) return sizeof(BOOL);
        return 0;
    case WM_WINE_SETWINDOWPOS:
        push_data( data, (WINDOWPOS *)lparam, sizeof(WINDOWPOS) );
        return 0;

    /* these contain an HFONT */
    case WM_SETFONT:
    case WM_GETFONT:
    /* these contain an HDC */
    case WM_PAINT:
    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
    case WM_NCPAINT:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
    case WM_PRINT:
    case WM_PRINTCLIENT:
    /* these contain an HGLOBAL */
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    /* these contain pointers */
    case WM_DROPOBJECT:
    case WM_QUERYDROPOBJECT:
    case WM_DRAGLOOP:
    case WM_DRAGSELECT:
    case WM_DRAGMOVE:
    case WM_DEVICECHANGE:
        FIXME( "msg %x (%s) not supported yet\n", message, SPY_GetMsgName(message, hwnd) );
        data->count = -1;
        return 0;
    }
    return 0;
}


/***********************************************************************
 *		unpack_message
 *
 * Unpack a message received from another process.
 */
static BOOL unpack_message( HWND hwnd, UINT message, WPARAM *wparam, LPARAM *lparam,
                            void **buffer, size_t size )
{
    size_t minsize = 0;

    switch(message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
    {
        CREATESTRUCTW *cs = *buffer;
        WCHAR *str = (WCHAR *)(cs + 1);
        if (size < sizeof(*cs)) return FALSE;
        size -= sizeof(*cs);
        if (HIWORD(cs->lpszName))
        {
            if (!check_string( str, size )) return FALSE;
            cs->lpszName = str;
            size -= (strlenW(str) + 1) * sizeof(WCHAR);
            str += strlenW(str) + 1;
        }
        if (HIWORD(cs->lpszClass))
        {
            if (!check_string( str, size )) return FALSE;
            cs->lpszClass = str;
        }
        break;
    }
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        if (!get_buffer_space( buffer, (*wparam * sizeof(WCHAR)) )) return FALSE;
        break;
    case WM_WININICHANGE:
        if (!*lparam) return TRUE;
        /* fall through */
    case WM_SETTEXT:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
        if (!check_string( *buffer, size )) return FALSE;
        break;
    case WM_GETMINMAXINFO:
        minsize = sizeof(MINMAXINFO);
        break;
    case WM_DRAWITEM:
        minsize = sizeof(DRAWITEMSTRUCT);
        break;
    case WM_MEASUREITEM:
        minsize = sizeof(MEASUREITEMSTRUCT);
        break;
    case WM_DELETEITEM:
        minsize = sizeof(DELETEITEMSTRUCT);
        break;
    case WM_COMPAREITEM:
        minsize = sizeof(COMPAREITEMSTRUCT);
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    case WM_WINE_SETWINDOWPOS:
        minsize = sizeof(WINDOWPOS);
        break;
    case WM_COPYDATA:
    {
        COPYDATASTRUCT *cp = *buffer;
        if (size < sizeof(*cp)) return FALSE;
        if (cp->lpData)
        {
            minsize = sizeof(*cp) + cp->cbData;
            cp->lpData = cp + 1;
        }
        break;
    }
    case WM_NOTIFY:
        /* WM_NOTIFY cannot be sent across processes (MSDN) */
        return FALSE;
    case WM_HELP:
        minsize = sizeof(HELPINFO);
        break;
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
        minsize = sizeof(STYLESTRUCT);
        break;
    case WM_NCCALCSIZE:
        if (!*wparam) minsize = sizeof(RECT);
        else
        {
            NCCALCSIZE_PARAMS *nc = *buffer;
            if (size < sizeof(*nc) + sizeof(*nc->lppos)) return FALSE;
            nc->lppos = (WINDOWPOS *)(nc + 1);
        }
        break;
    case WM_GETDLGCODE:
        if (!*lparam) return TRUE;
        minsize = sizeof(MSG);
        break;
    case SBM_SETSCROLLINFO:
        minsize = sizeof(SCROLLINFO);
        break;
    case SBM_GETSCROLLINFO:
        if (!get_buffer_space( buffer, sizeof(SCROLLINFO ))) return FALSE;
        break;
    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
        if (*wparam || *lparam)
        {
            if (!get_buffer_space( buffer, 2*sizeof(DWORD) )) return FALSE;
            if (*wparam) *wparam = (WPARAM)*buffer;
            if (*lparam) *lparam = (LPARAM)((DWORD *)*buffer + 1);
        }
        return TRUE;
    case EM_GETRECT:
    case LB_GETITEMRECT:
    case CB_GETDROPPEDCONTROLRECT:
        if (!get_buffer_space( buffer, sizeof(RECT) )) return FALSE;
        break;
    case EM_SETRECT:
    case EM_SETRECTNP:
        minsize = sizeof(RECT);
        break;
    case EM_GETLINE:
    {
        WORD len;
        if (size < sizeof(WORD)) return FALSE;
        len = *(WORD *)*buffer;
        if (!get_buffer_space( buffer, (len + 1) * sizeof(WCHAR) )) return FALSE;
        *lparam = (LPARAM)*buffer + sizeof(WORD);  /* don't erase WORD at start of buffer */
        return TRUE;
    }
    case EM_SETTABSTOPS:
    case LB_SETTABSTOPS:
        if (!*wparam) return TRUE;
        minsize = *wparam * sizeof(UINT);
        break;
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
    case LB_ADDSTRING:
    case LB_INSERTSTRING:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_SELECTSTRING:
        if (!*buffer) return TRUE;
        if (!check_string( *buffer, size )) return FALSE;
        break;
    case CB_GETLBTEXT:
    {
        size = sizeof(ULONG_PTR);
        if (combobox_has_strings( hwnd ))
            size = (SendMessageW( hwnd, CB_GETLBTEXTLEN, *wparam, 0 ) + 1) * sizeof(WCHAR);
        if (!get_buffer_space( buffer, size )) return FALSE;
        break;
    }
    case LB_GETTEXT:
    {
        size = sizeof(ULONG_PTR);
        if (listbox_has_strings( hwnd ))
            size = (SendMessageW( hwnd, LB_GETTEXTLEN, *wparam, 0 ) + 1) * sizeof(WCHAR);
        if (!get_buffer_space( buffer, size )) return FALSE;
        break;
    }
    case LB_GETSELITEMS:
        if (!get_buffer_space( buffer, *wparam * sizeof(UINT) )) return FALSE;
        break;
    case WM_NEXTMENU:
        minsize = sizeof(MDINEXTMENU);
        if (!get_buffer_space( buffer, sizeof(MDINEXTMENU) )) return FALSE;
        break;
    case WM_SIZING:
    case WM_MOVING:
        minsize = sizeof(RECT);
        if (!get_buffer_space( buffer, sizeof(RECT) )) return FALSE;
        break;
    case WM_MDICREATE:
    {
        MDICREATESTRUCTW *cs = *buffer;
        WCHAR *str = (WCHAR *)(cs + 1);
        if (size < sizeof(*cs)) return FALSE;
        size -= sizeof(*cs);
        if (HIWORD(cs->szTitle))
        {
            if (!check_string( str, size )) return FALSE;
            cs->szTitle = str;
            size -= (strlenW(str) + 1) * sizeof(WCHAR);
            str += strlenW(str) + 1;
        }
        if (HIWORD(cs->szClass))
        {
            if (!check_string( str, size )) return FALSE;
            cs->szClass = str;
        }
        break;
    }
    case WM_MDIGETACTIVE:
        if (!*lparam) return TRUE;
        if (!get_buffer_space( buffer, sizeof(BOOL) )) return FALSE;
        break;
    /* these contain an HFONT */
    case WM_SETFONT:
    case WM_GETFONT:
    /* these contain an HDC */
    case WM_PAINT:
    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
    case WM_NCPAINT:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
    case WM_PRINT:
    case WM_PRINTCLIENT:
    /* these contain an HGLOBAL */
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    /* these contain pointers */
    case WM_DROPOBJECT:
    case WM_QUERYDROPOBJECT:
    case WM_DRAGLOOP:
    case WM_DRAGSELECT:
    case WM_DRAGMOVE:
    case WM_DEVICECHANGE:
        FIXME( "msg %x (%s) not supported yet\n", message, SPY_GetMsgName(message, hwnd) );
        return FALSE;

    default:
        return TRUE; /* message doesn't need any unpacking */
    }

    /* default exit for most messages: check minsize and store buffer in lparam */
    if (size < minsize) return FALSE;
    *lparam = (LPARAM)*buffer;
    return TRUE;
}


/***********************************************************************
 *		pack_reply
 *
 * Pack a reply to a message for sending to another process.
 */
static void pack_reply( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
                        LRESULT res, struct packed_message *data )
{
    data->count = 0;
    switch(message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        push_data( data, (CREATESTRUCTW *)lparam, sizeof(CREATESTRUCTW) );
        break;
    case WM_GETTEXT:
    case CB_GETLBTEXT:
    case LB_GETTEXT:
        push_data( data, (WCHAR *)lparam, (res + 1) * sizeof(WCHAR) );
        break;
    case WM_GETMINMAXINFO:
        push_data( data, (MINMAXINFO *)lparam, sizeof(MINMAXINFO) );
        break;
    case WM_MEASUREITEM:
        push_data( data, (MEASUREITEMSTRUCT *)lparam, sizeof(MEASUREITEMSTRUCT) );
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        push_data( data, (WINDOWPOS *)lparam, sizeof(WINDOWPOS) );
        break;
    case WM_GETDLGCODE:
        if (lparam) push_data( data, (MSG *)lparam, sizeof(MSG) );
        break;
    case SBM_GETSCROLLINFO:
        push_data( data, (SCROLLINFO *)lparam, sizeof(SCROLLINFO) );
        break;
    case EM_GETRECT:
    case LB_GETITEMRECT:
    case CB_GETDROPPEDCONTROLRECT:
    case WM_SIZING:
    case WM_MOVING:
        push_data( data, (RECT *)lparam, sizeof(RECT) );
        break;
    case EM_GETLINE:
    {
        WORD *ptr = (WORD *)lparam;
        push_data( data, ptr, ptr[-1] * sizeof(WCHAR) );
        break;
    }
    case LB_GETSELITEMS:
        push_data( data, (UINT *)lparam, wparam * sizeof(UINT) );
        break;
    case WM_MDIGETACTIVE:
        if (lparam) push_data( data, (BOOL *)lparam, sizeof(BOOL) );
        break;
    case WM_NCCALCSIZE:
        if (!wparam)
            push_data( data, (RECT *)lparam, sizeof(RECT) );
        else
        {
            NCCALCSIZE_PARAMS *nc = (NCCALCSIZE_PARAMS *)lparam;
            push_data( data, nc, sizeof(*nc) );
            push_data( data, nc->lppos, sizeof(*nc->lppos) );
        }
        break;
    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
        if (wparam) push_data( data, (DWORD *)wparam, sizeof(DWORD) );
        if (lparam) push_data( data, (DWORD *)lparam, sizeof(DWORD) );
        break;
    case WM_NEXTMENU:
        push_data( data, (MDINEXTMENU *)lparam, sizeof(MDINEXTMENU) );
        break;
    case WM_MDICREATE:
        push_data( data, (MDICREATESTRUCTW *)lparam, sizeof(MDICREATESTRUCTW) );
        break;
    case WM_ASKCBFORMATNAME:
        push_data( data, (WCHAR *)lparam, (strlenW((WCHAR *)lparam) + 1) * sizeof(WCHAR) );
        break;
    }
}


/***********************************************************************
 *		unpack_reply
 *
 * Unpack a message reply received from another process.
 */
static void unpack_reply( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
                          void *buffer, size_t size )
{
    switch(message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
    {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)lparam;
        LPCWSTR name = cs->lpszName, class = cs->lpszClass;
        memcpy( cs, buffer, min( sizeof(*cs), size ));
        cs->lpszName = name;  /* restore the original pointers */
        cs->lpszClass = class;
        break;
    }
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        memcpy( (WCHAR *)lparam, buffer, min( wparam*sizeof(WCHAR), size ));
        break;
    case WM_GETMINMAXINFO:
        memcpy( (MINMAXINFO *)lparam, buffer, min( sizeof(MINMAXINFO), size ));
        break;
    case WM_MEASUREITEM:
        memcpy( (MEASUREITEMSTRUCT *)lparam, buffer, min( sizeof(MEASUREITEMSTRUCT), size ));
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        memcpy( (WINDOWPOS *)lparam, buffer, min( sizeof(WINDOWPOS), size ));
        break;
    case WM_GETDLGCODE:
        if (lparam) memcpy( (MSG *)lparam, buffer, min( sizeof(MSG), size ));
        break;
    case SBM_GETSCROLLINFO:
        memcpy( (SCROLLINFO *)lparam, buffer, min( sizeof(SCROLLINFO), size ));
        break;
    case EM_GETRECT:
    case CB_GETDROPPEDCONTROLRECT:
    case LB_GETITEMRECT:
    case WM_SIZING:
    case WM_MOVING:
        memcpy( (RECT *)lparam, buffer, min( sizeof(RECT), size ));
        break;
    case EM_GETLINE:
        size = min( size, (size_t)*(WORD *)lparam );
        memcpy( (WCHAR *)lparam, buffer, size );
        break;
    case LB_GETSELITEMS:
        memcpy( (UINT *)lparam, buffer, min( wparam*sizeof(UINT), size ));
        break;
    case LB_GETTEXT:
    case CB_GETLBTEXT:
        memcpy( (WCHAR *)lparam, buffer, size );
        break;
    case WM_NEXTMENU:
        memcpy( (MDINEXTMENU *)lparam, buffer, min( sizeof(MDINEXTMENU), size ));
        break;
    case WM_MDIGETACTIVE:
        if (lparam) memcpy( (BOOL *)lparam, buffer, min( sizeof(BOOL), size ));
        break;
    case WM_NCCALCSIZE:
        if (!wparam)
            memcpy( (RECT *)lparam, buffer, min( sizeof(RECT), size ));
        else
        {
            NCCALCSIZE_PARAMS *nc = (NCCALCSIZE_PARAMS *)lparam;
            WINDOWPOS *wp = nc->lppos;
            memcpy( nc, buffer, min( sizeof(*nc), size ));
            if (size > sizeof(*nc))
            {
                size -= sizeof(*nc);
                memcpy( wp, (NCCALCSIZE_PARAMS*)buffer + 1, min( sizeof(*wp), size ));
            }
            nc->lppos = wp;  /* restore the original pointer */
        }
        break;
    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
        if (wparam)
        {
            memcpy( (DWORD *)wparam, buffer, min( sizeof(DWORD), size ));
            if (size <= sizeof(DWORD)) break;
            size -= sizeof(DWORD);
            buffer = (DWORD *)buffer + 1;
        }
        if (lparam) memcpy( (DWORD *)lparam, buffer, min( sizeof(DWORD), size ));
        break;
    case WM_MDICREATE:
    {
        MDICREATESTRUCTW *cs = (MDICREATESTRUCTW *)lparam;
        LPCWSTR title = cs->szTitle, class = cs->szClass;
        memcpy( cs, buffer, min( sizeof(*cs), size ));
        cs->szTitle = title;  /* restore the original pointers */
        cs->szClass = class;
        break;
    }
    default:
        ERR( "should not happen: unexpected message %x\n", message );
        break;
    }
}


/***********************************************************************
 *           reply_message
 *
 * Send a reply to a sent message.
 */
static void reply_message( struct received_message_info *info, LRESULT result, BOOL remove )
{
    struct packed_message data;
    int i, replied = info->flags & ISMEX_REPLIED;

    if (info->flags & ISMEX_NOTIFY) return;  /* notify messages don't get replies */
    if (!remove && replied) return;  /* replied already */

    data.count = 0;
    info->flags |= ISMEX_REPLIED;

    if (info->type == MSG_OTHER_PROCESS && !replied)
    {
        pack_reply( info->msg.hwnd, info->msg.message, info->msg.wParam,
                    info->msg.lParam, result, &data );
    }

    SERVER_START_REQ( reply_message )
    {
        req->type   = info->type;
        req->result = result;
        req->remove = remove;
        for (i = 0; i < data.count; i++) wine_server_add_data( req, data.data[i], data.size[i] );
        wine_server_call( req );
    }
    SERVER_END_REQ;
}


/***********************************************************************
 *           handle_internal_message
 *
 * Handle an internal Wine message instead of calling the window proc.
 */
static LRESULT handle_internal_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if (hwnd == GetDesktopWindow()) return 0;
    switch(msg)
    {
    case WM_WINE_DESTROYWINDOW:
        return WIN_DestroyWindow( hwnd );
    case WM_WINE_SETWINDOWPOS:
        return USER_Driver.pSetWindowPos( (WINDOWPOS *)lparam );
    case WM_WINE_SHOWWINDOW:
        return ShowWindow( hwnd, wparam );
    case WM_WINE_SETPARENT:
        return (LRESULT)SetParent( hwnd, (HWND)wparam );
    case WM_WINE_SETWINDOWLONG:
        return (LRESULT)SetWindowLongW( hwnd, wparam, lparam );
    case WM_WINE_ENABLEWINDOW:
        return EnableWindow( hwnd, wparam );
    case WM_WINE_SETACTIVEWINDOW:
        return (LRESULT)SetActiveWindow( (HWND)wparam );
    default:
        FIXME( "unknown internal message %x\n", msg );
        return 0;
    }
}

/* since the WM_DDE_ACK response to a WM_DDE_EXECUTE message should contain the handle
 * to the memory handle, we keep track (in the server side) of all pairs of handle
 * used (the client passes its value and the content of the memory handle), and
 * the server stored both values (the client, and the local one, created after the
 * content). When a ACK message is generated, the list of pair is searched for a
 * matching pair, so that the client memory handle can be returned.
 */
struct DDE_pair {
    HGLOBAL     client_hMem;
    HGLOBAL     server_hMem;
};

static      struct DDE_pair*    dde_pairs;
static      int                 dde_num_alloc;
static      int                 dde_num_used;
static      CRITICAL_SECTION    dde_crst = CRITICAL_SECTION_INIT("Raw_DDE_CritSect");

static BOOL dde_add_pair(HGLOBAL chm, HGLOBAL shm)
{
    int  i;
#define GROWBY  4

    EnterCriticalSection(&dde_crst);

    /* now remember the pair of hMem on both sides */
    if (dde_num_used == dde_num_alloc)
    {
        struct DDE_pair* tmp = HeapReAlloc( GetProcessHeap(), 0, dde_pairs,
                                            (dde_num_alloc + GROWBY) * sizeof(struct DDE_pair));
        if (!tmp)
        {
            LeaveCriticalSection(&dde_crst);
            return FALSE;
        }
        dde_pairs = tmp;
        /* zero out newly allocated part */
        memset(&dde_pairs[dde_num_alloc], 0, GROWBY * sizeof(struct DDE_pair));
        dde_num_alloc += GROWBY;
    }
#undef GROWBY
    for (i = 0; i < dde_num_alloc; i++)
    {
        if (dde_pairs[i].server_hMem == 0)
        {
            dde_pairs[i].client_hMem = chm;
            dde_pairs[i].server_hMem = shm;
            dde_num_used++;
            break;
        }
    }
    LeaveCriticalSection(&dde_crst);
    return TRUE;
}

static HGLOBAL dde_get_pair(HGLOBAL shm)
{
    int  i;
    HGLOBAL     ret = 0;

    EnterCriticalSection(&dde_crst);
    for (i = 0; i < dde_num_alloc; i++)
    {
        if (dde_pairs[i].server_hMem == shm)
        {
            /* free this pair */
            dde_pairs[i].server_hMem = 0;
            dde_num_used--;
            ret = dde_pairs[i].client_hMem;
            break;
        }
    }
    LeaveCriticalSection(&dde_crst);
    return ret;
}

/***********************************************************************
 *		post_dde_message
 *
 * Post a DDE messag
 */
static BOOL post_dde_message( DWORD dest_tid, struct packed_message *data, const struct send_message_info *info )
{
    void*       ptr = NULL;
    int         size = 0;
    UINT        uiLo, uiHi;
    LPARAM      lp = 0;
    HGLOBAL     hunlock = 0;
    int         i;
    DWORD       res;

    if (!UnpackDDElParam( info->msg, info->lparam, &uiLo, &uiHi ))
        return FALSE;

    lp = info->lparam;
    switch (info->msg)
    {
        /* DDE messages which don't require packing are:
         * WM_DDE_INITIATE
         * WM_DDE_TERMINATE
         * WM_DDE_REQUEST
         * WM_DDE_UNADVISE
         */
    case WM_DDE_ACK:
        if (HIWORD(uiHi))
        {
            /* uiHi should contain a hMem from WM_DDE_EXECUTE */
            HGLOBAL h = dde_get_pair( (HANDLE)uiHi );
            if (h)
            {
                /* send back the value of h on the other side */
                push_data( data, &h, sizeof(HGLOBAL) );
                lp = uiLo;
                TRACE( "send dde-ack %x %08x => %08lx\n", uiLo, uiHi, (DWORD)h );
            }
        }
        else
        {
            /* uiHi should contain either an atom or 0 */
            TRACE( "send dde-ack %x atom=%x\n", uiLo, uiHi );
            lp = MAKELONG( uiLo, uiHi );
        }
        break;
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
        size = 0;
        if (uiLo)
        {
            size = GlobalSize( (HGLOBAL)uiLo ) ;
            if ((info->msg == WM_DDE_ADVISE && size < sizeof(DDEADVISE)) ||
                (info->msg == WM_DDE_DATA   && size < sizeof(DDEDATA))   ||
                (info->msg == WM_DDE_POKE   && size < sizeof(DDEPOKE))
                )
            return FALSE;
        }
        else if (info->msg != WM_DDE_DATA) return FALSE;

        lp = uiHi;
        if (uiLo)
        {
            if ((ptr = GlobalLock( (HGLOBAL)uiLo) ))
            {
                push_data( data, ptr, size );
                hunlock = (HGLOBAL)uiLo;
            }
        }
        TRACE( "send ddepack %u %x\n", size, uiHi );
        break;
    case WM_DDE_EXECUTE:
        if (info->lparam)
        {
            if ((ptr = GlobalLock( (HGLOBAL)info->lparam) ))
            {
                push_data(data, ptr, GlobalSize( (HGLOBAL)info->lparam ));
                /* so that the other side can send it back on ACK */
                lp = info->lparam;
                hunlock = (HGLOBAL)info->lparam;
            }
        }
        break;
    }
    SERVER_START_REQ( send_message )
    {
        req->id      = dest_tid;
        req->type    = info->type;
        req->win     = info->hwnd;
        req->msg     = info->msg;
        req->wparam  = info->wparam;
        req->lparam  = lp;
        req->time    = GetCurrentTime();
        req->timeout = -1;
        for (i = 0; i < data->count; i++)
            wine_server_add_data( req, data->data[i], data->size[i] );
        if ((res = wine_server_call( req )))
        {
            if (res == STATUS_INVALID_PARAMETER)
                /* FIXME: find a STATUS_ value for this one */
                SetLastError( ERROR_INVALID_THREAD_ID );
            else
                SetLastError( RtlNtStatusToDosError(res) );
        }
        else
            FreeDDElParam(info->msg, info->lparam);
    }
    SERVER_END_REQ;
    if (hunlock) GlobalUnlock(hunlock);

    return !res;
}

/***********************************************************************
 *		unpack_dde_message
 *
 * Unpack a posted DDE message received from another process.
 */
static BOOL unpack_dde_message( HWND hwnd, UINT message, WPARAM *wparam, LPARAM *lparam,
                                void **buffer, size_t size )
{
    UINT	uiLo, uiHi;
    HGLOBAL	hMem = 0;
    void*	ptr;

    switch (message)
    {
    case WM_DDE_ACK:
        if (size)
        {
            /* hMem is being passed */
            if (size != sizeof(HGLOBAL)) return FALSE;
            if (!buffer || !*buffer) return FALSE;
            uiLo = *lparam;
            memcpy( &hMem, *buffer, size );
            uiHi = (UINT)hMem;
            TRACE("recv dde-ack %u mem=%x[%lx]\n", uiLo, uiHi, GlobalSize( hMem ));
        }
        else
        {
            uiLo = LOWORD( *lparam );
            uiHi = HIWORD( *lparam );
            TRACE("recv dde-ack %x atom=%x\n", uiLo, uiHi);
        }
	*lparam = PackDDElParam( WM_DDE_ACK, uiLo, uiHi );
	break;
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
	if ((!buffer || !*buffer) && message != WM_DDE_DATA) return FALSE;
	uiHi = *lparam;
	TRACE( "recv ddepack %u %x\n", size, uiHi );
        if (size)
        {
            hMem = GlobalAlloc( GMEM_MOVEABLE|GMEM_DDESHARE, size );
            if (hMem && (ptr = GlobalLock( hMem )))
            {
                memcpy( ptr, *buffer, size );
                GlobalUnlock( hMem );
            }
            else return FALSE;
        }
        uiLo = (UINT)hMem;

	*lparam = PackDDElParam( message, uiLo, uiHi );
	break;
    case WM_DDE_EXECUTE:
	if (size)
	{
	    if (!buffer || !*buffer) return FALSE;
	    hMem = GlobalAlloc( GMEM_MOVEABLE|GMEM_DDESHARE, size );
	    if (hMem && (ptr = GlobalLock( hMem )))
	    {
		memcpy( ptr, *buffer, size );
		GlobalUnlock( hMem );
                TRACE( "exec: pairing c=%08lx s=%08lx\n", *lparam, (DWORD)hMem );
                if (!dde_add_pair( (HGLOBAL)*lparam, hMem ))
                {
                    GlobalFree( hMem );
                    return FALSE;
                }
	    }
	} else return FALSE;
        *lparam = (LPARAM)hMem;
        break;
    }
    return TRUE;
}

/***********************************************************************
 *           call_window_proc
 *
 * Call a window procedure and the corresponding hooks.
 */
static LRESULT call_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                 BOOL unicode, BOOL same_thread )
{
    LRESULT result = 0;
    WNDPROC winproc;
    CWPSTRUCT cwp;
    CWPRETSTRUCT cwpret;
    MESSAGEQUEUE *queue = QUEUE_Current();

    if (queue->recursion_count > MAX_SENDMSG_RECURSION) return 0;
    queue->recursion_count++;

    if (msg & 0x80000000)
    {
        result = handle_internal_message( hwnd, msg, wparam, lparam );
        goto done;
    }

    /* first the WH_CALLWNDPROC hook */
    hwnd = WIN_GetFullHandle( hwnd );
    cwp.lParam  = lparam;
    cwp.wParam  = wparam;
    cwp.message = msg;
    cwp.hwnd    = hwnd;
    HOOK_CallHooks( WH_CALLWNDPROC, HC_ACTION, same_thread, (LPARAM)&cwp, unicode );

    /* now call the window procedure */
    if (unicode)
    {
        if (!(winproc = (WNDPROC)GetWindowLongW( hwnd, GWL_WNDPROC ))) goto done;
        result = CallWindowProcW( winproc, hwnd, msg, wparam, lparam );
    }
    else
    {
        if (!(winproc = (WNDPROC)GetWindowLongA( hwnd, GWL_WNDPROC ))) goto done;
        result = CallWindowProcA( winproc, hwnd, msg, wparam, lparam );
    }

    /* and finally the WH_CALLWNDPROCRET hook */
    cwpret.lResult = result;
    cwpret.lParam  = lparam;
    cwpret.wParam  = wparam;
    cwpret.message = msg;
    cwpret.hwnd    = hwnd;
    HOOK_CallHooks( WH_CALLWNDPROCRET, HC_ACTION, same_thread, (LPARAM)&cwpret, unicode );
 done:
    queue->recursion_count--;
    return result;
}


/***********************************************************************
 *           process_hardware_message
 *
 * Process a hardware message; return TRUE if message should be passed on to the app
 */
static BOOL process_hardware_message( MSG *msg, ULONG_PTR extra_info, HWND hwnd,
                                      UINT first, UINT last, BOOL remove )
{
    BOOL ret;

    if (!MSG_process_raw_hardware_message( msg, extra_info, hwnd, first, last, remove ))
        return FALSE;

    ret = MSG_process_cooked_hardware_message( msg, extra_info, remove );

    /* tell the server we have passed it to the app
     * (even though we may end up dropping it later on)
     */
    SERVER_START_REQ( reply_message )
    {
        req->type   = MSG_HARDWARE;
        req->result = 0;
        req->remove = remove || !ret;
        wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           MSG_peek_message
 *
 * Peek for a message matching the given parameters. Return FALSE if none available.
 * All pending sent messages are processed before returning.
 */
BOOL MSG_peek_message( MSG *msg, HWND hwnd, UINT first, UINT last, int flags )
{
    LRESULT result;
    ULONG_PTR extra_info = 0;
    MESSAGEQUEUE *queue = QUEUE_Current();
    struct received_message_info info, *old_info;

    if (!first && !last) last = ~0;

    for (;;)
    {
        NTSTATUS res;
        void *buffer = NULL;
        size_t size = 0, buffer_size = 0;

        do  /* loop while buffer is too small */
        {
            if (buffer_size && !(buffer = HeapAlloc( GetProcessHeap(), 0, buffer_size )))
                return FALSE;
            SERVER_START_REQ( get_message )
            {
                req->flags     = flags;
                req->get_win   = hwnd;
                req->get_first = first;
                req->get_last  = last;
                if (buffer_size) wine_server_set_reply( req, buffer, buffer_size );
                if (!(res = wine_server_call( req )))
                {
                    size = wine_server_reply_size( reply );
                    info.type        = reply->type;
                    info.msg.hwnd    = reply->win;
                    info.msg.message = reply->msg;
                    info.msg.wParam  = reply->wparam;
                    info.msg.lParam  = reply->lparam;
                    info.msg.time    = reply->time;
                    info.msg.pt.x    = reply->x;
                    info.msg.pt.y    = reply->y;
                    extra_info       = reply->info;
                }
                else
                {
                    if (buffer) HeapFree( GetProcessHeap(), 0, buffer );
                    buffer_size = reply->total;
                }
            }
            SERVER_END_REQ;
        } while (res == STATUS_BUFFER_OVERFLOW);

        if (res) return FALSE;

        TRACE( "got type %d msg %x hwnd %p wp %x lp %lx\n",
               info.type, info.msg.message, info.msg.hwnd, info.msg.wParam, info.msg.lParam );

        switch(info.type)
        {
        case MSG_ASCII:
        case MSG_UNICODE:
            info.flags = ISMEX_SEND;
            break;
        case MSG_NOTIFY:
            info.flags = ISMEX_NOTIFY;
            break;
        case MSG_CALLBACK:
            info.flags = ISMEX_CALLBACK;
            break;
        case MSG_OTHER_PROCESS:
            info.flags = ISMEX_SEND;
            if (!unpack_message( info.msg.hwnd, info.msg.message, &info.msg.wParam,
                                 &info.msg.lParam, &buffer, size ))
            {
                ERR( "invalid packed message %x (%s) hwnd %p wp %x lp %lx size %d\n",
                     info.msg.message, SPY_GetMsgName(info.msg.message, info.msg.hwnd), info.msg.hwnd,
                     info.msg.wParam, info.msg.lParam, size );
                /* ignore it */
                reply_message( &info, 0, TRUE );
                goto next;
            }
            break;
        case MSG_HARDWARE:
            if (!process_hardware_message( &info.msg, extra_info,
                                           hwnd, first, last, flags & GET_MSG_REMOVE ))
            {
                TRACE("dropping msg %x\n", info.msg.message );
                goto next;  /* ignore it */
            }
            queue->GetMessagePosVal = MAKELONG( info.msg.pt.x, info.msg.pt.y );
            /* fall through */
        case MSG_POSTED:
            queue->GetMessageExtraInfoVal = extra_info;
	    if (info.msg.message >= WM_DDE_FIRST && info.msg.message <= WM_DDE_LAST)
	    {
		if (!unpack_dde_message( info.msg.hwnd, info.msg.message, &info.msg.wParam,
                                         &info.msg.lParam, &buffer, size ))
		{
		    ERR( "invalid packed dde-message %x (%s) hwnd %p wp %x lp %lx size %d\n",
			 info.msg.message, SPY_GetMsgName(info.msg.message, info.msg.hwnd),
			 info.msg.hwnd, info.msg.wParam, info.msg.lParam, size );
                    goto next;  /* ignore it */
		}
	    }
            *msg = info.msg;
            if (buffer) HeapFree( GetProcessHeap(), 0, buffer );
            return TRUE;
        }

        /* if we get here, we have a sent message; call the window procedure */
        old_info = queue->receive_info;
        queue->receive_info = &info;
        result = call_window_proc( info.msg.hwnd, info.msg.message, info.msg.wParam,
                                   info.msg.lParam, (info.type != MSG_ASCII), FALSE );
        reply_message( &info, result, TRUE );
        queue->receive_info = old_info;
    next:
        if (buffer) HeapFree( GetProcessHeap(), 0, buffer );
    }
}


/***********************************************************************
 *           wait_message_reply
 *
 * Wait until a sent message gets replied to.
 */
static void wait_message_reply( UINT flags )
{
    MESSAGEQUEUE *queue;

    if (!(queue = QUEUE_Current())) return;

    for (;;)
    {
        unsigned int wake_bits = 0, changed_bits = 0;
        DWORD dwlc, res;

        SERVER_START_REQ( set_queue_mask )
        {
            req->wake_mask    = QS_SMRESULT | ((flags & SMTO_BLOCK) ? 0 : QS_SENDMESSAGE);
            req->changed_mask = req->wake_mask;
            req->skip_wait    = 1;
            if (!wine_server_call( req ))
            {
                wake_bits    = reply->wake_bits;
                changed_bits = reply->changed_bits;
            }
        }
        SERVER_END_REQ;

        if (wake_bits & QS_SMRESULT) return;  /* got a result */
        if (wake_bits & QS_SENDMESSAGE)
        {
            /* Process the sent message immediately */
            MSG msg;
            MSG_peek_message( &msg, 0, 0, 0, GET_MSG_REMOVE | GET_MSG_SENT_ONLY );
            continue;
        }

        /* now wait for it */

        ReleaseThunkLock( &dwlc );

        if (USER_Driver.pMsgWaitForMultipleObjectsEx)
            res = USER_Driver.pMsgWaitForMultipleObjectsEx( 1, &queue->server_queue,
                                                            INFINITE, 0, 0 );
        else
            res = WaitForSingleObject( queue->server_queue, INFINITE );

        if (dwlc) RestoreThunkLock( dwlc );
    }
}

/***********************************************************************
 *		put_message_in_queue
 *
 * Put a sent message into the destination queue.
 * For inter-process message, reply_size is set to expected size of reply data.
 */
static BOOL put_message_in_queue( DWORD dest_tid, const struct send_message_info *info,
                                  size_t *reply_size )
{
    struct packed_message data;
    unsigned int res;
    int i, timeout = -1;

    if (info->type != MSG_NOTIFY &&
        info->type != MSG_CALLBACK &&
        info->type != MSG_POSTED &&
        info->timeout != INFINITE)
        timeout = info->timeout;

    data.count = 0;
    if (info->type == MSG_OTHER_PROCESS)
    {
        *reply_size = pack_message( info->hwnd, info->msg, info->wparam, info->lparam, &data );
        if (data.count == -1)
        {
            WARN( "cannot pack message %x\n", info->msg );
            return FALSE;
        }
    }
    else if (info->type == MSG_POSTED && info->msg >= WM_DDE_FIRST && info->msg <= WM_DDE_LAST)
    {
        return post_dde_message( dest_tid, &data, info );
    }

    SERVER_START_REQ( send_message )
    {
        req->id      = dest_tid;
        req->type    = info->type;
        req->win     = info->hwnd;
        req->msg     = info->msg;
        req->wparam  = info->wparam;
        req->lparam  = info->lparam;
        req->time    = GetCurrentTime();
        req->timeout = timeout;
        for (i = 0; i < data.count; i++) wine_server_add_data( req, data.data[i], data.size[i] );
        if ((res = wine_server_call( req )))
        {
            if (res == STATUS_INVALID_PARAMETER)
                /* FIXME: find a STATUS_ value for this one */
                SetLastError( ERROR_INVALID_THREAD_ID );
            else
                SetLastError( RtlNtStatusToDosError(res) );
        }
    }
    SERVER_END_REQ;
    return !res;
}


/***********************************************************************
 *		retrieve_reply
 *
 * Retrieve a message reply from the server.
 */
static LRESULT retrieve_reply( const struct send_message_info *info,
                               size_t reply_size, LRESULT *result )
{
    NTSTATUS status;
    void *reply_data = NULL;

    if (reply_size)
    {
        if (!(reply_data = HeapAlloc( GetProcessHeap(), 0, reply_size )))
        {
            WARN( "no memory for reply %d bytes, will be truncated\n", reply_size );
            reply_size = 0;
        }
    }
    SERVER_START_REQ( get_message_reply )
    {
        req->cancel = 1;
        if (reply_size) wine_server_set_reply( req, reply_data, reply_size );
        if (!(status = wine_server_call( req ))) *result = reply->result;
        reply_size = wine_server_reply_size( reply );
    }
    SERVER_END_REQ;
    if (!status && reply_size)
        unpack_reply( info->hwnd, info->msg, info->wparam, info->lparam, reply_data, reply_size );

    if (reply_data) HeapFree( GetProcessHeap(), 0, reply_data );

    TRACE( "hwnd %p msg %x (%s) wp %x lp %lx got reply %lx (err=%ld)\n",
           info->hwnd, info->msg, SPY_GetMsgName(info->msg, info->hwnd), info->wparam,
           info->lparam, *result, status );

    if (!status) return 1;
    if (status == STATUS_TIMEOUT) SetLastError(0);  /* timeout */
    else SetLastError( RtlNtStatusToDosError(status) );
    return 0;
}


/***********************************************************************
 *		send_inter_thread_message
 */
static LRESULT send_inter_thread_message( DWORD dest_tid, const struct send_message_info *info,
                                          LRESULT *res_ptr )
{
    LRESULT ret;
    int locks;
    size_t reply_size = 0;

    TRACE( "hwnd %p msg %x (%s) wp %x lp %lx\n",
           info->hwnd, info->msg, SPY_GetMsgName(info->msg, info->hwnd), info->wparam, info->lparam );

    if (!put_message_in_queue( dest_tid, info, &reply_size )) return 0;

    /* there's no reply to wait for on notify/callback messages */
    if (info->type == MSG_NOTIFY || info->type == MSG_CALLBACK) return 1;

    locks = WIN_SuspendWndsLock();

    wait_message_reply( info->flags );
    ret = retrieve_reply( info, reply_size, res_ptr );

    WIN_RestoreWndsLock( locks );
    return ret;
}


/***********************************************************************
 *		SendMessageTimeoutW  (USER32.@)
 */
LRESULT WINAPI SendMessageTimeoutW( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                    UINT flags, UINT timeout, PDWORD_PTR res_ptr )
{
    struct send_message_info info;
    DWORD dest_tid, dest_pid;
    LRESULT ret, result;

    info.type    = MSG_UNICODE;
    info.hwnd    = hwnd;
    info.msg     = msg;
    info.wparam  = wparam;
    info.lparam  = lparam;
    info.flags   = flags;
    info.timeout = timeout;

    if (is_broadcast(hwnd))
    {
        EnumWindows( broadcast_message_callback, (LPARAM)&info );
        if (res_ptr) *res_ptr = 1;
        return 1;
    }

    if (!(dest_tid = GetWindowThreadProcessId( hwnd, &dest_pid ))) return 0;

    if (USER_IsExitingThread( dest_tid )) return 0;

    SPY_EnterMessage( SPY_SENDMESSAGE, hwnd, msg, wparam, lparam );

    if (dest_tid == GetCurrentThreadId())
    {
        result = call_window_proc( hwnd, msg, wparam, lparam, TRUE, TRUE );
        ret = 1;
    }
    else
    {
        if (dest_pid != GetCurrentProcessId()) info.type = MSG_OTHER_PROCESS;
        ret = send_inter_thread_message( dest_tid, &info, &result );
    }

    SPY_ExitMessage( SPY_RESULT_OK, hwnd, msg, result, wparam, lparam );
    if (ret && res_ptr) *res_ptr = result;
    return ret;
}


/***********************************************************************
 *		SendMessageTimeoutA  (USER32.@)
 */
LRESULT WINAPI SendMessageTimeoutA( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                    UINT flags, UINT timeout, PDWORD_PTR res_ptr )
{
    struct send_message_info info;
    DWORD dest_tid, dest_pid;
    LRESULT ret, result;

    info.type    = MSG_ASCII;
    info.hwnd    = hwnd;
    info.msg     = msg;
    info.wparam  = wparam;
    info.lparam  = lparam;
    info.flags   = flags;
    info.timeout = timeout;

    if (is_broadcast(hwnd))
    {
        EnumWindows( broadcast_message_callback, (LPARAM)&info );
        if (res_ptr) *res_ptr = 1;
        return 1;
    }

    if (!(dest_tid = GetWindowThreadProcessId( hwnd, &dest_pid ))) return 0;

    if (USER_IsExitingThread( dest_tid )) return 0;

    SPY_EnterMessage( SPY_SENDMESSAGE, hwnd, msg, wparam, lparam );

    if (dest_tid == GetCurrentThreadId())
    {
        result = call_window_proc( hwnd, msg, wparam, lparam, FALSE, TRUE );
        ret = 1;
    }
    else if (dest_pid == GetCurrentProcessId())
    {
        ret = send_inter_thread_message( dest_tid, &info, &result );
    }
    else
    {
        /* inter-process message: need to map to Unicode */
        info.type = MSG_OTHER_PROCESS;
        if (is_unicode_message( info.msg ))
        {
            if (WINPROC_MapMsg32ATo32W( info.hwnd, info.msg, &info.wparam, &info.lparam ) == -1)
                return 0;
            ret = send_inter_thread_message( dest_tid, &info, &result );
            result = WINPROC_UnmapMsg32ATo32W( info.hwnd, info.msg, info.wparam,
                                               info.lparam, result );
        }
        else ret = send_inter_thread_message( dest_tid, &info, &result );
    }
    SPY_ExitMessage( SPY_RESULT_OK, hwnd, msg, result, wparam, lparam );
    if (ret && res_ptr) *res_ptr = result;
    return ret;
}


/***********************************************************************
 *		SendMessageW  (USER32.@)
 */
LRESULT WINAPI SendMessageW( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    LRESULT res = 0;
    SendMessageTimeoutW( hwnd, msg, wparam, lparam, SMTO_NORMAL, INFINITE, &res );
    return res;
}


/***********************************************************************
 *		SendMessageA  (USER32.@)
 */
LRESULT WINAPI SendMessageA( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    LRESULT res = 0;
    SendMessageTimeoutA( hwnd, msg, wparam, lparam, SMTO_NORMAL, INFINITE, &res );
    return res;
}


/***********************************************************************
 *		SendNotifyMessageA  (USER32.@)
 */
BOOL WINAPI SendNotifyMessageA( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    return SendNotifyMessageW( hwnd, msg, map_wparam_AtoW( msg, wparam ), lparam );
}


/***********************************************************************
 *		SendNotifyMessageW  (USER32.@)
 */
BOOL WINAPI SendNotifyMessageW( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    struct send_message_info info;
    DWORD dest_tid;
    LRESULT result;

    if (is_pointer_message(msg))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    info.type    = MSG_NOTIFY;
    info.hwnd    = hwnd;
    info.msg     = msg;
    info.wparam  = wparam;
    info.lparam  = lparam;

    if (is_broadcast(hwnd))
    {
        EnumWindows( broadcast_message_callback, (LPARAM)&info );
        return TRUE;
    }

    if (!(dest_tid = GetWindowThreadProcessId( hwnd, NULL ))) return FALSE;

    if (USER_IsExitingThread( dest_tid )) return TRUE;

    if (dest_tid == GetCurrentThreadId())
    {
        call_window_proc( hwnd, msg, wparam, lparam, TRUE, TRUE );
        return TRUE;
    }
    return send_inter_thread_message( dest_tid, &info, &result );
}


/***********************************************************************
 *		SendMessageCallbackA  (USER32.@)
 */
BOOL WINAPI SendMessageCallbackA( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                  SENDASYNCPROC callback, ULONG_PTR data )
{
    return SendMessageCallbackW( hwnd, msg, map_wparam_AtoW( msg, wparam ),
                                 lparam, callback, data );
}


/***********************************************************************
 *		SendMessageCallbackW  (USER32.@)
 */
BOOL WINAPI SendMessageCallbackW( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                  SENDASYNCPROC callback, ULONG_PTR data )
{
    struct send_message_info info;
    LRESULT result;
    DWORD dest_tid;

    if (is_pointer_message(msg))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    info.type     = MSG_CALLBACK;
    info.hwnd     = hwnd;
    info.msg      = msg;
    info.wparam   = wparam;
    info.lparam   = lparam;
    info.callback = callback;
    info.data     = data;

    if (is_broadcast(hwnd))
    {
        EnumWindows( broadcast_message_callback, (LPARAM)&info );
        return TRUE;
    }

    if (!(dest_tid = GetWindowThreadProcessId( hwnd, NULL ))) return FALSE;

    if (USER_IsExitingThread( dest_tid )) return TRUE;

    if (dest_tid == GetCurrentThreadId())
    {
        result = call_window_proc( hwnd, msg, wparam, lparam, TRUE, TRUE );
        callback( hwnd, msg, data, result );
        return TRUE;
    }
    FIXME( "callback will not be called\n" );
    return send_inter_thread_message( dest_tid, &info, &result );
}


/***********************************************************************
 *		ReplyMessage  (USER32.@)
 */
BOOL WINAPI ReplyMessage( LRESULT result )
{
    MESSAGEQUEUE *queue = QUEUE_Current();
    struct received_message_info *info = queue->receive_info;

    if (!info) return FALSE;
    reply_message( info, result, FALSE );
    return TRUE;
}


/***********************************************************************
 *		InSendMessage  (USER32.@)
 */
BOOL WINAPI InSendMessage(void)
{
    return (InSendMessageEx(NULL) & (ISMEX_SEND|ISMEX_REPLIED)) == ISMEX_SEND;
}


/***********************************************************************
 *		InSendMessageEx  (USER32.@)
 */
DWORD WINAPI InSendMessageEx( LPVOID reserved )
{
    MESSAGEQUEUE *queue = QUEUE_Current();
    struct received_message_info *info = queue->receive_info;

    if (info) return info->flags;
    return ISMEX_NOSEND;
}


/***********************************************************************
 *		PostMessageA  (USER32.@)
 */
BOOL WINAPI PostMessageA( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    return PostMessageW( hwnd, msg, map_wparam_AtoW( msg, wparam ), lparam );
}


/***********************************************************************
 *		PostMessageW  (USER32.@)
 */
BOOL WINAPI PostMessageW( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    struct send_message_info info;
    DWORD dest_tid;

    if (is_pointer_message( msg ))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    info.type   = MSG_POSTED;
    info.hwnd   = hwnd;
    info.msg    = msg;
    info.wparam = wparam;
    info.lparam = lparam;

    if (is_broadcast(hwnd))
    {
        EnumWindows( broadcast_message_callback, (LPARAM)&info );
        return TRUE;
    }

    if (!hwnd) return PostThreadMessageW( GetCurrentThreadId(), msg, wparam, lparam );

    if (!(dest_tid = GetWindowThreadProcessId( hwnd, NULL ))) return FALSE;

    if (USER_IsExitingThread( dest_tid )) return TRUE;

    return put_message_in_queue( dest_tid, &info, NULL );
}


/**********************************************************************
 *		PostThreadMessageA  (USER32.@)
 */
BOOL WINAPI PostThreadMessageA( DWORD thread, UINT msg, WPARAM wparam, LPARAM lparam )
{
    return PostThreadMessageW( thread, msg, map_wparam_AtoW( msg, wparam ), lparam );
}


/**********************************************************************
 *		PostThreadMessageW  (USER32.@)
 */
BOOL WINAPI PostThreadMessageW( DWORD thread, UINT msg, WPARAM wparam, LPARAM lparam )
{
    struct send_message_info info;

    if (is_pointer_message( msg ))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (USER_IsExitingThread( thread )) return TRUE;

    info.type   = MSG_POSTED;
    info.hwnd   = 0;
    info.msg    = msg;
    info.wparam = wparam;
    info.lparam = lparam;
    return put_message_in_queue( thread, &info, NULL );
}


/***********************************************************************
 *		PostQuitMessage  (USER32.@)
 */
void WINAPI PostQuitMessage( INT exitCode )
{
    PostThreadMessageW( GetCurrentThreadId(), WM_QUIT, exitCode, 0 );
}


/***********************************************************************
 *		PeekMessageW  (USER32.@)
 */
BOOL WINAPI PeekMessageW( MSG *msg_out, HWND hwnd, UINT first, UINT last, UINT flags )
{
    MESSAGEQUEUE *queue;
    MSG msg;
    int locks;

    /* check for graphics events */
    if (USER_Driver.pMsgWaitForMultipleObjectsEx)
        USER_Driver.pMsgWaitForMultipleObjectsEx( 0, NULL, 0, 0, 0 );

    hwnd = WIN_GetFullHandle( hwnd );
    locks = WIN_SuspendWndsLock();

    if (!MSG_peek_message( &msg, hwnd, first, last,
                           (flags & PM_REMOVE) ? GET_MSG_REMOVE : 0 ))
    {
        if (!(flags & PM_NOYIELD))
        {
            DWORD count;
            ReleaseThunkLock(&count);
            if (count) RestoreThunkLock(count);
        }
        WIN_RestoreWndsLock( locks );
        return FALSE;
    }

    WIN_RestoreWndsLock( locks );

    /* need to fill the window handle for WM_PAINT message */
    if (msg.message == WM_PAINT)
    {
        if (IsIconic( msg.hwnd ) && GetClassLongA( msg.hwnd, GCL_HICON ))
        {
            msg.message = WM_PAINTICON;
            msg.wParam = 1;
        }
        /* clear internal paint flag */
        RedrawWindow( msg.hwnd, NULL, 0, RDW_NOINTERNALPAINT | RDW_NOCHILDREN );
    }

    if ((queue = QUEUE_Current()))
    {
        queue->GetMessageTimeVal = msg.time;
        msg.pt.x = LOWORD( queue->GetMessagePosVal );
        msg.pt.y = HIWORD( queue->GetMessagePosVal );
    }

    HOOK_CallHooks( WH_GETMESSAGE, HC_ACTION, flags & PM_REMOVE, (LPARAM)&msg, TRUE );

    /* copy back our internal safe copy of message data to msg_out.
     * msg_out is a variable from the *program*, so it can't be used
     * internally as it can get "corrupted" by our use of SendMessage()
     * (back to the program) inside the message handling itself. */
    *msg_out = msg;
    return TRUE;
}


/***********************************************************************
 *		PeekMessageA  (USER32.@)
 */
BOOL WINAPI PeekMessageA( MSG *msg, HWND hwnd, UINT first, UINT last, UINT flags )
{
    BOOL ret = PeekMessageW( msg, hwnd, first, last, flags );
    if (ret) msg->wParam = map_wparam_WtoA( msg->message, msg->wParam );
    return ret;
}


/***********************************************************************
 *		GetMessageW  (USER32.@)
 */
BOOL WINAPI GetMessageW( MSG *msg, HWND hwnd, UINT first, UINT last )
{
    MESSAGEQUEUE *queue = QUEUE_Current();
    int mask, locks;

    mask = QS_POSTMESSAGE | QS_SENDMESSAGE;  /* Always selected */
    if (first || last)
    {
        if ((first <= WM_KEYLAST) && (last >= WM_KEYFIRST)) mask |= QS_KEY;
        if ( ((first <= WM_MOUSELAST) && (last >= WM_MOUSEFIRST)) ||
             ((first <= WM_NCMOUSELAST) && (last >= WM_NCMOUSEFIRST)) ) mask |= QS_MOUSE;
        if ((first <= WM_TIMER) && (last >= WM_TIMER)) mask |= QS_TIMER;
        if ((first <= WM_SYSTIMER) && (last >= WM_SYSTIMER)) mask |= QS_TIMER;
        if ((first <= WM_PAINT) && (last >= WM_PAINT)) mask |= QS_PAINT;
    }
    else mask |= QS_MOUSE | QS_KEY | QS_TIMER | QS_PAINT;

    locks = WIN_SuspendWndsLock();

    while (!PeekMessageW( msg, hwnd, first, last, PM_REMOVE ))
    {
        /* wait until one of the bits is set */
        unsigned int wake_bits = 0, changed_bits = 0;
        DWORD dwlc;

        SERVER_START_REQ( set_queue_mask )
        {
            req->wake_mask    = QS_SENDMESSAGE;
            req->changed_mask = mask;
            req->skip_wait    = 1;
            if (!wine_server_call( req ))
            {
                wake_bits    = reply->wake_bits;
                changed_bits = reply->changed_bits;
            }
        }
        SERVER_END_REQ;

        if (changed_bits & mask) continue;
        if (wake_bits & QS_SENDMESSAGE) continue;

        TRACE( "(%04x) mask=%08x, bits=%08x, changed=%08x, waiting\n",
               queue->self, mask, wake_bits, changed_bits );

        ReleaseThunkLock( &dwlc );
        if (USER_Driver.pMsgWaitForMultipleObjectsEx)
            USER_Driver.pMsgWaitForMultipleObjectsEx( 1, &queue->server_queue, INFINITE, 0, 0 );
        else
            WaitForSingleObject( queue->server_queue, INFINITE );
        if (dwlc) RestoreThunkLock( dwlc );
    }

    WIN_RestoreWndsLock( locks );

    return (msg->message != WM_QUIT);
}


/***********************************************************************
 *		GetMessageA  (USER32.@)
 */
BOOL WINAPI GetMessageA( MSG *msg, HWND hwnd, UINT first, UINT last )
{
    GetMessageW( msg, hwnd, first, last );
    msg->wParam = map_wparam_WtoA( msg->message, msg->wParam );
    return (msg->message != WM_QUIT);
}


/***********************************************************************
 *		IsDialogMessage  (USER32.@)
 *		IsDialogMessageA (USER32.@)
 */
BOOL WINAPI IsDialogMessageA( HWND hwndDlg, LPMSG pmsg )
{
    MSG msg = *pmsg;
    msg.wParam = map_wparam_AtoW( msg.message, msg.wParam );
    return IsDialogMessageW( hwndDlg, &msg );
}


/***********************************************************************
 *		SetMessageQueue (USER32.@)
 */
BOOL WINAPI SetMessageQueue( INT size )
{
    /* now obsolete the message queue will be expanded dynamically as necessary */
    return TRUE;
}


/**********************************************************************
 *		AttachThreadInput (USER32.@)
 *
 * Attaches the input processing mechanism of one thread to that of
 * another thread.
 */
BOOL WINAPI AttachThreadInput( DWORD from, DWORD to, BOOL attach )
{
    BOOL ret;

    SERVER_START_REQ( attach_thread_input )
    {
        req->tid_from = from;
        req->tid_to   = to;
        req->attach   = attach;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 *		GetGUIThreadInfo  (USER32.@)
 */
BOOL WINAPI GetGUIThreadInfo( DWORD id, GUITHREADINFO *info )
{
    BOOL ret;

    SERVER_START_REQ( get_thread_input )
    {
        req->tid = id;
        if ((ret = !wine_server_call_err( req )))
        {
            info->flags          = 0;
            info->hwndActive     = reply->active;
            info->hwndFocus      = reply->focus;
            info->hwndCapture    = reply->capture;
            info->hwndMenuOwner  = reply->menu_owner;
            info->hwndMoveSize   = reply->move_size;
            info->hwndCaret      = reply->caret;
            info->rcCaret.left   = reply->rect.left;
            info->rcCaret.top    = reply->rect.top;
            info->rcCaret.right  = reply->rect.right;
            info->rcCaret.bottom = reply->rect.bottom;
            if (reply->menu_owner) info->flags |= GUI_INMENUMODE;
            if (reply->move_size) info->flags |= GUI_INMOVESIZE;
            if (reply->caret) info->flags |= GUI_CARETBLINKING;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 *		GetKeyState (USER32.@)
 *
 * An application calls the GetKeyState function in response to a
 * keyboard-input message.  This function retrieves the state of the key
 * at the time the input message was generated.
 */
SHORT WINAPI GetKeyState(INT vkey)
{
    SHORT retval = 0;

    SERVER_START_REQ( get_key_state )
    {
        req->tid = GetCurrentThreadId();
        req->key = vkey;
        if (!wine_server_call( req )) retval = (signed char)reply->state;
    }
    SERVER_END_REQ;
    TRACE("key (0x%x) -> %x\n", vkey, retval);
    return retval;
}


/**********************************************************************
 *		GetKeyboardState (USER32.@)
 */
BOOL WINAPI GetKeyboardState( LPBYTE state )
{
    BOOL ret;

    TRACE("(%p)\n", state);

    memset( state, 0, 256 );
    SERVER_START_REQ( get_key_state )
    {
        req->tid = GetCurrentThreadId();
        req->key = -1;
        wine_server_set_reply( req, state, 256 );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 *		SetKeyboardState (USER32.@)
 */
BOOL WINAPI SetKeyboardState( LPBYTE state )
{
    BOOL ret;

    TRACE("(%p)\n", state);

    SERVER_START_REQ( set_key_state )
    {
        req->tid = GetCurrentThreadId();
        wine_server_add_data( req, state, 256 );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}
