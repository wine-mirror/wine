name	gdi
type	win16
heap	65488  # 65536 - 16 (instance data) - 32 (stock objects)

1   pascal SetBkColor(word long) SetBkColor
2   pascal16 SetBkMode(word word) SetBkMode
3   pascal16 SetMapMode(word word) SetMapMode
4   pascal16 SetROP2(word word) SetROP2
5   pascal16 SetRelAbs(word word) SetRelAbs
6   pascal16 SetPolyFillMode(word word) SetPolyFillMode
7   pascal16 SetStretchBltMode(word word) SetStretchBltMode
8   pascal16 SetTextCharacterExtra(word s_word) SetTextCharacterExtra
9   pascal SetTextColor(word long) SetTextColor
10  pascal16 SetTextJustification(word s_word s_word) SetTextJustification
11  pascal SetWindowOrg(word s_word s_word) SetWindowOrg
12  pascal SetWindowExt(word s_word s_word) SetWindowExt
13  pascal SetViewportOrg(word s_word s_word) SetViewportOrg
14  pascal SetViewportExt(word s_word s_word) SetViewportExt
15  pascal OffsetWindowOrg(word s_word s_word) OffsetWindowOrg
16  pascal ScaleWindowExt(word s_word s_word s_word s_word) ScaleWindowExt
17  pascal OffsetViewportOrg(word s_word s_word) OffsetViewportOrg
18  pascal ScaleViewportExt(word s_word s_word s_word s_word) ScaleViewportExt
19  pascal16 LineTo(word s_word s_word) LineTo
20  pascal   MoveTo(word s_word s_word) MoveTo
21  pascal16 ExcludeClipRect(word s_word s_word s_word s_word) ExcludeClipRect
22  pascal16 IntersectClipRect(word s_word s_word s_word s_word)
             IntersectClipRect
23  pascal16 Arc(word s_word s_word s_word s_word s_word s_word s_word s_word)
             Arc
24  pascal16 Ellipse(word s_word s_word s_word s_word) Ellipse
25  pascal16 FloodFill(word s_word s_word long) FloodFill
26  pascal16 Pie(word s_word s_word s_word s_word s_word s_word s_word s_word)
             Pie
27  pascal16 Rectangle(word s_word s_word s_word s_word) Rectangle
28  pascal16 RoundRect(word s_word s_word s_word s_word s_word s_word)
             RoundRect
29  pascal16 PatBlt(word s_word s_word s_word s_word long) PatBlt
30  pascal16 SaveDC(word) SaveDC
31  pascal   SetPixel(word s_word s_word long) SetPixel
32  pascal16 OffsetClipRgn(word s_word s_word) OffsetClipRgn
33  pascal16 TextOut(word s_word s_word ptr word) TextOut16
34  pascal16 BitBlt( word s_word s_word s_word s_word word s_word s_word long)
             BitBlt
35  pascal16 StretchBlt(word s_word s_word s_word s_word word s_word s_word
                        s_word s_word long) StretchBlt
36  pascal16 Polygon (word ptr word) Polygon16
37  pascal16 Polyline (word ptr word) Polyline16
38  pascal Escape(word word word segptr segptr) Escape
39  pascal16 RestoreDC(word s_word) RestoreDC
40  pascal16 FillRgn(word word word) FillRgn
41  pascal16 FrameRgn(word word word word word) FrameRgn
42  pascal16 InvertRgn(word word) InvertRgn
43  pascal16 PaintRgn(word word) PaintRgn
44  pascal16 SelectClipRgn(word word) SelectClipRgn
45  pascal16 SelectObject(word word) SelectObject
#46  pascal __GP?
47  pascal16 CombineRgn(word word word s_word) CombineRgn
48  pascal16 CreateBitmap(word word word word ptr) CreateBitmap
49  pascal16 CreateBitmapIndirect(ptr) CreateBitmapIndirect16
50  pascal16 CreateBrushIndirect(ptr) CreateBrushIndirect
51  pascal16 CreateCompatibleBitmap(word word word) CreateCompatibleBitmap
52  pascal16 CreateCompatibleDC(word) CreateCompatibleDC
53  pascal16 CreateDC(ptr ptr ptr ptr) CreateDC
54  pascal16 CreateEllipticRgn(s_word s_word s_word s_word) CreateEllipticRgn
55  pascal16 CreateEllipticRgnIndirect(ptr) CreateEllipticRgnIndirect16
56  pascal16 CreateFont(s_word s_word s_word s_word s_word word word word
                        word word word word word ptr) CreateFont16
