/*
 * Keyboard related functions
 *
 * Copyright 1993 Bob Amstadt
 * Copyright 1996 Albrecht Kleine 
 * Copyright 1997 David Faure
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "windows.h"
#include "win.h"
#include "gdi.h"
#include "keyboard.h"
#include "message.h"
#include "stddebug.h"
/* #define DEBUG_KEYBOARD */
#include "debug.h"
#include "xmalloc.h"
#include "accel.h"
#include "struct32.h"

BOOL32 MouseButtonsStates[3];
BOOL32 AsyncMouseButtonsStates[3];
BYTE InputKeyStateTable[256];
BYTE QueueKeyStateTable[256];
BYTE AsyncKeyStateTable[256];

static int NumLockMask;
static int AltGrMask;
static int min_keycode, max_keycode;
static int keyc2vkey[256];

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

/*
 * Table for vkey to scancode translation - 5/29/97 chrisf@america.com 
 */
static const BYTE vkey2scode[512] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x0e,0x0f,0x00,0x00,0x00,0x1c,0x00,0x00,
  0x2a,0x1d,0x38,0x00,0x3a,0x00,0x00,0x00, 0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,
  0x39,0x49,0x51,0x4f,0x47,0x4b,0x48,0x4d, 0x50,0x00,0x00,0x00,0x00,0x52,0x53,0x00,
  0x0b,0x02,0x03,0x04,0x05,0x06,0x07,0x08, 0x09,0x0a,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x1e,0x30,0x2e,0x20,0x12,0x21,0x22, 0x23,0x17,0x24,0x25,0x26,0x32,0x31,0x18,
  0x19,0x10,0x13,0x1f,0x14,0x16,0x2f,0x11, 0x2d,0x15,0x2c,0x00,0x00,0x00,0x00,0x00,
  0x0b,0x02,0x03,0x04,0x05,0x06,0x07,0x08, 0x09,0x0a,0x37,0x4e,0x00,0x4a,0x34,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x46,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x29,0x0c,0x0d,0x1a,0x1b,0x2b,
  0x27,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x28,0x33,0x34,0x35,0x4c,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/* 256 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x1c,0x00,0x00,
  0x00,0x1d,0x38,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x49,0x51,0x4f,0x47,0x4b,0x48,0x4d, 0x50,0x00,0x00,0x00,0x00,0x52,0x53,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x35,
  0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42, 0x43,0x44,0x57,0x58,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

static WORD EVENT_event_to_vkey( XKeyEvent *e)
{
    KeySym keysym;

    XLookupString(e, NULL, 0, &keysym, NULL);

    if ((keysym >= 0xFFAE) && (keysym <= 0xFFB9) && (e->state & NumLockMask)) 
        /* Only the Keypad keys 0-9 and . send different keysyms
         * depending on the NumLock state */
        return keypad_key[(keysym & 0xFF) - 0x7E];

    return keyc2vkey[e->keycode];
}

/**********************************************************************
 *		KEYBOARD_Init
 */
