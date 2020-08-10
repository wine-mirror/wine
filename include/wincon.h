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

#ifndef NOGDI
#include <wingdi.h>
#endif

#include <consoleapi.h>

/* AttachConsole special pid value */
#define ATTACH_PARENT_PROCESS ((DWORD) -1)

/* GetConsoleDisplayMode flags */
#define CONSOLE_FULLSCREEN          1
#define CONSOLE_FULLSCREEN_HARDWARE 2

/* SetConsoleDisplayMode flags */
#define CONSOLE_FULLSCREEN_MODE 1
#define CONSOLE_WINDOWED_MODE   2

/* CONSOLE_SELECTION_INFO.dwFlags */
#define CONSOLE_NO_SELECTION          0x00
#define CONSOLE_SELECTION_IN_PROGRESS 0x01
#define CONSOLE_SELECTION_NOT_EMPTY   0x02
#define CONSOLE_MOUSE_SELECTION       0x04
#define CONSOLE_MOUSE_DOWN            0x08


/* Attributes flags: */

#define FOREGROUND_BLUE            0x0001
#define FOREGROUND_GREEN           0x0002
#define FOREGROUND_RED             0x0004
#define FOREGROUND_INTENSITY       0x0008
#define BACKGROUND_BLUE            0x0010
#define BACKGROUND_GREEN           0x0020
#define BACKGROUND_RED             0x0040
#define BACKGROUND_INTENSITY       0x0080
#define COMMON_LVB_LEADING_BYTE    0x0100
#define COMMON_LVB_TRAILING_BYTE   0x0200
#define COMMON_LVB_GRID_HORIZONTAL 0x0400
#define COMMON_LVB_GRID_LVERTICAL  0x0800
#define COMMON_LVB_GRID_RVERTICAL  0x1000
#define COMMON_LVB_REVERSE_VIDEO   0x4000
#define COMMON_LVB_UNDERSCORE      0x8000

/* CONSOLE_HISTORY_INFO.dwFlags */
#define HISTORY_NO_DUP_FLAG  0x01

typedef struct _CONSOLE_CURSOR_INFO {
    DWORD	dwSize;   /* Between 1 & 100 for percentage of cell filled */
    BOOL	bVisible; /* Visibility of cursor */
} CONSOLE_CURSOR_INFO, *LPCONSOLE_CURSOR_INFO;

#ifndef NOGDI
typedef struct _CONSOLE_FONT_INFOEX
{
    ULONG       cbSize;
    DWORD       nFont;
    COORD       dwFontSize;
    UINT        FontFamily;
    UINT        FontWeight;
    WCHAR       FaceName[LF_FACESIZE];
} CONSOLE_FONT_INFOEX,*LPCONSOLE_FONT_INFOEX;

WINBASEAPI BOOL WINAPI GetCurrentConsoleFontEx(HANDLE,BOOL,LPCONSOLE_FONT_INFOEX);
WINBASEAPI BOOL WINAPI SetCurrentConsoleFontEx(HANDLE,BOOL,LPCONSOLE_FONT_INFOEX);
#endif

typedef struct tagCONSOLE_HISTORY_INFO
{
    UINT        cbSize;
    UINT        HistoryBufferSize;
    UINT        NumberOfHistoryBuffers;
    DWORD       dwFlags;
} CONSOLE_HISTORY_INFO,*LPCONSOLE_HISTORY_INFO;

typedef struct tagCONSOLE_SCREEN_BUFFER_INFO
{
    COORD       dwSize;
    COORD       dwCursorPosition;
    WORD        wAttributes;
    SMALL_RECT  srWindow;
    COORD       dwMaximumWindowSize;
} CONSOLE_SCREEN_BUFFER_INFO,*LPCONSOLE_SCREEN_BUFFER_INFO;

