/*
 * DOS memory emulation
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Marcus Meissner
 */

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "winbase.h"
#include "wine/winbase16.h"
#include "global.h"
#include "ldt.h"
#include "miscemu.h"
#include "vga.h"
#include "module.h"
#include "task.h"
#include "debug.h"

DECLARE_DEBUG_CHANNEL(dosmem)
DECLARE_DEBUG_CHANNEL(selector)

HANDLE16 DOSMEM_BiosDataSeg;  /* BIOS data segment at 0x40:0 */
HANDLE16 DOSMEM_BiosSysSeg;   /* BIOS ROM segment at 0xf000:0 */

#pragma pack(4)

static char	*DOSMEM_dosmem;

       DWORD 	 DOSMEM_CollateTable;

       DWORD	 DOSMEM_ErrorCall;
       DWORD	 DOSMEM_ErrorBuffer;

/* use 2 low bits of 'size' for the housekeeping */

#define DM_BLOCK_DEBUG		0xABE00000
#define DM_BLOCK_TERMINAL	0x00000001
#define DM_BLOCK_FREE		0x00000002
#define DM_BLOCK_MASK		0x001FFFFC

/*
#define __DOSMEM_DEBUG__
 */

typedef struct {
   unsigned	size;
} dosmem_entry;

typedef struct {
  unsigned      blocks;
  unsigned      free;
} dosmem_info;

#define NEXT_BLOCK(block) \
        (dosmem_entry*)(((char*)(block)) + \
	 sizeof(dosmem_entry) + ((block)->size & DM_BLOCK_MASK))

#define VM_STUB(x) (0x90CF00CD|(x<<8)) /* INT x; IRET; NOP */
#define VM_STUB_SEGMENT 0xf000         /* BIOS segment */

/***********************************************************************
 *           DOSMEM_MemoryBase
 *
 * Gets the DOS memory base.
 */
char *DOSMEM_MemoryBase(HMODULE16 hModule)
{
    TDB *pTask = hModule ? NULL : (TDB *)GlobalLock16( GetCurrentTask() );
    NE_MODULE *pModule = (hModule || pTask) ? NE_GetPtr( hModule ? hModule : pTask->hModule ) : NULL;

    GlobalUnlock16( GetCurrentTask() );
    if (pModule && pModule->dos_image)
        return pModule->dos_image;
    else
        return DOSMEM_dosmem;
}

/***********************************************************************
 *           DOSMEM_MemoryTop
 *
 * Gets the DOS memory top.
 */
static char *DOSMEM_MemoryTop(HMODULE16 hModule)
{
    return DOSMEM_MemoryBase(hModule)+0x9FFFC; /* 640K */
}

/***********************************************************************
 *           DOSMEM_InfoBlock
 *
 * Gets the DOS memory info block.
 */
static dosmem_info *DOSMEM_InfoBlock(HMODULE16 hModule)
{
    return (dosmem_info*)(DOSMEM_MemoryBase(hModule)+0x10000); /* 64K */
}

/***********************************************************************
 *           DOSMEM_RootBlock
 *
 * Gets the DOS memory root block.
 */
static dosmem_entry *DOSMEM_RootBlock(HMODULE16 hModule)
{
    /* first block has to be paragraph-aligned */
    return (dosmem_entry*)(((char*)DOSMEM_InfoBlock(hModule)) +
                           ((((sizeof(dosmem_info) + 0xf) & ~0xf) - sizeof(dosmem_entry))));
}

/***********************************************************************
 *           DOSMEM_FillIsrTable
 *
 * Fill the interrupt table with fake BIOS calls to BIOSSEG (0xf000).
 *
 * NOTES:
 * Linux normally only traps INTs performed from or destined to BIOSSEG
 * for us to handle, if the int_revectored table is empty. Filling the
 * interrupt table with calls to INT stubs in BIOSSEG allows DOS programs
 * to hook interrupts, as well as use their familiar retf tricks to call
 * them, AND let Wine handle any unhooked interrupts transparently.
 */
