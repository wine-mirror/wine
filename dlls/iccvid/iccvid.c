/*
 * Radius Cinepak Video Decoder
 *
 * Copyright 2001 Dr. Tim Ferguson (see below)
 * Portions Copyright 2003 Mike McCormack for CodeWeavers
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

/* Copyright notice from original source:
 * ------------------------------------------------------------------------
 * Radius Cinepak Video Decoder
 *
 * Dr. Tim Ferguson, 2001.
 * For more details on the algorithm:
 *         http://www.csse.monash.edu.au/~timf/videocodec.html
 *
 * This is basically a vector quantiser with adaptive vector density.  The
 * frame is segmented into 4x4 pixel blocks, and each block is coded using
 * either 1 or 4 vectors.
 *
 * There are still some issues with this code yet to be resolved.  In
 * particular with decoding in the strip boundaries.  However, I have not
 * yet found a sequence it doesn't work on.  Ill keep trying :)
 *
 * You may freely use this source code.  I only ask that you reference its
 * source in your projects documentation:
 *       Tim Ferguson: http://www.csse.monash.edu.au/~timf/
 * ------------------------------------------------------------------------ */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commdlg.h"
#include "vfw.h"

#include "mmsystem.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(iccvid);

#define ICCVID_MAGIC mmioFOURCC('c', 'v', 'i', 'd')

#define DBUG    0
#define MAX_STRIPS 32

/* ------------------------------------------------------------------------ */
typedef struct
{
    unsigned char y0, y1, y2, y3;
    char u, v;
    unsigned long rgb0, rgb1, rgb2, rgb3;        /* should be a union */
    unsigned char r[4], g[4], b[4];
} cvid_codebook;

typedef struct {
    cvid_codebook *v4_codebook[MAX_STRIPS];
    cvid_codebook *v1_codebook[MAX_STRIPS];
    int strip_num;
} cinepak_info;

typedef struct _ICCVID_Info
{
    DWORD         dwMagic;
    cinepak_info *cvinfo;
} ICCVID_Info;

static inline LPVOID ICCVID_Alloc( size_t size, size_t count )
{
    return HeapAlloc( GetProcessHeap(), 0, size*count );
}

static inline BOOL ICCVID_Free( LPVOID ptr )
{
    return HeapFree( GetProcessHeap(), 0, ptr );
}


/* ------------------------------------------------------------------------ */
static unsigned char *in_buffer, uiclip[1024], *uiclp = NULL;

#define get_byte() *(in_buffer++)
#define skip_byte() in_buffer++
#define get_word() ((unsigned short)(in_buffer += 2, \
    (in_buffer[-2] << 8 | in_buffer[-1])))
#define get_long() ((unsigned long)(in_buffer += 4, \
    (in_buffer[-4] << 24 | in_buffer[-3] << 16 | in_buffer[-2] << 8 | in_buffer[-1])))


/* ---------------------------------------------------------------------- */
static inline void read_codebook_32(cvid_codebook *c, int mode)
{
int uvr, uvg, uvb;

    if(mode)        /* black and white */
        {
        c->y0 = get_byte();
        c->y1 = get_byte();
        c->y2 = get_byte();
        c->y3 = get_byte();
        c->u = c->v = 0;

        c->rgb0 = (c->y0 << 16) | (c->y0 << 8) | c->y0;
        c->rgb1 = (c->y1 << 16) | (c->y1 << 8) | c->y1;
        c->rgb2 = (c->y2 << 16) | (c->y2 << 8) | c->y2;
        c->rgb3 = (c->y3 << 16) | (c->y3 << 8) | c->y3;
        }
    else            /* colour */
        {
        c->y0 = get_byte();  /* luma */
        c->y1 = get_byte();
        c->y2 = get_byte();
        c->y3 = get_byte();
        c->u = get_byte(); /* chroma */
        c->v = get_byte();

        uvr = c->v << 1;
        uvg = -((c->u+1) >> 1) - c->v;
        uvb = c->u << 1;

        c->rgb0 = (uiclp[c->y0 + uvr] << 16) | (uiclp[c->y0 + uvg] << 8) | uiclp[c->y0 + uvb];
        c->rgb1 = (uiclp[c->y1 + uvr] << 16) | (uiclp[c->y1 + uvg] << 8) | uiclp[c->y1 + uvb];
        c->rgb2 = (uiclp[c->y2 + uvr] << 16) | (uiclp[c->y2 + uvg] << 8) | uiclp[c->y2 + uvb];
        c->rgb3 = (uiclp[c->y3 + uvr] << 16) | (uiclp[c->y3 + uvg] << 8) | uiclp[c->y3 + uvb];
        }
}


