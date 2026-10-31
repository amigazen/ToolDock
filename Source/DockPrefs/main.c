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
#include "td_request.h"

#include <dos/dos.h>
#include <dos/rdargs.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>

#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/icon.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/button.h>
#include <proto/label.h>
#include <proto/chooser.h>
#include <proto/checkbox.h>
#include <proto/slider.h>
#include <proto/integer.h>
#include <proto/string.h>
#include <proto/listbrowser.h>
#include <clib/reaction_lib_protos.h>

#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/chooser.h>
#include <gadgets/checkbox.h>
#include <images/label.h>
#include <gadgets/slider.h>
#include <gadgets/integer.h>
#include <gadgets/listbrowser.h>
#include <intuition/gadgetclass.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include <stdio.h>
#include <string.h>

#ifndef HOOKFUNC
typedef ULONG (*HOOKFUNC)();
#endif

const char __ver[40] = "$VER:" APP_NAME " " VERSION_STR;

struct Library *ToolDockBase;

STATIC STRPTR positionNames[5];
STATIC STRPTR alignmentNames[4];

#ifdef DEBUG_BUILD

BOOL __DebugEnabled = FALSE;

#define TEMPLATE "DEBUG/S"
#define OPT_DEBUG 0

STATIC VOID parse_args(VOID)
{
    struct RDArgs *rd;
    LONG values[1];

    values[0] = FALSE;

    if( rd = ReadArgs(TEMPLATE, values, NULL) ) {
        __DebugEnabled = (BOOL)values[OPT_DEBUG];
        FreeArgs(rd);
    }
}

#endif

STATIC VOID update_background_path(struct DockPrefs *prefs, STRPTR newPath)
{
    UWORD l;

    FREE_STRING(prefs->cfg.bgBrushPath);

    if( newPath == NULL ) {
        return;
    }

    l = strlen(newPath) + 1;
    if( prefs->cfg.bgBrushPath = (STRPTR)TD_AllocMem(l, MEMF_CLEAR) ) {
        CopyMem(newPath, prefs->cfg.bgBrushPath, l);
    }
}

STATIC VOID select_background(struct DockPrefs *prefs)
{
    STRPTR newPath;

    if( newPath = TD_SelectFile((STRPTR)MSG_FR_ChooseBackground,
            (STRPTR)MSG_OK, (STRPTR)MSG_Cancel, prefs->cfg.bgBrushPath) ) {
        update_background_path(prefs, newPath);
        if( prefs->gads[GID_BG_BRUSH] && prefs->mainWin ) {
            SetGadgetAttrs(prefs->gads[GID_BG_BRUSH], prefs->mainWin, NULL,
                STRINGA_TextVal, prefs->cfg.bgBrushPath ? prefs->cfg.bgBrushPath : "",
                TAG_END);
        }
        FREE_STRING(newPath);
    }
}

STATIC VOID __asm __saveds AppMsgFunc(
    register __a0 struct Hook *hook,
    register __a2 Object *window,
    register __a1 struct AppMessage *msg)
{
    struct DockPrefs *prefs;

    prefs = (struct DockPrefs *)hook->h_Data;
    window = window;
    add_dropped_icon(prefs, msg);
}

