/*
 * Copyright 2000 Eric Pouech
 *
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

#include <stdio.h>
#include <string.h>

#include "winbase.h"
#include "windef.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "vfw.h"
#include "wine/mmsystem16.h"
#include "digitalv.h"
#include "commctrl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mci);

typedef struct {
   DWORD	dwStyle;
   MCIDEVICEID	mci;
   LPSTR	lpName;
   HWND		hWnd;
   UINT		uTimer;
} MCIWndInfo;

static LRESULT WINAPI	MCIWndProc(HWND hWnd, UINT wMsg, WPARAM lParam1, LPARAM lParam2);

#define CTL_PLAYSTOP	0x3200
#define CTL_MENU	0x3201
#define CTL_TRACKBAR	0x3202

/***********************************************************************
 *		MCIWndRegisterClass		[MSVFW32.@]
 */
BOOL WINAPI MCIWndRegisterClass(HINSTANCE hInst)
{
   WNDCLASSA		wc;

   /* since window creation will also require some common controls, init them */
   InitCommonControls();

   wc.style = 0;
   wc.lpfnWndProc = MCIWndProc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = sizeof(MCIWndInfo*);
   wc.hInstance = hInst;
   wc.hIcon = 0;
   wc.hCursor = 0;
   wc.hbrBackground = 0;
   wc.lpszMenuName = NULL;
   wc.lpszClassName = "MCIWndClass";

   return RegisterClassA(&wc);

}

/***********************************************************************
 *		MCIWndCreate		[MSVFW32.@]
 *		MCIWndCreateA		[MSVFW32.@]
 */
HWND VFWAPIV MCIWndCreateA(HWND hwndParent, HINSTANCE hInstance,
			   DWORD dwStyle, LPCSTR szFile)
{
   DWORD	wndStyle;
   MCIWndInfo*	mwi;

   TRACE("%x %x %lx %s\n", hwndParent, hInstance, dwStyle, szFile);

   MCIWndRegisterClass(hInstance);

   mwi = HeapAlloc(GetProcessHeap(), 0, sizeof(*mwi));
   if (!mwi) return 0;

   mwi->dwStyle = dwStyle;
   if (szFile)
     mwi->lpName = strcpy(HeapAlloc(GetProcessHeap(), 0, strlen(szFile) + 1), szFile);
   else
     mwi->lpName = NULL;
   mwi->uTimer = 0;

   wndStyle = ((hwndParent) ? (WS_CHILD|WS_BORDER) : WS_OVERLAPPEDWINDOW) |
              WS_VISIBLE | (dwStyle & 0xFFFF0000);

   if (CreateWindowExA(0, "MCIWndClass", NULL, wndStyle,
		       CW_USEDEFAULT, CW_USEDEFAULT,
		       CW_USEDEFAULT, CW_USEDEFAULT,
		       hwndParent, (HMENU)0, hInstance, mwi))
      return mwi->hWnd;

   if(mwi->lpName) HeapFree(GetProcessHeap(), 0, mwi->lpName);
   HeapFree(GetProcessHeap(), 0, mwi);
   return 0;
}

/***********************************************************************
 *		MCIWndCreateW				[MSVFW32.@]
 */
HWND VFWAPIV MCIWndCreateW(HWND hwndParent, HINSTANCE hInstance,
			   DWORD dwStyle, LPCWSTR szFile)
{
   FIXME("%x %x %lx %s\n", hwndParent, hInstance, dwStyle, debugstr_w(szFile));

   MCIWndRegisterClass(hInstance);

   return 0;
}

static DWORD MCIWND_GetStatus(MCIWndInfo* mwi)
{
   MCI_DGV_STATUS_PARMSA	mdsp;

   memset(&mdsp, 0, sizeof(mdsp));
   mdsp.dwItem = MCI_STATUS_MODE;
   if (mciSendCommandA(mwi->mci, MCI_STATUS, MCI_WAIT|MCI_STATUS_ITEM, (DWORD)&mdsp))
      return MCI_MODE_NOT_READY;
   if (mdsp.dwReturn == MCI_MODE_STOP && mwi->uTimer) {
      TRACE("Killing timer\n");
      KillTimer(mwi->hWnd, 0);
      mwi->uTimer = 0;
   }
   return mdsp.dwReturn;
}

static DWORD MCIWND_Get(MCIWndInfo* mwi, DWORD what)
{
   MCI_DGV_STATUS_PARMSA	mdsp;

   memset(&mdsp, 0, sizeof(mdsp));
   mdsp.dwItem = what;
   if (mciSendCommandA(mwi->mci, MCI_STATUS, MCI_WAIT|MCI_STATUS_ITEM, (DWORD)&mdsp))
      return 0;
   return mdsp.dwReturn;
}

