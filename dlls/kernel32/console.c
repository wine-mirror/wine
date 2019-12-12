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
#include <limits.h>
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
    BOOL        ret = TRUE;

    if (S_termios_raw)
    {
        if ((fd = get_console_bare_fd(hin)) == -1) return FALSE;
        ret = tcsetattr(fd, TCSANOW, &S_termios) >= 0;
        close(fd);
    }

    if (RtlGetCurrentPeb()->ProcessParameters->ConsoleHandle == KERNEL32_CONSOLE_SHELL)
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
                console_wait_event = wine_server_ptr_handle( reply->event );
        }
        SERVER_END_REQ;
    }
    return console_wait_event;
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
                idxw = MultiByteToWideChar(CP_UNIXCP, 0, input, idx, inputw, ARRAY_SIZE(inputw));

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
    DWORD	charsread;
    LPWSTR	xbuf = lpBuffer;
    DWORD	mode;
    BOOL        is_bare = FALSE;
    int         fd;

    TRACE("(%p,%p,%d,%p,%p)\n",
	  hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, lpReserved);

    if (nNumberOfCharsToRead > INT_MAX)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

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
 *            ReadConsoleInputA   (KERNEL32.@)
 */
BOOL WINAPI ReadConsoleInputA( HANDLE handle, INPUT_RECORD *buffer, DWORD length, DWORD *count )
{
    DWORD read;

    if (!ReadConsoleInputW( handle, buffer, length, &read )) return FALSE;
    input_records_WtoA( buffer, read );
    if (count) *count = read;
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
     * + INVALID_HANDLE_VALUE => TEB: 0, STARTUPINFO: INVALID_HANDLE_VALUE
     * + 0                    => TEB: 0, STARTUPINFO: INVALID_HANDLE_VALUE
     * + console handle needs to be mapped
     */
    if (!params->hStdInput || params->hStdInput == INVALID_HANDLE_VALUE)
        params->hStdInput = 0;
    else if (VerifyConsoleIoHandle(console_handle_map(params->hStdInput)))
    {
        params->hStdInput = console_handle_map(params->hStdInput);
        save_console_mode(params->hStdInput);
    }

    if (!params->hStdOutput || params->hStdOutput == INVALID_HANDLE_VALUE)
        params->hStdOutput = 0;
    else if (VerifyConsoleIoHandle(console_handle_map(params->hStdOutput)))
        params->hStdOutput = console_handle_map(params->hStdOutput);

    if (!params->hStdError || params->hStdError == INVALID_HANDLE_VALUE)
        params->hStdError = 0;
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
    BOOL ret;
    struct
    {
        unsigned int color_map[16];
        WCHAR face_name[LF_FACESIZE];
    } data;

    if (fontinfo->cbSize != sizeof(CONSOLE_FONT_INFOEX))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SERVER_START_REQ(get_console_output_info)
    {
        req->handle = console_handle_unmap(hConsole);
        wine_server_set_reply( req, &data, sizeof(data) - sizeof(WCHAR) );
        if ((ret = !wine_server_call_err(req)))
        {
            fontinfo->nFont = 0;
            if (maxwindow)
            {
                fontinfo->dwFontSize.X = min(reply->width, reply->max_width);
                fontinfo->dwFontSize.Y = min(reply->height, reply->max_height);
            }
            else
            {
                fontinfo->dwFontSize.X = reply->win_right - reply->win_left + 1;
                fontinfo->dwFontSize.Y = reply->win_bottom - reply->win_top + 1;
            }
            if (wine_server_reply_size( reply ) > sizeof(data.color_map))
            {
                data_size_t len = wine_server_reply_size( reply ) - sizeof(data.color_map);
                memcpy( fontinfo->FaceName, data.face_name, len );
                fontinfo->FaceName[len / sizeof(WCHAR)] = 0;
            }
            else
                fontinfo->FaceName[0] = 0;
            fontinfo->FontFamily = reply->font_pitch_family;
            fontinfo->FontWeight = reply->font_weight;
        }
    }
    SERVER_END_REQ;
    return ret;
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
    COORD c = {0,0};

    if (index >= GetNumberOfConsoleFonts())
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return c;
    }

    SERVER_START_REQ(get_console_output_info)
    {
        req->handle = console_handle_unmap(hConsole);
        if (!wine_server_call_err(req))
        {
            c.X = reply->font_width;
            c.Y = reply->font_height;
        }
    }
    SERVER_END_REQ;
    return c;
}

#if defined(__i386__) && !defined(__MINGW32__)
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
