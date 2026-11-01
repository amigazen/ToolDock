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

#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <clib/dos_protos.h>
#include <clib/wb_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/graphics_protos.h>
#include <clib/layers_protos.h>

#include "dock_handle.h"
#include "rearrange.h"
#include "applife.h"
#include <libraries/pm.h>
#include <proto/popupmenu.h>

enum {
    MENU_IDX_Project   =  0,
    MENU_IDX_About     =  1,
    MENU_IDX_Help      =  2,
    MENU_IDX_Iconify   =  4,
    MENU_IDX_Quit      =  6,
    MENU_IDX_Settings  =  7,
    MENU_IDX_OpenPrefs =  8
};

struct NewMenu mainMenu[] = {

    { NM_TITLE, 0 /*MSG_MENU_Project*/,     0, 0, 0, 0 },
    {  NM_ITEM, 0 /*MSG_MENU_About*/,       0, 0, 0, (APTR)MI_ABOUT },
    {  NM_ITEM, 0 /*MSG_MENU_Help*/,        0, 0, 0, (APTR)MI_HELP },
    {  NM_ITEM, NM_BARLABEL,                0, 0, 0, 0 }, 
    {  NM_ITEM, 0 /*MSG_MENU_Iconify*/,     0, 0, 0, (APTR)MI_HIDE },
    {  NM_ITEM, NM_BARLABEL,                0, 0, 0, 0 }, 
    {  NM_ITEM, 0 /*MSG_MENU_Quit*/,        0, 0, 0, (APTR)MI_QUIT },

    { NM_TITLE, 0 /*MSG_MENU_Settings*/,    0, 0, 0, 0 },
    {  NM_ITEM, 0 /*MSG_MENU_OpenPrefs*/,   0, 0, 0, (APTR)MI_OPEN_PREFS }, 

    { NM_END,   0,                       0, 0, 0, 0 }
};


BOOL build_menu(struct DockWindow *dock) {

    BOOL result = FALSE;
	struct Screen *screen;
    struct DrawInfo *di;
    APTR* vi;

    DEBUG(DB_Printf("build_menu\n"));

    mainMenu[MENU_IDX_Project].nm_Label    = (STRPTR)MSG_MENU_Project;
    mainMenu[MENU_IDX_About].nm_Label      = (STRPTR)MSG_MENU_About;
    mainMenu[MENU_IDX_About].nm_CommKey    = (STRPTR)MSG_KEY_About;
    mainMenu[MENU_IDX_Help].nm_Label       = (STRPTR)MSG_MENU_Help;
    mainMenu[MENU_IDX_Iconify].nm_Label    = (STRPTR)MSG_MENU_Iconify;
    mainMenu[MENU_IDX_Iconify].nm_CommKey  = (STRPTR)MSG_KEY_Iconify;
    mainMenu[MENU_IDX_Quit].nm_Label       = (STRPTR)MSG_MENU_Quit;
    mainMenu[MENU_IDX_Quit].nm_CommKey     = (STRPTR)MSG_KEY_Quit;

    mainMenu[MENU_IDX_Settings].nm_Label   = (STRPTR)MSG_MENU_Settings;
    mainMenu[MENU_IDX_OpenPrefs].nm_Label   = (STRPTR)MSG_MENU_OpenPrefs;
    mainMenu[MENU_IDX_OpenPrefs].nm_CommKey = (STRPTR)MSG_KEY_OpenPrefs;

	if( screen = LockPubScreen(NULL) ) {

        if( di = GetScreenDrawInfo(screen) ) {

            if( vi = GetVisualInfo(dock->win->WScreen, TAG_END) ) {
    
                if( dock->menu = CreateMenus(mainMenu, GTMN_FrontPen, di->dri_Pens[TEXTPEN], TAG_END) ) {           

                    if( LayoutMenus(dock->menu, vi, TAG_END) ) {

                        if( SetMenuStrip(dock->win, dock->menu) ) {

                            result = TRUE;
                        }
                    }
                }
                FreeVisualInfo(vi);  
            }
            FreeScreenDrawInfo(screen, di);   
        }
        UnlockPubScreen(NULL, screen);        
    }

    if( ! result ) {
        DEBUG(DB_Printf(" Failed\n"));
    }

    return result;
}

