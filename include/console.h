/* console.h */
/* Copyright 1998 - Joseph Pranevich */

/* Include file for definitions pertaining to Wine's text-console
   interface. 
*/

#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdio.h>

#include "config.h"

/* Can we compile with curses/ncurses? */
#if (	(defined(HAVE_LIBNCURSES) || defined(HAVE_LIBCURSES)) &&	\
	(defined(HAVE_CURSES_H) || defined(HAVE_NCURSES_H))		\
)
# define WINE_NCURSES
#else
# undef WINE_NCURSES
#endif

#define CONSOLE_DEFAULT_DRIVER "tty"
/* If you have problems, try setting the next line to xterm */
#define CONSOLE_XTERM_PROG "xterm" /* We should check for this first... */

typedef struct CONSOLE_DRIVER
{
   void (*init)();
   void (*close)();
   void (*write)(char, int, int, int);
   void (*moveCursor)(char, char);
   void (*getCursorPosition)(char *, char *);
   void (*getCharacterAtCursor)(char *, int *, int *, int *);
   void (*clearScreen)();

   /* Keyboard Functions */
   int (*checkForKeystroke)(char *, char *);
   void (*getKeystroke)(char *, char *);

   /* Windowing Functions */
   void (*resizeScreen)(int, int);
   void (*notifyResizeScreen)(int, int); /* May be rethought later... */

   /* Accellerator Functions (Screen) */
   void (*clearWindow)(char, char, char, char, int, int);
   void (*scrollUpWindow)(char, char, char, char, char, int, int);
   void (*scrollDownWindow)(char, char, char, char, char, int, int);

   /* Accellerator Functions (Keyboard) */
   char (*getCharacter)();

   /* Other functions */
   void (*refresh)();
   
   /* Other data */
   int norefresh;
   FILE *console_out;
   FILE *console_in;

} CONSOLE_device;

CONSOLE_device driver; /* Global driver struct */

/* Generic defines */
void CONSOLE_Init(char *drivers);
void CONSOLE_Close();
void CONSOLE_Write(char out, int fg_color, int bg_color, int attribute);
void CONSOLE_MoveCursor(char row, char col);
void CONSOLE_ClearWindow(char, char, char, char, int, int);
void CONSOLE_ScrollUpWindow(char, char, char, char, char, int, int);
void CONSOLE_ScrollDownWindow(char, char, char, char, char, int, int);
int  CONSOLE_CheckForKeystroke(char *, char*);
void CONSOLE_GetKeystroke(char *, char *);
void CONSOLE_GetCursorPosition(char *, char *);
void CONSOLE_GetCharacterAtCursor(char *, int *, int *, int *);
void CONSOLE_Refresh();
void CONSOLE_SetRefresh(int);
int  CONSOLE_GetRefresh();
void CONSOLE_ClearScreen();
char CONSOLE_GetCharacter();
void CONSOLE_ResizeScreen();
void CONSOLE_NotifyResizeScreen(); 
void CONSOLE_WriteRawString(char *);

/* Generic Defines */
void GENERIC_Start();
void GENERIC_ClearWindow(char, char, char, char, int, int);
void GENERIC_ScrollUpWindow(char, char, char, char, char, int, int);
void GENERIC_ScrollDownWindow(char, char, char, char, char, int, int);
char GENERIC_GetCharacter();

/* TTY specific defines */
void TTY_Write(char out, int fg_color, int bg_color, int attribute);
void TTY_Start();
void TTY_GetKeystroke(char *, char *);

#ifdef WINE_NCURSES

/* ncurses defines */
void NCURSES_Write(char out, int fg_color, int bg_color, int attribute);
void NCURSES_Start();
void NCURSES_Init();
void NCURSES_Close();
int  NCURSES_CheckForKeystroke(char *, char *);
void NCURSES_GetKeystroke(char *, char *);
void NCURSES_MoveCursor(char ,char);
void NCURSES_GetCursorPosition(char *, char *);
void NCURSES_GetCharacterAtCursor(char *, int *, int *, int *);
void NCURSES_Refresh();
void NCURSES_ClearScreen();
void NCURSES_NotifyResizeScreen(int x, int y);

#endif /* WINE_NCURSES */

/* Xterm specific defines */
void XTERM_Start();
void XTERM_Close();
void XTERM_Init();
void XTERM_ResizeScreen(int x, int y);

#endif /* CONSOLE_H */
