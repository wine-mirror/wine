/*  DirectInput Generic Joystick device
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2009 Aric Stewart, CodeWeavers
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

/*
 * To Do:
 *	dead zone
 *	force feedback
 */

#include "joystick_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

/******************************************************************************
  *     SetProperty : change input device properties
  */
HRESULT WINAPI JoystickAGenericImpl_SetProperty(
    LPDIRECTINPUTDEVICE8A iface,
    REFGUID rguid,
    LPCDIPROPHEADER ph)
{
    JoystickGenericImpl *This = (JoystickGenericImpl *)iface;
    DWORD i;

    TRACE("(%p,%s,%p)\n",This,debugstr_guid(rguid),ph);

    if (ph == NULL) {
        WARN("invalid parameter: ph == NULL\n");
        return DIERR_INVALIDPARAM;
    }

    if (TRACE_ON(dinput))
        _dump_DIPROPHEADER(ph);

    if (!HIWORD(rguid)) {
        switch (LOWORD(rguid)) {
        case (DWORD_PTR)DIPROP_RANGE: {
            LPCDIPROPRANGE pr = (LPCDIPROPRANGE)ph;
            if (ph->dwHow == DIPH_DEVICE) {
                TRACE("proprange(%d,%d) all\n", pr->lMin, pr->lMax);
                for (i = 0; i < This->base.data_format.wine_df->dwNumObjs; i++) {
                    This->props[i].lMin = pr->lMin;
                    This->props[i].lMax = pr->lMax;
                }
            } else {
                int obj = find_property(&This->base.data_format, ph);

                TRACE("proprange(%d,%d) obj=%d\n", pr->lMin, pr->lMax, obj);
                if (obj >= 0) {
                    This->props[obj].lMin = pr->lMin;
                    This->props[obj].lMax = pr->lMax;
                    return DI_OK;
                }
            }
            break;
        }
        case (DWORD_PTR)DIPROP_DEADZONE: {
            LPCDIPROPDWORD pd = (LPCDIPROPDWORD)ph;
            if (ph->dwHow == DIPH_DEVICE) {
                TRACE("deadzone(%d) all\n", pd->dwData);
                for (i = 0; i < This->base.data_format.wine_df->dwNumObjs; i++)
                    This->props[i].lDeadZone  = pd->dwData;
            } else {
                int obj = find_property(&This->base.data_format, ph);

                TRACE("deadzone(%d) obj=%d\n", pd->dwData, obj);
                if (obj >= 0) {
                    This->props[obj].lDeadZone  = pd->dwData;
                    return DI_OK;
                }
            }
            break;
        }
        case (DWORD_PTR)DIPROP_SATURATION: {
            LPCDIPROPDWORD pd = (LPCDIPROPDWORD)ph;
            if (ph->dwHow == DIPH_DEVICE) {
                TRACE("saturation(%d) all\n", pd->dwData);
                for (i = 0; i < This->base.data_format.wine_df->dwNumObjs; i++)
                    This->props[i].lSaturation = pd->dwData;
            } else {
                int obj = find_property(&This->base.data_format, ph);

                TRACE("saturation(%d) obj=%d\n", pd->dwData, obj);
                if (obj >= 0) {
                    This->props[obj].lSaturation = pd->dwData;
                    return DI_OK;
                }
            }
            break;
        }
        default:
            return IDirectInputDevice2AImpl_SetProperty(iface, rguid, ph);
        }
    }

    return DI_OK;
}

void _dump_DIDEVCAPS(const DIDEVCAPS *lpDIDevCaps)
{
    TRACE("dwSize: %d\n", lpDIDevCaps->dwSize);
    TRACE("dwFlags: %08x\n", lpDIDevCaps->dwFlags);
    TRACE("dwDevType: %08x %s\n", lpDIDevCaps->dwDevType,
          lpDIDevCaps->dwDevType == DIDEVTYPE_DEVICE ? "DIDEVTYPE_DEVICE" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_DEVICE ? "DIDEVTYPE_DEVICE" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_MOUSE ? "DIDEVTYPE_MOUSE" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_KEYBOARD ? "DIDEVTYPE_KEYBOARD" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_JOYSTICK ? "DIDEVTYPE_JOYSTICK" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_HID ? "DIDEVTYPE_HID" : "UNKNOWN");
    TRACE("dwAxes: %d\n", lpDIDevCaps->dwAxes);
    TRACE("dwButtons: %d\n", lpDIDevCaps->dwButtons);
    TRACE("dwPOVs: %d\n", lpDIDevCaps->dwPOVs);
    if (lpDIDevCaps->dwSize > sizeof(DIDEVCAPS_DX3)) {
        TRACE("dwFFSamplePeriod: %d\n", lpDIDevCaps->dwFFSamplePeriod);
        TRACE("dwFFMinTimeResolution: %d\n", lpDIDevCaps->dwFFMinTimeResolution);
        TRACE("dwFirmwareRevision: %d\n", lpDIDevCaps->dwFirmwareRevision);
        TRACE("dwHardwareRevision: %d\n", lpDIDevCaps->dwHardwareRevision);
        TRACE("dwFFDriverVersion: %d\n", lpDIDevCaps->dwFFDriverVersion);
    }
}