/* ------------------------------------------------------------------------ */
inline void cvid_v1_32(unsigned char *frm, unsigned char *limit, int stride, cvid_codebook *cb)
{
unsigned long *vptr = (unsigned long *)frm, rgb;
int row_inc = stride/4;

#ifndef ORIGINAL
    vptr += row_inc*3;
#endif
    vptr[0] = rgb = cb->rgb0; vptr[1] = rgb;
    vptr[2] = rgb = cb->rgb1; vptr[3] = rgb;
#ifndef ORIGINAL
    vptr -= row_inc; if(vptr < (unsigned long *)limit) return;
#else
    vptr += row_inc; if(vptr > (unsigned long *)limit) return;
#endif
    vptr[0] = rgb = cb->rgb0; vptr[1] = rgb;
    vptr[2] = rgb = cb->rgb1; vptr[3] = rgb;
#ifndef ORIGINAL
    vptr -= row_inc; if(vptr < (unsigned long *)limit) return;
#else
    vptr += row_inc; if(vptr > (unsigned long *)limit) return;
#endif
    vptr[0] = rgb = cb->rgb2; vptr[1] = rgb;
    vptr[2] = rgb = cb->rgb3; vptr[3] = rgb;
#ifndef ORIGINAL
    vptr -= row_inc; if(vptr < (unsigned long *)limit) return;
#else
    vptr += row_inc; if(vptr > (unsigned long *)limit) return;
#endif
    vptr[0] = rgb = cb->rgb2; vptr[1] = rgb;
    vptr[2] = rgb = cb->rgb3; vptr[3] = rgb;
}


/* ------------------------------------------------------------------------ */
inline void cvid_v4_32(unsigned char *frm, unsigned char *limit, int stride, cvid_codebook *cb0,
    cvid_codebook *cb1, cvid_codebook *cb2, cvid_codebook *cb3)
{
unsigned long *vptr = (unsigned long *)frm;
int row_inc = stride/4;

#ifndef ORIGINAL
    vptr += row_inc*3;
#endif
    vptr[0] = cb0->rgb0;
    vptr[1] = cb0->rgb1;
    vptr[2] = cb1->rgb0;
    vptr[3] = cb1->rgb1;
#ifndef ORIGINAL
    vptr -= row_inc; if(vptr < (unsigned long *)limit) return;
#else
    vptr += row_inc; if(vptr > (unsigned long *)limit) return;
#endif
    vptr[0] = cb0->rgb2;
    vptr[1] = cb0->rgb3;
    vptr[2] = cb1->rgb2;
    vptr[3] = cb1->rgb3;
#ifndef ORIGINAL
    vptr -= row_inc; if(vptr < (unsigned long *)limit) return;
#else
    vptr += row_inc; if(vptr > (unsigned long *)limit) return;
#endif
    vptr[0] = cb2->rgb0;
    vptr[1] = cb2->rgb1;
    vptr[2] = cb3->rgb0;
    vptr[3] = cb3->rgb1;
#ifndef ORIGINAL
    vptr -= row_inc; if(vptr < (unsigned long *)limit) return;
#else
    vptr += row_inc; if(vptr > (unsigned long *)limit) return;
#endif
    vptr[0] = cb2->rgb2;
    vptr[1] = cb2->rgb3;
    vptr[2] = cb3->rgb2;
    vptr[3] = cb3->rgb3;
}


/* ---------------------------------------------------------------------- */
static inline void read_codebook_24(cvid_codebook *c, int mode)
{
int uvr, uvg, uvb;

    if(mode)        /* black and white */
        {
        c->y0 = get_byte();
        c->y1 = get_byte();
        c->y2 = get_byte();
        c->y3 = get_byte();
        c->u = c->v = 0;

        c->r[0] = c->g[0] = c->b[0] = c->y0;
        c->r[1] = c->g[1] = c->b[1] = c->y1;
        c->r[2] = c->g[2] = c->b[2] = c->y2;
        c->r[3] = c->g[3] = c->b[3] = c->y3;
        }
    else            /* colour */
        {
        c->y0 = get_byte();  /* luma */
        c->y1 = get_byte();
        c->y2 = get_byte();
        c->y3 = get_byte();
        c->u = get_byte(); /* chroma */
        c->v = get_byte();

        uvr = c->v << 1;
        uvg = -((c->u+1) >> 1) - c->v;
        uvb = c->u << 1;

        c->r[0] = uiclp[c->y0 + uvr]; c->g[0] = uiclp[c->y0 + uvg]; c->b[0] = uiclp[c->y0 + uvb];
        c->r[1] = uiclp[c->y1 + uvr]; c->g[1] = uiclp[c->y1 + uvg]; c->b[1] = uiclp[c->y1 + uvb];
        c->r[2] = uiclp[c->y2 + uvr]; c->g[2] = uiclp[c->y2 + uvg]; c->b[2] = uiclp[c->y2 + uvb];
        c->r[3] = uiclp[c->y3 + uvr]; c->g[3] = uiclp[c->y3 + uvg]; c->b[3] = uiclp[c->y3 + uvb];
        }
}


