#pragma once

// #include "esphome/components/media_player/media_player.h"
// #include "esphome/core/component.h"
// #include "esphome/core/helpers.h"

#include "esphome/components/uart/uart.h"

// #define BAUD_RATE 9600
// #define SERIAL_CONFIG SERIAL_8N1
#define SERIAL_TIME_OUT 1000
//#define SERIAL_TIME_OUT 10

#define DEV_EC 0x1A // Device End Character

// #define VOL_MIN 0x00
// #define VOL_MAX 0x4A // 74
// #define VOL_CALL 0x19 // 25

// #define POWER_OFF 0x00
// #define POWER_ON 0x01

namespace esphome {
namespace onkyo {

// enum OnkyoState : uint8_t {
//   ONKYO_STATE_STOPPED = 0,
//   ONKYO_STATE_STARTING,
//   ONKYO_STATE_RUNNING,
//   ONKYO_STATE_STOPPING,
// };

class OnkyoDevice {
 public:  
  void setup();
  
  uart::UARTComponent *uart_;
  void set_uart(uart::UARTComponent *uart) { this->uart_ = uart; }

//   bool is_muted() const override { return this->muted_; }
  
//   void control(const media_player::MediaPlayerCall &call) override;

//   void mute_();
//   void unmute_();
//   void set_volume_(float volume, bool publish = true);
//   void refreshPowerState();
  
//   bool muted_{false};
//   float unmuted_volume_{0};
  
//   bool is_announcement_{false};
  
//   // Onkyo
//   float oldVolume = 0;
//   double volumeDelay;
  uint8_t get_volume();
  int get_power();
  int cmd(String cmd);

  String readStringUntil(char terminator);

  void setVolume(uint8_t vol);
  void power(uint8_t pow);
};

}  // namespace onkyo
}  // namespace esphome

