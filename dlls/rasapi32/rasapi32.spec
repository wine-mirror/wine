@ stub RasAutodialAddressToNetwork
@ stub RasAutodialEntryToNetwork
@ stub RasConnectionNotificationA
@ stub RasConnectionNotificationW
@ stub RasCreatePhonebookEntryA
@ stub RasCreatePhonebookEntryW
@ stdcall RasDeleteEntryA(str str)
@ stdcall RasDeleteEntryW(wstr wstr)
@ stub RasDeleteSubEntryA
@ stub RasDeleteSubEntryW
@ stub RasDialA
@ stub RasDialW
@ stub RasDialWow
@ stub RasEditPhonebookEntryA
@ stub RasEditPhonebookEntryW
@ stdcall RasEnumAutodialAddressesA(ptr ptr ptr)
@ stdcall RasEnumAutodialAddressesW(ptr ptr ptr)
@ stdcall RasEnumConnectionsA(ptr ptr ptr)
@ stdcall RasEnumConnectionsW(ptr ptr ptr)
@ stub RasEnumConnectionsWow
@ stdcall RasEnumDevicesA(ptr ptr ptr)
@ stdcall RasEnumDevicesW(ptr ptr ptr)
@ stdcall RasEnumEntriesA(str str ptr ptr ptr)
@ stdcall RasEnumEntriesW(str str ptr ptr ptr)
@ stub RasEnumEntriesWow
@ stdcall RasGetAutodialAddressA(str ptr ptr ptr ptr)
@ stdcall RasGetAutodialAddressW(wstr ptr ptr ptr ptr)
@ stdcall RasGetAutodialEnableA(long ptr)
@ stdcall RasGetAutodialEnableW(long ptr)
@ stdcall RasGetAutodialParamA(long ptr ptr)
@ stdcall RasGetAutodialParamW(long ptr ptr)
@ stub RasGetConnectResponse
@ stub RasGetConnectStatusA
@ stub RasGetConnectStatusW
@ stub RasGetConnectStatusWow
@ stub RasGetCountryInfoA
@ stub RasGetCountryInfoW
@ stub RasGetCredentialsA
@ stub RasGetCredentialsW
@ stdcall RasGetEntryDialParamsA(str ptr ptr)
@ stub RasGetEntryDialParamsW
@ stub RasGetEntryPropertiesA
@ stub RasGetEntryPropertiesW
@ stub RasGetErrorStringA
@ stub RasGetErrorStringW
@ stub RasGetErrorStringWow
@ stub RasGetHport
@ stub RasGetProjectionInfoA
@ stub RasGetProjectionInfoW
@ stub RasGetSubEntryHandleA
@ stub RasGetSubEntryHandleW
@ stub RasGetSubEntryPropertiesA
@ stub RasGetSubEntryPropertiesW
@ stdcall RasHangUpA(long)
@ stub RasHangUpW
@ stub RasHangUpWow
@ stub RasRenameEntryA
@ stub RasRenameEntryW
@ stdcall RasSetAutodialAddressA(str long ptr long long)
@ stdcall RasSetAutodialAddressW(wstr long ptr long long)
@ stdcall RasSetAutodialEnableA(long long)
@ stdcall RasSetAutodialEnableW(long long)
@ stdcall RasSetAutodialParamA(long ptr long)
@ stdcall RasSetAutodialParamW(long ptr long)
@ stub RasSetCredentialsA
@ stub RasSetCredentialsW
@ stub RasSetEntryDialParamsA
@ stub RasSetEntryDialParamsW
@ stdcall RasSetEntryPropertiesA(str str ptr long ptr long)
@ stdcall RasSetEntryPropertiesW(wstr wstr ptr long ptr long)
@ stub RasSetOldPassword
@ stub RasSetSubEntryPropertiesA
@ stub RasSetSubEntryPropertiesW
@ stdcall RasValidateEntryNameA(str str)
@ stub RasValidateEntryNameW

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
