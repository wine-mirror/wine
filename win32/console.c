/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 * Copyright 1997 Karl Garrison
 * Copyright 1998 John Richardson
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pty.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include "windows.h"
#include "k32obj.h"
#include "file.h"
#include "process.h"
#include "winerror.h"
#include "wincon.h"
#include "heap.h"
#include "stddebug.h"
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
 * ioctls later.  The pid is needed to detroy the xterm if needed.
 */
typedef struct _CONSOLE {
	K32OBJ  header;
	int	master;			/* xterm side of pty */
	int	slave;			/* wine side of pty */
	int	pid;			/* xterm's pid, -1 if no xterm */
	int	flags;			/* CONSOLE_STARTED_FROM */
        K32OBJ *file_in;                /* console input */
        K32OBJ *file_out;               /* console output */
        K32OBJ *file_err;               /* console error */
} CONSOLE;

#define CONSOLE_STARTED_FROM  (0x1)	/* FIXME: this is lame, it should have 
					   something to do with sharing... */

static void CONSOLE_Destroy( K32OBJ *obj );

const K32OBJ_OPS CONSOLE_Ops =
{
	NULL,			/* signaled */
	NULL,			/* satisfied */
	NULL,			/* add_wait */
	NULL,			/* remove_wait */
	CONSOLE_Destroy		/* destroy */
};


static int wine_openpty(int *master, int *slave, char *name, 
		     struct termios *term, struct winsize *winsize);



static void CONSOLE_Destroy(K32OBJ *obj)
{
	CONSOLE *console = (CONSOLE *)obj;
	assert(obj->type == K32OBJ_CONSOLE);

	obj->type = K32OBJ_UNKNOWN;

	/* make sure a xterm exists to kill */
	if (console->pid != -1) {
		kill(console->pid, SIGTERM);
	}
	HeapFree(SystemHeap, 0, console);

}


/***********************************************************************
 *           SetConsoleCtrlHandler               (KERNEL32.459)
 */
BOOL32 WINAPI SetConsoleCtrlHandler(HANDLER_ROUTINE * func,  BOOL32 a)
{
	return 0;
}

/***********************************************************************
 *           CreateConsoleScreenBuffer   (KERNEL32.151)
 */
