/*
 * GDI bit-blit operations
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include "gdi.h"
#include "metafile.h"
#include "options.h"
#include "stddebug.h"
/* #define DEBUG_GDI */
/* #undef  DEBUG_GDI */
#include "debug.h"

extern const int DC_XROPfunction[];
extern Colormap COLOR_WinColormap;

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))


/***********************************************************************
 *           BITBLT_GetImage
 */
static XImage *BITBLT_GetImage( HDC hdc, int x, int y, int width, int height )
{
    XImage *image;
    RECT rect;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );

    GetClipBox( hdc, &rect );
    OffsetRect( &rect, dc->w.DCOrgX, dc->w.DCOrgY );
    if ((x >= rect.left) && (y >= rect.top)
        && (x+width < rect.right) && (y+height < rect.bottom))
    {
        image = XGetImage( display, dc->u.x.drawable, x, y, width, height,
                           AllPlanes, ZPixmap );
    }
    else  /* Get only the visible sub-image */
    {
        int width_bytes = ((dc->w.bitsPerPixel == 24 ? 32 : dc->w.bitsPerPixel)
                           * width + 31) / 32 * 4;
        char *data = malloc( height * width_bytes );
        image = XCreateImage( display, DefaultVisualOfScreen(screen),
                              dc->w.bitsPerPixel, ZPixmap, 0, data,
                              width, height, 32, width_bytes );
        if (image && !IsRectEmpty(&rect))
        {
            int x1, y1, x2, y2;
            x1 = max( x, rect.left );
            y1 = max( y, rect.top );
            x2 = min( x + width, rect.right );
            y2 = min( y + height, rect.bottom );
            if ((x1 < x2) && (y1 < y2))
                XGetSubImage( display, dc->u.x.drawable, x1, y1, x2-x1, y2-y1,
                              AllPlanes, ZPixmap, image, x1-x, y1-y );
        }
    }
    return image;
}


/***********************************************************************
 *           PatBlt    (GDI.29)
 */
BOOL PatBlt( HDC hdc, short left, short top,
	     short width, short height, DWORD rop)
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam6(dc, META_PATBLT, left, top, width, height,
		      HIWORD(rop), LOWORD(rop));
	return TRUE;
    }

    dprintf_gdi(stddeb, "PatBlt: %d %d,%d %dx%d %06lx\n",
	    hdc, left, top, width, height, rop );

      /* Convert ROP3 code to ROP2 code */
    rop >>= 16;
    if (!DC_SetupGCForBrush( dc )) rop &= 0x0f;
    else rop = (rop & 0x03) | ((rop >> 4) & 0x0c);

      /* Special case for BLACKNESS and WHITENESS */
    if (!Options.usePrivateMap && ((rop == R2_BLACK-1) || (rop == R2_WHITE-1)))
    {
        XSetForeground( display, dc->u.x.gc, (rop == R2_BLACK-1) ?
                      BlackPixelOfScreen(screen) : WhitePixelOfScreen(screen));
        XSetFillStyle( display, dc->u.x.gc, FillSolid );
        rop = R2_COPYPEN;
    }

    XSetFunction( display, dc->u.x.gc, DC_XROPfunction[rop] );

    left = dc->w.DCOrgX + XLPTODP( dc, left );
    top  = dc->w.DCOrgY + YLPTODP( dc, top );

      /* Convert dimensions to device coords */
    if ((width = (width * dc->w.VportExtX) / dc->w.WndExtX) < 0)
    {
        width = -width;
        left -= width;
    }
    if ((height = (height * dc->w.VportExtY) / dc->w.WndExtY) < 0)
    {
        height = -height;
        top -= height;
    }

    XFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
                    left, top, width, height );
    return TRUE;
}


/***********************************************************************
 *           BitBlt    (GDI.34)
 */
