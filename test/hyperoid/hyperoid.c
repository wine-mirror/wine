//
// HYPEROID - a neato game
//
// Version: 1.1  Copyright (C) 1990,91 Hutchins Software
//      This software is licenced under the GNU General Public Licence
//      Please read the associated legal documentation
// Author: Edward Hutchins
// Internet: eah1@cec1.wustl.edu
// USNail: c/o Edward Hutchins, 63 Ridgemoor Dr., Clayton, MO, 63105
// Revisions:
// 10/31/91 made game better/harder - Ed.
//
// Music: R.E.M./The Cure/Ministry/Front 242/The Smiths/New Order/Hendrix...
// Beers: Bass Ale, Augsberger Dark
//

#include "hyperoid.h"

//
// imports
//

IMPORT POINT        LetterPart[] FROM( roidsupp.c );
IMPORT NPSTR        szNumberDesc[] FROM( roidsupp.c );
IMPORT NPSTR        szLetterDesc[] FROM( roidsupp.c );

//
// globals
//

GLOBAL CHAR         szAppName[32];
GLOBAL HANDLE       hAppInst;
GLOBAL HWND         hAppWnd;
GLOBAL HPALETTE     hAppPalette;
GLOBAL INT          nDrawDelay;
GLOBAL INT          nLevel;
GLOBAL INT          nSafe;
GLOBAL INT          nShield;
GLOBAL INT          nBomb;
GLOBAL INT          nBadGuys;
GLOBAL LONG         lScore;
GLOBAL LONG         lLastLife;
GLOBAL LONG         lHighScore;
GLOBAL BOOL         bRestart;
GLOBAL BOOL         bPaused;
GLOBAL BOOL         bBW;
GLOBAL INT          vkShld;
GLOBAL INT          vkClkw;
GLOBAL INT          vkCtrClkw;
GLOBAL INT          vkThrst;
GLOBAL INT          vkRvThrst;
GLOBAL INT          vkFire;
GLOBAL INT          vkBomb;
GLOBAL NPOBJ        npPlayer;
GLOBAL LIST         FreeList;
GLOBAL LIST         RoidList;
GLOBAL LIST         ShotList;
GLOBAL LIST         FlameList;
GLOBAL LIST         SpinnerList;
GLOBAL LIST         HunterList;
GLOBAL LIST         HunterShotList;
GLOBAL LIST         SwarmerList;
GLOBAL LIST         LetterList;
GLOBAL LIST         BonusList;
GLOBAL INT          nCos[DEGREE_SIZE];
GLOBAL INT          nSin[DEGREE_SIZE];
GLOBAL HPEN         hPen[PALETTE_SIZE];
GLOBAL OBJ          Obj[MAX_OBJS];
GLOBAL HBITMAP      hBitmap[IDB_MAX];

//
// locals
//

LOCAL DWORD         dwSeed;
LOCAL INT           nScoreLen;
LOCAL CHAR          szScore[40];
LOCAL RECT          rectScoreClip;
LOCAL RECT          rectShotClip;
LOCAL POINT         Player[] =
{ {0, 0}, {160, 150}, {0, 250}, {96, 150}, {0, 0} };
LOCAL POINT         Spinner[] =
{ {160, 150}, {224, 100}, {96, 100}, {32, 150}, {160, 150} };
LOCAL POINT         Swarmer[] =
{ {0, 100}, {64, 100}, {128, 100}, {192, 100}, {0, 100} };
LOCAL POINT         Hunter[] =
{
	{160, 150}, {0, 250}, {192, 30}, {64, 30},
	{0, 250}, {96, 150}, {128, 150}, {160, 150}
};
LOCAL POINT         Bonus[] =
{ {0, 150}, {102, 150}, {205, 150}, {51, 150}, {154, 150}, {0, 150} };

//
// KillBadGuy - kill off a badguy (made into a macro)
//

#define KillBadGuy() \
((--nBadGuys <= 0)?(SetRestart( RESTART_NEXTLEVEL ),TRUE):FALSE)

//
// arand - pseudorandom number from 0 to x-1 (thanks antman!)
//

INT NEAR PASCAL arand( INT x )
{
	dwSeed = dwSeed * 0x343fd + 0x269ec3;
	return( (INT)(((dwSeed >> 16) & 0x7fff) * x >> 15) );
}

//
// AddHead - add an object to the head of a list
//

VOID NEAR PASCAL AddHead( NPLIST npList, NPNODE npNode )
{
	if (npList->npHead)
	{
		npNode->npNext = npList->npHead;
		npNode->npPrev = NULL;
		npList->npHead = (npList->npHead->npPrev = npNode);
	}
	else // add to an empty list
	{
		npList->npHead = npList->npTail = npNode;
		npNode->npNext = npNode->npPrev = NULL;
	}
}

//
// RemHead - remove the first element in a list
//

NPNODE NEAR PASCAL RemHead( NPLIST npList )
{
	if (npList->npHead)
	{
		NPNODE npNode = npList->npHead;
		if (npList->npTail != npNode)
		{
			npList->npHead = npNode->npNext;
			npNode->npNext->npPrev = NULL;
		}
		else npList->npHead = npList->npTail = NULL;
		return( npNode );
	}
	else return( NULL );
}

//
// Remove - remove an arbitrary element from a list
//

VOID NEAR PASCAL Remove( NPLIST npList, NPNODE npNode )
{
	if (npNode->npPrev) npNode->npPrev->npNext = npNode->npNext;
	else npList->npHead = npNode->npNext;
	if (npNode->npNext) npNode->npNext->npPrev = npNode->npPrev;
	else npList->npTail = npNode->npPrev;
}

//
// DrawObject - draw a single object
//

VOID NEAR PASCAL DrawObject( HDC hDC, NPOBJ npObj )
{
	INT             nCnt;
	INT             nDir = (npObj->nDir += npObj->nSpin);
	INT             x = (npObj->Pos.x += npObj->Vel.x);
	INT             y = (npObj->Pos.y += npObj->Vel.y);
	POINT           Pts[MAX_PTS];

	if (x < -CLIP_COORD) npObj->Pos.x = x = CLIP_COORD;
	else if (x > CLIP_COORD) npObj->Pos.x = x = -CLIP_COORD;
	if (y < -CLIP_COORD) npObj->Pos.y = y = CLIP_COORD;
	else if (y > CLIP_COORD) npObj->Pos.y = y = -CLIP_COORD;

	for (nCnt = npObj->byPts - 1; nCnt >= 0; --nCnt)
	{
		WORD wDeg = DEG( npObj->Pts[nCnt].x + nDir );
		INT nLen = npObj->Pts[nCnt].y;
		Pts[nCnt].x = x + MULDEG( nLen, nCos[wDeg] );
		Pts[nCnt].y = y + MULDEG( nLen, nSin[wDeg] );
	}

	if (npObj->byPts > 1)
	{
		SelectObject( hDC, hPen[BLACK] );
		Polyline( hDC, npObj->Old, npObj->byPts );
		if (npObj->nCount > 0)
		{
			SelectObject( hDC, hPen[npObj->byColor] );
			Polyline( hDC, Pts, npObj->byPts );
			for (nCnt = npObj->byPts - 1; nCnt >= 0; --nCnt)
				npObj->Old[nCnt] = Pts[nCnt];
		}
	}
	else // just a point
	{
		SetPixel( hDC, npObj->Old[0].x, npObj->Old[0].y, PALETTEINDEX( BLACK ) );
		if (npObj->nCount > 0)
		{
			SetPixel( hDC, Pts[0].x, Pts[0].y, PALETTEINDEX( npObj->byColor ) );
			npObj->Old[0] = Pts[0];
		}
	}
}

//
// SetRestart - set the restart timer
//

