#include "onkyo_audio_media_player.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace onkyo_audio {

static const char *const TAG = "onkyo_audio";

static const uint8_t ISCP_EOF = 0x1A;
static const size_t MAX_MESSAGE_LENGTH = 32;
// While the volume slider is dragged, Home Assistant sends many values; forward at most one per interval.
static const uint32_t VOLUME_SEND_INTERVAL = 150;
// Ignore volume reports this long after sending one, so echoes of older steps don't move the slider back.
static const uint32_t VOLUME_SETTLE_TIME = 600;
// Report a problem after this many polls without any message from the receiver.
static const uint8_t MAX_MISSED_POLLS = 3;

static bool parse_hex_byte(const std::string &text, uint8_t *out) {
  if (text.size() != 2 || !std::isxdigit((unsigned char) text[0]) || !std::isxdigit((unsigned char) text[1]))
    return false;
  *out = (uint8_t) std::strtoul(text.c_str(), nullptr, 16);
  return true;
}

void OnkyoAudioMediaPlayer::setup() {
  this->state = media_player::MEDIA_PLAYER_STATE_NONE;
  this->query_all_();
}

void OnkyoAudioMediaPlayer::dump_config() {
  ESP_LOGCONFIG(TAG,
                "Onkyo receiver (ISCP over serial):\n"
                "  Max volume: %u\n"
                "  Poll interval: %u ms",
                this->max_volume_, (unsigned) this->get_update_interval());
  this->check_uart_settings(9600);
}

media_player::MediaPlayerTraits OnkyoAudioMediaPlayer::get_traits() {
  auto traits = media_player::MediaPlayerTraits();
  traits.set_supports_turn_off_on(true);
  traits.add_feature_flags(media_player::MediaPlayerEntityFeature::VOLUME_STEP);
  // Only power, volume and mute are supported; no media playback or announcements.
  traits.clear_feature_flags(media_player::MediaPlayerEntityFeature::PLAY_MEDIA |
                             media_player::MediaPlayerEntityFeature::BROWSE_MEDIA |
                             media_player::MediaPlayerEntityFeature::MEDIA_ANNOUNCE |
                             media_player::MediaPlayerEntityFeature::STOP);
  return traits;
}

// --- Receiving ---------------------------------------------------------------

void OnkyoAudioMediaPlayer::loop() {
  uint8_t byte;
  while (this->available() && this->read_byte(&byte)) {
    if (byte == ISCP_EOF || byte == '\r' || byte == '\n') {
      if (!this->rx_buffer_.empty())
        this->handle_message_(this->rx_buffer_);
      this->rx_buffer_.clear();
    } else if (this->rx_buffer_.size() < MAX_MESSAGE_LENGTH) {
      this->rx_buffer_ += (char) byte;
    } else {
      this->rx_buffer_.clear();  // garbage; resync on the next terminator
    }
  }

  if (this->pending_volume_.has_value() && millis() - this->last_volume_sent_ >= VOLUME_SEND_INTERVAL) {
    this->send_volume_(*this->pending_volume_);
    this->pending_volume_.reset();
  }
}

void OnkyoAudioMediaPlayer::handle_message_(const std::string &message) {
  // "!1" + 3-letter command + parameter, e.g. "!1MVL1E"
  if (message.size() < 6 || message.compare(0, 2, "!1") != 0) {
    ESP_LOGV(TAG, "Ignoring: %s", message.c_str());
    return;
  }
  ESP_LOGV(TAG, "Received: %s", message.c_str());

  this->awaiting_reply_ = false;
  if (this->missed_polls_ > 0 || this->status_has_warning()) {
    this->missed_polls_ = 0;
    this->status_clear_warning();
  }

  const std::string command = message.substr(2, 3);
  const std::string parameter = message.substr(5);
  uint8_t value;

  if (command == "PWR" && parse_hex_byte(parameter, &value)) {
    this->on_power_(value != 0);
  } else if (command == "MVL" && parse_hex_byte(parameter, &value)) {
    this->on_volume_(value);
  } else if (command == "AMT" && parse_hex_byte(parameter, &value)) {
    this->on_mute_(value != 0);
  }
  // Anything else (input selection, "N/A" while in standby, ...) is ignored.
}

void OnkyoAudioMediaPlayer::on_power_(bool on) {
  auto new_state = on ? media_player::MEDIA_PLAYER_STATE_ON : media_player::MEDIA_PLAYER_STATE_OFF;
  if (this->state == new_state)
    return;
  bool turned_on = on && this->state == media_player::MEDIA_PLAYER_STATE_OFF;
  this->state = new_state;
  ESP_LOGD(TAG, "Power %s", on ? "on" : "off");
  this->publish_state();
  if (turned_on) {
    // Volume and mute may have changed while it was off
    this->send_("MVL", "QSTN");
    this->send_("AMT", "QSTN");
  }
}

