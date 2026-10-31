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

#ifndef __DOCKBOT_PROTOS_H__
#define __DOCKBOT_PROTOS_H__

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <dos/dos.h>

#include "dockbot.h"

/****** tooldock.library/TD_GetDockGadgetEnvironment ***********************
*
*   NAME
*	TD_GetDockGadgetEnvironment -- Get the environment info for the gadget.
*
*   SYNOPSIS
*	TD_GetDockGadgetEnvironment(obj, env)
*	                            A0    A1
*	VOID TD_GetDockGadgetEnvironment(Object *, struct GadgetEnvironment *)
*
*   FUNCTION
*	Queries a dock gadget for it's position, size and information about
*   the dock window.
*
*   INPUTS
*	obj    - The dock gadget to query.
*	env - A pointer to a DockGadgetEnvironment to be filled in by the gadget.
*
*   RESULT
*	The parameter env will be filled contain the bounds of the gadget as well
*   as the position of the Dock window and its alignment.
*
*   EXAMPLE
*	struct DockGadgetEnvironment env;
*	TD_GetDockGadgetEnvironment(obj, &env);
*
***************************************************************************/
VOID __asm __saveds TD_GetDockGadgetEnvironment(
	register __a0 Object *obj, 
	register __a1 struct GadgetEnvironment *env);

/****** tooldock.library/TD_DrawOutsetFrame *********************************
*
*   NAME
*	TD_DrawOutsetFrame -- Draws a raised 3d rectangle.
*
*   SYNOPSIS
*	TD_DrawOutsetFrame(rp, bounds)
*	                   A0    A1
*	VOID TD_DrawOutsetFrame(struct RastPort *, struct Rect *);
*
*   FUNCTION
*	Draws a raised 3d frame with the supplied RastPort.
*
*   INPUTS
*	rp     - The RastPort to draw the frame with.
*	bounds - A pointer to a Rect that defines the frame to draw.
*
*   RESULT
*	A 3d frame will be drawn to the window.
*
*   EXAMPLE
*	struct Rect bounds;
*	TD_GetDockGadgetBounds(obj, &bounds);
*	TD_DrawOutsetFrame(rp, &bounds);
*
***************************************************************************/
VOID __asm __saveds TD_DrawOutsetFrame(
	register __a0 struct RastPort *rp, 
	register __a1 struct Rect *bounds);


/****** tooldock.library/TD_DrawInsetFrame **********************************
*
*   NAME
*	TD_DrawOutsetFrame -- Draws a lowered 3d rectangle.
*
*   SYNOPSIS
*	TD_DrawInsetFrame(rp, bounds)
*	                  A0    A1
*	VOID TD_DrawInsetFrame(struct RastPort *, struct Rect *);
*
*   FUNCTION
*	Draws a lowered 3d frame with the supplied RastPort.
*
*   INPUTS
*	rp     - The RastPort to draw the frame with.
*	bounds - A pointer to a Rect that defines the frame to draw.
*
*   RESULT
*	A 3d frame will be drawn to the window.
*
*   EXAMPLE
*	struct Rect bounds;
*	TD_GetDockGadgetBounds(obj, &bounds);
*	TD_DrawInsetFrame(rp, &bounds);
*
***************************************************************************/
VOID __asm __saveds TD_DrawInsetFrame(
	register __a0 struct RastPort *rp, 
	register __a1 struct Rect *bounds);



/****** tooldock.library/TD_OpenSettingsRead ********************************
*
*   NAME
*	TD_OpenSettingsRead -- Open the settings file for reading.
*
*   SYNOPSIS
*	Settings = TD_OpenSettingsRead(filename)
*	  D0                              A0
*	struct DockSettings *TD_OpenSettingsRead(STRPTR);
*
*   FUNCTION
*	Opens the settings file for reading.
*
*   INPUTS
*	filename - The full path to the settings file to open.
*
*   RESULT
*	Returns a pointer to a DockSettings structure set up for reading.
*	Returns NULL if there was an error opening or reading the file.
*
*   EXAMPLE
*	struct DockSettings *s;
*	if( s = TD_OpenSettingsRead("ENV:ToolDock.prefs") ) {
*	  ...
*	  TD_CloseSettings(s);
*	}
*
*   SEE ALSO
*	TD_OpenSettingsWrite(),TD_CloseSettings()
*
***************************************************************************/
struct DockSettings * __asm __saveds TD_OpenSettingsRead(
    register __a0 STRPTR filename);



