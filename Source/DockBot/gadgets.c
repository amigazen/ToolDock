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
#include "dropzone.h"

#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <clib/layers_protos.h>
#include <clib/graphics_protos.h>

#include <graphics/gfxmacros.h>

#include "dock_handle.h"
#include "rearrange.h"

BOOL create_dock_handle(struct DockWindow *dock)
{
    struct DgNode *dg;

    DEBUG(DB_Printf(__FUNC__ "\n"));

    if( dg = TD_AllocGadget(HANDLE_CLASS_NAME) ) {

        AddTail(&dock->cfg.gadgets, (struct Node *)dg);

        DEBUG(DB_Printf("  Created handle.\n"));

        return TRUE;
    }

    DEBUG(DB_Printf(__FUNC__ ": Failed to create handle.\n"));

    return FALSE;
}

BOOL init_gadget_classes(struct DockWindow *dock)
{
    DEBUG(DB_Printf(__FUNC__ "\n"));

    if( dock->gadgetPort = CreateMsgPort() ) {
        if( dock->handleClass = init_dock_handle_class() ) {
            return TRUE;
        }   
    }

    DEBUG(DB_Printf(__FUNC__ ": Failed\n"));
    return FALSE;
}

BOOL free_gadget_classes(struct DockWindow *dock)
{
    DEBUG(DB_Printf(__FUNC__ "\n"));

    if( dock->handleClass ) {
        
        if( ! free_dock_handle_class(dock->handleClass) ) {
            return FALSE;
        }
        dock->handleClass = NULL;
    }

    delete_port(dock->gadgetPort);
    dock->gadgetPort = NULL;        

    return TRUE;
}

VOID remove_dock_gadgets(struct DockWindow *dock)
{
    struct DgNode *dg;

    DEBUG(DB_Printf(__FUNC__ "\n"));

    disable_layout(dock);

    while( !IsListEmpty(&dock->cfg.gadgets) ) {

        if( dg = (struct DgNode *)RemTail(&dock->cfg.gadgets) ) {

            TD_FreeGadget(dg);
        }
    }
    dock->placeholder = NULL;
}

VOID fill_background(struct DockWindow *dock, struct RastPort *rp, struct Rect *b)
{
    UWORD x, y, sx, sy, stepX, stepY, ex, ey;

    if( ! dock->bgBrush ) {

        SetAPen(rp, 0);
        RectFill(rp, b->x, b->y, b->x + b->w - 1, b->y + b->h - 1);

        return;
    }

    TD_GetBrushSize(dock->bgBrush, &stepX, &stepY);

    sx = b->x;
    sy = b->y;
    
    if( sx > 0 ) {
        sx = sx - (sx % stepX);
    }
    
    if( sy > 0 ) {
        sy = sy - (sy % stepY);
    }    

    ex = b->x + b->w - 1;
    ey = b->y + b->h - 1;
    
    x = sx;
    y = sy;

    while( y < ey ) {

        TD_DrawBrush(dock->bgBrush, rp, 0, 0, x, y, stepX, stepY);

        x += stepX;
        if( x >= ex ) {
            x = sx;
            y += stepY;    
        }
    }    
}

