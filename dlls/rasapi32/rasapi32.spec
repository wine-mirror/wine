 1  stub RasAutodialAddressToNetwork
 2  stub RasAutodialEntryToNetwork
 3  stub RasConnectionNotificationA
 4  stub RasConnectionNotificationW
 7  stdcall RasDeleteEntryA(str str) RasDeleteEntryA
 8  stdcall RasDeleteEntryW(wstr wstr) RasDeleteEntryW
14  stdcall RasEnumAutodialAddressesA(ptr ptr ptr) RasEnumAutodialAddressesA
15  stdcall RasEnumAutodialAddressesW(ptr ptr ptr) RasEnumAutodialAddressesW
19  stdcall RasEnumDevicesA(ptr ptr ptr) RasEnumDevicesA
20  stdcall RasEnumDevicesW(ptr ptr ptr) RasEnumDevicesW
24  stdcall RasGetAutodialAddressA(str ptr ptr ptr ptr) RasGetAutodialAddressA
25  stdcall RasGetAutodialAddressW(wstr ptr ptr ptr ptr) RasGetAutodialAddressW
26  stdcall RasGetAutodialEnableA(long ptr) RasGetAutodialEnableA
27  stdcall RasGetAutodialEnableW(long ptr) RasGetAutodialEnableW
28  stdcall RasGetAutodialParamA(long ptr ptr) RasGetAutodialParamA
29  stdcall RasGetAutodialParamW(long ptr ptr) RasGetAutodialParamW
30  stub RasGetConnectResponse
34  stub RasGetCountryInfoA
35  stub RasGetCountryInfoW
36  stub RasGetCredentialsA
37  stub RasGetCredentialsW
40  stub RasGetEntryPropertiesA
41  stub RasGetEntryPropertiesW
45  stub RasGetHport
48  stub RasGetSubEntryHandleA
49  stub RasGetSubEntryHandleW
50  stub RasGetSubEntryPropertiesA
51  stub RasGetSubEntryPropertiesW
55  stub RasRenameEntryA
56  stub RasRenameEntryW
57  stdcall RasSetAutodialAddressA(str long ptr long long) RasSetAutodialAddressA
58  stdcall RasSetAutodialAddressW(wstr long ptr long long) RasSetAutodialAddressW
59  stdcall RasSetAutodialEnableA(long long) RasSetAutodialEnableA
60  stdcall RasSetAutodialEnableW(long long) RasSetAutodialEnableW
61  stdcall RasSetAutodialParamA(long ptr long) RasSetAutodialParamA
62  stdcall RasSetAutodialParamW(long ptr long) RasSetAutodialParamW
63  stub RasSetCredentialsA
64  stub RasSetCredentialsW
67  stub RasSetEntryPropertiesA
68  stub RasSetEntryPropertiesW
69  stub RasSetOldPassword
70  stub RasSetSubEntryPropertiesA
71  stub RasSetSubEntryPropertiesW
72  stub RasValidateEntryNameA
73  stub RasValidateEntryNameW

11  stub RasDialWow
18  stub RasEnumConnectionsWow
23  stub RasEnumEntriesWow
33  stub RasGetConnectStatusWow
44  stub RasGetErrorStringWow
54  stub RasHangUpWow

500 stub	RnaEngineRequest
501 stub	DialEngineRequest
502 stub	SuprvRequest
503 stub	DialInMessage
504 stub	RnaEnumConnEntries
505 stub	RnaGetConnEntry
506 stub	RnaFreeConnEntry
507 stub	RnaSaveConnEntry
508 stub	RnaDeleteConnEntry
509 stub	RnaRenameConnEntry
510 stub	RnaValidateEntryName
511 stub	RnaEnumDevices
512 stub	RnaGetDeviceInfo
513 stub	RnaGetDefaultDevConfig
514 stub	RnaBuildDevConfig
515 stub	RnaDevConfigDlg
516 stub	RnaFreeDevConfig
517 stub	RnaActivateEngine
518 stub	RnaDeactivateEngine
519 stub	SuprvEnumAccessInfo
520 stub	SuprvGetAccessInfo
521 stub	SuprvSetAccessInfo
522 stub	SuprvGetAdminConfig
523 stub	SuprvInitialize
524 stub	SuprvDeInitialize
525 stub	RnaUIDial
526 stub	RnaImplicitDial
527 stub	RasDial16
528 stub	RnaSMMInfoDialog
529 stub	RnaEnumerateMacNames
530 stub	RnaEnumCountryInfo
531 stub	RnaGetAreaCodeList
532 stub	RnaFindDriver
533 stub	RnaInstallDriver
534 stub	RnaGetDialSettings
535 stub	RnaSetDialSettings
536 stub	RnaGetIPInfo
537 stub	RnaSetIPInfo
538 stub	RasCreatePhonebookEntryA
539 stub	RasCreatePhonebookEntryW
540 stub	RasDialA
541 stub	RasDialW
542 stub	RasEditPhonebookEntryA
543 stub	RasEditPhonebookEntryW
544 stdcall	RasEnumConnectionsA(ptr ptr ptr) RasEnumConnectionsA
545 stdcall	RasEnumConnectionsW(ptr ptr ptr) RasEnumConnectionsW
546 stdcall	RasEnumEntriesA(str str ptr ptr ptr) RasEnumEntriesA
547 stub	RasEnumEntriesW
548 stub	RasGetConnectStatusA
549 stub	RasGetConnectStatusW
550 stdcall	RasGetEntryDialParamsA(str ptr ptr) RasGetEntryDialParamsA
551 stub	RasGetEntryDialParamsW
552 stub	RasGetErrorStringA
553 stub	RasGetErrorStringW
554 stub	RasGetProjectionInfoA
555 stub	RasGetProjectionInfoW
556 stdcall	RasHangUpA(long) RasHangUpA
557 stub	RasHangUpW
558 stub	RasSetEntryDialParamsA
559 stub	RasSetEntryDialParamsW
560 stub	RnaCloseMac
561 stub	RnaComplete
562 stub	RnaGetDevicePort
563 stub	RnaGetUserProfile
564 stub	RnaOpenMac
565 stub	RnaSessInitialize
566 stub	RnaStartCallback
567 stub	RnaTerminate
568 stub	RnaUICallbackDialog
569 stub	RnaUIUsernamePassword