/* ------------------------------------------------------------------------ */
void cvid_v1_24(unsigned char *vptr, unsigned char *limit, int stride, cvid_codebook *cb)
{
unsigned char r, g, b;
#ifndef ORIGINAL
int row_inc = stride+4*3;
#else
int row_inc = stride-4*3;
#endif

#ifndef ORIGINAL
    vptr += (3*row_inc);
#endif
    *vptr++ = b = cb->b[0]; *vptr++ = g = cb->g[0]; *vptr++ = r = cb->r[0];
    *vptr++ = b; *vptr++ = g; *vptr++ = r;
    *vptr++ = b = cb->b[1]; *vptr++ = g = cb->g[1]; *vptr++ = r = cb->r[1];
    *vptr++ = b; *vptr++ = g; *vptr++ = r;
#ifndef ORIGINAL
    vptr -= row_inc; if(vptr < limit) return;
#else
    vptr += row_inc; if(vptr > limit) return;
#endif
    *vptr++ = b = cb->b[0]; *vptr++ = g = cb->g[0]; *vptr++ = r = cb->r[0];
    *vptr++ = b; *vptr++ = g; *vptr++ = r;
    *vptr++ = b = cb->b[1]; *vptr++ = g = cb->g[1]; *vptr++ = r = cb->r[1];
    *vptr++ = b; *vptr++ = g; *vptr++ = r;
#ifndef ORIGINAL
    vptr -= row_inc; if(vptr < limit) return;
#else
    vptr += row_inc; if(vptr > limit) return;
#endif
    *vptr++ = b = cb->b[2]; *vptr++ = g = cb->g[2]; *vptr++ = r = cb->r[2];
    *vptr++ = b; *vptr++ = g; *vptr++ = r;
    *vptr++ = b = cb->b[3]; *vptr++ = g = cb->g[3]; *vptr++ = r = cb->r[3];
    *vptr++ = b; *vptr++ = g; *vptr++ = r;
#ifndef ORIGINAL
    vptr -= row_inc; if(vptr < limit) return;
#else
    vptr += row_inc; if(vptr > limit) return;
#endif
    *vptr++ = b = cb->b[2]; *vptr++ = g = cb->g[2]; *vptr++ = r = cb->r[2];
    *vptr++ = b; *vptr++ = g; *vptr++ = r;
    *vptr++ = b = cb->b[3]; *vptr++ = g = cb->g[3]; *vptr++ = r = cb->r[3];
    *vptr++ = b; *vptr++ = g; *vptr++ = r;
}


