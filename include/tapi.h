/*
 * TAPI definitions
 * 
 * Copyright (c) 1999 Andreas Mohr
 */

#ifndef __WINE_TAPI_H
#define __WINE_TAPI_H

#include "windef.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

typedef HANDLE HCALL, *LPHCALL;
typedef HANDLE HLINE, *LPHLINE;
typedef HANDLE HLINEAPP, *LPHLINEAPP;
typedef HANDLE HPHONE, *LPHPHONE;
typedef HANDLE HPHONEAPP, *LPHPHONEAPP;

/* FIXME: bogus codes !! */
#define TAPIERR_REQUESTFAILED   20

typedef struct lineaddresscaps_tag {
    DWORD dwTotalSize;
    DWORD dwNeededSize;
    DWORD dwUsedSize;
    DWORD dwLineDeviceID;
    DWORD dwAddressSize;
    DWORD dwAddressOffset;
    DWORD dwDevSpecificSize;
    DWORD dwDevSpecificOffset;
    DWORD dwAddressSharing;
    DWORD dwAddressStates;
    DWORD dwCallInfoStates;
    DWORD dwCallerIDFlags;
    DWORD dwCalledIDFlags;
    DWORD dwConnectedIDFlags;
    DWORD dwRedirectionIDFlags;
    DWORD dwRedirectingIDFlags;
    DWORD dwCallStates;
    DWORD dwDialToneModes;
    DWORD dwBusyModes;
    DWORD dwSpecialInfo;
    DWORD dwDisconnectModes;
    DWORD dwMaxNumActiveCalls;
    DWORD dwMaxNumOnHoldCalls;
    DWORD dwMaxNumOnHoldPendingCalls;
    DWORD dwMaxNumConference;
    DWORD dwMaxNumTransConf;
    DWORD dwAddrCapFlags;
    DWORD dwCallFeatures;
    DWORD dwRemoveFromConfCaps;
    DWORD dwRemoveFromConfState;
    DWORD dwTransferModes;
    DWORD dwParkModes;
    DWORD dwForwardModes;
    DWORD dwMaxForwardEntries;
    DWORD dwMaxSpecificEntries;
    DWORD dwMinFwdNumRings;
    DWORD dwMaxFwdNumRings;
    DWORD dwMaxCallCompletions;
    DWORD dwCallCompletionConds;
    DWORD dwCallCompletionModes;
    DWORD dwNumCompletionMessages;
    DWORD dwCompletionMsgTextEntrySize;
    DWORD dwCompletionMsgTextSize;
    DWORD dwCompletionMsgTextOffset;
    DWORD dwAddressFeatures;
} LINEADDRESSCAPS, *LPLINEADDRESSCAPS;

typedef struct lineaddressstatus_tag {
    DWORD dwTotalSize;
    DWORD dwNeededSize;
    DWORD dwUsedSize;
    DWORD dwNumInUse;
    DWORD dwNumActiveCalls;
    DWORD dwNumOnHoldCalls;
    DWORD dwNumOnHoldPendCalls;
    DWORD dwAddressFeatures;
    DWORD dwNumRingsNoAnswer;
    DWORD dwForwardNumEntries;
    DWORD dwForwardSize;
    DWORD dwForwardOffset;
    DWORD dwTerminalModesSize;
    DWORD dwTerminalModesOffset;
    DWORD dwDevSpecificSize;
    DWORD dwDevSpecificOffset;
} LINEADDRESSSTATUS, *LPLINEADDRESSSTATUS;

typedef struct linedialparams_tag {
    DWORD dwDialPause;
    DWORD dwDialSpeed;
    DWORD dwDigitDuration;
    DWORD dwWaitForDialtone;
} LINEDIALPARAMS, *LPLINEDIALPARAMS;

