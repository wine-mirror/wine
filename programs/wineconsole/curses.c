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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Known issues & FIXME:
 * - not all key mapping functions have been written
 * - allow dyn loading of curses library (extreme care should be taken for 
 *   functions which can be implemented as macros)
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_CURSES_H
#include <curses.h>
#endif
#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#endif
#include <unistd.h>
#include <winnls.h>
#include "winecon_private.h"

#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wineconsole);

#define PRIVATE(data)   ((struct inner_data_curse*)((data)->private))

#if defined(HAVE_CURSES_H) || defined(HAVE_NCURSES_H)

struct inner_data_curse 
{
    mmask_t             initial_mouse_mask;
    HANDLE              hInput;
    WINDOW*             pad;
    chtype*             line;
};

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
    PRIVATE(data)->line = HeapReAlloc(GetProcessHeap(), 0, PRIVATE(data)->line, 
                                      sizeof(chtype) * data->curcfg.sb_width);
}

/******************************************************************
 *		WCCURSES_PosCursor
 *
 * Set a new position for the cursor (and refresh any modified part of our pad)
 */
static void	WCCURSES_PosCursor(const struct inner_data* data)
{
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
    prefresh(PRIVATE(data)->pad,
             data->curcfg.win_pos.Y, data->curcfg.win_pos.X,
             0, 0, data->curcfg.win_height, data->curcfg.win_width);
}

/******************************************************************
 *		WCCURSES_ShapeCursor
 *
 * Sets a new shape for the cursor
 */
void	WCCURSES_ShapeCursor(struct inner_data* data, int size, int vis, BOOL force)
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
void	WCCURSES_ComputePositions(struct inner_data* data)
{
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
        
        WideCharToMultiByte(CP_ACP, 0, wbuf, -1, buffer, sizeof(buffer), 
                            NULL, NULL);
        fputs("\033]2;", stdout);
        fputs(buffer, stdout);
        fputc('\a', stdout);
        fflush(stdout);
    }
}

/******************************************************************
 *		Refresh
 *
 *
 */