/* ------------------------------------------------------------------------ */
void cvid_v4_24(unsigned char *vptr, unsigned char *limit, int stride, cvid_codebook *cb0,
    cvid_codebook *cb1, cvid_codebook *cb2, cvid_codebook *cb3)
{
#ifndef ORIGINAL
int row_inc = stride+4*3;
#else
int row_inc = stride-4*3;
#endif

#ifndef ORIGINAL
    vptr += (3*row_inc);
#endif
    *vptr++ = cb0->b[0]; *vptr++ = cb0->g[0]; *vptr++ = cb0->r[0];
    *vptr++ = cb0->b[1]; *vptr++ = cb0->g[1]; *vptr++ = cb0->r[1];
    *vptr++ = cb1->b[0]; *vptr++ = cb1->g[0]; *vptr++ = cb1->r[0];
    *vptr++ = cb1->b[1]; *vptr++ = cb1->g[1]; *vptr++ = cb1->r[1];
#ifndef ORIGINAL
    vptr -= row_inc; if(vptr < limit) return;
#else
    vptr += row_inc; if(vptr > limit) return;
#endif
    *vptr++ = cb0->b[2]; *vptr++ = cb0->g[2]; *vptr++ = cb0->r[2];
    *vptr++ = cb0->b[3]; *vptr++ = cb0->g[3]; *vptr++ = cb0->r[3];
    *vptr++ = cb1->b[2]; *vptr++ = cb1->g[2]; *vptr++ = cb1->r[2];
    *vptr++ = cb1->b[3]; *vptr++ = cb1->g[3]; *vptr++ = cb1->r[3];
#ifndef ORIGINAL
    vptr -= row_inc; if(vptr < limit) return;
#else
    vptr += row_inc; if(vptr > limit) return;
#endif
    *vptr++ = cb2->b[0]; *vptr++ = cb2->g[0]; *vptr++ = cb2->r[0];
    *vptr++ = cb2->b[1]; *vptr++ = cb2->g[1]; *vptr++ = cb2->r[1];
    *vptr++ = cb3->b[0]; *vptr++ = cb3->g[0]; *vptr++ = cb3->r[0];
    *vptr++ = cb3->b[1]; *vptr++ = cb3->g[1]; *vptr++ = cb3->r[1];
#ifndef ORIGINAL
    vptr -= row_inc; if(vptr < limit) return;
#else
    vptr += row_inc; if(vptr > limit) return;
#endif
    *vptr++ = cb2->b[2]; *vptr++ = cb2->g[2]; *vptr++ = cb2->r[2];
    *vptr++ = cb2->b[3]; *vptr++ = cb2->g[3]; *vptr++ = cb2->r[3];
    *vptr++ = cb3->b[2]; *vptr++ = cb3->g[2]; *vptr++ = cb3->r[2];
    *vptr++ = cb3->b[3]; *vptr++ = cb3->g[3]; *vptr++ = cb3->r[3];
}


/* ------------------------------------------------------------------------
 * Call this function once at the start of the sequence and save the
 * returned context for calls to decode_cinepak().
 */
cinepak_info *decode_cinepak_init(void)
{
    cinepak_info *cvinfo;
    int i;

    cvinfo = ICCVID_Alloc( sizeof (cinepak_info), 1 );
    if( !cvinfo )
        return NULL;
    cvinfo->strip_num = 0;

    if(uiclp == NULL)
    {
        uiclp = uiclip+512;
        for(i = -512; i < 512; i++)
            uiclp[i] = (i < 0 ? 0 : (i > 255 ? 255 : i));
    }

    return cvinfo;
}

void free_cvinfo( cinepak_info *cvinfo )
{
    int i;

    for( i=0; i<cvinfo->strip_num; i++ )
    {
        ICCVID_Free(cvinfo->v4_codebook[i]);
        ICCVID_Free(cvinfo->v1_codebook[i]);
    }
    ICCVID_Free( cvinfo );
}

typedef void (*fn_read_codebook)(cvid_codebook *c, int mode);
typedef void (*fn_cvid_v1)(unsigned char *frm, unsigned char *limit,
                           int stride, cvid_codebook *cb);
typedef void (*fn_cvid_v4)(unsigned char *frm, unsigned char *limit, int stride,
                           cvid_codebook *cb0, cvid_codebook *cb1,
                           cvid_codebook *cb2, cvid_codebook *cb3);

/* ------------------------------------------------------------------------
 * This function decodes a buffer containing a Cinepak encoded frame.
 *
 * context - the context created by decode_cinepak_init().
 * buf - the input buffer to be decoded
 * size - the size of the input buffer
 * frame - the output frame buffer (24 or 32 bit per pixel)
 * width - the width of the output frame
 * height - the height of the output frame
 * bit_per_pixel - the number of bits per pixel allocated to the output
 *   frame (only 24 or 32 bpp are supported)
 */
