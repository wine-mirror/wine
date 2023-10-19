/*
 * COMMDLG - File Dialogs
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
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
#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"
#include "commdlg.h"
#include "cdlg16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);


static inline WORD get_word( const char **ptr )
{
    WORD ret = *(WORD *)*ptr;
    *ptr += sizeof(WORD);
    return ret;
}

static inline void copy_string( WORD **out, const char **in, DWORD maxlen )
{
    DWORD len = MultiByteToWideChar( CP_ACP, 0, *in, -1, *out, maxlen );
    *in += strlen(*in) + 1;
    *out += len;
}

static inline void copy_dword( WORD **out, const char **in )
{
    *(DWORD *)*out = *(DWORD *)*in;
    *in += sizeof(DWORD);
    *out += sizeof(DWORD) / sizeof(WORD);
}

static LPDLGTEMPLATEA convert_dialog( const char *p, DWORD size )
{
    LPDLGTEMPLATEA dlg;
    WORD len, count, *out, *end;

    if (!(dlg = HeapAlloc( GetProcessHeap(), 0, size * 2 ))) return NULL;
    out = (WORD *)dlg;
    end = out + size;
    copy_dword( &out, &p );  /* style */
    *out++ = 0; *out++ = 0;  /* exstyle */
    *out++ = count = (BYTE)*p++;  /* count */
    *out++ = get_word( &p );  /* x */
    *out++ = get_word( &p );  /* y */
    *out++ = get_word( &p );  /* cx */
    *out++ = get_word( &p );  /* cy */

    if ((BYTE)*p == 0xff)  /* menu */
    {
        p++;
        *out++ = 0xffff;
        *out++ = get_word( &p );
    }
    else copy_string( &out, &p, end - out );

    copy_string( &out, &p, end - out );  /* class */
    copy_string( &out, &p, end - out );  /* caption */

    if (dlg->style & DS_SETFONT)
    {
        *out++ = get_word( &p );  /* point size */
        copy_string( &out, &p, end - out );  /* face name */
    }

    /* controls */
    while (count--)
    {
        WORD x = get_word( &p );
        WORD y = get_word( &p );
        WORD cx = get_word( &p );
        WORD cy = get_word( &p );
        WORD id = get_word( &p );

        out = (WORD *)(((UINT_PTR)out + 3) & ~3);

        copy_dword( &out, &p );  /* style */
        *out++ = 0; *out++ = 0;  /* exstyle */
        *out++ = x;
        *out++ = y;
        *out++ = cx;
        *out++ = cy;
        *out++ = id;

        if (*p & 0x80)  /* class */
        {
            *out++ = 0xffff;
            *out++ = (BYTE)*p++;
        }
        else copy_string( &out, &p, end - out );

        if ((BYTE)*p == 0xff)  /* window */
        {
            *out++ = 0xffff;
            *out++ = get_word( &p );
        }
        else copy_string( &out, &p, end - out );

        len = (BYTE)*p++;  /* data */
        *out++ = (len + 1) & ~1;
        memcpy( out, p, len );
        p += len;
        out += (len + 1) / sizeof(WORD);
    }

    assert( out <= end );
    return dlg;
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
    to->hwnd            = HWND_32(from->hwnd);
    to->hwndInsertAfter = (from->hwndInsertAfter == (HWND16)-1) ?
                           HWND_TOPMOST : HWND_32(from->hwndInsertAfter);
    to->x               = from->x;
    to->y               = from->y;
    to->cx              = from->cx;
    to->cy              = from->cy;
    to->flags           = from->flags;
}

static void CREATESTRUCT32Ato16( const CREATESTRUCTA* from, CREATESTRUCT16* to )
{
    to->lpCreateParams = (SEGPTR)from->lpCreateParams;
    to->hInstance      = 0;
    to->hMenu          = HMENU_16(from->hMenu);
    to->hwndParent     = HWND_16(from->hwndParent);
    to->cy             = from->cy;
    to->cx             = from->cx;
    to->y              = from->y;
    to->x              = from->x;
    to->style          = from->style;
    to->dwExStyle      = from->dwExStyle;
}