static void DOSMEM_FillIsrTable(HMODULE16 hModule)
{
    SEGPTR *isr = (SEGPTR*)DOSMEM_MemoryBase(hModule);
    DWORD *stub = (DWORD*)((char*)isr + (VM_STUB_SEGMENT << 4));
    int x;
 
    for (x=0; x<256; x++) isr[x]=PTR_SEG_OFF_TO_SEGPTR(VM_STUB_SEGMENT,x*4);
    for (x=0; x<256; x++) stub[x]=VM_STUB(x);
} 

/***********************************************************************
 *           DOSMEM_InitDPMI
 *
 * Allocate the global DPMI RMCB wrapper.
 */
static void DOSMEM_InitDPMI(void)
{
    extern UINT16 DPMI_wrap_seg;
    static char wrap_code[]={
     0xCD,0x31, /* int $0x31 */
     0xCB       /* lret */
    };
    LPSTR wrapper = (LPSTR)DOSMEM_GetBlock(0, sizeof(wrap_code), &DPMI_wrap_seg);

    memcpy(wrapper, wrap_code, sizeof(wrap_code));
}

BIOSDATA * DOSMEM_BiosData()
{
    return (BIOSDATA *)(DOSMEM_MemoryBase(0)+0x400);
}

BYTE * DOSMEM_BiosSys()
{
    return DOSMEM_MemoryBase(0)+0xf0000;
}

/***********************************************************************
 *           DOSMEM_FillBiosSegments
 *
 * Fill the BIOS data segment with dummy values.
 */