VOID draw_edit_controls(struct DockWindow *dock, struct Screen *screen, struct RastPort *rp, Object *gadget)
{
    struct DrawInfo *drawInfo;
    struct GadgetEnvironment env;
    ULONG pen;
    USHORT pat[] = { 0x5555, 0xAAAA };
    UWORD i1, i2, i3;
   
    TD_GetDockGadgetEnvironment(gadget, &env);

    if( OCLASS(gadget) == dock->handleClass ) {
        return;
    }

    if( drawInfo = GetScreenDrawInfo(screen) ) {

        pen = ObtainBestPenA(screen->ViewPort.ColorMap, 255, 255, 255, NULL);

        SetDrMd(rp, JAM1);
        SetAPen(rp, drawInfo->dri_Pens[BACKGROUNDPEN]);
        SetBPen(rp, pen);
        SetAfPt(rp, pat, 1);    
    
        RectFill(rp, env.gadgetBounds.x, env.gadgetBounds.y,
                     env.gadgetBounds.x + env.gadgetBounds.w - 1, 
                     env.gadgetBounds.y + env.gadgetBounds.h - 1);

        SetAfPt(rp, NULL, 0);
        ReleasePen(screen->ViewPort.ColorMap, pen);

        FreeScreenDrawInfo(screen, drawInfo);
    }

    if( DOCK_HORIZONTAL(dock) ) {

        i1 = II_ARROW_LEFT;
        i2 = II_ARROW_RIGHT;
        i3 = II_DELETE;

        if( dock->runState == RS_CHANGING && dock->editCount & 1 ) {
            switch( dock->editOp ) {
                case EO_MOVE_UP:
                    i1 = II_ARROW_LEFT_INV;
                    break;

                case EO_MOVE_DOWN:
                    i2 = II_ARROW_RIGHT_INV;
                    break;

                case EO_DELETE:
                    i3 = II_DELETE_INV;
                    break;
            }
        }

        if( env.isLast ) {
            i2 = II_NONE;
        }

    } else {

        i1 = II_ARROW_UP;
        i2 = II_DELETE;
        i3 = II_ARROW_DOWN;

        if( dock->runState == RS_CHANGING && dock->editCount & 1 ) {
            switch( dock->editOp ) {
                case EO_MOVE_UP:
                    i1 = II_ARROW_UP_INV;
                    break;

                case EO_MOVE_DOWN:
                    i3 = II_ARROW_DOWN_INV;
                    break;

                case EO_DELETE:
                    i2 = II_DELETE_INV;
                    break;

            }
        }

        if( env.isLast ) {
            i3 = II_NONE;
        }
    }

    if( env.index <= 1 ) {
        i1 = II_NONE;
    }


    if( i1 != II_NONE ) {
        TD_DrawBrush(dock->iconBrush, rp, i1 * dock->iconW, 0,
            env.gadgetBounds.x, env.gadgetBounds.y, dock->iconW, dock->iconH);
    }
    
    if( i2 != II_NONE ) {
        TD_DrawBrush(dock->iconBrush, rp, i2 * dock->iconW, 0,
                env.gadgetBounds.x + env.gadgetBounds.w - 1 - dock->iconW,
                env.gadgetBounds.y, dock->iconW, dock->iconH);
    }

    if( i3 != II_NONE ) {
        TD_DrawBrush(dock->iconBrush, rp, i3 * dock->iconW, 0,
                env.gadgetBounds.x,
                env.gadgetBounds.y + env.gadgetBounds.h - 1 - dock->iconH,
                dock->iconW, dock->iconH);
    }
}

VOID update_window(struct DockWindow *dock, UWORD x, UWORD y, UWORD w, UWORD h)
{
    UWORD ww;
    UWORD wh;

    if( dock->hoverOverDock ) {
        return;
    }
    if( dock->renderBM == NULL || dock->win == NULL || dock->win->RPort == NULL ) {
        return;
    }
    ww = dock->win->Width;
    wh = dock->win->Height;
    /* UWORD wrap of a negative ghost (e.g. 65528) is a Recoverable Alert. */
    if( x >= ww || y >= wh || w < 1 || h < 1 ) {
        return;
    }
    if( x + w > ww ) {
        w = (UWORD)(ww - x);
    }
    if( y + h > wh ) {
        h = (UWORD)(wh - y);
    }
    if( w < 1 || h < 1 ) {
        return;
    }
    DEBUG(DB_Printf(__FUNC__ ": %ld,%ld %ld,%ld\n", (LONG)x, (LONG)y, (LONG)w, (LONG)h));
    BltBitMapRastPort(dock->renderBM, x, y, dock->win->RPort, x, y, w, h, 0xC0);
}

VOID update_entire_window(struct DockWindow *dock)
{
    DEBUG(DB_Printf(__FUNC__ "\n"));

    BeginRefresh(dock->win);

    update_window(dock, 0, 0, dock->renderW, dock->renderH);

    EndRefresh(dock->win, TRUE);
}

struct Region *clip_to_gadget(struct DockWindow *dock, struct GadgetEnvironment *env)
{
    struct Region *oldR, *newR;
    struct Rectangle bounds;

    if( ! dock->renderL ) {
        return NULL;
    }
    if( env->gadgetBounds.w < 1 || env->gadgetBounds.h < 1 ) {
        return NULL;
    }
    if( env->gadgetBounds.x > 32000 || env->gadgetBounds.y > 32000 ) {
        return NULL;
    }

