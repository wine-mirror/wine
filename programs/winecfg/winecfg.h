/*
 * WineCfg configuration management
 *
 * Copyright 2002 Jaco Greeff
 * Copyright 2003 Dimitrie O. Paun
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
 *
 */

#ifndef WINE_CFG_H
#define WINE_CFG_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "properties.h"

#define IS_OPTION_TRUE(ch) \
    ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')
#define IS_OPTION_FALSE(ch) \
    ((ch) == 'n' || (ch) == 'N' || (ch) == 'f' || (ch) == 'F' || (ch) == '0')

#define return_if_fail(try, ret) \
    if (!(try)) { \
	WINE_ERR("assertion (##try) failed, returning\n"); \
	return ret; \
    }

/* Transaction management */
enum transaction_action {
    ACTION_SET,
    ACTION_REMOVE
};

struct transaction {
    char *section;
    char *key;
    char *newValue;
    enum transaction_action action;
    struct transaction *next, *prev;
};
extern struct transaction *tqhead, *tqtail;

extern int instantApply; /* non-zero means apply all changes instantly */


/* Commits a transaction to the registry */
void processTransaction(struct transaction *trans);

/* Processes every pending transaction in the queue, removing them as it works from head to tail */
void processTransQueue();

/* Adds a transaction to the head of the queue. If we're using instant apply, this calls processTransaction
 * action can be either:
 *   ACTION_SET -> this transaction will change a registry key, newValue is the replacement value
 *   ACTION_REMOVE -> this transaction will remove a registry key. In this case, newValue is ignored.
 */
void addTransaction(char *section, char *key, enum transaction_action action, char *newValue);

/* frees the transaction structure, all fields, and removes it from the queue if in it */
void destroyTransaction(struct transaction *trans);

/* Initializes the transaction system */
int initialize(void);
extern HKEY configKey;

int setConfigValue (char *subkey, char *valueName, const char *value);
char *getConfigValue (char *subkey, char *valueName, char *defaultResult);
HRESULT doesConfigValueExist (char *subkey, char *valueName);
HRESULT removeConfigValue (char *subkey, char *valueName);

/* X11DRV */

void initX11DrvDlg (HWND hDlg);
void saveX11DrvDlgSettings (HWND hDlg);
INT_PTR CALLBACK X11DrvDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* Drive management */
void initDriveDlg (HWND hDlg);
void saveDriveSettings (HWND hDlg);

INT_PTR CALLBACK DriveDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DriveEditDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define WINE_KEY_ROOT "Software\\Wine\\WineCfg\\Config"

#endif
