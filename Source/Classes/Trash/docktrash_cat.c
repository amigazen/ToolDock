
/****************************************************************

   Catalog strings for DockTrash.class

****************************************************************/

#include <exec/memory.h>
#include <libraries/iffparse.h>

#include <proto/locale.h>

#include "docktrash_cat.h"

struct FC_String DockTrash_Strings[4] = {
    { (STRPTR) "A trashcan gadget that uses the TRASH: volume", 0 },
    { (STRPTR) "Trash", 1 },
    { (STRPTR) "TRASH: is not available. Start WBTrash first.", 2 },
    { (STRPTR) "Could not move the dropped item to the trash.", 3 }
};

VOID InitCatalog(struct Catalog *catalog)
{
    struct FC_String *fc;
    UWORD i;

    for (i = 0, fc = DockTrash_Strings;  i < 4;  i++, fc++) {
        fc->msg = GetCatalogStr(catalog, fc->id, (STRPTR) fc->msg);
    }
}
