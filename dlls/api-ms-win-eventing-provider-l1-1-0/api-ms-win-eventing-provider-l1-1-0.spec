@ stdcall EventActivityIdControl(long ptr) advapi32.EventActivityIdControl
@ stdcall EventEnabled(int64 ptr) advapi32.EventEnabled
@ stdcall EventProviderEnabled(int64 long int64) advapi32.EventProviderEnabled
@ stdcall EventRegister(ptr ptr ptr ptr) advapi32.EventRegister
@ stdcall EventSetInformation(int64 long ptr long) advapi32.EventSetInformation
@ stdcall EventUnregister(int64) advapi32.EventUnregister
@ stdcall EventWrite(int64 ptr long ptr) advapi32.EventWrite
@ stub EventWriteEx
@ stdcall EventWriteString(int64 long int64 ptr) advapi32.EventWriteString
@ stdcall EventWriteTransfer(int64 ptr ptr ptr long ptr) advapi32.EventWriteTransfer