BOOL BitBlt( HDC hdcDest, short xDest, short yDest, short width, short height,
	     HDC hdcSrc, short xSrc, short ySrc, DWORD rop )
{
    int xs1, xs2, ys1, ys2;
    int xd1, xd2, yd1, yd2;
    DWORD saverop = rop;
    DC *dcDest, *dcSrc;

    dprintf_gdi(stddeb, "BitBlt: %04x %d,%d %dx%d %04x %d,%d %06lx\n",
                hdcDest, xDest, yDest, width, height, hdcSrc, xSrc, ySrc, rop);

    if (width == 0 || height == 0) return FALSE;
    if ((rop & 0xcc0000) == ((rop & 0x330000) << 2))
	return PatBlt( hdcDest, xDest, yDest, width, height, rop );

    rop >>= 16;

    dcSrc = (DC *) GDI_GetObjPtr( hdcSrc, DC_MAGIC );
    if (!dcSrc) return FALSE;
    dcDest = (DC *) GDI_GetObjPtr( hdcDest, DC_MAGIC );
    if (!dcDest) 
    {
	dcDest = (DC *)GDI_GetObjPtr(hdcDest, METAFILE_DC_MAGIC);
	if (!dcDest) return FALSE;
	MF_BitBlt(dcDest, xDest, yDest, width, height,
		  hdcSrc, xSrc, ySrc, saverop);
	return TRUE;
    }

    xs1 = dcSrc->w.DCOrgX + XLPTODP( dcSrc, xSrc );
    xs2 = dcSrc->w.DCOrgX + XLPTODP( dcSrc, xSrc + width );
    ys1 = dcSrc->w.DCOrgY + YLPTODP( dcSrc, ySrc );
    ys2 = dcSrc->w.DCOrgY + YLPTODP( dcSrc, ySrc + height );
    xd1 = dcDest->w.DCOrgX + XLPTODP( dcDest, xDest );
    xd2 = dcDest->w.DCOrgX + XLPTODP( dcDest, xDest + width );
    yd1 = dcDest->w.DCOrgY + YLPTODP( dcDest, yDest );
    yd2 = dcDest->w.DCOrgY + YLPTODP( dcDest, yDest + height );

    if ((abs(xs2-xs1) != abs(xd2-xd1)) || (abs(ys2-ys1) != abs(yd2-yd1)))
	return FALSE;  /* Should call StretchBlt here */
    
    DC_SetupGCForText( dcDest );
    if (((rop & 0x0f) == (rop >> 4))&&(rop!=0xbb))
	/* FIXME: Test, whether more than just 0xbb has to be excluded */
      {
        XSetFunction( display, dcDest->u.x.gc, DC_XROPfunction[rop & 0x0f] );
        if (dcSrc->w.bitsPerPixel == dcDest->w.bitsPerPixel)
        {
	    XCopyArea( display, dcSrc->u.x.drawable,
	    	       dcDest->u.x.drawable, dcDest->u.x.gc,
		       min(xs1,xs2), min(ys1,ys2), abs(xs2-xs1), abs(ys2-ys1),
		       min(xd1,xd2), min(yd1,yd2) );
        }
        else
        {
	   if (dcSrc->w.bitsPerPixel != 1) return FALSE;
	    XCopyPlane( display, dcSrc->u.x.drawable,
	  	        dcDest->u.x.drawable, dcDest->u.x.gc,
		        min(xs1,xs2), min(ys1,ys2), abs(xs2-xs1), abs(ys2-ys1),
		        min(xd1,xd2), min(yd1,yd2), 1 );
        }
      }  
    else
      {
        XImage *sxi, *dxi, *bxi;
	int x,y,s,d,p,res,ofs,i,cp,cs,cd,cres;
	XColor sentry,dentry,pentry,entry;
	long colors[256];

	/* HDC hdcBrush = CreateCompatibleDC(hdcDest);
	DC *dcBrush;*/
	RECT r = {min(xDest,xDest+width), min(yDest,yDest+height), 
		  MAX(xDest,xDest+width), MAX(yDest,yDest+height)};
	HBRUSH cur_brush=SelectObject(hdcDest, GetStockObject(BLACK_BRUSH));
	SelectObject(hdcDest, cur_brush);
        /* FillRect(hdcBrush, &r, cur_brush);*/
        sxi = BITBLT_GetImage( hdcSrc, min(xs1,xs2), min(ys1,ys2),
                               abs(xs2-xs1), abs(ys2-ys1) );
        dxi = BITBLT_GetImage( hdcDest, min(xd1,xd2), min(yd1,yd2),
                               abs(xs2-xs1), abs(ys2-ys1) );
        /* dcBrush = (DC *) GDI_GetObjPtr( hdcBrush, DC_MAGIC );*/
        /* bxi=XGetImage(display, dcBrush->u.x.drawable, min(xd1,xd2),min(yd1,yd2),
             abs(xs2-xs1), abs(ys2-ys1), AllPlanes, ZPixmap);*/
	/* FIXME: It's really not necessary to do this on the visible screen */
        FillRect(hdcDest, &r, cur_brush);
        bxi = BITBLT_GetImage( hdcDest, min(xd1,xd2), min(yd1,yd2),
                               abs(xs2-xs1), abs(ys2-ys1) );
        for (i=0; i<min(256,1<<(dcDest->w.bitsPerPixel)); i++)
	{
	  entry.pixel = i;
	  XQueryColor ( display, COLOR_WinColormap, &entry);
	  colors[i] = (int) RGB( entry.red>>8, entry.green>>8, entry.blue>>8 );
	}
	if (dcSrc->w.bitsPerPixel == dcDest->w.bitsPerPixel)
        {
 	  for(x=0; x<abs(xs2-xs1); x++)
	  {
	    for(y=0; y<abs(ys2-ys1); y++)
	    {
	      s = XGetPixel(sxi, x, y);
	      d = XGetPixel(dxi, x, y);
	      p = XGetPixel(bxi, x, y);
	      if (s<256)
		cs=colors[s];
	      else
	      {
	        sentry.pixel = s;
		XQueryColor ( display, COLOR_WinColormap, &sentry);
		cs = (int) RGB( sentry.red>>8,sentry.green>>8, sentry.blue>>8 );
	      }
	      if (d<256)
		cd=colors[d];
	      else
	      {
       		dentry.pixel = d;
		XQueryColor ( display, COLOR_WinColormap, &dentry);
		cd = (int) RGB( dentry.red>>8, dentry.green>>8,dentry.blue>>8 );
	      }
	      if (p<256)
		cp=colors[p];
	      else
    	      {	      
	        pentry.pixel = p;
	        XQueryColor ( display, COLOR_WinColormap, &pentry);
	        cp = (int) RGB( pentry.red>>8, pentry.green>>8,pentry.blue>>8 );
	      }
	      cres = 0;
	      for(i=0; i<24; i++)
 	      {
		ofs=1<<(((cp>>i)&1)*4+((cs>>i)&1)*2+((cd>>i)&1));
		if (rop & ofs)
		  cres |= (1<<i);
	      }
	      if (cres==cs)
		res=s;
	      else if (cres==cd)
		res=d;
	      else if (cres==cp)
		res=p;
	      else
	      {
		res = -1;
	        for (i=0; i<min(256,1<<(dcDest->w.bitsPerPixel)); i++)
		  if (colors[i]==cres)
		  {
		    res = i;
		    break;
	          }
		if (res == -1)
	          res = GetNearestPaletteIndex(dcDest->w.hPalette, res);
	      } 	
	      XPutPixel(dxi, x, y, res);
	    }
	  }
        }
	else
	    fprintf(stderr,"BitBlt // depths different!\n");
	XPutImage(display, dcDest->u.x.drawable, dcDest->u.x.gc,
            dxi, 0, 0, min(xd1,xd2), min(yd1,yd2), abs(xs2-xs1), abs(ys2-ys1)+38);
	XDestroyImage(sxi);
        XDestroyImage(dxi);
	XDestroyImage(bxi);
	/*DeleteDC(hdcBrush);*/
      }
    return TRUE;
}



