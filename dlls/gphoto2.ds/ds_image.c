/*
 * Copyright 2000 Corel Corporation
 * Copyright 2006 Marcus Meissner
 * Copyright 2006 CodeWeavers, Aric Stewart
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>

#include "gphoto2_i.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

#ifdef HAVE_GPHOTO2
static void *libjpeg_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(jpeg_std_error);
MAKE_FUNCPTR(jpeg_CreateDecompress);
MAKE_FUNCPTR(jpeg_read_header);
MAKE_FUNCPTR(jpeg_start_decompress);
MAKE_FUNCPTR(jpeg_read_scanlines);
MAKE_FUNCPTR(jpeg_finish_decompress);
MAKE_FUNCPTR(jpeg_destroy_decompress);
#undef MAKE_FUNCPTR

static void *load_libjpeg(void)
{
    if((libjpeg_handle = dlopen(SONAME_LIBJPEG, RTLD_NOW)) != NULL) {

#define LOAD_FUNCPTR(f) \
    if((p##f = dlsym(libjpeg_handle, #f)) == NULL) { \
        libjpeg_handle = NULL; \
        return NULL; \
    }

        LOAD_FUNCPTR(jpeg_std_error);
        LOAD_FUNCPTR(jpeg_CreateDecompress);
        LOAD_FUNCPTR(jpeg_read_header);
        LOAD_FUNCPTR(jpeg_start_decompress);
        LOAD_FUNCPTR(jpeg_read_scanlines);
        LOAD_FUNCPTR(jpeg_finish_decompress);
        LOAD_FUNCPTR(jpeg_destroy_decompress);
#undef LOAD_FUNCPTR
    }
    return libjpeg_handle;
}


/* for the jpeg decompressor source manager. */
static void _jpeg_init_source(j_decompress_ptr cinfo) { }

static boolean _jpeg_fill_input_buffer(j_decompress_ptr cinfo) {
    ERR("(), should not get here.\n");
    return FALSE;
}

static void _jpeg_skip_input_data(j_decompress_ptr cinfo,long num_bytes) {
    TRACE("Skipping %ld bytes...\n", num_bytes);
    cinfo->src->next_input_byte += num_bytes;
    cinfo->src->bytes_in_buffer -= num_bytes;
}

static boolean _jpeg_resync_to_restart(j_decompress_ptr cinfo, int desired) {
    ERR("(desired=%d), should not get here.\n",desired);
    return FALSE;
}
static void _jpeg_term_source(j_decompress_ptr cinfo) { }
#endif

