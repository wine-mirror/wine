/*
 * Interface to terminfo and termcap libraries
 *
 * Copyright 2010 Eric Pouech
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

#include "config.h"
#include "wine/port.h"

#ifdef HAVE_NCURSES_H
# include <ncurses.h>
#elif defined(HAVE_CURSES_H)
# include <curses.h>
#endif
/* avoid redefinition warnings */
#undef KEY_EVENT
#undef MOUSE_MOVED

#include <term.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <wincon.h>
#include "console_private.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(console);

static const int vkkeyscan_table[256] =
{
     0,0,0,0,0,0,0,0,8,9,0,0,0,13,0,0,0,0,0,19,145,556,0,0,0,0,0,27,0,0,0,
     0,32,305,478,307,308,309,311,222,313,304,312,443,188,189,190,191,48,
     49,50,51,52,53,54,55,56,57,442,186,444,187,446,447,306,321,322,323,
     324,325,326,327,328,329,330,331,332,333,334,335,336,337,338,339,340,
     341,342,343,344,345,346,219,220,221,310,445,192,65,66,67,68,69,70,71,
     72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,475,476,477,
     448,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,400,0,0,0,0,0,0
};

static const int mapvkey_0[256] =
{
     0,0,0,0,0,0,0,0,14,15,0,0,0,28,0,0,42,29,56,69,58,0,0,0,0,0,0,1,0,0,
     0,0,57,73,81,79,71,75,72,77,80,0,0,0,55,82,83,0,11,2,3,4,5,6,7,8,9,
     10,0,0,0,0,0,0,0,30,48,46,32,18,33,34,35,23,36,37,38,50,49,24,25,16,
     19,31,20,22,47,17,45,21,44,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,55,78,0,74,
     0,53,59,60,61,62,63,64,65,66,67,68,87,88,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,69,70,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,13,51,12,52,53,41,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,26,43,27,40,76,96,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static inline void init_complex_char(INPUT_RECORD* ir, BOOL down, WORD vk, WORD kc, DWORD cks)
{
    ir->EventType			 = KEY_EVENT;
    ir->Event.KeyEvent.bKeyDown	         = down;
    ir->Event.KeyEvent.wRepeatCount	 = 1;
    ir->Event.KeyEvent.wVirtualScanCode  = vk;
    ir->Event.KeyEvent.wVirtualKeyCode   = kc;
    ir->Event.KeyEvent.dwControlKeyState = cks;
    ir->Event.KeyEvent.uChar.UnicodeChar = 0;
}

/******************************************************************
 *		TERM_FillSimpleChar
 *
 */
unsigned TERM_FillSimpleChar(unsigned real_inchar, INPUT_RECORD* ir)
{
    unsigned            vk;
    unsigned            inchar;
    char                ch;
    unsigned            numEvent = 0;
    DWORD               cks = 0;

    switch (real_inchar)
    {
    case   9: inchar = real_inchar;
        real_inchar = 27; /* so that we don't think key is ctrl- something */
        break;
    case  13:
    case  10: inchar = '\r';
        real_inchar = 27; /* Fixme: so that we don't think key is ctrl- something */
        break;
    case 127: inchar = '\b';
        break;
    default:
        inchar = real_inchar;
        break;
    }
    if ((inchar & ~0xFF) != 0) FIXME("What a char (%u)\n", inchar);
    vk = vkkeyscan_table[inchar];
    if (vk & 0x0100)
        init_complex_char(&ir[numEvent++], 1, 0x2a, 0x10, SHIFT_PRESSED);
    if ((vk & 0x0200) || (unsigned char)real_inchar <= 26)
        init_complex_char(&ir[numEvent++], 1, 0x1d, 0x11, LEFT_CTRL_PRESSED);
    if (vk & 0x0400)
        init_complex_char(&ir[numEvent++], 1, 0x38, 0x12, LEFT_ALT_PRESSED);

    ir[numEvent].EventType                        = KEY_EVENT;
    ir[numEvent].Event.KeyEvent.bKeyDown          = 1;
    ir[numEvent].Event.KeyEvent.wRepeatCount      = 1;
    ir[numEvent].Event.KeyEvent.dwControlKeyState = cks;
    if (vk & 0x0100)
        ir[numEvent].Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;
    if ((vk & 0x0200) || (unsigned char)real_inchar <= 26)
        ir[numEvent].Event.KeyEvent.dwControlKeyState |= LEFT_CTRL_PRESSED;
    if (vk & 0x0400)
        ir[numEvent].Event.KeyEvent.dwControlKeyState |= LEFT_ALT_PRESSED;
    ir[numEvent].Event.KeyEvent.wVirtualKeyCode = vk;
    ir[numEvent].Event.KeyEvent.wVirtualScanCode = mapvkey_0[vk & 0x00ff]; /* VirtualKeyCodes to ScanCode */

    ch = inchar;
    MultiByteToWideChar(CP_UNIXCP, 0, &ch, 1, &ir[numEvent].Event.KeyEvent.uChar.UnicodeChar, 1);
    ir[numEvent + 1] = ir[numEvent];
    ir[numEvent + 1].Event.KeyEvent.bKeyDown = 0;

    numEvent += 2;

    if (vk & 0x0400)
        init_complex_char(&ir[numEvent++], 0, 0x38, 0x12, LEFT_ALT_PRESSED);
    if ((vk & 0x0200) || (unsigned char)real_inchar <= 26)
        init_complex_char(&ir[numEvent++], 0, 0x1d, 0x11, 0);
    if (vk & 0x0100)
        init_complex_char(&ir[numEvent++], 0, 0x2a, 0x10, 0);
    return numEvent;
}

#if defined(SONAME_LIBCURSES) || defined(SONAME_LIBNCURSES)

#ifdef HAVE_NCURSES_H
# define CURSES_NAME "ncurses"
#else
# define CURSES_NAME "curses"
#endif

static void *nc_handle = NULL;

#define MAKE_FUNCPTR(f) static typeof(f) * p_##f;

MAKE_FUNCPTR(setupterm)

#undef MAKE_FUNCPTR

/**********************************************************************/

static BOOL TERM_bind_libcurses(void)
{
#ifdef SONAME_LIBNCURSES
    static const char ncname[] = SONAME_LIBNCURSES;
#else
    static const char ncname[] = SONAME_LIBCURSES;
#endif

    if (!(nc_handle = wine_dlopen(ncname, RTLD_NOW, NULL, 0)))
    {
        WINE_MESSAGE("Wine cannot find the " CURSES_NAME " library (%s).\n",
                     ncname);
        return FALSE;
    }

#define LOAD_FUNCPTR(f)                                      \
    if((p_##f = wine_dlsym(nc_handle, #f, NULL, 0)) == NULL) \
    {                                                        \
        WINE_WARN("Can't find symbol %s\n", #f);             \
        goto sym_not_found;                                  \
    }

    LOAD_FUNCPTR(setupterm)

#undef LOAD_FUNCPTR

    return TRUE;

sym_not_found:
    WINE_MESSAGE(
      "Wine cannot find certain functions that it needs inside the "
       CURSES_NAME "\nlibrary.  To enable Wine to use " CURSES_NAME
      " please upgrade your " CURSES_NAME "\nlibraries\n");
    wine_dlclose(nc_handle, NULL, 0);
    nc_handle = NULL;
    return FALSE;
}

#define setupterm p_setupterm

BOOL TERM_Init(void)
{
    if (!TERM_bind_libcurses()) return FALSE;
    if (setupterm(NULL, 1 /* really ?? */, NULL) == -1) return FALSE;
    return TRUE;
}

BOOL TERM_Exit(void)
{
    return TRUE;
}
#else
BOOL     TERM_Init(void) {return FALSE;}
BOOL     TERM_Exit(void) {return FALSE;}
#endif