57  pascal16 CreateFontIndirect(ptr) CreateFontIndirect16
58  pascal16 CreateHatchBrush(word long) CreateHatchBrush
60  pascal16 CreatePatternBrush(word) CreatePatternBrush
61  pascal16 CreatePen(s_word s_word long) CreatePen
62  pascal16 CreatePenIndirect(ptr) CreatePenIndirect
63  pascal16 CreatePolygonRgn(ptr word word) CreatePolygonRgn16
64  pascal16 CreateRectRgn(s_word s_word s_word s_word) CreateRectRgn
65  pascal16 CreateRectRgnIndirect(ptr) CreateRectRgnIndirect16
66  pascal16 CreateSolidBrush(long) CreateSolidBrush
67  pascal16 DPtoLP(word ptr s_word) DPtoLP16
68  pascal16 DeleteDC(word) DeleteDC
69  pascal16 DeleteObject(word) DeleteObject
70  pascal16 EnumFonts(word ptr segptr long) THUNK_EnumFonts16
71  pascal16 EnumObjects(word word segptr long) THUNK_EnumObjects16
72  pascal16 EqualRgn(word word) EqualRgn
73  pascal16 ExcludeVisRect(word s_word s_word s_word s_word) ExcludeVisRect
74  pascal GetBitmapBits(word long ptr) GetBitmapBits
75  pascal GetBkColor(word) GetBkColor
76  pascal16 GetBkMode(word) GetBkMode
77  pascal16 GetClipBox(word ptr) GetClipBox16
78  pascal GetCurrentPosition(word) GetCurrentPosition
79  pascal GetDCOrg(word) GetDCOrg
80  pascal16 GetDeviceCaps(word s_word) GetDeviceCaps
81  pascal16 GetMapMode(word) GetMapMode
82  pascal16 GetObject(word s_word ptr) GetObject16
83  pascal GetPixel(word s_word s_word) GetPixel
84  pascal16 GetPolyFillMode(word) GetPolyFillMode
85  pascal16 GetROP2(word) GetROP2
86  pascal16 GetRelAbs(word) GetRelAbs
87  pascal16 GetStockObject(word) GetStockObject
88  pascal16 GetStretchBltMode(word) GetStretchBltMode
89  pascal16 GetTextCharacterExtra(word) GetTextCharacterExtra
90  pascal GetTextColor(word) GetTextColor
91  pascal GetTextExtent(word ptr s_word) GetTextExtent
92  pascal16 GetTextFace(word s_word ptr) GetTextFace
93  pascal16 GetTextMetrics(word ptr) GetTextMetrics16
94  pascal GetViewportExt(word) GetViewportExt
95  pascal GetViewportOrg(word) GetViewportOrg
96  pascal GetWindowExt(word) GetWindowExt
97  pascal GetWindowOrg(word) GetWindowOrg
98  pascal16 IntersectVisRect(word s_word s_word s_word s_word)
             IntersectVisRect
