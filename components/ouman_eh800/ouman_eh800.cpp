#include "ouman_eh800.h"

#include <algorithm>
#include <cstdlib>

#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ouman_eh800 {

static const char *const TAG = "ouman_eh800";

static const char *const REG_OUTDOOR_TEMPERATURE = "S_227_85";
static const char *const REG_L1_SUPPLY_TEMPERATURE = "S_259_85";
static const char *const REG_L1_SUPPLY_TARGET = "S_275_85";
static const char *const REG_L1_ROOM_TEMPERATURE = "S_261_85";
static const char *const REG_L1_VALVE_POSITION = "S_272_85";

void OumanEH800::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Ouman EH-800...");
}

void OumanEH800::dump_config() {
  ESP_LOGCONFIG(TAG, "Ouman EH-800:");
  ESP_LOGCONFIG(TAG, "  Host: %s", this->host_.c_str());
  ESP_LOGCONFIG(TAG, "  Port: %u", this->port_);
  ESP_LOGCONFIG(TAG, "  Update interval: %u ms", this->get_update_interval());
  ESP_LOGCONFIG(TAG, "  Maximum response size: %u bytes", static_cast<unsigned>(this->max_response_size_));
  LOG_SENSOR("  ", "Outdoor temperature", this->outdoor_temperature_sensor_);
  LOG_SENSOR("  ", "L1 supply temperature", this->l1_supply_temperature_sensor_);
  LOG_SENSOR("  ", "L1 supply target", this->l1_supply_target_sensor_);
  LOG_SENSOR("  ", "L1 room temperature", this->l1_room_temperature_sensor_);
  LOG_SENSOR("  ", "L1 valve position", this->l1_valve_position_sensor_);
}

std::string OumanEH800::base_url_() const {
  return "http://" + this->host_ + ":" + std::to_string(this->port_);
}

std::vector<const char *> OumanEH800::requested_registers_() const {
  std::vector<const char *> registers;
  registers.reserve(5);

  if (this->outdoor_temperature_sensor_ != nullptr)
    registers.push_back(REG_OUTDOOR_TEMPERATURE);
  if (this->l1_supply_temperature_sensor_ != nullptr)
    registers.push_back(REG_L1_SUPPLY_TEMPERATURE);
  if (this->l1_supply_target_sensor_ != nullptr)
    registers.push_back(REG_L1_SUPPLY_TARGET);
  if (this->l1_room_temperature_sensor_ != nullptr)
    registers.push_back(REG_L1_ROOM_TEMPERATURE);
  if (this->l1_valve_position_sensor_ != nullptr)
    registers.push_back(REG_L1_VALVE_POSITION);

  return registers;
}

bool OumanEH800::http_get_body_(const std::string &url, std::string &body) {
  if (this->http_ == nullptr) {
    ESP_LOGE(TAG, "HTTP request component is not configured");
    return false;
  }

  const std::vector<http_request::Header> headers{
      {"Connection", "close"},
  };

  auto container = this->http_->get(url, headers);
  if (container == nullptr) {
    ESP_LOGW(TAG, "HTTP request could not be started");
    return false;
  }

  if (!http_request::is_success(container->status_code)) {
    ESP_LOGW(TAG, "HTTP GET failed with status %d", container->status_code);
    container->end();
    return false;
  }

  body.clear();
  body.reserve(std::min(container->content_length, this->max_response_size_));

  uint8_t buffer[256];
  uint32_t last_data_time = millis();
  bool ok = false;

  while (body.size() < this->max_response_size_) {
    const size_t room = this->max_response_size_ - body.size();
    const size_t chunk = std::min(sizeof(buffer), room);

    int read_result = container->read(buffer, chunk);
    App.feed_wdt();
    yield();

    const auto state = http_request::http_read_loop_result(
        read_result, last_data_time, this->http_->get_timeout(), container->is_read_complete());

    if (state == http_request::HttpReadLoopResult::DATA) {
      body.append(reinterpret_cast<const char *>(buffer), static_cast<size_t>(read_result));
      continue;
    }

    if (state == http_request::HttpReadLoopResult::RETRY)
      continue;

    if (state == http_request::HttpReadLoopResult::COMPLETE) {
      ok = true;
      break;
    }

    if (state == http_request::HttpReadLoopResult::TIMEOUT)
      ESP_LOGW(TAG, "Timed out while reading EH-800 response");
    else
      ESP_LOGW(TAG, "Error while reading EH-800 response");

    break;
  }

  if (!ok && body.size() >= this->max_response_size_)
    ESP_LOGW(TAG, "EH-800 response exceeded max_response_size");

  container->end();
  return ok;
}

bool OumanEH800::login_() {
  std::string body;
  const std::string url =
      this->base_url_() + "/login?uid=" + this->username_ + ";pwd=" + this->password_ + ";";

  if (!this->http_get_body_(url, body)) {
    ESP_LOGW(TAG, "Login request failed");
    return false;
  }

  ESP_LOGV(TAG, "EH-800 login request completed");
  return true;
}

bool OumanEH800::read_values_(std::string &body) {
  const auto registers = this->requested_registers_();
  if (registers.empty()) {
    ESP_LOGD(TAG, "No EH-800 sensors configured");
    body.clear();
    return true;
  }

  std::string url = this->base_url_() + "/request?";
  for (size_t i = 0; i < registers.size(); i++) {
    if (i != 0)
      url += ";";
    url += registers[i];
  }
  url += ";";

  return this->http_get_body_(url, body);
}

bool OumanEH800::parse_float_(const std::string &body, const char *register_name, float &value) const {
  const std::string key = std::string(register_name) + "=";
  const size_t start = body.find(key);
  if (start == std::string::npos)
    return false;

  const size_t value_start = start + key.size();
  size_t value_end = body.find(';', value_start);
  if (value_end == std::string::npos)
    value_end = body.size();

  std::string raw = body.substr(value_start, value_end - value_start);
  const size_t first = raw.find_first_not_of(" \t\r\n");
  if (first == std::string::npos)
    return false;
  const size_t last = raw.find_last_not_of(" \t\r\n");
  raw = raw.substr(first, last - first + 1);

  char *end = nullptr;
  value = std::strtof(raw.c_str(), &end);
  return end != raw.c_str() && *end == '\0';
}

void OumanEH800::publish_if_present_(const std::string &body, const char *register_name,
                                     sensor::Sensor *sensor) const {
  if (sensor == nullptr)
    return;

  float value;
  if (!this->parse_float_(body, register_name, value)) {
    ESP_LOGW(TAG, "Register %s missing or invalid in response", register_name);
    return;
  }

  sensor->publish_state(value);
}

void OumanEH800::update() {
  if (!this->login_())
    return;

  std::string body;
  if (!this->read_values_(body))
    return;

  ESP_LOGV(TAG, "EH-800 response: %s", body.c_str());

  this->publish_if_present_(body, REG_OUTDOOR_TEMPERATURE, this->outdoor_temperature_sensor_);
  this->publish_if_present_(body, REG_L1_SUPPLY_TEMPERATURE, this->l1_supply_temperature_sensor_);
  this->publish_if_present_(body, REG_L1_SUPPLY_TARGET, this->l1_supply_target_sensor_);
  this->publish_if_present_(body, REG_L1_ROOM_TEMPERATURE, this->l1_room_temperature_sensor_);
  this->publish_if_present_(body, REG_L1_VALVE_POSITION, this->l1_valve_position_sensor_);
}

}  // namespace ouman_eh800
}  // namespace esphome
