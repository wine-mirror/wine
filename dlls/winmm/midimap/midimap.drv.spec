name midimap
file midimap.drv
type win32

debug_channels (msacm)

@ stdcall DriverProc(long long long long long) MIDIMAP_DriverProc
@ stdcall modMessage(long long long long long) MIDIMAP_modMessage