static void DOSMEM_FillBiosSegments(void)
{
    BYTE *pBiosSys = (BYTE *)GlobalLock16( DOSMEM_BiosSysSeg );
    BYTE *pBiosROMTable = pBiosSys+0xe6f5;
    BIOSDATA *pBiosData = (BIOSDATA *)GlobalLock16( DOSMEM_BiosDataSeg );

    /* bogus 0xe0xx addresses !! Adapt int 0x10/0x1b if change needed */
    BYTE *pVideoStaticFuncTable = pBiosSys+0xe000;
    BYTE *pVideoStateInfo = pBiosSys+0xe010;
    BYTE *p;
    int i;

      /* Clear all unused values */
    memset( pBiosData, 0, sizeof(*pBiosData) );

    /* FIXME: should check the number of configured drives and ports */

    pBiosData->Com1Addr             = 0x3f8;
    pBiosData->Com2Addr             = 0x2f8;
    pBiosData->Lpt1Addr             = 0x378;
    pBiosData->Lpt2Addr             = 0x278;
    pBiosData->InstalledHardware    = 0x5463;
    pBiosData->MemSize              = 640;
    pBiosData->NextKbdCharPtr       = 0x1e;
    pBiosData->FirstKbdCharPtr      = 0x1e;
    pBiosData->VideoMode            = 3;
    pBiosData->VideoColumns         = 80;
    pBiosData->VideoPageSize        = 80 * 25 * 2;
    pBiosData->VideoPageStartAddr   = 0xb800;
    pBiosData->VideoCtrlAddr        = 0x3d4;
    pBiosData->Ticks                = INT1A_GetTicksSinceMidnight();
    pBiosData->NbHardDisks          = 2;
    pBiosData->KbdBufferStart       = 0x1e;
    pBiosData->KbdBufferEnd         = 0x3e;
    pBiosData->RowsOnScreenMinus1   = 23;
    pBiosData->BytesPerChar         = 0x10;
    pBiosData->ModeOptions          = 0x64;
    pBiosData->FeatureBitsSwitches  = 0xf9;
    pBiosData->VGASettings          = 0x51;
    pBiosData->DisplayCombination   = 0x08;
    pBiosData->DiskDataRate         = 0;

    /* fill ROM configuration table (values from Award) */
    *(WORD *)(pBiosROMTable)= 0x08; /* number of bytes following */
    *(pBiosROMTable+0x2)	= 0xfc; /* model */
    *(pBiosROMTable+0x3)	= 0x01; /* submodel */
    *(pBiosROMTable+0x4)	= 0x00; /* BIOS revision */
    *(pBiosROMTable+0x5)	= 0x74; /* feature byte 1 */
    *(pBiosROMTable+0x6)	= 0x00; /* feature byte 2 */
    *(pBiosROMTable+0x7)	= 0x00; /* feature byte 3 */
    *(pBiosROMTable+0x8)	= 0x00; /* feature byte 4 */
    *(pBiosROMTable+0x9)	= 0x00; /* feature byte 5 */

    p = pVideoStaticFuncTable;
    for (i=0; i < 7; i++)
      *(p+i)  = 0xff; /* modes supported 1 to 7 */
    
    *(p+0x7)  = 7;                  /* scan lines supported */
    *(p+0x8)  = 0;                  /* tot nr of char blocks in text mode */
    *(p+0x9)  = 0;                  /* max nr of active char blocks in text */
    *(WORD *)(p+0xa) = 0x8ff;       /* misc support flags */
    *(WORD *)(p+0xc) = 0;           /* reserved */
    *(p+0xe)  = 0x3f;               /* save pointer function flags */  
    *(p+0xf)  = 0;                  /* reserved */

    p = pVideoStateInfo;
    *(DWORD *)p = 0xf000e000;       /* address of pVideoStaticFuncTable, FIXME: always real mode ? */
    *(p+0x04) =                     /* current video mode, needs updates ! */
      pBiosData->VideoMode;
    *(WORD *)(p+0x05) =             /* number of columns, needs updates ! */
      pBiosData->VideoColumns;
    *(WORD *)(p+0x07) = 0;          /* length of regen (???) buffer in bytes */
    *(WORD *)(p+0x09) = 0;          /* starting address of regen (?) buffer */
    *(WORD *)(p+0x0b) = 0;          /* cursorpos page 0 */
    *(WORD *)(p+0x0d) = 0;          /* cursorpos page 1 */
    *(WORD *)(p+0x0f) = 0;          /* page 2 */
    *(WORD *)(p+0x11) = 0;          /* page 3 */
    *(WORD *)(p+0x13) = 0;          /* page 4 */
    *(WORD *)(p+0x15) = 0;          /* page 5 */
    *(WORD *)(p+0x17) = 0;          /* page 6 */
    *(WORD *)(p+0x19) = 0;          /* page 7 */
    *(WORD *)(p+0x1b) = 0x0a0b;     /* cursor size (start/end line) */
    *(p+0x1d) = 0;                  /* active display page */
    *(WORD *)(p+0x1e) = 0x3da;      /* CRTC port address */
    *(p+0x20) = 0x0;                /* current setting of port 0x3x8 */
    *(p+0x21) = 0x0;                /* current setting of port 0x3x9 */
    *(p+0x22) = 23;                 /* number of rows - 1 */
    *(WORD *)(p+0x23) = 0x10;       /* bytes/character */
    *(p+0x25) =                     /* comb. of active display */
      pBiosData->DisplayCombination;
    *(p+0x26) = 0;                  /* DCC (???) of alternate display */
    *(WORD *)(p+0x27) = 16;         /* number of colors in current mode */
    *(p+0x29) = 1;                  /* number of pages in current mode */
    *(p+0x2a) = 3;                  /* number of scan lines active */
                                    /* (0,1,2,3) =  (200,350,400,480) */
    *(p+0x2b) = 0;                  /* primary character block (?) */
    *(p+0x2c) = 0;                  /* secondary character block (?) */
    *(p+0x2d) =                     /* miscellaneous flags */
        (pBiosData->VGASettings & 0x0f)
      | ((pBiosData->ModeOptions & 1) << 4); /* cursor emulation */
    *(p+0x2e) = 0;                  /* non-VGA mode support */
    *(WORD *)(p+0x2f) = 0;          /* reserved */
    *(p+0x31) =                     /* video memory available */
      (pBiosData->ModeOptions & 0x60 >> 5);
    *(p+0x32) = 0;                  /* save pointer state flags */
    *(p+0x33) = 4;                  /* display info and status */

    /* BIOS date string */
    strcpy((char *)pBiosSys+0xfff5, "13/01/99");

    /* BIOS ID */
    *(pBiosSys+0xfffe) = 0xfc;
}