    if( newR = NewRegion() ) {
    
        bounds.MinX = env->gadgetBounds.x;
        bounds.MinY = env->gadgetBounds.y;
        bounds.MaxX = env->gadgetBounds.x + env->gadgetBounds.w - 1;
        bounds.MaxY = env->gadgetBounds.y + env->gadgetBounds.h - 1;

        DEBUG(DB_Printf(__FUNC__ ": Clip to %ld,%ld -> %ld,%ld\n", 
            bounds.MinX, bounds.MinY, bounds.MaxX, bounds.MaxY));

        OrRectRegion(newR, &bounds);

        oldR = InstallClipRegion(dock->renderL, newR);
    
    }

    return oldR;
}

VOID unclip_gadget(struct DockWindow *dock, struct Region *r)
{
    struct Region *oldR;

    if( ! dock->renderL ) {
        return;
    }

    if( r ) {

        oldR = InstallClipRegion(dock->renderL, r);

        if( oldR ) {
            DisposeRegion(oldR);
        }
    }
}

VOID draw_gadgets(struct DockWindow *dock)
{
    struct RastPort *rp;
    struct DgNode *curr;
    struct GadgetEnvironment env;
    struct Rect r;
    struct Screen *screen;
    struct Region *region;

    if( dock->hoverOverDock ) {
        return;
    }

    if( dock->renderBM ) {

        dz_lock();
        rp =  &dock->renderRP;
        r.x = 0;
        r.y = 0;
        r.w = dock->renderW;
        r.h = dock->renderH;

        fill_background(dock, rp, &r);
        rearrange_capture_bg(dock);

        FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {

            if( dock->runState == RS_REARRANGING && curr == dock->dragNode
                && ! dock->rearrangeFromWB ) {
                continue;
            }
    
            DEBUG(DB_Printf(__FUNC__ ": Drawing %s\n", curr->n.ln_Name));

            TD_GetDockGadgetEnvironment(curr->dg, &env);

            if( region = clip_to_gadget(dock, &env) ) {

                dock_gadget_draw(curr->dg, rp);

                if( dock->hoverGad == curr->dg && DOCK_EDITING(dock) ) {

                    if( screen = LockPubScreen(NULL) ) {
                    
                        draw_edit_controls(dock, screen, rp, dock->hoverGad);

                        UnlockPubScreen(NULL, screen);
                    }                
                }

                unclip_gadget(dock, region);
            }
        }
        
        if( ! dock->cfg.showGadgetBorders ) {
            TD_DrawOutsetFrame(rp, &r);
        }

        update_window(dock, 0, 0, dock->renderW, dock->renderH);
        dz_unlock();
    }
}


VOID draw_gadget(struct DockWindow *dock, Object *gadget)
{
    struct Screen *screen;
    struct RastPort *rp;
    struct GadgetEnvironment env;
    struct Rect r;
    struct Region *region;

    if( dock->renderBM ) {

        rp = &dock->renderRP;

        TD_GetDockGadgetEnvironment(gadget, &env);
    
        if( region = clip_to_gadget(dock, &env) ) {
            
            fill_background(dock, rp, &env.gadgetBounds);

            dock_gadget_draw(gadget, rp);

            if( ! dock->cfg.showGadgetBorders ) {

                r.x = 0;
                r.y = 0;
                r.w = dock->win->Width;
                r.h = dock->win->Height;

                TD_DrawOutsetFrame(rp, &r);
            }

            if( DOCK_EDITING(dock) && gadget == dock->hoverGad ) {

                if( screen = LockPubScreen(NULL) ) {

                    draw_edit_controls(dock, screen, rp, gadget);
                
                    UnlockPubScreen(NULL, screen);
                }
            }

            unclip_gadget(dock, region);
        }

        update_window(dock, env.gadgetBounds.x, env.gadgetBounds.y,
                            env.gadgetBounds.w, env.gadgetBounds.h );
    }
}

struct DgNode *get_gadget_at(struct DockWindow *dock, UWORD x, UWORD y)
{
    struct DgNode *curr;

    FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {
                        
        if( dock_gadget_hit_test(curr->dg, x, y) ) {

            return curr;
        }
    }
    return NULL;    
}

VOID update_hover_gadget(struct DockWindow *dock)
{
    WORD mx = dock->win->MouseX;
    WORD my = dock->win->MouseY;
    struct DgNode *node;
    Object *gadget;
    Object *oldHoverGad;

    if( dock->hoverOverDock ) {
        return;
    }

    oldHoverGad = dock->hoverGad;

    if( mx < 0 || my < 0
     || mx > dock->win->LeftEdge + dock->win->Width
     || my > dock->win->TopEdge + dock->win->Height ) {

        dock->hoverGad = NULL;

        if( oldHoverGad ) {
            draw_gadget(dock, oldHoverGad);
        }

    } else if( (node = get_gadget_at(dock, mx, my) ) ) {

        gadget = node->dg;

        if( gadget == dock->hoverGad ) {
            return;
        }

        dock->hoverGad = gadget;

        if( dock->runState == RS_EDITING ) {

            if( oldHoverGad ) {
                draw_gadget(dock, oldHoverGad);
            }

            if( dock->hoverGad ) {
                draw_gadget(dock, dock->hoverGad);
            }
        }
    }
}


