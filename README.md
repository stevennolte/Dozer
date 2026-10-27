# Dozer

ESP32-S3 firmware for a toy bulldozer (MX1508 mini dual H-bridge, often sold as “L298N”). Control it from a tablet browser with [`web/index.html`](web/index.html).

## Wiring

This module has no ENA/ENB pins. Motor power and logic share **+ / −**. Keep motor voltage **≤ 10 V**. Common ground with the ESP32.

| MX1508 | ESP32-S3 | Function |
| --- | --- | --- |
| IN1 | GPIO 14 | Left forward (PWM) |
| IN2 | GPIO 13 | Left reverse (PWM) |
| IN3 | GPIO 12 | Right forward (PWM) |
| IN4 | GPIO 11 | Right reverse (PWM) |
| − | GND | Common ground |
| + | Toy battery + | Motors + logic |

Pins are in `firmware/include/pins.h`.

## BLE

Device name: `Dozer`

| | UUID |
| --- | --- |
| Service | `d02e0001-1212-efde-1523-785feabcd123` |
| Drive (write, no response) | `d02e0002-1212-efde-1523-785feabcd123` |
| Stop (write, no response) | `d02e0003-1212-efde-1523-785feabcd123` |
| Pins (write, no response) | `d02e0004-1212-efde-1523-785feabcd123` |

Drive payload: 2 bytes, signed `int8` left then right, `-127..127`. Pins payload: 4 bytes `uint8` duty for IN1–IN4 (`0..255`). Empty write to Stop coasts immediately.

No Drive write for 300 ms (or disconnect) coasts both tracks. Arcade mix (throttle + turn) is done in the HTML page.

## Firmware

[PlatformIO](https://platformio.org/) for the **ESP32-S3-DevKitC-1 N8R2** (8 MB Quad flash, 2 MB Quad PSRAM):

```
cd firmware
pio run -t upload
```

## WiFi debug / OTA

The board starts a WiFi AP:

- SSID `Dozer`
- password `dozer123`
- http://192.168.4.1/ status, reboot, and `.bin` upload
- http://192.168.4.1/status.json

## Tablet (HTML)

Use **Chrome or Edge** on the tablet (Web Bluetooth). Safari does not support it.

Web Bluetooth only works on `https://` or `http://localhost`, not `file://`.

1. Copy [`web/index.html`](web/index.html) to the tablet, or open it from this repo.
2. Serve it from localhost (any simple HTTP server app, or `python -m http.server`) and open that URL in Chrome.
3. Tap **Connect**, pick **Dozer**, then drive with the joystick.
4. Release the stick or tap **E-STOP** to coast.

If the page is hosted on HTTPS (for example GitHub Pages), you can open that URL directly.
