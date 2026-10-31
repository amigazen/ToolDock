/************************************
**
**  ToolDock - A Dock For AmigaOS 3
**
**  SPDX-License-Identifier: MIT
**
**  ToolDock changes Copyright 2026 amigazen project
**  DockBot Copyright 2019 Andrew Kennan
**
************************************/

#include "dock.h"

#include <exec/libraries.h>
#include <utility/tagitem.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <clib/wb_protos.h>
#include <clib/icon_protos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

extern struct Library *IconBase;
extern struct Library *WorkbenchBase;

#ifndef ICONA_Dummy
#define ICONA_Dummy (TAG_USER + 0x9000)
#endif
#ifndef ICONA_ErrorCode
#define ICONA_ErrorCode (ICONA_Dummy + 1)
#endif
#ifndef ICONGETA_FailIfUnavailable
#define ICONGETA_FailIfUnavailable (ICONA_Dummy + 47)
#endif
#ifndef WBAPPICONA_SupportsOpen
#define WBAPPICONA_SupportsOpen (TAG_USER + 0xA001)
#endif
#ifndef WBAPPICONA_SupportsInformation
#define WBAPPICONA_SupportsInformation (TAG_USER + 0xA004)
#endif
#ifndef AMCLASSICON_Open
#define AMCLASSICON_Open 0
#endif
#ifndef AMCLASSICON_Information
#define AMCLASSICON_Information 3
#endif
#ifndef AMCLASSICON_Selected
#define AMCLASSICON_Selected 11
#endif
#ifndef AMCLASSICON_Unselected
#define AMCLASSICON_Unselected 12
#endif

#pragma libcall IconBase GetIconTagList b4 9802

STATIC VOID drain_app_port(struct MsgPort *port)
{
    struct Message *msg;

    if( port == NULL ) {
        return;
    }

    while( msg = GetMsg(port) ) {
        ReplyMsg(msg);
    }
}

/* GetIconTagList() / GetDiskObject() / GetDiskObjectNew() append ".info". */
STATIC struct DiskObject *load_app_icon(VOID)
{
    struct DiskObject *icon;
    struct TagItem tags[3];
    LONG error;

    icon = NULL;
    error = 0;

    /* icon.library 44+: GetIconTagList is the current retrieval API.
       FailIfUnavailable FALSE falls back to the identification hook / default
       icon. Do not pass ICONGETA_GetDefaultType here — that tag ignores name. */
    if( IconBase && IconBase->lib_Version >= 44 ) {
        tags[0].ti_Tag = ICONGETA_FailIfUnavailable;
        tags[0].ti_Data = FALSE;
        tags[1].ti_Tag = ICONA_ErrorCode;
        tags[1].ti_Data = (ULONG)&error;
        tags[2].ti_Tag = TAG_DONE;
        icon = GetIconTagList(APP_ICON, tags);
        if( icon == NULL ) {
            DEBUG(DB_Printf("load_app_icon: GetIconTagList failed, IoErr %ld\n", error));
        }
    }

    if( icon == NULL ) {
        icon = GetDiskObjectNew(APP_ICON);
    }

    if( icon == NULL ) {
        icon = GetDefDiskObject(WBTOOL);
    }

    if( icon == NULL ) {
        DEBUG(DB_Printf("load_app_icon: no DiskObject for %s.info\n", APP_ICON));
        return NULL;
    }

    /* AddAppIconA recommends NO_ICON_POSITION so Workbench places the icon. */
    icon->do_CurrentX = NO_ICON_POSITION;
    icon->do_CurrentY = NO_ICON_POSITION;
    return icon;
}

STATIC struct AppIcon *add_app_icon(struct DockWindow *dock)
{
    struct TagItem tags[3];
    struct TagItem *tagList;

    tagList = NULL;

    /* WBAPPICONA_* is workbench.library 44+. Older WB ignores unknown tags
       only when a taglist is passed; keep NULL on pre-44. */
    if( WorkbenchBase && WorkbenchBase->lib_Version >= 44 ) {
        tags[0].ti_Tag = WBAPPICONA_SupportsOpen;
        tags[0].ti_Data = TRUE;
        tags[1].ti_Tag = WBAPPICONA_SupportsInformation;
        tags[1].ti_Data = TRUE;
        tags[2].ti_Tag = TAG_DONE;
        tagList = tags;
    }

    return AddAppIconA(0L, 0L, APP_NAME, dock->aiPort, NULL, dock->iconObj, tagList);
}

BOOL show_app_icon(struct DockWindow *dock)
{
    DEBUG(DB_Printf("show_app_icon\n"));

    if( dock->iconObj = load_app_icon() ) {

        if( dock->aiPort = CreateMsgPort() ) {

            if( dock->appIcon = add_app_icon(dock) ) {
                return TRUE;
            }
            DEBUG(DB_Printf("show_app_icon: AddAppIconA failed\n"));
            drain_app_port(dock->aiPort);
            delete_port(dock->aiPort);
        } else {
            DEBUG(DB_Printf("show_app_icon: CreateMsgPort failed\n"));
        }

        FreeDiskObject(dock->iconObj);
    }
    dock->appIcon = NULL;
    dock->iconObj = NULL;
    dock->aiPort = NULL;
    return FALSE;
}

VOID free_app_icon(struct DockWindow *dock)
{
    DEBUG(DB_Printf("free_app_icon\n"));

    /* RemoveAppIcon: drain the AppMessage port before and after. */
    drain_app_port(dock->aiPort);

    if( dock->appIcon ) {
        RemoveAppIcon(dock->appIcon);
        dock->appIcon = NULL;
        drain_app_port(dock->aiPort);
    }

    delete_port(dock->aiPort);
    dock->aiPort = NULL;

    if( dock->iconObj ) {
        FreeDiskObject(dock->iconObj);
        dock->iconObj = NULL;
    }
}

VOID handle_icon_event(struct DockWindow* dock)
{
    struct AppMessage *msg;
    struct WBArg *arg;
    ULONG i;
    ULONG first;

    if( dock->aiPort == NULL ) {
        return;
    }

    while( msg = (struct AppMessage *)GetMsg(dock->aiPort)) {
        if( msg->am_Type == AMTYPE_APPICON ) {

            if( msg->am_Class == AMCLASSICON_Information ) {
                open_dock_prefs(dock);
            } else if( msg->am_Class != AMCLASSICON_Selected
                    && msg->am_Class != AMCLASSICON_Unselected ) {

                /* Open menu, double-click, or icons dropped on the AppIcon.
                   wb.doc: drop NumArgs is dropped icons plus one (the AppIcon). */
                if( msg->am_NumArgs > 0 && msg->am_ArgList ) {
                    arg = msg->am_ArgList;
                    first = (msg->am_NumArgs > 1) ? 1 : 0;
                    for( i = first; i < msg->am_NumArgs; i++ ) {
                        add_dropped_icon(dock, arg[i].wa_Lock, arg[i].wa_Name);
                    }
                }
                dock->runState = RS_UNICONIFYING;
            }
        }

        ReplyMsg((struct Message*)msg);
    }
}
