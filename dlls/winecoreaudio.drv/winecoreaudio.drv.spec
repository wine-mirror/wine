# WinMM driver functions
@ stdcall -private DriverProc(long long long long long) CoreAudio_DriverProc
@ stdcall -private midMessage(long long long long long) CoreAudio_midMessage
@ stdcall -private modMessage(long long long long long) CoreAudio_modMessage

# MMDevAPI driver functions
@ stdcall -private GetPriority() AUDDRV_GetPriority
@ stdcall -private GetEndpointIDs(long ptr ptr ptr ptr) AUDDRV_GetEndpointIDs
@ stdcall -private GetAudioEndpoint(ptr ptr ptr) AUDDRV_GetAudioEndpoint
@ stdcall -private GetAudioSessionManager(ptr ptr) AUDDRV_GetAudioSessionManager
