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
@ stdcall MFAddPeriodicCallback(ptr ptr ptr)
@ stdcall MFAllocateWorkQueue(ptr)
@ stdcall MFAllocateWorkQueueEx(long ptr)
@ stub MFAppendCollection
@ stub MFAverageTimePerFrameToFrameRate
@ stdcall MFBeginCreateFile(long long long wstr ptr ptr ptr)
@ stub MFBeginGetHostByName
@ stub MFBeginRegisterWorkQueueWithMMCSS
@ stub MFBeginUnregisterWorkQueueWithMMCSS
@ stub MFBlockThread
@ stub MFCalculateBitmapImageSize
@ stdcall MFCalculateImageSize(ptr long long ptr)
@ stdcall MFCancelCreateFile(ptr)
@ stdcall MFCancelWorkItem(int64)
@ stdcall MFCompareFullToPartialMediaType(ptr ptr)
@ stub MFCompareSockaddrAddresses
@ stub MFConvertColorInfoFromDXVA
@ stub MFConvertColorInfoToDXVA
@ stub MFConvertFromFP16Array
@ stub MFConvertToFP16Array
@ stdcall MFCopyImage(ptr long ptr long long long)
@ stub MFCreateAMMediaTypeFromMFMediaType
@ stdcall MFCreateAlignedMemoryBuffer(long long ptr)
@ stdcall MFCreateAsyncResult(ptr ptr ptr ptr)
@ stdcall MFCreateAttributes(ptr long)
@ stub MFCreateAudioMediaType
@ stdcall MFCreateCollection(ptr)
@ stdcall MFCreateEventQueue(ptr)
@ stdcall MFCreateFile(long long long wstr ptr)
@ stub MFCreateLegacyMediaBufferOnMFMediaBuffer
@ stdcall MFCreateMFByteStreamOnStream(ptr ptr)
@ stdcall MFCreateMFByteStreamOnStreamEx(ptr ptr)
@ stdcall MFCreateMFByteStreamWrapper(ptr ptr)
@ stub MFCreateMFVideoFormatFromMFMediaType
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
@ stub MFCreateTransformActivate
@ stub MFCreateURLFromPath
@ stub MFCreateUdpSockets
@ stub MFCreateVideoMediaType
@ stub MFCreateVideoMediaTypeFromBitMapInfoHeader
@ stub MFCreateVideoMediaTypeFromBitMapInfoHeaderEx
@ stub MFCreateVideoMediaTypeFromSubtype
@ stub MFCreateVideoMediaTypeFromVideoInfoHeader2
@ stub MFCreateVideoMediaTypeFromVideoInfoHeader
@ stdcall MFCreateWaveFormatExFromMFMediaType(ptr ptr ptr long)
@ stub MFDeserializeAttributesFromStream
@ stub MFDeserializeEvent
@ stub MFDeserializeMediaTypeFromStream
@ stub MFDeserializePresentationDescriptor
@ stdcall MFEndCreateFile(ptr ptr)
@ stub MFEndGetHostByName
@ stub MFEndRegisterWorkQueueWithMMCSS
@ stub MFEndUnregisterWorkQueueWithMMCSS
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
@ stub MFGetPlaneSize
@ stub MFGetPlatform
@ stdcall MFGetPluginControl(ptr)
@ stub MFGetPrivateWorkqueues
@ stub MFGetSockaddrFromNumericName
@ stub MFGetStrideForBitmapInfoHeader
@ stdcall MFGetSystemTime()
@ stdcall MFGetTimerPeriodicity(ptr)
@ stub MFGetUncompressedVideoFormat
@ stub MFGetWorkQueueMMCSSClass
@ stub MFGetWorkQueueMMCSSTaskId
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
@ stub MFInitMediaTypeFromWaveFormatEx
@ stub MFInitVideoFormat
@ stub MFInitVideoFormat_RGB
@ stdcall MFInvokeCallback(ptr)
@ stub MFJoinIoPort
@ stdcall MFLockPlatform()
@ stdcall MFLockWorkQueue(long)
@ stdcall MFPutWaitingWorkItem(long long ptr ptr)
@ stdcall MFPutWorkItem(long ptr ptr)
@ stdcall MFPutWorkItem2(long long ptr ptr)
@ stdcall MFPutWorkItemEx(long ptr)
@ stdcall MFPutWorkItemEx2(long long ptr)
@ stub MFRecordError
@ stdcall MFRegisterLocalByteStreamHandler(wstr wstr ptr)
@ stdcall MFRegisterLocalSchemeHandler(wstr ptr)
@ stdcall MFRemovePeriodicCallback(long)
@ stdcall MFScheduleWorkItem(ptr ptr int64 ptr)
@ stdcall MFScheduleWorkItemEx(ptr int64 ptr)
@ stub MFSerializeAttributesToStream
@ stub MFSerializeEvent
@ stub MFSerializeMediaTypeToStream
@ stub MFSerializePresentationDescriptor
@ stub MFSetSockaddrAny
@ stdcall MFShutdown()
@ stdcall MFStartup(long long)
@ stub MFStreamDescriptorProtectMediaType
@ stdcall MFTEnum(int128 long ptr ptr ptr ptr ptr)
@ stdcall MFTEnumEx(int128 long ptr ptr ptr ptr)
@ stub MFTGetInfo
@ stdcall MFTRegister(int128 int128 wstr long long ptr long ptr ptr)
@ stdcall MFTRegisterLocal(ptr ptr wstr long long  ptr long ptr)
@ stub MFTRegisterLocalByCLSID
@ stdcall MFTUnregister(int128)
@ stdcall MFTUnregisterLocal(ptr)
@ stub MFTUnregisterLocalByCLSID
@ stub MFTraceError
@ stub MFTraceFuncEnter
@ stub MFUnblockThread
@ stdcall MFUnlockPlatform()
@ stdcall MFUnlockWorkQueue(long)
@ stdcall MFUnwrapMediaType(ptr ptr)
@ stub MFValidateMediaTypeSize
@ stdcall MFWrapMediaType(ptr ptr ptr ptr)
@ stub MFllMulDiv
@ stub PropVariantFromStream
@ stub PropVariantToStream
