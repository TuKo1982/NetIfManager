# NetIfManager

A MUI-based network interface manager for AmigaOS 3.x.

![AmigaOS](https://img.shields.io/badge/AmigaOS-3.x-blue) ![MUI](https://img.shields.io/badge/MUI-3.8+-green) ![SAS/C](https://img.shields.io/badge/SAS%2FC-6.x-orange)

## Overview

NetIfManager provides a graphical interface to manage network interfaces on AmigaOS. It displays available interfaces from `SYS:Storage/NetInterfaces` (stored/inactive) and `DEVS:NetInterfaces` (active), allowing you to easily activate or remove network interfaces.

## Features

- **Dual-panel interface** — stored interfaces on the left, active interfaces on the right
- **Add interface** — copy an interface (and its icon) from storage to DEVS: with a single click or double-click
- **Remove interface** — delete an interface (and its icon) from DEVS: with a single click or double-click
- **Device identification** — displays the underlying network device name and version for each selected interface
- **Refresh** — rescan both directories at any time
- **Keyboard shortcuts** — Amiga+R (Refresh), Amiga+Q (Quit), Amiga+? (About)

## Screenshots

*Coming soon*

## Requirements

- AmigaOS 3.0 or later
- MUI 3.8+ (muimaster.library v19)
- A TCP/IP stack with network interfaces in `SYS:Storage/NetInterfaces` and `DEVS:NetInterfaces`

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
3. Run from Shell or Workbench

## Usage

- Select an interface in either list to see its network device and version
- Click **Add >>** (or double-click in the left list) to copy an interface to `DEVS:NetInterfaces`
- Click **<< Delete** (or double-click in the right list) to remove an interface from `DEVS:NetInterfaces`
- Use **Refresh** to rescan both directories
- Access **About** from the Project menu

## File structure

| File | Description |
|---|---|
| `NetIfManager.c` | Main source code |
| `SMakefile` | Makefile for SAS/C (`smake`) |
| `README.md` | This file |

## Author

**Renaud Schweingruber**

- Email: renaud.schweingruber@protonmail.com
- GitHub: [TuKo1982](https://github.com/TuKo1982)

## License

This project is open source. See the [LICENSE](LICENSE) file for details.
