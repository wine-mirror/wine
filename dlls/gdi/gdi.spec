name	gdi
type	win16
heap	65488  # 65536 - 16 (instance data) - 32 (stock objects)
file	gdi.exe
owner	gdi32

1   pascal   SetBkColor(word long) SetBkColor16
2   pascal16 SetBkMode(word word) SetBkMode16
3   pascal16 SetMapMode(word word) SetMapMode16
4   pascal16 SetROP2(word word) SetROP216
5   pascal16 SetRelAbs(word word) SetRelAbs16
6   pascal16 SetPolyFillMode(word word) SetPolyFillMode16
7   pascal16 SetStretchBltMode(word word) SetStretchBltMode16
8   pascal16 SetTextCharacterExtra(word s_word) SetTextCharacterExtra16
9   pascal   SetTextColor(word long) SetTextColor16
10  pascal16 SetTextJustification(word s_word s_word) SetTextJustification16
11  pascal   SetWindowOrg(word s_word s_word) SetWindowOrg16
12  pascal   SetWindowExt(word s_word s_word) SetWindowExt16
13  pascal   SetViewportOrg(word s_word s_word) SetViewportOrg16
14  pascal   SetViewportExt(word s_word s_word) SetViewportExt16
15  pascal   OffsetWindowOrg(word s_word s_word) OffsetWindowOrg16
16  pascal   ScaleWindowExt(word s_word s_word s_word s_word) ScaleWindowExt16
17  pascal   OffsetViewportOrg(word s_word s_word) OffsetViewportOrg16
18  pascal ScaleViewportExt(word s_word s_word s_word s_word) ScaleViewportExt16
19  pascal16 LineTo(word s_word s_word) LineTo16
20  pascal   MoveTo(word s_word s_word) MoveTo16
21  pascal16 ExcludeClipRect(word s_word s_word s_word s_word) ExcludeClipRect16
22  pascal16 IntersectClipRect(word s_word s_word s_word s_word) IntersectClipRect16
23  pascal16 Arc(word s_word s_word s_word s_word s_word s_word s_word s_word)
             Arc16
24  pascal16 Ellipse(word s_word s_word s_word s_word) Ellipse16
25  pascal16 FloodFill(word s_word s_word long) FloodFill16
26  pascal16 Pie(word s_word s_word s_word s_word s_word s_word s_word s_word)
             Pie16
27  pascal16 Rectangle(word s_word s_word s_word s_word) Rectangle16
28  pascal16 RoundRect(word s_word s_word s_word s_word s_word s_word)
             RoundRect16
29  pascal16 PatBlt(word s_word s_word s_word s_word long) PatBlt16
30  pascal16 SaveDC(word) SaveDC16
31  pascal   SetPixel(word s_word s_word long) SetPixel16
32  pascal16 OffsetClipRgn(word s_word s_word) OffsetClipRgn16
33  pascal16 TextOut(word s_word s_word str word) TextOut16
34  pascal16 BitBlt( word s_word s_word s_word s_word word s_word s_word long)
             BitBlt16
35  pascal16 StretchBlt(word s_word s_word s_word s_word word s_word s_word
                        s_word s_word long) StretchBlt16
36  pascal16 Polygon (word ptr word) Polygon16
37  pascal16 Polyline (word ptr word) Polyline16
38  pascal   Escape(word word word segptr segptr) Escape16
39  pascal16 RestoreDC(word s_word) RestoreDC16
40  pascal16 FillRgn(word word word) FillRgn16
41  pascal16 FrameRgn(word word word word word) FrameRgn16
42  pascal16 InvertRgn(word word) InvertRgn16
43  pascal16 PaintRgn(word word) PaintRgn16
44  pascal16 SelectClipRgn(word word) SelectClipRgn16
45  pascal16 SelectObject(word word) SelectObject16
46  stub BITMAPBITS # W1.1, W2.0 
47  pascal16 CombineRgn(word word word s_word) CombineRgn16
48  pascal16 CreateBitmap(word word word word ptr) CreateBitmap16
49  pascal16 CreateBitmapIndirect(ptr) CreateBitmapIndirect16
50  pascal16 CreateBrushIndirect(ptr) CreateBrushIndirect16
51  pascal16 CreateCompatibleBitmap(word word word) CreateCompatibleBitmap16
52  pascal16 CreateCompatibleDC(word) CreateCompatibleDC16
53  pascal16 CreateDC(ptr ptr ptr ptr) CreateDC16
54  pascal16 CreateEllipticRgn(s_word s_word s_word s_word) CreateEllipticRgn16
55  pascal16 CreateEllipticRgnIndirect(ptr) CreateEllipticRgnIndirect16
56  pascal16 CreateFont(s_word s_word s_word s_word s_word word word word
                        word word word word word str) CreateFont16
