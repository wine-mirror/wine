/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 * Copyright 1997 Karl Garrison
 * Copyright 1998 John Richardson
 * Copyright 1998 Marcus Meissner
 * Copyright 2001 Eric Pouech
 */

/* Reference applications:
 * -  IDA (interactive disassembler) full version 3.75. Works.
 * -  LYNX/W32. Works mostly, some keys crash it.
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "wincon.h"
#include "heap.h"
#include "wine/server.h"
#include "wine/exception.h"
#include "debugtools.h"
#include "options.h"

DEFAULT_DEBUG_CHANNEL(console);

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
        ret = !SERVER_CALL_ERR();
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
    STARTUPINFOA	si;
    PROCESS_INFORMATION	pi;
    HANDLE		hEvent = 0;
    LPSTR		p, path = NULL;
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
	if (snprintf(buffer, sizeof(buffer), "%s %d", p, hEvent) > 0 &&
	    CreateProcessA(NULL, buffer, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &si, &pi))
	    goto succeed;
	ERR("Couldn't launch Wine console from WINECONSOLE env var... trying default access\n");
    }

    /* then the regular installation dir */
    if (snprintf(buffer, sizeof(buffer), "%s %d", BINDIR "/wineconsole", hEvent) > 0 &&
	CreateProcessA(NULL, buffer, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &si, &pi))
	goto succeed;

    /* then try the dir where we were started from */
    if ((path = HeapAlloc(GetProcessHeap(), 0, strlen(full_argv0) + sizeof(buffer))))
    {
	int	n;

	if ((p = strrchr(strcpy( path, full_argv0 ), '/')))
	{
	    p++;
	    sprintf(p, "wineconsole %d", hEvent);
	    if (CreateProcessA(NULL, path, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &si, &pi))
		goto succeed;
	    sprintf(p, "programs/wineconsole/wineconsole %d", hEvent);
	    if (CreateProcessA(NULL, path, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &si, &pi))
		goto succeed;
	}

	n = readlink(full_argv0, buffer, sizeof(buffer));
	if (n != -1 && n < sizeof(buffer))
	{
	    buffer[n] = 0;
	    if (buffer[0] == '/') /* absolute path ? */
		strcpy(path, buffer);
	    else if ((p = strrchr(strcpy( path, full_argv0 ), '/')))
	    {
		strcpy(p + 1, buffer);
	    }
	    else *path = 0;

	    if ((p = strrchr(path, '/')))
	    {
		p++;
		sprintf(p, "wineconsole %d", hEvent);
		if (CreateProcessA(NULL, path, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &si, &pi))
		    goto succeed;
		sprintf(p, "programs/wineconsole/wineconsole %d", hEvent);
		if (CreateProcessA(NULL, path, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &si, &pi))
		    goto succeed;
	    }
	} else perror("readlink");

	HeapFree(GetProcessHeap(), 0, path);	path = NULL;
    }
	
    /* then try the regular PATH */
    sprintf(buffer, "wineconsole %d\n", hEvent);
    if (CreateProcessA(NULL, buffer, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &si, &pi))
	goto succeed;

    goto the_end;

 succeed:    
    if (path) HeapFree(GetProcessHeap(), 0, path);
    if (WaitForSingleObject(hEvent, INFINITE) != WAIT_OBJECT_0) goto the_end;
    CloseHandle(hEvent);
    
    TRACE("Started wineconsole pid=%08lx tid=%08lx\n", pi.dwProcessId, pi.dwThreadId);

    return TRUE;

 the_end:
    ERR("Can't allocate console\n");
    if (path) 		HeapFree(GetProcessHeap(), 0, path);
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
    STARTUPINFOA	si;

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

    GetStartupInfoA(&si);
    if (si.dwFlags & STARTF_USESIZE)
    {
	COORD	c;
	c.X = si.dwXCountChars;
	c.Y = si.dwYCountChars;
	SetConsoleScreenBufferSize(handle_out, c);
    }
    if (si.dwFlags & STARTF_USEFILLATTRIBUTE)
	SetConsoleTextAttribute(handle_out, si.dwFillAttribute);
    if (si.lpTitle)
	SetConsoleTitleA(si.lpTitle);

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
    DWORD	mode;

    count = min(count, REQUEST_MAX_VAR_SIZE/sizeof(INPUT_RECORD));
    
    SERVER_START_VAR_REQ(read_console_input, count*sizeof(INPUT_RECORD))
    {
        req->handle = handle;
        req->flush = flush;
        if ((ret = !SERVER_CALL_ERR()))
        {
            if (count) memcpy(buffer, server_data_ptr(req), server_data_size(req));
            read = req->read;
        }
    }
    SERVER_END_VAR_REQ;
    if (count && flush && GetConsoleMode(handle, &mode) && (mode & ENABLE_PROCESSED_INPUT))
    {
	int	i;

	for (i = 0; i < read; i++)
	{
	    if (buffer[i].EventType == KEY_EVENT && buffer[i].Event.KeyEvent.bKeyDown &&
		buffer[i].Event.KeyEvent.uChar.UnicodeChar == 'C' - 64 &&
		!(buffer[i].Event.KeyEvent.dwControlKeyState & ENHANCED_KEY))
	    {
		GenerateConsoleCtrlEvent(CTRL_C_EVENT, GetCurrentProcessId());
		/* FIXME: this is hackish, but it easily disables IR handling afterwards */
		buffer[i].Event.KeyEvent.uChar.UnicodeChar = 0;
	    }
	}
    }
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

    if ((ret = ReadConsoleW(hConsoleInput, ptr, nNumberOfCharsToRead, &ncr, 0)))
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


