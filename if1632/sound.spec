# $Id: sound.spec,v 1.3 1993/07/04 04:04:21 root Exp root $
#
name	sound
id	7
length	20

1    pascal  OPENSOUND() OpenSound()
2    pascal  CLOSESOUND() CloseSound()
3    pascal  SETVOICEQUEUESIZE(word word) SetVoiceQueueSize(1 2)
4    pascal  SETVOICENOTE(word word word word) SetVoiceNote(1 2 3 4)
5    pascal  SETVOICEACCENT(word word word word word) 
		       SetVoiceAccent(1 2 3 4 5)
6    pascal  SETVOICEENVELOPE(word word word) SetVoiceEnvelope(1 2 3)
7    pascal  SETSOUNDNOISE(word word) SetSoundNoise(1 2)
8    pascal  SETVOICESOUND(word long word) SetVoiceSound(1 2 3)
9    pascal  STARTSOUND() StartSound()
10   pascal  STOPSOUND() StopSound()
11   pascal  WAITSOUNDSTATE(word) WaitSoundState(1)
12   pascal  SYNCALLVOICES() SyncAllVoices()
13   pascal  COUNTVOICENOTES(word) CountVoiceNotes(1)
14   pascal  GETTHRESHOLDEVENT() GetThresholdEvent()
15   pascal  GETTHRESHOLDSTATUS() GetThresholdStatus()
16   pascal  SETVOICETHRESHOLD(word word) SetVoiceThreshold(1 2)
17   pascal  DOBEEP() DoBeep()
