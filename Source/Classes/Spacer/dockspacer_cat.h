
/****************************************************************

   DockSpacer catalog strings.

****************************************************************/

#ifndef DockSpacer_CAT_H
#define DockSpacer_CAT_H

#include <exec/types.h>
#include <libraries/locale.h>

VOID InitCatalog(struct Catalog* catalog);

struct FC_String {
    const UBYTE *msg;
    LONG id;
};

extern struct FC_String DockSpacer_Strings[2];

#define MSG_Description (DockSpacer_Strings[0].msg)
#define _MSG_Description (DockSpacer_Strings+0)
#define MSG_Label (DockSpacer_Strings[1].msg)
#define _MSG_Label (DockSpacer_Strings+1)

#endif
