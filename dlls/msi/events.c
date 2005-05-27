/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Aric Stewart for CodeWeavers
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


/*
http://msdn.microsoft.com/library/default.asp?url=/library/en-us/msi/setup/controlevent_overview.asp
*/

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "msi.h"
#include "msipriv.h"
#include "action.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef void (*EVENTHANDLER)(MSIPACKAGE*,LPCWSTR,msi_dialog *);

struct _events {
    LPSTR event;
    EVENTHANDLER handler;
};

struct _subscriber {
    LPWSTR control;
    LPWSTR attribute;
    struct _subscriber *next;
};

struct _subscription_chain {
    LPWSTR event;
    struct _subscriber *chain;
};

struct _subscriptions {
    DWORD chain_count; 
    struct _subscription_chain* chain;
};


VOID ControlEvent_HandleControlEvent(MSIPACKAGE *, LPCWSTR, LPCWSTR, msi_dialog*);

/*
 * Create a dialog box and run it if it's modal
 */
static UINT event_do_dialog( MSIPACKAGE *package, LPCWSTR name )
{
    msi_dialog *dialog;
    UINT r;

    /* kill the current modeless dialog */
    if( package->dialog )
        msi_dialog_destroy( package->dialog );
    package->dialog = NULL;

    /* create a new dialog */
    dialog = msi_dialog_create( package, name,
                                ControlEvent_HandleControlEvent );
    if( dialog )
    {
        /* modeless dialogs return an error message */
        r = msi_dialog_run_message_loop( dialog );
        if( r == ERROR_SUCCESS )
            msi_dialog_destroy( dialog );
        else
            package->dialog = dialog;
    }
    else
        r = ERROR_FUNCTION_FAILED;

    return r;
}


/*
 * End a modal dialog box
 */
static VOID ControlEvent_EndDialog(MSIPACKAGE* package, LPCWSTR argument, 
                                   msi_dialog* dialog)
{
    static const WCHAR szExit[] = {
    'E','x','i','t',0};
    static const WCHAR szRetry[] = {
    'R','e','t','r','y',0};
    static const WCHAR szIgnore[] = {
    'I','g','n','o','r','e',0};
    static const WCHAR szReturn[] = {
    'R','e','t','u','r','n',0};

    if (lstrcmpW(argument,szExit)==0)
        package->CurrentInstallState = ERROR_INSTALL_USEREXIT;
    else if (lstrcmpW(argument, szRetry) == 0)
        package->CurrentInstallState = ERROR_INSTALL_SUSPEND;
    else if (lstrcmpW(argument, szIgnore) == 0)
        package->CurrentInstallState = -1;
    else if (lstrcmpW(argument, szReturn) == 0)
        package->CurrentInstallState = ERROR_SUCCESS;
    else
    {
        ERR("Unknown argument string %s\n",debugstr_w(argument));
        package->CurrentInstallState = ERROR_FUNCTION_FAILED;
    }

    msi_dialog_end_dialog( dialog );
}

/*
 * transition from one modal dialog to another modal dialog
 */
static VOID ControlEvent_NewDialog(MSIPACKAGE* package, LPCWSTR argument, 
                                   msi_dialog *dialog)
{
    /* store the name of the next dialog, and signal this one to end */
    package->next_dialog = strdupW(argument);
    msi_dialog_end_dialog( dialog );
}

/*
 * Create a new child dialog of an existing modal dialog
 */
static VOID ControlEvent_SpawnDialog(MSIPACKAGE* package, LPCWSTR argument, 
                              msi_dialog *dialog)
{
    event_do_dialog( package, argument );
    if( package->CurrentInstallState != ERROR_SUCCESS )
        msi_dialog_end_dialog( dialog );
}

/*
 * Creates a dialog that remains up for a period of time
 * based on a condition
 */
static VOID ControlEvent_SpawnWaitDialog(MSIPACKAGE* package, LPCWSTR argument, 
                                  msi_dialog* dialog)
{
    FIXME("Doing Nothing\n");
}

static VOID ControlEvent_DoAction(MSIPACKAGE* package, LPCWSTR argument, 
                                  msi_dialog* dialog)
{
    ACTION_PerformAction(package,argument);
}

static VOID ControlEvent_AddLocal(MSIPACKAGE* package, LPCWSTR argument, 
                                  msi_dialog* dialog)
{
    static const WCHAR szAll[] = {'A','L','L',0};
    int i;

    if (lstrcmpW(szAll,argument))
    {
        MSI_SetFeatureStateW(package,argument,INSTALLSTATE_LOCAL);
    }
    else
    {
        for (i = 0; i < package->loaded_features; i++)
        {
            package->features[i].ActionRequest = INSTALLSTATE_LOCAL;
            package->features[i].Action = INSTALLSTATE_LOCAL;
        }
        ACTION_UpdateComponentStates(package,argument);
    }

}

