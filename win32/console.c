/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 * Copyright 1997 Karl Garrison
 * Copyright 1998 John Richardson
 * Copyright 1998 Marcus Meissner
 */

/* FIXME:
 * - Completely lacks SCREENBUFFER interface.
 * - No abstraction for something other than xterm.
 * - Key input translation shouldn't use VkKeyScan and MapVirtualKey, since
 *   they are window (USER) driver dependend.
 * - Output sometimes is buffered (We switched off buffering by ~ICANON ?)
 */
/* Reference applications:
 * -  IDA (interactive disassembler) full version 3.75. Works.
 * -  LYNX/W32. Works mostly, some keys crash it.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#include <signal.h>
#include <assert.h>

#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "wine/keyboard16.h"
#include "thread.h"
#include "file.h"
#include "process.h"
#include "winerror.h"
#include "wincon.h"
#include "heap.h"
#include "server.h"
#include "debugtools.h"
#include "winnls.h"

DEFAULT_DEBUG_CHANNEL(console)


/* FIXME:  Should be in an internal header file.  OK, so which one?
   Used by CONSOLE_makecomplex. */
int wine_openpty(int *master, int *slave, char *name,
                 struct termios *term, struct winsize *winsize);

/****************************************************************************
 *		CONSOLE_GetPid
 */
static int CONSOLE_GetPid( HANDLE handle )
{
    struct get_console_info_request *req = get_req_buffer();
    req->handle = handle;
    if (server_call( REQ_GET_CONSOLE_INFO )) return 0;
    return req->pid;
}

/****************************************************************************
 *		XTERM_string_to_IR			[internal]
 *
 * Transfers a string read from XTERM to INPUT_RECORDs and adds them to the
 * queue. Does translation of vt100 style function keys and xterm-mouse clicks.
 */
static void
CONSOLE_string_to_IR( HANDLE hConsoleInput,unsigned char *buf,int len) {
    int			j,k;
    INPUT_RECORD	ir;
    DWORD		junk;

    for (j=0;j<len;j++) {
    	unsigned char inchar = buf[j];

	if (inchar!=27) { /* no escape -> 'normal' keyboard event */
	    ir.EventType = 1; /* Key_event */

	    ir.Event.KeyEvent.bKeyDown		= 1;
	    ir.Event.KeyEvent.wRepeatCount	= 0;

	    ir.Event.KeyEvent.dwControlKeyState	= 0;
	    if (inchar & 0x80) {
		ir.Event.KeyEvent.dwControlKeyState|=LEFT_ALT_PRESSED;
		inchar &= ~0x80;
	    }
#if 0  /* FIXME: cannot call USER functions here */
	    ir.Event.KeyEvent.wVirtualKeyCode = VkKeyScan16(inchar);
	    if (ir.Event.KeyEvent.wVirtualKeyCode & 0x0100)
		ir.Event.KeyEvent.dwControlKeyState|=SHIFT_PRESSED;
	    if (ir.Event.KeyEvent.wVirtualKeyCode & 0x0200)
		ir.Event.KeyEvent.dwControlKeyState|=LEFT_CTRL_PRESSED;
	    if (ir.Event.KeyEvent.wVirtualKeyCode & 0x0400)
		ir.Event.KeyEvent.dwControlKeyState|=LEFT_ALT_PRESSED;
	    ir.Event.KeyEvent.wVirtualScanCode = MapVirtualKey16(
	    	ir.Event.KeyEvent.wVirtualKeyCode & 0x00ff,
		0 /* VirtualKeyCodes to ScanCode */
	    );
#else
	    ir.Event.KeyEvent.wVirtualKeyCode = 0;
	    ir.Event.KeyEvent.wVirtualScanCode = 0;
#endif
	    ir.Event.KeyEvent.uChar.AsciiChar = inchar;

	    if ((inchar==127)||(inchar=='\b')) { /* backspace */
	    	ir.Event.KeyEvent.uChar.AsciiChar = '\b'; /* FIXME: hmm */
		ir.Event.KeyEvent.wVirtualScanCode = 0x0e;
		ir.Event.KeyEvent.wVirtualKeyCode = VK_BACK;
	    } else {
		if ((inchar=='\n')||(inchar=='\r')) {
		    ir.Event.KeyEvent.uChar.AsciiChar	= '\r';
		    ir.Event.KeyEvent.wVirtualKeyCode	= VK_RETURN;
		    ir.Event.KeyEvent.wVirtualScanCode	= 0x1c;
		    ir.Event.KeyEvent.dwControlKeyState = 0;
		} else {
		    if (inchar<' ') {
			/* FIXME: find good values for ^X */
			ir.Event.KeyEvent.wVirtualKeyCode = 0xdead;
			ir.Event.KeyEvent.wVirtualScanCode = 0xbeef;
		    } 
		}
	    }

	    assert(WriteConsoleInputA( hConsoleInput, &ir, 1, &junk ));
	    ir.Event.KeyEvent.bKeyDown = 0;
	    assert(WriteConsoleInputA( hConsoleInput, &ir, 1, &junk ));
	    continue;
	}
	/* inchar is ESC */
	if ((j==len-1) || (buf[j+1]!='[')) {/* add ESCape on its own */
	    ir.EventType = 1; /* Key_event */
	    ir.Event.KeyEvent.bKeyDown		= 1;
	    ir.Event.KeyEvent.wRepeatCount	= 0;

#if 0  /* FIXME: cannot call USER functions here */
	    ir.Event.KeyEvent.wVirtualKeyCode	= VkKeyScan16(27);
	    ir.Event.KeyEvent.wVirtualScanCode	= MapVirtualKey16(
	    	ir.Event.KeyEvent.wVirtualKeyCode,0
	    );
#else
	    ir.Event.KeyEvent.wVirtualKeyCode	= VK_ESCAPE;
	    ir.Event.KeyEvent.wVirtualScanCode	= 1;
#endif
	    ir.Event.KeyEvent.dwControlKeyState = 0;
	    ir.Event.KeyEvent.uChar.AsciiChar	= 27;
	    assert(WriteConsoleInputA( hConsoleInput, &ir, 1, &junk ));
	    ir.Event.KeyEvent.bKeyDown = 0;
	    assert(WriteConsoleInputA( hConsoleInput, &ir, 1, &junk ));
	    continue;
	}
	for (k=j;k<len;k++) {
	    if (((buf[k]>='A') && (buf[k]<='Z')) ||
		((buf[k]>='a') && (buf[k]<='z')) ||
	    	 (buf[k]=='~')
	    )
	    	break;
	}
	if (k<len) {
	    int	subid,scancode=0;

	    ir.EventType			= 1; /* Key_event */
	    ir.Event.KeyEvent.bKeyDown		= 1;
	    ir.Event.KeyEvent.wRepeatCount	= 0;
	    ir.Event.KeyEvent.dwControlKeyState	= 0;

	    ir.Event.KeyEvent.wVirtualKeyCode	= 0xad; /* FIXME */
	    ir.Event.KeyEvent.wVirtualScanCode	= 0xad; /* FIXME */
	    ir.Event.KeyEvent.uChar.AsciiChar	= 0;

	    switch (buf[k]) {
	    case '~':
		sscanf(&buf[j+2],"%d",&subid);
		switch (subid) {
		case  2:/*INS */scancode = 0xe052;break;
		case  3:/*DEL */scancode = 0xe053;break;
		case  6:/*PGDW*/scancode = 0xe051;break;
		case  5:/*PGUP*/scancode = 0xe049;break;
		case 11:/*F1  */scancode = 0x003b;break;
		case 12:/*F2  */scancode = 0x003c;break;
		case 13:/*F3  */scancode = 0x003d;break;
		case 14:/*F4  */scancode = 0x003e;break;
		case 15:/*F5  */scancode = 0x003f;break;
		case 17:/*F6  */scancode = 0x0040;break;
		case 18:/*F7  */scancode = 0x0041;break;
		case 19:/*F8  */scancode = 0x0042;break;
		case 20:/*F9  */scancode = 0x0043;break;
		case 21:/*F10 */scancode = 0x0044;break;
		case 23:/*F11 */scancode = 0x00d9;break;
		case 24:/*F12 */scancode = 0x00da;break;
		/* FIXME: Shift-Fx */
		default:
			FIXME("parse ESC[%d~\n",subid);
			break;
		}
		break;
	    case 'A': /* Cursor Up    */scancode = 0xe048;break;
	    case 'B': /* Cursor Down  */scancode = 0xe050;break;
	    case 'D': /* Cursor Left  */scancode = 0xe04b;break;
	    case 'C': /* Cursor Right */scancode = 0xe04d;break;
	    case 'F': /* End          */scancode = 0xe04f;break;
	    case 'H': /* Home         */scancode = 0xe047;break;
	    case 'M':
	    	/* Mouse Button Press  (ESCM<button+'!'><x+'!'><y+'!'>) or
		 *              Release (ESCM#<x+'!'><y+'!'>
		 */
		if (k<len-3) {
		    ir.EventType			= MOUSE_EVENT;
		    ir.Event.MouseEvent.dwMousePosition.X = buf[k+2]-'!';
		    ir.Event.MouseEvent.dwMousePosition.Y = buf[k+3]-'!';
		    if (buf[k+1]=='#')
			ir.Event.MouseEvent.dwButtonState = 0;
		    else
			ir.Event.MouseEvent.dwButtonState = 1<<(buf[k+1]-' ');
		    ir.Event.MouseEvent.dwEventFlags	  = 0; /* FIXME */
		    assert(WriteConsoleInputA( hConsoleInput, &ir, 1, &junk));
		    j=k+3;
		}
	    	break;
	    case 'c':
	    	j=k;
		break;
	    }
	    if (scancode) {
		ir.Event.KeyEvent.wVirtualScanCode = scancode;
#if 0  /* FIXME: cannot call USER functions here */
		ir.Event.KeyEvent.wVirtualKeyCode = MapVirtualKey16(scancode,1);
#else
		ir.Event.KeyEvent.wVirtualKeyCode = 0;
#endif
		assert(WriteConsoleInputA( hConsoleInput, &ir, 1, &junk ));
		ir.Event.KeyEvent.bKeyDown		= 0;
		assert(WriteConsoleInputA( hConsoleInput, &ir, 1, &junk ));
		j=k;
		continue;
	    }
	}
    }
}

