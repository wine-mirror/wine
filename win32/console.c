/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 * Copyright 1997 Karl Garrison
 * Copyright 1998 John Richardson
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
#include "file.h"
#include "process.h"
#include "winerror.h"
#include "wincon.h"
#include "heap.h"
#include "debug.h"

static CONSOLE_SCREEN_BUFFER_INFO dummyinfo =
{
    {80, 24},
    {0, 0},
    0,
    {0, 0, 79, 23},
    {80, 24}
};

/* The console -- I chose to keep the master and slave
 * (UNIX) file descriptors around in case they are needed for
 * ioctls later.  The pid is needed to destroy the xterm on close
 */
typedef struct _CONSOLE {
	K32OBJ  header;
	int	master;			/* xterm side of pty */
	int	slave;			/* wine side of pty */
	int	pid;			/* xterm's pid, -1 if no xterm */
        LPSTR   title;                  /* title of console */
} CONSOLE;


/* This probably belongs somewhere else */
typedef struct _CONSOLE_CURSOR_INFO {
    DWORD  dwSize;   /* Between 1 & 100 for percentage of cell filled */
    BOOL32 bVisible; /* Visibility of cursor */
} CONSOLE_CURSOR_INFO, *LPCONSOLE_CURSOR_INFO;



static void CONSOLE_Destroy( K32OBJ *obj );
static BOOL32 CONSOLE_Write(K32OBJ *ptr, LPCVOID lpBuffer, 
			    DWORD nNumberOfChars,  LPDWORD lpNumberOfChars, 
			    LPOVERLAPPED lpOverlapped);
static BOOL32 CONSOLE_Read(K32OBJ *ptr, LPVOID lpBuffer, 
			   DWORD nNumberOfChars, LPDWORD lpNumberOfChars, 
			   LPOVERLAPPED lpOverlapped);
static int wine_openpty(int *master, int *slave, char *name, 
		     struct termios *term, struct winsize *winsize);
static BOOL32 wine_createConsole(int *master, int *slave, int *pid);

const K32OBJ_OPS CONSOLE_Ops =
{
	NULL,			/* signaled */
	NULL,			/* satisfied */
	NULL,			/* add_wait */
	NULL,			/* remove_wait */
	CONSOLE_Read,		/* read */
	CONSOLE_Write,		/* write */
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
	/* make sure a xterm exists to kill */
	if (console->pid != -1) {
		kill(console->pid, SIGTERM);
	}
	HeapFree(SystemHeap, 0, console);
}


/***********************************************************************
 * CONSOLE_Read
 *
 * NOTES
 *    lpOverlapped is ignored
 */
static BOOL32 CONSOLE_Read(K32OBJ *ptr, LPVOID lpBuffer, DWORD nNumberOfChars,
			LPDWORD lpNumberOfChars, LPOVERLAPPED lpOverlapped)
{
	CONSOLE *console = (CONSOLE *)ptr;
	int result;

	TRACE(console, "%p %p %ld\n", ptr, lpBuffer,
		     nNumberOfChars);

	*lpNumberOfChars = 0; 

	if ((result = read(console->slave, lpBuffer, nNumberOfChars)) == -1) {
		FILE_SetDosError();
		return FALSE;
	}
	*lpNumberOfChars = result;
	return TRUE;
}


/***********************************************************************
 * CONSOLE_Write
 *
 * NOTES
 *    lpOverlapped is ignored
 */