57  pascal16 CreateFontIndirect(ptr) CreateFontIndirect16
58  pascal16 CreateHatchBrush(word long) CreateHatchBrush16
#59 ??? (not even in W1.1, W2.0)
60  pascal16 CreatePatternBrush(word) CreatePatternBrush16
61  pascal16 CreatePen(s_word s_word long) CreatePen16
62  pascal16 CreatePenIndirect(ptr) CreatePenIndirect16
63  pascal16 CreatePolygonRgn(ptr word word) CreatePolygonRgn16
64  pascal16 CreateRectRgn(s_word s_word s_word s_word) CreateRectRgn16
65  pascal16 CreateRectRgnIndirect(ptr) CreateRectRgnIndirect16
66  pascal16 CreateSolidBrush(long) CreateSolidBrush16
67  pascal16 DPtoLP(word ptr s_word) DPtoLP16
68  pascal16 DeleteDC(word) DeleteDC16
69  pascal16 DeleteObject(word) DeleteObject16
70  pascal16 EnumFonts(word str segptr long) THUNK_EnumFonts16
71  pascal16 EnumObjects(word word segptr long) THUNK_EnumObjects16
72  pascal16 EqualRgn(word word) EqualRgn16
73  pascal16 ExcludeVisRect(word s_word s_word s_word s_word) ExcludeVisRect16
74  pascal   GetBitmapBits(word long ptr) GetBitmapBits16
75  pascal   GetBkColor(word) GetBkColor16
76  pascal16 GetBkMode(word) GetBkMode16
77  pascal16 GetClipBox(word ptr) GetClipBox16
78  pascal   GetCurrentPosition(word) GetCurrentPosition16
79  pascal   GetDCOrg(word) GetDCOrg16
80  pascal16 GetDeviceCaps(word s_word) GetDeviceCaps16
81  pascal16 GetMapMode(word) GetMapMode16
82  pascal16 GetObject(word s_word ptr) GetObject16
83  pascal   GetPixel(word s_word s_word) GetPixel16
84  pascal16 GetPolyFillMode(word) GetPolyFillMode16
85  pascal16 GetROP2(word) GetROP216
86  pascal16 GetRelAbs(word) GetRelAbs16
87  pascal16 GetStockObject(word) GetStockObject16
88  pascal16 GetStretchBltMode(word) GetStretchBltMode16
89  pascal16 GetTextCharacterExtra(word) GetTextCharacterExtra16
90  pascal   GetTextColor(word) GetTextColor16
91  pascal   GetTextExtent(word ptr s_word) GetTextExtent16
92  pascal16 GetTextFace(word s_word ptr) GetTextFace16
93  pascal16 GetTextMetrics(word ptr) GetTextMetrics16
94  pascal   GetViewportExt(word) GetViewportExt16
95  pascal   GetViewportOrg(word) GetViewportOrg16
96  pascal   GetWindowExt(word) GetWindowExt16
97  pascal   GetWindowOrg(word) GetWindowOrg16
98  pascal16 IntersectVisRect(word s_word s_word s_word s_word)
             IntersectVisRect16
