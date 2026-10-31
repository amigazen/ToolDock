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

#include "prefs.h"

#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/button.h>
#include <proto/label.h>
#include <proto/chooser.h>
#include <proto/slider.h>
#include <proto/integer.h>
#include <proto/string.h>
#include <clib/reaction_lib_protos.h>

#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/chooser.h>
#include <gadgets/slider.h>
#include <gadgets/integer.h>
#include <intuition/gadgetclass.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>
#include <images/label.h>
#include <stdio.h>
#include <string.h>

STATIC STRPTR startNames[3];
STATIC STRPTR swPosNames[3];

STRPTR setting_get(struct EditSetting *set, UWORD count, STRPTR key)
{
    UWORD i;

    for( i = 0; i < count; i++ ) {
        if( strcmp(set[i].key, key) == 0 ) {
            return set[i].value;
        }
    }
    return NULL;
}

VOID setting_set(struct EditSetting *set, UWORD *count, STRPTR key, STRPTR value)
{
    UWORD i;
    UWORD n;

    if( key == NULL ) {
        return;
    }

    for( i = 0; i < *count; i++ ) {
        if( strcmp(set[i].key, key) == 0 ) {
            set[i].value[0] = '\0';
            if( value ) {
                strncpy(set[i].value, value, MAX_SETTING_VAL - 1);
                set[i].value[MAX_SETTING_VAL - 1] = '\0';
            }
            return;
        }
    }

    if( *count < MAX_EDIT_SETTINGS ) {
        n = *count;
        strncpy(set[n].key, key, MAX_SETTING_KEY - 1);
        set[n].key[MAX_SETTING_KEY - 1] = '\0';
        set[n].value[0] = '\0';
        if( value ) {
            strncpy(set[n].value, value, MAX_SETTING_VAL - 1);
            set[n].value[MAX_SETTING_VAL - 1] = '\0';
        }
        (*count)++;
    }
}

BOOL export_gadget_settings(Object *obj, struct EditSetting *set, UWORD *count)
{
    struct DockSettings *s;
    struct DockSettingValue v;
    UWORD n;
    UWORD copyLen;

    *count = 0;

    if( !(s = TD_OpenSettingsWrite(EDIT_TMP)) ) {
        return FALSE;
    }
    dock_gadget_write_settings(obj, s);
    TD_CloseSettings(s);

    if( !(s = TD_OpenSettingsRead(EDIT_TMP)) ) {
        return FALSE;
    }

    while( *count < MAX_EDIT_SETTINGS && TD_ReadSetting(s, &v) ) {
        n = *count;
        copyLen = v.KeyLength;
        if( copyLen >= MAX_SETTING_KEY ) {
            copyLen = MAX_SETTING_KEY - 1;
        }
        CopyMem(v.Key, set[n].key, copyLen);
        set[n].key[copyLen] = '\0';

        copyLen = v.ValueLength;
        if( copyLen >= MAX_SETTING_VAL ) {
            copyLen = MAX_SETTING_VAL - 1;
        }
        if( v.Value && copyLen ) {
            CopyMem(v.Value, set[n].value, copyLen);
        }
        set[n].value[copyLen] = '\0';
        (*count)++;
    }

    TD_CloseSettings(s);
    DeleteFile(EDIT_TMP);
    return TRUE;
}

BOOL import_gadget_settings(Object *obj, struct EditSetting *set, UWORD count)
{
    struct DockSettings *s;
    UWORD i;

    if( !(s = TD_OpenSettingsWrite(EDIT_TMP)) ) {
        return FALSE;
    }

    for( i = 0; i < count; i++ ) {
        if( set[i].key[0] ) {
            TD_WriteSetting(s, set[i].key, set[i].value);
        }
    }

    TD_CloseSettings(s);

    if( !(s = TD_OpenSettingsRead(EDIT_TMP)) ) {
        DeleteFile(EDIT_TMP);
        return FALSE;
    }

    dock_gadget_read_settings(obj, s);
    TD_CloseSettings(s);
    DeleteFile(EDIT_TMP);
    return TRUE;
}

