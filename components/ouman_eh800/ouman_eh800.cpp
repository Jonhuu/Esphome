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
static const char *const REG_L1_ROOM_TEMPERATURE = "S_284_85";
static const char *const REG_L1_ROOM_REMOTE = "S_274_85";
static const char *const REG_L1_VALVE_POSITION = "S_272_85";

static const char *const REG_L1_CURVE_SUPPLY_TARGET = "S_260_85";
static const char *const REG_L1_ROOM_COMPENSATION = "S_263_85";
static const char *const REG_L1_ROOM_COMPENSATION_TIME_CORRECTION = "S_264_85";
static const char *const REG_L1_SUPPLY_TARGET = "S_275_85";
static const char *const REG_L1_SMOOTHED_ROOM_TEMPERATURE = "S_262_85";
static const char *const REG_L1_CALCULATED_ROOM_TARGET = "S_278_85";

static const char *const REG_L1_ROOM_SETPOINT = "S_81_85";
static const char *const REG_L1_TEMPERATURE_DROP = "S_87_85";
static const char *const REG_L1_LARGE_TEMPERATURE_DROP = "S_88_85";
static const char *const REG_L1_SUPPLY_MIN = "S_54_85";
static const char *const REG_L1_SUPPLY_MAX = "S_55_85";
static const char *const REG_L1_CURVE_MINUS_20 = "S_67_85";
static const char *const REG_L1_CURVE_MINUS_10 = "S_69_85";
static const char *const REG_L1_CURVE_0 = "S_71_85";
static const char *const REG_L1_CURVE_PLUS_10 = "S_73_85";
static const char *const REG_L1_CURVE_PLUS_20 = "S_75_85";
static const char *const REG_L1_MANUAL_VALVE = "S_92_85";

static const char *const REG_L1_CONTROL_MODE = "S_59_85";
static const char *const REG_HOME_AWAY_MODE = "S_135_85";
static const char *const REG_HOME_AWAY_MIRROR = "S_222_85";

static const int CONTROL_MODE_CODES[] = {0, 3, 1, 2, 6, 5};
static const int HOME_AWAY_CODES[] = {0, 2, 1};

void OumanNumber::control(float value) {
  if (this->parent_ != nullptr)
    this->parent_->write_number(this->type_, value);
}

void OumanSelect::control(size_t index) {
  if (this->parent_ != nullptr)
    this->parent_->write_select(this->type_, index);
}

void OumanEH800::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Ouman EH-800...");
  this->logged_in_ = this->login_();
}

void OumanEH800::dump_config() {
  ESP_LOGCONFIG(TAG, "Ouman EH-800:");
  ESP_LOGCONFIG(TAG, "  Host: %s", this->host_.c_str());
  ESP_LOGCONFIG(TAG, "  Port: %u", this->port_);
  ESP_LOGCONFIG(TAG, "  Fast update interval: %u ms", this->get_update_interval());
  ESP_LOGCONFIG(TAG, "  Slow update interval: %u ms", this->slow_update_interval_);
  ESP_LOGCONFIG(TAG, "  Maximum response size: %u bytes", static_cast<unsigned>(this->max_response_size_));
  LOG_SENSOR("  ", "Outdoor temperature", this->outdoor_temperature_sensor_);
  LOG_SENSOR("  ", "L1 supply temperature", this->l1_supply_temperature_sensor_);
  LOG_SENSOR("  ", "L1 room temperature", this->l1_room_temperature_sensor_);
  LOG_SENSOR("  ", "L1 room remote value", this->l1_room_remote_sensor_);
  LOG_SENSOR("  ", "L1 valve position", this->l1_valve_position_sensor_);
  LOG_SENSOR("  ", "L1 curve supply target", this->l1_curve_supply_target_sensor_);
  LOG_SENSOR("  ", "L1 room compensation", this->l1_room_compensation_sensor_);
  LOG_SENSOR("  ", "L1 room compensation time correction", this->l1_room_compensation_time_correction_sensor_);
  LOG_SENSOR("  ", "L1 supply target", this->l1_supply_target_sensor_);
  LOG_SENSOR("  ", "L1 smoothed room temperature", this->l1_smoothed_room_temperature_sensor_);
  LOG_SENSOR("  ", "L1 calculated room target", this->l1_calculated_room_target_sensor_);
}

