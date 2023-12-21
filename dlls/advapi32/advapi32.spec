# 1000 stub ADVAPI32_1000
@ stdcall A_SHAFinal(ptr ptr) ntdll.A_SHAFinal
@ stdcall A_SHAInit(ptr) ntdll.A_SHAInit
@ stdcall A_SHAUpdate(ptr ptr long) ntdll.A_SHAUpdate
@ stdcall AbortSystemShutdownA(ptr)
@ stdcall AbortSystemShutdownW(ptr)
@ stdcall -import AccessCheck(ptr long long ptr ptr ptr ptr ptr)
@ stdcall AccessCheckAndAuditAlarmA(str ptr str str ptr long ptr long ptr ptr ptr)
@ stdcall -import AccessCheckAndAuditAlarmW(wstr ptr wstr wstr ptr long ptr long ptr ptr ptr)
@ stdcall -import AccessCheckByType(ptr ptr long long ptr long ptr ptr ptr ptr ptr)
# @ stub AccessCheckByTypeAndAuditAlarmA
# @ stub AccessCheckByTypeAndAuditAlarmW
# @ stub AccessCheckByTypeResultList
# @ stub AccessCheckByTypeResultListAndAuditAlarmA
# @ stub AccessCheckByTypeResultListAndAuditAlarmByHandleA
# @ stub AccessCheckByTypeResultListAndAuditAlarmByHandleW
# @ stub AccessCheckByTypeResultListAndAuditAlarmW
@ stdcall -import AddAccessAllowedAce (ptr long long ptr)
@ stdcall -import AddAccessAllowedAceEx (ptr long long long ptr)
@ stdcall -import AddAccessAllowedObjectAce(ptr long long long ptr ptr ptr)
@ stdcall -import AddAccessDeniedAce(ptr long long ptr)
@ stdcall -import AddAccessDeniedAceEx(ptr long long long ptr)
@ stdcall -import AddAccessDeniedObjectAce(ptr long long long ptr ptr ptr)
@ stdcall -import AddAce(ptr long long ptr long)
@ stdcall -import AddAuditAccessAce(ptr long long ptr long long)
@ stdcall -import AddAuditAccessAceEx(ptr long long long ptr long long)
@ stdcall -import AddAuditAccessObjectAce(ptr long long long ptr ptr ptr long long)
# @ stub AddConditionalAce
@ stdcall -import AddMandatoryAce(ptr long long long ptr)
# @ stub AddUsersToEncryptedFile
# @ stub AddUsersToEncryptedFileEx
@ stdcall -import AdjustTokenGroups(long long ptr long ptr ptr)
@ stdcall -import AdjustTokenPrivileges(long long ptr long ptr ptr)
@ stdcall -import AllocateAndInitializeSid(ptr long long long long long long long long long ptr)
@ stdcall -import AllocateLocallyUniqueId(ptr)
@ stdcall -import AreAllAccessesGranted(long long)
@ stdcall -import AreAnyAccessesGranted(long long)
# @ stub AuditComputeEffectivePolicyBySid
# @ stub AuditComputeEffectivePolicyByToken
# @ stub AuditEnumerateCategories
# @ stub AuditEnumeratePerUserPolicy
# @ stub AuditEnumerateSubCategories
# @ stub AuditFree
# @ stub AuditLookupCategoryGuidFromCategoryId
# @ stub AuditLookupCategoryIdFromCategoryGuid
# @ stub AuditLookupCategoryNameA
# @ stub AuditLookupCategoryNameW
# @ stub AuditLookupSubCategoryNameA
# @ stub AuditLookupSubCategoryNameW
# @ stub AuditQueryGlobalSaclA
# @ stub AuditQueryGlobalSaclW
# @ stub AuditQueryPerUserPolicy
# @ stub AuditQuerySecurity
@ stdcall AuditQuerySystemPolicy(ptr long ptr)
# @ stub AuditSetGlobalSaclA
# @ stub AuditSetGlobalSaclW
# @ stub AuditSetPerUserPolicy
# @ stub AuditSetSecurity
# @ stub AuditSetSystemPolicy
@ stdcall BackupEventLogA (long str)
@ stdcall BackupEventLogW (long wstr)
# @ stub BaseRegCloseKey
# @ stub BaseRegCreateKey
# @ stub BaseRegDeleteKeyEx
# @ stub BaseRegDeleteValue
# @ stub BaseRegFlushKey
# @ stub BaseRegGetVersion
# @ stub BaseRegLoadKey
# @ stub BaseRegOpenKey
# @ stub BaseRegRestoreKey
# @ stub BaseRegSaveKeyEx
# @ stub BaseRegSetKeySecurity
# @ stub BaseRegSetValue
# @ stub BaseRegUnLoadKey
@ stdcall BuildExplicitAccessWithNameA(ptr str long long long)
@ stdcall BuildExplicitAccessWithNameW(ptr wstr long long long)
# @ stub BuildImpersonateExplicitAccessWithNameA
# @ stub BuildImpersonateExplicitAccessWithNameW
# @ stub BuildImpersonateTrusteeA
# @ stub BuildImpersonateTrusteeW
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
# @ stub CancelOverlappedAccess
@ stdcall -import ChangeServiceConfig2A(long long ptr)
@ stdcall -import ChangeServiceConfig2W(long long ptr)
@ stdcall -import ChangeServiceConfigA(long long long long str str ptr str str str str)
@ stdcall -import ChangeServiceConfigW(long long long long wstr wstr ptr wstr wstr wstr wstr)
# @ stub CheckForHiberboot
@ stdcall -import CheckTokenMembership(long ptr ptr)
@ stdcall ClearEventLogA (long str)
@ stdcall ClearEventLogW (long wstr)
# @ stub CloseCodeAuthzLevel
@ stdcall CloseEncryptedFileRaw(ptr)
@ stdcall CloseEventLog (long)
@ stdcall -import CloseServiceHandle(long)
# @ stub CloseThreadWaitChainSession
@ stdcall -import CloseTrace(int64)
@ stdcall CommandLineFromMsiDescriptor(wstr ptr ptr)
# @ stub ComputeAccessTokenFromCodeAuthzLevel
@ stdcall -import ControlService(long long ptr)
# @ stub ControlServiceExA
# @ stub ControlServiceExW
@ stdcall -import ControlTraceA(int64 str ptr long)
@ stdcall -import ControlTraceW(int64 wstr ptr long)
# @ stub ConvertAccessToSecurityDescriptorA
# @ stub ConvertAccessToSecurityDescriptorW
# @ stub ConvertSDToStringSDDomainW
# @ stub ConvertSDToStringSDRootDomainA
# @ stub ConvertSDToStringSDRootDomainW
# @ stub ConvertSecurityDescriptorToAccessA
# @ stub ConvertSecurityDescriptorToAccessNamedA
# @ stub ConvertSecurityDescriptorToAccessNamedW
# @ stub ConvertSecurityDescriptorToAccessW
@ stdcall ConvertSecurityDescriptorToStringSecurityDescriptorA(ptr long long ptr ptr)
@ stdcall -import ConvertSecurityDescriptorToStringSecurityDescriptorW(ptr long long ptr ptr)
@ stdcall ConvertSidToStringSidA(ptr ptr)
@ stdcall -import ConvertSidToStringSidW(ptr ptr)
# @ stub ConvertStringSDToSDDomainA
# @ stub ConvertStringSDToSDDomainW
# @ stub ConvertStringSDToSDRootDomainA
# @ stub ConvertStringSDToSDRootDomainW
@ stdcall ConvertStringSecurityDescriptorToSecurityDescriptorA(str long ptr ptr)
@ stdcall -import ConvertStringSecurityDescriptorToSecurityDescriptorW(wstr long ptr ptr)
@ stdcall ConvertStringSidToSidA(str ptr)
@ stdcall -import ConvertStringSidToSidW(wstr ptr)
@ stdcall -import ConvertToAutoInheritPrivateObjectSecurity(ptr ptr ptr ptr long ptr)
@ stdcall -import CopySid(long ptr ptr)
# @ stub CreateCodeAuthzLevel
@ stdcall -import CreatePrivateObjectSecurity(ptr ptr ptr long long ptr)
@ stdcall -import CreatePrivateObjectSecurityEx(ptr ptr ptr ptr long long long ptr)
@ stdcall -import CreatePrivateObjectSecurityWithMultipleInheritance(ptr ptr ptr ptr long long long long ptr)
@ stdcall CreateProcessAsUserA(long str str ptr ptr long long ptr str ptr ptr) kernel32.CreateProcessAsUserA
# @ stub CreateProcessAsUserSecure
@ stdcall CreateProcessAsUserW(long wstr wstr ptr ptr long long ptr wstr ptr ptr) kernel32.CreateProcessAsUserW
@ stdcall CreateProcessWithLogonW(wstr wstr wstr long wstr wstr long ptr wstr ptr ptr)
@ stdcall CreateProcessWithTokenW(long long wstr wstr long ptr wstr ptr ptr)
@ stdcall -import CreateRestrictedToken(long long long ptr long ptr long ptr ptr)
@ stdcall -import CreateServiceA(long str str long long long long str str ptr str str str)
@ stdcall -import CreateServiceW(long wstr wstr long long long long wstr wstr ptr wstr wstr wstr)
# @ stub CreateTraceInstanceId
@ stdcall -import CreateWellKnownSid(long ptr ptr ptr)
# @ stub CredBackupCredentials
@ stdcall CredDeleteA(str long long)
@ stdcall CredDeleteW(wstr long long)
# @ stub CredEncryptAndMarshalBinaryBlob
@ stdcall CredEnumerateA(str long ptr ptr)
@ stdcall CredEnumerateW(wstr long ptr ptr)
# @ stub CredFindBestCredentialA
# @ stub CredFindBestCredentialW
@ stdcall CredFree(ptr)
@ stdcall CredGetSessionTypes(long ptr)
# @ stub CredGetTargetInfoA
# @ stub CredGetTargetInfoW
@ stdcall CredIsMarshaledCredentialA(str)
@ stdcall CredIsMarshaledCredentialW(wstr)
# @ stub CredIsProtectedA
# @ stub CredIsProtectedW
@ stdcall CredMarshalCredentialA(long ptr ptr)
@ stdcall CredMarshalCredentialW(long ptr ptr)
@ stub CredProfileLoaded
# @ stub CredProfileLoadedEx
# @ stub CredProfileUnloaded
# @ stub CredProtectA
# @ stub CredProtectW
@ stdcall CredReadA(str long long ptr)
# @ stub CredReadByTokenHandle
@ stdcall CredReadDomainCredentialsA(ptr long ptr ptr)
@ stdcall CredReadDomainCredentialsW(ptr long ptr ptr)
@ stdcall CredReadW(wstr long long ptr)
# @ stub CredRenameA
# @ stub CredRenameW
# @ stub CredRestoreCredentials
@ stdcall CredUnmarshalCredentialA(str ptr ptr)
@ stdcall CredUnmarshalCredentialW(wstr ptr ptr)
# @ stub CredUnprotectA
# @ stub CredUnprotectW
@ stdcall CredWriteA(ptr long)
# @ stub CredWriteDomainCredentialsA
# @ stub CredWriteDomainCredentialsW
@ stdcall CredWriteW(ptr long)
# @ stub CredpConvertCredential
# @ stub CredpConvertOneCredentialSize
# @ stub CredpConvertTargetInfo
# @ stub CredpDecodeCredential
# @ stub CredpEncodeCredential
# @ stub CredpEncodeSecret
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
@ stdcall CryptSignHashA(long long str long ptr ptr)
@ stdcall CryptSignHashW(long long wstr long ptr ptr)
@ stdcall CryptVerifySignatureA(long ptr long long str long)
@ stdcall CryptVerifySignatureW(long ptr long long wstr long)
# @ stub CveEventWrite
@ stdcall DecryptFileA(str long)
@ stdcall DecryptFileW(wstr long)
@ stdcall -import DeleteAce(ptr long)
@ stdcall -import DeleteService(long)
@ stdcall DeregisterEventSource(long)
@ stdcall -import DestroyPrivateObjectSecurity(ptr)
# @ stub DuplicateEncryptionInfoFile
@ stdcall -import DuplicateToken(long long ptr)
@ stdcall -import DuplicateTokenEx(long long ptr long long ptr)
# @ stub ElfBackupEventLogFileA
# @ stub ElfBackupEventLogFileW
# @ stub ElfChangeNotify
# @ stub ElfClearEventLogFileA
# @ stub ElfClearEventLogFileW
# @ stub ElfCloseEventLog
@ stub ElfDeregisterEventSource
@ stub ElfDeregisterEventSourceW
# @ stub ElfFlushEventLog
# @ stub ElfNumberOfRecords
# @ stub ElfOldestRecord
# @ stub ElfOpenBackupEventLogA
# @ stub ElfOpenBackupEventLogW
# @ stub ElfOpenEventLogA
# @ stub ElfOpenEventLogW
# @ stub ElfReadEventLogA
# @ stub ElfReadEventLogW
# @ stub ElfRegisterEventSourceA
@ stub ElfRegisterEventSourceW
# @ stub ElfReportEventA
# @ stub ElfReportEventAndSourceW
@ stub ElfReportEventW
@ stdcall EnableTrace(long long long ptr int64)
@ stdcall EnableTraceEx(ptr ptr int64 long long int64 int64 long ptr)
@ stdcall -import EnableTraceEx2(int64 ptr long long int64 int64 long ptr)
@ stdcall EncryptFileA(str)
@ stdcall EncryptFileW(wstr)
# @ stub EncryptedFileKeyInfo
# @ stub EncryptionDisable
@ stdcall EnumDependentServicesA(long long ptr long ptr ptr)
@ stdcall -import EnumDependentServicesW(long long ptr long ptr ptr)
@ stdcall -import EnumDynamicTimeZoneInformation(long ptr)
@ stub EnumServiceGroupA
@ stub EnumServiceGroupW
@ stdcall EnumServicesStatusA (long long long ptr long ptr ptr ptr)
@ stdcall EnumServicesStatusExA(long long long long ptr long ptr ptr ptr str)
@ stdcall -import EnumServicesStatusExW(long long long long ptr long ptr ptr ptr wstr)
@ stdcall EnumServicesStatusW (long long long ptr long ptr ptr ptr)
@ stdcall EnumerateTraceGuids(ptr long ptr)
# @ stub EnumerateTraceGuidsEx
@ stdcall -import EqualDomainSid(ptr ptr ptr)
@ stdcall -import EqualPrefixSid(ptr ptr)
@ stdcall -import EqualSid(ptr ptr)
# @ stub EventAccessControl
# @ stub EventAccessQuery
# @ stub EventAccessRemove
@ stdcall EventActivityIdControl(long ptr) ntdll.EtwEventActivityIdControl
@ stdcall EventEnabled(int64 ptr) ntdll.EtwEventEnabled
@ stdcall EventProviderEnabled(int64 long int64) ntdll.EtwEventProviderEnabled
@ stdcall EventRegister(ptr ptr ptr ptr) ntdll.EtwEventRegister
@ stdcall EventSetInformation(int64 long ptr long) ntdll.EtwEventSetInformation
@ stdcall EventUnregister(int64) ntdll.EtwEventUnregister
@ stdcall EventWrite(int64 ptr long ptr) ntdll.EtwEventWrite
# @ stub EventWriteEndScenario
# @ stub EventWriteEx
# @ stub EventWriteStartScenario
@ stdcall EventWriteString(int64 long int64 ptr) ntdll.EtwEventWriteString
@ stdcall EventWriteTransfer(int64 ptr ptr ptr long ptr) ntdll.EtwEventWriteTransfer
@ stdcall FileEncryptionStatusA(str ptr)
@ stdcall FileEncryptionStatusW(wstr ptr)
@ stdcall -import FindFirstFreeAce(ptr ptr)
# @ stub FlushEfsCache
@ stdcall FlushTraceA(int64 str ptr)
@ stdcall FlushTraceW(int64 wstr ptr)
# @ stub FreeEncryptedFileKeyInfo
# @ stub FreeEncryptedFileMetadata
# @ stub FreeEncryptionCertificateHashList
# @ stub FreeInheritedFromArray
@ stdcall -import FreeSid(ptr)
# @ stub GetAccessPermissionsForObjectA
# @ stub GetAccessPermissionsForObjectW
@ stdcall -import GetAce(ptr long ptr)
@ stdcall -import GetAclInformation(ptr ptr long long)
@ stdcall GetAuditedPermissionsFromAclA(ptr ptr ptr ptr)
@ stdcall GetAuditedPermissionsFromAclW(ptr ptr ptr ptr)
@ stdcall GetCurrentHwProfileA(ptr)
@ stdcall GetCurrentHwProfileW(ptr)
@ stdcall GetDynamicTimeZoneInformationEffectiveYears(ptr ptr ptr) kernel32.GetDynamicTimeZoneInformationEffectiveYears
@ stdcall GetEffectiveRightsFromAclA(ptr ptr ptr)
@ stdcall GetEffectiveRightsFromAclW(ptr ptr ptr)
# @ stub GetEncryptedFileMetadata
@ stdcall GetEventLogInformation(long long ptr long ptr)
@ stdcall GetExplicitEntriesFromAclA(ptr ptr ptr)
@ stdcall GetExplicitEntriesFromAclW(ptr ptr ptr)
@ stdcall GetFileSecurityA(str long ptr long ptr)
@ stdcall -import GetFileSecurityW(wstr long ptr long ptr)
# @ stub GetInformationCodeAuthzLevelW
# @ stub GetInformationCodeAuthzPolicyW
# @ stub GetInheritanceSourceA
# @ stub GetInheritanceSourceW
@ stdcall -import GetKernelObjectSecurity(long long ptr long ptr)
@ stdcall -import GetLengthSid(ptr)
# @ stub GetLocalManagedApplicationData
# @ stub GetLocalManagedApplications
# @ stub GetManagedApplicationCategories
# @ stub GetManagedApplications
@ stub GetMangledSiteSid
# @ stub GetMultipleTrusteeA
# @ stub GetMultipleTrusteeOperationA
# @ stub GetMultipleTrusteeOperationW
# @ stub GetMultipleTrusteeW
@ stdcall GetNamedSecurityInfoA (str long long ptr ptr ptr ptr ptr)
@ stdcall GetNamedSecurityInfoExA(str long long str str ptr ptr ptr ptr)
@ stdcall GetNamedSecurityInfoExW(wstr long long wstr wstr ptr ptr ptr ptr)
@ stdcall GetNamedSecurityInfoW (wstr long long ptr ptr ptr ptr ptr)
@ stdcall GetNumberOfEventLogRecords (long ptr)
@ stdcall GetOldestEventLogRecord (long ptr)
# @ stub GetOverlappedAccessResults
@ stdcall -import GetPrivateObjectSecurity(ptr long ptr long ptr)
@ stdcall -import GetSecurityDescriptorControl (ptr ptr ptr)
@ stdcall -import GetSecurityDescriptorDacl (ptr ptr ptr ptr)
@ stdcall -import GetSecurityDescriptorGroup(ptr ptr ptr)
@ stdcall -import GetSecurityDescriptorLength(ptr)
@ stdcall -import GetSecurityDescriptorOwner(ptr ptr ptr)
# @ stub GetSecurityDescriptorRMControl
@ stdcall -import GetSecurityDescriptorSacl (ptr ptr ptr ptr)
@ stdcall GetSecurityInfo (long long long ptr ptr ptr ptr ptr)
@ stdcall GetSecurityInfoExA (long long long str str ptr ptr ptr ptr)
@ stdcall GetSecurityInfoExW (long long long wstr wstr ptr ptr ptr ptr)
@ stdcall GetServiceDisplayNameA(ptr str ptr ptr)
@ stdcall -import GetServiceDisplayNameW(ptr wstr ptr ptr)
@ stdcall GetServiceKeyNameA(long str ptr ptr)
@ stdcall -import GetServiceKeyNameW(long wstr ptr ptr)
@ stdcall -import GetSidIdentifierAuthority(ptr)
@ stdcall -import GetSidLengthRequired(long)
@ stdcall -import GetSidSubAuthority(ptr long)
@ stdcall -import GetSidSubAuthorityCount(ptr)
@ stub GetSiteSidFromToken
# @ stub GetStringConditionFromBinary
@ stdcall GetThreadWaitChain(ptr ptr long long ptr ptr ptr)
@ stdcall -import GetTokenInformation(long long ptr long ptr)
@ stdcall GetTraceEnableFlags(int64) ntdll.EtwGetTraceEnableFlags
@ stdcall GetTraceEnableLevel(int64) ntdll.EtwGetTraceEnableLevel
@ stdcall -ret64 GetTraceLoggerHandle(ptr) ntdll.EtwGetTraceLoggerHandle
@ stdcall GetTrusteeFormA(ptr)
@ stdcall GetTrusteeFormW(ptr)
@ stdcall GetTrusteeNameA(ptr)
@ stdcall GetTrusteeNameW(ptr)
@ stdcall GetTrusteeTypeA(ptr)
@ stdcall GetTrusteeTypeW(ptr)
@ stdcall GetUserNameA(ptr ptr)
@ stdcall GetUserNameW(ptr ptr)
@ stdcall -import GetWindowsAccountDomainSid(ptr ptr ptr)
# @ stub I_QueryTagInformation
# @ stub I_ScGetCurrentGroupStateW
# @ stub I_ScIsSecurityProcess
# @ stub I_ScPnPGetServiceName
# @ stub I_ScQueryServiceConfig
# @ stub I_ScRegisterPreshutdownRestart
# @ stub I_ScReparseServiceDatabase
# @ stub I_ScSendPnPMessage
# @ stub I_ScSendTSMessage
@ stub I_ScSetServiceBit
@ stub I_ScSetServiceBitsA
# @ stub I_ScSetServiceBitsW
# @ stub I_ScValidatePnPService
# @ stub IdentifyCodeAuthzLevelW
@ stdcall -import ImpersonateAnonymousToken(long)
@ stdcall -import ImpersonateLoggedOnUser(long)
@ stdcall -import ImpersonateNamedPipeClient(long)
@ stdcall -import ImpersonateSelf(long)
@ stdcall -import InitializeAcl(ptr long long)
@ stdcall -import InitializeSecurityDescriptor(ptr long)
@ stdcall -import InitializeSid(ptr ptr long)
@ stdcall InitiateShutdownA(str str long long long)
@ stdcall InitiateShutdownW(wstr wstr long long long)
@ stdcall InitiateSystemShutdownA(str str long long long)
@ stdcall InitiateSystemShutdownExA(str str long long long long)
@ stdcall InitiateSystemShutdownExW(wstr wstr long long long long)
@ stdcall InitiateSystemShutdownW(wstr wstr long long long)
@ stub InstallApplication
@ stub IsProcessRestricted
@ stdcall IsTextUnicode(ptr long ptr)
@ stdcall -import IsTokenRestricted(long)
# @ stub IsTokenUntrusted
@ stdcall -import IsValidAcl(ptr)
# @ stub IsValidRelativeSecurityDescriptor
@ stdcall -import IsValidSecurityDescriptor(ptr)
@ stdcall -import IsValidSid(ptr)
@ stdcall -import IsWellKnownSid(ptr long)
@ stdcall LockServiceDatabase(ptr)
@ stdcall LogonUserA(str str str long long ptr)
# @ stub LogonUserExA
# @ stub LogonUserExExW
# @ stub LogonUserExW
@ stdcall LogonUserW(wstr wstr wstr long long ptr)
@ stdcall LookupAccountNameA(str str ptr ptr ptr ptr ptr)
@ stdcall LookupAccountNameW(wstr wstr ptr ptr ptr ptr ptr)
@ stdcall LookupAccountSidA(str ptr ptr ptr ptr ptr ptr)
@ stdcall LookupAccountSidLocalA(ptr ptr ptr ptr ptr ptr)
@ stdcall LookupAccountSidLocalW(ptr ptr ptr ptr ptr ptr)
@ stdcall LookupAccountSidW(wstr ptr ptr ptr ptr ptr ptr)
@ stdcall LookupPrivilegeDisplayNameA(str str ptr ptr ptr)
@ stdcall LookupPrivilegeDisplayNameW(wstr wstr ptr ptr ptr)
@ stdcall LookupPrivilegeNameA(str ptr ptr ptr)
@ stdcall LookupPrivilegeNameW(wstr ptr ptr ptr)
@ stdcall LookupPrivilegeValueA(str str ptr)
@ stdcall LookupPrivilegeValueW(wstr wstr ptr)
@ stdcall LookupSecurityDescriptorPartsA(ptr ptr ptr ptr ptr ptr ptr)
@ stdcall LookupSecurityDescriptorPartsW(ptr ptr ptr ptr ptr ptr ptr)
@ stdcall LsaAddAccountRights(ptr ptr ptr long)
@ stub LsaAddPrivilegesToAccount
# @ stub LsaClearAuditLog
@ stdcall LsaClose(ptr)
@ stub LsaCreateAccount
@ stub LsaCreateSecret
@ stub LsaCreateTrustedDomain
@ stdcall LsaCreateTrustedDomainEx(ptr ptr ptr long ptr)
@ stub LsaDelete
@ stdcall LsaDeleteTrustedDomain(ptr ptr)
@ stdcall LsaEnumerateAccountRights(ptr ptr ptr ptr)
@ stdcall LsaEnumerateAccounts(ptr ptr ptr long ptr)
@ stdcall LsaEnumerateAccountsWithUserRight(ptr ptr ptr ptr)
@ stub LsaEnumeratePrivileges
@ stub LsaEnumeratePrivilegesOfAccount
@ stdcall LsaEnumerateTrustedDomains(ptr ptr ptr long ptr)
@ stdcall LsaEnumerateTrustedDomainsEx(ptr ptr ptr long ptr)
@ stdcall LsaFreeMemory(ptr)
# @ stub LsaGetAppliedCAPIDs
# @ stub LsaGetQuotasForAccount
# @ stub LsaGetRemoteUserName
@ stub LsaGetSystemAccessAccount
@ stdcall LsaGetUserName(ptr ptr)
@ stub LsaICLookupNames
# @ stub LsaICLookupNamesWithCreds
@ stub LsaICLookupSids
# @ stub LsaICLookupSidsWithCreds
@ stdcall LsaLookupNames(long long ptr ptr ptr)
@ stdcall LsaLookupNames2(ptr long long ptr ptr ptr)
@ stdcall LsaLookupPrivilegeDisplayName(long ptr ptr ptr)
@ stdcall LsaLookupPrivilegeName(long ptr ptr)
# @ stub LsaLookupPrivilegeValue
@ stdcall LsaLookupSids(ptr long ptr ptr ptr)
# @ stub LsaLookupSids2
# @ stub LsaManageSidNameMapping
@ stdcall LsaNtStatusToWinError(long)
@ stub LsaOpenAccount
@ stdcall LsaOpenPolicy(long ptr long long)
# @ stub LsaOpenPolicySce
@ stub LsaOpenSecret
@ stub LsaOpenTrustedDomain
@ stdcall LsaOpenTrustedDomainByName(ptr ptr long ptr)
# @ stub LsaQueryCAPs
# @ stub LsaQueryDomainInformationPolicy
# @ stub LsaQueryForestTrustInformation
@ stub LsaQueryInfoTrustedDomain
@ stdcall LsaQueryInformationPolicy(ptr long ptr)
@ stub LsaQuerySecret
# @ stub LsaQuerySecurityObject
@ stdcall LsaQueryTrustedDomainInfo(ptr ptr long ptr)
@ stdcall LsaQueryTrustedDomainInfoByName(ptr ptr long ptr)
@ stdcall LsaRegisterPolicyChangeNotification(long long)
@ stdcall LsaRemoveAccountRights(ptr ptr long ptr long)
@ stub LsaRemovePrivilegesFromAccount
@ stdcall LsaRetrievePrivateData(ptr ptr ptr)
# @ stub LsaSetCAPs
# @ stub LsaSetDomainInformationPolicy
# @ stub LsaSetForestTrustInformation
@ stdcall LsaSetInformationPolicy(long long ptr)
@ stub LsaSetInformationTrustedDomain
# @ stub LsaSetQuotasForAccount
@ stdcall LsaSetSecret(ptr ptr ptr)
# @ stub LsaSetSecurityObject
@ stub LsaSetSystemAccessAccount
@ stdcall LsaSetTrustedDomainInfoByName(ptr ptr long ptr)
@ stdcall LsaSetTrustedDomainInformation(ptr ptr long ptr)
@ stdcall LsaStorePrivateData(ptr ptr ptr)
@ stdcall LsaUnregisterPolicyChangeNotification(long long)
@ stdcall MD4Final(ptr) ntdll.MD4Final
@ stdcall MD4Init(ptr) ntdll.MD4Init
@ stdcall MD4Update(ptr ptr long) ntdll.MD4Update
@ stdcall MD5Final(ptr) ntdll.MD5Final
@ stdcall MD5Init(ptr) ntdll.MD5Init
@ stdcall MD5Update(ptr ptr long) ntdll.MD5Update
# @ stub MIDL_user_free_Ext
# @ stub MSChapSrvChangePassword
# @ stub MSChapSrvChangePassword2
@ stdcall -import MakeAbsoluteSD(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
# @ stub MakeAbsoluteSD2
@ stdcall -import MakeSelfRelativeSD(ptr ptr ptr)
@ stdcall -import MapGenericMask(ptr ptr)
@ stdcall NotifyBootConfigStatus(long)
@ stdcall NotifyChangeEventLog (long long)
# @ stub NotifyServiceStatusChange
# @ stub NotifyServiceStatusChangeA
@ stdcall -import NotifyServiceStatusChangeW(ptr long ptr)
# @ stub NpGetUserName
@ stdcall ObjectCloseAuditAlarmA(str ptr long)
@ stdcall -import ObjectCloseAuditAlarmW(wstr ptr long)
# @ stub ObjectDeleteAuditAlarmA
@ stdcall -import ObjectDeleteAuditAlarmW(wstr ptr long)
@ stdcall ObjectOpenAuditAlarmA(str ptr str str ptr long long long ptr long long ptr)
@ stdcall -import ObjectOpenAuditAlarmW(wstr ptr wstr wstr ptr long long long ptr long long ptr)
@ stdcall ObjectPrivilegeAuditAlarmA(str ptr long long ptr long)
@ stdcall -import ObjectPrivilegeAuditAlarmW(wstr ptr long long ptr long)
@ stdcall OpenBackupEventLogA (str str)
@ stdcall OpenBackupEventLogW (wstr wstr)
@ stdcall OpenEncryptedFileRawA(str long ptr)
@ stdcall OpenEncryptedFileRawW(wstr long ptr)
@ stdcall OpenEventLogA (str str)
@ stdcall OpenEventLogW (wstr wstr)
@ stdcall -import OpenProcessToken(long long ptr)
@ stdcall -import OpenSCManagerA(str str long)
@ stdcall -import OpenSCManagerW(wstr wstr long)
@ stdcall -import OpenServiceA(long str long)
@ stdcall -import OpenServiceW(long wstr long)
@ stdcall -import OpenThreadToken(long long long ptr)
@ stdcall OpenThreadWaitChainSession(long ptr)
@ stdcall -ret64 OpenTraceA(ptr)
@ stdcall -ret64 -import OpenTraceW(ptr)
# @ stub OperationEnd
# @ stub OperationStart
@ stdcall PerfAddCounters(long ptr long)
@ stdcall PerfCloseQueryHandle(long)
@ stdcall -import PerfCreateInstance(long ptr wstr long)
# @ stub PerfDecrementULongCounterValue
# @ stub PerfDecrementULongLongCounterValue
# @ stub PerfDeleteCounters
@ stdcall -import PerfDeleteInstance(long ptr)
# @ stub PerfEnumerateCounterSet
# @ stub PerfEnumerateCounterSetInstances
# @ stub PerfIncrementULongCounterValue
# @ stub PerfIncrementULongLongCounterValue
@ stdcall PerfOpenQueryHandle(wstr ptr)
@ stdcall PerfQueryCounterData(long ptr long ptr)
# @ stub PerfQueryCounterInfo
# @ stub PerfQueryCounterSetRegistrationInfo
# @ stub PerfQueryInstance
# @ stub PerfRegCloseKey
# @ stub PerfRegEnumKey
# @ stub PerfRegEnumValue
# @ stub PerfRegQueryInfoKey
# @ stub PerfRegQueryValue
# @ stub PerfRegSetValue
@ stdcall -import PerfSetCounterRefValue(long ptr long ptr)
@ stdcall -import PerfSetCounterSetInfo(long ptr long)
# @ stub PerfSetULongCounterValue
# @ stub PerfSetULongLongCounterValue
@ stdcall -import PerfStartProvider(ptr ptr ptr)
@ stdcall -import PerfStartProviderEx(ptr ptr ptr)
@ stdcall -import PerfStopProvider(long)
@ stdcall -import PrivilegeCheck(ptr ptr ptr)
@ stdcall PrivilegedServiceAuditAlarmA(str str long ptr long)
@ stdcall -import PrivilegedServiceAuditAlarmW(wstr wstr long ptr long)
# @ stub ProcessIdleTasks
# @ stub ProcessIdleTasksW
@ stdcall -import ProcessTrace(ptr long ptr ptr)
@ stdcall -import QueryAllTracesA(ptr long ptr)
@ stdcall -import QueryAllTracesW(ptr long ptr)
# @ stub QueryLocalUserServiceName
# @ stub QueryRecoveryAgentsOnEncryptedFile
# @ stub QuerySecurityAccessMask
@ stdcall -import QueryServiceConfig2A(long long ptr long ptr)
@ stdcall -import QueryServiceConfig2W(long long ptr long ptr)
@ stdcall -import QueryServiceConfigA(long ptr long ptr)
@ stdcall -import QueryServiceConfigW(long ptr long ptr)
# @ stub QueryServiceDynamicInformation
@ stdcall QueryServiceLockStatusA(long ptr long ptr)
@ stdcall QueryServiceLockStatusW(long ptr long ptr)
@ stdcall -import QueryServiceObjectSecurity(long long ptr long ptr)
@ stdcall -import QueryServiceStatus(long ptr)
@ stdcall -import QueryServiceStatusEx (long long ptr long ptr)
@ stdcall QueryTraceA(int64 str ptr)
@ stdcall QueryTraceW(int64 wstr ptr)
# @ stub QueryUserServiceName
# @ stub QueryUsersOnEncryptedFile
@ stdcall QueryWindows31FilesMigration(long)
@ stdcall ReadEncryptedFileRaw(ptr ptr ptr)
@ stdcall ReadEventLogA (long long long ptr long ptr ptr)
@ stdcall ReadEventLogW (long long long ptr long ptr ptr)
@ stdcall -import RegCloseKey(long)
@ stdcall RegConnectRegistryA(str long ptr)
# @ stub RegConnectRegistryExA
# @ stub RegConnectRegistryExW
@ stdcall RegConnectRegistryW(wstr long ptr)
@ stdcall RegCopyTreeA(long str long)
@ stdcall -import RegCopyTreeW(long wstr long)
@ stdcall RegCreateKeyA(long str ptr)
@ stdcall -import RegCreateKeyExA(long str long ptr long long ptr ptr ptr)
@ stdcall -import RegCreateKeyExW(long wstr long ptr long long ptr ptr ptr)
@ stdcall RegCreateKeyTransactedA(long str long ptr long long ptr ptr ptr long ptr)
@ stdcall RegCreateKeyTransactedW(long wstr long ptr long long ptr ptr ptr long ptr)
@ stdcall RegCreateKeyW(long wstr ptr)
@ stdcall RegDeleteKeyA(long str)
@ stdcall -import RegDeleteKeyExA(long str long long)
@ stdcall -import RegDeleteKeyExW(long wstr long long)
# @ stub RegDeleteKeyTransactedA
# @ stub RegDeleteKeyTransactedW
@ stdcall -import RegDeleteKeyValueA(long str str)
@ stdcall -import RegDeleteKeyValueW(long wstr wstr)
@ stdcall RegDeleteKeyW(long wstr)
@ stdcall -import RegDeleteTreeA(long str)
@ stdcall -import RegDeleteTreeW(long wstr)
@ stdcall -import RegDeleteValueA(long str)
@ stdcall -import RegDeleteValueW(long wstr)
@ stdcall RegDisablePredefinedCache()
# @ stub RegDisablePredefinedCacheEx
@ stdcall RegDisableReflectionKey(ptr)
@ stdcall RegEnableReflectionKey(ptr)
@ stdcall RegEnumKeyA(long long ptr long)
@ stdcall -import RegEnumKeyExA(long long ptr ptr ptr ptr ptr ptr)
@ stdcall -import RegEnumKeyExW(long long ptr ptr ptr ptr ptr ptr)
@ stdcall RegEnumKeyW(long long ptr long)
@ stdcall -import RegEnumValueA(long long ptr ptr ptr ptr ptr ptr)
@ stdcall -import RegEnumValueW(long long ptr ptr ptr ptr ptr ptr)
@ stdcall -import RegFlushKey(long)
@ stdcall -import RegGetKeySecurity(long long ptr ptr)
@ stdcall -import RegGetValueA(long str str long ptr ptr ptr)
@ stdcall -import RegGetValueW(long wstr wstr long ptr ptr ptr)
@ stdcall -import RegLoadAppKeyA(str ptr long long long)
@ stdcall -import RegLoadAppKeyW(wstr ptr long long long)
@ stdcall -import RegLoadKeyA(long str str)
@ stdcall -import RegLoadKeyW(long wstr wstr)
@ stdcall -import RegLoadMUIStringA(long str str long ptr long str)
@ stdcall -import RegLoadMUIStringW(long wstr wstr long ptr long wstr)
@ stdcall -import RegNotifyChangeKeyValue(long long long long long)
@ stdcall -import RegOpenCurrentUser(long ptr)
@ stdcall RegOpenKeyA(long str ptr)
@ stdcall -import RegOpenKeyExA(long str long long ptr)
@ stdcall -import RegOpenKeyExW(long wstr long long ptr)
# @ stub RegOpenKeyTransactedA
# @ stub RegOpenKeyTransactedW
@ stdcall RegOpenKeyW(long wstr ptr)
@ stdcall -import RegOpenUserClassesRoot(ptr long long ptr)
@ stdcall RegOverridePredefKey(long long)
@ stdcall -import RegQueryInfoKeyA(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall -import RegQueryInfoKeyW(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall RegQueryMultipleValuesA(long ptr long ptr ptr)
@ stdcall RegQueryMultipleValuesW(long ptr long ptr ptr)
@ stdcall RegQueryReflectionKey(long ptr)
@ stdcall RegQueryValueA(long str ptr ptr)
@ stdcall -import RegQueryValueExA(long str ptr ptr ptr ptr)
@ stdcall -import RegQueryValueExW(long wstr ptr ptr ptr ptr)
@ stdcall RegQueryValueW(long wstr ptr ptr)
@ stub RegRemapPreDefKey
@ stdcall RegRenameKey(long wstr wstr)
@ stdcall RegReplaceKeyA(long str str str)
@ stdcall RegReplaceKeyW(long wstr wstr wstr)
@ stdcall -import RegRestoreKeyA(long str long)
@ stdcall -import RegRestoreKeyW(long wstr long)
@ stdcall RegSaveKeyA(long str ptr)
@ stdcall -import RegSaveKeyExA(long str ptr long)
@ stdcall -import RegSaveKeyExW(long wstr ptr long)
@ stdcall RegSaveKeyW(long wstr ptr)
@ stdcall -import RegSetKeySecurity(long long ptr)
@ stdcall -import RegSetKeyValueA(long str str long ptr long)
@ stdcall -import RegSetKeyValueW(long wstr wstr long ptr long)
@ stdcall RegSetValueA(long str long str long)
@ stdcall -import RegSetValueExA(long str long long ptr long)
@ stdcall -import RegSetValueExW(long wstr long long ptr long)
@ stdcall RegSetValueW(long wstr long wstr long)
@ stdcall -import RegUnLoadKeyA(long str)
@ stdcall -import RegUnLoadKeyW(long wstr)
@ stdcall RegisterEventSourceA(str str)
@ stdcall RegisterEventSourceW(wstr wstr)
# @ stub RegisterIdleTask
@ stdcall -import RegisterServiceCtrlHandlerA(str ptr)
@ stdcall -import RegisterServiceCtrlHandlerExA(str ptr ptr)
@ stdcall -import RegisterServiceCtrlHandlerExW(wstr ptr ptr)
@ stdcall -import RegisterServiceCtrlHandlerW(wstr ptr)
@ stdcall RegisterTraceGuidsA(ptr ptr ptr long ptr str str ptr) ntdll.EtwRegisterTraceGuidsA
@ stdcall RegisterTraceGuidsW(ptr ptr ptr long ptr wstr wstr ptr) ntdll.EtwRegisterTraceGuidsW
@ stdcall RegisterWaitChainCOMCallback(ptr ptr)
# @ stub RemoteRegEnumKeyWrapper
# @ stub RemoteRegEnumValueWrapper
# @ stub RemoteRegQueryInfoKeyWrapper
# @ stub RemoteRegQueryValueWrapper
# @ stub RemoveTraceCallback
# @ stub RemoveUsersFromEncryptedFile
@ stdcall ReportEventA(long long long long ptr long long ptr ptr)
@ stdcall ReportEventW(long long long long ptr long long ptr ptr)
@ stdcall -import RevertToSelf()
# @ stub SafeBaseRegGetKeySecurity
@ stdcall SaferCloseLevel(ptr)
@ stdcall SaferComputeTokenFromLevel(ptr ptr ptr long ptr)
@ stdcall SaferCreateLevel(long long long ptr ptr)
# @ stub SaferGetLevelInformation
@ stdcall SaferGetPolicyInformation(long long long ptr ptr ptr)
@ stdcall SaferIdentifyLevel(long ptr ptr ptr)
# @ stub SaferRecordEventLogEntry
@ stdcall SaferSetLevelInformation(ptr long ptr long)
# @ stub SaferSetPolicyInformation
# @ stub SaferiChangeRegistryScope
# @ stub SaferiCompareTokenLevels
# @ stub SaferiIsDllAllowed
# @ stub SaferiIsExecutableFileType
# @ stub SaferiPopulateDefaultsInRegistry
# @ stub SaferiRecordEventLogEntry
# @ stub SaferiReplaceProcessThreadTokens
# @ stub SaferiSearchMatchingHashRules
@ stdcall -import SetAclInformation(ptr ptr long long)
# @ stub SetEncryptedFileMetadata
# @ stub SetEntriesInAccessListA
# @ stub SetEntriesInAccessListW
@ stdcall SetEntriesInAclA(long ptr ptr ptr)
@ stdcall SetEntriesInAclW(long ptr ptr ptr)
# @ stub SetEntriesInAuditListA
# @ stub SetEntriesInAuditListW
@ stdcall SetFileSecurityA(str long ptr )
@ stdcall -import SetFileSecurityW(wstr long ptr)
# @ stub SetInformationCodeAuthzLevelW
# @ stub SetInformationCodeAuthzPolicyW
@ stdcall -import SetKernelObjectSecurity(long long ptr)
@ stdcall SetNamedSecurityInfoA(str long long ptr ptr ptr ptr)
# @ stub SetNamedSecurityInfoExA
# @ stub SetNamedSecurityInfoExW
@ stdcall SetNamedSecurityInfoW(wstr long long ptr ptr ptr ptr)
@ stdcall -import SetPrivateObjectSecurity(long ptr ptr ptr long)
@ stdcall -import SetPrivateObjectSecurityEx(long ptr ptr long ptr long)
# @ stub SetSecurityAccessMask
@ stdcall -import SetSecurityDescriptorControl(ptr long long)
@ stdcall -import SetSecurityDescriptorDacl(ptr long ptr long)
@ stdcall -import SetSecurityDescriptorGroup (ptr ptr long)
@ stdcall -import SetSecurityDescriptorOwner (ptr ptr long)
# @ stub SetSecurityDescriptorRMControl
@ stdcall -import SetSecurityDescriptorSacl(ptr long ptr long)
@ stdcall SetSecurityInfo (long long long ptr ptr ptr ptr)
# @ stub SetSecurityInfoExA
# @ stub SetSecurityInfoExW
@ stdcall SetServiceBits(long long long long)
@ stdcall -import SetServiceObjectSecurity(long long ptr)
@ stdcall -import SetServiceStatus(long ptr)
@ stdcall -import SetThreadToken(ptr ptr)
@ stdcall -import SetTokenInformation(long long ptr long)
# @ stub SetTraceCallback
# @ stub SetUserFileEncryptionKey
# @ stub SetUserFileEncryptionKeyEx
@ stdcall -import StartServiceA(long long ptr)
@ stdcall -import StartServiceCtrlDispatcherA(ptr)
@ stdcall -import StartServiceCtrlDispatcherW(ptr)
@ stdcall -import StartServiceW(long long ptr)
@ stdcall -import StartTraceA(ptr str ptr)
@ stdcall -import StartTraceW(ptr wstr ptr)
@ stdcall StopTraceA(int64 str ptr)
@ stdcall -import StopTraceW(int64 wstr ptr)
@ stdcall SynchronizeWindows31FilesAndWindowsNTRegistry(long long long long)
@ stdcall SystemFunction001(ptr ptr ptr)
@ stdcall SystemFunction002(ptr ptr ptr)
@ stdcall SystemFunction003(ptr ptr)
@ stdcall SystemFunction004(ptr ptr ptr)
@ stdcall SystemFunction005(ptr ptr ptr)
@ stdcall SystemFunction006(ptr ptr)
@ stdcall SystemFunction007(ptr ptr)
@ stdcall SystemFunction008(ptr ptr ptr)
@ stdcall SystemFunction009(ptr ptr ptr)
@ stdcall SystemFunction010(ptr ptr ptr)
@ stdcall SystemFunction011(ptr ptr ptr) SystemFunction010
@ stdcall SystemFunction012(ptr ptr ptr)
@ stdcall SystemFunction013(ptr ptr ptr)
@ stdcall SystemFunction014(ptr ptr ptr) SystemFunction012
@ stdcall SystemFunction015(ptr ptr ptr) SystemFunction013
@ stdcall SystemFunction016(ptr ptr ptr) SystemFunction012
@ stdcall SystemFunction017(ptr ptr ptr) SystemFunction013
@ stdcall SystemFunction018(ptr ptr ptr) SystemFunction012
@ stdcall SystemFunction019(ptr ptr ptr) SystemFunction013
@ stdcall SystemFunction020(ptr ptr ptr) SystemFunction012
@ stdcall SystemFunction021(ptr ptr ptr) SystemFunction013
@ stdcall SystemFunction022(ptr ptr ptr) SystemFunction012
@ stdcall SystemFunction023(ptr ptr ptr) SystemFunction013
@ stdcall SystemFunction024(ptr ptr ptr)
@ stdcall SystemFunction025(ptr ptr ptr)
@ stdcall SystemFunction026(ptr ptr ptr) SystemFunction024
@ stdcall SystemFunction027(ptr ptr ptr) SystemFunction025
@ stub SystemFunction028
@ stub SystemFunction029
@ stdcall SystemFunction030(ptr ptr)
@ stdcall SystemFunction031(ptr ptr) SystemFunction030
@ stdcall SystemFunction032(ptr ptr)
@ stub SystemFunction033
@ stub SystemFunction034
@ stdcall SystemFunction035(str)
@ stdcall SystemFunction036(ptr long) # RtlGenRandom
@ stdcall SystemFunction040(ptr long long) # RtlEncryptMemory
@ stdcall SystemFunction041(ptr long long) # RtlDecryptMemory
@ stdcall TraceEvent(int64 ptr) ntdll.EtwLogTraceEvent
@ stub TraceEventInstance
@ varargs TraceMessage(int64 long ptr long) ntdll.EtwTraceMessage
@ stdcall TraceMessageVa(int64 long ptr long ptr) ntdll.EtwTraceMessageVa
# @ stub TraceQueryInformation
@ stdcall -import TraceSetInformation(int64 long ptr long)
# @ stub TreeResetNamedSecurityInfoA
@ stdcall TreeResetNamedSecurityInfoW(wstr long long ptr ptr ptr ptr long ptr long ptr)
# @ stub TreeSetNamedSecurityInfoA
@ stdcall TreeSetNamedSecurityInfoW(wstr long long ptr ptr ptr ptr long ptr long ptr)
# @ stub TrusteeAccessToObjectA
# @ stub TrusteeAccessToObjectW
# @ stub UninstallApplication
@ stdcall UnlockServiceDatabase (ptr)
# @ stub UnregisterIdleTask
@ stdcall UnregisterTraceGuids(int64) ntdll.EtwUnregisterTraceGuids
@ stub UpdateTraceA
@ stub UpdateTraceW
# @ stub UsePinForEncryptedFilesA
# @ stub UsePinForEncryptedFilesW
# @ stub WaitServiceState
@ stub WdmWmiServiceMain
@ stub WmiCloseBlock
# @ stub WmiCloseTraceWithCursor
# @ stub WmiConvertTimestamp
# @ stub WmiDevInstToInstanceNameA
# @ stub WmiDevInstToInstanceNameW
# @ stub WmiEnumerateGuids
@ stdcall WmiExecuteMethodA(long str long long ptr ptr ptr)
@ stdcall WmiExecuteMethodW(long wstr long long ptr ptr ptr)
# @ stub WmiFileHandleToInstanceNameA
# @ stub WmiFileHandleToInstanceNameW
@ stdcall WmiFreeBuffer(ptr)
# @ stub WmiGetFirstTraceOffset
# @ stub WmiGetNextEvent
# @ stub WmiGetTraceHeader
@ stdcall WmiMofEnumerateResourcesA(long ptr ptr)
@ stdcall WmiMofEnumerateResourcesW(long ptr ptr)
@ stdcall WmiNotificationRegistrationA(ptr long ptr long long)
@ stdcall WmiNotificationRegistrationW(ptr long ptr long long)
@ stdcall WmiOpenBlock(ptr long ptr)
# @ stub WmiOpenTraceWithCursor
# @ stub WmiParseTraceEvent
@ stdcall WmiQueryAllDataA(long ptr ptr)
# @ stub WmiQueryAllDataMultipleA
# @ stub WmiQueryAllDataMultipleW
@ stdcall WmiQueryAllDataW(long ptr ptr)
@ stdcall WmiQueryGuidInformation(long ptr)
# @ stub WmiQuerySingleInstanceA
# @ stub WmiQuerySingleInstanceMultipleA
# @ stub WmiQuerySingleInstanceMultipleW
@ stub WmiQuerySingleInstanceW
# @ stub WmiReceiveNotificationsA
# @ stub WmiReceiveNotificationsW
@ stdcall WmiSetSingleInstanceA(long str long long ptr)
@ stdcall WmiSetSingleInstanceW(long wstr long long ptr)
@ stdcall WmiSetSingleItemA(long str long long long ptr)
@ stdcall WmiSetSingleItemW(long wstr long long long ptr)
# @ stub Wow64Win32ApiEntry
@ stdcall WriteEncryptedFileRaw(ptr ptr ptr)
