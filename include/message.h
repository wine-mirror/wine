/*
 * Message definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_MESSAGE_H
#define __WINE_MESSAGE_H

#include "win.h"
#include "queue.h"
#include "wintypes.h"

extern DWORD MSG_WineStartTicks;  /* Ticks at Wine startup */

/* message.c */
extern BOOL32 MSG_InternalGetMessage( MSG16 *msg, HWND32 hwnd,
                                      HWND32 hwndOwner, WPARAM32 code,
                                      WORD flags, BOOL32 sendIdle );

/* timer.c */
extern void TIMER_RemoveWindowTimers( HWND32 hwnd );
extern void TIMER_RemoveQueueTimers( HQUEUE16 hqueue );
extern void TIMER_SwitchQueue( HQUEUE16 hOldQueue, HQUEUE16 hNewQueue );
extern LONG TIMER_GetNextExpiration(void);
extern void TIMER_ExpireTimers(void);
extern BOOL32 TIMER_GetTimerMsg( MSG16 *msg, HWND32 hwnd,
                                 HQUEUE16 hQueue, BOOL32 remove );

#define EVENT_IO_READ		0
#define EVENT_IO_WRITE		1
#define EVENT_IO_EXCEPT		2

/* event.c */

typedef struct _EVENT_DRIVER {
  BOOL32 (*pInit)(void);
  void   (*pAddIO)(int, unsigned);
  void   (*pDeleteIO)(int, unsigned);
  BOOL32 (*pWaitNetEvent)(BOOL32, BOOL32);
  void   (*pSynchronize)(void);
  BOOL32 (*pCheckFocus)(void);
  BOOL32 (*pQueryPointer)(DWORD *, DWORD *, DWORD *);
  void   (*pDummyMotionNotify)(void);
  BOOL32 (*pPending)(void);
  BOOL16 (*pIsUserIdle)(void);
} EVENT_DRIVER;

extern void EVENT_AddIO( int fd, unsigned flag );
extern void EVENT_DeleteIO( int fd, unsigned flag );
extern BOOL32 EVENT_Init( void );
extern BOOL32 EVENT_WaitNetEvent( BOOL32 sleep, BOOL32 peek );
extern void EVENT_Synchronize(void);
extern BOOL32 EVENT_CheckFocus( void );
extern BOOL32 EVENT_QueryPointer(DWORD *posX, DWORD *posY, DWORD *state);
extern void EVENT_DummyMotionNotify(void);
extern BOOL32 EVENT_Pending(void);

/* input.c */

extern HWND32 EVENT_Capture( HWND32, INT16 );
extern INT16 EVENT_GetCaptureInfo(void);
extern BOOL32 EVENT_QueryPointer( DWORD *posX, DWORD *posY, DWORD *state );

extern void joySendMessages(void);

#endif  /* __WINE_MESSAGE_H */
