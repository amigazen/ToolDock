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
#include "rearrange.h"
#include "dropzone.h"
#include "td_request.h"

#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <clib/dos_protos.h>
#include <clib/graphics_protos.h>

#define ICON_PATH_HI "PROGDIR:icons_hi"
#define ICON_PATH_MED "PROGDIR:icons_med"

STATIC VOID ensure_placeholder(struct DockWindow *dock);
STATIC VOID remove_placeholder(struct DockWindow *dock);

VOID free_config(struct DockWindow *dock)
{
    dock->placeholder = NULL;
    dock->cfg.align = DA_CENTER;
    dock->cfg.pos = DP_RIGHT;
    dock->cfg.showGadgetLabels = TRUE;
    dock->cfg.showGadgetBorders = TRUE;

    /* Gadgets are disposed by remove_dock_gadgets / TD_DisposeConfig. */
    NewList(&dock->cfg.gadgets);

    if( dock->bgBrush ) {
        TD_FreeBrush(dock->bgBrush);
        dock->bgBrush = NULL;
    }

    FREE_STRING(dock->cfg.bgBrushPath);
}

STATIC VOID drain_gadget_messages(struct DockWindow *dock)
{
    struct GadgetMessage *msg;

    if( dock->gadgetPort == NULL ) {
        return;
    }
    while( msg = (struct GadgetMessage *)GetMsg(dock->gadgetPort) ) {
        TD_FreeMem(msg, msg->m.mn_Length);
    }
}

VOID reload_config(struct DockWindow *dock)
{
    DEBUG(DB_Printf(__FUNC__ "\n"));

    rearrange_cancel(dock);
    dz_remove(dock);

    dock->hoverGad = NULL;
    dock->editNode = NULL;
    dock->dragNode = NULL;
    dock->placeholder = NULL;
    dock->reloadPending = FALSE;

    drain_gadget_messages(dock);
    disable_layout(dock);
    remove_dock_gadgets(dock);
    load_config(dock);
}

BOOL load_config(struct DockWindow *dock)
{
    struct DockSettings *s;
    struct DgNode *curr;
    BOOL r = FALSE;

    DEBUG(DB_Printf(__FUNC__ ": %s\n", CONFIG_FILE));

    free_config(dock);

    create_dock_handle(dock);

    if( s = TD_OpenSettingsRead(CONFIG_FILE) ) {

        DEBUG(DB_Printf(__FUNC__ ":  settings opened\n"));

        if( TD_ReadConfig(&dock->cfg, s) ) {
            DEBUG(DB_Printf(__FUNC__ ":  config read\n"));
            r = TRUE;
        } else {
            DEBUG(DB_Printf(__FUNC__ ":  config incomplete\n"));
        }

        if( dock->cfg.bgBrushPath ) {

            DEBUG(DB_Printf(__FUNC__ ":  load background %s...", dock->cfg.bgBrushPath));
            if( dock->bgBrush = TD_LoadBrush(dock->cfg.bgBrushPath, BF_NONE) ) {
                DEBUG(DB_Printf("OK\n"));
            } else {
                DEBUG(DB_Printf("FAILED\n"));
            }
        }

        FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {

            dock_gadget_added(curr->dg, dock->gadgetPort);
        }

        TD_CloseSettings(s);
    }

    ensure_placeholder(dock);
    layout_gadgets(dock);
    return r;    
}

STATIC VOID remove_placeholder(struct DockWindow *dock)
{
    struct DgNode *ph;

    ph = dock->placeholder;
    if( ph == NULL ) {
        return;
    }
    dock->placeholder = NULL;
    Remove((struct Node *)ph);
    TD_FreeGadget(ph);
}

STATIC VOID create_placeholder(struct DockWindow *dock)
{
    struct DgNode *dg;
    UBYTE path[256];
    STRPTR pos;
    UWORD l;

    if( dock->placeholder ) {
        return;
    }
    pos = (STRPTR)&path;
    COPY_STRING((STRPTR)dock->progPath, pos);
    pos--;
    COPY_STRING("/Dock", pos);
    pos--;
    *pos = '\0';

    if( dg = TD_AllocGadget(DB_BUTTON_CLASS) ) {
        dock_gadget_init_button(dg->dg, (STRPTR)MSG_MENU_OpenPrefs, path);
        if( dock->gadgetPort ) {
            dock_gadget_added(dg->dg, dock->gadgetPort);
        }
        AddTail(&dock->cfg.gadgets, (struct Node *)dg);
        dock->placeholder = dg;
        DEBUG(DB_Printf("create_placeholder\n"));
    }
}

