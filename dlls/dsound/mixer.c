/*  			DirectSound
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2002 TransGaming Technologies, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>	/* Insomnia - pow() function */

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "mmsystem.h"
#include "winternl.h"
#include "mmddk.h"
#include "wine/windef16.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

void DSOUND_RecalcVolPan(PDSVOLUMEPAN volpan)
{
	double temp;

	/* the AmpFactors are expressed in 16.16 fixed point */
	volpan->dwVolAmpFactor = (ULONG) (pow(2.0, volpan->lVolume / 600.0) * 65536);
	/* FIXME: dwPan{Left|Right}AmpFactor */

	/* FIXME: use calculated vol and pan ampfactors */
	temp = (double) (volpan->lVolume - (volpan->lPan > 0 ? volpan->lPan : 0));
	volpan->dwTotalLeftAmpFactor = (ULONG) (pow(2.0, temp / 600.0) * 65536);
	temp = (double) (volpan->lVolume + (volpan->lPan < 0 ? volpan->lPan : 0));
	volpan->dwTotalRightAmpFactor = (ULONG) (pow(2.0, temp / 600.0) * 65536);

	TRACE("left = %lx, right = %lx\n", volpan->dwTotalLeftAmpFactor, volpan->dwTotalRightAmpFactor);
}

void DSOUND_RecalcFormat(IDirectSoundBufferImpl *dsb)
{
	DWORD sw;

	sw = dsb->wfx.nChannels * (dsb->wfx.wBitsPerSample / 8);
	/* calculate the 10ms write lead */
	dsb->writelead = (dsb->freq / 100) * sw;
}

void DSOUND_CheckEvent(IDirectSoundBufferImpl *dsb, int len)
{
	int			i;
	DWORD			offset;
	LPDSBPOSITIONNOTIFY	event;

	if (dsb->nrofnotifies == 0)
		return;

	TRACE("(%p) buflen = %ld, playpos = %ld, len = %d\n",
		dsb, dsb->buflen, dsb->playpos, len);
	for (i = 0; i < dsb->nrofnotifies ; i++) {
		event = dsb->notifies + i;
		offset = event->dwOffset;
		TRACE("checking %d, position %ld, event = %p\n",
			i, offset, event->hEventNotify);
		/* DSBPN_OFFSETSTOP has to be the last element. So this is */
		/* OK. [Inside DirectX, p274] */
		/*  */
		/* This also means we can't sort the entries by offset, */
		/* because DSBPN_OFFSETSTOP == -1 */
		if (offset == DSBPN_OFFSETSTOP) {
			if (dsb->state == STATE_STOPPED) {
				SetEvent(event->hEventNotify);
				TRACE("signalled event %p (%d)\n", event->hEventNotify, i);
				return;
			} else
				return;
		}
		if ((dsb->playpos + len) >= dsb->buflen) {
			if ((offset < ((dsb->playpos + len) % dsb->buflen)) ||
			    (offset >= dsb->playpos)) {
				TRACE("signalled event %p (%d)\n", event->hEventNotify, i);
				SetEvent(event->hEventNotify);
			}
		} else {
			if ((offset >= dsb->playpos) && (offset < (dsb->playpos + len))) {
				TRACE("signalled event %p (%d)\n", event->hEventNotify, i);
				SetEvent(event->hEventNotify);
			}
		}
	}
}

/* WAV format info can be found at:
 *
 *    http://www.cwi.nl/ftp/audio/AudioFormats.part2
 *    ftp://ftp.cwi.nl/pub/audio/RIFF-format
 *
 * Import points to remember:
 *    8-bit WAV is unsigned
 *    16-bit WAV is signed
 */
 /* Use the same formulas as pcmconverter.c */
static inline INT16 cvtU8toS16(BYTE b)
{
    return (short)((b+(b << 8))-32768);
}

static inline BYTE cvtS16toU8(INT16 s)
{
    return (s >> 8) ^ (unsigned char)0x80;
}

static inline void cp_fields(const IDirectSoundBufferImpl *dsb, BYTE *ibuf, BYTE *obuf )
{
        INT fl,fr;

        if (dsb->wfx.wBitsPerSample == 8)  {
                if (dsound->wfx.wBitsPerSample == 8 &&
                    dsound->wfx.nChannels == dsb->wfx.nChannels) {
                        /* avoid needless 8->16->8 conversion */
                        *obuf=*ibuf;
                        if (dsb->wfx.nChannels==2)
                                *(obuf+1)=*(ibuf+1);
                        return;
                }
                fl = cvtU8toS16(*ibuf);
                fr = (dsb->wfx.nChannels==2 ? cvtU8toS16(*(ibuf + 1)) : fl);
        } else {
                fl = *((INT16 *)ibuf);
                fr = (dsb->wfx.nChannels==2 ? *(((INT16 *)ibuf) + 1)  : fl);
        }

        if (dsound->wfx.nChannels == 2) {
                if (dsound->wfx.wBitsPerSample == 8) {
                        *obuf = cvtS16toU8(fl);
                        *(obuf + 1) = cvtS16toU8(fr);
                        return;
                }
                if (dsound->wfx.wBitsPerSample == 16) {
                        *((INT16 *)obuf) = fl;
                        *(((INT16 *)obuf) + 1) = fr;
                        return;
                }
        }
        if (dsound->wfx.nChannels == 1) {
                fl = (fl + fr) >> 1;
                if (dsound->wfx.wBitsPerSample == 8) {
                        *obuf = cvtS16toU8(fl);
                        return;
                }
                if (dsound->wfx.wBitsPerSample == 16) {
                        *((INT16 *)obuf) = fl;
                        return;
                }
        }
}

