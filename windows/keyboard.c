/*
 * Keyboard related functions
 *
 * Copyright 1993 Bob Amstadt
 * Copyright 1996 Albrecht Kleine 
 */

#include <stdio.h>
#include <string.h>
#include "win.h"
#include "windows.h"
#include "accel.h"
#include "stddebug.h"
#include "debug.h"

extern BOOL MouseButtonsStates[3];
extern BOOL AsyncMouseButtonsStates[3];
extern BYTE InputKeyStateTable[256];
BYTE AsyncKeyStateTable[256];

extern BYTE QueueKeyStateTable[256];

/**********************************************************************
 *		GetKeyState			[USER.106]
 * An application calls the GetKeyState function in response to a
 * keyboard-input message.  This function retrieves the state of the key
 * at the time the input message was generated.  (SDK 3.1 Vol 2. p 390)
 */
INT GetKeyState(INT vkey)
{
    INT retval;

    switch (vkey)
	{
	case VK_LBUTTON : /* VK_LBUTTON is 1 */
	    retval = MouseButtonsStates[0];
	    break;
	case VK_MBUTTON : /* VK_MBUTTON is 4 */
	    retval = MouseButtonsStates[1];
	    break;
	case VK_RBUTTON : /* VK_RBUTTON is 2 */
	    retval = MouseButtonsStates[2];
	    break;
	default :
	    if (vkey >= 'a' && vkey <= 'z')
		vkey += 'A' - 'a';
	    retval = ( (INT)(QueueKeyStateTable[vkey] & 0x80) << 8 ) |
		       (INT)(QueueKeyStateTable[vkey] & 0x01);
	}
    dprintf_key(stddeb, "GetKeyState(0x%x) -> %x\n", vkey, retval);
    return retval;
}

/**********************************************************************
 *		GetKeyboardState			[USER.222]
 * An application calls the GetKeyboardState function in response to a
 * keyboard-input message.  This function retrieves the state of the keyboard
 * at the time the input message was generated.  (SDK 3.1 Vol 2. p 387)
 */
void GetKeyboardState(BYTE *lpKeyState)
{
    dprintf_key(stddeb, "GetKeyboardState()\n");
    if (lpKeyState != NULL) {
	QueueKeyStateTable[VK_LBUTTON] = MouseButtonsStates[0] >> 8;
	QueueKeyStateTable[VK_MBUTTON] = MouseButtonsStates[1] >> 8;
	QueueKeyStateTable[VK_RBUTTON] = MouseButtonsStates[2] >> 8;
	memcpy(lpKeyState, QueueKeyStateTable, 256);
    }
}

/**********************************************************************
 *      SetKeyboardState            [USER.223]
 */
void SetKeyboardState(BYTE *lpKeyState)
{
    dprintf_key(stddeb, "SetKeyboardState()\n");
    if (lpKeyState != NULL) {
	memcpy(QueueKeyStateTable, lpKeyState, 256);
	MouseButtonsStates[0] = QueueKeyStateTable[VK_LBUTTON]? 0x8000: 0;
	MouseButtonsStates[1] = QueueKeyStateTable[VK_MBUTTON]? 0x8000: 0;
	MouseButtonsStates[2] = QueueKeyStateTable[VK_RBUTTON]? 0x8000: 0;
    }
}

/**********************************************************************
 *            GetAsyncKeyState        (USER.249)
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
int GetAsyncKeyState(int nKey)
{
    short retval;	

    switch (nKey) {
     case VK_LBUTTON:
	retval = AsyncMouseButtonsStates[0] | 
	MouseButtonsStates[0]? 0x0001: 0;
	break;
     case VK_MBUTTON:
	retval = AsyncMouseButtonsStates[1] |
	MouseButtonsStates[1]? 0x0001: 0;
	break;
     case VK_RBUTTON:
	retval = AsyncMouseButtonsStates[2] |
	MouseButtonsStates[2]? 0x0001: 0;
	break;
     default:
	retval = AsyncKeyStateTable[nKey] | 
	(InputKeyStateTable[nKey] ? 0x8000 : 0);
	break;
    }

    memset( AsyncMouseButtonsStates, 0, 3 );  /* all states to false */
    memset( AsyncKeyStateTable, 0, 256 );

    dprintf_key(stddeb, "GetAsyncKeyState(%x) -> %x\n", nKey, retval);
    return retval;
}


/**********************************************************************
 *			TranslateAccelerator 	[USER.178]
 *
 * FIXME: should send some WM_INITMENU or/and WM_INITMENUPOPUP  -messages
 */
