/*
 * X11 keyboard driver
 *
 * Copyright 1993 Bob Amstadt
 * Copyright 1996 Albrecht Kleine 
 * Copyright 1997 David Faure
 * Copyright 1998 Morten Welinder
 * Copyright 1998 Ulrich Weigand
 * Copyright 1999 Ove Kåven
 */

#include <ctype.h>
#include "config.h"

#ifndef X_DISPLAY_MISSING

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include "ts_xlib.h"
#include "ts_xresource.h"
#include "ts_xutil.h"

#include "wine/winuser16.h"
#include "debugtools.h"
#include "keyboard.h"
#include "message.h"
#include "windef.h"
#include "x11drv.h"
#include "winnls.h"

DECLARE_DEBUG_CHANNEL(key)
DECLARE_DEBUG_CHANNEL(keyboard)

extern LPBYTE pKeyStateTable;

int min_keycode, max_keycode, keysyms_per_keycode;
WORD keyc2vkey[256], keyc2scan[256];

static int NumLockMask, AltGrMask; /* mask in the XKeyEvent state */
static int kcControl, kcAlt, kcShift, kcNumLock, kcCapsLock; /* keycodes */

static char KEYBOARD_MapDeadKeysym(KeySym keysym);

/* Keyboard translation tables */
#define MAIN_LEN 48
static const int main_key_scan[MAIN_LEN] =
{
/* this is my (102-key) keyboard layout, sorry if it doesn't quite match yours */
   0x29,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,
   0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,
   0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x2B,
   0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,
   0x56 /* the 102nd key (actually to the right of l-shift) */
};

/*** DEFINE YOUR NEW LANGUAGE-SPECIFIC MAPPINGS BELOW, SEE EXISTING TABLES */

/* the VK mappings for the main keyboard will be auto-assigned as before,
   so what we have here is just the character tables */
/* order: Normal, Shift, AltGr, Shift-AltGr */
/* We recommend you write just what is guaranteed to be correct (i.e. what's
   written on the keycaps), not the bunch of special characters behind AltGr
   and Shift-AltGr if it can vary among different X servers */
/* Remember that your 102nd key (to the right of l-shift) should be on a
   separate line, see existing tables */
/* If Wine fails to match your new table, use -debugmsg +key to find out why */
/* Remember to also add your new table to the layout index table far below! */

/*** United States keyboard layout (mostly contributed by Uwe Bonnes) */
static const char main_key_US[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","[{","]}",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";:","'\"","\\|",
 "zZ","xX","cC","vV","bB","nN","mM",",<",".>","/?"
};

/*** British keyboard layout */
static const char main_key_UK[MAIN_LEN][4] =
{
 "`","1!","2\"","3£","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","[{","]}",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";:","'@","#~",
 "zZ","xX","cC","vV","bB","nN","mM",",<",".>","/?",
 "\\|"
};

/*** French keyboard layout (contributed by Eric Pouech) */
static const char main_key_FR[MAIN_LEN][4] =
{
 "²","&1","é2~","\"3#","'4{","(5[","-6|","è7","_8\\","ç9^±","à0@",")°]","=+}",
 "aA","zZ","eE","rR","tT","yY","uU","iI","oO","pP","^¨","$£¤",
 "qQ","sSß","dD","fF","gG","hH","jJ","kK","lL","mM","ù%","*µ",
 "wW","xX","cC","vV","bB","nN",",?",";.",":/","!§",
 "<>"
};

/*** Icelandic keyboard layout (contributed by Ríkharður Egilsson) */
static const char main_key_IS[MAIN_LEN][4] =
{
 "°","1!","2\"","3#","4$","5%","6&","7/{","8([","9)]","0=}","öÖ\\","-_",
 "qQ@","wW","eE","rR","tT","yY","uU","iI","oO","pP","ðÐ","'?~",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","æÆ","´^","+*`",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","þÞ",
 "<>|"
};

/*** German keyboard layout (contributed by Ulrich Weigand) */
static const char main_key_DE[MAIN_LEN][4] =
{
 "^°","1!","2\"","3§","4$","5%","6&","7/{","8([","9)]","0=}","ß?\\","'",
 "qQ","wW","eE","rR","tT","zZ","uU","iI","oO","pP","üÜ","+*~",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","öÖ","äÄ","#´",
 "yY","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Swiss German keyboard layout (contributed by Jonathan Naylor) */
static const char main_key_SG[MAIN_LEN][4] =
{
 "§°","1+|","2\"@","3*#","4ç","5%","6&¬","7/¦","8(¢","9)","0=","'?´","^`~",
 "qQ","wW","eE","rR","tT","zZ","uU","iI","oO","pP","üè[","¨!]",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","öé","äà{","$£}",
 "yY","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>\\"
};

/*** Swiss French keyboard layout (contributed by Philippe Froidevaux) */
static const char main_key_SF[MAIN_LEN][4] =
{
 "§°","1+|","2\"@","3*#","4ç","5%","6&¬","7/¦","8(¢","9)","0=","'?´","^`~",
 "qQ","wW","eE","rR","tT","zZ","uU","iI","oO","pP","èü[","¨!]",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","éö","àä{","$£}",
 "yY","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>\\"
};

