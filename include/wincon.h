#ifndef _WINECON_H_
#define _WINECON_H_

#define CTRL_C_EVENT 0
#define CTRL_BREAK_EVENT 1
#define CTRL_CLOSE_EVENT 2
#define CTRL_LOGOFF_EVENT 5
#define CTRL_SHUTDOWN_EVENT 6

typedef BOOL32 HANDLER_ROUTINE(WORD);

/*
 * Attributes flags:
 */

#define FOREGROUND_BLUE      0x0001 /* text color contains blue. */
#define FOREGROUND_GREEN     0x0002 /* text color contains green. */
#define FOREGROUND_RED       0x0004 /* text color contains red. */
#define FOREGROUND_INTENSITY 0x0008 /* text color is intensified. */
#define BACKGROUND_BLUE      0x0010 /* background color contains blue. */
#define BACKGROUND_GREEN     0x0020 /* background color contains green. */
#define BACKGROUND_RED       0x0040 /* background color contains red. */
#define BACKGROUND_INTENSITY 0x0080 /* background color is intensified. */

typedef struct tagCOORD
{
    INT16 x;
    INT16 y;
} COORD,*LPCOORD;

typedef struct tagSMALL_RECT
{
    INT16 Left;
    INT16 Right;
    INT16 Top;
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


#endif

#if 0
#ifndef _WINCON_H_
#define _WINCON_H_

#ifdef UNICODE
#define FillConsoleOutputCharacter FillConsoleOutputCharacterW
#define GetConsoleTitle GetConsoleTitleW
#define PeekConsoleInput PeekConsoleInputW
#define ReadConsole ReadConsoleW
#define ReadConsoleInput ReadConsoleInputW
#define ReadConsoleOutput ReadConsoleOutputW
#define ReadConsoleOutputCharacter ReadConsoleOutputCharacterW
#define ScrollConsoleScreenBuffer ScrollConsoleScreenBufferW
#define SetConsoleTitle SetConsoleTitleW
#define WriteConsole WriteConsoleW
#define WriteConsoleInput WriteConsoleInputW
#define WriteConsoleOutput WriteConsoleOutputW
#define WriteConsoleOutputCharacter WriteConsoleOutputCharacterW
#else
#define FillConsoleOutputCharacter FillConsoleOutputCharacterA
#define GetConsoleTitle GetConsoleTitleA
#define PeekConsoleInput PeekConsoleInputA
#define ReadConsole ReadConsoleA
#define ReadConsoleInput ReadConsoleInputA
#define ReadConsoleOutput ReadConsoleOutputA
#define ReadConsoleOutputCharacter ReadConsoleOutputCharacterA
#define ScrollConsoleScreenBuffer ScrollConsoleScreenBufferA
#define SetConsoleTitle SetConsoleTitleA
#define WriteConsole WriteConsoleA
#define WriteConsoleInput WriteConsoleInputA
#define WriteConsoleOutput WriteConsoleOutputA
#define WriteConsoleOutputCharacter WriteConsoleOutputCharacterA
#endif



#if 0

typedef struct
  {
    int bKeyDown;
    WORD wRepeatCount;
    WORD wVirtualKeyCode;
    WORD wVirtualScanCode;

    char AsciiChar;
char pad;
#if 0
    union
      {
	WCHAR UnicodeChar;
	CHAR AsciiChar;
      }
    uChar;
#endif
    DWORD dwControlKeyState;
  } __attribute__ ((packed)) KEY_EVENT_RECORD;



#define RIGHT_ALT_PRESSED 0x1
#define LEFT_ALT_PRESSED 0x2
#define RIGHT_CTRL_PRESSED 0x4
#define LEFT_CTRL_PRESSED 0x8
#define SHIFT_PRESSED 0x10
#define NUMLOCK_ON 0x20
#define SCROLLLOCK_ON 0x40
#define CAPSLOCK_ON 0x80
#define ENHANCED_KEY 0x100

typedef struct
  {
    COORD dwMousePosition;
    DWORD dwButtonState;
    DWORD dwControlKeyState;
    DWORD dwEventFlags;
  }
MOUSE_EVENT_RECORD;

#define CONSOLE_TEXTMODE_BUFFER 1


#define FROM_LEFT_1ST_BUTTON_PRESSED 0x0001
#define RIGHTMOST_BUTTON_PRESSED 0x0002
#define FROM_LEFT_2ND_BUTTON_PRESSED 0x0004
#define FROM_LEFT_3RD_BUTTON_PRESSED 0x0008
#define FROM_LEFT_4TH_BUTTON_PRESSED 0x0010




#define MOUSE_MOVED 0x0001
#define DOUBLE_CLICK 0x0002

typedef struct
  {
    COORD size;
  }
WINDOW_BUFFER_SIZE_RECORD;

typedef struct
  {
    UINT dwCommandId;
  }
MENU_EVENT_RECORD;

typedef struct
  {
    BOOL bSetFocus;
  }
FOCUS_EVENT_RECORD;

typedef struct
  {
    WORD EventType;
    union
      {
	KEY_EVENT_RECORD KeyEvent;
	MOUSE_EVENT_RECORD MouseEvent;
	WINDOW_BUFFER_SIZE_RECORD WindowBufferSizeEvent;
	MENU_EVENT_RECORD MenuEvent;
	FOCUS_EVENT_RECORD FocusEvent;
      }
    Event;
  }
INPUT_RECORD;

#define KEY_EVENT 0x1
#define MOUSE_EVENT 0x2
#define WINDOW_BUFFER_SIZE_EVENT 0x4
#define MENU_EVENT 0x8
#define FOCUS_EVENT 0x10

typedef struct
  {
    union
      {
	WCHAR UnicodeChar;
	CHAR AsciiChar;
      }
    Char;
    WORD Attributes;
  }
CHAR_INFO;

typedef struct
  {
    DWORD size;
    BOOL bVisible;
  }
CONSOLE_CURSOR_INFO;

#endif


#if 0

#define ENABLE_PROCESSED_INPUT 0x01
#define ENABLE_LINE_INPUT 0x02
#define ENABLE_ECHO_INPUT 0x04
#define ENABLE_WINDOW_INPUT 0x08
#define ENABLE_MOUSE_INPUT 0x10

#define ENABLE_PROCESSED_OUTPUT 0x01
#define ENABLE_WRAP_AT_EOL_OUTPUT 0x02


BOOL AllocConsole (VOID);


HANDLE CreateConsoleScreenBuffer (DWORD access, DWORD mode,
                                  CONST SECURITY_ATTRIBUTES * lattr,
                                  DWORD flags, VOID * ptr);
BOOL FillConsoleOutputAttribute (HANDLE h, WORD attr, DWORD len,
                                 COORD co, DWORD * done);
BOOL FillConsoleOutputCharacterA (HANDLE h, CHAR c, DWORD len,
                                  COORD co, DWORD * done);
BOOL FlushBuffer (HANDLE h);
BOOL FreeConsole (VOID);
BOOL GenerateConsoleCtrlEvent (DWORD  ev,    DWORD group);
UINT GetConsoleCP (VOID);
BOOL GetConsoleCursorInfo (HANDLE h, CONSOLE_CURSOR_INFO *info);
BOOL GetConsoleMode (HANDLE h, DWORD * mode);
UINT GetConsoleOutputCP (VOID);
BOOL GetConsoleScreenBufferInfo (HANDLE h, CONSOLE_SCREEN_BUFFER_INFO * ptr);
DWORD GetConsoleTitleA (LPSTR str, DWORD len);
COORD GetLargestConsoleWindowSize (HANDLE h);
BOOL GetNumberOfConsoleInputEvents (HANDLE h, DWORD * n);
BOOL GetNumberOfConsoleMouseButtons (DWORD * n);
BOOL PeekConsoleInputA (HANDLE h, INPUT_RECORD * ptr, DWORD len, DWORD * done);
BOOL ReadConsoleA (HANDLE h, VOID * ptr, DWORD len, DWORD * done, VOID * res);
BOOL ReadConsoleInputA (HANDLE h, INPUT_RECORD * ptr, DWORD len, DWORD * done);
BOOL ReadConsoleOutputA (HANDLE h, CHAR_INFO * ptr, COORD size,
                         COORD fred, SMALL_RECT * reg);
BOOL ReadConsoleOutputAttribute (HANDLE h, WORD * attr, DWORD len,
                                 COORD rc, DWORD * done);
BOOL ReadConsoleOutputCharacterA (HANDLE h, LPSTR c, DWORD len,
                                  COORD rc, DWORD * done);
BOOL ScrollConsoleScreenBufferA (HANDLE h, CONST SMALL_RECT * sr,
                                 CONST SMALL_RECT * cr, COORD cpos,
                                 CONST CHAR_INFO * i);
BOOL SetConsoleActiveScreenBuffer (HANDLE h);
BOOL SetConsoleCP (UINT i);
BOOL SetConsoleCtrlHandler (HANDLER_ROUTINE * func,  BOOL a);
BOOL SetConsoleCursorInfo (HANDLE h,  CONST CONSOLE_CURSOR_INFO * info);
BOOL SetConsoleCursorPosition (HANDLE h, COORD pos);
BOOL SetConsoleMode (HANDLE h, DWORD mode);
BOOL SetConsoleOutputCP (UINT i);
BOOL SetConsoleScreenBufferSize (HANDLE h, COORD size);
BOOL SetConsoleTextAttribute (HANDLE h, WORD attrs);
BOOL SetConsoleTitleA (const char * str);
BOOL SetConsoleWindowInfo (HANDLE h, BOOL abs, CONST SMALL_RECT * wnd);
BOOL WriteConsoleA (HANDLE h, CONST VOID *   ptr, DWORD slen,
                    DWORD * done, VOID * res);
BOOL WriteConsoleInputA (HANDLE	h, CONST INPUT_RECORD * ptr,
                         DWORD len, DWORD * done);
BOOL WriteConsoleOutputA (HANDLE  h, CONST CHAR_INFO * ptr,
                          COORD size, COORD fred, 
                          SMALL_RECT* where);
BOOL WriteConsoleOutputAttribute (HANDLE h, CONST WORD *attr, DWORD len,
                                  COORD co, DWORD * done);
BOOL WriteConsoleOutputCharacterA (HANDLE h, const char * c, DWORD len,
                                   COORD co, DWORD * done);
#endif
#endif

#endif /* 0 */
