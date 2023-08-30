/*
 * Message boxes
 *
 * Copyright 1995 Bernd Schmidt
 * Copyright 2004 Ivan Leo Puoti, Juan Lang
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winternl.h"
#include "dlgs.h"
#include "user_private.h"
#include "resources.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dialog);
WINE_DECLARE_DEBUG_CHANNEL(msgbox);

struct ThreadWindows
{
    UINT numHandles;
    UINT numAllocs;
    HWND *handles;
};

static BOOL CALLBACK MSGBOX_EnumProc(HWND hwnd, LPARAM lParam)
{
    struct ThreadWindows *threadWindows = (struct ThreadWindows *)lParam;

    if (!EnableWindow(hwnd, FALSE))
    {
        if(threadWindows->numHandles >= threadWindows->numAllocs)
        {
            threadWindows->handles = HeapReAlloc(GetProcessHeap(), 0, threadWindows->handles,
                                                 (threadWindows->numAllocs*2)*sizeof(HWND));
            threadWindows->numAllocs *= 2;
        }
        threadWindows->handles[threadWindows->numHandles++]=hwnd;
    }
   return TRUE;
}

static void MSGBOX_OnInit(HWND hwnd, LPMSGBOXPARAMSW lpmb)
{
    HFONT hPrevFont;
    RECT rect;
    HWND hItem;
    HDC hdc;
    int i, buttons;
    int bspace, bw, bh, theight, tleft, wwidth, wheight, wleft, wtop, bpos;
    int borheight, borwidth, iheight, ileft, iwidth, twidth, tiheight;
    NONCLIENTMETRICSW nclm;
    HMONITOR monitor;
    MONITORINFO mon_info;
    LPCWSTR lpszText;
    WCHAR *buffer = NULL;
    const WCHAR *ptr;

    /* Index the order the buttons need to appear to an ID* constant */
    static const int buttonOrder[10] = { IDYES, IDNO, IDOK, IDABORT, IDRETRY,
                                         IDCANCEL, IDIGNORE, IDTRYAGAIN,
                                         IDCONTINUE, IDHELP };

    nclm.cbSize = sizeof(nclm);
    SystemParametersInfoW (SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);

    if (!IS_INTRESOURCE(lpmb->lpszCaption)) {
       SetWindowTextW(hwnd, lpmb->lpszCaption);
    } else {
        UINT len = LoadStringW( lpmb->hInstance, LOWORD(lpmb->lpszCaption), (LPWSTR)&ptr, 0 );
        if (!len) len = LoadStringW( user32_module, IDS_ERROR, (LPWSTR)&ptr, 0 );
        buffer = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) );
        if (buffer)
        {
            memcpy( buffer, ptr, len * sizeof(WCHAR) );
            buffer[len] = 0;
            SetWindowTextW( hwnd, buffer );
            HeapFree( GetProcessHeap(), 0, buffer );
            buffer = NULL;
        }
    }
    if (IS_INTRESOURCE(lpmb->lpszText)) {
        UINT len = LoadStringW( lpmb->hInstance, LOWORD(lpmb->lpszText), (LPWSTR)&ptr, 0 );
        lpszText = buffer = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) );
        if (buffer)
        {
            memcpy( buffer, ptr, len * sizeof(WCHAR) );
            buffer[len] = 0;
        }
    } else {
       lpszText = lpmb->lpszText;
    }

    /* handle modal message boxes */
    if (lpmb->dwStyle & MB_TASKMODAL && lpmb->hwndOwner == NULL)
        NtUserSetWindowPos( hwnd, lpmb->dwStyle & MB_TOPMOST ? HWND_TOPMOST : HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
    else if (lpmb->dwStyle & MB_SYSTEMMODAL)
        NtUserSetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED );
    else if (lpmb->dwStyle & MB_TOPMOST)
        NtUserSetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

    TRACE_(msgbox)("%s\n", debugstr_w(lpszText));
    SetWindowTextW(GetDlgItem(hwnd, MSGBOX_IDTEXT), lpszText);

    /* Remove not selected buttons and assign the WS_GROUP style to the first button */
    hItem = 0;
    switch(lpmb->dwStyle & MB_TYPEMASK) {
    case MB_OK:
	NtUserDestroyWindow(GetDlgItem(hwnd, IDCANCEL));
	/* fall through */
    case MB_OKCANCEL:
	hItem = GetDlgItem(hwnd, IDOK);
	NtUserDestroyWindow(GetDlgItem(hwnd, IDABORT));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDRETRY));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDIGNORE));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDYES));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDNO));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDTRYAGAIN));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDCONTINUE));
	break;
    case MB_ABORTRETRYIGNORE:
	hItem = GetDlgItem(hwnd, IDABORT);
	NtUserDestroyWindow(GetDlgItem(hwnd, IDOK));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDCANCEL));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDYES));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDNO));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDCONTINUE));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDTRYAGAIN));
	break;
    case MB_YESNO:
	NtUserDestroyWindow(GetDlgItem(hwnd, IDCANCEL));
	/* fall through */
    case MB_YESNOCANCEL:
	hItem = GetDlgItem(hwnd, IDYES);
	NtUserDestroyWindow(GetDlgItem(hwnd, IDOK));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDABORT));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDRETRY));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDIGNORE));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDCONTINUE));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDTRYAGAIN));
	break;
    case MB_RETRYCANCEL:
	hItem = GetDlgItem(hwnd, IDRETRY);
	NtUserDestroyWindow(GetDlgItem(hwnd, IDOK));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDABORT));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDIGNORE));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDYES));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDNO));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDCONTINUE));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDTRYAGAIN));
	break;
    case MB_CANCELTRYCONTINUE:
	hItem = GetDlgItem(hwnd, IDCANCEL);
	NtUserDestroyWindow(GetDlgItem(hwnd, IDOK));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDABORT));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDIGNORE));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDYES));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDNO));
	NtUserDestroyWindow(GetDlgItem(hwnd, IDRETRY));
    }

    if (hItem) SetWindowLongW(hItem, GWL_STYLE, GetWindowLongW(hItem, GWL_STYLE) | WS_GROUP);

    /* Set the icon */
    switch(lpmb->dwStyle & MB_ICONMASK) {
    case MB_ICONEXCLAMATION:
	SendDlgItemMessageW(hwnd, MSGBOX_IDICON, STM_SETICON,
			    (WPARAM)LoadIconW(0, (LPWSTR)IDI_EXCLAMATION), 0);
	break;
    case MB_ICONQUESTION:
	SendDlgItemMessageW(hwnd, MSGBOX_IDICON, STM_SETICON,
			    (WPARAM)LoadIconW(0, (LPWSTR)IDI_QUESTION), 0);
	break;
    case MB_ICONASTERISK:
	SendDlgItemMessageW(hwnd, MSGBOX_IDICON, STM_SETICON,
			    (WPARAM)LoadIconW(0, (LPWSTR)IDI_ASTERISK), 0);
	break;
    case MB_ICONHAND:
      SendDlgItemMessageW(hwnd, MSGBOX_IDICON, STM_SETICON,
			    (WPARAM)LoadIconW(0, (LPWSTR)IDI_HAND), 0);
      break;
    case MB_USERICON:
      SendDlgItemMessageW(hwnd, MSGBOX_IDICON, STM_SETICON,
			  (WPARAM)LoadIconW(lpmb->hInstance, lpmb->lpszIcon), 0);
      break;
    default:
	/* By default, Windows 95/98/NT do not associate an icon to message boxes.
         * So wine should do the same.
         */
	break;
    }

    /* Remove Help button unless MB_HELP supplied */
    if (!(lpmb->dwStyle & MB_HELP)) {
        NtUserDestroyWindow(GetDlgItem(hwnd, IDHELP));
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
    if (!(lpmb->dwStyle & MB_ICONMASK))
    {
        rect.bottom = rect.top;
        rect.right = rect.left;
    }
    iheight = rect.bottom - rect.top;
    ileft = rect.left;
    iwidth = rect.right - ileft;

    hdc = NtUserGetDC(hwnd);
    hPrevFont = SelectObject( hdc, (HFONT)SendMessageW( hwnd, WM_GETFONT, 0, 0 ));

    /* Get the number of visible buttons and their size */
    bh = bw = 1; /* Minimum button sizes */
    for (buttons = 0, i = IDOK; i <= IDCONTINUE; i++)
    {
        if (i == IDCLOSE) continue; /* No CLOSE button */
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
    DrawTextW(hdc, lpszText, -1, &rect,
              DT_LEFT | DT_EXPANDTABS | DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX);
    /* Min text width corresponds to space for the buttons */
    tleft = ileft;
    if (iwidth) tleft += ileft + iwidth;
    twidth = max((bw + bspace) * buttons + bspace - tleft, rect.right);
    theight = rect.bottom;

    SelectObject(hdc, hPrevFont);
    NtUserReleaseDC( hwnd, hdc );

    tiheight = 16 + max(iheight, theight);
    wwidth  = tleft + twidth + ileft + borwidth;
    wheight = 8 + tiheight + bh + borheight;

    /* Message boxes are always desktop centered, so query desktop size and center window */
    monitor = MonitorFromWindow(lpmb->hwndOwner ? lpmb->hwndOwner : GetActiveWindow(), MONITOR_DEFAULTTOPRIMARY);
    mon_info.cbSize = sizeof(mon_info);
    GetMonitorInfoW(monitor, &mon_info);
    wleft = (mon_info.rcWork.left + mon_info.rcWork.right - wwidth) / 2;
    wtop = (mon_info.rcWork.top + mon_info.rcWork.bottom - wheight) / 2;

    /* Resize and center the window */
    NtUserSetWindowPos( hwnd, 0, wleft, wtop, wwidth, wheight,
                        SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW );

    /* Position the icon */
    NtUserSetWindowPos( GetDlgItem(hwnd, MSGBOX_IDICON), 0, ileft, (tiheight - iheight) / 2, 0, 0,
                        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW );

    /* Position the text */
    NtUserSetWindowPos( GetDlgItem(hwnd, MSGBOX_IDTEXT), 0, tleft, (tiheight - theight) / 2, twidth, theight,
                        SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW );

    /* Position the buttons */
    bpos = (wwidth - (bw + bspace) * buttons + bspace) / 2;
    for (buttons = i = 0; i < ARRAY_SIZE(buttonOrder); i++) {

	/* Convert the button order to ID* value to order for the buttons */
	hItem = GetDlgItem(hwnd, buttonOrder[i]);
	if (GetWindowLongW(hItem, GWL_STYLE) & WS_VISIBLE) {
	    if (buttons++ == ((lpmb->dwStyle & MB_DEFMASK) >> 8)) {
		NtUserSetFocus(hItem);
		SendMessageW( hItem, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE );
	    }
	    NtUserSetWindowPos( hItem, 0, bpos, tiheight, bw, bh,
                                SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOREDRAW );
	    bpos += bw + bspace;
	}
    }

    HeapFree( GetProcessHeap(), 0, buffer );
}


/**************************************************************************
 *           MSGBOX_DlgProc
 *
 * Dialog procedure for message boxes.
 */
static INT_PTR CALLBACK MSGBOX_DlgProc( HWND hwnd, UINT message,
                                        WPARAM wParam, LPARAM lParam )
{
  switch(message) {
   case WM_INITDIALOG:
   {
       LPMSGBOXPARAMSW mbp = (LPMSGBOXPARAMSW)lParam;
       SetWindowContextHelpId(hwnd, mbp->dwContextHelpId);
       MSGBOX_OnInit(hwnd, mbp);
       SetPropA(hwnd, "WINE_MSGBOX_HELPCALLBACK", mbp->lpfnMsgBoxCallback);
       break;
   }

   case WM_COMMAND:
    switch (LOWORD(wParam))
    {
     case IDOK:
     case IDCANCEL:
     case IDABORT:
     case IDRETRY:
     case IDIGNORE:
     case IDYES:
     case IDNO:
     case IDTRYAGAIN:
     case IDCONTINUE:
      EndDialog(hwnd, wParam);
      break;
     case IDHELP:
      FIXME("Help button not supported yet\n");
      break;
    }
    break;

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
 *      MessageBoxTimeoutA (USER32.@)
 */
INT WINAPI MessageBoxTimeoutA( HWND hWnd, LPCSTR text, LPCSTR title,
                               UINT type, WORD langid, DWORD timeout )
{
    FIXME("timeout not supported (%lu)\n", timeout);
    return MessageBoxExA( hWnd, text, title, type, langid );
}

/**************************************************************************
 *      MessageBoxTimeoutW (USER32.@)
 */
INT WINAPI MessageBoxTimeoutW( HWND hWnd, LPCWSTR text, LPCWSTR title,
                               UINT type, WORD langid, DWORD timeout )
{
    FIXME("timeout not supported (%lu)\n", timeout);
    return MessageBoxExW( hWnd, text, title, type, langid );
}

/**************************************************************************
 *		MessageBoxIndirectA (USER32.@)
 */
INT WINAPI MessageBoxIndirectA( LPMSGBOXPARAMSA msgbox )
{
    MSGBOXPARAMSW msgboxW;
    UNICODE_STRING textW, captionW, iconW;
    int ret;

    if (IS_INTRESOURCE(msgbox->lpszText))
        textW.Buffer = (LPWSTR)msgbox->lpszText;
    else
        RtlCreateUnicodeStringFromAsciiz(&textW, msgbox->lpszText);
    if (IS_INTRESOURCE(msgbox->lpszCaption))
        captionW.Buffer = (LPWSTR)msgbox->lpszCaption;
    else
        RtlCreateUnicodeStringFromAsciiz(&captionW, msgbox->lpszCaption);

    if (msgbox->dwStyle & MB_USERICON)
    {
        if (IS_INTRESOURCE(msgbox->lpszIcon))
            iconW.Buffer = (LPWSTR)msgbox->lpszIcon;
        else
            RtlCreateUnicodeStringFromAsciiz(&iconW, msgbox->lpszIcon);
    }
    else
        iconW.Buffer = NULL;

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

    if (!IS_INTRESOURCE(textW.Buffer)) RtlFreeUnicodeString(&textW);
    if (!IS_INTRESOURCE(captionW.Buffer)) RtlFreeUnicodeString(&captionW);
    if (!IS_INTRESOURCE(iconW.Buffer)) RtlFreeUnicodeString(&iconW);
    return ret;
}

/**************************************************************************
 *		MessageBoxIndirectW (USER32.@)
 */
INT WINAPI MessageBoxIndirectW( LPMSGBOXPARAMSW msgbox )
{
    LPVOID tmplate;
    HRSRC hRes;
    int ret;
    UINT i;
    struct ThreadWindows threadWindows;

    if (!(hRes = FindResourceExW(user32_module, (LPWSTR)RT_DIALOG, L"MSGBOX", msgbox->dwLanguageId)))
    {
        if (!msgbox->dwLanguageId ||
            !(hRes = FindResourceExW(user32_module, (LPWSTR)RT_DIALOG, L"MSGBOX", LANG_NEUTRAL)))
            return 0;
    }
    if (!(tmplate = LoadResource(user32_module, hRes)))
        return 0;

    if ((msgbox->dwStyle & MB_TASKMODAL) && (msgbox->hwndOwner==NULL))
    {
        threadWindows.numHandles = 0;
        threadWindows.numAllocs = 10;
        threadWindows.handles = HeapAlloc(GetProcessHeap(), 0, 10*sizeof(HWND));
        EnumThreadWindows(GetCurrentThreadId(), MSGBOX_EnumProc, (LPARAM)&threadWindows);
    }

    ret=DialogBoxIndirectParamW(msgbox->hInstance, tmplate,
                                msgbox->hwndOwner, MSGBOX_DlgProc, (LPARAM)msgbox);

    if ((msgbox->dwStyle & MB_TASKMODAL) && (msgbox->hwndOwner==NULL))
    {
        for (i = 0; i < threadWindows.numHandles; i++)
            EnableWindow(threadWindows.handles[i], TRUE);
        HeapFree(GetProcessHeap(), 0, threadWindows.handles);
    }
    return ret;
}
