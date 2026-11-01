/************************************
**
**  ToolDock - A Dock For AmigaOS 3
**
**  SPDX-License-Identifier: MIT
**
**  ToolDock changes Copyright 2026 amigazen project
**  DockBot Copyright 2019 Andrew Kennan
**
**  SimpleDock: during a Workbench drag the window layer is locked.
**  After a beat the dock task composes a gap into hookBM (clean
**  backdrop plus full-size tiles) and BltBitMaps that onto the screen.
**  No ChangeWindowBox. Drop fills the hole; leaving restores.
**
************************************/

#include "rearrange.h"
#include "dropzone.h"

#include <clib/compiler-specific.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <utility/hooks.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <utility/tagitem.h>
#include <exec/io.h>
#include <devices/timer.h>
#include <devices/input.h>
#include <graphics/gfx.h>

#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <clib/dos_protos.h>
#include <clib/wb_protos.h>
#include <clib/graphics_protos.h>
#include <clib/timer_protos.h>
#include <clib/input_protos.h>
#include <pragmas/wb_pragmas.h>
#include <pragmas/graphics_pragmas.h>
#include <pragmas/timer_pragmas.h>
#include <pragmas/input_pragmas.h>

struct Library *MotionBase = NULL;
struct Device *TimerBase = NULL;
struct Device *InputBase = NULL;
STATIC struct MsgPort *inputPort = NULL;
STATIC struct IOStdReq *inputReq = NULL;

#include <proto/motion.h>

extern struct Library *WorkbenchBase;
extern struct Library *GfxBase;
extern struct IntuitionBase *IntuitionBase;

STATIC VOID hook_glide(struct DockWindow *dock, struct RastPort *wrp, WORD gap);
STATIC WORD gap_from_mouse(struct DockWindow *dock);
STATIC BOOL dwell_elapsed(struct DockWindow *dock);
STATIC VOID step_rearrange(struct DockWindow *dock);
STATIC UWORD ease_p(UWORD p);
STATIC VOID lerp_boxes(struct IBox *out, struct IBox *from, struct IBox *to,
    UWORD n, UWORD p);

STATIC WORD zone_gap(ULONG id)
{
    LONG v;

    v = (LONG)id - REARRANGE_ZONE_BASE;
    if( v >= 0 && v < (REARRANGE_MAX_GADGETS * 2) && (v & 1) == 0 ) {
        v = v / 2;
        if( v < 1 ) {
            return 1;
        }
        return (WORD)v;
    }
    return (WORD)-1;
}

STATIC WORD zone_onto(ULONG id)
{
    LONG v;

    v = (LONG)id - REARRANGE_ZONE_BASE;
    if( v >= 0 && v < (REARRANGE_MAX_GADGETS * 2) && (v & 1) ) {
        return (WORD)(v / 2);
    }
    return (WORD)-1;
}

STATIC BOOL pointer_in_dock(struct DockWindow *dock)
{
    struct Window *w;
    struct Screen *s;
    WORD x;
    WORD y;

    w = dock->win;
    if( w == NULL || w->WScreen == NULL ) {
        return FALSE;
    }
    s = w->WScreen;
    x = (WORD)(s->MouseX - w->LeftEdge);
    y = (WORD)(s->MouseY - w->TopEdge);
    if( x < 0 || y < 0 || x >= (WORD)w->Width || y >= (WORD)w->Height ) {
        return FALSE;
    }
    return TRUE;
}

STATIC WORD gap_from_mouse(struct DockWindow *dock)
{
    struct Window *w;
    struct Screen *s;
    WORD along;
    WORD mid;
    UWORD i;
    UWORD n;

    n = dock->snapCount;
    w = dock->win;
    if( n < 2 || w == NULL || w->WScreen == NULL ) {
        return 1;
    }
    s = w->WScreen;
    if( DOCK_HORIZONTAL(dock) ) {
        along = (WORD)(s->MouseX - w->LeftEdge);
        for( i = 1; i < n; i++ ) {
            mid = (WORD)(dock->restBox[i].Left + dock->restBox[i].Width / 2);
            if( along < mid ) {
                return (WORD)i;
            }
        }
    } else {
        along = (WORD)(s->MouseY - w->TopEdge);
        for( i = 1; i < n; i++ ) {
            mid = (WORD)(dock->restBox[i].Top + dock->restBox[i].Height / 2);
            if( along < mid ) {
                return (WORD)i;
            }
        }
    }
    return (WORD)n;
}

/* Workbench calls this with the display locked. graphics.library only. */
STATIC __SAVE_DS__ __ASM__ ULONG drop_zone_hook(
    __REG__(a0, struct Hook *hook),
    __REG__(a2, APTR object),
    __REG__(a1, struct AppWindowDropZoneMsg *msg))
{
    struct DockWindow *dock;
    WORD gap;
    WORD onto;

    (void)object;
    dock = (struct DockWindow *)hook->h_Data;
    if( dock == NULL || msg == NULL ) {
        return 0;
    }
    if( ! AttemptSemaphore(&dock->dropLock) ) {
        return 0;
    }
    if( ! dock->dropValid ) {
        ReleaseSemaphore(&dock->dropLock);
        return 0;
    }

    /* Draw into adzm_RastPort only. No OpenWindow or ChangeWindowBox. */
    if( msg->adzm_Action == ADZMACTION_Enter ) {
        dock->hookEnterCount++;
        dock->hoverOverDock = TRUE;
        onto = zone_onto(msg->adzm_ID);
        gap = zone_gap(msg->adzm_ID);
        /* A new icon always gets a free place. Onto a cell still opens
           that place; SimpleDock only presses when the drop opens files. */
        if( onto >= 1 ) {
            gap = onto;
        } else if( gap < 1 ) {
            gap = gap_from_mouse(dock);
        }
        if( gap < 1 ) {
            gap = 1;
        }
        dock->dropOnto = (WORD)-1;
        dock->gapIndex = gap;
        dock->rearrangeFromWB = TRUE;
        /* SimpleDock: graphics.library into adzm_RastPort only. */
        hook_glide(dock, msg->adzm_RastPort, gap);
    } else if( msg->adzm_Action == ADZMACTION_Leave ) {
        dock->hookLeaveCount++;
        if( ! pointer_in_dock(dock) ) {
            hook_glide(dock, msg->adzm_RastPort, (WORD)-1);
            dock->hoverOverDock = FALSE;
            dock->rearrangeFromWB = FALSE;
            dock->gapIndex = (WORD)-1;
            dock->dropOnto = (WORD)-1;
            dock->dwellCount = 0;
        }
    }

    ReleaseSemaphore(&dock->dropLock);
    return 0;
}

STATIC UWORD gadget_index(struct DockWindow *dock, struct DgNode *node)
{
    struct DgNode *curr;
    UWORD i;

    i = 0;
    FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {
        if( curr == node ) {
            return i;
        }
        i++;
    }
    return 0;
}

UWORD rearrange_count(struct DockWindow *dock)
{
    struct DgNode *curr;
    UWORD n;

    n = 0;
    FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {
        n++;
    }
    return n;
}

struct DgNode *rearrange_node_at(struct DockWindow *dock, UWORD index)
{
    struct DgNode *curr;
    UWORD i;

    i = 0;
    FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {
        if( i == index ) {
            return curr;
        }
        i++;
    }
    return NULL;
}

