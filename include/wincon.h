#ifndef __WINE_WINCON_H
#define __WINE_WINCON_H

#define CTRL_C_EVENT 0
#define CTRL_BREAK_EVENT 1
#define CTRL_CLOSE_EVENT 2
#define CTRL_LOGOFF_EVENT 5
#define CTRL_SHUTDOWN_EVENT 6

/* Console Mode flags */
#define ENABLE_PROCESSED_INPUT	0x01
#define ENABLE_LINE_INPUT	0x02
#define ENABLE_ECHO_INPUT	0x04
#define ENABLE_WINDOW_INPUT	0x08
#define ENABLE_MOUSE_INPUT	0x10

#define ENABLE_PROCESSED_OUTPUT 0x01
#define ENABLE_WRAP_AT_EOL_OUTPUT 0x02


typedef BOOL (WINAPI *PHANDLER_ROUTINE)(DWORD);

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
	WCHAR UniCodeChar;		/* 0E */
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
} INPUT_RECORD,*LPINPUT_RECORD;

/* INPUT_RECORD.wEventType */
#define KEY_EVENT			0x01
#define MOUSE_EVENT			0x02
#define WINDOW_BUFFER_SIZE_EVENT	0x04
#define MENU_EVENT			0x08
#define FOCUS_EVENT 			0x10

#ifdef __i386__
/* Note: this should return a COORD, but calling convention for returning
 * structures is different between Windows and gcc on i386. */
DWORD WINAPI GetLargestConsoleWindowSize(HANDLE);

inline static COORD __wine_GetLargestConsoleWindowSize_wrapper(HANDLE h)
{
    DWORD dw = GetLargestConsoleWindowSize(h);
    return *(COORD *)&dw;
}
#define GetLargestConsoleWindowSize(h) __wine_GetLargestConsoleWindowSize_wrapper(h)

#else  /* __i386__ */
COORD WINAPI GetLargestConsoleWindowSize(HANDLE);
#endif  /* __i386__ */

BOOL        WINAPI ReadConsoleOutputCharacterA(HANDLE,LPSTR,DWORD,COORD,LPDWORD);
BOOL        WINAPI ReadConsoleOutputCharacterW(HANDLE,LPWSTR,DWORD,COORD,LPDWORD);
#define     ReadConsoleOutputCharacter WINELIB_NAME_AW(ReadConsoleOutputCharacter)
BOOL        WINAPI SetConsoleCursorPosition(HANDLE,COORD);

BOOL WINAPI WriteConsoleOutputA( HANDLE hConsoleOutput, LPCHAR_INFO lpBuffer, COORD dwBufferSize, COORD dwBufferCoord, LPSMALL_RECT lpWriteRegion);
BOOL WINAPI WriteConsoleOutputW( HANDLE hConsoleOutput, LPCHAR_INFO lpBuffer, COORD dwBufferSize, COORD dwBufferCoord, LPSMALL_RECT lpWriteRegion);
#define WriteConsoleOutput WINELIB_NAME_AW(WriteConsoleOutput)
BOOL WINAPI WriteConsoleInputA( HANDLE handle, INPUT_RECORD *buffer,
                                    DWORD count, LPDWORD written );
BOOL WINAPI WriteConsoleInputW( HANDLE handle, INPUT_RECORD *buffer,
                                    DWORD count, LPDWORD written );
#define WriteConsoleInput WINELIB_NAME_AW(WriteConsoleInput)
BOOL WINAPI PeekConsoleInputA( HANDLE handle, LPINPUT_RECORD buffer,
                                   DWORD count, LPDWORD read );
BOOL WINAPI PeekConsoleInputW( HANDLE handle, LPINPUT_RECORD buffer,
                                   DWORD count, LPDWORD read );
#define PeekConsoleInput WINELIB_NAME_AW(PeekConsoleInput)
BOOL WINAPI ReadConsoleA(	HANDLE hConsoleInput, LPVOID lpBuffer,
				DWORD nNumberOfCharsToRead,
	  			LPDWORD lpNumberOfCharsRead, LPVOID lpReserved);
BOOL WINAPI ReadConsoleW(	HANDLE hConsoleInput, LPVOID lpBuffer,
				DWORD nNumberOfCharsToRead,
	  			LPDWORD lpNumberOfCharsRead, LPVOID lpReserved);
#define ReadConsole WINELIB_NAME_AW(ReadConsole)
BOOL WINAPI ReadConsoleInputA(HANDLE hConsoleInput,
				  LPINPUT_RECORD lpBuffer, DWORD nLength,
				  LPDWORD lpNumberOfEventsRead);
BOOL WINAPI ReadConsoleInputW(HANDLE hConsoleInput,
				  LPINPUT_RECORD lpBuffer, DWORD nLength,
				  LPDWORD lpNumberOfEventsRead);
#define ReadConsoleInput WINELIB_NAME_AW(ReadConsoleInput)

BOOL WINAPI GetConsoleScreenBufferInfo(HANDLE hConsoleOutput,
					   LPCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo);