BOOL32 KEYBOARD_Init(void)
{
    int i, keysyms_per_keycode;
    KeySym *ksp;
    XModifierKeymap *mmp;
    KeySym keysym;
    KeyCode *kcp;
    XKeyEvent e2;
    WORD vkey, OEMvkey;

    XDisplayKeycodes(display, &min_keycode, &max_keycode);
    ksp = XGetKeyboardMapping(display, min_keycode,
                              max_keycode + 1 - min_keycode, &keysyms_per_keycode);
    /* We are only interested in keysyms_per_keycode.
       There is no need to hold a local copy of the keysyms table */
    XFree(ksp);
    mmp = XGetModifierMapping(display);
    kcp = mmp->modifiermap;
    for (i = 0; i < 8; i += 1) /* There are 8 modifier keys */
    {
        int j;
        
        for (j = 0; j < mmp->max_keypermod; j += 1, kcp += 1)
	    if (*kcp)
            {
		int k;
                
		for (k = 0; k < keysyms_per_keycode; k += 1)
                    if (XKeycodeToKeysym(display, *kcp, k) == XK_Mode_switch)
		    {
                        AltGrMask = 1 << i;
                        dprintf_key(stddeb, "AltGrMask is %x\n", AltGrMask);
		    }
                    else if (XKeycodeToKeysym(display, *kcp, k) == XK_Num_Lock)
		    {
                        NumLockMask = 1 << i;
                        dprintf_key(stddeb, "NumLockMask is %x\n", NumLockMask);
		    }
            }
    }
    XFreeModifiermap(mmp);

    /* Now build two conversion arrays :
     * keycode -> vkey + extended
     * vkey + extended -> keycode */

    e2.display = display;
    e2.state = 0;

    OEMvkey = 0xb9; /* first OEM virtual key available is ba */
    for (e2.keycode=min_keycode; e2.keycode<=max_keycode; e2.keycode++)
    {
        XLookupString(&e2, NULL, 0, &keysym, NULL);
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
                keysym = XLookupKeysym(&e2, i);
                if ((keysym >= VK_0 && keysym <= VK_9)
                    || (keysym >= VK_A && keysym <= VK_Z)
                    || keysym == VK_SPACE)
		    vkey = keysym;
            }

            if (!vkey)
            {
                /* Others keys: let's assign OEM virtual key codes in the allowed range,
                 * that is ([0xba,0xc0], [0xdb,0xe4], 0xe6 (given up) et [0xe9,0xf5]) */
                switch (++OEMvkey)
                {
                case 0xc1 : OEMvkey=0xdb; break;
                case 0xe5 : OEMvkey=0xe9; break;
                case 0xf6 : OEMvkey=0xf5; fprintf(stderr,"No more OEM vkey available!\n");
                }

                vkey = OEMvkey;
		  
                if (debugging_keyboard)
                {
                    fprintf(stddeb,"OEM specific virtual key %X assigned to keycode %X :\n ("
                            ,OEMvkey,e2.keycode);
                    for (i = 0; i < keysyms_per_keycode; i += 1)
                    {
                        char	*ksname;
                        
                        keysym = XLookupKeysym(&e2, i);
                        ksname = XKeysymToString(keysym);
                        if (!ksname)
			    ksname = "NoSymbol";
                        fprintf(stddeb, "%lX (%s) ", keysym, ksname);
                    }
                    fprintf(stddeb, ")\n");
                }
            }
        }
        keyc2vkey[e2.keycode] = vkey;
    } /* for */
    return TRUE;
}

static BOOL32 NumState=FALSE, CapsState=FALSE;

void KEYBOARD_GenerateMsg( WORD vkey, int Evtype, XKeyEvent * event, KEYLP localkeylp )
{
  BOOL32 * State = (vkey==VK_NUMLOCK? &NumState : &CapsState);

  if (*State) {
    /* The INTERMEDIARY state means : just after a 'press' event, if a 'release' event comes,
       don't treat it. It's from the same key press. Then the state goes to ON.
       And from there, a 'release' event will switch off the toggle key. */
    *State=FALSE;
    dprintf_keyboard(stddeb,"INTERM : don\'t treat release of toggle key. InputKeyStateTable[%#x] = %#x\n",vkey,InputKeyStateTable[vkey]);
  } else
    {
	if ( InputKeyStateTable[vkey] & 0x1 ) /* it was ON */
	  {
	    if (Evtype!=KeyPress)
	      {
		dprintf_keyboard(stddeb,"ON + KeyRelease => generating DOWN and UP messages.\n");
		localkeylp.lp1.previous = 0; /* ? */
		localkeylp.lp1.transition = 0;
		hardware_event( WM_KEYDOWN, vkey, localkeylp.lp2, event->x_root - desktopX,
				event->y_root - desktopY, event->time - MSG_WineStartTicks, 0);
		hardware_event( WM_KEYUP, vkey, localkeylp.lp2, event->x_root - desktopX,
				event->y_root - desktopY, event->time - MSG_WineStartTicks, 0);
		*State=FALSE;
		InputKeyStateTable[vkey] &= ~0x01; /* Toggle state to off. */ 
	      } 
	  }
	else /* it was OFF */
	  if (Evtype==KeyPress)
	    {
	      dprintf_keyboard(stddeb,"OFF + Keypress => generating DOWN and UP messages.\n");
	      hardware_event( WM_KEYDOWN, vkey, localkeylp.lp2, event->x_root - desktopX,
			      event->y_root - desktopY, event->time - MSG_WineStartTicks, 0);
	      localkeylp.lp1.previous = 1;
	      localkeylp.lp1.transition = 1;
	      hardware_event( WM_KEYUP, vkey, localkeylp.lp2, event->x_root - desktopX,
			      event->y_root - desktopY, event->time - MSG_WineStartTicks, 0);
	      *State=TRUE; /* Goes to intermediary state before going to ON */
	      InputKeyStateTable[vkey] |= 0x01; /* Toggle state to on. */
	    }
    }
}

