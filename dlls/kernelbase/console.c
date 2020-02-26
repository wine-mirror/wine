/*
 * Win32 console functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 * Copyright 1997 Karl Garrison
 * Copyright 1998 John Richardson
 * Copyright 1998 Marcus Meissner
 * Copyright 2001,2002,2004,2005,2010 Eric Pouech
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define NONAMELESSUNION
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "wincon.h"
#include "wine/server.h"
#include "wine/exception.h"
#include "wine/debug.h"
#include "kernelbase.h"

WINE_DEFAULT_DEBUG_CHANNEL(console);


static CRITICAL_SECTION console_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &console_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": console_section") }
};
static CRITICAL_SECTION console_section = { &critsect_debug, -1, 0, 0, 0, 0 };

static WCHAR input_exe[MAX_PATH + 1];


/* map a kernel32 console handle onto a real wineserver handle */
static inline obj_handle_t console_handle_unmap( HANDLE h )
{
    return wine_server_obj_handle( console_handle_map( h ) );
}

/* map input records to ASCII */
static void input_records_WtoA( INPUT_RECORD *buffer, int count )
{
    UINT cp = GetConsoleCP();
    int i;
    char ch;

    for (i = 0; i < count; i++)
    {
        if (buffer[i].EventType != KEY_EVENT) continue;
        WideCharToMultiByte( cp, 0, &buffer[i].Event.KeyEvent.uChar.UnicodeChar, 1, &ch, 1, NULL, NULL );
        buffer[i].Event.KeyEvent.uChar.AsciiChar = ch;
    }
}

/* map input records to Unicode */
static void input_records_AtoW( INPUT_RECORD *buffer, int count )
{
    UINT cp = GetConsoleCP();
    int i;
    WCHAR ch;

    for (i = 0; i < count; i++)
    {
        if (buffer[i].EventType != KEY_EVENT) continue;
        MultiByteToWideChar( cp, 0, &buffer[i].Event.KeyEvent.uChar.AsciiChar, 1, &ch, 1 );
        buffer[i].Event.KeyEvent.uChar.UnicodeChar = ch;
    }
}

/* map char infos to ASCII */
static void char_info_WtoA( CHAR_INFO *buffer, int count )
{
    UINT cp = GetConsoleOutputCP();
    char ch;

    while (count-- > 0)
    {
        WideCharToMultiByte( cp, 0, &buffer->Char.UnicodeChar, 1, &ch, 1, NULL, NULL );
        buffer->Char.AsciiChar = ch;
        buffer++;
    }
}

/* map char infos to Unicode */
static void char_info_AtoW( CHAR_INFO *buffer, int count )
{
    UINT cp = GetConsoleOutputCP();
    WCHAR ch;

    while (count-- > 0)
    {
        MultiByteToWideChar( cp, 0, &buffer->Char.AsciiChar, 1, &ch, 1 );
        buffer->Char.UnicodeChar = ch;
        buffer++;
    }
}

/* helper function for ScrollConsoleScreenBufferW */
static void fill_console_output( HANDLE handle, int i, int j, int len, CHAR_INFO *fill )
{
    SERVER_START_REQ( fill_console_output )
    {
        req->handle    = console_handle_unmap( handle );
        req->mode      = CHAR_INFO_MODE_TEXTATTR;
        req->x         = i;
        req->y         = j;
        req->count     = len;
        req->wrap      = FALSE;
        req->data.ch   = fill->Char.UnicodeChar;
        req->data.attr = fill->Attributes;
        wine_server_call_err( req );
    }
    SERVER_END_REQ;
}

/* helper function for GetLargestConsoleWindowSize */
static COORD get_largest_console_window_size( HANDLE handle )
{
    COORD c = { 0, 0 };

    SERVER_START_REQ( get_console_output_info )
    {
        req->handle = console_handle_unmap( handle );
        if (!wine_server_call_err( req ))
        {
            c.X = reply->max_width;
            c.Y = reply->max_height;
        }
    }
    SERVER_END_REQ;
    TRACE( "(%p), returning %dx%d\n", handle, c.X, c.Y );
    return c;
}

/* helper function to replace OpenConsoleW */
HANDLE open_console( BOOL output, DWORD access, SECURITY_ATTRIBUTES *sa, DWORD creation )
{
    HANDLE ret;

    if (creation != OPEN_EXISTING)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }

    SERVER_START_REQ( open_console )
    {
        req->from       = wine_server_obj_handle( ULongToHandle( output ));
        req->access     = access;
        req->attributes = sa && sa->bInheritHandle ? OBJ_INHERIT : 0;
        req->share      = FILE_SHARE_READ | FILE_SHARE_WRITE;
        wine_server_call_err( req );
        ret = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    if (ret) ret = console_handle_map( ret );
    return ret;
}


