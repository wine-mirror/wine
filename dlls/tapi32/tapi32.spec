name	tapi32
type	win32

  1 stdcall lineAccept(long str long) lineAccept
  2 stdcall lineAddProvider(str long ptr) lineAddProvider
  3 stdcall lineAddToConference(long long) lineAddToConference
  4 stdcall lineAnswer(long str long) lineAnswer
  5 stdcall lineBlindTransfer(long str long) lineBlindTransfer
  6 stdcall lineClose(long) lineClose
  7 stdcall lineCompleteCall(long ptr long long) lineCompleteCall
  8 stdcall lineCompleteTransfer(long long ptr long) lineCompleteTransfer
  9 stdcall lineConfigDialog(long long str) lineConfigDialog
 10 stdcall lineConfigDialogEdit(long long str ptr long ptr)
 lineConfigDialogEdit
 11 stdcall lineConfigProvider(long long) lineConfigProvider
 12 stdcall lineDeallocateCall(long) lineDeallocateCall
 13 stdcall lineDevSpecific(long long long ptr long) lineDevSpecific
 14 stdcall lineDevSpecificFeature(long long ptr long) lineDevSpecificFeature
 15 stdcall lineDial(long str long) lineDial
 16 stdcall lineDrop(long str long) lineDrop
 17 stdcall lineForward(long long long ptr long ptr ptr) lineForward
 18 stdcall lineGatherDigits(long long str long str long long)
 lineGatherDigits
 19 stdcall lineGenerateDigits(long long str long) lineGenerateDigits
 20 stdcall lineGenerateTone(long long long long ptr) lineGenerateTone
 21 stdcall lineGetAddressCaps(long long long long long ptr)
 lineGetAddressCaps
 22 stdcall lineGetAddressID(long ptr long str long) lineGetAddressID
 23 stdcall lineGetAddressStatus(long long ptr) lineGetAddressStatus
 24 stdcall lineGetAppPriority(str long ptr long ptr ptr) lineGetAppPriority
 25 stdcall lineGetCallInfo(long ptr) lineGetCallInfo
 26 stdcall lineGetCallStatus(long ptr) lineGetCallStatus
 27 stdcall lineGetConfRelatedCalls(long ptr) lineGetConfRelatedCalls
 28 stdcall lineGetCountry(long long ptr) lineGetCountry
 29 stdcall lineGetDevCaps(long long long long ptr) lineGetDevCaps
 30 stdcall lineGetDevConfig(long ptr str) lineGetDevConfig
 31 stdcall lineGetID(long long long long ptr str) lineGetID
 32 stdcall lineGetIcon(long str ptr) lineGetIcon
 33 stdcall lineGetLineDevStatus(long ptr) lineGetLineDevStatus
 34 stdcall lineGetNewCalls(long long long ptr) lineGetNewCalls
 35 stdcall lineGetNumRings(long long ptr) lineGetNumRings
 36 stdcall lineGetProviderList(long ptr) lineGetProviderList
 37 stdcall lineGetRequest(long long ptr) lineGetRequest
 38 stdcall lineGetStatusMessages(long ptr ptr) lineGetStatusMessages
 39 stdcall lineGetTranslateCaps(long long ptr) lineGetTranslateCaps
 40 stdcall lineHandoff(long str long) lineHandoff
 41 stdcall lineHold(long) lineHold
 42 stdcall lineInitialize(ptr long ptr str ptr) lineInitialize
 43 stdcall lineMakeCall(long ptr str long ptr) lineMakeCall
 44 stdcall lineMonitorDigits(long long) lineMonitorDigits
 45 stdcall lineMonitorMedia(long long) lineMonitorMedia
 46 stdcall lineMonitorTones(long ptr long) lineMonitorTones
 47 stdcall lineNegotiateAPIVersion(long long long long ptr ptr) lineNegotiateAPIVersion
 48 stdcall lineNegotiateExtVersion(long long long long long ptr)
 lineNegotiateExtVersion
 49 stdcall lineOpen(long long ptr long long long long long ptr) lineOpen
 50 stdcall linePark(long long str ptr) linePark
 51 stdcall linePickup(long long ptr str str) linePickup
 52 stdcall linePrepareAddToConference(long ptr ptr) linePrepareAddToConference
 53 stdcall lineRedirect(long str long) lineRedirect
 54 stdcall lineRegisterRequestRecipient(long long long long)
 lineRegisterRequestRecipient
 55 stdcall lineReleaseUserUserInfo(long) lineReleaseUserUserInfo
 56 stdcall lineRemoveFromConference(long) lineRemoveFromConference
 57 stdcall lineRemoveProvider(long long) lineRemoveProvider
 58 stdcall lineSecureCall(long) lineSecureCall
 59 stdcall lineSendUserUserInfo(long str long) lineSendUserUserInfo
 60 stdcall lineSetAppPriority(str long ptr long str long) lineSetAppPriority
 61 stdcall lineSetAppSpecific(long long) lineSetAppSpecific
 62 stdcall lineSetCallParams(long long long long ptr) lineSetCallParams
 63 stdcall lineSetCallPrivilege(long long) lineSetCallPrivilege
 64 stdcall lineSetCurrentLocation(long long) lineSetCurrentLocation
 65 stdcall lineSetDevConfig(long ptr long str) lineSetDevConfig
 66 stdcall lineSetMediaControl(long long long long ptr long ptr long ptr long ptr long) lineSetMediaControl
 67 stdcall lineSetMediaMode(long long) lineSetMediaMode
 68 stdcall lineSetNumRings(long long long) lineSetNumRings
 69 stdcall lineSetStatusMessages(long long long) lineSetStatusMessages
 70 stdcall lineSetTerminal(long long long long long long long)
 lineSetTerminal
 71 stdcall lineSetTollList(long long str long) lineSetTollList
 72 stdcall lineSetupConference(long long ptr ptr long ptr)
 lineSetupConference
 73 stdcall lineSetupTransfer(long ptr ptr) lineSetupTransfer
 74 stdcall lineShutdown(long) lineShutdown
 75 stdcall lineSwapHold(long long) lineSwapHold
 76 stdcall lineTranslateAddress(long long long str long long ptr)
 lineTranslateAddress
 77 stdcall lineTranslateDialog(long long long long str) lineTranslateDialog
 78 stdcall lineUncompleteCall(long long) lineUncompleteCall
 79 stdcall lineUnHold(long) lineUnHold
 80 stdcall lineUnpark(long long ptr str) lineUnpark
 81 stdcall phoneClose(long) phoneClose
 82 stdcall phoneConfigDialog(long long str) phoneConfigDialog
 83 stdcall phoneDevSpecific(long ptr long) phoneDevSpecific
 84 stdcall phoneGetButtonInfo(long long ptr) phoneGetButtonInfo
 85 stdcall phoneGetData(long long ptr long) phoneGetData
 86 stdcall phoneGetDevCaps(long long long long ptr) phoneGetDevCaps
 87 stdcall phoneGetDisplay(long ptr) phoneGetDisplay
 88 stdcall phoneGetGain(long long ptr) phoneGetGain
 89 stdcall phoneGetHookSwitch(long ptr) phoneGetHookSwitch
 90 stdcall phoneGetID(long ptr str) phoneGetID
 91 stdcall phoneGetIcon(long str ptr) phoneGetIcon
 92 stdcall phoneGetLamp(long long ptr) phoneGetLamp
 93 stdcall phoneGetRing(long ptr ptr) phoneGetRing
 94 stdcall phoneGetStatus(long ptr) phoneGetStatus
 95 stdcall phoneGetStatusMessages(long ptr ptr ptr) phoneGetStatusMessages
 96 stdcall phoneGetVolume(long long ptr) phoneGetVolume
 97 stdcall phoneInitialize(ptr long ptr str ptr) phoneInitialize
 98 stdcall phoneNegotiateAPIVersion(long long long long ptr ptr)
 phoneNegotiateAPIVersion
 99 stdcall phoneNegotiateExtVersion(long long long long long ptr)
 phoneNegotiateExtVersion
100 stdcall phoneOpen(long long ptr long long long long) phoneOpen
101 stdcall phoneSetButtonInfo(long long ptr) phoneSetButtonInfo
102 stdcall phoneSetData(long long ptr long) phoneSetData
103 stdcall phoneSetDisplay(long long long str long) phoneSetDisplay
104 stdcall phoneSetGain(long long long) phoneSetGain
105 stdcall phoneSetHookSwitch(long long long) phoneSetHookSwitch
106 stdcall phoneSetLamp(long long long) phoneSetLamp
107 stdcall phoneSetRing(long long long) phoneSetRing
108 stdcall phoneSetStatusMessages(long long long long) phoneSetStatusMessages
109 stdcall phoneSetVolume(long long long) phoneSetVolume
110 stdcall phoneShutdown(long) phoneShutdown
111 stdcall tapiGetLocationInfo(str str) tapiGetLocationInfo
112 stub    tapiRequestDrop
113 stdcall tapiRequestMakeCall(str str str str) tapiRequestMakeCall
114 stub    tapiRequestMediaCall
