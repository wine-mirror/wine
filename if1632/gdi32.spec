name	gdi32
type	win32
base	1

0000 stub AbortDoc
0001 stub AbortPath
0002 stdcall AddFontResourceA(ptr) AddFontResource32A
0003 stub AddFontResourceTracking
0004 stdcall AddFontResourceW(ptr) AddFontResource32W
0005 stub AngleArc
0006 stdcall AnimatePalette(long long long ptr) AnimatePalette32
0007 stdcall Arc(long long long long long long long long long) Arc32
0008 stub ArcTo
0009 stub BeginPath
0010 stdcall BitBlt(long long long long long long long long long) BitBlt32
0011 stub CancelDC
0012 stub CheckColorsInGamut
0013 stub ChoosePixelFormat
0014 stdcall Chord(long long long long long long long long long) Chord32
0015 stub CloseEnhMetaFile
0016 stub CloseFigure
0017 stub CloseMetaFile
0018 stub ColorMatchToTarget
0019 stdcall CombineRgn(long long long long) CombineRgn32
0020 stub CombineTransform
0021 stub CopyEnhMetaFileA
0022 stub CopyEnhMetaFileW
0023 stdcall CopyMetaFileA(long ptr) CopyMetaFile32A
0024 stdcall CopyMetaFileW(long ptr) CopyMetaFile32W
0025 stdcall CreateBitmap(long long long long ptr) CreateBitmap32
0026 stdcall CreateBitmapIndirect(ptr) CreateBitmapIndirect32
0027 stdcall CreateBrushIndirect(ptr) CreateBrushIndirect32
0028 stub CreateColorSpaceA
0029 stub CreateColorSpaceW
0030 stdcall CreateCompatibleBitmap(long long long) CreateCompatibleBitmap32
0031 stdcall CreateCompatibleDC(long) CreateCompatibleDC32
0032 stdcall CreateDCA(ptr ptr ptr ptr) CreateDC32A
0033 stdcall CreateDCW(ptr ptr ptr ptr) CreateDC32W
0034 stdcall CreateDIBPatternBrush(long long) CreateDIBPatternBrush32
0035 stub CreateDIBPatternBrushPt
0036 stub CreateDIBSection
0037 stdcall CreateDIBitmap(long ptr long ptr ptr long) CreateDIBitmap32
0038 stdcall CreateDiscardableBitmap(long long long) CreateDiscardableBitmap32
0039 stdcall CreateEllipticRgn(long long long long) CreateEllipticRgn32
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
0048 stdcall CreateHatchBrush(long long) CreateHatchBrush32
0049 stdcall CreateICA(ptr ptr ptr ptr) CreateIC32A
0050 stdcall CreateICW(ptr ptr ptr ptr) CreateIC32W
0051 stub CreateMetaFileA
0052 stub CreateMetaFileW
0053 stdcall CreatePalette(ptr) CreatePalette32
0054 stdcall CreatePatternBrush(long) CreatePatternBrush32
0055 stdcall CreatePen(long long long) CreatePen32
0056 stdcall CreatePenIndirect(ptr) CreatePenIndirect32
0057 stdcall CreatePolyPolygonRgn(ptr ptr long long) CreatePolyPolygonRgn32
0058 stdcall CreatePolygonRgn(ptr long long) CreatePolygonRgn32
0059 stdcall CreateRectRgn(long long long long) CreateRectRgn32
0060 stdcall CreateRectRgnIndirect(ptr) CreateRectRgnIndirect32
0061 stdcall CreateRoundRectRgn(long long long long long long)
             CreateRoundRectRgn32