/******************************************************************
 *	AttachConsole   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH AttachConsole( DWORD pid )
{
    BOOL ret;

    TRACE( "(%x)\n", pid );

    SERVER_START_REQ( attach_console )
    {
        req->pid = pid;
        if ((ret = !wine_server_call_err( req )))
        {
            SetStdHandle( STD_INPUT_HANDLE,  wine_server_ptr_handle( reply->std_in ));
            SetStdHandle( STD_OUTPUT_HANDLE, wine_server_ptr_handle( reply->std_out ));
            SetStdHandle( STD_ERROR_HANDLE,  wine_server_ptr_handle( reply->std_err ));
        }
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *	CreateConsoleScreenBuffer   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateConsoleScreenBuffer( DWORD access, DWORD share,
                                                           SECURITY_ATTRIBUTES *sa, DWORD flags,
                                                           void *data )
{
    HANDLE ret = INVALID_HANDLE_VALUE;

    TRACE( "(%x,%x,%p,%x,%p)\n", access, share, sa, flags, data );

    if (flags != CONSOLE_TEXTMODE_BUFFER || data)
    {
	SetLastError( ERROR_INVALID_PARAMETER );
	return INVALID_HANDLE_VALUE;
    }

    SERVER_START_REQ( create_console_output )
    {
        req->handle_in  = 0;
        req->access     = access;
        req->attributes = (sa && sa->bInheritHandle) ? OBJ_INHERIT : 0;
        req->share      = share;
        req->fd         = -1;
        if (!wine_server_call_err( req ))
            ret = console_handle_map( wine_server_ptr_handle( reply->handle_out ));
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *	FillConsoleOutputAttribute   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FillConsoleOutputAttribute( HANDLE handle, WORD attr, DWORD length,
                                                           COORD coord, DWORD *written )
{
    BOOL ret;

    TRACE( "(%p,%d,%d,(%dx%d),%p)\n", handle, attr, length, coord.X, coord.Y, written );

    if (!written)
    {
        SetLastError( ERROR_INVALID_ACCESS );
        return FALSE;
    }

    *written = 0;
    SERVER_START_REQ( fill_console_output )
    {
        req->handle    = console_handle_unmap( handle );
        req->x         = coord.X;
        req->y         = coord.Y;
        req->mode      = CHAR_INFO_MODE_ATTR;
        req->wrap      = TRUE;
        req->data.attr = attr;
        req->count     = length;
        if ((ret = !wine_server_call_err( req ))) *written = reply->written;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *	FillConsoleOutputCharacterA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FillConsoleOutputCharacterA( HANDLE handle, CHAR ch, DWORD length,
                                                           COORD coord, DWORD *written )
{
    WCHAR wch;

    MultiByteToWideChar( GetConsoleOutputCP(), 0, &ch, 1, &wch, 1 );
    return FillConsoleOutputCharacterW( handle, wch, length, coord, written );
}


/******************************************************************************
 *	FillConsoleOutputCharacterW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FillConsoleOutputCharacterW( HANDLE handle, WCHAR ch, DWORD length,
                                                           COORD coord, DWORD *written )
{
    BOOL ret;

    TRACE( "(%p,%s,%d,(%dx%d),%p)\n", handle, debugstr_wn(&ch, 1), length, coord.X, coord.Y, written );

    if (!written)
    {
        SetLastError( ERROR_INVALID_ACCESS );
        return FALSE;
    }

    *written = 0;
    SERVER_START_REQ( fill_console_output )
    {
        req->handle  = console_handle_unmap( handle );
        req->x       = coord.X;
        req->y       = coord.Y;
        req->mode    = CHAR_INFO_MODE_TEXT;
        req->wrap    = TRUE;
        req->data.ch = ch;
        req->count   = length;
        if ((ret = !wine_server_call_err( req ))) *written = reply->written;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *	FreeConsole   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FreeConsole(void)
{
    BOOL ret;

    SERVER_START_REQ( free_console )
    {
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *	GenerateConsoleCtrlEvent   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GenerateConsoleCtrlEvent( DWORD event, DWORD group )
{
    BOOL ret;

    TRACE( "(%d, %x)\n", event, group );

    if (event != CTRL_C_EVENT && event != CTRL_BREAK_EVENT)
    {
	ERR( "Invalid event %d for PGID %x\n", event, group );
	return FALSE;
    }

    SERVER_START_REQ( send_console_signal )
    {
        req->signal = event;
        req->group_id = group;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *	GetConsoleCP   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetConsoleCP(void)
{
    UINT codepage = GetOEMCP(); /* default value */

    SERVER_START_REQ( get_console_input_info )
    {
        req->handle = 0;
        if (!wine_server_call_err( req ))
        {
            if (reply->input_cp) codepage = reply->input_cp;
        }
    }
    SERVER_END_REQ;
    return codepage;
}


