/* Console.H */

/* Include file for definitions pertaining to Wine's text-console
   interface. 
*/

#ifndef CONSOLE_H
#define CONSOLE_H

#include "config.h"

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
} CONSOLE_device;

CONSOLE_device driver; /* Global driver struct */

/* Generic defines */
void CONSOLE_Init();
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

#endif /* WINE_NCURSES */

#endif /* CONSOLE_H */