static VOID ControlEvent_Remove(MSIPACKAGE* package, LPCWSTR argument, 
                                msi_dialog* dialog)
{
    static const WCHAR szAll[] = {'A','L','L',0};
    int i;

    if (lstrcmpW(szAll,argument))
    {
        MSI_SetFeatureStateW(package,argument,INSTALLSTATE_ABSENT);
    }
    else
    {
        for (i = 0; i < package->loaded_features; i++)
        {
            package->features[i].ActionRequest = INSTALLSTATE_ABSENT;
            package->features[i].Action= INSTALLSTATE_ABSENT;
        }
        ACTION_UpdateComponentStates(package,argument);
    }
}

static VOID ControlEvent_AddSource(MSIPACKAGE* package, LPCWSTR argument, 
                                   msi_dialog* dialog)
{
    static const WCHAR szAll[] = {'A','L','L',0};
    int i;

    if (lstrcmpW(szAll,argument))
    {
        MSI_SetFeatureStateW(package,argument,INSTALLSTATE_SOURCE);
    }
    else
    {
        for (i = 0; i < package->loaded_features; i++)
        {
            package->features[i].ActionRequest = INSTALLSTATE_SOURCE;
            package->features[i].Action = INSTALLSTATE_SOURCE;
        }
        ACTION_UpdateComponentStates(package,argument);
    }
}


/*
 * Subscribed events
 */
static void free_subscriber(struct _subscriber *who)
{
    HeapFree(GetProcessHeap(),0,who->control);
    HeapFree(GetProcessHeap(),0,who->attribute);
    HeapFree(GetProcessHeap(),0,who);
}

VOID ControlEvent_SubscribeToEvent(MSIPACKAGE *package, LPCWSTR event,
                                   LPCWSTR control, LPCWSTR attribute)
{
    int i;
    struct _subscription_chain *chain;
    struct _subscriber *subscriber, *ptr;

    if (!package->EventSubscriptions)
    {
        package->EventSubscriptions = HeapAlloc(GetProcessHeap(), 0,
                                       sizeof(struct _subscriptions));
        package->EventSubscriptions->chain_count = 0;
    }

    chain = NULL;

    for (i = 0; i < package->EventSubscriptions->chain_count; i++)
    {
        if (lstrcmpiW(package->EventSubscriptions->chain[i].event,event)==0)
        {
            chain = &package->EventSubscriptions->chain[i];
            break;
        }
    }

    if (chain == NULL)
    {
        if (package->EventSubscriptions->chain_count)
            chain = HeapReAlloc(GetProcessHeap(), 0,
                                package->EventSubscriptions->chain,
                               (package->EventSubscriptions->chain_count + 1) *
                                sizeof (struct _subscription_chain)); 
        else
            chain= HeapAlloc(GetProcessHeap(),0, sizeof (struct 
                                                        _subscription_chain));

        package->EventSubscriptions->chain = chain;
        chain = &package->EventSubscriptions->chain[
                                package->EventSubscriptions->chain_count];
        package->EventSubscriptions->chain_count++;
        memset(chain,0,sizeof(struct _subscription_chain));
        chain->event = strdupW(event);
    }

    subscriber = ptr = chain->chain;
    while (ptr)
    {
        subscriber = ptr;
        ptr = ptr->next;
    }

    ptr = HeapAlloc(GetProcessHeap(),0,sizeof(struct _subscriber));
    ptr->control = strdupW(control);
    ptr->attribute = strdupW(attribute);
    ptr->next = NULL;
    
    if (subscriber)
        subscriber->next = ptr;
    else
        chain->chain = ptr;
}

VOID ControlEvent_UnSubscribeToEvent(MSIPACKAGE *package, LPCWSTR event,
                                     LPCWSTR control, LPCWSTR attribute)
{
    int i;

    if (!package->EventSubscriptions)
        return;

    for (i = 0; i < package->EventSubscriptions->chain_count; i++)
    {
        if (lstrcmpiW(package->EventSubscriptions->chain[i].event,event)==0)
        {
            struct _subscriber *who;
            struct _subscriber *prev=NULL;
            who = package->EventSubscriptions->chain[i].chain;
            while (who)
            {
                if (lstrcmpiW(who->control,control)==0
                   && lstrcmpiW(who->attribute,attribute)==0)
                {
                    if (prev)
                        prev->next = who->next;
                    else
                        package->EventSubscriptions->chain[i].chain = who->next;
        
                    free_subscriber(who);
                }
                else
                {
                    prev = who;
                    who = who->next;
                }
            }
            break;
        }
    }
}