/*** Norwegian keyboard layout (contributed by Ove Kåven) */
static const char main_key_NO[MAIN_LEN][4] =
{
 "|§","1!","2\"@","3#£","4¤$","5%","6&","7/{","8([","9)]","0=}","+?","\\`´",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","åÅ","¨^~",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","øØ","æÆ","'*",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Danish keyboard layout (contributed by Bertho Stultiens) */
static const char main_key_DA[MAIN_LEN][4] =
{
 "½§","1!","2\"@","3#£","4¤$","5%","6&","7/{","8([","9)]","0=}","+?","´`|",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","åÅ","¨^~",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","æÆ","øØ","'*",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>\\"
};

/*** Swedish keyboard layout (contributed by Peter Bortas) */
static const char main_key_SE[MAIN_LEN][4] =
{
 "§½","1!","2\"@","3#£","4¤$","5%","6&","7/{","8([","9)]","0=}","+?\\","´`",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","åÅ","¨^~",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","öÖ","äÄ","'*",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>|"
};

/*** Canadian French keyboard layout */
static const char main_key_CF[MAIN_LEN][4] =
{
 "#|\\","1!±","2\"@","3/£","4$¢","5%¤","6?¬","7&¦","8*²","9(³","0)³","-_½","=+¾",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO§","pP¶","^^[","¨]",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";:~","``{","<>}",
 "zZ","xX","cC","vV","bB","nN","mM",",'_",".","éÉ",
 "«»°"
};

/*** Portuguese keyboard layout */
static const char main_key_PT[MAIN_LEN][4] =
{
 "\\¦","1!","2\"@","3#£","4$§","5%","6&","7/{","8([","9)]","0=}","'?","«»",
 "qQ",  "wW","eE",  "rR", "tT", "yY", "uU", "iI", "oO", "pP", "+*\\¨","\\'\\`",
 "aA",  "sS","dD",  "fF", "gG", "hH", "jJ", "kK", "lL", "çÇ", "ºª", "\\~\\^",
 "zZ",  "xX","cC",  "vV", "bB", "nN", "mM", ",;", ".:", "-_",
 "<>"
};

/*** Italian keyboard layout */
static const char main_key_IT[MAIN_LEN][4] =
{
 "\\|","1!1","2\"2","3?3","4$","5%½","6&¾","7/{","8([","9)]","0=}","'?`","i^~",
 "qQ@","wW","eE","rR?","tT","yY","uU","iI","oOo","pP?","ee[","+*]",
 "aAae","sS?","dD?","fF","gG","hH","jJ","kK","lL","oc@","a?#","u?",
 "zZ<","xX>","cC","vV","bB","nN","mM?",",;",".:*","-_","<>|"
};

/*** Finnish keyboard layout */
static const char main_key_FI[MAIN_LEN][4] =
{
 "","1!","2\"@","3#","4$","5%","6&","7/{","8([","9)]","0=}","+?\\","\'`",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","","\"^~",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","","","'*",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>|"
};

/*** Russian keyboard layout (contributed by Pavel Roskin) */
static const char main_key_RU[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQÊê","wWÃã","eEÕõ","rRËë","tTÅå","yYÎî","uUÇç","iIÛû","oOÝý","pPÚú","[{Èè","]}ßÿ",
 "aAÆæ","sSÙù","dD×÷","fFÁá","gGÐð","hHÒò","jJÏï","kKÌì","lLÄä",";:Öö","'\"Üü","\\|",
 "zZÑñ","xXÞþ","cCÓó","vVÍí","bBÉé","nNÔô","mMØø",",<Ââ",".>Àà","/?"
};

/*** Spanish keyboard layout (contributed by José Marcos López) */
static const char main_key_ES[MAIN_LEN][4] =
{
 "ºª\\","1!|","2\"@","3·#","4$","5%","6&¬","7/","8(","9)","0=","'?","¡¿",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","`^[","+*]",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","ñÑ","'¨{","çÇ}",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Belgian keyboard layout ***/
static const char main_key_BE[MAIN_LEN][4] =
{
 "","&1|","é2@","\"3#","'4","(5","§6^","è7","!8","ç9{","à0}",")°","-_",
 "aA","zZ","eE¤","rR","tT","yY","uU","iI","oO","pP","^¨[","$*]",
 "qQ","sSß","dD","fF","gG","hH","jJ","kK","lL","mM","ù%´","µ£`",
 "wW","xX","cC","vV","bB","nN",",?",";.",":/","=+~",
 "<>\\"
};

/*** Layout table. Add your keyboard mappings to this list */
static struct {
 WORD lang, ansi_codepage, oem_codepage;
 const char (*key)[MAIN_LEN][4];
} main_key_tab[]={
 {MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),     1252, 437, &main_key_US},
 {MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_UK),     1252, 850, &main_key_UK},
 {MAKELANGID(LANG_GERMAN,SUBLANG_DEFAULT),         1252, 850, &main_key_DE},
 {MAKELANGID(LANG_GERMAN,SUBLANG_GERMAN_SWISS),    1252, 850, &main_key_SG},
 {MAKELANGID(LANG_SWEDISH,SUBLANG_SWEDISH),        1252, 850, &main_key_SE},
 {MAKELANGID(LANG_NORWEGIAN,SUBLANG_DEFAULT),      1252, 865, &main_key_NO},
 {MAKELANGID(LANG_DANISH,SUBLANG_DEFAULT),         1252, 865, &main_key_DA},
 {MAKELANGID(LANG_FRENCH,SUBLANG_DEFAULT),         1252, 850, &main_key_FR},
 {MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH_CANADIAN), 1252, 863, &main_key_CF},
 {MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH_BELGIAN),  1252, 850, &main_key_BE},
 {MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH_SWISS),    1252, 850, &main_key_SF},
 {MAKELANGID(LANG_WALON,SUBLANG_DEFAULT),          1252, 850, &main_key_BE},
 {MAKELANGID(LANG_PORTUGUESE,SUBLANG_DEFAULT),     1252, 860, &main_key_PT},
 {MAKELANGID(LANG_FINNISH,SUBLANG_DEFAULT),        1252, 850, &main_key_FI},
 {MAKELANGID(LANG_RUSSIAN,SUBLANG_DEFAULT),        1251, 866, &main_key_RU},
 {MAKELANGID(LANG_SPANISH,SUBLANG_DEFAULT),        1252, 850, &main_key_ES},
 {MAKELANGID(LANG_DUTCH,SUBLANG_DUTCH_BELGIAN),    1252, 850, &main_key_BE},
 {MAKELANGID(LANG_ITALIAN,SUBLANG_DEFAULT),        1252, 850, &main_key_IT},
 {MAKELANGID(LANG_ICELANDIC,SUBLANG_DEFAULT),      1252, 850, &main_key_IS},

 {0} /* sentinel */
};
static unsigned kbd_layout=0; /* index into above table of layouts */

