/*
	lfs_wrap: Wrapper code for reader functions, both internal and external.

	copyright 2010-2023 by the mpg123 project
	free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org

	initially written by Thomas Orgis, thanks to Guido Draheim for consulting
	and later manx for stirring it all up again

	This file used to wrap 32 bit off_t client API to 64 bit off_t API. Now it is
	mapping any user API with either off_t to portable int64_t API, while also
	providing the guts of that also via a wrapper handle. This keeps the actual
	read and seek implementation out of readers.c.

	See doc/LARGEFILE for the big picture.

	Note about file descriptors: We just assume that they are generally
	interchangeable between large and small file code... and that a large file
	descriptor will trigger errors when accessed with small file code where it
	may cause trouble (a really large file).
*/

#include "config.h"

// Only activate the explicit largefile stuff here. The rest of the code shall
// work with abstract 64 bit offsets, or just plain default off_t (possibly
// using _FILE_OFFSET_BITS magic).
// Note that this macro does not influence normal off_t-using code.
#ifdef LFS_LARGEFILE_64
#define _LARGEFILE64_SOURCE
#endif

// For correct MPG123_EXPORT.
#include "../common/abi_align.h"

// Need the full header with non-portable API, for the bare mpg123_open*()
// declarations. But no renaming shenanigans.
#define MPG123_NO_LARGENAME
#include "mpg123.h"

#include "lfs_wrap.h"
#include "../common/abi_align.h"
#include "../compat/compat.h"
#include <sys/stat.h>
#include <fcntl.h>

#ifndef OFF_MAX
#undef OFF_MIN
#if SIZEOF_OFF_T == 4
#define OFF_MAX INT32_MAX
#define OFF_MIN INT32_MIN
#elif SIZEOF_OFF_T == 8
#define OFF_MAX INT64_MAX
#define OFF_MIN INT64_MIN
#else
#error "Unexpected width of off_t."
#endif
#endif

// A paranoid check that someone did not define a wrong SIZEOF_OFF_T at configure time.
typedef unsigned char MPG123_STATIC_ASSERT[(SIZEOF_OFF_T == sizeof(off_t)) ? 1 : -1];

#include "../common/debug.h"

// We do not want to expose this publicly, but it is cleaner to have it also defined
// as portable API to offer the legacy function wrapper over. It's an undocumented
// function for int64_t arguments.
int attribute_align_arg mpg123_position64( mpg123_handle *fr, int64_t no, int64_t buffsize
,	int64_t  *current_frame,   int64_t  *frames_left
,	double *current_seconds, double *seconds_left );

/* I see that I will need custom data storage. Main use is for the replaced I/O later, but the seek table for small file offsets needs extra storage, too. */

// The wrapper handle for descriptor and handle I/O.
// It is also used for storing a converted frame index.
// Plain portable API I/O should not need it at all.
#define IO_HANDLE64  0 /* no wrapping at all: client-provided callbacks */
#define IO_FD        1 /* default off_t callbacks */
#define IO_HANDLE    2 /* Wrapping over custom handle callbacks with off_t. */
#ifdef LFS_LARGEFILE_64
#define IO_FD_64     3 /* off64_t callbacks */
#define IO_HANDLE_64 4 /* ... with off64_t. */
#endif
#define IO_INT_FD    5 /* Internal I/O using a file descriptor and wrappers defined here. */

struct wrap_data
{
	/* Storage for small offset index table. */
#if SIZEOF_OFF_T == 4
	off_t *indextable;
	// And ironically, another big offset table for mpg123_set_index_32.
	// I wand to avoid having to change a line of code in the internals.
	int64_t *set_indextable;
#endif
	/* I/O handle stuff */
	int iotype; // one of the above numbers
	/* Data for IO_FD variants. */
	int fd;
	int my_fd; /* A descriptor that the wrapper code opened itself. */
#ifdef TIMEOUT_READ
	time_t timeout_sec;
#endif
	void* handle; // for IO_HANDLE variants
	/* The actual callbacks from the outside. */
	mpg123_ssize_t (*r_read) (int, void *, size_t);
	off_t (*r_lseek)(int, off_t, int);
	mpg123_ssize_t (*r_h_read)(void *, void *, size_t);
	off_t (*r_h_lseek)(void*, off_t, int);
#ifdef LFS_LARGEFILE_64
	mpg123_ssize_t (*r_read_64) (int, void *, size_t);
	off64_t (*r_lseek_64)(int, off64_t, int);
	mpg123_ssize_t (*r_h_read_64)(void *, void *, size_t);
	off64_t (*r_h_lseek_64)(void*, off64_t, int);
#endif
	void (*h_cleanup)(void*);
};

/* Cleanup I/O part of the handle handle... but not deleting the wrapper handle itself.
   That is stored in the frame and only deleted on mpg123_delete(). */
