2 stdcall SHChangeNotifyRegister(long long long long long ptr)
4 stdcall SHChangeNotifyDeregister(long)
5 stub CreateStorageItemFromPath_FullTrustCaller
6 stub CreateStorageItemFromPath_FullTrustCaller_ForPackage
7 stub GatherProviderSettings
8 stub GetUserChoiceForUrl
9 stub PathContainedByManifestedKnownFolder_FullTrustCaller_ForPackage
10 stub RegisterChangeNotifications
11 stub UnregisterChangeNotifications
27 stdcall ILSaveToStream(ptr ptr)
28 stdcall SHILCreateFromPath(ptr ptr ptr)
42 stdcall IsLFNDriveW(wstr)
68 stdcall SHGetSetSettings(ptr long long)
74 stdcall SHCreateStdEnumFmtEtc(long ptr ptr)
75 stdcall PathYetAnotherMakeUniqueName(ptr wstr wstr wstr)
77 stub @
90 stdcall SHFindFiles(ptr ptr)
95 stub @
100 stdcall SHRestricted(long)
147 stdcall SHCLSIDFromString(ptr ptr)
172 stub @
243 stub @
244 stub @
245 stub SHTestTokenMembership
264 stub @
644 stdcall SHChangeNotification_Lock(long long ptr ptr)
645 stdcall SHChangeNotification_Unlock(long)
660 stub @
680 stdcall IsUserAnAdmin()
740 stub @
750 stub @
761 stub @
764 stub @
765 stub @
781 stub @
788 stub @
789 stub @
791 stub @
814 stub @
815 stub @
816 stub @
817 stub @
818 stub @
819 stub @
823 stub @
824 stub @
825 stub @
830 stub @
844 stub @
845 stub @
846 stub ILLoadFromStreamEx
847 stub @
849 stub @
850 stub @
851 stub @
862 stub @
866 stub @
873 stub @
880 stub @
887 stub @
897 stub @
898 stub @
910 stub @
911 stub @
913 stub @
914 stub @
915 stub @
916 stub @
923 stub @
925 stub CreateStorageItemFromShellItem_FullTrustCaller_ForPackage
928 stub @
938 stub GetCachedFileUpdateInformation
942 stub CreateStorageItemFromShellItem
943 stub CreateExtrinsicPropertyStore
945 stub GetInfoForFileInUse
946 stub SHChangeNotifySuspendResume
947 stub CreateStorageProviderPropertyStore
1001 stub @
1002 stub @
1003 stub @
2000 stub @
2001 stub @
2002 stub @
2003 stub @
2004 stub @
2005 stub @
2006 stub @
2007 stub @
2008 stub @
2009 stub @

