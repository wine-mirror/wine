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

#include "heap.h"
#include "shellapi.h"
#include "shell32_main.h"
#include "windows.h"
#include "commctrl.h"
#include "debugtools.h"
#include "config.h"

DEFAULT_DEBUG_CHANNEL(shell)

typedef struct SystrayData {
  HWND                  hWnd;
  HWND                  hWndToolTip;
  NOTIFYICONDATAA       notifyIcon;
  int                   nitem; /* number of current element = tooltip id */
  struct SystrayData    *nextTrayItem;
} SystrayData;

typedef struct Systray {
  int              hasCritSection;
  CRITICAL_SECTION critSection;
  SystrayData      *systrayItemList;
} Systray;

static Systray systray;
static int nNumberTrayElements;


static BOOL SYSTRAY_Delete(PNOTIFYICONDATAA pnid);


/**************************************************************************
*  internal systray
*
*/

#define SMALL_ICON_SIZE GetSystemMetrics(SM_CXSMICON)

/* space between icons and frame */
#define IBORDER 3
#define OBORDER 2
#define TBORDER  (OBORDER+1+IBORDER)

static LRESULT CALLBACK SYSTRAY_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  HDC hdc;
  PAINTSTRUCT ps;

  switch (message) {
  case WM_PAINT:
  {
    RECT rc;
    SystrayData  *ptrayItem = systray.systrayItemList;

    while (ptrayItem)
    {
      if (ptrayItem->hWnd == hWnd)
      {
        hdc = BeginPaint(hWnd, &ps);
        GetClientRect(hWnd, &rc);

        if (!DrawIconEx(hdc, rc.left, rc.top, ptrayItem->notifyIcon.hIcon,
                        SMALL_ICON_SIZE, SMALL_ICON_SIZE, 0, 0, DI_DEFAULTSIZE|DI_NORMAL))
          SYSTRAY_Delete(&ptrayItem->notifyIcon);
      }
      ptrayItem = ptrayItem->nextTrayItem;
    }
    EndPaint(hWnd, &ps);
  }
  break;

  case WM_MOUSEMOVE:
  case WM_LBUTTONDOWN:
  case WM_LBUTTONUP:
  case WM_RBUTTONDOWN:
  case WM_RBUTTONUP:
  case WM_MBUTTONDOWN:
  case WM_MBUTTONUP:
  {
    MSG msg;
    SystrayData *ptrayItem = systray.systrayItemList;

    while ( ptrayItem )
    {
      if (ptrayItem->hWnd == hWnd)
      {
        msg.hwnd=hWnd;
        msg.message=message;
        msg.wParam=wParam;
        msg.lParam=lParam;
        msg.time = GetMessageTime ();
        msg.pt.x = LOWORD(GetMessagePos ());
        msg.pt.y = HIWORD(GetMessagePos ());

        SendMessageA(ptrayItem->hWndToolTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
      }
      ptrayItem = ptrayItem->nextTrayItem;
    }
  }

  case WM_LBUTTONDBLCLK:
  case WM_RBUTTONDBLCLK:
  case WM_MBUTTONDBLCLK:
  {
    int xPos;
    SystrayData *ptrayItem = systray.systrayItemList;

    while (ptrayItem)
    {
      if (ptrayItem->hWnd == hWnd)
      {
        xPos = LOWORD(lParam);
        if( (xPos >= TBORDER) &&
            (xPos < (TBORDER+SMALL_ICON_SIZE)) )
        {
          if (!PostMessageA(ptrayItem->notifyIcon.hWnd, ptrayItem->notifyIcon.uCallbackMessage,
                            (WPARAM)ptrayItem->notifyIcon.uID, (LPARAM)message))
            SYSTRAY_Delete(&ptrayItem->notifyIcon);
          break;
        }
      }
      ptrayItem = ptrayItem->nextTrayItem;
    }
  }
  break;

  default:
    return (DefWindowProcA(hWnd, message, wParam, lParam));
  }
  return (0);

}

