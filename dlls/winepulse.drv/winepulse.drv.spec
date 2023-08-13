# MMDevAPI driver functions
@ stdcall -private get_device_guid(long ptr ptr) get_device_guid
@ stdcall -private get_device_name_from_guid(ptr ptr ptr) get_device_name_from_guid

# WinMM driver functions
@ stdcall -private DriverProc(long long long long long) winealsa.drv.DriverProc
@ stdcall -private midMessage(long long long long long) winealsa.drv.midMessage
@ stdcall -private modMessage(long long long long long) winealsa.drv.modMessage