/* Now with PerfectPitch (tm) technology */
static INT DSOUND_MixerNorm(IDirectSoundBufferImpl *dsb, BYTE *buf, INT len)
{
	INT	i, size, ipos, ilen;
	BYTE	*ibp, *obp;
	INT	iAdvance = dsb->wfx.nBlockAlign;
	INT	oAdvance = dsb->dsound->wfx.nBlockAlign;

	ibp = dsb->buffer + dsb->buf_mixpos;
	obp = buf;

	TRACE("(%p, %p, %p), buf_mixpos=%ld\n", dsb, ibp, obp, dsb->buf_mixpos);
	/* Check for the best case */
	if ((dsb->freq == dsb->dsound->wfx.nSamplesPerSec) &&
	    (dsb->wfx.wBitsPerSample == dsb->dsound->wfx.wBitsPerSample) &&
	    (dsb->wfx.nChannels == dsb->dsound->wfx.nChannels)) {
	        DWORD bytesleft = dsb->buflen - dsb->buf_mixpos;
		TRACE("(%p) Best case\n", dsb);
	    	if (len <= bytesleft )
			memcpy(obp, ibp, len);
		else { /* wrap */
			memcpy(obp, ibp, bytesleft );
			memcpy(obp + bytesleft, dsb->buffer, len - bytesleft);
		}
		return len;
	}

	/* Check for same sample rate */
	if (dsb->freq == dsb->dsound->wfx.nSamplesPerSec) {
		TRACE("(%p) Same sample rate %ld = primary %ld\n", dsb,
			dsb->freq, dsb->dsound->wfx.nSamplesPerSec);
		ilen = 0;
		for (i = 0; i < len; i += oAdvance) {
			cp_fields(dsb, ibp, obp );
			ibp += iAdvance;
			ilen += iAdvance;
			obp += oAdvance;
			if (ibp >= (BYTE *)(dsb->buffer + dsb->buflen))
				ibp = dsb->buffer;	/* wrap */
		}
		return (ilen);
	}

	/* Mix in different sample rates */
	/* */
	/* New PerfectPitch(tm) Technology (c) 1998 Rob Riggs */
	/* Patent Pending :-] */

	/* Patent enhancements (c) 2000 Ove Kåven,
	 * TransGaming Technologies Inc. */

	/* FIXME("(%p) Adjusting frequency: %ld -> %ld (need optimization)\n",
	   dsb, dsb->freq, dsb->dsound->wfx.nSamplesPerSec); */

	size = len / oAdvance;
	ilen = 0;
	ipos = dsb->buf_mixpos;
	for (i = 0; i < size; i++) {
                cp_fields(dsb, (dsb->buffer + ipos), obp);
		obp += oAdvance;
		dsb->freqAcc += dsb->freqAdjust;
		if (dsb->freqAcc >= (1<<DSOUND_FREQSHIFT)) {
			ULONG adv = (dsb->freqAcc>>DSOUND_FREQSHIFT) * iAdvance;
			dsb->freqAcc &= (1<<DSOUND_FREQSHIFT)-1;
			ipos += adv; ilen += adv;
			while (ipos >= dsb->buflen)
				ipos -= dsb->buflen;
		}
	}
	return ilen;
}

static void DSOUND_MixerVol(IDirectSoundBufferImpl *dsb, BYTE *buf, INT len)
{
	INT	i, inc = dsb->dsound->wfx.wBitsPerSample >> 3;
	BYTE	*bpc = buf;
	INT16	*bps = (INT16 *) buf;

	TRACE("(%p) left = %lx, right = %lx\n", dsb,
		dsb->cvolpan.dwTotalLeftAmpFactor, dsb->cvolpan.dwTotalRightAmpFactor);
	if ((!(dsb->dsbd.dwFlags & DSBCAPS_CTRLPAN) || (dsb->cvolpan.lPan == 0)) &&
	    (!(dsb->dsbd.dwFlags & DSBCAPS_CTRLVOLUME) || (dsb->cvolpan.lVolume == 0)) &&
	    !(dsb->dsbd.dwFlags & DSBCAPS_CTRL3D))
		return;		/* Nothing to do */

	/* If we end up with some bozo coder using panning or 3D sound */
	/* with a mono primary buffer, it could sound very weird using */
	/* this method. Oh well, tough patooties. */

	for (i = 0; i < len; i += inc) {
		INT	val;

		switch (inc) {

		case 1:
			/* 8-bit WAV is unsigned, but we need to operate */
			/* on signed data for this to work properly */
			val = *bpc - 128;
			val = ((val * (i & inc ? dsb->cvolpan.dwTotalRightAmpFactor : dsb->cvolpan.dwTotalLeftAmpFactor)) >> 16);
			*bpc = val + 128;
			bpc++;
			break;
		case 2:
			/* 16-bit WAV is signed -- much better */
			val = *bps;
			val = ((val * ((i & inc) ? dsb->cvolpan.dwTotalRightAmpFactor : dsb->cvolpan.dwTotalLeftAmpFactor)) >> 16);
			*bps = val;
			bps++;
			break;
		default:
			/* Very ugly! */
			FIXME("MixerVol had a nasty error\n");
		}
	}
}

