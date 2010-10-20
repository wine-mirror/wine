/*
 * DOS devices
 *
 * Copyright 1999 Ove Kåven
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

#include <stdlib.h>
#include <string.h>
#include "wine/winbase16.h"
#include "dosexe.h"
#include "wine/debug.h"

#include "pshpack1.h"

/* Warning: need to return LOL ptr w/ offset 0 (&ptr_first_DPB) to programs ! */
typedef struct _DOS_LISTOFLISTS
{
    WORD  CX_Int21_5e01;        /* -24d contents of CX from INT 21/AX=5E01h */
    WORD  LRU_count_FCB_cache;  /* -22d */
    WORD  LRU_count_FCB_open;   /* -20d */
    DWORD OEM_func_handler;     /* -18d OEM function of INT 21/AH=F8h */
    WORD  INT21_offset;         /* -14d offset in DOS CS of code to return from INT 21 call */
    WORD  sharing_retry_count;  /* -12d */
    WORD  sharing_retry_delay;  /* -10d */
    DWORD ptr_disk_buf;         /* -8d ptr to current disk buf */
    WORD  offs_unread_CON;      /* -4d pointer in DOS data segment of unread CON input */
    WORD  seg_first_MCB;        /* -2d */
    DWORD ptr_first_DPB;        /* 00 */
    DWORD ptr_first_SysFileTable; /* 04 */
    DWORD ptr_clock_dev_hdr;    /* 08 */
    DWORD ptr_CON_dev_hdr;      /* 0C */
    WORD  max_byte_per_sec;     /* 10 maximum bytes per sector of any block device */
    DWORD ptr_disk_buf_info;    /* 12 */
    DWORD ptr_array_CDS;        /* 16 current directory structure */
    DWORD ptr_sys_FCB;          /* 1A */
    WORD  nr_protect_FCB;       /* 1E */
    BYTE  nr_block_dev;         /* 20 */
    BYTE  nr_avail_drive_letters; /* 21 */
    DOS_DEVICE_HEADER NUL_dev;  /* 22 */
    BYTE  nr_drives_JOINed;     /* 34 */
    WORD  ptr_spec_prg_names;   /* 35 */
    DWORD ptr_SETVER_prg_list;  /* 37 */
    WORD DOS_HIGH_A20_func_offs;/* 3B */
    WORD PSP_last_exec;         /* 3D if DOS in HMA: PSP of program executed last; if DOS low: 0000h */
    WORD BUFFERS_val;           /* 3F */
    WORD BUFFERS_nr_lookahead;  /* 41 */
    BYTE boot_drive;            /* 43 */
    BYTE flag_DWORD_moves;      /* 44 01h for 386+, 00h otherwise */
    WORD size_extended_mem;     /* 45 size of extended mem in KB */
    SEGPTR wine_rm_lol;         /* -- wine: Real mode pointer to LOL */
    SEGPTR wine_pm_lol;         /* -- wine: Protected mode pointer to LOL */
} DOS_LISTOFLISTS;

#include "poppack.h"

#define CON_BUFFER 128

enum strategy { SYSTEM_STRATEGY_NUL, SYSTEM_STRATEGY_CON, NB_SYSTEM_STRATEGIES };

static void *strategy_data[NB_SYSTEM_STRATEGIES];

#define LJMP 0xea


/* prototypes */
static void WINAPI nul_strategy(CONTEXT*ctx);
static void WINAPI nul_interrupt(CONTEXT*ctx);
static void WINAPI con_strategy(CONTEXT*ctx);
static void WINAPI con_interrupt(CONTEXT*ctx);

/* devices */
static const WINEDEV devs[] =
{
  { "NUL     ",
    ATTR_CHAR|ATTR_NUL|ATTR_DEVICE,
    nul_strategy, nul_interrupt },

  { "CON     ",
    ATTR_CHAR|ATTR_STDIN|ATTR_STDOUT|ATTR_FASTCON|ATTR_NOTEOF|ATTR_DEVICE,
    con_strategy, con_interrupt }
};

#define NR_DEVS (sizeof(devs)/sizeof(WINEDEV))

