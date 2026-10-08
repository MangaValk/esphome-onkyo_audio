#pragma once

#include <string>

#include "esphome/components/media_player/media_player.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace onkyo_audio {

/// Onkyo receiver over the RS-232 port, using the ISCP serial protocol.
///
/// Messages look like "!1PWR01" and end with 0x1A (EOF), sometimes followed by CR/LF.
/// The receiver also sends these messages by itself when something changes (remote, volume knob),
/// so the component never waits for an answer: loop() parses whatever arrives, commands are just
/// written, and a slow poll (update()) only catches anything that was missed.
class OnkyoAudioMediaPlayer : public media_player::MediaPlayer, public PollingComponent, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  media_player::MediaPlayerTraits get_traits() override;
  bool is_muted() const override { return this->muted_; }

  void set_max_volume(uint8_t max_volume) { this->max_volume_ = max_volume; }

 protected:
  void control(const media_player::MediaPlayerCall &call) override;

  /// Write "!1<command><parameter>\r".
  void send_(const char *command, const char *parameter);
  void send_volume_(uint8_t level);
  void query_all_();

  void handle_message_(const std::string &message);
  void on_power_(bool on);
  void on_volume_(uint8_t level);
  void on_mute_(bool muted);

  uint8_t max_volume_{78};
  std::string rx_buffer_;

  bool muted_{false};

  // Volume requested from Home Assistant, sent at most every VOLUME_SEND_INTERVAL ms
  optional<uint8_t> pending_volume_{};
  uint32_t last_volume_sent_{0};

  // Health: count polls that got no reply at all
  bool awaiting_reply_{false};
  uint8_t missed_polls_{0};
};

}  // namespace onkyo_audio
}  // namespace esphome