/****************************************************************************
 * 		CONSOLE_get_input		(internal)
 *
 * Reads (nonblocking) as much input events as possible and stores them
 * in an internal queue.
 */
static void
CONSOLE_get_input( HANDLE handle, BOOL blockwait )
{
    char	*buf = HeapAlloc(GetProcessHeap(),0,1);
    int		len = 0, escape_seen = 0;

    while (1)
    {
	DWORD res;
	char inchar;

	/* If we have at one time seen escape in this loop, we are 
	 * within an Escape sequence, so wait for a bit more input for the
	 * rest of the loop.
	 */
        if (WaitForSingleObject( handle, escape_seen*10 )) break;
        if (!ReadFile( handle, &inchar, 1, &res, NULL )) break;
	if (!res) /* res 0 but readable means EOF? Hmm. */
		break;
	buf = HeapReAlloc(GetProcessHeap(),0,buf,len+1);
	buf[len++]=inchar;
	if (inchar == 27) {
		if (len>1) {
			/* If we spot an ESC, we flush all up to it
			 * since we can be sure that we have a complete
			 * sequence.
			 */
			CONSOLE_string_to_IR(handle,buf,len-1);
			buf = HeapReAlloc(GetProcessHeap(),0,buf,1);
			buf[0] = 27;
			len = 1;
		}
		escape_seen = 1;
	}
    }
    CONSOLE_string_to_IR(handle,buf,len);
    HeapFree(GetProcessHeap(),0,buf);
}

