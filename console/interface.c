/* interface.c */

/* The primary purpose of this function is to provide CONSOLE_*
   reotines that immediately call the appropiate driver handler.
   This cleans up code in the individual modules considerably.
   This could be done using a macro, but additional functionality
   may be provided here in the future. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "windows.h"
#include "console.h"
#include "config.h"

static int pop_driver(char **, char **, int *);

void CONSOLE_Write(char out, int fg_color, int bg_color, int attribute)
{
   if (driver.write)
   {
      driver.write(out, fg_color, bg_color, attribute);
      if (!driver.norefresh)
         CONSOLE_Refresh();
   }
}

void CONSOLE_Init(char *drivers)
{
      /* When this function is called drivers should be a string
         that consists of driver names followed by plus (+) signs
         to denote additions. 

         For example:
            drivers = tty                Load just the tty driver
            drivers = ncurses+xterm      Load ncurses then xterm

         The "default" value is just tty.
      */
      
      char *single;
      int length;
      
      /* Suitable defaults... */
      driver.console_out = stdout;
      driver.console_in = stdin;

      while (pop_driver(&drivers, &single, &length))
      {
         if (!strncmp(single, "tty", length))
            TTY_Start();
#ifdef WINE_NCURSES
         else if (!strncmp(single, "ncurses", length))
            NCURSES_Start();
#endif /* WINE_NCURSES */
         else if (!strncmp(single, "xterm", length))
            XTERM_Start();
      }

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
      driver.getKeystroke(scan, ascii);
}

void CONSOLE_GetCursorPosition(char *row, char *col)
{
   if (driver.getCursorPosition)
      driver.getCursorPosition(row, col);
}

void CONSOLE_GetCharacterAtCursor(char *ch, int *fg, int *bg, int *a)
{
   if (driver.getCharacterAtCursor)
      driver.getCharacterAtCursor(ch, fg, bg, a);
}

void CONSOLE_Refresh()
{
   if (driver.refresh)
      driver.refresh();
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
   {
      driver.clearScreen();
      if (!driver.norefresh)
         CONSOLE_Refresh();
   }
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

void CONSOLE_ResizeScreen(int x, int y)
{
   if (driver.resizeScreen)
      driver.resizeScreen(x, y);
}

void CONSOLE_NotifyResizeScreen(int x, int y)
{
   if (driver.notifyResizeScreen)
      driver.notifyResizeScreen(x, y);
}

void CONSOLE_WriteRawString(char *str)
{
   /* This is a special function that is only for internal use and 
      does not actually call any of the console drivers. It's 
      primary purpose is to provide a way for higher-level drivers
      to write directly to the underlying terminal without worry that
      there will be any retranslation done by the assorted drivers. Care
      should be taken to ensure that this only gets called when the thing
      written does not actually produce any output or a CONSOLE_Redraw()
      is called immediately afterwards.
      CONSOLE_Redraw() is not yet implemented.
   */
   fprintf(driver.console_out, "%s", str);
}

/* Utility functions... */

int pop_driver(char **drivers, char **single, int *length)
{
   /* Take the string in drivers and extract the first "driver" entry */
   /* Advance the pointer in drivers to the next entry, put the origional
      pointer in single, and put the length in length. */
   /* Return TRUE if we found one */

   if (!*drivers)
      return FALSE;

   *single = *drivers;
   *length = 0;

   while ((*drivers[0] != NULL) && (*drivers[0] != '+'))
   {
      (*drivers)++;
      (*length)++;
   }
   
   while (*drivers[0] == '+')
      (*drivers)++;

   if (*length)
      return TRUE;
   else
      return FALSE;
      
}