/***********************************************************************
 *           DOSMEM_InitCollateTable
 *
 * Initialises the collate table (character sorting, language dependent)
 */
static void DOSMEM_InitCollateTable()
{
	DWORD		x;
	unsigned char	*tbl;
	int		i;

	x = GlobalDOSAlloc16(258);
	DOSMEM_CollateTable = MAKELONG(0,(x>>16));
	tbl = DOSMEM_MapRealToLinear(DOSMEM_CollateTable);
	*(WORD*)tbl	= 0x100;
	tbl += 2;
	for ( i = 0; i < 0x100; i++) *tbl++ = i;
}

/***********************************************************************
 *           DOSMEM_InitErrorTable
 *
 * Initialises the error tables (DOS 5+)
 */
static void DOSMEM_InitErrorTable()
{
	DWORD		x;
	char 		*call;

        /* We will use a snippet of real mode code that calls */
        /* a WINE-only interrupt to handle moving the requested */
        /* message into the buffer... */

        /* FIXME - There is still something wrong... */

        /* FIXME - Find hex values for opcodes...
           
           (On call, AX contains message number
                     DI contains 'offset' (??)
            Resturn, ES:DI points to counted string )

           PUSH BX
           MOV BX, AX
           MOV AX, (arbitrary subfunction number)
           INT (WINE-only interrupt)
           POP BX
           RET

        */
           
        const int	code = 4;	
        const int	buffer = 80; 
        const int 	SIZE_TO_ALLOCATE = code + buffer;

        /* FIXME - Complete rewrite of the table system to save */
        /* precious DOS space. Now, we return the 0001:???? as */
        /* DOS 4+ (??, it seems to be the case in MS 7.10) treats that */
        /* as a special case and programs will use the alternate */
        /* interface (a farcall returned with INT 24 (AX = 0x122e, DL = */
        /* 0x08) which lets us have a smaller memory footprint anyway. */
 
 	x = GlobalDOSAlloc16(SIZE_TO_ALLOCATE);  

	DOSMEM_ErrorCall = MAKELONG(0,(x>>16));
        DOSMEM_ErrorBuffer = DOSMEM_ErrorCall + code;

	call = DOSMEM_MapRealToLinear(DOSMEM_ErrorCall);

        memset(call, 0, SIZE_TO_ALLOCATE);

        /* Fixme - Copy assembly into buffer here */        
}

/***********************************************************************
 *           DOSMEM_InitMemory
 *
 * Initialises the DOS memory structures.
 */
static void DOSMEM_InitMemory(HMODULE16 hModule)
{
   /* Low 64Kb are reserved for DOS/BIOS so the useable area starts at
    * 1000:0000 and ends at 9FFF:FFEF. */

    dosmem_info*        info_block = DOSMEM_InfoBlock(hModule);
    dosmem_entry*       root_block = DOSMEM_RootBlock(hModule);
    dosmem_entry*       dm;

    root_block->size = DOSMEM_MemoryTop(hModule) - (((char*)root_block) + sizeof(dosmem_entry));

    info_block->blocks = 0;
    info_block->free = root_block->size;

    dm = NEXT_BLOCK(root_block);
    dm->size = DM_BLOCK_TERMINAL;
    root_block->size |= DM_BLOCK_FREE 
#ifdef __DOSMEM_DEBUG__
		     | DM_BLOCK_DEBUG;
#endif
		     ;
}

/***********************************************************************
 *           DOSMEM_Init
 *
 * Create the dos memory segments, and store them into the KERNEL
 * exported values.
 */
