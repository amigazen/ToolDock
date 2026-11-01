# Changelog

ToolDock is a fork of DockBot. DockBot versions below are the upstream
history. ToolDock versioning starts at 2.0.

## [2.0] - 2026-11-01 - ToolDock

Fork of DockBot by the amigazen project (PowerTools). Rebranded as
ToolDock. MIT license retained. Requires AmigaOS 3.0+ and IconLib 46.4.

### Added

- SAS/C 6.58 SMakefiles for 68000, 68020 and 68040, following the
  ToolKit layout, with CatComp catalogs instead of FlexCat
- `tooldock.library` with `ToolDockBase` / `TD_*` names so it can
  coexist with an installed `dockbot.library`
- Workbench launches via `OpenWorkbenchObjectA` (no WBRun / parm.library)
- DockTrash plugin, using WBTrash when available, with Empty Trash
- DockOpen plugin: smart open of the default tool, icon from
  SYS:Utilities/MultiView when possible, WBInfo support
- DockExchange plugin: commodities list with Show / Hide / Quit
- DockSpacer plugin: bevel line, horizontal or vertical with the dock
- Reaction ToolDockPrefs alongside the Triton Dock prefs
- DockButton app lifecycle from the program icon: `TD_CX`, `TD_RX`,
  `TD_RXOPEN`, `TD_LAUNCH`, `TD_OPEN`, plus `TD_CMD*` popup commands
- Heartbeat, click-to-front, and Show / Hide / Quit through
  popupmenu.library (soft-fails if the library is missing)
- Right-click popup on icons and the handle, including inactive-window
  RMB via the commodities broker on MENUUP
- Confirming Remove on every item
- Show Labels in layout and draw
- Built-in drag-rearrange of items
- Workbench drop after a beat: a vacant slot follows the pointer, drop 
  fills it, leaving the window restores the dock
- Dropped disks, drawers and projects become shortcuts that use the
  object's own icon (volume `Disk.info` when needed)
- Drop onto a disk or drawer icon copies through Workbench's copy hook
- Unique-instance drops via `TD_RXOPEN`
- ENV:ToolDock.prefs notify reload
- About prefers requester.class over EasyRequest
- French, Italian and Spanish catalogs for the new plugins

### Changed

- About shows ToolDock version and copyright only (no per-plugin credits)
- Drawers and disks dropped on the dock add a shortcut instead of being
  skipped as directories

### Fixed

- Enforcer hit in the clock module
- Iconify icon
- ToolDockPrefs crash on a valid prefs file (padding, NULL gadget,
  CR/LF parser)
- Dock crash when prefs were saved while the dock was open
- CxBase multiply-defined at link
- `ChangeWindowBox` on a NULL window
- Prefs notify no longer uses `NRF_WAIT_REPLY`, so closing prefs cannot
  stall the dock
- Duplicate dock source DisplayBeeps instead of adding a second item
- Trashcan (`WBGARBAGE`) drops are ignored
- Workbench drag icon stays visible (no `ChangeWindowBox` during the drag)
- Quit crash: `tooldock.library` is expunged before dos.library is closed,
  and the debug stream is dropped first

### Removed

- WBRun and parm.library launch path
- Hand-rolled intuition libcall include on the dock handle
- Edit mode menu (drag-rearrange and Remove cover it)
- File-only drop filter
- AppWindow drop-zone hook (crashed as soon as the pointer entered the dock)

## DockBot (historical)

Upstream by Andrew Kennan. Last DockBot release was 1.11.

### [1.11] - 2020-10

- Network monitor (DockNet) for Roadshow, contributed by flype
- Requested locale.library version is 38 (OS 3.0), thanks to Asmund Nordel
- Padding around gadgets

### [1.10] - 2020-05

- Rendering bugs and a memory leak
- All rendering moved to an off-screen bitmap
- Missing config file is tolerated

### [1.9] - 2020-04

- Edit Mode: gadgets can be rearranged, added and removed from the dock
- Memory leaks in the prefs tool
- Italian translation by Samir Hawamdeh

### [1.8] - 2020-03

- Button gadget can use any image, not only the file's icon
- Background image for the dock

### [1.7] - 2020-02

- Eyes crash when the dock is horizontal
- Keyboard response in Switcher
- Option to hide icon borders

### [1.6] - 2020-01

- Window Switcher and Eyes gadgets
- Clicking the Clock gadget launches Time prefs
- French translation by flype
- Memory-usage bugs

### [1.5] - 2019-11

- screennotify.library support, thanks to flype
- Strings moved to translatable catalogs
- Spanish catalog, thanks to Gulliver
- Memory Meter styling
- Clock gadget handles linefeeds in the format string

### [1.2] - 2019-08

- Horizontal alignment of the main window
- Cancel in prefs reverts to the last saved config

### [1.1] - 2019-06

- Installer issues that prevented the dock from running
- First public DockBot release (icons, clock, memory meter, Triton
  prefs, dockbot.library gadget classes)
