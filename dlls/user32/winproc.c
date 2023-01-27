/*
 * Window procedure callbacks
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1996 Alexandre Julliard
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

#include "user_private.h"
#include "controls.h"
#include "dbt.h"
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msg);
WINE_DECLARE_DEBUG_CHANNEL(relay);

union packed_structs
{
    struct packed_CREATESTRUCTW cs;
    struct packed_DRAWITEMSTRUCT dis;
    struct packed_MEASUREITEMSTRUCT mis;
    struct packed_DELETEITEMSTRUCT dls;
    struct packed_COMPAREITEMSTRUCT cis;
    struct packed_WINDOWPOS wp;
    struct packed_COPYDATASTRUCT cds;
    struct packed_HELPINFO hi;
    struct packed_NCCALCSIZE_PARAMS ncp;
    struct packed_MSG msg;
    struct packed_MDINEXTMENU mnm;
    struct packed_MDICREATESTRUCTW mcs;
};

static inline void *get_buffer( void *static_buffer, size_t size, size_t need )
{
    if (size >= need) return static_buffer;
    return HeapAlloc( GetProcessHeap(), 0, need );
}

static inline void free_buffer( void *static_buffer, void *buffer )
{
    if (buffer != static_buffer) HeapFree( GetProcessHeap(), 0, buffer );
}

#ifdef __i386__
/* Some window procedures modify registers they shouldn't, or are not
 * properly declared stdcall; so we need a small assembly wrapper to
 * call them. */
extern LRESULT WINPROC_wrapper( WNDPROC proc, HWND hwnd, UINT msg,
                                WPARAM wParam, LPARAM lParam );
__ASM_GLOBAL_FUNC( WINPROC_wrapper,
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
                   /* TreePad X Enterprise assumes that edi is < 0x80000000 in WM_TIMER messages */
                   "xorl %edi,%edi\n\t"
                   "subl $12,%esp\n\t"
                   "pushl 24(%ebp)\n\t"
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
static inline LRESULT WINPROC_wrapper( WNDPROC proc, HWND hwnd, UINT msg,
                                       WPARAM wParam, LPARAM lParam )
{
    return proc( hwnd, msg, wParam, lParam );
}
#endif  /* __i386__ */

static WPARAM map_wparam_char_WtoA( WPARAM wParam, DWORD len )
{
    WCHAR wch = wParam;
    BYTE ch[2];
    DWORD cp = get_input_codepage();

    len = WideCharToMultiByte( cp, 0, &wch, 1, (LPSTR)ch, len, NULL, NULL );
    if (len == 2)
        return MAKEWPARAM( (ch[0] << 8) | ch[1], HIWORD(wParam) );
    else
        return MAKEWPARAM( ch[0], HIWORD(wParam) );
}

/* call a 32-bit window procedure */
static LRESULT call_window_proc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result, void *arg )
{
    WNDPROC proc = arg;

    TRACE_(relay)( "\1Call window proc %p (hwnd=%p,msg=%s,wp=%08Ix,lp=%08Ix)\n",
                   proc, hwnd, SPY_GetMsgName(msg, hwnd), wp, lp );

    *result = WINPROC_wrapper( proc, hwnd, msg, wp, lp );

    TRACE_(relay)( "\1Ret  window proc %p (hwnd=%p,msg=%s,wp=%08Ix,lp=%08Ix) retval=%08Ix\n",
                   proc, hwnd, SPY_GetMsgName(msg, hwnd), wp, lp, *result );
    return *result;
}

/* call a 32-bit dialog procedure */
static LRESULT call_dialog_proc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result, void *arg )
{
    DPI_AWARENESS_CONTEXT context;
    WNDPROC proc = arg;
    LRESULT ret;

    hwnd = WIN_GetFullHandle( hwnd );
    TRACE_(relay)( "\1Call dialog proc %p (hwnd=%p,msg=%s,wp=%08Ix,lp=%08Ix)\n",
                   proc, hwnd, SPY_GetMsgName(msg, hwnd), wp, lp );

    context = SetThreadDpiAwarenessContext( GetWindowDpiAwarenessContext( hwnd ));
    ret = WINPROC_wrapper( proc, hwnd, msg, wp, lp );
    *result = GetWindowLongPtrW( hwnd, DWLP_MSGRESULT );
    SetThreadDpiAwarenessContext( context );

    TRACE_(relay)( "\1Ret  dialog proc %p (hwnd=%p,msg=%s,wp=%08Ix,lp=%08Ix) retval=%08Ix result=%08Ix\n",
                   proc, hwnd, SPY_GetMsgName(msg, hwnd), wp, lp, ret, *result );
    return ret;
}


/**********************************************************************
 *	     WINPROC_AllocProc
 *
 * Allocate a window procedure for a window or class.
 *
 * Note that allocated winprocs are never freed; the idea is that even if an app creates a
 * lot of windows, it will usually only have a limited number of window procedures, so the
 * array won't grow too large, and this way we avoid the need to track allocations per window.
 */
static WNDPROC WINPROC_AllocProc( WNDPROC func, BOOL unicode )
{
    return (WNDPROC)NtUserCallTwoParam( (UINT_PTR)func, !unicode, NtUserAllocWinProc );
}


/**********************************************************************
 *	     WINPROC_TestLBForStr
 *
 * Return TRUE if the lparam is a string
 */
static inline BOOL WINPROC_TestLBForStr( HWND hwnd, UINT msg )
{
    DWORD style = GetWindowLongA( hwnd, GWL_STYLE );
    if (msg <= CB_MSGMAX)
        return (!(style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) || (style & CBS_HASSTRINGS));
    else
        return (!(style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) || (style & LBS_HASSTRINGS));

}


/**********************************************************************
 *	     WINPROC_CallProcAtoW
 *
 * Call a window procedure, translating args from Ansi to Unicode.
 */
