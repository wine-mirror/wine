/*
 * USER input header file
 * Copyright 1997 David Faure
 *
 */

#ifndef __WINE_INPUT_H
#define __WINE_INPUT_H

#include "windef.h"

extern BOOL MouseButtonsStates[3];
extern BOOL AsyncMouseButtonsStates[3];
extern BYTE InputKeyStateTable[256];
extern BYTE QueueKeyStateTable[256];
extern BYTE AsyncKeyStateTable[256];
extern DWORD PosX, PosY;

extern BOOL SwappedButtons;

#define GET_KEYSTATE()							\
     ((MouseButtonsStates[SwappedButtons ? 2 : 0]  ? MK_LBUTTON : 0) |	\
      (MouseButtonsStates[1]                       ? MK_RBUTTON : 0) |	\
      (MouseButtonsStates[SwappedButtons ? 0 : 2]  ? MK_MBUTTON : 0) |	\
      (InputKeyStateTable[VK_SHIFT]   & 0x80       ? MK_SHIFT   : 0) |	\
      (InputKeyStateTable[VK_CONTROL] & 0x80       ? MK_CONTROL : 0))


#endif  /* __WINE_INPUT_H */

