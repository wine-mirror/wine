name	gdi32
type	win32
base	1

0000 stub AbortDoc
0001 stub AbortPath
0002 stdcall AddFontResourceA(ptr) AddFontResource
0003 stub AddFontResourceTracking
0004 stub AddFontResourceW
0005 stub AngleArc
0006 stub AnimatePalette
0007 stub Arc
0008 stub ArcTo
0009 stub BeginPath
0010 stdcall BitBlt(long long long long long long long long long) BitBlt
0011 stub CancelDC
0012 stub CheckColorsInGamut
0013 stub ChoosePixelFormat
0014 stub Chord
0015 stub CloseEnhMetaFile
0016 stub CloseFigure
0017 stub CloseMetaFile
0018 stub ColorMatchToTarget
0019 stdcall CombineRgn(long long long long) CombineRgn
0020 stub CombineTransform
0021 stub CopyEnhMetaFileA
0022 stub CopyEnhMetaFileW
0023 stub CopyMetaFileA
0024 stub CopyMetaFileW
0025 stdcall CreateBitmap(long long long long ptr) CreateBitmap
0026 stdcall CreateBitmapIndirect(ptr) CreateBitmapIndirect32
0027 stub CreateBrushIndirect
0028 stub CreateColorSpaceA
0029 stub CreateColorSpaceW
0030 stdcall CreateCompatibleBitmap(long long long) CreateCompatibleBitmap
0031 stdcall CreateCompatibleDC(long) CreateCompatibleDC
0032 stub CreateDCA
0033 stub CreateDCW
0034 stdcall CreateDIBPatternBrush(long long) CreateDIBPatternBrush
0035 stub CreateDIBPatternBrushPt
0036 stub CreateDIBSection
0037 stdcall CreateDIBitmap(long ptr long ptr ptr long) CreateDIBitmap
0038 stdcall CreateDiscardableBitmap(long long long) CreateDiscardableBitmap
0039 stdcall CreateEllipticRgn(long long long long) CreateEllipticRgn
0040 stdcall CreateEllipticRgnIndirect(ptr) CreateEllipticRgnIndirect32
0041 stub CreateEnhMetaFileA
0042 stub CreateEnhMetaFileW
0043 stdcall CreateFontA(long long long long long long long long
                        long long long long long ptr) CreateFont32A
0044 stdcall CreateFontIndirectA(ptr) CreateFontIndirect32A
0045 stdcall CreateFontIndirectW(ptr) CreateFontIndirect32W
0046 stdcall CreateFontW(long long long long long long long long
                        long long long long long ptr) CreateFont32W