VOID NEAR PASCAL SetRestart( RESTART_MODE Restart )
{
	POINT           Pt;
	CHAR            szBuff[32];

	if (bRestart) return;
	SetTimer( hAppWnd, RESTART_TIMER, RESTART_DELAY, NULL );
	bRestart = TRUE;

	Pt.x = Pt.y = 0;
	switch (Restart)
	{
	case RESTART_GAME:
		SpinLetters( "GAME OVER", Pt, Pt, RED, 400 );
		break;
	case RESTART_LEVEL:
		PrintLetters( "GET READY", Pt, Pt, BLUE, 300 );
		break;
	case RESTART_NEXTLEVEL:
		wsprintf( szBuff, "LEVEL %u", nLevel + 1 );
		PrintLetters( szBuff, Pt, Pt, BLUE, 300 );
		break;
	}
}

//
// PrintPlayerMessage - show the player a status message
//

VOID NEAR PASCAL PrintPlayerMessage( NPSTR npszText )
{
	POINT Pos, Vel;

	Pos = npPlayer->Pos;
	Pos.y -= 400;
	Vel.x = 0;
	Vel.y = -50;
	PrintLetters( npszText, Pos, Vel, GREEN, 150 );
}

//
// AddExtraLife - give the player another life
//

VOID NEAR PASCAL AddExtraLife( VOID )
{
	PrintPlayerMessage( "EXTRA LIFE" );
	++npPlayer->nCount;
	npPlayer->byColor = (BYTE)(BLACK + npPlayer->nCount);
	if (npPlayer->byColor > WHITE) npPlayer->byColor = WHITE;
}

//
// Hit - something hit an object, do fireworks
//

VOID NEAR PASCAL Hit( HDC hDC, NPOBJ npObj )
{
	INT             nCnt;

	for (nCnt = 0; nCnt < 6; ++nCnt)
	{
		NPOBJ npFlame = RemHeadObj( &FreeList );
		if (!npFlame) return;
		npFlame->Pos.x = npObj->Pos.x;
		npFlame->Pos.y = npObj->Pos.y;
		npFlame->Vel.x = npObj->Vel.x;
		npFlame->Vel.y = npObj->Vel.y;
		npFlame->nDir = npObj->nDir + (nCnt * DEGREE_SIZE) / 6;
		npFlame->nSpin = 0;
		npFlame->nCount = 10 + arand( 8 );
		npFlame->byColor = YELLOW;
		npFlame->byPts = 1;
		npFlame->Pts[0].x = npFlame->Pts[0].y = 0;
		ACCEL( npFlame, npFlame->nDir, 50 - npFlame->nCount );
		AddHeadObj( &FlameList, npFlame );
	}
}

//
// Explode - explode an object
//

VOID NEAR PASCAL Explode( HDC hDC, NPOBJ npObj )
{
	INT             nCnt, nSize = npObj->byPts;

	DrawObject( hDC, npObj );
	for (nCnt = 0; nCnt < nSize; ++nCnt)
	{
		NPOBJ npFlame;
		if (arand( 2 )) continue;
		if (!(npFlame = RemHeadObj( &FreeList ))) return;
		npFlame->Pos.x = npObj->Pos.x;
		npFlame->Pos.y = npObj->Pos.y;
		npFlame->Vel.x = npObj->Vel.x;
		npFlame->Vel.y = npObj->Vel.y;
		npFlame->nDir = npObj->nDir + nCnt * DEGREE_SIZE / nSize + arand( 32 );
		npFlame->nSpin = arand( 31 ) - 15;
		npFlame->nCount = 25 + arand( 16 );
		npFlame->byColor = npObj->byColor;
		npFlame->byPts = 2;
		npFlame->Pts[0] = npObj->Pts[nCnt];
		if (nCnt == nSize - 1) npFlame->Pts[1] = npObj->Pts[0];
		else npFlame->Pts[1] = npObj->Pts[nCnt + 1];
		ACCEL( npFlame, npFlame->nDir, 60 - npFlame->nCount );
		AddHeadObj( &FlameList, npFlame );
	}
	Hit( hDC, npObj );
}

//
// HitPlayer - blow up the player
//

BOOL NEAR PASCAL HitPlayer( HDC hDC, NPOBJ npObj )
{
	POINT           Vel;
	INT             nMass, nSpin;

	if (nSafe || (npPlayer->nCount <= 0)) return( FALSE );

	// rumble and shake both objects
	nMass = npPlayer->nMass + npObj->nMass;

	nSpin = npPlayer->nSpin + npObj->nSpin;
	npObj->nSpin -= MulDiv( nSpin, npPlayer->nMass, nMass );
	npPlayer->nSpin -= MulDiv( nSpin, npObj->nMass, nMass );

	Vel.x = npPlayer->Vel.x - npObj->Vel.x;
	Vel.y = npPlayer->Vel.y - npObj->Vel.y;
	npObj->Vel.x += MulDiv( Vel.x, npPlayer->nMass, nMass );
	npObj->Vel.y += MulDiv( Vel.y, npPlayer->nMass, nMass );
	npPlayer->Vel.x -= MulDiv( Vel.x, npObj->nMass, nMass );
	npPlayer->Vel.y -= MulDiv( Vel.y, npObj->nMass, nMass );

	if (--npPlayer->nCount)
	{
		npPlayer->byColor = (BYTE)(BLACK + npPlayer->nCount);
		if (npPlayer->byColor > WHITE) npPlayer->byColor = WHITE;
		Hit( hDC, npPlayer );
		return( TRUE );
	}

	// final death
	npPlayer->byColor = WHITE;
	Explode( hDC, npPlayer );
	SetRestart( RESTART_GAME );
	return( FALSE );
}

//
// CreateLetter - make a new letter object
//

NPOBJ FAR PASCAL CreateLetter( CHAR cLetter, INT nSize )
{
	NPOBJ           npLtr;
	INT             nCnt;
	NPSTR           npDesc;

	if (cLetter >= '0' && cLetter <= '9') npDesc = szNumberDesc[cLetter - '0'];
	else if (cLetter >= 'A' && cLetter <= 'Z') npDesc = szLetterDesc[cLetter - 'A'];
	else if (cLetter >= 'a' && cLetter <= 'z') npDesc = szLetterDesc[cLetter - 'a'];
	else if (cLetter == '.') npDesc = "l";
	else return( NULL );

	if (npLtr = RemHeadObj( &FreeList ))
	{
		npLtr->nMass = 1;
		npLtr->nDir = 0;
		npLtr->nSpin = 0;
		npLtr->nCount = 40;
		npLtr->byColor = WHITE;
		npLtr->byPts = (BYTE)(nCnt = strlen( npDesc ));
		while (nCnt--)
		{
			npLtr->Pts[nCnt] = LetterPart[npDesc[nCnt] - 'a'];
			npLtr->Pts[nCnt].y = MulDiv( npLtr->Pts[nCnt].y, nSize, LETTER_MAX );
		}
		AddHeadObj( &LetterList, npLtr );
	}
	return( npLtr );
}

//
// DrawLetters - draw letters and such
//

VOID NEAR PASCAL DrawLetters( HDC hDC )
{
	NPOBJ           npLtr, npNext;

	for (npLtr = HeadObj( &LetterList ); npLtr; npLtr = npNext)
	{
		npNext = NextObj( npLtr );
		switch (--npLtr->nCount)
		{
		case 3:
			--npLtr->byColor;
			break;
		case 0:
			RemoveObj( &LetterList, npLtr );
			AddHeadObj( &FreeList, npLtr );
			break;
		}
		DrawObject( hDC, npLtr );
	}
}

//
// CreateBonus - make a new bonus object
//

VOID NEAR PASCAL CreateBonus( VOID )
{
	NPOBJ           npBonus;
	INT             nCnt;

	if (npBonus = RemHeadObj( &FreeList ))
	{
		npBonus->Pos.x = arand( CLIP_COORD * 2 ) - CLIP_COORD;
		npBonus->Pos.y = -CLIP_COORD;
		npBonus->Vel.x = npBonus->Vel.y = 0;
		npBonus->nDir = arand( DEGREE_SIZE );
		npBonus->nSpin = (arand( 2 ) ? 12 : -12);
		npBonus->nCount = arand( 4 ) + 1;
		npBonus->nDelay = 64 + arand( 128 );
		npBonus->nMass = 1;
		npBonus->byColor = (BYTE)(WHITE + (npBonus->nCount * 2));
		npBonus->byPts = DIM(Bonus);
		for (nCnt = 0; nCnt < DIM(Bonus); ++nCnt)
			npBonus->Pts[nCnt] = Bonus[nCnt];
		ACCEL( npBonus, npBonus->nDir, 30 + nLevel * 2 );
		AddHeadObj( &BonusList, npBonus );
	}
}