/* DG_IMAGE/DAT_CIECOLOR/MSG_GET */
TW_UINT16 GPHOTO2_CIEColorGet (pTW_IDENTITY pOrigin, 
                             TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_EXTIMAGEINFO/MSG_GET */
TW_UINT16 GPHOTO2_ExtImageInfoGet (pTW_IDENTITY pOrigin, 
                                 TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_GRAYRESPONSE/MSG_RESET */
TW_UINT16 GPHOTO2_GrayResponseReset (pTW_IDENTITY pOrigin, 
                                   TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_GRAYRESPONSE/MSG_SET */
TW_UINT16 GPHOTO2_GrayResponseSet (pTW_IDENTITY pOrigin, 
                                 TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_IMAGEFILEXFER/MSG_GET */
TW_UINT16 GPHOTO2_ImageFileXferGet (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

#ifdef HAVE_GPHOTO2
static TW_UINT16 _get_image_and_startup_jpeg(void) {
    const char *folder = NULL, *filename = NULL;
    struct gphoto2_file *file;
    const unsigned char *filedata;
    unsigned long filesize;
    int ret;

    if (activeDS.file) /* Already loaded. */
	return TWRC_SUCCESS;

    if(!libjpeg_handle) {
	if(!load_libjpeg()) {
	    FIXME("Failed reading JPEG because unable to find %s\n", SONAME_LIBJPEG);
	    filedata = NULL;
	    return TWRC_FAILURE;
	}
    }

    LIST_FOR_EACH_ENTRY( file, &activeDS.files, struct gphoto2_file, entry ) {
	if (strstr(file->filename,".JPG") || strstr(file->filename,".jpg")) {
	    filename = file->filename;
	    folder = file->folder;
	    TRACE("downloading %s/%s\n", folder, filename);
	    if (file->download) {
		file->download = FALSE; /* mark as done */
		break;
	    }
	}
    }
    gp_file_new (&activeDS.file);
    ret = gp_camera_file_get(activeDS.camera, folder, filename, GP_FILE_TYPE_NORMAL,
			     activeDS.file, activeDS.context);
    if (ret < GP_OK) {
	FIXME("Failed to get file?\n");
	activeDS.twCC = TWCC_SEQERROR;
	return TWRC_FAILURE;
    }
    ret = gp_file_get_data_and_size (activeDS.file, (const char**)&filedata, &filesize);
    if (ret < GP_OK) {
	FIXME("Failed to get file data?\n");
	activeDS.twCC = TWCC_SEQERROR;
	return TWRC_FAILURE;
    }

    /* This is basically so we can use in-memory data for jpeg decompression.
     * We need to have all the functions.
     */
    activeDS.xjsm.next_input_byte	= filedata;
    activeDS.xjsm.bytes_in_buffer	= filesize;
    activeDS.xjsm.init_source	= _jpeg_init_source;
    activeDS.xjsm.fill_input_buffer	= _jpeg_fill_input_buffer;
    activeDS.xjsm.skip_input_data	= _jpeg_skip_input_data;
    activeDS.xjsm.resync_to_restart	= _jpeg_resync_to_restart;
    activeDS.xjsm.term_source	= _jpeg_term_source;

    activeDS.jd.err = pjpeg_std_error(&activeDS.jerr);
    /* jpeg_create_decompress is a macro that expands to jpeg_CreateDecompress - see jpeglib.h
     * jpeg_create_decompress(&jd); */
    pjpeg_CreateDecompress(&activeDS.jd, JPEG_LIB_VERSION, sizeof(struct jpeg_decompress_struct));
    activeDS.jd.src = &activeDS.xjsm;
    ret=pjpeg_read_header(&activeDS.jd,TRUE);
    activeDS.jd.out_color_space = JCS_RGB;
    pjpeg_start_decompress(&activeDS.jd);
    if (ret != JPEG_HEADER_OK) {
	ERR("Jpeg image in stream has bad format, read header returned %d.\n",ret);
	gp_file_unref (activeDS.file);
	activeDS.file = NULL;
	return TWRC_FAILURE;
    }
    return TWRC_SUCCESS;
}
#endif

/* DG_IMAGE/DAT_IMAGEINFO/MSG_GET */
TW_UINT16 GPHOTO2_ImageInfoGet (pTW_IDENTITY pOrigin, 
                              TW_MEMREF pData)
{
#ifdef HAVE_GPHOTO2
    pTW_IMAGEINFO pImageInfo = (pTW_IMAGEINFO) pData;

    TRACE("DG_IMAGE/DAT_IMAGEINFO/MSG_GET\n");

    if (activeDS.currentState != 6 && activeDS.currentState != 7) {
        activeDS.twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }
    if (TWRC_SUCCESS != _get_image_and_startup_jpeg()) {
	FIXME("Failed to get an image\n");
        activeDS.twCC = TWCC_SEQERROR;
	return TWRC_FAILURE;
    }
    if (activeDS.currentState == 6)
    {
        /* return general image description information about the image about to be transferred */
        TRACE("Getting parameters\n");
    }
    TRACE("activeDS.jd.output_width = %d\n", activeDS.jd.output_width);
    TRACE("activeDS.jd.output_height = %d\n", activeDS.jd.output_height);
    pImageInfo->Compression	= TWCP_NONE;
    pImageInfo->SamplesPerPixel	= 3;
    pImageInfo->BitsPerSample[0]= 8;
    pImageInfo->BitsPerSample[1]= 8;
    pImageInfo->BitsPerSample[2]= 8;
    pImageInfo->PixelType 	= TWPT_RGB;
    pImageInfo->Planar		= FALSE; /* R-G-B is chunky! */
    pImageInfo->XResolution.Whole = -1;
    pImageInfo->XResolution.Frac = 0;
    pImageInfo->YResolution.Whole = -1;
    pImageInfo->YResolution.Frac = 0;
    pImageInfo->ImageWidth 	= activeDS.jd.output_width;
    pImageInfo->ImageLength 	= activeDS.jd.output_height;
    pImageInfo->BitsPerPixel	= 24;
    return TWRC_SUCCESS;
#else
    return TWRC_FAILURE;
#endif
}

/* DG_IMAGE/DAT_IMAGELAYOUT/MSG_GET */
TW_UINT16 GPHOTO2_ImageLayoutGet (pTW_IDENTITY pOrigin, 
                                TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_IMAGELAYOUT/MSG_GETDEFAULT */
TW_UINT16 GPHOTO2_ImageLayoutGetDefault (pTW_IDENTITY pOrigin, 
                                       TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_IMAGELAYOUT/MSG_RESET */
TW_UINT16 GPHOTO2_ImageLayoutReset (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_IMAGELAYOUT/MSG_SET */
TW_UINT16 GPHOTO2_ImageLayoutSet (pTW_IDENTITY pOrigin, 
                                TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_IMAGEMEMXFER/MSG_GET */
TW_UINT16 GPHOTO2_ImageMemXferGet (pTW_IDENTITY pOrigin, 
                                 TW_MEMREF pData)
{
#ifdef HAVE_GPHOTO2
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_IMAGEMEMXFER pImageMemXfer = (pTW_IMAGEMEMXFER) pData;
    LPBYTE buffer;
    int readrows;
    unsigned int curoff;

    TRACE ("DG_IMAGE/DAT_IMAGEMEMXFER/MSG_GET\n");
    if (activeDS.currentState < 6 || activeDS.currentState > 7) {
        activeDS.twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }
    TRACE("pImageMemXfer.Compression is %d\n", pImageMemXfer->Compression);
    if (activeDS.currentState == 6) {
	if (TWRC_SUCCESS != _get_image_and_startup_jpeg()) {
	    FIXME("Failed to get an image\n");
	    activeDS.twCC = TWCC_SEQERROR;
	    return TWRC_FAILURE;
	}

    if (!activeDS.progressWnd)
        activeDS.progressWnd = TransferringDialogBox(NULL,0);
    TransferringDialogBox(activeDS.progressWnd,0);

        activeDS.currentState = 7;
    } else {
	if (!activeDS.file) {
    	    activeDS.twCC = TWRC_SUCCESS;
	    return TWRC_XFERDONE;
	}
    }

    if (pImageMemXfer->Memory.Flags & TWMF_HANDLE) {
	FIXME("Memory Handle, may not be locked correctly\n");
	buffer = LocalLock(pImageMemXfer->Memory.TheMem);
    } else
	buffer = pImageMemXfer->Memory.TheMem;
   
    memset(buffer,0,pImageMemXfer->Memory.Length);
    curoff = 0; readrows = 0;
    pImageMemXfer->YOffset	= activeDS.jd.output_scanline;
    pImageMemXfer->XOffset	= 0;	/* we do whole strips */
    while ((activeDS.jd.output_scanline<activeDS.jd.output_height) &&
	   ((pImageMemXfer->Memory.Length - curoff) > activeDS.jd.output_width*activeDS.jd.output_components)
    ) {
	JSAMPROW row = buffer+curoff;
	int x = pjpeg_read_scanlines(&activeDS.jd,&row,1);
	if (x != 1) {
		FIXME("failed to read current scanline?\n");
		break;
	}
	readrows++;
	curoff += activeDS.jd.output_width*activeDS.jd.output_components;
    }
    pImageMemXfer->Compression	= TWCP_NONE;
    pImageMemXfer->BytesPerRow	= activeDS.jd.output_components * activeDS.jd.output_width;
    pImageMemXfer->Rows		= readrows;
    pImageMemXfer->Columns	= activeDS.jd.output_width; /* we do whole strips */
    pImageMemXfer->BytesWritten = curoff;

    TransferringDialogBox(activeDS.progressWnd,0);

    if (activeDS.jd.output_scanline == activeDS.jd.output_height) {
        pjpeg_finish_decompress(&activeDS.jd);
        pjpeg_destroy_decompress(&activeDS.jd);
	gp_file_unref (activeDS.file);
	activeDS.file = NULL;
	TRACE("xfer is done!\n");

	/*TransferringDialogBox(activeDS.progressWnd, -1);*/
	twRC = TWRC_XFERDONE;
    }
    activeDS.twCC = TWRC_SUCCESS;
    if (pImageMemXfer->Memory.Flags & TWMF_HANDLE)
        LocalUnlock(pImageMemXfer->Memory.TheMem);
    return twRC;
#else
    return TWRC_FAILURE;
#endif
}

/* DG_IMAGE/DAT_IMAGENATIVEXFER/MSG_GET */
TW_UINT16 GPHOTO2_ImageNativeXferGet (pTW_IDENTITY pOrigin, 
                                    TW_MEMREF pData)
{
#ifdef HAVE_GPHOTO2
    pTW_UINT32 pHandle = (pTW_UINT32) pData;
    HBITMAP hDIB;
    BITMAPINFO bmpInfo;
    LPBYTE bits;
    JSAMPROW samprow, oldsamprow;

    FIXME("DG_IMAGE/DAT_IMAGENATIVEXFER/MSG_GET: implemented, but expect program crash due to DIB.\n");

/*  NOTE NOTE NOTE NOTE NOTE NOTE NOTE
 *
 *  While this is a mandatory transfer mode and this function
 *  is correctly implemented and fully works, the calling program
 *  will likely crash after calling.
 *
 *  Reason is that there is a lot of example code that does:
 *  bmpinfo = GlobalLock(hBITMAP); ... pointer access to bmpinfo
 *
 *  Our current HBITMAP handles do not support getting GlobalLocked -> App Crash
 *
 *  This needs a GDI Handle rewrite, at least for DIB sections.
 *  - Marcus
 */
    if (activeDS.currentState != 6) {
        activeDS.twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }
    if (TWRC_SUCCESS != _get_image_and_startup_jpeg()) {
	FIXME("Failed to get an image\n");
	activeDS.twCC = TWCC_OPERATIONERROR;
	return TWRC_FAILURE;
    }
    TRACE("Acquiring image %dx%dx%d bits from gphoto.\n",
	activeDS.jd.output_width, activeDS.jd.output_height,
	activeDS.jd.output_components*8);
    ZeroMemory (&bmpInfo, sizeof (BITMAPINFO));
    bmpInfo.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
    bmpInfo.bmiHeader.biWidth = activeDS.jd.output_width;
    bmpInfo.bmiHeader.biHeight = -activeDS.jd.output_height;
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = activeDS.jd.output_components*8;
    bmpInfo.bmiHeader.biCompression = BI_RGB;
    bmpInfo.bmiHeader.biSizeImage = 0;
    bmpInfo.bmiHeader.biXPelsPerMeter = 0;
    bmpInfo.bmiHeader.biYPelsPerMeter = 0;
    bmpInfo.bmiHeader.biClrUsed = 0;
    bmpInfo.bmiHeader.biClrImportant = 0;
    hDIB = CreateDIBSection (0, &bmpInfo, DIB_RGB_COLORS, (LPVOID)&bits, 0, 0);
    if (!hDIB) {
	FIXME("Failed creating DIB.\n");
	gp_file_unref (activeDS.file);
	activeDS.file = NULL;
	activeDS.twCC = TWCC_LOWMEMORY;
	return TWRC_FAILURE;
    }
    samprow = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,activeDS.jd.output_width*activeDS.jd.output_components);
    oldsamprow = samprow;
    while ( activeDS.jd.output_scanline<activeDS.jd.output_height ) {
        unsigned int i;
        int x = pjpeg_read_scanlines(&activeDS.jd,&samprow,1);
	if (x != 1) {
		FIXME("failed to read current scanline?\n");
		break;
	}
	/* We have to convert from RGB to BGR, see MSDN/ BITMAPINFOHEADER */
	for(i=0;i<activeDS.jd.output_width;i++,samprow+=activeDS.jd.output_components) {
	    *(bits++) = *(samprow+2);
	    *(bits++) = *(samprow+1);
	    *(bits++) = *(samprow);
	}
	bits = (LPBYTE)(((UINT_PTR)bits + 3) & ~3);
	samprow = oldsamprow;
    }
    HeapFree (GetProcessHeap(), 0, samprow);
    gp_file_unref (activeDS.file);
    activeDS.file = NULL;
    *pHandle = (UINT_PTR)hDIB;
    activeDS.twCC = TWCC_SUCCESS;
    activeDS.currentState = 7;
    return TWRC_XFERDONE;
#else
    return TWRC_FAILURE;
#endif
}

/* DG_IMAGE/DAT_JPEGCOMPRESSION/MSG_GET */
TW_UINT16 GPHOTO2_JPEGCompressionGet (pTW_IDENTITY pOrigin, 
                                    TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_JPEGCOMPRESSION/MSG_GETDEFAULT */
TW_UINT16 GPHOTO2_JPEGCompressionGetDefault (pTW_IDENTITY pOrigin,
                                           
                                           TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_JPEGCOMPRESSION/MSG_RESET */
TW_UINT16 GPHOTO2_JPEGCompressionReset (pTW_IDENTITY pOrigin, 
                                      TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_JPEGCOMPRESSION/MSG_SET */
TW_UINT16 GPHOTO2_JPEGCompressionSet (pTW_IDENTITY pOrigin, 
                                    TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_PALETTE8/MSG_GET */
TW_UINT16 GPHOTO2_Palette8Get (pTW_IDENTITY pOrigin, 
                             TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_PALETTE8/MSG_GETDEFAULT */
TW_UINT16 GPHOTO2_Palette8GetDefault (pTW_IDENTITY pOrigin, 
                                    TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_PALETTE8/MSG_RESET */
TW_UINT16 GPHOTO2_Palette8Reset (pTW_IDENTITY pOrigin, 
                               TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_PALETTE8/MSG_SET */
TW_UINT16 GPHOTO2_Palette8Set (pTW_IDENTITY pOrigin, 
                             TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_RGBRESPONSE/MSG_RESET */
TW_UINT16 GPHOTO2_RGBResponseReset (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_RGBRESPONSE/MSG_SET */
TW_UINT16 GPHOTO2_RGBResponseSet (pTW_IDENTITY pOrigin, 
                                TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

#ifdef HAVE_GPHOTO2
TW_UINT16
_get_gphoto2_file_as_DIB(
    const char *folder, const char *filename, CameraFileType type,
    HWND hwnd, HBITMAP *hDIB
) {
    const unsigned char *filedata;
    unsigned long	filesize;
    int			ret;
    CameraFile		*file;
    struct jpeg_source_mgr		xjsm;
    struct jpeg_decompress_struct	jd;
    struct jpeg_error_mgr		jerr;
    BITMAPINFO 		bmpInfo;
    LPBYTE		bits;
    JSAMPROW		samprow, oldsamprow;

    if(!libjpeg_handle) {
	if(!load_libjpeg()) {
	    FIXME("Failed reading JPEG because unable to find %s\n", SONAME_LIBJPEG);
	    filedata = NULL;
	    return TWRC_FAILURE;
	}
    }

    gp_file_new (&file);
    ret = gp_camera_file_get(activeDS.camera, folder, filename, type, file, activeDS.context);
    if (ret < GP_OK) {
	FIXME("Failed to get file?\n");
	gp_file_unref (file);
	return TWRC_FAILURE;
    }
    ret = gp_file_get_data_and_size (file, (const char**)&filedata, &filesize);
    if (ret < GP_OK) {
	FIXME("Failed to get file data?\n");
	return TWRC_FAILURE;
    }

    /* FIXME: Actually we might get other types than JPEG ... But only handle JPEG for now */
    if (filedata[0] != 0xff) {
	ERR("File %s/%s might not be JPEG, cannot decode!\n", folder, filename);
    }

    /* This is basically so we can use in-memory data for jpeg decompression.
     * We need to have all the functions.
     */
    xjsm.next_input_byte	= filedata;
    xjsm.bytes_in_buffer	= filesize;
    xjsm.init_source	= _jpeg_init_source;
    xjsm.fill_input_buffer	= _jpeg_fill_input_buffer;
    xjsm.skip_input_data	= _jpeg_skip_input_data;
    xjsm.resync_to_restart	= _jpeg_resync_to_restart;
    xjsm.term_source	= _jpeg_term_source;

    jd.err = pjpeg_std_error(&jerr);
    /* jpeg_create_decompress is a macro that expands to jpeg_CreateDecompress - see jpeglib.h
     * jpeg_create_decompress(&jd); */
    pjpeg_CreateDecompress(&jd, JPEG_LIB_VERSION, sizeof(struct jpeg_decompress_struct));
    jd.src = &xjsm;
    ret=pjpeg_read_header(&jd,TRUE);
    jd.out_color_space = JCS_RGB;
    pjpeg_start_decompress(&jd);
    if (ret != JPEG_HEADER_OK) {
	ERR("Jpeg image in stream has bad format, read header returned %d.\n",ret);
	gp_file_unref (file);
	return TWRC_FAILURE;
    }

    ZeroMemory (&bmpInfo, sizeof (BITMAPINFO));
    bmpInfo.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
    bmpInfo.bmiHeader.biWidth = jd.output_width;
    bmpInfo.bmiHeader.biHeight = -jd.output_height;
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = jd.output_components*8;
    bmpInfo.bmiHeader.biCompression = BI_RGB;
    bmpInfo.bmiHeader.biSizeImage = 0;
    bmpInfo.bmiHeader.biXPelsPerMeter = 0;
    bmpInfo.bmiHeader.biYPelsPerMeter = 0;
    bmpInfo.bmiHeader.biClrUsed = 0;
    bmpInfo.bmiHeader.biClrImportant = 0;
    *hDIB = CreateDIBSection(0, &bmpInfo, DIB_RGB_COLORS, (LPVOID)&bits, 0, 0);
    if (!*hDIB) {
	FIXME("Failed creating DIB.\n");
	gp_file_unref (file);
	return TWRC_FAILURE;
    }
    samprow = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,jd.output_width*jd.output_components);
    oldsamprow = samprow;
    while ( jd.output_scanline<jd.output_height ) {
        unsigned int i;
        int x = pjpeg_read_scanlines(&jd,&samprow,1);
	if (x != 1) {
	    FIXME("failed to read current scanline?\n");
	    break;
	}
	/* We have to convert from RGB to BGR, see MSDN/ BITMAPINFOHEADER */
	for(i=0;i<jd.output_width;i++,samprow+=jd.output_components) {
	    *(bits++) = *(samprow+2);
	    *(bits++) = *(samprow+1);
	    *(bits++) = *(samprow);
	}
	bits = (LPBYTE)(((UINT_PTR)bits + 3) & ~3);
	samprow = oldsamprow;
    }
    HeapFree (GetProcessHeap(), 0, samprow);
    gp_file_unref (file);
    return TWRC_SUCCESS;
}
#endif
