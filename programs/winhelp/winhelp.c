/*
 * Help Viewer
 *
 * Copyright 1996 Ulrich Schmid <uschmid@mail.hh.provi.de>
 */

#include <stdio.h>
#include "windows.h"
#ifdef WINELIB
#include "resource.h"
#include "options.h"
#include "shell.h"
extern const char people[];
#endif
#include "winhelp.h"

VOID LIBWINE_Register_De(void);
VOID LIBWINE_Register_En(void);
VOID LIBWINE_Register_Fi(void);
VOID LIBWINE_Register_Fr(void);

static BOOL    WINHELP_RegisterWinClasses();
static LRESULT WINHELP_MainWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT WINHELP_TextWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT WINHELP_ButtonBoxWndProc(HWND, UINT, WPARAM, LPARAM);
static VOID    WINHELP_CheckPopup(UINT);
static BOOL    WINHELP_SplitLines(HWND hWnd, LPSIZE);
static VOID    WINHELP_InitFonts(HWND hWnd);
static VOID    WINHELP_DeleteLines(WINHELP_WINDOW*);
static VOID    WINHELP_DeleteWindow(WINHELP_WINDOW*);
static VOID    WINHELP_SetupText(HWND hWnd);
static BOOL    WINHELP_AppendText(WINHELP_LINE***, WINHELP_LINE_PART***,
				  LPSIZE, LPSIZE, INT*, INT, LPCSTR, UINT,
				  HFONT, COLORREF, HLPFILE_LINK*);

WINHELP_GLOBALS Globals = {3, 0, 0, 0, 0, 0};

static BOOL MacroTest = FALSE;

/***********************************************************************
 *
 *           WinMain
 */

int PASCAL WinMain (HANDLE hInstance, HANDLE prev, LPSTR cmdline, int show)
{
  LPCSTR opt_lang = "En";
  CHAR   lang[3];
  MSG    msg;
  LONG   lHash = 0;
  INT    langnum;

#if defined(WINELIB) && !defined(HAVE_WINE_CONSTRUCTOR)
  /* Register resources */
  LIBWINE_Register_De();
  LIBWINE_Register_En();
  LIBWINE_Register_Fi();
  LIBWINE_Register_Fr();
#endif

  Globals.hInstance = hInstance;

  /* Get options */
  while (*cmdline && (*cmdline == ' ' || *cmdline == '-'))
    {
      CHAR   option;
      LPCSTR topic_id;
      if (*cmdline++ == ' ') continue;

      option = *cmdline;
      if (option) cmdline++;
      while (*cmdline && *cmdline == ' ') cmdline++;
      switch(option)
	{
	case 'i':
	case 'I':
	  topic_id = cmdline;
	  while (*cmdline && *cmdline != ' ') cmdline++;
	  if (*cmdline) *cmdline++ = '\0';
	  lHash = HLPFILE_Hash(topic_id);
	  break;

	case '3':
	case '4':
	  Globals.wVersion = option - '0';
	  break;

	case 't':
	  MacroTest = TRUE;
	  break;
	}
    }

#ifdef WINELIB
  opt_lang = Languages[Options.language].name;
#endif

  /* Find language specific string table */
  for (langnum = 0; langnum <= MAX_LANGUAGE_NUMBER; langnum++)
    {
      Globals.wStringTableOffset = langnum * 0x100;
      if (LoadString(hInstance, IDS_LANGUAGE_ID, lang, sizeof(lang)) &&
	  !lstrcmp(opt_lang, lang))
	break;
    }
  if (langnum > MAX_LANGUAGE_NUMBER)
    {
      /* Find fallback language */
      for (langnum = 0; langnum <= MAX_LANGUAGE_NUMBER; langnum++)
	{
	  Globals.wStringTableOffset = langnum * 0x100;
	  if (LoadString(hInstance, IDS_LANGUAGE_ID, lang, sizeof(lang)))
	    break;
	}
      if (langnum > MAX_LANGUAGE_NUMBER)
	{
	MessageBox(0, "No language found", "FATAL ERROR", MB_OK);
	return(1);
	}
    }

  /* Change Resource names */
  lstrcpyn(STRING_MENU_Xx + lstrlen(STRING_MENU_Xx) - 2, lang, 3);

  /* Create primary window */
  WINHELP_RegisterWinClasses();
  WINHELP_CreateHelpWindow(cmdline, lHash, "main", FALSE, NULL, NULL, show);

  /* Message loop */
  while (GetMessage (&msg, 0, 0, 0))
    {
      TranslateMessage (&msg);
      DispatchMessage (&msg);
    }
  return 0;
}

/***********************************************************************
 *
 *           RegisterWinClasses
 */

