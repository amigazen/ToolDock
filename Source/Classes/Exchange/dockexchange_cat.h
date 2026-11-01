
/****************************************************************

   Catalog strings for DockExchange.class

****************************************************************/

#ifndef DockExchange_CAT_H
#define DockExchange_CAT_H

#include <exec/types.h>
#include <libraries/locale.h>

VOID InitCatalog(struct Catalog* catalog);

struct FC_String {
    const UBYTE *msg;
    LONG id;
};

extern struct FC_String DockExchange_Strings[8];

#define MSG_Description (DockExchange_Strings[0].msg)
#define _MSG_Description (DockExchange_Strings+0)
#define MSG_Label (DockExchange_Strings[1].msg)
#define _MSG_Label (DockExchange_Strings+1)
#define MSG_POP_Show (DockExchange_Strings[2].msg)
#define _MSG_POP_Show (DockExchange_Strings+2)
#define MSG_POP_Hide (DockExchange_Strings[3].msg)
#define _MSG_POP_Hide (DockExchange_Strings+3)
#define MSG_POP_Quit (DockExchange_Strings[4].msg)
#define _MSG_POP_Quit (DockExchange_Strings+4)
#define MSG_POP_None (DockExchange_Strings[5].msg)
#define _MSG_POP_None (DockExchange_Strings+5)
#define MSG_ERR_NoExchange (DockExchange_Strings[6].msg)
#define _MSG_ERR_NoExchange (DockExchange_Strings+6)
#define MSG_POP_Remove (DockExchange_Strings[7].msg)
#define _MSG_POP_Remove (DockExchange_Strings+7)

#endif
