
/****************************************************************

   Catalog strings for DockOpen.class

****************************************************************/

#include <exec/memory.h>
#include <libraries/iffparse.h>

#include <proto/locale.h>

#include "dockopen_cat.h"

struct FC_String DockOpen_Strings[6] = {
    { (STRPTR) "A drop box that opens files with the right tool", 0 },
    { (STRPTR) "Open", 1 },
    { (STRPTR) "Could not open the dropped item.", 2 },
    { (STRPTR) "Choose a file to open", 3 },
    { (STRPTR) "Open", 4 },
    { (STRPTR) "Cancel", 5 }
};

VOID InitCatalog(struct Catalog *catalog)
{
    struct FC_String *fc;
    UWORD i;

    for (i = 0, fc = DockOpen_Strings;  i < 6;  i++, fc++) {
        fc->msg = GetCatalogStr(catalog, fc->id, (STRPTR) fc->msg);
    }
}
