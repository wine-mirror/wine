/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
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
			ENABLE_ECHO_INPUT	|
			ENABLE_WINDOW_INPUT	|
			ENABLE_MOUSE_INPUT;
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
 *            WriteConsoleA   (KERNEL32.567)
 */
BOOL32 WINAPI WriteConsole32A( HANDLE32 hConsoleOutput,
                               LPVOID lpBuffer,
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
 *            WriteConsoleW   (KERNEL32.577)
 */
BOOL32 WINAPI WriteConsole32W( HANDLE32 hConsoleOutput,
                               LPVOID lpBuffer,
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
BOOL32 WINAPI FlushConsoleInputBuffer(HANDLE32 hConsoleInput){
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
	*nrofevents = 1;
	return TRUE;
}