//
// DrawBonuses - process and draw the bonus list
//

VOID NEAR PASCAL DrawBonuses( HDC hDC )
{
	NPOBJ           npBonus, npNext;
	LOCAL INT       nNextBonus = 1000;

	if (nBadGuys && (--nNextBonus < 0))
	{
		CreateBonus();
		nNextBonus = 1000;
	}

	for (npBonus = HeadObj( &BonusList ); npBonus; npBonus = npNext)
	{
		NPOBJ           npShot;
		INT             nDelta;
		RECT            rect;

		npNext = NextObj( npBonus );

		MKRECT( &rect, npBonus->Pos, 150 );

		if (PTINRECT( &rect, npPlayer->Pos ))
		{
			if (npPlayer->nCount > 0) switch (npBonus->nCount)
			{
			case 1:
				{
					CHAR szBuff[32];
					LONG lBonus = 1000L * nLevel;
					if (lBonus == 0) lBonus = 500;
					lScore += lBonus;
					wsprintf( szBuff, "%ld", lBonus );
					PrintPlayerMessage( szBuff );
				}
				break;
			case 2:
				nSafe = 15;
				++nShield;
				npPlayer->byColor = GREEN;
				PrintPlayerMessage( "EXTRA SHIELD" );
				break;
			case 3:
				++nBomb;
				PrintPlayerMessage( "EXTRA BOMB" );
				break;
			case 4:
				AddExtraLife();
				break;
			}
			npBonus->nCount = 0;
			Explode( hDC, npBonus );
			RemoveObj( &BonusList, npBonus );
			AddHeadObj( &FreeList, npBonus );
		}
		else if (INTRECT(&rect, &rectShotClip))
		{
			for (npShot = HeadObj( &ShotList ); npShot; npShot = NextObj( npShot ))
			{
				if (!PTINRECT( &rect, npShot->Pos )) continue;
				npShot->nCount = 1;
				npBonus->nCount = 0;
				Explode( hDC, npBonus );
				RemoveObj( &BonusList, npBonus );
				AddHeadObj( &FreeList, npBonus );
			}
		}
		if (npBonus->nCount && --npBonus->nDelay <= 0)
		{
			--npBonus->nCount;
			npBonus->nDelay = 64 + arand( 128 );
			npBonus->byColor = (BYTE)(WHITE + (npBonus->nCount * 2));
			if (npBonus->nCount == 0)
			{
				Explode( hDC, npBonus );
				RemoveObj( &BonusList, npBonus );
				AddHeadObj( &FreeList, npBonus );
			}
		}
		nDelta = npPlayer->Pos.x - npBonus->Pos.x;
		while (nDelta < -16 || nDelta > 16) nDelta /= 2;
		npBonus->Vel.x += nDelta - npBonus->Vel.x / 16;
		nDelta = npPlayer->Pos.y - npBonus->Pos.y;
		while (nDelta < -16 || nDelta > 16) nDelta /= 2;
		npBonus->Vel.y += nDelta - npBonus->Vel.y / 16;
		DrawObject( hDC, npBonus );
	}
}

//
// DrawHunterShots - process and draw the hunter shot list
//

VOID NEAR PASCAL DrawHunterShots( HDC hDC )
{
	NPOBJ           npShot, npNext;

	for (npShot = HeadObj( &HunterShotList ); npShot; npShot = npNext)
	{
		RECT            rect;

		npNext = NextObj( npShot );

		MKRECT( &rect, npShot->Pos, 200 );

		if (PTINRECT( &rect, npPlayer->Pos ))
		{
			HitPlayer( hDC, npShot );
			npShot->nCount = 1;
		}
		switch (--npShot->nCount)
		{
		case 7:
			npShot->byColor = DKGREEN;
			break;
		case 0:
			RemoveObj( &HunterShotList, npShot );
			AddHeadObj( &FreeList, npShot );
			break;
		}
		DrawObject( hDC, npShot );
	}
}

//
// FireHunterShot - fire a hunter bullet
//

VOID NEAR PASCAL FireHunterShot( NPOBJ npHunt )
{
	NPOBJ           npShot;

	if (npShot = RemHeadObj( &FreeList ))
	{
		npShot->Pos.x = npHunt->Pos.x;
		npShot->Pos.y = npHunt->Pos.y;
		npShot->Vel.x = npHunt->Vel.x;
		npShot->Vel.y = npHunt->Vel.y;
		npShot->nMass = 8;
		npShot->nDir = npHunt->nDir + arand( 5 ) - 2;
		npShot->nSpin = (arand( 2 ) ? 10 : -10);
		npShot->nCount = 16 + arand( 8 );
		npShot->byColor = GREEN;
		npShot->byPts = 2;
		npShot->Pts[0].x = 128;
		npShot->Pts[0].y = 50;
		npShot->Pts[1].x = 0;
		npShot->Pts[1].y = 50;
		ACCEL( npShot, npShot->nDir, 200 + npShot->nCount );
		AddHeadObj( &HunterShotList, npShot );
	}
}

//
// CreateHunter - make a new hunter
//

VOID NEAR PASCAL CreateHunter( VOID )
{
	NPOBJ           npHunt;
	INT             nCnt;

	if (npHunt = RemHeadObj( &FreeList ))
	{
		npHunt->Pos.x = arand( CLIP_COORD * 2 ) - CLIP_COORD;
		npHunt->Pos.y = -CLIP_COORD;
		npHunt->Vel.x = npHunt->Vel.y = 0;
		npHunt->nMass = 256;
		npHunt->nDir = arand( DEGREE_SIZE );
		npHunt->nSpin = 0;
		npHunt->nCount = 1 + arand( nLevel );
		npHunt->nDelay = 2 + arand( 10 );
		npHunt->byColor = CYAN;
		npHunt->byPts = DIM(Hunter);
		for (nCnt = 0; nCnt < DIM(Hunter); ++nCnt)
			npHunt->Pts[nCnt] = Hunter[nCnt];
		ACCEL( npHunt, npHunt->nDir, 30 + nLevel * 2 );
		AddHeadObj( &HunterList, npHunt );
		++nBadGuys;
	}
}

//
// DrawHunters - process and draw the hunter list
//

VOID NEAR PASCAL DrawHunters( HDC hDC )
{
	NPOBJ           npHunt, npNext;
	LOCAL INT       nNextHunter = 200;

	if (nBadGuys && (--nNextHunter < 0))
	{
		CreateHunter();
		nNextHunter = 1000 + arand( 1000 ) - nLevel * 8;
	}

	for (npHunt = HeadObj( &HunterList ); npHunt; npHunt = npNext)
	{
		NPOBJ           npShot;
		RECT            rect;

		npNext = NextObj( npHunt );

		MKRECT( &rect, npHunt->Pos, 200 );

		if (PTINRECT( &rect, npPlayer->Pos ))
		{
			HitPlayer( hDC, npHunt );
			--npHunt->nCount;
			if (npHunt->nCount < 1)
			{
				KillBadGuy();
				npHunt->byColor = CYAN;
				Explode( hDC, npHunt );
				RemoveObj( &HunterList, npHunt );
				AddHeadObj( &FreeList, npHunt );
			}
			else if (npHunt->nCount == 1) npHunt->byColor = DKCYAN;
		}
		else if (INTRECT(&rect, &rectShotClip))
		{
			for (npShot = HeadObj( &ShotList ); npShot; npShot = NextObj( npShot ))
			{
				if (!PTINRECT( &rect, npShot->Pos )) continue;
				npShot->nCount = 1;
				lScore += npHunt->nCount * 1000;
				if (--npHunt->nCount < 1)
				{
					KillBadGuy();
					npHunt->byColor = CYAN;
					Explode( hDC, npHunt );
					RemoveObj( &HunterList, npHunt );
					AddHeadObj( &FreeList, npHunt );
				}
				else
				{
					if (npHunt->nCount == 1) npHunt->byColor = DKCYAN;
					Hit( hDC, npHunt );
				}
				break;
			}
		}
		ACCEL( npHunt, npHunt->nDir, 8 );
		npHunt->Vel.x -= npHunt->Vel.x / 16;
		npHunt->Vel.y -= npHunt->Vel.y / 16;
		if (--npHunt->nDelay <= 0)
		{
			npHunt->nDelay = arand( 10 );
			npHunt->nSpin = arand( 11 ) - 5;
			FireHunterShot( npHunt );
		}
		DrawObject( hDC, npHunt );
	}
}

