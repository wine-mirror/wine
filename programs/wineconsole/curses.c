/*
 * a GUI application for displaying a console
 *	(N)Curses back end
 *
 * Copyright 2002 Eric Pouech
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

/* Known issues & FIXME:
 * - not all key mapping functions have been written
 * - allow dyn loading of curses library (extreme care should be taken for 
 *   functions which can be implemented as macros)
 * - finish buffer scrolling (mainly, need to decide of a nice way for 
 *   requesting the UP/DOWN operations
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#ifdef HAVE_POLL_H
# include <poll.h>
#endif
#ifdef HAVE_NCURSES_H
# include <ncurses.h>
#elif defined(HAVE_CURSES_H)
# include <curses.h>
#endif
/* avoid redefinition warnings */
#undef KEY_EVENT
#undef MOUSE_MOVED

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include "winecon_private.h"

#include "wine/library.h"
#include "wine/debug.h"
#undef ERR
#define ERR (-1)

WINE_DEFAULT_DEBUG_CHANNEL(curses);

#define PRIVATE(data)   ((struct inner_data_curse*)((data)->private))

#if defined(SONAME_LIBCURSES) || defined(SONAME_LIBNCURSES)

#ifdef HAVE_NCURSES_H
# define CURSES_NAME "ncurses"
#else
# define CURSES_NAME "curses"
#endif

struct inner_data_curse 
{
    unsigned long       initial_mouse_mask;
    int                 sync_pipe[2];
    HANDLE              input_thread;
    CRITICAL_SECTION    lock;
    WINDOW*             pad;
    chtype*             line;
    int                 allow_scroll;
};

static void *nc_handle = NULL;

#ifdef initscr  /* work around Solaris breakage */
#undef initscr
extern WINDOW *initscr(void);
#endif

#define MAKE_FUNCPTR(f) static typeof(f) * p_##f;

MAKE_FUNCPTR(curs_set)
MAKE_FUNCPTR(delwin)
MAKE_FUNCPTR(endwin)
#ifndef getmaxx
MAKE_FUNCPTR(getmaxx)
#endif
#ifndef getmaxy
MAKE_FUNCPTR(getmaxy)
#endif
MAKE_FUNCPTR(getmouse)
MAKE_FUNCPTR(has_colors)
MAKE_FUNCPTR(init_pair)
MAKE_FUNCPTR(initscr)
#ifndef intrflush
MAKE_FUNCPTR(intrflush)
#endif
MAKE_FUNCPTR(keypad)
MAKE_FUNCPTR(newpad)
#ifndef nodelay
MAKE_FUNCPTR(nodelay)
#endif
#ifndef noecho
MAKE_FUNCPTR(noecho)
#endif
MAKE_FUNCPTR(prefresh)
MAKE_FUNCPTR(raw)
MAKE_FUNCPTR(start_color)
MAKE_FUNCPTR(stdscr)
MAKE_FUNCPTR(waddchnstr)
MAKE_FUNCPTR(wmove)
MAKE_FUNCPTR(wgetch)
#ifdef HAVE_MOUSEMASK
MAKE_FUNCPTR(mouseinterval)
MAKE_FUNCPTR(mousemask)
#endif
MAKE_FUNCPTR(acs_map)

#undef MAKE_FUNCPTR

/**********************************************************************/

