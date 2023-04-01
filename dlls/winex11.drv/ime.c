/*
 * The IME for interfacing with XIM
 *
 * Copyright 2008 CodeWeavers, Aric Stewart
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

/*
 * Notes:
 *  The normal flow for IMM/IME Processing is as follows.
 * 1) The Keyboard Driver generates key messages which are first passed to
 *    the IMM and then to IME via ImeProcessKey. If the IME returns 0  then
 *    it does not want the key and the keyboard driver then generates the
 *    WM_KEYUP/WM_KEYDOWN messages.  However if the IME is going to process the
 *    key it returns non-zero.
 * 2) If the IME is going to process the key then the IMM calls ImeToAsciiEx to
 *    process the key.  the IME modifies the HIMC structure to reflect the
 *    current state and generates any messages it needs the IMM to process.
 * 3) IMM checks the messages and send them to the application in question. From
 *    here the IMM level deals with if the application is IME aware or not.
 *
 *  This flow does not work well for the X11 driver and XIM.
 *   (It works fine for Mac)
 *  As such we will have to reroute step 1.  Instead the x11drv driver will
 *  generate an XIM events and call directly into this IME implementation.
 *  As such we will have to use the alternative ImmGenerateMessage path to be
 *  generate the messages that we want the IMM layer to send to the application.
 */

#include "x11drv_dll.h"
#include "wine/debug.h"
#include "imm.h"
#include "immdev.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

BOOL WINAPI ImeSelect(HIMC hIMC, BOOL fSelect)
{
    LPINPUTCONTEXT lpIMC;
    TRACE("%p %s\n",hIMC,(fSelect)?"TRUE":"FALSE");

    if (!hIMC || !fSelect) return TRUE;

    /* Initialize our structures */
    lpIMC = ImmLockIMC(hIMC);
    if (lpIMC != NULL)
    {
        LPIMEPRIVATE myPrivate;
        myPrivate = ImmLockIMCC(lpIMC->hPrivate);
        myPrivate->bInComposition = FALSE;
        myPrivate->bInternalState = FALSE;
        myPrivate->textfont = NULL;
        myPrivate->hwndDefault = NULL;
        ImmUnlockIMCC(lpIMC->hPrivate);
        ImmUnlockIMC(hIMC);
    }

    return TRUE;
}