STATIC STRPTR get_string_val(struct Gadget *gad)
{
    STRPTR text;

    text = NULL;
    if( gad ) {
        GetAttr(STRINGA_TextVal, gad, (ULONG *)&text);
    }
    return text;
}

STATIC VOID set_string_val(struct DockPrefs *prefs, ULONG id, STRPTR text)
{
    if( prefs->editGads[id] == NULL ) {
        return;
    }
    if( text == NULL ) {
        text = "";
    }
    if( prefs->editWin ) {
        SetGadgetAttrs(prefs->editGads[id], prefs->editWin, NULL,
            STRINGA_TextVal, text, TAG_END);
    } else {
        SetAttrs(prefs->editGads[id], STRINGA_TextVal, text, TAG_END);
    }
}

STATIC VOID set_edit_disabled(struct DockPrefs *prefs, ULONG id, BOOL disabled)
{
    if( prefs->editGads[id] == NULL ) {
        return;
    }
    if( prefs->editWin ) {
        if( SetGadgetAttrs(prefs->editGads[id], prefs->editWin, NULL,
                GA_Disabled, disabled, TAG_END) ) {
            RefreshGList(prefs->editGads[id], prefs->editWin, NULL, 1);
        }
    } else {
        SetAttrs(prefs->editGads[id], GA_Disabled, disabled, TAG_END);
    }
}

STATIC VOID pick_path(struct DockPrefs *prefs, ULONG destId, STRPTR title)
{
    STRPTR newPath;
    STRPTR current;

    current = get_string_val(prefs->editGads[destId]);
    if( newPath = TD_SelectFile(title, (STRPTR)MSG_OK, (STRPTR)MSG_Cancel, current) ) {
        set_string_val(prefs, destId, newPath);
        FREE_STRING(newPath);
    }
}

STATIC UWORD kind_from_name(STRPTR name)
{
    if( name == NULL ) {
        return EK_GENERIC;
    }
    if( strcmp(name, CLASS_BUTTON) == 0 ) {
        return EK_BUTTON;
    }
    if( strcmp(name, CLASS_CLOCK) == 0 ) {
        return EK_CLOCK;
    }
    if( strcmp(name, CLASS_EYES) == 0 ) {
        return EK_EYES;
    }
    if( strcmp(name, CLASS_SWITCHER) == 0 ) {
        return EK_SWITCHER;
    }
    return EK_GENERIC;
}