STATIC VOID fill_menus(struct DockPrefs *prefs)
{
    prefs->newMenu[0].nm_Type = NM_TITLE;
    prefs->newMenu[0].nm_Label = (STRPTR)MSG_MENU_Project;
    prefs->newMenu[0].nm_CommKey = NULL;
    prefs->newMenu[0].nm_Flags = 0;
    prefs->newMenu[0].nm_MutualExclude = 0;
    prefs->newMenu[0].nm_UserData = NULL;

    prefs->newMenu[1].nm_Type = NM_ITEM;
    prefs->newMenu[1].nm_Label = (STRPTR)MSG_MENU_About;
    prefs->newMenu[1].nm_CommKey = NULL;
    prefs->newMenu[1].nm_Flags = 0;
    prefs->newMenu[1].nm_MutualExclude = 0;
    prefs->newMenu[1].nm_UserData = (APTR)MID_ABOUT;

    prefs->newMenu[2].nm_Type = NM_ITEM;
    prefs->newMenu[2].nm_Label = NM_BARLABEL;
    prefs->newMenu[2].nm_CommKey = NULL;
    prefs->newMenu[2].nm_Flags = 0;
    prefs->newMenu[2].nm_MutualExclude = 0;
    prefs->newMenu[2].nm_UserData = NULL;

    prefs->newMenu[3].nm_Type = NM_ITEM;
    prefs->newMenu[3].nm_Label = (STRPTR)MSG_MENU_Quit;
    prefs->newMenu[3].nm_CommKey = NULL;
    prefs->newMenu[3].nm_Flags = 0;
    prefs->newMenu[3].nm_MutualExclude = 0;
    prefs->newMenu[3].nm_UserData = (APTR)MID_QUIT;

    prefs->newMenu[4].nm_Type = NM_END;
    prefs->newMenu[4].nm_Label = NULL;
    prefs->newMenu[4].nm_CommKey = NULL;
    prefs->newMenu[4].nm_Flags = 0;
    prefs->newMenu[4].nm_MutualExclude = 0;
    prefs->newMenu[4].nm_UserData = NULL;
}

STATIC VOID apply_config_to_gui(struct DockPrefs *prefs)
{
    if( prefs->gads[GID_POSITION] ) {
        SetAttrs(prefs->gads[GID_POSITION], CHOOSER_Selected, prefs->cfg.pos, TAG_END);
    }
    if( prefs->gads[GID_ALIGNMENT] ) {
        SetAttrs(prefs->gads[GID_ALIGNMENT], CHOOSER_Selected, prefs->cfg.align, TAG_END);
    }
    if( prefs->gads[GID_SHOW_LABELS] ) {
        SetAttrs(prefs->gads[GID_SHOW_LABELS], GA_Selected, prefs->cfg.showGadgetLabels, TAG_END);
    }
    if( prefs->gads[GID_SHOW_BORDERS] ) {
        SetAttrs(prefs->gads[GID_SHOW_BORDERS], GA_Selected, prefs->cfg.showGadgetBorders, TAG_END);
    }
    if( prefs->gads[GID_PADDING] ) {
        SetAttrs(prefs->gads[GID_PADDING], SLIDER_Level, prefs->cfg.gadgetPadding, TAG_END);
    }
    if( prefs->gads[GID_PADDING_INT] ) {
        SetAttrs(prefs->gads[GID_PADDING_INT], INTEGER_Number, prefs->cfg.gadgetPadding, TAG_END);
    }
    if( prefs->gads[GID_BG_BRUSH] ) {
        SetAttrs(prefs->gads[GID_BG_BRUSH],
            STRINGA_TextVal, prefs->cfg.bgBrushPath ? prefs->cfg.bgBrushPath : "",
            TAG_END);
    }
}

