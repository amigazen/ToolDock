/************************************
**
**  ToolDock - A Dock For AmigaOS 3
**
**  SPDX-License-Identifier: MIT
**
**  ToolDock changes Copyright 2026 amigazen project
**  DockBot Copyright 2019 Andrew Kennan
**
**  SimpleDock src/dropzone.c port. wb.doc AddAppWindowDropZoneA (V44):
**  the hook runs on Workbench's task with the display locked. Only
**  graphics.library drawing into adzm_RastPort. Return NULL. A zone
**  needs a position and a size (WBDZA_Box). RelWidth/RelHeight of 0
**  means omit. Once a zone exists, drops are AMTYPE_APPWINDOWZONE.
**  The dock task must not call Intuition until after ReplyMsg.
**
************************************/

#include "dropzone.h"
#include "rearrange.h"
#include "debug.h"

#include <clib/compiler-specific.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <utility/hooks.h>
#include <workbench/workbench.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <utility/tagitem.h>
#include <devices/timer.h>

#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <clib/graphics_protos.h>
#include <clib/timer_protos.h>
#include <clib/wb_protos.h>

extern struct Library *WorkbenchBase;
extern struct Library *GfxBase;
extern struct Device *TimerBase;

#include <pragmas/graphics_pragmas.h>
#include <pragmas/timer_pragmas.h>
#include <pragmas/wb_pragmas.h>

#define DZ_GAP_ID    1000
#define DZ_MAXZONES  96
#define DZ_MAXITEMS  48
#define DZ_GLIDE_MS  120

STATIC struct Hook dzHook;
STATIC struct AppWindowDropZone *dzZone[DZ_MAXZONES];
STATIC LONG dzNZones;
STATIC struct BitMap *dzHbm;
STATIC struct BitMap *dzRest;
STATIC WORD dzPos[(DZ_MAXITEMS + 1) * 2];
STATIC WORD dzRestX[DZ_MAXITEMS];
STATIC WORD dzRestY[DZ_MAXITEMS];
STATIC WORD dzRestW[DZ_MAXITEMS];
STATIC WORD dzRestH[DZ_MAXITEMS];
STATIC LONG dzN;
STATIC WORD dzW;
STATIC WORD dzH;
STATIC BOOL dzHoriz;
STATIC LONG dzShown;
STATIC struct SignalSemaphore dzLock;
STATIC BOOL dzValid;

STATIC VOID dz_place(LONG k, LONG gap, WORD *x, WORD *y)
{
    LONG i;
    LONG used;
    LONG extra;
    LONG avail;
    LONG along;
    LONG cell;

    if( k < 0 || k >= dzN ) {
        *x = 0;
        *y = 0;
        return;
    }
    used = 0;
    for( i = 0; i < dzN; i++ ) {
        used += dzHoriz ? dzRestW[i] : dzRestH[i];
    }
    extra = 0;
    if( gap >= 1 ) {
        extra = 48;
        if( dzN > 1 ) {
            extra = dzHoriz ? dzRestW[1] : dzRestH[1];
        }
        if( extra < 8 ) {
            extra = 48;
        }
    }
    avail = dzHoriz ? dzW : dzH;
    if( avail < 1 || used + extra < 1 ) {
        *x = dzRestX[k];
        *y = dzRestY[k];
        return;
    }
    along = 0;
    for( i = 0; i < dzN; i++ ) {
        if( gap >= 1 && gap == i && i >= 1 ) {
            along += extra * avail / (used + extra);
        }
        cell = (dzHoriz ? dzRestW[i] : dzRestH[i]) * avail / (used + extra);
        if( cell < 1 ) {
            cell = 1;
        }
        if( i == k ) {
            if( dzHoriz ) {
                *x = (WORD)along;
                *y = dzRestY[k];
            } else {
                *x = dzRestX[k];
                *y = (WORD)along;
            }
            return;
        }
        along += cell;
    }
    *x = dzRestX[k];
    *y = dzRestY[k];
}

