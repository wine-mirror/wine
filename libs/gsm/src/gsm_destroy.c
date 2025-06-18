/*
 * Copyright 1992 by Jutta Degener and Carsten Bormann, Technische
 * Universitaet Berlin.  See the accompanying file "COPYRIGHT" for
 * details.  THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 */

/* $Header: /tmp_amd/presto/export/kbs/jutta/src/gsm/RCS/gsm_destroy.c,v 1.3 1994/11/28 19:52:25 jutta Exp $ */

#include "gsm.h"
#include "proto.h"
#include <stdlib.h>

void gsm_destroy P1((S), gsm S)
{
	if (S) free((char *)S);
}
