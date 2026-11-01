/************************************
**
**  ToolDock - A Dock For AmigaOS 3
**
**  SPDX-License-Identifier: MIT
**
**  ToolDock changes Copyright 2026 amigazen project
**
************************************/

#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/screens.h>
#include <exec/memory.h>

#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include "dockbot.h"
#include "dockbot_protos.h"
#include "dockbot_pragmas.h"

#include "class_def.h"

extern struct GfxBase *GfxBase;

#define SPACER_THICK 8

ULONG __saveds spacer_lib_init(struct SpacerLibData *sld)
{
    if( sld->graphicsLib = OpenLibrary("graphics.library", 37) ) {
        GfxBase = (struct GfxBase *)sld->graphicsLib;
        return 1;
    }
    return 0;
}

ULONG __saveds spacer_lib_expunge(struct SpacerLibData *sld)
{
    if( sld->graphicsLib ) {
        CloseLibrary(sld->graphicsLib);
    }
    return 1;
}

DB_METHOD_M(DRAW, DockMessageDraw)

    struct GadgetEnvironment env;
    struct Screen *screen;
    struct DrawInfo *di;
    WORD x1, y1, x2, y2;
    WORD mx, my;

    TD_GetDockGadgetEnvironment(o, &env);
    if( env.gadgetBounds.w < 2 || env.gadgetBounds.h < 2 ) {
        return 1;
    }

    /* Bevel only. No frame around the gadget. */
    if( screen = LockPubScreen(NULL) ) {
        if( di = GetScreenDrawInfo(screen) ) {
            if( env.pos == DP_TOP || env.pos == DP_BOTTOM ) {
                mx = env.gadgetBounds.x + env.gadgetBounds.w / 2;
                y1 = env.gadgetBounds.y + 2;
                y2 = env.gadgetBounds.y + env.gadgetBounds.h - 3;
                SetAPen(msg->rp, di->dri_Pens[SHINEPEN]);
                Move(msg->rp, mx, y1);
                Draw(msg->rp, mx, y2);
                SetAPen(msg->rp, di->dri_Pens[SHADOWPEN]);
                Move(msg->rp, mx + 1, y1);
                Draw(msg->rp, mx + 1, y2);
            } else {
                my = env.gadgetBounds.y + env.gadgetBounds.h / 2;
                x1 = env.gadgetBounds.x + 2;
                x2 = env.gadgetBounds.x + env.gadgetBounds.w - 3;
                SetAPen(msg->rp, di->dri_Pens[SHINEPEN]);
                Move(msg->rp, x1, my);
                Draw(msg->rp, x2, my);
                SetAPen(msg->rp, di->dri_Pens[SHADOWPEN]);
                Move(msg->rp, x1, my + 1);
                Draw(msg->rp, x2, my + 1);
            }
            FreeScreenDrawInfo(screen, di);
        }
        UnlockPubScreen(NULL, screen);
    }
    return 1;
}

DB_METHOD_M(GETSIZE, DockMessageGetSize)

    if( msg->position == DP_TOP || msg->position == DP_BOTTOM ) {
        msg->w = SPACER_THICK;
        msg->h = DEFAULT_SIZE;
    } else {
        msg->w = DEFAULT_SIZE;
        msg->h = SPACER_THICK;
    }
    return 1;
}

DB_METHOD_M(GETLABEL, DockMessageGetLabel)

    msg->label = (STRPTR)MSG_Label;
    return 1;
}