99  pascal16 LPtoDP(word ptr s_word) LPtoDP16
100 pascal16 LineDDA(s_word s_word s_word s_word segptr long) THUNK_LineDDA16
101 pascal16 OffsetRgn(word s_word s_word) OffsetRgn
102 pascal16 OffsetVisRgn(word s_word s_word) OffsetVisRgn
103 pascal16 PtVisible(word s_word s_word) PtVisible
104 pascal16 RectVisibleOld(word ptr) RectVisible16
105 pascal16 SelectVisRgn(word word) SelectVisRgn
106 pascal SetBitmapBits(word long ptr) SetBitmapBits
117 pascal SetDCOrg(word s_word s_word) SetDCOrg
119 pascal16 AddFontResource(ptr) AddFontResource
#121 pascal Death
#122 pascal ReSurRection
123 pascal16 PlayMetaFile(word word) PlayMetaFile
124 pascal16 GetMetaFile(ptr) GetMetaFile
125 pascal16 CreateMetaFile(ptr) CreateMetaFile16
126 pascal16 CloseMetaFile(word) CloseMetaFile16
127 pascal16 DeleteMetaFile(word) DeleteMetaFile16
128 pascal16 MulDiv(s_word s_word s_word) MulDiv16
129 pascal16 SaveVisRgn(word) SaveVisRgn
130 pascal16 RestoreVisRgn(word) RestoreVisRgn
131 pascal16 InquireVisRgn(word) InquireVisRgn
132 pascal16 SetEnvironment(ptr ptr word) SetEnvironment
133 pascal16 GetEnvironment(ptr ptr word) GetEnvironment
134 pascal16 GetRgnBox(word ptr) GetRgnBox16
#135 pascal ScanLr
136 pascal16 RemoveFontResource(ptr) RemoveFontResource
148 pascal SetBrushOrg(word s_word s_word) SetBrushOrg
149 pascal GetBrushOrg(word) GetBrushOrg
150 pascal16 UnrealizeObject(word) UnrealizeObject
151 pascal16 CopyMetaFile(word ptr) CopyMetaFile
153 pascal16 CreateIC(ptr ptr ptr ptr) CreateIC
154 pascal GetNearestColor(word long) GetNearestColor
155 stub QueryAbort
156 pascal16 CreateDiscardableBitmap(word word word) CreateDiscardableBitmap
158 pascal16 EnumCallback(ptr ptr word long) WineEnumDFontCallback
159 pascal16 GetMetaFileBits(word) GetMetaFileBits
160 pascal16 SetMetaFileBits(word) SetMetaFileBits
161 pascal16 PtInRegion(word s_word s_word) PtInRegion
162 pascal   GetBitmapDimension(word) GetBitmapDimension
163 pascal   SetBitmapDimension(word s_word s_word) SetBitmapDimension
169 stub IsDCDirty
170 stub SetDCStatus
172 pascal16 SetRectRgn(word s_word s_word s_word s_word) SetRectRgn
173 pascal16 GetClipRgn(word) GetClipRgn
175 pascal16 EnumMetaFile(word word segptr long) THUNK_EnumMetaFile16
176 pascal16 PlayMetaFileRecord(word ptr ptr word) PlayMetaFileRecord
179 pascal16 GetDCState(word) GetDCState
180 pascal16 SetDCState(word word) SetDCState
181 pascal16 RectInRegionOld(word ptr) RectInRegion16
188 stub GetTextExtentEx
190 pascal16 SetDCHook(word segptr long) THUNK_SetDCHook
191 pascal   GetDCHook(word ptr) THUNK_GetDCHook
192 pascal16 SetHookFlags(word word) SetHookFlags
193 stub SetBoundsRect
194 stub GetBoundsRect
195 stub SelectBitmap
196 stub SetMetaFileBitsBetter
201 stub DMBITBLT
202 stub DMCOLORINFO
206 stub DMENUMDFONTS
207 stub DMENUMOBJ
208 stub DMOUTPUT
209 stub DMPIXEL
210 stub DMREALIZEOBJECT
211 stub DMSTRBLT
212 stub DMSCANLR
213 stub BRUTE
214 stub DMEXTTEXTOUT
215 stub DMGETCHARWIDTH
216 stub DMSTRETCHBLT
217 stub DMDIBBITS
218 stub DMSTRETCHDIBITS
219 stub DMSETDIBTODEV
220 stub DMTRANSPOSE
230 pascal16 CreatePQ(word) CreatePQ
231 pascal16 MinPQ(word) MinPQ
232 pascal16 ExtractPQ(word) ExtractPQ
233 pascal16 InsertPQ(word word word) InsertPQ
234 pascal16 SizePQ(word word) SizePQ
235 pascal16 DeletePQ(word) DeletePQ
240 pascal16 OpenJob(ptr ptr word) OpenJob
241 pascal16 WriteSpool(word ptr word) WriteSpool
242 pascal16 WriteDialog(word ptr word) WriteDialog
243 pascal16 CloseJob(word) CloseJob
244 pascal16 DeleteJob(word word) DeleteJob
245 pascal GetSpoolJob(word ptr) GetSpoolJob
246 pascal16 StartSpoolPage(word) StartSpoolPage
247 pascal16 EndSpoolPage(word) EndSpoolPage
248 stub QueryJob
250 pascal16 Copy(ptr ptr word) Copy
253 stub DeleteSpoolPage
254 stub SpoolFile
267 stub StartDocPrintEra
268 stub StartPagePrinter
269 stub WritePrinter
270 stub EndPagePrinter
271 stub AbortPrinter
272 stub EndDocPrinter
274 stub ClosePrinter
280 stub GetRealDriverInfo
281 pascal DrvSetPrinterData(ptr ptr ptr ptr long) DrvSetPrinterData
282 pascal DrvGetPrinterData(ptr ptr ptr ptr long ptr) DrvGetPrinterData
299 stub ENGINEGETCHARWIDTHEX
300 stub ENGINEENUMERATEFONT
301 stub ENGINEDELETEFONT
302 stub ENGINEREALIZEFONT
303 stub ENGINEGETCHARWIDTH
304 stub ENGINESETFONTCONTEXT
305 stub ENGINEGETGLYPHBMP
306 stub ENGINEMAKEFONTDIR
307 pascal16 GetCharABCWidths(word word word ptr) GetCharABCWidths
308 stub GetOutLineTextMetrics
309 pascal   GetGlyphOutLine(word word word ptr long ptr ptr) GetGlyphOutLine
310 pascal16 CreateScalableFontResource(word ptr ptr ptr) CreateScalableFontResource
311 stub GetFontData
312 stub ConvertOutLineFontFile
313 pascal16 GetRasterizerCaps(ptr word) GetRasterizerCaps
314 stub EngineExtTextOut
315 stub EngineRealizeFontExt
316 stub EngineGetCharWidthStr
317 stub EngineGetGlyphBmpExt
330 pascal16 EnumFontFamilies(word ptr segptr long) THUNK_EnumFontFamilies16
332 pascal16 GetKerningPairs(word word ptr) GetKerningPairs
345 pascal16 GetTextAlign(word) GetTextAlign
346 pascal16 SetTextAlign(word word) SetTextAlign
348 pascal16 Chord(word s_word s_word s_word s_word s_word s_word
                   s_word s_word) Chord
