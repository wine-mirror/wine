/*
 * DMA Emulation
 *
 * Copyright 2002 Christian Costa
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "dosexe.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dma);

/* Internal registers of the 2 DMA chips which control 8 DMA channels */
static DWORD DMA_BaseAddress[8];
static WORD  DMA_ByteCount[8];
static DWORD DMA_CurrentBaseAddress[8];
static WORD  DMA_CurrentByteCount[8];
static BYTE  DMA_Command[8];
static BYTE  DMA_Mask[2]={0x0F,0x0F};
static BYTE  DMA_Status[2]={0x00,0x00};
static BOOL  DMA_Toggle[2]={FALSE,FALSE};

/*
 * DMA_Transfer : Try to perform a transfer of reqlen elements (8 or 16 bits)
 * on the specified channel and return the elements transferred
 */
int DMA_Transfer(int channel,int reqlen,void* buffer)
{
    int i,size,ret=0;
    int opmode,increment,autoinit,trmode,dmachip;
    int regmode = DMA_Command[channel];
    char *p,*dmabuf;

    dmabuf = buffer;
    dmachip = (channel<4) ? 0 : 1;

    TRACE("DMA_Command = %x reqlen=%d\n",regmode,reqlen);

    /* Exit if channel is masked */
    if (DMA_Mask[dmachip]&(1<<(channel&3)))
        return 0;

    opmode = (regmode & 0xC0) >> 6;
    increment = !(regmode & 0x20);
    autoinit = regmode & 0x10;
    trmode = (regmode & 0x0C) >> 2;

    /* Transfer size : 8 bits for channels 0..3, 16 bits for channels 4..7 */
    size = (channel<4) ? 1 : 2;

    /* Process operating mode */
    switch(opmode)
    {
    case 0:
        /* Request mode */
        FIXME("Request Mode - Not Implemented\n");
        return 0;
    case 1:
        /* Single Mode */
        break;
    case 2:
        /* Request mode */
        FIXME("Block Mode - Not Implemented\n");
        return 0;
    case 3:
        /* Cascade Mode */
        ERR("Cascade Mode should not be used by regular apps\n");
        return 0;
    }

    /* Perform one the 4 transfer modes */
    if (trmode == 4) {
        /* Illegal */
        ERR("DMA Transfer Type Illegal\n");
        return 0;
    }

    ret = min(DMA_CurrentByteCount[channel],reqlen);

    /* Update DMA registers */
    DMA_CurrentByteCount[channel]-=ret;
    if (increment)
        DMA_CurrentBaseAddress[channel] += ret * size;
    else
        DMA_CurrentBaseAddress[channel] -= ret * size;

    switch(trmode)
    {
    case 0:
        /* Verification (no real transfer)*/
        TRACE("Verification DMA operation\n");
        break;
    case 1:
        /* Write */
        TRACE("Perform Write transfer of %d bytes at %x with count %x\n",ret,
            DMA_CurrentBaseAddress[channel],DMA_CurrentByteCount[channel]);
        if (increment)
            memcpy((void*)DMA_CurrentBaseAddress[channel],dmabuf,ret*size);
        else
            for(i=0,p=(char*)DMA_CurrentBaseAddress[channel];i<ret*size;i++)
                /* FIXME: possible endianness issue for 16 bits DMA */
                *(p-i) = dmabuf[i];
        break;
    case 2:
        /* Read */
        TRACE("Perform Read transfer of %d bytes at %x with count %x\n",ret,
            DMA_CurrentBaseAddress[channel],DMA_CurrentByteCount[channel]);
        if (increment)
            memcpy(dmabuf,(void*)DMA_CurrentBaseAddress[channel],ret*size);
        else
            for(i=0,p=(char*)DMA_CurrentBaseAddress[channel];i<ret*size;i++)
                /* FIXME: possible endianness issue for 16 bits DMA */
                dmabuf[i] = *(p-i);
        break;
    }

    /* Check for end of transfer */
    if (DMA_CurrentByteCount[channel]==0) {
        TRACE("DMA buffer empty\n");

        /* Update status register of the DMA chip corresponding to the channel */
        DMA_Status[dmachip] |= 1 << (channel & 0x3); /* Mark transfer as finished */
        DMA_Status[dmachip] &= ~(1 << ((channel & 0x3) + 4)); /* Reset soft request if any */

        if (autoinit) {
            /* Reload Current* register to their initial values */
            DMA_CurrentBaseAddress[channel] = DMA_BaseAddress[channel];
            DMA_CurrentByteCount[channel] = DMA_ByteCount[channel];
        }
    }

    return ret;
}


