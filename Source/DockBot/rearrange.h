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

#ifndef __REARRANGE_H__
#define __REARRANGE_H__

#include "dock.h"

#define REARRANGE_DWELL_TICKS  2
#define REARRANGE_DWELL_MS     500
#define REARRANGE_GLIDE_MS     120
#define REARRANGE_DRAG_PX      6
#define REARRANGE_MOTION_TICKS 2
#define REARRANGE_TICK_MS      50

VOID rearrange_init(struct DockWindow *dock);
VOID rearrange_cleanup(struct DockWindow *dock);

VOID rearrange_install_zone(struct DockWindow *dock);
VOID rearrange_remove_zone(struct DockWindow *dock);
VOID rearrange_capture_bg(struct DockWindow *dock);
VOID rearrange_sync(struct DockWindow *dock);

VOID rearrange_tick(struct DockWindow *dock);
VOID rearrange_intuitick(struct DockWindow *dock);

VOID rearrange_mouse_down(struct DockWindow *dock, UWORD x, UWORD y);
VOID rearrange_mouse_move(struct DockWindow *dock, UWORD x, UWORD y);
BOOL rearrange_mouse_up(struct DockWindow *dock, UWORD x, UWORD y);

VOID rearrange_wb_enter(struct DockWindow *dock);
VOID rearrange_wb_leave(struct DockWindow *dock);

BOOL rearrange_want_insert(struct DockWindow *dock, struct AppMessage *msg,
    UWORD *dest);
VOID rearrange_commit_drop(struct DockWindow *dock);

VOID rearrange_cancel(struct DockWindow *dock);

UWORD rearrange_gap_index(struct DockWindow *dock, UWORD x, UWORD y);
UWORD rearrange_count(struct DockWindow *dock);
struct DgNode *rearrange_node_at(struct DockWindow *dock, UWORD index);

VOID rearrange_draw(struct DockWindow *dock);

#endif
