/*
 * Copyright 2014 Michael Müller for Pipelight
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "d3d9.h"
#include "dxva2api.h"
#include "physicalmonitorenumerationapi.h"
#include "lowlevelmonitorconfigurationapi.h"
#include "highlevelmonitorconfigurationapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxva2);

BOOL WINAPI CapabilitiesRequestAndCapabilitiesReply( HMONITOR monitor, LPSTR buffer, DWORD length )
{
    FIXME("(%p, %p, %d): stub\n", monitor, buffer, length);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

HRESULT WINAPI DXVA2CreateDirect3DDeviceManager9( UINT *resetToken, IDirect3DDeviceManager9 **dxvManager )
{
    FIXME("(%p, %p): stub\n", resetToken, dxvManager);

    return E_NOTIMPL;
}

HRESULT WINAPI DXVA2CreateVideoService( IDirect3DDevice9 *device, REFIID riid, void **ppv )
{
    FIXME("(%p, %s, %p): stub\n", device, debugstr_guid(riid), ppv);

    return E_NOTIMPL;
}

BOOL WINAPI DegaussMonitor( HMONITOR monitor )
{
    FIXME("(%p): stub\n", monitor);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI DestroyPhysicalMonitor( HMONITOR monitor )
{
    FIXME("(%p): stub\n", monitor);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI DestroyPhysicalMonitors( DWORD arraySize, LPPHYSICAL_MONITOR array )
{
    FIXME("(0x%x, %p): stub\n", arraySize, array);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetCapabilitiesStringLength( HMONITOR monitor, LPDWORD length )
{
    FIXME("(%p, %p): stub\n", monitor, length);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorBrightness( HMONITOR monitor, LPDWORD minimum, LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, %p, %p, %p): stub\n", monitor, minimum, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorCapabilities( HMONITOR monitor, LPDWORD capabilities, LPDWORD temperatures )
{
    FIXME("(%p, %p, %p): stub\n", monitor, capabilities, temperatures);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL WINAPI GetMonitorColorTemperature( HMONITOR monitor, LPMC_COLOR_TEMPERATURE temperature )
{
    FIXME("(%p, %p): stub\n", monitor, temperature);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorContrast( HMONITOR monitor, LPDWORD minimum, LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, %p, %p, %p): stub\n", monitor, minimum, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorDisplayAreaPosition( HMONITOR monitor, MC_POSITION_TYPE type, LPDWORD minimum,
                                           LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, 0x%x, %p, %p, %p): stub\n", monitor, type, minimum, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorDisplayAreaSize( HMONITOR monitor, MC_SIZE_TYPE type, LPDWORD minimum,
                                       LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, 0x%x, %p, %p, %p): stub\n", monitor, type, minimum, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorRedGreenOrBlueDrive( HMONITOR monitor, MC_DRIVE_TYPE type, LPDWORD minimum,
                                           LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, 0x%x, %p, %p, %p): stub\n", monitor, type, minimum, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorRedGreenOrBlueGain( HMONITOR monitor, MC_GAIN_TYPE type, LPDWORD minimum,
                                          LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, 0x%x, %p, %p, %p): stub\n", monitor, type, minimum, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorTechnologyType( HMONITOR monitor, LPMC_DISPLAY_TECHNOLOGY_TYPE type )
{
    FIXME("(%p, %p): stub\n", monitor, type);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetNumberOfPhysicalMonitorsFromHMONITOR( HMONITOR monitor, LPDWORD number )
{
    FIXME("(%p, %p): stub\n", monitor, number);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

HRESULT WINAPI GetNumberOfPhysicalMonitorsFromIDirect3DDevice9( IDirect3DDevice9 *device, LPDWORD number )
{
    FIXME("(%p, %p): stub\n", device, number);

    return E_NOTIMPL;
}

BOOL WINAPI GetPhysicalMonitorsFromHMONITOR( HMONITOR monitor, DWORD arraySize, LPPHYSICAL_MONITOR array )
{
    FIXME("(%p, 0x%x, %p): stub\n", monitor, arraySize, array);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

HRESULT WINAPI GetPhysicalMonitorsFromIDirect3DDevice9( IDirect3DDevice9 *device, DWORD arraySize, LPPHYSICAL_MONITOR array )
{
    FIXME("(%p, 0x%x, %p): stub\n", device, arraySize, array);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetTimingReport( HMONITOR monitor, LPMC_TIMING_REPORT timingReport )
{
    FIXME("(%p, %p): stub\n", monitor, timingReport);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetVCPFeatureAndVCPFeatureReply( HMONITOR monitor, BYTE vcpCode, LPMC_VCP_CODE_TYPE pvct,
                                             LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, 0x%02x, %p, %p, %p): stub\n", monitor, vcpCode, pvct, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

HRESULT WINAPI OPMGetVideoOutputsFromHMONITOR( HMONITOR monitor, /* OPM_VIDEO_OUTPUT_SEMANTICS */ int vos,
                                               ULONG *numVideoOutputs, /* IOPMVideoOutput */ void ***videoOutputs )
{
    FIXME("(%p, 0x%x, %p, %p): stub\n", monitor, vos, numVideoOutputs, videoOutputs);

    return E_NOTIMPL;
}