STATIC BOOL open_main_window(struct DockPrefs *prefs)
{
    ULONG i;

    for( i = 0; i < GID_LAST; i++ ) {
        prefs->gads[i] = NULL;
    }

    positionNames[0] = (STRPTR)MSG_POS_Left;
    positionNames[1] = (STRPTR)MSG_POS_Right;
    positionNames[2] = (STRPTR)MSG_POS_Top;
    positionNames[3] = (STRPTR)MSG_POS_Bottom;
    positionNames[4] = NULL;

    alignmentNames[0] = (STRPTR)MSG_ALIGN_TopLeft;
    alignmentNames[1] = (STRPTR)MSG_ALIGN_Center;
    alignmentNames[2] = (STRPTR)MSG_ALIGN_BottomRight;
    alignmentNames[3] = NULL;

    prefs->posLabels = ChooserLabelsA(positionNames);
    prefs->alignLabels = ChooserLabelsA(alignmentNames);
    fill_menus(prefs);

    prefs->appHook.h_Entry = (HOOKFUNC)AppMsgFunc;
    prefs->appHook.h_SubEntry = NULL;
    prefs->appHook.h_Data = prefs;

    prefs->mainObj = WindowObject,
        WA_Title, MSG_MW_Title,
        WA_CloseGadget, TRUE,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_Activate, TRUE,
        WA_SizeGadget, TRUE,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_MENUPICK | IDCMP_GADGETUP,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WINDOW_SharedPort, prefs->idcmpPort,
        WINDOW_AppPort, prefs->appPort,
        WINDOW_AppWindow, TRUE,
        WINDOW_AppMsgHook, &prefs->appHook,
        WINDOW_IconifyGadget, TRUE,
        WINDOW_IconTitle, APP_NAME,
        WINDOW_NewMenu, prefs->newMenu,
        WINDOW_ParentGroup, VLayoutObject,
            LAYOUT_SpaceOuter, TRUE,
            LAYOUT_DeferLayout, TRUE,
            LAYOUT_BevelStyle, GroupFrame,
            LAYOUT_Label, MSG_MW_DockSettings,
            LAYOUT_AddChild, prefs->gads[GID_POSITION] = (struct Gadget *)ChooserObject,
                GA_ID, GID_POSITION,
                GA_RelVerify, TRUE,
                CHOOSER_Labels, prefs->posLabels,
                CHOOSER_Selected, prefs->cfg.pos,
            ChooserEnd,
            MemberLabel(MSG_MW_Position),
            LAYOUT_AddChild, prefs->gads[GID_ALIGNMENT] = (struct Gadget *)ChooserObject,
                GA_ID, GID_ALIGNMENT,
                GA_RelVerify, TRUE,
                CHOOSER_Labels, prefs->alignLabels,
                CHOOSER_Selected, prefs->cfg.align,
            ChooserEnd,
            MemberLabel(MSG_MW_Alignment),
            LAYOUT_AddChild, prefs->gads[GID_SHOW_LABELS] = (struct Gadget *)CheckBoxObject,
                GA_ID, GID_SHOW_LABELS,
                GA_RelVerify, TRUE,
                GA_Selected, prefs->cfg.showGadgetLabels,
            CheckBoxEnd,
            MemberLabel(MSG_MW_ShowLabels),
            LAYOUT_AddChild, prefs->gads[GID_SHOW_BORDERS] = (struct Gadget *)CheckBoxObject,
                GA_ID, GID_SHOW_BORDERS,
                GA_RelVerify, TRUE,
                GA_Selected, prefs->cfg.showGadgetBorders,
            CheckBoxEnd,
            MemberLabel(MSG_MW_ShowBorders),
            LAYOUT_AddChild, HLayoutObject,
                LAYOUT_AddChild, prefs->gads[GID_PADDING] = (struct Gadget *)SliderObject,
                    GA_ID, GID_PADDING,
                    GA_RelVerify, TRUE,
                    SLIDER_Min, MIN_PADDING,
                    SLIDER_Max, MAX_PADDING,
                    SLIDER_Level, prefs->cfg.gadgetPadding,
                SliderEnd,
                LAYOUT_AddChild, prefs->gads[GID_PADDING_INT] = (struct Gadget *)IntegerObject,
                    GA_ID, GID_PADDING_INT,
                    GA_RelVerify, TRUE,
                    INTEGER_Minimum, MIN_PADDING,
                    INTEGER_Maximum, MAX_PADDING,
                    INTEGER_Number, prefs->cfg.gadgetPadding,
                    INTEGER_Arrows, FALSE,
                IntegerEnd,
                CHILD_WeightedWidth, 0,
            End,
            MemberLabel(MSG_MW_Padding),
            LAYOUT_AddChild, HLayoutObject,
                LAYOUT_AddChild, prefs->gads[GID_BG_BRUSH] = (struct Gadget *)StringObject,
                    GA_ID, GID_BG_BRUSH,
                    GA_RelVerify, TRUE,
                    GA_TabCycle, TRUE,
                    STRINGA_TextVal, prefs->cfg.bgBrushPath ? prefs->cfg.bgBrushPath : "",
                    STRINGA_MaxChars, 256,
                StringEnd,
                LAYOUT_AddChild, prefs->gads[GID_BTN_BG_BRUSH] = (struct Gadget *)ButtonObject,
                    GA_ID, GID_BTN_BG_BRUSH,
                    GA_RelVerify, TRUE,
                    GA_Text, "...",
                ButtonEnd,
                CHILD_WeightedWidth, 0,
            End,
            MemberLabel(MSG_MW_Background),
            LAYOUT_AddChild, HLayoutObject,
                LAYOUT_AddChild, prefs->gads[GID_GADGETS] = (struct Gadget *)ListBrowserObject,
                    GA_ID, GID_GADGETS,
                    GA_RelVerify, TRUE,
                    LISTBROWSER_ShowSelected, TRUE,
                    LISTBROWSER_Labels, NULL,
                ListBrowserEnd,
                LAYOUT_AddChild, VLayoutObject,
                    LAYOUT_AddChild, prefs->gads[GID_BTN_NEW] = (struct Gadget *)ButtonObject,
                        GA_ID, GID_BTN_NEW,
                        GA_RelVerify, TRUE,
                        GA_Text, MSG_MW_NewGadget,
                    ButtonEnd,
                    LAYOUT_AddChild, prefs->gads[GID_BTN_EDIT] = (struct Gadget *)ButtonObject,
                        GA_ID, GID_BTN_EDIT,
                        GA_RelVerify, TRUE,
                        GA_Text, MSG_MW_EditGadget,
                    ButtonEnd,
                    LAYOUT_AddChild, prefs->gads[GID_BTN_DELETE] = (struct Gadget *)ButtonObject,
                        GA_ID, GID_BTN_DELETE,
                        GA_RelVerify, TRUE,
                        GA_Text, MSG_MW_DeleteGadget,
                    ButtonEnd,
                    LAYOUT_AddChild, prefs->gads[GID_BTN_UP] = (struct Gadget *)ButtonObject,
                        GA_ID, GID_BTN_UP,
                        GA_RelVerify, TRUE,
                        GA_Text, MSG_MW_GadgetUp,
                    ButtonEnd,
                    LAYOUT_AddChild, prefs->gads[GID_BTN_DOWN] = (struct Gadget *)ButtonObject,
                        GA_ID, GID_BTN_DOWN,
                        GA_RelVerify, TRUE,
                        GA_Text, MSG_MW_GadgetDown,
                    ButtonEnd,
                End,
                CHILD_WeightedWidth, 0,
            End,
            MemberLabel(MSG_MW_Gadgets),
            LAYOUT_AddChild, HLayoutObject,
                LAYOUT_EvenSize, TRUE,
                LAYOUT_AddChild, prefs->gads[GID_BTN_SAVE] = (struct Gadget *)ButtonObject,
                    GA_ID, GID_BTN_SAVE,
                    GA_RelVerify, TRUE,
                    GA_Text, MSG_MW_ConfigSave,
                ButtonEnd,
                LAYOUT_AddChild, prefs->gads[GID_BTN_USE] = (struct Gadget *)ButtonObject,
                    GA_ID, GID_BTN_USE,
                    GA_RelVerify, TRUE,
                    GA_Text, MSG_MW_ConfigUse,
                ButtonEnd,
                LAYOUT_AddChild, prefs->gads[GID_BTN_TEST] = (struct Gadget *)ButtonObject,
                    GA_ID, GID_BTN_TEST,
                    GA_RelVerify, TRUE,
                    GA_Text, MSG_MW_ConfigTest,
                ButtonEnd,
                LAYOUT_AddChild, prefs->gads[GID_BTN_CANCEL] = (struct Gadget *)ButtonObject,
                    GA_ID, GID_BTN_CANCEL,
                    GA_RelVerify, TRUE,
                    GA_Text, MSG_Cancel,
                ButtonEnd,
            End,
            CHILD_WeightedHeight, 0,
        End,
    EndWindow;

    if( prefs->mainObj == NULL ) {
        return FALSE;
    }

    apply_config_to_gui(prefs);

    prefs->mainWin = RA_OpenWindow(prefs->mainObj);
    return (BOOL)(prefs->mainWin != NULL);
}