void OnkyoAudioMediaPlayer::on_volume_(uint8_t level) {
  if (this->pending_volume_.has_value() || millis() - this->last_volume_sent_ < VOLUME_SETTLE_TIME)
    return;  // a newer value from Home Assistant is on its way
  if (level > this->max_volume_)
    level = this->max_volume_;
  float volume = (float) level / this->max_volume_;
  if (std::abs(volume - this->volume) < 0.001f)
    return;
  this->volume = volume;
  ESP_LOGD(TAG, "Volume %u (%.0f%%)", level, volume * 100.0f);
  this->publish_state();
}

void OnkyoAudioMediaPlayer::on_mute_(bool muted) {
  if (this->muted_ == muted)
    return;
  this->muted_ = muted;
  ESP_LOGD(TAG, "Mute %s", muted ? "on" : "off");
  this->publish_state();
}

// --- Sending -----------------------------------------------------------------

void OnkyoAudioMediaPlayer::send_(const char *command, const char *parameter) {
  char buffer[24];
  int length = snprintf(buffer, sizeof(buffer), "!1%s%s\r", command, parameter);
  if (length <= 0 || length >= (int) sizeof(buffer))
    return;
  ESP_LOGV(TAG, "Sending: !1%s%s", command, parameter);
  this->write_array((const uint8_t *) buffer, length);
}

void OnkyoAudioMediaPlayer::send_volume_(uint8_t level) {
  char parameter[3];
  snprintf(parameter, sizeof(parameter), "%02X", level);
  this->send_("MVL", parameter);
  this->last_volume_sent_ = millis();
}

void OnkyoAudioMediaPlayer::query_all_() {
  this->send_("PWR", "QSTN");
  this->send_("MVL", "QSTN");
  this->send_("AMT", "QSTN");
}

void OnkyoAudioMediaPlayer::update() {
  // Safety net only: the receiver reports changes by itself.
  if (this->awaiting_reply_ && this->missed_polls_ < 255)
    this->missed_polls_++;
  if (this->missed_polls_ >= MAX_MISSED_POLLS && !this->status_has_warning()) {
    ESP_LOGW(TAG, "No response from the receiver; check the serial cable and baud rate (9600)");
    this->status_set_warning();
  }
  this->awaiting_reply_ = true;
  this->query_all_();
}

void OnkyoAudioMediaPlayer::control(const media_player::MediaPlayerCall &call) {
  if (call.get_volume().has_value()) {
    float volume = clamp(*call.get_volume(), 0.0f, 1.0f);
    this->volume = volume;  // show the new value right away; the receiver confirms it
    this->pending_volume_ = (uint8_t) std::lround(volume * this->max_volume_);
    this->publish_state();
  }

  if (!call.get_command().has_value())
    return;

  switch (*call.get_command()) {
    case media_player::MEDIA_PLAYER_COMMAND_TURN_ON:
    case media_player::MEDIA_PLAYER_COMMAND_PLAY:  // first version used play/pause for power
      this->send_("PWR", "01");
      break;
    case media_player::MEDIA_PLAYER_COMMAND_TURN_OFF:
    case media_player::MEDIA_PLAYER_COMMAND_PAUSE:
    case media_player::MEDIA_PLAYER_COMMAND_STOP:
      this->send_("PWR", "00");
      break;
    case media_player::MEDIA_PLAYER_COMMAND_TOGGLE:
      this->send_("PWR", this->state == media_player::MEDIA_PLAYER_STATE_ON ? "00" : "01");
      break;
    case media_player::MEDIA_PLAYER_COMMAND_MUTE:
      this->send_("AMT", "01");
      break;
    case media_player::MEDIA_PLAYER_COMMAND_UNMUTE:
      this->send_("AMT", "00");
      break;
    case media_player::MEDIA_PLAYER_COMMAND_VOLUME_UP:
      this->send_("MVL", "UP");
      break;
    case media_player::MEDIA_PLAYER_COMMAND_VOLUME_DOWN:
      this->send_("MVL", "DOWN");
      break;
    default:
      ESP_LOGW(TAG, "Command not supported: %s", media_player::media_player_command_to_string(*call.get_command()));
      break;
  }
}

}  // namespace onkyo_audio
}  // namespace esphome
