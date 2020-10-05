@ stub FormatTagFromWfx
@ stub MFCreateGuid
@ stub MFGetIoPortHandle
@ stub MFGetPlatformVersion
@ stub MFGetRandomNumber
@ stub MFIsFeatureEnabled
@ stub MFIsQueueThread
@ stub MFPlatformBigEndian
@ stub MFPlatformLittleEndian
@ stub ValidateWaveFormat
@ stub CopyPropVariant
@ stub CreatePropVariant
@ stdcall CreatePropertyStore(ptr)
@ stub DestroyPropVariant
@ stub GetAMSubtypeFromD3DFormat
@ stub GetD3DFormatFromMFSubtype
@ stub LFGetGlobalPool
@ stdcall MFAddPeriodicCallback(ptr ptr ptr) rtworkq.RtwqAddPeriodicCallback
@ stdcall MFAllocateSerialWorkQueue(long ptr) rtworkq.RtwqAllocateSerialWorkQueue
@ stdcall MFAllocateWorkQueue(ptr)
@ stdcall MFAllocateWorkQueueEx(long ptr) rtworkq.RtwqAllocateWorkQueue
@ stub MFAppendCollection
@ stub MFAverageTimePerFrameToFrameRate
@ stdcall MFBeginCreateFile(long long long wstr ptr ptr ptr)
@ stub MFBeginGetHostByName
@ stub MFBeginRegisterWorkQueueWithMMCSS
@ stdcall MFBeginRegisterWorkQueueWithMMCSSEx(long wstr long long ptr ptr) rtworkq.RtwqBeginRegisterWorkQueueWithMMCSS
@ stdcall MFBeginUnregisterWorkQueueWithMMCSS(long ptr ptr) rtworkq.RtwqBeginUnregisterWorkQueueWithMMCSS
@ stub MFBlockThread
@ stub MFCalculateBitmapImageSize
@ stdcall MFCalculateImageSize(ptr long long ptr)
@ stdcall MFCancelCreateFile(ptr)
@ stdcall MFCancelWorkItem(int64) rtworkq.RtwqCancelWorkItem
@ stdcall MFCompareFullToPartialMediaType(ptr ptr)
@ stub MFCompareSockaddrAddresses
@ stub MFConvertColorInfoFromDXVA
@ stdcall MFConvertColorInfoToDXVA(ptr ptr)
@ stub MFConvertFromFP16Array
@ stub MFConvertToFP16Array
@ stdcall MFCopyImage(ptr long ptr long long long)
@ stdcall MFCreate2DMediaBuffer(long long long long ptr)
@ stub MFCreateAMMediaTypeFromMFMediaType
@ stdcall MFCreateAlignedMemoryBuffer(long long ptr)
@ stdcall MFCreateAsyncResult(ptr ptr ptr ptr) rtworkq.RtwqCreateAsyncResult
@ stdcall MFCreateAttributes(ptr long)
@ stub MFCreateAudioMediaType
@ stdcall MFCreateCollection(ptr)
@ stdcall MFCreateDXGIDeviceManager(ptr ptr)
@ stdcall MFCreateEventQueue(ptr)
@ stdcall MFCreateFile(long long long wstr ptr)
@ stub MFCreateLegacyMediaBufferOnMFMediaBuffer
@ stdcall MFCreateMFByteStreamOnStream(ptr ptr)
@ stdcall MFCreateMFByteStreamOnStreamEx(ptr ptr)
@ stdcall MFCreateMFByteStreamWrapper(ptr ptr)
@ stdcall MFCreateMFVideoFormatFromMFMediaType(ptr ptr ptr)
@ stdcall MFCreateMediaBufferFromMediaType(ptr int64 long long ptr)
@ stub MFCreateMediaBufferWrapper
@ stdcall MFCreateMediaEvent(long ptr long ptr ptr)
@ stdcall MFCreateMediaType(ptr)
@ stub MFCreateMediaTypeFromRepresentation
@ stdcall MFCreateMemoryBuffer(long ptr)
@ stub MFCreateMemoryStream
@ stub MFCreatePathFromURL
@ stdcall MFCreatePresentationDescriptor(long ptr ptr)
@ stdcall MFCreateSample(ptr)
@ stub MFCreateSocket
@ stub MFCreateSocketListener
@ stdcall MFCreateSourceResolver(ptr)
@ stdcall MFCreateStreamDescriptor(long long ptr ptr)
@ stdcall MFCreateSystemTimeSource(ptr)
@ stub MFCreateSystemUnderlyingClock
@ stub MFCreateTempFile
@ stdcall MFCreateTrackedSample(ptr)
@ stdcall MFCreateTransformActivate(ptr)
@ stub MFCreateURLFromPath
@ stub MFCreateUdpSockets
@ stub MFCreateVideoMediaType
@ stub MFCreateVideoMediaTypeFromBitMapInfoHeader
@ stub MFCreateVideoMediaTypeFromBitMapInfoHeaderEx
@ stdcall MFCreateVideoMediaTypeFromSubtype(ptr ptr)
@ stub MFCreateVideoMediaTypeFromVideoInfoHeader2
@ stub MFCreateVideoMediaTypeFromVideoInfoHeader
@ stdcall MFCreateWaveFormatExFromMFMediaType(ptr ptr ptr long)
@ stub MFDeserializeAttributesFromStream
@ stub MFDeserializeEvent
@ stub MFDeserializeMediaTypeFromStream
@ stub MFDeserializePresentationDescriptor
@ stdcall MFEndCreateFile(ptr ptr)
@ stub MFEndGetHostByName
@ stdcall MFEndRegisterWorkQueueWithMMCSS(ptr ptr) rtworkq.RtwqEndRegisterWorkQueueWithMMCSS
@ stdcall MFEndUnregisterWorkQueueWithMMCSS(ptr) rtworkq.RtwqEndUnregisterWorkQueueWithMMCSS
@ stub MFFrameRateToAverageTimePerFrame
@ stub MFFreeAdaptersAddresses
@ stub MFGetAdaptersAddresses
@ stdcall MFGetAttributesAsBlob(ptr ptr long)
@ stdcall MFGetAttributesAsBlobSize(ptr ptr)
@ stub MFGetConfigurationDWORD
@ stub MFGetConfigurationPolicy
@ stub MFGetConfigurationStore
@ stub MFGetConfigurationString
@ stub MFGetMFTMerit
@ stub MFGetNumericNameFromSockaddr
@ stdcall MFGetPlaneSize(long long long ptr)
@ stub MFGetPlatform
@ stdcall MFGetPluginControl(ptr)
@ stub MFGetPrivateWorkqueues
@ stub MFGetSockaddrFromNumericName
@ stdcall MFGetStrideForBitmapInfoHeader(long long ptr)
@ stdcall MFGetSystemTime()
@ stdcall MFGetTimerPeriodicity(ptr)
@ stub MFGetUncompressedVideoFormat
@ stdcall MFGetWorkQueueMMCSSClass(long ptr ptr) rtworkq.RtwqGetWorkQueueMMCSSClass
@ stdcall MFGetWorkQueueMMCSSTaskId(long ptr) rtworkq.RtwqGetWorkQueueMMCSSTaskId
@ stdcall MFGetWorkQueueMMCSSPriority(long ptr) rtworkq.RtwqGetWorkQueueMMCSSPriority
@ stdcall MFHeapAlloc(long long str long long)
@ stdcall MFHeapFree(ptr)
@ stub MFInitAMMediaTypeFromMFMediaType
@ stdcall MFInitAttributesFromBlob(ptr ptr long)
@ stub MFInitMediaTypeFromAMMediaType
@ stub MFInitMediaTypeFromMFVideoFormat
@ stub MFInitMediaTypeFromMPEG1VideoInfo
@ stub MFInitMediaTypeFromMPEG2VideoInfo
@ stub MFInitMediaTypeFromVideoInfoHeader2
@ stub MFInitMediaTypeFromVideoInfoHeader
@ stdcall MFInitMediaTypeFromWaveFormatEx(ptr ptr long)
@ stub MFInitVideoFormat
@ stub MFInitVideoFormat_RGB
@ stdcall MFInvokeCallback(ptr)
@ stub MFJoinIoPort
@ stdcall MFJoinWorkQueue(long long ptr) rtworkq.RtwqJoinWorkQueue
@ stdcall MFLockPlatform() rtworkq.RtwqLockPlatform
@ stdcall MFLockWorkQueue(long) rtworkq.RtwqLockWorkQueue
@ stdcall MFPutWaitingWorkItem(long long ptr ptr) rtworkq.RtwqPutWaitingWorkItem
@ stdcall MFPutWorkItem(long ptr ptr)
@ stdcall MFPutWorkItem2(long long ptr ptr)
@ stdcall MFPutWorkItemEx(long ptr)
@ stdcall MFPutWorkItemEx2(long long ptr)
@ stub MFRecordError
@ stdcall MFRegisterLocalByteStreamHandler(wstr wstr ptr)
@ stdcall MFRegisterLocalSchemeHandler(wstr ptr)
@ stdcall MFRegisterPlatformWithMMCSS(wstr ptr long) rtworkq.RtwqRegisterPlatformWithMMCSS
@ stdcall MFRemovePeriodicCallback(long) rtworkq.RtwqRemovePeriodicCallback
@ stdcall MFScheduleWorkItem(ptr ptr int64 ptr)
@ stdcall MFScheduleWorkItemEx(ptr int64 ptr) rtworkq.RtwqScheduleWorkItem
@ stub MFSerializeAttributesToStream
@ stub MFSerializeEvent
@ stub MFSerializeMediaTypeToStream
@ stub MFSerializePresentationDescriptor
@ stub MFSetSockaddrAny
@ stdcall MFShutdown()
@ stdcall MFStartup(long long)
@ stub MFStreamDescriptorProtectMediaType
@ stdcall MFTEnum(int128 long ptr ptr ptr ptr ptr)
@ stdcall MFTEnum2(int128 long ptr ptr ptr ptr ptr)
@ stdcall MFTEnumEx(int128 long ptr ptr ptr ptr)
@ stub MFTGetInfo
@ stdcall MFTRegister(int128 int128 wstr long long ptr long ptr ptr)
@ stdcall MFTRegisterLocal(ptr ptr wstr long long  ptr long ptr)
@ stdcall MFTRegisterLocalByCLSID(ptr ptr wstr long long ptr long ptr)
@ stdcall MFTUnregister(int128)
@ stdcall MFTUnregisterLocal(ptr)
@ stdcall MFTUnregisterLocalByCLSID(int128)
@ stdcall MFUnregisterPlatformFromMMCSS() rtworkq.RtwqUnregisterPlatformFromMMCSS
@ stub MFTraceError
@ stub MFTraceFuncEnter
@ stub MFUnblockThread
@ stdcall MFUnjoinWorkQueue(long long) rtworkq.RtwqUnjoinWorkQueue
@ stdcall MFUnlockPlatform() rtworkq.RtwqUnlockPlatform
@ stdcall MFUnlockWorkQueue(long) rtworkq.RtwqUnlockWorkQueue
@ stdcall MFUnwrapMediaType(ptr ptr)
@ stub MFValidateMediaTypeSize
@ stdcall MFWrapMediaType(ptr ptr ptr ptr)
@ stub MFllMulDiv
@ stub PropVariantFromStream
@ stub PropVariantToStream
