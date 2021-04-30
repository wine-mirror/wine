@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllMain(long long ptr)
@ stub WIMApplyImage
@ stub WIMCaptureImage
@ stub WIMCloseHandle
@ stub WIMCommitImageHandle
@ stub WIMCopyFile
@ stdcall WIMCreateFile(wstr long long long long ptr)
@ stub WIMCreateImageFile
@ stub WIMDeleteImage
@ stub WIMDeleteImageMounts
@ stub WIMEnumImageFiles
@ stub WIMExportImage
@ stub WIMExtractImagePath
@ stub WIMFindFirstImageFile
@ stub WIMFindNextImageFile
@ stub WIMGetAttributes
@ stub WIMGetImageCount
@ stub WIMGetImageInformation
@ stub WIMGetMessageCallbackCount
@ stub WIMGetMountedImageHandle
@ stub WIMGetMountedImageInfo
@ stub WIMGetMountedImageInfoFromFile
@ stdcall WIMGetMountedImages(ptr ptr)
@ stub WIMInitFileIOCallbacks
@ stub WIMLoadImage
@ stub WIMMountImage
@ stub WIMMountImageHandle
@ stub WIMReadImageFile
@ stub WIMRegisterLogFile
@ stdcall WIMRegisterMessageCallback(long ptr ptr)
@ stub WIMRemountImage
@ stub WIMSetBootImage
@ stub WIMSetFileIOCallbackTemporaryPath
@ stub WIMSetImageInformation
@ stub WIMSetReferenceFile
@ stub WIMSetTemporaryFile
@ stub WIMSplitFile
@ stub WIMUmountImage
@ stub WIMUmountImageHandle
@ stub WIMUnregisterLogFile
@ stub WIMUnregisterMessageCallback
