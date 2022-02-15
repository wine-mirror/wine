@ stdcall EventActivityIdControl(long ptr) kernelbase.EventActivityIdControl
@ stdcall EventEnabled(int64 ptr) kernelbase.EventEnabled
@ stdcall EventProviderEnabled(int64 long int64) kernelbase.EventProviderEnabled
@ stdcall EventRegister(ptr ptr ptr ptr) kernelbase.EventRegister
@ stdcall EventSetInformation(int64 long ptr long) kernelbase.EventSetInformation
@ stdcall EventUnregister(int64) kernelbase.EventUnregister
@ stdcall EventWrite(int64 ptr long ptr) kernelbase.EventWrite
@ stub EventWriteEx
@ stdcall EventWriteString(int64 long int64 ptr) kernelbase.EventWriteString
@ stdcall EventWriteTransfer(int64 ptr ptr ptr long ptr) kernelbase.EventWriteTransfer
