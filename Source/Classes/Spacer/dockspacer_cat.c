
/****************************************************************

   DockSpacer catalog strings.

****************************************************************/

#include <exec/memory.h>
#include <libraries/iffparse.h>

#include <proto/locale.h>

#include "dockspacer_cat.h"

struct FC_String DockSpacer_Strings[2] = {
    { (STRPTR) "A separator line for ToolDock", 0 },
    { (STRPTR) "Spacer", 1 }
};

VOID InitCatalog(struct Catalog *catalog)
{
    struct FC_String *fc;
    UWORD i;

    for( i = 0, fc = DockSpacer_Strings; i < 2; i++, fc++ ) {
        fc->msg = GetCatalogStr(catalog, fc->id, (STRPTR)fc->msg);
    }
}
