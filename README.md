# CPC4PS3

**CPC4PS3** is an **Amstrad CPC / Plus** home computer emulator for **PlayStation 3**, built on the [Caprice32](https://github.com/ColinPitrat/caprice32) emulation core and tailored for TV play with a DualShock 3 / Sixaxis controller, HD frontend, virtual keyboard, universal control presets, and automatic media launching.

---

## Features

- **Full Amstrad Hardware Support**
  - **Models**: Amstrad **CPC 464**, **CPC 664**, **CPC 6128**, and **CPC 6128+ (ASIC)**
  - **RAM Expansions**: `64K`, `128K`, `192K`, `256K`, `512K`, `576K`
  - **Bundled System ROMs**: `cpc464.rom`, `cpc664.rom`, `cpc6128.rom`, `amsdos.rom`, `system.cpr`, and `MF2.rom` included out of the box
- **HD Video & CRT Display Options**
  - `1280x720` video output with **4:3 CRT**, **Pixel 1:1**, and **16:9 Full** aspect ratios
  - **Color CRT** and classic **Green Phosphor** monitor modes
  - Optional **CRT Scanlines** overlay and on-screen **FPS Counter**
- **Cell Audio Engine**
  - `48 kHz` stereo **AY-3-8912 PSG** sound synthesis with adjustable volume (`0% - 100%`)
- **Broad Media & Archive Support**
  - **Disks**: `.dsk` (standard & extended), `.ipf` / `.raw` (via CAPSImage), `.zip` archives (with **multi-disk ZIP** image cycling)
  - **Tapes**: `.cdt`, `.voc` (with rewind & auto-launch)
  - **Cartridges**: `.cpr` (automatically switches hardware to CPC 6128+ mode)
  - **Snapshots**: `.sna` loading and `.sna` export
  - **Writable Disk Saves**: changes written to floppy disks can be saved non-destructively to `/dev_hdd0/CPC/saves` without modifying original ROM/ZIP files
  - **Drive A Directory Catalog Inspector**: parses AMSDOS / CP/M directory sectors on `.dsk` images so you can launch specific programs (`RUN"<FILE>` or `|CPM`) directly from multi-game disks
- **8 Save State Slots per Game**
  - `8` per-game snapshot slots with `160x90` visual preview thumbnails, timestamps, and instant in-game **Quick Save / Quick Load** on **Slot 1** (`L1` / `R1`)
- **Virtual Keyboard (`L2`)**
  - On-screen Amstrad CPC keyboard overlay with **4 visual themes** (`Classic Blue`, `Dark Graphite`, `Cool Slate`, `Amstrad Amber`), **4 opacity levels**, **3 vertical positions** (`Bottom`, `Center`, `Top`), sticky `SHIFT` / `CTRL` / `CAPS LOCK`, and one-click `RUN"`, `CAT`, and `|CPM` macros
- **Universal Control Presets & Per-Button Remapping**
  - Instant one-click global presets (`Joy + Cursors`, `QAOP + Space`, `WASD + Space`, `Cursors + ZX`, `QAZX + Space`) plus individual pad button remapping stored in `/dev_hdd0/CPC/cpc4ps3.ini`

---

## Controls

### In-Game Controls (Default `Joy + Cursors` Profile)

| PS3 Controller | CPC Action (Default) |
| --- | --- |
| **D-Pad / Left Stick** | CPC Joystick Directions + Cursor Keys simultaneously |
| **Cross (`X`)** | Joystick Fire 1 |
| **Circle (`O`)** | Joystick Fire 2 |
| **Square** | CPC `SPACE` |
| **Triangle** | CPC Key `1` *(common game start / 1P key)* |
| **Start** | CPC `RETURN` |
| **R2** | CPC `ESC` |
| **L1** | Quick Save State (`Slot 1`) |
| **R1** | Quick Load State (`Slot 1`) |
| **L2** | Open / Close Amstrad CPC Virtual Keyboard |
| **R3** | Swap Active Joystick Port (`Port A / Joy 0` ↔ `Port B / Joy 1`) |
| **SELECT** or **L3 + R3** | Open / Close Emulator Menu |

### Why Do Some Amstrad CPC Games Require Remapping?

Unlike consoles, the Amstrad CPC was a home computer with a full QWERTY keyboard and **no mandatory controller standard**:

1. **Joystick + Cursor Keys (Default)**: By default, `CPC4PS3` maps the D-Pad / Left Stick to **both** the physical CPC Joystick Port (`Port A` or `Port B`) and the CPC keyboard arrow keys at the same time. This works automatically for most joystick-compatible and cursor-key games.
2. **Keyboard-Only or Port B Games**: Many classic CPC titles (especially older cassette games, ZX Spectrum ports, racers, and multi-player titles) either read **Joystick Port B** (press **`R3`** in-game to toggle ports), expect an intro selection key (`1`, `2`, `J`, `Y`, etc. via **`L2` Virtual Keyboard**), or only accept fixed keyboard schemes such as **`Q/A/O/P + Space`**, **`W/A/S/D + Space`**, or **`Z/X`**.

### Universal Control Presets & Custom Remap

All control settings are **universal across games** and saved in `/dev_hdd0/CPC/cpc4ps3.ini`. You can switch profiles or customize any button in **Settings**:

- **Control Preset (Universal)** — switchable in one click (`Left / Right / X`):
  - **`Joy + Cursors`**: Joystick Port A/B + Cursor Keys, `X`: Fire 1, `O`: Fire 2, `Square`: `SPACE`, `Triangle`: `1`, `Start`: `RETURN`, `R2`: `ESC`
  - **`QAOP + Space`**: `Up`: `Q`, `Down`: `A`, `Left`: `O`, `Right`: `P`, `X`: `SPACE`, `O`: `M`, `Square`: `SPACE`, `Triangle`: `1`, `Start`: `RETURN`, `R2`: `ESC`
  - **`WASD + Space`**: `Up`: `W`, `Down`: `S`, `Left`: `A`, `Right`: `D`, `X`: `SPACE`, `O`: `Z`, `Square`: `X`, `Triangle`: `1`, `Start`: `RETURN`, `R2`: `ESC`
  - **`Cursors + ZX`**: Pure arrow keys, `X`: `SPACE`, `O`: `Z`, `Square`: `X`, `Triangle`: `1`, `Start`: `RETURN`, `R2`: `ESC`
  - **`QAZX + Space`**: `Up`: `Q`, `Down`: `A`, `Left`: `Z`, `Right`: `X`, `X`: `SPACE`, `O`: `RETURN`, `Square`: `SHIFT`, `Triangle`: `2`, `Start`: `RETURN`, `R2`: `ESC`
- **Remap Pad Buttons (Universal)**: Pick any individual CPC key (including `SHIFT` and `CTRL` modifiers) from the Virtual Keyboard for all 10 gameplay buttons (`D-Pad Up/Down/Left/Right`, `Cross`, `Circle`, `Square`, `Triangle`, `Start`, `R2`), or reset all slots back to `Joy + Cursors`.

### Virtual Keyboard Controls (`L2`)

| Button | Action |
| --- | --- |
| **D-Pad** | Navigate keys |
| **Cross (`X`)** | Type selected key / trigger macro (`RUN"`, `CAT`, `|CPM`) |
| **Triangle** | Toggle `SHIFT` |
| **Square** | Press `DEL` |
| **Start** | Press `RETURN` |
| **Select** | Cycle overlay opacity |
| **R2** | Cycle overlay position (`Bottom` / `Center` / `Top`) |
| **L1 / R1** | Cycle visual theme |
| **Circle (`O`)** or **L2** | Close Virtual Keyboard |

---

## Menu Overview

Press **SELECT** (or **L3 + R3**) to open the menu and switch tabs with **L1 / R1**:

1. **LIBRARY** — Scans `/dev_hdd0/CPC/roms` and connected USB drives (`/dev_usb000`–`003`). Shows media metadata and auto-launches selected disks, tapes, cartridges, or snapshots.
2. **BROWSER** — Browse HDD0 or USB folders and choose the mount target (`DRIVE A`, `DRIVE B`, `TAPE`, `CARTRIDGE`, `SNAPSHOT`).
3. **SAVE STATES** — Manage 8 save slots per game with thumbnail previews (`X`: Load, `Square`: Save, `Triangle`: Delete).
4. **SETTINGS** — Configure CPC model, RAM size, monitor mode, aspect ratio, CRT scanlines, audio volume, auto-run, auto-save writable `.dsk`, active joystick port, FPS counter, Virtual Keyboard theme/opacity, universal control presets, per-button pad remap, and core restart.
5. **CPC & DISK** — Inspect Drive A/B/Tape/Cartridge status, run `CAT` / `|CPM`, switch to the next image inside multi-disk `.zip` archives, manually flush writable `.dsk` overlays, export `.sna` snapshots, eject/reset, or launch individual files directly from the Drive A directory catalog.
6. **HELP / EXIT** — Full controller reference, universal mapping tips, session actions (`Resume Game`, `Reset CPC Machine`, `Take Screenshot`, `Save Config & Flush Disks`, **`About`**, `Exit CPC4PS3 to XMB`). Select **About** to view version credits and the Amstrad System ROM copyright notice.

---

## Installation & PS3 Directory Structure

1. Install **`CPC4PS3.pkg`** *(or **`CPC4PS3.gnpdrm.pkg`**)* on a PS3 running CFW or HEN.
2. Copy your Amstrad CPC media files (`.dsk`, `.ipf`, `.raw`, `.cdt`, `.voc`, `.cpr`, `.sna`, `.zip`) either to a FAT32 USB drive or onto the internal PS3 HDD under `/dev_hdd0/CPC/roms/`.

### Package Files on PS3

```text
/dev_hdd0/game/CPC4PS300/
  PARAM.SFO
  ICON0.PNG
  PIC1.PNG
  USRDIR/
    EBOOT.BIN
    font.ttf
    rom/                  (cpc464.rom, cpc664.rom, cpc6128.rom, amsdos.rom, system.cpr, MF2.rom)
    resources/            (keymap_us.map)
```

### Runtime User & Data Folders

```text
/dev_hdd0/CPC/
  cpc4ps3.ini             Global emulator settings & universal pad mapping (padmap0..padmap9)
  cpc4ps3.log             Runtime diagnostic log
  saves/                  Non-destructive writable .dsk save overlays
  states/                 Per-game save states (.st) and thumbnails (.th)
  screenshots/            Captured BMP screenshots (cpc_<ticks>.bmp)
  roms/
    README.txt
    disks/                .dsk, .ipf, .raw, .zip
    tapes/                .cdt, .voc
    carts/                .cpr
    snapshots/            .sna
```

> **Multi-Disk ZIP Archives**: Mount any `.zip` containing multiple `.dsk` images directly. To swap disks in-game, open the menu (**SELECT**), go to **`CPC & DISK`**, and select **`Next Disk Image in ZIP`**.

---

## Building from Source

### Prerequisites

- **WSL / Linux** environment with a **PSL1GHT / PS3DEV** toolchain (tested targeting Cell PPU `mcpu=cell` with `-m64`)
- `portlibs/ppu` libraries: `SDL2`, `SDL2_ttf`, `freetype`, `libpng`, `zlib`
- PS3 SDK packaging tools in `$PS3DEV/bin`: `ppu-g++`, `ppu-strip`, `sprxlinker`, `make_self_npdrm`, `sfo`, `pkg`, `package_finalize`

By default, `Makefile`, `build.sh`, and `package.sh` look for `PS3DEV` at `/mnt/d/ps3dev-sdk` unless `PS3DEV` is already exported in your environment.

### Compile & Package

```bash
export PS3DEV=/path/to/ps3dev-sdk   # Optional if installed at /mnt/d/ps3dev-sdk
cd /path/to/CPC4PS3

chmod +x build.sh package.sh
JOBS=4 ./build.sh
./package.sh
```

Outputs:
- `cpc_ps3.elf` — Unstripped Cell PPU ELF executable
- `dist/CPC4PS3.pkg` — Standard retail/debug PS3 install package
- `dist/CPC4PS3.gnpdrm.pkg` — Finalized Geohot/NPDRM-signed package (`Content ID: UP0001-CPC4PS300_00-0000000000000000`)

---

## Repository Structure

```text
.
├── Makefile              Cross-compilation rules for Caprice32, CAPSImage & PS3 frontend
├── build.sh              Portable wrapper to build cpc_ps3.elf
├── package.sh            Strips ELF, signs EBOOT.BIN, creates PARAM.SFO & builds .pkg installers
├── sfo.xml               PS3 Title & PARAM.SFO metadata (CPC4PS300 v01.00)
├── assets/               XMB ICON0.PNG, PIC1.PNG, and bundled UI TTF font
├── resources/            Caprice32 keyboard mapping (keymap_us.map)
├── rom/                  System ROMs (CPC 464/664/6128, AMSDOS, MF2, 6128+ system.cpr)
├── tools/                Helper asset generator (make_assets.ps1) and WSL dev scripts
└── src/
    ├── cap32/            Caprice32 Amstrad CPC emulation core
    ├── capsimg/          CAPSImage IPF / CT-RAW floppy image decoder
    └── ps3/              PS3 UI, video scaler, Cell Audio, input presets/remap & prebuilt render object
```

---

## Credits & Licenses

- **Caprice32** — Amstrad CPC emulation core by Ulrich Doewich, Colin Pitrat, and contributors ([GNU General Public License v2](LICENSE)).
- **CAPSImage / IPF Support** — Copyright (c) KryoFlux Products & Services Limited / Software Preservation Society, distributed under the [CAPSImage Non-Commercial Source Code License](src/capsimg/LICENCE.txt).
- **CPC4PS3** — PlayStation 3 port, HD menu frontend, Cell Audio backend, virtual keyboard, and universal controls by **thheroGAC**.

### Amstrad System ROM Copyright Notice

> AMSTRAD CPC ROMS ARE COPYRIGHTED BY AMSTRAD AND LOCOMOTIVE SOFTWARE.  
> AMSTRAD HAS KINDLY GRANTED PERMISSION FOR THE REDISTRIBUTION OF ITS COPYRIGHTED MATERIAL FOR USE WITH EMULATORS. ALL ORIGINAL COPYRIGHT NOTICES ARE PRESERVED. ALL RIGHTS REMAIN WITH THEIR RESPECTIVE COPYRIGHT HOLDERS.