/***********************************************************************
 *           KEYBOARD_HandleEvent
 *
 * Handle a X key event
 */
void KEYBOARD_HandleEvent( XKeyEvent *event )
{
    char Str[24]; 
    XComposeStatus cs; 
    KeySym keysym;
    WORD vkey = 0;
    KEYLP keylp;
    static BOOL32 force_extended = FALSE; /* hack for AltGr translation */

    int ascii_chars = XLookupString(event, Str, 1, &keysym, &cs);

    dprintf_key(stddeb, "EVENT_key : state = %X\n", event->state);
    if (keysym == XK_Mode_switch)
	{
	dprintf_key(stddeb, "Alt Gr key event received\n");
	event->keycode = XKeysymToKeycode(event->display, XK_Control_L);
	dprintf_key(stddeb, "Control_L is keycode 0x%x\n", event->keycode);
	KEYBOARD_HandleEvent(event);
	event->keycode = XKeysymToKeycode(event->display, XK_Alt_L);
	dprintf_key(stddeb, "Alt_L is keycode 0x%x\n", event->keycode);
	force_extended = TRUE;
	KEYBOARD_HandleEvent(event);
	force_extended = FALSE;
	return;
	}

    Str[ascii_chars] = '\0';
    if (debugging_key)
	{
	char	*ksname;

	ksname = XKeysymToString(keysym);
	if (!ksname)
	    ksname = "No Name";
	fprintf(stddeb, "%s : keysym=%lX (%s), ascii chars=%u / %X / '%s'\n", 
                (event->type == KeyPress) ? "KeyPress" : "KeyRelease",
                keysym, ksname, ascii_chars, Str[0] & 0xff, Str);
	}

    vkey = EVENT_event_to_vkey(event);
    if (force_extended) vkey |= 0x100;

    dprintf_key(stddeb, "keycode 0x%x converted to vkey 0x%x\n",
		    event->keycode, vkey);

   if (vkey)
   {
    keylp.lp1.count = 1;
    keylp.lp1.code = vkey2scode[vkey]; /* 5/29/97 chrisf@america.com */
    keylp.lp1.extended = (vkey & 0x100 ? 1 : 0);
    keylp.lp1.win_internal = 0; /* this has something to do with dialogs, 
				* don't remember where I read it - AK */
				/* it's '1' under windows, when a dialog box appears
				 * and you press one of the underlined keys - DF*/
    vkey &= 0xff;

    switch(vkey)
    {
    case VK_NUMLOCK:    
      KEYBOARD_GenerateMsg(VK_NUMLOCK,event->type,event,keylp); break;
    case VK_CAPITAL:
      dprintf_keyboard(stddeb,"Caps Lock event. (type %d). State before : %#.2x\n",event->type,InputKeyStateTable[vkey]);
      KEYBOARD_GenerateMsg(VK_CAPITAL,event->type,event,keylp); 
      dprintf_keyboard(stddeb,"State after : %#.2x\n",InputKeyStateTable[vkey]);
      break;
    default:
      {
	WORD message;
	if (event->type == KeyPress)
	  {
	    keylp.lp1.previous = (InputKeyStateTable[vkey] & 0x80) != 0;
	    if (!(InputKeyStateTable[vkey] & 0x80))
	      InputKeyStateTable[vkey] ^= 0x01;
	    InputKeyStateTable[vkey] |= 0x80;
	    keylp.lp1.transition = 0;
	    message = (InputKeyStateTable[VK_MENU] & 0x80)
	      && !(InputKeyStateTable[VK_CONTROL] & 0x80)
	      ? WM_SYSKEYDOWN : WM_KEYDOWN;
	  }
	else
	  {
            BOOL32 sysKey = (InputKeyStateTable[VK_MENU] & 0x80)
                && !(InputKeyStateTable[VK_CONTROL] & 0x80)
                && (force_extended == FALSE); /* for Alt from AltGr */
	    
	    InputKeyStateTable[vkey] &= ~0x80; 
	    keylp.lp1.previous = 1;
	    keylp.lp1.transition = 1;
	    message = sysKey ? WM_SYSKEYUP : WM_KEYUP;
	  }
	keylp.lp1.context = ( (event->state & Mod1Mask)  || 
			      (InputKeyStateTable[VK_MENU] & 0x80)) ? 1 : 0;
	if (!(InputKeyStateTable[VK_NUMLOCK] & 0x01) != !(event->state & NumLockMask))
	  { 
	    dprintf_keyboard(stddeb,"Adjusting NumLock state. \n");
	    KEYBOARD_GenerateMsg(VK_NUMLOCK,KeyPress,event,keylp);
	    KEYBOARD_GenerateMsg(VK_NUMLOCK,KeyRelease,event,keylp);
	  }
	if (!(InputKeyStateTable[VK_CAPITAL] & 0x01) != !(event->state & LockMask))
	  {
	    dprintf_keyboard(stddeb,"Adjusting Caps Lock state. State before %#.2x \n",InputKeyStateTable[VK_CAPITAL]);
	    KEYBOARD_GenerateMsg(VK_CAPITAL,KeyPress,event,keylp);
	    KEYBOARD_GenerateMsg(VK_CAPITAL,KeyRelease,event,keylp);
	    dprintf_keyboard(stddeb,"State after %#.2x \n",InputKeyStateTable[VK_CAPITAL]);
	  }
	/* End of intermediary states. */
	NumState = FALSE;
	CapsState = FALSE;

	dprintf_key(stddeb,"            wParam=%04X, lParam=%08lX\n", 
		    vkey, keylp.lp2 );
	dprintf_key(stddeb,"            InputKeyState=%X\n",
		    InputKeyStateTable[vkey]);

	hardware_event( message, vkey, keylp.lp2, event->x_root - desktopX,
			event->y_root - desktopY, event->time - MSG_WineStartTicks, 0 );
      }
    }
   }
}