static LRESULT call_hook16( WNDPROC16 hook, HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    CONTEXT context;
    WORD params[5];

    TRACE( "%p: %p %08x %x %Ix: stub\n", hook, hwnd, msg, wp, lp );

    memset( &context, 0, sizeof(context) );
    context.SegDs = context.SegEs = CURRENT_SS;
    context.SegCs = SELECTOROF( hook );
    context.Eip   = OFFSETOF( hook );
    context.Ebp   = CURRENT_SP + FIELD_OFFSET( STACK16FRAME, bp );
    context.Eax   = context.SegDs;

    params[4] = HWND_16( hwnd );
    params[3] = msg;
    params[2] = wp;
    params[1] = HIWORD( lp );
    params[0] = LOWORD( lp );
    WOWCallback16Ex( 0, WCB16_REGS, sizeof(params), params, (DWORD *)&context );
    return LOWORD( context.Eax );
}

static UINT_PTR CALLBACK call_hook_proc( WNDPROC16 hook, HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    LRESULT ret = 0;

    switch (msg)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCTA *cs32 = (CREATESTRUCTA *)lp;
            CREATESTRUCT16 cs;

            CREATESTRUCT32Ato16( cs32, &cs );
            cs.lpszName  = MapLS( (void *)cs32->lpszName );
            cs.lpszClass = MapLS( (void *)cs32->lpszClass );
            lp = MapLS( &cs );
            ret = call_hook16( hook, hwnd, msg, wp, lp );
            UnMapLS( lp );
            UnMapLS( cs.lpszName );
            UnMapLS( cs.lpszClass );
        }
        break;
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO *mmi32 = (MINMAXINFO *)lp;
            MINMAXINFO16 mmi;

            MINMAXINFO32to16( mmi32, &mmi );
            lp = MapLS( &mmi );
            ret = call_hook16( hook, hwnd, msg, wp, lp );
            UnMapLS( lp );
            MINMAXINFO16to32( &mmi, mmi32 );
        }
        break;
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS *nc32 = (NCCALCSIZE_PARAMS *)lp;
            NCCALCSIZE_PARAMS16 nc;
            WINDOWPOS16 winpos;

            RECT32to16( &nc32->rgrc[0], &nc.rgrc[0] );
            if (wp)
            {
                RECT32to16( &nc32->rgrc[1], &nc.rgrc[1] );
                RECT32to16( &nc32->rgrc[2], &nc.rgrc[2] );
                WINDOWPOS32to16( nc32->lppos, &winpos );
                nc.lppos = MapLS( &winpos );
            }
            lp = MapLS( &nc );
            ret = call_hook16( hook, hwnd, msg, wp, lp );
            UnMapLS( lp );
            RECT16to32( &nc.rgrc[0], &nc32->rgrc[0] );
            if (wp)
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
            WINDOWPOS *winpos32 = (WINDOWPOS *)lp;
            WINDOWPOS16 winpos;

            WINDOWPOS32to16( winpos32, &winpos );
            lp = MapLS( &winpos );
            ret = call_hook16( hook, hwnd, msg, wp, lp );
            UnMapLS( lp );
            WINDOWPOS16to32( &winpos, winpos32 );
        }
        break;
    case WM_COMPAREITEM:
        {
            COMPAREITEMSTRUCT *cis32 = (COMPAREITEMSTRUCT *)lp;
            COMPAREITEMSTRUCT16 cis;
            cis.CtlType    = cis32->CtlType;
            cis.CtlID      = cis32->CtlID;
            cis.hwndItem   = HWND_16( cis32->hwndItem );
            cis.itemID1    = cis32->itemID1;
            cis.itemData1  = cis32->itemData1;
            cis.itemID2    = cis32->itemID2;
            cis.itemData2  = cis32->itemData2;
            lp = MapLS( &cis );
            ret = call_hook16( hook, hwnd, msg, wp, lp );
            UnMapLS( lp );
        }
        break;
    case WM_DELETEITEM:
        {
            DELETEITEMSTRUCT *dis32 = (DELETEITEMSTRUCT *)lp;
            DELETEITEMSTRUCT16 dis;
            dis.CtlType  = dis32->CtlType;
            dis.CtlID    = dis32->CtlID;
            dis.itemID   = dis32->itemID;
            dis.hwndItem = (dis.CtlType == ODT_MENU) ? (HWND16)LOWORD(dis32->hwndItem)
                                                     : HWND_16( dis32->hwndItem );
            dis.itemData = dis32->itemData;
            lp = MapLS( &dis );
            ret = call_hook16( hook, hwnd, msg, wp, lp );
            UnMapLS( lp );
        }
        break;
    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT *dis32 = (DRAWITEMSTRUCT *)lp;
            DRAWITEMSTRUCT16 dis;
            dis.CtlType       = dis32->CtlType;
            dis.CtlID         = dis32->CtlID;
            dis.itemID        = dis32->itemID;
            dis.itemAction    = dis32->itemAction;
            dis.itemState     = dis32->itemState;
            dis.hwndItem      = HWND_16( dis32->hwndItem );
            dis.hDC           = HDC_16( dis32->hDC );
            dis.itemData      = dis32->itemData;
            dis.rcItem.left   = dis32->rcItem.left;
            dis.rcItem.top    = dis32->rcItem.top;
            dis.rcItem.right  = dis32->rcItem.right;
            dis.rcItem.bottom = dis32->rcItem.bottom;
            lp = MapLS( &dis );
            ret = call_hook16( hook, hwnd, msg, wp, lp );
            UnMapLS( lp );
        }
        break;
    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT *mis32 = (MEASUREITEMSTRUCT *)lp;
            MEASUREITEMSTRUCT16 mis;
            mis.CtlType    = mis32->CtlType;
            mis.CtlID      = mis32->CtlID;
            mis.itemID     = mis32->itemID;
            mis.itemWidth  = mis32->itemWidth;
            mis.itemHeight = mis32->itemHeight;
            mis.itemData   = mis32->itemData;
            lp = MapLS( &mis );
            ret = call_hook16( hook, hwnd, msg, wp, lp );
            UnMapLS( lp );
            mis32->itemWidth  = mis.itemWidth;
            mis32->itemHeight = mis.itemHeight;
        }
        break;
    case WM_COPYDATA:
        {
            COPYDATASTRUCT *cds32 = (COPYDATASTRUCT *)lp;
            COPYDATASTRUCT16 cds;

            cds.dwData = cds32->dwData;
            cds.cbData = cds32->cbData;
            cds.lpData = MapLS( cds32->lpData );
            lp = MapLS( &cds );
            ret = call_hook16( hook, hwnd, msg, wp, lp );
            UnMapLS( lp );
            UnMapLS( cds.lpData );
        }
        break;
    case WM_GETDLGCODE:
        if (lp)
        {
            MSG *msg32 = (MSG *)lp;
            MSG16 msg16;

            msg16.hwnd    = HWND_16( msg32->hwnd );
            msg16.message = msg32->message;
            msg16.wParam  = msg32->wParam;
            msg16.lParam  = msg32->lParam;
            msg16.time    = msg32->time;
            msg16.pt.x    = msg32->pt.x;
            msg16.pt.y    = msg32->pt.y;
            lp = MapLS( &msg16 );
            ret = call_hook16( hook, hwnd, msg, wp, lp );
            UnMapLS( lp );
        }
        else
            ret = call_hook16( hook, hwnd, msg, wp, lp );
        break;
    case WM_NEXTMENU:
        {
            LRESULT result;
            MDINEXTMENU *next = (MDINEXTMENU *)lp;
            ret = call_hook16( hook, hwnd, msg, wp, (LPARAM)next->hmenuIn );
            result = GetWindowLongPtrW( hwnd, DWLP_MSGRESULT );
            next->hmenuNext = HMENU_32( LOWORD(result) );
            next->hwndNext  = HWND_32( HIWORD(result) );
            SetWindowLongPtrW( hwnd, DWLP_MSGRESULT, 0 );
        }
        break;
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        wp = min( wp, 0xff80 ); /* Must be < 64K */
        /* fall through */
    case WM_NOTIFY:
    case WM_SETTEXT:
    case WM_WININICHANGE:
    case WM_DEVMODECHANGE:
        lp = MapLS( (void *)lp );
        ret = call_hook16( hook, hwnd, msg, wp, lp );
        UnMapLS( lp );
        break;
    case WM_ACTIVATE:
    case WM_CHARTOITEM:
    case WM_COMMAND:
    case WM_VKEYTOITEM:
        ret = call_hook16( hook, hwnd, msg, wp, MAKELPARAM( (HWND16)lp, HIWORD(wp) ));
        break;
    case WM_HSCROLL:
    case WM_VSCROLL:
        ret = call_hook16( hook, hwnd, msg, wp, MAKELPARAM( HIWORD(wp), (HWND16)lp ));
        break;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        ret = call_hook16( hook, hwnd, WM_CTLCOLOR, wp, MAKELPARAM( (HWND16)lp, msg - WM_CTLCOLORMSGBOX ));
        break;
    case WM_MENUSELECT:
        if(HIWORD(wp) & MF_POPUP)
        {
            HMENU hmenu;
            if ((HIWORD(wp) != 0xffff) || lp)
            {
                if ((hmenu = GetSubMenu( (HMENU)lp, LOWORD(wp) )))
                {
                    ret = call_hook16( hook, hwnd, msg, HMENU_16(hmenu),
                                       MAKELPARAM( HIWORD(wp), (HMENU16)lp ) );
                    break;
                }
            }
        }
        /* fall through */
    case WM_MENUCHAR:
        ret = call_hook16( hook, hwnd, msg, wp, MAKELPARAM( HIWORD(wp), (HMENU16)lp ));
        break;
    case WM_PARENTNOTIFY:
        if ((LOWORD(wp) == WM_CREATE) || (LOWORD(wp) == WM_DESTROY))
            ret = call_hook16( hook, hwnd, msg, wp, MAKELPARAM( (HWND16)lp, HIWORD(wp) ));
        else
            ret = call_hook16( hook, hwnd, msg, wp, lp );
        break;
    case WM_ACTIVATEAPP:
        ret = call_hook16( hook, hwnd, msg, wp, HTASK_16( lp ));
        break;
    case WM_INITDIALOG:
        {
            OPENFILENAMEA *ofn = (OPENFILENAMEA *)lp;
            ret = call_hook16( hook, hwnd, msg, wp, ofn->lCustData );
            break;
        }
    default:
        ret = call_hook16( hook, hwnd, msg, wp, lp );
        break;
    }
    return LOWORD(ret);
}


