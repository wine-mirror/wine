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
	DWORD			mode;
	CONSOLE_CURSOR_INFO	cinfo;

	int			master;	/* xterm side of pty */
	int			infd,outfd;
        int                     hread, hwrite;  /* server handles (hack) */
	int			pid;	/* xterm's pid, -1 if no xterm */
        LPSTR   		title;	/* title of console */
	INPUT_RECORD		*irs;	/* buffered input records */
	int			nrofirs;/* nr of buffered input records */
} CONSOLE;

static void CONSOLE_Destroy( K32OBJ *obj );

const K32OBJ_OPS CONSOLE_Ops =
{
	CONSOLE_Destroy		/* destroy */
};

/***********************************************************************
 * CONSOLE_Destroy
 */
static void CONSOLE_Destroy(K32OBJ *obj)
{
	CONSOLE *console = (CONSOLE *)obj;
	assert(obj->type == K32OBJ_CONSOLE);

	obj->type = K32OBJ_UNKNOWN;

	if (console->title)
	  	  HeapFree( SystemHeap, 0, console->title );
	console->title = NULL;
	/* if there is a xterm, kill it. */
	if (console->pid != -1)
		kill(console->pid, SIGTERM);
	HeapFree(SystemHeap, 0, console);
}


/****************************************************************************
 *		CONSOLE_add_input_record			[internal]
 *
 * Adds an INPUT_RECORD to the CONSOLEs input queue.
 */
static void
CONSOLE_add_input_record(CONSOLE *console,INPUT_RECORD *inp) {
	console->irs = HeapReAlloc(GetProcessHeap(),0,console->irs,sizeof(INPUT_RECORD)*(console->nrofirs+1));
	console->irs[console->nrofirs++]=*inp;
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
    CONSOLE *console = (CONSOLE*)HANDLE_GetObjPtr( PROCESS_Current(), hConsoleInput,
                                                   K32OBJ_CONSOLE, 0, NULL);

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
	    if (inchar=='\n') {
		ir.Event.KeyEvent.uChar.AsciiChar	= '\r';
	    	ir.Event.KeyEvent.wVirtualKeyCode	= 0x0d;
	    	ir.Event.KeyEvent.wVirtualScanCode	= 0x1c;
	    } else {
		ir.Event.KeyEvent.uChar.AsciiChar = inchar;
		if (inchar<' ') {
		    /* FIXME: find good values for ^X */
		    ir.Event.KeyEvent.wVirtualKeyCode = 0xdead;
		    ir.Event.KeyEvent.wVirtualScanCode = 0xbeef;
		} 
	    }

	    CONSOLE_add_input_record(console,&ir);
	    ir.Event.KeyEvent.bKeyDown = 0;
	    CONSOLE_add_input_record(console,&ir);
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
	    CONSOLE_add_input_record(console,&ir);
	    ir.Event.KeyEvent.bKeyDown = 0;
	    CONSOLE_add_input_record(console,&ir);
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
		    CONSOLE_add_input_record(console,&ir);
		    j=k+3;
		}
	    	break;
	    	
	    }
	    if (scancode) {
		ir.Event.KeyEvent.wVirtualScanCode = scancode;
		ir.Event.KeyEvent.wVirtualKeyCode = MapVirtualKey16(scancode,1);
		CONSOLE_add_input_record(console,&ir);
		ir.Event.KeyEvent.bKeyDown		= 0;
		CONSOLE_add_input_record(console,&ir);
		j=k;
		continue;
	    }
	}
    }
    K32OBJ_DecCount(&console->header);
}

/****************************************************************************
 * 		CONSOLE_get_input		(internal)
 *
 * Reads (nonblocking) as much input events as possible and stores them
 * in an internal queue.
 */
static void
CONSOLE_get_input( HANDLE32 handle )
{
    char	*buf = HeapAlloc(GetProcessHeap(),0,1);
    int		len = 0;

    while (1)
    {
	DWORD res;
	char inchar;
        if (WaitForSingleObject( handle, 0 )) break;
        if (!ReadFile( handle, &inchar, 1, &res, NULL )) break;
	buf = HeapReAlloc(GetProcessHeap(),0,buf,len+1);
	buf[len++]=inchar;
    }
    CONSOLE_string_to_IR(handle,buf,len);
    HeapFree(GetProcessHeap(),0,buf);
}

