init AVIFILE_DllMain

@ stub    AVIBuildFilter
@ stdcall AVIBuildFilterA(str long long) AVIBuildFilterA
@ stdcall AVIBuildFilterW(wstr long long) AVIBuildFilterW
@ stub    AVIClearClipboard
@ stdcall AVIFileAddRef(ptr) AVIFileAddRef
@ stub    AVIFileCreateStream
@ stdcall AVIFileCreateStreamA(ptr ptr ptr) AVIFileCreateStreamA
@ stdcall AVIFileCreateStreamW(ptr ptr ptr) AVIFileCreateStreamW
@ stdcall AVIFileEndRecord(ptr) AVIFileEndRecord
@ stdcall AVIFileExit() AVIFileExit
@ stdcall AVIFileGetStream(ptr ptr long long) AVIFileGetStream
@ stdcall AVIFileInfo (ptr ptr long) AVIFileInfoA # A in both Win95 and NT
@ stdcall AVIFileInfoA(ptr ptr long) AVIFileInfoA
@ stdcall AVIFileInfoW(ptr ptr long) AVIFileInfoW
@ stdcall AVIFileInit() AVIFileInit
@ stub    AVIFileOpen
@ stdcall AVIFileOpenA(ptr str long ptr) AVIFileOpenA
@ stdcall AVIFileOpenW(ptr str long ptr) AVIFileOpenW
@ stdcall AVIFileReadData(ptr long ptr ptr) AVIFileReadData
@ stdcall AVIFileRelease(ptr) AVIFileRelease
@ stdcall AVIFileWriteData(ptr long ptr long) AVIFileWriteData
@ stub    AVIGetFromClipboard
@ stdcall AVIMakeCompressedStream(ptr ptr ptr ptr) AVIMakeCompressedStream
@ stub    AVIMakeFileFromStreams
@ stub    AVIMakeStreamFromClipboard
@ stub    AVIPutFileOnClipboard
@ stub    AVISave
@ stub    AVISaveA
@ stdcall AVISaveOptions(long long long ptr ptr) AVISaveOptions
@ stdcall AVISaveOptionsFree(long ptr) AVISaveOptionsFree
@ stub    AVISaveV
@ stub    AVISaveVA
@ stub    AVISaveVW
@ stub    AVISaveW
@ stdcall AVIStreamAddRef(ptr) AVIStreamAddRef
@ stub    AVIStreamBeginStreaming
@ stdcall AVIStreamCreate(ptr long long ptr) AVIStreamCreate
@ stub    AVIStreamEndStreaming
@ stdcall AVIStreamFindSample(ptr long long) AVIStreamFindSample
@ stdcall AVIStreamGetFrame(ptr long) AVIStreamGetFrame
@ stdcall AVIStreamGetFrameClose(ptr) AVIStreamGetFrameClose
@ stdcall AVIStreamGetFrameOpen(ptr ptr) AVIStreamGetFrameOpen
@ stdcall AVIStreamInfo (ptr ptr long) AVIStreamInfoA
@ stdcall AVIStreamInfoA(ptr ptr long) AVIStreamInfoA
@ stdcall AVIStreamInfoW(ptr ptr long) AVIStreamInfoW
@ stdcall AVIStreamLength(ptr) AVIStreamLength
@ stub    AVIStreamOpenFromFile
@ stdcall AVIStreamOpenFromFileA(ptr str long long long ptr) AVIStreamOpenFromFileA
@ stdcall AVIStreamOpenFromFileW(ptr wstr long long long ptr) AVIStreamOpenFromFileW
@ stdcall AVIStreamRead(ptr long long ptr long ptr ptr) AVIStreamRead
@ stdcall AVIStreamReadData(ptr long ptr ptr) AVIStreamReadData
@ stdcall AVIStreamReadFormat(ptr long ptr long) AVIStreamReadFormat
@ stdcall AVIStreamRelease(ptr) AVIStreamRelease
@ stdcall AVIStreamSampleToTime(ptr long) AVIStreamSampleToTime
@ stdcall AVIStreamSetFormat(ptr long ptr long) AVIStreamSetFormat
@ stdcall AVIStreamStart(ptr) AVIStreamStart
@ stdcall AVIStreamTimeToSample(ptr long) AVIStreamTimeToSample
@ stdcall AVIStreamWrite(ptr long long ptr long long ptr ptr) AVIStreamWrite
@ stdcall AVIStreamWriteData(ptr long ptr long) AVIStreamWriteData
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
@ extern  IID_IAVIEditStream IID_IAVIEditStream
@ extern  IID_IAVIFile IID_IAVIFile
@ extern  IID_IAVIStream IID_IAVIStream
@ extern  IID_IGetFrame IID_IGetFrame
