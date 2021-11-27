@ stub I_ScSetServiceBitsA
@ stub I_ScSetServiceBitsW
@ stub AuditComputeEffectivePolicyBySid
@ stub AuditEnumerateCategories
@ stub AuditEnumeratePerUserPolicy
@ stub AuditEnumerateSubCategories
@ stub AuditFree
@ stub AuditLookupCategoryNameW
@ stub AuditLookupSubCategoryNameW
@ stub AuditQueryGlobalSaclW
@ stub AuditQueryPerUserPolicy
@ stub AuditQuerySecurity
@ stdcall AuditQuerySystemPolicy(ptr long ptr) advapi32.AuditQuerySystemPolicy
@ stub AuditSetGlobalSaclW
@ stub AuditSetPerUserPolicy
@ stub AuditSetSecurity
@ stub AuditSetSystemPolicy
@ stub BuildSecurityDescriptorForSharingAccess
@ stub BuildSecurityDescriptorForSharingAccessEx
@ stub CapabilityCheck
@ stub CapabilityCheckForSingleSessionSku
@ stdcall ChangeServiceConfig2A(long long ptr)
@ stdcall ChangeServiceConfig2W(long long ptr)
@ stdcall ChangeServiceConfigA(long long long long str str ptr str str str str)
@ stdcall ChangeServiceConfigW(long long long long wstr wstr ptr wstr wstr wstr wstr)
@ stdcall CloseServiceHandle(long)
@ stdcall CloseTrace(int64)
@ stdcall ControlService(long long ptr)
@ stub ControlServiceExA
@ stub ControlServiceExW
@ stdcall ControlTraceA(int64 str ptr long)
@ stdcall ControlTraceW(int64 wstr ptr long)
@ stub ConvertSDToStringSDRootDomainW
@ stdcall ConvertSecurityDescriptorToStringSecurityDescriptorW(ptr long long ptr ptr)
@ stdcall ConvertSidToStringSidW(ptr ptr)
@ stub ConvertStringSDToSDDomainA
@ stub ConvertStringSDToSDDomainW
@ stub ConvertStringSDToSDRootDomainW
@ stdcall ConvertStringSecurityDescriptorToSecurityDescriptorW(wstr long ptr ptr)
@ stdcall ConvertStringSidToSidW(wstr ptr)
@ stdcall CreateServiceA(long str str long long long long str str ptr str str str)
@ stub CreateServiceEx
@ stdcall CreateServiceW(long wstr wstr long long long long wstr wstr ptr wstr wstr wstr)
@ stub CredBackupCredentials
@ stdcall CredDeleteA(str long long) advapi32.CredDeleteA
@ stdcall CredDeleteW(wstr long long) advapi32.CredDeleteW
@ stub CredEncryptAndMarshalBinaryBlob
@ stdcall CredEnumerateA(str long ptr ptr) advapi32.CredEnumerateA
@ stdcall CredEnumerateW(wstr long ptr ptr) advapi32.CredEnumerateW
@ stub CredFindBestCredentialA
@ stub CredFindBestCredentialW
@ stdcall CredFree(ptr) advapi32.CredFree
@ stdcall CredGetSessionTypes(long ptr) advapi32.CredGetSessionTypes
@ stub CredGetTargetInfoA
@ stub CredGetTargetInfoW
@ stdcall CredIsMarshaledCredentialW(wstr) advapi32.CredIsMarshaledCredentialW
@ stub CredIsProtectedA
@ stub CredIsProtectedW
@ stdcall CredMarshalCredentialA(long ptr ptr) advapi32.CredMarshalCredentialA
@ stdcall CredMarshalCredentialW(long ptr ptr) advapi32.CredMarshalCredentialW
@ stub CredParseUserNameWithType
@ stub CredProfileLoaded
@ stub CredProfileLoadedEx
@ stub CredProfileUnloaded
@ stub CredProtectA
@ stub CredProtectW
@ stdcall CredReadA(str long long ptr) advapi32.CredReadA
@ stub CredReadByTokenHandle
@ stdcall CredReadDomainCredentialsA(ptr long ptr ptr) advapi32.CredReadDomainCredentialsA
@ stdcall CredReadDomainCredentialsW(ptr long ptr ptr) advapi32.CredReadDomainCredentialsW
@ stdcall CredReadW(wstr long long ptr) advapi32.CredReadW
@ stub CredRestoreCredentials
@ stdcall CredUnmarshalCredentialA(str ptr ptr) advapi32.CredUnmarshalCredentialA
@ stdcall CredUnmarshalCredentialW(wstr ptr ptr) advapi32.CredUnmarshalCredentialW
@ stub CredUnprotectA
@ stub CredUnprotectW
@ stdcall CredWriteA(ptr long) advapi32.CredWriteA
@ stub CredWriteDomainCredentialsA
@ stub CredWriteDomainCredentialsW
@ stdcall CredWriteW(ptr long) advapi32.CredWriteW
@ stub CredpConvertCredential
@ stub CredpConvertOneCredentialSize
@ stub CredpConvertTargetInfo
@ stub CredpDecodeCredential
@ stub CredpEncodeCredential
@ stub CredpEncodeSecret
@ stdcall DeleteService(long)
@ stdcall EnableTraceEx2(int64 ptr long long int64 int64 long ptr)
@ stdcall EnumDependentServicesW(long long ptr long ptr ptr)
@ stdcall EnumServicesStatusExW(long long long long ptr long ptr ptr ptr wstr)
@ stub EnumerateIdentityProviders
@ stub EnumerateTraceGuidsEx
@ stub EtwQueryRealtimeConsumer
@ stub EventAccessControl
@ stub EventAccessQuery
@ stub EventAccessRemove
@ stub FreeTransientObjectSecurityDescriptor
@ stub GetDefaultIdentityProvider
@ stub GetIdentityProviderInfoByGUID
@ stub GetIdentityProviderInfoByName
@ stub GetServiceDirectory
@ stdcall GetServiceDisplayNameW(ptr wstr ptr ptr)
@ stdcall GetServiceKeyNameW(long wstr ptr ptr)
@ stub GetServiceRegistryStateKey
@ stub I_QueryTagInformation
@ stub I_RegisterSvchostNotificationCallback
@ stub I_ScBroadcastServiceControlMessage
@ stub I_ScIsSecurityProcess
@ stub I_ScPnPGetServiceName
@ stub I_ScQueryServiceConfig
@ stdcall I_ScRegisterDeviceNotification(ptr ptr long)
@ stub I_ScRegisterPreshutdownRestart
@ stub I_ScReparseServiceDatabase
@ stub I_ScRpcBindA
@ stub I_ScRpcBindW
@ stub I_ScSendPnPMessage
@ stub I_ScSendTSMessage
@ stdcall I_ScUnregisterDeviceNotification(ptr)
@ stub I_ScValidatePnPService
@ stub LocalGetConditionForString
@ stub LocalGetReferencedTokenTypesForCondition
@ stub LocalGetStringForCondition
@ stub LocalRpcBindingCreateWithSecurity
@ stub LocalRpcBindingSetAuthInfoEx
@ stub LookupAccountNameLocalA
@ stub LookupAccountNameLocalW
@ stdcall LookupAccountSidLocalA(ptr ptr ptr ptr ptr ptr) advapi32.LookupAccountSidLocalA
@ stdcall LookupAccountSidLocalW(ptr ptr ptr ptr ptr ptr) advapi32.LookupAccountSidLocalW
@ stdcall LsaAddAccountRights(ptr ptr ptr long) advapi32.LsaAddAccountRights
@ stdcall LsaClose(ptr) advapi32.LsaClose
@ stub LsaCreateSecret
@ stub LsaDelete
@ stdcall LsaEnumerateAccountRights(ptr ptr ptr ptr) advapi32.LsaEnumerateAccountRights
@ stdcall LsaEnumerateAccountsWithUserRight(ptr ptr ptr ptr) advapi32.LsaEnumerateAccountsWithUserRight
@ stdcall LsaFreeMemory(ptr) advapi32.LsaFreeMemory
@ stub LsaICLookupNames
@ stub LsaICLookupNamesWithCreds
@ stub LsaICLookupSids
@ stub LsaICLookupSidsWithCreds
@ stub LsaLookupClose
@ stub LsaLookupFreeMemory
@ stub LsaLookupGetDomainInfo
@ stub LsaLookupManageSidNameMapping
@ stdcall LsaLookupNames2(ptr long long ptr ptr ptr) advapi32.LsaLookupNames2
@ stub LsaLookupOpenLocalPolicy
@ stdcall LsaLookupSids(ptr long ptr ptr ptr) advapi32.LsaLookupSids
@ stub LsaLookupSids2
@ stub LsaLookupTranslateNames
@ stub LsaLookupTranslateSids
@ stub LsaLookupUserAccountType
@ stdcall LsaOpenPolicy(long ptr long long) advapi32.LsaOpenPolicy
@ stub LsaOpenSecret
@ stdcall LsaQueryInformationPolicy(ptr long ptr) advapi32.LsaQueryInformationPolicy
@ stub LsaQuerySecret
@ stdcall LsaRemoveAccountRights(ptr ptr long ptr long) advapi32.LsaRemoveAccountRights
@ stdcall LsaRetrievePrivateData(ptr ptr ptr) advapi32.LsaRetrievePrivateData
@ stdcall LsaSetInformationPolicy(long long ptr) advapi32.LsaSetInformationPolicy
@ stdcall LsaSetSecret(ptr ptr ptr) advapi32.LsaSetSecret
@ stdcall LsaStorePrivateData(ptr ptr ptr) advapi32.LsaStorePrivateData
@ stub NotifyServiceStatusChange
@ stub NotifyServiceStatusChangeA
@ stdcall NotifyServiceStatusChangeW(ptr long ptr)
@ stdcall OpenSCManagerA(str str long)
@ stdcall OpenSCManagerW(wstr wstr long)
@ stdcall OpenServiceA(long str long)
@ stdcall OpenServiceW(long wstr long)
@ stdcall -ret64 OpenTraceW(ptr)
@ stdcall ProcessTrace(ptr long ptr ptr)
@ stdcall QueryAllTracesA(ptr long ptr)
@ stdcall QueryAllTracesW(ptr long ptr)
@ stub QueryLocalUserServiceName
@ stdcall QueryServiceConfig2A(long long ptr long ptr)
@ stdcall QueryServiceConfig2W(long long ptr long ptr)
@ stdcall QueryServiceConfigA(long ptr long ptr)
@ stdcall QueryServiceConfigW(long ptr long ptr)
@ stub QueryServiceDynamicInformation
@ stdcall QueryServiceObjectSecurity(long long ptr long ptr)
@ stdcall QueryServiceStatus(long ptr)
@ stdcall QueryServiceStatusEx(long long ptr long ptr)
@ stub QueryTraceProcessingHandle
@ stub QueryTransientObjectSecurityDescriptor
@ stub QueryUserServiceName
@ stub QueryUserServiceNameForContext
@ stdcall RegisterServiceCtrlHandlerA(str ptr)
@ stdcall RegisterServiceCtrlHandlerExA(str ptr ptr)
@ stdcall RegisterServiceCtrlHandlerExW(wstr ptr ptr)
@ stdcall RegisterServiceCtrlHandlerW(wstr ptr)
@ stdcall RegisterTraceGuidsA(ptr ptr ptr long ptr str str ptr) advapi32.RegisterTraceGuidsA
@ stub ReleaseIdentityProviderEnumContext
@ stub RemoveTraceCallback
@ stub RpcClientCapabilityCheck
@ stub SetLocalRpcServerInterfaceSecurity
@ stub SetLocalRpcServerProtseqSecurity
@ stdcall SetServiceObjectSecurity(long long ptr)
@ stdcall SetServiceStatus(long ptr)
@ stub SetTraceCallback
@ stdcall StartServiceA(long long ptr)
@ stdcall StartServiceCtrlDispatcherA(ptr)
@ stdcall StartServiceCtrlDispatcherW(ptr)
@ stdcall StartServiceW(long long ptr)
@ stdcall StartTraceA(ptr str ptr)
@ stdcall StartTraceW(ptr wstr ptr)
@ stdcall StopTraceW(int64 wstr ptr)
@ stub SubscribeServiceChangeNotifications
@ stub TraceQueryInformation
@ stdcall TraceSetInformation(int64 long ptr long)
@ stub UnsubscribeServiceChangeNotifications
@ stub WaitServiceState