/****************************************************************************
 * 		CONSOLE_drain_input		(internal)
 *
 * Drains 'n' console input events from the queue.
 */
static void
CONSOLE_drain_input(CONSOLE *console,int n) {
    assert(n<=console->nrofirs);
    if (n) {
	console->nrofirs-=n;
    	memcpy(	&console->irs[0],
		&console->irs[n],
		console->nrofirs*sizeof(INPUT_RECORD)
	);
	console->irs = HeapReAlloc(
		GetProcessHeap(),
		0,
		console->irs,
		console->nrofirs*sizeof(INPUT_RECORD)
	);
    }
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

        HANDLE_CloseAll( pdb, &console->header );

	K32OBJ_DecCount( &console->header );
	pdb->console = NULL;
	SYSTEM_UNLOCK();
	return TRUE;
}


/** 
 *  It looks like the openpty that comes with glibc in RedHat 5.0
 *  is buggy (second call returns what looks like a dup of 0 and 1
 *  instead of a new pty), this is a generic replacement.
 */
static int CONSOLE_openpty(CONSOLE *console, char *name, 
			struct termios *term, struct winsize *winsize)
{
        int temp, slave;
        struct set_console_fd_request req;

        temp = wine_openpty(&console->master, &slave, name, term,
           winsize);
        console->infd = console->outfd = slave;

        req.handle = console->hread;
        CLIENT_SendRequest(REQ_SET_CONSOLE_FD, dup(slave), 1,
           &req, sizeof(req));
        CLIENT_WaitReply( NULL, NULL, 0);
        req.handle = console->hwrite;
        CLIENT_SendRequest( REQ_SET_CONSOLE_FD, dup(slave), 1,
           &req, sizeof(req));
        CLIENT_WaitReply( NULL, NULL, 0);