struct MenuItem *find_menu_item(struct Menu *menu, MenuIndex ix)
{
    struct MenuItem *item = menu->FirstItem; 

    while( item ) {
        if( (MenuIndex)GTMENUITEM_USERDATA(item) == ix ) {
            return item;
        }
    
        item = item->NextItem;
    }

    if( menu->NextMenu ) {
        return find_menu_item(menu->NextMenu, ix);
    }

    return NULL;
}

VOID update_settings_menu(struct DockWindow *dock)
{
    if( dock->win == NULL || dock->menu == NULL ) {
        return;
    }
    ResetMenuStrip(dock->win, dock->menu);
}

BOOL show_dock_window(struct DockWindow *dock)
{
    BOOL result = FALSE;
    UWORD dw;
    UWORD dh;
    WORD left;
    WORD top;
    struct Screen *screen;
    struct TagItem tags[13];

    /* SimpleDock: OpenWindow at the final box. No 0,0 flash, no 1x1
       AppWindow that rejects every drop zone. */
    dw = DEFAULT_SIZE;
    dh = DEFAULT_SIZE;
    left = 0;
    top = 0;
    screen = LockPubScreen(NULL);
    if( screen ) {
        measure_dock_size(dock, &dw, &dh);
        if( dw < 8 ) {
            dw = DEFAULT_SIZE;
        }
        if( dh < 8 ) {
            dh = DEFAULT_SIZE;
        }
        left = (WORD)get_window_left(screen, dock->cfg.pos, dock->cfg.align, dw);
        top = (WORD)get_window_top(screen, dock->cfg.pos, dock->cfg.align, dh);
    }
    tags[0].ti_Tag = WA_PubScreen;
    tags[0].ti_Data = (ULONG)screen;
    tags[1].ti_Tag = WA_Left;
    tags[1].ti_Data = (ULONG)left;
    tags[2].ti_Tag = WA_Top;
    tags[2].ti_Data = (ULONG)top;
    tags[3].ti_Tag = WA_Width;
    tags[3].ti_Data = dw;
    tags[4].ti_Tag = WA_Height;
    tags[4].ti_Data = dh;
    tags[5].ti_Tag = WA_Borderless;
    tags[5].ti_Data = TRUE;
    tags[6].ti_Tag = WA_SimpleRefresh;
    tags[6].ti_Data = TRUE;
    tags[7].ti_Tag = WA_NewLookMenus;
    tags[7].ti_Data = TRUE;
    tags[8].ti_Tag = WA_ReportMouse;
    tags[8].ti_Data = TRUE;
    tags[9].ti_Tag = WA_Activate;
    tags[9].ti_Data = FALSE;
    tags[10].ti_Tag = WA_IDCMP;
    tags[10].ti_Data = IDCMP_MOUSEBUTTONS | IDCMP_REFRESHWINDOW
        | IDCMP_CHANGEWINDOW | IDCMP_MENUPICK | IDCMP_MOUSEMOVE;
    tags[11].ti_Tag = WA_RMBTrap;
    tags[11].ti_Data = applife_have_popup() ? TRUE : FALSE;
    tags[12].ti_Tag = TAG_DONE;
    tags[12].ti_Data = 0;

    DEBUG(DB_Printf("show_dock_window %ld,%ld %ld x %ld\n",
        (LONG)left, (LONG)top, (LONG)dw, (LONG)dh));

	if( dock->win = OpenWindowTagList(NULL, tags) ) {
	
        if( build_menu(dock) ) {

            if( dock->awPort = CreateMsgPort() ) {

                if( dock->appWin = AddAppWindowA(1, 0, dock->win, dock->awPort, NULL) ) {

                    result = TRUE;
                                    
                } else {
                    DEBUG(DB_Printf(" AddAppWindow Failed\n"));
                }
            
            } else {
                DEBUG(DB_Printf(" CreateMsgPort Failed\n"));
            }
        }
    } else {
       DEBUG(DB_Printf(" OpenWindow Failed\n"));
    }
    if( screen ) {
        UnlockPubScreen(NULL, screen);
    }
    return result;     
}

