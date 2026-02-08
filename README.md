# NetIfManager

A MUI-based network interface manager for AmigaOS 3.x.

![AmigaOS](https://img.shields.io/badge/AmigaOS-3.x-blue) ![MUI](https://img.shields.io/badge/MUI-3.8+-green) ![SAS/C](https://img.shields.io/badge/SAS%2FC-6.x-orange)

## Overview

NetIfManager provides a graphical interface to manage network interfaces on AmigaOS. It displays available interfaces from `SYS:Storage/NetInterfaces` (stored/inactive) and `DEVS:NetInterfaces` (active), allowing you to easily activate, edit or remove network interfaces.

## Features

- **Dual-panel interface** — stored interfaces on the left, active interfaces on the right
- **Add interface** — copy an interface (and its icon) from storage to DEVS: with a single click or double-click
- **Remove interface** — delete an interface (and its icon) from DEVS:
- **Edit interface** — edit an active interface via `C:EditInterface` (button or double-click on the right list)
- **Device identification** — displays the underlying network device name and version for each selected interface
- **Smart button states** — Delete and Edit buttons are automatically disabled when the active interface list is empty; Edit is also disabled if `C:EditInterface` is not installed
- **Refresh** — rescan both directories at any time
- **Keyboard shortcuts** — Amiga+R (Refresh), Amiga+Q (Quit), Amiga+? (About)
- **About window** — with author information and a link to the GitHub repository

## Screenshots

*Coming soon*

## Requirements

- AmigaOS 3.0 or later
- MUI 3.8+ (muimaster.library v19)
- A TCP/IP stack with network interfaces in `SYS:Storage/NetInterfaces` and `DEVS:NetInterfaces`
- Optional: `C:EditInterface` for interface editing functionality

## Building from source

### Prerequisites

- SAS/C 6.x
- MUI developer headers installed in `MUI:Developer/C/Include`

### Compilation

```
sc LINK NOSTKCHK DATA=NEAR INCLUDEDIR=MUI:Developer/C/Include NetIfManager.c PNAME=NetIfManager
```

**Note:** Do not compile from `RAM:` — SAS/C cannot create intermediate files on the RAM disk. Use a real filesystem volume (DH0:, DH1:, etc.).

## Installation

1. Copy `NetIfManager` to a location of your choice (e.g. `SYS:Tools/`)
2. Ensure MUI 3.8+ is installed
3. Optionally install `C:EditInterface` for interface editing support
4. Run from Shell or Workbench

## Usage

- Select an interface in either list to see its network device and version
- Click **Refresh** or press Amiga+R to rescan both directories
- Click **Add >>** (or double-click in the left list) to copy an interface to `DEVS:NetInterfaces`
- Click **<< Delete** to remove an interface from `DEVS:NetInterfaces`
- Click **Edit** (or double-click in the right list) to edit an active interface with `C:EditInterface`
- Access **About** from the Project menu

## File structure

| File | Description |
|---|---|
| `NetIfManager.c` | Main source code |
| `SMakefile` | Makefile for SAS/C (`smake`) |
| `README.md` | This file |

## Changelog

### v1.2 (08.02.2026)
- Quit button centered in the window
- Fixed empty first line in both lists (removed unused MUIA_List_Title)

### v1.1 (07.02.2026)
- Added Edit button to edit active interfaces via `C:EditInterface`
- Smart button states: Delete and Edit are disabled when the active list is empty
- Edit is disabled if `C:EditInterface` is not found on the system
- Double-click on right list now opens the editor instead of deleting
- Refresh button moved next to Add in the left panel
- About window with author information and GitHub link

### v1.0 (07.02.2026)
- Initial release
- Dual-panel interface for managing network interfaces
- Add and Delete functionality
- Device name and version display
- .info files filtered from lists

## Author

**Renaud Schweingruber**

- Email: renaud.schweingruber@protonmail.com
- GitHub: [TuKo1982](https://github.com/TuKo1982)

## License

This project is open source. See the [LICENSE](LICENSE) file for details.
