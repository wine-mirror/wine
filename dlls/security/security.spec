@ stdcall AcceptSecurityContext(ptr ptr ptr long long ptr ptr ptr ptr) secur32.AcceptSecurityContext
@ stdcall AcquireCredentialsHandleA(str str long ptr ptr ptr ptr ptr ptr) secur32.AcquireCredentialsHandleA
@ stdcall AcquireCredentialsHandleW(wstr wstr long ptr ptr ptr ptr ptr ptr) secur32.AcquireCredentialsHandleW
@ stdcall AddSecurityPackageA(str ptr) secur32.AddSecurityPackageA
@ stdcall AddSecurityPackageW(wstr ptr) secur32.AddSecurityPackageW
@ stdcall ApplyControlToken(ptr ptr) secur32.ApplyControlToken
@ stdcall CompleteAuthToken(ptr ptr) secur32.CompleteAuthToken
@ stdcall DecryptMessage(ptr ptr long ptr) secur32.DecryptMessage
@ stdcall DeleteSecurityContext(ptr) secur32.DeleteSecurityContext
@ stdcall DeleteSecurityPackageA(str) secur32.DeleteSecurityPackageA
@ stdcall DeleteSecurityPackageW(wstr) secur32.DeleteSecurityPackageW
@ stdcall EncryptMessage(ptr long ptr long) secur32.EncryptMessage
@ stdcall EnumerateSecurityPackagesA(ptr ptr) secur32.EnumerateSecurityPackagesA
@ stdcall EnumerateSecurityPackagesW(ptr ptr) secur32.EnumerateSecurityPackagesW
@ stdcall ExportSecurityContext(ptr long ptr ptr) secur32.ExportSecurityContext
@ stdcall FreeContextBuffer(ptr) secur32.FreeContextBuffer
@ stdcall FreeCredentialsHandle(ptr) secur32.FreeCredentialsHandle
@ stdcall ImpersonateSecurityContext(ptr) secur32.ImpersonateSecurityContext
@ stdcall ImportSecurityContextA(str ptr ptr ptr) secur32.ImportSecurityContextA
@ stdcall ImportSecurityContextW(wstr ptr ptr ptr) secur32.ImportSecurityContextW
@ stdcall InitSecurityInterfaceA() secur32.InitSecurityInterfaceA
@ stdcall InitSecurityInterfaceW() secur32.InitSecurityInterfaceW
@ stdcall InitializeSecurityContextA(ptr ptr str long long long ptr long ptr ptr ptr ptr) secur32.InitializeSecurityContextA
@ stdcall InitializeSecurityContextW(ptr ptr wstr long long long ptr long ptr ptr ptr ptr) secur32.InitializeSecurityContextW
@ stdcall MakeSignature(ptr long ptr long) secur32.MakeSignature
@ stdcall QueryContextAttributesA(ptr long ptr) secur32.QueryContextAttributesA
@ stdcall QueryContextAttributesW(ptr long ptr) secur32.QueryContextAttributesW
@ stdcall QueryCredentialsAttributesA(ptr long ptr) secur32.QueryCredentialsAttributesA
@ stdcall QueryCredentialsAttributesW(ptr long ptr) secur32.QueryCredentialsAttributesW
@ stdcall QuerySecurityContextToken(ptr ptr) secur32.QuerySecurityContextToken
@ stdcall QuerySecurityPackageInfoA(str ptr) secur32.QuerySecurityPackageInfoA
@ stdcall QuerySecurityPackageInfoW(wstr ptr) secur32.QuerySecurityPackageInfoW
@ stdcall RevertSecurityContext(ptr) secur32.RevertSecurityContext
@ stdcall SealMessage(ptr long ptr long) secur32.SealMessage
@ stdcall UnsealMessage(ptr ptr long ptr) secur32.UnsealMessage
@ stdcall VerifySignature(ptr ptr long ptr) secur32.VerifySignature
