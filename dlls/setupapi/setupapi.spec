@ stub AcquireSCMLock
@ stub AddMiniIconToList
@ stub AddTagToGroupOrderListEntry
@ stub AppendStringToMultiSz
@ stdcall AssertFail(str long str)
@ stub CMP_Init_Detection
@ stub CMP_RegisterNotification
@ stub CMP_Report_LogOn
@ stub CMP_UnregisterNotification
@ stdcall CMP_WaitNoPendingInstallEvents(long)
@ stub CMP_WaitServices
@ stub CM_Add_Empty_Log_Conf
@ stub CM_Add_Empty_Log_Conf_Ex
@ stub CM_Add_IDA
@ stub CM_Add_IDW
@ stub CM_Add_ID_ExA
@ stub CM_Add_ID_ExW
@ stub CM_Add_Range
@ stub CM_Add_Res_Des
@ stub CM_Add_Res_Des_Ex
@ stdcall CM_Connect_MachineA(str ptr)
@ stdcall CM_Connect_MachineW(wstr ptr)
@ stdcall CM_Create_DevNodeA(ptr str long long)
@ stdcall CM_Create_DevNodeW(ptr wstr long long)
@ stub CM_Create_DevNode_ExA
@ stub CM_Create_DevNode_ExW
@ stub CM_Create_Range_List
@ stub CM_Delete_Class_Key
@ stub CM_Delete_Class_Key_Ex
@ stub CM_Delete_DevNode_Key
@ stub CM_Delete_DevNode_Key_Ex
@ stub CM_Delete_Range
@ stub CM_Detect_Resource_Conflict
@ stub CM_Detect_Resource_Conflict_Ex
@ stub CM_Disable_DevNode
@ stub CM_Disable_DevNode_Ex
@ stdcall CM_Disconnect_Machine(long)
@ stub CM_Dup_Range_List
@ stub CM_Enable_DevNode
@ stub CM_Enable_DevNode_Ex
@ stdcall CM_Enumerate_Classes(long ptr long)
@ stub CM_Enumerate_Classes_Ex
@ stub CM_Enumerate_EnumeratorsA
@ stub CM_Enumerate_EnumeratorsW
@ stub CM_Enumerate_Enumerators_ExA
@ stub CM_Enumerate_Enumerators_ExW
@ stub CM_Find_Range
@ stub CM_First_Range
@ stub CM_Free_Log_Conf
@ stub CM_Free_Log_Conf_Ex
@ stub CM_Free_Log_Conf_Handle
@ stub CM_Free_Range_List
@ stub CM_Free_Res_Des
@ stub CM_Free_Res_Des_Ex
@ stub CM_Free_Res_Des_Handle
@ stdcall CM_Get_Child(ptr long long)
@ stdcall CM_Get_Child_Ex(ptr long long ptr)
@ stub CM_Get_Class_Key_NameA
@ stub CM_Get_Class_Key_NameW
@ stub CM_Get_Class_Key_Name_ExA
@ stub CM_Get_Class_Key_Name_ExW
@ stub CM_Get_Class_NameA
@ stub CM_Get_Class_NameW
@ stub CM_Get_Class_Name_ExA
@ stub CM_Get_Class_Name_ExW
@ stdcall CM_Get_Class_Registry_PropertyA(ptr long ptr ptr long long ptr)
@ stdcall CM_Get_Class_Registry_PropertyW(ptr long ptr ptr long long ptr)
@ stub CM_Get_Depth
@ stub CM_Get_Depth_Ex
@ stdcall CM_Get_DevNode_PropertyW(long ptr ptr ptr ptr long)
@ stdcall CM_Get_DevNode_Property_ExW(long ptr ptr ptr ptr long ptr)
@ stdcall CM_Get_DevNode_Registry_PropertyA(long long ptr ptr ptr long)
@ stdcall CM_Get_DevNode_Registry_PropertyW(long long ptr ptr ptr long)
@ stdcall CM_Get_DevNode_Registry_Property_ExA(long long ptr ptr ptr long ptr)
@ stdcall CM_Get_DevNode_Registry_Property_ExW(long long ptr ptr ptr long ptr)
@ stdcall CM_Get_DevNode_Status(ptr ptr long long)
@ stdcall CM_Get_DevNode_Status_Ex(ptr ptr long long ptr)
@ stdcall CM_Get_Device_IDA(ptr ptr long long)
@ stdcall CM_Get_Device_IDW(ptr ptr long long)
@ stdcall CM_Get_Device_ID_ExA(ptr ptr long long ptr)
@ stdcall CM_Get_Device_ID_ExW(ptr ptr long long ptr)
@ stdcall CM_Get_Device_ID_ListA(str ptr long long)
@ stdcall CM_Get_Device_ID_ListW(wstr ptr long long)
@ stdcall CM_Get_Device_ID_List_ExA(str ptr long long ptr)
@ stdcall CM_Get_Device_ID_List_ExW(wstr ptr long long ptr)
@ stdcall CM_Get_Device_ID_List_SizeA(ptr str long)
@ stdcall CM_Get_Device_ID_List_SizeW(ptr wstr long)
@ stdcall CM_Get_Device_ID_List_Size_ExA(ptr str long ptr)
@ stdcall CM_Get_Device_ID_List_Size_ExW(ptr wstr long ptr)
@ stdcall CM_Get_Device_ID_Size(ptr ptr long)
@ stub CM_Get_Device_ID_Size_Ex
@ stdcall CM_Get_Device_Interface_AliasA(str ptr ptr ptr long)
@ stdcall CM_Get_Device_Interface_AliasW(wstr ptr ptr ptr long)
@ stub CM_Get_Device_Interface_Alias_ExA
@ stub CM_Get_Device_Interface_Alias_ExW
@ stdcall CM_Get_Device_Interface_ListA(ptr ptr ptr long long)
@ stdcall CM_Get_Device_Interface_ListW(ptr ptr ptr long long)
@ stdcall CM_Get_Device_Interface_List_ExA(ptr ptr ptr long long ptr)
@ stdcall CM_Get_Device_Interface_List_ExW(ptr ptr ptr long long ptr)
@ stdcall CM_Get_Device_Interface_List_SizeA(ptr ptr str long)
@ stdcall CM_Get_Device_Interface_List_SizeW(ptr ptr wstr long)
@ stdcall CM_Get_Device_Interface_List_Size_ExA(ptr ptr str long ptr)
@ stdcall CM_Get_Device_Interface_List_Size_ExW(ptr ptr wstr long ptr)
@ stub CM_Get_First_Log_Conf
@ stub CM_Get_First_Log_Conf_Ex
@ stub CM_Get_Global_State
@ stub CM_Get_Global_State_Ex
@ stub CM_Get_HW_Prof_FlagsA
@ stub CM_Get_HW_Prof_FlagsW
@ stub CM_Get_HW_Prof_Flags_ExA
@ stub CM_Get_HW_Prof_Flags_ExW
@ stub CM_Get_Hardware_Profile_InfoA
@ stub CM_Get_Hardware_Profile_InfoW
@ stub CM_Get_Hardware_Profile_Info_ExA
@ stub CM_Get_Hardware_Profile_Info_ExW
@ stub CM_Get_Log_Conf_Priority
@ stub CM_Get_Log_Conf_Priority_Ex
@ stub CM_Get_Next_Log_Conf
@ stub CM_Get_Next_Log_Conf_Ex
@ stub CM_Get_Next_Res_Des
@ stub CM_Get_Next_Res_Des_Ex
@ stdcall CM_Get_Parent(ptr long long)
@ stub CM_Get_Parent_Ex
@ stub CM_Get_Res_Des_Data
@ stub CM_Get_Res_Des_Data_Ex
@ stub CM_Get_Res_Des_Data_Size
@ stub CM_Get_Res_Des_Data_Size_Ex
@ stdcall CM_Get_Sibling(ptr long long)
@ stdcall CM_Get_Sibling_Ex(ptr long long ptr)
@ stdcall CM_Get_Version()
@ stub CM_Get_Version_Ex
@ stub CM_Intersect_Range_List
@ stub CM_Invert_Range_List
@ stub CM_Is_Dock_Station_Present
@ stdcall CM_Locate_DevNodeA(ptr str long)
@ stdcall CM_Locate_DevNodeW(ptr wstr long)
@ stdcall CM_Locate_DevNode_ExA(ptr str long long)
@ stdcall CM_Locate_DevNode_ExW(ptr wstr long long)
@ stub CM_Merge_Range_List
@ stub CM_Modify_Res_Des
@ stub CM_Modify_Res_Des_Ex
@ stub CM_Move_DevNode
@ stub CM_Move_DevNode_Ex
@ stub CM_Next_Range
@ stub CM_Open_Class_KeyA
@ stub CM_Open_Class_KeyW
@ stub CM_Open_Class_Key_ExA
@ stub CM_Open_Class_Key_ExW
@ stdcall CM_Open_DevNode_Key(long long long long ptr long)
@ stub CM_Open_DevNode_Key_Ex
@ stub CM_Query_And_Remove_SubTreeA
@ stub CM_Query_And_Remove_SubTreeW
@ stub CM_Query_And_Remove_SubTree_ExA
@ stub CM_Query_And_Remove_SubTree_ExW
@ stub CM_Query_Arbitrator_Free_Data
@ stub CM_Query_Arbitrator_Free_Data_Ex
@ stub CM_Query_Arbitrator_Free_Size
@ stub CM_Query_Arbitrator_Free_Size_Ex
@ stub CM_Query_Remove_SubTree
@ stub CM_Query_Remove_SubTree_Ex
@ stdcall CM_Reenumerate_DevNode(ptr long)
@ stdcall CM_Reenumerate_DevNode_Ex(ptr long ptr)
@ stub CM_Register_Device_Driver
@ stub CM_Register_Device_Driver_Ex
@ stub CM_Register_Device_InterfaceA
@ stub CM_Register_Device_InterfaceW
@ stub CM_Register_Device_Interface_ExA
@ stub CM_Register_Device_Interface_ExW
@ stub CM_Remove_SubTree
@ stub CM_Remove_SubTree_Ex
@ stub CM_Remove_Unmarked_Children
@ stub CM_Remove_Unmarked_Children_Ex
@ stdcall CM_Request_Device_EjectA(ptr ptr ptr long long)
@ stdcall CM_Request_Device_EjectW(ptr ptr ptr long long)
@ stub CM_Request_Eject_PC
@ stub CM_Reset_Children_Marks
@ stub CM_Reset_Children_Marks_Ex
@ stub CM_Run_Detection
@ stub CM_Run_Detection_Ex
@ stdcall CM_Set_Class_Registry_PropertyA(ptr long ptr long long ptr)
@ stdcall CM_Set_Class_Registry_PropertyW(ptr long ptr long long ptr)
@ stub CM_Set_DevNode_Problem
@ stub CM_Set_DevNode_Problem_Ex
@ stub CM_Set_DevNode_Registry_PropertyA
@ stub CM_Set_DevNode_Registry_PropertyW
@ stub CM_Set_DevNode_Registry_Property_ExA
@ stub CM_Set_DevNode_Registry_Property_ExW
@ stub CM_Set_HW_Prof
@ stub CM_Set_HW_Prof_Ex
@ stub CM_Set_HW_Prof_FlagsA
@ stub CM_Set_HW_Prof_FlagsW
@ stub CM_Set_HW_Prof_Flags_ExA
@ stub CM_Set_HW_Prof_Flags_ExW
@ stub CM_Setup_DevNode
@ stub CM_Setup_DevNode_Ex
@ stub CM_Test_Range_Available
@ stub CM_Uninstall_DevNode
@ stub CM_Uninstall_DevNode_Ex
@ stub CM_Unregister_Device_InterfaceA
@ stub CM_Unregister_Device_InterfaceW
@ stub CM_Unregister_Device_Interface_ExA
@ stub CM_Unregister_Device_Interface_ExW
@ stdcall CaptureAndConvertAnsiArg(str ptr)
@ stdcall CaptureStringArg(wstr ptr)
@ stub CenterWindowRelativeToParent
@ stub ConcatenatePaths
@ stdcall DelayedMove(wstr wstr)
@ stub DelimStringToMultiSz
@ stub DestroyTextFileReadBuffer
@ stdcall DoesUserHavePrivilege(wstr)
@ stdcall DriverStoreAddDriverPackageA(ptr ptr ptr long ptr ptr)
@ stdcall DriverStoreAddDriverPackageW(ptr ptr ptr long ptr ptr)
@ stdcall DriverStoreDeleteDriverPackageA(ptr ptr ptr)
@ stdcall DriverStoreDeleteDriverPackageW(ptr ptr ptr)
@ stub DriverStoreEnumDriverPackageA
@ stub DriverStoreEnumDriverPackageW
@ stdcall DriverStoreFindDriverPackageA(ptr ptr ptr long ptr ptr ptr)
@ stdcall DriverStoreFindDriverPackageW(ptr ptr ptr long ptr ptr ptr)
@ stdcall DuplicateString(wstr)
@ stdcall EnablePrivilege(wstr long)
@ stub ExtensionPropSheetPageProc
@ stdcall FileExists(wstr ptr)
@ stub FreeStringArray
@ stub GetCurrentDriverSigningPolicy
@ stub GetNewInfName
@ stub GetSetFileTimestamp
@ stub GetVersionInfoFromImage
@ stub InfIsFromOemLocation
@ stdcall InstallCatalog(str str ptr)
@ stdcall InstallHinfSection(long long str long) InstallHinfSectionA
@ stdcall InstallHinfSectionA(long long str long)
@ stdcall InstallHinfSectionW(long long wstr long)
@ stub InstallStop
@ stub InstallStopEx
@ stdcall IsUserAdmin()
@ stub LookUpStringInTable
@ stub MemoryInitialize
@ stdcall MultiByteToUnicode(str long)
@ stub MultiSzFromSearchControl
@ stdcall MyFree(ptr)
@ stub MyGetFileTitle
@ stdcall MyMalloc(long)
@ stdcall MyRealloc(ptr long)
@ stdcall OpenAndMapFileForRead(wstr ptr ptr ptr ptr)
@ stub OutOfMemory
@ stub QueryMultiSzValueToArray
@ stdcall QueryRegistryValue(long wstr ptr ptr ptr)
@ stub ReadAsciiOrUnicodeTextFile
@ stdcall RegistryDelnode(long long)
# Yes, Microsoft really misspelled this one!
@ stdcall RetreiveFileSecurity(wstr ptr)
@ stub RetrieveServiceConfig
@ stub SearchForInfFile
@ stub SetArrayToMultiSzValue
@ stdcall SetupAddInstallSectionToDiskSpaceListA(long long long str ptr long)
@ stub SetupAddInstallSectionToDiskSpaceListW
@ stub SetupAddSectionToDiskSpaceListA
@ stub SetupAddSectionToDiskSpaceListW
@ stdcall SetupAddToDiskSpaceListA(long str int64 long ptr long)
@ stdcall SetupAddToDiskSpaceListW(long wstr int64 long ptr long)
@ stdcall SetupAddToSourceListA(long str)
@ stdcall SetupAddToSourceListW(long wstr)
@ stub SetupAdjustDiskSpaceListA
@ stub SetupAdjustDiskSpaceListW
@ stub SetupCancelTemporarySourceList
@ stdcall SetupCloseFileQueue(ptr)
@ stdcall SetupCloseInfFile(long)
@ stdcall SetupCloseLog()
@ stdcall SetupCommitFileQueue(long long ptr ptr) SetupCommitFileQueueW
@ stdcall SetupCommitFileQueueA(long long ptr ptr)
@ stdcall SetupCommitFileQueueW(long long ptr ptr)
@ stdcall SetupCopyErrorA(long str str str str str long long str long ptr)
@ stdcall SetupCopyErrorW(long wstr wstr wstr wstr wstr long long wstr long ptr)
@ stdcall SetupCopyOEMInfA(str str long long ptr long ptr ptr)
@ stdcall SetupCopyOEMInfW(wstr wstr long long ptr long ptr ptr)
@ stdcall SetupCreateDiskSpaceListA(ptr long long)
@ stdcall SetupCreateDiskSpaceListW(ptr long long)
@ stdcall SetupDecompressOrCopyFileA(str str ptr)
@ stdcall SetupDecompressOrCopyFileW(wstr wstr ptr)
@ stub SetupDefaultQueueCallback
@ stdcall SetupDefaultQueueCallbackA(ptr long long long)
@ stdcall SetupDefaultQueueCallbackW(ptr long long long)
@ stdcall SetupDeleteErrorA(long str str long long)
@ stdcall SetupDeleteErrorW(long wstr wstr long long)
@ stdcall SetupDestroyDiskSpaceList(long)
@ stub SetupDiAskForOEMDisk
@ stdcall SetupDiBuildClassInfoList(long ptr long ptr)
@ stdcall SetupDiBuildClassInfoListExA(long ptr long ptr str ptr)
@ stdcall SetupDiBuildClassInfoListExW(long ptr long ptr wstr ptr)
@ stdcall SetupDiBuildDriverInfoList(ptr ptr long)
@ stdcall SetupDiCallClassInstaller(long ptr ptr)
@ stub SetupDiCancelDriverInfoSearch
@ stub SetupDiChangeState
@ stdcall SetupDiClassGuidsFromNameA(str ptr long ptr)
@ stdcall SetupDiClassGuidsFromNameExA(str ptr long ptr str ptr)
@ stdcall SetupDiClassGuidsFromNameExW(wstr ptr long ptr wstr ptr)
@ stdcall SetupDiClassGuidsFromNameW(wstr ptr long ptr)
@ stdcall SetupDiClassNameFromGuidA(ptr str long ptr)
@ stdcall SetupDiClassNameFromGuidExA(ptr str long ptr str ptr)
@ stdcall SetupDiClassNameFromGuidExW(ptr wstr long ptr wstr ptr)
@ stdcall SetupDiClassNameFromGuidW(ptr wstr long ptr)
@ stdcall SetupDiCreateDevRegKeyA(ptr ptr long long long ptr str)
@ stdcall SetupDiCreateDevRegKeyW(ptr ptr long long long ptr wstr)
@ stdcall SetupDiCreateDeviceInfoA(long str ptr str long long ptr)
@ stdcall SetupDiCreateDeviceInfoList(ptr ptr)
@ stdcall SetupDiCreateDeviceInfoListExA(ptr long str ptr)
@ stdcall SetupDiCreateDeviceInfoListExW(ptr long wstr ptr)
@ stdcall SetupDiCreateDeviceInfoW(long wstr ptr wstr long long ptr)
@ stdcall SetupDiCreateDeviceInterfaceA(ptr ptr ptr str long ptr)
@ stdcall SetupDiCreateDeviceInterfaceW(ptr ptr ptr wstr long ptr)
@ stdcall SetupDiCreateDeviceInterfaceRegKeyA(ptr ptr long long ptr str)
@ stdcall SetupDiCreateDeviceInterfaceRegKeyW(ptr ptr long long ptr wstr)
@ stdcall SetupDiDeleteDevRegKey(ptr ptr long long long)
@ stdcall SetupDiDeleteDeviceInfo(ptr ptr)
@ stdcall SetupDiDeleteDeviceInterfaceData(ptr ptr)
@ stdcall SetupDiDeleteDeviceInterfaceRegKey(ptr ptr long)
@ stub SetupDiDeleteDeviceRegKey
@ stdcall SetupDiDestroyClassImageList(ptr)
@ stdcall SetupDiDestroyDeviceInfoList(long)
@ stdcall SetupDiDestroyDriverInfoList(ptr ptr long)
@ stdcall SetupDiDrawMiniIcon(ptr int128 long long)
@ stdcall SetupDiEnumDeviceInfo(long long ptr)
@ stdcall SetupDiEnumDeviceInterfaces(long ptr ptr long ptr)
@ stdcall SetupDiEnumDriverInfoA(ptr ptr long long ptr)
@ stdcall SetupDiEnumDriverInfoW(ptr ptr long long ptr)
@ stdcall SetupDiGetActualSectionToInstallA(long str ptr long ptr ptr)
@ stdcall SetupDiGetActualSectionToInstallExA(long str ptr ptr long ptr ptr ptr)
@ stdcall SetupDiGetActualSectionToInstallExW(long wstr ptr ptr long ptr ptr ptr)
@ stdcall SetupDiGetActualSectionToInstallW(long wstr ptr long ptr ptr)
@ stdcall SetupDiGetClassBitmapIndex(ptr ptr)
@ stdcall SetupDiGetClassDescriptionA(ptr str long ptr)
@ stdcall SetupDiGetClassDescriptionExA(ptr str long ptr str ptr)
@ stdcall SetupDiGetClassDescriptionExW(ptr wstr long ptr wstr ptr)
@ stdcall SetupDiGetClassDescriptionW(ptr wstr long ptr)
@ stub SetupDiGetClassDevPropertySheetsA
@ stub SetupDiGetClassDevPropertySheetsW
@ stdcall SetupDiGetClassDevsA(ptr str long long)
@ stdcall SetupDiGetClassDevsExA(ptr str ptr long ptr str ptr)
@ stdcall SetupDiGetClassDevsExW(ptr wstr ptr long ptr wstr ptr)
@ stdcall SetupDiGetClassDevsW(ptr wstr long long)
@ stdcall SetupDiGetClassImageIndex(ptr ptr ptr)
@ stdcall SetupDiGetClassImageList(ptr)
@ stub SetupDiGetClassImageListExA
@ stub SetupDiGetClassImageListExW
@ stub SetupDiGetClassInstallParamsA
@ stub SetupDiGetClassInstallParamsW
@ stdcall SetupDiGetClassRegistryPropertyW(ptr long ptr ptr long ptr wstr ptr)
@ stdcall SetupDiGetCustomDevicePropertyA(ptr ptr str long ptr ptr long ptr)
@ stdcall SetupDiGetCustomDevicePropertyW(ptr ptr wstr long ptr ptr long ptr)
@ stub SetupDiGetDeviceInfoListClass
@ stdcall SetupDiGetDeviceInfoListDetailA(ptr ptr)
@ stdcall SetupDiGetDeviceInfoListDetailW(ptr ptr)
@ stdcall SetupDiGetDeviceInstallParamsA(ptr ptr ptr)
@ stdcall SetupDiGetDeviceInstallParamsW(ptr ptr ptr)
@ stdcall SetupDiGetDeviceInstanceIdA(ptr ptr ptr long ptr)
@ stdcall SetupDiGetDeviceInstanceIdW(ptr ptr ptr long ptr)
@ stub SetupDiGetDeviceInterfaceAlias
@ stdcall SetupDiGetDeviceInterfaceDetailA(long ptr ptr long ptr ptr)
@ stdcall SetupDiGetDeviceInterfaceDetailW(long ptr ptr long ptr ptr)
@ stdcall SetupDiGetDeviceInterfacePropertyW(ptr ptr ptr ptr ptr long ptr long)
@ stdcall SetupDiGetDevicePropertyKeys(ptr ptr ptr long ptr long)
@ stdcall SetupDiGetDevicePropertyW(ptr ptr ptr ptr ptr long ptr long)
@ stdcall SetupDiGetDeviceRegistryPropertyA(long ptr long ptr ptr long ptr)
@ stdcall SetupDiGetDeviceRegistryPropertyW(long ptr long ptr ptr long ptr)
@ stdcall SetupDiGetDriverInfoDetailA(ptr ptr ptr ptr long ptr)
@ stdcall SetupDiGetDriverInfoDetailW(ptr ptr ptr ptr long ptr)
@ stub SetupDiGetDriverInstallParamsA
@ stub SetupDiGetDriverInstallParamsW
@ stub SetupDiGetHwProfileFriendlyNameA
@ stub SetupDiGetHwProfileFriendlyNameExA
@ stub SetupDiGetHwProfileFriendlyNameExW
@ stub SetupDiGetHwProfileFriendlyNameW
@ stub SetupDiGetHwProfileList
@ stub SetupDiGetHwProfileListExA
@ stub SetupDiGetHwProfileListExW
@ stdcall SetupDiGetINFClassA(str ptr ptr long ptr)
@ stdcall SetupDiGetINFClassW(wstr ptr ptr long ptr)
@ stub SetupDiGetSelectedDevice
@ stdcall SetupDiGetSelectedDriverA(ptr ptr ptr)
@ stdcall SetupDiGetSelectedDriverW(ptr ptr ptr)
@ stub SetupDiGetWizardPage
@ stdcall SetupDiInstallClassA(long str long ptr)
@ stub SetupDiInstallClassExA
@ stub SetupDiInstallClassExW
@ stdcall SetupDiInstallClassW(long wstr long ptr)
@ stdcall SetupDiInstallDevice(ptr ptr)
@ stdcall SetupDiInstallDeviceInterfaces(ptr ptr)
@ stdcall SetupDiInstallDriverFiles(ptr ptr)
@ stdcall SetupDiLoadClassIcon(ptr ptr ptr)
@ stub SetupDiMoveDuplicateDevice
@ stdcall SetupDiOpenClassRegKey(ptr long)
@ stdcall SetupDiOpenClassRegKeyExA(ptr long long str ptr)
@ stdcall SetupDiOpenClassRegKeyExW(ptr long long wstr ptr)
@ stdcall SetupDiOpenDevRegKey(ptr ptr long long long long)
@ stdcall SetupDiOpenDeviceInfoA(ptr str ptr long ptr)
@ stdcall SetupDiOpenDeviceInfoW(ptr wstr ptr long ptr)
@ stdcall SetupDiOpenDeviceInterfaceA(ptr str long ptr)
@ stdcall SetupDiOpenDeviceInterfaceRegKey(ptr ptr long long)
@ stdcall SetupDiOpenDeviceInterfaceW(ptr wstr long ptr)
@ stdcall SetupDiRegisterCoDeviceInstallers(ptr ptr)
@ stdcall SetupDiRegisterDeviceInfo(ptr ptr long ptr ptr ptr)
@ stdcall SetupDiRemoveDevice(ptr ptr)
@ stdcall SetupDiRemoveDeviceInterface(ptr ptr)
@ stdcall SetupDiSelectBestCompatDrv(ptr ptr)
@ stub SetupDiSelectDevice
@ stub SetupDiSelectOEMDrv
@ stdcall SetupDiSetClassInstallParamsA(ptr ptr ptr long)
@ stdcall SetupDiSetClassInstallParamsW(ptr ptr ptr long)
@ stdcall SetupDiSetDeviceInstallParamsA(ptr ptr ptr)
@ stdcall SetupDiSetDeviceInstallParamsW(ptr ptr ptr)
@ stdcall SetupDiSetDeviceInterfacePropertyW(ptr ptr ptr long ptr long long)
@ stdcall SetupDiSetDevicePropertyW(ptr ptr ptr long ptr long long)
@ stdcall SetupDiSetDeviceRegistryPropertyA(ptr ptr long ptr long)
@ stdcall SetupDiSetDeviceRegistryPropertyW(ptr ptr long ptr long)
@ stub SetupDiSetDriverInstallParamsA
@ stub SetupDiSetDriverInstallParamsW
@ stdcall SetupDiSetSelectedDevice(ptr ptr)
@ stub SetupDiSetSelectedDriverA
@ stub SetupDiSetSelectedDriverW
@ stub SetupDiUnremoveDevice
@ stdcall SetupDuplicateDiskSpaceListA(ptr ptr long long)
@ stdcall SetupDuplicateDiskSpaceListW(ptr ptr long long)
@ stdcall SetupEnumInfSectionsA(long long ptr long ptr)
@ stdcall SetupEnumInfSectionsW(long long ptr long ptr)
@ stdcall SetupFindFirstLineA(long str str ptr)
@ stdcall SetupFindFirstLineW(long wstr wstr ptr)
@ stdcall SetupFindNextLine(ptr ptr)
@ stdcall SetupFindNextMatchLineA(ptr str ptr)
@ stdcall SetupFindNextMatchLineW(ptr wstr ptr)
@ stub SetupFreeSourceListA
@ stub SetupFreeSourceListW
@ stub SetupGetBackupInformationA
@ stub SetupGetBackupInformationW
@ stdcall SetupGetBinaryField(ptr long ptr long ptr)
@ stdcall SetupGetFieldCount(ptr)
@ stdcall SetupGetFileCompressionInfoA(str ptr ptr ptr ptr)
@ stdcall SetupGetFileCompressionInfoExA(str ptr long ptr ptr ptr ptr)
@ stdcall SetupGetFileCompressionInfoExW(wstr ptr long ptr ptr ptr ptr)
@ stdcall SetupGetFileCompressionInfoW(wstr ptr ptr ptr ptr)
@ stdcall SetupGetFileQueueCount(long long ptr)
@ stdcall SetupGetFileQueueFlags(long ptr)
@ stdcall SetupGetInfDriverStoreLocationW(wstr ptr wstr ptr long ptr)
@ stdcall SetupGetInfFileListA(str long ptr long ptr)
@ stdcall SetupGetInfFileListW(wstr long ptr long ptr)
@ stdcall SetupGetInfInformationA(ptr long ptr long ptr)
@ stdcall SetupGetInfInformationW(ptr long ptr long ptr)
@ stub SetupGetInfSections
@ stdcall SetupGetIntField(ptr long ptr)
@ stdcall SetupGetLineByIndexA(long str long ptr)
@ stdcall SetupGetLineByIndexW(long wstr long ptr)
@ stdcall SetupGetLineCountA(long str)
@ stdcall SetupGetLineCountW(long wstr)
@ stdcall SetupGetLineTextA(ptr long str str ptr long ptr)
@ stdcall SetupGetLineTextW(ptr long wstr wstr ptr long ptr)
@ stdcall SetupGetMultiSzFieldA(ptr long ptr long ptr)
@ stdcall SetupGetMultiSzFieldW(ptr long ptr long ptr)
@ stdcall SetupGetNonInteractiveMode()
@ stdcall SetupGetSourceFileLocationA(ptr ptr str ptr ptr long ptr)
@ stdcall SetupGetSourceFileLocationW(ptr ptr wstr ptr ptr long ptr)
@ stub SetupGetSourceFileSizeA
@ stub SetupGetSourceFileSizeW
@ stdcall SetupGetSourceInfoA(ptr long long ptr long ptr)
@ stdcall SetupGetSourceInfoW(ptr long long ptr long ptr)
@ stdcall SetupGetStringFieldA(ptr long ptr long ptr)
@ stdcall SetupGetStringFieldW(ptr long ptr long ptr)
@ stdcall SetupGetTargetPathA(ptr ptr str ptr long ptr)
@ stdcall SetupGetTargetPathW(ptr ptr wstr ptr long ptr)
@ stdcall SetupInitDefaultQueueCallback(long)
@ stdcall SetupInitDefaultQueueCallbackEx(long long long long ptr)
@ stdcall SetupInitializeFileLogA (str long)
@ stdcall SetupInitializeFileLogW (wstr long)
@ stdcall SetupInstallFileA(ptr ptr str str str long ptr ptr)
@ stdcall SetupInstallFileExA(ptr ptr str str str long ptr ptr ptr)
@ stdcall SetupInstallFileExW(ptr ptr wstr wstr wstr long ptr ptr ptr)
@ stdcall SetupInstallFileW(ptr ptr wstr wstr wstr long ptr ptr)
@ stdcall SetupInstallFilesFromInfSectionA(long long long str str long)
@ stdcall SetupInstallFilesFromInfSectionW(long long long wstr wstr long)
@ stdcall SetupInstallFromInfSectionA(long long str long long str long ptr ptr long ptr)
@ stdcall SetupInstallFromInfSectionW(long long wstr long long wstr long ptr ptr long ptr)
@ stdcall SetupInstallServicesFromInfSectionA(long str long)
@ stub SetupInstallServicesFromInfSectionExA
@ stub SetupInstallServicesFromInfSectionExW
@ stdcall SetupInstallServicesFromInfSectionW(long wstr long)
@ stdcall SetupIterateCabinetA(str long ptr ptr)
@ stdcall SetupIterateCabinetW(wstr long ptr ptr)
@ stdcall SetupLogErrorA(str long)
@ stdcall SetupLogErrorW(wstr long)
@ stdcall SetupLogFileA(ptr str str str long str str str long)
@ stdcall SetupLogFileW(ptr wstr wstr wstr long wstr wstr wstr long)
@ stdcall SetupOpenAppendInfFileA(str long ptr)
@ stdcall SetupOpenAppendInfFileW(wstr long ptr)
@ stdcall SetupOpenFileQueue()
@ stdcall SetupOpenInfFileA(str str long ptr)
@ stdcall SetupOpenInfFileW(wstr wstr long ptr)
@ stdcall SetupOpenLog(long)
@ stdcall SetupOpenMasterInf()
@ stdcall SetupPromptForDiskA(ptr str str str str str long ptr long ptr)
@ stdcall SetupPromptForDiskW(ptr wstr wstr wstr wstr wstr long ptr long ptr)
@ stdcall SetupPromptReboot(ptr ptr long)
@ stdcall SetupQueryDrivesInDiskSpaceListA(ptr ptr long ptr)
@ stdcall SetupQueryDrivesInDiskSpaceListW(ptr ptr long ptr)
@ stub SetupQueryFileLogA
@ stub SetupQueryFileLogW
@ stdcall SetupQueryInfFileInformationA(ptr long str long ptr)
@ stdcall SetupQueryInfFileInformationW(ptr long wstr long ptr)
@ stdcall SetupQueryInfOriginalFileInformationA(ptr long ptr ptr)
@ stdcall SetupQueryInfOriginalFileInformationW(ptr long ptr ptr)
@ stdcall SetupQueryInfVersionInformationA(ptr long str ptr long ptr)
@ stdcall SetupQueryInfVersionInformationW(ptr long wstr ptr long ptr)
@ stub SetupQuerySourceListA
@ stub SetupQuerySourceListW
@ stdcall SetupQuerySpaceRequiredOnDriveA(long str ptr ptr long)
@ stdcall SetupQuerySpaceRequiredOnDriveW(long wstr ptr ptr long)
@ stdcall SetupQueueCopyA(long str str str str str str str long)
@ stdcall SetupQueueCopyIndirectA(ptr)
@ stdcall SetupQueueCopyIndirectW(ptr)
@ stdcall SetupQueueCopySectionA(long str long long str long)
@ stdcall SetupQueueCopySectionW(long wstr long long wstr long)
@ stdcall SetupQueueCopyW(long wstr wstr wstr wstr wstr wstr wstr long)
@ stdcall SetupQueueDefaultCopyA(long long str str str long)
@ stdcall SetupQueueDefaultCopyW(long long wstr wstr wstr long)
@ stdcall SetupQueueDeleteA(long str str)
@ stdcall SetupQueueDeleteSectionA(long long long str)
@ stdcall SetupQueueDeleteSectionW(long long long wstr)
@ stdcall SetupQueueDeleteW(long wstr wstr)
@ stdcall SetupQueueRenameA(long str str str str)
@ stdcall SetupQueueRenameSectionA(long long long str)
@ stdcall SetupQueueRenameSectionW(long long long wstr)
@ stdcall SetupQueueRenameW(long wstr wstr wstr wstr)
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
@ stdcall SetupRenameErrorA(long str str str long long)
@ stdcall SetupRenameErrorW(long wstr wstr wstr long long)
@ stub SetupScanFileQueue
@ stdcall SetupScanFileQueueA(long long long ptr ptr ptr)
@ stdcall SetupScanFileQueueW(long long long ptr ptr ptr)
@ stdcall SetupSetDirectoryIdA(long long str)
@ stub SetupSetDirectoryIdExA
@ stub SetupSetDirectoryIdExW
@ stdcall SetupSetDirectoryIdW(long long wstr)
@ stdcall SetupSetFileQueueAlternatePlatformA(ptr ptr str)
@ stdcall SetupSetFileQueueAlternatePlatformW(ptr ptr wstr)
@ stdcall SetupSetFileQueueFlags(long long long)
@ stdcall SetupSetNonInteractiveMode(long)
@ stub SetupSetPlatformPathOverrideA
@ stub SetupSetPlatformPathOverrideW
@ stdcall SetupSetSourceListA(long ptr long)
@ stdcall SetupSetSourceListW(long ptr long)
@ stdcall SetupTermDefaultQueueCallback(ptr)
@ stdcall SetupTerminateFileLog(long)
@ stdcall SetupUninstallOEMInfA(str long ptr)
@ stdcall SetupUninstallOEMInfW(wstr long ptr)
@ stub ShouldDeviceBeExcluded
@ stdcall StampFileSecurity(wstr ptr)
@ stdcall StringTableAddString(ptr wstr long)
@ stdcall StringTableAddStringEx(ptr wstr long ptr long)
@ stdcall StringTableDestroy(ptr)
@ stdcall StringTableDuplicate(ptr)
@ stub StringTableEnum
@ stdcall StringTableGetExtraData(ptr long ptr long)
@ stdcall StringTableInitialize()
@ stdcall StringTableInitializeEx(long long)
@ stdcall StringTableLookUpString(ptr wstr long)
@ stdcall StringTableLookUpStringEx(ptr wstr long ptr long)
@ stdcall StringTableSetExtraData(ptr long ptr long)
@ stdcall StringTableStringFromId(ptr long)
@ stdcall StringTableStringFromIdEx(ptr long ptr ptr)
@ stdcall StringTableTrim(ptr)
@ stdcall TakeOwnershipOfFile(wstr)
@ stdcall UnicodeToMultiByte(wstr long)
@ stdcall UnmapAndCloseFile(long long ptr)
@ stub VerifyCatalogFile
@ stub VerifyFile
@ stub pSetupAccessRunOnceNodeList
@ stub pSetupAddMiniIconToList
@ stub pSetupAddTagToGroupOrderListEntry
@ stub pSetupAppendStringToMultiSz
@ stub pSetupDestroyRunOnceNodeList
@ stub pSetupDirectoryIdToPath
@ stdcall pSetupFree(ptr) MyFree
@ stdcall pSetupGetField(ptr long)
@ stdcall pSetupGetGlobalFlags()
@ stub pSetupGetOsLoaderDriveAndPath
@ stdcall pSetupGetQueueFlags(ptr)
@ stub pSetupGetVersionDatum
@ stub pSetupGuidFromString
@ stub pSetupIsGuidNull
@ stdcall pSetupInstallCatalog(wstr wstr ptr)
@ stdcall pSetupIsUserAdmin() IsUserAdmin
@ stub pSetupMakeSurePathExists
@ stdcall pSetupMalloc(long) MyMalloc
@ stdcall pSetupRealloc(ptr long) MyRealloc
@ stdcall pSetupSetGlobalFlags(long)
@ stdcall pSetupSetQueueFlags(ptr long)
@ stub pSetupSetSystemSourceFlags
@ stub pSetupStringFromGuid
@ stdcall pSetupStringTableAddString(ptr wstr long) StringTableAddString
@ stdcall pSetupStringTableAddStringEx(ptr wstr long ptr long) StringTableAddStringEx
@ stdcall pSetupStringTableDestroy(ptr) StringTableDestroy
@ stdcall pSetupStringTableDuplicate(ptr) StringTableDuplicate
@ stub pSetupStringTableEnum
@ stdcall pSetupStringTableGetExtraData(ptr long ptr long) StringTableGetExtraData
@ stdcall pSetupStringTableInitialize() StringTableInitialize
@ stdcall pSetupStringTableInitializeEx(long long) StringTableInitializeEx
@ stdcall pSetupStringTableLookUpString(ptr wstr long) StringTableLookUpString
@ stdcall pSetupStringTableLookUpStringEx(ptr wstr long ptr ptr) StringTableLookUpStringEx
@ stdcall pSetupStringTableSetExtraData(ptr long ptr long) StringTableSetExtraData
@ stub pSetupVerifyCatalogFile
@ stub pSetupVerifyQueuedCatalogs
