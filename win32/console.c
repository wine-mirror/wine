/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <stdio.h>
#include "windows.h"
#include "winerror.h"
#include "wincon.h"
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
BOOL SetConsoleCtrlHandler(HANDLER_ROUTINE * func,  BOOL a)
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
COORD GetLargestConsoleWindowSize( HANDLE32 hConsoleOutput )
{
    return dummyinfo.dwMaximumWindowSize;
}




