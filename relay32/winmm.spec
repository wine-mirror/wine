name winmm
type win32

  1 stdcall PlaySoundA(ptr long long) PlaySound32A
  2 stdcall WINMM_2(ptr long long) PlaySound32A
  3 stub WINMM_3
  4 stub CloseDriver
  5 stub DefDriverProc
  6 stub DriverCallback
  7 stub DrvClose
  8 stub DrvDefDriverProc
  9 stdcall DrvGetModuleHandle(long) GetDriverModuleHandle32
 10 stub DrvOpen
 11 stub DrvOpenA
 12 stub DrvSendMessage
 13 stub GetDriverFlags
 14 stdcall GetDriverModuleHandle(long) GetDriverModuleHandle32
 15 stdcall OpenDriver(wstr wstr long) OpenDriver32W
 16 stdcall OpenDriverA(str str long) OpenDriver32A
 17 stdcall PlaySound(ptr long long) PlaySound32A
 18 stdcall PlaySoundW(ptr long long) PlaySound32W
 19 stub SendDriverMessage
 20 stdcall auxGetDevCapsA(long ptr long) auxGetDevCaps32A
 21 stdcall auxGetDevCapsW(long ptr long) auxGetDevCaps32W
 22 stdcall auxGetNumDevs() auxGetNumDevs32
 23 stdcall auxGetVolume(long ptr) auxGetVolume32
 24 stdcall auxOutMessage(long long long long) auxOutMessage32
 25 stdcall auxSetVolume(long long) auxSetVolume32
 26 stub joyConfigChanged
 27 stdcall joyGetDevCapsA(long ptr long) joyGetDevCaps32A
 28 stdcall joyGetDevCapsW(long ptr long) joyGetDevCaps32W
 29 stdcall joyGetNumDevs() joyGetNumDevs32
 30 stdcall joyGetPos(long ptr) joyGetPos32
 31 stdcall joyGetPosEx(long ptr) joyGetPosEx
 32 stdcall joyGetThreshold(long ptr) joyGetThreshold32
 33 stub joyReleaseCapture
 34 stub joySetCapture
 35 stdcall joySetThreshold(long long) joySetThreshold32
 36 stub mciDriverNotify
 37 stub mciDriverYield
 38 stub mciExecute
 39 stub mciFreeCommandResource
 40 stub mciGetCreatorTask
 41 stub mciGetDeviceIDA
 42 stub mciGetDeviceIDFromElementIDW
 43 stub mciGetDeviceIDW
 44 stub mciGetDriverData
 45 stdcall mciGetErrorStringA(long ptr long) mciGetErrorString32A
 46 stdcall mciGetErrorStringW(long ptr long) mciGetErrorString32W
 47 stub mciGetYieldProc
 48 stub mciLoadCommandResource
 49 stdcall mciSendCommandA(long long long long) mciSendCommand32A
 50 stub mciSendCommandW
 51 stdcall mciSendStringA(str ptr long long) mciSendString
 52 stub mciSendStringW
 53 stub mciSetDriverData
 54 stub mciSetYieldProc
 55 stub midiConnect
 56 stub midiDisconnect
 57 stdcall midiInAddBuffer(long ptr long) midiInAddBuffer32
 58 stdcall midiInClose(long) midiInClose32
 59 stdcall midiInGetDevCapsA(long ptr long) midiInGetDevCaps32A
 60 stdcall midiInGetDevCapsW(long ptr long) midiInGetDevCaps32W
 61 stdcall midiInGetErrorTextA(long ptr long) midiInGetErrorText32A
 62 stdcall midiInGetErrorTextW(long ptr long) midiInGetErrorText32W
 63 stdcall midiInGetID(long) midiInGetID32
 64 stdcall midiInGetNumDevs() midiInGetNumDevs32
 65 stdcall midiInMessage(long long long long) midiInMessage32
 66 stdcall midiInOpen(ptr long long long long) midiInOpen32
 67 stdcall midiInPrepareHeader(long ptr long) midiInPrepareHeader32
 68 stdcall midiInReset(long) midiInReset32
 69 stdcall midiInStart(long) midiInStart32
 70 stdcall midiInStop(long) midiInStop32
 71 stdcall midiInUnprepareHeader(long ptr long) midiInUnprepareHeader32
 72 stdcall midiOutCacheDrumPatches(long long ptr long) midiOutCacheDrumPatches32
 73 stdcall midiOutCachePatches(long long ptr long) midiOutCachePatches32
 74 stdcall midiOutClose(long) midiOutClose32
 75 stdcall midiOutGetDevCapsA(long ptr long) midiOutGetDevCaps32A
 76 stdcall midiOutGetDevCapsW(long ptr long) midiOutGetDevCaps32W
 77 stdcall midiOutGetErrorTextA(long ptr long) midiOutGetErrorText32A
 78 stdcall midiOutGetErrorTextW(long ptr long) midiOutGetErrorText32W
 79 stdcall midiOutGetID(long) midiOutGetID32
 80 stdcall midiOutGetNumDevs() midiOutGetNumDevs32
 81 stdcall midiOutGetVolume(long ptr) midiOutGetVolume32
 82 stdcall midiOutLongMsg(long ptr long) midiOutLongMsg32
 83 stdcall midiOutMessage(long long long long) midiOutMessage32
 84 stdcall midiOutOpen(ptr long long long long) midiOutOpen32
 85 stdcall midiOutPrepareHeader(long ptr long) midiOutPrepareHeader32
 86 stdcall midiOutReset(long) midiOutReset32
 87 stdcall midiOutSetVolume(long ptr) midiOutSetVolume32
 88 stdcall midiOutShortMsg(long long) midiOutShortMsg32
 89 stdcall midiOutUnprepareHeader(long ptr long) midiOutUnprepareHeader32
 90 stub midiStreamClose
 91 stub midiStreamOpen
 92 stub midiStreamOut
 93 stub midiStreamPause
 94 stub midiStreamPosition
 95 stub midiStreamProperty
 96 stub midiStreamRestart
 97 stub midiStreamStop
 98 stdcall mixerClose(long) mixerClose32
 99 stdcall mixerGetControlDetailsA(long ptr long) mixerGetControlDetails32A
