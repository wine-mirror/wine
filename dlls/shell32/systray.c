/*
 *	Systray
 *
 *	Copyright 1999 Kai Morich	<kai.morich@bigfoot.de>
 *
 *  Manage the systray window. That it actually appears in the docking
 *  area of KDE or GNOME is delegated to windows/x11drv/wnd.c,
 *  X11DRV_WND_DockWindow.
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "shellapi.h"
#include "shell32_main.h"
#include "windows.h"
#include "commctrl.h"
#include "debugtools.h"
#include "config.h" 

DEFAULT_DEBUG_CHANNEL(shell)


typedef struct SystrayItem {
  NOTIFYICONDATAA    notifyIcon;
  int                nitem; /* number of current element = tooltip id */
  struct SystrayItem *next; /* nextright systray item */
} SystrayItem;


typedef struct SystrayData {
  int              hasCritSection;
  CRITICAL_SECTION critSection;
  HWND             hWnd;
  HWND             hWndToolTip;
  int              nitems; /* number of elements in systray */
  SystrayItem      *next;  /* leftmost systray item */
} SystrayData;


static SystrayData systray;

static BOOL SYSTRAY_Delete(PNOTIFYICONDATAA pnid);


/**************************************************************************
*  internal systray 
*
*/
#define SMALL_ICON_SIZE GetSystemMetrics(SM_CXSMICON)
/* space between icons */
#define SMALL_ICON_FILL 1
/* space between icons and frame */
#define IBORDER 3
#define OBORDER 2
#define TBORDER  (OBORDER+1+IBORDER) 

#define ICON_TOP(i)    (TBORDER)
#define ICON_LEFT(i)   (TBORDER+(i)*(SMALL_ICON_SIZE+SMALL_ICON_FILL))
#define ICON_RIGHT(i)  (TBORDER+(i)*(SMALL_ICON_SIZE+SMALL_ICON_FILL)+SMALL_ICON_SIZE)
#define ICON_BOTTOM(i) (TBORDER+SMALL_ICON_SIZE)

static LRESULT CALLBACK SYSTRAY_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  HDC hdc;
  PAINTSTRUCT ps;

  switch (message) {
  case WM_PAINT:
  {
    RECT rc;
    SystrayItem *trayItem = systray.next;

    hdc = BeginPaint(hWnd, &ps);
    GetClientRect(hWnd, &rc);
    /*
    rc.top    += OBORDER;
    rc.bottom -= OBORDER;
    rc.left   += OBORDER;
    rc.right  -= OBORDER;
    DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_TOPLEFT|BF_BOTTOMRIGHT);
    */
    rc.top = TBORDER;
    rc.left = TBORDER;
    while (trayItem) {
      if (trayItem->notifyIcon.hIcon)
        if (!DrawIconEx(hdc, rc.left, rc.top, trayItem->notifyIcon.hIcon, 
			SMALL_ICON_SIZE, SMALL_ICON_SIZE, 0, 0, DI_DEFAULTSIZE|DI_NORMAL))
          SYSTRAY_Delete(&trayItem->notifyIcon);
      trayItem = trayItem->next;
      rc.left += SMALL_ICON_SIZE+SMALL_ICON_FILL;
    }
    EndPaint(hWnd, &ps);
  } break;
  case WM_MOUSEMOVE:
  case WM_LBUTTONDOWN:
  case WM_LBUTTONUP:
  case WM_RBUTTONDOWN:
  case WM_RBUTTONUP:
  case WM_MBUTTONDOWN:
  case WM_MBUTTONUP:
  {
    MSG msg;
    RECT rect;
    GetWindowRect(hWnd, &rect);
    msg.hwnd=hWnd;
    msg.message=message;
    msg.wParam=wParam;
    msg.lParam=lParam;
    msg.time = GetMessageTime ();
    msg.pt.x = LOWORD(GetMessagePos ());
    msg.pt.y = HIWORD(GetMessagePos ());
    SendMessageA(systray.hWndToolTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
  }
  case WM_LBUTTONDBLCLK:
  case WM_RBUTTONDBLCLK:
  case WM_MBUTTONDBLCLK:
  {
    int xPos  = LOWORD(lParam);
    int xItem = TBORDER;
    SystrayItem *trayItem = systray.next;
    while (trayItem) {
      if (xPos>=xItem && xPos<(xItem+SMALL_ICON_SIZE)) {
        if (!PostMessageA(trayItem->notifyIcon.hWnd, trayItem->notifyIcon.uCallbackMessage,
			  (WPARAM)trayItem->notifyIcon.hIcon, (LPARAM)message))
          SYSTRAY_Delete(&trayItem->notifyIcon);
        break;
      }
      trayItem = trayItem->next;
      xItem += SMALL_ICON_SIZE+SMALL_ICON_FILL;
    }
  } break;
  default:
    return (DefWindowProcA(hWnd, message, wParam, lParam));
   }
   return (0);
}

