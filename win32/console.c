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

#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/errno.h>
#include <signal.h>
#include <assert.h>

#include "windows.h"
#include "k32obj.h"
#include "thread.h"
#include "async.h"
#include "file.h"
#include "process.h"
#include "winerror.h"
#include "wincon.h"
#include "heap.h"
#include "debug.h"

#include "server/request.h"
#include "server.h"

/* The CONSOLE kernel32 Object */
typedef struct _CONSOLE {
	K32OBJ  		header;
} CONSOLE;

static void CONSOLE_Destroy( K32OBJ *obj );

const K32OBJ_OPS CONSOLE_Ops =
{
	CONSOLE_Destroy		/* destroy */
};

/* FIXME:  Should be in an internal header file.  OK, so which one?
   Used by CONSOLE_makecomplex. */
FILE *wine_openpty(int *master, int *slave, char *name,
                   struct termios *term, struct winsize *winsize);

/***********************************************************************
 * CONSOLE_Destroy
 */
static void CONSOLE_Destroy(K32OBJ *obj)
{
	CONSOLE *console = (CONSOLE *)obj;
	assert(obj->type == K32OBJ_CONSOLE);

	obj->type = K32OBJ_UNKNOWN;

	HeapFree(SystemHeap, 0, console);
}


/***********************************************************************
 * CONSOLE_GetPtr
 */
static CONSOLE *CONSOLE_GetPtr( HANDLE32 handle )
{
    return (CONSOLE*)HANDLE_GetObjPtr( PROCESS_Current(), handle,
                                       K32OBJ_CONSOLE, 0, NULL );
}

/****************************************************************************
 *		CONSOLE_GetInfo
 */
static BOOL32 CONSOLE_GetInfo( HANDLE32 handle, struct get_console_info_reply *reply )
{
    struct get_console_info_request req;

    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), handle,
                                              K32OBJ_CONSOLE, GENERIC_READ )) == -1)
        return FALSE;
    CLIENT_SendRequest( REQ_GET_CONSOLE_INFO, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitSimpleReply( reply, sizeof(*reply), NULL );
}

/****************************************************************************
 *		XTERM_string_to_IR			[internal]
 *
 * Transfers a string read from XTERM to INPUT_RECORDs and adds them to the
 * queue. Does translation of vt100 style function keys and xterm-mouse clicks.
 */
