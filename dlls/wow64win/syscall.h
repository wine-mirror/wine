/*
 * WoW64 USER32 syscall definitions
 *
 * Copyright 2021 Alexandre Julliard
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

#ifndef __WOW64WIN_SYSCALL_H
#define __WOW64WIN_SYSCALL_H

#define ALL_WIN32_SYSCALLS \
    SYSCALL_ENTRY( NtGdiAddFontMemResourceEx ) \
    SYSCALL_ENTRY( NtGdiAddFontResourceW ) \
    SYSCALL_ENTRY( NtGdiCombineRgn ) \
    SYSCALL_ENTRY( NtGdiCreateBitmap ) \
    SYSCALL_ENTRY( NtGdiCreateClientObj ) \
    SYSCALL_ENTRY( NtGdiCreateDIBBrush ) \
    SYSCALL_ENTRY( NtGdiCreateDIBSection ) \
    SYSCALL_ENTRY( NtGdiCreateEllipticRgn ) \
    SYSCALL_ENTRY( NtGdiCreateHalftonePalette ) \
    SYSCALL_ENTRY( NtGdiCreateHatchBrushInternal ) \
    SYSCALL_ENTRY( NtGdiCreatePaletteInternal ) \
    SYSCALL_ENTRY( NtGdiCreatePatternBrushInternal ) \
    SYSCALL_ENTRY( NtGdiCreatePen ) \
    SYSCALL_ENTRY( NtGdiCreateRectRgn ) \
    SYSCALL_ENTRY( NtGdiCreateRoundRectRgn ) \
    SYSCALL_ENTRY( NtGdiCreateSolidBrush ) \
    SYSCALL_ENTRY( NtGdiDdDDICloseAdapter ) \
    SYSCALL_ENTRY( NtGdiDdDDICreateDevice ) \
    SYSCALL_ENTRY( NtGdiDdDDIOpenAdapterFromDeviceName ) \
    SYSCALL_ENTRY( NtGdiDdDDIOpenAdapterFromHdc ) \
    SYSCALL_ENTRY( NtGdiDdDDIOpenAdapterFromLuid ) \
    SYSCALL_ENTRY( NtGdiDdDDIQueryStatistics ) \
    SYSCALL_ENTRY( NtGdiDdDDISetQueuedLimit ) \
    SYSCALL_ENTRY( NtGdiDeleteClientObj ) \
    SYSCALL_ENTRY( NtGdiDescribePixelFormat ) \
    SYSCALL_ENTRY( NtGdiDrawStream ) \
    SYSCALL_ENTRY( NtGdiEqualRgn ) \
    SYSCALL_ENTRY( NtGdiExtCreatePen ) \
    SYSCALL_ENTRY( NtGdiExtCreateRegion ) \
    SYSCALL_ENTRY( NtGdiExtGetObjectW ) \
    SYSCALL_ENTRY( NtGdiFlattenPath ) \
    SYSCALL_ENTRY( NtGdiFlush ) \
    SYSCALL_ENTRY( NtGdiGetBitmapBits ) \
    SYSCALL_ENTRY( NtGdiGetBitmapDimension ) \
    SYSCALL_ENTRY( NtGdiGetColorAdjustment ) \
    SYSCALL_ENTRY( NtGdiGetDCObject ) \
    SYSCALL_ENTRY( NtGdiGetFontFileData ) \
    SYSCALL_ENTRY( NtGdiGetFontFileInfo ) \
    SYSCALL_ENTRY( NtGdiGetNearestPaletteIndex ) \
    SYSCALL_ENTRY( NtGdiGetPath ) \
    SYSCALL_ENTRY( NtGdiGetRegionData ) \
    SYSCALL_ENTRY( NtGdiGetRgnBox ) \
    SYSCALL_ENTRY( NtGdiGetSpoolMessage ) \
    SYSCALL_ENTRY( NtGdiGetSystemPaletteUse ) \
    SYSCALL_ENTRY( NtGdiGetTransform ) \
    SYSCALL_ENTRY( NtGdiHfontCreate ) \
    SYSCALL_ENTRY( NtGdiInitSpool ) \
    SYSCALL_ENTRY( NtGdiOffsetRgn ) \
    SYSCALL_ENTRY( NtGdiPathToRegion ) \
    SYSCALL_ENTRY( NtGdiPtInRegion ) \
    SYSCALL_ENTRY( NtGdiRectInRegion ) \
    SYSCALL_ENTRY( NtGdiRemoveFontMemResourceEx ) \
    SYSCALL_ENTRY( NtGdiRemoveFontResourceW ) \
    SYSCALL_ENTRY( NtGdiSaveDC ) \
    SYSCALL_ENTRY( NtGdiSetBitmapBits ) \
    SYSCALL_ENTRY( NtGdiSetBitmapDimension ) \
    SYSCALL_ENTRY( NtGdiSetBrushOrg ) \
    SYSCALL_ENTRY( NtGdiSetColorAdjustment ) \
    SYSCALL_ENTRY( NtGdiSetMagicColors ) \
    SYSCALL_ENTRY( NtGdiSetMetaRgn ) \
    SYSCALL_ENTRY( NtGdiSetPixelFormat ) \
    SYSCALL_ENTRY( NtGdiSetRectRgn ) \
    SYSCALL_ENTRY( NtGdiSetTextJustification ) \
    SYSCALL_ENTRY( NtGdiSetVirtualResolution ) \
    SYSCALL_ENTRY( NtGdiSwapBuffers ) \
    SYSCALL_ENTRY( NtGdiTransformPoints ) \
    SYSCALL_ENTRY( NtUserAddClipboardFormatListener ) \
    SYSCALL_ENTRY( NtUserAttachThreadInput ) \
    SYSCALL_ENTRY( NtUserBuildHwndList ) \
    SYSCALL_ENTRY( NtUserCloseDesktop ) \
    SYSCALL_ENTRY( NtUserCloseWindowStation ) \
    SYSCALL_ENTRY( NtUserCopyAcceleratorTable ) \
    SYSCALL_ENTRY( NtUserCreateAcceleratorTable ) \
    SYSCALL_ENTRY( NtUserCreateDesktopEx ) \
    SYSCALL_ENTRY( NtUserCreateWindowStation ) \
    SYSCALL_ENTRY( NtUserDestroyAcceleratorTable ) \
    SYSCALL_ENTRY( NtUserFindExistingCursorIcon ) \
    SYSCALL_ENTRY( NtUserGetAncestor ) \
    SYSCALL_ENTRY( NtUserGetClipboardFormatName ) \
    SYSCALL_ENTRY( NtUserGetClipboardOwner ) \
    SYSCALL_ENTRY( NtUserGetClipboardSequenceNumber ) \
    SYSCALL_ENTRY( NtUserGetClipboardViewer ) \
    SYSCALL_ENTRY( NtUserGetCursor ) \
    SYSCALL_ENTRY( NtUserGetCursorFrameInfo ) \
    SYSCALL_ENTRY( NtUserGetDoubleClickTime ) \
    SYSCALL_ENTRY( NtUserGetDpiForMonitor ) \
    SYSCALL_ENTRY( NtUserGetForegroundWindow ) \
    SYSCALL_ENTRY( NtUserGetGUIThreadInfo ) \
    SYSCALL_ENTRY( NtUserGetIconSize ) \
    SYSCALL_ENTRY( NtUserGetKeyState ) \
    SYSCALL_ENTRY( NtUserGetKeyboardLayout ) \
    SYSCALL_ENTRY( NtUserGetKeyboardLayoutName ) \
    SYSCALL_ENTRY( NtUserGetKeyboardState ) \
    SYSCALL_ENTRY( NtUserGetLayeredWindowAttributes ) \
    SYSCALL_ENTRY( NtUserGetMouseMovePointsEx ) \
    SYSCALL_ENTRY( NtUserGetObjectInformation ) \
    SYSCALL_ENTRY( NtUserGetOpenClipboardWindow ) \
    SYSCALL_ENTRY( NtUserGetProcessDpiAwarenessContext ) \
    SYSCALL_ENTRY( NtUserGetProcessWindowStation ) \
    SYSCALL_ENTRY( NtUserGetProp ) \
    SYSCALL_ENTRY( NtUserGetSystemDpiForProcess ) \
    SYSCALL_ENTRY( NtUserGetThreadDesktop ) \
    SYSCALL_ENTRY( NtUserGetWindowRgnEx ) \
    SYSCALL_ENTRY( NtUserInitializeClientPfnArrays ) \
    SYSCALL_ENTRY( NtUserInternalGetWindowText ) \
    SYSCALL_ENTRY( NtUserNotifyWinEvent ) \
    SYSCALL_ENTRY( NtUserOpenDesktop ) \
    SYSCALL_ENTRY( NtUserOpenInputDesktop ) \
    SYSCALL_ENTRY( NtUserOpenWindowStation ) \
    SYSCALL_ENTRY( NtUserRemoveClipboardFormatListener ) \
    SYSCALL_ENTRY( NtUserRemoveProp ) \
    SYSCALL_ENTRY( NtUserSetKeyboardState ) \
    SYSCALL_ENTRY( NtUserSetObjectInformation ) \
    SYSCALL_ENTRY( NtUserSetProcessDpiAwarenessContext ) \
    SYSCALL_ENTRY( NtUserSetProcessWindowStation ) \
    SYSCALL_ENTRY( NtUserSetProp ) \
    SYSCALL_ENTRY( NtUserSetThreadDesktop ) \
    SYSCALL_ENTRY( NtUserSetWinEventHook ) \
    SYSCALL_ENTRY( NtUserSetWindowsHookEx ) \
    SYSCALL_ENTRY( NtUserUnhookWinEvent ) \
    SYSCALL_ENTRY( NtUserUnhookWindowsHookEx )

#endif /* __WOW64WIN_SYSCALL_H */
