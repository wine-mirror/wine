/*
 * Copyright 2000 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
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

#ifndef __WINE_DLLS_DINPUT_DINPUTDEVICE_PRIVATE_H
#define __WINE_DLLS_DINPUT_DINPUTDEVICE_PRIVATE_H

#include "winbase.h"
#include "dinput.h"

/* Device implementation */
typedef struct IDirectInputDevice2AImpl IDirectInputDevice2AImpl;
struct IDirectInputDevice2AImpl
{
        ICOM_VFIELD(IDirectInputDevice2A);
        DWORD                           ref;
        GUID                            guid;
};

/* Routines to do DataFormat / WineFormat conversions */
typedef struct {
  int size;
  int offset_in;
  int offset_out;
  int value;
} DataTransform;

typedef struct {
  int size;
  int internal_format_size;
  DataTransform *dt;
} DataFormat;
extern void fill_DataFormat(void *out, void *in, DataFormat *df) ;
extern DataFormat *create_DataFormat(DIDATAFORMAT *wine_format, LPCDIDATAFORMAT asked_format, int *offset) ;

/* Used to fill events in the queue */
#define GEN_EVENT(offset,data,xtime,seq)					\
{										\
  /* If queue_len > 0, queuing is requested -> TRACE the event queued */	\
  if (This->queue_len > 0) {							\
    DWORD nq;									\
    TRACE(" queueing %d at offset %d (queue head %d / size %d)\n", 		\
	  (int) (data), (int) (offset),                           		\
	  (int) (This->queue_head), (int) (This->queue_len));			\
										\
    nq = This->queue_head+1;							\
    while (nq >= This->queue_len) nq -= This->queue_len;			\
    if ((offset >= 0) && (nq != This->queue_tail)) {				\
      This->data_queue[This->queue_head].dwOfs = offset;			\
      This->data_queue[This->queue_head].dwData = data;				\
      This->data_queue[This->queue_head].dwTimeStamp = xtime;			\
      This->data_queue[This->queue_head].dwSequence = seq;			\
      This->queue_head = nq;							\
    }										\
  }										\
}


/* Various debug tools */
extern void _dump_cooperativelevel_DI(DWORD dwFlags) ;
extern void _dump_EnumObjects_flags(DWORD dwFlags) ;
extern void _dump_DIPROPHEADER(DIPROPHEADER *diph) ;
extern void _dump_OBJECTINSTANCEA(DIDEVICEOBJECTINSTANCEA *ddoi) ;

/* And the stubs */
extern HRESULT WINAPI IDirectInputDevice2AImpl_SetDataFormat(
	LPDIRECTINPUTDEVICE8A iface,LPCDIDATAFORMAT df ) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_SetCooperativeLevel(
	LPDIRECTINPUTDEVICE8A iface,HWND hwnd,DWORD dwflags ) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_SetEventNotification(
	LPDIRECTINPUTDEVICE8A iface,HANDLE hnd ) ;
extern ULONG WINAPI IDirectInputDevice2AImpl_Release(LPDIRECTINPUTDEVICE8A iface) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_QueryInterface(
	LPDIRECTINPUTDEVICE8A iface,REFIID riid,LPVOID *ppobj ) ;
extern ULONG WINAPI IDirectInputDevice2AImpl_AddRef(
	LPDIRECTINPUTDEVICE8A iface) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_EnumObjects(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIENUMDEVICEOBJECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_GetProperty(
	LPDIRECTINPUTDEVICE8A iface,
	REFGUID rguid,
	LPDIPROPHEADER pdiph) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_GetObjectInfo(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVICEOBJECTINSTANCEA pdidoi,
	DWORD dwObj,
	DWORD dwHow) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_GetDeviceInfo(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVICEINSTANCEA pdidi) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_RunControlPanel(
	LPDIRECTINPUTDEVICE8A iface,
	HWND hwndOwner,
	DWORD dwFlags) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_Initialize(
	LPDIRECTINPUTDEVICE8A iface,
	HINSTANCE hinst,
	DWORD dwVersion,
	REFGUID rguid) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_CreateEffect(
	LPDIRECTINPUTDEVICE8A iface,
	REFGUID rguid,
	LPCDIEFFECT lpeff,
	LPDIRECTINPUTEFFECT *ppdef,
	LPUNKNOWN pUnkOuter) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_EnumEffects(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIENUMEFFECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_GetEffectInfo(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIEFFECTINFOA lpdei,
	REFGUID rguid) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_GetForceFeedbackState(
	LPDIRECTINPUTDEVICE8A iface,
	LPDWORD pdwOut) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_SendForceFeedbackCommand(
	LPDIRECTINPUTDEVICE8A iface,
	DWORD dwFlags) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_EnumCreatedEffectObjects(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_Escape(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIEFFESCAPE lpDIEEsc) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_Poll(
	LPDIRECTINPUTDEVICE8A iface) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_SendDeviceData(
	LPDIRECTINPUTDEVICE8A iface,
	DWORD cbObjectData,
	LPCDIDEVICEOBJECTDATA rgdod,
	LPDWORD pdwInOut,
	DWORD dwFlags) ;
extern HRESULT WINAPI IDirectInputDevice7AImpl_EnumEffectsInFile(LPDIRECTINPUTDEVICE8A iface,
								 LPCSTR lpszFileName,
								 LPDIENUMEFFECTSINFILECALLBACK pec,
								 LPVOID pvRef,
								 DWORD dwFlags) ;
extern HRESULT WINAPI IDirectInputDevice7AImpl_WriteEffectToFile(LPDIRECTINPUTDEVICE8A iface,
								 LPCSTR lpszFileName,
								 DWORD dwEntries,
								 LPDIFILEEFFECT rgDiFileEft,
								 DWORD dwFlags) ;
extern HRESULT WINAPI IDirectInputDevice8AImpl_BuildActionMap(LPDIRECTINPUTDEVICE8A iface,
							      LPDIACTIONFORMATA lpdiaf,
							      LPCSTR lpszUserName,
							      DWORD dwFlags);
extern HRESULT WINAPI IDirectInputDevice8AImpl_SetActionMap(LPDIRECTINPUTDEVICE8A iface,
							    LPDIACTIONFORMATA lpdiaf,
							    LPCSTR lpszUserName,
							    DWORD dwFlags);
extern HRESULT WINAPI IDirectInputDevice8AImpl_GetImageInfo(LPDIRECTINPUTDEVICE8A iface,
							    LPDIDEVICEIMAGEINFOHEADERA lpdiDevImageInfoHeader);

#endif /* __WINE_DLLS_DINPUT_DINPUTDEVICE_PRIVATE_H */