/******************************************************************************
 * SetConsoleCtrlHandler [KERNEL32.459]  Adds function to calling process list
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
static unsigned int console_ignore_ctrl_c = 0;
static HANDLER_ROUTINE *handlers[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
BOOL WINAPI SetConsoleCtrlHandler( HANDLER_ROUTINE *func, BOOL add )
{
  unsigned int alloc_loop = sizeof(handlers)/sizeof(HANDLER_ROUTINE *);
  unsigned int done = 0;
  FIXME("(%p,%i) - no error checking or testing yet\n", func, add);
  if (!func)
    {
      console_ignore_ctrl_c = add;
      return TRUE;
    }
  if (add)
      {
	for (;alloc_loop--;)
	  if (!handlers[alloc_loop] && !done)
	    {
	      handlers[alloc_loop] = func;
	      done++;
	    }
	if (!done)
	   FIXME("Out of space on CtrlHandler table\n");
	return(done);
      }
    else
      {
	for (;alloc_loop--;)
	  if (handlers[alloc_loop] == func && !done)
	    {
	      handlers[alloc_loop] = 0;
	      done++;
	    }
	if (!done)
	   WARN("Attempt to remove non-installed CtrlHandler %p\n",
		func);
	return (done);
      }
    return (done);
}


/******************************************************************************
 * GenerateConsoleCtrlEvent [KERNEL32.275] Simulate a CTRL-C or CTRL-BREAK
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
BOOL WINAPI GenerateConsoleCtrlEvent( DWORD dwCtrlEvent,
					DWORD dwProcessGroupID )
{
  if (dwCtrlEvent != CTRL_C_EVENT && dwCtrlEvent != CTRL_BREAK_EVENT)
    {
      ERR("invalid event %d for PGID %ld\n", 
	   (unsigned short)dwCtrlEvent, dwProcessGroupID );
      return FALSE;
    }
  if (dwProcessGroupID == GetCurrentProcessId() )
    {
      FIXME("Attempt to send event %d to self - stub\n",
	     (unsigned short)dwCtrlEvent );
      return FALSE;
    }
  FIXME("event %d to external PGID %ld - not implemented yet\n",
	 (unsigned short)dwCtrlEvent, dwProcessGroupID );
  return FALSE;
}


/******************************************************************************
 * CreateConsoleScreenBuffer [KERNEL32.151]  Creates a console screen buffer
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
HANDLE WINAPI CreateConsoleScreenBuffer( DWORD dwDesiredAccess,
                DWORD dwShareMode, LPSECURITY_ATTRIBUTES sa,
                DWORD dwFlags, LPVOID lpScreenBufferData )
{
    FIXME("(%ld,%ld,%p,%ld,%p): stub\n",dwDesiredAccess,
          dwShareMode, sa, dwFlags, lpScreenBufferData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return INVALID_HANDLE_VALUE;
}


/***********************************************************************
 *           GetConsoleScreenBufferInfo   (KERNEL32.190)
 */
BOOL WINAPI GetConsoleScreenBufferInfo( HANDLE hConsoleOutput,
                                          LPCONSOLE_SCREEN_BUFFER_INFO csbi )
{
    csbi->dwSize.X = 80;
    csbi->dwSize.Y = 24;
    csbi->dwCursorPosition.X = 0;
    csbi->dwCursorPosition.Y = 0;
    csbi->wAttributes = 0;
    csbi->srWindow.Left	= 0;
    csbi->srWindow.Right	= 79;
    csbi->srWindow.Top	= 0;
    csbi->srWindow.Bottom	= 23;
    csbi->dwMaximumWindowSize.X = 80;
    csbi->dwMaximumWindowSize.Y = 24;
    return TRUE;
}


/******************************************************************************
 * SetConsoleActiveScreenBuffer [KERNEL32.623]  Sets buffer to current console
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetConsoleActiveScreenBuffer(
    HANDLE hConsoleOutput) /* [in] Handle to console screen buffer */
{
    FIXME("(%x): stub\n", hConsoleOutput);
    return FALSE;
}


/***********************************************************************
 *            GetLargestConsoleWindowSize   (KERNEL32.226)
 */
COORD WINAPI GetLargestConsoleWindowSize( HANDLE hConsoleOutput )
{
    COORD c;
    c.X = 80;
    c.Y = 24;
    return c;
}

/* gcc doesn't return structures the same way as dwords */
DWORD WINAPI WIN32_GetLargestConsoleWindowSize( HANDLE hConsoleOutput )
{
    COORD c = GetLargestConsoleWindowSize( hConsoleOutput );
    return *(DWORD *)&c;
}

/***********************************************************************
 *            FreeConsole (KERNEL32.267)
 */
BOOL WINAPI FreeConsole(VOID)
{
    return !server_call( REQ_FREE_CONSOLE );
}


/*************************************************************************
 * 		CONSOLE_OpenHandle
 *
 * Open a handle to the current process console.
 */
HANDLE CONSOLE_OpenHandle( BOOL output, DWORD access, LPSECURITY_ATTRIBUTES sa )
{
    int ret = -1;
    struct open_console_request *req = get_req_buffer();

    req->output  = output;
    req->access  = access;
    req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    SetLastError(0);
    if (!server_call( REQ_OPEN_CONSOLE )) ret = req->handle;
    return ret;
}


/*************************************************************************
 * 		CONSOLE_make_complex			[internal]
 *
 * Turns a CONSOLE kernel object into a complex one.
 * (switches from output/input using the terminal where WINE was started to 
 * its own xterm).
 * 
 * This makes simple commandline tools pipeable, while complex commandline 
 * tools work without getting messed up by debugoutput.
 * 
 * All other functions should work indedependend from this call.
 *
 * To test for complex console: pid == 0 -> simple, otherwise complex.
 */
static BOOL CONSOLE_make_complex(HANDLE handle)
{
        struct set_console_fd_request *req = get_req_buffer();
	struct termios term;
	char buf[256];
	char c = '\0';
	int i,xpid,master,slave,pty_handle;

        if (CONSOLE_GetPid( handle )) return TRUE; /* already complex */

	MESSAGE("Console: Making console complex (creating an xterm)...\n");

	if (tcgetattr(0, &term) < 0) {
		/* ignore failure, or we can't run from a script */
	}
	term.c_lflag = ~(ECHO|ICANON);

        if (wine_openpty(&master, &slave, NULL, &term, NULL) < 0)
            return FALSE;

	if ((xpid=fork()) == 0) {
		tcsetattr(slave, TCSADRAIN, &term);
                close( slave );
		sprintf(buf, "-Sxx%d", master);
		/* "-fn vga" for VGA font. Harmless if vga is not present:
		 *  xterm: unable to open font "vga", trying "fixed".... 
		 */
		execlp("xterm", "xterm", buf, "-fn","vga",NULL);
		ERR("error creating AllocConsole xterm\n");
		exit(1);
	}
        pty_handle = FILE_DupUnixHandle( slave, GENERIC_READ | GENERIC_WRITE );
        close( master );
        close( slave );
        if (pty_handle == -1) return FALSE;

	/* most xterms like to print their window ID when used with -S;
	 * read it and continue before the user has a chance...
	 */
        for (i = 0; i < 10000; i++)
        {
            BOOL ok = ReadFile( pty_handle, &c, 1, NULL, NULL );
            if (!ok && !c) usleep(100); /* wait for xterm to be created */
            else if (c == '\n') break;
        }
        if (i == 10000)
        {
            ERR("can't read xterm WID\n");
            CloseHandle( pty_handle );
            return FALSE;
	}
        req->handle = handle;
        req->file_handle = pty_handle;
        req->pid = xpid;
        server_call( REQ_SET_CONSOLE_FD );
        CloseHandle( pty_handle );

	/* enable mouseclicks */
	strcpy( buf, "\033[?1001s\033[?1000h" );
	WriteFile(handle,buf,strlen(buf),NULL,NULL);

        strcpy( buf, "\033]2;" );
        if (GetConsoleTitleA( buf + 4, sizeof(buf) - 5 ))
        {
            strcat( buf, "\a" );
	    WriteFile(handle,buf,strlen(buf),NULL,NULL);
	}
	return TRUE;

}