/* DOS data segment */
typedef struct
{
    DOS_LISTOFLISTS    lol;
    DOS_DEVICE_HEADER  dev[NR_DEVS-1];
    DOS_DEVICE_HEADER *last_dev; /* ptr to last registered device driver */
    WINEDEV_THUNK      thunk[NR_DEVS];
    REQ_IO             req;
    BYTE               buffer[CON_BUFFER];

} DOS_DATASEG;

#define DOS_DATASEG_OFF(xxx) FIELD_OFFSET(DOS_DATASEG, xxx)

static DWORD DOS_LOLSeg;

static struct _DOS_LISTOFLISTS * DOSMEM_LOL(void)
{
    return PTR_REAL_TO_LIN(HIWORD(DOS_LOLSeg),0);
}


/* the device implementations */
static void do_lret(CONTEXT*ctx)
{
  WORD *stack = CTX_SEG_OFF_TO_LIN(ctx, ctx->SegSs, ctx->Esp);

  ctx->Eip   = *(stack++);
  ctx->SegCs = *(stack++);
  ctx->Esp  += 2*sizeof(WORD);
}

static void do_strategy(CONTEXT*ctx, int id, int extra)
{
  REQUEST_HEADER *hdr = CTX_SEG_OFF_TO_LIN(ctx, ctx->SegEs, ctx->Ebx);
  void **hdr_ptr = strategy_data[id];

  if (!hdr_ptr) {
    hdr_ptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(void *)+extra);
    strategy_data[id] = hdr_ptr;
  }
  *hdr_ptr = hdr;
  do_lret(ctx);
}

static REQUEST_HEADER * get_hdr(int id, void**extra)
{
  void **hdr_ptr = strategy_data[id];
  if (extra)
    *extra = hdr_ptr ? (void*)(hdr_ptr+1) : NULL;
  return hdr_ptr ? *hdr_ptr : NULL;
}

static void WINAPI nul_strategy(CONTEXT*ctx)
{
  do_strategy(ctx, SYSTEM_STRATEGY_NUL, 0);
}

static void WINAPI nul_interrupt(CONTEXT*ctx)
{
  REQUEST_HEADER *hdr = get_hdr(SYSTEM_STRATEGY_NUL, NULL);
  /* eat everything and recycle nothing */
  switch (hdr->command) {
  case CMD_INPUT:
    ((REQ_IO*)hdr)->count = 0;
    hdr->status = STAT_DONE;
    break;
  case CMD_SAFEINPUT:
    hdr->status = STAT_DONE|STAT_BUSY;
    break;
  default:
    hdr->status = STAT_DONE;
  }
  do_lret(ctx);
}

static void WINAPI con_strategy(CONTEXT*ctx)
{
  do_strategy(ctx, SYSTEM_STRATEGY_CON, sizeof(int));
}

