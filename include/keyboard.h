/*
 * Keyboard header file
 * Copyright 1997 David Faure
 *
 */

#ifndef __WINE_KEYBOARD_H
#define __WINE_KEYBOARD_H

extern BOOL32 MouseButtonsStates[3];
extern BOOL32 AsyncMouseButtonsStates[3];
extern BYTE InputKeyStateTable[256];
extern BYTE QueueKeyStateTable[256];
extern BYTE AsyncKeyStateTable[256];

extern BOOL32 KEYBOARD_Init(void);
extern void KEYBOARD_HandleEvent( XKeyEvent *event );

#endif  /* __WINE_KEYBOARD_H */
