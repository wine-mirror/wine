/* generic.c */
/* Copyright 1999 - Joseph Pranevich */

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
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(console)

static void GENERIC_MoveLine(char row1, char row2, char col1, char col2);
static void GENERIC_ClearLine(char row, char col1, char col2, int bgcolor,
   int attribute);
void GENERIC_Start()
{
   /* Here, we only want to add a driver if there is not one already
      defined. */

   TRACE("GENERIC_Start\n");

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
   char trow, tcol, x;
   int old_refresh;

   /* Abort if we have only partial functionality */
   if (!(driver.getCursorPosition && driver.moveCursor && driver.write))
      return;

   old_refresh = CONSOLE_GetRefresh();
   CONSOLE_SetRefresh(FALSE);

   CONSOLE_GetCursorPosition(&trow, &tcol);
   
   for (x = row1; x <= row2; x++)
      GENERIC_ClearLine(x, col1, col2, bg_color, attribute);

   CONSOLE_MoveCursor(trow, tcol);

   CONSOLE_SetRefresh(old_refresh);
}      

void GENERIC_ScrollUpWindow(char row1, char col1, char row2, char col2, 
   char lines, int bg_color, int attribute)
{
   /* Scroll Up Window: Characters go down */

   char trow, tcol, x;
   int old_refresh;

   TRACE("Scroll Up %d lines from %d to %d.\n", lines, row1,
      row2);

   /* Abort if we have only partial functionality */
   if (!(driver.getCursorPosition && driver.moveCursor && driver.write
      && driver.getCharacterAtCursor && driver.clearWindow))
      return;
   
   /* Save initial state... */
   old_refresh = CONSOLE_GetRefresh();
   CONSOLE_SetRefresh(FALSE);
   CONSOLE_GetCursorPosition(&trow, &tcol);

   for (x = row1 + lines; x <= row2; x++)
   {
      GENERIC_MoveLine(x, x - lines, col1, col2);
      GENERIC_ClearLine(x, col1, col2, bg_color, attribute);
   }

   /* Restore State */
   CONSOLE_MoveCursor(trow, tcol); 
   CONSOLE_SetRefresh(old_refresh);
}

void GENERIC_ScrollDownWindow(char row1, char col1, char row2, char col2, 
   char lines, int bg_color, int attribute)
{
   /* Scroll Down Window: Characters go up */

   char trow, tcol, x;
   int old_refresh;

   /* Abort if we have only partial functionality */
   if (!(driver.getCursorPosition && driver.moveCursor && driver.write
      && driver.getCharacterAtCursor && driver.clearWindow))
      return;
   
   /* Save initial state... */
   old_refresh = CONSOLE_GetRefresh();
   CONSOLE_SetRefresh(FALSE);
   CONSOLE_GetCursorPosition(&trow, &tcol);

   for (x = row2; x >= row1 + lines; x--)
   {
      GENERIC_MoveLine(x, x + lines, col1, col2);
      GENERIC_ClearLine(x, col1, col1, bg_color, attribute);
   }

   /* Restore State */
   CONSOLE_MoveCursor(trow, tcol); 
   CONSOLE_SetRefresh(old_refresh);
}

char GENERIC_GetCharacter()
{
   /* Keep getting keys until we get one with a char value */
   char ch = (char) 0, scan;
   
   while (!ch)
   {
      CONSOLE_GetKeystroke(&scan, &ch);
   }
   return ch;
}

static void GENERIC_ClearLine(char row, char col1, char col2, int bgcolor,
   int attribute)
{
   /* This function is here to simplify the logic of the scroll and clear
      functions but may be useful elsewhere. If it can be used from
      outside here, it should be made non-static */

   char x;

   TRACE("Clear Line: %d from %d to %d.\n", row, col1, col2);

   for (x = col1; x <= col2; x++)
   {
      CONSOLE_MoveCursor(row, x);
      CONSOLE_Write(' ', 0, 0, 0);
   }

   /* Assume that the calling function will make sure that the cursor is
   repositioned properly. If this becomes non-static, that will need to be
   changed. */
}
   
static void GENERIC_MoveLine(char row1, char row2, char col1, char col2)
{
   /* This function is here to simplify the logic of the scroll and clear
      functions but may be useful elsewhere. If it can be used from
      outside here, it should be made non-static */

   char x;
   int bg_color, fg_color, attribute;
   char ch;

   TRACE("Move Line: Move %d to %d.\n", row1, row2);

   for (x = col1; x <= col2; x++)
   {
      CONSOLE_MoveCursor(row1, x);
      CONSOLE_GetCharacterAtCursor(&ch, &fg_color, &bg_color, &attribute);
      CONSOLE_MoveCursor(row2, x);
      CONSOLE_Write(ch, fg_color, bg_color, attribute);
   }

   /* Assume that the calling function will make sure that the cursor is
   repositioned properly. If this becomes non-static, that will need to be
   changed. */
}   
