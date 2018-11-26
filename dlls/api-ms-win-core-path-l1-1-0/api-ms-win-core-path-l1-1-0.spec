@ stdcall PathAllocCanonicalize(wstr long ptr) kernelbase.PathAllocCanonicalize
@ stdcall PathAllocCombine(wstr wstr long ptr) kernelbase.PathAllocCombine
@ stdcall PathCchAddBackslash(wstr long) kernelbase.PathCchAddBackslash
@ stdcall PathCchAddBackslashEx(wstr long ptr ptr) kernelbase.PathCchAddBackslashEx
@ stdcall PathCchAddExtension(wstr long wstr) kernelbase.PathCchAddExtension
@ stdcall PathCchAppend(wstr long wstr) kernelbase.PathCchAppend
@ stdcall PathCchAppendEx(wstr long wstr long) kernelbase.PathCchAppendEx
@ stdcall PathCchCanonicalize(ptr long wstr) kernelbase.PathCchCanonicalize
@ stdcall PathCchCanonicalizeEx(ptr long wstr long) kernelbase.PathCchCanonicalizeEx
@ stdcall PathCchCombine(ptr long wstr wstr) kernelbase.PathCchCombine
@ stdcall PathCchCombineEx(ptr long wstr wstr long) kernelbase.PathCchCombineEx
@ stdcall PathCchFindExtension(wstr long ptr) kernelbase.PathCchFindExtension
@ stdcall PathCchIsRoot(wstr) kernelbase.PathCchIsRoot
@ stdcall PathCchRemoveBackslash(wstr long) kernelbase.PathCchRemoveBackslash
@ stdcall PathCchRemoveBackslashEx(wstr long ptr ptr) kernelbase.PathCchRemoveBackslashEx
@ stdcall PathCchRemoveExtension(wstr long) kernelbase.PathCchRemoveExtension
@ stdcall PathCchRemoveFileSpec(wstr long) kernelbase.PathCchRemoveFileSpec
@ stdcall PathCchRenameExtension(wstr long wstr) kernelbase.PathCchRenameExtension
@ stdcall PathCchSkipRoot(wstr ptr) kernelbase.PathCchSkipRoot
@ stdcall PathCchStripPrefix(wstr long) kernelbase.PathCchStripPrefix
@ stdcall PathCchStripToRoot(wstr long) kernelbase.PathCchStripToRoot
@ stdcall PathIsUNCEx(wstr ptr) kernelbase.PathIsUNCEx