VOID launch(struct DockWindow *dock, struct GadgetMessageLaunch *msg)
{
    execute_external(
        dock,
        msg->path,
        msg->args, 
        msg->console,
        msg->wb
    );

    dock_gadget_launched(msg->m.sender,
                         msg->path,
                         msg->args,
                         msg->console,
                         msg->wb);
}

struct DgNode *get_gadget_node(struct DockWindow *dock, Object *obj)
{
    struct DgNode *curr;

    FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {
        if( obj == curr->dg ) {
            return curr;
        }
    }

    return NULL;
}

VOID register_gadget_port(struct DgNode *g, struct GadgetMessagePort *portMsg)
{
    struct PortReg *pr;

    DEBUG(DB_Printf(__FUNC__ ": %s %lx\n", g->n.ln_Name, portMsg->port));
    
    if( pr = TD_AllocMem(sizeof(struct PortReg), MEMF_CLEAR) ) {

        pr->port = portMsg->port;

        AddTail((struct List *)&g->ports, (struct Node *)pr);
    }
}

VOID unregister_gadget_port(struct DgNode *g, struct GadgetMessagePort *portMsg)
{
    struct PortReg *pr;

    DEBUG(DB_Printf(__FUNC__ ": %s %lx\n", g->n.ln_Name, portMsg->port));

    FOR_EACH_PORTREG(&g->ports, pr) {
        if( pr->port == portMsg->port ) {
            Remove((struct Node *)pr);

            TD_FreeMem(pr, sizeof(struct PortReg));

            break;
        }
    }         
}

VOID handle_gadget_message(struct DockWindow *dock)
{
    struct DgNode *curr;
    struct GadgetMessage *msg;
    
    while( msg = (struct GadgetMessage *)GetMsg(dock->gadgetPort) ) {
        if( dock->hoverOverDock ) {
            TD_FreeMem(msg, msg->m.mn_Length);
            continue;
        }
        if( dock->runState == RS_RUNNING ) {

            curr = get_gadget_node(dock, msg->sender);
            if( curr == NULL ) {
                TD_FreeMem(msg, msg->m.mn_Length);
                continue;
            }

            switch( msg->messageType ) {
                case GM_DRAW:
                    draw_gadget(dock, msg->sender);
                    break;

                case GM_QUIT:
                    dock->runState = RS_QUITTING;
                    break;    

                case GM_LAUNCH:
                    launch(dock, (struct GadgetMessageLaunch *)msg);
                    break;

                case GM_REGISTER_PORT:
                    register_gadget_port(curr, (struct GadgetMessagePort *)msg);
                    break;

                case GM_UNREGISTER_PORT:
                    unregister_gadget_port(curr, (struct GadgetMessagePort *)msg);
                    break;                
            }
        }

        TD_FreeMem(msg, msg->m.mn_Length);
    }
}

VOID remap_gadgets(struct DockWindow *dock)
{
    struct DgNode *curr;

    FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {
        dock_gadget_remap(curr->dg);
    }
}

ULONG get_custom_sigs(struct DockWindow *dock)
{
    struct DgNode *curr;
    struct PortReg *pr;
    ULONG sig = 0;

//    DEBUG(DB_Printf("get_custom_sigs\n"));

    FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {
        FOR_EACH_PORTREG(&curr->ports, pr) {

            sig |= (1 << pr->port->mp_SigBit);
        }
    }

    return sig;
}

VOID handle_custom_message(struct DockWindow *dock, ULONG signal)
{
    struct DgNode *curr;
    struct PortReg *pr;

    DEBUG(DB_Printf("handle_custom_message\n"));

    FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {
        FOR_EACH_PORTREG(&curr->ports, pr) {

            DEBUG(DB_Printf("  dg = %s, p = %lx\n", curr->n.ln_Name, pr->port));

            if( signal & (1 << pr->port->mp_SigBit) ) {
    
                dock_gadget_message(curr->dg, pr->port);
            }
        }
    }
}

