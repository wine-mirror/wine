@ stdcall AbortSystemShutdownA(ptr)
@ stdcall AbortSystemShutdownW(ptr)
@ stdcall AccessCheck(ptr long long ptr ptr ptr ptr ptr)
@ stdcall AccessCheckAndAuditAlarmA(str ptr str str ptr long ptr long ptr ptr ptr)
@ stdcall AccessCheckAndAuditAlarmW(wstr ptr wstr wstr ptr long ptr long ptr ptr ptr)
@ stub AccessCheckByType #(ptr ptr long long ptr long ptr ptr ptr ptr ptr) AccessCheckByType
@ stdcall AddAccessAllowedAce (ptr long long ptr)
@ stdcall AddAccessAllowedAceEx (ptr long long long ptr)
@ stdcall AddAccessDeniedAce(ptr long long ptr)
@ stdcall AddAccessDeniedAceEx(ptr long long long ptr)
@ stdcall AddAce(ptr long long ptr long)
@ stub AddAuditAccessAce
@ stub AdjustTokenGroups
@ stdcall AdjustTokenPrivileges(long long ptr long ptr ptr)
@ stdcall AllocateAndInitializeSid(ptr long long long long long long long long long ptr)
@ stdcall AllocateLocallyUniqueId(ptr)
@ stub AreAllAccessesGranted
@ stub AreAnyAccessesGranted
@ stdcall BackupEventLogA (long str)
@ stdcall BackupEventLogW (long wstr)
@ stdcall BuildTrusteeWithSidA(ptr ptr)
@ stdcall BuildTrusteeWithSidW(ptr ptr)
@ stub ChangeServiceConfigA
@ stub ChangeServiceConfigW
@ stdcall ClearEventLogA (long str)
@ stdcall ClearEventLogW (long wstr)
@ stdcall CloseEventLog (long)
@ stdcall CloseServiceHandle(long)
@ stub CommandLineFromMsiDescriptor
@ stdcall ControlService(long long ptr)
@ stub ConvertSidToStringSidA #(ptr str) ConvertSidToStringSidA
@ stub ConvertSidToStringSidW #(ptr wstr) ConvertSidToStringSidW
@ stub ConvertStringSecurityDescriptorToSecurityDescriptorA #(str long ptr ptr) ConvertStringSecurityDescriptorToSecurityDescriptorA
@ stub ConvertStringSecurityDescriptorToSecurityDescriptorW #(wstr long ptr ptr) ConvertStringSecurityDescriptorToSecurityDescriptorW
@ stdcall CopySid(long ptr ptr)
@ stub CreatePrivateObjectSecurity
@ stub CreateProcessAsUserA
@ stub CreateProcessAsUserW
@ stdcall CreateServiceA(long ptr ptr long long long long ptr ptr ptr ptr ptr ptr)
@ stdcall CreateServiceW (long ptr ptr long long long long ptr ptr ptr ptr ptr ptr)
@ stub CredProfileLoaded
@ stdcall CryptAcquireContextA(ptr str str long long)
@ stdcall CryptAcquireContextW(ptr wstr wstr long long)
@ stdcall CryptContextAddRef(long ptr long)
@ stdcall CryptCreateHash(long long long long ptr)
@ stdcall CryptDecrypt(long long long long ptr ptr)
@ stdcall CryptDeriveKey(long long long long ptr)
@ stdcall CryptDestroyHash(long)
@ stdcall CryptDestroyKey(long)
@ stdcall CryptDuplicateHash(long ptr long ptr)
@ stdcall CryptDuplicateKey(long ptr long ptr)
@ stdcall CryptEncrypt(long long long long ptr ptr long)
@ stdcall CryptEnumProvidersA(long ptr long ptr ptr ptr)
@ stdcall CryptEnumProvidersW(long ptr long ptr ptr ptr)
@ stdcall CryptEnumProviderTypesA(long ptr long ptr ptr ptr)
@ stdcall CryptEnumProviderTypesW(long ptr long ptr ptr ptr)
@ stdcall CryptExportKey(long long long long ptr ptr)
@ stdcall CryptGenKey(long long long ptr)
@ stdcall CryptGenRandom(long long ptr)
@ stdcall CryptGetDefaultProviderA(long ptr long ptr ptr)
@ stdcall CryptGetDefaultProviderW(long ptr long ptr ptr)
@ stdcall CryptGetHashParam(long long ptr ptr long)
@ stdcall CryptGetKeyParam(long long ptr ptr long)
@ stdcall CryptGetProvParam(long long ptr ptr long)
@ stdcall CryptGetUserKey(long long ptr)
@ stdcall CryptHashData(long ptr long long)
@ stdcall CryptHashSessionKey(long long long)
@ stdcall CryptImportKey(long ptr long long long ptr)
@ stdcall CryptReleaseContext(long long)
@ stdcall CryptSignHashA(long long ptr long ptr ptr)
@ stdcall CryptSignHashW(long long ptr long ptr ptr) CryptSignHashA
@ stdcall CryptSetHashParam(long long ptr long)
@ stdcall CryptSetKeyParam(long long ptr long)
@ stdcall CryptSetProviderA(str long)
@ stdcall CryptSetProviderW(wstr long)
@ stdcall CryptSetProviderExA(str long ptr long)
@ stdcall CryptSetProviderExW(wstr long ptr long)
@ stdcall CryptSetProvParam(long long ptr long)
@ stdcall CryptVerifySignatureA(long ptr long long ptr long)
@ stdcall CryptVerifySignatureW(long ptr long long ptr long) CryptVerifySignatureA
@ stub DeleteAce
@ stdcall DeleteService(long)
@ stdcall DeregisterEventSource(long)
@ stub DestroyPrivateObjectSecurity
@ stub DuplicateToken #(long long ptr) DuplicateToken
@ stub DuplicateTokenEx #(long long ptr long long ptr) DuplicateTokenEx
@ stub EnumDependentServicesA
@ stub EnumDependentServicesW
@ stdcall EnumServicesStatusA (long long long ptr long ptr ptr ptr)
@ stdcall EnumServicesStatusW (long long long ptr long ptr ptr ptr)
@ stdcall EqualPrefixSid(ptr ptr)
@ stdcall EqualSid(ptr ptr)
@ stdcall FindFirstFreeAce(ptr ptr)
@ stdcall FreeSid(ptr)
@ stdcall GetAce(ptr long ptr)
@ stdcall GetAclInformation(ptr ptr long long)
@ stdcall GetCurrentHwProfileA(ptr)
@ stub GetEffectiveRightsFromAclA
@ stdcall GetFileSecurityA(str long ptr long ptr)
@ stdcall GetFileSecurityW(wstr long ptr long ptr)
@ stub GetKernelObjectSecurity
@ stdcall GetLengthSid(ptr)
@ stub GetMangledSiteSid
@ stub GetNamedSecurityInfoA #(str long long ptr ptr ptr ptr ptr) GetNamedSecurityInfoA
@ stub GetNamedSecurityInfoW #(wstr long long ptr ptr ptr ptr ptr) GetNamedSecurityInfoW
@ stdcall GetNumberOfEventLogRecords (long ptr)
@ stdcall GetOldestEventLogRecord (long ptr)
@ stub GetPrivateObjectSecurity
@ stdcall GetSecurityDescriptorControl (ptr ptr ptr)
@ stdcall GetSecurityDescriptorDacl (ptr ptr ptr ptr)
@ stdcall GetSecurityDescriptorGroup(ptr ptr ptr)
@ stdcall GetSecurityDescriptorLength(ptr)
@ stdcall GetSecurityDescriptorOwner(ptr ptr ptr)
@ stdcall GetSecurityDescriptorSacl (ptr ptr ptr ptr)
@ stub GetSecurityInfo #(long long long ptr ptr ptr ptr ptr) GetSecurityInfo
@ stdcall GetSecurityInfoExW (long long long wstr wstr ptr ptr wstr wstr)
@ stub GetServiceDisplayNameA
@ stub GetServiceDisplayNameW
@ stub GetServiceKeyNameA
@ stub GetServiceKeyNameW
@ stdcall GetSidIdentifierAuthority(ptr)
@ stdcall GetSidLengthRequired(long)
@ stdcall GetSidSubAuthority(ptr long)
@ stdcall GetSidSubAuthorityCount(ptr)
@ stub GetSiteSidFromToken
@ stdcall GetTokenInformation(long long ptr long ptr)
@ stdcall GetUserNameA(ptr ptr)
@ stdcall GetUserNameW(ptr ptr)
@ stdcall ImpersonateLoggedOnUser(long)
@ stub ImpersonateNamedPipeClient
@ stdcall ImpersonateSelf(long)
@ stdcall InitializeAcl(ptr long long)
@ stdcall InitializeSecurityDescriptor(ptr long)
@ stdcall InitializeSid(ptr ptr long)
@ stub InitiateSystemShutdownA
@ stub InitiateSystemShutdownW
@ stdcall InitiateSystemShutdownExA(str str long long long long)
@ stdcall InitiateSystemShutdownExW(wstr wstr long long long long)
@ stub InstallApplication
@ stub IsProcessRestricted
@ stdcall IsTextUnicode(ptr long ptr) ntdll.RtlIsTextUnicode
@ stub IsTokenRestricted
@ stdcall IsValidAcl(ptr)
@ stdcall IsValidSecurityDescriptor(ptr)
@ stdcall IsValidSid(ptr)
@ stdcall LockServiceDatabase(ptr)
@ stub LogonUserA
@ stub LogonUserW
@ stdcall LookupAccountNameA(str str ptr ptr ptr ptr ptr)
@ stub LookupAccountNameW
@ stdcall LookupAccountSidA(ptr ptr ptr ptr ptr ptr ptr)
@ stdcall LookupAccountSidW(ptr ptr ptr ptr ptr ptr ptr)
@ stub LookupPrivilegeDisplayNameA
@ stub LookupPrivilegeDisplayNameW
@ stdcall LookupPrivilegeNameA(str ptr ptr long)
@ stdcall LookupPrivilegeNameW(wstr ptr ptr long)
@ stdcall LookupPrivilegeValueA(ptr ptr ptr)
@ stdcall LookupPrivilegeValueW(ptr ptr ptr)
@ stub MakeAbsoluteSD
@ stdcall MakeSelfRelativeSD(ptr ptr ptr)
@ stub MapGenericMask
@ stdcall NotifyBootConfigStatus(long)
@ stdcall NotifyChangeEventLog (long long)
@ stub ObjectCloseAuditAlarmA
@ stub ObjectCloseAuditAlarmW
@ stub ObjectOpenAuditAlarmA
@ stub ObjectOpenAuditAlarmW
@ stub ObjectPrivilegeAuditAlarmA
@ stub ObjectPrivilegeAuditAlarmW
@ stdcall OpenBackupEventLogA (str str)
@ stdcall OpenBackupEventLogW (wstr wstr)
@ stdcall OpenEventLogA (str str)
@ stdcall OpenEventLogW (wstr wstr)
@ stdcall OpenProcessToken(long long ptr)
@ stdcall OpenSCManagerA(ptr ptr long)
@ stdcall OpenSCManagerW(ptr ptr long)
@ stdcall OpenServiceA(long str long)
@ stdcall OpenServiceW(long wstr long)
@ stdcall OpenThreadToken(long long long ptr)
@ stdcall PrivilegeCheck(ptr ptr ptr)
@ stub PrivilegedServiceAuditAlarmA
@ stub PrivilegedServiceAuditAlarmW
@ stub QueryServiceConfigA
@ stub QueryServiceConfigW
@ stub QueryServiceLockStatusA
@ stub QueryServiceLockStatusW
@ stub QueryServiceObjectSecurity
@ stdcall QueryServiceStatus(long ptr)
@ stdcall QueryServiceStatusEx (long long ptr long ptr)
@ stdcall ReadEventLogA (long long long ptr long ptr ptr)
@ stdcall ReadEventLogW (long long long ptr long ptr ptr)
@ stdcall RegCloseKey(long)
@ stdcall RegConnectRegistryA(str long ptr)
@ stdcall RegConnectRegistryW(wstr long ptr)
@ stdcall RegCreateKeyA(long str ptr)
@ stdcall RegCreateKeyExA(long str long ptr long long ptr ptr ptr)
@ stdcall RegCreateKeyExW(long wstr long ptr long long ptr ptr ptr)
@ stdcall RegCreateKeyW(long wstr ptr)
@ stdcall RegDeleteKeyA(long str)
@ stdcall RegDeleteKeyW(long wstr)
@ stdcall RegDeleteValueA(long str)
@ stdcall RegDeleteValueW(long wstr)
@ stdcall RegEnumKeyA(long long ptr long)
@ stdcall RegEnumKeyExA(long long ptr ptr ptr ptr ptr ptr)
@ stdcall RegEnumKeyExW(long long ptr ptr ptr ptr ptr ptr)
@ stdcall RegEnumKeyW(long long ptr long)
@ stdcall RegEnumValueA(long long ptr ptr ptr ptr ptr ptr)
@ stdcall RegEnumValueW(long long ptr ptr ptr ptr ptr ptr)
@ stdcall RegFlushKey(long)
@ stdcall RegGetKeySecurity(long long ptr ptr)
@ stdcall RegLoadKeyA(long str str)
@ stdcall RegLoadKeyW(long wstr wstr)
@ stdcall RegNotifyChangeKeyValue(long long long long long)
@ stdcall RegOpenCurrentUser(long ptr)
@ stdcall RegOpenKeyA(long str ptr)
@ stdcall RegOpenKeyExA(long str long long ptr)
@ stdcall RegOpenKeyExW(long wstr long long ptr)
@ stdcall RegOpenKeyW(long wstr ptr)
@ stub RegOpenUserClassesRoot
@ stdcall RegQueryInfoKeyA(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall RegQueryInfoKeyW(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall RegQueryMultipleValuesA(long ptr long ptr ptr)
@ stdcall RegQueryMultipleValuesW(long ptr long ptr ptr)
@ stdcall RegQueryValueA(long str ptr ptr)
@ stdcall RegQueryValueExA(long str ptr ptr ptr ptr)
@ stdcall RegQueryValueExW(long wstr ptr ptr ptr ptr)
@ stdcall RegQueryValueW(long wstr ptr ptr)
@ stub RegRemapPreDefKey
@ stdcall RegReplaceKeyA(long str str str)
@ stdcall RegReplaceKeyW(long wstr wstr wstr)
@ stdcall RegRestoreKeyA(long str long)
@ stdcall RegRestoreKeyW(long wstr long)
@ stdcall RegSaveKeyA(long ptr ptr)
@ stdcall RegSaveKeyW(long ptr ptr)
@ stdcall RegSetKeySecurity(long long ptr)
@ stdcall RegSetValueA(long str long ptr long)
@ stdcall RegSetValueExA(long str long long ptr long)
@ stdcall RegSetValueExW(long wstr long long ptr long)
@ stdcall RegSetValueW(long wstr long ptr long)
@ stdcall RegUnLoadKeyA(long str)
@ stdcall RegUnLoadKeyW(long wstr)
@ stdcall RegisterEventSourceA(ptr ptr)
@ stdcall RegisterEventSourceW(ptr ptr)
@ stdcall RegisterServiceCtrlHandlerA (ptr ptr)
@ stdcall RegisterServiceCtrlHandlerW (ptr ptr)
@ stdcall ReportEventA (long long long long ptr long long str ptr)
@ stdcall ReportEventW (long long long long ptr long long wstr ptr)
@ stdcall RevertToSelf()
@ stub SetAclInformation
@ stdcall SetEntriesInAclA(long ptr ptr ptr)
@ stdcall SetEntriesInAclW(long ptr ptr ptr)
@ stdcall SetFileSecurityA(str long ptr )
@ stdcall SetFileSecurityW(wstr long ptr)
@ stdcall SetKernelObjectSecurity(long long ptr)
@ stdcall SetNamedSecurityInfoA(str long ptr ptr ptr ptr ptr)
@ stdcall SetNamedSecurityInfoW(wstr long ptr ptr ptr ptr ptr)
@ stub SetPrivateObjectSecurity
@ stub SetSecurityDescriptorControl #(ptr long long)
@ stdcall SetSecurityDescriptorDacl(ptr long ptr long)
@ stdcall SetSecurityDescriptorGroup (ptr ptr long)
@ stdcall SetSecurityDescriptorOwner (ptr ptr long)
@ stdcall SetSecurityDescriptorSacl(ptr long ptr long)
@ stub SetServiceBits
@ stub SetServiceObjectSecurity
@ stdcall SetServiceStatus(long long)
@ stdcall SetThreadToken (ptr ptr)
@ stdcall SetTokenInformation (long long ptr long)
@ stdcall StartServiceA(long long ptr)
@ stdcall StartServiceCtrlDispatcherA(ptr)
@ stdcall StartServiceCtrlDispatcherW(ptr)
@ stdcall StartServiceW(long long ptr)
@ stdcall UnlockServiceDatabase (ptr)
@ stdcall LsaOpenPolicy(long long long long)
@ stdcall LsaLookupSids(ptr long ptr ptr ptr)
@ stdcall LsaFreeMemory(ptr)
@ stdcall LsaQueryInformationPolicy(ptr long ptr)
@ stdcall LsaClose(ptr)
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
@ stdcall LsaNtStatusToWinError(long)
@ stub LsaOpenAccount
@ stub LsaEnumeratePrivileges
@ stub LsaLookupPrivilegeDisplayName
@ stub LsaICLookupNames
@ stub ElfRegisterEventSourceW
@ stub ElfReportEventW
@ stub ElfDeregisterEventSource
@ stub ElfDeregisterEventSourceW
@ stub I_ScSetServiceBit
@ stdcall SynchronizeWindows31FilesAndWindowsNTRegistry(long long long long)
@ stdcall QueryWindows31FilesMigration(long)
@ stub LsaICLookupSids
@ stub SystemFunction031
@ stub I_ScSetServiceBitsA
@ stub EnumServiceGroupA
@ stub EnumServiceGroupW
@ stdcall CheckTokenMembership(long ptr ptr)
@ stub WmiQuerySingleInstanceW
@ stub WmiSetSingleInstanceW
@ stub WmiOpenBlock
@ stub WmiCloseBlock
