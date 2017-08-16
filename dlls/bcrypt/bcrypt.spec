@ stub BCryptAddContextFunction
@ stub BCryptAddContextFunctionProvider
@ stdcall BCryptCloseAlgorithmProvider(ptr long)
@ stub BCryptConfigureContext
@ stub BCryptConfigureContextFunction
@ stub BCryptCreateContext
@ stdcall BCryptCreateHash(ptr ptr ptr long ptr long long)
@ stdcall BCryptDecrypt(ptr ptr long ptr ptr long ptr long ptr long)
@ stub BCryptDeleteContext
@ stub BCryptDeriveKey
@ stdcall BCryptDestroyHash(ptr)
@ stdcall BCryptDestroyKey(ptr)
@ stub BCryptDestroySecret
@ stdcall BCryptDuplicateHash(ptr ptr ptr long long)
@ stub BCryptDuplicateKey
@ stdcall BCryptEncrypt(ptr ptr long ptr ptr long ptr long ptr long)
@ stdcall BCryptEnumAlgorithms(long ptr ptr long)
@ stub BCryptEnumContextFunctionProviders
@ stub BCryptEnumContextFunctions
@ stub BCryptEnumContexts
@ stub BCryptEnumProviders
@ stub BCryptEnumRegisteredProviders
@ stub BCryptExportKey
@ stub BCryptFinalizeKeyPair
@ stdcall BCryptFinishHash(ptr ptr long long)
@ stub BCryptFreeBuffer
@ stdcall BCryptGenRandom(ptr ptr long long)
@ stub BCryptGenerateKeyPair
@ stdcall BCryptGenerateSymmetricKey(ptr ptr ptr long ptr long long)
@ stdcall BCryptGetFipsAlgorithmMode(ptr)
@ stdcall BCryptGetProperty(ptr wstr ptr long ptr long)
@ stdcall BCryptHash(ptr ptr long ptr long ptr long)
@ stdcall BCryptHashData(ptr ptr long long)
@ stub BCryptImportKey
@ stub BCryptImportKeyPair
@ stdcall BCryptOpenAlgorithmProvider(ptr wstr wstr long)
@ stub BCryptQueryContextConfiguration
@ stub BCryptQueryContextFunctionConfiguration
@ stub BCryptQueryContextFunctionProperty
@ stub BCryptQueryProviderRegistration
@ stub BCryptRegisterConfigChangeNotify
@ stub BCryptRegisterProvider
@ stub BCryptRemoveContextFunction
@ stub BCryptRemoveContextFunctionProvider
@ stub BCryptResolveProviders
@ stub BCryptSecretAgreement
@ stub BCryptSetAuditingInterface
@ stub BCryptSetContextFunctionProperty
@ stdcall BCryptSetProperty(ptr wstr ptr long long)
@ stub BCryptSignHash
@ stub BCryptUnregisterConfigChangeNotify
@ stub BCryptUnregisterProvider
@ stub BCryptVerifySignature
@ stub GetAsymmetricEncryptionInterface
@ stub GetCipherInterface
@ stub GetHashInterface
@ stub GetRngInterface
@ stub GetSecretAgreementInterface
@ stub GetSignatureInterface