static void *tmp_buffer;
static size_t tmp_buffer_len = 0;

static void *DSOUND_tmpbuffer(size_t len)
{
  if (len>tmp_buffer_len) {
    void *new_buffer = realloc(tmp_buffer, len);
    if (new_buffer) {
      tmp_buffer = new_buffer;
      tmp_buffer_len = len;
    }
    return new_buffer;
  }
  return tmp_buffer;
}

static DWORD DSOUND_MixInBuffer(IDirectSoundBufferImpl *dsb, DWORD writepos, DWORD fraglen)
{
	INT	i, len, ilen, temp, field;
	INT	advance = dsb->dsound->wfx.wBitsPerSample >> 3;
	BYTE	*buf, *ibuf, *obuf;
	INT16	*ibufs, *obufs;

	len = fraglen;
	if (!(dsb->playflags & DSBPLAY_LOOPING)) {
		temp = MulDiv(dsb->dsound->wfx.nAvgBytesPerSec, dsb->buflen,
			dsb->nAvgBytesPerSec) -
		       MulDiv(dsb->dsound->wfx.nAvgBytesPerSec, dsb->buf_mixpos,
			dsb->nAvgBytesPerSec);
		len = (len > temp) ? temp : len;
	}
	len &= ~3;				/* 4 byte alignment */

	if (len == 0) {
		/* This should only happen if we aren't looping and temp < 4 */

		/* We skip the remainder, so check for possible events */
		DSOUND_CheckEvent(dsb, dsb->buflen - dsb->buf_mixpos);
		/* Stop */
		dsb->state = STATE_STOPPED;
		dsb->playpos = 0;
		dsb->last_playpos = 0;
		dsb->buf_mixpos = 0;
		dsb->leadin = FALSE;
		/* Check for DSBPN_OFFSETSTOP */
		DSOUND_CheckEvent(dsb, 0);
		return 0;
	}

	/* Been seeing segfaults in malloc() for some reason... */
	TRACE("allocating buffer (size = %d)\n", len);
	if ((buf = ibuf = (BYTE *) DSOUND_tmpbuffer(len)) == NULL)
		return 0;

	TRACE("MixInBuffer (%p) len = %d, dest = %ld\n", dsb, len, writepos);

	ilen = DSOUND_MixerNorm(dsb, ibuf, len);
	if ((dsb->dsbd.dwFlags & DSBCAPS_CTRLPAN) ||
	    (dsb->dsbd.dwFlags & DSBCAPS_CTRLVOLUME))
		DSOUND_MixerVol(dsb, ibuf, len);

	obuf = dsb->dsound->buffer + writepos;
	for (i = 0; i < len; i += advance) {
		obufs = (INT16 *) obuf;
		ibufs = (INT16 *) ibuf;
		if (dsb->dsound->wfx.wBitsPerSample == 8) {
			/* 8-bit WAV is unsigned */
			field = (*ibuf - 128);
			field += (*obuf - 128);
			field = field > 127 ? 127 : field;
			field = field < -128 ? -128 : field;
			*obuf = field + 128;
		} else {
			/* 16-bit WAV is signed */
			field = *ibufs;
			field += *obufs;
			field = field > 32767 ? 32767 : field;
			field = field < -32768 ? -32768 : field;
			*obufs = field;
		}
		ibuf += advance;
		obuf += advance;
		if (obuf >= (BYTE *)(dsb->dsound->buffer + dsb->dsound->buflen))
			obuf = dsb->dsound->buffer;
	}
	/* free(buf); */

	if (dsb->dsbd.dwFlags & DSBCAPS_CTRLPOSITIONNOTIFY)
		DSOUND_CheckEvent(dsb, ilen);

	if (dsb->leadin && (dsb->startpos > dsb->buf_mixpos) && (dsb->startpos <= dsb->buf_mixpos + ilen)) {
		/* HACK... leadin should be reset when the PLAY position reaches the startpos,
		 * not the MIX position... but if the sound buffer is bigger than our prebuffering
		 * (which must be the case for the streaming buffers that need this hack anyway)
		 * plus DS_HEL_MARGIN or equivalent, then this ought to work anyway. */
		dsb->leadin = FALSE;
	}

	dsb->buf_mixpos += ilen;

	if (dsb->buf_mixpos >= dsb->buflen) {
		if (!(dsb->playflags & DSBPLAY_LOOPING)) {
			dsb->state = STATE_STOPPED;
			dsb->playpos = 0;
			dsb->last_playpos = 0;
			dsb->buf_mixpos = 0;
			dsb->leadin = FALSE;
			DSOUND_CheckEvent(dsb, 0);		/* For DSBPN_OFFSETSTOP */
		} else {
			/* wrap */
			while (dsb->buf_mixpos >= dsb->buflen)
				dsb->buf_mixpos -= dsb->buflen;
			if (dsb->leadin && (dsb->startpos <= dsb->buf_mixpos))
				dsb->leadin = FALSE; /* HACK: see above */
		}
	}

	return len;
}

