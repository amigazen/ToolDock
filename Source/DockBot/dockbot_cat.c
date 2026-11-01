
/****************************************************************

   This file was created automatically by `FlexCat 2.18'
   from "/Locale/Catalogs/DockBot.cd".

   Do NOT edit by hand!

****************************************************************/

#include <exec/memory.h>
#include <libraries/iffparse.h>

#include <proto/locale.h>

#include "dockbot.h"

#include "DockBot_cat.h"

struct FC_String DockBot_Strings[27] = {
    { (STRPTR) "OK", 0 },
    { (STRPTR) "Unable to open version %d of %s.", 1 },
    { (STRPTR) "Sorry, directories cannot be added to the dock.", 2 },
    { (STRPTR) "Project", 3 },
    { (STRPTR) "Settings", 4 },
    { (STRPTR) "About...", 5 },
    { (STRPTR) "?", 6 },
    { (STRPTR) "Help", 7 },
    { (STRPTR) "Iconify", 8 },
    { (STRPTR) "I", 9 },
    { (STRPTR) "Quit", 10 },
    { (STRPTR) "Q", 11 },
    { (STRPTR) "Edit", 12 },
    { (STRPTR) "E", 13 },
    { (STRPTR) "Save Settings", 14 },
    { (STRPTR) "W", 15 },
    { (STRPTR) "Revert Settings", 16 },
    { (STRPTR) "Z", 17 },
    { (STRPTR) "Open Preferences...", 18 },
    { (STRPTR) "S", 19 },
    { (STRPTR) "About ToolDock", 20 },
    { (STRPTR) "ToolDock Version %s\n\nA Dock For AmigaOS 3\n\nCopyright 2026 amigazen project\nBased on DockBot by Andrew Kennan", 21 },
    { (STRPTR) "A Dock For AmigaOS 3", 22 },
    { (STRPTR) "Remove", 23 },
    { (STRPTR) "Remove", 24 },
    { (STRPTR) "Are you sure you want to remove this from the dock?", 25 },
    { (STRPTR) "_Remove|_Cancel", 26 }
};

STATIC struct Catalog *DockBotCatalog = NULL;

VOID open_catalog(VOID)
{	
    struct FC_String *fc;
	UWORD i;

	DEBUG(DB_Printf(__FUNC__ ": ToolDock.catalog\n"));
	if( ! LocaleBase ) {
		DEBUG(DB_Printf(__FUNC__ ": Locale library not found.\n"));
		return;
	}

	if( !(DockBotCatalog = OpenCatalog(NULL, (STRPTR) "ToolDock.catalog",
			 			 		     OC_BuiltInLanguage, "english",
								     OC_Version, 1,
								     TAG_DONE))) {
		DEBUG(DB_Printf(__FUNC__ ": Unable to open catalog.\n"));
		return;
	}

    for (i = 0, fc = DockBot_Strings;  i < 27;  i++, fc++) {
	
		fc->msg = GetCatalogStr(DockBotCatalog, fc->id, (STRPTR) fc->msg);
    }
}

VOID close_catalog(VOID) 
{
	DEBUG(DB_Printf(__FUNC__ ": ToolDock.catalog\n"));
	if( ! DockBotCatalog ) {
		DEBUG(DB_Printf(__FUNC__ ": Catalog not open.\n"));
		return;
	}

	CloseCatalog(DockBotCatalog);
}