BOOL DOSMEM_Init(HMODULE16 hModule)
{
    if (!hModule)
    {
        /* Allocate 1 MB dosmemory 
         * - it is mostly wasted but we can use some of it to 
         *   store internal translation tables, etc...
         */
        DOSMEM_dosmem = VirtualAlloc( NULL, 0x100000, MEM_COMMIT,
                                      PAGE_EXECUTE_READWRITE );
        if (!DOSMEM_dosmem)
        {
            WARN(dosmem, "Could not allocate DOS memory.\n" );
            return FALSE;
        }
        DOSMEM_BiosDataSeg = GLOBAL_CreateBlock(GMEM_FIXED,DOSMEM_dosmem+0x400,
                                     0x100, 0, FALSE, FALSE, FALSE, NULL );
        DOSMEM_BiosSysSeg = GLOBAL_CreateBlock(GMEM_FIXED,DOSMEM_dosmem+0xf0000,
                                     0x10000, 0, FALSE, FALSE, FALSE, NULL );
        DOSMEM_FillIsrTable(0);
        DOSMEM_FillBiosSegments();
        DOSMEM_InitMemory(0);
        DOSMEM_InitCollateTable();
        DOSMEM_InitErrorTable();
        DOSMEM_InitDPMI();
    }
    else
    {
#if 0
        DOSMEM_FillIsrTable(hModule);
        DOSMEM_InitMemory(hModule);
#else
        /* bootstrap the new V86 task with a copy of the "system" memory */
        memcpy(DOSMEM_MemoryBase(hModule), DOSMEM_dosmem, 0x100000);
#endif
    }
    return TRUE;
}


/***********************************************************************
 *           DOSMEM_Tick
 *
 * Increment the BIOS tick counter. Called by timer signal handler.
 */
void DOSMEM_Tick( WORD timer )
{
    BIOSDATA *pBiosData = DOSMEM_BiosData();
    if (pBiosData) pBiosData->Ticks++;
}

/***********************************************************************
 *           DOSMEM_GetBlock
 *
 * Carve a chunk of the DOS memory block (without selector).
 */
LPVOID DOSMEM_GetBlock(HMODULE16 hModule, UINT size, UINT16* pseg)
{
   UINT  	 blocksize;
   char         *block = NULL;
   dosmem_info  *info_block = DOSMEM_InfoBlock(hModule);
   dosmem_entry *dm;
#ifdef __DOSMEM_DEBUG_
   dosmem_entry *prev = NULL;
#endif
 
   if( size > info_block->free ) return NULL;
   dm = DOSMEM_RootBlock(hModule);

   while (dm && dm->size != DM_BLOCK_TERMINAL)
   {
#ifdef __DOSMEM_DEBUG__
       if( (dm->size & DM_BLOCK_DEBUG) != DM_BLOCK_DEBUG )
       {
	    WARN(dosmem,"MCB overrun! [prev = 0x%08x]\n", 4 + (UINT)prev);
	    return NULL;
       }
       prev = dm;
#endif
       if( dm->size & DM_BLOCK_FREE )
       {
	   dosmem_entry  *next = NEXT_BLOCK(dm);

	   while( next->size & DM_BLOCK_FREE ) /* collapse free blocks */
	   {
	       dm->size += sizeof(dosmem_entry) + (next->size & DM_BLOCK_MASK);
	       next->size = (DM_BLOCK_FREE | DM_BLOCK_TERMINAL);
	       next = NEXT_BLOCK(dm);
	   }

	   blocksize = dm->size & DM_BLOCK_MASK;
	   if( blocksize >= size )
           {
	       block = ((char*)dm) + sizeof(dosmem_entry);
	       if( blocksize - size > 0x20 )
	       {
		   /* split dm so that the next one stays
		    * paragraph-aligned (and dm loses free bit) */

	           dm->size = (((size + 0xf + sizeof(dosmem_entry)) & ~0xf) -
			         	      sizeof(dosmem_entry));
	           next = (dosmem_entry*)(((char*)dm) + 
	 		   sizeof(dosmem_entry) + dm->size);
	           next->size = (blocksize - (dm->size + 
			   sizeof(dosmem_entry))) | DM_BLOCK_FREE 
#ifdef __DOSMEM_DEBUG__
					          | DM_BLOCK_DEBUG
#endif
						  ;
	       } else dm->size &= DM_BLOCK_MASK;

	       info_block->blocks++;
	       info_block->free -= dm->size;
	       if( pseg ) *pseg = (block - DOSMEM_MemoryBase(hModule)) >> 4;
#ifdef __DOSMEM_DEBUG__
               dm->size |= DM_BLOCK_DEBUG;
#endif
	       break;
	   }
 	   dm = next;
       }
       else dm = NEXT_BLOCK(dm);
   }
   return (LPVOID)block;
}

