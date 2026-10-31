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

#ifndef __DOCK_HANDLE_H__
#define __DOCK_HANDLE_H__

#include <intuition/classes.h>

#define HANDLE_CLASS_NAME "DockHandle"

Class *init_dock_handle_class(VOID);

BOOL free_dock_handle_class(Class *c);

#endif