/***********************************************************************
 *            AllocConsole (KERNEL32.103)
 *
 * creates an xterm with a pty to our program
 */
BOOL WINAPI AllocConsole(VOID)
{
    struct alloc_console_request *req = get_req_buffer();
    HANDLE hStderr;
    int handle_in, handle_out;

    TRACE("()\n");
    req->access  = GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE;
    req->inherit = FALSE;
    if (server_call( REQ_ALLOC_CONSOLE )) return FALSE;
    handle_in = req->handle_in;
    handle_out = req->handle_out;

    if (!DuplicateHandle( GetCurrentProcess(), req->handle_out, GetCurrentProcess(), &hStderr,
                          0, TRUE, DUPLICATE_SAME_ACCESS ))
    {
        CloseHandle( handle_in );
        CloseHandle( handle_out );
        FreeConsole();
        return FALSE;
    }

    /* NT resets the STD_*_HANDLEs on console alloc */
    SetStdHandle( STD_INPUT_HANDLE, handle_in );
    SetStdHandle( STD_OUTPUT_HANDLE, handle_out );
    SetStdHandle( STD_ERROR_HANDLE, hStderr );

    SetLastError(ERROR_SUCCESS);
    SetConsoleTitleA("Wine Console");
    return TRUE;
}


/******************************************************************************
 * GetConsoleCP [KERNEL32.295]  Returns the OEM code page for the console
 *
 * RETURNS
 *    Code page code
 */
UINT WINAPI GetConsoleCP(VOID)
{
    return GetACP();
}


/***********************************************************************
 *            GetConsoleOutputCP   (KERNEL32.189)
 */
UINT WINAPI GetConsoleOutputCP(VOID)
{
    return GetConsoleCP();
}

/***********************************************************************
 *            GetConsoleMode   (KERNEL32.188)
 */
BOOL WINAPI GetConsoleMode(HANDLE hcon,LPDWORD mode)
{
    BOOL ret = FALSE;
    struct get_console_mode_request *req = get_req_buffer();
    req->handle = hcon;
    if (!server_call( REQ_GET_CONSOLE_MODE ))
    {
        if (mode) *mode = req->mode;
        ret = TRUE;
    }
    return ret;
}


/******************************************************************************
 * SetConsoleMode [KERNEL32.628]  Sets input mode of console's input buffer
 *
 * PARAMS
 *    hcon [I] Handle to console input or screen buffer
 *    mode [I] Input or output mode to set
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetConsoleMode( HANDLE hcon, DWORD mode )
{
    struct set_console_mode_request *req = get_req_buffer();
    req->handle = hcon;
    req->mode = mode;
    return !server_call( REQ_SET_CONSOLE_MODE );
}


/******************************************************************************
 * SetConsoleOutputCP [KERNEL32.629]  Set the output codepage used by the console
 *
 * PARAMS
 *    cp [I] code page to set
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetConsoleOutputCP( UINT cp )
{
    FIXME("stub\n");
    return TRUE;
}


/***********************************************************************
 *            GetConsoleTitleA   (KERNEL32.191)
 */
DWORD WINAPI GetConsoleTitleA(LPSTR title,DWORD size)
{
    struct get_console_info_request *req = get_req_buffer();
    DWORD ret = 0;
    HANDLE hcon;

    if ((hcon = CreateFileA( "CONOUT$", GENERIC_READ, 0, NULL,
                               OPEN_EXISTING, 0, 0 )) == INVALID_HANDLE_VALUE)
        return 0;
    req->handle = hcon;
    if (!server_call( REQ_GET_CONSOLE_INFO ))
    {
        lstrcpynA( title, req->title, size );
        ret = strlen(req->title);
    }
    CloseHandle( hcon );
    return ret;
}


/******************************************************************************
 * GetConsoleTitleW [KERNEL32.192]  Retrieves title string for console
 *
 * PARAMS
 *    title [O] Address of buffer for title
 *    size  [I] Size of buffer
 *
 * RETURNS
 *    Success: Length of string copied
 *    Failure: 0
 */
DWORD WINAPI GetConsoleTitleW( LPWSTR title, DWORD size )
{
    char *tmp;
    DWORD ret;

    if (!(tmp = HeapAlloc( GetProcessHeap(), 0, size ))) return 0;
    ret = GetConsoleTitleA( tmp, size );
    lstrcpyAtoW( title, tmp );
    HeapFree( GetProcessHeap(), 0, tmp );
    return ret;
}


/***********************************************************************
 *            WriteConsoleA   (KERNEL32.729)
 */
BOOL WINAPI WriteConsoleA( HANDLE hConsoleOutput,
                               LPCVOID lpBuffer,
                               DWORD nNumberOfCharsToWrite,
                               LPDWORD lpNumberOfCharsWritten,
                               LPVOID lpReserved )
{
	/* FIXME: should I check if this is a console handle? */
	return WriteFile(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite,
			 lpNumberOfCharsWritten, NULL);
}


#define CADD(c) 							\
	if (bufused==curbufsize-1)					\
	    buffer = HeapReAlloc(GetProcessHeap(),0,buffer,(curbufsize+=100));\
	buffer[bufused++]=c;
#define SADD(s) { char *x=s;while (*x) {CADD(*x);x++;}}

/***********************************************************************
 *            WriteConsoleOutputA   (KERNEL32.732)
 */