static void wrap_io_cleanup(void *handle)
{
	struct wrap_data *ioh = handle;
	debug("wrapper I/O cleanup");
	if(ioh->iotype == IO_HANDLE
#ifdef LFS_LARGEFILE_64
	|| ioh->iotype == IO_HANDLE_64
#endif
	){
		if(ioh->h_cleanup != NULL && ioh->handle != NULL)
		{
			mdebug("calling client handle cleanup %p", ioh->handle);
			ioh->h_cleanup(ioh->handle);
		}
		ioh->handle = NULL;
	}
	if(ioh->my_fd >= 0)
	{
		mdebug("closing my fd %d", ioh->my_fd);
#if defined(MPG123_COMPAT_MSVCRT_IO)
		_close(ioh->my_fd);
#else
		close(ioh->my_fd);
#endif
		ioh->my_fd = -1;
	}
}

/* Really finish off the handle... freeing all memory. */
void INT123_wrap_destroy(void *handle)
{
	struct wrap_data *wh = handle;
	if(!wh)
		return;
	wrap_io_cleanup(handle);
#if SIZEOF_OFF_T == 4
	if(wh->indextable != NULL)
		free(wh->indextable);
	if(wh->set_indextable != NULL)
		free(wh->set_indextable);
#endif

	free(wh);
}

/* More helper code... extract the special wrapper handle, possible allocate and initialize it. */
static struct wrap_data* wrap_get(mpg123_handle *mh, int force_alloc)
{
	struct wrap_data* whd;
	void ** whd_ = INT123_wrap_handle(mh);

	if(whd_ == NULL)
		return NULL;

	/* Access the private storage inside the mpg123 handle.
	   The real callback functions and handles are stored there. */
	if(*whd_ == NULL && force_alloc)
	{
		/* Create a new one. */
		*whd_ = malloc(sizeof(struct wrap_data));
		if(*whd_ == NULL)
		{
			INT123_set_err(mh, MPG123_OUT_OF_MEM);
			return NULL;
		}
		whd = *whd_;
#if SIZEOF_OFF_T == 4
		whd->indextable = NULL;
		whd->set_indextable = NULL;
#endif
		whd->iotype = IO_HANDLE64; // By default, the I/O path is not affected.
		whd->fd = -1;
		whd->my_fd = -1;
		whd->handle = NULL;
		whd->r_read = NULL;
		whd->r_lseek = NULL;
		whd->r_h_read = NULL;
		whd->r_h_lseek = NULL;
#ifdef LFS_LARGEFILE_64
		whd->r_read_64 = NULL;
		whd->r_lseek_64 = NULL;
		whd->r_h_read_64 = NULL;
		whd->r_h_lseek_64 = NULL;
#endif
		whd->h_cleanup = NULL;
	}
	else whd = *whd_;

	return whd;
}

/* After settling the data... start with some simple wrappers. */

// The first block of wrappers is always present, using the native off_t width.
// (Exception: If explicitly disabled using FORCED_OFF_64.)
// A second block mirrors that in case of sizeof(off_t)==4 with _32 suffix.
// A third block follows if 64 bit off_t is available with _64 suffix, just aliasing
// the int64_t functions.

#ifndef FORCED_OFF_64

#define OFF_CONV(value, variable, handle) \
  if((value) >= OFF_MIN && (value) <= OFF_MAX) \
     variable = (off_t)(value); \
  else return INT123_set_err(handle, MPG123_LFS_OVERFLOW);

#define OFF_CONVP(value, varpointer, handle) \
  if(varpointer){ OFF_CONV(value, *(varpointer), handle) }

#define OFF_RETURN(value, handle) \
  return ((value) >= OFF_MIN && (value) <= OFF_MAX) \
  ?  (off_t)(value) \
  :  INT123_set_err(handle, MPG123_LFS_OVERFLOW);

int attribute_align_arg mpg123_framebyframe_decode(mpg123_handle *mh, off_t *num, unsigned char **audio, size_t *bytes)
{
	int64_t fnum = 0;
	int ret = mpg123_framebyframe_decode64(mh, &fnum, audio, bytes);
	OFF_CONVP(fnum, num, mh)
	return ret;
}

int attribute_align_arg mpg123_decode_frame(mpg123_handle *mh, off_t *num, unsigned char **audio, size_t *bytes)
{
	int64_t fnum = 0;
	int ret = mpg123_decode_frame64(mh, &fnum, audio, bytes);
	OFF_CONVP(fnum, num, mh)
	return ret;
}

off_t attribute_align_arg mpg123_timeframe(mpg123_handle *mh, double seconds)
{
	int64_t b = mpg123_timeframe64(mh, seconds);
	OFF_RETURN(b, mh)
}

off_t attribute_align_arg mpg123_tell(mpg123_handle *mh)
{
	int64_t pos = mpg123_tell64(mh);
	OFF_RETURN(pos, mh)
}