/****** tooldock.library/TD_OpenSettingsWrite *****************************
*
*   NAME
*	TD_OpenSettingsWrite -- Open the settings file for writing.
*
*   SYNOPSIS
*	Settings = TD_OpenSettingsWrite(filename)
*	  D0                              A0
*	struct DockSettings *TD_OpenSettingsWrite(STRPTR);
*
*   FUNCTION
*	Opens the settings file for writing. If the file doesn't exist it will
*	be created.
*
*   INPUTS
*	filename - The full path to the settings file to open.
*
*   RESULT
*	Returns a pointer to a DockSettings structure set up for writing.
*	Returns NULL if there was an error opening the file.
*
*   EXAMPLE
*	struct DockSettings *s;
*	if( s = TD_OpenSettingsWrite("ENV:ToolDock.prefs") ) {
*	  ...
*	  TD_CloseSettings(s);
*	}
*
*   SEE ALSO
*	TD_OpenSettingsRead(),TD_CloseSettings()
*
***************************************************************************/
struct DockSettings * __asm __saveds TD_OpenSettingsWrite(
    register __a0 STRPTR filename);



/****** tooldock.library/TD_CloseSettings ***********************************
*
*   NAME
*	TD_CloseSettings -- Closes a previously opened settings file.
*
*   SYNOPSIS
*	TD_CloseSettings(settings)
*	                   A0
*	VOID TD_OpenSettingsWrite(struct DockSettings *);
*
*   FUNCTION
*	Closes and deallocates a DockSettings structure previously allocated by
*	TD_OpenSettingsRead or TD_OpenSettingsWrite. 
*	Do not use the settings object after calling this function.
*
*   INPUTS
*	settings - The DockSettings structure previously allocated.
*
*   RESULT
*	The DockSettings structure will be deallocated.
*
*   EXAMPLE
*	struct DockSettings *s;
*	if( s = TD_OpenSettingsRead("ENV:ToolDock.prefs") ) {
*	  ...
*	  TD_CloseSettings(s);
*	}
*
*   SEE ALSO
*	TD_OpenSettingsRead(),TD_OpenSettingsWrite()
*
***************************************************************************/
VOID __asm __saveds TD_CloseSettings(
    register __a0 struct DockSettings *settings);


/****** tooldock.library/TD_DisposeConfig ***********************************
*
*   NAME
*	TD_DisposeConfig - Cleans up the gadgets on a DockConfig structure.
*
*   SYNOPSIS
*	TD_DisposeConfig(cfg)
*	                 A0
*	VOID TD_DisposeConfig(struct DockConfig *);
*
*   FUNCTION
*   Disposes of gadget objects and cleans up associated memory allocated 
*   by the DockConfig structure.
*
*   INPUTS
*	cfg - The DockConfig structure containing references to gadgets.
*
*   RESULT
*	The gadgets allocated by TD_ReadConfig will be cleaned up.
*
*   SEE ALSO
*	TD_ReadConfig()
*
***************************************************************************/
VOID __asm __saveds TD_DisposeConfig(
    register __a0 struct DockConfig *cfg);