void decode_cinepak(cinepak_info *cvinfo, unsigned char *buf, int size,
           unsigned char *frame, int width, int height, int bit_per_pixel)
{
    cvid_codebook *v4_codebook, *v1_codebook, *codebook = NULL;
    unsigned long x, y, y_bottom, frame_flags, strips, cv_width, cv_height,
                  cnum, strip_id, chunk_id, x0, y0, x1, y1, ci, flag, mask;
    long len, top_size, chunk_size;
    unsigned char *frm_ptr, *frm_end;
    int i, cur_strip, d0, d1, d2, d3, frm_stride, bpp = 3;
    fn_read_codebook read_codebook = read_codebook_24;
    fn_cvid_v1 cvid_v1 = cvid_v1_24;
    fn_cvid_v4 cvid_v4 = cvid_v4_24;

    x = y = 0;
    y_bottom = 0;
    in_buffer = buf;

    frame_flags = get_byte();
    len = get_byte() << 16;
    len |= get_byte() << 8;
    len |= get_byte();

    switch(bit_per_pixel)
        {
        case 24:
            bpp = 3;
            read_codebook = read_codebook_24;
            cvid_v1 = cvid_v1_24;
            cvid_v4 = cvid_v4_24;
            break;
        case 32:
            bpp = 4;
            read_codebook = read_codebook_32;
            cvid_v1 = cvid_v1_32;
            cvid_v4 = cvid_v4_32;
            break;
        }

    frm_stride = width * bpp;
    frm_ptr = frame;
    frm_end = frm_ptr + width * height * bpp;

    if(len != size)
        {
        if(len & 0x01) len++; /* AVIs tend to have a size mismatch */
        if(len != size)
            {
            ERR("CVID: corruption %d (QT/AVI) != %ld (CV)\n", size, len);
            /* return; */
            }
        }

    cv_width = get_word();
    cv_height = get_word();
    strips = get_word();

    if(strips > cvinfo->strip_num)
        {
        if(strips >= MAX_STRIPS)
            {
            ERR("CVID: strip overflow (more than %d)\n", MAX_STRIPS);
            return;
            }

        for(i = cvinfo->strip_num; i < strips; i++)
            {
            if((cvinfo->v4_codebook[i] = (cvid_codebook *)ICCVID_Alloc(sizeof(cvid_codebook), 260)) == NULL)
                {
                ERR("CVID: codebook v4 alloc err\n");
                return;
                }

            if((cvinfo->v1_codebook[i] = (cvid_codebook *)ICCVID_Alloc(sizeof(cvid_codebook), 260)) == NULL)
                {
                ERR("CVID: codebook v1 alloc err\n");
                return;
                }
            }
        }
    cvinfo->strip_num = strips;

    TRACE("CVID: <%ld,%ld> strips %ld\n", cv_width, cv_height, strips);

    for(cur_strip = 0; cur_strip < strips; cur_strip++)
        {
        v4_codebook = cvinfo->v4_codebook[cur_strip];
        v1_codebook = cvinfo->v1_codebook[cur_strip];

        if((cur_strip > 0) && (!(frame_flags & 0x01)))
            {
            memcpy(cvinfo->v4_codebook[cur_strip], cvinfo->v4_codebook[cur_strip-1], 260 * sizeof(cvid_codebook));
            memcpy(cvinfo->v1_codebook[cur_strip], cvinfo->v1_codebook[cur_strip-1], 260 * sizeof(cvid_codebook));
            }

        strip_id = get_word();        /* 1000 = key strip, 1100 = iter strip */
        top_size = get_word();
        y0 = get_word();        /* FIXME: most of these are ignored at the moment */
        x0 = get_word();
        y1 = get_word();
        x1 = get_word();

        y_bottom += y1;
        top_size -= 12;
        x = 0;
        if(x1 != width)
            WARN("CVID: Warning x1 (%ld) != width (%d)\n", x1, width);

        TRACE("   %d) %04lx %04ld <%ld,%ld> <%ld,%ld> yt %ld\n",
              cur_strip, strip_id, top_size, x0, y0, x1, y1, y_bottom);

        while(top_size > 0)
            {
            chunk_id  = get_word();
            chunk_size = get_word();

            TRACE("        %04lx %04ld\n", chunk_id, chunk_size);
            top_size -= chunk_size;
            chunk_size -= 4;

            switch(chunk_id)
                {
                    /* -------------------- Codebook Entries -------------------- */
                case 0x2000:
                case 0x2200:
                    codebook = (chunk_id == 0x2200 ? v1_codebook : v4_codebook);
                    cnum = chunk_size/6;
                    for(i = 0; i < cnum; i++) read_codebook(codebook+i, 0);
                    break;

                case 0x2400:
                case 0x2600:        /* 8 bit per pixel */
                    codebook = (chunk_id == 0x2600 ? v1_codebook : v4_codebook);
                    cnum = chunk_size/4;
                    for(i = 0; i < cnum; i++) read_codebook(codebook+i, 1);
                    break;

                case 0x2100:
                case 0x2300:
                    codebook = (chunk_id == 0x2300 ? v1_codebook : v4_codebook);

                    ci = 0;
                    while(chunk_size > 0)
                        {
                        flag = get_long();
                        chunk_size -= 4;

                        for(i = 0; i < 32; i++)
                            {
                            if(flag & 0x80000000)
                                {
                                chunk_size -= 6;
                                read_codebook(codebook+ci, 0);
                                }

                            ci++;
                            flag <<= 1;
                            }
                        }
                    while(chunk_size > 0) { skip_byte(); chunk_size--; }
                    break;

                case 0x2500:
                case 0x2700:        /* 8 bit per pixel */
                    codebook = (chunk_id == 0x2700 ? v1_codebook : v4_codebook);

                    ci = 0;
                    while(chunk_size > 0)
                        {
                        flag = get_long();
                        chunk_size -= 4;

                        for(i = 0; i < 32; i++)
                            {
                            if(flag & 0x80000000)
                                {
                                chunk_size -= 4;
                                read_codebook(codebook+ci, 1);
                                }

                            ci++;
                            flag <<= 1;
                            }
                        }
                    while(chunk_size > 0) { skip_byte(); chunk_size--; }
                    break;

                    /* -------------------- Frame -------------------- */
                case 0x3000:
                    while((chunk_size > 0) && (y < y_bottom))
                        {
                        flag = get_long();
                        chunk_size -= 4;

                        for(i = 0; i < 32; i++)
                            {
                            if(y >= y_bottom) break;
                            if(flag & 0x80000000)    /* 4 bytes per block */
                                {
                                d0 = get_byte();
                                d1 = get_byte();
                                d2 = get_byte();
                                d3 = get_byte();
                                chunk_size -= 4;
#ifdef ORIGINAL
                                cvid_v4(frm_ptr + (y * frm_stride + x * bpp), frm_end, frm_stride, v4_codebook+d0, v4_codebook+d1, v4_codebook+d2, v4_codebook+d3);
#else
                                cvid_v4(frm_ptr + ((height - 1 - y) * frm_stride + x * bpp), frame, frm_stride, v4_codebook+d0, v4_codebook+d1, v4_codebook+d2, v4_codebook+d3);
#endif
                                }
                            else        /* 1 byte per block */
                                {
#ifdef ORIGINAL
                                cvid_v1(frm_ptr + (y * frm_stride + x * bpp), frm_end, frm_stride, v1_codebook + get_byte());
#else
                                cvid_v1(frm_ptr + ((height - 1 - y) * frm_stride + x * bpp), frame, frm_stride, v1_codebook + get_byte());
#endif
                                chunk_size--;
                                }

                            x += 4;
                            if(x >= width)
                                {
                                x = 0;
                                y += 4;
                                }
                            flag <<= 1;
                            }
                        }
                    while(chunk_size > 0) { skip_byte(); chunk_size--; }
                    break;

                case 0x3100:
                    while((chunk_size > 0) && (y < y_bottom))
                        {
                            /* ---- flag bits: 0 = SKIP, 10 = V1, 11 = V4 ---- */
                        flag = (unsigned long)get_long();
                        chunk_size -= 4;
                        mask = 0x80000000;

                        while((mask) && (y < y_bottom))
                            {
                            if(flag & mask)
                                {
                                if(mask == 1)
                                    {
                                    if(chunk_size < 0) break;
                                    flag = (unsigned long)get_long();
                                    chunk_size -= 4;
                                    mask = 0x80000000;
                                    }
                                else mask >>= 1;

                                if(flag & mask)        /* V4 */
                                    {
                                    d0 = get_byte();
                                    d1 = get_byte();
                                    d2 = get_byte();
                                    d3 = get_byte();
                                    chunk_size -= 4;
#ifdef ORIGINAL
                                    cvid_v4(frm_ptr + (y * frm_stride + x * bpp), frm_end, frm_stride, v4_codebook+d0, v4_codebook+d1, v4_codebook+d2, v4_codebook+d3);
#else
                                    cvid_v4(frm_ptr + ((height - 1 - y) * frm_stride + x * bpp), frame, frm_stride, v4_codebook+d0, v4_codebook+d1, v4_codebook+d2, v4_codebook+d3);
#endif
                                    }
                                else        /* V1 */
                                    {
                                    chunk_size--;
#ifdef ORIGINAL
                                    cvid_v1(frm_ptr + (y * frm_stride + x * bpp), frm_end, frm_stride, v1_codebook + get_byte());
#else
                                    cvid_v1(frm_ptr + ((height - 1 - y) * frm_stride + x * bpp), frame, frm_stride, v1_codebook + get_byte());
#endif
                                    }
                                }        /* else SKIP */

                            mask >>= 1;
                            x += 4;
                            if(x >= width)
                                {
                                x = 0;
                                y += 4;
                                }
                            }
                        }

                    while(chunk_size > 0) { skip_byte(); chunk_size--; }
                    break;

                case 0x3200:        /* each byte is a V1 codebook */
                    while((chunk_size > 0) && (y < y_bottom))
                        {
#ifdef ORIGINAL
                        cvid_v1(frm_ptr + (y * frm_stride + x * bpp), frm_end, frm_stride, v1_codebook + get_byte());
#else
                        cvid_v1(frm_ptr + ((height - 1 - y) * frm_stride + x * bpp), frame, frm_stride, v1_codebook + get_byte());
#endif
                        chunk_size--;
                        x += 4;
                        if(x >= width)
                            {
                            x = 0;
                            y += 4;
                            }
                        }
                    while(chunk_size > 0) { skip_byte(); chunk_size--; }
                    break;

                default:
                    ERR("CVID: unknown chunk_id %08lx\n", chunk_id);
                    while(chunk_size > 0) { skip_byte(); chunk_size--; }
                    break;
                }
            }
        }

    if(len != size)
        {
        if(len & 0x01) len++; /* AVIs tend to have a size mismatch */
        if(len != size)
            {
            long xlen;
            skip_byte();
            xlen = get_byte() << 16;
            xlen |= get_byte() << 8;
            xlen |= get_byte(); /* Read Len */
            WARN("CVID: END INFO chunk size %d cvid size1 %ld cvid size2 %ld\n",
                  size, len, xlen);
            }
        }
}

