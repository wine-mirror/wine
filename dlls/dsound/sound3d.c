/*  			DirectSound
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2001 TransGaming Technologies, Inc.
 * Copyright 2002 Rok Mandeljc <rok.mandeljc@gimb.org>
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
/*
 * Most thread locking is complete. There may be a few race
 * conditions still lurking.
 *
 * Tested with a Soundblaster clone, a Gravis UltraSound Classic,
 * and a Turtle Beach Tropez+.
 *
 * TODO:
 *	Implement SetCooperativeLevel properly (need to address focus issues)
 *	Implement DirectSound3DBuffers (stubs in place)
 *	Use hardware 3D support if available
 *      Add critical section locking inside Release and AddRef methods
 *      Handle static buffers - put those in hardware, non-static not in hardware
 *      Hardware DuplicateSoundBuffer
 *      Proper volume calculation, and setting volume in HEL primary buffer
 *      Optimize WINMM and negotiate fragment size, decrease DS_HEL_MARGIN
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
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

WINE_DEFAULT_DEBUG_CHANNEL(dsound3d);

/*******************************************************************************
 *              Auxiliary functions
 */

/* scalar product (i believe it's called dot product in english) */
static inline D3DVALUE ScalarProduct (LPD3DVECTOR a, LPD3DVECTOR b)
{
	D3DVALUE c;
	c = (a->u1.x*b->u1.x) + (a->u2.y*b->u2.y) + (a->u3.z*b->u3.z);
	TRACE("(%f,%f,%f) * (%f,%f,%f) = %f)\n", a->u1.x, a->u2.y, a->u3.z, b->u1.x, b->u2.y, \
	      b->u3.z, c);
	return c;
}

/* vector product (i believe it's called cross product in english */
static inline LPD3DVECTOR VectorProduct (LPD3DVECTOR a, LPD3DVECTOR b)
{
	LPD3DVECTOR c;
	c->u1.x = (a->u2.y*b->u3.z) - (a->u3.z*b->u2.y);
	c->u2.y = (a->u3.z*b->u1.x) - (a->u1.x*b->u3.z);
	c->u3.z = (a->u1.x*b->u2.y) - (a->u2.y*b->u1.x);
	TRACE("(%f,%f,%f) x (%f,%f,%f) = (%f,%f,%f)\n", a->u1.x, a->u2.y, a->u3.z, b->u1.x, b->u2.y, \
	      b->u3.z, c->u1.x, c->u2.y, c->u3.z);
	return c;
}

/* magnitude (lenght) of vector */
static inline D3DVALUE VectorMagnitude (LPD3DVECTOR a)
{
	D3DVALUE l;
	l = sqrt (ScalarProduct (a, a));
	TRACE("|(%f,%f,%f)| = %f\n", a->u1.x, a->u2.y, a->u3.z, l);
	return l;
}

/* conversion between radians and degrees */
static inline DWORD RadToDeg (DWORD angle)
{
	DWORD newangle;
	newangle = angle * (360/(2*M_PI));
	TRACE("%ld rad = %ld deg\n", angle, newangle);
	return newangle;
}

/* conversion between degrees and radians */
static inline DWORD DegToRad (DWORD angle)
{
	DWORD newangle;
	newangle = angle * (2*M_PI/360);
	TRACE("%ld deg = %ld rad\n", angle, newangle);
	return newangle;
}

/* angle between vectors */
static inline DWORD AngleBetweenVectorsDeg (LPD3DVECTOR a, LPD3DVECTOR b)
{
	DWORD angle, cos;
	D3DVALUE la, lb, product;
	/* definition of scalar product: a*b = |a|*|b|*cos...therefore: */
	product = ScalarProduct (a,b);
	la = VectorMagnitude (a);
	lb = VectorMagnitude (b);
	cos = product/(la*lb);
	/* we now have angle in radians */
	angle = RadToDeg(cos);
	TRACE("angle between (%f,%f,%f) and (%f,%f,%f) = %ld degrees\n",  a->u1.x, a->u2.y, a->u3.z, b->u1.x, \
	      b->u2.y, b->u3.z, angle);
	return angle;	
}

