/*
 * X11 windows driver
 *
 * Copyright 1993 Bob Amstadt
 * Copyright 1996 Albrecht Kleine 
 * Copyright 1997 David Faure
 * Copyright 1998 Morten Welinder
 * Copyright 1998 Ulrich Weigand
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include "ts_xlib.h"
#include "ts_xresource.h"
#include "ts_xutil.h"

#include "debug.h"
#include "keyboard.h"
#include "message.h"
#include "wintypes.h"
#include "x11drv.h"

extern LPBYTE pKeyStateTable;

int min_keycode, max_keycode, keysyms_per_keycode;
WORD keyc2vkey[256];

static int NumLockMask, AltGrMask; /* mask in the XKeyEvent state */
static int kcControl, kcAlt, kcShift, kcNumLock, kcCapsLock; /* keycodes */

/* Keyboard translation tables */
static const int special_key[] =
{
    VK_BACK, VK_TAB, 0, VK_CLEAR, 0, VK_RETURN, 0, 0,           /* FF08 */
    0, 0, 0, VK_PAUSE, VK_SCROLL, 0, 0, 0,                      /* FF10 */
    0, 0, 0, VK_ESCAPE                                          /* FF18 */
};

static const int cursor_key[] =
{
    VK_HOME, VK_LEFT, VK_UP, VK_RIGHT, VK_DOWN, VK_PRIOR, 
                                       VK_NEXT, VK_END          /* FF50 */
};

static const int misc_key[] =
{
    VK_SELECT, VK_SNAPSHOT, VK_EXECUTE, VK_INSERT, 0, 0, 0, 0,  /* FF60 */
    VK_CANCEL, VK_HELP, VK_CANCEL, VK_MENU                      /* FF68 */
};

static const int keypad_key[] =
{
    0, VK_NUMLOCK,                                        	/* FF7E */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF80 */
    0, 0, 0, 0, 0, VK_RETURN, 0, 0,                             /* FF88 */
    0, 0, 0, 0, 0, VK_HOME, VK_LEFT, VK_UP,                     /* FF90 */
    VK_RIGHT, VK_DOWN, VK_PRIOR, VK_NEXT, VK_END, 0,
				 VK_INSERT, VK_DELETE,          /* FF98 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FFA0 */
    0, 0, VK_MULTIPLY, VK_ADD, VK_SEPARATOR, VK_SUBTRACT, 
                               VK_DECIMAL, VK_DIVIDE,           /* FFA8 */
    VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4,
                            VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7, /* FFB0 */
    VK_NUMPAD8, VK_NUMPAD9                                      /* FFB8 */
};
    
static const int function_key[] =
{
    VK_F1, VK_F2,                                               /* FFBE */
    VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10,    /* FFC0 */
    VK_F11, VK_F12, VK_F13, VK_F14, VK_F15, VK_F16              /* FFC8 */
};

static const int modifier_key[] =
{
    VK_SHIFT, VK_SHIFT, VK_CONTROL, VK_CONTROL, VK_CAPITAL, 0, /* FFE1 */
    VK_MENU, VK_MENU, VK_MENU, VK_MENU                         /* FFE7 */
};

/* Returns the Windows virtual key code associated with the X event <e> */
static WORD EVENT_event_to_vkey( XKeyEvent *e)
{
    KeySym keysym;

    TSXLookupString(e, NULL, 0, &keysym, NULL);

    if ((keysym >= 0xFFAE) && (keysym <= 0xFFB9) && (e->state & NumLockMask)) 
        /* Only the Keypad keys 0-9 and . send different keysyms
         * depending on the NumLock state */
        return keypad_key[(keysym & 0xFF) - 0x7E];

    return keyc2vkey[e->keycode];
}

static BOOL NumState=FALSE, CapsState=FALSE;

/**********************************************************************
 *		KEYBOARD_GenerateMsg
 *
 * Generate Down+Up messages when NumLock or CapsLock is pressed.
 *
 * Convention : called with vkey only VK_NUMLOCK or VK_CAPITAL
 *
 */