static BOOL WINHELP_RegisterWinClasses()
{
  WNDCLASS class_main, class_button_box, class_text, class_shadow;

  class_main.style               = CS_HREDRAW | CS_VREDRAW;
  class_main.lpfnWndProc         = WINHELP_MainWndProc;
  class_main.cbClsExtra          = 0;
  class_main.cbWndExtra          = sizeof(LONG);
  class_main.hInstance           = Globals.hInstance;
  class_main.hIcon               = LoadIcon (0, IDI_APPLICATION);
  class_main.hCursor             = LoadCursor (0, IDC_ARROW);
  class_main.hbrBackground       = GetStockObject (WHITE_BRUSH);
  class_main.lpszMenuName        = 0;
  class_main.lpszClassName       = MAIN_WIN_CLASS_NAME;

  class_button_box               = class_main;
  class_button_box.lpfnWndProc   = WINHELP_ButtonBoxWndProc;
  class_button_box.hbrBackground = GetStockObject(GRAY_BRUSH);
  class_button_box.lpszClassName = BUTTON_BOX_WIN_CLASS_NAME;

  class_text = class_main;
  class_text.lpfnWndProc         = WINHELP_TextWndProc;
  class_text.lpszClassName       = TEXT_WIN_CLASS_NAME;

  class_shadow = class_main;
  class_shadow.lpfnWndProc       = DefWindowProc;
  class_shadow.hbrBackground     = GetStockObject(GRAY_BRUSH);
  class_shadow.lpszClassName     = SHADOW_WIN_CLASS_NAME;

  return (RegisterClass(&class_main) &&
	  RegisterClass(&class_button_box) &&
	  RegisterClass(&class_text) &&
	  RegisterClass(&class_shadow));
}

/***********************************************************************
 *
 *           WINHELP_CreateHelpWindow
 */