BOOL SYSTRAY_Create(void)
{
  WNDCLASSA  wc;
  RECT rect;
  wc.style         = CS_SAVEBITS;
  wc.lpfnWndProc   = (WNDPROC)SYSTRAY_WndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = 0;
  wc.hIcon         = 0; /* LoadIcon (NULL, IDI_EXCLAMATION); */
  wc.hCursor       = LoadCursorA(0, IDC_ARROWA);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
  wc.lpszMenuName  = NULL;
  wc.lpszClassName = "WineSystray";
  
  TRACE("\n");
  if (!RegisterClassA(&wc)) {
    ERR("RegisterClass(WineSystray) failed\n");
    return FALSE;
  }
  rect.left   = 0;
  rect.top    = 0;
  rect.right  = SMALL_ICON_SIZE+2*TBORDER;
  rect.bottom = SMALL_ICON_SIZE+2*TBORDER;
/*  AdjustWindowRect(&rect, WS_CAPTION, FALSE);*/

  systray.hWnd  =  CreateWindowExA(
		  		WS_EX_TRAYWINDOW,
		  		"WineSystray", "Wine-Systray",
				WS_VISIBLE,
				CW_USEDEFAULT, CW_USEDEFAULT,
				rect.right-rect.left, rect.bottom-rect.top,
				0, 0, 0, 0);
  if (!systray.hWnd) {
    ERR("CreateWindow(WineSystray) failed\n");
    return FALSE;
  }
  systray.hWndToolTip = CreateWindowA(TOOLTIPS_CLASSA,NULL,TTS_ALWAYSTIP, 
				     CW_USEDEFAULT, CW_USEDEFAULT,
				     CW_USEDEFAULT, CW_USEDEFAULT, 
				     systray.hWnd, 0, 0, 0);
  if (systray.hWndToolTip==0) {
    ERR("CreateWindow(TOOLTIP) failed\n");
    return FALSE;
  }
  return TRUE;
}


static void SYSTRAY_RepaintItem(int nitem)
{
  if(nitem<0) { /* repaint all items */
    RECT rc1, rc2;
    GetWindowRect(systray.hWnd, &rc1);
    rc2.left   = 0;
    rc2.top    = 0;
    rc2.right  = systray.nitems*(SMALL_ICON_SIZE+SMALL_ICON_FILL)+2*TBORDER;
    rc2.bottom = SMALL_ICON_SIZE+2*TBORDER;
/*      TRACE("%d %d %d %d %d\n",systray.nitems, rc1.left, rc1.top, rc2.right-rc2.left, rc2.bottom-rc2.top); */
/*    AdjustWindowRect(&rc2, WS_CAPTION, FALSE);*/
    MoveWindow(systray.hWnd, rc1.left, rc1.top, rc2.right-rc2.left, rc2.bottom-rc2.top, TRUE);
    InvalidateRect(systray.hWnd, NULL, TRUE);
/*      TRACE("%d %d %d %d %d\n",systray.nitems, rc1.left, rc1.top, rc2.right-rc2.left, rc2.bottom-rc2.top); */
  } else {
    RECT rc;
    rc.left   = ICON_LEFT(nitem);
    rc.top    = ICON_TOP(nitem);
    rc.right  = ICON_RIGHT(nitem);
    rc.bottom = ICON_BOTTOM(nitem);
    InvalidateRect(systray.hWnd, &rc, TRUE);
  }
}

void SYSTRAY_InitItem(SystrayItem *trayItem)
{
  trayItem->nitem = systray.nitems++;
  /* create window only if needed */
  if (systray.hWnd==0)
    SYSTRAY_Create();
}