LRESULT ICCVID_DecompressQuery( ICCVID_Info *info, LPBITMAPINFO in, LPBITMAPINFO out )
{
    TRACE("ICM_DECOMPRESS_QUERY %p %p %p\n", info, in, out);

    if( (info==NULL) || (info->dwMagic!=ICCVID_MAGIC) )
        return ICERR_BADPARAM;

    TRACE("planes = %d\n", in->bmiHeader.biPlanes );
    TRACE("bpp    = %d\n", in->bmiHeader.biBitCount );
    TRACE("height = %ld\n", in->bmiHeader.biHeight );
    TRACE("width  = %ld\n", in->bmiHeader.biWidth );
    TRACE("compr  = %lx\n", in->bmiHeader.biCompression );

    if( in->bmiHeader.biCompression != ICCVID_MAGIC )
        return ICERR_UNSUPPORTED;

    switch( in->bmiHeader.biBitCount )
    {
    case 24:
    case 32:
        break;
    default:
        TRACE("bitcount = %d\n", in->bmiHeader.biBitCount );
        return ICERR_UNSUPPORTED;
    }

    if( out )
    {
        if( in->bmiHeader.biPlanes != out->bmiHeader.biPlanes )
            return ICERR_UNSUPPORTED;
        if( in->bmiHeader.biBitCount != out->bmiHeader.biBitCount )
            return ICERR_UNSUPPORTED;
        if( in->bmiHeader.biHeight != out->bmiHeader.biHeight )
            return ICERR_UNSUPPORTED;
        if( in->bmiHeader.biWidth != out->bmiHeader.biWidth )
            return ICERR_UNSUPPORTED;
    }

    return ICERR_OK;
}

