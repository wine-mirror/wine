/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 * Copyright 1997 Karl Garrison
 * Copyright 1998 John Richardson
 * Copyright 1998 Marcus Meissner
 * Copyright 2001,2002 Eric Pouech
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

/* Reference applications:
 * -  IDA (interactive disassembler) full version 3.75. Works.
 * -  LYNX/W32. Works mostly, some keys crash it.
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "wincon.h"
#include "heap.h"
#include "wine/server.h"
#include "wine/exception.h"
#include "wine/debug.h"
#include "msvcrt/excpt.h"

WINE_DEFAULT_DEBUG_CHANNEL(console);

/* editline.c */
extern WCHAR* CONSOLE_Readline(HANDLE, int);

static WCHAR*	S_EditString /* = NULL */;
static unsigned S_EditStrPos /* = 0 */;

/***********************************************************************
 *            FreeConsole (KERNEL32.@)
 */
BOOL WINAPI FreeConsole(VOID)
{
    BOOL ret;

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
static	BOOL	start_console_renderer(void)
{
    char		buffer[256];
    int			ret;
    STARTUPINFOA	si;
    PROCESS_INFORMATION	pi;
    HANDLE		hEvent = 0;
    LPSTR		p;
    OBJECT_ATTRIBUTES	attr;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.Attributes               = OBJ_INHERIT;
    attr.ObjectName               = NULL;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;

    NtCreateEvent(&hEvent, EVENT_ALL_ACCESS, &attr, TRUE, FALSE);
    if (!hEvent) return FALSE;

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    /* FIXME: use dynamic allocation for most of the buffers below */
    /* first try environment variable */
    if ((p = getenv("WINECONSOLE")) != NULL)
    {
	ret = snprintf(buffer, sizeof(buffer), "%s --use-event=%d", p, hEvent);
	if ((ret > -1) && (ret < sizeof(buffer)) &&
	    CreateProcessA(NULL, buffer, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &si, &pi))
	    goto succeed;
	ERR("Couldn't launch Wine console from WINECONSOLE env var... trying default access\n");
    }

    /* then try the regular PATH */
    sprintf(buffer, "wineconsole --use-event=%d", hEvent);
    if (CreateProcessA(NULL, buffer, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &si, &pi))
	goto succeed;

    goto the_end;

 succeed:
    if (WaitForSingleObject(hEvent, INFINITE) != WAIT_OBJECT_0) goto the_end;
    CloseHandle(hEvent);

    TRACE("Started wineconsole pid=%08lx tid=%08lx\n", pi.dwProcessId, pi.dwThreadId);

    return TRUE;

 the_end:
    ERR("Can't allocate console\n");
    CloseHandle(hEvent);
    return FALSE;
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
    STARTUPINFOW si;

    TRACE("()\n");

    handle_in = CreateFileA( "CONIN$", GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,
			     0, NULL, OPEN_EXISTING, 0, 0 );

    if (handle_in != INVALID_HANDLE_VALUE)
    {
	/* we already have a console opened on this process, don't create a new one */
	CloseHandle(handle_in);
	return FALSE;
    }

    if (!start_console_renderer())
	goto the_end;

    handle_in = CreateFileA( "CONIN$", GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,
			     0, NULL, OPEN_EXISTING, 0, 0 );
    if (handle_in == INVALID_HANDLE_VALUE) goto the_end;

    handle_out = CreateFileA( "CONOUT$", GENERIC_READ|GENERIC_WRITE,
			     0, NULL, OPEN_EXISTING, 0, 0 );
    if (handle_out == INVALID_HANDLE_VALUE) goto the_end;

    if (!DuplicateHandle(GetCurrentProcess(), handle_out, GetCurrentProcess(), &handle_err,
			 0, TRUE, DUPLICATE_SAME_ACCESS))
	goto the_end;

    /* NT resets the STD_*_HANDLEs on console alloc */
    SetStdHandle(STD_INPUT_HANDLE,  handle_in);
    SetStdHandle(STD_OUTPUT_HANDLE, handle_out);
    SetStdHandle(STD_ERROR_HANDLE,  handle_err);

    GetStartupInfoW(&si);
    if (si.dwFlags & STARTF_USECOUNTCHARS)
    {
	COORD	c;
	c.X = si.dwXCountChars;
	c.Y = si.dwYCountChars;
	SetConsoleScreenBufferSize(handle_out, c);
    }
    if (si.dwFlags & STARTF_USEFILLATTRIBUTE)
	SetConsoleTextAttribute(handle_out, si.dwFillAttribute);
    if (si.lpTitle)
	SetConsoleTitleW(si.lpTitle);

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


/******************************************************************************
 * read_console_input
 *
 * Helper function for ReadConsole, ReadConsoleInput and PeekConsoleInput
 */
static BOOL read_console_input(HANDLE handle, LPINPUT_RECORD buffer, DWORD count,
			       LPDWORD pRead, BOOL flush)
{
    BOOL	ret;
    unsigned	read = 0;

    SERVER_START_REQ( read_console_input )
    {
        req->handle = handle;
        req->flush = flush;
        wine_server_set_reply( req, buffer, count * sizeof(INPUT_RECORD) );
        if ((ret = !wine_server_call_err( req ))) read = reply->read;
    }
    SERVER_END_REQ;
    if (pRead) *pRead = read;
    return ret;
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

    if ((ret = ReadConsoleW(hConsoleInput, ptr, nNumberOfCharsToRead, &ncr, NULL)))
	ncr = WideCharToMultiByte(CP_ACP, 0, ptr, ncr, lpBuffer, nNumberOfCharsToRead, NULL, NULL);

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
    LPWSTR	xbuf = (LPWSTR)lpBuffer;
    DWORD	mode;

    TRACE("(%d,%p,%ld,%p,%p)\n",
	  hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, lpReserved);

    if (!GetConsoleMode(hConsoleInput, &mode))
        return FALSE;

    if (mode & ENABLE_LINE_INPUT)
    {
	if (!S_EditString || S_EditString[S_EditStrPos] == 0)
	{
	    if (S_EditString) HeapFree(GetProcessHeap(), 0, S_EditString);
	    if (!(S_EditString = CONSOLE_Readline(hConsoleInput, mode & WINE_ENABLE_LINE_INPUT_EMACS)))
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
	DWORD 		count;

	/* FIXME: should we read at least 1 char? The SDK does not say */
	/* wait for at least one available input record (it doesn't mean we'll have
	 * chars stored in xbuf...
	 */
	WaitForSingleObject(hConsoleInput, INFINITE);
	for (charsread = 0; charsread < nNumberOfCharsToRead;)
	{
	    if (!read_console_input(hConsoleInput, &ir, 1, &count, TRUE)) return FALSE;
	    if (count && ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown &&
		ir.Event.KeyEvent.uChar.UnicodeChar &&
		!(ir.Event.KeyEvent.dwControlKeyState & ENHANCED_KEY))
	    {
		xbuf[charsread++] = ir.Event.KeyEvent.uChar.UnicodeChar;
	    }
	}
    }

    if (lpNumberOfCharsRead) *lpNumberOfCharsRead = charsread;

    return TRUE;
}


/***********************************************************************
 *            ReadConsoleInputW   (KERNEL32.@)
 */
BOOL WINAPI ReadConsoleInputW(HANDLE hConsoleInput, LPINPUT_RECORD lpBuffer,
                              DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
    DWORD count;

    if (!nLength)
    {
        if (lpNumberOfEventsRead) *lpNumberOfEventsRead = 0;
        return TRUE;
    }

    /* loop until we get at least one event */
    for (;;)
    {
        WaitForSingleObject(hConsoleInput, INFINITE);
	if (!read_console_input(hConsoleInput, lpBuffer, nLength, &count, TRUE))
	    return FALSE;
        if (count)
        {
            if (lpNumberOfEventsRead) *lpNumberOfEventsRead = count;
            return TRUE;
        }
    }
}


/******************************************************************************
 * WriteConsoleOutputCharacterW [KERNEL32.@]  Copies character to consecutive
 * 					      cells in the console screen buffer
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

    TRACE("(%d,%s,%ld,%dx%d,%p)\n", hConsoleOutput,
          debugstr_wn(str, length), length, coord.X, coord.Y, lpNumCharsWritten);

    SERVER_START_REQ( write_console_output )
    {
        req->handle = hConsoleOutput;
        req->x      = coord.X;
        req->y      = coord.Y;
        req->mode   = CHAR_INFO_MODE_TEXT;
        req->wrap   = TRUE;
        wine_server_add_data( req, str, length * sizeof(WCHAR) );
        if ((ret = !wine_server_call_err( req )))
        {
            if (lpNumCharsWritten) *lpNumCharsWritten = reply->written;
        }
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
 *
 * BUGS
 *   Unimplemented
 */
BOOL WINAPI SetConsoleInputExeNameW(LPCWSTR name)
{
    FIXME("(%s): stub!\n", debugstr_w(name));

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return TRUE;
}

/******************************************************************************
 *  SetConsoleInputExeNameA	 [KERNEL32.@]
 *
 * BUGS
 *   Unimplemented
 */
BOOL WINAPI SetConsoleInputExeNameA(LPCSTR name)
{
    int		len = MultiByteToWideChar(CP_ACP, 0, name, -1, NULL, 0);
    LPWSTR	xptr = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    BOOL	ret;

    if (!xptr) return FALSE;

    MultiByteToWideChar(CP_ACP, 0, name, -1, xptr, len);
    ret = SetConsoleInputExeNameW(xptr);
    HeapFree(GetProcessHeap(), 0, xptr);

    return ret;
}

/******************************************************************
 *		CONSOLE_DefaultHandler
 *
 * Final control event handler
 */
static BOOL WINAPI CONSOLE_DefaultHandler(DWORD dwCtrlType)
{
    FIXME("Terminating process %lx on event %lx\n", GetCurrentProcessId(), dwCtrlType);
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
 *
 * CHANGED
 * James Sutherland (JamesSutherland@gmx.de)
 * Added global variables console_ignore_ctrl_c and handlers[]
 * Does not yet do any error checking, or set LastError if failed.
 * This doesn't yet matter, since these handlers are not yet called...!
 */

struct ConsoleHandler {
    PHANDLER_ROUTINE            handler;
    struct ConsoleHandler*      next;
};

static unsigned int             CONSOLE_IgnoreCtrlC = 0; /* FIXME: this should be inherited somehow */
static struct ConsoleHandler    CONSOLE_DefaultConsoleHandler = {CONSOLE_DefaultHandler, NULL};
static struct ConsoleHandler*   CONSOLE_Handlers = &CONSOLE_DefaultConsoleHandler;
static CRITICAL_SECTION         CONSOLE_CritSect = CRITICAL_SECTION_INIT("console_ctrl_section");

/*****************************************************************************/

BOOL WINAPI SetConsoleCtrlHandler(PHANDLER_ROUTINE func, BOOL add)
{
    BOOL        ret = TRUE;

    FIXME("(%p,%i) - no error checking or testing yet\n", func, add);

    if (!func)
    {
	CONSOLE_IgnoreCtrlC = add;
    }
    else if (add)
    {
        struct ConsoleHandler*  ch = HeapAlloc(GetProcessHeap(), 0, sizeof(struct ConsoleHandler));

        if (!ch) return FALSE;
        ch->handler = func;
        EnterCriticalSection(&CONSOLE_CritSect);
        ch->next = CONSOLE_Handlers;
        CONSOLE_Handlers = ch;
        LeaveCriticalSection(&CONSOLE_CritSect);
    }
    else
    {
        struct ConsoleHandler**  ch;
        EnterCriticalSection(&CONSOLE_CritSect);
        for (ch = &CONSOLE_Handlers; *ch; *ch = (*ch)->next)
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
                ret = FALSE;
            }
            else
            {
                rch = *ch;
                *ch = (*ch)->next;
                HeapFree(GetProcessHeap(), 0, rch);
            }
        }
        else
        {
            WARN("Attempt to remove non-installed CtrlHandler %p\n", func);
            ret = FALSE;
        }
        LeaveCriticalSection(&CONSOLE_CritSect);
    }
    return ret;
}

static WINE_EXCEPTION_FILTER(CONSOLE_CtrlEventHandler)
{
    TRACE("(%lx)\n", GetExceptionCode());
    return EXCEPTION_EXECUTE_HANDLER;
}

static DWORD WINAPI CONSOLE_HandleCtrlCEntry(void* pmt)
{
    struct ConsoleHandler*  ch;

    EnterCriticalSection(&CONSOLE_CritSect);
    /* the debugger didn't continue... so, pass to ctrl handlers */
    for (ch = CONSOLE_Handlers; ch; ch = ch->next)
    {
        if (ch->handler((DWORD)pmt)) break;
    }
    LeaveCriticalSection(&CONSOLE_CritSect);
    return 0;
}

/******************************************************************
 *		CONSOLE_HandleCtrlC
 *
 * Check whether the shall manipulate CtrlC events
 */
int     CONSOLE_HandleCtrlC(void)
{
    /* FIXME: better test whether a console is attached to this process ??? */
    extern    unsigned CONSOLE_GetNumHistoryEntries(void);
    if (CONSOLE_GetNumHistoryEntries() == (unsigned)-1) return 0;
    if (CONSOLE_IgnoreCtrlC) return 1;

    /* try to pass the exception to the debugger
     * if it continues, there's nothing more to do
     * otherwise, we need to send the ctrl-event to the handlers
     */
    __TRY
    {
        RaiseException( DBG_CONTROL_C, 0, 0, NULL );
    }
    __EXCEPT(CONSOLE_CtrlEventHandler)
    {
        /* Create a separate thread to signal all the events. This would allow to
         * synchronize between setting the handlers and actually calling them
         */
        CreateThread(NULL, 0, CONSOLE_HandleCtrlCEntry, (void*)CTRL_C_EVENT, 0, NULL);
    }
    __ENDTRY;
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

    TRACE("(%ld, %ld)\n", dwCtrlEvent, dwProcessGroupID);

    if (dwCtrlEvent != CTRL_C_EVENT && dwCtrlEvent != CTRL_BREAK_EVENT)
    {
	ERR("Invalid event %ld for PGID %ld\n", dwCtrlEvent, dwProcessGroupID);
	return FALSE;
    }

    SERVER_START_REQ( send_console_signal )
    {
        req->signal = dwCtrlEvent;
        req->group_id = (void*)dwProcessGroupID;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

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

    TRACE("(%ld,%ld,%p,%ld,%p)\n",
	  dwDesiredAccess, dwShareMode, sa, dwFlags, lpScreenBufferData);

    if (dwFlags != CONSOLE_TEXTMODE_BUFFER || lpScreenBufferData != NULL)
    {
	SetLastError(ERROR_INVALID_PARAMETER);
	return INVALID_HANDLE_VALUE;
    }

    SERVER_START_REQ(create_console_output)
    {
	req->handle_in = 0;
	req->access    = dwDesiredAccess;
	req->share     = dwShareMode;
	req->inherit   = (sa && sa->bInheritHandle);
	if (!wine_server_call_err( req )) ret = reply->handle_out;
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
        req->handle = hConsoleOutput;
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

    TRACE("(%x)\n", hConsoleOutput);

    SERVER_START_REQ( set_console_input_info )
    {
        req->handle    = 0;
        req->mask      = SET_CONSOLE_INPUT_INFO_ACTIVE_SB;
        req->active_sb = hConsoleOutput;
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

    SERVER_START_REQ(get_console_mode)
    {
	req->handle = hcon;
	ret = !wine_server_call_err( req );
	if (ret && mode) *mode = reply->mode;
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
	req->handle = hcon;
	req->mode = mode;
	ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    /* FIXME: when resetting a console input to editline mode, I think we should
     * empty the S_EditString buffer
     */

    TRACE("(%x,%lx) retval == %d\n", hcon, mode, ret);

    return ret;
}


/******************************************************************
 *		write_char
 *
 * WriteConsoleOutput helper: hides server call semantics
 */
static int write_char(HANDLE hCon, LPCWSTR lpBuffer, int nc, COORD* pos)
{
    int written = -1;

    if (!nc) return 0;

    SERVER_START_REQ( write_console_output )
    {
        req->handle = hCon;
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
static int	next_line(HANDLE hCon, CONSOLE_SCREEN_BUFFER_INFO* csbi)
{
    SMALL_RECT	src;
    CHAR_INFO	ci;
    COORD	dst;

    csbi->dwCursorPosition.X = 0;
    csbi->dwCursorPosition.Y++;

    if (csbi->dwCursorPosition.Y < csbi->dwSize.Y) return 1;

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
	return 0;
    return 1;
}

/******************************************************************
 *		write_block
 *
 * WriteConsoleOutput helper: writes a block of non special characters
 * Block can spread on several lines, and wrapping, if needed, is
 * handled
 *
 */
static int     	write_block(HANDLE hCon, CONSOLE_SCREEN_BUFFER_INFO* csbi,
			    DWORD mode, LPWSTR ptr, int len)
{
    int	blk;	/* number of chars to write on current line */

    if (len <= 0) return 1;

    if (mode & ENABLE_WRAP_AT_EOL_OUTPUT) /* writes remaining on next line */
    {
        int     done;

        for (done = 0; done < len; done += blk)
        {
            blk = min(len - done, csbi->dwSize.X - csbi->dwCursorPosition.X);

            if (write_char(hCon, ptr + done, blk, &csbi->dwCursorPosition) != blk)
                return 0;
            if (csbi->dwCursorPosition.X == csbi->dwSize.X && !next_line(hCon, csbi))
                return 0;
        }
    }
    else
    {
        blk = min(len, csbi->dwSize.X - csbi->dwCursorPosition.X);

        if (write_char(hCon, ptr, blk, &csbi->dwCursorPosition) != blk)
            return 0;
        if (blk < len)
        {
            csbi->dwCursorPosition.X = csbi->dwSize.X - 1;
            /* all remaining chars should be written on last column,
             * so only overwrite the last column with last char in block
             */
            if (write_char(hCon, ptr + len - 1, 1, &csbi->dwCursorPosition) != 1)
                return 0;
            csbi->dwCursorPosition.X = csbi->dwSize.X - 1;
        }
    }

    return 1;
}

/***********************************************************************
 *            WriteConsoleW   (KERNEL32.@)
 */
BOOL WINAPI WriteConsoleW(HANDLE hConsoleOutput, LPCVOID lpBuffer, DWORD nNumberOfCharsToWrite,
			  LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved)
{
    DWORD			mode;
    DWORD			nw = 0;
    WCHAR*			psz = (WCHAR*)lpBuffer;
    CONSOLE_SCREEN_BUFFER_INFO	csbi;
    int				k, first = 0;

    TRACE("%d %s %ld %p %p\n",
	  hConsoleOutput, debugstr_wn(lpBuffer, nNumberOfCharsToWrite),
	  nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);

    if (lpNumberOfCharsWritten) *lpNumberOfCharsWritten = 0;

    if (!GetConsoleMode(hConsoleOutput, &mode) ||
	!GetConsoleScreenBufferInfo(hConsoleOutput, &csbi))
	return FALSE;

    if (mode & ENABLE_PROCESSED_OUTPUT)
    {
	int	i;

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
		    WCHAR tmp[8] = {' ',' ',' ',' ',' ',' ',' ',' '};

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

    n = MultiByteToWideChar(CP_ACP, 0, lpBuffer, nNumberOfCharsToWrite, NULL, 0);

    if (lpNumberOfCharsWritten) *lpNumberOfCharsWritten = 0;
    xstring = HeapAlloc(GetProcessHeap(), 0, n * sizeof(WCHAR));
    if (!xstring) return 0;

    MultiByteToWideChar(CP_ACP, 0, lpBuffer, nNumberOfCharsToWrite, xstring, n);

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
 * RETURNS STD
 */
BOOL WINAPI SetConsoleCursorPosition(HANDLE hcon, COORD pos)
{
    BOOL 			ret;
    CONSOLE_SCREEN_BUFFER_INFO	csbi;
    int				do_move = 0;
    int				w, h;

    TRACE("%x %d %d\n", hcon, pos.X, pos.Y);

    SERVER_START_REQ(set_console_output_info)
    {
        req->handle         = hcon;
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
BOOL WINAPI GetConsoleCursorInfo(HANDLE hcon, LPCONSOLE_CURSOR_INFO cinfo)
{
    BOOL ret;

    SERVER_START_REQ(get_console_output_info)
    {
        req->handle = hcon;
        ret = !wine_server_call_err( req );
        if (ret && cinfo)
        {
            cinfo->dwSize = reply->cursor_size;
            cinfo->bVisible = reply->cursor_visible;
        }
    }
    SERVER_END_REQ;
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

    SERVER_START_REQ(set_console_output_info)
    {
        req->handle         = hCon;
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

    if (!bAbsolute)
    {
	CONSOLE_SCREEN_BUFFER_INFO	csbi;
	if (!GetConsoleScreenBufferInfo(hCon, &csbi))
	    return FALSE;
	p.Left   += csbi.srWindow.Left;
	p.Top    += csbi.srWindow.Top;
	p.Right  += csbi.srWindow.Left;
	p.Bottom += csbi.srWindow.Top;
    }
    SERVER_START_REQ(set_console_output_info)
    {
        req->handle         = hCon;
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

    SERVER_START_REQ(set_console_output_info)
    {
        req->handle = hConsoleOutput;
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

    SERVER_START_REQ(set_console_output_info)
    {
        req->handle = hConsoleOutput;
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
    MultiByteToWideChar(CP_ACP, 0, &lpFill->Char.AsciiChar, 1, &ciw.Char.UnicodeChar, 1);

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
        req->handle    = hConsoleOutput;
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

    if (lpClipRect)
	TRACE("(%d,(%d,%d-%d,%d),(%d,%d-%d,%d),%d-%d,%p)\n", hConsoleOutput,
	      lpScrollRect->Left, lpScrollRect->Top,
	      lpScrollRect->Right, lpScrollRect->Bottom,
	      lpClipRect->Left, lpClipRect->Top,
	      lpClipRect->Right, lpClipRect->Bottom,
	      dwDestOrigin.X, dwDestOrigin.Y, lpFill);
    else
	TRACE("(%d,(%d,%d-%d,%d),(nil),%d-%d,%p)\n", hConsoleOutput,
	      lpScrollRect->Left, lpScrollRect->Top,
	      lpScrollRect->Right, lpScrollRect->Bottom,
	      dwDestOrigin.X, dwDestOrigin.Y, lpFill);

    if (!GetConsoleScreenBufferInfo(hConsoleOutput, &csbi))
	return FALSE;

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
    if (dst.Left   < clip.Left  ) dst.Left   = clip.Left;
    if (dst.Top    < clip.Top   ) dst.Top    = clip.Top;
    if (dst.Right  > clip.Right ) dst.Right  = clip.Right;
    if (dst.Bottom > clip.Bottom) dst.Bottom = clip.Bottom;

    /* step 3: transfer the bits */
    SERVER_START_REQ(move_console_output)
    {
        req->handle = hConsoleOutput;
	req->x_src = lpScrollRect->Left;
	req->y_src = lpScrollRect->Top;
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


/* ====================================================================
 *
 * Console manipulation functions
 *
 * ====================================================================*/

/* some missing functions...
 * FIXME: those are likely to be defined as undocumented function in kernel32 (or part of them)
 * should get the right API and implement them
 *	GetConsoleCommandHistory[AW] (dword dword dword)
 *	GetConsoleCommandHistoryLength[AW]
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
