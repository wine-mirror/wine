/* interface.c */

/* The primary purpose of this function is to provide CONSOLE_*
   reotines that immediately call the appropiate driver handler.
   This cleans up code in the individual modules considerably.
   This could be done using a macro, but additional functionality
   may be provided here in the future. */

#include "windows.h"
#include "console.h"
#include "config.h"

void CONSOLE_Write(char out, int fg_color, int bg_color, int attribute)
{
   if (driver.write)
   {
      driver.write(out, fg_color, bg_color, attribute);
      if (!driver.norefresh)
         CONSOLE_Refresh();
   }
}

void CONSOLE_Init()
{
      /* Eventually, this will be a command-line choice */
#ifndef WINE_NCURSES
    TTY_Start(); 
#else
    NCURSES_Start();
#endif
    GENERIC_Start();

   if (driver.init)
      driver.init();
}

void CONSOLE_Close()
{
   if (driver.close)
      driver.close();
}

void CONSOLE_MoveCursor(char row, char col)
{
   if (driver.moveCursor)
   {
      driver.moveCursor(row, col);
      if (!driver.norefresh)
         CONSOLE_Refresh();
   }
}

void CONSOLE_ClearWindow(char row1, char col1, char row2, char col2, 
   int bg_color, int attribute)
{
   if (driver.clearWindow)
   {
      driver.clearWindow(row1, col1, row2, col2, bg_color, attribute);
      if (!driver.norefresh)
         CONSOLE_Refresh();
   }
}

void CONSOLE_ScrollUpWindow(char row1, char col1, char row2, char col2, 
   char lines, int bg_color, int attribute)
{
   if (driver.scrollUpWindow)
   {
      driver.scrollUpWindow(row1, col1, row2, col2, lines, bg_color, 
         attribute);
      if (!driver.norefresh)
         CONSOLE_Refresh();
   }
}

void CONSOLE_ScrollDownWindow(char row1, char col1, char row2, char col2, 
   char lines, int bg_color, int attribute)
{
   if (driver.scrollDownWindow)
   {
      driver.scrollDownWindow(row1, col1, row2, col2, lines, bg_color, 
         attribute);
      if (!driver.norefresh)
         CONSOLE_Refresh();
   }
}

int CONSOLE_CheckForKeystroke(char *scan, char *ascii)
/* These functions need to go through a conversion layer. Scancodes
   should *not* be determined by the driver, rather they should have
   a conv_* function in int16.c. Yuck. */
{
   if (driver.checkForKeystroke)
      return driver.checkForKeystroke(scan, ascii);
   else
      return FALSE;
}

void CONSOLE_GetKeystroke(char *scan, char *ascii)
{
   if (driver.getKeystroke)
      return driver.getKeystroke(scan, ascii);
}

void CONSOLE_GetCursorPosition(char *row, char *col)
{
   if (driver.getCursorPosition)
      return driver.getCursorPosition(row, col);
}

void CONSOLE_GetCharacterAtCursor(char *ch, int *fg, int *bg, int *a)
{
   if (driver.getCharacterAtCursor)
      return driver.getCharacterAtCursor(ch, fg, bg, a);
}

void CONSOLE_Refresh()
{
   if (driver.refresh)
      return driver.refresh();
}

/* This function is only at the CONSOLE level. */
/* Admittably, calling the variable norefresh might be a bit dumb...*/
void CONSOLE_SetRefresh(int setting)
{
   if (setting)
      driver.norefresh = FALSE;
   else
      driver.norefresh = TRUE;
}

/* This function is only at the CONSOLE level. */
int CONSOLE_GetRefresh()
{
   if (driver.norefresh)
      return FALSE;
   else 
      return TRUE;
}

void CONSOLE_ClearScreen()
{
   if (driver.clearScreen)
      return driver.clearScreen();
}

char CONSOLE_GetCharacter()
{
   /* I'm not sure if we need this really. This is a function that can be
      accelerated that returns the next *non extended* keystroke */
   if (driver.getCharacter)
      return driver.getCharacter();
   else
      return (char) 0; /* Sure, this will probably break programs... */
}