LRESULT WINPROC_CallProcAtoW( winproc_callback_t callback, HWND hwnd, UINT msg, WPARAM wParam,
                              LPARAM lParam, LRESULT *result, void *arg, enum wm_char_mapping mapping )
{
    LRESULT ret = 0;

    TRACE_(msg)("(hwnd=%p,msg=%s,wp=%08Ix,lp=%08Ix)\n",
                hwnd, SPY_GetMsgName(msg, hwnd), wParam, lParam);

    switch(msg)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        {
            WCHAR *ptr, buffer[512];
            CREATESTRUCTA *csA = (CREATESTRUCTA *)lParam;
            CREATESTRUCTW csW = *(CREATESTRUCTW *)csA;
            MDICREATESTRUCTW mdi_cs;
            DWORD name_lenA = 0, name_lenW = 0, class_lenA = 0, class_lenW = 0;

            if (!IS_INTRESOURCE(csA->lpszClass))
            {
                class_lenA = strlen(csA->lpszClass) + 1;
                RtlMultiByteToUnicodeSize( &class_lenW, csA->lpszClass, class_lenA );
            }
            if (!IS_INTRESOURCE(csA->lpszName))
            {
                name_lenA = strlen(csA->lpszName) + 1;
                RtlMultiByteToUnicodeSize( &name_lenW, csA->lpszName, name_lenA );
            }

            if (!(ptr = get_buffer( buffer, sizeof(buffer), class_lenW + name_lenW ))) break;

            if (class_lenW)
            {
                csW.lpszClass = ptr;
                RtlMultiByteToUnicodeN( ptr, class_lenW, NULL, csA->lpszClass, class_lenA );
            }
            if (name_lenW)
            {
                csW.lpszName = ptr + class_lenW/sizeof(WCHAR);
                RtlMultiByteToUnicodeN( ptr + class_lenW/sizeof(WCHAR), name_lenW, NULL,
                                        csA->lpszName, name_lenA );
            }

            if (GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_MDICHILD)
            {
                mdi_cs = *(MDICREATESTRUCTW *)csA->lpCreateParams;
                mdi_cs.szTitle = csW.lpszName;
                mdi_cs.szClass = csW.lpszClass;
                csW.lpCreateParams = &mdi_cs;
            }

            ret = callback( hwnd, msg, wParam, (LPARAM)&csW, result, arg );
            free_buffer( buffer, ptr );
        }
        break;

    case WM_MDICREATE:
        {
            WCHAR *ptr, buffer[512];
            DWORD title_lenA = 0, title_lenW = 0, class_lenA = 0, class_lenW = 0;
            MDICREATESTRUCTA *csA = (MDICREATESTRUCTA *)lParam;
            MDICREATESTRUCTW csW;

            memcpy( &csW, csA, sizeof(csW) );

            if (!IS_INTRESOURCE(csA->szTitle))
            {
                title_lenA = strlen(csA->szTitle) + 1;
                RtlMultiByteToUnicodeSize( &title_lenW, csA->szTitle, title_lenA );
            }
            if (!IS_INTRESOURCE(csA->szClass))
            {
                class_lenA = strlen(csA->szClass) + 1;
                RtlMultiByteToUnicodeSize( &class_lenW, csA->szClass, class_lenA );
            }

            if (!(ptr = get_buffer( buffer, sizeof(buffer), title_lenW + class_lenW ))) break;

            if (title_lenW)
            {
                csW.szTitle = ptr;
                RtlMultiByteToUnicodeN( ptr, title_lenW, NULL, csA->szTitle, title_lenA );
            }
            if (class_lenW)
            {
                csW.szClass = ptr + title_lenW/sizeof(WCHAR);
                RtlMultiByteToUnicodeN( ptr + title_lenW/sizeof(WCHAR), class_lenW, NULL,
                                        csA->szClass, class_lenA );
            }
            ret = callback( hwnd, msg, wParam, (LPARAM)&csW, result, arg );
            free_buffer( buffer, ptr );
        }
        break;

    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        {
            WCHAR *ptr, buffer[512];
            LPSTR str = (LPSTR)lParam;
            DWORD len = wParam * sizeof(WCHAR);

            if (!(ptr = get_buffer( buffer, sizeof(buffer), len ))) break;
            ret = callback( hwnd, msg, wParam, (LPARAM)ptr, result, arg );
            if (wParam)
            {
                len = 0;
                if (*result)
                    RtlUnicodeToMultiByteN( str, wParam - 1, &len, ptr, ret * sizeof(WCHAR) );
                str[len] = 0;
                *result = len;
            }
            free_buffer( buffer, ptr );
        }
        break;

    case LB_ADDSTRING:
    case LB_INSERTSTRING:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_SELECTSTRING:
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
        if (!lParam || !WINPROC_TestLBForStr( hwnd, msg ))
        {
            ret = callback( hwnd, msg, wParam, lParam, result, arg );
            break;
        }
        /* fall through */
    case WM_SETTEXT:
    case WM_WININICHANGE:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
        if (!lParam) ret = callback( hwnd, msg, wParam, lParam, result, arg );
        else
        {
            WCHAR *ptr, buffer[512];
            LPCSTR strA = (LPCSTR)lParam;
            DWORD lenW, lenA = strlen(strA) + 1;

            RtlMultiByteToUnicodeSize( &lenW, strA, lenA );
            if ((ptr = get_buffer( buffer, sizeof(buffer), lenW )))
            {
                RtlMultiByteToUnicodeN( ptr, lenW, NULL, strA, lenA );
                ret = callback( hwnd, msg, wParam, (LPARAM)ptr, result, arg );
                free_buffer( buffer, ptr );
            }
        }
        break;

    case LB_GETTEXT:
    case CB_GETLBTEXT:
        if (lParam && WINPROC_TestLBForStr( hwnd, msg ))
        {
            WCHAR buffer[512];  /* FIXME: fixed sized buffer */

            ret = callback( hwnd, msg, wParam, (LPARAM)buffer, result, arg );
            if (*result >= 0)
            {
                DWORD len;
                RtlUnicodeToMultiByteN( (LPSTR)lParam, 512 * 3, &len,
                                        buffer, (lstrlenW(buffer) + 1) * sizeof(WCHAR) );
                *result = len - 1;
            }
        }
        else ret = callback( hwnd, msg, wParam, lParam, result, arg );
        break;

    case EM_GETLINE:
        {
            WCHAR *ptr, buffer[512];
            WORD len = *(WORD *)lParam;

            if (!(ptr = get_buffer( buffer, sizeof(buffer), len * sizeof(WCHAR) ))) break;
            *((WORD *)ptr) = len;   /* store the length */
            ret = callback( hwnd, msg, wParam, (LPARAM)ptr, result, arg );
            if (*result)
            {
                DWORD reslen;
                RtlUnicodeToMultiByteN( (LPSTR)lParam, len, &reslen, ptr, *result * sizeof(WCHAR) );
                if (reslen < len) ((LPSTR)lParam)[reslen] = 0;
                *result = reslen;
            }
            free_buffer( buffer, ptr );
        }
        break;

    case WM_GETDLGCODE:
        if (lParam)
        {
            MSG newmsg = *(MSG *)lParam;
            if (map_wparam_AtoW( newmsg.message, &newmsg.wParam, WMCHAR_MAP_NOMAPPING ))
                ret = callback( hwnd, msg, wParam, (LPARAM)&newmsg, result, arg );
        }
        else ret = callback( hwnd, msg, wParam, lParam, result, arg );
        break;

    case WM_CHARTOITEM:
    case WM_MENUCHAR:
    case WM_CHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case EM_SETPASSWORDCHAR:
    case WM_IME_CHAR:
        if (map_wparam_AtoW( msg, &wParam, mapping ))
            ret = callback( hwnd, msg, wParam, lParam, result, arg );
        break;

    case WM_GETTEXTLENGTH:
    case CB_GETLBTEXTLEN:
    case LB_GETTEXTLEN:
        ret = callback( hwnd, msg, wParam, lParam, result, arg );
        if (*result >= 0)
        {
            WCHAR *ptr, buffer[512];
            LRESULT tmp;
            DWORD len = *result + 1;
            /* Determine respective GETTEXT message */
            UINT msgGetText = (msg == WM_GETTEXTLENGTH) ? WM_GETTEXT :
                              ((msg == CB_GETLBTEXTLEN) ? CB_GETLBTEXT : LB_GETTEXT);
            /* wParam differs between the messages */
            WPARAM wp = (msg == WM_GETTEXTLENGTH) ? len : wParam;

            if (!(ptr = get_buffer( buffer, sizeof(buffer), len * sizeof(WCHAR) ))) break;

            if (callback == call_window_proc)  /* FIXME: hack */
                callback( hwnd, msgGetText, wp, (LPARAM)ptr, &tmp, arg );
            else
                tmp = SendMessageW( hwnd, msgGetText, wp, (LPARAM)ptr );
            RtlUnicodeToMultiByteSize( &len, ptr, tmp * sizeof(WCHAR) );
            *result = len;
            free_buffer( buffer, ptr );
        }
        break;

    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
        FIXME_(msg)( "message %s (0x%x) needs translation, please report\n",
                     SPY_GetMsgName(msg, hwnd), msg );
        break;

    default:
        ret = callback( hwnd, msg, wParam, lParam, result, arg );
        break;
    }
    return ret;
}


