@ stdcall CloseServiceHandle(long) sechost.CloseServiceHandle
@ stdcall ConvertSidToStringSidW(ptr ptr) sechost.ConvertSidToStringSidW
@ stdcall ConvertStringSecurityDescriptorToSecurityDescriptorW(wstr long ptr ptr) sechost.ConvertStringSecurityDescriptorToSecurityDescriptorW
@ stdcall ConvertStringSidToSidW(wstr ptr) sechost.ConvertStringSidToSidW
@ stdcall CredDeleteW(wstr long long) sechost.CredDeleteW
@ stdcall CredEnumerateW(wstr long ptr ptr) sechost.CredEnumerateW
@ stdcall CredFree(ptr) sechost.CredFree
@ stdcall CredReadDomainCredentialsW(ptr long ptr ptr) sechost.CredReadDomainCredentialsW
@ stdcall CredReadW(wstr long long ptr) sechost.CredReadW
@ stub CredWriteDomainCredentialsW
@ stdcall CredWriteW(ptr long) sechost.CredWriteW
@ stdcall OpenSCManagerW(wstr wstr long) sechost.OpenSCManagerW
@ stdcall OpenServiceW(long wstr long) sechost.OpenServiceW
@ stdcall QueryServiceConfigW(long ptr long ptr) sechost.QueryServiceConfigW
