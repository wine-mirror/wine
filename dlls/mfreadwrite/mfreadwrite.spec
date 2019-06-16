@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stdcall MFCreateSinkWriterFromMediaSink(ptr ptr ptr)
@ stub MFCreateSinkWriterFromURL
@ stdcall MFCreateSourceReaderFromByteStream(ptr ptr ptr)
@ stdcall MFCreateSourceReaderFromMediaSource(ptr ptr ptr)
@ stdcall MFCreateSourceReaderFromURL(wstr ptr ptr)