static void DSOUND_PhaseCancel(IDirectSoundBufferImpl *dsb, DWORD writepos, DWORD len)
{
	INT     i, ilen, field;
	INT     advance = dsb->dsound->wfx.wBitsPerSample >> 3;
	BYTE	*buf, *ibuf, *obuf;
	INT16	*ibufs, *obufs;

	len &= ~3;				/* 4 byte alignment */

	TRACE("allocating buffer (size = %ld)\n", len);
	if ((buf = ibuf = (BYTE *) DSOUND_tmpbuffer(len)) == NULL)
		return;

	TRACE("PhaseCancel (%p) len = %ld, dest = %ld\n", dsb, len, writepos);

	ilen = DSOUND_MixerNorm(dsb, ibuf, len);
	if ((dsb->dsbd.dwFlags & DSBCAPS_CTRLPAN) ||
	    (dsb->dsbd.dwFlags & DSBCAPS_CTRLVOLUME))
		DSOUND_MixerVol(dsb, ibuf, len);

	/* subtract instead of add, to phase out premixed data */
	obuf = dsb->dsound->buffer + writepos;
	for (i = 0; i < len; i += advance) {
		obufs = (INT16 *) obuf;
		ibufs = (INT16 *) ibuf;
		if (dsb->dsound->wfx.wBitsPerSample == 8) {
			/* 8-bit WAV is unsigned */
			field = (*ibuf - 128);
			field -= (*obuf - 128);
			field = field > 127 ? 127 : field;
			field = field < -128 ? -128 : field;
			*obuf = field + 128;
		} else {
			/* 16-bit WAV is signed */
			field = *ibufs;
			field -= *obufs;
			field = field > 32767 ? 32767 : field;
			field = field < -32768 ? -32768 : field;
			*obufs = field;
		}
		ibuf += advance;
		obuf += advance;
		if (obuf >= (BYTE *)(dsb->dsound->buffer + dsb->dsound->buflen))
			obuf = dsb->dsound->buffer;
	}
	/* free(buf); */
}

static void DSOUND_MixCancel(IDirectSoundBufferImpl *dsb, DWORD writepos, BOOL cancel)
{
	DWORD   size, flen, len, npos, nlen;
	INT	iAdvance = dsb->wfx.nBlockAlign;
	INT	oAdvance = dsb->dsound->wfx.nBlockAlign;
	/* determine amount of premixed data to cancel */
	DWORD primary_done =
		((dsb->primary_mixpos < writepos) ? dsb->dsound->buflen : 0) +
		dsb->primary_mixpos - writepos;

	TRACE("(%p, %ld), buf_mixpos=%ld\n", dsb, writepos, dsb->buf_mixpos);

	/* backtrack the mix position */
	size = primary_done / oAdvance;
	flen = size * dsb->freqAdjust;
	len = (flen >> DSOUND_FREQSHIFT) * iAdvance;
	flen &= (1<<DSOUND_FREQSHIFT)-1;
	while (dsb->freqAcc < flen) {
		len += iAdvance;
		dsb->freqAcc += 1<<DSOUND_FREQSHIFT;
	}
	len %= dsb->buflen;
	npos = ((dsb->buf_mixpos < len) ? dsb->buflen : 0) +
		dsb->buf_mixpos - len;
	if (dsb->leadin && (dsb->startpos > npos) && (dsb->startpos <= npos + len)) {
		/* stop backtracking at startpos */
		npos = dsb->startpos;
		len = ((dsb->buf_mixpos < npos) ? dsb->buflen : 0) +
			dsb->buf_mixpos - npos;
		flen = dsb->freqAcc;
		nlen = len / dsb->wfx.nBlockAlign;
		nlen = ((nlen << DSOUND_FREQSHIFT) + flen) / dsb->freqAdjust;
		nlen *= dsb->dsound->wfx.nBlockAlign;
		writepos =
			((dsb->primary_mixpos < nlen) ? dsb->dsound->buflen : 0) +
			dsb->primary_mixpos - nlen;
	}

	dsb->freqAcc -= flen;
	dsb->buf_mixpos = npos;
	dsb->primary_mixpos = writepos;

	TRACE("new buf_mixpos=%ld, primary_mixpos=%ld (len=%ld)\n",
	      dsb->buf_mixpos, dsb->primary_mixpos, len);

	if (cancel) DSOUND_PhaseCancel(dsb, writepos, len);
}

void DSOUND_MixCancelAt(IDirectSoundBufferImpl *dsb, DWORD buf_writepos)
{
#if 0
	DWORD   i, size, flen, len, npos, nlen;
	INT	iAdvance = dsb->wfx.nBlockAlign;
	INT	oAdvance = dsb->dsound->wfx.nBlockAlign;
	/* determine amount of premixed data to cancel */
	DWORD buf_done =
		((dsb->buf_mixpos < buf_writepos) ? dsb->buflen : 0) +
		dsb->buf_mixpos - buf_writepos;
#endif

	WARN("(%p, %ld), buf_mixpos=%ld\n", dsb, buf_writepos, dsb->buf_mixpos);
	/* since this is not implemented yet, just cancel *ALL* prebuffering for now
	 * (which is faster anyway when there's only a single secondary buffer) */
	dsb->dsound->need_remix = TRUE;
}