/******************************************************************************
 *	GetConsoleCursorInfo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetConsoleCursorInfo( HANDLE handle, CONSOLE_CURSOR_INFO *info )
{
    BOOL ret;

    SERVER_START_REQ( get_console_output_info )
    {
        req->handle = console_handle_unmap( handle );
        ret = !wine_server_call_err( req );
        if (ret && info)
        {
            info->dwSize = reply->cursor_size;
            info->bVisible = reply->cursor_visible;
        }
    }
    SERVER_END_REQ;

    if (!ret) return FALSE;
    if (!info)
    {
        SetLastError( ERROR_INVALID_ACCESS );
        return FALSE;
    }
    TRACE("(%p) returning (%d,%d)\n", handle, info->dwSize, info->bVisible);
    return TRUE;
}


/***********************************************************************
 *	GetConsoleInputExeNameA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetConsoleInputExeNameA( DWORD len, LPSTR buffer )
{
    RtlEnterCriticalSection( &console_section );
    if (WideCharToMultiByte( CP_ACP, 0, input_exe, -1, NULL, 0, NULL, NULL ) <= len)
        WideCharToMultiByte( CP_ACP, 0, input_exe, -1, buffer, len, NULL, NULL );
    else SetLastError(ERROR_BUFFER_OVERFLOW);
    RtlLeaveCriticalSection( &console_section );
    return TRUE;
}


/***********************************************************************
 *	GetConsoleInputExeNameW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetConsoleInputExeNameW( DWORD len, LPWSTR buffer )
{
    RtlEnterCriticalSection( &console_section );
    if (len > lstrlenW(input_exe)) lstrcpyW( buffer, input_exe );
    else SetLastError( ERROR_BUFFER_OVERFLOW );
    RtlLeaveCriticalSection( &console_section );
    return TRUE;
}


/***********************************************************************
 *	GetConsoleMode   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetConsoleMode( HANDLE handle, DWORD *mode )
{
    BOOL ret;

    SERVER_START_REQ( get_console_mode )
    {
        req->handle = console_handle_unmap( handle );
        if ((ret = !wine_server_call_err( req )))
        {
            if (mode) *mode = reply->mode;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *	GetConsoleOutputCP   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetConsoleOutputCP(void)
{
    UINT codepage = GetOEMCP(); /* default value */

    SERVER_START_REQ( get_console_input_info )
    {
        req->handle = 0;
        if (!wine_server_call_err( req ))
        {
            if (reply->output_cp) codepage = reply->output_cp;
        }
    }
    SERVER_END_REQ;
    return codepage;
}


/***********************************************************************
 *	GetConsoleScreenBufferInfo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetConsoleScreenBufferInfo( HANDLE handle, CONSOLE_SCREEN_BUFFER_INFO *info )
{
    BOOL ret;

    SERVER_START_REQ( get_console_output_info )
    {
        req->handle = console_handle_unmap( handle );
        if ((ret = !wine_server_call_err( req )))
        {
            info->dwSize.X              = reply->width;
            info->dwSize.Y              = reply->height;
            info->dwCursorPosition.X    = reply->cursor_x;
            info->dwCursorPosition.Y    = reply->cursor_y;
            info->wAttributes           = reply->attr;
            info->srWindow.Left         = reply->win_left;
            info->srWindow.Right        = reply->win_right;
            info->srWindow.Top          = reply->win_top;
            info->srWindow.Bottom       = reply->win_bottom;
            info->dwMaximumWindowSize.X = min(reply->width, reply->max_width);
            info->dwMaximumWindowSize.Y = min(reply->height, reply->max_height);
        }
    }
    SERVER_END_REQ;

    TRACE( "(%p,(%d,%d) (%d,%d) %d (%d,%d-%d,%d) (%d,%d)\n", handle,
           info->dwSize.X, info->dwSize.Y, info->dwCursorPosition.X, info->dwCursorPosition.Y,
	  info->wAttributes, info->srWindow.Left, info->srWindow.Top, info->srWindow.Right,
           info->srWindow.Bottom, info->dwMaximumWindowSize.X, info->dwMaximumWindowSize.Y );
    return ret;
}


/***********************************************************************
 *	GetConsoleScreenBufferInfoEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetConsoleScreenBufferInfoEx( HANDLE handle,
                                                            CONSOLE_SCREEN_BUFFER_INFOEX *info )
{
    BOOL ret;

    if (info->cbSize != sizeof(CONSOLE_SCREEN_BUFFER_INFOEX))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    SERVER_START_REQ( get_console_output_info )
    {
        req->handle = console_handle_unmap( handle );
        wine_server_set_reply( req, info->ColorTable, sizeof(info->ColorTable) );
        if ((ret = !wine_server_call_err( req )))
        {
            info->dwSize.X              = reply->width;
            info->dwSize.Y              = reply->height;
            info->dwCursorPosition.X    = reply->cursor_x;
            info->dwCursorPosition.Y    = reply->cursor_y;
            info->wAttributes           = reply->attr;
            info->srWindow.Left         = reply->win_left;
            info->srWindow.Top          = reply->win_top;
            info->srWindow.Right        = reply->win_right;
            info->srWindow.Bottom       = reply->win_bottom;
            info->dwMaximumWindowSize.X = min( reply->width, reply->max_width );
            info->dwMaximumWindowSize.Y = min( reply->height, reply->max_height );
            info->wPopupAttributes      = reply->popup_attr;
            info->bFullscreenSupported  = FALSE;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *	GetConsoleTitleW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetConsoleTitleW( LPWSTR title, DWORD size )
{
    DWORD ret = 0;

    SERVER_START_REQ( get_console_input_info )
    {
        req->handle = 0;
        wine_server_set_reply( req, title, (size - 1) * sizeof(WCHAR) );
        if (!wine_server_call_err( req ))
        {
            ret = wine_server_reply_size(reply) / sizeof(WCHAR);
            title[ret] = 0;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *	GetLargestConsoleWindowSize   (kernelbase.@)
 */