STATIC Object *build_button_layout(struct DockPrefs *prefs)
{
    STRPTR name;
    STRPTR path;
    STRPTR brush;
    STRPTR args;
    STRPTR key;
    STRPTR start;
    STRPTR con;
    ULONG startSel;
    Object *layout;

    name = setting_get(prefs->editSet, prefs->editCount, S_NAME);
    path = setting_get(prefs->editSet, prefs->editCount, S_PATH);
    brush = setting_get(prefs->editSet, prefs->editCount, S_BRUSH);
    args = setting_get(prefs->editSet, prefs->editCount, S_ARGS);
    key = setting_get(prefs->editSet, prefs->editCount, S_HOTKEY);
    start = setting_get(prefs->editSet, prefs->editCount, S_START);
    con = setting_get(prefs->editSet, prefs->editCount, S_CON);

    startSel = ST_WB;
    if( start && (start[0] == 's' || start[0] == 'S') ) {
        startSel = ST_SH;
    }

    startNames[0] = (STRPTR)MSG_ED_Workbench;
    startNames[1] = (STRPTR)MSG_ED_Shell;
    startNames[2] = NULL;
    if( prefs->startLabels ) {
        FreeChooserLabels(prefs->startLabels);
    }
    prefs->startLabels = ChooserLabelsA(startNames);

    layout = VLayoutObject,
        LAYOUT_SpaceOuter, TRUE,
        LAYOUT_AddChild, HLayoutObject,
            LAYOUT_AddChild, prefs->editGads[EID_NAME] = (struct Gadget *)StringObject,
                GA_ID, EID_NAME,
                GA_RelVerify, TRUE,
                GA_TabCycle, TRUE,
                STRINGA_TextVal, name ? name : "",
                STRINGA_MaxChars, 128,
            StringEnd,
            LAYOUT_AddChild, prefs->editGads[EID_PATH_BTN] = (struct Gadget *)ButtonObject,
                GA_ID, EID_PATH_BTN,
                GA_RelVerify, TRUE,
                GA_Text, "...",
            ButtonEnd,
            CHILD_WeightedWidth, 0,
        End,
        MemberLabel(MSG_ED_Name),
        LAYOUT_AddChild, HLayoutObject,
            LAYOUT_AddChild, prefs->editGads[EID_PATH] = (struct Gadget *)StringObject,
                GA_ID, EID_PATH,
                GA_RelVerify, TRUE,
                GA_TabCycle, TRUE,
                STRINGA_TextVal, path ? path : "",
                STRINGA_MaxChars, 256,
            StringEnd,
        End,
        MemberLabel(MSG_ED_Path),
        LAYOUT_AddChild, HLayoutObject,
            LAYOUT_AddChild, prefs->editGads[EID_BRUSH] = (struct Gadget *)StringObject,
                GA_ID, EID_BRUSH,
                GA_RelVerify, TRUE,
                GA_TabCycle, TRUE,
                STRINGA_TextVal, brush ? brush : "",
                STRINGA_MaxChars, 256,
            StringEnd,
            LAYOUT_AddChild, prefs->editGads[EID_BRUSH_BTN] = (struct Gadget *)ButtonObject,
                GA_ID, EID_BRUSH_BTN,
                GA_RelVerify, TRUE,
                GA_Text, "...",
            ButtonEnd,
            CHILD_WeightedWidth, 0,
        End,
        MemberLabel(MSG_ED_Brush),
        LAYOUT_AddChild, prefs->editGads[EID_ARGS] = (struct Gadget *)StringObject,
            GA_ID, EID_ARGS,
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            STRINGA_TextVal, args ? args : "",
            STRINGA_MaxChars, 256,
        StringEnd,
        MemberLabel(MSG_ED_Args),
        LAYOUT_AddChild, prefs->editGads[EID_HOTKEY] = (struct Gadget *)StringObject,
            GA_ID, EID_HOTKEY,
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            STRINGA_TextVal, key ? key : "",
            STRINGA_MaxChars, 64,
        StringEnd,
        MemberLabel(MSG_ED_Key),
        LAYOUT_AddChild, prefs->editGads[EID_START] = (struct Gadget *)ChooserObject,
            GA_ID, EID_START,
            GA_RelVerify, TRUE,
            CHOOSER_Labels, prefs->startLabels,
            CHOOSER_Selected, startSel,
        ChooserEnd,
        MemberLabel(MSG_ED_StartType),
        LAYOUT_AddChild, prefs->editGads[EID_CONSOLE] = (struct Gadget *)StringObject,
            GA_ID, EID_CONSOLE,
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            GA_Disabled, startSel == ST_WB,
            STRINGA_TextVal, con ? con : "",
            STRINGA_MaxChars, 128,
        StringEnd,
        MemberLabel(MSG_ED_Console),
    End;

    return layout;
}

STATIC Object *build_clock_layout(struct DockPrefs *prefs)
{
    STRPTR format;
    Object *layout;

    format = setting_get(prefs->editSet, prefs->editCount, S_FORMAT);
    if( format == NULL ) {
        format = "%Q:%M";
    }

    layout = VLayoutObject,
        LAYOUT_SpaceOuter, TRUE,
        LAYOUT_AddChild, prefs->editGads[EID_FORMAT] = (struct Gadget *)StringObject,
            GA_ID, EID_FORMAT,
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            STRINGA_TextVal, format,
            STRINGA_MaxChars, 64,
        StringEnd,
        MemberLabel(MSG_ED_Format),
    End;

    return layout;
}

