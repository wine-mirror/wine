@ stdcall CM_Connect_MachineW(wstr ptr) CM_Connect_MachineW
@ stdcall CM_Disconnect_Machine(long) CM_Disconnect_Machine
@ stub CM_Free_Log_Conf_Handle
@ stub CM_Free_Res_Des_Handle
@ stub CM_Get_DevNode_Status_Ex
@ stub CM_Get_Device_ID_ExW
@ stub CM_Get_First_Log_Conf_Ex
@ stub CM_Get_Next_Res_Des_Ex
@ stub CM_Get_Res_Des_Data_Ex
@ stub CM_Get_Res_Des_Data_Size_Ex
@ stub CM_Locate_DevNode_ExW
@ stub CM_Reenumerate_DevNode_Ex
@ stub CaptureAndConvertAnsiArg
@ stub CaptureStringArg
@ stub CenterWindowRelativeToParent
@ stub ConcatenatePaths
@ stub DelayedMove
@ stub DelimStringToMultiSz
@ stub DestroyTextFileReadBuffer
@ stub DoesUserHavePrivilege
@ stub DuplicateString
@ stub EnablePrivilege
@ stub ExtensionPropSheetPageProc
@ stub FileExists
@ stub FreeStringArray
@ stub GetNewInfName
@ stub GetSetFileTimestamp
@ stub GetVersionInfoFromImage
@ stub InfIsFromOemLocation
@ stub InstallHinfSection
@ stub InstallHinfSectionA
@ stub InstallHinfSectionW
@ stub InstallStop
@ stub IsUserAdmin
@ stub LookUpStringInTable
@ stub MemoryInitialize
@ stub MultiByteToUnicode
@ stub MultiSzFromSearchControl
@ stub MyFree
@ stub MyGetFileTitle
@ stub MyMalloc
@ stub MyRealloc
@ stub OpenAndMapFileForRead
@ stub OutOfMemory
@ stub QueryMultiSzValueToArray
@ stub QueryRegistryValue
@ stub ReadAsciiOrUnicodeTextFile
@ stub RegistryDelnode
@ stub RetreiveFileSecurity
@ stub RetrieveServiceConfig
@ stub SearchForInfFile
@ stub SetArrayToMultiSzValue
@ stub SetupAddInstallSectionToDiskSpaceListA
@ stub SetupAddInstallSectionToDiskSpaceListW
@ stub SetupAddSectionToDiskSpaceListA
@ stub SetupAddSectionToDiskSpaceListW
@ stub SetupAddToDiskSpaceListA
@ stub SetupAddToDiskSpaceListW
@ stub SetupAddToSourceListA
@ stub SetupAddToSourceListW
@ stub SetupAdjustDiskSpaceListA
@ stub SetupAdjustDiskSpaceListW
@ stub SetupCancelTemporarySourceList
@ stdcall SetupCloseFileQueue(ptr) SetupCloseFileQueue
@ stdcall SetupCloseInfFile(long) SetupCloseInfFile
@ stub SetupCommitFileQueue
@ stdcall SetupCommitFileQueueA(long long ptr ptr) SetupCommitFileQueueA
@ stdcall SetupCommitFileQueueW(long long ptr ptr) SetupCommitFileQueueW
@ stub SetupCopyErrorA
@ stub SetupCopyErrorW
@ stub SetupCreateDiskSpaceListA
@ stub SetupCreateDiskSpaceListW
@ stub SetupDecompressOrCopyFileA
@ stub SetupDecompressOrCopyFileW
@ stub SetupDefaultQueueCallback
@ stdcall SetupDefaultQueueCallbackA(ptr long long long) SetupDefaultQueueCallbackA
@ stdcall SetupDefaultQueueCallbackW(ptr long long long) SetupDefaultQueueCallbackW
@ stub SetupDeleteErrorA
@ stub SetupDeleteErrorW
@ stub SetupDestroyDiskSpaceList
@ stub SetupDiAskForOEMDisk
@ stub SetupDiBuildClassInfoList
@ stdcall SetupDiBuildClassInfoListExW(long ptr long ptr wstr ptr) SetupDiBuildClassInfoListExW
@ stub SetupDiBuildDriverInfoList
@ stub SetupDiCallClassInstaller
@ stub SetupDiCancelDriverInfoSearch
@ stub SetupDiChangeState
@ stub SetupDiClassGuidsFromNameA
@ stdcall SetupDiClassGuidsFromNameExW(wstr ptr long ptr wstr ptr) SetupDiClassGuidsFromNameExW
@ stub SetupDiClassGuidsFromNameW
@ stub SetupDiClassNameFromGuidA
@ stdcall SetupDiClassNameFromGuidExW(ptr ptr long ptr wstr ptr) SetupDiClassNameFromGuidExW
@ stub SetupDiClassNameFromGuidW
@ stub SetupDiCreateDevRegKeyA
@ stub SetupDiCreateDevRegKeyW
@ stub SetupDiCreateDeviceInfoA
@ stub SetupDiCreateDeviceInfoList
@ stdcall SetupDiCreateDeviceInfoListExW(ptr long str ptr)  SetupDiCreateDeviceInfoListExW
@ stub SetupDiCreateDeviceInfoW
@ stub SetupDiDeleteDevRegKey
@ stub SetupDiDeleteDeviceInfo
@ stub SetupDiDestroyClassImageList
@ stdcall SetupDiDestroyDeviceInfoList(long) SetupDiDestroyDeviceInfoList
@ stub SetupDiDestroyDriverInfoList
@ stub SetupDiDrawMiniIcon
@ stdcall SetupDiEnumDeviceInfo(long long ptr) SetupDiEnumDeviceInfo
@ stdcall SetupDiEnumDeviceInterfaces(long ptr ptr long ptr) SetupDiEnumDeviceInterfaces
@ stub SetupDiEnumDriverInfoA
@ stub SetupDiEnumDriverInfoW
@ stub SetupDiGetActualSectionToInstallA
@ stub SetupDiGetActualSectionToInstallW
@ stub SetupDiGetClassBitmapIndex
@ stub SetupDiGetClassDescriptionA
@ stdcall SetupDiGetClassDescriptionExW(ptr ptr long ptr wstr ptr) SetupDiGetClassDescriptionExW
@ stub SetupDiGetClassDescriptionW
@ stub SetupDiGetClassDevPropertySheetsA
@ stub SetupDiGetClassDevPropertySheetsW
@ stdcall SetupDiGetClassDevsA(ptr ptr long long) SetupDiGetClassDevsA
@ stdcall SetupDiGetClassDevsExW(ptr wstr ptr long ptr wstr ptr) SetupDiGetClassDevsExW
@ stdcall SetupDiGetClassDevsW(ptr ptr long long) SetupDiGetClassDevsW
@ stub SetupDiGetClassImageIndex
@ stub SetupDiGetClassImageList
@ stub SetupDiGetClassInstallParamsA
@ stub SetupDiGetClassInstallParamsW
@ stub SetupDiGetDeviceInfoListClass
@ stdcall SetupDiGetDeviceInfoListDetailW(ptr ptr) SetupDiGetDeviceInfoListDetailW
@ stub SetupDiGetDeviceInstallParamsA
@ stub SetupDiGetDeviceInstallParamsW
@ stub SetupDiGetDeviceInstanceIdA
@ stub SetupDiGetDeviceInstanceIdW
@ stdcall SetupDiGetDeviceRegistryPropertyA(long ptr long ptr ptr long ptr) SetupDiGetDeviceRegistryPropertyA
@ stub SetupDiGetDeviceRegistryPropertyW
@ stub SetupDiGetDriverInfoDetailA
@ stub SetupDiGetDriverInfoDetailW
@ stub SetupDiGetDriverInstallParamsA
@ stub SetupDiGetDriverInstallParamsW
@ stub SetupDiGetDeviceInterfaceAlias
@ stdcall SetupDiGetDeviceInterfaceDetailA(long ptr ptr long ptr ptr) SetupDiGetDeviceInterfaceDetailA
@ stdcall SetupDiGetDeviceInterfaceDetailW(long ptr ptr long ptr ptr) SetupDiGetDeviceInterfaceDetailW
@ stub SetupDiGetHwProfileFriendlyNameA
@ stub SetupDiGetHwProfileFriendlyNameW
@ stub SetupDiGetHwProfileList
@ stub SetupDiGetINFClassA
@ stub SetupDiGetINFClassW
@ stub SetupDiGetSelectedDevice
@ stub SetupDiGetSelectedDriverA
@ stub SetupDiGetSelectedDriverW
@ stub SetupDiGetWizardPage
@ stub SetupDiInstallClassA
@ stub SetupDiInstallClassW
@ stub SetupDiInstallDevice
@ stub SetupDiInstallDriverFiles
@ stub SetupDiLoadClassIcon
@ stub SetupDiMoveDuplicateDevice
@ stub SetupDiOpenClassRegKey
@ stdcall SetupDiOpenClassRegKeyExW(ptr long long wstr ptr) SetupDiOpenClassRegKeyExW
@ stub SetupDiOpenDevRegKey
@ stub SetupDiOpenDeviceInfoA
@ stub SetupDiOpenDeviceInfoW
@ stub SetupDiRegisterDeviceInfo
@ stub SetupDiRemoveDevice
@ stub SetupDiSelectDevice
@ stub SetupDiSelectOEMDrv
@ stub SetupDiSetClassInstallParamsA
@ stub SetupDiSetClassInstallParamsW
@ stub SetupDiSetDeviceInstallParamsA
@ stub SetupDiSetDeviceInstallParamsW
@ stub SetupDiSetDeviceRegistryPropertyA
@ stub SetupDiSetDeviceRegistryPropertyW
@ stub SetupDiSetDriverInstallParamsA
@ stub SetupDiSetDriverInstallParamsW
@ stub SetupDiSetSelectedDevice
@ stub SetupDiSetSelectedDriverA
@ stub SetupDiSetSelectedDriverW
@ stub SetupDuplicateDiskSpaceListA
@ stub SetupDuplicateDiskSpaceListW
@ stdcall SetupFindFirstLineA(long str str ptr) SetupFindFirstLineA
@ stdcall SetupFindFirstLineW(long wstr wstr ptr) SetupFindFirstLineW
@ stdcall SetupFindNextLine(ptr ptr) SetupFindNextLine
@ stdcall SetupFindNextMatchLineA(ptr str ptr) SetupFindNextMatchLineA
@ stdcall SetupFindNextMatchLineW(ptr wstr ptr) SetupFindNextMatchLineW
@ stub SetupFreeSourceListA
@ stub SetupFreeSourceListW
@ stdcall SetupGetBinaryField(ptr long ptr long ptr) SetupGetBinaryField
@ stdcall SetupGetFieldCount(ptr) SetupGetFieldCount
@ stub SetupGetFileCompressionInfoA
@ stub SetupGetFileCompressionInfoW
@ stdcall SetupGetFileQueueCount(long long ptr) SetupGetFileQueueCount
@ stdcall SetupGetFileQueueFlags(long ptr) SetupGetFileQueueFlags
@ stub SetupGetInfFileListA
@ stub SetupGetInfFileListW
@ stub SetupGetInfInformationA
@ stub SetupGetInfInformationW
@ stdcall SetupGetIntField(ptr long ptr) SetupGetIntField
@ stdcall SetupGetLineByIndexA(long str long ptr) SetupGetLineByIndexA
@ stdcall SetupGetLineByIndexW(long wstr long ptr) SetupGetLineByIndexW
@ stdcall SetupGetLineCountA(long str) SetupGetLineCountA
@ stdcall SetupGetLineCountW(long wstr) SetupGetLineCountW
@ stdcall SetupGetLineTextA(ptr long str str ptr long ptr) SetupGetLineTextA
@ stdcall SetupGetLineTextW(ptr long wstr wstr ptr long ptr) SetupGetLineTextW
@ stdcall SetupGetMultiSzFieldA(ptr long ptr long ptr) SetupGetMultiSzFieldA
@ stdcall SetupGetMultiSzFieldW(ptr long ptr long ptr) SetupGetMultiSzFieldW
@ stub SetupGetSourceFileLocationA
@ stub SetupGetSourceFileLocationW
@ stub SetupGetSourceFileSizeA
@ stub SetupGetSourceFileSizeW
@ stub SetupGetSourceInfoA
@ stub SetupGetSourceInfoW
@ stdcall SetupGetStringFieldA(ptr long ptr long ptr) SetupGetStringFieldA
@ stdcall SetupGetStringFieldW(ptr long ptr long ptr) SetupGetStringFieldW
@ stub SetupGetTargetPathA
@ stub SetupGetTargetPathW
@ stdcall SetupInitDefaultQueueCallback(long) SetupInitDefaultQueueCallback
@ stdcall SetupInitDefaultQueueCallbackEx(long long long long ptr) SetupInitDefaultQueueCallbackEx
@ stub SetupInitializeFileLogA
@ stub SetupInitializeFileLogW
@ stub SetupInstallFileA
@ stub SetupInstallFileExA
@ stub SetupInstallFileExW
@ stub SetupInstallFileW
@ stdcall SetupInstallFilesFromInfSectionA(long long long str str long) SetupInstallFilesFromInfSectionA
@ stdcall SetupInstallFilesFromInfSectionW(long long long wstr wstr long) SetupInstallFilesFromInfSectionW
@ stdcall SetupInstallFromInfSectionA(long long str long long str long ptr ptr long ptr) SetupInstallFromInfSectionA
@ stdcall SetupInstallFromInfSectionW(long long wstr long long wstr long ptr ptr long ptr) SetupInstallFromInfSectionW
@ stub SetupInstallServicesFromInfSectionA
@ stub SetupInstallServicesFromInfSectionW
@ stdcall SetupIterateCabinetA(str long ptr ptr) SetupIterateCabinetA
@ stdcall SetupIterateCabinetW(wstr long ptr ptr) SetupIterateCabinetW
@ stub SetupLogFileA
@ stub SetupLogFileW
@ stdcall SetupOpenAppendInfFileA(str long ptr) SetupOpenAppendInfFileA
@ stdcall SetupOpenAppendInfFileW(wstr long ptr) SetupOpenAppendInfFileW
@ stdcall SetupOpenFileQueue() SetupOpenFileQueue
@ stdcall SetupOpenInfFileA(str str long ptr) SetupOpenInfFileA
@ stdcall SetupOpenInfFileW(wstr wstr long ptr) SetupOpenInfFileW
@ stub SetupOpenMasterInf
@ stub SetupPromptForDiskA
@ stub SetupPromptForDiskW
@ stub SetupPromptReboot
@ stub SetupQueryDrivesInDiskSpaceListA
@ stub SetupQueryDrivesInDiskSpaceListW
@ stub SetupQueryFileLogA
@ stub SetupQueryFileLogW
@ stub SetupQueryInfFileInformationA
@ stub SetupQueryInfFileInformationW
@ stub SetupQueryInfVersionInformationA
@ stub SetupQueryInfVersionInformationW
@ stub SetupQuerySourceListA
@ stub SetupQuerySourceListW
@ stub SetupQuerySpaceRequiredOnDriveA
@ stub SetupQuerySpaceRequiredOnDriveW
@ stdcall SetupQueueCopyA(long str str str str str str str long) SetupQueueCopyA
@ stdcall SetupQueueCopyIndirectA(ptr) SetupQueueCopyIndirectA
@ stdcall SetupQueueCopyIndirectW(ptr) SetupQueueCopyIndirectW
@ stdcall SetupQueueCopySectionA(long str long long str long) SetupQueueCopySectionA
@ stdcall SetupQueueCopySectionW(long wstr long long wstr long) SetupQueueCopySectionW
@ stdcall SetupQueueCopyW(long wstr wstr wstr wstr wstr wstr wstr long) SetupQueueCopyW
@ stdcall SetupQueueDefaultCopyA(long long str str str long) SetupQueueDefaultCopyA
@ stdcall SetupQueueDefaultCopyW(long long wstr wstr wstr long) SetupQueueDefaultCopyW
@ stdcall SetupQueueDeleteA(long str str) SetupQueueDeleteA
@ stdcall SetupQueueDeleteSectionA(long long long str) SetupQueueDeleteSectionA
@ stdcall SetupQueueDeleteSectionW(long long long wstr) SetupQueueDeleteSectionW
@ stdcall SetupQueueDeleteW(long wstr wstr) SetupQueueDeleteW
@ stdcall SetupQueueRenameA(long str str str str) SetupQueueRenameA
@ stdcall SetupQueueRenameSectionA(long long long str) SetupQueueRenameSectionA
@ stdcall SetupQueueRenameSectionW(long long long wstr) SetupQueueRenameSectionW
@ stdcall SetupQueueRenameW(long wstr wstr wstr wstr) SetupQueueRenameW
@ stub SetupRemoveFileLogEntryA
@ stub SetupRemoveFileLogEntryW
@ stub SetupRemoveFromDiskSpaceListA
@ stub SetupRemoveFromDiskSpaceListW
@ stub SetupRemoveFromSourceListA
@ stub SetupRemoveFromSourceListW
@ stub SetupRemoveInstallSectionFromDiskSpaceListA
@ stub SetupRemoveInstallSectionFromDiskSpaceListW
@ stub SetupRemoveSectionFromDiskSpaceListA
@ stub SetupRemoveSectionFromDiskSpaceListW
@ stub SetupRenameErrorA
@ stub SetupRenameErrorW
@ stub SetupScanFileQueue
@ stdcall SetupScanFileQueueA(long long long ptr ptr ptr) SetupScanFileQueueA
@ stdcall SetupScanFileQueueW(long long long ptr ptr ptr) SetupScanFileQueueW
@ stdcall SetupSetDirectoryIdA(long long str) SetupSetDirectoryIdA
@ stub SetupSetDirectoryIdExA
@ stub SetupSetDirectoryIdExW
@ stdcall SetupSetDirectoryIdW(long long wstr) SetupSetDirectoryIdW
@ stdcall SetupSetFileQueueFlags(long long long) SetupSetFileQueueFlags
@ stub SetupSetPlatformPathOverrideA
@ stub SetupSetPlatformPathOverrideW
@ stub SetupSetSourceListA
@ stub SetupSetSourceListW
@ stdcall SetupTermDefaultQueueCallback(ptr) SetupTermDefaultQueueCallback
@ stub SetupTerminateFileLog
@ stub ShouldDeviceBeExcluded
@ stub StampFileSecurity
@ stub StringTableAddString
@ stub StringTableAddStringEx
@ stub StringTableDestroy
@ stub StringTableDuplicate
@ stub StringTableEnum
@ stub StringTableGetExtraData
@ stub StringTableInitialize
@ stub StringTableInitializeEx
@ stub StringTableLookUpString
@ stub StringTableLookUpStringEx
@ stub StringTableSetExtraData
@ stub StringTableStringFromId
@ stub StringTableTrim
@ stub TakeOwnershipOfFile
@ stub UnicodeToMultiByte
@ stub UnmapAndCloseFile
@ stub pSetupAddMiniIconToList
@ stub pSetupAddTagToGroupOrderListEntry
@ stub pSetupAppendStringToMultiSz
@ stub pSetupDirectoryIdToPath
@ stub pSetupGetField
@ stub pSetupGetOsLoaderDriveAndPath
@ stub pSetupGetVersionDatum
@ stub pSetupGuidFromString
@ stub pSetupIsGuidNull
@ stub pSetupMakeSurePathExists
@ stub pSetupStringFromGuid
