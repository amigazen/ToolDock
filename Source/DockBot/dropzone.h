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

#ifndef __DROPZONE_H__
#define __DROPZONE_H__

#include "dock.h"

/* SimpleDock drop-zone hover. Workbench V44+. */
VOID dz_init(struct DockWindow *dock);
VOID dz_add(struct DockWindow *dock);
VOID dz_remove(struct DockWindow *dock);
VOID dz_reset(struct DockWindow *dock);
VOID dz_lock(VOID);
VOID dz_unlock(VOID);
LONG dz_gap(ULONG id);
LONG dz_icon(ULONG id);

#endif
