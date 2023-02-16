@ stub ClassInstall32
@ stub SCardAccessNewReaderEvent
@ stub SCardReleaseAllEvents
@ stub SCardReleaseNewReaderEvent
@ stdcall SCardAccessStartedEvent()
@ stdcall SCardAddReaderToGroupA(long str str)
@ stdcall SCardAddReaderToGroupW(long wstr wstr)
@ stdcall SCardBeginTransaction(long)
@ stdcall SCardCancel(long)
@ stdcall SCardConnectA(long str long long ptr ptr)
@ stdcall SCardConnectW(long wstr long long ptr ptr)
@ stdcall SCardControl(long long ptr long ptr long ptr)
@ stdcall SCardDisconnect(long long)
@ stdcall SCardEndTransaction(long long)
@ stdcall SCardEstablishContext(long ptr ptr ptr)
@ stub SCardForgetCardTypeA
@ stub SCardForgetCardTypeW
@ stub SCardForgetReaderA
@ stub SCardForgetReaderGroupA
@ stub SCardForgetReaderGroupW
@ stub SCardForgetReaderW
@ stdcall SCardFreeMemory(long ptr)
@ stdcall SCardGetAttrib(long long ptr ptr)
@ stub SCardGetCardTypeProviderNameA
@ stub SCardGetCardTypeProviderNameW
@ stub SCardGetProviderIdA
@ stub SCardGetProviderIdW
@ stdcall SCardGetStatusChangeA(long long ptr long)
@ stdcall SCardGetStatusChangeW(long long ptr long)
@ stub SCardIntroduceCardTypeA
@ stub SCardIntroduceCardTypeW
@ stub SCardIntroduceReaderA
@ stub SCardIntroduceReaderGroupA
@ stub SCardIntroduceReaderGroupW
@ stub SCardIntroduceReaderW
@ stdcall SCardIsValidContext(long)
@ stdcall SCardListCardsA(long ptr ptr long str ptr)
@ stub SCardListCardsW
@ stub SCardListInterfacesA
@ stub SCardListInterfacesW
@ stdcall SCardListReaderGroupsA(long ptr ptr)
@ stdcall SCardListReaderGroupsW(long ptr ptr)
@ stdcall SCardListReadersA(long str ptr ptr)
@ stdcall SCardListReadersW(long wstr ptr ptr)
@ stub SCardLocateCardsA
@ stub SCardLocateCardsByATRA
@ stub SCardLocateCardsByATRW
@ stub SCardLocateCardsW
@ stdcall SCardReconnect(long long long long ptr)
@ stdcall SCardReleaseContext(long)
@ stdcall SCardReleaseStartedEvent()
@ stub SCardRemoveReaderFromGroupA
@ stub SCardRemoveReaderFromGroupW
@ stdcall SCardSetAttrib(long long ptr long)
@ stub SCardSetCardTypeProviderNameA
@ stub SCardSetCardTypeProviderNameW
@ stub SCardState
@ stdcall SCardStatusA(long str ptr ptr ptr ptr ptr)
@ stdcall SCardStatusW(long wstr ptr ptr ptr ptr ptr)
@ stdcall SCardTransmit(long ptr ptr long ptr ptr ptr)
@ extern g_rgSCardRawPci
@ extern g_rgSCardT0Pci
@ extern g_rgSCardT1Pci