#include "pshpack1.h"
struct hook_proc
{
    BYTE popl_eax;    /* popl %eax */
    BYTE pushl_hook;  /* pushl $hook_ptr */
    LPOFNHOOKPROC16 hook_ptr;
    BYTE pushl_eax;   /* pushl %eax */
    BYTE jmp;         /* jmp call_hook */
    DWORD call_hook;
};
#include "poppack.h"

static LPOFNHOOKPROC alloc_hook( LPOFNHOOKPROC16 hook16 )
{
    static struct hook_proc *hooks;
    static unsigned int count;
    SIZE_T size = 0x1000;
    unsigned int i;

    if (!hooks && !(hooks = VirtualAlloc( NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE )))
        return NULL;

    for (i = 0; i < count; i++)
        if (hooks[i].hook_ptr == hook16)
            return (LPOFNHOOKPROC)&hooks[i];

    if (count >= size / sizeof(*hooks))
    {
        FIXME( "all hooks are in use\n" );
        return NULL;
    }

    hooks[count].popl_eax   = 0x58;
    hooks[count].pushl_hook = 0x68;
    hooks[count].hook_ptr   = hook16;
    hooks[count].pushl_eax  = 0x50;
    hooks[count].jmp        = 0xe9;
    hooks[count].call_hook  = (char *)call_hook_proc - (char *)(&hooks[count].call_hook + 1);
    return (LPOFNHOOKPROC)&hooks[count++];
}