static void
CONSOLE_string_to_IR( HANDLE32 hConsoleInput,unsigned char *buf,int len) {
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
	    ir.Event.KeyEvent.uChar.AsciiChar = inchar;

	    if (inchar==127) { /* backspace */
	    	ir.Event.KeyEvent.uChar.AsciiChar = '\b'; /* FIXME: hmm */
		ir.Event.KeyEvent.wVirtualScanCode = 0x0e;
		ir.Event.KeyEvent.wVirtualKeyCode = VK_BACK;
	    } else {
		if (inchar=='\n') {
		    ir.Event.KeyEvent.uChar.AsciiChar	= '\r';
		    ir.Event.KeyEvent.wVirtualKeyCode	= VK_RETURN;
		    ir.Event.KeyEvent.wVirtualScanCode	= 0x1c;
		} else {
		    if (inchar<' ') {
			/* FIXME: find good values for ^X */
			ir.Event.KeyEvent.wVirtualKeyCode = 0xdead;
			ir.Event.KeyEvent.wVirtualScanCode = 0xbeef;
		    } 
		}
	    }

	    assert(WriteConsoleInput32A( hConsoleInput, &ir, 1, &junk ));
	    ir.Event.KeyEvent.bKeyDown = 0;
	    assert(WriteConsoleInput32A( hConsoleInput, &ir, 1, &junk ));
	    continue;
	}
	/* inchar is ESC */
	if ((j==len-1) || (buf[j+1]!='[')) {/* add ESCape on its own */
	    ir.EventType = 1; /* Key_event */
	    ir.Event.KeyEvent.bKeyDown		= 1;
	    ir.Event.KeyEvent.wRepeatCount	= 0;

	    ir.Event.KeyEvent.wVirtualKeyCode	= VkKeyScan16(27);
	    ir.Event.KeyEvent.wVirtualScanCode	= MapVirtualKey16(
	    	ir.Event.KeyEvent.wVirtualKeyCode,0
	    );
	    ir.Event.KeyEvent.dwControlKeyState = 0;
	    ir.Event.KeyEvent.uChar.AsciiChar	= 27;
	    assert(WriteConsoleInput32A( hConsoleInput, &ir, 1, &junk ));
	    ir.Event.KeyEvent.bKeyDown = 0;
	    assert(WriteConsoleInput32A( hConsoleInput, &ir, 1, &junk ));
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
			FIXME(console,"parse ESC[%d~\n",subid);
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
		    ir.Event.MouseEvent.dwMousePosition.x = buf[k+2]-'!';
		    ir.Event.MouseEvent.dwMousePosition.y = buf[k+3]-'!';
		    if (buf[k+1]=='#')
			ir.Event.MouseEvent.dwButtonState = 0;
		    else
			ir.Event.MouseEvent.dwButtonState = 1<<(buf[k+1]-' ');
		    ir.Event.MouseEvent.dwEventFlags	  = 0; /* FIXME */
		    assert(WriteConsoleInput32A( hConsoleInput, &ir, 1, &junk));
		    j=k+3;
		}
	    	break;
	    	
	    }
	    if (scancode) {
		ir.Event.KeyEvent.wVirtualScanCode = scancode;
		ir.Event.KeyEvent.wVirtualKeyCode = MapVirtualKey16(scancode,1);
		assert(WriteConsoleInput32A( hConsoleInput, &ir, 1, &junk ));
		ir.Event.KeyEvent.bKeyDown		= 0;
		assert(WriteConsoleInput32A( hConsoleInput, &ir, 1, &junk ));
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
CONSOLE_get_input( HANDLE32 handle, BOOL32 blockwait )
{
    char	*buf = HeapAlloc(GetProcessHeap(),0,1);
    int		len = 0;

    while (1)
    {
	DWORD res;
	char inchar;
        if (WaitForSingleObject( handle, 0 )) break;
        if (!ReadFile( handle, &inchar, 1, &res, NULL )) break;
	if (!res) /* res 0 but readable means EOF? Hmm. */
		break;
	buf = HeapReAlloc(GetProcessHeap(),0,buf,len+1);
	buf[len++]=inchar;
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
BOOL32 WINAPI SetConsoleCtrlHandler( HANDLER_ROUTINE *func, BOOL32 add )
{
  unsigned int alloc_loop = sizeof(handlers)/sizeof(HANDLER_ROUTINE *);
  unsigned int done = 0;
  FIXME(console, "(%p,%i) - no error checking or testing yet\n", func, add);
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
	   FIXME(console, "Out of space on CtrlHandler table\n");
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
	   WARN(console, "Attempt to remove non-installed CtrlHandler %p\n",
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
BOOL32 WINAPI GenerateConsoleCtrlEvent( DWORD dwCtrlEvent,
					DWORD dwProcessGroupID )
{
  if (dwCtrlEvent != CTRL_C_EVENT && dwCtrlEvent != CTRL_BREAK_EVENT)
    {
      ERR( console, "invalid event %d for PGID %ld\n", 
	   (unsigned short)dwCtrlEvent, dwProcessGroupID );
      return FALSE;
    }
  if (dwProcessGroupID == GetCurrentProcessId() )
    {
      FIXME( console, "Attempt to send event %d to self - stub\n",
	     (unsigned short)dwCtrlEvent );
      return FALSE;
    }
  FIXME( console,"event %d to external PGID %ld - not implemented yet\n",
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
HANDLE32 WINAPI CreateConsoleScreenBuffer( DWORD dwDesiredAccess,
                DWORD dwShareMode, LPSECURITY_ATTRIBUTES sa,
                DWORD dwFlags, LPVOID lpScreenBufferData )
{
    FIXME(console, "(%ld,%ld,%p,%ld,%p): stub\n",dwDesiredAccess,
          dwShareMode, sa, dwFlags, lpScreenBufferData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return INVALID_HANDLE_VALUE32;
}


/***********************************************************************
 *           GetConsoleScreenBufferInfo   (KERNEL32.190)
 */
BOOL32 WINAPI GetConsoleScreenBufferInfo( HANDLE32 hConsoleOutput,
                                          LPCONSOLE_SCREEN_BUFFER_INFO csbi )
{
    csbi->dwSize.x = 80;
    csbi->dwSize.y = 24;
    csbi->dwCursorPosition.x = 0;
    csbi->dwCursorPosition.y = 0;
    csbi->wAttributes = 0;
    csbi->srWindow.Left	= 0;
    csbi->srWindow.Right	= 79;
    csbi->srWindow.Top	= 0;
    csbi->srWindow.Bottom	= 23;
    csbi->dwMaximumWindowSize.x = 80;
    csbi->dwMaximumWindowSize.y = 24;
    return TRUE;
}


/******************************************************************************
 * SetConsoleActiveScreenBuffer [KERNEL32.623]  Sets buffer to current console
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL32 WINAPI SetConsoleActiveScreenBuffer(
    HANDLE32 hConsoleOutput) /* [in] Handle to console screen buffer */
{
    FIXME(console, "(%x): stub\n", hConsoleOutput);
    return FALSE;
}


/***********************************************************************
 *            GetLargestConsoleWindowSize   (KERNEL32.226)
 */
DWORD WINAPI GetLargestConsoleWindowSize( HANDLE32 hConsoleOutput )
{
    return (DWORD)MAKELONG(80,24);
}

/***********************************************************************
 *            FreeConsole (KERNEL32.267)
 */
BOOL32 WINAPI FreeConsole(VOID)
{

	PDB32 *pdb = PROCESS_Current();
	CONSOLE *console;

	SYSTEM_LOCK();

	console = (CONSOLE *)pdb->console;

	if (console == NULL) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

        CLIENT_SendRequest( REQ_FREE_CONSOLE, -1, 0 );
        if (CLIENT_WaitReply( NULL, NULL, 0 ) != ERROR_SUCCESS)
        {
            K32OBJ_DecCount(&console->header);
            SYSTEM_UNLOCK();
            return FALSE;
        }

        HANDLE_CloseAll( pdb, &console->header );
	K32OBJ_DecCount( &console->header );
	pdb->console = NULL;
	SYSTEM_UNLOCK();
	return TRUE;
}


/*************************************************************************
 * 		CONSOLE_OpenHandle
 *
 * Open a handle to the current process console.
 */
HANDLE32 CONSOLE_OpenHandle( BOOL32 output, DWORD access, LPSECURITY_ATTRIBUTES sa )
{
    struct open_console_request req;
    struct open_console_reply reply;
    CONSOLE *console;
    HANDLE32 handle;

    req.output  = output;
    req.access  = access;
    req.inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    CLIENT_SendRequest( REQ_OPEN_CONSOLE, -1, 1, &req, sizeof(req) );
    CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL );
    if (reply.handle == -1) return INVALID_HANDLE_VALUE32;

    SYSTEM_LOCK();
    if (!(console = (CONSOLE*)HeapAlloc( SystemHeap, 0, sizeof(*console))))
    {
        SYSTEM_UNLOCK();
        return FALSE;
    }
    console->header.type     = K32OBJ_CONSOLE;
    console->header.refcount = 1;
    handle = HANDLE_Alloc( PROCESS_Current(), &console->header, req.access,
                           req.inherit, reply.handle );
    SYSTEM_UNLOCK();
    K32OBJ_DecCount(&console->header);
    return handle;
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
static BOOL32 CONSOLE_make_complex(HANDLE32 handle)
{
        struct set_console_fd_request req;
        struct get_console_info_reply info;
	struct termios term;
	char buf[256];
	char c = '\0';
	int status = 0;
	int i,xpid,master,slave;
	DWORD	xlen;

        if (!CONSOLE_GetInfo( handle, &info )) return FALSE;
        if (info.pid) return TRUE; /* already complex */

	MSG("Console: Making console complex (creating an xterm)...\n");

	if (tcgetattr(0, &term) < 0) {
		/* ignore failure, or we can't run from a script */
	}
	term.c_lflag = ~(ECHO|ICANON);

        if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), handle,
                                                  K32OBJ_CONSOLE, 0 )) == -1)
            return FALSE;

        if (wine_openpty(&master, &slave, NULL, &term, NULL) < 0)
            return FALSE;

	if ((xpid=fork()) == 0) {
		tcsetattr(slave, TCSADRAIN, &term);
		sprintf(buf, "-Sxx%d", master);
		/* "-fn vga" for VGA font. Harmless if vga is not present:
		 *  xterm: unable to open font "vga", trying "fixed".... 
		 */
		execlp("xterm", "xterm", buf, "-fn","vga",NULL);
		ERR(console, "error creating AllocConsole xterm\n");
		exit(1);
	}

        req.pid = xpid;
        CLIENT_SendRequest( REQ_SET_CONSOLE_FD, dup(slave), 1, &req, sizeof(req) );
        CLIENT_WaitReply( NULL, NULL, 0 );

	/* most xterms like to print their window ID when used with -S;
	 * read it and continue before the user has a chance...
	 */
	for (i=0; c!='\n'; (status=read(slave, &c, 1)), i++) {
		if (status == -1 && c == '\0') {
				/* wait for xterm to be created */
			usleep(100);
		}
		if (i > 10000) {
			ERR(console, "can't read xterm WID\n");
			kill(xpid, SIGKILL);
			return FALSE;
		}
	}
	/* enable mouseclicks */
	sprintf(buf,"%c[?1001s%c[?1000h",27,27);
	WriteFile(handle,buf,strlen(buf),&xlen,NULL);
        
        if (GetConsoleTitle32A( buf, sizeof(buf) ))
        {
	    WriteFile(handle,"\033]2;",4,&xlen,NULL);
	    WriteFile(handle,buf,strlen(buf),&xlen,NULL);
	    WriteFile(handle,"\a",1,&xlen,NULL);
	}
	return TRUE;

}


/***********************************************************************
 *            AllocConsole (KERNEL32.103)
 *
 * creates an xterm with a pty to our program
 */
BOOL32 WINAPI AllocConsole(VOID)
{
        struct open_console_request req;
        struct open_console_reply reply;
	PDB32 *pdb = PROCESS_Current();
	CONSOLE *console;
	HANDLE32 hIn, hOut, hErr;

	SYSTEM_LOCK();		/* FIXME: really only need to lock the process */

	console = (CONSOLE *)pdb->console;

	/* don't create a console if we already have one */
	if (console != NULL) {
		SetLastError(ERROR_ACCESS_DENIED);
		SYSTEM_UNLOCK();
		return FALSE;
	}

        if (!(console = (CONSOLE*)HeapAlloc( SystemHeap, 0, sizeof(*console))))
        {
            SYSTEM_UNLOCK();
            return FALSE;
        }

        console->header.type     = K32OBJ_CONSOLE;
        console->header.refcount = 1;

        CLIENT_SendRequest( REQ_ALLOC_CONSOLE, -1, 0 );
        if (CLIENT_WaitReply( NULL, NULL, 0 ) != ERROR_SUCCESS)
        {
            K32OBJ_DecCount(&console->header);
            SYSTEM_UNLOCK();
            return FALSE;
        }

        req.output = 0;
        req.access = GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE;
        req.inherit = FALSE;
        CLIENT_SendRequest( REQ_OPEN_CONSOLE, -1, 1, &req, sizeof(req) );
        if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL ) != ERROR_SUCCESS)
        {
            K32OBJ_DecCount(&console->header);
            SYSTEM_UNLOCK();
            return FALSE;
        }
	if ((hIn = HANDLE_Alloc(pdb,&console->header, req.access,
                                FALSE, reply.handle)) == INVALID_HANDLE_VALUE32)
        {
            K32OBJ_DecCount(&console->header);
            SYSTEM_UNLOCK();
            return FALSE;
	}

        req.output = 1;
        CLIENT_SendRequest( REQ_OPEN_CONSOLE, -1, 1, &req, sizeof(req) );
        if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL ) != ERROR_SUCCESS)
        {
            CloseHandle(hIn);
            K32OBJ_DecCount(&console->header);
            SYSTEM_UNLOCK();
            return FALSE;
        }
	if ((hOut = HANDLE_Alloc(pdb,&console->header, req.access,
                                 FALSE, reply.handle)) == INVALID_HANDLE_VALUE32)
        {
            CloseHandle(hIn);
            K32OBJ_DecCount(&console->header);
            SYSTEM_UNLOCK();
            return FALSE;
	}

        if (!DuplicateHandle( GetCurrentProcess(), hOut,
                              GetCurrentProcess(), &hErr,
                              0, TRUE, DUPLICATE_SAME_ACCESS ))
        {
            CloseHandle(hIn);
            CloseHandle(hOut);
            K32OBJ_DecCount(&console->header);
            SYSTEM_UNLOCK();
            return FALSE;
	}

        if (pdb->console) K32OBJ_DecCount( pdb->console );
	pdb->console = (K32OBJ *)console;
        K32OBJ_IncCount( pdb->console );

	/* NT resets the STD_*_HANDLEs on console alloc */
	SetStdHandle(STD_INPUT_HANDLE, hIn);
	SetStdHandle(STD_OUTPUT_HANDLE, hOut);
	SetStdHandle(STD_ERROR_HANDLE, hErr);

	SetLastError(ERROR_SUCCESS);
	SYSTEM_UNLOCK();
	SetConsoleTitle32A("Wine Console");
	return TRUE;
}