static BOOL WCCURSES_bind_libcurses(void)
{
#ifdef SONAME_LIBNCURSES
    static const char ncname[] = SONAME_LIBNCURSES;
#else
    static const char ncname[] = SONAME_LIBCURSES;
#endif

    nc_handle = wine_dlopen(ncname, RTLD_NOW, NULL, 0);
    if(!nc_handle)
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

    LOAD_FUNCPTR(curs_set)
    LOAD_FUNCPTR(delwin)
    LOAD_FUNCPTR(endwin)
#ifndef getmaxx
    LOAD_FUNCPTR(getmaxx)
#endif
#ifndef getmaxy
    LOAD_FUNCPTR(getmaxy)
#endif
    LOAD_FUNCPTR(getmouse)
    LOAD_FUNCPTR(has_colors)
    LOAD_FUNCPTR(init_pair)
    LOAD_FUNCPTR(initscr)
#ifndef intrflush
    LOAD_FUNCPTR(intrflush)
#endif
    LOAD_FUNCPTR(keypad)
    LOAD_FUNCPTR(newpad)
#ifndef nodelay
    LOAD_FUNCPTR(nodelay)
#endif
#ifndef noecho
    LOAD_FUNCPTR(noecho)
#endif
    LOAD_FUNCPTR(prefresh)
    LOAD_FUNCPTR(raw)
    LOAD_FUNCPTR(start_color)
    LOAD_FUNCPTR(stdscr)
    LOAD_FUNCPTR(waddchnstr)
    LOAD_FUNCPTR(wmove)
    LOAD_FUNCPTR(wgetch)
#ifdef HAVE_MOUSEMASK
    LOAD_FUNCPTR(mouseinterval)
    LOAD_FUNCPTR(mousemask)
#endif
    LOAD_FUNCPTR(acs_map)

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

#define curs_set p_curs_set
#define delwin p_delwin
#define endwin p_endwin
#ifndef getmaxx
#define getmaxx p_getmaxx
#endif
#ifndef getmaxy
#define getmaxy p_getmaxy
#endif
#define getmouse p_getmouse
#define has_colors p_has_colors
#define init_pair p_init_pair
#define initscr p_initscr
#ifndef intrflush
#define intrflush p_intrflush
#endif
#define keypad p_keypad
#define mouseinterval p_mouseinterval
#define mousemask p_mousemask
#define newpad p_newpad
#ifndef nodelay
#define nodelay p_nodelay
#endif
#ifndef noecho
#define noecho p_noecho
#endif
#define prefresh p_prefresh
#define raw p_raw
#define start_color p_start_color
#define stdscr (*p_stdscr)
#define waddchnstr p_waddchnstr
#define wmove p_wmove
#define wgetch p_wgetch
#define acs_map (*p_acs_map)

/******************************************************************
 *		WCCURSES_ResizeScreenBuffer
 *
 *
 */
static void WCCURSES_ResizeScreenBuffer(struct inner_data* data)
{
    /* reallocate a new pad. next event would redraw the whole pad */
    if (PRIVATE(data)->pad) delwin(PRIVATE(data)->pad);
    PRIVATE(data)->pad = newpad(data->curcfg.sb_height, data->curcfg.sb_width);
    if (!PRIVATE(data)->pad)
        WINE_FIXME("Cannot create pad\n");
    if (PRIVATE(data)->line) 
	PRIVATE(data)->line = HeapReAlloc(GetProcessHeap(), 0, PRIVATE(data)->line, 
                                      sizeof(chtype) * data->curcfg.sb_width);
    else 
	PRIVATE(data)->line = HeapAlloc(GetProcessHeap(), 0, 
                                      sizeof(chtype) * data->curcfg.sb_width);
}

/******************************************************************
 *		WCCURSES_PosCursor
 *
 * Set a new position for the cursor (and refresh any modified part of our pad)
 */
static void	WCCURSES_PosCursor(const struct inner_data* data)
{
    int scr_width;
    int scr_height;

    if (data->curcfg.cursor_visible &&
        data->cursor.Y >= data->curcfg.win_pos.Y &&
        data->cursor.Y < data->curcfg.win_pos.Y + data->curcfg.win_height &&
        data->cursor.X >= data->curcfg.win_pos.X &&
        data->cursor.X < data->curcfg.win_pos.X + data->curcfg.win_width)
    {
        if (curs_set(2) == ERR) curs_set(1);
        wmove(PRIVATE(data)->pad, data->cursor.Y, data->cursor.X);
    }
    else
    {
        curs_set(0);
    }
    getmaxyx(stdscr, scr_height, scr_width); 
    prefresh(PRIVATE(data)->pad,
             data->curcfg.win_pos.Y, data->curcfg.win_pos.X,
             0, 0, 
             min(scr_height, data->curcfg.win_height) - 1, 
             min(scr_width, data->curcfg.win_width) - 1);
}

/******************************************************************
 *		WCCURSES_ShapeCursor
 *
 * Sets a new shape for the cursor
 */
static void	WCCURSES_ShapeCursor(struct inner_data* data, int size, int vis, BOOL force)
{
    /* we can't do much about the size... */
    data->curcfg.cursor_size = size;
    data->curcfg.cursor_visible = vis ? TRUE : FALSE;
    WCCURSES_PosCursor(data);
}

/******************************************************************
 *		WCCURSES_ComputePositions
 *
 * Recomputes all the components (mainly scroll bars) positions
 */
static void	WCCURSES_ComputePositions(struct inner_data* data)
{
    int         x, y;

    getmaxyx(stdscr, y, x);
    if ((data->curcfg.win_height && y < data->curcfg.win_height) ||
        (data->curcfg.win_width && x < data->curcfg.win_width))
    {
        SMALL_RECT  pos;

        WINE_WARN("Window too large (%dx%d), adjusting to curses' size (%dx%d)\n",
                  data->curcfg.win_width, data->curcfg.win_height, x, y);
        pos.Left = pos.Top = 0;
        pos.Right = x - 1; pos.Bottom = y - 1;
        SetConsoleWindowInfo(data->hConOut, FALSE, &pos);
        return; /* we'll get called again upon event for new window size */
    }
    if (PRIVATE(data)->pad) WCCURSES_PosCursor(data);
}

/******************************************************************
 *		WCCURSES_SetTitle
 *
 * Sets the title to the wine console
 */
static void	WCCURSES_SetTitle(const struct inner_data* data)
{
    WCHAR   wbuf[256];

    if (WINECON_GetConsoleTitle(data->hConIn, wbuf, sizeof(wbuf)/sizeof(WCHAR)))
    {
        char        buffer[256];

        WideCharToMultiByte(CP_UNIXCP, 0, wbuf, -1, buffer, sizeof(buffer),
                            NULL, NULL);
        fputs("\033]2;", stdout);
        fputs(buffer, stdout);
        fputc('\a', stdout);
        fflush(stdout);
    }
}

/******************************************************************
 *		WCCURSES_Refresh
 *
 *
 */
static void WCCURSES_Refresh(const struct inner_data* data, int tp, int bm)
{
    unsigned int x;
    int         y;
    CHAR_INFO*	cell;
    DWORD       attr;

    for (y = tp; y <= bm; y++)
    {
	cell = &data->cells[y * data->curcfg.sb_width];
        for (x = 0; x < data->curcfg.sb_width; x++)
        {
            /* check for some mapping to ACS characters (drawing boxes, arrows) */
            if ((cell[x].Char.UnicodeChar >= 0x2500 && cell[x].Char.UnicodeChar <= 0x257F) ||
                (cell[x].Char.UnicodeChar >= 0x2190 && cell[x].Char.UnicodeChar <= 0x21FF))
            {
                /* FIXME: we're also mapping heavy and lines item to single lines
                 * (that's ugly, but that's better than crap)
                 * Moreover, as the ACS_ macros refer to values in array acs_map[], we
                 * cannot simply build static tables for the mapping (FIXME: this could be done
                 * at load time)
                 */
                switch (cell[x].Char.UnicodeChar)
                {
                case 0x2190: case 0x219E: case 0x21A2: case 0x21A4:
                case 0x21BC: case 0x21BD: case 0x21D0: case 0x21E6: attr = ACS_LARROW;   break;
                case 0x2191: case 0x219F: case 0x21A3: case 0x21A5:
                case 0x21BE: case 0x21BF: case 0x21D1: case 0x21E7: attr = ACS_UARROW;   break;
                case 0x2192: case 0x21A0: case 0x21A6: case 0x21C0:
                case 0x21C1: case 0x21D2: case 0x21E8:              attr = ACS_RARROW;   break;
                case 0x2193: case 0x21A1: case 0x21A7: case 0x21C2:
                case 0x21C3: case 0x21D3: case 0x21E9:              attr = ACS_DARROW;   break;

                case 0x2500: case 0x2501: case 0x257C: case 0x257E: attr = ACS_HLINE;    break;
                case 0x2502: case 0x2503: case 0x257D: case 0x257F: attr = ACS_VLINE;    break;
                case 0x250C: case 0x250D: case 0x250E: case 0x250F: attr = ACS_ULCORNER; break;
                case 0x2510: case 0x2511: case 0x2512: case 0x2513: attr = ACS_URCORNER; break;
                case 0x2514: case 0x2515: case 0x2516: case 0x2517: attr = ACS_LLCORNER; break;
                case 0x2518: case 0x2519: case 0x251A: case 0x251B: attr = ACS_LRCORNER; break;
                case 0x251C: case 0x251D: case 0x251E: case 0x251F:
                case 0x2520: case 0x2521: case 0x2522: case 0x2523: attr = ACS_LTEE;     break;
                case 0x2524: case 0x2525: case 0x2526: case 0x2527:
                case 0x2528: case 0x2529: case 0x252A: case 0x252B: attr = ACS_RTEE;     break;

                case 0x252C: case 0x252D: case 0x252E: case 0x252F:
                case 0x2530: case 0x2531: case 0x2532: case 0x2533: attr = ACS_TTEE;     break;
                case 0x2534: case 0x2535: case 0x2536: case 0x2537:
                case 0x2538: case 0x2539: case 0x253A: case 0x253B: attr = ACS_BTEE;     break;

                case 0x253C: case 0x253D: case 0x253E: case 0x253F:
                case 0x2540: case 0x2541: case 0x2542: case 0x2543:
                case 0x2544: case 0x2545: case 0x2546: case 0x2547:
                case 0x2548: case 0x2549: case 0x254A: case 0x254B: attr = ACS_PLUS;     break;

                case 0x2550:                                        attr = ACS_HLINE;    break;
                case 0x2551:                                        attr = ACS_VLINE;    break;
                case 0x2552: case 0x2553: case 0x2554:              attr = ACS_ULCORNER; break;
                case 0x2555: case 0x2556: case 0x2557:              attr = ACS_URCORNER; break;
                case 0x2558: case 0x2559: case 0x255A:              attr = ACS_LLCORNER; break;
                case 0x255B: case 0x255C: case 0x255D:              attr = ACS_LRCORNER; break;
                case 0x255E: case 0x255F: case 0x2560:              attr = ACS_LTEE;     break;
                case 0x2561: case 0x2562: case 0x2563:              attr = ACS_RTEE;     break;
                case 0x2564: case 0x2565: case 0x2566:              attr = ACS_TTEE;     break;
                case 0x2567: case 0x2568: case 0x2569:              attr = ACS_BTEE;     break;
                case 0x256A: case 0x256B: case 0x256C:              attr = ACS_PLUS;     break;
                default:
                    WINE_FIXME("Unmapped special character (%x)\n", cell[x].Char.UnicodeChar);
                    attr = ' ';
                }
            }
            else
            {
                char     ch[2];

                if (WideCharToMultiByte(CP_UNIXCP, 0, &cell[x].Char.UnicodeChar, 1,
                                        ch, sizeof(ch), NULL, NULL) == 1)
                    attr = ((BYTE)ch[0] < 32) ? 32 : (BYTE)ch[0];
                else
                    attr = 32;
            }

            if (cell[x].Attributes & FOREGROUND_RED)       attr |= COLOR_PAIR(COLOR_RED);
            if (cell[x].Attributes & FOREGROUND_BLUE)      attr |= COLOR_PAIR(COLOR_BLUE);
            if (cell[x].Attributes & FOREGROUND_GREEN)     attr |= COLOR_PAIR(COLOR_GREEN);
            if (cell[x].Attributes & BACKGROUND_RED)       attr |= COLOR_PAIR(COLOR_RED << 3);
            if (cell[x].Attributes & BACKGROUND_BLUE)      attr |= COLOR_PAIR(COLOR_BLUE << 3);
            if (cell[x].Attributes & BACKGROUND_GREEN)     attr |= COLOR_PAIR(COLOR_GREEN << 3);

            if (cell[x].Attributes & FOREGROUND_INTENSITY) attr |= A_BOLD;
            PRIVATE(data)->line[x] = attr;
        }
        mvwaddchnstr(PRIVATE(data)->pad, y, 0, PRIVATE(data)->line, data->curcfg.sb_width);
    }

    WCCURSES_PosCursor(data);
}

/******************************************************************
 *		WCCURSES_Scroll
 *
 *
 */
static void WCCURSES_Scroll(struct inner_data* data, int pos, BOOL horz)
{
    if (horz)
    {
	data->curcfg.win_pos.X = pos;
    }
    else
    {
	data->curcfg.win_pos.Y = pos;
    }
    WCCURSES_PosCursor(data);
}

/******************************************************************
 *		WCCURSES_SetFont
 *
 *
 */
static void WCCURSES_SetFont(struct inner_data* data, const WCHAR* font, 
                            unsigned height, unsigned weight)
{
    /* FIXME: really not much to do ? */
}

/******************************************************************
 *		WCCURSES_Resize
 *
 */
static void WCCURSES_Resize(struct inner_data* data)
{
    int width, height;

    getmaxyx(stdscr, height, width);
    WINECON_ResizeWithContainer(data, width, height);
}

/******************************************************************
 *		WCCURSES_ScrollV
 *
 *
 */
static void WCCURSES_ScrollV(struct inner_data* data, int delta)
{
    struct config_data  cfg = data->curcfg;

    cfg.win_pos.Y += delta;
    if (cfg.win_pos.Y < 0) cfg.win_pos.Y = 0;
    if (cfg.win_pos.Y > data->curcfg.sb_height - data->curcfg.win_height)
        cfg.win_pos.Y = data->curcfg.sb_height - data->curcfg.win_height;
    if (cfg.win_pos.Y != data->curcfg.win_pos.Y)
    {
        WCCURSES_PosCursor(data);
        WINECON_SetConfig(data, &cfg);
    }
}

/* Ascii -> VK, generated by calling VkKeyScanA(i) */
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

/******************************************************************
 *		WCCURSES_InitComplexChar
 *
 *
 */
static inline void WCCURSES_InitComplexChar(INPUT_RECORD* ir, BOOL down, WORD vk, WORD kc, DWORD cks)
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
 *		WCCURSES_FillSimpleChar
 *
 *
 */
static unsigned WCCURSES_FillSimpleChar(INPUT_RECORD* ir, unsigned real_inchar)
{
    unsigned vk;
    unsigned inchar;
    char ch;
    unsigned numEvent = 0;
    DWORD    cks = 0;

    switch (real_inchar)
    {
    case   9: inchar = real_inchar;
              real_inchar = 27; /* so that we don't think key is ctrl- something */ 	
	      break;
    case  10: inchar = '\r'; 
              real_inchar = 27; /* Fixme: so that we don't think key is ctrl- something */ 
	      break;
    case 127: inchar = '\b'; 
	      break;
    case  27:
        /* we assume that ESC & and the second character are atomically
         * generated otherwise, we'll have a race here. FIXME: This gives 1 sec. delay
         * because curses looks for a second character.
         */
        if ((inchar = wgetch(stdscr)) != ERR)
        {
            /* we got a alt-something key... */
            cks = LEFT_ALT_PRESSED;
        }
        else
            inchar = 27;
        break;
    default:
        inchar = real_inchar;
        break;
    }
    if ((inchar & ~0xFF) != 0) WINE_FIXME("What a char (%u)\n", inchar);
    vk = vkkeyscan_table[inchar];
    if (vk & 0x0100)
        WCCURSES_InitComplexChar(&ir[numEvent++], 1, 0x2a, 0x10, SHIFT_PRESSED);
    if ((vk & 0x0200) || (unsigned char)real_inchar <= 26)
        WCCURSES_InitComplexChar(&ir[numEvent++], 1, 0x1d, 0x11, LEFT_CTRL_PRESSED);
    if (vk & 0x0400)
        WCCURSES_InitComplexChar(&ir[numEvent++], 1, 0x38, 0x12, LEFT_ALT_PRESSED);

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
    MultiByteToWideChar(CP_UNIXCP, 0,&ch,1,&ir[numEvent].Event.KeyEvent.uChar.UnicodeChar, 1);
    ir[numEvent + 1] = ir[numEvent];
    ir[numEvent + 1].Event.KeyEvent.bKeyDown      = 0;

    numEvent += 2;

    if (vk & 0x0400)
        WCCURSES_InitComplexChar(&ir[numEvent++], 0, 0x38, 0x12, LEFT_ALT_PRESSED);
    if ((vk & 0x0200) || (unsigned char)real_inchar <= 26)
        WCCURSES_InitComplexChar(&ir[numEvent++], 0, 0x1d, 0x11, 0);
    if (vk & 0x0100)
        WCCURSES_InitComplexChar(&ir[numEvent++], 0, 0x2a, 0x10, 0);

    return numEvent;
}

/******************************************************************
 *		WCCURSES_FillComplexChar
 *
 *
 */
static unsigned WCCURSES_FillComplexChar(INPUT_RECORD* ir, WORD vk, WORD kc, DWORD cks)
{
    WCCURSES_InitComplexChar(&ir[0], 1, vk, kc, ENHANCED_KEY | cks);
    WCCURSES_InitComplexChar(&ir[1], 0, vk, kc, ENHANCED_KEY | cks);

    return 2;
}

/******************************************************************
 *		WCCURSES_FillMouse
 *
 *
 */
static unsigned WCCURSES_FillMouse(INPUT_RECORD* ir)
{
#ifdef HAVE_MOUSEMASK
    static	unsigned	bstate /* = 0 */;
    static	COORD 		pos /* = {0, 0} */;

    MEVENT	mevt;

    if (getmouse(&mevt) == ERR)
        return 0;

    WINE_TRACE("[%u]: (%d, %d) %08lx\n", 
               mevt.id, mevt.x, mevt.y, (unsigned long)mevt.bstate);

    /* macros to ease mapping ncurses button numbering to windows' one */
#define	BTN1_BIT	FROM_LEFT_1ST_BUTTON_PRESSED
#define	BTN2_BIT	RIGHTMOST_BUTTON_PRESSED
#define	BTN3_BIT	FROM_LEFT_2ND_BUTTON_PRESSED
#define	BTN4_BIT	0 /* not done yet */

    if (mevt.bstate & BUTTON1_PRESSED)	 bstate |= BTN1_BIT;
    if (mevt.bstate & BUTTON1_RELEASED)  bstate &= ~BTN1_BIT;
    if (mevt.bstate & BUTTON2_PRESSED)	 bstate |= BTN2_BIT;
    if (mevt.bstate & BUTTON2_RELEASED)  bstate &= ~BTN2_BIT;
    if (mevt.bstate & BUTTON3_PRESSED)	 bstate |= BTN3_BIT;
    if (mevt.bstate & BUTTON3_RELEASED)  bstate &= ~BTN3_BIT;

    ir->EventType = MOUSE_EVENT;
    ir->Event.MouseEvent.dwMousePosition.X = mevt.x;
    ir->Event.MouseEvent.dwMousePosition.Y = mevt.y;

    ir->Event.MouseEvent.dwButtonState = bstate;

    /* partial conversion */
    ir->Event.MouseEvent.dwControlKeyState = 0;
    if (mevt.bstate & BUTTON_SHIFT)	ir->Event.MouseEvent.dwControlKeyState |= SHIFT_PRESSED;
    /* choose to map to left ctrl... could use both ? */
    if (mevt.bstate & BUTTON_CTRL)	ir->Event.MouseEvent.dwControlKeyState |= LEFT_CTRL_PRESSED;
    /* choose to map to left alt... could use both ? */
    if (mevt.bstate & BUTTON_ALT)	ir->Event.MouseEvent.dwControlKeyState |= LEFT_ALT_PRESSED;
    /* FIXME: unsupported yet flags: CAPSLOCK_ON, ENHANCED_KEY (??), NUMLOCK_ON, SCROLLLOCK_ON 
     * could be reported from the key events...
     */

    ir->Event.MouseEvent.dwEventFlags = 0;
    /* FIXME: we no longer generate double click events */

    if (!(mevt.bstate & (BUTTON1_PRESSED|BUTTON1_RELEASED|BUTTON2_PRESSED|BUTTON2_RELEASED|BUTTON3_PRESSED|BUTTON3_RELEASED)) &&
        (mevt.x != pos.X || mevt.y != pos.Y))
    {
        ir->Event.MouseEvent.dwEventFlags |= MOUSE_MOVED;
    }
    pos.X = mevt.x; pos.Y = mevt.y;

    return 1;
#else
    return 0;
#endif
}

/******************************************************************
 *		WCCURSES_FillCode
 *
 *
 */
static unsigned WCCURSES_FillCode(struct inner_data* data, INPUT_RECORD* ir, int inchar)
{
    unsigned numEvent = 0;
    
    switch (inchar)
    {
    case KEY_BREAK:
        goto notFound;
    case KEY_DOWN:
        numEvent = WCCURSES_FillComplexChar(ir, 0x50, 0x28, 0);
        break;
    case KEY_UP:
        numEvent = WCCURSES_FillComplexChar(ir, 0x48, 0x26, 0);
        break;
    case KEY_LEFT:
        numEvent = WCCURSES_FillComplexChar(ir, 0x4b, 0x25, 0);
        break;
    case KEY_RIGHT:
        numEvent = WCCURSES_FillComplexChar(ir, 0x4d, 0x27, 0);
        break;
    case KEY_HOME:
        numEvent = WCCURSES_FillComplexChar(ir, 0x47, 0x24, 0);
        break;
    case KEY_BACKSPACE:
        numEvent = WCCURSES_FillSimpleChar(ir, 127);
        break;
        
    case KEY_F0: /* up to F63 */
        goto notFound;
		    
    case KEY_F( 1):
    case KEY_F( 2):
    case KEY_F( 3):
    case KEY_F( 4):
    case KEY_F( 5):
    case KEY_F( 6):
    case KEY_F( 7):
    case KEY_F( 8):
    case KEY_F( 9):
    case KEY_F(10):
        numEvent = WCCURSES_FillComplexChar(ir, 0x3b + inchar - KEY_F(1),
                                            0x70 + inchar - KEY_F(1), 0);
        break;
    case KEY_F(11):
    case KEY_F(12):
        if (PRIVATE(data)->allow_scroll)
        {
            WCCURSES_ScrollV(data, inchar == KEY_F(11) ? 8 : -8);
        }
        else
        {
            numEvent = WCCURSES_FillComplexChar(ir, 0xd9 + inchar - KEY_F(11),
                                                0x7a + inchar - KEY_F(11), 0);
        }
        break;
		    
    case KEY_DL:
    case KEY_IL:
        goto notFound;

    case KEY_DC:
        numEvent = WCCURSES_FillComplexChar(ir, 0x53, 0x2e, 0);
        break;
    case KEY_IC:
        numEvent = WCCURSES_FillComplexChar(ir, 0x52, 0x2d, 0);
        break;

    case KEY_EIC:
    case KEY_CLEAR:
    case KEY_EOS:
    case KEY_EOL:
    case KEY_SF:
    case KEY_SR:
        goto notFound;
		    
    case KEY_NPAGE:
        numEvent = WCCURSES_FillComplexChar(ir, 0x51, 0x22, 0);
        break;
    case KEY_PPAGE:
        numEvent = WCCURSES_FillComplexChar(ir, 0x49, 0x21, 0);
        break;
        
    case KEY_STAB:
    case KEY_CTAB:
    case KEY_CATAB:
    case KEY_ENTER:
    case KEY_SRESET:
    case KEY_RESET:
    case KEY_PRINT:
    case KEY_LL:
    case KEY_A1:
    case KEY_A3:
    case KEY_B2:
    case KEY_C1:
    case KEY_C3:
        goto notFound;
    case KEY_BTAB:      /* shift tab */
        numEvent = WCCURSES_FillSimpleChar(ir, 0x9);
	ir[0].Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;
	ir[1].Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;	
	if (numEvent != 2) WINE_ERR("FillsimpleChar has changed\n");
	break;

    case KEY_BEG:
    case KEY_CANCEL:
    case KEY_CLOSE:
    case KEY_COMMAND:
    case KEY_COPY:
    case KEY_CREATE:
        goto notFound;

    case KEY_END:
        numEvent = WCCURSES_FillComplexChar(ir, 0x4f, 0x23, 0);
        break;
        
    case KEY_EXIT:
    case KEY_FIND:
    case KEY_HELP:
    case KEY_MARK:
    case KEY_MESSAGE:
        goto notFound;
		    
    case KEY_MOUSE:
        numEvent = WCCURSES_FillMouse(ir);
        break;
#ifdef KEY_RESIZE
    case KEY_RESIZE:
        WCCURSES_Resize(data);
        break;
#endif

    case KEY_MOVE:
    case KEY_NEXT:
    case KEY_OPEN:
    case KEY_OPTIONS:
    case KEY_PREVIOUS:
    case KEY_REDO:
    case KEY_REFERENCE:
    case KEY_REFRESH:
    case KEY_REPLACE:
    case KEY_RESTART:
    case KEY_RESUME:
    case KEY_SAVE:
    case KEY_SBEG:
    case KEY_SCANCEL:
    case KEY_SCOMMAND:
    case KEY_SCOPY:
    case KEY_SCREATE:
        goto notFound;

    case KEY_SDC:
        numEvent = WCCURSES_FillComplexChar(ir, 0x53, 0x2e, SHIFT_PRESSED);
        break;
    case KEY_SDL:
    case KEY_SELECT:
        goto notFound;

    case KEY_SEND:
        numEvent = WCCURSES_FillComplexChar(ir, 0x4f, 0x23, SHIFT_PRESSED);
        break;

    case KEY_SEOL:
    case KEY_SEXIT:
    case KEY_SFIND:
    case KEY_SHELP:
        goto notFound;

    case KEY_SHOME:
        numEvent = WCCURSES_FillComplexChar(ir, 0x47, 0x24, SHIFT_PRESSED);
        break;
    case KEY_SIC:
        numEvent = WCCURSES_FillComplexChar(ir, 0x52, 0x2d, SHIFT_PRESSED);
        break;
    case KEY_SLEFT:
        numEvent = WCCURSES_FillComplexChar(ir, 0x4b, 0x25, SHIFT_PRESSED);
        break;

    case KEY_SMESSAGE:
    case KEY_SMOVE:
    case KEY_SNEXT:
    case KEY_SOPTIONS:
    case KEY_SPREVIOUS:
    case KEY_SPRINT:
    case KEY_SREDO:
    case KEY_SREPLACE:
        goto notFound;

    case KEY_SRIGHT:
        numEvent = WCCURSES_FillComplexChar(ir, 0x4d, 0x27, SHIFT_PRESSED);
        break;

    case KEY_SRSUME:
    case KEY_SSAVE:
    case KEY_SSUSPEND:
    case KEY_SUNDO:
    case KEY_SUSPEND:
    case KEY_UNDO:
    notFound:
        WINE_FIXME("Not done yet (%o)\n", inchar);
        break;
    default:
        WINE_ERR("Unknown val (%o)\n", inchar);
        break;
    }
    return numEvent;
}

/******************************************************************
 *		input_thread
 */
static DWORD CALLBACK input_thread( void *arg )
{
    struct inner_data* data = arg;
    int		        inchar;
    INPUT_RECORD        ir[8];
    unsigned		numEvent;
    DWORD               n;
    struct pollfd pfd[2];

    pfd[0].fd = 0;
    pfd[0].events = POLLIN;
    pfd[1].fd = PRIVATE(data)->sync_pipe[0];
    pfd[1].events = POLLHUP;

    for (;;)
    {
        pfd[0].revents = pfd[1].revents = 0;
        if (poll( pfd, 2, -1 ) == -1) break;
        if (pfd[0].revents & (POLLHUP|POLLERR)) break;
        if (pfd[1].revents & (POLLHUP|POLLERR)) break;
        if (!(pfd[0].revents & POLLIN)) continue;

        /* we're called from input thread (not main thread), so force unique access */
        EnterCriticalSection(&PRIVATE(data)->lock);
        if ((inchar = wgetch(stdscr)) != ERR)
        {
            WINE_TRACE("Got o%o (0x%x)\n", inchar,inchar);

            if (inchar >= KEY_MIN && inchar <= KEY_MAX)
                numEvent = WCCURSES_FillCode(data, ir, inchar);
            else
                numEvent = WCCURSES_FillSimpleChar(ir, inchar);

            if (numEvent) WriteConsoleInputW(data->hConIn, ir, numEvent, &n);
        }
        LeaveCriticalSection(&PRIVATE(data)->lock);
    }
    close( PRIVATE(data)->sync_pipe[0] );
    return 0;
}

/******************************************************************
 *		WCCURSES_DeleteBackend
 *
 *
 */
static void WCCURSES_DeleteBackend(struct inner_data* data)
{
    if (!PRIVATE(data)) return;

    if (PRIVATE(data)->input_thread)
    {
        close( PRIVATE(data)->sync_pipe[1] );
        WaitForSingleObject( PRIVATE(data)->input_thread, INFINITE );
        CloseHandle( PRIVATE(data)->input_thread );
    }
    PRIVATE(data)->lock.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&PRIVATE(data)->lock);

    delwin(PRIVATE(data)->pad);
#ifdef HAVE_MOUSEMASK
    {
        mmask_t mm;
        mousemask(PRIVATE(data)->initial_mouse_mask, &mm);
    }
#endif
    endwin();

    HeapFree(GetProcessHeap(), 0, PRIVATE(data)->line);
    HeapFree(GetProcessHeap(), 0, PRIVATE(data));
    data->private = NULL;
}

