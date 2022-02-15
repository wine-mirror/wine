@ stdcall AddVectoredContinueHandler(long ptr) kernelbase.AddVectoredContinueHandler
@ stdcall AddVectoredExceptionHandler(long ptr) kernelbase.AddVectoredExceptionHandler
@ stdcall FatalAppExitA(long str) kernelbase.FatalAppExitA
@ stdcall FatalAppExitW(long wstr) kernelbase.FatalAppExitW
@ stdcall GetErrorMode() kernelbase.GetErrorMode
@ stdcall GetLastError() kernelbase.GetLastError
@ stdcall GetThreadErrorMode() kernelbase.GetThreadErrorMode
@ stdcall RaiseException(long long long ptr) kernelbase.RaiseException
@ stub RaiseFailFastException
@ stdcall RemoveVectoredContinueHandler(ptr) kernelbase.RemoveVectoredContinueHandler
@ stdcall RemoveVectoredExceptionHandler(ptr) kernelbase.RemoveVectoredExceptionHandler
@ stdcall SetErrorMode(long) kernelbase.SetErrorMode
@ stdcall SetLastError(long) kernelbase.SetLastError
@ stdcall SetThreadErrorMode(long ptr) kernelbase.SetThreadErrorMode
@ stdcall SetUnhandledExceptionFilter(ptr) kernelbase.SetUnhandledExceptionFilter
@ stdcall UnhandledExceptionFilter(ptr) kernelbase.UnhandledExceptionFilter