typedef struct linecallinfo_tag {
    DWORD dwTotalSize;
    DWORD dwNeededSize;
    DWORD dwUsedSize;
    HLINE hLine;
    DWORD dwLineDeviceID;
    DWORD dwAddressID;
    DWORD dwBearerMode;
    DWORD dwRate;
    DWORD dwMediaMode;
    DWORD dwAppSpecific;
    DWORD dwCallID;
    DWORD dwRelatedCallID;
    DWORD dwCallParamFlags;
    DWORD dwCallStates;
    DWORD dwMonitorDigitModes;
    DWORD dwMonitorMediaModes;
    LINEDIALPARAMS DialParams;
    DWORD dwOrigin;
    DWORD dwReason;
    DWORD dwCompletionID;
    DWORD dwNumOwners;
    DWORD dwNumMonitors;
    DWORD dwCountryCode;
    DWORD dwTrunk;
    DWORD dwCallerIDFlags;
    DWORD dwCallerIDSize;
    DWORD dwCallerIDOffset;
    DWORD dwCallerIDNameSize;
    DWORD dwCallerIDNameOffset;
    DWORD dwCalledIDFlags;
    DWORD dwCalledIDSize;
    DWORD dwCalledIDOffset;
    DWORD dwCalledIDNameSize;
    DWORD dwCalledIDNameOffset;
    DWORD dwConnectedIDFlags;
    DWORD dwConnectedIDSize;
    DWORD dwConnectedIDOffset;
    DWORD dwConnectedIDNameSize;
    DWORD dwConnectedIDNameOffset;
    DWORD dwRedirectionIDFlags;
    DWORD dwRedirectionIDSize;
    DWORD dwRedirectionIDOffset;
    DWORD dwRedirectionIDNameSize;
    DWORD dwRedirectionIDNameOffset;
    DWORD dwRedirectingIDFlags;
    DWORD dwRedirectingIDSize;
    DWORD dwRedirectingIDOffset;
    DWORD dwRedirectingIDNameSize;
    DWORD dwRedirectingIDNameOffset;
    DWORD dwAppNameSize;
    DWORD dwAppNameOffset;
    DWORD dwDisplayableAddressSize;
    DWORD dwDisplayableAddressOffset;
    DWORD dwCalledPartySize;
    DWORD dwCalledPartyOffset;
    DWORD dwCommentSize;
    DWORD dwCommentOffset;
    DWORD dwDisplaySize;
    DWORD dwDisplayOffset;
    DWORD dwUserUserInfoSize;
    DWORD dwUserUserInfoOffset;
    DWORD dwHighLevelCompSize;
    DWORD dwHighLevelCompOffset;
    DWORD dwLowLevelCompSize;
    DWORD dwLowLevelCompOffset;
    DWORD dwChargingInfoSize;
    DWORD dwChargingInfoOffset;
    DWORD dwTerminalModesSize;
    DWORD dwTerminalModesOffset;
    DWORD dwDevSpecificSize;
    DWORD dwDevSpecificOffset;
} LINECALLINFO, *LPLINECALLINFO;

typedef struct linecalllist_tag {
    DWORD dwTotalSize;
    DWORD dwNeededSize;
    DWORD dwUsedSize;
    DWORD dwCallsNumEntries;
    DWORD dwCallsSize;
    DWORD dwCallsOffset;
} LINECALLLIST, *LPLINECALLLIST;

typedef struct linecallparams_tag {
    DWORD dwTotalSize;
    DWORD dwBearerMode;
    DWORD dwMinRate;
    DWORD dwMaxRate;
    DWORD dwMediaMode;
    DWORD dwCallParamFlags;
    DWORD dwAddressMode;
    DWORD dwAddressID;
    LINEDIALPARAMS DialParams;
    DWORD dwOrigAddressSize;
    DWORD dwOrigAddressOffset;
    DWORD dwDisplayableAddressSize;
    DWORD dwDisplayableAddressOffset;
    DWORD dwCalledPartySize;
    DWORD dwCalledPartyOffset;
    DWORD dwCommentSize;
    DWORD dwCommentOffset;
    DWORD dwUserUserInfoSize;
    DWORD dwUserUserInfoOffset;
    DWORD dwHighLevelCompSize;
    DWORD dwHighLevelCompOffset;
    DWORD dwLowLevelCompSize;
    DWORD dwLowLevelCompOffset;
    DWORD dwDevSpecificSize;
    DWORD dwDevSpecificOffset;
} LINECALLPARAMS, *LPLINECALLPARAMS;

