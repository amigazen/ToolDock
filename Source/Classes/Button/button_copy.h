/************************************
**
**  ToolDock - A Dock For AmigaOS 3
**
**  SPDX-License-Identifier: MIT
**
**  ToolDock changes Copyright 2026 amigazen project
**
************************************/

#ifndef __BUTTON_COPY_H__
#define __BUTTON_COPY_H__

#include <exec/types.h>

BOOL button_path_is_drawer(STRPTR path);
BOOL button_copy_paths(STRPTR destPath, STRPTR *paths, UWORD count);

#endif
