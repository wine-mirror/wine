name	msvfw32
type	win32

import winmm.dll
import user32.dll
import gdi32.dll
import kernel32.dll

# Yes, ICCompress,ICDecompress,MCIWnd* and ICDraw* are cdecl (VFWAPIV).
# The rest is stdcall (VFWAPI) however. -Marcus Meissner, 990124

  2 stdcall VideoForWindowsVersion() VideoForWindowsVersion
  3 stdcall DrawDibBegin(long long long long ptr long long long) DrawDibBegin
  4 stub    DrawDibChangePalette
  5 stdcall DrawDibClose(long) DrawDibClose
  6 stdcall DrawDibDraw(long long long long long long ptr ptr long long long long long) DrawDibDraw
  7 stdcall DrawDibEnd(long) DrawDibEnd
  8 stub    DrawDibGetBuffer
  9 stdcall DrawDibGetPalette(long) DrawDibGetPalette
 10 stdcall DrawDibOpen() DrawDibOpen
 11 stub    DrawDibProfileDisplay
 12 stdcall DrawDibRealize(long long long) DrawDibRealize
 13 stdcall DrawDibSetPalette(long long) DrawDibSetPalette
 14 stdcall DrawDibStart(long long) DrawDibStart
 15 stdcall DrawDibStop(long) DrawDibStop
 16 stub    DrawDibTime
 17 stub    GetOpenFileNamePreview
 18 stub    GetOpenFileNamePreviewA
 19 stub    GetOpenFileNamePreviewW
 20 stub    GetSaveFileNamePreviewA
 21 stub    GetSaveFileNamePreviewW
 22 stdcall ICClose(long) ICClose
 23 cdecl   ICCompress(long long ptr ptr ptr ptr ptr ptr long long long ptr ptr) ICCompress
 24 stub    ICCompressorChoose
 25 stub    ICCompressorFree
 26 cdecl   ICDecompress(long long ptr ptr ptr ptr) ICDecompress
 27 cdecl   ICDraw(long long ptr ptr long long) ICDraw
 28 cdecl   ICDrawBegin(long long long long long long long long long ptr long long long long long long) ICDrawBegin
 29 stdcall ICGetDisplayFormat(long ptr ptr long long long) ICGetDisplayFormat
 30 stdcall ICGetInfo(long ptr long) ICGetInfo
 31 stub    ICImageCompress
 32 stub    ICImageDecompress
 33 stdcall ICInfo(long long ptr) ICInfo
 34 stub    ICInstall
 35 stdcall ICLocate(long long ptr ptr long) ICLocate
 36 stub    ICMThunk
 37 stdcall ICOpen(long long long) ICOpen
 38 stdcall ICOpenFunction(long long long ptr) ICOpenFunction
 39 stub    ICRemove
 40 stdcall ICSendMessage(long long long long) ICSendMessage
 41 stub    ICSeqCompressFrame
 42 stub    ICSeqCompressFrameEnd
 43 stub    ICSeqCompressFrameStart
 44 cdecl   MCIWndCreate (long long long str) MCIWndCreateA
 45 cdecl   MCIWndCreateA (long long long str) MCIWndCreateA
 46 cdecl   MCIWndCreateW (long long long wstr) MCIWndCreateW
 47 stub    MCIWndRegisterClass
 48 stub    StretchDIB
 49 stub    ls_ThunkData32
 50 stub    sl_ThunkData32