/* calculates vector between two points */
static inline D3DVECTOR VectorBetweenTwoPoints (LPD3DVECTOR a, LPD3DVECTOR b)
{
	D3DVECTOR c;
	c.u1.x = b->u1.x - a->u1.x;
	c.u2.y = b->u2.y - a->u2.y;
	c.u3.z = b->u3.z - a->u3.z;
	TRACE("A (%f,%f,%f), B (%f,%f,%f), AB = (%f,%f,%f)\n", a->u1.x, a->u2.y, a->u3.z, b->u1.x, b->u2.y, \
	      b->u3.z, c.u1.x, c.u2.y, c.u3.z);
	return c;
}

/*******************************************************************************
 *              3D Buffer and Listener mixing
 */

static void WINAPI DSOUND_Mix3DBuffer(IDirectSound3DBufferImpl *ds3db)
{
	IDirectSound3DListenerImpl *dsl;
	
	/* volume, at which the sound will be played after all calcs. */
	LONG lVolume;
	/* attuneation (temp variable) */
	LONG lAttuneation;
	int iPower;
	/* stuff for distance related stuff calc. */
	D3DVECTOR vDistance;
	D3DVALUE fDistance;
	
	/* stuff for cone angle calc. */
	DWORD dwAlpha, dwTheta, dwInsideConeAngle, dwOutsideConeAngle;
	D3DVECTOR vConeOrientation;
	DWORD dwConstVolAng; /* Volume/Angle constant */
	
	if (ds3db->dsb->dsound->listener == NULL)
		return;
	
	dsl = ds3db->dsb->dsound->listener;
	
	switch (ds3db->ds3db.dwMode)
	{
		case DS3DMODE_NORMAL:
		{
			/* initial volume */
			lVolume = ds3db->lVolume;

			/* distance attuneation stuff */
			vDistance = VectorBetweenTwoPoints(&ds3db->ds3db.vPosition, &dsl->ds3dl.vPosition);
			fDistance = VectorMagnitude (&vDistance);
			
			if (fDistance > ds3db->ds3db.flMaxDistance)
			{
				/* some apps don't want you to hear too distant sounds... */
				if (ds3db->dsb->dsbd.dwFlags & DSBCAPS_MUTE3DATMAXDISTANCE)
				{
					ds3db->dsb->volpan.lVolume = DSBVOLUME_MIN;
					DSOUND_RecalcVolPan (&ds3db->dsb->volpan);		
					/* i guess mixing here would be a waste of power */
					return;
				}
				else
					fDistance = ds3db->ds3db.flMaxDistance;
			}
			
			if (fDistance < ds3db->ds3db.flMinDistance)
				fDistance = ds3db->ds3db.flMinDistance;
			
			/* the formula my dad and i have figured out after reading msdn info about min/max distance
			   ...hope it works */
			iPower = fDistance/ds3db->ds3db.flMinDistance - 1; /* this sucks, but for unknown reason damn thing works only if you reduce it for 1 */
			lAttuneation = (((pow(2, iPower) - 1)*DSBVOLUME_MIN) + lVolume)/pow(2, iPower);
			lAttuneation *= dsl->ds3dl.flRolloffFactor; /* attuneation according to rolloff factor */
			lAttuneation /= 5; /* i've figured this value wih trying (without it, sound is too quiet */
			TRACE ("distance att.: distance = %f, min distance = %f => adjusting volume %ld for attuneation %ld\n", fDistance, ds3db->ds3db.flMinDistance, lVolume, lAttuneation);
			lVolume += lAttuneation;
			
			/* conning */
			/* correct me if I'm wrong, but i believe 'D3DVECTORS' used below are only points
			   between which vectors are yet to be calculated */
			vConeOrientation = VectorBetweenTwoPoints(&ds3db->ds3db.vPosition, &ds3db->ds3db.vConeOrientation);
			/* I/O ConeAngles are defined in both directions; for comparing, we need only half of their values */
			dwOutsideConeAngle = ds3db->ds3db.dwOutsideConeAngle / 2;
			dwInsideConeAngle = ds3db->ds3db.dwInsideConeAngle / 2;
			dwTheta = AngleBetweenVectorsDeg (&vDistance, &vConeOrientation);
			/* actual conning */
			if (dwTheta <= dwInsideConeAngle)
			{
				lAttuneation = 0;
				TRACE("conning: angle (%ld) < InsideConeAngle (%ld), leaving volume at %ld\n", dwTheta, dwInsideConeAngle, lVolume);
			}
			if (dwTheta > dwOutsideConeAngle)
			{
				/* attuneation is equal to lConeOutsideVolume */
				lAttuneation = ds3db->ds3db.lConeOutsideVolume;
				TRACE("conning: angle (%ld) > OutsideConeAngle (%ld), attuneation = %ld, final volume = %ld\n", dwTheta, dwOutsideConeAngle, \
				      ds3db->ds3db.lConeOutsideVolume, lVolume);
			}
			if (dwTheta > dwInsideConeAngle && dwTheta <= dwOutsideConeAngle)
			{
				dwAlpha = dwTheta - dwInsideConeAngle;
				dwConstVolAng = ((lVolume + ds3db->ds3db.lConeOutsideVolume) - lVolume) / (dwOutsideConeAngle - dwInsideConeAngle);
				lAttuneation = dwAlpha*dwConstVolAng;
				TRACE("conning: angle = %ld, attuneation = %ld, final Volume = %ld\n", dwTheta, dwAlpha*dwConstVolAng, lVolume);
			}
			lVolume += lAttuneation;
			TRACE("final volume = %ld\n", lVolume);		
			
			/* at last, we got the desired volume */
			ds3db->dsb->volpan.lVolume = lVolume;
			DSOUND_RecalcVolPan (&ds3db->dsb->volpan);
			DSOUND_ForceRemix (ds3db->dsb);			
			break;
		}
		case DS3DMODE_HEADRELATIVE:
		case DS3DMODE_DISABLE:
			DSOUND_RecalcVolPan (&ds3db->dsb->volpan);
			DSOUND_ForceRemix (ds3db->dsb);
			break;
	}
}