/******************************************************************************
 * GetConsoleCP [KERNEL32.295]  Returns the OEM code page for the console
 *
 * RETURNS
 *    Code page code
 */
UINT32 WINAPI GetConsoleCP(VOID)
{
    return GetACP();
}


/***********************************************************************
 *            GetConsoleOutputCP   (KERNEL32.189)
 */
UINT32 WINAPI GetConsoleOutputCP(VOID)
{
    return GetConsoleCP();
}

/***********************************************************************
 *            GetConsoleMode   (KERNEL32.188)
 */
BOOL32 WINAPI GetConsoleMode(HANDLE32 hcon,LPDWORD mode)
{
    struct get_console_mode_request req;
    struct get_console_mode_reply reply;

    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hcon,
                                              K32OBJ_CONSOLE, GENERIC_READ )) == -1)
        return FALSE;
    CLIENT_SendRequest( REQ_GET_CONSOLE_MODE, -1, 1, &req, sizeof(req));
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL )) return FALSE;
    *mode = reply.mode;
    return TRUE;
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
BOOL32 WINAPI SetConsoleMode( HANDLE32 hcon, DWORD mode )
{
    struct set_console_mode_request req;

    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hcon,
                                              K32OBJ_CONSOLE, GENERIC_READ )) == -1)
        return FALSE;
    req.mode = mode;
    CLIENT_SendRequest( REQ_SET_CONSOLE_MODE, -1, 1, &req, sizeof(req));
    return !CLIENT_WaitReply( NULL, NULL, 0 );
}


