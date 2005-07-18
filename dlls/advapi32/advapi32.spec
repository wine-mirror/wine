@ stdcall A_SHAFinal(ptr ptr)
@ stdcall A_SHAInit(ptr)
@ stdcall A_SHAUpdate(ptr ptr long)
@ stdcall AbortSystemShutdownA(ptr)
@ stdcall AbortSystemShutdownW(ptr)
@ stdcall AccessCheck(ptr long long ptr ptr ptr ptr ptr)
@ stdcall AccessCheckAndAuditAlarmA(str ptr str str ptr long ptr long ptr ptr ptr)
@ stdcall AccessCheckAndAuditAlarmW(wstr ptr wstr wstr ptr long ptr long ptr ptr ptr)
@ stdcall AccessCheckByType(ptr ptr long long ptr long ptr ptr ptr ptr ptr)
@ stdcall AddAccessAllowedAce (ptr long long ptr)
@ stdcall AddAccessAllowedAceEx (ptr long long long ptr)
@ stdcall AddAccessDeniedAce(ptr long long ptr)
@ stdcall AddAccessDeniedAceEx(ptr long long long ptr)
@ stdcall AddAce(ptr long long ptr long)
@ stdcall AddAuditAccessAce(ptr long long ptr long long)
@ stdcall AdjustTokenGroups(long long ptr long ptr ptr)
@ stdcall AdjustTokenPrivileges(long long ptr long ptr ptr)
@ stdcall AllocateAndInitializeSid(ptr long long long long long long long long long ptr)
@ stdcall AllocateLocallyUniqueId(ptr)
@ stdcall AreAllAccessesGranted(long long)
@ stdcall AreAnyAccessesGranted(long long)
@ stdcall BackupEventLogA (long str)
@ stdcall BackupEventLogW (long wstr)
@ stdcall BuildExplicitAccessWithNameA(ptr str long long long)
@ stdcall BuildExplicitAccessWithNameW(ptr wstr long long long)
@ stdcall BuildSecurityDescriptorA(ptr ptr long ptr long ptr ptr ptr ptr)
@ stdcall BuildSecurityDescriptorW(ptr ptr long ptr long ptr ptr ptr ptr)
@ stdcall BuildTrusteeWithNameA(ptr str)
@ stdcall BuildTrusteeWithNameW(ptr wstr)
@ stdcall BuildTrusteeWithObjectsAndNameA(ptr ptr long str str str)
@ stdcall BuildTrusteeWithObjectsAndNameW(ptr ptr long wstr wstr wstr)
@ stdcall BuildTrusteeWithObjectsAndSidA(ptr ptr ptr ptr ptr)
@ stdcall BuildTrusteeWithObjectsAndSidW(ptr ptr ptr ptr ptr)
@ stdcall BuildTrusteeWithSidA(ptr ptr)
@ stdcall BuildTrusteeWithSidW(ptr ptr)
@ stdcall ChangeServiceConfig2A(long long ptr)
@ stdcall ChangeServiceConfig2W(long long ptr)
@ stdcall ChangeServiceConfigA(long long long long wstr str ptr str str str str)
@ stdcall ChangeServiceConfigW(long long long long wstr wstr ptr wstr wstr wstr wstr)
@ stdcall CheckTokenMembership(long ptr ptr)
@ stdcall ClearEventLogA (long str)
@ stdcall ClearEventLogW (long wstr)
@ stdcall CloseEventLog (long)
@ stdcall CloseServiceHandle(long)
@ stdcall CommandLineFromMsiDescriptor(wstr wstr ptr)
@ stdcall ControlService(long long ptr)
@ stdcall ConvertSidToStringSidA(ptr ptr)
@ stdcall ConvertSidToStringSidW(ptr ptr)
@ stdcall ConvertStringSecurityDescriptorToSecurityDescriptorA(str long ptr ptr)
@ stdcall ConvertStringSecurityDescriptorToSecurityDescriptorW(wstr long ptr ptr)
@ stdcall ConvertStringSidToSidA(ptr ptr)
@ stdcall ConvertStringSidToSidW(ptr ptr)
@ stdcall CopySid(long ptr ptr)
@ stdcall CreatePrivateObjectSecurity(ptr ptr ptr long long ptr)
@ stdcall CreateProcessAsUserA(long str str ptr ptr long long ptr str ptr ptr)
@ stdcall CreateProcessAsUserW(long str str ptr ptr long long ptr str ptr ptr)
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
@ stdcall CryptEnumProviderTypesA(long ptr long ptr ptr ptr)
@ stdcall CryptEnumProviderTypesW(long ptr long ptr ptr ptr)
@ stdcall CryptEnumProvidersA(long ptr long ptr ptr ptr)
@ stdcall CryptEnumProvidersW(long ptr long ptr ptr ptr)
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
@ stdcall CryptSetHashParam(long long ptr long)
@ stdcall CryptSetKeyParam(long long ptr long)
@ stdcall CryptSetProvParam(long long ptr long)
@ stdcall CryptSetProviderA(str long)
@ stdcall CryptSetProviderExA(str long ptr long)
@ stdcall CryptSetProviderExW(wstr long ptr long)
@ stdcall CryptSetProviderW(wstr long)
@ stdcall CryptSignHashA(long long ptr long ptr ptr)
@ stdcall CryptSignHashW(long long ptr long ptr ptr)
@ stdcall CryptVerifySignatureA(long ptr long long ptr long)
@ stdcall CryptVerifySignatureW(long ptr long long ptr long)
@ stdcall DecryptFileA(str long)
@ stdcall DecryptFileW(wstr long)
@ stdcall DeleteAce(ptr long)
@ stdcall DeleteService(long)
@ stdcall DeregisterEventSource(long)
@ stdcall DestroyPrivateObjectSecurity(ptr)
@ stdcall DuplicateToken(long long ptr)
@ stdcall DuplicateTokenEx(long long ptr long long ptr)
@ stub ElfDeregisterEventSource
@ stub ElfDeregisterEventSourceW
@ stub ElfRegisterEventSourceW
@ stub ElfReportEventW
@ stdcall EncryptFileA(str)
@ stdcall EncryptFileW(wstr)
@ stdcall EnumDependentServicesA(long long ptr long ptr ptr)
@ stdcall EnumDependentServicesW(long long ptr long ptr ptr)
@ stub EnumServiceGroupA
@ stub EnumServiceGroupW
@ stdcall EnumServicesStatusA (long long long ptr long ptr ptr ptr)
@ stdcall EnumServicesStatusW (long long long ptr long ptr ptr ptr)
@ stdcall EqualPrefixSid(ptr ptr)
@ stdcall EqualSid(ptr ptr)
@ stdcall FindFirstFreeAce(ptr ptr)
@ stdcall FreeSid(ptr)
@ stdcall GetAce(ptr long ptr)
@ stdcall GetAclInformation(ptr ptr long long)
@ stdcall GetCurrentHwProfileA(ptr)
@ stdcall GetCurrentHwProfileW(ptr)
@ stdcall GetEffectiveRightsFromAclA(ptr ptr ptr)
@ stdcall GetExplicitEntriesFromAclA(ptr ptr ptr)
@ stdcall GetExplicitEntriesFromAclW(ptr ptr ptr)
@ stdcall GetFileSecurityA(str long ptr long ptr)
@ stdcall GetFileSecurityW(wstr long ptr long ptr)
@ stdcall GetKernelObjectSecurity(long long ptr long ptr)
@ stdcall GetLengthSid(ptr)
@ stub GetMangledSiteSid
@ stdcall GetNamedSecurityInfoA (str long long ptr ptr ptr ptr ptr)
@ stdcall GetNamedSecurityInfoW (wstr long long ptr ptr ptr ptr ptr)
@ stdcall GetNumberOfEventLogRecords (long ptr)
@ stdcall GetOldestEventLogRecord (long ptr)
@ stdcall GetPrivateObjectSecurity(ptr long ptr long ptr)
@ stdcall GetSecurityDescriptorControl (ptr ptr ptr)
@ stdcall GetSecurityDescriptorDacl (ptr ptr ptr ptr)
@ stdcall GetSecurityDescriptorGroup(ptr ptr ptr)
@ stdcall GetSecurityDescriptorLength(ptr)
@ stdcall GetSecurityDescriptorOwner(ptr ptr ptr)
@ stdcall GetSecurityDescriptorSacl (ptr ptr ptr ptr)
@ stdcall GetSecurityInfo (long long long ptr ptr ptr ptr ptr)
@ stdcall GetSecurityInfoExW (long long long wstr wstr ptr ptr wstr wstr)
@ stdcall GetServiceDisplayNameA(ptr str ptr ptr)
@ stdcall GetServiceDisplayNameW(ptr wstr ptr ptr)
@ stdcall GetServiceKeyNameA(long str ptr ptr)
@ stdcall GetServiceKeyNameW(long wstr ptr ptr)
@ stdcall GetSidIdentifierAuthority(ptr)
@ stdcall GetSidLengthRequired(long)
@ stdcall GetSidSubAuthority(ptr long)
@ stdcall GetSidSubAuthorityCount(ptr)
@ stub GetSiteSidFromToken
@ stdcall GetTokenInformation(long long ptr long ptr)
@ stdcall GetTrusteeFormA(ptr) 
@ stdcall GetTrusteeFormW(ptr) 
@ stdcall GetTrusteeNameA(ptr) 
@ stdcall GetTrusteeNameW(ptr) 
@ stdcall GetTrusteeTypeA(ptr) 
@ stdcall GetTrusteeTypeW(ptr) 
@ stdcall GetUserNameA(ptr ptr)
@ stdcall GetUserNameW(ptr ptr)
@ stub I_ScSetServiceBit
@ stub I_ScSetServiceBitsA
@ stdcall ImpersonateLoggedOnUser(long)
@ stdcall ImpersonateNamedPipeClient(long)
@ stdcall ImpersonateSelf(long)
@ stdcall InitializeAcl(ptr long long)
@ stdcall InitializeSecurityDescriptor(ptr long)
@ stdcall InitializeSid(ptr ptr long)
@ stdcall InitiateSystemShutdownA(str str long long long)
@ stdcall InitiateSystemShutdownExA(str str long long long long)
@ stdcall InitiateSystemShutdownExW(wstr wstr long long long long)
@ stdcall InitiateSystemShutdownW(str str long long long)
@ stub InstallApplication
@ stub IsProcessRestricted
@ stdcall IsTextUnicode(ptr long ptr)
@ stdcall IsTokenRestricted(long)
@ stdcall IsValidAcl(ptr)
@ stdcall IsValidSecurityDescriptor(ptr)
@ stdcall IsValidSid(ptr)
@ stdcall LockServiceDatabase(ptr)
@ stdcall LogonUserA(str str str long long ptr)
@ stdcall LogonUserW(wstr wstr wstr long long ptr)
@ stdcall LookupAccountNameA(str str ptr ptr ptr ptr ptr)
@ stdcall LookupAccountNameW(wstr wstr ptr ptr ptr ptr ptr)
@ stdcall LookupAccountSidA(ptr ptr ptr ptr ptr ptr ptr)
@ stdcall LookupAccountSidW(ptr ptr ptr ptr ptr ptr ptr)
@ stdcall LookupPrivilegeDisplayNameA(str str str ptr ptr)
@ stdcall LookupPrivilegeDisplayNameW(wstr wstr wstr ptr ptr)
@ stdcall LookupPrivilegeNameA(str ptr ptr long)
@ stdcall LookupPrivilegeNameW(wstr ptr ptr long)
@ stdcall LookupPrivilegeValueA(ptr ptr ptr)
@ stdcall LookupPrivilegeValueW(ptr ptr ptr)
@ stub LsaAddPrivilegesToAccount
@ stdcall LsaClose(ptr)
@ stub LsaCreateAccount
@ stub LsaCreateSecret
@ stub LsaCreateTrustedDomain
@ stub LsaDelete
@ stub LsaEnumerateAccounts
@ stub LsaEnumeratePrivileges
@ stub LsaEnumeratePrivilegesOfAccount
@ stdcall LsaEnumerateTrustedDomains(long ptr ptr long ptr)
@ stdcall LsaFreeMemory(ptr)
@ stub LsaGetSystemAccessAccount
@ stub LsaICLookupNames
@ stub LsaICLookupSids
@ stdcall LsaLookupNames(long long ptr ptr ptr)
@ stub LsaLookupPrivilegeDisplayName
@ stdcall LsaLookupSids(ptr long ptr ptr ptr)
@ stdcall LsaNtStatusToWinError(long)
@ stub LsaOpenAccount
@ stdcall LsaOpenPolicy(long long long long)
@ stub LsaOpenSecret
@ stub LsaOpenTrustedDomain
@ stub LsaQueryInfoTrustedDomain
@ stdcall LsaQueryInformationPolicy(ptr long ptr)
@ stub LsaQuerySecret
@ stub LsaRemovePrivilegesFromAccount
@ stdcall LsaRetrievePrivateData(ptr ptr ptr)
@ stdcall LsaSetInformationPolicy(long long ptr)
@ stub LsaSetInformationTrustedDomain
@ stub LsaSetSecret
@ stub LsaSetSystemAccessAccount
@ stdcall LsaStorePrivateData(ptr ptr ptr)
@ stdcall MD4Final(ptr)
@ stdcall MD4Init(ptr)
@ stdcall MD4Update(ptr ptr long)
@ stdcall MD5Final(ptr)
@ stdcall MD5Init(ptr)
@ stdcall MD5Update(ptr ptr long)
@ stdcall MakeAbsoluteSD(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall MakeSelfRelativeSD(ptr ptr ptr)
@ stdcall MapGenericMask(ptr ptr)
@ stdcall NotifyBootConfigStatus(long)
@ stdcall NotifyChangeEventLog (long long)
@ stdcall ObjectCloseAuditAlarmA(str ptr long)
@ stdcall ObjectCloseAuditAlarmW(wstr ptr long)
@ stdcall ObjectOpenAuditAlarmA(str ptr str str ptr long long long ptr long long ptr)
@ stdcall ObjectOpenAuditAlarmW(wstr ptr wstr wstr ptr long long long ptr long long ptr)
@ stdcall ObjectPrivilegeAuditAlarmA(str ptr long long ptr long)
@ stdcall ObjectPrivilegeAuditAlarmW(wstr ptr long long ptr long)
@ stdcall OpenBackupEventLogA (str str)
@ stdcall OpenBackupEventLogW (wstr wstr)
@ stdcall OpenEventLogA (str str)
@ stdcall OpenEventLogW (wstr wstr)
@ stdcall OpenProcessToken(long long ptr)
@ stdcall OpenSCManagerA(str str long)
@ stdcall OpenSCManagerW(wstr wstr long)
@ stdcall OpenServiceA(long str long)
@ stdcall OpenServiceW(long wstr long)
@ stdcall OpenThreadToken(long long long ptr)
@ stdcall PrivilegeCheck(ptr ptr ptr)
@ stdcall PrivilegedServiceAuditAlarmA(str str long ptr long)
@ stdcall PrivilegedServiceAuditAlarmW(wstr wstr long ptr long)
@ stdcall QueryServiceConfigA(long ptr long ptr)
@ stdcall QueryServiceConfigW(long ptr long ptr)
@ stdcall QueryServiceLockStatusA(long ptr long ptr)
@ stdcall QueryServiceLockStatusW(long ptr long ptr)
@ stdcall QueryServiceObjectSecurity(long long ptr long ptr)
@ stdcall QueryServiceStatus(long ptr)
@ stdcall QueryServiceStatusEx (long long ptr long ptr)
@ stdcall QueryWindows31FilesMigration(long)
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
@ stdcall RegGetValueA(long str str long ptr ptr ptr)
@ stdcall RegGetValueW(long wstr wstr long ptr ptr ptr)
@ stdcall RegLoadKeyA(long str str)
@ stdcall RegLoadKeyW(long wstr wstr)
@ stdcall RegNotifyChangeKeyValue(long long long long long)
@ stdcall RegOpenCurrentUser(long ptr)
@ stdcall RegOpenKeyA(long str ptr)
@ stdcall RegOpenKeyExA(long str long long ptr)
@ stdcall RegOpenKeyExW(long wstr long long ptr)
@ stdcall RegOpenKeyW(long wstr ptr)
@ stdcall RegOpenUserClassesRoot(ptr long long ptr)
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
@ stdcall RegisterTraceGuidsA(ptr ptr ptr long ptr str str ptr)
@ stdcall RegisterTraceGuidsW(ptr ptr ptr long ptr wstr wstr ptr)
@ stdcall ReportEventA (long long long long ptr long long str ptr)
@ stdcall ReportEventW (long long long long ptr long long wstr ptr)
@ stdcall RevertToSelf()
@ stdcall SetAclInformation(ptr ptr long long)
@ stdcall SetEntriesInAclA(long ptr ptr ptr)
@ stdcall SetEntriesInAclW(long ptr ptr ptr)
@ stdcall SetFileSecurityA(str long ptr )
@ stdcall SetFileSecurityW(wstr long ptr)
@ stdcall SetKernelObjectSecurity(long long ptr)
@ stdcall SetNamedSecurityInfoA(str long ptr ptr ptr ptr ptr)
@ stdcall SetNamedSecurityInfoW(wstr long ptr ptr ptr ptr ptr)
@ stdcall SetPrivateObjectSecurity(long ptr ptr ptr long)
@ stdcall SetSecurityDescriptorControl(ptr long long)
@ stdcall SetSecurityDescriptorDacl(ptr long ptr long)
@ stdcall SetSecurityDescriptorGroup (ptr ptr long)
@ stdcall SetSecurityDescriptorOwner (ptr ptr long)
@ stdcall SetSecurityDescriptorSacl(ptr long ptr long)
@ stdcall SetSecurityInfo (long long long ptr ptr ptr ptr)
@ stdcall SetServiceBits(long long long long)
@ stdcall SetServiceObjectSecurity(long long ptr)
@ stdcall SetServiceStatus(long long)
@ stdcall SetThreadToken (ptr ptr)
@ stdcall SetTokenInformation (long long ptr long)
@ stdcall StartServiceA(long long ptr)
@ stdcall StartServiceCtrlDispatcherA(ptr)
@ stdcall StartServiceCtrlDispatcherW(ptr)
@ stdcall StartServiceW(long long ptr)
@ stdcall SynchronizeWindows31FilesAndWindowsNTRegistry(long long long long)
@ stub SystemFunction001
@ stub SystemFunction002
@ stub SystemFunction003
@ stub SystemFunction004
@ stub SystemFunction005
@ stdcall SystemFunction006(ptr ptr)
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
@ stub SystemFunction031
@ stub SystemFunction032
@ stub SystemFunction033
@ stub SystemFunction034
@ stub SystemFunction035
@ stdcall SystemFunction036(ptr long) # RtlGenRandom
@ stdcall SystemFunction040(ptr long long) # RtlEncryptMemory
@ stdcall SystemFunction041(ptr long long) # RtlDecryptMemory
@ stub TraceEvent
@ stub TraceEventInstance
@ stub TraceMessage
@ stub TraceMessageVa
@ stdcall UnlockServiceDatabase (ptr)
@ stub UnregisterTraceGuids
@ stub UpdateTraceA
@ stub UpdateTraceW
@ stub WdmWmiServiceMain
@ stub WmiCloseBlock
@ stub WmiOpenBlock
@ stub WmiQuerySingleInstanceW
@ stub WmiSetSingleInstanceW