STATIC UWORD typical_cell(struct DockWindow *dock)
{
    struct DgNode *curr;
    UWORD w;
    UWORD h;
    UWORD along;
    UWORD best;
    UWORD i;

    best = 0;
    i = 0;
    FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {
        if( i > 0 && curr != dock->dragNode ) {
            dock_gadget_get_size(curr->dg, dock->cfg.pos, dock->cfg.align, &w, &h);
            if( w < 1 || w > 128 ) {
                w = DEFAULT_SIZE;
            }
            if( h < 1 || h > 128 ) {
                h = DEFAULT_SIZE;
            }
            if( DOCK_HORIZONTAL(dock) ) {
                along = (UWORD)(w + dock->cfg.gadgetPadding * 2);
            } else {
                along = (UWORD)(h + dock->cfg.gadgetPadding * 2);
            }
            if( dock_gadget_wants_label(dock, curr->dg) ) {
                along = (UWORD)(along + dock_label_band(dock));
            }
            if( along > best ) {
                best = along;
            }
        }
        i++;
    }
    if( best < 16 ) {
        return DEFAULT_SIZE;
    }
    return best;
}

UWORD rearrange_gap_index(struct DockWindow *dock, UWORD x, UWORD y)
{
    struct DgNode *curr;
    struct GadgetEnvironment env;
    UWORD i;
    UWORD along;
    UWORD mid;

    along = DOCK_HORIZONTAL(dock) ? x : y;
    i = 0;
    FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {
        if( i == 0 || curr == dock->dragNode ) {
            i++;
            continue;
        }
        TD_GetDockGadgetEnvironment(curr->dg, &env);
        if( DOCK_HORIZONTAL(dock) ) {
            mid = env.gadgetBounds.x + env.gadgetBounds.w / 2;
        } else {
            mid = env.gadgetBounds.y + env.gadgetBounds.h / 2;
        }
        if( along < mid ) {
            return i;
        }
        i++;
    }
    if( i < 1 ) {
        return 1;
    }
    return i;
}

STATIC VOID pointer_in_window(struct DockWindow *dock, UWORD *x, UWORD *y)
{
    struct Window *w;
    struct Screen *s;

    w = dock->win;
    if( w == NULL || w->WScreen == NULL ) {
        *x = 0;
        *y = 0;
        return;
    }
    s = w->WScreen;
    *x = (UWORD)(s->MouseX - w->LeftEdge);
    *y = (UWORD)(s->MouseY - w->TopEdge);
}

STATIC VOID snapshot_boxes(struct DockWindow *dock, struct IBox *boxes, UWORD count)
{
    struct DgNode *curr;
    struct GadgetEnvironment env;
    UWORD i;

    if( boxes == NULL || dock->win == NULL ) {
        return;
    }
    boxes[0].Left = dock->win->LeftEdge;
    boxes[0].Top = dock->win->TopEdge;
    boxes[0].Width = dock->win->Width;
    boxes[0].Height = dock->win->Height;
    i = 1;
    FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {
        if( i >= count ) {
            break;
        }
        TD_GetDockGadgetEnvironment(curr->dg, &env);
        boxes[i].Left = env.gadgetBounds.x;
        boxes[i].Top = env.gadgetBounds.y;
        boxes[i].Width = env.gadgetBounds.w;
        boxes[i].Height = env.gadgetBounds.h;
        i++;
    }
}

STATIC VOID apply_boxes(struct DockWindow *dock, struct IBox *boxes, UWORD count)
{
    struct DgNode *curr;
    struct GadgetEnvironment env;
    UWORD i;

    if( boxes == NULL || dock->win == NULL || count < 1 ) {
        return;
    }
    ChangeWindowBox(dock->win, boxes[0].Left, boxes[0].Top,
        boxes[0].Width, boxes[0].Height);
    update_render_bitmap(dock, boxes[0].Width, boxes[0].Height);

    i = 1;
    FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {
        if( i >= count ) {
            break;
        }
        if( curr == dock->dragNode && dock->runState == RS_REARRANGING ) {
            i++;
            continue;
        }
        TD_GetDockGadgetEnvironment(curr->dg, &env);
        env.gadgetBounds.x = boxes[i].Left;
        env.gadgetBounds.y = boxes[i].Top;
        env.gadgetBounds.w = boxes[i].Width;
        env.gadgetBounds.h = boxes[i].Height;
        env.windowBounds.x = boxes[0].Left;
        env.windowBounds.y = boxes[0].Top;
        env.windowBounds.w = boxes[0].Width;
        env.windowBounds.h = boxes[0].Height;
        dock_gadget_set_environment(curr->dg, &env);
        i++;
    }
}

STATIC VOID free_motion(struct DockWindow *dock)
{
    if( dock->motionDisplay ) {
        RemoveMotion(dock->motionDisplay);
        dock->motionDisplay = NULL;
    }
    if( dock->motionHandle ) {
        FreeMotion(dock->motionHandle);
        dock->motionHandle = NULL;
    }
    if( dock->motionFrom ) {
        TD_FreeMem(dock->motionFrom, dock->motionCount * sizeof(struct IBox));
        dock->motionFrom = NULL;
    }
    if( dock->motionModel ) {
        TD_FreeMem(dock->motionModel, dock->motionCount * sizeof(struct IBox));
        dock->motionModel = NULL;
    }
    dock->motionCount = 0;
}

STATIC VOID apply_presentation(struct DockWindow *dock)
{
    struct IBox box;
    struct IBox *boxes;
    UWORD i;
    ULONG ok;

    if( dock->motionHandle == NULL || dock->motionCount == 0 ) {
        return;
    }
    boxes = (struct IBox *)TD_AllocMem(dock->motionCount * sizeof(struct IBox), MEMF_CLEAR);
    if( boxes == NULL ) {
        return;
    }
    for( i = 0; i < dock->motionCount; i++ ) {
        box.Left = 0;
        box.Top = 0;
        box.Width = 0;
        box.Height = 0;
        ok = GetMotionAttr(dock->motionHandle, i, MOTION_PresentationBox, (ULONG *)&box);
        if( ok ) {
            boxes[i] = box;
        } else if( dock->motionModel ) {
            boxes[i] = dock->motionModel[i];
        }
    }
    apply_boxes(dock, boxes, dock->motionCount);
    TD_FreeMem(boxes, dock->motionCount * sizeof(struct IBox));
}

