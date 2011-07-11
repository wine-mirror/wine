# GDI driver

@ cdecl Arc(ptr long long long long long long long long) PSDRV_Arc
@ cdecl Chord(ptr long long long long long long long long) PSDRV_Chord
@ cdecl CreateDC(long ptr wstr wstr wstr ptr) PSDRV_CreateDC
@ cdecl DeleteDC(ptr) PSDRV_DeleteDC
@ cdecl DeviceCapabilities(ptr ptr ptr long ptr ptr) PSDRV_DeviceCapabilities
@ cdecl Ellipse(ptr long long long long) PSDRV_Ellipse
@ cdecl EndDoc(ptr) PSDRV_EndDoc
@ cdecl EndPage(ptr) PSDRV_EndPage
@ cdecl EnumDeviceFonts(ptr ptr ptr long) PSDRV_EnumDeviceFonts
@ cdecl ExtDeviceMode(ptr long ptr ptr ptr ptr ptr long) PSDRV_ExtDeviceMode
@ cdecl ExtEscape(ptr long long ptr long ptr) PSDRV_ExtEscape
@ cdecl ExtTextOut(ptr long long long ptr ptr long ptr) PSDRV_ExtTextOut
@ cdecl GetCharWidth(ptr long long ptr) PSDRV_GetCharWidth
@ cdecl GetDeviceCaps(ptr long) PSDRV_GetDeviceCaps
@ cdecl GetTextExtentExPoint(ptr ptr long long ptr ptr ptr) PSDRV_GetTextExtentExPoint
@ cdecl GetTextMetrics(ptr ptr) PSDRV_GetTextMetrics
@ cdecl LineTo(ptr long long) PSDRV_LineTo
@ cdecl PaintRgn(ptr long) PSDRV_PaintRgn
@ cdecl PatBlt(ptr ptr long) PSDRV_PatBlt
@ cdecl Pie(ptr long long long long long long long long) PSDRV_Pie
@ cdecl PolyPolygon(ptr ptr ptr long) PSDRV_PolyPolygon
@ cdecl PolyPolyline(ptr ptr ptr long) PSDRV_PolyPolyline
@ cdecl Polygon(ptr ptr long) PSDRV_Polygon
@ cdecl Polyline(ptr ptr long) PSDRV_Polyline
@ cdecl Rectangle(ptr long long long long) PSDRV_Rectangle
@ cdecl ResetDC(ptr ptr) PSDRV_ResetDC
@ cdecl RoundRect(ptr long long long long long long) PSDRV_RoundRect
@ cdecl SelectBrush(ptr long) PSDRV_SelectBrush
@ cdecl SelectFont(ptr long long) PSDRV_SelectFont
@ cdecl SelectPen(ptr long) PSDRV_SelectPen
@ cdecl SetBkColor(ptr long) PSDRV_SetBkColor
@ cdecl SetDCBrushColor(ptr long) PSDRV_SetDCBrushColor
@ cdecl SetDCPenColor(ptr long) PSDRV_SetDCPenColor
@ cdecl SetPixel(ptr long long long) PSDRV_SetPixel
@ cdecl SetTextColor(ptr long) PSDRV_SetTextColor
@ cdecl StartDoc(ptr ptr) PSDRV_StartDoc
@ cdecl StartPage(ptr) PSDRV_StartPage
@ cdecl StretchDIBits(ptr long long long long long long long long ptr ptr long long) PSDRV_StretchDIBits