/***********************************************************************
 *           black on white stretch -- favors color pixels over white
 * 
 */
static void bonw_stretch(XImage *sxi, XImage *dxi, 
	short widthSrc, short heightSrc, short widthDest, short heightDest)
{
    float deltax, deltay, sourcex, sourcey, oldsourcex, oldsourcey;
    register int x, y;
    Pixel whitep;
    int totalx, totaly, xavgwhite, yavgwhite;
    register int i;
    int endx, endy;
    
    deltax = (float)widthSrc/widthDest;
    deltay = (float)heightSrc/heightDest;
    whitep  = WhitePixel(display, DefaultScreen(display));

    oldsourcex = 0;
    for (x=0, sourcex=0.0; x<widthDest; 
			x++, oldsourcex=sourcex, sourcex+=deltax) {
        xavgwhite = 0;
	if (deltax > 1.0) {
            totalx = 0;
	    endx = (int)sourcex;
	    for (i=(int)oldsourcex; i<=endx; i++)
	        if (XGetPixel(sxi, i, (int)sourcey) == whitep)
	            totalx++;
  	    xavgwhite = (totalx > (int)(deltax / 2.0));
	} else {
	    xavgwhite = 0;
	}

        oldsourcey = 0;
        for (y=0, sourcey=0.0; y<heightDest; 
				y++, oldsourcey=sourcey, sourcey+=deltay) {
	    yavgwhite = 0;
	    if (deltay > 1.0) {
	        totaly = 0;
	        endy = (int)sourcey;
	        for (i=(int)oldsourcey; i<=endy; i++) 
	            if (XGetPixel(sxi, (int)sourcex, i) == whitep)
		        totaly++;
	        yavgwhite = (totaly > ((int)deltay / 2));
	    } else {
		yavgwhite = 0;
	    }
	    if (xavgwhite && yavgwhite)
	        XPutPixel(dxi, x, y, whitep);
	    else
	        XPutPixel(dxi, x, y, XGetPixel(sxi, (int)sourcex, (int)sourcey));

	} /* for all y in dest */
    } /* for all x in dest */

}

