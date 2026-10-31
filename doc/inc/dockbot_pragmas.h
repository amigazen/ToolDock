@DATABASE "dockbot_pragmas.h"
@MASTER   "Stuff:Andrew/DockBot/include/dockbot_pragmas.h"
@REMARK   This file was created by ADtoHT 2.1 on 05-May-20 21:03:26
@REMARK   Do not edit
@REMARK   ADtoHT is © 1993-1995 Christian Stieber

@NODE MAIN "dockbot_pragmas.h"

@{"dockbot_pragmas.h" LINK File}

@ENDNODE
@NODE File "dockbot_pragmas.h"
/************************************/
/*                                  */
/*  ToolDock - A Dock For AmigaOS 3 */
/*                                  */
/*  SPDX-License-Identifier: MIT */
/*  ToolDock changes Copyright 2026 amigazen project */
/*  DockBot Copyright 2019 Andrew Kennan */
/*                                  */
/************************************/
/* Dock gadget functions*/
#pragma libcall ToolDockBase TD_GetDockGadgetEnvironment 1e 9802
#pragma libcall ToolDockBase TD_RequestDockQuit 24 801
#pragma libcall ToolDockBase TD_RequestDockGadgetDraw 2a 801
#pragma libcall ToolDockBase TD_RequestLaunch 30 0BA9805
/* Drawing utilities*/
#pragma libcall ToolDockBase TD_DrawOutsetFrame 36 9802
#pragma libcall ToolDockBase TD_DrawInsetFrame 3c 9802
/* Settings functions*/
#pragma libcall ToolDockBase TD_OpenSettingsRead 42 801
#pragma libcall ToolDockBase TD_OpenSettingsWrite 48 801
#pragma libcall ToolDockBase TD_CloseSettings 4e 801
#pragma libcall ToolDockBase TD_ReadBeginBlock 54 801
#pragma libcall ToolDockBase TD_ReadEndBlock 5a 801
#pragma libcall ToolDockBase TD_ReadSetting 60 9802
#pragma libcall ToolDockBase TD_WriteBeginBlock 66 801
#pragma libcall ToolDockBase TD_WriteEndBlock 6c 801
#pragma libcall ToolDockBase TD_WriteSetting 72 A9803
#pragma libcall ToolDockBase TD_ReadConfig 78 9802
#pragma libcall ToolDockBase TD_WriteConfig 7e 9802
/* Memory Management*/
#pragma libcall ToolDockBase TD_AllocMem 84 1002
#pragma libcall ToolDockBase TD_FreeMem 8a 0802
#pragma libcall ToolDockBase TD_GetMemInfo 90 0
/* Class Management*/
#pragma libcall ToolDockBase TD_CreateDockGadget 96 801
#pragma libcall ToolDockBase TD_ListClasses 9c 801
/* User Messages*/
#pragma libcall ToolDockBase TD_ShowError a2 801
/* Gadget Message Ports*/
#pragma libcall ToolDockBase TD_RegisterPort a8 9802
#pragma libcall ToolDockBase TD_UnregisterPort ae 9802
/* Config cleanup*/
#pragma libcall ToolDockBase TD_DisposeConfig b4 801
/* More gadget stuff*/
#pragma libcall ToolDockBase TD_DisposeDockGadget ba 801
#pragma libcall ToolDockBase TD_FreeGadget c0 801
#pragma libcall ToolDockBase TD_AllocGadget c6 801
/* Debug Logging*/
#pragma libcall ToolDockBase TD_RegisterDebugStream cc 801
#pragma libcall ToolDockBase TD_DebugLog d2 9802
/* Brushes*/
#pragma libcall ToolDockBase TD_LoadBrush d8 0802
#pragma libcall ToolDockBase TD_FreeBrush de 801
#pragma libcall ToolDockBase TD_DrawBrush e4 5432109808
#pragma libcall ToolDockBase TD_GetBrushSize ea A9803
/* File Requester*/
#pragma libcall ToolDockBase TD_SelectFile f0 BA9804
@ENDNODE
