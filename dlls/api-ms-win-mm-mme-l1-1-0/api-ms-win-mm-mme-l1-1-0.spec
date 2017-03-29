@ stdcall auxGetDevCapsA(long ptr long) winmm.auxGetDevCapsA
@ stdcall auxGetDevCapsW(long ptr long) winmm.auxGetDevCapsW
@ stdcall auxGetNumDevs() winmm.auxGetNumDevs
@ stdcall auxGetVolume(long ptr) winmm.auxGetVolume
@ stdcall auxOutMessage(long long long long) winmm.auxOutMessage
@ stdcall auxSetVolume(long long) winmm.auxSetVolume
@ stdcall midiConnect(long long ptr) winmm.midiConnect
@ stdcall midiDisconnect(long long ptr) winmm.midiDisconnect
@ stdcall midiInAddBuffer(long ptr long) winmm.midiInAddBuffer
@ stdcall midiInClose(long) winmm.midiInClose
@ stdcall midiInGetDevCapsA(long ptr long) winmm.midiInGetDevCapsA
@ stdcall midiInGetDevCapsW(long ptr long) winmm.midiInGetDevCapsW
@ stdcall midiInGetErrorTextA(long ptr long) winmm.midiInGetErrorTextA
@ stdcall midiInGetErrorTextW(long ptr long) winmm.midiInGetErrorTextW
@ stdcall midiInGetID(long ptr) winmm.midiInGetID
@ stdcall midiInGetNumDevs() winmm.midiInGetNumDevs
@ stdcall midiInMessage(long long long long) winmm.midiInMessage
@ stdcall midiInOpen(ptr long long long long) winmm.midiInOpen
@ stdcall midiInPrepareHeader(long ptr long) winmm.midiInPrepareHeader
@ stdcall midiInReset(long) winmm.midiInReset
@ stdcall midiInStart(long) winmm.midiInStart
@ stdcall midiInStop(long) winmm.midiInStop
@ stdcall midiInUnprepareHeader(long ptr long) winmm.midiInUnprepareHeader
@ stdcall midiOutCacheDrumPatches(long long ptr long) winmm.midiOutCacheDrumPatches
@ stdcall midiOutCachePatches(long long ptr long) winmm.midiOutCachePatches
@ stdcall midiOutClose(long) winmm.midiOutClose
@ stdcall midiOutGetDevCapsA(long ptr long) winmm.midiOutGetDevCapsA
@ stdcall midiOutGetDevCapsW(long ptr long) winmm.midiOutGetDevCapsW
@ stdcall midiOutGetErrorTextA(long ptr long) winmm.midiOutGetErrorTextA
@ stdcall midiOutGetErrorTextW(long ptr long) winmm.midiOutGetErrorTextW
@ stdcall midiOutGetID(long ptr) winmm.midiOutGetID
@ stdcall midiOutGetNumDevs() winmm.midiOutGetNumDevs
@ stdcall midiOutGetVolume(long ptr) winmm.midiOutGetVolume
@ stdcall midiOutLongMsg(long ptr long) winmm.midiOutLongMsg
@ stdcall midiOutMessage(long long long long) winmm.midiOutMessage
@ stdcall midiOutOpen(ptr long long long long) winmm.midiOutOpen
@ stdcall midiOutPrepareHeader(long ptr long) winmm.midiOutPrepareHeader
@ stdcall midiOutReset(long) winmm.midiOutReset
@ stdcall midiOutSetVolume(long long) winmm.midiOutSetVolume
@ stdcall midiOutShortMsg(long long) winmm.midiOutShortMsg
@ stdcall midiOutUnprepareHeader(long ptr long) winmm.midiOutUnprepareHeader
@ stdcall midiStreamClose(long) winmm.midiStreamClose
@ stdcall midiStreamOpen(ptr ptr long long long long) winmm.midiStreamOpen
@ stdcall midiStreamOut(long ptr long) winmm.midiStreamOut
@ stdcall midiStreamPause(long) winmm.midiStreamPause
@ stdcall midiStreamPosition(long ptr long) winmm.midiStreamPosition
@ stdcall midiStreamProperty(long ptr long) winmm.midiStreamProperty
@ stdcall midiStreamRestart(long) winmm.midiStreamRestart
@ stdcall midiStreamStop(long) winmm.midiStreamStop
@ stdcall mixerClose(long) winmm.mixerClose
@ stdcall mixerGetControlDetailsA(long ptr long) winmm.mixerGetControlDetailsA
@ stdcall mixerGetControlDetailsW(long ptr long) winmm.mixerGetControlDetailsW
@ stdcall mixerGetDevCapsA(long ptr long) winmm.mixerGetDevCapsA
@ stdcall mixerGetDevCapsW(long ptr long) winmm.mixerGetDevCapsW
@ stdcall mixerGetID(long ptr long) winmm.mixerGetID
@ stdcall mixerGetLineControlsA(long ptr long) winmm.mixerGetLineControlsA
@ stdcall mixerGetLineControlsW(long ptr long) winmm.mixerGetLineControlsW
@ stdcall mixerGetLineInfoA(long ptr long) winmm.mixerGetLineInfoA
@ stdcall mixerGetLineInfoW(long ptr long) winmm.mixerGetLineInfoW
@ stdcall mixerGetNumDevs() winmm.mixerGetNumDevs
@ stdcall mixerMessage(long long long long) winmm.mixerMessage
@ stdcall mixerOpen(ptr long long long long) winmm.mixerOpen
@ stdcall mixerSetControlDetails(long ptr long) winmm.mixerSetControlDetails
@ stdcall waveInAddBuffer(long ptr long) winmm.waveInAddBuffer
@ stdcall waveInClose(long) winmm.waveInClose
@ stdcall waveInGetDevCapsA(long ptr long) winmm.waveInGetDevCapsA
@ stdcall waveInGetDevCapsW(long ptr long) winmm.waveInGetDevCapsW
@ stdcall waveInGetErrorTextA(long ptr long) winmm.waveInGetErrorTextA
@ stdcall waveInGetErrorTextW(long ptr long) winmm.waveInGetErrorTextW
@ stdcall waveInGetID(long ptr) winmm.waveInGetID
@ stdcall waveInGetNumDevs() winmm.waveInGetNumDevs
@ stdcall waveInGetPosition(long ptr long) winmm.waveInGetPosition
@ stdcall waveInMessage(long long long long) winmm.waveInMessage
@ stdcall waveInOpen(ptr long ptr long long long) winmm.waveInOpen
@ stdcall waveInPrepareHeader(long ptr long) winmm.waveInPrepareHeader
@ stdcall waveInReset(long) winmm.waveInReset
@ stdcall waveInStart(long) winmm.waveInStart
@ stdcall waveInStop(long) winmm.waveInStop
@ stdcall waveInUnprepareHeader(long ptr long) winmm.waveInUnprepareHeader
@ stdcall waveOutBreakLoop(long) winmm.waveOutBreakLoop
@ stdcall waveOutClose(long) winmm.waveOutClose
@ stdcall waveOutGetDevCapsA(long ptr long) winmm.waveOutGetDevCapsA
@ stdcall waveOutGetDevCapsW(long ptr long) winmm.waveOutGetDevCapsW
@ stdcall waveOutGetErrorTextA(long ptr long) winmm.waveOutGetErrorTextA
@ stdcall waveOutGetErrorTextW(long ptr long) winmm.waveOutGetErrorTextW
@ stdcall waveOutGetID(long ptr) winmm.waveOutGetID
@ stdcall waveOutGetNumDevs() winmm.waveOutGetNumDevs
@ stdcall waveOutGetPitch(long ptr) winmm.waveOutGetPitch
@ stdcall waveOutGetPlaybackRate(long ptr) winmm.waveOutGetPlaybackRate
@ stdcall waveOutGetPosition(long ptr long) winmm.waveOutGetPosition
@ stdcall waveOutGetVolume(long ptr) winmm.waveOutGetVolume
@ stdcall waveOutMessage(long long long long) winmm.waveOutMessage
@ stdcall waveOutOpen(ptr long ptr long long long) winmm.waveOutOpen
@ stdcall waveOutPause(long) winmm.waveOutPause
@ stdcall waveOutPrepareHeader(long ptr long) winmm.waveOutPrepareHeader
@ stdcall waveOutReset(long) winmm.waveOutReset
@ stdcall waveOutRestart(long) winmm.waveOutRestart
@ stdcall waveOutSetPitch(long long) winmm.waveOutSetPitch
@ stdcall waveOutSetPlaybackRate(long long) winmm.waveOutSetPlaybackRate
@ stdcall waveOutSetVolume(long long) winmm.waveOutSetVolume
@ stdcall waveOutUnprepareHeader(long ptr long) winmm.waveOutUnprepareHeader
@ stdcall waveOutWrite(long ptr long) winmm.waveOutWrite
