name	ttydrv
type	win32
init	TTYDRV_Init

import	user32.dll
import	gdi32.dll
import	kernel32.dll
import	ntdll.dll

debug_channels (ttydrv)

# USER driver

@ cdecl InitKeyboard() TTYDRV_InitKeyboard
@ cdecl VkKeyScan(long) TTYDRV_VkKeyScan
@ cdecl MapVirtualKey(long long) TTYDRV_MapVirtualKey
@ cdecl GetKeyNameText(long str long) TTYDRV_GetKeyNameText
@ cdecl ToUnicode(long long ptr ptr long long) TTYDRV_ToUnicode
@ cdecl Beep() TTYDRV_Beep
@ cdecl GetDIState(long ptr) TTYDRV_GetDIState
@ cdecl GetDIData(ptr long ptr ptr long) TTYDRV_GetDIData
@ cdecl InitMouse(ptr) TTYDRV_InitMouse
@ cdecl SetCursor(ptr) TTYDRV_SetCursor
@ cdecl MoveCursor(long long) TTYDRV_MoveCursor
@ cdecl GetScreenSaveActive() TTYDRV_GetScreenSaveActive
@ cdecl SetScreenSaveActive(long) TTYDRV_SetScreenSaveActive
@ cdecl GetScreenSaveTimeout() TTYDRV_GetScreenSaveTimeout
@ cdecl SetScreenSaveTimeout(long) TTYDRV_SetScreenSaveTimeout
@ cdecl LoadOEMResource(long long) TTYDRV_LoadOEMResource
@ cdecl CreateWindow(long ptr) TTYDRV_CreateWindow
@ cdecl DestroyWindow(long) TTYDRV_DestroyWindow
@ cdecl GetDC(long long long long) TTYDRV_GetDC
@ cdecl SetWindowPos(ptr) TTYDRV_SetWindowPos
@ cdecl AcquireClipboard() TTYDRV_AcquireClipboard
@ cdecl ReleaseClipboard() TTYDRV_ReleaseClipboard
@ cdecl SetClipboardData(long) TTYDRV_SetClipboardData
@ cdecl GetClipboardData(long) TTYDRV_GetClipboardData
@ cdecl IsClipboardFormatAvailable(long) TTYDRV_IsClipboardFormatAvailable
@ cdecl RegisterClipboardFormat(str) TTYDRV_RegisterClipboardFormat
@ cdecl IsSelectionOwner() TTYDRV_IsSelectionOwner
@ cdecl ResetSelectionOwner(ptr long) TTYDRV_ResetSelectionOwner