STATIC VOID start_motion(struct DockWindow *dock)
{
    struct TagItem tags[8];
    struct TagItem inst[3];
    ULONG err;
    UWORD i;
    UWORD n;

    if( dock->win == NULL || dock->hoverOverDock ) {
        return;
    }
    /* AllocMotion/InstallMotion was in every crash log. Jump to layout. */
    layout_gadgets(dock);
    return;

    n = rearrange_count(dock) + 1;
    if( n < 2 ) {
        layout_gadgets(dock);
        return;
    }

    free_motion(dock);

    dock->motionFrom = (struct IBox *)TD_AllocMem(n * sizeof(struct IBox), MEMF_CLEAR);
    dock->motionModel = (struct IBox *)TD_AllocMem(n * sizeof(struct IBox), MEMF_CLEAR);
    if( dock->motionFrom == NULL || dock->motionModel == NULL ) {
        free_motion(dock);
        layout_gadgets(dock);
        return;
    }
    dock->motionCount = n;
    dock->motionTick = 0;
    snapshot_boxes(dock, dock->motionFrom, n);

    layout_gadgets(dock);
    snapshot_boxes(dock, dock->motionModel, n);
    apply_boxes(dock, dock->motionFrom, n);

    dock->motionBusy = TRUE;

    if( MotionBase == NULL ) {
        DEBUG(DB_Printf("start_motion: no motion.library, lerp\n"));
        return;
    }

    err = 0;
    dock->motionHandle = AllocMotion(dock->win, MOTION_BoxControl, n, &err);
    DEBUG(DB_Printf("start_motion: AllocMotion %lx err=%ld\n",
        (ULONG)dock->motionHandle, (LONG)err));
    if( dock->motionHandle == NULL ) {
        return;
    }

    for( i = 0; i < n; i++ ) {
        tags[0].ti_Tag = MOTION_From;
        tags[0].ti_Data = (ULONG)&dock->motionFrom[i];
        tags[1].ti_Tag = MOTION_Model;
        tags[1].ti_Data = (ULONG)&dock->motionModel[i];
        tags[2].ti_Tag = MOTION_Duration;
        tags[2].ti_Data = REARRANGE_MOTION_TICKS;
        tags[3].ti_Tag = MOTION_Ease;
        tags[3].ti_Data = MOTION_EaseInOut;
        tags[4].ti_Tag = MOTION_Apply;
        tags[4].ti_Data = MOTION_ApplyNone;
        tags[5].ti_Tag = MOTION_Flags;
        tags[5].ti_Data = MOTIONF_MODIFY | MOTIONF_FROM;
        tags[6].ti_Tag = TAG_DONE;
        tags[6].ti_Data = 0;
        SetMotionAttrsA(dock->motionHandle, i, tags);
    }

    inst[0].ti_Tag = MOTION_InstallEffect;
    inst[0].ti_Data = (ULONG)dock->motionHandle;
    inst[1].ti_Tag = TAG_DONE;
    inst[1].ti_Data = 0;
    dock->motionDisplay = InstallMotionA(dock->win, inst);
    if( dock->motionDisplay == NULL ) {
        FreeMotion(dock->motionHandle);
        dock->motionHandle = NULL;
        DEBUG(DB_Printf("start_motion: InstallMotion failed, lerp\n"));
    }
}

STATIC VOID step_rearrange(struct DockWindow *dock)
{
    struct IBox cur[REARRANGE_MAX_GADGETS + 1];
    ULONG state;
    UWORD p;
    UWORD n;

    if( ! dock->motionBusy || dock->motionFrom == NULL
        || dock->motionModel == NULL ) {
        return;
    }
    n = dock->motionCount;
    if( n < 1 || n > REARRANGE_MAX_GADGETS + 1 ) {
        return;
    }

    if( dock->motionDisplay ) {
        AnimateMotion(dock->motionDisplay);
        apply_presentation(dock);
        rearrange_draw(dock);
        state = 0;
        GetMotionAttr(dock->motionHandle, 0, MOTION_State, &state);
        if( state & MOTIONF_LAST ) {
            apply_boxes(dock, dock->motionModel, n);
            free_motion(dock);
            dock->motionBusy = FALSE;
            rearrange_draw(dock);
        }
        return;
    }

    dock->motionTick++;
    if( dock->motionTick >= REARRANGE_MOTION_TICKS ) {
        apply_boxes(dock, dock->motionModel, n);
        free_motion(dock);
        dock->motionBusy = FALSE;
        rearrange_draw(dock);
        return;
    }
    p = (UWORD)((ULONG)dock->motionTick * 1024 / REARRANGE_MOTION_TICKS);
    lerp_boxes(cur, dock->motionFrom, dock->motionModel, n, ease_p(p));
    apply_boxes(dock, cur, n);
    rearrange_draw(dock);
}

STATIC VOID retarget(struct DockWindow *dock)
{
    start_motion(dock);
    rearrange_draw(dock);
}

STATIC VOID set_intuiticks(struct DockWindow *dock, BOOL on)
{
    ULONG flags;

    if( dock->win == NULL ) {
        return;
    }
    flags = IDCMP_MOUSEBUTTONS | IDCMP_REFRESHWINDOW | IDCMP_CHANGEWINDOW
        | IDCMP_MENUPICK | IDCMP_MOUSEMOVE;
    if( on ) {
        flags |= IDCMP_INTUITICKS;
    }
    ModifyIDCMP(dock->win, flags);
}

STATIC VOID begin_rearrange(struct DockWindow *dock, BOOL fromWB, UWORD x, UWORD y)
{
    struct GadgetEnvironment env;

    if( dock->hoverOverDock || fromWB ) {
        return;
    }
    if( dock->runState == RS_REARRANGING ) {
        return;
    }

    dock->runState = RS_REARRANGING;
    dock->rearrangeFromWB = fromWB;
    dock->dwellCount = REARRANGE_DWELL_TICKS;
    dock->gapSize = typical_cell(dock);
    dock->dragW = DEFAULT_SIZE;
    dock->dragH = DEFAULT_SIZE;
    if( dock->dragNode ) {
        TD_GetDockGadgetEnvironment(dock->dragNode->dg, &env);
        if( env.gadgetBounds.w >= 8 ) {
            dock->dragW = env.gadgetBounds.w;
        }
        if( env.gadgetBounds.h >= 8 ) {
            dock->dragH = env.gadgetBounds.h;
        }
    }
    dock->gapIndex = rearrange_gap_index(dock, x, y);
    if( dock->gapIndex < 1 ) {
        dock->gapIndex = 1;
    }

    disable_notification(dock);
    set_intuiticks(dock, TRUE);
    retarget(dock);
}

STATIC VOID finish_rearrange(struct DockWindow *dock, BOOL restore)
{
    free_motion(dock);
    dock->gapIndex = (WORD)-1;
    dock->gapSize = 0;
    dock->dragNode = NULL;
    dock->dragArmed = FALSE;
    dock->rearrangeFromWB = FALSE;
    dock->dwellCount = 0;
    dock->dwellReady = FALSE;
    dock->wbVacant = FALSE;
    dock->hoverOverDock = FALSE;
    dock->motionBusy = FALSE;
    set_intuiticks(dock, FALSE);
    if( restore ) {
        layout_gadgets(dock);
        draw_gadgets(dock);
    }
    if( dock->runState == RS_REARRANGING ) {
        dock->runState = RS_RUNNING;
        enable_notification(dock);
    }
}

VOID rearrange_cancel(struct DockWindow *dock)
{
    if( dock->runState != RS_REARRANGING ) {
        dock->dragArmed = FALSE;
        dock->dragNode = NULL;
        dock->dwellCount = 0;
        return;
    }
    finish_rearrange(dock, TRUE);
}

STATIC VOID commit_move(struct DockWindow *dock)
{
    struct DgNode *pred;
    UWORD dest;
    UWORD old;

    if( dock->dragNode == NULL ) {
        finish_rearrange(dock, TRUE);
        return;
    }

    dest = dock->gapIndex;
    old = gadget_index(dock, dock->dragNode);
    Remove((struct Node *)dock->dragNode);
    if( old < dest && dest > 0 ) {
        dest--;
    }
    pred = rearrange_node_at(dock, dest);
    if( pred ) {
        Insert(&dock->cfg.gadgets, (struct Node *)dock->dragNode, pred->n.ln_Pred);
    } else {
        AddTail(&dock->cfg.gadgets, (struct Node *)dock->dragNode);
    }
    dock->dragNode = NULL;
    finish_rearrange(dock, FALSE);
    layout_gadgets(dock);
    draw_gadgets(dock);
    save_config(dock);
}