INT16 TranslateAccelerator(HWND hWnd, HACCEL16 hAccel, LPMSG16 msg)
{
    ACCELHEADER	*lpAccelTbl;
    int 	i;
    BOOL sendmsg;
    
    if (hAccel == 0 || msg == NULL) return 0;
    if (msg->message != WM_KEYDOWN &&
    	msg->message != WM_KEYUP &&
	msg->message != WM_SYSKEYDOWN &&
	msg->message != WM_SYSKEYUP &&
    	msg->message != WM_CHAR) return 0;

    dprintf_accel(stddeb, "TranslateAccelerators hAccel=%04x, hWnd=%04x,\
msg->hwnd=%04x, msg->message=%04x\n", hAccel,hWnd,msg->hwnd,msg->message);

    lpAccelTbl = (LPACCELHEADER)GlobalLock16(hAccel);
    for (sendmsg= i = 0; i < lpAccelTbl->wCount; i++) 
    {
     if(msg->wParam == lpAccelTbl->tbl[i].wEvent) 
     {
      if (msg->message == WM_CHAR) 
      {
        if ( !(lpAccelTbl->tbl[i].type & ALT_ACCEL) && 
             !(lpAccelTbl->tbl[i].type & VIRTKEY_ACCEL) )
        {
   	  dprintf_accel(stddeb,"found accel for WM_CHAR: ('%c')",msg->wParam&0xff);
   	  sendmsg=TRUE;
   	}  
      }
      else
      {
       if(lpAccelTbl->tbl[i].type & VIRTKEY_ACCEL) 
       {
	INT mask = 0;
        dprintf_accel(stddeb,"found accel for virt_key %04x (scan %04x)",
  	                       msg->wParam,0xff & HIWORD(msg->lParam));                
	if(GetKeyState(VK_SHIFT) & 0x8000) mask |= SHIFT_ACCEL;
	if(GetKeyState(VK_CONTROL) & 0x8000) mask |= CONTROL_ACCEL;
	if(GetKeyState(VK_MENU) & 0x8000) mask |= ALT_ACCEL;
	if(mask == (lpAccelTbl->tbl[i].type &
			    (SHIFT_ACCEL | CONTROL_ACCEL | ALT_ACCEL)))
          sendmsg=TRUE;			    
        else
          dprintf_accel(stddeb,", but incorrect SHIFT/CTRL/ALT-state\n");
       }
       else
       {
         if (!(msg->lParam & 0x01000000))  /* no special_key */
         {
           if ((lpAccelTbl->tbl[i].type & ALT_ACCEL) && (msg->lParam & 0x20000000))
           {                                                   /* ^^ ALT pressed */
	    dprintf_accel(stddeb,"found accel for Alt-%c", msg->wParam&0xff);
	    sendmsg=TRUE;	    
	   } 
         } 
       }
      } 

      if (sendmsg)      /* found an accelerator, but send a message... ? */
      {
        INT16  iSysStat,iStat,mesg=0;
        HMENU16 hSysMenu,hMenu;
        
        if (msg->message == WM_KEYUP || msg->message == WM_SYSKEYUP)
          mesg=1;
        else 
         if (GetCapture16())
           mesg=2;
         else
          if (!IsWindowEnabled(hWnd))
            mesg=3;
          else
          {
            hMenu=GetMenu32(hWnd);
            hSysMenu=GetSystemMenu32(hWnd,FALSE);
            if (hSysMenu)
              iSysStat=GetMenuState32(hSysMenu,lpAccelTbl->tbl[i].wIDval,MF_BYCOMMAND);
            else
              iSysStat=-1;
            if (hMenu)
              iStat=GetMenuState32(hMenu,lpAccelTbl->tbl[i].wIDval,MF_BYCOMMAND);
            else
              iStat=-1;
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
                if (IsIconic(hWnd))
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
              dprintf_accel(stddeb,", sending %s, wParam=%0x\n",
                  mesg==WM_COMMAND ? "WM_COMMAND" : "WM_SYSCOMMAND",
                  lpAccelTbl->tbl[i].wIDval);
	      SendMessage16(hWnd, mesg, lpAccelTbl->tbl[i].wIDval,0x00010000L);
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
	    dprintf_accel(stddeb,", but won't send WM_{SYS}COMMAND, reason is #%d\n",mesg);
	  }          
          GlobalUnlock16(hAccel);
          return 1;         
      }
     }
    }
    GlobalUnlock16(hAccel);
    return 0;
}