BOOL SYSTRAY_RegisterClass(void)
{
  WNDCLASSA  wc;

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

  if (!RegisterClassA(&wc)) {
    ERR("RegisterClass(WineSystray) failed\n");
    return FALSE;
  }
  return TRUE;
}


BOOL SYSTRAY_Create(SystrayData *ptrayItem)
{
  RECT rect;

  /* Register the class if this is our first tray item. */
  if ( nNumberTrayElements == 1 )
  {
    if ( !SYSTRAY_RegisterClass() )
    {
      ERR( "RegisterClass(WineSystray) failed\n" );
      return FALSE;
    }
  }

  /* Initialize the window size. */
  rect.left   = 0;
  rect.top    = 0;
  rect.right  = SMALL_ICON_SIZE+2*TBORDER;
  rect.bottom = SMALL_ICON_SIZE+2*TBORDER;

  /* Create tray window for icon. */
  ptrayItem->hWnd = CreateWindowExA( WS_EX_TRAYWINDOW,
                                "WineSystray", "Wine-Systray",
                                WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                rect.right-rect.left, rect.bottom-rect.top,
                                0, 0, 0, 0 );
  if ( !ptrayItem->hWnd )
  {
    ERR( "CreateWindow(WineSystray) failed\n" );
    return FALSE;
  }

  /* Create tooltip for icon. */
  ptrayItem->hWndToolTip = CreateWindowA( TOOLTIPS_CLASSA,NULL,TTS_ALWAYSTIP,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     ptrayItem->hWnd, 0, 0, 0 );
  if ( ptrayItem->hWndToolTip==0 )
  {
    ERR( "CreateWindow(TOOLTIP) failed\n" );
    return FALSE;
  }
  return TRUE;
}

static void SYSTRAY_RepaintAll(void)
{
  SystrayData  *ptrayItem = systray.systrayItemList;

  while(ptrayItem)
  {
    InvalidateRect(ptrayItem->hWnd, NULL, TRUE);
    ptrayItem = ptrayItem->nextTrayItem;
  }

}

static void SYSTRAY_RepaintItem(int nitem)
{

  SystrayData *ptrayItem = systray.systrayItemList;

  while(ptrayItem)
  {
    if (ptrayItem->nitem == nitem)
      InvalidateRect(ptrayItem->hWnd, NULL, TRUE);

    ptrayItem = ptrayItem->nextTrayItem;
  }
}

void SYSTRAY_InitItem(SystrayData *ptrayItem)
{
  ptrayItem->nitem = nNumberTrayElements++;
  SYSTRAY_Create(ptrayItem);
}

void SYSTRAY_SetIcon(SystrayData *ptrayItem, HICON hIcon)
{
  ptrayItem->notifyIcon.hIcon = hIcon;
}

void SYSTRAY_SetTip(SystrayData *ptrayItem, CHAR* szTip)
{
  TTTOOLINFOA ti;

  strncpy(ptrayItem->notifyIcon.szTip, szTip, sizeof(ptrayItem->notifyIcon.szTip));
  ptrayItem->notifyIcon.szTip[sizeof(ptrayItem->notifyIcon.szTip)-1]=0;

  ti.cbSize = sizeof(TTTOOLINFOA);
  ti.uFlags = 0;
  ti.hwnd = ptrayItem->hWnd;
  ti.hinst = 0;
  ti.uId = ptrayItem->nitem;
  ti.lpszText = ptrayItem->notifyIcon.szTip;
  ti.rect.left   = 0;
  ti.rect.top    = 0;
  ti.rect.right  = SMALL_ICON_SIZE+2*TBORDER;
  ti.rect.bottom = SMALL_ICON_SIZE+2*TBORDER;

  SendMessageA(ptrayItem->hWndToolTip, TTM_ADDTOOLA, 0, (LPARAM)&ti);
}

