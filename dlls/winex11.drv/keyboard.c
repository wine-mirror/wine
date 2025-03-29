/*
 * X11 keyboard driver
 *
 * Copyright 1993 Bob Amstadt
 * Copyright 1996 Albrecht Kleine
 * Copyright 1997 David Faure
 * Copyright 1998 Morten Welinder
 * Copyright 1998 Ulrich Weigand
 * Copyright 1999 Ove Kåven
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#include "x11drv.h"

#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winnls.h"
#include "kbd.h"
#include "wine/server.h"
#include "wine/debug.h"

/* log format (add 0-padding as appropriate):
    keycode  %u  as in output from xev
    keysym   %lx as in X11/keysymdef.h
    vkey     %X  as in winuser.h
    scancode %x
*/
WINE_DEFAULT_DEBUG_CHANNEL(keyboard);
WINE_DECLARE_DEBUG_CHANNEL(key);

static const unsigned int ControlMask = 1 << 2;

static int min_keycode, max_keycode, keysyms_per_keycode;
static WORD keyc2vkey[256], keyc2scan[256];

static int NumLockMask, ScrollLockMask, AltGrMask; /* mask in the XKeyEvent state */

static pthread_mutex_t kbd_mutex = PTHREAD_MUTEX_INITIALIZER;

static char KEYBOARD_MapDeadKeysym(KeySym keysym);

/* Keyboard translation tables */
#define MAIN_LEN 49
static const WORD main_key_scan_qwerty[MAIN_LEN] =
{
/* this is my (102-key) keyboard layout, sorry if it doesn't quite match yours */
 /* `    1    2    3    4    5    6    7    8    9    0    -    = */
   0x29,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,
 /* q    w    e    r    t    y    u    i    o    p    [    ] */
   0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,
 /* a    s    d    f    g    h    j    k    l    ;    '    \ */
   0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x2B,
 /* z    x    c    v    b    n    m    ,    .    / */
   0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,
   0x56 /* the 102nd key (actually to the right of l-shift) */
};

static const WORD main_key_scan_abnt_qwerty[MAIN_LEN] =
{
 /* `    1    2    3    4    5    6    7    8    9    0    -    = */
   0x29,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,
 /* q    w    e    r    t    y    u    i    o    p    [    ] */
   0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,
 /* a    s    d    f    g    h    j    k    l    ;    '    \ */
   0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x2B,
 /* \      z    x    c    v    b    n    m    ,    .    / */
   0x5e,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,
   0x56, /* the 102nd key (actually to the right of l-shift) */
};

static const WORD main_key_scan_qwerty_jp106[MAIN_LEN] =
{
 /* 1    2    3    4    5    6    7    8    9    0    -    ^    \ (Yen) */
   0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x7D,
 /* q    w    e    r    t    y    u    i    o    p    @    [ */
   0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,
 /* a    s    d    f    g    h    j    k    l    ;    :    ] */
   0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x2B,
 /* z    x    c    v    b    n    m    ,    .    /    \ (Underscore) */
   0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,0x73
};


static const WORD main_key_vkey_qwerty[MAIN_LEN] =
{
/* NOTE: this layout must concur with the scan codes layout above */
   VK_OEM_3,'1','2','3','4','5','6','7','8','9','0',VK_OEM_MINUS,VK_OEM_PLUS,
   'Q','W','E','R','T','Y','U','I','O','P',VK_OEM_4,VK_OEM_6,
   'A','S','D','F','G','H','J','K','L',VK_OEM_1,VK_OEM_7,VK_OEM_5,
   'Z','X','C','V','B','N','M',VK_OEM_COMMA,VK_OEM_PERIOD,VK_OEM_2,
   VK_OEM_102 /* the 102nd key (actually to the right of l-shift) */
};

static const WORD main_key_vkey_qwerty_jp106[MAIN_LEN] =
{
/* NOTE: this layout must concur with the scan codes layout above */
   '1','2','3','4','5','6','7','8','9','0',VK_OEM_MINUS,VK_OEM_7,VK_OEM_5,
   'Q','W','E','R','T','Y','U','I','O','P',VK_OEM_3,VK_OEM_4,
   'A','S','D','F','G','H','J','K','L',VK_OEM_PLUS,VK_OEM_1,VK_OEM_6,
   'Z','X','C','V','B','N','M',VK_OEM_COMMA,VK_OEM_PERIOD,VK_OEM_2,
   VK_OEM_102 /* the 102nd key (actually to the left of r-shift) */
};

static const WORD main_key_vkey_qwerty_v2[MAIN_LEN] =
{
/* NOTE: this layout must concur with the scan codes layout above */
   VK_OEM_5,'1','2','3','4','5','6','7','8','9','0',VK_OEM_PLUS,VK_OEM_4,
   'Q','W','E','R','T','Y','U','I','O','P',VK_OEM_6,VK_OEM_1,
   'A','S','D','F','G','H','J','K','L',VK_OEM_3,VK_OEM_7,VK_OEM_2,
   'Z','X','C','V','B','N','M',VK_OEM_COMMA,VK_OEM_PERIOD,VK_OEM_MINUS,
   VK_OEM_102 /* the 102nd key (actually to the right of l-shift) */
};

static const WORD main_key_vkey_qwertz[MAIN_LEN] =
{
/* NOTE: this layout must concur with the scan codes layout above */
   VK_OEM_3,'1','2','3','4','5','6','7','8','9','0',VK_OEM_MINUS,VK_OEM_PLUS,
   'Q','W','E','R','T','Z','U','I','O','P',VK_OEM_4,VK_OEM_6,
   'A','S','D','F','G','H','J','K','L',VK_OEM_1,VK_OEM_7,VK_OEM_5,
   'Y','X','C','V','B','N','M',VK_OEM_COMMA,VK_OEM_PERIOD,VK_OEM_2,
   VK_OEM_102 /* the 102nd key (actually to the right of l-shift) */
};

static const WORD main_key_vkey_abnt_qwerty[MAIN_LEN] =
{
/* NOTE: this layout must concur with the scan codes layout above */
   VK_OEM_3,'1','2','3','4','5','6','7','8','9','0',VK_OEM_MINUS,VK_OEM_PLUS,
   'Q','W','E','R','T','Y','U','I','O','P',VK_OEM_4,VK_OEM_6,
   'A','S','D','F','G','H','J','K','L',VK_OEM_1,VK_OEM_8,VK_OEM_5,
   VK_OEM_7,'Z','X','C','V','B','N','M',VK_OEM_COMMA,VK_OEM_PERIOD,VK_OEM_2,
   VK_OEM_102, /* the 102nd key (actually to the right of l-shift) */
};

static const WORD main_key_vkey_azerty[MAIN_LEN] =
{
/* NOTE: this layout must concur with the scan codes layout above */
   VK_OEM_7,'1','2','3','4','5','6','7','8','9','0',VK_OEM_4,VK_OEM_PLUS,
   'A','Z','E','R','T','Y','U','I','O','P',VK_OEM_6,VK_OEM_1,
   'Q','S','D','F','G','H','J','K','L','M',VK_OEM_3,VK_OEM_5,
   'W','X','C','V','B','N',VK_OEM_COMMA,VK_OEM_PERIOD,VK_OEM_2,VK_OEM_8,
   VK_OEM_102 /* the 102nd key (actually to the right of l-shift) */
};

static const WORD main_key_vkey_dvorak[MAIN_LEN] =
{
/* NOTE: this layout must concur with the scan codes layout above */
   VK_OEM_3,'1','2','3','4','5','6','7','8','9','0',VK_OEM_4,VK_OEM_6,
   VK_OEM_7,VK_OEM_COMMA,VK_OEM_PERIOD,'P','Y','F','G','C','R','L',VK_OEM_2,VK_OEM_PLUS,
   'A','O','E','U','I','D','H','T','N','S',VK_OEM_MINUS,VK_OEM_5,
   VK_OEM_1,'Q','J','K','X','B','M','W','V','Z',
   VK_OEM_102 /* the 102nd key (actually to the right of l-shift) */
};

/*** DEFINE YOUR NEW LANGUAGE-SPECIFIC MAPPINGS BELOW, SEE EXISTING TABLES */

/* the VK mappings for the main keyboard will be auto-assigned as before,
   so what we have here is just the character tables */
/* order: Normal, Shift, AltGr, Shift-AltGr */
/* We recommend you write just what is guaranteed to be correct (i.e. what's
   written on the keycaps), not the bunch of special characters behind AltGr
   and Shift-AltGr if it can vary among different X servers */
/* These tables serve to guess the keyboard type and scancode mapping.
   Complete modeling is not important, identification/discrimination is. */
/* Remember that your 102nd key (to the right of l-shift) should be on a
   separate line, see existing tables */
/* If Wine fails to match your new table, use WINEDEBUG=+key to find out why */
/* Remember to also add your new table to the layout index table far below! */

/*** United States keyboard layout (mostly contributed by Uwe Bonnes) */
static const char main_key_US[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","[{","]}",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";:","'\"","\\|",
 "zZ","xX","cC","vV","bB","nN","mM",",<",".>","/?"
};

/*** United States keyboard layout (phantom key version) */
/* (XFree86 reports the <> key even if it's not physically there) */
static const char main_key_US_phantom[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","[{","]}",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";:","'\"","\\|",
 "zZ","xX","cC","vV","bB","nN","mM",",<",".>","/?",
 "<>" /* the phantom key */
};

/*** United States keyboard layout (dvorak version) */
static const char main_key_US_dvorak[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","[{","]}",
 "'\"",",<",".>","pP","yY","fF","gG","cC","rR","lL","/?","=+",
 "aA","oO","eE","uU","iI","dD","hH","tT","nN","sS","-_","\\|",
 ";:","qQ","jJ","kK","xX","bB","mM","wW","vV","zZ"
};

/*** United States keyboard layout (dvorak phantom key version) */
static const char main_key_US_dvorak_phantom[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","[{","]}",
 "'\"",",<",".>","pP","yY","fF","gG","cC","rR","lL","/?","=+",
 "aA","oO","eE","uU","iI","dD","hH","tT","nN","sS","-_","\\|",
 ";:","qQ","jJ","kK","xX","bB","mM","wW","vV","zZ",
 "<>"
};

/*** British keyboard layout */
static const char main_key_UK[MAIN_LEN][4] =
{
 "`","1!","2\"","3\xa3","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","[{","]}",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";:","'@","#~",
 "zZ","xX","cC","vV","bB","nN","mM",",<",".>","/?",
 "\\|"
};

/*** French keyboard layout (setxkbmap fr) */
static const char main_key_FR[MAIN_LEN][4] =
{
 "\xb2","&1","\xe9""2","\"3","'4","(5","-6","\xe8""7","_8","\xe7""9","\xe0""0",")\xb0","=+",
 "aA","zZ","eE","rR","tT","yY","uU","iI","oO","pP","^\xa8","$\xa3",
 "qQ","sS","dD","fF","gG","hH","jJ","kK","lL","mM","\xf9%","*\xb5",
 "wW","xX","cC","vV","bB","nN",",?",";.",":/","!\xa7",
 "<>"
};

/*** Icelandic keyboard layout (setxkbmap is) */
static const char main_key_IS[MAIN_LEN][4] =
{
 "\xb0","1!","2\"","3#","4$","5%","6&","7/","8(","9)","0=","\xf6\xd6","-_",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","\xf0\xd0","'?",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xe6\xc6","\xb4\xc4","+*",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","\xfe\xde",
 "<>"
};

/* All german keyb layout tables have the acute/apostrophe symbol next to
 * the BACKSPACE key removed (replaced with NULL which is ignored by the
 * detection code).
 * This was done because the mapping of the acute (and apostrophe) is done
 * differently in various xkb-data/xkeyboard-config versions. Some replace
 * the acute with a normal apostrophe, so that the apostrophe is found twice
 * on the keyboard (one next to BACKSPACE and one next to ENTER).
 * Others put the acute and grave accents on the key left of BACKSPACE.
 * More information on the fd.o bugtracker:
 * https://bugs.freedesktop.org/show_bug.cgi?id=11514
 * Keys reachable via AltGr (@, [], ~, \, |, {}) differ completely
 * among PC and Mac keyboards, so these are not listed.
 */