/***********************************************************************
 *            GetConsoleTitleA   (KERNEL32.191)
 */
DWORD WINAPI GetConsoleTitle32A(LPSTR title,DWORD size)
{
    struct get_console_info_request req;
    struct get_console_info_reply reply;
    int len;
    DWORD ret = 0;
    HANDLE32 hcon;

    if ((hcon = CreateFile32A( "CONOUT$", GENERIC_READ, 0, NULL,
                               OPEN_EXISTING, 0, 0 )) == INVALID_HANDLE_VALUE32)
        return 0;
    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hcon,
                                              K32OBJ_CONSOLE, GENERIC_READ )) == -1)
    {
        CloseHandle( hcon );
        return 0;
    }
    CLIENT_SendRequest( REQ_GET_CONSOLE_INFO, -1, 1, &req, sizeof(req) );
    if (!CLIENT_WaitReply( &len, NULL, 2, &reply, sizeof(reply), title, size ))
    {
        if (len > sizeof(reply)+size) title[size-1] = 0;
        ret = strlen(title);
    }
    CloseHandle( hcon );
    return ret;
}


/******************************************************************************
 * GetConsoleTitle32W [KERNEL32.192]  Retrieves title string for console
 *
 * PARAMS
 *    title [O] Address of buffer for title
 *    size  [I] Size of buffer
 *
 * RETURNS
 *    Success: Length of string copied
 *    Failure: 0
 */