/***********************************************************************
 *           DOSMEM_FreeBlock
 */
BOOL DOSMEM_FreeBlock(HMODULE16 hModule, void* ptr)
{
   dosmem_info  *info_block = DOSMEM_InfoBlock(hModule);

   if( ptr >= (void*)(((char*)DOSMEM_RootBlock(hModule)) + sizeof(dosmem_entry)) &&
       ptr < (void*)DOSMEM_MemoryTop(hModule) && !((((char*)ptr)
                  - DOSMEM_MemoryBase(hModule)) & 0xf) )
   {
       dosmem_entry  *dm = (dosmem_entry*)(((char*)ptr) - sizeof(dosmem_entry));

       if( !(dm->size & (DM_BLOCK_FREE | DM_BLOCK_TERMINAL))
#ifdef __DOSMEM_DEBUG__
	 && ((dm->size & DM_BLOCK_DEBUG) == DM_BLOCK_DEBUG )
#endif
	 )
       {
	     info_block->blocks--;
	     info_block->free += dm->size;

	     dm->size |= DM_BLOCK_FREE;
	     return TRUE;
       }
   }
   return FALSE;
}

/***********************************************************************
 *           DOSMEM_ResizeBlock
 */
LPVOID DOSMEM_ResizeBlock(HMODULE16 hModule, void* ptr, UINT size, UINT16* pseg)
{
   char         *block = NULL;
   dosmem_info  *info_block = DOSMEM_InfoBlock(hModule);

   if( ptr >= (void*)(((char*)DOSMEM_RootBlock(hModule)) + sizeof(dosmem_entry)) &&
       ptr < (void*)DOSMEM_MemoryTop(hModule) && !((((char*)ptr)
                  - DOSMEM_MemoryBase(hModule)) & 0xf) )
   {
       dosmem_entry  *dm = (dosmem_entry*)(((char*)ptr) - sizeof(dosmem_entry));

       if( pseg ) *pseg = ((char*)ptr - DOSMEM_MemoryBase(hModule)) >> 4;

       if( !(dm->size & (DM_BLOCK_FREE | DM_BLOCK_TERMINAL))
	 )
       {
	     dosmem_entry  *next = NEXT_BLOCK(dm);
	     UINT blocksize, orgsize = dm->size & DM_BLOCK_MASK;

	     while( next->size & DM_BLOCK_FREE ) /* collapse free blocks */
	     {
	         dm->size += sizeof(dosmem_entry) + (next->size & DM_BLOCK_MASK);
	         next->size = (DM_BLOCK_FREE | DM_BLOCK_TERMINAL);
	         next = NEXT_BLOCK(dm);
	     }

	     blocksize = dm->size & DM_BLOCK_MASK;
	     if (blocksize >= size)
	     {
	         block = ((char*)dm) + sizeof(dosmem_entry);
	         if( blocksize - size > 0x20 )
	         {
		     /* split dm so that the next one stays
		      * paragraph-aligned (and next gains free bit) */

	             dm->size = (((size + 0xf + sizeof(dosmem_entry)) & ~0xf) -
			         	        sizeof(dosmem_entry));
	             next = (dosmem_entry*)(((char*)dm) + 
	 		     sizeof(dosmem_entry) + dm->size);
	             next->size = (blocksize - (dm->size + 
			     sizeof(dosmem_entry))) | DM_BLOCK_FREE 
						    ;
	         } else dm->size &= DM_BLOCK_MASK;

		 info_block->free += orgsize - dm->size;
	     } else {
		 /* the collapse didn't help, try getting a new block */
		 block = DOSMEM_GetBlock(hModule, size, pseg);
		 if (block) {
		     /* we got one, copy the old data there (we do need to, right?) */
		     memcpy(block, ((char*)dm) + sizeof(dosmem_entry),
				   (size<orgsize) ? size : orgsize);
		     /* free old block */
		     info_block->blocks--;
		     info_block->free += dm->size;

		     dm->size |= DM_BLOCK_FREE;
		 } else {
		     /* and Bill Gates said 640K should be enough for everyone... */

		     /* need to split original and collapsed blocks apart again,
		      * and free the collapsed blocks again, before exiting */
		     if( blocksize - orgsize > 0x20 )
		     {
			 /* split dm so that the next one stays
			  * paragraph-aligned (and next gains free bit) */

			 dm->size = (((orgsize + 0xf + sizeof(dosmem_entry)) & ~0xf) -
						       sizeof(dosmem_entry));
			 next = (dosmem_entry*)(((char*)dm) + 
				 sizeof(dosmem_entry) + dm->size);
			 next->size = (blocksize - (dm->size + 
				 sizeof(dosmem_entry))) | DM_BLOCK_FREE 
							;
		     } else dm->size &= DM_BLOCK_MASK;
		 }
	     }
       }
   }
   return (LPVOID)block;
}