/****** tooldock.library/TD_ReadBeginBlock *********************************
*
*   NAME
*	TD_ReadBeginBlock -- Attempts to read a block start from the setings.
*
*   SYNOPSIS
*	success = TD_ReadBeginBlock(settings)
*	  D0                          A0
*	BOOL TD_ReadBeginBlock(struct DockSettings *);
*
*   FUNCTION
*	Looks for the word "begin" on the current line of the settings file.
*	If found the DockSettings struct records the beginning of an indented 
*	block.
*
*   INPUTS
*	settings - The DockSettings structure allocated by TD_OpenSettingsRead.
*
*   RESULT
*	TRUE if a block start was found, FALSE if it was not.
*
*   EXAMPLE
*	struct DockSettings *s;
*	if( s = TD_OpenSettingsRead("ENV:ToolDock.prefs") ) {
*	  if( TD_ReadBeginBlock(s) ) {
*	    while(!TD_ReadEndBlock(s)) {
*	      ...
*	    }
*	  }
*	  TD_CloseSettings(s);
*	}
*
*   SEE ALSO
*	TD_OpenSettingsRead(),TD_ReadEndBlock()
*
***************************************************************************/
BOOL __asm __saveds TD_ReadBeginBlock(
    register __a0 struct DockSettings *settings);


/****** tooldock.library/TD_ReadEndBlock ************************************
*
*   NAME
*	TD_ReadEndBlock -- Attempts to read a block end from the setings.
*
*   SYNOPSIS
*	success = TD_ReadEndBlock(settings)
*	  D0                        A0
*	BOOL TD_ReadEndBlock(struct DockSettings *);
*
*   FUNCTION
*	Looks for the word "end" on the current line of the settings file.
*	If found the DockSettings struct records the end of an indented block.
*
*   INPUTS
*	settings - The DockSettings structure allocated by TD_OpenSettingsRead.
*
*   RESULT
*	TRUE if a block end was found, FALSE if it was not.
*
*   EXAMPLE
*	struct DockSettings *s;
*	if( s = TD_OpenSettingsRead("ENV:ToolDock.prefs") ) {
*	  if( TD_ReadBeginBlock(s) ) {
*	    while(!TD_ReadEndBlock(s)) {
*	      ...
*	    }
*	  }
*	  TD_CloseSettings(s);
*	}
*
*   SEE ALSO
*	TD_OpenSettingsRead(),TD_ReadBeginBlock()
*
***************************************************************************/
BOOL __asm __saveds TD_ReadEndBlock(
    register __a0 struct DockSettings *settings);


/****** tooldock.library/TD_ReadSetting *************************************
*
*   NAME
*	TD_ReadSetting -- Attempts to read a value from the settings file.
*
*   SYNOPSIS
*	success = TD_ReadSetting(settings, value)
*	  D0                       A0        A1
*	BOOL TD_ReadSetting(struct DockSettings *, struct DockSettingValue *);
*
*   FUNCTION
*	Looks for the key and value separated by an equals sign on the current 
*	line of the settings file.
*	If found the value struct will be filled in with key and value.
*
*   INPUTS
*	settings - The DockSettings structure allocated by TD_OpenSettingsRead.
*
*   RESULT
*	TRUE if a block end was found, FALSE if it was not.
*
*   EXAMPLE
*	struct DockSettings *s;
*	struct DockSettingValue val;
*	if( s = TD_OpenSettingsRead("ENV:ToolDock.prefs") ) {
*	  if( TD_ReadBeginBlock(s) ) {
*	    while(!TD_ReadEndBlock(s)) {
*	      if( TD_ReadSetting(s, &val) ) {
*	        ...
*	      }
*	    }
*	  }
*	  TD_CloseSettings(s);
*	}
*
*   SEE ALSO
*	TD_OpenSettingsRead()
*
***************************************************************************/
BOOL __asm __saveds TD_ReadSetting(
    register __a0 struct DockSettings *settings, 
    register __a1 struct DockSettingValue *value);


