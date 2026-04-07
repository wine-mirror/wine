@ stub CMP_GetBlockedDriverInfo
@ stub CMP_GetServerSideDeviceInstallFlags
@ stub CMP_Init_Detection
@ stub CMP_RegisterServiceNotification
@ stub CMP_Register_Notification
@ stub CMP_Report_LogOn
@ stdcall CMP_WaitNoPendingInstallEvents(long)
@ stub CMP_WaitServicesAvailable
@ stub CM_Add_Driver_PackageW
@ stub CM_Add_Driver_Package_ExW
@ stub CM_Add_Empty_Log_Conf
@ stub CM_Add_Empty_Log_Conf_Ex
@ stub CM_Add_IDA
@ stub CM_Add_IDW
@ stub CM_Add_ID_ExA
@ stub CM_Add_ID_ExW
@ stub CM_Add_Range
@ stub CM_Add_Res_Des
@ stub CM_Add_Res_Des_Ex
@ stub CM_Apply_PowerScheme
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
@ stub CM_Delete_Device_Interface_KeyA
@ stub CM_Delete_Device_Interface_KeyW
@ stub CM_Delete_Device_Interface_Key_ExA
@ stub CM_Delete_Device_Interface_Key_ExW
@ stub CM_Delete_Driver_PackageW
@ stub CM_Delete_Driver_Package_ExW
@ stub CM_Delete_PowerScheme
@ stub CM_Delete_Range
@ stub CM_Detect_Resource_Conflict
@ stub CM_Detect_Resource_Conflict_Ex
@ stub CM_Disable_DevNode
@ stub CM_Disable_DevNode_Ex
@ stdcall CM_Disconnect_Machine(long)
@ stub CM_Dup_Range_List
@ stub CM_Duplicate_PowerScheme
@ stub CM_Enable_DevNode
@ stub CM_Enable_DevNode_Ex
@ stdcall CM_Enumerate_Classes(long ptr long)
@ stdcall CM_Enumerate_Classes_Ex(long ptr long ptr)
@ stdcall CM_Enumerate_EnumeratorsA(long ptr ptr long)
@ stdcall CM_Enumerate_EnumeratorsW(long ptr ptr long)
@ stdcall CM_Enumerate_Enumerators_ExA(long ptr ptr long ptr)
@ stdcall CM_Enumerate_Enumerators_ExW(long ptr ptr long ptr)
@ stub CM_Find_Range
@ stub CM_First_Range
@ stub CM_Free_Log_Conf
@ stub CM_Free_Log_Conf_Ex
@ stub CM_Free_Log_Conf_Handle
@ stub CM_Free_Range_List
@ stub CM_Free_Res_Des
@ stub CM_Free_Res_Des_Ex
@ stub CM_Free_Res_Des_Handle
@ stub CM_Free_Resource_Conflict_Handle
@ stdcall CM_Get_Child(ptr long long)
@ stdcall CM_Get_Child_Ex(ptr long long ptr)
@ stdcall CM_Get_Class_Key_NameA(ptr ptr ptr long)
@ stdcall CM_Get_Class_Key_NameW(ptr ptr ptr long)
@ stdcall CM_Get_Class_Key_Name_ExA(ptr ptr ptr long ptr)
@ stdcall CM_Get_Class_Key_Name_ExW(ptr ptr ptr long ptr)
@ stub CM_Get_Class_NameA
@ stub CM_Get_Class_NameW
@ stub CM_Get_Class_Name_ExA
@ stub CM_Get_Class_Name_ExW
@ stdcall CM_Get_Class_PropertyW(ptr ptr ptr ptr long long)
@ stdcall CM_Get_Class_Property_ExW(ptr ptr ptr ptr long long ptr)
@ stdcall CM_Get_Class_Property_Keys(ptr ptr ptr long)
@ stdcall CM_Get_Class_Property_Keys_Ex(ptr ptr ptr long ptr)
@ stdcall CM_Get_Class_Registry_PropertyA(ptr long ptr ptr long long ptr)
@ stdcall CM_Get_Class_Registry_PropertyW(ptr long ptr ptr long long ptr)
@ stub CM_Get_Depth
@ stub CM_Get_Depth_Ex
@ stub CM_Get_DevNode_Custom_PropertyA
@ stub CM_Get_DevNode_Custom_PropertyW
@ stub CM_Get_DevNode_Custom_Property_ExA
@ stub CM_Get_DevNode_Custom_Property_ExW
@ stdcall CM_Get_DevNode_PropertyW(long ptr ptr ptr ptr long)
@ stdcall CM_Get_DevNode_Property_ExW(long ptr ptr ptr ptr long ptr)
@ stdcall CM_Get_DevNode_Property_Keys(long ptr ptr long)
@ stdcall CM_Get_DevNode_Property_Keys_Ex(long ptr ptr long ptr)
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
@ stdcall CM_Get_Device_ID_Size_Ex(ptr ptr long ptr)
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
@ stdcall CM_Get_Device_Interface_PropertyW(wstr ptr ptr ptr ptr long)
@ stdcall CM_Get_Device_Interface_Property_ExW(wstr ptr ptr ptr ptr long ptr)
@ stdcall CM_Get_Device_Interface_Property_KeysW(wstr ptr ptr long)
@ stdcall CM_Get_Device_Interface_Property_Keys_ExW(wstr ptr ptr long ptr)
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
@ stub CM_Get_Resource_Conflict_Count
@ stub CM_Get_Resource_Conflict_DetailsA
@ stub CM_Get_Resource_Conflict_DetailsW
@ stdcall CM_Get_Sibling(ptr long long)
@ stdcall CM_Get_Sibling_Ex(ptr long long ptr)
@ stdcall CM_Get_Version()
@ stub CM_Get_Version_Ex
@ stub CM_Import_PowerScheme
@ stub CM_Install_DevNodeW
@ stub CM_Install_DevNode_ExW
@ stub CM_Intersect_Range_List
@ stub CM_Invert_Range_List
@ stub CM_Is_Dock_Station_Present
@ stub CM_Is_Dock_Station_Present_Ex
@ stub CM_Is_Version_Available
@ stub CM_Is_Version_Available_Ex
@ stdcall CM_Locate_DevNodeA(ptr str long)
@ stdcall CM_Locate_DevNodeW(ptr wstr long)
@ stdcall CM_Locate_DevNode_ExA(ptr str long long)
@ stdcall CM_Locate_DevNode_ExW(ptr wstr long long)
@ stub CM_MapCrToSpErr
@ stdcall CM_MapCrToWin32Err(long long)
@ stub CM_Merge_Range_List
@ stub CM_Modify_Res_Des
@ stub CM_Modify_Res_Des_Ex
@ stub CM_Move_DevNode
@ stub CM_Move_DevNode_Ex
@ stub CM_Next_Range
@ stdcall CM_Open_Class_KeyA(ptr str long long ptr long)
@ stdcall CM_Open_Class_KeyW(ptr wstr long long ptr long)
@ stdcall CM_Open_Class_Key_ExA(ptr str long long ptr long ptr)
@ stdcall CM_Open_Class_Key_ExW(ptr wstr long long ptr long ptr)
@ stdcall CM_Open_DevNode_Key(long long long long ptr long)
@ stdcall CM_Open_DevNode_Key_Ex(long long long long ptr long ptr)
@ stdcall CM_Open_Device_Interface_KeyA(str long long ptr long)
@ stdcall CM_Open_Device_Interface_KeyW(wstr long long ptr long)
@ stdcall CM_Open_Device_Interface_Key_ExA(str long long ptr long ptr)
@ stdcall CM_Open_Device_Interface_Key_ExW(wstr long long ptr long ptr)
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
@ stub CM_Query_Resource_Conflict_List
@ stdcall CM_Reenumerate_DevNode(ptr long)
@ stdcall CM_Reenumerate_DevNode_Ex(ptr long ptr)
@ stub CM_Register_Device_Driver
@ stub CM_Register_Device_Driver_Ex
@ stub CM_Register_Device_InterfaceA
@ stub CM_Register_Device_InterfaceW
@ stub CM_Register_Device_Interface_ExA
@ stub CM_Register_Device_Interface_ExW
@ stdcall CM_Register_Notification(ptr ptr ptr ptr)
@ stub CM_Remove_SubTree
@ stub CM_Remove_SubTree_Ex
@ stub CM_Request_Device_EjectA
@ stub CM_Request_Device_EjectW
@ stub CM_Request_Device_Eject_ExA
@ stub CM_Request_Device_Eject_ExW
@ stub CM_Request_Eject_PC
@ stub CM_Request_Eject_PC_Ex
@ stub CM_RestoreAll_DefaultPowerSchemes
@ stub CM_Restore_DefaultPowerScheme
@ stub CM_Run_Detection
@ stub CM_Run_Detection_Ex
@ stub CM_Set_ActiveScheme
@ stub CM_Set_Class_PropertyW
@ stub CM_Set_Class_Property_ExW
@ stdcall CM_Set_Class_Registry_PropertyA(ptr long ptr long long ptr)
@ stdcall CM_Set_Class_Registry_PropertyW(ptr long ptr long long ptr)
@ stub CM_Set_DevNode_Problem
@ stub CM_Set_DevNode_Problem_Ex
@ stub CM_Set_DevNode_PropertyW
@ stub CM_Set_DevNode_Property_ExW
@ stub CM_Set_DevNode_Registry_PropertyA
@ stub CM_Set_DevNode_Registry_PropertyW
@ stub CM_Set_DevNode_Registry_Property_ExA
@ stub CM_Set_DevNode_Registry_Property_ExW
@ stub CM_Set_Device_Interface_PropertyW
@ stub CM_Set_Device_Interface_Property_ExW
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
@ stdcall CM_Unregister_Notification(ptr)
@ stub CM_Write_UserPowerKey
@ stdcall DevCloseObjectQuery(ptr)
@ stdcall DevCreateObjectQuery(long long long ptr long ptr ptr ptr ptr)
@ stdcall DevCreateObjectQueryEx(long long long ptr long ptr long ptr ptr ptr ptr)
@ stub DevCreateObjectQueryFromId
@ stub DevCreateObjectQueryFromIdEx
@ stub DevCreateObjectQueryFromIds
@ stub DevCreateObjectQueryFromIdsEx
@ stdcall DevFindProperty(ptr long wstr long ptr)
@ stdcall DevFreeObjectProperties(long ptr)
@ stdcall DevFreeObjects(long ptr)
@ stdcall DevGetObjectProperties(long ptr long long ptr ptr ptr)
@ stdcall DevGetObjectPropertiesEx(long ptr long long ptr long ptr ptr ptr)
@ stdcall DevGetObjects(long long long ptr long ptr ptr ptr)
@ stdcall DevGetObjectsEx(long long long ptr long ptr long ptr ptr ptr)
@ stub DevSetObjectProperties
@ stub SwDeviceClose
@ stub SwDeviceCreate
@ stub SwDeviceGetLifetime
@ stub SwDeviceInterfacePropertySet
@ stub SwDeviceInterfaceRegister
@ stub SwDeviceInterfaceSetState
@ stub SwDevicePropertySet
@ stub SwDeviceSetLifetime
@ stub SwMemFree
