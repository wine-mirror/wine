@ stdcall ClearCommBreak(long) kernelbase.ClearCommBreak
@ stdcall ClearCommError(long ptr ptr) kernelbase.ClearCommError
@ stdcall EscapeCommFunction(long long) kernelbase.EscapeCommFunction
@ stdcall GetCommConfig(long ptr ptr) kernelbase.GetCommConfig
@ stdcall GetCommMask(long ptr) kernelbase.GetCommMask
@ stdcall GetCommModemStatus(long ptr) kernelbase.GetCommModemStatus
@ stdcall GetCommProperties(long ptr) kernelbase.GetCommProperties
@ stdcall GetCommState(long ptr) kernelbase.GetCommState
@ stdcall GetCommTimeouts(long ptr) kernelbase.GetCommTimeouts
@ stdcall PurgeComm(long long) kernelbase.PurgeComm
@ stdcall SetCommBreak(long) kernelbase.SetCommBreak
@ stdcall SetCommConfig(long ptr long) kernelbase.SetCommConfig
@ stdcall SetCommMask(long long) kernelbase.SetCommMask
@ stdcall SetCommState(long ptr) kernelbase.SetCommState
@ stdcall SetCommTimeouts(long ptr) kernelbase.SetCommTimeouts
@ stdcall SetupComm(long long long) kernelbase.SetupComm
@ stdcall TransmitCommChar(long long) kernelbase.TransmitCommChar
@ stdcall WaitCommEvent(long ptr ptr) kernelbase.WaitCommEvent