#if defined(__i386__) && !defined(__MINGW32__)
#undef GetLargestConsoleWindowSize
DWORD WINAPI DECLSPEC_HOTPATCH GetLargestConsoleWindowSize( HANDLE handle )
{
    union {
	COORD c;
	DWORD w;
    } x;
    x.c = get_largest_console_window_size( handle );
    return x.w;
}

#else

COORD WINAPI DECLSPEC_HOTPATCH GetLargestConsoleWindowSize( HANDLE handle )
{
    return get_largest_console_window_size( handle );
}

#endif /* !defined(__i386__) */


/***********************************************************************
 *	GetNumberOfConsoleInputEvents   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetNumberOfConsoleInputEvents( HANDLE handle, DWORD *count )
{
    BOOL ret;

    SERVER_START_REQ( read_console_input )
    {
        req->handle = console_handle_unmap( handle );
        req->flush  = FALSE;
        if ((ret = !wine_server_call_err( req )))
        {
            if (count) *count = reply->read;
            else
            {
                SetLastError( ERROR_INVALID_ACCESS );
                ret = FALSE;
            }
        }
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *	PeekConsoleInputA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH PeekConsoleInputA( HANDLE handle, INPUT_RECORD *buffer,
                                                 DWORD length, DWORD *count )
{
    DWORD read;

    if (!PeekConsoleInputW( handle, buffer, length, &read )) return FALSE;
    input_records_WtoA( buffer, read );
    if (count) *count = read;
    return TRUE;
}


/***********************************************************************
 *	PeekConsoleInputW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH PeekConsoleInputW( HANDLE handle, INPUT_RECORD *buffer,
                                                 DWORD length, DWORD *count )
{
    BOOL ret;

    SERVER_START_REQ( read_console_input )
    {
        req->handle = console_handle_unmap( handle );
        req->flush  = FALSE;
        wine_server_set_reply( req, buffer, length * sizeof(INPUT_RECORD) );
        if ((ret = !wine_server_call_err( req )))
        {
            if (count) *count = length ? reply->read : 0;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *	ReadConsoleOutputAttribute   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReadConsoleOutputAttribute( HANDLE handle, WORD *attr, DWORD length,
                                                          COORD coord, DWORD *count )
{
    BOOL ret;

    TRACE( "(%p,%p,%d,%dx%d,%p)\n", handle, attr, length, coord.X, coord.Y, count );

    if (!count)
    {
        SetLastError( ERROR_INVALID_ACCESS );
        return FALSE;
    }

    *count = 0;
    SERVER_START_REQ( read_console_output )
    {
        req->handle = console_handle_unmap( handle );
        req->x      = coord.X;
        req->y      = coord.Y;
        req->mode   = CHAR_INFO_MODE_ATTR;
        req->wrap   = TRUE;
        wine_server_set_reply( req, attr, length * sizeof(WORD) );
        if ((ret = !wine_server_call_err( req ))) *count = wine_server_reply_size(reply) / sizeof(WORD);
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *	ReadConsoleOutputCharacterA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReadConsoleOutputCharacterA( HANDLE handle, LPSTR buffer, DWORD length,
                                                           COORD coord, DWORD *count )
{
    DWORD read;
    BOOL ret;
    LPWSTR wptr;

    if (!count)
    {
        SetLastError( ERROR_INVALID_ACCESS );
        return FALSE;
    }

    *count = 0;
    if (!(wptr = HeapAlloc( GetProcessHeap(), 0, length * sizeof(WCHAR) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    if ((ret = ReadConsoleOutputCharacterW( handle, wptr, length, coord, &read )))
    {
        read = WideCharToMultiByte( GetConsoleOutputCP(), 0, wptr, read, buffer, length, NULL, NULL);
        *count = read;
    }
    HeapFree( GetProcessHeap(), 0, wptr );
    return ret;
}


/******************************************************************************
 *	ReadConsoleOutputCharacterW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReadConsoleOutputCharacterW( HANDLE handle, LPWSTR buffer, DWORD length,
                                                           COORD coord, DWORD *count )
{
    BOOL ret;

    TRACE( "(%p,%p,%d,%dx%d,%p)\n", handle, buffer, length, coord.X, coord.Y, count );

    if (!count)
    {
        SetLastError( ERROR_INVALID_ACCESS );
        return FALSE;
    }

    *count = 0;
    SERVER_START_REQ( read_console_output )
    {
        req->handle = console_handle_unmap( handle );
        req->x      = coord.X;
        req->y      = coord.Y;
        req->mode   = CHAR_INFO_MODE_TEXT;
        req->wrap   = TRUE;
        wine_server_set_reply( req, buffer, length * sizeof(WCHAR) );
        if ((ret = !wine_server_call_err( req ))) *count = wine_server_reply_size(reply) / sizeof(WCHAR);
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *	ReadConsoleOutputA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReadConsoleOutputA( HANDLE handle, CHAR_INFO *buffer, COORD size,
                                                  COORD coord, SMALL_RECT *region )
{
    BOOL ret;
    int y;

    ret = ReadConsoleOutputW( handle, buffer, size, coord, region );
    if (ret && region->Right >= region->Left)
    {
        for (y = 0; y <= region->Bottom - region->Top; y++)
            char_info_WtoA( &buffer[(coord.Y + y) * size.X + coord.X], region->Right - region->Left + 1 );
    }
    return ret;
}


/******************************************************************************
 *	ReadConsoleOutputW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReadConsoleOutputW( HANDLE handle, CHAR_INFO *buffer, COORD size,
                                                  COORD coord, SMALL_RECT *region )
{
    int width, height, y;
    BOOL ret = TRUE;

    width = min( region->Right - region->Left + 1, size.X - coord.X );
    height = min( region->Bottom - region->Top + 1, size.Y - coord.Y );

    if (width > 0 && height > 0)
    {
        for (y = 0; y < height; y++)
        {
            SERVER_START_REQ( read_console_output )
            {
                req->handle = console_handle_unmap( handle );
                req->x      = region->Left;
                req->y      = region->Top + y;
                req->mode   = CHAR_INFO_MODE_TEXTATTR;
                req->wrap   = FALSE;
                wine_server_set_reply( req, &buffer[(y+coord.Y) * size.X + coord.X],
                                       width * sizeof(CHAR_INFO) );
                if ((ret = !wine_server_call_err( req )))
                {
                    width  = min( width, reply->width - region->Left );
                    height = min( height, reply->height - region->Top );
                }
            }
            SERVER_END_REQ;
            if (!ret) break;
        }
    }
    region->Bottom = region->Top + height - 1;
    region->Right = region->Left + width - 1;
    return ret;
}


/******************************************************************************
 *	ScrollConsoleScreenBufferA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ScrollConsoleScreenBufferA( HANDLE handle, SMALL_RECT *scroll,
                                                          SMALL_RECT *clip, COORD origin, CHAR_INFO *fill )
{
    CHAR_INFO ciW;

    ciW.Attributes = fill->Attributes;
    MultiByteToWideChar( GetConsoleOutputCP(), 0, &fill->Char.AsciiChar, 1, &ciW.Char.UnicodeChar, 1 );

    return ScrollConsoleScreenBufferW( handle, scroll, clip, origin, &ciW );
}


/******************************************************************************
 *	ScrollConsoleScreenBufferW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ScrollConsoleScreenBufferW( HANDLE handle, SMALL_RECT *scroll,
                                                          SMALL_RECT *clip_rect, COORD origin,
                                                          CHAR_INFO *fill )
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    SMALL_RECT dst, clip;
    int i, j, start = -1;
    DWORD ret;
    BOOL inside;
    COORD src;

    if (clip_rect)
	TRACE( "(%p,(%d,%d-%d,%d),(%d,%d-%d,%d),%d-%d,%p)\n", handle,
               scroll->Left, scroll->Top, scroll->Right, scroll->Bottom,
               clip_rect->Left, clip_rect->Top, clip_rect->Right, clip_rect->Bottom,
               origin.X, origin.Y, fill );
    else
	TRACE("(%p,(%d,%d-%d,%d),(nil),%d-%d,%p)\n", handle,
	      scroll->Left, scroll->Top, scroll->Right, scroll->Bottom,
	      origin.X, origin.Y, fill );

    if (!GetConsoleScreenBufferInfo( handle, &info )) return FALSE;

    src.X = scroll->Left;
    src.Y = scroll->Top;

    /* step 1: get dst rect */
    dst.Left = origin.X;
    dst.Top = origin.Y;
    dst.Right = dst.Left + (scroll->Right - scroll->Left);
    dst.Bottom = dst.Top + (scroll->Bottom - scroll->Top);

    /* step 2a: compute the final clip rect (optional passed clip and screen buffer limits */
    if (clip_rect)
    {
	clip.Left   = max(0, clip_rect->Left);
	clip.Right  = min(info.dwSize.X - 1, clip_rect->Right);
	clip.Top    = max(0, clip_rect->Top);
	clip.Bottom = min(info.dwSize.Y - 1, clip_rect->Bottom);
    }
    else
    {
	clip.Left   = 0;
	clip.Right  = info.dwSize.X - 1;
	clip.Top    = 0;
	clip.Bottom = info.dwSize.Y - 1;
    }
    if (clip.Left > clip.Right || clip.Top > clip.Bottom) return FALSE;

    /* step 2b: clip dst rect */
    if (dst.Left   < clip.Left  ) {src.X += clip.Left - dst.Left; dst.Left   = clip.Left;}
    if (dst.Top    < clip.Top   ) {src.Y += clip.Top  - dst.Top;  dst.Top    = clip.Top;}
    if (dst.Right  > clip.Right ) dst.Right  = clip.Right;
    if (dst.Bottom > clip.Bottom) dst.Bottom = clip.Bottom;

    /* step 3: transfer the bits */
    SERVER_START_REQ( move_console_output )
    {
        req->handle = console_handle_unmap( handle );
	req->x_src = src.X;
	req->y_src = src.Y;
	req->x_dst = dst.Left;
	req->y_dst = dst.Top;
	req->w = dst.Right - dst.Left + 1;
	req->h = dst.Bottom - dst.Top + 1;
	ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

    if (!ret) return FALSE;

    /* step 4: clean out the exposed part */

    /* have to write cell [i,j] if it is not in dst rect (because it has already
     * been written to by the scroll) and is in clip (we shall not write
     * outside of clip)
     */
    for (j = max(scroll->Top, clip.Top); j <= min(scroll->Bottom, clip.Bottom); j++)
    {
	inside = dst.Top <= j && j <= dst.Bottom;
	start = -1;
	for (i = max(scroll->Left, clip.Left); i <= min(scroll->Right, clip.Right); i++)
	{
	    if (inside && dst.Left <= i && i <= dst.Right)
	    {
		if (start != -1)
		{
		    fill_console_output( handle, start, j, i - start, fill );
		    start = -1;
		}
	    }
	    else
	    {
		if (start == -1) start = i;
	    }
	}
	if (start != -1) fill_console_output( handle, start, j, i - start, fill );
    }

    return TRUE;
}