static UINT_PTR CALLBACK dummy_hook( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    return FALSE;
}

/***********************************************************************
 *           FileOpenDlgProc   (COMMDLG.6)
 */
BOOL16 CALLBACK FileOpenDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam)
{
    FIXME( "%04x %04x %04x %08Ix: stub\n", hWnd16, wMsg, wParam, lParam );
    return FALSE;
}

/***********************************************************************
 *           FileSaveDlgProc   (COMMDLG.7)
 */
BOOL16 CALLBACK FileSaveDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam)
{
    FIXME( "%04x %04x %04x %08Ix: stub\n", hWnd16, wMsg, wParam, lParam );
    return FALSE;
}

/***********************************************************************
 *           GetOpenFileName   (COMMDLG.1)
 *
 * Creates a dialog box for the user to select a file to open.
 *
 * RETURNS
 *    TRUE on success: user selected a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 * BUGS
 *    unknown, there are some FIXMEs left.
 */
BOOL16 WINAPI GetOpenFileName16( SEGPTR ofn ) /* [in/out] address of structure with data*/
{
    LPOPENFILENAME16 lpofn = MapSL(ofn);
    LPDLGTEMPLATEA template = NULL;
    OPENFILENAMEA ofn32;
    BOOL ret;

    if (!lpofn) return FALSE;

    ofn32.lStructSize       = OPENFILENAME_SIZE_VERSION_400A;
    ofn32.hwndOwner         = HWND_32( lpofn->hwndOwner );
    ofn32.lpstrFilter       = MapSL( lpofn->lpstrFilter );
    ofn32.lpstrCustomFilter = MapSL( lpofn->lpstrCustomFilter );
    ofn32.nMaxCustFilter    = lpofn->nMaxCustFilter;
    ofn32.nFilterIndex      = lpofn->nFilterIndex;
    ofn32.lpstrFile         = MapSL( lpofn->lpstrFile );
    ofn32.nMaxFile          = lpofn->nMaxFile;
    ofn32.lpstrFileTitle    = MapSL( lpofn->lpstrFileTitle );
    ofn32.nMaxFileTitle     = lpofn->nMaxFileTitle;
    ofn32.lpstrInitialDir   = MapSL( lpofn->lpstrInitialDir );
    ofn32.lpstrTitle        = MapSL( lpofn->lpstrTitle );
    ofn32.Flags             = (lpofn->Flags & ~OFN_ENABLETEMPLATE) | OFN_ENABLEHOOK;
    ofn32.nFileOffset       = lpofn->nFileOffset;
    ofn32.nFileExtension    = lpofn->nFileExtension;
    ofn32.lpstrDefExt       = MapSL( lpofn->lpstrDefExt );
    ofn32.lCustData         = ofn; /* See WM_INITDIALOG in the hook proc */
    ofn32.lpfnHook          = dummy_hook;  /* this is to force old 3.1 dialog style */

    if (lpofn->Flags & OFN_ENABLETEMPLATE)
    {
        HRSRC16 res = FindResource16( lpofn->hInstance, MapSL(lpofn->lpTemplateName), (LPCSTR)RT_DIALOG );
        HGLOBAL16 handle = LoadResource16( lpofn->hInstance, res );
        DWORD size = SizeofResource16( lpofn->hInstance, res );
        void *ptr = LockResource16( handle );

        if (ptr && (template = convert_dialog( ptr, size )))
        {
            ofn32.hInstance = (HINSTANCE)template;
            ofn32.Flags |= OFN_ENABLETEMPLATEHANDLE;
        }
        FreeResource16( handle );
    }

    if (lpofn->Flags & OFN_ENABLEHOOK) ofn32.lpfnHook = alloc_hook( lpofn->lpfnHook );

    if ((ret = GetOpenFileNameA( &ofn32 )))
    {
	lpofn->nFilterIndex   = ofn32.nFilterIndex;
	lpofn->nFileOffset    = ofn32.nFileOffset;
	lpofn->nFileExtension = ofn32.nFileExtension;
    }
    HeapFree( GetProcessHeap(), 0, template );
    return ret;
}

