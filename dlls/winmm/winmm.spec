name winmm
type win32
init WINMM_LibMain
rsrc winmm_res

import user32.dll

# ordinal exports
1 stdcall @(ptr long long) PlaySoundA
3 stub @
4 stub @

@ stdcall PlaySoundA(ptr long long) PlaySoundA
@ stdcall CloseDriver(long long long) CloseDriver
@ stdcall DefDriverProc(long long long long long) DefDriverProc
@ stdcall DriverCallback(long long long long long long long) DriverCallback
@ stdcall DrvClose(long long long) CloseDriver
@ stdcall DrvDefDriverProc(long long long long long) DefDriverProc
@ stdcall DrvGetModuleHandle(long) GetDriverModuleHandle
@ stdcall DrvOpen(wstr wstr long) OpenDriverW
@ stdcall DrvOpenA(str str long) OpenDriverA
@ stdcall DrvSendMessage(long long long long) SendDriverMessage
@ stdcall GetDriverFlags(long) GetDriverFlags
@ stdcall GetDriverModuleHandle(long) GetDriverModuleHandle
@ stdcall OpenDriver(wstr wstr long) OpenDriverW
@ stdcall OpenDriverA(str str long) OpenDriverA
@ stdcall PlaySound(ptr long long) PlaySoundA
@ stdcall PlaySoundW(ptr long long) PlaySoundW
@ stdcall SendDriverMessage(long long long long) SendDriverMessage
@ stdcall auxGetDevCapsA(long ptr long) auxGetDevCapsA
@ stdcall auxGetDevCapsW(long ptr long) auxGetDevCapsW
@ stdcall auxGetNumDevs() auxGetNumDevs
@ stdcall auxGetVolume(long ptr) auxGetVolume
@ stdcall auxOutMessage(long long long long) auxOutMessage
@ stdcall auxSetVolume(long long) auxSetVolume
@ stub joyConfigChanged
@ stdcall joyGetDevCapsA(long ptr long) joyGetDevCapsA
@ stdcall joyGetDevCapsW(long ptr long) joyGetDevCapsW
@ stdcall joyGetNumDevs() joyGetNumDevs
@ stdcall joyGetPos(long ptr) joyGetPos
@ stdcall joyGetPosEx(long ptr) joyGetPosEx
@ stdcall joyGetThreshold(long ptr) joyGetThreshold
@ stdcall joyReleaseCapture(long) joyReleaseCapture
@ stdcall joySetCapture(long long long long) joySetCapture
@ stdcall joySetThreshold(long long) joySetThreshold
@ stdcall mciDriverNotify(long long long) mciDriverNotify
@ stdcall mciDriverYield(long) mciDriverYield
@ stdcall mciExecute(str) mciExecute
@ stdcall mciFreeCommandResource(long) mciFreeCommandResource
@ stdcall mciGetCreatorTask(long) mciGetCreatorTask
@ stdcall mciGetDeviceIDA(str) mciGetDeviceIDA
@ stdcall mciGetDeviceIDFromElementIDW(long str) mciGetDeviceIDFromElementIDW
@ stdcall mciGetDeviceIDW(str) mciGetDeviceIDW
@ stdcall mciGetDriverData(long) mciGetDriverData
@ stdcall mciGetErrorStringA(long ptr long) mciGetErrorStringA
@ stdcall mciGetErrorStringW(long ptr long) mciGetErrorStringW
@ stdcall mciGetYieldProc(long ptr) mciGetYieldProc
@ stdcall mciLoadCommandResource(long wstr long) mciLoadCommandResource 
@ stdcall mciSendCommandA(long long long long) mciSendCommandA
@ stdcall mciSendCommandW(long long long long) mciSendCommandW
@ stdcall mciSendStringA(str ptr long long) mciSendStringA
@ stdcall mciSendStringW(str ptr long long) mciSendStringW
@ stdcall mciSetDriverData(long long) mciSetDriverData
@ stdcall mciSetYieldProc(long ptr long) mciSetYieldProc
@ stub midiConnect
@ stub midiDisconnect
@ stdcall midiInAddBuffer(long ptr long) midiInAddBuffer
@ stdcall midiInClose(long) midiInClose
@ stdcall midiInGetDevCapsA(long ptr long) midiInGetDevCapsA
@ stdcall midiInGetDevCapsW(long ptr long) midiInGetDevCapsW
@ stdcall midiInGetErrorTextA(long ptr long) midiInGetErrorTextA
@ stdcall midiInGetErrorTextW(long ptr long) midiInGetErrorTextW
@ stdcall midiInGetID(long ptr) midiInGetID
@ stdcall midiInGetNumDevs() midiInGetNumDevs
@ stdcall midiInMessage(long long long long) midiInMessage
@ stdcall midiInOpen(ptr long long long long) midiInOpen
@ stdcall midiInPrepareHeader(long ptr long) midiInPrepareHeader
@ stdcall midiInReset(long) midiInReset
@ stdcall midiInStart(long) midiInStart
@ stdcall midiInStop(long) midiInStop
@ stdcall midiInUnprepareHeader(long ptr long) midiInUnprepareHeader
@ stdcall midiOutCacheDrumPatches(long long ptr long) midiOutCacheDrumPatches
@ stdcall midiOutCachePatches(long long ptr long) midiOutCachePatches
@ stdcall midiOutClose(long) midiOutClose
@ stdcall midiOutGetDevCapsA(long ptr long) midiOutGetDevCapsA
@ stdcall midiOutGetDevCapsW(long ptr long) midiOutGetDevCapsW
@ stdcall midiOutGetErrorTextA(long ptr long) midiOutGetErrorTextA
@ stdcall midiOutGetErrorTextW(long ptr long) midiOutGetErrorTextW
@ stdcall midiOutGetID(long ptr) midiOutGetID
@ stdcall midiOutGetNumDevs() midiOutGetNumDevs
@ stdcall midiOutGetVolume(long ptr) midiOutGetVolume
@ stdcall midiOutLongMsg(long ptr long) midiOutLongMsg
@ stdcall midiOutMessage(long long long long) midiOutMessage
@ stdcall midiOutOpen(ptr long long long long) midiOutOpen
@ stdcall midiOutPrepareHeader(long ptr long) midiOutPrepareHeader
@ stdcall midiOutReset(long) midiOutReset
@ stdcall midiOutSetVolume(long ptr) midiOutSetVolume
@ stdcall midiOutShortMsg(long long) midiOutShortMsg
@ stdcall midiOutUnprepareHeader(long ptr long) midiOutUnprepareHeader
@ stdcall midiStreamClose(long) midiStreamClose
@ stdcall midiStreamOpen(ptr ptr long long long long) midiStreamOpen
@ stdcall midiStreamOut(long ptr long) midiStreamOut
@ stdcall midiStreamPause(long) midiStreamPause
@ stdcall midiStreamPosition(long ptr long) midiStreamPosition
@ stdcall midiStreamProperty(long ptr long) midiStreamProperty
@ stdcall midiStreamRestart(long) midiStreamRestart
@ stdcall midiStreamStop(long) midiStreamStop
@ stdcall mixerClose(long) mixerClose
@ stdcall mixerGetControlDetailsA(long ptr long) mixerGetControlDetailsA
@ stdcall mixerGetControlDetailsW(long ptr long) mixerGetControlDetailsW
@ stdcall mixerGetDevCapsA(long ptr long) mixerGetDevCapsA
@ stdcall mixerGetDevCapsW(long ptr long) mixerGetDevCapsW
@ stdcall mixerGetID(long ptr long) mixerGetID
@ stdcall mixerGetLineControlsA(long ptr long) mixerGetLineControlsA
@ stdcall mixerGetLineControlsW(long ptr long) mixerGetLineControlsW
@ stdcall mixerGetLineInfoA(long ptr long) mixerGetLineInfoA
@ stdcall mixerGetLineInfoW(long ptr long) mixerGetLineInfoW
@ stdcall mixerGetNumDevs() mixerGetNumDevs
@ stdcall mixerMessage(long long long long) mixerMessage
@ stdcall mixerOpen(ptr long long long long) mixerOpen
@ stdcall mixerSetControlDetails(long ptr long) mixerSetControlDetails
@ stdcall mmioAdvance(long ptr long) mmioAdvance
@ stdcall mmioAscend(long ptr long) mmioAscend
@ stdcall mmioClose(long long) mmioClose
@ stdcall mmioCreateChunk(long ptr long) mmioCreateChunk
@ stdcall mmioDescend(long ptr ptr long) mmioDescend
@ stdcall mmioFlush(long long) mmioFlush
@ stdcall mmioGetInfo(long ptr long) mmioGetInfo
@ stub mmioInstallIOProc16
@ stdcall mmioInstallIOProcA(long ptr long) mmioInstallIOProcA
@ stdcall mmioInstallIOProcW(long ptr long) mmioInstallIOProcW
@ stdcall mmioOpenA(str ptr long) mmioOpenA
@ stdcall mmioOpenW(wstr ptr long) mmioOpenW
@ stdcall mmioRead(long ptr long) mmioRead
@ stdcall mmioRenameA(ptr ptr ptr long) mmioRenameA
@ stdcall mmioRenameW(ptr ptr ptr long) mmioRenameW
@ stdcall mmioSeek(long long long) mmioSeek
@ stdcall mmioSendMessage(long long long long) mmioSendMessage
@ stdcall mmioSetBuffer(long ptr long long) mmioSetBuffer
@ stdcall mmioSetInfo(long ptr long) mmioSetInfo
@ stdcall mmioStringToFOURCCA(str long) mmioStringToFOURCCA
@ stdcall mmioStringToFOURCCW(wstr long) mmioStringToFOURCCW
@ stdcall mmioWrite(long ptr long) mmioWrite
@ stdcall mmsystemGetVersion() mmsystemGetVersion
@ stdcall sndPlaySoundA(ptr long) sndPlaySoundA
@ stdcall sndPlaySoundW(ptr long) sndPlaySoundW
@ stdcall timeBeginPeriod(long) timeBeginPeriod
@ stdcall timeEndPeriod(long) timeEndPeriod
@ stdcall timeGetDevCaps(ptr long) timeGetDevCaps
@ stdcall timeGetSystemTime(ptr long) timeGetSystemTime
@ stdcall timeGetTime() timeGetTime
@ stdcall timeKillEvent(long) timeKillEvent
@ stdcall timeSetEvent(long long ptr long long) timeSetEvent
@ stdcall waveInAddBuffer(long ptr long) waveInAddBuffer
@ stdcall waveInClose(long) waveInClose
@ stdcall waveInGetDevCapsA(long ptr long) waveInGetDevCapsA
@ stdcall waveInGetDevCapsW(long ptr long) waveInGetDevCapsW
@ stdcall waveInGetErrorTextA(long ptr long) waveInGetErrorTextA
@ stdcall waveInGetErrorTextW(long ptr long) waveInGetErrorTextW
@ stdcall waveInGetID(long ptr) waveInGetID
@ stdcall waveInGetNumDevs() waveInGetNumDevs
@ stdcall waveInGetPosition(long ptr long) waveInGetPosition
@ stdcall waveInMessage(long long long long) waveInMessage
@ stdcall waveInOpen(ptr long ptr long long long) waveInOpen
@ stdcall waveInPrepareHeader(long ptr long) waveInPrepareHeader
@ stdcall waveInReset(long) waveInReset
@ stdcall waveInStart(long) waveInStart
@ stdcall waveInStop(long) waveInStop
@ stdcall waveInUnprepareHeader(long ptr long) waveInUnprepareHeader
@ stdcall waveOutBreakLoop(long) waveOutBreakLoop
@ stdcall waveOutClose(long) waveOutClose
@ stdcall waveOutGetDevCapsA(long ptr long) waveOutGetDevCapsA
@ stdcall waveOutGetDevCapsW(long ptr long) waveOutGetDevCapsW
@ stdcall waveOutGetErrorTextA(long ptr long) waveOutGetErrorTextA
@ stdcall waveOutGetErrorTextW(long ptr long) waveOutGetErrorTextW
@ stdcall waveOutGetID(long ptr) waveOutGetID
@ stdcall waveOutGetNumDevs() waveOutGetNumDevs
@ stdcall waveOutGetPitch(long ptr) waveOutGetPitch
@ stdcall waveOutGetPlaybackRate(long ptr) waveOutGetPlaybackRate
@ stdcall waveOutGetPosition(long ptr long) waveOutGetPosition
@ stdcall waveOutGetVolume(long ptr) waveOutGetVolume
@ stdcall waveOutMessage(long long long long) waveOutMessage
@ stdcall waveOutOpen(ptr long ptr long long long) waveOutOpen
@ stdcall waveOutPause(long) waveOutPause
@ stdcall waveOutPrepareHeader(long ptr long) waveOutPrepareHeader
@ stdcall waveOutReset(long) waveOutReset
@ stdcall waveOutRestart(long) waveOutRestart
@ stdcall waveOutSetPitch(long long) waveOutSetPitch
@ stdcall waveOutSetPlaybackRate(long long) waveOutSetPlaybackRate
@ stdcall waveOutSetVolume(long long) waveOutSetVolume
@ stdcall waveOutUnprepareHeader(long ptr long) waveOutUnprepareHeader
@ stdcall waveOutWrite(long ptr long) waveOutWrite
@ stub winmmf_ThunkData32
@ stub winmmsl_ThunkData32
