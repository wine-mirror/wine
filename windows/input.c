/*
 * USER Input processing
 *
 * Copyright 1993 Bob Amstadt
 * Copyright 1996 Albrecht Kleine 
 * Copyright 1997 David Faure
 * Copyright 1998 Morten Welinder
 * Copyright 1998 Ulrich Weigand
 *
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "windows.h"
#include "win.h"
#include "gdi.h"
#include "heap.h"
#include "input.h"
#include "keyboard.h"
#include "mouse.h"
#include "message.h"
#include "debug.h"
#include "debugtools.h"
#include "struct32.h"
#include "winerror.h"

static INT16  captureHT = HTCLIENT;
static HWND32 captureWnd = 0;
static BOOL32 InputEnabled = TRUE;
static BOOL32 SwappedButtons = FALSE;

BOOL32 MouseButtonsStates[3];
BOOL32 AsyncMouseButtonsStates[3];
BYTE InputKeyStateTable[256];
BYTE QueueKeyStateTable[256];
BYTE AsyncKeyStateTable[256];

typedef union
{
    struct
    {
	unsigned long count : 16;
	unsigned long code : 8;
	unsigned long extended : 1;
	unsigned long unused : 2;
	unsigned long win_internal : 2;
	unsigned long context : 1;
	unsigned long previous : 1;
	unsigned long transition : 1;
    } lp1;
    unsigned long lp2;
} KEYLP;

/***********************************************************************
 *           keybd_event   (USER32.583)
 */
void WINAPI keybd_event( BYTE bVk, BYTE bScan,
                         DWORD dwFlags, DWORD dwExtraInfo )
{
    DWORD posX, posY, time, extra;
    WORD message;
    KEYLP keylp;
    keylp.lp2 = 0;

    if (!InputEnabled) return;

    /*
     * If we are called by the Wine keyboard driver, use the additional
     * info pointed to by the dwExtraInfo argument.
     * Otherwise, we need to determine that info ourselves (probably
     * less accurate, but we can't help that ...).
     */
    if (   !IsBadReadPtr32( (LPVOID)dwExtraInfo, sizeof(WINE_KEYBDEVENT) )
        && ((WINE_KEYBDEVENT *)dwExtraInfo)->magic == WINE_KEYBDEVENT_MAGIC )
    {
        WINE_KEYBDEVENT *wke = (WINE_KEYBDEVENT *)dwExtraInfo;
        posX = wke->posX;
        posY = wke->posY;
        time = wke->time;
        extra = 0;
    }
    else
    {
        DWORD keyState;
        time = GetTickCount();
        extra = dwExtraInfo;

        if ( !EVENT_QueryPointer( &posX, &posY, &keyState ))
            return;
    }


    keylp.lp1.count = 1;
    keylp.lp1.code = bScan;
    keylp.lp1.extended = (dwFlags & KEYEVENTF_EXTENDEDKEY) != 0;
    keylp.lp1.win_internal = 0; /* this has something to do with dialogs,
                                * don't remember where I read it - AK */
                                /* it's '1' under windows, when a dialog box appears
                                 * and you press one of the underlined keys - DF*/

    if ( dwFlags & KEYEVENTF_KEYUP )
    {
        BOOL32 sysKey = (InputKeyStateTable[VK_MENU] & 0x80)
                && !(InputKeyStateTable[VK_CONTROL] & 0x80)
                && !(dwFlags & KEYEVENTF_WINE_FORCEEXTENDED); /* for Alt from AltGr */

        InputKeyStateTable[bVk] &= ~0x80;
        keylp.lp1.previous = 1;
        keylp.lp1.transition = 1;
        message = sysKey ? WM_SYSKEYUP : WM_KEYUP;
    }
    else
    {
        keylp.lp1.previous = (InputKeyStateTable[bVk] & 0x80) != 0;
        keylp.lp1.transition = 0;

        if (!(InputKeyStateTable[bVk] & 0x80))
            InputKeyStateTable[bVk] ^= 0x01;
        InputKeyStateTable[bVk] |= 0x80;

        message = (InputKeyStateTable[VK_MENU] & 0x80)
              && !(InputKeyStateTable[VK_CONTROL] & 0x80)
              ? WM_SYSKEYDOWN : WM_KEYDOWN;
    }

    if ( message == WM_SYSKEYDOWN || message == WM_SYSKEYUP )
        keylp.lp1.context = (InputKeyStateTable[VK_MENU] & 0x80) != 0; /* 1 if alt */


    TRACE(key, "            wParam=%04X, lParam=%08lX\n", bVk, keylp.lp2 );
    TRACE(key, "            InputKeyState=%X\n", InputKeyStateTable[bVk] );

    hardware_event( message, bVk, keylp.lp2, posX, posY, time, extra );
}

