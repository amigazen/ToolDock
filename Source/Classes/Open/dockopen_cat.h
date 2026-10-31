
/****************************************************************

   Catalog strings for DockOpen.class

****************************************************************/

#ifndef DockOpen_CAT_H
#define DockOpen_CAT_H

#include <exec/types.h>
#include <libraries/locale.h>

VOID InitCatalog(struct Catalog* catalog);

struct FC_String {
    const UBYTE *msg;
    LONG id;
};

extern struct FC_String DockOpen_Strings[6];

#define MSG_Description (DockOpen_Strings[0].msg)
#define MSG_Label (DockOpen_Strings[1].msg)
#define MSG_ERR_OpenFailed (DockOpen_Strings[2].msg)
#define MSG_FR_Choose (DockOpen_Strings[3].msg)
#define MSG_FR_Ok (DockOpen_Strings[4].msg)
#define MSG_FR_Cancel (DockOpen_Strings[5].msg)

#endif
