/*
 * Message boxes
 *
 * Copyright 1995 Bernd Schmidt
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>

#include "windef.h"
#include "wingdi.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "winternl.h"
#include "dlgs.h"
#include "heap.h"
#include "user.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dialog);

#define MSGBOX_IDICON 1088
#define MSGBOX_IDTEXT 100
#define IDS_ERROR     2

static HFONT MSGBOX_OnInit(HWND hwnd, LPMSGBOXPARAMSW lpmb)
{
    HFONT hFont = 0, hPrevFont = 0;
    RECT rect;
    HWND hItem;
    HDC hdc;
    int i, buttons;
    int bspace, bw, bh, theight, tleft, wwidth, wheight, bpos;
    int borheight, borwidth, iheight, ileft, iwidth, twidth, tiheight;
    LPCWSTR lpszText;
    WCHAR buf[256];

    if (TWEAK_WineLook >= WIN95_LOOK) {
	NONCLIENTMETRICSW nclm;
	nclm.cbSize = sizeof(nclm);
	SystemParametersInfoW (SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
	hFont = CreateFontIndirectW (&nclm.lfMessageFont);
	/* set button font */
	for (i=1; i < 8; i++)
	    SendDlgItemMessageW (hwnd, i, WM_SETFONT, (WPARAM)hFont, 0);
	/* set text font */
	SendDlgItemMessageW (hwnd, MSGBOX_IDTEXT, WM_SETFONT, (WPARAM)hFont, 0);
    }
    if (HIWORD(lpmb->lpszCaption)) {
       SetWindowTextW(hwnd, lpmb->lpszCaption);
    } else {
       UINT res_id = LOWORD(lpmb->lpszCaption);
       if (res_id)
       {
           if (LoadStringW(lpmb->hInstance, res_id, buf, 256))
               SetWindowTextW(hwnd, buf);
       }
       else
       {
           if (LoadStringW(0, IDS_ERROR, buf, 256))
               SetWindowTextW(hwnd, buf);
       }
    }
    if (HIWORD(lpmb->lpszText)) {
       lpszText = lpmb->lpszText;
    } else {
       lpszText = buf;
       if (!LoadStringW(lpmb->hInstance, LOWORD(lpmb->lpszText), buf, 256))
	  *buf = 0;	/* FIXME ?? */
    }
    SetWindowTextW(GetDlgItem(hwnd, MSGBOX_IDTEXT), lpszText);

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
	SendDlgItemMessageW(hwnd, stc1, STM_SETICON,
			    (WPARAM)LoadIconW(0, IDI_EXCLAMATIONW), 0);
	break;
    case MB_ICONQUESTION:
	SendDlgItemMessageW(hwnd, stc1, STM_SETICON,
			    (WPARAM)LoadIconW(0, IDI_QUESTIONW), 0);
	break;
    case MB_ICONASTERISK:
	SendDlgItemMessageW(hwnd, stc1, STM_SETICON,
			    (WPARAM)LoadIconW(0, IDI_ASTERISKW), 0);
	break;
    case MB_ICONHAND:
      SendDlgItemMessageW(hwnd, stc1, STM_SETICON,
			    (WPARAM)LoadIconW(0, IDI_HANDW), 0);
      break;
    case MB_USERICON:
      SendDlgItemMessageW(hwnd, stc1, STM_SETICON,
			  (WPARAM)LoadIconW(lpmb->hInstance, lpmb->lpszIcon), 0);
      break;
    default:
	/* By default, Windows 95/98/NT do not associate an icon to message boxes.
         * So wine should do the same.
         */
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
	if (GetWindowLongW(hItem, GWL_STYLE) & WS_VISIBLE)
	{
	    WCHAR buttonText[1024];
	    int w, h;
	    buttons++;
	    if (GetWindowTextW(hItem, buttonText, 1024))
	    {
		DrawTextW( hdc, buttonText, -1, &rect, DT_LEFT | DT_EXPANDTABS | DT_CALCRECT);
		h = rect.bottom - rect.top;
		w = rect.right - rect.left;
		if (h > bh) bh = h;
		if (w > bw)  bw = w ;
	    }
	}
    }
    bw = max(bw, bh * 2);
    /* Button white space */
    bh = bh * 2;
    bw = bw * 2;
    bspace = bw/3; /* Space between buttons */

    /* Get the text size */
    GetClientRect(GetDlgItem(hwnd, MSGBOX_IDTEXT), &rect);
    rect.top = rect.left = rect.bottom = 0;
    DrawTextW( hdc, lpszText, -1, &rect,
	       DT_LEFT | DT_EXPANDTABS | DT_WORDBREAK | DT_CALCRECT);
    /* Min text width corresponds to space for the buttons */
    tleft = 2 * ileft + iwidth;
    twidth = max((bw + bspace) * buttons + bspace - tleft, rect.right);
    theight = rect.bottom;

    if (hFont)
	SelectObject(hdc, hPrevFont);
    ReleaseDC(hItem, hdc);

    tiheight = 16 + max(iheight, theight);
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
	if (GetWindowLongW(hItem, GWL_STYLE) & WS_VISIBLE) {
	    if (buttons++ == ((lpmb->dwStyle & MB_DEFMASK) >> 8)) {
		SetFocus(hItem);
		SendMessageW( hItem, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE );
	    }
	    SetWindowPos(hItem, 0, bpos, tiheight, bw, bh,
			 SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOREDRAW);
	    bpos += bw + bspace;
	}
    }

    /* handle modal MessageBoxes */
    if (lpmb->dwStyle & (MB_TASKMODAL|MB_SYSTEMMODAL))
    {
	FIXME("%s modal msgbox ! Not modal yet.\n",
		lpmb->dwStyle & MB_TASKMODAL ? "task" : "system");
	/* Probably do EnumTaskWindows etc. here for TASKMODAL
	 * and work your way up to the top - I'm lazy (HWND_TOP) */
	SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0,
			SWP_NOSIZE | SWP_NOMOVE);
	if (lpmb->dwStyle & MB_TASKMODAL)
	    /* at least MB_TASKMODAL seems to imply a ShowWindow */
	    ShowWindow(hwnd, SW_SHOW);
    }
    if (lpmb->dwStyle & MB_APPLMODAL)
	FIXME("app modal msgbox ! Not modal yet.\n");

    return hFont;
}