/**********************************************************************
 *		GetKeyState			[USER.106]
 */
WORD WINAPI GetKeyState16(INT16 vkey)
{
    return GetKeyState32(vkey);
}

/**********************************************************************
 *		GetKeyState			[USER32.248]
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
	    retval = ( (WORD)(QueueKeyStateTable[vkey] & 0x80) << 8 ) |
		       (WORD)(QueueKeyStateTable[vkey] & 0x01);
	}
    dprintf_key(stddeb, "GetKeyState(0x%x) -> %x\n", vkey, retval);
    return retval;
}

/**********************************************************************
 *		GetKeyboardState	[USER.222][USER32.253]
 * An application calls the GetKeyboardState function in response to a
 * keyboard-input message.  This function retrieves the state of the keyboard
 * at the time the input message was generated.  (SDK 3.1 Vol 2. p 387)
 */
VOID WINAPI GetKeyboardState(LPBYTE lpKeyState)
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
 *      SetKeyboardState            [USER.223][USER32.483]
 */
VOID WINAPI SetKeyboardState(LPBYTE lpKeyState)
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
 *            GetAsyncKeyState        (USER32.206)
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
	  	((InputKeyStateTable[nKey] & 0x80) ? 0x8000 : 0); 
	break;
    }

    memset( AsyncMouseButtonsStates, 0, 3 );  /* all states to false */
    memset( AsyncKeyStateTable, 0, 256 );

    dprintf_key(stddeb, "GetAsyncKeyState(%x) -> %x\n", nKey, retval);
    return retval;
}

/**********************************************************************
 *            GetAsyncKeyState        (USER.249)
 */
WORD WINAPI GetAsyncKeyState16(INT16 nKey)
{
    return GetAsyncKeyState32(nKey);
}