static void WINAPI con_interrupt(CONTEXT*ctx)
{
  int *scan;
  REQUEST_HEADER *hdr = get_hdr(SYSTEM_STRATEGY_CON,(void **)&scan);
  BIOSDATA *bios = DOSVM_BiosData();
  WORD CurOfs = bios->NextKbdCharPtr;
  DOS_LISTOFLISTS *lol = DOSMEM_LOL();
  DOS_DATASEG *dataseg = (DOS_DATASEG *)lol;
  BYTE *linebuffer = dataseg->buffer;
  BYTE *curbuffer = (lol->offs_unread_CON) ?
    (((BYTE*)dataseg) + lol->offs_unread_CON) : NULL;
  DOS_DEVICE_HEADER *con = dataseg->dev;
  DWORD w;

  switch (hdr->command) {
  case CMD_INPUT:
    {
      REQ_IO *io = (REQ_IO *)hdr;
      WORD count = io->count, len = 0;
      BYTE *buffer = CTX_SEG_OFF_TO_LIN(ctx,
					SELECTOROF(io->buffer),
					(DWORD)OFFSETOF(io->buffer));

      hdr->status = STAT_BUSY;
      /* first, check whether we already have data in line buffer */
      if (curbuffer) {
	/* yep, copy as much as we can */
	BYTE data = 0;
	while ((len<count) && (data != '\r')) {
	  data = *curbuffer++;
	  buffer[len++] = data;
	}
	if (data == '\r') {
	  /* line buffer emptied */
	  lol->offs_unread_CON = 0;
	  curbuffer = NULL;
	  /* if we're not in raw mode, call it a day */
	  if (!(con->attr & ATTR_RAW)) {
	    hdr->status = STAT_DONE;
	    io->count = len;
	    break;
	  }
	} else {
	  /* still some data left */
	  lol->offs_unread_CON = curbuffer - (BYTE*)lol;
	  /* but buffer was filled, we're done */
	  hdr->status = STAT_DONE;
	  io->count = len;
	  break;
	}
      }

      /* if we're in raw mode, we just need to fill the buffer */
      if (con->attr & ATTR_RAW) {
	while (len<count) {
	  WORD data;

	  /* do we have a waiting scancode? */
	  if (*scan) {
	    /* yes, store scancode in buffer */
	    buffer[len++] = *scan;
	    *scan = 0;
	    if (len==count) break;
	  }

	  /* check for new keyboard input */
	  while (CurOfs == bios->FirstKbdCharPtr) {
	    /* no input available yet, so wait... */
	    DOSVM_Wait( ctx );
	  }
	  /* read from keyboard queue (call int16?) */
	  data = ((WORD*)bios)[CurOfs];
	  CurOfs += 2;
	  if (CurOfs >= bios->KbdBufferEnd) CurOfs = bios->KbdBufferStart;
	  bios->NextKbdCharPtr = CurOfs;
	  /* if it's an extended key, save scancode */
	  if (LOBYTE(data) == 0) *scan = HIBYTE(data);
	  /* store ASCII char in buffer */
	  buffer[len++] = LOBYTE(data);
	}
      } else {
	/* we're not in raw mode, so we need to do line input... */
	while (TRUE) {
	  WORD data;
	  /* check for new keyboard input */
	  while (CurOfs == bios->FirstKbdCharPtr) {
	    /* no input available yet, so wait... */
	    DOSVM_Wait( ctx );
	  }
	  /* read from keyboard queue (call int16?) */
	  data = ((WORD*)bios)[CurOfs];
	  CurOfs += 2;
	  if (CurOfs >= bios->KbdBufferEnd) CurOfs = bios->KbdBufferStart;
	  bios->NextKbdCharPtr = CurOfs;

	  if (LOBYTE(data) == '\r') {
	    /* it's the return key, we're done */
	    linebuffer[len++] = LOBYTE(data);
	    break;
	  }
	  else if (LOBYTE(data) >= ' ') {
	    /* a character */
	    if ((len+1)<CON_BUFFER) {
	      linebuffer[len] = LOBYTE(data);
	      WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), &linebuffer[len++], 1, &w, NULL);
	    }
	    /* else beep, but I don't like noise */
	  }
	  else switch (LOBYTE(data)) {
	  case '\b':
	    if (len>0) {
	      len--;
	      WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "\b \b", 3, &w, NULL);
	    }
	    break;
	  }
	}
	if (len > count) {
	  /* save rest of line for later */
	  lol->offs_unread_CON = linebuffer - (BYTE*)lol + count;
	  len = count;
	}
	memcpy(buffer, linebuffer, len);
      }
      hdr->status = STAT_DONE;
      io->count = len;
    }
    break;
  case CMD_SAFEINPUT:
    if (curbuffer) {
      /* some line input waiting */
      hdr->status = STAT_DONE;
      ((REQ_SAFEINPUT*)hdr)->data = *curbuffer;
    }
    else if (con->attr & ATTR_RAW) {
      if (CurOfs == bios->FirstKbdCharPtr) {
	/* no input */
	hdr->status = STAT_DONE|STAT_BUSY;
      } else {
	/* some keyboard input waiting */
	hdr->status = STAT_DONE;
	((REQ_SAFEINPUT*)hdr)->data = ((BYTE*)bios)[CurOfs];
      }
    } else {
      /* no line input */
      hdr->status = STAT_DONE|STAT_BUSY;
    }
    break;
  case CMD_INSTATUS:
    if (curbuffer) {
      /* we have data */
      hdr->status = STAT_DONE;
    }
    else if (con->attr & ATTR_RAW) {
      if (CurOfs == bios->FirstKbdCharPtr) {
	/* no input */
	hdr->status = STAT_DONE|STAT_BUSY;
      } else {
	/* some keyboard input waiting */
	hdr->status = STAT_DONE;
      }
    } else {
      /* no line input */
      hdr->status = STAT_DONE|STAT_BUSY;
    }

    break;
  case CMD_INFLUSH:
    /* flush line and keyboard queue */
    lol->offs_unread_CON = 0;
    bios->NextKbdCharPtr = bios->FirstKbdCharPtr;
    break;
  case CMD_OUTPUT:
  case CMD_SAFEOUTPUT:
    {
      REQ_IO *io = (REQ_IO *)hdr;
      BYTE *buffer = CTX_SEG_OFF_TO_LIN(ctx,
					SELECTOROF(io->buffer),
					(DWORD)OFFSETOF(io->buffer));
      DWORD result = 0;
      WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buffer, io->count, &result, NULL);
      io->count = result;
      hdr->status = STAT_DONE;
    }
    break;
  default:
    hdr->status = STAT_DONE;
  }
  do_lret(ctx);
}