99  pascal16 LPtoDP(word ptr s_word) LPtoDP16
100 pascal16 LineDDA(s_word s_word s_word s_word segptr long) LineDDA16
101 pascal16 OffsetRgn(word s_word s_word) OffsetRgn16
102 pascal16 OffsetVisRgn(word s_word s_word) OffsetVisRgn16
103 pascal16 PtVisible(word s_word s_word) PtVisible16
104 pascal16 RectVisibleOld(word ptr) RectVisible16 # also named RECTVISIBLE
105 pascal16 SelectVisRgn(word word) SelectVisRgn16
106 pascal   SetBitmapBits(word long ptr) SetBitmapBits16
# ??? (not even in W1.1)
117 pascal   SetDCOrg(word s_word s_word) SetDCOrg16
118 stub InternalCreateDC # W1.1, W2.0
119 pascal16 AddFontResource(str) AddFontResource16
120 stub GetContinuingTextExtent # W1.1, W2.0
121 pascal16 Death(word) Death16
122 pascal16 Resurrection(word word word word word word word) Resurrection16
123 pascal16 PlayMetaFile(word word) PlayMetaFile16
124 pascal16 GetMetaFile(str) GetMetaFile16
125 pascal16 CreateMetaFile(str) CreateMetaFile16
126 pascal16 CloseMetaFile(word) CloseMetaFile16
127 pascal16 DeleteMetaFile(word) DeleteMetaFile16
128 pascal16 MulDiv(s_word s_word s_word) MulDiv16
129 pascal16 SaveVisRgn(word) SaveVisRgn16
130 pascal16 RestoreVisRgn(word) RestoreVisRgn16
131 pascal16 InquireVisRgn(word) InquireVisRgn16
132 pascal16 SetEnvironment(str str word) SetEnvironment16
133 pascal16 GetEnvironment(str str word) GetEnvironment16
134 pascal16 GetRgnBox(word ptr) GetRgnBox16
#135 pascal ScanLr
136 pascal16 RemoveFontResource(segptr) RemoveFontResource16
#137 - 147 removed sometime after W2.0
137 stub GSV
138 stub DPXlate
139 stub SetWinViewExt
140 stub ScaleExt
141 stub WordSet
142 stub RectStuff
143 stub OffsetOrg
144 stub LockDC # < W2.0
145 stub UnlockDC # < W2.0
146 stub LockUnlock # < W2.0
147 stub GDI_FarFrame
148 pascal SetBrushOrg(word s_word s_word) SetBrushOrg16
149 pascal GetBrushOrg(word) GetBrushOrg16
150 pascal16 UnrealizeObject(word) UnrealizeObject16
151 pascal16 CopyMetaFile(word str) CopyMetaFile16
152 stub GDIInitApp # W1.1, W2.0
153 pascal16 CreateIC(str str str ptr) CreateIC16
154 pascal   GetNearestColor(word long) GetNearestColor16
155 pascal16 QueryAbort(word word) QueryAbort16
156 pascal16 CreateDiscardableBitmap(word word word) CreateDiscardableBitmap16
157 stub CompatibleBitmap # W1.1, W2.0
158 pascal16 EnumCallback(ptr ptr word long) EnumCallback16
159 pascal16 GetMetaFileBits(word) GetMetaFileBits16
160 pascal16 SetMetaFileBits(word) SetMetaFileBits16
161 pascal16 PtInRegion(word s_word s_word) PtInRegion16
162 pascal   GetBitmapDimension(word) GetBitmapDimension16
163 pascal   SetBitmapDimension(word s_word s_word) SetBitmapDimension16
164 stub PixToLine # W1.1, W2.0
#165 - 200 not in W1.1
169 stub IsDCDirty
170 stub SetDCStatus
171 stub LVBUNION # W2.0 (only ?)
172 pascal16 SetRectRgn(word s_word s_word s_word s_word) SetRectRgn16
173 pascal16 GetClipRgn(word) GetClipRgn16
174 stub BLOAT # W2.0 (only ?) ROTFL ! ;-))
175 pascal16 EnumMetaFile(word word segptr long) THUNK_EnumMetaFile16
176 pascal16 PlayMetaFileRecord(word ptr ptr word) PlayMetaFileRecord16
177 stub RCOS # W2.0 (only ?)
178 stub RSIN # W2.0 (only ?)
179 pascal16 GetDCState(word) GetDCState16
180 pascal16 SetDCState(word word) SetDCState16
181 pascal16 RectInRegionOld(word ptr) RectInRegion16 # also named RECTINREGION
182 stub REQUESTSEM # W2.0 (only ?)
183 stub CLEARSEM # W2.0 (only ?)
184 stub STUFFVISIBLE # W2.0 (only ?)
185 stub STUFFINREGION # W2.0 (only ?)
186 stub DELETEABOVELINEFONTS # W2.0 (only ?)
188 stub GetTextExtentEx
190 pascal16 SetDCHook(word segptr long) SetDCHook
191 pascal   GetDCHook(word ptr) GetDCHook
192 pascal16 SetHookFlags(word word) SetHookFlags16
193 pascal16 SetBoundsRect(word ptr word) SetBoundsRect16
194 pascal16 GetBoundsRect(word ptr word) GetBoundsRect16
195 stub SelectBitmap
196 pascal16 SetMetaFileBitsBetter(word) SetMetaFileBitsBetter16
201 stub DMBITBLT
202 stub DMCOLORINFO
206 pascal16 dmEnumDFonts(ptr str ptr ptr) dmEnumDFonts16
207 stub DMENUMOBJ
208 stub DMOUTPUT
209 stub DMPIXEL
210 pascal16 dmRealizeObject(ptr word ptr ptr segptr) dmRealizeObject16
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
230 pascal16 CreatePQ(word) CreatePQ16
231 pascal16 MinPQ(word) MinPQ16
232 pascal16 ExtractPQ(word) ExtractPQ16
233 pascal16 InsertPQ(word word word) InsertPQ16
234 pascal16 SizePQ(word word) SizePQ16
235 pascal16 DeletePQ(word) DeletePQ16
240 pascal16 OpenJob(ptr ptr word) OpenJob16
241 pascal16 WriteSpool(word ptr word) WriteSpool16
242 pascal16 WriteDialog(word ptr word) WriteDialog16
243 pascal16 CloseJob(word) CloseJob16
244 pascal16 DeleteJob(word word) DeleteJob16
245 pascal   GetSpoolJob(word ptr) GetSpoolJob16
246 pascal16 StartSpoolPage(word) StartSpoolPage16
247 pascal16 EndSpoolPage(word) EndSpoolPage16
248 stub QueryJob
250 pascal16 Copy(ptr ptr word) Copy16
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
281 pascal   DrvSetPrinterData(ptr ptr long ptr long) DrvSetPrinterData16
282 pascal   DrvGetPrinterData(ptr ptr ptr ptr long ptr) DrvGetPrinterData16
299 stub ENGINEGETCHARWIDTHEX
300 pascal   EngineEnumerateFont(ptr segptr long) EngineEnumerateFont16
301 pascal16 EngineDeleteFont(ptr) EngineDeleteFont16
302 pascal   EngineRealizeFont(ptr ptr ptr) EngineRealizeFont16
303 pascal16 EngineGetCharWidth(ptr word word ptr) EngineGetCharWidth16
304 stub ENGINESETFONTCONTEXT
305 stub ENGINEGETGLYPHBMP
306 stub ENGINEMAKEFONTDIR
307 pascal16 GetCharABCWidths(word word word ptr) GetCharABCWidths16
308 pascal16 GetOutlineTextMetrics(word word ptr) GetOutlineTextMetrics16
309 pascal   GetGlyphOutline(word word word ptr long ptr ptr) GetGlyphOutline16
310 pascal16 CreateScalableFontResource(word str str str) CreateScalableFontResource16
311 pascal   GetFontData(word long long ptr long) GetFontData16
312 stub ConvertOutLineFontFile
313 pascal16 GetRasterizerCaps(ptr word) GetRasterizerCaps16
314 stub EngineExtTextOut
315 pascal   EngineRealizeFontExt(long long long long) EngineRealizeFontExt16
316 stub EngineGetCharWidthStr
317 stub EngineGetGlyphBmpExt
330 pascal16 EnumFontFamilies(word str segptr long) THUNK_EnumFontFamilies16
332 pascal16 GetKerningPairs(word word ptr) GetKerningPairs16
345 pascal16 GetTextAlign(word) GetTextAlign16
346 pascal16 SetTextAlign(word word) SetTextAlign16
347 stub MFDRAWTEXT # W2.0 (only ?)
348 pascal16 Chord(word s_word s_word s_word s_word s_word s_word
                   s_word s_word) Chord16