/**********************************************************************
 *			TranslateAccelerator 	[USER.178][USER32.551..]
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
   	  dprintf_accel(stddeb,"found accel for WM_CHAR: ('%c')",msg->wParam&0xff);
   	  sendmsg=TRUE;
   	}  
      } else {
       if(fVirt & FVIRTKEY) {
	INT32 mask = 0;
        dprintf_accel(stddeb,"found accel for virt_key %04x (scan %04x)",
  	                       msg->wParam,0xff & HIWORD(msg->lParam));                
	if(GetKeyState32(VK_SHIFT) & 0x8000) mask |= FSHIFT;
	if(GetKeyState32(VK_CONTROL) & 0x8000) mask |= FCONTROL;
	if(GetKeyState32(VK_MENU) & 0x8000) mask |= FALT;
	if(mask == (fVirt & (FSHIFT | FCONTROL | FALT)))
          sendmsg=TRUE;			    
        else
          dprintf_accel(stddeb,", but incorrect SHIFT/CTRL/ALT-state\n");
       }
       else
       {
         if (!(msg->lParam & 0x01000000))  /* no special_key */
         {
           if ((fVirt & FALT) && (msg->lParam & 0x20000000))
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
              dprintf_accel(stddeb,", sending %s, wParam=%0x\n",
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
	    dprintf_accel(stddeb,", but won't send WM_{SYS}COMMAND, reason is #%d\n",mesg);
	  }          
          return TRUE;         
      }
    }
    return FALSE;
}

INT32 WINAPI TranslateAccelerator32(HWND32 hWnd, HACCEL32 hAccel, LPMSG32 msg)
{
    LPACCEL32	lpAccelTbl = (LPACCEL32)LockResource32(hAccel);
    int 	i;
    
    if (hAccel == 0 || msg == NULL) return 0;
    if (msg->message != WM_KEYDOWN &&
    	msg->message != WM_KEYUP &&
	msg->message != WM_SYSKEYDOWN &&
	msg->message != WM_SYSKEYUP &&
    	msg->message != WM_CHAR) return 0;

    dprintf_accel(stddeb, "TranslateAccelerators hAccel=%04x, hWnd=%04x,\
msg->hwnd=%04x, msg->message=%04x\n", hAccel,hWnd,msg->hwnd,msg->message);

    for (i = 0; lpAccelTbl[i].key ; i++)
    	if (KBD_translate_accelerator(hWnd,msg,lpAccelTbl[i].fVirt,
                                      lpAccelTbl[i].key,lpAccelTbl[i].cmd))
		return 1;
    return 0;
}
	
INT16 WINAPI TranslateAccelerator16(HWND16 hWnd, HACCEL16 hAccel, LPMSG16 msg)
{
    LPACCEL16	lpAccelTbl = (LPACCEL16)LockResource16(hAccel);
    int 	i;
    MSG32	msg32;
    
    if (hAccel == 0 || msg == NULL) return 0;
    if (msg->message != WM_KEYDOWN &&
    	msg->message != WM_KEYUP &&
	msg->message != WM_SYSKEYDOWN &&
	msg->message != WM_SYSKEYUP &&
    	msg->message != WM_CHAR) return 0;

    dprintf_accel(stddeb, "TranslateAccelerators hAccel=%04x, hWnd=%04x,\
msg->hwnd=%04x, msg->message=%04x\n", hAccel,hWnd,msg->hwnd,msg->message);
    STRUCT32_MSG16to32(msg,&msg32);


    for (i=0;lpAccelTbl[i].key;i++) 
    	if (KBD_translate_accelerator(hWnd,&msg32,lpAccelTbl[i].fVirt,
                                      lpAccelTbl[i].key,lpAccelTbl[i].cmd))
		return 1;
    return 0;
}


/******************************************************************************
 *    	OemKeyScan			[KEYBOARD.128][USER32.400]
 */
DWORD WINAPI OemKeyScan(WORD wOemChar)
{
    dprintf_keyboard(stddeb,"*OemKeyScan (%d)\n",wOemChar);

    return wOemChar;
}

/******************************************************************************
 *    	VkKeyScanA			[USER32.572]
 */
