name	x11drv
type	win32
init	X11DRV_Init

import	user32.dll
import	gdi32.dll
import	kernel32.dll

debug_channels (bitblt bitmap clipboard cursor dinput event font gdi graphics
                key keyboard opengl palette text win x11drv)

# USER driver

@ cdecl Synchronize() X11DRV_Synchronize
@ cdecl CheckFocus() X11DRV_CheckFocus
@ cdecl UserRepaintDisable(long) X11DRV_UserRepaintDisable
@ cdecl InitKeyboard() X11DRV_InitKeyboard
@ cdecl VkKeyScan(long) X11DRV_VkKeyScan
@ cdecl MapVirtualKey(long long) X11DRV_MapVirtualKey
@ cdecl GetKeyNameText(long str long) X11DRV_GetKeyNameText
@ cdecl ToUnicode(long long ptr ptr long long) X11DRV_ToUnicode
@ cdecl GetBeepActive() X11DRV_GetBeepActive
@ cdecl SetBeepActive(long) X11DRV_SetBeepActive
@ cdecl Beep() X11DRV_Beep
@ cdecl GetDIState(long ptr) X11DRV_GetDIState
@ cdecl GetDIData(ptr long ptr ptr long) X11DRV_GetDIData
@ cdecl GetKeyboardConfig(ptr) X11DRV_GetKeyboardConfig
@ cdecl SetKeyboardConfig(ptr long) X11DRV_SetKeyboardConfig
@ cdecl InitMouse(ptr) X11DRV_InitMouse
@ cdecl SetCursor(ptr) X11DRV_SetCursor
@ cdecl MoveCursor(long long) X11DRV_MoveCursor
@ cdecl GetScreenSaveActive() X11DRV_GetScreenSaveActive
@ cdecl SetScreenSaveActive(long) X11DRV_SetScreenSaveActive
@ cdecl GetScreenSaveTimeout() X11DRV_GetScreenSaveTimeout
@ cdecl SetScreenSaveTimeout(long) X11DRV_SetScreenSaveTimeout
@ cdecl LoadOEMResource(long long) X11DRV_LoadOEMResource
@ cdecl IsSingleWindow() X11DRV_IsSingleWindow
@ cdecl AcquireClipboard() X11DRV_AcquireClipboard
@ cdecl ReleaseClipboard() X11DRV_ReleaseClipboard
@ cdecl SetClipboardData(long) X11DRV_SetClipboardData
@ cdecl GetClipboardData(long) X11DRV_GetClipboardData
@ cdecl IsClipboardFormatAvailable(long) X11DRV_IsClipboardFormatAvailable
@ cdecl RegisterClipboardFormat(str) X11DRV_RegisterClipboardFormat
@ cdecl IsSelectionOwner() X11DRV_IsSelectionOwner
@ cdecl ResetSelectionOwner(ptr long) X11DRV_ResetSelectionOwner
