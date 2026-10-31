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

#ifndef DOCK_CAT_H
#define DOCK_CAT_H

#include <exec/types.h>

VOID open_catalog(VOID);
VOID close_catalog(VOID);

struct FC_String {
    const UBYTE *msg;
    LONG id;
};

extern struct FC_String Dock_Strings[60];

#define MSG_APP_Title (Dock_Strings[0].msg)
#define MSG_APP_Info (Dock_Strings[1].msg)
#define MSG_OK (Dock_Strings[2].msg)
#define MSG_Cancel (Dock_Strings[3].msg)
#define MSG_ERR_ClassList (Dock_Strings[4].msg)
#define MSG_ERR_LoadConfig (Dock_Strings[5].msg)
#define MSG_ERR_OpenWindow (Dock_Strings[6].msg)
#define MSG_ERR_OpenLibrary (Dock_Strings[7].msg)
#define MSG_ERR_EditorFailure (Dock_Strings[8].msg)
#define MSG_ERR_CantAddDirectory (Dock_Strings[9].msg)
#define MSG_POS_Left (Dock_Strings[10].msg)
#define MSG_POS_Right (Dock_Strings[11].msg)
#define MSG_POS_Top (Dock_Strings[12].msg)
#define MSG_POS_Bottom (Dock_Strings[13].msg)
#define MSG_ALIGN_TopLeft (Dock_Strings[14].msg)
#define MSG_ALIGN_Center (Dock_Strings[15].msg)
#define MSG_ALIGN_BottomRight (Dock_Strings[16].msg)
#define MSG_NGD_Title (Dock_Strings[17].msg)
#define MSG_NGD_ChooseGadget (Dock_Strings[18].msg)
#define MSG_NGD_About (Dock_Strings[19].msg)
#define MSG_MENU_Project (Dock_Strings[20].msg)
#define MSG_MENU_About (Dock_Strings[21].msg)
#define MSG_MENU_Quit (Dock_Strings[22].msg)
#define MSG_MW_Title (Dock_Strings[23].msg)
#define MSG_MW_DockSettings (Dock_Strings[24].msg)
#define MSG_MW_Position (Dock_Strings[25].msg)
#define MSG_MW_Alignment (Dock_Strings[26].msg)
#define MSG_MW_ShowLabels (Dock_Strings[27].msg)
#define MSG_MW_ShowBorders (Dock_Strings[28].msg)
#define MSG_MW_Background (Dock_Strings[29].msg)
#define MSG_MW_Gadgets (Dock_Strings[30].msg)
#define MSG_MW_NewGadget (Dock_Strings[31].msg)
#define MSG_MW_EditGadget (Dock_Strings[32].msg)
#define MSG_MW_DeleteGadget (Dock_Strings[33].msg)
#define MSG_MW_GadgetUp (Dock_Strings[34].msg)
#define MSG_MW_GadgetDown (Dock_Strings[35].msg)
#define MSG_MW_ConfigSave (Dock_Strings[36].msg)
#define MSG_MW_ConfigTest (Dock_Strings[37].msg)
#define MSG_MW_ConfigUse (Dock_Strings[38].msg)
#define MSG_DEL_Title (Dock_Strings[39].msg)
#define MSG_DEL_Text (Dock_Strings[40].msg)
#define MSG_DEL_Buttons (Dock_Strings[41].msg)
#define MSG_FR_ChooseBackground (Dock_Strings[42].msg)
#define MSG_MW_Padding (Dock_Strings[43].msg)
#define MSG_ED_Name (Dock_Strings[44].msg)
#define MSG_ED_Path (Dock_Strings[45].msg)
#define MSG_ED_Brush (Dock_Strings[46].msg)
#define MSG_ED_Args (Dock_Strings[47].msg)
#define MSG_ED_Key (Dock_Strings[48].msg)
#define MSG_ED_StartType (Dock_Strings[49].msg)
#define MSG_ED_Console (Dock_Strings[50].msg)
#define MSG_ED_Format (Dock_Strings[51].msg)
#define MSG_ED_NumEyes (Dock_Strings[52].msg)
#define MSG_ED_WinPos (Dock_Strings[53].msg)
#define MSG_ED_Workbench (Dock_Strings[54].msg)
#define MSG_ED_Shell (Dock_Strings[55].msg)
#define MSG_ED_DockWindow (Dock_Strings[56].msg)
#define MSG_ED_ScreenCentre (Dock_Strings[57].msg)
#define MSG_FR_ChooseTarget (Dock_Strings[58].msg)
#define MSG_FR_ChooseBrush (Dock_Strings[59].msg)

#endif