void KEYBOARD_GenerateMsg( WORD vkey, int Evtype, INT event_x, INT event_y,
                           DWORD event_time )
{
  BOOL * State = (vkey==VK_NUMLOCK? &NumState : &CapsState);

  if (*State) {
    /* The INTERMEDIARY state means : just after a 'press' event, if a 'release' event comes,
       don't treat it. It's from the same key press. Then the state goes to ON.
       And from there, a 'release' event will switch off the toggle key. */
    *State=FALSE;
    TRACE(keyboard,"INTERM : don\'t treat release of toggle key. InputKeyStateTable[%#x] = %#x\n",vkey,pKeyStateTable[vkey]);
  } else
    {
	if ( pKeyStateTable[vkey] & 0x1 ) /* it was ON */
	  {
	    if (Evtype!=KeyPress)
	      {
		TRACE(keyboard,"ON + KeyRelease => generating DOWN and UP messages.\n");
	        KEYBOARD_SendEvent( vkey, 0, 0,
                                    event_x, event_y, event_time );
	        KEYBOARD_SendEvent( vkey, 0, KEYEVENTF_KEYUP, 
                                    event_x, event_y, event_time );
		*State=FALSE;
		pKeyStateTable[vkey] &= ~0x01; /* Toggle state to off. */ 
	      } 
	  }
	else /* it was OFF */
	  if (Evtype==KeyPress)
	    {
	      TRACE(keyboard,"OFF + Keypress => generating DOWN and UP messages.\n");
	      KEYBOARD_SendEvent( vkey, 0, 0,
                                  event_x, event_y, event_time );
	      KEYBOARD_SendEvent( vkey, 0, KEYEVENTF_KEYUP, 
                                  event_x, event_y, event_time );
	      *State=TRUE; /* Goes to intermediary state before going to ON */
	      pKeyStateTable[vkey] |= 0x01; /* Toggle state to on. */
	    }
    }
}

/***********************************************************************
 *           KEYBOARD_UpdateOneState
 *
 * Updates internal state for <vkey>, depending on key <state> under X
 *
 */
void KEYBOARD_UpdateOneState ( int vkey, int state )
{
    /* Do something if internal table state != X state for keycode */
    if (((pKeyStateTable[vkey] & 0x80)!=0) != state)
    {
        TRACE(keyboard,"Adjusting state for vkey %#.2x. State before %#.2x \n",
              vkey, pKeyStateTable[vkey]);

        /* Fake key being pressed inside wine */
	KEYBOARD_SendEvent( vkey, 0, state? 0 : KEYEVENTF_KEYUP, 
                            0, 0, GetTickCount() );

        TRACE(keyboard,"State after %#.2x \n",pKeyStateTable[vkey]);
    }
}

/***********************************************************************
 *           X11DRV_KEYBOARD_UpdateState
 *
 * Update modifiers state (Ctrl, Alt, Shift)
 * when window is activated (called by EVENT_FocusIn in event.c)
 *
 * This handles the case where one uses Ctrl+... Alt+... or Shift+.. to switch
 * from wine to another application and back.
 * Toggle keys are handled in HandleEvent. (because XQueryKeymap says nothing
 *  about them)
 */
void X11DRV_KEYBOARD_UpdateState ( void )
{
/* extract a bit from the char[32] bit suite */
#define KeyState(keycode) ((keys_return[keycode/8] & (1<<(keycode%8)))!=0)

    char keys_return[32];

    TRACE(keyboard,"called\n");
    if (!TSXQueryKeymap(display, keys_return)) {
        ERR(keyboard,"Error getting keymap !");
        return;
    }

    /* Adjust the ALT and CONTROL state if any has been changed outside wine */
    KEYBOARD_UpdateOneState(VK_MENU, KeyState(kcAlt));
    KEYBOARD_UpdateOneState(VK_CONTROL, KeyState(kcControl));
    KEYBOARD_UpdateOneState(VK_SHIFT, KeyState(kcShift));
#undef KeyState
}

/***********************************************************************
 *           X11DRV_KEYBOARD_HandleEvent
 *
 * Handle a X key event
 */
