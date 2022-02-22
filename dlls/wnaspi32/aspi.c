/**************************************************************************
 * ASPI routines
 * Copyright (C) 2000 David Elliott <dfe@infinite-internet.net>
 * Copyright (C) 2005 Vitaliy Margolen
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

/* These routines are to be called from either WNASPI32 or WINASPI */

/* FIXME:
 * - No way to override automatic /proc detection, maybe provide an
 *   HKEY_LOCAL_MACHINE\Software\Wine\Wine\Scsi regkey
 * - Somewhat debating an #ifdef linux... technically all this code will
 *   run on another UNIX.. it will fail nicely.
 * - Please add support for mapping multiple channels on host adapters to
 *   aspi controllers, e-mail me if you need help.
 */

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winerror.h"
#include "winescsi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(aspi);

static const WCHAR wDevicemapScsi[] = {'H','A','R','D','W','A','R','E','\\','D','E','V','I','C','E','M','A','P','\\','S','c','s','i',0};

/* Exported functions */
int ASPI_GetNumControllers(void)
{
    HKEY hkeyScsi, hkeyPort;
    DWORD i = 0, numPorts, num_ha = 0;
    WCHAR wPortName[15];

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, wDevicemapScsi, 0,
        KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hkeyScsi) != ERROR_SUCCESS )
    {
        ERR("Could not open HKLM\\%s\n", debugstr_w(wDevicemapScsi));
        return 0;
    }
    while (RegEnumKeyW(hkeyScsi, i++, wPortName, ARRAY_SIZE(wPortName)) == ERROR_SUCCESS)
    {
        if (RegOpenKeyExW(hkeyScsi, wPortName, 0, KEY_QUERY_VALUE, &hkeyPort) == ERROR_SUCCESS)
        {
            if (RegQueryInfoKeyW(hkeyPort, NULL, NULL, NULL, &numPorts, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                num_ha += numPorts;
            }
            RegCloseKey(hkeyPort);
        }
    }

    RegCloseKey(hkeyScsi);
    TRACE("Returning %ld host adapters\n", num_ha );
    return num_ha;
}

/* SCSI_GetHCforController
 * RETURNS
 * 	HIWORD: Host Adapter
 * 	LOWORD: Channel
 */
DWORD ASPI_GetHCforController( int controller )
{
    HKEY hkeyScsi, hkeyPort;
    DWORD i = 0, numPorts;
    int num_ha = controller + 1;
    WCHAR wPortName[15];
    WCHAR wBusName[15];

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, wDevicemapScsi, 0,
        KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hkeyScsi) != ERROR_SUCCESS )
    {
        ERR("Could not open HKLM\\%s\n", debugstr_w(wDevicemapScsi));
        return 0xFFFFFFFF;
    }
    while (RegEnumKeyW(hkeyScsi, i++, wPortName, ARRAY_SIZE(wPortName)) == ERROR_SUCCESS)
    {
        if (RegOpenKeyExW(hkeyScsi, wPortName, 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                          &hkeyPort) == ERROR_SUCCESS)
        {
            if (RegQueryInfoKeyW(hkeyPort, NULL, NULL, NULL, &numPorts, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                num_ha -= numPorts;
                if (num_ha <= 0) break;
            }
            else
            RegCloseKey(hkeyPort);
        }
    }
    RegCloseKey(hkeyScsi);

    if (num_ha > 0)
    {
        ERR("Invalid controller(%d)\n", controller);
        return 0xFFFFFFFF;
    }

    if (RegEnumKeyW(hkeyPort, -num_ha, wBusName, ARRAY_SIZE(wBusName)) != ERROR_SUCCESS)
    {
        ERR("Failed to enumerate keys\n");
        RegCloseKey(hkeyPort);
        return 0xFFFFFFFF;
    }
    RegCloseKey(hkeyPort);

    return (wcstol(&wPortName[9], NULL, 10) << 16) + wcstol(&wBusName[9], NULL, 10);
}
