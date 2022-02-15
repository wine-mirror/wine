@ stdcall CredDeleteA(str long long) sechost.CredDeleteA
@ stdcall CredDeleteW(wstr long long) sechost.CredDeleteW
@ stdcall CredEnumerateA(str long ptr ptr) sechost.CredEnumerateA
@ stdcall CredEnumerateW(wstr long ptr ptr) sechost.CredEnumerateW
@ stub CredFindBestCredentialA
@ stub CredFindBestCredentialW
@ stdcall CredFree(ptr) sechost.CredFree
@ stdcall CredGetSessionTypes(long ptr) sechost.CredGetSessionTypes
@ stub CredGetTargetInfoA
@ stub CredGetTargetInfoW
@ stdcall CredIsMarshaledCredentialW(wstr) sechost.CredIsMarshaledCredentialW
@ stub CredIsProtectedA
@ stub CredIsProtectedW
@ stdcall CredMarshalCredentialA(long ptr ptr) sechost.CredMarshalCredentialA
@ stdcall CredMarshalCredentialW(long ptr ptr) sechost.CredMarshalCredentialW
@ stub CredProtectA
@ stub CredProtectW
@ stdcall CredReadA(str long long ptr) sechost.CredReadA
@ stdcall CredReadDomainCredentialsA(ptr long ptr ptr) sechost.CredReadDomainCredentialsA
@ stdcall CredReadDomainCredentialsW(ptr long ptr ptr) sechost.CredReadDomainCredentialsW
@ stdcall CredReadW(wstr long long ptr) sechost.CredReadW
@ stdcall CredUnmarshalCredentialA(str ptr ptr) sechost.CredUnmarshalCredentialA
@ stdcall CredUnmarshalCredentialW(wstr ptr ptr) sechost.CredUnmarshalCredentialW
@ stub CredUnprotectA
@ stub CredUnprotectW
@ stdcall CredWriteA(ptr long) sechost.CredWriteA
@ stub CredWriteDomainCredentialsA
@ stub CredWriteDomainCredentialsW
@ stdcall CredWriteW(ptr long) sechost.CredWriteW
