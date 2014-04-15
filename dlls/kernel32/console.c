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

/* Reference applications:
 * -  IDA (interactive disassembler) full version 3.75. Works.
 * -  LYNX/W32. Works mostly, some keys crash it.
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <assert.h>
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif

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
#include "wine/unicode.h"
#include "wine/debug.h"
#include "excpt.h"
#include "console_private.h"
#include "kernel_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(console);

static CRITICAL_SECTION CONSOLE_CritSect;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &CONSOLE_CritSect,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": CONSOLE_CritSect") }
};
static CRITICAL_SECTION CONSOLE_CritSect = { &critsect_debug, -1, 0, 0, 0, 0 };

static const WCHAR coninW[] = {'C','O','N','I','N','$',0};
static const WCHAR conoutW[] = {'C','O','N','O','U','T','$',0};

/* FIXME: this is not thread safe */
static HANDLE console_wait_event;

/* map input records to ASCII */
static void input_records_WtoA( INPUT_RECORD *buffer, int count )
{
    int i;
    char ch;

    for (i = 0; i < count; i++)
    {
        if (buffer[i].EventType != KEY_EVENT) continue;
        WideCharToMultiByte( GetConsoleCP(), 0,
                             &buffer[i].Event.KeyEvent.uChar.UnicodeChar, 1, &ch, 1, NULL, NULL );
        buffer[i].Event.KeyEvent.uChar.AsciiChar = ch;
    }
}

/* map input records to Unicode */
static void input_records_AtoW( INPUT_RECORD *buffer, int count )
{
    int i;
    WCHAR ch;

    for (i = 0; i < count; i++)
    {
        if (buffer[i].EventType != KEY_EVENT) continue;
        MultiByteToWideChar( GetConsoleCP(), 0,
                             &buffer[i].Event.KeyEvent.uChar.AsciiChar, 1, &ch, 1 );
        buffer[i].Event.KeyEvent.uChar.UnicodeChar = ch;
    }
}

/* map char infos to ASCII */
static void char_info_WtoA( CHAR_INFO *buffer, int count )
{
    char ch;

    while (count-- > 0)
    {
        WideCharToMultiByte( GetConsoleOutputCP(), 0, &buffer->Char.UnicodeChar, 1,
                             &ch, 1, NULL, NULL );
        buffer->Char.AsciiChar = ch;
        buffer++;
    }
}

/* map char infos to Unicode */
static void char_info_AtoW( CHAR_INFO *buffer, int count )
{
    WCHAR ch;

    while (count-- > 0)
    {
        MultiByteToWideChar( GetConsoleOutputCP(), 0, &buffer->Char.AsciiChar, 1, &ch, 1 );
        buffer->Char.UnicodeChar = ch;
        buffer++;
    }
}

static struct termios S_termios;        /* saved termios for bare consoles */
static BOOL S_termios_raw /* = FALSE */;

/* The scheme for bare consoles for managing raw/cooked settings is as follows:
 * - a bare console is created for all CUI programs started from command line (without
 *   wineconsole) (let's call those PS)
 * - of course, every child of a PS which requires console inheritance will get it
 * - the console termios attributes are saved at the start of program which is attached to be
 *   bare console
 * - if any program attached to a bare console requests input from console, the console is
 *   turned into raw mode
 * - when the program which created the bare console (the program started from command line)
 *   exits, it will restore the console termios attributes it saved at startup (this
 *   will put back the console into cooked mode if it had been put in raw mode)
 * - if any other program attached to this bare console is still alive, the Unix shell will put
 *   it in the background, hence forbidding access to the console. Therefore, reading console
 *   input will not be available when the bare console creator has died.
 *   FIXME: This is a limitation of current implementation
 */

/* returns the fd for a bare console (-1 otherwise) */
static int  get_console_bare_fd(HANDLE hin)
{
    int         fd;

    if (is_console_handle(hin) &&
        wine_server_handle_to_fd(wine_server_ptr_handle(console_handle_unmap(hin)),
                                 0, &fd, NULL) == STATUS_SUCCESS)
        return fd;
    return -1;
}

static BOOL save_console_mode(HANDLE hin)
{
    int         fd;
    BOOL        ret;

    if ((fd = get_console_bare_fd(hin)) == -1) return FALSE;
    ret = tcgetattr(fd, &S_termios) >= 0;
    close(fd);
    return ret;
}

static BOOL put_console_into_raw_mode(int fd)
{
    RtlEnterCriticalSection(&CONSOLE_CritSect);
    if (!S_termios_raw)
    {
        struct termios term = S_termios;

        term.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);
        term.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        term.c_cflag &= ~(CSIZE | PARENB);
        term.c_cflag |= CS8;
        /* FIXME: we should actually disable output processing here
         * and let kernel32/console.c do the job (with support of enable/disable of
         * processed output)
         */
        /* term.c_oflag &= ~(OPOST); */
        term.c_cc[VMIN] = 1;
        term.c_cc[VTIME] = 0;
        S_termios_raw = tcsetattr(fd, TCSANOW, &term) >= 0;
    }
    RtlLeaveCriticalSection(&CONSOLE_CritSect);

    return S_termios_raw;
}

/* put back the console in cooked mode iff we're the process which created the bare console
 * we don't test if this process has set the console in raw mode as it could be one of its
 * children who did it
 */
static BOOL restore_console_mode(HANDLE hin)
{
    int         fd;
    BOOL        ret;

    if (!S_termios_raw ||
        RtlGetCurrentPeb()->ProcessParameters->ConsoleHandle != KERNEL32_CONSOLE_SHELL)
        return TRUE;
    if ((fd = get_console_bare_fd(hin)) == -1) return FALSE;
    ret = tcsetattr(fd, TCSANOW, &S_termios) >= 0;
    close(fd);
    TERM_Exit();
    return ret;
}

/******************************************************************************
 * GetConsoleWindow [KERNEL32.@] Get hwnd of the console window.
 *
 * RETURNS
 *   Success: hwnd of the console window.
 *   Failure: NULL
 */
HWND WINAPI GetConsoleWindow(VOID)
{
    HWND hWnd = NULL;

    SERVER_START_REQ(get_console_input_info)
    {
        req->handle = 0;
        if (!wine_server_call_err(req)) hWnd = wine_server_ptr_handle( reply->win );
    }
    SERVER_END_REQ;

    return hWnd;
}


/******************************************************************************
 * GetConsoleCP [KERNEL32.@]  Returns the OEM code page for the console
 *
 * RETURNS
 *    Code page code
 */
UINT WINAPI GetConsoleCP(VOID)
{
    BOOL ret;
    UINT codepage = GetOEMCP(); /* default value */

    SERVER_START_REQ(get_console_input_info)
    {
        req->handle = 0;
        ret = !wine_server_call_err(req);
        if (ret && reply->input_cp)
            codepage = reply->input_cp;
    }
    SERVER_END_REQ;

    return codepage;
}


/******************************************************************************
 *  SetConsoleCP	 [KERNEL32.@]
 */
BOOL WINAPI SetConsoleCP(UINT cp)
{
    BOOL ret;

    if (!IsValidCodePage(cp))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SERVER_START_REQ(set_console_input_info)
    {
        req->handle   = 0;
        req->mask     = SET_CONSOLE_INPUT_INFO_INPUT_CODEPAGE;
        req->input_cp = cp;
        ret = !wine_server_call_err(req);
    }
    SERVER_END_REQ;

    return ret;
}


/***********************************************************************
 *            GetConsoleOutputCP   (KERNEL32.@)
 */
UINT WINAPI GetConsoleOutputCP(VOID)
{
    BOOL ret;
    UINT codepage = GetOEMCP(); /* default value */

    SERVER_START_REQ(get_console_input_info)
    {
        req->handle = 0;
        ret = !wine_server_call_err(req);
        if (ret && reply->output_cp)
            codepage = reply->output_cp;
    }
    SERVER_END_REQ;

    return codepage;
}


/******************************************************************************
 * SetConsoleOutputCP [KERNEL32.@]  Set the output codepage used by the console
 *
 * PARAMS
 *    cp [I] code page to set
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetConsoleOutputCP(UINT cp)
{
    BOOL ret;

    if (!IsValidCodePage(cp))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SERVER_START_REQ(set_console_input_info)
    {
        req->handle   = 0;
        req->mask     = SET_CONSOLE_INPUT_INFO_OUTPUT_CODEPAGE;
        req->output_cp = cp;
        ret = !wine_server_call_err(req);
    }
    SERVER_END_REQ;

    return ret;
}


/***********************************************************************
 *           Beep   (KERNEL32.@)
 */
BOOL WINAPI Beep( DWORD dwFreq, DWORD dwDur )
{
    static const char beep = '\a';
    /* dwFreq and dwDur are ignored by Win95 */
    if (isatty(2)) write( 2, &beep, 1 );
    return TRUE;
}


/******************************************************************
 *		OpenConsoleW            (KERNEL32.@)
 *
 * Undocumented
 *      Open a handle to the current process console.
 *      Returns INVALID_HANDLE_VALUE on failure.
 */
