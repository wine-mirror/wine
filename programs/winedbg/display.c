/*
 * File display.c - display handling for Wine internal debugger.
 *
 * Copyright (C) 1997, Eric Youngdale.
 * Copyright (C) 2003, Michal Miroslaw
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>

#include "debugger.h"

#include <stdarg.h>

#define DISPTAB_DELTA 8		/* needs to be power of 2, search for MARK to see why :) */

struct display {
	struct expr *exp;
	int count;
	char format;
	char enabled;
	struct name_hash *function_name;
};

static struct display *displaypoints = NULL;
static unsigned int maxdisplays = 0, ndisplays = 0;

static struct name_hash *DEBUG_GetCurrentFrameFunctionName(void)
{
	struct name_hash *name;
	unsigned int eip, ebp;

	if (DEBUG_GetCurrentFrame(&name, &eip, &ebp))
		return name;
	return NULL;
}

int DEBUG_AddDisplay(struct expr *exp, int count, char format, int in_frame)
{
	int i;

	for (i = 0; i < ndisplays; i++)
		if (displaypoints[i].exp == NULL)
			break;

	if (i == maxdisplays)	/* no space left - expand */
		displaypoints = DBG_realloc(displaypoints,
			(maxdisplays += DISPTAB_DELTA) * sizeof(*displaypoints));

	if (i == ndisplays)
		++ndisplays;

	displaypoints[i].exp = DEBUG_CloneExpr(exp);
	displaypoints[i].count = count;
	displaypoints[i].format = format;
	displaypoints[i].enabled = TRUE;
	displaypoints[i].function_name =
		(in_frame ? DEBUG_GetCurrentFrameFunctionName() : NULL);

	return TRUE;
}

int DEBUG_InfoDisplay(void)
{
	int i;

	for (i = 0; i < ndisplays; i++) {
		if (displaypoints[i].exp == NULL)
			continue;

		if (displaypoints[i].function_name)
			DEBUG_Printf("%d in %s%s : ", i + 1,
				DEBUG_GetSymbolName(displaypoints[i].function_name),
				(displaypoints[i].enabled ?
					(displaypoints[i].function_name != DEBUG_GetCurrentFrameFunctionName() ?
						" (out of scope)" : "")
					: " (disabled)")
				);
		else
			DEBUG_Printf("%d%s : ", i + 1,
                                     (displaypoints[i].enabled ? "" : " (disabled)"));
		DEBUG_DisplayExpr(displaypoints[i].exp);
		DEBUG_Printf("\n");
	}

	return TRUE;
}

void DEBUG_PrintOneDisplay(int i)
{
	DBG_VALUE value;

	if (displaypoints[i].enabled) {
		value = DEBUG_EvalExpr(displaypoints[i].exp);
		if (value.type == NULL) {
			DEBUG_Printf("Unable to evaluate expression ");
			DEBUG_DisplayExpr(displaypoints[i].exp);
			DEBUG_Printf("\nDisabling display %d ...\n", i + 1);
			displaypoints[i].enabled = FALSE;
			return;
		}
	}

	DEBUG_Printf("%d  : ", i + 1);
	DEBUG_DisplayExpr(displaypoints[i].exp);
	DEBUG_Printf(" = ");
	if (!displaypoints[i].enabled)
		DEBUG_Printf("(disabled)\n");
	else
	if (displaypoints[i].format == 'i')
		DEBUG_ExamineMemory(&value, displaypoints[i].count, displaypoints[i].format);
	else
		DEBUG_Print(&value, displaypoints[i].count, displaypoints[i].format, 0);
}

int DEBUG_DoDisplay(void)
{
	int i;
	struct name_hash *cur_function_name = DEBUG_GetCurrentFrameFunctionName();

	for (i = 0; i < ndisplays; i++) {
		if (displaypoints[i].exp == NULL || !displaypoints[i].enabled)
			continue;
		if (displaypoints[i].function_name
		    && displaypoints[i].function_name != cur_function_name)
			continue;
		DEBUG_PrintOneDisplay(i);
	}

	return TRUE;
}

int DEBUG_DelDisplay(int displaynum)
{
	int i;

	if (displaynum > ndisplays || displaynum == 0 || displaynum < -1
	    || displaypoints[displaynum - 1].exp == NULL) {
		DEBUG_Printf("Invalid display number\n");
		return TRUE;
	}

	if (displaynum == -1) {
		for (i = 0; i < ndisplays; i++) {
			if (displaypoints[i].exp != NULL) {
				DEBUG_FreeExpr(displaypoints[i].exp);
				displaypoints[i].exp = NULL;
			}
		}
		displaypoints = DBG_realloc(displaypoints,
			(maxdisplays = DISPTAB_DELTA) * sizeof(*displaypoints));
		ndisplays = 0;
	} else if (displaypoints[--displaynum].exp != NULL) {
		DEBUG_FreeExpr(displaypoints[displaynum].exp);
		displaypoints[displaynum].exp = NULL;
		while (displaynum == ndisplays - 1
		    && displaypoints[displaynum].exp == NULL)
			--ndisplays, --displaynum;
		if (maxdisplays - ndisplays >= 2 * DISPTAB_DELTA) {
			maxdisplays = (ndisplays + DISPTAB_DELTA - 1) & ~(DISPTAB_DELTA - 1); /* MARK */
			displaypoints = DBG_realloc(displaypoints,
				maxdisplays * sizeof(*displaypoints));
		}
	}
	return TRUE;
}

int DEBUG_EnableDisplay(int displaynum, int enable)
{
	--displaynum;
	if (displaynum >= ndisplays || displaynum < 0 || displaypoints[displaynum].exp == NULL) {
		DEBUG_Printf("Invalid display number\n");
		return TRUE;
	}

	displaypoints[displaynum].enabled = enable;
	if (!displaypoints[displaynum].function_name
	    || displaypoints[displaynum].function_name == DEBUG_GetCurrentFrameFunctionName())
		DEBUG_PrintOneDisplay(displaynum);

	return TRUE;
}
