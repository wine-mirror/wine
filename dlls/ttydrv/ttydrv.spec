name	ttydrv
type	win32
init	TTYDRV_Init

import	user32.dll
import	gdi32.dll
import	kernel32.dll
import	ntdll.dll

debug_channels (ttydrv)

# USER driver

@ cdecl Synchronize() TTYDRV_Synchronize
@ cdecl CheckFocus() TTYDRV_CheckFocus
@ cdecl UserRepaintDisable(long) TTYDRV_UserRepaintDisable
@ cdecl InitKeyboard() TTYDRV_InitKeyboard
@ cdecl VkKeyScan(long) TTYDRV_VkKeyScan
@ cdecl MapVirtualKey(long long) TTYDRV_MapVirtualKey
@ cdecl GetKeyNameText(long str long) TTYDRV_GetKeyNameText
@ cdecl ToUnicode(long long ptr ptr long long) TTYDRV_ToUnicode
@ cdecl GetBeepActive() TTYDRV_GetBeepActive
@ cdecl SetBeepActive(long) TTYDRV_SetBeepActive
@ cdecl Beep() TTYDRV_Beep
@ cdecl GetDIState(long ptr) TTYDRV_GetDIState
@ cdecl GetDIData(ptr long ptr ptr long) TTYDRV_GetDIData
@ cdecl GetKeyboardConfig(ptr) TTYDRV_GetKeyboardConfig
@ cdecl SetKeyboardConfig(ptr long) TTYDRV_SetKeyboardConfig
@ cdecl InitMouse(ptr) TTYDRV_InitMouse
@ cdecl SetCursor(ptr) TTYDRV_SetCursor
@ cdecl MoveCursor(long long) TTYDRV_MoveCursor
@ cdecl GetScreenSaveActive() TTYDRV_GetScreenSaveActive
@ cdecl SetScreenSaveActive(long) TTYDRV_SetScreenSaveActive
@ cdecl GetScreenSaveTimeout() TTYDRV_GetScreenSaveTimeout
@ cdecl SetScreenSaveTimeout(long) TTYDRV_SetScreenSaveTimeout
@ cdecl LoadOEMResource(long long) TTYDRV_LoadOEMResource
@ cdecl IsSingleWindow() TTYDRV_IsSingleWindow
@ cdecl AcquireClipboard() TTYDRV_AcquireClipboard
@ cdecl ReleaseClipboard() TTYDRV_ReleaseClipboard
@ cdecl SetClipboardData(long) TTYDRV_SetClipboardData
@ cdecl GetClipboardData(long) TTYDRV_GetClipboardData
@ cdecl IsClipboardFormatAvailable(long) TTYDRV_IsClipboardFormatAvailable
@ cdecl RegisterClipboardFormat(str) TTYDRV_RegisterClipboardFormat
@ cdecl IsSelectionOwner() TTYDRV_IsSelectionOwner
@ cdecl ResetSelectionOwner(ptr long) TTYDRV_ResetSelectionOwner
