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
extern void TIMER_RemoveWindowTimers( HWND hwnd );
extern void TIMER_RemoveQueueTimers( HQUEUE16 hqueue );
extern void TIMER_SwitchQueue( HQUEUE16 hOldQueue, HQUEUE16 hNewQueue );
extern LONG TIMER_GetNextExpiration(void);
extern void TIMER_ExpireTimers(void);
extern BOOL TIMER_GetTimerMsg( MSG *msg, HWND hwnd,
                                 HQUEUE16 hQueue, BOOL remove );

#define EVENT_IO_READ		0
#define EVENT_IO_WRITE		1
#define EVENT_IO_EXCEPT		2

/* event.c */

typedef struct _EVENT_DRIVER {
  BOOL (*pInit)(void);
  void   (*pAddIO)(int, unsigned);
  void   (*pDeleteIO)(int, unsigned);
  BOOL (*pWaitNetEvent)(BOOL, BOOL);
  void   (*pSynchronize)(void);
  BOOL (*pCheckFocus)(void);
  BOOL (*pQueryPointer)(DWORD *, DWORD *, DWORD *);
  void   (*pDummyMotionNotify)(void);
  BOOL (*pPending)(void);
  BOOL16 (*pIsUserIdle)(void);
  void   (*pWakeUp)(void);
} EVENT_DRIVER;

extern void EVENT_AddIO( int fd, unsigned flag );
extern void EVENT_DeleteIO( int fd, unsigned flag );
extern BOOL EVENT_Init( void );
extern BOOL EVENT_WaitNetEvent( BOOL sleep, BOOL peek );
extern void EVENT_Synchronize(void);
extern BOOL EVENT_CheckFocus( void );
extern BOOL EVENT_QueryPointer(DWORD *posX, DWORD *posY, DWORD *state);
extern void EVENT_DummyMotionNotify(void);
extern BOOL EVENT_Pending(void);
extern void EVENT_WakeUp(void);

/* input.c */

extern HWND EVENT_Capture( HWND, INT16 );
extern BOOL EVENT_QueryPointer( DWORD *posX, DWORD *posY, DWORD *state );

extern void joySendMessages(void);

#endif  /* __WINE_MESSAGE_H */