/* maybe more of these scancodes should be extended? */
                /* extended must be set for ALT_R, CTRL_R,
                   INS, DEL, HOME, END, PAGE_UP, PAGE_DOWN, ARROW keys,
                   keypad / and keypad ENTER (SDK 3.1 Vol.3 p 138) */
                /* FIXME should we set extended bit for NumLock ? My
                 * Windows does ... DF */
static const int special_key_vkey[] =
{
    VK_BACK, VK_TAB, 0, VK_CLEAR, 0, VK_RETURN, 0, 0,           /* FF08 */
    0, 0, 0, VK_PAUSE, VK_SCROLL, 0, 0, 0,                      /* FF10 */
    0, 0, 0, VK_ESCAPE                                          /* FF18 */
};
static const int special_key_scan[] =
{
    0x0E, 0x0F, 0, /*?*/ 0, 0, 0x1C, 0, 0,                      /* FF08 */
    0,    0,    0, 0x45, 0x46, 0   , 0, 0,                      /* FF10 */
    0,    0,    0, 0x01                                         /* FF18 */
};

static const int cursor_key_vkey[] =
{
    VK_HOME, VK_LEFT, VK_UP, VK_RIGHT, VK_DOWN, VK_PRIOR, 
                                       VK_NEXT, VK_END          /* FF50 */
};
static const int cursor_key_scan[] =
{
    0x147, 0x14B, 0x148, 0x14D, 0x150, 0x149, 0x151, 0x14F      /* FF50 */
};

static const int misc_key_vkey[] =
{
    VK_SELECT, VK_SNAPSHOT, VK_EXECUTE, VK_INSERT, 0, 0, 0, 0,  /* FF60 */
    VK_CANCEL, VK_HELP, VK_CANCEL, VK_MENU                      /* FF68 */
};
static const int misc_key_scan[] =
{
    /*?*/ 0, 0x37, /*?*/ 0, 0x152, 0, 0, 0, 0,                  /* FF60 */
    /*?*/ 0, /*?*/ 0, 0x38                                      /* FF68 */
};

static const int keypad_key_vkey[] =
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
static const int keypad_key_scan[] =
{
    0x138, 0x45,                                                /* FF7E */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF80 */
    0, 0, 0, 0, 0, 0x11C, 0, 0,                                 /* FF88 */
    0, 0, 0, 0, 0, 0x47, 0x4B, 0x48,                            /* FF90 */
    0x4D, 0x50, 0x49, 0x51, 0x4F, 0x4C, 0x52, 0x53,             /* FF98 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FFA0 */
    0, 0, 0x37, 0x4E, /*?*/ 0, 0x4A, 0x53, 0x135,               /* FFA8 */
    0x52, 0x4F, 0x50, 0x51, 0x4B, 0x4C, 0x4D, 0x47,             /* FFB0 */
    0x48, 0x49                                                  /* FFB8 */
};
    
static const int function_key_vkey[] =
{
    VK_F1, VK_F2,                                               /* FFBE */
    VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10,    /* FFC0 */
    VK_F11, VK_F12, VK_F13, VK_F14, VK_F15, VK_F16              /* FFC8 */
};
static const int function_key_scan[] =
{
    0x3B, 0x3C,                                                 /* FFBE */
    0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44,             /* FFC0 */
    0x57, 0x58, 0, 0, 0, 0                                      /* FFC8 */
};

static const int modifier_key_vkey[] =
{
    VK_SHIFT, VK_SHIFT, VK_CONTROL, VK_CONTROL, VK_CAPITAL, 0, /* FFE1 */
    VK_MENU, VK_MENU, VK_MENU, VK_MENU                         /* FFE7 */
};
static const int modifier_key_scan[] =
{
    0x2A, 0x36, 0x1D, 0x11D, 0x3A, 0,                          /* FFE1 */
    0x38, 0x138, 0x38, 0x138                                   /* FFE7 */
};

