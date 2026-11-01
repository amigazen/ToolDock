/************************************
**
**  ToolDock - A Dock For AmigaOS 3
**
**  SPDX-License-Identifier: MIT
**
**  ToolDock changes Copyright 2026 amigazen project
**
************************************/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/lists.h>
#include <exec/libraries.h>
#include <intuition/intuition.h>
#include <utility/tagitem.h>
#include <libraries/commodities.h>
#include <libraries/pm.h>
#include <rexx/storage.h>
#include <rexx/rxslib.h>
#include <string.h>

#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <clib/rexxsyslib_protos.h>
#include <pragmas/rexxsyslib_pragmas.h>
#include <proto/popupmenu.h>

#include "dockbot.h"
#include "dockbot_protos.h"
#include "dockbot_pragmas.h"
#include "applife.h"

#ifndef STATIC
#define STATIC static
#endif

/*
 * Exchange controller LVOs are in the NDK SFD but not in the public
 * commodities_protos.h. Offsets match commodities_lib.sfd and CxCommander:
 * CopyBrokerList 0xba, FreeBrokerList 0xc0, BrokerCommand 0xc6.
 */
#pragma libcall CxBase CopyBrokerList ba 801
#pragma libcall CxBase FreeBrokerList c0 801
#pragma libcall CxBase BrokerCommand c6 802

LONG CopyBrokerList(struct List *blist);
VOID FreeBrokerList(struct List *list);
LONG BrokerCommand(STRPTR name, LONG id);

/* Same layout Exchange and CxCommander use for CopyBrokerList nodes. */
struct BrokerCopy {
    struct Node bc_Node;
    char bc_Name[CBD_NAMELEN];
    char bc_Title[CBD_TITLELEN];
    char bc_Descr[CBD_DESCRLEN];
    LONG bc_Task;
    LONG bc_Dummy1;
    LONG bc_Dummy2;
    UWORD bc_Flags;
};

/* File-local: ToolDock already has a global CxBase in main.c. */
STATIC struct Library *CxBase;
struct Library *RexxSysBase;
struct PopupMenuBase *PopupMenuBase;

STATIC BOOL alReady;
STATIC struct MsgPort *alReply;

BOOL applife_init(VOID)
{
    if( alReady ) {
        return TRUE;
    }

    if( CxBase == NULL ) {
        CxBase = OpenLibrary("commodities.library", 37);
    }
    if( RexxSysBase == NULL ) {
        RexxSysBase = OpenLibrary(RXSNAME, 0);
    }
    /* Optional. Icon and Project popups stay unused if this fails. */
    if( PopupMenuBase == NULL ) {
        PopupMenuBase = (struct PopupMenuBase *)OpenLibrary(POPUPMENU_NAME,
            POPUPMENU_VERSION);
    }
    if( alReply == NULL ) {
        alReply = CreateMsgPort();
    }

    alReady = TRUE;
    return TRUE;
}

BOOL applife_have_popup(VOID)
{
    if( PopupMenuBase ) {
        return TRUE;
    }
    return FALSE;
}

VOID applife_reap(VOID)
{
    struct RexxMsg *rm;

    if( alReply == NULL ) {
        return;
    }
    while( rm = (struct RexxMsg *)GetMsg(alReply) ) {
        ClearRexxMsg(rm, 1);
        DeleteRexxMsg(rm);
    }
}

VOID applife_expunge(VOID)
{
    if( ! alReady ) {
        return;
    }

    applife_reap();

    if( alReply ) {
        DeleteMsgPort(alReply);
        alReply = NULL;
    }
    if( PopupMenuBase ) {
        CloseLibrary((struct Library *)PopupMenuBase);
        PopupMenuBase = NULL;
    }
    if( RexxSysBase ) {
        CloseLibrary(RexxSysBase);
        RexxSysBase = NULL;
    }
    if( CxBase ) {
        CloseLibrary(CxBase);
        CxBase = NULL;
    }

    alReady = FALSE;
}