/* VkKeyScan translates an ANSI character to a virtual-key and shift code
 * for the current keyboard.
 * high-order byte yields :
 *	0	Unshifted
 *	1	Shift
 *	2	Ctrl
 *	3-5	Shift-key combinations that are not used for characters
 *	6	Ctrl-Alt
 *	7	Ctrl-Alt-Shift
 *	I.e. :	Shift = 1, Ctrl = 2, Alt = 4.
 * FIXME : works ok except for dead chars :
 * VkKeyScan '^'(0x5e, 94) ... got keycode 00 ... returning 00
 * VkKeyScan '`'(0x60, 96) ... got keycode 00 ... returning 00
 */

WORD WINAPI VkKeyScan32A(CHAR cChar)
{
	KeyCode keycode;
	KeySym keysym;    	
	int i,index;
	int highbyte=0;

	/* char->keysym (same for ANSI chars) */
	keysym=(unsigned char) cChar;/* (!) cChar is signed */
	if (keysym<=27) keysym+=0xFF00;/*special chars : return, backspace...*/
	
	keycode = XKeysymToKeycode(display, keysym);  /* keysym -> keycode */
	if (!keycode)
	{ /* It didn't work ... let's try with deadchar code. */
	  keycode = XKeysymToKeycode(display, keysym | 0xFE00);
	}

	dprintf_keyboard(stddeb,"VkKeyScan '%c'(%#lx, %lu) : got keycode %#.2x ",
	    cChar,keysym,keysym,keycode);
	
	if (keycode)
	  {
	    for (index=-1, i=0; (i<8) && (index<0); i++) /* find shift state */
	      if (XKeycodeToKeysym(display,keycode,i)==keysym) index=i;
	    switch (index) {
	    case -1 :
	      fprintf(stderr,"Keysym %lx not found while parsing the keycode table\n",keysym); break;
	    case 0 : break;
	    case 1 : highbyte = 0x0100; break;
	    case 2 : highbyte = 0X0600; break;
	    default : fprintf(stderr,"index %d found by XKeycodeToKeysym. please report! \n",index);
	    }
	    /*
	      index : 0     adds 0x0000
	      index : 1     adds 0x0100 (shift)
	      index : ?     adds 0x0200 (ctrl)
	      index : 2     adds 0x0600 (ctrl+alt)
	      index : ?     adds 0x0700 (ctrl+alt+shit (used?))
	     */
	  }
	dprintf_keyboard(stddeb," ... returning %#.2x\n", keyc2vkey[keycode]+highbyte);
	return keyc2vkey[keycode]+highbyte;   /* keycode -> (keyc2vkey) vkey */
}

/******************************************************************************
 *    	VkKeyScan			[KEYBOARD.129]
 */
WORD WINAPI VkKeyScan16(CHAR cChar)
{
	return VkKeyScan32A(cChar);
}

/******************************************************************************
 *    	VkKeyScanW			[USER32.575]
 */
WORD WINAPI VkKeyScan32W(WCHAR cChar)
{
	return VkKeyScan32A((CHAR)cChar); /* FIXME: check unicode */
}

/******************************************************************************
 *    	GetKeyboardType			[KEYBOARD.130]
 */
INT16 WINAPI GetKeyboardType16(INT16 nTypeFlag)
{
  return GetKeyboardType32(nTypeFlag);
}

/******************************************************************************
 *    	GetKeyboardType			[USER32.254]
 */
INT32 WINAPI GetKeyboardType32(INT32 nTypeFlag)
{
  dprintf_keyboard(stddeb,"GetKeyboardType(%d)\n",nTypeFlag);
  switch(nTypeFlag)
    {
    case 0:      /* Keyboard type */
      return 4;    /* AT-101 */
      break;
    case 1:      /* Keyboard Subtype */
      return 0;    /* There are no defined subtypes */
      break;
    case 2:      /* Number of F-keys */
      return 12;   /* We're doing an 101 for now, so return 12 F-keys */
      break;
    default:     
      fprintf(stderr, "Unknown type on GetKeyboardType\n");
      return 0;    /* The book says 0 here, so 0 */
    }
}


/******************************************************************************
 *    	MapVirtualKeyA			[USER32.382]
 */
