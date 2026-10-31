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

#ifndef __PREFS_H__
#define __PREFS_H__

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/libraries.h>
#include <exec/ports.h>
#include <utility/hooks.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>

#include <clib/window_protos.h>
#include <clib/layout_protos.h>
#include <clib/label_protos.h>
#include <clib/chooser_protos.h>
#include <clib/checkbox_protos.h>
#include <clib/slider_protos.h>
#include <clib/integer_protos.h>
#include <clib/string_protos.h>
#include <clib/listbrowser_protos.h>
#include <clib/reaction_lib_protos.h>

#include "dockbot.h"
#include "dockbot_protos.h"
#include "dockbot_pragmas.h"
#include "dock_cat.h"

#define APP_NAME "Dock"

#define MIN_PADDING 0
#define MAX_PADDING 8
#define MIN_EYES 1
#define MAX_EYES 4

#define MAX_EDIT_SETTINGS 16
#define MAX_SETTING_KEY 32
#define MAX_SETTING_VAL 256
#define MAX_PATH_LEN 1024

#define EDIT_TMP "T:Dock-edit.tmp"

#define CLASS_BUTTON   "DockButton"
#define CLASS_CLOCK    "DockClock"
#define CLASS_EYES     "Eyes"
#define CLASS_SWITCHER "Switcher"
#define CLASS_TRASH    "DockTrash"
#define CLASS_OPEN     "DockOpen"

#define S_NAME     "name"
#define S_PATH     "path"
#define S_BRUSH    "brush"
#define S_START    "start"
#define S_ARGS     "args"
#define S_CON      "console"
#define S_HOTKEY   "key"
#define S_DEFAULT  "default"
#define S_FORMAT   "format"
#define S_NUMEYES  "numberOfEyes"
#define S_SWPOS    "position"

#define ST_WB 0
#define ST_SH 1

enum {
    GID_POSITION = 1,
    GID_ALIGNMENT,
    GID_SHOW_LABELS,
    GID_SHOW_BORDERS,
    GID_PADDING,
    GID_PADDING_INT,
    GID_BG_BRUSH,
    GID_BTN_BG_BRUSH,
    GID_GADGETS,
    GID_BTN_NEW,
    GID_BTN_EDIT,
    GID_BTN_DELETE,
    GID_BTN_UP,
    GID_BTN_DOWN,
    GID_BTN_SAVE,
    GID_BTN_USE,
    GID_BTN_TEST,
    GID_BTN_CANCEL,
    GID_LAST
};

enum {
    NID_LIST = 1,
    NID_OK,
    NID_CANCEL,
    NID_DESC
};

enum {
    EID_OK = 1,
    EID_CANCEL,
    EID_NAME,
    EID_PATH,
    EID_PATH_BTN,
    EID_BRUSH,
    EID_BRUSH_BTN,
    EID_ARGS,
    EID_HOTKEY,
    EID_START,
    EID_CONSOLE,
    EID_FORMAT,
    EID_NUMEYES,
    EID_NUMEYES_INT,
    EID_WINPOS,
    EID_GENERIC = 100
};

enum {
    MID_ABOUT = 1,
    MID_QUIT
};

enum {
    EK_NONE,
    EK_BUTTON,
    EK_CLOCK,
    EK_EYES,
    EK_SWITCHER,
    EK_GENERIC
};

struct EditSetting {
    UBYTE key[MAX_SETTING_KEY];
    UBYTE value[MAX_SETTING_VAL];
};

struct DockPrefs {
    struct DockConfig cfg;

    Object *mainObj;
    struct Window *mainWin;
    struct Gadget *gads[GID_LAST];

    struct List classes;
    struct List *gadNodes;
    struct List *classNodes;
    struct List *posLabels;
    struct List *alignLabels;
    struct List *startLabels;
    struct List *swPosLabels;

    struct MsgPort *idcmpPort;
    struct MsgPort *appPort;
    struct Hook appHook;
    struct NewMenu newMenu[6];

    Object *newObj;
    struct Window *newWin;
    struct Gadget *newList;
    struct Gadget *newDesc;

    Object *editObj;
    struct Window *editWin;
    struct Gadget *editGads[EID_GENERIC + MAX_EDIT_SETTINGS];
    struct DgNode *editGadget;
    BOOL editGadgetIsNew;
    UWORD editKind;
    UWORD editCount;
    struct EditSetting editSet[MAX_EDIT_SETTINGS];
    UBYTE editDump[512];
};

/* gadgets.c */
VOID dock_gadget_get_info(Object *obj, STRPTR *name, STRPTR *version,
    STRPTR *description, STRPTR *copyright);
VOID dock_gadget_get_label(Object *obj, STRPTR *label);
BOOL dock_gadget_can_edit(Object *obj);
VOID dock_gadget_init_button(Object *obj, STRPTR name, STRPTR path);
VOID dock_gadget_read_settings(Object *obj, struct DockSettings *settings);
VOID dock_gadget_write_settings(Object *obj, struct DockSettings *settings);

/* classes.c */
BOOL open_reaction_classes(VOID);
VOID close_reaction_classes(VOID);
VOID show_open_lib_err(STRPTR name, UWORD version);
struct Library *open_lib(STRPTR name, UWORD version);
VOID close_lib(STRPTR name, struct Library *lib);

/* settings.c */
struct DgNode *add_dock_gadget(struct DockPrefs *prefs, STRPTR name);
VOID remove_dock_gadget(struct DgNode *dg);
VOID init_config(struct DockPrefs *prefs);
BOOL load_config(struct DockPrefs *prefs);
BOOL save_config(struct DockPrefs *prefs, BOOL permanent);
VOID free_config(struct DockPrefs *prefs);
VOID revert_config(VOID);

/* gadget_list.c */
VOID gadget_selected(struct DockPrefs *prefs, LONG index);
VOID update_gadget_list(struct DockPrefs *prefs);
struct DgNode *get_selected_gadget(struct DockPrefs *prefs);
VOID move_gadget_up(struct DockPrefs *prefs);
VOID move_gadget_down(struct DockPrefs *prefs);
VOID delete_gadget(struct DockPrefs *prefs);
LONG get_list_index(struct DockPrefs *prefs);
VOID set_list_index(struct DockPrefs *prefs, LONG index);
VOID set_main_disabled(struct DockPrefs *prefs, ULONG id, BOOL disabled);
VOID free_gadget_nodes(struct DockPrefs *prefs);

/* new_gadget.c */
VOID open_new_gadget_dialog(struct DockPrefs *prefs);
VOID close_new_gadget_dialog(struct DockPrefs *prefs);
VOID class_selected(struct DockPrefs *prefs);
VOID create_new_gadget(struct DockPrefs *prefs);
VOID handle_new_gadget_input(struct DockPrefs *prefs);
VOID add_dropped_icon(struct DockPrefs *prefs, struct AppMessage *msg);

/* editor.c */
VOID edit_gadget(struct DockPrefs *prefs, struct DgNode *dg);
VOID close_edit_dialog(struct DockPrefs *prefs, BOOL apply);
VOID handle_edit_input(struct DockPrefs *prefs);
BOOL export_gadget_settings(Object *obj, struct EditSetting *set, UWORD *count);
BOOL import_gadget_settings(Object *obj, struct EditSetting *set, UWORD count);
STRPTR setting_get(struct EditSetting *set, UWORD count, STRPTR key);
VOID setting_set(struct EditSetting *set, UWORD *count, STRPTR key, STRPTR value);

#endif