VOID WINHELP_CreateHelpWindow(LPCSTR lpszFile, LONG lHash, LPCSTR lpszWindow,
			      BOOL bPopup, HWND hParentWnd, LPPOINT mouse, INT nCmdShow)
{
  CHAR    szCaption[MAX_STRING_LEN];
  CHAR    szContents[MAX_STRING_LEN];
  CHAR    szSearch[MAX_STRING_LEN];
  CHAR    szBack[MAX_STRING_LEN];
  CHAR    szHistory[MAX_STRING_LEN];
  SIZE    size   = {CW_USEDEFAULT, CW_USEDEFAULT};
  POINT   origin = {240, 0};
  LPSTR   ptr;
  HGLOBAL handle;
  WINHELP_WINDOW *win, *oldwin;
  HLPFILE_PAGE   *page;
  HLPFILE_MACRO  *macro;
  HWND hWnd;
  BOOL bPrimary;

  if (bPopup)
    lpszWindow = NULL;
  else if (!lpszWindow || !lpszWindow[0])
    lpszWindow = Globals.active_win->lpszName;
  bPrimary = lpszWindow && !lstrcmpi(lpszWindow, "main");

  /* Read help file */
  if (lpszFile[0])
    {
      page = lHash ? HLPFILE_PageByHash(lpszFile, lHash) : HLPFILE_Contents(lpszFile);

      /* Add Suffix `.hlp' */
      if (!page && lstrcmpi(lpszFile + strlen(lpszFile) - 4, ".hlp"))
	{
	  CHAR      szFile_hlp[MAX_PATHNAME_LEN];

	  lstrcpyn(szFile_hlp, lpszFile, sizeof(szFile_hlp) - 4);
	  szFile_hlp[sizeof(szFile_hlp) - 5] = '\0';
	  lstrcat(szFile_hlp, ".hlp");

	  page = lHash ? HLPFILE_PageByHash(szFile_hlp, lHash) : HLPFILE_Contents(szFile_hlp);
	  if (!page)
	    {
	      WINHELP_MessageBoxIDS_s(IDS_HLPFILE_ERROR_s, lpszFile, IDS_ERROR, MB_OK);
	      if (Globals.win_list) return;
	    }
	}
    }
  else page = 0;

  /* Calculate horizontal size and position of a popup window */
  if (bPopup)
    {
      RECT parent_rect;
      GetWindowRect(hParentWnd, &parent_rect);
      size.cx = (parent_rect.right  - parent_rect.left) / 2;

      origin = *mouse;
      ClientToScreen(hParentWnd, &origin);
      origin.x -= size.cx / 2;
      origin.x  = MIN(origin.x, GetSystemMetrics(SM_CXSCREEN) - size.cx);
      origin.x  = MAX(origin.x, 0);
    }

  /* Initialize WINHELP_WINDOW struct */
  handle = GlobalAlloc(GMEM_FIXED, sizeof(WINHELP_WINDOW) +
		       (lpszWindow ? strlen(lpszWindow) + 1 : 0));
  if (!handle) return;
  win = GlobalLock(handle);
  win->hSelf = handle;
  win->next  = Globals.win_list;
  Globals.win_list = win;
  if (lpszWindow)
    {
      ptr = GlobalLock(handle);
      ptr += sizeof(WINHELP_WINDOW);
      lstrcpy(ptr, (LPSTR) lpszWindow);
      win->lpszName = ptr;
    }
  else win->lpszName = NULL;
  win->page = page;
  win->first_button = 0;
  win->first_line = 0;
  win->hMainWnd = 0;
  win->hButtonBoxWnd = 0;
  win->hTextWnd = 0;
  win->hShadowWnd = 0;

  Globals.active_win = win;

  /* Initialize default pushbuttons */
  if (MacroTest && !bPopup)
    MACRO_CreateButton("BTN_TEST", "&Test", "MacroTest");
  if (bPrimary && page)
    {
      LoadString(Globals.hInstance, IDS_CONTENTS, szContents, sizeof(szContents));
      LoadString(Globals.hInstance, IDS_SEARCH,   szSearch,   sizeof(szSearch));
      LoadString(Globals.hInstance, IDS_BACK,     szBack,     sizeof(szBack));
      LoadString(Globals.hInstance, IDS_HISTORY,  szHistory,  sizeof(szHistory));
      MACRO_CreateButton("BTN_CONTENTS", szContents, "Contents()");
      MACRO_CreateButton("BTN_SEARCH",   szSearch,   "Search()");
      MACRO_CreateButton("BTN_BACK",     szBack,     "Back()");
      MACRO_CreateButton("BTN_HISTORY",  szHistory,  "History()");
    }

  /* Initialize file specific pushbuttons */
  if (!bPopup && page)
    for (macro = page->file->first_macro; macro; macro = macro->next)
      MACRO_ExecuteMacro(macro->lpszMacro);

  /* Reuse existing window */
  if (lpszWindow)
    for (oldwin = win->next; oldwin; oldwin = oldwin->next)
      if (oldwin->lpszName && !lstrcmpi(oldwin->lpszName, lpszWindow))
	{
	  WINHELP_BUTTON *button;

	  win->hMainWnd      = oldwin->hMainWnd;
	  win->hButtonBoxWnd = oldwin->hButtonBoxWnd;
	  win->hTextWnd      = oldwin->hTextWnd;
	  oldwin->hMainWnd = oldwin->hButtonBoxWnd = oldwin->hTextWnd = 0;

	  SetWindowLong(win->hMainWnd,      0, (LONG) win);
	  SetWindowLong(win->hButtonBoxWnd, 0, (LONG) win);
	  SetWindowLong(win->hTextWnd,      0, (LONG) win);

	  WINHELP_InitFonts(win->hMainWnd);

	  SetWindowText(win->hMainWnd, page->file->lpszTitle);

	  WINHELP_SetupText(win->hTextWnd);
	  InvalidateRect(win->hTextWnd, NULL, TRUE);
	  SendMessage(win->hMainWnd, WM_USER, 0, 0);
	  UpdateWindow(win->hTextWnd);


	  for (button = oldwin->first_button; button; button = button->next)
	    DestroyWindow(button->hWnd);
  
	  WINHELP_DeleteWindow(oldwin);
	  return;
	}

  /* Create main Window */
  if (!page) LoadString(Globals.hInstance, IDS_WINE_HELP, szCaption, sizeof(szCaption));
  hWnd = CreateWindow (bPopup ? TEXT_WIN_CLASS_NAME : MAIN_WIN_CLASS_NAME,
		       page ? page->file->lpszTitle : szCaption,
		       bPopup ? WS_POPUPWINDOW | WS_BORDER : WS_OVERLAPPEDWINDOW,
		       origin.x, origin.y, size.cx, size.cy,
		       0, bPrimary ? LoadMenu(Globals.hInstance, STRING_MENU_Xx) : 0,
		       Globals.hInstance, win);

  ShowWindow (hWnd, nCmdShow);
  UpdateWindow (hWnd);
}

/***********************************************************************
 *
 *           WINHELP_MainWndProc
 */

