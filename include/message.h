/*
 * Message definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_MESSAGE_H
#define __WINE_MESSAGE_H

#include "windef.h"
#include "wine/windef16.h"
#include "winproc.h"

struct tagMSG;

/* message.c */
extern BOOL MSG_InternalGetMessage( struct tagMSG *msg, HWND hwnd, HWND hwndOwner,
                                    UINT first, UINT last, WPARAM code,
                                    WORD flags, BOOL sendIdle, BOOL* idleSent );

/* timer.c */
extern void TIMER_RemoveWindowTimers( HWND hwnd );
extern void TIMER_RemoveQueueTimers( HQUEUE16 hqueue );
extern BOOL TIMER_GetTimerMsg( struct tagMSG *msg, HWND hwnd,
                                 HQUEUE16 hQueue, BOOL remove );
extern BOOL TIMER_IsTimerValid( HWND hwnd, UINT id, HWINDOWPROC hProc );

/* event.c */
extern void EVENT_Synchronize( void );
extern BOOL EVENT_CheckFocus( void );

/* input.c */

extern HWND EVENT_Capture( HWND, INT16 );

#endif  /* __WINE_MESSAGE_H */