DWORD WINAPI GetConsoleTitle32W( LPWSTR title, DWORD size )
{
    char *tmp;
    DWORD ret;

    if (!(tmp = HeapAlloc( GetProcessHeap(), 0, size ))) return 0;
    ret = GetConsoleTitle32A( tmp, size );
    lstrcpyAtoW( title, tmp );
    HeapFree( GetProcessHeap(), 0, tmp );
    return ret;
}


/***********************************************************************
 *            WriteConsoleA   (KERNEL32.729)
 */
BOOL32 WINAPI WriteConsole32A( HANDLE32 hConsoleOutput,
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
BOOL32 WINAPI WriteConsoleOutput32A( HANDLE32 hConsoleOutput,
                                     LPCHAR_INFO lpBuffer,
                                     COORD dwBufferSize,
                                     COORD dwBufferCoord,
                                     LPSMALL_RECT lpWriteRegion)
{
    int i,j,off=0,lastattr=-1;
    char	sbuf[20],*buffer=NULL;
    int		bufused=0,curbufsize = 100;
    DWORD	res;
    const int colormap[8] = {
    	0,4,2,6,
	1,5,3,7,
    };
    CONSOLE_make_complex(hConsoleOutput);
    buffer = HeapAlloc(GetProcessHeap(),0,100);;
    curbufsize = 100;

    TRACE(console,"wr: top = %d, bottom=%d, left=%d,right=%d\n",
    	lpWriteRegion->Top,
    	lpWriteRegion->Bottom,
    	lpWriteRegion->Left,
    	lpWriteRegion->Right
    );

    for (i=lpWriteRegion->Top;i<=lpWriteRegion->Bottom;i++) {
    	sprintf(sbuf,"%c[%d;%dH",27,i+1,lpWriteRegion->Left+1);
	SADD(sbuf);
	for (j=lpWriteRegion->Left;j<=lpWriteRegion->Right;j++) {
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
	    off++;
	}
    }
    sprintf(sbuf,"%c[0m",27);SADD(sbuf);
    WriteFile(hConsoleOutput,buffer,bufused,&res,NULL);
    HeapFree(GetProcessHeap(),0,buffer);
    return TRUE;
}

/***********************************************************************
 *            WriteConsoleW   (KERNEL32.577)
 */
BOOL32 WINAPI WriteConsole32W( HANDLE32 hConsoleOutput,
                               LPCVOID lpBuffer,
                               DWORD nNumberOfCharsToWrite,
                               LPDWORD lpNumberOfCharsWritten,
                               LPVOID lpReserved )
{
        BOOL32 ret;
        LPSTR xstring=HeapAlloc( GetProcessHeap(), 0, nNumberOfCharsToWrite );

	lstrcpynWtoA( xstring,  lpBuffer,nNumberOfCharsToWrite);

	/* FIXME: should I check if this is a console handle? */
	ret= WriteFile(hConsoleOutput, xstring, nNumberOfCharsToWrite,
			 lpNumberOfCharsWritten, NULL);
	HeapFree( GetProcessHeap(), 0, xstring );
	return ret;
}


/***********************************************************************
 *            ReadConsoleA   (KERNEL32.419)
 */
BOOL32 WINAPI ReadConsole32A( HANDLE32 hConsoleInput,
                              LPVOID lpBuffer,
                              DWORD nNumberOfCharsToRead,
                              LPDWORD lpNumberOfCharsRead,
                              LPVOID lpReserved )
{
    int		charsread = 0;
    LPSTR	xbuf = (LPSTR)lpBuffer;
    struct read_console_input_request req;
    INPUT_RECORD	ir;

    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hConsoleInput,
                                              K32OBJ_CONSOLE, GENERIC_READ )) == -1)
        return FALSE;
    TRACE(console,"(%d,%p,%ld,%p,%p)\n",
	    hConsoleInput,lpBuffer,nNumberOfCharsToRead,
	    lpNumberOfCharsRead,lpReserved
    );

    CONSOLE_get_input(hConsoleInput,FALSE);

    req.count = 1;
    req.flush = 1;

    /* FIXME: should we read at least 1 char? The SDK does not say */
    while (charsread<nNumberOfCharsToRead)
    {
    	int len;

        CLIENT_SendRequest( REQ_READ_CONSOLE_INPUT, -1, 1, &req, sizeof(req) );
        if (CLIENT_WaitReply( &len, NULL, 1, &ir, sizeof(ir) ))
            return FALSE;
        assert( !(len % sizeof(ir)) );
        if (!len) break;
    	if (!ir.Event.KeyEvent.bKeyDown)
		continue;
    	if (ir.EventType != KEY_EVENT)
		continue;
	*xbuf++ = ir.Event.KeyEvent.uChar.AsciiChar; 
	charsread++;
    }
    if (lpNumberOfCharsRead)
    	*lpNumberOfCharsRead = charsread;
    return TRUE;
}

