
/*
 *  Cdrom - device driver emulation - Audio features.
 *  (c) 1998 Petr Tomasek <tomasek@etf.cuni.cz>
 *
 */

#ifdef linux

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
/* FIXME - how to make this OS independent ?? */
#include <linux/cdrom.h>
#include <linux/ucdrom.h>

#include "ldt.h"
#include "drive.h"
#include "msdos.h"
#include "miscemu.h"
#include "module.h"
/* #define DEBUG_INT */
#include "debug.h"


/*  FIXME - more general ?? */
#define cdrom_dev "/dev/cdrom"

u_char cdrom_a_status (int fd)
{
struct cdrom_subchnl sc;

ioctl(fd,CDROMSUBCHNL,&sc);
return sc.cdsc_audiostatus;
}

BYTE * get_io_stru (WORD * reqh,int dorealmode)
{
WORD ofst,segm;
BYTE * io_stru;
ofst = reqh[7]; segm = reqh[8];

if (dorealmode)
  io_stru = DOSMEM_MapRealToLinear (MAKELONG(ofst,segm));
else
  io_stru = PTR_SEG_OFF_TO_LIN(segm,ofst);

return io_stru;
}

DWORD msf0_to_abs (struct cdrom_msf0 msf)
{
return  (msf.minute *60 + 
        msf.second) *75 +
        msf.frame-150; 
}

void abs_to_msf0 (DWORD abs, struct cdrom_msf0 * msf)
{
DWORD d;
d=abs+150;
msf->frame=d%75;  d=d/75;
msf->second=d%60; msf->minute=d/60;
}

void msf0_to_msf (struct cdrom_msf0 from, struct cdrom_msf0 to, struct cdrom_msf * msf)
{
msf->cdmsf_min0=from.minute;
msf->cdmsf_min1=to.minute;
msf->cdmsf_sec0=from.second;
msf->cdmsf_sec1=to.second;
msf->cdmsf_frame0=from.frame;
msf->cdmsf_frame1=to.frame;
} 

void abs_to_msf (DWORD from, DWORD to, struct cdrom_msf * msf)
{
struct cdrom_msf0 fr,tt;
abs_to_msf0(from, &fr);
abs_to_msf0(to, &tt);
msf0_to_msf(fr,tt,msf);
}

/************************************************************
 *
 *   Cdrom ms-dos driver emulation. 
 *    (accesible throught the MSCDEX 0x10 function.)
 */