STATIC VOID ensure_placeholder(struct DockWindow *dock)
{
    struct DgNode *curr;
    UWORD n;

    n = 0;
    FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {
        if( curr != dock->placeholder ) {
            n++;
        }
    }
    if( n <= 1 ) {
        if( dock->placeholder == NULL ) {
            create_placeholder(dock);
        }
    } else if( dock->placeholder ) {
        remove_placeholder(dock);
    }
}

VOID write_config(struct DockWindow *dock, STRPTR path)
{
    struct DockSettings *s;
    struct DgNode *ph;
    struct Node *pred;

    ph = dock->placeholder;
    pred = NULL;
    if( ph ) {
        pred = ph->n.ln_Pred;
        Remove((struct Node *)ph);
    }

    if( s = TD_OpenSettingsWrite(path) ) {

        TD_WriteConfig(&dock->cfg, s);

        TD_CloseSettings(s);
    }

    if( ph ) {
        Insert(&dock->cfg.gadgets, (struct Node *)ph, pred);
        dock->placeholder = ph;
    }
}

VOID request_remove_gadget(struct DockWindow *dock, struct DgNode *dg)
{
    LONG ans;

    if( dock == NULL || dg == NULL || dock->win == NULL ) {
        return;
    }
    if( dock->handleClass && dg->dg && OCLASS(dg->dg) == dock->handleClass ) {
        return;
    }
    if( dg == dock->placeholder ) {
        return;
    }

    /* Blocks the dock window while the requester is open. */
    ans = TD_OpenRequest(dock->win,
        (STRPTR)MSG_DEL_Title,
        (STRPTR)MSG_DEL_Text,
        (STRPTR)MSG_DEL_Buttons,
        TDREQIMAGE_QUESTION);
    if( ans != 1 ) {
        return;
    }

    rearrange_cancel(dock);
    if( dock->hoverGad == dg->dg ) {
        dock->hoverGad = NULL;
    }
    if( dock->editNode == dg ) {
        dock->editNode = NULL;
    }

    Remove((struct Node *)dg);
    TD_FreeGadget(dg);

    ensure_placeholder(dock);
    layout_gadgets(dock);
    update_hover_gadget(dock);
    draw_gadgets(dock);
    save_config(dock);
}

VOID save_config(struct DockWindow *dock)
{
    struct Message *msg;

    /* Writing ENV: must not reload the dock. That OpenWindow/rebuild is
       the empty full-height window seen after a drop. */
    disable_notification(dock);
    write_config(dock, CONFIG_FILE_PERM);
    write_config(dock, CONFIG_FILE);
    if( dock->notifyPort ) {
        while( msg = GetMsg(dock->notifyPort) ) {
            if( msg->mn_ReplyPort ) {
                ReplyMsg(msg);
            }
        }
    }
    enable_notification(dock);
}

VOID disable_notification(struct DockWindow *dock)
{
    if( ! dock->notifyEnabled ) {
        return;
    }

    dock->notifyEnabled = FALSE;       

    if( dock->notifySupported ) {
        EndNotify(&dock->notifyReq);
    }
}   

VOID enable_notification(struct DockWindow *dock)
{
    if( dock->notifyEnabled ) {
        return;
    }

    if( StartNotify(&dock->notifyReq) ) {
        dock->notifyEnabled = TRUE;
        dock->notifySupported = TRUE;
    
    } else {
    
        DEBUG(DB_Printf("init_config_notification: FS does not support notification.\n"));
    }
}   