static LRESULT WINHELP_MainWndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  WINHELP_WINDOW *win;
  WINHELP_BUTTON *button;
  RECT rect, button_box_rect;
  INT  text_top;

  WINHELP_CheckPopup(msg);

  switch (msg)
    {
    case WM_NCCREATE:
      win = (WINHELP_WINDOW*) ((LPCREATESTRUCT) lParam)->lpCreateParams;
      SetWindowLong(hWnd, 0, (LONG) win);
      win->hMainWnd = hWnd;
      break;

    case WM_CREATE:
      win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);

      /* Create button box and text Window */
      CreateWindow(BUTTON_BOX_WIN_CLASS_NAME, "", WS_CHILD | WS_VISIBLE,
		   0, 0, 0, 0, hWnd, 0, Globals.hInstance, win);

      CreateWindow(TEXT_WIN_CLASS_NAME, "", WS_CHILD | WS_VISIBLE,
		   0, 0, 0, 0, hWnd, 0, Globals.hInstance, win);

      /* Fall through */
    case WM_USER:
    case WM_WINDOWPOSCHANGED:
      win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
      GetClientRect(hWnd, &rect);

      /* Update button box and text Window */
      SetWindowPos(win->hButtonBoxWnd, HWND_TOP,
		   rect.left, rect.top,
		   rect.right - rect.left,
		   rect.bottom - rect.top, 0);

      GetWindowRect(win->hButtonBoxWnd, &button_box_rect);
      text_top = rect.top + button_box_rect.bottom - button_box_rect.top;

      SetWindowPos(win->hTextWnd, HWND_TOP,
		   rect.left, text_top,
		   rect.right - rect.left,
		   rect.bottom - text_top, 0);

      break;

    case WM_COMMAND:
      Globals.active_win = win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
      switch (wParam)
	{
	  /* Menu FILE */
	case WH_OPEN:            MACRO_FileOpen();       break;
	case WH_PRINT:           MACRO_Print();          break;
	case WH_PRINTER_SETUP:   MACRO_PrinterSetup();   break;
	case WH_EXIT:            MACRO_Exit();           break;

	  /* Menu EDIT */
	case WH_COPY_DIALOG:     MACRO_CopyDialog();     break;
	case WH_ANNOTATE:        MACRO_Annotate();       break;

	  /* Menu Bookmark */
	case WH_BOOKMARK_DEFINE: MACRO_BookmarkDefine(); break;

	  /* Menu Help */
	case WH_HELP_ON_HELP:    MACRO_HelpOn();         break;
	case WH_HELP_ON_TOP:     MACRO_HelpOnTop();      break;

	  /* Menu Info */
	case WH_ABOUT:           MACRO_About();          break;
#ifdef WINELIB
	case WH_ABOUT_WINE: 
	  ShellAbout(hWnd, "WINE", people, 0);
	  break;
#endif

	default:
	  /* Buttons */
	  for (button = win->first_button; button; button = button->next)
	    if (wParam == button->wParam) break;
	  if (button)
	    MACRO_ExecuteMacro(button->lpszMacro);
	  else
	    WINHELP_MessageBoxIDS(IDS_NOT_IMPLEMENTED, IDS_ERROR, MB_OK);
	  break;
	}
      break;
    }

  return DefWindowProc (hWnd, msg, wParam, lParam);
}

/***********************************************************************
 *
 *           WINHELP_ButtonBoxWndProc
 */

static LRESULT WINHELP_ButtonBoxWndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  WINDOWPOS      *winpos;
  WINHELP_WINDOW *win;
  WINHELP_BUTTON *button;
  SIZE button_size;
  INT  x, y;

  WINHELP_CheckPopup(msg);

  switch(msg)
    {
    case WM_NCCREATE:
      win = (WINHELP_WINDOW*) ((LPCREATESTRUCT) lParam)->lpCreateParams;
      SetWindowLong(hWnd, 0, (LONG) win);
      win->hButtonBoxWnd = hWnd;
      break;

    case WM_WINDOWPOSCHANGING:
      winpos = (WINDOWPOS*) lParam;
      win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);

      /* Update buttons */
      button_size.cx = 0;
      button_size.cy = 0;
      for (button = win->first_button; button; button = button->next)
	{
	  HDC  hDc;
	  SIZE textsize;
	  if (!button->hWnd)
	    button->hWnd = CreateWindow(STRING_BUTTON, (LPSTR) button->lpszName,
					WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					0, 0, 0, 0,
					hWnd, (HMENU) button->wParam,
					Globals.hInstance, 0);
	  hDc = GetDC(button->hWnd);
	  GetTextExtentPoint(hDc, button->lpszName,
			     lstrlen(button->lpszName), &textsize);
	  ReleaseDC(button->hWnd, hDc);

	  button_size.cx = MAX(button_size.cx, textsize.cx + BUTTON_CX);
	  button_size.cy = MAX(button_size.cy, textsize.cy + BUTTON_CY);
	}

      x = 0;
      y = 0;
      for (button = win->first_button; button; button = button->next)
	{
	  SetWindowPos(button->hWnd, HWND_TOP, x, y, button_size.cx, button_size.cy, 0);

	  if (x + 2 * button_size.cx <= winpos->cx)
	    x += button_size.cx;
	  else
	    x = 0, y += button_size.cy;
	}
      winpos->cy = y + (x ? button_size.cy : 0);
      break;

    case WM_COMMAND:
      SendMessage(GetParent(hWnd), msg, wParam, lParam);
      break;
    }

  return(DefWindowProc(hWnd, msg, wParam, lParam));
}