/**********************************************************************
 *	     WINPROC_CallProcWtoA
 *
 * Call a window procedure, translating args from Unicode to Ansi.
 */
static LRESULT WINPROC_CallProcWtoA( winproc_callback_t callback, HWND hwnd, UINT msg, WPARAM wParam,
                                     LPARAM lParam, LRESULT *result, void *arg )
{
    LRESULT ret = 0;

    TRACE_(msg)("(hwnd=%p,msg=%s,wp=%08Ix,lp=%08Ix)\n",
                hwnd, SPY_GetMsgName(msg, hwnd), wParam, lParam);

    switch(msg)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        {
            char buffer[1024], *cls;
            CREATESTRUCTW *csW = (CREATESTRUCTW *)lParam;
            CREATESTRUCTA csA = *(CREATESTRUCTA *)csW;
            MDICREATESTRUCTA mdi_cs;
            DWORD name_lenA = 0, name_lenW = 0, class_lenA = 0, class_lenW = 0;
            char int_name_buf[4];

            if (!IS_INTRESOURCE(csW->lpszClass))
            {
                class_lenW = (lstrlenW(csW->lpszClass) + 1) * sizeof(WCHAR);
                RtlUnicodeToMultiByteSize(&class_lenA, csW->lpszClass, class_lenW);
            }
            if (!IS_INTRESOURCE(csW->lpszName))
            {
                /* resource ID string is a special case */
                if (csW->lpszName[0] == 0xffff)
                {
                    int_name_buf[0] = 0xff;
                    int_name_buf[1] = csW->lpszName[1];
                    int_name_buf[2] = csW->lpszName[1] >> 8;
                    int_name_buf[3] = 0;
                    csA.lpszName = int_name_buf;
                }
                else
                {
                    name_lenW = (lstrlenW(csW->lpszName) + 1) * sizeof(WCHAR);
                    RtlUnicodeToMultiByteSize(&name_lenA, csW->lpszName, name_lenW);
                }
            }

            if (!(cls = get_buffer( buffer, sizeof(buffer), class_lenA + name_lenA ))) break;

            if (class_lenA)
            {
                RtlUnicodeToMultiByteN(cls, class_lenA, NULL, csW->lpszClass, class_lenW);
                csA.lpszClass = cls;
            }
            if (name_lenA)
            {
                char *name = cls + class_lenA;
                RtlUnicodeToMultiByteN(name, name_lenA, NULL, csW->lpszName, name_lenW);
                csA.lpszName = name;
            }

            if (GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_MDICHILD)
            {
                mdi_cs = *(MDICREATESTRUCTA *)csW->lpCreateParams;
                mdi_cs.szTitle = csA.lpszName;
                mdi_cs.szClass = csA.lpszClass;
                csA.lpCreateParams = &mdi_cs;
            }

            ret = callback( hwnd, msg, wParam, (LPARAM)&csA, result, arg );
            free_buffer( buffer, cls );
        }
        break;

    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        {
            char *ptr, buffer[512];
            DWORD len = wParam * 2;

            if (!(ptr = get_buffer( buffer, sizeof(buffer), len ))) break;
            ret = callback( hwnd, msg, wParam, (LPARAM)ptr, result, arg );
            if (len)
            {
                if (*result)
                {
                    RtlMultiByteToUnicodeN( (LPWSTR)lParam, wParam*sizeof(WCHAR), &len, ptr, ret + 1 );
                    *result = len/sizeof(WCHAR) - 1;  /* do not count terminating null */
                }
                ((LPWSTR)lParam)[*result] = 0;
            }
            free_buffer( buffer, ptr );
        }
        break;

    case LB_ADDSTRING:
    case LB_INSERTSTRING:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_SELECTSTRING:
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
        if (!lParam || !WINPROC_TestLBForStr( hwnd, msg ))
        {
            ret = callback( hwnd, msg, wParam, lParam, result, arg );
            break;
        }
        /* fall through */
    case WM_SETTEXT:
    case WM_WININICHANGE:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
        if (!lParam) ret = callback( hwnd, msg, wParam, lParam, result, arg );
        else
        {
            char *ptr, buffer[512];
            LPCWSTR strW = (LPCWSTR)lParam;
            DWORD lenA, lenW = (lstrlenW(strW) + 1) * sizeof(WCHAR);

            RtlUnicodeToMultiByteSize( &lenA, strW, lenW );
            if ((ptr = get_buffer( buffer, sizeof(buffer), lenA )))
            {
                RtlUnicodeToMultiByteN( ptr, lenA, NULL, strW, lenW );
                ret = callback( hwnd, msg, wParam, (LPARAM)ptr, result, arg );
                free_buffer( buffer, ptr );
            }
        }
        break;

    case WM_MDICREATE:
        {
            char *ptr, buffer[1024];
            DWORD title_lenA = 0, title_lenW = 0, class_lenA = 0, class_lenW = 0;
            MDICREATESTRUCTW *csW = (MDICREATESTRUCTW *)lParam;
            MDICREATESTRUCTA csA;

            memcpy( &csA, csW, sizeof(csA) );

            if (!IS_INTRESOURCE(csW->szTitle))
            {
                title_lenW = (lstrlenW(csW->szTitle) + 1) * sizeof(WCHAR);
                RtlUnicodeToMultiByteSize( &title_lenA, csW->szTitle, title_lenW );
            }
            if (!IS_INTRESOURCE(csW->szClass))
            {
                class_lenW = (lstrlenW(csW->szClass) + 1) * sizeof(WCHAR);
                RtlUnicodeToMultiByteSize( &class_lenA, csW->szClass, class_lenW );
            }

            if (!(ptr = get_buffer( buffer, sizeof(buffer), title_lenA + class_lenA ))) break;

            if (title_lenA)
            {
                RtlUnicodeToMultiByteN( ptr, title_lenA, NULL, csW->szTitle, title_lenW );
                csA.szTitle = ptr;
            }
            if (class_lenA)
            {
                RtlUnicodeToMultiByteN( ptr + title_lenA, class_lenA, NULL, csW->szClass, class_lenW );
                csA.szClass = ptr + title_lenA;
            }
            ret = callback( hwnd, msg, wParam, (LPARAM)&csA, result, arg );
            free_buffer( buffer, ptr );
        }
        break;

    case LB_GETTEXT:
    case CB_GETLBTEXT:
        if (lParam && WINPROC_TestLBForStr( hwnd, msg ))
        {
            char buffer[512];  /* FIXME: fixed sized buffer */

            ret = callback( hwnd, msg, wParam, (LPARAM)buffer, result, arg );
            if (*result >= 0)
            {
                DWORD len;
                RtlMultiByteToUnicodeN( (LPWSTR)lParam, 512 * 3, &len, buffer, strlen(buffer) + 1 );
                *result = len / sizeof(WCHAR) - 1;
            }
        }
        else ret = callback( hwnd, msg, wParam, lParam, result, arg );
        break;

    case EM_GETLINE:
        {
            char *ptr, buffer[512];
            WORD len = *(WORD *)lParam;

            if (!(ptr = get_buffer( buffer, sizeof(buffer), len * 2 ))) break;
            *((WORD *)ptr) = len * 2;   /* store the length */
            ret = callback( hwnd, msg, wParam, (LPARAM)ptr, result, arg );
            if (*result)
            {
                DWORD reslen;
                RtlMultiByteToUnicodeN( (LPWSTR)lParam, len*sizeof(WCHAR), &reslen, ptr, *result );
                *result = reslen / sizeof(WCHAR);
                if (*result < len) ((LPWSTR)lParam)[*result] = 0;
            }
            free_buffer( buffer, ptr );
        }
        break;

    case WM_GETDLGCODE:
        if (lParam)
        {
            MSG newmsg = *(MSG *)lParam;
            switch(newmsg.message)
            {
            case WM_CHAR:
            case WM_DEADCHAR:
            case WM_SYSCHAR:
            case WM_SYSDEADCHAR:
                newmsg.wParam = map_wparam_char_WtoA( newmsg.wParam, 1 );
                break;
            case WM_IME_CHAR:
                newmsg.wParam = map_wparam_char_WtoA( newmsg.wParam, 2 );
                break;
            }
            ret = callback( hwnd, msg, wParam, (LPARAM)&newmsg, result, arg );
        }
        else ret = callback( hwnd, msg, wParam, lParam, result, arg );
        break;

    case WM_CHAR:
        {
            WCHAR wch = wParam;
            char ch[2];
            DWORD cp = get_input_codepage();
            DWORD len = WideCharToMultiByte( cp, 0, &wch, 1, ch, 2, NULL, NULL );
            ret = callback( hwnd, msg, (BYTE)ch[0], lParam, result, arg );
            if (len == 2) ret = callback( hwnd, msg, (BYTE)ch[1], lParam, result, arg );
        }
        break;

    case WM_CHARTOITEM:
    case WM_MENUCHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case EM_SETPASSWORDCHAR:
        ret = callback( hwnd, msg, map_wparam_char_WtoA(wParam,1), lParam, result, arg );
        break;

    case WM_IME_CHAR:
        ret = callback( hwnd, msg, map_wparam_char_WtoA(wParam,2), lParam, result, arg );
        break;

    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
        FIXME_(msg)( "message %s (%04x) needs translation, please report\n",
                     SPY_GetMsgName(msg, hwnd), msg );
        break;

    default:
        ret = callback( hwnd, msg, wParam, lParam, result, arg );
        break;
    }

    return ret;
}


