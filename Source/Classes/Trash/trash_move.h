/************************************
**
**  ToolDock - A Dock For AmigaOS 3
**
**  SPDX-License-Identifier: MIT
**
**  ToolDock changes Copyright 2026 amigazen project
**  DockBot Copyright 2019 Andrew Kennan
**
**  Same move-to-trash rules as WBTrash trash_ops:
**  same-volume Rename into Volume:Trashcan, paired .info, WBT= metadata.
**
************************************/

#ifndef __TRASH_MOVE_H__
#define __TRASH_MOVE_H__

#include <exec/types.h>
#include <dos/dos.h>

BOOL trash_move_path(STRPTR src);
BOOL trash_empty(VOID);

#endif