off_t attribute_align_arg mpg123_tellframe(mpg123_handle *mh)
{
	int64_t frame = mpg123_tellframe64(mh);
	OFF_RETURN(frame, mh)
}

off_t attribute_align_arg mpg123_tell_stream(mpg123_handle *mh)
{
	int64_t off = mpg123_tell_stream64(mh);
	OFF_RETURN(off, mh)
}

off_t attribute_align_arg mpg123_seek(mpg123_handle *mh, off_t sampleoff, int whence)
{
	int64_t ret = mpg123_seek64(mh, (int64_t)sampleoff, whence);
	OFF_RETURN(ret, mh)
}

off_t attribute_align_arg mpg123_feedseek(mpg123_handle *mh, off_t sampleoff, int whence, off_t *input_offset)
{
	int64_t inoff = 0;
	int64_t ret = mpg123_feedseek64(mh, (int64_t)sampleoff, whence, &inoff);
	OFF_CONVP(inoff, input_offset, mh)
	OFF_RETURN(ret, mh)
}

off_t attribute_align_arg mpg123_seek_frame(mpg123_handle *mh, off_t offset, int whence)
{
	int64_t ret = mpg123_seek_frame64(mh, (int64_t)offset, whence);
	OFF_RETURN(ret, mh)
}

int attribute_align_arg mpg123_set_filesize(mpg123_handle *mh, off_t size)
{
	return mpg123_set_filesize64(mh, (int64_t)size);
}

off_t attribute_align_arg mpg123_framelength(mpg123_handle *mh)
{
	int64_t ret = mpg123_framelength64(mh);
	OFF_RETURN(ret, mh)
}

off_t attribute_align_arg mpg123_length(mpg123_handle *mh)
{
	int64_t ret = mpg123_length64(mh);
	OFF_RETURN(ret, mh)
}

// Native off_t is either identical to int32_t or int64_t.
// If the former, we create a copy of the index table.
int attribute_align_arg mpg123_index(mpg123_handle *mh, off_t **offsets, off_t *step, size_t *fill)
{
#if SIZEOF_OFF_T == 8
	return mpg123_index64(mh, (int64_t**)offsets, (int64_t*)step, fill);
#else
	int err;
	int64_t largestep;
	int64_t *largeoffsets;
	struct wrap_data *whd;
	if(mh == NULL)
		return MPG123_BAD_HANDLE;
	if(offsets == NULL || step == NULL || fill == NULL)
		return INT123_set_err(mh, MPG123_BAD_INDEX_PAR);
	*fill = 0; // better safe than sorry

	whd = wrap_get(mh, 1);
	if(whd == NULL) return MPG123_ERR;

	err = mpg123_index64(mh, &largeoffsets, &largestep, fill);
	if(err != MPG123_OK) return err;

	/* For a _very_ large file, even the step could overflow. */
	OFF_CONV(largestep, *step, mh);

	/* When there are no values stored, there is no table content to take care of.
	   Table pointer does not matter. Mission completed. */
	if(*fill == 0) return MPG123_OK;

	/* Construct a copy of the index to hand over to the small-minded client. */
	*offsets = INT123_safe_realloc(whd->indextable, (*fill)*sizeof(int32_t));
	if(*offsets == NULL)
		return INT123_set_err(mh, MPG123_OUT_OF_MEM);
	whd->indextable = *offsets;
	/* Elaborate conversion of each index value, with overflow check. */
	for(size_t i=0; i<*fill; ++i)
		OFF_CONV(largeoffsets[i], whd->indextable[i], mh);
	/* If we came that far... there should be a valid copy of the table now. */
	return MPG123_OK;
#endif
}

int attribute_align_arg mpg123_set_index(mpg123_handle *mh, off_t *offsets, off_t step, size_t fill)
{
#if SIZEOF_OFF_T == 8
	return mpg123_set_index64(mh, (int64_t*)offsets, (int64_t)step, fill);
#else
	int err;
	struct wrap_data *whd;
	int64_t *indextmp;
	if(mh == NULL) return MPG123_BAD_HANDLE;

	whd = wrap_get(mh, 1);
	if(whd == NULL) return MPG123_ERR;

	if(fill > 0 && offsets == NULL)
		return INT123_set_err(mh, MPG123_BAD_INDEX_PAR);
	else
	{
		/* Expensive temporary storage... for staying outside at the API layer. */
		indextmp = INT123_safe_realloc(whd->set_indextable, fill*sizeof(int64_t));
		if(indextmp == NULL)
			return INT123_set_err(mh, MPG123_OUT_OF_MEM);
		whd->set_indextable = indextmp;
		/* Fill the large-file copy of the provided index, then feed it to mpg123. */
		for(size_t i=0; i<fill; ++i)
			indextmp[i] = offsets[i];
		err = mpg123_set_index64(mh, indextmp, (int64_t)step, fill);
	}

	return err;
#endif
}