typedef struct linecallstatus_tag {
    DWORD dwTotalSize;
    DWORD dwNeededSize;
    DWORD dwUsedSize;
    DWORD dwCallState;
    DWORD dwCallStateMode;
    DWORD dwCallPrivilege;
    DWORD dwCallFeatures;
    DWORD dwDevSpecificSize;
    DWORD dwDevSpecificOffset;
} LINECALLSTATUS, *LPLINECALLSTATUS;

typedef struct linecountrylist_tag {
    DWORD dwTotalSize;
    DWORD dwNeededSize;
    DWORD dwUsedSize;
    DWORD dwNumCountries;
    DWORD dwCountryListSize;
    DWORD dwCountryListOffset;
} LINECOUNTRYLIST, *LPLINECOUNTRYLIST;

typedef struct linedevcaps_tag {
    DWORD dwTotalSize;
    DWORD dwNeededSize;
    DWORD dwUsedSize;
    DWORD dwProviderInfoSize;
    DWORD dwProviderInfoOffset;
    DWORD dwSwitchInfoSize;
    DWORD dwSwitchInfoOffset;
    DWORD dwPermanentLineID;
    DWORD dwLineNameSize;
    DWORD dwLineNameOffset;
    DWORD dwStringFormat;
    DWORD dwAddressModes;
    DWORD dwNumAddresses;
    DWORD dwBearerModes;
    DWORD dwMaxRate;
    DWORD dwMediaModes;
    DWORD dwGenerateToneModes;
    DWORD dwGenerateToneMaxNumFreq;
    DWORD dwGenerateDigitModes;
    DWORD dwMonitorToneMaxNumFreq;
    DWORD dwMonitorToneMaxNumEntries;
    DWORD dwMonitorDigitModes;
    DWORD dwGatherDigitsMinTimeout;
    DWORD dwGatherDigitsMaxTimeout;
    DWORD dwMedCtlDigitMaxListSize;
    DWORD dwMedCtlMediaMaxListSize;
    DWORD dwMedCtlToneMaxListSize;
    DWORD dwMedCtlCallStateMaxListSize;
    DWORD dwDevCapFlags;
    DWORD dwMaxNumActiveCalls;
    DWORD dwAnswerMode;
    DWORD dwRingModes;
    DWORD dwLineStates;
    DWORD dwUUIAcceptSize;
    DWORD dwUUIAnswerSize;
    DWORD dwUUIMakeCallSize;
    DWORD dwUUIDropSize;
    DWORD dwUUISendUserUserInfoSize;
    DWORD dwUUICallInfoSize;
    LINEDIALPARAMS MinDialParams;
    LINEDIALPARAMS MaxDialParams;
    LINEDIALPARAMS DefaultDialParams;
    DWORD dwNumTerminals;
    DWORD dwTerminalCapsSize;
    DWORD dwTerminalCapsOffset;
    DWORD dwTerminalTextEntrySize;
    DWORD dwTerminalTextSize;
    DWORD dwTerminalTextOffset;
    DWORD dwDevSpecificSize;
    DWORD dwDevSpecificOffset;
    DWORD dwLineFeatures;
} LINEDEVCAPS, *LPLINEDEVCAPS;

typedef struct linedevstatus_tag {
    DWORD dwTotalSize;
    DWORD dwNeededSize;
    DWORD dwUsedSize;
    DWORD dwNumOpens;
    DWORD dwOpenMediaModes;
    DWORD dwNumActiveCalls;
    DWORD dwNumOnHoldCalls;
    DWORD dwNumOnHoldPendingCalls;
    DWORD dwLineFeatures;
    DWORD dwNumCallCompletion;
    DWORD dwRingMode;
    DWORD dwSignalLevel;
    DWORD dwBatteryLevel;
    DWORD dwRoamMode;
    DWORD dwDevStatusFlags;
    DWORD dwTerminalModesSize;
    DWORD dwTerminalModesOffset;
    DWORD dwDevSpecificSize;
    DWORD dwDevSpecificOffset;
} LINEDEVSTATUS, *LPLINEDEVSTATUS;