HRESULT WINAPI JoystickAGenericImpl_GetCapabilities(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVCAPS lpDIDevCaps)
{
    JoystickGenericImpl *This = (JoystickGenericImpl *)iface;
    int size;

    TRACE("%p->(%p)\n",iface,lpDIDevCaps);

    if (lpDIDevCaps == NULL) {
        WARN("invalid pointer\n");
        return E_POINTER;
    }

    size = lpDIDevCaps->dwSize;

    if (!(size == sizeof(DIDEVCAPS) || size == sizeof(DIDEVCAPS_DX3))) {
        WARN("invalid parameter\n");
        return DIERR_INVALIDPARAM;
    }

    CopyMemory(lpDIDevCaps, &This->devcaps, size);
    lpDIDevCaps->dwSize = size;

    if (TRACE_ON(dinput))
        _dump_DIDEVCAPS(lpDIDevCaps);

    return DI_OK;
}

/******************************************************************************
  *     GetObjectInfo : get object info
  */
HRESULT WINAPI JoystickWGenericImpl_GetObjectInfo(LPDIRECTINPUTDEVICE8W iface,
        LPDIDEVICEOBJECTINSTANCEW pdidoi, DWORD dwObj, DWORD dwHow)
{
    static const WCHAR axisW[] = {'A','x','i','s',' ','%','d',0};
    static const WCHAR povW[] = {'P','O','V',' ','%','d',0};
    static const WCHAR buttonW[] = {'B','u','t','t','o','n',' ','%','d',0};
    HRESULT res;

    res = IDirectInputDevice2WImpl_GetObjectInfo(iface, pdidoi, dwObj, dwHow);
    if (res != DI_OK) return res;

    if      (pdidoi->dwType & DIDFT_AXIS)
        sprintfW(pdidoi->tszName, axisW, DIDFT_GETINSTANCE(pdidoi->dwType));
    else if (pdidoi->dwType & DIDFT_POV)
        sprintfW(pdidoi->tszName, povW, DIDFT_GETINSTANCE(pdidoi->dwType));
    else if (pdidoi->dwType & DIDFT_BUTTON)
        sprintfW(pdidoi->tszName, buttonW, DIDFT_GETINSTANCE(pdidoi->dwType));

    _dump_OBJECTINSTANCEW(pdidoi);
    return res;
}

HRESULT WINAPI JoystickAGenericImpl_GetObjectInfo(LPDIRECTINPUTDEVICE8A iface,
        LPDIDEVICEOBJECTINSTANCEA pdidoi, DWORD dwObj, DWORD dwHow)
{
    HRESULT res;
    DIDEVICEOBJECTINSTANCEW didoiW;
    DWORD dwSize = pdidoi->dwSize;

    didoiW.dwSize = sizeof(didoiW);
    res = JoystickWGenericImpl_GetObjectInfo((LPDIRECTINPUTDEVICE8W)iface, &didoiW, dwObj, dwHow);
    if (res != DI_OK) return res;

    memset(pdidoi, 0, pdidoi->dwSize);
    memcpy(pdidoi, &didoiW, FIELD_OFFSET(DIDEVICEOBJECTINSTANCEW, tszName));
    pdidoi->dwSize = dwSize;
    WideCharToMultiByte(CP_ACP, 0, didoiW.tszName, -1, pdidoi->tszName,
                        sizeof(pdidoi->tszName), NULL, NULL);

    return res;
}

/******************************************************************************
  *     GetProperty : get input device properties
  */
HRESULT WINAPI JoystickAGenericImpl_GetProperty(
    LPDIRECTINPUTDEVICE8A iface,
    REFGUID rguid,
    LPDIPROPHEADER pdiph)
{
    JoystickGenericImpl *This = (JoystickGenericImpl *)iface;

    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(rguid), pdiph);

    if (TRACE_ON(dinput))
        _dump_DIPROPHEADER(pdiph);

    if (!HIWORD(rguid)) {
        switch (LOWORD(rguid)) {
        case (DWORD_PTR) DIPROP_RANGE: {
            LPDIPROPRANGE pr = (LPDIPROPRANGE)pdiph;
            int obj = find_property(&This->base.data_format, pdiph);

            /* The app is querying the current range of the axis
             * return the lMin and lMax values */
            if (obj >= 0) {
                pr->lMin = This->props[obj].lMin;
                pr->lMax = This->props[obj].lMax;
                TRACE("range(%d, %d) obj=%d\n", pr->lMin, pr->lMax, obj);
                return DI_OK;
            }
            break;
        }
        case (DWORD_PTR) DIPROP_DEADZONE: {
            LPDIPROPDWORD pd = (LPDIPROPDWORD)pdiph;
            int obj = find_property(&This->base.data_format, pdiph);

            if (obj >= 0) {
                pd->dwData = This->props[obj].lDeadZone;
                TRACE("deadzone(%d) obj=%d\n", pd->dwData, obj);
                return DI_OK;
            }
            break;
        }
        case (DWORD_PTR) DIPROP_SATURATION: {
            LPDIPROPDWORD pd = (LPDIPROPDWORD)pdiph;
            int obj = find_property(&This->base.data_format, pdiph);

            if (obj >= 0) {
                pd->dwData = This->props[obj].lSaturation;
                TRACE("saturation(%d) obj=%d\n", pd->dwData, obj);
                return DI_OK;
            }
            break;
        }
        default:
            return IDirectInputDevice2AImpl_GetProperty(iface, rguid, pdiph);
        }
    }

    return DI_OK;
}
