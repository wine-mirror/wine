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

#define EVENT_IO_READ		0
#define EVENT_IO_WRITE		1
#define EVENT_IO_EXCEPT		2

/* event.c */
typedef struct tagEVENT_DRIVER {
  BOOL   (*pInit)(void);
  void   (*pSynchronize)(BOOL);
  BOOL   (*pCheckFocus)(void);
  BOOL   (*pQueryPointer)(DWORD *, DWORD *, DWORD *);
  void   (*pDummyMotionNotify)(void);
  void   (*pUserRepaintDisable)(BOOL);
} EVENT_DRIVER;

extern EVENT_DRIVER *EVENT_Driver;

extern void EVENT_AddIO( int fd, unsigned flag );
extern void EVENT_DeleteIO( int fd, unsigned flag );
extern BOOL EVENT_Init( void );
extern void EVENT_WaitNetEvent( void );
extern void EVENT_Synchronize( BOOL bProcessEvents );
extern BOOL EVENT_CheckFocus( void );
extern BOOL EVENT_QueryPointer(DWORD *posX, DWORD *posY, DWORD *state);
extern void EVENT_DummyMotionNotify(void);
extern void EVENT_WakeUp(void);

/* input.c */

extern HWND EVENT_Capture( HWND, INT16 );
extern BOOL EVENT_QueryPointer( DWORD *posX, DWORD *posY, DWORD *state );

extern void joySendMessages(void);

#endif  /* __WINE_MESSAGE_H */