static void MCIWND_SetText(MCIWndInfo* mwi)
{
   char	buffer[1024];

   if (mwi->dwStyle & MCIWNDF_SHOWNAME) {
      strcpy(buffer, mwi->lpName);
   } else {
      *buffer = 0;
   }

   if (mwi->dwStyle & (MCIWNDF_SHOWPOS|MCIWNDF_SHOWMODE)) {
      if (*buffer) strcat(buffer, " ");
      strcat(buffer, "(");
   }

   if (mwi->dwStyle & MCIWNDF_SHOWPOS) {
      sprintf(buffer + strlen(buffer), "%ld", MCIWND_Get(mwi, MCI_STATUS_POSITION));
   }

   if ((mwi->dwStyle & (MCIWNDF_SHOWPOS|MCIWNDF_SHOWMODE)) == (MCIWNDF_SHOWPOS|MCIWNDF_SHOWMODE)) {
      strcat(buffer, " - ");
   }

   if (mwi->dwStyle & MCIWNDF_SHOWMODE) {
      switch (MCIWND_GetStatus(mwi)) {
      case MCI_MODE_NOT_READY: 	strcat(buffer, "not ready");	break;
      case MCI_MODE_PAUSE:	strcat(buffer, "paused");	break;
      case MCI_MODE_PLAY:	strcat(buffer, "playing");	break;
      case MCI_MODE_STOP:	strcat(buffer, "stopped");	break;
      case MCI_MODE_OPEN:	strcat(buffer, "open");		break;
      case MCI_MODE_RECORD:	strcat(buffer, "recording");	break;
      case MCI_MODE_SEEK:	strcat(buffer, "seeking");	break;
      default:			strcat(buffer, "???");		break;
      }
   }
   if (mwi->dwStyle & (MCIWNDF_SHOWPOS|MCIWNDF_SHOWMODE)) {
      strcat(buffer, " )");
   }
   TRACE("=> '%s'\n", buffer);
   SetWindowTextA(mwi->hWnd, buffer);
}

static void MCIWND_Create(HWND hWnd, LPCREATESTRUCTA cs)
{
   MCI_DGV_OPEN_PARMSA	mdopn;
   MCI_DGV_RECT_PARMS	mdrct;
   MMRESULT		mmr;
   int			cx, cy;
   HWND			hChld;
   MCIWndInfo* 		mwi = (MCIWndInfo*)cs->lpCreateParams;

   SetWindowLongA(hWnd, 0, (LPARAM)mwi);
   mwi->hWnd = hWnd;

   /* now open MCI player for AVI file */
   memset(&mdopn, 0, sizeof(mdopn));
   mdopn.lpstrElementName = mwi->lpName;
   mdopn.dwStyle = WS_VISIBLE|WS_CHILD;
   mdopn.hWndParent = hWnd;

   mmr = mciSendCommandA(0,  MCI_OPEN, MCI_OPEN_ELEMENT|MCI_DGV_OPEN_PARENT|MCI_DGV_OPEN_WS, (LPARAM)&mdopn);
   if (mmr) {
      MessageBoxA(GetTopWindow(hWnd), "Cannot open file", "MciWnd", MB_OK);
      return;
   }
   mwi->mci = mdopn.wDeviceID;

   /* grab AVI window size */
   memset(&mdrct, 0, sizeof(mdrct));
   mmr = mciSendCommandA(mwi->mci,  MCI_WHERE, MCI_DGV_WHERE_DESTINATION, (LPARAM)&mdrct);
   if (mmr) {
      WARN("Cannot get window rect\n");
      return;
   }
   cx = mdrct.rc.right - mdrct.rc.left;
   cy = mdrct.rc.bottom - mdrct.rc.top;

   AdjustWindowRect(&mdrct.rc, GetWindowLongA(hWnd, GWL_STYLE), FALSE);
   SetWindowPos(hWnd, 0, 0, 0, mdrct.rc.right - mdrct.rc.left,
		mdrct.rc.bottom - mdrct.rc.top + 32, SWP_NOMOVE|SWP_NOZORDER);

   /* adding the other elements: play/stop button, menu button, status */
   hChld = CreateWindowExA(0, "BUTTON", "Play", WS_CHILD|WS_VISIBLE, 0, cy, 32, 32,
			   hWnd, (HMENU)CTL_PLAYSTOP, GetWindowLongA(hWnd, GWL_HINSTANCE), 0L);
   TRACE("Get Button1: %04x\n", hChld);
   hChld = CreateWindowExA(0, "BUTTON", "Menu", WS_CHILD|WS_VISIBLE, 32, cy, 32, 32,
			   hWnd, (HMENU)CTL_MENU, GetWindowLongA(hWnd, GWL_HINSTANCE), 0L);
   TRACE("Get Button2: %04x\n", hChld);
   hChld = CreateWindowExA(0, TRACKBAR_CLASSA, "", WS_CHILD|WS_VISIBLE, 64, cy, cx - 64, 32,
			   hWnd, (HMENU)CTL_TRACKBAR, GetWindowLongA(hWnd, GWL_HINSTANCE), 0L);
   TRACE("Get status: %04x\n", hChld);
   SendMessageA(hChld, TBM_SETRANGEMIN, 0L, 0L);
   SendMessageA(hChld, TBM_SETRANGEMAX, 1L, MCIWND_Get(mwi, MCI_STATUS_LENGTH));

   /* FIXME: no need to set it if child window */
   MCIWND_SetText(mwi);
}