//
// CreateSwarmer - make a new swarmer
//

VOID NEAR PASCAL CreateSwarmer( POINT Pos, INT nDir, INT nCount )
{
	NPOBJ           npSwarm;
	INT             nCnt;

	if (npSwarm = RemHeadObj( &FreeList ))
	{
		npSwarm->Pos = Pos;
		npSwarm->Vel.x = npSwarm->Vel.y = 0;
		npSwarm->nDir = nDir;
		npSwarm->nSpin = arand( 31 ) - 15;
		npSwarm->nCount = nCount;
		npSwarm->nDelay = 64 + arand( 64 );
		npSwarm->nMass = 32;
		npSwarm->byColor = DKGREEN;
		npSwarm->byPts = DIM(Swarmer);
		for (nCnt = 0; nCnt < DIM(Swarmer); ++nCnt)
		{
			npSwarm->Pts[nCnt] = Swarmer[nCnt];
			npSwarm->Pts[nCnt].y += nCount * 10;
		}
		ACCEL( npSwarm, npSwarm->nDir, 30 + nLevel * 2 );
		AddHeadObj( &SwarmerList, npSwarm );
		++nBadGuys;
	}
}

//
// DrawSwarmers - process and draw the swarmer list
//

VOID NEAR PASCAL DrawSwarmers( HDC hDC )
{
	NPOBJ           npSwarm, npNext;
	LOCAL INT       nNextSwarmer = 1000;

	if (nBadGuys && (--nNextSwarmer < 0))
	{
		POINT Pos;
		Pos.x = arand( CLIP_COORD * 2 ) - CLIP_COORD;
		Pos.y = -CLIP_COORD;
		CreateSwarmer( Pos, arand( DEGREE_SIZE ), 8 + nLevel * 2 );
		nNextSwarmer = 1000 + arand( 500 ) - nLevel * 4;
	}

	for (npSwarm = HeadObj( &SwarmerList ); npSwarm; npSwarm = npNext)
	{
		NPOBJ           npShot;
		RECT            rect;

		npNext = NextObj( npSwarm );

		MKRECT( &rect, npSwarm->Pos, 150 + npSwarm->nCount * 10 );

		if (PTINRECT( &rect, npPlayer->Pos ))
		{
			HitPlayer( hDC, npSwarm );
			npSwarm->nCount = 0;
		}
		else if (INTRECT(&rect, &rectShotClip))
		{
			for (npShot = HeadObj( &ShotList ); npShot; npShot = NextObj( npShot ))
			{
				if (!PTINRECT( &rect, npShot->Pos )) continue;
				npShot->nCount = 1;
				lScore += npSwarm->nCount * 25;
				npSwarm->nCount = 0;
				break;
			}
		}
		if (npSwarm->nCount <= 0)
		{
			npSwarm->byColor = GREEN;
			KillBadGuy();
			Explode( hDC, npSwarm );
			RemoveObj( &SwarmerList, npSwarm );
			AddHeadObj( &FreeList, npSwarm );
		}
		else
		{
			if ((npSwarm->nCount > 1) && (--npSwarm->nDelay <= 0))
			{
				INT nDir = arand( DEGREE_SIZE );
				INT nCount = npSwarm->nCount / 2;
				CreateSwarmer( npSwarm->Pos, nDir, nCount );
				nCount = npSwarm->nCount - nCount;
				CreateSwarmer( npSwarm->Pos, nDir + 128, nCount );
				npSwarm->nCount = 0;
			}
			DrawObject( hDC, npSwarm );
		}
	}
}

//
// CreateSpinner - make a new spinner
//

VOID NEAR PASCAL CreateSpinner( VOID )
{
	NPOBJ           npSpin;
	INT             nCnt;

	if (npSpin = RemHeadObj( &FreeList ))
	{
		npSpin->Pos.x = arand( CLIP_COORD * 2 ) - CLIP_COORD;
		npSpin->Pos.y = -CLIP_COORD;
		npSpin->Vel.x = npSpin->Vel.y = 0;
		npSpin->nDir = arand( DEGREE_SIZE );
		npSpin->nSpin = -12;
		npSpin->nCount = 1 + arand( nLevel );
		npSpin->nMass = 64 + npSpin->nCount * 32;
		npSpin->byColor = (BYTE)(MAGENTA - npSpin->nCount);
		npSpin->byPts = DIM(Spinner);
		for (nCnt = 0; nCnt < DIM(Spinner); ++nCnt)
			npSpin->Pts[nCnt] = Spinner[nCnt];
		ACCEL( npSpin, npSpin->nDir, 30 + nLevel * 2 );
		AddHeadObj( &SpinnerList, npSpin );
		++nBadGuys;
	}
}

//
// DrawSpinners - process and draw the spinner list
//

VOID NEAR PASCAL DrawSpinners( HDC hDC )
{
	NPOBJ           npSpin, npNext;
	LOCAL INT       nNextSpinner = 1000;

	if (nBadGuys && (--nNextSpinner < 0))
	{
		CreateSpinner();
		nNextSpinner = 100 + arand( 900 ) - nLevel * 2;
	}

	for (npSpin = HeadObj( &SpinnerList ); npSpin; npSpin = npNext)
	{
		NPOBJ           npShot;
		INT             nDelta;
		RECT            rect;

		npNext = NextObj( npSpin );

		MKRECT( &rect, npSpin->Pos, 150 );

		if (PTINRECT( &rect, npPlayer->Pos ))
		{
			HitPlayer( hDC, npSpin );
			--npSpin->nCount;
			npSpin->byColor = (BYTE)(MAGENTA - npSpin->nCount);
			if (npSpin->nCount < 1)
			{
				KillBadGuy();
				Explode( hDC, npSpin );
				RemoveObj( &SpinnerList, npSpin );
				AddHeadObj( &FreeList, npSpin );
			}
		}
		else if (INTRECT(&rect, &rectShotClip))
		{
			for (npShot = HeadObj( &ShotList ); npShot; npShot = NextObj( npShot ))
			{
				if (!PTINRECT( &rect, npShot->Pos )) continue;
				npShot->nCount = 1;
				lScore += npSpin->nCount * 500;
				npSpin->byColor = (BYTE)(MAGENTA - (--npSpin->nCount));
				if (npSpin->nCount < 1)
				{
					KillBadGuy();
					Explode( hDC, npSpin );
					RemoveObj( &SpinnerList, npSpin );
					AddHeadObj( &FreeList, npSpin );
				}
				else Hit( hDC, npSpin );
				break;
			}
		}
		nDelta = npPlayer->Pos.x - npSpin->Pos.x;
		while (nDelta < -16 || nDelta > 16) nDelta /= 2;
		npSpin->Vel.x += nDelta - npSpin->Vel.x / 16;
		nDelta = npPlayer->Pos.y - npSpin->Pos.y;
		while (nDelta < -16 || nDelta > 16) nDelta /= 2;
		npSpin->Vel.y += nDelta - npSpin->Vel.y / 16;
		DrawObject( hDC, npSpin );
	}
}

//
// CreateRoid - make a new asteroid
//