BOOL WINAPI WriteConsoleOutputA( HANDLE hConsoleOutput,
                                     LPCHAR_INFO lpBuffer,
                                     COORD dwBufferSize,
                                     COORD dwBufferCoord,
                                     LPSMALL_RECT lpWriteRegion)
{
    int i,j,off=0,lastattr=-1;
    int offbase;
    char	sbuf[20],*buffer=NULL;
    int		bufused=0,curbufsize = 100;
    DWORD	res;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    const int colormap[8] = {
    	0,4,2,6,
	1,5,3,7,
    };
    CONSOLE_make_complex(hConsoleOutput);
    buffer = HeapAlloc(GetProcessHeap(),0,curbufsize);
    offbase = (dwBufferCoord.Y - 1) * dwBufferSize.X +
              (dwBufferCoord.X - lpWriteRegion->Left);

    TRACE("orig rect top = %d, bottom=%d, left=%d, right=%d\n",
    	lpWriteRegion->Top,
    	lpWriteRegion->Bottom,
    	lpWriteRegion->Left,
    	lpWriteRegion->Right
    );

    GetConsoleScreenBufferInfo(hConsoleOutput, &csbi);
    sprintf(sbuf,"%c7",27);SADD(sbuf);

    /* Step 1. Make (Bottom,Right) offset of intersection with
       Screen Buffer                                              */
    lpWriteRegion->Bottom = min(lpWriteRegion->Bottom, csbi.dwSize.Y-1) - 
                                lpWriteRegion->Top;
    lpWriteRegion->Right = min(lpWriteRegion->Right, csbi.dwSize.X-1) -
                                lpWriteRegion->Left;

    /* Step 2. If either offset is negative, then no action 
       should be performed. (Implies that requested rectangle is
       outside the current screen buffer rectangle.)              */
    if ((lpWriteRegion->Bottom < 0) ||
        (lpWriteRegion->Right < 0)) {
        /* readjust (Bottom Right) for rectangle */
        lpWriteRegion->Bottom += lpWriteRegion->Top;
        lpWriteRegion->Right += lpWriteRegion->Left;

        TRACE("invisible rect top = %d, bottom=%d, left=%d, right=%d\n",
            lpWriteRegion->Top,
    	    lpWriteRegion->Bottom,
    	    lpWriteRegion->Left,
    	    lpWriteRegion->Right
        );

        HeapFree(GetProcessHeap(),0,buffer);
        return TRUE;
    }

    /* Step 3. Intersect with source rectangle                    */
    lpWriteRegion->Bottom = lpWriteRegion->Top - dwBufferCoord.Y +
             min(lpWriteRegion->Bottom + dwBufferCoord.Y, dwBufferSize.Y-1);
    lpWriteRegion->Right = lpWriteRegion->Left - dwBufferCoord.X +
             min(lpWriteRegion->Right + dwBufferCoord.X, dwBufferSize.X-1);

    TRACE("clipped rect top = %d, bottom=%d, left=%d,right=%d\n",
    	lpWriteRegion->Top,
    	lpWriteRegion->Bottom,
    	lpWriteRegion->Left,
    	lpWriteRegion->Right
    );

    /* Validate above computations made sense, if not then issue
       error and fudge to single character rectangle                  */
    if ((lpWriteRegion->Bottom < lpWriteRegion->Top) ||
        (lpWriteRegion->Right < lpWriteRegion->Left)) {
       ERR("Invalid clipped rectangle top = %d, bottom=%d, left=%d,right=%d\n",
    	      lpWriteRegion->Top,
    	      lpWriteRegion->Bottom,
    	      lpWriteRegion->Left,
    	      lpWriteRegion->Right
          );
       lpWriteRegion->Bottom = lpWriteRegion->Top;
       lpWriteRegion->Right = lpWriteRegion->Left;
    }

    /* Now do the real processing and move the characters    */
    for (i=lpWriteRegion->Top;i<=lpWriteRegion->Bottom;i++) {
        offbase += dwBufferSize.X;
    	sprintf(sbuf,"%c[%d;%dH",27,i+1,lpWriteRegion->Left+1);
	SADD(sbuf);
	for (j=lpWriteRegion->Left;j<=lpWriteRegion->Right;j++) {
            off = j + offbase;
	    if (lastattr!=lpBuffer[off].Attributes) {
		lastattr = lpBuffer[off].Attributes;
		sprintf(sbuf,"%c[0;%s3%d;4%dm",
			27,
			(lastattr & FOREGROUND_INTENSITY)?"1;":"",
			colormap[lastattr&7],
			colormap[(lastattr&0x70)>>4]
		);
		/* FIXME: BACKGROUND_INTENSITY */
		SADD(sbuf);
	    }
	    CADD(lpBuffer[off].Char.AsciiChar);
	}
    }
    sprintf(sbuf,"%c[0m%c8",27,27);SADD(sbuf);
    WriteFile(hConsoleOutput,buffer,bufused,&res,NULL);
    HeapFree(GetProcessHeap(),0,buffer);
    return TRUE;
}

/***********************************************************************
 *            WriteConsoleOutputW   (KERNEL32.734)
 */
BOOL WINAPI WriteConsoleOutputW( HANDLE hConsoleOutput,
                                     LPCHAR_INFO lpBuffer,
                                     COORD dwBufferSize,
                                     COORD dwBufferCoord,
                                     LPSMALL_RECT lpWriteRegion)
{
    FIXME("(%d,%p,%dx%d,%dx%d,%p): stub\n", hConsoleOutput, lpBuffer,
	  dwBufferSize.X,dwBufferSize.Y,dwBufferCoord.X,dwBufferCoord.Y,lpWriteRegion);
    
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;   
}

/***********************************************************************
 *            WriteConsoleW   (KERNEL32.577)
 */
BOOL WINAPI WriteConsoleW( HANDLE hConsoleOutput,
                               LPCVOID lpBuffer,
                               DWORD nNumberOfCharsToWrite,
                               LPDWORD lpNumberOfCharsWritten,
                               LPVOID lpReserved )
{
        BOOL ret;
        LPSTR xstring;
	DWORD n;

	n = WideCharToMultiByte(CP_ACP,0,lpBuffer,nNumberOfCharsToWrite,NULL,0,NULL,NULL);
	xstring=HeapAlloc( GetProcessHeap(), 0, n );

	n = WideCharToMultiByte(CP_ACP,0,lpBuffer,nNumberOfCharsToWrite,xstring,n,NULL,NULL);

	/* FIXME: should I check if this is a console handle? */
	ret= WriteFile(hConsoleOutput, xstring, n,
			 lpNumberOfCharsWritten, NULL);
	/* FIXME: lpNumberOfCharsWritten should be converted to numofchars in UNICODE */ 
	HeapFree( GetProcessHeap(), 0, xstring );
	return ret;
}


/***********************************************************************
 *            ReadConsoleA   (KERNEL32.419)
 */
