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

        if (*p & 0x80)  /* window */
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

static UINT_PTR CALLBACK dummy_hook( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    return FALSE;
}

/***********************************************************************
 *           FileOpenDlgProc   (COMMDLG.6)
 */
BOOL16 CALLBACK FileOpenDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam)
{
    FIXME( "%04x %04x %04x %08lx: stub\n", hWnd16, wMsg, wParam, lParam );
    return FALSE;
}

/***********************************************************************
 *           FileSaveDlgProc   (COMMDLG.7)
 */
BOOL16 CALLBACK FileSaveDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam)
{
    FIXME( "%04x %04x %04x %08lx: stub\n", hWnd16, wMsg, wParam, lParam );
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
    ofn32.lCustData         = lpofn->lCustData;
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

    if (lpofn->Flags & OFN_ENABLEHOOK)
        FIXME( "custom hook %p no longer supported\n", lpofn->lpfnHook );

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
    ofn32.lCustData         = lpofn->lCustData;
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

    if (lpofn->Flags & OFN_ENABLEHOOK)
        FIXME( "custom hook %p no longer supported\n", lpofn->lpfnHook );

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
