@ extern -arch=win32 Wow64Transition __wine_syscall_dispatcher
@ stub NtBindCompositionSurface
@ stub NtCloseCompositionInputSink
@ stub NtCompositionInputThread
@ stub NtCompositionSetDropTarget
@ stub NtCompositorNotifyExitWindows
@ stub NtCompositorNotifyForegroundChanged
@ stub NtCompositorUpdateLastInputTime
@ stub NtConfigureInputSpace
@ stub NtCreateCompositionInputSink
@ stub NtCreateCompositionSurfaceHandle
@ stub NtCreateImplicitCompositionInputSink
@ stub NtDCompositionAddCrossDeviceVisualChild
@ stub NtDCompositionBeginFrame
@ stub NtDCompositionCommitChannel
@ stub NtDCompositionCommitSynchronizationObject
@ stub NtDCompositionConfirmFrame
@ stub NtDCompositionConnectPipe
@ stub NtDCompositionCreateAndBindSharedSection
@ stub NtDCompositionCreateChannel
@ stub NtDCompositionCreateConnection
@ stub NtDCompositionCreateDwmChannel
@ stub NtDCompositionCreateSharedResourceHandle
@ stub NtDCompositionCreateSynchronizationObject
@ stub NtDCompositionDestroyChannel
@ stub NtDCompositionDestroyConnection
@ stub NtDCompositionDiscardFrame
@ stub NtDCompositionDuplicateHandleToProcess
@ stub NtDCompositionDuplicateSwapchainHandleToDwm
@ stub NtDCompositionEnableMMCSS
@ stub NtDCompositionGetBatchId
@ stub NtDCompositionGetChannels
@ stub NtDCompositionGetConnectionBatch
@ stub NtDCompositionGetDeletedResources
@ stub NtDCompositionGetFrameLegacyTokens
@ stub NtDCompositionGetFrameStatistics
@ stub NtDCompositionGetFrameSurfaceUpdates
@ stub NtDCompositionGetMaterialProperty
@ stub NtDCompositionProcessChannelBatchBuffer
@ stub NtDCompositionReferenceSharedResourceOnDwmChannel
@ stub NtDCompositionRegisterThumbnailVisual
@ stub NtDCompositionRegisterVirtualDesktopVisual
@ stub NtDCompositionReleaseAllResources
@ stub NtDCompositionRemoveCrossDeviceVisualChild
@ stub NtDCompositionRetireFrame
@ stub NtDCompositionSetChannelCommitCompletionEvent
@ stub NtDCompositionSetChannelConnectionId
@ stub NtDCompositionSetChildRootVisual
@ stub NtDCompositionSetDebugCounter
@ stub NtDCompositionSetMaterialProperty
@ stub NtDCompositionSubmitDWMBatch
@ stub NtDCompositionSuspendAnimations
@ stub NtDCompositionSynchronize
@ stub NtDCompositionTelemetryAnimationScenarioBegin
@ stub NtDCompositionTelemetryAnimationScenarioReference
@ stub NtDCompositionTelemetryAnimationScenarioUnreference
@ stub NtDCompositionTelemetrySetApplicationId
@ stub NtDCompositionTelemetryTouchInteractionBegin
@ stub NtDCompositionTelemetryTouchInteractionEnd
@ stub NtDCompositionTelemetryTouchInteractionUpdate
@ stub NtDCompositionUpdatePointerCapture
@ stub NtDCompositionWaitForChannel
@ stub NtDesktopCaptureBits
@ stub NtDuplicateCompositionInputSink
@ stub NtDxgkCreateTrackedWorkload
@ stub NtDxgkDestroyTrackedWorkload
@ stub NtDxgkDispMgrOperation
@ stub NtDxgkDisplayPortOperation
@ stub NtDxgkDuplicateHandle
@ stub NtDxgkEnumAdapters3
@ stub NtDxgkGetAvailableTrackedWorkloadIndex
@ stub NtDxgkGetProcessList
@ stub NtDxgkGetTrackedWorkloadStatistics
@ stub NtDxgkOutputDuplPresentToHwQueue
@ stub NtDxgkPinResources
@ stub NtDxgkRegisterVailProcess
@ stub NtDxgkResetTrackedWorkloadStatistics
@ stub NtDxgkSubmitPresentBltToHwQueue
@ stub NtDxgkSubmitPresentToHwQueue
@ stub NtDxgkUnpinResources
@ stub NtDxgkUpdateTrackedWorkload
@ stub NtDxgkVailConnect
@ stub NtDxgkVailDisconnect
@ stub NtDxgkVailPromoteCompositionSurface
@ stub NtEnableOneCoreTransformMode
@ stub NtFlipObjectAddContent
@ stub NtFlipObjectAddPoolBuffer
@ stub NtFlipObjectConsumerAcquirePresent
@ stub NtFlipObjectConsumerAdjustUsageReference
@ stub NtFlipObjectConsumerBeginProcessPresent
@ stub NtFlipObjectConsumerEndProcessPresent
@ stub NtFlipObjectConsumerPostMessage
@ stub NtFlipObjectConsumerQueryBufferInfo
@ stub NtFlipObjectCreate
@ stub NtFlipObjectDisconnectEndpoint
@ stub NtFlipObjectEnablePresentStatisticsType
@ stub NtFlipObjectOpen
@ stub NtFlipObjectPresentCancel
@ stub NtFlipObjectQueryBufferAvailableEvent
@ stub NtFlipObjectQueryEndpointConnected
@ stub NtFlipObjectQueryNextMessageToProducer
@ stub NtFlipObjectReadNextMessageToProducer
@ stub NtFlipObjectRemoveContent
@ stub NtFlipObjectRemovePoolBuffer
@ stub NtFlipObjectSetContent
@ stub NtFlipObjectSetMaximumBackchannelQueueDepth
@ stdcall NtGdiAbortDoc(long)
@ stdcall NtGdiAbortPath(long)
@ stub NtGdiAddEmbFontToDC
@ stdcall -syscall NtGdiAddFontMemResourceEx(ptr long ptr long ptr)
@ stdcall -syscall NtGdiAddFontResourceW(wstr long long long long ptr)
@ stub NtGdiAddInitialFonts
@ stub NtGdiAddRemoteFontToDC
@ stub NtGdiAddRemoteMMInstanceToDC
@ stdcall NtGdiAlphaBlend(long long long long long long long long long long long ptr)
@ stdcall NtGdiAngleArc(long long long long float float)
@ stub NtGdiAnyLinkedFonts
@ stdcall NtGdiArcInternal(long long long long long long long long long long)
@ stub NtGdiBRUSHOBJ_DeleteRbrush
@ stub NtGdiBRUSHOBJ_hGetColorTransform
@ stub NtGdiBRUSHOBJ_pvAllocRbrush
@ stub NtGdiBRUSHOBJ_pvGetRbrush
@ stub NtGdiBRUSHOBJ_ulGetBrushColor
@ stub NtGdiBeginGdiRendering
@ stdcall NtGdiBeginPath(long)
@ stdcall NtGdiBitBlt(long long long long long long long long long long float)
@ stub NtGdiCLIPOBJ_bEnum
@ stub NtGdiCLIPOBJ_cEnumStart
@ stub NtGdiCLIPOBJ_ppoGetPath
@ stub NtGdiCancelDC
@ stub NtGdiChangeGhostFont
@ stub NtGdiCheckBitmapBits
@ stub NtGdiClearBitmapAttributes
@ stub NtGdiClearBrushAttributes
@ stdcall NtGdiCloseFigure(long)
@ stub NtGdiColorCorrectPalette
@ stdcall -syscall NtGdiCombineRgn(long long long long)
@ stub NtGdiCombineTransform
@ stdcall NtGdiComputeXformCoefficients(long)
@ stub NtGdiConfigureOPMProtectedOutput
@ stub NtGdiConvertMetafileRect
@ stdcall -syscall NtGdiCreateBitmap(long long long long ptr)
@ stub NtGdiCreateBitmapFromDxSurface
@ stub NtGdiCreateBitmapFromDxSurface2
@ stdcall -syscall NtGdiCreateClientObj(long)
@ stub NtGdiCreateColorSpace
@ stub NtGdiCreateColorTransform
@ stdcall NtGdiCreateCompatibleBitmap(long long long)
@ stdcall NtGdiCreateCompatibleDC(long)
@ stdcall -syscall NtGdiCreateDIBBrush(ptr long long long long ptr)
@ stdcall -syscall NtGdiCreateDIBSection(long long long ptr long long long long ptr)
@ stdcall NtGdiCreateDIBitmapInternal(long long long long ptr ptr long long long long long)
@ stdcall -syscall NtGdiCreateEllipticRgn(long long long long)
@ stdcall -syscall NtGdiCreateHalftonePalette(long)
@ stdcall -syscall NtGdiCreateHatchBrushInternal(long long long)
@ stdcall NtGdiCreateMetafileDC(long)
@ stub NtGdiCreateOPMProtectedOutput
@ stub NtGdiCreateOPMProtectedOutputs
@ stdcall -syscall NtGdiCreatePaletteInternal(ptr long)
@ stdcall -syscall NtGdiCreatePatternBrushInternal(long long long)
@ stdcall -syscall NtGdiCreatePen(long long long long)
@ stdcall -syscall NtGdiCreateRectRgn(long long long long)
@ stdcall -syscall NtGdiCreateRoundRectRgn(long long long long long long)
@ stub NtGdiCreateServerMetaFile
@ stub NtGdiCreateSessionMappedDIBSection
@ stdcall -syscall NtGdiCreateSolidBrush(long long)
@ stub NtGdiDDCCIGetCapabilitiesString
@ stub NtGdiDDCCIGetCapabilitiesStringLength
@ stub NtGdiDDCCIGetTimingReport
@ stub NtGdiDDCCIGetVCPFeature
@ stub NtGdiDDCCISaveCurrentSettings
@ stub NtGdiDDCCISetVCPFeature
@ stub NtGdiDdCreateFullscreenSprite
@ stub NtGdiDdDDIAbandonSwapChain
@ stub NtGdiDdDDIAcquireKeyedMutex
@ stub NtGdiDdDDIAcquireKeyedMutex2
@ stub NtGdiDdDDIAcquireSwapChain
@ stub NtGdiDdDDIAddSurfaceToSwapChain
@ stub NtGdiDdDDIAdjustFullscreenGamma
@ stub NtGdiDdDDICacheHybridQueryValue
@ stub NtGdiDdDDIChangeVideoMemoryReservation
@ stub NtGdiDdDDICheckExclusiveOwnership
@ stub NtGdiDdDDICheckMonitorPowerState
@ stub NtGdiDdDDICheckMultiPlaneOverlaySupport
@ stub NtGdiDdDDICheckMultiPlaneOverlaySupport2
@ stub NtGdiDdDDICheckMultiPlaneOverlaySupport3
@ stub NtGdiDdDDICheckOcclusion
@ stub NtGdiDdDDICheckSharedResourceAccess
@ stdcall NtGdiDdDDICheckVidPnExclusiveOwnership(ptr)
@ stdcall -syscall NtGdiDdDDICloseAdapter(ptr)
@ stub NtGdiDdDDIConfigureSharedResource
@ stub NtGdiDdDDICreateAllocation
@ stub NtGdiDdDDICreateBundleObject
@ stub NtGdiDdDDICreateContext
@ stub NtGdiDdDDICreateContextVirtual
@ stdcall NtGdiDdDDICreateDCFromMemory(ptr)
@ stdcall -syscall NtGdiDdDDICreateDevice(ptr)
@ stub NtGdiDdDDICreateHwContext
@ stub NtGdiDdDDICreateHwQueue
@ stub NtGdiDdDDICreateKeyedMutex
@ stub NtGdiDdDDICreateKeyedMutex2
@ stub NtGdiDdDDICreateOutputDupl
@ stub NtGdiDdDDICreateOverlay
@ stub NtGdiDdDDICreatePagingQueue
@ stub NtGdiDdDDICreateProtectedSession
@ stub NtGdiDdDDICreateSwapChain
@ stub NtGdiDdDDICreateSynchronizationObject
@ stub NtGdiDdDDIDDisplayEnum
@ stub NtGdiDdDDIDestroyAllocation
@ stub NtGdiDdDDIDestroyAllocation2
@ stub NtGdiDdDDIDestroyContext
@ stdcall NtGdiDdDDIDestroyDCFromMemory(ptr)
@ stdcall NtGdiDdDDIDestroyDevice(ptr)
@ stub NtGdiDdDDIDestroyHwContext
@ stub NtGdiDdDDIDestroyHwQueue
@ stub NtGdiDdDDIDestroyKeyedMutex
@ stub NtGdiDdDDIDestroyOutputDupl
@ stub NtGdiDdDDIDestroyOverlay
@ stub NtGdiDdDDIDestroyPagingQueue
@ stub NtGdiDdDDIDestroyProtectedSession
@ stub NtGdiDdDDIDestroySynchronizationObject
@ stub NtGdiDdDDIDispMgrCreate
@ stub NtGdiDdDDIDispMgrSourceOperation
@ stub NtGdiDdDDIDispMgrTargetOperation
@ stub NtGdiDdDDIEnumAdapters
@ stub NtGdiDdDDIEnumAdapters2
@ stdcall NtGdiDdDDIEscape(ptr)
@ stub NtGdiDdDDIEvict
@ stub NtGdiDdDDIExtractBundleObject
@ stub NtGdiDdDDIFlipOverlay
@ stub NtGdiDdDDIFlushHeapTransitions
@ stub NtGdiDdDDIFreeGpuVirtualAddress
@ stub NtGdiDdDDIGetAllocationPriority
@ stub NtGdiDdDDIGetCachedHybridQueryValue
@ stub NtGdiDdDDIGetContextInProcessSchedulingPriority
@ stub NtGdiDdDDIGetContextSchedulingPriority
@ stub NtGdiDdDDIGetDWMVerticalBlankEvent
@ stub NtGdiDdDDIGetDeviceState
@ stub NtGdiDdDDIGetDisplayModeList
@ stub NtGdiDdDDIGetMemoryBudgetTarget
@ stub NtGdiDdDDIGetMultiPlaneOverlayCaps
@ stub NtGdiDdDDIGetMultisampleMethodList
@ stub NtGdiDdDDIGetOverlayState
@ stub NtGdiDdDDIGetPostCompositionCaps
@ stub NtGdiDdDDIGetPresentHistory
@ stub NtGdiDdDDIGetPresentQueueEvent
@ stub NtGdiDdDDIGetProcessDeviceRemovalSupport
@ stub NtGdiDdDDIGetProcessSchedulingPriorityBand
@ stub NtGdiDdDDIGetProcessSchedulingPriorityClass
@ stub NtGdiDdDDIGetResourcePresentPrivateDriverData
@ stub NtGdiDdDDIGetRuntimeData
@ stub NtGdiDdDDIGetScanLine
@ stub NtGdiDdDDIGetSetSwapChainMetadata
@ stub NtGdiDdDDIGetSharedPrimaryHandle
@ stub NtGdiDdDDIGetSharedResourceAdapterLuid
@ stub NtGdiDdDDIGetSharedResourceAdapterLuidFlipManager
@ stub NtGdiDdDDIGetYieldPercentage
@ stub NtGdiDdDDIInvalidateActiveVidPn
@ stub NtGdiDdDDIInvalidateCache
@ stub NtGdiDdDDILock
@ stub NtGdiDdDDILock2
@ stub NtGdiDdDDIMakeResident
@ stub NtGdiDdDDIMapGpuVirtualAddress
@ stub NtGdiDdDDIMarkDeviceAsError
@ stub NtGdiDdDDINetDispGetNextChunkInfo
@ stub NtGdiDdDDINetDispQueryMiracastDisplayDeviceStatus
@ stub NtGdiDdDDINetDispQueryMiracastDisplayDeviceSupport
@ stub NtGdiDdDDINetDispStartMiracastDisplayDevice
@ stub NtGdiDdDDINetDispStopMiracastDisplayDevice
@ stub NtGdiDdDDIOfferAllocations
@ stdcall -syscall NtGdiDdDDIOpenAdapterFromDeviceName(ptr)
@ stdcall -syscall NtGdiDdDDIOpenAdapterFromHdc(ptr)
@ stdcall -syscall NtGdiDdDDIOpenAdapterFromLuid(ptr)
@ stub NtGdiDdDDIOpenBundleObjectNtHandleFromName
@ stub NtGdiDdDDIOpenKeyedMutex
@ stub NtGdiDdDDIOpenKeyedMutex2
@ stub NtGdiDdDDIOpenKeyedMutexFromNtHandle
@ stub NtGdiDdDDIOpenNtHandleFromName
@ stub NtGdiDdDDIOpenProtectedSessionFromNtHandle
@ stub NtGdiDdDDIOpenResource
@ stub NtGdiDdDDIOpenResourceFromNtHandle
@ stub NtGdiDdDDIOpenSwapChain
@ stub NtGdiDdDDIOpenSyncObjectFromNtHandle
@ stub NtGdiDdDDIOpenSyncObjectFromNtHandle2
@ stub NtGdiDdDDIOpenSyncObjectNtHandleFromName
@ stub NtGdiDdDDIOpenSynchronizationObject
@ stub NtGdiDdDDIOutputDuplGetFrameInfo
@ stub NtGdiDdDDIOutputDuplGetMetaData
@ stub NtGdiDdDDIOutputDuplGetPointerShapeData
@ stub NtGdiDdDDIOutputDuplPresent
@ stub NtGdiDdDDIOutputDuplReleaseFrame
@ stub NtGdiDdDDIPollDisplayChildren
@ stub NtGdiDdDDIPresent
@ stub NtGdiDdDDIPresentMultiPlaneOverlay
@ stub NtGdiDdDDIPresentMultiPlaneOverlay2
@ stub NtGdiDdDDIPresentMultiPlaneOverlay3
@ stub NtGdiDdDDIPresentRedirected
@ stub NtGdiDdDDIQueryAdapterInfo
@ stub NtGdiDdDDIQueryAllocationResidency
@ stub NtGdiDdDDIQueryClockCalibration
@ stub NtGdiDdDDIQueryFSEBlock
@ stub NtGdiDdDDIQueryProcessOfferInfo
@ stub NtGdiDdDDIQueryProtectedSessionInfoFromNtHandle
@ stub NtGdiDdDDIQueryProtectedSessionStatus
@ stub NtGdiDdDDIQueryRemoteVidPnSourceFromGdiDisplayName
@ stub NtGdiDdDDIQueryResourceInfo
@ stub NtGdiDdDDIQueryResourceInfoFromNtHandle
@ stdcall -syscall NtGdiDdDDIQueryStatistics(ptr)
@ stub NtGdiDdDDIQueryVidPnExclusiveOwnership
@ stub NtGdiDdDDIQueryVideoMemoryInfo
@ stub NtGdiDdDDIReclaimAllocations
@ stub NtGdiDdDDIReclaimAllocations2
@ stub NtGdiDdDDIReleaseKeyedMutex
@ stub NtGdiDdDDIReleaseKeyedMutex2
@ stub NtGdiDdDDIReleaseProcessVidPnSourceOwners
@ stub NtGdiDdDDIReleaseSwapChain
@ stub NtGdiDdDDIRemoveSurfaceFromSwapChain
@ stub NtGdiDdDDIRender
@ stub NtGdiDdDDIReserveGpuVirtualAddress
@ stub NtGdiDdDDISetAllocationPriority
@ stub NtGdiDdDDISetContextInProcessSchedulingPriority
@ stub NtGdiDdDDISetContextSchedulingPriority
@ stub NtGdiDdDDISetDisplayMode
@ stub NtGdiDdDDISetDodIndirectSwapchain
@ stub NtGdiDdDDISetFSEBlock
@ stub NtGdiDdDDISetGammaRamp
@ stub NtGdiDdDDISetHwProtectionTeardownRecovery
@ stub NtGdiDdDDISetMemoryBudgetTarget
@ stub NtGdiDdDDISetMonitorColorSpaceTransform
@ stub NtGdiDdDDISetProcessDeviceRemovalSupport
@ stub NtGdiDdDDISetProcessSchedulingPriorityBand
@ stub NtGdiDdDDISetProcessSchedulingPriorityClass
@ stdcall -syscall NtGdiDdDDISetQueuedLimit(ptr)
@ stub NtGdiDdDDISetStablePowerState
@ stub NtGdiDdDDISetStereoEnabled
@ stub NtGdiDdDDISetSyncRefreshCountWaitTarget
@ stub NtGdiDdDDISetVidPnSourceHwProtection
@ stdcall NtGdiDdDDISetVidPnSourceOwner(ptr)
@ stub NtGdiDdDDISetYieldPercentage
@ stub NtGdiDdDDIShareObjects
@ stub NtGdiDdDDISharedPrimaryLockNotification
@ stub NtGdiDdDDISharedPrimaryUnLockNotification
@ stub NtGdiDdDDISignalSynchronizationObject
@ stub NtGdiDdDDISignalSynchronizationObjectFromCpu
@ stub NtGdiDdDDISignalSynchronizationObjectFromGpu
@ stub NtGdiDdDDISignalSynchronizationObjectFromGpu2
@ stub NtGdiDdDDISubmitCommand
@ stub NtGdiDdDDISubmitCommandToHwQueue
@ stub NtGdiDdDDISubmitSignalSyncObjectsToHwQueue
@ stub NtGdiDdDDISubmitWaitForSyncObjectsToHwQueue
@ stub NtGdiDdDDITrimProcessCommitment
@ stub NtGdiDdDDIUnOrderedPresentSwapChain
@ stub NtGdiDdDDIUnlock
@ stub NtGdiDdDDIUnlock2
@ stub NtGdiDdDDIUpdateAllocationProperty
@ stub NtGdiDdDDIUpdateGpuVirtualAddress
@ stub NtGdiDdDDIUpdateOverlay
@ stub NtGdiDdDDIWaitForIdle
@ stub NtGdiDdDDIWaitForSynchronizationObject
@ stub NtGdiDdDDIWaitForSynchronizationObjectFromCpu
@ stub NtGdiDdDDIWaitForSynchronizationObjectFromGpu
@ stub NtGdiDdDDIWaitForVerticalBlankEvent
@ stub NtGdiDdDDIWaitForVerticalBlankEvent2
@ stub NtGdiDdDestroyFullscreenSprite
@ stub NtGdiDdNotifyFullscreenSpriteUpdate
@ stub NtGdiDdQueryVisRgnUniqueness
@ stdcall -syscall NtGdiDeleteClientObj(long)
@ stub NtGdiDeleteColorSpace
@ stub NtGdiDeleteColorTransform
@ stdcall NtGdiDeleteObjectApp(long )
@ stdcall -syscall NtGdiDescribePixelFormat(long long long ptr)
@ stub NtGdiDestroyOPMProtectedOutput
@ stub NtGdiDestroyPhysicalMonitor
@ stub NtGdiDoBanding
@ stdcall NtGdiDoPalette(long long long ptr long long)
@ stub NtGdiDrawEscape
@ stdcall -syscall NtGdiDrawStream(long long ptr)
@ stub NtGdiDwmCreatedBitmapRemotingOutput
@ stdcall NtGdiEllipse(long long long long long)
@ stub NtGdiEnableEudc
@ stdcall NtGdiEndDoc(long)
@ stub NtGdiEndGdiRendering
@ stdcall NtGdiEndPage(long)
@ stdcall NtGdiEndPath(long)
@ stub NtGdiEngAlphaBlend
@ stub NtGdiEngAssociateSurface
@ stub NtGdiEngBitBlt
@ stub NtGdiEngCheckAbort
@ stub NtGdiEngComputeGlyphSet
@ stub NtGdiEngCopyBits
@ stub NtGdiEngCreateBitmap
@ stub NtGdiEngCreateClip
@ stub NtGdiEngCreateDeviceBitmap
@ stub NtGdiEngCreateDeviceSurface
@ stub NtGdiEngCreatePalette
@ stub NtGdiEngDeleteClip
@ stub NtGdiEngDeletePalette
@ stub NtGdiEngDeletePath
@ stub NtGdiEngDeleteSurface
@ stub NtGdiEngEraseSurface
@ stub NtGdiEngFillPath
@ stub NtGdiEngGradientFill
@ stub NtGdiEngLineTo
@ stub NtGdiEngLockSurface
@ stub NtGdiEngMarkBandingSurface
@ stub NtGdiEngPaint
@ stub NtGdiEngPlgBlt
@ stub NtGdiEngStretchBlt
@ stub NtGdiEngStretchBltROP
@ stub NtGdiEngStrokeAndFillPath
@ stub NtGdiEngStrokePath
@ stub NtGdiEngTextOut
@ stub NtGdiEngTransparentBlt
@ stub NtGdiEngUnlockSurface
@ stub NtGdiEnsureDpiDepDefaultGuiFontForPlateau
@ stdcall NtGdiEnumFonts(long long long long wstr long ptr ptr)
@ stub NtGdiEnumObjects
@ stdcall -syscall NtGdiEqualRgn(long long)
@ stub NtGdiEudcLoadUnloadLink
@ stdcall NtGdiExcludeClipRect(long long long long long)
@ stdcall -syscall NtGdiExtCreatePen(long long long long long long long ptr long long long)
@ stdcall -syscall NtGdiExtCreateRegion(ptr long ptr)
@ stdcall NtGdiExtEscape(long wstr long long long ptr long ptr)
@ stdcall NtGdiExtFloodFill(long long long long long)
@ stdcall -syscall NtGdiExtGetObjectW(long long ptr)
@ stdcall NtGdiExtSelectClipRgn(long long long)
@ stdcall NtGdiExtTextOutW(long long long long ptr ptr long ptr long)
@ stub NtGdiFONTOBJ_cGetAllGlyphHandles
@ stub NtGdiFONTOBJ_cGetGlyphs
@ stub NtGdiFONTOBJ_pQueryGlyphAttrs
@ stub NtGdiFONTOBJ_pfdg
@ stub NtGdiFONTOBJ_pifi
@ stub NtGdiFONTOBJ_pvTrueTypeFontFile
@ stub NtGdiFONTOBJ_pxoGetXform
@ stub NtGdiFONTOBJ_vGetInfo
@ stdcall NtGdiFillPath(long)
@ stdcall NtGdiFillRgn(long long long)
@ stdcall -syscall NtGdiFlattenPath(long)
@ stdcall -syscall NtGdiFlush()
@ stdcall NtGdiFontIsLinked(long)
@ stub NtGdiForceUFIMapping
@ stdcall NtGdiFrameRgn(long long long long long)
@ stub NtGdiFullscreenControl
@ stdcall NtGdiGetAndSetDCDword(long long long ptr)
@ stdcall NtGdiGetAppClipBox(long ptr)
@ stub NtGdiGetAppliedDeviceGammaRamp
@ stdcall -syscall NtGdiGetBitmapBits(long long ptr)
@ stdcall -syscall NtGdiGetBitmapDimension(long ptr)
@ stub NtGdiGetBitmapDpiScaleValue
@ stdcall NtGdiGetBoundsRect(long ptr long)
@ stub NtGdiGetCOPPCompatibleOPMInformation
@ stub NtGdiGetCertificate
@ stub NtGdiGetCertificateByHandle
@ stub NtGdiGetCertificateSize
@ stub NtGdiGetCertificateSizeByHandle
@ stdcall NtGdiGetCharABCWidthsW(long long long ptr long ptr)
@ stub NtGdiGetCharSet
@ stdcall NtGdiGetCharWidthInfo(long ptr)
@ stdcall NtGdiGetCharWidthW(long long long ptr long ptr)
@ stub NtGdiGetCharacterPlacementW
@ stdcall -syscall NtGdiGetColorAdjustment(long ptr)
@ stub NtGdiGetColorSpaceforBitmap
@ stub NtGdiGetCurrentDpiInfo
@ stub NtGdiGetDCDpiScaleValue
@ stub NtGdiGetDCDword
@ stdcall -syscall NtGdiGetDCObject(long long)
@ stub NtGdiGetDCPoint
@ stub NtGdiGetDCforBitmap
@ stdcall NtGdiGetDIBitsInternal(long long long long ptr ptr long long long)
@ stdcall NtGdiGetDeviceCaps(long long)
@ stub NtGdiGetDeviceCapsAll
@ stdcall NtGdiGetDeviceGammaRamp(long ptr)
@ stub NtGdiGetDeviceWidth
@ stub NtGdiGetDhpdev
@ stub NtGdiGetETM
@ stub NtGdiGetEmbUFI
@ stub NtGdiGetEmbedFonts
@ stub NtGdiGetEntry
@ stub NtGdiGetEudcTimeStampEx
@ stdcall NtGdiGetFontData(long long long ptr long)
@ stdcall -syscall NtGdiGetFontFileData(long long ptr ptr long)
@ stdcall -syscall NtGdiGetFontFileInfo(long long ptr long ptr)
@ stub NtGdiGetFontResourceInfoInternalW
@ stdcall NtGdiGetFontUnicodeRanges(long ptr)
@ stub NtGdiGetGammaRampCapability
@ stdcall NtGdiGetGlyphIndicesW(long wstr long ptr long)
@ stub NtGdiGetGlyphIndicesWInternal
@ stdcall NtGdiGetGlyphOutline(long long long ptr long ptr ptr long)
@ stdcall NtGdiGetKerningPairs(long long ptr)
@ stub NtGdiGetLinkedUFIs
@ stub NtGdiGetMiterLimit
@ stub NtGdiGetMonitorID
@ stdcall NtGdiGetNearestColor(long long)
@ stdcall -syscall NtGdiGetNearestPaletteIndex(long long)
@ stub NtGdiGetNumberOfPhysicalMonitors
@ stub NtGdiGetOPMInformation
@ stub NtGdiGetOPMRandomNumber
@ stub NtGdiGetObjectBitmapHandle
@ stdcall NtGdiGetOutlineTextMetricsInternalW(long long ptr long)
@ stdcall -syscall NtGdiGetPath(long ptr ptr long)
@ stub NtGdiGetPerBandInfo
@ stub NtGdiGetPhysicalMonitorDescription
@ stub NtGdiGetPhysicalMonitors
@ stdcall NtGdiGetPixel(long long long)
@ stub NtGdiGetProcessSessionFonts
@ stub NtGdiGetPublicFontTableChangeCookie
@ stdcall NtGdiGetRandomRgn(long long long)
@ stdcall NtGdiGetRasterizerCaps(ptr long)
@ stdcall NtGdiGetRealizationInfo(long ptr)
@ stdcall -syscall NtGdiGetRegionData(long long ptr)
@ stdcall -syscall NtGdiGetRgnBox(long ptr)
@ stub NtGdiGetServerMetaFileBits
@ stdcall -syscall NtGdiGetSpoolMessage(ptr long ptr long)
@ stub NtGdiGetStats
@ stub NtGdiGetStringBitmapW
@ stub NtGdiGetSuggestedOPMProtectedOutputArraySize
@ stdcall -syscall NtGdiGetSystemPaletteUse(long)
@ stdcall NtGdiGetTextCharsetInfo(long ptr long)
@ stub NtGdiGetTextExtent
@ stdcall NtGdiGetTextExtentExW(long wstr long long ptr ptr ptr long)
@ stdcall NtGdiGetTextFaceW(long long ptr long)
@ stdcall NtGdiGetTextMetricsW(long ptr long)
@ stdcall -syscall NtGdiGetTransform(long long ptr)
@ stub NtGdiGetUFI
@ stub NtGdiGetUFIPathname
@ stub NtGdiGetWidthTable
@ stdcall NtGdiGradientFill(long ptr long ptr long long)
@ stub NtGdiHLSurfGetInformation
@ stub NtGdiHLSurfSetInformation
@ stub NtGdiHT_Get8BPPFormatPalette
@ stub NtGdiHT_Get8BPPMaskPalette
@ stdcall -syscall NtGdiHfontCreate(ptr long long long ptr)
@ stub NtGdiIcmBrushInfo
@ stub NtGdiInit
@ stdcall -syscall NtGdiInitSpool()
@ stdcall NtGdiIntersectClipRect(long long long long long)
@ stdcall NtGdiInvertRgn(long long)
@ stdcall NtGdiLineTo(long long long)
@ stub NtGdiMakeFontDir
@ stub NtGdiMakeInfoDC
@ stub NtGdiMakeObjectUnXferable
@ stub NtGdiMakeObjectXferable
@ stdcall NtGdiMaskBlt(long long long long long long long long long long long long long)
@ stub NtGdiMirrorWindowOrg
@ stdcall NtGdiModifyWorldTransform(long ptr long)
@ stub NtGdiMonoBitmap
@ stdcall NtGdiMoveTo(long long long ptr)
@ stdcall NtGdiOffsetClipRgn(long long long)
@ stdcall -syscall NtGdiOffsetRgn(long long long)
@ stdcall NtGdiOpenDCW(ptr ptr ptr long long ptr ptr ptr)
@ stub NtGdiPATHOBJ_bEnum
@ stub NtGdiPATHOBJ_bEnumClipLines
@ stub NtGdiPATHOBJ_vEnumStart
@ stub NtGdiPATHOBJ_vEnumStartClipLines
@ stub NtGdiPATHOBJ_vGetBounds
@ stdcall NtGdiPatBlt(long long long long long long)
@ stdcall -syscall NtGdiPathToRegion(long)
@ stdcall NtGdiPlgBlt(long ptr long long long long long long long long long)
@ stdcall NtGdiPolyDraw(long ptr ptr long)
@ stub NtGdiPolyPatBlt
@ stdcall NtGdiPolyPolyDraw(long ptr ptr long long)
@ stub NtGdiPolyTextOutW
@ stdcall -syscall NtGdiPtInRegion(long long long)
@ stdcall NtGdiPtVisible(long long long)
@ stub NtGdiQueryFontAssocInfo
@ stub NtGdiQueryFonts
@ stdcall -syscall NtGdiRectInRegion(long ptr)
@ stdcall NtGdiRectVisible(long ptr)
@ stdcall NtGdiRectangle(long long long long long)
@ stdcall -syscall NtGdiRemoveFontMemResourceEx(long)
@ stdcall -syscall NtGdiRemoveFontResourceW(wstr long long long long ptr)
@ stub NtGdiRemoveMergeFont
@ stdcall NtGdiResetDC(long ptr ptr ptr ptr)
@ stdcall NtGdiResizePalette(long long)
@ stdcall NtGdiRestoreDC(long long)
@ stdcall NtGdiRoundRect(long long long long long long long)
@ stub NtGdiSTROBJ_bEnum
@ stub NtGdiSTROBJ_bEnumPositionsOnly
@ stub NtGdiSTROBJ_bGetAdvanceWidths
@ stub NtGdiSTROBJ_dwGetCodePage
@ stub NtGdiSTROBJ_vEnumStart
@ stdcall -syscall NtGdiSaveDC(long)
@ stub NtGdiScaleRgn
@ stub NtGdiScaleValues
@ stdcall NtGdiScaleViewportExtEx(long long long long long ptr)
@ stdcall NtGdiScaleWindowExtEx(long long long long long ptr)
@ stdcall NtGdiSelectBitmap(long long)
@ stdcall NtGdiSelectBrush(long long)
@ stdcall NtGdiSelectClipPath(long long)
@ stdcall NtGdiSelectFont(long long)
@ stdcall NtGdiSelectPen(long long)
@ stub NtGdiSetBitmapAttributes
@ stdcall -syscall NtGdiSetBitmapBits(long long ptr)
@ stdcall -syscall NtGdiSetBitmapDimension(long long long ptr)
@ stdcall NtGdiSetBoundsRect(long ptr long)
@ stub NtGdiSetBrushAttributes
@ stdcall -syscall NtGdiSetBrushOrg(long long long ptr)
@ stdcall -syscall NtGdiSetColorAdjustment(long ptr)
@ stub NtGdiSetColorSpace
@ stdcall NtGdiSetDIBitsToDeviceInternal(long long long long long long long long long ptr ptr long long long long long)
@ stdcall NtGdiSetDeviceGammaRamp(ptr ptr)
@ stub NtGdiSetFontEnumeration
@ stub NtGdiSetFontXform
@ stub NtGdiSetIcmMode
@ stdcall NtGdiSetLayout(long long long)
@ stub NtGdiSetLinkedUFIs
@ stdcall -syscall NtGdiSetMagicColors(long long long)
@ stdcall -syscall NtGdiSetMetaRgn(long)
@ stub NtGdiSetMiterLimit
@ stub NtGdiSetOPMSigningKeyAndSequenceNumbers
@ stub NtGdiSetPUMPDOBJ
@ stdcall NtGdiSetPixel(long long long long)
@ stdcall -syscall NtGdiSetPixelFormat(long long)
@ stub NtGdiSetPrivateDeviceGammaRamp
@ stdcall -syscall NtGdiSetRectRgn(long long long long long)
@ stub NtGdiSetSizeDevice
@ stdcall NtGdiSetSystemPaletteUse(long long)
@ stdcall -syscall NtGdiSetTextJustification(long long long)
@ stub NtGdiSetUMPDSandboxState
@ stdcall -syscall NtGdiSetVirtualResolution(long long long long long)
@ stdcall NtGdiStartDoc(long ptr ptr long)
@ stdcall NtGdiStartPage(long)
@ stdcall NtGdiStretchBlt(long long long long long long long long long long long long)
@ stdcall NtGdiStretchDIBitsInternal(long long long long long long long long long ptr ptr long long long long long)
@ stdcall NtGdiStrokeAndFillPath(long)
@ stdcall NtGdiStrokePath(long)
@ stdcall -syscall NtGdiSwapBuffers(long)
@ stdcall -syscall NtGdiTransformPoints(long ptr ptr long long)
@ stdcall NtGdiTransparentBlt(long long long long long long long long long long long)
@ stub NtGdiUMPDEngFreeUserMem
@ stub NtGdiUnloadPrinterDriver
@ stub NtGdiUnmapMemFont
@ stdcall NtGdiUnrealizeObject(long)
@ stdcall NtGdiUpdateColors(long)
@ stub NtGdiUpdateTransform
@ stdcall NtGdiWidenPath(long)
@ stub NtGdiXFORMOBJ_bApplyXform
@ stub NtGdiXFORMOBJ_iGetXform
@ stub NtGdiXLATEOBJ_cGetPalette
@ stub NtGdiXLATEOBJ_hGetColorTransform
@ stub NtGdiXLATEOBJ_iXlate
@ stub NtHWCursorUpdatePointer
@ stub NtInputSpaceRegionFromPoint
@ stub NtIsOneCoreTransformMode
@ stub NtMITAccessibilityTimerNotification
@ stub NtMITActivateInputProcessing
@ stub NtMITCoreMsgKOpenConnectionTo
@ stub NtMITDeactivateInputProcessing
@ stub NtMITDisableMouseIntercept
@ stub NtMITDispatchCompletion
@ stub NtMITEnableMouseIntercept
@ stub NtMITGetCursorUpdateHandle
@ stub NtMITInitMinuserThread
@ stub NtMITMinuserSetInputTransformOffset
@ stub NtMITMinuserWindowCreated
@ stub NtMITMinuserWindowDestroyed
@ stub NtMITPostMouseInputMessage
@ stub NtMITPostThreadEventMessage
@ stub NtMITPostWindowEventMessage
@ stub NtMITPrepareReceiveInputMessage
@ stub NtMITPrepareSendInputMessage
@ stub NtMITProcessDelegateCapturedPointers
@ stub NtMITSetForegroundRoutingInfo
@ stub NtMITSetInputCallbacks
@ stub NtMITSetInputDelegationMode
@ stub NtMITSetInputObservationState
@ stub NtMITSetKeyboardInputRoutingPolicy
@ stub NtMITSetKeyboardOverriderState
@ stub NtMITSetLastInputRecipient
@ stub NtMITSynthesizeKeyboardInput
@ stub NtMITSynthesizeMouseInput
@ stub NtMITSynthesizeTouchInput
@ stub NtMITUninitMinuserThread
@ stub NtMITUpdateInputGlobals
@ stub NtMapVisualRelativePoints
@ stub NtMinGetInputTransform
@ stub NtMinInteropCoreMessagingWithInput
@ stub NtMinQPeekForInput
@ stub NtMinQSuspendInputProcessing
@ stub NtMinQUpdateWakeMask
@ stub NtModerncoreBeginLayoutUpdate
@ stub NtModerncoreCreateDCompositionHwndTarget
@ stub NtModerncoreCreateGDIHwndTarget
@ stub NtModerncoreDestroyDCompositionHwndTarget
@ stub NtModerncoreDestroyGDIHwndTarget
@ stub NtModerncoreEnableResizeLayoutSynchronization
@ stub NtModerncoreGetNavigationWindowVisual
@ stub NtModerncoreGetResizeDCompositionSynchronizationObject
@ stub NtModerncoreGetWindowContentVisual
@ stub NtModerncoreIdleTimerThread
@ stub NtModerncoreIsResizeLayoutSynchronizationEnabled
@ stub NtModerncoreProcessConnect
@ stub NtModerncoreRegisterEnhancedNavigationWindowHandle
@ stub NtModerncoreRegisterNavigationWindowHandle
@ stub NtModerncoreSetNavigationServiceSid
@ stub NtModerncoreUnregisterNavigationWindowHandle
@ stub NtNotifyPresentToCompositionSurface
@ stub NtOpenCompositionSurfaceDirtyRegion
@ stub NtOpenCompositionSurfaceSectionInfo
@ stub NtOpenCompositionSurfaceSwapChainHandleInfo
@ stub NtQueryCompositionInputIsImplicit
@ stub NtQueryCompositionInputQueueAndTransform
@ stub NtQueryCompositionInputSink
@ stub NtQueryCompositionInputSinkLuid
@ stub NtQueryCompositionInputSinkViewId
@ stub NtQueryCompositionSurfaceBinding
@ stub NtQueryCompositionSurfaceHDRMetaData
@ stub NtQueryCompositionSurfaceRenderingRealization
@ stub NtQueryCompositionSurfaceStatistics
@ stub NtRIMAddInputObserver
@ stub NtRIMAreSiblingDevices
@ stub NtRIMDeviceIoControl
@ stub NtRIMEnableMonitorMappingForDevice
@ stub NtRIMFreeInputBuffer
@ stub NtRIMGetDevicePreparsedData
@ stub NtRIMGetDevicePreparsedDataLockfree
@ stub NtRIMGetDeviceProperties
@ stub NtRIMGetDevicePropertiesLockfree
@ stub NtRIMGetPhysicalDeviceRect
@ stub NtRIMGetSourceProcessId
@ stub NtRIMObserveNextInput
@ stub NtRIMOnPnpNotification
@ stub NtRIMOnTimerNotification
@ stub NtRIMQueryDevicePath
@ stub NtRIMReadInput
@ stub NtRIMRegisterForInput
@ stub NtRIMRemoveInputObserver
@ stub NtRIMSetExtendedDeviceProperty
@ stub NtRIMSetTestModeStatus
@ stub NtRIMUnregisterForInput
@ stub NtRIMUpdateInputObserverRegistration
@ stub NtSetCompositionSurfaceAnalogExclusive
@ stub NtSetCompositionSurfaceBufferUsage
@ stub NtSetCompositionSurfaceDirectFlipState
@ stub NtSetCompositionSurfaceIndependentFlipInfo
@ stub NtSetCompositionSurfaceStatistics
@ stub NtSetCursorInputSpace
@ stub NtSetPointerDeviceInputSpace
@ stub NtSetShellCursorState
@ stub NtTokenManagerConfirmOutstandingAnalogToken
@ stub NtTokenManagerCreateCompositionTokenHandle
@ stub NtTokenManagerCreateFlipObjectReturnTokenHandle
@ stub NtTokenManagerCreateFlipObjectTokenHandle
@ stub NtTokenManagerGetAnalogExclusiveSurfaceUpdates
@ stub NtTokenManagerGetAnalogExclusiveTokenEvent
@ stub NtTokenManagerOpenSectionAndEvents
@ stub NtTokenManagerThread
@ stub NtUnBindCompositionSurface
@ stub NtUpdateInputSinkTransforms
@ stub NtUserAcquireIAMKey
@ stub NtUserAcquireInteractiveControlBackgroundAccess
@ stdcall NtUserActivateKeyboardLayout(long long)
@ stdcall -syscall NtUserAddClipboardFormatListener(long)
@ stub NtUserAddVisualIdentifier
@ stub NtUserAlterWindowStyle
@ stub NtUserAssociateInputContext
@ stdcall -syscall NtUserAttachThreadInput(long long long)
@ stub NtUserAutoPromoteMouseInPointer
@ stub NtUserAutoRotateScreen
@ stub NtUserBeginLayoutUpdate
@ stub NtUserBeginPaint
@ stub NtUserBitBltSysBmp
@ stub NtUserBlockInput
@ stub NtUserBroadcastThemeChangeEvent
@ stub NtUserBuildHimcList
@ stdcall -syscall NtUserBuildHwndList(long long long long long long ptr ptr)
@ stub NtUserBuildNameList
@ stub NtUserBuildPropList
@ stub NtUserCalcMenuBar
@ stub NtUserCalculatePopupWindowPosition
@ stub NtUserCallHwnd
@ stub NtUserCallHwndLock
@ stub NtUserCallHwndLockSafe
@ stub NtUserCallHwndOpt
@ stub NtUserCallHwndParam
@ stub NtUserCallHwndParamLock
@ stub NtUserCallHwndParamLockSafe
@ stub NtUserCallHwndSafe
@ stub NtUserCallMsgFilter
@ stub NtUserCallNextHookEx
@ stub NtUserCallNoParam
@ stdcall NtUserCallOneParam(long long)
@ stdcall NtUserCallTwoParam(long long long)
@ stub NtUserCanBrokerForceForeground
@ stub NtUserChangeClipboardChain
@ stdcall NtUserChangeDisplaySettings(ptr ptr long long ptr)
@ stub NtUserChangeWindowMessageFilterEx
@ stub NtUserCheckAccessForIntegrityLevel
@ stub NtUserCheckMenuItem
@ stub NtUserCheckProcessForClipboardAccess
@ stub NtUserCheckProcessSession
@ stub NtUserCheckWindowThreadDesktop
@ stub NtUserChildWindowFromPointEx
@ stub NtUserClearForeground
@ stub NtUserClipCursor
@ stub NtUserCloseClipboard
@ stdcall -syscall NtUserCloseDesktop(long)
@ stdcall -syscall NtUserCloseWindowStation(long)
@ stub NtUserCompositionInputSinkLuidFromPoint
@ stub NtUserCompositionInputSinkViewInstanceIdFromPoint
@ stub NtUserConfigureActivationObject
@ stub NtUserConfirmResizeCommit
@ stub NtUserConsoleControl
@ stub NtUserConvertMemHandle
@ stub NtUserCopyAcceleratorTable
@ stdcall NtUserCountClipboardFormats()
@ stub NtUserCreateAcceleratorTable
@ stub NtUserCreateActivationGroup
@ stub NtUserCreateActivationObject
@ stub NtUserCreateCaret
@ stub NtUserCreateDCompositionHwndTarget
@ stdcall -syscall NtUserCreateDesktopEx(ptr ptr ptr long long long)
@ stub NtUserCreateEmptyCursorObject
@ stub NtUserCreateInputContext
@ stub NtUserCreateLocalMemHandle
@ stub NtUserCreatePalmRejectionDelayZone
@ stub NtUserCreateWindowEx
@ stub NtUserCreateWindowGroup
@ stdcall -syscall NtUserCreateWindowStation(ptr long long long long long long)
@ stub NtUserCtxDisplayIOCtl
@ stub NtUserDdeInitialize
@ stub NtUserDefSetText
@ stub NtUserDeferWindowDpiChanges
@ stub NtUserDeferWindowPosAndBand
@ stub NtUserDelegateCapturePointers
@ stub NtUserDelegateInput
@ stub NtUserDeleteMenu
@ stub NtUserDeleteWindowGroup
@ stub NtUserDestroyAcceleratorTable
@ stub NtUserDestroyActivationGroup
@ stub NtUserDestroyActivationObject
@ stub NtUserDestroyCursor
@ stub NtUserDestroyDCompositionHwndTarget
@ stub NtUserDestroyInputContext
@ stub NtUserDestroyMenu
@ stub NtUserDestroyPalmRejectionDelayZone
@ stub NtUserDestroyWindow
@ stub NtUserDisableImmersiveOwner
@ stub NtUserDisableProcessWindowFiltering
@ stub NtUserDisableThreadIme
@ stub NtUserDiscardPointerFrameMessages
@ stub NtUserDispatchMessage
@ stub NtUserDisplayConfigGetDeviceInfo
@ stub NtUserDisplayConfigSetDeviceInfo
@ stub NtUserDoSoundConnect
@ stub NtUserDoSoundDisconnect
@ stub NtUserDownlevelTouchpad
@ stub NtUserDragDetect
@ stub NtUserDragObject
@ stub NtUserDrawAnimatedRects
@ stub NtUserDrawCaption
@ stub NtUserDrawCaptionTemp
@ stub NtUserDrawIconEx
@ stub NtUserDrawMenuBarTemp
@ stub NtUserDwmGetRemoteSessionOcclusionEvent
@ stub NtUserDwmGetRemoteSessionOcclusionState
@ stub NtUserDwmKernelShutdown
@ stub NtUserDwmKernelStartup
@ stub NtUserDwmValidateWindow
@ stub NtUserEmptyClipboard
@ stub NtUserEnableChildWindowDpiMessage
@ stub NtUserEnableIAMAccess
@ stub NtUserEnableMenuItem
@ stub NtUserEnableMouseInPointer
@ stub NtUserEnableMouseInPointerForWindow
@ stub NtUserEnableMouseInputForCursorSuppression
@ stub NtUserEnableNonClientDpiScaling
@ stub NtUserEnableResizeLayoutSynchronization
@ stub NtUserEnableScrollBar
@ stub NtUserEnableSoftwareCursorForScreenCapture
@ stub NtUserEnableTouchPad
@ stub NtUserEnableWindowGDIScaledDpiMessage
@ stub NtUserEnableWindowGroupPolicy
@ stub NtUserEnableWindowResizeOptimization
@ stub NtUserEndDeferWindowPosEx
@ stub NtUserEndMenu
@ stub NtUserEndPaint
@ stdcall NtUserEnumDisplayDevices(ptr long ptr long)
@ stdcall NtUserEnumDisplayMonitors(long ptr ptr long)
@ stdcall NtUserEnumDisplaySettings(ptr long ptr long)
@ stub NtUserEvent
@ stub NtUserExcludeUpdateRgn
@ stub NtUserFillWindow
@ stub NtUserFindExistingCursorIcon
@ stub NtUserFindWindowEx
@ stub NtUserFlashWindowEx
@ stub NtUserForceWindowToDpiForTest
@ stub NtUserFrostCrashedWindow
@ stub NtUserFunctionalizeDisplayConfig
@ stub NtUserGetActiveProcessesDpis
@ stub NtUserGetAltTabInfo
@ stub NtUserGetAncestor
@ stub NtUserGetAppImeLevel
@ stub NtUserGetAsyncKeyState
@ stub NtUserGetAtomName
@ stub NtUserGetAutoRotationState
@ stub NtUserGetCIMSSM
@ stub NtUserGetCPD
@ stub NtUserGetCaretBlinkTime
@ stub NtUserGetCaretPos
@ stub NtUserGetClassInfoEx
@ stub NtUserGetClassName
@ stub NtUserGetClipCursor
@ stub NtUserGetClipboardAccessToken
@ stub NtUserGetClipboardData
@ stdcall -syscall NtUserGetClipboardFormatName(long ptr long)
@ stdcall -syscall NtUserGetClipboardOwner()
@ stdcall -syscall NtUserGetClipboardSequenceNumber()
@ stdcall -syscall NtUserGetClipboardViewer()
@ stub NtUserGetComboBoxInfo
@ stub NtUserGetControlBrush
@ stub NtUserGetControlColor
@ stub NtUserGetCurrentDpiInfoForWindow
@ stub NtUserGetCurrentInputMessageSource
@ stdcall -syscall NtUserGetCursor()
@ stub NtUserGetCursorFrameInfo
@ stub NtUserGetCursorInfo
@ stub NtUserGetDC
@ stub NtUserGetDCEx
@ stub NtUserGetDManipHookInitFunction
@ stub NtUserGetDesktopID
@ stub NtUserGetDisplayAutoRotationPreferences
@ stub NtUserGetDisplayAutoRotationPreferencesByProcessId
@ stdcall NtUserGetDisplayConfigBufferSizes(long ptr ptr)
@ stdcall -syscall NtUserGetDoubleClickTime()
@ stub NtUserGetDpiForCurrentProcess
@ stdcall -syscall NtUserGetDpiForMonitor(long long ptr ptr)
@ stub NtUserGetExtendedPointerDeviceProperty
@ stub NtUserGetForegroundWindow
@ stub NtUserGetGUIThreadInfo
@ stub NtUserGetGestureConfig
@ stub NtUserGetGestureExtArgs
@ stub NtUserGetGestureInfo
@ stub NtUserGetGuiResources
@ stub NtUserGetHDevName
@ stub NtUserGetHimetricScaleFactorFromPixelLocation
@ stub NtUserGetIconInfo
@ stub NtUserGetIconSize
@ stub NtUserGetImeHotKey
@ stub NtUserGetImeInfoEx
@ stub NtUserGetInputContainerId
@ stub NtUserGetInputLocaleInfo
@ stub NtUserGetInteractiveControlDeviceInfo
@ stub NtUserGetInteractiveControlInfo
@ stub NtUserGetInteractiveCtrlSupportedWaveforms
@ stub NtUserGetInternalWindowPos
@ stdcall NtUserGetKeyNameText(long ptr long)
@ stdcall -syscall NtUserGetKeyState(long)
@ stdcall -syscall NtUserGetKeyboardLayout(long)
@ stdcall NtUserGetKeyboardLayoutList(long ptr)
@ stdcall -syscall NtUserGetKeyboardLayoutName(ptr)
@ stdcall -syscall NtUserGetKeyboardState(ptr)
@ stdcall -syscall NtUserGetLayeredWindowAttributes(long ptr ptr ptr)
@ stub NtUserGetListBoxInfo
@ stub NtUserGetMenuBarInfo
@ stub NtUserGetMenuIndex
@ stub NtUserGetMenuItemRect
@ stub NtUserGetMessage
@ stdcall -syscall NtUserGetMouseMovePointsEx(long ptr ptr long long)
@ stdcall -syscall NtUserGetObjectInformation(long long long long ptr)
@ stub NtUserGetOemBitmapSize
@ stdcall -syscall NtUserGetOpenClipboardWindow()
@ stub NtUserGetOwnerTransformedMonitorRect
@ stub NtUserGetPhysicalDeviceRect
@ stub NtUserGetPointerCursorId
@ stub NtUserGetPointerDevice
@ stub NtUserGetPointerDeviceCursors
@ stub NtUserGetPointerDeviceInputSpace
@ stub NtUserGetPointerDeviceOrientation
@ stub NtUserGetPointerDeviceProperties
@ stub NtUserGetPointerDeviceRects
@ stub NtUserGetPointerDevices
@ stub NtUserGetPointerFrameTimes
@ stub NtUserGetPointerInfoList
@ stub NtUserGetPointerInputTransform
@ stub NtUserGetPointerProprietaryId
@ stub NtUserGetPointerType
@ stub NtUserGetPrecisionTouchPadConfiguration
@ stdcall NtUserGetPriorityClipboardFormat(ptr long)
@ stdcall -syscall NtUserGetProcessDpiAwarenessContext(long)
@ stub NtUserGetProcessUIContextInformation
@ stdcall -syscall NtUserGetProcessWindowStation()
@ stdcall -syscall NtUserGetProp(long wstr)
@ stub NtUserGetQueueStatus
@ stub NtUserGetQueueStatusReadonly
@ stub NtUserGetRawInputBuffer
@ stub NtUserGetRawInputData
@ stub NtUserGetRawInputDeviceInfo
@ stub NtUserGetRawInputDeviceList
@ stub NtUserGetRawPointerDeviceData
@ stub NtUserGetRegisteredRawInputDevices
@ stub NtUserGetRequiredCursorSizes
@ stub NtUserGetResizeDCompositionSynchronizationObject
@ stub NtUserGetScrollBarInfo
@ stub NtUserGetSharedWindowData
@ stdcall -syscall NtUserGetSystemDpiForProcess(long)
@ stub NtUserGetSystemMenu
@ stdcall -syscall NtUserGetThreadDesktop(long)
@ stub NtUserGetThreadState
@ stub NtUserGetTitleBarInfo
@ stub NtUserGetTopLevelWindow
@ stub NtUserGetTouchInputInfo
@ stub NtUserGetTouchValidationStatus
@ stub NtUserGetUniformSpaceMapping
@ stub NtUserGetUpdateRect
@ stub NtUserGetUpdateRgn
@ stdcall NtUserGetUpdatedClipboardFormats(ptr long ptr)
@ stub NtUserGetWOWClass
@ stub NtUserGetWindowBand
@ stub NtUserGetWindowCompositionAttribute
@ stub NtUserGetWindowCompositionInfo
@ stub NtUserGetWindowDC
@ stub NtUserGetWindowDisplayAffinity
@ stub NtUserGetWindowFeedbackSetting
@ stub NtUserGetWindowGroupId
@ stub NtUserGetWindowMinimizeRect
@ stub NtUserGetWindowPlacement
@ stub NtUserGetWindowProcessHandle
@ stub NtUserGetWindowRgnEx
@ stub NtUserGhostWindowFromHungWindow
@ stub NtUserHandleDelegatedInput
@ stub NtUserHardErrorControl
@ stub NtUserHideCaret
@ stub NtUserHidePointerContactVisualization
@ stub NtUserHiliteMenuItem
@ stub NtUserHungWindowFromGhostWindow
@ stub NtUserHwndQueryRedirectionInfo
@ stub NtUserHwndSetRedirectionInfo
@ stub NtUserImpersonateDdeClientWindow
@ stub NtUserInheritWindowMonitor
@ stub NtUserInitTask
@ stub NtUserInitialize
@ stub NtUserInitializeClientPfnArrays
@ stub NtUserInitializeGenericHidInjection
@ stub NtUserInitializeInputDeviceInjection
@ stub NtUserInitializePointerDeviceInjection
@ stub NtUserInitializePointerDeviceInjectionEx
@ stub NtUserInitializeTouchInjection
@ stub NtUserInjectDeviceInput
@ stub NtUserInjectGenericHidInput
@ stub NtUserInjectGesture
@ stub NtUserInjectKeyboardInput
@ stub NtUserInjectMouseInput
@ stub NtUserInjectPointerInput
@ stub NtUserInjectTouchInput
@ stub NtUserInteractiveControlQueryUsage
@ stub NtUserInternalGetWindowIcon
@ stub NtUserInternalGetWindowText
@ stub NtUserInternalToUnicode
@ stub NtUserInvalidateRect
@ stub NtUserInvalidateRgn
@ stub NtUserIsChildWindowDpiMessageEnabled
@ stdcall NtUserIsClipboardFormatAvailable(long)
@ stub NtUserIsMouseInPointerEnabled
@ stub NtUserIsMouseInputEnabled
@ stub NtUserIsNonClientDpiScalingEnabled
@ stub NtUserIsResizeLayoutSynchronizationEnabled
@ stub NtUserIsTopLevelWindow
@ stub NtUserIsTouchWindow
@ stub NtUserIsWindowBroadcastingDpiToChildren
@ stub NtUserIsWindowGDIScaledDpiMessageEnabled
@ stub NtUserKillTimer
@ stub NtUserLayoutCompleted
@ stub NtUserLinkDpiCursor
@ stub NtUserLoadKeyboardLayoutEx
@ stub NtUserLockCursor
@ stub NtUserLockWindowStation
@ stub NtUserLockWindowUpdate
@ stub NtUserLockWorkStation
@ stub NtUserLogicalToPerMonitorDPIPhysicalPoint
@ stub NtUserLogicalToPhysicalDpiPointForWindow
@ stub NtUserLogicalToPhysicalPoint
@ stub NtUserMNDragLeave
@ stub NtUserMNDragOver
@ stub NtUserMagControl
@ stub NtUserMagGetContextInformation
@ stub NtUserMagSetContextInformation
@ stub NtUserMapPointsByVisualIdentifier
@ stdcall NtUserMapVirtualKeyEx(long long long)
@ stub NtUserMarkWindowForRawMouse
@ stub NtUserMenuItemFromPoint
@ stub NtUserMessageCall
@ stub NtUserMinInitialize
@ stub NtUserMinMaximize
@ stub NtUserModifyUserStartupInfoFlags
@ stub NtUserModifyWindowTouchCapability
@ stub NtUserMoveWindow
@ stub NtUserMsgWaitForMultipleObjectsEx
@ stub NtUserNavigateFocus
@ stub NtUserNotifyIMEStatus
@ stub NtUserNotifyProcessCreate
@ stub NtUserNotifyWinEvent
@ stub NtUserOpenClipboard
@ stdcall -syscall NtUserOpenDesktop(ptr long long)
@ stdcall -syscall NtUserOpenInputDesktop(long long long)
@ stub NtUserOpenThreadDesktop
@ stdcall -syscall NtUserOpenWindowStation(ptr long)
@ stub NtUserPaintDesktop
@ stub NtUserPaintMenuBar
@ stub NtUserPaintMonitor
@ stub NtUserPeekMessage
@ stub NtUserPerMonitorDPIPhysicalToLogicalPoint
@ stub NtUserPhysicalToLogicalDpiPointForWindow
@ stub NtUserPhysicalToLogicalPoint
@ stub NtUserPostKeyboardInputMessage
@ stub NtUserPostMessage
@ stub NtUserPostThreadMessage
@ stub NtUserPrintWindow
@ stub NtUserProcessConnect
@ stub NtUserProcessInkFeedbackCommand
@ stub NtUserPromoteMouseInPointer
@ stub NtUserPromotePointer
@ stub NtUserQueryActivationObject
@ stub NtUserQueryBSDRWindow
@ stub NtUserQueryDisplayConfig
@ stub NtUserQueryInformationThread
@ stub NtUserQueryInputContext
@ stub NtUserQuerySendMessage
@ stub NtUserQueryWindow
@ stub NtUserRealChildWindowFromPoint
@ stub NtUserRealInternalGetMessage
@ stub NtUserRealWaitMessageEx
@ stub NtUserRedrawWindow
@ stub NtUserRegisterBSDRWindow
@ stub NtUserRegisterClassExWOW
@ stub NtUserRegisterDManipHook
@ stub NtUserRegisterEdgy
@ stub NtUserRegisterErrorReportingDialog
@ stub NtUserRegisterHotKey
@ stub NtUserRegisterManipulationThread
@ stub NtUserRegisterPointerDeviceNotifications
@ stub NtUserRegisterPointerInputTarget
@ stub NtUserRegisterRawInputDevices
@ stub NtUserRegisterServicesProcess
@ stub NtUserRegisterSessionPort
@ stub NtUserRegisterShellPTPListener
@ stub NtUserRegisterTasklist
@ stub NtUserRegisterTouchHitTestingWindow
@ stub NtUserRegisterTouchPadCapable
@ stub NtUserRegisterUserApiHook
@ stub NtUserRegisterWindowMessage
@ stub NtUserReleaseDC
@ stub NtUserReleaseDwmHitTestWaiters
@ stub NtUserRemoteConnect
@ stub NtUserRemoteRedrawRectangle
@ stub NtUserRemoteRedrawScreen
@ stub NtUserRemoteStopScreenUpdates
@ stdcall -syscall NtUserRemoveClipboardFormatListener(long)
@ stub NtUserRemoveInjectionDevice
@ stub NtUserRemoveMenu
@ stdcall -syscall NtUserRemoveProp(long wstr)
@ stub NtUserRemoveVisualIdentifier
@ stub NtUserReportInertia
@ stub NtUserRequestMoveSizeOperation
@ stub NtUserResolveDesktopForWOW
@ stub NtUserRestoreWindowDpiChanges
@ stub NtUserSBGetParms
@ stdcall NtUserScrollDC(long long long ptr ptr long ptr)
@ stub NtUserScrollWindowEx
@ stdcall NtUserSelectPalette(long long long)
@ stub NtUserSendEventMessage
@ stub NtUserSendInput
@ stub NtUserSendInteractiveControlHapticsReport
@ stub NtUserSetActivationFilter
@ stub NtUserSetActiveProcessForMonitor
@ stub NtUserSetActiveWindow
@ stub NtUserSetAppImeLevel
@ stub NtUserSetAutoRotation
@ stub NtUserSetBridgeWindowChild
@ stub NtUserSetBrokeredForeground
@ stub NtUserSetCalibrationData
@ stub NtUserSetCapture
@ stub NtUserSetChildWindowNoActivate
@ stub NtUserSetClassLong
@ stub NtUserSetClassLongPtr
@ stub NtUserSetClassWord
@ stub NtUserSetClipboardData
@ stub NtUserSetClipboardViewer
@ stub NtUserSetCoreWindow
@ stub NtUserSetCoreWindowPartner
@ stub NtUserSetCursor
@ stub NtUserSetCursorContents
@ stub NtUserSetCursorIconData
@ stub NtUserSetCursorPos
@ stub NtUserSetDesktopColorTransform
@ stub NtUserSetDesktopVisualInputSink
@ stub NtUserSetDialogControlDpiChangeBehavior
@ stub NtUserSetDisplayAutoRotationPreferences
@ stub NtUserSetDisplayConfig
@ stub NtUserSetDisplayMapping
@ stub NtUserSetFallbackForeground
@ stub NtUserSetFeatureReportResponse
@ stub NtUserSetFocus
@ stub NtUserSetForegroundWindowForApplication
@ stub NtUserSetFullscreenMagnifierOffsetsDWMUpdated
@ stub NtUserSetGestureConfig
@ stub NtUserSetImeHotKey
@ stub NtUserSetImeInfoEx
@ stub NtUserSetImeOwnerWindow
@ stub NtUserSetInformationThread
@ stub NtUserSetInputServiceState
@ stub NtUserSetInteractiveControlFocus
@ stub NtUserSetInteractiveCtrlRotationAngle
@ stub NtUserSetInternalWindowPos
@ stdcall -syscall NtUserSetKeyboardState(ptr)
@ stub NtUserSetLayeredWindowAttributes
@ stub NtUserSetMagnificationDesktopMagnifierOffsetsDWMUpdated
@ stub NtUserSetManipulationInputTarget
@ stub NtUserSetMenu
@ stub NtUserSetMenuContextHelpId
@ stub NtUserSetMenuDefaultItem
@ stub NtUserSetMenuFlagRtoL
@ stub NtUserSetMirrorRendering
@ stub NtUserSetMonitorWorkArea
@ stub NtUserSetMouseInputRateLimitingTimer
@ stdcall -syscall NtUserSetObjectInformation(long long ptr long)
@ stub NtUserSetParent
@ stub NtUserSetPrecisionTouchPadConfiguration
@ stdcall -syscall NtUserSetProcessDpiAwarenessContext(long long)
@ stub NtUserSetProcessInteractionFlags
@ stub NtUserSetProcessMousewheelRoutingMode
@ stub NtUserSetProcessRestrictionExemption
@ stub NtUserSetProcessUIAccessZorder
@ stdcall -syscall NtUserSetProcessWindowStation(long)
@ stdcall -syscall NtUserSetProp(long wstr ptr)
@ stub NtUserSetScrollInfo
@ stub NtUserSetSensorPresence
@ stub NtUserSetSharedWindowData
@ stub NtUserSetShellWindowEx
@ stdcall NtUserSetSysColors(long ptr ptr)
@ stub NtUserSetSystemCursor
@ stub NtUserSetSystemMenu
@ stub NtUserSetSystemTimer
@ stub NtUserSetTargetForResourceBrokering
@ stdcall -syscall NtUserSetThreadDesktop(long)
@ stub NtUserSetThreadInputBlocked
@ stub NtUserSetThreadLayoutHandles
@ stub NtUserSetThreadState
@ stub NtUserSetTimer
@ stub NtUserSetWinEventHook
@ stub NtUserSetWindowArrangement
@ stub NtUserSetWindowBand
@ stub NtUserSetWindowCompositionAttribute
@ stub NtUserSetWindowCompositionTransition
@ stub NtUserSetWindowDisplayAffinity
@ stub NtUserSetWindowFNID
@ stub NtUserSetWindowFeedbackSetting
@ stub NtUserSetWindowGroup
@ stub NtUserSetWindowLong
@ stub NtUserSetWindowLongPtr
@ stub NtUserSetWindowPlacement
@ stub NtUserSetWindowPos
@ stub NtUserSetWindowRgn
@ stub NtUserSetWindowRgnEx
@ stub NtUserSetWindowShowState
@ stub NtUserSetWindowStationUser
@ stub NtUserSetWindowWord
@ stub NtUserSetWindowsHookAW
@ stub NtUserSetWindowsHookEx
@ stub NtUserShowCaret
@ stdcall NtUserShowCursor(long)
@ stub NtUserShowScrollBar
@ stub NtUserShowSystemCursor
@ stub NtUserShowWindow
@ stub NtUserShowWindowAsync
@ stub NtUserShutdownBlockReasonCreate
@ stub NtUserShutdownBlockReasonQuery
@ stub NtUserShutdownReasonDestroy
@ stub NtUserSignalRedirectionStartComplete
@ stub NtUserSlicerControl
@ stub NtUserSoundSentry
@ stub NtUserStopAndEndInertia
@ stub NtUserSwitchDesktop
@ stdcall NtUserSystemParametersInfo(long long ptr long)
@ stdcall NtUserSystemParametersInfoForDpi(long long ptr long long)
@ stub NtUserTestForInteractiveUser
@ stub NtUserThunkedMenuInfo
@ stub NtUserThunkedMenuItemInfo
@ stdcall NtUserToUnicodeEx(long long ptr ptr long long long)
@ stub NtUserTrackMouseEvent
@ stub NtUserTrackPopupMenuEx
@ stub NtUserTransformPoint
@ stub NtUserTransformRect
@ stub NtUserTranslateAccelerator
@ stub NtUserTranslateMessage
@ stub NtUserUndelegateInput
@ stub NtUserUnhookWinEvent
@ stub NtUserUnhookWindowsHookEx
@ stub NtUserUnloadKeyboardLayout
@ stub NtUserUnlockWindowStation
@ stub NtUserUnregisterClass
@ stdcall NtUserUnregisterHotKey(long long)
@ stub NtUserUnregisterSessionPort
@ stub NtUserUnregisterUserApiHook
@ stub NtUserUpdateDefaultDesktopThumbnail
@ stub NtUserUpdateInputContext
@ stub NtUserUpdateInstance
@ stub NtUserUpdateLayeredWindow
@ stub NtUserUpdatePerUserSystemParameters
@ stub NtUserUpdateWindowInputSinkHints
@ stub NtUserUpdateWindowTrackingInfo
@ stub NtUserUserHandleGrantAccess
@ stub NtUserValidateRect
@ stub NtUserValidateTimerCallback
@ stdcall NtUserVkKeyScanEx(long long)
@ stub NtUserWOWCleanup
@ stub NtUserWaitAvailableMessageEx
@ stub NtUserWaitForInputIdle
@ stub NtUserWaitForMsgAndEvent
@ stub NtUserWaitForRedirectionStartComplete
@ stub NtUserWaitMessage
@ stub NtUserWindowFromDC
@ stub NtUserWindowFromPhysicalPoint
@ stub NtUserWindowFromPoint
@ stub NtUserYieldTask
@ stub NtValidateCompositionSurfaceHandle
@ stub NtVisualCaptureBits
# extern gDispatchTableValues

################################################################
# Wine internal extensions

# user32
@ stdcall GetDCHook(long ptr)
@ stdcall SetDCHook(long ptr long)
@ stdcall SetHookFlags(long long)
@ cdecl __wine_set_visible_region(long long ptr ptr ptr)

# Graphics drivers
@ cdecl __wine_set_display_driver(ptr long)

# OpenGL
@ cdecl __wine_get_wgl_driver(long long)

# Vulkan
@ cdecl __wine_get_vulkan_driver(long)

# gdi32
@ stdcall SetDIBits(long long long long ptr ptr long)
@ cdecl __wine_get_brush_bitmap_info(long ptr ptr ptr)
@ cdecl __wine_get_icm_profile(long long ptr ptr)
@ cdecl __wine_get_file_outline_text_metric(wstr ptr)
