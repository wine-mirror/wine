name    gdi32
type    win32
init    MAIN_GdiInit

100 stdcall GDI_CallDevInstall16(long long str str str) GDI_CallDevInstall16  # by ordinal!
101 stdcall GDI_CallExtDeviceModePropSheet16(long str str ptr) GDI_CallExtDeviceModePropSheet16  # by ordinal!
102 stdcall GDI_CallExtDeviceMode16(long ptr str str ptr str long) GDI_CallExtDeviceMode16  # by ordinal!
103 stdcall GDI_CallAdvancedSetupDialog16(long str ptr ptr) GDI_CallAdvancedSetupDialog16  # by ordinal!
104 stdcall GDI_CallDeviceCapabilities16(str str long ptr ptr) GDI_CallDeviceCapabilities16  # by ordinal!
105 stdcall AbortDoc(long) AbortDoc
106 stdcall AbortPath(long) AbortPath
107 stdcall AddFontResourceA(str) AddFontResourceA
108 stub AddFontResourceTracking
109 stdcall AddFontResourceW(wstr) AddFontResourceW
110 stdcall AngleArc(long long long long long long) AngleArc
111 stdcall AnimatePalette(long long long ptr) AnimatePalette
112 stdcall Arc(long long long long long long long long long) Arc
113 stdcall ArcTo(long long long long long long long long long) ArcTo
114 stdcall BeginPath(long) BeginPath
115 stdcall BitBlt(long long long long long long long long long) BitBlt
116 stub ByeByeGDI
117 stub CancelDC
118 stub CheckColorsInGamut
119 stdcall ChoosePixelFormat(long ptr) ChoosePixelFormat
120 stdcall Chord(long long long long long long long long long) Chord
121 stdcall CloseEnhMetaFile(long) CloseEnhMetaFile
122 stdcall CloseFigure(long) CloseFigure
123 stdcall CloseMetaFile(long) CloseMetaFile
124 stub ColorCorrectPalette
125 stub ColorMatchToTarget
126 stdcall CombineRgn(long long long long) CombineRgn
127 stdcall CombineTransform(ptr ptr ptr) CombineTransform
128 stdcall CopyEnhMetaFileA(long str) CopyEnhMetaFileA
129 stub CopyEnhMetaFileW
130 stdcall CopyMetaFileA(long str) CopyMetaFileA
131 stdcall CopyMetaFileW(long wstr) CopyMetaFileW
132 stdcall CreateBitmap(long long long long ptr) CreateBitmap
133 stdcall CreateBitmapIndirect(ptr) CreateBitmapIndirect
134 stdcall CreateBrushIndirect(ptr) CreateBrushIndirect
135 stub CreateColorSpaceA
136 stub CreateColorSpaceW
137 stdcall CreateCompatibleBitmap(long long long) CreateCompatibleBitmap
138 stdcall CreateCompatibleDC(long) CreateCompatibleDC
139 stdcall CreateDCA(str str str ptr) CreateDCA
140 stdcall CreateDCW(wstr wstr wstr ptr) CreateDCW
141 stdcall CreateDIBPatternBrush(long long) CreateDIBPatternBrush
142 stdcall CreateDIBPatternBrushPt(long long) CreateDIBPatternBrushPt
143 stdcall CreateDIBSection(long ptr long ptr long long) CreateDIBSection
144 stdcall CreateDIBitmap(long ptr long ptr ptr long) CreateDIBitmap
145 stdcall CreateDiscardableBitmap(long long long) CreateDiscardableBitmap
146 stdcall CreateEllipticRgn(long long long long) CreateEllipticRgn
147 stdcall CreateEllipticRgnIndirect(ptr) CreateEllipticRgnIndirect
148 stdcall CreateEnhMetaFileA(long str ptr str) CreateEnhMetaFileA
149 stdcall CreateEnhMetaFileW(long wstr ptr wstr) CreateEnhMetaFileW
150 stdcall CreateFontA(long long long long long long long long long long long long long str) CreateFontA
151 stdcall CreateFontIndirectA(ptr) CreateFontIndirectA
152 stdcall CreateFontIndirectW(ptr) CreateFontIndirectW
153 stdcall CreateFontW(long long long long long long long long long long long long long wstr) CreateFontW
154 stdcall CreateHalftonePalette(long) CreateHalftonePalette
155 stdcall CreateHatchBrush(long long) CreateHatchBrush
156 stdcall CreateICA(str str str ptr) CreateICA
157 stdcall CreateICW(wstr wstr wstr ptr) CreateICW
158 stdcall CreateMetaFileA(str) CreateMetaFileA
159 stdcall CreateMetaFileW(wstr) CreateMetaFileW
160 stdcall CreatePalette(ptr) CreatePalette
161 stdcall CreatePatternBrush(long) CreatePatternBrush
162 stdcall CreatePen(long long long) CreatePen
163 stdcall CreatePenIndirect(ptr) CreatePenIndirect
164 stdcall CreatePolyPolygonRgn(ptr ptr long long) CreatePolyPolygonRgn
165 stdcall CreatePolygonRgn(ptr long long) CreatePolygonRgn
166 stdcall CreateRectRgn(long long long long) CreateRectRgn
167 stdcall CreateRectRgnIndirect(ptr) CreateRectRgnIndirect
168 stdcall CreateRoundRectRgn(long long long long long long) CreateRoundRectRgn
169 stdcall CreateScalableFontResourceA(long str str str) CreateScalableFontResourceA
170 stdcall CreateScalableFontResourceW(long wstr wstr wstr) CreateScalableFontResourceW
171 stdcall CreateSolidBrush(long) CreateSolidBrush
172 stdcall DPtoLP(long ptr long) DPtoLP
173 stub DeleteColorSpace
174 stdcall DeleteDC(long) DeleteDC
175 stdcall DeleteEnhMetaFile(long) DeleteEnhMetaFile
176 stdcall DeleteMetaFile(long) DeleteMetaFile
177 stdcall DeleteObject(long)	DeleteObject
178 stdcall DescribePixelFormat(long long long ptr) DescribePixelFormat
179 stub DeviceCapabilitiesEx
180 stub DeviceCapabilitiesExA
181 stub DeviceCapabilitiesExW
182 stdcall DrawEscape(long long long ptr) DrawEscape
183 stdcall Ellipse(long long long long long) Ellipse
184 stub EnableEUDC
185 stdcall EndDoc(long) EndDoc
186 stdcall EndPage(long) EndPage
187 stdcall EndPath(long) EndPath
188 stdcall EnumEnhMetaFile(long long ptr ptr ptr) EnumEnhMetaFile
189 stdcall EnumFontFamiliesA(long str ptr long) EnumFontFamiliesA
190 stdcall EnumFontFamiliesExA(long str ptr long long) EnumFontFamiliesExA
191 stdcall EnumFontFamiliesExW(long wstr ptr long long) EnumFontFamiliesExW
192 stdcall EnumFontFamiliesW(long wstr ptr long) EnumFontFamiliesW
193 stdcall EnumFontsA(long str ptr long) EnumFontsA
194 stdcall EnumFontsW(long wstr ptr long) EnumFontsW
195 stub EnumICMProfilesA
196 stub EnumICMProfilesW
197 stdcall EnumMetaFile(long long ptr ptr) EnumMetaFile
198 stdcall EnumObjects(long long ptr long) EnumObjects
199 stdcall EqualRgn(long long) EqualRgn
200 stdcall Escape(long long long ptr ptr) Escape
201 stdcall ExcludeClipRect(long long long long long) ExcludeClipRect
202 stdcall ExtCreatePen(long long ptr long ptr) ExtCreatePen
203 stdcall ExtCreateRegion(ptr long ptr) ExtCreateRegion
204 stdcall ExtEscape(long long long ptr long ptr) ExtEscape
205 stdcall ExtFloodFill(long long long long long) ExtFloodFill
206 stdcall ExtSelectClipRgn(long long long) ExtSelectClipRgn
207 stdcall ExtTextOutA(long long long long ptr str long ptr) ExtTextOutA
208 stdcall ExtTextOutW(long long long long ptr wstr long ptr) ExtTextOutW
209 stdcall FillPath(long) FillPath
210 stdcall FillRgn(long long long) FillRgn
211 stdcall FixBrushOrgEx(long long long ptr) FixBrushOrgEx
212 stdcall FlattenPath(long) FlattenPath
213 stdcall FloodFill(long long long long) FloodFill
214 stdcall FrameRgn(long long long long long) FrameRgn
215 stub FreeImageColorMatcher
216 stub GdiAssociateObject
217 stub GdiCleanCacheDC
218 stdcall GdiComment(long long ptr) GdiComment
219 stub GdiConvertAndCheckDC
220 stub GdiConvertBitmap
221 stub GdiConvertBrush
222 stub GdiConvertDC
223 stub GdiConvertEnhMetaFile
224 stub GdiConvertFont
225 stub GdiConvertMetaFilePict
226 stub GdiConvertPalette
227 stub GdiConvertRegion
228 stub GdiCreateLocalBitmap
229 stub GdiCreateLocalBrush
230 stub GdiCreateLocalEnhMetaFile
231 stub GdiCreateLocalFont
232 stub GdiCreateLocalMetaFilePict
233 stub GdiCreateLocalPalette
234 stub GdiCreateLocalRegion
235 stub GdiDciBeginAccess
236 stub GdiDciCreateOffscreenSurface
237 stub GdiDciCreateOverlaySurface
238 stub GdiDciCreatePrimarySurface
239 stub GdiDciDestroySurface
240 stub GdiDciDrawSurface
241 stub GdiDciEndAccess
242 stub GdiDciEnumSurface
243 stub GdiDciInitialize
244 stub GdiDciSetClipList
245 stub GdiDciSetDestination
246 stub GdiDeleteLocalDC
247 stub GdiDeleteLocalObject
248 stub GdiDllInitialize
249 stdcall GdiFlush() GdiFlush
250 stdcall GdiGetBatchLimit() GdiGetBatchLimit
251 stub GdiGetLocalBitmap
252 stub GdiGetLocalBrush
253 stub GdiGetLocalDC
254 stub GdiGetLocalFont
255 stub GdiIsMetaFileDC
256 stub GdiPlayDCScript
257 stub GdiPlayJournal
258 stub GdiPlayScript
259 stub GdiReleaseLocalDC
260 stub GdiSetAttrs
261 stdcall GdiSetBatchLimit(long) GdiSetBatchLimit
262 stub GdiSetServerAttr
263 stub GdiWinWatchClose
264 stub GdiWinWatchDidStatusChange
265 stub GdiWinWatchGetClipList
266 stub GdiWinWatchOpen
267 stdcall GetArcDirection(long) GetArcDirection
268 stdcall GetAspectRatioFilterEx(long ptr) GetAspectRatioFilterEx
269 stdcall GetBitmapBits(long long ptr) GetBitmapBits
270 stdcall GetBitmapDimensionEx(long ptr) GetBitmapDimensionEx
271 stdcall GetBkColor(long) GetBkColor
272 stdcall GetBkMode(long) GetBkMode
273 stdcall GetBoundsRect(long ptr long) GetBoundsRect
274 stdcall GetBrushOrgEx(long ptr) GetBrushOrgEx
275 stdcall GetCharABCWidthsA(long long long ptr) GetCharABCWidthsA
276 stdcall GetCharABCWidthsFloatA(long long long ptr) GetCharABCWidthsFloatA
277 stdcall GetCharABCWidthsFloatW(long long long ptr) GetCharABCWidthsFloatW
278 stdcall GetCharABCWidthsW(long long long ptr) GetCharABCWidthsW
279 stdcall GetCharWidth32A(long long long long) GetCharWidth32A
280 stdcall GetCharWidth32W(long long long long) GetCharWidth32W
281 stdcall GetCharWidthA(long long long long) GetCharWidth32A
282 stdcall GetCharWidthFloatA(long long long ptr) GetCharWidthFloatA
283 stdcall GetCharWidthFloatW(long long long ptr) GetCharWidthFloatW
284 stdcall GetCharWidthW(long long long long) GetCharWidth32W
285 stub GetCharWidthWOW
286 stdcall GetCharacterPlacementA(long str long long ptr long) GetCharacterPlacementA
287 stdcall GetCharacterPlacementW(long wstr long long ptr long) GetCharacterPlacementW
288 stdcall GetClipBox(long ptr) GetClipBox
289 stdcall GetClipRgn(long long) GetClipRgn
290 stdcall GetColorAdjustment(long ptr) GetColorAdjustment
291 stdcall GetColorSpace(long) GetColorSpace
292 stdcall GetCurrentObject(long long) GetCurrentObject
293 stdcall GetCurrentPositionEx(long ptr) GetCurrentPositionEx
294 stdcall GetDCOrgEx(long ptr) GetDCOrgEx
295 stdcall GetDIBColorTable(long long long ptr) GetDIBColorTable
296 stdcall GetDIBits(long long long long ptr ptr long) GetDIBits
297 stdcall GetDeviceCaps(long long) GetDeviceCaps
298 stub GetDeviceGammaRamp
299 stub GetETM
300 stdcall GetEnhMetaFileA(str) GetEnhMetaFileA
301 stdcall GetEnhMetaFileBits(long long ptr) GetEnhMetaFileBits
302 stdcall GetEnhMetaFileDescriptionA(long long ptr) GetEnhMetaFileDescriptionA
303 stdcall GetEnhMetaFileDescriptionW(long long ptr) GetEnhMetaFileDescriptionW
304 stdcall GetEnhMetaFileHeader(long long ptr) GetEnhMetaFileHeader
305 stdcall GetEnhMetaFilePaletteEntries (long long ptr) GetEnhMetaFilePaletteEntries
306 stdcall GetEnhMetaFileW(wstr) GetEnhMetaFileW
307 stdcall GetFontData(long long long ptr long) GetFontData
308 stdcall GetFontLanguageInfo(long) GetFontLanguageInfo
309 stub GetFontResourceInfo
310 stub GetFontResourceInfoW
311 stub GetGlyphOutline
312 stdcall GetGlyphOutlineA(long long long ptr long ptr ptr) GetGlyphOutlineA
313 stdcall GetGlyphOutlineW(long long long ptr long ptr ptr) GetGlyphOutlineW
314 stub GetGlyphOutlineWow
315 stdcall GetGraphicsMode(long) GetGraphicsMode
316 stub GetICMProfileA
317 stub GetICMProfileW
318 stub GetKerningPairs
319 stdcall GetKerningPairsA(long long ptr) GetKerningPairsA
320 stdcall GetKerningPairsW(long long ptr) GetKerningPairsW
321 stub GetLayout
322 stub GetLogColorSpaceA
323 stub GetLogColorSpaceW
324 stdcall GetMapMode(long) GetMapMode
325 stdcall GetMetaFileA(str) GetMetaFileA
326 stdcall GetMetaFileBitsEx(long long ptr) GetMetaFileBitsEx
327 stdcall GetMetaFileW(wstr) GetMetaFileW
328 stub GetMetaRgn
329 stdcall GetMiterLimit(long ptr) GetMiterLimit
330 stdcall GetNearestColor(long long) GetNearestColor
331 stdcall GetNearestPaletteIndex(long long) GetNearestPaletteIndex
332 stdcall GetObjectA(long long ptr) GetObjectA
333 stdcall GetObjectType(long) GetObjectType
334 stdcall GetObjectW(long long ptr) GetObjectW
335 stdcall GetOutlineTextMetricsA(long long ptr) GetOutlineTextMetricsA
336 stdcall GetOutlineTextMetricsW(long long ptr) GetOutlineTextMetricsW
337 stdcall GetPaletteEntries(long long long ptr) GetPaletteEntries
338 stdcall GetPath(long ptr ptr long) GetPath
339 stdcall GetPixel(long long long) GetPixel
340 stdcall GetPixelFormat(long) GetPixelFormat
341 stdcall GetPolyFillMode(long) GetPolyFillMode
342 stdcall GetROP2(long) GetROP2
343 stdcall GetRandomRgn(long long long) GetRandomRgn
344 stdcall GetRasterizerCaps(ptr long) GetRasterizerCaps
345 stdcall GetRegionData(long long ptr) GetRegionData
346 stdcall GetRelAbs(long) GetRelAbs
347 stdcall GetRgnBox(long ptr) GetRgnBox
348 stdcall GetStockObject(long) GetStockObject
349 stdcall GetStretchBltMode(long) GetStretchBltMode
350 stdcall GetSystemPaletteEntries(long long long ptr) GetSystemPaletteEntries
351 stdcall GetSystemPaletteUse(long) GetSystemPaletteUse
352 stdcall GetTextAlign(long) GetTextAlign
353 stdcall GetTextCharacterExtra(long) GetTextCharacterExtra
354 stdcall GetTextCharset(long) GetTextCharset
355 stdcall GetTextCharsetInfo(long ptr long) GetTextCharsetInfo
356 stdcall GetTextColor(long) GetTextColor
357 stdcall GetTextExtentExPointA(long str long long ptr ptr ptr) GetTextExtentExPointA
358 stdcall GetTextExtentExPointW(long wstr long long ptr ptr ptr) GetTextExtentExPointW
359 stdcall GetTextExtentPoint32A(long ptr long ptr) GetTextExtentPoint32A
360 stdcall GetTextExtentPoint32W(long ptr long ptr) GetTextExtentPoint32W
361 stdcall GetTextExtentPointA(long ptr long ptr) GetTextExtentPointA
362 stdcall GetTextExtentPointW(long ptr long ptr) GetTextExtentPointW
363 stdcall GetTextFaceA(long long ptr) GetTextFaceA
364 stdcall GetTextFaceW(long long ptr) GetTextFaceW
365 stdcall GetTextMetricsA(long ptr) GetTextMetricsA
366 stdcall GetTextMetricsW(long ptr) GetTextMetricsW
367 stub GetTransform
368 stdcall GetViewportExtEx(long ptr) GetViewportExtEx
369 stdcall GetViewportOrgEx(long ptr) GetViewportOrgEx
370 stdcall GetWinMetaFileBits(long long ptr long long) GetWinMetaFileBits
371 stdcall GetWindowExtEx(long ptr) GetWindowExtEx
372 stdcall GetWindowOrgEx(long ptr) GetWindowOrgEx
373 stdcall GetWorldTransform(long ptr) GetWorldTransform
374 stdcall IntersectClipRect(long long long long long) IntersectClipRect
375 stdcall InvertRgn(long long) InvertRgn
376 stdcall LPtoDP(long ptr long) LPtoDP
377 stdcall LineDDA(long long long long ptr long) LineDDA
378 stdcall LineTo(long long long) LineTo
379 stub LoadImageColorMatcherA
380 stub LoadImageColorMatcherW
381 stdcall MaskBlt(long long long long long long long long long long long long) MaskBlt
382 stdcall ModifyWorldTransform(long ptr long) ModifyWorldTransform
383 stdcall MoveToEx(long long long ptr) MoveToEx
384 stdcall OffsetClipRgn(long long long) OffsetClipRgn
385 stdcall OffsetRgn(long long long) OffsetRgn
386 stdcall OffsetViewportOrgEx(long long long ptr) OffsetViewportOrgEx
387 stdcall OffsetWindowOrgEx(long long long ptr) OffsetWindowOrgEx
388 stdcall PaintRgn(long long) PaintRgn
389 stdcall PatBlt(long long long long long long) PatBlt
390 stdcall PathToRegion(long) PathToRegion
391 stdcall Pie(long long long long long long long long long) Pie
392 stdcall PlayEnhMetaFile(long long ptr) PlayEnhMetaFile
393 stdcall PlayEnhMetaFileRecord(long ptr ptr long) PlayEnhMetaFileRecord
394 stdcall PlayMetaFile(long long) PlayMetaFile
395 stdcall PlayMetaFileRecord(long ptr ptr long) PlayMetaFileRecord
396 stdcall PlgBlt(long ptr long long long long long long long long) PlgBlt
397 stdcall PolyBezier(long ptr long) PolyBezier
398 stdcall PolyBezierTo(long ptr long) PolyBezierTo
399 stdcall PolyDraw(long ptr ptr long) PolyDraw
400 stdcall PolyPolygon(long ptr ptr long) PolyPolygon
401 stdcall PolyPolyline(long ptr ptr long) PolyPolyline
402 stub PolyTextOutA
403 stub PolyTextOutW
404 stdcall Polygon(long ptr long) Polygon
405 stdcall Polyline(long ptr long) Polyline
406 stdcall PolylineTo(long ptr long) PolylineTo
407 stdcall PtInRegion(long long long) PtInRegion
408 stdcall PtVisible(long long long) PtVisible
409 stdcall RealizePalette(long) RealizePalette
410 stdcall RectInRegion(long ptr) RectInRegion
411 stdcall RectVisible(long ptr) RectVisible
412 stdcall Rectangle(long long long long long) Rectangle
413 stdcall RemoveFontResourceA(str) RemoveFontResourceA
414 stub RemoveFontResourceTracking
415 stdcall RemoveFontResourceW(wstr) RemoveFontResourceW
416 stdcall ResetDCA(long ptr) ResetDCA
417 stdcall ResetDCW(long ptr) ResetDCW
418 stdcall ResizePalette(long long) ResizePalette
419 stdcall RestoreDC(long long) RestoreDC
420 stdcall RoundRect(long long long long long long long) RoundRect
421 stdcall SaveDC(long) SaveDC
422 stdcall ScaleViewportExtEx(long long long long long ptr) ScaleViewportExtEx
423 stdcall ScaleWindowExtEx(long long long long long ptr) ScaleWindowExtEx
424 stub SelectBrushLocal
425 stdcall SelectClipPath(long long) SelectClipPath
426 stdcall SelectClipRgn(long long) SelectClipRgn
427 stub SelectFontLocal
428 stdcall SelectObject(long long) SelectObject
429 stdcall SelectPalette(long long long) SelectPalette
430 stdcall SetAbortProc(long ptr) SetAbortProc
431 stdcall SetArcDirection(long long) SetArcDirection
432 stdcall SetBitmapBits(long long ptr) SetBitmapBits
433 stdcall SetBitmapDimensionEx(long long long ptr) SetBitmapDimensionEx
434 stdcall SetBkColor(long long) SetBkColor
435 stdcall SetBkMode(long long) SetBkMode
436 stdcall SetBoundsRect(long ptr long) SetBoundsRect
437 stdcall SetBrushOrgEx(long long long ptr) SetBrushOrgEx
438 stdcall SetColorAdjustment(long ptr) SetColorAdjustment
439 stub SetColorSpace
440 stdcall SetDIBColorTable(long long long ptr) SetDIBColorTable
441 stdcall SetDIBits(long long long long ptr ptr long) SetDIBits
442 stdcall SetDIBitsToDevice(long long long long long long long long long ptr ptr long) SetDIBitsToDevice
443 stub SetDeviceGammaRamp
444 stdcall SetEnhMetaFileBits(long ptr) SetEnhMetaFileBits
445 stub SetFontEnumeration
446 stdcall SetGraphicsMode(long long) SetGraphicsMode
447 stdcall SetICMMode(long long) SetICMMode
448 stub SetICMProfileA
449 stub SetICMProfileW
450 stub SetLayout
451 stub SetMagicColors
452 stdcall SetMapMode(long long) SetMapMode
453 stdcall SetMapperFlags(long long) SetMapperFlags
454 stdcall SetMetaFileBitsEx(long ptr) SetMetaFileBitsEx
455 stub SetMetaRgn
456 stdcall SetMiterLimit(long long ptr) SetMiterLimit
457 stdcall SetObjectOwner(long long) SetObjectOwner
458 stdcall SetPaletteEntries(long long long ptr) SetPaletteEntries
459 stdcall SetPixel(long long long long) SetPixel
460 stdcall SetPixelFormat(long long ptr) SetPixelFormat
461 stdcall SetPixelV(long long long long) SetPixelV
462 stdcall SetPolyFillMode(long long) SetPolyFillMode
463 stdcall SetROP2(long long) SetROP2
464 stdcall SetRectRgn(long long long long long) SetRectRgn
465 stdcall SetRelAbs(long long) SetRelAbs
466 stdcall SetStretchBltMode(long long) SetStretchBltMode
467 stdcall SetSystemPaletteUse(long long) SetSystemPaletteUse
468 stdcall SetTextAlign(long long) SetTextAlign
469 stdcall SetTextCharacterExtra(long long) SetTextCharacterExtra
470 stdcall SetTextColor(long long) SetTextColor
471 stdcall SetTextJustification(long long long) SetTextJustification
472 stdcall SetViewportExtEx(long long long ptr) SetViewportExtEx
473 stdcall SetViewportOrgEx(long long long ptr) SetViewportOrgEx
474 stub SetVirtualResolution
475 stdcall SetWinMetaFileBits(long ptr long ptr) SetWinMetaFileBits
476 stdcall SetWindowExtEx(long long long ptr) SetWindowExtEx
477 stdcall SetWindowOrgEx(long long long ptr) SetWindowOrgEx
478 stdcall SetWorldTransform(long ptr) SetWorldTransform
479 stdcall StartDocA(long ptr) StartDocA
480 stdcall StartDocW(long ptr) StartDocW
481 stdcall StartPage(long) StartPage 
482 stdcall StretchBlt(long long long long long long long long long long long) StretchBlt
483 stdcall StretchDIBits(long long long long long long long long long ptr ptr long long) StretchDIBits
484 stdcall StrokeAndFillPath(long) StrokeAndFillPath
485 stdcall StrokePath(long) StrokePath
486 stdcall SwapBuffers(long) SwapBuffers
487 stdcall TextOutA(long long long str long) TextOutA
488 stdcall TextOutW(long long long wstr long) TextOutW
489 stdcall TranslateCharsetInfo(ptr ptr long) TranslateCharsetInfo
490 stub UnloadNetworkFonts
491 stdcall UnrealizeObject(long) UnrealizeObject
492 stdcall UpdateColors(long) UpdateColors
493 stub UpdateICMRegKey
494 stub UpdateICMRegKeyA
495 stub UpdateICMRegKeyW
496 stdcall WidenPath(long) WidenPath
497 stub gdiPlaySpoolStream
498 extern pfnRealizePalette pfnRealizePalette
499 extern pfnSelectPalette pfnSelectPalette
500 stub pstackConnect