/* One frame: rest tiles squeezed toward 'gap'. p = 0 rest, 1024 dest. */
STATIC VOID dz_frame(struct RastPort *wrp, LONG gap, LONG p, BOOL last)
{
    struct RastPort rp;
    LONG k;
    WORD x;
    WORD y;
    WORD sx;
    WORD sy;
    WORD dx;
    WORD dy;
    WORD bw;
    WORD bh;

    if( wrp == NULL || wrp->BitMap == NULL
        || dzHbm == NULL || dzRest == NULL || dzW < 2 || dzH < 2 ) {
        return;
    }

    InitRastPort(&rp);
    rp.BitMap = dzHbm;
    SetAPen(&rp, 0);
    RectFill(&rp, 0, 0, (WORD)(dzW - 1), (WORD)(dzH - 1));

    for( k = 0; k < dzN; k++ ) {
        dz_place(k, gap, &x, &y);
        x = (WORD)(dzPos[2 * k] + ((LONG)x - dzPos[2 * k]) * p / 1024);
        y = (WORD)(dzPos[2 * k + 1] + ((LONG)y - dzPos[2 * k + 1]) * p / 1024);
        sx = dzRestX[k];
        sy = dzRestY[k];
        dx = x;
        dy = y;
        bw = dzRestW[k];
        bh = dzRestH[k];
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
        if( sx + bw > dzW ) {
            bw = (WORD)(dzW - sx);
        }
        if( sy + bh > dzH ) {
            bh = (WORD)(dzH - sy);
        }
        if( dx + bw > dzW ) {
            bw = (WORD)(dzW - dx);
        }
        if( dy + bh > dzH ) {
            bh = (WORD)(dzH - dy);
        }
        if( bw < 1 || bh < 1 ) {
            continue;
        }
        BltBitMap(dzRest, sx, sy, dzHbm, dx, dy, bw, bh, 0xC0, 0xFF, NULL);
        if( last ) {
            dzPos[2 * k] = x;
            dzPos[2 * k + 1] = y;
        }
    }
    WaitBlit();
    BltBitMapRastPort(dzHbm, 0, 0, wrp, 0, 0, dzW, dzH, 0xC0);
}

STATIC VOID dz_glide(struct RastPort *wrp, LONG gap)
{
    struct timeval t0;
    struct timeval now;
    LONG t;

    if( dzHbm == NULL || dzRest == NULL || gap == dzShown ) {
        return;
    }
    if( TimerBase == NULL ) {
        dzShown = gap;
        dz_frame(wrp, gap, 1024, TRUE);
        return;
    }
    dzShown = gap;
    GetSysTime(&t0);
    for(;;) {
        GetSysTime(&now);
        SubTime(&now, &t0);
        t = now.tv_secs * 1000 + now.tv_micro / 1000;
        if( t >= DZ_GLIDE_MS || t < 0 ) {
            break;
        }
        dz_frame(wrp, gap, t * 1024 / DZ_GLIDE_MS, FALSE);
    }
    dz_frame(wrp, gap, 1024, TRUE);
}

STATIC BOOL dz_pointer_in(struct DockWindow *dock)
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

/* Workbench's task. graphics.library only. Always return NULL.
   Drawing from this hook was a Recoverable Alert (System Request
   with corrupt text). The hook is not installed. */
STATIC __SAVE_DS__ __ASM__ ULONG dz_hook(
    __REG__(a0, struct Hook *h),
    __REG__(a2, APTR reserved),
    __REG__(a1, struct AppWindowDropZoneMsg *m))
{
    (void)h;
    (void)reserved;
    (void)m;
    return 0;
}

#if 0
STATIC __SAVE_DS__ __ASM__ ULONG dz_hook_unused(
    __REG__(a0, struct Hook *h),
    __REG__(a2, APTR reserved),
    __REG__(a1, struct AppWindowDropZoneMsg *m))
{
    struct DockWindow *dock;
    LONG icon;
    LONG gap;
    LONG v;

    (void)reserved;
    dock = (struct DockWindow *)h->h_Data;
    if( dock == NULL || m == NULL ) {
        return 0;
    }
    if( ! AttemptSemaphore(&dzLock) ) {
        return 0;
    }
    if( dzValid ) {
        if( m->adzm_Action == ADZMACTION_Enter ) {
            v = (LONG)m->adzm_ID - DZ_GAP_ID;
            icon = -1;
            gap = -1;
            if( v >= 0 && v < 4000 ) {
                if( v & 1 ) {
                    icon = v / 2;
                } else {
                    gap = v / 2;
                }
            }
            if( icon >= 1 ) {
                gap = icon;
            }
            if( gap < 1 ) {
                gap = 1;
            }
            dock->hoverOverDock = TRUE;
            dock->gapIndex = (WORD)gap;
            dock->dropOnto = (WORD)-1;
            dock->dragArmed = FALSE;
            dock->dragNode = NULL;
            dz_glide(m->adzm_RastPort, gap);
        } else if( m->adzm_Action == ADZMACTION_Leave ) {
            if( ! dz_pointer_in(dock) ) {
                dz_glide(m->adzm_RastPort, -1);
                dock->hoverOverDock = FALSE;
                dock->gapIndex = (WORD)-1;
                dock->dropOnto = (WORD)-1;
            }
        }
    }
    ReleaseSemaphore(&dzLock);
    return 0;
}
#endif