BOOL applife_port_present(STRPTR name)
{
    struct MsgPort *p;
    BOOL found;

    found = FALSE;
    if( name == NULL || name[0] == 0 ) {
        return FALSE;
    }

    Forbid();
    p = FindPort(name);
    if( p ) {
        found = TRUE;
    }
    Permit();

    return found;
}

BOOL applife_broker_present(STRPTR name)
{
    struct List blist;
    struct Node *n;
    struct Node *next;
    struct BrokerCopy *bc;
    BOOL found;

    found = FALSE;
    if( CxBase == NULL || name == NULL || name[0] == 0 ) {
        return FALSE;
    }

    NewList(&blist);
    CopyBrokerList(&blist);

    n = blist.lh_Head;
    while( n && n->ln_Succ ) {
        next = n->ln_Succ;
        bc = (struct BrokerCopy *)n;
        if( strcmp(bc->bc_Name, name) == 0 ) {
            found = TRUE;
            break;
        }
        n = next;
    }

    FreeBrokerList(&blist);
    return found;
}

BOOL applife_is_running(STRPTR cxName, STRPTR rxName)
{
    if( applife_broker_present(cxName) ) {
        return TRUE;
    }
    if( applife_port_present(rxName) ) {
        return TRUE;
    }
    return FALSE;
}

BOOL applife_send_rexx(STRPTR portName, STRPTR cmd)
{
    struct RexxMsg *rm;
    struct MsgPort *port;
    ULONG len;
    BOOL ok;

    ok = FALSE;
    applife_reap();

    if( RexxSysBase == NULL || alReply == NULL ) {
        return FALSE;
    }
    if( portName == NULL || portName[0] == 0 ) {
        return FALSE;
    }
    if( cmd == NULL || cmd[0] == 0 ) {
        return FALSE;
    }

    rm = CreateRexxMsg(alReply, NULL, NULL);
    if( rm == NULL ) {
        return FALSE;
    }

    len = strlen(cmd);
    rm->rm_Action = RXCOMM;
    rm->rm_Args[0] = (STRPTR)CreateArgstring(cmd, len);
    if( rm->rm_Args[0] == NULL ) {
        DeleteRexxMsg(rm);
        return FALSE;
    }

    Forbid();
    port = FindPort(portName);
    if( port ) {
        PutMsg(port, (struct Message *)rm);
        ok = TRUE;
    }
    Permit();

    if( ! ok ) {
        ClearRexxMsg(rm, 1);
        DeleteRexxMsg(rm);
    }
    return ok;
}

STATIC VOID send_cx(STRPTR cxName, LONG cmd)
{
    if( CxBase && cxName && cxName[0] ) {
        BrokerCommand(cxName, cmd);
    }
}

VOID applife_show(STRPTR cxName, STRPTR rxName, STRPTR rxShow)
{
    send_cx(cxName, CXCMD_APPEAR);
    if( rxName && rxShow ) {
        applife_send_rexx(rxName, rxShow);
    }
}

VOID applife_hide(STRPTR cxName, STRPTR rxName, STRPTR rxHide)
{
    send_cx(cxName, CXCMD_DISAPPEAR);
    if( rxName && rxHide ) {
        applife_send_rexx(rxName, rxHide);
    }
}

VOID applife_quit(STRPTR cxName, STRPTR rxName, STRPTR rxQuit)
{
    send_cx(cxName, CXCMD_KILL);
    if( rxName && rxQuit ) {
        applife_send_rexx(rxName, rxQuit);
    }
}