static void WINAPI DSOUND_ChangeListener(IDirectSound3DListenerImpl *ds3dl)
{
	int i;
	for (i = 0; i < ds3dl->dsb->dsound->nrofbuffers; i++)
	{
		/* some buffers don't have 3d buffer (Ultima IX seems to
		crash without the following line) */
		if (ds3dl->dsb->dsound->buffers[i]->ds3db == NULL)
			continue;
		if (ds3dl->dsb->dsound->buffers[i]->ds3db->need_recalc == TRUE)
		{
			DSOUND_Mix3DBuffer(ds3dl->dsb->dsound->buffers[i]->ds3db);
		}
			
	}
}

/*******************************************************************************
 *              IDirectSound3DBuffer
 */

/* IUnknown methods */
static HRESULT WINAPI IDirectSound3DBufferImpl_QueryInterface(
	LPDIRECTSOUND3DBUFFER iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);

	TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
	return IDirectSoundBuffer_QueryInterface((LPDIRECTSOUNDBUFFER8)This->dsb, riid, ppobj);
}

static ULONG WINAPI IDirectSound3DBufferImpl_AddRef(LPDIRECTSOUND3DBUFFER iface)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	ULONG ulReturn;

	TRACE("(%p) ref was %ld\n", This, This->ref);
	ulReturn = InterlockedIncrement(&This->ref);
	if (ulReturn == 1)
		IDirectSoundBuffer8_AddRef((LPDIRECTSOUNDBUFFER8)This->dsb);
	return ulReturn;
}

static ULONG WINAPI IDirectSound3DBufferImpl_Release(LPDIRECTSOUND3DBUFFER iface)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	ULONG ulReturn;

	TRACE("(%p) ref was %ld\n", This, This->ref);

	ulReturn = InterlockedDecrement(&This->ref);
	if(ulReturn)
		return ulReturn;

	if (This->dsb) {
		BOOL std = (This->dsb->dsbd.dwFlags & DSBCAPS_CTRL3D);

		IDirectSoundBuffer8_Release((LPDIRECTSOUNDBUFFER8)This->dsb);

		if (std)
			return 0; /* leave it to IDirectSoundBufferImpl_Release */
	}

	if (This->dsb->ds3db == This) This->dsb->ds3db = NULL;

	DeleteCriticalSection(&This->lock);

	HeapFree(GetProcessHeap(),0,This);

	return 0;
}