STATIC UWORD ease_p(UWORD p)
{
    ULONG u;
    ULONG v;

    u = p;
    if( u > 1024 ) {
        u = 1024;
    }
    v = (3 * u * u - (2 * u * u * u) / 1024) / 1024;
    return (UWORD)v;
}

STATIC VOID compute_dest(struct DockWindow *dock, WORD gap, struct IBox *dest)
{
    UWORD i;
    UWORD n;
    WORD extra;
    WORD pos;
    BOOL horiz;

    n = dock->snapCount;
    horiz = DOCK_HORIZONTAL(dock);
    extra = 0;
    if( gap >= 1 ) {
        extra = (WORD)dock->gapSize;
        if( extra < 8 ) {
            extra = DEFAULT_SIZE;
        }
    }
    pos = 0;
    for( i = 0; i < n; i++ ) {
        dest[i] = dock->restBox[i];
        if( extra > 0 && gap == (WORD)i && i >= 1 ) {
            pos = (WORD)(pos + extra);
        }
        if( horiz ) {
            dest[i].Left = pos;
            dest[i].Top = dock->restBox[i].Top;
            dest[i].Width = dock->restBox[i].Width;
            dest[i].Height = dock->restBox[i].Height;
            pos = (WORD)(pos + dest[i].Width);
        } else {
            dest[i].Top = pos;
            dest[i].Left = dock->restBox[i].Left;
            dest[i].Width = dock->restBox[i].Width;
            dest[i].Height = dock->restBox[i].Height;
            pos = (WORD)(pos + dest[i].Height);
        }
    }
}

STATIC VOID compose_hook(struct DockWindow *dock, struct RastPort *wrp,
    struct IBox *dest, UWORD n)
{
    struct RastPort rp;
    UWORD i;
    UWORD w;
    UWORD h;
    WORD sx;
    WORD sy;
    WORD dx;
    WORD dy;
    WORD bw;
    WORD bh;

    /* SimpleDock frame(): compose in hookBM, blit to adzm_RastPort at 0,0. */
    w = dock->snapW;
    h = dock->snapH;
    if( dock->hookBM == NULL || dock->snapBM == NULL || w < 1 || h < 1 ) {
        return;
    }

    if( dock->snapBG && dock->snapBGReady ) {
        BltBitMap(dock->snapBG, 0, 0, dock->hookBM, 0, 0, w, h, 0xC0, 0xFF, NULL);
    } else {
        InitRastPort(&rp);
        rp.BitMap = dock->hookBM;
        SetAPen(&rp, 0);
        RectFill(&rp, 0, 0, (WORD)(w - 1), (WORD)(h - 1));
    }

    for( i = 0; i < n; i++ ) {
        sx = dock->restBox[i].Left;
        sy = dock->restBox[i].Top;
        dx = dest[i].Left;
        dy = dest[i].Top;
        bw = dest[i].Width;
        bh = dest[i].Height;
        if( bw < 1 ) {
            bw = dock->restBox[i].Width;
        }
        if( bh < 1 ) {
            bh = dock->restBox[i].Height;
        }
        if( sx < 0 ) {
            bw = (WORD)(bw + sx);
            dx = (WORD)(dx - sx);
            sx = 0;
        }
        if( sy < 0 ) {
            bh = (WORD)(bh + sy);
            dy = (WORD)(dy - sy);
            sy = 0;
        }
        if( dx < 0 ) {
            bw = (WORD)(bw + dx);
            sx = (WORD)(sx - dx);
            dx = 0;
        }
        if( dy < 0 ) {
            bh = (WORD)(bh + dy);
            sy = (WORD)(sy - dy);
            dy = 0;
        }
        if( sx + bw > (WORD)w ) {
            bw = (WORD)((WORD)w - sx);
        }
        if( sy + bh > (WORD)h ) {
            bh = (WORD)((WORD)h - sy);
        }
        if( dx + bw > (WORD)w ) {
            bw = (WORD)((WORD)w - dx);
        }
        if( dy + bh > (WORD)h ) {
            bh = (WORD)((WORD)h - dy);
        }
        if( bw < 1 || bh < 1 ) {
            continue;
        }
        BltBitMap(dock->snapBM, sx, sy, dock->hookBM, dx, dy, bw, bh,
            0xC0, 0xFF, NULL);
    }

    WaitBlit();
    /* Window rastport ClipBlit/BltBitMapRastPort locks the layer and
       stalls until Workbench drops. Blit the screen bitmap instead. */
    if( dock->win && dock->win->WScreen
        && dock->win->WScreen->RastPort.BitMap ) {
        BltBitMap(dock->hookBM, 0, 0, dock->win->WScreen->RastPort.BitMap,
            (WORD)(dock->win->LeftEdge + dock->win->BorderLeft),
            (WORD)(dock->win->TopEdge + dock->win->BorderTop),
            w, h, 0xC0, 0xFF, NULL);
        WaitBlit();
        return;
    }
    if( wrp ) {
        BltBitMapRastPort(dock->hookBM, 0, 0, wrp, 0, 0, w, h, 0xC0);
    }
}

STATIC VOID lerp_boxes(struct IBox *out, struct IBox *from, struct IBox *to,
    UWORD n, UWORD p)
{
    UWORD i;
    LONG q;

    q = (LONG)p;
    for( i = 0; i < n; i++ ) {
        out[i].Left = (WORD)(from[i].Left
            + ((LONG)to[i].Left - from[i].Left) * q / 1024);
        out[i].Top = (WORD)(from[i].Top
            + ((LONG)to[i].Top - from[i].Top) * q / 1024);
        out[i].Width = from[i].Width;
        out[i].Height = from[i].Height;
    }
}

STATIC VOID hook_glide(struct DockWindow *dock, struct RastPort *wrp, WORD gap)
{
    struct timeval t0;
    struct timeval now;
    LONG t;
    UWORD n;
    UWORD p;

    /* SimpleDock: Workbench's task. graphics.library into hookBM, then
       BltBitMapRastPort to adzm_RastPort. GetSysTime loop is the glide.
       No Intuition. Dest boxes live on the dock, not this stack. */
    n = dock->snapCount;
    if( n == 0 || n > REARRANGE_MAX_GADGETS ) {
        return;
    }
    if( dock->hookBM == NULL || dock->snapBM == NULL ) {
        return;
    }
    if( dock->snapW < 2 || dock->snapH < 2 ) {
        return;
    }
    compute_dest(dock, gap, dock->hookDest);
    if( gap == dock->shownGap || TimerBase == NULL ) {
        compose_hook(dock, wrp, dock->hookDest, n);
        dock->shownGap = gap;
        return;
    }
    GetSysTime(&t0);
    for(;;) {
        GetSysTime(&now);
        SubTime(&now, &t0);
        t = now.tv_secs * 1000 + now.tv_micro / 1000;
        if( t >= REARRANGE_GLIDE_MS || t < 0 ) {
            break;
        }
        p = ease_p((UWORD)(t * 1024 / REARRANGE_GLIDE_MS));
        lerp_boxes(dock->drawBox, dock->restBox, dock->hookDest, n, p);
        compose_hook(dock, wrp, dock->drawBox, n);
    }
    compose_hook(dock, wrp, dock->hookDest, n);
    dock->shownGap = gap;
}

