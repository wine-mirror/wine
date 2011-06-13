# WinMM driver functions
@ stdcall -private DriverProc(long long long long long) ALSA_DriverProc
@ stdcall -private midMessage(long long long long long) ALSA_midMessage
@ stdcall -private modMessage(long long long long long) ALSA_modMessage
@ stdcall -private mxdMessage(long long long long long) ALSA_mxdMessage
@ stdcall -private widMessage(long long long long long) ALSA_widMessage
@ stdcall -private wodMessage(long long long long long) ALSA_wodMessage

# MMDevAPI driver functions
@ stdcall -private GetEndpointIDs(long ptr ptr ptr ptr) AUDDRV_GetEndpointIDs
@ stdcall -private GetAudioEndpoint(ptr ptr long ptr) AUDDRV_GetAudioEndpoint
@ stdcall -private GetAudioSessionManager(ptr ptr) AUDDRV_GetAudioSessionManager
