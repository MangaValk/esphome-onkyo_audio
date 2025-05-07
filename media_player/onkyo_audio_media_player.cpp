#include "esphome/core/log.h"
#include "onkyo_audio_media_player.h"

namespace esphome {
namespace onkyo_audio {

static const char *const TAG = "audio";

void OnkyoAudioMediaPlayer::control(const media_player::MediaPlayerCall &call) {
  media_player::MediaPlayerState play_state = media_player::MEDIA_PLAYER_STATE_PLAYING;
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

  // check incoming command
  if (call.get_command().has_value()) {
    auto command = *call.get_command();
    if (command == media_player::MEDIA_PLAYER_COMMAND_PLAY) {
      this->onkyoObj->power(1);
      this->state = media_player::MEDIA_PLAYER_STATE_PLAYING;
      ESP_LOGD("onkyo power turn on", "%i", this->onkyoObj->get_power());
    } else if (command == media_player::MEDIA_PLAYER_COMMAND_STOP) {
      this->onkyoObj->power(0);
      this->state = media_player::MEDIA_PLAYER_STATE_PAUSED;
      ESP_LOGD("onkyo power turn off", "%i", this->onkyoObj->get_power());
    } else if (command == media_player::MEDIA_PLAYER_COMMAND_PAUSE) {
      this->onkyoObj->power(0);
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

  if (this->onkyoObj == NULL)
    this->onkyoObj = new OnkyoDevice();
  this->onkyoObj->set_uart(this->uart_);

  refreshPowerState();

  float volume = this->onkyoObj->get_volume();
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

  float newVolume = this->onkyoObj->get_volume();
  
  // set physical volume.
  if (this->oldVolume != this->volume) {
    this->oldVolume = this->volume;
	int mappedVolume = remap<uint8_t, float>(this->volume, 0.0f, 1.0f, 0, 78);
	
	for (int counter = 0; counter <= 15; counter++)
	{		
		this->onkyoObj->setVolume(mappedVolume);
		float volumeAfterSet = this->onkyoObj->get_volume();
		if (abs(volumeAfterSet) - abs(mappedVolume) <= 3
				&& abs(mappedVolume) - abs(volumeAfterSet) <= 3)
			break;
		
		ESP_LOGD("onkyo retrying set volume: ", "%f", this->volume);
		delay(100);	
	}
	
	this->volumeDelay = millis() + 5000;
  }
  // update volume state from physical state.
  else if(this->volume != newVolume && newVolume != -1 && (millis() - this->volumeDelay) > 0 )
  {
	  delay(100);
	  // check if volume hasnt changed.
	  if(newVolume == this->onkyoObj->get_volume())
	  {
		  if (newVolume >= 0 && newVolume <= 78)
		  {
			  this->volume = remap<float, uint8_t>(newVolume, 0, 78, 0.0f, 1.0f);
			  this->oldVolume = this->volume;
			  this->publish_state();
			  ESP_LOGD("onkyo volume updated from physical state", "%f", this->volume);
			  
			  this->volumeDelay = millis() + 500;
		  }
	  }
  }

  refreshPowerState();
}

// functions

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

void OnkyoAudioMediaPlayer::refreshPowerState()
{
  int power = this->onkyoObj->get_power();
  //if(millis() % 1000 < 50)
  //{
  //	ESP_LOGD("onkyo power", "%i", power);
  //	ESP_LOGD("onkyo volume", "%i", get_volume());
  //}
  ESP_LOGD("onkyo power", "%i", power);
  
  // update power state from physical power state.
  if(power == 1 && (this->state != media_player::MEDIA_PLAYER_STATE_PLAYING))
  {
	  this->state = media_player::MEDIA_PLAYER_STATE_PLAYING;
      this->publish_state();
	  ESP_LOGD("onkyo power updated from physical state", "%i", power);
  }
  else if(power == 0 && (this->state != media_player::MEDIA_PLAYER_STATE_PAUSED))
  {
	  this->state = media_player::MEDIA_PLAYER_STATE_PAUSED;
      this->publish_state();
	  ESP_LOGD("onkyo power updated from physical state", "%i", power);
  }
}

}  // namespace onkyo_audio
}  // namespace esphome
