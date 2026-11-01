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
#include "dockexchange_cat.h"

#define CLASS_NAME      "DockExchange"
#define CLASS_VERSION   1
#define CLASS_REVISION  1
#define CLASS_VER_STR   "1.1 (" DS ")"
#define CLASS_COPYRIGHT "Copyright 2026 amigazen project. Based on DockBot by Andrew Kennan"
#define PRIVATE_DATA    ExchangeGadgetData
#define PARENT_CLASS    DB_ROOT_CLASS

#define GADGET_LIB_DATA     ExchangeLibData

#define GADGET_LIB_INIT     exchange_lib_init
#define GADGET_LIB_EXPUNGE  exchange_lib_expunge

#define METHOD_NEW          exchange_new
#define METHOD_DISPOSE      exchange_dispose

#define METHOD_TICK         exchange_tick
#define METHOD_CLICK        exchange_click
#define METHOD_POPUP        exchange_popup
#define METHOD_LAUNCHED     exchange_launched

#define METHOD_GETSIZE      exchange_get_size
#define METHOD_DRAW         exchange_draw
#define METHOD_REMAP        exchange_remap

#define METHOD_GETLABEL     exchange_get_label

struct ExchangeLibData
{
    struct Library *dosBase;
    struct Library *iconBase;
};

struct ExchangeGadgetData
{
    struct DiskObject *diskObj;
    STRPTR launchPath;
    UWORD imageW;
    UWORD imageH;
    UWORD iconState;
    UWORD counter;
};

#endif
