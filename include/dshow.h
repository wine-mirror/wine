/*
 * Copyright (C) 2001 Hidenori Takeshima
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

#ifndef __WINE_DSHOW_H
#define __WINE_DSHOW_H

#include "windows.h"
#include "windowsx.h"
#include "olectl.h"
#include "ddraw.h"
#include "mmsystem.h"

#include "strmif.h"
#include "amvideo.h"
#include "amaudio.h"
#include "control.h"
#include "evcode.h"
#include "uuids.h"
#include "errors.h"
/*#include "edevdefs.h"*/  /* not yet */
#include "audevcod.h"
/*#include "dvdevcod.h"*/  /* not yet */

#ifndef AM_NOVTABLE
#define AM_NOVTABLE
#endif /* AM_NOVTABLE */

#ifndef NUMELEMS
#define NUMELEMS(elem)  (sizeof(elem)/sizeof((elem)[0]))
#endif  /* NUMELEMS */

#ifndef OATRUE
#define OATRUE  (-1)
#endif  /* OATRUE */
#ifndef OAFALSE
#define OAFALSE (0)
#endif  /* OAFALSE */


#endif /* __WINE_DSHOW_H */