/***********************************************************************
 *            ReadConsoleW   (KERNEL32.427)
 */
BOOL32 WINAPI ReadConsole32W( HANDLE32 hConsoleInput,
                              LPVOID lpBuffer,
                              DWORD nNumberOfCharsToRead,
                              LPDWORD lpNumberOfCharsRead,
                              LPVOID lpReserved )
{
    BOOL32 ret;
    LPSTR buf = (LPSTR)HeapAlloc(GetProcessHeap(), 0, nNumberOfCharsToRead);

    ret = ReadConsole32A(
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
 * ReadConsoleInput32A [KERNEL32.569]  Reads data from a console
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
BOOL32 WINAPI ReadConsoleInput32A(HANDLE32 hConsoleInput,
				  LPINPUT_RECORD lpBuffer,
				  DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
    struct read_console_input_request req;
    int len;

    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hConsoleInput,
                                              K32OBJ_CONSOLE, GENERIC_READ )) == -1)
        return FALSE;
    req.count = nLength;
    req.flush = 1;

    /* loop until we get at least one event */
    for (;;)
    {
        CLIENT_SendRequest( REQ_READ_CONSOLE_INPUT, -1, 1, &req, sizeof(req) );
        if (CLIENT_WaitReply( &len, NULL, 1, lpBuffer, nLength * sizeof(*lpBuffer) ))
            return FALSE;
        assert( !(len % sizeof(INPUT_RECORD)) );
        if (len) break;
	CONSOLE_get_input(hConsoleInput,TRUE);
        /*WaitForSingleObject( hConsoleInput, INFINITE32 );*/
    }
    if (lpNumberOfEventsRead) *lpNumberOfEventsRead = len / sizeof(INPUT_RECORD);
    return TRUE;
}


/***********************************************************************
 *            ReadConsoleInput32W   (KERNEL32.570)
 */
BOOL32 WINAPI ReadConsoleInput32W( HANDLE32 handle, LPINPUT_RECORD buffer,
                                   DWORD count, LPDWORD read )
{
    /* FIXME: Fix this if we get UNICODE input. */
    return ReadConsoleInput32A( handle, buffer, count, read );
}


/***********************************************************************
 *            FlushConsoleInputBuffer   (KERNEL32.132)
 */
BOOL32 WINAPI FlushConsoleInputBuffer( HANDLE32 handle )
{
    struct read_console_input_request req;
    int len;

    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), handle,
                                              K32OBJ_CONSOLE, GENERIC_READ )) == -1)
        return FALSE;
    req.count = -1;  /* get all records */
    req.flush = 1;
    CLIENT_SendRequest( REQ_READ_CONSOLE_INPUT, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitReply( &len, NULL, 0 );
}


/***********************************************************************
 *            PeekConsoleInputA   (KERNEL32.550)
 *
 * Gets 'count' first events (or less) from input queue.
 *
 * Does not need a complex console.
 */
BOOL32 WINAPI PeekConsoleInput32A( HANDLE32 handle, LPINPUT_RECORD buffer,
                                   DWORD count, LPDWORD read )
{
    struct read_console_input_request req;
    int len;

    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), handle,
                                              K32OBJ_CONSOLE, GENERIC_READ )) == -1)
        return FALSE;

    CONSOLE_get_input(handle,FALSE);
    req.count = count;
    req.flush = 0;

    CLIENT_SendRequest( REQ_READ_CONSOLE_INPUT, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitReply( &len, NULL, 1, buffer, count * sizeof(*buffer) ))
        return FALSE;
    assert( !(len % sizeof(INPUT_RECORD)) );
    if (read) *read = len / sizeof(INPUT_RECORD);
    return TRUE;
}


/***********************************************************************
 *            PeekConsoleInputW   (KERNEL32.551)
 */
BOOL32 WINAPI PeekConsoleInput32W(HANDLE32 hConsoleInput,
                                  LPINPUT_RECORD pirBuffer,
                                  DWORD cInRecords,
                                  LPDWORD lpcRead)
{
    /* FIXME: Hmm. Fix this if we get UNICODE input. */
    return PeekConsoleInput32A(hConsoleInput,pirBuffer,cInRecords,lpcRead);
}


/******************************************************************************
 * WriteConsoleInput32A [KERNEL32.730]  Write data to a console input buffer
 *
 */
BOOL32 WINAPI WriteConsoleInput32A( HANDLE32 handle, INPUT_RECORD *buffer,
                                    DWORD count, LPDWORD written )
{
    struct write_console_input_request req;
    struct write_console_input_reply reply;

    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), handle,
                                              K32OBJ_CONSOLE, GENERIC_WRITE )) == -1)
        return FALSE;
    req.count = count;
    CLIENT_SendRequest( REQ_WRITE_CONSOLE_INPUT, -1, 2, &req, sizeof(req),
                        buffer, count * sizeof(*buffer) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL )) return FALSE;
    if (written) *written = reply.written;
    return TRUE;
}