void X11DRV_KEYBOARD_HandleEvent( WND *pWnd, XKeyEvent *event )
{
    char Str[24]; 
    XComposeStatus cs; 
    KeySym keysym;
    WORD vkey = 0, bScan;
    DWORD dwFlags;
    static BOOL force_extended = FALSE; /* hack for AltGr translation */
    
    int ascii_chars;

    INT event_x = (pWnd? pWnd->rectWindow.left : 0) + event->x;
    INT event_y = (pWnd? pWnd->rectWindow.top  : 0) + event->y;
    DWORD event_time = event->time - MSG_WineStartTicks;

    /* this allows support for dead keys */
    if ((event->keycode >> 8) == 0x10)
	event->keycode=(event->keycode & 0xff);

    ascii_chars = TSXLookupString(event, Str, 1, &keysym, &cs);

    TRACE(key, "EVENT_key : state = %X\n", event->state);
    if (keysym == XK_Mode_switch)
	{
	TRACE(key, "Alt Gr key event received\n");
	event->keycode = kcControl; /* Simulate Control */
	X11DRV_KEYBOARD_HandleEvent( pWnd, event );

	event->keycode = kcAlt; /* Simulate Alt */
	force_extended = TRUE;
	X11DRV_KEYBOARD_HandleEvent( pWnd, event );
	force_extended = FALSE;
	return;
	}

    Str[ascii_chars] = '\0';
    if (TRACE_ON(key)){
	char	*ksname;

	ksname = TSXKeysymToString(keysym);
	if (!ksname)
	  ksname = "No Name";
	TRACE(key, "%s : keysym=%lX (%s), ascii chars=%u / %X / '%s'\n", 
		     (event->type == KeyPress) ? "KeyPress" : "KeyRelease",
		     keysym, ksname, ascii_chars, Str[0] & 0xff, Str);
    }

    vkey = EVENT_event_to_vkey(event);
    if (force_extended) vkey |= 0x100;

    TRACE(key, "keycode 0x%x converted to vkey 0x%x\n",
		    event->keycode, vkey);

   if (vkey)
   {
    switch (vkey & 0xff)
    {
    case VK_NUMLOCK:    
      KEYBOARD_GenerateMsg( VK_NUMLOCK, event->type, event_x, event_y,
                            event_time );
      break;
    case VK_CAPITAL:
      TRACE(keyboard,"Caps Lock event. (type %d). State before : %#.2x\n",event->type,pKeyStateTable[vkey]);
      KEYBOARD_GenerateMsg( VK_CAPITAL, event->type, event_x, event_y,
                            event_time ); 
      TRACE(keyboard,"State after : %#.2x\n",pKeyStateTable[vkey]);
      break;
    default:
        /* Adjust the NUMLOCK state if it has been changed outside wine */
	if (!(pKeyStateTable[VK_NUMLOCK] & 0x01) != !(event->state & NumLockMask))
	  { 
	    TRACE(keyboard,"Adjusting NumLock state. \n");
	    KEYBOARD_GenerateMsg( VK_NUMLOCK, KeyPress, event_x, event_y,
                                  event_time );
	    KEYBOARD_GenerateMsg( VK_NUMLOCK, KeyRelease, event_x, event_y,
                                  event_time );
	  }
        /* Adjust the CAPSLOCK state if it has been changed outside wine */
	if (!(pKeyStateTable[VK_CAPITAL] & 0x01) != !(event->state & LockMask))
	  {
              TRACE(keyboard,"Adjusting Caps Lock state.\n");
	    KEYBOARD_GenerateMsg( VK_CAPITAL, KeyPress, event_x, event_y,
                                  event_time );
	    KEYBOARD_GenerateMsg( VK_CAPITAL, KeyRelease, event_x, event_y,
                                  event_time );
	  }
	/* Not Num nor Caps : end of intermediary states for both. */
	NumState = FALSE;
	CapsState = FALSE;

	switch (keysym) {
	    /* Windows expects extended keys to generate the unexetended scan
	       code and then set the extended flag */
	    case XK_Control_R :	bScan = TSXKeysymToKeycode(display, XK_Control_L); break;
	    case XK_Alt_R :	bScan = TSXKeysymToKeycode(display, XK_Alt_L); break;
	    case XK_Insert :	bScan = TSXKeysymToKeycode(display, XK_KP_Insert); break;
	    case XK_Delete :	bScan = TSXKeysymToKeycode(display, XK_KP_Delete); break;
	    case XK_Home :	bScan = TSXKeysymToKeycode(display, XK_KP_Home); break;
	    case XK_End :	bScan = TSXKeysymToKeycode(display, XK_KP_End); break;
	    case XK_Prior :	bScan = TSXKeysymToKeycode(display, XK_KP_Prior); break;
	    case XK_Next :	bScan = TSXKeysymToKeycode(display, XK_KP_Next); break;
	    case XK_Left :	bScan = TSXKeysymToKeycode(display, XK_KP_Left); break;
	    case XK_Up :	bScan = TSXKeysymToKeycode(display, XK_KP_Up); break;
	    case XK_Right :	bScan = TSXKeysymToKeycode(display, XK_KP_Right); break;
	    case XK_Down :	bScan = TSXKeysymToKeycode(display, XK_KP_Down); break;
	    case XK_KP_Divide : bScan = TSXKeysymToKeycode(display, XK_slash); break;
	    case XK_KP_Enter :	bScan = TSXKeysymToKeycode(display, XK_Return); break;
	    default:
		bScan = event->keycode;
		break;
	}
	bScan -= min_keycode - 1; /* Windows starts from 1, 
                                     X from min_keycode (8 usually) */
	TRACE(key, "bScan = 0x%02x.\n", bScan);

	dwFlags = 0;
	if ( event->type == KeyRelease ) dwFlags |= KEYEVENTF_KEYUP;
	if ( vkey & 0x100 )              dwFlags |= KEYEVENTF_EXTENDEDKEY;
	if ( force_extended )            dwFlags |= KEYEVENTF_WINE_FORCEEXTENDED;

	KEYBOARD_SendEvent( vkey & 0xff, bScan, dwFlags, 
                            event_x, event_y, event_time );
    }
   }
}