VOID hide_dock_window(struct DockWindow *dock)
{
    struct Message *amsg;

    DEBUG(DB_Printf("hide_dock_window\n"));

    if( dock->awPort ) {
        while( amsg = GetMsg(dock->awPort) ) {
            ReplyMsg(amsg);
        }
    }

    if( dock->appWin ) {
        rearrange_remove_zone(dock);
        RemoveAppWindow(dock->appWin);
        dock->appWin = NULL;
    }

    if( dock->awPort ) {
        while( amsg = GetMsg(dock->awPort) ) {
            ReplyMsg(amsg);
        }
    }

    delete_port(dock->awPort);
    dock->awPort = NULL;

    if( dock->win ) {

        if( dock->menu ) {

            ClearMenuStrip(dock->win);
            FreeMenus(dock->menu);
            dock->menu = NULL;
        }

    	CloseWindow(dock->win);
        dock->win = NULL;
    }
}

VOID launch_dropped_icon(struct DockWindow *dock, struct AppMessage *msg)
{
    struct WBArg *arg;
    UWORD i, len;
    STRPTR tmpBuf = NULL;
    STRPTR* buffers;
    struct DgNode *dg;
    Object *gadget;

    if( dg = get_gadget_at(dock, msg->am_MouseX, msg->am_MouseY) ) {
        gadget = dg->dg;
        arg = msg->am_ArgList;

        if( tmpBuf == NULL ) {
            tmpBuf = TD_AllocMem(2048, MEMF_CLEAR);
        }

        buffers = TD_AllocMem(msg->am_NumArgs * sizeof(STRPTR), MEMF_ANY);
        for( i = 0; i < msg->am_NumArgs; i++ ) {

            NameFromLock(arg->wa_Lock, tmpBuf, 2048);
            AddPart(tmpBuf, arg->wa_Name, 2048);
            len = strlen(tmpBuf);

            buffers[i] = TD_AllocMem(len + 1, MEMF_ANY);
            CopyMem(tmpBuf, buffers[i], len + 1);

            arg++;
        }

        dock_gadget_drop(gadget, buffers, msg->am_NumArgs);

        for( i = 0; i < msg->am_NumArgs; i++ ) {
            TD_FreeMem(buffers[i], strlen(buffers[i]) + 1);
        }
        TD_FreeMem(buffers, msg->am_NumArgs * sizeof(STRPTR));
    }

    if( tmpBuf != NULL ) {
        TD_FreeMem(tmpBuf, 2048);
    }
}

VOID handle_dropped_icon(struct DockWindow *dock, struct AppMessage *msg)
{
    add_dropped_icon(dock, msg->am_ArgList->wa_Lock, msg->am_ArgList->wa_Name);
}

STATIC VOID copy_wb_name(UBYTE *dst, STRPTR src)
{
    UWORD n;

    dst[0] = 0;
    if( src == NULL ) {
        return;
    }
    n = 0;
    while( src[n] && n < 107 ) {
        n++;
    }
    if( n ) {
        CopyMem(src, dst, n);
    }
    dst[n] = 0;
}

STATIC VOID path_from_lock(BPTR dir, STRPTR name, STRPTR out, ULONG outLen)
{
    out[0] = 0;
    if( dir == NULL || outLen < 2 ) {
        return;
    }
    NameFromLock(dir, out, outLen);
    if( name && name[0] ) {
        AddPart(out, name, outLen);
    }
}

