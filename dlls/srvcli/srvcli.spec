@ stub I_NetDfsGetVersion
@ stub I_NetServerSetServiceBits
@ stub I_NetServerSetServiceBitsEx
@ stub LocalAliasGet
@ stub LocalFileClose
@ stub LocalFileEnum
@ stub LocalFileEnumEx
@ stub LocalFileGetInfo
@ stub LocalFileGetInfoEx
@ stub LocalServerCertificateMappingAdd
@ stub LocalServerCertificateMappingEnum
@ stub LocalServerCertificateMappingGet
@ stub LocalServerCertificateMappingRemove
@ stub LocalSessionDel
@ stub LocalSessionEnum
@ stub LocalSessionEnumEx
@ stub LocalSessionGetInfo
@ stub LocalSessionGetInfoEx
@ stub LocalShareAdd
@ stub LocalShareDelEx
@ stub LocalShareEnum
@ stub LocalShareEnumEx
@ stub LocalShareGetInfo
@ stub LocalShareGetInfoEx
@ stub LocalShareSetInfo
@ stub NetConnectionEnum
@ stub NetFileClose
@ stdcall NetFileEnum(wstr wstr wstr long ptr long ptr ptr ptr) netapi32.NetFileEnum
@ stub NetFileGetInfo
@ stdcall NetRemoteTOD(wstr ptr) netapi32.NetRemoteTOD
@ stub NetServerAliasAdd
@ stub NetServerAliasDel
@ stub NetServerAliasEnum
@ stub NetServerComputerNameAdd
@ stub NetServerComputerNameDel
@ stdcall NetServerDiskEnum(wstr long ptr long ptr ptr ptr) netapi32.NetServerDiskEnum
@ stdcall NetServerGetInfo(wstr long ptr) netapi32.NetServerGetInfo
@ stub NetServerSetInfo
@ stub NetServerStatisticsGet
@ stub NetServerTransportAdd
@ stub NetServerTransportAddEx
@ stub NetServerTransportDel
@ stub NetServerTransportEnum
@ stub NetSessionDel
@ stdcall NetSessionEnum(wstr wstr wstr long ptr long ptr ptr ptr) netapi32.NetSessionEnum
@ stub NetSessionGetInfo
@ stdcall NetShareAdd(wstr long ptr ptr) netapi32.NetShareAdd
@ stub NetShareCheck
@ stdcall NetShareDel(wstr wstr long) netapi32.NetShareDel
@ stub NetShareDelEx
@ stub NetShareDelSticky
@ stdcall NetShareEnum(wstr long ptr long ptr ptr ptr) netapi32.NetShareEnum
@ stub NetShareEnumSticky
@ stdcall NetShareGetInfo(wstr wstr long ptr) netapi32.NetShareGetInfo
@ stub NetShareSetInfo
@ stub NetpsNameCanonicalize
@ stub NetpsNameCompare
@ stub NetpsNameValidate
@ stub NetpsPathCanonicalize
@ stub NetpsPathCompare
@ stub NetpsPathType
