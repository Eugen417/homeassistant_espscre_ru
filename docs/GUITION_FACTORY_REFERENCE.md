# Guition 4848S040: independent manufacturer test

This test distinguishes our ESPHome/UI from the original panel driving.
A matching demo was found in the manufacturer package offered by SpotPear:

- [Vendor with download link](https://spotpear.com/ESP32-S3-4-inch-LCD-Touchscreen-Display-SHT20-Temperature-Humidity-480x480-RS485-Relay-GC9503-ST7701-FT6336U-GT911-86-TVbox/forum-answer/357.html)
- [Original archive](https://cdn.static.spotpear.com/uploads/picture/learn/ESP32/ESP32-S3-4inch/4.0inch_ESP32-4848S040.zip)
- [Community write-up for building with LVGL 8/9](https://github.com/paulhamsh/LVGL_GUITION)

In the archive:

- `1-Demo/Demo_Arduino/1_2_4.0_LvglWidgets/4.0_LvglWidgets/`: source code.
- `1-Demo/Demo_Arduino/Libraries/Arduino_GFX-master/`: bundled panel driver.
- `8-Burn operation/Burn files/4.0_LvglWidgets.bin`: combined firmware,
  652640 bytes, flash address 0. Bootloader at 0, partition table at 0x8000,
  application at 0x10000.
- SHA256 of that demo: `9662c193fc52407bba2ed1462c582d40148f7d8eedfc806db4e8fff7685abba1`.

Use only the Widgets demo for the display test. The other bundled
examples are relay and music applications. The Widgets source only uses the
panel, GT911, and backlight, and has no HA/Wi-Fi configuration.

## Notable reference settings

The original source uses ArduinoGFX, `st7701_type1_init_operations`,
a 16 MHz pixel clock, RGB pins R=11/12/13/14/0, G=8/20/3/46/9/10,
B=4/5/6/7/15. Horizontal front/pulse/back=10/8/50; vertical=10/8/20.
The panel mode is 0x3A=0x60, RGB666 on a 16-bit wired bus, with 0xCD=0x00.
Rotation 0 writes C7=0 in register bank 0x10 and MADCTL=0 in bank 0.
The backlight is on continuously via GPIO38, without PWM.

The gamma and voltage values resemble ESPHome's ST7701S base, but the order,
porches, and rotation commands differ. That's grounds for comparison,
not proof that a specific change fixes banding.

## Backup and recovery

First verify the chip/MAC and the actual USB port. A full flash backup is
safest. For this specific demo, only sectors within the first
1 MiB are overwritten. An exact backup of that range is enough to
undo that overwrite, as long as the demo doesn't write to other flash regions.
Also keep the device's own firmware/YAML. Flash backups contain personal
keys: keep them local only, never in Git.

Don't use erase-all. After the test, restore the backed-up sectors and verify the
ESPHome identity, version, preferences, and HA pairing. Record the physical result
separately: a successful upload says nothing about the visible image.


## Test performed on September 12, 2026

- Fetched the manufacturer archive and read the Widgets source plus bundled driver.
- Backed up the first 1 MiB (every sector this demo overwrites) over USB;
  921600/460800 baud caused communication problems, 115200 baud worked.
- Wrote the original Widgets binary unchanged at address 0; hash verification passed.
- The USB boot log reports `LVGL Widgets Demo` and `Setup done`, then GT911 polling.
  The original demo also reports GPIO errors during setup; physical
  confirmation therefore remains necessary. The owner then confirmed: the manufacturer demo is clean, no banding.
  That demonstrates good image output on this physical panel. This doesn't
  rule out every hardware/power factor, but it makes our own driving the
  first line of investigation.


## Native ESPHome candidate 0.1.3

The standard `st7701s` component gets the same 16 MHz clock and horizontal
10/8/50 and vertical 10/8/20 timing as the manufacturer demo. SPI uses MODE0;
color order RGB, no inversion or hardware mirroring. After the built-in
initialization, the 16-bit bus configuration CD=00 and the standard register bank
are explicitly selected, followed by 3A=60 and the wait after sleep-out. The GT911 stays
in the same unmirrored orientation. The ESPHome RGB driver isn't copied
or patched; the earlier extra SDK cache/restart settings have been removed.

The owner reports that this candidate is cleaner but still shows a few fixed vertical
bands. So this is not a finished solution. The manufacturer demo remains the
clean reference. Differences include ESP-IDF/LVGL versions, framebuffer
and bounce buffer usage, Wi-Fi/API, PWM, and the extra initialization steps of the
standard driver (including a software reset). "Native" here means the built-in
ESPHome driver, not a byte-for-byte identical execution to the manufacturer demo.

After that, only the Guition interface was given a light design: background F7F7F7,
white cards, border DDDDDD, dark text, and blue accents. The detail cards also
use light controls. Geometry, pagination, touch, HA layout, and
saved settings are preserved. This style change is not a banding fix.

### Light interface: software check

The Python suite (33 tests), seven C++ regression tests, and the package generator check
pass. The first light firmware was installed over OTA; ten page checks
plus fifty overlay switches pass without HA actions. The screen capture shows the
new light cards with the existing HA layout. Long names then got
fixed line heights with truncation, so the title and status don't overlap.
An internal screen capture doesn't prove glitch-free physical panel output.

The final light version (config_hash 0x0a70a194) was installed over OTA.
Both Easy Setup and both manual profiles build successfully. The owner
then confirmed: 'Style good, no bands visible'. This is physical
acceptance of the current image, not an endurance test after prolonged standby, and not
proof that a driver change alone fixed the root cause.