HRESULT WINAPI OPMGetVideoOutputsFromIDirect3DDevice9Object( IDirect3DDevice9 *device, /* OPM_VIDEO_OUTPUT_SEMANTICS */ int vos,
                                                             ULONG *numVideoOutputs,  /* IOPMVideoOutput */ void ***videoOutputs )
{
    FIXME("(%p, 0x%x, %p, %p): stub\n", device, vos, numVideoOutputs, videoOutputs);

    return E_NOTIMPL;
}

BOOL WINAPI RestoreMonitorFactoryColorDefaults( HMONITOR monitor )
{
    FIXME("(%p): stub\n", monitor);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI RestoreMonitorFactoryDefaults( HMONITOR monitor )
{
    FIXME("(%p): stub\n", monitor);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SaveCurrentMonitorSettings( HMONITOR monitor )
{
    FIXME("(%p): stub\n", monitor);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SaveCurrentSettings( HMONITOR monitor )
{
    FIXME("(%p): stub\n", monitor);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorBrightness( HMONITOR monitor, DWORD brightness )
{
    FIXME("(%p, 0x%x): stub\n", monitor, brightness);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorColorTemperature( HMONITOR monitor, MC_COLOR_TEMPERATURE temperature )
{
    FIXME("(%p, 0x%x): stub\n", monitor, temperature);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorContrast( HMONITOR monitor, DWORD contrast )
{
    FIXME("(%p, 0x%x): stub\n", monitor, contrast);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorDisplayAreaPosition( HMONITOR monitor, MC_POSITION_TYPE type, DWORD position )
{
    FIXME("(%p, 0x%x, 0x%x): stub\n", monitor, type, position);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorDisplayAreaSize( HMONITOR monitor, MC_SIZE_TYPE type, DWORD size )
{
    FIXME("(%p, 0x%x, 0x%x): stub\n", monitor, type, size);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorRedGreenOrBlueDrive( HMONITOR monitor, MC_DRIVE_TYPE type, DWORD drive )
{
    FIXME("(%p, 0x%x, 0x%x): stub\n", monitor, type, drive);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorRedGreenOrBlueGain( HMONITOR monitor, MC_GAIN_TYPE type, DWORD gain )
{
    FIXME("(%p, 0x%x, 0x%x): stub\n", monitor, type, gain);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetVCPFeature( HMONITOR monitor, BYTE vcpCode, DWORD value )
{
    FIXME("(%p, 0x%02x, 0x%x): stub\n", monitor, vcpCode, value);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%x,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
        case DLL_WINE_PREATTACH:
            return FALSE;  /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}
