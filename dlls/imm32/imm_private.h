/*
 * Copyright 2023 Rémi Bernon for CodeWeavers
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

#include <stdarg.h>
#include <stddef.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"

#include "imm.h"
#include "immdev.h"
#include "ntuser.h"
#include "objbase.h"

#include "wine/debug.h"
#include "wine/list.h"

extern HMODULE imm32_module;

/* MSIME messages */
extern UINT WM_MSIME_SERVICE;
extern UINT WM_MSIME_RECONVERTOPTIONS;
extern UINT WM_MSIME_MOUSE;
extern UINT WM_MSIME_RECONVERTREQUEST;
extern UINT WM_MSIME_RECONVERT;
extern UINT WM_MSIME_QUERYPOSITION;
extern UINT WM_MSIME_DOCUMENTFEED;