/**********************************************************************
 *		X11DRV_KEYBOARD_Init
 */
void X11DRV_KEYBOARD_Init(void)
{
    int i;
    KeySym *ksp;
    XModifierKeymap *mmp;
    KeySym keysym;
    KeyCode *kcp;
    XKeyEvent e2;
    WORD vkey, OEMvkey;
    int keyc;

    TSXDisplayKeycodes(display, &min_keycode, &max_keycode);
    ksp = TSXGetKeyboardMapping(display, min_keycode,
                              max_keycode + 1 - min_keycode, &keysyms_per_keycode);
    /* We are only interested in keysyms_per_keycode.
       There is no need to hold a local copy of the keysyms table */
    TSXFree(ksp);
    mmp = TSXGetModifierMapping(display);
    kcp = mmp->modifiermap;
    for (i = 0; i < 8; i += 1) /* There are 8 modifier keys */
    {
        int j;
        
        for (j = 0; j < mmp->max_keypermod; j += 1, kcp += 1)
	    if (*kcp)
            {
		int k;
                
		for (k = 0; k < keysyms_per_keycode; k += 1)
                    if (TSXKeycodeToKeysym(display, *kcp, k) == XK_Mode_switch)
		    {
                        AltGrMask = 1 << i;
                        TRACE(key, "AltGrMask is %x\n", AltGrMask);
		    }
                    else if (TSXKeycodeToKeysym(display, *kcp, k) == XK_Num_Lock)
		    {
                        NumLockMask = 1 << i;
                        TRACE(key, "NumLockMask is %x\n", NumLockMask);
		    }
            }
    }
    TSXFreeModifiermap(mmp);

    /* Now build two conversion arrays :
     * keycode -> vkey + extended
     * vkey + extended -> keycode */

    e2.display = display;
    e2.state = 0;

    OEMvkey = VK_OEM_7; /* next is available.  */
    for (keyc = min_keycode; keyc <= max_keycode; keyc++)
    {
        e2.keycode = (KeyCode)keyc;
        TSXLookupString(&e2, NULL, 0, &keysym, NULL);
        vkey = 0;
        if (keysym)  /* otherwise, keycode not used */
        {
            if ((keysym >> 8) == 0xFF)         /* non-character key */
            {
                int key = keysym & 0xff;
		
                if (key >= 0x08 && key <= 0x1B)         /* special key */
		    vkey = special_key[key - 0x08];
                else if (key >= 0x50 && key <= 0x57)    /* cursor key */
		    vkey = cursor_key[key - 0x50];
                else if (key >= 0x60 && key <= 0x6B)    /* miscellaneous key */
		    vkey = misc_key[key - 0x60];
                else if (key >= 0x7E && key <= 0xB9)    /* keypad key */
		    vkey = keypad_key[key - 0x7E];
                else if (key >= 0xBE && key <= 0xCD)    /* function key */
                {
                    vkey = function_key[key - 0xBE];
                    vkey |= 0x100; /* set extended bit */
                }
                else if (key >= 0xE1 && key <= 0xEA)    /* modifier key */
		    vkey = modifier_key[key - 0xE1];
                else if (key == 0xFF)                   /* DEL key */
		    vkey = VK_DELETE;
                /* extended must also be set for ALT_R, CTRL_R,
                   INS, DEL, HOME, END, PAGE_UP, PAGE_DOWN, ARROW keys,
                   keypad / and keypad ENTER (SDK 3.1 Vol.3 p 138) */
                /* FIXME should we set extended bit for NumLock ? My
                 * Windows does ... DF */
                switch (keysym)
                {
                case XK_Control_R :
                case XK_Alt_R :
                case XK_Insert :
                case XK_Delete :
                case XK_Home :
                case XK_End :
                case XK_Prior :
                case XK_Next :
                case XK_Left :
                case XK_Up :
                case XK_Right :
                case XK_Down :
                case XK_KP_Divide :
                case XK_KP_Enter :
                    vkey |= 0x100;
                }
            }
            for (i = 0; (i < keysyms_per_keycode) && (!vkey); i++)
            {
                keysym = TSXLookupKeysym(&e2, i);
                if ((keysym >= VK_0 && keysym <= VK_9)
                    || (keysym >= VK_A && keysym <= VK_Z)
                    || keysym == VK_SPACE)
		    vkey = keysym;
            }

            for (i = 0; (i < keysyms_per_keycode) && (!vkey); i++)
            {
                keysym = TSXLookupKeysym(&e2, i);
		switch (keysym)
		{
		case ';':             vkey = VK_OEM_1; break;
		case '/':             vkey = VK_OEM_2; break;
		case '`':             vkey = VK_OEM_3; break;
		case '[':             vkey = VK_OEM_4; break;
		case '\\':            vkey = VK_OEM_5; break;
		case ']':             vkey = VK_OEM_6; break;
		case '\'':            vkey = VK_OEM_7; break;
		case ',':             vkey = VK_OEM_COMMA; break;
		case '.':             vkey = VK_OEM_PERIOD; break;
		case '-':             vkey = VK_OEM_MINUS; break;
		case '+':             vkey = VK_OEM_PLUS; break;
		}
	    }

            if (!vkey)
            {
                /* Others keys: let's assign OEM virtual key codes in the allowed range,
                 * that is ([0xba,0xc0], [0xdb,0xe4], 0xe6 (given up) et [0xe9,0xf5]) */
                switch (++OEMvkey)
                {
                case 0xc1 : OEMvkey=0xdb; break;
                case 0xe5 : OEMvkey=0xe9; break;
                case 0xf6 : OEMvkey=0xf5; WARN(keyboard,"No more OEM vkey available!\n");
                }

                vkey = OEMvkey;
		  
                if (TRACE_ON(keyboard))
                {
		    dbg_decl_str(keyboard, 1024);

                    TRACE(keyboard, "OEM specific virtual key %X assigned "
				 "to keycode %X:\n", OEMvkey, e2.keycode);
                    for (i = 0; i < keysyms_per_keycode; i += 1)
                    {
                        char	*ksname;
                        
                        keysym = TSXLookupKeysym(&e2, i);
                        ksname = TSXKeysymToString(keysym);
                        if (!ksname)
			    ksname = "NoSymbol";
                        dsprintf(keyboard, "%lX (%s) ", keysym, ksname);
                    }
                    TRACE(keyboard, "(%s)\n", dbg_str(keyboard));
                }
            }
        }
        keyc2vkey[e2.keycode] = vkey;
    } /* for */
    /* Now store one keycode for each modifier. Used to simulate keypresses. */
    kcControl = TSXKeysymToKeycode(display, XK_Control_L);
    kcAlt = TSXKeysymToKeycode(display, XK_Alt_L);
    if (!kcAlt) kcAlt = TSXKeysymToKeycode(display, XK_Meta_L);
    kcShift = TSXKeysymToKeycode(display, XK_Shift_L);
    kcNumLock = TSXKeysymToKeycode(display, XK_Num_Lock);
    kcCapsLock = TSXKeysymToKeycode(display, XK_Caps_Lock);
}

