#pragma once

#include "esphome/components/media_player/media_player.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include "esphome/components/uart/uart.h"

#define BAUD_RATE 9600
#define SERIAL_CONFIG SERIAL_8N1
#define SERIAL_TIME_OUT 1000
//#define SERIAL_TIME_OUT 10

#define DEV_EC 0x1A // Device End Character

#define VOL_MIN 0x00
#define VOL_MAX 0x4A // 74
#define VOL_CALL 0x19 // 25

#define POWER_OFF 0x00
#define POWER_ON 0x01

namespace esphome {
namespace onkyo_audio {

enum OnkyoState : uint8_t {
  ONKYO_STATE_STOPPED = 0,
  ONKYO_STATE_STARTING,
  ONKYO_STATE_RUNNING,
  ONKYO_STATE_STOPPING,
};

class OnkyoAudioMediaPlayer : public Component, public media_player::MediaPlayer {
 public:  
  void setup() override;
  float get_setup_priority() const override { return esphome::setup_priority::LATE; }

  void loop() override;

  void dump_config() override;

  media_player::MediaPlayerTraits get_traits() override;

  bool is_muted() const override { return this->muted_; }
  
  void set_uart(uart::UARTComponent *uart) { this->uart_ = uart; }

 protected:
  OnkyoState onkyo_state_{ONKYO_STATE_STOPPED};
  
  void control(const media_player::MediaPlayerCall &call) override;

  void mute_();
  void unmute_();
  void set_volume_(float volume, bool publish = true);
  
  bool muted_{false};
  float unmuted_volume_{0};
  
  bool is_announcement_{false};
  
  uart::UARTComponent *uart_;
  
  // Onkyo
  float oldVolume = 0;
  double volumeDelay;
  String readStringUntil(char terminator);
  int cmd(String cmd);
  uint8_t get_volume();
  int get_power();
  void setVolume(uint8_t vol);
  void power(uint8_t pow);
};

}  // namespace onkyo_audio
}  // namespace esphome

