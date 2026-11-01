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

#ifndef __DOCK_APPICON_H__
#define __DOCK_APPICON_H__

#include <intuition/classes.h>

struct DockIcon;

Class *init_dock_appicon_class(VOID);
BOOL free_dock_appicon_class(Class *c);
VOID dock_appicon_bind(Object *o, struct DockIcon *icon);

#endif