STATIC Object *build_eyes_layout(struct DockPrefs *prefs)
{
    STRPTR num;
    ULONG eyes;
    Object *layout;

    num = setting_get(prefs->editSet, prefs->editCount, S_NUMEYES);
    eyes = MIN_EYES;
    if( num && num[0] >= '1' && num[0] <= '4' ) {
        eyes = (ULONG)(num[0] - '0');
    }

    layout = VLayoutObject,
        LAYOUT_SpaceOuter, TRUE,
        LAYOUT_AddChild, HLayoutObject,
            LAYOUT_AddChild, prefs->editGads[EID_NUMEYES] = (struct Gadget *)SliderObject,
                GA_ID, EID_NUMEYES,
                GA_RelVerify, TRUE,
                SLIDER_Min, MIN_EYES,
                SLIDER_Max, MAX_EYES,
                SLIDER_Level, eyes,
            SliderEnd,
            LAYOUT_AddChild, prefs->editGads[EID_NUMEYES_INT] = (struct Gadget *)IntegerObject,
                GA_ID, EID_NUMEYES_INT,
                GA_RelVerify, TRUE,
                INTEGER_Minimum, MIN_EYES,
                INTEGER_Maximum, MAX_EYES,
                INTEGER_Number, eyes,
                INTEGER_Arrows, FALSE,
            IntegerEnd,
            CHILD_WeightedWidth, 0,
        End,
        MemberLabel(MSG_ED_NumEyes),
    End;

    return layout;
}

STATIC Object *build_switcher_layout(struct DockPrefs *prefs)
{
    STRPTR key;
    STRPTR pos;
    ULONG posSel;
    Object *layout;

    key = setting_get(prefs->editSet, prefs->editCount, S_HOTKEY);
    pos = setting_get(prefs->editSet, prefs->editCount, S_SWPOS);

    posSel = 0;
    if( pos && (pos[0] == 'c' || pos[0] == 'C') ) {
        posSel = 1;
    }

    swPosNames[0] = (STRPTR)MSG_ED_DockWindow;
    swPosNames[1] = (STRPTR)MSG_ED_ScreenCentre;
    swPosNames[2] = NULL;
    if( prefs->swPosLabels ) {
        FreeChooserLabels(prefs->swPosLabels);
    }
    prefs->swPosLabels = ChooserLabelsA(swPosNames);

    layout = VLayoutObject,
        LAYOUT_SpaceOuter, TRUE,
        LAYOUT_AddChild, prefs->editGads[EID_WINPOS] = (struct Gadget *)ChooserObject,
            GA_ID, EID_WINPOS,
            GA_RelVerify, TRUE,
            CHOOSER_Labels, prefs->swPosLabels,
            CHOOSER_Selected, posSel,
        ChooserEnd,
        MemberLabel(MSG_ED_WinPos),
        LAYOUT_AddChild, prefs->editGads[EID_HOTKEY] = (struct Gadget *)StringObject,
            GA_ID, EID_HOTKEY,
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            STRINGA_TextVal, key ? key : "",
            STRINGA_MaxChars, 64,
        StringEnd,
        MemberLabel(MSG_ED_Key),
    End;

    return layout;
}

STATIC Object *build_generic_layout(struct DockPrefs *prefs)
{
    Object *layout;
    UWORD i;
    UWORD pos;
    UWORD room;

    prefs->editDump[0] = '\0';
    pos = 0;
    for( i = 0; i < prefs->editCount; i++ ) {
        room = sizeof(prefs->editDump) - pos;
        if( room < 8 ) {
            break;
        }
        pos += (UWORD)sprintf(prefs->editDump + pos, "%s=%s\n",
            prefs->editSet[i].key, prefs->editSet[i].value);
    }
    if( prefs->editDump[0] == '\0' ) {
        strncpy(prefs->editDump, (STRPTR)MSG_ERR_EditorFailure, sizeof(prefs->editDump) - 1);
    }

    layout = VLayoutObject,
        LAYOUT_SpaceOuter, TRUE,
        LAYOUT_AddChild, ButtonObject,
            GA_ReadOnly, TRUE,
            GA_Text, prefs->editDump,
        ButtonEnd,
    End;

    return layout;
}

