name olecli32
type win32

import ole32.dll
import olesvr32.dll

   1 stub WEP
   2 stub OleDelete
   3 forward OleSaveToStream ole32.OleSaveToStream
   4 forward OleLoadFromStream ole32.OleLoadFromStream
   6 stub OleClone
   7 stub OleCopyFromLink
   8 stub OleEqual
   9 stdcall OleQueryLinkFromClip(str long long) OleQueryLinkFromClip
  10 stdcall OleQueryCreateFromClip(str long long) OleQueryCreateFromClip
  11 stdcall OleCreateLinkFromClip(str ptr long str ptr long long) OleCreateLinkFromClip
  12 stdcall OleCreateFromClip(str ptr long str ptr long long) OleCreateFromClip
  13 stub OleCopyToClipboard
  14 stdcall OleQueryType(ptr ptr) OleQueryType
  15 stdcall OleSetHostNames(ptr str str) OleSetHostNames
  16 stub OleSetTargetDevice
  17 stub OleSetBounds
  18 stub OleQueryBounds
  19 stub OleDraw
  20 stub OleQueryOpen
  21 stub OleActivate
  22 stub OleUpdate
  23 stub OleReconnect
  24 stub OleGetLinkUpdateOptions
  25 stub OleSetLinkUpdateOptions
  26 stub OleEnumFormats
  27 stub OleClose
  28 stub OleGetData
  29 stub OleSetData
  30 stub OleQueryProtocol
  31 stub OleQueryOutOfDate
  32 stub OleObjectConvert
  33 stub OleCreateFromTemplate
  34 forward OleCreate ole32.OleCreate
  35 stub OleQueryReleaseStatus
  36 stub OleQueryReleaseError
  37 stub OleQueryReleaseMethod
  38 forward OleCreateFromFile ole32.OleCreateFromFile
  39 stub OleCreateLinkFromFile
  40 stub OleRelease
  41 stdcall OleRegisterClientDoc(str str long ptr) OleRegisterClientDoc
  42 stdcall OleRevokeClientDoc(long) OleRevokeClientDoc
  43 stdcall OleRenameClientDoc(long str) OleRenameClientDoc
  44 stub OleRevertClientDoc
  45 stub OleSavedClientDoc
  46 stub OleRename
  47 stub OleEnumObjects
  48 stub OleQueryName
  49 stub OleSetColorScheme
  50 stub OleRequestData
  54 stub OleLockServer
  55 stub OleUnlockServer
  56 stub OleQuerySize
  57 stub OleExecute
  58 stub OleCreateInvisible
  59 stub OleQueryClientVersion
  60 stdcall OleIsDcMeta(long) OleIsDcMeta
