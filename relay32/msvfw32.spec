name	msvfw32
type	win32

  2 stdcall VideoForWindowsVersion() VideoForWindowsVersion
  3 stub    DrawDibBegin
  4 stub    DrawDibChangePalette
  5 stub    DrawDibClose
  6 stub    DrawDibDraw
  7 stub    DrawDibEnd
  8 stub    DrawDibGetBuffer
  9 stub    DrawDibGetPalette
 10 stdcall DrawDibOpen() DrawDibOpen32
 11 stub    DrawDibProfileDisplay
 12 stub    DrawDibRealize
 13 stub    DrawDibSetPalette
 14 stub    DrawDibStart
 15 stub    DrawDibStop
 16 stub    DrawDibTime
 17 stub    GetOpenFileNamePreview
 18 stub    GetOpenFileNamePreviewA
 19 stub    GetOpenFileNamePreviewW
 20 stub    GetSaveFileNamePreviewA
 21 stub    GetSaveFileNamePreviewW
 22 stub    ICClose
 23 stub    ICCompress
 24 stub    ICCompressorChoose
 25 stub    ICCompressorFree
 26 stub    ICDecompress
 27 stub    ICDraw
 28 stdcall ICDrawBegin(long long long long long long long long long ptr long long long long long long) ICDrawBegin32
 29 stub    ICGetDisplayFormat
 30 stdcall ICGetInfo(long ptr long) ICGetInfo32
 31 stub    ICImageCompress
 32 stub    ICImageDecompress
 33 stdcall ICInfo(long long ptr) ICInfo32
 34 stub    ICInstall
 35 stub    ICLocate
 36 stub    ICMThunk32
 37 stdcall ICOpen(long long long) ICOpen32
 38 stub    ICOpenFunction
 39 stub    ICRemove
 40 stdcall ICSendMessage(long long long long) ICSendMessage32
 41 stub    ICSeqCompressFrame
 42 stub    ICSeqCompressFrameEnd
 43 stub    ICSeqCompressFrameStart
 44 stdcall MCIWndCreate (long long long ptr) MCIWndCreate32
 45 stdcall MCIWndCreateA (long long long str) MCIWndCreate32A
 46 stdcall MCIWndCreateW (long long long wstr) MCIWndCreate32W
 47 stub    MCIWndRegisterClass
 48 stub    StretchDIB
 49 stub    ls_ThunkData32
 50 stub    sl_ThunkData32
