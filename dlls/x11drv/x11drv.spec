# GDI driver

@ cdecl Arc(ptr long long long long long long long long) X11DRV_Arc
@ cdecl BitBlt(ptr long long long long ptr long long long) X11DRV_BitBlt
@ cdecl ChoosePixelFormat(ptr ptr) X11DRV_ChoosePixelFormat
@ cdecl Chord(ptr long long long long long long long long) X11DRV_Chord
@ cdecl CreateBitmap(ptr long) X11DRV_CreateBitmap
@ cdecl CreateDC(ptr ptr str str str ptr) X11DRV_CreateDC
@ cdecl CreateDIBSection(ptr ptr long ptr long long long) X11DRV_DIB_CreateDIBSection
@ cdecl DeleteBitmap(long) X11DRV_DeleteBitmap
@ cdecl DeleteDC(ptr) X11DRV_DeleteDC
@ cdecl DescribePixelFormat(ptr long long ptr) X11DRV_DescribePixelFormat
@ cdecl Ellipse(ptr long long long long) X11DRV_Ellipse
@ cdecl EnumDeviceFonts(ptr ptr ptr long) X11DRV_EnumDeviceFonts
@ cdecl ExtEscape(ptr long long ptr long ptr) X11DRV_ExtEscape
@ cdecl ExtFloodFill(ptr long long long long) X11DRV_ExtFloodFill
@ cdecl ExtTextOut(ptr long long long ptr ptr long ptr) X11DRV_ExtTextOut
@ cdecl GetBitmapBits(long ptr long) X11DRV_GetBitmapBits
@ cdecl GetCharWidth(ptr long long ptr) X11DRV_GetCharWidth
@ cdecl GetDCOrgEx(ptr ptr) X11DRV_GetDCOrgEx
@ cdecl GetDIBColorTable(ptr long long ptr) X11DRV_GetDIBColorTable
@ cdecl GetDIBits(ptr long long long ptr ptr long) X11DRV_GetDIBits
@ cdecl GetDeviceCaps(ptr long) X11DRV_GetDeviceCaps
@ cdecl GetDeviceGammaRamp(ptr ptr) X11DRV_GetDeviceGammaRamp
@ cdecl GetNearestColor(ptr long) X11DRV_GetNearestColor
@ cdecl GetPixel(ptr long long) X11DRV_GetPixel
@ cdecl GetPixelFormat(ptr) X11DRV_GetPixelFormat
@ cdecl GetSystemPaletteEntries(ptr long long ptr) X11DRV_GetSystemPaletteEntries
@ cdecl GetTextExtentPoint(ptr ptr long ptr) X11DRV_GetTextExtentPoint
@ cdecl GetTextMetrics(ptr ptr) X11DRV_GetTextMetrics
@ cdecl LineTo(ptr long long) X11DRV_LineTo
@ cdecl PaintRgn(ptr long) X11DRV_PaintRgn
@ cdecl PatBlt(ptr long long long long long) X11DRV_PatBlt
@ cdecl Pie(ptr long long long long long long long long) X11DRV_Pie
@ cdecl PolyPolygon(ptr ptr ptr long) X11DRV_PolyPolygon
@ cdecl PolyPolyline(ptr ptr ptr long) X11DRV_PolyPolyline
@ cdecl Polygon(ptr ptr long) X11DRV_Polygon
@ cdecl Polyline(ptr ptr long) X11DRV_Polyline
@ cdecl RealizeDefaultPalette(ptr) X11DRV_RealizeDefaultPalette
@ cdecl RealizePalette(ptr long long) X11DRV_RealizePalette
@ cdecl Rectangle(ptr long long long long) X11DRV_Rectangle
@ cdecl RoundRect(ptr long long long long long long) X11DRV_RoundRect
@ cdecl SelectBitmap(ptr long) X11DRV_SelectBitmap
@ cdecl SelectBrush(ptr long) X11DRV_SelectBrush
@ cdecl SelectFont(ptr long) X11DRV_SelectFont
@ cdecl SelectPen(ptr long) X11DRV_SelectPen
@ cdecl SetBitmapBits(long ptr long) X11DRV_SetBitmapBits
@ cdecl SetBkColor(ptr long) X11DRV_SetBkColor
@ cdecl SetDCOrg(ptr long long) X11DRV_SetDCOrg
@ cdecl SetDIBColorTable(ptr long long ptr) X11DRV_SetDIBColorTable
@ cdecl SetDIBits(ptr long long long ptr ptr long) X11DRV_SetDIBits
@ cdecl SetDIBitsToDevice(ptr long long long long long long long long ptr ptr long) X11DRV_SetDIBitsToDevice
@ cdecl SetDeviceClipping(ptr long) X11DRV_SetDeviceClipping
@ cdecl SetDeviceGammaRamp(ptr ptr) X11DRV_SetDeviceGammaRamp
@ cdecl SetPixel(ptr long long long) X11DRV_SetPixel
@ cdecl SetPixelFormat(ptr long ptr) X11DRV_SetPixelFormat
@ cdecl SetTextColor(ptr long) X11DRV_SetTextColor
@ cdecl StretchBlt(ptr long long long long ptr long long long long long) X11DRV_StretchBlt
@ cdecl SwapBuffers(ptr) X11DRV_SwapBuffers