/****** tooldock.library/TD_WriteBeginBlock *********************************
*
*   NAME
*	TD_WriteBeginBlock -- Writes the start of a block to the settings file.
*
*   SYNOPSIS
*	success = TD_WriteBeginBlock(settings)
*	  D0                            A0       
*	BOOL TD_WriteBeginBlock(struct DockSettings *);
*
*   FUNCTION
*	Writes the word "begin" to the settings file previously opened with 
*	TD_OpenSettingsWrite. Subsequent writes to the file will be indented.
*
*   INPUTS
*	settings - The DockSettings structure allocated by TD_OpenSettingsWrite.
*
*   RESULT
*	TRUE if a block begin was written or FALSE if it was not.
*
*   EXAMPLE
*	struct DockSettings *s;
*	if( s = TD_OpenSettingsWrite("ENV:ToolDock.prefs") ) {
*	  if( TD_WriteBeginBlock(s) ) {
*	    ...
*	  }
*	  TD_CloseSettings(s);
*	}
*
*   SEE ALSO
*	TD_OpenSettingsWrite(), TD_WriteEndBlock()
*
***************************************************************************/
BOOL __asm __saveds TD_WriteBeginBlock(
    register __a0 struct DockSettings *settings);


/****** tooldock.library/TD_WriteEndBlock ***********************************
*
*   NAME
*	TD_WriteEndBlock -- Writes the end of a block to the settings file.
*
*   SYNOPSIS
*	success = TD_WriteBeginEnd(settings)
*	  D0                         A0       
*	BOOL TD_WriteBeginEnd(struct DockSettings *);
*
*   FUNCTION
*	Writes the word "end" to the settings file previously opened with 
*	TD_OpenSettingsWrite.
*
*   INPUTS
*	settings - The DockSettings structure allocated by TD_OpenSettingsWrite.
*
*   RESULT
*	TRUE if a block end was written or FALSE if it was not.
*
*   EXAMPLE
*	struct DockSettings *s;
*	if( s = TD_OpenSettingsWrite("ENV:ToolDock.prefs") ) {
*	  if( TD_WriteBeginBlock(s) ) {
*	    ...
*	    if( TD_WriteEndBlock(s) ) {
*	      ...
*	    }
*	  }
*	  TD_CloseSettings(s);
*	}
*
*   SEE ALSO
*	TD_OpenSettingsWrite(), TD_WriteBeginBlock()
*
***************************************************************************/
BOOL __asm __saveds TD_WriteEndBlock(
    register __a0 struct DockSettings *settings);


/****** tooldock.library/TD_WriteSetting ************************************
*
*   NAME
*	TD_WriteSetting -- Writes a key and value pair to the settings file.
*
*   SYNOPSIS
*	success = TD_WriteSetting(settings,key,value)
*	  D0                        A0     A1  A2  
*	BOOL TD_WriteBeginEnd(struct DockSettings *,STRPTR,STRPTR);
*
*   FUNCTION
*	Writes a key and value to a settings file previously opened with
*	TD_OpenSettingsWrite. An equals sign, =, will separate the two.
*
*   INPUTS
*	settings - The DockSettings structure allocated by TD_OpenSettingsWrite.
*	key      - A pointer to a NULL terminated string containing the key.
*	value    - A pointer to a NULL terminated string containing the value.
*
*   RESULT
*	TRUE if the setting was written or FALSE if it was not.
*
*   EXAMPLE
*	struct DockSettings *s;
*	if( s = TD_OpenSettingsWrite("ENV:ToolDock.prefs") ) {
*	  if( TD_WriteBeginBlock(s) ) {
*	    if( TD_WriteSetting(s, "key", "value") ) {
*	      ...
*	    }
*	  }
*	  TD_CloseSettings(s);
*	}
*
*   SEE ALSO
*	TD_OpenSettingsWrite(), TD_WriteBeginBlock()
*
***************************************************************************/
BOOL __asm __saveds TD_WriteSetting(
    register __a0 struct DockSettings *settings, 
    register __a1 STRPTR key, 
    register __a2 STRPTR value);

/****** tooldock.library/TD_ReadConfig ***********************************
*
*   NAME
*	TD_ReadConfig - Read the ToolDock configuration file.
*
*   SYNOPSIS
*	success = TD_ReadConfig(cfg,settings)
*	  D0					A0  A1
*   BOOL TD_ReadConfig(struct DockConfig *, struct DockSettings *)
*
*   FUNCTION
*	Reads the configuration data from the settings object and populates
*   the cfg object.
*
*   INPUTS
*   cfg			- The container for configuration data.
*	settings	- The structure used to read configuration data.
*
*   RESULT
*	TRUE if the configuration was read.
*
***************************************************************************/
BOOL __asm __saveds TD_ReadConfig(
    register __a0 struct DockConfig *cfg,
    register __a1 struct DockSettings *settings);