/***********************************************************************
 *		X11DRV_KEYBOARD_VkKeyScan
 */
WORD X11DRV_KEYBOARD_VkKeyScan(CHAR cChar)
{
	KeyCode keycode;
	KeySym keysym;    	
	int i,index;
	int highbyte=0;

	/* char->keysym (same for ANSI chars) */
	keysym=(unsigned char) cChar;/* (!) cChar is signed */
	if (keysym<=27) keysym+=0xFF00;/*special chars : return, backspace...*/
	
	keycode = TSXKeysymToKeycode(display, keysym);  /* keysym -> keycode */
	if (!keycode)
	{ /* It didn't work ... let's try with deadchar code. */
	  keycode = TSXKeysymToKeycode(display, keysym | 0xFE00);
	}

	TRACE(keyboard,"VkKeyScan '%c'(%#lx, %lu): got keycode %#.2x\n",
			 cChar,keysym,keysym,keycode);
	
	if (keycode)
	  {
	    for (index=-1, i=0; (i<8) && (index<0); i++) /* find shift state */
	      if (TSXKeycodeToKeysym(display,keycode,i)==keysym) index=i;
	    switch (index) {
	    case -1 :
	      WARN(keyboard,"Keysym %lx not found while parsing the keycode table\n",keysym); break;
	    case 0 : break;
	    case 1 : highbyte = 0x0100; break;
	    case 2 : highbyte = 0X0600; break;
	    default : ERR(keyboard,"index %d found by XKeycodeToKeysym. please report! \n",index);
	    }
	    /*
	      index : 0     adds 0x0000
	      index : 1     adds 0x0100 (shift)
	      index : ?     adds 0x0200 (ctrl)
	      index : 2     adds 0x0600 (ctrl+alt)
	      index : ?     adds 0x0700 (ctrl+alt+shift (used?))
	     */
	  }
	TRACE(keyboard," ... returning %#.2x\n", keyc2vkey[keycode]+highbyte);
	return keyc2vkey[keycode]+highbyte;   /* keycode -> (keyc2vkey) vkey */
}