VOID ControlEvent_FireSubscribedEvent(MSIPACKAGE *package, LPCWSTR event, 
                                      MSIRECORD *data)
{
    int i;

    TRACE("Firing Event %s\n",debugstr_w(event));

    if (!package->dialog)
        return;

    if (!package->EventSubscriptions)
        return;

    for (i = 0; i < package->EventSubscriptions->chain_count; i++)
    {
        if (lstrcmpiW(package->EventSubscriptions->chain[i].event,event)==0)
        {
            struct _subscriber *who;
            who = package->EventSubscriptions->chain[i].chain;
            while (who)
            {
                ERR("Should Fire event for %s %s\n",
                    debugstr_w(who->control), debugstr_w(who->attribute));
                /*
                 msi_dialog_fire_subscribed_event(package->dialog, who->control,
                                                  who->attribute, data);
                */
                who = who->next;
            }
            break;
        }
    }
}

VOID ControlEvent_CleanupSubscriptions(MSIPACKAGE *package)
{
    int i;

    if (!package->EventSubscriptions)
        return;

    for (i = 0; i < package->EventSubscriptions->chain_count; i++)
    {
        struct _subscriber *who;
        struct _subscriber *ptr;
        who = package->EventSubscriptions->chain[i].chain;
        while (who)
        {
            ptr = who;
            who = who->next;
            free_subscriber(ptr);
        }
        HeapFree(GetProcessHeap(), 0,
                    package->EventSubscriptions->chain[i].event);
    }
    HeapFree(GetProcessHeap(),0,package->EventSubscriptions->chain);
    HeapFree(GetProcessHeap(),0,package->EventSubscriptions);
    package->EventSubscriptions = NULL;
}

/*
 * ACTION_DialogBox()
 *
 * Return ERROR_SUCCESS if dialog is process and ERROR_FUNCTION_FAILED
 * if the given parameter is not a dialog box
 */
UINT ACTION_DialogBox( MSIPACKAGE* package, LPCWSTR szDialogName )
{
    UINT r = ERROR_SUCCESS;

    if( package->next_dialog )
        ERR("Already a next dialog... ignoring it\n");
    package->next_dialog = NULL;

    /*
     * Dialogs are chained by filling in the next_dialog member
     *  of the package structure, then terminating the current dialog.
     *  The code below sees the next_dialog member set, and runs the
     *  next dialog.
     * We fall out of the loop below if we come across a modeless
     *  dialog, as it returns ERROR_IO_PENDING when we try to run
     *  its message loop.
     */
    r = event_do_dialog( package, szDialogName );
    while( r == ERROR_SUCCESS && package->next_dialog )
    {
        LPWSTR name = package->next_dialog;

        package->next_dialog = NULL;
        r = event_do_dialog( package, name );
        HeapFree( GetProcessHeap(), 0, name );
    }

    if( r == ERROR_IO_PENDING )
        r = ERROR_SUCCESS;

    return r;
}

struct _events Events[] = {
    { "EndDialog",ControlEvent_EndDialog },
    { "NewDialog",ControlEvent_NewDialog },
    { "SpawnDialog",ControlEvent_SpawnDialog },
    { "SpawnWaitDialog",ControlEvent_SpawnWaitDialog },
    { "DoAction",ControlEvent_DoAction },
    { "AddLocal",ControlEvent_AddLocal },
    { "Remove",ControlEvent_Remove },
    { "AddSource",ControlEvent_AddSource },
    { NULL,NULL },
};

VOID ControlEvent_HandleControlEvent(MSIPACKAGE *package, LPCWSTR event,
                                     LPCWSTR argument, msi_dialog* dialog)
{
    int i = 0;

    TRACE("Handling Control Event %s\n",debugstr_w(event));
    if (!event)
        return;

    while( Events[i].event != NULL)
    {
        LPWSTR wevent = strdupAtoW(Events[i].event);
        if (lstrcmpW(wevent,event)==0)
        {
            HeapFree(GetProcessHeap(),0,wevent);
            Events[i].handler(package,argument,dialog);
            return;
        }
        HeapFree(GetProcessHeap(),0,wevent);
        i++;
    }
    FIXME("unhandled control event %s arg(%s)\n",
          debugstr_w(event), debugstr_w(argument));
}