STATIC VOID show_about(struct DockPrefs *prefs)
{
    TD_OpenRequest(prefs->mainWin, (STRPTR)MSG_APP_Title,
        (STRPTR)MSG_APP_Info, (STRPTR)MSG_OK, TDREQIMAGE_INFO);
}

STATIC VOID sync_main_values(struct DockPrefs *prefs, ULONG id)
{
    ULONG val;
    STRPTR text;

    val = 0;
    text = NULL;

    switch( id ) {
        case GID_POSITION:
            GetAttr(CHOOSER_Selected, prefs->gads[GID_POSITION], &val);
            prefs->cfg.pos = (DockPosition)val;
            break;
        case GID_ALIGNMENT:
            GetAttr(CHOOSER_Selected, prefs->gads[GID_ALIGNMENT], &val);
            prefs->cfg.align = (DockAlign)val;
            break;
        case GID_SHOW_LABELS:
            GetAttr(GA_Selected, prefs->gads[GID_SHOW_LABELS], &val);
            prefs->cfg.showGadgetLabels = (BOOL)val;
            break;
        case GID_SHOW_BORDERS:
            GetAttr(GA_Selected, prefs->gads[GID_SHOW_BORDERS], &val);
            prefs->cfg.showGadgetBorders = (BOOL)val;
            break;
        case GID_PADDING:
            GetAttr(SLIDER_Level, prefs->gads[GID_PADDING], &val);
            prefs->cfg.gadgetPadding = (UWORD)val;
            if( prefs->gads[GID_PADDING_INT] && prefs->mainWin ) {
                SetGadgetAttrs(prefs->gads[GID_PADDING_INT], prefs->mainWin, NULL,
                    INTEGER_Number, val, TAG_END);
            }
            break;
        case GID_PADDING_INT:
            GetAttr(INTEGER_Number, prefs->gads[GID_PADDING_INT], &val);
            prefs->cfg.gadgetPadding = (UWORD)val;
            if( prefs->gads[GID_PADDING] && prefs->mainWin ) {
                SetGadgetAttrs(prefs->gads[GID_PADDING], prefs->mainWin, NULL,
                    SLIDER_Level, val, TAG_END);
            }
            break;
        case GID_BG_BRUSH:
            GetAttr(STRINGA_TextVal, prefs->gads[GID_BG_BRUSH], (ULONG *)&text);
            update_background_path(prefs, text);
            break;
        case GID_GADGETS:
            gadget_selected(prefs, get_list_index(prefs));
            break;
    }
}