void DSOUND_ForceRemix(IDirectSoundBufferImpl *dsb)
{
	EnterCriticalSection(&dsb->lock);
	if (dsb->state == STATE_PLAYING) {
#if 0 /* this may not be quite reliable yet */
		dsb->need_remix = TRUE;
#else
		dsb->dsound->need_remix = TRUE;
#endif
	}
	LeaveCriticalSection(&dsb->lock);
}

static DWORD DSOUND_MixOne(IDirectSoundBufferImpl *dsb, DWORD playpos, DWORD writepos, DWORD mixlen)
{
	DWORD len, slen;
	/* determine this buffer's write position */
	DWORD buf_writepos = DSOUND_CalcPlayPosition(dsb, dsb->state & dsb->dsound->state, writepos,
						     writepos, dsb->primary_mixpos, dsb->buf_mixpos);
	/* determine how much already-mixed data exists */
	DWORD buf_done =
		((dsb->buf_mixpos < buf_writepos) ? dsb->buflen : 0) +
		dsb->buf_mixpos - buf_writepos;
	DWORD primary_done =
		((dsb->primary_mixpos < writepos) ? dsb->dsound->buflen : 0) +
		dsb->primary_mixpos - writepos;
	DWORD adv_done =
		((dsb->dsound->mixpos < writepos) ? dsb->dsound->buflen : 0) +
		dsb->dsound->mixpos - writepos;
	int still_behind;

	TRACE("buf_writepos=%ld, primary_writepos=%ld\n", buf_writepos, writepos);
	TRACE("buf_done=%ld, primary_done=%ld\n", buf_done, primary_done);
	TRACE("buf_mixpos=%ld, primary_mixpos=%ld, mixlen=%ld\n", dsb->buf_mixpos, dsb->primary_mixpos,
	      mixlen);
	TRACE("looping=%ld, startpos=%ld, leadin=%ld\n", dsb->playflags, dsb->startpos, dsb->leadin);

	/* save write position for non-GETCURRENTPOSITION2... */
	dsb->playpos = buf_writepos;

	/* check whether CalcPlayPosition detected a mixing underrun */
	if ((buf_done == 0) && (dsb->primary_mixpos != writepos)) {
		/* it did, but did we have more to play? */
		if ((dsb->playflags & DSBPLAY_LOOPING) ||
		    (dsb->buf_mixpos < dsb->buflen)) {
			/* yes, have to recover */
			ERR("underrun on sound buffer %p\n", dsb);
			TRACE("recovering from underrun: primary_mixpos=%ld\n", writepos);
		}
		dsb->primary_mixpos = writepos;
		primary_done = 0;
	}
	/* determine how far ahead we should mix */
	if (((dsb->playflags & DSBPLAY_LOOPING) ||
	     (dsb->leadin && (dsb->probably_valid_to != 0))) &&
	    !(dsb->dsbd.dwFlags & DSBCAPS_STATIC)) {
		/* if this is a streaming buffer, it typically means that
		 * we should defer mixing past probably_valid_to as long
		 * as we can, to avoid unnecessary remixing */
		/* the heavy-looking calculations shouldn't be that bad,
		 * as any game isn't likely to be have more than 1 or 2
		 * streaming buffers in use at any time anyway... */
		DWORD probably_valid_left =
			(dsb->probably_valid_to == (DWORD)-1) ? dsb->buflen :
			((dsb->probably_valid_to < buf_writepos) ? dsb->buflen : 0) +
			dsb->probably_valid_to - buf_writepos;
		/* check for leadin condition */
		if ((probably_valid_left == 0) &&
		    (dsb->probably_valid_to == dsb->startpos) &&
		    dsb->leadin)
			probably_valid_left = dsb->buflen;
		TRACE("streaming buffer probably_valid_to=%ld, probably_valid_left=%ld\n",
		      dsb->probably_valid_to, probably_valid_left);
		/* check whether the app's time is already up */
		if (probably_valid_left < dsb->writelead) {
			WARN("probably_valid_to now within writelead, possible streaming underrun\n");
			/* once we pass the point of no return,
			 * no reason to hold back anymore */
			dsb->probably_valid_to = (DWORD)-1;
			/* we just have to go ahead and mix what we have,
			 * there's no telling what the app is thinking anyway */
		} else {
			/* adjust for our frequency and our sample size */
			probably_valid_left = MulDiv(probably_valid_left,
						     1 << DSOUND_FREQSHIFT,
						     dsb->wfx.nBlockAlign * dsb->freqAdjust) *
				              dsb->dsound->wfx.nBlockAlign;
			/* check whether to clip mix_len */
			if (probably_valid_left < mixlen) {
				TRACE("clipping to probably_valid_left=%ld\n", probably_valid_left);
				mixlen = probably_valid_left;
			}
		}
	}
	/* cut mixlen with what's already been mixed */
	if (mixlen < primary_done) {
		/* huh? and still CalcPlayPosition didn't
		 * detect an underrun? */
		FIXME("problem with underrun detection (mixlen=%ld < primary_done=%ld)\n", mixlen, primary_done);
		return 0;
	}
	len = mixlen - primary_done;
	TRACE("remaining mixlen=%ld\n", len);

	if (len < dsb->dsound->fraglen) {
		/* smaller than a fragment, wait until it gets larger
		 * before we take the mixing overhead */
		TRACE("mixlen not worth it, deferring mixing\n");
		return 0;
	}

	/* ok, we know how much to mix, let's go */
	still_behind = (adv_done > primary_done);
	while (len) {
		slen = dsb->dsound->buflen - dsb->primary_mixpos;
		if (slen > len) slen = len;
		slen = DSOUND_MixInBuffer(dsb, dsb->primary_mixpos, slen);

		if ((dsb->primary_mixpos < dsb->dsound->mixpos) &&
		    (dsb->primary_mixpos + slen >= dsb->dsound->mixpos))
			still_behind = FALSE;

		dsb->primary_mixpos += slen; len -= slen;
		while (dsb->primary_mixpos >= dsb->dsound->buflen)
			dsb->primary_mixpos -= dsb->dsound->buflen;

		if ((dsb->state == STATE_STOPPED) || !slen) break;
	}
	TRACE("new primary_mixpos=%ld, primary_advbase=%ld\n", dsb->primary_mixpos, dsb->dsound->mixpos);
	TRACE("mixed data len=%ld, still_behind=%d\n", mixlen-len, still_behind);
	/* return how far we think the primary buffer can
	 * advance its underrun detector...*/
	if (still_behind) return 0;
	if ((mixlen - len) < primary_done) return 0;
	slen = ((dsb->primary_mixpos < dsb->dsound->mixpos) ?
		dsb->dsound->buflen : 0) + dsb->primary_mixpos -
		dsb->dsound->mixpos;
	if (slen > mixlen) {
		/* the primary_done and still_behind checks above should have worked */
		FIXME("problem with advancement calculation (advlen=%ld > mixlen=%ld)\n", slen, mixlen);
		slen = 0;
	}
	return slen;
}