/*** German keyboard layout (setxkbmap de [-variant nodeadkeys|deadacute etc.]) */
static const char main_key_DE[MAIN_LEN][4] =
{
 "^\xb0","1!","2\"","3\xa7","4$","5%","6&","7/","8(","9)","0=","\xdf?","",
 "qQ","wW","eE","rR","tT","zZ","uU","iI","oO","pP","\xfc\xdc","+*",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xf6\xd6","\xe4\xc4","#'",
 "yY","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Swiss German keyboard layout (setxkbmap ch -variant de) */
static const char main_key_SG[MAIN_LEN][4] =
{
 "\xa7\xb0","1+","2\"","3*","4\xe7","5%","6&","7/","8(","9)","0=","'?","^`",
 "qQ","wW","eE","rR","tT","zZ","uU","iI","oO","pP","\xfc\xe8","\xa8!",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xf6\xe9","\xe4\xe0","$\xa3",
 "yY","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Swiss French keyboard layout (setxkbmap ch -variant fr) */
static const char main_key_SF[MAIN_LEN][4] =
{
 "\xa7\xb0","1+","2\"","3*","4\xe7","5%","6&","7/","8(","9)","0=","'?","^`",
 "qQ","wW","eE","rR","tT","zZ","uU","iI","oO","pP","\xe8\xfc","\xa8!",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xe9\xf6","\xe0\xe4","$\xa3",
 "yY","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Norwegian keyboard layout (contributed by Ove Kåven) */
static const char main_key_NO[MAIN_LEN][4] =
{
 "|\xa7","1!","2\"@","3#\xa3","4\xa4$","5%","6&","7/{","8([","9)]","0=}","+?","\\`\xb4",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","\xe5\xc5","\xa8^~",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xf8\xd8","\xe6\xc6","'*",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Danish keyboard layout (setxkbmap dk) */
static const char main_key_DA[MAIN_LEN][4] =
{
 "\xbd\xa7","1!","2\"","3#","4\xa4","5%","6&","7/","8(","9)","0=","+?","\xb4`",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","\xe5\xc5","\xa8^",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xe6\xc6","\xf8\xd8","'*",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Swedish keyboard layout (setxkbmap se) */
static const char main_key_SE[MAIN_LEN][4] =
{
 "\xa7\xbd","1!","2\"","3#","4\xa4","5%","6&","7/","8(","9)","0=","+?","\xb4`",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","\xe5\xc5","\xa8^",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xf6\xd6","\xe4\xc4","'*",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Estonian keyboard layout (setxkbmap ee) */
static const char main_key_ET[MAIN_LEN][4] =
{
 "\xb7~","1!","2\"","3#","4\xa4","5%","6&","7/","8(","9)","0=","+?","\xb4`",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","\xfc\xdc","\xf5\xd5",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xf6\xd6","\xe4\xc4","'*",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Canadian French keyboard layout (setxkbmap ca_enhanced) */
static const char main_key_CF[MAIN_LEN][4] =
{
 "#|\\","1!\xb1","2\"@","3/\xa3","4$\xa2","5%\xa4","6?\xac","7&\xa6","8*\xb2","9(\xb3","0)\xbc","-_\xbd","=+\xbe",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO\xa7","pP\xb6","^^[","\xb8\xa8]",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";:~","``{","<>}",
 "zZ","xX","cC","vV","bB","nN","mM",",'-",".","\xe9\xc9",
 "\xab\xbb\xb0"
};

/*** Canadian French keyboard layout (setxkbmap ca -variant fr) */
static const char main_key_CA_fr[MAIN_LEN][4] =
{
 "#|","1!","2\"","3/","4$","5%","6?","7&","8*","9(","0)","-_","=+",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","^^","\xb8\xa8",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";:","``","<>",
 "zZ","xX","cC","vV","bB","nN","mM",",'",".","\xe9\xc9",
 "\xab\xbb"
};

/*** Canadian keyboard layout (setxkbmap ca) */
static const char main_key_CA[MAIN_LEN][4] =
{
 "/\\","1!\xb9\xa1","2@\xb2","3#\xb3\xa3","4$\xbc\xa4","5%\xbd","6?\xbe","7&","8*","9(","0)","-_","=+",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO\xf8\xd8","pP\xfe\xde","^\xa8\xa8","\xe7\xc7~",
 "aA\xe6\xc6","sS\xdf\xa7","dD\xf0\xd0","fF","gG","hH","jJ","kK","lL",";:\xb4","\xe8\xc8","\xe0\xc0",
 "zZ","xX","cC\xa2\xa9","vV","bB","nN","mM\xb5\xba",",'",".\"\xb7\xf7","\xe9\xc9",
 "\xf9\xd9"
};

/*** Portuguese keyboard layout (setxkbmap pt) */
static const char main_key_PT[MAIN_LEN][4] =
{
 "\\|","1!","2\"","3#","4$","5%","6&","7/","8(","9)","0=","'?","\xab\xbb",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","+*","\xb4`",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xe7\xc7","\xba\xaa","~^",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Italian keyboard layout (setxkbmap it) */
static const char main_key_IT[MAIN_LEN][4] =
{
 "\\|","1!","2\"","3\xa3","4$","5%","6&","7/","8(","9)","0=","'?","\xec^",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","\xe8\xe9","+*",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xf2\xe7","\xe0\xb0","\xf9\xa7",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Finnish keyboard layout (setxkbmap fi) */
static const char main_key_FI[MAIN_LEN][4] =
{
 "\xa7\xbd","1!","2\"","3#","4\xa4","5%","6&","7/","8(","9)","0=","+?","\xb4`",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","\xe5\xc5","\xa8^",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xf6\xd6","\xe4\xc4","'*",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Bulgarian bds keyboard layout */
static const char main_key_BG_bds[MAIN_LEN][4] =
{
 "`~()","1!","2@2?","3#3+","4$4\"","5%","6^6=","7&7:","8*8/","9(","0)","-_-I","=+.V",
 "qQ,\xfb","wW\xf3\xd3","eE\xe5\xc5","rR\xe8\xc8","tT\xf8\xd8","yY\xf9\xd9","uU\xea\xca","iI\xf1\xd1","oO\xe4\xc4","pP\xe7\xc7","[{\xf6\xd6","]};",
 "aA\xfc\xdc","sS\xff\xdf","dD\xe0\xc0","fF\xee\xce","gG\xe6\xc6","hH\xe3\xc3","jJ\xf2\xd2","kK\xed\xcd","lL\xe2\xc2",";:\xec\xcc","'\"\xf7\xd7","\\|'\xdb",
 "zZ\xfe\xde","xX\xe9\xc9","cC\xfa\xda","vV\xfd\xdd","bB\xf4\xd4","nN\xf5\xd5","mM\xef\xcf",",<\xf0\xd0",".>\xeb\xcb","/?\xe1\xc1",
 "<>" /* the phantom key */
};

/*** Bulgarian phonetic keyboard layout */
static const char main_key_BG_phonetic[MAIN_LEN][4] =
{
 "`~\xf7\xd7","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ\xff\xdf","wW\xe2\xc2","eE\xe5\xc5","rR\xf0\xd0","tT\xf2\xd2","yY\xfa\xda","uU\xf3\xd3","iI\xe8\xc8","oO\xee\xce","pP\xef\xcf","[{\xf8\xd8","]}\xf9\xd9",
 "aA\xe0\xc0","sS\xf1\xd1","dD\xe4\xc4","fF\xf4\xd4","gG\xe3\xc3","hH\xf5\xd5","jJ\xe9\xc9","kK\xea\xca","lL\xeb\xcb",";:","'\"","\\|\xfe\xde",
 "zZ\xe7\xc7","xX\xfc\xdc","cC\xf6\xd6","vV\xe6\xc6","bB\xe1\xc1","nN\xed\xcd","mM\xec\xcc",",<",".>","/?",
 "<>" /* the phantom key */
};

/*** Belarusian standard keyboard layout (contributed by Hleb Valoska) */
/*** It matches Belarusian layout for XKB from Alexander Mikhailian    */
static const char main_key_BY[MAIN_LEN][4] =
{
 "`~\xa3\xb3","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ\xca\xea","wW\xc3\xe3","eE\xd5\xf5","rR\xcb\xeb","tT\xc5\xe5","yY\xce\xee","uU\xc7\xe7","iI\xdb\xfb","oO\xae\xbe","pP\xda\xfa","[{\xc8\xe8","]}''",
 "aA\xc6\xe6","sS\xd9\xf9","dD\xd7\xf7","fF\xc1\xe1","gG\xd0\xf0","hH\xd2\xf2","jJ\xcf\xef","kK\xcc\xec","lL\xc4\xe4",";:\xd6\xf6","'\"\xdc\xfc","\\|/|",
 "zZ\xd1\xf1","xX\xde\xfe","cC\xd3\xf3","vV\xcd\xed","bB\xa6\xb6","nN\xd4\xf4","mM\xd8\xf8",",<\xc2\xe2",".>\xc0\xe0","/?.,", "<>|\xa6",
};


/*** Russian keyboard layout (contributed by Pavel Roskin) */
static const char main_key_RU[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ\xca\xea","wW\xc3\xe3","eE\xd5\xf5","rR\xcb\xeb","tT\xc5\xe5","yY\xce\xee","uU\xc7\xe7","iI\xdb\xfb","oO\xdd\xfd","pP\xda\xfa","[{\xc8\xe8","]}\xdf\xff",
 "aA\xc6\xe6","sS\xd9\xf9","dD\xd7\xf7","fF\xc1\xe1","gG\xd0\xf0","hH\xd2\xf2","jJ\xcf\xef","kK\xcc\xec","lL\xc4\xe4",";:\xd6\xf6","'\"\xdc\xfc","\\|",
 "zZ\xd1\xf1","xX\xde\xfe","cC\xd3\xf3","vV\xcd\xed","bB\xc9\xe9","nN\xd4\xf4","mM\xd8\xf8",",<\xc2\xe2",".>\xc0\xe0","/?"
};

/*** Russian keyboard layout (phantom key version) */
static const char main_key_RU_phantom[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ\xca\xea","wW\xc3\xe3","eE\xd5\xf5","rR\xcb\xeb","tT\xc5\xe5","yY\xce\xee","uU\xc7\xe7","iI\xdb\xfb","oO\xdd\xfd","pP\xda\xfa","[{\xc8\xe8","]}\xdf\xff",
 "aA\xc6\xe6","sS\xd9\xf9","dD\xd7\xf7","fF\xc1\xe1","gG\xd0\xf0","hH\xd2\xf2","jJ\xcf\xef","kK\xcc\xec","lL\xc4\xe4",";:\xd6\xf6","'\"\xdc\xfc","\\|",
 "zZ\xd1\xf1","xX\xde\xfe","cC\xd3\xf3","vV\xcd\xed","bB\xc9\xe9","nN\xd4\xf4","mM\xd8\xf8",",<\xc2\xe2",".>\xc0\xe0","/?",
 "<>" /* the phantom key */
};

/*** Russian keyboard layout KOI8-R */
static const char main_key_RU_koi8r[MAIN_LEN][4] =
{
 "()","1!","2\"","3/","4$","5:","6,","7.","8;","9?","0%","-_","=+",
 "\xca\xea","\xc3\xe3","\xd5\xf5","\xcb\xeb","\xc5\xe5","\xce\xee","\xc7\xe7","\xdb\xfb","\xdd\xfd","\xda\xfa","\xc8\xe8","\xdf\xff",
 "\xc6\xe6","\xd9\xf9","\xd7\xf7","\xc1\xe1","\xd0\xf0","\xd2\xf2","\xcf\xef","\xcc\xec","\xc4\xe4","\xd6\xf6","\xdc\xfc","\\|",
 "\xd1\xf1","\xde\xfe","\xd3\xf3","\xcd\xed","\xc9\xe9","\xd4\xf4","\xd8\xf8","\xc2\xe2","\xc0\xe0","/?",
 "<>" /* the phantom key */
};

/*** Russian keyboard layout cp1251 */
static const char main_key_RU_cp1251[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ\xe9\xc9","wW\xf6\xd6","eE\xf3\xd3","rR\xea\xca","tT\xe5\xc5","yY\xed\xcd","uU\xe3\xc3","iI\xf8\xd8","oO\xf9\xd9","pP\xe7\xc7","[{\xf5\xd5","]}\xfa\xda",
 "aA\xf4\xd4","sS\xfb\xdb","dD\xe2\xc2","fF\xe0\xc0","gG\xef\xcf","hH\xf0\xd0","jJ\xee\xce","kK\xeb\xcb","lL\xe4\xc4",";:\xe6\xc6","'\"\xfd\xdd","\\|",
 "zZ\xff\xdf","xX\xf7\xd7","cC\xf1\xd1","vV\xec\xcc","bB\xe8\xc8","nN\xf2\xd2","mM\xfc\xdc",",<\xe1\xc1",".>\xfe\xde","/?",
 "<>" /* the phantom key */
};

/*** Russian phonetic keyboard layout */
static const char main_key_RU_phonetic[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ\xd1\xf1","wW\xd7\xf7","eE\xc5\xe5","rR\xd2\xf2","tT\xd4\xf4","yY\xd9\xf9","uU\xd5\xf5","iI\xc9\xe9","oO\xcf\xef","pP\xd0\xf0","[{\xdb\xfb","]}\xdd\xfd",
 "aA\xc1\xe1","sS\xd3\xf3","dD\xc4\xe4","fF\xc6\xe6","gG\xc7\xe7","hH\xc8\xe8","jJ\xca\xea","kK\xcb\xeb","lL\xcc\xec",";:","'\"","\\|",
 "zZ\xda\xfa","xX\xd8\xf8","cC\xc3\xe3","vV\xd6\xf6","bB\xc2\xe2","nN\xce\xee","mM\xcd\xed",",<",".>","/?",
 "<>" /* the phantom key */
};

/*** Ukrainian keyboard layout KOI8-U */
static const char main_key_UA[MAIN_LEN][4] =
{
 "`~\xad\xbd","1!1!","2@2\"","3#3'","4$4*","5%5:","6^6,","7&7.","8*8;","9(9(","0)0)","-_-_","=+=+",
 "qQ\xca\xea","wW\xc3\xe3","eE\xd5\xf5","rR\xcb\xeb","tT\xc5\xe5","yY\xce\xee","uU\xc7\xe7","iI\xdb\xfb","oO\xdd\xfd","pP\xda\xfa","[{\xc8\xe8","]}\xa7\xb7",
 "aA\xc6\xe6","sS\xa6\xb6","dD\xd7\xf7","fF\xc1\xe1","gG\xd0\xf0","hH\xd2\xf2","jJ\xcf\xef","kK\xcc\xec","lL\xc4\xe4",";:\xd6\xf6","'\"\xa4\xb4","\\|\\|",
 "zZ\xd1\xf1","xX\xde\xfe","cC\xd3\xf3","vV\xcd\xed","bB\xc9\xe9","nN\xd4\xf4","mM\xd8\xf8",",<\xc2\xe2",".>\xc0\xe0","/?/?",
 "<>" /* the phantom key */
};

/*** Ukrainian keyboard layout KOI8-U by O. Nykyforchyn */
/***  (as it appears on most of keyboards sold today)   */
static const char main_key_UA_std[MAIN_LEN][4] =
{
 "\xad\xbd","1!","2\"","3'","4;","5%","6:","7?","8*","9(","0)","-_","=+",
 "\xca\xea","\xc3\xe3","\xd5\xf5","\xcb\xeb","\xc5\xe5","\xce\xee","\xc7\xe7","\xdb\xfb","\xdd\xfd","\xda\xfa","\xc8\xe8","\xa7\xb7",
 "\xc6\xe6","\xa6\xb6","\xd7\xf7","\xc1\xe1","\xd0\xf0","\xd2\xf2","\xcf\xef","\xcc\xec","\xc4\xe4","\xd6\xf6","\xa4\xb4","\\/",
 "\xd1\xf1","\xde\xfe","\xd3\xf3","\xcd\xed","\xc9\xe9","\xd4\xf4","\xd8\xf8","\xc2\xe2","\xc0\xe0",".,",
 "<>" /* the phantom key */
};

/*** Russian keyboard layout KOI8-R (pair to the previous) */
static const char main_key_RU_std[MAIN_LEN][4] =
{
 "\xa3\xb3","1!","2\"","3'","4;","5%","6:","7?","8*","9(","0)","-_","=+",
 "\xca\xea","\xc3\xe3","\xd5\xf5","\xcb\xeb","\xc5\xe5","\xce\xee","\xc7\xe7","\xdb\xfb","\xdd\xfd","\xda\xfa","\xc8\xe8","\xdf\xff",
 "\xc6\xe6","\xd9\xf9","\xd7\xf7","\xc1\xe1","\xd0\xf0","\xd2\xf2","\xcf\xef","\xcc\xec","\xc4\xe4","\xd6\xf6","\xdc\xfc","\\/",
 "\xd1\xf1","\xde\xfe","\xd3\xf3","\xcd\xed","\xc9\xe9","\xd4\xf4","\xd8\xf8","\xc2\xe2","\xc0\xe0",".,",
 "<>" /* the phantom key */
};

/*** Spanish keyboard layout (setxkbmap es) */
static const char main_key_ES[MAIN_LEN][4] =
{
 "\xba\xaa","1!","2\"","3\xb7","4$","5%","6&","7/","8(","9)","0=","'?","\xa1\xbf",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","`^","+*",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xf1\xd1","\xb4\xa8","\xe7\xc7",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Belgian keyboard layout ***/
static const char main_key_BE[MAIN_LEN][4] =
{
 "","&1|","\xe9""2@","\"3#","'4","(5","\xa7""6^","\xe8""7","!8","\xe7""9{","\xe0""0}",")\xb0","-_",
 "aA","zZ","eE\xa4","rR","tT","yY","uU","iI","oO","pP","^\xa8[","$*]",
 "qQ","sS\xdf","dD","fF","gG","hH","jJ","kK","lL","mM","\xf9%\xb4","\xb5\xa3`",
 "wW","xX","cC","vV","bB","nN",",?",";.",":/","=+~",
 "<>\\"
};

/*** Hungarian keyboard layout (setxkbmap hu) */
static const char main_key_HU[MAIN_LEN][4] =
{
 "0\xa7","1'","2\"","3+","4!","5%","6/","7=","8(","9)","\xf6\xd6","\xfc\xdc","\xf3\xd3",
 "qQ","wW","eE","rR","tT","zZ","uU","iI","oO","pP","\xf5\xd5","\xfa\xda",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xe9\xc9","\xe1\xc1","\xfb\xdb",
 "yY","xX","cC","vV","bB","nN","mM",",?",".:","-_",
 "\xed\xcd"
};

/*** Polish (programmer's) keyboard layout ***/
static const char main_key_PL[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&\xa7","8*","9(","0)","-_","=+",
 "qQ","wW","eE\xea\xca","rR","tT","yY","uU","iI","oO\xf3\xd3","pP","[{","]}",
 "aA\xb1\xa1","sS\xb6\xa6","dD","fF","gG","hH","jJ","kK","lL\xb3\xa3",";:","'\"","\\|",
 "zZ\xbf\xaf","xX\xbc\xac","cC\xe6\xc6","vV","bB","nN\xf1\xd1","mM",",<",".>","/?",
 "<>|"
};

/*** Slovenian keyboard layout (setxkbmap si) ***/
static const char main_key_SI[MAIN_LEN][4] =
{
 "\xb8\xa8","1!","2\"","3#","4$","5%","6&","7/","8(","9)","0=","'?","+*",
 "qQ","wW","eE","rR","tT","zZ","uU","iI","oO","pP","\xb9\xa9","\xf0\xd0",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xe8\xc8","\xe6\xc6","\xbe\xae",
 "yY","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Serbian keyboard layout (setxkbmap sr) ***/
static const char main_key_SR[MAIN_LEN][4] =
{
 "`~","1!","2\"","3#","4$","5%","6&","7/","8(","9)","0=","'?","+*",
 "\xa9\xb9","\xaa\xba","\xc5\xe5","\xd2\xf2","\xd4\xf4","\xda\xfa","\xd5\xf5","\xc9\xe9","\xcf\xef","\xd0\xf0","\xdb\xfb","[]",
 "\xc1\xe1","\xd3\xf3","\xc4\xe4","\xc6\xe6","\xc7\xe7","\xc8\xe8","\xa8\xb8","\xcb\xeb","\xcc\xec","\xde\xfe","\xab\xbb","-_",
 "\xa1\xb1","\xaf\xbf","\xc3\xe3","\xd7\xf7","\xc2\xe2","\xce\xee","\xcd\xed",",;",".:","\xd6\xf6",
 "<>" /* the phantom key */
};

/*** Serbian keyboard layout (setxkbmap us,sr) ***/
static const char main_key_US_SR[MAIN_LEN][4] =
{
 "`~","1!","2@2\"","3#","4$","5%","6^6&","7&7/","8*8(","9(9)","0)0=","-_'?","=++*",
 "qQ\xa9\xb9","wW\xaa\xba","eE\xc5\xe5","rR\xd2\xf2","tT\xd4\xf4","yY\xda\xfa","uU\xd5\xf5","iI\xc9\xe9","oO\xcf\xef","pP\xd0\xf0","[{\xdb\xfb","]}[]",
 "aA\xc1\xe1","sS\xd3\xf3","dD\xc4\xe4","fF\xc6\xe6","gG\xc7\xe7","hH\xc8\xe8","jJ\xa8\xb8","kK\xcb\xeb","lL\xcc\xec",";:\xde\xfe","'\"\xab\xbb","\\|-_",
 "zZ\xa1\xb1","xX\xaf\xbf","cC\xc3\xe3","vV\xd7\xf7","bB\xc2\xe2","nN\xce\xee","mM\xcd\xed",",<,;",".>.:","/?\xd6\xf6",
 "<>" /* the phantom key */
};

/*** Croatian keyboard layout specific for me <jelly@srk.fer.hr> ***/
static const char main_key_HR_jelly[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","[{\xb9\xa9","]}\xf0\xd0",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";:\xe8\xc8","'\"\xe6\xc6","\\|\xbe\xae",
 "zZ","xX","cC","vV","bB","nN","mM",",<",".>","/?",
 "<>|"
};

/*** Croatian keyboard layout (setxkbmap hr) ***/
static const char main_key_HR[MAIN_LEN][4] =
{
 "\xb8\xa8","1!","2\"","3#","4$","5%","6&","7/","8(","9)","0=","'?","+*",
 "qQ","wW","eE","rR","tT","zZ","uU","iI","oO","pP","\xb9\xa9","\xf0\xd0",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xe8\xc8","\xe6\xc6","\xbe\xae",
 "yY","xX","cC","vV","bB","nN","mM",",;",".:","/?",
 "<>"
};

/*** Japanese 106 keyboard layout ***/
static const char main_key_JA_jp106[MAIN_LEN][4] =
{
 "1!","2\"","3#","4$","5%","6&","7'","8(","9)","0~","-=","^~","\\|",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","@`","[{",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";+",":*","]}",
 "zZ","xX","cC","vV","bB","nN","mM",",<",".>","/?",
 "\\_",
};

static const char main_key_JA_macjp[MAIN_LEN][4] =
{
 "1!","2\"","3#","4$","5%","6&","7'","8(","9)","0","-=","^~","\\|",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","@`","[{",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";+",":*","]}",
 "zZ","xX","cC","vV","bB","nN","mM",",<",".>","/?",
 "__",
};

/*** Japanese pc98x1 keyboard layout ***/
static const char main_key_JA_pc98x1[MAIN_LEN][4] =
{
 "1!","2\"","3#","4$","5%","6&","7'","8(","9)","0","-=","^`","\\|",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","@~","[{",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";+",":*","]}",
 "zZ","xX","cC","vV","bB","nN","mM",",<",".>","/?",
 "\\_",
};

/*** Brazilian ABNT-2 keyboard layout (contributed by Raul Gomes Fernandes) */
static const char main_key_PT_br[MAIN_LEN][4] =
{
 "'\"","1!","2@","3#","4$","5%","6\xa8","7&","8*","9(","0)","-_","=+",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","\xb4`","[{",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xe7\xc7","~^","]}",
 "\\|","zZ","xX","cC","vV","bB","nN","mM",",<",".>",";:","/?",
};

/*** Brazilian ABNT-2 keyboard layout with <ALT GR> (contributed by Mauro Carvalho Chehab) */
static const char main_key_PT_br_alt_gr[MAIN_LEN][4] =
{
 "'\"","1!9","2@2","3#3","4$#","5%\"","6(,","7&","8*","9(","0)","-_","=+'",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","4`","[{*",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","gG","~^","]}:",
 "\\|","zZ","xX","cC","vV","bB","nN","mM",",<",".>",";:","/?0"
};

/*** US international keyboard layout (contributed by Gustavo Noronha (kov@debian.org)) */
static const char main_key_US_intl[MAIN_LEN][4] =
{
  "`~", "1!", "2@", "3#", "4$", "5%", "6^", "7&", "8*", "9(", "0)", "-_", "=+", "\\|",
  "qQ", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "[{", "]}",
  "aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", ";:", "'\"",
  "zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",<", ".>", "/?"
};

/*** Slovak keyboard layout (see cssk_ibm(sk_qwerty) in xkbsel)
  - dead_abovering replaced with degree - no symbol in iso8859-2
  - brokenbar replaced with bar					*/
static const char main_key_SK[MAIN_LEN][4] =
{
 ";0","+1","\xb5""2","\xb9""3","\xe8""4","\xbb""5","\xbe""6","\xfd""7","\xe1""8","\xed""9","\xe9""0","=%","'v",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","\xfa/","\xe4(",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xf4\"","\xa7!","\xf2)",
 "zZ","xX","cC","vV","bB","nN","mM",",?",".:","-_",
 "<>"
};

/*** Czech keyboard layout (setxkbmap cz) */
static const char main_key_CZ[MAIN_LEN][4] =
{
 ";","+1","\xec""2","\xb9""3","\xe8""4","\xf8""5","\xbe""6","\xfd""7","\xe1""8","\xed""9","\xe9""0","=%","\xb4\xb7",
 "qQ","wW","eE","rR","tT","zZ","uU","iI","oO","pP","\xfa/",")(",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xf9\"","\xa7!","\xa8'",
 "yY","xX","cC","vV","bB","nN","mM",",?",".:","-_",
 "\\"
};

/*** Czech keyboard layout (setxkbmap cz_qwerty) */
static const char main_key_CZ_qwerty[MAIN_LEN][4] =
{
 ";","+1","\xec""2","\xb9""3","\xe8""4","\xf8""5","\xbe""6","\xfd""7","\xe1""8","\xed""9","\xe9""0","=%","\xb4\xb7",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","\xfa/",")(",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xf9\"","\xa7!","\xa8'",
 "zZ","xX","cC","vV","bB","nN","mM",",?",".:","-_",
 "\\"
};

/*** Slovak and Czech (programmer's) keyboard layout (see cssk_dual(cs_sk_ucw)) */
static const char main_key_SK_prog[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ\xe4\xc4","wW\xec\xcc","eE\xe9\xc9","rR\xf8\xd8","tT\xbb\xab","yY\xfd\xdd","uU\xf9\xd9","iI\xed\xcd","oO\xf3\xd3","pP\xf6\xd6","[{","]}",
 "aA\xe1\xc1","sS\xb9\xa9","dD\xef\xcf","fF\xeb\xcb","gG\xe0\xc0","hH\xfa\xda","jJ\xfc\xdc","kK\xf4\xd4","lL\xb5\xa5",";:","'\"","\\|",
 "zZ\xbe\xae","xX\xa4","cC\xe8\xc8","vV\xe7\xc7","bB","nN\xf2\xd2","mM\xe5\xc5",",<",".>","/?",
 "<>"
};

/*** Czech keyboard layout (see cssk_ibm(cs_qwerty) in xkbsel) */
static const char main_key_CS[MAIN_LEN][4] =
{
 ";","+1","\xec""2","\xb9""3","\xe8""4","\xf8""5","\xbe""6","\xfd""7","\xe1""8","\xed""9","\xe9""0\xbd)","=%","",
 "qQ\\","wW|","eE","rR","tT","yY","uU","iI","oO","pP","\xfa/[{",")(]}",
 "aA","sS\xf0","dD\xd0","fF[","gG]","hH","jJ","kK\xb3","lL\xa3","\xf9\"$","\xa7!\xdf","\xa8'",
 "zZ>","xX#","cC&","vV@","bB{","nN}","mM",",?<",".:>","-_*",
 "<>\\|"
};

/*** Latin American keyboard layout (contributed by Gabriel Orlando Garcia) */
static const char main_key_LA[MAIN_LEN][4] =
{
 "|\xb0","1!","2\"","3#","4$","5%","6&","7/","8(","9)","0=","'?","\xbf\xa1",
 "qQ@","wW","eE","rR","tT","yY","uU","iI","oO","pP","\xb4\xa8","+*",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xf1\xd1","{[^","}]",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Lithuanian keyboard layout (setxkbmap lt) */
static const char main_key_LT_B[MAIN_LEN][4] =
{
 "`~","\xe0\xc0","\xe8\xc8","\xe6\xc6","\xeb\xcb","\xe1\xc1","\xf0\xd0","\xf8\xd8","\xfb\xdb","\xa5(","\xb4)","-_","\xfe\xde",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","[{","]}",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";:","'\"","\\|",
 "zZ","xX","cC","vV","bB","nN","mM",",<",".>","/?",
 "\xaa\xac"
};

/*** Turkish keyboard Layout */
static const char main_key_TK[MAIN_LEN][4] =
{
"\"\xe9","1!","2'","3^#","4+$","5%","6&","7/{","8([","9)]","0=}","*?\\","-_",
"qQ@","wW","eE","rR","tT","yY","uU","\xfdI\xee","oO","pP","\xf0\xd0","\xfc\xdc~",
"aA\xe6","sS\xdf","dD","fF","gG","hH","jJ","kK","lL","\xfe\xde","i\xdd",",;`",
"zZ","xX","cC","vV","bB","nN","mM","\xf6\xd6","\xe7\xc7",".:"
};

/*** Turkish keyboard layout (setxkbmap tr) */
static const char main_key_TR[MAIN_LEN][4] =
{
"\"\\","1!","2'","3^","4+","5%","6&","7/","8(","9)","0=","*?","-_",
"qQ","wW","eE","rR","tT","yY","uU","\xb9I","oO","pP","\xbb\xab","\xfc\xdc",
"aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xba\xaa","i\0",",;",
"zZ","xX","cC","vV","bB","nN","mM","\xf6\xd6","\xe7\xc7",".:",
"<>"
};

/*** Turkish F keyboard layout (setxkbmap trf) */
static const char main_key_TR_F[MAIN_LEN][4] =
{
"+*","1!","2\"","3^#","4$","5%","6&","7'","8(","9)","0=","/?","-_",
"fF","gG","\xbb\xab","\xb9I","oO","dD","rR","nN","hH","pP","qQ","wW",
"uU","i\0","eE","aA","\xfc\xdc","tT","kK","mM","lL","yY","\xba\xaa","xX",
"jJ","\xf6\xd6","vV","cC","\xe7\xc7","zZ","sS","bB",".:",",;",
"<>"
};

/*** Israelian keyboard layout (setxkbmap us,il) */
static const char main_key_IL[MAIN_LEN][4] =
{
 "`~;","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ/","wW'","eE\xf7","rR\xf8","tT\xe0","yY\xe8","uU\xe5","iI\xef","oO\xed","pP\xf4","[{","]}",
 "aA\xf9","sS\xe3","dD\xe2","fF\xeb","gG\xf2","hH\xe9","jJ\xe7","kK\xec","lL\xea",";:\xf3","\'\",","\\|",
 "zZ\xe6","xX\xf1","cC\xe1","vV\xe4","bB\xf0","nN\xee","mM\xf6",",<\xfa",".>\xf5","/?.",
 "<>"
};

/*** Israelian phonetic keyboard layout (setxkbmap us,il_phonetic) */
static const char main_key_IL_phonetic[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ\xf7","wW\xe5","eE\xe0","rR\xf8","tT\xfa","yY\xf2","uU\xe5","iI\xe9","oO\xf1","pP\xf4","[{","]}",
 "aA\xe0","sS\xf9","dD\xe3","fF\xf4","gG\xe2","hH\xe4","jJ\xe9","kK\xeb","lL\xec",";:","'\"","\\|",
 "zZ\xe6","xX\xe7","cC\xf6","vV\xe5","bB\xe1","nN\xf0","mM\xee",",<",".>","/?",
 "<>"
};

/*** Israelian Saharon keyboard layout (setxkbmap -symbols "us(pc105)+il_saharon") */
static const char main_key_IL_saharon[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ\xf7","wW\xf1","eE","rR\xf8","tT\xe8","yY\xe3","uU","iI","oO","pP\xf4","[{","]}",
 "aA\xe0","sS\xe5","dD\xec","fF\xfa","gG\xe2","hH\xe4","jJ\xf9","kK\xeb","lL\xe9",";:","'\"","\\|",
 "zZ\xe6","xX\xe7","cC\xf6","vV\xf2","bB\xe1","nN\xf0","mM\xee",",<",".>","/?",
 "<>"
};

/*** Greek keyboard layout (contributed by Kriton Kyrimis <kyrimis@cti.gr>)
  Greek characters for "wW" and "sS" are omitted to not produce a mismatch
  message since they have different characters in gr and el XFree86 layouts. */
static const char main_key_EL[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ;:","wW","eE\xe5\xc5","rR\xf1\xd1","tT\xf4\xd4","yY\xf5\xd5","uU\xe8\xc8","iI\xe9\xc9","oO\xef\xcf","pP\xf0\xd0","[{","]}",
 "aA\xe1\xc1","sS","dD\xe4\xc4","fF\xf6\xd6","gG\xe3\xc3","hH\xe7\xc7","jJ\xee\xce","kK\xea\xca","lL\xeb\xcb",";:\xb4\xa8","'\"","\\|",
 "zZ\xe6\xc6","xX\xf7\xd7","cC\xf8\xd8","vV\xf9\xd9","bB\xe2\xc2","nN\xed\xcd","mM\xec\xcc",",<",".>","/?",
 "<>"
};

/*** Thai (Kedmanee) keyboard layout by Supphachoke Suntiwichaya <mrchoke@opentle.org> */
static const char main_key_th[MAIN_LEN][4] =
{
 "`~_%","1!\xe5+","2@/\xf1","3#-\xf2","4$\xc0\xf3","5%\xb6\xf4","6^\xd8\xd9","7&\xd6\xdf","8*\xa4\xf5","9(\xb5\xf6","0)\xa8\xf7","-_\xa2\xf8","=+\xaa\xf9",
 "qQ\xe6\xf0","wW\xe4\"","eE\xd3\xae","rR\xbe\xb1","tT\xd0\xb8","yY\xd1\xed","uU\xd5\xea","iI\xc3\xb3","oO\xb9\xcf","pP\xc2\xad","[{\xba\xb0","]}\xc5,",
 "aA\xbf\xc4","sS\xcb\xa6","dD\xa1\xaf","fF\xb4\xe2","gG\xe0\xac","hH\xe9\xe7","jJ\xe8\xeb","kK\xd2\xc9","lL\xca\xc8",";:\xc7\xab","\'\"\xa7.","\\|\xa3\xa5",
 "zZ\xbc(","xX\xbb)","cC\xe1\xa9","vV\xcd\xce","bB\xda","nN\xd7\xec","mM\xb7?",",<\xc1\xb2",".>\xe3\xcc","/?\xbd\xc6"
}; 

/*** VNC keyboard layout */
static const WORD main_key_scan_vnc[MAIN_LEN] =
{
   0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x1A,0x1B,0x27,0x28,0x29,0x33,0x34,0x35,0x2B,
   0x1E,0x30,0x2E,0x20,0x12,0x21,0x22,0x23,0x17,0x24,0x25,0x26,0x32,0x31,0x18,0x19,0x10,0x13,0x1F,0x14,0x16,0x2F,0x11,0x2D,0x15,0x2C,
   0x56
};

static const WORD main_key_vkey_vnc[MAIN_LEN] =
{
   '1','2','3','4','5','6','7','8','9','0',VK_OEM_MINUS,VK_OEM_PLUS,VK_OEM_4,VK_OEM_6,VK_OEM_1,VK_OEM_7,VK_OEM_3,VK_OEM_COMMA,VK_OEM_PERIOD,VK_OEM_2,VK_OEM_5,
   'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
   VK_OEM_102
};

static const char main_key_vnc[MAIN_LEN][4] =
{
 "1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+","[{","]}",";:","'\"","`~",",<",".>","/?","\\|",
 "aA","bB","cC","dD","eE","fF","gG","hH","iI","jJ","kK","lL","mM","nN","oO","pP","qQ","rR","sS","tT","uU","vV","wW","xX","yY","zZ"
};

/*** Dutch keyboard layout (setxkbmap nl) ***/
static const char main_key_NL[MAIN_LEN][4] =
{
 "@\xa7","1!","2\"","3#","4$","5%","6&","7_","8(","9)","0'","/?","\xb0~",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","\xa8~","*|",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","+\xb1","'`","<>",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-=",
 "[]"
};



/*** Layout table. Add your keyboard mappings to this list */
static const struct {
    LCID lcid; /* input locale identifier, look for LOCALE_ILANGUAGE
                 in the appropriate dlls/kernel/nls/.nls file */
    const char *comment;
    const char (*key)[MAIN_LEN][4];
    const WORD (*scan)[MAIN_LEN]; /* scan codes mapping */
    const WORD (*vkey)[MAIN_LEN]; /* virtual key codes mapping */
} main_key_tab[]={
 {0x0409, "United States keyboard layout", &main_key_US, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0409, "United States keyboard layout (phantom key version)", &main_key_US_phantom, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 /* Dvorak users tend to run QWERTY keyboards and rely on Windows/X11/Wayland to translate to the correct keysyms */
 {0x0409, "United States keyboard layout (dvorak)", &main_key_US_dvorak, &main_key_scan_qwerty, &main_key_vkey_dvorak},
 {0x0409, "United States keyboard layout (dvorak with phantom key)", &main_key_US_dvorak_phantom, &main_key_scan_qwerty, &main_key_vkey_dvorak},
 {0x0409, "United States International keyboard layout", &main_key_US_intl, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0809, "British keyboard layout", &main_key_UK, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0407, "German keyboard layout", &main_key_DE, &main_key_scan_qwerty, &main_key_vkey_qwertz},
 {0x0807, "Swiss German keyboard layout", &main_key_SG, &main_key_scan_qwerty, &main_key_vkey_qwertz},
 {0x100c, "Swiss French keyboard layout", &main_key_SF, &main_key_scan_qwerty, &main_key_vkey_qwertz},
 {0x041d, "Swedish keyboard layout", &main_key_SE, &main_key_scan_qwerty, &main_key_vkey_qwerty_v2},
 {0x0425, "Estonian keyboard layout", &main_key_ET, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0414, "Norwegian keyboard layout", &main_key_NO, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0406, "Danish keyboard layout", &main_key_DA, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x040c, "French keyboard layout", &main_key_FR, &main_key_scan_qwerty, &main_key_vkey_azerty},
 {0x0c0c, "Canadian French keyboard layout", &main_key_CF, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0c0c, "Canadian French keyboard layout (CA_fr)", &main_key_CA_fr, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0c0c, "Canadian keyboard layout", &main_key_CA, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x080c, "Belgian keyboard layout", &main_key_BE, &main_key_scan_qwerty, &main_key_vkey_azerty},
 {0x0816, "Portuguese keyboard layout", &main_key_PT, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0416, "Brazilian ABNT-2 keyboard layout", &main_key_PT_br, &main_key_scan_abnt_qwerty, &main_key_vkey_abnt_qwerty},
 {0x0416, "Brazilian ABNT-2 keyboard layout ALT GR", &main_key_PT_br_alt_gr,&main_key_scan_abnt_qwerty, &main_key_vkey_abnt_qwerty},
 {0x040b, "Finnish keyboard layout", &main_key_FI, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0402, "Bulgarian bds keyboard layout", &main_key_BG_bds, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0402, "Bulgarian phonetic keyboard layout", &main_key_BG_phonetic, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0423, "Belarusian keyboard layout", &main_key_BY, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0419, "Russian keyboard layout", &main_key_RU, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0419, "Russian keyboard layout (phantom key version)", &main_key_RU_phantom, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0419, "Russian keyboard layout KOI8-R", &main_key_RU_koi8r, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0419, "Russian keyboard layout cp1251", &main_key_RU_cp1251, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0419, "Russian phonetic keyboard layout", &main_key_RU_phonetic, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0422, "Ukrainian keyboard layout KOI8-U", &main_key_UA, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0422, "Ukrainian keyboard layout (standard)", &main_key_UA_std, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0419, "Russian keyboard layout (standard)", &main_key_RU_std, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x040a, "Spanish keyboard layout", &main_key_ES, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0410, "Italian keyboard layout", &main_key_IT, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x040f, "Icelandic keyboard layout", &main_key_IS, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x040e, "Hungarian keyboard layout", &main_key_HU, &main_key_scan_qwerty, &main_key_vkey_qwertz},
 {0x0415, "Polish (programmer's) keyboard layout", &main_key_PL, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0424, "Slovenian keyboard layout", &main_key_SI, &main_key_scan_qwerty, &main_key_vkey_qwertz},
 {0x0c1a, "Serbian keyboard layout sr", &main_key_SR, &main_key_scan_qwerty, &main_key_vkey_qwerty}, /* LANG_SERBIAN,SUBLANG_SERBIAN_CYRILLIC */
 {0x0c1a, "Serbian keyboard layout us,sr", &main_key_US_SR, &main_key_scan_qwerty, &main_key_vkey_qwerty}, /* LANG_SERBIAN,SUBLANG_SERBIAN_CYRILLIC */
 {0x041a, "Croatian keyboard layout", &main_key_HR, &main_key_scan_qwerty, &main_key_vkey_qwertz},
 {0x041a, "Croatian keyboard layout (specific)", &main_key_HR_jelly, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0411, "Japanese 106 keyboard layout", &main_key_JA_jp106, &main_key_scan_qwerty_jp106, &main_key_vkey_qwerty_jp106},
 {0x0411, "Japanese Mac keyboard layout", &main_key_JA_macjp, &main_key_scan_qwerty_jp106, &main_key_vkey_qwerty_jp106},
 {0x0411, "Japanese pc98x1 keyboard layout", &main_key_JA_pc98x1, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x041b, "Slovak keyboard layout", &main_key_SK, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x041b, "Slovak and Czech keyboard layout without dead keys", &main_key_SK_prog, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0405, "Czech keyboard layout", &main_key_CS, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0405, "Czech keyboard layout cz", &main_key_CZ, &main_key_scan_qwerty, &main_key_vkey_qwertz},
 {0x0405, "Czech keyboard layout cz_qwerty", &main_key_CZ_qwerty, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x040a, "Latin American keyboard layout", &main_key_LA, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0427, "Lithuanian (Baltic) keyboard layout", &main_key_LT_B, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x041f, "Turkish keyboard layout", &main_key_TK, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x041f, "Turkish keyboard layout tr", &main_key_TR, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x041f, "Turkish keyboard layout trf", &main_key_TR_F, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x040d, "Israelian keyboard layout", &main_key_IL, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x040d, "Israelian phonetic keyboard layout", &main_key_IL_phonetic, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x040d, "Israelian Saharon keyboard layout", &main_key_IL_saharon, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0409, "VNC keyboard layout", &main_key_vnc, &main_key_scan_vnc, &main_key_vkey_vnc},
 {0x0408, "Greek keyboard layout", &main_key_EL, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x041e, "Thai (Kedmanee)  keyboard layout", &main_key_th, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0413, "Dutch keyboard layout", &main_key_NL, &main_key_scan_qwerty, &main_key_vkey_qwerty},

 {0, NULL, NULL, NULL, NULL} /* sentinel */
};
static unsigned kbd_layout=0; /* index into above table of layouts */

/* maybe more of these scancodes should be extended? */
                /* extended must be set for ALT_R, CTRL_R,
                   INS, DEL, HOME, END, PAGE_UP, PAGE_DOWN, ARROW keys,
                   keypad / and keypad ENTER (SDK 3.1 Vol.3 p 138) */
                /* FIXME should we set extended bit for NumLock ? My
                 * Windows does ... DF */
                /* Yes, to distinguish based on scan codes, also
                   for PrtScn key ... GA */

static const WORD nonchar_key_vkey[256] =
{
    /* unused */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF00 */
    /* special keys */
    VK_BACK, VK_TAB, 0, VK_CLEAR, 0, VK_RETURN, 0, 0,           /* FF08 */
    0, 0, 0, VK_PAUSE, VK_SCROLL, 0, 0, 0,                      /* FF10 */
    0, 0, 0, VK_ESCAPE, 0, 0, 0, 0,                             /* FF18 */
    /* Japanese special keys */
    0, VK_KANJI, VK_NONCONVERT, VK_CONVERT,                     /* FF20 */
    VK_DBE_ROMAN, 0, 0, VK_DBE_HIRAGANA,
    0, 0, VK_DBE_SBCSCHAR, 0, 0, 0, 0, 0,                       /* FF28 */
    /* Korean special keys (FF31-) */
    VK_DBE_ALPHANUMERIC, VK_HANGUL, 0, 0, VK_HANJA, 0, 0, 0,    /* FF30 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF38 */
    /* unused */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF40 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF48 */
    /* cursor keys */
    VK_HOME, VK_LEFT, VK_UP, VK_RIGHT,                          /* FF50 */
    VK_DOWN, VK_PRIOR, VK_NEXT, VK_END,
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF58 */
    /* misc keys */
    VK_SELECT, VK_SNAPSHOT, VK_EXECUTE, VK_INSERT, 0,0,0, VK_APPS, /* FF60 */
    0, VK_CANCEL, VK_HELP, VK_CANCEL, 0, 0, 0, 0,               /* FF68 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF70 */
    /* keypad keys */
    0, 0, 0, 0, 0, 0, 0, VK_NUMLOCK,                            /* FF78 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF80 */
    0, 0, 0, 0, 0, VK_RETURN, 0, 0,                             /* FF88 */
    0, 0, 0, 0, 0, VK_HOME, VK_LEFT, VK_UP,                     /* FF90 */
    VK_RIGHT, VK_DOWN, VK_PRIOR, VK_NEXT,                       /* FF98 */
    VK_END, VK_CLEAR, VK_INSERT, VK_DELETE,
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FFA0 */
    0, 0, VK_MULTIPLY, VK_ADD,                                  /* FFA8 */
    /* Windows always generates VK_DECIMAL for Del/. on keypad while some
     * X11 keyboard layouts generate XK_KP_Separator instead of XK_KP_Decimal
     * in order to produce a locale dependent numeric separator.
     */
    VK_DECIMAL, VK_SUBTRACT, VK_DECIMAL, VK_DIVIDE,
    VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3,             /* FFB0 */
    VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7,
    VK_NUMPAD8, VK_NUMPAD9, 0, 0, 0, VK_OEM_NEC_EQUAL,          /* FFB8 */
    /* function keys */
    VK_F1, VK_F2,
    VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10,    /* FFC0 */
    VK_F11, VK_F12, VK_F13, VK_F14, VK_F15, VK_F16, VK_F17, VK_F18, /* FFC8 */
    VK_F19, VK_F20, VK_F21, VK_F22, VK_F23, VK_F24, 0, 0,       /* FFD0 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FFD8 */
    /* modifier keys */
    0, VK_LSHIFT, VK_RSHIFT, VK_LCONTROL,                       /* FFE0 */
    VK_RCONTROL, VK_CAPITAL, 0, VK_LMENU,
    VK_RMENU, VK_LMENU, VK_RMENU, VK_LWIN, VK_RWIN, 0, 0, 0,    /* FFE8 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FFF0 */
    0, 0, 0, 0, 0, 0, 0, VK_DELETE                              /* FFF8 */
};

static const WORD nonchar_key_scan[256] =
{
    /* unused */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF00 */
    /* special keys */
    0x0E, 0x0F, 0x00, /*?*/ 0, 0x00, 0x1C, 0x00, 0x00,           /* FF08 */
    0x00, 0x00, 0x00, 0x45, 0x46, 0x00, 0x00, 0x00,              /* FF10 */
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,              /* FF18 */
    /* Japanese special keys */
    0x00, 0x29, 0x7B, 0x79, 0x70, 0x00, 0x00, 0x70,              /* FF20 */
    0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF28 */
    /* Korean special keys (FF31-) */
    0x3A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF30 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF38 */
    /* unused */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF40 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF48 */
    /* cursor keys */
    0x147, 0x14B, 0x148, 0x14D, 0x150, 0x149, 0x151, 0x14F,      /* FF50 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF58 */
    /* misc keys */
    /*?*/ 0, 0x137, /*?*/ 0, 0x152, 0x00, 0x00, 0x00, 0x15D,     /* FF60 */
    /*?*/ 0, /*?*/ 0, 0x38, 0x146, 0x00, 0x00, 0x00, 0x00,       /* FF68 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF70 */
    /* keypad keys */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x145,             /* FF78 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF80 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x11C, 0x00, 0x00,             /* FF88 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x4B, 0x48,              /* FF90 */
    0x4D, 0x50, 0x49, 0x51, 0x4F, 0x4C, 0x52, 0x53,              /* FF98 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FFA0 */
    0x00, 0x00, 0x37, 0x4E, 0x53, 0x4A, 0x53, 0x135,             /* FFA8 */
    0x52, 0x4F, 0x50, 0x51, 0x4B, 0x4C, 0x4D, 0x47,              /* FFB0 */
    0x48, 0x49, 0x00, 0x00, 0x00, 0x00,                          /* FFB8 */
    /* function keys */
    0x3B, 0x3C,
    0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44,              /* FFC0 */
    0x57, 0x58, 0x5B, 0x5C, 0x5D, 0x00, 0x00, 0x00,              /* FFC8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FFD0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FFD8 */
    /* modifier keys */
    0x00, 0x2A, 0x136, 0x1D, 0x11D, 0x3A, 0x00, 0x38,            /* FFE0 */
    0x138, 0x38, 0x138, 0x15b, 0x15c, 0x00, 0x00, 0x00,          /* FFE8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FFF0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x153              /* FFF8 */
};

static const WORD xfree86_vendor_key_vkey[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF00 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF08 */
    0, VK_VOLUME_DOWN, VK_VOLUME_MUTE, VK_VOLUME_UP,            /* 1008FF10 */
    VK_MEDIA_PLAY_PAUSE, VK_MEDIA_STOP,
    VK_MEDIA_PREV_TRACK, VK_MEDIA_NEXT_TRACK,
    0, VK_LAUNCH_MAIL, 0, VK_BROWSER_SEARCH,                    /* 1008FF18 */
    0, 0, 0, VK_BROWSER_HOME,
    0, 0, 0, 0, 0, 0, VK_BROWSER_BACK, VK_BROWSER_FORWARD,      /* 1008FF20 */
    VK_BROWSER_STOP, VK_BROWSER_REFRESH, 0, 0, 0, 0, 0, 0,      /* 1008FF28 */
    VK_BROWSER_FAVORITES, 0, VK_LAUNCH_MEDIA_SELECT, 0,         /* 1008FF30 */
    0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF38 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF40 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF48 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF50 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF58 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF60 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF68 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF70 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF78 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF80 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF88 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF90 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF98 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFA0 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFA8 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFB0 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFB8 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFC0 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFC8 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFD0 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFD8 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFE0 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFE8 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFF0 */
    0, 0, 0, 0, 0, 0, 0, 0                                      /* 1008FFF8 */
};

/* Returns the Windows virtual key code associated with the X event <e> */
/* kbd_section must be held */
static WORD EVENT_event_to_vkey( XIC xic, XKeyEvent *e)
{
    KeySym keysym = 0;
    Status status;
    char buf[24];

    /* Clients should pass only KeyPress events to XmbLookupString */
    if (xic && e->type == KeyPress)
        XmbLookupString(xic, e, buf, sizeof(buf), &keysym, &status);
    else
        XLookupString(e, buf, sizeof(buf), &keysym, NULL);

    if ((e->state & NumLockMask) &&
        (keysym == XK_KP_Separator || keysym == XK_KP_Decimal ||
         (keysym >= XK_KP_0 && keysym <= XK_KP_9)))
        /* Only the Keypad keys 0-9 and . send different keysyms
         * depending on the NumLock state */
        return nonchar_key_vkey[keysym & 0xFF];

    /* Pressing the Pause/Break key alone produces VK_PAUSE vkey, while
     * pressing Ctrl+Pause/Break produces VK_CANCEL. */
    if ((e->state & ControlMask) && (keysym == XK_Break))
        return VK_CANCEL;

    TRACE_(key)("e->keycode = %u\n", e->keycode);

    return keyc2vkey[e->keycode];
}


/***********************************************************************
 *           X11DRV_send_keyboard_input
 */
static void X11DRV_send_keyboard_input( HWND hwnd, WORD vkey, WORD scan, UINT flags, UINT time )
{
    INPUT input;

    TRACE_(key)( "hwnd %p vkey=%04x scan=%04x flags=%04x\n", hwnd, vkey, scan, flags );

    input.type           = INPUT_KEYBOARD;
    input.ki.wVk         = vkey;
    input.ki.wScan       = scan;
    input.ki.dwFlags     = flags;
    input.ki.time        = time;
    input.ki.dwExtraInfo = 0;

    NtUserSendHardwareInput( hwnd, 0, &input, 0 );
}


/***********************************************************************
 *           set_async_key_state
 */
static void set_async_key_state( const BYTE state[256] )
{
    SERVER_START_REQ( set_key_state )
    {
        req->async = 1;
        wine_server_add_data( req, state, 256 );
        wine_server_call( req );
    }
    SERVER_END_REQ;
}

static void update_key_state( BYTE *keystate, BYTE key, int down )
{
    if (down)
    {
        if (!(keystate[key] & 0x80)) keystate[key] ^= 0x01;
        keystate[key] |= 0x80;
    }
    else keystate[key] &= ~0x80;
}

/***********************************************************************
 *           X11DRV_KeymapNotify
 *
 * Update modifiers state (Ctrl, Alt, Shift) when window is activated.
 *
 * This handles the case where one uses Ctrl+... Alt+... or Shift+.. to switch
 * from wine to another application and back.
 * Toggle keys are handled in HandleEvent.
 */
BOOL X11DRV_KeymapNotify( HWND hwnd, XEvent *event )
{
    int i, j;
    BYTE keystate[256];
    WORD vkey;
    DWORD flags;
    KeyCode keycode;
    HWND keymapnotify_hwnd;
    BOOL changed = FALSE;
    struct {
        WORD vkey;
        WORD scan;
        WORD pressed;
    } keys[256];
    struct x11drv_thread_data *thread_data = x11drv_thread_data();

    keymapnotify_hwnd = thread_data->keymapnotify_hwnd;
    thread_data->keymapnotify_hwnd = NULL;

    if (!NtUserGetAsyncKeyboardState( keystate )) return FALSE;

    memset(keys, 0, sizeof(keys));

    pthread_mutex_lock( &kbd_mutex );

    /* the minimum keycode is always greater or equal to 8, so we can
     * skip the first 8 values, hence start at 1
     */
    for (i = 1; i < 32; i++)
    {
        for (j = 0; j < 8; j++)
        {
            keycode = (i * 8) + j;
            vkey = keyc2vkey[keycode];

            /* If multiple keys map to the same vkey, we want to report it as
             * pressed iff any of them are pressed. */
            if (!keys[vkey & 0xff].vkey)
            {
                keys[vkey & 0xff].vkey = vkey;
                keys[vkey & 0xff].scan = keyc2scan[keycode] & 0xff;
            }

            if (event->xkeymap.key_vector[i] & (1<<j)) keys[vkey & 0xff].pressed = TRUE;
        }
    }

    for (vkey = 1; vkey <= 0xff; vkey++)
    {
        if (keys[vkey].vkey && !(keystate[vkey] & 0x80) != !keys[vkey].pressed)
        {
            TRACE( "Adjusting state for vkey %#.2x. State before %#.2x\n",
                   keys[vkey].vkey, keystate[vkey]);

            /* This KeymapNotify follows a FocusIn(mode=NotifyUngrab) event,
             * which is caused by a keyboard grab being released.
             * See XGrabKeyboard().
             *
             * We might have missed some key press/release events while the
             * keyboard was grabbed, but keyboard grab doesn't generate focus
             * lost / focus gained events on the Windows side, so the affected
             * program is not aware that it needs to resync the keyboard state.
             *
             * This, for example, may cause Alt being stuck after using Alt+Tab.
             *
             * To work around that problem we sync any possible key releases.
             *
             * Syncing key presses is not feasible as window managers differ in
             * event sequences, e.g. KDE performs two keyboard grabs for
             * Alt+Tab, which would sync the Tab press.
             */
            if (keymapnotify_hwnd && !keys[vkey].pressed)
            {
                TRACE( "Sending KEYUP for a modifier %#.2x\n", vkey);
                flags = KEYEVENTF_KEYUP;
                if (keys[vkey].vkey & 0x1000) flags |= KEYEVENTF_EXTENDEDKEY;
                X11DRV_send_keyboard_input( keymapnotify_hwnd, vkey, keys[vkey].scan, flags, NtGetTickCount() );
            }

            update_key_state( keystate, vkey, keys[vkey].pressed );
            changed = TRUE;
        }
    }

    pthread_mutex_unlock( &kbd_mutex );
    if (!changed) return FALSE;

    update_key_state( keystate, VK_CONTROL, (keystate[VK_LCONTROL] | keystate[VK_RCONTROL]) & 0x80 );
    update_key_state( keystate, VK_MENU, (keystate[VK_LMENU] | keystate[VK_RMENU]) & 0x80 );
    update_key_state( keystate, VK_SHIFT, (keystate[VK_LSHIFT] | keystate[VK_RSHIFT]) & 0x80 );
    set_async_key_state( keystate );
    return TRUE;
}

static void adjust_lock_state( BYTE *keystate, HWND hwnd, WORD vkey, WORD scan, DWORD flags, DWORD time )
{
    BYTE prev_state = keystate[vkey] & 0x01;

    X11DRV_send_keyboard_input( hwnd, vkey, scan, flags, time );
    X11DRV_send_keyboard_input( hwnd, vkey, scan, flags ^ KEYEVENTF_KEYUP, time );

    /* Keyboard hooks may have blocked processing lock keys causing our state
     * to be different than state on X server side. Although Windows allows hooks
     * to block changing state, we can't prevent it on X server side. Having
     * different states would cause us to try to adjust it again on the next
     * key event. We prevent that by overriding hooks and setting key states here. */
    if (NtUserGetAsyncKeyboardState( keystate ) && (keystate[vkey] & 0x01) == prev_state)
    {
        WARN("keystate %x not changed (%#.2x), probably blocked by hooks\n", vkey, keystate[vkey]);
        keystate[vkey] ^= 0x01;
        set_async_key_state( keystate );
    }
}

static void update_lock_state( HWND hwnd, WORD vkey, UINT state, UINT time )
{
    BYTE keystate[256];

    /* Note: X sets the below states on key down and clears them on key up.
       Windows triggers them on key down. */

    if (!NtUserGetAsyncKeyboardState( keystate )) return;

    /* Adjust the CAPSLOCK state if it has been changed outside wine */
    if (!(keystate[VK_CAPITAL] & 0x01) != !(state & LockMask) && vkey != VK_CAPITAL)
    {
        DWORD flags = 0;
        if (keystate[VK_CAPITAL] & 0x80) flags ^= KEYEVENTF_KEYUP;
        TRACE("Adjusting CapsLock state (%#.2x)\n", keystate[VK_CAPITAL]);
        adjust_lock_state( keystate, hwnd, VK_CAPITAL, 0x3a, flags, time );
    }

    /* Adjust the NUMLOCK state if it has been changed outside wine */
    if (!(keystate[VK_NUMLOCK] & 0x01) != !(state & NumLockMask) && (vkey & 0xff) != VK_NUMLOCK)
    {
        DWORD flags = KEYEVENTF_EXTENDEDKEY;
        if (keystate[VK_NUMLOCK] & 0x80) flags ^= KEYEVENTF_KEYUP;
        TRACE("Adjusting NumLock state (%#.2x)\n", keystate[VK_NUMLOCK]);
        adjust_lock_state( keystate, hwnd, VK_NUMLOCK, 0x45, flags, time );
    }

    /* Adjust the SCROLLLOCK state if it has been changed outside wine */
    if (!(keystate[VK_SCROLL] & 0x01) != !(state & ScrollLockMask) && vkey != VK_SCROLL)
    {
        DWORD flags = 0;
        if (keystate[VK_SCROLL] & 0x80) flags ^= KEYEVENTF_KEYUP;
        TRACE("Adjusting ScrLock state (%#.2x)\n", keystate[VK_SCROLL]);
        adjust_lock_state( keystate, hwnd, VK_SCROLL, 0x46, flags, time );
    }
}

/***********************************************************************
 *           X11DRV_KeyEvent
 *
 * Handle a X key event
 */
BOOL X11DRV_KeyEvent( HWND hwnd, XEvent *xev )
{
    XKeyEvent *event = &xev->xkey;
    char buf[24];
    char *Str = buf;
    KeySym keysym = 0;
    WORD vkey = 0, bScan;
    DWORD dwFlags;
    int ascii_chars;
    XIC xic = X11DRV_get_ic( hwnd );
    DWORD event_time = EVENT_x11_time_to_win32_time(event->time);
    struct x11drv_win_data *data;
    Status status = 0;

    TRACE_(key)("type %d, window %lx, state 0x%04x, keycode %u\n",
		event->type, event->window, event->state, event->keycode);

    if (event->type == KeyPress && (data = get_win_data( hwnd )))
    {
        window_set_user_time( data, event->time, FALSE );
        release_win_data( data );
    }

    /* Clients should pass only KeyPress events to XmbLookupString */
    if (xic && event->type == KeyPress)
    {
        ascii_chars = XmbLookupString(xic, event, buf, sizeof(buf), &keysym, &status);
        TRACE_(key)("XmbLookupString needs %i byte(s)\n", ascii_chars);
        if (status == XBufferOverflow)
        {
            Str = malloc( ascii_chars );
            if (Str == NULL)
            {
                ERR_(key)("Failed to allocate memory!\n");
                return FALSE;
            }
            ascii_chars = XmbLookupString(xic, event, Str, ascii_chars, &keysym, &status);
        }
    }
    else
        ascii_chars = XLookupString(event, buf, sizeof(buf), &keysym, NULL);

    TRACE_(key)("nbyte = %d, status %d\n", ascii_chars, status);

    if (status == XLookupChars)
    {
        xim_set_result_string( hwnd, Str, ascii_chars );
        if (buf != Str)
            free( Str );
        return TRUE;
    }

    pthread_mutex_lock( &kbd_mutex );

    /* If XKB extensions are used, the state mask for AltGr will use the group
       index instead of the modifier mask. The group index is set in bits
       13-14 of the state field in the XKeyEvent structure. So if AltGr is
       pressed, look if the group index is different than 0. From XKB
       extension documentation, the group index for AltGr should be 2
       (event->state = 0x2000). It's probably better to not assume a
       predefined group index and find it dynamically

       Ref: X Keyboard Extension: Library specification (section 14.1.1 and 17.1.1) */
    /* Save also all possible modifier states. */
    AltGrMask = event->state & (0x6000 | Mod1Mask | Mod2Mask | Mod3Mask | Mod4Mask | Mod5Mask);

    if (TRACE_ON(key)){
	const char *ksname;

        ksname = XKeysymToString(keysym);
	if (!ksname)
	  ksname = "No Name";
	TRACE_(key)("%s : keysym=%lx (%s), # of chars=%d / %s\n",
                    (event->type == KeyPress) ? "KeyPress" : "KeyRelease",
                    keysym, ksname, ascii_chars, debugstr_an(Str, ascii_chars));
    }
    if (buf != Str)
        free( Str );

    vkey = EVENT_event_to_vkey(xic,event);
    /* X returns keycode 0 for composed characters */
    if (!vkey && ascii_chars) vkey = VK_NONAME;
    bScan = keyc2scan[event->keycode] & 0xFF;

    TRACE_(key)("keycode %u converted to vkey 0x%X scan %02x\n",
                event->keycode, vkey, bScan);

    pthread_mutex_unlock( &kbd_mutex );

    if (!vkey) return FALSE;

    dwFlags = 0;
    if ( event->type == KeyRelease ) dwFlags |= KEYEVENTF_KEYUP;
    if ( vkey & 0x100 )              dwFlags |= KEYEVENTF_EXTENDEDKEY;

    update_lock_state( hwnd, vkey, event->state, event_time );

    X11DRV_send_keyboard_input( hwnd, vkey & 0xff, bScan, dwFlags, event_time );
    return TRUE;
}

/**********************************************************************
 *		X11DRV_KEYBOARD_DetectLayout
 *
 * Called from X11DRV_InitKeyboard
 *  This routine walks through the defined keyboard layouts and selects
 *  whichever matches most closely.
 * kbd_section must be held.
 */
static void
X11DRV_KEYBOARD_DetectLayout( Display *display )
{
  unsigned current, match, mismatch, seq, i, syms;
  int score, keyc, key, pkey, ok;
  KeySym keysym = 0;
  const char (*lkey)[MAIN_LEN][4];
  unsigned max_seq = 0;
  int max_score = INT_MIN, ismatch = 0;
  char ckey[256][4];

  syms = keysyms_per_keycode;
  if (syms > 4) {
    WARN("%d keysyms per keycode not supported, set to 4\n", syms);
    syms = 4;
  }

  memset( ckey, 0, sizeof(ckey) );
  for (keyc = min_keycode; keyc <= max_keycode; keyc++) {
      /* get data for keycode from X server */
      for (i = 0; i < syms; i++) {
        if (!(keysym = XkbKeycodeToKeysym( display, keyc, 0, i ))) continue;
	/* Allow both one-byte and two-byte national keysyms */
	if ((keysym < 0x8000) && (keysym != ' '))
        {
            if (!XkbTranslateKeySym(display, &keysym, 0, &ckey[keyc][i], 1, NULL))
            {
                TRACE("XKB could not translate keysym %04lx\n", keysym);
                /* FIXME: query what keysym is used as Mode_switch, fill XKeyEvent
                 * with appropriate ShiftMask and Mode_switch, use XLookupString
                 * to get character in the local encoding.
                 */
                ckey[keyc][i] = keysym & 0xFF;
            }
        }
	else {
	  ckey[keyc][i] = KEYBOARD_MapDeadKeysym(keysym);
	}
      }
  }

  for (current = 0; main_key_tab[current].comment; current++) {
    TRACE("Attempting to match against \"%s\"\n", main_key_tab[current].comment);
    match = 0;
    mismatch = 0;
    score = 0;
    seq = 0;
    lkey = main_key_tab[current].key;
    pkey = -1;
    for (keyc = min_keycode; keyc <= max_keycode; keyc++) {
      if (ckey[keyc][0]) {
	/* search for a match in layout table */
	/* right now, we just find an absolute match for defined positions */
	/* (undefined positions are ignored, so if it's defined as "3#" in */
	/* the table, it's okay that the X server has "3#£", for example) */
	/* however, the score will be higher for longer matches */
	for (key = 0; key < MAIN_LEN; key++) {
	  for (ok = 0, i = 0; (ok >= 0) && (i < syms); i++) {
	    if ((*lkey)[key][i] && ((*lkey)[key][i] == ckey[keyc][i]))
	      ok++;
	    if ((*lkey)[key][i] && ((*lkey)[key][i] != ckey[keyc][i]))
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
          /* print spaces instead of \0's */
          char str[5];
          for (i = 0; i < 4; i++) str[i] = ckey[keyc][i] ? ckey[keyc][i] : ' ';
          str[4] = 0;
          TRACE_(key)("mismatch for keycode %u, got %s\n", keyc, debugstr_a(str));
          mismatch++;
          score -= syms;
	}
      }
    }
    TRACE("matches=%d, mismatches=%d, seq=%d, score=%d\n",
	   match, mismatch, seq, score);
    if (score + (int)seq > max_score + (int)max_seq) {
      /* best match so far */
      kbd_layout = current;
      max_score = score;
      max_seq = seq;
      ismatch = !mismatch;
    }
  }
  /* we're done, report results if necessary */
  if (!ismatch)
    WARN("Using closest match (%s) for scan/virtual codes mapping.\n",
        main_key_tab[kbd_layout].comment);

  TRACE("detected layout is \"%s\"\n", main_key_tab[kbd_layout].comment);
}


/**********************************************************************
 *		X11DRV_InitKeyboard
 */
void X11DRV_InitKeyboard( Display *display )
{
    XModifierKeymap *mmp;
    KeySym keysym;
    KeyCode *kcp;
    XKeyEvent e2;
    WORD scan, vkey;
    int keyc, i, keyn, syms;
    char ckey[4]={0,0,0,0};
    const char (*lkey)[MAIN_LEN][4];
    char vkey_used[256] = { 0 };

    /* Ranges of OEM, function key, and character virtual key codes.
     * Don't include those handled specially in X11DRV_ToUnicodeEx and
     * X11DRV_MapVirtualKeyEx, like VK_NUMPAD0 - VK_DIVIDE. */
    static const struct {
        WORD first, last;
    } vkey_ranges[] = {
        { VK_OEM_1, VK_OEM_3 },
        { VK_OEM_4, VK_OEM_8 },
        { VK_OEM_AX, VK_ICO_00 },
        { 0xe6, 0xe6 },
        { 0xe9, 0xf5 },
        { VK_OEM_NEC_EQUAL, VK_OEM_NEC_EQUAL },
        { VK_F1, VK_F24 },
        { 0x30, 0x39 }, /* VK_0 - VK_9 */
        { 0x41, 0x5a }, /* VK_A - VK_Z */
        { 0, 0 }
    };
    int vkey_range;

    pthread_mutex_lock( &kbd_mutex );
    XDisplayKeycodes(display, &min_keycode, &max_keycode);
    XFree( XGetKeyboardMapping( display, min_keycode, max_keycode + 1 - min_keycode, &keysyms_per_keycode ) );

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
                    if (XkbKeycodeToKeysym( display, *kcp, 0, k ) == XK_Num_Lock)
		    {
                        NumLockMask = 1 << i;
                        TRACE_(key)("NumLockMask is %x\n", NumLockMask);
		    }
                    else if (XkbKeycodeToKeysym( display, *kcp, 0, k ) == XK_Scroll_Lock)
		    {
                        ScrollLockMask = 1 << i;
                        TRACE_(key)("ScrollLockMask is %x\n", ScrollLockMask);
		    }
            }
    }
    XFreeModifiermap(mmp);

    /* Detect the keyboard layout */
    X11DRV_KEYBOARD_DetectLayout( display );
    lkey = main_key_tab[kbd_layout].key;
    syms = (keysyms_per_keycode > 4) ? 4 : keysyms_per_keycode;

    /* Now build two conversion arrays :
     * keycode -> vkey + scancode + extended
     * vkey + extended -> keycode */

    e2.display = display;
    e2.state = 0;
    e2.type = KeyPress;

    memset(keyc2vkey, 0, sizeof(keyc2vkey));
    for (keyc = min_keycode; keyc <= max_keycode; keyc++)
    {
        char buf[30];
        int have_chars;

        keysym = 0;
        e2.keycode = (KeyCode)keyc;
        have_chars = XLookupString(&e2, buf, sizeof(buf), &keysym, NULL);
        vkey = 0; scan = 0;
        if (keysym)  /* otherwise, keycode not used */
        {
            if ((keysym >> 8) == 0xFF)         /* non-character key */
            {
                vkey = nonchar_key_vkey[keysym & 0xff];
                scan = nonchar_key_scan[keysym & 0xff];
		/* set extended bit when necessary */
		if (scan & 0x100) vkey |= 0x100;
            } else if ((keysym >> 8) == 0x1008FF) { /* XFree86 vendor keys */
                vkey = xfree86_vendor_key_vkey[keysym & 0xff];
                /* All vendor keys are extended with a scan code of 0 per testing on WinXP */
                scan = 0x100;
		vkey |= 0x100;
            } else if (keysym == 0x20) {                 /* Spacebar */
	        vkey = VK_SPACE;
		scan = 0x39;
	    } else if (have_chars) {
	      /* we seem to need to search the layout-dependent scancodes */
	      int maxlen=0,maxval=-1,ok;
	      for (i=0; i<syms; i++) {
		keysym = XkbKeycodeToKeysym( display, keyc, 0, i );
		if ((keysym<0x8000) && (keysym!=' '))
                {
                    if (!XkbTranslateKeySym(display, &keysym, 0, &ckey[i], 1, NULL))
                    {
                        /* FIXME: query what keysym is used as Mode_switch, fill XKeyEvent
                         * with appropriate ShiftMask and Mode_switch, use XLookupString
                         * to get character in the local encoding.
                         */
                        ckey[i] = (keysym <= 0x7F) ? keysym : 0;
                    }
		} else {
		  ckey[i] = KEYBOARD_MapDeadKeysym(keysym);
		}
	      }
	      /* find key with longest match streak */
	      for (keyn=0; keyn<MAIN_LEN; keyn++) {
		for (ok=(*lkey)[keyn][i=0]; ok&&(i<4); i++)
		  if ((*lkey)[keyn][i] && (*lkey)[keyn][i]!=ckey[i]) ok=0;
		if (!ok) i--; /* we overshot */
		if (ok||(i>maxlen)) {
		  maxlen=i; maxval=keyn;
		}
		if (ok) break;
	      }
	      if (maxval>=0) {
		/* got it */
		const WORD (*lscan)[MAIN_LEN] = main_key_tab[kbd_layout].scan;
		const WORD (*lvkey)[MAIN_LEN] = main_key_tab[kbd_layout].vkey;
		scan = (*lscan)[maxval];
		vkey = (*lvkey)[maxval];
	      }
	    }
        }
        TRACE("keycode %u => vkey %04X\n", e2.keycode, vkey);
        keyc2vkey[e2.keycode] = vkey;
        keyc2scan[e2.keycode] = scan;
        if ((vkey & 0xff) && vkey_used[(vkey & 0xff)])
            WARN("vkey %04X is being used by more than one keycode\n", vkey);
        vkey_used[(vkey & 0xff)] = 1;
    } /* for */

#define VKEY_IF_NOT_USED(vkey) (vkey_used[(vkey)] ? 0 : (vkey_used[(vkey)] = 1, (vkey)))
    for (keyc = min_keycode; keyc <= max_keycode; keyc++)
    {
        vkey = keyc2vkey[keyc] & 0xff;
        if (vkey)
            continue;

        e2.keycode = (KeyCode)keyc;
        keysym = XLookupKeysym(&e2, 0);
        if (!keysym)
           continue;

        /* find a suitable layout-dependent VK code */
        /* (most Winelib apps ought to be able to work without layout tables!) */
        for (i = 0; (i < keysyms_per_keycode) && (!vkey); i++)
        {
            keysym = XLookupKeysym(&e2, i);
            if ((keysym >= XK_0 && keysym <= XK_9)
                || (keysym >= XK_A && keysym <= XK_Z)) {
                vkey = VKEY_IF_NOT_USED(keysym);
            }
        }

        for (i = 0; (i < keysyms_per_keycode) && (!vkey); i++)
        {
            keysym = XLookupKeysym(&e2, i);
            switch (keysym)
            {
            case ';':             vkey = VKEY_IF_NOT_USED(VK_OEM_1); break;
            case '/':             vkey = VKEY_IF_NOT_USED(VK_OEM_2); break;
            case '`':             vkey = VKEY_IF_NOT_USED(VK_OEM_3); break;
            case '[':             vkey = VKEY_IF_NOT_USED(VK_OEM_4); break;
            case '\\':            vkey = VKEY_IF_NOT_USED(VK_OEM_5); break;
            case ']':             vkey = VKEY_IF_NOT_USED(VK_OEM_6); break;
            case '\'':            vkey = VKEY_IF_NOT_USED(VK_OEM_7); break;
            case ',':             vkey = VKEY_IF_NOT_USED(VK_OEM_COMMA); break;
            case '.':             vkey = VKEY_IF_NOT_USED(VK_OEM_PERIOD); break;
            case '-':             vkey = VKEY_IF_NOT_USED(VK_OEM_MINUS); break;
            case '+':             vkey = VKEY_IF_NOT_USED(VK_OEM_PLUS); break;
            }
        }

        if (vkey)
        {
            TRACE("keycode %u => vkey %04X\n", e2.keycode, vkey);
            keyc2vkey[e2.keycode] = vkey;
        }
    } /* for */

    /* For any keycodes which still don't have a vkey, assign any spare
     * character, function key, or OEM virtual key code. */
    vkey_range = 0;
    vkey = vkey_ranges[vkey_range].first;
    for (keyc = min_keycode; keyc <= max_keycode; keyc++)
    {
        if (keyc2vkey[keyc] & 0xff)
            continue;

        e2.keycode = (KeyCode)keyc;
        keysym = XLookupKeysym(&e2, 0);
        if (!keysym)
           continue;

        while (vkey && vkey_used[vkey])
        {
            if (vkey == vkey_ranges[vkey_range].last)
            {
                vkey_range++;
                vkey = vkey_ranges[vkey_range].first;
            }
            else
                vkey++;
        }

        if (!vkey)
        {
            WARN("No more vkeys available!\n");
            break;
        }

        if (TRACE_ON(keyboard))
        {
            TRACE("spare virtual key %04X assigned to keycode %u:\n",
                             vkey, e2.keycode);
            TRACE("(");
            for (i = 0; i < keysyms_per_keycode; i += 1)
            {
                const char *ksname;

                keysym = XLookupKeysym(&e2, i);
                ksname = XKeysymToString(keysym);
                if (!ksname)
                    ksname = "NoSymbol";
                TRACE( "%lx (%s) ", keysym, ksname);
            }
            TRACE(")\n");
        }

        TRACE("keycode %u => vkey %04X\n", e2.keycode, vkey);
        keyc2vkey[e2.keycode] = vkey;
        vkey_used[vkey] = 1;
    } /* for */
#undef VKEY_IF_NOT_USED

    /* If some keys still lack scancodes, assign some arbitrary ones to them now */
    for (scan = 0x60, keyc = min_keycode; keyc <= max_keycode; keyc++)
      if (keyc2vkey[keyc]&&!keyc2scan[keyc]) {
	const char *ksname;
	keysym = XkbKeycodeToKeysym( display, keyc, 0, 0 );
	ksname = XKeysymToString(keysym);
	if (!ksname) ksname = "NoSymbol";

	/* should make sure the scancode is unassigned here, but >=0x60 currently always is */

	TRACE_(key)("assigning scancode %02x to unidentified keycode %u (%s)\n",scan,keyc,ksname);
	keyc2scan[keyc]=scan++;
      }

    pthread_mutex_unlock( &kbd_mutex );
}


/***********************************************************************
 *		ActivateKeyboardLayout (X11DRV.@)
 */
BOOL X11DRV_ActivateKeyboardLayout(HKL hkl, UINT flags)
{
    FIXME("%p, %04x: semi-stub!\n", hkl, flags);

    if (flags & KLF_SETFORPROCESS)
    {
        RtlSetLastWin32Error( ERROR_CALL_NOT_IMPLEMENTED );
        FIXME("KLF_SETFORPROCESS not supported\n");
        return FALSE;
    }

    return TRUE;
}


/***********************************************************************
 *           X11DRV_MappingNotify
 */
BOOL X11DRV_MappingNotify( HWND dummy, XEvent *event )
{
    HWND hwnd;

    XRefreshKeyboardMapping(&event->xmapping);
    X11DRV_InitKeyboard( event->xmapping.display );

    hwnd = get_focus();
    if (!hwnd) hwnd = get_active_window();
    NtUserPostMessage( hwnd, WM_INPUTLANGCHANGEREQUEST,
                       0 /*FIXME*/, (LPARAM)NtUserGetKeyboardLayout(0) );
    return TRUE;
}


/***********************************************************************
 *		VkKeyScanEx (X11DRV.@)
 *
 * Note: Windows ignores HKL parameter and uses current active layout instead
 */
SHORT X11DRV_VkKeyScanEx( WCHAR wChar, HKL hkl )
{
    Display *display = thread_init_display();
    KeyCode keycode;
    KeySym keysym;
    int index;
    CHAR cChar;
    SHORT ret;

    /* FIXME: what happens if wChar is not a Latin1 character and CP_UNIXCP
     * is UTF-8 (multibyte encoding)?
     */
    if (!ntdll_wcstoumbs( &wChar, 1, &cChar, 1, FALSE ))
    {
        WARN("no translation from unicode to CP_UNIXCP for 0x%02x\n", wChar);
        return -1;
    }

    TRACE("wChar 0x%02x -> cChar '%c'\n", wChar, cChar);

    /* char->keysym (same for ANSI chars) */
    keysym = (unsigned char)cChar; /* (!) cChar is signed */
    if (keysym <= 27) keysym += 0xFF00; /* special chars : return, backspace... */

    keycode = XKeysymToKeycode(display, keysym);  /* keysym -> keycode */
    if (!keycode)
    {
        if (keysym >= 0xFF00) /* Windows returns 0x0240 + cChar in this case */
        {
            ret = 0x0240 + cChar; /* 0x0200 indicates a control character */
            TRACE(" ... returning ctrl char %#.2x\n", ret);
            return ret;
        }
        /* It didn't work ... let's try with deadchar code. */
        TRACE("retrying with | 0xFE00\n");
        keycode = XKeysymToKeycode(display, keysym | 0xFE00);
    }

    TRACE("'%c'(%lx): got keycode %u\n", cChar, keysym, keycode);
    if (!keycode) return -1;

    pthread_mutex_lock( &kbd_mutex );

    /* keycode -> (keyc2vkey) vkey */
    ret = keyc2vkey[keycode];
    if (!ret)
    {
        pthread_mutex_unlock( &kbd_mutex );
        TRACE("keycode for '%c' not found, returning -1\n", cChar);
        return -1;
    }

    for (index = 0; index < 4; index++) /* find shift state */
        if (XkbKeycodeToKeysym( display, keycode, 0, index ) == keysym) break;

    pthread_mutex_unlock( &kbd_mutex );

    switch (index)
    {
        case 0: break;
        case 1: ret += 0x0100; break;
        case 2: ret += 0x0600; break;
        case 3: ret += 0x0700; break;
        default:
            WARN("Keysym %lx not found while parsing the keycode table\n", keysym);
            return -1;
    }
    /*
      index : 0     adds 0x0000
      index : 1     adds 0x0100 (shift)
      index : ?     adds 0x0200 (ctrl)
      index : 2     adds 0x0600 (ctrl+alt)
      index : 3     adds 0x0700 (ctrl+alt+shift)
     */

    TRACE(" ... returning %#.2x\n", ret);
    return ret;
}

/***********************************************************************
 *		MapVirtualKeyEx (X11DRV.@)
 */
UINT X11DRV_MapVirtualKeyEx( UINT wCode, UINT wMapType, HKL hkl )
{
    UINT ret = 0;
    int keyc;
    Display *display = thread_init_display();

    TRACE("wCode=0x%x, wMapType=%d, hkl %p\n", wCode, wMapType, hkl);

    pthread_mutex_lock( &kbd_mutex );

    switch(wMapType)
    {
        case MAPVK_VK_TO_VSC: /* vkey-code to scan-code */
        case MAPVK_VK_TO_VSC_EX:
            switch (wCode)
            {
                case VK_SHIFT: wCode = VK_LSHIFT; break;
                case VK_CONTROL: wCode = VK_LCONTROL; break;
                case VK_MENU: wCode = VK_LMENU; break;
            }

            /* let's do vkey -> keycode -> scan */
            for (keyc = min_keycode; keyc <= max_keycode; keyc++)
            {
                if ((keyc2vkey[keyc] & 0xFF) == wCode)
                {
                    ret = keyc2scan[keyc] & 0xFF;
                    break;
                }
            }

            /* set scan code prefix */
            if (wMapType == MAPVK_VK_TO_VSC_EX &&
                (wCode == VK_RCONTROL || wCode == VK_RMENU))
                ret |= 0xe000;
            break;

        case MAPVK_VSC_TO_VK: /* scan-code to vkey-code */
        case MAPVK_VSC_TO_VK_EX:

            /* let's do scan -> keycode -> vkey */
            for (keyc = min_keycode; keyc <= max_keycode; keyc++)
                if ((keyc2scan[keyc] & 0xFF) == (wCode & 0xFF))
                {
                    ret = keyc2vkey[keyc] & 0xFF;
                    /* Only stop if it's not a numpad vkey; otherwise keep
                       looking for a potential better vkey. */
                    if (ret && (ret < VK_NUMPAD0 || VK_DIVIDE < ret))
                        break;
                }

            if (wMapType == MAPVK_VSC_TO_VK)
                switch (ret)
                {
                    case VK_LSHIFT:
                    case VK_RSHIFT:
                        ret = VK_SHIFT; break;
                    case VK_LCONTROL:
                    case VK_RCONTROL:
                        ret = VK_CONTROL; break;
                    case VK_LMENU:
                    case VK_RMENU:
                        ret = VK_MENU; break;
                }

            break;

        case MAPVK_VK_TO_CHAR: /* vkey-code to unshifted ANSI code */
        {
            /* we still don't know what "unshifted" means. in windows VK_W (0x57)
             * returns 0x57, which is uppercase 'W'. So we have to return the uppercase
             * key.. Looks like something is wrong with the MS docs?
             * This is only true for letters, for example VK_0 returns '0' not ')'.
             * - hence we use the lock mask to ensure this happens.
             */
            /* let's do vkey -> keycode -> (XLookupString) ansi char */
            XKeyEvent e;
            KeySym keysym;
            int len;
            char s[10];

            e.display = display;
            e.state = 0;
            e.keycode = 0;
            e.type = KeyPress;

            /* We exit on the first keycode found, to speed up the thing. */
            for (keyc=min_keycode; (keyc<=max_keycode) && (!e.keycode) ; keyc++)
            { /* Find a keycode that could have generated this virtual key */
                if  ((keyc2vkey[keyc] & 0xFF) == wCode)
                { /* We filter the extended bit, we don't know it */
                    e.keycode = keyc; /* Store it temporarily */
                    if ((EVENT_event_to_vkey(0,&e) & 0xFF) != wCode) {
                        e.keycode = 0; /* Wrong one (ex: because of the NumLock
                                          state), so set it to 0, we'll find another one */
                    }
                }
            }

            if ((wCode>=VK_NUMPAD0) && (wCode<=VK_NUMPAD9))
                e.keycode = XKeysymToKeycode(e.display, wCode-VK_NUMPAD0+XK_KP_0);

            /* Windows always generates VK_DECIMAL for Del/. on keypad while some
             * X11 keyboard layouts generate XK_KP_Separator instead of XK_KP_Decimal
             * in order to produce a locale dependent numeric separator.
             */
            if (wCode == VK_DECIMAL || wCode == VK_SEPARATOR)
            {
                e.keycode = XKeysymToKeycode(e.display, XK_KP_Separator);
                if (!e.keycode)
                    e.keycode = XKeysymToKeycode(e.display, XK_KP_Decimal);
            }

            if (!e.keycode)
            {
                WARN("Unknown virtual key %X !!!\n", wCode);
                break;
            }
            TRACE("Found keycode %u\n",e.keycode);

            len = XLookupString(&e, s, sizeof(s), &keysym, NULL);
            if (len)
            {
                WCHAR wch;
                if (ntdll_umbstowcs( s, len, &wch, 1 )) ret = RtlUpcaseUnicodeChar( wch );
            }
            break;
        }

        default: /* reserved */
            FIXME("Unknown wMapType %d !\n", wMapType);
            break;
    }

    pthread_mutex_unlock( &kbd_mutex );
    TRACE( "returning 0x%x.\n", ret );
    return ret;
}

/***********************************************************************
 *		GetKeyNameText (X11DRV.@)
 */
INT X11DRV_GetKeyNameText( LONG lParam, LPWSTR lpBuffer, INT nSize )
{
  Display *display = thread_init_display();
  int vkey, ansi, scanCode;
  KeyCode keyc;
  int keyi;
  KeySym keys;
  char *name;

  scanCode = lParam >> 16;
  scanCode &= 0x1ff;  /* keep "extended-key" flag with code */

  vkey = X11DRV_MapVirtualKeyEx( scanCode, MAPVK_VSC_TO_VK_EX, NtUserGetKeyboardLayout(0) );

  /*  handle "don't care" bit (0x02000000) */
  if (!(lParam & 0x02000000)) {
    switch (vkey) {
         case VK_RSHIFT:
                          /* R-Shift is "special" - it is an extended key with separate scan code */
                          scanCode |= 0x100;
                          /* fall through */
         case VK_LSHIFT:
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
    }
  }

  ansi = X11DRV_MapVirtualKeyEx( vkey, MAPVK_VK_TO_CHAR, NtUserGetKeyboardLayout(0) );
  TRACE("scan 0x%04x, vkey 0x%04X, ANSI 0x%04x\n", scanCode, vkey, ansi);

  /* first get the name of the "regular" keys which is the Upper case
     value of the keycap imprint.                                     */
  if ( ((ansi >= 0x21) && (ansi <= 0x7e)) &&
       (scanCode != 0x137) &&   /* PrtScn   */
       (scanCode != 0x135) &&   /* numpad / */
       (scanCode != 0x37 ) &&   /* numpad * */
       (scanCode != 0x4a ) &&   /* numpad - */
       (scanCode != 0x4e ) )    /* numpad + */
      {
        if (nSize >= 2)
	{
          *lpBuffer = RtlUpcaseUnicodeChar( ansi );
          *(lpBuffer+1) = 0;
          return 1;
        }
     else
        return 0;
  }

  /* FIXME: horrible hack to fix function keys. Windows reports scancode
            without "extended-key" flag. However Wine generates scancode
            *with* "extended-key" flag. Seems to occur *only* for the
            function keys. Soooo.. We will leave the table alone and
            fudge the lookup here till the other part is found and fixed!!! */

  if ( ((scanCode >= 0x13b) && (scanCode <= 0x144)) ||
       (scanCode == 0x157) || (scanCode == 0x158))
    scanCode &= 0xff;   /* remove "extended-key" flag for Fx keys */

  /* let's do scancode -> keycode -> keysym -> String */

  pthread_mutex_lock( &kbd_mutex );

  for (keyi=min_keycode; keyi<=max_keycode; keyi++)
      if ((keyc2scan[keyi]) == scanCode)
         break;
  if (keyi <= max_keycode)
  {
      INT rc;

      keyc = (KeyCode) keyi;
      keys = XkbKeycodeToKeysym( display, keyc, 0, 0 );
      name = XKeysymToString(keys);

      if (name && (vkey == VK_SHIFT || vkey == VK_CONTROL || vkey == VK_MENU))
      {
          char* idx = strrchr(name, '_');
          if (idx && (idx[1] == 'r' || idx[1] == 'R' || idx[1] == 'l' || idx[1] == 'L') && !idx[2])
          {
              pthread_mutex_unlock( &kbd_mutex );
              TRACE("found scan=%04x keyc=%u keysym=%lx modified_string=%s\n",
                    scanCode, keyc, keys, debugstr_an(name,idx-name));
              rc = ntdll_umbstowcs( name, idx - name + 1, lpBuffer, nSize );
              if (!rc) rc = nSize;
              lpBuffer[--rc] = 0;
              return rc;
          }
      }

      if (name)
      {
          pthread_mutex_unlock( &kbd_mutex );
          TRACE("found scan=%04x keyc=%u keysym=%04x vkey=%04x string=%s\n",
                scanCode, keyc, (int)keys, vkey, debugstr_a(name));
          rc = ntdll_umbstowcs( name, strlen(name) + 1, lpBuffer, nSize );
          if (!rc) rc = nSize;
          lpBuffer[--rc] = 0;
          return rc;
      }
  }

  /* Finally issue WARN for unknown keys   */

  pthread_mutex_unlock( &kbd_mutex );
  WARN("(%08x,%p,%d): unsupported key, vkey=%04X, ansi=%04x\n",(int)lParam,lpBuffer,nSize,vkey,ansi);
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
	TRACE("no character for dead keysym 0x%08lx\n",keysym);
	return 0;
}

/***********************************************************************
 *		ToUnicodeEx (X11DRV.@)
 *
 * The ToUnicode function translates the specified virtual-key code and keyboard
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
INT X11DRV_ToUnicodeEx( UINT virtKey, UINT scanCode, const BYTE *lpKeyState,
                        LPWSTR bufW, int bufW_size, UINT flags, HKL hkl )
{
    Display *display = thread_init_display();
    XKeyEvent e;
    KeySym keysym = 0;
    INT ret;
    int keyc;
    char buf[10];
    char *lpChar = buf;
    HWND focus;
    XIC xic;
    Status status = 0;

    if (scanCode & 0x8000)
    {
        TRACE_(key)("Key UP, doing nothing\n" );
        return 0;
    }

    if ((lpKeyState[VK_MENU] & 0x80) && (lpKeyState[VK_CONTROL] & 0x80))
    {
        TRACE_(key)("Ctrl+Alt+[key] won't generate a character\n");
        return 0;
    }

    e.display = display;
    e.keycode = 0;
    e.state = 0;
    e.type = KeyPress;

    focus = x11drv_thread_data()->last_xic_hwnd;
    if (!focus)
    {
        focus = get_focus();
        if (focus) focus = NtUserGetAncestor( focus, GA_ROOT );
        if (!focus) focus = get_active_window();
    }
    e.window = X11DRV_get_whole_window( focus );
    xic = X11DRV_get_ic( focus );

    pthread_mutex_lock( &kbd_mutex );

    if (lpKeyState[VK_SHIFT] & 0x80)
    {
	TRACE_(key)("ShiftMask = %04x\n", ShiftMask);
	e.state |= ShiftMask;
    }
    if (lpKeyState[VK_CAPITAL] & 0x01)
    {
	TRACE_(key)("LockMask = %04x\n", LockMask);
	e.state |= LockMask;
    }
    if (lpKeyState[VK_CONTROL] & 0x80)
    {
	TRACE_(key)("ControlMask = %04x\n", ControlMask);
	e.state |= ControlMask;
    }
    if (lpKeyState[VK_NUMLOCK] & 0x01)
    {
	TRACE_(key)("NumLockMask = %04x\n", NumLockMask);
	e.state |= NumLockMask;
    }

    /* Restore saved AltGr state */
    TRACE_(key)("AltGrMask = %04x\n", AltGrMask);
    e.state |= AltGrMask;

    TRACE_(key)("(%04X, %04X) : faked state = 0x%04x\n",
		virtKey, scanCode, e.state);

    /* We exit on the first keycode found, to speed up the thing. */
    for (keyc=min_keycode; (keyc<=max_keycode) && (!e.keycode) ; keyc++)
      { /* Find a keycode that could have generated this virtual key */
          if  ((keyc2vkey[keyc] & 0xFF) == virtKey)
          { /* We filter the extended bit, we don't know it */
              e.keycode = keyc; /* Store it temporarily */
              if ((EVENT_event_to_vkey(xic,&e) & 0xFF) != virtKey) {
                  e.keycode = 0; /* Wrong one (ex: because of the NumLock
                         state), so set it to 0, we'll find another one */
              }
	  }
      }

    if (virtKey >= VK_LEFT && virtKey <= VK_DOWN)
        e.keycode = XKeysymToKeycode(e.display, virtKey - VK_LEFT + XK_Left);

    if ((virtKey>=VK_NUMPAD0) && (virtKey<=VK_NUMPAD9))
        e.keycode = XKeysymToKeycode(e.display, virtKey-VK_NUMPAD0+XK_KP_0);

    /* Windows always generates VK_DECIMAL for Del/. on keypad while some
     * X11 keyboard layouts generate XK_KP_Separator instead of XK_KP_Decimal
     * in order to produce a locale dependent numeric separator.
     */
    if (virtKey == VK_DECIMAL || virtKey == VK_SEPARATOR)
    {
        e.keycode = XKeysymToKeycode(e.display, XK_KP_Separator);
        if (!e.keycode)
            e.keycode = XKeysymToKeycode(e.display, XK_KP_Decimal);
    }

    /* Ctrl-Space generates space on Windows */
    if (e.state == ControlMask && virtKey == VK_SPACE)
    {
        bufW[0] = ' ';
        ret = 1;
        goto found;
    }

    if (!e.keycode && virtKey != VK_NONAME)
      {
	WARN_(key)("Unknown virtual key %X !!!\n", virtKey);
        pthread_mutex_unlock( &kbd_mutex );
	return 0;
      }
    else TRACE_(key)("Found keycode %u\n",e.keycode);

    TRACE_(key)("type %d, window %lx, state 0x%04x, keycode %u\n",
		e.type, e.window, e.state, e.keycode);

    /* Clients should pass only KeyPress events to XmbLookupString,
     * e.type was set to KeyPress above.
     */
    if (xic)
    {
        ret = XmbLookupString(xic, &e, buf, sizeof(buf), &keysym, &status);
        TRACE_(key)("XmbLookupString needs %d byte(s)\n", ret);
        if (status == XBufferOverflow)
        {
            lpChar = malloc( ret );
            if (lpChar == NULL)
            {
                ERR_(key)("Failed to allocate memory!\n");
                pthread_mutex_unlock( &kbd_mutex );
                return 0;
            }
            ret = XmbLookupString(xic, &e, lpChar, ret, &keysym, &status);
        }
    }
    else
        ret = XLookupString(&e, buf, sizeof(buf), &keysym, NULL);

    TRACE_(key)("nbyte = %d, status 0x%x\n", ret, status);

    if (TRACE_ON(key))
    {
        const char *ksname;

        ksname = XKeysymToString(keysym);
        if (!ksname) ksname = "No Name";
        TRACE_(key)("%s : keysym=%lx (%s), # of chars=%d / %s\n",
                    (e.type == KeyPress) ? "KeyPress" : "KeyRelease",
                    keysym, ksname, ret, debugstr_an(lpChar, ret));
    }

    if (ret == 0)
    {
	char dead_char;

#ifdef XK_EuroSign
        /* An ugly hack for EuroSign: X can't translate it to a character
           for some locales. */
        if (keysym == XK_EuroSign)
        {
            bufW[0] = 0x20AC;
            ret = 1;
            goto found;
        }
#endif
        /* Special case: X turns shift-tab into ISO_Left_Tab. */
        /* Here we change it back. */
        if (keysym == XK_ISO_Left_Tab && !(e.state & ControlMask))
        {
            bufW[0] = 0x09;
            ret = 1;
            goto found;
        }

	dead_char = KEYBOARD_MapDeadKeysym(keysym);
	if (dead_char)
        {
	    ntdll_umbstowcs( &dead_char, 1, bufW, bufW_size );
	    ret = -1;
            goto found;
        }

        if (keysym >= 0x01000100 && keysym <= 0x0100ffff)
        {
            /* Unicode direct mapping */
            bufW[0] = keysym & 0xffff;
            ret = 1;
            goto found;
        }
        else if ((keysym >> 8) == 0x1008FF) {
            bufW[0] = 0;
            ret = 0;
            goto found;
        }
	else
	    {
	    const char *ksname;

	    ksname = XKeysymToString(keysym);
	    if (!ksname)
		ksname = "No Name";
	    if ((keysym >> 8) != 0xff)
		{
		WARN_(key)("no char for keysym %04lx (%s) :\n",
                    keysym, ksname);
		WARN_(key)("virtKey=%X, scanCode=%X, keycode=%u, state=%X\n",
                    virtKey, scanCode, e.keycode, e.state);
		}
	    }
	}
    else {  /* ret != 0 */
        /* We have a special case to handle : Shift + arrow, shift + home, ...
           X returns a char for it, but Windows doesn't. Let's eat it. */
        if (!(e.state & NumLockMask)  /* NumLock is off */
            && (e.state & ShiftMask) /* Shift is pressed */
            && (keysym>=XK_KP_0) && (keysym<=XK_KP_9))
        {
            lpChar[0] = 0;
            ret = 0;
        }

        /* more areas where X returns characters but Windows does not
           CTRL + number or CTRL + symbol */
        if (e.state & ControlMask)
        {
            if (((keysym>=33) && (keysym < '@')) ||
                (keysym == '`') ||
                (keysym == XK_Tab))
            {
                lpChar[0] = 0;
                ret = 0;
            }
        }

        /* We have another special case for delete key (XK_Delete) on an
         extended keyboard. X returns a char for it, but Windows doesn't */
        if (keysym == XK_Delete)
        {
            lpChar[0] = 0;
            ret = 0;
        }
	else if((lpKeyState[VK_SHIFT] & 0x80) /* Shift is pressed */
		&& (keysym == XK_KP_Decimal))
        {
            lpChar[0] = 0;
            ret = 0;
        }
	else if((lpKeyState[VK_CONTROL] & 0x80) /* Control is pressed */
		&& (keysym == XK_Return || keysym == XK_KP_Enter))
        {
            if (lpKeyState[VK_SHIFT] & 0x80)
            {
                lpChar[0] = 0;
                ret = 0;
            }
            else
            {
                lpChar[0] = '\n';
                ret = 1;
            }
        }

        /* Hack to detect an XLookupString hard-coded to Latin1 */
        if (ret == 1 && keysym >= 0x00a0 && keysym <= 0x00ff && (BYTE)lpChar[0] == keysym)
        {
            bufW[0] = (BYTE)lpChar[0];
            goto found;
        }

	/* perform translation to unicode */
	if(ret)
	{
	    TRACE_(key)("Translating char 0x%02x to unicode\n", *(BYTE *)lpChar);
            ret = ntdll_umbstowcs( lpChar, ret, bufW, bufW_size );
	}
    }

found:
    if (buf != lpChar)
        free( lpChar );

    pthread_mutex_unlock( &kbd_mutex );

    /* Null-terminate the buffer, if there's room.  MSDN clearly states that the
       caller must not assume this is done, but some programs (e.g. Audiosurf) do. */
    if (1 <= ret && ret < bufW_size)
        bufW[ret] = 0;

    TRACE_(key)("returning %d with %s\n", ret, debugstr_wn(bufW, ret));
    return ret;
}

/***********************************************************************
 *		Beep (X11DRV.@)
 */
void X11DRV_Beep(void)
{
    XBell(gdi_display, 0);
}