std::string OumanEH800::base_url_() const {
  return "http://" + this->host_ + ":" + std::to_string(this->port_);
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
    this->logged_in_ = false;
    return false;
  }

  if (!http_request::is_success(container->status_code)) {
    ESP_LOGW(TAG, "HTTP GET failed with status %d", container->status_code);
    container->end();
    this->logged_in_ = false;
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

  if (!ok)
    this->logged_in_ = false;

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

  if (body.find("login?ok") == std::string::npos) {
    ESP_LOGW(TAG, "EH-800 rejected login");
    return false;
  }

  ESP_LOGD(TAG, "EH-800 login successful");
  return true;
}

bool OumanEH800::ensure_login_() {
  if (this->logged_in_)
    return true;

  this->logged_in_ = this->login_();
  return this->logged_in_;
}

bool OumanEH800::read_registers_(const std::vector<const char *> &registers, std::string &body) {
  if (registers.empty()) {
    body.clear();
    return true;
  }

  if (!this->ensure_login_())
    return false;

  std::string url = this->base_url_() + "/request?";
  for (size_t i = 0; i < registers.size(); i++) {
    if (i != 0)
      url += ";";
    url += registers[i];
  }
  url += ";";

  return this->http_get_body_(url, body);
}

std::vector<const char *> OumanEH800::requested_fast_registers_() const {
  std::vector<const char *> registers;
  registers.reserve(11);

  if (this->outdoor_temperature_sensor_ != nullptr)
    registers.push_back(REG_OUTDOOR_TEMPERATURE);
  if (this->l1_supply_temperature_sensor_ != nullptr)
    registers.push_back(REG_L1_SUPPLY_TEMPERATURE);
  if (this->l1_room_temperature_sensor_ != nullptr)
    registers.push_back(REG_L1_ROOM_TEMPERATURE);
  if (this->l1_room_remote_sensor_ != nullptr)
    registers.push_back(REG_L1_ROOM_REMOTE);
  if (this->l1_valve_position_sensor_ != nullptr)
    registers.push_back(REG_L1_VALVE_POSITION);
  if (this->l1_curve_supply_target_sensor_ != nullptr)
    registers.push_back(REG_L1_CURVE_SUPPLY_TARGET);
  if (this->l1_room_compensation_sensor_ != nullptr)
    registers.push_back(REG_L1_ROOM_COMPENSATION);
  if (this->l1_room_compensation_time_correction_sensor_ != nullptr)
    registers.push_back(REG_L1_ROOM_COMPENSATION_TIME_CORRECTION);
  if (this->l1_supply_target_sensor_ != nullptr)
    registers.push_back(REG_L1_SUPPLY_TARGET);
  if (this->l1_smoothed_room_temperature_sensor_ != nullptr)
    registers.push_back(REG_L1_SMOOTHED_ROOM_TEMPERATURE);
  if (this->l1_calculated_room_target_sensor_ != nullptr)
    registers.push_back(REG_L1_CALCULATED_ROOM_TARGET);

  return registers;
}