349 pascal SetMapperFlags(word long) SetMapperFlags
350 pascal16 GetCharWidth(word word word ptr) GetCharWidth
351 pascal16 ExtTextOut(word s_word s_word word ptr ptr word ptr) ExtTextOut16
352 stub GetPhysicalFontHandle
353 stub GetAspectRatioFilter
354 stub ShrinkGDIHeap
355 stub FTrapping0
360 pascal16 CreatePalette(ptr) CreatePalette
361 pascal16 GDISelectPalette(word word word) GDISelectPalette
362 pascal16 GDIRealizePalette(word) GDIRealizePalette
363 pascal16 GetPaletteEntries(word word word ptr) GetPaletteEntries
364 pascal16 SetPaletteEntries(word word word ptr) SetPaletteEntries
365 pascal16 RealizeDefaultPalette(word) RealizeDefaultPalette
366 pascal16 UpdateColors(word) UpdateColors
367 pascal16 AnimatePalette(word word word ptr) AnimatePalette
368 pascal16 ResizePalette(word word) ResizePalette
370 pascal16 GetNearestPaletteIndex(word long) GetNearestPaletteIndex
372 pascal16 ExtFloodFill(word s_word s_word long word) ExtFloodFill
373 pascal16 SetSystemPaletteUse(word word) SetSystemPaletteUse
374 pascal16 GetSystemPaletteUse(word) GetSystemPaletteUse
375 pascal16 GetSystemPaletteEntries(word word word ptr)
             GetSystemPaletteEntries