HANDLE32 WINAPI CreateConsoleScreenBuffer( DWORD dwDesiredAccess,
                                           DWORD dwShareMode,
                                           LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                           DWORD dwFlags,
                                           LPVOID lpScreenBufferData)
{
	fprintf(stderr, "CreateConsoleScreenBuffer(): stub !\n");
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

/***********************************************************************
 *           SetConsoleActiveScreenBuffer   (KERNEL32.623)
 */
BOOL32 WINAPI SetConsoleActiveScreenBuffer(HANDLE32 hConsoleOutput)
{
	fprintf(stderr, "SetConsoleActiveScreenBuffer(): stub !\n");
	return 0;
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

        if (console->file_in) K32OBJ_DecCount( console->file_in );
        if (console->file_out) K32OBJ_DecCount( console->file_out );
        if (console->file_err) K32OBJ_DecCount( console->file_err );
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
		fprintf(stderr, "error creating AllocConsole xterm\n");
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
			fprintf(stderr, "can't read xterm WID\n");
			kill(*pid, SIGKILL);
			return FALSE;
		}
	}
	term.c_lflag |= ECHO;
	tcsetattr(*master, TCSADRAIN, &term);

	return TRUE;

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

	/* we only want to be able to open a console if the process doesn't have one
	 * now or we got the one we have from our parent
	 *  - invalid handle comes from when the console was closed via FreeConsole()
	 *  - CONSOLE_STARTED_FROM is when this process inherits its console from
	 *    its parent
	 */
	if (console && (console->flags & CONSOLE_STARTED_FROM) == 0) {
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
        console->file_in         = NULL;
        console->file_out        = NULL;
        console->file_err        = NULL;

	if (wine_createConsole(&master, &slave, &pid) == FALSE) {
		K32OBJ_DecCount(&console->header);
		SYSTEM_UNLOCK();
		return FALSE;
	}

	/* save the pid and other info for future use */
        console->master = master;
	console->slave = slave;
	console->pid = pid;
	console->flags = 0;

	if ((hIn = FILE_DupUnixHandle(slave)) == INVALID_HANDLE_VALUE32)
        {
            K32OBJ_DecCount(&console->header);
            SYSTEM_UNLOCK();
            return FALSE;
	}
	FILE_SetFileType(hIn, FILE_TYPE_CHAR);

	if ((hOut = FILE_DupUnixHandle(slave)) == INVALID_HANDLE_VALUE32)
        {
            CloseHandle(hIn);
            K32OBJ_DecCount(&console->header);
            SYSTEM_UNLOCK();
            return FALSE;
	}
	FILE_SetFileType(hOut, FILE_TYPE_CHAR);

	if ((hErr = FILE_DupUnixHandle(slave)) == INVALID_HANDLE_VALUE32)
        {
            CloseHandle(hIn);
            CloseHandle(hOut);
            K32OBJ_DecCount(&console->header);
            SYSTEM_UNLOCK();
            return FALSE;
	}
	FILE_SetFileType(hErr, FILE_TYPE_CHAR);

        console->file_in = HANDLE_GetObjPtr( hIn, K32OBJ_FILE, 0 /*FIXME*/ );
        console->file_out = HANDLE_GetObjPtr( hIn, K32OBJ_FILE, 0 /*FIXME*/ );
        console->file_err = HANDLE_GetObjPtr( hIn, K32OBJ_FILE, 0 /*FIXME*/ );

	/* associate this console with the process */
        if (pdb->console) K32OBJ_DecCount( pdb->console );
	pdb->console = (K32OBJ *)console;

	/* NT resets the STD_*_HANDLEs on console alloc */
	SetStdHandle(STD_INPUT_HANDLE, hIn);
	SetStdHandle(STD_OUTPUT_HANDLE, hOut);
	SetStdHandle(STD_ERROR_HANDLE, hErr);

	SetLastError(ERROR_SUCCESS);
	SYSTEM_UNLOCK();
	return TRUE;
}


/***********************************************************************
 *            GetConsoleCP   (KERNEL32.226)
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

/***********************************************************************
 *            SetConsoleMode   (KERNEL32.188)
 */
BOOL32 WINAPI SetConsoleMode(HANDLE32 hcon,DWORD mode)
{
    fprintf(stdnimp,"SetConsoleMode(%08x,%08lx)\n",hcon,mode);
    return TRUE;
}

/***********************************************************************
 *            GetConsoleTitleA   (KERNEL32.191)
 */
DWORD WINAPI GetConsoleTitle32A(LPSTR title,DWORD size)
{
    lstrcpyn32A(title,"Console",size);
    return strlen("Console");
}

/***********************************************************************
 *            GetConsoleTitleW   (KERNEL32.192)
 */
