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

#include "prefs.h"

#include <stdio.h>

#include <graphics/gfxbase.h>
#include <intuition/intuitionbase.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/utility.h>
#include <proto/icon.h>
#include <proto/locale.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/button.h>
#include <proto/label.h>
#include <proto/chooser.h>
#include <proto/checkbox.h>
#include <proto/slider.h>
#include <proto/integer.h>
#include <proto/string.h>
#include <proto/listbrowser.h>

VOID show_open_lib_err(STRPTR name, UWORD version)
{
    STRPTR msg;
    UWORD l;

    l = strlen(name) + strlen(MSG_ERR_OpenLibrary);
    if( msg = TD_AllocMem(l, MEMF_ANY) ) {
        sprintf(msg, MSG_ERR_OpenLibrary, version, name);
        DEBUG(printf("%s\n", msg));
        TD_ShowError(msg);
        TD_FreeMem(msg, l);
    }
}

struct Library *open_lib(STRPTR name, UWORD version)
{
    struct Library *lib;

    if( !(lib = OpenLibrary(name, version)) ) {
        show_open_lib_err(name, version);
    }
    return lib;
}

VOID close_lib(STRPTR name, struct Library *lib)
{
    if( lib ) {
        DEBUG(printf("close_lib: %s\n", name));
        CloseLibrary(lib);
    }
}

BOOL open_reaction_classes(VOID)
{
    if( !(IntuitionBase = (struct IntuitionBase *)open_lib("intuition.library", 40)) ) {
        return FALSE;
    }
    if( !(GfxBase = (struct GfxBase *)open_lib("graphics.library", 39)) ) {
        return FALSE;
    }
    if( !(GadToolsBase = open_lib("gadtools.library", 37)) ) {
        return FALSE;
    }
    if( !(UtilityBase = open_lib("utility.library", 37)) ) {
        return FALSE;
    }
    if( !(IconBase = open_lib("icon.library", 37)) ) {
        return FALSE;
    }
    if( !(WindowBase = open_lib("window.class", 44)) ) {
        return FALSE;
    }
    if( !(LayoutBase = open_lib("layout.gadget", 44)) ) {
        return FALSE;
    }
    if( !(ButtonBase = open_lib("button.gadget", 44)) ) {
        return FALSE;
    }
    if( !(LabelBase = open_lib("label.image", 44)) ) {
        return FALSE;
    }
    if( !(ChooserBase = open_lib("chooser.gadget", 44)) ) {
        return FALSE;
    }
    if( !(CheckBoxBase = open_lib("checkbox.gadget", 44)) ) {
        return FALSE;
    }
    if( !(SliderBase = open_lib("slider.gadget", 44)) ) {
        return FALSE;
    }
    if( !(IntegerBase = open_lib("integer.gadget", 44)) ) {
        return FALSE;
    }
    if( !(StringBase = open_lib("string.gadget", 44)) ) {
        return FALSE;
    }
    if( !(ListBrowserBase = open_lib("listbrowser.gadget", 44)) ) {
        return FALSE;
    }
    return TRUE;
}

VOID close_reaction_classes(VOID)
{
    close_lib("listbrowser.gadget", ListBrowserBase);
    close_lib("string.gadget", StringBase);
    close_lib("integer.gadget", IntegerBase);
    close_lib("slider.gadget", SliderBase);
    close_lib("checkbox.gadget", CheckBoxBase);
    close_lib("chooser.gadget", ChooserBase);
    close_lib("label.image", LabelBase);
    close_lib("button.gadget", ButtonBase);
    close_lib("layout.gadget", LayoutBase);
    close_lib("window.class", WindowBase);
    close_lib("icon.library", IconBase);
    close_lib("utility.library", UtilityBase);
    close_lib("gadtools.library", GadToolsBase);
    close_lib("graphics.library", (struct Library *)GfxBase);
    close_lib("intuition.library", (struct Library *)IntuitionBase);

    ListBrowserBase = NULL;
    StringBase = NULL;
    IntegerBase = NULL;
    SliderBase = NULL;
    CheckBoxBase = NULL;
    ChooserBase = NULL;
    LabelBase = NULL;
    ButtonBase = NULL;
    LayoutBase = NULL;
    WindowBase = NULL;
    IconBase = NULL;
    UtilityBase = NULL;
    GadToolsBase = NULL;
    GfxBase = NULL;
    IntuitionBase = NULL;
}