/***********************************************************************
 *		X11DRV_KEYBOARD_MapVirtualKey
 */
UINT16 X11DRV_KEYBOARD_MapVirtualKey(UINT16 wCode, UINT16 wMapType)
{
#define returnMVK(value) { TRACE(keyboard,"returning 0x%x.\n",value); return value; }

	TRACE(keyboard,"MapVirtualKey wCode=0x%x wMapType=%d ... \n",
			 wCode,wMapType);
	switch(wMapType) {
		case 0:	{ /* vkey-code to scan-code */
			/* let's do vkey -> keycode -> scan */
			int keyc;
			for (keyc=min_keycode; keyc<=max_keycode; keyc++)
				if ((keyc2vkey[keyc] & 0xFF) == wCode)
					returnMVK (keyc - min_keycode);
		        return 0; }

		case 1: /* scan-code to vkey-code */
			/* let's do scan -> keycode -> vkey */

			returnMVK (keyc2vkey[(wCode & 0xFF) + min_keycode]);

		case 2: { /* vkey-code to unshifted ANSI code */
			/* (was FIXME) : what does unshifted mean ? 'a' or 'A' ? */
		        /* My Windows returns 'A'. */
			/* let's do vkey -> keycode -> (XLookupString) ansi char */
			XKeyEvent e;
			KeySym keysym;
			char s[2];
			e.display = display;
			e.state = 0; /* unshifted */
			e.keycode = MapVirtualKey16( wCode, 0);
			if (!TSXLookupString(&e, s , 2 , &keysym, NULL))
			  returnMVK (*s);
			
			return 0;
			}
		default: /* reserved */
			WARN(keyboard, "Unknown wMapType %d !\n",
				wMapType);
			return 0;	
	}
	return 0;
}

