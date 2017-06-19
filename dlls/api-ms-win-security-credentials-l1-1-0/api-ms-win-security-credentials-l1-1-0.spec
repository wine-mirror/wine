@ stdcall CredDeleteA(str long long) advapi32.CredDeleteA
@ stdcall CredDeleteW(wstr long long) advapi32.CredDeleteW
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
@ stub CredProtectA
@ stub CredProtectW
@ stdcall CredReadA(str long long ptr) advapi32.CredReadA
@ stdcall CredReadDomainCredentialsA(ptr long ptr ptr) advapi32.CredReadDomainCredentialsA
@ stdcall CredReadDomainCredentialsW(ptr long ptr ptr) advapi32.CredReadDomainCredentialsW
@ stdcall CredReadW(wstr long long ptr) advapi32.CredReadW
@ stdcall CredUnmarshalCredentialA(str ptr ptr) advapi32.CredUnmarshalCredentialA
@ stdcall CredUnmarshalCredentialW(wstr ptr ptr) advapi32.CredUnmarshalCredentialW
@ stub CredUnprotectA
@ stub CredUnprotectW
@ stdcall CredWriteA(ptr long) advapi32.CredWriteA
@ stub CredWriteDomainCredentialsA
@ stub CredWriteDomainCredentialsW
@ stdcall CredWriteW(ptr long) advapi32.CredWriteW
