#include "esphome/core/log.h"
#include "onkyo_audio_media_player.h"

namespace esphome {
namespace onkyo_audio {

static const char *const TAG = "audio";

void OnkyoAudioMediaPlayer::control(const media_player::MediaPlayerCall &call) {
  media_player::MediaPlayerState play_state =
      media_player::MEDIA_PLAYER_STATE_PLAYING;
  if (call.get_announcement().has_value()) {
    play_state = call.get_announcement().value()
                     ? media_player::MEDIA_PLAYER_STATE_ANNOUNCING
                     : media_player::MEDIA_PLAYER_STATE_PLAYING;
  }

  if (play_state == media_player::MEDIA_PLAYER_STATE_ANNOUNCING) {
    this->is_announcement_ = true;
  }

  if (call.get_volume().has_value()) {
    this->volume = call.get_volume().value();
    this->set_volume_(volume);
    // this->unmute_();
  }

  if (call.get_command().has_value()) {
    auto command = *call.get_command();
    if (command == media_player::MEDIA_PLAYER_COMMAND_PLAY) {
      this->power(1);
      this->state = media_player::MEDIA_PLAYER_STATE_PLAYING;
      ESP_LOGD("onkyo power turn on", "%i", get_power());
    } else if (command == media_player::MEDIA_PLAYER_COMMAND_STOP) {
      this->power(0);
      this->state = media_player::MEDIA_PLAYER_STATE_PAUSED;
      ESP_LOGD("onkyo power turn off", "%i", get_power());
    } else if (command == media_player::MEDIA_PLAYER_COMMAND_PAUSE) {
      this->power(0);
      this->state = media_player::MEDIA_PLAYER_STATE_PAUSED;
    }
  }

  /*if (call.get_command().has_value()) {
    switch (call.get_command().value()) {
      case media_player::MEDIA_PLAYER_COMMAND_MUTE:
        this->mute_();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_UNMUTE:
        this->unmute_();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_UP: {
        float new_volume = this->volume + 0.01f;
        if (new_volume > 1.0f)
          new_volume = 1.0f;
        this->set_volume_(new_volume);
        this->unmute_();
        break;
      }
      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_DOWN: {
        float new_volume = this->volume - 0.01f;
        if (new_volume < 0.0f)
          new_volume = 0.0f;
        this->set_volume_(new_volume);
        this->unmute_();
        break;
      }
      default:
        break;
    }

    if (this->onkyo_state_ != ONKYO_STATE_RUNNING) {
      return;
    }

    switch (call.get_command().value()) {
      case media_player::MEDIA_PLAYER_COMMAND_PLAY:
        this->state = play_state;
        break;
      case media_player::MEDIA_PLAYER_COMMAND_PAUSE:
        this->state = media_player::MEDIA_PLAYER_STATE_PAUSED;
        break;
      case media_player::MEDIA_PLAYER_COMMAND_STOP:
        break;
      case media_player::MEDIA_PLAYER_COMMAND_TOGGLE:
        break;
      default:
        break;
    }
  }*/
  this->publish_state();
}

void OnkyoAudioMediaPlayer::mute_() {
  this->set_volume_(0);
  this->muted_ = true;
}
void OnkyoAudioMediaPlayer::unmute_() {
  this->set_volume_(this->volume);
  this->muted_ = false;
}
void OnkyoAudioMediaPlayer::set_volume_(float volume, bool publish) {
  this->volume = volume;
  if (publish) this->volume = volume;
}

void OnkyoAudioMediaPlayer::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Audio...");
  this->state = media_player::MEDIA_PLAYER_STATE_IDLE;