/***********************************************************************
 *
 *           WINHELP_TextWndProc
 */

static LRESULT WINHELP_TextWndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  WINHELP_WINDOW    *win;
  WINHELP_LINE      *line;
  WINHELP_LINE_PART *part;
  WINDOWPOS         *winpos;
  PAINTSTRUCT        ps;
  HDC   hDc;
  POINT mouse;
  INT   scroll_pos;
  HWND  hPopupWnd;
  BOOL  bExit;

  if (msg != WM_LBUTTONDOWN)
    WINHELP_CheckPopup(msg);

  switch (msg)
    {
    case WM_NCCREATE:
      win = (WINHELP_WINDOW*) ((LPCREATESTRUCT) lParam)->lpCreateParams;
      SetWindowLong(hWnd, 0, (LONG) win);
      win->hTextWnd = hWnd;
      if (!win->lpszName) Globals.hPopupWnd = win->hMainWnd = hWnd;
      WINHELP_InitFonts(hWnd);
      break;

    case WM_CREATE:
      win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);

      /* Calculate vertical size and position of a popup window */
      if (!win->lpszName)
	{
	  POINT origin;
	  RECT old_window_rect;
	  RECT old_client_rect;
	  SIZE old_window_size;
	  SIZE old_client_size;
	  SIZE new_client_size;
	  SIZE new_window_size;

	  GetWindowRect(hWnd, &old_window_rect);
	  origin.x = old_window_rect.left;
	  origin.y = old_window_rect.top;
	  old_window_size.cx = old_window_rect.right  - old_window_rect.left;
	  old_window_size.cy = old_window_rect.bottom - old_window_rect.top;

	  GetClientRect(hWnd, &old_client_rect);
	  old_client_size.cx = old_client_rect.right  - old_client_rect.left;
	  old_client_size.cy = old_client_rect.bottom - old_client_rect.top;

	  new_client_size = old_client_size;
	  WINHELP_SplitLines(hWnd, &new_client_size);

	  if (origin.y + POPUP_YDISTANCE + new_client_size.cy <= GetSystemMetrics(SM_CYSCREEN))
	    origin.y += POPUP_YDISTANCE;
	  else
	    origin.y -= POPUP_YDISTANCE + new_client_size.cy;

	  new_window_size.cx = old_window_size.cx - old_client_size.cx + new_client_size.cx;
	  new_window_size.cy = old_window_size.cy - old_client_size.cy + new_client_size.cy;

	  win->hShadowWnd = 
	    CreateWindow(SHADOW_WIN_CLASS_NAME, "", WS_POPUP | WS_VISIBLE,
			 origin.x + SHADOW_DX, origin.y + SHADOW_DY,
			 new_window_size.cx, new_window_size.cy,
			 0, 0, Globals.hInstance, 0);

	  SetWindowPos(hWnd, HWND_TOP, origin.x, origin.y,
		       new_window_size.cx, new_window_size.cy,
		       SWP_NOZORDER | SWP_NOACTIVATE);
	  ShowWindow(win->hShadowWnd, SW_NORMAL);
	}
      break;

    case WM_WINDOWPOSCHANGED:
      winpos = (WINDOWPOS*) lParam;
      if (!(winpos->flags & SWP_NOSIZE)) WINHELP_SetupText(hWnd);
      break;

    case WM_VSCROLL:
      {
	BOOL  update = TRUE;
	RECT  rect;
	INT   Min, Max;
	INT   CurPos = GetScrollPos(hWnd, SB_VERT);
	GetScrollRange(hWnd, SB_VERT, &Min, &Max);
	GetClientRect(hWnd, &rect);

	switch (wParam & 0xffff)
	  {
	  case SB_THUMBTRACK:
	  case SB_THUMBPOSITION: CurPos  = wParam >> 16;                   break;
	  case SB_TOP:           CurPos  = Min;                            break;
	  case SB_BOTTOM:        CurPos  = Max;                            break;
	  case SB_PAGEUP:        CurPos -= (rect.bottom - rect.top) / 2;   break;
	  case SB_PAGEDOWN:      CurPos += (rect.bottom - rect.top) / 2;   break;
	  case SB_LINEUP:        CurPos -= GetSystemMetrics(SM_CXVSCROLL); break;
	  case SB_LINEDOWN:      CurPos += GetSystemMetrics(SM_CXVSCROLL); break;
	  default: update = FALSE;
	  }
	if (update)
	  {
	    INT dy = GetScrollPos(hWnd, SB_VERT) - CurPos;
	    SetScrollPos(hWnd, SB_VERT, CurPos, TRUE);
	    ScrollWindow(hWnd, 0, dy, NULL, NULL);
	    UpdateWindow(hWnd);
	  }
      }
      break;

    case WM_PAINT:
      hDc = BeginPaint (hWnd, &ps);
      win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
      scroll_pos = GetScrollPos(hWnd, SB_VERT);

      for (line = win->first_line; line; line = line->next)
	for (part = &line->first_part; part; part = part->next)
	  {
	    SelectObject(hDc, part->hFont);
	    SetTextColor(hDc, part->color);
	    TextOut(hDc, part->rect.left, part->rect.top - scroll_pos,
		    (LPSTR) part->lpsText, part->wTextLen);
	  }

      EndPaint (hWnd, &ps);
      break;

    case WM_LBUTTONDOWN:
      win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
      scroll_pos = GetScrollPos(hWnd, SB_VERT);

      hPopupWnd = Globals.hPopupWnd;
      Globals.hPopupWnd = 0;

      mouse.x = LOWORD(lParam);
      mouse.y = HIWORD(lParam);
      for (line = win->first_line; line; line = line->next)
	for (part = &line->first_part; part; part = part->next)
	  if (part->link.lpszPath &&
	      part->rect.left   <= mouse.x &&
	      part->rect.right  >= mouse.x &&
	      part->rect.top    <= mouse.y + scroll_pos &&
	      part->rect.bottom >= mouse.y + scroll_pos)
	    WINHELP_CreateHelpWindow(part->link.lpszPath, part->link.lHash, NULL,
				     part->link.bPopup, hWnd, &mouse,  SW_NORMAL);
      if (hPopupWnd)
	DestroyWindow(hPopupWnd);
      break;

    case WM_NCDESTROY:
      win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);

      if (hWnd == Globals.hPopupWnd) Globals.hPopupWnd = 0;

      bExit = (Globals.wVersion >= 4 && !lstrcmpi(win->lpszName, "main"));

      WINHELP_DeleteWindow(win);

      if (bExit) MACRO_Exit();

      if (!Globals.win_list)
	PostQuitMessage (0);
      break;
    }

  return DefWindowProc (hWnd, msg, wParam, lParam);
}

