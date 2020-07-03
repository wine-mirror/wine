@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stub MFConvertColorInfoFromDXVA
@ stub MFConvertColorInfoToDXVA
@ stub MFConvertFromFP16Array
@ stub MFConvertToFP16Array
@ stub MFCopyImage
@ stub MFCreateDXSurfaceBuffer
@ stub MFCreateVideoMediaType
@ stub MFCreateVideoMediaTypeFromBitMapInfoHeader
@ stub MFCreateVideoMediaTypeFromSubtype
@ stub MFCreateVideoMediaTypeFromVideoInfoHeader2
@ stub MFCreateVideoMediaTypeFromVideoInfoHeader
@ stdcall MFCreateVideoMixer(ptr ptr ptr ptr)
@ stdcall MFCreateVideoMixerAndPresenter(ptr ptr ptr ptr ptr ptr)
@ stub MFCreateVideoOTA
@ stub MFCreateVideoPresenter2
@ stdcall MFCreateVideoPresenter(ptr ptr ptr ptr)
@ stdcall MFCreateVideoSampleAllocator(ptr ptr)
@ stdcall MFCreateVideoSampleFromSurface(ptr ptr)
@ stub MFGetPlaneSize
@ stub MFGetStrideForBitmapInfoHeader
@ stub MFGetUncompressedVideoFormat
@ stub MFInitVideoFormat
@ stub MFInitVideoFormat_RGB
@ stub MFIsFormatYUV
