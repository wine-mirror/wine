name	advapi32
type	win32

@ stub AbortSystemShutdownA
@ stub AbortSystemShutdownW
@ stdcall AccessCheck(ptr long long ptr ptr ptr ptr ptr) AccessCheck
@ stub AccessCheckAndAuditAlarmA
@ stub AccessCheckAndAuditAlarmW
@ stub AddAccessAllowedAce
@ stub AddAccessDeniedAce
@ stub AddAce
@ stub AddAuditAccessAce
@ stub AdjustTokenGroups
@ stdcall AdjustTokenPrivileges(long long ptr long ptr ptr) AdjustTokenPrivileges
@ stdcall AllocateAndInitializeSid(ptr long long long long long long long long long ptr) AllocateAndInitializeSid
@ stdcall AllocateLocallyUniqueId(ptr) AllocateLocallyUniqueId
@ stub AreAllAccessesGranted
@ stub AreAnyAccessesGranted
@ stdcall BackupEventLogA (long str) BackupEventLogA
@ stdcall BackupEventLogW (long wstr) BackupEventLogW
@ stub ChangeServiceConfigA
@ stub ChangeServiceConfigW
@ stdcall ClearEventLogA (long str) ClearEventLogA
@ stdcall ClearEventLogW (long wstr) ClearEventLogW
@ stdcall CloseEventLog (long) CloseEventLog
@ stdcall CloseServiceHandle(long) CloseServiceHandle
@ stdcall ControlService(long long ptr) ControlService
@ stdcall CopySid(long ptr ptr) CopySid
@ stub CreatePrivateObjectSecurity
@ stub CreateProcessAsUserA
@ stub CreateProcessAsUserW
@ stdcall CreateServiceA(long ptr ptr long long long long ptr ptr ptr ptr ptr ptr) CreateServiceA
@ stdcall CreateServiceW (long ptr ptr long long long long ptr ptr ptr ptr ptr ptr) CreateServiceW
@ stdcall CryptAcquireContextA(ptr str str long long) CryptAcquireContextA
@ stub CryptAcquireContextW
@ stub CryptContextAddRef
@ stub CryptCreateHash
@ stub CryptDecrypt
@ stub CryptDeriveKey
@ stub CryptDestroyHash
@ stub CryptDestroyKey
@ stub CryptDuplicateKey
@ stub CryptDuplicateHash
@ stub CryptEncrypt
@ stub CryptEnumProvidersA
@ stub CryptEnumProvidersW
@ stub CryptEnumProviderTypesA
@ stub CryptEnumProviderTypesW
@ stub CryptExportKey
@ stub CryptGenKey
@ stub CryptGetKeyParam
@ stub CryptGetHashParam
@ stub CryptGetProvParam
@ stub CryptGenRandom
@ stub CryptGetDefaultProviderA
@ stub CryptGetDefaultProviderW
@ stub CryptGetUserKey
@ stub CryptHashData
@ stub CryptHashSessionKey
@ stub CryptImportKey
@ stub CryptReleaseContext
@ stub CryptSetHashParam
@ stdcall CryptSetKeyParam(long long ptr long) CryptSetKeyParam
@ stub CryptSetProvParam
@ stub CryptSignHashA
@ stub CryptSignHashW
@ stub CryptSetProviderA
@ stub CryptSetProviderW
@ stub CryptSetProviderExA
@ stub CryptSetProviderExW
@ stub CryptVerifySignatureA
@ stub CryptVerifySignatureW
@ stub DeleteAce
@ stdcall DeleteService(long) DeleteService
@ stdcall DeregisterEventSource(long) DeregisterEventSource
@ stub DestroyPrivateObjectSecurity
@ stub DuplicateToken
@ stub EnumDependentServicesA
@ stub EnumDependentServicesW
@ stdcall EnumServicesStatusA (long long long ptr long ptr ptr ptr) EnumServicesStatusA
@ stdcall EnumServicesStatusW (long long long ptr long ptr ptr ptr) EnumServicesStatusA
@ stdcall EqualPrefixSid(ptr ptr) EqualPrefixSid
@ stdcall EqualSid(ptr ptr) EqualSid
@ stub FindFirstFreeAce
@ stdcall FreeSid(ptr) FreeSid
@ stub GetAce
@ stub GetAclInformation
@ stdcall GetFileSecurityA(str long ptr long ptr) GetFileSecurityA
@ stdcall GetFileSecurityW(wstr long ptr long ptr) GetFileSecurityW
@ stub GetKernelObjectSecurity
@ stdcall GetLengthSid(ptr) GetLengthSid
@ stdcall GetNumberOfEventLogRecords (long ptr) GetNumberOfEventLogRecords
@ stdcall GetOldestEventLogRecord (long ptr) GetOldestEventLogRecord
@ stub GetPrivateObjectSecurity
@ stdcall GetSecurityDescriptorControl (ptr ptr ptr) GetSecurityDescriptorControl
@ stdcall GetSecurityDescriptorDacl (ptr ptr ptr ptr) GetSecurityDescriptorDacl
@ stdcall GetSecurityDescriptorGroup(ptr ptr ptr) GetSecurityDescriptorGroup
@ stdcall GetSecurityDescriptorLength(ptr) GetSecurityDescriptorLength
@ stdcall GetSecurityDescriptorOwner(ptr ptr ptr) GetSecurityDescriptorOwner
@ stdcall GetSecurityDescriptorSacl (ptr ptr ptr ptr) GetSecurityDescriptorSacl
@ stub GetServiceDisplayNameA
@ stub GetServiceDisplayNameW
@ stub GetServiceKeyNameA
@ stub GetServiceKeyNameW
@ stdcall GetSidIdentifierAuthority(ptr) GetSidIdentifierAuthority
@ stdcall GetSidLengthRequired(long) GetSidLengthRequired
@ stdcall GetSidSubAuthority(ptr long) GetSidSubAuthority
@ stdcall GetSidSubAuthorityCount(ptr) GetSidSubAuthorityCount
@ stdcall GetTokenInformation(long long ptr long ptr) GetTokenInformation
@ stdcall GetUserNameA(ptr ptr) GetUserNameA
@ stdcall GetUserNameW(ptr ptr) GetUserNameW
@ stub ImpersonateLoggedOnUser
@ stub ImpersonateNamedPipeClient
@ stdcall ImpersonateSelf(long) ImpersonateSelf
@ stub InitializeAcl
@ stdcall InitializeSecurityDescriptor(ptr long) InitializeSecurityDescriptor
@ stdcall InitializeSid(ptr ptr long) InitializeSid
@ stub InitiateSystemShutdownA
@ stub InitiateSystemShutdownW
@ stdcall IsTextUnicode(ptr long ptr) RtlIsTextUnicode
@ stub IsValidAcl
@ stdcall IsValidSecurityDescriptor(ptr) IsValidSecurityDescriptor
@ stdcall IsValidSid(ptr) IsValidSid
@ stub LockServiceDatabase
@ stub LogonUserA
@ stub LogonUserW
@ stub LookupAccountNameA
@ stub LookupAccountNameW
@ stdcall LookupAccountSidA(ptr ptr ptr ptr ptr ptr ptr) LookupAccountSidA
@ stdcall LookupAccountSidW(ptr ptr ptr ptr ptr ptr ptr) LookupAccountSidW
@ stub LookupPrivilegeDisplayNameA
@ stub LookupPrivilegeDisplayNameW
@ stub LookupPrivilegeNameA
@ stub LookupPrivilegeNameW
@ stdcall LookupPrivilegeValueA(ptr ptr ptr) LookupPrivilegeValueA
@ stdcall LookupPrivilegeValueW(ptr ptr ptr) LookupPrivilegeValueW
@ stub MakeAbsoluteSD
@ stdcall MakeSelfRelativeSD(ptr ptr ptr) MakeSelfRelativeSD
@ stub MapGenericMask
@ stdcall NotifyBootConfigStatus(long) NotifyBootConfigStatus
@ stdcall NotifyChangeEventLog (long long) NotifyChangeEventLog
@ stub ObjectCloseAuditAlarmA
@ stub ObjectCloseAuditAlarmW
@ stub ObjectOpenAuditAlarmA
@ stub ObjectOpenAuditAlarmW
@ stub ObjectPrivilegeAuditAlarmA
@ stub ObjectPrivilegeAuditAlarmW
@ stdcall OpenBackupEventLogA (str str) OpenBackupEventLogA
@ stdcall OpenBackupEventLogW (wstr wstr) OpenBackupEventLogW
@ stdcall OpenEventLogA (str str) OpenEventLogA
@ stdcall OpenEventLogW (wstr wstr) OpenEventLogW
@ stdcall OpenProcessToken(long long ptr) OpenProcessToken
@ stdcall OpenSCManagerA(ptr ptr long) OpenSCManagerA
@ stdcall OpenSCManagerW(ptr ptr long) OpenSCManagerW
@ stdcall OpenServiceA(long str long) OpenServiceA
@ stdcall OpenServiceW(long wstr long) OpenServiceW
@ stdcall OpenThreadToken(long long long ptr) OpenThreadToken
@ stub PrivilegeCheck
@ stub PrivilegedServiceAuditAlarmA
@ stub PrivilegedServiceAuditAlarmW
@ stub QueryServiceConfigA
@ stub QueryServiceConfigW
@ stub QueryServiceLockStatusA
@ stub QueryServiceLockStatusW
@ stub QueryServiceObjectSecurity
@ stdcall QueryServiceStatus(long ptr) QueryServiceStatus
@ stdcall ReadEventLogA (long long long ptr long ptr ptr) ReadEventLogA
@ stdcall ReadEventLogW (long long long ptr long ptr ptr) ReadEventLogW
@ stdcall RegCloseKey(long) RegCloseKey
@ stdcall RegConnectRegistryA(str long ptr) RegConnectRegistryA
@ stdcall RegConnectRegistryW(wstr long ptr) RegConnectRegistryW
@ stdcall RegCreateKeyA(long str ptr) RegCreateKeyA
@ stdcall RegCreateKeyExA(long str long ptr long long ptr ptr ptr) RegCreateKeyExA
@ stdcall RegCreateKeyExW(long wstr long ptr long long ptr ptr ptr) RegCreateKeyExW
@ stdcall RegCreateKeyW(long wstr ptr) RegCreateKeyW
@ stdcall RegDeleteKeyA(long str) RegDeleteKeyA
@ stdcall RegDeleteKeyW(long wstr) RegDeleteKeyW
@ stdcall RegDeleteValueA(long str) RegDeleteValueA
@ stdcall RegDeleteValueW(long wstr) RegDeleteValueW
@ stdcall RegEnumKeyA(long long ptr long) RegEnumKeyA
@ stdcall RegEnumKeyExA(long long ptr ptr ptr ptr ptr ptr) RegEnumKeyExA
@ stdcall RegEnumKeyExW(long long ptr ptr ptr ptr ptr ptr) RegEnumKeyExW
@ stdcall RegEnumKeyW(long long ptr long) RegEnumKeyW
@ stdcall RegEnumValueA(long long ptr ptr ptr ptr ptr ptr) RegEnumValueA
@ stdcall RegEnumValueW(long long ptr ptr ptr ptr ptr ptr) RegEnumValueW
@ stdcall RegFlushKey(long) RegFlushKey
@ stdcall RegGetKeySecurity(long long ptr ptr) RegGetKeySecurity
@ stdcall RegLoadKeyA(long str str) RegLoadKeyA
@ stdcall RegLoadKeyW(long wstr wstr) RegLoadKeyW
@ stdcall RegNotifyChangeKeyValue(long long long long long) RegNotifyChangeKeyValue
@ stdcall RegOpenKeyA(long str ptr) RegOpenKeyA
@ stdcall RegOpenKeyExA(long str long long ptr) RegOpenKeyExA
@ stdcall RegOpenKeyExW(long wstr long long ptr) RegOpenKeyExW
@ stdcall RegOpenKeyW(long wstr ptr) RegOpenKeyW
@ stdcall RegQueryInfoKeyA(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) RegQueryInfoKeyA
@ stdcall RegQueryInfoKeyW(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) RegQueryInfoKeyW
@ stub RegQueryMultipleValuesA
@ stub RegQueryMultipleValuesW
@ stdcall RegQueryValueA(long str ptr ptr) RegQueryValueA
@ stdcall RegQueryValueExA(long str ptr ptr ptr ptr) RegQueryValueExA
@ stdcall RegQueryValueExW(long wstr ptr ptr ptr ptr) RegQueryValueExW
@ stdcall RegQueryValueW(long wstr ptr ptr) RegQueryValueW
@ stub RegRemapPreDefKey
@ stdcall RegReplaceKeyA(long str str str) RegReplaceKeyA
@ stdcall RegReplaceKeyW(long wstr wstr wstr) RegReplaceKeyW
@ stdcall RegRestoreKeyA(long str long) RegRestoreKeyA
@ stdcall RegRestoreKeyW(long wstr long) RegRestoreKeyW
@ stdcall RegSaveKeyA(long ptr ptr) RegSaveKeyA
@ stdcall RegSaveKeyW(long ptr ptr) RegSaveKeyW
@ stdcall RegSetKeySecurity(long long ptr) RegSetKeySecurity
@ stdcall RegSetValueA(long str long ptr long) RegSetValueA
@ stdcall RegSetValueExA(long str long long ptr long) RegSetValueExA
@ stdcall RegSetValueExW(long wstr long long ptr long) RegSetValueExW
@ stdcall RegSetValueW(long wstr long ptr long) RegSetValueW
@ stdcall RegUnLoadKeyA(long str) RegUnLoadKeyA
@ stdcall RegUnLoadKeyW(long wstr) RegUnLoadKeyW
@ stdcall RegisterEventSourceA(ptr ptr) RegisterEventSourceA
@ stdcall RegisterEventSourceW(ptr ptr) RegisterEventSourceW
@ stdcall RegisterServiceCtrlHandlerA (ptr ptr) RegisterServiceCtrlHandlerA
@ stdcall RegisterServiceCtrlHandlerW (ptr ptr) RegisterServiceCtrlHandlerW
@ stdcall ReportEventA (long long long long ptr long long str ptr) ReportEventA
@ stdcall ReportEventW (long long long long ptr long long wstr ptr) ReportEventW
@ stdcall RevertToSelf() RevertToSelf
@ stub SetAclInformation
@ stdcall SetFileSecurityA(str long ptr ) SetFileSecurityA
@ stdcall SetFileSecurityW(wstr long ptr) SetFileSecurityW
@ stub SetKernelObjectSecurity
@ stub SetPrivateObjectSecurity
@ stdcall SetSecurityDescriptorDacl(ptr long ptr long) SetSecurityDescriptorDacl
@ stdcall SetSecurityDescriptorGroup (ptr ptr long) SetSecurityDescriptorGroup
@ stdcall SetSecurityDescriptorOwner (ptr ptr long) SetSecurityDescriptorOwner
@ stdcall SetSecurityDescriptorSacl(ptr long ptr long) SetSecurityDescriptorSacl
@ stub SetServiceBits
@ stub SetServiceObjectSecurity
@ stdcall SetServiceStatus(long long)SetServiceStatus
@ stdcall SetThreadToken (ptr ptr) SetThreadToken
@ stub SetTokenInformation
@ stdcall StartServiceA(long long ptr) StartServiceA
@ stdcall StartServiceCtrlDispatcherA(ptr) StartServiceCtrlDispatcherA
@ stdcall StartServiceCtrlDispatcherW(ptr) StartServiceCtrlDispatcherW
@ stdcall StartServiceW(long long ptr) StartServiceW
@ stub UnlockServiceDatabase
@ stdcall LsaOpenPolicy(long long long long) LsaOpenPolicy
@ stub LsaLookupSids
@ stdcall LsaFreeMemory(ptr)LsaFreeMemory
@ stdcall LsaQueryInformationPolicy(ptr long ptr)LsaQueryInformationPolicy
@ stdcall LsaClose(ptr)LsaClose
@ stub LsaSetInformationPolicy
@ stub LsaLookupNames
@ stub SystemFunction001
@ stub SystemFunction002
@ stub SystemFunction003
@ stub SystemFunction004
@ stub SystemFunction005
@ stub SystemFunction006
@ stub SystemFunction007
@ stub SystemFunction008
@ stub SystemFunction009
@ stub SystemFunction010
@ stub SystemFunction011
@ stub SystemFunction012
@ stub SystemFunction013
@ stub SystemFunction014
@ stub SystemFunction015
@ stub SystemFunction016
@ stub SystemFunction017
@ stub SystemFunction018
@ stub SystemFunction019
@ stub SystemFunction020
@ stub SystemFunction021
@ stub SystemFunction022
@ stub SystemFunction023
@ stub SystemFunction024
@ stub SystemFunction025
@ stub SystemFunction026
@ stub SystemFunction027
@ stub SystemFunction028
@ stub SystemFunction029
@ stub SystemFunction030
@ stub LsaQueryInfoTrustedDomain
@ stub LsaQuerySecret
@ stub LsaCreateSecret
@ stub LsaOpenSecret
@ stub LsaCreateTrustedDomain
@ stub LsaOpenTrustedDomain
@ stub LsaSetSecret
@ stub LsaCreateAccount
@ stub LsaAddPrivilegesToAccount
@ stub LsaRemovePrivilegesFromAccount
@ stub LsaDelete
@ stub LsaSetSystemAccessAccount
@ stub LsaEnumeratePrivilegesOfAccount
@ stub LsaEnumerateAccounts
@ stub LsaGetSystemAccessAccount
@ stub LsaSetInformationTrustedDomain
@ stub LsaEnumerateTrustedDomains
@ stub LsaOpenAccount
@ stub LsaEnumeratePrivileges
@ stub LsaLookupPrivilegeDisplayName
@ stub LsaICLookupNames
@ stub ElfRegisterEventSourceW
@ stub ElfReportEventW
@ stub ElfDeregisterEventSource
@ stub ElfDeregisterEventSourceW
@ stub I_ScSetServiceBit
@ stdcall SynchronizeWindows31FilesAndWindowsNTRegistry(long long long long) SynchronizeWindows31FilesAndWindowsNTRegistry
@ stdcall QueryWindows31FilesMigration(long) QueryWindows31FilesMigration
@ stub LsaICLookupSids
@ stub SystemFunction031
@ stub I_ScSetServiceBitsA
@ stub EnumServiceGroupA
@ stub EnumServiceGroupW
