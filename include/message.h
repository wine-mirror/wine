/*
 * Message definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_MESSAGE_H
#define __WINE_MESSAGE_H

#include "windef.h"

struct tagMSG;

extern DWORD MSG_WineStartTicks;  /* Ticks at Wine startup */

/* message.c */
extern BOOL MSG_InternalGetMessage( int type, struct tagMSG *msg, HWND hwnd,
                                    HWND hwndOwner, WPARAM code,
                                    WORD flags, BOOL sendIdle, BOOL* idleSent );

/* timer.c */
extern BOOL TIMER_Init( void );
extern void TIMER_RemoveWindowTimers( HWND hwnd );
extern void TIMER_RemoveQueueTimers( HQUEUE16 hqueue );
extern BOOL TIMER_GetTimerMsg( struct tagMSG *msg, HWND hwnd,
                                 HQUEUE16 hQueue, BOOL remove );

/* event.c */
typedef struct tagEVENT_DRIVER {
  BOOL   (*pInit)(void);
  void   (*pSynchronize)(void);
  BOOL   (*pCheckFocus)(void);
  void   (*pUserRepaintDisable)(BOOL);
} EVENT_DRIVER;

extern EVENT_DRIVER *EVENT_Driver;

extern BOOL EVENT_Init( void );
extern void EVENT_Synchronize( void );
extern BOOL EVENT_CheckFocus( void );

/* input.c */

extern HWND EVENT_Capture( HWND, INT16 );

#endif  /* __WINE_MESSAGE_H */
