@ stub ClassInstall32
@ stub SCardAccessNewReaderEvent
@ stub SCardReleaseAllEvents
@ stub SCardReleaseNewReaderEvent
@ stdcall SCardAccessStartedEvent()
@ stdcall SCardAddReaderToGroupA(long str str)
@ stdcall SCardAddReaderToGroupW(long wstr wstr)
@ stub SCardBeginTransaction
@ stub SCardCancel
@ stub SCardConnectA
@ stub SCardConnectW
@ stub SCardControl
@ stub SCardDisconnect
@ stub SCardEndTransaction
@ stdcall SCardEstablishContext(long ptr ptr ptr)
@ stub SCardForgetCardTypeA
@ stub SCardForgetCardTypeW
@ stub SCardForgetReaderA
@ stub SCardForgetReaderGroupA
@ stub SCardForgetReaderGroupW
@ stub SCardForgetReaderW
@ stub SCardFreeMemory
@ stub SCardGetAttrib
@ stub SCardGetCardTypeProviderNameA
@ stub SCardGetCardTypeProviderNameW
@ stub SCardGetProviderIdA
@ stub SCardGetProviderIdW
@ stub SCardGetStatusChangeA
@ stub SCardGetStatusChangeW
@ stub SCardIntroduceCardTypeA
@ stub SCardIntroduceCardTypeW
@ stub SCardIntroduceReaderA
@ stub SCardIntroduceReaderGroupA
@ stub SCardIntroduceReaderGroupW
@ stub SCardIntroduceReaderW
@ stdcall SCardIsValidContext(long)
@ stdcall SCardListCardsA(long ptr ptr long str long)
@ stub SCardListCardsW
@ stub SCardListInterfacesA
@ stub SCardListInterfacesW
@ stub SCardListReaderGroupsA
@ stub SCardListReaderGroupsW
@ stdcall SCardListReadersA(long str ptr ptr)
@ stdcall SCardListReadersW(long wstr ptr ptr)
@ stub SCardLocateCardsA
@ stub SCardLocateCardsByATRA
@ stub SCardLocateCardsByATRW
@ stub SCardLocateCardsW
@ stub SCardReconnect
@ stdcall SCardReleaseContext(long)
@ stdcall SCardReleaseStartedEvent()
@ stub SCardRemoveReaderFromGroupA
@ stub SCardRemoveReaderFromGroupW
@ stub SCardSetAttrib
@ stub SCardSetCardTypeProviderNameA
@ stub SCardSetCardTypeProviderNameW
@ stub SCardState
@ stdcall SCardStatusA (long str long long long ptr long )
@ stdcall SCardStatusW (long wstr long long long ptr long )
@ stub SCardTransmit
@ extern g_rgSCardRawPci
@ extern g_rgSCardT0Pci
@ extern g_rgSCardT1Pci