static void SYSTRAY_ModifyTip(SystrayData *ptrayItem, CHAR* szTip)
{
  TTTOOLINFOA ti;

  strncpy(ptrayItem->notifyIcon.szTip, szTip, sizeof(ptrayItem->notifyIcon.szTip));
  ptrayItem->notifyIcon.szTip[sizeof(ptrayItem->notifyIcon.szTip)-1]=0;

  ti.cbSize = sizeof(TTTOOLINFOA);
  ti.uFlags = 0;
  ti.hwnd = ptrayItem->hWnd;
  ti.hinst = 0;
  ti.uId = ptrayItem->nitem;
  ti.lpszText = ptrayItem->notifyIcon.szTip;
  ti.rect.left   = 0;
  ti.rect.top    = 0;
  ti.rect.right  = SMALL_ICON_SIZE+2*TBORDER;
  ti.rect.bottom = SMALL_ICON_SIZE+2*TBORDER;

  SendMessageA(ptrayItem->hWndToolTip, TTM_UPDATETIPTEXTA, 0, (LPARAM)&ti);
}

static void SYSTRAY_TermItem(SystrayData *removeItem)
{
  int nitem;
  SystrayData **trayItem;
  TTTOOLINFOA ti;
  ti.cbSize = sizeof(TTTOOLINFOA);
  ti.uFlags = 0;
  ti.hinst = 0;

  /* delete all tooltips ...*/
  trayItem = &systray.systrayItemList;
  while (*trayItem) {
    ti.uId = (*trayItem)->nitem;
    ti.hwnd = (*trayItem)->hWnd;
    SendMessageA((*trayItem)->hWndToolTip, TTM_DELTOOLA, 0, (LPARAM)&ti);
    trayItem = &((*trayItem)->nextTrayItem);
  }
  /* ... and add them again, because uID may shift */
  nitem=0;
  trayItem = &systray.systrayItemList;
  while (*trayItem)
  {
    if (*trayItem != removeItem)
    {
      (*trayItem)->nitem = nitem;
      ti.uId = nitem;
      ti.hwnd = (*trayItem)->hWnd;
      ti.lpszText = (*trayItem)->notifyIcon.szTip;
      ti.rect.left   = 0;
      ti.rect.top    = 0;
      ti.rect.right  = SMALL_ICON_SIZE+2*TBORDER;
      ti.rect.bottom = SMALL_ICON_SIZE+2*TBORDER;
      SendMessageA((*trayItem)->hWndToolTip, TTM_ADDTOOLA, 0, (LPARAM)&ti);
      nitem++;
    }
    trayItem = &((*trayItem)->nextTrayItem);
  }
  nNumberTrayElements--;
}


/**************************************************************************
*  helperfunctions
*
*/
void SYSTRAY_SetMessage(SystrayData *ptrayItem, UINT uCallbackMessage)
{
  ptrayItem->notifyIcon.uCallbackMessage = uCallbackMessage;
}

static BOOL SYSTRAY_IsEqual(PNOTIFYICONDATAA pnid1, PNOTIFYICONDATAA pnid2)
{
  if (pnid1->hWnd != pnid2->hWnd) return FALSE;
  if (pnid1->uID  != pnid2->uID)  return FALSE;
  return TRUE;
}

static BOOL SYSTRAY_Add(PNOTIFYICONDATAA pnid)
{
  SystrayData **ptrayItem = &systray.systrayItemList;

  /* Find empty space for new element. */
  while( *ptrayItem )
  {
    if ( SYSTRAY_IsEqual(pnid, &(*ptrayItem)->notifyIcon) )
      return FALSE;
    ptrayItem = &((*ptrayItem)->nextTrayItem);
  }

  /* Allocate SystrayData for element and zero memory. */
  (*ptrayItem) = ( SystrayData *)malloc( sizeof(SystrayData) );
  ZeroMemory( (*ptrayItem), sizeof(SystrayData) );

  /* Copy notification data */
  memcpy( &(*ptrayItem)->notifyIcon, pnid, sizeof(NOTIFYICONDATAA) );

  /* Initialize and set data for the tray element. */
  SYSTRAY_InitItem( (*ptrayItem) );

  SYSTRAY_SetIcon   (*ptrayItem, (pnid->uFlags&NIF_ICON)   ?pnid->hIcon           :0);
  SYSTRAY_SetMessage(*ptrayItem, (pnid->uFlags&NIF_MESSAGE)?pnid->uCallbackMessage:0);
  SYSTRAY_SetTip    (*ptrayItem, (pnid->uFlags&NIF_TIP)    ?pnid->szTip           :"");

  (*ptrayItem)->nextTrayItem = NULL; /* may be overkill after the ZeroMemory call. */

  /* Repaint all system tray icons as we have added one. */
  SYSTRAY_RepaintAll();

  TRACE("%p: 0x%08x %d %s\n",  (*ptrayItem), (*ptrayItem)->notifyIcon.hWnd,
                               (*ptrayItem)->notifyIcon.uID,
                               (*ptrayItem)->notifyIcon.szTip);
  return TRUE;
}

