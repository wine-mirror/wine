1   stub BitBlt
2   stub ColorInfo
3   stub Control
4   stub Disable
5   stub Enable
6   stub EnumFonts
7   stub EnumObj
8   stub Output
9   stub Pixel
10  stub RealizeObject
11  stub StrBlt
12  stub ScanLR
13  stub DeviceMode
14  stub ExtTextOut
15  stub GetCharWidth
16  stub DeviceBitmap
17  stub FastBorder
18  stub SetAttribute
19  stub DeviceBitmapBits
20  stub CreateBitmap
21  stub DIBScreenBlt

# ATI driver exports
22 stub SetPalette
23 stub GetPalette
24 stub SetPaletteTranslate
25 stub GetPaletteTranslate
26 stub UpdateColors
27 stub StretchBlt
28 stub StretchDIBits
29 stub SelectBitmap
30 stub BitmapBits
31 stub ReEnable
# these conflict with ordinals 19-21, thus relocated to 39-41 !
39 stub DIBBlt
40 stub CreateDIBitmap
41 stub DIBToDevice
# ATI driver end

90  stub Do_Polylines
91  stub Do_Scanlines
92  stub SaveScreenBitmap
101 pascal -ret16 Inquire(ptr) Inquire16
102 pascal -ret16 SetCursor(ptr) DISPLAY_SetCursor
103 pascal -ret16 MoveCursor(word word) MoveCursor16
104 pascal -ret16 CheckCursor() CheckCursor16
400 stub PExtTextOut
401 stub PStrBlt
402 stub RExtTextOut
403 stub RStrBlt
450 pascal GetDriverResourceID(word str) GetDriverResourceID16
500 pascal -ret16 UserRepaintDisable(word) UserRepaintDisable16
501 stub ORDINAL_ONLY1
502 stub ORDINAL_ONLY2
600 stub InkReady
601 stub GetLPDevice
