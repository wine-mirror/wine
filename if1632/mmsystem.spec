name	mmsystem
type	win16
id	10

#1      pascal  MMSYSTEM_WEP(word word word ptr) MMSYSTEM_WEP
2      pascal  SNDPLAYSOUND(ptr word) sndPlaySound
5      pascal  MMSYSTEMGETVERSION() mmsystemGetVersion
6      pascal  DriverProc(long word word long long) DriverProc
30     pascal  OUTPUTDEBUGSTR(ptr) OutputDebugStr
31     pascal  DriverCallback(long word word word long long long)
               DriverCallback
#32    pascal  STACKENTER
#33    pascal  STACKLEAVE
#34    pascal  MMDRVINSTALL
101    pascal  JOYGETNUMDEVS() JoyGetNumDevs
102    pascal  JOYGETDEVCAPS(word ptr word) JoyGetDevCaps
103    pascal  JOYGETPOS(word ptr) JoyGetPos
104    pascal  JOYGETTHRESHOLD(word ptr) JoyGetThreshold
105    pascal  JOYRELEASECAPTURE(word) JoyReleaseCapture
106    pascal  JOYSETCAPTURE(word word word word) JoySetCapture
107    pascal  JOYSETTHRESHOLD(word word) JoySetThreshold
109    pascal  JOYSETCALIBRATION(word) JoySetCalibration
201    pascal  MIDIOUTGETNUMDEVS() midiOutGetNumDevs
202    pascal  MIDIOUTGETDEVCAPS(word segptr word) midiOutGetDevCaps
203    pascal  MIDIOUTGETERRORTEXT(word ptr word) midiOutGetErrorText
204    pascal  MIDIOUTOPEN(ptr word ptr long long long) midiOutOpen
205    pascal  MIDIOUTCLOSE(word) midiOutClose
206    pascal  MIDIOUTPREPAREHEADER(word segptr word) midiOutPrepareHeader
207    pascal  MIDIOUTUNPREPAREHEADER(word segptr word) midiOutUnprepareHeader
208    pascal  MIDIOUTSHORTMSG(word long) midiOutShortMsg
209    pascal  MIDIOUTLONGMSG(word ptr word) midiOutLongMsg
210    pascal  MIDIOUTRESET(word) midiOutReset
211    pascal  MIDIOUTGETVOLUME(word segptr) midiOutGetVolume
212    pascal  MIDIOUTSETVOLUME(word long) midiOutSetVolume
213    pascal  MIDIOUTCACHEPATCHES(word word segptr word) midiOutCachePatches
214    pascal  MIDIOUTCACHEDRUMPATCHES(word word segptr word) midiOutCacheDrumPatches
215    pascal  MIDIOUTGETID(word ptr) midiOutGetID
216    pascal  MIDIOUTMESSAGE(word word long long) midiOutMessage
301    pascal  MIDIINGETNUMDEVS() midiInGetNumDevs
302    pascal  MIDIINGETDEVCAPS(word segptr word) midiInGetDevCaps
303    pascal  MIDIINGETERRORTEXT(word ptr word) midiInGetErrorText
304    pascal  MIDIINOPEN(ptr word ptr long long long) midiInOpen
305    pascal  MIDIINCLOSE(word) midiInClose
306    pascal  MIDIINPREPAREHEADER(word segptr word) midiInPrepareHeader
307    pascal  MIDIINUNPREPAREHEADER(word segptr word) midiInUnprepareHeader
309    pascal  MIDIINSTART(word) midiInStart
310    pascal  MIDIINSTOP(word) midiInStop
311    pascal  MIDIINRESET(word) midiInReset
312    pascal  MIDIINGETID(word ptr) midiInGetID
313    pascal  MIDIINMESSAGE(word word long long) midiInMessage
350    pascal  AUXGETNUMDEVS() auxGetNumDevs
351    pascal  AUXGETDEVCAPS(word segptr word) auxGetDevCaps
352    pascal  AUXGETVOLUME(word segptr) auxGetVolume
353    pascal  AUXSETVOLUME(word long) auxSetVolume
354    pascal  AUXOUTMESSAGE(word word long long) auxOutMessage
401    pascal  WAVEOUTGETNUMDEVS() waveOutGetNumDevs
402    pascal  WAVEOUTGETDEVCAPS(word segptr word) waveOutGetDevCaps
403    pascal  WAVEOUTGETERRORTEXT(word ptr word) waveOutGetErrorText
404    pascal  WAVEOUTOPEN(ptr word ptr long long long) waveOutOpen
405    pascal  WAVEOUTCLOSE(word) waveOutClose
406    pascal  WAVEOUTPREPAREHEADER(word segptr word) waveOutPrepareHeader
407    pascal  WAVEOUTUNPREPAREHEADER(word segptr word) waveOutUnprepareHeader
408    pascal  WAVEOUTWRITE(word segptr word) waveOutWrite
409    pascal  WAVEOUTPAUSE(word) waveOutPause
410    pascal  WAVEOUTRESTART(word) waveOutRestart
411    pascal  WAVEOUTRESET(word) waveOutReset
412    pascal  WAVEOUTGETPOSITION(word segptr word) waveOutGetPosition
413    pascal  WAVEOUTGETPITCH(word ptr) waveOutGetPitch
414    pascal  WAVEOUTSETPITCH(word long) waveOutSetPitch
415    pascal  WAVEOUTGETVOLUME(word segptr) waveOutGetVolume
416    pascal  WAVEOUTSETVOLUME(word long) waveOutSetVolume
417    pascal  WAVEOUTGETPLAYBACKRATE(word ptr) waveOutGetPlaybackRate
418    pascal  WAVEOUTSETPLAYBACKRATE(word long) waveOutSetPlaybackRate
419    pascal  WAVEOUTBREAKLOOP(word) waveOutBreakLoop
420    pascal  WAVEOUTGETID(word ptr) waveOutGetID
421    pascal  WAVEOUTMESSAGE(word word long long) waveOutMessage
501    pascal  WAVEINGETNUMDEVS() waveInGetNumDevs
502    pascal  WAVEINGETDEVCAPS(word segptr word) waveInGetDevCaps
503    pascal  WAVEINGETERRORTEXT(word ptr word) waveInGetErrorText
504    pascal  WAVEINOPEN(ptr word ptr long long long) waveInOpen
505    pascal  WAVEINCLOSE(word) waveInClose
506    pascal  WAVEINPREPAREHEADER(word segptr word) waveInPrepareHeader
507    pascal  WAVEINUNPREPAREHEADER(word segptr word) waveInUnprepareHeader
508    pascal  WAVEINADDBUFFER(word segptr word) waveInAddBuffer
509    pascal  WAVEINSTART(word) waveInStart
510    pascal  WAVEINSTOP(word) waveInStop
511    pascal  WAVEINRESET(word) waveInReset
512    pascal  WAVEINGETPOSITION(word segptr word) waveInGetPosition
513    pascal  WAVEINGETID(word ptr) waveInGetID
514    pascal  WAVEINMESSAGE(word word long long) waveInMessage
601    pascal  timeGetSystemTime(ptr word) timeGetSystemTime
602    pascal  timeSetEvent(word word segptr long word) timeSetEvent
603    pascal  timeKillEvent(word) timeKillEvent
604    pascal  timeGetDevCaps(ptr word) timeGetDevCaps
605    pascal  timeBeginPeriod(word) timeBeginPeriod
606    pascal  timeEndPeriod(word) timeEndPeriod
607    pascal  timeGetTime() timeGetTime
701    pascal  MCISENDCOMMAND(word word long long) mciSendCommand
702    pascal  MCISENDSTRING(ptr ptr word word) mciSendString
703    pascal  MCIGETDEVICEID(ptr) mciSendCommand
706    pascal  MCIGETERRORSTRING(long ptr word) mciGetErrorString
#900   pascal  MMTASKCREATE
#902   pascal  MMTASKBLOCK
#903   pascal  MMTASKSIGNAL
#904   pascal  MMGETCURRENTTASK
#905   pascal  MMTASKYIELD
1100   pascal  DRVOPEN(ptr ptr long) DrvOpen
1101   pascal  DRVCLOSE(word long long) DrvClose
1102   pascal  DRVSENDMESSAGE(word word long long) DrvSendMessage
1103   pascal  DRVGETMODULEHANDLE(word) DrvGetModuleHandle
1104   pascal  DRVDEFDRIVERPROC(long word word long long) DrvDefDriverProc
1210   pascal  MMIOOPEN(ptr ptr long) mmioOpen
1211   pascal  MMIOCLOSE(word word) mmioClose
1212   pascal  MMIOREAD(word ptr long) mmioRead
1213   pascal  MMIOWRITE(word ptr long) mmioWrite
1214   pascal  MMIOSEEK(word long word) mmioSeek
1215   pascal  MMIOGETINFO(word ptr word) mmioGetInfo
1216   pascal  MMIOSETINFO(word ptr word) mmioSetInfo
1217   pascal  MMIOSETBUFFER(word ptr long word) mmioSetBuffer
1218   pascal  MMIOFLUSH(word word) mmioFlush
1219   pascal  MMIOADVANCE(word ptr word) mmioAdvance
1220   pascal  MMIOSTRINGTOFOURCC(ptr word) mmioStringToFOURCC
1221   pascal  MMIOINSTALLIOPROC(long ptr long) mmioInstallIOProc
1222   pascal  MMIOSENDMESSAGE(word word long long) mmioSendMessage
1223   pascal  MMIODESCEND(word ptr ptr word) mmioDescend
1224   pascal  MMIOASCEND(word ptr word) mmioAscend
1225   pascal  MMIOCREATECHUNK(word ptr word) mmioCreateChunk
1226   pascal  MMIORENAME(ptr ptr ptr long) mmioRename


