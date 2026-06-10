# Wish Mai: Play Maimai DX on iPad with Raspberry Pi Zero W and Physical Buttons

This project enables you to play maimai DX, a world-famous arcade rhythm game, on
your iPad. Maimai is well known for its physical buttons, which provide a gameplay 
experience that is significantly different from that of touchscreen-only rhythm games.

Due to inconvenience for game machine access at many places as well as at-home practice 
needs, the game community for maimai DX has developed 
[AstroDX](https://github.com/2394425147/astrodx), a mobile app whose screen layout 
resembles maimai DX. It supports gamepad inputs from v2.0.0 Release Candidate 6.

This project connects physical buttons to a Raspberry Pi Zero W and configures the Pi 
to emulate a DualShock 4 (PS4) controller, allowing button presses to be transmitted to 
an iPad. It also supports a fancy OLED addon to visualize your button presses.

This project is built on the [Circle](https://github.com/rsta2/circle) bare-metal C++
framework.

## Artifacts

1. Video demos for game playing and OLED display is available at [GoogleDrive](https://drive.google.com/drive/u/2/folders/1GlM-d_Bunc-BRCLeduyQuy7GR8Z5YI_3).
2. A writeup for the project summary including background and implementation details
   is available in the `demo/` directory.

## Usage

### Required hardware

| Item | Details |
|------|---------|
| Raspberry Pi Zero / Zero W | Target board |
| iPad | USB host running AstroDX v2.0.0 RC6+ |
| USB OTG adapter | Connects Pi micro-USB port to iPad USB-C port |
| SSD1306 128×64 OLED (optional) | I2C: SDA=GPIO2, SCL=GPIO3, address 0x3C |
| 8× momentary buttons | Wired to GPIO pins below (active-low, pulled up) |
| MicroSD card | FAT32, for boot files and kernel image |

### GPIO button wiring
```
  #7 ------- #0
 /             \
#6             #1
|               |
#5             #2
 \             /
  #4 ------- #3
```
| GPIO | DS4 button      | Maimai position |
|------|-----------------|-----------------|
| 4    | North (Triangle)| #0              |
| 5    | East (Circle)   | #1              |
| 6    | South (Cross)   | #2              |
| 16   | West (Square)   | #3              |
| 17   | D-pad Up        | #4              |
| 20   | D-pad Right     | #5              |
| 21   | D-pad Down      | #6              |
| 22   | D-pad Left      | #7              |

### Building

Requires an ARM bare-metal GCC toolchain (`arm-none-eabi-gcc`).

```bash
# 1. Build Circle libraries (one-time)
cd circle
./makeall RASPPI=1
cd ..

# 2. Build kernel image
make RASPPI=1
```

Output: `kernel.img`

### Flashing

Copy these files to the root of a FAT32 microSD card:

```
firmware/bootcode.bin
firmware/start_cd.elf
firmware/fixup_cd.dat
firmware/config.txt
kernel.img
```

### Hardware setup

1. Flash the SD card (see Usage below) and insert it into the Pi Zero W.
2. (Optional) Connect a 128×64 SSD1306 OLED to I2C (SDA=GPIO2, SCL=GPIO3).
3. Wire 8 momentary buttons to the GPIO pins listed above.
4. Connect the Pi's micro-USB OTG port to the iPad via a USB OTG adapter.
5. Test buttons presses on an online gamepad tester and play in AstroDX!

## Major parts implemented

### DS4 controller emulation with button GPIO integration

- `src/usbds4gadget.cpp`: Handlings for various DS4 USB descriptors and reports required by the USB host.
- `src/usbds4gadgetendpoint.cpp`: DS4 (at EP1 IN) endpoint with DMA double-buffering and spinlock for thread-safe report updates.
- `src/gpiocontroller.h`: Reads and maps physical buttons, with D-pad button conflict resolution.

### Interrupt-based asynchronous I2C and OLED display

- `src/i2cmasterasync.cpp`: Custom interrupt-driven I2C master so OLED writes do not block button reads or USB polling.
- `src/oled.cpp`: SSD1306 128×64 driver that renders live button state as an octagon visualization.

## Resources

- AstroDX: https://github.com/2394425147/astrodx
- Circle bare-metal framework: https://github.com/rsta2/circle
- DS4 USB protocol: https://www.psdevwiki.com/ps4/DS4-USB
- Linux DS4 driver reference: https://github.com/torvalds/linux/blob/master/drivers/hid/hid-playstation.c
- Stanford CS 240LX course: https://github.com/dddrrreee/cs240lx-26spr