/***********************************************************************
 *           mouse_event   (USER32.584)
 */
void WINAPI mouse_event( DWORD dwFlags, DWORD dx, DWORD dy,
                         DWORD cButtons, DWORD dwExtraInfo )
{
    DWORD posX, posY, keyState, time, extra;

    if (!InputEnabled) return;

    /*
     * If we are called by the Wine mouse driver, use the additional
     * info pointed to by the dwExtraInfo argument.
     * Otherwise, we need to determine that info ourselves (probably
     * less accurate, but we can't help that ...).
     */
    if (   !IsBadReadPtr32( (LPVOID)dwExtraInfo, sizeof(WINE_MOUSEEVENT) )
        && ((WINE_MOUSEEVENT *)dwExtraInfo)->magic == WINE_MOUSEEVENT_MAGIC )
    {
        WINE_MOUSEEVENT *wme = (WINE_MOUSEEVENT *)dwExtraInfo;
        keyState = wme->keyState;
        time = wme->time;
        extra = (DWORD)wme->hWnd;

        assert( dwFlags & MOUSEEVENTF_ABSOLUTE );
        posX = (dx * screenWidth) >> 16;
        posY = (dy * screenHeight) >> 16;
    }
    else
    {
        time = GetTickCount();
        extra = dwExtraInfo;

        if ( !EVENT_QueryPointer( &posX, &posY, &keyState ))
            return;

        if ( dwFlags & MOUSEEVENTF_MOVE )
        {
            if ( dwFlags & MOUSEEVENTF_ABSOLUTE )
            {
                posX = (dx * screenWidth) >> 16;
                posY = (dy * screenHeight) >> 16;
            }
            else
            {
                posX += dx;
                posY += dy;
            }
            /* We have to actually move the cursor */
            SetCursorPos32( posX, posY );
        }
    }

    if ( dwFlags & MOUSEEVENTF_MOVE )
    {
        hardware_event( WM_MOUSEMOVE,
                        keyState, 0L, posX, posY, time, extra );
    }
    if ( dwFlags & (!SwappedButtons? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN) )
    {
        MouseButtonsStates[0] = AsyncMouseButtonsStates[0] = TRUE;
        hardware_event( WM_LBUTTONDOWN,
                        keyState, 0L, posX, posY, time, extra );
    }
    if ( dwFlags & (!SwappedButtons? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP) )
    {
        MouseButtonsStates[0] = FALSE;
        hardware_event( WM_LBUTTONUP,
                        keyState, 0L, posX, posY, time, extra );
    }
    if ( dwFlags & (!SwappedButtons? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_LEFTDOWN) )
    {
        MouseButtonsStates[2] = AsyncMouseButtonsStates[2] = TRUE;
        hardware_event( WM_RBUTTONDOWN,
                        keyState, 0L, posX, posY, time, extra );
    }
    if ( dwFlags & (!SwappedButtons? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_LEFTUP) )
    {
        MouseButtonsStates[2] = FALSE;
        hardware_event( WM_RBUTTONUP,
                        keyState, 0L, posX, posY, time, extra );
    }
    if ( dwFlags & MOUSEEVENTF_MIDDLEDOWN )
    {
        MouseButtonsStates[1] = AsyncMouseButtonsStates[1] = TRUE;
        hardware_event( WM_MBUTTONDOWN,
                        keyState, 0L, posX, posY, time, extra );
    }
    if ( dwFlags & MOUSEEVENTF_MIDDLEUP )
    {
        MouseButtonsStates[1] = FALSE;
        hardware_event( WM_MBUTTONUP,
                        keyState, 0L, posX, posY, time, extra );
    }
}