0062 stdcall CreateScalableFontResourceA(long ptr ptr ptr) CreateScalableFontResource32A
0063 stdcall CreateScalableFontResourceW(long ptr ptr ptr) CreateScalableFontResource32W
0064 stdcall CreateSolidBrush(long) CreateSolidBrush32
0065 stdcall DPtoLP(long ptr long) DPtoLP32
0066 stub DeleteColorSpace
0067 stdcall DeleteDC(long) DeleteDC32
0068 stub DeleteEnhMetaFile
0069 stub DeleteMetaFile
0070 stdcall DeleteObject(long)	DeleteObject32
0071 stub DescribePixelFormat
0072 stub DeviceCapabilitiesExA
0073 stub DeviceCapabilitiesExW
0074 stub DrawEscape
0075 stdcall Ellipse(long long long long long) Ellipse32
0076 stub EndDoc
0077 stub EndPage
0078 stub EndPath
0079 stub EnumEnhMetaFile
0080 stdcall EnumFontFamiliesA(long ptr ptr long) THUNK_EnumFontFamilies32A
0081 stdcall EnumFontFamiliesExA(long ptr ptr long long) THUNK_EnumFontFamiliesEx32A
0082 stdcall EnumFontFamiliesExW(long ptr ptr long long) THUNK_EnumFontFamiliesEx32W
0083 stdcall EnumFontFamiliesW(long ptr ptr long) THUNK_EnumFontFamilies32W
0084 stdcall EnumFontsA(long ptr ptr long) THUNK_EnumFonts32A
0085 stdcall EnumFontsW(long ptr ptr long) THUNK_EnumFonts32W
0086 stub EnumICMProfilesA
0087 stub EnumICMProfilesW
0088 stub EnumMetaFile
0089 stdcall EnumObjects(long long ptr long) THUNK_EnumObjects32
0090 stdcall EqualRgn(long long) EqualRgn32
0091 stdcall Escape(long long long ptr ptr) Escape32
0092 stdcall ExcludeClipRect(long long long long long) ExcludeClipRect32
0093 stub ExtCreatePen
0094 stub ExtCreateRegion
0095 stub ExtEscape
0096 stdcall ExtFloodFill(long long long long long) ExtFloodFill32
0097 stub ExtSelectClipRgn
0098 stdcall ExtTextOutA(long long long long ptr ptr long ptr) ExtTextOut32A
0099 stdcall ExtTextOutW(long long long long ptr ptr long ptr) ExtTextOut32W
0100 stub FillPath
0101 stdcall FillRgn(long long long) FillRgn32
0102 stub FixBrushOrgEx
0103 stub FlattenPath
0104 stdcall FloodFill(long long long long) FloodFill32
0105 stdcall FrameRgn(long long long long long) FrameRgn32
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
0129 return GdiGetBatchLimit 0 1
0130 stub GdiGetLocalBrush
0131 stub GdiGetLocalDC
0132 stub GdiGetLocalFont
0133 stub GdiIsMetaFileDC
0134 stub GdiPlayDCScript
0135 stub GdiPlayJournal
0136 stub GdiPlayScript
0137 stub GdiReleaseLocalDC
0138 stub GdiSetAttrs
0139 return GdiSetBatchLimit 4 1
0140 stub GdiSetServerAttr
0141 stub GetArcDirection
0142 stub GetAspectRatioFilterEx
0143 stdcall GetBitmapBits(long long ptr) GetBitmapBits32
0144 stdcall GetBitmapDimensionEx(long ptr) GetBitmapDimensionEx32
0145 stdcall GetBkColor(long) GetBkColor32
0146 stdcall GetBkMode(long) GetBkMode32
0147 stub GetBoundsRect
0148 stdcall GetBrushOrgEx(long ptr) GetBrushOrgEx32
0149 stdcall GetCharABCWidthsA(long long long ptr) GetCharABCWidths32A
0150 stub GetCharABCWidthsFloatA
0151 stub GetCharABCWidthsFloatW
0152 stdcall GetCharABCWidthsW(long long long ptr) GetCharABCWidths32W
0153 stdcall GetCharWidth32A(long long long long) GetCharWidth32A
0154 stdcall GetCharWidth32W(long long long long) GetCharWidth32W
0155 stdcall GetCharWidthA(long long long long) GetCharWidth32A
0156 stub GetCharWidthFloatA
0157 stub GetCharWidthFloatW
0158 stdcall GetCharWidthW(long long long long) GetCharWidth32W
0159 stub GetCharWidthWOW
0160 stub GetCharacterPlacementA
0161 stub GetCharacterPlacementW
0162 stdcall GetClipBox(long ptr) GetClipBox32
0163 stdcall GetClipRgn(long long) GetClipRgn32
0164 stub GetColorAdjustment
0165 stub GetColorSpace
0166 stub GetCurrentObject
0167 stdcall GetCurrentPositionEx(long ptr) GetCurrentPositionEx32
0168 stdcall GetDCOrgEx(long ptr) GetDCOrgEx
0169 stub GetDIBColorTable
0170 stdcall GetDIBits(long long long long ptr ptr long) GetDIBits32
0171 stdcall GetDeviceCaps(long long) GetDeviceCaps32
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
0186 stdcall GetGlyphOutlineA(long long long ptr long ptr ptr) GetGlyphOutline32A
0187 stdcall GetGlyphOutlineW(long long long ptr long ptr ptr) GetGlyphOutline32W
0188 return GetGraphicsMode 4 1 	# just return 1
0189 stub GetICMProfileA
0190 stub GetICMProfileW
0191 stub GetKerningPairs
0192 stdcall GetKerningPairsA(long long ptr) GetKerningPairs32A
0193 stdcall GetKerningPairsW(long long ptr) GetKerningPairs32W
0194 stub GetLogColorSpaceA
0195 stub GetLogColorSpaceW
0196 stdcall GetMapMode(long) GetMapMode32
0197 stdcall GetMetaFileA(ptr) GetMetaFile32A
0198 stub GetMetaFileBitsEx
0199 stdcall GetMetaFileW(ptr) GetMetaFile32W
0200 stub GetMetaRgn
0201 stub GetMiterLimit
0202 stdcall GetNearestColor(long long) GetNearestColor32
0203 stdcall GetNearestPaletteIndex(long long) GetNearestPaletteIndex32
0204 stdcall GetObjectA(long long ptr) GetObject32A
0205 stub GetObjectType
0206 stdcall GetObjectW(long long ptr) GetObject32W
0207 stub GetOutlineTextMetricsA
0208 stub GetOutlineTextMetricsW
0209 stdcall GetPaletteEntries(long long long ptr) GetPaletteEntries32
0210 stub GetPath
0211 stdcall GetPixel(long long long) GetPixel32
0212 stub GetPixelFormat
0213 stdcall GetPolyFillMode(long) GetPolyFillMode32
0214 stdcall GetROP2(long) GetROP232
0215 stub GetRandomRgn
0216 stdcall GetRasterizerCaps(ptr long) GetRasterizerCaps32
0217 stub GetRegionData
0218 stdcall GetRelAbs(long) GetRelAbs32
0219 stdcall GetRgnBox(long ptr) GetRgnBox32
0220 stdcall GetStockObject(long) GetStockObject32
0221 stdcall GetStretchBltMode(long) GetStretchBltMode32
0222 stdcall GetSystemPaletteEntries(long long long ptr) GetSystemPaletteEntries32
0223 stdcall GetSystemPaletteUse() GetSystemPaletteUse32
0224 stdcall GetTextAlign(long) GetTextAlign32
0225 stdcall GetTextCharacterExtra(long) GetTextCharacterExtra32
0226 stdcall GetTextCharset(long) GetTextCharset32
0227 stdcall GetTextColor(long) GetTextColor32
0228 stdcall GetTextExtentExPointA(long ptr long long ptr ptr ptr) GetTextExtentExPoint32A
0229 stdcall GetTextExtentExPointW(long ptr long long ptr ptr ptr) GetTextExtentExPoint32W
0230 stdcall GetTextExtentPoint32A(long ptr long ptr) GetTextExtentPoint32A
0231 stdcall GetTextExtentPoint32W(long ptr long ptr) GetTextExtentPoint32W
0232 stdcall GetTextExtentPointA(long ptr long ptr) GetTextExtentPoint32ABuggy
0233 stdcall GetTextExtentPointW(long ptr long ptr) GetTextExtentPoint32WBuggy
0234 stdcall GetTextFaceA(long long ptr) GetTextFace32A
0235 stdcall GetTextFaceW(long long ptr) GetTextFace32W
0236 stdcall GetTextMetricsA(long ptr) GetTextMetrics32A
0237 stdcall GetTextMetricsW(long ptr) GetTextMetrics32W
0238 stub GetTransform
0239 stdcall GetViewportExtEx(long ptr) GetViewportExtEx32
0240 stdcall GetViewportOrgEx(long ptr) GetViewportOrgEx32
0241 stub GetWinMetaFileBits
0242 stdcall GetWindowExtEx(long ptr) GetWindowExtEx32
0243 stdcall GetWindowOrgEx(long ptr) GetWindowOrgEx32
0244 return GetWorldTransform 8 0
0245 stdcall IntersectClipRect(long long long long long) IntersectClipRect32
0246 stdcall InvertRgn(long long) InvertRgn32
0247 stdcall LPtoDP(long ptr long) LPtoDP32
0248 stdcall LineDDA(long long long long ptr long) THUNK_LineDDA32
0249 stdcall LineTo(long long long) LineTo32
0250 stub LoadImageColorMatcherA
0251 stub LoadImageColorMatcherW
0252 stub MaskBlt
0253 stub ModifyWorldTransform
0254 stdcall MoveToEx(long long long ptr) MoveToEx32
0255 stdcall OffsetClipRgn(long long long) OffsetClipRgn32
0256 stdcall OffsetRgn(long long long) OffsetRgn32
0257 stdcall OffsetViewportOrgEx(long long long ptr) OffsetViewportOrgEx32
0258 stdcall OffsetWindowOrgEx(long long long ptr) OffsetWindowOrgEx32
0259 stdcall PaintRgn(long long) PaintRgn32
0260 stdcall PatBlt(long long long long long long) PatBlt32
0261 stub PathToRegion
0262 stdcall Pie(long long long long long long long long long) Pie32
0263 stub PlayEnhMetaFile
0264 stub PlayEnhMetaFileRecord
0265 stdcall PlayMetaFile(long long) PlayMetaFile32
0266 stub PlayMetaFileRecord
0267 stub PlgBlt
0268 stub PolyBezier
0269 stub PolyBezierTo
0270 stub PolyDraw
0271 stdcall PolyPolygon(long ptr ptr long) PolyPolygon32
0272 stub PolyPolyline
0273 stub PolyTextOutA
0274 stub PolyTextOutW
0275 stdcall Polygon(long ptr long) Polygon32
0276 stdcall Polyline(long ptr long) Polyline32
0277 stub PolylineTo
0278 stdcall PtInRegion(long long long) PtInRegion32
0279 stdcall PtVisible(long long long) PtVisible32
0280 stdcall RealizePalette(long) RealizePalette32
0281 stdcall RectInRegion(long ptr) RectInRegion32
0282 stdcall RectVisible(long ptr) RectVisible32
0283 stdcall Rectangle(long long long long long) Rectangle32
0284 stdcall RemoveFontResourceA(ptr) RemoveFontResource32A
0285 stub RemoveFontResourceTracking
0286 stdcall RemoveFontResourceW(ptr) RemoveFontResource32W
0287 stdcall ResetDCA(long ptr) ResetDC32A
0288 stdcall ResetDCW(long ptr) ResetDC32W
0289 stdcall ResizePalette(long long) ResizePalette32
0290 stdcall RestoreDC(long long) RestoreDC32
0291 stdcall RoundRect(long long long long long long long) RoundRect32
0292 stdcall SaveDC(long) SaveDC32
0293 stdcall ScaleViewportExtEx(long long long long long ptr) ScaleViewportExtEx32
0294 stdcall ScaleWindowExtEx(long long long long long ptr) ScaleWindowExtEx32
0295 stub SelectBrushLocal
0296 stub SelectClipPath
0297 stdcall SelectClipRgn(long long) SelectClipRgn32
0298 stub SelectFontLocal
0299 stdcall SelectObject(long long) SelectObject32
0300 stdcall SelectPalette(long long long) SelectPalette32
0301 stub SetAbortProc
0302 stub SetArcDirection
0303 stdcall SetBitmapBits(long long ptr) SetBitmapBits32
0304 stdcall SetBitmapDimensionEx(long long long ptr) SetBitmapDimensionEx32
0305 stdcall SetBkColor(long long) SetBkColor32
0306 stdcall SetBkMode(long long) SetBkMode32
0307 stub SetBoundsRect
0308 stdcall SetBrushOrgEx(long long long ptr) SetBrushOrgEx
0309 stub SetColorAdjustment
0310 stub SetColorSpace
0311 stub SetDIBColorTable
0312 stdcall SetDIBits(long long long long ptr ptr long) SetDIBits32
0313 stdcall SetDIBitsToDevice(long long long long long long long long long
                               ptr ptr long) SetDIBitsToDevice32
