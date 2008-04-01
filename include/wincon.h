/*
 * Copyright (C) the Wine project
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

#ifndef __WINE_WINCON_H
#define __WINE_WINCON_H

#ifdef __cplusplus
extern "C" {
#endif

#define CONSOLE_FULLSCREEN          1
#define CONSOLE_FULLSCREEN_HARDWARE 2

#define CTRL_C_EVENT        0
#define CTRL_BREAK_EVENT    1
#define CTRL_CLOSE_EVENT    2
#define CTRL_LOGOFF_EVENT   5
#define CTRL_SHUTDOWN_EVENT 6

/* Console Mode flags */
#define ENABLE_PROCESSED_INPUT 0x01
#define ENABLE_LINE_INPUT      0x02
#define ENABLE_ECHO_INPUT      0x04
#define ENABLE_WINDOW_INPUT    0x08
#define ENABLE_MOUSE_INPUT     0x10

#define ENABLE_PROCESSED_OUTPUT   0x01
#define ENABLE_WRAP_AT_EOL_OUTPUT 0x02


typedef BOOL (WINAPI *PHANDLER_ROUTINE)(DWORD dwCtrlType);

/* Attributes flags: */

#define FOREGROUND_BLUE      0x0001 /* text color contains blue. */
#define FOREGROUND_GREEN     0x0002 /* text color contains green. */
#define FOREGROUND_RED       0x0004 /* text color contains red. */
#define FOREGROUND_INTENSITY 0x0008 /* text color is intensified. */
#define BACKGROUND_BLUE      0x0010 /* background color contains blue. */
#define BACKGROUND_GREEN     0x0020 /* background color contains green. */
#define BACKGROUND_RED       0x0040 /* background color contains red. */
#define BACKGROUND_INTENSITY 0x0080 /* background color is intensified. */

typedef struct _CONSOLE_CURSOR_INFO {
    DWORD	dwSize;   /* Between 1 & 100 for percentage of cell filled */
    BOOL	bVisible; /* Visibility of cursor */
} CONSOLE_CURSOR_INFO, *LPCONSOLE_CURSOR_INFO;

typedef struct tagCOORD
{
    SHORT X;
    SHORT Y;
} COORD, *LPCOORD;

typedef struct tagSMALL_RECT
{
    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;
} SMALL_RECT,*LPSMALL_RECT;

typedef struct tagCONSOLE_SCREEN_BUFFER_INFO
{
    COORD       dwSize;
    COORD       dwCursorPosition;
    WORD        wAttributes;
    SMALL_RECT  srWindow;
    COORD       dwMaximumWindowSize;
} CONSOLE_SCREEN_BUFFER_INFO,*LPCONSOLE_SCREEN_BUFFER_INFO;

typedef struct tagCHAR_INFO
{
    union
	{
	WCHAR UnicodeChar;
	CHAR AsciiChar;
	} Char;
    WORD	Attributes;
} CHAR_INFO,*LPCHAR_INFO;

typedef struct tagKEY_EVENT_RECORD
{
    BOOL	bKeyDown;		/* 04 */
    WORD	wRepeatCount;		/* 08 */
    WORD	wVirtualKeyCode;	/* 0A */
    WORD	wVirtualScanCode;	/* 0C */
    union				/* 0E */
	{
	WCHAR UnicodeChar;		/* 0E */
	CHAR AsciiChar;			/* 0E */
	} uChar;
    DWORD	dwControlKeyState;	/* 10 */
} KEY_EVENT_RECORD,*LPKEY_EVENT_RECORD;

/* dwControlKeyState bitmask */
#define	RIGHT_ALT_PRESSED	0x0001
#define	LEFT_ALT_PRESSED	0x0002
#define	RIGHT_CTRL_PRESSED	0x0004
#define	LEFT_CTRL_PRESSED	0x0008
#define	SHIFT_PRESSED		0x0010
#define	NUMLOCK_ON		0x0020
#define	SCROLLLOCK_ON		0x0040
#define	CAPSLOCK_ON		0x0080
#define	ENHANCED_KEY		0x0100

typedef struct tagMOUSE_EVENT_RECORD
{
    COORD	dwMousePosition;
    DWORD	dwButtonState;
    DWORD	dwControlKeyState;
    DWORD	dwEventFlags;
} MOUSE_EVENT_RECORD,*LPMOUSE_EVENT_RECORD;

#define FROM_LEFT_1ST_BUTTON_PRESSED    0x0001
#define RIGHTMOST_BUTTON_PRESSED        0x0002
#define FROM_LEFT_2ND_BUTTON_PRESSED    0x0004
#define FROM_LEFT_3RD_BUTTON_PRESSED    0x0008
#define FROM_LEFT_4TH_BUTTON_PRESSED    0x0010

#define MOUSE_MOVED                     0x0001
#define DOUBLE_CLICK                    0x0002
#define MOUSE_WHEELED                   0x0004

typedef struct tagWINDOW_BUFFER_SIZE_RECORD
{
    COORD	dwSize;
} WINDOW_BUFFER_SIZE_RECORD,*LPWINDOW_BUFFER_SIZE_RECORD;