# USER driver

@ cdecl InitKeyboard(ptr) X11DRV_InitKeyboard
@ cdecl VkKeyScan(long) X11DRV_VkKeyScan
@ cdecl MapVirtualKey(long long) X11DRV_MapVirtualKey
@ cdecl GetKeyNameText(long str long) X11DRV_GetKeyNameText
@ cdecl ToUnicode(long long ptr ptr long long) X11DRV_ToUnicode
@ cdecl Beep() X11DRV_Beep
@ cdecl InitMouse(ptr) X11DRV_InitMouse
@ cdecl SetCursor(ptr) X11DRV_SetCursor
@ cdecl GetCursorPos(ptr) X11DRV_GetCursorPos
@ cdecl SetCursorPos(long long) X11DRV_SetCursorPos
@ cdecl GetScreenSaveActive() X11DRV_GetScreenSaveActive
@ cdecl SetScreenSaveActive(long) X11DRV_SetScreenSaveActive
@ cdecl ChangeDisplaySettingsExW (ptr ptr long long long) X11DRV_ChangeDisplaySettingsExW
@ cdecl EnumDisplaySettingsExW (ptr long ptr long) X11DRV_EnumDisplaySettingsExW
@ cdecl CreateWindow(long ptr long) X11DRV_CreateWindow
@ cdecl DestroyWindow(long) X11DRV_DestroyWindow
@ cdecl GetDC(long long long long) X11DRV_GetDC
@ cdecl ForceWindowRaise(long) X11DRV_ForceWindowRaise
@ cdecl MsgWaitForMultipleObjectsEx(long ptr long long long) X11DRV_MsgWaitForMultipleObjectsEx
@ cdecl ReleaseDC(long long) X11DRV_ReleaseDC
@ cdecl ScrollDC(long long long ptr ptr long ptr) X11DRV_ScrollDC
@ cdecl ScrollWindowEx(long long long ptr ptr long ptr long) X11DRV_ScrollWindowEx
@ cdecl SetFocus(long) X11DRV_SetFocus
@ cdecl SetParent(long long) X11DRV_SetParent
@ cdecl SetWindowPos(ptr) X11DRV_SetWindowPos
@ cdecl SetWindowRgn(long long long) X11DRV_SetWindowRgn
@ cdecl SetWindowIcon(long long long) X11DRV_SetWindowIcon
@ cdecl SetWindowStyle(ptr long) X11DRV_SetWindowStyle
@ cdecl SetWindowText(long wstr) X11DRV_SetWindowText
@ cdecl ShowWindow(long long) X11DRV_ShowWindow
@ cdecl SysCommandSizeMove(long long) X11DRV_SysCommandSizeMove
@ cdecl AcquireClipboard() X11DRV_AcquireClipboard
@ cdecl ReleaseClipboard() X11DRV_ReleaseClipboard
@ cdecl SetClipboardData(long) X11DRV_SetClipboardData
@ cdecl GetClipboardData(long) X11DRV_GetClipboardData
@ cdecl IsClipboardFormatAvailable(long) X11DRV_IsClipboardFormatAvailable
@ cdecl RegisterClipboardFormat(str) X11DRV_RegisterClipboardFormat
@ cdecl GetClipboardFormatName(long str long) X11DRV_GetClipboardFormatName
@ cdecl IsSelectionOwner() X11DRV_IsSelectionOwner
@ cdecl ResetSelectionOwner(ptr long) X11DRV_ResetSelectionOwner

# X11 locks
@ cdecl -norelay wine_tsx11_lock()
@ cdecl -norelay wine_tsx11_unlock()
