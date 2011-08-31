# WinMM driver functions
@ stdcall -private DriverProc(long long long long long) OSS_DriverProc
@ stdcall -private auxMessage(long long long long long) OSS_auxMessage
@ stdcall -private mxdMessage(long long long long long) OSS_mxdMessage
@ stdcall -private midMessage(long long long long long) OSS_midMessage
@ stdcall -private modMessage(long long long long long) OSS_modMessage
@ stdcall -private widMessage(long long long long long) OSS_widMessage
@ stdcall -private wodMessage(long long long long long) OSS_wodMessage

# MMDevAPI driver functions
@ stdcall -private GetPriority() AUDDRV_GetPriority
@ stdcall -private GetEndpointIDs(long ptr ptr ptr ptr) AUDDRV_GetEndpointIDs
@ stdcall -private GetAudioEndpoint(ptr ptr long ptr) AUDDRV_GetAudioEndpoint
@ stdcall -private GetAudioSessionManager(ptr ptr) AUDDRV_GetAudioSessionManager