VOID NEAR PASCAL CreateRoid( POINT Pos, POINT Vel, INT nSides, BYTE byColor,
							 INT nDir, INT nSpeed, INT nSpin )
{
	NPOBJ           npRoid;
	INT             nCnt;

	if (npRoid = RemHeadObj( &FreeList ))
	{
		npRoid->Pos = Pos;
		npRoid->Vel = Vel;
		npRoid->nMass = nSides * 128;
		npRoid->nDir = nDir;
		npRoid->nSpin = nSpin + arand( 11 ) - 5;
		npRoid->nCount = nSides * 100;
		npRoid->byColor = byColor;
		npRoid->byPts = (BYTE)(nSides + 1);
		for (nCnt = 0; nCnt < nSides; ++nCnt)
		{
			npRoid->Pts[nCnt].x = nCnt * DEGREE_SIZE / nSides + arand( 30 );
			npRoid->Pts[nCnt].y = (nSides - 1) * 100 + 20 + arand( 80 );
		}
		npRoid->Pts[nSides] = npRoid->Pts[0];
		ACCEL( npRoid, nDir, nSpeed );
		AddHeadObj( &RoidList, npRoid );
		++nBadGuys;
	}
}

//
// BreakRoid - break up an asteroid
//

VOID NEAR PASCAL BreakRoid( HDC hDC, NPOBJ npRoid, NPOBJ npShot )
{
	INT             nCnt, nNew;

	lScore += npRoid->nCount;
	if (npShot) npShot->nCount = 1;
	switch (npRoid->byPts)
	{
	case 8:
		nNew = 2 + arand( 3 );
		break;
	case 7:
		nNew = 1 + arand( 3 );
		break;
	case 6:
		nNew = 1 + arand( 2 );
		break;
	case 5:
		nNew = arand( 2 );
		break;
	default:
		nNew = 0;
		break;
	}
	if (nNew == 1) // don't explode outward
	{
		POINT Pt = npRoid->Pos;
		Pt.x += arand( 301 ) - 150; Pt.y += arand( 301 ) - 150;
		CreateRoid( Pt, npRoid->Vel, npRoid->byPts - (nNew + 1),
					npRoid->byColor, npShot->nDir, 8, npRoid->nSpin );
	}
	else if (nNew > 0)
	{
		INT nSpeed = npRoid->nSpin * npRoid->nSpin * nNew + 16;
		for (nCnt = 0; nCnt < nNew; ++nCnt)
		{
			POINT Pt = npRoid->Pos;
			Pt.x += arand( 601 ) - 300; Pt.y += arand( 601 ) - 300;
			CreateRoid( Pt, npRoid->Vel, npRoid->byPts - (nNew + 1),
						npRoid->byColor,
						npRoid->nDir + nCnt * DEGREE_SIZE / nNew + arand( 32 ),
						nSpeed + arand( nLevel * 4 ),
						npRoid->nSpin / 2 );
		}
	}
	KillBadGuy();
	++npRoid->byColor;
	npRoid->nCount = 0;
	if (nNew)
	{
		Hit( hDC, npRoid );
		DrawObject( hDC, npRoid );
	}
	else Explode( hDC, npRoid );
	RemoveObj( &RoidList, npRoid );
	AddHeadObj( &FreeList, npRoid );
}

//
// DrawRoids - process and draw the asteroid list
//

VOID NEAR PASCAL DrawRoids( HDC hDC )
{
	NPOBJ           npRoid, npNext;

	for (npRoid = HeadObj( &RoidList ); npRoid; npRoid = npNext)
	{
		INT             nSize = npRoid->nCount;
		NPOBJ           npShot;
		RECT            rect;

		npNext = NextObj( npRoid );

		DrawObject( hDC, npRoid );

		MKRECT( &rect, npRoid->Pos, nSize );

		if (PTINRECT( &rect, npPlayer->Pos ) && HitPlayer( hDC, npRoid ))
		{
			npPlayer->nCount = -npPlayer->nCount;
			npPlayer->byColor = WHITE;
			Explode( hDC, npPlayer );
			BreakRoid( hDC, npRoid, NULL );
			if (nBadGuys) SetRestart( RESTART_LEVEL );
			else SetRestart( RESTART_NEXTLEVEL );
		}
		else if (INTRECT(&rect, &rectShotClip))
		{
			for (npShot = HeadObj( &ShotList ); npShot; npShot = NextObj( npShot ))
			{
				if (!PTINRECT( &rect, npShot->Pos )) continue;
				BreakRoid( hDC, npRoid, npShot );
				break;
			}
		}
	}
}

//
// DrawShots - process and draw the player shot list
//

VOID NEAR PASCAL DrawShots( HDC hDC )
{
	NPOBJ           npShot, npNext;

	if (npShot = HeadObj( &ShotList ))
	{
		rectShotClip.left = rectShotClip.right = npShot->Pos.x;
		rectShotClip.top = rectShotClip.bottom = npShot->Pos.y;
		while (npShot)
		{
			npNext = NextObj( npShot );
			switch (--npShot->nCount)
			{
			case 10:
				npShot->byColor = DKCYAN;
				break;
			case 5:
				npShot->byColor = DKBLUE;
				break;
			case 0:
				RemoveObj( &ShotList, npShot );
				AddHeadObj( &FreeList, npShot );
				break;
			}
			DrawObject( hDC, npShot );
			if (npShot->Pos.x < rectShotClip.left) rectShotClip.left = npShot->Pos.x;
			else if (npShot->Pos.x > rectShotClip.right) rectShotClip.right = npShot->Pos.x;
			if (npShot->Pos.y < rectShotClip.top) rectShotClip.top = npShot->Pos.y;
			else if (npShot->Pos.y > rectShotClip.bottom) rectShotClip.bottom = npShot->Pos.y;
			npShot = npNext;
		}
	}
	else rectShotClip.left = rectShotClip.right = rectShotClip.top = rectShotClip.bottom = 32767;
}

//
// DrawFlames - process and draw the flame list
//

VOID NEAR PASCAL DrawFlames( HDC hDC )
{
	NPOBJ           npFlame, npNext;

	for (npFlame = HeadObj( &FlameList ); npFlame; npFlame = npNext)
	{
		npNext = NextObj( npFlame );
		switch (--npFlame->nCount)
		{
		case 7:
			npFlame->byColor = RED;
			break;
		case 3:
			npFlame->byColor = DKRED;
			break;
		case 0:
			RemoveObj( &FlameList, npFlame );
			AddHeadObj( &FreeList, npFlame );
			break;
		}
		DrawObject( hDC, npFlame );
	}
}

//
// FireShot - fire a bullet
//

VOID NEAR PASCAL FireShot( VOID )
{
	NPOBJ           npShot;

	if (npShot = RemHeadObj( &FreeList ))
	{
		npShot->Pos.x = npPlayer->Pos.x;
		npShot->Pos.y = npPlayer->Pos.y;
		npShot->Vel.x = npPlayer->Vel.x;
		npShot->Vel.y = npPlayer->Vel.y;
		npShot->nMass = 8;
		npShot->nDir = npPlayer->nDir + arand( 5 ) - 2;
		npShot->nSpin = 0;
		npShot->nCount = 16 + arand( 8 );
		npShot->byColor = CYAN;
		npShot->byPts = 2;
		npShot->Pts[0].x = 128;
		npShot->Pts[0].y = 50;
		npShot->Pts[1].x = 0;
		npShot->Pts[1].y = 50;
		ACCEL( npShot, npShot->nDir, 200 + npShot->nCount );
		AddHeadObj( &ShotList, npShot );
	}
}

//
// AccelPlayer - move the player forward
//