0047 stub CreateHalftonePalette
0048 stub CreateHatchBrush
0049 stdcall CreateICA(ptr ptr ptr ptr) CreateIC
0050 stub CreateICW
0051 stub CreateMetaFileA
0052 stub CreateMetaFileW
0053 stdcall CreatePalette(ptr) CreatePalette
0054 stdcall CreatePatternBrush(long) CreatePatternBrush
0055 stdcall CreatePen(long long long) CreatePen
0056 stub CreatePenIndirect
0057 stub CreatePolyPolygonRgn
0058 stub CreatePolygonRgn
0059 stdcall CreateRectRgn(long long long long) CreateRectRgn
0060 stdcall CreateRectRgnIndirect(ptr) CreateRectRgnIndirect32
0061 stdcall CreateRoundRectRgn(long long long long long long) CreateRoundRectRgn
0062 stub CreateScalableFontResourceA
0063 stub CreateScalableFontResourceW
0064 stdcall CreateSolidBrush(long) CreateSolidBrush
0065 stdcall DPtoLP(long ptr long) DPtoLP32
0066 stub DeleteColorSpace
0067 stdcall DeleteDC(long) DeleteDC
0068 stub DeleteEnhMetaFile
0069 stub DeleteMetaFile
0070 stdcall DeleteObject(long)	DeleteObject
0071 stub DescribePixelFormat
0072 stub DeviceCapabilitiesExA
0073 stub DeviceCapabilitiesExW
0074 stub DrawEscape
0075 stub Ellipse
0076 stub EndDoc
0077 stub EndPage
0078 stub EndPath
0079 stub EnumEnhMetaFile
0080 stub EnumFontFamiliesA
0081 stub EnumFontFamiliesExA
0082 stub EnumFontFamiliesExW
0083 stub EnumFontFamiliesW
0084 stub EnumFontsA
0085 stub EnumFontsW
0086 stub EnumICMProfilesA
0087 stub EnumICMProfilesW
0088 stub EnumMetaFile
0089 stub EnumObjects
0090 stdcall EqualRgn(long long) EqualRgn
0091 stub Escape
0092 stub ExcludeClipRect
0093 stub ExtCreatePen
0094 stub ExtCreateRegion
0095 stub ExtEscape
0096 stdcall ExtFloodFill(long long long long long) ExtFloodFill
0097 stub ExtSelectClipRgn
0098 stdcall ExtTextOutA(long long long long ptr ptr long ptr) ExtTextOut32A
0099 stdcall ExtTextOutW(long long long long ptr ptr long ptr) ExtTextOut32W
0100 stub FillPath
0101 stub FillRgn
0102 stub FixBrushOrgEx
0103 stub FlattenPath
0104 stdcall FloodFill(long long long long) FloodFill
0105 stub FrameRgn
0106 stub FreeImageColorMatcher
0107 stub GdiAssociateObject
0108 stub GdiCleanCacheDC
0109 stub GdiComment
0110 stub GdiConvertAndCheckDC
0111 stub GdiConvertBitmap
0112 stub GdiConvertBrush
0113 stub GdiConvertDC
0114 stub GdiConvertEnhMetaFile
0115 stub GdiConvertFont
0116 stub GdiConvertMetaFilePict
0117 stub GdiConvertPalette
0118 stub GdiConvertRegion
0119 stub GdiCreateLocalBitmap
0120 stub GdiCreateLocalBrush
0121 stub GdiCreateLocalEnhMetaFile
0122 stub GdiCreateLocalFont
0123 stub GdiCreateLocalMetaFilePict
0124 stub GdiCreateLocalPalette
0125 stub GdiCreateLocalRegion
0126 stub GdiDeleteLocalDC
0127 stub GdiDeleteLocalObject
0128 stub GdiFlush
0129 stub GdiGetBatchLimit
0130 stub GdiGetLocalBrush
0131 stub GdiGetLocalDC
0132 stub GdiGetLocalFont
0133 stub GdiIsMetaFileDC
0134 stub GdiPlayDCScript
0135 stub GdiPlayJournal
0136 stub GdiPlayScript
0137 stub GdiReleaseLocalDC
0138 stub GdiSetAttrs
0139 stub GdiSetBatchLimit
0140 stub GdiSetServerAttr
0141 stub GetArcDirection
0142 stub GetAspectRatioFilterEx
0143 stdcall GetBitmapBits(long long ptr) GetBitmapBits
0144 stdcall GetBitmapDimensionEx(long ptr) GetBitmapDimensionEx32
0145 stdcall GetBkColor(long) GetBkColor
0146 stub GetBkMode
0147 stub GetBoundsRect
0148 stub GetBrushOrgEx
0149 stub GetCharABCWidthsA
0150 stub GetCharABCWidthsFloatA
0151 stub GetCharABCWidthsFloatW
0152 stub GetCharABCWidthsW
0153 stub GetCharWidth32A
0154 stub GetCharWidth32W
0155 stub GetCharWidthA
0156 stub GetCharWidthFloatA
0157 stub GetCharWidthFloatW
0158 stub GetCharWidthW
0159 stub GetCharWidthWOW
0160 stub GetCharacterPlacementA
0161 stub GetCharacterPlacementW
0162 stdcall GetClipBox(long ptr) GetClipBox32
0163 stub GetClipRgn
0164 stub GetColorAdjustment
0165 stub GetColorSpace
0166 stub GetCurrentObject
0167 stub GetCurrentPositionEx
0168 stub GetDCOrgEx
0169 stub GetDIBColorTable
0170 stub GetDIBits
0171 stdcall GetDeviceCaps(long long) GetDeviceCaps
0172 stub GetDeviceGammaRamp
0173 stub GetETM
0174 stub GetEnhMetaFileA
0175 stub GetEnhMetaFileBits
0176 stub GetEnhMetaFileDescriptionA
0177 stub GetEnhMetaFileDescriptionW
0178 stub GetEnhMetaFileHeader
0179 stub GetEnhMetaFilePaletteEntries
0180 stub GetEnhMetaFileW
0181 stub GetFontData
0182 stub GetFontLanguageInfo
0183 stub GetFontResourceInfo
0184 stub GetFontResourceInfoW
0185 stub GetGlyphOutline
0186 stub GetGlyphOutlineA
0187 stub GetGlyphOutlineW
0188 stub GetGraphicsMode
0189 stub GetICMProfileA
0190 stub GetICMProfileW
0191 stub GetKerningPairs
0192 stub GetKerningPairsA
0193 stub GetKerningPairsW
0194 stub GetLogColorSpaceA
0195 stub GetLogColorSpaceW
0196 stub GetMapMode
0197 stub GetMetaFileA
0198 stub GetMetaFileBitsEx
0199 stub GetMetaFileW
0200 stub GetMetaRgn
0201 stub GetMiterLimit
0202 stdcall GetNearestColor(long long) GetNearestColor
0203 stub GetNearestPaletteIndex
0204 stdcall GetObjectA(long long ptr) GetObject32A
0205 stub GetObjectType
0206 stdcall GetObjectW(long long ptr) GetObject32W
0207 stub GetOutlineTextMetricsA
0208 stub GetOutlineTextMetricsW
0209 stub GetPaletteEntries
0210 stub GetPath
0211 stdcall GetPixel(long long long)	GetPixel
0212 stub GetPixelFormat
0213 stub GetPolyFillMode
0214 stub GetROP2
0215 stub GetRandomRgn
0216 stub GetRasterizerCaps
0217 stub GetRegionData
0218 stub GetRelAbs
0219 stdcall GetRgnBox(long ptr) GetRgnBox32
0220 stdcall GetStockObject(long) GetStockObject
0221 stub GetStretchBltMode
0222 stdcall GetSystemPaletteEntries(long long long ptr) GetSystemPaletteEntries
0223 stub GetSystemPaletteUse
0224 stub GetTextAlign
0225 stub GetTextCharacterExtra
0226 stub GetTextCharset
0227 stdcall GetTextColor(long) GetTextColor
0228 stub GetTextExtentExPointA
0229 stub GetTextExtentExPointW
0230 stdcall GetTextExtentPoint32A(long ptr long ptr) GetTextExtentPoint32A
0231 stub GetTextExtentPoint32W
0232 stdcall GetTextExtentPointA(long ptr long ptr) GetTextExtentPoint32A
0233 stdcall GetTextExtentPointW(long ptr long ptr) GetTextExtentPoint32W
0234 stub GetTextFaceA
0235 stub GetTextFaceW
0236 stdcall GetTextMetricsA(long ptr) GetTextMetrics32A
0237 stdcall GetTextMetricsW(long ptr) GetTextMetrics32W
0238 stub GetTransform
0239 stub GetViewportExtEx
0240 stub GetViewportOrgEx
0241 stub GetWinMetaFileBits
0242 stub GetWindowExtEx
0243 stub GetWindowOrgEx
0244 stub GetWorldTransform
0245 stub IntersectClipRect
0246 stdcall InvertRgn(long long) InvertRgn
0247 stdcall LPtoDP(long ptr long) LPtoDP32
0248 stdcall LineDDA(long long long long ptr long) THUNK_LineDDA32
0249 stdcall LineTo(long long long) LineTo
0250 stub LoadImageColorMatcherA
0251 stub LoadImageColorMatcherW
0252 stub MaskBlt
0253 stub ModifyWorldTransform
0254 stdcall MoveToEx(long long long ptr) MoveToEx32
0255 stub OffsetClipRgn
0256 stdcall OffsetRgn(long long long) OffsetRgn
0257 stdcall OffsetViewportOrgEx(long long long ptr) OffsetViewportOrgEx32
0258 stdcall OffsetWindowOrgEx(long long long ptr) OffsetWindowOrgEx32
0259 stub PaintRgn
0260 stdcall PatBlt(long long long long long long) PatBlt
0261 stub PathToRegion
0262 stub Pie
0263 stub PlayEnhMetaFile
0264 stub PlayEnhMetaFileRecord
0265 stub PlayMetaFile
0266 stub PlayMetaFileRecord
0267 stub PlgBlt
0268 stub PolyBezier
0269 stub PolyBezierTo
0270 stub PolyDraw
0271 stub PolyPolygon
0272 stub PolyPolyline
0273 stub PolyTextOutA
0274 stub PolyTextOutW
0275 stub Polygon
0276 stub Polyline
0277 stub PolylineTo
0278 stdcall PtInRegion(long long long) PtInRegion
0279 stdcall PtVisible(long long long) PtVisible
0280 stdcall RealizePalette(long) RealizePalette
0281 stdcall RectInRegion(long ptr) RectInRegion32
0282 stdcall RectVisible(long ptr) RectVisible32
0283 stdcall Rectangle(long long long long long) Rectangle
0284 stub RemoveFontResourceA
0285 stub RemoveFontResourceTracking
0286 stub RemoveFontResourceW
0287 stub ResetDCA
0288 stub ResetDCW
0289 stub ResizePalette
0290 stdcall RestoreDC(long long) RestoreDC
0291 stub RoundRect
0292 stdcall SaveDC(long) SaveDC
0293 stdcall ScaleViewportExtEx(long long long long long ptr) ScaleViewportExtEx32
0294 stdcall ScaleWindowExtEx(long long long long long ptr) ScaleWindowExtEx32
0295 stub SelectBrushLocal
0296 stub SelectClipPath
0297 stdcall SelectClipRgn(long long) SelectClipRgn
0298 stub SelectFontLocal
0299 stdcall SelectObject(long long) SelectObject
0300 stdcall SelectPalette(long long long) SelectPalette
0301 stub SetAbortProc
0302 stub SetArcDirection
0303 stdcall SetBitmapBits(long long ptr) SetBitmapBits
0304 stdcall SetBitmapDimensionEx(long long long ptr) SetBitmapDimensionEx32
0305 stdcall SetBkColor(long long) SetBkColor
0306 stdcall SetBkMode(long long) SetBkMode
0307 stub SetBoundsRect
0308 stdcall SetBrushOrgEx(long long long ptr) SetBrushOrgEx
0309 stub SetColorAdjustment
0310 stub SetColorSpace
0311 stub SetDIBColorTable
0312 stdcall SetDIBits(long long long long ptr ptr long) SetDIBits
0313 stdcall SetDIBitsToDevice(long long long long long long long long long
                               ptr ptr long) SetDIBitsToDevice