/**********************************************************************
 *                      EnableHardwareInput   (USER.331)
 */
BOOL16 WINAPI EnableHardwareInput(BOOL16 bEnable)
{
  BOOL16 bOldState = InputEnabled;
  FIXME(event,"(%d) - stub\n", bEnable);
  InputEnabled = bEnable;
  return bOldState;
}


/***********************************************************************
 *           SwapMouseButton16   (USER.186)
 */
BOOL16 WINAPI SwapMouseButton16( BOOL16 fSwap )
{
    BOOL16 ret = SwappedButtons;
    SwappedButtons = fSwap;
    return ret;
}


/***********************************************************************
 *           SwapMouseButton32   (USER32.537)
 */
BOOL32 WINAPI SwapMouseButton32( BOOL32 fSwap )
{
    BOOL32 ret = SwappedButtons;
    SwappedButtons = fSwap;
    return ret;
}

/**********************************************************************
 *              EVENT_Capture
 *
 * We need this to be able to generate double click messages
 * when menu code captures mouse in the window without CS_DBLCLK style.
 */
HWND32 EVENT_Capture(HWND32 hwnd, INT16 ht)
{
    HWND32 capturePrev = captureWnd;

    if (!hwnd)
    {
        captureWnd = 0L;
        captureHT = 0;
    }
    else
    {
        WND* wndPtr = WIN_FindWndPtr( hwnd );
        if (wndPtr)
        {
            TRACE(win, "(0x%04x)\n", hwnd );
            captureWnd   = hwnd;
            captureHT    = ht;
        }
    }

    if( capturePrev && capturePrev != captureWnd )
    {
        WND* wndPtr = WIN_FindWndPtr( capturePrev );
        if( wndPtr && (wndPtr->flags & WIN_ISWIN32) )
            SendMessage32A( capturePrev, WM_CAPTURECHANGED, 0L, hwnd);
    }
    return capturePrev;
}

/**********************************************************************
 *              EVENT_GetCaptureInfo
 */
INT16 EVENT_GetCaptureInfo()
{
    return captureHT;
}

/**********************************************************************
 *              SetCapture16   (USER.18)
 */
HWND16 WINAPI SetCapture16( HWND16 hwnd )
{
    return (HWND16)EVENT_Capture( hwnd, HTCLIENT );
}


/**********************************************************************
 *              SetCapture32   (USER32.464)
 */
HWND32 WINAPI SetCapture32( HWND32 hwnd )
{
    return EVENT_Capture( hwnd, HTCLIENT );
}


/**********************************************************************
 *              ReleaseCapture   (USER.19) (USER32.439)
 */
void WINAPI ReleaseCapture(void)
{
    TRACE(win, "captureWnd=%04x\n", captureWnd );
    if( captureWnd ) EVENT_Capture( 0, 0 );
}


/**********************************************************************
 *              GetCapture16   (USER.236)
 */
HWND16 WINAPI GetCapture16(void)
{
    return captureWnd;
}

/**********************************************************************
 *              GetCapture32   (USER32.208)
 */
HWND32 WINAPI GetCapture32(void)
{
    return captureWnd;
}

/**********************************************************************
 *           GetKeyState      (USER.106)
 */
WORD WINAPI GetKeyState16(INT16 vkey)
{
    return GetKeyState32(vkey);
}

/**********************************************************************
 *           GetKeyState      (USER32.249)
 *
 * An application calls the GetKeyState function in response to a
 * keyboard-input message.  This function retrieves the state of the key
 * at the time the input message was generated.  (SDK 3.1 Vol 2. p 390)
 */