static DWORD DSOUND_MixToPrimary(DWORD playpos, DWORD writepos, DWORD mixlen, BOOL recover)
{
	INT			i, len, maxlen = 0;
	IDirectSoundBufferImpl	*dsb;

	TRACE("(%ld,%ld,%ld)\n", playpos, writepos, mixlen);
	for (i = dsound->nrofbuffers - 1; i >= 0; i--) {
		dsb = dsound->buffers[i];

		if (!dsb || !(ICOM_VTBL(dsb)))
			continue;
		if (dsb->buflen && dsb->state && !dsb->hwbuf) {
			TRACE("Checking %p, mixlen=%ld\n", dsb, mixlen);
			EnterCriticalSection(&(dsb->lock));
			if (dsb->state == STATE_STOPPING) {
				DSOUND_MixCancel(dsb, writepos, TRUE);
				dsb->state = STATE_STOPPED;
			} else {
				if ((dsb->state == STATE_STARTING) || recover) {
					dsb->primary_mixpos = writepos;
					memcpy(&dsb->cvolpan, &dsb->volpan, sizeof(dsb->cvolpan));
					dsb->need_remix = FALSE;
				}
				else if (dsb->need_remix) {
					DSOUND_MixCancel(dsb, writepos, TRUE);
					memcpy(&dsb->cvolpan, &dsb->volpan, sizeof(dsb->cvolpan));
					dsb->need_remix = FALSE;
				}
				len = DSOUND_MixOne(dsb, playpos, writepos, mixlen);
				if (dsb->state == STATE_STARTING)
					dsb->state = STATE_PLAYING;
				maxlen = (len > maxlen) ? len : maxlen;
			}
			LeaveCriticalSection(&(dsb->lock));
		}
	}

	return maxlen;
}

static void DSOUND_MixReset(DWORD writepos)
{
	INT			i;
	IDirectSoundBufferImpl	*dsb;
	int nfiller;

	TRACE("(%ld)\n", writepos);

	/* the sound of silence */
	nfiller = dsound->wfx.wBitsPerSample == 8 ? 128 : 0;

	/* reset all buffer mix positions */
	for (i = dsound->nrofbuffers - 1; i >= 0; i--) {
		dsb = dsound->buffers[i];

		if (!dsb || !(ICOM_VTBL(dsb)))
			continue;
		if (dsb->buflen && dsb->state && !dsb->hwbuf) {
			TRACE("Resetting %p\n", dsb);
			EnterCriticalSection(&(dsb->lock));
			if (dsb->state == STATE_STOPPING) {
				dsb->state = STATE_STOPPED;
			}
			else if (dsb->state == STATE_STARTING) {
				/* nothing */
			} else {
				DSOUND_MixCancel(dsb, writepos, FALSE);
				memcpy(&dsb->cvolpan, &dsb->volpan, sizeof(dsb->cvolpan));
				dsb->need_remix = FALSE;
			}
			LeaveCriticalSection(&(dsb->lock));
		}
	}

	/* wipe out premixed data */
	if (dsound->mixpos < writepos) {
		memset(dsound->buffer + writepos, nfiller, dsound->buflen - writepos);
		memset(dsound->buffer, nfiller, dsound->mixpos);
	} else {
		memset(dsound->buffer + writepos, nfiller, dsound->mixpos - writepos);
	}

	/* reset primary mix position */
	dsound->mixpos = writepos;
}