BOOL WINAPI ReadConsoleA( HANDLE hConsoleInput,
                              LPVOID lpBuffer,
                              DWORD nNumberOfCharsToRead,
                              LPDWORD lpNumberOfCharsRead,
                              LPVOID lpReserved )
{
    int		charsread = 0;
    LPSTR	xbuf = (LPSTR)lpBuffer;
    LPINPUT_RECORD	ir;

    TRACE("(%d,%p,%ld,%p,%p)\n",
	    hConsoleInput,lpBuffer,nNumberOfCharsToRead,
	    lpNumberOfCharsRead,lpReserved
    );

    CONSOLE_get_input(hConsoleInput,FALSE);

    /* FIXME: should we read at least 1 char? The SDK does not say */
    while (charsread<nNumberOfCharsToRead)
    {
        struct read_console_input_request *req = get_req_buffer();
        req->handle = hConsoleInput;
        req->count = 1;
        req->flush = 1;
        if (server_call( REQ_READ_CONSOLE_INPUT )) return FALSE;
        if (!req->read) break;
	ir = (LPINPUT_RECORD)(req+1);
    	if (!ir->Event.KeyEvent.bKeyDown)
		continue;
    	if (ir->EventType != KEY_EVENT)
		continue;
	*xbuf++ = ir->Event.KeyEvent.uChar.AsciiChar; 
	charsread++;
    }
    if (lpNumberOfCharsRead)
    	*lpNumberOfCharsRead = charsread;
    return TRUE;
}

/***********************************************************************
 *            ReadConsoleW   (KERNEL32.427)
 */
BOOL WINAPI ReadConsoleW( HANDLE hConsoleInput,
                              LPVOID lpBuffer,
                              DWORD nNumberOfCharsToRead,
                              LPDWORD lpNumberOfCharsRead,
                              LPVOID lpReserved )
{
    BOOL ret;
    LPSTR buf = (LPSTR)HeapAlloc(GetProcessHeap(), 0, nNumberOfCharsToRead);

    ret = ReadConsoleA(
    	hConsoleInput,
	buf,
	nNumberOfCharsToRead,
	lpNumberOfCharsRead,
	lpReserved
    );
    if (ret)
    	lstrcpynAtoW(lpBuffer,buf,nNumberOfCharsToRead);
    HeapFree( GetProcessHeap(), 0, buf );
    return ret;
}


/******************************************************************************
 * ReadConsoleInputA [KERNEL32.569]  Reads data from a console
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
    struct read_console_input_request *req = get_req_buffer();

    /* loop until we get at least one event */
    for (;;)
    {
        req->handle = hConsoleInput;
        req->count = nLength;
        req->flush = 1;
        if (server_call( REQ_READ_CONSOLE_INPUT )) return FALSE;
        if (req->read)
        {
            memcpy( lpBuffer, req + 1, req->read * sizeof(*lpBuffer) );
            if (lpNumberOfEventsRead) *lpNumberOfEventsRead = req->read;
            return TRUE;
        }
	CONSOLE_get_input(hConsoleInput,TRUE);
        /*WaitForSingleObject( hConsoleInput, INFINITE32 );*/
    }
}


/***********************************************************************
 *            ReadConsoleInputW   (KERNEL32.570)
 */
BOOL WINAPI ReadConsoleInputW( HANDLE handle, LPINPUT_RECORD buffer,
                                   DWORD count, LPDWORD read )
{
    /* FIXME: Fix this if we get UNICODE input. */
    return ReadConsoleInputA( handle, buffer, count, read );
}


/***********************************************************************
 *            FlushConsoleInputBuffer   (KERNEL32.132)
 */
BOOL WINAPI FlushConsoleInputBuffer( HANDLE handle )
{
    struct read_console_input_request *req = get_req_buffer();
    req->handle = handle;
    req->count = -1;  /* get all records */
    req->flush = 1;
    return !server_call( REQ_READ_CONSOLE_INPUT );
}


/***********************************************************************
 *            PeekConsoleInputA   (KERNEL32.550)
 *
 * Gets 'count' first events (or less) from input queue.
 *
 * Does not need a complex console.
 */
BOOL WINAPI PeekConsoleInputA( HANDLE handle, LPINPUT_RECORD buffer,
                                   DWORD count, LPDWORD read )
{
    struct read_console_input_request *req = get_req_buffer();

    CONSOLE_get_input(handle,FALSE);

    req->handle = handle;
    req->count = count;
    req->flush = 0;
    if (server_call( REQ_READ_CONSOLE_INPUT )) return FALSE;
    if (req->read) memcpy( buffer, req + 1, req->read * sizeof(*buffer) );
    if (read) *read = req->read;
    return TRUE;
}


/***********************************************************************
 *            PeekConsoleInputW   (KERNEL32.551)
 */
BOOL WINAPI PeekConsoleInputW(HANDLE hConsoleInput,
                                  LPINPUT_RECORD pirBuffer,
                                  DWORD cInRecords,
                                  LPDWORD lpcRead)
{
    /* FIXME: Hmm. Fix this if we get UNICODE input. */
    return PeekConsoleInputA(hConsoleInput,pirBuffer,cInRecords,lpcRead);
}


/******************************************************************************
 * WriteConsoleInputA [KERNEL32.730]  Write data to a console input buffer
 *
 */
BOOL WINAPI WriteConsoleInputA( HANDLE handle, INPUT_RECORD *buffer,
                                DWORD count, LPDWORD written )
{
    struct write_console_input_request *req = get_req_buffer();
    const DWORD max = server_remaining( req + 1 ) / sizeof(INPUT_RECORD);

    if (written) *written = 0;
    while (count)
    {
        DWORD len = count < max ? count : max;
        req->count = len;
        req->handle = handle;
        memcpy( req + 1, buffer, len * sizeof(*buffer) );
        if (server_call( REQ_WRITE_CONSOLE_INPUT )) return FALSE;
        if (written) *written += req->written;
        count -= len;
        buffer += len;
    }
    return TRUE;
}

/******************************************************************************
 * WriteConsoleInputW [KERNEL32.731]  Write data to a console input buffer
 *
 */
BOOL WINAPI WriteConsoleInputW( HANDLE handle, INPUT_RECORD *buffer,
                                DWORD count, LPDWORD written )
{
    FIXME("(%d,%p,%ld,%p): stub!\n", handle, buffer, count, written);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;    
}


/***********************************************************************
 *            SetConsoleTitleA   (KERNEL32.476)
 *
 * Sets the console title.
 *
 * We do not necessarily need to create a complex console for that,
 * but should remember the title and set it on creation of the latter.
 * (not fixed at this time).
 */