UINT32 WINAPI MapVirtualKey32A(UINT32 code, UINT32 maptype)
{
    return MapVirtualKey16(code,maptype);
}

/******************************************************************************
 *    	MapVirtualKeyA			[USER32.384]
 */
UINT32 WINAPI MapVirtualKey32W(UINT32 code, UINT32 maptype)
{
    return MapVirtualKey16(code,maptype);
}

/******************************************************************************
 *    	MapVirtualKeyA			[KEYBOARD.131]
 * MapVirtualKey translates keycodes from one format to another
 */
UINT16 WINAPI MapVirtualKey16(UINT16 wCode, UINT16 wMapType)
{
#define returnMVK(value) { dprintf_keyboard(stddeb,"returning 0x%x.\n",value); return value; }

	dprintf_keyboard(stddeb,"MapVirtualKey wCode=0x%x wMapType=%d ... ",wCode,wMapType);
	switch(wMapType) {
		case 0:	{ /* vkey-code to scan-code */
			/* let's do vkey -> keycode -> scan */
			KeyCode keyc;
			for (keyc=min_keycode; keyc<=max_keycode; keyc++) /* see event.c */
				if ((keyc2vkey[keyc] & 0xFF)== wCode)
					returnMVK (keyc - 8);
		        return 0; }

		case 1: /* scan-code to vkey-code */
			/* let's do scan -> keycode -> vkey */

			returnMVK (keyc2vkey[(wCode & 0xFF) + 8]);

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
			if (!XLookupString(&e, s , 2 , &keysym, NULL))
			  returnMVK (*s);
			
			return 0;
			}
		default: /* reserved */
			fprintf(stderr, "MapVirtualKey: unknown wMapType %d !\n",
				wMapType);
			return 0;	
	}
	return 0;
}


/****************************************************************************
 *	GetKBCodePage16   (KEYBOARD.132)
 */
INT16 WINAPI GetKBCodePage16(void)
{
    dprintf_keyboard(stddeb,"GetKBCodePage()\n");
    return 850;
}


/****************************************************************************
 *	GetKBCodePage32   (USER32.245)
 */
UINT32 WINAPI GetKBCodePage32(void)
{
    dprintf_keyboard(stddeb,"GetKbCodePage()\n");
    return 850;
}


/****************************************************************************
 *	GetKeyNameText32W   (USER32.247)
 */
INT32 WINAPI GetKeyNameText32W(LONG lParam, LPWSTR lpBuffer, INT32 nSize)
{
	LPSTR buf = xmalloc(nSize);
	int	res = GetKeyNameText32A(lParam,buf,nSize);

	lstrcpynAtoW(lpBuffer,buf,nSize);
	free(buf);
	return res;
}

/****************************************************************************
 *	GetKeyNameText32A   (USER32.246)
 */
INT32 WINAPI GetKeyNameText32A(LONG lParam, LPSTR lpBuffer, INT32 nSize)
{
	return GetKeyNameText16(lParam,lpBuffer,nSize);
}

/****************************************************************************
 *	GetKeyNameText16   (KEYBOARD.133)
 */
INT16 WINAPI GetKeyNameText16(LONG lParam, LPSTR lpBuffer, INT16 nSize)
{
  /*	int i; */
	
    	dprintf_keyboard(stddeb,"GetKeyNameText(%ld,<ptr>,%d)\n",lParam,nSize);

	lParam >>= 16;
	lParam &= 0xff;

	/*	for (i = 0 ; i != KeyTableSize ; i++) 
		if (KeyTable[i].scancode == lParam)  {
			lstrcpyn32A( lpBuffer, KeyTable[i].name, nSize );
			return strlen(lpBuffer);
		}
		*/
	/* FIXME ! GetKeyNameText is still to do...
 */
	*lpBuffer = 0;
	return 0;
}


/****************************************************************************
 *	ToAscii   (KEYBOARD.4)
 */
INT16 WINAPI ToAscii16(UINT16 virtKey,UINT16 scanCode, LPBYTE lpKeyState, 
                       LPVOID lpChar, UINT16 flags) 
{
    return ToAscii32(virtKey,scanCode,lpKeyState,lpChar,flags);
}

/****************************************************************************
 *	ToAscii   (USER32.545)
 */