void DMA_ioport_out( WORD port, BYTE val )
{
    int channel,dmachip;

    switch(port)
    {
    case 0x00:
    case 0x02:
    case 0x04:
    case 0x06:
    case 0xC0:
    case 0xC4:
    case 0xC8:
    case 0xCC:
        /* Base Address*/
        channel = (port&0xC0)?((port-0xC0)>>2):(port>>1);
        dmachip = (channel<4) ? 0 : 1;
        if (!DMA_Toggle[dmachip])
            DMA_BaseAddress[channel]=(DMA_BaseAddress[channel] & ~0xFF)|(val & 0xFF);
        else {
            DMA_BaseAddress[channel]=(DMA_BaseAddress[channel] & (~(0xFF << 8)))|((val & 0xFF) << 8);
            DMA_CurrentBaseAddress[channel] = DMA_BaseAddress[channel];
            TRACE("Write Base Address = %x\n",DMA_BaseAddress[channel]);
        }
        DMA_Toggle[dmachip] = !DMA_Toggle[dmachip];
        break;

    case 0x01:
    case 0x03:
    case 0x05:
    case 0x07:
    case 0xC2:
    case 0xC6:
    case 0xCA:
    case 0xCE:
        /* Count*/
        channel = ((port-1)&0xC0)?(((port-1)-0xC0)>>2):(port>>1);
        dmachip = (channel<4) ? 0 : 1;
        if (!DMA_Toggle[dmachip])
            DMA_ByteCount[channel]=(DMA_ByteCount[channel] & ~0xFF)|((val+1) & 0xFF);
        else {
            DMA_ByteCount[channel]=(DMA_ByteCount[channel] & (~(0xFF << 8)))|(((val+1) & 0xFF) << 8);
            DMA_CurrentByteCount[channel] = DMA_ByteCount[channel];
            TRACE("Write Count = %x.\n",DMA_ByteCount[channel]);
        }
        DMA_Toggle[dmachip] = !DMA_Toggle[dmachip];
        break;

    /* Low Page Base Address */
    case 0x87: DMA_BaseAddress[0]=(DMA_BaseAddress[0] & 0xFF00FFFF)|((val & 0xFF) << 16); break;
    case 0x83: DMA_BaseAddress[1]=(DMA_BaseAddress[1] & 0xFF00FFFF)|((val & 0xFF) << 16); break;
    case 0x81: DMA_BaseAddress[2]=(DMA_BaseAddress[2] & 0xFF00FFFF)|((val & 0xFF) << 16); break;
    case 0x82: DMA_BaseAddress[3]=(DMA_BaseAddress[3] & 0xFF00FFFF)|((val & 0xFF) << 16); break;
    case 0x8B: DMA_BaseAddress[5]=(DMA_BaseAddress[5] & 0xFF00FFFF)|((val & 0xFF) << 16); break;
    case 0x89: DMA_BaseAddress[6]=(DMA_BaseAddress[6] & 0xFF00FFFF)|((val & 0xFF) << 16); break;
    case 0x8A: DMA_BaseAddress[7]=(DMA_BaseAddress[7] & 0xFF00FFFF)|((val & 0xFF) << 16); break;

    /* Low Page Base Address (only 4 lower bits are significant) */
    case 0x487: DMA_BaseAddress[0]=(DMA_BaseAddress[0] & 0x00FFFFFF)|((val & 0x0F) << 24); break;
    case 0x483: DMA_BaseAddress[1]=(DMA_BaseAddress[1] & 0x00FFFFFF)|((val & 0x0F) << 24); break;
    case 0x481: DMA_BaseAddress[2]=(DMA_BaseAddress[2] & 0x00FFFFFF)|((val & 0x0F) << 24); break;
    case 0x482: DMA_BaseAddress[3]=(DMA_BaseAddress[3] & 0x00FFFFFF)|((val & 0x0F) << 24); break;
    case 0x48B: DMA_BaseAddress[5]=(DMA_BaseAddress[5] & 0x00FFFFFF)|((val & 0x0F) << 24); break;
    case 0x489: DMA_BaseAddress[6]=(DMA_BaseAddress[6] & 0x00FFFFFF)|((val & 0x0F) << 24); break;
    case 0x48A: DMA_BaseAddress[7]=(DMA_BaseAddress[7] & 0x00FFFFFF)|((val & 0x0F) << 24); break;

    case 0x08:
    case 0xD0:
        /* Command */
        FIXME("Write Command (%x) - Not Implemented\n",val);
        break;

    case 0x0B:
    case 0xD6:
        /* Mode */
        TRACE("Write Mode (%x)\n",val);
        DMA_Command[((port==0xD6)?4:0)+(val&0x3)]=val;
        switch(val>>6)
        {
        case 0:
            /* Request mode */
            FIXME("Request Mode - Not Implemented\n");
            break;
        case 1:
            /* Single Mode */
            break;
        case 2:
            /* Block mode */
            FIXME("Block Mode - Not Implemented\n");
            break;
        case 3:
            /* Cascade Mode */
            ERR("Cascade Mode should not be used by regular apps\n");
            break;
        }
        break;

    case 0x0A:
    case 0xD4:
        /* Write Single Mask Bit */
        TRACE("Write Single Mask Bit (%x)\n",val);
        dmachip = (port==0x0A) ? 0 : 1;
        if (val&4)
            DMA_Mask[dmachip] |= 1<<(val&3);
        else
            DMA_Mask[dmachip] &= ~(1<<(val&3));
        break;

    case 0x0F:
    case 0xDE:
        /* Write All Mask Bits (only 4 lower bits are significant */
        FIXME("Write All Mask Bits (%x)\n",val);
        dmachip = (port==0x0F) ? 0 : 1;
        DMA_Mask[dmachip] = val & 0x0F;
        break;

    case 0x09:
    case 0xD2:
        /* Software DRQx Request */
        FIXME("Software DRQx Request (%x) - Not Implemented\n",val);
        break;

    case 0x0C:
    case 0xD8:
        /* Reset DMA Pointer Flip-Flop */
        TRACE("Reset Flip-Flop\n");
        DMA_Toggle[port==0xD8]=FALSE;
        break;

    case 0x0D:
    case 0xDA:
        /* Master Reset */
        TRACE("Master Reset\n");
        dmachip = (port==0x0D) ? 0 : 1;
        /* Reset DMA Pointer Flip-Flop */
        DMA_Toggle[dmachip]=FALSE;
        /* Mask all channels */
        DMA_Mask[dmachip] = 0x0F;
        break;

    case 0x0E:
    case 0xDC:
        /* Reset Mask Register */
        FIXME("Reset Mask Register\n");
        dmachip = (port==0x0E) ? 0 : 1;
        /* Unmask all channels */
        DMA_Mask[dmachip] = 0x00;
        break;
    }
}