BOOL init_config_notification(struct DockWindow *dock)
{
    DEBUG(DB_Printf("init_config_notification\n"));

    if( dock->notifyPort = CreateMsgPort() ) {
        /* No NRF_WAIT_REPLY: a prefs Close() on ENV: must not stall
           this task, and EndNotify must not fight an in-flight reply. */
        dock->notifyReq.nr_Name = CONFIG_FILE;
        dock->notifyReq.nr_Flags = NRF_SEND_MESSAGE;
        dock->notifyReq.nr_stuff.nr_Msg.nr_Port = dock->notifyPort;
    
        enable_notification(dock);

        return TRUE;
    }

    DEBUG(DB_Printf(" Failed\n"));
    return FALSE; 
}

VOID free_config_notification(struct DockWindow *dock)
{
    DEBUG(DB_Printf("free_config_notification\n"));

    if( dock->notifyPort ) {

        disable_notification(dock);

        delete_port(dock->notifyPort);
    }
}

VOID handle_notify_message(struct DockWindow *dock)
{
    struct Message *msg;

    DEBUG(DB_Printf("handle_notify_message\n"));

    while( msg = GetMsg(dock->notifyPort) ) {
        if( msg->mn_ReplyPort ) {
            ReplyMsg(msg);
        }
    }

    if( ! dock->notifyEnabled ) {
        return;
    }

    /* Rebuild only from a quiet running/hidden dock. Edit, rearrange
       and Workbench hover keep the current gadgets and retry later. */
    if( dock->hoverOverDock
        || dock->runState == RS_EDITING
        || dock->runState == RS_CHANGING
        || dock->runState == RS_REARRANGING
        || dock->runState == RS_START_EDIT
        || dock->runState == RS_STOP_EDIT ) {
        dock->reloadPending = TRUE;
        return;
    }

    if( dock->runState == RS_RUNNING || dock->runState == RS_HIDDEN ) {
        dock->runState = RS_LOADING;
    } else {
        dock->reloadPending = TRUE;
    }
}

VOID open_settings(struct DockWindow *dock)
{
    UWORD l;
    UBYTE path[256];
    STRPTR pos = (STRPTR)&path;

    COPY_STRING((STRPTR)dock->progPath, pos);
    pos--;
    COPY_STRING("/ToolDockPrefs", pos);
    pos--;
    *pos = '\0';

    DEBUG(DB_Printf("open_settings: %s\n", path));

    execute_external(dock, (STRPTR)&path, NULL, NULL, TRUE);
}

VOID open_dock_prefs(struct DockWindow *dock)
{
    UWORD l;
    UBYTE path[256];
    STRPTR pos;

    pos = (STRPTR)&path;

    COPY_STRING((STRPTR)dock->progPath, pos);
    pos--;
    COPY_STRING("/Dock", pos);
    pos--;
    *pos = '\0';

    DEBUG(DB_Printf("open_dock_prefs: %s\n", path));

    execute_external(dock, (STRPTR)&path, NULL, NULL, TRUE);
}

VOID load_icon_brushes(struct DockWindow *dock)
{
    struct Screen *screen;
    struct DisplayInfo dispInfo;
    ULONG dispMode;
    UBYTE xAspect = 1, yAspect = 1;
    STRPTR path;
    UWORD w, h;

    DEBUG(DB_Printf(__FUNC__ ": Loading edit icons\n"));
   
    if( dock->iconBrush != NULL ) {
        DEBUG(DB_Printf(__FUNC__ ": Already loaded.\n"));
        return;
    }

    if( screen = LockPubScreen(NULL) ) {

        dispMode = GetVPModeID(&screen->ViewPort);
        if( GetDisplayInfoData(NULL, (UBYTE *)&dispInfo, sizeof(struct DisplayInfo), DTAG_DISP, dispMode) ) {

            xAspect = dispInfo.Resolution.x;
            yAspect = dispInfo.Resolution.y;
        }

        UnlockPubScreen(NULL, screen);
    }

    if( xAspect / yAspect < 2 ) {
        path = ICON_PATH_HI;        
    } else {
        path = ICON_PATH_MED;
    }    

    DEBUG(DB_Printf(__FUNC__ ": Loading %s\n", path));
    dock->iconBrush = TD_LoadBrush(path, BF_CREATE_MASK);

    TD_GetBrushSize(dock->iconBrush, &w, &h);

    dock->iconW = w / ICON_COUNT;
    dock->iconH = h;
}