/******************************************************************************
 *	SetConsoleActiveScreenBuffer   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetConsoleActiveScreenBuffer( HANDLE handle )
{
    BOOL ret;

    TRACE( "(%p)\n", handle );

    SERVER_START_REQ( set_console_input_info )
    {
        req->handle    = 0;
        req->mask      = SET_CONSOLE_INPUT_INFO_ACTIVE_SB;
        req->active_sb = wine_server_obj_handle( handle );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *	SetConsoleCP   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetConsoleCP( UINT cp )
{
    BOOL ret;

    if (!IsValidCodePage( cp ))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    SERVER_START_REQ( set_console_input_info )
    {
        req->handle   = 0;
        req->mask     = SET_CONSOLE_INPUT_INFO_INPUT_CODEPAGE;
        req->input_cp = cp;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *	SetConsoleCursorInfo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetConsoleCursorInfo( HANDLE handle, CONSOLE_CURSOR_INFO *info )
{
    BOOL ret;

    TRACE( "(%p,%d,%d)\n", handle, info->dwSize, info->bVisible);

    SERVER_START_REQ( set_console_output_info )
    {
        req->handle         = console_handle_unmap( handle );
        req->cursor_size    = info->dwSize;
        req->cursor_visible = info->bVisible;
        req->mask           = SET_CONSOLE_OUTPUT_INFO_CURSOR_GEOM;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *	SetConsoleCursorPosition   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetConsoleCursorPosition( HANDLE handle, COORD pos )
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    int w, h, do_move = 0;
    BOOL ret;

    TRACE( "%p %d %d\n", handle, pos.X, pos.Y );

    SERVER_START_REQ( set_console_output_info )
    {
        req->handle         = console_handle_unmap( handle );
        req->cursor_x       = pos.X;
        req->cursor_y       = pos.Y;
        req->mask           = SET_CONSOLE_OUTPUT_INFO_CURSOR_POS;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

    if (!ret || !GetConsoleScreenBufferInfo( handle, &info )) return FALSE;

    /* if cursor is no longer visible, scroll the visible window... */
    w = info.srWindow.Right - info.srWindow.Left + 1;
    h = info.srWindow.Bottom - info.srWindow.Top + 1;
    if (pos.X < info.srWindow.Left)
    {
	info.srWindow.Left = min(pos.X, info.dwSize.X - w);
	do_move = 1;
    }
    else if (pos.X > info.srWindow.Right)
    {
	info.srWindow.Left = max(pos.X, w) - w + 1;
	do_move = 1;
    }
    info.srWindow.Right = info.srWindow.Left + w - 1;

    if (pos.Y < info.srWindow.Top)
    {
	info.srWindow.Top = min(pos.Y, info.dwSize.Y - h);
	do_move = 1;
    }
    else if (pos.Y > info.srWindow.Bottom)
    {
	info.srWindow.Top = max(pos.Y, h) - h + 1;
	do_move = 1;
    }
    info.srWindow.Bottom = info.srWindow.Top + h - 1;

    if (do_move) ret = SetConsoleWindowInfo( handle, TRUE, &info.srWindow );
    return ret;
}