static void DSOUND_CheckReset(IDirectSoundImpl *dsound, DWORD writepos)
{
	if (dsound->need_remix) {
		DSOUND_MixReset(writepos);
		dsound->need_remix = FALSE;
		/* maximize Half-Life performance */
		dsound->prebuf = ds_snd_queue_min;
		dsound->precount = 0;
	} else {
		dsound->precount++;
		if (dsound->precount >= 4) {
			if (dsound->prebuf < ds_snd_queue_max)
				dsound->prebuf++;
			dsound->precount = 0;
		}
	}
	TRACE("premix adjust: %d\n", dsound->prebuf);
}

void DSOUND_WaveQueue(IDirectSoundImpl *dsound, DWORD mixq)
{
	if (mixq + dsound->pwqueue > ds_hel_queue) mixq = ds_hel_queue - dsound->pwqueue;
	TRACE("queueing %ld buffers, starting at %d\n", mixq, dsound->pwwrite);
	for (; mixq; mixq--) {
		waveOutWrite(dsound->hwo, dsound->pwave[dsound->pwwrite], sizeof(WAVEHDR));
		dsound->pwwrite++;
		if (dsound->pwwrite >= DS_HEL_FRAGS) dsound->pwwrite = 0;
		dsound->pwqueue++;
	}
}

/* #define SYNC_CALLBACK */

void DSOUND_PerformMix(void)
{
	int nfiller;
	BOOL forced;
	HRESULT hres;

	RtlAcquireResourceShared(&(dsound->lock), TRUE);

	if (!dsound || !dsound->ref) {
		/* seems the dsound object is currently being released */
		RtlReleaseResource(&(dsound->lock));
		return;
	}

	/* the sound of silence */
	nfiller = dsound->wfx.wBitsPerSample == 8 ? 128 : 0;

	/* whether the primary is forced to play even without secondary buffers */
	forced = ((dsound->state == STATE_PLAYING) || (dsound->state == STATE_STARTING));

        TRACE("entering at %ld\n", GetTickCount());
	if (dsound->priolevel != DSSCL_WRITEPRIMARY) {
		BOOL paused = ((dsound->state == STATE_STOPPED) || (dsound->state == STATE_STARTING));
		/* FIXME: document variables */
 		DWORD playpos, writepos, inq, maxq, frag;
 		if (dsound->hwbuf) {
			hres = IDsDriverBuffer_GetPosition(dsound->hwbuf, &playpos, &writepos);
			if (hres) {
			    RtlReleaseResource(&(dsound->lock));
			    return;
			}
			/* Well, we *could* do Just-In-Time mixing using the writepos,
			 * but that's a little bit ambitious and unnecessary... */
			/* rather add our safety margin to the writepos, if we're playing */
			if (!paused) {
				writepos += dsound->writelead;
				while (writepos >= dsound->buflen)
					writepos -= dsound->buflen;
			} else writepos = playpos;
		}
		else {
 			playpos = dsound->pwplay * dsound->fraglen;
 			writepos = playpos;
 			if (!paused) {
	 			writepos += ds_hel_margin * dsound->fraglen;
	 			while (writepos >= dsound->buflen)
 					writepos -= dsound->buflen;
	 		}
		}
		TRACE("primary playpos=%ld, writepos=%ld, clrpos=%ld, mixpos=%ld\n",
		      playpos,writepos,dsound->playpos,dsound->mixpos);
		/* wipe out just-played sound data */
		if (playpos < dsound->playpos) {
			memset(dsound->buffer + dsound->playpos, nfiller, dsound->buflen - dsound->playpos);
			memset(dsound->buffer, nfiller, playpos);
		} else {
			memset(dsound->buffer + dsound->playpos, nfiller, playpos - dsound->playpos);
		}
		dsound->playpos = playpos;

		EnterCriticalSection(&(dsound->mixlock));

		/* reset mixing if necessary */
		DSOUND_CheckReset(dsound, writepos);

		/* check how much prebuffering is left */
		inq = dsound->mixpos;
		if (inq < writepos)
			inq += dsound->buflen;
		inq -= writepos;

		/* find the maximum we can prebuffer */
		if (!paused) {
			maxq = playpos;
			if (maxq < writepos)
				maxq += dsound->buflen;
			maxq -= writepos;
		} else maxq = dsound->buflen;

		/* clip maxq to dsound->prebuf */
		frag = dsound->prebuf * dsound->fraglen;
		if (maxq > frag) maxq = frag;

		/* check for consistency */
		if (inq > maxq) {
			/* the playback position must have passed our last
			 * mixed position, i.e. it's an underrun, or we have
			 * nothing more to play */
			TRACE("reached end of mixed data (inq=%ld, maxq=%ld)\n", inq, maxq);
			inq = 0;
			/* stop the playback now, to allow buffers to refill */
			if (dsound->state == STATE_PLAYING) {
				dsound->state = STATE_STARTING;
			}
			else if (dsound->state == STATE_STOPPING) {
				dsound->state = STATE_STOPPED;
			}
			else {
				/* how can we have an underrun if we aren't playing? */
				WARN("unexpected primary state (%ld)\n", dsound->state);
			}
#ifdef SYNC_CALLBACK
			/* DSOUND_callback may need this lock */
			LeaveCriticalSection(&(dsound->mixlock));
#endif
			/* FIXME: OSS doesn't allow independent stopping of input and output streams */
			/* in full duplex mode so don't stop when capturing. This should be moved into */
			/* the OSS driver someday. */ 
			if ( (dsound_capture == NULL) || (dsound_capture->state != STATE_CAPTURING) )
				DSOUND_PrimaryStop(dsound);
#ifdef SYNC_CALLBACK
			EnterCriticalSection(&(dsound->mixlock));
#endif
			if (dsound->hwbuf) {
				/* the Stop is supposed to reset play position to beginning of buffer */
				/* unfortunately, OSS is not able to do so, so get current pointer */
				hres = IDsDriverBuffer_GetPosition(dsound->hwbuf, &playpos, NULL);
				if (hres) {
					LeaveCriticalSection(&(dsound->mixlock));
					RtlReleaseResource(&(dsound->lock));
					return;
				}
			} else {
	 			playpos = dsound->pwplay * dsound->fraglen;
			}
			writepos = playpos;
			dsound->playpos = playpos;
			dsound->mixpos = writepos;
			inq = 0;
			maxq = dsound->buflen;
			if (maxq > frag) maxq = frag;
			memset(dsound->buffer, nfiller, dsound->buflen);
			paused = TRUE;
		}

		/* do the mixing */
		frag = DSOUND_MixToPrimary(playpos, writepos, maxq, paused);
		if (forced) frag = maxq - inq;
		dsound->mixpos += frag;
		while (dsound->mixpos >= dsound->buflen)
			dsound->mixpos -= dsound->buflen;

		if (frag) {
			/* buffers have been filled, restart playback */
			if (dsound->state == STATE_STARTING) {
				dsound->state = STATE_PLAYING;
			}
			else if (dsound->state == STATE_STOPPED) {
				/* the dsound is supposed to play if there's something to play
				 * even if it is reported as stopped, so don't let this confuse you */
				dsound->state = STATE_STOPPING;
			}
			LeaveCriticalSection(&(dsound->mixlock));
			if (paused) {
				DSOUND_PrimaryPlay(dsound);
				TRACE("starting playback\n");
			}
		}
		else
			LeaveCriticalSection(&(dsound->mixlock));
	} else {
		/* in the DSSCL_WRITEPRIMARY mode, the app is totally in charge... */
		if (dsound->state == STATE_STARTING) {
			DSOUND_PrimaryPlay(dsound);
			dsound->state = STATE_PLAYING;
		}
		else if (dsound->state == STATE_STOPPING) {
			DSOUND_PrimaryStop(dsound);
			dsound->state = STATE_STOPPED;
		}
	}
	TRACE("completed processing at %ld\n", GetTickCount());
	RtlReleaseResource(&(dsound->lock));
}

