name	tapi32
type	win32

@ stdcall lineAccept(long str long) lineAccept
@ stdcall lineAddProvider(str long ptr) lineAddProvider
@ stdcall lineAddToConference(long long) lineAddToConference
@ stdcall lineAnswer(long str long) lineAnswer
@ stdcall lineBlindTransfer(long str long) lineBlindTransfer
@ stdcall lineClose(long) lineClose
@ stdcall lineCompleteCall(long ptr long long) lineCompleteCall
@ stdcall lineCompleteTransfer(long long ptr long) lineCompleteTransfer
@ stdcall lineConfigDialog(long long str) lineConfigDialog
@ stdcall lineConfigDialogEdit(long long str ptr long ptr) lineConfigDialogEdit
@ stdcall lineConfigProvider(long long) lineConfigProvider
@ stdcall lineDeallocateCall(long) lineDeallocateCall
@ stdcall lineDevSpecific(long long long ptr long) lineDevSpecific
@ stdcall lineDevSpecificFeature(long long ptr long) lineDevSpecificFeature
@ stdcall lineDial(long str long) lineDial
@ stdcall lineDrop(long str long) lineDrop
@ stdcall lineForward(long long long ptr long ptr ptr) lineForward
@ stdcall lineGatherDigits(long long str long str long long) lineGatherDigits
@ stdcall lineGenerateDigits(long long str long) lineGenerateDigits
@ stdcall lineGenerateTone(long long long long ptr) lineGenerateTone
@ stdcall lineGetAddressCaps(long long long long long ptr) lineGetAddressCaps
@ stdcall lineGetAddressID(long ptr long str long) lineGetAddressID
@ stdcall lineGetAddressStatus(long long ptr) lineGetAddressStatus
@ stdcall lineGetAppPriority(str long ptr long ptr ptr) lineGetAppPriority
@ stdcall lineGetCallInfo(long ptr) lineGetCallInfo
@ stdcall lineGetCallStatus(long ptr) lineGetCallStatus
@ stdcall lineGetConfRelatedCalls(long ptr) lineGetConfRelatedCalls
@ stdcall lineGetCountry(long long ptr) lineGetCountry
@ stdcall lineGetDevCaps(long long long long ptr) lineGetDevCaps
@ stdcall lineGetDevConfig(long ptr str) lineGetDevConfig
@ stdcall lineGetID(long long long long ptr str) lineGetID
@ stdcall lineGetIcon(long str ptr) lineGetIcon
@ stdcall lineGetLineDevStatus(long ptr) lineGetLineDevStatus
@ stdcall lineGetNewCalls(long long long ptr) lineGetNewCalls
@ stdcall lineGetNumRings(long long ptr) lineGetNumRings
@ stdcall lineGetProviderList(long ptr) lineGetProviderList
@ stdcall lineGetRequest(long long ptr) lineGetRequest
@ stdcall lineGetStatusMessages(long ptr ptr) lineGetStatusMessages
@ stdcall lineGetTranslateCaps(long long ptr) lineGetTranslateCaps
@ stdcall lineHandoff(long str long) lineHandoff
@ stdcall lineHold(long) lineHold
@ stdcall lineInitialize(ptr long ptr str ptr) lineInitialize
@ stdcall lineMakeCall(long ptr str long ptr) lineMakeCall
@ stdcall lineMonitorDigits(long long) lineMonitorDigits
@ stdcall lineMonitorMedia(long long) lineMonitorMedia
@ stdcall lineMonitorTones(long ptr long) lineMonitorTones
@ stdcall lineNegotiateAPIVersion(long long long long ptr ptr) lineNegotiateAPIVersion
@ stdcall lineNegotiateExtVersion(long long long long long ptr) lineNegotiateExtVersion
@ stdcall lineOpen(long long ptr long long long long long ptr) lineOpen
@ stdcall linePark(long long str ptr) linePark
@ stdcall linePickup(long long ptr str str) linePickup
@ stdcall linePrepareAddToConference(long ptr ptr) linePrepareAddToConference
@ stdcall lineRedirect(long str long) lineRedirect
@ stdcall lineRegisterRequestRecipient(long long long long) lineRegisterRequestRecipient
@ stdcall lineReleaseUserUserInfo(long) lineReleaseUserUserInfo
@ stdcall lineRemoveFromConference(long) lineRemoveFromConference
@ stdcall lineRemoveProvider(long long) lineRemoveProvider
@ stdcall lineSecureCall(long) lineSecureCall
@ stdcall lineSendUserUserInfo(long str long) lineSendUserUserInfo
@ stdcall lineSetAppPriority(str long ptr long str long) lineSetAppPriority
@ stdcall lineSetAppSpecific(long long) lineSetAppSpecific
@ stdcall lineSetCallParams(long long long long ptr) lineSetCallParams
@ stdcall lineSetCallPrivilege(long long) lineSetCallPrivilege
@ stdcall lineSetCurrentLocation(long long) lineSetCurrentLocation
@ stdcall lineSetDevConfig(long ptr long str) lineSetDevConfig
@ stdcall lineSetMediaControl(long long long long ptr long ptr long ptr long ptr long) lineSetMediaControl
@ stdcall lineSetMediaMode(long long) lineSetMediaMode
@ stdcall lineSetNumRings(long long long) lineSetNumRings
@ stdcall lineSetStatusMessages(long long long) lineSetStatusMessages
@ stdcall lineSetTerminal(long long long long long long long) lineSetTerminal
@ stdcall lineSetTollList(long long str long) lineSetTollList
@ stdcall lineSetupConference(long long ptr ptr long ptr) lineSetupConference
@ stdcall lineSetupTransfer(long ptr ptr) lineSetupTransfer
@ stdcall lineShutdown(long) lineShutdown
@ stdcall lineSwapHold(long long) lineSwapHold
@ stdcall lineTranslateAddress(long long long str long long ptr) lineTranslateAddress
@ stdcall lineTranslateDialog(long long long long str) lineTranslateDialog
@ stdcall lineUncompleteCall(long long) lineUncompleteCall
@ stdcall lineUnhold(long) lineUnhold
@ stdcall lineUnpark(long long ptr str) lineUnpark
@ stdcall phoneClose(long) phoneClose
@ stdcall phoneConfigDialog(long long str) phoneConfigDialog
@ stdcall phoneDevSpecific(long ptr long) phoneDevSpecific
@ stdcall phoneGetButtonInfo(long long ptr) phoneGetButtonInfo
@ stdcall phoneGetData(long long ptr long) phoneGetData
@ stdcall phoneGetDevCaps(long long long long ptr) phoneGetDevCaps
@ stdcall phoneGetDisplay(long ptr) phoneGetDisplay
@ stdcall phoneGetGain(long long ptr) phoneGetGain
@ stdcall phoneGetHookSwitch(long ptr) phoneGetHookSwitch
@ stdcall phoneGetID(long ptr str) phoneGetID
@ stdcall phoneGetIcon(long str ptr) phoneGetIcon
@ stdcall phoneGetLamp(long long ptr) phoneGetLamp
@ stdcall phoneGetRing(long ptr ptr) phoneGetRing
@ stdcall phoneGetStatus(long ptr) phoneGetStatus
@ stdcall phoneGetStatusMessages(long ptr ptr ptr) phoneGetStatusMessages
@ stdcall phoneGetVolume(long long ptr) phoneGetVolume
@ stdcall phoneInitialize(ptr long ptr str ptr) phoneInitialize
@ stdcall phoneNegotiateAPIVersion(long long long long ptr ptr) phoneNegotiateAPIVersion
@ stdcall phoneNegotiateExtVersion(long long long long long ptr) phoneNegotiateExtVersion
@ stdcall phoneOpen(long long ptr long long long long) phoneOpen
@ stdcall phoneSetButtonInfo(long long ptr) phoneSetButtonInfo
@ stdcall phoneSetData(long long ptr long) phoneSetData
@ stdcall phoneSetDisplay(long long long str long) phoneSetDisplay
@ stdcall phoneSetGain(long long long) phoneSetGain
@ stdcall phoneSetHookSwitch(long long long) phoneSetHookSwitch
@ stdcall phoneSetLamp(long long long) phoneSetLamp
@ stdcall phoneSetRing(long long long) phoneSetRing
@ stdcall phoneSetStatusMessages(long long long long) phoneSetStatusMessages
@ stdcall phoneSetVolume(long long long) phoneSetVolume
@ stdcall phoneShutdown(long) phoneShutdown
@ stdcall tapiGetLocationInfo(str str) tapiGetLocationInfo
@ stub    tapiRequestDrop
@ stdcall tapiRequestMakeCall(str str str str) tapiRequestMakeCall
@ stub    tapiRequestMediaCall
