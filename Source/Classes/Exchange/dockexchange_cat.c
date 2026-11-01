
/****************************************************************

   Catalog strings for DockExchange.class

****************************************************************/

#include <exec/memory.h>
#include <libraries/iffparse.h>

#include <proto/locale.h>

#include "dockexchange_cat.h"

struct FC_String DockExchange_Strings[8] = {
    { (STRPTR) "A commodities Exchange gadget", 0 },
    { (STRPTR) "Exchange", 1 },
    { (STRPTR) "Show", 2 },
    { (STRPTR) "Hide", 3 },
    { (STRPTR) "Quit", 4 },
    { (STRPTR) "None", 5 },
    { (STRPTR) "Could not find SYS:Utilities/Exchange.", 6 },
    { (STRPTR) "Remove", 7 }
};

VOID InitCatalog(struct Catalog *catalog)
{
    struct FC_String *fc;
    UWORD i;

    for (i = 0, fc = DockExchange_Strings;  i < 8;  i++, fc++) {
        fc->msg = GetCatalogStr(catalog, fc->id, (STRPTR) fc->msg);
    }
}