STATIC VOID dz_add_zone(struct DockWindow *dock, ULONG id,
    WORD a0, WORD a1, WORD b0, WORD b1)
{
    struct IBox box;
    struct TagItem tags[3];
    struct AppWindowDropZone *z;
    WORD alen;

    alen = dzHoriz ? dzW : dzH;
    if( a0 < 0 ) {
        a0 = 0;
    }
    if( a1 > alen ) {
        a1 = alen;
    }
    if( dzNZones >= DZ_MAXZONES || a1 <= a0 || b1 <= b0 ) {
        return;
    }
    if( dzHoriz ) {
        box.Left = a0;
        box.Width = (WORD)(a1 - a0);
        box.Top = b0;
        box.Height = (WORD)(b1 - b0);
    } else {
        box.Left = b0;
        box.Width = (WORD)(b1 - b0);
        box.Top = a0;
        box.Height = (WORD)(a1 - a0);
    }
    if( box.Width < 1 || box.Height < 1 ) {
        return;
    }
    if( box.Left < 0 || box.Top < 0 ) {
        return;
    }
    if( box.Left + box.Width > dzW || box.Top + box.Height > dzH ) {
        return;
    }
    tags[0].ti_Tag = WBDZA_Box;
    tags[0].ti_Data = (ULONG)&box;
    tags[1].ti_Tag = WBDZA_Hook;
    tags[1].ti_Data = (ULONG)&dzHook;
    tags[2].ti_Tag = TAG_DONE;
    tags[2].ti_Data = 0;
    z = AddAppWindowDropZoneA(dock->appWin, id, 0, tags);
    if( z ) {
        dzZone[dzNZones] = z;
        dzNZones++;
    }
}

VOID dz_lock(VOID)
{
    if( dzLock.ss_Link.ln_Type == 0 ) {
        InitSemaphore(&dzLock);
    }
    ObtainSemaphore(&dzLock);
}

VOID dz_unlock(VOID)
{
    if( dzLock.ss_Link.ln_Type ) {
        ReleaseSemaphore(&dzLock);
    }
}

VOID dz_init(struct DockWindow *dock)
{
    dzNZones = 0;
    dzHbm = NULL;
    dzRest = NULL;
    dzN = 0;
    dzW = 0;
    dzH = 0;
    dzShown = -1;
    dzValid = FALSE;
    if( dzLock.ss_Link.ln_Type == 0 ) {
        InitSemaphore(&dzLock);
    }
    dzHook.h_Entry = (HOOKFUNC)dz_hook;
    dzHook.h_SubEntry = NULL;
    dzHook.h_Data = dock;
}

VOID dz_remove(struct DockWindow *dock)
{
    if( dzLock.ss_Link.ln_Type ) {
        ObtainSemaphore(&dzLock);
        dzValid = FALSE;
        ReleaseSemaphore(&dzLock);
    }
    while( dzNZones > 0 ) {
        dzNZones--;
        if( dock && dock->appWin && dzZone[dzNZones] ) {
            RemoveAppWindowDropZone(dock->appWin, dzZone[dzNZones]);
        }
        dzZone[dzNZones] = NULL;
    }
    WaitBlit();
    if( dzHbm ) {
        FreeBitMap(dzHbm);
        dzHbm = NULL;
    }
    if( dzRest ) {
        FreeBitMap(dzRest);
        dzRest = NULL;
    }
    dzShown = -1;
    dzN = 0;
    if( dock ) {
        dock->dropZoneCount = 0;
        dock->dropValid = FALSE;
        dock->hoverOverDock = FALSE;
    }
}

VOID dz_reset(struct DockWindow *dock)
{
    LONG k;
    WORD x;
    WORD y;

    (void)dock;
    ObtainSemaphore(&dzLock);
    dzShown = -1;
    for( k = 0; k < dzN; k++ ) {
        dz_place(k, -1, &x, &y);
        dzPos[2 * k] = x;
        dzPos[2 * k + 1] = y;
    }
    ReleaseSemaphore(&dzLock);
}

LONG dz_gap(ULONG id)
{
    LONG v;

    v = (LONG)id - DZ_GAP_ID;
    if( v >= 0 && v < 4000 && (v & 1) == 0 ) {
        return v / 2;
    }
    return -1;
}

LONG dz_icon(ULONG id)
{
    LONG v;

    v = (LONG)id - DZ_GAP_ID;
    if( v >= 0 && v < 4000 && (v & 1) ) {
        return v / 2;
    }
    return -1;
}

VOID dz_add(struct DockWindow *dock)
{
    /* AddAppWindowDropZone + hook blit is the Recoverable Alert with
       corrupt text. AppWindow still accepts drops (AMTYPE_APPWINDOW). */
    (void)dock;
    DEBUG(DB_Printf("dz_add: no zones\n"));
    return;
}

