1000 stub @
1001 stub @
1002 stub @
1003 stub @
1004 stub @
1005 stub @
1006 stub @
1007 stub @
1008 stub @
1009 stub @
1010 stub @

1013 stub @
1014 stub DwmCreatedBitmapRemotingOutput
1015 stub @
1016 stub @

@ stdcall AbortDoc(long)
@ stdcall AbortPath(long)
@ stdcall AddFontMemResourceEx(ptr long ptr ptr)
@ stdcall AddFontResourceA(str)
@ stdcall AddFontResourceExA(str long ptr)
@ stdcall AddFontResourceExW(wstr long ptr)
@ stub AddFontResourceTracking
@ stdcall AddFontResourceW(wstr)
@ stdcall AngleArc(long long long long float float)
@ stdcall AnimatePalette(long long long ptr)
# @ stub AnyLinkedFonts
@ stdcall Arc(long long long long long long long long long)
@ stdcall ArcTo(long long long long long long long long long)
# @ stub BeginGdiRendering
@ stdcall BeginPath(long)
@ stdcall BitBlt(long long long long long long long long long)
@ stub ByeByeGDI
@ stdcall CancelDC(long)
@ stub CheckColorsInGamut
@ stdcall ChoosePixelFormat(long ptr)
@ stdcall Chord(long long long long long long long long long)
# @ stub ClearBitmapAttributes
# @ stub ClearBrushAttributes
@ stdcall CloseEnhMetaFile(long)
@ stdcall CloseFigure(long)
@ stdcall CloseMetaFile(long)
@ stub ColorCorrectPalette
@ stub ColorMatchToTarget
@ stdcall CombineRgn(long long long long) NtGdiCombineRgn
@ stdcall CombineTransform(ptr ptr ptr)
# @ stub ConfigureOPMProtectedOutput
@ stdcall CopyEnhMetaFileA(long str)
@ stdcall CopyEnhMetaFileW(long wstr)
@ stdcall CopyMetaFileA(long str)
@ stdcall CopyMetaFileW(long wstr)
@ stdcall CreateBitmap(long long long long ptr)
# @ stub CreateBitmapFromDxSurface
# @ stub CreateBitmapFromDxSurface2
@ stdcall CreateBitmapIndirect(ptr)
@ stdcall CreateBrushIndirect(ptr)
@ stdcall CreateColorSpaceA(ptr)
@ stdcall CreateColorSpaceW(ptr)
@ stdcall CreateCompatibleBitmap(long long long)
@ stdcall CreateCompatibleDC(long) NtGdiCreateCompatibleDC
@ stdcall CreateDCA(str str str ptr)
@ stdcall CreateDCW(wstr wstr wstr ptr)
@ stdcall CreateDIBPatternBrush(long long)
@ stdcall CreateDIBPatternBrushPt(ptr long)
@ stdcall CreateDIBSection(long ptr long ptr long long)
@ stdcall CreateDIBitmap(long ptr long ptr ptr long)
# @ stub CreateDPIScaledDIBSection
@ stdcall CreateDiscardableBitmap(long long long)
@ stdcall CreateEllipticRgn(long long long long) NtGdiCreateEllipticRgn
@ stdcall CreateEllipticRgnIndirect(ptr)
@ stdcall CreateEnhMetaFileA(long str ptr str)
@ stdcall CreateEnhMetaFileW(long wstr ptr wstr)
@ stdcall CreateFontA(long long long long long long long long long long long long long str)
@ stdcall CreateFontIndirectA(ptr)
@ stdcall CreateFontIndirectExA(ptr)
@ stdcall CreateFontIndirectExW(ptr)
@ stdcall CreateFontIndirectW(ptr)
@ stdcall CreateFontW(long long long long long long long long long long long long long wstr)
@ stdcall CreateHalftonePalette(long) NtGdiCreateHalftonePalette
@ stdcall CreateHatchBrush(long long)
@ stdcall CreateICA(str str str ptr)
@ stdcall CreateICW(wstr wstr wstr ptr)
@ stdcall CreateMetaFileA(str)
@ stdcall CreateMetaFileW(wstr)
# @ stub CreateOPMProtectedOutput
# @ stub CreateOPMProtectedOutputs
@ stdcall CreatePalette(ptr)
@ stdcall CreatePatternBrush(long)
@ stdcall CreatePen(long long long)
@ stdcall CreatePenIndirect(ptr)
@ stdcall CreatePolyPolygonRgn(ptr ptr long long)
@ stdcall CreatePolygonRgn(ptr long long)
@ stdcall CreateRectRgn(long long long long) NtGdiCreateRectRgn
@ stdcall CreateRectRgnIndirect(ptr)
@ stdcall CreateRoundRectRgn(long long long long long long) NtGdiCreateRoundRectRgn
@ stdcall CreateScalableFontResourceA(long str str str)
@ stdcall CreateScalableFontResourceW(long wstr wstr wstr)
# @ stub CreateScaledCompatibleBitmap
# @ stub CreateSessionMappedDIBSection
@ stdcall CreateSolidBrush(long)
@ stdcall D3DKMTCheckOcclusion(ptr) win32u.NtGdiDdDDICheckOcclusion
@ stdcall D3DKMTCheckVidPnExclusiveOwnership(ptr) win32u.NtGdiDdDDICheckVidPnExclusiveOwnership
@ stdcall D3DKMTCloseAdapter(ptr) win32u.NtGdiDdDDICloseAdapter
@ stdcall D3DKMTCreateAllocation(ptr) win32u.NtGdiDdDDICreateAllocation
@ stdcall D3DKMTCreateAllocation2(ptr) win32u.NtGdiDdDDICreateAllocation2
@ stdcall D3DKMTCreateDCFromMemory(ptr) win32u.NtGdiDdDDICreateDCFromMemory
@ stdcall D3DKMTCreateDevice(ptr) win32u.NtGdiDdDDICreateDevice
@ stdcall D3DKMTCreateKeyedMutex(ptr) win32u.NtGdiDdDDICreateKeyedMutex
@ stdcall D3DKMTCreateKeyedMutex2(ptr) win32u.NtGdiDdDDICreateKeyedMutex2
@ stdcall D3DKMTCreateSynchronizationObject(ptr) win32u.NtGdiDdDDICreateSynchronizationObject
@ stdcall D3DKMTCreateSynchronizationObject2(ptr) win32u.NtGdiDdDDICreateSynchronizationObject2
@ stdcall D3DKMTDestroyAllocation(ptr) win32u.NtGdiDdDDIDestroyAllocation
@ stdcall D3DKMTDestroyAllocation2(ptr) win32u.NtGdiDdDDIDestroyAllocation2
@ stdcall D3DKMTDestroyDCFromMemory(ptr) win32u.NtGdiDdDDIDestroyDCFromMemory
@ stdcall D3DKMTDestroyDevice(ptr) win32u.NtGdiDdDDIDestroyDevice
@ stdcall D3DKMTDestroyKeyedMutex(ptr) win32u.NtGdiDdDDIDestroyKeyedMutex
@ stdcall D3DKMTDestroySynchronizationObject(ptr) win32u.NtGdiDdDDIDestroySynchronizationObject
@ stdcall D3DKMTEnumAdapters(ptr) win32u.NtGdiDdDDIEnumAdapters
@ stdcall D3DKMTEnumAdapters2(ptr) win32u.NtGdiDdDDIEnumAdapters2
@ stdcall D3DKMTEscape(ptr) win32u.NtGdiDdDDIEscape
@ stdcall D3DKMTOpenAdapterFromDeviceName(ptr) win32u.NtGdiDdDDIOpenAdapterFromDeviceName
@ stdcall D3DKMTOpenAdapterFromGdiDisplayName(ptr)
@ stdcall D3DKMTOpenAdapterFromHdc(ptr) win32u.NtGdiDdDDIOpenAdapterFromHdc
@ stdcall D3DKMTOpenAdapterFromLuid(ptr) win32u.NtGdiDdDDIOpenAdapterFromLuid
@ stdcall D3DKMTOpenKeyedMutex(ptr) win32u.NtGdiDdDDIOpenKeyedMutex
@ stdcall D3DKMTOpenKeyedMutex2(ptr) win32u.NtGdiDdDDIOpenKeyedMutex2
@ stdcall D3DKMTOpenKeyedMutexFromNtHandle(ptr) win32u.NtGdiDdDDIOpenKeyedMutexFromNtHandle
@ stdcall D3DKMTOpenNtHandleFromName(ptr) win32u.NtGdiDdDDIOpenNtHandleFromName
@ stdcall D3DKMTOpenResource(ptr) win32u.NtGdiDdDDIOpenResource
@ stdcall D3DKMTOpenResource2(ptr) win32u.NtGdiDdDDIOpenResource2
@ stdcall D3DKMTOpenResourceFromNtHandle(ptr) win32u.NtGdiDdDDIOpenResourceFromNtHandle
@ stdcall D3DKMTOpenSyncObjectFromNtHandle(ptr) win32u.NtGdiDdDDIOpenSyncObjectFromNtHandle
@ stdcall D3DKMTOpenSyncObjectFromNtHandle2(ptr) win32u.NtGdiDdDDIOpenSyncObjectFromNtHandle2
@ stdcall D3DKMTOpenSyncObjectNtHandleFromName(ptr) win32u.NtGdiDdDDIOpenSyncObjectNtHandleFromName
@ stdcall D3DKMTOpenSynchronizationObject(ptr) win32u.NtGdiDdDDIOpenSynchronizationObject
@ stdcall D3DKMTQueryAdapterInfo(ptr) win32u.NtGdiDdDDIQueryAdapterInfo
@ stdcall D3DKMTQueryResourceInfo(ptr) win32u.NtGdiDdDDIQueryResourceInfo
@ stdcall D3DKMTQueryResourceInfoFromNtHandle(ptr) win32u.NtGdiDdDDIQueryResourceInfoFromNtHandle
@ stdcall D3DKMTQueryStatistics(ptr) win32u.NtGdiDdDDIQueryStatistics
@ stdcall D3DKMTQueryVideoMemoryInfo(ptr) win32u.NtGdiDdDDIQueryVideoMemoryInfo
@ stdcall D3DKMTSetQueuedLimit(ptr) win32u.NtGdiDdDDISetQueuedLimit
@ stdcall D3DKMTSetVidPnSourceOwner(ptr) win32u.NtGdiDdDDISetVidPnSourceOwner
@ stdcall D3DKMTShareObjects(long ptr ptr long ptr) win32u.NtGdiDdDDIShareObjects
@ stdcall DPtoLP(long ptr long)
@ stdcall DeleteColorSpace(long)
@ stdcall DeleteDC(long)
@ stdcall DeleteEnhMetaFile(long)
@ stdcall DeleteMetaFile(long)
@ stdcall DeleteObject(long)
@ stdcall DescribePixelFormat(long long long ptr)
# @ stub DestroyOPMProtectedOutput
# @ stub DestroyPhysicalMonitorInternal
@ stub DeviceCapabilitiesEx
@ stub DeviceCapabilitiesExA
@ stub DeviceCapabilitiesExW
@ stdcall DrawEscape(long long long ptr)
@ stdcall Ellipse(long long long long long)
@ stdcall EnableEUDC(long)
@ stdcall EndDoc(long)
# @ stub EndFormPage
# @ stub EndGdiRendering
@ stdcall EndPage(long)
@ stdcall EndPath(long)
@ stdcall EnumEnhMetaFile(long long ptr ptr ptr)
@ stdcall EnumFontFamiliesA(long str ptr long)
@ stdcall EnumFontFamiliesExA(long ptr ptr long long)
@ stdcall EnumFontFamiliesExW(long ptr ptr long long)
@ stdcall EnumFontFamiliesW(long wstr ptr long)
@ stdcall EnumFontsA(long str ptr long)
@ stdcall EnumFontsW(long wstr ptr long)
@ stdcall EnumICMProfilesA(long ptr long)
@ stdcall EnumICMProfilesW(long ptr long)
@ stdcall EnumMetaFile(long long ptr ptr)
@ stdcall EnumObjects(long long ptr long)
@ stdcall EqualRgn(long long) NtGdiEqualRgn
@ stdcall Escape(long long long ptr ptr)
# @ stub EudcLoadLinkW
# @ stub EudcUnloadLinkW
@ stdcall ExcludeClipRect(long long long long long)
@ stdcall ExtCreatePen(long long ptr long ptr)
@ stdcall ExtCreateRegion(ptr long ptr)
@ stdcall ExtEscape(long long long ptr long ptr)
@ stdcall ExtFloodFill(long long long long long)
@ stdcall ExtSelectClipRgn(long long long)
@ stdcall ExtTextOutA(long long long long ptr str long ptr)
@ stdcall ExtTextOutW(long long long long ptr wstr long ptr)
@ stdcall FillPath(long)
@ stdcall FillRgn(long long long)
@ stdcall FixBrushOrgEx(long long long ptr)
@ stdcall FlattenPath(long)
@ stdcall FloodFill(long long long long)
@ stdcall FontIsLinked(long) NtGdiFontIsLinked
@ stdcall FrameRgn(long long long long long)
@ stub FreeImageColorMatcher
# @ stub Gdi32DllInitialize
# @ stub GdiAddFontResourceW
# @ stub GdiAddGlsBounds
# @ stub GdiAddGlsRecord
# @ stub GdiAddInitialFonts
@ stdcall GdiAlphaBlend(long long long long long long long long long long long)
# @ stub GdiArtificialDecrementDriver
@ stub GdiAssociateObject
# @ stub GdiBatchLimit
@ stub GdiCleanCacheDC
@ stdcall GdiComment(long long ptr)
# @ stub GdiConsoleTextOut
@ stub GdiConvertAndCheckDC
@ stub GdiConvertBitmap
# @ stub GdiConvertBitmapV5
@ stub GdiConvertBrush
@ stub GdiConvertDC
@ stub GdiConvertEnhMetaFile
@ stub GdiConvertFont
@ stub GdiConvertMetaFilePict
@ stub GdiConvertPalette
@ stub GdiConvertRegion
@ stdcall GdiConvertToDevmodeW(ptr)
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
@ stdcall GdiDeleteSpoolFileHandle(ptr)
@ stdcall GdiDescribePixelFormat(long long long ptr) NtGdiDescribePixelFormat
# @ stub GdiDisableUMPDSandboxing
@ stdcall GdiDllInitialize(ptr long ptr)
@ stdcall GdiDrawStream(long long ptr) NtGdiDrawStream
# @ stub GdiEndDocEMF
# @ stub GdiEndPageEMF
# @ stub GdiEntry1
# @ stub GdiEntry10
# @ stub GdiEntry11
# @ stub GdiEntry12
@ stdcall GdiEntry13()
# @ stub GdiEntry14
# @ stub GdiEntry15
# @ stub GdiEntry16
# @ stub GdiEntry2
# @ stub GdiEntry3
# @ stub GdiEntry4
# @ stub GdiEntry5
# @ stub GdiEntry6
# @ stub GdiEntry7
# @ stub GdiEntry8
# @ stub GdiEntry9
# @ stub GdiFixUpHandle
@ stdcall GdiFlush() NtGdiFlush
# @ stub GdiFullscreenControl
@ stdcall GdiGetBatchLimit()
# @ stub GdiGetBitmapBitsSize
@ stdcall GdiGetCharDimensions(long ptr ptr)
@ stdcall GdiGetCodePage(long)
# @ stub GdiGetDC
@ stdcall GdiGetDevmodeForPage(ptr long ptr ptr)
@ stub GdiGetLocalBitmap
@ stub GdiGetLocalBrush
@ stub GdiGetLocalDC
@ stub GdiGetLocalFont
# @ stub GdiGetPageCount
# @ stub GdiGetPageHandle
@ stdcall GdiGetSpoolFileHandle(wstr ptr wstr)
@ stdcall GdiGetSpoolMessage(ptr long ptr long) NtGdiGetSpoolMessage
# @ stub GdiGetVariationStoreDelta
@ stdcall GdiGradientFill(long ptr long ptr long long) 
# @ stub GdiHandleBeingTracked
@ stdcall GdiInitSpool() NtGdiInitSpool
@ stdcall GdiInitializeLanguagePack(long)
@ stdcall GdiIsMetaFileDC(long)
@ stdcall GdiIsMetaPrintDC(long)
@ stdcall GdiIsPlayMetafileDC(long)
# @ stub GdiIsScreenDC
# @ stub GdiIsUMPDSandboxingEnabled
# @ stub GdiLoadType1Fonts
@ stub GdiPlayDCScript
# @ stub GdiPlayEMF
@ stub GdiPlayJournal
# @ stub GdiPlayPageEMF
# @ stub GdiPlayPrivatePageEMF
@ stub GdiPlayScript
# @ stub GdiPrinterThunk
# @ stub GdiProcessSetup
# @ stub GdiQueryFonts
# @ stub GdiQueryTable
@ stdcall GdiRealizationInfo(long ptr)
# @ stub GdiReleaseDC
@ stub GdiReleaseLocalDC
# @ stub GdiResetDCEMF
@ stub GdiSetAttrs
@ stdcall GdiSetBatchLimit(long)
# @ stub GdiSetLastError
@ stdcall GdiSetPixelFormat(long long ptr)
@ stub GdiSetServerAttr
# @ stub GdiStartDocEMF
# @ stub GdiStartPageEMF
# @ stub GdiSupportsFontChangeEvent
@ stdcall GdiSwapBuffers(long) NtGdiSwapBuffers
# @ stub GdiTrackHCreate
# @ stub GdiTrackHDelete
@ stdcall GdiTransparentBlt(long long long long long long long long long long long)
# @ stub GdiValidateHandle
@ stub GdiWinWatchClose
@ stub GdiWinWatchDidStatusChange
@ stub GdiWinWatchGetClipList
@ stub GdiWinWatchOpen
@ stdcall GetArcDirection(long)
@ stdcall GetAspectRatioFilterEx(long ptr)
# @ stub GetBitmapAttributes
@ stdcall GetBitmapBits(long long ptr) NtGdiGetBitmapBits
@ stdcall GetBitmapDimensionEx(long ptr) NtGdiGetBitmapDimension
# @ stub GetBitmapDpiScaleValue
@ stdcall GetBkColor(long)
@ stdcall GetBkMode(long)
@ stdcall GetBoundsRect(long ptr long) NtGdiGetBoundsRect
# @ stub GetBrushAttributes
@ stdcall GetBrushOrgEx(long ptr)
# @ stub GetCOPPCompatibleOPMInformation
# @ stub GetCertificate
# @ stub GetCertificateByHandle
# @ stub GetCertificateSize
# @ stub GetCertificateSizeByHandle
@ stdcall GetCharABCWidthsA(long long long ptr)
@ stdcall GetCharABCWidthsFloatA(long long long ptr)
# @ stub GetCharABCWidthsFloatI
@ stdcall GetCharABCWidthsFloatW(long long long ptr)
@ stdcall GetCharABCWidthsI(long long long ptr ptr)
@ stdcall GetCharABCWidthsW(long long long ptr)
@ stdcall GetCharWidth32A(long long long ptr)
@ stdcall GetCharWidth32W(long long long ptr)
@ stdcall GetCharWidthA(long long long ptr) GetCharWidth32A
@ stdcall GetCharWidthFloatA(long long long ptr)
@ stdcall GetCharWidthFloatW(long long long ptr)
@ stdcall GetCharWidthI(ptr long long ptr ptr)
@ stdcall GetCharWidthInfo(ptr ptr) NtGdiGetCharWidthInfo
@ stdcall GetCharWidthW(long long long ptr) GetCharWidth32W
@ stub GetCharWidthWOW
@ stdcall GetCharacterPlacementA(long str long long ptr long)
@ stdcall GetCharacterPlacementW(long wstr long long ptr long)
@ stdcall GetClipBox(long ptr) NtGdiGetAppClipBox
@ stdcall GetClipRgn(long long)
@ stdcall GetColorAdjustment(long ptr) NtGdiGetColorAdjustment
@ stdcall GetColorSpace(long)
# @ stub GetCurrentDpiInfo
@ stdcall GetCurrentObject(long long)
@ stdcall GetCurrentPositionEx(long ptr)
@ stdcall GetDCBrushColor(long)
# @ stub GetDCDpiScaleValue
@ stdcall GetDCOrgEx(long ptr)
@ stdcall GetDCPenColor(long)
@ stdcall GetDIBColorTable(long long long ptr)
@ stdcall GetDIBits(long long long long ptr ptr long)
@ stdcall GetDeviceCaps(long long)
@ stdcall GetDeviceGammaRamp(long ptr) NtGdiGetDeviceGammaRamp
@ stub GetETM
# @ stub GetEUDCTimeStamp
# @ stub GetEUDCTimeStampExW
@ stdcall GetEnhMetaFileA(str)
@ stdcall GetEnhMetaFileBits(long long ptr)
@ stdcall GetEnhMetaFileDescriptionA(long long ptr)
@ stdcall GetEnhMetaFileDescriptionW(long long ptr)
@ stdcall GetEnhMetaFileHeader(long long ptr)
@ stdcall GetEnhMetaFilePaletteEntries(long long ptr)
# @ stub GetEnhMetaFilePixelFormat
@ stdcall GetEnhMetaFileW(wstr)
# @ stub GetFontAssocStatus
@ stdcall GetFontData(long long long ptr long) NtGdiGetFontData
@ stdcall GetFontFileData(long long int64 ptr long)
@ stdcall GetFontFileInfo(long long ptr long ptr) NtGdiGetFontFileInfo
@ stdcall GetFontLanguageInfo(long)
@ stdcall GetFontRealizationInfo(long ptr) NtGdiGetRealizationInfo
@ stub GetFontResourceInfo
@ stdcall GetFontResourceInfoW(wstr ptr ptr long)
@ stdcall GetFontUnicodeRanges(ptr ptr) NtGdiGetFontUnicodeRanges
@ stdcall GetGlyphIndicesA(long ptr long ptr long)
@ stdcall GetGlyphIndicesW(long ptr long ptr long) NtGdiGetGlyphIndicesW
@ stdcall GetGlyphOutline(long long long ptr long ptr ptr) GetGlyphOutlineA
@ stdcall GetGlyphOutlineA(long long long ptr long ptr ptr)
@ stdcall GetGlyphOutlineW(long long long ptr long ptr ptr)
@ stub GetGlyphOutlineWow
@ stdcall GetGraphicsMode(long)
# @ stub GetHFONT
@ stdcall GetICMProfileA(long ptr ptr)
@ stdcall GetICMProfileW(long ptr ptr)
@ stdcall GetKerningPairs(long long ptr) GetKerningPairsA
@ stdcall GetKerningPairsA(long long ptr)
@ stdcall GetKerningPairsW(long long ptr) NtGdiGetKerningPairs
@ stdcall GetLayout(long)
@ stdcall GetLogColorSpaceA(long ptr long)
@ stdcall GetLogColorSpaceW(long ptr long)
@ stdcall GetMapMode(long)
@ stdcall GetMetaFileA(str)
@ stdcall GetMetaFileBitsEx(long long ptr)
@ stdcall GetMetaFileW(wstr)
@ stdcall GetMetaRgn(long long)
@ stdcall GetMiterLimit(long ptr) NtGdiGetMiterLimit
@ stdcall GetNearestColor(long long) NtGdiGetNearestColor
@ stdcall GetNearestPaletteIndex(long long) NtGdiGetNearestPaletteIndex
# @ stub GetNumberOfPhysicalMonitors
# @ stub GetOPMInformation
# @ stub GetOPMRandomNumber
@ stdcall GetObjectA(long long ptr)
@ stdcall GetObjectType(long)
@ stdcall GetObjectW(long long ptr)
@ stdcall GetOutlineTextMetricsA(long long ptr)
@ stdcall GetOutlineTextMetricsW(long long ptr)
@ stdcall GetPaletteEntries(long long long ptr)
@ stdcall GetPath(long ptr ptr long) NtGdiGetPath
# @ stub GetPhysicalMonitorDescription
# @ stub GetPhysicalMonitorFromTargetInternal
# @ stub GetPhysicalMonitors
@ stdcall GetPixel(long long long) NtGdiGetPixel
@ stdcall GetPixelFormat(long)
@ stdcall GetPolyFillMode(long)
# @ stub GetProcessSessionFonts
@ stdcall GetROP2(long)
@ stdcall GetRandomRgn(long long long) NtGdiGetRandomRgn
@ stdcall GetRasterizerCaps(ptr long) NtGdiGetRasterizerCaps
@ stdcall GetRegionData(long long ptr) NtGdiGetRegionData
@ stdcall GetRelAbs(long long)
@ stdcall GetRgnBox(long ptr) NtGdiGetRgnBox
@ stdcall GetStockObject(long)
@ stdcall GetStretchBltMode(long)
# @ stub GetStringBitmapA
# @ stub GetStringBitmapW
# @ stub GetSuggestedOPMProtectedOutputArraySize
@ stdcall GetSystemPaletteEntries(long long long ptr)
@ stdcall GetSystemPaletteUse(long) NtGdiGetSystemPaletteUse
@ stdcall GetTextAlign(long)
@ stdcall GetTextCharacterExtra(long)
@ stdcall GetTextCharset(long)
@ stdcall GetTextCharsetInfo(long ptr long) NtGdiGetTextCharsetInfo
@ stdcall GetTextColor(long)
@ stdcall GetTextExtentExPointA(long str long long ptr ptr ptr)
@ stdcall GetTextExtentExPointI(long ptr long long ptr ptr ptr)
@ stdcall GetTextExtentExPointW(long wstr long long ptr ptr ptr)
# @ stub GetTextExtentExPointWPri
@ stdcall GetTextExtentPoint32A(long str long ptr)
@ stdcall GetTextExtentPoint32W(long wstr long ptr)
@ stdcall GetTextExtentPointA(long str long ptr)
@ stdcall GetTextExtentPointI(long ptr long ptr)
@ stdcall GetTextExtentPointW(long wstr long ptr)
@ stdcall GetTextFaceA(long long ptr)
# @ stub GetTextFaceAliasW
@ stdcall GetTextFaceW(long long ptr)
@ stdcall GetTextMetricsA(long ptr)
@ stdcall GetTextMetricsW(long ptr)
@ stdcall GetTransform(long long ptr) NtGdiGetTransform
@ stdcall GetViewportExtEx(long ptr)
@ stdcall GetViewportOrgEx(long ptr)
@ stdcall GetWinMetaFileBits(long long ptr long long)
@ stdcall GetWindowExtEx(long ptr)
@ stdcall GetWindowOrgEx(long ptr)
@ stdcall GetWorldTransform(long ptr)
@ stdcall IntersectClipRect(long long long long long)
@ stdcall InvertRgn(long long)
# @ stub IsValidEnhMetaRecord
# @ stub IsValidEnhMetaRecordOffExt
@ stdcall LPtoDP(long ptr long)
@ stdcall LineDDA(long long long long ptr long)
@ stdcall LineTo(long long long)
@ stub LoadImageColorMatcherA
@ stub LoadImageColorMatcherW
@ stdcall MaskBlt(long long long long long long long long long long long long)
@ stdcall MirrorRgn(long long)
@ stdcall ModifyWorldTransform(long ptr long)
@ stdcall MoveToEx(long long long ptr)
@ stdcall NamedEscape(long wstr long long ptr long ptr)
@ stdcall OffsetClipRgn(long long long)
@ stdcall OffsetRgn(long long long) NtGdiOffsetRgn
@ stdcall OffsetViewportOrgEx(long long long ptr)
@ stdcall OffsetWindowOrgEx(long long long ptr)
@ stdcall PaintRgn(long long)
@ stdcall PatBlt(long long long long long long)
@ stdcall PathToRegion(long) NtGdiPathToRegion
@ stdcall Pie(long long long long long long long long long)
@ stdcall PlayEnhMetaFile(long long ptr)
@ stdcall PlayEnhMetaFileRecord(long ptr ptr long)
@ stdcall PlayMetaFile(long long)
@ stdcall PlayMetaFileRecord(long ptr ptr long)
@ stdcall PlgBlt(long ptr long long long long long long long long)
@ stdcall PolyBezier(long ptr long)
@ stdcall PolyBezierTo(long ptr long)
@ stdcall PolyDraw(long ptr ptr long)
# @ stub PolyPatBlt
@ stdcall PolyPolygon(long ptr ptr long)
@ stdcall PolyPolyline(long ptr ptr long)
@ stdcall PolyTextOutA(long ptr long)
@ stdcall PolyTextOutW(long ptr long)
@ stdcall Polygon(long ptr long)
@ stdcall Polyline(long ptr long)
@ stdcall PolylineTo(long ptr long)
@ stdcall PtInRegion(long long long) NtGdiPtInRegion
@ stdcall PtVisible(long long long) NtGdiPtVisible
# @ stub QueryFontAssocStatus
@ stdcall RealizePalette(long)
@ stdcall RectInRegion(long ptr) NtGdiRectInRegion
@ stdcall RectVisible(long ptr) NtGdiRectVisible
@ stdcall Rectangle(long long long long long)
@ stdcall RemoveFontMemResourceEx(ptr) NtGdiRemoveFontMemResourceEx
@ stdcall RemoveFontResourceA(str)
@ stdcall RemoveFontResourceExA(str long ptr)
@ stdcall RemoveFontResourceExW(wstr long ptr)
@ stub RemoveFontResourceTracking
@ stdcall RemoveFontResourceW(wstr)
@ stdcall ResetDCA(long ptr)
@ stdcall ResetDCW(long ptr)
@ stdcall ResizePalette(long long) NtGdiResizePalette
@ stdcall RestoreDC(long long)
@ stdcall RoundRect(long long long long long long long)
@ stdcall SaveDC(long)
# @ stub ScaleRgn
# @ stub ScaleValues
@ stdcall ScaleViewportExtEx(long long long long long ptr)
@ stdcall ScaleWindowExtEx(long long long long long ptr)
@ stdcall ScriptApplyDigitSubstitution(ptr ptr ptr)
@ stdcall ScriptApplyLogicalWidth(ptr long long ptr ptr ptr ptr ptr ptr)
@ stdcall ScriptBreak(ptr long ptr ptr)
@ stdcall ScriptCPtoX(long long long long ptr ptr ptr ptr ptr)
@ stdcall ScriptCacheGetHeight(ptr ptr ptr)
@ stdcall ScriptFreeCache(ptr)
@ stdcall ScriptGetCMap(ptr ptr ptr long long ptr)
@ stdcall ScriptGetFontAlternateGlyphs(long ptr ptr long long long long long ptr ptr)
@ stdcall ScriptGetFontFeatureTags(long ptr ptr long long long ptr ptr)
@ stdcall ScriptGetFontLanguageTags(long ptr ptr long long ptr ptr)
@ stdcall ScriptGetFontProperties(long ptr ptr)
@ stdcall ScriptGetFontScriptTags(long ptr ptr long ptr ptr)
@ stdcall ScriptGetGlyphABCWidth(ptr ptr long ptr)
@ stdcall ScriptGetLogicalWidths(ptr long long ptr ptr ptr ptr)
@ stdcall ScriptGetProperties(ptr ptr)
@ stdcall ScriptIsComplex(wstr long long)
@ stdcall ScriptItemize(wstr long long ptr ptr ptr ptr)
@ stdcall ScriptItemizeOpenType(wstr long long ptr ptr ptr ptr ptr)
@ stdcall ScriptJustify(ptr ptr long long long ptr)
@ stdcall ScriptLayout(long ptr ptr ptr)
@ stdcall ScriptPlace(ptr ptr ptr long ptr ptr ptr ptr ptr)
@ stdcall ScriptPlaceOpenType(ptr ptr ptr long long ptr ptr long wstr ptr ptr long ptr ptr long ptr ptr ptr)
# @ stub ScriptPositionSingleGlyph
@ stdcall ScriptRecordDigitSubstitution(long ptr)
@ stdcall ScriptShape(ptr ptr ptr long long ptr ptr ptr ptr ptr)
@ stdcall ScriptShapeOpenType(ptr ptr ptr long long ptr ptr long wstr long long ptr ptr ptr ptr ptr)
@ stdcall ScriptStringAnalyse(ptr ptr long long long long long ptr ptr ptr ptr ptr ptr)
@ stdcall ScriptStringCPtoX(ptr long long ptr)
@ stdcall ScriptStringFree(ptr)
@ stdcall ScriptStringGetLogicalWidths(ptr ptr)
@ stdcall ScriptStringGetOrder(ptr ptr)
@ stdcall ScriptStringOut(ptr long long long ptr long long long)
@ stdcall ScriptStringValidate(ptr)
@ stdcall ScriptStringXtoCP(ptr long ptr ptr)
@ stdcall ScriptString_pLogAttr(ptr)
@ stdcall ScriptString_pSize(ptr)
@ stdcall ScriptString_pcOutChars(ptr)
# @ stub ScriptSubstituteSingleGlyph
@ stdcall ScriptTextOut(ptr ptr long long long ptr ptr ptr long ptr long ptr ptr ptr)
@ stdcall ScriptXtoCP(long long long ptr ptr ptr ptr ptr ptr)
@ stub SelectBrushLocal
@ stdcall SelectClipPath(long long)
@ stdcall SelectClipRgn(long long)
@ stub SelectFontLocal
@ stdcall SelectObject(long long)
@ stdcall SelectPalette(long long long)
@ stdcall SetAbortProc(long ptr)
@ stdcall SetArcDirection(long long)
# @ stub SetBitmapAttributes
@ stdcall SetBitmapBits(long long ptr) NtGdiSetBitmapBits
@ stdcall SetBitmapDimensionEx(long long long ptr) NtGdiSetBitmapDimension
@ stdcall SetBkColor(long long)
@ stdcall SetBkMode(long long)
@ stdcall SetBoundsRect(long ptr long) NtGdiSetBoundsRect
# @ stub SetBrushAttributes
@ stdcall SetBrushOrgEx(long long long ptr)
@ stdcall SetColorAdjustment(long ptr) NtGdiSetColorAdjustment
@ stdcall SetColorSpace(long long)
@ stdcall SetDCBrushColor(long long)
@ stdcall SetDCPenColor(long long)
@ stdcall SetDIBColorTable(long long long ptr)
@ stdcall SetDIBits(long long long long ptr ptr long)
@ stdcall SetDIBitsToDevice(long long long long long long long long long ptr ptr long)
@ stdcall SetDeviceGammaRamp(long ptr) NtGdiSetDeviceGammaRamp
@ stdcall SetEnhMetaFileBits(long ptr)
@ stub SetFontEnumeration
@ stdcall SetGraphicsMode(long long)
@ stdcall SetICMMode(long long)
@ stdcall SetICMProfileA(long str)
@ stdcall SetICMProfileW(long wstr)
@ stdcall SetLayout(long long)
# @ stub SetLayoutWidth
@ stdcall SetMagicColors(ptr long long) NtGdiSetMagicColors
@ stdcall SetMapMode(long long)
@ stdcall SetMapperFlags(long long)
@ stdcall SetMetaFileBitsEx(long ptr)
@ stdcall SetMetaRgn(long)
@ stdcall SetMiterLimit(long float ptr)
# @ stub SetOPMSigningKeyAndSequenceNumbers
@ stdcall SetObjectOwner(long long)
@ stdcall SetPaletteEntries(long long long ptr)
@ stdcall SetPixel(long long long long)
@ stdcall SetPixelFormat(long long ptr)
@ stdcall SetPixelV(long long long long)
@ stdcall SetPolyFillMode(long long)
@ stdcall SetROP2(long long)
@ stdcall SetRectRgn(long long long long long) NtGdiSetRectRgn
@ stdcall SetRelAbs(long long)
@ stdcall SetStretchBltMode(long long)
@ stdcall SetSystemPaletteUse(long long) NtGdiSetSystemPaletteUse
@ stdcall SetTextAlign(long long)
@ stdcall SetTextCharacterExtra(long long)
@ stdcall SetTextColor(long long)
@ stdcall SetTextJustification(long long long)
@ stdcall SetViewportExtEx(long long long ptr)
@ stdcall SetViewportOrgEx(long long long ptr)
@ stdcall SetVirtualResolution(long long long long long) NtGdiSetVirtualResolution
@ stdcall SetWinMetaFileBits(long ptr long ptr)
@ stdcall SetWindowExtEx(long long long ptr)
@ stdcall SetWindowOrgEx(long long long ptr)
@ stdcall SetWorldTransform(long ptr)
@ stdcall StartDocA(long ptr)
@ stdcall StartDocW(long ptr)
# @ stub StartFormPage
@ stdcall StartPage(long)
@ stdcall StretchBlt(long long long long long long long long long long long)
@ stdcall StretchDIBits(long long long long long long long long long ptr ptr long long)
@ stdcall StrokeAndFillPath(long)
@ stdcall StrokePath(long)
@ stdcall SwapBuffers(long)
@ stdcall TextOutA(long long long str long)
@ stdcall TextOutW(long long long wstr long)
@ stdcall TranslateCharsetInfo(ptr ptr long)
@ stub UnloadNetworkFonts
@ stdcall UnrealizeObject(long) NtGdiUnrealizeObject
@ stdcall UpdateColors(long) NtGdiUpdateColors
@ stdcall UpdateICMRegKey(long str str long) UpdateICMRegKeyA
@ stdcall UpdateICMRegKeyA(long str str long)
@ stdcall UpdateICMRegKeyW(long wstr wstr long)
@ stdcall WidenPath(long)
@ stub gdiPlaySpoolStream
@ extern pfnRealizePalette
@ extern pfnSelectPalette
@ stub pstackConnect