VOID NEAR PASCAL AccelPlayer( INT nDir, INT nAccel )
{
	NPOBJ           npFlame;

	nDir += npPlayer->nDir;
	if (nAccel) ACCEL( npPlayer, nDir, nAccel );
	if (npFlame = RemHeadObj( &FreeList ))
	{
		npFlame->Pos.x = npPlayer->Pos.x;
		npFlame->Pos.y = npPlayer->Pos.y;
		npFlame->Vel.x = npPlayer->Vel.x;
		npFlame->Vel.y = npPlayer->Vel.y;
		npFlame->nDir = nDir + 100 + arand( 57 );
		npFlame->nSpin = 0;
		npFlame->nCount = nAccel + arand( 7 );
		npFlame->byColor = YELLOW;
		npFlame->byPts = 1;
		npFlame->Pts[0].x = npFlame->Pts[0].y = 0;
		ACCEL( npFlame, npFlame->nDir, 50 + arand( 10 ) );
		AddHeadObj( &FlameList, npFlame );
	}
}

//
// DrawPlayer - process and draw the player
//

VOID NEAR PASCAL DrawPlayer( HDC hDC )
{
	LOCAL INT       nBombing = 0;
	LOCAL INT       nShotDelay = 0;

	if (npPlayer->nCount <= 0) return;

	if (nSafe > 0)
	{
		if (--nSafe == 0)
		{
			npPlayer->byColor = (BYTE)(BLACK + npPlayer->nCount);
			if (npPlayer->byColor > WHITE) npPlayer->byColor = WHITE;
		}
	}
	else if (IsKeyDown( vkShld ) && nShield > 0)
	{
		nSafe = 15;
		if (--nShield > 0) npPlayer->byColor = GREEN;
		else npPlayer->byColor = DKGREEN;
	}

	if (nBombing > 0)
	{
		if (--nBombing == 0)
		{
			ExplodeBadguys( hDC, &SpinnerList );
			ExplodeBadguys( hDC, &SwarmerList );
			ExplodeBadguys( hDC, &HunterList );
		}
		else
		{
			HitList( hDC, &SpinnerList );
			HitList( hDC, &SwarmerList );
			HitList( hDC, &HunterList );
		}
	}
	else if (nBomb && IsKeyDown( vkBomb )) --nBomb, nBombing = 5;

	if (IsKeyDown( vkClkw )) npPlayer->nSpin += 8;
	if (IsKeyDown( vkCtrClkw )) npPlayer->nSpin -= 8;
	if (IsKeyDown( vkThrst )) AccelPlayer( 0, 12 );
	if (IsKeyDown( vkRvThrst )) AccelPlayer( 128, 12 );
	if (nShotDelay) --nShotDelay;
	else if (IsKeyDown( vkFire )) FireShot(), nShotDelay = 2;
	DrawObject( hDC, npPlayer );
	npPlayer->nSpin /= 2;
}

//
// GetHyperoidDC - get the correct DC for hyperoid rendering
//

HDC NEAR PASCAL GetHyperoidDC( HWND hWnd )
{
	HDC             hDC;
	INT             cx, cy;
	RECT            rect;

	GetClientRect( hWnd, &rect );
	cx = rect.right - rect.left;
	cy = rect.bottom - rect.top;

	hDC = GetDC( hWnd );

	// set up the mapping mode
	SetMapMode( hDC, MM_ISOTROPIC );
	SetWindowExt( hDC, MAX_COORD, MAX_COORD );
	SetViewportExt( hDC, cx / 2, -cy / 2 );
	SetViewportOrg( hDC, cx / 2, cy / 2 );

	// realize the palette
	SelectPalette( hDC, hAppPalette, 0 );
	RealizePalette( hDC );

	return( hDC );
}

//
// DrawObjects - transform and redraw everything in the system
//

VOID NEAR PASCAL DrawObjects( HWND hWnd )
{
	HDC             hDC = GetHyperoidDC( hWnd );

	// move and draw things (I don't think the order is important...)
	DrawPlayer( hDC );
	DrawFlames( hDC );
	DrawShots( hDC );
	DrawRoids( hDC );
	DrawSpinners( hDC );
	DrawSwarmers( hDC );
	DrawHunters( hDC );
	DrawHunterShots( hDC );
	DrawLetters( hDC );
	DrawBonuses( hDC );
	// (...but I'm not changing it!!! :-)

	ReleaseDC( hWnd, hDC );
}

//
// SetIndicator - set a quantity indicator
//

INT NEAR PASCAL SetIndicator( NPSTR npBuff, CHAR IDBitmap, INT nQuant )
{
	if (nQuant > 5)
	{
		*npBuff++ = IDBitmap; *npBuff++ = IDBitmap;
		*npBuff++ = IDBitmap; *npBuff++ = IDBitmap;
		*npBuff++ = IDB_plus;
	}
	else
	{
		INT nBlank = 5 - nQuant;
		while (nQuant--) *npBuff++ = IDBitmap;
		while (nBlank--) *npBuff++ = IDB_blank;
	}
	return( 5 );
}

//
// CheckScore - show the score and such stuff
//

VOID NEAR PASCAL CheckScore( HWND hWnd )
{
	CHAR            szBuff[sizeof(szScore)];
	NPSTR           npBuff = szBuff;
	INT             nLives, nLen, nCnt, x, y;
	HBITMAP         hbmOld;
	HDC             hDC, hDCMem;

	if (IsIconic( hWnd )) return;
	if (lScore - lLastLife > EXTRA_LIFE)
	{
		AddExtraLife();
		lLastLife = lScore;
	}
	nLives = ((npPlayer->nCount > 0) ? npPlayer->nCount : -npPlayer->nCount);

	*npBuff++ = IDB_level;
	wsprintf( npBuff, "%2.2u", nLevel );
	while (isdigit( *npBuff ))
		*npBuff = (CHAR)(*npBuff + IDB_num0 - '0'), ++npBuff;
	*npBuff++ = IDB_blank; *npBuff++ = IDB_score;
	wsprintf( npBuff, "%7.7lu", lScore );
	while (isdigit( *npBuff ))
		*npBuff = (CHAR)(*npBuff + IDB_num0 - '0'), ++npBuff;
	*npBuff++ = IDB_blank;
	npBuff += SetIndicator( npBuff, IDB_life, nLives );
	npBuff += SetIndicator( npBuff, IDB_shield, nShield );
	npBuff += SetIndicator( npBuff, IDB_bomb, nBomb );
	nLen = npBuff - szBuff;

	hDC = GetWindowDC( hWnd );
	IntersectClipRect( hDC, rectScoreClip.left, rectScoreClip.top,
							rectScoreClip.right, rectScoreClip.bottom );
	hDCMem = CreateCompatibleDC( hDC );
	hbmOld = SelectObject( hDCMem, hBitmap[0] );
	x = rectScoreClip.left;
	y = rectScoreClip.top;

	for (nCnt = 0; nCnt < nLen; ++nCnt)
	{
		if (szBuff[nCnt] != szScore[nCnt])
		{
			SelectObject( hDCMem, hBitmap[szBuff[nCnt] - IDB_blank] );
			BitBlt( hDC, x, y, CX_BITMAP, CY_BITMAP, hDCMem, 0, 0, SRCCOPY );
			szScore[nCnt] = szBuff[nCnt];
		}
		x += CX_BITMAP;
	}
	if (nCnt < nScoreLen)
	{
		SelectObject( hDCMem, hBitmap[0] );
		do {
			if (szScore[nCnt] != IDB_blank)
			{
				BitBlt( hDC, x, y, CX_BITMAP, CY_BITMAP, hDCMem, 0, 0, SRCCOPY );
				szScore[nCnt] = IDB_blank;
			}
			x += CX_BITMAP;
		} while (++nCnt < nScoreLen);
	}
	nScoreLen = nLen;

	SelectObject( hDCMem, hbmOld );
	DeleteDC( hDCMem );
	ReleaseDC( hWnd, hDC );
}

//
// HitList - Hit() a list of things
//

VOID NEAR PASCAL HitList( HDC hDC, NPLIST npList )
{
	NPOBJ           npObj;

	for (npObj = HeadObj( npList ); npObj; npObj = NextObj( npObj ))
		if (npObj->nCount) Hit( hDC, npObj );
}

//
// ExplodeBadguys - explode a list of badguys
//