typedef struct _CONSOLE_SCREEN_BUFFER_INFOEX
{
    ULONG       cbSize;
    COORD       dwSize;
    COORD       dwCursorPosition;
    WORD        wAttributes;
    SMALL_RECT  srWindow;
    COORD       dwMaximumWindowSize;
    WORD        wPopupAttributes;
    BOOL        bFullscreenSupported;
    COLORREF    ColorTable[16];
} CONSOLE_SCREEN_BUFFER_INFOEX,*LPCONSOLE_SCREEN_BUFFER_INFOEX;

typedef struct _CONSOLE_SELECTION_INFO
{
    DWORD       dwFlags;
    COORD       dwSelectionAnchor;
    SMALL_RECT  srSelection;
} CONSOLE_SELECTION_INFO,*LPCONSOLE_SELECTION_INFO;


#define CONSOLE_TEXTMODE_BUFFER  1

#if defined(__i386__) && !defined(__MINGW32__) && !defined(_MSC_VER)
/* Note: this should return a COORD, but calling convention for returning
 * structures is different between Windows and gcc on i386. */

WINBASEAPI DWORD WINAPI GetConsoleFontSize(HANDLE, DWORD);
WINBASEAPI DWORD WINAPI GetLargestConsoleWindowSize(HANDLE);

static inline COORD __wine_GetConsoleFontSize_wrapper(HANDLE h, DWORD d)
{
    union {
      COORD c;
      DWORD dw;
    } u;
    u.dw = GetConsoleFontSize(h, d);
    return u.c;
}
#define GetConsoleFontSize(h, d) __wine_GetConsoleFontSize_wrapper(h, d)

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
WINBASEAPI COORD WINAPI GetConsoleFontSize(HANDLE, DWORD);
WINBASEAPI COORD WINAPI GetLargestConsoleWindowSize(HANDLE);
#endif  /* __i386__ */