void CALLBACK DSOUND_timer(UINT timerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	if (!dsound) {
		ERR("dsound died without killing us?\n");
		timeKillEvent(timerID);
		timeEndPeriod(DS_TIME_RES);
		return;
	}

	TRACE("entered\n");
	DSOUND_PerformMix();
}

void CALLBACK DSOUND_callback(HWAVEOUT hwo, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
        IDirectSoundImpl* This = (IDirectSoundImpl*)dwUser;
	TRACE("entering at %ld, msg=%08x\n", GetTickCount(), msg);
	if (msg == MM_WOM_DONE) {
		DWORD inq, mixq, fraglen, buflen, pwplay, playpos, mixpos;
		if (This->pwqueue == (DWORD)-1) {
			TRACE("completed due to reset\n");
			return;
		}
/* it could be a bad idea to enter critical section here... if there's lock contention,
 * the resulting scheduling delays might obstruct the winmm player thread */
#ifdef SYNC_CALLBACK
		EnterCriticalSection(&(This->mixlock));
#endif
		/* retrieve current values */
		fraglen = dsound->fraglen;
		buflen = dsound->buflen;
		pwplay = dsound->pwplay;
		playpos = pwplay * fraglen;
		mixpos = dsound->mixpos;
		/* check remaining mixed data */
		inq = ((mixpos < playpos) ? buflen : 0) + mixpos - playpos;
		mixq = inq / fraglen;
		if ((inq - (mixq * fraglen)) > 0) mixq++;
		/* complete the playing buffer */
		TRACE("done playing primary pos=%ld\n", playpos);
		pwplay++;
		if (pwplay >= DS_HEL_FRAGS) pwplay = 0;
		/* write new values */
		dsound->pwplay = pwplay;
		dsound->pwqueue--;
		/* queue new buffer if we have data for it */
		if (inq>1) DSOUND_WaveQueue(This, inq-1);
#ifdef SYNC_CALLBACK
		LeaveCriticalSection(&(This->mixlock));
#endif
	}
	TRACE("completed\n");
}
