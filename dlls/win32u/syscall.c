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
#include "ntuser_private.h"
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
    NtGdiDdDDICreateDevice,
    NtGdiDdDDIOpenAdapterFromHdc,
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
    NtGdiGetDCDword,
    NtGdiGetDCObject,
    NtGdiGetDCPoint,
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
    NtUserActivateKeyboardLayout,
    NtUserAddClipboardFormatListener,
    NtUserAssociateInputContext,
    NtUserAttachThreadInput,
    NtUserBeginPaint,
    NtUserBuildHwndList,
    NtUserCallHwnd,
    NtUserCallHwndParam,
    NtUserCallMsgFilter,
    NtUserCallNextHookEx,
    NtUserCallNoParam,
    NtUserCallOneParam,
    NtUserCallTwoParam,
    NtUserChangeClipboardChain,
    NtUserChangeDisplaySettings,
    NtUserCheckMenuItem,
    NtUserChildWindowFromPointEx,
    NtUserClipCursor,
    NtUserCloseClipboard,
    NtUserCloseDesktop,
    NtUserCloseWindowStation,
    NtUserCopyAcceleratorTable,
    NtUserCountClipboardFormats,
    NtUserCreateAcceleratorTable,
    NtUserCreateCaret,
    NtUserCreateDesktopEx,
    NtUserCreateInputContext,
    NtUserCreateWindowEx,
    NtUserCreateWindowStation,
    NtUserDeferWindowPosAndBand,
    NtUserDeleteMenu,
    NtUserDestroyAcceleratorTable,
    NtUserDestroyCursor,
    NtUserDestroyInputContext,
    NtUserDestroyMenu,
    NtUserDestroyWindow,
    NtUserDisableThreadIme,
    NtUserDispatchMessage,
    NtUserDisplayConfigGetDeviceInfo,
    NtUserDragDetect,
    NtUserDragObject,
    NtUserDrawIconEx,
    NtUserEmptyClipboard,
    NtUserEnableMenuItem,
    NtUserEnableScrollBar,
    NtUserEndDeferWindowPosEx,
    NtUserEndMenu,
    NtUserEnumDisplayDevices,
    NtUserEnumDisplayMonitors,
    NtUserEnumDisplaySettings,
    NtUserFindExistingCursorIcon,
    NtUserFindWindowEx,
    NtUserFlashWindowEx,
    NtUserGetAncestor,
    NtUserGetAsyncKeyState,
    NtUserGetAtomName,
    NtUserGetCaretBlinkTime,
    NtUserGetCaretPos,
    NtUserGetClassInfoEx,
    NtUserGetClassName,
    NtUserGetClipboardData,
    NtUserGetClipboardFormatName,
    NtUserGetClipboardOwner,
    NtUserGetClipboardSequenceNumber,
    NtUserGetClipboardViewer,
    NtUserGetCursor,
    NtUserGetCursorFrameInfo,
    NtUserGetCursorInfo,
    NtUserGetDC,
    NtUserGetDCEx,
    NtUserGetDisplayConfigBufferSizes,
    NtUserGetDoubleClickTime,
    NtUserGetDpiForMonitor,
    NtUserGetForegroundWindow,
    NtUserGetGUIThreadInfo,
    NtUserGetIconInfo,
    NtUserGetIconSize,
    NtUserGetInternalWindowPos,
    NtUserGetKeyNameText,
    NtUserGetKeyState,
    NtUserGetKeyboardLayout,
    NtUserGetKeyboardLayoutList,
    NtUserGetKeyboardLayoutName,
    NtUserGetKeyboardState,
    NtUserGetLayeredWindowAttributes,
    NtUserGetMenuBarInfo,
    NtUserGetMenuItemRect,
    NtUserGetMessage,
    NtUserGetMouseMovePointsEx,
    NtUserGetObjectInformation,
    NtUserGetOpenClipboardWindow,
    NtUserGetPriorityClipboardFormat,
    NtUserGetProcessDpiAwarenessContext,
    NtUserGetProcessWindowStation,
    NtUserGetProp,
    NtUserGetQueueStatus,
    NtUserGetRawInputBuffer,
    NtUserGetRawInputData,
    NtUserGetRawInputDeviceInfo,
    NtUserGetRawInputDeviceList,
    NtUserGetRegisteredRawInputDevices,
    NtUserGetScrollBarInfo,
    NtUserGetSystemDpiForProcess,
    NtUserGetSystemMenu,
    NtUserGetThreadDesktop,
    NtUserGetTitleBarInfo,
    NtUserGetUpdateRect,
    NtUserGetUpdateRgn,
    NtUserGetUpdatedClipboardFormats,
    NtUserGetWindowDC,
    NtUserGetWindowPlacement,
    NtUserGetWindowRgnEx,
    NtUserHideCaret,
    NtUserHiliteMenuItem,
    NtUserInitializeClientPfnArrays,
    NtUserInternalGetWindowIcon,
    NtUserInternalGetWindowText,
    NtUserInvalidateRect,
    NtUserInvalidateRgn,
    NtUserIsClipboardFormatAvailable,
    NtUserKillTimer,
    NtUserLockWindowUpdate,
    NtUserLogicalToPerMonitorDPIPhysicalPoint,
    NtUserMapVirtualKeyEx,
    NtUserMenuItemFromPoint,
    NtUserMessageCall,
    NtUserMoveWindow,
    NtUserMsgWaitForMultipleObjectsEx,
    NtUserNotifyWinEvent,
    NtUserOpenClipboard,
    NtUserOpenDesktop,
    NtUserOpenInputDesktop,
    NtUserOpenWindowStation,
    NtUserPeekMessage,
    NtUserPerMonitorDPIPhysicalToLogicalPoint,
    NtUserPostMessage,
    NtUserPostThreadMessage,
    NtUserPrintWindow,
    NtUserQueryInputContext,
    NtUserRealChildWindowFromPoint,
    NtUserRedrawWindow,
    NtUserRegisterClassExWOW,
    NtUserRegisterHotKey,
    NtUserRegisterRawInputDevices,
    NtUserRemoveClipboardFormatListener,
    NtUserRemoveMenu,
    NtUserRemoveProp,
    NtUserScrollWindowEx,
    NtUserSendInput,
    NtUserSetActiveWindow,
    NtUserSetCapture,
    NtUserSetClassLong,
    NtUserSetClassLongPtr,
    NtUserSetClassWord,
    NtUserSetClipboardData,
    NtUserSetClipboardViewer,
    NtUserSetCursor,
    NtUserSetCursorIconData,
    NtUserSetCursorPos,
    NtUserSetFocus,
    NtUserSetInternalWindowPos,
    NtUserSetKeyboardState,
    NtUserSetLayeredWindowAttributes,
    NtUserSetMenu,
    NtUserSetMenuContextHelpId,
    NtUserSetMenuDefaultItem,
    NtUserSetObjectInformation,
    NtUserSetParent,
    NtUserSetProcessDpiAwarenessContext,
    NtUserSetProcessWindowStation,
    NtUserSetProp,
    NtUserSetScrollInfo,
    NtUserSetShellWindowEx,
    NtUserSetSysColors,
    NtUserSetSystemMenu,
    NtUserSetSystemTimer,
    NtUserSetThreadDesktop,
    NtUserSetTimer,
    NtUserSetWinEventHook,
    NtUserSetWindowLong,
    NtUserSetWindowLongPtr,
    NtUserSetWindowPlacement,
    NtUserSetWindowPos,
    NtUserSetWindowRgn,
    NtUserSetWindowWord,
    NtUserSetWindowsHookEx,
    NtUserShowCaret,
    NtUserShowCursor,
    NtUserShowScrollBar,
    NtUserShowWindow,
    NtUserShowWindowAsync,
    NtUserSystemParametersInfo,
    NtUserSystemParametersInfoForDpi,
    NtUserThunkedMenuInfo,
    NtUserThunkedMenuItemInfo,
    NtUserToUnicodeEx,
    NtUserTrackMouseEvent,
    NtUserTrackPopupMenuEx,
    NtUserTranslateAccelerator,
    NtUserTranslateMessage,
    NtUserUnhookWinEvent,
    NtUserUnhookWindowsHookEx,
    NtUserUnregisterClass,
    NtUserUnregisterHotKey,
    NtUserUpdateInputContext,
    NtUserValidateRect,
    NtUserVkKeyScanEx,
    NtUserWaitForInputIdle,
    NtUserWaitMessage,
    NtUserWindowFromDC,
    NtUserWindowFromPoint,
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
    return ntdll_init_syscalls( 1, &syscall_table, dispatcher );
}

unixlib_entry_t __wine_unix_call_funcs[] =
{
    init,
    callbacks_init,
};

#ifdef _WIN64

WINE_DEFAULT_DEBUG_CHANNEL(win32u);

static NTSTATUS wow64_init( void *args )
{
    FIXME( "\n" );
    return STATUS_NOT_SUPPORTED;
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    init,
    wow64_init,
};

#endif /* _WIN64 */