@ stub ApplyProviderSettings
@ stub AssocCreateForClasses
@ stub AssocGetDetailsOfPropKey
@ stub AssocShouldProcessUseAppToAppLaunching
@ stub BrokerAppSiloShellItemsForApplicationUserModelId
@ stub BrokerAppSiloShellItemsForPackageFamilyName
@ stub CCachedShellItem_CreateInstance
@ stub CCollectionFactory_CreateInstance
@ stub CDesktopFolder_CreateInstanceWithBindContext
@ stub CFSFolder_AdjustForSlowColumn
@ stub CFSFolder_CreateFolder
@ stub CFSFolder_IsCommonItem
@ stub CFileOperationRecorder_CreateInstance
@ stub CFreeThreadedItemContainer_CreateInstance
@ stub CMruLongList_CreateInstance
@ stub CPrivateProfileCache_Save
@ stub CRegFolder_CreateAndInit
@ stub CRegFolder_CreateInstance
@ stub CShellItemArrayAsCollection_CreateInstance
@ stub CShellItemArrayAsVirtualizedObjectArray_CreateInstance
@ stub CShellItemArrayWithCommonParent_CreateInstance
@ stub CShellItemArray_CreateInstance
@ stub CShellItem_CreateInstance
@ stub CStorageItem_GetValidatedStorageItemObject
@ stub CTaskAddDoc_Create
@ stub CViewSettings_CreateInstance
@ stub CheckSmartScreenWithAltFile
@ stub CopyDefaultLibrariesFromGroupPolicy
@ stub CreateItemArrayFromItemStore
@ stub CreateItemArrayFromObjectArray
@ stub CreateLocalizationDesktopIni
@ stub CreateSortColumnArray
@ stub CreateStorageItemFromPath_PartialTrustCaller
@ stub CreateVolatilePropertyStore
@ stub CustomStatePropertyDescription_CreateWithItemPropertyStore
@ stub CustomStatePropertyDescription_CreateWithStateIdentifier
@ stub DataAccessCaches_InvalidateForLibrary
@ stub DeserializeTextToLink
@ stub DetermineFolderDestinationParentAppID
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetActivationFactory(ptr ptr)
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllMain(long long ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stdcall DragQueryFileW(long long ptr long)
@ stub EnumShellItemsFromEnumFullIdList
@ stub GetCommandProviderForFolderType
@ stub GetFileUndoText
@ stub GetFindDataForPath
@ stub GetFindDataFromFileInformationByHandle
@ stub GetRegDataDrivenCommand
@ stub GetRegDataDrivenCommandWithAssociation
@ stub GetSelectionStateFromItemArray
@ stub GetSystemPersistedStorageItemList
@ stub GetSystemPersistedStorageItemListForUser
@ stub GetThreadFlags
@ stub Global_WindowsStorage_MaxIcons
@ stub Global_WindowsStorage_Untyped_FileClassSRWLock
@ stub Global_WindowsStorage_Untyped_MountPoint
@ stub Global_WindowsStorage_Untyped_pFileClassCacheTable
@ stub Global_WindowsStorage_Untyped_pFileHanderMap
@ stub Global_WindowsStorage_Untyped_rgshil
@ stub Global_WindowsStorage_afNotRedirected
@ stub Global_WindowsStorage_ccIcon
@ stub Global_WindowsStorage_csIconCache
@ stub Global_WindowsStorage_csSCN
@ stub Global_WindowsStorage_dwThreadBindCtx
@ stub Global_WindowsStorage_dwThreadInitializing
@ stub Global_WindowsStorage_esServerMode
@ stub Global_WindowsStorage_fEndInitialized
@ stub Global_WindowsStorage_fIconCacheHasBeenSuccessfullyCreated
@ stub Global_WindowsStorage_fIconCacheIsValid
@ stub Global_WindowsStorage_fNeedsInitBroadcast
@ stub Global_WindowsStorage_hwndSCN
@ stub Global_WindowsStorage_iLastSysIcon
@ stub Global_WindowsStorage_iLastSystemColorDepth
@ stub Global_WindowsStorage_iUseLinkPrefix
@ stub Global_WindowsStorage_lProcessClassCount
@ stub Global_WindowsStorage_lrFlags
@ stub Global_WindowsStorage_nImageManagerVersion
@ stub Global_WindowsStorage_tlsChangeClientProxy
@ stub Global_WindowsStorage_tlsIconCache
@ stub Global_WindowsStorage_tlsThreadFlags
@ stub Global_WindowsStorage_ulNextID
@ stub GrantPathAccess_FullTrustCaller_ForPackage
@ stub GrantWorkingDirectoryAccess_FullTrustCaller_ForPackage
@ stub HideExtension
@ stdcall ILAppendID(ptr ptr long)
@ stdcall ILClone(ptr)
@ stdcall ILCloneFirst(ptr)
@ stdcall ILCombine(ptr ptr)
@ stdcall ILFindChild(ptr ptr)
@ stdcall ILFindLastID(ptr)
@ stdcall ILFree(ptr)
@ stdcall ILGetNext(ptr)
@ stdcall ILGetSize(ptr)
@ stdcall ILIsEqual(ptr ptr)
@ stdcall ILIsParent(ptr ptr long)
@ stdcall ILRemoveLastID(ptr)
@ stub IsLibraryCreatedByPolicy
@ stub IsLibraryPolicyEnabled
@ stub IsNameListedUnderKey
@ stub NeverProvidedByJunction
@ stdcall PathCleanupSpec(ptr ptr)
@ stdcall PathIsExe(ptr)
@ stdcall PathMakeUniqueName(ptr long ptr ptr ptr)
@ stub QueryStorageAccess_FullTrustCaller_ForPackage
@ stub QueryStorageAccess_FullTrustCaller_ForToken
@ stub RebaseOnDriveLetter
@ stub RebaseOnVolumeID
@ stub RegistryVerbs_GetHandlerMultiSelectModel
@ stdcall SHAssocEnumHandlers(wstr long ptr)
@ stdcall SHAssocEnumHandlersForProtocolByApplication(wstr ptr ptr)
@ stdcall SHBindToFolderIDListParent(ptr ptr ptr ptr ptr)
@ stub SHBindToFolderIDListParentEx
@ stdcall SHBindToObject(ptr ptr ptr ptr ptr)
@ stdcall SHBindToParent(ptr ptr ptr ptr)
@ stdcall SHChangeNotify(long long ptr ptr)
@ stub SHChangeNotifyRegisterThread
@ stub SHCoCreateInstanceWorker
@ stub SHCreateAssocHandler
@ stdcall SHCreateAssociationRegistration(ptr ptr)
@ stdcall SHCreateDataObject(ptr long ptr ptr ptr ptr)
@ stub SHCreateDefaultExtractIcon
@ stdcall SHCreateDirectory(long ptr)
@ stdcall SHCreateDirectoryExA(long str ptr)
@ stdcall SHCreateDirectoryExW(long wstr ptr)
@ stdcall SHCreateItemFromIDList(ptr ptr ptr)
@ stdcall SHCreateItemFromParsingName(wstr ptr ptr ptr)
@ stdcall SHCreateItemFromRelativeName(ptr wstr ptr ptr ptr)
@ stdcall SHCreateItemInKnownFolder(ptr long wstr ptr ptr)
@ stdcall SHCreateItemWithParent(ptr ptr ptr ptr ptr)
@ stub SHCreateItemWithParentAndChildId
@ stdcall SHCreateShellItemArray(ptr ptr long ptr ptr)
@ stdcall SHCreateShellItemArrayFromDataObject(ptr ptr ptr)
@ stdcall SHCreateShellItemArrayFromIDLists(long ptr ptr)
@ stdcall SHCreateShellItemArrayFromShellItem(ptr ptr ptr)
@ stub SHCreateShellItemArrayWithFolderParent
@ stub SHFileOperationWithAdditionalFlags
@ stdcall SHFlushSFCache()
@ stdcall SHGetDesktopFolder(ptr)
@ stdcall SHGetFileInfoW(wstr long ptr long long)
@ stdcall SHGetFolderLocation(long long long long ptr)
@ stdcall SHGetFolderPathA(long long long long ptr)
@ stdcall SHGetFolderPathAndSubDirA(long long long long str ptr)
@ stdcall SHGetFolderPathAndSubDirW(long long long long wstr ptr)
@ stdcall SHGetFolderPathEx(ptr long ptr ptr long)
@ stdcall SHGetFolderPathW(long long long long ptr)
@ stdcall SHGetIDListFromObject(ptr ptr)
@ stdcall SHGetInstanceExplorer(ptr)
@ stdcall SHGetItemFromObject(ptr ptr ptr)
@ stdcall SHGetKnownFolderIDList(ptr long ptr ptr)
@ stub SHGetKnownFolderIDList_Internal
@ stdcall SHGetKnownFolderItem(ptr long long ptr ptr)
@ stdcall SHGetKnownFolderPath(ptr long ptr ptr)
@ stdcall SHGetNameFromIDList(ptr long ptr)
@ stdcall SHGetPathFromIDListEx(ptr ptr long long)
@ stdcall SHGetPathFromIDListW(ptr ptr)
@ stdcall SHGetSpecialFolderLocation(long long ptr)
@ stdcall SHGetSpecialFolderPathA(long ptr long long)
@ stdcall SHGetSpecialFolderPathW(long ptr long long)
@ stdcall SHGetStockIconInfo(long long ptr)
@ stub SHGetTemporaryPropertyForItem
@ stdcall SHHandleUpdateImage(ptr)
@ stub SHKnownFolderFromCSIDL
@ stub SHKnownFolderToCSIDL
@ stdcall SHOpenFolderAndSelectItems(ptr long ptr long)
@ stdcall SHParseDisplayName(wstr ptr ptr long ptr)
@ stub SHPrepareKnownFoldersCommon
@ stub SHPrepareKnownFoldersUser
@ stub SHResolveLibrary
@ stub SHSetFolderPathA
@ stub SHSetFolderPathW
@ stub SHSetKnownFolderPath
@ stdcall SHSetLocalizedName(wstr wstr long)
@ stdcall SHSetTemporaryPropertyForItem(ptr ptr ptr)
@ stub SHSysErrorMessageBox
@ stdcall SHUpdateImageA(str long long long)
@ stdcall SHUpdateImageW(wstr long long long)
@ stub STORAGE_AddItemToRecentDocs
@ stub STORAGE_AddNewFolderToFrequentPlaces
@ stub STORAGE_CEnumFiles_CreateInstance
@ stub STORAGE_CStatusProvider_CreateInstance
@ stub STORAGE_CStorageItem_GetValidatedStorageItem
@ stub STORAGE_CStorageItem_GetValidatedStorageItemObject
@ stub STORAGE_ClearDestinationsForAllApps
@ stub STORAGE_CreateSortColumnArrayFromListDesc
@ stub STORAGE_CreateStorageItemFromPath_FullTrustCaller
@ stub STORAGE_CreateStorageItemFromPath_FullTrustCaller_ForPackage
@ stub STORAGE_CreateStorageItemFromPath_PartialTrustCaller
@ stub STORAGE_CreateStorageItemFromShellItem_FullTrustCaller
@ stub STORAGE_CreateStorageItemFromShellItem_FullTrustCaller_ForPackage
@ stub STORAGE_CreateStorageItemFromShellItem_FullTrustCaller_ForPackage_WithProcessHandle
@ stub STORAGE_CreateStorageItemFromShellItem_FullTrustCaller_ForPackage_WithProcessHandleAndSecondaryStreamName
@ stub STORAGE_CreateStorageItemFromShellItem_FullTrustCaller_UseImplicitFlagsAndPackage
@ stub STORAGE_FillResultWithNullForKeys
@ stub STORAGE_GetShellItemFromStorageItem
@ stub STORAGE_GetSystemPersistedStorageItemList
@ stub STORAGE_MakeDestinationItem
@ stub STORAGE_PathIsEqualOrSubFolderOfKnownFolders
@ stub STORAGE_SHAddToRecentDocs
@ stub STORAGE_SHAddToRecentDocsEx
@ stub STORAGE_SHConfirmOperation
@ stub STORAGE_SHCreateDirectory
@ stub STORAGE_SHCreateDirectoryExA
@ stub STORAGE_SHCreateDirectoryExWWorker
@ stub STORAGE_SHCreateShellItemArray
@ stub STORAGE_SHCreateShellItemArrayFromDataObject
@ stub STORAGE_SHCreateShellItemArrayFromIDLists
@ stub STORAGE_SHCreateShellItemArrayFromShellItem
@ stub STORAGE_SHFileOperation
@ stub STORAGE_SHFileOperationA
@ stub STORAGE_SHFreeNameMappings
@ stub STORAGE_SHGetDesktopFolderWorker
@ stub STORAGE_SHGetPathFromMsUri
@ stub STORAGE_SHOpenFolderAndSelectItems
@ stub STORAGE_SHPathPrepareForWriteA
@ stub STORAGE_SHPathPrepareForWriteW
@ stub STORAGE_SHValidateMSUri
@ stub SendNotificationsForLibraryItem
@ stub SerializeLinkToText
@ stub SetThreadFlags
@ stdcall ShellExecuteA(long str str str str long)
@ stdcall ShellExecuteExW(long)
@ stdcall ShellExecuteW(long wstr wstr wstr wstr long)
@ stub StateRepoVerbsCache_Destroy
@ stub StateRepoVerbsCache_GetContextMenuVerbs
@ stub StateRepoVerbsCache_RebuildCacheAsync
@ stub StorageItemHelpers_IsSupportedRemovablePath
@ stub Storage_Internal_GetAccessListForPackage
@ stub _CleanRecentDocs
@ stub _PredictReasonableImpact