STATIC VOID launch_copied_drop(Object *gadget, BPTR *dlocks,
    UBYTE dnames[][108], UWORD ncopy)
{
    STRPTR *buffers;
    STRPTR tmpBuf;
    UWORD i;
    UWORD len;

    if( gadget == NULL || ncopy == 0 ) {
        return;
    }
    tmpBuf = (STRPTR)TD_AllocMem(2048, MEMF_CLEAR);
    buffers = (STRPTR *)TD_AllocMem(ncopy * sizeof(STRPTR), MEMF_CLEAR);
    if( tmpBuf == NULL || buffers == NULL ) {
        if( tmpBuf ) {
            TD_FreeMem(tmpBuf, 2048);
        }
        if( buffers ) {
            TD_FreeMem(buffers, ncopy * sizeof(STRPTR));
        }
        return;
    }
    for( i = 0; i < ncopy; i++ ) {
        path_from_lock(dlocks[i], dnames[i], tmpBuf, 2048);
        len = 0;
        while( tmpBuf[len] ) {
            len++;
        }
        buffers[i] = (STRPTR)TD_AllocMem(len + 1, MEMF_CLEAR);
        if( buffers[i] && len ) {
            CopyMem(tmpBuf, buffers[i], len);
        }
    }
    dock_gadget_drop(gadget, buffers, ncopy);
    for( i = 0; i < ncopy; i++ ) {
        if( buffers[i] ) {
            len = 0;
            while( buffers[i][len] ) {
                len++;
            }
            TD_FreeMem(buffers[i], len + 1);
        }
    }
    TD_FreeMem(buffers, ncopy * sizeof(STRPTR));
    TD_FreeMem(tmpBuf, 2048);
}

VOID handle_drop_event(struct DockWindow* dock)
{
    struct AppMessage *msg;
    struct WBArg *arg;
    struct DgNode *before;
    struct DgNode *dg;
    struct GadgetEnvironment env;
    Object *launchGad;
    BPTR dlocks[16];
    UBYTE dnames[16][108];
    UWORD dest;
    UWORD i;
    UWORD ncopy;
    BOOL inserted;
    BOOL launching;

    while( msg = (struct AppMessage *)GetMsg(dock->awPort)) {
        inserted = FALSE;
        launching = FALSE;
        launchGad = NULL;
        ncopy = 0;
        dest = 1;
        for( i = 0; i < 16; i++ ) {
            dlocks[i] = NULL;
        }
        DEBUG(DB_Printf("handle_drop_event type=%ld args=%ld state=%ld\n",
            (LONG)msg->am_Type, (LONG)msg->am_NumArgs, (LONG)dock->runState));
        switch( msg->am_Type ) {
          case AMTYPE_APPWINDOW:
          case AMTYPE_APPWINDOWZONE:

            if( msg->am_NumArgs > 0 ) {

                if( dock->runState == RS_EDITING ) {
                    dest = (UWORD)rearrange_count(dock);
                    inserted = TRUE;

                } else if( dock->runState == RS_RUNNING ) {

                    if( ! rearrange_want_insert(dock, msg, &dest) ) {
                        dg = get_gadget_at(dock, msg->am_MouseX, msg->am_MouseY);
                        env.index = 0;
                        if( dg ) {
                            TD_GetDockGadgetEnvironment(dg->dg, &env);
                        }
                        if( dg && env.index != 0 ) {
                            launchGad = dg->dg;
                            launching = TRUE;
                        } else {
                            dest = 1;
                            inserted = TRUE;
                        }
                    } else {
                        inserted = TRUE;
                    }
                }
                if( inserted || launching ) {
                    ncopy = msg->am_NumArgs;
                    if( ncopy > 16 ) {
                        ncopy = 16;
                    }
                    arg = msg->am_ArgList;
                    for( i = 0; i < ncopy; i++ ) {
                        dlocks[i] = DupLock(arg->wa_Lock);
                        copy_wb_name(dnames[i], arg->wa_Name);
                        arg++;
                    }
                }
            }
            break;

          case AMTYPE_APPICON:
            dock->runState = RS_UNICONIFYING;
            break;
        }

        ReplyMsg((struct Message*)msg);

        dock->hoverOverDock = FALSE;
        dock->rearrangeFromWB = FALSE;

        if( launching ) {
            DEBUG(DB_Printf("handle_drop_event: launch after reply\n"));
            launch_copied_drop(launchGad, dlocks, dnames, ncopy);
        }
        if( inserted ) {
            DEBUG(DB_Printf("handle_drop_event: insert dest=%ld n=%ld\n",
                (LONG)dest, (LONG)ncopy));
            for( i = 0; i < ncopy; i++ ) {
                if( dlocks[i] ) {
                    before = rearrange_node_at(dock, dest);
                    if( insert_dropped_icon(dock, dlocks[i], dnames[i], before) ) {
                        dest++;
                    }
                }
            }
            rearrange_commit_drop(dock);
        }
        for( i = 0; i < ncopy; i++ ) {
            if( dlocks[i] ) {
                UnLock(dlocks[i]);
                dlocks[i] = NULL;
            }
        }
    }
}