extern void do_mscdex_dd (CONTEXT * context, int dorealmode)
 { 
 BYTE * driver_request;	
 BYTE * io_stru;	
 static int fdcd=-1; /* file descriptor.. */
 struct cdrom_tochdr tochdr; /* the Toc header */
 struct cdrom_tocentry tocentry; /* a Toc entry */
 struct cdrom_msf msf; 
 struct cdrom_subchnl subchnl; 
 u_char Error=255; /*No Error */ 

if (dorealmode)
  driver_request=DOSMEM_MapRealToLinear
                        (MAKELONG(BX_reg(context),ES_reg(context)));
else
  driver_request=PTR_SEG_OFF_TO_LIN(ES_reg(context),BX_reg(context));

if (!driver_request) 
     {             /* FIXME - to be deleted ?? */
	ERR(int,"   ES:BX==0 ! SEGFAULT ?\n");
	ERR(int," -->BX=0x%04X, ES=0x%04X, DS=0x%04X, CX=0x%04X\n\n",
		BX_reg(context),
		ES_reg(context),
		DS_reg(context),
		CX_reg(context));
     }
else

/* FIXME - would be best to open the device at the begining of the wine
   session .... */
 if (fdcd<0) fdcd=open(cdrom_dev,O_RDONLY);

   TRACE(int,"CDROM device driver -> command <%d>\n",
         (unsigned char)driver_request[2]);

/* set status to 0 */
 driver_request[3]=0;
 driver_request[4]=0;


 switch(driver_request[2])
 {
 case 3:
   io_stru=get_io_stru((WORD *)driver_request,dorealmode);
   FIXME(int," --> IOCTL INPUT <%d>\n",io_stru[0]); 
   switch (io_stru[0])
	{
	case 1: /* location of head */
	 if (io_stru[1]==0)
          {
          ioctl(fdcd,CDROMSUBCHNL,&subchnl);
	((DWORD*)io_stru+2)[0]=(DWORD)msf0_to_abs(subchnl.cdsc_absaddr.msf); 
         FIXME(int," ----> HEAD LOCATION <%ld>\n\n", 
                                ((DWORD *)io_stru+2)[0]); 
	  }
         else
     	  {
 	 ERR(int,"CDRom-Driver: Unsupported address mode !!\n");
   	 Error=0x0c;
    	  }
	 break;

	case 6: /* device status */
              /* FIXME .. does this work properly ?? */
	 io_stru[3]=io_stru[4]=0;
	 io_stru[2]=1; /* supports audio channels (?? FIXME ??) */
	 io_stru[1]=16; /* data read and plays audio racks */
	 io_stru[1]|=(ioctl(fdcd,CDROM_DRIVE_STATUS,0)==CDS_TRAY_OPEN);
         TRACE(int," ----> DEVICE STATUS <0x%08X>\n\n",(DWORD)io_stru[1]); 
	 break;

	case 9: /* media changed ? */
	 if (ioctl(fdcd,CDROM_MEDIA_CHANGED,0))
          io_stru[1]=0xff;
         else
	  io_stru[0]=0; /* FIXME? 1? */
	 break;

	case 10: /* audio disk info */
         ioctl(fdcd,CDROMREADTOCHDR,&tochdr);
	 io_stru[1]=tochdr.cdth_trk0; /* staring track of the disc */
	 io_stru[2]=tochdr.cdth_trk1; /* ending track */
         tocentry.cdte_track=CDROM_LEADOUT; /* Now the leadout track ...*/
         tocentry.cdte_format=CDROM_MSF; 
         ioctl(fdcd,CDROMREADTOCENTRY,&tocentry); /* ... get position of it */
	 ((DWORD*)io_stru+3)[0]=(DWORD)msf0_to_abs(tocentry.cdte_addr.msf); 
         TRACE(int," ----> AUDIO DISK INFO <%d-%d/%ld>\n\n",
				io_stru[1],io_stru[2],
                                ((DWORD *)io_stru+3)[0]); 
	 break;


	case 11: /* audio track info */
	 tocentry.cdte_track=io_stru[1]; /* track of the disc */ 
         tocentry.cdte_format=CDROM_MSF; 
         ioctl(fdcd,CDROMREADTOCENTRY,&tocentry);
	 ((DWORD*)io_stru+2)[0]=(DWORD)msf0_to_abs(tocentry.cdte_addr.msf); 
                        /* starting point if the track */
         io_stru[6]=tocentry.cdte_adr+16*tocentry.cdte_ctrl;
         TRACE(int," ----> AUDIO TRACK INFO <track=%d>[%ld] \n\n",
                         io_stru[1],((DWORD *)io_stru+2)[0]); 
	 break;

	case 12: /* get Q-Channel / Subchannel (??) info */
	 subchnl.cdsc_format=CDROM_MSF;
	 ioctl(fdcd,CDROMSUBCHNL,&subchnl);
	 io_stru[1]=subchnl.cdsc_adr+16*subchnl.cdsc_ctrl;
	 io_stru[2]=subchnl.cdsc_trk;
	 io_stru[3]=subchnl.cdsc_ind; /* FIXME - ?? */
	 io_stru[4]=subchnl.cdsc_reladdr.msf.minute;
	 io_stru[5]=subchnl.cdsc_reladdr.msf.second;
	 io_stru[6]=subchnl.cdsc_reladdr.msf.frame;
	 io_stru[7]=0; /* always zero */
	 io_stru[8]=subchnl.cdsc_absaddr.msf.minute;
	 io_stru[9]=subchnl.cdsc_absaddr.msf.second;
	 io_stru[10]=subchnl.cdsc_absaddr.msf.frame;
	 break;
	
	case 15: /* fixme !!!!!!! just a small workaround ! */
	 /* !!!! FIXME FIXME FIXME !! */
         tocentry.cdte_track=CDROM_LEADOUT; /* Now the leadout track ...*/
         tocentry.cdte_format=CDROM_MSF; 
         ioctl(fdcd,CDROMREADTOCENTRY,&tocentry); /* ... get position of it */
	 ((DWORD*)io_stru+7)[0]=(DWORD)msf0_to_abs(tocentry.cdte_addr.msf); 
 	break;

	default:
         FIXME(int," Cdrom-driver: IOCTL INPUT: Unimplemented <%d>!! \n",
                                                             io_stru[0]); 
	 Error=0x0c; 
	 break;	
	}	
   break;

 case 12:
   io_stru=get_io_stru((WORD *)driver_request,dorealmode);
   TRACE(int," --> IOCTL OUTPUT <%d>\n",io_stru[0]); 
   switch (io_stru[0])
	{
	case 0: /* eject */ 
	 ioctl (fdcd,CDROMEJECT);
         TRACE(int," ----> EJECT \n\n"); 
	 break;
	case 5: /* close tray */
	 ioctl (fdcd,CDROMCLOSETRAY);
         TRACE(int," ----> CLOSE TRAY \n\n"); 
	 break;
	case 2: /* reset drive */
	 ioctl (fdcd,CDROMRESET);
         TRACE(int," ----> RESET \n\n"); 
	 break;
	default:
         FIXME(int," Cdrom-driver: IOCTL OUPUT: Unimplemented <%d>!! \n",
                                                             io_stru[0]); 
	 Error=0x0c;
	 break;	
	}	
   break;
 
case 133:
   if (cdrom_a_status(fdcd)==CDROM_AUDIO_PLAY)
     {
     ioctl (fdcd,CDROMPAUSE);
     TRACE(int," --> STOP AUDIO (Paused)\n\n");
     }
   else
     {
     ioctl (fdcd,CDROMSTOP);
     TRACE(int," --> STOP AUDIO (Stopped)\n\n");
     }
   break;

 case 132:  /* FIXME - It didn't function for me... */
   FIXME(int," --> PLAY AUDIO \n");
     ioctl (fdcd,CDROMSTART);
   FIXME(int,"Mode :<0x%02X> , [%ld-%ld]\n\n",
          (unsigned char)driver_request[13],
          ((DWORD*)driver_request+14)[0],
          ((DWORD*)driver_request+18)[0]);
   if (driver_request[13]==0)
     {
    abs_to_msf(((DWORD*)driver_request+14)[0],
               ((DWORD*)driver_request+18)[0],&msf);
    ioctl(fdcd,CDROMPLAYMSF,&msf);
     }
   else 
     {
    ERR(int,"CDRom-Driver: Unsupported address mode !!\n");
    Error=0x0c;
     }
   break;
 case 136:
   TRACE(int," --> RESUME AUDIO \n\n");
   ioctl(fdcd,CDROMRESUME);
   break;
 default:
   FIXME(int," CDRom-Driver - ioctl uninplemented <%d>\n",driver_request[2]); 
   Error=0x0c;	

 }

if (Error<255)
 {
 driver_request[4]|=127;
 driver_request[3]=Error;
 }
 driver_request[4]|=2*(cdrom_a_status(fdcd)==CDROM_AUDIO_PLAY);

/*  close (fdcd); FIXME !! -- cannot use close when ejecting 
    the cd-rom - close would close it again */ 
}

#endif

