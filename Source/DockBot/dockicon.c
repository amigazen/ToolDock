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

#include "dockicon_host.h"
#include "dock_appicon.h"
#include "applife.h"
#include "rearrange.h"

#include <exec/memory.h>
#include <workbench/startup.h>
#include <workbench/icon.h>
#include <rexx/storage.h>
#include <rexx/rxslib.h>
#include <utility/tagitem.h>
#include <string.h>

#include <clib/alib_protos.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <clib/icon_protos.h>
#include <clib/intuition_protos.h>
#include <clib/rexxsyslib_protos.h>
#include <pragmas/rexxsyslib_pragmas.h>

#ifndef WBAPPICONA_SupportsOpen
#define WBAPPICONA_SupportsOpen         (TAG_USER + 0xA001)
#define WBAPPICONA_SupportsCopy         (TAG_USER + 0xA002)
#define WBAPPICONA_SupportsRename       (TAG_USER + 0xA003)
#define WBAPPICONA_SupportsInformation  (TAG_USER + 0xA004)
#define WBAPPICONA_SupportsSnapshot     (TAG_USER + 0xA005)
#define WBAPPICONA_SupportsUnSnapshot   (TAG_USER + 0xA006)
#define WBAPPICONA_SupportsLeaveOut     (TAG_USER + 0xA007)
#define WBAPPICONA_SupportsPutAway      (TAG_USER + 0xA008)
#define WBAPPICONA_SupportsDelete       (TAG_USER + 0xA009)
#define WBAPPICONA_SupportsFormatDisk   (TAG_USER + 0xA00A)
#define WBAPPICONA_SupportsEmptyTrash   (TAG_USER + 0xA00B)
#define WBAPPICONA_SupportsEjectDisk    (TAG_USER + 0xA000 + 158)
#define WBAPPICONA_RenderHook           (TAG_USER + 0xA00D)
#define WBAPPICONA_NotifySelectState    (TAG_USER + 0xA00E)
#endif

extern struct Library *IconBase;
extern struct Library *RexxSysBase;

#pragma libcall IconBase DupDiskObjectA 96 9802

STATIC BOOL words_eq_i(STRPTR a, STRPTR b)
{
    UBYTE ca;
    UBYTE cb;

    if( a == NULL || b == NULL ) {
        return FALSE;
    }
    while( *a || *b ) {
        ca = *a;
        cb = *b;
        if( ca >= 'a' && ca <= 'z' ) {
            ca = (UBYTE)(ca - 32);
        }
        if( cb >= 'a' && cb <= 'z' ) {
            cb = (UBYTE)(cb - 32);
        }
        if( ca != cb ) {
            return FALSE;
        }
        a++;
        b++;
    }
    return TRUE;
}

STATIC VOID copy_text(STRPTR dst, STRPTR src, UWORD max)
{
    UWORD n;

    dst[0] = 0;
    if( src == NULL ) {
        return;
    }
    n = 0;
    while( src[n] && n < (UWORD)(max - 1) ) {
        n++;
    }
    if( n ) {
        CopyMem(src, dst, n);
    }
    dst[n] = 0;
}