void SYSTRAY_SetIcon(SystrayItem *trayItem, HICON hIcon)
{
  trayItem->notifyIcon.hIcon = hIcon;
  SYSTRAY_RepaintItem(trayItem->nitem>systray.nitems?-1:trayItem->nitem);
}

void SYSTRAY_SetTip2(SystrayItem *trayItem)
{
  TTTOOLINFOA ti; 

  ti.cbSize = sizeof(TTTOOLINFOA);
  ti.uFlags = 0; 
  ti.hwnd = systray.hWnd; 
  ti.hinst = 0; 
  ti.uId = trayItem->nitem;
  ti.lpszText = trayItem->notifyIcon.szTip;
  ti.rect.left   = ICON_LEFT(trayItem->nitem);
  ti.rect.top    = ICON_TOP(trayItem->nitem);
  ti.rect.right  = ICON_RIGHT(trayItem->nitem);
  ti.rect.bottom = ICON_BOTTOM(trayItem->nitem);
  SendMessageA(systray.hWndToolTip, TTM_ADDTOOLA, 0, (LPARAM)&ti);
}

static void SYSTRAY_TermItem(SystrayItem *removeItem)
{
  int nitem;
  SystrayItem **trayItem;
  TTTOOLINFOA ti; 
  ti.cbSize = sizeof(TTTOOLINFOA);
  ti.uFlags = 0; 
  ti.hwnd = systray.hWnd; 
  ti.hinst = 0; 

  /* delete all tooltips ...*/
  trayItem = &systray.next;
  while (*trayItem) {
    ti.uId = (*trayItem)->nitem;
    SendMessageA(systray.hWndToolTip, TTM_DELTOOLA, 0, (LPARAM)&ti);
    trayItem = &((*trayItem)->next);
  }
  /* ... and add them again, because uID may shift */
  nitem=0;
  trayItem = &systray.next;
  while (*trayItem) {
    if (*trayItem != removeItem) {
      (*trayItem)->nitem = nitem;
      ti.uId = nitem;
      ti.lpszText = (*trayItem)->notifyIcon.szTip;
      ti.rect.left   = ICON_LEFT(nitem);
      ti.rect.top    = ICON_TOP(nitem);
      ti.rect.right  = ICON_RIGHT(nitem);
      ti.rect.bottom = ICON_BOTTOM(nitem);
      SendMessageA(systray.hWndToolTip, TTM_ADDTOOLA, 0, (LPARAM)&ti);
      nitem++;
    }
    trayItem = &((*trayItem)->next);
  }
  /* remove icon */
  SYSTRAY_RepaintItem(-1);
  systray.nitems--;
}

/**************************************************************************
*  helperfunctions 
*
*/
void SYSTRAY_SetMessage(SystrayItem *trayItem, UINT uCallbackMessage)
{
  trayItem->notifyIcon.uCallbackMessage = uCallbackMessage;
}


void SYSTRAY_SetTip(SystrayItem *trayItem, CHAR* szTip)
{
/*    char *s; */
  strncpy(trayItem->notifyIcon.szTip, szTip, sizeof(trayItem->notifyIcon.szTip));
  trayItem->notifyIcon.szTip[sizeof(trayItem->notifyIcon.szTip)-1]=0;
  /* cut off trailing spaces */
/*
  s=trayItem->notifyIcon.szTip+strlen(trayItem->notifyIcon.szTip);
  while (--s >= trayItem->notifyIcon.szTip && *s == ' ')
    *s=0;
*/
  SYSTRAY_SetTip2(trayItem);
}


static BOOL SYSTRAY_IsEqual(PNOTIFYICONDATAA pnid1, PNOTIFYICONDATAA pnid2)
{
  if (pnid1->hWnd != pnid2->hWnd) return FALSE;
  if (pnid1->uID  != pnid2->uID)  return FALSE;
  return TRUE;
}


