/*
 * Cards dll implementation
 *
 * Copyright (C) 2004 Sami Nopanen
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"

#include "cards.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cards);


void WINAPI cdtTerm(void);


static HINSTANCE hInst;
static int cardWidth;
static int cardHeight;
static HBITMAP cardBitmaps[CARD_MAX + 1];


/***********************************************************************
 * Initializes the cards.dll library. Loads the card bitmaps from the
 * resources, and initializes the card size variables.
 */
BOOL WINAPI cdtInit(int *width, int *height)
{
	BITMAP bm;
	int i;

	TRACE("(%p, %p)\n", width, height);

	for(i = 0; i <= CARD_MAX; i++)
		cardBitmaps[i] = 0;

	for(i = 0; i <= CARD_MAX; i++)
	{
		cardBitmaps[i] = LoadBitmapA(hInst, MAKEINTRESOURCEA(i));
		if(cardBitmaps[i] == 0)
		{
			cdtTerm();
			return FALSE;
		}
	}

	GetObjectA(cardBitmaps[0], sizeof(BITMAP), &bm);
	*width = cardWidth = bm.bmWidth;
	*height = cardHeight = bm.bmHeight;
	return TRUE;
}


/***********************************************************************
 * Draw a card. Unlike cdtDrawCard, this version allows you to stretch
 * card bitmaps to the size you specify (dx, dy). See cdtDraw for info
 * on card, type and color parameters.
 */
BOOL WINAPI cdtDrawExt(HDC hdc, int x, int y, int dx, int dy, int card, int type, DWORD color)
{
	HDC hMemoryDC;
	HBITMAP hCardBitmap;
	HGDIOBJ result;

	TRACE("(%p, %d, %d, %d, %d, %d, %d, %ld)\n", hdc, x, y, dx, dy, card, type, color);

	if((card < 0) || (card > CARD_MAX))
	{
		FIXME("Unexpected card: %d\n", card);
		return 0;
	}

	if((type < 0) || (type > 2))
		FIXME("Unexpected type: %d\n", type);

	hCardBitmap = cardBitmaps[card];
	if(hCardBitmap == 0)
		return FALSE;

	hMemoryDC = CreateCompatibleDC(hdc);
	if(hMemoryDC == 0)
		return FALSE;

	result = SelectObject(hMemoryDC, hCardBitmap);
	if((result == 0) || (result == HGDI_ERROR))
	{
		DeleteDC(hMemoryDC);
		return FALSE;
	}

	SetBkColor(hdc, color);

	if((cardWidth == dx) && (cardHeight == dy))
		BitBlt(hdc, x, y, cardWidth, cardHeight, hMemoryDC, 0, 0, SRCCOPY);
	else
		StretchBlt(hdc, x, y, dx, dy, hMemoryDC, 0, 0, cardWidth, cardHeight, SRCCOPY);

	DeleteDC(hMemoryDC);

	return TRUE;
}


/***********************************************************************
 * Draws a card at position x, y in its default size (as returned by
 * cdtInit.
 *
 * Type parameter controls whether the front (0), back (1), or inverted
 * front (2) of the card is to be drawn.
 *
 * The card parameter defines the card graphic to be drawn. If we are
 * drawing fronts of cards, card should have a value from 0 through 51
 * to represent the card face. If we are drawing card backs, 53 through
 * 68 represent different card backs.
 *
 * When drawing card faces, two lowest bits represent the card suit
 * (clubs, diamonds, hearts, spades), and the bits above that define the
 * card value (ace, 2, ..., king). That is,
 *   card = face * 4 + suit.
 *
 * Color parameter defines the background color, used when drawing some
 * card backs.
 */
BOOL WINAPI cdtDraw(HDC hdc, int x, int y, int card, int type, DWORD color)
{
	TRACE("(%p, %d, %d, %d, %d, %ld)\n", hdc, x, y, card, type, color);

	return cdtDrawExt(hdc, x, y, cardWidth, cardHeight, card, type, color);
}


/***********************************************************************
 * Animates the card backs, e.g. blinking lights on the robot, the sun
 * donning sunglasses, bats flying across the caste, etc.. Works only
 * for cards of normal size (as drawn with cdtDraw). To draw frames of
 * the card back animation, start with frame = 0, and increment the
 * frame by one, until cdtAnimate returns FALSE (to indicate that we
 * have gone through all frames of animation).
 */
BOOL WINAPI cdtAnimate(HDC hdc, int cardback, int x, int y, int frame)
{
	TRACE("(%p, %d, %d, %d, %d)\n", hdc, cardback, x, y, frame);
	FIXME("Implement me.");

	return FALSE;
}


/***********************************************************************
 * Frees resources reserved by cdtInitialize.
 */
void WINAPI cdtTerm()
{
	int i;

	TRACE("()\n");

	for(i = 0; i <= CARD_MAX; i++)
	{
		if(cardBitmaps[i] != 0)
			DeleteObject(cardBitmaps[i]);
		cardBitmaps[i] = 0;
	}
}


/***********************************************************************
 * DllMain.
 */
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        hInst = inst;
        DisableThreadLibraryCalls( inst );
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
