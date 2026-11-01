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

#include <exec/io.h>
#include <devices/timer.h>
#include <clib/exec_protos.h>
#include <clib/alib_protos.h>

#include "debug.h"
#include "rearrange.h"

BOOL init_timer_notification(struct DockWindow *dock)
{
    DEBUG(DB_Printf("init_timer_notification\n"));

    if( dock->timerPort = CreateMsgPort() ) {

        if( dock->timerReq = (struct timerequest *)CreateExtIO(dock->timerPort, sizeof(struct timerequest) ) ) {
            
            if( OpenDevice(TIMERNAME, UNIT_VBLANK, (struct IORequest *)dock->timerReq, 0L) == 0 ) {

                return TRUE;
            }
        }
    }

    DEBUG(DB_Printf(" Failed\n"));
    return FALSE;
}

VOID free_timer_notification(struct DockWindow *dock)
{
    DEBUG(DB_Printf("free_timer_notification\n"));

    if( dock->timerPort ) {

        if( dock->timerReq ) {

            CloseDevice((struct IORequest *)dock->timerReq);    
            DeleteExtIO((struct IORequest *)dock->timerReq);    
        }    

        delete_port(dock->timerPort);
    }
}

VOID set_timer(struct DockWindow *dock, ULONG milliseconds) 
{
    dock->timerReq->tr_node.io_Command = TR_ADDREQUEST;
    dock->timerReq->tr_time.tv_secs = milliseconds / 1000;
    dock->timerReq->tr_time.tv_micro = (milliseconds % 1000) * 1000;

    SendIO((struct IORequest *)dock->timerReq);
}


VOID handle_timer_message(struct DockWindow *dock)
{
    struct DgNode *curr;
    struct Message *msg;

    while( msg = GetMsg(dock->timerPort) ) {
    }

    /* wb.doc: Workbench holds the display for the whole icon drag.
       The hook draws. This task must not call Intuition. */
    if( dock->hoverOverDock ) {
        if( dock->runState == RS_QUITTING ) {
            dock->runState = RS_STOPPED;
        } else if( dock->runState != RS_STOPPED ) {
            set_timer(dock, TIMER_INTERVAL);
        }
        return;
    }

    if( dock->runState == RS_RUNNING
        || dock->runState == RS_CHANGING
        || dock->runState == RS_EDITING
        || dock->runState == RS_REARRANGING ) {

        FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {

            dock_gadget_tick(curr->dg);
        }
    }

    switch( dock->runState ) {

        case RS_QUITTING:
            dock->runState = RS_STOPPED;
            break;

        case RS_CHANGING:
            handle_change_config(dock);

            set_timer(dock, TIMER_INTERVAL);
            break;            

        case RS_REARRANGING:
            rearrange_tick(dock);
            set_timer(dock, REARRANGE_TICK_MS);
            break;

        case RS_EDITING:
        case RS_RUNNING:
            rearrange_tick(dock);
            update_hover_gadget(dock);

            set_timer(dock, TIMER_INTERVAL);
            break;

        case RS_HIDDEN:
            set_timer(dock, TIMER_INTERVAL);
            break;
        
        default:
            break;
    }
}
