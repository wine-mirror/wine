name winmm
type win32

  1 stdcall PlaySoundA(ptr long long) PlaySoundA
  2 stdcall WINMM_2(ptr long long) PlaySoundA
  3 stub WINMM_3
  4 stdcall CloseDriver(long long long) CloseDriver
  5 stdcall DefDriverProc(long long long long long) DefDriverProc
  6 stub DriverCallback
  7 stub DrvClose
  8 stub DrvDefDriverProc
  9 stdcall DrvGetModuleHandle(long) GetDriverModuleHandle
 10 stub DrvOpen
 11 stub DrvOpenA
 12 stub DrvSendMessage
 13 stub GetDriverFlags
 14 stdcall GetDriverModuleHandle(long) GetDriverModuleHandle
 15 stdcall OpenDriver(wstr wstr long) OpenDriverW
 16 stdcall OpenDriverA(str str long) OpenDriverA
 17 stdcall PlaySound(ptr long long) PlaySoundA
 18 stdcall PlaySoundW(ptr long long) PlaySoundW
 19 stdcall SendDriverMessage(long long long long) SendDriverMessage
 20 stdcall auxGetDevCapsA(long ptr long) auxGetDevCapsA
 21 stdcall auxGetDevCapsW(long ptr long) auxGetDevCapsW
 22 stdcall auxGetNumDevs() auxGetNumDevs
 23 stdcall auxGetVolume(long ptr) auxGetVolume
 24 stdcall auxOutMessage(long long long long) auxOutMessage
 25 stdcall auxSetVolume(long long) auxSetVolume
 26 stub joyConfigChanged
 27 stdcall joyGetDevCapsA(long ptr long) joyGetDevCapsA
 28 stdcall joyGetDevCapsW(long ptr long) joyGetDevCapsW
 29 stdcall joyGetNumDevs() joyGetNumDevs
 30 stdcall joyGetPos(long ptr) joyGetPos
 31 stdcall joyGetPosEx(long ptr) joyGetPosEx
 32 stdcall joyGetThreshold(long ptr) joyGetThreshold
 33 stdcall joyReleaseCapture(long) joyReleaseCapture
 34 stdcall joySetCapture(long long long long) joySetCapture
 35 stdcall joySetThreshold(long long) joySetThreshold
 36 stdcall mciDriverNotify(long long long) mciDriverNotify
 37 stdcall mciDriverYield(long) mciDriverYield
 38 stdcall mciExecute(str) mciExecute
 39 stdcall mciFreeCommandResource(long) mciFreeCommandResource
 40 stdcall mciGetCreatorTask(long) mciGetCreatorTask
 41 stdcall mciGetDeviceIDA(str) mciGetDeviceIDA
 42 stdcall mciGetDeviceIDFromElementIDW(long str) mciGetDeviceIDFromElementIDW
 43 stdcall mciGetDeviceIDW(str) mciGetDeviceIDW
 44 stdcall mciGetDriverData(long) mciGetDriverData
 45 stdcall mciGetErrorStringA(long ptr long) mciGetErrorStringA
 46 stdcall mciGetErrorStringW(long ptr long) mciGetErrorStringW
 47 stdcall mciGetYieldProc(long ptr) mciGetYieldProc
 48 stdcall mciLoadCommandResource(long wstr long) mciLoadCommandResource 
 49 stdcall mciSendCommandA(long long long long) mciSendCommandA
 50 stdcall mciSendCommandW(long long long long) mciSendCommandW
 51 stdcall mciSendStringA(str ptr long long) mciSendStringA
 52 stdcall mciSendStringW(str ptr long long) mciSendStringW
 53 stdcall mciSetDriverData(long long) mciSetDriverData
 54 stdcall mciSetYieldProc(ptr) mciSetYieldProc
 55 stub midiConnect
 56 stub midiDisconnect
 57 stdcall midiInAddBuffer(long ptr long) midiInAddBuffer
 58 stdcall midiInClose(long) midiInClose
 59 stdcall midiInGetDevCapsA(long ptr long) midiInGetDevCapsA
 60 stdcall midiInGetDevCapsW(long ptr long) midiInGetDevCapsW
 61 stdcall midiInGetErrorTextA(long ptr long) midiInGetErrorTextA
 62 stdcall midiInGetErrorTextW(long ptr long) midiInGetErrorTextW
 63 stdcall midiInGetID(long) midiInGetID
 64 stdcall midiInGetNumDevs() midiInGetNumDevs
 65 stdcall midiInMessage(long long long long) midiInMessage
 66 stdcall midiInOpen(ptr long long long long) midiInOpen
 67 stdcall midiInPrepareHeader(long ptr long) midiInPrepareHeader
 68 stdcall midiInReset(long) midiInReset
 69 stdcall midiInStart(long) midiInStart
 70 stdcall midiInStop(long) midiInStop
 71 stdcall midiInUnprepareHeader(long ptr long) midiInUnprepareHeader
 72 stdcall midiOutCacheDrumPatches(long long ptr long) midiOutCacheDrumPatches
 73 stdcall midiOutCachePatches(long long ptr long) midiOutCachePatches
 74 stdcall midiOutClose(long) midiOutClose
 75 stdcall midiOutGetDevCapsA(long ptr long) midiOutGetDevCapsA
 76 stdcall midiOutGetDevCapsW(long ptr long) midiOutGetDevCapsW
 77 stdcall midiOutGetErrorTextA(long ptr long) midiOutGetErrorTextA
 78 stdcall midiOutGetErrorTextW(long ptr long) midiOutGetErrorTextW
 79 stdcall midiOutGetID(long) midiOutGetID
 80 stdcall midiOutGetNumDevs() midiOutGetNumDevs
 81 stdcall midiOutGetVolume(long ptr) midiOutGetVolume
 82 stdcall midiOutLongMsg(long ptr long) midiOutLongMsg
 83 stdcall midiOutMessage(long long long long) midiOutMessage
 84 stdcall midiOutOpen(ptr long long long long) midiOutOpen
 85 stdcall midiOutPrepareHeader(long ptr long) midiOutPrepareHeader
 86 stdcall midiOutReset(long) midiOutReset
 87 stdcall midiOutSetVolume(long ptr) midiOutSetVolume
 88 stdcall midiOutShortMsg(long long) midiOutShortMsg
 89 stdcall midiOutUnprepareHeader(long ptr long) midiOutUnprepareHeader
 90 stdcall midiStreamClose(long) midiStreamClose
 91 stdcall midiStreamOpen(ptr ptr long long long long) midiStreamOpen
 92 stdcall midiStreamOut(long ptr long) midiStreamOut
 93 stdcall midiStreamPause(long) midiStreamPause
 94 stdcall midiStreamPosition(long ptr long) midiStreamPosition
 95 stdcall midiStreamProperty(long ptr long) midiStreamProperty
 96 stdcall midiStreamRestart(long) midiStreamRestart
 97 stdcall midiStreamStop(long) midiStreamStop
 98 stdcall mixerClose(long) mixerClose
 99 stdcall mixerGetControlDetailsA(long ptr long) mixerGetControlDetailsA