/***********************************************************************
 *           white on black stretch -- favors color pixels over black
 * 
 */
static void wonb_stretch(XImage *sxi, XImage *dxi, 
	short widthSrc, short heightSrc, short widthDest, short heightDest)
{
    float deltax, deltay, sourcex, sourcey, oldsourcex, oldsourcey;
    register int x, y;
    Pixel blackp;
    int totalx, totaly, xavgblack, yavgblack;
    register int i;
    int endx, endy;
    
    deltax = (float)widthSrc/widthDest;
    deltay = (float)heightSrc/heightDest;
    blackp  = WhitePixel(display, DefaultScreen(display));

    oldsourcex = 0;
    for (x=0, sourcex=0.0; x<widthDest; 
			x++, oldsourcex=sourcex, sourcex+=deltax) {
        xavgblack = 0;
	if (deltax > 1.0) {
            totalx = 0;
	    endx = (int)sourcex;
	    for (i=(int)oldsourcex; i<=endx; i++)
	        if (XGetPixel(sxi, i, (int)sourcey) == blackp)
	            totalx++;
  	    xavgblack = (totalx > (int)(deltax / 2.0));
	} else {
	    xavgblack = 0;
	}

        oldsourcey = 0;
        for (y=0, sourcey=0.0; y<heightDest; 
				y++, oldsourcey=sourcey, sourcey+=deltay) {
	    yavgblack = 0;
	    if (deltay > 1.0) {
	        totaly = 0;
	        endy = (int)sourcey;
	        for (i=(int)oldsourcey; i<=endy; i++) 
	            if (XGetPixel(sxi, (int)sourcex, i) == blackp)
		        totaly++;
	        yavgblack = (totaly > ((int)deltay / 2));
	    } else {
		yavgblack = 0;
	    }
	    if (xavgblack && yavgblack)
	        XPutPixel(dxi, x, y, blackp);
	    else
	        XPutPixel(dxi, x, y, XGetPixel(sxi, (int)sourcex, (int)sourcey));

	} /* for all y in dest */
    } /* for all x in dest */
}