/****** tooldock.library/TD_WriteConfig ***********************************
*
*   NAME
*	TD_WriteConfig - Write the ToolDock configuration file.
*
*   SYNOPSIS
*	success = TD_WriteConfig(cfg,settings)
*	  D0					 A0  A1
*   BOOL TD_WriteConfig(struct DockConfig *, struct DockSettings *)
*
*   FUNCTION
*	Writes the configuration data in cfg to the settings object.
*
*   INPUTS
*   cfg			- The container for configuration data.
*	settings	- The structure used to write configuration data.
*
*   RESULT
*	TRUE if the configuration was written.
*
***************************************************************************/
BOOL __asm __saveds TD_WriteConfig(
    register __a0 struct DockConfig *cfg,
    register __a1 struct DockSettings *settings);


/****** tooldock.library/TD_RequestDockQuit *********************************
*
*   NAME
*	TD_RequestDockQuit -- Asks the ToolDock application to quit.
*
*   SYNOPSIS
*	TD_RequestDockQuit(obj)
*	                   A0  
*	TD_RequestDockQuit(Object *);
*
*   FUNCTION
*	Sends an asynchronous message from a gadget to the ToolDock application 
*	requesting that it quit.
*
*   INPUTS
*	obj - The dock gadget requesting the quit.
*
*   EXAMPLE
*	void request_quit(Object *obj) {
*	  TD_RequestDockQuit(obj);
*	}
*
*   SEE ALSO
*	dockbot.h
*
***************************************************************************/
VOID __asm __saveds TD_RequestDockQuit(
	register __a0 Object *obj);


/****** tooldock.library/TD_RequestDockGadgetDraw ****************************
*
*   NAME
*	TD_RequestDockGadgetDraw -- Requests the redraw of a dock gadget.
*
*   SYNOPSIS
*	TD_RequestDockGadgetDraw(obj)
*	                         A0  
*	TD_RequestDockGadgetDraw(Object *);
*
*   FUNCTION
*	Sends an asynchronous message from a gadget to the ToolDock application 
*	requesting that it be redrawn.
*
*   INPUTS
*	obj - The dock gadget requesting to be redrawn.
*
*   EXAMPLE
*	void request_draw(Object *obj) {
*	  TD_RequestDockGadgetDraw(obj);
*	}
*
*   SEE ALSO
*	dockbot.h
*
***************************************************************************/
VOID __asm __saveds TD_RequestDockGadgetDraw(
	register __a0 Object *obj);

/****** tooldock.library/TD_RequestLaunch ****************************
*
*   NAME
*	TD_RequestLaunch -- Requests the launch of an external program.
*
*   SYNOPSIS
*	TD_RequestLaunch(obj, path, args, console, wb)
*	                 A0   A1    A2    A3       D0
*	TD_RequestLaunch(Object *, STRPTR, STRPTR, STRPTR, BOOL);
*
*   FUNCTION
*	Sends an asynchronous message from a gadget to the ToolDock application 
*	requesting that it launch an external program.
*
*   INPUTS
*	obj - The dock gadget making the request.
*   path - The path to the executable to be launched.
*   args - Optional command line arguments to pass.
*   console - Optional console options to use when launching a DOS application.
*   wb - Indicates whether the program should be launched as if started by
*        Workbench.
*
*   EXAMPLE
*
*   TD_RequestLaunch(obj, "SYS:Utilities/MultiView", "T:image.iff", NULL, TRUE);
*
*   SEE ALSO
*	dockbot.h
*
***************************************************************************/
VOID __asm __saveds TD_RequestLaunch(
	register __a0 Object *obj,
    register __a1 STRPTR path,
    register __a2 STRPTR args,
    register __a3 STRPTR console,
    register __d0 BOOL wb);