/***********************************************************************
 *
 *           SetupText
 */

static VOID WINHELP_SetupText(HWND hWnd)
{
  HDC  hDc = GetDC(hWnd);
  RECT rect;
  SIZE newsize;

  ShowScrollBar(hWnd, SB_VERT, FALSE);
  if (!WINHELP_SplitLines(hWnd, NULL))
    {
      ShowScrollBar(hWnd, SB_VERT, TRUE);
      GetClientRect(hWnd, &rect);

      WINHELP_SplitLines(hWnd, &newsize);
      SetScrollRange(hWnd, SB_VERT, 0, rect.top + newsize.cy - rect.bottom, TRUE);
    }
  else SetScrollPos(hWnd, SB_VERT, 0, FALSE);

  ReleaseDC(hWnd, hDc);
}

/***********************************************************************
 *
 *           WINHELP_SplitLines
 */

static BOOL WINHELP_SplitLines(HWND hWnd, LPSIZE newsize)
{
  WINHELP_WINDOW     *win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
  HLPFILE_PARAGRAPH  *p;
  WINHELP_LINE      **line = &win->first_line;
  WINHELP_LINE_PART **part = 0;
  INT                 line_ascent = 0;
  SIZE                space;
  RECT                rect;
  HDC                 hDc;

  if (newsize) newsize->cx = newsize->cy = 0;

  if (!win->page) return TRUE;

  WINHELP_DeleteLines(win);

  GetClientRect(hWnd, &rect);

  rect.top    += INTERNAL_BORDER_WIDTH;
  rect.left   += INTERNAL_BORDER_WIDTH;
  rect.right  -= INTERNAL_BORDER_WIDTH;
  rect.bottom -= INTERNAL_BORDER_WIDTH;


  space.cy = rect.top;
  space.cx = rect.left;

  hDc = GetDC(hWnd);

  for (p = win->page->first_paragraph; p; p = p->next)
    {
      TEXTMETRIC tm;
      SIZE textsize = {0, 0};
      LPCSTR text    = p->lpszText;
      UINT len    = strlen(text);
      UINT indent = 0;

      UINT  wFont      = (p->wFont < win->fonts_len) ? p->wFont : 0;
      BOOL  bUnderline = p->link && !p->link->bPopup;
      HFONT hFont      = win->fonts[wFont][bUnderline ? 1 : 0];

      COLORREF       color = RGB(0, 0, 0);
      if (p->link)   color = RGB(0, 0x80, 0);
      if (p->bDebug) color = RGB(0xff, 0, 0);

      SelectObject(hDc, hFont);

      GetTextMetrics (hDc, &tm);

      if (p->wIndent)
	{
	  indent = p->wIndent * 5 * tm.tmAveCharWidth;
	  if (!part)
	    space.cx = rect.left + indent - 2 * tm.tmAveCharWidth;
	}

      if (p->wVSpace)
	{
	  part = 0;
	  space.cx = rect.left + indent;
	  space.cy += (p->wVSpace - 1) * tm.tmHeight;
	}

      if (p->wHSpace)
	{
	  space.cx += p->wHSpace * 2 * tm.tmAveCharWidth;
	}

      while (len)
	{
	  INT free_width = rect.right - (part ? (*line)->rect.right : rect.left) - space.cx;
	  UINT low = 0, curr = len, high = len, textlen = 0;

	  if (free_width > 0)
	    {
	      while (1)
		{
		  GetTextExtentPoint(hDc, text, curr, &textsize);

		  if (textsize.cx <= free_width) low = curr;
		  else high = curr;

		  if (high <= low + 1) break;

		  if (textsize.cx) curr = (curr * free_width) / textsize.cx;
		  if (curr <= low) curr = low + 1;
		  else if (curr >= high) curr = high - 1;
		}
	      textlen = low;
	      while (textlen && text[textlen] && text[textlen] != ' ') textlen--;
	    }
	  if (!part && !textlen) textlen = MAX(low, 1);

	  if (free_width <= 0 || !textlen)
	    {
	      part = 0;
	      space.cx = rect.left + indent;
	      space.cx = MIN(space.cx, rect.right - rect.left - 1);
	      continue;
	    }

	  if (!WINHELP_AppendText(&line, &part, &space, &textsize,
				  &line_ascent, tm.tmAscent,
				  text, textlen, hFont, color, p->link) ||
	      (!newsize && (*line)->rect.bottom > rect.bottom))
	    {
	      ReleaseDC(hWnd, hDc);
	      return FALSE;
	    }

	  if (newsize)
	    newsize->cx = MAX(newsize->cx, (*line)->rect.right + INTERNAL_BORDER_WIDTH);

	  len -= textlen;
	  text += textlen;
	  if (text[0] == ' ') text++, len--;
	}
    }

  if (newsize)
    newsize->cy = (*line)->rect.bottom + INTERNAL_BORDER_WIDTH;

  ReleaseDC(hWnd, hDc);
  return TRUE;
}