0314 stub SetDeviceGammaRamp
0315 stub SetEnhMetaFileBits
0316 stub SetFontEnumeration
0317 stub SetGraphicsMode
0318 stub SetICMMode
0319 stub SetICMProfileA
0320 stub SetICMProfileW
0321 stdcall SetMapMode(long long) SetMapMode
0322 stub SetMapperFlags
0323 stub SetMetaFileBitsEx
0324 stub SetMetaRgn
0325 stub SetMiterLimit
0326 stub SetPaletteEntries
0327 stdcall SetPixel(long long long long)	SetPixel
0328 stub SetPixelFormat
0329 stub SetPixelV
0330 stub SetPolyFillMode
0331 stdcall SetROP2(long long) SetROP2
0332 stdcall SetRectRgn(long long long long long) SetRectRgn
0333 stub SetRelAbs
0334 stdcall SetStretchBltMode(long long) SetStretchBltMode
0335 stub SetSystemPaletteUse
0336 stub SetTextAlign
0337 stub SetTextCharacterExtra
0338 stdcall SetTextColor(long long) SetTextColor
0339 stub SetTextJustification
0340 stdcall SetViewportExtEx(long long long ptr) SetViewportExtEx32
0341 stdcall SetViewportOrgEx(long long long ptr) SetViewportOrgEx32
0342 stub SetVirtualResolution
0343 stub SetWinMetaFileBits
0344 stdcall SetWindowExtEx(long long long ptr) SetWindowExtEx32
0345 stdcall SetWindowOrgEx(long long long ptr) SetWindowOrgEx32
0346 stub SetWorldTransform
0347 stub StartDocA
0348 stub StartDocW
0349 stub StartPage
0350 stdcall StretchBlt(long long long long long long long long long long long) StretchBlt
0351 stdcall StretchDIBits(long long long long long long long
                               long long long long long long) StretchDIBits
