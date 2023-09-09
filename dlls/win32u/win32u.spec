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
@ stdcall -syscall NtGdiAbortDoc(long)
@ stdcall -syscall NtGdiAbortPath(long)
@ stub NtGdiAddEmbFontToDC
@ stdcall -syscall NtGdiAddFontMemResourceEx(ptr long ptr long ptr)
@ stdcall -syscall NtGdiAddFontResourceW(wstr long long long long ptr)
@ stub NtGdiAddInitialFonts
@ stub NtGdiAddRemoteFontToDC
@ stub NtGdiAddRemoteMMInstanceToDC
@ stdcall -syscall NtGdiAlphaBlend(long long long long long long long long long long long ptr)
@ stdcall -syscall NtGdiAngleArc(long long long long long long)
@ stub NtGdiAnyLinkedFonts
@ stdcall -syscall NtGdiArcInternal(long long long long long long long long long long)
@ stub NtGdiBRUSHOBJ_DeleteRbrush
@ stub NtGdiBRUSHOBJ_hGetColorTransform
@ stub NtGdiBRUSHOBJ_pvAllocRbrush
@ stub NtGdiBRUSHOBJ_pvGetRbrush
@ stub NtGdiBRUSHOBJ_ulGetBrushColor
@ stub NtGdiBeginGdiRendering
@ stdcall -syscall NtGdiBeginPath(long)
@ stdcall -syscall NtGdiBitBlt(long long long long long long long long long long long)
@ stub NtGdiCLIPOBJ_bEnum
@ stub NtGdiCLIPOBJ_cEnumStart
@ stub NtGdiCLIPOBJ_ppoGetPath
@ stub NtGdiCancelDC
@ stub NtGdiChangeGhostFont
@ stub NtGdiCheckBitmapBits
@ stub NtGdiClearBitmapAttributes
@ stub NtGdiClearBrushAttributes
@ stdcall -syscall NtGdiCloseFigure(long)
@ stub NtGdiColorCorrectPalette
@ stdcall -syscall NtGdiCombineRgn(long long long long)
@ stub NtGdiCombineTransform
@ stdcall -syscall NtGdiComputeXformCoefficients(long)
@ stub NtGdiConfigureOPMProtectedOutput
@ stub NtGdiConvertMetafileRect
@ stdcall -syscall NtGdiCreateBitmap(long long long long ptr)
@ stub NtGdiCreateBitmapFromDxSurface
@ stub NtGdiCreateBitmapFromDxSurface2
@ stdcall -syscall NtGdiCreateClientObj(long)
@ stub NtGdiCreateColorSpace
@ stub NtGdiCreateColorTransform
@ stdcall -syscall NtGdiCreateCompatibleBitmap(long long long)
@ stdcall -syscall NtGdiCreateCompatibleDC(long)
@ stdcall -syscall NtGdiCreateDIBBrush(ptr long long long long ptr)
@ stdcall -syscall NtGdiCreateDIBSection(long long long ptr long long long long ptr)
@ stdcall -syscall NtGdiCreateDIBitmapInternal(long long long long ptr ptr long long long long long)
@ stdcall -syscall NtGdiCreateEllipticRgn(long long long long)
@ stdcall -syscall NtGdiCreateHalftonePalette(long)
@ stdcall -syscall NtGdiCreateHatchBrushInternal(long long long)
@ stdcall -syscall NtGdiCreateMetafileDC(long)
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
@ stdcall -syscall NtGdiDdDDICheckVidPnExclusiveOwnership(ptr)
@ stdcall -syscall NtGdiDdDDICloseAdapter(ptr)
@ stub NtGdiDdDDIConfigureSharedResource
@ stub NtGdiDdDDICreateAllocation
@ stub NtGdiDdDDICreateBundleObject
@ stub NtGdiDdDDICreateContext
@ stub NtGdiDdDDICreateContextVirtual
@ stdcall -syscall NtGdiDdDDICreateDCFromMemory(ptr)
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
@ stdcall -syscall NtGdiDdDDIDestroyDCFromMemory(ptr)
@ stdcall -syscall NtGdiDdDDIDestroyDevice(ptr)
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
@ stdcall -syscall NtGdiDdDDIEscape(ptr)
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
@ stdcall -syscall NtGdiDdDDIQueryAdapterInfo(ptr)
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
@ stdcall -syscall NtGdiDdDDIQueryVideoMemoryInfo(ptr)
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
@ stdcall -syscall NtGdiDdDDISetVidPnSourceOwner(ptr)
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
@ stdcall -syscall NtGdiDeleteObjectApp(long )
@ stdcall -syscall NtGdiDescribePixelFormat(long long long ptr)
@ stub NtGdiDestroyOPMProtectedOutput
@ stub NtGdiDestroyPhysicalMonitor
@ stub NtGdiDoBanding
@ stdcall -syscall NtGdiDoPalette(long long long ptr long long)
@ stub NtGdiDrawEscape
@ stdcall -syscall NtGdiDrawStream(long long ptr)
@ stub NtGdiDwmCreatedBitmapRemotingOutput
@ stdcall -syscall NtGdiEllipse(long long long long long)
@ stub NtGdiEnableEudc
@ stdcall -syscall NtGdiEndDoc(long)
@ stub NtGdiEndGdiRendering
@ stdcall -syscall NtGdiEndPage(long)
@ stdcall -syscall NtGdiEndPath(long)
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
@ stdcall -syscall NtGdiEnumFonts(long long long long wstr long ptr ptr)
@ stub NtGdiEnumObjects
@ stdcall -syscall NtGdiEqualRgn(long long)
@ stub NtGdiEudcLoadUnloadLink
@ stdcall -syscall NtGdiExcludeClipRect(long long long long long)
@ stdcall -syscall NtGdiExtCreatePen(long long long long long long long ptr long long long)
@ stdcall -syscall NtGdiExtCreateRegion(ptr long ptr)
@ stdcall -syscall NtGdiExtEscape(long wstr long long long ptr long ptr)
@ stdcall -syscall NtGdiExtFloodFill(long long long long long)
@ stdcall -syscall NtGdiExtGetObjectW(long long ptr)
@ stdcall -syscall NtGdiExtSelectClipRgn(long long long)
@ stdcall -syscall NtGdiExtTextOutW(long long long long ptr ptr long ptr long)
@ stub NtGdiFONTOBJ_cGetAllGlyphHandles
@ stub NtGdiFONTOBJ_cGetGlyphs
@ stub NtGdiFONTOBJ_pQueryGlyphAttrs
@ stub NtGdiFONTOBJ_pfdg
@ stub NtGdiFONTOBJ_pifi
@ stub NtGdiFONTOBJ_pvTrueTypeFontFile
@ stub NtGdiFONTOBJ_pxoGetXform
@ stub NtGdiFONTOBJ_vGetInfo
@ stdcall -syscall NtGdiFillPath(long)
@ stdcall -syscall NtGdiFillRgn(long long long)
@ stdcall -syscall NtGdiFlattenPath(long)
@ stdcall -syscall NtGdiFlush()
@ stdcall -syscall NtGdiFontIsLinked(long)
@ stub NtGdiForceUFIMapping
@ stdcall -syscall NtGdiFrameRgn(long long long long long)
@ stub NtGdiFullscreenControl
@ stdcall -syscall NtGdiGetAndSetDCDword(long long long ptr)
@ stdcall -syscall NtGdiGetAppClipBox(long ptr)
@ stub NtGdiGetAppliedDeviceGammaRamp
@ stdcall -syscall NtGdiGetBitmapBits(long long ptr)
@ stdcall -syscall NtGdiGetBitmapDimension(long ptr)
@ stub NtGdiGetBitmapDpiScaleValue
@ stdcall -syscall NtGdiGetBoundsRect(long ptr long)
@ stub NtGdiGetCOPPCompatibleOPMInformation
@ stub NtGdiGetCertificate
@ stub NtGdiGetCertificateByHandle
@ stub NtGdiGetCertificateSize
@ stub NtGdiGetCertificateSizeByHandle
@ stdcall -syscall NtGdiGetCharABCWidthsW(long long long ptr long ptr)
@ stub NtGdiGetCharSet
@ stdcall -syscall NtGdiGetCharWidthInfo(long ptr)
@ stdcall -syscall NtGdiGetCharWidthW(long long long ptr long ptr)
@ stub NtGdiGetCharacterPlacementW
@ stdcall -syscall NtGdiGetColorAdjustment(long ptr)
@ stub NtGdiGetColorSpaceforBitmap
@ stub NtGdiGetCurrentDpiInfo
@ stub NtGdiGetDCDpiScaleValue
@ stdcall -syscall NtGdiGetDCDword(long long ptr)
@ stdcall -syscall NtGdiGetDCObject(long long)
@ stdcall -syscall NtGdiGetDCPoint(long long ptr)
@ stub NtGdiGetDCforBitmap
@ stdcall -syscall NtGdiGetDIBitsInternal(long long long long ptr ptr long long long)
@ stdcall -syscall NtGdiGetDeviceCaps(long long)
@ stub NtGdiGetDeviceCapsAll
@ stdcall -syscall NtGdiGetDeviceGammaRamp(long ptr)
@ stub NtGdiGetDeviceWidth
@ stub NtGdiGetDhpdev
@ stub NtGdiGetETM
@ stub NtGdiGetEmbUFI
@ stub NtGdiGetEmbedFonts
@ stub NtGdiGetEntry
@ stub NtGdiGetEudcTimeStampEx
@ stdcall -syscall NtGdiGetFontData(long long long ptr long)
@ stdcall -syscall NtGdiGetFontFileData(long long ptr ptr long)
@ stdcall -syscall NtGdiGetFontFileInfo(long long ptr long ptr)
@ stub NtGdiGetFontResourceInfoInternalW
@ stdcall -syscall NtGdiGetFontUnicodeRanges(long ptr)
@ stub NtGdiGetGammaRampCapability
@ stdcall -syscall NtGdiGetGlyphIndicesW(long wstr long ptr long)
@ stub NtGdiGetGlyphIndicesWInternal
@ stdcall -syscall NtGdiGetGlyphOutline(long long long ptr long ptr ptr long)
@ stdcall -syscall NtGdiGetKerningPairs(long long ptr)
@ stub NtGdiGetLinkedUFIs
@ stub NtGdiGetMiterLimit
@ stub NtGdiGetMonitorID
@ stdcall -syscall NtGdiGetNearestColor(long long)
@ stdcall -syscall NtGdiGetNearestPaletteIndex(long long)
@ stub NtGdiGetNumberOfPhysicalMonitors
@ stub NtGdiGetOPMInformation
@ stub NtGdiGetOPMRandomNumber
@ stub NtGdiGetObjectBitmapHandle
@ stdcall -syscall NtGdiGetOutlineTextMetricsInternalW(long long ptr long)
@ stdcall -syscall NtGdiGetPath(long ptr ptr long)
@ stub NtGdiGetPerBandInfo
@ stub NtGdiGetPhysicalMonitorDescription
@ stub NtGdiGetPhysicalMonitors
@ stdcall -syscall NtGdiGetPixel(long long long)
@ stub NtGdiGetProcessSessionFonts
@ stub NtGdiGetPublicFontTableChangeCookie
@ stdcall -syscall NtGdiGetRandomRgn(long long long)
@ stdcall -syscall NtGdiGetRasterizerCaps(ptr long)
@ stdcall -syscall NtGdiGetRealizationInfo(long ptr)
@ stdcall -syscall NtGdiGetRegionData(long long ptr)
@ stdcall -syscall NtGdiGetRgnBox(long ptr)
@ stub NtGdiGetServerMetaFileBits
@ stdcall -syscall NtGdiGetSpoolMessage(ptr long ptr long)
@ stub NtGdiGetStats
@ stub NtGdiGetStringBitmapW
@ stub NtGdiGetSuggestedOPMProtectedOutputArraySize
@ stdcall -syscall NtGdiGetSystemPaletteUse(long)
@ stdcall -syscall NtGdiGetTextCharsetInfo(long ptr long)
@ stub NtGdiGetTextExtent
@ stdcall -syscall NtGdiGetTextExtentExW(long wstr long long ptr ptr ptr long)
@ stdcall -syscall NtGdiGetTextFaceW(long long ptr long)
@ stdcall -syscall NtGdiGetTextMetricsW(long ptr long)
@ stdcall -syscall NtGdiGetTransform(long long ptr)
@ stub NtGdiGetUFI
@ stub NtGdiGetUFIPathname
@ stub NtGdiGetWidthTable
@ stdcall -syscall NtGdiGradientFill(long ptr long ptr long long)
@ stub NtGdiHLSurfGetInformation
@ stub NtGdiHLSurfSetInformation
@ stub NtGdiHT_Get8BPPFormatPalette
@ stub NtGdiHT_Get8BPPMaskPalette
@ stdcall -syscall NtGdiHfontCreate(ptr long long long ptr)
@ stdcall -syscall NtGdiIcmBrushInfo(long long ptr ptr ptr ptr ptr long)
@ stub NtGdiInit
@ stdcall -syscall NtGdiInitSpool()
@ stdcall -syscall NtGdiIntersectClipRect(long long long long long)
@ stdcall -syscall NtGdiInvertRgn(long long)
@ stdcall -syscall NtGdiLineTo(long long long)
@ stub NtGdiMakeFontDir
@ stub NtGdiMakeInfoDC
@ stub NtGdiMakeObjectUnXferable
@ stub NtGdiMakeObjectXferable
@ stdcall -syscall NtGdiMaskBlt(long long long long long long long long long long long long long)
@ stub NtGdiMirrorWindowOrg
@ stdcall -syscall NtGdiModifyWorldTransform(long ptr long)
@ stub NtGdiMonoBitmap
@ stdcall -syscall NtGdiMoveTo(long long long ptr)
@ stdcall -syscall NtGdiOffsetClipRgn(long long long)
@ stdcall -syscall NtGdiOffsetRgn(long long long)
@ stdcall -syscall NtGdiOpenDCW(ptr ptr ptr long long ptr ptr ptr)
@ stub NtGdiPATHOBJ_bEnum
@ stub NtGdiPATHOBJ_bEnumClipLines
@ stub NtGdiPATHOBJ_vEnumStart
@ stub NtGdiPATHOBJ_vEnumStartClipLines
@ stub NtGdiPATHOBJ_vGetBounds
@ stdcall -syscall NtGdiPatBlt(long long long long long long)
@ stdcall -syscall NtGdiPathToRegion(long)
@ stdcall -syscall NtGdiPlgBlt(long ptr long long long long long long long long long)
@ stdcall -syscall NtGdiPolyDraw(long ptr ptr long)
@ stub NtGdiPolyPatBlt
@ stdcall -syscall NtGdiPolyPolyDraw(long ptr ptr long long)
@ stub NtGdiPolyTextOutW
@ stdcall -syscall NtGdiPtInRegion(long long long)
@ stdcall -syscall NtGdiPtVisible(long long long)
@ stub NtGdiQueryFontAssocInfo
@ stub NtGdiQueryFonts
@ stdcall -syscall NtGdiRectInRegion(long ptr)
@ stdcall -syscall NtGdiRectVisible(long ptr)
@ stdcall -syscall NtGdiRectangle(long long long long long)
@ stdcall -syscall NtGdiRemoveFontMemResourceEx(long)
@ stdcall -syscall NtGdiRemoveFontResourceW(wstr long long long long ptr)
@ stub NtGdiRemoveMergeFont
@ stdcall -syscall NtGdiResetDC(long ptr ptr ptr ptr)
@ stdcall -syscall NtGdiResizePalette(long long)
@ stdcall -syscall NtGdiRestoreDC(long long)
@ stdcall -syscall NtGdiRoundRect(long long long long long long long)
@ stub NtGdiSTROBJ_bEnum
@ stub NtGdiSTROBJ_bEnumPositionsOnly
@ stub NtGdiSTROBJ_bGetAdvanceWidths
@ stub NtGdiSTROBJ_dwGetCodePage
@ stub NtGdiSTROBJ_vEnumStart
@ stdcall -syscall NtGdiSaveDC(long)
@ stub NtGdiScaleRgn
@ stub NtGdiScaleValues
@ stdcall -syscall NtGdiScaleViewportExtEx(long long long long long ptr)
@ stdcall -syscall NtGdiScaleWindowExtEx(long long long long long ptr)
@ stdcall -syscall NtGdiSelectBitmap(long long)
@ stdcall -syscall NtGdiSelectBrush(long long)
@ stdcall -syscall NtGdiSelectClipPath(long long)
@ stdcall -syscall NtGdiSelectFont(long long)
@ stdcall -syscall NtGdiSelectPen(long long)
@ stub NtGdiSetBitmapAttributes
@ stdcall -syscall NtGdiSetBitmapBits(long long ptr)
@ stdcall -syscall NtGdiSetBitmapDimension(long long long ptr)
@ stdcall -syscall NtGdiSetBoundsRect(long ptr long)
@ stub NtGdiSetBrushAttributes
@ stdcall -syscall NtGdiSetBrushOrg(long long long ptr)
@ stdcall -syscall NtGdiSetColorAdjustment(long ptr)
@ stub NtGdiSetColorSpace
@ stdcall -syscall NtGdiSetDIBitsToDeviceInternal(long long long long long long long long long ptr ptr long long long long long)
@ stdcall -syscall NtGdiSetDeviceGammaRamp(ptr ptr)
@ stub NtGdiSetFontEnumeration
@ stub NtGdiSetFontXform
@ stub NtGdiSetIcmMode
@ stdcall -syscall NtGdiSetLayout(long long long)
@ stub NtGdiSetLinkedUFIs
@ stdcall -syscall NtGdiSetMagicColors(long long long)
@ stdcall -syscall NtGdiSetMetaRgn(long)
@ stub NtGdiSetMiterLimit
@ stub NtGdiSetOPMSigningKeyAndSequenceNumbers
@ stub NtGdiSetPUMPDOBJ
@ stdcall -syscall NtGdiSetPixel(long long long long)
@ stdcall -syscall NtGdiSetPixelFormat(long long)
@ stub NtGdiSetPrivateDeviceGammaRamp
@ stdcall -syscall NtGdiSetRectRgn(long long long long long)
@ stub NtGdiSetSizeDevice
@ stdcall -syscall NtGdiSetSystemPaletteUse(long long)
@ stdcall -syscall NtGdiSetTextJustification(long long long)
@ stub NtGdiSetUMPDSandboxState
@ stdcall -syscall NtGdiSetVirtualResolution(long long long long long)
@ stdcall -syscall NtGdiStartDoc(long ptr ptr long)
@ stdcall -syscall NtGdiStartPage(long)
@ stdcall -syscall NtGdiStretchBlt(long long long long long long long long long long long long)
@ stdcall -syscall NtGdiStretchDIBitsInternal(long long long long long long long long long ptr ptr long long long long long)
@ stdcall -syscall NtGdiStrokeAndFillPath(long)
@ stdcall -syscall NtGdiStrokePath(long)
@ stdcall -syscall NtGdiSwapBuffers(long)
@ stdcall -syscall NtGdiTransformPoints(long ptr ptr long long)
@ stdcall -syscall NtGdiTransparentBlt(long long long long long long long long long long long)
@ stub NtGdiUMPDEngFreeUserMem
@ stub NtGdiUnloadPrinterDriver
@ stub NtGdiUnmapMemFont
@ stdcall -syscall NtGdiUnrealizeObject(long)
@ stdcall -syscall NtGdiUpdateColors(long)
@ stub NtGdiUpdateTransform
@ stdcall -syscall NtGdiWidenPath(long)
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
@ stdcall -syscall NtUserActivateKeyboardLayout(long long)
@ stdcall -syscall NtUserAddClipboardFormatListener(long)
@ stub NtUserAddVisualIdentifier
@ stub NtUserAlterWindowStyle
@ stdcall -syscall NtUserAssociateInputContext(long long long)
@ stdcall -syscall NtUserAttachThreadInput(long long long)
@ stub NtUserAutoPromoteMouseInPointer
@ stub NtUserAutoRotateScreen
@ stub NtUserBeginLayoutUpdate
@ stdcall -syscall NtUserBeginPaint(long ptr)
@ stub NtUserBitBltSysBmp
@ stub NtUserBlockInput
@ stub NtUserBroadcastThemeChangeEvent
@ stdcall -syscall NtUserBuildHimcList(long long ptr ptr)
@ stdcall -syscall NtUserBuildHwndList(long long long long long long ptr ptr)
@ stub NtUserBuildNameList
@ stub NtUserBuildPropList
@ stub NtUserCalcMenuBar
@ stub NtUserCalculatePopupWindowPosition
@ stdcall -syscall NtUserCallHwnd(long long)
@ stub NtUserCallHwndLock
@ stub NtUserCallHwndLockSafe
@ stub NtUserCallHwndOpt
@ stdcall -syscall NtUserCallHwndParam(long ptr long)
@ stub NtUserCallHwndParamLock
@ stub NtUserCallHwndParamLockSafe
@ stub NtUserCallHwndSafe
@ stdcall -syscall NtUserCallMsgFilter(ptr long)
@ stdcall -syscall NtUserCallNextHookEx(long long long long)
@ stdcall -syscall NtUserCallNoParam(long)
@ stdcall -syscall NtUserCallOneParam(long long)
@ stdcall -syscall NtUserCallTwoParam(long long long)
@ stub NtUserCanBrokerForceForeground
@ stdcall -syscall NtUserChangeClipboardChain(long long)
@ stdcall -syscall NtUserChangeDisplaySettings(ptr ptr long long ptr)
@ stub NtUserChangeWindowMessageFilterEx
@ stub NtUserCheckAccessForIntegrityLevel
@ stdcall -syscall NtUserCheckMenuItem(long long long)
@ stub NtUserCheckProcessForClipboardAccess
@ stub NtUserCheckProcessSession
@ stub NtUserCheckWindowThreadDesktop
@ stdcall -syscall NtUserChildWindowFromPointEx(long long long long)
@ stub NtUserClearForeground
@ stdcall -syscall NtUserClipCursor(ptr)
@ stdcall -syscall NtUserCloseClipboard()
@ stdcall -syscall NtUserCloseDesktop(long)
@ stdcall -syscall NtUserCloseWindowStation(long)
@ stub NtUserCompositionInputSinkLuidFromPoint
@ stub NtUserCompositionInputSinkViewInstanceIdFromPoint
@ stub NtUserConfigureActivationObject
@ stub NtUserConfirmResizeCommit
@ stub NtUserConsoleControl
@ stub NtUserConvertMemHandle
@ stdcall -syscall NtUserCopyAcceleratorTable(long ptr long)
@ stdcall -syscall NtUserCountClipboardFormats()
@ stdcall -syscall NtUserCreateAcceleratorTable(ptr long)
@ stub NtUserCreateActivationGroup
@ stub NtUserCreateActivationObject
@ stdcall -syscall NtUserCreateCaret(long long long long)
@ stub NtUserCreateDCompositionHwndTarget
@ stdcall -syscall NtUserCreateDesktopEx(ptr ptr ptr long long long)
@ stub NtUserCreateEmptyCursorObject
@ stdcall -syscall NtUserCreateInputContext(ptr)
@ stub NtUserCreateLocalMemHandle
@ stub NtUserCreatePalmRejectionDelayZone
@ stdcall -syscall NtUserCreateWindowEx(long ptr ptr ptr long long long long long long long long ptr long long long long)
@ stub NtUserCreateWindowGroup
@ stdcall -syscall NtUserCreateWindowStation(ptr long long long long long long)
@ stub NtUserCtxDisplayIOCtl
@ stub NtUserDdeInitialize
@ stub NtUserDefSetText
@ stub NtUserDeferWindowDpiChanges
@ stdcall -syscall NtUserDeferWindowPosAndBand(long long long long long long long long long long)
@ stub NtUserDelegateCapturePointers
@ stub NtUserDelegateInput
@ stdcall -syscall NtUserDeleteMenu(long long long)
@ stub NtUserDeleteWindowGroup
@ stdcall -syscall NtUserDestroyAcceleratorTable(long)
@ stub NtUserDestroyActivationGroup
@ stub NtUserDestroyActivationObject
@ stdcall -syscall NtUserDestroyCursor(long long)
@ stub NtUserDestroyDCompositionHwndTarget
@ stdcall -syscall NtUserDestroyInputContext(long)
@ stdcall -syscall NtUserDestroyMenu(long)
@ stub NtUserDestroyPalmRejectionDelayZone
@ stdcall -syscall NtUserDestroyWindow(long)
@ stub NtUserDisableImmersiveOwner
@ stub NtUserDisableProcessWindowFiltering
@ stdcall -syscall NtUserDisableThreadIme(long)
@ stub NtUserDiscardPointerFrameMessages
@ stdcall -syscall NtUserDispatchMessage(ptr)
@ stdcall -syscall NtUserDisplayConfigGetDeviceInfo(ptr)
@ stub NtUserDisplayConfigSetDeviceInfo
@ stub NtUserDoSoundConnect
@ stub NtUserDoSoundDisconnect
@ stub NtUserDownlevelTouchpad
@ stdcall -syscall NtUserDragDetect(long long long)
@ stdcall -syscall NtUserDragObject(long long long long long)
@ stub NtUserDrawAnimatedRects
@ stub NtUserDrawCaption
@ stdcall -syscall NtUserDrawCaptionTemp(long long ptr long long wstr long)
@ stdcall -syscall NtUserDrawIconEx(long long long long long long long long long)
@ stdcall -syscall NtUserDrawMenuBarTemp(long long ptr long long)
@ stub NtUserDwmGetRemoteSessionOcclusionEvent
@ stub NtUserDwmGetRemoteSessionOcclusionState
@ stub NtUserDwmKernelShutdown
@ stub NtUserDwmKernelStartup
@ stub NtUserDwmValidateWindow
@ stdcall -syscall NtUserEmptyClipboard()
@ stub NtUserEnableChildWindowDpiMessage
@ stub NtUserEnableIAMAccess
@ stdcall -syscall NtUserEnableMenuItem(long long long)
@ stdcall -syscall NtUserEnableMouseInPointer(long)
@ stub NtUserEnableMouseInPointerForWindow
@ stub NtUserEnableMouseInputForCursorSuppression
@ stub NtUserEnableNonClientDpiScaling
@ stub NtUserEnableResizeLayoutSynchronization
@ stdcall -syscall NtUserEnableScrollBar(long long long)
@ stub NtUserEnableSoftwareCursorForScreenCapture
@ stub NtUserEnableTouchPad
@ stub NtUserEnableWindowGDIScaledDpiMessage
@ stub NtUserEnableWindowGroupPolicy
@ stub NtUserEnableWindowResizeOptimization
@ stdcall -syscall NtUserEndDeferWindowPosEx(long long)
@ stdcall -syscall NtUserEndMenu()
@ stdcall -syscall NtUserEndPaint(long ptr)
@ stdcall -syscall NtUserEnumDisplayDevices(ptr long ptr long)
@ stdcall -syscall NtUserEnumDisplayMonitors(long ptr ptr long)
@ stdcall -syscall NtUserEnumDisplaySettings(ptr long ptr long)
@ stub NtUserEvent
@ stdcall -syscall NtUserExcludeUpdateRgn(long long)
@ stub NtUserFillWindow
@ stdcall -syscall NtUserFindExistingCursorIcon(ptr ptr ptr)
@ stdcall -syscall NtUserFindWindowEx(long long ptr ptr long)
@ stdcall -syscall NtUserFlashWindowEx(ptr)
@ stub NtUserForceWindowToDpiForTest
@ stub NtUserFrostCrashedWindow
@ stub NtUserFunctionalizeDisplayConfig
@ stub NtUserGetActiveProcessesDpis
@ stub NtUserGetAltTabInfo
@ stdcall -syscall NtUserGetAncestor(long long)
@ stub NtUserGetAppImeLevel
@ stdcall -syscall NtUserGetAsyncKeyState(long)
@ stdcall -syscall NtUserGetAtomName(long ptr)
@ stub NtUserGetAutoRotationState
@ stub NtUserGetCIMSSM
@ stub NtUserGetCPD
@ stdcall -syscall NtUserGetCaretBlinkTime()
@ stdcall -syscall NtUserGetCaretPos(ptr)
@ stdcall -syscall NtUserGetClassInfoEx(ptr ptr ptr ptr long)
@ stdcall -syscall NtUserGetClassName(long long ptr)
@ stub NtUserGetClipCursor
@ stub NtUserGetClipboardAccessToken
@ stdcall -syscall NtUserGetClipboardData(long ptr)
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
@ stdcall -syscall NtUserGetCursorFrameInfo(long long ptr ptr)
@ stdcall -syscall NtUserGetCursorInfo(ptr)
@ stdcall -syscall NtUserGetDC(long)
@ stdcall -syscall NtUserGetDCEx(long long long)
@ stub NtUserGetDManipHookInitFunction
@ stub NtUserGetDesktopID
@ stub NtUserGetDisplayAutoRotationPreferences
@ stub NtUserGetDisplayAutoRotationPreferencesByProcessId
@ stdcall -syscall NtUserGetDisplayConfigBufferSizes(long ptr ptr)
@ stdcall -syscall NtUserGetDoubleClickTime()
@ stub NtUserGetDpiForCurrentProcess
@ stdcall -syscall NtUserGetDpiForMonitor(long long ptr ptr)
@ stub NtUserGetExtendedPointerDeviceProperty
@ stdcall -syscall NtUserGetForegroundWindow()
@ stdcall -syscall NtUserGetGUIThreadInfo(long ptr)
@ stub NtUserGetGestureConfig
@ stub NtUserGetGestureExtArgs
@ stub NtUserGetGestureInfo
@ stub NtUserGetGuiResources
@ stub NtUserGetHDevName
@ stub NtUserGetHimetricScaleFactorFromPixelLocation
@ stdcall -syscall NtUserGetIconInfo(long ptr ptr ptr ptr long)
@ stdcall -syscall NtUserGetIconSize(long long ptr ptr)
@ stub NtUserGetImeHotKey
@ stub NtUserGetImeInfoEx
@ stub NtUserGetInputContainerId
@ stub NtUserGetInputLocaleInfo
@ stub NtUserGetInteractiveControlDeviceInfo
@ stub NtUserGetInteractiveControlInfo
@ stub NtUserGetInteractiveCtrlSupportedWaveforms
@ stdcall -syscall NtUserGetInternalWindowPos(long ptr ptr)
@ stdcall -syscall NtUserGetKeyNameText(long ptr long)
@ stdcall -syscall NtUserGetKeyState(long)
@ stdcall -syscall NtUserGetKeyboardLayout(long)
@ stdcall -syscall NtUserGetKeyboardLayoutList(long ptr)
@ stdcall -syscall NtUserGetKeyboardLayoutName(ptr)
@ stdcall -syscall NtUserGetKeyboardState(ptr)
@ stdcall -syscall NtUserGetLayeredWindowAttributes(long ptr ptr ptr)
@ stub NtUserGetListBoxInfo
@ stdcall -syscall NtUserGetMenuBarInfo(long long long ptr)
@ stub NtUserGetMenuIndex
@ stdcall -syscall NtUserGetMenuItemRect(long long long ptr)
@ stdcall -syscall NtUserGetMessage(ptr long long long)
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
@ stdcall -syscall NtUserGetPointerInfoList(long long long long long ptr ptr ptr)
@ stub NtUserGetPointerInputTransform
@ stub NtUserGetPointerProprietaryId
@ stub NtUserGetPointerType
@ stub NtUserGetPrecisionTouchPadConfiguration
@ stdcall -syscall NtUserGetPriorityClipboardFormat(ptr long)
@ stdcall -syscall NtUserGetProcessDpiAwarenessContext(long)
@ stub NtUserGetProcessUIContextInformation
@ stdcall -syscall NtUserGetProcessWindowStation()
@ stdcall -syscall NtUserGetProp(long wstr)
@ stdcall -syscall NtUserGetQueueStatus(long)
@ stub NtUserGetQueueStatusReadonly
@ stdcall -syscall NtUserGetRawInputBuffer(ptr ptr long)
@ stdcall -syscall NtUserGetRawInputData(ptr long ptr ptr long)
@ stdcall -syscall NtUserGetRawInputDeviceInfo(ptr long ptr ptr)
@ stdcall -syscall NtUserGetRawInputDeviceList(ptr ptr long)
@ stub NtUserGetRawPointerDeviceData
@ stdcall -syscall NtUserGetRegisteredRawInputDevices(ptr ptr long)
@ stub NtUserGetRequiredCursorSizes
@ stub NtUserGetResizeDCompositionSynchronizationObject
@ stdcall -syscall NtUserGetScrollBarInfo(long long ptr)
@ stub NtUserGetSharedWindowData
@ stdcall -syscall NtUserGetSystemDpiForProcess(long)
@ stdcall -syscall NtUserGetSystemMenu(long long)
@ stdcall -syscall NtUserGetThreadDesktop(long)
@ stub NtUserGetThreadState
@ stdcall -syscall NtUserGetTitleBarInfo(long ptr)
@ stub NtUserGetTopLevelWindow
@ stub NtUserGetTouchInputInfo
@ stub NtUserGetTouchValidationStatus
@ stub NtUserGetUniformSpaceMapping
@ stdcall -syscall NtUserGetUpdateRect(long ptr long)
@ stdcall -syscall NtUserGetUpdateRgn(long long long)
@ stdcall -syscall NtUserGetUpdatedClipboardFormats(ptr long ptr)
@ stub NtUserGetWOWClass
@ stub NtUserGetWindowBand
@ stub NtUserGetWindowCompositionAttribute
@ stub NtUserGetWindowCompositionInfo
@ stdcall -syscall NtUserGetWindowDC(long)
@ stub NtUserGetWindowDisplayAffinity
@ stub NtUserGetWindowFeedbackSetting
@ stub NtUserGetWindowGroupId
@ stub NtUserGetWindowMinimizeRect
@ stdcall -syscall NtUserGetWindowPlacement(long ptr)
@ stub NtUserGetWindowProcessHandle
@ stdcall -syscall NtUserGetWindowRgnEx(long long long)
@ stub NtUserGhostWindowFromHungWindow
@ stub NtUserHandleDelegatedInput
@ stub NtUserHardErrorControl
@ stdcall -syscall NtUserHideCaret(long)
@ stub NtUserHidePointerContactVisualization
@ stdcall -syscall NtUserHiliteMenuItem(long long long long)
@ stub NtUserHungWindowFromGhostWindow
@ stub NtUserHwndQueryRedirectionInfo
@ stub NtUserHwndSetRedirectionInfo
@ stub NtUserImpersonateDdeClientWindow
@ stub NtUserInheritWindowMonitor
@ stub NtUserInitTask
@ stub NtUserInitialize
@ stdcall -syscall NtUserInitializeClientPfnArrays(ptr ptr ptr ptr)
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
@ stdcall -syscall NtUserInternalGetWindowIcon(ptr long)
@ stdcall -syscall NtUserInternalGetWindowText(long ptr long)
@ stub NtUserInternalToUnicode
@ stdcall -syscall NtUserInvalidateRect(long ptr long)
@ stdcall -syscall NtUserInvalidateRgn(long long long)
@ stub NtUserIsChildWindowDpiMessageEnabled
@ stdcall -syscall NtUserIsClipboardFormatAvailable(long)
@ stdcall -syscall NtUserIsMouseInPointerEnabled()
@ stub NtUserIsMouseInputEnabled
@ stub NtUserIsNonClientDpiScalingEnabled
@ stub NtUserIsResizeLayoutSynchronizationEnabled
@ stub NtUserIsTopLevelWindow
@ stub NtUserIsTouchWindow
@ stub NtUserIsWindowBroadcastingDpiToChildren
@ stub NtUserIsWindowGDIScaledDpiMessageEnabled
@ stdcall -syscall NtUserKillTimer(long long)
@ stub NtUserLayoutCompleted
@ stub NtUserLinkDpiCursor
@ stub NtUserLoadKeyboardLayoutEx
@ stub NtUserLockCursor
@ stub NtUserLockWindowStation
@ stdcall -syscall NtUserLockWindowUpdate(long)
@ stub NtUserLockWorkStation
@ stdcall -syscall NtUserLogicalToPerMonitorDPIPhysicalPoint(long ptr)
@ stub NtUserLogicalToPhysicalDpiPointForWindow
@ stub NtUserLogicalToPhysicalPoint
@ stub NtUserMNDragLeave
@ stub NtUserMNDragOver
@ stub NtUserMagControl
@ stub NtUserMagGetContextInformation
@ stub NtUserMagSetContextInformation
@ stub NtUserMapPointsByVisualIdentifier
@ stdcall -syscall NtUserMapVirtualKeyEx(long long long)
@ stub NtUserMarkWindowForRawMouse
@ stdcall -syscall NtUserMenuItemFromPoint(long long long long)
@ stdcall -syscall NtUserMessageCall(long long long long long long long)
@ stub NtUserMinInitialize
@ stub NtUserMinMaximize
@ stub NtUserModifyUserStartupInfoFlags
@ stub NtUserModifyWindowTouchCapability
@ stdcall -syscall NtUserMoveWindow(long long long long long long)
@ stdcall -syscall NtUserMsgWaitForMultipleObjectsEx(long ptr long long long)
@ stub NtUserNavigateFocus
@ stdcall -syscall NtUserNotifyIMEStatus(long long)
@ stub NtUserNotifyProcessCreate
@ stdcall -syscall NtUserNotifyWinEvent(long long long long)
@ stdcall -syscall NtUserOpenClipboard(long long)
@ stdcall -syscall NtUserOpenDesktop(ptr long long)
@ stdcall -syscall NtUserOpenInputDesktop(long long long)
@ stub NtUserOpenThreadDesktop
@ stdcall -syscall NtUserOpenWindowStation(ptr long)
@ stub NtUserPaintDesktop
@ stub NtUserPaintMenuBar
@ stub NtUserPaintMonitor
@ stdcall -syscall NtUserPeekMessage(ptr long long long long)
@ stdcall -syscall NtUserPerMonitorDPIPhysicalToLogicalPoint(long ptr)
@ stub NtUserPhysicalToLogicalDpiPointForWindow
@ stub NtUserPhysicalToLogicalPoint
@ stub NtUserPostKeyboardInputMessage
@ stdcall -syscall NtUserPostMessage(long long long long)
@ stdcall -syscall NtUserPostThreadMessage(long long long long)
@ stdcall -syscall NtUserPrintWindow(long long long)
@ stub NtUserProcessConnect
@ stub NtUserProcessInkFeedbackCommand
@ stub NtUserPromoteMouseInPointer
@ stub NtUserPromotePointer
@ stub NtUserQueryActivationObject
@ stub NtUserQueryBSDRWindow
@ stdcall -syscall NtUserQueryDisplayConfig(long ptr ptr ptr ptr ptr)
@ stub NtUserQueryInformationThread
@ stdcall -syscall NtUserQueryInputContext(long long)
@ stub NtUserQuerySendMessage
@ stub NtUserQueryWindow
@ stdcall -syscall NtUserRealChildWindowFromPoint(long long long)
@ stub NtUserRealInternalGetMessage
@ stub NtUserRealWaitMessageEx
@ stdcall -syscall NtUserRedrawWindow(long ptr long long)
@ stub NtUserRegisterBSDRWindow
@ stdcall -syscall NtUserRegisterClassExWOW(ptr ptr ptr ptr long long long)
@ stub NtUserRegisterDManipHook
@ stub NtUserRegisterEdgy
@ stub NtUserRegisterErrorReportingDialog
@ stdcall -syscall NtUserRegisterHotKey(long long long long)
@ stub NtUserRegisterManipulationThread
@ stub NtUserRegisterPointerDeviceNotifications
@ stub NtUserRegisterPointerInputTarget
@ stdcall -syscall NtUserRegisterRawInputDevices(ptr long long)
@ stub NtUserRegisterServicesProcess
@ stub NtUserRegisterSessionPort
@ stub NtUserRegisterShellPTPListener
@ stub NtUserRegisterTasklist
@ stub NtUserRegisterTouchHitTestingWindow
@ stub NtUserRegisterTouchPadCapable
@ stub NtUserRegisterUserApiHook
@ stub NtUserRegisterWindowMessage
@ stdcall -syscall NtUserReleaseDC(long long)
@ stub NtUserReleaseDwmHitTestWaiters
@ stub NtUserRemoteConnect
@ stub NtUserRemoteRedrawRectangle
@ stub NtUserRemoteRedrawScreen
@ stub NtUserRemoteStopScreenUpdates
@ stdcall -syscall NtUserRemoveClipboardFormatListener(long)
@ stub NtUserRemoveInjectionDevice
@ stdcall -syscall NtUserRemoveMenu(long long long)
@ stdcall -syscall NtUserRemoveProp(long wstr)
@ stub NtUserRemoveVisualIdentifier
@ stub NtUserReportInertia
@ stub NtUserRequestMoveSizeOperation
@ stub NtUserResolveDesktopForWOW
@ stub NtUserRestoreWindowDpiChanges
@ stub NtUserSBGetParms
@ stdcall -syscall NtUserScrollDC(long long long ptr ptr long ptr)
@ stdcall -syscall NtUserScrollWindowEx(long long long ptr ptr long ptr long)
@ stdcall -syscall NtUserSelectPalette(long long long)
@ stub NtUserSendEventMessage
@ stdcall -syscall NtUserSendInput(long ptr long)
@ stub NtUserSendInteracsetiveControlHapticsReport
@ stub NtUserSetActivationFilter
@ stub NtUserSetActiveProcessForMonitor
@ stdcall -syscall NtUserSetActiveWindow(long)
@ stub NtUserSetAppImeLevel
@ stub NtUserSetAutoRotation
@ stub NtUserSetBridgeWindowChild
@ stub NtUserSetBrokeredForeground
@ stub NtUserSetCalibrationData
@ stdcall -syscall NtUserSetCapture(long)
@ stub NtUserSetChildWindowNoActivate
@ stdcall -syscall NtUserSetClassLong(long long long long)
@ stdcall -syscall NtUserSetClassLongPtr(long long long long)
@ stdcall -syscall NtUserSetClassWord(long long long)
@ stdcall -syscall NtUserSetClipboardData(long ptr ptr)
@ stdcall -syscall NtUserSetClipboardViewer(long)
@ stub NtUserSetCoreWindow
@ stub NtUserSetCoreWindowPartner
@ stdcall -syscall NtUserSetCursor(long)
@ stub NtUserSetCursorContents
@ stdcall -syscall NtUserSetCursorIconData(long ptr ptr ptr)
@ stdcall -syscall NtUserSetCursorPos(long long)
@ stub NtUserSetDesktopColorTransform
@ stub NtUserSetDesktopVisualInputSink
@ stub NtUserSetDialogControlDpiChangeBehavior
@ stub NtUserSetDisplayAutoRotationPreferences
@ stub NtUserSetDisplayConfig
@ stub NtUserSetDisplayMapping
@ stub NtUserSetFallbackForeground
@ stub NtUserSetFeatureReportResponse
@ stdcall -syscall NtUserSetFocus(long)
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
@ stdcall -syscall NtUserSetInternalWindowPos(long long ptr ptr)
@ stdcall -syscall NtUserSetKeyboardState(ptr)
@ stdcall -syscall NtUserSetLayeredWindowAttributes(ptr long long long)
@ stub NtUserSetMagnificationDesktopMagnifierOffsetsDWMUpdated
@ stub NtUserSetManipulationInputTarget
@ stdcall -syscall NtUserSetMenu(long long)
@ stdcall -syscall NtUserSetMenuContextHelpId(long long)
@ stdcall -syscall NtUserSetMenuDefaultItem(long long long)
@ stub NtUserSetMenuFlagRtoL
@ stub NtUserSetMirrorRendering
@ stub NtUserSetMonitorWorkArea
@ stub NtUserSetMouseInputRateLimitingTimer
@ stdcall -syscall NtUserSetObjectInformation(long long ptr long)
@ stdcall -syscall NtUserSetParent(long long)
@ stub NtUserSetPrecisionTouchPadConfiguration
@ stdcall -syscall NtUserSetProcessDpiAwarenessContext(long long)
@ stub NtUserSetProcessInteractionFlags
@ stub NtUserSetProcessMousewheelRoutingMode
@ stub NtUserSetProcessRestrictionExemption
@ stub NtUserSetProcessUIAccessZorder
@ stdcall -syscall NtUserSetProcessWindowStation(long)
@ stdcall -syscall NtUserSetProp(long wstr ptr)
@ stdcall -syscall NtUserSetScrollInfo(long long ptr long)
@ stub NtUserSetSensorPresence
@ stub NtUserSetSharedWindowData
@ stdcall -syscall NtUserSetShellWindowEx(long long)
@ stdcall -syscall NtUserSetSysColors(long ptr ptr)
@ stub NtUserSetSystemCursor
@ stdcall -syscall NtUserSetSystemMenu(long long)
@ stdcall -syscall NtUserSetSystemTimer(long long long)
@ stub NtUserSetTargetForResourceBrokering
@ stdcall -syscall NtUserSetThreadDesktop(long)
@ stub NtUserSetThreadInputBlocked
@ stub NtUserSetThreadLayoutHandles
@ stub NtUserSetThreadState
@ stdcall -syscall NtUserSetTimer(long long long ptr long)
@ stdcall -syscall NtUserSetWinEventHook(long long long ptr ptr long long long)
@ stub NtUserSetWindowArrangement
@ stub NtUserSetWindowBand
@ stub NtUserSetWindowCompositionAttribute
@ stub NtUserSetWindowCompositionTransition
@ stub NtUserSetWindowDisplayAffinity
@ stub NtUserSetWindowFNID
@ stub NtUserSetWindowFeedbackSetting
@ stub NtUserSetWindowGroup
@ stdcall -syscall NtUserSetWindowLong(long long long long)
@ stdcall -syscall NtUserSetWindowLongPtr(long long long long)
@ stdcall -syscall NtUserSetWindowPlacement(long ptr)
@ stdcall -syscall NtUserSetWindowPos(long long long long long long long)
@ stdcall -syscall NtUserSetWindowRgn(long long long)
@ stub NtUserSetWindowRgnEx
@ stub NtUserSetWindowShowState
@ stub NtUserSetWindowStationUser
@ stdcall -syscall NtUserSetWindowWord(long long long)
@ stub NtUserSetWindowsHookAW
@ stdcall -syscall NtUserSetWindowsHookEx(ptr ptr long long ptr long)
@ stdcall -syscall NtUserShowCaret(long)
@ stdcall -syscall NtUserShowCursor(long)
@ stdcall -syscall NtUserShowScrollBar(long long long)
@ stub NtUserShowSystemCursor
@ stdcall -syscall NtUserShowWindow(long long)
@ stdcall -syscall NtUserShowWindowAsync(long long)
@ stub NtUserShutdownBlockReasonCreate
@ stub NtUserShutdownBlockReasonQuery
@ stub NtUserShutdownReasonDestroy
@ stub NtUserSignalRedirectionStartComplete
@ stub NtUserSlicerControl
@ stub NtUserSoundSentry
@ stub NtUserStopAndEndInertia
@ stub NtUserSwitchDesktop
@ stdcall -syscall NtUserSystemParametersInfo(long long ptr long)
@ stdcall -syscall NtUserSystemParametersInfoForDpi(long long ptr long long)
@ stub NtUserTestForInteractiveUser
@ stdcall -syscall NtUserThunkedMenuInfo(long ptr)
@ stdcall -syscall NtUserThunkedMenuItemInfo(long long long long ptr ptr)
@ stdcall -syscall NtUserToUnicodeEx(long long ptr ptr long long long)
@ stdcall -syscall NtUserTrackMouseEvent(ptr)
@ stdcall -syscall NtUserTrackPopupMenuEx(long long long long long ptr)
@ stub NtUserTransformPoint
@ stub NtUserTransformRect
@ stdcall -syscall NtUserTranslateAccelerator(long long ptr)
@ stdcall -syscall NtUserTranslateMessage(ptr long)
@ stub NtUserUndelegateInput
@ stdcall -syscall NtUserUnhookWinEvent(long)
@ stdcall -syscall NtUserUnhookWindowsHookEx(long)
@ stub NtUserUnloadKeyboardLayout
@ stub NtUserUnlockWindowStation
@ stdcall -syscall NtUserUnregisterClass(ptr ptr ptr)
@ stdcall -syscall NtUserUnregisterHotKey(long long)
@ stub NtUserUnregisterSessionPort
@ stub NtUserUnregisterUserApiHook
@ stub NtUserUpdateDefaultDesktopThumbnail
@ stdcall -syscall NtUserUpdateInputContext(long long ptr)
@ stub NtUserUpdateInstance
@ stdcall -syscall NtUserUpdateLayeredWindow(long long ptr ptr long ptr long ptr long ptr)
@ stub NtUserUpdatePerUserSystemParameters
@ stub NtUserUpdateWindowInputSinkHints
@ stub NtUserUpdateWindowTrackingInfo
@ stub NtUserUserHandleGrantAccess
@ stdcall -syscall NtUserValidateRect(long ptr)
@ stub NtUserValidateTimerCallback
@ stdcall -syscall NtUserVkKeyScanEx(long long)
@ stub NtUserWOWCleanup
@ stub NtUserWaitAvailableMessageEx
@ stdcall -syscall NtUserWaitForInputIdle(long long long)
@ stub NtUserWaitForMsgAndEvent
@ stub NtUserWaitForRedirectionStartComplete
@ stdcall -syscall NtUserWaitMessage()
@ stdcall -syscall NtUserWindowFromDC(long)
@ stub NtUserWindowFromPhysicalPoint
@ stdcall -syscall NtUserWindowFromPoint(long long)
@ stub NtUserYieldTask
@ stub NtValidateCompositionSurfaceHandle
@ stub NtVisualCaptureBits
# extern gDispatchTableValues

################################################################
# Wine internal extensions

@ stdcall -syscall __wine_get_icm_profile(long long ptr ptr)
@ stdcall -syscall __wine_get_file_outline_text_metric(wstr ptr ptr ptr)
@ stdcall -syscall __wine_send_input(long ptr ptr)