STRPTR applife_format_rxopen(STRPTR fmt, STRPTR path)
{
    STRPTR p;
    STRPTR q;
    STRPTR out;
    ULONG pl;
    ULONG ol;
    BOOL replaced;

    if( fmt == NULL || path == NULL ) {
        return NULL;
    }

    pl = strlen(path);
    ol = 1;
    p = fmt;
    replaced = FALSE;
    while( *p ) {
        if( ! replaced && p[0] == '%' && p[1] == 's' ) {
            ol += pl;
            p += 2;
            replaced = TRUE;
        } else {
            ol++;
            p++;
        }
    }
    if( ! replaced ) {
        ol += 3 + pl;
    }

    out = (STRPTR)TD_AllocMem(ol, MEMF_CLEAR);
    if( out == NULL ) {
        return NULL;
    }

    p = fmt;
    q = out;
    replaced = FALSE;
    while( *p ) {
        if( ! replaced && p[0] == '%' && p[1] == 's' ) {
            CopyMem(path, q, pl);
            q += pl;
            p += 2;
            replaced = TRUE;
        } else {
            *q++ = *p++;
        }
    }
    if( ! replaced ) {
        *q++ = ' ';
        *q++ = '"';
        CopyMem(path, q, pl);
        q += pl;
        *q++ = '"';
    }
    *q = 0;
    return out;
}

ULONG applife_open_app_popup(struct Window *win, STRPTR title,
    STRPTR openStr, STRPTR showStr, STRPTR hideStr, STRPTR quitStr,
    STRPTR removeStr, STRPTR *cmdLabels, UWORD cmdCount, UWORD flags)
{
    struct PopupMenu *menu;
    struct TagItem mt[32];
    ULONG result;
    STRPTR head;
    UWORD t;
    UWORD i;
    UWORD ncmd;
    BOOL ok;

    result = AL_POP_NONE;
    if( PopupMenuBase == NULL || win == NULL ) {
        return AL_POP_NONE;
    }

    head = title;
    if( head == NULL || head[0] == 0 ) {
        head = (STRPTR)"App";
    }
    if( openStr == NULL ) {
        openStr = (STRPTR)"Open";
    }
    if( showStr == NULL ) {
        showStr = (STRPTR)"Show";
    }
    if( hideStr == NULL ) {
        hideStr = (STRPTR)"Hide";
    }
    if( quitStr == NULL ) {
        quitStr = (STRPTR)"Quit";
    }
    if( removeStr == NULL ) {
        removeStr = (STRPTR)"Remove";
    }

    ncmd = cmdCount;
    if( ncmd > AL_CMD_MAX ) {
        ncmd = AL_CMD_MAX;
    }
    if( cmdLabels == NULL ) {
        ncmd = 0;
    }

    t = 0;
    mt[t].ti_Tag = PM_Item;
    mt[t].ti_Data = (ULONG)PM_MakeItem(PM_Hidden, TRUE, TAG_DONE);
    t++;
    mt[t].ti_Tag = PM_Item;
    mt[t].ti_Data = (ULONG)PM_MakeItem(PM_Title, head,
        PM_NoSelect, TRUE, PM_ShinePen, TRUE, PM_Shadowed, TRUE,
        PM_Center, TRUE, TAG_DONE);
    t++;
    mt[t].ti_Tag = PM_Item;
    mt[t].ti_Data = (ULONG)PM_MakeItem(PM_WideTitleBar, TRUE, TAG_DONE);
    t++;
    mt[t].ti_Tag = PM_Item;
    mt[t].ti_Data = (ULONG)PM_MakeItem(PM_Title, openStr,
        PM_UserData, AL_POP_OPEN,
        PM_Disabled, (flags & AL_CAN_OPEN) ? FALSE : TRUE,
        TAG_DONE);
    t++;
    mt[t].ti_Tag = PM_Item;
    mt[t].ti_Data = (ULONG)PM_MakeItem(PM_WideTitleBar, TRUE, TAG_DONE);
    t++;
    mt[t].ti_Tag = PM_Item;
    mt[t].ti_Data = (ULONG)PM_MakeItem(PM_Title, showStr,
        PM_UserData, AL_POP_SHOW,
        PM_Disabled, (flags & AL_CAN_SHOW) ? FALSE : TRUE,
        TAG_DONE);
    t++;
    mt[t].ti_Tag = PM_Item;
    mt[t].ti_Data = (ULONG)PM_MakeItem(PM_Title, hideStr,
        PM_UserData, AL_POP_HIDE,
        PM_Disabled, (flags & AL_CAN_HIDE) ? FALSE : TRUE,
        TAG_DONE);
    t++;
    mt[t].ti_Tag = PM_Item;
    mt[t].ti_Data = (ULONG)PM_MakeItem(PM_Title, quitStr,
        PM_UserData, AL_POP_QUIT,
        PM_Disabled, (flags & AL_CAN_QUIT) ? FALSE : TRUE,
        TAG_DONE);
    t++;

    if( ncmd > 0 ) {
        mt[t].ti_Tag = PM_Item;
        mt[t].ti_Data = (ULONG)PM_MakeItem(PM_WideTitleBar, TRUE, TAG_DONE);
        t++;
        for( i = 0; i < ncmd; i++ ) {
            if( cmdLabels[i] == NULL || cmdLabels[i][0] == 0 ) {
                continue;
            }
            mt[t].ti_Tag = PM_Item;
            mt[t].ti_Data = (ULONG)PM_MakeItem(PM_Title, cmdLabels[i],
                PM_UserData, (ULONG)(AL_POP_CMD + i),
                TAG_DONE);
            t++;
        }
    }

    mt[t].ti_Tag = PM_Item;
    mt[t].ti_Data = (ULONG)PM_MakeItem(PM_WideTitleBar, TRUE, TAG_DONE);
    t++;
    mt[t].ti_Tag = PM_Item;
    mt[t].ti_Data = (ULONG)PM_MakeItem(PM_Title, removeStr,
        PM_UserData, AL_POP_REMOVE,
        TAG_DONE);
    t++;
    mt[t].ti_Tag = TAG_DONE;

    ok = TRUE;
    for( i = 0; i < t; i++ ) {
        if( mt[i].ti_Tag == PM_Item && mt[i].ti_Data == 0 ) {
            ok = FALSE;
            break;
        }
    }

    menu = NULL;
    if( ok ) {
        menu = PM_MakeMenuA(mt);
    }
    if( menu == NULL ) {
        for( i = 0; i < t; i++ ) {
            if( mt[i].ti_Tag == PM_Item && mt[i].ti_Data ) {
                PM_FreePopupMenu((struct PopupMenu *)mt[i].ti_Data);
            }
        }
        return AL_POP_NONE;
    }

    result = PM_OpenPopupMenu(win, PM_Menu, menu, TAG_DONE);
    PM_FreePopupMenu(menu);
    return result;
}