/**************************************************************************
 *           MSGBOX_DlgProc
 *
 * Dialog procedure for message boxes.
 */
static BOOL CALLBACK MSGBOX_DlgProc( HWND hwnd, UINT message,
                                        WPARAM wParam, LPARAM lParam )
{
  HFONT hFont;

  switch(message) {
   case WM_INITDIALOG:
   {
       LPMSGBOXPARAMSW mbp = (LPMSGBOXPARAMSW)lParam;
       SetWindowContextHelpId(hwnd, mbp->dwContextHelpId);
       hFont = MSGBOX_OnInit(hwnd, mbp);
       SetPropA(hwnd, "WINE_MSGBOX_HFONT", (HANDLE)hFont);
       SetPropA(hwnd, "WINE_MSGBOX_HELPCALLBACK", (HANDLE)mbp->lpfnMsgBoxCallback);
       break;
   }

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
      hFont = GetPropA(hwnd, "WINE_MSGBOX_HFONT");
      EndDialog(hwnd, wParam);
      if (hFont)
	  DeleteObject(hFont);
      break;
    }

    case WM_HELP:
    {
        MSGBOXCALLBACK callback = (MSGBOXCALLBACK)GetPropA(hwnd, "WINE_MSGBOX_HELPCALLBACK");
        HELPINFO hi;

        memcpy(&hi, (void *)lParam, sizeof(hi));
        hi.dwContextId = GetWindowContextHelpId(hwnd);

        if (callback)
            callback(&hi);
        else
            SendMessageW(GetWindow(hwnd, GW_OWNER), WM_HELP, 0, (LPARAM)&hi);
	break;
   }

   default:
     /* Ok. Ignore all the other messages */
     TRACE("Message number 0x%04x is being ignored.\n", message);
    break;
  }
  return 0;
}


/**************************************************************************
 *		MessageBoxA (USER32.@)
 *
 * NOTES
 *   The WARN is here to help debug erroneous MessageBoxes
 *   Use: -debugmsg warn+dialog,+relay
 */
INT WINAPI MessageBoxA(HWND hWnd, LPCSTR text, LPCSTR title, UINT type)
{
    return MessageBoxExA(hWnd, text, title, type, LANG_NEUTRAL);
}


/**************************************************************************
 *		MessageBoxW (USER32.@)
 */
INT WINAPI MessageBoxW( HWND hwnd, LPCWSTR text, LPCWSTR title, UINT type )
{
    return MessageBoxExW(hwnd, text, title, type, LANG_NEUTRAL);
}


