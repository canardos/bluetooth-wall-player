# Bluetooth Wall Audio Player

![Bluetooth audio player](bt-wall-player.jpg?raw=true)

## A low power, wall mounted, Bluetooth audio player, clock, and environmental monitor

> [!Warning]
> This is personal project. It was built and works without issue for my purposes. It is made public here in case some part of it is useful to others, but **neither the hardware nor firmware have been rigorously tested**.

**Features:**

- Media/volume controls
- Song metadata display
- Time, temperature, pressure, humidity display on idle
- Ultra low power sleep with proximity sensing wakeup
- Differential analog and S/PDIF digital audio output

**This repo contains the firmware and PCB/enclosure design.**

**See [custom electronics projects at duk.io](https://www.duk.io/blog/electronics-projects/) for full project details.**

## Build

The project uses the [PlatformIO](https://platformio.org/) dependency management/build system. If you have PlatformIO installed, executing the `run` command in the root folder *should* 🙏 be all that is required for a successful build. The `platformio.ini` file contains the required configuration  and PlatformIO will download the required platform dependencies and toolchain.

**1. Clone repo and download submodules**

```shell
> git clone https://github.com/canardos/bluetooth-wall-player.git
> cd bluetooth-wall-player
> git submodule update --init
```

**2. Build firmware (PlatformIO must be installed)**

```shell
> pio run

```

**3. Upload to device**

Similarly, use `pio run --target upload` to upload the firmware to the device. You'll likely need to update the upload settings in `platformio.ini` to reflect your settings, or use your own upload tool.

```shell
> pio run --target upload

```

**4. [optional] Create project files for IDE**

If you want to view/edit the project in your favorite IDE, use PlatformIO to generate the appropriate setup/workspace files and import the project:

```shell
> pio project init --ide [atom|clion|codeblocks|eclipse|emacs|netbeans|qtcreator|sublimetext|vim|visualstudio|vscode]
```

See the [PlatformIO documentation](https://docs.platformio.org) for further details.

#### Using an alternate build system

**Platform dependencies**

If you wish to use a different build system, you will need:

| | |
|-|-|
| ST STM32 CMSIS headers | Peripheral/register definitions |
| CMSIS headers/source | CMSIS HAL functionality - SysTick_Config, NVIC_xxx etc.|
| CMSIS Cortex-M3 system source | `SystemInit()`/`SystemCoreClockUpdate()` implementation and `SystemCoreClock`|
| Startup source file | ASM file containing the startup code and vector table |
| Linker script | Tells the linker how to setup everything in flash/SRAM |

All of these are available in the [STM32 SDK](https://www.st.com/en/development-tools/stm32-software-development-tools.html) available from ST Micro.

**Compiling / linking**

`platformio.ini` lists the required compiler flags. Be sure to use `--specs=nano.specs` and `--specs=nosys.specs` when linking using your own tools.

#### Compiler

The project was built with `gcc-arm-none-eabi 9.2.1` It should work with any later GCC version and possibly with other compilers. The code contains some GCC flags and some C99 VLAs.

#### Target requirements

| | |
|-|-|
|MCU  | STM32F1xx |
|Flash| >42kb     |
|SRAM | >8kb      |

## Dependencies (firmware)

**CMSIS**

PlatformIO should automatically include/link the correct headers/sources from the STM32 SDK. Version 9.0.0 was used in production.

**Libpekin**

[Libpekin](https://github.com/canardos/libpekin) is a collection of shared MCU-related code resulting from this and other projects. It's included as a Git submodule in the `lib` folder. PlatformIO will include the `lib` subfolders automatically.

## Hardware

#### PCB

The PCB was designed with KiCad v.5, but the files have been upgraded to v.7 and are located in `/hardware/pcb`. Gerber output for the first revision is located in `/hardware/pcb/plots_rev_a`.

> [!Important]
> [Several fixes have been made to the design](https://www.duk.io/blog/electronics-projects/bt-player/bluetooth-audio-player-conclusion/#problems-and-screwups), which are implemented in the latest schematic/pcb files, but are not present in the revision A plot output.

See the [KiCad documentation](https://docs.kicad.org/) for information on installing and using KiCad.

**Custom symbols and footprints required by the board are included as a Git submodule at `/hardware/pcb/kicad-mycustom-lib`, and are included in the KiCad project as a project-specific library.**

#### Enclosure

The [enclosure design](http://www.duk.io/blog/electronics-projects/bt-player/bluetooth-audio-player-mechanical-design/#mechanical-design) includes a [laser-cut wooden facade](http://www.duk.io/blog/electronics-projects/bt-player/bluetooth-audio-player-mechanical-design/#laser-etched-wood-facade). The CAD files for the design are located in `/hardware/enclosure/laser_cut_facade`.

A Sketchup model I used to model the mechanical constraints of the design is included in `/hardware/enclosure`.

## Folder structure

```
BTWallPlayer
|
+--artwork           | SVG sources for animation
|  |
|  +--render         | bitmap output frames
|
+--hardware
|  |
|  +--pcb           | KiCad schematic and board design files
|  +--enclosure     | Enclosure Sketchup model and laser cutting
|                   | design files
|
+--lib
|  |
|  +--libpekin       | Libpekin platform independent code
|  +--libpekin_stm32 | Libpekin STM32 specific code
|
+--src
|  |
|  +--data           | Image and font data
|  +--devices        | Hardware device drivers
|
|--test              | No tests in use

```

## License

This software is available under the [MIT license](https://opensource.org/license/MIT).
