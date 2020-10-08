@ stdcall CPAcquireContext(ptr str long ptr)
@ stdcall CPCreateHash(ptr long ptr long ptr)
@ stub CPDecrypt
@ stdcall CPDeriveKey(ptr long ptr long ptr)
@ stdcall CPDestroyHash(ptr ptr)
@ stdcall CPDestroyKey(ptr ptr)
@ stub CPDuplicateHash
@ stub CPDuplicateKey
@ stub CPEncrypt
@ stdcall CPExportKey(ptr ptr ptr long long ptr ptr)
@ stdcall CPGenKey(ptr long long ptr)
@ stub CPGenRandom
@ stdcall CPGetHashParam(ptr ptr long ptr ptr long)
@ stub CPGetKeyParam
@ stdcall CPGetProvParam(ptr long ptr ptr long)
@ stub CPGetUserKey
@ stdcall CPHashData(ptr ptr ptr long long)
@ stub CPHashSessionKey
@ stdcall CPImportKey(ptr ptr long ptr long ptr)
@ stdcall CPReleaseContext(ptr long)
@ stub CPSetHashParam
@ stub CPSetKeyParam
@ stub CPSetProvParam
@ stdcall CPSignHash(ptr ptr long wstr long ptr ptr)
@ stdcall CPVerifySignature(ptr ptr ptr long ptr wstr long)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