376 pascal16 ResetDC(word ptr) ResetDC
377 stub STARTDOC
378 stub ENDDOC
379 stub STARTPAGE
380 stub ENDPAGE
381 stub SETABORTPROC
382 stub ABORTDOC
400 pascal16 FastWindowFrame(word long word word long) FastWindowFrame
401 stub GDIMOVEBITMAP
403 stub GDIINIT2
404 stub GetTTGlyphIndexMap
405 stub FINALGDIINIT
407 stub CREATEUSERBITMAP
409 stub CREATEUSERDISCARDABLEBITMAP
410 pascal16 IsValidMetaFile (word) IsValidMetaFile
411 pascal16 GetCurLogFont(word) GetCurLogFont
412 pascal16 IsDCCurrentPalette(word) IsDCCurrentPalette
439 pascal16 StretchDIBits (word s_word s_word word word word word
                               word word ptr ptr word long) StretchDIBits
440 pascal16 SetDIBits(word word word word ptr ptr word) SetDIBits
441 pascal16 GetDIBits(word word word word ptr ptr word) GetDIBits
442 pascal16 CreateDIBitmap(word ptr long ptr ptr word) CreateDIBitmap
443 pascal16 SetDIBitsToDevice(word s_word s_word word word word word
                               word word ptr ptr word) SetDIBitsToDevice
444 pascal16 CreateRoundRectRgn(s_word s_word s_word s_word s_word s_word)
             CreateRoundRectRgn
445 pascal16 CreateDIBPatternBrush(word word) CreateDIBPatternBrush
449 stub DEVICECOLORMATCH
450 pascal16 PolyPolygon(word ptr ptr word) PolyPolygon16
451 pascal16 CreatePolyPolygonRgn(ptr ptr word word) CreatePolyPolygonRgn16
452 stub GDISEEGDIDO
460 stub GDITASKTERMINATION
461 return SetObjectOwner 4 0
462 pascal16 IsGDIObject(word) IsGDIObject
463 stub MAKEOBJECTPRIVATE
464 stub FIXUPBOGUSPUBLISHERMETAFILE
465 pascal16 RectVisible(word ptr) RectVisible16
466 pascal16 RectInRegion(word ptr) RectInRegion16
467 stub UNICODETOANSI
468 pascal16 GetBitmapDimensionEx(word ptr) GetBitmapDimensionEx16
469 pascal16 GetBrushOrgEx(word ptr) GetBrushOrgEx16
470 pascal16 GetCurrentPositionEx(word ptr) GetCurrentPositionEx16
471 pascal16 GetTextExtentPoint(word ptr s_word ptr) GetTextExtentPoint16
472 pascal16 GetViewportExtEx(word ptr) GetViewportExtEx16
473 pascal16 GetViewportOrgEx(word ptr) GetViewportOrgEx16
474 pascal16 GetWindowExtEx(word ptr) GetWindowExtEx16
475 pascal16 GetWindowOrgEx(word ptr) GetWindowOrgEx16
476 pascal16 OffsetViewportOrgEx(word s_word s_word ptr) OffsetViewportOrgEx16
477 pascal16 OffsetWindowOrgEx(word s_word s_word ptr) OffsetWindowOrgEx16
478 pascal16 SetBitmapDimensionEx(word s_word s_word ptr) SetBitmapDimensionEx16
479 pascal16 SetViewportExtEx(word s_word s_word ptr) SetViewportExtEx16
480 pascal16 SetViewportOrgEx(word s_word s_word ptr) SetViewportOrgEx16
481 pascal16 SetWindowExtEx(word s_word s_word ptr) SetWindowExtEx16
482 pascal16 SetWindowOrgEx(word s_word s_word ptr) SetWindowOrgEx16
483 pascal16 MoveToEx(word s_word s_word ptr) MoveToEx16
484 pascal16 ScaleViewportExtEx(word s_word s_word s_word s_word ptr)
             ScaleViewportExtEx16
