/*
 * Message definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_MESSAGE_H
#define __WINE_MESSAGE_H

#include "windows.h"

extern DWORD MSG_WineStartTicks;  /* Ticks at Wine startup */

extern void MSG_Synchronize();
extern BOOL MSG_WaitXEvent( LONG maxWait );
extern BOOL MSG_GetHardwareMessage( LPMSG msg );
extern BOOL MSG_InternalGetMessage( SEGPTR msg, HWND hwnd, HWND hwndOwner,
				    short code, WORD flags, BOOL sendIdle );

#endif  /* __WINE_MESSAGE_H */
