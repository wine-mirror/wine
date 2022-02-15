@ stdcall ChangeServiceConfig2A(long long ptr) sechost.ChangeServiceConfig2A
@ stdcall ChangeServiceConfigA(long long long long str str ptr str str str str) sechost.ChangeServiceConfigA
@ stdcall ControlService(long long ptr) sechost.ControlService
@ stub ControlServiceExA
@ stdcall CreateServiceA(long str str long long long long str str ptr str str str) sechost.CreateServiceA
@ stub I_QueryTagInformation
@ stub I_ScBroadcastServiceControlMessage
@ stub I_ScIsSecurityProcess
@ stub I_ScPnPGetServiceName
@ stub I_ScQueryServiceConfig
@ stub I_ScRpcBindA
@ stub I_ScRpcBindW
@ stub I_ScSendPnPMessage
@ stub I_ScSendTSMessage
@ stub I_ScValidatePnPService
@ stub NotifyServiceStatusChangeA
@ stdcall OpenSCManagerA(str str long) sechost.OpenSCManagerA
@ stdcall OpenServiceA(long str long) sechost.OpenServiceA
@ stdcall QueryServiceConfig2A(long long ptr long ptr) sechost.QueryServiceConfig2A
@ stdcall QueryServiceConfigA(long ptr long ptr) sechost.QueryServiceConfigA
@ stdcall QueryServiceStatus(long ptr) sechost.QueryServiceStatus
@ stdcall RegisterServiceCtrlHandlerA(str ptr) sechost.RegisterServiceCtrlHandlerA
@ stdcall RegisterServiceCtrlHandlerExA(str ptr ptr) sechost.RegisterServiceCtrlHandlerExA
@ stdcall RegisterServiceCtrlHandlerW(wstr ptr) sechost.RegisterServiceCtrlHandlerW
@ stdcall StartServiceA(long long ptr) sechost.StartServiceA
@ stdcall StartServiceCtrlDispatcherA(ptr) sechost.StartServiceCtrlDispatcherA
