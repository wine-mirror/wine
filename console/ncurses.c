/* ncurses.c */
/* Copyright 1999 - Joseph Pranevich */

#include <stdio.h>
#include "config.h"
#include "console.h"

#ifdef WINE_NCURSES

/* This is the console driver for systems that support the ncurses
   interface. 
*/

/* Actually, this should work for curses, as well. But there may be
   individual functions that are unsupported in plain curses or other
   variants. Those should be detected and special-cased by autoconf. 
*/

/* When creating new drivers, you need to assign all the functions that
   that driver supports into the driver struct. If it is a supplementary
   driver, it should make sure to perserve the old values. 
*/

#include "debug.h"
#undef ERR /* Use ncurses's err() */
#ifdef HAVE_NCURSES_H
# include <ncurses.h>
#else
# ifdef HAVE_CURSES_H
#  include <curses.h>
# endif
#endif

SCREEN *ncurses_screen;

static int get_color_pair(int fg_color, int bg_color);

void NCURSES_Start()
{
   /* This should be the root driver so we can ignore anything
      already in the struct. */

   driver.norefresh = FALSE;

   driver.init = NCURSES_Init;
   driver.write = NCURSES_Write;
   driver.close = NCURSES_Close;
   driver.moveCursor = NCURSES_MoveCursor;
   driver.getCursorPosition = NCURSES_GetCursorPosition;
   driver.getCharacterAtCursor = NCURSES_GetCharacterAtCursor;
   driver.clearScreen = NCURSES_ClearScreen;
   driver.allocColor = NCURSES_AllocColor;
   driver.setBackgroundColor = NCURSES_SetBackgroundColor;
#ifdef HAVE_RESIZETERM
   driver.notifyResizeScreen = NCURSES_NotifyResizeScreen;
#endif

   driver.checkForKeystroke = NCURSES_CheckForKeystroke;
   driver.getKeystroke = NCURSES_GetKeystroke;

   driver.refresh = NCURSES_Refresh; 
}

void NCURSES_Init()
{
   ncurses_screen = newterm("xterm", driver.console_out,
      driver.console_in);
   set_term(ncurses_screen);
   start_color();
   raw();
   noecho();
   nonl();
/*   intrflush(stdscr, FALSE); */ 
   keypad(stdscr, TRUE);
   nodelay(stdscr, TRUE);
}

void NCURSES_Write(char output, int fg, int bg, int attribute)
{
   char row, col;
   int pair;
   
   if (!fg)
      fg = COLOR_WHITE; /* Default */

   if (!bg)
      bg = COLOR_BLACK; /* Default */

   pair = get_color_pair(fg, bg);

   if (waddch(stdscr, output | COLOR_PAIR(pair)) == ERR)
   {
      NCURSES_GetCursorPosition(&row, &col);
      FIXME(console, "NCURSES: waddch() failed at %d, %d.\n", row, col);
   }
}

void NCURSES_Close()
{
   endwin();
}

void NCURSES_GetKeystroke(char *scan, char *ascii)
{
   while (!NCURSES_CheckForKeystroke(scan, ascii))
   {} /* Wait until keystroke is detected */
   
   /* When it is detected, we will already have the right value 
      in scan and ascii, but we need to take this keystroke
      out of the buffer. */
   wgetch(stdscr);
}

int NCURSES_CheckForKeystroke(char *scan, char *ascii)
{
   /* We don't currently support scan codes here */
   /* FIXME */
   int temp;
   temp = wgetch(stdscr);
   if (temp == ERR)
   {
      return FALSE;
   }
   else
   {
      ungetch(temp);  /* Keystroke not removed from buffer */
      *ascii = (char) temp;
      return TRUE;
   }
}

void NCURSES_MoveCursor(char row, char col)
{
   if (wmove(stdscr, row, col) == ERR)
      FIXME(console, "NCURSES: wmove() failed to %d, %d.\n", row, col);
}

void NCURSES_GetCursorPosition(char *row, char *col)
{
   int trow, tcol;

   getyx(stdscr, trow, tcol); /* MACRO, no need to pass pointer */

   *row = (char) trow;
   *col = (char) tcol;
}

void NCURSES_GetCharacterAtCursor(char *ch, int *fg_color, int
   *bg_color, int *attribute)
{
   /* If any of the pointers are NULL, ignore them */
   /* We will eventually have to convert the color data */
   if (ch)
      *ch = (char) winch(stdscr);
   if (fg_color)
      *fg_color = WINE_WHITE;
   if (bg_color)
      *bg_color = WINE_BLACK;
   if (attribute)
      *attribute = 0;
};

void NCURSES_Refresh()
{
   wrefresh(stdscr);
}

void NCURSES_ClearScreen()
{
   werase(stdscr);
}

int NCURSES_AllocColor(int color)
{
   /* Currently support only internal colors */
   switch (color)
   {
      case WINE_BLACK:		return COLOR_BLACK;
      case WINE_WHITE:		return COLOR_WHITE;
      case WINE_RED:		return COLOR_RED;
      case WINE_GREEN:		return COLOR_GREEN;
      case WINE_YELLOW:		return COLOR_YELLOW;
      case WINE_BLUE:     	return COLOR_BLUE;
      case WINE_MAGENTA:	return COLOR_MAGENTA;
      case WINE_CYAN:		return COLOR_CYAN;
   }

   FIXME(console, "Unable to allocate color %d\n", color);

   /* Don't allocate a color... yet */
   return 0;
}

void NCURSES_SetBackgroundColor(int fg, int bg)
{
   int pair;

   pair = get_color_pair(fg, bg);

   bkgdset(COLOR_PAIR(pair));
}

#ifdef HAVE_RESIZETERM

void NCURSES_NotifyResizeScreen(int x, int y)
{
   /* Note: This function gets called *after* another driver in the chain
      calls ResizeScreen(). It is meant to resize the ncurses internal
      data structures to know about the new window dimensions. */
 
   resizeterm(y, x);
}

#endif /* HAVE_RESIZETERM */

static int get_color_pair(int fg_color, int bg_color)
{
   /* ncurses internally uses "color pairs" in addition to the "pallet" */
   /* This isn't the best way to do this. Or even close */

   static int current = 0;
   static int fg[255];     /* 16 x 16 is enough */
   static int bg[255];
   int x;

   /* The first pair is hardwired into ncurses */
   fg[0] = COLOR_WHITE;
   bg[0] = COLOR_BLACK;

   for (x = 0; x <= current; x++)
   {
      if ((fg_color == fg[x]) && (bg_color == bg[x]))
      {
         TRACE(console, "Color pair: already allocated\n");
         return x;  
      }    
   }

   /* Need to allocate new color */
   current++;
   fg[current] = fg_color;
   bg[current] = bg_color;
   TRACE(console, "Color pair: allocated.\n");
   return init_pair(current, fg_color, bg_color);
}

#endif /* WINE_NCURSES */
