name	ttydrv
type	win32
init	TTYDRV_Init

import	user32.dll
import	gdi32.dll
import	kernel32.dll
import	ntdll.dll

debug_channels (ttydrv)

# GDI driver

@ cdecl Arc(ptr long long long long long long long long) TTYDRV_DC_Arc
@ cdecl BitBlt(ptr long long long long ptr long long long) TTYDRV_DC_BitBlt
@ cdecl BitmapBits(long ptr long long) TTYDRV_DC_BitmapBits
@ cdecl Chord(ptr long long long long long long long long) TTYDRV_DC_Chord
@ cdecl CreateBitmap(long) TTYDRV_DC_CreateBitmap
@ cdecl CreateDC(ptr str str str ptr) TTYDRV_DC_CreateDC
@ cdecl DeleteDC(ptr) TTYDRV_DC_DeleteDC
@ cdecl DeleteObject(long) TTYDRV_DC_DeleteObject
@ cdecl Ellipse(ptr long long long long) TTYDRV_DC_Ellipse
@ cdecl ExtFloodFill(ptr long long long long) TTYDRV_DC_ExtFloodFill
@ cdecl ExtTextOut(ptr long long long ptr ptr long ptr) TTYDRV_DC_ExtTextOut
@ cdecl GetCharWidth(ptr long long ptr) TTYDRV_DC_GetCharWidth
@ cdecl GetDeviceCaps(ptr long) TTYDRV_GetDeviceCaps
@ cdecl GetPixel(ptr long long) TTYDRV_DC_GetPixel
@ cdecl GetTextExtentPoint(ptr ptr long ptr) TTYDRV_DC_GetTextExtentPoint
@ cdecl GetTextMetrics(ptr ptr) TTYDRV_DC_GetTextMetrics
@ cdecl LineTo(ptr long long) TTYDRV_DC_LineTo
@ cdecl PaintRgn(ptr long) TTYDRV_DC_PaintRgn
@ cdecl PatBlt(ptr long long long long long) TTYDRV_DC_PatBlt
@ cdecl Pie(ptr long long long long long long long long) TTYDRV_DC_Pie
@ cdecl PolyPolygon(ptr ptr ptr long) TTYDRV_DC_PolyPolygon
@ cdecl PolyPolyline(ptr ptr ptr long) TTYDRV_DC_PolyPolyline
@ cdecl Polygon(ptr ptr long) TTYDRV_DC_Polygon
@ cdecl Polyline(ptr ptr long) TTYDRV_DC_Polyline
@ cdecl Rectangle(ptr long long long long) TTYDRV_DC_Rectangle
@ cdecl RoundRect(ptr long long long long long long) TTYDRV_DC_RoundRect
@ cdecl SelectObject(ptr long) TTYDRV_DC_SelectObject
@ cdecl SetBkColor(ptr long) TTYDRV_DC_SetBkColor
@ cdecl SetDeviceClipping(ptr) TTYDRV_DC_SetDeviceClipping
@ cdecl SetDIBitsToDevice(ptr long long long long long long long long ptr ptr long) TTYDRV_DC_SetDIBitsToDevice
@ cdecl SetPixel(ptr long long long) TTYDRV_DC_SetPixel
@ cdecl SetTextColor(ptr long) TTYDRV_DC_SetTextColor
@ cdecl StretchBlt(ptr long long long long ptr long long long long long) TTYDRV_DC_StretchBlt

# USER driver

@ cdecl VkKeyScan(long) TTYDRV_VkKeyScan
@ cdecl MapVirtualKey(long long) TTYDRV_MapVirtualKey
@ cdecl GetKeyNameText(long str long) TTYDRV_GetKeyNameText
@ cdecl ToUnicode(long long ptr ptr long long) TTYDRV_ToUnicode
@ cdecl Beep() TTYDRV_Beep
@ cdecl SetCursor(ptr) TTYDRV_SetCursor
@ cdecl GetScreenSaveActive() TTYDRV_GetScreenSaveActive
@ cdecl SetScreenSaveActive(long) TTYDRV_SetScreenSaveActive
@ cdecl CreateWindow(long ptr long) TTYDRV_CreateWindow
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
@ cdecl ShowWindow(long long) TTYDRV_ShowWindow
