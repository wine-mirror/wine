/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 * Copyright 1997 Karl Garrison
 */

#include <stdio.h>
#include <stdlib.h>
#include "windows.h"
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
    *lpNumberOfCharsWritten = fprintf( stderr, "%.*s",
                                       (int)nNumberOfCharsToWrite,
                                       (LPSTR)lpBuffer );
    return TRUE;
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
    LPSTR buf =  HEAP_strdupWtoA( GetProcessHeap(), 0, lpBuffer );
    *lpNumberOfCharsWritten = fprintf( stderr, "%.*s",
                                       (int)nNumberOfCharsToWrite, buf );
    HeapFree( GetProcessHeap(), 0, buf );
    return TRUE;
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
	fgets(lpBuffer,nNumberOfCharsToRead,stdin);
	*lpNumberOfCharsRead = strlen(lpBuffer);
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
    LPSTR buf = (LPSTR)HEAP_xalloc( GetProcessHeap(), 0, nNumberOfCharsToRead);
    fgets(buf,nNumberOfCharsToRead,stdin);
    lstrcpynAtoW(lpBuffer,buf,nNumberOfCharsToRead);
    *lpNumberOfCharsRead = strlen(buf);
    HeapFree( GetProcessHeap(), 0, buf );
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
		fprintf(stderr,"[%dC",c.x);
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