typedef struct lineextensionid_tag {
    DWORD dwExtensionID0;
    DWORD dwExtensionID1;
    DWORD dwExtensionID2;
    DWORD dwExtensionID3;
} LINEEXTENSIONID, *LPLINEEXTENSIONID;

typedef struct lineforward_tag {
    DWORD dwForwardMode;
    DWORD dwCallerAddressSize;
    DWORD dwCallerAddressOffset;
    DWORD dwDestCountryCode;
    DWORD dwDestAddressSize;
    DWORD dwDestAddressOffset;
} LINEFORWARD, *LPLINEFORWARD;

typedef struct lineforwardlist_tag {
    DWORD dwTotalSize;
    DWORD dwNumEntries;
    LINEFORWARD ForwardList[1];
} LINEFORWARDLIST, *LPLINEFORWARDLIST;
    
typedef struct linegeneratetone_tag {
    DWORD dwFrequency;
    DWORD dwCadenceOn;
    DWORD dwCadenceOff;
    DWORD dwVolume;
} LINEGENERATETONE, *LPLINEGENERATETONE;

typedef struct linemediacontrolcallstate_tag {
    DWORD dwCallStates;
    DWORD dwMediaControl;
} LINEMEDIACONTROLCALLSTATE, *LPLINEMEDIACONTROLCALLSTATE;

typedef struct linemediacontroldigit_tag {
    DWORD dwDigit;
    DWORD dwDigitModes;
    DWORD dwMediaControl;
} LINEMEDIACONTROLDIGIT, *LPLINEMEDIACONTROLDIGIT;

typedef struct linemediacontrolmedia_tag {
    DWORD dwMediaModes;
    DWORD dwDuration;
    DWORD dwMediaControl;
} LINEMEDIACONTROLMEDIA, *LPLINEMEDIACONTROLMEDIA;

typedef struct linemediacontroltone_tag {
    DWORD dwAppSpecific;
    DWORD dwDuration;
    DWORD dwFrequency1;
    DWORD dwFrequency2;
    DWORD dwFrequency3;
    DWORD dwMediaControl;
} LINEMEDIACONTROLTONE, *LPLINEMEDIACONTROLTONE;

typedef struct linemonitortone_tag {
    DWORD dwAppSpecific;
    DWORD dwDuration;
    DWORD dwFrequency1;
    DWORD dwFrequency2;
    DWORD dwFrequency3;
} LINEMONITORTONE, *LPLINEMONITORTONE;

typedef struct lineproviderlist_tag {
    DWORD dwTotalSize;
    DWORD dwNeededSize;
    DWORD dwUsedSize;
    DWORD dwNumProviders;
    DWORD dwProviderListSize;
    DWORD dwProviderListOffset;
} LINEPROVIDERLIST, *LPLINEPROVIDERLIST;

typedef struct linetranslatecaps_tag {
    DWORD dwTotalSize;
    DWORD dwNeededSize;
    DWORD dwUsedSize;
    DWORD dwNumLocations;
    DWORD dwLocationListSize;
    DWORD dwLocationListOffset;
    DWORD dwCurrentLocationID;
    DWORD dwNumCards;
    DWORD dwCardListSize;
    DWORD dwCardListOffset;
    DWORD dwCurrentPreferredCardID;
} LINETRANSLATECAPS, *LPLINETRANSLATECAPS;

typedef struct linetranslateoutput_tag {
    DWORD dwTotalSize;
    DWORD dwNeededSize;
    DWORD dwUsedSize;
    DWORD dwDialableStringSize;
    DWORD dwDialableStringOffset;
    DWORD dwDisplayableStringSize;
    DWORD dwDisplayableStringOffset;
    DWORD dwCurrentCountry;
    DWORD dwDestCountry;
    DWORD dwTranslateResults;
} LINETRANSLATEOUTPUT, *LPLINETRANSLATEOUTPUT;

typedef void (CALLBACK * LINECALLBACK)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD);