/* One blit, no GetSysTime spin. Safe to call from the dock timer. */
STATIC VOID hook_show(struct DockWindow *dock, struct RastPort *wrp, WORD gap)
{
    UWORD n;

    n = dock->snapCount;
    if( n == 0 || n > REARRANGE_MAX_GADGETS ) {
        return;
    }
    if( dock->hookBM == NULL || dock->snapBM == NULL ) {
        return;
    }
    if( dock->snapW < 2 || dock->snapH < 2 ) {
        return;
    }
    compute_dest(dock, gap, dock->hookDest);
    compose_hook(dock, wrp, dock->hookDest, n);
    dock->shownGap = gap;
}

STATIC VOID free_snap(struct DockWindow *dock)
{
    WaitBlit();
    if( dock->snapBM ) {
        FreeBitMap(dock->snapBM);
        dock->snapBM = NULL;
    }
    if( dock->snapBG ) {
        FreeBitMap(dock->snapBG);
        dock->snapBG = NULL;
    }
    if( dock->hookBM ) {
        FreeBitMap(dock->hookBM);
        dock->hookBM = NULL;
    }
    dock->snapW = 0;
    dock->snapH = 0;
    dock->snapBGReady = FALSE;
}

STATIC struct BitMap *alloc_friend(struct BitMap *friend, UWORD w, UWORD h)
{
    ULONG d;

    if( friend == NULL || w < 1 || h < 1 ) {
        return NULL;
    }
    /* SimpleDock: friend of the screen bitmap, never the window layer. */
    d = GetBitMapAttr(friend, BMA_DEPTH);
    return AllocBitMap(w, h, d, BMF_MINPLANES, friend);
}

STATIC VOID copy_rp(struct RastPort *src, struct BitMap *destBM, UWORD w, UWORD h)
{
    struct RastPort dst;

    if( src == NULL || destBM == NULL || w < 1 || h < 1 ) {
        return;
    }
    InitRastPort(&dst);
    dst.BitMap = destBM;
    ClipBlit(src, 0, 0, &dst, 0, 0, w, h, 0xC0);
}

STATIC BOOL ensure_snap(struct DockWindow *dock)
{
    struct BitMap *friend;
    UWORD w;
    UWORD h;

    w = dock->renderW;
    h = dock->renderH;
    if( w < 2 || h < 2 || dock->win == NULL || dock->win->WScreen == NULL ) {
        DEBUG(DB_Printf("ensure_snap: skip w=%ld h=%ld\n", (LONG)w, (LONG)h));
        return FALSE;
    }
    if( dock->snapBM && dock->hookBM && dock->snapW == w && dock->snapH == h ) {
        return TRUE;
    }
    friend = dock->win->WScreen->RastPort.BitMap;
    DEBUG(DB_Printf("ensure_snap: alloc %ld x %ld friend=%lx\n",
        (LONG)w, (LONG)h, (ULONG)friend));
    free_snap(dock);
    dock->snapBM = alloc_friend(friend, w, h);
    dock->snapBG = alloc_friend(friend, w, h);
    dock->hookBM = alloc_friend(friend, w, h);
    if( dock->snapBM == NULL || dock->hookBM == NULL ) {
        DEBUG(DB_Printf("ensure_snap: AllocBitMap failed\n"));
        free_snap(dock);
        return FALSE;
    }
    dock->snapW = w;
    dock->snapH = h;
    return TRUE;
}

STATIC VOID copy_rest(struct DockWindow *dock)
{
    struct DgNode *curr;
    struct GadgetEnvironment env;
    UWORD i;

    i = 0;
    FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {
        if( i >= REARRANGE_MAX_GADGETS ) {
            break;
        }
        TD_GetDockGadgetEnvironment(curr->dg, &env);
        dock->restBox[i].Left = (WORD)env.gadgetBounds.x;
        dock->restBox[i].Top = (WORD)env.gadgetBounds.y;
        dock->restBox[i].Width = (WORD)env.gadgetBounds.w;
        dock->restBox[i].Height = (WORD)env.gadgetBounds.h;
        if( dock->shownGap < 0 ) {
            dock->drawBox[i] = dock->restBox[i];
        }
        i++;
    }
    dock->snapCount = i;
    if( i > 1 ) {
        for( i = 0; i + 1 < dock->snapCount; i++ ) {
            if( DOCK_HORIZONTAL(dock) ) {
                dock->restBox[i].Width =
                    (WORD)(dock->restBox[i + 1].Left - dock->restBox[i].Left);
                if( dock->restBox[i].Width < 1 ) {
                    dock->restBox[i].Width = 1;
                }
            } else {
                dock->restBox[i].Height =
                    (WORD)(dock->restBox[i + 1].Top - dock->restBox[i].Top);
                if( dock->restBox[i].Height < 1 ) {
                    dock->restBox[i].Height = 1;
                }
            }
        }
        i = dock->snapCount - 1;
        if( DOCK_HORIZONTAL(dock) ) {
            dock->restBox[i].Width =
                (WORD)((WORD)dock->snapW - dock->restBox[i].Left);
            if( dock->restBox[i].Width < 1 ) {
                dock->restBox[i].Width = 1;
            }
        } else {
            dock->restBox[i].Height =
                (WORD)((WORD)dock->snapH - dock->restBox[i].Top);
            if( dock->restBox[i].Height < 1 ) {
                dock->restBox[i].Height = 1;
            }
        }
    }
    dock->gapSize = typical_cell(dock);
}

STATIC BOOL prepare_wb_snap(struct DockWindow *dock)
{
    struct RastPort rp;
    struct Rect r;

    if( ! ensure_snap(dock) ) {
        return FALSE;
    }
    copy_rest(dock);
    if( dock->snapCount < 1 ) {
        return FALSE;
    }
    if( dock->renderRP.BitMap && dock->snapBM ) {
        copy_rp(&dock->renderRP, dock->snapBM, dock->snapW, dock->snapH);
    }
    if( dock->snapBG ) {
        InitRastPort(&rp);
        rp.BitMap = dock->snapBG;
        r.x = 0;
        r.y = 0;
        r.w = dock->snapW;
        r.h = dock->snapH;
        fill_background(dock, &rp, &r);
        dock->snapBGReady = TRUE;
    }
    return TRUE;
}

STATIC VOID present_wb_vacant(struct DockWindow *dock)
{
    hook_glide(dock, NULL, dock->gapIndex);
}

STATIC VOID begin_wb_insert(struct DockWindow *dock, UWORD x, UWORD y)
{
    if( dock->wbVacant || dock->runState != RS_RUNNING ) {
        return;
    }
    if( ! prepare_wb_snap(dock) ) {
        DEBUG(DB_Printf("begin_wb_insert: snap failed\n"));
        return;
    }
    dock->wbVacant = TRUE;
    dock->dwellHold = TRUE;
    dock->runState = RS_REARRANGING;
    dock->rearrangeFromWB = TRUE;
    dock->dragNode = NULL;
    dock->hoverOverDock = FALSE;
    dock->gapSize = typical_cell(dock);
    dock->gapIndex = (WORD)rearrange_gap_index(dock, x, y);
    if( dock->gapIndex < 1 ) {
        dock->gapIndex = 1;
    }
    disable_notification(dock);
    present_wb_vacant(dock);
    DEBUG(DB_Printf("begin_wb_insert: gap=%ld size=%ld\n",
        (LONG)dock->gapIndex, (LONG)dock->gapSize));
}