off_t attribute_align_arg mpg123_framepos(mpg123_handle *mh)
{
	int64_t pos = mpg123_framepos64(mh);
	OFF_RETURN(pos, mh)
}

int attribute_align_arg mpg123_position( mpg123_handle *mh, off_t INT123_frame_offset
,	off_t buffered_bytes, off_t *current_frame, off_t *frames_left
,	double *current_seconds, double *seconds_left )
{
	int64_t curframe, frameleft;
	int err;

	err = mpg123_position64( mh, (int64_t)INT123_frame_offset, (int64_t)buffered_bytes
	,	&curframe, &frameleft, current_seconds, seconds_left );
	if(err != MPG123_OK) return err;

	OFF_CONVP(curframe, current_frame, mh)
	OFF_CONVP(frameleft, frames_left, mh);
	return MPG123_OK;
}

#endif // FORCED_OFF_64

// _32 aliases only for native 32 bit off_t
// Will compilers be smart enough to optimize away the extra function call?
#if SIZEOF_OFF_T == 4

// The open routines are trivial now. I only have differeing symbols suffixes
// to keep legacy ABI.

int attribute_align_arg mpg123_open_32(mpg123_handle *mh, const char *path)
{
	return mpg123_open(mh, path);
}

int attribute_align_arg mpg123_open_fixed_32( mpg123_handle *mh, const char *path
,	int channels, int encoding )
{
	return mpg123_open_fixed(mh, path, channels, encoding);
}

int attribute_align_arg mpg123_open_fd_32(mpg123_handle *mh, int fd)
{
	return mpg123_open_fd(mh, fd);
}

int attribute_align_arg mpg123_open_handle_32(mpg123_handle *mh, void *iohandle)
{
	return mpg123_open_handle(mh, iohandle);
}

int attribute_align_arg mpg123_framebyframe_decode_32(mpg123_handle *mh, off_t *num, unsigned char **audio, size_t *bytes)
{
	return mpg123_framebyframe_decode(mh, num, audio, bytes);
}

int attribute_align_arg mpg123_decode_frame_32(mpg123_handle *mh, off_t *num, unsigned char **audio, size_t *bytes)
{
	return mpg123_decode_frame(mh, num, audio, bytes);
}

off_t attribute_align_arg mpg123_timeframe_32(mpg123_handle *mh, double seconds)
{
	return mpg123_timeframe64(mh, seconds);
}

off_t attribute_align_arg mpg123_tell_32(mpg123_handle *mh)
{
	return mpg123_tell(mh);
}

off_t attribute_align_arg mpg123_tellframe_32(mpg123_handle *mh)
{
	return mpg123_tellframe(mh);
}

off_t attribute_align_arg mpg123_tell_stream_32(mpg123_handle *mh)
{
	return mpg123_tell_stream(mh);
}

off_t attribute_align_arg mpg123_seek_32(mpg123_handle *mh, off_t sampleoff, int whence)
{
	return mpg123_seek(mh, sampleoff, whence);
}

off_t attribute_align_arg mpg123_feedseek_32(mpg123_handle *mh, off_t sampleoff, int whence, off_t *input_offset)
{
	return mpg123_feedseek(mh, sampleoff, whence, input_offset);
}

off_t attribute_align_arg mpg123_seek_frame_32(mpg123_handle *mh, off_t offset, int whence)
{
	return mpg123_seek_frame(mh, offset, whence);
}

int attribute_align_arg mpg123_set_filesize_32(mpg123_handle *mh, off_t size)
{
	return mpg123_set_filesize(mh, size);
}

off_t attribute_align_arg mpg123_framelength_32(mpg123_handle *mh)
{
	return mpg123_framelength(mh);
}

off_t attribute_align_arg mpg123_length_32(mpg123_handle *mh)
{
	return mpg123_length(mh);
}

int attribute_align_arg mpg123_index_32(mpg123_handle *mh, off_t **offsets, off_t *step, size_t *fill)
{
	return mpg123_index(mh, offsets, step, fill);
}

int attribute_align_arg mpg123_set_index_32(mpg123_handle *mh, off_t *offsets, off_t step, size_t fill)
{
	return mpg123_set_index(mh, offsets, step, fill);
}

off_t attribute_align_arg mpg123_framepos_32(mpg123_handle *mh)
{
	return mpg123_framepos(mh);
}

int attribute_align_arg mpg123_position_32( mpg123_handle *mh, off_t INT123_frame_offset
,	off_t buffered_bytes, off_t *current_frame, off_t *frames_left
,	double *current_seconds, double *seconds_left )
{
	return mpg123_position( mh, INT123_frame_offset, buffered_bytes
	,	current_frame, frames_left, current_seconds, seconds_left );
}