static void MCIWND_Paint(MCIWndInfo* mwi, WPARAM wParam)
{
   HDC 		hdc;
   PAINTSTRUCT	ps;

   hdc = (wParam) ? (HDC)wParam : BeginPaint(mwi->hWnd, &ps);
   /* something to do ? */
   if (!wParam) EndPaint(mwi->hWnd, &ps);
}

static void MCIWND_ToggleState(MCIWndInfo* mwi)
{
   MCI_GENERIC_PARMS	mgp;
   MCI_DGV_PLAY_PARMS	mdply;

   memset(&mgp, 0, sizeof(mgp));
   memset(&mdply, 0, sizeof(mdply));

   switch (MCIWND_GetStatus(mwi)) {
   case MCI_MODE_NOT_READY:
   case MCI_MODE_RECORD:
   case MCI_MODE_SEEK:
   case MCI_MODE_OPEN:
      TRACE("Cannot do much...\n");
      break;
   case MCI_MODE_PAUSE:
      mciSendCommandA(mwi->mci, MCI_RESUME, MCI_WAIT, (LPARAM)&mgp);
      break;
   case MCI_MODE_PLAY:
      mciSendCommandA(mwi->mci, MCI_PAUSE, MCI_WAIT, (LPARAM)&mgp);
      break;
   case MCI_MODE_STOP:
      mdply.dwFrom = 0L;
      mciSendCommandA(mwi->mci, MCI_PLAY, MCI_FROM, (LPARAM)&mdply);
      mwi->uTimer = SetTimer(mwi->hWnd, 0, 333, 0L);
      TRACE("Timer=%u\n", mwi->uTimer);
      break;
   }
}

static LRESULT MCIWND_Command(MCIWndInfo* mwi, WPARAM wParam, LPARAM lParam)
{
   switch (LOWORD(wParam)) {
   case CTL_PLAYSTOP:	MCIWND_ToggleState(mwi);	break;
   case CTL_MENU:
   case CTL_TRACKBAR:
   default:
      MessageBoxA(0, "ooch", "NIY", MB_OK);
   }
   return 0L;
}

static void MCIWND_Timer(MCIWndInfo* mwi, WPARAM wParam, LPARAM lParam)
{
   TRACE("%ld\n", MCIWND_Get(mwi, MCI_STATUS_POSITION));
   SendDlgItemMessageA(mwi->hWnd, CTL_TRACKBAR, TBM_SETPOS, 1, MCIWND_Get(mwi, MCI_STATUS_POSITION));
   MCIWND_SetText(mwi);
}

static void MCIWND_Close(MCIWndInfo* mwi)
{
   MCI_GENERIC_PARMS	mgp;

   memset(&mgp, 0, sizeof(mgp));

   mciSendCommandA(mwi->mci, MCI_CLOSE, 0, (LPARAM)&mgp);
}

static LRESULT WINAPI	MCIWndProc(HWND hWnd, UINT wMsg, WPARAM lParam1, LPARAM lParam2)
{
   MCIWndInfo*	mwi = (MCIWndInfo*)GetWindowLongA(hWnd, 0);

   if (mwi || wMsg == WM_CREATE) {
      switch (wMsg) {
      case WM_CREATE:
	 MCIWND_Create(hWnd, (CREATESTRUCTA*)lParam2);
	 return 0;
      case WM_DESTROY:
	 MCIWND_Close(mwi);
	 HeapFree(GetProcessHeap(), 0, mwi->lpName);
	 HeapFree(GetProcessHeap(), 0, mwi);
	 break;
      case WM_PAINT:
	 MCIWND_Paint(mwi, lParam1);
	 break;
      case WM_COMMAND:
	 return MCIWND_Command(mwi, lParam1, lParam2);
      case WM_TIMER:
	 MCIWND_Timer(mwi, lParam1, lParam2);
	 return TRUE;
      }
   }

   return DefWindowProcA(hWnd, wMsg, lParam1, lParam2);
}

