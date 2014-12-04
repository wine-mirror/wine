@ stdcall AddVectoredContinueHandler(long ptr) kernel32.AddVectoredContinueHandler
@ stdcall AddVectoredExceptionHandler(long ptr) kernel32.AddVectoredExceptionHandler
@ stdcall GetErrorMode() kernel32.GetErrorMode
@ stdcall GetLastError() kernel32.GetLastError
@ stdcall RaiseException(long long long ptr) kernel32.RaiseException
@ stub RaiseFailFastException
@ stdcall RemoveVectoredContinueHandler(ptr) kernel32.RemoveVectoredContinueHandler
@ stdcall RemoveVectoredExceptionHandler(ptr) kernel32.RemoveVectoredExceptionHandler
@ stdcall RestoreLastError(long) kernel32.RestoreLastError
@ stdcall SetErrorMode(long) kernel32.SetErrorMode
@ stdcall SetLastError(long) kernel32.SetLastError
@ stdcall SetUnhandledExceptionFilter(ptr) kernel32.SetUnhandledExceptionFilter
@ stdcall UnhandledExceptionFilter(ptr) kernel32.UnhandledExceptionFilter
