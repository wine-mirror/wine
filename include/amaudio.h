/*
 * Copyright (C) 2001 Hidenori Takeshima
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

#ifndef __WINE_AMAUDIO_H_
#define __WINE_AMAUDIO_H_

#include "ole2.h"
#include "mmsystem.h"
#include "dsound.h"

typedef struct IAMDirectSound IAMDirectSound;


/**************************************************************************
 *
 * IAMDirectSound interface
 *
 */

#define ICOM_INTERFACE IAMDirectSound
#define IAMDirectSound_METHODS \
    ICOM_METHOD1(HRESULT,GetDirectSoundInterface,LPDIRECTSOUND*,a1) \
    ICOM_METHOD1(HRESULT,GetPrimaryBufferInterface,LPDIRECTSOUNDBUFFER*,a1) \
    ICOM_METHOD1(HRESULT,GetSecondaryBufferInterface,LPDIRECTSOUNDBUFFER*,a1) \
    ICOM_METHOD1(HRESULT,ReleaseDirectSoundInterface,LPDIRECTSOUND,a1) \
    ICOM_METHOD1(HRESULT,ReleasePrimaryBufferInterface,LPDIRECTSOUNDBUFFER,a1) \
    ICOM_METHOD1(HRESULT,ReleaseSecondaryBufferInterface,LPDIRECTSOUNDBUFFER,a1) \
    ICOM_METHOD2(HRESULT,SetFocusWindow,HWND,a1,BOOL,a2) \
    ICOM_METHOD2(HRESULT,GetFocusWindow,HWND*,a1,BOOL*,a2)

#define IAMDirectSound_IMETHODS \
    IUnknown_IMETHODS \
    IAMDirectSound_METHODS

ICOM_DEFINE(IAMDirectSound,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IAMDirectSound_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IAMDirectSound_AddRef(p) ICOM_CALL (AddRef,p)
#define IAMDirectSound_Release(p) ICOM_CALL (Release,p)
    /*** IAMDirectSound methods ***/
#define IAMDirectSound_GetDirectSoundInterface(p,a1) ICOM_CALL1(GetDirectSoundInterface,p,a1)
#define IAMDirectSound_GetPrimaryBufferInterface(p,a1) ICOM_CALL1(GetPrimaryBufferInterface,p,a1)
#define IAMDirectSound_GetSecondaryBufferInterface(p,a1) ICOM_CALL1(GetSecondaryBufferInterface,p,a1)
#define IAMDirectSound_ReleaseDirectSoundInterface(p,a1) ICOM_CALL1(ReleaseDirectSoundInterface,p,a1)
#define IAMDirectSound_ReleasePrimaryBufferInterface(p,a1) ICOM_CALL1(ReleasePrimaryBufferInterface,p,a1)
#define IAMDirectSound_ReleaseSecondaryBufferInterface(p,a1) ICOM_CALL1(ReleaseSecondaryBufferInterface,p,a1)
#define IAMDirectSound_SetFocusWindow(p,a1,a2) ICOM_CALL2(SetFocusWindow,p,a1,a2)
#define IAMDirectSound_GetFocusWindow(p,a1,a2) ICOM_CALL2(GetFocusWindow,p,a1,a2)

#endif  /* __WINE_AMAUDIO_H_ */
