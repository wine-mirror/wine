@ stub TTCharToUnicode
@ stdcall TTDeleteEmbeddedFont(long long ptr)
@ stdcall TTEmbedFont(long long long ptr ptr ptr ptr ptr long long ptr)
@ stub TTEmbedFontFromFileA
@ stub TTEnableEmbeddingForFacename
@ stub TTGetEmbeddedFontInfo
@ stdcall TTGetEmbeddingType(long ptr)
@ stdcall TTIsEmbeddingEnabled(long ptr)
@ stdcall TTIsEmbeddingEnabledForFacename(str ptr)
@ stdcall TTLoadEmbeddedFont(ptr long ptr long ptr ptr ptr wstr str ptr)
@ stub TTRunValidationTests
@ stub _TTCharToUnicode@24
@ stub _TTDeleteEmbeddedFont@12
@ stdcall _TTEmbedFont@44(long long long ptr ptr ptr ptr ptr long long ptr) TTEmbedFont
@ stub _TTEmbedFontFromFileA@52
@ stub _TTEnableEmbeddingForFacename@8
@ stub _TTGetEmbeddedFontInfo@28
@ stdcall _TTGetEmbeddingType@8(long ptr) TTGetEmbeddingType
@ stdcall _TTIsEmbeddingEnabled@8(long ptr) TTIsEmbeddingEnabled
@ stdcall _TTIsEmbeddingEnabledForFacename@8(str ptr) TTIsEmbeddingEnabledForFacename
@ stdcall _TTLoadEmbeddedFont@40(ptr long ptr long ptr ptr ptr wstr str ptr) TTLoadEmbeddedFont
@ stub _TTRunValidationTests@8
@ stub TTEmbedFontEx
@ stub TTRunValidationTestsEx
@ stub TTGetNewFontName