/******************************************************************************
 *	SetConsoleInputExeNameA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetConsoleInputExeNameA( LPCSTR name )
{
    if (!name || !name[0])
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    RtlEnterCriticalSection( &console_section );
    MultiByteToWideChar( CP_ACP, 0, name, -1, input_exe, ARRAY_SIZE(input_exe) );
    RtlLeaveCriticalSection( &console_section );
    return TRUE;
}


/******************************************************************************
 *	SetConsoleInputExeNameW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetConsoleInputExeNameW( LPCWSTR name )
{
    if (!name || !name[0])
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    RtlEnterCriticalSection( &console_section );
    lstrcpynW( input_exe, name, ARRAY_SIZE(input_exe) );
    RtlLeaveCriticalSection( &console_section );
    return TRUE;
}


/******************************************************************************
 *	SetConsoleMode   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetConsoleMode( HANDLE handle, DWORD mode )
{
    BOOL ret;

    SERVER_START_REQ(set_console_mode)
    {
	req->handle = console_handle_unmap( handle );
	req->mode = mode;
	ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

    TRACE( "(%p,%x) retval == %d\n", handle, mode, ret );
    return ret;
}


/******************************************************************************
 *	SetConsoleOutputCP   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetConsoleOutputCP( UINT cp )
{
    BOOL ret;

    if (!IsValidCodePage( cp ))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    SERVER_START_REQ( set_console_input_info )
    {
        req->handle    = 0;
        req->mask      = SET_CONSOLE_INPUT_INFO_OUTPUT_CODEPAGE;
        req->output_cp = cp;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *	SetConsoleScreenBufferInfoEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetConsoleScreenBufferInfoEx( HANDLE handle,
                                                            CONSOLE_SCREEN_BUFFER_INFOEX *info )
{
    FIXME( "(%p %p): stub!\n", handle, info );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/******************************************************************************
 *	SetConsoleScreenBufferSize   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetConsoleScreenBufferSize( HANDLE handle, COORD size )
{
    BOOL ret;

    TRACE( "(%p,(%d,%d))\n", handle, size.X, size.Y );
    SERVER_START_REQ( set_console_output_info )
    {
        req->handle = console_handle_unmap( handle );
        req->width  = size.X;
        req->height = size.Y;
        req->mask   = SET_CONSOLE_OUTPUT_INFO_SIZE;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *	SetConsoleTextAttribute   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetConsoleTextAttribute( HANDLE handle, WORD attr )
{
    BOOL ret;

    TRACE( "(%p,%d)\n", handle, attr );
    SERVER_START_REQ( set_console_output_info )
    {
        req->handle = console_handle_unmap( handle );
        req->attr   = attr;
        req->mask   = SET_CONSOLE_OUTPUT_INFO_ATTR;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *	SetConsoleTitleW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetConsoleTitleW( LPCWSTR title )
{
    BOOL ret;

    TRACE( "%s\n", debugstr_w( title ));
    SERVER_START_REQ( set_console_input_info )
    {
        req->handle = 0;
        req->mask = SET_CONSOLE_INPUT_INFO_TITLE;
        wine_server_add_data( req, title, lstrlenW(title) * sizeof(WCHAR) );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *	SetConsoleWindowInfo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetConsoleWindowInfo( HANDLE handle, BOOL absolute, SMALL_RECT *window )
{
    SMALL_RECT rect = *window;
    BOOL ret;

    TRACE( "(%p,%d,(%d,%d-%d,%d))\n", handle, absolute, rect.Left, rect.Top, rect.Right, rect.Bottom );

    if (!absolute)
    {
	CONSOLE_SCREEN_BUFFER_INFO info;

	if (!GetConsoleScreenBufferInfo( handle, &info )) return FALSE;
	rect.Left   += info.srWindow.Left;
	rect.Top    += info.srWindow.Top;
	rect.Right  += info.srWindow.Right;
	rect.Bottom += info.srWindow.Bottom;
    }
    SERVER_START_REQ( set_console_output_info )
    {
        req->handle     = console_handle_unmap( handle );
	req->win_left   = rect.Left;
	req->win_top    = rect.Top;
	req->win_right  = rect.Right;
	req->win_bottom = rect.Bottom;
        req->mask       = SET_CONSOLE_OUTPUT_INFO_DISPLAY_WINDOW;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

    return ret;
}


/******************************************************************************
 *	WriteConsoleInputA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteConsoleInputA( HANDLE handle, const INPUT_RECORD *buffer,
                                                  DWORD count, DWORD *written )
{
    INPUT_RECORD *recW = NULL;
    BOOL ret;

    if (count > 0)
    {
        if (!buffer)
        {
            SetLastError( ERROR_INVALID_ACCESS );
            return FALSE;
        }
        if (!(recW = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*recW) )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }
        memcpy( recW, buffer, count * sizeof(*recW) );
        input_records_AtoW( recW, count );
    }
    ret = WriteConsoleInputW( handle, recW, count, written );
    HeapFree( GetProcessHeap(), 0, recW );
    return ret;
}


/******************************************************************************
 *	WriteConsoleInputW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteConsoleInputW( HANDLE handle, const INPUT_RECORD *buffer,
                                                  DWORD count, DWORD *written )
{
    DWORD events_written = 0;
    BOOL ret;

    TRACE( "(%p,%p,%d,%p)\n", handle, buffer, count, written );

    if (count > 0 && !buffer)
    {
        SetLastError( ERROR_INVALID_ACCESS );
        return FALSE;
    }
    SERVER_START_REQ( write_console_input )
    {
        req->handle = console_handle_unmap( handle );
        wine_server_add_data( req, buffer, count * sizeof(INPUT_RECORD) );
        if ((ret = !wine_server_call_err( req ))) events_written = reply->written;
    }
    SERVER_END_REQ;

    if (written) *written = events_written;
    else
    {
        SetLastError( ERROR_INVALID_ACCESS );
        ret = FALSE;
    }
    return ret;
}


/***********************************************************************
 *	WriteConsoleOutputA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteConsoleOutputA( HANDLE handle, const CHAR_INFO *buffer,
                                                   COORD size, COORD coord, SMALL_RECT *region )
{
    int y;
    BOOL ret;
    COORD new_size, new_coord;
    CHAR_INFO *ciW;

    new_size.X = min( region->Right - region->Left + 1, size.X - coord.X );
    new_size.Y = min( region->Bottom - region->Top + 1, size.Y - coord.Y );

    if (new_size.X <= 0 || new_size.Y <= 0)
    {
        region->Bottom = region->Top + new_size.Y - 1;
        region->Right = region->Left + new_size.X - 1;
        return TRUE;
    }

    /* only copy the useful rectangle */
    if (!(ciW = HeapAlloc( GetProcessHeap(), 0, sizeof(CHAR_INFO) * new_size.X * new_size.Y )))
        return FALSE;
    for (y = 0; y < new_size.Y; y++)
        memcpy( &ciW[y * new_size.X], &buffer[(y + coord.Y) * size.X + coord.X],
                new_size.X * sizeof(CHAR_INFO) );
    char_info_AtoW( ciW, new_size.X * new_size.Y );
    new_coord.X = new_coord.Y = 0;
    ret = WriteConsoleOutputW( handle, ciW, new_size, new_coord, region );
    HeapFree( GetProcessHeap(), 0, ciW );
    return ret;
}