void dispatch_win_proc_params( struct win_proc_params *params )
{
    DPI_AWARENESS_CONTEXT context = SetThreadDpiAwarenessContext( params->dpi_awareness );

    if (!params->ansi)
    {
        if (params->procW == WINPROC_PROC16)
            WINPROC_CallProcWtoA( wow_handlers.call_window_proc, params->hwnd, params->msg, params->wparam,
                                  params->lparam, params->result, params->func );
        else if (!params->ansi_dst)
            call_window_proc( params->hwnd, params->msg, params->wparam, params->lparam,
                              params->result, params->procW );
        else
            WINPROC_CallProcWtoA( call_window_proc, params->hwnd, params->msg, params->wparam,
                                  params->lparam, params->result, params->procA );
    }
    else
    {
        if (params->procA == WINPROC_PROC16)
            wow_handlers.call_window_proc( params->hwnd, params->msg, params->wparam, params->lparam,
                                           params->result, params->func );
        else if (!params->ansi_dst)
            WINPROC_CallProcAtoW( call_window_proc, params->hwnd, params->msg, params->wparam,
                                  params->lparam, params->result, params->procW, params->mapping );
        else
            call_window_proc( params->hwnd, params->msg, params->wparam, params->lparam,
                              params->result, params->procA );
    }

    SetThreadDpiAwarenessContext( context );
}

