/*
 * Copyright 2000 Eric Pouech
 * Copyright 2003 Dmitry Timoshkov
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
 *
 * FIXME:
 * Add support for all remaining MCI_ and MCIWNDM_ messages.
 * Add support for MCIWNDF_NOTIFYMODE (all cases), MCIWNDF_NOTIFYPOS.
 */

#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winternl.h"
#include "vfw.h"
#include "digitalv.h"
#include "commctrl.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mci);

extern HMODULE MSVFW32_hModule;
static const WCHAR mciWndClassW[] = {'M','C','I','W','n','d','C','l','a','s','s',0};

typedef struct
{
    DWORD       dwStyle;
    MCIDEVICEID mci;
    UINT        dev_type;
    LPWSTR      lpName;
    HWND        hWnd, hwndOwner;
    UINT        uTimer;
    MCIERROR    lasterror;
} MCIWndInfo;

#define MCIWND_NOTIFY_MODE(info) \
    if ((info)->dwStyle & MCIWNDF_NOTIFYMODE) \
        SendMessageW((info)->hwndOwner, MCIWNDM_NOTIFYMODE, (WPARAM)(info)->hWnd, (LPARAM)SendMessageW((info)->hWnd, MCIWNDM_GETMODEW, 0, 0))

#define MCIWND_NOTIFY_SIZE(info) \
    if ((info)->dwStyle & MCIWNDF_NOTIFYSIZE) \
        SendMessageW((info)->hwndOwner, MCIWNDM_NOTIFYSIZE, (WPARAM)(info)->hWnd, 0);

#define MCIWND_NOTIFY_ERROR(info) \
    if ((info)->dwStyle & MCIWNDF_NOTIFYERROR) \
        SendMessageW((info)->hwndOwner, MCIWNDM_NOTIFYERROR, (WPARAM)(info)->hWnd, (LPARAM)(info)->lasterror)

#define MCIWND_NOTIFY_MEDIA(info) MCIWND_notify_media(info)

static LRESULT WINAPI MCIWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);

#define CTL_PLAYSTOP    0x3200
#define CTL_MENU        0x3201
#define CTL_TRACKBAR    0x3202

/***********************************************************************
 *                MCIWndRegisterClass                [MSVFW32.@]
 *
 * NOTE: Native always uses its own hInstance
 */
BOOL VFWAPIV MCIWndRegisterClass(HINSTANCE hInst)
{
    WNDCLASSW wc;

    /* Since we are going to register a class belonging to MSVFW32
     * and later we will create windows with a different hInstance
     * CS_GLOBALCLASS is needed. And because the second attempt
     * to register a global class will fail we need to test whether
     * the class was already registered.
     */
    wc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_OWNDC | CS_GLOBALCLASS;
    wc.lpfnWndProc = MCIWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(MCIWndInfo*);
    wc.hInstance = MSVFW32_hModule;
    wc.hIcon = 0;
    wc.hCursor = LoadCursorW(0, MAKEINTRESOURCEW(IDC_ARROW));
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = mciWndClassW;

    if (RegisterClassW(&wc)) return TRUE;
    if (GetLastError() == ERROR_CLASS_ALREADY_EXISTS) return TRUE;

    return FALSE;
}

/***********************************************************************
 *                MCIWndCreateW                                [MSVFW32.@]
 */
HWND VFWAPIV MCIWndCreateW(HWND hwndParent, HINSTANCE hInstance,
                           DWORD dwStyle, LPCWSTR szFile)
{
    TRACE("%p %p %lx %s\n", hwndParent, hInstance, dwStyle, debugstr_w(szFile));

    MCIWndRegisterClass(hInstance);

    /* window becomes visible after MCI_PLAY command in the case of MCIWNDF_NOOPEN */
    if (dwStyle & MCIWNDF_NOOPEN)
        dwStyle &= ~WS_VISIBLE;

    return CreateWindowExW(0, mciWndClassW, NULL,
                           dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                           0, 0, 300, 0,
                           hwndParent, 0, hInstance, (LPVOID)szFile);
}

/***********************************************************************
 *                MCIWndCreate                [MSVFW32.@]
 *                MCIWndCreateA                [MSVFW32.@]
 */
