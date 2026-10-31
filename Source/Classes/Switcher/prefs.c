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

#include "class_def.h"

#include <clib/utility_protos.h>

#include <libraries/triton.h>
#include <proto/triton.h>
#include "triton_support.h"

#define S_POS       "position"
#define S_HOTKEY    "key"

struct Values PosValues[] = {
    { "dock",   POS_DOCK   },
    { "centre", POS_CENTRE },
    { NULL, 0 }
};

STRPTR positions[] = { NULL /*"Dock Window"*/, NULL /*"Screen Centre"*/, NULL };

enum {
    OBJ_CYC_POS = 1001,
    OBJ_STR_HOTKEY
};

STRPTR get_pos_type(struct Values *values, UWORD val) {

    while( values->Name ) {
        if( values->Value == val ) {
            return values->Name;
        }
        values++;
    }
    return NULL;
}


DB_METHOD_DM(GETEDITOR,DockMessageGetEditor)

    DEBUG(TD_Printf(__METHOD__ "\n"));

    positions[0] = (STRPTR)MSG_POS_Dock;
    positions[1] = (STRPTR)MSG_POS_Screen;

    msg->uiTags = TD_MakeTagList(   
        VertGroupA,
            Space,
            LineArray,
                Space,
                BeginLine,
                    Space,
                    TextN(MSG_UI_Position),
                    Space,
                    CycleGadget(positions, data->position, OBJ_CYC_POS),
                    Space,
                EndLine,
                Space,
                BeginLine,
                    Space,
                    TextN(MSG_UI_Key),
                    Space,
                    StringGadget(data->hotkey, OBJ_STR_HOTKEY),
                    Space,
                EndLine,
                Space,
            EndArray,
        EndGroup,
        TAG_END);

    return 1;
}

DB_METHOD_DM(EDITORUPDATE, DockMessageEditorUpdate)

    STRPTR str;
    UWORD len;
    struct TR_Project *proj = msg->window;

    DEBUG(TD_Printf(__METHOD__ "\n"));

    FREE_STRING(data->hotkey)

    str = (STRPTR)TR_GetAttribute(proj, OBJ_STR_HOTKEY, 0);
    if( str && (len = strlen(str)) ) {        
        data->hotkey = (STRPTR)TD_AllocMem(len + 1, MEMF_ANY);
        CopyMem(str, data->hotkey, len + 1);
    }

    data->position = (UWORD)TR_GetAttribute(proj, OBJ_CYC_POS, TRAT_Value);

    return 1;
}

DB_METHOD_M(CANEDIT, DockMessageCanEdit)

    DEBUG(TD_Printf(__METHOD__ "\n"));

    msg->canEdit = TRUE;

    return 1;
}

DB_METHOD_DM(READCONFIG,DockMessageConfig)

    struct DockSettingValue v;
    struct Values *vals;
    UWORD len;

    while( TD_ReadSetting(msg->settings, &v) ) {
        
        if( IS_KEY(S_POS, v) ) {
            GET_VALUE(v, PosValues, vals, len, data->position)
        }
        else if( IS_KEY(S_HOTKEY, v) ) {
            GET_STRING(v, data->hotkey)
        }
    }

    DEBUG(TD_Printf(__METHOD__ "pos = %ld, hotkey = %s\n", data->position, data->hotkey));

    return 1;
}

DB_METHOD_DM(WRITECONFIG,DockMessageConfig)

    struct DockSettings *s = msg->settings;

    if( data->hotkey ) {
        TD_WriteSetting(s, S_HOTKEY, data->hotkey);
    }

    TD_WriteSetting(s, S_POS, get_pos_type(PosValues, data->position));

    return 1;
}