/***********************************************************************
 *            SetConsoleTitle32A   (KERNEL32.476)
 *
 * Sets the console title.
 *
 * We do not necessarily need to create a complex console for that,
 * but should remember the title and set it on creation of the latter.
 * (not fixed at this time).
 */
BOOL32 WINAPI SetConsoleTitle32A(LPCSTR title)
{
#if 0
    PDB32 *pdb = PROCESS_Current();
    CONSOLE *console;
    DWORD written;
    char titleformat[]="\033]2;%s\a"; /*this should work for xterms*/
    LPSTR titlestring; 
    BOOL32 ret=FALSE;

    TRACE(console,"(%s)\n",title);

    console = (CONSOLE *)pdb->console;
    if (!console)
	return FALSE;
    if(console->title) /* Free old title, if there is one */
	HeapFree( SystemHeap, 0, console->title );
    console->title = (LPSTR)HeapAlloc(SystemHeap, 0,strlen(title)+1);
    if(console->title) strcpy(console->title,title);
    titlestring = HeapAlloc(GetProcessHeap(), 0,strlen(title)+strlen(titleformat)+1);
    if (!titlestring) {
	K32OBJ_DecCount(&console->header);
	return FALSE;
    }

    sprintf(titlestring,titleformat,title);
#if 0
    /* only set title for complex console (own xterm) */
    if (console->pid != -1) {
	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),titlestring,strlen(titlestring),&written,NULL);
	if (written == strlen(titlestring))
	    ret =TRUE;
    } else
    	ret = TRUE;
#endif
    HeapFree( GetProcessHeap(), 0, titlestring );
    K32OBJ_DecCount(&console->header);
    return ret;



#endif

    struct set_console_info_request req;
    struct get_console_info_reply info;
    HANDLE32 hcon;
    DWORD written;

    if ((hcon = CreateFile32A( "CONOUT$", GENERIC_READ|GENERIC_WRITE, 0, NULL,
                               OPEN_EXISTING, 0, 0 )) == INVALID_HANDLE_VALUE32)
        return FALSE;
    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hcon,
                                              K32OBJ_CONSOLE, GENERIC_WRITE )) == -1)
        goto error;
    req.mask = SET_CONSOLE_INFO_TITLE;
    CLIENT_SendRequest( REQ_SET_CONSOLE_INFO, -1, 2, &req, sizeof(req),
                        title, strlen(title)+1 );
    if (CLIENT_WaitReply( NULL, NULL, 0 )) goto error;
    if (CONSOLE_GetInfo( hcon, &info ) && info.pid)
    {
        /* only set title for complex console (own xterm) */
        WriteFile( hcon, "\033]2;", 4, &written, NULL );
        WriteFile( hcon, title, strlen(title), &written, NULL );
        WriteFile( hcon, "\a", 1, &written, NULL );
    }
    return TRUE;
 error:
    CloseHandle( hcon );
    return FALSE;
}


/******************************************************************************
 * SetConsoleTitle32W [KERNEL32.477]  Sets title bar string for console
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
BOOL32 WINAPI SetConsoleTitle32W( LPCWSTR title )
{
    BOOL32 ret;

    LPSTR titleA = HEAP_strdupWtoA( GetProcessHeap(), 0, title );
    ret = SetConsoleTitle32A(titleA);
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
BOOL32 WINAPI SetConsoleCursorPosition( HANDLE32 hcon, COORD pos )
{
    char 	xbuf[20];
    DWORD	xlen;

    /* make console complex only if we change lines, not just in the line */
    if (pos.y)
    	CONSOLE_make_complex(hcon);

    TRACE(console, "%d (%dx%d)\n", hcon, pos.x , pos.y );
    /* x are columns, y rows */
    if (pos.y) 
    	/* full screen cursor absolute positioning */
	sprintf(xbuf,"%c[%d;%dH", 0x1B, pos.y+1, pos.x+1);
    else
    	/* relative cursor positioning in line (\r to go to 0) */
	sprintf(xbuf,"\r%c[%dC", 0x1B, pos.x);
    /* FIXME: store internal if we start using own console buffers */
    WriteFile(hcon,xbuf,strlen(xbuf),&xlen,NULL);
    return TRUE;
}

/***********************************************************************
 *            GetNumberOfConsoleInputEvents   (KERNEL32.246)
 */
BOOL32 WINAPI GetNumberOfConsoleInputEvents(HANDLE32 hcon,LPDWORD nrofevents)
{
    CONSOLE *console = CONSOLE_GetPtr( hcon );

    if (!console) {
    	FIXME(console,"(%d,%p), no console handle!\n",hcon,nrofevents);
    	return FALSE;
    }
    CONSOLE_get_input(hcon,FALSE);
    *nrofevents = 1; /* UMM */
    K32OBJ_DecCount(&console->header);
    return TRUE;
}

/***********************************************************************
 *            GetNumberOfConsoleMouseButtons   (KERNEL32.358)
 */
BOOL32 WINAPI GetNumberOfConsoleMouseButtons(LPDWORD nrofbuttons)
{
    FIXME(console,"(%p): stub\n", nrofbuttons);
    *nrofbuttons = 2;
    return TRUE;
}

