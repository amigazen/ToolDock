
/****************************************************************

   Catalog strings for DockTrash.class

****************************************************************/

#ifndef DockTrash_CAT_H
#define DockTrash_CAT_H

#include <exec/types.h>
#include <libraries/locale.h>

VOID InitCatalog(struct Catalog* catalog);

struct FC_String {
    const UBYTE *msg;
    LONG id;
};

extern struct FC_String DockTrash_Strings[4];

#define MSG_Description (DockTrash_Strings[0].msg)
#define _MSG_Description (DockTrash_Strings+0)
#define MSG_Label (DockTrash_Strings[1].msg)
#define _MSG_Label (DockTrash_Strings+1)
#define MSG_ERR_NoTrash (DockTrash_Strings[2].msg)
#define _MSG_ERR_NoTrash (DockTrash_Strings+2)
#define MSG_ERR_MoveFailed (DockTrash_Strings[3].msg)
#define _MSG_ERR_MoveFailed (DockTrash_Strings+3)

#endif
