/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <stdio.h>
#include <malloc.h>
#include "windows.h"
#include "winerror.h"
#include "wincon.h"
#include "heap.h"
#include "xmalloc.h"
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
BOOL32 SetConsoleCtrlHandler(HANDLER_ROUTINE * func,  BOOL32 a)
{
	return 0;
}

/***********************************************************************
 *           GetConsoleScreenBufferInfo   (KERNEL32.190)
 */
BOOL32 GetConsoleScreenBufferInfo( HANDLE32 hConsoleOutput,
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
DWORD GetLargestConsoleWindowSize( HANDLE32 hConsoleOutput )
{
    return (DWORD)MAKELONG(dummyinfo.dwMaximumWindowSize.x,dummyinfo.dwMaximumWindowSize.y);
}

/***********************************************************************
 *            GetConsoleCP   (KERNEL32.226)
 */
UINT32 GetConsoleCP(VOID)
{
    return GetACP();
}

/***********************************************************************
 *            GetConsoleOutputCP   (KERNEL32.189)
 */
UINT32 GetConsoleOutputCP(VOID)
{
    return GetConsoleCP();
}

/***********************************************************************
 *            GetConsoleMode   (KERNEL32.188)
 */
BOOL32 GetConsoleMode(HANDLE32 hcon,LPDWORD mode)
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
BOOL32 SetConsoleMode(HANDLE32 hcon,DWORD mode)
{
    fprintf(stdnimp,"SetConsoleMode(%08x,%08lx)\n",hcon,mode);
    return TRUE;
}

/***********************************************************************
 *            GetConsoleTitleA   (KERNEL32.191)
 */
DWORD GetConsoleTitle32A(LPSTR title,DWORD size)
{
    lstrcpyn32A(title,"Console",size);
    return strlen("Console");
}

/***********************************************************************
 *            GetConsoleTitleW   (KERNEL32.192)
 */
DWORD GetConsoleTitle32W(LPWSTR title,DWORD size)
{
    lstrcpynAtoW(title,"Console",size);
    return strlen("Console");
}

/***********************************************************************
 *            WriteConsoleA   (KERNEL32.567)
 */
BOOL32 WriteConsole32A(
	HANDLE32 hConsoleOutput,
	LPVOID lpBuffer,
	DWORD nNumberOfCharsToWrite,
	LPDWORD lpNumberOfCharsWritten,
	LPVOID lpReserved )
{
	LPSTR	buf = (LPSTR)xmalloc(nNumberOfCharsToWrite+1);

	lstrcpyn32A(buf,lpBuffer,nNumberOfCharsToWrite);
	buf[nNumberOfCharsToWrite]=0;
	fprintf(stderr,"%s",buf);
	free(buf);
	*lpNumberOfCharsWritten=nNumberOfCharsToWrite;
	return TRUE;
}

/***********************************************************************
 *            WriteConsoleW   (KERNEL32.577)
 */
BOOL32 WriteConsole32W(
	HANDLE32 hConsoleOutput,
	LPVOID lpBuffer,
	DWORD nNumberOfCharsToWrite,
	LPDWORD lpNumberOfCharsWritten,
	LPVOID lpReserved )
{
	LPSTR	buf = (LPSTR)xmalloc(2*nNumberOfCharsToWrite+1);

	lstrcpynWtoA(buf,lpBuffer,nNumberOfCharsToWrite);
	buf[nNumberOfCharsToWrite]=0;
	fprintf(stderr,"%s",buf);
	free(buf);
	*lpNumberOfCharsWritten=nNumberOfCharsToWrite;
	return TRUE;
}

/***********************************************************************
 *            ReadConsoleA   (KERNEL32.419)
 */
BOOL32 ReadConsole32A(
	HANDLE32 hConsoleInput,
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
BOOL32 ReadConsole32W(
	HANDLE32 hConsoleInput,
	LPVOID lpBuffer,
	DWORD nNumberOfCharsToRead,
	LPDWORD lpNumberOfCharsRead,
	LPVOID lpReserved )
{
	LPSTR	buf = (LPSTR)xmalloc(nNumberOfCharsToRead);

	fgets(buf,nNumberOfCharsToRead,stdin);
	lstrcpynAtoW(lpBuffer,buf,nNumberOfCharsToRead);
	*lpNumberOfCharsRead = strlen(buf);
	return TRUE;
}

/***********************************************************************
 *            SetConsoleTitle32A   (KERNEL32.476)
 */
BOOL32 SetConsoleTitle32A(LPCSTR title)
{
    fprintf(stderr,"SetConsoleTitle(%s)\n",title);
    return TRUE;
}

/***********************************************************************
 *            SetConsoleTitle32W   (KERNEL32.477)
 */
BOOL32 SetConsoleTitle32W( LPCWSTR title )
{
    LPSTR titleA = HEAP_strdupWtoA( GetProcessHeap(), 0, title );
    fprintf(stderr,"SetConsoleTitle(%s)\n",titleA);
    HeapFree( GetProcessHeap(), 0, titleA );
    return TRUE;
}

/***********************************************************************
 *            FlushConsoleInputBuffer   (KERNEL32.132)
 */
BOOL32 FlushConsoleInputBuffer(HANDLE32 hConsoleInput){
    fprintf(stderr,"FlushConsoleInputBuffer(%d)\n",hConsoleInput);
    return TRUE;
}
