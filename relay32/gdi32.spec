name	gdi32
type	win32
init	MAIN_GdiInit

  0 stub AbortDoc
  1 stdcall AbortPath(long) AbortPath32
  2 stdcall AddFontResourceA(str) AddFontResource32A
  3 stub AddFontResourceTracking
  4 stdcall AddFontResourceW(wstr) AddFontResource32W
  5 stub AngleArc
  6 stdcall AnimatePalette(long long long ptr) AnimatePalette32
  7 stdcall Arc(long long long long long long long long long) Arc32
  8 stub ArcTo
  9 stdcall BeginPath(long) BeginPath32
 10 stdcall BitBlt(long long long long long long long long long) BitBlt32
 11 stub CancelDC
 12 stub CheckColorsInGamut
 13 stdcall ChoosePixelFormat(long ptr) ChoosePixelFormat
 14 stdcall Chord(long long long long long long long long long) Chord32
 15 stdcall CloseEnhMetaFile(long) CloseEnhMetaFile32
 16 stdcall CloseFigure(long) CloseFigure32
 17 stdcall CloseMetaFile(long) CloseMetaFile32
 18 stub ColorMatchToTarget
 19 stdcall CombineRgn(long long long long) CombineRgn32
 20 stdcall CombineTransform(ptr ptr ptr) CombineTransform
 21 stdcall CopyEnhMetaFileA(long str) CopyEnhMetaFile32A
 22 stub CopyEnhMetaFileW
 23 stdcall CopyMetaFileA(long str) CopyMetaFile32A
 24 stdcall CopyMetaFileW(long wstr) CopyMetaFile32W
 25 stdcall CreateBitmap(long long long long ptr) CreateBitmap32
 26 stdcall CreateBitmapIndirect(ptr) CreateBitmapIndirect32
 27 stdcall CreateBrushIndirect(ptr) CreateBrushIndirect32
 28 stub CreateColorSpaceA
 29 stub CreateColorSpaceW
 30 stdcall CreateCompatibleBitmap(long long long) CreateCompatibleBitmap32
 31 stdcall CreateCompatibleDC(long) CreateCompatibleDC32
 32 stdcall CreateDCA(str str str ptr) CreateDC32A
 33 stdcall CreateDCW(wstr wstr wstr ptr) CreateDC32W
 34 stdcall CreateDIBPatternBrush(long long) CreateDIBPatternBrush32
 35 stdcall CreateDIBPatternBrushPt(long long) CreateDIBPatternBrushPt32
 36 stdcall CreateDIBSection(long ptr long ptr long long) CreateDIBSection32
 37 stdcall CreateDIBitmap(long ptr long ptr ptr long) CreateDIBitmap32
 38 stdcall CreateDiscardableBitmap(long long long) CreateDiscardableBitmap32
 39 stdcall CreateEllipticRgn(long long long long) CreateEllipticRgn32
 40 stdcall CreateEllipticRgnIndirect(ptr) CreateEllipticRgnIndirect32
 41 stdcall CreateEnhMetaFileA(long ptr ptr ptr) CreateEnhMetaFile32A
 42 stub CreateEnhMetaFileW
 43 stdcall CreateFontA(long long long long long long long long long long long long long str) CreateFont32A
 44 stdcall CreateFontIndirectA(ptr) CreateFontIndirect32A
 45 stdcall CreateFontIndirectW(ptr) CreateFontIndirect32W
 46 stdcall CreateFontW(long long long long long long long long long long long long long wstr) CreateFont32W
 47 stdcall CreateHalftonePalette(long) CreateHalftonePalette
 48 stdcall CreateHatchBrush(long long) CreateHatchBrush32
 49 stdcall CreateICA(str str str ptr) CreateIC32A
 50 stdcall CreateICW(wstr wstr wstr ptr) CreateIC32W
 51 stdcall CreateMetaFileA(str) CreateMetaFile32A
 52 stub CreateMetaFileW
 53 stdcall CreatePalette(ptr) CreatePalette32
 54 stdcall CreatePatternBrush(long) CreatePatternBrush32
 55 stdcall CreatePen(long long long) CreatePen32
 56 stdcall CreatePenIndirect(ptr) CreatePenIndirect32
 57 stdcall CreatePolyPolygonRgn(ptr ptr long long) CreatePolyPolygonRgn32
 58 stdcall CreatePolygonRgn(ptr long long) CreatePolygonRgn32
 59 stdcall CreateRectRgn(long long long long) CreateRectRgn32
 60 stdcall CreateRectRgnIndirect(ptr) CreateRectRgnIndirect32
 61 stdcall CreateRoundRectRgn(long long long long long long) CreateRoundRectRgn32
 62 stdcall CreateScalableFontResourceA(long str str str) CreateScalableFontResource32A
 63 stdcall CreateScalableFontResourceW(long wstr wstr wstr) CreateScalableFontResource32W
 64 stdcall CreateSolidBrush(long) CreateSolidBrush32
 65 stdcall DPtoLP(long ptr long) DPtoLP32
 66 stub DeleteColorSpace
 67 stdcall DeleteDC(long) DeleteDC32
 68 stdcall DeleteEnhMetaFile(long) DeleteEnhMetaFile
 69 stdcall DeleteMetaFile(long) DeleteMetaFile32
 70 stdcall DeleteObject(long)	DeleteObject32
 71 stdcall DescribePixelFormat(long long long ptr) DescribePixelFormat
 72 stub DeviceCapabilitiesExA
 73 stub DeviceCapabilitiesExW
 74 stub DrawEscape
 75 stdcall Ellipse(long long long long long) Ellipse32
 76 stdcall EndDoc(long) EndDoc32
 77 stdcall EndPage(long) EndPage32
 78 stdcall EndPath(long) EndPath32
 79 stdcall EnumEnhMetaFile(long long ptr ptr ptr) EnumEnhMetaFile32
 80 stdcall EnumFontFamiliesA(long str ptr long) EnumFontFamilies32A
 81 stdcall EnumFontFamiliesExA(long str ptr long long) EnumFontFamiliesEx32A
 82 stdcall EnumFontFamiliesExW(long wstr ptr long long) EnumFontFamiliesEx32W
 83 stdcall EnumFontFamiliesW(long wstr ptr long) EnumFontFamilies32W
 84 stdcall EnumFontsA(long str ptr long) EnumFonts32A
 85 stdcall EnumFontsW(long wstr ptr long) EnumFonts32W
 86 stub EnumICMProfilesA
 87 stub EnumICMProfilesW
 88 stdcall EnumMetaFile(long long ptr ptr) EnumMetaFile32
 89 stdcall EnumObjects(long long ptr long) EnumObjects32
 90 stdcall EqualRgn(long long) EqualRgn32
 91 stdcall Escape(long long long ptr ptr) Escape32
 92 stdcall ExcludeClipRect(long long long long long) ExcludeClipRect32
 93 stdcall ExtCreatePen(long long ptr long ptr) ExtCreatePen32
 94 stdcall ExtCreateRegion(ptr long ptr) ExtCreateRegion
 95 stdcall ExtEscape(long long long ptr long ptr) ExtEscape32
 96 stdcall ExtFloodFill(long long long long long) ExtFloodFill32
 97 stdcall ExtSelectClipRgn(long long long) ExtSelectClipRgn
 98 stdcall ExtTextOutA(long long long long ptr str long ptr) ExtTextOut32A
 99 stdcall ExtTextOutW(long long long long ptr wstr long ptr) ExtTextOut32W
