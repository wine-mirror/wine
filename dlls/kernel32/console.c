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
#include "wine/condrv.h"
#include "wine/server.h"
#include "wine/exception.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "excpt.h"
#include "console_private.h"
#include "kernel_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(console);

static const WCHAR coninW[] = {'C','O','N','I','N','$',0};
static const WCHAR conoutW[] = {'C','O','N','O','U','T','$',0};

/******************************************************************************
 * GetConsoleWindow [KERNEL32.@] Get hwnd of the console window.
 *
 * RETURNS
 *   Success: hwnd of the console window.
 *   Failure: NULL
 */
HWND WINAPI GetConsoleWindow(void)
{
    struct condrv_input_info info;
    BOOL ret;

    ret = DeviceIoControl( RtlGetCurrentPeb()->ProcessParameters->ConsoleHandle,
                           IOCTL_CONDRV_GET_INPUT_INFO, NULL, 0, &info, sizeof(info), NULL, NULL );
    return ret ? wine_server_ptr_handle(info.win) : NULL;
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
    SECURITY_ATTRIBUTES sa;

    TRACE("(%s, 0x%08x, %d, %u)\n", debugstr_w(name), access, inherit, creation);

    if (!name || (strcmpiW( coninW, name ) && strcmpiW( conoutW, name )) || creation != OPEN_EXISTING)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = inherit;

    return CreateFileW( name, access, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, creation, 0, NULL );
}

/******************************************************************
 *		VerifyConsoleIoHandle            (KERNEL32.@)
 *
 * Undocumented
 */