STATIC ULONG parse_supports(struct TagItem *tags)
{
    struct TagItem *t;
    ULONG supports;

    supports = DISUP_OPEN;
    if( tags == NULL ) {
        return supports;
    }
    t = tags;
    while( t->ti_Tag != TAG_END && t->ti_Tag != TAG_DONE ) {
        if( t->ti_Tag == TAG_IGNORE ) {
            t++;
            continue;
        }
        if( t->ti_Tag == TAG_MORE ) {
            t = (struct TagItem *)t->ti_Data;
            continue;
        }
        if( t->ti_Data ) {
            if( t->ti_Tag == WBAPPICONA_SupportsOpen ) {
                supports |= DISUP_OPEN;
            } else if( t->ti_Tag == WBAPPICONA_SupportsCopy ) {
                supports |= DISUP_COPY;
            } else if( t->ti_Tag == WBAPPICONA_SupportsRename ) {
                supports |= DISUP_RENAME;
            } else if( t->ti_Tag == WBAPPICONA_SupportsInformation ) {
                supports |= DISUP_INFORMATION;
            } else if( t->ti_Tag == WBAPPICONA_SupportsSnapshot ) {
                supports |= DISUP_SNAPSHOT;
            } else if( t->ti_Tag == WBAPPICONA_SupportsUnSnapshot ) {
                supports |= DISUP_UNSNAPSHOT;
            } else if( t->ti_Tag == WBAPPICONA_SupportsLeaveOut ) {
                supports |= DISUP_LEAVEOUT;
            } else if( t->ti_Tag == WBAPPICONA_SupportsPutAway ) {
                supports |= DISUP_PUTAWAY;
            } else if( t->ti_Tag == WBAPPICONA_SupportsDelete ) {
                supports |= DISUP_DELETE;
            } else if( t->ti_Tag == WBAPPICONA_SupportsFormatDisk ) {
                supports |= DISUP_FORMATDISK;
            } else if( t->ti_Tag == WBAPPICONA_SupportsEmptyTrash ) {
                supports |= DISUP_EMPTYTRASH;
            } else if( t->ti_Tag == WBAPPICONA_SupportsEjectDisk ) {
                supports |= DISUP_EJECTDISK;
            }
        }
        t++;
    }
    return supports;
}

STATIC struct Hook *parse_hook(struct TagItem *tags)
{
    struct TagItem *t;

    if( tags == NULL ) {
        return NULL;
    }
    t = tags;
    while( t->ti_Tag != TAG_END && t->ti_Tag != TAG_DONE ) {
        if( t->ti_Tag == TAG_IGNORE ) {
            t++;
            continue;
        }
        if( t->ti_Tag == TAG_MORE ) {
            t = (struct TagItem *)t->ti_Data;
            continue;
        }
        if( t->ti_Tag == WBAPPICONA_RenderHook ) {
            return (struct Hook *)t->ti_Data;
        }
        t++;
    }
    return NULL;
}

STATIC BOOL parse_notify(struct TagItem *tags)
{
    struct TagItem *t;

    if( tags == NULL ) {
        return FALSE;
    }
    t = tags;
    while( t->ti_Tag != TAG_END && t->ti_Tag != TAG_DONE ) {
        if( t->ti_Tag == TAG_IGNORE ) {
            t++;
            continue;
        }
        if( t->ti_Tag == TAG_MORE ) {
            t = (struct TagItem *)t->ti_Data;
            continue;
        }
        if( t->ti_Tag == WBAPPICONA_NotifySelectState ) {
            return t->ti_Data ? TRUE : FALSE;
        }
        t++;
    }
    return FALSE;
}

STATIC VOID free_draw_obj(struct DockIcon *icon)
{
    if( icon->di_DrawObj && icon->di_DrawObj != icon->di_DiskObj ) {
        FreeDiskObject(icon->di_DrawObj);
    }
    icon->di_DrawObj = NULL;
}

STATIC VOID dispose_icon(struct DockIcon *icon)
{
    if( icon == NULL ) {
        return;
    }
    if( icon->di_Node ) {
        if( icon->di_Node->n.ln_Succ ) {
            Remove((struct Node *)icon->di_Node);
        }
        TD_FreeGadget(icon->di_Node);
        icon->di_Node = NULL;
        icon->di_Gadget = NULL;
    }
    free_draw_obj(icon);
    FreeMem(icon, sizeof(struct DockIcon));
}

STATIC BOOL icon_on_list(struct DockWindow *dock, struct DockIcon *icon)
{
    struct DockIcon *curr;

    if( icon == NULL ) {
        return FALSE;
    }
    curr = (struct DockIcon *)dock->dockIcons.mlh_Head;
    while( curr->n.mln_Succ ) {
        if( curr == icon ) {
            return TRUE;
        }
        curr = (struct DockIcon *)curr->n.mln_Succ;
    }
    return FALSE;
}

STATIC VOID relayout_if_shown(struct DockWindow *dock)
{
    if( dock->win == NULL ) {
        return;
    }
    disable_layout(dock);
    remap_gadgets(dock);
    enable_layout(dock);
    draw_gadgets(dock);
    rearrange_sync(dock);
}

