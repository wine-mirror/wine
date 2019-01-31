@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stub MFCreateSinkWriterFromMediaSink
@ stub MFCreateSinkWriterFromURL
@ stdcall MFCreateSourceReaderFromByteStream(ptr ptr ptr)
@ stdcall MFCreateSourceReaderFromMediaSource(ptr ptr ptr)
@ stub MFCreateSourceReaderFromURL