VOID handle_edit_click(struct DockWindow *dock, struct DgNode *dg, UWORD mouseX, UWORD mouseY)
{
    struct GadgetEnvironment env;
    UWORD x, y, cx, cy, op;

    TD_GetDockGadgetEnvironment(dg->dg, &env);
    
    if( env.index == 0 ) {
        return;
    }

    x = mouseX - env.gadgetBounds.x;
    y = mouseY - env.gadgetBounds.y;
    
    cx = env.gadgetBounds.w / 2;
    cy = env.gadgetBounds.h / 2;

    op = EO_NONE;

    if( y < cy ) {
        if( x < cx ) {
            if( env.index > 1 ) {
                op = EO_MOVE_UP;
            }    
        } else if( DOCK_HORIZONTAL(dock) ) {
            if( ! env.isLast ) {
                op = EO_MOVE_DOWN;  
            }   
        } else {
            op = EO_DELETE;
        }             
    } else if( x < cx ) {
     
       if( DOCK_HORIZONTAL(dock) ) {
            op = EO_DELETE;
   
        } else {
            if( ! env.isLast ) {
                op = EO_MOVE_DOWN;
            }
        }
    }

    if( op != EO_NONE ) {

        dock->runState = RS_CHANGING;
        dock->editOp = op;
        dock->editNode = dg;
        dock->editCount = 2;
    }    
}


STATIC VOID handle_menu_choice(struct DockWindow *dock, MenuIndex menuItem)
{
    switch( menuItem ){
       case MI_QUIT:
            rearrange_cancel(dock);
            dock->runState = RS_QUITTING;
            break;

       case MI_ABOUT:
            show_about(dock);
            break;

       case MI_HIDE:
            rearrange_cancel(dock);
            dock->runState = RS_ICONIFYING;
            break;

       case MI_OPEN_PREFS:
            rearrange_cancel(dock);
            open_settings(dock);
            break;

       case MI_HELP:
            open_help(dock);
            break;

       default:
            break;
    }
}

STATIC VOID open_dock_rmb_menu(struct DockWindow *dock)
{
    struct PopupMenu *menu;
    ULONG result;

    if( PopupMenuBase == NULL || dock->win == NULL ) {
        return;
    }

    menu = PMMenu((STRPTR)MSG_MENU_Project),
        PMItem((STRPTR)MSG_MENU_About),
            PM_UserData, MI_ABOUT, PMEnd,
        PMItem((STRPTR)MSG_MENU_Iconify),
            PM_UserData, MI_HIDE, PMEnd,
        PMItem((STRPTR)MSG_MENU_Quit),
            PM_UserData, MI_QUIT, PMEnd,
        PMBar, PMEnd,
        PMItem((STRPTR)MSG_MENU_OpenPrefs),
            PM_UserData, MI_OPEN_PREFS, PMEnd,
        PMEnd;

    if( menu ) {
        result = PM_OpenPopupMenu(dock->win, PM_Menu, menu, TAG_DONE);
        PM_FreePopupMenu(menu);
        if( result ) {
            handle_menu_choice(dock, (MenuIndex)result);
        }
    }
}

STATIC VOID handle_rmb(struct DockWindow *dock, UWORD mouseX, UWORD mouseY);