/**************************************************************************
 *		MessageBoxExA (USER32.@)
 */
INT WINAPI MessageBoxExA( HWND hWnd, LPCSTR text, LPCSTR title,
                              UINT type, WORD langid )
{
    MSGBOXPARAMSA msgbox;

    msgbox.cbSize = sizeof(msgbox);
    msgbox.hwndOwner = hWnd;
    msgbox.hInstance = 0;
    msgbox.lpszText = text;
    msgbox.lpszCaption = title;
    msgbox.dwStyle = type;
    msgbox.lpszIcon = NULL;
    msgbox.dwContextHelpId = 0;
    msgbox.lpfnMsgBoxCallback = NULL;
    msgbox.dwLanguageId = langid;

    return MessageBoxIndirectA(&msgbox);
}

/**************************************************************************
 *		MessageBoxExW (USER32.@)
 */
INT WINAPI MessageBoxExW( HWND hWnd, LPCWSTR text, LPCWSTR title,
                              UINT type, WORD langid )
{
    MSGBOXPARAMSW msgbox;

    msgbox.cbSize = sizeof(msgbox);
    msgbox.hwndOwner = hWnd;
    msgbox.hInstance = 0;
    msgbox.lpszText = text;
    msgbox.lpszCaption = title;
    msgbox.dwStyle = type;
    msgbox.lpszIcon = NULL;
    msgbox.dwContextHelpId = 0;
    msgbox.lpfnMsgBoxCallback = NULL;
    msgbox.dwLanguageId = langid;

    return MessageBoxIndirectW(&msgbox);
}

/**************************************************************************
 *		MessageBoxIndirectA (USER32.@)
 */
INT WINAPI MessageBoxIndirectA( LPMSGBOXPARAMSA msgbox )
{
    MSGBOXPARAMSW msgboxW;
    UNICODE_STRING textW, captionW, iconW;
    int ret;

    if (HIWORD(msgbox->lpszText))
        RtlCreateUnicodeStringFromAsciiz(&textW, msgbox->lpszText);
    else
        textW.Buffer = (LPWSTR)msgbox->lpszText;
    if (HIWORD(msgbox->lpszCaption))
        RtlCreateUnicodeStringFromAsciiz(&captionW, msgbox->lpszCaption);
    else
        captionW.Buffer = (LPWSTR)msgbox->lpszCaption;
    if (HIWORD(msgbox->lpszIcon))
        RtlCreateUnicodeStringFromAsciiz(&iconW, msgbox->lpszIcon);
    else
        captionW.Buffer = (LPWSTR)msgbox->lpszIcon;

    msgboxW.cbSize = sizeof(msgboxW);
    msgboxW.hwndOwner = msgbox->hwndOwner;
    msgboxW.hInstance = msgbox->hInstance;
    msgboxW.lpszText = textW.Buffer;
    msgboxW.lpszCaption = captionW.Buffer;
    msgboxW.dwStyle = msgbox->dwStyle;
    msgboxW.lpszIcon = iconW.Buffer;
    msgboxW.dwContextHelpId = msgbox->dwContextHelpId;
    msgboxW.lpfnMsgBoxCallback = msgbox->lpfnMsgBoxCallback;
    msgboxW.dwLanguageId = msgbox->dwLanguageId;

    ret = MessageBoxIndirectW(&msgboxW);

    if (HIWORD(textW.Buffer)) RtlFreeUnicodeString(&textW);
    if (HIWORD(captionW.Buffer)) RtlFreeUnicodeString(&captionW);
    if (HIWORD(iconW.Buffer)) RtlFreeUnicodeString(&iconW);
    return ret;
}

/**************************************************************************
 *		MessageBoxIndirectW (USER32.@)
 */
INT WINAPI MessageBoxIndirectW( LPMSGBOXPARAMSW msgbox )
{
    LPVOID tmplate;
    HRSRC hRes;
    HMODULE hUser32 = GetModuleHandleA("user32.dll");
    static const WCHAR msg_box_res_nameW[] = { 'M','S','G','B','O','X',0 };
    if (!(hRes = FindResourceExW(hUser32, RT_DIALOGW, msg_box_res_nameW, msgbox->dwLanguageId)))
        return 0;
    if (!(tmplate = (LPVOID)LoadResource(hUser32, hRes)))
        return 0;

    return DialogBoxIndirectParamW(msgbox->hInstance, tmplate, msgbox->hwndOwner,
                                   MSGBOX_DlgProc, (LPARAM)msgbox);
}
