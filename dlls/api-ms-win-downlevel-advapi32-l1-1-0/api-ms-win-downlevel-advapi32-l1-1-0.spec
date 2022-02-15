@ stdcall AccessCheck(ptr long long ptr ptr ptr ptr ptr) kernelbase.AccessCheck
@ stdcall AccessCheckAndAuditAlarmW(wstr ptr wstr wstr ptr long ptr long ptr ptr ptr) kernelbase.AccessCheckAndAuditAlarmW
@ stdcall AccessCheckByType(ptr ptr long long ptr long ptr ptr ptr ptr ptr) kernelbase.AccessCheckByType
@ stub AccessCheckByTypeAndAuditAlarmW
@ stub AccessCheckByTypeResultList
@ stub AccessCheckByTypeResultListAndAuditAlarmByHandleW
@ stub AccessCheckByTypeResultListAndAuditAlarmW
@ stdcall AddAccessAllowedAce(ptr long long ptr) kernelbase.AddAccessAllowedAce
@ stdcall AddAccessAllowedAceEx(ptr long long long ptr) kernelbase.AddAccessAllowedAceEx
@ stdcall AddAccessAllowedObjectAce(ptr long long long ptr ptr ptr) kernelbase.AddAccessAllowedObjectAce
@ stdcall AddAccessDeniedAce(ptr long long ptr) kernelbase.AddAccessDeniedAce
@ stdcall AddAccessDeniedAceEx(ptr long long long ptr) kernelbase.AddAccessDeniedAceEx
@ stdcall AddAccessDeniedObjectAce(ptr long long long ptr ptr ptr) kernelbase.AddAccessDeniedObjectAce
@ stdcall AddAce(ptr long long ptr long) kernelbase.AddAce
@ stdcall AddAuditAccessAce(ptr long long ptr long long) kernelbase.AddAuditAccessAce
@ stdcall AddAuditAccessAceEx(ptr long long long ptr long long) kernelbase.AddAuditAccessAceEx
@ stdcall AddAuditAccessObjectAce(ptr long long long ptr ptr ptr long long) kernelbase.AddAuditAccessObjectAce
@ stdcall AddMandatoryAce(ptr long long long ptr) kernelbase.AddMandatoryAce
@ stdcall AdjustTokenGroups(long long ptr long ptr ptr) kernelbase.AdjustTokenGroups
@ stdcall AdjustTokenPrivileges(long long ptr long ptr ptr) kernelbase.AdjustTokenPrivileges
@ stdcall AllocateAndInitializeSid(ptr long long long long long long long long long ptr) kernelbase.AllocateAndInitializeSid
@ stdcall AllocateLocallyUniqueId(ptr) kernelbase.AllocateLocallyUniqueId
@ stdcall AreAllAccessesGranted(long long) kernelbase.AreAllAccessesGranted
@ stdcall AreAnyAccessesGranted(long long) kernelbase.AreAnyAccessesGranted
@ stdcall CheckTokenMembership(long ptr ptr) kernelbase.CheckTokenMembership
@ stdcall ConvertToAutoInheritPrivateObjectSecurity(ptr ptr ptr ptr long ptr) kernelbase.ConvertToAutoInheritPrivateObjectSecurity
@ stdcall CopySid(long ptr ptr) kernelbase.CopySid
@ stdcall CreatePrivateObjectSecurity(ptr ptr ptr long long ptr) kernelbase.CreatePrivateObjectSecurity
@ stdcall CreatePrivateObjectSecurityEx(ptr ptr ptr ptr long long long ptr) kernelbase.CreatePrivateObjectSecurityEx
@ stdcall CreatePrivateObjectSecurityWithMultipleInheritance(ptr ptr ptr ptr long long long long ptr) kernelbase.CreatePrivateObjectSecurityWithMultipleInheritance
@ stdcall CreateProcessAsUserW(long wstr wstr ptr ptr long long ptr wstr ptr ptr) kernelbase.CreateProcessAsUserW
@ stdcall CreateRestrictedToken(long long long ptr long ptr long ptr ptr) kernelbase.CreateRestrictedToken
@ stdcall CreateWellKnownSid(long ptr ptr ptr) kernelbase.CreateWellKnownSid
@ stdcall DeleteAce(ptr long) kernelbase.DeleteAce
@ stdcall DestroyPrivateObjectSecurity(ptr) kernelbase.DestroyPrivateObjectSecurity
@ stdcall DuplicateToken(long long ptr) kernelbase.DuplicateToken
@ stdcall DuplicateTokenEx(long long ptr long long ptr) kernelbase.DuplicateTokenEx
@ stdcall EqualDomainSid(ptr ptr ptr) kernelbase.EqualDomainSid
@ stdcall EqualPrefixSid(ptr ptr) kernelbase.EqualPrefixSid
@ stdcall EqualSid(ptr ptr) kernelbase.EqualSid
@ stdcall EventActivityIdControl(long ptr) kernelbase.EventActivityIdControl
@ stdcall EventEnabled(int64 ptr) kernelbase.EventEnabled
@ stdcall EventProviderEnabled(int64 long int64) kernelbase.EventProviderEnabled
@ stdcall EventRegister(ptr ptr ptr ptr) kernelbase.EventRegister
@ stdcall EventUnregister(int64) kernelbase.EventUnregister
@ stdcall EventWrite(int64 ptr long ptr) kernelbase.EventWrite
@ stdcall EventWriteString(int64 long int64 ptr) kernelbase.EventWriteString
@ stdcall EventWriteTransfer(int64 ptr ptr ptr long ptr) kernelbase.EventWriteTransfer
@ stdcall FindFirstFreeAce(ptr ptr) kernelbase.FindFirstFreeAce
@ stdcall FreeSid(ptr) kernelbase.FreeSid
@ stdcall GetAce(ptr long ptr) kernelbase.GetAce
@ stdcall GetAclInformation(ptr ptr long long) kernelbase.GetAclInformation
@ stdcall GetFileSecurityW(wstr long ptr long ptr) kernelbase.GetFileSecurityW
@ stdcall GetKernelObjectSecurity(long long ptr long ptr) kernelbase.GetKernelObjectSecurity
@ stdcall GetLengthSid(ptr) kernelbase.GetLengthSid
@ stdcall GetPrivateObjectSecurity(ptr long ptr long ptr) kernelbase.GetPrivateObjectSecurity
@ stdcall GetSecurityDescriptorControl(ptr ptr ptr) kernelbase.GetSecurityDescriptorControl
@ stdcall GetSecurityDescriptorDacl(ptr ptr ptr ptr) kernelbase.GetSecurityDescriptorDacl
@ stdcall GetSecurityDescriptorGroup(ptr ptr ptr) kernelbase.GetSecurityDescriptorGroup
@ stdcall GetSecurityDescriptorLength(ptr) kernelbase.GetSecurityDescriptorLength
@ stdcall GetSecurityDescriptorOwner(ptr ptr ptr) kernelbase.GetSecurityDescriptorOwner
@ stub GetSecurityDescriptorRMControl
@ stdcall GetSecurityDescriptorSacl(ptr ptr ptr ptr) kernelbase.GetSecurityDescriptorSacl
@ stdcall GetSidIdentifierAuthority(ptr) kernelbase.GetSidIdentifierAuthority
@ stdcall GetSidLengthRequired(long) kernelbase.GetSidLengthRequired
@ stdcall GetSidSubAuthority(ptr long) kernelbase.GetSidSubAuthority
@ stdcall GetSidSubAuthorityCount(ptr) kernelbase.GetSidSubAuthorityCount
@ stdcall GetTokenInformation(long long ptr long ptr) kernelbase.GetTokenInformation
@ stdcall GetTraceEnableFlags(int64) kernelbase.GetTraceEnableFlags
@ stdcall GetTraceEnableLevel(int64) kernelbase.GetTraceEnableLevel
@ stdcall -ret64 GetTraceLoggerHandle(ptr) kernelbase.GetTraceLoggerHandle
@ stdcall InitializeAcl(ptr long long) kernelbase.InitializeAcl
@ stdcall InitializeSecurityDescriptor(ptr long) kernelbase.InitializeSecurityDescriptor
@ stdcall InitializeSid(ptr ptr long) kernelbase.InitializeSid
@ stdcall IsTokenRestricted(long) kernelbase.IsTokenRestricted
@ stdcall IsValidAcl(ptr) kernelbase.IsValidAcl
@ stdcall IsValidSecurityDescriptor(ptr) kernelbase.IsValidSecurityDescriptor
@ stdcall IsValidSid(ptr) kernelbase.IsValidSid
@ stdcall MakeAbsoluteSD(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) kernelbase.MakeAbsoluteSD
@ stdcall MakeSelfRelativeSD(ptr ptr ptr) kernelbase.MakeSelfRelativeSD
@ stdcall OpenProcessToken(long long ptr) kernelbase.OpenProcessToken
@ stdcall OpenThreadToken(long long long ptr) kernelbase.OpenThreadToken
@ stdcall PrivilegeCheck(ptr ptr ptr) kernelbase.PrivilegeCheck
@ stdcall PrivilegedServiceAuditAlarmW(wstr wstr long ptr long) kernelbase.PrivilegedServiceAuditAlarmW
@ stub QuerySecurityAccessMask
@ stdcall RegCloseKey(long) kernelbase.RegCloseKey
@ stdcall RegCopyTreeW(long wstr long) kernelbase.RegCopyTreeW
@ stdcall RegCreateKeyExA(long str long ptr long long ptr ptr ptr) kernelbase.RegCreateKeyExA
@ stdcall RegCreateKeyExW(long wstr long ptr long long ptr ptr ptr) kernelbase.RegCreateKeyExW
@ stdcall RegDeleteKeyExA(long str long long) kernelbase.RegDeleteKeyExA
@ stdcall RegDeleteKeyExW(long wstr long long) kernelbase.RegDeleteKeyExW
@ stdcall RegDeleteTreeA(long str) kernelbase.RegDeleteTreeA
@ stdcall RegDeleteTreeW(long wstr) kernelbase.RegDeleteTreeW
@ stdcall RegDeleteValueA(long str) kernelbase.RegDeleteValueA
@ stdcall RegDeleteValueW(long wstr) kernelbase.RegDeleteValueW
@ stub RegDisablePredefinedCacheEx
@ stdcall RegEnumKeyExA(long long ptr ptr ptr ptr ptr ptr) kernelbase.RegEnumKeyExA
@ stdcall RegEnumKeyExW(long long ptr ptr ptr ptr ptr ptr) kernelbase.RegEnumKeyExW
@ stdcall RegEnumValueA(long long ptr ptr ptr ptr ptr ptr) kernelbase.RegEnumValueA
@ stdcall RegEnumValueW(long long ptr ptr ptr ptr ptr ptr) kernelbase.RegEnumValueW
@ stdcall RegFlushKey(long) kernelbase.RegFlushKey
@ stdcall RegGetKeySecurity(long long ptr ptr) kernelbase.RegGetKeySecurity
@ stdcall RegGetValueA(long str str long ptr ptr ptr) kernelbase.RegGetValueA
@ stdcall RegGetValueW(long wstr wstr long ptr ptr ptr) kernelbase.RegGetValueW
@ stdcall RegLoadAppKeyA(str ptr long long long) kernelbase.RegLoadAppKeyA
@ stdcall RegLoadAppKeyW(wstr ptr long long long) kernelbase.RegLoadAppKeyW
@ stdcall RegLoadKeyA(long str str) kernelbase.RegLoadKeyA
@ stdcall RegLoadKeyW(long wstr wstr) kernelbase.RegLoadKeyW
@ stdcall RegLoadMUIStringA(long str str long ptr long str) kernelbase.RegLoadMUIStringA
@ stdcall RegLoadMUIStringW(long wstr wstr long ptr long wstr) kernelbase.RegLoadMUIStringW
@ stdcall RegNotifyChangeKeyValue(long long long long long) kernelbase.RegNotifyChangeKeyValue
@ stdcall RegOpenCurrentUser(long ptr) kernelbase.RegOpenCurrentUser
@ stdcall RegOpenKeyExA(long str long long ptr) kernelbase.RegOpenKeyExA
@ stdcall RegOpenKeyExW(long wstr long long ptr) kernelbase.RegOpenKeyExW
@ stdcall RegOpenUserClassesRoot(ptr long long ptr) kernelbase.RegOpenUserClassesRoot
@ stdcall RegQueryInfoKeyA(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) kernelbase.RegQueryInfoKeyA
@ stdcall RegQueryInfoKeyW(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) kernelbase.RegQueryInfoKeyW
@ stdcall RegQueryValueExA(long str ptr ptr ptr ptr) kernelbase.RegQueryValueExA
@ stdcall RegQueryValueExW(long wstr ptr ptr ptr ptr) kernelbase.RegQueryValueExW
@ stdcall RegRestoreKeyA(long str long) kernelbase.RegRestoreKeyA
@ stdcall RegRestoreKeyW(long wstr long) kernelbase.RegRestoreKeyW
@ stdcall RegSaveKeyExA(long str ptr long) kernelbase.RegSaveKeyExA
@ stdcall RegSaveKeyExW(long wstr ptr long) kernelbase.RegSaveKeyExW
@ stdcall RegSetKeySecurity(long long ptr) kernelbase.RegSetKeySecurity
@ stdcall RegSetValueExA(long str long long ptr long) kernelbase.RegSetValueExA
@ stdcall RegSetValueExW(long wstr long long ptr long) kernelbase.RegSetValueExW
@ stdcall RegUnLoadKeyA(long str) kernelbase.RegUnLoadKeyA
@ stdcall RegUnLoadKeyW(long wstr) kernelbase.RegUnLoadKeyW
@ stdcall RegisterTraceGuidsW(ptr ptr ptr long ptr wstr wstr ptr) kernelbase.RegisterTraceGuidsW
@ stdcall RevertToSelf() kernelbase.RevertToSelf
@ stdcall SetAclInformation(ptr ptr long long) kernelbase.SetAclInformation
@ stdcall SetFileSecurityW(wstr long ptr) kernelbase.SetFileSecurityW
@ stdcall SetKernelObjectSecurity(long long ptr) kernelbase.SetKernelObjectSecurity
@ stub SetSecurityAccessMask
@ stdcall SetSecurityDescriptorControl(ptr long long) kernelbase.SetSecurityDescriptorControl
@ stdcall SetSecurityDescriptorDacl(ptr long ptr long) kernelbase.SetSecurityDescriptorDacl
@ stdcall SetSecurityDescriptorGroup(ptr ptr long) kernelbase.SetSecurityDescriptorGroup
@ stdcall SetSecurityDescriptorOwner(ptr ptr long) kernelbase.SetSecurityDescriptorOwner
@ stub SetSecurityDescriptorRMControl
@ stdcall SetSecurityDescriptorSacl(ptr long ptr long) kernelbase.SetSecurityDescriptorSacl
@ stdcall SetTokenInformation(long long ptr long) kernelbase.SetTokenInformation
@ stdcall TraceEvent(int64 ptr) kernelbase.TraceEvent
@ varargs TraceMessage(int64 long ptr long) kernelbase.TraceMessage
@ stdcall TraceMessageVa(int64 long ptr long ptr) kernelbase.TraceMessageVa
@ stdcall UnregisterTraceGuids(int64) kernelbase.UnregisterTraceGuids
