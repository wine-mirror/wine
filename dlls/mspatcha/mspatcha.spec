@ stdcall ApplyPatchToFileA(str str str long)
@ stdcall ApplyPatchToFileByBuffers(ptr long ptr long ptr long ptr ptr long ptr ptr)
@ stdcall ApplyPatchToFileByHandles(ptr ptr ptr long)
@ stdcall ApplyPatchToFileByHandlesEx(ptr ptr ptr long ptr ptr)
@ stdcall ApplyPatchToFileExA(str str str long ptr ptr)
@ stdcall ApplyPatchToFileExW(wstr wstr wstr long ptr ptr)
@ stdcall ApplyPatchToFileW(wstr wstr wstr long)
@ stub GetFilePatchSignatureA
@ stub GetFilePatchSignatureByBuffer
@ stub GetFilePatchSignatureByHandle
@ stub GetFilePatchSignatureW
@ stub NormalizeFileForPatchSignature
@ stdcall TestApplyPatchToFileA(str str long)
@ stdcall TestApplyPatchToFileByBuffers(ptr long ptr long ptr long)
@ stdcall TestApplyPatchToFileByHandles(ptr ptr long)
@ stdcall TestApplyPatchToFileW(wstr wstr long)
