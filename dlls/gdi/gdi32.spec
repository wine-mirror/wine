name    gdi32
type    win32
init    MAIN_GdiInit

import	kernel32.dll

# ordinal exports
100 stdcall @(long long str str str) GDI_CallDevInstall16
101 stdcall @(long str str ptr) GDI_CallExtDeviceModePropSheet16
102 stdcall @(long ptr str str ptr str long) GDI_CallExtDeviceMode16
103 stdcall @(long str ptr ptr) GDI_CallAdvancedSetupDialog16
104 stdcall @(str str long ptr ptr) GDI_CallDeviceCapabilities16

@ stdcall AbortDoc(long) AbortDoc
@ stdcall AbortPath(long) AbortPath
@ stdcall AddFontResourceA(str) AddFontResourceA
@ stub AddFontResourceTracking
@ stdcall AddFontResourceW(wstr) AddFontResourceW
@ stdcall AngleArc(long long long long long long) AngleArc
@ stdcall AnimatePalette(long long long ptr) AnimatePalette
@ stdcall Arc(long long long long long long long long long) Arc
@ stdcall ArcTo(long long long long long long long long long) ArcTo
@ stdcall BeginPath(long) BeginPath
@ stdcall BitBlt(long long long long long long long long long) BitBlt
@ stub ByeByeGDI
@ stub CancelDC
@ stub CheckColorsInGamut
@ stdcall ChoosePixelFormat(long ptr) ChoosePixelFormat
@ stdcall Chord(long long long long long long long long long) Chord
@ stdcall CloseEnhMetaFile(long) CloseEnhMetaFile
@ stdcall CloseFigure(long) CloseFigure
@ stdcall CloseMetaFile(long) CloseMetaFile
@ stub ColorCorrectPalette
@ stub ColorMatchToTarget
@ stdcall CombineRgn(long long long long) CombineRgn
@ stdcall CombineTransform(ptr ptr ptr) CombineTransform
@ stdcall CopyEnhMetaFileA(long str) CopyEnhMetaFileA
@ stub CopyEnhMetaFileW
@ stdcall CopyMetaFileA(long str) CopyMetaFileA
@ stdcall CopyMetaFileW(long wstr) CopyMetaFileW
@ stdcall CreateBitmap(long long long long ptr) CreateBitmap
@ stdcall CreateBitmapIndirect(ptr) CreateBitmapIndirect
@ stdcall CreateBrushIndirect(ptr) CreateBrushIndirect
@ stdcall CreateColorSpaceA(ptr) CreateColorSpaceA
@ stdcall CreateColorSpaceW(ptr) CreateColorSpaceW
@ stdcall CreateCompatibleBitmap(long long long) CreateCompatibleBitmap
@ stdcall CreateCompatibleDC(long) CreateCompatibleDC
@ stdcall CreateDCA(str str str ptr) CreateDCA
@ stdcall CreateDCW(wstr wstr wstr ptr) CreateDCW
@ stdcall CreateDIBPatternBrush(long long) CreateDIBPatternBrush
@ stdcall CreateDIBPatternBrushPt(long long) CreateDIBPatternBrushPt
@ stdcall CreateDIBSection(long ptr long ptr long long) CreateDIBSection
@ stdcall CreateDIBitmap(long ptr long ptr ptr long) CreateDIBitmap
@ stdcall CreateDiscardableBitmap(long long long) CreateDiscardableBitmap
@ stdcall CreateEllipticRgn(long long long long) CreateEllipticRgn
@ stdcall CreateEllipticRgnIndirect(ptr) CreateEllipticRgnIndirect
@ stdcall CreateEnhMetaFileA(long str ptr str) CreateEnhMetaFileA
@ stdcall CreateEnhMetaFileW(long wstr ptr wstr) CreateEnhMetaFileW
@ stdcall CreateFontA(long long long long long long long long long long long long long str) CreateFontA
@ stdcall CreateFontIndirectA(ptr) CreateFontIndirectA
@ stdcall CreateFontIndirectW(ptr) CreateFontIndirectW
@ stdcall CreateFontW(long long long long long long long long long long long long long wstr) CreateFontW
@ stdcall CreateHalftonePalette(long) CreateHalftonePalette
@ stdcall CreateHatchBrush(long long) CreateHatchBrush
@ stdcall CreateICA(str str str ptr) CreateICA
@ stdcall CreateICW(wstr wstr wstr ptr) CreateICW
@ stdcall CreateMetaFileA(str) CreateMetaFileA
@ stdcall CreateMetaFileW(wstr) CreateMetaFileW
@ stdcall CreatePalette(ptr) CreatePalette
@ stdcall CreatePatternBrush(long) CreatePatternBrush
@ stdcall CreatePen(long long long) CreatePen
@ stdcall CreatePenIndirect(ptr) CreatePenIndirect
@ stdcall CreatePolyPolygonRgn(ptr ptr long long) CreatePolyPolygonRgn
@ stdcall CreatePolygonRgn(ptr long long) CreatePolygonRgn
@ stdcall CreateRectRgn(long long long long) CreateRectRgn
@ stdcall CreateRectRgnIndirect(ptr) CreateRectRgnIndirect
@ stdcall CreateRoundRectRgn(long long long long long long) CreateRoundRectRgn
@ stdcall CreateScalableFontResourceA(long str str str) CreateScalableFontResourceA
@ stdcall CreateScalableFontResourceW(long wstr wstr wstr) CreateScalableFontResourceW
@ stdcall CreateSolidBrush(long) CreateSolidBrush
@ stdcall DPtoLP(long ptr long) DPtoLP
@ stdcall DeleteColorSpace(long) DeleteColorSpace
@ stdcall DeleteDC(long) DeleteDC
@ stdcall DeleteEnhMetaFile(long) DeleteEnhMetaFile
@ stdcall DeleteMetaFile(long) DeleteMetaFile
@ stdcall DeleteObject(long)	DeleteObject
@ stdcall DescribePixelFormat(long long long ptr) DescribePixelFormat
@ stub DeviceCapabilitiesEx
@ stub DeviceCapabilitiesExA
@ stub DeviceCapabilitiesExW
@ stdcall DrawEscape(long long long ptr) DrawEscape
@ stdcall Ellipse(long long long long long) Ellipse
@ stub EnableEUDC
@ stdcall EndDoc(long) EndDoc
@ stdcall EndPage(long) EndPage
@ stdcall EndPath(long) EndPath
@ stdcall EnumEnhMetaFile(long long ptr ptr ptr) EnumEnhMetaFile
@ stdcall EnumFontFamiliesA(long str ptr long) EnumFontFamiliesA
@ stdcall EnumFontFamiliesExA(long ptr ptr long long) EnumFontFamiliesExA
@ stdcall EnumFontFamiliesExW(long ptr ptr long long) EnumFontFamiliesExW
@ stdcall EnumFontFamiliesW(long wstr ptr long) EnumFontFamiliesW
@ stdcall EnumFontsA(long str ptr long) EnumFontsA
@ stdcall EnumFontsW(long wstr ptr long) EnumFontsW
@ stub EnumICMProfilesA
@ stub EnumICMProfilesW
@ stdcall EnumMetaFile(long long ptr ptr) EnumMetaFile
@ stdcall EnumObjects(long long ptr long) EnumObjects
@ stdcall EqualRgn(long long) EqualRgn
@ stdcall Escape(long long long ptr ptr) Escape
@ stdcall ExcludeClipRect(long long long long long) ExcludeClipRect
@ stdcall ExtCreatePen(long long ptr long ptr) ExtCreatePen
@ stdcall ExtCreateRegion(ptr long ptr) ExtCreateRegion
@ stdcall ExtEscape(long long long ptr long ptr) ExtEscape
@ stdcall ExtFloodFill(long long long long long) ExtFloodFill
@ stdcall ExtSelectClipRgn(long long long) ExtSelectClipRgn
@ stdcall ExtTextOutA(long long long long ptr str long ptr) ExtTextOutA
@ stdcall ExtTextOutW(long long long long ptr wstr long ptr) ExtTextOutW
@ stdcall FillPath(long) FillPath
@ stdcall FillRgn(long long long) FillRgn
@ stdcall FixBrushOrgEx(long long long ptr) FixBrushOrgEx
@ stdcall FlattenPath(long) FlattenPath
@ stdcall FloodFill(long long long long) FloodFill
@ stdcall FrameRgn(long long long long long) FrameRgn
@ stub FreeImageColorMatcher
@ stub GdiAssociateObject
@ stub GdiCleanCacheDC
@ stdcall GdiComment(long long ptr) GdiComment
@ stub GdiConvertAndCheckDC
@ stub GdiConvertBitmap
@ stub GdiConvertBrush
@ stub GdiConvertDC
@ stub GdiConvertEnhMetaFile
@ stub GdiConvertFont
@ stub GdiConvertMetaFilePict
@ stub GdiConvertPalette
@ stub GdiConvertRegion
@ stub GdiCreateLocalBitmap
@ stub GdiCreateLocalBrush
@ stub GdiCreateLocalEnhMetaFile
@ stub GdiCreateLocalFont
@ stub GdiCreateLocalMetaFilePict
@ stub GdiCreateLocalPalette
@ stub GdiCreateLocalRegion
@ stub GdiDciBeginAccess
@ stub GdiDciCreateOffscreenSurface
@ stub GdiDciCreateOverlaySurface
@ stub GdiDciCreatePrimarySurface
@ stub GdiDciDestroySurface
@ stub GdiDciDrawSurface
@ stub GdiDciEndAccess
@ stub GdiDciEnumSurface
@ stub GdiDciInitialize
@ stub GdiDciSetClipList
@ stub GdiDciSetDestination
@ stub GdiDeleteLocalDC
@ stub GdiDeleteLocalObject
@ stub GdiDllInitialize
@ stdcall GdiFlush() GdiFlush
@ stdcall GdiGetBatchLimit() GdiGetBatchLimit
@ stub GdiGetLocalBitmap
@ stub GdiGetLocalBrush
@ stub GdiGetLocalDC
@ stub GdiGetLocalFont
@ stub GdiIsMetaFileDC
@ stub GdiPlayDCScript
@ stub GdiPlayJournal
@ stub GdiPlayScript
@ stub GdiReleaseLocalDC
@ stub GdiSetAttrs
@ stdcall GdiSetBatchLimit(long) GdiSetBatchLimit
@ stub GdiSetServerAttr
@ stub GdiWinWatchClose
@ stub GdiWinWatchDidStatusChange
@ stub GdiWinWatchGetClipList
@ stub GdiWinWatchOpen
@ stdcall GetArcDirection(long) GetArcDirection
@ stdcall GetAspectRatioFilterEx(long ptr) GetAspectRatioFilterEx
@ stdcall GetBitmapBits(long long ptr) GetBitmapBits
@ stdcall GetBitmapDimensionEx(long ptr) GetBitmapDimensionEx
@ stdcall GetBkColor(long) GetBkColor
@ stdcall GetBkMode(long) GetBkMode
@ stdcall GetBoundsRect(long ptr long) GetBoundsRect
@ stdcall GetBrushOrgEx(long ptr) GetBrushOrgEx
@ stdcall GetCharABCWidthsA(long long long ptr) GetCharABCWidthsA
@ stdcall GetCharABCWidthsFloatA(long long long ptr) GetCharABCWidthsFloatA
@ stdcall GetCharABCWidthsFloatW(long long long ptr) GetCharABCWidthsFloatW
@ stdcall GetCharABCWidthsW(long long long ptr) GetCharABCWidthsW
@ stdcall GetCharWidth32A(long long long long) GetCharWidth32A
@ stdcall GetCharWidth32W(long long long long) GetCharWidth32W
@ stdcall GetCharWidthA(long long long long) GetCharWidth32A
@ stdcall GetCharWidthFloatA(long long long ptr) GetCharWidthFloatA
@ stdcall GetCharWidthFloatW(long long long ptr) GetCharWidthFloatW
@ stdcall GetCharWidthW(long long long long) GetCharWidth32W
@ stub GetCharWidthWOW
@ stdcall GetCharacterPlacementA(long str long long ptr long) GetCharacterPlacementA
@ stdcall GetCharacterPlacementW(long wstr long long ptr long) GetCharacterPlacementW
@ stdcall GetClipBox(long ptr) GetClipBox
@ stdcall GetClipRgn(long long) GetClipRgn
@ stdcall GetColorAdjustment(long ptr) GetColorAdjustment
@ stdcall GetColorSpace(long) GetColorSpace
@ stdcall GetCurrentObject(long long) GetCurrentObject
@ stdcall GetCurrentPositionEx(long ptr) GetCurrentPositionEx
@ stdcall GetDCOrgEx(long ptr) GetDCOrgEx
@ stdcall GetDIBColorTable(long long long ptr) GetDIBColorTable
@ stdcall GetDIBits(long long long long ptr ptr long) GetDIBits
@ stdcall GetDeviceCaps(long long) GetDeviceCaps
@ stub GetDeviceGammaRamp
@ stub GetETM
@ stdcall GetEnhMetaFileA(str) GetEnhMetaFileA
@ stdcall GetEnhMetaFileBits(long long ptr) GetEnhMetaFileBits
@ stdcall GetEnhMetaFileDescriptionA(long long ptr) GetEnhMetaFileDescriptionA
@ stdcall GetEnhMetaFileDescriptionW(long long ptr) GetEnhMetaFileDescriptionW
@ stdcall GetEnhMetaFileHeader(long long ptr) GetEnhMetaFileHeader
@ stdcall GetEnhMetaFilePaletteEntries (long long ptr) GetEnhMetaFilePaletteEntries
@ stdcall GetEnhMetaFileW(wstr) GetEnhMetaFileW
@ stdcall GetFontData(long long long ptr long) GetFontData
@ stdcall GetFontLanguageInfo(long) GetFontLanguageInfo
@ stub GetFontResourceInfo
@ stub GetFontResourceInfoW
@ stub GetGlyphOutline
@ stdcall GetGlyphOutlineA(long long long ptr long ptr ptr) GetGlyphOutlineA
@ stdcall GetGlyphOutlineW(long long long ptr long ptr ptr) GetGlyphOutlineW
@ stub GetGlyphOutlineWow
@ stdcall GetGraphicsMode(long) GetGraphicsMode
@ stdcall GetICMProfileA(long ptr ptr) GetICMProfileA
@ stub GetICMProfileW
@ stub GetKerningPairs
@ stdcall GetKerningPairsA(long long ptr) GetKerningPairsA
@ stdcall GetKerningPairsW(long long ptr) GetKerningPairsW
@ stdcall GetLayout(long) GetLayout
@ stub GetLogColorSpaceA
@ stub GetLogColorSpaceW
@ stdcall GetMapMode(long) GetMapMode
@ stdcall GetMetaFileA(str) GetMetaFileA
@ stdcall GetMetaFileBitsEx(long long ptr) GetMetaFileBitsEx
@ stdcall GetMetaFileW(wstr) GetMetaFileW
@ stdcall GetMetaRgn(long long) GetMetaRgn
@ stdcall GetMiterLimit(long ptr) GetMiterLimit
@ stdcall GetNearestColor(long long) GetNearestColor
@ stdcall GetNearestPaletteIndex(long long) GetNearestPaletteIndex
@ stdcall GetObjectA(long long ptr) GetObjectA
@ stdcall GetObjectType(long) GetObjectType
@ stdcall GetObjectW(long long ptr) GetObjectW
@ stdcall GetOutlineTextMetricsA(long long ptr) GetOutlineTextMetricsA
@ stdcall GetOutlineTextMetricsW(long long ptr) GetOutlineTextMetricsW
@ stdcall GetPaletteEntries(long long long ptr) GetPaletteEntries
@ stdcall GetPath(long ptr ptr long) GetPath
@ stdcall GetPixel(long long long) GetPixel
@ stdcall GetPixelFormat(long) GetPixelFormat
@ stdcall GetPolyFillMode(long) GetPolyFillMode
@ stdcall GetROP2(long) GetROP2
@ stdcall GetRandomRgn(long long long) GetRandomRgn
@ stdcall GetRasterizerCaps(ptr long) GetRasterizerCaps
@ stdcall GetRegionData(long long ptr) GetRegionData
@ stdcall GetRelAbs(long long) GetRelAbs
@ stdcall GetRgnBox(long ptr) GetRgnBox
@ stdcall GetStockObject(long) GetStockObject
@ stdcall GetStretchBltMode(long) GetStretchBltMode
@ stdcall GetSystemPaletteEntries(long long long ptr) GetSystemPaletteEntries
@ stdcall GetSystemPaletteUse(long) GetSystemPaletteUse
@ stdcall GetTextAlign(long) GetTextAlign
@ stdcall GetTextCharacterExtra(long) GetTextCharacterExtra
@ stdcall GetTextCharset(long) GetTextCharset
@ stdcall GetTextCharsetInfo(long ptr long) GetTextCharsetInfo
@ stdcall GetTextColor(long) GetTextColor
@ stdcall GetTextExtentExPointA(long str long long ptr ptr ptr) GetTextExtentExPointA
@ stdcall GetTextExtentExPointW(long wstr long long ptr ptr ptr) GetTextExtentExPointW
@ stdcall GetTextExtentPoint32A(long ptr long ptr) GetTextExtentPoint32A
@ stdcall GetTextExtentPoint32W(long ptr long ptr) GetTextExtentPoint32W
@ stdcall GetTextExtentPointA(long ptr long ptr) GetTextExtentPointA
@ stdcall GetTextExtentPointW(long ptr long ptr) GetTextExtentPointW
@ stdcall GetTextFaceA(long long ptr) GetTextFaceA
@ stdcall GetTextFaceW(long long ptr) GetTextFaceW
@ stdcall GetTextMetricsA(long ptr) GetTextMetricsA
@ stdcall GetTextMetricsW(long ptr) GetTextMetricsW
@ stub GetTransform
@ stdcall GetViewportExtEx(long ptr) GetViewportExtEx
@ stdcall GetViewportOrgEx(long ptr) GetViewportOrgEx
@ stdcall GetWinMetaFileBits(long long ptr long long) GetWinMetaFileBits
@ stdcall GetWindowExtEx(long ptr) GetWindowExtEx
@ stdcall GetWindowOrgEx(long ptr) GetWindowOrgEx
@ stdcall GetWorldTransform(long ptr) GetWorldTransform
@ stdcall IntersectClipRect(long long long long long) IntersectClipRect
@ stdcall InvertRgn(long long) InvertRgn
@ stdcall LPtoDP(long ptr long) LPtoDP
@ stdcall LineDDA(long long long long ptr long) LineDDA
@ stdcall LineTo(long long long) LineTo
@ stub LoadImageColorMatcherA
@ stub LoadImageColorMatcherW
@ stdcall MaskBlt(long long long long long long long long long long long long) MaskBlt
@ stdcall ModifyWorldTransform(long ptr long) ModifyWorldTransform
@ stdcall MoveToEx(long long long ptr) MoveToEx
@ stdcall OffsetClipRgn(long long long) OffsetClipRgn
@ stdcall OffsetRgn(long long long) OffsetRgn
@ stdcall OffsetViewportOrgEx(long long long ptr) OffsetViewportOrgEx
@ stdcall OffsetWindowOrgEx(long long long ptr) OffsetWindowOrgEx
@ stdcall PaintRgn(long long) PaintRgn
@ stdcall PatBlt(long long long long long long) PatBlt
@ stdcall PathToRegion(long) PathToRegion
@ stdcall Pie(long long long long long long long long long) Pie
@ stdcall PlayEnhMetaFile(long long ptr) PlayEnhMetaFile
@ stdcall PlayEnhMetaFileRecord(long ptr ptr long) PlayEnhMetaFileRecord
@ stdcall PlayMetaFile(long long) PlayMetaFile
@ stdcall PlayMetaFileRecord(long ptr ptr long) PlayMetaFileRecord
@ stdcall PlgBlt(long ptr long long long long long long long long) PlgBlt
@ stdcall PolyBezier(long ptr long) PolyBezier
@ stdcall PolyBezierTo(long ptr long) PolyBezierTo
@ stdcall PolyDraw(long ptr ptr long) PolyDraw
@ stdcall PolyPolygon(long ptr ptr long) PolyPolygon
@ stdcall PolyPolyline(long ptr ptr long) PolyPolyline
@ stdcall PolyTextOutA(long ptr long) PolyTextOutA
@ stdcall PolyTextOutW(long ptr long) PolyTextOutW
@ stdcall Polygon(long ptr long) Polygon
@ stdcall Polyline(long ptr long) Polyline
@ stdcall PolylineTo(long ptr long) PolylineTo
@ stdcall PtInRegion(long long long) PtInRegion
@ stdcall PtVisible(long long long) PtVisible
@ stdcall RealizePalette(long) RealizePalette
@ stdcall RectInRegion(long ptr) RectInRegion
@ stdcall RectVisible(long ptr) RectVisible
@ stdcall Rectangle(long long long long long) Rectangle
@ stdcall RemoveFontResourceA(str) RemoveFontResourceA
@ stub RemoveFontResourceTracking
@ stdcall RemoveFontResourceW(wstr) RemoveFontResourceW
@ stdcall ResetDCA(long ptr) ResetDCA
@ stdcall ResetDCW(long ptr) ResetDCW
@ stdcall ResizePalette(long long) ResizePalette
@ stdcall RestoreDC(long long) RestoreDC
@ stdcall RoundRect(long long long long long long long) RoundRect
@ stdcall SaveDC(long) SaveDC
@ stdcall ScaleViewportExtEx(long long long long long ptr) ScaleViewportExtEx
@ stdcall ScaleWindowExtEx(long long long long long ptr) ScaleWindowExtEx
@ stub SelectBrushLocal
@ stdcall SelectClipPath(long long) SelectClipPath
@ stdcall SelectClipRgn(long long) SelectClipRgn
@ stub SelectFontLocal
@ stdcall SelectObject(long long) SelectObject
@ stdcall SelectPalette(long long long) SelectPalette
@ stdcall SetAbortProc(long ptr) SetAbortProc
@ stdcall SetArcDirection(long long) SetArcDirection
@ stdcall SetBitmapBits(long long ptr) SetBitmapBits
@ stdcall SetBitmapDimensionEx(long long long ptr) SetBitmapDimensionEx
@ stdcall SetBkColor(long long) SetBkColor
@ stdcall SetBkMode(long long) SetBkMode
@ stdcall SetBoundsRect(long ptr long) SetBoundsRect
@ stdcall SetBrushOrgEx(long long long ptr) SetBrushOrgEx
@ stdcall SetColorAdjustment(long ptr) SetColorAdjustment
@ stdcall SetColorSpace(long long) SetColorSpace
@ stdcall SetDIBColorTable(long long long ptr) SetDIBColorTable
@ stdcall SetDIBits(long long long long ptr ptr long) SetDIBits
@ stdcall SetDIBitsToDevice(long long long long long long long long long ptr ptr long) SetDIBitsToDevice
@ stub SetDeviceGammaRamp
@ stdcall SetEnhMetaFileBits(long ptr) SetEnhMetaFileBits
@ stub SetFontEnumeration
@ stdcall SetGraphicsMode(long long) SetGraphicsMode
@ stdcall SetICMMode(long long) SetICMMode
@ stub SetICMProfileA
@ stub SetICMProfileW
@ stdcall SetLayout(long long) SetLayout
@ stub SetMagicColors
@ stdcall SetMapMode(long long) SetMapMode
@ stdcall SetMapperFlags(long long) SetMapperFlags
@ stdcall SetMetaFileBitsEx(long ptr) SetMetaFileBitsEx
@ stdcall SetMetaRgn(long) SetMetaRgn
@ stdcall SetMiterLimit(long long ptr) SetMiterLimit
@ stdcall SetObjectOwner(long long) SetObjectOwner
@ stdcall SetPaletteEntries(long long long ptr) SetPaletteEntries
@ stdcall SetPixel(long long long long) SetPixel
@ stdcall SetPixelFormat(long long ptr) SetPixelFormat
@ stdcall SetPixelV(long long long long) SetPixelV
@ stdcall SetPolyFillMode(long long) SetPolyFillMode
@ stdcall SetROP2(long long) SetROP2
@ stdcall SetRectRgn(long long long long long) SetRectRgn
@ stdcall SetRelAbs(long long) SetRelAbs
@ stdcall SetStretchBltMode(long long) SetStretchBltMode
@ stdcall SetSystemPaletteUse(long long) SetSystemPaletteUse
@ stdcall SetTextAlign(long long) SetTextAlign
@ stdcall SetTextCharacterExtra(long long) SetTextCharacterExtra
@ stdcall SetTextColor(long long) SetTextColor
@ stdcall SetTextJustification(long long long) SetTextJustification
@ stdcall SetViewportExtEx(long long long ptr) SetViewportExtEx
@ stdcall SetViewportOrgEx(long long long ptr) SetViewportOrgEx
@ stub SetVirtualResolution
@ stdcall SetWinMetaFileBits(long ptr long ptr) SetWinMetaFileBits
@ stdcall SetWindowExtEx(long long long ptr) SetWindowExtEx
@ stdcall SetWindowOrgEx(long long long ptr) SetWindowOrgEx
@ stdcall SetWorldTransform(long ptr) SetWorldTransform
@ stdcall StartDocA(long ptr) StartDocA
@ stdcall StartDocW(long ptr) StartDocW
@ stdcall StartPage(long) StartPage 
@ stdcall StretchBlt(long long long long long long long long long long long) StretchBlt
@ stdcall StretchDIBits(long long long long long long long long long ptr ptr long long) StretchDIBits
@ stdcall StrokeAndFillPath(long) StrokeAndFillPath
@ stdcall StrokePath(long) StrokePath
@ stdcall SwapBuffers(long) SwapBuffers
@ stdcall TextOutA(long long long str long) TextOutA
@ stdcall TextOutW(long long long wstr long) TextOutW
@ stdcall TranslateCharsetInfo(ptr ptr long) TranslateCharsetInfo
@ stub UnloadNetworkFonts
@ stdcall UnrealizeObject(long) UnrealizeObject
@ stdcall UpdateColors(long) UpdateColors
@ stub UpdateICMRegKey
@ stub UpdateICMRegKeyA
@ stub UpdateICMRegKeyW
@ stdcall WidenPath(long) WidenPath
@ stub gdiPlaySpoolStream
@ extern pfnRealizePalette pfnRealizePalette
@ extern pfnSelectPalette pfnSelectPalette
@ stub pstackConnect
