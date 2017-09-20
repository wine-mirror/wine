@ stdcall AddVectoredContinueHandler(long ptr) kernel32.AddVectoredContinueHandler
@ stdcall AddVectoredExceptionHandler(long ptr) kernel32.AddVectoredExceptionHandler
@ stdcall FatalAppExitA(long str) kernel32.FatalAppExitA
@ stdcall FatalAppExitW(long wstr) kernel32.FatalAppExitW
@ stdcall GetErrorMode() kernel32.GetErrorMode
@ stdcall GetLastError() kernel32.GetLastError
@ stdcall GetThreadErrorMode() kernel32.GetThreadErrorMode
@ stdcall RaiseException(long long long ptr) kernel32.RaiseException
@ stub RaiseFailFastException
@ stdcall RemoveVectoredContinueHandler(ptr) kernel32.RemoveVectoredContinueHandler
@ stdcall RemoveVectoredExceptionHandler(ptr) kernel32.RemoveVectoredExceptionHandler
@ stdcall SetErrorMode(long) kernel32.SetErrorMode
@ stdcall SetLastError(long) kernel32.SetLastError
@ stdcall SetThreadErrorMode(long ptr) kernel32.SetThreadErrorMode
@ stdcall SetUnhandledExceptionFilter(ptr) kernel32.SetUnhandledExceptionFilter
@ stdcall UnhandledExceptionFilter(ptr) kernel32.UnhandledExceptionFilter
