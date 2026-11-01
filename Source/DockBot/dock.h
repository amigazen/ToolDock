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

#ifndef __DOCK_H__
#define __DOCK_H__

#define APP_NAME        "ToolDock"

#include <exec/types.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <dos/notify.h>
#include <workbench/workbench.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
/* NDK proto: official intuition.library LVOs. Do not add libcalls. */
#include <proto/intuition.h>
#include <libraries/commodities.h>
#include <graphics/layers.h>
#include <utility/hooks.h>
#include <exec/semaphores.h>

#define REARRANGE_MAX_GADGETS 48
#define REARRANGE_MAX_ZONES   96
#define REARRANGE_ZONE_BASE   1000

#include "dockbot.h"
#include "dockbot_protos.h"
#include "dockbot_pragmas.h"
#include "dockbot_cat.h"

#include "dock_gadget.h"

#include "debug.h"

#define DEFAULT_CONSOLE "NIL:"

#define COPY_STRING(src, dst) \
    l = strlen(src);\
    CopyMem(src, dst, l);\
    dst += l;\
    *dst = ' ';\
    dst++; 

#define COPY_STRING_QUOTED(src, dst) \
    l = strlen(src);\
    *dst = '"';\
    dst++;\
    CopyMem(src, dst, l);\
    dst += l;\
    *dst = '"';\
    dst++;\
    *dst = ' ';\
    dst++;

typedef enum {
    RS_STARTING = 1,
    RS_RUNNING = 2,
    RS_LOADING = 3,
	RS_ICONIFYING = 4,
	RS_UNICONIFYING = 5,
    RS_QUITTING = 6,
    RS_STOPPED = 7,

    RS_START_EDIT = 10,
	RS_EDITING = 11,
    RS_STOP_EDIT = 12,
	RS_CHANGING = 13,

    RS_HIDDEN = 14,
    RS_REARRANGING = 15
} RunState;

typedef enum {
    MI_ABOUT = 1,
    MI_OPEN_PREFS = 2,
    MI_QUIT = 3,
	MI_HIDE = 4,
    MI_HELP = 5,
	MI_EDIT = 6,
	MI_SAVE = 7,
    MI_REVERT = 8
} MenuIndex;

typedef enum {
	II_ARROW_LEFT = 0,
	II_ARROW_RIGHT = 1,
	II_ARROW_UP = 2,
	II_ARROW_DOWN = 3,
	II_DELETE = 4,
	II_ARROW_LEFT_INV = 5,
	II_ARROW_RIGHT_INV = 6,
	II_ARROW_UP_INV = 7,
	II_ARROW_DOWN_INV = 8,
	II_DELETE_INV = 9,
    II_NONE = 10
} IconIndex;

#define ICON_COUNT 10

typedef enum {
	EO_NONE = 0,
	EO_MOVE_UP = 1,
	EO_MOVE_DOWN = 2,
	EO_DELETE = 3
} EditOperation;

struct DockWindow 
{
	struct DockConfig cfg;

    // Window
	struct Window *win;
    struct AppWindow *appWin;
    struct MsgPort *awPort;
    struct Menu* menu;

    // Built in classes
    Class *handleClass;
    Class *appIconClass;

    // Client dock icons (TD_AddDockIconA)
    struct MinList dockIcons;
    struct MsgPort *diReplyPort;

    // Prefs file change notification
    struct MsgPort* notifyPort;
    struct NotifyRequest notifyReq; 
    BOOL notifyEnabled;
    BOOL notifySupported;
    BOOL reloadPending;

    // Timer
    struct timerequest *timerReq;
    struct MsgPort* timerPort;

    // Gadget communication
    struct MsgPort* gadgetPort;

    // AppIcon
    struct MsgPort *aiPort;
	struct DiskObject* iconObj;
	struct AppIcon* appIcon;

    // Public message port.
    struct MsgPort *pubPort;

    RunState runState;
    BOOL disableLayout;