HANDLE WINAPI OpenConsoleW(LPCWSTR name, DWORD access, BOOL inherit, DWORD creation)
{
    HANDLE      output = INVALID_HANDLE_VALUE;
    HANDLE      ret;

    TRACE("(%s, 0x%08x, %d, %u)\n", debugstr_w(name), access, inherit, creation);

    if (name)
    {
        if (strcmpiW(coninW, name) == 0)
            output = (HANDLE) FALSE;
        else if (strcmpiW(conoutW, name) == 0)
            output = (HANDLE) TRUE;
    }

    if (output == INVALID_HANDLE_VALUE || creation != OPEN_EXISTING)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    SERVER_START_REQ( open_console )
    {
        req->from       = wine_server_obj_handle( output );
        req->access     = access;
        req->attributes = inherit ? OBJ_INHERIT : 0;
        req->share      = FILE_SHARE_READ | FILE_SHARE_WRITE;
        wine_server_call_err( req );
        ret = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    if (ret)
        ret = console_handle_map(ret);

    return ret;
}

/******************************************************************
 *		VerifyConsoleIoHandle            (KERNEL32.@)
 *
 * Undocumented
 */
BOOL WINAPI VerifyConsoleIoHandle(HANDLE handle)
{
    BOOL ret;

    if (!is_console_handle(handle)) return FALSE;
    SERVER_START_REQ(get_console_mode)
    {
	req->handle = console_handle_unmap(handle);
	ret = !wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************
 *		DuplicateConsoleHandle            (KERNEL32.@)
 *
 * Undocumented
 */
HANDLE WINAPI DuplicateConsoleHandle(HANDLE handle, DWORD access, BOOL inherit,
                                     DWORD options)
{
    HANDLE      ret;

    if (!is_console_handle(handle) ||
        !DuplicateHandle(GetCurrentProcess(), wine_server_ptr_handle(console_handle_unmap(handle)),
                         GetCurrentProcess(), &ret, access, inherit, options))
        return INVALID_HANDLE_VALUE;
    return console_handle_map(ret);
}

/******************************************************************
 *		CloseConsoleHandle            (KERNEL32.@)
 *
 * Undocumented
 */
BOOL WINAPI CloseConsoleHandle(HANDLE handle)
{
    if (!is_console_handle(handle)) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return CloseHandle(wine_server_ptr_handle(console_handle_unmap(handle)));
}

/******************************************************************
 *		GetConsoleInputWaitHandle            (KERNEL32.@)
 *
 * Undocumented
 */
HANDLE WINAPI GetConsoleInputWaitHandle(void)
{
    if (!console_wait_event)
    {
        SERVER_START_REQ(get_console_wait_event)
        {
            if (!wine_server_call_err( req ))
                console_wait_event = wine_server_ptr_handle( reply->handle );
        }
        SERVER_END_REQ;
    }
    return console_wait_event;
}


/******************************************************************************
 * WriteConsoleInputA [KERNEL32.@]
 */
BOOL WINAPI WriteConsoleInputA( HANDLE handle, const INPUT_RECORD *buffer,
                                DWORD count, LPDWORD written )
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
 * WriteConsoleInputW [KERNEL32.@]
 */
BOOL WINAPI WriteConsoleInputW( HANDLE handle, const INPUT_RECORD *buffer,
                                DWORD count, LPDWORD written )
{
    DWORD events_written = 0;
    BOOL ret;

    TRACE("(%p,%p,%d,%p)\n", handle, buffer, count, written);

    if (count > 0 && !buffer)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        return FALSE;
    }

    SERVER_START_REQ( write_console_input )
    {
        req->handle = console_handle_unmap(handle);
        wine_server_add_data( req, buffer, count * sizeof(INPUT_RECORD) );
        if ((ret = !wine_server_call_err( req )))
            events_written = reply->written;
    }
    SERVER_END_REQ;

    if (written) *written = events_written;
    else
    {
        SetLastError(ERROR_INVALID_ACCESS);
        ret = FALSE;
    }
    return ret;
}


/***********************************************************************
 *            WriteConsoleOutputA   (KERNEL32.@)
 */
BOOL WINAPI WriteConsoleOutputA( HANDLE hConsoleOutput, const CHAR_INFO *lpBuffer,
                                 COORD size, COORD coord, LPSMALL_RECT region )
{
    int y;
    BOOL ret;
    COORD new_size, new_coord;
    CHAR_INFO *ciw;

    new_size.X = min( region->Right - region->Left + 1, size.X - coord.X );
    new_size.Y = min( region->Bottom - region->Top + 1, size.Y - coord.Y );

    if (new_size.X <= 0 || new_size.Y <= 0)
    {
        region->Bottom = region->Top + new_size.Y - 1;
        region->Right = region->Left + new_size.X - 1;
        return TRUE;
    }

    /* only copy the useful rectangle */
    if (!(ciw = HeapAlloc( GetProcessHeap(), 0, sizeof(CHAR_INFO) * new_size.X * new_size.Y )))
        return FALSE;
    for (y = 0; y < new_size.Y; y++)
    {
        memcpy( &ciw[y * new_size.X], &lpBuffer[(y + coord.Y) * size.X + coord.X],
                new_size.X * sizeof(CHAR_INFO) );
        char_info_AtoW( &ciw[ y * new_size.X ], new_size.X );
    }
    new_coord.X = new_coord.Y = 0;
    ret = WriteConsoleOutputW( hConsoleOutput, ciw, new_size, new_coord, region );
    HeapFree( GetProcessHeap(), 0, ciw );
    return ret;
}


/***********************************************************************
 *            WriteConsoleOutputW   (KERNEL32.@)
 */
BOOL WINAPI WriteConsoleOutputW( HANDLE hConsoleOutput, const CHAR_INFO *lpBuffer,
                                 COORD size, COORD coord, LPSMALL_RECT region )
{
    int width, height, y;
    BOOL ret = TRUE;

    TRACE("(%p,%p,(%d,%d),(%d,%d),(%d,%dx%d,%d)\n",
          hConsoleOutput, lpBuffer, size.X, size.Y, coord.X, coord.Y,
          region->Left, region->Top, region->Right, region->Bottom);

    width = min( region->Right - region->Left + 1, size.X - coord.X );
    height = min( region->Bottom - region->Top + 1, size.Y - coord.Y );

    if (width > 0 && height > 0)
    {
        for (y = 0; y < height; y++)
        {
            SERVER_START_REQ( write_console_output )
            {
                req->handle = console_handle_unmap(hConsoleOutput);
                req->x      = region->Left;
                req->y      = region->Top + y;
                req->mode   = CHAR_INFO_MODE_TEXTATTR;
                req->wrap   = FALSE;
                wine_server_add_data( req, &lpBuffer[(y + coord.Y) * size.X + coord.X],
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
 * WriteConsoleOutputCharacterA [KERNEL32.@]
 *
 * See WriteConsoleOutputCharacterW.
 */
BOOL WINAPI WriteConsoleOutputCharacterA( HANDLE hConsoleOutput, LPCSTR str, DWORD length,
                                          COORD coord, LPDWORD lpNumCharsWritten )
{
    BOOL ret;
    LPWSTR strW = NULL;
    DWORD lenW = 0;

    TRACE("(%p,%s,%d,%dx%d,%p)\n", hConsoleOutput,
          debugstr_an(str, length), length, coord.X, coord.Y, lpNumCharsWritten);

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

    ret = WriteConsoleOutputCharacterW( hConsoleOutput, strW, lenW, coord, lpNumCharsWritten );
    HeapFree( GetProcessHeap(), 0, strW );
    return ret;
}


/******************************************************************************
 * WriteConsoleOutputAttribute [KERNEL32.@]  Sets attributes for some cells in
 * 					     the console screen buffer
 *
 * PARAMS
 *    hConsoleOutput    [I] Handle to screen buffer
 *    attr              [I] Pointer to buffer with write attributes
 *    length            [I] Number of cells to write to
 *    coord             [I] Coords of first cell
 *    lpNumAttrsWritten [O] Pointer to number of cells written
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 *
 */
BOOL WINAPI WriteConsoleOutputAttribute( HANDLE hConsoleOutput, const WORD *attr, DWORD length,
                                         COORD coord, LPDWORD lpNumAttrsWritten )
{
    BOOL ret;

    TRACE("(%p,%p,%d,%dx%d,%p)\n", hConsoleOutput,attr,length,coord.X,coord.Y,lpNumAttrsWritten);

    if ((length > 0 && !attr) || !lpNumAttrsWritten)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        return FALSE;
    }

    *lpNumAttrsWritten = 0;

    SERVER_START_REQ( write_console_output )
    {
        req->handle = console_handle_unmap(hConsoleOutput);
        req->x      = coord.X;
        req->y      = coord.Y;
        req->mode   = CHAR_INFO_MODE_ATTR;
        req->wrap   = TRUE;
        wine_server_add_data( req, attr, length * sizeof(WORD) );
        if ((ret = !wine_server_call_err( req )))
            *lpNumAttrsWritten = reply->written;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 * FillConsoleOutputCharacterA [KERNEL32.@]
 *
 * See FillConsoleOutputCharacterW.
 */
BOOL WINAPI FillConsoleOutputCharacterA( HANDLE hConsoleOutput, CHAR ch, DWORD length,
                                         COORD coord, LPDWORD lpNumCharsWritten )
{
    WCHAR wch;

    MultiByteToWideChar( GetConsoleOutputCP(), 0, &ch, 1, &wch, 1 );
    return FillConsoleOutputCharacterW(hConsoleOutput, wch, length, coord, lpNumCharsWritten);
}


/******************************************************************************
 * FillConsoleOutputCharacterW [KERNEL32.@]  Writes characters to console
 *
 * PARAMS
 *    hConsoleOutput    [I] Handle to screen buffer
 *    ch                [I] Character to write
 *    length            [I] Number of cells to write to
 *    coord             [I] Coords of first cell
 *    lpNumCharsWritten [O] Pointer to number of cells written
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI FillConsoleOutputCharacterW( HANDLE hConsoleOutput, WCHAR ch, DWORD length,
                                         COORD coord, LPDWORD lpNumCharsWritten)
{
    BOOL ret;

    TRACE("(%p,%s,%d,(%dx%d),%p)\n",
          hConsoleOutput, debugstr_wn(&ch, 1), length, coord.X, coord.Y, lpNumCharsWritten);

    if (!lpNumCharsWritten)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        return FALSE;
    }

    *lpNumCharsWritten = 0;

    SERVER_START_REQ( fill_console_output )
    {
        req->handle  = console_handle_unmap(hConsoleOutput);
        req->x       = coord.X;
        req->y       = coord.Y;
        req->mode    = CHAR_INFO_MODE_TEXT;
        req->wrap    = TRUE;
        req->data.ch = ch;
        req->count   = length;
        if ((ret = !wine_server_call_err( req )))
            *lpNumCharsWritten = reply->written;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 * FillConsoleOutputAttribute [KERNEL32.@]  Sets attributes for console
 *
 * PARAMS
 *    hConsoleOutput    [I] Handle to screen buffer
 *    attr              [I] Color attribute to write
 *    length            [I] Number of cells to write to
 *    coord             [I] Coords of first cell
 *    lpNumAttrsWritten [O] Pointer to number of cells written
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI FillConsoleOutputAttribute( HANDLE hConsoleOutput, WORD attr, DWORD length,
                                        COORD coord, LPDWORD lpNumAttrsWritten )
{
    BOOL ret;

    TRACE("(%p,%d,%d,(%dx%d),%p)\n",
          hConsoleOutput, attr, length, coord.X, coord.Y, lpNumAttrsWritten);

    if (!lpNumAttrsWritten)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        return FALSE;
    }

    *lpNumAttrsWritten = 0;

    SERVER_START_REQ( fill_console_output )
    {
        req->handle    = console_handle_unmap(hConsoleOutput);
        req->x         = coord.X;
        req->y         = coord.Y;
        req->mode      = CHAR_INFO_MODE_ATTR;
        req->wrap      = TRUE;
        req->data.attr = attr;
        req->count     = length;
        if ((ret = !wine_server_call_err( req )))
            *lpNumAttrsWritten = reply->written;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 * ReadConsoleOutputCharacterA [KERNEL32.@]
 *
 */
BOOL WINAPI ReadConsoleOutputCharacterA(HANDLE hConsoleOutput, LPSTR lpstr, DWORD count,
                                        COORD coord, LPDWORD read_count)
{
    DWORD read;
    BOOL ret;
    LPWSTR wptr;

    if (!read_count)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        return FALSE;
    }

    *read_count = 0;

    if (!(wptr = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WCHAR))))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if ((ret = ReadConsoleOutputCharacterW( hConsoleOutput, wptr, count, coord, &read )))
    {
        read = WideCharToMultiByte( GetConsoleOutputCP(), 0, wptr, read, lpstr, count, NULL, NULL);
        *read_count = read;
    }
    HeapFree( GetProcessHeap(), 0, wptr );
    return ret;
}


/******************************************************************************
 * ReadConsoleOutputCharacterW [KERNEL32.@]
 *
 */
BOOL WINAPI ReadConsoleOutputCharacterW( HANDLE hConsoleOutput, LPWSTR buffer, DWORD count,
                                         COORD coord, LPDWORD read_count )
{
    BOOL ret;

    TRACE( "(%p,%p,%d,%dx%d,%p)\n", hConsoleOutput, buffer, count, coord.X, coord.Y, read_count );

    if (!read_count)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        return FALSE;
    }

    *read_count = 0;

    SERVER_START_REQ( read_console_output )
    {
        req->handle = console_handle_unmap(hConsoleOutput);
        req->x      = coord.X;
        req->y      = coord.Y;
        req->mode   = CHAR_INFO_MODE_TEXT;
        req->wrap   = TRUE;
        wine_server_set_reply( req, buffer, count * sizeof(WCHAR) );
        if ((ret = !wine_server_call_err( req )))
            *read_count = wine_server_reply_size(reply) / sizeof(WCHAR);
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *  ReadConsoleOutputAttribute [KERNEL32.@]
 */
BOOL WINAPI ReadConsoleOutputAttribute(HANDLE hConsoleOutput, LPWORD lpAttribute, DWORD length,
                                       COORD coord, LPDWORD read_count)
{
    BOOL ret;

    TRACE("(%p,%p,%d,%dx%d,%p)\n",
          hConsoleOutput, lpAttribute, length, coord.X, coord.Y, read_count);

    if (!read_count)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        return FALSE;
    }

    *read_count = 0;

    SERVER_START_REQ( read_console_output )
    {
        req->handle = console_handle_unmap(hConsoleOutput);
        req->x      = coord.X;
        req->y      = coord.Y;
        req->mode   = CHAR_INFO_MODE_ATTR;
        req->wrap   = TRUE;
        wine_server_set_reply( req, lpAttribute, length * sizeof(WORD) );
        if ((ret = !wine_server_call_err( req )))
            *read_count = wine_server_reply_size(reply) / sizeof(WORD);
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *  ReadConsoleOutputA [KERNEL32.@]
 *
 */
BOOL WINAPI ReadConsoleOutputA( HANDLE hConsoleOutput, LPCHAR_INFO lpBuffer, COORD size,
                                COORD coord, LPSMALL_RECT region )
{
    BOOL ret;
    int y;

    ret = ReadConsoleOutputW( hConsoleOutput, lpBuffer, size, coord, region );
    if (ret && region->Right >= region->Left)
    {
        for (y = 0; y <= region->Bottom - region->Top; y++)
        {
            char_info_WtoA( &lpBuffer[(coord.Y + y) * size.X + coord.X],
                            region->Right - region->Left + 1 );
        }
    }
    return ret;
}


/******************************************************************************
 *  ReadConsoleOutputW [KERNEL32.@]
 *
 * NOTE: The NT4 (sp5) kernel crashes on me if size is (0,0). I don't
 * think we need to be *that* compatible.  -- AJ
 */
BOOL WINAPI ReadConsoleOutputW( HANDLE hConsoleOutput, LPCHAR_INFO lpBuffer, COORD size,
                                COORD coord, LPSMALL_RECT region )
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
                req->handle = console_handle_unmap(hConsoleOutput);
                req->x      = region->Left;
                req->y      = region->Top + y;
                req->mode   = CHAR_INFO_MODE_TEXTATTR;
                req->wrap   = FALSE;
                wine_server_set_reply( req, &lpBuffer[(y+coord.Y) * size.X + coord.X],
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
 * ReadConsoleInputA [KERNEL32.@]  Reads data from a console
 *
 * PARAMS
 *    handle   [I] Handle to console input buffer
 *    buffer   [O] Address of buffer for read data
 *    count    [I] Number of records to read
 *    pRead    [O] Address of number of records read
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI ReadConsoleInputA( HANDLE handle, PINPUT_RECORD buffer, DWORD count, LPDWORD pRead )
{
    DWORD read;

    if (!ReadConsoleInputW( handle, buffer, count, &read )) return FALSE;
    input_records_WtoA( buffer, read );
    if (pRead) *pRead = read;
    return TRUE;
}


/***********************************************************************
 *            PeekConsoleInputA   (KERNEL32.@)
 *
 * Gets 'count' first events (or less) from input queue.
 */
BOOL WINAPI PeekConsoleInputA( HANDLE handle, PINPUT_RECORD buffer, DWORD count, LPDWORD pRead )
{
    DWORD read;

    if (!PeekConsoleInputW( handle, buffer, count, &read )) return FALSE;
    input_records_WtoA( buffer, read );
    if (pRead) *pRead = read;
    return TRUE;
}


/***********************************************************************
 *            PeekConsoleInputW   (KERNEL32.@)
 */
BOOL WINAPI PeekConsoleInputW( HANDLE handle, PINPUT_RECORD buffer, DWORD count, LPDWORD read )
{
    BOOL ret;
    SERVER_START_REQ( read_console_input )
    {
        req->handle = console_handle_unmap(handle);
        req->flush  = FALSE;
        wine_server_set_reply( req, buffer, count * sizeof(INPUT_RECORD) );
        if ((ret = !wine_server_call_err( req )))
        {
            if (read) *read = count ? reply->read : 0;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *            GetNumberOfConsoleInputEvents   (KERNEL32.@)
 */
BOOL WINAPI GetNumberOfConsoleInputEvents( HANDLE handle, LPDWORD nrofevents )
{
    BOOL ret;
    SERVER_START_REQ( read_console_input )
    {
        req->handle = console_handle_unmap(handle);
        req->flush  = FALSE;
        if ((ret = !wine_server_call_err( req )))
        {
            if (nrofevents)
                *nrofevents = reply->read;
            else
            {
                SetLastError(ERROR_INVALID_ACCESS);
                ret = FALSE;
            }
        }
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 * read_console_input
 *
 * Helper function for ReadConsole, ReadConsoleInput and FlushConsoleInputBuffer
 *
 * Returns
 *      0 for error, 1 for no INPUT_RECORD ready, 2 with INPUT_RECORD ready
 */
enum read_console_input_return {rci_error = 0, rci_timeout = 1, rci_gotone = 2};

static enum read_console_input_return bare_console_fetch_input(HANDLE handle, int fd, DWORD timeout)
{
    enum read_console_input_return      ret;
    char                                input[8];
    WCHAR                               inputw[8];
    int                                 i;
    size_t                              idx = 0, idxw;
    unsigned                            numEvent;
    INPUT_RECORD                        ir[8];
    DWORD                               written;
    struct pollfd                       pollfd;
    BOOL                                locked = FALSE, next_char;

    do
    {
        if (idx == sizeof(input))
        {
            FIXME("buffer too small (%s)\n", wine_dbgstr_an(input, idx));
            ret = rci_error;
            break;
        }
        pollfd.fd = fd;
        pollfd.events = POLLIN;
        pollfd.revents = 0;
        next_char = FALSE;

        switch (poll(&pollfd, 1, timeout))
        {
        case 1:
            if (!locked)
            {
                RtlEnterCriticalSection(&CONSOLE_CritSect);
                locked = TRUE;
            }
            i = read(fd, &input[idx], 1);
            if (i < 0)
            {
                ret = rci_error;
                break;
            }
            if (i == 0)
            {
                /* actually another thread likely beat us to reading the char
                 * return rci_gotone, while not perfect, it should work in most of the cases (as the new event
                 * should be now in the queue, fed from the other thread)
                 */
                ret = rci_gotone;
                break;
            }

            idx++;
            numEvent = TERM_FillInputRecord(input, idx, ir);
            switch (numEvent)
            {
            case 0:
                /* we need more char(s) to tell if it matches a key-db entry. wait 1/2s for next char */
                timeout = 500;
                next_char = TRUE;
                break;
            case -1:
                /* we haven't found the string into key-db, push full input string into server */
                idxw = MultiByteToWideChar(CP_UNIXCP, 0, input, idx, inputw, sizeof(inputw) / sizeof(inputw[0]));

                /* we cannot translate yet... likely we need more chars (wait max 1/2s for next char) */
                if (idxw == 0)
                {
                    timeout = 500;
                    next_char = TRUE;
                    break;
                }
                for (i = 0; i < idxw; i++)
                {
                    numEvent = TERM_FillSimpleChar(inputw[i], ir);
                    WriteConsoleInputW(handle, ir, numEvent, &written);
                }
                ret = rci_gotone;
                break;
            default:
                /* we got a transformation from key-db... push this into server */
                ret = WriteConsoleInputW(handle, ir, numEvent, &written) ? rci_gotone : rci_error;
                break;
            }
            break;
        case 0: ret = rci_timeout; break;
        default: ret = rci_error; break;
        }
    } while (next_char);
    if (locked) RtlLeaveCriticalSection(&CONSOLE_CritSect);

    return ret;
}

static enum read_console_input_return read_console_input(HANDLE handle, PINPUT_RECORD ir, DWORD timeout)
{
    int fd;
    enum read_console_input_return      ret;

    if ((fd = get_console_bare_fd(handle)) != -1)
    {
        put_console_into_raw_mode(fd);
        if (WaitForSingleObject(GetConsoleInputWaitHandle(), 0) != WAIT_OBJECT_0)
        {
            ret = bare_console_fetch_input(handle, fd, timeout);
        }
        else ret = rci_gotone;
        close(fd);
        if (ret != rci_gotone) return ret;
    }
    else
    {
        if (!VerifyConsoleIoHandle(handle)) return rci_error;

        if (WaitForSingleObject(GetConsoleInputWaitHandle(), timeout) != WAIT_OBJECT_0)
            return rci_timeout;
    }

    SERVER_START_REQ( read_console_input )
    {
        req->handle = console_handle_unmap(handle);
        req->flush = TRUE;
        wine_server_set_reply( req, ir, sizeof(INPUT_RECORD) );
        if (wine_server_call_err( req ) || !reply->read) ret = rci_error;
        else ret = rci_gotone;
    }
    SERVER_END_REQ;

    return ret;
}


/***********************************************************************
 *            FlushConsoleInputBuffer   (KERNEL32.@)
 */
BOOL WINAPI FlushConsoleInputBuffer( HANDLE handle )
{
    enum read_console_input_return      last;
    INPUT_RECORD                        ir;

    while ((last = read_console_input(handle, &ir, 0)) == rci_gotone);

    return last == rci_timeout;
}


/***********************************************************************
 *            SetConsoleTitleA   (KERNEL32.@)
 */
BOOL WINAPI SetConsoleTitleA( LPCSTR title )
{
    LPWSTR titleW;
    BOOL ret;

    DWORD len = MultiByteToWideChar( GetConsoleOutputCP(), 0, title, -1, NULL, 0 );
    if (!(titleW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR)))) return FALSE;
    MultiByteToWideChar( GetConsoleOutputCP(), 0, title, -1, titleW, len );
    ret = SetConsoleTitleW(titleW);
    HeapFree(GetProcessHeap(), 0, titleW);
    return ret;
}


/***********************************************************************
 *            GetConsoleKeyboardLayoutNameA   (KERNEL32.@)
 */
BOOL WINAPI GetConsoleKeyboardLayoutNameA(LPSTR layoutName)
{
    FIXME( "stub %p\n", layoutName);
    return TRUE;
}

/***********************************************************************
 *            GetConsoleKeyboardLayoutNameW   (KERNEL32.@)
 */
BOOL WINAPI GetConsoleKeyboardLayoutNameW(LPWSTR layoutName)
{
    static int once;
    if (!once++)
        FIXME( "stub %p\n", layoutName);
    return TRUE;
}

static WCHAR input_exe[MAX_PATH + 1];

/***********************************************************************
 *            GetConsoleInputExeNameW   (KERNEL32.@)
 */
BOOL WINAPI GetConsoleInputExeNameW(DWORD buflen, LPWSTR buffer)
{
    TRACE("%u %p\n", buflen, buffer);

    RtlEnterCriticalSection(&CONSOLE_CritSect);
    if (buflen > strlenW(input_exe)) strcpyW(buffer, input_exe);
    else SetLastError(ERROR_BUFFER_OVERFLOW);
    RtlLeaveCriticalSection(&CONSOLE_CritSect);

    return TRUE;
}

/***********************************************************************
 *            GetConsoleInputExeNameA   (KERNEL32.@)
 */
BOOL WINAPI GetConsoleInputExeNameA(DWORD buflen, LPSTR buffer)
{
    TRACE("%u %p\n", buflen, buffer);

    RtlEnterCriticalSection(&CONSOLE_CritSect);
    if (WideCharToMultiByte(CP_ACP, 0, input_exe, -1, NULL, 0, NULL, NULL) <= buflen)
        WideCharToMultiByte(CP_ACP, 0, input_exe, -1, buffer, buflen, NULL, NULL);
    else SetLastError(ERROR_BUFFER_OVERFLOW);
    RtlLeaveCriticalSection(&CONSOLE_CritSect);

    return TRUE;
}

/***********************************************************************
 *            GetConsoleTitleA   (KERNEL32.@)
 *
 * See GetConsoleTitleW.
 */
DWORD WINAPI GetConsoleTitleA(LPSTR title, DWORD size)
{
    WCHAR *ptr = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * size);
    DWORD ret;

    if (!ptr) return 0;
    ret = GetConsoleTitleW( ptr, size );
    if (ret)
    {
        WideCharToMultiByte( GetConsoleOutputCP(), 0, ptr, ret + 1, title, size, NULL, NULL);
        ret = strlen(title);
    }
    HeapFree(GetProcessHeap(), 0, ptr);
    return ret;
}


/******************************************************************************
 * GetConsoleTitleW [KERNEL32.@]  Retrieves title string for console
 *
 * PARAMS
 *    title [O] Address of buffer for title
 *    size  [I] Size of buffer
 *
 * RETURNS
 *    Success: Length of string copied
 *    Failure: 0
 */
DWORD WINAPI GetConsoleTitleW(LPWSTR title, DWORD size)
{
    DWORD ret = 0;

    SERVER_START_REQ( get_console_input_info )
    {
        req->handle = 0;
        wine_server_set_reply( req, title, (size-1) * sizeof(WCHAR) );
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
 *            GetLargestConsoleWindowSize   (KERNEL32.@)
 *
 * NOTE
 *	This should return a COORD, but calling convention for returning
 *      structures is different between Windows and gcc on i386.
 *
 * VERSION: [i386]
 */
#ifdef __i386__
#undef GetLargestConsoleWindowSize
DWORD WINAPI GetLargestConsoleWindowSize(HANDLE hConsoleOutput)
{
    union {
	COORD c;
	DWORD w;
    } x;
    x.c.X = 80;
    x.c.Y = 24;
    TRACE("(%p), returning %dx%d (%x)\n", hConsoleOutput, x.c.X, x.c.Y, x.w);
    return x.w;
}
#endif /* defined(__i386__) */


/***********************************************************************
 *            GetLargestConsoleWindowSize   (KERNEL32.@)
 *
 * NOTE
 *	This should return a COORD, but calling convention for returning
 *      structures is different between Windows and gcc on i386.
 *
 * VERSION: [!i386]
 */
#ifndef __i386__
COORD WINAPI GetLargestConsoleWindowSize(HANDLE hConsoleOutput)
{
    COORD c;
    c.X = 80;
    c.Y = 24;
    TRACE("(%p), returning %dx%d\n", hConsoleOutput, c.X, c.Y);
    return c;
}
#endif /* defined(__i386__) */

static WCHAR*	S_EditString /* = NULL */;
static unsigned S_EditStrPos /* = 0 */;

/***********************************************************************
 *            FreeConsole (KERNEL32.@)
 */
BOOL WINAPI FreeConsole(VOID)
{
    BOOL ret;

    /* invalidate local copy of input event handle */
    console_wait_event = 0;

    SERVER_START_REQ(free_console)
    {
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************
 *		start_console_renderer
 *
 * helper for AllocConsole
 * starts the renderer process
 */
static  BOOL    start_console_renderer_helper(const char* appname, STARTUPINFOA* si,
                                              HANDLE hEvent)
{
    char		buffer[1024];
    int                 ret;
    PROCESS_INFORMATION	pi;

    /* FIXME: use dynamic allocation for most of the buffers below */
    ret = snprintf(buffer, sizeof(buffer), "%s --use-event=%ld", appname, (DWORD_PTR)hEvent);
    if ((ret > -1) && (ret < sizeof(buffer)) &&
        CreateProcessA(NULL, buffer, NULL, NULL, TRUE, DETACHED_PROCESS,
                       NULL, NULL, si, &pi))
    {
        HANDLE  wh[2];
        DWORD   res;

        wh[0] = hEvent;
        wh[1] = pi.hProcess;
        res = WaitForMultipleObjects(2, wh, FALSE, INFINITE);

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        if (res != WAIT_OBJECT_0) return FALSE;

        TRACE("Started wineconsole pid=%08x tid=%08x\n",
              pi.dwProcessId, pi.dwThreadId);

        return TRUE;
    }
    return FALSE;
}

static	BOOL	start_console_renderer(STARTUPINFOA* si)
{
    HANDLE		hEvent = 0;
    LPSTR		p;
    OBJECT_ATTRIBUTES	attr;
    BOOL                ret = FALSE;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.Attributes               = OBJ_INHERIT;
    attr.ObjectName               = NULL;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;

    NtCreateEvent(&hEvent, EVENT_ALL_ACCESS, &attr, NotificationEvent, FALSE);
    if (!hEvent) return FALSE;

    /* first try environment variable */
    if ((p = getenv("WINECONSOLE")) != NULL)
    {
        ret = start_console_renderer_helper(p, si, hEvent);
        if (!ret)
            ERR("Couldn't launch Wine console from WINECONSOLE env var (%s)... "
                "trying default access\n", p);
    }

    /* then try the regular PATH */
    if (!ret)
        ret = start_console_renderer_helper("wineconsole", si, hEvent);

    CloseHandle(hEvent);
    return ret;
}

/***********************************************************************
 *            AllocConsole (KERNEL32.@)
 *
 * creates an xterm with a pty to our program
 */
BOOL WINAPI AllocConsole(void)
{
    HANDLE 		handle_in = INVALID_HANDLE_VALUE;
    HANDLE		handle_out = INVALID_HANDLE_VALUE;
    HANDLE 		handle_err = INVALID_HANDLE_VALUE;
    STARTUPINFOA        siCurrent;
    STARTUPINFOA	siConsole;
    char                buffer[1024];

    TRACE("()\n");

    handle_in = OpenConsoleW( coninW, GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,
                              FALSE, OPEN_EXISTING );

    if (VerifyConsoleIoHandle(handle_in))
    {
	/* we already have a console opened on this process, don't create a new one */
	CloseHandle(handle_in);
	return FALSE;
    }

    /* invalidate local copy of input event handle */
    console_wait_event = 0;

    GetStartupInfoA(&siCurrent);

    memset(&siConsole, 0, sizeof(siConsole));
    siConsole.cb = sizeof(siConsole);
    /* setup a view arguments for wineconsole (it'll use them as default values)  */
    if (siCurrent.dwFlags & STARTF_USECOUNTCHARS)
    {
        siConsole.dwFlags |= STARTF_USECOUNTCHARS;
        siConsole.dwXCountChars = siCurrent.dwXCountChars;
        siConsole.dwYCountChars = siCurrent.dwYCountChars;
    }
    if (siCurrent.dwFlags & STARTF_USEFILLATTRIBUTE)
    {
        siConsole.dwFlags |= STARTF_USEFILLATTRIBUTE;
        siConsole.dwFillAttribute = siCurrent.dwFillAttribute;
    }
    if (siCurrent.dwFlags & STARTF_USESHOWWINDOW)
    {
        siConsole.dwFlags |= STARTF_USESHOWWINDOW;
        siConsole.wShowWindow = siCurrent.wShowWindow;
    }
    /* FIXME (should pass the unicode form) */
    if (siCurrent.lpTitle)
        siConsole.lpTitle = siCurrent.lpTitle;
    else if (GetModuleFileNameA(0, buffer, sizeof(buffer)))
    {
        buffer[sizeof(buffer) - 1] = '\0';
        siConsole.lpTitle = buffer;
    }

    if (!start_console_renderer(&siConsole))
	goto the_end;

    if( !(siCurrent.dwFlags & STARTF_USESTDHANDLES) ) {
        /* all std I/O handles are inheritable by default */
        handle_in = OpenConsoleW( coninW, GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,
                                  TRUE, OPEN_EXISTING );
        if (handle_in == INVALID_HANDLE_VALUE) goto the_end;
  
        handle_out = OpenConsoleW( conoutW, GENERIC_READ|GENERIC_WRITE,
                                   TRUE, OPEN_EXISTING );
        if (handle_out == INVALID_HANDLE_VALUE) goto the_end;
  
        if (!DuplicateHandle(GetCurrentProcess(), handle_out, GetCurrentProcess(),
                    &handle_err, 0, TRUE, DUPLICATE_SAME_ACCESS))
            goto the_end;
    } else {
        /*  STARTF_USESTDHANDLES flag: use handles from StartupInfo */
        handle_in  =  siCurrent.hStdInput;
        handle_out =  siCurrent.hStdOutput;
        handle_err =  siCurrent.hStdError;
    }

    /* NT resets the STD_*_HANDLEs on console alloc */
    SetStdHandle(STD_INPUT_HANDLE,  handle_in);
    SetStdHandle(STD_OUTPUT_HANDLE, handle_out);
    SetStdHandle(STD_ERROR_HANDLE,  handle_err);

    SetLastError(ERROR_SUCCESS);

    return TRUE;

 the_end:
    ERR("Can't allocate console\n");
    if (handle_in != INVALID_HANDLE_VALUE)	CloseHandle(handle_in);
    if (handle_out != INVALID_HANDLE_VALUE)	CloseHandle(handle_out);
    if (handle_err != INVALID_HANDLE_VALUE)	CloseHandle(handle_err);
    FreeConsole();
    return FALSE;
}


/***********************************************************************
 *            ReadConsoleA   (KERNEL32.@)
 */
BOOL WINAPI ReadConsoleA(HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead,
			 LPDWORD lpNumberOfCharsRead, LPVOID lpReserved)
{
    LPWSTR	ptr = HeapAlloc(GetProcessHeap(), 0, nNumberOfCharsToRead * sizeof(WCHAR));
    DWORD	ncr = 0;
    BOOL	ret;

    if (!ptr)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if ((ret = ReadConsoleW(hConsoleInput, ptr, nNumberOfCharsToRead, &ncr, NULL)))
        ncr = WideCharToMultiByte(GetConsoleCP(), 0, ptr, ncr, lpBuffer, nNumberOfCharsToRead, NULL, NULL);

    if (lpNumberOfCharsRead) *lpNumberOfCharsRead = ncr;
    HeapFree(GetProcessHeap(), 0, ptr);

    return ret;
}

/***********************************************************************
 *            ReadConsoleW   (KERNEL32.@)
 */
BOOL WINAPI ReadConsoleW(HANDLE hConsoleInput, LPVOID lpBuffer,
			 DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, LPVOID lpReserved)
{
    DWORD	charsread;
    LPWSTR	xbuf = lpBuffer;
    DWORD	mode;
    BOOL        is_bare = FALSE;
    int         fd;

    TRACE("(%p,%p,%d,%p,%p)\n",
	  hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, lpReserved);

    if (!GetConsoleMode(hConsoleInput, &mode))
        return FALSE;
    if ((fd = get_console_bare_fd(hConsoleInput)) != -1)
    {
        close(fd);
        is_bare = TRUE;
    }
    if (mode & ENABLE_LINE_INPUT)
    {
	if (!S_EditString || S_EditString[S_EditStrPos] == 0)
	{
	    HeapFree(GetProcessHeap(), 0, S_EditString);
	    if (!(S_EditString = CONSOLE_Readline(hConsoleInput, !is_bare)))
		return FALSE;
	    S_EditStrPos = 0;
	}
	charsread = lstrlenW(&S_EditString[S_EditStrPos]);
	if (charsread > nNumberOfCharsToRead) charsread = nNumberOfCharsToRead;
	memcpy(xbuf, &S_EditString[S_EditStrPos], charsread * sizeof(WCHAR));
	S_EditStrPos += charsread;
    }
    else
    {
	INPUT_RECORD 	ir;
        DWORD           timeout = INFINITE;

	/* FIXME: should we read at least 1 char? The SDK does not say */
	/* wait for at least one available input record (it doesn't mean we'll have
	 * chars stored in xbuf...)
	 *
	 * Although SDK doc keeps silence about 1 char, SDK examples assume
	 * that we should wait for at least one character (not key). --KS
	 */
	charsread = 0;
        do 
	{
	    if (read_console_input(hConsoleInput, &ir, timeout) != rci_gotone) break;
	    if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown &&
		ir.Event.KeyEvent.uChar.UnicodeChar)
	    {
		xbuf[charsread++] = ir.Event.KeyEvent.uChar.UnicodeChar;
		timeout = 0;
	    }
        } while (charsread < nNumberOfCharsToRead);
        /* nothing has been read */
        if (timeout == INFINITE) return FALSE;
    }

    if (lpNumberOfCharsRead) *lpNumberOfCharsRead = charsread;

    return TRUE;
}


/***********************************************************************
 *            ReadConsoleInputW   (KERNEL32.@)
 */
BOOL WINAPI ReadConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer,
                              DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
    DWORD idx = 0;
    DWORD timeout = INFINITE;

    if (!nLength)
    {
        if (lpNumberOfEventsRead) *lpNumberOfEventsRead = 0;
        return TRUE;
    }

    /* loop until we get at least one event */
    while (read_console_input(hConsoleInput, &lpBuffer[idx], timeout) == rci_gotone &&
           ++idx < nLength)
        timeout = 0;

    if (lpNumberOfEventsRead) *lpNumberOfEventsRead = idx;
    return idx != 0;
}


/******************************************************************************
 * WriteConsoleOutputCharacterW [KERNEL32.@]
 * 
 * Copy character to consecutive cells in the console screen buffer.
 *
 * PARAMS
 *    hConsoleOutput    [I] Handle to screen buffer
 *    str               [I] Pointer to buffer with chars to write
 *    length            [I] Number of cells to write to
 *    coord             [I] Coords of first cell
 *    lpNumCharsWritten [O] Pointer to number of cells written
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 *
 */
BOOL WINAPI WriteConsoleOutputCharacterW( HANDLE hConsoleOutput, LPCWSTR str, DWORD length,
                                          COORD coord, LPDWORD lpNumCharsWritten )
{
    BOOL ret;

    TRACE("(%p,%s,%d,%dx%d,%p)\n", hConsoleOutput,
          debugstr_wn(str, length), length, coord.X, coord.Y, lpNumCharsWritten);

    if ((length > 0 && !str) || !lpNumCharsWritten)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        return FALSE;
    }

    *lpNumCharsWritten = 0;

    SERVER_START_REQ( write_console_output )
    {
        req->handle = console_handle_unmap(hConsoleOutput);
        req->x      = coord.X;
        req->y      = coord.Y;
        req->mode   = CHAR_INFO_MODE_TEXT;
        req->wrap   = TRUE;
        wine_server_add_data( req, str, length * sizeof(WCHAR) );
        if ((ret = !wine_server_call_err( req )))
            *lpNumCharsWritten = reply->written;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 * SetConsoleTitleW [KERNEL32.@]  Sets title bar string for console
 *
 * PARAMS
 *    title [I] Address of new title
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetConsoleTitleW(LPCWSTR title)
{
    BOOL ret;

    TRACE("(%s)\n", debugstr_w(title));
    SERVER_START_REQ( set_console_input_info )
    {
        req->handle = 0;
        req->mask = SET_CONSOLE_INPUT_INFO_TITLE;
        wine_server_add_data( req, title, strlenW(title) * sizeof(WCHAR) );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *            GetNumberOfConsoleMouseButtons   (KERNEL32.@)
 */
BOOL WINAPI GetNumberOfConsoleMouseButtons(LPDWORD nrofbuttons)
{
    FIXME("(%p): stub\n", nrofbuttons);
    *nrofbuttons = 2;
    return TRUE;
}

/******************************************************************************
 *  SetConsoleInputExeNameW	 [KERNEL32.@]
 */
BOOL WINAPI SetConsoleInputExeNameW(LPCWSTR name)
{
    TRACE("(%s)\n", debugstr_w(name));

    if (!name || !name[0])
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlEnterCriticalSection(&CONSOLE_CritSect);
    if (strlenW(name) < sizeof(input_exe)/sizeof(WCHAR)) strcpyW(input_exe, name);
    RtlLeaveCriticalSection(&CONSOLE_CritSect);

    return TRUE;
}

/******************************************************************************
 *  SetConsoleInputExeNameA	 [KERNEL32.@]
 */
BOOL WINAPI SetConsoleInputExeNameA(LPCSTR name)
{
    int len;
    LPWSTR nameW;
    BOOL ret;

    if (!name || !name[0])
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    len = MultiByteToWideChar(CP_ACP, 0, name, -1, NULL, 0);
    if (!(nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR)))) return FALSE;

    MultiByteToWideChar(CP_ACP, 0, name, -1, nameW, len);
    ret = SetConsoleInputExeNameW(nameW);
    HeapFree(GetProcessHeap(), 0, nameW);

    return ret;
}

/******************************************************************
 *		CONSOLE_DefaultHandler
 *
 * Final control event handler
 */
static BOOL WINAPI CONSOLE_DefaultHandler(DWORD dwCtrlType)
{
    FIXME("Terminating process %x on event %x\n", GetCurrentProcessId(), dwCtrlType);
    ExitProcess(0);
    /* should never go here */
    return TRUE;
}

/******************************************************************************
 * SetConsoleCtrlHandler [KERNEL32.@]  Adds function to calling process list
 *
 * PARAMS
 *    func [I] Address of handler function
 *    add  [I] Handler to add or remove
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */

struct ConsoleHandler
{
    PHANDLER_ROUTINE            handler;
    struct ConsoleHandler*      next;
};

static struct ConsoleHandler    CONSOLE_DefaultConsoleHandler = {CONSOLE_DefaultHandler, NULL};
static struct ConsoleHandler*   CONSOLE_Handlers = &CONSOLE_DefaultConsoleHandler;

/*****************************************************************************/

/******************************************************************
 *		SetConsoleCtrlHandler (KERNEL32.@)
 */
BOOL WINAPI SetConsoleCtrlHandler(PHANDLER_ROUTINE func, BOOL add)
{
    BOOL        ret = TRUE;

    TRACE("(%p,%i)\n", func, add);

    if (!func)
    {
        RtlEnterCriticalSection(&CONSOLE_CritSect);
        if (add)
            NtCurrentTeb()->Peb->ProcessParameters->ConsoleFlags |= 1;
        else
            NtCurrentTeb()->Peb->ProcessParameters->ConsoleFlags &= ~1;
        RtlLeaveCriticalSection(&CONSOLE_CritSect);
    }
    else if (add)
    {
        struct ConsoleHandler*  ch = HeapAlloc(GetProcessHeap(), 0, sizeof(struct ConsoleHandler));

        if (!ch) return FALSE;
        ch->handler = func;
        RtlEnterCriticalSection(&CONSOLE_CritSect);
        ch->next = CONSOLE_Handlers;
        CONSOLE_Handlers = ch;
        RtlLeaveCriticalSection(&CONSOLE_CritSect);
    }
    else
    {
        struct ConsoleHandler**  ch;
        RtlEnterCriticalSection(&CONSOLE_CritSect);
        for (ch = &CONSOLE_Handlers; *ch; ch = &(*ch)->next)
        {
            if ((*ch)->handler == func) break;
        }
        if (*ch)
        {
            struct ConsoleHandler*   rch = *ch;

            /* sanity check */
            if (rch == &CONSOLE_DefaultConsoleHandler)
            {
                ERR("Who's trying to remove default handler???\n");
                SetLastError(ERROR_INVALID_PARAMETER);
                ret = FALSE;
            }
            else
            {
                *ch = rch->next;
                HeapFree(GetProcessHeap(), 0, rch);
            }
        }
        else
        {
            WARN("Attempt to remove non-installed CtrlHandler %p\n", func);
            SetLastError(ERROR_INVALID_PARAMETER);
            ret = FALSE;
        }
        RtlLeaveCriticalSection(&CONSOLE_CritSect);
    }
    return ret;
}

static LONG WINAPI CONSOLE_CtrlEventHandler(EXCEPTION_POINTERS *eptr)
{
    TRACE("(%x)\n", eptr->ExceptionRecord->ExceptionCode);
    return EXCEPTION_EXECUTE_HANDLER;
}

/******************************************************************
 *		CONSOLE_SendEventThread
 *
 * Internal helper to pass an event to the list on installed handlers
 */
static DWORD WINAPI CONSOLE_SendEventThread(void* pmt)
{
    DWORD_PTR                   event = (DWORD_PTR)pmt;
    struct ConsoleHandler*      ch;

    if (event == CTRL_C_EVENT)
    {
        BOOL    caught_by_dbg = TRUE;
        /* First, try to pass the ctrl-C event to the debugger (if any)
         * If it continues, there's nothing more to do
         * Otherwise, we need to send the ctrl-C event to the handlers
         */
        __TRY
        {
            RaiseException( DBG_CONTROL_C, 0, 0, NULL );
        }
        __EXCEPT(CONSOLE_CtrlEventHandler)
        {
            caught_by_dbg = FALSE;
        }
        __ENDTRY;
        if (caught_by_dbg) return 0;
        /* the debugger didn't continue... so, pass to ctrl handlers */
    }
    RtlEnterCriticalSection(&CONSOLE_CritSect);
    for (ch = CONSOLE_Handlers; ch; ch = ch->next)
    {
        if (ch->handler(event)) break;
    }
    RtlLeaveCriticalSection(&CONSOLE_CritSect);
    return 1;
}

/******************************************************************
 *		CONSOLE_HandleCtrlC
 *
 * Check whether the shall manipulate CtrlC events
 */
int     CONSOLE_HandleCtrlC(unsigned sig)
{
    HANDLE thread;

    /* FIXME: better test whether a console is attached to this process ??? */
    extern    unsigned CONSOLE_GetNumHistoryEntries(void);
    if (CONSOLE_GetNumHistoryEntries() == (unsigned)-1) return 0;

    /* check if we have to ignore ctrl-C events */
    if (!(NtCurrentTeb()->Peb->ProcessParameters->ConsoleFlags & 1))
    {
        /* Create a separate thread to signal all the events. 
         * This is needed because:
         *  - this function can be called in an Unix signal handler (hence on an
         *    different stack than the thread that's running). This breaks the 
         *    Win32 exception mechanisms (where the thread's stack is checked).
         *  - since the current thread, while processing the signal, can hold the
         *    console critical section, we need another execution environment where
         *    we can wait on this critical section 
         */
        thread = CreateThread(NULL, 0, CONSOLE_SendEventThread, (void*)CTRL_C_EVENT, 0, NULL);
        if (thread == NULL)
            return 0;

        CloseHandle(thread);
    }
    return 1;
}

/******************************************************************************
 * GenerateConsoleCtrlEvent [KERNEL32.@] Simulate a CTRL-C or CTRL-BREAK
 *
 * PARAMS
 *    dwCtrlEvent        [I] Type of event
 *    dwProcessGroupID   [I] Process group ID to send event to
 *
 * RETURNS
 *    Success: True
 *    Failure: False (and *should* [but doesn't] set LastError)
 */
BOOL WINAPI GenerateConsoleCtrlEvent(DWORD dwCtrlEvent,
				     DWORD dwProcessGroupID)
{
    BOOL ret;

    TRACE("(%d, %d)\n", dwCtrlEvent, dwProcessGroupID);

    if (dwCtrlEvent != CTRL_C_EVENT && dwCtrlEvent != CTRL_BREAK_EVENT)
    {
	ERR("Invalid event %d for PGID %d\n", dwCtrlEvent, dwProcessGroupID);
	return FALSE;
    }

    SERVER_START_REQ( send_console_signal )
    {
        req->signal = dwCtrlEvent;
        req->group_id = dwProcessGroupID;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

    /* FIXME: Shall this function be synchronous, i.e., only return when all events
     * have been handled by all processes in the given group?
     * As of today, we don't wait...
     */
    return ret;
}


/******************************************************************************
 * CreateConsoleScreenBuffer [KERNEL32.@]  Creates a console screen buffer
 *
 * PARAMS
 *    dwDesiredAccess    [I] Access flag
 *    dwShareMode        [I] Buffer share mode
 *    sa                 [I] Security attributes
 *    dwFlags            [I] Type of buffer to create
 *    lpScreenBufferData [I] Reserved
 *
 * NOTES
 *    Should call SetLastError
 *
 * RETURNS
 *    Success: Handle to new console screen buffer
 *    Failure: INVALID_HANDLE_VALUE
 */
HANDLE WINAPI CreateConsoleScreenBuffer(DWORD dwDesiredAccess, DWORD dwShareMode,
					LPSECURITY_ATTRIBUTES sa, DWORD dwFlags,
					LPVOID lpScreenBufferData)
{
    HANDLE	ret = INVALID_HANDLE_VALUE;

    TRACE("(%d,%d,%p,%d,%p)\n",
	  dwDesiredAccess, dwShareMode, sa, dwFlags, lpScreenBufferData);

    if (dwFlags != CONSOLE_TEXTMODE_BUFFER || lpScreenBufferData != NULL)
    {
	SetLastError(ERROR_INVALID_PARAMETER);
	return INVALID_HANDLE_VALUE;
    }

    SERVER_START_REQ(create_console_output)
    {
        req->handle_in  = 0;
        req->access     = dwDesiredAccess;
        req->attributes = (sa && sa->bInheritHandle) ? OBJ_INHERIT : 0;
        req->share      = dwShareMode;
        req->fd         = -1;
        if (!wine_server_call_err( req ))
            ret = console_handle_map( wine_server_ptr_handle( reply->handle_out ));
    }
    SERVER_END_REQ;

    return ret;
}


/***********************************************************************
 *           GetConsoleScreenBufferInfo   (KERNEL32.@)
 */
BOOL WINAPI GetConsoleScreenBufferInfo(HANDLE hConsoleOutput, LPCONSOLE_SCREEN_BUFFER_INFO csbi)
{
    BOOL	ret;

    SERVER_START_REQ(get_console_output_info)
    {
        req->handle = console_handle_unmap(hConsoleOutput);
        if ((ret = !wine_server_call_err( req )))
        {
            csbi->dwSize.X              = reply->width;
            csbi->dwSize.Y              = reply->height;
            csbi->dwCursorPosition.X    = reply->cursor_x;
            csbi->dwCursorPosition.Y    = reply->cursor_y;
            csbi->wAttributes           = reply->attr;
            csbi->srWindow.Left         = reply->win_left;
            csbi->srWindow.Right        = reply->win_right;
            csbi->srWindow.Top          = reply->win_top;
            csbi->srWindow.Bottom       = reply->win_bottom;
            csbi->dwMaximumWindowSize.X = reply->max_width;
            csbi->dwMaximumWindowSize.Y = reply->max_height;
        }
    }
    SERVER_END_REQ;

    TRACE("(%p,(%d,%d) (%d,%d) %d (%d,%d-%d,%d) (%d,%d)\n", 
	  hConsoleOutput, csbi->dwSize.X, csbi->dwSize.Y,
	  csbi->dwCursorPosition.X, csbi->dwCursorPosition.Y,
	  csbi->wAttributes,
	  csbi->srWindow.Left, csbi->srWindow.Top, csbi->srWindow.Right, csbi->srWindow.Bottom,
	  csbi->dwMaximumWindowSize.X, csbi->dwMaximumWindowSize.Y);

    return ret;
}


/******************************************************************************
 * SetConsoleActiveScreenBuffer [KERNEL32.@]  Sets buffer to current console
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetConsoleActiveScreenBuffer(HANDLE hConsoleOutput)
{
    BOOL ret;

    TRACE("(%p)\n", hConsoleOutput);

    SERVER_START_REQ( set_console_input_info )
    {
        req->handle    = 0;
        req->mask      = SET_CONSOLE_INPUT_INFO_ACTIVE_SB;
        req->active_sb = wine_server_obj_handle( hConsoleOutput );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *            GetConsoleMode   (KERNEL32.@)
 */
BOOL WINAPI GetConsoleMode(HANDLE hcon, LPDWORD mode)
{
    BOOL ret;

    SERVER_START_REQ( get_console_mode )
    {
        req->handle = console_handle_unmap(hcon);
        if ((ret = !wine_server_call_err( req )))
        {
            if (mode) *mode = reply->mode;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 * SetConsoleMode [KERNEL32.@]  Sets input mode of console's input buffer
 *
 * PARAMS
 *    hcon [I] Handle to console input or screen buffer
 *    mode [I] Input or output mode to set
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 *
 *    mode:
 *      ENABLE_PROCESSED_INPUT	0x01
 *      ENABLE_LINE_INPUT	0x02
 *      ENABLE_ECHO_INPUT	0x04
 *      ENABLE_WINDOW_INPUT	0x08
 *      ENABLE_MOUSE_INPUT	0x10
 */
BOOL WINAPI SetConsoleMode(HANDLE hcon, DWORD mode)
{
    BOOL ret;

    SERVER_START_REQ(set_console_mode)
    {
	req->handle = console_handle_unmap(hcon);
	req->mode = mode;
	ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    /* FIXME: when resetting a console input to editline mode, I think we should
     * empty the S_EditString buffer
     */

    TRACE("(%p,%x) retval == %d\n", hcon, mode, ret);

    return ret;
}


/******************************************************************
 *		CONSOLE_WriteChars
 *
 * WriteConsoleOutput helper: hides server call semantics
 * writes a string at a given pos with standard attribute
 */
static int CONSOLE_WriteChars(HANDLE hCon, LPCWSTR lpBuffer, int nc, COORD* pos)
{
    int written = -1;

    if (!nc) return 0;

    SERVER_START_REQ( write_console_output )
    {
        req->handle = console_handle_unmap(hCon);
        req->x      = pos->X;
        req->y      = pos->Y;
        req->mode   = CHAR_INFO_MODE_TEXTSTDATTR;
        req->wrap   = FALSE;
        wine_server_add_data( req, lpBuffer, nc * sizeof(WCHAR) );
        if (!wine_server_call_err( req )) written = reply->written;
    }
    SERVER_END_REQ;

    if (written > 0) pos->X += written;
    return written;
}

/******************************************************************
 *		next_line
 *
 * WriteConsoleOutput helper: handles passing to next line (+scrolling if necessary)
 *
 */
static BOOL next_line(HANDLE hCon, CONSOLE_SCREEN_BUFFER_INFO* csbi)
{
    SMALL_RECT	src;
    CHAR_INFO	ci;
    COORD	dst;

    csbi->dwCursorPosition.X = 0;
    csbi->dwCursorPosition.Y++;

    if (csbi->dwCursorPosition.Y < csbi->dwSize.Y) return TRUE;

    src.Top    = 1;
    src.Bottom = csbi->dwSize.Y - 1;
    src.Left   = 0;
    src.Right  = csbi->dwSize.X - 1;

    dst.X      = 0;
    dst.Y      = 0;

    ci.Attributes = csbi->wAttributes;
    ci.Char.UnicodeChar = ' ';

    csbi->dwCursorPosition.Y--;
    if (!ScrollConsoleScreenBufferW(hCon, &src, NULL, dst, &ci))
        return FALSE;
    return TRUE;
}

/******************************************************************
 *		write_block
 *
 * WriteConsoleOutput helper: writes a block of non special characters
 * Block can spread on several lines, and wrapping, if needed, is
 * handled
 *
 */
static BOOL write_block(HANDLE hCon, CONSOLE_SCREEN_BUFFER_INFO* csbi,
                        DWORD mode, LPCWSTR ptr, int len)
{
    int	blk;	/* number of chars to write on current line */
    int done;   /* number of chars already written */

    if (len <= 0) return TRUE;

    if (mode & ENABLE_WRAP_AT_EOL_OUTPUT) /* writes remaining on next line */
    {
        for (done = 0; done < len; done += blk)
        {
            blk = min(len - done, csbi->dwSize.X - csbi->dwCursorPosition.X);

            if (CONSOLE_WriteChars(hCon, ptr + done, blk, &csbi->dwCursorPosition) != blk)
                return FALSE;
            if (csbi->dwCursorPosition.X == csbi->dwSize.X && !next_line(hCon, csbi))
                return FALSE;
        }
    }
    else
    {
        int     pos = csbi->dwCursorPosition.X;
        /* FIXME: we could reduce the number of loops
         * but, in most cases we wouldn't gain lots of time (it would only
         * happen if we're asked to overwrite more than twice the part of the line,
         * which is unlikely
         */
        for (done = 0; done < len; done += blk)
        {
            blk = min(len - done, csbi->dwSize.X - csbi->dwCursorPosition.X);

            csbi->dwCursorPosition.X = pos;
            if (CONSOLE_WriteChars(hCon, ptr + done, blk, &csbi->dwCursorPosition) != blk)
                return FALSE;
        }
    }

    return TRUE;
}

/***********************************************************************
 *            WriteConsoleW   (KERNEL32.@)
 */
BOOL WINAPI WriteConsoleW(HANDLE hConsoleOutput, LPCVOID lpBuffer, DWORD nNumberOfCharsToWrite,
			  LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved)
{
    DWORD			mode;
    DWORD			nw = 0;
    const WCHAR*		psz = lpBuffer;
    CONSOLE_SCREEN_BUFFER_INFO	csbi;
    int				k, first = 0, fd;

    TRACE("%p %s %d %p %p\n",
	  hConsoleOutput, debugstr_wn(lpBuffer, nNumberOfCharsToWrite),
	  nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);

    if (lpNumberOfCharsWritten) *lpNumberOfCharsWritten = 0;

    if ((fd = get_console_bare_fd(hConsoleOutput)) != -1)
    {
        char*           ptr;
        unsigned        len;
        HANDLE          hFile;
        NTSTATUS        status;
        IO_STATUS_BLOCK iosb;

        close(fd);
        /* FIXME: mode ENABLED_OUTPUT is not processed (or actually we rely on underlying Unix/TTY fd
         * to do the job
         */
        len = WideCharToMultiByte(CP_UNIXCP, 0, lpBuffer, nNumberOfCharsToWrite, NULL, 0, NULL, NULL);
        if ((ptr = HeapAlloc(GetProcessHeap(), 0, len)) == NULL)
            return FALSE;

        WideCharToMultiByte(CP_UNIXCP, 0, lpBuffer, nNumberOfCharsToWrite, ptr, len, NULL, NULL);
        hFile = wine_server_ptr_handle(console_handle_unmap(hConsoleOutput));
        status = NtWriteFile(hFile, NULL, NULL, NULL, &iosb, ptr, len, 0, NULL);
        if (status == STATUS_PENDING)
        {
            WaitForSingleObject(hFile, INFINITE);
            status = iosb.u.Status;
        }

        if (status != STATUS_PENDING && lpNumberOfCharsWritten)
        {
            if (iosb.Information == len)
                *lpNumberOfCharsWritten = nNumberOfCharsToWrite;
            else
                FIXME("Conversion not supported yet\n");
        }
        HeapFree(GetProcessHeap(), 0, ptr);
        if (status != STATUS_SUCCESS)
        {
            SetLastError(RtlNtStatusToDosError(status));
            return FALSE;
        }
        return TRUE;
    }

    if (!GetConsoleMode(hConsoleOutput, &mode) || !GetConsoleScreenBufferInfo(hConsoleOutput, &csbi))
	return FALSE;

    if (!nNumberOfCharsToWrite) return TRUE;

    if (mode & ENABLE_PROCESSED_OUTPUT)
    {
	unsigned int	i;

	for (i = 0; i < nNumberOfCharsToWrite; i++)
	{
	    switch (psz[i])
	    {
	    case '\b': case '\t': case '\n': case '\a': case '\r':
		/* don't handle here the i-th char... done below */
		if ((k = i - first) > 0)
		{
		    if (!write_block(hConsoleOutput, &csbi, mode, &psz[first], k))
			goto the_end;
		    nw += k;
		}
		first = i + 1;
		nw++;
	    }
	    switch (psz[i])
	    {
	    case '\b':
		if (csbi.dwCursorPosition.X > 0) csbi.dwCursorPosition.X--;
		break;
	    case '\t':
	        {
		    static const WCHAR tmp[] = {' ',' ',' ',' ',' ',' ',' ',' '};
		    if (!write_block(hConsoleOutput, &csbi, mode, tmp,
				     ((csbi.dwCursorPosition.X + 8) & ~7) - csbi.dwCursorPosition.X))
			goto the_end;
		}
		break;
	    case '\n':
		next_line(hConsoleOutput, &csbi);
		break;
 	    case '\a':
		Beep(400, 300);
 		break;
	    case '\r':
		csbi.dwCursorPosition.X = 0;
		break;
	    default:
		break;
	    }
	}
    }

    /* write the remaining block (if any) if processed output is enabled, or the
     * entire buffer otherwise
     */
    if ((k = nNumberOfCharsToWrite - first) > 0)
    {
	if (!write_block(hConsoleOutput, &csbi, mode, &psz[first], k))
	    goto the_end;
	nw += k;
    }

 the_end:
    SetConsoleCursorPosition(hConsoleOutput, csbi.dwCursorPosition);
    if (lpNumberOfCharsWritten) *lpNumberOfCharsWritten = nw;
    return nw != 0;
}


/***********************************************************************
 *            WriteConsoleA   (KERNEL32.@)
 */
BOOL WINAPI WriteConsoleA(HANDLE hConsoleOutput, LPCVOID lpBuffer, DWORD nNumberOfCharsToWrite,
			  LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved)
{
    BOOL	ret;
    LPWSTR	xstring;
    DWORD 	n;

    n = MultiByteToWideChar(GetConsoleOutputCP(), 0, lpBuffer, nNumberOfCharsToWrite, NULL, 0);

    if (lpNumberOfCharsWritten) *lpNumberOfCharsWritten = 0;
    xstring = HeapAlloc(GetProcessHeap(), 0, n * sizeof(WCHAR));
    if (!xstring) return FALSE;

    MultiByteToWideChar(GetConsoleOutputCP(), 0, lpBuffer, nNumberOfCharsToWrite, xstring, n);

    ret = WriteConsoleW(hConsoleOutput, xstring, n, lpNumberOfCharsWritten, 0);

    HeapFree(GetProcessHeap(), 0, xstring);

    return ret;
}

/******************************************************************************
 * SetConsoleCursorPosition [KERNEL32.@]
 * Sets the cursor position in console
 *
 * PARAMS
 *    hConsoleOutput   [I] Handle of console screen buffer
 *    dwCursorPosition [I] New cursor position coordinates
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetConsoleCursorPosition(HANDLE hcon, COORD pos)
{
    BOOL 			ret;
    CONSOLE_SCREEN_BUFFER_INFO	csbi;
    int				do_move = 0;
    int				w, h;

    TRACE("%p %d %d\n", hcon, pos.X, pos.Y);

    SERVER_START_REQ(set_console_output_info)
    {
        req->handle         = console_handle_unmap(hcon);
        req->cursor_x       = pos.X;
        req->cursor_y       = pos.Y;
        req->mask           = SET_CONSOLE_OUTPUT_INFO_CURSOR_POS;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

    if (!ret || !GetConsoleScreenBufferInfo(hcon, &csbi))
	return FALSE;

    /* if cursor is no longer visible, scroll the visible window... */
    w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    if (pos.X < csbi.srWindow.Left)
    {
	csbi.srWindow.Left   = min(pos.X, csbi.dwSize.X - w);
	do_move++;
    }
    else if (pos.X > csbi.srWindow.Right)
    {
	csbi.srWindow.Left   = max(pos.X, w) - w + 1;
	do_move++;
    }
    csbi.srWindow.Right  = csbi.srWindow.Left + w - 1;

    if (pos.Y < csbi.srWindow.Top)
    {
	csbi.srWindow.Top    = min(pos.Y, csbi.dwSize.Y - h);
	do_move++;
    }
    else if (pos.Y > csbi.srWindow.Bottom)
    {
	csbi.srWindow.Top   = max(pos.Y, h) - h + 1;
	do_move++;
    }
    csbi.srWindow.Bottom = csbi.srWindow.Top + h - 1;

    ret = (do_move) ? SetConsoleWindowInfo(hcon, TRUE, &csbi.srWindow) : TRUE;

    return ret;
}

/******************************************************************************
 * GetConsoleCursorInfo [KERNEL32.@]  Gets size and visibility of console
 *
 * PARAMS
 *    hcon  [I] Handle to console screen buffer
 *    cinfo [O] Address of cursor information
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetConsoleCursorInfo(HANDLE hCon, LPCONSOLE_CURSOR_INFO cinfo)
{
    BOOL ret;

    SERVER_START_REQ(get_console_output_info)
    {
        req->handle = console_handle_unmap(hCon);
        ret = !wine_server_call_err( req );
        if (ret && cinfo)
        {
            cinfo->dwSize = reply->cursor_size;
            cinfo->bVisible = reply->cursor_visible;
        }
    }
    SERVER_END_REQ;

    if (!ret) return FALSE;

    if (!cinfo)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        ret = FALSE;
    }
    else TRACE("(%p) returning (%d,%d)\n", hCon, cinfo->dwSize, cinfo->bVisible);

    return ret;
}


/******************************************************************************
 * SetConsoleCursorInfo [KERNEL32.@]  Sets size and visibility of cursor
 *
 * PARAMS
 * 	hcon	[I] Handle to console screen buffer
 * 	cinfo	[I] Address of cursor information
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetConsoleCursorInfo(HANDLE hCon, LPCONSOLE_CURSOR_INFO cinfo)
{
    BOOL ret;

    TRACE("(%p,%d,%d)\n", hCon, cinfo->dwSize, cinfo->bVisible);
    SERVER_START_REQ(set_console_output_info)
    {
        req->handle         = console_handle_unmap(hCon);
        req->cursor_size    = cinfo->dwSize;
        req->cursor_visible = cinfo->bVisible;
        req->mask           = SET_CONSOLE_OUTPUT_INFO_CURSOR_GEOM;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 * SetConsoleWindowInfo [KERNEL32.@]  Sets size and position of console
 *
 * PARAMS
 *	hcon	        [I] Handle to console screen buffer
 *	bAbsolute	[I] Coordinate type flag
 *	window		[I] Address of new window rectangle
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetConsoleWindowInfo(HANDLE hCon, BOOL bAbsolute, LPSMALL_RECT window)
{
    SMALL_RECT	p = *window;
    BOOL	ret;

    TRACE("(%p,%d,(%d,%d-%d,%d))\n", hCon, bAbsolute, p.Left, p.Top, p.Right, p.Bottom);

    if (!bAbsolute)
    {
	CONSOLE_SCREEN_BUFFER_INFO	csbi;

	if (!GetConsoleScreenBufferInfo(hCon, &csbi))
	    return FALSE;
	p.Left   += csbi.srWindow.Left;
	p.Top    += csbi.srWindow.Top;
	p.Right  += csbi.srWindow.Right;
	p.Bottom += csbi.srWindow.Bottom;
    }
    SERVER_START_REQ(set_console_output_info)
    {
        req->handle         = console_handle_unmap(hCon);
	req->win_left       = p.Left;
	req->win_top        = p.Top;
	req->win_right      = p.Right;
	req->win_bottom     = p.Bottom;
        req->mask           = SET_CONSOLE_OUTPUT_INFO_DISPLAY_WINDOW;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

    return ret;
}


/******************************************************************************
 * SetConsoleTextAttribute [KERNEL32.@]  Sets colors for text
 *
 * Sets the foreground and background color attributes of characters
 * written to the screen buffer.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetConsoleTextAttribute(HANDLE hConsoleOutput, WORD wAttr)
{
    BOOL ret;

    TRACE("(%p,%d)\n", hConsoleOutput, wAttr);
    SERVER_START_REQ(set_console_output_info)
    {
        req->handle = console_handle_unmap(hConsoleOutput);
        req->attr   = wAttr;
        req->mask   = SET_CONSOLE_OUTPUT_INFO_ATTR;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 * SetConsoleScreenBufferSize [KERNEL32.@]  Changes size of console
 *
 * PARAMS
 *    hConsoleOutput [I] Handle to console screen buffer
 *    dwSize         [I] New size in character rows and cols
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetConsoleScreenBufferSize(HANDLE hConsoleOutput, COORD dwSize)
{
    BOOL ret;

    TRACE("(%p,(%d,%d))\n", hConsoleOutput, dwSize.X, dwSize.Y);
    SERVER_START_REQ(set_console_output_info)
    {
        req->handle = console_handle_unmap(hConsoleOutput);
        req->width  = dwSize.X;
        req->height = dwSize.Y;
        req->mask   = SET_CONSOLE_OUTPUT_INFO_SIZE;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 * ScrollConsoleScreenBufferA [KERNEL32.@]
 *
 */
BOOL WINAPI ScrollConsoleScreenBufferA(HANDLE hConsoleOutput, LPSMALL_RECT lpScrollRect,
				       LPSMALL_RECT lpClipRect, COORD dwDestOrigin,
				       LPCHAR_INFO lpFill)
{
    CHAR_INFO	ciw;

    ciw.Attributes = lpFill->Attributes;
    MultiByteToWideChar(GetConsoleOutputCP(), 0, &lpFill->Char.AsciiChar, 1, &ciw.Char.UnicodeChar, 1);

    return ScrollConsoleScreenBufferW(hConsoleOutput, lpScrollRect, lpClipRect,
				      dwDestOrigin, &ciw);
}

/******************************************************************
 *		CONSOLE_FillLineUniform
 *
 * Helper function for ScrollConsoleScreenBufferW
 * Fills a part of a line with a constant character info
 */
void CONSOLE_FillLineUniform(HANDLE hConsoleOutput, int i, int j, int len, LPCHAR_INFO lpFill)
{
    SERVER_START_REQ( fill_console_output )
    {
        req->handle    = console_handle_unmap(hConsoleOutput);
        req->mode      = CHAR_INFO_MODE_TEXTATTR;
        req->x         = i;
        req->y         = j;
        req->count     = len;
        req->wrap      = FALSE;
        req->data.ch   = lpFill->Char.UnicodeChar;
        req->data.attr = lpFill->Attributes;
        wine_server_call_err( req );
    }
    SERVER_END_REQ;
}

/******************************************************************************
 * ScrollConsoleScreenBufferW [KERNEL32.@]
 *
 */

BOOL WINAPI ScrollConsoleScreenBufferW(HANDLE hConsoleOutput, LPSMALL_RECT lpScrollRect,
				       LPSMALL_RECT lpClipRect, COORD dwDestOrigin,
				       LPCHAR_INFO lpFill)
{
    SMALL_RECT			dst;
    DWORD			ret;
    int				i, j;
    int				start = -1;
    SMALL_RECT			clip;
    CONSOLE_SCREEN_BUFFER_INFO	csbi;
    BOOL			inside;
    COORD                       src;

    if (lpClipRect)
	TRACE("(%p,(%d,%d-%d,%d),(%d,%d-%d,%d),%d-%d,%p)\n", hConsoleOutput,
	      lpScrollRect->Left, lpScrollRect->Top,
	      lpScrollRect->Right, lpScrollRect->Bottom,
	      lpClipRect->Left, lpClipRect->Top,
	      lpClipRect->Right, lpClipRect->Bottom,
	      dwDestOrigin.X, dwDestOrigin.Y, lpFill);
    else
	TRACE("(%p,(%d,%d-%d,%d),(nil),%d-%d,%p)\n", hConsoleOutput,
	      lpScrollRect->Left, lpScrollRect->Top,
	      lpScrollRect->Right, lpScrollRect->Bottom,
	      dwDestOrigin.X, dwDestOrigin.Y, lpFill);

    if (!GetConsoleScreenBufferInfo(hConsoleOutput, &csbi))
	return FALSE;

    src.X = lpScrollRect->Left;
    src.Y = lpScrollRect->Top;

    /* step 1: get dst rect */
    dst.Left = dwDestOrigin.X;
    dst.Top = dwDestOrigin.Y;
    dst.Right = dst.Left + (lpScrollRect->Right - lpScrollRect->Left);
    dst.Bottom = dst.Top + (lpScrollRect->Bottom - lpScrollRect->Top);

    /* step 2a: compute the final clip rect (optional passed clip and screen buffer limits */
    if (lpClipRect)
    {
	clip.Left   = max(0, lpClipRect->Left);
	clip.Right  = min(csbi.dwSize.X - 1, lpClipRect->Right);
	clip.Top    = max(0, lpClipRect->Top);
	clip.Bottom = min(csbi.dwSize.Y - 1, lpClipRect->Bottom);
    }
    else
    {
	clip.Left   = 0;
	clip.Right  = csbi.dwSize.X - 1;
	clip.Top    = 0;
	clip.Bottom = csbi.dwSize.Y - 1;
    }
    if (clip.Left > clip.Right || clip.Top > clip.Bottom) return FALSE;

    /* step 2b: clip dst rect */
    if (dst.Left   < clip.Left  ) {src.X += clip.Left - dst.Left; dst.Left   = clip.Left;}
    if (dst.Top    < clip.Top   ) {src.Y += clip.Top  - dst.Top;  dst.Top    = clip.Top;}
    if (dst.Right  > clip.Right ) dst.Right  = clip.Right;
    if (dst.Bottom > clip.Bottom) dst.Bottom = clip.Bottom;

    /* step 3: transfer the bits */
    SERVER_START_REQ(move_console_output)
    {
        req->handle = console_handle_unmap(hConsoleOutput);
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
    for (j = max(lpScrollRect->Top, clip.Top); j <= min(lpScrollRect->Bottom, clip.Bottom); j++)
    {
	inside = dst.Top <= j && j <= dst.Bottom;
	start = -1;
	for (i = max(lpScrollRect->Left, clip.Left); i <= min(lpScrollRect->Right, clip.Right); i++)
	{
	    if (inside && dst.Left <= i && i <= dst.Right)
	    {
		if (start != -1)
		{
		    CONSOLE_FillLineUniform(hConsoleOutput, start, j, i - start, lpFill);
		    start = -1;
		}
	    }
	    else
	    {
		if (start == -1) start = i;
	    }
	}
	if (start != -1)
	    CONSOLE_FillLineUniform(hConsoleOutput, start, j, i - start, lpFill);
    }

    return TRUE;
}

/******************************************************************
 *              AttachConsole  (KERNEL32.@)
 */
BOOL WINAPI AttachConsole(DWORD dwProcessId)
{
    FIXME("stub %x\n",dwProcessId);
    return TRUE;
}

/******************************************************************
 *              GetConsoleDisplayMode  (KERNEL32.@)
 */
BOOL WINAPI GetConsoleDisplayMode(LPDWORD lpModeFlags)
{
    TRACE("semi-stub: %p\n", lpModeFlags);
    /* It is safe to successfully report windowed mode */
    *lpModeFlags = 0;
    return TRUE;
}

/******************************************************************
 *              SetConsoleDisplayMode  (KERNEL32.@)
 */
BOOL WINAPI SetConsoleDisplayMode(HANDLE hConsoleOutput, DWORD dwFlags,
                                  COORD *lpNewScreenBufferDimensions)
{
    TRACE("(%p, %x, (%d, %d))\n", hConsoleOutput, dwFlags,
          lpNewScreenBufferDimensions->X, lpNewScreenBufferDimensions->Y);
    if (dwFlags == 1)
    {
        /* We cannot switch to fullscreen */
        return FALSE;
    }
    return TRUE;
}


/* ====================================================================
 *
 * Console manipulation functions
 *
 * ====================================================================*/

/* some missing functions...
 * FIXME: those are likely to be defined as undocumented function in kernel32 (or part of them)
 * should get the right API and implement them
 *	SetConsoleCommandHistoryMode
 *	SetConsoleNumberOfCommands[AW]
 */
int CONSOLE_GetHistory(int idx, WCHAR* buf, int buf_len)
{
    int len = 0;

    SERVER_START_REQ( get_console_input_history )
    {
        req->handle = 0;
        req->index = idx;
        if (buf && buf_len > 1)
        {
            wine_server_set_reply( req, buf, (buf_len - 1) * sizeof(WCHAR) );
        }
        if (!wine_server_call_err( req ))
        {
            if (buf) buf[wine_server_reply_size(reply) / sizeof(WCHAR)] = 0;
            len = reply->total / sizeof(WCHAR) + 1;
        }
    }
    SERVER_END_REQ;
    return len;
}

/******************************************************************
 *		CONSOLE_AppendHistory
 *
 *
 */
BOOL	CONSOLE_AppendHistory(const WCHAR* ptr)
{
    size_t	len = strlenW(ptr);
    BOOL	ret;

    while (len && (ptr[len - 1] == '\n' || ptr[len - 1] == '\r')) len--;
    if (!len) return FALSE;

    SERVER_START_REQ( append_console_input_history )
    {
        req->handle = 0;
        wine_server_add_data( req, ptr, len * sizeof(WCHAR) );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************
 *		CONSOLE_GetNumHistoryEntries
 *
 *
 */
unsigned CONSOLE_GetNumHistoryEntries(void)
{
    unsigned ret = -1;
    SERVER_START_REQ(get_console_input_info)
    {
        req->handle = 0;
        if (!wine_server_call_err( req )) ret = reply->history_index;
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************
 *		CONSOLE_GetEditionMode
 *
 *
 */
BOOL CONSOLE_GetEditionMode(HANDLE hConIn, int* mode)
{
    unsigned ret = 0;
    SERVER_START_REQ(get_console_input_info)
    {
        req->handle = console_handle_unmap(hConIn);
        if ((ret = !wine_server_call_err( req )))
            *mode = reply->edition_mode;
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************
 *              GetConsoleAliasW
 *
 *
 * RETURNS
 *    0 if an error occurred, non-zero for success
 *
 */
DWORD WINAPI GetConsoleAliasW(LPWSTR lpSource, LPWSTR lpTargetBuffer,
                              DWORD TargetBufferLength, LPWSTR lpExename)
{
    FIXME("(%s,%p,%d,%s): stub\n", debugstr_w(lpSource), lpTargetBuffer, TargetBufferLength, debugstr_w(lpExename));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/******************************************************************
 *              GetConsoleProcessList  (KERNEL32.@)
 */
DWORD WINAPI GetConsoleProcessList(LPDWORD processlist, DWORD processcount)
{
    FIXME("(%p,%d): stub\n", processlist, processcount);

    if (!processlist || processcount < 1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    return 0;
}

BOOL CONSOLE_Init(RTL_USER_PROCESS_PARAMETERS *params)
{
    memset(&S_termios, 0, sizeof(S_termios));
    if (params->ConsoleHandle == KERNEL32_CONSOLE_SHELL)
    {
        HANDLE  conin;

        /* FIXME: to be done even if program is a GUI ? */
        /* This is wine specific: we have no parent (we're started from unix)
         * so, create a simple console with bare handles
         */
        TERM_Init();
        wine_server_send_fd(0);
        SERVER_START_REQ( alloc_console )
        {
            req->access     = GENERIC_READ | GENERIC_WRITE;
            req->attributes = OBJ_INHERIT;
            req->pid        = 0xffffffff;
            req->input_fd   = 0;
            wine_server_call( req );
            conin = wine_server_ptr_handle( reply->handle_in );
            /* reply->event shouldn't be created by server */
        }
        SERVER_END_REQ;

        if (!params->hStdInput)
            params->hStdInput = conin;

        if (!params->hStdOutput)
        {
            wine_server_send_fd(1);
            SERVER_START_REQ( create_console_output )
            {
                req->handle_in  = wine_server_obj_handle(conin);
                req->access     = GENERIC_WRITE|GENERIC_READ;
                req->attributes = OBJ_INHERIT;
                req->share      = FILE_SHARE_READ|FILE_SHARE_WRITE;
                req->fd         = 1;
                wine_server_call(req);
                params->hStdOutput = wine_server_ptr_handle(reply->handle_out);
            }
            SERVER_END_REQ;
        }
        if (!params->hStdError)
        {
            wine_server_send_fd(2);
            SERVER_START_REQ( create_console_output )
            {
                req->handle_in  = wine_server_obj_handle(conin);
                req->access     = GENERIC_WRITE|GENERIC_READ;
                req->attributes = OBJ_INHERIT;
                req->share      = FILE_SHARE_READ|FILE_SHARE_WRITE;
                req->fd         = 2;
                wine_server_call(req);
                params->hStdError = wine_server_ptr_handle(reply->handle_out);
            }
            SERVER_END_REQ;
        }
    }

    /* convert value from server:
     * + 0 => INVALID_HANDLE_VALUE
     * + console handle needs to be mapped
     */
    if (!params->hStdInput)
        params->hStdInput = INVALID_HANDLE_VALUE;
    else if (VerifyConsoleIoHandle(console_handle_map(params->hStdInput)))
    {
        params->hStdInput = console_handle_map(params->hStdInput);
        save_console_mode(params->hStdInput);
    }

    if (!params->hStdOutput)
        params->hStdOutput = INVALID_HANDLE_VALUE;
    else if (VerifyConsoleIoHandle(console_handle_map(params->hStdOutput)))
        params->hStdOutput = console_handle_map(params->hStdOutput);

    if (!params->hStdError)
        params->hStdError = INVALID_HANDLE_VALUE;
    else if (VerifyConsoleIoHandle(console_handle_map(params->hStdError)))
        params->hStdError = console_handle_map(params->hStdError);

    return TRUE;
}

BOOL CONSOLE_Exit(void)
{
    /* the console is in raw mode, put it back in cooked mode */
    return restore_console_mode(GetStdHandle(STD_INPUT_HANDLE));
}

/* Undocumented, called by native doskey.exe */
/* FIXME: Should use CONSOLE_GetHistory() above for full implementation */
DWORD WINAPI GetConsoleCommandHistoryA(DWORD unknown1, DWORD unknown2, DWORD unknown3)
{
    FIXME(": (0x%x, 0x%x, 0x%x) stub!\n", unknown1, unknown2, unknown3);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/* Undocumented, called by native doskey.exe */
/* FIXME: Should use CONSOLE_GetHistory() above for full implementation */
DWORD WINAPI GetConsoleCommandHistoryW(DWORD unknown1, DWORD unknown2, DWORD unknown3)
{
    FIXME(": (0x%x, 0x%x, 0x%x) stub!\n", unknown1, unknown2, unknown3);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/* Undocumented, called by native doskey.exe */
/* FIXME: Should use CONSOLE_GetHistory() above for full implementation */
DWORD WINAPI GetConsoleCommandHistoryLengthA(LPCSTR unknown)
{
    FIXME(": (%s) stub!\n", debugstr_a(unknown));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/* Undocumented, called by native doskey.exe */
/* FIXME: Should use CONSOLE_GetHistory() above for full implementation */
DWORD WINAPI GetConsoleCommandHistoryLengthW(LPCWSTR unknown)
{
    FIXME(": (%s) stub!\n", debugstr_w(unknown));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

DWORD WINAPI GetConsoleAliasesLengthA(LPSTR unknown)
{
    FIXME(": (%s) stub!\n", debugstr_a(unknown));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

DWORD WINAPI GetConsoleAliasesLengthW(LPWSTR unknown)
{
    FIXME(": (%s) stub!\n", debugstr_w(unknown));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

VOID WINAPI ExpungeConsoleCommandHistoryA(LPCSTR unknown)
{
    FIXME(": (%s) stub!\n", debugstr_a(unknown));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

VOID WINAPI ExpungeConsoleCommandHistoryW(LPCWSTR unknown)
{
    FIXME(": (%s) stub!\n", debugstr_w(unknown));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

BOOL WINAPI AddConsoleAliasA(LPSTR source, LPSTR target, LPSTR exename)
{
    FIXME(": (%s, %s, %s) stub!\n", debugstr_a(source), debugstr_a(target), debugstr_a(exename));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI AddConsoleAliasW(LPWSTR source, LPWSTR target, LPWSTR exename)
{
    FIXME(": (%s, %s, %s) stub!\n", debugstr_w(source), debugstr_w(target), debugstr_w(exename));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL WINAPI SetConsoleIcon(HICON icon)
{
    FIXME(": (%p) stub!\n", icon);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetCurrentConsoleFont(HANDLE hConsole, BOOL maxwindow, LPCONSOLE_FONT_INFO fontinfo)
{
    FIXME(": (%p, %d, %p) stub!\n", hConsole, maxwindow, fontinfo);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#ifdef __i386__
#undef GetConsoleFontSize
DWORD WINAPI GetConsoleFontSize(HANDLE hConsole, DWORD font)
{
    union {
        COORD c;
        DWORD w;
    } x;

    FIXME(": (%p, %d) stub!\n", hConsole, font);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    x.c.X = 0;
    x.c.Y = 0;
    return x.w;
}
#endif /* defined(__i386__) */


#ifndef __i386__
COORD WINAPI GetConsoleFontSize(HANDLE hConsole, DWORD font)
{
    COORD c;
    c.X = 80;
    c.Y = 24;
     FIXME(": (%p, %d) stub!\n", hConsole, font);
    return c;
}
#endif /* defined(__i386__) */
