/* ncurses.c */

#include "config.h"

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

#include "console.h"
#include "debug.h"
#undef ERR /* Use ncurses's err() */
#include <curses.h>

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

   driver.checkForKeystroke = NCURSES_CheckForKeystroke;
   driver.getKeystroke = NCURSES_GetKeystroke;

   driver.refresh = NCURSES_Refresh; 
}

void NCURSES_Init()
{
   initscr();
   cbreak();
   noecho();
   nonl();
   intrflush(stdscr, FALSE);
   keypad(stdscr, TRUE);
   nodelay(stdscr, TRUE);
}

void NCURSES_Write(char output, int fg, int bg, int attribute)
{
   /* We can discard all extended information. */
   addch(output);
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
   getch();
}

int NCURSES_CheckForKeystroke(char *scan, char *ascii)
{
   /* We don't currently support scan codes here */
   /* FIXME */
   int temp;
   temp = getch();
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
   move(row, col);
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
   /* We will eventually have to convert the color data */
   *ch = (char) inch();
   *fg_color = 0;
   *bg_color = 0;
   *attribute = 0;
};

void NCURSES_Refresh()
{
   refresh();
}

void NCURSES_ClearScreen()
{
   erase();
}

#endif /* WINE_NCURSES */