STATIC BOOL handle_main_input(struct DockPrefs *prefs)
{
    ULONG result;
    UWORD code;
    BOOL done;

    done = FALSE;

    if( prefs->mainObj == NULL ) {
        return TRUE;
    }

    while( (result = RA_HandleInput(prefs->mainObj, &code)) != WMHI_LASTMSG ) {
        switch( result & WMHI_CLASSMASK ) {
            case WMHI_CLOSEWINDOW:
                revert_config();
                done = TRUE;
                break;

            case WMHI_ICONIFY:
                if( RA_Iconify(prefs->mainObj) ) {
                    prefs->mainWin = NULL;
                }
                break;

            case WMHI_UNICONIFY:
                prefs->mainWin = RA_OpenWindow(prefs->mainObj);
                break;

            case WMHI_MENUPICK:
                switch( result & WMHI_MENUMASK ) {
                    case MID_ABOUT:
                        show_about(prefs);
                        break;
                    case MID_QUIT:
                        revert_config();
                        done = TRUE;
                        break;
                }
                break;

            case WMHI_GADGETUP:
                switch( result & WMHI_GADGETMASK ) {
                    case GID_BTN_CANCEL:
                        revert_config();
                        done = TRUE;
                        break;
                    case GID_BTN_SAVE:
                        save_config(prefs, TRUE);
                        save_config(prefs, FALSE);
                        done = TRUE;
                        break;
                    case GID_BTN_USE:
                        save_config(prefs, FALSE);
                        done = TRUE;
                        break;
                    case GID_BTN_TEST:
                        save_config(prefs, FALSE);
                        break;
                    case GID_BTN_NEW:
                        open_new_gadget_dialog(prefs);
                        break;
                    case GID_BTN_EDIT:
                        edit_gadget(prefs, get_selected_gadget(prefs));
                        break;
                    case GID_BTN_DELETE:
                        delete_gadget(prefs);
                        break;
                    case GID_BTN_UP:
                        move_gadget_up(prefs);
                        break;
                    case GID_BTN_DOWN:
                        move_gadget_down(prefs);
                        break;
                    case GID_BTN_BG_BRUSH:
                        select_background(prefs);
                        break;
                    default:
                        sync_main_values(prefs, result & WMHI_GADGETMASK);
                        break;
                }
                break;
        }
    }
    return done;
}

