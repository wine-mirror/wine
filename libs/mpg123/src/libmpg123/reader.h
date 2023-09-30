/*
	reader: reading input data

	copyright ?-2023 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis (after code from Michael Hipp)
*/

#ifndef MPG123_READER_H
#define MPG123_READER_H

#ifndef MPG123_H_INTERN
#error "include internal mpg123 header first"
#endif

#ifndef NO_FEEDER
struct buffy
{
	unsigned char *data;
	ptrdiff_t size;
	ptrdiff_t realsize;
	struct buffy *next;
};


struct bufferchain
{
	struct buffy* first; /* The beginning of the chain. */
	struct buffy* last;  /* The end...    of the chain. */
	ptrdiff_t size;        /* Aggregated size of all buffies. */
	/* These positions are relative to buffer chain beginning. */
	ptrdiff_t pos;         /* Position in whole chain. */
	ptrdiff_t firstpos;    /* The point of return on non-forget() */
	/* The "real" filepos is fileoff + pos. */
	int64_t fileoff;     /* Beginning of chain is at this file offset. */
	// Unsigned since no direct arithmetic with offsets. Overflow of overall
	// size needs to be checked anyway.
	size_t bufblock;     /* Default (minimal) size of buffers. */
	size_t pool_size;    /* Keep that many buffers in storage. */
	size_t pool_fill;    /* That many buffers are there. */
	/* A pool of buffers to re-use, if activated. It's a linked list that is worked on from the front. */
	struct buffy *pool;
};

/* Call this before any buffer chain use (even bc_init()). */
void INT123_bc_prepare(struct bufferchain *, size_t pool_size, size_t bufblock);
/* Free persistent data in the buffer chain, after bc_reset(). */
void INT123_bc_cleanup(struct bufferchain *);
/* Change pool size. This does not actually allocate/free anything on itself, just instructs later operations to free less / allocate more buffers. */
void INT123_bc_poolsize(struct bufferchain *, size_t pool_size, size_t bufblock);
/* Return available byte count in the buffer. */
size_t INT123_bc_fill(struct bufferchain *bc);

#endif

struct reader_data
{
	int64_t filelen; /* total file length or total buffer size */
	int64_t filepos; /* position in file or position in buffer chain */
	/* Custom opaque I/O handle from the client. */
	void *iohandle;
	int   flags;
	// The one and only lowlevel reader wrapper, wrapping over all others.
	// This is either libmpg123's wrapper or directly the user-supplied functions.
	int (*r_read64) (void *, void *, size_t, size_t *);
	int64_t (*r_lseek64)(void *, int64_t, int);
	void    (*cleanup_handle)(void *handle);
#ifndef NO_FEEDER
	struct bufferchain buffer; /* Not dynamically allocated, these few struct bytes aren't worth the trouble. */
#endif
};

/* start to use off_t to properly do LFS in future ... used to be long */
#ifdef __MORPHOS__
#undef tell /* unistd.h defines it as lseek(x, 0L, 1) */
#endif
struct reader
{
	int     (*init)           (mpg123_handle *);
	void    (*close)          (mpg123_handle *);
	ptrdiff_t (*fullread)     (mpg123_handle *, unsigned char *, ptrdiff_t);
	int     (*head_read)      (mpg123_handle *, unsigned long *newhead);    /* succ: TRUE, else <= 0 (FALSE or READER_MORE) */
	int     (*head_shift)     (mpg123_handle *, unsigned long *head);       /* succ: TRUE, else <= 0 (FALSE or READER_MORE) */
	int64_t (*skip_bytes)     (mpg123_handle *, int64_t len);               /* succ: >=0, else error or READER_MORE         */
	int     (*read_frame_body)(mpg123_handle *, unsigned char *, int size);
	int     (*back_bytes)     (mpg123_handle *, int64_t bytes);
	int     (*seek_frame)     (mpg123_handle *, int64_t num);
	int64_t (*tell)           (mpg123_handle *);
	void    (*rewind)         (mpg123_handle *);
	void    (*forget)         (mpg123_handle *);
};

/* Open an external handle. */
int INT123_open_stream_handle(mpg123_handle *, void *iohandle);

/* feed based operation has some specials */
int INT123_open_feed(mpg123_handle *);
/* externally called function, returns 0 on success, -1 on error */
int  INT123_feed_more(mpg123_handle *fr, const unsigned char *in, size_t count);
void INT123_feed_forget(mpg123_handle *fr);  /* forget the data that has been read (free some buffers) */
int64_t INT123_feed_set_pos(mpg123_handle *fr, int64_t pos); /* Set position (inside available data if possible), return wanted byte offset of next feed. */

void INT123_open_bad(mpg123_handle *);

#define READER_ID3TAG    0x2
#define READER_SEEKABLE  0x4
#define READER_BUFFERED  0x8
#define READER_NOSEEK    0x10
#define READER_HANDLEIO  0x40
#define READER_STREAM 0
#define READER_ICY_STREAM 1
#define READER_FEED       2
/* These two add a little buffering to enable small seeks for peek ahead. */
#define READER_BUF_STREAM 3
#define READER_BUF_ICY_STREAM 4

#define READER_ERROR MPG123_ERR
#define READER_MORE  MPG123_NEED_MORE

#endif