/* make sure that there is space for 'size' bytes in buffer, growing it if needed */
static inline void *get_buffer_space( void **buffer, size_t size, size_t prev_size )
{
    if (prev_size < size)
        *buffer = HeapAlloc( GetProcessHeap(), 0, size );
    return *buffer;
}

/* check whether a combobox expects strings or ids in CB_ADDSTRING/CB_INSERTSTRING */
static inline BOOL combobox_has_strings( HWND hwnd )
{
    DWORD style = GetWindowLongA( hwnd, GWL_STYLE );
    return (!(style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) || (style & CBS_HASSTRINGS));
}

/* check whether a listbox expects strings or ids in LB_ADDSTRING/LB_INSERTSTRING */
static inline BOOL listbox_has_strings( HWND hwnd )
{
    DWORD style = GetWindowLongA( hwnd, GWL_STYLE );
    return (!(style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) || (style & LBS_HASSTRINGS));
}

/* unpack a potentially 64-bit pointer, returning 0 when truncated */
static inline void *unpack_ptr( ULONGLONG ptr64 )
{
    if ((ULONG_PTR)ptr64 != ptr64) return 0;
    return (void *)(ULONG_PTR)ptr64;
}

/* convert a server handle to a generic handle */
static inline HANDLE unpack_handle( UINT handle )
{
    return (HANDLE)(INT_PTR)(int)handle;
}

/* make sure that the buffer contains a valid null-terminated Unicode string */
static inline BOOL check_string( LPCWSTR str, size_t size )
{
    for (size /= sizeof(WCHAR); size; size--, str++)
        if (!*str) return TRUE;
    return FALSE;
}

/***********************************************************************
 *		unpack_message
 *
 * Unpack a message received from another process.
 */
