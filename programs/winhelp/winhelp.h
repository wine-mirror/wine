/*
 * Help Viewer
 *
 * Copyright 1996 Ulrich Schmid
 */

#define MAX_LANGUAGE_NUMBER 255
#define MAX_PATHNAME_LEN   1024
#define MAX_STRING_LEN      255

#define INTERNAL_BORDER_WIDTH  5
#define POPUP_YDISTANCE       20
#define SHADOW_DX     20
#define SHADOW_DY     20
#define BUTTON_CX      6
#define BUTTON_CY      6

#ifndef RC_INVOKED

#include "hlpfile.h"
#include "macro.h"

typedef struct tagHelpLinePart
{
  RECT      rect;
  LPCSTR    lpsText;
  UINT      wTextLen;
  HFONT     hFont;
  COLORREF  color;

  struct
  {
  LPCSTR    lpszPath;
  LONG      lHash;
  BOOL      bPopup;
  }         link;

  HGLOBAL   hSelf;
  struct tagHelpLinePart *next;
} WINHELP_LINE_PART;

typedef struct tagHelpLine
{
  RECT              rect;
  WINHELP_LINE_PART first_part;
  struct tagHelpLine *next;
} WINHELP_LINE;

typedef struct tagHelpButton
{
  HWND hWnd;

  LPCSTR lpszID;
  LPCSTR lpszName;
  LPCSTR lpszMacro;

  WPARAM wParam;

  RECT rect;

  HGLOBAL hSelf;
  struct tagHelpButton *next;
} WINHELP_BUTTON;

typedef struct tagWinHelp
{
  LPCSTR lpszName;

  WINHELP_BUTTON *first_button;
  HLPFILE_PAGE   *page;
  WINHELP_LINE   *first_line;

  HWND hMainWnd;
  HWND hButtonBoxWnd;
  HWND hTextWnd;
  HWND hShadowWnd;

  HFONT (*fonts)[2];
  UINT  fonts_len;

  HGLOBAL hSelf;
  struct tagWinHelp *next;
} WINHELP_WINDOW;

typedef struct
{
  UINT   wVersion;
  HANDLE hInstance;
  HWND   hPopupWnd;
  UINT   wStringTableOffset;
  WINHELP_WINDOW *active_win;
  WINHELP_WINDOW *win_list;
} WINHELP_GLOBALS;

extern WINHELP_GLOBALS Globals;

VOID WINHELP_CreateHelpWindow(LPCSTR, LONG, LPCSTR, BOOL, HWND, LPPOINT, INT);
INT  WINHELP_MessageBoxIDS(UINT, UINT, WORD);
INT  WINHELP_MessageBoxIDS_s(UINT, LPCSTR, UINT, WORD);

extern CHAR MAIN_WIN_CLASS_NAME[];
extern CHAR BUTTON_BOX_WIN_CLASS_NAME[];
extern CHAR TEXT_WIN_CLASS_NAME[];
extern CHAR SHADOW_WIN_CLASS_NAME[];
extern CHAR STRING_BUTTON[];
extern CHAR STRING_MENU_Xx[];
extern CHAR STRING_DIALOG_TEST[];

#define STRINGID(id) (0x##id + Globals.wStringTableOffset)

#else /* RC_INVOKED */

#define STRINGID(id) id

#endif

/* Stringtable index */
#define IDS_LANGUAGE_ID      STRINGID(00)
#define IDS_WINE_HELP        STRINGID(01)
#define IDS_ERROR            STRINGID(02)
#define IDS_WARNING          STRINGID(03)
#define IDS_INFO             STRINGID(04)
#define IDS_NOT_IMPLEMENTED  STRINGID(05)
#define IDS_HLPFILE_ERROR_s  STRINGID(06)
#define IDS_CONTENTS         STRINGID(07)
#define IDS_SEARCH           STRINGID(08)
#define IDS_BACK             STRINGID(09)
#define IDS_HISTORY          STRINGID(0a)
#define IDS_ALL_FILES        STRINGID(0b)
#define IDS_HELP_FILES_HLP   STRINGID(0c)

/* Menu `File' */
#define WH_OPEN             11
#define WH_PRINT            12
#define WH_PRINTER_SETUP    13
#define WH_EXIT             14

/* Menu `Edit' */
#define WH_COPY_DIALOG      21
#define WH_ANNOTATE         22

/* Menu `Bookmark' */
#define WH_BOOKMARK_DEFINE  31

/* Menu `Help' */
#define WH_HELP_ON_HELP     41
#define WH_HELP_ON_TOP      42
#define WH_ABOUT            43
#define WH_ABOUT_WINE       44

/* Buttons */
#define WH_FIRST_BUTTON     500

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
