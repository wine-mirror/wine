/*
 * Message definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_MESSAGE_H
#define __WINE_MESSAGE_H

#include "win.h"
#include "queue.h"

extern DWORD MSG_WineStartTicks;  /* Ticks at Wine startup */

/* message.c */
extern BOOL MSG_InternalGetMessage( MSG16 *msg, HWND hwnd, HWND hwndOwner,
				    short code, WORD flags, BOOL sendIdle );

/* timer.c */
extern void TIMER_RemoveWindowTimers( HWND32 hwnd );
extern void TIMER_RemoveQueueTimers( HQUEUE16 hqueue );
extern void TIMER_SwitchQueue( HQUEUE16 hOldQueue, HQUEUE16 hNewQueue );
extern LONG TIMER_GetNextExpiration(void);
extern void TIMER_ExpireTimers(void);
extern BOOL32 TIMER_GetTimerMsg( MSG16 *msg, HWND32 hwnd,
                                 HQUEUE16 hQueue, BOOL32 remove );

/* event.c */
extern BOOL EVENT_WaitXEvent( BOOL sleep );
extern void EVENT_Synchronize(void);
extern void EVENT_ProcessEvent( XEvent *event );
extern void EVENT_RegisterWindow( WND *pWnd );
extern void EVENT_DummyMotionNotify(void);

#endif  /* __WINE_MESSAGE_H */
