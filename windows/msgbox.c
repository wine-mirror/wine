/*
 * Message boxes
 *
 * Copyright 1995 Bernd Schmidt
 *
 */

#include <stdio.h>
#include <malloc.h>
#include "windows.h"
#include "dlgs.h"
#include "heap.h"
#include "module.h"
#include "win.h"
#include "resource.h"
#include "task.h"

typedef struct {
  LPCSTR title;
  LPCSTR text;
  WORD  type;
} MSGBOX, *LPMSGBOX;

LRESULT SystemMessageBoxProc(HWND hwnd,UINT message,WPARAM16 wParam,LPARAM lParam)
{
  LPMSGBOX lpmb;
  RECT16 rect, textrect;
  HWND hItem;
  HDC32 hdc;
  LONG lRet;
  int i, buttons, bwidth, bheight, theight, wwidth, bpos;
  int borheight, iheight, tiheight;
  
  switch(message) {
   case WM_INITDIALOG:
    lpmb = (LPMSGBOX)lParam;
    if (lpmb->title) SetWindowText32A(hwnd, lpmb->title);
    SetWindowText32A(GetDlgItem(hwnd, 100), lpmb->text);
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
      SendDlgItemMessage16(hwnd, stc1, STM_SETICON, 
                           (WPARAM16)LoadIcon16(0, IDI_EXCLAMATION), 0);
      break;
     case MB_ICONQUESTION:
      SendDlgItemMessage16(hwnd, stc1, STM_SETICON, 
                           (WPARAM16)LoadIcon16(0, IDI_QUESTION), 0);
      break;
     case MB_ICONASTERISK:
      SendDlgItemMessage16(hwnd, stc1, STM_SETICON, 
                           (WPARAM16)LoadIcon16(0, IDI_ASTERISK), 0);
      break;
     case MB_ICONHAND:
     default:
      SendDlgItemMessage16(hwnd, stc1, STM_SETICON, 
                           (WPARAM16)LoadIcon16(0, IDI_HAND), 0);
      break;
    }
    
    /* Position everything */
    GetWindowRect16(hwnd, &rect);
    borheight = rect.bottom - rect.top;
    wwidth = rect.right - rect.left;
    GetClientRect16(hwnd, &rect);
    borheight -= rect.bottom - rect.top;

    /* Get the icon height */
    GetWindowRect16(GetDlgItem(hwnd, 1088), &rect);
    iheight = rect.bottom - rect.top;
    
    /* Get the number of visible buttons and their width */
    GetWindowRect16(GetDlgItem(hwnd, 2), &rect);
    bheight = rect.bottom - rect.top;
    bwidth = rect.left;
    GetWindowRect16(GetDlgItem(hwnd, 1), &rect);
    bwidth -= rect.left;
    for (buttons = 0, i = 1; i < 8; i++)
    {
      hItem = GetDlgItem(hwnd, i);
      if (GetWindowLong32A(hItem, GWL_STYLE) & WS_VISIBLE) buttons++;
    }
    
    /* Get the text size */
    hItem = GetDlgItem(hwnd, 100);
    GetWindowRect16(hItem, &textrect);
    MapWindowPoints16(0, hwnd, (LPPOINT16)&textrect, 2);
    
    GetClientRect16(hItem, &rect);
    hdc = GetDC32(hItem);
    lRet = DrawText16( hdc, lpmb->text, -1, &rect,
                       DT_LEFT | DT_EXPANDTABS | DT_WORDBREAK | DT_CALCRECT);
    theight = rect.bottom  - rect.top;
    tiheight = 16 + MAX(iheight, theight);
    ReleaseDC32(hItem, hdc);
    
    /* Position the text */
    SetWindowPos(hItem, 0, textrect.left, (tiheight - theight) / 2, 
		 rect.right - rect.left, theight,
		 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    
    /* Position the icon */
    hItem = GetDlgItem(hwnd, 1088);
    GetWindowRect16(hItem, &rect);
    MapWindowPoints16(0, hwnd, (LPPOINT16)&rect, 2);
    SetWindowPos(hItem, 0, rect.left, (tiheight - iheight) / 2, 0, 0,
		 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    
    /* Resize the window */
    SetWindowPos(hwnd, 0, 0, 0, wwidth, 8 + tiheight + bheight + borheight,
		 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    
    /* Position the buttons */
    bpos = (wwidth - bwidth * buttons) / 2;
    GetWindowRect16(GetDlgItem(hwnd, 1), &rect);
    for (buttons = i = 0; i < 7; i++) {
      /* some arithmetic to get the right order for YesNoCancel windows */
      hItem = GetDlgItem(hwnd, (i + 5) % 7 + 1);
      if (GetWindowLong32A(hItem, GWL_STYLE) & WS_VISIBLE) {
	if (buttons++ == ((lpmb->type & MB_DEFMASK) >> 8)) {
	  SetFocus32(hItem);
	  SendMessage32A( hItem, BM_SETSTYLE32, BS_DEFPUSHBUTTON, TRUE );
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
 *           MessageBox16   (USER.1)
 */
INT16 MessageBox16( HWND16 hwnd, LPCSTR text, LPCSTR title, UINT16 type )
{
    return MessageBox32A( hwnd, text, title, type );
}

/**************************************************************************
 *           MessageBox32A   (USER32.390)
 */
INT32 MessageBox32A( HWND32 hWnd, LPCSTR text, LPCSTR title, UINT32 type )
{
    HANDLE16 handle;
    MSGBOX mbox;
    int ret;

    mbox.title = title;
    mbox.text  = text;
    mbox.type  = type;

    handle = SYSRES_LoadResource( SYSRES_DIALOG_MSGBOX );
    if (!handle) return 0;
    ret = DialogBoxIndirectParam16( WIN_GetWindowInstance(hWnd),
                                  handle, hWnd,
                                  MODULE_GetWndProcEntry16("SystemMessageBoxProc"),
                                  (LONG)&mbox );
    SYSRES_FreeResource( handle );
    return ret;
}

/**************************************************************************
 *           MessageBox32W   (USER32.395)
 */
INT32 MessageBox32W( HWND32 hWnd, LPCWSTR text, LPCWSTR title, UINT32 type )
{
    HANDLE16 handle;
    MSGBOX mbox;
    int ret;

    mbox.title = HEAP_strdupWtoA( GetProcessHeap(), 0, title );
    mbox.text  = HEAP_strdupWtoA( GetProcessHeap(), 0, text );
    mbox.type  = type;

    fprintf(stderr,"MessageBox(%s,%s)\n",mbox.text,mbox.title);
    handle = SYSRES_LoadResource( SYSRES_DIALOG_MSGBOX );
    if (!handle) return 0;
    ret = DialogBoxIndirectParam16( WIN_GetWindowInstance(hWnd),
                                  handle, hWnd,
                                  MODULE_GetWndProcEntry16("SystemMessageBoxProc"),
                                  (LONG)&mbox );
    SYSRES_FreeResource( handle );
    HeapFree( GetProcessHeap(), 0, mbox.title );
    HeapFree( GetProcessHeap(), 0, mbox.text );
    return ret;
}

/**************************************************************************
 *			FatalAppExit  [USER.137]
 */

void FatalAppExit(UINT fuAction, LPCSTR str)
{
  MessageBox16(0, str, NULL, MB_SYSTEMMODAL | MB_OK);
  TASK_KillCurrentTask(0);
}