/******************************************************************************
 * GetConsoleCursorInfo32 [KERNEL32.296]  Gets size and visibility of console
 *
 * PARAMS
 *    hcon  [I] Handle to console screen buffer
 *    cinfo [O] Address of cursor information
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL32 WINAPI GetConsoleCursorInfo32( HANDLE32 hcon,
                                      LPCONSOLE_CURSOR_INFO cinfo )
{
    struct get_console_info_reply reply;

    if (!CONSOLE_GetInfo( hcon, &reply )) return FALSE;
    if (cinfo)
    {
        cinfo->dwSize = reply.cursor_size;
        cinfo->bVisible = reply.cursor_visible;
    }
    return TRUE;
}


/******************************************************************************
 * SetConsoleCursorInfo32 [KERNEL32.626]  Sets size and visibility of cursor
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL32 WINAPI SetConsoleCursorInfo32( 
    HANDLE32 hcon,                /* [in] Handle to console screen buffer */
    LPCONSOLE_CURSOR_INFO cinfo)  /* [in] Address of cursor information */
{
    struct set_console_info_request req;
    char	buf[8];
    DWORD	xlen;

    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hcon,
                                              K32OBJ_CONSOLE, GENERIC_WRITE )) == -1)
        return FALSE;
    CONSOLE_make_complex(hcon);
    sprintf(buf,"\033[?25%c",cinfo->bVisible?'h':'l');
    WriteFile(hcon,buf,strlen(buf),&xlen,NULL);

    req.cursor_size    = cinfo->dwSize;
    req.cursor_visible = cinfo->bVisible;
    req.mask           = SET_CONSOLE_INFO_CURSOR;
    CLIENT_SendRequest( REQ_SET_CONSOLE_INFO, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitReply( NULL, NULL, 0 );
}


/******************************************************************************
 * SetConsoleWindowInfo [KERNEL32.634]  Sets size and position of console
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL32 WINAPI SetConsoleWindowInfo(
    HANDLE32 hcon,       /* [in] Handle to console screen buffer */
    BOOL32 bAbsolute,    /* [in] Coordinate type flag */
    LPSMALL_RECT window) /* [in] Address of new window rectangle */
{
    FIXME(console, "(%x,%d,%p): stub\n", hcon, bAbsolute, window);
    return TRUE;
}


/******************************************************************************
 * SetConsoleTextAttribute32 [KERNEL32.631]  Sets colors for text
 *
 * Sets the foreground and background color attributes of characters
 * written to the screen buffer.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL32 WINAPI SetConsoleTextAttribute32(HANDLE32 hConsoleOutput,WORD wAttr)
{
    const int colormap[8] = {
    	0,4,2,6,
	1,5,3,7,
    };
    DWORD xlen;
    char buffer[20];

    TRACE(console,"(%d,%d)\n",hConsoleOutput,wAttr);
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
BOOL32 WINAPI SetConsoleScreenBufferSize( HANDLE32 hConsoleOutput, 
                                          COORD dwSize )
{
    FIXME(console, "(%d,%dx%d): stub\n",hConsoleOutput,dwSize.x,dwSize.y);
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
BOOL32 WINAPI FillConsoleOutputCharacterA(
    HANDLE32 hConsoleOutput,
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
BOOL32 WINAPI FillConsoleOutputCharacterW(HANDLE32 hConsoleOutput,
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
BOOL32 WINAPI FillConsoleOutputAttribute( HANDLE32 hConsoleOutput, 
              WORD wAttribute, DWORD nLength, COORD dwCoord, 
              LPDWORD lpNumAttrsWritten)
{
    FIXME(console, "(%d,%d,%ld,%dx%d,%p): stub\n", hConsoleOutput,
          wAttribute,nLength,dwCoord.x,dwCoord.y,lpNumAttrsWritten);
    *lpNumAttrsWritten = nLength;
    return TRUE;
}

/******************************************************************************
 * ReadConsoleOutputCharacter32A [KERNEL32.573]
 * 
 * BUGS
 *   Unimplemented
 */
BOOL32 WINAPI ReadConsoleOutputCharacter32A(HANDLE32 hConsoleOutput, 
	      LPSTR lpstr, DWORD dword, COORD coord, LPDWORD lpdword)
{
    FIXME(console, "(%d,%p,%ld,%dx%d,%p): stub\n", hConsoleOutput,lpstr,
	  dword,coord.x,coord.y,lpdword);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/******************************************************************************
 * ScrollConsoleScreenBuffer [KERNEL32.612]
 * 
 * BUGS
 *   Unimplemented
 */
BOOL32 WINAPI ScrollConsoleScreenBuffer( HANDLE32 hConsoleOutput, 
	      LPSMALL_RECT lpScrollRect, LPSMALL_RECT lpClipRect,
              COORD dwDestOrigin, LPCHAR_INFO lpFill)
{
    FIXME(console, "(%d,%p,%p,%dx%d,%p): stub\n", hConsoleOutput,lpScrollRect,
	  lpClipRect,dwDestOrigin.x,dwDestOrigin.y,lpFill);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