BOOL WINAPI SetConsoleScreenBufferSize(HANDLE hConsoleOutput,
					   COORD dwSize);

BOOL WINAPI AllocConsole(VOID);
HANDLE WINAPI CreateConsoleScreenBuffer( DWORD dwDesiredAccess, DWORD dwShareMode,
			    LPSECURITY_ATTRIBUTES sa, DWORD dwFlags, LPVOID lpScreenBufferData);
BOOL WINAPI FillConsoleOutputAttribute( HANDLE hConsoleOutput, WORD wAttribute,
			    DWORD nLength, COORD dwCoord, LPDWORD lpNumAttrsWritten);
BOOL WINAPI FillConsoleOutputCharacterA( HANDLE hConsoleOutput, BYTE cCharacter,
			    DWORD nLength, COORD dwCoord, LPDWORD lpNumCharsWritten);
BOOL WINAPI FillConsoleOutputCharacterW( HANDLE hConsoleOutput, WCHAR cCharacter,
			    DWORD nLength, COORD dwCoord, LPDWORD lpNumCharsWritten);
#define FillConsoleOutputCharacter WINELIB_NAME_AW(FillConsoleOutputCharacter)
BOOL WINAPI FlushConsoleInputBuffer( HANDLE handle);
BOOL WINAPI FreeConsole(VOID);
BOOL WINAPI GenerateConsoleCtrlEvent( DWORD dwCtrlEvent, DWORD dwProcessGroupID);
UINT WINAPI GetConsoleCP(VOID);
BOOL WINAPI GetConsoleCursorInfo( HANDLE hcon, LPCONSOLE_CURSOR_INFO cinfo);
BOOL WINAPI GetConsoleMode( HANDLE hcon,LPDWORD mode);
UINT WINAPI GetConsoleOutputCP(VOID);
DWORD WINAPI GetConsoleTitleA(LPSTR title,DWORD size);
DWORD WINAPI GetConsoleTitleW( LPWSTR title, DWORD size);
#define GetConsoleTitle WINELIB_NAME_AW(GetConsoleTitle)
BOOL WINAPI GetNumberOfConsoleInputEvents( HANDLE hcon,LPDWORD nrofevents);
BOOL WINAPI GetNumberOfConsoleMouseButtons(LPDWORD nrofbuttons);
BOOL WINAPI ReadConsoleOutputAttribute( HANDLE hConsoleOutput, LPWORD lpAttribute,
			    DWORD nLength, COORD dwReadCoord, LPDWORD lpNumberOfAttrsRead);
BOOL WINAPI ScrollConsoleScreenBufferA( HANDLE hConsoleOutput, LPSMALL_RECT lpScrollRect,
			    LPSMALL_RECT lpClipRect, COORD dwDestOrigin, LPCHAR_INFO lpFill);
BOOL WINAPI ScrollConsoleScreenBufferW( HANDLE hConsoleOutput, LPSMALL_RECT lpScrollRect,
			    LPSMALL_RECT lpClipRect, COORD dwDestOrigin, LPCHAR_INFO lpFill);
#define ScrollConsoleScreenBuffer WINELIB_NAME_AW(ScrollConsoleScreenBuffer)
BOOL WINAPI SetConsoleActiveScreenBuffer( HANDLE hConsoleOutput);
BOOL WINAPI SetConsoleCP(UINT cp);
BOOL WINAPI SetConsoleCtrlHandler( PHANDLER_ROUTINE func, BOOL add);
BOOL WINAPI SetConsoleCursorInfo( HANDLE hcon, LPCONSOLE_CURSOR_INFO cinfo);
BOOL WINAPI SetConsoleMode( HANDLE hcon, DWORD mode);
BOOL WINAPI SetConsoleOutputCP(UINT cp);
BOOL WINAPI SetConsoleTextAttribute( HANDLE hConsoleOutput,WORD wAttr);
BOOL WINAPI SetConsoleTitleA(LPCSTR title);
BOOL WINAPI SetConsoleTitleW(LPCWSTR title);
#define SetConsoleTitle WINELIB_NAME_AW(SetConsoleTitle)
BOOL WINAPI SetConsoleWindowInfo( HANDLE hcon, BOOL bAbsolute, LPSMALL_RECT window);

BOOL WINAPI WriteConsoleOutputAttribute( HANDLE hConsoleOutput, CONST WORD *lpAttribute,
			    DWORD nLength, COORD dwCoord, LPDWORD lpNumAttrsWritten);

BOOL WINAPI WriteConsoleOutputCharacterA( HANDLE hConsoleOutput, LPCSTR lpCharacter,
			    DWORD nLength, COORD dwCoord, LPDWORD lpNumCharsWritten);
BOOL WINAPI WriteConsoleOutputCharacterW( HANDLE hConsoleOutput, LPCWSTR lpCharacter,
			    DWORD nLength, COORD dwCoord, LPDWORD lpNumCharsWritten);
#define WriteConsoleOutputCharacter WINELIB_NAME_AW(WriteConsoleOutputCharacter)

#endif  /* __WINE_WINCON_H */
