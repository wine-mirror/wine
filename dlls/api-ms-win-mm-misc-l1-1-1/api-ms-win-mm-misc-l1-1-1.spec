@ stdcall CloseDriver(long long long) winmm.CloseDriver
@ stdcall DefDriverProc(long long long long long) winmm.DefDriverProc
@ stdcall DriverCallback(long long long long long long long) winmm.DriverCallback
@ stdcall DrvGetModuleHandle(long) winmm.DrvGetModuleHandle
@ stdcall GetDriverModuleHandle(long) winmm.GetDriverModuleHandle
@ stdcall OpenDriver(wstr wstr long) winmm.OpenDriver
@ stdcall SendDriverMessage(long long long long) winmm.SendDriverMessage
@ stub mmDrvInstall
@ stdcall mmioAdvance(long ptr long) winmm.mmioAdvance
@ stdcall mmioAscend(long ptr long) winmm.mmioAscend
@ stdcall mmioClose(long long) winmm.mmioClose
@ stdcall mmioCreateChunk(long ptr long) winmm.mmioCreateChunk
@ stdcall mmioDescend(long ptr ptr long) winmm.mmioDescend
@ stdcall mmioFlush(long long) winmm.mmioFlush
@ stdcall mmioGetInfo(long ptr long) winmm.mmioGetInfo
@ stdcall mmioInstallIOProcA(long ptr long) winmm.mmioInstallIOProcA
@ stdcall mmioInstallIOProcW(long ptr long) winmm.mmioInstallIOProcW
@ stdcall mmioOpenA(str ptr long) winmm.mmioOpenA
@ stdcall mmioOpenW(wstr ptr long) winmm.mmioOpenW
@ stdcall mmioRead(long ptr long) winmm.mmioRead
@ stdcall mmioRenameA(str str ptr long) winmm.mmioRenameA
@ stdcall mmioRenameW(wstr wstr ptr long) winmm.mmioRenameW
@ stdcall mmioSeek(long long long) winmm.mmioSeek
@ stdcall mmioSendMessage(long long long long) winmm.mmioSendMessage
@ stdcall mmioSetBuffer(long ptr long long) winmm.mmioSetBuffer
@ stdcall mmioSetInfo(long ptr long) winmm.mmioSetInfo
@ stdcall mmioStringToFOURCCA(str long) winmm.mmioStringToFOURCCA
@ stdcall mmioStringToFOURCCW(wstr long) winmm.mmioStringToFOURCCW
@ stdcall mmioWrite(long ptr long) winmm.mmioWrite
@ stub sndOpenSound