#if 0
VOID dz_add_unused(struct DockWindow *dock)
{
    struct Window *win;
    struct BitMap *friend;
    struct RastPort rp;
    struct DgNode *curr;
    struct GadgetEnvironment env;
    ULONG depth;
    LONG k;
    WORD alen;
    WORD blen;
    WORD start;
    WORD size;
    WORD band;
    WORD x;
    WORD y;

    if( dock == NULL || dock->appWin == NULL || dock->win == NULL ) {
        return;
    }
    if( WorkbenchBase == NULL || WorkbenchBase->lib_Version < 44 ) {
        return;
    }
    if( dzLock.ss_Link.ln_Type == 0 ) {
        InitSemaphore(&dzLock);
    }
    if( dzNZones ) {
        return;
    }
    win = dock->win;
    dzW = (WORD)win->Width;
    dzH = (WORD)win->Height;
    if( dzW < 8 || dzH < 8 ) {
        return;
    }
    dzHoriz = DOCK_HORIZONTAL(dock);
    dzHook.h_Data = dock;

    k = 0;
    FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {
        if( k >= DZ_MAXITEMS ) {
            break;
        }
        TD_GetDockGadgetEnvironment(curr->dg, &env);
        dzRestX[k] = (WORD)env.gadgetBounds.x;
        dzRestY[k] = (WORD)env.gadgetBounds.y;
        dzRestW[k] = (WORD)env.gadgetBounds.w;
        dzRestH[k] = (WORD)env.gadgetBounds.h;
        if( dzRestW[k] < 1 ) {
            dzRestW[k] = 1;
        }
        if( dzRestH[k] < 1 ) {
            dzRestH[k] = 1;
        }
        k++;
    }
    dzN = k;
    if( dzN < 1 ) {
        return;
    }

    /* SimpleDock: hbm is a friend of a plain offscreen bitmap, never of
       the layered window bitmap. rest is a ClipBlit of the dock image. */
    friend = NULL;
    depth = 8;
    if( win->WScreen && win->WScreen->RastPort.BitMap ) {
        friend = win->WScreen->RastPort.BitMap;
        depth = GetBitMapAttr(friend, BMA_DEPTH);
    }
    if( friend == NULL || depth < 1 ) {
        return;
    }
    dzRest = AllocBitMap((UWORD)dzW, (UWORD)dzH, depth, BMF_MINPLANES, friend);
    if( dzRest == NULL ) {
        return;
    }
    dzHbm = AllocBitMap((UWORD)dzW, (UWORD)dzH, depth, BMF_MINPLANES, dzRest);
    if( dzHbm == NULL ) {
        dz_remove(dock);
        return;
    }
    if( dock->renderRP.BitMap ) {
        InitRastPort(&rp);
        rp.BitMap = dzRest;
        ClipBlit(&dock->renderRP, 0, 0, &rp, 0, 0, dzW, dzH, 0xC0);
    }
    for( k = 0; k < dzN; k++ ) {
        dz_place(k, -1, &x, &y);
        dzPos[2 * k] = x;
        dzPos[2 * k + 1] = y;
    }
    dzShown = -1;

    alen = dzHoriz ? dzW : dzH;
    blen = dzHoriz ? dzH : dzW;
    if( dzN <= 1 ) {
        dz_add_zone(dock, DZ_GAP_ID + 2, 0, alen, 0, blen);
    } else {
        for( k = 0; k < dzN; k++ ) {
            start = dzHoriz ? dzRestX[k] : dzRestY[k];
            size = dzHoriz ? dzRestW[k] : dzRestH[k];
            band = (WORD)(size / 5);
            if( band < 1 ) {
                band = 1;
            }
            if( k >= 1 ) {
                dz_add_zone(dock, DZ_GAP_ID + 2 * k,
                    start, (WORD)(start + band), 0, blen);
                dz_add_zone(dock, DZ_GAP_ID + 2 * k + 1,
                    (WORD)(start + band), (WORD)(start + size - band), 0, blen);
            }
            if( k == dzN - 1 ) {
                dz_add_zone(dock, DZ_GAP_ID + 2 * (k + 1),
                    (WORD)(start + size - band), alen, 0, blen);
            }
        }
    }
    if( dzNZones == 0 ) {
        dz_remove(dock);
        return;
    }
    ObtainSemaphore(&dzLock);
    dzValid = TRUE;
    ReleaseSemaphore(&dzLock);
    dock->dropZoneCount = (UWORD)dzNZones;
    dock->dropValid = TRUE;
    DEBUG(DB_Printf("dz_add: n=%ld zones=%ld win=%ld x %ld\n",
        dzN, dzNZones, (LONG)dzW, (LONG)dzH));
}
#endif
