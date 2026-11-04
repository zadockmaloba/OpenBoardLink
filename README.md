An open, low-cost ARM SWD programmer/debugger built around an STM32F103C8 running Zephyr RTOS. Rev 1, October 2026.

## Hardware

Schematic: `./hardware/USB2SWD.kicad_sch` (KiCad 10).

## Firmware

**NOTE:** 
- CH340P/FTDI drivers required for serial communication.
    - See: https://sparks.gogo.co.nz/ch340.html?srsltid=AYCX9vJ7uKf6C0Zw0mtyxMH3N8wv1fY7HUsu3qdjR-UEwpCyX_hEoPqb

- STM32CubeProgrammer is currently required to flash firmware to the programmer.
    - See: https://www.st.com/en/development-tools/stm32cubeprog.html

Zephyr 4.3.0 with a custom board definition (`openboardlink`, SoC `stm32f103xb`).


Build:

```
cd firmware
west build -p always -b openboardlink/stm32f103xb
```

Requires west, CMake and the Zephyr SDK 0.17.x.

## Snapshots

### Rev-1 (v0.5) 4-Nov-2026
![][rev-1.0.5]

[rev-1.0.5]: ./snapshots/REV-1-v0.5.png

### Rev-1 (v0.4) 1-Nov-2026
![][rev-1.0.4]

[rev-1.0.4]: ./snapshots/REV-1-v0.4.png