0314 stub SetDeviceGammaRamp
0315 stub SetEnhMetaFileBits
0316 stub SetFontEnumeration
0317 return SetGraphicsMode 8 1
0318 stub SetICMMode
0319 stub SetICMProfileA
0320 stub SetICMProfileW
0321 stdcall SetMapMode(long long) SetMapMode32
0322 stdcall SetMapperFlags(long long) SetMapperFlags32
0323 stub SetMetaFileBitsEx
0324 stub SetMetaRgn
0325 stub SetMiterLimit
0326 stdcall SetPaletteEntries(long long long ptr) SetPaletteEntries32
0327 stdcall SetPixel(long long long long) SetPixel32
0328 stub SetPixelFormat
0329 stub SetPixelV
0330 stdcall SetPolyFillMode(long long) SetPolyFillMode32
0331 stdcall SetROP2(long long) SetROP232
0332 stdcall SetRectRgn(long long long long long) SetRectRgn32
0333 stdcall SetRelAbs(long long) SetRelAbs32
0334 stdcall SetStretchBltMode(long long) SetStretchBltMode32
0335 stdcall SetSystemPaletteUse(long long) SetSystemPaletteUse32
0336 stdcall SetTextAlign(long long) SetTextAlign32
0337 stdcall SetTextCharacterExtra(long long) SetTextCharacterExtra32
0338 stdcall SetTextColor(long long) SetTextColor32
0339 stdcall SetTextJustification(long long long) SetTextJustification32
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
0350 stdcall StretchBlt(long long long long long long long long long long long)
             StretchBlt32
0351 stdcall StretchDIBits(long long long long long long long long long
                           ptr ptr long long) StretchDIBits32
0352 stub StrokeAndFillPath
0353 stub StrokePath
0354 stub SwapBuffers
0355 stdcall TextOutA(long long long ptr long) TextOut32A
0356 stdcall TextOutW(long long long ptr long) TextOut32W
0357 stub UnloadNetworkFonts
0358 stdcall UnrealizeObject(long) UnrealizeObject32
0359 stdcall UpdateColors(long) UpdateColors32
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
0382 stdcall TranslateCharsetInfo(ptr ptr long) TranslateCharSetInfo
0383 stub UpdateICMRegKeyA
0384 stub UpdateICMRegKeyW
0385 stub gdiPlaySpoolStream
0386 return SetObjectOwner 8 0