STATIC VOID cancel_wb_insert(struct DockWindow *dock)
{
    if( ! dock->wbVacant ) {
        dock->dwellReady = FALSE;
        return;
    }
    DEBUG(DB_Printf("cancel_wb_insert\n"));
    hook_show(dock, NULL, (WORD)-1);
    finish_rearrange(dock, FALSE);
}

VOID rearrange_wb_tick(struct DockWindow *dock)
{
    struct Window *w;
    WORD mx;
    WORD my;
    WORD gap;

    if( dock->runState != RS_RUNNING
        && ! (dock->runState == RS_REARRANGING && dock->wbVacant) ) {
        return;
    }
    w = dock->win;
    if( w == NULL || w->WScreen == NULL ) {
        return;
    }
    mx = (WORD)(w->WScreen->MouseX - w->LeftEdge);
    my = (WORD)(w->WScreen->MouseY - w->TopEdge);

    if( mx < 0 || my < 0 || mx >= (WORD)w->Width || my >= (WORD)w->Height ) {
        if( dock->wbVacant || dock->dwellReady ) {
            cancel_wb_insert(dock);
            dock->dwellHold = FALSE;
            DEBUG(DB_Printf("rearrange_wb_tick: leave\n"));
        } else {
            dock->dwellReady = FALSE;
            dock->dwellHold = FALSE;
        }
        return;
    }

    if( dock->dwellHold && ! dock->wbVacant ) {
        return;
    }

    /* LMB up over the dock: keep the vacant slot, a drop may follow. */
    if( dock->dragArmed || InputBase == NULL
        || (PeekQualifier() & IEQUALIFIER_LEFTBUTTON) == 0 ) {
        if( ! dock->wbVacant ) {
            dock->dwellReady = FALSE;
        }
        return;
    }

    if( ! dock->dwellReady ) {
        if( IntuitionBase ) {
            dock->dwellSec = IntuitionBase->Seconds;
            dock->dwellMicro = IntuitionBase->Micros;
            dock->dwellReady = TRUE;
        }
        return;
    }

    if( ! dock->wbVacant && ! dwell_elapsed(dock) ) {
        return;
    }

    if( ! dock->wbVacant ) {
        begin_wb_insert(dock, (UWORD)mx, (UWORD)my);
        return;
    }

    gap = (WORD)rearrange_gap_index(dock, (UWORD)mx, (UWORD)my);
    if( gap < 1 ) {
        gap = 1;
    }
    if( gap != dock->gapIndex ) {
        dock->gapIndex = gap;
        present_wb_vacant(dock);
        DEBUG(DB_Printf("rearrange_wb_tick: gap=%ld\n", (LONG)gap));
    }
}

STATIC VOID add_zone(struct DockWindow *dock, ULONG id,
    WORD left, WORD top, WORD width, WORD height)
{
    struct IBox box;
    struct TagItem tags[3];
    struct AppWindowDropZone *z;

    if( dock->dropZoneCount >= REARRANGE_MAX_ZONES || dock->appWin == NULL ) {
        return;
    }
    /* SimpleDock: zone boxes are layout pixels (d->w/d->h), not win->Width.
       ChangeWindowBox has not reached the Window yet on first sync. */
    if( left < 0 ) {
        width = (WORD)(width + left);
        left = 0;
    }
    if( top < 0 ) {
        height = (WORD)(height + top);
        top = 0;
    }
    if( left + width > (WORD)dock->snapW ) {
        width = (WORD)((WORD)dock->snapW - left);
    }
    if( top + height > (WORD)dock->snapH ) {
        height = (WORD)((WORD)dock->snapH - top);
    }
    if( width < 2 || height < 2 ) {
        return;
    }
    /* SimpleDock: stack IBox; Workbench copies it before return. */
    box.Left = left;
    box.Top = top;
    box.Width = width;
    box.Height = height;
    tags[0].ti_Tag = WBDZA_Box;
    tags[0].ti_Data = (ULONG)&box;
    tags[1].ti_Tag = WBDZA_Hook;
    tags[1].ti_Data = (ULONG)&dock->dropHook;
    tags[2].ti_Tag = TAG_DONE;
    tags[2].ti_Data = 0;
    z = AddAppWindowDropZoneA(dock->appWin, id, 0, tags);
    if( z ) {
        dock->dropZones[dock->dropZoneCount] = z;
        dock->dropZoneCount++;
    } else {
        DEBUG(DB_Printf("add_zone: fail id=%ld box=%ld,%ld %ld x %ld io=%ld\n",
            (LONG)id, (LONG)left, (LONG)top, (LONG)width, (LONG)height,
            IoErr()));
    }
}

STATIC VOID add_cell_zones(struct DockWindow *dock)
{
    UWORD i;
    UWORD n;
    WORD band;
    WORD along0;
    WORD along1;
    WORD across0;
    WORD across1;
    WORD size;
    BOOL horiz;

    n = dock->snapCount;
    horiz = DOCK_HORIZONTAL(dock);
    if( n < 1 ) {
        return;
    }

    /* One zone per cell, like SimpleDock. WB must not get a full-window box. */
    for( i = 0; i < n; i++ ) {
        if( horiz ) {
            along0 = dock->restBox[i].Left;
            size = dock->restBox[i].Width;
            across0 = dock->restBox[i].Top;
            across1 = (WORD)(across0 + dock->restBox[i].Height);
        } else {
            along0 = dock->restBox[i].Top;
            size = dock->restBox[i].Height;
            across0 = dock->restBox[i].Left;
            across1 = (WORD)(across0 + dock->restBox[i].Width);
        }
        along1 = (WORD)(along0 + size);
        band = (WORD)(size / 5);
        if( band < 2 ) {
            band = 2;
        }
        if( i >= 1 ) {
            if( horiz ) {
                add_zone(dock, REARRANGE_ZONE_BASE + 2 * i,
                    along0, across0, band, (WORD)(across1 - across0));
                add_zone(dock, REARRANGE_ZONE_BASE + 2 * i + 1,
                    (WORD)(along0 + band), across0,
                    (WORD)(size - 2 * band), (WORD)(across1 - across0));
            } else {
                add_zone(dock, REARRANGE_ZONE_BASE + 2 * i,
                    across0, along0, (WORD)(across1 - across0), band);
                add_zone(dock, REARRANGE_ZONE_BASE + 2 * i + 1,
                    across0, (WORD)(along0 + band),
                    (WORD)(across1 - across0), (WORD)(size - 2 * band));
            }
        }
        if( i == n - 1 ) {
            if( horiz ) {
                add_zone(dock, REARRANGE_ZONE_BASE + 2 * (i + 1),
                    (WORD)(along1 - band), across0, band,
                    (WORD)(across1 - across0));
            } else {
                add_zone(dock, REARRANGE_ZONE_BASE + 2 * (i + 1),
                    across0, (WORD)(along1 - band),
                    (WORD)(across1 - across0), band);
            }
        }
    }

    if( dock->dropZoneCount == 0 ) {
        add_zone(dock, REARRANGE_ZONE_BASE + 2,
            0, 0, (WORD)dock->snapW, (WORD)dock->snapH);
    }
    DEBUG(DB_Printf("add_cell_zones: n=%ld zones=%ld\n",
        (LONG)n, (LONG)dock->dropZoneCount));
}