WORD WINAPI GetKeyState32(INT32 vkey)
{
    INT32 retval;

    switch (vkey)
	{
	case VK_LBUTTON : /* VK_LBUTTON is 1 */
	    retval = MouseButtonsStates[0] ? 0x8000 : 0;
	    break;
	case VK_MBUTTON : /* VK_MBUTTON is 4 */
	    retval = MouseButtonsStates[1] ? 0x8000 : 0;
	    break;
	case VK_RBUTTON : /* VK_RBUTTON is 2 */
	    retval = MouseButtonsStates[2] ? 0x8000 : 0;
	    break;
	default :
	    if (vkey >= 'a' && vkey <= 'z')
		vkey += 'A' - 'a';
	    retval = ( (WORD)(QueueKeyStateTable[vkey] & 0x80) << 8 ) |
		       (WORD)(QueueKeyStateTable[vkey] & 0x01);
	}
    /* TRACE(key, "(0x%x) -> %x\n", vkey, retval); */
    return retval;
}

/**********************************************************************
 *           GetKeyboardState      (USER.222)(USER32.254)
 *
 * An application calls the GetKeyboardState function in response to a
 * keyboard-input message.  This function retrieves the state of the keyboard
 * at the time the input message was generated.  (SDK 3.1 Vol 2. p 387)
 */
VOID WINAPI GetKeyboardState(LPBYTE lpKeyState)
{
    TRACE(key, "(%p)\n", lpKeyState);
    if (lpKeyState != NULL) {
	QueueKeyStateTable[VK_LBUTTON] = MouseButtonsStates[0] ? 0x80 : 0;
	QueueKeyStateTable[VK_MBUTTON] = MouseButtonsStates[1] ? 0x80 : 0;
	QueueKeyStateTable[VK_RBUTTON] = MouseButtonsStates[2] ? 0x80 : 0;
	memcpy(lpKeyState, QueueKeyStateTable, 256);
    }
}

/**********************************************************************
 *          SetKeyboardState      (USER.223)(USER32.484)
 */
VOID WINAPI SetKeyboardState(LPBYTE lpKeyState)
{
    TRACE(key, "(%p)\n", lpKeyState);
    if (lpKeyState != NULL) {
	memcpy(QueueKeyStateTable, lpKeyState, 256);
	MouseButtonsStates[0] = (QueueKeyStateTable[VK_LBUTTON] != 0);
	MouseButtonsStates[1] = (QueueKeyStateTable[VK_MBUTTON] != 0);
	MouseButtonsStates[2] = (QueueKeyStateTable[VK_RBUTTON] != 0);
    }
}

/**********************************************************************
 *           GetAsyncKeyState32      (USER32.207)
 *
 *	Determine if a key is or was pressed.  retval has high-order 
 * bit set to 1 if currently pressed, low-order bit set to 1 if key has
 * been pressed.
 *
 *	This uses the variable AsyncMouseButtonsStates and
 * AsyncKeyStateTable (set in event.c) which have the mouse button
 * number or key number (whichever is applicable) set to true if the
 * mouse or key had been depressed since the last call to 
 * GetAsyncKeyState.
 */
WORD WINAPI GetAsyncKeyState32(INT32 nKey)
{
    short retval;	

    switch (nKey) {
     case VK_LBUTTON:
	retval = (AsyncMouseButtonsStates[0] ? 0x0001 : 0) | 
                 (MouseButtonsStates[0] ? 0x8000 : 0);
	break;
     case VK_MBUTTON:
	retval = (AsyncMouseButtonsStates[1] ? 0x0001 : 0) | 
                 (MouseButtonsStates[1] ? 0x8000 : 0);
	break;
     case VK_RBUTTON:
	retval = (AsyncMouseButtonsStates[2] ? 0x0001 : 0) | 
                 (MouseButtonsStates[2] ? 0x8000 : 0);
	break;
     default:
	retval = AsyncKeyStateTable[nKey] | 
	  	((InputKeyStateTable[nKey] & 0x80) ? 0x8000 : 0); 
	break;
    }

    /* all states to false */
    memset( AsyncMouseButtonsStates, 0, sizeof(AsyncMouseButtonsStates) );
    memset( AsyncKeyStateTable, 0, sizeof(AsyncKeyStateTable) );

    TRACE(key, "(%x) -> %x\n", nKey, retval);
    return retval;
}

/**********************************************************************
 *            GetAsyncKeyState16        (USER.249)
 */