BOOL WINAPI SetConsoleTitleA(LPCSTR title)
{
    struct set_console_info_request *req = get_req_buffer();
    HANDLE hcon;
    DWORD written;

    if ((hcon = CreateFileA( "CONOUT$", GENERIC_READ|GENERIC_WRITE, 0, NULL,
                               OPEN_EXISTING, 0, 0 )) == INVALID_HANDLE_VALUE)
        return FALSE;
    req->handle = hcon;
    req->mask = SET_CONSOLE_INFO_TITLE;
    lstrcpynA( req->title, title, server_remaining(req->title) );
    if (server_call( REQ_SET_CONSOLE_INFO )) goto error;
    if (CONSOLE_GetPid( hcon ))
    {
        /* only set title for complex console (own xterm) */
        WriteFile( hcon, "\033]2;", 4, &written, NULL );
        WriteFile( hcon, title, strlen(title), &written, NULL );
        WriteFile( hcon, "\a", 1, &written, NULL );
    }
    CloseHandle( hcon );
    return TRUE;
 error:
    CloseHandle( hcon );
    return FALSE;
}


/******************************************************************************
 * SetConsoleTitleW [KERNEL32.477]  Sets title bar string for console
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
BOOL WINAPI SetConsoleTitleW( LPCWSTR title )
{
    BOOL ret;

    LPSTR titleA = HEAP_strdupWtoA( GetProcessHeap(), 0, title );
    ret = SetConsoleTitleA(titleA);
    HeapFree( GetProcessHeap(), 0, titleA );
    return ret;
}

/******************************************************************************
 * SetConsoleCursorPosition [KERNEL32.627]
 * Sets the cursor position in console
 *
 * PARAMS
 *    hConsoleOutput   [I] Handle of console screen buffer
 *    dwCursorPosition [I] New cursor position coordinates
 *
 * RETURNS STD
 */
BOOL WINAPI SetConsoleCursorPosition( HANDLE hcon, COORD pos )
{
    char 	xbuf[20];
    DWORD	xlen;

    /* make console complex only if we change lines, not just in the line */
    if (pos.Y)
    	CONSOLE_make_complex(hcon);

    TRACE("%d (%dx%d)\n", hcon, pos.X , pos.Y );
    /* x are columns, y rows */
    if (pos.Y) 
    	/* full screen cursor absolute positioning */
	sprintf(xbuf,"%c[%d;%dH", 0x1B, pos.Y+1, pos.X+1);
    else
    	/* relative cursor positioning in line (\r to go to 0) */
	sprintf(xbuf,"\r%c[%dC", 0x1B, pos.X);
    /* FIXME: store internal if we start using own console buffers */
    WriteFile(hcon,xbuf,strlen(xbuf),&xlen,NULL);
    return TRUE;
}

/***********************************************************************
 *            GetNumberOfConsoleInputEvents   (KERNEL32.246)
 */
BOOL WINAPI GetNumberOfConsoleInputEvents(HANDLE hcon,LPDWORD nrofevents)
{
    struct read_console_input_request *req = get_req_buffer();

    CONSOLE_get_input(hcon,FALSE);

    req->handle = hcon;
    req->count = -1;
    req->flush = 0;
    if (server_call( REQ_READ_CONSOLE_INPUT )) return FALSE;
    if (nrofevents) *nrofevents = req->read;
    return TRUE;
}

/***********************************************************************
 *            GetNumberOfConsoleMouseButtons   (KERNEL32.358)
 */
BOOL WINAPI GetNumberOfConsoleMouseButtons(LPDWORD nrofbuttons)
{
    FIXME("(%p): stub\n", nrofbuttons);
    *nrofbuttons = 2;
    return TRUE;
}

/******************************************************************************
 * GetConsoleCursorInfo [KERNEL32.296]  Gets size and visibility of console
 *
 * PARAMS
 *    hcon  [I] Handle to console screen buffer
 *    cinfo [O] Address of cursor information
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetConsoleCursorInfo( HANDLE hcon, LPCONSOLE_CURSOR_INFO cinfo )
{
    struct get_console_info_request *req = get_req_buffer();
    req->handle = hcon;
    if (server_call( REQ_GET_CONSOLE_INFO )) return FALSE;
    if (cinfo)
    {
        cinfo->dwSize = req->cursor_size;
        cinfo->bVisible = req->cursor_visible;
    }
    return TRUE;
}


/******************************************************************************
 * SetConsoleCursorInfo [KERNEL32.626]  Sets size and visibility of cursor
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetConsoleCursorInfo( 
    HANDLE hcon,                /* [in] Handle to console screen buffer */
    LPCONSOLE_CURSOR_INFO cinfo)  /* [in] Address of cursor information */
{
    struct set_console_info_request *req = get_req_buffer();
    char	buf[8];
    DWORD	xlen;

    CONSOLE_make_complex(hcon);
    sprintf(buf,"\033[?25%c",cinfo->bVisible?'h':'l');
    WriteFile(hcon,buf,strlen(buf),&xlen,NULL);

    req->handle         = hcon;
    req->cursor_size    = cinfo->dwSize;
    req->cursor_visible = cinfo->bVisible;
    req->mask           = SET_CONSOLE_INFO_CURSOR;
    return !server_call( REQ_SET_CONSOLE_INFO );
}


/******************************************************************************
 * SetConsoleWindowInfo [KERNEL32.634]  Sets size and position of console
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetConsoleWindowInfo(
    HANDLE hcon,       /* [in] Handle to console screen buffer */
    BOOL bAbsolute,    /* [in] Coordinate type flag */
    LPSMALL_RECT window) /* [in] Address of new window rectangle */
{
    FIXME("(%x,%d,%p): stub\n", hcon, bAbsolute, window);
    return TRUE;
}


