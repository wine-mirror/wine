@ stdcall -private DllCanUnloadNow()
@ stub -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stub NdfRunDllDiagnoseIncident
@ stub NdfRunDllDiagnoseNetConnectionIncident
@ stub NdfRunDllDiagnoseWithAnswerFile
@ stub NdfRunDllDuplicateIPDefendingSystem
@ stub NdfRunDllDuplicateIPOffendingSystem
@ stub NdfRunDllHelpTopic
@ stub NdfCancelIncident
@ stdcall NdfCloseIncident(ptr)
@ stdcall NdfCreateConnectivityIncident(ptr)
@ stub NdfCreateDNSIncident
@ stub NdfCreateGroupingIncident
@ stub NdfCreateInboundIncident
@ stub NdfCreateIncident
@ stub NdfCreateNetConnectionIncident
@ stub NdfCreatePnrpIncident
@ stub NdfCreateSharingIncident
@ stdcall NdfCreateWebIncident(str ptr)
@ stub NdfCreateWebIncidentEx
@ stdcall NdfCreateWinSockIncident(ptr str long str ptr ptr)
@ stdcall NdfDiagnoseIncident(ptr ptr ptr long long)
@ stdcall NdfExecuteDiagnosis(ptr ptr)
@ stub NdfGetTraceFile
@ stub NdfRepairIncident
@ stub NdfRepairIncidentEx
