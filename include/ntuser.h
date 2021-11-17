/*
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#ifndef _NTUSER_
#define _NTUSER_

#include <winuser.h>
#include <wingdi.h>
#include <winternl.h>

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

HKL     WINAPI NtUserActivateKeyboardLayout( HKL layout, UINT flags );
BOOL    WINAPI NtUserAddClipboardFormatListener( HWND hwnd );
BOOL    WINAPI NtUserAttachThreadInput( DWORD from, DWORD to, BOOL attach );
BOOL    WINAPI NtUserCloseDesktop( HDESK handle );
BOOL    WINAPI NtUserCloseWindowStation( HWINSTA handle );
INT     WINAPI NtUserCountClipboardFormats(void);
HDESK   WINAPI NtUserCreateDesktopEx( OBJECT_ATTRIBUTES *attr, UNICODE_STRING *device,
                                      DEVMODEW *devmode, DWORD flags, ACCESS_MASK access,
                                      ULONG heap_size );
HWINSTA WINAPI NtUserCreateWindowStation( OBJECT_ATTRIBUTES *attr, ACCESS_MASK mask, ULONG arg3,
                                          ULONG arg4, ULONG arg5, ULONG arg6, ULONG arg7 );
INT     WINAPI NtUserGetClipboardFormatName( UINT format, WCHAR *buffer, INT maxlen );
HWND    WINAPI NtUserGetClipboardOwner(void);
DWORD   WINAPI NtUserGetClipboardSequenceNumber(void);
HWND    WINAPI NtUserGetClipboardViewer(void);
INT     WINAPI NtUserGetKeyNameText( LONG lparam, WCHAR *buffer, INT size );
SHORT   WINAPI NtUserGetKeyState( INT vkey );
HKL     WINAPI NtUserGetKeyboardLayout( DWORD thread_id );
UINT    WINAPI NtUserGetKeyboardLayoutList( INT size, HKL *layouts );
BOOL    WINAPI NtUserGetKeyboardState( BYTE *state );
BOOL    WINAPI NtUserGetLayeredWindowAttributes( HWND hwnd, COLORREF *key, BYTE *alpha, DWORD *flags );
int     WINAPI NtUserGetMouseMovePointsEx( UINT size, MOUSEMOVEPOINT *ptin, MOUSEMOVEPOINT *ptout,
                                           int count, DWORD resolution );
BOOL    WINAPI NtUserGetObjectInformation( HANDLE handle, INT index, void *info,
                                           DWORD len, DWORD *needed );
HWND    WINAPI NtUserGetOpenClipboardWindow(void);
INT     WINAPI NtUserGetPriorityClipboardFormat( UINT *list, INT count );
HWINSTA WINAPI NtUserGetProcessWindowStation(void);
HANDLE  WINAPI NtUserGetProp( HWND hwnd, const WCHAR *str );
HDESK   WINAPI NtUserGetThreadDesktop( DWORD thread );
BOOL    WINAPI NtUserGetUpdatedClipboardFormats( UINT *formats, UINT size, UINT *out_size );
BOOL    WINAPI NtUserIsClipboardFormatAvailable( UINT format );
UINT    WINAPI NtUserMapVirtualKeyEx( UINT code, UINT type, HKL layout );
HWINSTA WINAPI NtUserOpenWindowStation( OBJECT_ATTRIBUTES *attr, ACCESS_MASK access );
BOOL    WINAPI NtUserSetObjectInformation( HANDLE handle, INT index, void *info, DWORD len );
HDESK   WINAPI NtUserOpenDesktop( OBJECT_ATTRIBUTES *attr, DWORD flags, ACCESS_MASK access );
HDESK   WINAPI NtUserOpenInputDesktop( DWORD flags, BOOL inherit, ACCESS_MASK access );
BOOL    WINAPI NtUserRemoveClipboardFormatListener( HWND hwnd );
HANDLE  WINAPI NtUserRemoveProp( HWND hwnd, const WCHAR *str );
BOOL    WINAPI NtUserScrollDC( HDC hdc, INT dx, INT dy, const RECT *scroll, const RECT *clip,
                               HRGN ret_update_rgn, RECT *update_rect );
BOOL    WINAPI NtUserSetKeyboardState( BYTE *state );
BOOL    WINAPI NtUserSetProcessWindowStation( HWINSTA handle );
BOOL    WINAPI NtUserSetProp( HWND hwnd, const WCHAR *str, HANDLE handle );
BOOL    WINAPI NtUserSetThreadDesktop( HDESK handle );
INT     WINAPI NtUserShowCursor( BOOL show );
INT     WINAPI NtUserToUnicodeEx( UINT virt, UINT scan, const BYTE *state,
                                  WCHAR *str, int size, UINT flags, HKL layout );
BOOL    WINAPI NtUserUnregisterHotKey( HWND hwnd, INT id );
WORD    WINAPI NtUserVkKeyScanEx( WCHAR chr, HKL layout );

#endif /* _NTUSER_ */