        return temp;  /* The same result as the openpty call */
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
 * To test for complex console: pid == -1 -> simple, otherwise complex.
 */
static BOOL32 CONSOLE_make_complex(HANDLE32 handle,CONSOLE *console)
{
	struct termios term;
	char buf[30],*title;
	char c = '\0';
	int status = 0;
	int i,xpid;
	DWORD	xlen;

	if (console->pid != -1)
		return TRUE; /* already complex */

	MSG("Console: Making console complex (creating an xterm)...\n");

	if (tcgetattr(0, &term) < 0) return FALSE;
	term.c_lflag = ~(ECHO|ICANON);
	if (CONSOLE_openpty(console, NULL, &term, NULL) < 0) return FALSE;

	if ((xpid=fork()) == 0) {
		tcsetattr(console->infd, TCSADRAIN, &term);
		sprintf(buf, "-Sxx%d", console->master);
		/* "-fn vga" for VGA font. Harmless if vga is not present:
		 *  xterm: unable to open font "vga", trying "fixed".... 
		 */
		execlp("xterm", "xterm", buf, "-fn","vga",NULL);
		ERR(console, "error creating AllocConsole xterm\n");
		exit(1);
	}
	console->pid = xpid;

	/* most xterms like to print their window ID when used with -S;
	 * read it and continue before the user has a chance...
	 */
	for (i=0; c!='\n'; (status=read(console->infd, &c, 1)), i++) {
		if (status == -1 && c == '\0') {
				/* wait for xterm to be created */
			usleep(100);
		}
		if (i > 10000) {
			ERR(console, "can't read xterm WID\n");
			kill(console->pid, SIGKILL);
			return FALSE;
		}
	}
	/* enable mouseclicks */
	sprintf(buf,"%c[?1001s%c[?1000h",27,27);
	WriteFile(handle,buf,strlen(buf),&xlen,NULL);
	if (console->title) {
	    title = HeapAlloc(GetProcessHeap(),0,strlen("\033]2;\a")+1+strlen(console->title));
	    sprintf(title,"\033]2;%s\a",console->title);
	    WriteFile(handle,title,strlen(title),&xlen,NULL);
	    HeapFree(GetProcessHeap(),0,title);
	}
	return TRUE;

}

/***********************************************************************
 *		CONSOLE_GetConsoleHandle
 *	 returns a 16-bit style console handle
 * note: only called from _lopen
 */
HFILE32 CONSOLE_GetConsoleHandle(VOID)
{
	PDB32 *pdb = PROCESS_Current();
	HFILE32 handle = HFILE_ERROR32;

	SYSTEM_LOCK();
	if (pdb->console != NULL) {
		CONSOLE *console = (CONSOLE *)pdb->console;
		handle = (HFILE32)HANDLE_Alloc(pdb, &console->header, 0, TRUE, -1);
	}
	SYSTEM_UNLOCK();
	return handle;
}

/***********************************************************************
 *            AllocConsole (KERNEL32.103)
 *
 * creates an xterm with a pty to our program
 */
BOOL32 WINAPI AllocConsole(VOID)
{
        struct create_console_request req;
        struct create_console_reply reply;
	PDB32 *pdb = PROCESS_Current();
	CONSOLE *console;
	HANDLE32 hIn, hOut, hErr;

	SYSTEM_LOCK();		/* FIXME: really only need to lock the process */

	SetLastError(ERROR_CANNOT_MAKE);  /* this might not be the right 
					     error, but it's a good guess :) */

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
        console->pid             = -1;
        console->title           = NULL;
	console->nrofirs	 = 0;
	console->irs	 	 = HeapAlloc(GetProcessHeap(),0,1);;
    	console->mode		 =   ENABLE_PROCESSED_INPUT
				   | ENABLE_LINE_INPUT
				   | ENABLE_ECHO_INPUT;
	/* FIXME: we shouldn't probably use hardcoded UNIX values here. */
	console->infd		 = 0;
	console->outfd		 = 1;
        console->hread = console->hwrite = -1;

        CLIENT_SendRequest( REQ_CREATE_CONSOLE, -1, 1, &req, sizeof(req) );
        if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL ) != ERROR_SUCCESS)
        {
            K32OBJ_DecCount(&console->header);
            SYSTEM_UNLOCK();
            return FALSE;
        }
        console->hread = reply.handle_read;
        console->hwrite = reply.handle_write;

	if ((hIn = HANDLE_Alloc(pdb,&console->header, 0, TRUE,
                                reply.handle_read)) == INVALID_HANDLE_VALUE32)
        {
            CLIENT_CloseHandle( reply.handle_write );
            K32OBJ_DecCount(&console->header);
            SYSTEM_UNLOCK();
            return FALSE;
	}

	if ((hOut = HANDLE_Alloc(pdb,&console->header, 0, TRUE,
                                 reply.handle_write)) == INVALID_HANDLE_VALUE32)
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
    CONSOLE	*console = (CONSOLE*)HANDLE_GetObjPtr( PROCESS_Current(), hcon, K32OBJ_CONSOLE, 0, NULL);

    if (!console) {
    	FIXME(console,"(%d,%p), no console handle passed!\n",hcon,mode);
	return FALSE;
    }
    *mode = console->mode;
    K32OBJ_DecCount(&console->header);
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
    CONSOLE	*console = (CONSOLE*)HANDLE_GetObjPtr( PROCESS_Current(), hcon, K32OBJ_CONSOLE, 0, NULL);

    if (!console) {
    	FIXME(console,"(%d,%ld), no console handle passed!\n",hcon,mode);
	return FALSE;
    }
    FIXME(console,"(0x%08x,0x%08lx): stub\n",hcon,mode);
    console->mode = mode;
    K32OBJ_DecCount(&console->header);
    return TRUE;
}


/***********************************************************************
 *            GetConsoleTitleA   (KERNEL32.191)
 */