INT32 WINAPI ToAscii32( UINT32 virtKey,UINT32 scanCode,LPBYTE lpKeyState,
                        LPWORD lpChar,UINT32 flags )
{
    XKeyEvent e;
    KeySym keysym;
    static XComposeStatus cs;
    INT32 ret;
    WORD keyc;

    e.display = display;
    e.keycode = 0;
    for (keyc=min_keycode; keyc<=max_keycode; keyc++)
      { /* this could be speeded up by making another table, an array of struct vkey,keycode
	 * (vkey -> keycode) with vkeys sorted .... but it takes memory (512*3 bytes)!  DF */
	if ((keyc2vkey[keyc] & 0xFF)== virtKey) /* no need to make a more precise test (with the extended bit correctly set above virtKey ... VK* are different enough... */
	  {
	    if ((e.keycode) && ((virtKey<0x10) || (virtKey>0x12))) 
		/* it's normal to have 2 shift, control, and alt ! */
		dprintf_keyboard(stddeb,"ToAscii : The keycodes %d and %d are matching the same vkey %#X\n",
				 e.keycode,keyc,virtKey);
	    e.keycode = keyc;
	  }
      }
    if ((!e.keycode) && (lpKeyState[VK_NUMLOCK] & 0x01)) 
      {
	if ((virtKey>=VK_NUMPAD0) && (virtKey<=VK_NUMPAD9))
	  e.keycode = XKeysymToKeycode(e.display, virtKey-VK_NUMPAD0+XK_KP_0);
	if (virtKey==VK_DECIMAL)
	  e.keycode = XKeysymToKeycode(e.display, XK_KP_Decimal);
      }
    if (!e.keycode)
      {
	fprintf(stderr,"ToAscii : Unknown virtual key %X !!! \n",virtKey);
	return virtKey; /* whatever */
      }
    e.state = 0;
    if (lpKeyState[VK_SHIFT] & 0x80)
	e.state |= ShiftMask;
    dprintf_keyboard(stddeb,"ToAscii : lpKeyState[0x14(VK_CAPITAL)]=%#x\n",lpKeyState[VK_CAPITAL]);
    if (lpKeyState[VK_CAPITAL] & 0x01)
	e.state |= LockMask;
    if (lpKeyState[VK_CONTROL] & 0x80)
	if (lpKeyState[VK_MENU] & 0x80)
	    e.state |= AltGrMask;
	else
	    e.state |= ControlMask;
    if (lpKeyState[VK_NUMLOCK] & 0x01)
	e.state |= NumLockMask;
    dprintf_key(stddeb, "ToAscii(%04X, %04X) : faked state = %X\n",
		virtKey, scanCode, e.state);
    ret = XLookupString(&e, (LPVOID)lpChar, 2, &keysym, &cs);
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

	    ksname = XKeysymToString(keysym);
	    if (!ksname)
		ksname = "No Name";
	    if ((keysym >> 8) != 0xff)
		{
		fprintf(stderr, "Please report : no char for keysym %04lX (%s) :\n",
			keysym, ksname);
		fprintf(stderr, "  virtKey = %X, scanCode = %X, keycode = %X, state = %X\n",
			virtKey, scanCode, e.keycode, e.state);
		}
	    }
	}
    dprintf_key(stddeb, "ToAscii about to return %d with char %x\n",
		ret, *(char*)lpChar);
    return ret;
}


/***********************************************************************
 *           GetKeyboardLayout			(USER32.249)
 */
HKL32 WINAPI GetKeyboardLayout(DWORD dwLayout)
{
	fprintf(stderr,"GetKeyboardLayout(%ld),STUB!\n",dwLayout);
	return (0xcafe<<16)|GetSystemDefaultLCID(); /* FIXME */
}

/***********************************************************************
 *           GetKeyboardLayoutList		(USER32.250)
 * FIXME
 */
INT32 WINAPI GetKeyboardLayoutList(INT32 nBuff,HKL32 *layouts)
{
	fprintf(stderr,"GetKeyboardLayoutList(%d,%p),STUB!\n",nBuff,layouts);
	if (layouts)
		layouts[0] = GetKeyboardLayout(0);
	return 1;
}

