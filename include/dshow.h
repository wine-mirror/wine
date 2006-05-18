/*
 * Copyright (C) 2002 Alexandre Julliard
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __DSHOW_INCLUDED__
#define __DSHOW_INCLUDED__

#define AM_NOVTABLE

#include <windef.h>
#include <wingdi.h>
#include <objbase.h>
#include <ddraw.h>
#include <mmsystem.h>

#ifndef NUMELMS
#define NUMELMS(array) (sizeof(array)/sizeof((array)[0]))
#endif

#include <strmif.h>
/*#include <amvideo.h>*/
/*#include <amaudio.h>*/
#include <control.h>
/*#include <evcode.h>*/
#include <uuids.h>
#include <errors.h>
#include <audevcod.h>

#ifndef OATRUE
#define OATRUE (-1)
#endif
#ifndef OAFALSE
#define OAFALSE (0)
#endif

#endif /* __DSHOW_INCLUDED__ */
