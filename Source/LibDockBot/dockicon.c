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

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/tasks.h>

#include <clib/exec_protos.h>
#include <pragmas/exec_pragmas.h>

#include "dockbot.h"
#include "dockbot_protos.h"
#include "dockicon.h"

STATIC BOOL send_dockicon_msg(struct DockIconMsg *msg)
{
    struct MsgPort *port;
    struct MsgPort *reply;

    reply = CreateMsgPort();
    if( reply == NULL ) {
        return FALSE;
    }

    msg->dim_Magic = DIM_MAGIC;
    msg->dim_Error = -1;
    msg->dim_Message.mn_Node.ln_Type = NT_MESSAGE;
    msg->dim_Message.mn_Length = (UWORD)sizeof(struct DockIconMsg);
    msg->dim_Message.mn_ReplyPort = reply;

    Forbid();
    port = FindPort((STRPTR)TD_PORT_NAME);
    if( port ) {
        PutMsg(port, (struct Message *)msg);
    }
    Permit();

    if( port == NULL ) {
        DeleteMsgPort(reply);
        return FALSE;
    }

    WaitPort(reply);
    GetMsg(reply);
    DeleteMsgPort(reply);
    return TRUE;
}

struct DockIcon * __asm __saveds TD_AddDockIconA(
    register __d0 ULONG id,
    register __d1 ULONG userdata,
    register __a0 STRPTR text,
    register __a1 struct MsgPort *msgport,
    register __a2 BPTR lock,
    register __a3 struct DiskObject *diskobj,
    register __a4 struct TagItem *taglist)
{
    struct DockIconMsg *msg;
    struct DockIcon *icon;
    struct TagItem *tags;

    tags = taglist;
    icon = NULL;

    if( msgport == NULL || diskobj == NULL ) {
        return NULL;
    }

    msg = (struct DockIconMsg *)AllocMem(sizeof(struct DockIconMsg),
        MEMF_PUBLIC | MEMF_CLEAR);
    if( msg == NULL ) {
        return NULL;
    }

    msg->dim_Command = DIMCMD_ADD;
    msg->dim_ID = id;
    msg->dim_UserData = userdata;
    msg->dim_Text = text;
    msg->dim_AppPort = msgport;
    msg->dim_Lock = lock;
    msg->dim_DiskObj = diskobj;
    msg->dim_Tags = tags;

    if( send_dockicon_msg(msg) && msg->dim_Error == 0 ) {
        icon = msg->dim_Icon;
    }

    FreeMem(msg, sizeof(struct DockIconMsg));
    return icon;
}

BOOL __asm __saveds TD_RemoveDockIcon(
    register __a0 struct DockIcon *icon)
{
    struct DockIconMsg *msg;
    BOOL ok;

    ok = FALSE;
    if( icon == NULL ) {
        return FALSE;
    }

    msg = (struct DockIconMsg *)AllocMem(sizeof(struct DockIconMsg),
        MEMF_PUBLIC | MEMF_CLEAR);
    if( msg == NULL ) {
        return FALSE;
    }

    msg->dim_Command = DIMCMD_REMOVE;
    msg->dim_Icon = icon;

    if( send_dockicon_msg(msg) && msg->dim_Error == 0 ) {
        ok = TRUE;
    }

    FreeMem(msg, sizeof(struct DockIconMsg));
    return ok;
}

VOID __asm __saveds TD_RedrawDockIcon(
    register __a0 struct DockIcon *icon)
{
    struct DockIconMsg *msg;

    if( icon == NULL ) {
        return;
    }

    msg = (struct DockIconMsg *)AllocMem(sizeof(struct DockIconMsg),
        MEMF_PUBLIC | MEMF_CLEAR);
    if( msg == NULL ) {
        return;
    }

    msg->dim_Command = DIMCMD_REDRAW;
    msg->dim_Icon = icon;
    send_dockicon_msg(msg);
    FreeMem(msg, sizeof(struct DockIconMsg));
}
