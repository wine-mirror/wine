@ stub    AVIBuildFilter
@ stdcall AVIBuildFilterA(str long long)
@ stdcall AVIBuildFilterW(wstr long long)
@ stub    AVIClearClipboard
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
@ stdcall AVIFileOpenW(ptr str long ptr)
@ stdcall AVIFileReadData(ptr long ptr ptr)
@ stdcall AVIFileRelease(ptr)
@ stdcall AVIFileWriteData(ptr long ptr long)
@ stub    AVIGetFromClipboard
@ stdcall AVIMakeCompressedStream(ptr ptr ptr ptr)
@ stub    AVIMakeFileFromStreams
@ stub    AVIMakeStreamFromClipboard
@ stub    AVIPutFileOnClipboard
@ stub    AVISave
@ stub    AVISaveA
@ stdcall AVISaveOptions(long long long ptr ptr)
@ stdcall AVISaveOptionsFree(long ptr)
@ stub    AVISaveV
@ stub    AVISaveVA
@ stub    AVISaveVW
@ stub    AVISaveW
@ stdcall AVIStreamAddRef(ptr)
@ stub    AVIStreamBeginStreaming
@ stdcall AVIStreamCreate(ptr long long ptr)
@ stub    AVIStreamEndStreaming
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
@ stub    CreateEditableStream
@ stdcall DllCanUnloadNow() AVIFILE_DllCanUnloadNow
@ stdcall DllGetClassObject(ptr ptr ptr) AVIFILE_DllGetClassObject
@ stub    EditStreamClone
@ stub    EditStreamCopy
@ stub    EditStreamCut
@ stub    EditStreamPaste
@ stub    EditStreamSetInfo
@ stub    EditStreamSetInfoA
@ stub    EditStreamSetInfoW
@ stub    EditStreamSetName
@ stub    EditStreamSetNameA
@ stub    EditStreamSetNameW
@ extern  IID_IAVIEditStream
@ extern  IID_IAVIFile
@ extern  IID_IAVIStream
@ extern  IID_IGetFrame
