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


typedef BOOL32 HANDLER_ROUTINE(WORD);

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
    BOOL32	bVisible; /* Visibility of cursor */
} CONSOLE_CURSOR_INFO, *LPCONSOLE_CURSOR_INFO;

/*
typedef struct tagCOORD
{
    INT16 x;
    INT16 y;
} COORD,*LPCOORD;
*/

typedef struct tagSMALL_RECT
{
    INT16 Left;
    INT16 Top;
    INT16 Right;
    INT16 Bottom;
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
    BOOL32	bKeyDown;		/* 04 */
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
    UINT32	dwCommandId; /* perhaps UINT16 ??? */
} MENU_EVENT_RECORD,*LPMENU_EVENT_RECORD;

typedef struct tagFOCUS_EVENT_RECORD
{
    BOOL32      bSetFocus; /* perhaps BOOL16 ??? */
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

BOOL32 WINAPI WriteConsoleOutput32A( HANDLE32 hConsoleOutput, LPCHAR_INFO lpBuffer, COORD dwBufferSize, COORD dwBufferCoord, LPSMALL_RECT lpWriteRegion);
BOOL32 WINAPI WriteConsoleOutput32W( HANDLE32 hConsoleOutput, LPCHAR_INFO lpBuffer, COORD dwBufferSize, COORD dwBufferCoord, LPSMALL_RECT lpWriteRegion);
#define WriteConsoleOutput WINELIB_NAME_AW(WriteConsoleOutput)

#ifdef __WINE__
extern HANDLE32 CONSOLE_OpenHandle( BOOL32 output, DWORD access, LPSECURITY_ATTRIBUTES sa );
#endif

#endif  /* __WINE_WINCON_H */