/***********************************************************************
 *		X11DRV_KEYBOARD_GetKeyNameText
 */
INT16 X11DRV_KEYBOARD_GetKeyNameText(LONG lParam, LPSTR lpBuffer, INT16 nSize)
{
#if 0
  int i;
#endif
	
  FIXME(keyboard,"(%ld,<ptr>,%d): stub\n",lParam,nSize);

#if 0
  lParam >>= 16;
  lParam &= 0xff;
  
  for (i = 0 ; i != KeyTableSize ; i++) 
    if (KeyTable[i].scancode == lParam)  {
      lstrcpynA( lpBuffer, KeyTable[i].name, nSize );
      return strlen(lpBuffer);
    }
#endif

  *lpBuffer = 0;
  return 0;
}

/***********************************************************************
 *		X11DRV_KEYBOARD_ToAscii
 *
 * The ToAscii function translates the specified virtual-key code and keyboard
 * state to the corresponding Windows character or characters.
 *
 * If the specified key is a dead key, the return value is negative. Otherwise,
 * it is one of the following values:
 * Value	Meaning
 * 0	The specified virtual key has no translation for the current state of the keyboard.
 * 1	One Windows character was copied to the buffer.
 * 2	Two characters were copied to the buffer. This usually happens when a
 *      dead-key character (accent or diacritic) stored in the keyboard layout cannot
 *      be composed with the specified virtual key to form a single character.
 *
 * FIXME : should do the above (return 2 for non matching deadchar+char combinations)
 *
 */
