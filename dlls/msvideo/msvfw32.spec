# Yes, ICCompress,ICDecompress,MCIWnd* and ICDraw* are cdecl (VFWAPIV).
# The rest is stdcall (VFWAPI) however. -Marcus Meissner, 990124

2 stdcall VideoForWindowsVersion() VideoForWindowsVersion

@ stdcall DrawDibBegin(long long long long ptr long long long) DrawDibBegin
@ stub    DrawDibChangePalette
@ stdcall DrawDibClose(long) DrawDibClose
@ stdcall DrawDibDraw(long long long long long long ptr ptr long long long long long) DrawDibDraw
@ stdcall DrawDibEnd(long) DrawDibEnd
@ stub    DrawDibGetBuffer
@ stdcall DrawDibGetPalette(long) DrawDibGetPalette
@ stdcall DrawDibOpen() DrawDibOpen
@ stub    DrawDibProfileDisplay
@ stdcall DrawDibRealize(long long long) DrawDibRealize
@ stdcall DrawDibSetPalette(long long) DrawDibSetPalette
@ stdcall DrawDibStart(long long) DrawDibStart
@ stdcall DrawDibStop(long) DrawDibStop
@ stub    DrawDibTime
@ stub    GetOpenFileNamePreview
@ stub    GetOpenFileNamePreviewA
@ stub    GetOpenFileNamePreviewW
@ stub    GetSaveFileNamePreviewA
@ stub    GetSaveFileNamePreviewW
@ stdcall ICClose(long) ICClose
@ cdecl   ICCompress(long long ptr ptr ptr ptr ptr ptr long long long ptr ptr) ICCompress
@ stdcall ICCompressorChoose(long long ptr ptr ptr ptr) ICCompressorChoose
@ stdcall ICCompressorFree(ptr) ICCompressorFree
@ cdecl   ICDecompress(long long ptr ptr ptr ptr) ICDecompress
@ cdecl   ICDraw(long long ptr ptr long long) ICDraw
@ cdecl   ICDrawBegin(long long long long long long long long long ptr long long long long long long) ICDrawBegin
@ stdcall ICGetDisplayFormat(long ptr ptr long long long) ICGetDisplayFormat
@ stdcall ICGetInfo(long ptr long) ICGetInfo
@ stdcall ICImageCompress(long long ptr ptr ptr long ptr) ICImageCompress
@ stdcall ICImageDecompress(long long ptr ptr ptr) ICImageDecompress
@ stdcall ICInfo(long long ptr) ICInfo
@ stub    ICInstall
@ stdcall ICLocate(long long ptr ptr long) ICLocate
@ stub    ICMThunk
@ stdcall ICOpen(long long long) ICOpen
@ stdcall ICOpenFunction(long long long ptr) ICOpenFunction
@ stub    ICRemove
@ stdcall ICSendMessage(long long long long) ICSendMessage
@ stub    ICSeqCompressFrame
@ stub    ICSeqCompressFrameEnd
@ stub    ICSeqCompressFrameStart
@ cdecl   MCIWndCreate (long long long str) MCIWndCreateA
@ cdecl   MCIWndCreateA (long long long str) MCIWndCreateA
@ cdecl   MCIWndCreateW (long long long wstr) MCIWndCreateW
@ stdcall MCIWndRegisterClass (long) MCIWndRegisterClass
@ stub    StretchDIB