VOID handle_cx_rmb(struct DockWindow *dock)
{
    struct Window *win;
    struct Screen *scr;
    WORD mx, my;
    UWORD x, y;

    win = dock->win;
    if( win == NULL || dock->hoverOverDock ) {
        return;
    }
    scr = win->WScreen;
    if( scr == NULL ) {
        return;
    }

    mx = scr->MouseX;
    my = scr->MouseY;
    if( mx < win->LeftEdge || my < win->TopEdge ) {
        return;
    }
    if( mx >= (WORD)(win->LeftEdge + win->Width) ) {
        return;
    }
    if( my >= (WORD)(win->TopEdge + win->Height) ) {
        return;
    }

    x = (UWORD)(mx - win->LeftEdge);
    y = (UWORD)(my - win->TopEdge);
    DEBUG(DB_Printf("cx_rmb %ld,%ld\n", (LONG)x, (LONG)y));
    handle_rmb(dock, x, y);
}

STATIC VOID handle_rmb(struct DockWindow *dock, UWORD mouseX, UWORD mouseY)
{
    struct DgNode *dg;
    struct GadgetEnvironment env;
    ULONG choice;

    if( ! applife_have_popup() ) {
        return;
    }

    if( dock->runState != RS_RUNNING ) {
        open_dock_rmb_menu(dock);
        return;
    }

    dg = get_gadget_at(dock, mouseX, mouseY);
    if( dg ) {
        TD_GetDockGadgetEnvironment(dg->dg, &env);
        if( env.index != 0 ) {
            if( dock_gadget_popup(dg->dg, dock->win) ) {
                return;
            }
            if( dg != dock->placeholder ) {
                choice = applife_open_choice_popup(dock->win, dg->n.ln_Name,
                    (STRPTR)MSG_POP_Remove, FALSE);
                if( choice ) {
                    request_remove_gadget(dock, dg);
                }
                return;
            }
        }
    }

    open_dock_rmb_menu(dock);
}

VOID handle_window_event(struct DockWindow *dock)
{
    struct IntuiMessage *msg;
    struct DgNode *dg;
    MenuIndex menuItem;
    UWORD mouseX, mouseY, msgClass, msgCode, menuNum;
    struct MenuItem *item;

    while( msg = (struct IntuiMessage *)GetMsg(dock->win->UserPort) ) {
    
        msgClass = msg->Class;
        msgCode = msg->Code;
        mouseX = msg->MouseX;
        mouseY = msg->MouseY;

        ReplyMsg((struct Message *)msg);

        /* Workbench icon drag: ignore mouse and layout. Must still
           answer IDCMP_REFRESHWINDOW or Intuition never unlocks. */
        if( dock->hoverOverDock ) {
            if( msgClass == IDCMP_REFRESHWINDOW ) {
                BeginRefresh(dock->win);
                EndRefresh(dock->win, TRUE);
            }
            continue;
        }
        
        switch( msgClass )
        {
            case IDCMP_MOUSEBUTTONS: 
                if( msgCode == SELECTDOWN ) {
                    if( dock->runState == RS_RUNNING ) {
                        rearrange_mouse_down(dock, mouseX, mouseY);
                    } else if( dg = get_gadget_at(dock, mouseX, mouseY) ) {
                        handle_edit_click(dock, dg, mouseX, mouseY);
                    }
                } else if( msgCode == SELECTUP ) {
                    rearrange_mouse_up(dock, mouseX, mouseY);
                } else if( msgCode == MENUDOWN ) {
                    if( ! dock->rmbCxWatch ) {
                        handle_rmb(dock, mouseX, mouseY);
                    }
                }
                break;

            case IDCMP_MOUSEMOVE:
                rearrange_mouse_move(dock, mouseX, mouseY);
                break;

            case IDCMP_INTUITICKS:
                rearrange_intuitick(dock);
                break;
    
            case IDCMP_CHANGEWINDOW:
            case IDCMP_REFRESHWINDOW:
                if( ! dock->hoverOverDock ) {
                    update_entire_window(dock);
                }
                break;
        
            case IDCMP_MENUPICK:
                menuNum = msgCode;
                while( menuNum != MENUNULL ) {
                    item = ItemAddress(dock->menu, menuNum);
                    menuItem = (MenuIndex)GTMENUITEM_USERDATA(item);
                    handle_menu_choice(dock, menuItem);
                    menuNum = item->NextSelect;
                }
                break;

        }
        
    }
}