HWND VFWAPIV MCIWndCreateA(HWND hwndParent, HINSTANCE hInstance,
                           DWORD dwStyle, LPCSTR szFile)
{
    HWND ret;
    UNICODE_STRING fileW;

    if (szFile)
        RtlCreateUnicodeStringFromAsciiz(&fileW, szFile);
    else
        fileW.Buffer = NULL;

    ret = MCIWndCreateW(hwndParent, hInstance, dwStyle, fileW.Buffer);

    RtlFreeUnicodeString(&fileW);
    return ret;
}

static void MCIWND_UpdateText(MCIWndInfo *mwi)
{
    WCHAR buffer[1024];

    if ((mwi->dwStyle & MCIWNDF_SHOWNAME) && mwi->lpName)
        strcpyW(buffer, mwi->lpName);
    else
        *buffer = 0;

    if (mwi->dwStyle & (MCIWNDF_SHOWPOS|MCIWNDF_SHOWMODE))
    {
        static const WCHAR spaceW[] = {' ',0};
        static const WCHAR l_braceW[] = {'(',0};

        if (*buffer) strcatW(buffer, spaceW);
        strcatW(buffer, l_braceW);
    }

    if (mwi->dwStyle & MCIWNDF_SHOWPOS)
    {
        static const WCHAR formatW[] = {'%','l','d',0};
        sprintfW(buffer + strlenW(buffer), formatW, SendMessageW(mwi->hWnd, MCIWNDM_GETPOSITIONW, 0, 0));
    }

    if ((mwi->dwStyle & (MCIWNDF_SHOWPOS|MCIWNDF_SHOWMODE)) == (MCIWNDF_SHOWPOS|MCIWNDF_SHOWMODE))
    {
        static const WCHAR dashW[] = {' ','-',' ',0};
        strcatW(buffer, dashW);
    }

    if (mwi->dwStyle & MCIWNDF_SHOWMODE)
    {
        /* FIXME: get the status string from resources */
        static const WCHAR not_readyW[] = {'n','o','t',' ','r','e','a','d','y',0};
        static const WCHAR pausedW[] = {'p','a','u','s','e','d',0};
        static const WCHAR playingW[] = {'p','l','a','y','i','n','g',0};
        static const WCHAR stoppedW[] = {'s','t','o','p','p','e','d',0};
        static const WCHAR openW[] = {'o','p','e','n',0};
        static const WCHAR recordingW[] = {'r','e','c','o','r','d','i','n','g',0};
        static const WCHAR seekingW[] = {'s','e','e','k','i','n','g',0};
        static const WCHAR unknownW[] = {'?','?','?',0};

        switch (SendMessageW(mwi->hWnd, MCIWNDM_GETMODEW, 0, 0))
        {
            case MCI_MODE_NOT_READY: strcatW(buffer, not_readyW); break;
            case MCI_MODE_PAUSE: strcatW(buffer, pausedW); break;
            case MCI_MODE_PLAY: strcatW(buffer, playingW); break;
            case MCI_MODE_STOP: strcatW(buffer, stoppedW); break;
            case MCI_MODE_OPEN: strcatW(buffer, openW); break;
            case MCI_MODE_RECORD: strcatW(buffer, recordingW); break;
            case MCI_MODE_SEEK: strcatW(buffer, seekingW); break;
            default: strcatW(buffer, unknownW); break;
        }
    }

    if (mwi->dwStyle & (MCIWNDF_SHOWPOS|MCIWNDF_SHOWMODE))
    {
        static const WCHAR r_braceW[] = {' ',')',0};
        strcatW(buffer, r_braceW);
    }

    TRACE("=> '%s'\n", debugstr_w(buffer));
    SetWindowTextW(mwi->hWnd, buffer);
}