STATIC VOID add_dock_icon(struct DockWindow *dock, struct DockIconMsg *msg)
{
    struct DockIcon *icon;
    struct DgNode *dg;

    msg->dim_Icon = NULL;
    msg->dim_Error = -1;

    if( msg->dim_AppPort == NULL || msg->dim_DiskObj == NULL ) {
        return;
    }
    if( dock->runState == RS_QUITTING || dock->runState == RS_STOPPED ) {
        return;
    }

    icon = (struct DockIcon *)AllocMem(sizeof(struct DockIcon),
        MEMF_PUBLIC | MEMF_CLEAR);
    if( icon == NULL ) {
        return;
    }

    icon->di_ID = msg->dim_ID;
    icon->di_UserData = msg->dim_UserData;
    icon->di_AppPort = msg->dim_AppPort;
    icon->di_Lock = msg->dim_Lock;
    icon->di_DiskObj = msg->dim_DiskObj;
    icon->di_Supports = parse_supports(msg->dim_Tags);
    icon->di_RenderHook = parse_hook(msg->dim_Tags);
    icon->di_NotifySelect = parse_notify(msg->dim_Tags);
    icon->di_Dock = dock;
    copy_text((STRPTR)icon->di_Text, msg->dim_Text, DI_TEXT_LEN);

    if( IconBase && IconBase->lib_Version >= 44 ) {
        icon->di_DrawObj = DupDiskObjectA(icon->di_DiskObj, NULL);
    }
    if( icon->di_DrawObj == NULL ) {
        icon->di_DrawObj = icon->di_DiskObj;
    }

    dg = TD_AllocGadget((STRPTR)DB_APPICON_CLASS);
    if( dg == NULL ) {
        free_draw_obj(icon);
        FreeMem(icon, sizeof(struct DockIcon));
        return;
    }

    icon->di_Node = dg;
    icon->di_Gadget = dg->dg;
    dock_appicon_bind(dg->dg, icon);
    if( dock->gadgetPort ) {
        dock_gadget_added(dg->dg, dock->gadgetPort);
    }

    AddTail(&dock->cfg.gadgets, (struct Node *)dg);
    AddTail((struct List *)&dock->dockIcons, (struct Node *)icon);

    relayout_if_shown(dock);

    msg->dim_Icon = icon;
    msg->dim_Error = 0;
}

STATIC VOID remove_dock_icon(struct DockWindow *dock, struct DockIconMsg *msg)
{
    struct DockIcon *icon;

    icon = msg->dim_Icon;
    msg->dim_Error = -1;
    if( ! icon_on_list(dock, icon) ) {
        return;
    }
    Remove((struct Node *)icon);
    dispose_icon(icon);
    relayout_if_shown(dock);
    msg->dim_Error = 0;
}

STATIC VOID redraw_dock_icon(struct DockWindow *dock, struct DockIconMsg *msg)
{
    struct DockIcon *icon;

    icon = msg->dim_Icon;
    msg->dim_Error = -1;
    if( ! icon_on_list(dock, icon) ) {
        return;
    }
    if( icon->di_Gadget ) {
        TD_RequestDockGadgetDraw(icon->di_Gadget);
    }
    msg->dim_Error = 0;
}

STATIC VOID reply_rexx(struct RexxMsg *rm, LONG error, STRPTR result)
{
    if( rm == NULL ) {
        return;
    }
    rm->rm_Result1 = error == 0 ? 0 : 10;
    rm->rm_Result2 = 0;
    if( error == 0 && result && RexxSysBase
        && (rm->rm_Action & RXFF_RESULT) ) {
        rm->rm_Result2 = (LONG)CreateArgstring(result, (LONG)strlen(result));
        if( rm->rm_Result2 == 0 ) {
            rm->rm_Result1 = 10;
        }
    }
    ReplyMsg((struct Message *)rm);
}

