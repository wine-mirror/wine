/*
 * OleUIPasteSpecial implementation
 *
 * Copyright 2006 Huw Davies
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

#define COM_NO_WINDOWS_H
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "oledlg.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/***********************************************************************
 *           OleUIPasteSpecialA (OLEDLG.4)
 */
UINT WINAPI OleUIPasteSpecialA(LPOLEUIPASTESPECIALA lpOleUIPasteSpecial)
{
  FIXME("(%p): stub\n", lpOleUIPasteSpecial);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIPasteSpecialW (OLEDLG.22)
 */
UINT WINAPI OleUIPasteSpecialW(LPOLEUIPASTESPECIALW lpOleUIPasteSpecial)
{
  FIXME("(%p): stub\n", lpOleUIPasteSpecial);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}
