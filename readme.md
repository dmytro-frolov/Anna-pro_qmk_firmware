# Quantum Mechanical Keyboard Firmware

## Repository Note

This repository is maintained as a personal GitHub mirror for custom Anne Pro / QMK changes.

It is not intended to be treated as the canonical upstream project. For the original source history and ongoing development, use:

* [OpenAnnePro/qmk_firmware](https://github.com/OpenAnnePro/qmk_firmware)

If this repository diverges from upstream, the local custom state in this repo is considered authoritative for its own history and published snapshots.

---

## Anne Pro 2 Custom Features & Improvements

This fork introduces host-controlled RGB integration with **OpenRGB** and several low-level reliability enhancements over standard OpenAnnePro QMK firmware:

### 1. OpenRGB QMK Protocol (v0xC) Support
- **Full Host RGB Control**: Native support for OpenRGB via standard 64-byte Raw HID (`Usage Page: 0xFF60`, `Usage ID: 0x61`).
- **Direct & Static Modes**: Stream realtime per-key colors and full-matrix effects directly from OpenRGB (Mode 1: Direct, Mode 2: Static).
- **All 70 Key Positions Mapped**: Complete LED coordinate geometry and keycode mapping for the 60% ANSI matrix.

### 2. Dual-MCU Lighting Backend & UART Optimizations
- **Universal Shine Compatibility**: Direct RGB streams are dispatched using both `CMD_LED_COLOR_SET_ROW` (0x32) and mask-based `CMD_LED_MASK_SET_ROW` (0x11), ensuring full compatibility with both new and legacy `AnnePro2-Shine` firmware versions.
- **Expanded UART Buffer**: Increased serial buffer size (`SERIAL_BUFFERS_SIZE = 128`) in `halconf.h` to prevent serial queue congestion and packet loss when pushing 62-byte RGB rows across the Main-to-Shine UART bridge (`SD0` @ 115200 baud).
- **Matrix Power Management**: Automatic matrix power assertion (`annepro2LedEnable`) ensuring the LED hardware power rail (`LINE_LED_PWR`) remains energized during host streaming and boot.

### 3. Non-Intrusive Local Control Preservation
- **Hardware Hotkey Priority**: Pressing any onboard lighting hotkey (`FN + 9`, `FN + 0`, `FN + -`, `FN + +`, brightness, speed) instantly yields host control back to the keyboard's internal animated profiles without needing a reboot.

### 4. Modern Toolchain Compatibility
- **ARM GCC 14+ Ready**: Compiler flag tuning (`-Wno-misleading-indentation`, `-DPORT_IGNORE_GCC_VERSION_CHECK=1`) for building cleanly with modern Arm GNU toolchains.

### Quick Build & Flash
```bash
# Build C18 (HT32F52352) or C15 (HT32F52342)
make annepro2/c18:default

# Flash to keyboard (put in IAP mode by holding ESC while plugging in USB)
annepro2_tools --boot annepro2_c18_default.bin
```

---

[![Current Version](https://img.shields.io/github/tag/qmk/qmk_firmware.svg)](https://github.com/qmk/qmk_firmware/tags)
[![Build Status](https://travis-ci.org/qmk/qmk_firmware.svg?branch=master)](https://travis-ci.org/qmk/qmk_firmware)
[![Discord](https://img.shields.io/discord/440868230475677696.svg)](https://discord.gg/Uq7gcHh)
[![Docs Status](https://img.shields.io/badge/docs-ready-orange.svg)](https://docs.qmk.fm)
[![GitHub contributors](https://img.shields.io/github/contributors/qmk/qmk_firmware.svg)](https://github.com/qmk/qmk_firmware/pulse/monthly)
[![GitHub forks](https://img.shields.io/github/forks/qmk/qmk_firmware.svg?style=social&label=Fork)](https://github.com/qmk/qmk_firmware/)

This is a keyboard firmware based on the [tmk\_keyboard firmware](https://github.com/tmk/tmk_keyboard) with some useful features for Atmel AVR and ARM controllers, and more specifically, the [OLKB product line](https://olkb.com), the [ErgoDox EZ](https://ergodox-ez.com) keyboard, and the [Clueboard product line](https://clueboard.co).

## Documentation

* [See the official documentation on docs.qmk.fm](https://docs.qmk.fm)

The docs are powered by [Docsify](https://docsify.js.org/) and hosted on [GitHub](/docs/). They are also viewable offline; see [Previewing the Documentation](https://docs.qmk.fm/#/contributing?id=previewing-the-documentation) for more details.

You can request changes by making a fork and opening a [pull request](https://github.com/qmk/qmk_firmware/pulls), or by clicking the "Edit this page" link at the bottom of any page.

## Supported Keyboards

* [Planck](/keyboards/planck/)
* [Preonic](/keyboards/preonic/)
* [ErgoDox EZ](/keyboards/ergodox_ez/)
* [Clueboard](/keyboards/clueboard/)
* [Cluepad](/keyboards/clueboard/17/)
* [Atreus](/keyboards/atreus/)

The project also includes community support for [lots of other keyboards](/keyboards/).

## Maintainers

QMK is developed and maintained by Jack Humbert of OLKB with contributions from the community, and of course, [Hasu](https://github.com/tmk). The OLKB product firmwares are maintained by [Jack Humbert](https://github.com/jackhumbert), the Ergodox EZ by [ZSA Technology Labs](https://github.com/zsa), the Clueboard by [Zach White](https://github.com/skullydazed), and the Atreus by [Phil Hagelberg](https://github.com/technomancy).

## Official Website

[qmk.fm](https://qmk.fm) is the official website of QMK, where you can find links to this page, the documentation, and the keyboards supported by QMK.