STATIC VOID handle_rexx(struct DockWindow *dock, struct RexxMsg *rm)
{
    UBYTE work[64];
    STRPTR cmd;
    UWORD n;

    cmd = (STRPTR)rm->rm_Args[0];
    if( cmd == NULL ) {
        reply_rexx(rm, 1, NULL);
        return;
    }
    while( *cmd == ' ' || *cmd == '\t' ) {
        cmd++;
    }
    n = 0;
    while( cmd[n] && n < 63 ) {
        if( cmd[n] == ' ' || cmd[n] == '\t' || cmd[n] == '\n'
            || cmd[n] == '\r' ) {
            break;
        }
        work[n] = cmd[n];
        n++;
    }
    work[n] = 0;
    cmd = (STRPTR)work;

    if( words_eq_i(cmd, (STRPTR)"SHOW") ) {
        if( dock->runState == RS_HIDDEN ) {
            dock->runState = RS_UNICONIFYING;
        }
        reply_rexx(rm, 0, (STRPTR)"OK");
        return;
    }
    if( words_eq_i(cmd, (STRPTR)"HIDE")
        || words_eq_i(cmd, (STRPTR)"ICONIFY") ) {
        if( dock->runState == RS_RUNNING ) {
            dock->runState = RS_ICONIFYING;
        }
        reply_rexx(rm, 0, (STRPTR)"OK");
        return;
    }
    if( words_eq_i(cmd, (STRPTR)"QUIT") ) {
        dock->runState = RS_QUITTING;
        reply_rexx(rm, 0, (STRPTR)"OK");
        return;
    }

    reply_rexx(rm, 1, (STRPTR)"Unknown command");
}

STATIC VOID handle_native(struct DockWindow *dock, struct DockIconMsg *msg)
{
    if( msg->dim_Command == DIMCMD_ADD ) {
        add_dock_icon(dock, msg);
    } else if( msg->dim_Command == DIMCMD_REMOVE ) {
        remove_dock_icon(dock, msg);
    } else if( msg->dim_Command == DIMCMD_REDRAW ) {
        redraw_dock_icon(dock, msg);
    } else {
        msg->dim_Error = -1;
    }
    ReplyMsg((struct Message *)msg);
}

STATIC VOID free_app_message(struct AppMessage *am)
{
    LONG i;

    if( am == NULL ) {
        return;
    }
    if( am->am_ArgList ) {
        for( i = 0; i < am->am_NumArgs; i++ ) {
            if( am->am_ArgList[i].wa_Lock ) {
                UnLock(am->am_ArgList[i].wa_Lock);
            }
            if( am->am_ArgList[i].wa_Name ) {
                FreeMem(am->am_ArgList[i].wa_Name, 1);
            }
        }
        FreeMem(am->am_ArgList,
            (ULONG)am->am_NumArgs * sizeof(struct WBArg));
    }
    FreeMem(am, sizeof(struct AppMessage));
}

BOOL init_dock_icons(struct DockWindow *dock)
{
    NewList((struct List *)&dock->dockIcons);
    dock->appIconClass = init_dock_appicon_class();
    dock->diReplyPort = CreateMsgPort();
    if( dock->appIconClass == NULL || dock->diReplyPort == NULL ) {
        return FALSE;
    }
    return TRUE;
}

VOID free_dock_icons(struct DockWindow *dock)
{
    struct DockIcon *icon;
    struct Message *msg;

    while( (icon = (struct DockIcon *)RemHead((struct List *)&dock->dockIcons)) ) {
        dispose_icon(icon);
    }

    if( dock->diReplyPort ) {
        while( (msg = GetMsg(dock->diReplyPort)) ) {
            free_app_message((struct AppMessage *)msg);
        }
        DeleteMsgPort(dock->diReplyPort);
        dock->diReplyPort = NULL;
    }

    if( dock->pubPort ) {
        while( (msg = GetMsg(dock->pubPort)) ) {
            if( RexxSysBase && IsRexxMsg(msg) ) {
                reply_rexx((struct RexxMsg *)msg, 1, NULL);
            } else {
                ReplyMsg(msg);
            }
        }
    }

    if( dock->appIconClass ) {
        free_dock_appicon_class(dock->appIconClass);
        dock->appIconClass = NULL;
    }
}

VOID dock_icons_preserve(struct DockWindow *dock)
{
    struct DockIcon *icon;

    icon = (struct DockIcon *)dock->dockIcons.mlh_Head;
    while( icon->n.mln_Succ ) {
        if( icon->di_Node && icon->di_Node->n.ln_Succ ) {
            Remove((struct Node *)icon->di_Node);
        }
        icon = (struct DockIcon *)icon->n.mln_Succ;
    }
}