#endif

// _64 aliases if we either got some off64_t to work with or
// if there is no explicit 64 bit API but off_t is just always
// 64 bits.
#if defined(LFS_LARGEFILE_64) || (SIZEOF_OFF_T == 8)

#ifdef LFS_LARGEFILE_64
#define OFF64 off64_t
#else
#define OFF64 off_t
#endif

#ifndef FORCED_OFF_64
// When 64 bit offsets are enforced, libmpg123.c defines the _64 functions directly.
// There is no actual wrapper work, anyway.

int attribute_align_arg mpg123_open_64(mpg123_handle *mh, const char *path)
{
	return mpg123_open(mh, path);
}

int attribute_align_arg mpg123_open_fixed_64( mpg123_handle *mh, const char *path
,	int channels, int encoding )
{
	return mpg123_open_fixed(mh, path, channels, encoding);
}

int attribute_align_arg mpg123_open_fd_64(mpg123_handle *mh, int fd)
{
	return mpg123_open_fd(mh, fd);
}

int attribute_align_arg mpg123_open_handle_64(mpg123_handle *mh, void *iohandle)
{
	return mpg123_open_handle(mh, iohandle);
}
#endif


int attribute_align_arg mpg123_framebyframe_decode_64(mpg123_handle *mh, OFF64 *num, unsigned char **audio, size_t *bytes)
{
	return mpg123_framebyframe_decode64(mh, (int64_t*)num, audio, bytes);
}

int attribute_align_arg mpg123_decode_frame_64(mpg123_handle *mh, OFF64 *num, unsigned char **audio, size_t *bytes)
{
	return mpg123_decode_frame64(mh, (int64_t*)num, audio, bytes);
}

OFF64 attribute_align_arg mpg123_timeframe_64(mpg123_handle *mh, double seconds)
{
	return mpg123_timeframe64(mh, seconds);
}

OFF64 attribute_align_arg mpg123_tell_64(mpg123_handle *mh)
{
	return mpg123_tell64(mh);
}

OFF64 attribute_align_arg mpg123_tellframe_64(mpg123_handle *mh)
{
	return mpg123_tellframe64(mh);
}

OFF64 attribute_align_arg mpg123_tell_stream_64(mpg123_handle *mh)
{
	return mpg123_tell_stream64(mh);
}

OFF64 attribute_align_arg mpg123_seek_64(mpg123_handle *mh, OFF64 sampleoff, int whence)
{
	return mpg123_seek64(mh, (int64_t)sampleoff, whence);
}

OFF64 attribute_align_arg mpg123_feedseek_64(mpg123_handle *mh, OFF64 sampleoff, int whence, OFF64 *input_offset)
{
	return mpg123_feedseek64(mh, (int64_t)sampleoff, whence, (int64_t*)input_offset);
}

OFF64 attribute_align_arg mpg123_seek_frame_64(mpg123_handle *mh, OFF64 offset, int whence)
{
	return mpg123_seek_frame64(mh, (int64_t)offset, whence);
}

int attribute_align_arg mpg123_set_filesize_64(mpg123_handle *mh, OFF64 size)
{
	return mpg123_set_filesize64(mh, (int64_t)size);
}

OFF64 attribute_align_arg mpg123_framelength_64(mpg123_handle *mh)
{
	return mpg123_framelength64(mh);
}

OFF64 attribute_align_arg mpg123_length_64(mpg123_handle *mh)
{
	return mpg123_length64(mh);
}

int attribute_align_arg mpg123_index_64(mpg123_handle *mh, OFF64 **offsets, OFF64 *step, size_t *fill)
{
	return mpg123_index64(mh, (int64_t**)offsets, (int64_t*)step, fill);
}

int attribute_align_arg mpg123_set_index_64(mpg123_handle *mh, OFF64 *offsets, OFF64 step, size_t fill)
{
	return mpg123_set_index64(mh, (int64_t*)offsets, (int64_t)step, fill);
}

OFF64 attribute_align_arg mpg123_framepos_64(mpg123_handle *mh)
{
	return mpg123_framepos64(mh);
}

int attribute_align_arg mpg123_position_64( mpg123_handle *mh, OFF64 INT123_frame_offset
,	OFF64 buffered_bytes, OFF64 *current_frame, OFF64 *frames_left
,	double *current_seconds, double *seconds_left )
{
	return mpg123_position64( mh, (int64_t)INT123_frame_offset, (int64_t)buffered_bytes
	,	(int64_t*)current_frame, (int64_t*)frames_left, current_seconds, seconds_left );
}

#undef OFF64
#endif

/* =========================================
             THE BOUNDARY OF SANITY
               Behold, stranger!
   ========================================= */

