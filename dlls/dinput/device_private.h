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
    TRACE(" queueing %d at offset %d (queue pos %d / size %d)\n", 		\
	  (int) (data), (int) (offset),                           		\
	  (int) (This->queue_pos), (int) (This->queue_len));			\
										\
    if ((offset >= 0) && (This->queue_pos < This->queue_len)) {			\
      This->data_queue[This->queue_pos].dwOfs = offset;				\
      This->data_queue[This->queue_pos].dwData = data;				\
      This->data_queue[This->queue_pos].dwTimeStamp = xtime;			\
      This->data_queue[This->queue_pos].dwSequence = seq;			\
      This->queue_pos++;							\
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
	LPDIRECTINPUTDEVICE2A iface,LPCDIDATAFORMAT df ) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_SetCooperativeLevel(
	LPDIRECTINPUTDEVICE2A iface,HWND hwnd,DWORD dwflags ) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_SetEventNotification(
	LPDIRECTINPUTDEVICE2A iface,HANDLE hnd ) ;
extern ULONG WINAPI IDirectInputDevice2AImpl_Release(LPDIRECTINPUTDEVICE2A iface) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_QueryInterface(
	LPDIRECTINPUTDEVICE2A iface,REFIID riid,LPVOID *ppobj ) ;
extern ULONG WINAPI IDirectInputDevice2AImpl_AddRef(
	LPDIRECTINPUTDEVICE2A iface) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_EnumObjects(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIENUMDEVICEOBJECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_GetProperty(
	LPDIRECTINPUTDEVICE2A iface,
	REFGUID rguid,
	LPDIPROPHEADER pdiph) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_GetObjectInfo(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIDEVICEOBJECTINSTANCEA pdidoi,
	DWORD dwObj,
	DWORD dwHow) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_GetDeviceInfo(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIDEVICEINSTANCEA pdidi) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_RunControlPanel(
	LPDIRECTINPUTDEVICE2A iface,
	HWND hwndOwner,
	DWORD dwFlags) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_Initialize(
	LPDIRECTINPUTDEVICE2A iface,
	HINSTANCE hinst,
	DWORD dwVersion,
	REFGUID rguid) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_CreateEffect(
	LPDIRECTINPUTDEVICE2A iface,
	REFGUID rguid,
	LPCDIEFFECT lpeff,
	LPDIRECTINPUTEFFECT *ppdef,
	LPUNKNOWN pUnkOuter) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_EnumEffects(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIENUMEFFECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_GetEffectInfo(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIEFFECTINFOA lpdei,
	REFGUID rguid) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_GetForceFeedbackState(
	LPDIRECTINPUTDEVICE2A iface,
	LPDWORD pdwOut) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_SendForceFeedbackCommand(
	LPDIRECTINPUTDEVICE2A iface,
	DWORD dwFlags) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_EnumCreatedEffectObjects(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_Escape(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIEFFESCAPE lpDIEEsc) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_Poll(
	LPDIRECTINPUTDEVICE2A iface) ;
extern HRESULT WINAPI IDirectInputDevice2AImpl_SendDeviceData(
	LPDIRECTINPUTDEVICE2A iface,
	DWORD cbObjectData,
	LPDIDEVICEOBJECTDATA rgdod,
	LPDWORD pdwInOut,
	DWORD dwFlags) ;
extern HRESULT WINAPI IDirectInputDevice7AImpl_EnumEffectsInFile(LPDIRECTINPUTDEVICE7A iface,
								 LPCSTR lpszFileName,
								 LPDIENUMEFFECTSINFILECALLBACK pec,
								 LPVOID pvRef,
								 DWORD dwFlags) ;
extern HRESULT WINAPI IDirectInputDevice7AImpl_WriteEffectToFile(LPDIRECTINPUTDEVICE7A iface,
								 LPCSTR lpszFileName,
								 DWORD dwEntries,
								 LPDIFILEEFFECT rgDiFileEft,
								 DWORD dwFlags) ;

#endif /* __WINE_DLLS_DINPUT_DINPUTDEVICE_PRIVATE_H */