/***********************************************************************
 *           DOSMEM_Available
 */
UINT DOSMEM_Available(HMODULE16 hModule)
{
   UINT  	 blocksize, available = 0;
   dosmem_entry *dm;
   
   dm = DOSMEM_RootBlock(hModule);

   while (dm && dm->size != DM_BLOCK_TERMINAL)
   {
#ifdef __DOSMEM_DEBUG__
       if( (dm->size & DM_BLOCK_DEBUG) != DM_BLOCK_DEBUG )
       {
	    WARN(dosmem,"MCB overrun! [prev = 0x%08x]\n", 4 + (UINT)prev);
	    return NULL;
       }
       prev = dm;
#endif
       if( dm->size & DM_BLOCK_FREE )
       {
	   dosmem_entry  *next = NEXT_BLOCK(dm);

	   while( next->size & DM_BLOCK_FREE ) /* collapse free blocks */
	   {
	       dm->size += sizeof(dosmem_entry) + (next->size & DM_BLOCK_MASK);
	       next->size = (DM_BLOCK_FREE | DM_BLOCK_TERMINAL);
	       next = NEXT_BLOCK(dm);
	   }

	   blocksize = dm->size & DM_BLOCK_MASK;
	   if ( blocksize > available ) available = blocksize;
 	   dm = next;
       }
       else dm = NEXT_BLOCK(dm);
   }
   return available;
}


/***********************************************************************
 *           DOSMEM_MapLinearToDos
 *
 * Linear address to the DOS address space.
 */
UINT DOSMEM_MapLinearToDos(LPVOID ptr)
{
    if (((char*)ptr >= DOSMEM_MemoryBase(0)) &&
        ((char*)ptr < DOSMEM_MemoryBase(0) + 0x100000))
	  return (UINT)ptr - (UINT)DOSMEM_MemoryBase(0);
    return (UINT)ptr;
}


/***********************************************************************
 *           DOSMEM_MapDosToLinear
 *
 * DOS linear address to the linear address space.
 */
LPVOID DOSMEM_MapDosToLinear(UINT ptr)
{
    if (ptr < 0x100000) return (LPVOID)(ptr + (UINT)DOSMEM_MemoryBase(0));
    return (LPVOID)ptr;
}


/***********************************************************************
 *           DOSMEM_MapRealToLinear
 *
 * Real mode DOS address into a linear pointer
 */
LPVOID DOSMEM_MapRealToLinear(DWORD x)
{
   LPVOID       lin;

   lin=DOSMEM_MemoryBase(0)+(x&0xffff)+(((x&0xffff0000)>>16)*16);
   TRACE(selector,"(0x%08lx) returns 0x%p.\n",
                    x,lin );
   return lin;
}

/***********************************************************************
 *           DOSMEM_AllocSelector
 *
 * Allocates a protected mode selector for a realmode segment.
 */
WORD DOSMEM_AllocSelector(WORD realsel)
{
	HMODULE16 hModule = GetModuleHandle16("KERNEL");
	WORD	sel;

	sel=GLOBAL_CreateBlock(
		GMEM_FIXED,DOSMEM_dosmem+realsel*16,0x10000,
		hModule,FALSE,FALSE,FALSE,NULL
	);
	TRACE(selector,"(0x%04x) returns 0x%04x.\n",
		realsel,sel
	);
	return sel;
}

