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
#include "applife.h"

#include <devices/inputevent.h>
#include <clib/commodities_protos.h>

#define CX_RMB_ID  0x7FFF

STATIC VOID attach_rmb_filter(struct DockWindow *dock)
{
    struct InputXpression ix;
    CxObj *filter;
    CxObj *sender;

    if( dock->cxBroker == NULL || dock->cxPort == NULL ) {
        return;
    }
    if( ! applife_have_popup() ) {
        return;
    }

    /* MENUUP, not MENUDOWN. Opening popupmenu on press deadlocks against
       the still-in-flight down event (same pattern as Menu/menu_cx.c). */
    ix.ix_Version = IX_VERSION;
    ix.ix_Class = IECLASS_RAWMOUSE;
    ix.ix_Code = (UWORD)(IECODE_RBUTTON | IECODE_UP_PREFIX);
    ix.ix_CodeMask = 0xFFFF;
    ix.ix_Qualifier = 0;
    ix.ix_QualMask = 0;
    ix.ix_QualSame = 0;

    filter = CxFilter("rawmouse");
    if( filter == NULL ) {
        return;
    }
    SetFilterIX(filter, &ix);
    if( CxObjError(filter) ) {
        DeleteCxObj(filter);
        return;
    }

    sender = CxSender(dock->cxPort, CX_RMB_ID);
    if( sender == NULL ) {
        DeleteCxObj(filter);
        return;
    }

    /* No CxTranslate: Intuition still sees the RMB. */
    AttachCxObj(filter, sender);
    AttachCxObj(dock->cxBroker, filter);
    dock->rmbCxWatch = TRUE;
}

BOOL create_cx_broker(struct DockWindow *dock)
{
    struct NewBroker nb = {
        NB_VERSION,
        APP_NAME,
        APP_NAME,
        0,
        NBU_UNIQUE, COF_SHOW_HIDE, 0, 0, 0
    };

    DEBUG(DB_Printf("create_cx_broker\n"));

    nb.nb_Descr = (STRPTR)MSG_CX_Description;

    if( dock->cxPort = CreateMsgPort() ) {

        nb.nb_Port = dock->cxPort;

        if( dock->cxBroker = CxBroker(&nb, NULL) ) {

            return TRUE;   
        }

    }

    DEBUG(DB_Printf(" Failed\n"));

    return FALSE;
}

VOID free_cx_broker(struct DockWindow *dock) 
{
    DEBUG(DB_Printf("free_cx_broker\n"));

    if( dock->cxBroker ) {

        DeleteCxObjAll(dock->cxBroker);
        dock->cxBroker = NULL;
    }
    dock->rmbCxWatch = FALSE;

    if( dock->cxPort ) {
        delete_port(dock->cxPort);
        dock->cxPort = NULL;
    }
}

BOOL init_cx_broker(struct DockWindow *dock)
{
    struct DgNode *curr;
    STRPTR hotKey;
    CxObj *filter, *sender, *translate;
    UWORD msgId;    
    
    DEBUG(DB_Printf("init_cx_broker\n"));

    if( dock->cxBroker ) {
        free_cx_broker(dock);
    } 
    
    if( create_cx_broker(dock) ) {

        msgId = 0;

        FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {

            dock_gadget_get_hotkey(curr->dg, &hotKey);
            if( hotKey ) {
                if( filter = CxFilter(hotKey) ) {
                                    
                    AttachCxObj(dock->cxBroker, filter);

                    if( sender = CxSender(dock->cxPort, msgId) ) {

                        AttachCxObj(filter, sender);

                        if( translate = CxTranslate(NULL) ) {

                            AttachCxObj(filter, translate);
    
                            if( CxObjError(filter) ) {
    DEBUG(DB_Printf(" Failed 1\n"));
                                return FALSE;
                            }
                        } else {
    DEBUG(DB_Printf(" Failed 2\n"));
                            return FALSE;
                        }
                    } else {
    DEBUG(DB_Printf(" Failed 3\n"));
                        return FALSE;
                    }

                } else {
    DEBUG(DB_Printf(" Failed 4\n"));
                    return FALSE;
                }
            }
            msgId++;
        }

        attach_rmb_filter(dock);

        ActivateCxObj(dock->cxBroker, 1L);

        return TRUE;
    }

    DEBUG(DB_Printf(" Failed 5\n"));
    return FALSE;
}


VOID handle_cx_message(struct DockWindow *dock)
{
    CxMsg *msg;
    ULONG msgType, msgId, ix;
    struct DgNode *curr;
    struct InputEvent *ie;
    BOOL rmbUp;

    while( msg = (CxMsg*)GetMsg(dock->cxPort) ) {

        msgType = CxMsgType(msg);
        msgId = CxMsgID(msg);
        rmbUp = FALSE;
        if( msgType == CXM_IEVENT && msgId == CX_RMB_ID ) {
            ie = (struct InputEvent *)CxMsgData(msg);
            if( ie != NULL
                && ie->ie_Class == IECLASS_RAWMOUSE
                && ie->ie_Code == (UWORD)(IECODE_RBUTTON | IECODE_UP_PREFIX) ) {
                rmbUp = TRUE;
            }
        }

        ReplyMsg((struct Message *)msg);

        switch(msgType) {
            
            case CXM_COMMAND:
                switch( msgId ) {
                    case CXCMD_DISABLE:
                        if( dock->cxBroker ) {
                            ActivateCxObj(dock->cxBroker, 0L);
                        }
                        break;
            
                    case CXCMD_ENABLE:
                        if( dock->cxBroker ) {
                            ActivateCxObj(dock->cxBroker, 1L);    
                        }
                        break;

                    case CXCMD_KILL:
                        dock->runState = RS_QUITTING;
                        break;

                    case CXCMD_APPEAR:
                        if( ! dock->win ) {
                            dock->runState = RS_UNICONIFYING;
                        }
                        break;

                    case CXCMD_DISAPPEAR:
                        if( dock->win ) {
                            dock->runState = RS_ICONIFYING;
                        }
                        break;
                }
                break;

            case CXM_IEVENT:
                if( msgId == CX_RMB_ID ) {
                    if( rmbUp ) {
                        handle_cx_rmb(dock);
                    }
                    break;
                }

                ix = 0;

                FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {

                    if( ix == msgId ) {
                        dock_gadget_hotkey(curr->dg);
                        break;
                    }
                    ix++;
                }
                break;
        }
    }
}