WORD WINAPI GetAsyncKeyState16(INT16 nKey)
{
    return GetAsyncKeyState32(nKey);
}

/**********************************************************************
 *           KBD_translate_accelerator
 *
 * FIXME: should send some WM_INITMENU or/and WM_INITMENUPOPUP  -messages
 */
static BOOL32 KBD_translate_accelerator(HWND32 hWnd,LPMSG32 msg,
                                        BYTE fVirt,WORD key,WORD cmd)
{
    BOOL32	sendmsg = FALSE;

    if(msg->wParam == key) 
    {
    	if (msg->message == WM_CHAR) {
        if ( !(fVirt & FALT) && !(fVirt & FVIRTKEY) )
        {
   	  TRACE(accel,"found accel for WM_CHAR: ('%c')\n",
			msg->wParam&0xff);
   	  sendmsg=TRUE;
   	}  
      } else {
       if(fVirt & FVIRTKEY) {
	INT32 mask = 0;
        TRACE(accel,"found accel for virt_key %04x (scan %04x)\n",
  	                       msg->wParam,0xff & HIWORD(msg->lParam));                
	if(GetKeyState32(VK_SHIFT) & 0x8000) mask |= FSHIFT;
	if(GetKeyState32(VK_CONTROL) & 0x8000) mask |= FCONTROL;
	if(GetKeyState32(VK_MENU) & 0x8000) mask |= FALT;
	if(mask == (fVirt & (FSHIFT | FCONTROL | FALT)))
          sendmsg=TRUE;			    
        else
          TRACE(accel,", but incorrect SHIFT/CTRL/ALT-state\n");
       }
       else
       {
         if (!(msg->lParam & 0x01000000))  /* no special_key */
         {
           if ((fVirt & FALT) && (msg->lParam & 0x20000000))
           {                                                   /* ^^ ALT pressed */
	    TRACE(accel,"found accel for Alt-%c\n", msg->wParam&0xff);
	    sendmsg=TRUE;	    
	   } 
         } 
       }
      } 

      if (sendmsg)      /* found an accelerator, but send a message... ? */
      {
        INT16  iSysStat,iStat,mesg=0;
        HMENU16 hMenu;
        
        if (msg->message == WM_KEYUP || msg->message == WM_SYSKEYUP)
          mesg=1;
        else 
         if (GetCapture32())
           mesg=2;
         else
          if (!IsWindowEnabled32(hWnd))
            mesg=3;
          else
          {
	    WND* wndPtr = WIN_FindWndPtr(hWnd);

            hMenu = (wndPtr->dwStyle & WS_CHILD) ? 0 : (HMENU32)wndPtr->wIDmenu;
	    iSysStat = (wndPtr->hSysMenu) ? GetMenuState32(GetSubMenu16(wndPtr->hSysMenu, 0),
					    cmd, MF_BYCOMMAND) : -1 ;
	    iStat = (hMenu) ? GetMenuState32(hMenu,
					    cmd, MF_BYCOMMAND) : -1 ;

            if (iSysStat!=-1)
            {
              if (iSysStat & (MF_DISABLED|MF_GRAYED))
                mesg=4;
              else
                mesg=WM_SYSCOMMAND;
            }
            else
            {
              if (iStat!=-1)
              {
                if (IsIconic32(hWnd))
                  mesg=5;
                else
                {
                 if (iStat & (MF_DISABLED|MF_GRAYED))
                   mesg=6;
                 else
                   mesg=WM_COMMAND;  
                }   
              }
              else
               mesg=WM_COMMAND;  
            }
          }
          if ( mesg==WM_COMMAND || mesg==WM_SYSCOMMAND )
          {
              TRACE(accel,", sending %s, wParam=%0x\n",
                  mesg==WM_COMMAND ? "WM_COMMAND" : "WM_SYSCOMMAND",
                  cmd);
	      SendMessage32A(hWnd, mesg, cmd, 0x00010000L);
	  }
	  else
	  {
	   /*  some reasons for NOT sending the WM_{SYS}COMMAND message: 
	    *   #0: unknown (please report!)
	    *   #1: for WM_KEYUP,WM_SYSKEYUP
	    *   #2: mouse is captured
	    *   #3: window is disabled 
	    *   #4: it's a disabled system menu option
	    *   #5: it's a menu option, but window is iconic
	    *   #6: it's a menu option, but disabled
	    */
	    TRACE(accel,", but won't send WM_{SYS}COMMAND, reason is #%d\n",mesg);
	    if(mesg==0)
	      ERR(accel, " unknown reason - please report!");
	  }          
          return TRUE;         
      }
    }
    return FALSE;
}