static BOOL SYSTRAY_Modify(PNOTIFYICONDATAA pnid)
{
  SystrayData *ptrayItem = systray.systrayItemList;

  while ( ptrayItem )
  {
    if ( SYSTRAY_IsEqual(pnid, &ptrayItem->notifyIcon) )
    {
      if (pnid->uFlags & NIF_ICON)
      {
        SYSTRAY_SetIcon(ptrayItem, pnid->hIcon);
        SYSTRAY_RepaintItem(ptrayItem->nitem);
      }

      if (pnid->uFlags & NIF_MESSAGE)
        SYSTRAY_SetMessage(ptrayItem, pnid->uCallbackMessage);

      if (pnid->uFlags & NIF_TIP)
        SYSTRAY_ModifyTip(ptrayItem, pnid->szTip);

      TRACE("%p: 0x%08x %d %s\n", ptrayItem, ptrayItem->notifyIcon.hWnd,
            ptrayItem->notifyIcon.uID, ptrayItem->notifyIcon.szTip);
      return TRUE;
    }
    ptrayItem = ptrayItem->nextTrayItem;
  }
  return FALSE; /* not found */
}

static BOOL SYSTRAY_Delete(PNOTIFYICONDATAA pnid)
{
  SystrayData **ptrayItem = &systray.systrayItemList;

  while (*ptrayItem)
  {
    if (SYSTRAY_IsEqual(pnid, &(*ptrayItem)->notifyIcon))
    {
      SystrayData *next = (*ptrayItem)->nextTrayItem;
      TRACE("%p: 0x%08x %d %s\n", *ptrayItem, (*ptrayItem)->notifyIcon.hWnd,
            (*ptrayItem)->notifyIcon.uID, (*ptrayItem)->notifyIcon.szTip);
      SYSTRAY_TermItem(*ptrayItem);

      DestroyWindow((*ptrayItem)->hWndToolTip);
      DestroyWindow((*ptrayItem)->hWnd);

      free(*ptrayItem);
      *ptrayItem = next;

      SYSTRAY_RepaintAll();

      return TRUE;
    }
    ptrayItem = &((*ptrayItem)->nextTrayItem);
  }

  return FALSE; /* not found */
}

/*************************************************************************
 *
 */
BOOL SYSTRAY_Init(void)
{
  if (!systray.hasCritSection)
  {
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
    TRACE("Calling systray add\n");
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
 * Shell_NotifyIconA			[SHELL32.297]
 */
BOOL WINAPI Shell_NotifyIconW (DWORD dwMessage, PNOTIFYICONDATAW pnid )
{
	BOOL ret;

	PNOTIFYICONDATAA p = HeapAlloc(GetProcessHeap(),0,sizeof(NOTIFYICONDATAA));
	memcpy(p, pnid, sizeof(NOTIFYICONDATAA));
	if (*(pnid->szTip))
	  lstrcpynWtoA (p->szTip, pnid->szTip, 64 );

	ret = Shell_NotifyIconA(dwMessage, p );

	HeapFree(GetProcessHeap(),0,p);
	return ret;
}
/*************************************************************************
 * Shell_NotifyIcon			[SHELL32.296]
 */
BOOL WINAPI Shell_NotifyIcon(DWORD dwMessage, PNOTIFYICONDATAA pnid)
{
  return Shell_NotifyIconA(dwMessage, pnid);
}