typedef struct tagMENU_EVENT_RECORD
{
    UINT	dwCommandId; /* perhaps UINT16 ??? */
} MENU_EVENT_RECORD,*LPMENU_EVENT_RECORD;

typedef struct tagFOCUS_EVENT_RECORD
{
    BOOL      bSetFocus; /* perhaps BOOL16 ??? */
} FOCUS_EVENT_RECORD,*LPFOCUS_EVENT_RECORD;

typedef struct tagINPUT_RECORD
{
    WORD		EventType;		/* 00 */
    union
	{
	KEY_EVENT_RECORD KeyEvent;
	MOUSE_EVENT_RECORD MouseEvent;
	WINDOW_BUFFER_SIZE_RECORD WindowBufferSizeEvent;
	MENU_EVENT_RECORD MenuEvent;
	FOCUS_EVENT_RECORD FocusEvent;
	} Event;
} INPUT_RECORD,*PINPUT_RECORD;

/* INPUT_RECORD.wEventType */
#define KEY_EVENT			0x01
#define MOUSE_EVENT			0x02
#define WINDOW_BUFFER_SIZE_EVENT	0x04
#define MENU_EVENT			0x08
#define FOCUS_EVENT 			0x10

#define CONSOLE_TEXTMODE_BUFFER  1

#ifdef __i386__
/* Note: this should return a COORD, but calling convention for returning
 * structures is different between Windows and gcc on i386. */
WINBASEAPI DWORD WINAPI GetLargestConsoleWindowSize(HANDLE);

static inline COORD __wine_GetLargestConsoleWindowSize_wrapper(HANDLE h)
{
    union {
      COORD c;
      DWORD dw;
    } u;
    u.dw = GetLargestConsoleWindowSize(h);
    return u.c;
}
#define GetLargestConsoleWindowSize(h) __wine_GetLargestConsoleWindowSize_wrapper(h)

#else  /* __i386__ */
WINBASEAPI COORD WINAPI GetLargestConsoleWindowSize(HANDLE);
#endif  /* __i386__ */