/****** tooldock.library/TD_AllocMem ****************************************
*
*   NAME
*	TD_AllocMem -- Allocate memory from the pools maintained by the library.
*
*   SYNOPSIS
*	BLOCK = TD_AllocMem(byteSize, attributes)
*	  D0                  D0         D1
*	VOID* TD_AllocMem(ULONG, ULONG);
*
*   FUNCTION
*	Allocates a block of memory from the pools maintained internally by the
*	ToolDock library.
*
*   INPUTS
*	byteSize   - The size, in bytes, to allocate.
*	attributes - The requirements and options for the memory. This is a 
*	             subset of those accepted by the AllocMem function.
*
*	             requirements:
*
*	               MEMF_CHIP  - Allocate chip memory.
*	               
*	               MEMF_FAST  - Attempt to allocate fast memory.
*
*	             options:
*
*	               MEMF_CLEAR - The memory will be initialized to all zeros.
*
*   RESULT
*	The newly allocated block of memory will be returned. If the request
*	could not be satisfied 0 will be returned.
*
*   EXAMPLE
*	TD_AllocMem(64,0) - Allocate the best memory available.
*	TD_AllocMem(64, MEMF_CLEAR) - Allocate the best memory available and
*	                              clear it.
*	TD_AllocMem(64, MEMF_CHIP|MEMF_CLEAR) - Allocate cleared chip memory.
*
*   SEE ALSO
*	TD_FreeMem(), AllocMem()
*
***************************************************************************/
VOID* __asm __saveds TD_AllocMem(
    register __d0 ULONG byteSize,
    register __d1 ULONG attributes);


/****** tooldock.library/TD_FreeMem *****************************************
*
*   NAME
*	TD_FreeMem - Free a block of memory allocated by TD_AllocMem.
*
*   SYNOPSIS
*	TD_FreeMem(memoryBlock,byteSize)
*	              A0         D0
*	TD_FreeMem(VOID *,ULONG);
*
*   FUNCTION
* 	Frees a block of memory previously allocated with TD_AllocMem.
*
*   INPUTS
*	memoryBlock - The block of memory allocated by TD_AllocMem.
*	byteSize    - The size of the block.
*
*   SEE ALSO
*	TD_AllocMem(), FreeMem()
*
***************************************************************************/
VOID __asm __saveds TD_FreeMem(
    register __a0 VOID *memoryBlock,
    register __d0 ULONG byteSize);

VOID* __asm __saveds TD_GetMemInfo(VOID);

Object * __asm __saveds TD_CreateDockGadget(
    register __a0 STRPTR name);

VOID __asm __saveds TD_DisposeDockGadget(
    register __a0 Object *o);

VOID __asm __saveds TD_FreeGadget(
    register __a0 struct DgNode *dg);

struct DgNode * __asm __saveds TD_AllocGadget(
    register __a0 STRPTR name);

/****** tooldock.library/TD_ListClasses *************************************
*
*   NAME
*	TD_ListClasses - List the available gadget classes.
*
*   SYNOPSIS
*	TD_ListClasses(list)
*	               A0
*   TD_ListClasses(struct List *)
*
*   FUNCTION
*	Populates the supplied list object with nodes containing the names of
* 	
* 
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   SEE ALSO
*
***************************************************************************/
BOOL __asm __saveds TD_ListClasses(
    register __a0 struct List *list);


/****** tooldock.library/TD_ShowError ***************************************
*
*   NAME
*	TD_ShowError - Display an error message to the user.
*
*   SYNOPSIS
*	TD_ShowError(message)
*	              A0
*	TD_ShowError(STRPTR);
*
*   FUNCTION
*   Displays the message to the user in a requester.
*
*   INPUTS
*   message - The error message to display to the user.
*
***************************************************************************/
VOID __asm __saveds TD_ShowError(
    register __a0 STRPTR message);

