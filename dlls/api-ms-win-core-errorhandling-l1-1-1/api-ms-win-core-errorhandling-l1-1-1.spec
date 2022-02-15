@ stdcall AddVectoredContinueHandler(long ptr) kernelbase.AddVectoredContinueHandler
@ stdcall AddVectoredExceptionHandler(long ptr) kernelbase.AddVectoredExceptionHandler
@ stdcall GetErrorMode() kernelbase.GetErrorMode
@ stdcall GetLastError() kernelbase.GetLastError
@ stdcall RaiseException(long long long ptr) kernelbase.RaiseException
@ stdcall RemoveVectoredContinueHandler(ptr) kernelbase.RemoveVectoredContinueHandler
@ stdcall RemoveVectoredExceptionHandler(ptr) kernelbase.RemoveVectoredExceptionHandler
@ stdcall RestoreLastError(long) kernelbase.RestoreLastError
@ stdcall SetErrorMode(long) kernelbase.SetErrorMode
@ stdcall SetLastError(long) kernelbase.SetLastError
@ stdcall SetUnhandledExceptionFilter(ptr) kernelbase.SetUnhandledExceptionFilter
@ stdcall UnhandledExceptionFilter(ptr) kernelbase.UnhandledExceptionFilter
