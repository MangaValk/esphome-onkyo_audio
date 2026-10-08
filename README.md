# esphome-onkyo_audio

ESPHome external component that controls an Onkyo receiver through its RS-232 serial port.
It shows up in Home Assistant as a media player with **power on/off, volume and mute**.

Based on the protocol work of [zarpli/onkyo](https://github.com/zarpli/onkyo).

## Features

- Power on/off (real `turn_on` / `turn_off`; play/pause/stop still work for older automations)
- Volume: set, step up/down
- Mute / unmute
- Changes made on the receiver itself (remote control, volume knob, standby) show up in Home Assistant right away
- Never blocks ESPHome: commands are written without waiting, and incoming messages are parsed as they arrive
- Warns in the log and in Home Assistant when the receiver stops answering

## Hardware

The Onkyo serial port uses **RS-232 levels (±12 V)**. Do not connect it directly to an ESP:
use an RS-232 to TTL converter such as a MAX3232 module.

| Converter | ESP8266 (D1 mini) |
|-----------|-------------------|
| TX        | RX (GPIO3)        |
| RX        | TX (GPIO1)        |
| VCC       | 3V3               |
| GND       | GND               |

The receiver talks at 9600 baud, 8N1.

On the ESP8266, GPIO1/GPIO3 are the hardware UART, so move the logger to the second UART
(`hardware_uart: uart1`) as in the example below.

## Installation

```yaml
external_components:
  - source: github://MangaValk/esphome-onkyo_audio
    components: [onkyo_audio]

uart:
  id: uart_bus
  tx_pin: GPIO1
  rx_pin: GPIO3
  baud_rate: 9600

logger:
  hardware_uart: uart1   # keep the log off the receiver's serial port

media_player:
  - platform: onkyo_audio
    name: "Onkyo receiver"
```

To use a local copy instead, put the `components/onkyo_audio` folder in e.g. `custom_components/`
next to your YAML and use:

```yaml
external_components:
  - source:
      type: local
      path: custom_components
```

## Configuration variables

- **name** (Required): Name of the media player in Home Assistant.
- **max_volume** (Optional, default `78`): Highest volume step of your receiver (the `MVL` value
  that matches 100 % in Home Assistant). Many models use 80 (`0x50`) or 100 (`0x64`); check the
  ISCP documentation of your model or turn the volume all the way up and look at the log.
- **update_interval** (Optional, default `30s`): How often power, volume and mute are queried as a
  safety net. The receiver reports changes by itself, so this does not need to be short.
- **uart_id** (Optional): The UART to use, if you have more than one.
- All other options from [Media Player](https://esphome.io/components/media_player/).

## How it works

The receiver speaks ISCP over serial: messages like `!1PWR01` (power on), `!1MVL1E` (volume 30)
or `!1AMT00` (mute off), terminated by `0x1A`. It sends these by itself whenever something changes.

- `loop()` collects incoming bytes and handles each complete message (`PWR`, `MVL`, `AMT`).
- Commands from Home Assistant are written immediately; the receiver answers with the new state,
  which is what updates Home Assistant.
- While the volume slider is dragged, at most one volume command is sent every 150 ms, and the
  receiver's echoes of older steps are ignored for a moment so the slider does not jump back.
- Every `update_interval` the component asks for `PWR`, `MVL` and `AMT`. If three of these polls
  get no answer at all, the component logs a warning and sets the ESPHome warning status.

Use `logger: level: VERBOSE` (or `logs: onkyo_audio: VERBOSE`) to see every message sent and received.

## Upgrading from the first version

The first version is kept as the tag [`v0.1-original`](../../tree/v0.1-original).

- Existing configurations keep working: `uart: uart_bus` and `type: internal/external` are still
  accepted (`uart_id` is the standard name now; `type` is ignored).
- If you used the files at the root of this repository, use the `components/onkyo_audio` folder now.
- Power is now a real on/off in Home Assistant. Automations that used play/pause for power still work,
  but `media_player.turn_on` / `turn_off` is the better choice.
- The state is now `on` / `off` instead of `playing` / `paused`. Update automations or templates
  that check for `playing`.
- Volume mapping uses `max_volume` (default 78, the value the first version used).

## Troubleshooting

- **"No response from the receiver"**: check TX/RX (swap them if needed), the RS-232 converter,
  and that `baud_rate` is 9600. Some models only answer over serial when "Network/RS-232 standby"
  (or similar) is enabled in the setup menu.
- **Volume in Home Assistant does not match the receiver**: set `max_volume` for your model.
