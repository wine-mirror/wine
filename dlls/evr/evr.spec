@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stub MFConvertColorInfoFromDXVA
@ stdcall -import MFConvertColorInfoToDXVA(ptr ptr)
@ stub MFConvertFromFP16Array
@ stub MFConvertToFP16Array
@ stdcall -import MFCopyImage(ptr long ptr long long long)
@ stdcall -import MFCreateDXSurfaceBuffer(ptr ptr long ptr)
@ stub MFCreateVideoMediaType
@ stub MFCreateVideoMediaTypeFromBitMapInfoHeader
@ stdcall -import MFCreateVideoMediaTypeFromSubtype(ptr ptr)
@ stub MFCreateVideoMediaTypeFromVideoInfoHeader2
@ stub MFCreateVideoMediaTypeFromVideoInfoHeader
@ stdcall MFCreateVideoMixer(ptr ptr ptr ptr)
@ stdcall MFCreateVideoMixerAndPresenter(ptr ptr ptr ptr ptr ptr)
@ stub MFCreateVideoOTA
@ stub MFCreateVideoPresenter2
@ stdcall MFCreateVideoPresenter(ptr ptr ptr ptr)
@ stdcall MFCreateVideoSampleAllocator(ptr ptr)
@ stdcall MFCreateVideoSampleFromSurface(ptr ptr)
@ stdcall -import MFGetPlaneSize(long long long ptr)
@ stub MFGetStrideForBitmapInfoHeader
@ stub MFGetUncompressedVideoFormat
@ stub MFInitVideoFormat
@ stub MFInitVideoFormat_RGB
@ stub MFIsFormatYUV
