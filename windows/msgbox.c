/*
 * Message boxes
 *
 * Copyright 1995 Bernd Schmidt
 */

#include "windows.h"
#include "dlgs.h"
#include "heap.h"
#include "module.h"
#include "win.h"
#include "resource.h"
#include "task.h"
#include "debug.h"
#include "debugstr.h"
#include "tweak.h"

/**************************************************************************
 *           MSGBOX_DlgProc
 *
 * Dialog procedure for message boxes.
 */
static LRESULT CALLBACK MSGBOX_DlgProc( HWND32 hwnd, UINT32 message,
                                        WPARAM32 wParam, LPARAM lParam )
{
  static HFONT32 hFont = 0;
  LPMSGBOXPARAMS32A lpmb;

  RECT32 rect, textrect;
  HWND32 hItem;
  HDC32 hdc;
  LRESULT lRet;
  int i, buttons, bwidth, bheight, theight, wwidth, bpos;
  int borheight, iheight, tiheight;
  
  switch(message) {
   case WM_INITDIALOG:
    lpmb = (LPMSGBOXPARAMS32A)lParam;
    if (TWEAK_WineLook >= WIN95_LOOK) {
	NONCLIENTMETRICS32A nclm;
	INT32 i;
	nclm.cbSize = sizeof(NONCLIENTMETRICS32A);
	SystemParametersInfo32A (SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
	hFont = CreateFontIndirect32A (&nclm.lfMessageFont);
        /* set button font */
	for (i=1; i < 8; i++)
	    SendDlgItemMessage32A (hwnd, i, WM_SETFONT, (WPARAM32)hFont, 0);
        /* set text font */
	SendDlgItemMessage32A (hwnd, 100, WM_SETFONT, (WPARAM32)hFont, 0);
    }
    if (lpmb->lpszCaption) SetWindowText32A(hwnd, lpmb->lpszCaption);
    SetWindowText32A(GetDlgItem32(hwnd, 100), lpmb->lpszText);
    /* Hide not selected buttons */
    switch(lpmb->dwStyle & MB_TYPEMASK) {
     case MB_OK:
      ShowWindow32(GetDlgItem32(hwnd, 2), SW_HIDE);
      /* fall through */
     case MB_OKCANCEL:
      ShowWindow32(GetDlgItem32(hwnd, 3), SW_HIDE);
      ShowWindow32(GetDlgItem32(hwnd, 4), SW_HIDE);
      ShowWindow32(GetDlgItem32(hwnd, 5), SW_HIDE);
      ShowWindow32(GetDlgItem32(hwnd, 6), SW_HIDE);
      ShowWindow32(GetDlgItem32(hwnd, 7), SW_HIDE);
      break;
     case MB_ABORTRETRYIGNORE:
      ShowWindow32(GetDlgItem32(hwnd, 1), SW_HIDE);
      ShowWindow32(GetDlgItem32(hwnd, 2), SW_HIDE);
      ShowWindow32(GetDlgItem32(hwnd, 6), SW_HIDE);
      ShowWindow32(GetDlgItem32(hwnd, 7), SW_HIDE);
      break;
     case MB_YESNO:
      ShowWindow32(GetDlgItem32(hwnd, 2), SW_HIDE);
      /* fall through */
     case MB_YESNOCANCEL:
      ShowWindow32(GetDlgItem32(hwnd, 1), SW_HIDE);
      ShowWindow32(GetDlgItem32(hwnd, 3), SW_HIDE);
      ShowWindow32(GetDlgItem32(hwnd, 4), SW_HIDE);
      ShowWindow32(GetDlgItem32(hwnd, 5), SW_HIDE);
      break;
    }
    /* Set the icon */
    switch(lpmb->dwStyle & MB_ICONMASK) {
     case MB_ICONEXCLAMATION:
      SendDlgItemMessage16(hwnd, stc1, STM_SETICON16,
                           (WPARAM16)LoadIcon16(0, IDI_EXCLAMATION16), 0);
      break;
     case MB_ICONQUESTION:
      SendDlgItemMessage16(hwnd, stc1, STM_SETICON16,
                           (WPARAM16)LoadIcon16(0, IDI_QUESTION16), 0);
      break;
     case MB_ICONASTERISK:
      SendDlgItemMessage16(hwnd, stc1, STM_SETICON16,
                           (WPARAM16)LoadIcon16(0, IDI_ASTERISK16), 0);
      break;
     case MB_ICONHAND:
     default:
      SendDlgItemMessage16(hwnd, stc1, STM_SETICON16,
                           (WPARAM16)LoadIcon16(0, IDI_HAND16), 0);
      break;
    }
    
    /* Position everything */
    GetWindowRect32(hwnd, &rect);
    borheight = rect.bottom - rect.top;
    wwidth = rect.right - rect.left;
    GetClientRect32(hwnd, &rect);
    borheight -= rect.bottom - rect.top;

    /* Get the icon height */
    GetWindowRect32(GetDlgItem32(hwnd, 1088), &rect);
    iheight = rect.bottom - rect.top;
    
    /* Get the number of visible buttons and their width */
    GetWindowRect32(GetDlgItem32(hwnd, 2), &rect);
    bheight = rect.bottom - rect.top;
    bwidth = rect.left;
    GetWindowRect32(GetDlgItem32(hwnd, 1), &rect);
    bwidth -= rect.left;
    for (buttons = 0, i = 1; i < 8; i++)
    {
      hItem = GetDlgItem32(hwnd, i);
      if (GetWindowLong32A(hItem, GWL_STYLE) & WS_VISIBLE) buttons++;
    }
    
    /* Get the text size */
    hItem = GetDlgItem32(hwnd, 100);
    GetWindowRect32(hItem, &textrect);
    MapWindowPoints32(0, hwnd, (LPPOINT32)&textrect, 2);
    
    GetClientRect32(hItem, &rect);
    hdc = GetDC32(hItem);
    lRet = DrawText32A( hdc, lpmb->lpszText, -1, &rect,
                        DT_LEFT | DT_EXPANDTABS | DT_WORDBREAK | DT_CALCRECT);
    theight = rect.bottom  - rect.top;
    tiheight = 16 + MAX(iheight, theight);
    ReleaseDC32(hItem, hdc);
    
    /* Position the text */
    SetWindowPos32(hItem, 0, textrect.left, (tiheight - theight) / 2, 
		   rect.right - rect.left, theight,
		   SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    
    /* Position the icon */
    hItem = GetDlgItem32(hwnd, 1088);
    GetWindowRect32(hItem, &rect);
    MapWindowPoints32(0, hwnd, (LPPOINT32)&rect, 2);
    SetWindowPos32(hItem, 0, rect.left, (tiheight - iheight) / 2, 0, 0,
		   SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    
    /* Resize the window */
    SetWindowPos32(hwnd, 0, 0, 0, wwidth, 8 + tiheight + bheight + borheight,
		   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    
    /* Position the buttons */
    bpos = (wwidth - bwidth * buttons) / 2;
    GetWindowRect32(GetDlgItem32(hwnd, 1), &rect);
    for (buttons = i = 0; i < 7; i++) {
      /* some arithmetic to get the right order for YesNoCancel windows */
      hItem = GetDlgItem32(hwnd, (i + 5) % 7 + 1);
      if (GetWindowLong32A(hItem, GWL_STYLE) & WS_VISIBLE) {
	if (buttons++ == ((lpmb->dwStyle & MB_DEFMASK) >> 8)) {
	  SetFocus32(hItem);
	  SendMessage32A( hItem, BM_SETSTYLE32, BS_DEFPUSHBUTTON, TRUE );
	}
	SetWindowPos32(hItem, 0, bpos, tiheight, 0, 0,
		       SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOREDRAW);
	bpos += bwidth;
      }
    }
    return 0;
    break;
    
   case WM_COMMAND:
    switch (wParam)
    {
     case IDOK:
     case IDCANCEL:
     case IDABORT:
     case IDRETRY:
     case IDIGNORE:
     case IDYES:
     case IDNO:
      if ((TWEAK_WineLook > WIN31_LOOK) && hFont)
        DeleteObject32 (hFont);
      EndDialog32(hwnd, wParam);
      break;
    }

   default:
     /* Ok. Ignore all the other messages */
     TRACE (dialog, "Message number %i is being ignored.\n", message);
    break;
  }
  return 0;
}


/**************************************************************************
 *           MessageBox16   (USER.1)
 */
INT16 WINAPI MessageBox16( HWND16 hwnd, LPCSTR text, LPCSTR title, UINT16 type)
{
    WARN(dialog,"Messagebox\n");
    return MessageBox32A( hwnd, text, title, type );
}


/**************************************************************************
 *           MessageBox32A   (USER32.391)
 *
 * NOTES
 *   The WARN is here to help debug erroneous MessageBoxes
 *   Use: -debugmsg warn+dialog,+relay
 */
INT32 WINAPI MessageBox32A(HWND32 hWnd, LPCSTR text, LPCSTR title, UINT32 type)
{
    MSGBOXPARAMS32A mbox;
    WARN(dialog,"Messagebox\n");
    if (!text) text="<WINE-NULL>";
    if (!title)
      title="Error";
    mbox.lpszCaption = title;
    mbox.lpszText  = text;
    mbox.dwStyle  = type;
    return DialogBoxIndirectParam32A( WIN_GetWindowInstance(hWnd),
                                      SYSRES_GetResPtr( SYSRES_DIALOG_MSGBOX ),
                                      hWnd, MSGBOX_DlgProc, (LPARAM)&mbox );
}


/**************************************************************************
 *           MessageBox32W   (USER32.396)
 */
INT32 WINAPI MessageBox32W( HWND32 hwnd, LPCWSTR text, LPCWSTR title,
                            UINT32 type )
{
    LPSTR titleA = HEAP_strdupWtoA( GetProcessHeap(), 0, title );
    LPSTR textA  = HEAP_strdupWtoA( GetProcessHeap(), 0, text );
    INT32 ret;
    
    WARN(dialog,"Messagebox\n");

    ret = MessageBox32A( hwnd, textA, titleA, type );
    HeapFree( GetProcessHeap(), 0, titleA );
    HeapFree( GetProcessHeap(), 0, textA );
    return ret;
}


/**************************************************************************
 *           MessageBoxEx32A   (USER32.392)
 */
INT32 WINAPI MessageBoxEx32A( HWND32 hWnd, LPCSTR text, LPCSTR title,
                              UINT32 type, WORD langid )
{
    WARN(dialog,"Messagebox\n");
    /* ignore language id for now */
    return MessageBox32A(hWnd,text,title,type);
}

/**************************************************************************
 *           MessageBoxEx32W   (USER32.393)
 */
INT32 WINAPI MessageBoxEx32W( HWND32 hWnd, LPCWSTR text, LPCWSTR title,
                              UINT32 type, WORD langid )
{
    WARN(dialog,"Messagebox\n");
    /* ignore language id for now */
    return MessageBox32W(hWnd,text,title,type);
}

/**************************************************************************
 *           MessageBoxIndirect16   (USER.827)
 */
INT16 WINAPI MessageBoxIndirect16( LPMSGBOXPARAMS16 msgbox )
{
    MSGBOXPARAMS32A msgbox32;
    WARN(dialog,"Messagebox\n");    
    
    msgbox32.cbSize		= msgbox->cbSize;
    msgbox32.hwndOwner		= msgbox->hwndOwner;
    msgbox32.hInstance		= msgbox->hInstance;
    msgbox32.lpszText		= PTR_SEG_TO_LIN(msgbox->lpszText);
    msgbox32.lpszCaption	= PTR_SEG_TO_LIN(msgbox->lpszCaption);
    msgbox32.dwStyle		= msgbox->dwStyle;
    msgbox32.lpszIcon		= PTR_SEG_TO_LIN(msgbox->lpszIcon);
    msgbox32.dwContextHelpId	= msgbox->dwContextHelpId;
    msgbox32.lpfnMsgBoxCallback	= msgbox->lpfnMsgBoxCallback;
    msgbox32.dwLanguageId	= msgbox->dwLanguageId;

    return DialogBoxIndirectParam32A( msgbox32.hInstance,
                                      SYSRES_GetResPtr( SYSRES_DIALOG_MSGBOX ),
                                      msgbox32.hwndOwner, MSGBOX_DlgProc,
                                      (LPARAM)&msgbox32 );
}

/**************************************************************************
 *           MessageBoxIndirect32A   (USER32.394)
 */
INT32 WINAPI MessageBoxIndirect32A( LPMSGBOXPARAMS32A msgbox )
{
    WARN(dialog,"Messagebox\n");
    return DialogBoxIndirectParam32A( msgbox->hInstance,
   				      SYSRES_GetResPtr( SYSRES_DIALOG_MSGBOX ),
                                      msgbox->hwndOwner, MSGBOX_DlgProc,
				      (LPARAM)msgbox );
}

/**************************************************************************
 *           MessageBoxIndirect32W   (USER32.395)
 */
INT32 WINAPI MessageBoxIndirect32W( LPMSGBOXPARAMS32W msgbox )
{
    MSGBOXPARAMS32A	msgboxa;
    WARN(dialog,"Messagebox\n");

    memcpy(&msgboxa,msgbox,sizeof(msgboxa));
    if (msgbox->lpszCaption)	
      lstrcpyWtoA(msgboxa.lpszCaption,msgbox->lpszCaption);
    if (msgbox->lpszText)	
      lstrcpyWtoA(msgboxa.lpszText,msgbox->lpszText);

    return MessageBoxIndirect32A(&msgboxa);
}


/**************************************************************************
 *           FatalAppExit16   (KERNEL.137)
 */
void WINAPI FatalAppExit16( UINT16 action, LPCSTR str )
{
    WARN(dialog,"AppExit\n");
    FatalAppExit32A( action, str );
}


/**************************************************************************
 *           FatalAppExit32A   (KERNEL32.108)
 */
void WINAPI FatalAppExit32A( UINT32 action, LPCSTR str )
{
    WARN(dialog,"AppExit\n");
    MessageBox32A( 0, str, NULL, MB_SYSTEMMODAL | MB_OK );
    ExitProcess(0);
}


/**************************************************************************
 *           FatalAppExit32W   (KERNEL32.109)
 */
void WINAPI FatalAppExit32W( UINT32 action, LPCWSTR str )
{
    WARN(dialog,"AppExit\n");
    MessageBox32W( 0, str, NULL, MB_SYSTEMMODAL | MB_OK );
    ExitProcess(0);
}


