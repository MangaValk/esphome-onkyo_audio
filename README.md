# esphome-onkyo_audio
esphome component to control Onkyo AV trough serial port

Based on the work of https://github.com/zarpli/onkyo

This is still a work in progress and will have bugs

esphome example yaml:
```
esphome:
  name: onkyo-remote
  friendly_name: Onkyo_remote
  platform: ESP8266
  board: d1_mini

external_components:
  - source:
      type: local
      path: custom_components

uart:
  id: uart_bus
  tx_pin: GPIO01
  rx_pin: GPIO03
  baud_rate: 9600
  parity: NONE
  stop_bits: 1

# Enable logging
logger:
  hardware_uart: uart1

captive_portal:
    
web_server:
  port: 80

media_player:
  - platform: onkyo_audio
    type: external
    name: "onkyo media player"
    uart: uart_bus

```
