/*
 * Top level resource file for SETUPX
 *
 * Copyright 2001 Andreas Mohr
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

#include "windef.h"
#include "winuser.h"
#include "winnls.h"
#include "setupapi_private.h"

/*--------------------- FIXME --------------------------
 *
 * These must be seperated into the language files
 * and translated. The language 0,0 is a hack to get it
 * loaded properly for all languages by pretending that
 * they are neutral.
 * The menus are not yet properly implemented.
 * Don't localize it yet. (js)
 */

LANGUAGE 0,0

COPYFILEDLGORD DIALOG LOADONCALL MOVEABLE DISCARDABLE 20, 20, 208, 105
STYLE DS_MODALFRAME | DS_SETFONT | WS_POPUP | WS_VISIBLE | WS_CAPTION
CAPTION "Copying Files..."
FONT 8, "MS Sans Serif"
BEGIN
	PUSHBUTTON "Cancel", IDCANCEL, 79, 84, 50, 14, WS_CHILD | WS_VISIBLE | WS_TABSTOP
	LTEXT "Source:", -1, 7, 7, 77, 11, WS_CHILD | WS_VISIBLE | WS_GROUP
	LTEXT "", SOURCESTRORD, 7, 18, 194, 11, WS_CHILD | WS_VISIBLE | WS_GROUP
	LTEXT "Destination:", -1, 7, 30, 77, 11, WS_CHILD | WS_VISIBLE | WS_GROUP
	LTEXT "", DESTSTRORD, 7, 41, 194, 22, WS_CHILD | WS_VISIBLE | WS_GROUP
	CONTROL "", PROGRESSORD, "setupx_progress", 7, 63, 194, 13, WS_CHILD | WS_VISIBLE | WS_TABSTOP 
END