/* Returns the Windows virtual key code associated with the X event <e> */
static WORD EVENT_event_to_vkey( XKeyEvent *e)
{
    KeySym keysym;

    TSXLookupString(e, NULL, 0, &keysym, NULL);

    if ((keysym >= 0xFFAE) && (keysym <= 0xFFB9) && (keysym != 0xFFAF) 
	&& (e->state & NumLockMask)) 
        /* Only the Keypad keys 0-9 and . send different keysyms
         * depending on the NumLock state */
        return keypad_key_vkey[(keysym & 0xFF) - 0x7E];

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
void KEYBOARD_GenerateMsg( WORD vkey, WORD scan, int Evtype, INT event_x, INT event_y,
                           DWORD event_time )
{
  BOOL * State = (vkey==VK_NUMLOCK? &NumState : &CapsState);
  DWORD up, down;

  if (*State) {
    /* The INTERMEDIARY state means : just after a 'press' event, if a 'release' event comes,
       don't treat it. It's from the same key press. Then the state goes to ON.
       And from there, a 'release' event will switch off the toggle key. */
    *State=FALSE;
    TRACE_(keyboard)("INTERM : don\'t treat release of toggle key. InputKeyStateTable[%#x] = %#x\n",vkey,pKeyStateTable[vkey]);
  } else
    {
        down = (vkey==VK_NUMLOCK ? KEYEVENTF_EXTENDEDKEY : 0);
        up = (vkey==VK_NUMLOCK ? KEYEVENTF_EXTENDEDKEY : 0) | KEYEVENTF_KEYUP;
	if ( pKeyStateTable[vkey] & 0x1 ) /* it was ON */
	  {
	    if (Evtype!=KeyPress)
	      {
		TRACE_(keyboard)("ON + KeyRelease => generating DOWN and UP messages.\n");
	        KEYBOARD_SendEvent( vkey, scan, down,
                                    event_x, event_y, event_time );
	        KEYBOARD_SendEvent( vkey, scan, up, 
                                    event_x, event_y, event_time );
		*State=FALSE;
		pKeyStateTable[vkey] &= ~0x01; /* Toggle state to off. */ 
	      } 
	  }
	else /* it was OFF */
	  if (Evtype==KeyPress)
	    {
	      TRACE_(keyboard)("OFF + Keypress => generating DOWN and UP messages.\n");
	      KEYBOARD_SendEvent( vkey, scan, down,
                                  event_x, event_y, event_time );
	      KEYBOARD_SendEvent( vkey, scan, up, 
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
        TRACE_(keyboard)("Adjusting state for vkey %#.2x. State before %#.2x \n",
              vkey, pKeyStateTable[vkey]);

        /* Fake key being pressed inside wine */
	KEYBOARD_SendEvent( vkey, 0, state? 0 : KEYEVENTF_KEYUP, 
                            0, 0, GetTickCount() );

        TRACE_(keyboard)("State after %#.2x \n",pKeyStateTable[vkey]);
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

    TRACE_(keyboard)("called\n");
    if (!TSXQueryKeymap(display, keys_return)) {
        ERR_(keyboard)("Error getting keymap !");
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

    TRACE_(key)("EVENT_key : state = %X\n", event->state);
    if (keysym == XK_Mode_switch)
	{
	TRACE_(key)("Alt Gr key event received\n");
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
	TRACE_(key)("%s : keysym=%lX (%s), ascii chars=%u / %X / '%s'\n", 
		     (event->type == KeyPress) ? "KeyPress" : "KeyRelease",
		     keysym, ksname, ascii_chars, Str[0] & 0xff, Str);
    }

    vkey = EVENT_event_to_vkey(event);
    if (force_extended) vkey |= 0x100;

    TRACE_(key)("keycode 0x%x converted to vkey 0x%x\n",
		    event->keycode, vkey);

   if (vkey)
   {
    switch (vkey & 0xff)
    {
    case VK_NUMLOCK:    
      KEYBOARD_GenerateMsg( VK_NUMLOCK, 0x45, event->type, event_x, event_y,
                            event_time );
      break;
    case VK_CAPITAL:
      TRACE_(keyboard)("Caps Lock event. (type %d). State before : %#.2x\n",event->type,pKeyStateTable[vkey]);
      KEYBOARD_GenerateMsg( VK_CAPITAL, 0x3A, event->type, event_x, event_y,
                            event_time ); 
      TRACE_(keyboard)("State after : %#.2x\n",pKeyStateTable[vkey]);
      break;
    default:
        /* Adjust the NUMLOCK state if it has been changed outside wine */
	if (!(pKeyStateTable[VK_NUMLOCK] & 0x01) != !(event->state & NumLockMask))
	  { 
	    TRACE_(keyboard)("Adjusting NumLock state. \n");
	    KEYBOARD_GenerateMsg( VK_NUMLOCK, 0x45, KeyPress, event_x, event_y,
                                  event_time );
	    KEYBOARD_GenerateMsg( VK_NUMLOCK, 0x45, KeyRelease, event_x, event_y,
                                  event_time );
	  }
        /* Adjust the CAPSLOCK state if it has been changed outside wine */
	if (!(pKeyStateTable[VK_CAPITAL] & 0x01) != !(event->state & LockMask))
	  {
              TRACE_(keyboard)("Adjusting Caps Lock state.\n");
	    KEYBOARD_GenerateMsg( VK_CAPITAL, 0x3A, KeyPress, event_x, event_y,
                                  event_time );
	    KEYBOARD_GenerateMsg( VK_CAPITAL, 0x3A, KeyRelease, event_x, event_y,
                                  event_time );
	  }
	/* Not Num nor Caps : end of intermediary states for both. */
	NumState = FALSE;
	CapsState = FALSE;

	bScan = keyc2scan[event->keycode] & 0xFF;
	TRACE_(key)("bScan = 0x%02x.\n", bScan);

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
 *		X11DRV_KEYBOARD_DetectLayout
 *
 * Called from X11DRV_KEYBOARD_Init
 *  This routine walks through the defined keyboard layouts and selects
 *  whichever matches most closely.
 */
void
X11DRV_KEYBOARD_DetectLayout (void)
{
  unsigned current, match, mismatch, seq;
  int score, keyc, i, key, pkey, ok, syms;
  KeySym keysym;
  const char (*lkey)[MAIN_LEN][4];
  unsigned max_seq = 0;
  int max_score = 0, ismatch = 0;
  char ckey[4] =
  {0, 0, 0, 0};

  syms = keysyms_per_keycode;
  if (syms > 4) {
    WARN_(keyboard)("%d keysyms per keycode not supported, set to 4", syms);
    syms = 4;
  }
  for (current = 0; main_key_tab[current].lang; current++) {
    TRACE_(keyboard)("Attempting to match against layout %04x\n",
	   main_key_tab[current].lang);
    match = 0;
    mismatch = 0;
    score = 0;
    seq = 0;
    lkey = main_key_tab[current].key;
    pkey = -1;
    for (keyc = min_keycode; keyc <= max_keycode; keyc++) {
      /* get data for keycode from X server */
      for (i = 0; i < syms; i++) {
	keysym = TSXKeycodeToKeysym (display, keyc, i);
	/* Allow both one-byte and two-byte national keysyms */
	if ((keysym < 0x800) && (keysym != ' '))
	  ckey[i] = keysym & 0xFF;
	else {
	  ckey[i] = KEYBOARD_MapDeadKeysym(keysym);
	}
      }
      if (ckey[0]) {
	/* search for a match in layout table */
	/* right now, we just find an absolute match for defined positions */
	/* (undefined positions are ignored, so if it's defined as "3#" in */
	/* the table, it's okay that the X server has "3#£", for example) */
	/* however, the score will be higher for longer matches */
	for (key = 0; key < MAIN_LEN; key++) {
	  for (ok = 0, i = 0; (ok >= 0) && (i < syms); i++) {
	    if ((*lkey)[key][i] && ((*lkey)[key][i] == ckey[i]))
	      ok++;
	    if ((*lkey)[key][i] && ((*lkey)[key][i] != ckey[i]))
	      ok = -1;
	  }
	  if (ok > 0) {
	    score += ok;
	    break;
	  }
	}
	/* count the matches and mismatches */
	if (ok > 0) {
	  match++;
	  /* and how much the keycode order matches */
	  if (key > pkey) seq++;
	  pkey = key;
	} else {
	  TRACE_(key)("mismatch for keycode %d, character %c\n", keyc,
		 ckey[0]);
	  mismatch++;
	  score -= syms;
	}
      }
    }
    TRACE_(keyboard)("matches=%d, mismatches=%d, score=%d\n",
	   match, mismatch, score);
    if ((score > max_score) ||
	((score == max_score) && (seq > max_seq))) {
      /* best match so far */
      kbd_layout = current;
      max_score = score;
      max_seq = seq;
      ismatch = !mismatch;
    }
  }
  /* we're done, report results if necessary */
  if (!ismatch) {
    FIXME_(keyboard)(
	   "Your keyboard layout was not found!\n"
	   "Instead using closest match (%04x) for scancode mapping.\n"
	   "Please define your layout in windows/x11drv/keyboard.c and submit them\n"
	   "to us for inclusion into future Wine releases.\n"
	   "See documentation/keyboard for more information.\n",
	   main_key_tab[kbd_layout].lang);
  }
  TRACE_(keyboard)("detected layout is %04x\n", main_key_tab[kbd_layout].lang);
}

/**********************************************************************
 *		X11DRV_KEYBOARD_Init
 */
void X11DRV_KEYBOARD_Init(void)
{
    KeySym *ksp;
    XModifierKeymap *mmp;
    KeySym keysym;
    KeyCode *kcp;
    XKeyEvent e2;
    WORD scan, vkey, OEMvkey;
    int keyc, i, keyn, syms;
    char ckey[4]={0,0,0,0};
    const char (*lkey)[MAIN_LEN][4];

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
                        TRACE_(key)("AltGrMask is %x\n", AltGrMask);
		    }
                    else if (TSXKeycodeToKeysym(display, *kcp, k) == XK_Num_Lock)
		    {
                        NumLockMask = 1 << i;
                        TRACE_(key)("NumLockMask is %x\n", NumLockMask);
		    }
            }
    }
    TSXFreeModifiermap(mmp);

    /* Detect the keyboard layout */
    X11DRV_KEYBOARD_DetectLayout();
    lkey = main_key_tab[kbd_layout].key;
    syms = (keysyms_per_keycode > 4) ? 4 : keysyms_per_keycode;

    /* Now build two conversion arrays :
     * keycode -> vkey + scancode + extended
     * vkey + extended -> keycode */

    e2.display = display;
    e2.state = 0;

    OEMvkey = VK_OEM_7; /* next is available.  */
    for (keyc = min_keycode; keyc <= max_keycode; keyc++)
    {
        e2.keycode = (KeyCode)keyc;
        TSXLookupString(&e2, NULL, 0, &keysym, NULL);
        vkey = 0; scan = 0;
        if (keysym)  /* otherwise, keycode not used */
        {
            if ((keysym >> 8) == 0xFF)         /* non-character key */
            {
                int key = keysym & 0xff;
		
                if (key >= 0x08 && key <= 0x1B) {        /* special key */
		    vkey = special_key_vkey[key - 0x08];
		    scan = special_key_scan[key - 0x08];
                } else if (key >= 0x50 && key <= 0x57) { /* cursor key */
		    vkey = cursor_key_vkey[key - 0x50];
		    scan = cursor_key_scan[key - 0x50];
                } else if (key >= 0x60 && key <= 0x6B) { /* miscellaneous key */
		    vkey = misc_key_vkey[key - 0x60];
		    scan = misc_key_scan[key - 0x60];
                } else if (key >= 0x7E && key <= 0xB9) { /* keypad key */
		    vkey = keypad_key_vkey[key - 0x7E];
		    scan = keypad_key_scan[key - 0x7E];
                } else if (key >= 0xBE && key <= 0xCD) { /* function key */
                    vkey = function_key_vkey[key - 0xBE] | 0x100; /* set extended bit */
                    scan = function_key_scan[key - 0xBE];
                } else if (key >= 0xE1 && key <= 0xEA) { /* modifier key */
		    vkey = modifier_key_vkey[key - 0xE1];
		    scan = modifier_key_scan[key - 0xE1];
                } else if (key == 0xFF) {                /* DEL key */
		    vkey = VK_DELETE;
		    scan = 0x153;
		}
		/* set extended bit when necessary */
		if (scan & 0x100) vkey |= 0x100;
            } else if (keysym == 0x20) {                 /* Spacebar */
	        vkey = VK_SPACE;
		scan = 0x39;
	    } else {
	      /* we seem to need to search the layout-dependent scancodes */
	      int maxlen=0,maxval=-1,ok;
	      for (i=0; i<syms; i++) {
		keysym = TSXKeycodeToKeysym(display, keyc, i);
		if ((keysym<0x800) && (keysym!=' ')) {
		  ckey[i] = keysym & 0xFF;
		} else {
		  ckey[i] = KEYBOARD_MapDeadKeysym(keysym);
		}
	      }
	      /* find key with longest match streak */
	      for (keyn=0; keyn<MAIN_LEN; keyn++) {
		for (ok=(*lkey)[keyn][i=0]; ok&&(i<4); i++)
		  if ((*lkey)[keyn][i] && (*lkey)[keyn][i]!=ckey[i]) ok=0;
		if (ok||(i>maxlen)) {
		  maxlen=i; maxval=keyn;
		}
		if (ok) break;
	      }
	      if (maxval>=0) {
		/* got it */
		scan = main_key_scan[maxval];
	      }
	    }

            /* find a suitable layout-dependent VK code */
	    /* (most Winelib apps ought to be able to work without layout tables!) */
            for (i = 0; (i < keysyms_per_keycode) && (!vkey); i++)
            {
                keysym = TSXLookupKeysym(&e2, i);
                if ((keysym >= VK_0 && keysym <= VK_9)
                    || (keysym >= VK_A && keysym <= VK_Z)) {
		    vkey = keysym;
		}
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
                case 0xf6 : OEMvkey=0xf5; WARN_(keyboard)("No more OEM vkey available!\n");
                }

                vkey = OEMvkey;
		  
                if (TRACE_ON(keyboard))
                {
		    dbg_decl_str(keyboard, 1024);

                    TRACE_(keyboard)("OEM specific virtual key %X assigned "
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
                    TRACE_(keyboard)("(%s)\n", dbg_str(keyboard));
                }
            }
        }
        keyc2vkey[e2.keycode] = vkey;
        keyc2scan[e2.keycode] = scan;
    } /* for */

    /* If some keys still lack scancodes, assign some arbitrary ones to them now */
    for (scan = 0x60, keyc = min_keycode; keyc <= max_keycode; keyc++)
      if (keyc2vkey[keyc]&&!keyc2scan[keyc]) {
	char *ksname;
	keysym = TSXKeycodeToKeysym(display, keyc, 0);
	ksname = TSXKeysymToString(keysym);
	if (!ksname) ksname = "NoSymbol";

	/* should make sure the scancode is unassigned here, but >=0x60 currently always is */

	TRACE_(key)("assigning scancode %02x to unidentified keycode %02x (%s)\n",scan,keyc,ksname);
	keyc2scan[keyc]=scan++;
      }

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

	TRACE_(keyboard)("VkKeyScan '%c'(%#lx, %lu): got keycode %#.2x\n",
			 cChar,keysym,keysym,keycode);
	
	if (keycode)
	  {
	    for (index=-1, i=0; (i<8) && (index<0); i++) /* find shift state */
	      if (TSXKeycodeToKeysym(display,keycode,i)==keysym) index=i;
	    switch (index) {
	    case -1 :
	      WARN_(keyboard)("Keysym %lx not found while parsing the keycode table\n",keysym); break;
	    case 0 : break;
	    case 1 : highbyte = 0x0100; break;
	    case 2 : highbyte = 0x0600; break;
	    case 3 : highbyte = 0x0700; break;
	    default : ERR_(keyboard)("index %d found by XKeycodeToKeysym. please report! \n",index);
	    }
	    /*
	      index : 0     adds 0x0000
	      index : 1     adds 0x0100 (shift)
	      index : ?     adds 0x0200 (ctrl)
	      index : 2     adds 0x0600 (ctrl+alt)
	      index : 3     adds 0x0700 (ctrl+alt+shift)
	     */
	  }
	TRACE_(keyboard)(" ... returning %#.2x\n", keyc2vkey[keycode]+highbyte);
	return keyc2vkey[keycode]+highbyte;   /* keycode -> (keyc2vkey) vkey */
}

/***********************************************************************
 *		X11DRV_KEYBOARD_MapVirtualKey
 */
UINT16 X11DRV_KEYBOARD_MapVirtualKey(UINT16 wCode, UINT16 wMapType)
{
#define returnMVK(value) { TRACE_(keyboard)("returning 0x%x.\n",value); return value; }

	TRACE_(keyboard)("MapVirtualKey wCode=0x%x wMapType=%d ... \n",
			 wCode,wMapType);
	switch(wMapType) {
		case 0:	{ /* vkey-code to scan-code */
			/* let's do vkey -> keycode -> scan */
			int keyc;
			for (keyc=min_keycode; keyc<=max_keycode; keyc++)
				if ((keyc2vkey[keyc] & 0xFF) == wCode)
					returnMVK (keyc2scan[keyc] & 0xFF);
			TRACE_(keyboard)("returning no scan-code.\n");
		        return 0; }

		case 1: { /* scan-code to vkey-code */
			/* let's do scan -> keycode -> vkey */
			int keyc;
			for (keyc=min_keycode; keyc<=max_keycode; keyc++)
				if ((keyc2scan[keyc] & 0xFF) == (wCode & 0xFF))
					returnMVK (keyc2vkey[keyc] & 0xFF);
			TRACE_(keyboard)("returning no vkey-code.\n");
		        return 0; }

		case 2: { /* vkey-code to unshifted ANSI code */
			/* (was FIXME) : what does unshifted mean ? 'a' or 'A' ? */
		        /* My Windows returns 'A'. */
			/* let's do vkey -> keycode -> (XLookupString) ansi char */
			XKeyEvent e;
			KeySym keysym;
			int keyc;
			char s[2];
			e.display = display;
			e.state = 0; /* unshifted */

			e.keycode = 0;
			/* We exit on the first keycode found, to speed up the thing. */
			for (keyc=min_keycode; (keyc<=max_keycode) && (!e.keycode) ; keyc++)
			{ /* Find a keycode that could have generated this virtual key */
			    if  ((keyc2vkey[keyc] & 0xFF) == wCode)
			    { /* We filter the extended bit, we don't know it */
			        e.keycode = keyc; /* Store it temporarily */
				if ((EVENT_event_to_vkey(&e) & 0xFF) != wCode) {
				    e.keycode = 0; /* Wrong one (ex: because of the NumLock
					 state), so set it to 0, we'll find another one */
				}
			    }
			}

			if ((wCode>=VK_NUMPAD0) && (wCode<=VK_NUMPAD9))
			  e.keycode = TSXKeysymToKeycode(e.display, wCode-VK_NUMPAD0+XK_KP_0);
          
			if (wCode==VK_DECIMAL)
			  e.keycode = TSXKeysymToKeycode(e.display, XK_KP_Decimal);

			if (!e.keycode)
			{
			  WARN_(keyboard)("Unknown virtual key %X !!! \n", wCode);
			  return 0; /* whatever */
			}
			TRACE_(keyboard)("Found keycode %d (0x%2X)\n",e.keycode,e.keycode);

			if (TSXLookupString(&e, s, 2, &keysym, NULL))
			  returnMVK (*s);
			
			TRACE_(keyboard)("returning no ANSI.\n");
			return 0;
			}

		case 3:   /* **NT only** scan-code to vkey-code but distinguish between  */
              		  /*             left and right  */
		          FIXME_(keyboard)(" stub for NT\n");
                          return 0;

		default: /* reserved */
			WARN_(keyboard)("Unknown wMapType %d !\n",
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
  int vkey, ansi, scanCode;
	
  scanCode = lParam >> 16;
  scanCode &= 0x1ff;  /* keep "extended-key" flag with code */

  /* FIXME: should use MVK type 3 (NT version that distinguishes right and left */
  vkey = X11DRV_KEYBOARD_MapVirtualKey(scanCode, 1);

  /*  handle "don't care" bit (0x02000000) */
  if (!(lParam & 0x02000000)) {
    switch (vkey) {
         case VK_LSHIFT:
         case VK_RSHIFT:
                          vkey = VK_SHIFT;
                          break;
       case VK_LCONTROL:
       case VK_RCONTROL:
                          vkey = VK_CONTROL;
                          break;
          case VK_LMENU:
          case VK_RMENU:
                          vkey = VK_MENU;
                          break;
               default:
                          break;
    }
  }

  ansi = X11DRV_KEYBOARD_MapVirtualKey(vkey, 2);
  TRACE_(keyboard)("scan 0x%04x, vkey 0x%04x, ANSI 0x%04x\n",
          scanCode, vkey, ansi);

  if ( ((vkey >= 0x30) && (vkey <= 0x39)) ||
      ( (vkey >= 0x41) && (vkey <= 0x5a)) ) /* Windows VK_* are ANSI codes */
      {
        if ((nSize >= 2) && lpBuffer)
	{
        *lpBuffer = toupper((char)ansi);
          *(lpBuffer+1) = 0;
          return 1;
        } 
     else
        return 0;
  }

  /* use vkey values to construct names */

  FIXME_(keyboard)("(%08lx,%p,%d): unsupported key\n",lParam,lpBuffer,nSize);

  if (lpBuffer && nSize)
    *lpBuffer = 0;
  return 0;
}

/***********************************************************************
 *		X11DRV_KEYBOARD_MapDeadKeysym
 */
static char KEYBOARD_MapDeadKeysym(KeySym keysym)
{
	switch (keysym)
	    {
	/* symbolic ASCII is the same as defined in rfc1345 */
#ifdef XK_dead_tilde
	    case XK_dead_tilde :
#endif
	    case 0x1000FE7E : /* Xfree's XK_Dtilde */
		return '~';	/* '? */
#ifdef XK_dead_acute
	    case XK_dead_acute :
#endif
	    case 0x1000FE27 : /* Xfree's XK_Dacute_accent */
		return 0xb4;	/* '' */
#ifdef XK_dead_circumflex
	    case XK_dead_circumflex:
#endif
	    case 0x1000FE5E : /* Xfree's XK_Dcircumflex_accent */
		return '^';	/* '> */
#ifdef XK_dead_grave
	    case XK_dead_grave :
#endif
	    case 0x1000FE60 : /* Xfree's XK_Dgrave_accent */
		return '`';	/* '! */
#ifdef XK_dead_diaeresis
	    case XK_dead_diaeresis :
#endif
	    case 0x1000FE22 : /* Xfree's XK_Ddiaeresis */
		return 0xa8;	/* ': */
#ifdef XK_dead_cedilla
	    case XK_dead_cedilla :
	        return 0xb8;	/* ', */
#endif
#ifdef XK_dead_macron
	    case XK_dead_macron :
	        return '-';	/* 'm isn't defined on iso-8859-x */
#endif
#ifdef XK_dead_breve
	    case XK_dead_breve :
	        return 0xa2;	/* '( */
#endif
#ifdef XK_dead_abovedot
	    case XK_dead_abovedot :
	        return 0xff;	/* '. */
#endif
#ifdef XK_dead_abovering
	    case XK_dead_abovering :
	        return '0';	/* '0 isn't defined on iso-8859-x */
#endif
#ifdef XK_dead_doubleacute
	    case XK_dead_doubleacute :
	        return 0xbd;	/* '" */
#endif
#ifdef XK_dead_caron
	    case XK_dead_caron :
	        return 0xb7;	/* '< */
#endif
#ifdef XK_dead_ogonek
	    case XK_dead_ogonek :
	        return 0xb2;	/* '; */
#endif
/* FIXME: I don't know this three.
	    case XK_dead_iota :
	        return 'i';	 
	    case XK_dead_voiced_sound :
	        return 'v';
	    case XK_dead_semivoiced_sound :
	        return 's';
*/
	    }
	TRACE_(keyboard)("no character for dead keysym 0x%08lx\n",keysym);
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
        TRACE_(keyboard)("scanCode=0, doing nothing\n");
        return 0;
    }
    if (scanCode & 0x8000)
    {
        TRACE_(keyboard)("Key UP, doing nothing\n" );
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
    TRACE_(key)("(%04X, %04X) : faked state = %X\n",
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
	WARN_(keyboard)("Unknown virtual key %X !!! \n",virtKey);
	return virtKey; /* whatever */
      }
    else TRACE_(keyboard)("Found keycode %d (0x%2X)\n",e.keycode,e.keycode);

    ret = TSXLookupString(&e, (LPVOID)lpChar, 2, &keysym, &cs);
    if (ret == 0)
	{
	BYTE dead_char = 0;

	((char*)lpChar)[1] = '\0';
	dead_char = KEYBOARD_MapDeadKeysym(keysym);
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
		ERR_(keyboard)("Please report: no char for keysym %04lX (%s) :\n",
			keysym, ksname);
		ERR_(keyboard)("(virtKey=%X,scanCode=%X,keycode=%X,state=%X)\n",
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

    TRACE_(key)("ToAscii about to return %d with char %x\n",
		ret, *(char*)lpChar);
    return ret;
}

/***********************************************************************
 *		X11DRV_KEYBOARD_GetBeepActive
 */
BOOL X11DRV_KEYBOARD_GetBeepActive()
{
  XKeyboardState  keyboard_state;

  TSXGetKeyboardControl(display, &keyboard_state);

  return keyboard_state.bell_percent != 0;
}

/***********************************************************************
 *		X11DRV_KEYBOARD_SetBeepActive
 */
void X11DRV_KEYBOARD_SetBeepActive(BOOL bActivate)
{
  XKeyboardControl keyboard_value;
  
  if(bActivate)
    keyboard_value.bell_percent = -1;
  else
    keyboard_value.bell_percent = 0;
  
  TSXChangeKeyboardControl(display, KBBellPercent, &keyboard_value);
}

/***********************************************************************
 *		X11DRV_KEYBOARD_Beep
 */
void X11DRV_KEYBOARD_Beep()
{
  TSXBell(display, 0);
}

#endif /* !defined(X_DISPLAY_MISSING) */