STATIC VOID clear_zones(struct DockWindow *dock)
{
    dz_remove(dock);
}

STATIC VOID rebuild_zones(struct DockWindow *dock)
{
    dz_add(dock);
}

VOID rearrange_init(struct DockWindow *dock)
{
    dock->gapIndex = (WORD)-1;
    dock->gapSize = 0;
    dock->dwellCount = 0;
    dock->dwellSec = 0;
    dock->dwellMicro = 0;
    dock->dwellReady = FALSE;
    dock->dwellHold = FALSE;
    dock->wbVacant = FALSE;
    dock->layoutX = 0;
    dock->layoutY = 0;
    dock->layoutW = 0;
    dock->layoutH = 0;
    dock->hoverOverDock = FALSE;
    dock->rearrangeFromWB = FALSE;
    dock->dragNode = NULL;
    dock->dragArmed = FALSE;
    dock->dropZoneCount = 0;
    dock->snapCount = 0;
    dock->zoneSnapCount = 0;
    dock->zoneSnapW = 0;
    dock->zoneSnapH = 0;
    dock->dropValid = FALSE;
    dock->shownGap = (WORD)-1;
    dock->dropOnto = (WORD)-1;
    dock->hookEnterCount = 0;
    dock->hookLeaveCount = 0;
    dock->motionHandle = NULL;
    dock->motionDisplay = NULL;
    dock->motionFrom = NULL;
    dock->motionModel = NULL;
    dock->motionCount = 0;
    dock->motionTick = 0;
    dock->motionBusy = FALSE;

    InitSemaphore(&dock->dropLock);
    dz_init(dock);

    if( dock->timerReq ) {
        TimerBase = dock->timerReq->tr_node.io_Device;
    }

    if( inputPort == NULL ) {
        inputPort = CreateMsgPort();
    }
    if( inputPort && inputReq == NULL ) {
        inputReq = (struct IOStdReq *)CreateIORequest(inputPort,
            sizeof(struct IOStdReq));
    }
    if( inputReq && InputBase == NULL ) {
        if( OpenDevice("input.device", 0, (struct IORequest *)inputReq, 0) == 0 ) {
            InputBase = inputReq->io_Device;
        }
    }

    if( MotionBase == NULL ) {
        MotionBase = OpenLibrary(MOTIONNAME, MOTIONVERSION);
    }
    DEBUG(DB_Printf("rearrange_init: MotionBase=%lx\n", (ULONG)MotionBase));
}

VOID rearrange_cleanup(struct DockWindow *dock)
{
    rearrange_remove_zone(dock);
    free_snap(dock);
    free_motion(dock);
    if( MotionBase ) {
        CloseLibrary(MotionBase);
        MotionBase = NULL;
    }
    if( inputReq ) {
        if( InputBase ) {
            CloseDevice((struct IORequest *)inputReq);
            InputBase = NULL;
        }
        DeleteIORequest((struct IORequest *)inputReq);
        inputReq = NULL;
    }
    if( inputPort ) {
        DeleteMsgPort(inputPort);
        inputPort = NULL;
    }
}

VOID rearrange_install_zone(struct DockWindow *dock)
{
    rearrange_sync(dock);
}

VOID rearrange_remove_zone(struct DockWindow *dock)
{
    dz_remove(dock);
    dock->zoneSnapCount = 0;
    dock->zoneSnapW = 0;
    dock->zoneSnapH = 0;
}

VOID rearrange_capture_bg(struct DockWindow *dock)
{
    if( dock->hoverOverDock || dock->runState == RS_REARRANGING ) {
        return;
    }
    if( dock->snapBG == NULL || dock->snapW != dock->renderW
        || dock->snapH != dock->renderH ) {
        return;
    }
    copy_rp(&dock->renderRP, dock->snapBG, dock->snapW, dock->snapH);
    dock->snapBGReady = TRUE;
}

VOID rearrange_sync(struct DockWindow *dock)
{
    if( dock->hoverOverDock ) {
        return;
    }
    dz_add(dock);
}

VOID rearrange_wb_enter(struct DockWindow *dock)
{
    dock->hoverOverDock = TRUE;
    if( ! dock->rearrangeFromWB ) {
        dock->dwellCount = 0;
    }
}

VOID rearrange_wb_leave(struct DockWindow *dock)
{
    dock->hoverOverDock = FALSE;
    dock->dwellCount = 0;
    dock->rearrangeFromWB = FALSE;
}

VOID rearrange_tick(struct DockWindow *dock)
{
    if( dock->hoverOverDock ) {
        return;
    }
    if( dock->runState == RS_REARRANGING ) {
        step_rearrange(dock);
        return;
    }
}

VOID rearrange_intuitick(struct DockWindow *dock)
{
    if( dock->runState == RS_REARRANGING ) {
        step_rearrange(dock);
    }
}

VOID rearrange_mouse_down(struct DockWindow *dock, UWORD x, UWORD y)
{
    struct DgNode *dg;
    struct GadgetEnvironment env;

    dock->dragArmed = FALSE;
    dock->dragNode = NULL;
    if( dock->hoverOverDock || dock->dwellHold || dock->wbVacant
        || dock->runState != RS_RUNNING ) {
        return;
    }
    dg = get_gadget_at(dock, x, y);
    if( dg == NULL ) {
        return;
    }
    TD_GetDockGadgetEnvironment(dg->dg, &env);
    if( env.index == 0 ) {
        dock->dragNode = dg;
        dock->dragArmed = TRUE;
        dock->dragStartX = x;
        dock->dragStartY = y;
        return;
    }
    dock->dragNode = dg;
    dock->dragArmed = TRUE;
    dock->dragStartX = x;
    dock->dragStartY = y;
}

VOID rearrange_mouse_move(struct DockWindow *dock, UWORD x, UWORD y)
{
    WORD dx;
    WORD dy;
    UWORD gap;

    if( dock->hoverOverDock ) {
        return;
    }

    if( dock->runState == RS_REARRANGING && dock->dragNode ) {
        gap = rearrange_gap_index(dock, x, y);
        if( gap != dock->gapIndex ) {
            dock->gapIndex = gap;
            retarget(dock);
        } else {
            rearrange_draw(dock);
        }
        return;
    }

    if( ! dock->dragArmed || dock->dragNode == NULL
        || dock->runState != RS_RUNNING ) {
        return;
    }

    dx = (WORD)x - (WORD)dock->dragStartX;
    dy = (WORD)y - (WORD)dock->dragStartY;
    if( dx < 0 ) {
        dx = (WORD)-dx;
    }
    if( dy < 0 ) {
        dy = (WORD)-dy;
    }
    if( dx < REARRANGE_DRAG_PX && dy < REARRANGE_DRAG_PX ) {
        return;
    }

    {
        struct GadgetEnvironment env;

        TD_GetDockGadgetEnvironment(dock->dragNode->dg, &env);
        if( env.index == 0 || dock->dragNode == dock->placeholder ) {
            dock->dragArmed = FALSE;
            dock->dragNode = NULL;
            return;
        }
    }

    begin_rearrange(dock, FALSE, x, y);
}