/******************************************************************************
 * ReadConsoleInputA [KERNEL32.@]  Reads data from a console
 *
 * PARAMS
 *    hConsoleInput        [I] Handle to console input buffer
 *    lpBuffer             [O] Address of buffer for read data
 *    nLength              [I] Number of records to read
 *    lpNumberOfEventsRead [O] Address of number of records read
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI ReadConsoleInputA(HANDLE hConsoleInput, LPINPUT_RECORD lpBuffer,
                              DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
    DWORD	nread;
    
    if (!ReadConsoleInputW(hConsoleInput, lpBuffer, nLength, &nread))
	return FALSE;
    
    /* FIXME for now, the low part of unicode would do as ASCII */
    if (lpNumberOfEventsRead) *lpNumberOfEventsRead = nread;
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


/***********************************************************************
 *            FlushConsoleInputBuffer   (KERNEL32.@)
 */
BOOL WINAPI FlushConsoleInputBuffer(HANDLE handle)
{
    return read_console_input(handle, NULL, 0, NULL, TRUE);
}


/***********************************************************************
 *            PeekConsoleInputA   (KERNEL32.@)
 *
 * Gets 'count' first events (or less) from input queue.
 *
 * Does not need a complex console.
 */
BOOL WINAPI PeekConsoleInputA(HANDLE hConsoleInput, LPINPUT_RECORD pirBuffer, 
			      DWORD cInRecords, LPDWORD lpcRead)
{
    /* FIXME: Hmm. Fix this if we get UNICODE input. */
    return PeekConsoleInputW(hConsoleInput, pirBuffer, cInRecords, lpcRead);
}


/***********************************************************************
 *            PeekConsoleInputW   (KERNEL32.@)
 */
BOOL WINAPI PeekConsoleInputW(HANDLE hConsoleInput, LPINPUT_RECORD pirBuffer, 
			      DWORD cInRecords, LPDWORD lpcRead)
{
    if (!cInRecords)
    {
        if (lpcRead) *lpcRead = 0;
        return TRUE;
    }
    return read_console_input(hConsoleInput, pirBuffer, cInRecords, lpcRead, FALSE);
}


/***********************************************************************
 *            GetNumberOfConsoleInputEvents   (KERNEL32.@)
 */
