name	gdi32
type	win32
init	MAIN_GdiInit

  0 stdcall AbortDoc(long) AbortDoc
  1 stdcall AbortPath(long) AbortPath
  2 stdcall AddFontResourceA(str) AddFontResourceA
  3 stub AddFontResourceTracking
  4 stdcall AddFontResourceW(wstr) AddFontResourceW
  5 stdcall AngleArc(long long long long long long) AngleArc
  6 stdcall AnimatePalette(long long long ptr) AnimatePalette
  7 stdcall Arc(long long long long long long long long long) Arc
  8 stdcall ArcTo(long long long long long long long long long) ArcTo
  9 stdcall BeginPath(long) BeginPath
 10 stdcall BitBlt(long long long long long long long long long) BitBlt
 11 stub CancelDC
 12 stub CheckColorsInGamut
 13 stdcall ChoosePixelFormat(long ptr) ChoosePixelFormat
 14 stdcall Chord(long long long long long long long long long) Chord
 15 stdcall CloseEnhMetaFile(long) CloseEnhMetaFile
 16 stdcall CloseFigure(long) CloseFigure
 17 stdcall CloseMetaFile(long) CloseMetaFile
 18 stub ColorMatchToTarget
 19 stdcall CombineRgn(long long long long) CombineRgn
 20 stdcall CombineTransform(ptr ptr ptr) CombineTransform
 21 stdcall CopyEnhMetaFileA(long str) CopyEnhMetaFileA
 22 stub CopyEnhMetaFileW
 23 stdcall CopyMetaFileA(long str) CopyMetaFileA
 24 stdcall CopyMetaFileW(long wstr) CopyMetaFileW
 25 stdcall CreateBitmap(long long long long ptr) CreateBitmap
 26 stdcall CreateBitmapIndirect(ptr) CreateBitmapIndirect
 27 stdcall CreateBrushIndirect(ptr) CreateBrushIndirect
 28 stub CreateColorSpaceA
 29 stub CreateColorSpaceW
 30 stdcall CreateCompatibleBitmap(long long long) CreateCompatibleBitmap
 31 stdcall CreateCompatibleDC(long) CreateCompatibleDC
 32 stdcall CreateDCA(str str str ptr) CreateDCA
 33 stdcall CreateDCW(wstr wstr wstr ptr) CreateDCW
 34 stdcall CreateDIBPatternBrush(long long) CreateDIBPatternBrush
 35 stdcall CreateDIBPatternBrushPt(long long) CreateDIBPatternBrushPt
 36 stdcall CreateDIBSection(long ptr long ptr long long) CreateDIBSection
 37 stdcall CreateDIBitmap(long ptr long ptr ptr long) CreateDIBitmap
 38 stdcall CreateDiscardableBitmap(long long long) CreateDiscardableBitmap
 39 stdcall CreateEllipticRgn(long long long long) CreateEllipticRgn
 40 stdcall CreateEllipticRgnIndirect(ptr) CreateEllipticRgnIndirect
 41 stdcall CreateEnhMetaFileA(long str ptr str) CreateEnhMetaFileA
 42 stdcall CreateEnhMetaFileW(long wstr ptr wstr) CreateEnhMetaFileW
 43 stdcall CreateFontA(long long long long long long long long long long long long long str) CreateFontA
 44 stdcall CreateFontIndirectA(ptr) CreateFontIndirectA
 45 stdcall CreateFontIndirectW(ptr) CreateFontIndirectW
 46 stdcall CreateFontW(long long long long long long long long long long long long long wstr) CreateFontW
 47 stdcall CreateHalftonePalette(long) CreateHalftonePalette
 48 stdcall CreateHatchBrush(long long) CreateHatchBrush
 49 stdcall CreateICA(str str str ptr) CreateICA
 50 stdcall CreateICW(wstr wstr wstr ptr) CreateICW
 51 stdcall CreateMetaFileA(str) CreateMetaFileA
 52 stdcall CreateMetaFileW(wstr) CreateMetaFileW
 53 stdcall CreatePalette(ptr) CreatePalette
 54 stdcall CreatePatternBrush(long) CreatePatternBrush
 55 stdcall CreatePen(long long long) CreatePen
 56 stdcall CreatePenIndirect(ptr) CreatePenIndirect
 57 stdcall CreatePolyPolygonRgn(ptr ptr long long) CreatePolyPolygonRgn
 58 stdcall CreatePolygonRgn(ptr long long) CreatePolygonRgn
 59 stdcall CreateRectRgn(long long long long) CreateRectRgn
 60 stdcall CreateRectRgnIndirect(ptr) CreateRectRgnIndirect
 61 stdcall CreateRoundRectRgn(long long long long long long) CreateRoundRectRgn
 62 stdcall CreateScalableFontResourceA(long str str str) CreateScalableFontResourceA
 63 stdcall CreateScalableFontResourceW(long wstr wstr wstr) CreateScalableFontResourceW
 64 stdcall CreateSolidBrush(long) CreateSolidBrush
 65 stdcall DPtoLP(long ptr long) DPtoLP
 66 stub DeleteColorSpace
 67 stdcall DeleteDC(long) DeleteDC
 68 stdcall DeleteEnhMetaFile(long) DeleteEnhMetaFile
 69 stdcall DeleteMetaFile(long) DeleteMetaFile
 70 stdcall DeleteObject(long)	DeleteObject
 71 stdcall DescribePixelFormat(long long long ptr) DescribePixelFormat
 72 stub DeviceCapabilitiesExA
 73 stub DeviceCapabilitiesExW
 74 stdcall DrawEscape(long long long ptr) DrawEscape
 75 stdcall Ellipse(long long long long long) Ellipse
 76 stdcall EndDoc(long) EndDoc
 77 stdcall EndPage(long) EndPage
 78 stdcall EndPath(long) EndPath
 79 stdcall EnumEnhMetaFile(long long ptr ptr ptr) EnumEnhMetaFile
 80 stdcall EnumFontFamiliesA(long str ptr long) EnumFontFamiliesA
 81 stdcall EnumFontFamiliesExA(long str ptr long long) EnumFontFamiliesExA
 82 stdcall EnumFontFamiliesExW(long wstr ptr long long) EnumFontFamiliesExW
 83 stdcall EnumFontFamiliesW(long wstr ptr long) EnumFontFamiliesW
 84 stdcall EnumFontsA(long str ptr long) EnumFontsA
 85 stdcall EnumFontsW(long wstr ptr long) EnumFontsW
 86 stub EnumICMProfilesA
 87 stub EnumICMProfilesW
 88 stdcall EnumMetaFile(long long ptr ptr) EnumMetaFile
 89 stdcall EnumObjects(long long ptr long) EnumObjects
 90 stdcall EqualRgn(long long) EqualRgn
 91 stdcall Escape(long long long ptr ptr) Escape
 92 stdcall ExcludeClipRect(long long long long long) ExcludeClipRect
 93 stdcall ExtCreatePen(long long ptr long ptr) ExtCreatePen
 94 stdcall ExtCreateRegion(ptr long ptr) ExtCreateRegion
 95 stdcall ExtEscape(long long long ptr long ptr) ExtEscape
 96 stdcall ExtFloodFill(long long long long long) ExtFloodFill
 97 stdcall ExtSelectClipRgn(long long long) ExtSelectClipRgn
 98 stdcall ExtTextOutA(long long long long ptr str long ptr) ExtTextOutA
 99 stdcall ExtTextOutW(long long long long ptr wstr long ptr) ExtTextOutW
