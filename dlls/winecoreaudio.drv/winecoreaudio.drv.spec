# WinMM driver functions
@ stdcall -private DriverProc(long long long long long) CoreAudio_DriverProc
@ stdcall -private midMessage(long long long long long) CoreAudio_midMessage
@ stdcall -private modMessage(long long long long long) CoreAudio_modMessage

# MMDevAPI driver functions
@ stdcall -private get_device_guid(long ptr ptr) get_device_guid