typedef struct _PHONEAPP {
  int dummy;
} PHONEAPP, *LPPHONEAPP;

typedef struct _PHONE {
    DWORD dwRingMode;
    DWORD dwVolume;
} PHONE, *LPPHONE;

typedef struct phonebuttoninfo_tag {
    DWORD dwTotalSize;
    DWORD dwNeededSize;
    DWORD dwUsedSize;
    DWORD dwButtonMode;
    DWORD dwButtonFunction;
    DWORD dwButtonTextSize;
    DWORD dwButtonTextOffset;
    DWORD dwDevSpecificSize;
    DWORD dwDevSpecificOffset;
    DWORD dwButtonState;
} PHONEBUTTONINFO, *LPPHONEBUTTONINFO;

typedef struct phonecaps_tag {
    DWORD dwTotalSize;
    DWORD dwNeededSize;
    DWORD dwUsedSize;
    DWORD dwProviderInfoSize;
    DWORD dwProviderInfoOffset;
    DWORD dwPhoneInfoSize;
    DWORD dwPhoneInfoOffset;
    DWORD dwPermanentPhoneID;
    DWORD dwPhoneNameSize;
    DWORD dwPhoneNameOffset;
    DWORD dwStringFormat;
    DWORD dwPhoneStates;
    DWORD dwHookSwitchDevs;
    DWORD dwHandsetHookSwitchModes;
    DWORD dwSpeakerHookSwitchModes;
    DWORD dwHeadsetHookSwitchModes;
    DWORD dwVolumeFlags;
    DWORD dwGainFlags;
    DWORD dwDisplayNumRows;
    DWORD dwDisplayNumColumns;
    DWORD dwNumRingModes;
    DWORD dwNumButtonLamps;
    DWORD dwButtonModesSize;
    DWORD dwButtonModesOffset;
    DWORD dwButtonFunctionsSize;
    DWORD dwButtonFunctionsOffset;
    DWORD dwLampModesSize;
    DWORD dwLampModesOffset;
    DWORD dwNumSetData;
    DWORD dwSetDataSize;
    DWORD dwSetDataOffset;
    DWORD dwNumGetData;
    DWORD dwGetDataSize;
    DWORD dwGetDataOffset;
    DWORD dwDevSpecificSize;
    DWORD dwDevSpecificOffset;
} PHONECAPS, *LPPHONECAPS;

typedef struct phoneextensionid_tag {
    DWORD dwExtensionID0;
    DWORD dwExtensionID1;
    DWORD dwExtensionID2;
    DWORD dwExtensionID3;
} PHONEEXTENSIONID, *LPPHONEEXTENSIONID;

typedef struct phonestatus_tag {
    DWORD dwTotalSize;
    DWORD dwNeededSize;
    DWORD dwUsedSize;
    DWORD dwStatusFlags;
    DWORD dwNumOwners;
    DWORD dwRingMOde;
    DWORD dwRingVolume;
    DWORD dwHandsetHookSwitchMode;
    DWORD dwHandsetVolume;
    DWORD dwHandsetGain;
    DWORD dwSpeakerHookSwitchMode;
    DWORD dwSpeakerVolume;
    DWORD dwSpeakerGain;
    DWORD dwHeadsetHookSwitchMode;
    DWORD dwHeadsetVolume;
    DWORD dwHeadsetGain;
    DWORD dwDisplaySize;
    DWORD dwDisplayOffset;
    DWORD dwLampModesSize;
    DWORD dwLampModesOffset;
    DWORD dwOwnerNameSize;
    DWORD dwOwnerNameOffset;
    DWORD dwDevSpecificSize;
    DWORD dwDevSpecificOffset;
} PHONESTATUS, *LPPHONESTATUS;

typedef void (CALLBACK * PHONECALLBACK)(HANDLE, DWORD, DWORD, DWORD, DWORD, DWORD);

typedef struct varstring_tag {
    DWORD dwTotalSize;
    DWORD dwNeededSize;
    DWORD dwUsedSize;
    DWORD dwStringFormat;
    DWORD dwStringSize;
    DWORD dwStringOffset;
} VARSTRING, *LPVARSTRING;