ULONG applife_open_choice_popup(struct Window *win, STRPTR title,
    STRPTR item, BOOL disabled)
{
    struct PopupMenu *menu;
    ULONG result;

    result = 0;
    if( PopupMenuBase == NULL || win == NULL ) {
        return 0;
    }
    if( title == NULL || title[0] == 0 ) {
        title = (STRPTR)"Menu";
    }
    if( item == NULL || item[0] == 0 ) {
        item = (STRPTR)"OK";
    }

    menu = PMMenu(title),
        PMItem(item),
            PM_UserData, 1,
            PM_Disabled, disabled ? TRUE : FALSE,
            PMEnd,
        PMEnd;

    if( menu ) {
        result = PM_OpenPopupMenu(win, PM_Menu, menu, TAG_DONE);
        PM_FreePopupMenu(menu);
    }
    return result;
}

ULONG applife_open_action_popup(struct Window *win, STRPTR title,
    STRPTR item, BOOL itemDisabled, ULONG itemId, STRPTR removeStr)
{
    struct PopupMenu *menu;
    ULONG result;

    result = AL_POP_NONE;
    if( PopupMenuBase == NULL || win == NULL ) {
        return AL_POP_NONE;
    }
    if( title == NULL || title[0] == 0 ) {
        title = (STRPTR)"Menu";
    }
    if( item == NULL || item[0] == 0 ) {
        item = (STRPTR)"OK";
    }
    if( removeStr == NULL ) {
        removeStr = (STRPTR)"Remove";
    }
    if( itemId == 0 ) {
        itemId = 1;
    }

    menu = PMMenu(title),
        PMItem(item),
            PM_UserData, itemId,
            PM_Disabled, itemDisabled ? TRUE : FALSE,
            PMEnd,
        PMBar, PMEnd,
        PMItem(removeStr),
            PM_UserData, AL_POP_REMOVE,
            PMEnd,
        PMEnd;

    if( menu ) {
        result = PM_OpenPopupMenu(win, PM_Menu, menu, TAG_DONE);
        PM_FreePopupMenu(menu);
    }
    return result;
}