// One read callback wrapping over all 4 client callback variants.
static int wrap_read(void* handle, void *buf, size_t count, size_t *got)
{
	struct wrap_data *ioh = handle;
	ptrdiff_t retgot = -1;
	switch(ioh->iotype)
	{
		case IO_FD:
			retgot = ioh->r_read(ioh->fd, buf, count);
		break;
		case IO_HANDLE:
			retgot = ioh->r_h_read(ioh->handle, buf, count);
		break;
#ifdef LFS_LARGEFILE_64
		case IO_FD_64:
			retgot = ioh->r_read_64(ioh->fd, buf, count);
		break;
		case IO_HANDLE_64:
			retgot = ioh->r_h_read_64(ioh->handle, buf, count);
		break;
#endif
		default:
			error("Serious breakage - bad IO type in LFS wrapper!");
	}
	if(got)
		*got = retgot > 0 ? (size_t)retgot : 0;
	return retgot >= 0 ? 0 : -1;
}

// One seek callback wrapping over all 4 client callback variants.
static int64_t wrap_lseek(void *handle, int64_t offset, int whence)
{
	struct wrap_data *ioh = handle;

	if( (ioh->iotype == IO_FD || ioh->iotype == IO_HANDLE) &&
		(offset < OFF_MIN || offset > OFF_MAX) )
	{
		errno = EOVERFLOW;
		return -1;
	}
	switch(ioh->iotype)
	{
		case IO_FD: return ioh->r_lseek(ioh->fd, (off_t)offset, whence);
		case IO_HANDLE: return ioh->r_h_lseek(ioh->handle, (off_t)offset, whence);
#ifdef LFS_LARGEFILE_64
		case IO_FD_64: return ioh->r_lseek_64(ioh->fd, offset, whence);
		case IO_HANDLE_64: return ioh->r_h_lseek_64(ioh->handle, offset, whence);
#endif
	}
	error("Serious breakage - bad IO type in LFS wrapper!");
	return -1;
}

// Defining a wrapper to the native read to be sure the prototype matches.
// There are platforms where it is read(int, void*, unsigned int).
// We know that we read small chunks where the difference does not matter. Could
// apply specific hackery, use a common compat_read() (INT123_unintr_read()?) with system
// specifics.
static mpg123_ssize_t fallback_read(int fd, void *buf, size_t count)
{
#if defined(MPG123_COMPAT_MSVCRT_IO)
	if(count > UINT_MAX)
	{
		errno = EOVERFLOW;
		return -1;
	}
	return _read(fd, buf, (unsigned int)count);
#else
	return read(fd, buf, count);
#endif
}

static off_t fallback_lseek(int fd, off_t offset, int whence)
{
#if defined(MPG123_COMPAT_MSVCRT_IO)
	// Off_t is 32 bit and does fit into long. We know that.
	return _lseek(fd, (long)offset, whence);
#else
	return lseek(fd, offset, whence);
#endif
}

// This is assuming an internally opened file, which usually will be
// using 64 bit offsets. It keeps reading on on trivial interruptions.
// I guess any file descriptor that matches the libc should work fine.
static int internal_read64(void *handle, void *buf, size_t bytes, size_t *got_bytes)
{
	int ret = 0;
	if(!handle || (!buf && bytes))
		return -1;
	struct wrap_data* ioh = handle;
	int fd = ioh->fd;
	size_t got = 0;
	errno = 0;
	while(bytes)
	{
#ifdef TIMEOUT_READ
		if(ioh->timeout_sec)
		{
			fd_set fds;
			tv.tv_sec = ioh->timeout_sec;
			tv.tv_usec = 0;
			FD_ZERO(&fds);
			FD_SET(fd, &fds);
			int sret = select(fd+1, &fds, NULL, NULL, &tv);
			if(sret < 1)
			{
				return -1; // timeout means error
				// communicate quietness flag? if(NOQUIET) error("stream timed out");
			}
		}
#endif
		errno = 0;
		ptrdiff_t part = fallback_read(fd, (char*)buf+got, bytes);
		if(part > 0) // == 0 is end of file
		{
			SATURATE_SUB(bytes, part, 0)
			SATURATE_ADD(got, part, SIZE_MAX)
		} else if(errno != EINTR && errno != EAGAIN
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
			&& errno != EWOULDBLOCK
#endif
		){
			if(part < 0)
				ret = -1;
			break;
		}
	}
	if(got_bytes)
		*got_bytes = got;
	return ret;
}

static int64_t internal_lseek64(void *handle, int64_t offset, int whence)
{
	struct wrap_data* ioh = handle;
#ifdef LFS_LARGEFILE_64
	return lseek64(ioh->fd, offset, whence);
#elif defined(MPG123_COMPAT_MSVCRT_IO_64)
	return _lseeki64(ioh->fd, offset, whence);
#else
	if(offset < OFF_MIN || offset > OFF_MAX)
	{
		errno = EOVERFLOW;
		return -1;
	}
	return fallback_lseek(ioh->fd, (off_t)offset, whence);
#endif
}

