/*
 * Unix interface for Win32 syscalls
 *
 * Copyright (C) 2021 Alexandre Julliard
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

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "ntgdi_private.h"
#include "ntuser.h"
#include "wine/unixlib.h"


static void * const syscalls[] =
{
    NtGdiAddFontMemResourceEx,
    NtGdiAddFontResourceW,
    NtGdiCombineRgn,
    NtGdiCreateBitmap,
    NtGdiCreateClientObj,
    NtGdiCreateDIBBrush,
    NtGdiCreateDIBSection,
    NtGdiCreateEllipticRgn,
    NtGdiCreateHalftonePalette,
    NtGdiCreateHatchBrushInternal,
    NtGdiCreatePaletteInternal,
    NtGdiCreatePatternBrushInternal,
    NtGdiCreatePen,
    NtGdiCreateRectRgn,
    NtGdiCreateRoundRectRgn,
    NtGdiCreateSolidBrush,
    NtGdiDdDDICloseAdapter,
    NtGdiDdDDICreateDevice,
    NtGdiDdDDIOpenAdapterFromDeviceName,
    NtGdiDdDDIOpenAdapterFromHdc,
    NtGdiDdDDIOpenAdapterFromLuid,
    NtGdiDdDDIQueryStatistics,
    NtGdiDdDDISetQueuedLimit,
    NtGdiDeleteClientObj,
    NtGdiDescribePixelFormat,
    NtGdiDrawStream,
    NtGdiEqualRgn,
    NtGdiExtCreatePen,
    NtGdiExtCreateRegion,
    NtGdiExtGetObjectW,
    NtGdiFlattenPath,
    NtGdiFlush,
    NtGdiGetBitmapBits,
    NtGdiGetBitmapDimension,
    NtGdiGetColorAdjustment,
    NtGdiGetDCObject,
    NtGdiGetFontFileData,
    NtGdiGetFontFileInfo,
    NtGdiGetNearestPaletteIndex,
    NtGdiGetPath,
    NtGdiGetRegionData,
    NtGdiGetRgnBox,
    NtGdiGetSpoolMessage,
    NtGdiGetSystemPaletteUse,
    NtGdiGetTransform,
    NtGdiHfontCreate,
    NtGdiInitSpool,
    NtGdiOffsetRgn,
    NtGdiPathToRegion,
    NtGdiPtInRegion,
    NtGdiRectInRegion,
    NtGdiRemoveFontMemResourceEx,
    NtGdiRemoveFontResourceW,
    NtGdiSaveDC,
    NtGdiSetBitmapBits,
    NtGdiSetBitmapDimension,
    NtGdiSetBrushOrg,
    NtGdiSetColorAdjustment,
    NtGdiSetMagicColors,
    NtGdiSetMetaRgn,
    NtGdiSetPixelFormat,
    NtGdiSetRectRgn,
    NtGdiSetTextJustification,
    NtGdiSetVirtualResolution,
    NtGdiSwapBuffers,
    NtGdiTransformPoints,
    NtUserCloseDesktop,
    NtUserCloseWindowStation,
    NtUserCreateDesktopEx,
    NtUserCreateWindowStation,
    NtUserGetObjectInformation,
    NtUserGetProcessWindowStation,
    NtUserGetThreadDesktop,
    NtUserOpenDesktop,
    NtUserOpenInputDesktop,
    NtUserOpenWindowStation,
    NtUserSetObjectInformation,
    NtUserSetProcessWindowStation,
    NtUserSetThreadDesktop,
};

static BYTE arguments[ARRAY_SIZE(syscalls)];

static SYSTEM_SERVICE_TABLE syscall_table =
{
    (ULONG_PTR *)syscalls,
    0,
    ARRAY_SIZE(syscalls),
    arguments
};

static NTSTATUS init( void *dispatcher )
{
    NTSTATUS status;
    if ((status = ntdll_init_syscalls( 1, &syscall_table, dispatcher ))) return status;
    if ((status = gdi_init())) return status;
    winstation_init();
    return STATUS_SUCCESS;
}

unixlib_entry_t __wine_unix_call_funcs[] =
{
    init,
    callbacks_init,
};