100 stdcall FillPath(long) FillPath
101 stdcall FillRgn(long long long) FillRgn
102 stdcall FixBrushOrgEx(long long long ptr) FixBrushOrgEx
103 stdcall FlattenPath(long) FlattenPath
104 stdcall FloodFill(long long long long) FloodFill
105 stdcall FrameRgn(long long long long long) FrameRgn
106 stub FreeImageColorMatcher
107 stub GdiAssociateObject
108 stub GdiCleanCacheDC
109 stdcall GdiComment(long long ptr) GdiComment
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
141 stdcall GetArcDirection(long) GetArcDirection
142 stdcall GetAspectRatioFilterEx(long ptr) GetAspectRatioFilterEx
143 stdcall GetBitmapBits(long long ptr) GetBitmapBits
144 stdcall GetBitmapDimensionEx(long ptr) GetBitmapDimensionEx
145 stdcall GetBkColor(long) GetBkColor
146 stdcall GetBkMode(long) GetBkMode
147 stdcall GetBoundsRect(long ptr long) GetBoundsRect
148 stdcall GetBrushOrgEx(long ptr) GetBrushOrgEx
149 stdcall GetCharABCWidthsA(long long long ptr) GetCharABCWidthsA
150 stdcall GetCharABCWidthsFloatA(long long long ptr) GetCharABCWidthsFloatA
151 stdcall GetCharABCWidthsFloatW(long long long ptr) GetCharABCWidthsFloatW
152 stdcall GetCharABCWidthsW(long long long ptr) GetCharABCWidthsW
153 stdcall GetCharWidth32A(long long long long) GetCharWidth32A
154 stdcall GetCharWidth32W(long long long long) GetCharWidth32W
155 stdcall GetCharWidthA(long long long long) GetCharWidth32A
156 stdcall GetCharWidthFloatA(long long long ptr) GetCharWidthFloatA
157 stdcall GetCharWidthFloatW(long long long ptr) GetCharWidthFloatW
158 stdcall GetCharWidthW(long long long long) GetCharWidth32W
159 stub GetCharWidthWOW
160 stdcall GetCharacterPlacementA(long str long long ptr long) GetCharacterPlacementA
161 stdcall GetCharacterPlacementW(long wstr long long ptr long) GetCharacterPlacementW
162 stdcall GetClipBox(long ptr) GetClipBox
163 stdcall GetClipRgn(long long) GetClipRgn
164 stdcall GetColorAdjustment(long ptr) GetColorAdjustment
165 stdcall GetColorSpace(long) GetColorSpace
166 stdcall GetCurrentObject(long long) GetCurrentObject
167 stdcall GetCurrentPositionEx(long ptr) GetCurrentPositionEx
168 stdcall GetDCOrgEx(long ptr) GetDCOrgEx
169 stdcall GetDIBColorTable(long long long ptr) GetDIBColorTable
170 stdcall GetDIBits(long long long long ptr ptr long) GetDIBits
171 stdcall GetDeviceCaps(long long) GetDeviceCaps
172 stub GetDeviceGammaRamp
173 stub GetETM
174 stdcall GetEnhMetaFileA(str) GetEnhMetaFileA
175 stdcall GetEnhMetaFileBits(long long ptr) GetEnhMetaFileBits
176 stdcall GetEnhMetaFileDescriptionA(long long ptr) GetEnhMetaFileDescriptionA
177 stdcall GetEnhMetaFileDescriptionW(long long ptr) GetEnhMetaFileDescriptionW
178 stdcall GetEnhMetaFileHeader(long long ptr) GetEnhMetaFileHeader
179 stdcall GetEnhMetaFilePaletteEntries (long long ptr) GetEnhMetaFilePaletteEntries
180 stdcall GetEnhMetaFileW(wstr) GetEnhMetaFileW
181 stdcall GetFontData(long long long ptr long) GetFontData
182 stdcall GetFontLanguageInfo(long) GetFontLanguageInfo
183 stub GetFontResourceInfo
184 stub GetFontResourceInfoW
185 stub GetGlyphOutline
186 stdcall GetGlyphOutlineA(long long long ptr long ptr ptr) GetGlyphOutlineA
187 stdcall GetGlyphOutlineW(long long long ptr long ptr ptr) GetGlyphOutlineW
188 stdcall GetGraphicsMode(long) GetGraphicsMode
189 stub GetICMProfileA
190 stub GetICMProfileW
191 stub GetKerningPairs
192 stdcall GetKerningPairsA(long long ptr) GetKerningPairsA
193 stdcall GetKerningPairsW(long long ptr) GetKerningPairsW
194 stub GetLogColorSpaceA
195 stub GetLogColorSpaceW
196 stdcall GetMapMode(long) GetMapMode
197 stdcall GetMetaFileA(str) GetMetaFileA
198 stdcall GetMetaFileBitsEx(long long ptr) GetMetaFileBitsEx
199 stdcall GetMetaFileW(wstr) GetMetaFileW
200 stub GetMetaRgn
201 stdcall GetMiterLimit(long ptr) GetMiterLimit
202 stdcall GetNearestColor(long long) GetNearestColor
203 stdcall GetNearestPaletteIndex(long long) GetNearestPaletteIndex
204 stdcall GetObjectA(long long ptr) GetObjectA
205 stdcall GetObjectType(long) GetObjectType
206 stdcall GetObjectW(long long ptr) GetObjectW
207 stdcall GetOutlineTextMetricsA(long long ptr) GetOutlineTextMetricsA
208 stdcall GetOutlineTextMetricsW(long long ptr) GetOutlineTextMetricsW
209 stdcall GetPaletteEntries(long long long ptr) GetPaletteEntries
210 stdcall GetPath(long ptr ptr long) GetPath
211 stdcall GetPixel(long long long) GetPixel
212 stdcall GetPixelFormat(long) GetPixelFormat
213 stdcall GetPolyFillMode(long) GetPolyFillMode
214 stdcall GetROP2(long) GetROP2
215 stdcall GetRandomRgn(long long long) GetRandomRgn
216 stdcall GetRasterizerCaps(ptr long) GetRasterizerCaps
217 stdcall GetRegionData(long long ptr) GetRegionData
218 stdcall GetRelAbs(long) GetRelAbs
219 stdcall GetRgnBox(long ptr) GetRgnBox
220 stdcall GetStockObject(long) GetStockObject
221 stdcall GetStretchBltMode(long) GetStretchBltMode
222 stdcall GetSystemPaletteEntries(long long long ptr) GetSystemPaletteEntries
223 stdcall GetSystemPaletteUse(long) GetSystemPaletteUse
224 stdcall GetTextAlign(long) GetTextAlign
225 stdcall GetTextCharacterExtra(long) GetTextCharacterExtra
226 stdcall GetTextCharset(long) GetTextCharset
227 stdcall GetTextColor(long) GetTextColor
228 stdcall GetTextExtentExPointA(long str long long ptr ptr ptr) GetTextExtentExPointA
229 stdcall GetTextExtentExPointW(long wstr long long ptr ptr ptr) GetTextExtentExPointW
230 stdcall GetTextExtentPoint32A(long ptr long ptr) GetTextExtentPoint32A
231 stdcall GetTextExtentPoint32W(long ptr long ptr) GetTextExtentPoint32W
232 stdcall GetTextExtentPointA(long ptr long ptr) GetTextExtentPointA
233 stdcall GetTextExtentPointW(long ptr long ptr) GetTextExtentPointW
234 stdcall GetTextFaceA(long long ptr) GetTextFaceA
235 stdcall GetTextFaceW(long long ptr) GetTextFaceW
236 stdcall GetTextMetricsA(long ptr) GetTextMetricsA
237 stdcall GetTextMetricsW(long ptr) GetTextMetricsW
238 stub GetTransform
239 stdcall GetViewportExtEx(long ptr) GetViewportExtEx
240 stdcall GetViewportOrgEx(long ptr) GetViewportOrgEx
241 stdcall GetWinMetaFileBits(long long ptr long long) GetWinMetaFileBits
242 stdcall GetWindowExtEx(long ptr) GetWindowExtEx
243 stdcall GetWindowOrgEx(long ptr) GetWindowOrgEx
244 stdcall GetWorldTransform(long ptr) GetWorldTransform
245 stdcall IntersectClipRect(long long long long long) IntersectClipRect
246 stdcall InvertRgn(long long) InvertRgn
247 stdcall LPtoDP(long ptr long) LPtoDP
248 stdcall LineDDA(long long long long ptr long) LineDDA
249 stdcall LineTo(long long long) LineTo
250 stub LoadImageColorMatcherA
251 stub LoadImageColorMatcherW
252 stdcall MaskBlt(long long long long long long long long long long long long) MaskBlt
253 stdcall ModifyWorldTransform(long ptr long) ModifyWorldTransform
254 stdcall MoveToEx(long long long ptr) MoveToEx
255 stdcall OffsetClipRgn(long long long) OffsetClipRgn
256 stdcall OffsetRgn(long long long) OffsetRgn
257 stdcall OffsetViewportOrgEx(long long long ptr) OffsetViewportOrgEx
258 stdcall OffsetWindowOrgEx(long long long ptr) OffsetWindowOrgEx
259 stdcall PaintRgn(long long) PaintRgn
260 stdcall PatBlt(long long long long long long) PatBlt
261 stdcall PathToRegion(long) PathToRegion
262 stdcall Pie(long long long long long long long long long) Pie
263 stdcall PlayEnhMetaFile(long long ptr) PlayEnhMetaFile
264 stdcall PlayEnhMetaFileRecord(long ptr ptr long) PlayEnhMetaFileRecord
265 stdcall PlayMetaFile(long long) PlayMetaFile
266 stdcall PlayMetaFileRecord(long ptr ptr long) PlayMetaFileRecord
267 stdcall PlgBlt(long ptr long long long long long long long long) PlgBlt
268 stdcall PolyBezier(long ptr long) PolyBezier
269 stdcall PolyBezierTo(long ptr long) PolyBezierTo
270 stdcall PolyDraw(long ptr ptr long) PolyDraw
271 stdcall PolyPolygon(long ptr ptr long) PolyPolygon
272 stdcall PolyPolyline(long ptr ptr long) PolyPolyline
273 stub PolyTextOutA
274 stub PolyTextOutW
275 stdcall Polygon(long ptr long) Polygon
276 stdcall Polyline(long ptr long) Polyline
277 stdcall PolylineTo(long ptr long) PolylineTo
278 stdcall PtInRegion(long long long) PtInRegion
279 stdcall PtVisible(long long long) PtVisible
280 stdcall RealizePalette(long) RealizePalette
281 stdcall RectInRegion(long ptr) RectInRegion
282 stdcall RectVisible(long ptr) RectVisible
283 stdcall Rectangle(long long long long long) Rectangle
284 stdcall RemoveFontResourceA(str) RemoveFontResourceA
285 stub RemoveFontResourceTracking
286 stdcall RemoveFontResourceW(wstr) RemoveFontResourceW
287 stdcall ResetDCA(long ptr) ResetDCA
288 stdcall ResetDCW(long ptr) ResetDCW
289 stdcall ResizePalette(long long) ResizePalette
290 stdcall RestoreDC(long long) RestoreDC
291 stdcall RoundRect(long long long long long long long) RoundRect
292 stdcall SaveDC(long) SaveDC
293 stdcall ScaleViewportExtEx(long long long long long ptr) ScaleViewportExtEx
294 stdcall ScaleWindowExtEx(long long long long long ptr) ScaleWindowExtEx
295 stub SelectBrushLocal
296 stdcall SelectClipPath(long long) SelectClipPath
297 stdcall SelectClipRgn(long long) SelectClipRgn
298 stub SelectFontLocal
299 stdcall SelectObject(long long) SelectObject
300 stdcall SelectPalette(long long long) SelectPalette
301 stdcall SetAbortProc(long ptr) SetAbortProc
302 stdcall SetArcDirection(long long) SetArcDirection
303 stdcall SetBitmapBits(long long ptr) SetBitmapBits
304 stdcall SetBitmapDimensionEx(long long long ptr) SetBitmapDimensionEx
305 stdcall SetBkColor(long long) SetBkColor
306 stdcall SetBkMode(long long) SetBkMode
307 stdcall SetBoundsRect(long ptr long) SetBoundsRect
308 stdcall SetBrushOrgEx(long long long ptr) SetBrushOrgEx
309 stdcall SetColorAdjustment(long ptr) SetColorAdjustment
310 stub SetColorSpace
311 stdcall SetDIBColorTable(long long long ptr) SetDIBColorTable
312 stdcall SetDIBits(long long long long ptr ptr long) SetDIBits
313 stdcall SetDIBitsToDevice(long long long long long long long long long ptr ptr long) SetDIBitsToDevice
314 stub SetDeviceGammaRamp
315 stdcall SetEnhMetaFileBits(long ptr) SetEnhMetaFileBits
316 stub SetFontEnumeration
317 stdcall SetGraphicsMode(long long) SetGraphicsMode
318 stdcall SetICMMode(long long) SetICMMode
319 stub SetICMProfileA
320 stub SetICMProfileW
321 stdcall SetMapMode(long long) SetMapMode
322 stdcall SetMapperFlags(long long) SetMapperFlags
323 stdcall SetMetaFileBitsEx(long ptr) SetMetaFileBitsEx
324 stub SetMetaRgn
325 stdcall SetMiterLimit(long long ptr) SetMiterLimit
326 stdcall SetPaletteEntries(long long long ptr) SetPaletteEntries
327 stdcall SetPixel(long long long long) SetPixel
328 stdcall SetPixelFormat(long long ptr) SetPixelFormat
329 stdcall SetPixelV(long long long long) SetPixelV
330 stdcall SetPolyFillMode(long long) SetPolyFillMode
331 stdcall SetROP2(long long) SetROP2
332 stdcall SetRectRgn(long long long long long) SetRectRgn
333 stdcall SetRelAbs(long long) SetRelAbs
334 stdcall SetStretchBltMode(long long) SetStretchBltMode
335 stdcall SetSystemPaletteUse(long long) SetSystemPaletteUse
336 stdcall SetTextAlign(long long) SetTextAlign
337 stdcall SetTextCharacterExtra(long long) SetTextCharacterExtra
338 stdcall SetTextColor(long long) SetTextColor
339 stdcall SetTextJustification(long long long) SetTextJustification
340 stdcall SetViewportExtEx(long long long ptr) SetViewportExtEx
341 stdcall SetViewportOrgEx(long long long ptr) SetViewportOrgEx
342 stub SetVirtualResolution
343 stdcall SetWinMetaFileBits(long ptr long ptr) SetWinMetaFileBits
344 stdcall SetWindowExtEx(long long long ptr) SetWindowExtEx
345 stdcall SetWindowOrgEx(long long long ptr) SetWindowOrgEx
346 stdcall SetWorldTransform(long ptr) SetWorldTransform
347 stdcall StartDocA(long ptr) StartDocA
348 stdcall StartDocW(long ptr) StartDocW
349 stdcall StartPage(long) StartPage 
350 stdcall StretchBlt(long long long long long long long long long long long) StretchBlt
351 stdcall StretchDIBits(long long long long long long long long long ptr ptr long long) StretchDIBits
352 stdcall StrokeAndFillPath(long) StrokeAndFillPath
353 stdcall StrokePath(long) StrokePath
354 stdcall SwapBuffers(long) SwapBuffers
355 stdcall TextOutA(long long long str long) TextOutA
356 stdcall TextOutW(long long long wstr long) TextOutW
357 stub UnloadNetworkFonts
358 stdcall UnrealizeObject(long) UnrealizeObject
359 stdcall UpdateColors(long) UpdateColors
360 stdcall WidenPath(long) WidenPath
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
382 stdcall TranslateCharsetInfo(ptr ptr long) TranslateCharsetInfo
383 stub UpdateICMRegKeyA
384 stub UpdateICMRegKeyW
385 stub gdiPlaySpoolStream
386 stdcall SetObjectOwner(long long) SetObjectOwner
387 stub UpdateICMRegKey
388 extern pfnRealizePalette pfnRealizePalette
389 extern pfnSelectPalette pfnSelectPalette
