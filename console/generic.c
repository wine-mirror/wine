/* generic.c */

/* This is a driver to implement, when possible, "high-level"
   routines using only low level calls. This is to make it possible
   to have accelerated functions for the individual drivers...
   or to simply not bother with them. */

/* When creating new drivers, you need to assign all the functions that
   that driver supports into the driver struct. If it is a supplementary
   driver, it should make sure to perserve the old values. */

#include <stdio.h>
#include "console.h"
#include "config.h"
#include "debug.h"

void GENERIC_Start()
{
   /* Here, we only want to add a driver if there is not one already
      defined. */

   TRACE(console, "GENERIC_Start\n");

   if (!driver.clearWindow)
      driver.clearWindow = GENERIC_ClearWindow;

   if (!driver.scrollUpWindow)
      driver.scrollUpWindow = GENERIC_ScrollUpWindow;

   if (!driver.scrollDownWindow)
      driver.scrollDownWindow = GENERIC_ScrollDownWindow;

   if (!driver.getCharacter)
      driver.getCharacter = GENERIC_GetCharacter;
}

void GENERIC_ClearWindow(char row1, char col1, char row2, char col2, 
   int bg_color, int attribute)
{
   char trow, tcol;
   char x, y;
   int old_refresh;

   TRACE(console, "GENERIC_ClearWindow()\n");
   /* Abort if we have only partial functionality */
   if (!(driver.getCursorPosition && driver.moveCursor && driver.write))
      return;

   old_refresh = CONSOLE_GetRefresh();
   CONSOLE_SetRefresh(FALSE);

   CONSOLE_GetCursorPosition(&trow, &tcol);
   
   for (x = row1; x < row2; x++)
   {
      CONSOLE_MoveCursor(x, col1);

      for (y = col1; y < col2; y++)
      {
         CONSOLE_Write(' ', 0, bg_color, attribute);
      }
   }
   CONSOLE_MoveCursor(trow, tcol);

   CONSOLE_SetRefresh(old_refresh);

   TRACE(console, "GENERIC_ClearWindow() completed.\n");
}      

/* These are in-progress. I just haven't finished them yet... */
void GENERIC_ScrollUpWindow(char row1, char col1, char row2, char col2, 
   char lines, int bg_color, int attribute)
{
   char trow, tcol;
   int x, y;
   char ch;
   int bg, fg, attr;
   int old_refresh;

   TRACE(console, "GENERIC_ScrollUpWindow()\n");

   /* Abort if we have only partial functionality */
   if (!(driver.getCursorPosition && driver.moveCursor && driver.write
      && driver.getCharacterAtCursor && driver.clearWindow))
      return;

   old_refresh = CONSOLE_GetRefresh();
   CONSOLE_SetRefresh(FALSE);

   CONSOLE_GetCursorPosition(&trow, &tcol);
   
   for (x = row1 + lines; x < row2; x++)
   {
      for (y = col1; y < col2; y++)
      {
         CONSOLE_MoveCursor(x, y);
         CONSOLE_GetCharacterAtCursor(&ch, &fg, &bg, &attr);
         CONSOLE_MoveCursor(x - lines, y);
         CONSOLE_Write(ch, fg, bg, attr);
      }
   }         
   CONSOLE_ClearWindow(row2 - lines, col1, row2, col2, bg_color,
      attribute);         
   CONSOLE_MoveCursor(trow, tcol);

   CONSOLE_SetRefresh(old_refresh);

   TRACE(console, "GENERIC_ScrollUpWindow() completed.\n");
}

void GENERIC_ScrollDownWindow(char row1, char col1, char row2, char col2, 
   char lines, int bg_color, int attribute)
{
   char trow, tcol;
   int x, y;
   char ch;
   int bg, fg, attr;
   int old_refresh;

   TRACE(console, "GENERIC_ScrollDownWindow()\n");

   /* Abort if we have only partial functionality */
   if (!(driver.getCursorPosition && driver.moveCursor && driver.write
      && driver.getCharacterAtCursor && driver.clearWindow))
      return;

   old_refresh = CONSOLE_GetRefresh();
   CONSOLE_SetRefresh(FALSE);

   CONSOLE_GetCursorPosition(&trow, &tcol);
   
   for (x = row2 - lines; x > row1; x--)
   {
      for (y = col1; y < col2; y++)
      {
         CONSOLE_MoveCursor(x, y);
         CONSOLE_GetCharacterAtCursor(&ch, &fg, &bg, &attr);
         CONSOLE_MoveCursor(x + lines, y);
         CONSOLE_Write(ch, fg, bg, attr);
      }
   }         
   CONSOLE_ClearWindow(row1, col1, row1 + lines, col2, bg_color,
      attribute); 
   CONSOLE_MoveCursor(trow, tcol);

   CONSOLE_SetRefresh(old_refresh);

   TRACE(console, "GENERIC_ScrollDownWindow() completed.\n");

}

char GENERIC_GetCharacter()
{
   /* Keep getting keys until we get one with a char value */
   char ch = (char) 0, scan;
   
   while (!ch)
   {
      CONSOLE_GetKeystroke(&ch, &scan);
   }
   return ch;
}