100 stdcall FillPath(long) FillPath32
101 stdcall FillRgn(long long long) FillRgn32
102 stdcall FixBrushOrgEx(long long long ptr) FixBrushOrgEx
103 stub FlattenPath
104 stdcall FloodFill(long long long long) FloodFill32
105 stdcall FrameRgn(long long long long long) FrameRgn32
106 stub FreeImageColorMatcher
107 stub GdiAssociateObject
108 stub GdiCleanCacheDC
109 stub GdiComment
110 stub GdiConvertAndCheckDC
111 stub GdiConvertBitmap
112 stub GdiConvertBrush
113 stub GdiConvertDC
114 stub GdiConvertEnhMetaFile
115 stub GdiConvertFont
116 stub GdiConvertMetaFilePict
117 stub GdiConvertPalette
118 stub GdiConvertRegion
119 stub GdiCreateLocalBitmap
120 stub GdiCreateLocalBrush
121 stub GdiCreateLocalEnhMetaFile
122 stub GdiCreateLocalFont
123 stub GdiCreateLocalMetaFilePict
124 stub GdiCreateLocalPalette
125 stub GdiCreateLocalRegion
126 stub GdiDeleteLocalDC
127 stub GdiDeleteLocalObject
128 stdcall GdiFlush() GdiFlush
129 stdcall GdiGetBatchLimit() GdiGetBatchLimit
130 stub GdiGetLocalBrush
131 stub GdiGetLocalDC
132 stub GdiGetLocalFont
133 stub GdiIsMetaFileDC
134 stub GdiPlayDCScript
135 stub GdiPlayJournal
136 stub GdiPlayScript
137 stub GdiReleaseLocalDC
138 stub GdiSetAttrs
139 stdcall GdiSetBatchLimit(long) GdiSetBatchLimit
140 stub GdiSetServerAttr
141 stdcall GetArcDirection(long) GetArcDirection32
142 stub GetAspectRatioFilterEx
143 stdcall GetBitmapBits(long long ptr) GetBitmapBits32
144 stdcall GetBitmapDimensionEx(long ptr) GetBitmapDimensionEx32
145 stdcall GetBkColor(long) GetBkColor32
146 stdcall GetBkMode(long) GetBkMode32
147 stub GetBoundsRect
148 stdcall GetBrushOrgEx(long ptr) GetBrushOrgEx32
149 stdcall GetCharABCWidthsA(long long long ptr) GetCharABCWidths32A
150 stub GetCharABCWidthsFloatA
151 stub GetCharABCWidthsFloatW
152 stdcall GetCharABCWidthsW(long long long ptr) GetCharABCWidths32W
153 stdcall GetCharWidth32A(long long long long) GetCharWidth32A
154 stdcall GetCharWidth32W(long long long long) GetCharWidth32W
155 stdcall GetCharWidthA(long long long long) GetCharWidth32A
156 stub GetCharWidthFloatA
157 stub GetCharWidthFloatW
158 stdcall GetCharWidthW(long long long long) GetCharWidth32W
159 stub GetCharWidthWOW
160 stdcall GetCharacterPlacementA(long str long long ptr long) GetCharacterPlacement32A
161 stdcall GetCharacterPlacementW(long wstr long long ptr long) GetCharacterPlacement32W
162 stdcall GetClipBox(long ptr) GetClipBox32
163 stdcall GetClipRgn(long long) GetClipRgn32
164 stub GetColorAdjustment
165 stdcall GetColorSpace(long) GetColorSpace
166 stdcall GetCurrentObject(long long) GetCurrentObject
167 stdcall GetCurrentPositionEx(long ptr) GetCurrentPositionEx32
168 stdcall GetDCOrgEx(long ptr) GetDCOrgEx
169 stdcall GetDIBColorTable(long long long ptr) GetDIBColorTable32
170 stdcall GetDIBits(long long long long ptr ptr long) GetDIBits32
171 stdcall GetDeviceCaps(long long) GetDeviceCaps32
172 stub GetDeviceGammaRamp
173 stub GetETM
174 stdcall GetEnhMetaFileA(ptr) GetEnhMetaFile32A
175 stdcall GetEnhMetaFileBits(long long ptr) GetEnhMetaFileBits
176 stdcall GetEnhMetaFileDescriptionA(long long ptr) GetEnhMetaFileDescription32A
177 stdcall GetEnhMetaFileDescriptionW(long long ptr) GetEnhMetaFileDescription32W
178 stdcall GetEnhMetaFileHeader(long long ptr) GetEnhMetaFileHeader
179 stdcall GetEnhMetaFilePaletteEntries (long long ptr) GetEnhMetaFilePaletteEntries
180 stub GetEnhMetaFileW
181 stdcall GetFontData(long long long ptr long) GetFontData32
182 stdcall GetFontLanguageInfo(long) GetFontLanguageInfo32
183 stub GetFontResourceInfo
184 stub GetFontResourceInfoW
185 stub GetGlyphOutline
186 stdcall GetGlyphOutlineA(long long long ptr long ptr ptr) GetGlyphOutline32A
187 stdcall GetGlyphOutlineW(long long long ptr long ptr ptr) GetGlyphOutline32W
188 stdcall GetGraphicsMode(long) GetGraphicsMode
189 stub GetICMProfileA
190 stub GetICMProfileW
191 stub GetKerningPairs
192 stdcall GetKerningPairsA(long long ptr) GetKerningPairs32A
193 stdcall GetKerningPairsW(long long ptr) GetKerningPairs32W
194 stub GetLogColorSpaceA
195 stub GetLogColorSpaceW
196 stdcall GetMapMode(long) GetMapMode32
197 stdcall GetMetaFileA(str) GetMetaFile32A
198 stdcall GetMetaFileBitsEx(long long ptr) GetMetaFileBitsEx
199 stdcall GetMetaFileW(wstr) GetMetaFile32W
200 stub GetMetaRgn
201 stub GetMiterLimit
202 stdcall GetNearestColor(long long) GetNearestColor32
203 stdcall GetNearestPaletteIndex(long long) GetNearestPaletteIndex32
204 stdcall GetObjectA(long long ptr) GetObject32A
205 stdcall GetObjectType(long) GetObjectType
206 stdcall GetObjectW(long long ptr) GetObject32W
207 stdcall GetOutlineTextMetricsA(long long ptr) GetOutlineTextMetrics32A
208 stub GetOutlineTextMetricsW
209 stdcall GetPaletteEntries(long long long ptr) GetPaletteEntries32
210 stdcall GetPath(long ptr ptr long) GetPath32
211 stdcall GetPixel(long long long) GetPixel32
212 stdcall GetPixelFormat(long) GetPixelFormat
213 stdcall GetPolyFillMode(long) GetPolyFillMode32
214 stdcall GetROP2(long) GetROP232
215 stdcall GetRandomRgn(long long long) GetRandomRgn
216 stdcall GetRasterizerCaps(ptr long) GetRasterizerCaps32
217 stdcall GetRegionData(long long ptr) GetRegionData32
218 stdcall GetRelAbs(long) GetRelAbs32
219 stdcall GetRgnBox(long ptr) GetRgnBox32
220 stdcall GetStockObject(long) GetStockObject32
221 stdcall GetStretchBltMode(long) GetStretchBltMode32
222 stdcall GetSystemPaletteEntries(long long long ptr) GetSystemPaletteEntries32
223 stdcall GetSystemPaletteUse(long) GetSystemPaletteUse32
224 stdcall GetTextAlign(long) GetTextAlign32
225 stdcall GetTextCharacterExtra(long) GetTextCharacterExtra32
226 stdcall GetTextCharset(long) GetTextCharset32
227 stdcall GetTextColor(long) GetTextColor32
228 stdcall GetTextExtentExPointA(long str long long ptr ptr ptr) GetTextExtentExPoint32A
229 stdcall GetTextExtentExPointW(long wstr long long ptr ptr ptr) GetTextExtentExPoint32W
230 stdcall GetTextExtentPoint32A(long ptr long ptr) GetTextExtentPoint32A
231 stdcall GetTextExtentPoint32W(long ptr long ptr) GetTextExtentPoint32W
232 stdcall GetTextExtentPointA(long ptr long ptr) GetTextExtentPoint32ABuggy
233 stdcall GetTextExtentPointW(long ptr long ptr) GetTextExtentPoint32WBuggy
234 stdcall GetTextFaceA(long long ptr) GetTextFace32A
235 stdcall GetTextFaceW(long long ptr) GetTextFace32W
236 stdcall GetTextMetricsA(long ptr) GetTextMetrics32A
237 stdcall GetTextMetricsW(long ptr) GetTextMetrics32W
238 stub GetTransform
239 stdcall GetViewportExtEx(long ptr) GetViewportExtEx32
240 stdcall GetViewportOrgEx(long ptr) GetViewportOrgEx32
241 stub GetWinMetaFileBits
242 stdcall GetWindowExtEx(long ptr) GetWindowExtEx32
243 stdcall GetWindowOrgEx(long ptr) GetWindowOrgEx32
244 stdcall GetWorldTransform(long ptr) GetWorldTransform
245 stdcall IntersectClipRect(long long long long long) IntersectClipRect32
246 stdcall InvertRgn(long long) InvertRgn32
247 stdcall LPtoDP(long ptr long) LPtoDP32
248 stdcall LineDDA(long long long long ptr long) LineDDA32
249 stdcall LineTo(long long long) LineTo32
250 stub LoadImageColorMatcherA
251 stub LoadImageColorMatcherW
252 stub MaskBlt
253 stdcall ModifyWorldTransform(long ptr long) ModifyWorldTransform
254 stdcall MoveToEx(long long long ptr) MoveToEx32
255 stdcall OffsetClipRgn(long long long) OffsetClipRgn32
256 stdcall OffsetRgn(long long long) OffsetRgn32
257 stdcall OffsetViewportOrgEx(long long long ptr) OffsetViewportOrgEx32
258 stdcall OffsetWindowOrgEx(long long long ptr) OffsetWindowOrgEx32
259 stdcall PaintRgn(long long) PaintRgn32
260 stdcall PatBlt(long long long long long long) PatBlt32
261 stdcall PathToRegion(long) PathToRegion32
262 stdcall Pie(long long long long long long long long long) Pie32
263 stdcall PlayEnhMetaFile(long long ptr) PlayEnhMetaFile
264 stdcall PlayEnhMetaFileRecord(long ptr ptr long) PlayEnhMetaFileRecord
265 stdcall PlayMetaFile(long long) PlayMetaFile32
266 stdcall PlayMetaFileRecord(long ptr ptr long) PlayMetaFileRecord32
267 stub PlgBlt
268 stdcall PolyBezier(long ptr long) PolyBezier32
269 stdcall PolyBezierTo(long ptr long) PolyBezierTo32
270 stub PolyDraw
271 stdcall PolyPolygon(long ptr ptr long) PolyPolygon32
272 stdcall PolyPolyline(long ptr ptr long) PolyPolyline32
273 stub PolyTextOutA
274 stub PolyTextOutW
275 stdcall Polygon(long ptr long) Polygon32
276 stdcall Polyline(long ptr long) Polyline32
277 stub PolylineTo
278 stdcall PtInRegion(long long long) PtInRegion32
279 stdcall PtVisible(long long long) PtVisible32
280 stdcall RealizePalette(long) RealizePalette32
281 stdcall RectInRegion(long ptr) RectInRegion32
282 stdcall RectVisible(long ptr) RectVisible32
283 stdcall Rectangle(long long long long long) Rectangle32
284 stdcall RemoveFontResourceA(str) RemoveFontResource32A
285 stub RemoveFontResourceTracking
286 stdcall RemoveFontResourceW(wstr) RemoveFontResource32W
287 stdcall ResetDCA(long ptr) ResetDC32A
288 stdcall ResetDCW(long ptr) ResetDC32W
289 stdcall ResizePalette(long long) ResizePalette32
290 stdcall RestoreDC(long long) RestoreDC32
291 stdcall RoundRect(long long long long long long long) RoundRect32
292 stdcall SaveDC(long) SaveDC32
293 stdcall ScaleViewportExtEx(long long long long long ptr) ScaleViewportExtEx32
294 stdcall ScaleWindowExtEx(long long long long long ptr) ScaleWindowExtEx32
295 stub SelectBrushLocal
296 stdcall SelectClipPath(long long) SelectClipPath32
297 stdcall SelectClipRgn(long long) SelectClipRgn32
298 stub SelectFontLocal
299 stdcall SelectObject(long long) SelectObject32
300 stdcall SelectPalette(long long long) SelectPalette32
301 stdcall SetAbortProc(long ptr) SetAbortProc32
302 stdcall SetArcDirection(long long) SetArcDirection32
303 stdcall SetBitmapBits(long long ptr) SetBitmapBits32
304 stdcall SetBitmapDimensionEx(long long long ptr) SetBitmapDimensionEx32
305 stdcall SetBkColor(long long) SetBkColor32
306 stdcall SetBkMode(long long) SetBkMode32
307 stub SetBoundsRect
308 stdcall SetBrushOrgEx(long long long ptr) SetBrushOrgEx
309 stub SetColorAdjustment
310 stub SetColorSpace
311 stdcall SetDIBColorTable(long long long ptr) SetDIBColorTable32
312 stdcall SetDIBits(long long long long ptr ptr long) SetDIBits32
313 stdcall SetDIBitsToDevice(long long long long long long long long long ptr ptr long) SetDIBitsToDevice32
314 stub SetDeviceGammaRamp
315 stdcall SetEnhMetaFileBits(long ptr) SetEnhMetaFileBits
316 stub SetFontEnumeration
317 stdcall SetGraphicsMode(long long) SetGraphicsMode
318 stdcall SetICMMode(long long) SetICMMode
319 stub SetICMProfileA
320 stub SetICMProfileW
321 stdcall SetMapMode(long long) SetMapMode32
322 stdcall SetMapperFlags(long long) SetMapperFlags32
323 stdcall SetMetaFileBitsEx(long ptr) SetMetaFileBitsEx
324 stub SetMetaRgn
325 stub SetMiterLimit
326 stdcall SetPaletteEntries(long long long ptr) SetPaletteEntries32
327 stdcall SetPixel(long long long long) SetPixel32
328 stdcall SetPixelFormat(long long ptr) SetPixelFormat
329 stdcall SetPixelV(long long long long) SetPixelV32
330 stdcall SetPolyFillMode(long long) SetPolyFillMode32
331 stdcall SetROP2(long long) SetROP232
332 stdcall SetRectRgn(long long long long long) SetRectRgn32
333 stdcall SetRelAbs(long long) SetRelAbs32
334 stdcall SetStretchBltMode(long long) SetStretchBltMode32
335 stdcall SetSystemPaletteUse(long long) SetSystemPaletteUse32
336 stdcall SetTextAlign(long long) SetTextAlign32
337 stdcall SetTextCharacterExtra(long long) SetTextCharacterExtra32
338 stdcall SetTextColor(long long) SetTextColor32
339 stdcall SetTextJustification(long long long) SetTextJustification32
340 stdcall SetViewportExtEx(long long long ptr) SetViewportExtEx32
341 stdcall SetViewportOrgEx(long long long ptr) SetViewportOrgEx32
342 stub SetVirtualResolution
343 stdcall SetWinMetaFileBits(long ptr long ptr) SetWinMetaFileBits
344 stdcall SetWindowExtEx(long long long ptr) SetWindowExtEx32
345 stdcall SetWindowOrgEx(long long long ptr) SetWindowOrgEx32
346 stdcall SetWorldTransform(long ptr) SetWorldTransform
347 stdcall StartDocA(long ptr) StartDoc32A
348 stub StartDocW
349 stdcall StartPage(long) StartPage32 
350 stdcall StretchBlt(long long long long long long long long long long long) StretchBlt32
351 stdcall StretchDIBits(long long long long long long long long long ptr ptr long long) StretchDIBits32
352 stub StrokeAndFillPath
353 stub StrokePath
354 stdcall SwapBuffers(long) SwapBuffers
355 stdcall TextOutA(long long long str long) TextOut32A
356 stdcall TextOutW(long long long wstr long) TextOut32W
357 stub UnloadNetworkFonts
358 stdcall UnrealizeObject(long) UnrealizeObject32
359 stdcall UpdateColors(long) UpdateColors32
360 stub WidenPath
361 stub pstackConnect
#late additions
362 stub DeviceCapabilitiesEx
363 stub GdiDciBeginAccess
364 stub GdiDciCreateOffscreenSurface
365 stub GdiDciCreateOverlaySurface
366 stub GdiDciCreatePrimarySurface
367 stub GdiDciDestroySurface
368 stub GdiDciDrawSurface
369 stub GdiDciEndAccess
370 stub GdiDciEnumSurface
371 stub GdiDciInitialize
372 stub GdiDciSetClipList
373 stub GdiDciSetDestination
374 stub GdiDllInitialize
375 stub GdiGetLocalBitmap
376 stub GdiWinWatchClose
377 stub GdiWinWatchDidStatusChange
378 stub GdiWinWatchGetClipList
379 stub GdiWinWatchOpen
380 stub GetGlyphOutlineWow
381 stdcall GetTextCharsetInfo(long ptr long) GetTextCharsetInfo
382 stdcall TranslateCharsetInfo(ptr ptr long) TranslateCharSetInfo
383 stub UpdateICMRegKeyA
384 stub UpdateICMRegKeyW
385 stub gdiPlaySpoolStream
386 stdcall SetObjectOwner(long long) SetObjectOwner32
387 stub UpdateICMRegKey
388 extern pfnRealizePalette pfnRealizePalette
389 extern pfnSelectPalette pfnSelectPalette