static BOOL unpack_message( HWND hwnd, UINT message, WPARAM *wparam, LPARAM *lparam,
                            void **buffer, size_t size )
{
    size_t minsize = 0, prev_size = size;
    union packed_structs *ps = *buffer;

    switch(message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
    {
        CREATESTRUCTW cs;
        WCHAR *str = (WCHAR *)(&ps->cs + 1);
        if (size < sizeof(ps->cs)) return FALSE;
        size -= sizeof(ps->cs);
        cs.lpCreateParams = unpack_ptr( ps->cs.lpCreateParams );
        cs.hInstance      = unpack_ptr( ps->cs.hInstance );
        cs.hMenu          = unpack_handle( ps->cs.hMenu );
        cs.hwndParent     = unpack_handle( ps->cs.hwndParent );
        cs.cy             = ps->cs.cy;
        cs.cx             = ps->cs.cx;
        cs.y              = ps->cs.y;
        cs.x              = ps->cs.x;
        cs.style          = ps->cs.style;
        cs.dwExStyle      = ps->cs.dwExStyle;
        cs.lpszName       = unpack_ptr( ps->cs.lpszName );
        cs.lpszClass      = unpack_ptr( ps->cs.lpszClass );
        if (ps->cs.lpszName >> 16)
        {
            if (!check_string( str, size )) return FALSE;
            cs.lpszName = str;
            size -= (lstrlenW(str) + 1) * sizeof(WCHAR);
            str += lstrlenW(str) + 1;
        }
        if (ps->cs.lpszClass >> 16)
        {
            if (!check_string( str, size )) return FALSE;
            cs.lpszClass = str;
        }
        memcpy( *buffer, &cs, sizeof(cs) );
        break;
    }
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        if (!get_buffer_space( buffer, (*wparam * sizeof(WCHAR)), size )) return FALSE;
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
    {
        DRAWITEMSTRUCT dis;
        if (size < sizeof(ps->dis)) return FALSE;
        dis.CtlType    = ps->dis.CtlType;
        dis.CtlID      = ps->dis.CtlID;
        dis.itemID     = ps->dis.itemID;
        dis.itemAction = ps->dis.itemAction;
        dis.itemState  = ps->dis.itemState;
        dis.hwndItem   = unpack_handle( ps->dis.hwndItem );
        dis.hDC        = unpack_handle( ps->dis.hDC );
        dis.rcItem     = ps->dis.rcItem;
        dis.itemData   = (ULONG_PTR)unpack_ptr( ps->dis.itemData );
        memcpy( *buffer, &dis, sizeof(dis) );
        break;
    }
    case WM_MEASUREITEM:
    {
        MEASUREITEMSTRUCT mis;
        if (size < sizeof(ps->mis)) return FALSE;
        mis.CtlType    = ps->mis.CtlType;
        mis.CtlID      = ps->mis.CtlID;
        mis.itemID     = ps->mis.itemID;
        mis.itemWidth  = ps->mis.itemWidth;
        mis.itemHeight = ps->mis.itemHeight;
        mis.itemData   = (ULONG_PTR)unpack_ptr( ps->mis.itemData );
        memcpy( *buffer, &mis, sizeof(mis) );
        break;
    }
    case WM_DELETEITEM:
    {
        DELETEITEMSTRUCT dls;
        if (size < sizeof(ps->dls)) return FALSE;
        dls.CtlType    = ps->dls.CtlType;
        dls.CtlID      = ps->dls.CtlID;
        dls.itemID     = ps->dls.itemID;
        dls.hwndItem   = unpack_handle( ps->dls.hwndItem );
        dls.itemData   = (ULONG_PTR)unpack_ptr( ps->dls.itemData );
        memcpy( *buffer, &dls, sizeof(dls) );
        break;
    }
    case WM_COMPAREITEM:
    {
        COMPAREITEMSTRUCT cis;
        if (size < sizeof(ps->cis)) return FALSE;
        cis.CtlType    = ps->cis.CtlType;
        cis.CtlID      = ps->cis.CtlID;
        cis.hwndItem   = unpack_handle( ps->cis.hwndItem );
        cis.itemID1    = ps->cis.itemID1;
        cis.itemData1  = (ULONG_PTR)unpack_ptr( ps->cis.itemData1 );
        cis.itemID2    = ps->cis.itemID2;
        cis.itemData2  = (ULONG_PTR)unpack_ptr( ps->cis.itemData2 );
        cis.dwLocaleId = ps->cis.dwLocaleId;
        memcpy( *buffer, &cis, sizeof(cis) );
        break;
    }
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    {
        WINDOWPOS wp;
        if (size < sizeof(ps->wp)) return FALSE;
        wp.hwnd            = unpack_handle( ps->wp.hwnd );
        wp.hwndInsertAfter = unpack_handle( ps->wp.hwndInsertAfter );
        wp.x               = ps->wp.x;
        wp.y               = ps->wp.y;
        wp.cx              = ps->wp.cx;
        wp.cy              = ps->wp.cy;
        wp.flags           = ps->wp.flags;
        memcpy( *buffer, &wp, sizeof(wp) );
        break;
    }
    case WM_COPYDATA:
    {
        COPYDATASTRUCT cds;
        if (size < sizeof(ps->cds)) return FALSE;
        cds.dwData = (ULONG_PTR)unpack_ptr( ps->cds.dwData );
        if (ps->cds.lpData)
        {
            cds.cbData = ps->cds.cbData;
            cds.lpData = &ps->cds + 1;
            minsize = sizeof(ps->cds) + cds.cbData;
        }
        else
        {
            cds.cbData = 0;
            cds.lpData = 0;
        }
        memcpy( &ps->cds, &cds, sizeof(cds) );
        break;
    }
    case WM_NOTIFY:
        /* WM_NOTIFY cannot be sent across processes (MSDN) */
        return FALSE;
    case WM_HELP:
    {
        HELPINFO hi;
        if (size < sizeof(ps->hi)) return FALSE;
        hi.cbSize       = sizeof(hi);
        hi.iContextType = ps->hi.iContextType;
        hi.iCtrlId      = ps->hi.iCtrlId;
        hi.hItemHandle  = unpack_handle( ps->hi.hItemHandle );
        hi.dwContextId  = (ULONG_PTR)unpack_ptr( ps->hi.dwContextId );
        hi.MousePos     = ps->hi.MousePos;
        memcpy( &ps->hi, &hi, sizeof(hi) );
        break;
    }
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
        minsize = sizeof(STYLESTRUCT);
        break;
    case WM_NCCALCSIZE:
        if (!*wparam) minsize = sizeof(RECT);
        else
        {
            NCCALCSIZE_PARAMS ncp;
            WINDOWPOS wp;
            if (size < sizeof(ps->ncp)) return FALSE;
            ncp.rgrc[0]        = ps->ncp.rgrc[0];
            ncp.rgrc[1]        = ps->ncp.rgrc[1];
            ncp.rgrc[2]        = ps->ncp.rgrc[2];
            wp.hwnd            = unpack_handle( ps->ncp.hwnd );
            wp.hwndInsertAfter = unpack_handle( ps->ncp.hwndInsertAfter );
            wp.x               = ps->ncp.x;
            wp.y               = ps->ncp.y;
            wp.cx              = ps->ncp.cx;
            wp.cy              = ps->ncp.cy;
            wp.flags           = ps->ncp.flags;
            ncp.lppos = (WINDOWPOS *)((NCCALCSIZE_PARAMS *)&ps->ncp + 1);
            memcpy( &ps->ncp, &ncp, sizeof(ncp) );
            *ncp.lppos = wp;
        }
        break;
    case WM_GETDLGCODE:
        if (*lparam)
        {
            MSG msg;
            if (size < sizeof(ps->msg)) return FALSE;
            msg.hwnd    = unpack_handle( ps->msg.hwnd );
            msg.message = ps->msg.message;
            msg.wParam  = (ULONG_PTR)unpack_ptr( ps->msg.wParam );
            msg.lParam  = (ULONG_PTR)unpack_ptr( ps->msg.lParam );
            msg.time    = ps->msg.time;
            msg.pt      = ps->msg.pt;
            memcpy( &ps->msg, &msg, sizeof(msg) );
            break;
        }
        return TRUE;
    case SBM_SETSCROLLINFO:
        minsize = sizeof(SCROLLINFO);
        break;
    case SBM_GETSCROLLINFO:
        if (!get_buffer_space( buffer, sizeof(SCROLLINFO), size )) return FALSE;
        break;
    case SBM_GETSCROLLBARINFO:
        if (!get_buffer_space( buffer, sizeof(SCROLLBARINFO), size )) return FALSE;
        break;
    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
        if (*wparam || *lparam)
        {
            if (!get_buffer_space( buffer, 2*sizeof(DWORD), size )) return FALSE;
            if (*wparam) *wparam = (WPARAM)*buffer;
            if (*lparam) *lparam = (LPARAM)((DWORD *)*buffer + 1);
        }
        return TRUE;
    case EM_GETRECT:
    case LB_GETITEMRECT:
    case CB_GETDROPPEDCONTROLRECT:
        if (!get_buffer_space( buffer, sizeof(RECT), size )) return FALSE;
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
        if (!get_buffer_space( buffer, (len + 1) * sizeof(WCHAR), size )) return FALSE;
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
        if (!get_buffer_space( buffer, size, prev_size )) return FALSE;
        break;
    }
    case LB_GETTEXT:
    {
        size = sizeof(ULONG_PTR);
        if (listbox_has_strings( hwnd ))
            size = (SendMessageW( hwnd, LB_GETTEXTLEN, *wparam, 0 ) + 1) * sizeof(WCHAR);
        if (!get_buffer_space( buffer, size, prev_size )) return FALSE;
        break;
    }
    case LB_GETSELITEMS:
        if (!get_buffer_space( buffer, *wparam * sizeof(UINT), size )) return FALSE;
        break;
    case WM_NEXTMENU:
    {
        MDINEXTMENU mnm;
        if (size < sizeof(ps->mnm)) return FALSE;
        mnm.hmenuIn   = unpack_handle( ps->mnm.hmenuIn );
        mnm.hmenuNext = unpack_handle( ps->mnm.hmenuNext );
        mnm.hwndNext  = unpack_handle( ps->mnm.hwndNext );
        memcpy( *buffer, &mnm, sizeof(mnm) );
        break;
    }
    case WM_SIZING:
    case WM_MOVING:
        minsize = sizeof(RECT);
        if (!get_buffer_space( buffer, sizeof(RECT), size )) return FALSE;
        break;
    case WM_MDICREATE:
    {
        MDICREATESTRUCTW mcs;
        WCHAR *str = (WCHAR *)(&ps->mcs + 1);
        if (size < sizeof(ps->mcs)) return FALSE;
        size -= sizeof(ps->mcs);

        mcs.szClass = unpack_ptr( ps->mcs.szClass );
        mcs.szTitle = unpack_ptr( ps->mcs.szTitle );
        mcs.hOwner  = unpack_ptr( ps->mcs.hOwner );
        mcs.x       = ps->mcs.x;
        mcs.y       = ps->mcs.y;
        mcs.cx      = ps->mcs.cx;
        mcs.cy      = ps->mcs.cy;
        mcs.style   = ps->mcs.style;
        mcs.lParam  = (LPARAM)unpack_ptr( ps->mcs.lParam );
        if (ps->mcs.szClass >> 16)
        {
            if (!check_string( str, size )) return FALSE;
            mcs.szClass = str;
            size -= (lstrlenW(str) + 1) * sizeof(WCHAR);
            str += lstrlenW(str) + 1;
        }
        if (ps->mcs.szTitle >> 16)
        {
            if (!check_string( str, size )) return FALSE;
            mcs.szTitle = str;
        }
        memcpy( *buffer, &mcs, sizeof(mcs) );
        break;
    }
    case WM_MDIGETACTIVE:
        if (!*lparam) return TRUE;
        if (!get_buffer_space( buffer, sizeof(BOOL), size )) return FALSE;
        break;
    case WM_DEVICECHANGE:
        if (!(*wparam & 0x8000)) return TRUE;
        minsize = sizeof(DEV_BROADCAST_HDR);
        break;
    case WM_NCPAINT:
        if (*wparam <= 1) return TRUE;
        FIXME( "WM_NCPAINT hdc unpacking not supported\n" );
        return FALSE;
    case WM_PAINT:
        if (!*wparam) return TRUE;
        /* fall through */

    /* these contain an HFONT */
    case WM_SETFONT:
    case WM_GETFONT:
    /* these contain an HDC */
    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
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
    /* these contain HICON */
    case WM_GETICON:
    case WM_SETICON:
    case WM_QUERYDRAGICON:
    case WM_QUERYPARKICON:
    /* these contain pointers */
    case WM_DROPOBJECT:
    case WM_QUERYDROPOBJECT:
    case WM_DRAGLOOP:
    case WM_DRAGSELECT:
    case WM_DRAGMOVE:
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

BOOL WINAPI User32CallWindowProc( struct win_proc_params *params, ULONG size )
{
    LRESULT result, *result_ptr = params->result;
    params->result = &result;

    if (params->needs_unpack)
    {
        char stack_buffer[128];
        void *buffer;

        if (size > sizeof(*params))
        {
            size -= sizeof(*params);
            buffer = params + 1;
        }
        else
        {
            size = sizeof(stack_buffer);
            buffer = stack_buffer;
        }
        if (!unpack_message( params->hwnd, params->msg, &params->wparam,
                             &params->lparam, &buffer, size ))
            return 0;

        dispatch_win_proc_params( params );

        NtUserMessageCall( params->hwnd, params->msg, params->wparam, params->lparam,
                           (void *)result, NtUserWinProcResult, FALSE );
        if (buffer != stack_buffer && buffer != params + 1)
            HeapFree( GetProcessHeap(), 0, buffer );
    }
    else
    {
        dispatch_win_proc_params( params );
        if (result_ptr)
        {
            *result_ptr = result;
            return TRUE;
        }
        NtCallbackReturn( &result, sizeof(result), TRUE );
    }
    return TRUE;
}

BOOL WINAPI User32CallSendAsyncCallback( const struct send_async_params *params, ULONG size )
{
    params->callback( params->hwnd, params->msg, params->data, params->result );
    return TRUE;
}

/**********************************************************************
 *		CallWindowProcA (USER32.@)
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
LRESULT WINAPI CallWindowProcA( WNDPROC func, HWND hwnd, UINT msg, WPARAM wParam,  LPARAM lParam )
{
    struct win_proc_params params;
    LRESULT result;

    params.func = func;
    params.result = &result;
    if (!NtUserMessageCall( hwnd, msg, wParam, lParam, &params, NtUserCallWindowProc, TRUE ))
        return 0;
    dispatch_win_proc_params( &params );
    return result;
}


/**********************************************************************
 *		CallWindowProcW (USER32.@)
 *
 * See CallWindowProcA.
 */
LRESULT WINAPI CallWindowProcW( WNDPROC func, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    struct win_proc_params params;
    LRESULT result;

    params.func = func;
    params.result = &result;
    if (!NtUserMessageCall( hwnd, msg, wParam, lParam, &params, NtUserCallWindowProc, FALSE ))
        return 0;
    dispatch_win_proc_params( &params );
    return result;
}


/**********************************************************************
 *		WINPROC_CallDlgProcA
 */
INT_PTR WINPROC_CallDlgProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    DLGPROC func, proc;
    LRESULT result;

#ifdef _WIN64
    proc = (DLGPROC)NtUserGetWindowLongPtrA( hwnd, DWLP_DLGPROC );
#else
    proc = (DLGPROC)NtUserGetWindowLongA( hwnd, DWLP_DLGPROC );
#endif
    if (!proc) return 0;
    if (!(func = NtUserGetDialogProc( proc, TRUE )) &&
        !(func = NtUserGetDialogProc( proc, FALSE ))) return 0;

    if (func == WINPROC_PROC16)
    {
        INT_PTR ret;
        ret = wow_handlers.call_dialog_proc( hwnd, msg, wParam, lParam, &result, proc );
        SetWindowLongPtrW( hwnd, DWLP_MSGRESULT, result );
        return ret;
    }

    return call_dialog_proc( hwnd, msg, wParam, lParam, &result, func );
}