/* IDirectSound3DBuffer methods */
static HRESULT WINAPI IDirectSound3DBufferImpl_GetAllParameters(
	LPDIRECTSOUND3DBUFFER iface,
	LPDS3DBUFFER lpDs3dBuffer)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("returning: all parameters\n");
	*lpDs3dBuffer = This->ds3db;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetConeAngles(
	LPDIRECTSOUND3DBUFFER iface,
	LPDWORD lpdwInsideConeAngle,
	LPDWORD lpdwOutsideConeAngle)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("returning: Inside Cone Angle = %ld degrees; Outside Cone Angle = %ld degrees\n", This->ds3db.dwInsideConeAngle, This->ds3db.dwOutsideConeAngle);
	*lpdwInsideConeAngle = This->ds3db.dwInsideConeAngle;
	*lpdwOutsideConeAngle = This->ds3db.dwOutsideConeAngle;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetConeOrientation(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVECTOR lpvConeOrientation)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("returning: Cone Orientation vector = (%f,%f,%f)\n", This->ds3db.vConeOrientation.u1.x, This->ds3db.vConeOrientation.u2.y, This->ds3db.vConeOrientation.u3.z);
	*lpvConeOrientation = This->ds3db.vConeOrientation;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetConeOutsideVolume(
	LPDIRECTSOUND3DBUFFER iface,
	LPLONG lplConeOutsideVolume)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("returning: Cone Outside Volume = %ld\n", This->ds3db.lConeOutsideVolume);
	*lplConeOutsideVolume = This->ds3db.lConeOutsideVolume;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetMaxDistance(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVALUE lpfMaxDistance)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("returning: Max Distance = %f\n", This->ds3db.flMaxDistance);
	*lpfMaxDistance = This->ds3db.flMaxDistance;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetMinDistance(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVALUE lpfMinDistance)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("returning: Min Distance = %f\n", This->ds3db.flMinDistance);
	*lpfMinDistance = This->ds3db.flMinDistance;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetMode(
	LPDIRECTSOUND3DBUFFER iface,
	LPDWORD lpdwMode)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("returning: Mode = %ld\n", This->ds3db.dwMode);
	*lpdwMode = This->ds3db.dwMode;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetPosition(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVECTOR lpvPosition)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("returning: Position vector = (%f,%f,%f)\n", This->ds3db.vPosition.u1.x, This->ds3db.vPosition.u2.y, This->ds3db.vPosition.u1.x);
	*lpvPosition = This->ds3db.vPosition;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetVelocity(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVECTOR lpvVelocity)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("returning: Velocity vector = (%f,%f,%f)\n", This->ds3db.vVelocity.u1.x, This->ds3db.vVelocity.u2.y, This->ds3db.vVelocity.u3.z);
	*lpvVelocity = This->ds3db.vVelocity;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetAllParameters(
	LPDIRECTSOUND3DBUFFER iface,
	LPCDS3DBUFFER lpcDs3dBuffer,
	DWORD dwApply)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("setting: all parameters; dwApply = %ld\n", dwApply);
	This->ds3db = *lpcDs3dBuffer;
	if (dwApply == DS3D_IMMEDIATE)
	{
		DSOUND_Mix3DBuffer(This);
	}
	This->need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetConeAngles(
	LPDIRECTSOUND3DBUFFER iface,
	DWORD dwInsideConeAngle,
	DWORD dwOutsideConeAngle,
	DWORD dwApply)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("setting: Inside Cone Angle = %ld; Outside Cone Angle = %ld; dwApply = %ld\n", dwInsideConeAngle, dwOutsideConeAngle, dwApply);
	This->ds3db.dwInsideConeAngle = dwInsideConeAngle;
	This->ds3db.dwOutsideConeAngle = dwOutsideConeAngle;
	if (dwApply == DS3D_IMMEDIATE)
	{
		DSOUND_Mix3DBuffer(This);
	}
	This->need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetConeOrientation(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("setting: Cone Orientation vector = (%f,%f,%f); dwApply = %ld\n", x, y, z, dwApply);
	This->ds3db.vConeOrientation.u1.x = x;
	This->ds3db.vConeOrientation.u2.y = y;
	This->ds3db.vConeOrientation.u3.z = z;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->need_recalc = FALSE;
		DSOUND_Mix3DBuffer(This);
	}
	This->need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetConeOutsideVolume(
	LPDIRECTSOUND3DBUFFER iface,
	LONG lConeOutsideVolume,
	DWORD dwApply)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("setting: ConeOutsideVolume = %ld; dwApply = %ld\n", lConeOutsideVolume, dwApply);
	This->ds3db.lConeOutsideVolume = lConeOutsideVolume;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->need_recalc = FALSE;
		DSOUND_Mix3DBuffer(This);
	}
	This->need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetMaxDistance(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE fMaxDistance,
	DWORD dwApply)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("setting: MaxDistance = %f; dwApply = %ld\n", fMaxDistance, dwApply);
	This->ds3db.flMaxDistance = fMaxDistance;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->need_recalc = FALSE;
		DSOUND_Mix3DBuffer(This);
	}
	This->need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetMinDistance(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE fMinDistance,
	DWORD dwApply)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("setting: MinDistance = %f; dwApply = %ld\n", fMinDistance, dwApply);
	This->ds3db.flMinDistance = fMinDistance;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->need_recalc = FALSE;
		DSOUND_Mix3DBuffer(This);
	}
	This->need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetMode(
	LPDIRECTSOUND3DBUFFER iface,
	DWORD dwMode,
	DWORD dwApply)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("setting: Mode = %ld; dwApply = %ld\n", dwMode, dwApply);
	This->ds3db.dwMode = dwMode;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->need_recalc = FALSE;
		DSOUND_Mix3DBuffer(This);
	}
	This->need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetPosition(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("setting: Position vector = (%f,%f,%f); dwApply = %ld\n", x, y, z, dwApply);
	This->ds3db.vPosition.u1.x = x;
	This->ds3db.vPosition.u2.y = y;
	This->ds3db.vPosition.u3.z = z;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->need_recalc = FALSE;
		DSOUND_Mix3DBuffer(This);
	}
	This->need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetVelocity(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("setting: Velocity vector = (%f,%f,%f); dwApply = %ld\n", x, y, z, dwApply);
	This->ds3db.vVelocity.u1.x = x;
	This->ds3db.vVelocity.u2.y = y;
	This->ds3db.vVelocity.u3.z = z;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->need_recalc = FALSE;
		DSOUND_Mix3DBuffer(This);
	}
	This->need_recalc = TRUE;
	return DS_OK;
}