349 pascal   SetMapperFlags(word long) SetMapperFlags16
350 pascal16 GetCharWidth(word word word ptr) GetCharWidth16
351 pascal16 ExtTextOut(word s_word s_word word ptr str word ptr) ExtTextOut16
352 stub GetPhysicalFontHandle
353 stub GetAspectRatioFilter
354 stub ShrinkGDIHeap
355 stub FTrapping0
360 pascal16 CreatePalette(ptr) CreatePalette16
361 pascal16 GDISelectPalette(word word word) GDISelectPalette16
362 pascal16 GDIRealizePalette(word) GDIRealizePalette16
363 pascal16 GetPaletteEntries(word word word ptr) GetPaletteEntries16
364 pascal16 SetPaletteEntries(word word word ptr) SetPaletteEntries16
365 pascal16 RealizeDefaultPalette(word) RealizeDefaultPalette16
366 pascal16 UpdateColors(word) UpdateColors16
367 pascal16 AnimatePalette(word word word ptr) AnimatePalette16
368 pascal16 ResizePalette(word word) ResizePalette16
370 pascal16 GetNearestPaletteIndex(word long) GetNearestPaletteIndex16
372 pascal16 ExtFloodFill(word s_word s_word long word) ExtFloodFill16
373 pascal16 SetSystemPaletteUse(word word) SetSystemPaletteUse16
374 pascal16 GetSystemPaletteUse(word) GetSystemPaletteUse16
375 pascal16 GetSystemPaletteEntries(word word word ptr) GetSystemPaletteEntries16
376 pascal16 ResetDC(word ptr) ResetDC16
377 pascal16 StartDoc(word ptr) StartDoc16
378 pascal16 EndDoc(word) EndDoc16
379 pascal16 StartPage(word) StartPage16
380 pascal16 EndPage(word) EndPage16
381 pascal16 SetAbortProc(word segptr) SetAbortProc16
382 pascal16 AbortDoc(word) AbortDoc16
400 pascal16 FastWindowFrame(word ptr s_word s_word long) FastWindowFrame16
401 stub GDIMOVEBITMAP
402 stub GDIGETBITSGLOBAL # W2.0 (only ?)
403 stub GDIINIT2
404 stub GetTTGlyphIndexMap
405 pascal16 FinalGdiInit(word) FinalGdiInit16
406 stub CREATEREALBITMAPINDIRECT # W2.0 (only ?)
407 pascal16 CreateUserBitmap(word word word word ptr) CreateUserBitmap16
408 stub CREATEREALBITMAP # W2.0 (only ?)
409 pascal16 CreateUserDiscardableBitmap(word word word) CreateUserDiscardableBitmap16
410 pascal16 IsValidMetaFile (word) IsValidMetaFile16
411 pascal16 GetCurLogFont(word) GetCurLogFont16
412 pascal16 IsDCCurrentPalette(word) IsDCCurrentPalette16
439 pascal16 StretchDIBits (word s_word s_word s_word s_word s_word s_word
                            s_word s_word ptr ptr word long) StretchDIBits16