BOOL WINAPI VerifyConsoleIoHandle(HANDLE handle)
{
    IO_STATUS_BLOCK io;
    DWORD mode;
    return !NtDeviceIoControlFile( handle, NULL, NULL, NULL, &io, IOCTL_CONDRV_GET_MODE,
                                   NULL, 0, &mode, sizeof(mode) );
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
    return GetStdHandle( STD_INPUT_HANDLE );
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

static enum read_console_input_return read_console_input(HANDLE handle, PINPUT_RECORD ir, DWORD timeout)
{
    int blocking = timeout != 0;
    DWORD read_bytes;

    if (!DeviceIoControl( handle, IOCTL_CONDRV_READ_INPUT, &blocking, sizeof(blocking), ir, sizeof(*ir), &read_bytes, NULL ))
        return rci_error;
    return read_bytes ? rci_gotone : rci_timeout;
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


static WCHAR*	S_EditString /* = NULL */;
static unsigned S_EditStrPos /* = 0 */;


/***********************************************************************
 *            ReadConsoleA   (KERNEL32.@)
 */
BOOL WINAPI ReadConsoleA( HANDLE handle, LPVOID buffer, DWORD length, DWORD *ret_count, void *reserved )
{
    LPWSTR strW = HeapAlloc( GetProcessHeap(), 0, length * sizeof(WCHAR) );
    DWORD count = 0;
    BOOL ret;

    if (!strW)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    if ((ret = ReadConsoleW( handle, strW, length, &count, NULL )))
    {
        count = WideCharToMultiByte( GetConsoleCP(), 0, strW, count, buffer, length, NULL, NULL );
        if (ret_count) *ret_count = count;
    }
    HeapFree( GetProcessHeap(), 0, strW );
    return ret;
}


/***********************************************************************
 *            ReadConsoleW   (KERNEL32.@)
 */
BOOL WINAPI ReadConsoleW(HANDLE hConsoleInput, LPVOID lpBuffer,
			 DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, LPVOID lpReserved)
{
    IO_STATUS_BLOCK io;
    DWORD	charsread;
    LPWSTR	xbuf = lpBuffer;
    DWORD	mode;

    TRACE("(%p,%p,%d,%p,%p)\n",
	  hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, lpReserved);

    if (nNumberOfCharsToRead > INT_MAX)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if (!NtDeviceIoControlFile(hConsoleInput, NULL, NULL, NULL, &io, IOCTL_CONDRV_READ_CONSOLE, NULL, 0, lpBuffer,
                               nNumberOfCharsToRead * sizeof(WCHAR)))
    {
        if (lpNumberOfCharsRead) *lpNumberOfCharsRead = io.Information / sizeof(WCHAR);
        return TRUE;
    }

    if (!GetConsoleMode(hConsoleInput, &mode))
        return FALSE;
    if (mode & ENABLE_LINE_INPUT)
    {
	if (!S_EditString || S_EditString[S_EditStrPos] == 0)
	{
	    HeapFree(GetProcessHeap(), 0, S_EditString);
	    if (!(S_EditString = CONSOLE_Readline(hConsoleInput, TRUE)))
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
 *            GetNumberOfConsoleMouseButtons   (KERNEL32.@)
 */
BOOL WINAPI GetNumberOfConsoleMouseButtons(LPDWORD nrofbuttons)
{
    FIXME("(%p): stub\n", nrofbuttons);
    *nrofbuttons = 2;
    return TRUE;
}

/******************************************************************
 *		CONSOLE_WriteChars
 *
 * WriteConsoleOutput helper: hides server call semantics
 * writes a string at a given pos with standard attribute
 */
static int CONSOLE_WriteChars(HANDLE handle, const WCHAR *str, size_t length, COORD *coord)
{
    struct condrv_output_params *params;
    DWORD written = 0, size;

    if (!length) return 0;

    size = sizeof(*params) + length * sizeof(WCHAR);
    if (!(params = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;
    params->mode   = CHAR_INFO_MODE_TEXTSTDATTR;
    params->x      = coord->X;
    params->y      = coord->Y;
    params->width  = 0;
    memcpy( params + 1, str, length * sizeof(*str) );
    if (DeviceIoControl( handle, IOCTL_CONDRV_WRITE_OUTPUT, params, size,
                         &written, sizeof(written), NULL, NULL ))
        coord->X += written;
    HeapFree( GetProcessHeap(), 0, params );
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
 *            WriteConsoleA   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteConsoleA( HANDLE handle, LPCVOID buffer, DWORD length,
                                             DWORD *written, void *reserved )
{
    UINT cp = GetConsoleOutputCP();
    LPWSTR strW;
    DWORD lenW;
    BOOL ret;

    if (written) *written = 0;
    lenW = MultiByteToWideChar( cp, 0, buffer, length, NULL, 0 );
    if (!(strW = HeapAlloc( GetProcessHeap(), 0, lenW * sizeof(WCHAR) ))) return FALSE;
    MultiByteToWideChar( cp, 0, buffer, length, strW, lenW );
    ret = WriteConsoleW( handle, strW, lenW, written, 0 );
    HeapFree( GetProcessHeap(), 0, strW );
    return ret;
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
    int				k, first = 0;
    IO_STATUS_BLOCK io;

    TRACE("%p %s %d %p %p\n",
	  hConsoleOutput, debugstr_wn(lpBuffer, nNumberOfCharsToWrite),
	  nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);

    if (lpNumberOfCharsWritten) *lpNumberOfCharsWritten = 0;

    if (!NtDeviceIoControlFile(hConsoleOutput, NULL, NULL, NULL, &io, IOCTL_CONDRV_WRITE_CONSOLE, (void *)lpBuffer,
                               nNumberOfCharsToWrite * sizeof(WCHAR), NULL, 0))
    {
        if (lpNumberOfCharsWritten) *lpNumberOfCharsWritten = nNumberOfCharsToWrite;
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


/******************************************************************
 *		CONSOLE_FillLineUniform
 *
 * Helper function for ScrollConsoleScreenBufferW
 * Fills a part of a line with a constant character info
 */
void CONSOLE_FillLineUniform(HANDLE hConsoleOutput, int i, int j, int len, LPCHAR_INFO lpFill)
{
    struct condrv_fill_output_params params;

    params.mode  = CHAR_INFO_MODE_TEXTATTR;
    params.x     = i;
    params.y     = j;
    params.count = len;
    params.wrap  = FALSE;
    params.ch    = lpFill->Char.UnicodeChar;
    params.attr  = lpFill->Attributes;
    DeviceIoControl( hConsoleOutput, IOCTL_CONDRV_FILL_OUTPUT, &params, sizeof(params), NULL, 0, NULL, NULL );
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
unsigned CONSOLE_GetNumHistoryEntries(HANDLE console)
{
    struct condrv_input_info info;
    BOOL ret = DeviceIoControl( console, IOCTL_CONDRV_GET_INPUT_INFO, NULL, 0, &info, sizeof(info), NULL, NULL );
    return ret ? info.history_index : ~0;
}

/******************************************************************
 *		CONSOLE_GetEditionMode
 *
 *
 */
BOOL CONSOLE_GetEditionMode(HANDLE hConIn, int* mode)
{
    struct condrv_input_info info;
    return DeviceIoControl( hConIn, IOCTL_CONDRV_GET_INPUT_INFO, NULL, 0, &info, sizeof(info), NULL, NULL )
        ? info.edition_mode : 0;
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
    /* convert value from server:
     * + INVALID_HANDLE_VALUE => TEB: 0, STARTUPINFO: INVALID_HANDLE_VALUE
     * + 0                    => TEB: 0, STARTUPINFO: INVALID_HANDLE_VALUE
     * + console handle needs to be mapped
     */
    if (!params->hStdInput || params->hStdInput == INVALID_HANDLE_VALUE)
        params->hStdInput = 0;
    else if (!is_console_handle(params->hStdInput) && VerifyConsoleIoHandle(params->hStdInput))
        params->hStdInput = console_handle_map(params->hStdInput);

    if (!params->hStdOutput || params->hStdOutput == INVALID_HANDLE_VALUE)
        params->hStdOutput = 0;
    else if (!is_console_handle(params->hStdOutput) && VerifyConsoleIoHandle(params->hStdOutput))
        params->hStdOutput = console_handle_map(params->hStdOutput);

    if (!params->hStdError || params->hStdError == INVALID_HANDLE_VALUE)
        params->hStdError = 0;
    else if (!is_console_handle(params->hStdError) && VerifyConsoleIoHandle(params->hStdError))
        params->hStdError = console_handle_map(params->hStdError);

    return TRUE;
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

DWORD WINAPI GetConsoleAliasExesLengthA(void)
{
    FIXME(": stub!\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

DWORD WINAPI GetConsoleAliasExesLengthW(void)
{
    FIXME(": stub!\n");
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

DWORD WINAPI GetNumberOfConsoleFonts(void)
{
    return 1;
}

BOOL WINAPI SetConsoleFont(HANDLE hConsole, DWORD index)
{
    FIXME("(%p, %u): stub!\n", hConsole, index);
    SetLastError(LOWORD(E_NOTIMPL) /* win10 1709+ */);
    return FALSE;
}

BOOL WINAPI SetConsoleKeyShortcuts(BOOL set, BYTE keys, VOID *a, DWORD b)
{
    FIXME(": (%u %u %p %u) stub!\n", set, keys, a, b);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL WINAPI GetCurrentConsoleFontEx(HANDLE hConsole, BOOL maxwindow, CONSOLE_FONT_INFOEX *fontinfo)
{
    DWORD size;
    struct
    {
        struct condrv_output_info info;
        WCHAR face_name[LF_FACESIZE - 1];
    } data;

    if (fontinfo->cbSize != sizeof(CONSOLE_FONT_INFOEX))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!DeviceIoControl( hConsole, IOCTL_CONDRV_GET_OUTPUT_INFO, NULL, 0,
                          &data, sizeof(data), &size, NULL ))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    fontinfo->nFont = 0;
    if (maxwindow)
    {
        fontinfo->dwFontSize.X = min( data.info.width, data.info.max_width );
        fontinfo->dwFontSize.Y = min( data.info.height, data.info.max_height );
    }
    else
    {
        fontinfo->dwFontSize.X = data.info.win_right - data.info.win_left + 1;
        fontinfo->dwFontSize.Y = data.info.win_bottom - data.info.win_top + 1;
    }
    size -= sizeof(data.info);
    if (size) memcpy( fontinfo->FaceName, data.face_name, size );
    fontinfo->FaceName[size / sizeof(WCHAR)] = 0;
    fontinfo->FontFamily = data.info.font_pitch_family;
    fontinfo->FontWeight = data.info.font_weight;
    return TRUE;
}

BOOL WINAPI GetCurrentConsoleFont(HANDLE hConsole, BOOL maxwindow, CONSOLE_FONT_INFO *fontinfo)
{
    BOOL ret;
    CONSOLE_FONT_INFOEX res;

    res.cbSize = sizeof(CONSOLE_FONT_INFOEX);

    ret = GetCurrentConsoleFontEx(hConsole, maxwindow, &res);
    if(ret)
    {
        fontinfo->nFont = res.nFont;
        fontinfo->dwFontSize.X = res.dwFontSize.X;
        fontinfo->dwFontSize.Y = res.dwFontSize.Y;
    }
    return ret;
}

static COORD get_console_font_size(HANDLE hConsole, DWORD index)
{
    struct condrv_output_info info;
    COORD c = {0,0};

    if (index >= GetNumberOfConsoleFonts())
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return c;
    }

    if (DeviceIoControl( hConsole, IOCTL_CONDRV_GET_OUTPUT_INFO, NULL, 0, &info, sizeof(info), NULL, NULL ))
    {
        c.X = info.font_width;
        c.Y = info.font_height;
    }
    else SetLastError( ERROR_INVALID_HANDLE );
    return c;
}

#if defined(__i386__) && !defined(__MINGW32__) && !defined(_MSC_VER)
#undef GetConsoleFontSize
DWORD WINAPI GetConsoleFontSize(HANDLE hConsole, DWORD index)
{
    union {
        COORD c;
        DWORD w;
    } x;

    x.c = get_console_font_size(hConsole, index);
    return x.w;
}
#else
COORD WINAPI GetConsoleFontSize(HANDLE hConsole, DWORD index)
{
    return get_console_font_size(hConsole, index);
}
#endif /* !defined(__i386__) */

BOOL WINAPI GetConsoleFontInfo(HANDLE hConsole, BOOL maximize, DWORD numfonts, CONSOLE_FONT_INFO *info)
{
    FIXME("(%p %d %u %p): stub!\n", hConsole, maximize, numfonts, info);
    SetLastError(LOWORD(E_NOTIMPL) /* win10 1709+ */);
    return FALSE;
}

BOOL WINAPI SetCurrentConsoleFontEx(HANDLE hConsole, BOOL maxwindow, CONSOLE_FONT_INFOEX *cfix)
{
    FIXME("(%p %d %p): stub!\n", hConsole, maxwindow, cfix);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