LRESULT ICCVID_DecompressGetFormat( ICCVID_Info *info, LPBITMAPINFO in, LPBITMAPINFO out )
{
    DWORD size;

    TRACE("ICM_DECOMPRESS_GETFORMAT %p %p %p\n", info, in, out);

    if( (info==NULL) || (info->dwMagic!=ICCVID_MAGIC) )
        return ICERR_BADPARAM;

    size = in->bmiHeader.biSize;
    if (in->bmiHeader.biBitCount <= 8)
        size += in->bmiHeader.biClrUsed * sizeof(RGBQUAD);

    if( out )
    {
        memcpy( out, in, size );
        out->bmiHeader.biCompression = BI_RGB;
        out->bmiHeader.biSizeImage = in->bmiHeader.biHeight
                                   * in->bmiHeader.biWidth *4;
        return ICERR_OK;
    }
    return size;
}

LRESULT ICCVID_DecompressBegin( ICCVID_Info *info, LPBITMAPINFO in, LPBITMAPINFO out )
{
    TRACE("ICM_DECOMPRESS_BEGIN %p %p %p\n", info, in, out);

    if( (info==NULL) || (info->dwMagic!=ICCVID_MAGIC) )
        return ICERR_BADPARAM;

    if( info->cvinfo )
        free_cvinfo( info->cvinfo );
    info->cvinfo = decode_cinepak_init();

    return ICERR_OK;
}

