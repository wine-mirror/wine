@ stdcall CredPackAuthenticationBufferW(long wstr ptr ptr ptr)
@ stub CredUICmdLinePromptForCredentialsA
@ stub CredUICmdLinePromptForCredentialsW
@ stub CredUIConfirmCredentialsA
@ stdcall CredUIConfirmCredentialsW(wstr long)
@ stdcall CredUIInitControls()
@ stub CredUIParseUserNameA
@ stdcall CredUIParseUserNameW(wstr ptr long ptr long)
@ stub CredUIPromptForCredentialsA
@ stdcall CredUIPromptForCredentialsW(ptr wstr ptr long ptr long ptr long ptr long)
@ stdcall CredUIPromptForWindowsCredentialsW(ptr long ptr ptr long ptr ptr ptr long)
@ stdcall CredUIReadSSOCredA(str ptr)
@ stdcall CredUIReadSSOCredW(wstr ptr)
@ stdcall CredUIStoreSSOCredA(str str str long)
@ stdcall CredUIStoreSSOCredW(wstr wstr wstr long)
@ stdcall CredUnPackAuthenticationBufferW(long ptr long ptr ptr ptr ptr ptr ptr)
@ stdcall -private DllCanUnloadNow()
@ stub DllGetClassObject
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stdcall SspiPromptForCredentialsW(wstr ptr long wstr ptr ptr ptr long)