static ICOM_VTABLE(IDirectSound3DBuffer) ds3dbvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown methods */
	IDirectSound3DBufferImpl_QueryInterface,
	IDirectSound3DBufferImpl_AddRef,
	IDirectSound3DBufferImpl_Release,
	/* IDirectSound3DBuffer methods */
	IDirectSound3DBufferImpl_GetAllParameters,
	IDirectSound3DBufferImpl_GetConeAngles,
	IDirectSound3DBufferImpl_GetConeOrientation,
	IDirectSound3DBufferImpl_GetConeOutsideVolume,
	IDirectSound3DBufferImpl_GetMaxDistance,
	IDirectSound3DBufferImpl_GetMinDistance,
	IDirectSound3DBufferImpl_GetMode,
	IDirectSound3DBufferImpl_GetPosition,
	IDirectSound3DBufferImpl_GetVelocity,
	IDirectSound3DBufferImpl_SetAllParameters,
	IDirectSound3DBufferImpl_SetConeAngles,
	IDirectSound3DBufferImpl_SetConeOrientation,
	IDirectSound3DBufferImpl_SetConeOutsideVolume,
	IDirectSound3DBufferImpl_SetMaxDistance,
	IDirectSound3DBufferImpl_SetMinDistance,
	IDirectSound3DBufferImpl_SetMode,
	IDirectSound3DBufferImpl_SetPosition,
	IDirectSound3DBufferImpl_SetVelocity,
};