/* line functions */
DWORD WINAPI lineAccept(HCALL,LPCSTR,DWORD);
DWORD WINAPI lineAddProvider(LPCSTR,HWND,LPDWORD);
DWORD WINAPI lineAddToConference(HCALL,HCALL);
DWORD WINAPI lineAnswer(HCALL,LPCSTR,DWORD);
DWORD WINAPI lineBlindTransfer(HCALL,LPCSTR,DWORD);
DWORD WINAPI lineClose(HLINE);
DWORD WINAPI lineCompleteCall(HCALL,LPDWORD,DWORD,DWORD);
DWORD WINAPI lineCompleteTransfer(HCALL,HCALL,LPHCALL,DWORD);
DWORD WINAPI lineConfigDialog(DWORD,HWND,LPCSTR);
DWORD WINAPI lineConfigDialogEdit(DWORD,HWND,LPCSTR,LPVOID const,DWORD,LPVARSTRING);
DWORD WINAPI lineConfigProvider(HWND,DWORD);
DWORD WINAPI lineDeallocateCall(HCALL);
DWORD WINAPI lineDevSpecific(HLINE,DWORD,HCALL,LPVOID,DWORD);
DWORD WINAPI lineDevSpecificFeature(HLINE,DWORD,LPVOID,DWORD);
DWORD WINAPI lineDial(HCALL,LPCSTR,DWORD);
DWORD WINAPI lineDrop(HCALL,LPCSTR,DWORD);
DWORD WINAPI lineForward(HLINE,DWORD,DWORD,LPLINEFORWARDLIST,DWORD,LPHCALL,LPLINECALLPARAMS);
DWORD WINAPI lineGatherDigits(HCALL,DWORD,LPSTR,DWORD,LPCSTR,DWORD,DWORD);
DWORD WINAPI lineGenerateDigits(HCALL,DWORD,LPCSTR,DWORD);
DWORD WINAPI lineGenerateTone(HCALL,DWORD,DWORD,DWORD,LPLINEGENERATETONE);
DWORD WINAPI lineGetAddressCaps(HLINEAPP,DWORD,DWORD,DWORD,DWORD,LPLINEADDRESSCAPS);
DWORD WINAPI lineGetAddressID(HLINE,LPDWORD,DWORD,LPCSTR,DWORD);
DWORD WINAPI lineGetAddressStatus(HLINE,DWORD,LPLINEADDRESSSTATUS);
DWORD WINAPI lineGetAppPriority(LPCSTR,DWORD,LPLINEEXTENSIONID const,DWORD,LPVARSTRING,LPDWORD);
DWORD WINAPI lineGetCallInfo(HCALL,LPLINECALLINFO);
DWORD WINAPI lineGetCallStatus(HCALL,LPLINECALLSTATUS);
DWORD WINAPI lineGetConfRelatedCalls(HCALL,LPLINECALLLIST);
DWORD WINAPI lineGetCountry(DWORD,DWORD,LPLINECOUNTRYLIST);
DWORD WINAPI lineGetDevCaps(HLINEAPP,DWORD,DWORD,DWORD,LPLINEDEVCAPS);
DWORD WINAPI lineGetDevConfig(DWORD,LPVARSTRING,LPCSTR);
DWORD WINAPI lineGetID(HLINE,DWORD,HCALL,DWORD,LPVARSTRING,LPCSTR);
DWORD WINAPI lineGetIcon(DWORD,LPCSTR,HICON *);
DWORD WINAPI lineGetLineDevStatus(HLINE,LPLINEDEVSTATUS);
DWORD WINAPI lineGetNewCalls(HLINE,DWORD,DWORD,LPLINECALLLIST);
DWORD WINAPI lineGetNumRings(HLINE,DWORD,LPDWORD);
DWORD WINAPI lineGetProviderList(DWORD dwAPIVersion,LPLINEPROVIDERLIST);
DWORD WINAPI lineGetRequest(HLINEAPP,DWORD,LPVOID);
DWORD WINAPI lineGetStatusMessages(HLINE,LPDWORD,LPDWORD);
DWORD WINAPI lineGetTranslateCaps(HLINEAPP,DWORD,LPLINETRANSLATECAPS);
DWORD WINAPI lineHandoff(HCALL,LPCSTR,DWORD);
DWORD WINAPI lineHold(HCALL);
DWORD WINAPI lineInitialize(LPHLINEAPP,HINSTANCE,LINECALLBACK,LPCSTR,LPDWORD);
DWORD WINAPI lineMakeCall(HLINE,LPHCALL,LPCSTR,DWORD,LPLINECALLPARAMS);
DWORD WINAPI lineMonitorDigits(HCALL,DWORD);
DWORD WINAPI lineMonitorMedia(HCALL,DWORD);
DWORD WINAPI lineMonitorTones(HCALL,LPLINEMONITORTONE,DWORD);
DWORD WINAPI lineNegotiateAPIVersion(HLINEAPP,DWORD,DWORD,DWORD,LPDWORD,LPLINEEXTENSIONID);
DWORD WINAPI lineNegotiateExtVersion(HLINEAPP,DWORD,DWORD,DWORD,DWORD,LPDWORD);
DWORD WINAPI lineOpen(HLINEAPP,DWORD,LPHLINE,DWORD,DWORD,DWORD,DWORD,DWORD,LPLINECALLPARAMS);
DWORD WINAPI linePark(HCALL,DWORD,LPCSTR,LPVARSTRING);
DWORD WINAPI linePickup(HLINE,DWORD,LPHCALL,LPCSTR,LPCSTR);
DWORD WINAPI linePrepareAddToConference(HCALL,LPHCALL,LPLINECALLPARAMS);
DWORD WINAPI lineRedirect(HCALL,LPCSTR,DWORD);
DWORD WINAPI lineRegisterRequestRecipient(HLINEAPP,DWORD,DWORD,DWORD);
DWORD WINAPI lineReleaseUserUserInfo(HCALL);
DWORD WINAPI lineRemoveFromConference(HCALL);
DWORD WINAPI lineRemoveProvider(DWORD,HWND);
DWORD WINAPI lineSecureCall(HCALL);
DWORD WINAPI lineSendUserUserInfo(HCALL,LPCSTR,DWORD);
DWORD WINAPI lineSetAppPriority(LPCSTR,DWORD,LPLINEEXTENSIONID const,DWORD,LPCSTR,DWORD);
DWORD WINAPI lineSetAppSpecific(HCALL,DWORD);
DWORD WINAPI lineSetCallParams(HCALL,DWORD,DWORD,DWORD,LPLINEDIALPARAMS);
DWORD WINAPI lineSetCallPrivilege(HCALL,DWORD);
DWORD WINAPI lineSetCurrentLocation(HLINEAPP,DWORD);
DWORD WINAPI lineSetDevConfig(DWORD,LPVOID,DWORD,LPCSTR);
DWORD WINAPI lineSetMediaControl(HLINE,DWORD,HCALL,DWORD,LPLINEMEDIACONTROLDIGIT,DWORD,LPLINEMEDIACONTROLMEDIA,DWORD,LPLINEMEDIACONTROLTONE,DWORD,LPLINEMEDIACONTROLCALLSTATE,DWORD);
DWORD WINAPI lineSetMediaMode(HCALL,DWORD);
DWORD WINAPI lineSetNumRings(HLINE,DWORD,DWORD);
DWORD WINAPI lineSetStatusMessages(HLINE,DWORD,DWORD);
DWORD WINAPI lineSetTerminal(HLINE,DWORD,HCALL,DWORD,DWORD,DWORD,DWORD);
DWORD WINAPI lineSetTollList(HLINEAPP,DWORD,LPCSTR,DWORD);
DWORD WINAPI lineSetupConference(HCALL,HLINE,LPHCALL,LPHCALL,DWORD,LPLINECALLPARAMS);
DWORD WINAPI lineSetupTransfer(HCALL,LPHCALL,LPLINECALLPARAMS);
DWORD WINAPI lineShutdown(HLINEAPP);
DWORD WINAPI lineSwapHold(HCALL,HCALL);
DWORD WINAPI lineTranslateAddress(HLINEAPP,DWORD,DWORD,LPCSTR,DWORD,DWORD,LPLINETRANSLATEOUTPUT);
DWORD WINAPI lineTranslateDialog(HLINEAPP,DWORD,DWORD,HWND,LPCSTR);
DWORD WINAPI lineUncompleteCall(HLINE,DWORD);
DWORD WINAPI lineUnHold(HCALL);
DWORD WINAPI lineUnpark(HLINE,DWORD,LPHCALL,LPCSTR);