/**********************************************************************
 *		WINPROC_CallDlgProcW
 */
INT_PTR WINPROC_CallDlgProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    DLGPROC func, proc;
    LRESULT result;

#ifdef _WIN64
    proc = (DLGPROC)NtUserGetWindowLongPtrW( hwnd, DWLP_DLGPROC );
#else
    proc = (DLGPROC)NtUserGetWindowLongW( hwnd, DWLP_DLGPROC );
#endif
    if (!proc) return 0;
    if (!(func = NtUserGetDialogProc( proc, FALSE )) &&
        !(func = NtUserGetDialogProc( proc, TRUE ))) return 0;

    if (func == WINPROC_PROC16)
    {
        INT_PTR ret;
        ret = WINPROC_CallProcWtoA( wow_handlers.call_dialog_proc,
                                    hwnd, msg, wParam, lParam, &result, proc );
        SetWindowLongPtrW( hwnd, DWLP_MSGRESULT, result );
        return ret;
    }

    return call_dialog_proc( hwnd, msg, wParam, lParam, &result, func );
}


/***********************************************************************
 * Window procedures for builtin classes
 */

static LRESULT WINAPI ButtonWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.button_proc( hwnd, msg, wParam, lParam, FALSE );
}

static LRESULT WINAPI ButtonWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.button_proc( hwnd, msg, wParam, lParam, TRUE );
}

