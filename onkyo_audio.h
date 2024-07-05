#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace onkyo_audio {

class OnkyoAudioComponent : public Component {
 public:
  void setup() override;

  void lock() { this->lock_.lock(); }
  bool try_lock() { return this->lock_.try_lock(); }
  void unlock() { this->lock_.unlock(); }
  
 protected:
  Mutex lock_;
};

}  // namespace onkyo_audio
}  // namespace esphome