/***********************************************************************
 *
 *           WINHELP_AppendText
 */

static BOOL WINHELP_AppendText(WINHELP_LINE ***linep, WINHELP_LINE_PART ***partp,
			       LPSIZE space, LPSIZE textsize,
			       INT *line_ascent, INT ascent,
			       LPCSTR text, UINT textlen,
			       HFONT font, COLORREF color, HLPFILE_LINK *link)
{
  HGLOBAL handle;
  WINHELP_LINE      *line;
  WINHELP_LINE_PART *part;
  LPSTR ptr;

  if (!*partp) /* New line */
    {
      *line_ascent  = ascent;

      handle = GlobalAlloc(GMEM_FIXED, sizeof(WINHELP_LINE) + textlen +
			   (link ? lstrlen(link->lpszPath) + 1 : 0));
      if (!handle) return FALSE;
      line          = GlobalLock(handle);
      line->next    = 0;
      part          = &line->first_part;
      ptr           = GlobalLock(handle);
      ptr          += sizeof(WINHELP_LINE);

      line->rect.top    = (**linep ? (**linep)->rect.bottom : 0) + space->cy;
      line->rect.bottom = line->rect.top;
      line->rect.left   = space->cx;
      line->rect.right  = space->cx;

      if (**linep) *linep = &(**linep)->next; 
      **linep = line;
      space->cy = 0;
    }
  else /* Same line */
    {
      line = **linep;

      if (*line_ascent < ascent)
	{
	  WINHELP_LINE_PART *p;
	  for (p = &line->first_part; p; p = p->next)
	    {
	      p->rect.top    += ascent - *line_ascent;
	      p->rect.bottom += ascent - *line_ascent;
	    }
	  line->rect.bottom += ascent - *line_ascent;
	  *line_ascent = ascent;
	}

      handle = GlobalAlloc(GMEM_FIXED, sizeof(WINHELP_LINE_PART) + textlen +
			   (link ? lstrlen(link->lpszPath) + 1 : 0));
      if (!handle) return FALSE;
      part    = GlobalLock(handle);
      **partp = part;
      ptr     = GlobalLock(handle);
      ptr    += sizeof(WINHELP_LINE_PART);
    }

  hmemcpy(ptr, text, textlen);
  part->rect.left     = line->rect.right + (*partp ? space->cx : 0);
  part->rect.right    = part->rect.left + textsize->cx;
  line->rect.right    = part->rect.right;
  part->rect.top      =
    ((*partp) ? line->rect.top : line->rect.bottom) + *line_ascent - ascent;
  part->rect.bottom   = part->rect.top + textsize->cy;
  line->rect.bottom   = MAX(line->rect.bottom, part->rect.bottom);
  part->hSelf         = handle;
  part->lpsText       = ptr;
  part->wTextLen      = textlen;
  part->hFont         = font;
  part->color         = color;
  if (link)
    {
      strcpy(ptr + textlen, link->lpszPath);
      part->link.lpszPath = ptr + textlen;
      part->link.lHash    = link->lHash;
      part->link.bPopup   = link->bPopup;
    }
  else part->link.lpszPath = 0;

  part->next          = 0;
  *partp              = &part->next;

  space->cx = 0;

  return TRUE;
}