DWORD WINAPI GetConsoleTitle32W(LPWSTR title,DWORD size)
{
    lstrcpynAtoW(title,"Console",size);
    return strlen("Console");
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

#ifdef OLD
	*lpNumberOfCharsWritten = fprintf(CONSOLE_console.conIO, "%.*s",
					   (int)nNumberOfCharsToWrite,
					   (LPSTR)lpBuffer );
	if (ferror(CONSOLE_console.conIO) {
		clearerr();
		return FALSE;
	}

	return TRUE;
#endif
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
	return FALSE;
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

	/* FIXME: should I check if this is a console handle? */
	return WriteFile(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite,
			 lpNumberOfCharsWritten, NULL);


#ifdef OLD
	LPSTR buf =  HEAP_strdupWtoA( GetProcessHeap(), 0, lpBuffer );
	*lpNumberOfCharsWritten = fprintf(CONSOLE_console.conIO, "%.*s",
					  (int)nNumberOfCharsToWrite, buf );
	HeapFree( GetProcessHeap(), 0, buf );

	if (ferror(CONSOLE_console.conIO) {
		clearerr();
		return FALSE;
	}

	return TRUE;
#endif
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
	return ReadFile(hConsoleInput, lpBuffer, nNumberOfCharsToRead,
			lpNumberOfCharsRead, NULL);

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
#endif
	return TRUE;

}

/***********************************************************************
 *            SetConsoleTitle32A   (KERNEL32.476)
 */
BOOL32 WINAPI SetConsoleTitle32A(LPCSTR title)
{
    fprintf(stderr,"SetConsoleTitle(%s)\n",title);
    return TRUE;
}

/***********************************************************************
 *            SetConsoleTitle32W   (KERNEL32.477)
 */
BOOL32 WINAPI SetConsoleTitle32W( LPCWSTR title )
{
    LPSTR titleA = HEAP_strdupWtoA( GetProcessHeap(), 0, title );
    fprintf(stderr,"SetConsoleTitle(%s)\n",titleA);
    HeapFree( GetProcessHeap(), 0, titleA );
    return TRUE;
}

/***********************************************************************
 *            FlushConsoleInputBuffer   (KERNEL32.132)
 */
BOOL32 WINAPI FlushConsoleInputBuffer(HANDLE32 hConsoleInput)
{
    fprintf(stderr,"FlushConsoleInputBuffer(%d)\n",hConsoleInput);
    return TRUE;
}

BOOL32 WINAPI SetConsoleCursorPosition(HANDLE32 hcons,COORD c)
{
    /* x are columns, y rows */
    if (!c.y) {
    	fprintf(stderr,"\r");
	if (c.x)
		fprintf(stderr,"%c[%dC", 0x1B, c.x); /* note: 0x1b == ESC */
	return TRUE;
    }
    /* handle rest of the cases */
    return FALSE;
}

/***********************************************************************
 *            GetNumberOfConsoleInputEvents   (KERNEL32.246)
 */
BOOL32 WINAPI GetNumberOfConsoleInputEvents(HANDLE32 hcon,LPDWORD nrofevents)
{
	*nrofevents = 0;
	return TRUE;
}

/***********************************************************************
 *            GetNumberOfConsoleMouseButtons   (KERNEL32.358)
 */
BOOL32 WINAPI GetNumberOfConsoleMouseButtons(LPDWORD nrofbuttons)
{
    *nrofbuttons = 2;
    fprintf(stderr,"GetNumberOfConsoleMouseButtons: STUB returning 2\n");
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
    pirBuffer = NULL;
    cInRecords = 0;
    *lpcRead = 0;
    fprintf(stderr,"PeekConsoleInput32A: STUB returning TRUE\n");
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
    fprintf(stderr,"PeekConsoleInput32W: STUB returning TRUE\n");
    return TRUE;
}

/***********************************************************************
 *            GetConsoleCursorInfo32   (KERNEL32.296)
 */
BOOL32 WINAPI GetConsoleCursorInfo32(HANDLE32 hcon, LPDWORD cinfo)
{
  cinfo[0] = 10; /* 10% of character box is cursor.  */
  cinfo[1] = TRUE;  /* Cursor is visible.  */
  fprintf (stdnimp, "GetConsoleCursorInfo32 -- STUB!\n");
  return TRUE;
}

/***********************************************************************
 *            SetConsoleCursorInfo32   (KERNEL32.626)
 */
BOOL32 WINAPI SetConsoleCursorInfo32(HANDLE32 hcon, LPDWORD cinfo)
{
  fprintf (stdnimp, "SetConsoleCursorInfo32 -- STUB!\n");
  return TRUE;
}