/* We use the 32-bit to 64-bit multiply and 64-bit to 32-bit divide of the */
/* 386 (which gcc doesn't know well enough) to efficiently perform integer */
/* scaling without having to worry about overflows. */

/* ##### muldiv64() borrowed from svgalib 1.03 ##### */
static __inline__ int muldiv64( int m1, int m2, int d ) 
{
	/* int32 * int32 -> int64 / int32 -> int32 */
#ifdef i386	
	int result;
	__asm__(
		"imull %%edx\n\t"
		"idivl %3\n\t"
		: "=&a" (result)		/* out */
		: "0" (m1), "d" (m2), "g" (d)	/* in */
		: "%edx"			/* mod */
	);
	return result;
#else
	return m1 * m2 / d;
#endif
}

/***********************************************************************
 *          color stretch -- deletes unused pixels
 * 
 */
static void color_stretch(XImage *sxi, XImage *dxi, 
	short widthSrc, short heightSrc, short widthDest, short heightDest)
{
	register int x, y, sx, sy, xfactor, yfactor;

	xfactor = muldiv64(widthSrc, 65536, widthDest);
	yfactor = muldiv64(heightSrc, 65536, heightDest);

	sy = 0;

	for (y = 0; y < heightDest;) 
	{
		int sourcey = sy >> 16;
		sx = 0;
		for (x = 0; x < widthDest; x++) {
            		XPutPixel(dxi, x, y, XGetPixel(sxi, sx >> 16, sourcey));
			sx += xfactor;
		}
		y++;
		while (y < heightDest) {
			int py;

			sourcey = sy >> 16;
			sy += yfactor;

			if ((sy >> 16) != sourcey)
				break;

			/* vertical stretch => copy previous line */
			
			py = y - 1;

			for (x = 0; x < widthDest; x++)
	            		XPutPixel(dxi, x, y, XGetPixel(dxi, x, py));
        	    	y++;
		}
	}
}

/***********************************************************************
 *           StretchBlt    (GDI.35)
 * 
 * 	o StretchBlt is CPU intensive so we only call it if we have
 *        to.  Checks are made to see if we can call BitBlt instead.
 *
 * 	o the stretching is slowish, some integer interpolation would
 *        speed it up.
 *
 *      o only black on white and color copy have been tested
 */