/***********************************************************************
 *           GetSaveFileName   (COMMDLG.2)
 *
 * Creates a dialog box for the user to select a file to save.
 *
 * RETURNS
 *    TRUE on success: user enters a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 * BUGS
 *    unknown. There are some FIXMEs left.
 */
BOOL16 WINAPI GetSaveFileName16( SEGPTR ofn ) /* [in/out] address of structure with data*/
{
    LPOPENFILENAME16 lpofn = MapSL(ofn);
    LPDLGTEMPLATEA template = NULL;
    OPENFILENAMEA ofn32;
    BOOL ret;

    if (!lpofn) return FALSE;

    ofn32.lStructSize       = OPENFILENAME_SIZE_VERSION_400A;
    ofn32.hwndOwner         = HWND_32( lpofn->hwndOwner );
    ofn32.lpstrFilter       = MapSL( lpofn->lpstrFilter );
    ofn32.lpstrCustomFilter = MapSL( lpofn->lpstrCustomFilter );
    ofn32.nMaxCustFilter    = lpofn->nMaxCustFilter;
    ofn32.nFilterIndex      = lpofn->nFilterIndex;
    ofn32.lpstrFile         = MapSL( lpofn->lpstrFile );
    ofn32.nMaxFile          = lpofn->nMaxFile;
    ofn32.lpstrFileTitle    = MapSL( lpofn->lpstrFileTitle );
    ofn32.nMaxFileTitle     = lpofn->nMaxFileTitle;
    ofn32.lpstrInitialDir   = MapSL( lpofn->lpstrInitialDir );
    ofn32.lpstrTitle        = MapSL( lpofn->lpstrTitle );
    ofn32.Flags             = (lpofn->Flags & ~OFN_ENABLETEMPLATE) | OFN_ENABLEHOOK;
    ofn32.nFileOffset       = lpofn->nFileOffset;
    ofn32.nFileExtension    = lpofn->nFileExtension;
    ofn32.lpstrDefExt       = MapSL( lpofn->lpstrDefExt );
    ofn32.lCustData         = ofn; /* See WM_INITDIALOG in the hook proc */
    ofn32.lpfnHook          = dummy_hook;  /* this is to force old 3.1 dialog style */

    if (lpofn->Flags & OFN_ENABLETEMPLATE)
    {
        HRSRC16 res = FindResource16( lpofn->hInstance, MapSL(lpofn->lpTemplateName), (LPCSTR)RT_DIALOG );
        HGLOBAL16 handle = LoadResource16( lpofn->hInstance, res );
        DWORD size = SizeofResource16( lpofn->hInstance, res );
        void *ptr = LockResource16( handle );

        if (ptr && (template = convert_dialog( ptr, size )))
        {
            ofn32.hInstance = (HINSTANCE)template;
            ofn32.Flags |= OFN_ENABLETEMPLATEHANDLE;
        }
        FreeResource16( handle );
    }

    if (lpofn->Flags & OFN_ENABLEHOOK) ofn32.lpfnHook = alloc_hook( lpofn->lpfnHook );

    if ((ret = GetSaveFileNameA( &ofn32 )))
    {
	lpofn->nFilterIndex   = ofn32.nFilterIndex;
	lpofn->nFileOffset    = ofn32.nFileOffset;
	lpofn->nFileExtension = ofn32.nFileExtension;
    }
    HeapFree( GetProcessHeap(), 0, template );
    return ret;
}


/***********************************************************************
 *	GetFileTitle		(COMMDLG.27)
 */
short WINAPI GetFileTitle16(LPCSTR lpFile, LPSTR lpTitle, UINT16 cbBuf)
{
	return GetFileTitleA(lpFile, lpTitle, cbBuf);
}