0352 stub StrokeAndFillPath
0353 stub StrokePath
0354 stub SwapBuffers
0355 stdcall TextOutA(long long long ptr long) TextOut32A
0356 stdcall TextOutW(long long long ptr long) TextOut32W
0357 stub UnloadNetworkFonts
0358 stdcall UnrealizeObject(long) UnrealizeObject
0359 stub UpdateColors
0360 stub WidenPath
0361 stub pstackConnect
#late additions
0362 stub DeviceCapabilitiesEx
0363 stub GdiDciBeginAccess
0364 stub GdiDciCreateOffscreenSurface
0365 stub GdiDciCreateOverlaySurface
0366 stub GdiDciCreatePrimarySurface
0367 stub GdiDciDestroySurface
0368 stub GdiDciDrawSurface
0369 stub GdiDciEndAccess
0370 stub GdiDciEnumSurface
0371 stub GdiDciInitialize
0372 stub GdiDciSetClipList
0373 stub GdiDciSetDestination
0374 stub GdiDllInitialize
0375 stub GdiGetLocalBitmap
0376 stub GdiWinWatchClose
0377 stub GdiWinWatchDidStatusChange
0378 stub GdiWinWatchGetClipList
0379 stub GdiWinWatchOpen
0380 stub GetGlyphOutlineWow
0381 stub GetTextCharsetInfo
0382 stub TranslateCharsetInfo
0383 stub UpdateICMRegKeyA
0384 stub UpdateICMRegKeyW
0385 stub gdiPlaySpoolStream
0386 return SetObjectOwner 8 0