STATIC VOID apply_edit_values(struct DockPrefs *prefs)
{
    ULONG sel;
    ULONG num;
    UBYTE buf[8];

    if( prefs->editKind == EK_BUTTON ) {
        setting_set(prefs->editSet, &prefs->editCount, S_NAME, get_string_val(prefs->editGads[EID_NAME]));
        setting_set(prefs->editSet, &prefs->editCount, S_PATH, get_string_val(prefs->editGads[EID_PATH]));
        setting_set(prefs->editSet, &prefs->editCount, S_BRUSH, get_string_val(prefs->editGads[EID_BRUSH]));
        setting_set(prefs->editSet, &prefs->editCount, S_ARGS, get_string_val(prefs->editGads[EID_ARGS]));
        setting_set(prefs->editSet, &prefs->editCount, S_HOTKEY, get_string_val(prefs->editGads[EID_HOTKEY]));
        setting_set(prefs->editSet, &prefs->editCount, S_CON, get_string_val(prefs->editGads[EID_CONSOLE]));
        sel = ST_WB;
        if( prefs->editGads[EID_START] ) {
            GetAttr(CHOOSER_Selected, prefs->editGads[EID_START], &sel);
        }
        setting_set(prefs->editSet, &prefs->editCount, S_START, sel == ST_SH ? "sh" : "wb");
    } else if( prefs->editKind == EK_CLOCK ) {
        setting_set(prefs->editSet, &prefs->editCount, S_FORMAT, get_string_val(prefs->editGads[EID_FORMAT]));
    } else if( prefs->editKind == EK_EYES ) {
        num = MIN_EYES;
        if( prefs->editGads[EID_NUMEYES] ) {
            GetAttr(SLIDER_Level, prefs->editGads[EID_NUMEYES], &num);
        }
        buf[0] = (UBYTE)('0' + (num & 0x7));
        buf[1] = '\0';
        setting_set(prefs->editSet, &prefs->editCount, S_NUMEYES, buf);
    } else if( prefs->editKind == EK_SWITCHER ) {
        setting_set(prefs->editSet, &prefs->editCount, S_HOTKEY, get_string_val(prefs->editGads[EID_HOTKEY]));
        sel = 0;
        if( prefs->editGads[EID_WINPOS] ) {
            GetAttr(CHOOSER_Selected, prefs->editGads[EID_WINPOS], &sel);
        }
        setting_set(prefs->editSet, &prefs->editCount, S_SWPOS, sel ? "centre" : "dock");
    }
}

VOID close_edit_dialog(struct DockPrefs *prefs, BOOL apply)
{
    if( apply && prefs->editGadget ) {
        apply_edit_values(prefs);
        import_gadget_settings(prefs->editGadget->dg, prefs->editSet, prefs->editCount);
        update_gadget_list(prefs);
    } else if( prefs->editGadgetIsNew && prefs->editGadget ) {
        remove_dock_gadget(prefs->editGadget);
        update_gadget_list(prefs);
    }

    if( prefs->editObj ) {
        DisposeObject(prefs->editObj);
        prefs->editObj = NULL;
        prefs->editWin = NULL;
    }

    prefs->editGadget = NULL;
    prefs->editGadgetIsNew = FALSE;
    prefs->editKind = EK_NONE;
    memset(prefs->editGads, 0, sizeof(prefs->editGads));
}

