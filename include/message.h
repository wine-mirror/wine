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

/* message.c */
extern BOOL MSG_process_raw_hardware_message( MSG *msg, ULONG_PTR extra_info, HWND hwnd_filter,
                                              UINT first, UINT last, BOOL remove );
extern BOOL MSG_process_cooked_hardware_message( MSG *msg, ULONG_PTR extra_info, BOOL remove );
extern void MSG_JournalPlayBackMsg(void);

/* sendmsg.c */
extern BOOL MSG_peek_message( MSG *msg, HWND hwnd, UINT first, UINT last, int flags );

/* timer.c */
extern void TIMER_RemoveWindowTimers( HWND hwnd );
extern void TIMER_RemoveQueueTimers( HQUEUE16 hqueue );
extern BOOL TIMER_IsTimerValid( HWND hwnd, UINT id, HWINDOWPROC hProc );

/* input.c */

extern HWND EVENT_Capture( HWND, INT16 );

#endif  /* __WINE_MESSAGE_H */