WINBASEAPI BOOL   WINAPI AllocConsole(VOID);
WINBASEAPI HANDLE WINAPI CreateConsoleScreenBuffer( DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,LPVOID);
WINBASEAPI BOOL WINAPI   FillConsoleOutputAttribute( HANDLE,WORD,DWORD,COORD,LPDWORD);
WINBASEAPI BOOL WINAPI   FillConsoleOutputCharacterA(HANDLE,CHAR,DWORD,COORD,LPDWORD);
WINBASEAPI BOOL WINAPI   FillConsoleOutputCharacterW(HANDLE,WCHAR,DWORD,COORD,LPDWORD);
#define                  FillConsoleOutputCharacter WINELIB_NAME_AW(FillConsoleOutputCharacter)
WINBASEAPI BOOL WINAPI   FlushConsoleInputBuffer( HANDLE);
WINBASEAPI BOOL WINAPI   FreeConsole(VOID);
WINBASEAPI BOOL WINAPI   GenerateConsoleCtrlEvent( DWORD,DWORD);
WINBASEAPI UINT WINAPI   GetConsoleCP(VOID);
WINBASEAPI BOOL WINAPI   GetConsoleCursorInfo( HANDLE,LPCONSOLE_CURSOR_INFO);
WINBASEAPI BOOL WINAPI   GetConsoleMode( HANDLE,LPDWORD);
WINBASEAPI UINT WINAPI   GetConsoleOutputCP(VOID);
WINBASEAPI BOOL WINAPI   GetConsoleScreenBufferInfo(HANDLE,LPCONSOLE_SCREEN_BUFFER_INFO);
WINBASEAPI DWORD WINAPI  GetConsoleTitleA(LPSTR,DWORD);
WINBASEAPI DWORD WINAPI  GetConsoleTitleW(LPWSTR,DWORD);
#define                  GetConsoleTitle WINELIB_NAME_AW(GetConsoleTitle)
WINBASEAPI HWND WINAPI   GetConsoleWindow(void);
WINBASEAPI BOOL WINAPI   GetNumberOfConsoleInputEvents( HANDLE,LPDWORD);
WINBASEAPI BOOL WINAPI   GetNumberOfConsoleMouseButtons(LPDWORD);
WINBASEAPI BOOL WINAPI   PeekConsoleInputA( HANDLE,PINPUT_RECORD,DWORD,LPDWORD);
WINBASEAPI BOOL WINAPI   PeekConsoleInputW( HANDLE,PINPUT_RECORD,DWORD,LPDWORD);
#define                  PeekConsoleInput WINELIB_NAME_AW(PeekConsoleInput)
WINBASEAPI BOOL WINAPI   ReadConsoleA(HANDLE,LPVOID,DWORD,LPDWORD,LPVOID);
WINBASEAPI BOOL WINAPI   ReadConsoleW(HANDLE,LPVOID,DWORD,LPDWORD,LPVOID);
#define                  ReadConsole WINELIB_NAME_AW(ReadConsole)
WINBASEAPI BOOL WINAPI   ReadConsoleInputA(HANDLE,PINPUT_RECORD,DWORD,LPDWORD);
WINBASEAPI BOOL WINAPI   ReadConsoleInputW(HANDLE,PINPUT_RECORD,DWORD,LPDWORD);
#define                  ReadConsoleInput WINELIB_NAME_AW(ReadConsoleInput)
WINBASEAPI BOOL WINAPI   ReadConsoleOutputA( HANDLE,LPCHAR_INFO,COORD,COORD,LPSMALL_RECT);
WINBASEAPI BOOL WINAPI   ReadConsoleOutputW( HANDLE,LPCHAR_INFO,COORD,COORD,LPSMALL_RECT);
#define                  ReadConsoleOutput WINELIB_NAME_AW(ReadConsoleOutput)
WINBASEAPI BOOL WINAPI   ReadConsoleOutputAttribute( HANDLE,LPWORD,DWORD,COORD,LPDWORD);
WINBASEAPI BOOL WINAPI   ReadConsoleOutputCharacterA(HANDLE,LPSTR,DWORD,COORD,LPDWORD);
WINBASEAPI BOOL WINAPI   ReadConsoleOutputCharacterW(HANDLE,LPWSTR,DWORD,COORD,LPDWORD);
#define                  ReadConsoleOutputCharacter WINELIB_NAME_AW(ReadConsoleOutputCharacter)
WINBASEAPI BOOL WINAPI   ScrollConsoleScreenBufferA( HANDLE,LPSMALL_RECT,LPSMALL_RECT,COORD,LPCHAR_INFO);
WINBASEAPI BOOL WINAPI   ScrollConsoleScreenBufferW( HANDLE,LPSMALL_RECT,LPSMALL_RECT,COORD,LPCHAR_INFO);
#define                  ScrollConsoleScreenBuffer WINELIB_NAME_AW(ScrollConsoleScreenBuffer)
WINBASEAPI BOOL WINAPI   SetConsoleActiveScreenBuffer( HANDLE);
WINBASEAPI BOOL WINAPI   SetConsoleCP(UINT);
WINBASEAPI BOOL WINAPI   SetConsoleCtrlHandler( PHANDLER_ROUTINE,BOOL);
WINBASEAPI BOOL WINAPI   SetConsoleCursorInfo( HANDLE,LPCONSOLE_CURSOR_INFO);
WINBASEAPI BOOL WINAPI   SetConsoleCursorPosition(HANDLE,COORD);
WINBASEAPI BOOL WINAPI   SetConsoleMode( HANDLE,DWORD);
WINBASEAPI BOOL WINAPI   SetConsoleOutputCP(UINT);
WINBASEAPI BOOL WINAPI   SetConsoleScreenBufferSize(HANDLE,COORD);
WINBASEAPI BOOL WINAPI   SetConsoleTextAttribute( HANDLE,WORD);
WINBASEAPI BOOL WINAPI   SetConsoleTitleA(LPCSTR);
WINBASEAPI BOOL WINAPI   SetConsoleTitleW(LPCWSTR);
#define                  SetConsoleTitle WINELIB_NAME_AW(SetConsoleTitle)
WINBASEAPI BOOL WINAPI   SetConsoleWindowInfo( HANDLE,BOOL,LPSMALL_RECT);
WINBASEAPI BOOL WINAPI   WriteConsoleA(HANDLE,CONST VOID *,DWORD,LPDWORD,LPVOID);
WINBASEAPI BOOL WINAPI   WriteConsoleW(HANDLE,CONST VOID *,DWORD,LPDWORD,LPVOID);
#define                  WriteConsole WINELIB_NAME_AW(WriteConsole)
WINBASEAPI BOOL WINAPI   WriteConsoleInputA(HANDLE,const INPUT_RECORD *,DWORD,LPDWORD);
WINBASEAPI BOOL WINAPI   WriteConsoleInputW(HANDLE,const INPUT_RECORD *,DWORD,LPDWORD);
#define                  WriteConsoleInput WINELIB_NAME_AW(WriteConsoleInput)
WINBASEAPI BOOL WINAPI   WriteConsoleOutputA(HANDLE,const CHAR_INFO*,COORD,COORD,LPSMALL_RECT);
WINBASEAPI BOOL WINAPI   WriteConsoleOutputW(HANDLE,const CHAR_INFO*,COORD,COORD,LPSMALL_RECT);
#define                  WriteConsoleOutput WINELIB_NAME_AW(WriteConsoleOutput)
WINBASEAPI BOOL WINAPI   WriteConsoleOutputAttribute(HANDLE,CONST WORD *,DWORD,COORD,LPDWORD);
WINBASEAPI BOOL WINAPI   WriteConsoleOutputCharacterA(HANDLE,LPCSTR,DWORD,COORD,LPDWORD);
WINBASEAPI BOOL WINAPI   WriteConsoleOutputCharacterW(HANDLE,LPCWSTR,DWORD,COORD,LPDWORD);
#define                  WriteConsoleOutputCharacter WINELIB_NAME_AW(WriteConsoleOutputCharacter)

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_WINCON_H */
