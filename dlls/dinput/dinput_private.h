#ifndef __WINE_DLLS_DINPUT_DINPUT_PRIVATE_H
#define __WINE_DLLS_DINPUT_DINPUT_PRIVATE_H

#include "winbase.h"
#include "dinput.h"

/* Implementation specification */
typedef struct IDirectInputAImpl IDirectInputAImpl;
struct IDirectInputAImpl
{
        ICOM_VFIELD(IDirectInputA);
        DWORD                   ref;

	/* Used to have an unique sequence number for all the events */
	DWORD evsequence;
};

/* Function called by all devices that Wine supports */
typedef struct dinput_device {
  INT pref;
  BOOL (*enum_device)(DWORD dwDevType, DWORD dwFlags, LPCDIDEVICEINSTANCEA lpddi);
  HRESULT (*create_device)(IDirectInputAImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEA* pdev);
} dinput_device;

extern void dinput_register_device(dinput_device *device) ;

#endif /* __WINE_DLLS_DINPUT_DINPUT_PRIVATE_H */
