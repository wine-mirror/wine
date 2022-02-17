/*
 * User definitions
 *
 * Copyright 1993 Alexandre Julliard
 * Copyright 2022 Jacek Caban
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_NTUSER_PRIVATE_H
#define __WINE_NTUSER_PRIVATE_H

#include "ntuser.h"

struct user_callbacks
{
    HWND (WINAPI *pGetDesktopWindow)(void);
    BOOL (WINAPI *pGetWindowRect)( HWND hwnd, LPRECT rect );
    BOOL (WINAPI *pRedrawWindow)( HWND, const RECT*, HRGN, UINT );
    LRESULT (WINAPI *pSendMessageTimeoutW)( HWND, UINT, WPARAM, LPARAM, UINT, UINT, PDWORD_PTR );
    HWND (WINAPI *pWindowFromDC)( HDC );
};

/* this is the structure stored in TEB->Win32ClientInfo */
/* no attempt is made to keep the layout compatible with the Windows one */
struct user_thread_info
{
    HANDLE                        server_queue;           /* Handle to server-side queue */
    DWORD                         wake_mask;              /* Current queue wake mask */
    DWORD                         changed_mask;           /* Current queue changed mask */
    WORD                          recursion_count;        /* SendMessage recursion counter */
    WORD                          message_count;          /* Get/PeekMessage loop counter */
    WORD                          hook_call_depth;        /* Number of recursively called hook procs */
    WORD                          hook_unicode;           /* Is current hook unicode? */
    HHOOK                         hook;                   /* Current hook */
    UINT                          active_hooks;           /* Bitmap of active hooks */
    DPI_AWARENESS                 dpi_awareness;          /* DPI awareness */
    INPUT_MESSAGE_SOURCE          msg_source;             /* Message source for current message */
    struct received_message_info *receive_info;           /* Message being currently received */
    struct wm_char_mapping_data  *wmchar_data;            /* Data for WM_CHAR mappings */
    DWORD                         GetMessageTimeVal;      /* Value for GetMessageTime */
    DWORD                         GetMessagePosVal;       /* Value for GetMessagePos */
    ULONG_PTR                     GetMessageExtraInfoVal; /* Value for GetMessageExtraInfo */
    struct user_key_state_info   *key_state;              /* Cache of global key state */
    HKL                           kbd_layout;             /* Current keyboard layout */
    DWORD                         kbd_layout_id;          /* Current keyboard layout ID */
    HWND                          top_window;             /* Desktop window */
    HWND                          msg_window;             /* HWND_MESSAGE parent window */
    struct rawinput_thread_data  *rawinput;               /* RawInput thread local data / buffer */
};

C_ASSERT( sizeof(struct user_thread_info) <= sizeof(((TEB *)0)->Win32ClientInfo) );

struct user_key_state_info
{
    UINT  time;          /* Time of last key state refresh */
    INT   counter;       /* Counter to invalidate the key state */
    BYTE  state[256];    /* State for each key */
};

#endif /* __WINE_NTUSER_PRIVATE_H */
