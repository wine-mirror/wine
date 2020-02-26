@ stdcall CreateProcessA(str str ptr ptr long long ptr str ptr ptr) kernel32.CreateProcessA
@ stdcall CreateProcessAsUserW(long wstr wstr ptr ptr long long ptr wstr ptr ptr) kernel32.CreateProcessAsUserW
@ stdcall CreateProcessW(wstr wstr ptr ptr long long ptr wstr ptr ptr) kernel32.CreateProcessW
@ stdcall CreateRemoteThread(long ptr long ptr long long ptr) kernel32.CreateRemoteThread
@ stdcall CreateRemoteThreadEx(long ptr long ptr ptr long ptr ptr) kernel32.CreateRemoteThreadEx
@ stdcall CreateThread(ptr long ptr long long ptr) kernel32.CreateThread
@ stdcall DeleteProcThreadAttributeList(ptr) kernel32.DeleteProcThreadAttributeList
@ stdcall ExitProcess(long) kernel32.ExitProcess
@ stdcall ExitThread(long) kernel32.ExitThread
@ stdcall FlushProcessWriteBuffers() kernel32.FlushProcessWriteBuffers
@ stdcall -norelay GetCurrentProcess() kernel32.GetCurrentProcess
@ stdcall -norelay GetCurrentProcessId() kernel32.GetCurrentProcessId
@ stdcall -norelay GetCurrentThread() kernel32.GetCurrentThread
@ stdcall -norelay GetCurrentThreadId() kernel32.GetCurrentThreadId
@ stdcall GetExitCodeProcess(long ptr) kernel32.GetExitCodeProcess
@ stdcall GetExitCodeThread(long ptr) kernel32.GetExitCodeThread
@ stdcall GetPriorityClass(long) kernel32.GetPriorityClass
@ stdcall GetProcessId(long) kernel32.GetProcessId
@ stdcall GetProcessIdOfThread(long) kernel32.GetProcessIdOfThread
@ stdcall GetProcessTimes(long ptr ptr ptr ptr) kernel32.GetProcessTimes
@ stdcall GetProcessVersion(long) kernel32.GetProcessVersion
@ stdcall GetStartupInfoW(ptr) kernel32.GetStartupInfoW
@ stdcall GetThreadId(ptr) kernel32.GetThreadId
@ stdcall GetThreadPriority(long) kernel32.GetThreadPriority
@ stdcall GetThreadPriorityBoost(long ptr) kernel32.GetThreadPriorityBoost
@ stdcall InitializeProcThreadAttributeList(ptr long long ptr) kernel32.InitializeProcThreadAttributeList
@ stdcall OpenProcessToken(long long ptr) kernel32.OpenProcessToken
@ stdcall OpenThread(long long long) kernel32.OpenThread
@ stdcall OpenThreadToken(long long long ptr) kernel32.OpenThreadToken
@ stdcall ProcessIdToSessionId(long ptr) kernel32.ProcessIdToSessionId
@ stub QueryProcessAffinityUpdateMode
@ stdcall QueueUserAPC(ptr long long) kernel32.QueueUserAPC
@ stdcall ResumeThread(long) kernel32.ResumeThread
@ stdcall SetPriorityClass(long long) kernel32.SetPriorityClass
@ stdcall SetProcessAffinityUpdateMode(long long) kernel32.SetProcessAffinityUpdateMode
@ stdcall SetProcessShutdownParameters(long long) kernel32.SetProcessShutdownParameters
@ stdcall SetThreadPriority(long long) kernel32.SetThreadPriority
@ stdcall SetThreadPriorityBoost(long long) kernel32.SetThreadPriorityBoost
@ stdcall SetThreadStackGuarantee(ptr) kernel32.SetThreadStackGuarantee
@ stdcall SetThreadToken(ptr ptr) advapi32.SetThreadToken
@ stdcall SuspendThread(long) kernel32.SuspendThread
@ stdcall SwitchToThread() kernel32.SwitchToThread
@ stdcall TerminateProcess(long long) kernel32.TerminateProcess
@ stdcall TerminateThread(long long) kernel32.TerminateThread
@ stdcall TlsAlloc() kernel32.TlsAlloc
@ stdcall TlsFree(long) kernel32.TlsFree
@ stdcall TlsGetValue(long) kernel32.TlsGetValue
@ stdcall TlsSetValue(long ptr) kernel32.TlsSetValue
@ stdcall UpdateProcThreadAttribute(ptr long long ptr long ptr ptr) kernel32.UpdateProcThreadAttribute