std::vector<const char *> OumanEH800::requested_slow_registers_() const {
  std::vector<const char *> registers;
  registers.reserve(13);

  if (this->l1_room_setpoint_number_ != nullptr)
    registers.push_back(REG_L1_ROOM_SETPOINT);
  if (this->l1_temperature_drop_number_ != nullptr)
    registers.push_back(REG_L1_TEMPERATURE_DROP);
  if (this->l1_large_temperature_drop_number_ != nullptr)
    registers.push_back(REG_L1_LARGE_TEMPERATURE_DROP);
  if (this->l1_supply_min_number_ != nullptr)
    registers.push_back(REG_L1_SUPPLY_MIN);
  if (this->l1_supply_max_number_ != nullptr)
    registers.push_back(REG_L1_SUPPLY_MAX);
  if (this->l1_curve_minus_20_number_ != nullptr)
    registers.push_back(REG_L1_CURVE_MINUS_20);
  if (this->l1_curve_minus_10_number_ != nullptr)
    registers.push_back(REG_L1_CURVE_MINUS_10);
  if (this->l1_curve_0_number_ != nullptr)
    registers.push_back(REG_L1_CURVE_0);
  if (this->l1_curve_plus_10_number_ != nullptr)
    registers.push_back(REG_L1_CURVE_PLUS_10);
  if (this->l1_curve_plus_20_number_ != nullptr)
    registers.push_back(REG_L1_CURVE_PLUS_20);
  if (this->l1_manual_valve_number_ != nullptr)
    registers.push_back(REG_L1_MANUAL_VALVE);
  if (this->l1_control_mode_select_ != nullptr)
    registers.push_back(REG_L1_CONTROL_MODE);
  if (this->home_away_mode_select_ != nullptr)
    registers.push_back(REG_HOME_AWAY_MODE);

  return registers;
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

bool OumanEH800::parse_int_(const std::string &body, const char *register_name, int &value) const {
  float parsed;
  if (!this->parse_float_(body, register_name, parsed))
    return false;

  value = static_cast<int>(parsed);
  return true;
}

void OumanEH800::publish_sensor_(const std::string &body, const char *register_name,
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

void OumanEH800::publish_number_(const std::string &body, const char *register_name,
                                 OumanNumber *number) const {
  if (number == nullptr)
    return;

  float value;
  if (!this->parse_float_(body, register_name, value)) {
    ESP_LOGW(TAG, "Register %s missing or invalid in response", register_name);
    return;
  }

  number->publish_state(value);
}

void OumanEH800::publish_control_mode_(const std::string &body) const {
  if (this->l1_control_mode_select_ == nullptr)
    return;

  int code;
  if (!this->parse_int_(body, REG_L1_CONTROL_MODE, code)) {
    ESP_LOGW(TAG, "Register %s missing or invalid in response", REG_L1_CONTROL_MODE);
    return;
  }

  for (size_t i = 0; i < sizeof(CONTROL_MODE_CODES) / sizeof(CONTROL_MODE_CODES[0]); i++) {
    if (CONTROL_MODE_CODES[i] == code) {
      this->l1_control_mode_select_->publish_state(i);
      return;
    }
  }

  ESP_LOGW(TAG, "Unknown L1 control mode code: %d", code);
}

void OumanEH800::publish_home_away_mode_(const std::string &body) const {
  if (this->home_away_mode_select_ == nullptr)
    return;

  int code;
  if (!this->parse_int_(body, REG_HOME_AWAY_MODE, code)) {
    ESP_LOGW(TAG, "Register %s missing or invalid in response", REG_HOME_AWAY_MODE);
    return;
  }

  for (size_t i = 0; i < sizeof(HOME_AWAY_CODES) / sizeof(HOME_AWAY_CODES[0]); i++) {
    if (HOME_AWAY_CODES[i] == code) {
      this->home_away_mode_select_->publish_state(i);
      return;
    }
  }

  ESP_LOGW(TAG, "Unknown home/away mode code: %d", code);
}

bool OumanEH800::poll_fast_() {
  std::string body;
  if (!this->read_registers_(this->requested_fast_registers_(), body))
    return false;

  ESP_LOGV(TAG, "EH-800 fast response: %s", body.c_str());

  this->publish_sensor_(body, REG_OUTDOOR_TEMPERATURE, this->outdoor_temperature_sensor_);
  this->publish_sensor_(body, REG_L1_SUPPLY_TEMPERATURE, this->l1_supply_temperature_sensor_);
  this->publish_sensor_(body, REG_L1_ROOM_TEMPERATURE, this->l1_room_temperature_sensor_);
  this->publish_sensor_(body, REG_L1_ROOM_REMOTE, this->l1_room_remote_sensor_);
  this->publish_sensor_(body, REG_L1_VALVE_POSITION, this->l1_valve_position_sensor_);
  this->publish_sensor_(body, REG_L1_CURVE_SUPPLY_TARGET, this->l1_curve_supply_target_sensor_);
  this->publish_sensor_(body, REG_L1_ROOM_COMPENSATION, this->l1_room_compensation_sensor_);
  this->publish_sensor_(body, REG_L1_ROOM_COMPENSATION_TIME_CORRECTION,
                        this->l1_room_compensation_time_correction_sensor_);
  this->publish_sensor_(body, REG_L1_SUPPLY_TARGET, this->l1_supply_target_sensor_);
  this->publish_sensor_(body, REG_L1_SMOOTHED_ROOM_TEMPERATURE, this->l1_smoothed_room_temperature_sensor_);
  this->publish_sensor_(body, REG_L1_CALCULATED_ROOM_TARGET, this->l1_calculated_room_target_sensor_);

  return true;
}

bool OumanEH800::poll_slow_() {
  std::string body;
  if (!this->read_registers_(this->requested_slow_registers_(), body))
    return false;

  ESP_LOGV(TAG, "EH-800 slow response: %s", body.c_str());

  this->publish_number_(body, REG_L1_ROOM_SETPOINT, this->l1_room_setpoint_number_);
  this->publish_number_(body, REG_L1_TEMPERATURE_DROP, this->l1_temperature_drop_number_);
  this->publish_number_(body, REG_L1_LARGE_TEMPERATURE_DROP, this->l1_large_temperature_drop_number_);
  this->publish_number_(body, REG_L1_SUPPLY_MIN, this->l1_supply_min_number_);
  this->publish_number_(body, REG_L1_SUPPLY_MAX, this->l1_supply_max_number_);
  this->publish_number_(body, REG_L1_CURVE_MINUS_20, this->l1_curve_minus_20_number_);
  this->publish_number_(body, REG_L1_CURVE_MINUS_10, this->l1_curve_minus_10_number_);
  this->publish_number_(body, REG_L1_CURVE_0, this->l1_curve_0_number_);
  this->publish_number_(body, REG_L1_CURVE_PLUS_10, this->l1_curve_plus_10_number_);
  this->publish_number_(body, REG_L1_CURVE_PLUS_20, this->l1_curve_plus_20_number_);
  this->publish_number_(body, REG_L1_MANUAL_VALVE, this->l1_manual_valve_number_);
  this->publish_control_mode_(body);
  this->publish_home_away_mode_(body);

  this->last_slow_update_ = millis();
  return true;
}

const char *OumanEH800::number_register_(OumanNumberType type) const {
  switch (type) {
    case OumanNumberType::ROOM_SETPOINT:
      return REG_L1_ROOM_SETPOINT;
    case OumanNumberType::TEMPERATURE_DROP:
      return REG_L1_TEMPERATURE_DROP;
    case OumanNumberType::LARGE_TEMPERATURE_DROP:
      return REG_L1_LARGE_TEMPERATURE_DROP;
    case OumanNumberType::SUPPLY_MIN:
      return REG_L1_SUPPLY_MIN;
    case OumanNumberType::SUPPLY_MAX:
      return REG_L1_SUPPLY_MAX;
    case OumanNumberType::CURVE_MINUS_20:
      return REG_L1_CURVE_MINUS_20;
    case OumanNumberType::CURVE_MINUS_10:
      return REG_L1_CURVE_MINUS_10;
    case OumanNumberType::CURVE_0:
      return REG_L1_CURVE_0;
    case OumanNumberType::CURVE_PLUS_10:
      return REG_L1_CURVE_PLUS_10;
    case OumanNumberType::CURVE_PLUS_20:
      return REG_L1_CURVE_PLUS_20;
    case OumanNumberType::MANUAL_VALVE:
      return REG_L1_MANUAL_VALVE;
  }

  return nullptr;
}

bool OumanEH800::send_update_(const std::string &parameters) {
  if (!this->ensure_login_())
    return false;

  std::string body;
  const std::string url = this->base_url_() + "/update?" + parameters;
  return this->http_get_body_(url, body);
}

bool OumanEH800::write_number(OumanNumberType type, float value) {
  const char *register_name = this->number_register_(type);
  if (register_name == nullptr)
    return false;

  const std::string parameters = std::string(register_name) + "=" + str_sprintf("%g", value) + ";";
  ESP_LOGD(TAG, "Writing %s=%g", register_name, value);

  if (!this->send_update_(parameters))
    return false;

  // Read back all writable values immediately. The state shown in Home
  // Assistant is therefore the value confirmed by EH-800, not an optimistic
  // local value.
  return this->poll_slow_();
}

bool OumanEH800::write_select(OumanSelectType type, size_t index) {
  std::string parameters;

  if (type == OumanSelectType::CONTROL_MODE) {
    if (index >= sizeof(CONTROL_MODE_CODES) / sizeof(CONTROL_MODE_CODES[0]))
      return false;

    const int code = CONTROL_MODE_CODES[index];
    parameters = std::string(REG_L1_CONTROL_MODE) + "=" + std::to_string(code) + ";";
    ESP_LOGD(TAG, "Writing L1 control mode code %d", code);
  } else if (type == OumanSelectType::HOME_AWAY_MODE) {
    if (index >= sizeof(HOME_AWAY_CODES) / sizeof(HOME_AWAY_CODES[0]))
      return false;

    const int code = HOME_AWAY_CODES[index];
    parameters = std::string(REG_HOME_AWAY_MODE) + "=" + std::to_string(code) + ";" +
                 REG_HOME_AWAY_MIRROR + "=" + std::to_string(code) + ";";
    ESP_LOGD(TAG, "Writing home/away mode code %d", code);
  } else {
    return false;
  }

  if (!this->send_update_(parameters))
    return false;

  return this->poll_slow_();
}

void OumanEH800::update() {
  this->poll_fast_();

  const uint32_t now = millis();
  if (this->last_slow_update_ == 0 || now - this->last_slow_update_ >= this->slow_update_interval_)
    this->poll_slow_();
}

}  // namespace ouman_eh800
}  // namespace esphome
