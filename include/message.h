/*
 * Message definitions
 *
 * Copyright 1993 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
extern void TIMER_RemoveThreadTimers(void);
extern BOOL TIMER_IsTimerValid( HWND hwnd, UINT id, HWINDOWPROC hProc );

#endif  /* __WINE_MESSAGE_H */
