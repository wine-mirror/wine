/* Xbox ERA ETW+ telemetry DLL stubs
 *
 * Based on XWine1/SlimEra EtwPlus.cpp (MIT)
 * All ETW+ functions are no-ops - Xbox telemetry has no meaning on Linux.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "evntprov.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(etwplus);

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(inst);
    return TRUE;
}

ULONG WINAPI EtxEventWrite(void *EventDescriptor, void *ProviderDescriptor,
        REGHANDLE RegHandle, ULONG UserDataCount, EVENT_DATA_DESCRIPTOR *UserData)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}

ULONG WINAPI EtxRegister(void *ProviderDescriptor, PREGHANDLE RegHandle)
{
    TRACE("\n");
    if (RegHandle) *RegHandle = 0;
    return ERROR_SUCCESS;
}

void WINAPI EtxResumeUploading(void)
{
    TRACE("\n");
}

void WINAPI EtxSuspendUploading(void)
{
    TRACE("\n");
}

ULONG WINAPI EtxUnregister(REGHANDLE RegHandle)
{
    TRACE("(%I64x)\n", RegHandle);
    return ERROR_SUCCESS;
}