LRESULT ICCVID_Decompress( ICCVID_Info *info, ICDECOMPRESS *icd, DWORD size )
{
    LONG width, height;
    WORD bit_per_pixel;

    TRACE("ICM_DECOMPRESS %p %p %ld\n", info, icd, size);

    if( (info==NULL) || (info->dwMagic!=ICCVID_MAGIC) )
        return ICERR_BADPARAM;

    width  = icd->lpbiInput->biWidth;
    height = icd->lpbiInput->biHeight;
    bit_per_pixel = icd->lpbiInput->biBitCount;

    decode_cinepak(info->cvinfo, icd->lpInput, icd->lpbiInput->biSizeImage,
                   icd->lpOutput, width, height, bit_per_pixel);

    return ICERR_OK;
}

LRESULT ICCVID_DecompressEx( ICCVID_Info *info, ICDECOMPRESSEX *icd, DWORD size )
{
    LONG width, height;
    WORD bit_per_pixel;

    TRACE("ICM_DECOMPRESSEX %p %p %ld\n", info, icd, size);

    if( (info==NULL) || (info->dwMagic!=ICCVID_MAGIC) )
        return ICERR_BADPARAM;

    /* FIXME: flags are ignored */

    width  = icd->lpbiSrc->biWidth;
    height = icd->lpbiSrc->biHeight;
    bit_per_pixel = icd->lpbiSrc->biBitCount;

    decode_cinepak(info->cvinfo, icd->lpSrc, icd->lpbiSrc->biSizeImage,
                   icd->lpDst, width, height, bit_per_pixel);

    return ICERR_OK;
}

LRESULT ICCVID_Close( ICCVID_Info *info )
{
    if( (info==NULL) || (info->dwMagic!=ICCVID_MAGIC) )
        return 0;
    if( info->cvinfo )
        free_cvinfo( info->cvinfo );
    ICCVID_Free( info );
    return 1;
}

LRESULT WINAPI ICCVID_DriverProc( DWORD dwDriverId, HDRVR hdrvr, UINT msg,
                                  LONG lParam1, LONG lParam2)
{
    ICCVID_Info *info = (ICCVID_Info *) dwDriverId;

    TRACE("%ld %p %d %ld %ld\n", dwDriverId, hdrvr, msg, lParam1, lParam2);

    switch( msg )
    {
    case DRV_LOAD:
        TRACE("Loaded\n");
        return 1;
    case DRV_ENABLE:
        return 0;
    case DRV_DISABLE:
        return 0;
    case DRV_FREE:
        return 0;

    case DRV_OPEN:
        TRACE("Opened\n");
        info = ICCVID_Alloc( sizeof (ICCVID_Info), 1 );
        if( info )
        {
            info->dwMagic = ICCVID_MAGIC;
            info->cvinfo = NULL;
        }
        return (LRESULT) info;

    case ICM_DECOMPRESS_QUERY:
        return ICCVID_DecompressQuery( info, (LPBITMAPINFO) lParam1,
                                       (LPBITMAPINFO) lParam2 );
    case ICM_DECOMPRESS_GET_FORMAT:
        return ICCVID_DecompressGetFormat( info, (LPBITMAPINFO) lParam1,
                                       (LPBITMAPINFO) lParam2 );
    case ICM_DECOMPRESS_BEGIN:
        return ICCVID_DecompressBegin( info, (LPBITMAPINFO) lParam1,
                                       (LPBITMAPINFO) lParam2 );
    case ICM_DECOMPRESS:
        return ICCVID_Decompress( info, (ICDECOMPRESS*) lParam1,
                                  (DWORD) lParam2 );
    case ICM_DECOMPRESSEX:
        return ICCVID_DecompressEx( info, (ICDECOMPRESSEX*) lParam1, 
                                  (DWORD) lParam2 );

    case DRV_CLOSE:
        return ICCVID_Close( info );

    default:
        FIXME("Unknown message: %04x %ld %ld\n", msg, lParam1, lParam2);
    }
    return 0;
}