VOID edit_gadget(struct DockPrefs *prefs, struct DgNode *dg)
{
    Object *body;
    UWORD i;

    if( dg == NULL || prefs->editObj ) {
        return;
    }

    prefs->editGadgetIsNew = FALSE;
    prefs->editGadget = dg;
    prefs->editKind = kind_from_name(dg->n.ln_Name);
    prefs->editCount = 0;

    for( i = 0; i < (EID_GENERIC + MAX_EDIT_SETTINGS); i++ ) {
        prefs->editGads[i] = NULL;
    }

    if( ! export_gadget_settings(dg->dg, prefs->editSet, &prefs->editCount) ) {
        if( prefs->editKind == EK_GENERIC ) {
            TD_ShowError((STRPTR)MSG_ERR_EditorFailure);
            prefs->editGadget = NULL;
            return;
        }
    }

    body = NULL;
    if( prefs->editKind == EK_BUTTON ) {
        body = build_button_layout(prefs);
    } else if( prefs->editKind == EK_CLOCK ) {
        body = build_clock_layout(prefs);
    } else if( prefs->editKind == EK_EYES ) {
        body = build_eyes_layout(prefs);
    } else if( prefs->editKind == EK_SWITCHER ) {
        body = build_switcher_layout(prefs);
    } else {
        body = build_generic_layout(prefs);
    }

    if( body == NULL ) {
        TD_ShowError((STRPTR)MSG_ERR_EditorFailure);
        prefs->editGadget = NULL;
        return;
    }

    prefs->editObj = WindowObject,
        WA_Title, dg->n.ln_Name,
        WA_CloseGadget, TRUE,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_Activate, TRUE,
        WA_SizeGadget, TRUE,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WINDOW_SharedPort, prefs->idcmpPort,
        WINDOW_ParentGroup, VLayoutObject,
            LAYOUT_SpaceOuter, TRUE,
            LAYOUT_DeferLayout, TRUE,
            LAYOUT_AddChild, body,
            LAYOUT_AddChild, HLayoutObject,
                LAYOUT_EvenSize, TRUE,
                LAYOUT_AddChild, ButtonObject,
                    GA_ID, EID_OK,
                    GA_RelVerify, TRUE,
                    GA_Text, MSG_OK,
                ButtonEnd,
                LAYOUT_AddChild, ButtonObject,
                    GA_ID, EID_CANCEL,
                    GA_RelVerify, TRUE,
                    GA_Text, MSG_Cancel,
                ButtonEnd,
            End,
            CHILD_WeightedHeight, 0,
        End,
    EndWindow;

    if( prefs->editObj == NULL ) {
        DisposeObject(body);
        TD_ShowError((STRPTR)MSG_ERR_EditorFailure);
        prefs->editGadget = NULL;
        return;
    }

    prefs->editWin = RA_OpenWindow(prefs->editObj);
    if( prefs->editWin == NULL ) {
        close_edit_dialog(prefs, FALSE);
        prefs->editGadgetIsNew = FALSE;
        TD_ShowError((STRPTR)MSG_ERR_OpenWindow);
    }
}

VOID handle_edit_input(struct DockPrefs *prefs)
{
    ULONG result;
    UWORD code;
    ULONG level;

    if( prefs->editObj == NULL ) {
        return;
    }

    while( (result = RA_HandleInput(prefs->editObj, &code)) != WMHI_LASTMSG ) {
        switch( result & WMHI_CLASSMASK ) {
            case WMHI_CLOSEWINDOW:
                close_edit_dialog(prefs, FALSE);
                return;

            case WMHI_GADGETUP:
                switch( result & WMHI_GADGETMASK ) {
                    case EID_OK:
                        close_edit_dialog(prefs, TRUE);
                        return;
                    case EID_CANCEL:
                        close_edit_dialog(prefs, FALSE);
                        return;
                    case EID_PATH_BTN:
                        pick_path(prefs, EID_PATH, (STRPTR)MSG_FR_ChooseTarget);
                        break;
                    case EID_BRUSH_BTN:
                        pick_path(prefs, EID_BRUSH, (STRPTR)MSG_FR_ChooseBrush);
                        break;
                    case EID_START:
                        level = ST_WB;
                        if( prefs->editGads[EID_START] ) {
                            GetAttr(CHOOSER_Selected, prefs->editGads[EID_START], &level);
                        }
                        set_edit_disabled(prefs, EID_CONSOLE, level == ST_WB);
                        break;
                    case EID_NUMEYES:
                        if( prefs->editGads[EID_NUMEYES] && prefs->editGads[EID_NUMEYES_INT] ) {
                            GetAttr(SLIDER_Level, prefs->editGads[EID_NUMEYES], &level);
                            SetGadgetAttrs(prefs->editGads[EID_NUMEYES_INT], prefs->editWin, NULL,
                                INTEGER_Number, level, TAG_END);
                        }
                        break;
                    case EID_NUMEYES_INT:
                        if( prefs->editGads[EID_NUMEYES] && prefs->editGads[EID_NUMEYES_INT] ) {
                            GetAttr(INTEGER_Number, prefs->editGads[EID_NUMEYES_INT], &level);
                            SetGadgetAttrs(prefs->editGads[EID_NUMEYES], prefs->editWin, NULL,
                                SLIDER_Level, level, TAG_END);
                        }
                        break;
                }
                break;
        }
    }
}
