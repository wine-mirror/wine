name	mmsystem
type	win16

#1      pascal  MMSYSTEM_WEP(word word word ptr) MMSYSTEM_WEP
2      pascal  SNDPLAYSOUND(ptr word) sndPlaySound
3      stub    PLAYSOUND
5      pascal  mmsystemGetVersion() mmsystemGetVersion16
6      pascal  DriverProc(long word word long long) DriverProc
8      stub    WMMMIDIRUNONCE
30     pascal16 OutputDebugStr(ptr) OutputDebugString16
31     pascal  DriverCallback(long word word word long long long) DriverCallback
32     stub    STACKENTER
33     stub    STACKLEAVE
34     stub    MMDRVINSTALL
101    pascal  joyGetNumDevs() joyGetNumDevs16
102    pascal  joyGetDevCaps(word ptr word) joyGetDevCaps16
103    pascal  joyGetPos(word ptr) joyGetPos16
104    pascal  joyGetThreshold(word ptr) joyGetThreshold16
105    pascal  joyReleaseCapture(word) joyReleaseCapture16
106    pascal  joySetCapture(word word word word) joySetCapture16
107    pascal  joySetThreshold(word word) joySetThreshold16
109    pascal  joySetCalibration(word) joySetCalibration16
110    stub    JOYGETPOSEX
111    stub    JOYCONFIGCHANGED
201    pascal  midiOutGetNumDevs() midiOutGetNumDevs16
202    pascal  midiOutGetDevCaps(word str word) midiOutGetDevCaps16
203    pascal  midiOutGetErrorText(word ptr word) midiOutGetErrorText16
204    pascal  midiOutOpen(ptr word long long long) midiOutOpen16
205    pascal  midiOutClose(word) midiOutClose16
206    pascal  midiOutPrepareHeader(word ptr word) midiOutPrepareHeader16
207    pascal  midiOutUnprepareHeader(word ptr word) midiOutUnprepareHeader16
208    pascal  midiOutShortMsg(word long) midiOutShortMsg16
209    pascal  midiOutLongMsg(word ptr word) midiOutLongMsg16
210    pascal  midiOutReset(word) midiOutReset16
211    pascal  midiOutGetVolume(word ptr) midiOutGetVolume16
212    pascal  midiOutSetVolume(word long) midiOutSetVolume16
213    pascal  midiOutCachePatches(word word ptr word) midiOutCachePatches16
214    pascal  midiOutCacheDrumPatches(word word ptr word) midiOutCacheDrumPatches16
215    pascal  midiOutGetID(word ptr) midiOutGetID16
216    pascal  midiOutMessage(word word long long) midiOutMessage16
250    stub    MIDISTREAMPROPERTY
251    stub    MIDISTREAMOPEN
252    stub    MIDISTREAMCLOSE
253    stub    MIDISTREAMPOSITION
254    stub    MIDISTREAMOUT
255    stub    MIDISTREAMPAUSE
256    stub    MIDISTREAMRESTART
257    stub    MIDISTREAMSTOP
301    pascal  midiInGetNumDevs() midiInGetNumDevs16
302    pascal  midiInGetDevCaps(word ptr word) midiInGetDevCaps16
303    pascal  midiInGetErrorText(word ptr word) midiInGetErrorText16
304    pascal  midiInOpen(ptr word ptr long long long) midiInOpen16
305    pascal  midiInClose(word) midiInClose16
306    pascal  midiInPrepareHeader(word ptr word) midiInPrepareHeader16
307    pascal  midiInUnprepareHeader(word ptr word) midiInUnprepareHeader16
308    pascal  midiInAddBuffer(word ptr word) midiInAddBuffer16
309    pascal  midiInStart(word) midiInStart16
310    pascal  midiInStop(word) midiInStop16
311    pascal  midiInReset(word) midiInReset16
312    pascal  midiInGetID(word ptr) midiInGetID16
313    pascal  midiInMessage(word word long long) midiInMessage16
350    pascal  auxGetNumDevs() auxGetNumDevs16
351    pascal  auxGetDevCaps(word ptr word) auxGetDevCaps16
352    pascal  auxGetVolume(word ptr) auxGetVolume16
353    pascal  auxSetVolume(word long) auxSetVolume16
354    pascal  auxOutMessage(word word long long) auxOutMessage16
401    pascal  waveOutGetNumDevs() waveOutGetNumDevs16
402    pascal  waveOutGetDevCaps(word ptr word) waveOutGetDevCaps16
403    pascal  waveOutGetErrorText(word ptr word) waveOutGetErrorText16
404    pascal  waveOutOpen(ptr word ptr long long long) waveOutOpen16
405    pascal  waveOutClose(word) waveOutClose16
406    pascal  waveOutPrepareHeader(word ptr word) waveOutPrepareHeader16
407    pascal  waveOutUnprepareHeader(word ptr word) waveOutUnprepareHeader16
408    pascal  waveOutWrite(word ptr word) waveOutWrite16
409    pascal  waveOutPause(word) waveOutPause16
410    pascal  waveOutRestart(word) waveOutRestart16
411    pascal  waveOutReset(word) waveOutReset16
412    pascal  waveOutGetPosition(word ptr word) waveOutGetPosition16
413    pascal  waveOutGetPitch(word ptr) waveOutGetPitch16
414    pascal  waveOutSetPitch(word long) waveOutSetPitch16
415    pascal  waveOutGetVolume(word ptr) waveOutGetVolume16
416    pascal  waveOutSetVolume(word long) waveOutSetVolume16
417    pascal  waveOutGetPlaybackRate(word ptr) waveOutGetPlaybackRate16
418    pascal  waveOutSetPlaybackRate(word long) waveOutSetPlaybackRate16
419    pascal  waveOutBreakLoop(word) waveOutBreakLoop16
420    pascal  waveOutGetID(word ptr) waveOutGetID16
421    pascal  waveOutMessage(word word long long) waveOutMessage16
501    pascal  waveInGetNumDevs() waveInGetNumDevs16
502    pascal  waveInGetDevCaps(word ptr word) waveInGetDevCaps16
503    pascal  waveInGetErrorText(word ptr word) waveInGetErrorText16
504    pascal  waveInOpen(ptr word ptr long long long) waveInOpen16
505    pascal  waveInClose(word) waveInClose16
506    pascal  waveInPrepareHeader(word ptr word) waveInPrepareHeader16
507    pascal  waveInUnprepareHeader(word ptr word) waveInUnprepareHeader16
508    pascal  waveInAddBuffer(word ptr word) waveInAddBuffer16
509    pascal  waveInStart(word) waveInStart16
510    pascal  waveInStop(word) waveInStop16
511    pascal  waveInReset(word) waveInReset16
512    pascal  waveInGetPosition(word ptr word) waveInGetPosition16
513    pascal  waveInGetID(word ptr) waveInGetID16
514    pascal  waveInMessage(word word long long) waveInMessage16
601    pascal  timeGetSystemTime(ptr word) timeGetSystemTime16
602    pascal  timeSetEvent(word word segptr long word) timeSetEvent16
603    pascal  timeKillEvent(word) timeKillEvent16
604    pascal  timeGetDevCaps(ptr word) timeGetDevCaps16
605    pascal  timeBeginPeriod(word) timeBeginPeriod16
606    pascal  timeEndPeriod(word) timeEndPeriod16
607    pascal  timeGetTime() timeGetTime
701    pascal  MCISENDCOMMAND(word word long long) mciSendCommand
702    pascal  MCISENDSTRING(str ptr word word) mciSendString
703    pascal  mciGetDeviceID(ptr) mciGetDeviceID
705    pascal  mciLoadCommandResource(word str word) mciLoadCommandResource16
706    pascal  mciGetErrorString(long ptr word) mciGetErrorString16
707    pascal  mciSetDriverData(word long) mciSetDriverData16
708    pascal  mciGetDriverData(word) mciGetDriverData16
710    stub    MCIDRIVERYIELD
711    stub    MCIDRIVERNOTIFY
712    stub    MCIEXECUTE
713    stub    MCIFREECOMMANDRESOURCE
714    stub    MCISETYIELDPROC
715    stub    MCIGETDEVICEIDFROMELEMENTID
716    stub    MCIGETYIELDPROC
717    stub    MCIGETCREATORTASK
800    pascal  mixerGetNumDevs() mixerGetNumDevs16
801    pascal  mixerGetDevCaps(word ptr long) mixerGetDevCaps16
802    pascal  mixerOpen(ptr word long long) mixerOpen16
803    pascal  mixerClose(word) mixerClose16
804    pascal  mixerMessage(word word long long) mixerMessage16
805    pascal  mixerGetLineInfo(word ptr long) mixerGetLineInfo16
806    pascal  mixerGetID(word) mixerGetID16
807    pascal  mixerGetLineControls(word ptr long) mixerGetLineControls16
808    pascal  mixerGetControlDetails(word ptr long) mixerGetControlDetails16
809    pascal  mixerSetControlDetails(word ptr long) mixerSetControlDetails16
900    pascal  mmTaskCreate(ptr long long) mmTaskCreate16
902    stub    MMTASKBLOCK
903    pascal  mmTaskSignal(word) mmTaskSignal16
904    stub    MMGETCURRENTTASK
905    stub    MMTASKYIELD
1100   pascal  DRVOPEN(str str long) DrvOpen
1101   pascal  DRVCLOSE(word long long) DrvClose
1102   pascal  DRVSENDMESSAGE(word word long long) DrvSendMessage
1103   pascal  DRVGETMODULEHANDLE(word) DrvGetModuleHandle
1104   pascal  DRVDEFDRIVERPROC(long word word long long) DrvDefDriverProc
1120   pascal  mmThreadCreate(ptr ptr long long) mmThreadCreate16
1121   pascal  mmThreadSignal(word) mmThreadSignal16
1122   stub    MMTHREADBLOCK
1123   stub    MMTHREADISCURRENT
1124   stub    MMTHREADISVALID
1125   pascal  mmThreadGetTask(word) mmThreadGetTask16
1150   stub    MMSHOWMMCPLPROPERTYSHEET
1210   pascal  mmioOpen(str ptr long) mmioOpen16
1211   pascal  MMIOCLOSE(word word) mmioClose
1212   pascal  MMIOREAD(word ptr long) mmioRead
1213   pascal  MMIOWRITE(word ptr long) mmioWrite
1214   pascal  MMIOSEEK(word long word) mmioSeek
1215   pascal  MMIOGETINFO(word ptr word) mmioGetInfo
1216   pascal  MMIOSETINFO(word ptr word) mmioSetInfo
1217   pascal  MMIOSETBUFFER(word ptr long word) mmioSetBuffer
1218   pascal  MMIOFLUSH(word word) mmioFlush
1219   pascal  MMIOADVANCE(word ptr word) mmioAdvance
1220   pascal  mmioStringToFOURCC(str word) mmioStringToFOURCC16
1221   pascal  MMIOINSTALLIOPROC(long ptr long) mmioInstallIOProc16
1222   pascal  MMIOSENDMESSAGE(word word long long) mmioSendMessage
1223   pascal  MMIODESCEND(word ptr ptr word) mmioDescend
1224   pascal  MMIOASCEND(word ptr word) mmioAscend
1225   pascal  MMIOCREATECHUNK(word ptr word) mmioCreateChunk
1226   pascal  MMIORENAME(ptr ptr ptr long) mmioRename
#2000   stub    WINMMF_THUNKDATA16
#2001   stub    RING3_DEVLOADER
#2002   stub    WINMMTILEBUFFER
#2003   stub    WINMMUNTILEBUFFER
#2005   stub    MCIGETTHUNKTABLE
#2006   stub    WINMMSL_THUNKDATA16
