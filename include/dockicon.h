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

#ifndef __DOCKICON_H__
#define __DOCKICON_H__

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_PORTS_H
#include <exec/ports.h>
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef WORKBENCH_WORKBENCH_H
#include <workbench/workbench.h>
#endif

/*
** AppIcon-shaped dock icons.
**
** C clients call TD_AddDockIconA() on tooldock.library. That FindPorts
** this name and PutMsgs a DockIconMsg. The same public port is also the
** ARexx host (ADDRESS TOOLDOCK). FindPort is case-insensitive.
**
** Events come back as AppMessage on the MsgPort you passed in, same
** AMTYPE_APPICON / AMCLASSICON_* values as workbench.library/AddAppIconA.
** Use TD_RemoveDockIcon(), not RemoveAppIcon().
*/

#define TD_PORT_NAME            "ToolDock"

#define DIM_MAGIC               0x54444943UL    /* 'TDIC' */

#define DIMCMD_ADD              1
#define DIMCMD_REMOVE           2
#define DIMCMD_REDRAW           3

/* Opaque handle. Do not pass to RemoveAppIcon(). */
struct DockIcon;

struct DockIconMsg {
    struct Message dim_Message;
    ULONG dim_Magic;
    ULONG dim_Command;
    LONG dim_Error;
    ULONG dim_ID;
    ULONG dim_UserData;
    STRPTR dim_Text;
    struct MsgPort *dim_AppPort;
    BPTR dim_Lock;
    struct DiskObject *dim_DiskObj;
    struct TagItem *dim_Tags;
    struct DockIcon *dim_Icon;
};

#endif