DWORD WINAPI GetConsoleTitle32A(LPSTR title,DWORD size)
{
    PDB32 *pdb = PROCESS_Current();
    CONSOLE *console= (CONSOLE *)pdb->console;

    if(console && console->title) {
	lstrcpyn32A(title,console->title,size);
	return strlen(title);
    }
    return 0;
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
    PDB32 *pdb = PROCESS_Current();
    CONSOLE *console= (CONSOLE *)pdb->console;
    if(console && console->title) 
    {
        lstrcpynAtoW(title,console->title,size);
        return (lstrlen32W(title));
    }
    return 0;
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
    CONSOLE	*console = (CONSOLE*)HANDLE_GetObjPtr( PROCESS_Current(), hConsoleOutput, K32OBJ_CONSOLE, 0, NULL);

    if (!console) {
    	FIXME(console,"(%d,...): no console handle!\n",hConsoleOutput);
	return FALSE;
    }
    CONSOLE_make_complex(hConsoleOutput,console);
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
    K32OBJ_DecCount(&console->header);
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
    CONSOLE	*console = (CONSOLE*)HANDLE_GetObjPtr( PROCESS_Current(), hConsoleInput, K32OBJ_CONSOLE, 0, NULL);
    int		i,charsread = 0;
    LPSTR	xbuf = (LPSTR)lpBuffer;

    if (!console) {
    	SetLastError(ERROR_INVALID_HANDLE);
	FIXME(console,"(%d,...), no console handle!\n",hConsoleInput);
    	return FALSE;
    }
    TRACE(console,"(%d,%p,%ld,%p,%p)\n",
	    hConsoleInput,lpBuffer,nNumberOfCharsToRead,
	    lpNumberOfCharsRead,lpReserved
    );
    CONSOLE_get_input(hConsoleInput);

    /* FIXME: should we read at least 1 char? The SDK does not say */
    for (i=0;(i<console->nrofirs)&&(charsread<nNumberOfCharsToRead);i++) {
    	if (console->irs[i].EventType != KEY_EVENT)
		continue;
    	if (!console->irs[i].Event.KeyEvent.bKeyDown)
		continue;
	*xbuf++ = console->irs[i].Event.KeyEvent.uChar.AsciiChar; 
	charsread++;
    }
    /* SDK says: Drains all other input events from queue. */
    CONSOLE_drain_input(console,i);
    if (lpNumberOfCharsRead)
    	*lpNumberOfCharsRead = charsread;
    K32OBJ_DecCount(&console->header);
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
    CONSOLE	*console = (CONSOLE*)HANDLE_GetObjPtr( PROCESS_Current(), hConsoleInput, K32OBJ_CONSOLE, 0, NULL);

    TRACE(console, "(%d,%p,%ld,%p)\n",hConsoleInput, lpBuffer, nLength,
          lpNumberOfEventsRead);
    if (!console) {
    	FIXME(console, "(%d,%p,%ld,%p), No console handle!\n",hConsoleInput,
		lpBuffer, nLength, lpNumberOfEventsRead);

        /* Indicate that nothing was read */
        *lpNumberOfEventsRead = 0;

	return FALSE;
    }
    CONSOLE_get_input(hConsoleInput);
    /* SDK: return at least 1 input record */
    while (!console->nrofirs) {
    	DWORD res;

    	res=WaitForSingleObject(hConsoleInput,0);
	switch (res) {
	case STATUS_TIMEOUT:	continue;
	case 0:			break; /*ok*/
	case WAIT_FAILED:	return 0;/*FIXME: SetLastError?*/
	default:		break;	/*hmm*/
	}
	CONSOLE_get_input(hConsoleInput);
    }
    
    if (nLength>console->nrofirs)
    	nLength = console->nrofirs;
    memcpy(lpBuffer,console->irs,sizeof(INPUT_RECORD)*nLength);
    if (lpNumberOfEventsRead)
	*lpNumberOfEventsRead = nLength;
    CONSOLE_drain_input(console,nLength);
    K32OBJ_DecCount(&console->header);
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
    /* only set title for complex console (own xterm) */
    if (console->pid != -1) {
	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),titlestring,strlen(titlestring),&written,NULL);
	if (written == strlen(titlestring))
	    ret =TRUE;
    } else
    	ret = TRUE;
    HeapFree( GetProcessHeap(), 0, titlestring );
    K32OBJ_DecCount(&console->header);
    return ret;
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

/***********************************************************************
 *            ReadConsoleInput32W   (KERNEL32.570)
 */
BOOL32 WINAPI ReadConsoleInput32W(HANDLE32 hConsoleInput,
                                LPINPUT_RECORD lpBuffer,
                                DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
    FIXME(console, "(%d,%p,%ld,%p): stub\n",hConsoleInput, lpBuffer, nLength,
          lpNumberOfEventsRead);
    return 0;
}

/***********************************************************************
 *            FlushConsoleInputBuffer   (KERNEL32.132)
 */
