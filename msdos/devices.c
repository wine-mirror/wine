/*
 * DOS devices
 *
 * Copyright 1999 Ove Kåven
 */

#include <stdlib.h>
#include <string.h>
#include "wine/winbase16.h"
#include "msdos.h"
#include "miscemu.h"
#include "debug.h"

#include "pshpack1.h"

typedef struct {
  DOS_DEVICE_HEADER hdr;
  BYTE ljmp1;
  RMCBPROC strategy;
  BYTE ljmp2;
  RMCBPROC interrupt;
} WINEDEV;

#include "poppack.h"

DOS_LISTOFLISTS * DOS_LOL;
DWORD DOS_LOLSeg;

#define NONEXT ((DWORD)-1)

#define ATTR_STDIN     0x0001
#define ATTR_STDOUT    0x0002
#define ATTR_NUL       0x0004
#define ATTR_CLOCK     0x0008
#define ATTR_FASTCON   0x0010
#define ATTR_REMOVABLE 0x0800
#define ATTR_NONIBM    0x2000 /* block devices */
#define ATTR_UNTILBUSY 0x2000 /* char devices */
#define ATTR_IOCTL     0x4000
#define ATTR_CHAR      0x8000

#define LJMP 0x9a

static void nul_strategy(CONTEXT*ctx)
{
}

static void nul_interrupt(CONTEXT*ctx)
{
}

static void con_strategy(CONTEXT*ctx)
{
}

static void con_interrupt(CONTEXT*ctx)
{
}

#define STRATEGY_OFS sizeof(DOS_DEVICE_HEADER)
#define INTERRUPT_OFS STRATEGY_OFS+5

static DOS_DEVICE_HEADER dev_nul_hdr={
 NONEXT,
 ATTR_CHAR|ATTR_NUL,
 STRATEGY_OFS,INTERRUPT_OFS,
 "NUL     "
};

static WINEDEV devs={
 {NONEXT,
  ATTR_CHAR|ATTR_STDIN|ATTR_STDOUT|ATTR_FASTCON,
  STRATEGY_OFS,INTERRUPT_OFS,
  "CON     "},
 LJMP,con_strategy,
 LJMP,con_interrupt
};
#define nr_devs (sizeof(devs)/sizeof(WINEDEV))

static void InitListOfLists()
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

void DOSDEV_InstallDOSDevices(void)
{
  WINEDEV *dev;
  DOS_DEVICE_HEADER *pdev;
  UINT16 seg;
  int n;
  WORD ofs = sizeof(DOS_LISTOFLISTS)-sizeof(DOS_DEVICE_HEADER);

  /* allocate DOS data segment or something */
  DOS_LOLSeg = GlobalDOSAlloc16(ofs+sizeof(WINEDEV)+sizeof(devs));
  seg = HIWORD(DOS_LOLSeg);
  DOS_LOL = PTR_SEG_OFF_TO_LIN(LOWORD(DOS_LOLSeg), 0);

  InitListOfLists();

  /* copy first device (NUL) */
  pdev = &(DOS_LOL->NUL_dev);
  memcpy(pdev,&dev_nul_hdr,sizeof(DOS_DEVICE_HEADER));
  pdev->strategy += ofs;
  pdev->interrupt += ofs;
  /* set up dev so we can copy over the rest */
  dev = (WINEDEV*)(((char*)DOS_LOL)+ofs);
  dev[0].ljmp1 = LJMP;
  dev[0].strategy = (RMCBPROC)DPMI_AllocInternalRMCB(nul_strategy);
  dev[0].ljmp2 = LJMP;
  dev[0].interrupt = (RMCBPROC)DPMI_AllocInternalRMCB(nul_interrupt);

  dev++;
  ofs += sizeof(WINEDEV);

  memcpy(dev,&devs,sizeof(devs));
  for (n=0; n<nr_devs; n++) {
    pdev->next_dev = PTR_SEG_OFF_TO_SEGPTR(seg, ofs);
    dev[n].hdr.strategy += ofs;
    dev[n].hdr.interrupt += ofs;
    dev[n].strategy = (RMCBPROC)DPMI_AllocInternalRMCB(dev[n].strategy);
    dev[n].interrupt = (RMCBPROC)DPMI_AllocInternalRMCB(dev[n].interrupt);
    ofs += sizeof(WINEDEV);
    pdev = &(dev[n].hdr);
  }
}