/***********************************************************************
 *	WriteConsoleOutputW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteConsoleOutputW( HANDLE handle, const CHAR_INFO *buffer,
                                                   COORD size, COORD coord, SMALL_RECT *region )
{
    int width, height, y;
    BOOL ret = TRUE;

    TRACE( "(%p,%p,(%d,%d),(%d,%d),(%d,%dx%d,%d)\n",
           handle, buffer, size.X, size.Y, coord.X, coord.Y,
           region->Left, region->Top, region->Right, region->Bottom );

    width = min( region->Right - region->Left + 1, size.X - coord.X );
    height = min( region->Bottom - region->Top + 1, size.Y - coord.Y );

    if (width > 0 && height > 0)
    {
        for (y = 0; y < height; y++)
        {
            SERVER_START_REQ( write_console_output )
            {
                req->handle = console_handle_unmap( handle );
                req->x      = region->Left;
                req->y      = region->Top + y;
                req->mode   = CHAR_INFO_MODE_TEXTATTR;
                req->wrap   = FALSE;
                wine_server_add_data( req, &buffer[(y + coord.Y) * size.X + coord.X],
                                      width * sizeof(CHAR_INFO));
                if ((ret = !wine_server_call_err( req )))
                {
                    width  = min( width, reply->width - region->Left );
                    height = min( height, reply->height - region->Top );
                }
            }
            SERVER_END_REQ;
            if (!ret) break;
        }
    }
    region->Bottom = region->Top + height - 1;
    region->Right = region->Left + width - 1;
    return ret;
}


/******************************************************************************
 *	WriteConsoleOutputAttribute   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteConsoleOutputAttribute( HANDLE handle, const WORD *attr, DWORD length,
                                                           COORD coord, DWORD *written )
{
    BOOL ret;

    TRACE( "(%p,%p,%d,%dx%d,%p)\n", handle, attr, length, coord.X, coord.Y, written );

    if ((length > 0 && !attr) || !written)
    {
        SetLastError( ERROR_INVALID_ACCESS );
        return FALSE;
    }

    *written = 0;
    SERVER_START_REQ( write_console_output )
    {
        req->handle = console_handle_unmap( handle );
        req->x      = coord.X;
        req->y      = coord.Y;
        req->mode   = CHAR_INFO_MODE_ATTR;
        req->wrap   = TRUE;
        wine_server_add_data( req, attr, length * sizeof(WORD) );
        if ((ret = !wine_server_call_err( req ))) *written = reply->written;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *	WriteConsoleOutputCharacterA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteConsoleOutputCharacterA( HANDLE handle, LPCSTR str, DWORD length,
                                                            COORD coord, DWORD *written )
{
    BOOL ret;
    LPWSTR strW = NULL;
    DWORD lenW = 0;

    TRACE( "(%p,%s,%d,%dx%d,%p)\n", handle, debugstr_an(str, length), length, coord.X, coord.Y, written );

    if (length > 0)
    {
        if (!str)
        {
            SetLastError( ERROR_INVALID_ACCESS );
            return FALSE;
        }
        lenW = MultiByteToWideChar( GetConsoleOutputCP(), 0, str, length, NULL, 0 );

        if (!(strW = HeapAlloc( GetProcessHeap(), 0, lenW * sizeof(WCHAR) )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }
        MultiByteToWideChar( GetConsoleOutputCP(), 0, str, length, strW, lenW );
    }
    ret = WriteConsoleOutputCharacterW( handle, strW, lenW, coord, written );
    HeapFree( GetProcessHeap(), 0, strW );
    return ret;
}


/******************************************************************************
 *	WriteConsoleOutputCharacterW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteConsoleOutputCharacterW( HANDLE handle, LPCWSTR str, DWORD length,
                                                            COORD coord, DWORD *written )
{
    BOOL ret;

    TRACE( "(%p,%s,%d,%dx%d,%p)\n", handle, debugstr_wn(str, length), length, coord.X, coord.Y, written );

    if ((length > 0 && !str) || !written)
    {
        SetLastError( ERROR_INVALID_ACCESS );
        return FALSE;
    }

    *written = 0;
    SERVER_START_REQ( write_console_output )
    {
        req->handle = console_handle_unmap( handle );
        req->x      = coord.X;
        req->y      = coord.Y;
        req->mode   = CHAR_INFO_MODE_TEXT;
        req->wrap   = TRUE;
        wine_server_add_data( req, str, length * sizeof(WCHAR) );
        if ((ret = !wine_server_call_err( req ))) *written = reply->written;
    }
    SERVER_END_REQ;
    return ret;
}
