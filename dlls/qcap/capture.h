/* DirectShow private capture header (QCAP.DLL)
 *
 * Copyright 2005 Maarten Lankhorst
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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

#ifndef __QCAP_CAPTURE_H__
#define __QCAP_CAPTURE_H__

typedef HRESULT (* Video_Destroy)(void *pBox);
typedef HRESULT (* Video_SetMediaType)(void *pBox, AM_MEDIA_TYPE * mT);
typedef HRESULT (* Video_GetMediaType)(void *pBox, AM_MEDIA_TYPE ** mT);
typedef HRESULT (* Video_GetPropRange)(void *pBox, long Property, long *pMin, long *pMax, long *pSteppingDelta, long *pDefault, long *pCapsFlags);
typedef HRESULT (* Video_GetProp)(void *pBox, long Property, long *lValue, long *Flags);
typedef HRESULT (* Video_SetProp)(void *pBox, long Property, long lValue, long Flags);
typedef HRESULT (* Video_Control)(void *pBox, FILTER_STATE *state);

typedef struct capturefunctions {
    Video_Destroy Destroy;
    Video_SetMediaType SetFormat;
    Video_GetMediaType GetFormat;
    Video_GetPropRange GetPropRange;
    Video_GetProp Get_Prop;
    Video_SetProp Set_Prop;
    Video_Control Run, Pause, Stop;
    void *pMine;
} Capture;

typedef HRESULT (* Video_Init)(Capture *pBox, IPin *pOut, USHORT card);
HRESULT V4l_Init(Capture *pBox, IPin *pOut, USHORT card);

#define INVOKE(from, func, para...) from->func(from->pMine, para)
#define INVOKENP(from, func) from->func(from->pMine)

#endif /* __QCAP_CAPTURE_H__ */
