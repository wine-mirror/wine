/*
 * Menu functions
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995, 2009 Alexandre Julliard
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

#include "win32u_private.h"
#include "ntuser_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(menu);
WINE_DECLARE_DEBUG_CHANNEL(accel);

/* the accelerator user object */
struct accelerator
{
    struct user_object obj;
    unsigned int       count;
    ACCEL              table[1];
};

/* (other menu->FocusedItem values give the position of the focused item) */
#define NO_SELECTED_ITEM  0xffff

/**********************************************************************
 *           NtUserCopyAcceleratorTable   (win32u.@)
 */
INT WINAPI NtUserCopyAcceleratorTable( HACCEL src, ACCEL *dst, INT count )
{
    struct accelerator *accel;
    int i;

    if (!(accel = get_user_handle_ptr( src, NTUSER_OBJ_ACCEL ))) return 0;
    if (accel == OBJ_OTHER_PROCESS)
    {
        FIXME_(accel)( "other process handle %p?\n", src );
        return 0;
    }
    if (dst)
    {
        if (count > accel->count) count = accel->count;
        for (i = 0; i < count; i++)
        {
            dst[i].fVirt = accel->table[i].fVirt & 0x7f;
            dst[i].key   = accel->table[i].key;
            dst[i].cmd   = accel->table[i].cmd;
        }
    }
    else count = accel->count;
    release_user_handle_ptr( accel );
    return count;
}

/*********************************************************************
 *           NtUserCreateAcceleratorTable   (win32u.@)
 */
HACCEL WINAPI NtUserCreateAcceleratorTable( ACCEL *table, INT count )
{
    struct accelerator *accel;
    HACCEL handle;

    if (count < 1)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    accel = malloc( FIELD_OFFSET( struct accelerator, table[count] ));
    if (!accel) return 0;
    accel->count = count;
    memcpy( accel->table, table, count * sizeof(*table) );

    if (!(handle = alloc_user_handle( &accel->obj, NTUSER_OBJ_ACCEL ))) free( accel );
    TRACE_(accel)("returning %p\n", handle );
    return handle;
}

/******************************************************************************
 *           NtUserDestroyAcceleratorTable   (win32u.@)
 */
BOOL WINAPI NtUserDestroyAcceleratorTable( HACCEL handle )
{
    struct accelerator *accel;

    if (!(accel = free_user_handle( handle, NTUSER_OBJ_ACCEL ))) return FALSE;
    if (accel == OBJ_OTHER_PROCESS)
    {
        FIXME_(accel)( "other process handle %p\n", accel );
        return FALSE;
    }
    free( accel );
    return TRUE;
}

/* see GetMenu */
HMENU get_menu( HWND hwnd )
{
    return UlongToHandle( get_window_long( hwnd, GWLP_ID ));
}

/* see CreateMenu */
HMENU create_menu(void)
{
    POPUPMENU *menu;
    HMENU handle;

    if (!(menu = calloc( 1, sizeof(*menu) ))) return 0;
    menu->FocusedItem = NO_SELECTED_ITEM;
    menu->refcount = 1;

    if (!(handle = alloc_user_handle( &menu->obj, NTUSER_OBJ_MENU ))) free( menu );

    TRACE( "return %p\n", handle );
    return handle;
}

/**********************************************************************
 *         NtUserDestroyMenu   (win32u.@)
 */
BOOL WINAPI NtUserDestroyMenu( HMENU handle )
{
    POPUPMENU *menu;

    TRACE( "(%p)\n", handle );

    if (!(menu = free_user_handle( handle, NTUSER_OBJ_MENU ))) return FALSE;
    if (menu == OBJ_OTHER_PROCESS) return FALSE;

    /* DestroyMenu should not destroy system menu popup owner */
    if ((menu->wFlags & (MF_POPUP | MF_SYSMENU)) == MF_POPUP && menu->hWnd)
    {
        NtUserDestroyWindow( menu->hWnd );
        menu->hWnd = 0;
    }

    if (menu->items && user_callbacks) /* recursively destroy submenus */
        user_callbacks->free_menu_items( menu );

    free( menu );
    return TRUE;
}

/*******************************************************************
 *           NtUserSetSystemMenu    (win32u.@)
 */
BOOL WINAPI NtUserSetSystemMenu( HWND hwnd, HMENU menu )
{
    return user_callbacks && user_callbacks->pSetSystemMenu( hwnd, menu );
}
