name setupapi
type win32

import kernel32.dll

# almost all functions are commented out for now. Ordinals are from setupapi.dll 4.0

# 45 stdcall SetupAddInstallSectionToDiskSpaceListA()         SetupAddInstallSectionToDiskSpaceListA
# 46 stdcall SetupAddInstallSectionToDiskSpaceListW()         SetupAddInstallSectionToDiskSpaceListW
# 47 stdcall SetupAddSectionToDiskSpaceListA()                SetupAddSectionToDiskSpaceListA
# 48 stdcall SetupAddSectionToDiskSpaceListW()                SetupAddSectionToDiskSpaceListW
# 49 stdcall SetupAddToDiskSpaceListA()                       SetupAddToDiskSpaceListA
# 50 stdcall SetupAddToDiskSpaceListW()                       SetupAddToDiskSpaceListW
# 51 stdcall SetupAddToSourceListA()                          SetupAddToSourceListA
# 52 stdcall SetupAddToSourceListW()                          SetupAddToSourceListW
# 53 stdcall SetupAdjustDiskSpaceListA()                      SetupAdjustDiskSpaceListA
# 54 stdcall SetupAdjustDiskSpaceListW()                      SetupAdjustDiskSpaceListW
# 55 stdcall SetupCancelTemporarySourceList()                 SetupCancelTemporarySourceList
# 56 stdcall SetupCloseFileQueue()                            SetupCloseFileQueue
# 57 stdcall SetupCloseInfFile()                              SetupCloseInfFile
# 58 stdcall SetupCommitFileQueue()                           SetupCommitFileQueue
# 59 stdcall SetupCommitFileQueueA()                          SetupCommitFileQueueA
# 60 stdcall SetupCommitFileQueueW()                          SetupCommitFileQueueW
# 61 stdcall SetupCopyErrorA()                                SetupCopyErrorA
# 62 stdcall SetupCopyErrorW()                                SetupCopyErrorW
# 63 stdcall SetupCreateDiskSpaceListA()                      SetupCreateDiskSpaceListA
# 64 stdcall SetupCreateDiskSpaceListW()                      SetupCreateDiskSpaceListW
# 65 stdcall SetupDecompressOrCopyFileA()                     SetupDecompressOrCopyFileA
# 66 stdcall SetupDecompressOrCopyFileW()                     SetupDecompressOrCopyFileW
# 67 stdcall SetupDefaultQueueCallback()                      SetupDefaultQueueCallback
# 68 stdcall SetupDefaultQueueCallbackA()                     SetupDefaultQueueCallbackA
# 69 stdcall SetupDefaultQueueCallbackW()                     SetupDefaultQueueCallbackW
# 70 stdcall SetupDeleteErrorA()                              SetupDeleteErrorA
# 71 stdcall SetupDeleteErrorW()                              SetupDeleteErrorW
# 72 stdcall SetupDestroyDiskSpaceList()                      SetupDestroyDiskSpaceList
# 73 stdcall SetupDiAskForOEMDisk()                           SetupDiAskForOEMDisk
# 74 stdcall SetupDiBuildClassInfoList()                      SetupDiBuildClassInfoList
# 75 stdcall SetupDiBuildDriverInfoList()                     SetupDiBuildDriverInfoList
# 76 stdcall SetupDiCallClassInstaller()                      SetupDiCallClassInstaller
# 77 stdcall SetupDiCancelDriverInfoSearch()                  SetupDiCancelDriverInfoSearch
# 78 stdcall SetupDiChangeState()                             SetupDiChangeState
# 79 stdcall SetupDiClassGuidsFromNameA()                     SetupDiClassGuidsFromNameA
# 80 stdcall SetupDiClassGuidsFromNameW()                     SetupDiClassGuidsFromNameW
# 81 stdcall SetupDiClassNameFromGuidA()                      SetupDiClassNameFromGuidA
# 82 stdcall SetupDiClassNameFromGuidW()                      SetupDiClassNameFromGuidW
# 83 stdcall SetupDiCreateDevRegKeyA()                        SetupDiCreateDevRegKeyA
# 84 stdcall SetupDiCreateDevRegKeyW()                        SetupDiCreateDevRegKeyW
# 85 stdcall SetupDiCreateDeviceInfoA()                       SetupDiCreateDeviceInfoA
# 86 stdcall SetupDiCreateDeviceInfoList()                    SetupDiCreateDeviceInfoList
# 87 stdcall SetupDiCreateDeviceInfoW()                       SetupDiCreateDeviceInfoW
# 88 stdcall SetupDiDeleteDevRegKey()                         SetupDiDeleteDevRegKey
# 89 stdcall SetupDiDeleteDeviceInfo()                        SetupDiDeleteDeviceInfo
# 90 stdcall SetupDiDestroyClassImageList()                   SetupDiDestroyClassImageList
# 91 stdcall SetupDiDestroyDeviceInfoList()                   SetupDiDestroyDeviceInfoList
# 92 stdcall SetupDiDestroyDriverInfoList()                   SetupDiDestroyDriverInfoList
# 93 stdcall SetupDiDrawMiniIcon()                            SetupDiDrawMiniIcon
# 94 stdcall SetupDiEnumDeviceInfo()                          SetupDiEnumDeviceInfo
# 95 stdcall SetupDiEnumDriverInfoA()                         SetupDiEnumDriverInfoA
# 96 stdcall SetupDiEnumDriverInfoW()                         SetupDiEnumDriverInfoW
# 97 stdcall SetupDiGetActualSectionToInstallA()              SetupDiGetActualSectionToInstallA
# 98 stdcall SetupDiGetActualSectionToInstallW()              SetupDiGetActualSectionToInstallW
# 99 stdcall SetupDiGetClassBitmapIndex()                     SetupDiGetClassBitmapIndex
#100 stdcall SetupDiGetClassDescriptionA()                    SetupDiGetClassDescriptionA
#101 stdcall SetupDiGetClassDescriptionW()                    SetupDiGetClassDescriptionW
#102 stdcall SetupDiGetClassDevPropertySheetsA()              SetupDiGetClassDevPropertySheetsA
#103 stdcall SetupDiGetClassDevPropertySheetsW()              SetupDiGetClassDevPropertySheetsW
#104 stdcall SetupDiGetClassDevsA()                           SetupDiGetClassDevsA
#105 stdcall SetupDiGetClassDevsW()                           SetupDiGetClassDevsW
#106 stdcall SetupDiGetClassImageIndex()                      SetupDiGetClassImageIndex
#107 stdcall SetupDiGetClassImageList()                       SetupDiGetClassImageList
#108 stdcall SetupDiGetClassInstallParamsA()                  SetupDiGetClassInstallParamsA
#109 stdcall SetupDiGetClassInstallParamsW()                  SetupDiGetClassInstallParamsW
#110 stdcall SetupDiGetDeviceInfoListClass()                  SetupDiGetDeviceInfoListClass
#111 stdcall SetupDiGetDeviceInstallParamsA()                 SetupDiGetDeviceInstallParamsA
#112 stdcall SetupDiGetDeviceInstallParamsW()                 SetupDiGetDeviceInstallParamsW
#113 stdcall SetupDiGetDeviceInstanceIdA()                    SetupDiGetDeviceInstanceIdA
#114 stdcall SetupDiGetDeviceInstanceIdW()                    SetupDiGetDeviceInstanceIdW
#115 stdcall SetupDiGetDeviceRegistryPropertyA()              SetupDiGetDeviceRegistryPropertyA
#116 stdcall SetupDiGetDeviceRegistryPropertyW()              SetupDiGetDeviceRegistryPropertyW
#117 stdcall SetupDiGetDriverInfoDetailA()                    SetupDiGetDriverInfoDetailA
#118 stdcall SetupDiGetDriverInfoDetailW()                    SetupDiGetDriverInfoDetailW
#119 stdcall SetupDiGetDriverInstallParamsA()                 SetupDiGetDriverInstallParamsA
#120 stdcall SetupDiGetDriverInstallParamsW()                 SetupDiGetDriverInstallParamsW
#121 stdcall SetupDiGetHwProfileFriendlyNameA()               SetupDiGetHwProfileFriendlyNameA
#122 stdcall SetupDiGetHwProfileFriendlyNameW()               SetupDiGetHwProfileFriendlyNameW
#123 stdcall SetupDiGetHwProfileList()                        SetupDiGetHwProfileList
#124 stdcall SetupDiGetINFClassA()                            SetupDiGetINFClassA
#125 stdcall SetupDiGetINFClassW()                            SetupDiGetINFClassW
#126 stdcall SetupDiGetSelectedDevice()                       SetupDiGetSelectedDevice
#127 stdcall SetupDiGetSelectedDriverA()                      SetupDiGetSelectedDriverA
#128 stdcall SetupDiGetSelectedDriverW()                      SetupDiGetSelectedDriverW
#129 stdcall SetupDiGetWizardPage()                           SetupDiGetWizardPage
#130 stdcall SetupDiInstallClassA()                           SetupDiInstallClassA
#131 stdcall SetupDiInstallClassW()                           SetupDiInstallClassW
#132 stdcall SetupDiInstallDevice()                           SetupDiInstallDevice
#133 stdcall SetupDiInstallDriverFiles()                      SetupDiInstallDriverFiles
#134 stdcall SetupDiLoadClassIcon()                           SetupDiLoadClassIcon
#135 stdcall SetupDiMoveDuplicateDevice()                     SetupDiMoveDuplicateDevice
#136 stdcall SetupDiOpenClassRegKey()                         SetupDiOpenClassRegKey
#137 stdcall SetupDiOpenDevRegKey()                           SetupDiOpenDevRegKey
#138 stdcall SetupDiOpenDeviceInfoA()                         SetupDiOpenDeviceInfoA
#139 stdcall SetupDiOpenDeviceInfoW()                         SetupDiOpenDeviceInfoW
#140 stdcall SetupDiRegisterDeviceInfo()                      SetupDiRegisterDeviceInfo
#141 stdcall SetupDiRemoveDevice()                            SetupDiRemoveDevice
#142 stdcall SetupDiSelectDevice()                            SetupDiSelectDevice
#143 stdcall SetupDiSelectOEMDrv()                            SetupDiSelectOEMDrv
#144 stdcall SetupDiSetClassInstallParamsA()                  SetupDiSetClassInstallParamsA
#145 stdcall SetupDiSetClassInstallParamsW()                  SetupDiSetClassInstallParamsW
#146 stdcall SetupDiSetDeviceInstallParamsA()                 SetupDiSetDeviceInstallParamsA
#147 stdcall SetupDiSetDeviceInstallParamsW()                 SetupDiSetDeviceInstallParamsW
#148 stdcall SetupDiSetDeviceRegistryPropertyA()              SetupDiSetDeviceRegistryPropertyA
#149 stdcall SetupDiSetDeviceRegistryPropertyW()              SetupDiSetDeviceRegistryPropertyW
#150 stdcall SetupDiSetDriverInstallParamsA()                 SetupDiSetDriverInstallParamsA
#151 stdcall SetupDiSetDriverInstallParamsW()                 SetupDiSetDriverInstallParamsW
#152 stdcall SetupDiSetSelectedDevice()                       SetupDiSetSelectedDevice
#153 stdcall SetupDiSetSelectedDriverA()                      SetupDiSetSelectedDriverA
#154 stdcall SetupDiSetSelectedDriverW()                      SetupDiSetSelectedDriverW
#155 stdcall SetupDuplicateDiskSpaceListA()                   SetupDuplicateDiskSpaceListA
#156 stdcall SetupDuplicateDiskSpaceListW()                   SetupDuplicateDiskSpaceListW
#157 stdcall SetupFindFirstLineA()                            SetupFindFirstLineA
#158 stdcall SetupFindFirstLineW()                            SetupFindFirstLineW
#159 stdcall SetupFindNextLine()                              SetupFindNextLine
#160 stdcall SetupFindNextMatchLineA()                        SetupFindNextMatchLineA
#161 stdcall SetupFindNextMatchLineW()                        SetupFindNextMatchLineW
#162 stdcall SetupFreeSourceListA()                           SetupFreeSourceListA
#163 stdcall SetupFreeSourceListW()                           SetupFreeSourceListW
#164 stdcall SetupGetBinaryField()                            SetupGetBinaryField
#165 stdcall SetupGetFieldCount()                             SetupGetFieldCount
#166 stdcall SetupGetFileCompressionInfoA()                   SetupGetFileCompressionInfoA
#167 stdcall SetupGetFileCompressionInfoW()                   SetupGetFileCompressionInfoW
#168 stdcall SetupGetInfFileListA()                           SetupGetInfFileListA
#169 stdcall SetupGetInfFileListW()                           SetupGetInfFileListW
#170 stdcall SetupGetInfInformationA()                        SetupGetInfInformationA
#171 stdcall SetupGetInfInformationW()                        SetupGetInfInformationW
#172 stdcall SetupGetIntField()                               SetupGetIntField
#173 stdcall SetupGetLineByIndexA()                           SetupGetLineByIndexA
#174 stdcall SetupGetLineByIndexW()                           SetupGetLineByIndexW
#175 stdcall SetupGetLineCountA()                             SetupGetLineCountA
#176 stdcall SetupGetLineCountW()                             SetupGetLineCountW
#177 stdcall SetupGetLineTextA()                              SetupGetLineTextA
#178 stdcall SetupGetLineTextW()                              SetupGetLineTextW
#179 stdcall SetupGetMultiSzFieldA()                          SetupGetMultiSzFieldA
#180 stdcall SetupGetMultiSzFieldW()                          SetupGetMultiSzFieldW
#181 stdcall SetupGetSourceFileLocationA()                    SetupGetSourceFileLocationA
#182 stdcall SetupGetSourceFileLocationW()                    SetupGetSourceFileLocationW
#183 stdcall SetupGetSourceFileSizeA()                        SetupGetSourceFileSizeA
#184 stdcall SetupGetSourceFileSizeW()                        SetupGetSourceFileSizeW
#185 stdcall SetupGetSourceInfoA()                            SetupGetSourceInfoA
#186 stdcall SetupGetSourceInfoW()                            SetupGetSourceInfoW
#187 stdcall SetupGetStringFieldA()                           SetupGetStringFieldA
#188 stdcall SetupGetStringFieldW()                           SetupGetStringFieldW
#189 stdcall SetupGetTargetPathA()                            SetupGetTargetPathA
#190 stdcall SetupGetTargetPathW()                            SetupGetTargetPathW
#191 stdcall SetupInitDefaultQueueCallback()                  SetupInitDefaultQueueCallback
#192 stdcall SetupInitDefaultQueueCallbackEx()                SetupInitDefaultQueueCallbackEx
#193 stdcall SetupInitializeFileLogA()                        SetupInitializeFileLogA
#194 stdcall SetupInitializeFileLogW()                        SetupInitializeFileLogW
#195 stdcall SetupInstallFileA()                              SetupInstallFileA
#196 stdcall SetupInstallFileExA()                            SetupInstallFileExA
#197 stdcall SetupInstallFileExW()                            SetupInstallFileExW
#198 stdcall SetupInstallFileW()                              SetupInstallFileW
#199 stdcall SetupInstallFilesFromInfSectionA()               SetupInstallFilesFromInfSectionA
#200 stdcall SetupInstallFilesFromInfSectionW()               SetupInstallFilesFromInfSectionW
#201 stdcall SetupInstallFromInfSectionA()                    SetupInstallFromInfSectionA
#202 stdcall SetupInstallFromInfSectionW()                    SetupInstallFromInfSectionW
#203 stdcall SetupInstallServicesFromInfSectionA()            SetupInstallServicesFromInfSectionA
#204 stdcall SetupInstallServicesFromInfSectionW()            SetupInstallServicesFromInfSectionW
205 stdcall SetupIterateCabinetA(str long ptr ptr) SetupIterateCabinetA
206 stdcall SetupIterateCabinetW(wstr long ptr ptr) SetupIterateCabinetW
#207 stdcall SetupLogFileA()                                  SetupLogFileA
#208 stdcall SetupLogFileW()                                  SetupLogFileW
#209 stdcall SetupOpenAppendInfFileA()                        SetupOpenAppendInfFileA
#210 stdcall SetupOpenAppendInfFileW()                        SetupOpenAppendInfFileW
#211 stdcall SetupOpenFileQueue()                             SetupOpenFileQueue
#212 stdcall SetupOpenInfFileA()                              SetupOpenInfFileA
#213 stdcall SetupOpenInfFileW()                              SetupOpenInfFileW
#214 stdcall SetupOpenMasterInf()                             SetupOpenMasterInf
#215 stdcall SetupPromptForDiskA()                            SetupPromptForDiskA
#216 stdcall SetupPromptForDiskW()                            SetupPromptForDiskW
#217 stdcall SetupPromptReboot()                              SetupPromptReboot
#218 stdcall SetupQueryDrivesInDiskSpaceListA()               SetupQueryDrivesInDiskSpaceListA
#219 stdcall SetupQueryDrivesInDiskSpaceListW()               SetupQueryDrivesInDiskSpaceListW
#220 stdcall SetupQueryFileLogA()                             SetupQueryFileLogA
#221 stdcall SetupQueryFileLogW()                             SetupQueryFileLogW
#222 stdcall SetupQueryInfFileInformationA()                  SetupQueryInfFileInformationA
#223 stdcall SetupQueryInfFileInformationW()                  SetupQueryInfFileInformationW
#224 stdcall SetupQueryInfVersionInformationA()               SetupQueryInfVersionInformationA
#225 stdcall SetupQueryInfVersionInformationW()               SetupQueryInfVersionInformationW
#226 stdcall SetupQuerySourceListA()                          SetupQuerySourceListA
#227 stdcall SetupQuerySourceListW()                          SetupQuerySourceListW
#228 stdcall SetupQuerySpaceRequiredOnDriveA()                SetupQuerySpaceRequiredOnDriveA
#229 stdcall SetupQuerySpaceRequiredOnDriveW()                SetupQuerySpaceRequiredOnDriveW
#230 stdcall SetupQueueCopyA()                                SetupQueueCopyA
#231 stdcall SetupQueueCopySectionA()                         SetupQueueCopySectionA
#232 stdcall SetupQueueCopySectionW()                         SetupQueueCopySectionW
#233 stdcall SetupQueueCopyW()                                SetupQueueCopyW
#234 stdcall SetupQueueDefaultCopyA()                         SetupQueueDefaultCopyA
#235 stdcall SetupQueueDefaultCopyW()                         SetupQueueDefaultCopyW
#236 stdcall SetupQueueDeleteA()                              SetupQueueDeleteA
#237 stdcall SetupQueueDeleteSectionA()                       SetupQueueDeleteSectionA
#238 stdcall SetupQueueDeleteSectionW()                       SetupQueueDeleteSectionW
#239 stdcall SetupQueueDeleteW()                              SetupQueueDeleteW
#240 stdcall SetupQueueRenameA()                              SetupQueueRenameA
#241 stdcall SetupQueueRenameSectionA()                       SetupQueueRenameSectionA
#242 stdcall SetupQueueRenameSectionW()                       SetupQueueRenameSectionW
#243 stdcall SetupQueueRenameW()                              SetupQueueRenameW
#244 stdcall SetupRemoveFileLogEntryA()                       SetupRemoveFileLogEntryA
#245 stdcall SetupRemoveFileLogEntryW()                       SetupRemoveFileLogEntryW
#246 stdcall SetupRemoveFromDiskSpaceListA()                  SetupRemoveFromDiskSpaceListA
#247 stdcall SetupRemoveFromDiskSpaceListW()                  SetupRemoveFromDiskSpaceListW
#248 stdcall SetupRemoveFromSourceListA()                     SetupRemoveFromSourceListA
#249 stdcall SetupRemoveFromSourceListW()                     SetupRemoveFromSourceListW
#250 stdcall SetupRemoveInstallSectionFromDiskSpaceListA()    SetupRemoveInstallSectionFromDiskSpaceListA
#251 stdcall SetupRemoveInstallSectionFromDiskSpaceListW()    SetupRemoveInstallSectionFromDiskSpaceListW
#252 stdcall SetupRemoveSectionFromDiskSpaceListA()           SetupRemoveSectionFromDiskSpaceListA
#253 stdcall SetupRemoveSectionFromDiskSpaceListW()           SetupRemoveSectionFromDiskSpaceListW
#254 stdcall SetupRenameErrorA()                              SetupRenameErrorA
#255 stdcall SetupRenameErrorW()                              SetupRenameErrorW
#256 stdcall SetupScanFileQueue()                             SetupScanFileQueue
#257 stdcall SetupScanFileQueueA()                            SetupScanFileQueueA
#258 stdcall SetupScanFileQueueW()                            SetupScanFileQueueW
#259 stdcall SetupSetDirectoryIdA(long long str)               SetupSetDirectoryIdA
#260 stdcall SetupSetDirectoryIdExA(long long str long long ptr)  SetupSetDirectoryIdExA
#261 stdcall SetupSetDirectoryIdExW(long long wstr long long ptr) SetupSetDirectoryIdExW
#262 stdcall SetupSetDirectoryIdW(long long wstr)             SetupSetDirectoryIdW
#263 stdcall SetupSetPlatformPathOverrideA(str)               SetupSetPlatformPathOverrideA
#264 stdcall SetupSetPlatformPathOverrideW(wstr)              SetupSetPlatformPathOverrideW
#265 stdcall SetupSetSourceListA(long str long)               SetupSetSourceListA
#266 stdcall SetupSetSourceListW(long wstr long)              SetupSetSourceListW
#267 stdcall SetupTermDefaultQueueCallback(ptr)               SetupTermDefaultQueueCallback
#268 stdcall SetupTerminateFileLog(ptr)                       SetupTerminateFileLog