/**********************************************************************
 *      TranslateAccelerator32      (USER32.551)(USER32.552)(USER32.553)
 */
INT32 WINAPI TranslateAccelerator32(HWND32 hWnd, HACCEL32 hAccel, LPMSG32 msg)
{
    LPACCEL32	lpAccelTbl = (LPACCEL32)LockResource32(hAccel);
    int 	i;

    TRACE(accel,"hwnd=0x%x hacc=0x%x msg=0x%x wp=0x%x lp=0x%lx\n", hWnd, hAccel, msg->message, msg->wParam, msg->lParam);
    
    if (hAccel == 0 || msg == NULL ||
	(msg->message != WM_KEYDOWN &&
	 msg->message != WM_KEYUP &&
	 msg->message != WM_SYSKEYDOWN &&
	 msg->message != WM_SYSKEYUP &&
	 msg->message != WM_CHAR)) {
      WARN(accel, "erraneous input parameters\n");
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
    }

    TRACE(accel, "TranslateAccelerators hAccel=%04x, hWnd=%04x,"
	  "msg->hwnd=%04x, msg->message=%04x\n",
	  hAccel,hWnd,msg->hwnd,msg->message);

    i = 0;
    do
    {
    	if (KBD_translate_accelerator(hWnd,msg,lpAccelTbl[i].fVirt,
                                      lpAccelTbl[i].key,lpAccelTbl[i].cmd))
		return 1;
    } while ((lpAccelTbl[i++].fVirt & 0x80) == 0);
    WARN(accel, "couldn't translate accelerator key\n");
    return 0;
}

/**********************************************************************
 *           TranslateAccelerator16      (USER.178)
 */	
INT16 WINAPI TranslateAccelerator16(HWND16 hWnd, HACCEL16 hAccel, LPMSG16 msg)
{
    LPACCEL16	lpAccelTbl = (LPACCEL16)LockResource16(hAccel);
    int 	i;
    MSG32	msg32;
    
    if (hAccel == 0 || msg == NULL ||
	(msg->message != WM_KEYDOWN &&
	 msg->message != WM_KEYUP &&
	 msg->message != WM_SYSKEYDOWN &&
	 msg->message != WM_SYSKEYUP &&
	 msg->message != WM_CHAR)) {
      WARN(accel, "erraneous input parameters\n");
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
    }

    TRACE(accel, "TranslateAccelerators hAccel=%04x, hWnd=%04x,\
msg->hwnd=%04x, msg->message=%04x\n", hAccel,hWnd,msg->hwnd,msg->message);
    STRUCT32_MSG16to32(msg,&msg32);


    i = 0;
    do
    {
    	if (KBD_translate_accelerator(hWnd,&msg32,lpAccelTbl[i].fVirt,
                                      lpAccelTbl[i].key,lpAccelTbl[i].cmd))
		return 1;
    } while ((lpAccelTbl[i++].fVirt & 0x80) == 0);
    WARN(accel, "couldn't translate accelerator key\n");
    return 0;
}


/**********************************************************************
 *           VkKeyScanA      (USER32.573)
 */
WORD WINAPI VkKeyScan32A(CHAR cChar)
{
	return VkKeyScan16(cChar);
}

/******************************************************************************
 *    	VkKeyScanW      (USER32.576)
 */
WORD WINAPI VkKeyScan32W(WCHAR cChar)
{
	return VkKeyScan32A((CHAR)cChar); /* FIXME: check unicode */
}

/******************************************************************************
 *    	GetKeyboardType32      (USER32.255)
 */
INT32 WINAPI GetKeyboardType32(INT32 nTypeFlag)
{
  return GetKeyboardType16(nTypeFlag);
}

