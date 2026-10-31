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

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <exec/types.h>

#ifdef DEBUG_BUILD

VOID parse_args(VOID);
VOID DB_Printf(STRPTR fmt, ...);

#endif

#endif