int INT123_wrap_open(mpg123_handle *mh, void *handle, const char *path, int fd, long timeout, int quiet)
{
	int force_alloc = (path || fd >= 0) ? 1 : 0;
	struct wrap_data *ioh = wrap_get(mh, force_alloc);
	if(ioh == NULL && force_alloc)
		return MPG123_ERR;
	if(!path && fd < 0)
	{
		if(!ioh || ioh->iotype == IO_HANDLE64)
		{
			mdebug("user-supplied 64 bit I/O on user-supplied handle %p", handle);
			return LFS_WRAP_NONE;
		}
		if(ioh->iotype == IO_HANDLE)
		{
			mdebug("wrapped user handle %p", handle);
			ioh->handle = handle;
			if(ioh->r_h_read && ioh->r_h_lseek)
				return mpg123_reader64(mh, wrap_read, wrap_lseek, wrap_io_cleanup);
			return INT123_set_err(mh, MPG123_NO_READER);
		}
#ifdef LFS_LARGEFILE_64
		if(ioh->iotype == IO_HANDLE_64)
		{
			mdebug("wrapped 64 bit user handle %p", handle);
			ioh->handle = handle;
			if(ioh->r_h_read_64 && ioh->r_h_lseek_64)
				return mpg123_reader64(mh, wrap_read, wrap_lseek, wrap_io_cleanup);
			return INT123_set_err(mh, MPG123_NO_READER);
		}
#endif
	}
	if(path)
	{
		debug("opening path (providing fd)");
		// Open the resource and store the descriptor for closing later.
		int flags=O_RDONLY;
#ifdef O_BINARY
		flags |= O_BINARY;
#endif
#if defined(LFS_LARGEFILE_64) && defined(HAVE_O_LARGEFILE)
		flags |= O_LARGEFILE;
#endif
		errno = 0;
		ioh->my_fd = fd = INT123_compat_open(path, flags);
		if(fd < 0)
		{
			if(!quiet)
				error2("Cannot open file %s: %s", path, INT123_strerror(errno));
			return INT123_set_err(mh, MPG123_BAD_FILE);
		}
	}
	if(fd >= 0)
	{
		mdebug("working with given fd %d", fd);
		ioh->fd = fd;
		// Prepared I/O using external callbacks.
		if(ioh->iotype == IO_FD)
		{
				debug("native fd callbacks");
				if(ioh->r_read && ioh->r_lseek)
					return mpg123_reader64(mh, wrap_read, wrap_lseek, wrap_io_cleanup);
				return INT123_set_err(mh, MPG123_NO_READER);
		}
#ifdef LFS_LARGEFILE_64
		if(ioh->iotype == IO_FD_64)
		{
				debug("64 bit fd callbacks");
				if(ioh->r_read_64 && ioh->r_lseek_64)
					return mpg123_reader64(mh, wrap_read, wrap_lseek, wrap_io_cleanup);
				return INT123_set_err(mh, MPG123_NO_READER);
		}
		debug("internal 64 bit I/O");
#else
		debug("internal 32-behind-64 bit I/O");
#endif
		// Doing our own thing using the given/just-opened descriptor.
		ioh->iotype = IO_INT_FD;
#ifdef TIMEOUT_READ
		ioh->timeout_sec = (time_t)(timeout > 0 ? timeout : 0);
		if(ioh->timeout_sec > 0)
		{
			mdebug("timeout reader with %ld s", timeout);
			int flags;
			flags = fcntl(fd, F_GETFL);
			flags |= O_NONBLOCK;
			fcntl(fd, F_SETFL, flags);
		}
#endif
		return mpg123_reader64(mh, internal_read64, internal_lseek64, wrap_io_cleanup);
	}
	return MPG123_ERR;
}

// So, native off_t reader replacement.

// In forced 64 bit offset mode, the only definitions of these are
// the _64 ones.
#ifdef FORCED_OFF_64
#define mpg123_replace_reader mpg123_replace_reader_64
#define mpg123_replace_reader_handle mpg123_replace_reader_handle_64
#endif

/* Reader replacement prepares the hidden handle storage for next mpg123_open_fd() or plain mpg123_open(). */
int attribute_align_arg mpg123_replace_reader(mpg123_handle *mh, mpg123_ssize_t (*r_read) (int, void *, size_t), off_t (*r_lseek)(int, off_t, int) )
{
	struct wrap_data* ioh;

	if(mh == NULL) return MPG123_ERR;

	mpg123_close(mh);
	ioh = wrap_get(mh, 1);
	if(ioh == NULL) return MPG123_ERR;

	/* If both callbacks are NULL, switch totally to internal I/O, else just use fallback for at most half of them. */
	if(r_read == NULL && r_lseek == NULL)
	{
		ioh->iotype = IO_INT_FD;
		ioh->fd = -1;
		ioh->r_read = NULL;
		ioh->r_lseek = NULL;
	}
	else
	{
		ioh->iotype = IO_FD;
		ioh->fd = -1; /* On next mpg123_open_fd(), this gets a value. */
		ioh->r_read = r_read != NULL ? r_read : fallback_read;
		ioh->r_lseek = r_lseek != NULL ? r_lseek : fallback_lseek;
	}

	/* The real reader replacement will happen while opening. */
	return MPG123_OK;
}