440 pascal16 SetDIBits(word word word word ptr ptr word) SetDIBits16
441 pascal16 GetDIBits(word word word word ptr ptr word) GetDIBits16
442 pascal16 CreateDIBitmap(word ptr long ptr ptr word) CreateDIBitmap16
443 pascal16 SetDIBitsToDevice(word s_word s_word s_word s_word s_word s_word
                               word word ptr ptr word) SetDIBitsToDevice16
444 pascal16 CreateRoundRectRgn(s_word s_word s_word s_word s_word s_word)
             CreateRoundRectRgn16
445 pascal16 CreateDIBPatternBrush(word word) CreateDIBPatternBrush16
449 stub DEVICECOLORMATCH
450 pascal16 PolyPolygon(word ptr ptr word) PolyPolygon16
451 pascal16 CreatePolyPolygonRgn(ptr ptr word word) CreatePolyPolygonRgn16
452 pascal   GdiSeeGdiDo(word word word word) GdiSeeGdiDo16
460 stub GDITASKTERMINATION
461 pascal16 SetObjectOwner(word word) SetObjectOwner16
462 pascal16 IsGDIObject(word) IsGDIObject16
463 pascal16 MakeObjectPrivate(word word) MakeObjectPrivate16
464 stub FIXUPBOGUSPUBLISHERMETAFILE
465 pascal16 RectVisible(word ptr) RectVisible16 # RECTVISIBLE_EHH ??
466 pascal16 RectInRegion(word ptr) RectInRegion16 # RECTINREGION_EHH ??
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
486 pascal16 GetAspectRatioFilterEx(word ptr) GetAspectRatioFilterEx16
489 pascal16 CreateDIBSection(word ptr word ptr long long) CreateDIBSection16
490 stub CloseEnhMetafile
#490 stub POLYLINEWOW # conflicts with CloseEnhMetaFile !!
491 stub CopyEnhMetafile
492 stub CreateEnhMetafile
493 stub DeleteEnhMetafile
495 stub GDIComment
496 stub GetEnhMetafile
497 stub GetEnhMetafileBits
498 stub GetEnhMetafileDescription
499 stub GetEnhMetafileHeader
501 stub GetEnhMetafilePaletteEntries
502 pascal16 PolyBezier(word ptr word) PolyBezier16
503 pascal16 PolyBezierTo(word ptr word) PolyBezierTo16
504 stub PlayEnhMetafileRecord
505 stub SetEnhMetafileBits
506 stub SetMetaRgn
508 pascal16 ExtSelectClipRgn(word word word) ExtSelectClipRgn16
511 pascal16 AbortPath(word) AbortPath16
512 pascal16 BeginPath(word) BeginPath16
513 pascal16 CloseFigure(word) CloseFigure16
514 pascal16 EndPath(word) EndPath16
515 pascal16 FillPath(word) FillPath16
516 pascal16 FlattenPath(word) FlattenPath16
517 pascal16 GetPath(word ptr ptr word) GetPath16
518 pascal16 PathToRegion(word) PathToRegion16
519 pascal16 SelectClipPath(word word) SelectClipPath16
520 pascal16 StrokeAndFillPath(word) StrokeAndFillPath16
521 pascal16 StrokePath(word) StrokePath16
522 pascal16 WidenPath(word) WidenPath16
523 stub ExtCreatePen
524 pascal16 GetArcDirection(word) GetArcDirection16
525 pascal16 SetArcDirection(word word) SetArcDirection16
526 stub GetMiterLimit
527 stub SetMiterLimit
528 stub GDIParametersInfo
529 pascal16 CreateHalftonePalette(word) CreateHalftonePalette16
# Hebrew version API's
530 pascal16 RawTextOut() RawTextOut16
531 pascal16 RawExtTextOut() RawExtTextOut16
532 pascal16 RawGetTextExtent(word str word) RawGetTextExtent16
536 pascal16 BiDiLayout() BiDiLayout16
538 pascal16 BiDiCreateTabString() BiDiCreateTabString16
540 pascal16 BiDiGlyphOut() BiDiGlyphOut16
543 pascal16 BiDiGetStringExtent() BiDiGetStringExtent16
555 pascal16 BiDiDeleteString() BiDiDeleteString16
556 pascal16 BiDiSetDefaults() BiDiSetDefaults16
558 pascal16 BiDiGetDefaults() BiDiGetDefaults16
560 pascal16 BiDiShape() BiDiShape16
561 pascal16 BiDiFontComplement() BiDiFontComplement16
564 pascal16 BiDiSetKashida() BiDiSetKashida16
565 pascal16 BiDiKExtTextOut() BiDiKExtTextOut16
566 pascal16 BiDiShapeEx() BiDiShapeEx16
569 pascal16 BiDiCreateStringEx() BiDiCreateStringEx16
571 pascal16 GetTextExtentRtoL() GetTextExtentRtoL16
572 pascal16 GetHDCCharSet() GetHDCCharSet16
573 pascal16 BiDiLayoutEx() BiDiLayoutEx16
602 pascal16 SetDIBColorTable(word word word ptr) SetDIBColorTable16
603 pascal16 GetDIBColorTable(word word word ptr) GetDIBColorTable16
604 pascal16 SetSolidBrush(word long) SetSolidBrush16
605 pascal16 SysDeleteObject(word) DeleteObject16    # ???
606 pascal16 SetMagicColors(word long word) SetMagicColors16
607 pascal   GetRegionData(word long ptr) GetRegionData16
608 stub ExtCreateRegion
609 pascal16 GdiFreeResources(long) GdiFreeResources16
610 pascal16 GdiSignalProc32(long long long word) GdiSignalProc
611 stub GetRandomRgn
612 pascal16 GetTextCharset(word) GetTextCharset16
613 pascal16 EnumFontFamiliesEx(word ptr segptr long long) THUNK_EnumFontFamiliesEx16
614 stub AddLpkToGDI
615 stub GetCharacterPlacement
616 pascal   GetFontLanguageInfo(word) GetFontLanguageInfo16
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
1000 pascal16 SetLayout(word long) SetLayout16
1001 stub GetLayout