#define AL_CX_MAX       32
#define AL_CX_SHIFT     4
#define AL_CX_SHOW      1
#define AL_CX_HIDE      2
#define AL_CX_QUIT      3

struct AlCxEntry {
    UBYTE name[CBD_NAMELEN];
    UBYTE title[CBD_TITLELEN];
};

STATIC VOID al_copy(STRPTR dst, STRPTR src, ULONG dstlen)
{
    ULONG i;

    i = 0;
    if( dst == NULL || dstlen == 0 ) {
        return;
    }
    if( src ) {
        while( src[i] && i < dstlen - 1 ) {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = 0;
}

ULONG applife_run_exchange_popup(struct Window *win, STRPTR title,
    STRPTR showStr, STRPTR hideStr, STRPTR quitStr, STRPTR noneStr,
    STRPTR removeStr)
{
    struct List blist;
    struct Node *n;
    struct BrokerCopy *bc;
    struct AlCxEntry *ents;
    struct PopupMenu *menu;
    struct PopupMenu *items[AL_CX_MAX];
    struct PopupMenu *subs[AL_CX_MAX];
    struct PopupMenu *noneItem;
    struct TagItem mt[(AL_CX_MAX + 6) * 2];
    UWORD count;
    UWORD i;
    UWORD t;
    UWORD made;
    ULONG result;
    UWORD idx;
    UWORD act;
    LONG cmd;
    BOOL ok;

    if( PopupMenuBase == NULL || win == NULL ) {
        return AL_POP_NONE;
    }
    if( title == NULL || title[0] == 0 ) {
        title = (STRPTR)"Exchange";
    }
    if( showStr == NULL ) {
        showStr = (STRPTR)"Show";
    }
    if( hideStr == NULL ) {
        hideStr = (STRPTR)"Hide";
    }
    if( quitStr == NULL ) {
        quitStr = (STRPTR)"Quit";
    }
    if( noneStr == NULL ) {
        noneStr = (STRPTR)"None";
    }
    if( removeStr == NULL ) {
        removeStr = (STRPTR)"Remove";
    }

    ents = (struct AlCxEntry *)AllocMem(
        sizeof(struct AlCxEntry) * AL_CX_MAX, MEMF_CLEAR);
    if( ents == NULL ) {
        return AL_POP_NONE;
    }

    /* Snapshot names so the CX list is not held while the menu is open. */
    count = 0;
    if( CxBase ) {
        NewList(&blist);
        CopyBrokerList(&blist);
        n = blist.lh_Head;
        while( n && n->ln_Succ && count < AL_CX_MAX ) {
            bc = (struct BrokerCopy *)n;
            al_copy(ents[count].name, bc->bc_Name, CBD_NAMELEN);
            if( bc->bc_Title[0] ) {
                al_copy(ents[count].title, bc->bc_Title, CBD_TITLELEN);
            } else {
                al_copy(ents[count].title, bc->bc_Name, CBD_TITLELEN);
            }
            count++;
            n = n->ln_Succ;
        }
        FreeBrokerList(&blist);
    }

    made = 0;
    ok = TRUE;
    noneItem = NULL;
    for( i = 0; i < count; i++ ) {
        items[i] = NULL;
        subs[i] = NULL;
    }

    for( i = 0; i < count; i++ ) {
        subs[i] = PM_MakeMenu(
            PM_Item, PM_MakeItem(PM_Title, ents[i].title,
                PM_NoSelect, TRUE, PM_ShinePen, TRUE, TAG_DONE),
            PM_Item, PM_MakeItem(PM_WideTitleBar, TRUE, TAG_DONE),
            PM_Item, PM_MakeItem(PM_Title, showStr,
                PM_UserData, (ULONG)((i << AL_CX_SHIFT) | AL_CX_SHOW),
                TAG_DONE),
            PM_Item, PM_MakeItem(PM_Title, hideStr,
                PM_UserData, (ULONG)((i << AL_CX_SHIFT) | AL_CX_HIDE),
                TAG_DONE),
            PM_Item, PM_MakeItem(PM_Title, quitStr,
                PM_UserData, (ULONG)((i << AL_CX_SHIFT) | AL_CX_QUIT),
                TAG_DONE),
            TAG_DONE);
        if( subs[i] == NULL ) {
            ok = FALSE;
            break;
        }
        items[i] = PM_MakeItem(PM_Title, ents[i].title,
            PM_Sub, subs[i], TAG_DONE);
        if( items[i] == NULL ) {
            PM_FreePopupMenu(subs[i]);
            subs[i] = NULL;
            ok = FALSE;
            break;
        }
        made++;
    }

    menu = NULL;
    if( ok ) {
        t = 0;
        mt[t].ti_Tag = PM_Item;
        mt[t].ti_Data = (ULONG)PM_MakeItem(PM_Hidden, TRUE, TAG_DONE);
        t++;
        mt[t].ti_Tag = PM_Item;
        mt[t].ti_Data = (ULONG)PM_MakeItem(PM_Title, title,
            PM_NoSelect, TRUE, PM_ShinePen, TRUE, PM_Shadowed, TRUE,
            PM_Center, TRUE, TAG_DONE);
        t++;
        mt[t].ti_Tag = PM_Item;
        mt[t].ti_Data = (ULONG)PM_MakeItem(PM_WideTitleBar, TRUE, TAG_DONE);
        t++;
        if( count == 0 ) {
            noneItem = PM_MakeItem(PM_Title, noneStr,
                PM_Disabled, TRUE, TAG_DONE);
            if( noneItem == NULL ) {
                ok = FALSE;
            } else {
                mt[t].ti_Tag = PM_Item;
                mt[t].ti_Data = (ULONG)noneItem;
                t++;
            }
        } else {
            for( i = 0; i < count; i++ ) {
                mt[t].ti_Tag = PM_Item;
                mt[t].ti_Data = (ULONG)items[i];
                t++;
            }
        }
        if( ok ) {
            mt[t].ti_Tag = PM_Item;
            mt[t].ti_Data = (ULONG)PM_MakeItem(PM_WideTitleBar, TRUE, TAG_DONE);
            t++;
            mt[t].ti_Tag = PM_Item;
            mt[t].ti_Data = (ULONG)PM_MakeItem(PM_Title, removeStr,
                PM_UserData, AL_POP_REMOVE, TAG_DONE);
            t++;
            mt[t].ti_Tag = TAG_DONE;
            menu = PM_MakeMenuA(mt);
        }
    }

    if( menu == NULL ) {
        for( i = 0; i < made; i++ ) {
            if( items[i] ) {
                PM_FreePopupMenu(items[i]);
            }
        }
        if( noneItem ) {
            PM_FreePopupMenu(noneItem);
        }
        FreeMem(ents, sizeof(struct AlCxEntry) * AL_CX_MAX);
        return AL_POP_NONE;
    }

    result = PM_OpenPopupMenu(win, PM_Menu, menu, TAG_DONE);
    PM_FreePopupMenu(menu);

    if( result == AL_POP_REMOVE ) {
        FreeMem(ents, sizeof(struct AlCxEntry) * AL_CX_MAX);
        return AL_POP_REMOVE;
    }

    if( result && CxBase ) {
        idx = (UWORD)(result >> AL_CX_SHIFT);
        act = (UWORD)(result & 15);
        if( idx < count && ents[idx].name[0] ) {
            cmd = CXCMD_APPEAR;
            if( act == AL_CX_HIDE ) {
                cmd = CXCMD_DISAPPEAR;
            } else if( act == AL_CX_QUIT ) {
                cmd = CXCMD_KILL;
            }
            BrokerCommand(ents[idx].name, cmd);
        }
    }

    FreeMem(ents, sizeof(struct AlCxEntry) * AL_CX_MAX);
    return AL_POP_NONE;
}