/******************************************************************************
 * SetConsoleTextAttribute [KERNEL32.631]  Sets colors for text
 *
 * Sets the foreground and background color attributes of characters
 * written to the screen buffer.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetConsoleTextAttribute(HANDLE hConsoleOutput,WORD wAttr)
{
    const int colormap[8] = {
    	0,4,2,6,
	1,5,3,7,
    };
    DWORD xlen;
    char buffer[20];

    TRACE("(%d,%d)\n",hConsoleOutput,wAttr);
    sprintf(buffer,"%c[0;%s3%d;4%dm",
	27,
	(wAttr & FOREGROUND_INTENSITY)?"1;":"",
	colormap[wAttr&7],
	colormap[(wAttr&0x70)>>4]
    );
    WriteFile(hConsoleOutput,buffer,strlen(buffer),&xlen,NULL);
    return TRUE;
}


/******************************************************************************
 * SetConsoleScreenBufferSize [KERNEL32.630]  Changes size of console 
 *
 * PARAMS
 *    hConsoleOutput [I] Handle to console screen buffer
 *    dwSize         [I] New size in character rows and cols
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetConsoleScreenBufferSize( HANDLE hConsoleOutput, 
                                          COORD dwSize )
{
    FIXME("(%d,%dx%d): stub\n",hConsoleOutput,dwSize.X,dwSize.Y);
    return TRUE;
}


/******************************************************************************
 * FillConsoleOutputCharacterA [KERNEL32.242]
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
BOOL WINAPI FillConsoleOutputCharacterA(
    HANDLE hConsoleOutput,
    BYTE cCharacter,
    DWORD nLength,
    COORD dwCoord,
    LPDWORD lpNumCharsWritten)
{
    long 	count;
    DWORD	xlen;

    SetConsoleCursorPosition(hConsoleOutput,dwCoord);
    for(count=0;count<nLength;count++)
	WriteFile(hConsoleOutput,&cCharacter,1,&xlen,NULL);
    *lpNumCharsWritten = nLength;
    return TRUE;
}


/******************************************************************************
 * FillConsoleOutputCharacterW [KERNEL32.243]  Writes characters to console
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
BOOL WINAPI FillConsoleOutputCharacterW(HANDLE hConsoleOutput,
                                            WCHAR cCharacter,
                                            DWORD nLength,
                                           COORD dwCoord, 
                                            LPDWORD lpNumCharsWritten)
{
    long	count;
    DWORD	xlen;

    SetConsoleCursorPosition(hConsoleOutput,dwCoord);
    /* FIXME: not quite correct ... but the lower part of UNICODE char comes
     * first 
     */
    for(count=0;count<nLength;count++)
	WriteFile(hConsoleOutput,&cCharacter,1,&xlen,NULL);
    *lpNumCharsWritten = nLength;
    return TRUE;
}


/******************************************************************************
 * FillConsoleOutputAttribute [KERNEL32.241]  Sets attributes for console
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
BOOL WINAPI FillConsoleOutputAttribute( HANDLE hConsoleOutput, 
              WORD wAttribute, DWORD nLength, COORD dwCoord, 
              LPDWORD lpNumAttrsWritten)
{
    FIXME("(%d,%d,%ld,%dx%d,%p): stub\n", hConsoleOutput,
          wAttribute,nLength,dwCoord.X,dwCoord.Y,lpNumAttrsWritten);
    *lpNumAttrsWritten = nLength;
    return TRUE;
}

/******************************************************************************
 * ReadConsoleOutputCharacterA [KERNEL32.573]
 * 
 * BUGS
 *   Unimplemented
 */
BOOL WINAPI ReadConsoleOutputCharacterA(HANDLE hConsoleOutput, 
	      LPSTR lpstr, DWORD dword, COORD coord, LPDWORD lpdword)
{
    FIXME("(%d,%p,%ld,%dx%d,%p): stub\n", hConsoleOutput,lpstr,
	  dword,coord.X,coord.Y,lpdword);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 * ReadConsoleOutputCharacterW [KERNEL32.574]
 * 
 * BUGS
 *   Unimplemented
 */
BOOL WINAPI ReadConsoleOutputCharacterW(HANDLE hConsoleOutput, 
	      LPWSTR lpstr, DWORD dword, COORD coord, LPDWORD lpdword)
{
    FIXME("(%d,%p,%ld,%dx%d,%p): stub\n", hConsoleOutput,lpstr,
	  dword,coord.X,coord.Y,lpdword);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/******************************************************************************
 * ScrollConsoleScreenBufferA [KERNEL32.612]
 * 
 * BUGS
 *   Unimplemented
 */
BOOL WINAPI ScrollConsoleScreenBufferA( HANDLE hConsoleOutput, 
	      LPSMALL_RECT lpScrollRect, LPSMALL_RECT lpClipRect,
              COORD dwDestOrigin, LPCHAR_INFO lpFill)
{
    FIXME("(%d,%p,%p,%dx%d,%p): stub\n", hConsoleOutput,lpScrollRect,
	  lpClipRect,dwDestOrigin.X,dwDestOrigin.Y,lpFill);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 * ScrollConsoleScreenBufferW [KERNEL32.613]
 * 
 * BUGS
 *   Unimplemented
 */
BOOL WINAPI ScrollConsoleScreenBufferW( HANDLE hConsoleOutput, 
	      LPSMALL_RECT lpScrollRect, LPSMALL_RECT lpClipRect,
              COORD dwDestOrigin, LPCHAR_INFO lpFill)
{
    FIXME("(%d,%p,%p,%dx%d,%p): stub\n", hConsoleOutput,lpScrollRect,
	  lpClipRect,dwDestOrigin.X,dwDestOrigin.Y,lpFill);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 *  ReadConsoleOutputA [KERNEL32.571]
 * 
 * BUGS
 *   Unimplemented
 */
BOOL WINAPI ReadConsoleOutputA( HANDLE hConsoleOutput, 
				LPCHAR_INFO lpBuffer,
				COORD dwBufferSize,
				COORD dwBufferCoord,
				LPSMALL_RECT lpReadRegion )
{
    FIXME("(%d,%p,%dx%d,%dx%d,%p): stub\n", hConsoleOutput, lpBuffer,
	  dwBufferSize.X, dwBufferSize.Y, dwBufferSize.X, dwBufferSize.Y, 
	  lpReadRegion);
    
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 *  ReadConsoleOutputW [KERNEL32.575]
 * 
 * BUGS
 *   Unimplemented
 */
BOOL WINAPI ReadConsoleOutputW( HANDLE hConsoleOutput, 
				LPCHAR_INFO lpBuffer,
				COORD dwBufferSize,
				COORD dwBufferCoord,
				LPSMALL_RECT lpReadRegion )
{
    FIXME("(%d,%p,%dx%d,%dx%d,%p): stub\n", hConsoleOutput, lpBuffer,
	  dwBufferSize.X, dwBufferSize.Y, dwBufferSize.X, dwBufferSize.Y, 
	  lpReadRegion);
    
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 *  ReadConsoleOutputAttribute [KERNEL32.572]
 * 
 * BUGS
 *   Unimplemented
 */
BOOL WINAPI ReadConsoleOutputAttribute( HANDLE hConsoleOutput, 
					LPWORD lpAttribute,
					DWORD nLength,
					COORD dwReadCoord,
					LPDWORD lpNumberOfAttrsRead)
{
    FIXME("(%d,%p,%ld,%dx%d,%p): stub\n", hConsoleOutput, lpAttribute,
	  nLength, dwReadCoord.X, dwReadCoord.Y, lpNumberOfAttrsRead);
    
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 *  SetConsoleCP	 [KERNEL32.572]
 * 
 * BUGS
 *   Unimplemented
 */
BOOL WINAPI SetConsoleCP( UINT cp )
{
    FIXME("(%d): stub\n", cp);
    
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
