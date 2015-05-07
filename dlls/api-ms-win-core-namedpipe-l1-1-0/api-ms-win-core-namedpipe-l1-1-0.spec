@ stdcall ConnectNamedPipe(long ptr) kernel32.ConnectNamedPipe
@ stdcall CreateNamedPipeW(wstr long long long long long long ptr) kernel32.CreateNamedPipeW
@ stdcall CreatePipe(ptr ptr ptr long) kernel32.CreatePipe
@ stdcall DisconnectNamedPipe(long) kernel32.DisconnectNamedPipe
@ stub GetNamedPipeAttribute
@ stub GetNamedPipeClientComputerNameW
@ stdcall ImpersonateNamedPipeClient(long) advapi32.ImpersonateNamedPipeClient
@ stdcall PeekNamedPipe(long ptr long ptr ptr ptr) kernel32.PeekNamedPipe
@ stdcall SetNamedPipeHandleState(long ptr ptr ptr) kernel32.SetNamedPipeHandleState
@ stdcall TransactNamedPipe(long ptr long ptr long ptr ptr) kernel32.TransactNamedPipe
@ stdcall WaitNamedPipeW(wstr long) kernel32.WaitNamedPipeW
