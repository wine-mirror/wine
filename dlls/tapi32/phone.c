/*
 * TAPI32 phone services
 *
 * Copyright 1999  Andreas Mohr
 */

#include "winbase.h"
#include "windef.h"
#include "tapi.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(tapi)

static LPPHONE PHONE_Alloc(void)
{
    return 0;
}

static LPPHONE PHONE_Get(HPHONE hPhone)
{	
    return 0;
}

DWORD WINAPI phoneClose(HPHONE hPhone)
{
    FIXME(tapi, "(%04x), stub.\n", hPhone);
    return 0;
}

DWORD WINAPI phoneConfigDialog(DWORD dwDeviceID, HWND hwndOwner, LPCSTR lpszDeviceClass)
{
    FIXME(tapi, "(%08lx, %04x, %s): stub.\n", dwDeviceID, hwndOwner, lpszDeviceClass);
    return 0;
}

DWORD WINAPI phoneDevSpecific(HPHONE hPhone, LPVOID lpParams, DWORD dwSize)
{
    FIXME(tapi, "(%04x, %p, %08ld): stub.\n", hPhone, lpParams, dwSize);
    return 1;
}

DWORD WINAPI phoneGetButtonInfo(HPHONE hPhone, DWORD dwButtonLampID,
                                LPPHONEBUTTONINFO lpButtonInfo)
{
    FIXME(tapi, "(%04x, %08lx, %p): stub.\n", hPhone, dwButtonLampID, lpButtonInfo);
    return 0;
}

DWORD WINAPI phoneGetData(HPHONE hPhone, DWORD dwDataID, LPVOID lpData, DWORD dwSize)
{
    FIXME(tapi, "(%04x, %08ld, %p, %08ld): stub.\n", hPhone, dwDataID, lpData, dwSize);
    return 0;
}

DWORD WINAPI phoneGetDevCaps(HPHONEAPP hPhoneApp, DWORD dwDeviceID,
               DWORD dwAPIVersion, DWORD dwExtVersion, LPPHONECAPS lpPhoneCaps)
{
    FIXME(tapi, "(%04x, %08ld, %08lx, %08lx, %p): stub.\n", hPhoneApp, dwDeviceID, dwAPIVersion, dwExtVersion, lpPhoneCaps);
    return 0;
}

DWORD WINAPI phoneGetDisplay(HPHONE hPhone, LPVARSTRING lpDisplay)
{
    FIXME(tapi, "(%04x, %p): stub.\n", hPhone, lpDisplay);
    return 0;
}

DWORD WINAPI phoneGetGain(HPHONE hPhone, DWORD dwHookSwitchDev, LPDWORD lpdwGain)
{
    FIXME(tapi, "(%04x, %08lx, %p): stub.\n", hPhone, dwHookSwitchDev, lpdwGain);
    return 0;
}	
		
DWORD WINAPI phoneGetHookSwitch(HPHONE hPhone, LPDWORD lpdwHookSwitchDevs)
{
   FIXME(tapi, "(%04x, %p): stub.\n", hPhone, lpdwHookSwitchDevs);
   return 0;
}

DWORD WINAPI phoneGetID(HPHONE hPhone, LPVARSTRING lpDeviceID,
                        LPCSTR lpszDeviceClass)
{
    FIXME(tapi, "(%04x, %p, %s): stub.\n", hPhone, lpDeviceID, lpszDeviceClass);    return 0;
}

DWORD WINAPI phoneGetIcon(DWORD dwDeviceID, LPCSTR lpszDeviceClass,
		          HICON *lphIcon)
{
    FIXME(tapi, "(%08lx, %s, %p): stub.\n", dwDeviceID, lpszDeviceClass, lphIcon);
    return 0;
}

DWORD WINAPI phoneGetLamp(HPHONE hPhone, DWORD dwButtonLampID,
		          LPDWORD lpdwLampMode)
{
    FIXME(tapi, "(%04x, %08lx, %p): stub.\n", hPhone, dwButtonLampID, lpdwLampMode);
    return 0;
}

DWORD WINAPI phoneGetRing(HPHONE hPhone, LPDWORD lpdwRingMode, LPDWORD lpdwVolume)
{
    FIXME(tapi, "(%04x, %p, %p): stub.\n", hPhone, lpdwRingMode, lpdwVolume);
    return 0;
}

DWORD WINAPI phoneGetStatus(HPHONE hPhone, LPPHONESTATUS lpPhoneStatus)
{
    FIXME(tapi, "(%04x, %p): stub.\n", hPhone, lpPhoneStatus);
    return 0;
}

DWORD WINAPI phoneGetStatusMessages(HPHONE hPhone, LPDWORD lpdwPhoneStates,
		          LPDWORD lpdwButtonModes, LPDWORD lpdwButtonStates)
{
    FIXME(tapi, "(%04x, %p, %p, %p): stub.\n", hPhone, lpdwPhoneStates, lpdwButtonModes, lpdwButtonStates);
    return 0;
}

