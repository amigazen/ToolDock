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

#ifndef __CLASS_DEF_H__
#define __CLASS_DEF_H__

#include "dockbot.h"
#include "dockbot_protos.h"
#include "dockbot_pragmas.h"

#include "gadget_class.h"
#include "dockopen_cat.h"

#define CLASS_NAME      "DockOpen"
#define CLASS_VERSION   1
#define CLASS_REVISION  0
#define CLASS_VER_STR   "1.0 (" DS ")"
#define CLASS_COPYRIGHT "Copyright 2026 amigazen project. Based on DockBot by Andrew Kennan"
#define PRIVATE_DATA    OpenGadgetData
#define PARENT_CLASS    DB_ROOT_CLASS

#define GADGET_LIB_DATA     OpenLibData

#define GADGET_LIB_INIT     open_lib_init
#define GADGET_LIB_EXPUNGE  open_lib_expunge

#define METHOD_NEW          open_new
#define METHOD_DISPOSE      open_dispose

#define METHOD_TICK         open_tick
#define METHOD_CLICK        open_click
#define METHOD_DROP         open_drop
#define METHOD_LAUNCHED     open_launched

#define METHOD_GETSIZE      open_get_size
#define METHOD_DRAW         open_draw
#define METHOD_REMAP        open_remap

#define METHOD_READCONFIG   open_read_config
#define METHOD_WRITECONFIG  open_write_config
#define METHOD_GETLABEL     open_get_label

struct OpenLibData
{
    struct Library *dosBase;
    struct Library *iconBase;
    struct Library *wbBase;
    struct Library *dtBase;
};

struct OpenGadgetData
{
    struct DiskObject *diskObj;
    STRPTR name;
    STRPTR defaultTool;
    UWORD imageW;
    UWORD imageH;
    UWORD iconState;
    UWORD counter;
};

#endif