static void InitListOfLists(DOS_LISTOFLISTS *DOS_LOL)
{
/*
Output of DOS 6.22:

0133:0020                    6A 13-33 01 CC 00 33 01 59 00         j.3...3.Y.
0133:0030  70 00 00 00 72 02 00 02-6D 00 33 01 00 00 2E 05   p...r...m.3.....
0133:0040  00 00 FC 04 00 00 03 08-92 21 11 E0 04 80 C6 0D   .........!......
0133:0050  CC 0D 4E 55 4C 20 20 20-20 20 00 00 00 00 00 00   ..NUL     ......
0133:0060  00 4B BA C1 06 14 00 00-00 03 01 00 04 70 CE FF   .K...........p..
0133:0070  FF 00 00 00 00 00 00 00-00 01 00 00 0D 05 00 00   ................
0133:0080  00 FF FF 00 00 00 00 FE-00 00 F8 03 FF 9F 70 02   ..............p.
0133:0090  D0 44 C8 FD D4 44 C8 FD-D4 44 C8 FD D0 44 C8 FD   .D...D...D...D..
0133:00A0  D0 44 C8 FD D0 44                                 .D...D
*/
  DOS_LOL->CX_Int21_5e01		= 0x0;
  DOS_LOL->LRU_count_FCB_cache	= 0x0;
  DOS_LOL->LRU_count_FCB_open		= 0x0;
  DOS_LOL->OEM_func_handler		= -1; /* not available */
  DOS_LOL->INT21_offset		= 0x0;
  DOS_LOL->sharing_retry_count	= 3;
  DOS_LOL->sharing_retry_delay	= 1;
  DOS_LOL->ptr_disk_buf		= 0x0;
  DOS_LOL->offs_unread_CON		= 0x0;
  DOS_LOL->seg_first_MCB		= 0x0;
  DOS_LOL->ptr_first_DPB		= 0x0;
  DOS_LOL->ptr_first_SysFileTable	= 0x0;
  DOS_LOL->ptr_clock_dev_hdr		= 0x0;
  DOS_LOL->ptr_CON_dev_hdr		= 0x0;
  DOS_LOL->max_byte_per_sec		= 512;
  DOS_LOL->ptr_disk_buf_info		= 0x0;
  DOS_LOL->ptr_array_CDS		= 0x0;
  DOS_LOL->ptr_sys_FCB		= 0x0;
  DOS_LOL->nr_protect_FCB		= 0x0;
  DOS_LOL->nr_block_dev		= 0x0;
  DOS_LOL->nr_avail_drive_letters	= 26; /* A - Z */
  DOS_LOL->nr_drives_JOINed		= 0x0;
  DOS_LOL->ptr_spec_prg_names		= 0x0;
  DOS_LOL->ptr_SETVER_prg_list	= 0x0; /* no SETVER list */
  DOS_LOL->DOS_HIGH_A20_func_offs	= 0x0;
  DOS_LOL->PSP_last_exec		= 0x0;
  DOS_LOL->BUFFERS_val		= 99; /* maximum: 99 */
  DOS_LOL->BUFFERS_nr_lookahead	= 8; /* maximum: 8 */
  DOS_LOL->boot_drive			= 3; /* C: */
  DOS_LOL->flag_DWORD_moves		= 0x01; /* i386+ */
  DOS_LOL->size_extended_mem		= 0xf000; /* very high value */
}