static void WCCURSES_Refresh(const struct inner_data* data, int tp, int bm)
{
    int         x, y;
    CHAR_INFO*	cell;
    DWORD       attr;
    char        ch;

    for (y = tp; y <= bm; y++)
    {
	cell = &data->cells[y * data->curcfg.sb_width];
        for (x = 0; x < data->curcfg.sb_width; x++)
        {
            WideCharToMultiByte(CP_ACP, 0, &cell[x].Char.UnicodeChar, 1, 
                                &ch, 1, NULL, NULL);
            attr = (BYTE)ch;
            
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
    prefresh(PRIVATE(data)->pad,
             data->curcfg.win_pos.Y, data->curcfg.win_pos.X,
             0, 0, data->curcfg.win_height, data->curcfg.win_width);
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

/* Ascii -> VK, generated by calling VkKeyScanA(i) */
static int vkkeyscan_table[256] = 
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

static int mapvkey_0[256] = 
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
 *		WCCURSES_FillSimpleChar
 *
 *
 */
static unsigned WCCURSES_FillSimpleChar(INPUT_RECORD* ir, unsigned inchar)
{
     unsigned vk;
     unsigned second;
     DWORD    cks = 0;

     switch (inchar)
     {
     case 127: inchar = '\b'; break;
     case  10: inchar = '\r'; break;
     case  27:
         /* we assume that ESC & and the second character are atomically generated
          * otherwise, we'll have a race here
          */
         if ((second = wgetch(stdscr)) != ERR)
         {
             /* we got a alt-something key... */
             /* FIXME: we don't generate the keydown message for the Alt key */
             cks = LEFT_ALT_PRESSED;
             inchar = second;
         }
         break;
     }
     
     ir->EventType                        = KEY_EVENT;
     ir->Event.KeyEvent.bKeyDown          = 1;
     ir->Event.KeyEvent.wRepeatCount      = 1;
     ir->Event.KeyEvent.dwControlKeyState = cks;
     vk = vkkeyscan_table[inchar];
     if (vk & 0x0100)
	  ir->Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;
     if (vk & 0x0200 || (unsigned char)inchar <= 26)
	  ir->Event.KeyEvent.dwControlKeyState |= LEFT_CTRL_PRESSED;
     if (vk & 0x0400)
	  ir->Event.KeyEvent.dwControlKeyState |= LEFT_ALT_PRESSED;
     ir->Event.KeyEvent.wVirtualKeyCode = vk;
     ir->Event.KeyEvent.wVirtualScanCode = mapvkey_0[vk & 0x00ff]; /* VirtualKeyCodes to ScanCode */
     ir->Event.KeyEvent.uChar.UnicodeChar = (unsigned char)inchar;

     return TRUE;
}

/******************************************************************
 *		WCCURSES_FillComplexChar
 *
 *
 */
static unsigned WCCURSES_FillComplexChar(INPUT_RECORD* ir, WORD vk, WORD kc)
{
     ir->EventType			  = KEY_EVENT;
     ir->Event.KeyEvent.bKeyDown	  = 1;
     ir->Event.KeyEvent.wRepeatCount	  = 1;
     ir->Event.KeyEvent.dwControlKeyState = 0;
     
     ir->Event.KeyEvent.wVirtualScanCode  = vk;
     ir->Event.KeyEvent.wVirtualKeyCode   = kc;
     ir->Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY;
     ir->Event.KeyEvent.uChar.UnicodeChar = 0;
     
     return TRUE;
}

/******************************************************************
 *		WCCURSES_FillMouse
 *
 *
 */
static unsigned WCCURSES_FillMouse(INPUT_RECORD* ir)
{
     static	unsigned	bstate /* = 0 */;
     static	COORD 		pos /* = {0, 0} */;
     
     MEVENT	mevt;
     
     if (getmouse(&mevt) == ERR)
         return FALSE;
     
     WINE_TRACE("[%u]: (%d, %d) %08lx\n", 
                mevt.id, mevt.x, mevt.y, (unsigned long)mevt.bstate);
     
     /* macros to ease mapping ncurse button numbering to windows's one */
#define	BTN1_BIT	FROM_LEFT_1ST_BUTTON_PRESSED
#define	BTN2_BIT	RIGHTMOST_BUTTON_PRESSED
#define	BTN3_BIT	FROM_LEFT_2ND_BUTTON_PRESSED
#define	BTN4_BIT	0 /* not done yet */
     
     if (mevt.bstate & BUTTON1_PRESSED)	 bstate |= BTN1_BIT;
     if (mevt.bstate & BUTTON1_RELEASED) bstate &= ~BTN1_BIT;
     if (mevt.bstate & BUTTON2_PRESSED)	 bstate |= BTN2_BIT;
     if (mevt.bstate & BUTTON2_RELEASED) bstate &= ~BTN2_BIT;
     if (mevt.bstate & BUTTON3_PRESSED)	 bstate |= BTN3_BIT;
     if (mevt.bstate & BUTTON3_RELEASED) bstate &= ~BTN3_BIT;
     
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

     return FALSE;
}

/******************************************************************
 *		WCCURSES_FillCode
 *
 *
 */
static unsigned WCCURSES_FillCode(INPUT_RECORD* ir, int inchar)
{
    unsigned secondEvent = 0;

    switch (inchar)
    {
    case KEY_BREAK:
        goto notFound;
    case KEY_DOWN:
        secondEvent = WCCURSES_FillComplexChar(ir, 0x50, 0x28);
        break;
    case KEY_UP:
        secondEvent = WCCURSES_FillComplexChar(ir, 0x48, 0x26);
        break;
    case KEY_LEFT:
        secondEvent = WCCURSES_FillComplexChar(ir, 0x4b, 0x25);
        break;
    case KEY_RIGHT:
        secondEvent = WCCURSES_FillComplexChar(ir, 0x4d, 0x27);
        break;
    case KEY_HOME:
        secondEvent = WCCURSES_FillComplexChar(ir, 0x47, 0x24);
        break;
    case KEY_BACKSPACE:
        secondEvent = WCCURSES_FillSimpleChar(ir, '\b');
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
        secondEvent = WCCURSES_FillComplexChar(ir, 0x3b + inchar - KEY_F(1), 0);
        break;
    case KEY_F(11):
    case KEY_F(12):
        secondEvent = WCCURSES_FillComplexChar(ir, 0xd9 + inchar - KEY_F(11), 0);
        break;
		    
    case KEY_DL:
    case KEY_IL:
    case KEY_DC:
    case KEY_IC:
    case KEY_EIC:
    case KEY_CLEAR:
    case KEY_EOS:
    case KEY_EOL:
    case KEY_SF:
    case KEY_SR:
        goto notFound;
		    
    case KEY_NPAGE:
        secondEvent = WCCURSES_FillComplexChar(ir, 0x51, 0x22);
        break;
    case KEY_PPAGE:
        secondEvent = WCCURSES_FillComplexChar(ir, 0x49, 0x21);
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
    case KEY_BTAB:
    case KEY_BEG:
    case KEY_CANCEL:
    case KEY_CLOSE:
    case KEY_COMMAND:
    case KEY_COPY:
    case KEY_CREATE:
    case KEY_END:
    case KEY_EXIT:
    case KEY_FIND:
    case KEY_HELP:
    case KEY_MARK:
    case KEY_MESSAGE:
        goto notFound;
		    
    case KEY_MOUSE:
        secondEvent = WCCURSES_FillMouse(ir);
        break;
        
    case KEY_MOVE:
    case KEY_NEXT:
    case KEY_OPEN:
    case KEY_OPTIONS:
    case KEY_PREVIOUS:
    case KEY_REDO:
    case KEY_REFERENCE:
    case KEY_REFRESH:
    case KEY_REPLACE:
    case KEY_RESIZE:
    case KEY_RESTART:
    case KEY_RESUME:
    case KEY_SAVE:
    case KEY_SBEG:
    case KEY_SCANCEL:
    case KEY_SCOMMAND:
    case KEY_SCOPY:
    case KEY_SCREATE:
    case KEY_SDC:
    case KEY_SDL:
    case KEY_SELECT:
    case KEY_SEND:
    case KEY_SEOL:
    case KEY_SEXIT:
    case KEY_SFIND:
    case KEY_SHELP:
    case KEY_SHOME:
    case KEY_SIC:
    case KEY_SLEFT:
    case KEY_SMESSAGE:
    case KEY_SMOVE:
    case KEY_SNEXT:
    case KEY_SOPTIONS:
    case KEY_SPREVIOUS:
    case KEY_SPRINT:
    case KEY_SREDO:
    case KEY_SREPLACE:
    case KEY_SRIGHT:
    case KEY_SRSUME:
    case KEY_SSAVE:
    case KEY_SSUSPEND:
    case KEY_SUNDO:
    case KEY_SUSPEND:
    case KEY_UNDO:
    notFound:
        WINE_FIXME("Not done yet (%d)\n", inchar);
        break;
    default:
        WINE_ERR("Unknown val (%d)\n", inchar);
        break;
    }
    return secondEvent;
}

/******************************************************************
 *		WCCURSES_GetEvents
 *
 *
 */
static void WCCURSES_GetEvents(struct inner_data* data)
{
     int		inchar;
     INPUT_RECORD       ir[2];
     unsigned		secondEvent = 0;
     DWORD              n;

     if ((inchar = wgetch(stdscr)) == ERR) {WINE_FIXME("Ooch. somebody beat us\n");return;}

     WINE_TRACE("Got %d\n", inchar);

     ir->EventType = 0;
	  
     if (inchar & KEY_CODE_YES)
     {
         secondEvent = WCCURSES_FillCode(ir, inchar);
     }
     else
     {
         secondEvent = WCCURSES_FillSimpleChar(ir, inchar);
     }

     if (secondEvent != 0)
     {
         ir[1] = ir[0];

         switch (ir[1].EventType)
         {
         case KEY_EVENT:
             ir[1].Event.KeyEvent.bKeyDown = 0;
             break;
         case MOUSE_EVENT:
             ir[1].Event.MouseEvent.dwButtonState &= ~secondEvent;
             break;
         default:	
             WINE_FIXME("oooo\n");
             break;
         }
     }
     if (ir[0].EventType != 0)
         WriteConsoleInput(data->hConIn, ir, secondEvent ? 2 : 1, &n);
}

/******************************************************************
 *		WCCURSES_DeleteBackend
 *
 *
 */
static void WCCURSES_DeleteBackend(struct inner_data* data)
{
    mmask_t     mm;

    if (!PRIVATE(data)) return;

    CloseHandle(PRIVATE(data)->hInput);

    delwin(PRIVATE(data)->pad);
    mousemask(PRIVATE(data)->initial_mouse_mask, &mm);
    endwin();

    HeapFree(GetProcessHeap(), 0, PRIVATE(data)->line);
    HeapFree(GetProcessHeap(), 0, PRIVATE(data));
    PRIVATE(data) = NULL;
}

/******************************************************************
 *		WCCURSES_MainLoop
 *
 *
 */
static int WCCURSES_MainLoop(struct inner_data* data)
{
    HANDLE hin[2];

    hin[0] = PRIVATE(data)->hInput;
    hin[1] = data->hSynchro;

    for (;;) 
    {
        unsigned ret = WaitForMultipleObjects(2, hin, FALSE, INFINITE);
        switch (ret)
        {
        case WAIT_OBJECT_0:
            WCCURSES_GetEvents(data);
            break;
        case WAIT_OBJECT_0+1:
            if (!WINECON_GrabChanges(data)) return 0;
            break;
	default:
	    WINE_ERR("got pb\n");
	    /* err */
	    break;
        }
    }
}

/******************************************************************
 *		WCCURSES_InitBackend
 *
 * Initialisation part II: creation of window.
 *
 */
BOOL WCCURSES_InitBackend(struct inner_data* data)
{
    data->private = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct inner_data_curse));
    if (!data->private) return FALSE;

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

    if (wine_server_fd_to_handle(0, GENERIC_READ|SYNCHRONIZE, FALSE, 
                                 (obj_handle_t*)&PRIVATE(data)->hInput))
    {
        WINE_FIXME("Cannot open 0\n");
        return 0;
    }

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
    mousemask(0xffffffff, &PRIVATE(data)->initial_mouse_mask);
    /* no click event generation... we just need button up/down event */
    mouseinterval(-1);

    return TRUE;
}

#else
BOOL WCCURSES_InitBackend(struct inner_data* data)
{
    return FALSE;
}
#endif