static LRESULT MCIWND_Create(HWND hWnd, LPCREATESTRUCTW cs)
{
    HWND hChld;
    MCIWndInfo *mwi;
    static const WCHAR buttonW[] = {'b','u','t','t','o','n',0};

    /* This sets the default window size */
    SendMessageW(hWnd, MCI_CLOSE, 0, 0);

    mwi = HeapAlloc(GetProcessHeap(), 0, sizeof(*mwi));
    if (!mwi) return -1;

    SetWindowLongW(hWnd, 0, (LPARAM)mwi);

    mwi->dwStyle = cs->style;
    mwi->mci = 0;
    mwi->lpName = NULL;
    mwi->uTimer = 0;
    mwi->hWnd = hWnd;
    mwi->hwndOwner = cs->hwndParent;
    mwi->lasterror = 0;

    if (!(mwi->dwStyle & MCIWNDF_NOMENU))
    {
        static const WCHAR menuW[] = {'M','e','n','u',0};

        hChld = CreateWindowExW(0, buttonW, menuW, WS_CHILD|WS_VISIBLE, 32, cs->cy, 32, 32,
                                hWnd, (HMENU)CTL_MENU, cs->hInstance, 0L);
        TRACE("Get Button2: %p\n", hChld);
    }

    if (!(mwi->dwStyle & MCIWNDF_NOPLAYBAR))
    {
        INITCOMMONCONTROLSEX init;
        static const WCHAR playW[] = {'P','l','a','y',0};

        /* adding the other elements: play/stop button, menu button, status */
        hChld = CreateWindowExW(0, buttonW, playW, WS_CHILD|WS_VISIBLE, 0, cs->cy, 32, 32,
                                hWnd, (HMENU)CTL_PLAYSTOP, cs->hInstance, 0L);
        TRACE("Get Button1: %p\n", hChld);

        init.dwSize = sizeof(init);
        init.dwICC = ICC_BAR_CLASSES;
        InitCommonControlsEx(&init);

        hChld = CreateWindowExW(0, TRACKBAR_CLASSW, NULL, WS_CHILD|WS_VISIBLE, 64, cs->cy, cs->cx - 64, 32,
                                hWnd, (HMENU)CTL_TRACKBAR, cs->hInstance, 0L);
        TRACE("Get status: %p\n", hChld);
    }

    SendMessageW(hWnd, MCIWNDM_OPENW, 0, (LPARAM)cs->lpCreateParams);

    MCIWND_UpdateText(mwi);
    return 0;
}

static void MCIWND_ToggleState(MCIWndInfo *mwi)
{
    switch (SendMessageW(mwi->hWnd, MCIWNDM_GETMODEW, 0, 0))
    {
    case MCI_MODE_NOT_READY:
    case MCI_MODE_RECORD:
    case MCI_MODE_SEEK:
    case MCI_MODE_OPEN:
        TRACE("Cannot do much...\n");
        break;

    case MCI_MODE_PAUSE:
        SendMessageW(mwi->hWnd, MCI_RESUME, 0, 0);
        break;

    case MCI_MODE_PLAY:
        SendMessageW(mwi->hWnd, MCI_PAUSE, 0, 0);
        break;

    case MCI_MODE_STOP:
        SendMessageW(mwi->hWnd, MCI_STOP, 0, 0);
        break;
    }
}

static LRESULT MCIWND_Command(MCIWndInfo *mwi, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
    case CTL_PLAYSTOP: MCIWND_ToggleState(mwi); break;
    case CTL_MENU:
    case CTL_TRACKBAR:
    default:
        MessageBoxA(0, "ooch", "NIY", MB_OK);
    }
    return 0L;
}

static void MCIWND_Timer(MCIWndInfo *mwi)
{
    LONG pos = SendMessageW(mwi->hWnd, MCIWNDM_GETPOSITIONW, 0, 0);
    TRACE("%ld\n", pos);
    SendDlgItemMessageW(mwi->hWnd, CTL_TRACKBAR, TBM_SETPOS, TRUE, pos);
    MCIWND_UpdateText(mwi);
}

static void MCIWND_notify_media(MCIWndInfo *mwi)
{
    if (mwi->dwStyle & (MCIWNDF_NOTIFYMEDIAA | MCIWNDF_NOTIFYMEDIAW))
    {
        if (!mwi->lpName)
        {
            static const WCHAR empty_str[1];
            SendMessageW(mwi->hwndOwner, MCIWNDM_NOTIFYMEDIA, (WPARAM)mwi->hWnd, (LPARAM)empty_str);
        }
        else
        {
            if (mwi->dwStyle & MCIWNDF_NOTIFYANSI)
            {
                char *ansi_name;
                int len;

                len = WideCharToMultiByte(CP_ACP, 0, mwi->lpName, -1, NULL, 0, NULL, NULL);
                ansi_name = HeapAlloc(GetProcessHeap(), 0, len);
                WideCharToMultiByte(CP_ACP, 0, mwi->lpName, -1, ansi_name, len, NULL, NULL);

                SendMessageW(mwi->hwndOwner, MCIWNDM_NOTIFYMEDIA, (WPARAM)mwi->hWnd, (LPARAM)ansi_name);

                HeapFree(GetProcessHeap(), 0, ansi_name);
            }
            else
                SendMessageW(mwi->hwndOwner, MCIWNDM_NOTIFYMEDIA, (WPARAM)mwi->hWnd, (LPARAM)mwi->lpName);
        }
    }
}