HRESULT WINAPI IDirectSound3DBufferImpl_Create(
	IDirectSoundBufferImpl *This,
	IDirectSound3DBufferImpl **pds3db)
{
	IDirectSound3DBufferImpl *ds3db;

	ds3db = (IDirectSound3DBufferImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(*ds3db));
	ds3db->ref = 0;
	ds3db->dsb = This;
	ICOM_VTBL(ds3db) = &ds3dbvt;
	InitializeCriticalSection(&ds3db->lock);

	ds3db->ds3db.dwSize = sizeof(DS3DBUFFER);
	ds3db->ds3db.vPosition.u1.x = 0.0;
	ds3db->ds3db.vPosition.u2.y = 0.0;
	ds3db->ds3db.vPosition.u3.z = 0.0;
	ds3db->ds3db.vVelocity.u1.x = 0.0;
	ds3db->ds3db.vVelocity.u2.y = 0.0;
	ds3db->ds3db.vVelocity.u3.z = 0.0;
	ds3db->ds3db.dwInsideConeAngle = DS3D_DEFAULTCONEANGLE;
	ds3db->ds3db.dwOutsideConeAngle = DS3D_DEFAULTCONEANGLE;
	ds3db->ds3db.vConeOrientation.u1.x = 0.0;
	ds3db->ds3db.vConeOrientation.u2.y = 0.0;
	ds3db->ds3db.vConeOrientation.u3.z = 0.0;
	ds3db->ds3db.lConeOutsideVolume = DS3D_DEFAULTCONEOUTSIDEVOLUME;
	ds3db->ds3db.flMinDistance = DS3D_DEFAULTMINDISTANCE;
	ds3db->ds3db.flMaxDistance = DS3D_DEFAULTMAXDISTANCE;
	ds3db->ds3db.dwMode = DS3DMODE_NORMAL;

	*pds3db = ds3db;
	return S_OK;
}

/*******************************************************************************
 *	      IDirectSound3DListener
 */

/* IUnknown methods */
static HRESULT WINAPI IDirectSound3DListenerImpl_QueryInterface(
	LPDIRECTSOUND3DLISTENER iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);

	TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
	return IDirectSoundBuffer_QueryInterface((LPDIRECTSOUNDBUFFER8)This->dsb, riid, ppobj);
}

static ULONG WINAPI IDirectSound3DListenerImpl_AddRef(LPDIRECTSOUND3DLISTENER iface)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IDirectSound3DListenerImpl_Release(LPDIRECTSOUND3DLISTENER iface)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	ULONG ulReturn;

	TRACE("(%p) ref was %ld\n", This, This->ref);

	ulReturn = InterlockedDecrement(&This->ref);

	/* Free all resources */
	if( ulReturn == 0 ) {
		if(This->dsb)
			IDirectSoundBuffer8_Release((LPDIRECTSOUNDBUFFER8)This->dsb);
		DeleteCriticalSection(&This->lock);
		HeapFree(GetProcessHeap(),0,This);
	}

	return ulReturn;
}