/***********************************************************************
 *
 *           WINHELP_CheckPopup
 */

static VOID WINHELP_CheckPopup(UINT msg)
{
  if (!Globals.hPopupWnd) return;

  switch (msg)
    {
    case WM_COMMAND:
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_NCLBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
      DestroyWindow(Globals.hPopupWnd);
      Globals.hPopupWnd = 0;
    }
}

/***********************************************************************
 *
 *           WINHELP_DeleteLines
 */

static VOID WINHELP_DeleteLines(WINHELP_WINDOW *win)
{
  WINHELP_LINE      *line, *next_line;
  WINHELP_LINE_PART *part, *next_part;
  for(line = win->first_line; line; line = next_line)
    {
      next_line = line->next;
      for(part = &line->first_part; part; part = next_part)
	{
	  next_part = part->next;
	  GlobalFree(part->hSelf);
	}
    }
  win->first_line = 0;
}

/***********************************************************************
 *
 *           WINHELP_DeleteWindow
 */

static VOID WINHELP_DeleteWindow(WINHELP_WINDOW *win)
{
  WINHELP_WINDOW **w;

  for (w = &Globals.win_list; *w; w = &(*w)->next)
    if (*w == win)
      {
	*w = win->next;
	break;
      }

  if (win->hShadowWnd) DestroyWindow(win->hShadowWnd);
  HLPFILE_FreeHlpFilePage(win->page);
  WINHELP_DeleteLines(win);
  GlobalFree(win->hSelf);
}

/***********************************************************************
 *
 *           WINHELP_InitFonts
 */

static VOID WINHELP_InitFonts(HWND hWnd)
{
  WINHELP_WINDOW *win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
  LOGFONT logfontlist[] = {
    {-10, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"},
    {-12, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"},
    {-12, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"},
    {-12, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"},
    {-12, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"},
    {-10, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"},
    { -8, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"}};
#define FONTS_LEN (sizeof(logfontlist)/sizeof(*logfontlist))

  static HFONT fonts[FONTS_LEN][2];
  static BOOL init = 0;

  win->fonts_len = FONTS_LEN;
  win->fonts = fonts;

  if (!init)
    {
      INT i;

      for(i = 0; i < FONTS_LEN; i++)
	{
	  LOGFONT logfont = logfontlist[i];

	  fonts[i][0] = CreateFontIndirect(&logfont);
	  logfont.lfUnderline = 1;
	  fonts[i][1] = CreateFontIndirect(&logfont);
	}

      init = 1;
    }
}

/***********************************************************************
 *
 *           WINHELP_MessageBoxIDS
 */

INT WINHELP_MessageBoxIDS(UINT ids_text, UINT ids_title, WORD type)
{
  CHAR text[MAX_STRING_LEN];
  CHAR title[MAX_STRING_LEN];

  LoadString(Globals.hInstance, ids_text, text, sizeof(text));
  LoadString(Globals.hInstance, ids_title, title, sizeof(title));

  return(MessageBox(0, text, title, type));
}

/***********************************************************************
 *
 *           MAIN_MessageBoxIDS_s
 */

INT WINHELP_MessageBoxIDS_s(UINT ids_text, LPCSTR str, UINT ids_title, WORD type)
{
  CHAR text[MAX_STRING_LEN];
  CHAR title[MAX_STRING_LEN];
  CHAR newtext[MAX_STRING_LEN + MAX_PATHNAME_LEN];

  LoadString(Globals.hInstance, ids_text, text, sizeof(text));
  LoadString(Globals.hInstance, ids_title, title, sizeof(title));
  wsprintf(newtext, text, str);

  return(MessageBox(0, newtext, title, type));
}

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