100 stdcall mixerGetControlDetailsW(long ptr long) mixerGetControlDetailsW
101 stdcall mixerGetDevCapsA(long ptr long) mixerGetDevCapsA
102 stdcall mixerGetDevCapsW(long ptr long) mixerGetDevCapsW
103 stdcall mixerGetID(long ptr long) mixerGetID
104 stdcall mixerGetLineControlsA(long ptr long) mixerGetLineControlsA
105 stdcall mixerGetLineControlsW(long ptr long) mixerGetLineControlsW
106 stdcall mixerGetLineInfoA(long ptr long) mixerGetLineInfoA
107 stdcall mixerGetLineInfoW(long ptr long) mixerGetLineInfoW
108 stdcall mixerGetNumDevs() mixerGetNumDevs
109 stdcall mixerMessage(long long long long) mixerMessage
110 stdcall mixerOpen(ptr long long long long) mixerOpen
111 stdcall mixerSetControlDetails(long ptr long) mixerSetControlDetails
112 stdcall mmioAdvance(long ptr long) mmioAdvance
113 stdcall mmioAscend(long ptr long) mmioAscend
114 stdcall mmioClose(long long) mmioClose
115 stdcall mmioCreateChunk(long ptr long) mmioCreateChunk
116 stdcall mmioDescend(long ptr ptr long) mmioDescend
117 stdcall mmioFlush(long long) mmioFlush
118 stdcall mmioGetInfo(long ptr long) mmioGetInfo
119 stub mmioInstallIOProc16
120 stdcall mmioInstallIOProcA(long ptr long) mmioInstallIOProcA
121 stub mmioInstallIOProcW
122 stdcall mmioOpenA(str ptr long) mmioOpenA
123 stdcall mmioOpenW(wstr ptr long) mmioOpenW
124 stdcall mmioRead(long ptr long) mmioRead
125 stdcall mmioRenameA(ptr ptr ptr long) mmioRenameA
126 stdcall mmioRenameW(ptr ptr ptr long) mmioRenameW
127 stdcall mmioSeek(long long long) mmioSeek
128 stdcall mmioSendMessage(long long long long) mmioSendMessage
129 stdcall mmioSetBuffer(long ptr long long) mmioSetBuffer
130 stdcall mmioSetInfo(long ptr long) mmioSetInfo
131 stdcall mmioStringToFOURCCA(str long) mmioStringToFOURCCA
132 stdcall mmioStringToFOURCCW(wstr long) mmioStringToFOURCCW
133 stdcall mmioWrite(long ptr long) mmioWrite
134 stdcall mmsystemGetVersion() mmsystemGetVersion
135 stdcall sndPlaySoundA(ptr long) sndPlaySoundA
136 stdcall sndPlaySoundW(ptr long) sndPlaySoundW
137 stdcall timeBeginPeriod(long) timeBeginPeriod
138 stdcall timeEndPeriod(long) timeEndPeriod
139 stdcall timeGetDevCaps(ptr long) timeGetDevCaps
140 stdcall timeGetSystemTime(ptr long) timeGetSystemTime
141 stdcall timeGetTime() timeGetTime
142 stdcall timeKillEvent(long) timeKillEvent
143 stdcall timeSetEvent(long long ptr long long) timeSetEvent
144 stdcall waveInAddBuffer(long ptr long) waveInAddBuffer
145 stdcall waveInClose(long) waveInClose
146 stdcall waveInGetDevCapsA(long ptr long) waveInGetDevCapsA
147 stdcall waveInGetDevCapsW(long ptr long) waveInGetDevCapsW
148 stdcall waveInGetErrorTextA(long ptr long) waveInGetErrorTextA
149 stdcall waveInGetErrorTextW(long ptr long) waveInGetErrorTextW
150 stdcall waveInGetID(long ptr) waveInGetID
151 stdcall waveInGetNumDevs() waveInGetNumDevs
152 stdcall waveInGetPosition(long ptr long) waveInGetPosition
153 stdcall waveInMessage(long long long long) waveInMessage
154 stdcall waveInOpen(ptr long ptr long long long) waveInOpen
155 stdcall waveInPrepareHeader(long ptr long) waveInPrepareHeader
156 stdcall waveInReset(long) waveInReset
157 stdcall waveInStart(long) waveInStart
158 stdcall waveInStop(long) waveInStop
159 stdcall waveInUnprepareHeader(long ptr long) waveInUnprepareHeader
160 stdcall waveOutBreakLoop(long) waveOutBreakLoop
161 stdcall waveOutClose(long) waveOutClose
162 stdcall waveOutGetDevCapsA(long ptr long) waveOutGetDevCapsA
163 stdcall waveOutGetDevCapsW(long ptr long) waveOutGetDevCapsW
164 stdcall waveOutGetErrorTextA(long ptr long) waveOutGetErrorTextA
165 stdcall waveOutGetErrorTextW(long ptr long) waveOutGetErrorTextW
166 stdcall waveOutGetID(long ptr) waveOutGetID
167 stdcall waveOutGetNumDevs() waveOutGetNumDevs
168 stdcall waveOutGetPitch(long ptr) waveOutGetPitch
169 stdcall waveOutGetPlaybackRate(long ptr) waveOutGetPlaybackRate
170 stdcall waveOutGetPosition(long ptr long) waveOutGetPosition
171 stdcall waveOutGetVolume(long ptr) waveOutGetVolume
172 stdcall waveOutMessage(long long long long) waveOutMessage
173 stdcall waveOutOpen(ptr long ptr long long long) waveOutOpen
174 stdcall waveOutPause(long) waveOutPause
175 stdcall waveOutPrepareHeader(long ptr long) waveOutPrepareHeader
176 stdcall waveOutReset(long) waveOutReset
177 stdcall waveOutRestart(long) waveOutRestart
178 stdcall waveOutSetPitch(long long) waveOutSetPitch
179 stdcall waveOutSetPlaybackRate(long long) waveOutSetPlaybackRate
180 stdcall waveOutSetVolume(long long) waveOutSetVolume
181 stdcall waveOutUnprepareHeader(long ptr long) waveOutUnprepareHeader
182 stdcall waveOutWrite(long ptr long) waveOutWrite
183 stub winmmf_ThunkData32
184 stub winmmsl_ThunkData32
