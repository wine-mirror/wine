@ stdcall ChangeServiceConfig2A(long long ptr) advapi32.ChangeServiceConfig2A
@ stdcall ChangeServiceConfigA(long long long long str str ptr str str str str) advapi32.ChangeServiceConfigA
@ stdcall ControlService(long long ptr) advapi32.ControlService
@ stub ControlServiceExA
@ stdcall CreateServiceA(long str str long long long long str str ptr str str str) advapi32.CreateServiceA
@ stub NotifyServiceStatusChangeA
@ stdcall OpenSCManagerA(str str long) advapi32.OpenSCManagerA
@ stdcall OpenServiceA(long str long) advapi32.OpenServiceA
@ stdcall QueryServiceConfig2A(long long ptr long ptr) advapi32.QueryServiceConfig2A
@ stdcall QueryServiceConfigA(long ptr long ptr) advapi32.QueryServiceConfigA
@ stdcall QueryServiceStatus(long ptr) advapi32.QueryServiceStatus
@ stdcall RegisterServiceCtrlHandlerA(str ptr) advapi32.RegisterServiceCtrlHandlerA
@ stdcall RegisterServiceCtrlHandlerExA(str ptr ptr) advapi32.RegisterServiceCtrlHandlerExA
@ stdcall RegisterServiceCtrlHandlerW(wstr ptr) advapi32.RegisterServiceCtrlHandlerW
@ stdcall StartServiceA(long long ptr) advapi32.StartServiceA
@ stdcall StartServiceCtrlDispatcherA(ptr) advapi32.StartServiceCtrlDispatcherA