	// Commodities
	struct MsgPort *cxPort;
	CxObj *cxBroker;
    BOOL rmbHandled;
    BOOL rmbCxWatch;

    // Gadget name tooltips
    Object *hoverGad;

    // Background image
    APTR bgBrush;

    /* Directory containing the ToolDock executable. */
    UBYTE progPath[2048];

	// Editing
	APTR iconBrush;
	UWORD iconW;
	UWORD iconH;
	UWORD editCount;
	UWORD editOp;
	struct DgNode *editNode;

	// Rendering
    struct RastPort renderRP;
	struct Layer_Info *renderLI;
	struct Layer *renderL;
	struct BitMap *renderBM;
	UWORD renderW, renderH;

    /* Dwell-to-rearrange: reserved slot, WB hover, internal drag, Motion. */
    WORD gapIndex;
    UWORD gapSize;
    UWORD dwellCount;
    ULONG dwellSec;
    ULONG dwellMicro;
    BOOL dwellReady;
    BOOL dwellHold;
    BOOL wbVacant;
    WORD layoutX;
    WORD layoutY;
    UWORD layoutW;
    UWORD layoutH;
    BOOL hoverOverDock;
    BOOL rearrangeFromWB;
    BOOL dragArmed;
    BOOL motionBusy;
    struct DgNode *placeholder;
    WORD dropOnto;
    struct DgNode *dragNode;
    UWORD dragW;
    UWORD dragH;
    WORD dragStartX;
    WORD dragStartY;
    struct Hook dropHook;
    struct AppWindowDropZone *dropZones[REARRANGE_MAX_ZONES];
    struct IBox zoneBoxes[REARRANGE_MAX_ZONES];
    UWORD dropZoneCount;
    struct IBox restBox[REARRANGE_MAX_GADGETS];
    struct IBox drawBox[REARRANGE_MAX_GADGETS];
    struct IBox hookDest[REARRANGE_MAX_GADGETS];
    UWORD snapCount;
    struct BitMap *snapBM;
    struct BitMap *snapBG;
    struct BitMap *hookBM;
    UWORD snapW;
    UWORD snapH;
    BOOL snapBGReady;
    UWORD hookEnterCount;
    UWORD hookLeaveCount;
    UWORD zoneSnapCount;
    UWORD zoneSnapW;
    UWORD zoneSnapH;
    struct SignalSemaphore dropLock;
    BOOL dropValid;
    WORD shownGap;
    APTR motionHandle;
    APTR motionDisplay;
    struct IBox *motionFrom;
    struct IBox *motionModel;
    UWORD motionCount;
    UWORD motionTick;
};

#define TIMER_INTERVAL 250L

/* GetDiskObject()/GetIconTagList() append ".info" to this name. */
#define APP_ICON "PROGDIR:" APP_NAME

#define DOCK_HORIZONTAL(dock) (dock->cfg.pos == DP_TOP || dock->cfg.pos == DP_BOTTOM)

#define DOCK_EDITING(dock) (dock->runState == RS_EDITING || dock->runState == RS_CHANGING)

#define DOCK_REARRANGING(dock) (dock->runState == RS_REARRANGING)

// dock.c - Startup/shutdown, general functions.

struct DockWindow *create_dock(VOID);

VOID free_dock(struct DockWindow *);

VOID execute_external(struct DockWindow* dock, STRPTR path, STRPTR args, STRPTR console, BOOL wb);

VOID open_help(struct DockWindow *dock);

VOID delete_port(struct MsgPort *port);

VOID show_about(struct DockWindow *dock);


// settings.c

BOOL load_config(struct DockWindow *dock);

VOID free_config(struct DockWindow *dock);

BOOL init_config_notification(struct DockWindow *dock);

VOID free_config_notification(struct DockWindow *dock);

VOID handle_notify_message(struct DockWindow *dock);

VOID reload_config(struct DockWindow *dock);

VOID request_remove_gadget(struct DockWindow *dock, struct DgNode *dg);

VOID load_icon_brushes(struct DockWindow *dock);

VOID free_icon_brushes(struct DockWindow *dock);