/* phone functions */
DWORD WINAPI phoneClose(HPHONE);
DWORD WINAPI phoneConfigDialog(DWORD,HWND,LPCSTR);
DWORD WINAPI phoneDevSpecific(HPHONE,LPVOID,DWORD);
DWORD WINAPI phoneGetButtonInfo(HPHONE,DWORD,LPPHONEBUTTONINFO);
DWORD WINAPI phoneGetData(HPHONE,DWORD,LPVOID,DWORD);
DWORD WINAPI phoneGetDevCaps(HPHONEAPP,DWORD,DWORD,DWORD,LPPHONECAPS);
DWORD WINAPI phoneGetDisplay(HPHONE,LPVARSTRING);
DWORD WINAPI phoneGetGain(HPHONE,DWORD,LPDWORD);
DWORD WINAPI phoneGetHookSwitch(HPHONE,LPDWORD);
DWORD WINAPI phoneGetID(HPHONE,LPVARSTRING,LPCSTR);
DWORD WINAPI phoneGetIcon(DWORD,LPCSTR,HICON *);
DWORD WINAPI phoneGetLamp(HPHONE,DWORD,LPDWORD);
DWORD WINAPI phoneGetRing(HPHONE,LPDWORD,LPDWORD);
DWORD WINAPI phoneGetStatus(HPHONE,LPPHONESTATUS);
DWORD WINAPI phoneGetStatusMessages(HPHONE,LPDWORD,LPDWORD,LPDWORD);
DWORD WINAPI phoneGetVolume(HPHONE,DWORD,LPDWORD);
DWORD WINAPI phoneInitialize(LPHPHONEAPP,HINSTANCE,PHONECALLBACK,LPCSTR,LPDWORD);
DWORD WINAPI phoneNegotiateAPIVersion(HPHONEAPP,DWORD,DWORD,DWORD,LPDWORD,LPPHONEEXTENSIONID);
DWORD WINAPI phoneNegotiateExtVersion(HPHONEAPP,DWORD,DWORD,DWORD,DWORD,LPDWORD);
DWORD WINAPI phoneOpen(HPHONEAPP,DWORD,LPHPHONE,DWORD,DWORD,DWORD,DWORD);
DWORD WINAPI phoneSetButtonInfo(HPHONE,DWORD,LPPHONEBUTTONINFO);
DWORD WINAPI phoneSetData(HPHONE,DWORD,LPVOID,DWORD);
DWORD WINAPI phoneSetDisplay(HPHONE,DWORD,DWORD,LPCSTR,DWORD);
DWORD WINAPI phoneSetGain(HPHONE,DWORD,DWORD);
DWORD WINAPI phoneSetHookSwitch(HPHONE,DWORD,DWORD);
DWORD WINAPI phoneSetLamp(HPHONE,DWORD,DWORD);
DWORD WINAPI phoneSetRing(HPHONE,DWORD,DWORD);
DWORD WINAPI phoneSetStatusMessages(HPHONE,DWORD,DWORD,DWORD);
DWORD WINAPI phoneSetVolume(HPHONE,DWORD,DWORD);
DWORD WINAPI phoneShutdown(HPHONEAPP);

/* "assisted" functions */
DWORD WINAPI tapiGetLocationInfo(LPSTR,LPSTR);
DWORD WINAPI tapiRequestMakeCall(LPCSTR,LPCSTR,LPCSTR,LPCSTR);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_TAPI_H */