VOID free_icon_brushes(struct DockWindow *dock)
{
    DEBUG(DB_Printf(__FUNC__ "\n"));

    if( dock->iconBrush ) {
        TD_FreeBrush(dock->iconBrush);
    }
}

VOID handle_change_config(struct DockWindow *dock)
{
    struct Node *prev;

    DEBUG(DB_Printf(__FUNC__ ": editCount = %d\n", dock->editCount));

    if( dock->editCount == 0 ) {

        switch( dock->editOp ) {

            case EO_MOVE_UP:
                DEBUG(DB_Printf(__FUNC__ ": Move Up %s\n", dock->editNode->n.ln_Name));
                prev = dock->editNode->n.ln_Pred->ln_Pred;
                if( prev ) {
                    Remove((struct Node *)dock->editNode);
                    Insert(&dock->cfg.gadgets, (struct Node *)dock->editNode, prev);
                }
                break;
            
            case EO_MOVE_DOWN:
                DEBUG(DB_Printf(__FUNC__ ": Move Down %s\n", dock->editNode->n.ln_Name));

                prev = dock->editNode->n.ln_Succ;
                if( prev ) {
                    Remove((struct Node *)dock->editNode);
                    Insert(&dock->cfg.gadgets, (struct Node *)dock->editNode, prev);
                }
                break;

            case EO_DELETE:
                DEBUG(DB_Printf(__FUNC__ ": Delete %lx - %s\n", dock->editNode, dock->editNode->n.ln_Name));

                Remove((struct Node *)dock->editNode);

                DEBUG(DB_Printf(__FUNC__ ": Removed\n"));                
                TD_FreeGadget(dock->editNode);
                DEBUG(DB_Printf(__FUNC__ ": Deleted\n"));                
            
                break;                
        }

        dock->editNode = NULL;
        dock->hoverGad = NULL;
        dock->runState = RS_EDITING;

        ensure_placeholder(dock);
        layout_gadgets(dock);
        update_hover_gadget(dock);
        draw_gadgets(dock);

    } else {

        dock->editCount--;

        draw_gadget(dock, dock->editNode->dg);
    }
}

BOOL insert_dropped_icon(struct DockWindow *dock, BPTR dir, STRPTR name,
    struct DgNode *before)
{
    STRPTR path;
    struct FileInfoBlock *fib;
    struct DgNode *dg;
    BPTR lock;
    BOOL ok;

    ok = FALSE;
    DEBUG(DB_Printf(__FUNC__ "\n"));

    if( dock->placeholder ) {
        if( before == dock->placeholder ) {
            before = NULL;
        }
        remove_placeholder(dock);
    }

    if( path = (STRPTR)TD_AllocMem(MAX_PATH_LENGTH, MEMF_ANY) ) {

        path[0] = 0;
        NameFromLock(dir, path, MAX_PATH_LENGTH);
        AddPart(path, name, MAX_PATH_LENGTH);

        DEBUG(DB_Printf(__FUNC__ ":  -> %s\n", path));

        if( lock = Lock(path, ACCESS_READ) ) {

            if( fib = AllocDosObjectTags(DOS_FIB, TAG_DONE) ) {

                if( Examine(lock, fib) ) {

                   if( fib->fib_DirEntryType < 0 ) {

                        if( dg = TD_AllocGadget(DB_BUTTON_CLASS) ) {

                            dock_gadget_init_button(dg->dg, name, path);
                            if( dock->gadgetPort ) {
                                dock_gadget_added(dg->dg, dock->gadgetPort);
                            }
                            dock_gadget_remap(dg->dg);

                            if( before ) {
                                Insert(&dock->cfg.gadgets, (struct Node *)dg,
                                    before->n.ln_Pred);
                            } else {
                                AddTail(&dock->cfg.gadgets, (struct Node *)dg);
                            }
                            ok = TRUE;
                        }

                    } else {
                        DEBUG(DB_Printf(__FUNC__ ": skip directory\n"));
                    }
                }

                FreeDosObject(DOS_FIB, fib);
            }
            UnLock(lock);
        }

        TD_FreeMem(path, MAX_PATH_LENGTH);
    }
    return ok;
}

VOID add_dropped_icon(struct DockWindow *dock, BPTR dir, STRPTR name)
{
    insert_dropped_icon(dock, dir, name, NULL);
}