static BOOL32 CONSOLE_Write(K32OBJ *ptr, LPCVOID lpBuffer, 
			    DWORD nNumberOfChars,
			    LPDWORD lpNumberOfChars, LPOVERLAPPED lpOverlapped)
{
	CONSOLE *console = (CONSOLE *)ptr;
	int result;

	TRACE(console, "%p %p %ld\n", ptr, lpBuffer,
		     nNumberOfChars);

	*lpNumberOfChars = 0; 

	/* 
	 * I assume this loop around EAGAIN is here because
	 * win32 doesn't have interrupted system calls 
	 */

	for (;;)
        {
		result = write(console->slave, lpBuffer, nNumberOfChars);
		if (result != -1) {
			*lpNumberOfChars = result;
                        return TRUE;
		}
		if (errno != EINTR) {
			FILE_SetDosError();
			return FALSE;
		}
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
 */
BOOL32 WINAPI SetConsoleCtrlHandler( HANDLER_ROUTINE *func, BOOL32 add )
{
    FIXME(console, "(%p,%i): stub\n",func,add);
    return TRUE;
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
    return 1;
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
    return (DWORD)MAKELONG(dummyinfo.dwMaximumWindowSize.x,dummyinfo.dwMaximumWindowSize.y);
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
static int wine_openpty(int *master, int *slave, char *name, 
			struct termios *term, struct winsize *winsize)
{
        int fdm, fds;
        char *ptr1, *ptr2;
        char pts_name[512];

        strcpy (pts_name, "/dev/ptyXY");

        for (ptr1 = "pqrstuvwxyzPQRST"; *ptr1 != 0; ptr1++) {
                pts_name[8] = *ptr1;
                for (ptr2 = "0123456789abcdef"; *ptr2 != 0; ptr2++) {
                        pts_name[9] = *ptr2;

                        if ((fdm = open(pts_name, O_RDWR)) < 0) {
                                if (errno == ENOENT)
                                        return -1;
                                else
                                        continue;
                        }
                        pts_name[5] = 't';
                        if ((fds = open(pts_name, O_RDWR)) < 0) {
                                pts_name[5] = 'p';
                                continue;
                        }
                        *master = fdm;
                        *slave = fds;
			
			if (term != NULL)
				tcsetattr(*slave, TCSANOW, term);
			if (winsize != NULL)
				ioctl(*slave, TIOCSWINSZ, winsize);
			if (name != NULL)
				strcpy(name, pts_name);
                        return fds;
                }
        }
	return -1;
}

static BOOL32 wine_createConsole(int *master, int *slave, int *pid)
{
	struct termios term;
	char buf[1024];
	char c = '\0';
	int status = 0;
	int i;

	if (tcgetattr(0, &term) < 0) return FALSE;
	term.c_lflag |= ICANON;
	term.c_lflag &= ~ECHO;
	if (wine_openpty(master, slave, NULL, &term, NULL) < 0) return FALSE;

	if ((*pid=fork()) == 0) {
		tcsetattr(*slave, TCSADRAIN, &term);
		sprintf(buf, "-Sxx%d", *master);
		execlp("xterm", "xterm", buf, NULL);
		ERR(console, "error creating AllocConsole xterm\n");
		exit(1);
	}

	/* most xterms like to print their window ID when used with -S;
	 * read it and continue before the user has a chance...
	 * NOTE: this is the reason we started xterm with ECHO off, 
	 * we'll turn it back on below
	 */

	for (i=0; c!='\n'; (status=read(*slave, &c, 1)), i++) {
		if (status == -1 && c == '\0') {
				/* wait for xterm to be created */
			usleep(100);
		}
		if (i > 10000) {
			WARN(console, "can't read xterm WID\n");
			kill(*pid, SIGKILL);
			return FALSE;
		}
	}
	term.c_lflag |= ECHO;
	tcsetattr(*master, TCSADRAIN, &term);

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

	int master;
	int slave;
	int pid;
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

	if (wine_createConsole(&master, &slave, &pid) == FALSE) {
		K32OBJ_DecCount(&console->header);
		SYSTEM_UNLOCK();
		return FALSE;
	}

	/* save the pid and other info for future use */
        console->master = master;
	console->slave = slave;
	console->pid = pid;

	if ((hIn = HANDLE_Alloc(pdb,&console->header, 0, TRUE,-1)) == INVALID_HANDLE_VALUE32)
        {
            K32OBJ_DecCount(&console->header);
            SYSTEM_UNLOCK();
            return FALSE;
	}

	if ((hOut = HANDLE_Alloc(pdb,&console->header, 0, TRUE,-1)) == INVALID_HANDLE_VALUE32)
        {
            CloseHandle(hIn);
            K32OBJ_DecCount(&console->header);
            SYSTEM_UNLOCK();
            return FALSE;
	}


	if ((hErr = HANDLE_Alloc(pdb,&console->header, 0, TRUE,-1)) == INVALID_HANDLE_VALUE32)
        {
            CloseHandle(hIn);
            CloseHandle(hOut);
            K32OBJ_DecCount(&console->header);
            SYSTEM_UNLOCK();
            return FALSE;
	}

	/* associate this console with the process */
        if (pdb->console) K32OBJ_DecCount( pdb->console );
	pdb->console = (K32OBJ *)console;

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
	*mode = 	ENABLE_PROCESSED_INPUT	|
			ENABLE_LINE_INPUT	|
			ENABLE_ECHO_INPUT;
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
    FIXME(console,"(0x%08x,0x%08lx): stub\n",hcon,mode);
    return TRUE;
}


/***********************************************************************
 *            GetConsoleTitleA   (KERNEL32.191)
 */
DWORD WINAPI GetConsoleTitle32A(LPSTR title,DWORD size)
{
	PDB32 *pdb = PROCESS_Current();
	CONSOLE *console= (CONSOLE *)pdb->console;

	if(console && console->title) 
	  {
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
    char	buffer[200];
    DWORD	res;
    static int colormap[8] = {
    	0,4,2,6,
	1,5,3,7,
    };

    TRACE(console,"wr: top = %d, bottom=%d, left=%d,right=%d\n",
    	lpWriteRegion->Top,
    	lpWriteRegion->Bottom,
    	lpWriteRegion->Left,
    	lpWriteRegion->Right
    );

    for (i=lpWriteRegion->Top;i<=lpWriteRegion->Bottom;i++) {
    	sprintf(buffer,"%c[%d;%dH",27,i,lpWriteRegion->Left);
	WriteFile(hConsoleOutput,buffer,strlen(buffer),&res,NULL);
	if (lastattr != lpBuffer[off].Attributes) {
		lastattr = lpBuffer[off].Attributes;
		sprintf(buffer,"%c[0;3%d;4%d",27,colormap[lastattr&7],colormap[(lastattr&0x70)>>4]);
		if (lastattr & FOREGROUND_INTENSITY)
			strcat(buffer,";1");
		/* BACKGROUND_INTENSITY */
		strcat(buffer,"m");
		WriteFile(hConsoleOutput,buffer,strlen(buffer),&res,NULL);
	}
	for (j=lpWriteRegion->Left;j<=lpWriteRegion->Right;j++)
		WriteFile(hConsoleOutput,&(lpBuffer[off++].Char.AsciiChar),1,&res,NULL);
    }
    sprintf(buffer,"%c[0m",27);
    WriteFile(hConsoleOutput,buffer,strlen(buffer),&res,NULL);
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
	return ReadFile(hConsoleInput, lpBuffer, nNumberOfCharsToRead,
			lpNumberOfCharsRead, NULL);

#ifdef OLD
        fgets(lpBuffer,nNumberOfCharsToRead, CONSOLE_console.conIO);
	if (ferror(CONSOLE_console.conIO) {
		clearerr();
		return FALSE;
	}
        *lpNumberOfCharsRead = strlen(lpBuffer);
        return TRUE;
#endif

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
	LPSTR buf = (LPSTR)HeapAlloc(GetProcessHeap(), 0, 
				       nNumberOfCharsToRead);
	ret = ReadFile(hConsoleInput, buf, nNumberOfCharsToRead,
			lpNumberOfCharsRead, NULL);
	lstrcpynAtoW(lpBuffer,buf,nNumberOfCharsToRead);
	*lpNumberOfCharsRead = strlen(buf);
	HeapFree( GetProcessHeap(), 0, buf );
	return ret;

#ifdef OLD
	LPSTR buf = (LPSTR)HEAP_xalloc(GetProcessHeap(), 0, 
				       nNumberOfCharsToRead);
	fgets(buf, nNumberOfCharsToRead, CONSOLE_console.conIO);

	if (ferror(CONSOLE_console.conIO) {
		HeapFree( GetProcessHeap(), 0, buf );
		clearerr();
		return FALSE;
	}

	lstrcpynAtoW(lpBuffer,buf,nNumberOfCharsToRead);
	*lpNumberOfCharsRead = strlen(buf);
	HeapFree( GetProcessHeap(), 0, buf );
	return TRUE;
#endif

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
BOOL32 WINAPI ReadConsoleInputA( HANDLE32 hConsoleInput,
                                 LPINPUT_RECORD lpBuffer,
                                 DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
    FIXME(console, "(%d,%p,%ld,%p): stub\n",hConsoleInput, lpBuffer, nLength,
          lpNumberOfEventsRead);
    return ReadConsole32A(hConsoleInput, lpBuffer, nLength, 
                          lpNumberOfEventsRead, 0);
}


/***********************************************************************
 *            SetConsoleTitle32A   (KERNEL32.476)
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
	if (!titlestring)
	  return FALSE;
	
 	sprintf(titlestring,titleformat,title);
	/* Ugly casting */
	CONSOLE_Write((K32OBJ *)console,titlestring,strlen(titlestring),&written,NULL);
	if (written == strlen(titlestring))
	  ret =TRUE;
	HeapFree( GetProcessHeap(), 0, titlestring );
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
 *            FlushConsoleInputBuffer   (KERNEL32.132)
 */
BOOL32 WINAPI FlushConsoleInputBuffer(HANDLE32 hConsoleInput)
{
    FIXME(console,"(%d): stub\n",hConsoleInput);
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
BOOL32 WINAPI SetConsoleCursorPosition( HANDLE32 hConsoleOutput, 
                                        COORD dwCursorPosition )
{
    TRACE(console, "%d (%d x %d)\n", hConsoleOutput, dwCursorPosition.x, 
          dwCursorPosition.y);
    /* x are columns, y rows */
    	fprintf(stderr,"\r");
            /* note: 0x1B == ESC */
    fprintf(stdout,"%c[%d;%dH", 0x1B, dwCursorPosition.y, dwCursorPosition.x);
	return TRUE;
}


/***********************************************************************
 *            GetNumberOfConsoleInputEvents   (KERNEL32.246)
 */
BOOL32 WINAPI GetNumberOfConsoleInputEvents(HANDLE32 hcon,LPDWORD nrofevents)
{
    *nrofevents = 0;
    FIXME(console,"(%x): stub\n", hcon);
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
 */
BOOL32 WINAPI PeekConsoleInput32A(HANDLE32 hConsoleInput,
                                  LPINPUT_RECORD pirBuffer,
                                  DWORD cInRecords,
                                  LPDWORD lpcRead)
{
    fd_set infds;
    int	i,fd = FILE_GetUnixHandle(hConsoleInput);
    struct timeval tv;
    char	inchar;
    DWORD	res;
    static int keystate[256];
    LPINPUT_RECORD	ir = pirBuffer;

    TRACE(console,"(%d,%p,%ld,%p): stub\n",hConsoleInput, pirBuffer, cInRecords, lpcRead);

    memset(&tv,0,sizeof(tv));
    FD_ZERO(&infds);
    FD_SET(fd,&infds);
    select(fd+1,&infds,NULL,NULL,&tv);

    if (FD_ISSET(fd,&infds)) {
    	ReadFile(hConsoleInput,&inchar,1,&res,NULL);
	for (i=0;i<256;i++)  {
	    if (keystate[i] && i!=inchar) {
	    	ir->EventType = 1;
	    	ir->Event.KeyEvent.bKeyDown = 0;
	    	ir->Event.KeyEvent.wRepeatCount = 0;
	    	ir->Event.KeyEvent.wVirtualKeyCode = 0;
	    	ir->Event.KeyEvent.wVirtualScanCode = 0;
	    	ir->Event.KeyEvent.uChar.AsciiChar = i;
	    	ir->Event.KeyEvent.dwControlKeyState = 0;
		ir++;
		keystate[i]=0;
	    }
	}

	ir->EventType = 1; /* Key_event */
	ir->Event.KeyEvent.bKeyDown = 1;
	ir->Event.KeyEvent.wRepeatCount = 0;
	ir->Event.KeyEvent.wVirtualKeyCode = 0;
	ir->Event.KeyEvent.wVirtualScanCode = 0;
	ir->Event.KeyEvent.uChar.AsciiChar = inchar;
	ir->Event.KeyEvent.dwControlKeyState = 0;
	keystate[inchar]=1;
	ir++;
	*lpcRead = ir-pirBuffer;
    } else {
	*lpcRead = 0;
    }
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
    pirBuffer = NULL;
    cInRecords = 0;
    *lpcRead = 0;
    FIXME(console,"(%d,%p,%ld,%p): stub\n", hConsoleInput, pirBuffer,
          cInRecords, lpcRead);
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
    FIXME(console, "(%x,%p): stub\n", hcon, cinfo);
    if (!cinfo) return FALSE;
    cinfo->dwSize = 10;      /* 10% of character box is cursor.  */
    cinfo->bVisible = TRUE;  /* Cursor is visible.  */
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
    FIXME(console, "(%x,%ld,%i): stub\n", hcon,cinfo->dwSize,cinfo->bVisible);
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
    int forecolor = 0;
    int backcolor = 0;
    int boldtext = 0;
    unsigned int attrib;
    
    attrib = wAttr;
    if( attrib >= BACKGROUND_INTENSITY )
        attrib -= BACKGROUND_INTENSITY; /* Background intensity is ignored */
    if( attrib >= BACKGROUND_RED )
    {
        attrib -= BACKGROUND_RED;
        if( attrib >= BACKGROUND_GREEN )
        {
            attrib -= BACKGROUND_GREEN;
            if( attrib >= BACKGROUND_BLUE )
            {
                attrib -= BACKGROUND_BLUE;
                backcolor = 47; /* White background */
            }
            else
                backcolor = 43; /* Yellow background */
        }
        else if( attrib >= BACKGROUND_BLUE )
        {
            attrib -= BACKGROUND_BLUE;
            backcolor = 45; /* Magenta background */
        }
        else
            backcolor = 41; /* Red Background */
    }
    else if( attrib >= BACKGROUND_GREEN )
    {
        attrib -= BACKGROUND_GREEN;
        if( attrib >= BACKGROUND_BLUE )
        {
            attrib -= BACKGROUND_BLUE;
            backcolor = 46; /* Cyan background */
        }
        else
            backcolor = 42; /* Green background */
    }
    else if( attrib >= BACKGROUND_BLUE )
    {
        attrib -= BACKGROUND_BLUE;
        backcolor = 44; /* Blue background */
    }
    else
        backcolor = 40; /* Black background */
    if( attrib >= FOREGROUND_INTENSITY )
    {
        attrib -= FOREGROUND_INTENSITY;
        boldtext = 1;   /* Bold attribute is on */
    }
    if( attrib >= FOREGROUND_RED )
    {
        attrib -= FOREGROUND_RED;
        if( attrib >= FOREGROUND_GREEN )
        {
            attrib -= FOREGROUND_GREEN;
            if( attrib >= FOREGROUND_BLUE )
            {
                attrib -= FOREGROUND_BLUE;
                forecolor = 37; /* White foreground */
            }
            else
                forecolor = 33; /* Yellow foreground */
        }
        else if( attrib >= FOREGROUND_BLUE )
        {
            attrib -= FOREGROUND_BLUE;
            forecolor = 35; /* Magenta foreground */
        }
        else
            forecolor = 31; /* Red foreground */
    }
    else if( attrib >= FOREGROUND_GREEN )
    {
        attrib -= FOREGROUND_GREEN;
        if( attrib >= FOREGROUND_BLUE )
        {
            attrib -= FOREGROUND_BLUE;
            forecolor = 36; /* Cyan foreground */
        }
        else
            forecolor = 32; /* Green foreground */
    }
    else if( attrib >= FOREGROUND_BLUE )
    {
        attrib -= FOREGROUND_BLUE;
        forecolor = 34; /* Blue foreground */
    }
    else
        forecolor = 30; /* Black foreground */

    fprintf(stdout,"%c[%d;%d;%dm",0x1B,boldtext,forecolor,backcolor);
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
    long count;
    SetConsoleCursorPosition(hConsoleOutput,dwCoord);
    for(count=0;count<nLength;count++)
        putc(cCharacter,stdout);
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
    long count;
    SetConsoleCursorPosition(hConsoleOutput,dwCoord);
    for(count=0;count<nLength;count++)
        putc(cCharacter,stdout);
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