BOOL32 WINAPI FlushConsoleInputBuffer(HANDLE32 hConsoleInput)
{
    CONSOLE	*console = (CONSOLE*)HANDLE_GetObjPtr( PROCESS_Current(), hConsoleInput, K32OBJ_CONSOLE, 0, NULL);

    if (!console)
    	return FALSE;
    CONSOLE_drain_input(console,console->nrofirs);
    K32OBJ_DecCount(&console->header);
    return TRUE;
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
    CONSOLE	*console = (CONSOLE*)HANDLE_GetObjPtr( PROCESS_Current(), hcon, K32OBJ_CONSOLE, 0, NULL);

    if (!console) {
    	FIXME(console,"(%d,...), no console handle!\n",hcon);
    	return FALSE;
    }
    CONSOLE_make_complex(hcon,console);
    TRACE(console, "%d (%dx%d)\n", hcon, pos.x , pos.y );
    /* x are columns, y rows */
    sprintf(xbuf,"%c[%d;%dH", 0x1B, pos.y+1, pos.x+1);
    /* FIXME: store internal if we start using own console buffers */
    WriteFile(hcon,xbuf,strlen(xbuf),&xlen,NULL);
    K32OBJ_DecCount(&console->header);
    return TRUE;
}

/***********************************************************************
 *            GetNumberOfConsoleInputEvents   (KERNEL32.246)
 */
BOOL32 WINAPI GetNumberOfConsoleInputEvents(HANDLE32 hcon,LPDWORD nrofevents)
{
    CONSOLE	*console = (CONSOLE*)HANDLE_GetObjPtr( PROCESS_Current(), hcon, K32OBJ_CONSOLE, 0, NULL);

    if (!console) {
    	FIXME(console,"(%d,%p), no console handle!\n",hcon,nrofevents);
    	return FALSE;
    }
    CONSOLE_get_input(hcon);
    *nrofevents = console->nrofirs;
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

/***********************************************************************
 *            PeekConsoleInputA   (KERNEL32.550)
 *
 * Gets 'cInRecords' first events (or less) from input queue.
 *
 * Does not need a complex console.
 */
BOOL32 WINAPI PeekConsoleInput32A(HANDLE32 hConsoleInput,
                                  LPINPUT_RECORD pirBuffer,
                                  DWORD cInRecords,
                                  LPDWORD lpcRead)
{
    CONSOLE	*console = (CONSOLE*)HANDLE_GetObjPtr( PROCESS_Current(), hConsoleInput, K32OBJ_CONSOLE, 0, NULL);

    if (!console) {
        FIXME(console,"(%d,%p,%ld,%p), No console handle passed!\n",hConsoleInput, pirBuffer, cInRecords, lpcRead);

        /* Indicate that nothing was read */
        *lpcRead = 0; 

    	return FALSE;
    }
    TRACE(console,"(%d,%p,%ld,%p)\n",hConsoleInput, pirBuffer, cInRecords, lpcRead);
    CONSOLE_get_input(hConsoleInput);
    if (cInRecords>console->nrofirs)
    	cInRecords = console->nrofirs;
    if (pirBuffer)
	memcpy(pirBuffer,console->irs,cInRecords*sizeof(INPUT_RECORD));
    if (lpcRead)
	*lpcRead = cInRecords;
    K32OBJ_DecCount(&console->header);
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
    CONSOLE	*console = (CONSOLE*)HANDLE_GetObjPtr( PROCESS_Current(), hcon, K32OBJ_CONSOLE, 0, NULL);

    if (!console) {
    	FIXME(console, "(%x,%p), no console handle!\n", hcon, cinfo);
	return FALSE;
    }
    TRACE(console, "(%x,%p)\n", hcon, cinfo);
    if (!cinfo)
    	return FALSE;
    *cinfo = console->cinfo;
    K32OBJ_DecCount(&console->header);
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
    char	buf[8];
    DWORD	xlen;
    CONSOLE	*console = (CONSOLE*)HANDLE_GetObjPtr( PROCESS_Current(), hcon, K32OBJ_CONSOLE, 0, NULL);

    TRACE(console, "(%x,%ld,%i): stub\n", hcon,cinfo->dwSize,cinfo->bVisible);
    if (!console)
    	return FALSE;
    CONSOLE_make_complex(hcon,console);
    sprintf(buf,"%c[?25%c",27,cinfo->bVisible?'h':'l');
    WriteFile(hcon,buf,strlen(buf),&xlen,NULL);
    console->cinfo = *cinfo;
    K32OBJ_DecCount(&console->header);
    return TRUE;
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