VOID dock_icons_restore(struct DockWindow *dock)
{
    struct DockIcon *icon;

    icon = (struct DockIcon *)dock->dockIcons.mlh_Head;
    while( icon->n.mln_Succ ) {
        if( icon->di_Node ) {
            AddTail(&dock->cfg.gadgets, (struct Node *)icon->di_Node);
            if( dock->gadgetPort && icon->di_Gadget ) {
                dock_gadget_added(icon->di_Gadget, dock->gadgetPort);
            }
        }
        icon = (struct DockIcon *)icon->n.mln_Succ;
    }
}

VOID handle_pub_port(struct DockWindow *dock)
{
    struct Message *msg;
    struct DockIconMsg *dim;

    if( dock->pubPort == NULL ) {
        return;
    }

    while( (msg = GetMsg(dock->pubPort)) ) {
        if( RexxSysBase && IsRexxMsg(msg) ) {
            handle_rexx(dock, (struct RexxMsg *)msg);
        } else {
            dim = (struct DockIconMsg *)msg;
            if( dim->dim_Magic == DIM_MAGIC ) {
                handle_native(dock, dim);
            } else {
                ReplyMsg(msg);
            }
        }
    }
}

VOID handle_dockicon_replies(struct DockWindow *dock)
{
    struct Message *msg;

    if( dock->diReplyPort == NULL ) {
        return;
    }
    while( (msg = GetMsg(dock->diReplyPort)) ) {
        free_app_message((struct AppMessage *)msg);
    }
}

VOID dock_icon_send_event(struct DockIcon *icon, UWORD amClass,
    STRPTR *paths, UWORD pathCount)
{
    struct AppMessage *am;
    struct WBArg *args;
    UWORD n;
    UWORD i;
    ULONG secs;
    ULONG micros;
    BPTR lock;

    if( icon == NULL || icon->di_AppPort == NULL || icon->di_Dock == NULL ) {
        return;
    }
    if( icon->di_Dock->diReplyPort == NULL ) {
        return;
    }

    n = pathCount;
    args = NULL;
    if( n ) {
        args = (struct WBArg *)AllocMem((ULONG)n * sizeof(struct WBArg),
            MEMF_PUBLIC | MEMF_CLEAR);
        if( args == NULL ) {
            return;
        }
        for( i = 0; i < n; i++ ) {
            args[i].wa_Name = (STRPTR)AllocMem(1, MEMF_PUBLIC | MEMF_CLEAR);
            lock = NULL;
            if( paths && paths[i] && paths[i][0] ) {
                lock = Lock(paths[i], ACCESS_READ);
            }
            args[i].wa_Lock = lock;
        }
    }

    am = (struct AppMessage *)AllocMem(sizeof(struct AppMessage),
        MEMF_PUBLIC | MEMF_CLEAR);
    if( am == NULL ) {
        if( args ) {
            for( i = 0; i < n; i++ ) {
                if( args[i].wa_Lock ) {
                    UnLock(args[i].wa_Lock);
                }
                if( args[i].wa_Name ) {
                    FreeMem(args[i].wa_Name, 1);
                }
            }
            FreeMem(args, (ULONG)n * sizeof(struct WBArg));
        }
        return;
    }

    CurrentTime(&secs, &micros);

    am->am_Message.mn_Node.ln_Type = NT_MESSAGE;
    am->am_Message.mn_Length = (UWORD)sizeof(struct AppMessage);
    am->am_Message.mn_ReplyPort = icon->di_Dock->diReplyPort;
    am->am_Type = AMTYPE_APPICON;
    am->am_UserData = icon->di_UserData;
    am->am_ID = icon->di_ID;
    am->am_NumArgs = (LONG)n;
    am->am_ArgList = args;
    am->am_Version = AM_VERSION;
    am->am_Class = amClass;
    am->am_Seconds = secs;
    am->am_Micros = micros;

    PutMsg(icon->di_AppPort, (struct Message *)am);
}

VOID dock_icon_bind(Object *o, struct DockIcon *icon)
{
    dock_appicon_bind(o, icon);
}