/******************************************************************
 *		WCCURSES_MainLoop
 *
 *
 */
static int WCCURSES_MainLoop(struct inner_data* data)
{
    DWORD       id;

    WCCURSES_Resize(data);

    if (pipe( PRIVATE(data)->sync_pipe ) == -1) return 0;
    PRIVATE(data)->input_thread = CreateThread( NULL, 0, input_thread, data, 0, &id );

    while (!data->dying && WaitForSingleObject(data->hSynchro, INFINITE) == WAIT_OBJECT_0)
    {
        EnterCriticalSection(&PRIVATE(data)->lock);
        WINECON_GrabChanges(data);
        LeaveCriticalSection(&PRIVATE(data)->lock);
    }

    close( PRIVATE(data)->sync_pipe[1] );
    WaitForSingleObject( PRIVATE(data)->input_thread, INFINITE );
    CloseHandle( PRIVATE(data)->input_thread );
    PRIVATE(data)->input_thread = 0;
    return 0;
}

/******************************************************************
 *		WCCURSES_InitBackend
 *
 * Initialisation part II: creation of window.
 *
 */
enum init_return WCCURSES_InitBackend(struct inner_data* data)
{
    if( !WCCURSES_bind_libcurses() )
        return init_not_supported;

    data->private = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct inner_data_curse));
    if (!data->private) return init_failed;

    data->fnMainLoop           = WCCURSES_MainLoop;
    data->fnPosCursor          = WCCURSES_PosCursor;
    data->fnShapeCursor        = WCCURSES_ShapeCursor;
    data->fnComputePositions   = WCCURSES_ComputePositions;
    data->fnRefresh            = WCCURSES_Refresh;
    data->fnResizeScreenBuffer = WCCURSES_ResizeScreenBuffer;
    data->fnSetTitle           = WCCURSES_SetTitle;
    data->fnScroll             = WCCURSES_Scroll;
    data->fnSetFont            = WCCURSES_SetFont;
    data->fnDeleteBackend      = WCCURSES_DeleteBackend;
    data->hWnd                 = NULL;

    /* FIXME: should find a good way to enable buffer scrolling
     * For the time being, setting this to 1 will allow scrolling up/down 
     * on buffer with F11/F12.
     */
    /* PRIVATE(data)->allow_scroll = 1; */

    initscr();

    /* creating the basic colors - FIXME intensity not handled yet */
    if (has_colors())
    {
        int i, j;

        start_color();
        for (i = 0; i < 8; i++)
            for (j = 0; j < 8; j++)
                init_pair(i | (j << 3), i, j);
    }

    raw();
    noecho();
    intrflush(stdscr, FALSE);
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
#ifdef HAVE_MOUSEMASK
    if (data->curcfg.quick_edit)
    {
        mmask_t mm;
        mousemask(BUTTON1_PRESSED|BUTTON1_RELEASED|
                  BUTTON2_PRESSED|BUTTON2_RELEASED|
                  BUTTON3_PRESSED|BUTTON3_RELEASED|
                  BUTTON_SHIFT|BUTTON_CTRL|BUTTON_ALT|REPORT_MOUSE_POSITION,
                  &mm);
        /* no click event generation... we just need button up/down events
         * it doesn't seem that mouseinterval(-1) behaves as documented... 
         * 0 seems to be better value to disable click event generation
         */
        mouseinterval(0);
        PRIVATE(data)->initial_mouse_mask = mm;
    }
    else
    {
        mmask_t mm;
        mousemask(0, &mm);
        PRIVATE(data)->initial_mouse_mask = mm;
    }
#endif
    InitializeCriticalSection(&PRIVATE(data)->lock);
    PRIVATE(data)->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": curses");

    return init_success;
}

#else
enum init_return WCCURSES_InitBackend(struct inner_data* data)
{
    WINE_ERR("(n)curses was not found at configuration time.\n"
             "If you want (n)curses support, please install relevant packages.\n");
    return init_not_supported;
}
#endif