static LRESULT WINAPI MCIWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    MCIWndInfo *mwi;

    if (wMsg == WM_CREATE)
        return MCIWND_Create(hWnd, (CREATESTRUCTW *)lParam);

    mwi = (MCIWndInfo*)GetWindowLongW(hWnd, 0);
    if (!mwi)
        return DefWindowProcW(hWnd, wMsg, wParam, lParam);

    switch (wMsg)
    {
    case WM_DESTROY:
        if (mwi->uTimer)
            KillTimer(hWnd, mwi->uTimer);

        SendMessageW(hWnd, MCI_CLOSE, 0, 0);

        if (mwi->lpName)
            HeapFree(GetProcessHeap(), 0, mwi->lpName);
        HeapFree(GetProcessHeap(), 0, mwi);
        return 0;

    case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT ps;

            hdc = (wParam) ? (HDC)wParam : BeginPaint(mwi->hWnd, &ps);
            /* something to do ? */
            if (!wParam) EndPaint(mwi->hWnd, &ps);
            return 1;
        }

    case WM_COMMAND:
        return MCIWND_Command(mwi, wParam, lParam);

    case WM_TIMER:
        MCIWND_Timer(mwi);
        return 0;

    case WM_SIZE:
        {
            MCIWND_NOTIFY_SIZE(mwi);

            if (wParam == SIZE_MINIMIZED) return 0;

            SetWindowPos(GetDlgItem(hWnd, CTL_PLAYSTOP), 0, 0, HIWORD(lParam) - 32, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
            SetWindowPos(GetDlgItem(hWnd, CTL_MENU), 0, 32, HIWORD(lParam) - 32, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
            SetWindowPos(GetDlgItem(hWnd, CTL_TRACKBAR), 0, 64, HIWORD(lParam) - 32, LOWORD(lParam) - 64, 32, SWP_NOACTIVATE);
            return 0;
        }

    case MM_MCINOTIFY:
        MCIWND_NOTIFY_MODE(mwi);
        return 0;

    case MCIWNDM_OPENA:
        {
            UNICODE_STRING nameW;
            RtlCreateUnicodeStringFromAsciiz(&nameW, (LPCSTR)lParam);
            lParam = (LPARAM)nameW.Buffer;
        }
        /* fall through */
    case MCIWNDM_OPENW:
        {
            RECT rc;

            if (mwi->uTimer)
            {
                KillTimer(hWnd, mwi->uTimer);
                mwi->uTimer = 0;
            }

            if (lParam)
            {
                HCURSOR hCursor;
                MCI_OPEN_PARMSW mci_open;
                MCI_GETDEVCAPS_PARMS mci_devcaps;

                hCursor = LoadCursorW(0, (LPWSTR)IDC_WAIT);
                hCursor = SetCursor(hCursor);

                mci_open.lpstrElementName = (LPWSTR)lParam;
                mwi->lasterror = mciSendCommandW(mwi->mci, MCI_OPEN,
                                                 MCI_OPEN_ELEMENT | MCI_WAIT,
                                                 (DWORD_PTR)&mci_open);
                SetCursor(hCursor);

                if (mwi->lasterror)
                {
                    /* FIXME: get the caption from resources */
                    static const WCHAR caption[] = {'M','C','I',' ','E','r','r','o','r',0};
                    WCHAR error_str[MAXERRORLENGTH];

                    mciGetErrorStringW(mwi->lasterror, error_str, MAXERRORLENGTH);
                    MessageBoxW(hWnd, error_str, caption, MB_ICONEXCLAMATION | MB_OK);
                    MCIWND_NOTIFY_ERROR(mwi);
                    goto end_of_mci_open;
                }


                mwi->mci = mci_open.wDeviceID;
                mwi->lpName = HeapAlloc(GetProcessHeap(), 0, (strlenW((LPWSTR)lParam) + 1) * sizeof(WCHAR));
                strcpyW(mwi->lpName, (LPWSTR)lParam);

                mci_devcaps.dwItem = MCI_GETDEVCAPS_DEVICE_TYPE;
                mwi->lasterror = mciSendCommandW(mwi->mci, MCI_GETDEVCAPS,
                                                 MCI_GETDEVCAPS_ITEM,
                                                 (DWORD_PTR)&mci_devcaps);
                if (mwi->lasterror)
                {
                    MCIWND_NOTIFY_ERROR(mwi);
                    goto end_of_mci_open;
                }

                mwi->dev_type = mci_devcaps.dwReturn;

                if (mwi->dev_type == MCI_DEVTYPE_DIGITAL_VIDEO)
                {
                    MCI_DGV_WINDOW_PARMSW mci_window;

                    mci_window.hWnd = hWnd;
                    mwi->lasterror = mciSendCommandW(mwi->mci, MCI_WINDOW,
                                                     MCI_DGV_WINDOW_HWND,
                                                     (DWORD_PTR)&mci_window);
                    if (mwi->lasterror)
                    {
                        MCIWND_NOTIFY_ERROR(mwi);
                        goto end_of_mci_open;
                    }
                }
            }

            if (SendMessageW(hWnd, MCIWNDM_GET_DEST, 0, (LPARAM)&rc) != 0)
            {
                GetClientRect(hWnd, &rc);
                rc.bottom = rc.top;
            }

            AdjustWindowRect(&rc, GetWindowLongW(hWnd, GWL_STYLE), FALSE);
            if (!(mwi->dwStyle & MCIWNDF_NOPLAYBAR))
                rc.bottom += 32; /* add the height of the playbar */
            SetWindowPos(hWnd, 0, 0, 0, rc.right - rc.left,
                rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

            MCIWND_NOTIFY_MEDIA(mwi);

            SendDlgItemMessageW(hWnd, CTL_TRACKBAR, TBM_SETRANGEMIN, 0L, 0L);
            SendDlgItemMessageW(hWnd, CTL_TRACKBAR, TBM_SETRANGEMAX, 1L, SendMessageW(mwi->hWnd, MCIWNDM_GETLENGTH, 0, 0));
            SetTimer(hWnd, 1, 500, NULL);

end_of_mci_open:
            if (wMsg == MCIWNDM_OPENA)
                HeapFree(GetProcessHeap(), 0, (void *)lParam);
            return mwi->lasterror;
        }

    case MCIWNDM_GETDEVICEID:
        return mwi->mci;

    case MCIWNDM_GET_SOURCE:
        {
            MCI_DGV_RECT_PARMS mci_rect;

            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_WHERE,
                                             MCI_DGV_WHERE_SOURCE,
                                             (DWORD_PTR)&mci_rect);
            if (mwi->lasterror)
            {
                MCIWND_NOTIFY_ERROR(mwi);
                return mwi->lasterror;
            }
            *(RECT *)lParam = mci_rect.rc;
            return 0;
        }

    case MCIWNDM_GET_DEST:
        {
            MCI_DGV_RECT_PARMS mci_rect;

            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_WHERE,
                                             MCI_DGV_WHERE_DESTINATION,
                                             (DWORD_PTR)&mci_rect);
            if (mwi->lasterror)
            {
                MCIWND_NOTIFY_ERROR(mwi);
                return mwi->lasterror;
            }
            *(RECT *)lParam = mci_rect.rc;
            return 0;
        }

    case MCIWNDM_PUT_SOURCE:
        {
            MCI_DGV_PUT_PARMS mci_put;

            mci_put.rc = *(RECT *)lParam;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_PUT,
                                             MCI_DGV_PUT_SOURCE,
                                             (DWORD_PTR)&mci_put);
            if (mwi->lasterror)
            {
                MCIWND_NOTIFY_ERROR(mwi);
                return mwi->lasterror;
            }
            return 0;
        }

    case MCIWNDM_PUT_DEST:
        {
            MCI_DGV_PUT_PARMS mci_put;

            mci_put.rc = *(RECT *)lParam;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_PUT,
                                             MCI_DGV_PUT_DESTINATION,
                                             (DWORD_PTR)&mci_put);
            if (mwi->lasterror)
            {
                MCIWND_NOTIFY_ERROR(mwi);
                return mwi->lasterror;
            }
            return 0;
        }

    case MCIWNDM_GETLENGTH:
        {
            MCI_STATUS_PARMS mci_status;

            mci_status.dwItem = MCI_STATUS_LENGTH;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_STATUS,
                                             MCI_STATUS_ITEM,
                                             (DWORD_PTR)&mci_status);
            if (mwi->lasterror)
            {
                MCIWND_NOTIFY_ERROR(mwi);
                return 0;
            }
            return mci_status.dwReturn;
        }

    case MCIWNDM_GETSTART:
        {
            MCI_STATUS_PARMS mci_status;

            mci_status.dwItem = MCI_STATUS_POSITION;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_STATUS,
                                             MCI_STATUS_ITEM | MCI_STATUS_START,
                                             (DWORD_PTR)&mci_status);
            if (mwi->lasterror)
            {
                MCIWND_NOTIFY_ERROR(mwi);
                return 0;
            }
            return mci_status.dwReturn;
        }

    case MCIWNDM_GETEND:
        {
            LRESULT start, length;

            start = SendMessageW(hWnd, MCIWNDM_GETSTART, 0, 0);
            length = SendMessageW(hWnd, MCIWNDM_GETLENGTH, 0, 0);
            return (start + length);
        }

    case MCIWNDM_GETPOSITIONA:
    case MCIWNDM_GETPOSITIONW:
        {
            MCI_STATUS_PARMS mci_status;

            mci_status.dwItem = MCI_STATUS_POSITION;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_STATUS,
                                             MCI_STATUS_ITEM,
                                             (DWORD_PTR)&mci_status);
            if (mwi->lasterror)
            {
                MCIWND_NOTIFY_ERROR(mwi);
                return 0;
            }
            return mci_status.dwReturn;
        }

    case MCIWNDM_GETMODEA:
    case MCIWNDM_GETMODEW:
        {
            MCI_STATUS_PARMS mci_status;

            mci_status.dwItem = MCI_STATUS_MODE;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_STATUS,
                                             MCI_STATUS_ITEM,
                                             (DWORD_PTR)&mci_status);
            if (mwi->lasterror)
                return MCI_MODE_NOT_READY;

            return mci_status.dwReturn;
        }

    case MCIWNDM_PLAYTO:
        {
            MCI_PLAY_PARMS mci_play;

            mci_play.dwCallback = (DWORD_PTR)hWnd;
            mci_play.dwTo = lParam;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_PLAY,
                                             MCI_TO | MCI_NOTIFY, (DWORD_PTR)&mci_play);
            if (mwi->lasterror)
            {
                MCIWND_NOTIFY_ERROR(mwi);
                return mwi->lasterror;
            }
            return 0;
        }

    case MCIWNDM_RETURNSTRINGA:
        mciGetErrorStringA(mwi->lasterror, (LPSTR)lParam, wParam);
        return mwi->lasterror;

    case MCIWNDM_RETURNSTRINGW:
        mciGetErrorStringW(mwi->lasterror, (LPWSTR)lParam, wParam);
        return mwi->lasterror;

    case MCIWNDM_SETOWNER:
        mwi->hwndOwner = (HWND)wParam;
        return 0;

    case MCI_PLAY:
        {
            LRESULT end = SendMessageW(hWnd, MCIWNDM_GETEND, 0, 0);
            return SendMessageW(hWnd, MCIWNDM_PLAYTO, 0, end);
        }

    case MCI_STOP:
        {
            MCI_GENERIC_PARMS mci_generic;

            mci_generic.dwCallback = 0;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_STOP, 0, (DWORD_PTR)&mci_generic);

            if (mwi->lasterror)
            {
                MCIWND_NOTIFY_ERROR(mwi);
                return mwi->lasterror;
            }
            return 0;
        }

    case MCI_SEEK:
        {
            MCI_SEEK_PARMS mci_seek;

            switch (lParam)
            {
            case MCIWND_START:
                lParam = SendMessageW(hWnd, MCIWNDM_GETSTART, 0, 0);
                break;

            case MCIWND_END:
                lParam = SendMessageW(hWnd, MCIWNDM_GETEND, 0, 0);
                break;
            }

            mci_seek.dwTo = lParam;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_SEEK,
                                             MCI_TO, (DWORD_PTR)&mci_seek);
            if (mwi->lasterror)
            {
                MCIWND_NOTIFY_ERROR(mwi);
                return mwi->lasterror;
            }
            return 0;
        }

    case MCI_CLOSE:
        {
            MCI_GENERIC_PARMS mci_generic;

            mci_generic.dwCallback = 0;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_CLOSE, 0, (DWORD_PTR)&mci_generic);

            if (mwi->lasterror)
            {
                MCIWND_NOTIFY_ERROR(mwi);
                return mwi->lasterror;
            }
            return 0;
        }
    }

    if ((wMsg >= WM_USER) && (wMsg < WM_APP))
    {
        FIXME("support for MCIWNDM_ message WM_USER+%d not implemented\n", wMsg - WM_USER);
        return 0;
    }

    return DefWindowProcW(hWnd, wMsg, wParam, lParam);
}