BYTE DMA_ioport_in( WORD port )
{
    int channel,dmachip;
    BYTE res = 0;

    switch(port)
    {
    case 0x00:
    case 0x02:
    case 0x04:
    case 0x06:
    case 0xC0:
    case 0xC4:
    case 0xC8:
    case 0xCC:
        /* Base Address*/
        channel = (port&0xC0)?((port-0xC0)>>2):(port>>1);
        dmachip = (channel<4) ? 0 : 1;
        if (!DMA_Toggle[dmachip])
            res = DMA_CurrentBaseAddress[channel] & 0xFF;
        else {
            res = (DMA_CurrentBaseAddress[channel] & (0xFF << 8))>>8;
            TRACE("Read Current Base Address = %x\n",DMA_CurrentBaseAddress[channel]);
        }
        DMA_Toggle[dmachip] = !DMA_Toggle[dmachip];
        break;

    case 0x01:
    case 0x03:
    case 0x05:
    case 0x07:
    case 0xC2:
    case 0xC6:
    case 0xCA:
    case 0xCE:
        /* Count*/
        channel = ((port-1)&0xC0)?(((port-1)-0xC0)>>2):(port>>1);
        dmachip = (channel<4) ? 0 : 1;
        if (!DMA_Toggle[dmachip])
            res = DMA_CurrentByteCount[channel];
        else {
            res = DMA_CurrentByteCount[channel] >> 8;
            TRACE("Read Current Count = %x.\n",DMA_CurrentByteCount[channel]);
        }
        DMA_Toggle[dmachip] = !DMA_Toggle[dmachip];
        break;

    /* Low Page Base Address */
    case 0x87: res = DMA_BaseAddress[0] >> 16; break;
    case 0x83: res = DMA_BaseAddress[1] >> 16; break;
    case 0x81: res = DMA_BaseAddress[2] >> 16; break;
    case 0x82: res = DMA_BaseAddress[3] >> 16; break;
    case 0x8B: res = DMA_BaseAddress[5] >> 16; break;
    case 0x89: res = DMA_BaseAddress[6] >> 16; break;
    case 0x8A: res = DMA_BaseAddress[7] >> 16; break;

    /* High Page Base Address */
    case 0x487: res = DMA_BaseAddress[0] >> 24; break;
    case 0x483: res = DMA_BaseAddress[1] >> 24; break;
    case 0x481: res = DMA_BaseAddress[2] >> 24; break;
    case 0x482: res = DMA_BaseAddress[3] >> 24; break;
    case 0x48B: res = DMA_BaseAddress[5] >> 24; break;
    case 0x489: res = DMA_BaseAddress[6] >> 24; break;
    case 0x48A: res = DMA_BaseAddress[7] >> 24; break;

    case 0x08:
    case 0xD0:
        /* Status */
        TRACE("Status Register Read\n");
        res = DMA_Status[(port==0x08)?0:1];
        break;

    case 0x0D:
    case 0xDA:
        /* Temporary */
        FIXME("Temporary Register Read- Not Implemented\n");
        break;
    }
    return res;
}
