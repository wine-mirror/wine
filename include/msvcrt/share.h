/*
 * share.h
 *
 * Constants for file sharing functions.
 *
 * Derived from the Mingw32 header written by Colin Peters.
 * Modified for Wine use by Bill Medland
 * This file is in the public domain.
 *
 * Original header contained the following
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __WINE_SHARE_H
#define __WINE_SHARE_H
#define __WINE_USE_MSVCRT

#define	SH_COMPAT	0x00	/* Compatibility */
#define	SH_DENYRW	0x10	/* Deny read/write */
#define	SH_DENYWR	0x20	/* Deny write */
#define	SH_DENYRD	0x30	/* Deny read */
#define	SH_DENYNO	0x40	/* Deny nothing */

#endif	/* __WINE_SHARE_H_ */

