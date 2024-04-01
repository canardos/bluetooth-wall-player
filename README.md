# Bluetooth Wall Audio Player

![Bluetooth audio player](bt-wall-player.jpg?raw=true)

**A low power, wall mounted, Bluetooth audio player, clock, and environmental monitor**

**This repo contains the firmware and PCB/enclosure design**

Features:

- Media/volume controls
- Song metadata display
- Time, temperature, pressure, humidity display on idle
- Ultra low power sleep with proximity sensing wakeup
- Differential analog and S/PDIF digital audio output

**Full project details at [duk.io](https://blog.duk.io/custom-electronics-projects/)**

## Build

The project uses the [PlatformIO](https://platformio.org/) dependency management/build system. If you have PlatformIO installed, executing the `run` command in the root folder should be all that is required for a successful build. The `platformio.ini` file contains the required configuration  and PlatformIO will download the required platform dependencies and toolchain.

```
> pio run

```

Similarly, use `pio run --target upload` to upload the firmware to the device. You'll likely need to update the upload settings in `platformio.ini` to reflect your settings, or use your own upload tool.

```
> pio run --target upload

```

If you want to view/edit the project in your favorite IDE, use PlatformIO to generate the appropriate setup/workspace files and import the project:

```
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

All of these are available in the ST STM32 SDK available from ST Micro.

**Compiling / linking**

`platformio.ini` lists the required compiler flags. Be sure to use `--specs=nano.specs` and `--specs=nosys.specs` when linking.

#### Compiler

The project was built with `gcc-arm-none-eabi 9.2.1` It should work with any later GCC version and possibly with other compilers. The code contains some GCC flags and some C99 VLAs.

#### Target requirements

| | |
|-|-|
|MCU  | STM32F1xx |
|Flash| >42kb              |
|SRAM | >8kb               |

## Dependencies

**CMSIS**

PlatformIO should automatically include/link the correct headers/sources from the STM32 SDK.

**Libpekin**

[Libpekin](https://github.com/canardos/libpekin) is a collection of shared MCU related code resulting from this and other projects. It's included as a Git submodule in the `lib` folder. PlatformIO will include the `lib` subfolders automatically.

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
|  +--enclosure     | Enclosure laser cutting design files
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