static BOOL SYSTRAY_Add(PNOTIFYICONDATAA pnid)
{
  SystrayItem **trayItem = &systray.next;
  while (*trayItem) {
    if (SYSTRAY_IsEqual(pnid, &(*trayItem)->notifyIcon))
      return FALSE;
    trayItem = &((*trayItem)->next);
  }
  (*trayItem) = (SystrayItem*)malloc(sizeof(SystrayItem));
  memcpy(&(*trayItem)->notifyIcon, pnid, sizeof(NOTIFYICONDATAA));
  SYSTRAY_InitItem(*trayItem);
  SYSTRAY_SetIcon   (*trayItem, (pnid->uFlags&NIF_ICON)   ?pnid->hIcon           :0);
  SYSTRAY_SetMessage(*trayItem, (pnid->uFlags&NIF_MESSAGE)?pnid->uCallbackMessage:0);
  SYSTRAY_SetTip    (*trayItem, (pnid->uFlags&NIF_TIP)    ?pnid->szTip           :"");
  (*trayItem)->next = NULL;
  TRACE("%p: 0x%08x %d %s\n", *trayItem, (*trayItem)->notifyIcon.hWnd,
	(*trayItem)->notifyIcon.uID, (*trayItem)->notifyIcon.szTip);
  return TRUE;
}
    

static BOOL SYSTRAY_Modify(PNOTIFYICONDATAA pnid)
{
  SystrayItem *trayItem = systray.next;
  while (trayItem) {
    if (SYSTRAY_IsEqual(pnid, &trayItem->notifyIcon)) {
      if (pnid->uFlags & NIF_ICON)
        SYSTRAY_SetIcon(trayItem, pnid->hIcon);
      if (pnid->uFlags & NIF_MESSAGE)
        SYSTRAY_SetMessage(trayItem, pnid->uCallbackMessage);
      if (pnid->uFlags & NIF_TIP)
	SYSTRAY_SetTip(trayItem, pnid->szTip);
      TRACE("%p: 0x%08x %d %s\n", trayItem, trayItem->notifyIcon.hWnd, 
	    trayItem->notifyIcon.uID, trayItem->notifyIcon.szTip);
      return TRUE;
    }
    trayItem = trayItem->next;
  }
  return FALSE; /* not found */
}


static BOOL SYSTRAY_Delete(PNOTIFYICONDATAA pnid)
{
  SystrayItem **trayItem = &systray.next;
  while (*trayItem) {
    if (SYSTRAY_IsEqual(pnid, &(*trayItem)->notifyIcon)) {
      SystrayItem *next = (*trayItem)->next;
      TRACE("%p: 0x%08x %d %s\n", *trayItem, (*trayItem)->notifyIcon.hWnd,
	    (*trayItem)->notifyIcon.uID, (*trayItem)->notifyIcon.szTip);
      SYSTRAY_TermItem(*trayItem);
      free(*trayItem);
      *trayItem = next;

      return TRUE;
    }
    trayItem = &((*trayItem)->next);
  }

  return FALSE; /* not found */
}


/*************************************************************************
 *
 */
BOOL SYSTRAY_Init(void)
{
  if (!systray.hasCritSection) {
    systray.hasCritSection=1;
    InitializeCriticalSection(&systray.critSection);
    MakeCriticalSectionGlobal(&systray.critSection);
    TRACE(" =%p\n", &systray.critSection); 
  }
  return TRUE;
}


/*************************************************************************
 * Shell_NotifyIconA			[SHELL32.297]
 */
BOOL WINAPI Shell_NotifyIconA(DWORD dwMessage, PNOTIFYICONDATAA pnid )
{
  BOOL flag=FALSE;
  TRACE("wait %d %d %ld\n", pnid->hWnd, pnid->uID, dwMessage);
  /* must be serialized because all apps access same systray */
  EnterCriticalSection(&systray.critSection);
  TRACE("enter %d %d %ld\n", pnid->hWnd, pnid->uID, dwMessage);

  switch(dwMessage) {
  case NIM_ADD:
    flag = SYSTRAY_Add(pnid);
    break;
  case NIM_MODIFY:
    flag = SYSTRAY_Modify(pnid);
    break;
  case NIM_DELETE:
    flag = SYSTRAY_Delete(pnid);
    break;
  }

  LeaveCriticalSection(&systray.critSection);
  TRACE("leave %d %d %ld\n", pnid->hWnd, pnid->uID, dwMessage);
  return flag;
}

/*************************************************************************
 * Shell_NotifyIcon			[SHELL32.296]
 */
BOOL WINAPI Shell_NotifyIcon(DWORD dwMessage, PNOTIFYICONDATAA pnid)
{
  return Shell_NotifyIconA(dwMessage, pnid);
}
