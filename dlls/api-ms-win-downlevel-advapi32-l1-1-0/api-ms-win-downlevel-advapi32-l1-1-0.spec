@ stdcall AccessCheck(ptr long long ptr ptr ptr ptr ptr) advapi32.AccessCheck
@ stdcall AccessCheckAndAuditAlarmW(wstr ptr wstr wstr ptr long ptr long ptr ptr ptr) advapi32.AccessCheckAndAuditAlarmW
@ stdcall AccessCheckByType(ptr ptr long long ptr long ptr ptr ptr ptr ptr) advapi32.AccessCheckByType
@ stub AccessCheckByTypeAndAuditAlarmW
@ stub AccessCheckByTypeResultList
@ stub AccessCheckByTypeResultListAndAuditAlarmByHandleW
@ stub AccessCheckByTypeResultListAndAuditAlarmW
@ stdcall AddAccessAllowedAce(ptr long long ptr) advapi32.AddAccessAllowedAce
@ stdcall AddAccessAllowedAceEx(ptr long long long ptr) advapi32.AddAccessAllowedAceEx
@ stdcall AddAccessAllowedObjectAce(ptr long long long ptr ptr ptr) advapi32.AddAccessAllowedObjectAce
@ stdcall AddAccessDeniedAce(ptr long long ptr) advapi32.AddAccessDeniedAce
@ stdcall AddAccessDeniedAceEx(ptr long long long ptr) advapi32.AddAccessDeniedAceEx
@ stdcall AddAccessDeniedObjectAce(ptr long long long ptr ptr ptr) advapi32.AddAccessDeniedObjectAce
@ stdcall AddAce(ptr long long ptr long) advapi32.AddAce
@ stdcall AddAuditAccessAce(ptr long long ptr long long) advapi32.AddAuditAccessAce
@ stdcall AddAuditAccessAceEx(ptr long long long ptr long long) advapi32.AddAuditAccessAceEx
@ stdcall AddAuditAccessObjectAce(ptr long long long ptr ptr ptr long long) advapi32.AddAuditAccessObjectAce
@ stdcall AddMandatoryAce(ptr long long long ptr) advapi32.AddMandatoryAce
@ stdcall AdjustTokenGroups(long long ptr long ptr ptr) advapi32.AdjustTokenGroups
@ stdcall AdjustTokenPrivileges(long long ptr long ptr ptr) advapi32.AdjustTokenPrivileges
@ stdcall AllocateAndInitializeSid(ptr long long long long long long long long long ptr) advapi32.AllocateAndInitializeSid
@ stdcall AllocateLocallyUniqueId(ptr) advapi32.AllocateLocallyUniqueId
@ stdcall AreAllAccessesGranted(long long) advapi32.AreAllAccessesGranted
@ stdcall AreAnyAccessesGranted(long long) advapi32.AreAnyAccessesGranted
@ stdcall CheckTokenMembership(long ptr ptr) advapi32.CheckTokenMembership
@ stub ConvertToAutoInheritPrivateObjecSecurity
@ stdcall CopySid(long ptr ptr) advapi32.CopySid
@ stdcall CreatePrivateObjectSecurity(ptr ptr ptr long long ptr) advapi32.CreatePrivateObjectSecurity
@ stdcall CreatePrivateObjectSecurityEx(ptr ptr ptr ptr long long long ptr) advapi32.CreatePrivateObjectSecurityEx
@ stdcall CreatePrivateObjectSecurityWithMultipleInheritance(ptr ptr ptr ptr long long long long ptr) advapi32.CreatePrivateObjectSecurityWithMultipleInheritance
@ stdcall CreateProcessAsUserW(long wstr wstr ptr ptr long long ptr wstr ptr ptr) advapi32.CreateProcessAsUserW
@ stdcall CreateRestrictedToken(long long long ptr long ptr long ptr ptr) advapi32.CreateRestrictedToken
@ stdcall CreateWellKnownSid(long ptr ptr ptr) advapi32.CreateWellKnownSid
@ stdcall DeleteAce(ptr long) advapi32.DeleteAce
@ stdcall DestroyPrivateObjectSecurity(ptr) advapi32.DestroyPrivateObjectSecurity
@ stdcall DuplicateToken(long long ptr) advapi32.DuplicateToken
@ stdcall DuplicateTokenEx(long long ptr long long ptr) advapi32.DuplicateTokenEx
@ stub EqualDomainSid
@ stdcall EqualPrefixSid(ptr ptr) advapi32.EqualPrefixSid
@ stdcall EqualSid(ptr ptr) advapi32.EqualSid
@ stdcall EventActivityIdControl(long ptr) advapi32.EventActivityIdControl
@ stdcall EventEnabled(int64 ptr) advapi32.EventEnabled
@ stdcall EventProviderEnabled(int64 long int64) advapi32.EventProviderEnabled
@ stdcall EventRegister(ptr ptr ptr ptr) advapi32.EventRegister
@ stdcall EventUnregister(int64) advapi32.EventUnregister
@ stdcall EventWrite(int64 ptr long ptr) advapi32.EventWrite
@ stub EventWriteString
@ stdcall EventWriteTransfer(int64 ptr ptr ptr long ptr) advapi32.EventWriteTransfer
@ stdcall FindFirstFreeAce(ptr ptr) advapi32.FindFirstFreeAce
@ stdcall FreeSid(ptr) advapi32.FreeSid
@ stdcall GetAce(ptr long ptr) advapi32.GetAce
@ stdcall GetAclInformation(ptr ptr long long) advapi32.GetAclInformation
@ stdcall GetFileSecurityW(wstr long ptr long ptr) advapi32.GetFileSecurityW
@ stdcall GetKernelObjectSecurity(long long ptr long ptr) advapi32.GetKernelObjectSecurity
@ stdcall GetLengthSid(ptr) advapi32.GetLengthSid
@ stdcall GetPrivateObjectSecurity(ptr long ptr long ptr) advapi32.GetPrivateObjectSecurity
@ stdcall GetSecurityDescriptorControl(ptr ptr ptr) advapi32.GetSecurityDescriptorControl
@ stdcall GetSecurityDescriptorDacl(ptr ptr ptr ptr) advapi32.GetSecurityDescriptorDacl
@ stdcall GetSecurityDescriptorGroup(ptr ptr ptr) advapi32.GetSecurityDescriptorGroup
@ stdcall GetSecurityDescriptorLength(ptr) advapi32.GetSecurityDescriptorLength
@ stdcall GetSecurityDescriptorOwner(ptr ptr ptr) advapi32.GetSecurityDescriptorOwner
@ stub GetSecurityDescriptorRMControl
@ stdcall GetSecurityDescriptorSacl(ptr ptr ptr ptr) advapi32.GetSecurityDescriptorSacl
@ stdcall GetSidIdentifierAuthority(ptr) advapi32.GetSidIdentifierAuthority
@ stdcall GetSidLengthRequired(long) advapi32.GetSidLengthRequired
@ stdcall GetSidSubAuthority(ptr long) advapi32.GetSidSubAuthority
@ stdcall GetSidSubAuthorityCount(ptr) advapi32.GetSidSubAuthorityCount
@ stdcall GetTokenInformation(long long ptr long ptr) advapi32.GetTokenInformation
@ stdcall GetTraceEnableFlags(int64) advapi32.GetTraceEnableFlags
@ stdcall GetTraceEnableLevel(int64) advapi32.GetTraceEnableLevel
@ stdcall -ret64 GetTraceLoggerHandle(ptr) advapi32.GetTraceLoggerHandle
@ stdcall InitializeAcl(ptr long long) advapi32.InitializeAcl
@ stdcall InitializeSecurityDescriptor(ptr long) advapi32.InitializeSecurityDescriptor
@ stdcall InitializeSid(ptr ptr long) advapi32.InitializeSid
@ stdcall IsTokenRestricted(long) advapi32.IsTokenRestricted
@ stdcall IsValidAcl(ptr) advapi32.IsValidAcl
@ stdcall IsValidSecurityDescriptor(ptr) advapi32.IsValidSecurityDescriptor
@ stdcall IsValidSid(ptr) advapi32.IsValidSid
@ stdcall MakeAbsoluteSD(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) advapi32.MakeAbsoluteSD
@ stdcall MakeSelfRelativeSD(ptr ptr ptr) advapi32.MakeSelfRelativeSD
@ stdcall OpenProcessToken(long long ptr) advapi32.OpenProcessToken
@ stdcall OpenThreadToken(long long long ptr) advapi32.OpenThreadToken
@ stdcall PrivilegeCheck(ptr ptr ptr) advapi32.PrivilegeCheck
@ stdcall PrivilegedServiceAuditAlarmW(wstr wstr long ptr long) advapi32.PrivilegedServiceAuditAlarmW
@ stub QuerySecurityAccessMask
@ stdcall RegCloseKey(long) advapi32.RegCloseKey
@ stdcall RegCopyTreeW(long wstr long) advapi32.RegCopyTreeW
@ stdcall RegCreateKeyExA(long str long ptr long long ptr ptr ptr) advapi32.RegCreateKeyExA
@ stdcall RegCreateKeyExW(long wstr long ptr long long ptr ptr ptr) advapi32.RegCreateKeyExW
@ stdcall RegDeleteKeyExA(long str long long) advapi32.RegDeleteKeyExA
@ stdcall RegDeleteKeyExW(long wstr long long) advapi32.RegDeleteKeyExW
@ stdcall RegDeleteTreeA(long str) advapi32.RegDeleteTreeA
@ stdcall RegDeleteTreeW(long wstr) advapi32.RegDeleteTreeW
@ stdcall RegDeleteValueA(long str) advapi32.RegDeleteValueA
@ stdcall RegDeleteValueW(long wstr) advapi32.RegDeleteValueW
@ stub RegDisablePredefinedCacheEx
@ stdcall RegEnumKeyExA(long long ptr ptr ptr ptr ptr ptr) advapi32.RegEnumKeyExA
@ stdcall RegEnumKeyExW(long long ptr ptr ptr ptr ptr ptr) advapi32.RegEnumKeyExW
@ stdcall RegEnumValueA(long long ptr ptr ptr ptr ptr ptr) advapi32.RegEnumValueA
@ stdcall RegEnumValueW(long long ptr ptr ptr ptr ptr ptr) advapi32.RegEnumValueW
@ stdcall RegFlushKey(long) advapi32.RegFlushKey
@ stdcall RegGetKeySecurity(long long ptr ptr) advapi32.RegGetKeySecurity
@ stdcall RegGetValueA(long str str long ptr ptr ptr) advapi32.RegGetValueA
@ stdcall RegGetValueW(long wstr wstr long ptr ptr ptr) advapi32.RegGetValueW
@ stdcall RegLoadAppKeyA(str ptr long long long) advapi32.RegLoadAppKeyA
@ stdcall RegLoadAppKeyW(wstr ptr long long long) advapi32.RegLoadAppKeyW
@ stdcall RegLoadKeyA(long str str) advapi32.RegLoadKeyA
@ stdcall RegLoadKeyW(long wstr wstr) advapi32.RegLoadKeyW
@ stdcall RegLoadMUIStringA(long str str long ptr long str) advapi32.RegLoadMUIStringA
@ stdcall RegLoadMUIStringW(long wstr wstr long ptr long wstr) advapi32.RegLoadMUIStringW
@ stdcall RegNotifyChangeKeyValue(long long long long long) advapi32.RegNotifyChangeKeyValue
@ stdcall RegOpenCurrentUser(long ptr) advapi32.RegOpenCurrentUser
@ stdcall RegOpenKeyExA(long str long long ptr) advapi32.RegOpenKeyExA
@ stdcall RegOpenKeyExW(long wstr long long ptr) advapi32.RegOpenKeyExW
@ stdcall RegOpenUserClassesRoot(ptr long long ptr) advapi32.RegOpenUserClassesRoot
@ stdcall RegQueryInfoKeyA(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) advapi32.RegQueryInfoKeyA
@ stdcall RegQueryInfoKeyW(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) advapi32.RegQueryInfoKeyW
@ stdcall RegQueryValueExA(long str ptr ptr ptr ptr) advapi32.RegQueryValueExA
@ stdcall RegQueryValueExW(long wstr ptr ptr ptr ptr) advapi32.RegQueryValueExW
@ stdcall RegRestoreKeyA(long str long) advapi32.RegRestoreKeyA
@ stdcall RegRestoreKeyW(long wstr long) advapi32.RegRestoreKeyW
@ stdcall RegSaveKeyExA(long str ptr long) advapi32.RegSaveKeyExA
@ stdcall RegSaveKeyExW(long wstr ptr long) advapi32.RegSaveKeyExW
@ stdcall RegSetKeySecurity(long long ptr) advapi32.RegSetKeySecurity
@ stdcall RegSetValueExA(long str long long ptr long) advapi32.RegSetValueExA
@ stdcall RegSetValueExW(long wstr long long ptr long) advapi32.RegSetValueExW
@ stdcall RegUnLoadKeyA(long str) advapi32.RegUnLoadKeyA
@ stdcall RegUnLoadKeyW(long wstr) advapi32.RegUnLoadKeyW
@ stdcall RegisterTraceGuidsW(ptr ptr ptr long ptr wstr wstr ptr) advapi32.RegisterTraceGuidsW
@ stdcall RevertToSelf() advapi32.RevertToSelf
@ stdcall SetAclInformation(ptr ptr long long) advapi32.SetAclInformation
@ stdcall SetFileSecurityW(wstr long ptr) advapi32.SetFileSecurityW
@ stdcall SetKernelObjectSecurity(long long ptr) advapi32.SetKernelObjectSecurity
@ stub SetSecurityAccessMask
@ stdcall SetSecurityDescriptorControl(ptr long long) advapi32.SetSecurityDescriptorControl
@ stdcall SetSecurityDescriptorDacl(ptr long ptr long) advapi32.SetSecurityDescriptorDacl
@ stdcall SetSecurityDescriptorGroup(ptr ptr long) advapi32.SetSecurityDescriptorGroup
@ stdcall SetSecurityDescriptorOwner(ptr ptr long) advapi32.SetSecurityDescriptorOwner
@ stub SetSecurityDescriptorRMControl
@ stdcall SetSecurityDescriptorSacl(ptr long ptr long) advapi32.SetSecurityDescriptorSacl
@ stdcall SetTokenInformation(long long ptr long) advapi32.SetTokenInformation
@ stdcall TraceEvent(int64 ptr) advapi32.TraceEvent
@ varargs TraceMessage(int64 long ptr long) advapi32.TraceMessage
@ stdcall TraceMessageVa(int64 long ptr long ptr) advapi32.TraceMessageVa
@ stdcall UnregisterTraceGuids(int64) advapi32.UnregisterTraceGuids