BOOL rearrange_mouse_up(struct DockWindow *dock, UWORD x, UWORD y)
{
    struct DgNode *node;
    BOOL clicked;

    clicked = FALSE;
    if( dock->dwellHold || dock->wbVacant ) {
        dock->dragArmed = FALSE;
        dock->dragNode = NULL;
        return FALSE;
    }
    if( dock->runState == RS_REARRANGING && dock->dragNode
        && ! dock->rearrangeFromWB ) {
        commit_move(dock);
        return FALSE;
    }

    if( dock->dragArmed && dock->runState == RS_RUNNING && dock->dragNode ) {
        node = dock->dragNode;
        dock->dragArmed = FALSE;
        dock->dragNode = NULL;
        if( node == dock->placeholder ) {
            open_dock_prefs(dock);
        } else {
            dock_gadget_click(node->dg, x, y);
        }
        clicked = TRUE;
        return clicked;
    }

    dock->dragArmed = FALSE;
    dock->dragNode = NULL;
    return clicked;
}

STATIC BOOL class_name_is(STRPTR a, STRPTR b)
{
    if( a == NULL || b == NULL ) {
        return FALSE;
    }
    while( *a && *b && *a == *b ) {
        a++;
        b++;
    }
    return (BOOL)(*a == 0 && *b == 0);
}

STATIC BOOL gadget_takes_drop(Object *obj)
{
    Class *cl;
    STRPTR id;

    if( obj == NULL ) {
        return FALSE;
    }
    cl = OCLASS(obj);
    if( cl == NULL ) {
        return FALSE;
    }
    id = cl->cl_ID;
    if( class_name_is(id, DB_BUTTON_CLASS) ) {
        return TRUE;
    }
    if( class_name_is(id, DB_OPEN_CLASS) ) {
        return TRUE;
    }
    if( class_name_is(id, DB_TRASH_CLASS) ) {
        return TRUE;
    }
    return FALSE;
}

STATIC BOOL dwell_elapsed(struct DockWindow *dock)
{
    ULONG nowSec;
    ULONG nowMicro;
    ULONG ms;

    if( dock == NULL || ! dock->dwellReady ) {
        return FALSE;
    }
    nowSec = 0;
    nowMicro = 0;
    if( IntuitionBase ) {
        nowSec = IntuitionBase->Seconds;
        nowMicro = IntuitionBase->Micros;
    }
    if( nowSec < dock->dwellSec ) {
        return FALSE;
    }
    ms = (nowSec - dock->dwellSec) * 1000UL;
    if( nowMicro >= dock->dwellMicro ) {
        ms += (nowMicro - dock->dwellMicro) / 1000UL;
    } else {
        if( ms >= 1000UL ) {
            ms -= 1000UL;
        }
        ms += (1000000UL + nowMicro - dock->dwellMicro) / 1000UL;
    }
    return (BOOL)(ms >= (ULONG)REARRANGE_DWELL_MS);
}

BOOL rearrange_want_insert(struct DockWindow *dock, struct AppMessage *msg,
    UWORD *dest)
{
    struct DgNode *dg;
    struct GadgetEnvironment env;
    BOOL dwelt;
    BOOL takes;

    if( msg == NULL || dest == NULL ) {
        return FALSE;
    }

    if( dock->gapIndex >= 1 ) {
        *dest = (UWORD)dock->gapIndex;
    } else {
        *dest = rearrange_gap_index(dock, msg->am_MouseX, msg->am_MouseY);
        if( *dest < 1 ) {
            *dest = 1;
        }
    }

    /* Insert only after the pointer has sat on the dock for a beat.
       An immediate drop on Open/Button/Trash uses that item instead. */
    dwelt = dock->wbVacant ? TRUE : dwell_elapsed(dock);

    dock->hoverOverDock = FALSE;
    dock->rearrangeFromWB = FALSE;
    dock->dwellCount = 0;
    dock->dwellReady = FALSE;
    dock->dwellHold = TRUE;
    dock->shownGap = (WORD)-1;
    dock->dropOnto = (WORD)-1;
    if( dock->runState == RS_REARRANGING ) {
        finish_rearrange(dock, FALSE);
    }
    dock->gapIndex = (WORD)-1;

    if( dock->placeholder ) {
        DEBUG(DB_Printf("rearrange_want_insert: placeholder\n"));
        return TRUE;
    }
    if( dwelt ) {
        DEBUG(DB_Printf("rearrange_want_insert: dwelt dest=%ld\n", (LONG)*dest));
        return TRUE;
    }

    dg = get_gadget_at(dock, msg->am_MouseX, msg->am_MouseY);
    takes = FALSE;
    env.index = 0;
    if( dg ) {
        TD_GetDockGadgetEnvironment(dg->dg, &env);
        if( env.index != 0 ) {
            takes = gadget_takes_drop(dg->dg);
        }
    }
    if( takes ) {
        DEBUG(DB_Printf("rearrange_want_insert: use gadget\n"));
        return FALSE;
    }
    DEBUG(DB_Printf("rearrange_want_insert: insert dest=%ld\n", (LONG)*dest));
    return TRUE;
}

VOID rearrange_commit_drop(struct DockWindow *dock)
{
    DEBUG(DB_Printf("rearrange_commit_drop\n"));
    dock->dwellHold = FALSE;
    dz_remove(dock);
    remap_gadgets(dock);
    layout_gadgets(dock);
    draw_gadgets(dock);
    save_config(dock);
    dz_add(dock);
}

VOID rearrange_draw(struct DockWindow *dock)
{
    struct GadgetEnvironment env;
    UWORD x;
    UWORD y;
    WORD gx;
    WORD gy;

    draw_gadgets(dock);
    if( dock->runState != RS_REARRANGING || dock->dragNode == NULL
        || dock->rearrangeFromWB ) {
        return;
    }
    pointer_in_window(dock, &x, &y);
    TD_GetDockGadgetEnvironment(dock->dragNode->dg, &env);
    env.gadgetBounds.w = dock->dragW;
    env.gadgetBounds.h = dock->dragH;
    gx = (WORD)x - (WORD)dock->dragW / 2;
    gy = (WORD)y - (WORD)dock->dragH / 2;
    if( gx < 0 ) {
        gx = 0;
    }
    if( gy < 0 ) {
        gy = 0;
    }
    if( dock->win ) {
        if( gx + (WORD)dock->dragW > (WORD)dock->win->Width ) {
            gx = (WORD)dock->win->Width - (WORD)dock->dragW;
        }
        if( gy + (WORD)dock->dragH > (WORD)dock->win->Height ) {
            gy = (WORD)dock->win->Height - (WORD)dock->dragH;
        }
        if( gx < 0 ) {
            gx = 0;
        }
        if( gy < 0 ) {
            gy = 0;
        }
    }
    env.gadgetBounds.x = (UWORD)gx;
    env.gadgetBounds.y = (UWORD)gy;
    dock_gadget_set_environment(dock->dragNode->dg, &env);
    draw_gadget(dock, dock->dragNode->dg);
}