BOOL StretchBlt( HDC hdcDest, short xDest, short yDest, short widthDest, short heightDest,
               HDC hdcSrc, short xSrc, short ySrc, short widthSrc, short heightSrc, DWORD rop )
{
    int xs1, xs2, ys1, ys2;
    int xd1, xd2, yd1, yd2;
    DC *dcDest, *dcSrc;
    XImage *sxi, *dxi;
    DWORD saverop = rop;
    WORD stretchmode;

    dprintf_gdi(stddeb, "StretchBlt: %d %d,%d %dx%d %d %d,%d %dx%d %06lx\n",
           hdcDest, xDest, yDest, widthDest, heightDest, hdcSrc, xSrc, 
           ySrc, widthSrc, heightSrc, rop );
    dprintf_gdi(stddeb, "StretchMode is %x\n", 
           ((DC *)GDI_GetObjPtr(hdcDest, DC_MAGIC))->w.stretchBltMode);	

	if (widthDest == 0 || heightDest == 0) return FALSE;
	if (widthSrc == 0 || heightSrc == 0) return FALSE;
    if ((rop & 0xcc0000) == ((rop & 0x330000) << 2))
        return PatBlt( hdcDest, xDest, yDest, widthDest, heightDest, rop );

    /* don't stretch the bitmap unless we have to; if we don't,
     * call BitBlt for a performance boost
     */

    if (widthSrc == widthDest && heightSrc == heightDest) {
	return BitBlt(hdcDest, xDest, yDest, widthSrc, heightSrc,
               hdcSrc, xSrc, ySrc, rop);
    }

    rop >>= 16;
    if ((rop & 0x0f) != (rop >> 4))
    {
        fprintf(stdnimp, "StretchBlt: Unimplemented ROP %02lx\n", rop );
        return FALSE;
    }

    dcSrc = (DC *) GDI_GetObjPtr( hdcSrc, DC_MAGIC );
    if (!dcSrc) return FALSE;
    dcDest = (DC *) GDI_GetObjPtr( hdcDest, DC_MAGIC );
    if (!dcDest) 
    {
	dcDest = (DC *)GDI_GetObjPtr(hdcDest, METAFILE_DC_MAGIC);
	if (!dcDest) return FALSE;
	MF_StretchBlt(dcDest, xDest, yDest, widthDest, heightDest,
		  hdcSrc, xSrc, ySrc, widthSrc, heightSrc, saverop);
	return TRUE;
    }

    xs1 = dcSrc->w.DCOrgX + XLPTODP( dcSrc, xSrc );
    xs2 = dcSrc->w.DCOrgX + XLPTODP( dcSrc, xSrc + widthSrc );
    ys1 = dcSrc->w.DCOrgY + YLPTODP( dcSrc, ySrc );
    ys2 = dcSrc->w.DCOrgY + YLPTODP( dcSrc, ySrc + heightSrc );
    xd1 = dcDest->w.DCOrgX + XLPTODP( dcDest, xDest );
    xd2 = dcDest->w.DCOrgX + XLPTODP( dcDest, xDest + widthDest );
    yd1 = dcDest->w.DCOrgY + YLPTODP( dcDest, yDest );
    yd2 = dcDest->w.DCOrgY + YLPTODP( dcDest, yDest + heightDest );


    /* get a source and destination image so we can manipulate
     * the pixels
     */

    sxi = BITBLT_GetImage( hdcSrc, xs1, ys1, widthSrc, heightSrc );
    dxi = XCreateImage(display, DefaultVisualOfScreen(screen),
	  		    screenDepth, ZPixmap,
			    0, NULL, widthDest, heightDest,
			    32, 0);
    dxi->data = malloc(dxi->bytes_per_line * heightDest);

    stretchmode = ((DC *)GDI_GetObjPtr(hdcDest, DC_MAGIC))->w.stretchBltMode;

     /* the actual stretching is done here, we'll try to use
      * some interolation to get some speed out of it in
      * the future
      */

    switch (stretchmode) {
	case BLACKONWHITE:
		color_stretch(sxi, dxi, widthSrc, heightSrc, 
				widthDest, heightDest);
/*		bonw_stretch(sxi, dxi, widthSrc, heightSrc,
				widthDest, heightDest);
*/		break;
	case WHITEONBLACK:
		color_stretch(sxi, dxi, widthSrc, heightSrc, 
				widthDest, heightDest);
/*		wonb_stretch(sxi, dxi, widthSrc, heightSrc, 
				widthDest, heightDest);
*/		break;
	case COLORONCOLOR:
		color_stretch(sxi, dxi, widthSrc, heightSrc, 
				widthDest, heightDest);
		break;
	default:
		fprintf(stderr, "StretchBlt: unknown stretchmode '%d'\n",
			stretchmode);
		break;
    }

    DC_SetupGCForText(dcDest);
    XSetFunction(display, dcDest->u.x.gc, DC_XROPfunction[rop & 0x0f]);
    XPutImage(display, dcDest->u.x.drawable, dcDest->u.x.gc,
	 	dxi, 0, 0, min(xd1,xd2), min(yd1,yd2), 
		widthDest, heightDest);

    /* now free the images we created */

    XDestroyImage(sxi);
    XDestroyImage(dxi);

    return TRUE;
}

