/************************************
**
**  ToolDock - A Dock For AmigaOS 3
**
**  SPDX-License-Identifier: MIT
**
**  ToolDock changes Copyright 2026 amigazen project
**
************************************/

#ifndef __CLASS_DEF_H__
#define __CLASS_DEF_H__

#include "gadget_class.h"
#include "dockspacer_cat.h"

#define CLASS_NAME      "DockSpacer"
#define CLASS_VERSION   1
#define CLASS_REVISION  0
#define CLASS_VER_STR   "1.0 (" DS ")"
#define CLASS_COPYRIGHT "Copyright 2026 amigazen project"
#define PRIVATE_DATA    SpacerGadgetData

#define GADGET_LIB_DATA     SpacerLibData

#define GADGET_LIB_INIT     spacer_lib_init
#define GADGET_LIB_EXPUNGE  spacer_lib_expunge

#undef METHOD_NEW
#undef METHOD_DISPOSE

#define METHOD_GETSIZE      spacer_get_size
#define METHOD_DRAW         spacer_draw
#define METHOD_GETLABEL     spacer_get_label

#undef METHOD_GETHOTKEY
#undef METHOD_READCONFIG
#undef METHOD_WRITECONFIG

struct SpacerLibData
{
    struct Library *graphicsLib;
};

struct SpacerGadgetData
{
    UBYTE pad;
};

#endif