STRPTR __asm __saveds TD_SelectFile(
    register __a0 STRPTR title,
    register __a1 STRPTR okText,
    register __a2 STRPTR cancelText,
    register __a3 STRPTR path);

/****** tooldock.library/TD_RegisterPort ************************************
*
*   NAME
*	TD_RegisterPort - Registers a message port with the Dock so that gadgets
*                     can receive messages.
*
*   SYNOPSIS
*	TD_RegisterPort(obj, port);
*	                 A0   A1
*	TD_RegisterPort(Object *, struct MsgPort *);
*
*   FUNCTION
*   Registers a gadget-owned port with the Dock so that the gadget can receive
*   custom messages. For example, if the gadget causes a window to be opened
*   this function allows messages received by the window's port to be routed
*   back to the gadget.
*
*   INPUTS
*   obj - The gadget object registering the port.
*   port - The port to be registered.
*
*   SEE ALSO
*   TD_UnregisterPort
*
***************************************************************************/
VOID __asm __saveds TD_RegisterPort(
    register __a0 Object *obj,
    register __a1 struct MsgPort *port);


/****** tooldock.library/TD_UnregisterPort ************************************
*
*   NAME
*	TD_UnregisterPort - Removes a previously registered port from the Dock.
*
*   SYNOPSIS
*	TD_UnregisterPort(obj, port);
*	                   A0   A1
*	TD_UnregisterPort(Object *, struct MsgPort *);
*
*   FUNCTION
*   Removes a port registration from the Dock when the gadget is finished with
*   it.
*
*   INPUTS
*   obj - The gadget object that registered the port.
*   port - The port to be un-registered.
*
*   SEE ALSO
*   TD_RegisterPort
*
***************************************************************************/
VOID __asm __saveds TD_UnregisterPort(
    register __a0 Object *obj,
    register __a1 struct MsgPort *port);

/****** tooldock.library/TD_RegisterDebugStream *****************************
*
*   NAME
*	TD_RegisterDebugStream - Registers an output stream with tooldock.library.
*
*   SYNOPSIS
*	TD_RegisterDebugStream(fh);
*	                       A0
*	TD_RegisterDebugStream(BPTR);
*
*   FUNCTION
*   Used for debug builds only. Registers an output stream for the library
*   and gadget classes to send debug output to.
*
*   INPUTS
*   fh - The output stream.
*
*   SEE ALSO
*   TD_DebugLog
*
***************************************************************************/
VOID __asm __saveds TD_RegisterDebugStream(
	register __a0 BPTR fh);

/****** tooldock.library/TD_DebugLog ***************************************
*
*   NAME
*	TD_DebugLog - Writes a message to the debug logging stream.
*
*   SYNOPSIS
*	TD_DebugLog(fmt,    argv);
*	            A0      A1
*	TD_DebugLog(STRPTR, LONG *);
*
*   FUNCTION
*   Used for debug builds only. Writes a log message to the debug output 
*   stream if one has been registered.
*
*   INPUTS
*   fmt - A printf style format string.
*   argv - Pointer to the first arg value.
*
*   SEE ALSO
*   TD_RegisterDebugStream, VFPrintf
*
***************************************************************************/
VOID __asm __saveds TD_DebugLog(
	register __a0 STRPTR fmt,
	register __a1 LONG *argv);

APTR __asm __saveds TD_LoadBrush(
    register __a0 STRPTR fileName,
    register __d0 UWORD flags);

VOID __asm __saveds TD_FreeBrush(
    register __a0 APTR brush);

VOID __asm __saveds TD_DrawBrush(
    register __a0 APTR brush,
    register __a1 struct RastPort *rp,
    register __d0 UWORD srcX,
    register __d1 UWORD srcY,
    register __d2 UWORD dstX,
    register __d3 UWORD dstY,
    register __d4 UWORD w,
    register __d5 UWORD h);

VOID __asm __saveds TD_GetBrushSize(
    register __a0 APTR brush,
    register __a1 UWORD *w,
    register __a2 UWORD *h);

#endif