DWORD WINAPI phoneGetVolume(HPHONE hPhone, DWORD dwHookSwitchDevs,
		            LPDWORD lpdwVolume)
{
    FIXME(tapi, "(%04x, %08lx, %p): stub.\n", hPhone, dwHookSwitchDevs, lpdwVolume);
    return 0;
}

DWORD WINAPI phoneInitialize(LPHPHONEAPP lphPhoneApp, HINSTANCE hInstance, PHONECALLBACK lpfnCallback, LPCSTR lpszAppName, LPDWORD lpdwNumDevs)
{
    FIXME(tapi, "(%p, %04x, %p, %s, %p): stub.\n", lphPhoneApp, hInstance, lpfnCallback, lpszAppName, lpdwNumDevs);
    return 0;
}

DWORD WINAPI phoneNegotiateAPIVersion(HPHONEAPP hPhoneApp, DWORD dwDeviceID, DWORD dwAPILowVersion, DWORD dwAPIHighVersion, LPDWORD lpdwAPIVersion, LPPHONEEXTENSIONID lpExtensionID)
{
    FIXME(tapi, "(): stub.\n");
    return 0;
}

DWORD WINAPI phoneNegotiateExtVersion(HPHONEAPP hPhoneApp, DWORD dwDeviceID,
		                 DWORD dwAPIVersion, DWORD dwExtLowVersion,
				 DWORD dwExtHighVersion, LPDWORD lpdwExtVersion)
{
    FIXME(tapi, "(): stub.\n");
    return 0;
}

DWORD WINAPI phoneOpen(HPHONEAPP hPhoneApp, DWORD dwDeviceID, LPHPHONE lphPhone, DWORD dwAPIVersion, DWORD dwExtVersion, DWORD dwCallbackInstance, DWORD dwPrivileges)
{
    FIXME(tapi, "(): stub.\n");
    return 0;
}

DWORD WINAPI phoneSetButtonInfo(HPHONE hPhone, DWORD dwButtonLampID, LPPHONEBUTTONINFO lpButtonInfo)
{
    FIXME(tapi, "(%04x, %08lx, %p): stub.\n", hPhone, dwButtonLampID, lpButtonInfo);
    return 0;
}

DWORD WINAPI phoneSetData(HPHONE hPhone, DWORD dwDataID, LPVOID lpData, DWORD dwSize)
{
    FIXME(tapi, "(%04x, %08lx, %p, %ld): stub.\n", hPhone, dwDataID, lpData, dwSize);
    return 1;
}

DWORD WINAPI phoneSetDisplay(HPHONE hPhone, DWORD dwRow, DWORD dwColumn, LPCSTR lpszDisplay, DWORD dwSize)
{
    FIXME(tapi, "(%04x, '%s' at %ld/%ld, len %ld): stub.\n", hPhone, lpszDisplay, dwRow, dwColumn, dwSize);
    return 1;
}

DWORD WINAPI phoneSetGain(HPHONE hPhone, DWORD dwHookSwitchDev, DWORD dwGain)
{
    FIXME(tapi, "(%04x, %08lx, %ld): stub.\n", hPhone, dwHookSwitchDev, dwGain);
    return 1;
}

DWORD WINAPI phoneSetHookSwitch(HPHONE hPhone, DWORD dwHookSwitchDevs, DWORD dwHookSwitchMode)
{
    FIXME(tapi, "(%04x, %08lx, %08lx): stub.\n", hPhone, dwHookSwitchDevs, dwHookSwitchMode);
    return 1;
}

DWORD WINAPI phoneSetLamp(HPHONE hPhone, DWORD dwButtonLampID, DWORD lpdwLampMode)
{
    FIXME(tapi, "(%04x, %08lx, %08lx): stub.\n", hPhone, dwButtonLampID, lpdwLampMode);
    return 1;
}

DWORD WINAPI phoneSetRing(HPHONE hPhone, DWORD dwRingMode, DWORD dwVolume)
{
    FIXME(tapi, "(%04x, %08lx, %08ld): stub.\n", hPhone, dwRingMode, dwVolume);
    return 1;
}

DWORD WINAPI phoneSetStatusMessages(HPHONE hPhone, DWORD dwPhoneStates, DWORD dwButtonModes, DWORD dwButtonStates)
{
    FIXME(tapi, "(%04x, %08lx, %08lx, %08lx): stub.\n", hPhone, dwPhoneStates, dwButtonModes, dwButtonStates);
    return 0; /* FIXME ? */
}

DWORD WINAPI phoneSetVolume(HPHONE hPhone, DWORD dwHookSwitchDev, DWORD dwVolume)
{
    FIXME(tapi, "(%04x, %08lx, %08ld): stub.\n", hPhone, dwHookSwitchDev, dwVolume);
    return 1;
}

DWORD WINAPI phoneShutdown(HPHONEAPP hPhoneApp)
{
    FIXME(tapi, "(%04x): stub.\n", hPhoneApp);
    return 0;
}
