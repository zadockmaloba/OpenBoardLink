An open, low-cost ARM SWD programmer/debugger built around an STM32F103C8 running Zephyr RTOS. Rev 1, October 2026.

## Hardware

| Block | Part |
|---|---|
| Controller | STM32F103C8Tx (LQFP-48), HSE crystal |
| USB-UART bridge | CH340N, wired to USART1 (PA9/PA10) |
| Onboard flash | W25Q32JVSS, 32 Mbit SPI NOR on SPI1 |
| Power | USB-C (USB 2.0, CC pull-downs) → ferrite bead → AP2112K-3.3 LDO |
| Protection | USBLC6-2P6 ESD protection on the USB data lines |
| Target connector | 1x6 header (J2): `NRST`, `SWCLK`, `SWDIO`, `SWO`, `VTREF`, `GND` |
| User controls | BOOT0 slide switch, reset button, power LED |

Notes:
- The SWCLK line has a 47R series resistor and a 1k pull-down. SWDIO has a 1k pull-up and a 330R series resistor.
- `VTREF` is routed to the target header so the target's I/O voltage can be sensed.
- The STM32 USB pins (PA11/PA12) are unconnected. Host communication goes through the CH340N.

Schematic: `USB2SWD.kicad_sch` (KiCad 10).

## Firmware

Zephyr 4.3.0 with a custom board definition (`openboardlink`, SoC `stm32f103xb`).


Build:

```
cd firmware
west build -p always -b openboardlink/stm32f103xb
```

Requires west, CMake and the Zephyr SDK 0.17.x.

## Snapshots

### Rev-1 (v0.3) 28-Oct-2026
![][rev-1.0.3]

[rev-1.0.3]: ./snapshots/REV-1-v0.3.png