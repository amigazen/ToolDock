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
#include "docktrash_cat.h"

#define CLASS_NAME      "DockTrash"
#define CLASS_VERSION   1
#define CLASS_REVISION  0
#define CLASS_VER_STR   "1.0 (" DS ")"
#define CLASS_COPYRIGHT "Copyright 2026 amigazen project. Based on DockBot by Andrew Kennan"
#define PRIVATE_DATA    TrashGadgetData
#define PARENT_CLASS    DB_ROOT_CLASS

#define GADGET_LIB_DATA     TrashLibData

#define GADGET_LIB_INIT     trash_lib_init
#define GADGET_LIB_EXPUNGE  trash_lib_expunge

#define METHOD_NEW          trash_new
#define METHOD_DISPOSE      trash_dispose

#define METHOD_TICK         trash_tick
#define METHOD_CLICK        trash_click
#define METHOD_DROP         trash_drop
#define METHOD_LAUNCHED     trash_launched

#define METHOD_GETSIZE      trash_get_size
#define METHOD_DRAW         trash_draw
#define METHOD_REMAP        trash_remap

#define METHOD_GETLABEL     trash_get_label

struct TrashLibData
{
    struct Library *dosBase;
    struct Library *iconBase;
    struct Library *wbBase;
};

struct TrashGadgetData
{
    struct DiskObject *diskObj;
    UWORD imageW;
    UWORD imageH;
    UWORD iconState;
    UWORD counter;
    BOOL devicePresent;
    BOOL hasContents;
};

#endif
