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
extern BOOL MSG_InternalGetMessage( MSG *msg, HWND hwnd,
                                      HWND hwndOwner, WPARAM code,
                                      WORD flags, BOOL sendIdle );

/* timer.c */
extern BOOL TIMER_Init( void );
extern void TIMER_RemoveWindowTimers( HWND hwnd );
extern void TIMER_RemoveQueueTimers( HQUEUE16 hqueue );
extern BOOL TIMER_GetTimerMsg( MSG *msg, HWND hwnd,
                                 HQUEUE16 hQueue, BOOL remove );

/* event.c */
typedef struct tagEVENT_DRIVER {
  BOOL   (*pInit)(void);
  void   (*pSynchronize)(void);
  BOOL   (*pCheckFocus)(void);
  BOOL   (*pQueryPointer)(DWORD *, DWORD *, DWORD *);
  void   (*pUserRepaintDisable)(BOOL);
} EVENT_DRIVER;

extern EVENT_DRIVER *EVENT_Driver;

extern BOOL EVENT_Init( void );
extern void EVENT_Synchronize( void );
extern BOOL EVENT_CheckFocus( void );
extern BOOL EVENT_QueryPointer(DWORD *posX, DWORD *posY, DWORD *state);

/* input.c */

extern HWND EVENT_Capture( HWND, INT16 );
extern BOOL EVENT_QueryPointer( DWORD *posX, DWORD *posY, DWORD *state );

extern void joySendMessages(void);

#endif  /* __WINE_MESSAGE_H */