BOOL WINAPI GetNumberOfConsoleInputEvents(HANDLE hcon, LPDWORD nrofevents)
{
    return read_console_input(hcon, NULL, 0, nrofevents, FALSE);
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
 * WriteConsoleInputA [KERNEL32.@]  Write data to a console input buffer
 *
 */
BOOL WINAPI WriteConsoleInputA(HANDLE handle, INPUT_RECORD *buffer,
			       DWORD count, LPDWORD written)
{
    BOOL ret = TRUE;

    if (written) *written = 0;
    /* FIXME should zero out the non ASCII part for key events */

    while (count && ret)
    {
        DWORD len = min(count, REQUEST_MAX_VAR_SIZE/sizeof(INPUT_RECORD));
        SERVER_START_VAR_REQ(write_console_input, len * sizeof(INPUT_RECORD))
	{
	    req->handle = handle;
	    memcpy(server_data_ptr(req), buffer, len * sizeof(INPUT_RECORD));
	    if ((ret = !SERVER_CALL_ERR()))
	    {
		if (written) *written += req->written;
		count -= len;
		buffer += len;
	    }
	}
        SERVER_END_VAR_REQ;
    }
    return ret;
}

/******************************************************************************
 * WriteConsoleInputW [KERNEL32.@]  Write data to a console input buffer
 *
 */
BOOL WINAPI WriteConsoleInputW(HANDLE handle, INPUT_RECORD *buffer,
			       DWORD count, LPDWORD written)
{
    BOOL ret = TRUE;
    
    TRACE("(%d,%p,%ld,%p)\n", handle, buffer, count, written);
    
    if (written) *written = 0;
    while (count && ret)
    {
        DWORD len = min(count, REQUEST_MAX_VAR_SIZE/sizeof(INPUT_RECORD));
        SERVER_START_VAR_REQ(write_console_input, len * sizeof(INPUT_RECORD))
        {
            req->handle = handle;
            memcpy(server_data_ptr(req), buffer, len * sizeof(INPUT_RECORD));
            if ((ret = !SERVER_CALL_ERR()))
            {
                if (written) *written += req->written;
                count -= len;
                buffer += len;
            }
        }
        SERVER_END_VAR_REQ;
    }

    return ret;
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
static unsigned int console_ignore_ctrl_c = 0; /* FIXME: this should be inherited somehow */
static PHANDLER_ROUTINE handlers[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,CONSOLE_DefaultHandler};

BOOL WINAPI SetConsoleCtrlHandler(PHANDLER_ROUTINE func, BOOL add)
{
    int alloc_loop = sizeof(handlers)/sizeof(handlers[0]) - 1;
    
    FIXME("(%p,%i) - no error checking or testing yet\n", func, add);
    
    if (!func)
    {
	console_ignore_ctrl_c = add;
	return TRUE;
    }
    if (add)
    {
	for (; alloc_loop >= 0 && handlers[alloc_loop]; alloc_loop--);
	if (alloc_loop <= 0)
	{
	    FIXME("Out of space on CtrlHandler table\n");
	    return FALSE;
	}
	handlers[alloc_loop] = func;
    }
    else
    {
	for (; alloc_loop >= 0 && handlers[alloc_loop] != func; alloc_loop--);
	if (alloc_loop <= 0)
	{
	    WARN("Attempt to remove non-installed CtrlHandler %p\n", func);
	    return FALSE;
	}
	/* sanity check */
	if (alloc_loop == sizeof(handlers)/sizeof(handlers[0]) - 1)
	{
	    ERR("Who's trying to remove default handler???\n");
	    return FALSE;
	}
	if (alloc_loop)
	    memmove(&handlers[1], &handlers[0], alloc_loop * sizeof(handlers[0]));
	handlers[0] = 0;
    }
    return TRUE;
}

static WINE_EXCEPTION_FILTER(CONSOLE_CtrlEventHandler)
{
    TRACE("(%lx)\n", GetExceptionCode());
    return EXCEPTION_EXECUTE_HANDLER;
}

/******************************************************************************
 * GenerateConsoleCtrlEvent [KERNEL32.@] Simulate a CTRL-C or CTRL-BREAK
 *
 * PARAMS
 *    dwCtrlEvent        [I] Type of event
 *    dwProcessGroupID   [I] Process group ID to send event to
 *
 * NOTES
 *    Doesn't yet work...!
 *
 * RETURNS
 *    Success: True
 *    Failure: False (and *should* [but doesn't] set LastError)
 */
BOOL WINAPI GenerateConsoleCtrlEvent(DWORD dwCtrlEvent,
				     DWORD dwProcessGroupID)
{
    BOOL	dbgOn = FALSE;

    if (dwCtrlEvent != CTRL_C_EVENT && dwCtrlEvent != CTRL_BREAK_EVENT)
    {
	ERR("invalid event %ld for PGID %ld\n", dwCtrlEvent, dwProcessGroupID);
	return FALSE;
    }

    if (dwProcessGroupID == GetCurrentProcessId() || dwProcessGroupID == 0)
    {
	int	i;
	
	FIXME("Attempt to send event %ld to self groupID, doing locally only\n", dwCtrlEvent);
	
	/* this is only meaningfull when done locally, otherwise it will have to be done on
	 * the 'receive' side of the event generation
	 */
	if (dwCtrlEvent == CTRL_C_EVENT && console_ignore_ctrl_c)
	    return TRUE;

	/* if the program is debugged, then generate an exception to the debugger first */
	SERVER_START_REQ( get_process_info )
	{
	    req->handle = GetCurrentProcess();
	    if (!SERVER_CALL_ERR()) dbgOn = req->debugged;
	}
	SERVER_END_REQ;

	if (dbgOn && (dwCtrlEvent == CTRL_C_EVENT || dwCtrlEvent == CTRL_BREAK_EVENT))
	{
	    /* the debugger is running... so try to pass the exception to it
	     * if it continues, there's nothing more to do
	     * otherwise, we need to send the ctrl-event to the handlers
	     */
	    BOOL	seen;
	    __TRY
	    {
		seen = TRUE;
		RaiseException((dwCtrlEvent == CTRL_C_EVENT) ? DBG_CONTROL_C : DBG_CONTROL_BREAK, 0, 0, NULL);
	    }
	    __EXCEPT(CONSOLE_CtrlEventHandler)
	    {
		/* the debugger didn't continue... so, pass to ctrl handlers */
		seen = FALSE;
	    }
	    __ENDTRY;
	    if (seen) return TRUE;
	}

	/* proceed with installed handlers */
	for (i = 0; i < sizeof(handlers)/sizeof(handlers[0]); i++)
	{
	    if (handlers[i] && (handlers[i])(dwCtrlEvent)) break;
	}
	
	return TRUE;
    }
    FIXME("event %ld to external PGID %ld - not implemented yet\n", 
	  dwCtrlEvent, dwProcessGroupID);
    return FALSE;
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
	if (!SERVER_CALL_ERR())
	    ret = req->handle_out;
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
	req->handle = (handle_t)hConsoleOutput;
	if ((ret = !SERVER_CALL_ERR()))
	{
	    csbi->dwSize.X              = req->width;
	    csbi->dwSize.Y              = req->height;
	    csbi->dwCursorPosition.X    = req->cursor_x;
	    csbi->dwCursorPosition.Y    = req->cursor_y;
	    csbi->wAttributes           = req->attr;
	    csbi->srWindow.Left	        = req->win_left;
	    csbi->srWindow.Right        = req->win_right;
	    csbi->srWindow.Top	        = req->win_top;
	    csbi->srWindow.Bottom       = req->win_bottom;
	    csbi->dwMaximumWindowSize.X = req->max_width;
	    csbi->dwMaximumWindowSize.Y = req->max_height;
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
    BOOL	ret;
    
    TRACE("(%x)\n", hConsoleOutput);
    
    SERVER_START_VAR_REQ(set_console_input_info, 0)
    {
	req->handle    = 0;
	req->mask      = SET_CONSOLE_INPUT_INFO_ACTIVE_SB;
	req->active_sb = hConsoleOutput;
	
	ret = !SERVER_CALL_ERR();
    }
    SERVER_END_VAR_REQ;
    
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
    COORD c;
    c.X = 80;
    c.Y = 24;
    return *(DWORD *)&c;
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
    return c;
}
#endif /* defined(__i386__) */


/******************************************************************************
 * GetConsoleCP [KERNEL32.@]  Returns the OEM code page for the console
 *
 * RETURNS
 *    Code page code
 */
UINT WINAPI GetConsoleCP(VOID)
{
    return GetACP();
}


/******************************************************************************
 *  SetConsoleCP	 [KERNEL32.@]
 * 
 * BUGS
 *   Unimplemented
 */
BOOL WINAPI SetConsoleCP(UINT cp)
{
    FIXME("(%d): stub\n", cp);
    
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *            GetConsoleOutputCP   (KERNEL32.@)
 */
UINT WINAPI GetConsoleOutputCP(VOID)
{
    return GetConsoleCP();
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
    FIXME("stub\n");
    return TRUE;
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
	ret = !SERVER_CALL_ERR();
	if (ret && mode) *mode = req->mode;
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
 */
BOOL WINAPI SetConsoleMode(HANDLE hcon, DWORD mode)
{
    BOOL ret;
    
    TRACE("(%x,%lx)\n", hcon, mode);
    
    SERVER_START_REQ(set_console_mode)
    {
	req->handle = hcon;
	req->mode = mode;
	ret = !SERVER_CALL_ERR();
    }
    SERVER_END_REQ;
    /* FIXME: when resetting a console input to editline mode, I think we should
     * empty the S_EditString buffer
     */
    return ret;
}


/***********************************************************************
 *            SetConsoleTitleA   (KERNEL32.@)
 *
 * Sets the console title.
 *
 * We do not necessarily need to create a complex console for that,
 * but should remember the title and set it on creation of the latter.
 * (not fixed at this time).
 */
BOOL WINAPI SetConsoleTitleA(LPCSTR title)
{
    LPWSTR	titleW = NULL;
    BOOL	ret;
    DWORD	len;
    
    len = MultiByteToWideChar(CP_ACP, 0, title, -1, NULL, 0);
    titleW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!titleW) return FALSE;
    
    MultiByteToWideChar(CP_ACP, 0, title, -1, titleW, len);
    ret = SetConsoleTitleW(titleW);
    
    HeapFree(GetProcessHeap(), 0, titleW);
    return ret;
}


/******************************************************************************
 * SetConsoleTitleW [KERNEL32.@]  Sets title bar string for console
 *
 * PARAMS
 *    title [I] Address of new title
 *
 * NOTES
 *    This should not be calling the A version
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetConsoleTitleW(LPCWSTR title)
{
    size_t	len = strlenW(title) * sizeof(WCHAR);
    BOOL 	ret;

    len = min(len, REQUEST_MAX_VAR_SIZE);
    SERVER_START_VAR_REQ(set_console_input_info, len)
    {
	req->handle = 0;
        req->mask = SET_CONSOLE_INPUT_INFO_TITLE;
        memcpy(server_data_ptr(req), title, len);
        ret = !SERVER_CALL_ERR();
    }
    SERVER_END_VAR_REQ;

    return ret;
}

/***********************************************************************
 *            GetConsoleTitleA   (KERNEL32.@)
 */
DWORD WINAPI GetConsoleTitleA(LPSTR title, DWORD size)
{
    WCHAR*	ptr = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * size);
    DWORD	ret;

    if (!ptr) return 0;
    
    ret = GetConsoleTitleW(ptr, size);
    if (ret) WideCharToMultiByte(CP_ACP, 0, ptr, ret + 1, title, size, NULL, NULL);

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

    SERVER_START_VAR_REQ(get_console_input_info, REQUEST_MAX_VAR_SIZE)
    {
	req->handle = 0;
        if (!SERVER_CALL_ERR())
        {
            ret = server_data_size(req) / sizeof(WCHAR);
            size = min(size - 1, ret);
            memcpy(title, server_data_ptr(req), size * sizeof(WCHAR));
            title[size] = 0;
        }
    }
    SERVER_END_VAR_REQ;

    return ret;
}

/******************************************************************
 *		write_char
 *
 * WriteConsoleOutput helper: hides server call semantics
 */
static	int	write_char(HANDLE hCon, LPCVOID lpBuffer, int nc, COORD* pos)
{
    BOOL	ret;
    int		written = -1;

    if (!nc) return 0;

    assert(nc * sizeof(WCHAR) <= REQUEST_MAX_VAR_SIZE);

    SERVER_START_VAR_REQ(write_console_output, nc * sizeof(WCHAR))
    {
	req->handle = hCon;
	req->x      = pos->X;
	req->y      = pos->Y;
	req->mode   = WRITE_CONSOLE_MODE_TEXTSTDATTR;
	memcpy(server_data_ptr(req), lpBuffer, nc * sizeof(WCHAR));
	if ((ret = !SERVER_CALL_ERR()))
	{
	    written = req->written;
	}
    }
    SERVER_END_VAR_REQ;
    
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
 *
 */
static int     	write_block(HANDLE hCon, CONSOLE_SCREEN_BUFFER_INFO* csbi,
			    DWORD mode, LPWSTR ptr, int len)
{
    int	blk;	/* number of chars to write on first line */

    if (len <= 0) return 1;

    blk = min(len, csbi->dwSize.X - csbi->dwCursorPosition.X);

    if (write_char(hCon, ptr, blk, &csbi->dwCursorPosition) != blk)
	return 0;

    if (blk < len) /* special handling for right border */
    {
	if (mode & ENABLE_WRAP_AT_EOL_OUTPUT) /* writes remaining on next line */
	{
	    if (!next_line(hCon, csbi) ||
		write_char(hCon, ptr + blk, len - blk, &csbi->dwCursorPosition) != len - blk)
		return 0;
	}
	else /* all remaining chars should be written on last column, so only write the last one */
	{
	    csbi->dwCursorPosition.X = csbi->dwSize.X - 1;
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

/***********************************************************************
 *            WriteConsoleOutputA   (KERNEL32.@)
 */
BOOL WINAPI WriteConsoleOutputA(HANDLE hConsoleOutput, LPCHAR_INFO lpBuffer, COORD dwBufferSize,
				COORD dwBufferCoord, LPSMALL_RECT lpWriteRegion)
{
    CHAR_INFO	*ciw;
    int		i;
    BOOL	ret;
    
    ciw = HeapAlloc(GetProcessHeap(), 0, sizeof(CHAR_INFO) * dwBufferSize.X * dwBufferSize.Y);
    if (!ciw) return FALSE;
    
    for (i = 0; i < dwBufferSize.X * dwBufferSize.Y; i++)
    {
	ciw[i].Attributes = lpBuffer[i].Attributes;
	MultiByteToWideChar(CP_ACP, 0, &lpBuffer[i].Char.AsciiChar, 1, &ciw[i].Char.UnicodeChar, 1);
    }
    ret = WriteConsoleOutputW(hConsoleOutput, ciw, dwBufferSize, dwBufferCoord, lpWriteRegion);
    HeapFree(GetProcessHeap(), 0, ciw);
    
    return ret;
}

/***********************************************************************
 *            WriteConsoleOutputW   (KERNEL32.@)
 */
BOOL WINAPI WriteConsoleOutputW(HANDLE hConsoleOutput, LPCHAR_INFO lpBuffer, COORD dwBufferSize,
				COORD dwBufferCoord, LPSMALL_RECT lpWriteRegion)
{
    short int	w, h;
    unsigned	y;
    DWORD	ret = TRUE;
    DWORD	actual_width;
    
    TRACE("(%x,%p,(%d,%d),(%d,%d),(%d,%dx%d,%d)\n", 
	  hConsoleOutput, lpBuffer, dwBufferSize.X, dwBufferSize.Y, dwBufferCoord.X, dwBufferCoord.Y,
	  lpWriteRegion->Left, lpWriteRegion->Top, lpWriteRegion->Right, lpWriteRegion->Bottom);
    
    w = min(lpWriteRegion->Right - lpWriteRegion->Left + 1, dwBufferSize.X - dwBufferCoord.X);
    h = min(lpWriteRegion->Bottom - lpWriteRegion->Top + 1, dwBufferSize.Y - dwBufferCoord.Y);
    
    if (w <= 0 || h <= 0)
    {
	memset(lpWriteRegion, 0, sizeof(SMALL_RECT));
	return FALSE;
    }
    
    /* this isn't supported for now, even if hConsoleOutput's row size fits in a single
     * server's request... it would request cropping on client side
     */
    if (w * sizeof(CHAR_INFO) > REQUEST_MAX_VAR_SIZE)
    {
	FIXME("This isn't supported yet, too wide CHAR_INFO array (%d)\n", w);
	memset(lpWriteRegion, 0, sizeof(SMALL_RECT));
	return FALSE;
    }

    actual_width = w;
    for (y = 0; ret && y < h; y++)
    {
	SERVER_START_VAR_REQ(write_console_output, w * sizeof(CHAR_INFO))
	{
	    req->handle = hConsoleOutput;
	    req->mode = WRITE_CONSOLE_MODE_TEXTATTR;
	    req->x = lpWriteRegion->Left;
	    req->y = lpWriteRegion->Top + y;
	    memcpy(server_data_ptr(req), &lpBuffer[(y + dwBufferCoord.Y) * dwBufferSize.X + dwBufferCoord.X], w * sizeof(CHAR_INFO));
	    if ((ret = !SERVER_CALL()))
		actual_width = min(actual_width, req->written);
	}
	SERVER_END_VAR_REQ;
    }
    lpWriteRegion->Bottom = lpWriteRegion->Top + h;
    lpWriteRegion->Right = lpWriteRegion->Left + actual_width;

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
        ret = !SERVER_CALL_ERR();
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
        ret = !SERVER_CALL_ERR();
        if (ret && cinfo)
        {
            cinfo->dwSize = req->cursor_size;
            cinfo->bVisible = req->cursor_visible;
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
        ret = !SERVER_CALL_ERR();
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
        ret = !SERVER_CALL_ERR();
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
        req->handle         = hConsoleOutput;
        req->attr	    = wAttr;
        req->mask           = SET_CONSOLE_OUTPUT_INFO_ATTR;
        ret = !SERVER_CALL_ERR();
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

    /* FIXME: most code relies on the fact we can transfer a complete row at a time...
     * so check if it's possible...
     */
    if (dwSize.X > REQUEST_MAX_VAR_SIZE / 4)
    {
	FIXME("too wide width not supported\n");
	SetLastError(STATUS_INVALID_PARAMETER);
	return FALSE;
    }

    SERVER_START_REQ(set_console_output_info)
    {
        req->handle	= hConsoleOutput;
        req->width	= dwSize.X;
        req->height	= dwSize.Y;
	req->mask       = SET_CONSOLE_OUTPUT_INFO_SIZE;
        ret = !SERVER_CALL_ERR();
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 * FillConsoleOutputCharacterA [KERNEL32.@]
 *
 * PARAMS
 *    hConsoleOutput    [I] Handle to screen buffer
 *    cCharacter        [I] Character to write
 *    nLength           [I] Number of cells to write to
 *    dwCoord           [I] Coords of first cell
 *    lpNumCharsWritten [O] Pointer to number of cells written
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI FillConsoleOutputCharacterA(HANDLE hConsoleOutput, BYTE cCharacter,
					DWORD nLength, COORD dwCoord, LPDWORD lpNumCharsWritten)
{
    WCHAR	wch;

    MultiByteToWideChar(CP_ACP, 0, &cCharacter, 1, &wch, 1);

    return FillConsoleOutputCharacterW(hConsoleOutput, wch, nLength, dwCoord, lpNumCharsWritten);
}


/******************************************************************************
 * FillConsoleOutputCharacterW [KERNEL32.@]  Writes characters to console
 *
 * PARAMS
 *    hConsoleOutput    [I] Handle to screen buffer
 *    cCharacter        [I] Character to write
 *    nLength           [I] Number of cells to write to
 *    dwCoord           [I] Coords of first cell
 *    lpNumCharsWritten [O] Pointer to number of cells written
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI FillConsoleOutputCharacterW(HANDLE hConsoleOutput, WCHAR cCharacter,
					DWORD nLength, COORD dwCoord, LPDWORD lpNumCharsWritten)
{
    CONSOLE_SCREEN_BUFFER_INFO	csbi;
    int				written;
    DWORD			initLen = nLength;

    TRACE("(%d,%s,%ld,(%dx%d),%p)\n", 
	  hConsoleOutput, debugstr_wn(&cCharacter, 1), nLength, 
	  dwCoord.X, dwCoord.Y, lpNumCharsWritten);

    if (!GetConsoleScreenBufferInfo(hConsoleOutput, &csbi))
	return FALSE;

    while (nLength)
    {
	SERVER_START_VAR_REQ(write_console_output, 
			     min(csbi.dwSize.X - dwCoord.X, nLength) * sizeof(WCHAR))
	{
	    req->handle = hConsoleOutput;
	    req->x      = dwCoord.X;
	    req->y      = dwCoord.Y;
	    req->mode   = WRITE_CONSOLE_MODE_TEXTSTDATTR|WRITE_CONSOLE_MODE_UNIFORM;
	    memcpy(server_data_ptr(req), &cCharacter, sizeof(WCHAR));
	    written = SERVER_CALL_ERR() ? 0 : req->written;
	}
	SERVER_END_VAR_REQ;

	if (!written) break;
	nLength -= written;
	dwCoord.X = 0;
	if (++dwCoord.Y == csbi.dwSize.Y) break;
    }
    
    if (lpNumCharsWritten) *lpNumCharsWritten = initLen - nLength;
    return initLen != nLength;
}


/******************************************************************************
 * FillConsoleOutputAttribute [KERNEL32.@]  Sets attributes for console
 *
 * PARAMS
 *    hConsoleOutput    [I] Handle to screen buffer
 *    wAttribute        [I] Color attribute to write
 *    nLength           [I] Number of cells to write to
 *    dwCoord           [I] Coords of first cell
 *    lpNumAttrsWritten [O] Pointer to number of cells written
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI FillConsoleOutputAttribute(HANDLE hConsoleOutput, WORD wAttribute, 
				       DWORD nLength, COORD dwCoord, LPDWORD lpNumAttrsWritten)
{
    CONSOLE_SCREEN_BUFFER_INFO	csbi;
    int				written;
    DWORD			initLen = nLength;
    
    TRACE("(%d,%d,%ld,(%dx%d),%p)\n", 
	  hConsoleOutput, wAttribute, nLength, dwCoord.X, dwCoord.Y, lpNumAttrsWritten);
    
    if (!GetConsoleScreenBufferInfo(hConsoleOutput, &csbi))
	return FALSE;

    while (nLength)
    {
	SERVER_START_VAR_REQ(write_console_output, 
			     min(csbi.dwSize.X - dwCoord.X, nLength) * sizeof(WCHAR))
	{
	    req->handle = hConsoleOutput;
	    req->x      = dwCoord.X;
	    req->y      = dwCoord.Y;
	    req->mode   = WRITE_CONSOLE_MODE_ATTR|WRITE_CONSOLE_MODE_UNIFORM;
	    memcpy(server_data_ptr(req), &wAttribute, sizeof(WORD));
	    written = SERVER_CALL_ERR() ? 0 : req->written;
	}
	SERVER_END_VAR_REQ;

	if (!written) break;
	nLength -= written;
	dwCoord.X = 0;
	if (++dwCoord.Y == csbi.dwSize.Y) break;
    }

    if (lpNumAttrsWritten) *lpNumAttrsWritten = initLen - nLength;
    return initLen != nLength;
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
 *		fill_line_uniform
 *
 * Helper function for ScrollConsoleScreenBufferW
 * Fills a part of a line with a constant character info
 */
static void fill_line_uniform(HANDLE hConsoleOutput, int i, int j, int len, LPCHAR_INFO lpFill)
{
    SERVER_START_VAR_REQ(write_console_output, len * sizeof(CHAR_INFO))
    {
	req->handle = hConsoleOutput;
	req->x      = i;
	req->y      = j;
	req->mode   = WRITE_CONSOLE_MODE_TEXTATTR|WRITE_CONSOLE_MODE_UNIFORM;
	memcpy(server_data_ptr(req), lpFill, sizeof(CHAR_INFO));
	SERVER_CALL_ERR();
    }
    SERVER_END_VAR_REQ;
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
	ret = !SERVER_CALL_ERR();
    }
    SERVER_END_REQ;

    if (!ret) return FALSE;

    /* step 4: clean out the exposed part */

    /* have to write celll [i,j] if it is not in dst rect (because it has already
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
		    fill_line_uniform(hConsoleOutput, start, j, i - start, lpFill);
		    start = -1;
		}
	    }
	    else
	    {
		if (start == -1) start = i;
	    }
	}
	if (start != -1)
	    fill_line_uniform(hConsoleOutput, start, j, i - start, lpFill);
    }

    return TRUE;
}

/******************************************************************************
 * ReadConsoleOutputCharacterA [KERNEL32.@]
 * 
 */
BOOL WINAPI ReadConsoleOutputCharacterA(HANDLE hConsoleOutput, LPSTR lpstr, DWORD toRead, 
					COORD coord, LPDWORD lpdword)
{
    DWORD	read;
    LPWSTR	wptr = HeapAlloc(GetProcessHeap(), 0, toRead * sizeof(WCHAR));
    BOOL	ret;

    if (lpdword) *lpdword = 0;
    if (!wptr) return FALSE;

    ret = ReadConsoleOutputCharacterW(hConsoleOutput, wptr, toRead, coord, &read);

    read = WideCharToMultiByte(CP_ACP, 0, wptr, read, lpstr, toRead, NULL, NULL);
    if (lpdword) *lpdword = read;

    HeapFree(GetProcessHeap(), 0, wptr);

    return ret;
}

/******************************************************************************
 * ReadConsoleOutputCharacterW [KERNEL32.@]
 * 
 */
BOOL WINAPI ReadConsoleOutputCharacterW(HANDLE hConsoleOutput, LPWSTR lpstr, DWORD toRead, 
					COORD coord, LPDWORD lpdword)
{
    DWORD	read = 0;
    DWORD	ret = TRUE;
    int		i;
    DWORD*	ptr;

    TRACE("(%d,%p,%ld,%dx%d,%p)\n", hConsoleOutput, lpstr, toRead, coord.X, coord.Y, lpdword);

    while (ret && (read < toRead))
    {
	SERVER_START_VAR_REQ(read_console_output, REQUEST_MAX_VAR_SIZE)
	{
	    req->handle       = (handle_t)hConsoleOutput;
	    req->x            = coord.X;
	    req->y            = coord.Y;
	    req->w            = REQUEST_MAX_VAR_SIZE / 4;
	    req->h            = 1;
	    if ((ret = !SERVER_CALL_ERR()))
	    {
		ptr = server_data_ptr(req);
		
		for (i = 0; i < req->eff_w && read < toRead; i++)
		{
		    lpstr[read++] = LOWORD(ptr[i]);
		}
		coord.X = 0;	coord.Y++;
	    }
	}
	SERVER_END_VAR_REQ;
    }
    if (lpdword) *lpdword = read;
    
    TRACE("=> %lu %s\n", read, debugstr_wn(lpstr, read));
    
    return ret;
}


/******************************************************************************
 *  ReadConsoleOutputA [KERNEL32.@]
 * 
 */
BOOL WINAPI ReadConsoleOutputA(HANDLE hConsoleOutput, LPCHAR_INFO lpBuffer, COORD dwBufferSize,
			       COORD dwBufferCoord, LPSMALL_RECT lpReadRegion)
{
    BOOL	ret;
    int		x, y;
    int		pos;
    
    ret = ReadConsoleOutputW(hConsoleOutput, lpBuffer, dwBufferSize, dwBufferCoord, lpReadRegion);
    if (!ret) return FALSE;
    for (y = 0; y <= lpReadRegion->Bottom - lpReadRegion->Top; y++)
    {
	for (x = 0; x <= lpReadRegion->Right - lpReadRegion->Left; x++)
	{
	    pos = (dwBufferCoord.Y + y) * dwBufferSize.X + dwBufferCoord.X + x;
	    WideCharToMultiByte(CP_ACP, 0, &lpBuffer[pos].Char.UnicodeChar, 1, 
				&lpBuffer[pos].Char.AsciiChar, 1, NULL, NULL);
	}
    }
    return TRUE;
}

/******************************************************************************
 *  ReadConsoleOutputW [KERNEL32.@]
 * 
 */
BOOL WINAPI ReadConsoleOutputW(HANDLE hConsoleOutput, LPCHAR_INFO lpBuffer, COORD dwBufferSize,
			       COORD dwBufferCoord, LPSMALL_RECT lpReadRegion)
{
    int		w, h;
    int		actual_width;
    int		y;
    BOOL	ret = TRUE;
    
    w = min(lpReadRegion->Right - lpReadRegion->Left + 1, dwBufferSize.X - dwBufferCoord.X);
    h = min(lpReadRegion->Bottom - lpReadRegion->Top + 1, dwBufferSize.Y - dwBufferCoord.Y);
    
    if (w <= 0 || h <= 0) goto got_err;

    /* this isn't supported for now, even if hConsoleOutput's row size fits in a single
     * server's request... it would request cropping on client side
     */
    if (w * sizeof(CHAR_INFO) > REQUEST_MAX_VAR_SIZE)
    {
	FIXME("This isn't supported yet, too wide CHAR_INFO array (%d)\n", w);
	goto got_err;
    }

    actual_width = w;
    for (y = 0; ret && y < h; y++)
    {
	SERVER_START_VAR_REQ(read_console_output, w * sizeof(CHAR_INFO))
	{
	    req->handle = hConsoleOutput;
	    req->x = lpReadRegion->Left;
	    req->y = lpReadRegion->Top;
	    req->w = w;
	    req->h = 1;
	    if ((ret = !SERVER_CALL()))
	    {
		actual_width = min(actual_width, req->eff_w);
		memcpy(&lpBuffer[(y + dwBufferCoord.Y) * dwBufferSize.X + dwBufferCoord.X], 
		       server_data_ptr(req), 
		       req->eff_w * sizeof(CHAR_INFO));
	    }
	}
	SERVER_END_VAR_REQ;
    }
    if (!ret) goto got_err;
    
    lpReadRegion->Bottom = lpReadRegion->Top + y;
    lpReadRegion->Right = lpReadRegion->Left + actual_width;
    
    return ret;
 got_err:
    memset(lpReadRegion, 0, sizeof(SMALL_RECT));
    return FALSE;
}

/******************************************************************************
 *  ReadConsoleOutputAttribute [KERNEL32.@]
 * 
 */
BOOL WINAPI ReadConsoleOutputAttribute(HANDLE hConsoleOutput, LPWORD lpAttribute, DWORD nLength,
				       COORD coord, LPDWORD lpNumberOfAttrsRead)
{
    DWORD	read = 0;
    DWORD	ret = TRUE;
    int		i;
    DWORD*	ptr;
    
    TRACE("(%d,%p,%ld,%dx%d,%p)\n", 
	  hConsoleOutput, lpAttribute, nLength, coord.X, coord.Y, lpNumberOfAttrsRead);
    
    while (ret && (read < nLength))
    {
	SERVER_START_VAR_REQ(read_console_output, REQUEST_MAX_VAR_SIZE)
	{
	    req->handle       = (handle_t)hConsoleOutput;
	    req->x            = coord.X;
	    req->y            = coord.Y;
	    req->w            = REQUEST_MAX_VAR_SIZE / 4;
	    req->h            = 1;
	    if (SERVER_CALL_ERR()) 
	    {
		ret = FALSE;
	    }
	    else
	    {
		ptr = server_data_ptr(req);
		
		for (i = 0; i < req->eff_w && read < nLength; i++)
		{
		    lpAttribute[read++] = HIWORD(ptr[i]);
		}
		coord.X = 0;	coord.Y++;
	    }
	}
	SERVER_END_VAR_REQ;
    }
    if (lpNumberOfAttrsRead) *lpNumberOfAttrsRead = read;
    
    return ret;
}

/******************************************************************************
 * WriteConsoleOutputAttribute [KERNEL32.@]  Sets attributes for some cells in
 * 					     the console screen buffer
 *
 * PARAMS
 *    hConsoleOutput    [I] Handle to screen buffer
 *    lpAttribute       [I] Pointer to buffer with write attributes
 *    nLength           [I] Number of cells to write to
 *    dwCoord           [I] Coords of first cell
 *    lpNumAttrsWritten [O] Pointer to number of cells written
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 * 
 */
BOOL WINAPI WriteConsoleOutputAttribute(HANDLE hConsoleOutput, CONST WORD *lpAttribute, 
					DWORD nLength, COORD dwCoord, LPDWORD lpNumAttrsWritten)
{
    int		written = 0;
    int		len;
    BOOL	ret = TRUE;
    DWORD	init_len = nLength;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    
    TRACE("(%d,%p,%ld,%dx%d,%p)\n", hConsoleOutput,
          lpAttribute,nLength,dwCoord.X,dwCoord.Y,lpNumAttrsWritten);
    
    if (!GetConsoleScreenBufferInfo(hConsoleOutput, & csbi))
	return FALSE;
    
    while (ret && nLength)
    {	
	len = min(nLength * sizeof(WORD), REQUEST_MAX_VAR_SIZE);
	SERVER_START_VAR_REQ(write_console_output, len)
	{
	    req->handle = hConsoleOutput;
	    req->x      = dwCoord.X;
	    req->y      = dwCoord.Y;
	    req->mode   = WRITE_CONSOLE_MODE_ATTR;
	    memcpy(server_data_ptr(req), &lpAttribute[written],  len);
	    written = (SERVER_CALL_ERR()) ? 0 : req->written;
	}
	SERVER_END_VAR_REQ;

	if (!written) break;
	nLength -= written;
	dwCoord.X = 0;
	if (++dwCoord.Y == csbi.dwSize.Y) break;
    }

    if (lpNumAttrsWritten) *lpNumAttrsWritten = init_len - nLength;
    return nLength != init_len;
}

/******************************************************************************
 * WriteConsoleOutputCharacterA [KERNEL32.@]  Copies character to consecutive
 * 					      cells in the console screen buffer
 *
 * PARAMS
 *    hConsoleOutput    [I] Handle to screen buffer
 *    lpCharacter       [I] Pointer to buffer with chars to write
 *    nLength           [I] Number of cells to write to
 *    dwCoord           [I] Coords of first cell
 *    lpNumCharsWritten [O] Pointer to number of cells written
 */
BOOL WINAPI WriteConsoleOutputCharacterA(HANDLE hConsoleOutput, LPCSTR lpCharacter, DWORD nLength, 
					 COORD dwCoord, LPDWORD lpNumCharsWritten)
{
    BOOL	ret;
    LPWSTR	xstring;
    DWORD 	n;
    
    TRACE("(%d,%s,%ld,%dx%d,%p)\n", hConsoleOutput,
          debugstr_an(lpCharacter, nLength), nLength, dwCoord.X, dwCoord.Y, lpNumCharsWritten);
    
    n = MultiByteToWideChar(CP_ACP, 0, lpCharacter, nLength, NULL, 0);
    
    if (lpNumCharsWritten) *lpNumCharsWritten = 0;
    xstring = HeapAlloc(GetProcessHeap(), 0, n * sizeof(WCHAR));
    if (!xstring) return FALSE;
    
    MultiByteToWideChar(CP_ACP, 0, lpCharacter, nLength, xstring, n);
    
    ret = WriteConsoleOutputCharacterW(hConsoleOutput, xstring, n, dwCoord, lpNumCharsWritten);
    
    HeapFree(GetProcessHeap(), 0, xstring);
    
    return ret;
}

/******************************************************************************
 * WriteConsoleOutputCharacterW [KERNEL32.@]  Copies character to consecutive
 * 					      cells in the console screen buffer
 *
 * PARAMS
 *    hConsoleOutput    [I] Handle to screen buffer
 *    lpCharacter       [I] Pointer to buffer with chars to write
 *    nLength           [I] Number of cells to write to
 *    dwCoord           [I] Coords of first cell
 *    lpNumCharsWritten [O] Pointer to number of cells written
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 * 
 */
BOOL WINAPI WriteConsoleOutputCharacterW(HANDLE hConsoleOutput, LPCWSTR lpCharacter, DWORD nLength, 
					 COORD dwCoord, LPDWORD lpNumCharsWritten)
{
    int		written = 0;
    int		len;
    DWORD	init_len = nLength;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    
    TRACE("(%d,%s,%ld,%dx%d,%p)\n", hConsoleOutput,
          debugstr_wn(lpCharacter, nLength), nLength, dwCoord.X, dwCoord.Y, lpNumCharsWritten);
    
    if (!GetConsoleScreenBufferInfo(hConsoleOutput, &csbi))
	return FALSE;
    
    while (nLength)
    {	
	len = min(nLength * sizeof(WCHAR), REQUEST_MAX_VAR_SIZE);
	SERVER_START_VAR_REQ(write_console_output, len)
	{
	    req->handle = hConsoleOutput;
	    req->x      = dwCoord.X;
	    req->y      = dwCoord.Y;
	    req->mode   = WRITE_CONSOLE_MODE_TEXT;
	    memcpy(server_data_ptr(req), &lpCharacter[written], len);
	    written = (SERVER_CALL_ERR()) ? 0 : req->written;
	}
	SERVER_END_VAR_REQ;

	if (!written) break;
	nLength -= written;
	dwCoord.X += written;
	if (dwCoord.X >= csbi.dwSize.X)
	{
	    dwCoord.X = 0;
	    if (++dwCoord.Y == csbi.dwSize.Y) break;
	}
    }

    if (lpNumCharsWritten) *lpNumCharsWritten = init_len - nLength;
    return nLength != init_len;
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
int	CONSOLE_GetHistory(int idx, WCHAR* buf, int buf_len)
{
    int		len = 0;

    SERVER_START_VAR_REQ(get_console_input_history, REQUEST_MAX_VAR_SIZE)
    {
	req->handle = 0;
	req->index = idx;
	if (!SERVER_CALL_ERR())
	{
	    len = server_data_size(req) / sizeof(WCHAR) + 1;
	    if (buf)
	    {
		len = min(len, buf_len);
		memcpy(buf, server_data_ptr(req), len * sizeof(WCHAR));
		buf[len - 1] = 0;
	    }
	}
    }		
    SERVER_END_VAR_REQ;
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

    while (len && (ptr[len - 1] == '\n' || ptr[len - 1] == '\r'))
	len--;

    len *= sizeof(WCHAR);
    SERVER_START_VAR_REQ(append_console_input_history, len)
    {
	req->handle = 0;
	memcpy(server_data_ptr(req), ptr, len);
	ret = !SERVER_CALL_ERR();
    }
    SERVER_END_VAR_REQ;
    return ret;
}

/******************************************************************
 *		CONSOLE_GetNumHistoryEntries
 *
 *
 */
unsigned CONSOLE_GetNumHistoryEntries(void)
{
    unsigned ret = 0;
    SERVER_START_REQ(get_console_input_info)
    {
	req->handle = 0;
	if (!SERVER_CALL_ERR()) ret = req->history_index;
    }
    SERVER_END_REQ;
    return ret;
}