static LRESULT WINAPI ComboWndProcA( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.combo_proc( hwnd, message, wParam, lParam, FALSE );
}

static LRESULT WINAPI ComboWndProcW( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.combo_proc( hwnd, message, wParam, lParam, TRUE );
}

LRESULT WINAPI EditWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.edit_proc( hwnd, msg, wParam, lParam, FALSE );
}

static LRESULT WINAPI EditWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.edit_proc( hwnd, msg, wParam, lParam, TRUE );
}

static LRESULT WINAPI ListBoxWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.listbox_proc( hwnd, msg, wParam, lParam, FALSE );
}

static LRESULT WINAPI ListBoxWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.listbox_proc( hwnd, msg, wParam, lParam, TRUE );
}

static LRESULT WINAPI MDIClientWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.mdiclient_proc( hwnd, msg, wParam, lParam, FALSE );
}

static LRESULT WINAPI MDIClientWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.mdiclient_proc( hwnd, msg, wParam, lParam, TRUE );
}

static LRESULT WINAPI ScrollBarWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.scrollbar_proc( hwnd, msg, wParam, lParam, FALSE );
}

static LRESULT WINAPI ScrollBarWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.scrollbar_proc( hwnd, msg, wParam, lParam, TRUE );
}

static LRESULT WINAPI StaticWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.static_proc( hwnd, msg, wParam, lParam, FALSE );
}

static LRESULT WINAPI StaticWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.static_proc( hwnd, msg, wParam, lParam, TRUE );
}

/**********************************************************************
 *		UserRegisterWowHandlers (USER32.@)
 *
 * NOTE: no attempt has been made to be compatible here,
 * the Windows function is most likely completely different.
 */
void WINAPI UserRegisterWowHandlers( const struct wow_handlers16 *new, struct wow_handlers32 *orig )
{
    orig->button_proc     = ButtonWndProc_common;
    orig->combo_proc      = ComboWndProc_common;
    orig->edit_proc       = EditWndProc_common;
    orig->listbox_proc    = ListBoxWndProc_common;
    orig->mdiclient_proc  = MDIClientWndProc_common;
    orig->scrollbar_proc  = ScrollBarWndProc_common;
    orig->static_proc     = StaticWndProc_common;
    orig->create_window   = WIN_CreateWindowEx;
    orig->get_win_handle  = WIN_GetFullHandle;
    orig->alloc_winproc   = WINPROC_AllocProc;
    orig->get_dialog_info = DIALOG_get_info;
    orig->dialog_box_loop = DIALOG_DoDialogBox;

    wow_handlers = *new;
}

struct wow_handlers16 wow_handlers =
{
    ButtonWndProc_common,
    ComboWndProc_common,
    EditWndProc_common,
    ListBoxWndProc_common,
    MDIClientWndProc_common,
    ScrollBarWndProc_common,
    StaticWndProc_common,
    WIN_CreateWindowEx,
    NULL,  /* call_window_proc */
    NULL,  /* call_dialog_proc */
};

static const struct user_client_procs client_procsA =
{
    .pButtonWndProc = ButtonWndProcA,
    .pComboWndProc = ComboWndProcA,
    .pDefWindowProc = DefWindowProcA,
    .pDefDlgProc = DefDlgProcA,
    .pEditWndProc = EditWndProcA,
    .pListBoxWndProc = ListBoxWndProcA,
    .pMDIClientWndProc = MDIClientWndProcA,
    .pScrollBarWndProc = ScrollBarWndProcA,
    .pStaticWndProc = StaticWndProcA,
    .pImeWndProc = ImeWndProcA,
};

static const struct user_client_procs client_procsW =
{
    .pButtonWndProc = ButtonWndProcW,
    .pComboWndProc = ComboWndProcW,
    .pDefWindowProc = DefWindowProcW,
    .pDefDlgProc = DefDlgProcW,
    .pEditWndProc = EditWndProcW,
    .pListBoxWndProc = ListBoxWndProcW,
    .pMDIClientWndProc = MDIClientWndProcW,
    .pScrollBarWndProc = ScrollBarWndProcW,
    .pStaticWndProc = StaticWndProcW,
    .pImeWndProc = ImeWndProcW,
    .pDesktopWndProc = DesktopWndProc,
    .pIconTitleWndProc = IconTitleWndProc,
    .pPopupMenuWndProc = PopupMenuWndProc,
    .pMessageWndProc = MessageWndProc,
};

void winproc_init(void)
{
    NtUserInitializeClientPfnArrays( &client_procsA, &client_procsW, NULL, user32_module );
}
