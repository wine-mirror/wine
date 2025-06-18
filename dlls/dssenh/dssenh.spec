@ stdcall CPAcquireContext(ptr str long ptr)
@ stdcall CPCreateHash(ptr long ptr long ptr)
@ stub CPDecrypt
@ stdcall CPDeriveKey(ptr long ptr long ptr)
@ stdcall CPDestroyHash(ptr ptr)
@ stdcall CPDestroyKey(ptr ptr)
@ stdcall CPDuplicateHash(ptr ptr ptr long ptr)
@ stdcall CPDuplicateKey(ptr ptr ptr long ptr)
@ stub CPEncrypt
@ stdcall CPExportKey(ptr ptr ptr long long ptr ptr)
@ stdcall CPGenKey(ptr long long ptr)
@ stdcall CPGenRandom(ptr long ptr)
@ stdcall CPGetHashParam(ptr ptr long ptr ptr long)
@ stub CPGetKeyParam
@ stdcall CPGetProvParam(ptr long ptr ptr long)
@ stdcall CPGetUserKey(ptr long ptr)
@ stdcall CPHashData(ptr ptr ptr long long)
@ stub CPHashSessionKey
@ stdcall CPImportKey(ptr ptr long ptr long ptr)
@ stdcall CPReleaseContext(ptr long)
@ stdcall CPSetHashParam(ptr ptr long ptr long)
@ stdcall CPSetKeyParam(ptr ptr long ptr long)
@ stub CPSetProvParam
@ stdcall CPSignHash(ptr ptr long wstr long ptr ptr)
@ stdcall CPVerifySignature(ptr ptr ptr long ptr wstr long)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
