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




#define FOREGROUND_BLUE 0x01
#define FOREGROUND_GREEN 0x02
#define FOREGROUND_RED 0x04
#define FOREGROUND_INTENSITY 0x08
#define BACKGROUND_BLUE 0x10
#define BACKGROUND_GREEN 0x20
#define BACKGROUND_RED 0x40
#define BACKGROUND_INTENSITY 0x80


typedef struct
  {
    COORD dwSize;
    COORD dwCursorPosition;
    WORD wAttrs;
    SMALL_RECT srWindow;
    COORD dwMaximumWindowSize;
  }
CONSOLE_SCREEN_BUFFER_INFO;

typedef struct
  {
    DWORD size;
    BOOL bVisible;
  }
CONSOLE_CURSOR_INFO;

#endif


#define CTRL_C_EVENT 0
#define CTRL_BREAK_EVENT 1
#define CTRL_CLOSE_EVENT 2
#define CTRL_LOGOFF_EVENT 5
#define CTRL_SHUTDOWN_EVENT 6

typedef BOOL HANDLER_ROUTINE (WORD ctrltype);

#if 0

#define ENABLE_PROCESSED_INPUT 0x01
#define ENABLE_LINE_INPUT 0x02
#define ENABLE_ECHO_INPUT 0x04
#define ENABLE_WINDOW_INPUT 0x08
#define ENABLE_MOUSE_INPUT 0x10

#define ENABLE_PROCESSED_OUTPUT 0x01
#define ENABLE_WRAP_AT_EOL_OUTPUT 0x02


BOOL WINAPI AllocConsole (VOID);


HANDLE WINAPI CreateConsoleScreenBuffer (DWORD access,
					 DWORD mode,
					 CONST SECURITY_ATTRIBUTES * lattr,
					 DWORD flags,
					 VOID * ptr);

BOOL WINAPI FillConsoleOutputAttribute (HANDLE h,
					WORD attr,
					DWORD len,
					COORD co,
					DWORD * done);

BOOL WINAPI FillConsoleOutputCharacterA (HANDLE h,
					 CHAR c,
					 DWORD len,
					 COORD co,
					 DWORD * done);


BOOL WINAPI FlushBuffer (HANDLE h);

BOOL WINAPI FreeConsole (VOID);
BOOL WINAPI GenerateConsoleCtrlEvent (DWORD  ev,    DWORD group);
UINT WINAPI GetConsoleCP (VOID);
BOOL WINAPI GetConsoleCursorInfo (HANDLE h, CONSOLE_CURSOR_INFO *info);
BOOL WINAPI GetConsoleMode (HANDLE h, DWORD * mode);
UINT WINAPI GetConsoleOutputCP (VOID);
BOOL WINAPI GetConsoleScreenBufferInfo (HANDLE h, CONSOLE_SCREEN_BUFFER_INFO *
					ptr);

DWORD WINAPI GetConsoleTitleA (LPSTR str, DWORD len);


COORD WINAPI GetLargestConsoleWindowSize (HANDLE h);

BOOL WINAPI GetNumberOfConsoleInputEvents (HANDLE h,
					   DWORD * n);

BOOL WINAPI GetNumberOfConsoleMouseButtons (DWORD * n);

BOOL WINAPI PeekConsoleInputA (HANDLE h,
			       INPUT_RECORD * ptr,
			       DWORD len,
			       DWORD * done);



BOOL WINAPI ReadConsoleA (HANDLE h,
			  VOID * ptr,
			  DWORD len,
			  DWORD * done,
			  VOID * res);

BOOL WINAPI ReadConsoleInputA (HANDLE h,
			       INPUT_RECORD * ptr,
			       DWORD len,
			       DWORD * done);

BOOL WINAPI ReadConsoleOutputA (HANDLE h,
				CHAR_INFO * ptr,
				COORD size,
				COORD fred,
				SMALL_RECT * reg);

BOOL WINAPI ReadConsoleOutputAttribute (HANDLE h,
					WORD * attr,
					DWORD len,
					COORD rc,
					DWORD * done);

BOOL WINAPI ReadConsoleOutputCharacterA (HANDLE h,
					 LPSTR c,
					 DWORD len,
					 COORD rc,
					 DWORD * done);

BOOL WINAPI ScrollConsoleScreenBufferA (HANDLE h,
					CONST SMALL_RECT * sr,
					CONST SMALL_RECT * cr,
					COORD cpos,
					CONST CHAR_INFO * i);


BOOL WINAPI SetConsoleActiveScreenBuffer (HANDLE h);
BOOL WINAPI SetConsoleCP (UINT i);
BOOL WINAPI SetConsoleCtrlHandler (HANDLER_ROUTINE * func,  BOOL a);

BOOL WINAPI SetConsoleCursorInfo (HANDLE h,  CONST CONSOLE_CURSOR_INFO * info);

BOOL WINAPI SetConsoleCursorPosition (HANDLE h, COORD pos);

BOOL WINAPI SetConsoleMode (HANDLE h, DWORD mode);

BOOL WINAPI SetConsoleOutputCP (UINT i);
BOOL WINAPI SetConsoleScreenBufferSize (HANDLE h, COORD size);
BOOL WINAPI SetConsoleTextAttribute (HANDLE h,
				     WORD attrs);
BOOL WINAPI SetConsoleTitleA (const char * str);

BOOL WINAPI SetConsoleWindowInfo (HANDLE h,
				  BOOL abs, 
				  CONST SMALL_RECT * wnd);

BOOL WINAPI WriteConsoleA (HANDLE h, 
			   CONST VOID *   ptr,
			   DWORD slen,
			   DWORD * done,
			   VOID * res);

BOOL WINAPI WriteConsoleInputA (HANDLE	h, 
				CONST INPUT_RECORD * ptr,
				DWORD len, 
				DWORD * done);

BOOL WINAPI WriteConsoleOutputA (HANDLE  h,
				 CONST CHAR_INFO * ptr,
				 COORD size, 
				 COORD fred, 
				 SMALL_RECT* where);

BOOL WINAPI WriteConsoleOutputAttribute (HANDLE h,
					 CONST WORD *attr,
					 DWORD len,
					 COORD co,
					 DWORD * done);

BOOL WINAPI WriteConsoleOutputCharacterA (HANDLE h,
					  const char * c,
					  DWORD len,
					  COORD co,
					  DWORD * done);
#endif
#endif