VOID save_config(struct DockWindow *dock);

VOID add_dropped_icon(struct DockWindow *dock, BPTR dir, STRPTR name);

BOOL insert_dropped_icon(struct DockWindow *dock, BPTR dir, STRPTR name,
    struct DgNode *before);

// window.c - Window functions

VOID hide_dock_window(struct DockWindow* dock);

BOOL show_dock_window(struct DockWindow* dock);

VOID handle_drop_event(struct DockWindow* dock);

VOID handle_window_event(struct DockWindow *dock);

VOID handle_cx_rmb(struct DockWindow *dock);

VOID open_settings(struct DockWindow *dock);

VOID open_dock_prefs(struct DockWindow *dock);

VOID handle_change_config(struct DockWindow *dock);

VOID update_settings_menu(struct DockWindow *dock);

VOID enable_notification(struct DockWindow *dock);

VOID disable_notification(struct DockWindow *dock);

// events.c 

VOID run_event_loop(struct DockWindow *);


// timer.c

BOOL init_timer_notification(struct DockWindow *dock);

VOID free_timer_notification(struct DockWindow *dock);

VOID set_timer(struct DockWindow *dock, ULONG milliseconds);

VOID handle_timer_message(struct DockWindow *dock);


// appicon.c

BOOL show_app_icon(struct DockWindow *dock);

VOID free_app_icon(struct DockWindow *dock);

VOID handle_icon_event(struct DockWindow *dock);


// gadgets.c

BOOL create_dock_handle(struct DockWindow *dock);

BOOL init_gadget_classes(struct DockWindow *dock);

BOOL free_gadget_classes(struct DockWindow *dock);

VOID draw_gadgets(struct DockWindow *dock);

VOID draw_gadget(struct DockWindow *dock, Object *gadget);

VOID remove_dock_gadgets(struct DockWindow *dock);

struct DgNode *get_gadget_at(struct DockWindow *dock, UWORD x, UWORD y);

VOID update_hover_gadget(struct DockWindow *dock);

VOID handle_gadget_message(struct DockWindow *dock);

VOID remap_gadgets(struct DockWindow *dock);

ULONG get_custom_sigs(struct DockWindow *dock);

VOID handle_custom_message(struct DockWindow *dock, ULONG signal);

VOID update_entire_window(struct DockWindow *dock);

UWORD dock_label_band(struct DockWindow *dock);

BOOL dock_gadget_wants_label(struct DockWindow *dock, Object *gad);

VOID draw_gadget_label(struct DockWindow *dock, struct RastPort *rp, Object *gad);

VOID fill_background(struct DockWindow *dock, struct RastPort *rp, struct Rect *b);

// layout.c

ULONG get_window_top(struct Screen* screen, DockPosition pos, DockAlign align, UWORD height);

ULONG get_window_left(struct Screen* screen, DockPosition pos, DockAlign align, UWORD width);

VOID measure_dock_size(struct DockWindow *dock, UWORD *outW, UWORD *outH);

VOID layout_gadgets(struct DockWindow *dock);

VOID disable_layout(struct DockWindow *dock);

VOID enable_layout(struct DockWindow *dock);

VOID free_render_bitmap(struct DockWindow *dock);

VOID update_render_bitmap(struct DockWindow *dock, UWORD w, UWORD h);

// cx.c - Commodities Exchange handling

BOOL init_cx_broker(struct DockWindow *dock);

VOID free_cx_broker(struct DockWindow *dock);

VOID handle_cx_message(struct DockWindow *dock);

// dockicon.c - TD_AddDockIconA host and TOOLDOCK ARexx

BOOL init_dock_icons(struct DockWindow *dock);

VOID free_dock_icons(struct DockWindow *dock);

VOID dock_icons_preserve(struct DockWindow *dock);

VOID dock_icons_restore(struct DockWindow *dock);

VOID handle_pub_port(struct DockWindow *dock);

VOID handle_dockicon_replies(struct DockWindow *dock);

#endif