WINBASEAPI BOOL   WINAPI AddConsoleAliasA(LPSTR,LPSTR,LPSTR);
WINBASEAPI BOOL   WINAPI AddConsoleAliasW(LPWSTR,LPWSTR,LPWSTR);
#define                  AddConsoleAlias WINELIB_NAME_AW(AddConsoleAlias)
WINBASEAPI BOOL   WINAPI CloseConsoleHandle(HANDLE);
WINBASEAPI HANDLE WINAPI CreateConsoleScreenBuffer(DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,LPVOID);
WINBASEAPI HANDLE WINAPI DuplicateConsoleHandle(HANDLE,DWORD,BOOL,DWORD);
WINBASEAPI BOOL WINAPI   FillConsoleOutputAttribute( HANDLE,WORD,DWORD,COORD,LPDWORD);
WINBASEAPI BOOL WINAPI   FillConsoleOutputCharacterA(HANDLE,CHAR,DWORD,COORD,LPDWORD);
WINBASEAPI BOOL WINAPI   FillConsoleOutputCharacterW(HANDLE,WCHAR,DWORD,COORD,LPDWORD);
#define                  FillConsoleOutputCharacter WINELIB_NAME_AW(FillConsoleOutputCharacter)
WINBASEAPI BOOL WINAPI   FlushConsoleInputBuffer( HANDLE);
WINBASEAPI BOOL WINAPI   GenerateConsoleCtrlEvent( DWORD,DWORD);
WINBASEAPI DWORD WINAPI  GetConsoleAliasA(LPSTR,LPSTR,DWORD,LPSTR);
WINBASEAPI DWORD WINAPI  GetConsoleAliasW(LPWSTR,LPWSTR,DWORD,LPWSTR);
#define                  GetConsoleAlias WINELIB_NAME_AW(GetConsoleAlias)
WINBASEAPI DWORD WINAPI  GetConsoleAliasesA(LPSTR,DWORD,LPSTR);
WINBASEAPI DWORD WINAPI  GetConsoleAliasesW(LPWSTR,DWORD,LPWSTR);
#define                  GetConsoleAliases WINELIB_NAME_AW(GetConsoleAliases)
WINBASEAPI DWORD WINAPI  GetConsoleAliasesLengthA(LPSTR);
WINBASEAPI DWORD WINAPI  GetConsoleAliasesLengthW(LPWSTR);
#define                  GetConsoleAliasesLength WINELIB_NAME_AW(GetConsoleAliasesLength)
WINBASEAPI DWORD WINAPI  GetConsoleAliasExesA(LPSTR,DWORD);
WINBASEAPI DWORD WINAPI  GetConsoleAliasExesW(LPWSTR,DWORD);
#define                  GetConsoleAliasExes WINELIB_NAME_AW(GetConsoleAliasExes)
WINBASEAPI DWORD WINAPI  GetConsoleAliasExesLengthA(VOID);
WINBASEAPI DWORD WINAPI  GetConsoleAliasExesLengthW(VOID);
#define                  GetConsoleAliasExesLength WINELIB_NAME_AW(GetConsoleAliasExesLength)
WINBASEAPI BOOL WINAPI   GetConsoleCursorInfo( HANDLE,LPCONSOLE_CURSOR_INFO);
WINBASEAPI BOOL WINAPI   GetConsoleDisplayMode(LPDWORD);
WINBASEAPI BOOL WINAPI   GetConsoleHistoryInfo(LPCONSOLE_HISTORY_INFO);
WINBASEAPI BOOL WINAPI   GetConsoleInputExeNameA(DWORD,LPSTR);
WINBASEAPI BOOL WINAPI   GetConsoleInputExeNameW(DWORD,LPWSTR);
#define                  GetConsoleInputExeName WINELIB_NAME_AW(GetConsoleInputExeName)
WINBASEAPI HANDLE WINAPI GetConsoleInputWaitHandle(void);
WINBASEAPI DWORD WINAPI  GetConsoleOriginalTitleA(LPSTR,DWORD);
WINBASEAPI DWORD WINAPI  GetConsoleOriginalTitleW(LPWSTR,DWORD);
#define                  GetConsoleOriginalTitle WINELIB_NAME_AW(GetConsoleOriginalTitle)
WINBASEAPI DWORD WINAPI  GetConsoleProcessList(LPDWORD,DWORD);
WINBASEAPI BOOL WINAPI   GetConsoleScreenBufferInfo(HANDLE,LPCONSOLE_SCREEN_BUFFER_INFO);
WINBASEAPI BOOL WINAPI   GetConsoleScreenBufferInfoEx(HANDLE,LPCONSOLE_SCREEN_BUFFER_INFOEX);
WINBASEAPI DWORD WINAPI  GetConsoleTitleA(LPSTR,DWORD);
WINBASEAPI DWORD WINAPI  GetConsoleTitleW(LPWSTR,DWORD);
#define                  GetConsoleTitle WINELIB_NAME_AW(GetConsoleTitle)
WINBASEAPI HWND WINAPI   GetConsoleWindow(void);
WINBASEAPI BOOL WINAPI   GetCurrentConsoleFont(HANDLE,BOOL,LPCONSOLE_FONT_INFO);
WINBASEAPI BOOL WINAPI   GetNumberOfConsoleMouseButtons(LPDWORD);
WINBASEAPI HANDLE WINAPI OpenConsoleA(LPCSTR,DWORD,BOOL,DWORD);
WINBASEAPI HANDLE WINAPI OpenConsoleW(LPCWSTR,DWORD,BOOL,DWORD);
#define                  OpenConsole WINELIB_NAME_AW(OpenConsole)
WINBASEAPI BOOL WINAPI   ReadConsoleOutputA( HANDLE,LPCHAR_INFO,COORD,COORD,LPSMALL_RECT);
WINBASEAPI BOOL WINAPI   ReadConsoleOutputW( HANDLE,LPCHAR_INFO,COORD,COORD,LPSMALL_RECT);
#define                  ReadConsoleOutput WINELIB_NAME_AW(ReadConsoleOutput)
WINBASEAPI BOOL WINAPI   ReadConsoleOutputAttribute( HANDLE,LPWORD,DWORD,COORD,LPDWORD);
WINBASEAPI BOOL WINAPI   ReadConsoleOutputCharacterA(HANDLE,LPSTR,DWORD,COORD,LPDWORD);
WINBASEAPI BOOL WINAPI   ReadConsoleOutputCharacterW(HANDLE,LPWSTR,DWORD,COORD,LPDWORD);
#define                  ReadConsoleOutputCharacter WINELIB_NAME_AW(ReadConsoleOutputCharacter)
WINBASEAPI BOOL WINAPI   ScrollConsoleScreenBufferA( HANDLE,const SMALL_RECT *,const SMALL_RECT *,COORD,const CHAR_INFO *);
WINBASEAPI BOOL WINAPI   ScrollConsoleScreenBufferW( HANDLE,const SMALL_RECT *,const SMALL_RECT *com,COORD,const CHAR_INFO *);
#define                  ScrollConsoleScreenBuffer WINELIB_NAME_AW(ScrollConsoleScreenBuffer)
WINBASEAPI BOOL WINAPI   SetConsoleActiveScreenBuffer( HANDLE);
WINBASEAPI BOOL WINAPI   SetConsoleCP(UINT);
WINBASEAPI BOOL WINAPI   SetConsoleCursorInfo( HANDLE,LPCONSOLE_CURSOR_INFO);
WINBASEAPI BOOL WINAPI   SetConsoleCursorPosition(HANDLE,COORD);
WINBASEAPI BOOL WINAPI   SetConsoleDisplayMode(HANDLE,DWORD,LPCOORD);
WINBASEAPI BOOL WINAPI   SetConsoleHistoryInfo(LPCONSOLE_HISTORY_INFO);
WINBASEAPI BOOL WINAPI   SetConsoleOutputCP(UINT);
WINBASEAPI BOOL WINAPI   SetConsoleScreenBufferInfoEx(HANDLE,LPCONSOLE_SCREEN_BUFFER_INFOEX);
WINBASEAPI BOOL WINAPI   SetConsoleScreenBufferSize(HANDLE,COORD);
WINBASEAPI BOOL WINAPI   SetConsoleTextAttribute( HANDLE,WORD);
WINBASEAPI BOOL WINAPI   SetConsoleTitleA(LPCSTR);
WINBASEAPI BOOL WINAPI   SetConsoleTitleW(LPCWSTR);
#define                  SetConsoleTitle WINELIB_NAME_AW(SetConsoleTitle)
WINBASEAPI BOOL WINAPI   SetConsoleWindowInfo( HANDLE,BOOL,LPSMALL_RECT);
WINBASEAPI BOOL WINAPI   VerifyConsoleIoHandle(HANDLE);
WINBASEAPI BOOL WINAPI   WriteConsoleInputA(HANDLE,const INPUT_RECORD *,DWORD,LPDWORD);
WINBASEAPI BOOL WINAPI   WriteConsoleInputW(HANDLE,const INPUT_RECORD *,DWORD,LPDWORD);
#define                  WriteConsoleInput WINELIB_NAME_AW(WriteConsoleInput)
WINBASEAPI BOOL WINAPI   WriteConsoleOutputA(HANDLE,const CHAR_INFO*,COORD,COORD,LPSMALL_RECT);
WINBASEAPI BOOL WINAPI   WriteConsoleOutputW(HANDLE,const CHAR_INFO*,COORD,COORD,LPSMALL_RECT);
#define                  WriteConsoleOutput WINELIB_NAME_AW(WriteConsoleOutput)
WINBASEAPI BOOL WINAPI   WriteConsoleOutputAttribute(HANDLE,const WORD *,DWORD,COORD,LPDWORD);
WINBASEAPI BOOL WINAPI   WriteConsoleOutputCharacterA(HANDLE,LPCSTR,DWORD,COORD,LPDWORD);
WINBASEAPI BOOL WINAPI   WriteConsoleOutputCharacterW(HANDLE,LPCWSTR,DWORD,COORD,LPDWORD);
#define                  WriteConsoleOutputCharacter WINELIB_NAME_AW(WriteConsoleOutputCharacter)

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_WINCON_H */