485 pascal16 ScaleWindowExtEx(word s_word s_word s_word s_word ptr)
             ScaleWindowExtEx16
486 stub GETASPECTRATIOFILTEREX
489 stub CreateDIBSection
490 stub CloseEnhMetafile
491 stub CopyEnhMetafile
492 stub CreateEnhMetafile
493 stub DeleteEnhMetafile
495 stub GDIComment
496 stub GetEnhMetafile
497 stub GetEnhMetafileBits
498 stub GetEnhMetafileDescription
499 stub GetEnhMetafileHeader
501 stub GetEnhMetafilePaletteEntries
502 stub PolyBezier
503 stub PolyBezierTo
504 stub PlayEnhMetafileRecord
505 stub SetEnhMetafileBits
506 stub SetMetaRgn
508 stub ExtSelectClipRgn
511 stub AbortPath
512 stub BeginPath
513 stub CloseFigure
514 stub EndPath
515 stub FillPath
516 stub FlattenPath
517 stub GetPath
518 stub PathToRegion
519 stub SelectClipPath
520 stub StrokeAndFillPath
521 stub StrokePath
522 stub WidenPath
523 stub ExtCreatePen
524 stub GetArcDirection
525 stub SetArcDirection
526 stub GetMiterLimit
527 stub SetMiterLimit
528 stub GDIParametersInfo
529 stub CreateHalftonePalette
# Stubs for Hebrew version
530 pascal16 GDI_530() stub_GDI_530
531 pascal16 GDI_531() stub_GDI_531
532 pascal16 GDI_532() stub_GDI_532
536 pascal16 GDI_536() stub_GDI_536
538 pascal16 GDI_538() stub_GDI_538
540 pascal16 GDI_540() stub_GDI_540
543 pascal16 GDI_543() stub_GDI_543
555 pascal16 GDI_555() stub_GDI_555
560 pascal16 GDI_560() stub_GDI_560
561 pascal16 GDI_561() stub_GDI_561
564 pascal16 GDI_564() stub_GDI_564
565 pascal16 GDI_565() stub_GDI_565
566 pascal16 GDI_566() stub_GDI_566
571 pascal16 GDI_571() stub_GDI_571
572 pascal16 GDI_572() stub_GDI_572
573 pascal16 GDI_573() stub_GDI_573
556 pascal16 GDI_556() stub_GDI_556
558 pascal16 GDI_558() stub_GDI_558
569 pascal16 GDI_569() stub_GDI_569
602 stub SetDIBColorTable
603 stub GetDIBColorTable
604 stub SetSolidBrush
605 stub SysDeleteObject
606 stub SetMagicColors
607 stub GetRegionData
608 stub ExtCreateRegion
609 stub GDIFreeResources
610 stub GDISignalProc32
611 stub GetRandomRgn
612 stub GetTextCharSet
613 stub EnumFontFamiliesEx
614 stub AddLpkToGDI
615 stub GetCharacterPlacement
616 stub GetFontLanguageInfo
650 stub BuildInverseTableDIB
701 stub GDITHKCONNECTIONDATALS
702 stub FT_GDIFTHKTHKCONNECTIONDATA
703 stub FDTHKCONNECTIONDATASL
704 stub ICMTHKCONNECTIONDATASL
820 stub ICMCreateTransform
821 stub ICMDeleteTransform
822 stub ICMTranslateRGB
823 stub ICMTranslateRGBs
824 stub ICMCheckCOlorsInGamut