STATIC VOID run_event_loop(struct DockPrefs *prefs)
{
    ULONG sigs;
    ULONG wsig;
    BOOL done;

    done = FALSE;

    while( !done ) {
        wsig = 0;
        if( prefs->mainObj ) {
            GetAttr(WINDOW_SigMask, prefs->mainObj, &wsig);
        }
        if( prefs->newObj ) {
            ULONG nsig;
            nsig = 0;
            GetAttr(WINDOW_SigMask, prefs->newObj, &nsig);
            wsig |= nsig;
        }
        if( prefs->editObj ) {
            ULONG esig;
            esig = 0;
            GetAttr(WINDOW_SigMask, prefs->editObj, &esig);
            wsig |= esig;
        }

        sigs = Wait(wsig | SIGBREAKF_CTRL_C);
        if( sigs & SIGBREAKF_CTRL_C ) {
            revert_config();
            done = TRUE;
        } else {
            if( prefs->newObj ) {
                handle_new_gadget_input(prefs);
            }
            if( prefs->editObj ) {
                handle_edit_input(prefs);
            }
            if( handle_main_input(prefs) ) {
                done = TRUE;
            }
        }
    }
}

STATIC VOID cleanup_gui(struct DockPrefs *prefs)
{
    close_new_gadget_dialog(prefs);
    if( prefs->editObj ) {
        close_edit_dialog(prefs, FALSE);
    }
    free_gadget_nodes(prefs);

    if( prefs->mainObj ) {
        DisposeObject(prefs->mainObj);
        prefs->mainObj = NULL;
        prefs->mainWin = NULL;
    }

    FreeChooserLabels(prefs->posLabels);
    FreeChooserLabels(prefs->alignLabels);
    FreeChooserLabels(prefs->startLabels);
    FreeChooserLabels(prefs->swPosLabels);
    prefs->posLabels = NULL;
    prefs->alignLabels = NULL;
    prefs->startLabels = NULL;
    prefs->swPosLabels = NULL;
}

int main(int argc, char **argv)
{
    struct DockPrefs prefs;

    argc = argc;
    argv = argv;

#ifdef DEBUG_BUILD
    parse_args();
#endif

    memset(&prefs, 0, sizeof(prefs));
    open_catalog();

    if( ToolDockBase = OpenLibrary("PROGDIR:tooldock.library", 1) ) {
        DEBUG(TD_RegisterDebugStream(Output()));

        if( open_reaction_classes() ) {
            prefs.idcmpPort = CreateMsgPort();
            prefs.appPort = CreateMsgPort();

            if( prefs.idcmpPort && prefs.appPort ) {
                init_config(&prefs);
                load_config(&prefs);

                if( TD_ListClasses(&prefs.classes) ) {
                    if( open_main_window(&prefs) ) {
                        update_gadget_list(&prefs);
                        gadget_selected(&prefs, 0);
                        run_event_loop(&prefs);
                    } else {
                        TD_ShowError((STRPTR)MSG_ERR_OpenWindow);
                    }
                    cleanup_gui(&prefs);
                } else {
                    TD_ShowError((STRPTR)MSG_ERR_ClassList);
                }
                free_config(&prefs);
            }

            if( prefs.idcmpPort ) {
                DeleteMsgPort(prefs.idcmpPort);
            }
            if( prefs.appPort ) {
                DeleteMsgPort(prefs.appPort);
            }
            close_reaction_classes();
        }
        close_lib("PROGDIR:tooldock.library", ToolDockBase);
        ToolDockBase = NULL;
    }

    close_catalog();
    return 0;
}
