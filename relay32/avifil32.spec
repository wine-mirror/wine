name	avifil32
type	win32

  0 stub    AVIBuildFilter
  1 stub    AVIBuildFilterA
  2 stub    AVIBuildFilterW
  3 stub    AVIClearClipboard
  4 stub    AVIFileAddRef
  5 stub    AVIFileCreateStream
  6 stdcall AVIFileCreateStreamA(ptr ptr ptr) AVIFileCreateStreamA
  7 stub    AVIFileCreateStreamW
  8 stub    AVIFileEndRecord
  9 stdcall AVIFileExit() AVIFileExit
 10 stub    AVIFileGetStream
 11 stdcall AVIFileInfo (ptr ptr long) AVIFileInfoA # A in both Win95 and NT
 12 stdcall AVIFileInfoA(ptr ptr long) AVIFileInfoA
 13 stdcall AVIFileInfoW(ptr ptr long) AVIFileInfoW
 14 stdcall AVIFileInit() AVIFileInit
 15 stub    AVIFileOpen
 16 stdcall AVIFileOpenA(ptr str long ptr) AVIFileOpenA
 17 stub    AVIFileOpenW
 18 stub    AVIFileReadData
 19 stdcall AVIFileRelease(ptr) AVIFileRelease
 20 stub    AVIFileWriteData
 21 stub    AVIGetFromClipboard
 22 stdcall AVIMakeCompressedStream(ptr ptr ptr ptr) AVIMakeCompressedStream
 23 stub    AVIMakeFileFromStreams
 24 stub    AVIMakeStreamFromClipboard
 25 stub    AVIPutFileOnClipboard
 26 stub    AVISave
 27 stub    AVISaveA
 28 stub    AVISaveOptions
 29 stub    AVISaveOptionsFree
 30 stub    AVISaveV
 31 stub    AVISaveVA
 32 stub    AVISaveVW
 33 stub    AVISaveW
 34 stub    AVIStreamAddRef
 35 stub    AVIStreamBeginStreaming
 36 stub    AVIStreamCreate
 37 stub    AVIStreamEndStreaming
 38 stub    AVIStreamFindSample
 39 stdcall AVIStreamGetFrame(ptr long) AVIStreamGetFrame
 40 stdcall AVIStreamGetFrameClose(ptr) AVIStreamGetFrameClose
 41 stdcall AVIStreamGetFrameOpen(ptr ptr) AVIStreamGetFrameOpen
 42 stdcall AVIStreamInfo (ptr ptr long) AVIStreamInfoA
 43 stdcall AVIStreamInfoA(ptr ptr long) AVIStreamInfoA
 44 stdcall AVIStreamInfoW(ptr ptr long) AVIStreamInfoW
 45 stdcall AVIStreamLength(ptr) AVIStreamLength
 46 stub    AVIStreamOpenFromFile
 47 stub    AVIStreamOpenFromFileA
 48 stub    AVIStreamOpenFromFileW
 49 stdcall AVIStreamRead(ptr long long ptr long ptr ptr) AVIStreamRead
 50 stdcall AVIStreamReadData(ptr long ptr ptr) AVIStreamReadData
 51 stdcall AVIStreamReadFormat(ptr long ptr long) AVIStreamReadFormat
 52 stdcall AVIStreamRelease(ptr) AVIStreamRelease
 53 stub    AVIStreamSampleToTime
 54 stdcall AVIStreamSetFormat(ptr long ptr long) AVIStreamSetFormat
 55 stdcall AVIStreamStart(ptr) AVIStreamStart
 56 stub    AVIStreamTimeToSample
 57 stdcall AVIStreamWrite(ptr long long ptr long long ptr ptr) AVIStreamWrite
 58 stdcall AVIStreamWriteData(ptr long ptr long) AVIStreamWriteData
 59 stub    CLSID_AVISimpleUnMarshal
 60 stub    CreateEditableStream
 61 stub    DllCanUnloadNow
 62 stub    DllGetClassObject
 63 stub    EditStreamClone
 64 stub    EditStreamCopy
 65 stub    EditStreamCut
 66 stub    EditStreamPaste
 67 stub    EditStreamSetInfo
 68 stub    EditStreamSetInfoA
 69 stub    EditStreamSetInfoW
 70 stub    EditStreamSetName
 71 stub    EditStreamSetNameA
 72 stub    EditStreamSetNameW
 73 stub    IID_IAVIEditStream
 74 stub    IID_IAVIFile
 75 stub    IID_IAVIStream
 76 stub    IID_IGetFrame