void DOSDEV_SetupDevice(const WINEDEV * devinfo,
			WORD seg, WORD off_dev, WORD off_thunk)
{
  DOS_DEVICE_HEADER *dev = PTR_REAL_TO_LIN(seg, off_dev);
  WINEDEV_THUNK *thunk = PTR_REAL_TO_LIN(seg, off_thunk);
  DOS_DATASEG *dataseg = (DOS_DATASEG*)DOSMEM_LOL();

  dev->attr = devinfo->attr;
  dev->strategy  = off_thunk + FIELD_OFFSET(WINEDEV_THUNK, ljmp1);
  dev->interrupt = off_thunk + FIELD_OFFSET(WINEDEV_THUNK, ljmp2);
  memcpy(dev->name, devinfo->name, 8);

  thunk->ljmp1     = LJMP;
  thunk->strategy  = DPMI_AllocInternalRMCB(devinfo->strategy);
  thunk->ljmp2     = LJMP;
  thunk->interrupt = DPMI_AllocInternalRMCB(devinfo->interrupt);

  dev->next_dev = NONEXT;
  if (dataseg->last_dev)
      dataseg->last_dev->next_dev = MAKESEGPTR(seg, off_dev);
  dataseg->last_dev = dev;
}

void DOSDEV_InstallDOSDevices(void)
{
  DOS_DATASEG *dataseg;
  WORD seg;
  WORD selector;
  unsigned int n;

  /* allocate DOS data segment or something */
  dataseg = DOSVM_AllocDataUMB( sizeof(DOS_DATASEG), &seg, &selector );

  DOS_LOLSeg = MAKESEGPTR( seg, 0 );
  DOSMEM_LOL()->wine_rm_lol = 
      MAKESEGPTR( seg, FIELD_OFFSET(DOS_LISTOFLISTS, ptr_first_DPB) );
  DOSMEM_LOL()->wine_pm_lol = 
      MAKESEGPTR( selector, FIELD_OFFSET(DOS_LISTOFLISTS, ptr_first_DPB) );

  /* initialize the magnificent List Of Lists */
  InitListOfLists(&dataseg->lol);

  /* Set up first device (NUL) */
  dataseg->last_dev = NULL;
  DOSDEV_SetupDevice( &devs[0],
		      seg,
		      DOS_DATASEG_OFF(lol.NUL_dev),
		      DOS_DATASEG_OFF(thunk[0]) );

  /* Set up the remaining devices */
  for (n = 1; n < NR_DEVS; n++)
    DOSDEV_SetupDevice( &devs[n],
			seg,
			DOS_DATASEG_OFF(dev[n-1]),
			DOS_DATASEG_OFF(thunk[n]) );

  /* CON is device 1 */
  dataseg->lol.ptr_CON_dev_hdr = MAKESEGPTR(seg, DOS_DATASEG_OFF(dev[0]));
}

void DOSDEV_SetSharingRetry(WORD delay, WORD count)
{
    DOSMEM_LOL()->sharing_retry_delay = delay;
    if (count) DOSMEM_LOL()->sharing_retry_count = count;
}

SEGPTR DOSDEV_GetLOL(BOOL v86)
{
    if (v86) return DOSMEM_LOL()->wine_rm_lol;
    else return DOSMEM_LOL()->wine_pm_lol;
}
