/* interface.c */

/* The primary purpose of this function is to provide CONSOLE_*
   reotines that immediately call the appropiate driver handler.
   This cleans up code in the individual modules considerably.
   This could be done using a macro, but additional functionality
   may be provided here in the future. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "windef.h"
#include "console.h"
#include "options.h"

CONSOLE_device driver;

static int pop_driver(char **, char **, int *);

static int console_initialized = FALSE;

static int CONSOLE_Init(void)
{
      char buffer[256];
      char *single, *drivers = buffer;
      int length;
      char initial_rows[5];
      char initial_columns[5];

      /* Suitable defaults... */
      driver.console_out = stdout;
      driver.console_in = stdin;

      /* drivers should be a string that consists of driver names
         followed by plus (+) signs to denote additions. 

         For example:
            drivers = tty                Load just the tty driver
            drivers = ncurses+xterm      Load ncurses then xterm

         The "default" value is just tty.
      */
      PROFILE_GetWineIniString( "console", "Drivers", CONSOLE_DEFAULT_DRIVER,
                                buffer, sizeof(buffer) );

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

   /* Read in generic configuration */
   /* This is primarily to work around a limitation in nxterm where
      this information is not correctly passed to the ncurses layer
      through the terminal. At least, I'm not doing it correctly if there
      is a way. But this serves as a generic way to do the same anyway. */

   /* We are setting this to 80x25 here which is *not* the default for
      most xterm variants... It is however the standard VGA resolution */

   /* FIXME: We need to be able to be able to specify that the window's
      dimensions should be used. This is required for correct emulation
      of Win32's console and Win32's DOS emulation */

   PROFILE_GetWineIniString("console", "InitialRows",
      "24", initial_rows, 5);
   PROFILE_GetWineIniString("console", "InitialColumns",
      "80", initial_columns, 5);

   sscanf(initial_rows, "%d", &driver.y_res);
   sscanf(initial_columns, "%d", &driver.x_res);
   
   GENERIC_Start();

   if (driver.init)
      driver.init();

   /* Not all terminals let our programs know the proper resolution
      if the resolution is set on the command-line... */
   CONSOLE_NotifyResizeScreen(driver.x_res, driver.y_res);

   atexit(CONSOLE_Close);

   /* For now, always return TRUE */
   return TRUE;
}

void CONSOLE_Write(char out, int fg_color, int bg_color, int attribute)
{
   if (!console_initialized)
      console_initialized = CONSOLE_Init();
      
   if (driver.write)
   {
      driver.write(out, fg_color, bg_color, attribute);
      if (!driver.norefresh)
         CONSOLE_Refresh();
   }
}

void CONSOLE_Close(void)
{
   if (driver.close)
      driver.close();
}

void CONSOLE_MoveCursor(char row, char col)
{
   if (!console_initialized)
      console_initialized = CONSOLE_Init();
      
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
   if (!console_initialized)
      console_initialized = CONSOLE_Init();
      
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
   if (!console_initialized)
      console_initialized = CONSOLE_Init();
      
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
   if (!console_initialized)
      console_initialized = CONSOLE_Init();
      
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
   if (!console_initialized)
      console_initialized = CONSOLE_Init();
      
   if (driver.checkForKeystroke)
      return driver.checkForKeystroke(scan, ascii);
   else
      return FALSE;
}

void CONSOLE_GetKeystroke(char *scan, char *ascii)
{
   if (!console_initialized)
      console_initialized = CONSOLE_Init();
      
   if (driver.getKeystroke)
      driver.getKeystroke(scan, ascii);
}

void CONSOLE_GetCursorPosition(char *row, char *col)
{
   if (!console_initialized)
      console_initialized = CONSOLE_Init();
      
   if (driver.getCursorPosition)
      driver.getCursorPosition(row, col);
}

void CONSOLE_GetCharacterAtCursor(char *ch, int *fg, int *bg, int *a)
{
   if (!console_initialized)
      console_initialized = CONSOLE_Init();
      
   if (driver.getCharacterAtCursor)
      driver.getCharacterAtCursor(ch, fg, bg, a);
}

void CONSOLE_Refresh()
{
   if (!console_initialized)
      console_initialized = CONSOLE_Init();
      
   if (driver.refresh)
      driver.refresh();
}

int CONSOLE_AllocColor(int color)
{
   if (!console_initialized)
      console_initialized = CONSOLE_Init();
      
   if (driver.allocColor)
      return driver.allocColor(color);
   else 
      return 0;
}

void CONSOLE_ClearScreen()
{
   if (!console_initialized)
      console_initialized = CONSOLE_Init();
      
   if (driver.clearScreen)
   {
      driver.clearScreen();
      if (!driver.norefresh)
         CONSOLE_Refresh();
   }
}

char CONSOLE_GetCharacter()
{
   if (!console_initialized)
      console_initialized = CONSOLE_Init();
      
   /* I'm not sure if we need this really. This is a function that can be
      accelerated that returns the next *non extended* keystroke */
   if (driver.getCharacter)
      return driver.getCharacter();
   else
      return (char) 0; /* Sure, this will probably break programs... */
}

void CONSOLE_ResizeScreen(int x, int y)
{
   if (!console_initialized)
      console_initialized = CONSOLE_Init();
      
   if (driver.resizeScreen)
      driver.resizeScreen(x, y);
}

void CONSOLE_NotifyResizeScreen(int x, int y)
{
   if (driver.notifyResizeScreen)
      driver.notifyResizeScreen(x, y);
}

void CONSOLE_SetBackgroundColor(int fg, int bg)
{
   if (!console_initialized)
      console_initialized = CONSOLE_Init();
      
   if (driver.setBackgroundColor)
      driver.setBackgroundColor(fg, bg);
}

void CONSOLE_GetBackgroundColor(int *fg, int *bg)
{
   if (!console_initialized)
      console_initialized = CONSOLE_Init();
      
   if (driver.getBackgroundColor)
      driver.getBackgroundColor(fg, bg);
}

void CONSOLE_WriteRawString(char *str)
{
   if (!console_initialized)
      console_initialized = CONSOLE_Init();
      
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
