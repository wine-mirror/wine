/*
 * Message boxes
 *
 * Copyright 1995 Bernd Schmidt
 */

#include <string.h>

#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "dlgs.h"
#include "heap.h"
#include "ldt.h"
#include "debugtools.h"
#include "debugstr.h"
#include "tweak.h"

DEFAULT_DEBUG_CHANNEL(dialog)

#define MSGBOX_IDICON 1088
#define MSGBOX_IDTEXT 100

static HFONT MSGBOX_OnInit(HWND hwnd, LPMSGBOXPARAMSA lpmb)
{
    static HFONT hFont = 0, hPrevFont = 0;
    RECT rect;
    HWND hItem;
    HDC hdc;
    int i, buttons;
    int bspace, bw, bh, theight, tleft, wwidth, wheight, bpos;
    int borheight, borwidth, iheight, ileft, iwidth, twidth, tiheight;
    
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
	SendDlgItemMessageA (hwnd, MSGBOX_IDTEXT, WM_SETFONT, (WPARAM)hFont, 0);
    }
    if (lpmb->lpszCaption) SetWindowTextA(hwnd, lpmb->lpszCaption);
    SetWindowTextA(GetDlgItem(hwnd, MSGBOX_IDTEXT), lpmb->lpszText);
    /* Hide not selected buttons */
    switch(lpmb->dwStyle & MB_TYPEMASK) {
    case MB_OK:
	ShowWindow(GetDlgItem(hwnd, IDCANCEL), SW_HIDE);
	/* fall through */
    case MB_OKCANCEL:
	ShowWindow(GetDlgItem(hwnd, IDABORT), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDRETRY), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDIGNORE), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDYES), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDNO), SW_HIDE);
	break;
    case MB_ABORTRETRYIGNORE:
	ShowWindow(GetDlgItem(hwnd, IDOK), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDCANCEL), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDYES), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDNO), SW_HIDE);
	break;
    case MB_YESNO:
	ShowWindow(GetDlgItem(hwnd, IDCANCEL), SW_HIDE);
	/* fall through */
    case MB_YESNOCANCEL:
	ShowWindow(GetDlgItem(hwnd, IDOK), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDABORT), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDRETRY), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDIGNORE), SW_HIDE);
	break;
    case MB_RETRYCANCEL:
	ShowWindow(GetDlgItem(hwnd, IDOK), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDABORT), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDIGNORE), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDYES), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDNO), SW_HIDE);
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
    borwidth  = rect.right - rect.left;
    GetClientRect(hwnd, &rect);
    borheight -= rect.bottom - rect.top;
    borwidth  -= rect.right - rect.left;
    
    /* Get the icon height */
    GetWindowRect(GetDlgItem(hwnd, MSGBOX_IDICON), &rect);
    MapWindowPoints(0, hwnd, (LPPOINT)&rect, 2);
    iheight = rect.bottom - rect.top;
    ileft = rect.left;
    iwidth = rect.right - ileft;
    
    hdc = GetDC(hwnd);
    if (hFont)
	hPrevFont = SelectObject(hdc, hFont);
    
    /* Get the number of visible buttons and their size */
    bh = bw = 1; /* Minimum button sizes */
    for (buttons = 0, i = 1; i < 8; i++)
    {
	hItem = GetDlgItem(hwnd, i);
	if (GetWindowLongA(hItem, GWL_STYLE) & WS_VISIBLE)
	{
	    char buttonText[1024];
	    int w, h;
	    buttons++;
	    if (GetWindowTextA(hItem, buttonText, sizeof buttonText))
	    {
		DrawTextA( hdc, buttonText, -1, &rect, DT_LEFT | DT_EXPANDTABS | DT_CALCRECT);
		h = rect.bottom - rect.top;
		w = rect.right - rect.left;
		if (h > bh) bh = h;
		if (w > bw)  bw = w ;
	    }
	}
    }
    bw = MAX(bw, bh * 2);
    /* Button white space */
    bh = bh * 2;
    bw = bw * 2;
    bspace = bw/3; /* Space between buttons */
    
    /* Get the text size */
    GetClientRect(GetDlgItem(hwnd, MSGBOX_IDTEXT), &rect);
    rect.top = rect.left = rect.bottom = 0;
    DrawTextA( hdc, lpmb->lpszText, -1, &rect,
	       DT_LEFT | DT_EXPANDTABS | DT_WORDBREAK | DT_CALCRECT);
    /* Min text width corresponds to space for the buttons */
    tleft = 2 * ileft + iwidth;
    twidth = MAX((bw + bspace) * buttons + bspace - tleft, rect.right);
    theight = rect.bottom;
    
    if (hFont)
	SelectObject(hdc, hPrevFont);
    ReleaseDC(hItem, hdc);
    
    tiheight = 16 + MAX(iheight, theight);
    wwidth  = tleft + twidth + ileft + borwidth;
    wheight = 8 + tiheight + bh + borheight;
    
    /* Resize the window */
    SetWindowPos(hwnd, 0, 0, 0, wwidth, wheight,
		 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    
    /* Position the icon */
    SetWindowPos(GetDlgItem(hwnd, MSGBOX_IDICON), 0, ileft, (tiheight - iheight) / 2, 0, 0,
		 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    
    /* Position the text */
    SetWindowPos(GetDlgItem(hwnd, MSGBOX_IDTEXT), 0, tleft, (tiheight - theight) / 2, twidth, theight,
		 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    
    /* Position the buttons */
    bpos = (wwidth - (bw + bspace) * buttons + bspace) / 2;
    for (buttons = i = 0; i < 7; i++) {
	/* some arithmetic to get the right order for YesNoCancel windows */
	hItem = GetDlgItem(hwnd, (i + 5) % 7 + 1);
	if (GetWindowLongA(hItem, GWL_STYLE) & WS_VISIBLE) {
	    if (buttons++ == ((lpmb->dwStyle & MB_DEFMASK) >> 8)) {
		SetFocus(hItem);
		SendMessageA( hItem, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE );
	    }
	    SetWindowPos(hItem, 0, bpos, tiheight, bw, bh,
			 SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOREDRAW);
	    bpos += bw + bspace;
	}
    }
    return hFont;
}


/**************************************************************************
 *           MSGBOX_DlgProc
 *
 * Dialog procedure for message boxes.
 */
static LRESULT CALLBACK MSGBOX_DlgProc( HWND hwnd, UINT message,
                                        WPARAM wParam, LPARAM lParam )
{  
  static HFONT hFont;
  switch(message) {
   case WM_INITDIALOG:
    hFont = MSGBOX_OnInit(hwnd, (LPMSGBOXPARAMSA)lParam);
    return 0;
    
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
      EndDialog(hwnd, wParam);
      if (hFont)
	  DeleteObject(hFont);
      break;
    }

   default:
     /* Ok. Ignore all the other messages */
     TRACE("Message number %i is being ignored.\n", message);
    break;
  }
  return 0;
}


/**************************************************************************
 *           MessageBox16   (USER.1)
 */
INT16 WINAPI MessageBox16( HWND16 hwnd, LPCSTR text, LPCSTR title, UINT16 type)
{
    WARN("Messagebox\n");
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

    WARN("Messagebox\n");

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
    return DialogBoxIndirectParamA( GetWindowLongA(hWnd,GWL_HINSTANCE), template,
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
    
    WARN("Messagebox\n");

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
    WARN("Messagebox\n");
    /* ignore language id for now */
    return MessageBoxA(hWnd,text,title,type);
}

/**************************************************************************
 *           MessageBoxEx32W   (USER32.393)
 */
INT WINAPI MessageBoxExW( HWND hWnd, LPCWSTR text, LPCWSTR title,
                              UINT type, WORD langid )
{
    WARN("Messagebox\n");
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

    WARN("Messagebox\n");    
    
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

    WARN("Messagebox\n");

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
    WARN("Messagebox\n");

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
    WARN("AppExit\n");
    FatalAppExitA( action, str );
}


/**************************************************************************
 *           FatalAppExit32A   (KERNEL32.108)
 */
void WINAPI FatalAppExitA( UINT action, LPCSTR str )
{
    WARN("AppExit\n");
    MessageBoxA( 0, str, NULL, MB_SYSTEMMODAL | MB_OK );
    ExitProcess(0);
}


/**************************************************************************
 *           FatalAppExit32W   (KERNEL32.109)
 */
void WINAPI FatalAppExitW( UINT action, LPCWSTR str )
{
    WARN("AppExit\n");
    MessageBoxW( 0, str, NULL, MB_SYSTEMMODAL | MB_OK );
    ExitProcess(0);
}