/* IDirectSound3DListener methods */
static HRESULT WINAPI IDirectSound3DListenerImpl_GetAllParameter(
	LPDIRECTSOUND3DLISTENER iface,
	LPDS3DLISTENER lpDS3DL)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	TRACE("returning: all parameters\n");
	*lpDS3DL = This->ds3dl;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetDistanceFactor(
	LPDIRECTSOUND3DLISTENER iface,
	LPD3DVALUE lpfDistanceFactor)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	TRACE("returning: Distance Factor = %f\n", This->ds3dl.flDistanceFactor);
	*lpfDistanceFactor = This->ds3dl.flDistanceFactor;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetDopplerFactor(
	LPDIRECTSOUND3DLISTENER iface,
	LPD3DVALUE lpfDopplerFactor)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	TRACE("returning: Doppler Factor = %f\n", This->ds3dl.flDopplerFactor);
	*lpfDopplerFactor = This->ds3dl.flDopplerFactor;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetOrientation(
	LPDIRECTSOUND3DLISTENER iface,
	LPD3DVECTOR lpvOrientFront,
	LPD3DVECTOR lpvOrientTop)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	TRACE("returning: OrientFront vector = (%f,%f,%f); OrientTop vector = (%f,%f,%f)\n", This->ds3dl.vOrientFront.u1.x, \
	This->ds3dl.vOrientFront.u2.y, This->ds3dl.vOrientFront.u3.z, This->ds3dl.vOrientTop.u1.x, This->ds3dl.vOrientTop.u2.y, \
	This->ds3dl.vOrientTop.u3.z);
	*lpvOrientFront = This->ds3dl.vOrientFront;
	*lpvOrientTop = This->ds3dl.vOrientTop;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetPosition(
	LPDIRECTSOUND3DLISTENER iface,
	LPD3DVECTOR lpvPosition)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	TRACE("returning: Position vector = (%f,%f,%f)\n", This->ds3dl.vPosition.u1.x, This->ds3dl.vPosition.u2.y, This->ds3dl.vPosition.u3.z);
	*lpvPosition = This->ds3dl.vPosition;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetRolloffFactor(
	LPDIRECTSOUND3DLISTENER iface,
	LPD3DVALUE lpfRolloffFactor)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	TRACE("returning: RolloffFactor = %f\n", This->ds3dl.flRolloffFactor);
	*lpfRolloffFactor = This->ds3dl.flRolloffFactor;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetVelocity(
	LPDIRECTSOUND3DLISTENER iface,
	LPD3DVECTOR lpvVelocity)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	TRACE("returning: Velocity vector = (%f,%f,%f)\n", This->ds3dl.vVelocity.u1.x, This->ds3dl.vVelocity.u2.y, This->ds3dl.vVelocity.u3.z);
	*lpvVelocity = This->ds3dl.vVelocity;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetAllParameters(
	LPDIRECTSOUND3DLISTENER iface,
	LPCDS3DLISTENER lpcDS3DL,
	DWORD dwApply)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	TRACE("setting: all parameters; dwApply = %ld\n", dwApply);
	This->ds3dl = *lpcDS3DL;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetDistanceFactor(
	LPDIRECTSOUND3DLISTENER iface,
	D3DVALUE fDistanceFactor,
	DWORD dwApply)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	TRACE("setting: Distance Factor = %f; dwApply = %ld\n", fDistanceFactor, dwApply);
	This->ds3dl.flDistanceFactor = fDistanceFactor;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetDopplerFactor(
	LPDIRECTSOUND3DLISTENER iface,
	D3DVALUE fDopplerFactor,
	DWORD dwApply)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	TRACE("setting: Doppler Factor = %f; dwApply = %ld\n", fDopplerFactor, dwApply);
	This->ds3dl.flDopplerFactor = fDopplerFactor;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetOrientation(
	LPDIRECTSOUND3DLISTENER iface,
	D3DVALUE xFront, D3DVALUE yFront, D3DVALUE zFront,
	D3DVALUE xTop, D3DVALUE yTop, D3DVALUE zTop,
	DWORD dwApply)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	TRACE("setting: Front vector = (%f,%f,%f); Top vector = (%f,%f,%f); dwApply = %ld\n", \
	xFront, yFront, zFront, xTop, yTop, zTop, dwApply);
	This->ds3dl.vOrientFront.u1.x = xFront;
	This->ds3dl.vOrientFront.u2.y = yFront;
	This->ds3dl.vOrientFront.u3.z = zFront;
	This->ds3dl.vOrientTop.u1.x = xTop;
	This->ds3dl.vOrientTop.u2.y = yTop;
	This->ds3dl.vOrientTop.u3.z = zTop;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetPosition(
	LPDIRECTSOUND3DLISTENER iface,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	TRACE("setting: Position vector = (%f,%f,%f); dwApply = %ld\n", x, y, z, dwApply);
	This->ds3dl.vPosition.u1.x = x;
	This->ds3dl.vPosition.u2.y = y;
	This->ds3dl.vPosition.u3.z = z;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetRolloffFactor(
	LPDIRECTSOUND3DLISTENER iface,
	D3DVALUE fRolloffFactor,
	DWORD dwApply)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	TRACE("setting: Rolloff Factor = %f; dwApply = %ld\n", fRolloffFactor, dwApply);
	This->ds3dl.flRolloffFactor = fRolloffFactor;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetVelocity(
	LPDIRECTSOUND3DLISTENER iface,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	TRACE("setting: Velocity vector = (%f,%f,%f); dwApply = %ld\n", x, y, z, dwApply);
	This->ds3dl.vVelocity.u1.x = x;
	This->ds3dl.vVelocity.u2.y = y;
	This->ds3dl.vVelocity.u3.z = z;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_CommitDeferredSettings(
	LPDIRECTSOUND3DLISTENER iface)

{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	TRACE("\n");
	DSOUND_ChangeListener(This);
	return DS_OK;
}

static ICOM_VTABLE(IDirectSound3DListener) ds3dlvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown methods */
	IDirectSound3DListenerImpl_QueryInterface,
	IDirectSound3DListenerImpl_AddRef,
	IDirectSound3DListenerImpl_Release,
	/* IDirectSound3DListener methods */
	IDirectSound3DListenerImpl_GetAllParameter,
	IDirectSound3DListenerImpl_GetDistanceFactor,
	IDirectSound3DListenerImpl_GetDopplerFactor,
	IDirectSound3DListenerImpl_GetOrientation,
	IDirectSound3DListenerImpl_GetPosition,
	IDirectSound3DListenerImpl_GetRolloffFactor,
	IDirectSound3DListenerImpl_GetVelocity,
	IDirectSound3DListenerImpl_SetAllParameters,
	IDirectSound3DListenerImpl_SetDistanceFactor,
	IDirectSound3DListenerImpl_SetDopplerFactor,
	IDirectSound3DListenerImpl_SetOrientation,
	IDirectSound3DListenerImpl_SetPosition,
	IDirectSound3DListenerImpl_SetRolloffFactor,
	IDirectSound3DListenerImpl_SetVelocity,
	IDirectSound3DListenerImpl_CommitDeferredSettings,
};

HRESULT WINAPI IDirectSound3DListenerImpl_Create(
	PrimaryBufferImpl *This,
	IDirectSound3DListenerImpl **pdsl)
{
	IDirectSound3DListenerImpl *dsl;

	dsl = (IDirectSound3DListenerImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(*dsl));
	dsl->ref = 1;
	ICOM_VTBL(dsl) = &ds3dlvt;

	dsl->ds3dl.dwSize = sizeof(DS3DLISTENER);
	dsl->ds3dl.vPosition.u1.x = 0.0;
	dsl->ds3dl.vPosition.u2.y = 0.0;
	dsl->ds3dl.vPosition.u3.z = 0.0;
	dsl->ds3dl.vVelocity.u1.x = 0.0;
	dsl->ds3dl.vVelocity.u2.y = 0.0;
	dsl->ds3dl.vVelocity.u3.z = 0.0;
	dsl->ds3dl.vOrientFront.u1.x = 0.0;
	dsl->ds3dl.vOrientFront.u2.y = 0.0;
	dsl->ds3dl.vOrientFront.u3.z = 1.0;
	dsl->ds3dl.vOrientTop.u1.x = 0.0;
	dsl->ds3dl.vOrientTop.u2.y = 1.0;
	dsl->ds3dl.vOrientTop.u3.z = 0.0;
	dsl->ds3dl.flDistanceFactor = DS3D_DEFAULTDISTANCEFACTOR;
	dsl->ds3dl.flRolloffFactor = DS3D_DEFAULTROLLOFFFACTOR;
	dsl->ds3dl.flDopplerFactor = DS3D_DEFAULTDOPPLERFACTOR;

	InitializeCriticalSection(&dsl->lock);

	dsl->dsb = This;
	IDirectSoundBuffer8_AddRef((LPDIRECTSOUNDBUFFER8)This);

	*pdsl = dsl;
	return S_OK;
}