VOID NEAR PASCAL ExplodeBadguys( HDC hDC, NPLIST npList )
{
	NPOBJ           npObj;

	while (npObj = HeadObj( npList ))
	{
		KillBadGuy();
		npObj->nCount = 0;
		Explode( hDC, npObj );
		RemoveObj( npList, npObj );
		AddHeadObj( &FreeList, npObj );
	}
}

//
// NewGame - start a new game
//

VOID NEAR PASCAL NewGame( HWND hWnd )
{
	HDC             hDC = GetHyperoidDC( hWnd );

	npPlayer->nCount = 0;
	npPlayer->byColor = WHITE;
	Explode( hDC, npPlayer );
	SetRestart( RESTART_GAME );
	ExplodeBadguys( hDC, &RoidList );
	ExplodeBadguys( hDC, &SpinnerList );
	ExplodeBadguys( hDC, &SwarmerList );
	ExplodeBadguys( hDC, &HunterList );

	ReleaseDC( hWnd, hDC );
}

//
// RestartHyperoid - set up a game!
//

VOID NEAR PASCAL RestartHyperoid( VOID )
{
	if (npPlayer->nCount == 0)
	{
		POINT Pos, Vel;
		Pos.x = 0;
		Pos.y = -CLIP_COORD / 2;
		Vel.x = 0;
		Vel.y = 150;
		PrintLetters( szAppName, Pos, Vel, YELLOW, 800 );
		npPlayer->nCount = 3;
		if (lHighScore < lScore) lHighScore = lScore;
		lLastLife = lScore = 0;
		nLevel = 0;
		nShield = nBomb = 3;
	}
	else if (npPlayer->nCount < 0)
	{
		// cheesy way of restarting after a major collision
		npPlayer->nCount = -npPlayer->nCount;
		nShield = nBomb = 3;
	}

	npPlayer->Pos.x = npPlayer->Pos.y = 0;
	npPlayer->Vel.x = npPlayer->Vel.y = 0;
	npPlayer->nDir = 64;
	npPlayer->nSpin = 0;
	npPlayer->byColor = GREEN;
	nSafe = 30;

	if (ShotList.npHead)
	{
		NPOBJ npShot;
		for (npShot = HeadObj( &ShotList ); npShot; npShot = NextObj( npShot ))
			npShot->nCount = 1;
	}

	// reseed the asteroid field
	if (nBadGuys == 0)
	{
		INT nCnt;
		++nLevel;
		for (nCnt = 5 + nLevel; nCnt; --nCnt)
		{
			POINT Pos, Vel;
			Pos.x = arand( MAX_COORD * 2 ) - MAX_COORD;
			Pos.y = arand( MAX_COORD * 2 ) - MAX_COORD;
			Vel.x = Vel.y = 0;
			CreateRoid( Pos, Vel, 6 + arand( 2 ),
						(BYTE)(arand( 2 ) ? DKYELLOW : DKGREY),
						arand( DEGREE_MAX ), 30 + arand( nLevel * 8 ), 0 );
		}
	}
}

//
// Panic - boss key (or just pause)
//

VOID NEAR PASCAL Panic( BOOL bPanic )
{
	if (bPanic && !bPaused)
	{
		bPaused = TRUE;
		KillTimer( hAppWnd, DRAW_TIMER );
		SetWindowText( hAppWnd, "Program Manager Help - PROGMAN.HLP" );
		ShowWindow( hAppWnd, SW_SHOWMINNOACTIVE );
		InvalidateRect( hAppWnd, NULL, TRUE );
	}
	else if (bPaused) // double-panic == normal
	{
		bPaused = FALSE;
		SetWindowText( hAppWnd, szAppName );
		if (bPanic) ShowWindow( hAppWnd, SW_RESTORE );
		SetTimer( hAppWnd, DRAW_TIMER, nDrawDelay, NULL );
	}
}

//
// PaintHyperoid - paint the hyperoid window
//

VOID NEAR PASCAL PaintHyperoid( HWND hWnd )
{
	PAINTSTRUCT     ps;

	BeginPaint( hWnd, &ps );
	if (bPaused) DrawIcon( ps.hdc, 2, 2, LoadIcon( hAppInst, INTRES(IDI_PANIC) ) );
	EndPaint( hWnd, &ps );
}

//
// EraseHyperoidBkgnd - fill in the background
//

BOOL NEAR PASCAL EraseHyperoidBkgnd( HWND hWnd, HDC hDC )
{
	HBRUSH          hbr;
	RECT            rect;

	GetClientRect( hWnd, &rect );

	if (bPaused)
	{
		SetBrushOrg( hDC, 0, 0 );
		hbr = CreateSolidBrush( GetSysColor( COLOR_BACKGROUND ) );
	}
	else
	{
		SelectPalette( hDC, hAppPalette, 0 );
		RealizePalette( hDC );
		hbr = CreateSolidBrush( PALETTEINDEX( BLACK ) );
	}

	FillRect( hDC, &rect, hbr );
	DeleteObject( hbr );
	return( TRUE );
}

//
// DrawShadowRect - draw a shaded rectangle around an object
//

VOID NEAR PASCAL DrawShadowRect( HDC hDC, NPRECT npRect, HPEN hHi, HPEN hLo )
{
	SelectObject( hDC, hHi );
	MoveTo( hDC, npRect->right, npRect->top );
	LineTo( hDC, npRect->left, npRect->top );
	LineTo( hDC, npRect->left, npRect->bottom );
	SelectObject( hDC, hLo );
	LineTo( hDC, npRect->right, npRect->bottom );
	LineTo( hDC, npRect->right, npRect->top );
}

//
// NCPaintHyperoid - paint a custom frame
//

VOID NEAR PASCAL NCPaintHyperoid( HWND hWnd )
{
	HDC             hDC, hDCMem;
	INT             cx, cy, cyCap, h;
	HPEN            hpenHi, hpenLo;
	HBRUSH          hbr;
	HBITMAP         hbm, hbmOld;
	BITMAP          bm;
	RECT            rect;

	if (IsIconic( hWnd )) return;
	hDC = GetWindowDC( hWnd );
	GetWindowRect( hWnd, &rect );
	rect.right -= rect.left, rect.left = 0;
	rect.bottom -= rect.top, rect.top = 0;
	cx = GetSystemMetrics( SM_CXFRAME );
	cy = GetSystemMetrics( SM_CYFRAME );
	cyCap = cy + GetSystemMetrics( SM_CYCAPTION ) - 1;
	h = rect.bottom - (cyCap + cy);

	SelectPalette( hDC, hAppPalette, 0 );
	RealizePalette( hDC );
	if (bBW)
	{
		hbr = SelectObject( hDC, CreateSolidBrush( PALETTEINDEX( WHITE ) ) );
		hpenHi = hPen[BLACK];
		hpenLo = hPen[BLACK];
	}
	else
	{
		hbr = SelectObject( hDC, CreateSolidBrush( PALETTEINDEX( GREY ) ) );
		hpenHi = hPen[WHITE];
		hpenLo = hPen[DKGREY];
	}

	PatBlt( hDC, 0, 0, rect.right, cyCap, PATCOPY );
	PatBlt( hDC, 0, rect.bottom - cy, rect.right, rect.bottom, PATCOPY );
	PatBlt( hDC, 0, cyCap, cx, h, PATCOPY );
	PatBlt( hDC, rect.right - cx, cyCap, cx, h, PATCOPY );

	--rect.bottom; --rect.right;
	DrawShadowRect( hDC, &rect, hpenHi, hpenLo );
	--cx; --cy;
	rect.left += cx; rect.top += cy;
	rect.right -= cx; rect.bottom -= cy;
	if (!bBW) DrawShadowRect( hDC, &rect, hpenLo, hpenHi );

	// get the title bar rect
	++rect.left; ++rect.top; --rect.right;
	rect.bottom = rect.top + cyCap - (cy + 2);
	DrawShadowRect( hDC, &rect, hpenHi, hpenLo );
	++rect.right; // for zoom/restore bitmap

	hDCMem = CreateCompatibleDC( hDC );

	hbm = LoadBitmap( NULL, INTRES(OBM_CLOSE) );
	GetObject( hbm, sizeof(bm), (LPSTR)&bm );
	bm.bmWidth /= 2; // they packed two images in here!
	hbmOld = SelectObject( hDCMem, hbm );
	BitBlt( hDC, rect.left, rect.top, bm.bmWidth, bm.bmHeight, hDCMem, 0, 0, SRCCOPY );
	rect.left += bm.bmWidth;

	if (IsZoomed( hWnd )) hbm = LoadBitmap( NULL, INTRES(OBM_RESTORE) );
	else hbm = LoadBitmap( NULL, INTRES(OBM_ZOOM) );
	GetObject( hbm, sizeof(bm), (LPSTR)&bm );
	SelectObject( hDCMem, hbm );
	rect.right -= bm.bmWidth;
	BitBlt( hDC, rect.right, rect.top, bm.bmWidth, bm.bmHeight, hDCMem, 0, 0, SRCCOPY );

	hbm = LoadBitmap( NULL, INTRES(OBM_REDUCE) );
	GetObject( hbm, sizeof(bm), (LPSTR)&bm );
	SelectObject( hDCMem, hbm );
	rect.right -= bm.bmWidth;
	BitBlt( hDC, rect.right, rect.top, bm.bmWidth, bm.bmHeight, hDCMem, 0, 0, SRCCOPY );

	--rect.right;
	DrawShadowRect( hDC, &rect, hpenHi, hpenLo );

	// clip the score to the free titlebar area
	++rect.left; ++rect.top;
	rectScoreClip = rect;

	DeleteObject( SelectObject( hDCMem, hbmOld ) );
	DeleteObject( SelectObject( hDC, hbr ) );
	DeleteDC( hDCMem );
	ReleaseDC( hWnd, hDC );

	// make sure the score gets redrawn
	for (cx = 0; cx < nScoreLen; ++cx) szScore[cx] = '\0';
}

