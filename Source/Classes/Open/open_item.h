/************************************
**
**  ToolDock - A Dock For AmigaOS 3
**
**  SPDX-License-Identifier: MIT
**
**  ToolDock changes Copyright 2026 amigazen project
**  DockBot Copyright 2019 Andrew Kennan
**
**  WBPROJECT icons open with the default tool. Everything else
**  uses WBInfo().
**
************************************/

#ifndef __OPEN_ITEM_H__
#define __OPEN_ITEM_H__

#include <exec/types.h>
#include <dos/dos.h>

BOOL open_dropped_path(STRPTR path, STRPTR defaultTool);

#endif
