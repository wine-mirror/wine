/*
 * Message boxes
 *
 * Copyright 1995 Bernd Schmidt
 *
 */

#include "windows.h"
#include "dlgs.h"
#include "dialog.h"
#include "selectors.h"
#include "../rc/sysres.h"
#include "task.h"

typedef struct {
  LPSTR	title;
  LPSTR text;
  WORD  type;
} MSGBOX, *LPMSGBOX;

LONG SystemMessageBoxProc(HWND hwnd, WORD message, WORD wParam, LONG lParam)
{
  LPMSGBOX lpmb;
  RECT rect, textrect;
  HWND hItem;
  HDC hdc;
  LONG lRet;
  int i, buttons, bwidth, bheight, theight, wwidth, bpos;
  int borheight, iheight, tiheight;
  
  switch(message) {
   case WM_INITDIALOG:
    lpmb = (LPMSGBOX)lParam;
    if (lpmb->title != NULL) {
      SetWindowText(hwnd, lpmb->title);
    }
    SetWindowText(GetDlgItem(hwnd, 100), lpmb->text);
    /* Hide not selected buttons */
    switch(lpmb->type & MB_TYPEMASK) {
     case MB_OK:
      ShowWindow(GetDlgItem(hwnd, 2), SW_HIDE);
      /* fall through */
     case MB_OKCANCEL:
      ShowWindow(GetDlgItem(hwnd, 3), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, 4), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, 5), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, 6), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, 7), SW_HIDE);
      break;
     case MB_ABORTRETRYIGNORE:
      ShowWindow(GetDlgItem(hwnd, 1), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, 2), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, 6), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, 7), SW_HIDE);
      break;
     case MB_YESNO:
      ShowWindow(GetDlgItem(hwnd, 2), SW_HIDE);
      /* fall through */
     case MB_YESNOCANCEL:
      ShowWindow(GetDlgItem(hwnd, 1), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, 3), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, 4), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, 5), SW_HIDE);
      break;
    }
    /* Set the icon */
    switch(lpmb->type & MB_ICONMASK) {
     case MB_ICONEXCLAMATION:
      SendDlgItemMessage(hwnd, stc1, STM_SETICON, LoadIcon(0, IDI_EXCLAMATION), 0);
      break;
     case MB_ICONQUESTION:
      SendDlgItemMessage(hwnd, stc1, STM_SETICON, LoadIcon(0, IDI_QUESTION), 0);
      break;
     case MB_ICONASTERISK:
      SendDlgItemMessage(hwnd, stc1, STM_SETICON, LoadIcon(0, IDI_ASTERISK), 0);
      break;
     case MB_ICONHAND:
     default:
      SendDlgItemMessage(hwnd, stc1, STM_SETICON, LoadIcon(0, IDI_HAND), 0);
      break;
    }
    
    /* Position everything */
    GetWindowRect(hwnd, &rect);
    borheight = rect.bottom - rect.top;
    wwidth = rect.right - rect.left;
    GetClientRect(hwnd, &rect);
    borheight -= rect.bottom - rect.top;

    /* Get the icon height */
    GetWindowRect(GetDlgItem(hwnd, 1088), &rect);
    iheight = rect.bottom - rect.top;
    
    /* Get the number of visible buttons and their width */
    GetWindowRect(GetDlgItem(hwnd, 2), &rect);
    bheight = rect.bottom - rect.top;
    bwidth = rect.left;
    GetWindowRect(GetDlgItem(hwnd, 1), &rect);
    bwidth -= rect.left;
    for (buttons = 0, i = 1; i < 8; i++) {
      hItem = GetDlgItem(hwnd, i);
      if (GetWindowLong(hItem, GWL_STYLE) & WS_VISIBLE) {
	buttons++;
      }
    }
    
    /* Get the text size */
    hItem = GetDlgItem(hwnd, 100);
    GetWindowRect(hItem, &textrect);
    MapWindowPoints(0, hwnd, (LPPOINT)&textrect, 2);
    
    GetClientRect(hItem, &rect);
    hdc = GetDC(hItem);
    lRet = DrawText(hdc, lpmb->text, -1, &rect,
		    DT_LEFT | DT_EXPANDTABS | DT_WORDBREAK | DT_CALCRECT);
    theight = rect.bottom  - rect.top;
    tiheight = 16 + max(iheight, theight);
    ReleaseDC(hItem, hdc);
    
    /* Position the text */
    SetWindowPos(hItem, 0, textrect.left, (tiheight - theight) / 2, 
		 rect.right - rect.left, theight,
		 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    
    /* Position the icon */
    hItem = GetDlgItem(hwnd, 1088);
    GetWindowRect(hItem, &rect);
    MapWindowPoints(0, hwnd, (LPPOINT)&rect, 2);
    SetWindowPos(hItem, 0, rect.left, (tiheight - iheight) / 2, 0, 0,
		 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    
    /* Resize the window */
    SetWindowPos(hwnd, 0, 0, 0, wwidth, 8 + tiheight + bheight + borheight,
		 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    
    /* Position the buttons */
    bpos = (wwidth - bwidth * buttons) / 2;
    GetWindowRect(GetDlgItem(hwnd, 1), &rect);
    for (buttons = i = 0; i < 7; i++) {
      /* some arithmetic to get the right order for YesNoCancel windows */
      hItem = GetDlgItem(hwnd, (i + 5) % 7 + 1);
      if (GetWindowLong(hItem, GWL_STYLE) & WS_VISIBLE) {
	if (buttons++ == ((lpmb->type & MB_DEFMASK) >> 8)) {
	  SetFocus(hItem);
	  SendMessage(hItem, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
	}
	SetWindowPos(hItem, 0, bpos, tiheight, 0, 0,
		     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
	bpos += bwidth;
      }
    }
    return 0;
    break;
    
   case WM_COMMAND:
    switch (wParam) {
     case IDOK:
     case IDCANCEL:
     case IDABORT:
     case IDRETRY:
     case IDIGNORE:
     case IDYES:
     case IDNO:
      EndDialog(hwnd, wParam);
      break;
    }
    break;
  }
  return 0;
}

/**************************************************************************
 *			MessageBox  [USER.1]
 */

int MessageBox(HWND hWnd, LPSTR text, LPSTR title, WORD type)
{
  MSGBOX mbox;

  mbox.title = title;
  mbox.text  = text;
  mbox.type  = type;
  return DialogBoxIndirectParamPtr(GetWindowWord(hWnd, GWW_HINSTANCE),
				   sysres_DIALOG_MSGBOX, hWnd,
				   GetWndProcEntry16("SystemMessageBoxProc"),
				   (LONG)&mbox);
}

/**************************************************************************
 *			FatalAppExit  [USER.137]
 */

void FatalAppExit(WORD wAction, LPSTR str)
{
  MessageBox(0, str, NULL, MB_SYSTEMMODAL | MB_OK);
  TASK_KillCurrentTask(0);
}
