#include "esphome/core/log.h"
#include "onkyo.h"

namespace esphome {
namespace onkyo {

  static const char *const TAG = "onkyo";

void OnkyoDevice::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Audio...");
}



uint8_t OnkyoDevice::get_volume() {
  // flush serial input buffer
  uint8_t ch;
  while (this->uart_->available()) this->uart_->read_byte(&ch);

  String command = "!1MVLQSTN\r";
  std::vector<uint8_t> vec(command.begin(), command.end());
  this->uart_->write_array(vec);

  unsigned long currentMillis = millis();
  while (this->uart_->available() == 0)
    if (millis() - currentMillis > SERIAL_TIME_OUT) return -1;

  String incoming_string = this->readStringUntil(DEV_EC).substring(5, 7);
  return (uint8_t)strtoul(incoming_string.c_str(), NULL, 16);
}

int OnkyoDevice::get_power() {
  // flush serial input buffer
  uint8_t ch;
  while (this->uart_->available()) this->uart_->read_byte(&ch);

  return this->cmd("PWRQSTN") <= 0 ? 0 : 1;
}

int OnkyoDevice::cmd(String cmd) {
  String command = "!1" + cmd + "\r";
  this->uart_->write_str(command.c_str());

  unsigned long currentMillis = millis();

  while (this->uart_->available() == 0)
    if (millis() - currentMillis > SERIAL_TIME_OUT) return 0;

  String incoming_string = readStringUntil(DEV_EC).substring(5, 7);

  return strtoul(incoming_string.c_str(), NULL, 16);
}

// copilot code snippet
String OnkyoDevice::readStringUntil(char terminator) {
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

void OnkyoDevice::setVolume(uint8_t vol) {
  char buffer[9];

  sprintf(buffer, "!1MVL%02X\r", vol);

  this->uart_->write_array((const uint8_t *)buffer, 9);
}

void OnkyoDevice::power(uint8_t pow) {
  char buffer[9];

  sprintf(buffer, "!1PWR%02X\r", pow);

  this->uart_->write_array((const uint8_t *)buffer, 9);
}

}  // namespace onkyo
}  // namespace esphome