//
// HyperoidWndProc - the main window proc for Hyperoid
//

LONG FAR PASCAL EXPORT HyperoidWndProc( HWND hWnd, unsigned message,
										WORD wParam, LONG lParam )
{
	switch (message)
	{
	case WM_CREATE:
		RestartHyperoid();
		SetTimer( hWnd, DRAW_TIMER, nDrawDelay, NULL );
		NCPaintHyperoid( hWnd );
		break;

	case WM_TIMER:
		switch (wParam)
		{
		case DRAW_TIMER:
			CheckScore( hWnd );
			DrawObjects( hWnd );
			return( 0 );

		case RESTART_TIMER:
			KillTimer( hWnd, RESTART_TIMER );
			bRestart = FALSE;
			RestartHyperoid();
			return( 0 );
		}
		break;

	case WM_SYSCOMMAND:
		switch (wParam)
		{
		case IDM_NEW:
			NewGame( hWnd );
			break;

		case IDM_ABOUT:
			AboutHyperoid( hWnd );
			break;

		default:
			return( DefWindowProc( hWnd, message, wParam, lParam ) );
		}
		break;

	case WM_QUERYOPEN:
		Panic( FALSE );
		return( DefWindowProc( hWnd, message, wParam, lParam ) );

	case WM_CHAR:
		if (wParam == VK_ESCAPE) Panic( TRUE );
		break;

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_SYSCHAR:
		if (lParam & (1L<<29)) // alt key is down
		{
			return( DefWindowProc( hWnd, message, wParam, lParam ) );
		}
		switch (wParam)
		{
		case VK_ESCAPE:
			if (message == WM_SYSKEYDOWN) Panic( TRUE );
			return( 0 );
		case VK_SPACE:
		case VK_TAB:
			return( 0 );
		default:
			return( DefWindowProc( hWnd, message, wParam, lParam ) );
		}
		break;

	case WM_ERASEBKGND:
		return( EraseHyperoidBkgnd( hWnd, (HDC)wParam ) );

	case WM_NCACTIVATE:
	case WM_NCPAINT:
		NCPaintHyperoid( hWnd );
		return( TRUE );

	case WM_PAINT:
		PaintHyperoid( hWnd );
		break;

	case WM_QUERYNEWPALETTE:
		{
			HDC hDC = GetDC( hWnd );
			SelectPalette( hDC, hAppPalette, 0 );
			RealizePalette( hDC );
			ReleaseDC( hWnd, hDC );
		}
		return( TRUE );

    case WM_DESTROY:
		KillTimer( hWnd, DRAW_TIMER );
		KillTimer( hWnd, RESTART_TIMER );
		SaveHyperoidWindowPos( hWnd );
		PostQuitMessage( 0 );
        break;

	default:
		return( DefWindowProc( hWnd, message, wParam, lParam ) );
    }
	return( 0 );
}

//
// InitHyperoid - initialize everything
//

BOOL NEAR PASCAL InitHyperoid( VOID )
{
	DOUBLE          dRad;
	INT             nCnt;

	// allocate the logical palette
	hAppPalette = CreateHyperoidPalette();
	if (!hAppPalette) return( FALSE );
	for (nCnt = 0; nCnt < PALETTE_SIZE; ++nCnt)
	{
		hPen[nCnt] = CreatePen( PS_SOLID, 1, PALETTEINDEX( nCnt ) );
		if (!hPen[nCnt]) return( FALSE );
	}
	for (nCnt = 0; nCnt < IDB_MAX; ++nCnt)
	{
		hBitmap[nCnt] = LoadBitmap( hAppInst, INTRES(IDB_blank + nCnt) );
		if (!hPen[nCnt]) return( FALSE );
	}

	// seed the randomizer
	dwSeed = GetCurrentTime();

	// create the lookup table (should use resources)
	for (nCnt = 0; nCnt < DEGREE_SIZE; ++nCnt)
	{
		dRad = nCnt * 6.2831855 / DEGREE_SIZE;
		nCos[nCnt] = (INT)(DEGREE_MAX * cos( dRad ));
		nSin[nCnt] = (INT)(DEGREE_MAX * sin( dRad ));
	}

	// get the initialization file info
	GetHyperoidIni();

	// allocate all objects as free
	for (nCnt = 0; nCnt < MAX_OBJS; ++nCnt)
		AddHeadObj( &FreeList, &(Obj[nCnt]) );

	// set up the player
	npPlayer = RemHeadObj( &FreeList );
	npPlayer->byPts = DIM(Player);
	npPlayer->nMass = 256;
	for (nCnt = 0; nCnt < DIM(Player); ++nCnt)
		npPlayer->Pts[nCnt] = Player[nCnt];

	return( TRUE );
}

//
// ExitHyperoid - quit the damn game already!
//

VOID NEAR PASCAL ExitHyperoid( VOID )
{
	INT             nCnt;

	if (hAppPalette) DeleteObject( hAppPalette );
	for (nCnt = 0; nCnt < PALETTE_SIZE; ++nCnt)
		if (hPen[nCnt]) DeleteObject( hPen[nCnt] );
	for (nCnt = 0; nCnt < IDB_MAX; ++nCnt)
		if (hBitmap[nCnt]) DeleteObject( hBitmap[nCnt] );
}

//
// WinMain - everybody has to have one
//

INT FAR PASCAL WinMain( HANDLE hInstance, HANDLE hPrevInstance,
						LPSTR lpszCmdLine, INT nCmdShow )
{
	MSG         msg;

	hAppInst = hInstance;
	if (!hPrevInstance)
	{
		// create the class if we're first
		if (!CreateHyperoidClass()) return( FALSE );
	}
	else
	{
		// Copy data from previous instance
		GetInstanceData( hPrevInstance, (PSTR)szAppName, sizeof(szAppName) );
	}
	if (!InitHyperoid()) goto Abort; // I LOVE GOTOS! REALLY I DO!
	hAppWnd = CreateHyperoidWindow( lpszCmdLine, nCmdShow );
	if (!hAppWnd) return( FALSE );

	while (GetMessage( &msg, NULL, 0, 0 ))
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

Abort:
	ExitHyperoid();
	return( msg.wParam );
}
