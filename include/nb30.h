#ifndef NCB_INCLUDED
#define NCB_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define NCBNAMSZ 16
#define MAX_LANA 0xfe

#define NCBRESET       0x32
#define NCBADDNAME     0x30
#define NCBADDGRNAME   0x36
#define NCBDELNAME     0x31
#define NCBSEND        0x14
#define NCBRECV        0x15
#define NCBHANGUP      0x12
#define NCBCANCEL      0x35
#define NCBLISTEN      0x11
#define NCBCALL        0x10
#define NCBASTAT       0x33
#define NCBENUM        0x37

#include "pshpack1.h"

typedef struct _NCB
{
	UCHAR	ncb_command;
	UCHAR	ncb_retcode;
	UCHAR	ncb_lsn;
	UCHAR	ncb_num;
	PUCHAR	ncb_buffer;
	WORD	ncb_length;
	UCHAR	ncb_callname[NCBNAMSZ];
	UCHAR	ncb_name[NCBNAMSZ];
	UCHAR	ncb_rto;
	UCHAR	ncb_sto;
	VOID	(*ncb_post)(struct _NCB *);
	UCHAR	ncb_lana_num;
	UCHAR	ncb_cmd_cplt;
	UCHAR	ncb_reserved[10];
	HANDLE	ncb_event;
} NCB, *PNCB;

typedef struct _ADAPTER_STATUS
{
	UCHAR   adapter_address[6];
	UCHAR	rev_major;
	UCHAR	reserved0;
	UCHAR	adapter_type;
	UCHAR	rev_minor;
	WORD	duration;
	WORD	frmr_recv;
	WORD	frmr_xmit;
	WORD	iframe_recv_error;
	WORD	xmit_aborts;
	WORD	xmit_success;
	WORD	recv_success;
	WORD	iframe_xmit_error;
	WORD	recv_buffer_unavail;
	WORD	t1_timeouts;
	WORD	ti_timeouts;
	DWORD	reserved1;
	WORD	free_ncbs;
	WORD	max_cfg_ncbs;
	WORD	max_ncbs;
	WORD	xmit_buf_unavail;
	WORD	max_dgram_size;
	WORD	pending_sess;
	WORD	max_cfg_sess;
	WORD	max_sess;
	WORD	max_sess_pktsize;
	WORD	name_count;
} ADAPTER_STATUS, *PADAPTER_STATUS;

typedef struct _LANA_ENUM
{
	UCHAR length;
	UCHAR lana[MAX_LANA+1];
} LANA_ENUM, *PLANA_ENUM;

#define NRC_GOODRET 0x00
#define NRC_BUFLEN  0x01
#define NRC_ILLCMD  0x03
#define NRC_CMDTMO  0x05
#define NRC_INCOMP  0x06
#define NRC_INVADDRESS 0x39
#define NRC_PENDING 0xff
#define NRC_OPENERROR 0x3f

#ifdef __cplusplus
}
#endif

#endif  /* NCB_INCLUDED */