  float volume = this->get_volume();
  if(volume == -1)
	  volume = 0;
  this->volume = remap<float, uint8_t>(volume, 0, 78, 0.0f, 1.0f);
  this->oldVolume = this->volume;
  this->muted_ = false;
}

void OnkyoAudioMediaPlayer::loop() {
  switch (this->onkyo_state_) {
    case ONKYO_STATE_STARTING:
      // this->start_();
      break;
    case ONKYO_STATE_RUNNING:
      // this->play_();
      break;
    case ONKYO_STATE_STOPPING:
      // this->stop_();
      break;
    case ONKYO_STATE_STOPPED:
      break;
  }

  //float newVolume = remap<float, uint8_t>(this->get_volume(), 0, 78, 0.0f, 1.0f);
  float newVolume = this->get_volume();
  // set physical volume.
  if (this->oldVolume != this->volume) {
    this->oldVolume = this->volume;
    this->setVolume(remap<uint8_t, float>(this->volume, 0.0f, 1.0f, 0, 78));
	
	this->volumeDelay = millis();
  }
  // update volume state from physical state.
  else if(this->volume != newVolume && newVolume != -1)
  {
	  //delay(100);
	  // check if volume hasnt changed.
	  //if(newVolume == remap<float, uint8_t>(this->get_volume(), 0, 78, 0.0f, 1.0f))
	  if(newVolume == this->get_volume())
	  {
		  if (newVolume >= 0 && newVolume <= 78 && (millis() - this->volumeDelay) > 1000 )
		  {
			  this->volume = remap<float, uint8_t>(newVolume, 0, 78, 0.0f, 1.0f);
			  this->oldVolume = this->volume;
			  this->publish_state();
			  ESP_LOGD("onkyo volume updated from physical state", "%f", this->volume);
			  
			  this->volumeDelay = millis();
		  }
	  }
  }

  int power = get_power();
  //if(millis() % 1000 < 50)
  //{
  //	ESP_LOGD("onkyo power", "%i", power);
  //	ESP_LOGD("onkyo volume", "%i", get_volume());
  //}
  
  // update power state from physical power state.
  if(power == 1 && (this->state == media_player::MEDIA_PLAYER_STATE_PAUSED))
  {
	  this->state = media_player::MEDIA_PLAYER_STATE_PLAYING;
      this->publish_state();
	  ESP_LOGD("onkyo power updated from physical state", "%i", power);
  }
  else if(power == 0 && (this->state == media_player::MEDIA_PLAYER_STATE_PLAYING))
  {
	  this->state = media_player::MEDIA_PLAYER_STATE_PAUSED;
      this->publish_state();
	  ESP_LOGD("onkyo power updated from physical state", "%i", power);
  }
}

media_player::MediaPlayerTraits OnkyoAudioMediaPlayer::get_traits() {
  auto traits = media_player::MediaPlayerTraits();
  traits.set_supports_pause(true);
  return traits;
};

void OnkyoAudioMediaPlayer::dump_config() {
  ESP_LOGCONFIG(TAG, "Audio:");
  if (this->is_failed()) {
    ESP_LOGCONFIG(TAG, "Audio failed to initialize!");
    return;
  }
}

// copilot code snippet
String OnkyoAudioMediaPlayer::readStringUntil(char terminator) {
  String str = "";
  // char ch;
  uint8_t ch;
  while (this->uart_->available()) {
    // ch = read();
    this->uart_->read_byte(&ch);
    if (ch == terminator) {
      break;
    }
    str += (char)ch;
    delay(10);  // small delay to allow buffer to fill
  }
  return str;
}

int OnkyoAudioMediaPlayer::cmd(String cmd) {
  String command = "!1" + cmd + "\r";
  this->uart_->write_str(command.c_str());

  unsigned long currentMillis = millis();

  while (this->uart_->available() == 0)
    if (millis() - currentMillis > SERIAL_TIME_OUT) return 0;

  String incoming_string = readStringUntil(DEV_EC).substring(5, 7);

  return strtoul(incoming_string.c_str(), NULL, 16);
}

uint8_t OnkyoAudioMediaPlayer::get_volume() {
  // flush serial input buffer
  uint8_t ch;
  while (this->uart_->available()) this->uart_->read_byte(&ch);

  String command = "!1MVLQSTN\r";
  std::vector<uint8_t> vec(command.begin(), command.end());
  this->uart_->write_array(vec);

  unsigned long currentMillis = millis();
  while (this->uart_->available() == 0)
    if (millis() - currentMillis > SERIAL_TIME_OUT) return -1;

  String incoming_string = readStringUntil(DEV_EC).substring(5, 7);
  return (uint8_t)strtoul(incoming_string.c_str(), NULL, 16);
}

int OnkyoAudioMediaPlayer::get_power() {
  // flush serial input buffer
  uint8_t ch;
  while (this->uart_->available()) this->uart_->read_byte(&ch);

  return this->cmd("PWRQSTN") <= 0 ? 0 : 1;
}

void OnkyoAudioMediaPlayer::setVolume(uint8_t vol) {
  char buffer[9];

  sprintf(buffer, "!1MVL%02X\r", vol);

  this->uart_->write_array((const uint8_t *)buffer, 9);
}

void OnkyoAudioMediaPlayer::power(uint8_t pow) {
  char buffer[9];

  sprintf(buffer, "!1PWR%02X\r", pow);

  this->uart_->write_array((const uint8_t *)buffer, 9);
}

}  // namespace onkyo_audio
}  // namespace esphome
