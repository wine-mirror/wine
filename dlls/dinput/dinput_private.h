/*
 * Copyright 2000 Lionel Ulmer
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

#ifndef __WINE_DLLS_DINPUT_DINPUT_PRIVATE_H
#define __WINE_DLLS_DINPUT_DINPUT_PRIVATE_H

#include "winbase.h"
#include "dinput.h"

/* Implementation specification */
typedef struct IDirectInputAImpl IDirectInputAImpl;
struct IDirectInputAImpl
{
   LPVOID lpVtbl;
   DWORD  ref;

   /* Used to have an unique sequence number for all the events */
   DWORD evsequence;
};

/* Function called by all devices that Wine supports */
typedef struct dinput_device {
  INT pref;
  BOOL (*enum_device)(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi);
  HRESULT (*create_device)(IDirectInputAImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEA* pdev);
} dinput_device;

extern void dinput_register_device(dinput_device *device) ;

HHOOK keyboard_hook;

LRESULT CALLBACK KeyboardCallback( int code, WPARAM wparam, LPARAM lparam );

#endif /* __WINE_DLLS_DINPUT_DINPUT_PRIVATE_H */
