# WinMM driver functions
@ stdcall -private DriverProc(long long long long long) OSS_DriverProc
@ stdcall -private auxMessage(long long long long long) OSS_auxMessage
@ stdcall -private midMessage(long long long long long) OSS_midMessage
@ stdcall -private modMessage(long long long long long) OSS_modMessage

# MMDevAPI driver functions
@ stdcall -private get_device_guid(long ptr ptr) get_device_guid
@ stdcall -private get_device_name_from_guid(ptr ptr ptr) get_device_name_from_guid
