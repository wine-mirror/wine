/*
 * Message boxes
 *
 * Copyright 1995 Bernd Schmidt
 */

#include <string.h>
#include "wine/winuser16.h"
#include "dlgs.h"
#include "heap.h"
#include "module.h"
#include "win.h"
#include "resource.h"
#include "task.h"
#include "debug.h"
#include "debugstr.h"
#include "tweak.h"

DEFAULT_DEBUG_CHANNEL(dialog)

/**************************************************************************
 *           MSGBOX_DlgProc
 *
 * Dialog procedure for message boxes.
 */
static LRESULT CALLBACK MSGBOX_DlgProc( HWND hwnd, UINT message,
                                        WPARAM wParam, LPARAM lParam )
{
  static HFONT hFont = 0;
  LPMSGBOXPARAMSA lpmb;

  RECT rect, textrect;
  HWND hItem;
  HDC hdc;
  LRESULT lRet;
  int i, buttons, bwidth, bheight, theight, wwidth, bpos;
  int borheight, iheight, tiheight;
  
  switch(message) {
   case WM_INITDIALOG:
    lpmb = (LPMSGBOXPARAMSA)lParam;
    if (TWEAK_WineLook >= WIN95_LOOK) {
	NONCLIENTMETRICSA nclm;
	INT i;
	nclm.cbSize = sizeof(NONCLIENTMETRICSA);
	SystemParametersInfoA (SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
	hFont = CreateFontIndirectA (&nclm.lfMessageFont);
        /* set button font */
	for (i=1; i < 8; i++)
	    SendDlgItemMessageA (hwnd, i, WM_SETFONT, (WPARAM)hFont, 0);
        /* set text font */
	SendDlgItemMessageA (hwnd, 100, WM_SETFONT, (WPARAM)hFont, 0);
    }
    if (lpmb->lpszCaption) SetWindowTextA(hwnd, lpmb->lpszCaption);
    SetWindowTextA(GetDlgItem(hwnd, 100), lpmb->lpszText);
    /* Hide not selected buttons */
    switch(lpmb->dwStyle & MB_TYPEMASK) {
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
    for (buttons = 0, i = 1; i < 8; i++)
    {
      hItem = GetDlgItem(hwnd, i);
      if (GetWindowLongA(hItem, GWL_STYLE) & WS_VISIBLE) buttons++;
    }
    
    /* Get the text size */
    hItem = GetDlgItem(hwnd, 100);
    GetWindowRect(hItem, &textrect);
    MapWindowPoints(0, hwnd, (LPPOINT)&textrect, 2);
    
    GetClientRect(hItem, &rect);
    hdc = GetDC(hItem);
    lRet = DrawTextA( hdc, lpmb->lpszText, -1, &rect,
                        DT_LEFT | DT_EXPANDTABS | DT_WORDBREAK | DT_CALCRECT);
    theight = rect.bottom  - rect.top;
    tiheight = 16 + MAX(iheight, theight);
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
      if (GetWindowLongA(hItem, GWL_STYLE) & WS_VISIBLE) {
	if (buttons++ == ((lpmb->dwStyle & MB_DEFMASK) >> 8)) {
	  SetFocus(hItem);
	  SendMessageA( hItem, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE );
	}
	SetWindowPos(hItem, 0, bpos, tiheight, 0, 0,
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
        DeleteObject (hFont);
      EndDialog(hwnd, wParam);
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
    return MessageBoxA( hwnd, text, title, type );
}


/**************************************************************************
 *           MessageBox32A   (USER32.391)
 *
 * NOTES
 *   The WARN is here to help debug erroneous MessageBoxes
 *   Use: -debugmsg warn+dialog,+relay
 */
INT WINAPI MessageBoxA(HWND hWnd, LPCSTR text, LPCSTR title, UINT type)
{
    LPVOID template;
    HRSRC hRes;
    MSGBOXPARAMSA mbox;

    WARN(dialog,"Messagebox\n");

    if(!(hRes = FindResourceA(GetModuleHandleA("USER32"), "MSGBOX", RT_DIALOGA)))
        return 0;
    if(!(template = (LPVOID)LoadResource(GetModuleHandleA("USER32"), hRes)))
        return 0;

    if (!text) text="<WINE-NULL>";
    if (!title)
      title="Error";
    mbox.lpszCaption = title;
    mbox.lpszText  = text;
    mbox.dwStyle  = type;
    return DialogBoxIndirectParamA( WIN_GetWindowInstance(hWnd), template,
                                      hWnd, (DLGPROC)MSGBOX_DlgProc, (LPARAM)&mbox );
}


/**************************************************************************
 *           MessageBox32W   (USER32.396)
 */
INT WINAPI MessageBoxW( HWND hwnd, LPCWSTR text, LPCWSTR title,
                            UINT type )
{
    LPSTR titleA = HEAP_strdupWtoA( GetProcessHeap(), 0, title );
    LPSTR textA  = HEAP_strdupWtoA( GetProcessHeap(), 0, text );
    INT ret;
    
    WARN(dialog,"Messagebox\n");

    ret = MessageBoxA( hwnd, textA, titleA, type );
    HeapFree( GetProcessHeap(), 0, titleA );
    HeapFree( GetProcessHeap(), 0, textA );
    return ret;
}


/**************************************************************************
 *           MessageBoxEx32A   (USER32.392)
 */
INT WINAPI MessageBoxExA( HWND hWnd, LPCSTR text, LPCSTR title,
                              UINT type, WORD langid )
{
    WARN(dialog,"Messagebox\n");
    /* ignore language id for now */
    return MessageBoxA(hWnd,text,title,type);
}

/**************************************************************************
 *           MessageBoxEx32W   (USER32.393)
 */
INT WINAPI MessageBoxExW( HWND hWnd, LPCWSTR text, LPCWSTR title,
                              UINT type, WORD langid )
{
    WARN(dialog,"Messagebox\n");
    /* ignore language id for now */
    return MessageBoxW(hWnd,text,title,type);
}

/**************************************************************************
 *           MessageBoxIndirect16   (USER.827)
 */
INT16 WINAPI MessageBoxIndirect16( LPMSGBOXPARAMS16 msgbox )
{
    LPVOID template;
    HRSRC hRes;
    MSGBOXPARAMSA msgbox32;

    WARN(dialog,"Messagebox\n");    
    
    if(!(hRes = FindResourceA(GetModuleHandleA("USER32"), "MSGBOX", RT_DIALOGA)))
        return 0;
    if(!(template = (LPVOID)LoadResource(GetModuleHandleA("USER32"), hRes)))
        return 0;

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

    return DialogBoxIndirectParamA( msgbox32.hInstance, template,
                                      msgbox32.hwndOwner, (DLGPROC)MSGBOX_DlgProc,
                                      (LPARAM)&msgbox32 );
}

/**************************************************************************
 *           MessageBoxIndirect32A   (USER32.394)
 */
INT WINAPI MessageBoxIndirectA( LPMSGBOXPARAMSA msgbox )
{
    LPVOID template;
    HRSRC hRes;

    WARN(dialog,"Messagebox\n");

    if(!(hRes = FindResourceA(GetModuleHandleA("USER32"), "MSGBOX", RT_DIALOGA)))
        return 0;
    if(!(template = (LPVOID)LoadResource(GetModuleHandleA("USER32"), hRes)))
        return 0;

    return DialogBoxIndirectParamA( msgbox->hInstance, template,
                                      msgbox->hwndOwner, (DLGPROC)MSGBOX_DlgProc,
				      (LPARAM)msgbox );
}

/**************************************************************************
 *           MessageBoxIndirect32W   (USER32.395)
 */
INT WINAPI MessageBoxIndirectW( LPMSGBOXPARAMSW msgbox )
{
    MSGBOXPARAMSA	msgboxa;
    WARN(dialog,"Messagebox\n");

    memcpy(&msgboxa,msgbox,sizeof(msgboxa));
    if (msgbox->lpszCaption)	
      lstrcpyWtoA((LPSTR)msgboxa.lpszCaption,msgbox->lpszCaption);
    if (msgbox->lpszText)	
      lstrcpyWtoA((LPSTR)msgboxa.lpszText,msgbox->lpszText);

    return MessageBoxIndirectA(&msgboxa);
}


/**************************************************************************
 *           FatalAppExit16   (KERNEL.137)
 */
void WINAPI FatalAppExit16( UINT16 action, LPCSTR str )
{
    WARN(dialog,"AppExit\n");
    FatalAppExitA( action, str );
}


/**************************************************************************
 *           FatalAppExit32A   (KERNEL32.108)
 */
void WINAPI FatalAppExitA( UINT action, LPCSTR str )
{
    WARN(dialog,"AppExit\n");
    MessageBoxA( 0, str, NULL, MB_SYSTEMMODAL | MB_OK );
    ExitProcess(0);
}


/**************************************************************************
 *           FatalAppExit32W   (KERNEL32.109)
 */
void WINAPI FatalAppExitW( UINT action, LPCWSTR str )
{
    WARN(dialog,"AppExit\n");
    MessageBoxW( 0, str, NULL, MB_SYSTEMMODAL | MB_OK );
    ExitProcess(0);
}


