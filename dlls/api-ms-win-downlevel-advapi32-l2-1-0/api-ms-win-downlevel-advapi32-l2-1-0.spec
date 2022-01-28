@ stdcall CloseServiceHandle(long) advapi32.CloseServiceHandle
@ stdcall ConvertSidToStringSidW(ptr ptr) advapi32.ConvertSidToStringSidW
@ stdcall ConvertStringSecurityDescriptorToSecurityDescriptorW(wstr long ptr ptr) advapi32.ConvertStringSecurityDescriptorToSecurityDescriptorW
@ stdcall ConvertStringSidToSidW(wstr ptr) advapi32.ConvertStringSidToSidW
@ stdcall CredDeleteW(wstr long long) advapi32.CredDeleteW
@ stdcall CredEnumerateW(wstr long ptr ptr) advapi32.CredEnumerateW
@ stdcall CredFree(ptr) advapi32.CredFree
@ stdcall CredReadDomainCredentialsW(ptr long ptr ptr) advapi32.CredReadDomainCredentialsW
@ stdcall CredReadW(wstr long long ptr) advapi32.CredReadW
@ stub CredWriteDomainCredentialsW
@ stdcall CredWriteW(ptr long) advapi32.CredWriteW
@ stdcall OpenSCManagerW(wstr wstr long) advapi32.OpenSCManagerW
@ stdcall OpenServiceW(long wstr long) advapi32.OpenServiceW
@ stdcall QueryServiceConfigW(long ptr long ptr) advapi32.QueryServiceConfigW