int attribute_align_arg mpg123_replace_reader_handle(mpg123_handle *mh, mpg123_ssize_t (*r_read) (void*, void *, size_t), off_t (*r_lseek)(void*, off_t, int), void (*cleanup)(void*))
{
	struct wrap_data* ioh;

	if(mh == NULL) return MPG123_ERR;

	mpg123_close(mh);
	ioh = wrap_get(mh, 1);
	if(ioh == NULL) return MPG123_ERR;

	ioh->iotype = IO_HANDLE;
	ioh->handle = NULL;
	ioh->r_h_read = r_read;
	ioh->r_h_lseek = r_lseek;
	ioh->h_cleanup = cleanup;

	/* The real reader replacement will happen while opening. */
	return MPG123_OK;
}

#if SIZEOF_OFF_T == 4
int attribute_align_arg mpg123_replace_reader_32(mpg123_handle *mh, mpg123_ssize_t (*r_read) (int, void *, size_t), off_t (*r_lseek)(int, off_t, int) )
{
	return mpg123_replace_reader(mh, r_read, r_lseek);
}

int attribute_align_arg mpg123_replace_reader_handle_32(mpg123_handle *mh, mpg123_ssize_t (*r_read) (void*, void *, size_t), off_t (*r_lseek)(void*, off_t, int), void (*cleanup)(void*))
{
	return mpg123_replace_reader_handle(mh, r_read, r_lseek, cleanup);
}

#endif

#ifdef LFS_LARGEFILE_64

int attribute_align_arg mpg123_replace_reader_64(mpg123_handle *mh, mpg123_ssize_t (*r_read) (int, void *, size_t), off64_t (*r_lseek)(int, off64_t, int) )
{
	struct wrap_data* ioh;

	if(mh == NULL) return MPG123_ERR;

	mpg123_close(mh);
	ioh = wrap_get(mh, 1);
	if(ioh == NULL) return MPG123_ERR;

	/* If both callbacks are NULL, switch totally to internal I/O, else just use fallback for at most half of them. */
	if(r_read == NULL && r_lseek == NULL)
	{
		ioh->iotype = IO_INT_FD;
		ioh->fd = -1;
		ioh->r_read_64 = NULL;
		ioh->r_lseek_64 = NULL;
	}
	else
	{
		ioh->iotype = IO_FD_64;
		ioh->fd = -1; /* On next mpg123_open_fd(), this gets a value. */
		ioh->r_read_64 = r_read != NULL ? r_read : fallback_read;
		ioh->r_lseek_64 = r_lseek != NULL ? r_lseek : lseek64;
	}

	/* The real reader replacement will happen while opening. */
	return MPG123_OK;
}

int attribute_align_arg mpg123_replace_reader_handle_64(mpg123_handle *mh, mpg123_ssize_t (*r_read) (void*, void *, size_t), off64_t (*r_lseek)(void*, off64_t, int), void (*cleanup)(void*))
{
	struct wrap_data* ioh;

	if(mh == NULL) return MPG123_ERR;

	mpg123_close(mh);
	ioh = wrap_get(mh, 1);
	if(ioh == NULL) return MPG123_ERR;

	ioh->iotype = IO_HANDLE_64;
	ioh->handle = NULL;
	ioh->r_h_read_64 = r_read;
	ioh->r_h_lseek_64 = r_lseek;
	ioh->h_cleanup = cleanup;

	/* The real reader replacement will happen while opening. */
	return MPG123_OK;
}

#elif SIZEOF_OFF_T == 8

// If 64 bit off_t is enforced, libmpg123.c already defines the _64 functions.
#ifndef FORCED_OFF_64
int attribute_align_arg mpg123_replace_reader_64(mpg123_handle *mh, mpg123_ssize_t (*r_read) (int, void *, size_t), off_t (*r_lseek)(int, off_t, int) )
{
	return mpg123_replace_reader(mh, r_read, r_lseek);
}

int attribute_align_arg mpg123_replace_reader_handle_64(mpg123_handle *mh, mpg123_ssize_t (*r_read) (void*, void *, size_t), off_t (*r_lseek)(void*, off_t, int), void (*cleanup)(void*))
{
	return mpg123_replace_reader_handle(mh, r_read, r_lseek, cleanup);
}
#endif

#endif