/******************************************************************************
 *    	MapVirtualKey32A      (USER32.383)
 */
UINT32 WINAPI MapVirtualKey32A(UINT32 code, UINT32 maptype)
{
    return MapVirtualKey16(code,maptype);
}

/******************************************************************************
 *    	MapVirtualKey32W      (USER32.385)
 */
UINT32 WINAPI MapVirtualKey32W(UINT32 code, UINT32 maptype)
{
    return MapVirtualKey16(code,maptype);
}

/****************************************************************************
 *	GetKBCodePage32   (USER32.246)
 */
UINT32 WINAPI GetKBCodePage32(void)
{
    return GetKBCodePage16();
}

/****************************************************************************
 *	GetKeyboardLayoutName32A   (USER32.252)
 */
INT32 WINAPI GetKeyboardLayoutName32A(LPSTR pwszKLID)
{
	return GetKeyboardLayoutName16(pwszKLID);
}

/****************************************************************************
 *	GetKeyboardLayoutName32W   (USER32.253)
 */
INT32 WINAPI GetKeyboardLayoutName32W(LPWSTR pwszKLID)
{
	LPSTR buf = HEAP_xalloc( GetProcessHeap(), 0, strlen("00000409")+1);
	int res = GetKeyboardLayoutName32A(buf);
	lstrcpyAtoW(pwszKLID,buf);
	HeapFree( GetProcessHeap(), 0, buf );
	return res;
}

/****************************************************************************
 *	GetKeyNameText32A   (USER32.247)
 */
INT32 WINAPI GetKeyNameText32A(LONG lParam, LPSTR lpBuffer, INT32 nSize)
{
	return GetKeyNameText16(lParam,lpBuffer,nSize);
}

/****************************************************************************
 *	GetKeyNameText32W   (USER32.248)
 */
INT32 WINAPI GetKeyNameText32W(LONG lParam, LPWSTR lpBuffer, INT32 nSize)
{
	LPSTR buf = HEAP_xalloc( GetProcessHeap(), 0, nSize );
	int res = GetKeyNameText32A(lParam,buf,nSize);

	lstrcpynAtoW(lpBuffer,buf,nSize);
	HeapFree( GetProcessHeap(), 0, buf );
	return res;
}

/****************************************************************************
 *	ToAscii32      (USER32.546)
 */
INT32 WINAPI ToAscii32( UINT32 virtKey,UINT32 scanCode,LPBYTE lpKeyState,
                        LPWORD lpChar,UINT32 flags )
{
    return ToAscii16(virtKey,scanCode,lpKeyState,lpChar,flags);
}

/***********************************************************************
 *           GetKeyboardLayout			(USER32.250)
 */
HKL32 WINAPI GetKeyboardLayout(DWORD dwLayout)
{
        HKL32 layout;
	FIXME(keyboard,"(%ld): stub\n",dwLayout);
        layout = (0xcafe<<16)|GetSystemDefaultLCID(); /* FIXME */
        TRACE(keyboard,"returning %x\n",layout);
        return layout;
}

/***********************************************************************
 *           GetKeyboardLayoutList		(USER32.251)
 * FIXME
 */
INT32 WINAPI GetKeyboardLayoutList(INT32 nBuff,HKL32 *layouts)
{
	FIXME(keyboard,"(%d,%p): stub\n",nBuff,layouts);
	if (layouts)
		layouts[0] = GetKeyboardLayout(0);
	return 1;
}


/***********************************************************************
 *           RegisterHotKey			(USER32.433)
 */
BOOL32 WINAPI RegisterHotKey(HWND32 hwnd,INT32 id,UINT32 modifiers,UINT32 vk) {
	FIXME(keyboard,"(0x%08x,%d,0x%08x,%d): stub\n",hwnd,id,modifiers,vk);
	return TRUE;
}

/***********************************************************************
 *           UnregisterHotKey			(USER32.565)
 */
BOOL32 WINAPI UnregisterHotKey(HWND32 hwnd,INT32 id) {
	FIXME(keyboard,"(0x%08x,%d): stub\n",hwnd,id);
	return TRUE;
}

