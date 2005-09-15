@ stdcall CertAddCRLContextToStore(long ptr long ptr)
@ stdcall CertAddCTLContextToStore(long ptr long ptr)
@ stdcall CertAddCertificateContextToStore(long ptr long ptr)
@ stdcall CertAddEncodedCRLToStore(long long ptr long long ptr)
@ stdcall CertAddEncodedCTLToStore(long long ptr long long ptr)
@ stdcall CertAddEncodedCertificateToStore(long long ptr long long ptr)
@ stub CertAddEncodedCertificateToSystemStoreA
@ stub CertAddEncodedCertificateToSystemStoreW
@ stub CertAddEnhancedKeyUsageIdentifier
@ stdcall CertAddSerializedElementToStore(ptr ptr long long long long ptr ptr)
@ stdcall CertAddStoreToCollection(ptr ptr long long)
@ stdcall CertAlgIdToOID(long)
@ stdcall CertCloseStore(ptr long)
@ stub CertCompareCertificate
@ stub CertCompareCertificateName
@ stub CertCompareIntegerBlob
@ stub CertComparePublicKeyInfo
@ stdcall CertControlStore(long long long ptr)
@ stdcall CertCreateCRLContext(long ptr long)
@ stdcall CertCreateCTLContext(long ptr long)
@ stub CertCreateCertificateChainEngine
@ stdcall CertCreateCertificateContext(long ptr long)
@ stdcall CertDeleteCRLFromStore(ptr)
@ stdcall CertDeleteCTLFromStore(ptr)
@ stdcall CertDeleteCertificateFromStore(ptr)
@ stub CertDuplicateCRLContext
@ stub CertDuplicateCTLContext
@ stdcall CertDuplicateCertificateContext(ptr)
@ stub CertDuplicateStore
@ stub CertEnumCRLContextProperties
@ stdcall CertEnumCRLsInStore(ptr ptr)
@ stub CertEnumCTLContextProperties
@ stdcall CertEnumCTLsInStore(ptr ptr)
@ stdcall CertEnumCertificateContextProperties(ptr long)
@ stdcall CertEnumCertificatesInStore(long ptr)
@ stdcall CertFindAttribute(str long ptr)
@ stub CertFindCTLInStore
@ stdcall CertFindCertificateInStore(long long long long ptr ptr)
@ stdcall CertFindExtension(str long ptr)
@ stdcall CertFindRDNAttr(str ptr)
@ stub CertFindSubjectInCTL
@ stdcall CertFreeCRLContext(ptr)
@ stdcall CertFreeCTLContext(ptr)
@ stub CertFreeCertificateChain
@ stub CertFreeCertificateChainEngine
@ stdcall CertFreeCertificateContext(ptr)
@ stdcall CertGetCRLContextProperty(ptr long ptr ptr)
@ stub CertGetCRLFromStore
@ stdcall CertGetCTLContextProperty(ptr long ptr ptr)
@ stub CertGetCertificateChain
@ stdcall CertGetCertificateContextProperty(ptr long ptr ptr)
@ stub CertGetEnhancedKeyUsage
@ stub CertGetIntendedKeyUsage
@ stub CertGetIssuerCertificateFromStore
@ stub CertGetPublicKeyLength
@ stub CertGetSubjectCertificateFromStore
@ stub CertIsRDNAttrsInCertificateName
@ stub CertNameToStrA
@ stub CertNameToStrW
@ stdcall CertOIDToAlgId(str)
@ stdcall CertOpenStore(str long long long ptr)
@ stdcall CertOpenSystemStoreA(long str)
@ stdcall CertOpenSystemStoreW(long wstr)
@ stub CertRDNValueToStrA
@ stub CertRDNValueToStrW
@ stub CertRemoveEnhancedKeyUsageIdentifier
@ stdcall CertRemoveStoreFromCollection(long long)
@ stdcall CertSaveStore(long long long long ptr long)
@ stdcall CertSerializeCRLStoreElement(ptr long ptr ptr)
@ stdcall CertSerializeCTLStoreElement(ptr long ptr ptr)
@ stdcall CertSerializeCertificateStoreElement(ptr long ptr ptr)
@ stdcall CertSetCRLContextProperty(ptr long long ptr)
@ stdcall CertSetCTLContextProperty(ptr long long ptr)
@ stdcall CertSetCertificateContextProperty(ptr long long ptr)
@ stub CertSetEnhancedKeyUsage
@ stub CertStrToNameA
@ stub CertStrToNameW
@ stub CertVerifyCRLRevocation
@ stub CertVerifyCRLTimeValidity
@ stub CertVerifyCTLUsage
@ stub CertVerifyRevocation
@ stub CertVerifySubjectCertificateContext
@ stdcall CertVerifyTimeValidity(ptr ptr)
@ stub CertVerifyValidityNesting
@ stub CreateFileU
@ stub CryptAcquireContextU
@ stub CryptCloseAsyncHandle
@ stub CryptCreateAsyncHandle
@ stub CryptDecodeMessage
@ stdcall CryptDecodeObject(long str ptr long long ptr ptr)
@ stdcall CryptDecodeObjectEx(long str ptr long long ptr ptr ptr)
@ stub CryptDecryptAndVerifyMessageSignature
@ stub CryptDecryptMessage
@ stdcall CryptEncodeObject(long str ptr ptr ptr)
@ stdcall CryptEncodeObjectEx(long str ptr long ptr ptr ptr)
@ stub CryptEncryptMessage
@ stub CryptEnumOIDFunction
@ stub CryptEnumOIDInfo
@ stub CryptEnumProvidersU
@ stub CryptExportPKCS8
@ stub CryptExportPublicKeyInfo
@ stub CryptExportPublicKeyInfoEx
@ stub CryptFindOIDInfo
@ stub CryptFormatObject
@ stub CryptFreeOIDFunctionAddress
@ stub CryptGetAsyncParam
@ stub CryptGetDefaultOIDDllList
@ stub CryptGetDefaultOIDFunctionAddress
@ stub CryptGetMessageCertificates
@ stub CryptGetMessageSignerCount
@ stub CryptGetOIDFunctionAddress
@ stdcall CryptGetOIDFunctionValue(long str str wstr ptr ptr ptr)
@ stdcall CryptHashCertificate(long long long ptr long ptr ptr)
@ stub CryptHashMessage
@ stub CryptHashPublicKeyInfo
@ stub CryptHashToBeSigned
@ stub CryptImportPKCS8
@ stub CryptImportPublicKeyInfo
@ stub CryptImportPublicKeyInfoEx
@ stdcall CryptInitOIDFunctionSet(str long)
@ stub CryptInstallOIDFunctionAddress
@ stub CryptLoadSip
@ stub CryptMemAlloc
@ stub CryptMemFree
@ stub CryptMemRealloc
@ stub CryptMsgCalculateEncodedLength
@ stub CryptMsgClose
@ stub CryptMsgControl
@ stub CryptMsgCountersign
@ stub CryptMsgCountersignEncoded
@ stub CryptMsgEncodeAndSignCTL
@ stub CryptMsgGetAndVerifySigner
@ stub CryptMsgGetParam
@ stub CryptMsgOpenToDecode
@ stub CryptMsgOpenToEncode
@ stub CryptMsgSignCTL
@ stub CryptMsgUpdate
@ stub CryptMsgVerifyCountersignatureEncoded
@ stdcall CryptProtectData(ptr wstr ptr ptr ptr long ptr)
@ stdcall CryptRegisterDefaultOIDFunction(long str long wstr)
@ stdcall CryptRegisterOIDFunction(long str str wstr str)
@ stub CryptRegisterOIDInfo
@ stdcall CryptSIPAddProvider(ptr)
@ stdcall CryptSIPLoad(ptr long ptr)
@ stdcall CryptSIPRemoveProvider(ptr)
@ stdcall CryptSIPRetrieveSubjectGuid(wstr long ptr)
@ stub CryptSetAsyncParam
@ stdcall CryptSetOIDFunctionValue(long str str wstr long ptr long)
@ stub CryptSetProviderU
@ stub CryptSignAndEncodeCertificate
@ stub CryptSignAndEncryptMessage
@ stub CryptSignCertificate
@ stub CryptSignHashU
@ stub CryptSignMessage
@ stub CryptSignMessageWithKey
@ stdcall CryptUnprotectData(ptr ptr ptr ptr ptr long ptr)
@ stdcall CryptUnregisterDefaultOIDFunction(long str wstr)
@ stdcall CryptUnregisterOIDFunction(long str str)
@ stub CryptUnregisterOIDInfo
@ stub CryptVerifyCertificateSignature
@ stub CryptVerifyDetachedMessageHash
@ stub CryptVerifyDetachedMessageSignature
@ stub CryptVerifyMessageHash
@ stub CryptVerifyMessageSignature
@ stub CryptVerifyMessageSignatureWithKey
@ stub CryptVerifySignatureU
@ stub I_CryptAllocTls
@ stdcall I_CryptCreateLruCache(long long)
@ stub I_CryptCreateLruEntry
@ stub I_CryptDetachTls
@ stdcall I_CryptFindLruEntryData(long)
@ stdcall I_CryptFlushLruCache(long)
@ stdcall I_CryptFreeLruCache(long)
@ stub I_CryptGetDefaultCryptProv
@ stub I_CryptGetDefaultCryptProvForEncrypt
@ stub I_CryptGetOssGlobal
@ stub I_CryptGetTls
@ stub I_CryptInsertLruEntry
@ stub I_CryptInstallOssGlobal
@ stub I_CryptReleaseLruEntry
@ stub I_CryptSetTls
@ stub I_CryptUninstallOssGlobal
@ stub PFXExportCertStore
@ stub PFXImportCertStore
@ stub RegCreateHKCUKeyExU
@ stub RegCreateKeyExU
@ stub RegDeleteValueU
@ stub RegEnumValueU
@ stub RegOpenHKCUKeyExU
@ stub RegOpenKeyExU
@ stub RegQueryInfoKeyU
@ stub RegQueryValueExU
@ stub RegSetValueExU
