/* Xbox PIX GPU capture event stubs
 *
 * Based on XWine1/SlimEra pixEvt.cpp (MIT)
 * PIX GPU capture has no meaning outside Xbox/Windows PIX tooling.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(pixevt);

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(inst);
    return TRUE;
}

HRESULT WINAPI PIXBeginCapture(DWORD flags, const void *params)
{
    TRACE("(%08lx, %p)\n", flags, params);
    return S_OK;
}

HRESULT WINAPI PIXEndCapture(BOOL discard)
{
    TRACE("(%d)\n", discard);
    return S_OK;
}

UINT64 WINAPI PIXEventsReplaceBlock(BOOL getEarliestTime)
{
    TRACE("(%d)\n", getEarliestTime);
    return 0;
}

DWORD WINAPI PIXGetCaptureState(void)
{
    TRACE("\n");
    return 0;
}

void WINAPI PIXReportCounter(LPCWSTR name, float value)
{
    TRACE("(%s, %f)\n", debugstr_w(name), value);
}

HRESULT WINAPI ConfigurePMCs(UINT e0, UINT e1, UINT e2, UINT e3)
{
    TRACE("(%u, %u, %u, %u)\n", e0, e1, e2, e3);
    return S_OK;
}
