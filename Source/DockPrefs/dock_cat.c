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

#include <proto/exec.h>
#include <proto/locale.h>

#include "dockbot.h"
#include "dock_cat.h"

STATIC BOOL localeOpened;

#define DOCK_STRING_COUNT 60

struct FC_String Dock_Strings[DOCK_STRING_COUNT] = {
    { (STRPTR) "Dock", 0 },
    { (STRPTR) "Preferences editor for ToolDock", 1 },
    { (STRPTR) "OK", 2 },
    { (STRPTR) "Cancel", 3 },
    { (STRPTR) "Couldn't load class list.", 4 },
    { (STRPTR) "Couldn't load config.", 5 },
    { (STRPTR) "Couldn't open window.", 6 },
    { (STRPTR) "Unable to open version %d of %s.", 7 },
    { (STRPTR) "Unable to display editor dialogue.", 8 },
    { (STRPTR) "Sorry, directories cannot be added to the dock.", 9 },
    { (STRPTR) "Left", 10 },
    { (STRPTR) "Right", 11 },
    { (STRPTR) "Top", 12 },
    { (STRPTR) "Bottom", 13 },
    { (STRPTR) "Top/Left", 14 },
    { (STRPTR) "Center", 15 },
    { (STRPTR) "Bottom/Right", 16 },
    { (STRPTR) "New Dock Gadget", 17 },
    { (STRPTR) "Choose Gadget", 18 },
    { (STRPTR) "About", 19 },
    { (STRPTR) "Project", 20 },
    { (STRPTR) "?_About", 21 },
    { (STRPTR) "Q_Quit", 22 },
    { (STRPTR) "Dock", 23 },
    { (STRPTR) "Dock Settings", 24 },
    { (STRPTR) "Position", 25 },
    { (STRPTR) "Alignment", 26 },
    { (STRPTR) "Show Labels", 27 },
    { (STRPTR) "Show Borders", 28 },
    { (STRPTR) "Background Image", 29 },
    { (STRPTR) "Gadgets", 30 },
    { (STRPTR) "_New...", 31 },
    { (STRPTR) "_Edit...", 32 },
    { (STRPTR) "Delete", 33 },
    { (STRPTR) "_Up", 34 },
    { (STRPTR) "_Down", 35 },
    { (STRPTR) "Save", 36 },
    { (STRPTR) "Test", 37 },
    { (STRPTR) "Use", 38 },
    { (STRPTR) "Delete Gadget", 39 },
    { (STRPTR) "Are you sure you want to delete this gadget?", 40 },
    { (STRPTR) "_OK|Cancel", 41 },
    { (STRPTR) "Choose a background", 42 },
    { (STRPTR) "Padding", 43 },
    { (STRPTR) "Name", 44 },
    { (STRPTR) "Path", 45 },
    { (STRPTR) "Brush", 46 },
    { (STRPTR) "Arguments", 47 },
    { (STRPTR) "Key", 48 },
    { (STRPTR) "Start Type", 49 },
    { (STRPTR) "Console", 50 },
    { (STRPTR) "Format", 51 },
    { (STRPTR) "Number of Eyes", 52 },
    { (STRPTR) "Window Position", 53 },
    { (STRPTR) "Workbench", 54 },
    { (STRPTR) "Shell", 55 },
    { (STRPTR) "Dock Window", 56 },
    { (STRPTR) "Screen Centre", 57 },
    { (STRPTR) "Choose a program", 58 },
    { (STRPTR) "Choose a brush", 59 }
};

STATIC struct Catalog *DockCatalog = NULL;

VOID open_catalog(VOID)
{
    struct FC_String *fc;
    UWORD i;

    localeOpened = FALSE;

    if( ! LocaleBase ) {
        LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 38);
        if( LocaleBase ) {
            localeOpened = TRUE;
        } else {
            return;
        }
    }

    if( !(DockCatalog = OpenCatalog(NULL, (STRPTR)"Dock.catalog",
            OC_BuiltInLanguage, "english",
            OC_Version, 1,
            TAG_DONE)) ) {
        return;
    }

    for( i = 0, fc = Dock_Strings; i < DOCK_STRING_COUNT; i++, fc++ ) {
        fc->msg = GetCatalogStr(DockCatalog, fc->id, (STRPTR)fc->msg);
    }
}

VOID close_catalog(VOID)
{
    if( DockCatalog ) {
        CloseCatalog(DockCatalog);
        DockCatalog = NULL;
    }
    if( localeOpened && LocaleBase ) {
        CloseLibrary((struct Library *)LocaleBase);
        LocaleBase = NULL;
        localeOpened = FALSE;
    }
}
