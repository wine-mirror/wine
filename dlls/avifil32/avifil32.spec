name	avifil32
type	win32

@ stub    AVIBuildFilter
@ stub    AVIBuildFilterA
@ stub    AVIBuildFilterW
@ stub    AVIClearClipboard
@ stub    AVIFileAddRef
@ stub    AVIFileCreateStream
@ stdcall AVIFileCreateStreamA(ptr ptr ptr) AVIFileCreateStreamA
@ stdcall AVIFileCreateStreamW(ptr ptr ptr) AVIFileCreateStreamW
@ stub    AVIFileEndRecord
@ stdcall AVIFileExit() AVIFileExit
@ stdcall AVIFileGetStream(ptr ptr long long) AVIFileGetStream
@ stdcall AVIFileInfo (ptr ptr long) AVIFileInfoA # A in both Win95 and NT
@ stdcall AVIFileInfoA(ptr ptr long) AVIFileInfoA
@ stdcall AVIFileInfoW(ptr ptr long) AVIFileInfoW
@ stdcall AVIFileInit() AVIFileInit
@ stub    AVIFileOpen
@ stdcall AVIFileOpenA(ptr str long ptr) AVIFileOpenA
@ stub    AVIFileOpenW
@ stub    AVIFileReadData
@ stdcall AVIFileRelease(ptr) AVIFileRelease
@ stub    AVIFileWriteData
@ stub    AVIGetFromClipboard
@ stdcall AVIMakeCompressedStream(ptr ptr ptr ptr) AVIMakeCompressedStream
@ stub    AVIMakeFileFromStreams
@ stub    AVIMakeStreamFromClipboard
@ stub    AVIPutFileOnClipboard
@ stub    AVISave
@ stub    AVISaveA
@ stub    AVISaveOptions
@ stub    AVISaveOptionsFree
@ stub    AVISaveV
@ stub    AVISaveVA
@ stub    AVISaveVW
@ stub    AVISaveW
@ stub    AVIStreamAddRef
@ stub    AVIStreamBeginStreaming
@ stub    AVIStreamCreate
@ stub    AVIStreamEndStreaming
@ stub    AVIStreamFindSample
@ stdcall AVIStreamGetFrame(ptr long) AVIStreamGetFrame
@ stdcall AVIStreamGetFrameClose(ptr) AVIStreamGetFrameClose
@ stdcall AVIStreamGetFrameOpen(ptr ptr) AVIStreamGetFrameOpen
@ stdcall AVIStreamInfo (ptr ptr long) AVIStreamInfoA
@ stdcall AVIStreamInfoA(ptr ptr long) AVIStreamInfoA
@ stdcall AVIStreamInfoW(ptr ptr long) AVIStreamInfoW
@ stdcall AVIStreamLength(ptr) AVIStreamLength
@ stub    AVIStreamOpenFromFile
@ stub    AVIStreamOpenFromFileA
@ stub    AVIStreamOpenFromFileW
@ stdcall AVIStreamRead(ptr long long ptr long ptr ptr) AVIStreamRead
@ stdcall AVIStreamReadData(ptr long ptr ptr) AVIStreamReadData
@ stdcall AVIStreamReadFormat(ptr long ptr long) AVIStreamReadFormat
@ stdcall AVIStreamRelease(ptr) AVIStreamRelease
@ stub    AVIStreamSampleToTime
@ stdcall AVIStreamSetFormat(ptr long ptr long) AVIStreamSetFormat
@ stdcall AVIStreamStart(ptr) AVIStreamStart
@ stub    AVIStreamTimeToSample
@ stdcall AVIStreamWrite(ptr long long ptr long long ptr ptr) AVIStreamWrite
@ stdcall AVIStreamWriteData(ptr long ptr long) AVIStreamWriteData
@ stub    CLSID_AVISimpleUnMarshal
@ stub    CreateEditableStream
@ stub    DllCanUnloadNow
@ stub    DllGetClassObject
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
@ stub    IID_IAVIEditStream
@ stub    IID_IAVIFile
@ stub    IID_IAVIStream
@ stub    IID_IGetFrame