INT16 X11DRV_KEYBOARD_ToAscii(
    UINT16 virtKey,UINT16 scanCode, LPBYTE lpKeyState, 
    LPVOID lpChar, UINT16 flags)
{
    XKeyEvent e;
    KeySym keysym;
    static XComposeStatus cs;
    INT ret;
    int keyc;

    if (scanCode==0) {
        /* This happens when doing Alt+letter : a fake 'down arrow' key press
           event is generated by windows. Just ignore it. */
        TRACE(keyboard,"scanCode=0, doing nothing\n");
        return 0;
    }
    if (scanCode & 0x8000)
    {
        TRACE( keyboard, "Key UP, doing nothing\n" );
        return 0;
    }
    e.display = display;
    e.keycode = 0;
    e.state = 0;
    if (lpKeyState[VK_SHIFT] & 0x80)
	e.state |= ShiftMask;
    if (lpKeyState[VK_CAPITAL] & 0x01)
	e.state |= LockMask;
    if (lpKeyState[VK_CONTROL] & 0x80)
    {
	if (lpKeyState[VK_MENU] & 0x80)
	    e.state |= AltGrMask;
	else
	    e.state |= ControlMask;
    }
    if (lpKeyState[VK_NUMLOCK] & 0x01)
	e.state |= NumLockMask;
    TRACE(key, "(%04X, %04X) : faked state = %X\n",
		virtKey, scanCode, e.state);
    /* We exit on the first keycode found, to speed up the thing. */
    for (keyc=min_keycode; (keyc<=max_keycode) && (!e.keycode) ; keyc++)
      { /* Find a keycode that could have generated this virtual key */
          if  ((keyc2vkey[keyc] & 0xFF) == virtKey)
          { /* We filter the extended bit, we don't know it */
              e.keycode = keyc; /* Store it temporarily */
              if ((EVENT_event_to_vkey(&e) & 0xFF) != virtKey) {
                  e.keycode = 0; /* Wrong one (ex: because of the NumLock
                         state), so set it to 0, we'll find another one */
              }
	  }
      }

    if ((virtKey>=VK_NUMPAD0) && (virtKey<=VK_NUMPAD9))
        e.keycode = TSXKeysymToKeycode(e.display, virtKey-VK_NUMPAD0+XK_KP_0);
          
    if (virtKey==VK_DECIMAL)
        e.keycode = TSXKeysymToKeycode(e.display, XK_KP_Decimal);

    if (!e.keycode)
      {
	WARN(keyboard,"Unknown virtual key %X !!! \n",virtKey);
	return virtKey; /* whatever */
      }
    else TRACE(keyboard,"Found keycode %d (0x%2X)\n",e.keycode,e.keycode);

    ret = TSXLookupString(&e, (LPVOID)lpChar, 2, &keysym, &cs);
    if (ret == 0)
	{
	BYTE dead_char = 0;

	((char*)lpChar)[1] = '\0';
	switch (keysym)
	    {
	/* symbolic ASCII is the same as defined in rfc1345 */
#ifdef XK_dead_tilde
	    case XK_dead_tilde :
#endif
	    case 0x1000FE7E : /* Xfree's XK_Dtilde */
		dead_char = '~';	/* '? */
		break;
#ifdef XK_dead_acute
	    case XK_dead_acute :
#endif
	    case 0x1000FE27 : /* Xfree's XK_Dacute_accent */
		dead_char = 0xb4;	/* '' */
		break;
#ifdef XK_dead_circumflex
	    case XK_dead_circumflex :
#endif
	    case 0x1000FE5E : /* Xfree's XK_Dcircumflex_accent */
		dead_char = '^';	/* '> */
		break;
#ifdef XK_dead_grave
	    case XK_dead_grave :
#endif
	    case 0x1000FE60 : /* Xfree's XK_Dgrave_accent */
		dead_char = '`';	/* '! */
		break;
#ifdef XK_dead_diaeresis
	    case XK_dead_diaeresis :
#endif
	    case 0x1000FE22 : /* Xfree's XK_Ddiaeresis */
		dead_char = 0xa8;	/* ': */
		break;
#ifdef XK_dead_cedilla
	    case XK_dead_cedilla :
	        dead_char = 0xb8;	/* ', */
	        break;
#endif
#ifdef XK_dead_macron
	    case XK_dead_macron :
	        dead_char = '-';	/* 'm isn't defined on iso-8859-x */
	        break;
#endif
#ifdef XK_dead_breve
	    case XK_dead_breve :
	        dead_char = 0xa2;	/* '( */
	        break;
#endif
#ifdef XK_dead_abovedot
	    case XK_dead_abovedot :
	        dead_char = 0xff;	/* '. */
	        break;
#endif
#ifdef XK_dead_abovering
	    case XK_dead_abovering :
	        dead_char = '0';	/* '0 isn't defined on iso-8859-x */
	        break;
#endif
#ifdef XK_dead_doubleacute
	    case XK_dead_doubleacute :
	        dead_char = 0xbd;	/* '" */
	        break;
#endif
#ifdef XK_dead_caron
	    case XK_dead_caron :
	        dead_char = 0xb7;	/* '< */
	        break;
#endif
#ifdef XK_dead_ogonek
	    case XK_dead_ogonek :
	        dead_char = 0xb2;	/* '; */
	        break;
#endif
/* FIXME: I don't know this three.
	    case XK_dead_iota :
	        dead_char = 'i';	 
	        break;
	    case XK_dead_voiced_sound :
	        dead_char = 'v';
	        break;
	    case XK_dead_semivoiced_sound :
	        dead_char = 's';
	        break;
*/
	    }
	if (dead_char)
	    {
	    *(char*)lpChar = dead_char;
	    ret = -1;
	    }
	else
	    {
	    char	*ksname;

	    ksname = TSXKeysymToString(keysym);
	    if (!ksname)
		ksname = "No Name";
	    if ((keysym >> 8) != 0xff)
		{
		ERR(keyboard, "Please report: no char for keysym %04lX (%s) :\n",
			keysym, ksname);
		ERR(keyboard, "(virtKey=%X,scanCode=%X,keycode=%X,state=%X)\n",
			virtKey, scanCode, e.keycode, e.state);
		}
	    }
	}
    else {  /* ret = 1 */
        /* We have a special case to handle : Shift + arrow, shift + home, ...
           X returns a char for it, but Windows doesn't. Let's eat it. */
        if (!(lpKeyState[VK_NUMLOCK] & 0x01)  /* NumLock is off */
            && (lpKeyState[VK_SHIFT] & 0x80) /* Shift is pressed */
            && (keysym>=XK_KP_0) && (keysym<=XK_KP_9))
        {
            *(char*)lpChar = 0;
            ret = 0;
        }
    }

    TRACE(key, "ToAscii about to return %d with char %x\n",
		ret, *(char*)lpChar);
    return ret;
}

#endif /* !defined(X_DISPLAY_MISSING) */


