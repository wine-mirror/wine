/*
 * A simple wrapper of JPEG decoder.
 *
 * Copyright (C) Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ijgdec.h"

#if defined(HAVE_LIBJPEG) && defined(HAVE_JPEGLIB_H)

#include <jpeglib.h>
#include <jerror.h>
#include <setjmp.h>

typedef struct IJGSrcImpl IJGSrcImpl;
typedef struct IJGErrImpl IJGErrImpl;
struct IJGSrcImpl
{
	struct jpeg_source_mgr	pub;	/* must be first */

	const char** ppsrcs;
	const int* plenofsrcs;
	int srccount;
	int srcindex;
};

struct IJGErrImpl
{
	struct jpeg_error_mgr err;

	jmp_buf	env;
};


/* for the jpeg decompressor source manager. */
static void IJGDec_init_source(j_decompress_ptr cinfo) {}

static boolean IJGDec_fill_input_buffer(j_decompress_ptr cinfo)
{
	IJGSrcImpl* pImpl = (IJGSrcImpl*)cinfo->src;

	if ( pImpl->srcindex >= pImpl->srccount )
	{
		ERREXIT(cinfo, JERR_INPUT_EMPTY);
	}
	pImpl->pub.next_input_byte = pImpl->ppsrcs[pImpl->srcindex];
	pImpl->pub.bytes_in_buffer = pImpl->plenofsrcs[pImpl->srcindex];
	pImpl->srcindex ++;

	return TRUE;
}

static void IJGDec_skip_input_data(j_decompress_ptr cinfo,long num_bytes)
{
	IJGSrcImpl* pImpl = (IJGSrcImpl*)cinfo->src;

	if ( num_bytes <= 0 ) return;

	while ( num_bytes > pImpl->pub.bytes_in_buffer )
	{
		num_bytes -= pImpl->pub.bytes_in_buffer;
		if ( !IJGDec_fill_input_buffer(cinfo) )
		{
			ERREXIT(cinfo, JERR_INPUT_EMPTY);
		}
	}

	pImpl->pub.next_input_byte += num_bytes;
	pImpl->pub.bytes_in_buffer -= num_bytes;
}

static void IJGDec_term_source(j_decompress_ptr cinfo)
{
}

static void IJGDec_error_exit(j_common_ptr cinfo)
{
	IJGErrImpl* pImpl = (IJGErrImpl*)cinfo->err;

	longjmp(pImpl->env,1);
}

static void rgb_to_bgr(char* pdata,int width)
{
	int x;
	char c;

	for(x=0;x<width;x++)
	{
		c = pdata[0];
		pdata[0] = pdata[2];
		pdata[2] = c;
		pdata += 3;
	}
}

int IJGDEC_Decode( char* pdst, int dstpitch, int dstwidth, int dstheight, int dstbpp, const char** ppsrcs, const int* plenofsrcs, int srccount )
{
	IJGSrcImpl jsrc;
	IJGErrImpl jerr;
	struct jpeg_decompress_struct jdec;
	int ret = -1;

	jsrc.ppsrcs = ppsrcs;
	jsrc.plenofsrcs = plenofsrcs;
	jsrc.srccount = srccount;
	jsrc.srcindex = 0;
	jsrc.pub.bytes_in_buffer = 0;
	jsrc.pub.next_input_byte = NULL;
	jsrc.pub.init_source = IJGDec_init_source;
	jsrc.pub.fill_input_buffer = IJGDec_fill_input_buffer;
	jsrc.pub.skip_input_data = IJGDec_skip_input_data;
	jsrc.pub.resync_to_restart = jpeg_resync_to_restart;
	jsrc.pub.term_source = IJGDec_term_source;

	jdec.err = jpeg_std_error(&jerr.err);
	jerr.err.error_exit = IJGDec_error_exit;

	if ( setjmp(jerr.env) != 0 )
	{
		jpeg_destroy_decompress(&jdec);
		return -1;
	}

	jpeg_create_decompress(&jdec);
	jdec.src = &jsrc;

	ret = jpeg_read_header(&jdec,TRUE);
	if ( ret != JPEG_HEADER_OK ) goto err;

	jpeg_start_decompress(&jdec);

	if ( jdec.output_width != dstwidth ||
		 jdec.output_height != dstheight ||
		 (jdec.output_components*8) != dstbpp ) goto err;

	while (jdec.output_scanline < jdec.output_height)
	{
		jpeg_read_scanlines(&jdec,(JSAMPLE**)&pdst,1);
		rgb_to_bgr(pdst,dstwidth);
		pdst += dstpitch;
	}

	jpeg_finish_decompress(&jdec);
	ret = 0;
err:
	jpeg_destroy_decompress(&jdec);

	return ret;
}

#else

int IJGDEC_Decode( char* pdst, int dstpitch, int dstwidth, int dstheight, int dstbpp, const char** ppsrcs, const int* plenofsrcs, int srccount )
{
	return -1;
}

#endif


