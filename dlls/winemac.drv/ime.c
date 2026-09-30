/*
 * MACDRV input method driver
 *
 * Copyright 2026 Marc-Aurel Zent for CodeWeavers Inc.
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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "macdrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

pthread_mutex_t ime_composition_rect_mutex = PTHREAD_MUTEX_INITIALIZER;
CGRect ime_composition_rect;


/***********************************************************************
 *              ImeToAsciiEx (MACDRV.@)
 */
UINT macdrv_ImeToAsciiEx( UINT vkey, UINT vsc, const BYTE *state, void *update )
{
    struct macdrv_thread_data *thread_data = macdrv_thread_data();
    unsigned int flags;
    int keyc;
    BOOL repeat = !!(vsc & KF_REPEAT);

    TRACE( "vkey %#x vsc %#x state %p update %p\n", vkey, vsc, state, update );

    if (!state) return STATUS_SUCCESS;

    /* Only key down events should be sent to the Cocoa input context. We do not
     * handle key ups, and instead let those go through as a normal WM_KEYUP.
     */
    if (vsc & KF_UP) return STATUS_NOT_IMPLEMENTED;

    switch (vkey)
    {
    case VK_KANA:
    case VK_KANJI: TRACE( "Skipping metakey\n" ); return STATUS_NOT_IMPLEMENTED;
    }

    flags = thread_data->last_modifiers;
    if (state[VK_SHIFT] & 0x80) flags |= NX_SHIFTMASK;
    else flags &= ~(NX_SHIFTMASK | NX_DEVICELSHIFTKEYMASK | NX_DEVICERSHIFTKEYMASK);
    if (state[VK_CAPITAL] & 0x01) flags |= NX_ALPHASHIFTMASK;
    else flags &= ~NX_ALPHASHIFTMASK;
    if (state[VK_CONTROL] & 0x80) flags |= NX_CONTROLMASK;
    else flags &= ~(NX_CONTROLMASK | NX_DEVICELCTLKEYMASK | NX_DEVICERCTLKEYMASK);
    if (state[VK_MENU] & 0x80) flags |= NX_COMMANDMASK;
    else flags &= ~(NX_COMMANDMASK | NX_DEVICELCMDKEYMASK | NX_DEVICERCMDKEYMASK);

    /* Find the Mac keycode corresponding to the scan code */
    for (keyc = 0; keyc < ARRAY_SIZE(thread_data->keyc2vkey); keyc++)
        if (thread_data->keyc2vkey[keyc] == vkey) break;
    if (keyc >= ARRAY_SIZE(thread_data->keyc2vkey)) return 0;

    TRACE( "flags %#x keyc %#x\n", flags, keyc );

    return macdrv_send_keydown_to_input_source( keyc, flags, repeat, update ) ? STATUS_SUCCESS : STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *              SetIMECompositionRect (MACDRV.@)
 */
BOOL macdrv_SetIMECompositionRect( HWND hwnd, RECT rect )
{
    TRACE( "hwnd %p, rect %s\n", hwnd, wine_dbgstr_rect( &rect ) );

    pthread_mutex_lock( &ime_composition_rect_mutex );
    ime_composition_rect = cgrect_from_rect( rect );
    pthread_mutex_unlock( &ime_composition_rect_mutex );

    return TRUE;
}


/***********************************************************************
 *              NotifyIMEStatus (MACDRV.@)
 */
void macdrv_NotifyIMEStatus( HWND hwnd, UINT status )
{
    TRACE( "hwnd %p, status %#x\n", hwnd, status );
    if (!status) macdrv_clear_ime_text();
}


static void post_ime_update( HWND hwnd, UINT cursor_pos, WCHAR *comp_str, WCHAR *result_str, void *update )
{
    const WCHAR *strings[] = { comp_str, result_str };
    NtUserMessageCall( hwnd, WINE_IME_POST_UPDATE, cursor_pos, (LPARAM)strings,
                       update, NtUserImeDriverCall, FALSE );
}

void macdrv_ime_set_text( HWND hwnd, CFStringRef text, bool complete, unsigned int cursor_begin,
                          unsigned int cursor_end, void *update )
{
    WCHAR *str = NULL;

    TRACE( "hwnd %p text %s complete %u cursor %u-%u\n", hwnd, debugstr_cf( text ),
           complete, cursor_begin, cursor_end );

    if (text)
    {
        CFIndex length = CFStringGetLength( text );
        if (!(str = malloc( (length + 1) * sizeof(WCHAR) ))) return;
        if (length) CFStringGetCharacters( text, CFRangeMake( 0, length ), str );
        str[length] = 0;
    }

    if (complete) post_ime_update( hwnd, -1, NULL, str, update );
    else post_ime_update( hwnd, MAKELONG(cursor_begin, cursor_end), str, NULL, update );
    free( str );
}