100 stdcall mixerGetControlDetailsW(long ptr long) mixerGetControlDetails32W
101 stdcall mixerGetDevCapsA(long ptr long) mixerGetDevCaps32A
102 stdcall mixerGetDevCapsW(long ptr long) mixerGetDevCaps32W
103 stdcall mixerGetID(long ptr long) mixerGetID32
104 stdcall mixerGetLineControlsA(long ptr long) mixerGetLineControls32A
105 stdcall mixerGetLineControlsW(long ptr long) mixerGetLineControls32W
106 stdcall mixerGetLineInfoA(long ptr long) mixerGetLineInfo32A
107 stdcall mixerGetLineInfoW(long ptr long) mixerGetLineInfo32W
108 stdcall mixerGetNumDevs() mixerGetNumDevs32
109 stdcall mixerMessage(long long long long) mixerMessage32
110 stdcall mixerOpen(ptr long long long long) mixerOpen32
111 stdcall mixerSetControlDetails(long ptr long) mixerSetControlDetails32
112 stdcall mmioAdvance(long ptr long) mmioAdvance32
113 stdcall mmioAscend(long ptr long) mmioAscend32
114 stdcall mmioClose(long long) mmioClose32
115 stub mmioCreateChunk
116 stdcall mmioDescend(long ptr ptr long) mmioDescend
117 stdcall mmioFlush(long long) mmioFlush32
118 stdcall mmioGetInfo(long ptr long) mmioGetInfo32
119 stub mmioInstallIOProc16
120 stdcall mmioInstallIOProcA(long ptr long) mmioInstallIOProc32A
121 stub mmioInstallIOProcW
122 stdcall mmioOpenA(str ptr long) mmioOpen32A
123 stdcall mmioOpenW(wstr ptr long) mmioOpen32W
124 stdcall mmioRead(long ptr long) mmioRead32
125 stub mmioRenameA
126 stub mmioRenameW
127 stub mmioSeek
128 stub mmioSendMessage
129 stub mmioSetBuffer
130 stub mmioSetInfo
131 stdcall mmioStringToFOURCCA(str long) mmioStringToFOURCC32A
132 stdcall mmioStringToFOURCCW(wstr long) mmioStringToFOURCC32W
133 stub mmioWrite
134 stdcall mmsystemGetVersion() mmsystemGetVersion32
135 stdcall sndPlaySoundA(ptr long) sndPlaySound
136 stub sndPlaySoundW
137 stdcall timeBeginPeriod(long) timeBeginPeriod32
138 stdcall timeEndPeriod(long) timeEndPeriod32
139 stdcall timeGetDevCaps(ptr long) timeGetDevCaps32
140 stdcall timeGetSystemTime(ptr long) timeGetSystemTime32
141 stdcall timeGetTime() timeGetTime
142 stdcall timeKillEvent(long) timeKillEvent32
143 stdcall timeSetEvent(long long ptr long long) timeSetEvent32
144 stdcall waveInAddBuffer(long ptr long) waveInAddBuffer32
145 stdcall waveInClose(long) waveInClose32
146 stdcall waveInGetDevCapsA(long ptr long) waveInGetDevCaps32A
147 stdcall waveInGetDevCapsW(long ptr long) waveInGetDevCaps32W
148 stdcall waveInGetErrorTextA(long ptr long) waveInGetErrorText32A
149 stdcall waveInGetErrorTextW(long ptr long) waveInGetErrorText32W
150 stdcall waveInGetID(long ptr) waveInGetID32
151 stdcall waveInGetNumDevs() waveInGetNumDevs32
152 stdcall waveInGetPosition(long ptr long) waveInGetPosition32
153 stdcall waveInMessage(long long long long) waveInMessage32
154 stdcall waveInOpen(ptr long ptr long long long) waveInOpen32
155 stdcall waveInPrepareHeader(long ptr long) waveInPrepareHeader32
156 stdcall waveInReset(long) waveInReset32
157 stdcall waveInStart(long) waveInStart32
158 stdcall waveInStop(long) waveInStop32
159 stdcall waveInUnprepareHeader(long ptr long) waveInUnprepareHeader32
160 stdcall waveOutBreakLoop(long) waveOutBreakLoop32
161 stdcall waveOutClose(long) waveOutClose32
162 stdcall waveOutGetDevCapsA(long ptr long) waveOutGetDevCaps32A
163 stdcall waveOutGetDevCapsW(long ptr long) waveOutGetDevCaps32W
164 stdcall waveOutGetErrorTextA(long ptr long) waveOutGetErrorText32A
165 stdcall waveOutGetErrorTextW(long ptr long) waveOutGetErrorText32W
166 stdcall waveOutGetID(long ptr) waveOutGetID32
167 stdcall waveOutGetNumDevs() waveOutGetNumDevs32
168 stdcall waveOutGetPitch(long ptr) waveOutGetPitch32
169 stdcall waveOutGetPlaybackRate(long ptr) waveOutGetPlaybackRate32
170 stdcall waveOutGetPosition(long ptr long) waveOutGetPosition32
171 stdcall waveOutGetVolume(long ptr) waveOutGetVolume32
172 stdcall waveOutMessage(long long long long) waveOutMessage32
173 stdcall waveOutOpen(ptr long ptr long long long) waveOutOpen32
174 stdcall waveOutPause(long) waveOutPause32
175 stdcall waveOutPrepareHeader(long ptr long) waveOutPrepareHeader32
176 stdcall waveOutReset(long) waveOutReset32
177 stdcall waveOutRestart(long) waveOutRestart32
178 stdcall waveOutSetPitch(long long) waveOutSetPitch32
179 stdcall waveOutSetPlaybackRate(long long) waveOutSetPlaybackRate32
180 stdcall waveOutSetVolume(long long) waveOutSetVolume32
181 stdcall waveOutUnprepareHeader(long ptr long) waveOutPrepareHeader32
182 stdcall waveOutWrite(long ptr long) waveOutWrite32
183 stub winmmf_ThunkData32
184 stub winmmsl_ThunkData32
