/*
 * Copyright 1999 Marcus Meissner
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

#ifndef __WINE_MSVIDEO_PRIVATE_H
#define __WINE_MSVIDEO_PRIVATE_H

/* HIC struct (same layout as Win95 one) */
typedef struct tagWINE_HIC {
	DWORD		magic;		/* 00: 'Smag' */
	HANDLE		curthread;	/* 04: */
	DWORD		type;		/* 08: */
	DWORD		handler;	/* 0C: */
	HDRVR		hdrv;		/* 10: */
	DWORD		private;	/* 14:(handled by SendDriverMessage)*/
	DRIVERPROC	driverproc;	/* 18:(handled by SendDriverMessage)*/
	DWORD		x1;		/* 1c: name? */
	WORD		x2;		/* 20: */
	DWORD		x3;		/* 22: */
					/* 26: */
        DRIVERPROC16    driverproc16;   /* Wine specific flags */
} WINE_HIC;

HIC     MSVIDEO_OpenFunction(DWORD, DWORD, UINT, DRIVERPROC, DRIVERPROC16);
LRESULT MSVIDEO_SendMessage(HIC, UINT, DWORD, DWORD);

extern LRESULT  (CALLBACK *pFnCallTo16)(HDRVR, HIC, UINT, LPARAM, LPARAM);

/* handle16 --> handle conversions */
#define HDRAWDIB_32(h16)	((HDRAWDIB)(ULONG_PTR)(h16))
#define HIC_32(h16)		((HIC)(ULONG_PTR)(h16))

/* handle --> handle16 conversions */
#define HDRVR_16(h32)		(LOWORD(h32))
#define HDRAWDIB_16(h32)	(LOWORD(h32))
#define HIC_16(h32)		(LOWORD(h32))

#endif  /* __WINE_MSVIDEO_PRIVATE_H */
