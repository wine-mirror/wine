@ stub    AVIBuildFilter
@ stdcall AVIBuildFilterA(str long long)
@ stdcall AVIBuildFilterW(wstr long long)
@ stdcall AVIClearClipboard()
@ stdcall AVIFileAddRef(ptr)
@ stub    AVIFileCreateStream
@ stdcall AVIFileCreateStreamA(ptr ptr ptr)
@ stdcall AVIFileCreateStreamW(ptr ptr ptr)
@ stdcall AVIFileEndRecord(ptr)
@ stdcall AVIFileExit()
@ stdcall AVIFileGetStream(ptr ptr long long)
@ stdcall AVIFileInfo (ptr ptr long) AVIFileInfoA # A in both Win95 and NT
@ stdcall AVIFileInfoA(ptr ptr long)
@ stdcall AVIFileInfoW(ptr ptr long)
@ stdcall AVIFileInit()
@ stub    AVIFileOpen
@ stdcall AVIFileOpenA(ptr str long ptr)
@ stdcall AVIFileOpenW(ptr wstr long ptr)
@ stdcall AVIFileReadData(ptr long ptr ptr)
@ stdcall AVIFileRelease(ptr)
@ stdcall AVIFileWriteData(ptr long ptr long)
@ stdcall AVIGetFromClipboard(ptr)
@ stdcall AVIMakeCompressedStream(ptr ptr ptr ptr)
@ stdcall AVIMakeFileFromStreams(ptr long ptr)
@ stub    AVIMakeStreamFromClipboard
@ stdcall AVIPutFileOnClipboard(ptr)
@ stub    AVISave
@ stub    AVISaveA
@ stdcall AVISaveOptions(long long long ptr ptr)
@ stdcall AVISaveOptionsFree(long ptr)
@ stub    AVISaveV
@ stdcall AVISaveVA(str ptr ptr long ptr ptr)
@ stdcall AVISaveVW(wstr ptr ptr long ptr ptr)
@ stub    AVISaveW
@ stdcall AVIStreamAddRef(ptr)
@ stdcall AVIStreamBeginStreaming(ptr long long long)
@ stdcall AVIStreamCreate(ptr long long ptr)
@ stdcall AVIStreamEndStreaming(ptr) 
@ stdcall AVIStreamFindSample(ptr long long)
@ stdcall AVIStreamGetFrame(ptr long)
@ stdcall AVIStreamGetFrameClose(ptr)
@ stdcall AVIStreamGetFrameOpen(ptr ptr)
@ stdcall AVIStreamInfo (ptr ptr long) AVIStreamInfoA
@ stdcall AVIStreamInfoA(ptr ptr long)
@ stdcall AVIStreamInfoW(ptr ptr long)
@ stdcall AVIStreamLength(ptr)
@ stub    AVIStreamOpenFromFile
@ stdcall AVIStreamOpenFromFileA(ptr str long long long ptr)
@ stdcall AVIStreamOpenFromFileW(ptr wstr long long long ptr)
@ stdcall AVIStreamRead(ptr long long ptr long ptr ptr)
@ stdcall AVIStreamReadData(ptr long ptr ptr)
@ stdcall AVIStreamReadFormat(ptr long ptr long)
@ stdcall AVIStreamRelease(ptr)
@ stdcall AVIStreamSampleToTime(ptr long)
@ stdcall AVIStreamSetFormat(ptr long ptr long)
@ stdcall AVIStreamStart(ptr)
@ stdcall AVIStreamTimeToSample(ptr long)
@ stdcall AVIStreamWrite(ptr long long ptr long long ptr ptr)
@ stdcall AVIStreamWriteData(ptr long ptr long)
@ stub    CLSID_AVISimpleUnMarshal
@ stdcall CreateEditableStream(ptr ptr)
@ stdcall -private DllCanUnloadNow() AVIFILE_DllCanUnloadNow
@ stdcall -private DllGetClassObject(ptr ptr ptr) AVIFILE_DllGetClassObject
@ stdcall -private DllRegisterServer() AVIFILE_DllRegisterServer
@ stdcall -private DllUnregisterServer() AVIFILE_DllUnregisterServer
@ stdcall EditStreamClone(ptr ptr)
@ stdcall EditStreamCopy(ptr ptr ptr ptr)
@ stdcall EditStreamCut(ptr ptr ptr ptr)
@ stdcall EditStreamPaste(ptr ptr ptr ptr long long)
@ stub    EditStreamSetInfo
@ stdcall EditStreamSetInfoA(ptr ptr long)
@ stdcall EditStreamSetInfoW(ptr ptr long)
@ stub    EditStreamSetName
@ stdcall EditStreamSetNameA(ptr str)
@ stdcall EditStreamSetNameW(ptr wstr)
@ extern  IID_IAVIEditStream
@ extern  IID_IAVIFile
@ extern  IID_IAVIStream
@ extern  IID_IGetFrame
