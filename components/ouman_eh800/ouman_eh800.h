#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "esphome/components/http_request/http_request.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace ouman_eh800 {

class OumanEH800;

enum class OumanNumberType : uint8_t {
  ROOM_SETPOINT,
  TEMPERATURE_DROP,
  LARGE_TEMPERATURE_DROP,
  SUPPLY_MIN,
  SUPPLY_MAX,
  CURVE_MINUS_20,
  CURVE_MINUS_10,
  CURVE_0,
  CURVE_PLUS_10,
  CURVE_PLUS_20,
  MANUAL_VALVE,
};

enum class OumanSelectType : uint8_t {
  CONTROL_MODE,
  HOME_AWAY_MODE,
};

class OumanNumber : public number::Number, public Parented<OumanEH800> {
 public:
  explicit OumanNumber(OumanNumberType type) : type_(type) {}

 protected:
  void control(float value) override;
  OumanNumberType type_;
};

class OumanSelect : public select::Select, public Parented<OumanEH800> {
 public:
  explicit OumanSelect(OumanSelectType type) : type_(type) {}

 protected:
  void control(size_t index) override;
  OumanSelectType type_;
};

class OumanEH800 : public PollingComponent {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void set_http_request(http_request::HttpRequestComponent *http) { this->http_ = http; }
  void set_host(const std::string &host) { this->host_ = host; }
  void set_port(uint16_t port) { this->port_ = port; }
  void set_username(const std::string &username) { this->username_ = username; }
  void set_password(const std::string &password) { this->password_ = password; }
  void set_max_response_size(size_t size) { this->max_response_size_ = size; }
  void set_slow_update_interval(uint32_t interval) { this->slow_update_interval_ = interval; }

  void set_outdoor_temperature_sensor(sensor::Sensor *sensor) { this->outdoor_temperature_sensor_ = sensor; }
  void set_l1_supply_temperature_sensor(sensor::Sensor *sensor) { this->l1_supply_temperature_sensor_ = sensor; }
  void set_l1_room_temperature_sensor(sensor::Sensor *sensor) { this->l1_room_temperature_sensor_ = sensor; }
  void set_l1_room_remote_sensor(sensor::Sensor *sensor) { this->l1_room_remote_sensor_ = sensor; }
  void set_l1_valve_position_sensor(sensor::Sensor *sensor) { this->l1_valve_position_sensor_ = sensor; }
  void set_l1_curve_supply_target_sensor(sensor::Sensor *sensor) { this->l1_curve_supply_target_sensor_ = sensor; }
  void set_l1_room_compensation_sensor(sensor::Sensor *sensor) { this->l1_room_compensation_sensor_ = sensor; }
  void set_l1_room_compensation_time_correction_sensor(sensor::Sensor *sensor) {
    this->l1_room_compensation_time_correction_sensor_ = sensor;
  }
  void set_l1_supply_target_sensor(sensor::Sensor *sensor) { this->l1_supply_target_sensor_ = sensor; }
  void set_l1_smoothed_room_temperature_sensor(sensor::Sensor *sensor) {
    this->l1_smoothed_room_temperature_sensor_ = sensor;
  }
  void set_l1_calculated_room_target_sensor(sensor::Sensor *sensor) {
    this->l1_calculated_room_target_sensor_ = sensor;
  }

  void set_l1_room_setpoint_number(OumanNumber *number) { this->l1_room_setpoint_number_ = number; }
  void set_l1_temperature_drop_number(OumanNumber *number) { this->l1_temperature_drop_number_ = number; }
  void set_l1_large_temperature_drop_number(OumanNumber *number) { this->l1_large_temperature_drop_number_ = number; }
  void set_l1_supply_min_number(OumanNumber *number) { this->l1_supply_min_number_ = number; }
  void set_l1_supply_max_number(OumanNumber *number) { this->l1_supply_max_number_ = number; }
  void set_l1_curve_minus_20_number(OumanNumber *number) { this->l1_curve_minus_20_number_ = number; }
  void set_l1_curve_minus_10_number(OumanNumber *number) { this->l1_curve_minus_10_number_ = number; }
  void set_l1_curve_0_number(OumanNumber *number) { this->l1_curve_0_number_ = number; }
  void set_l1_curve_plus_10_number(OumanNumber *number) { this->l1_curve_plus_10_number_ = number; }
  void set_l1_curve_plus_20_number(OumanNumber *number) { this->l1_curve_plus_20_number_ = number; }
  void set_l1_manual_valve_number(OumanNumber *number) { this->l1_manual_valve_number_ = number; }

  void set_l1_control_mode_select(OumanSelect *select) { this->l1_control_mode_select_ = select; }
  void set_home_away_mode_select(OumanSelect *select) { this->home_away_mode_select_ = select; }

  bool write_number(OumanNumberType type, float value);
  bool write_select(OumanSelectType type, size_t index);

 protected:
  std::string base_url_() const;
  bool login_();
  bool ensure_login_();
  bool http_get_body_(const std::string &url, std::string &body);

  bool read_registers_(const std::vector<const char *> &registers, std::string &body);
  bool poll_fast_();
  bool poll_slow_();

  bool parse_float_(const std::string &body, const char *register_name, float &value) const;
  bool parse_int_(const std::string &body, const char *register_name, int &value) const;

  void publish_sensor_(const std::string &body, const char *register_name, sensor::Sensor *sensor) const;
  void publish_number_(const std::string &body, const char *register_name, OumanNumber *number) const;
  void publish_control_mode_(const std::string &body) const;
  void publish_home_away_mode_(const std::string &body) const;

  std::vector<const char *> requested_fast_registers_() const;
  std::vector<const char *> requested_slow_registers_() const;

  const char *number_register_(OumanNumberType type) const;
  bool send_update_(const std::string &parameters);

  http_request::HttpRequestComponent *http_{nullptr};

  std::string host_;
  uint16_t port_{80};
  std::string username_;
  std::string password_;
  size_t max_response_size_{4096};

  uint32_t slow_update_interval_{60000};
  uint32_t last_slow_update_{0};
  bool logged_in_{false};

  sensor::Sensor *outdoor_temperature_sensor_{nullptr};
  sensor::Sensor *l1_supply_temperature_sensor_{nullptr};
  sensor::Sensor *l1_room_temperature_sensor_{nullptr};
  sensor::Sensor *l1_room_remote_sensor_{nullptr};
  sensor::Sensor *l1_valve_position_sensor_{nullptr};
  sensor::Sensor *l1_curve_supply_target_sensor_{nullptr};
  sensor::Sensor *l1_room_compensation_sensor_{nullptr};
  sensor::Sensor *l1_room_compensation_time_correction_sensor_{nullptr};
  sensor::Sensor *l1_supply_target_sensor_{nullptr};
  sensor::Sensor *l1_smoothed_room_temperature_sensor_{nullptr};
  sensor::Sensor *l1_calculated_room_target_sensor_{nullptr};

  OumanNumber *l1_room_setpoint_number_{nullptr};
  OumanNumber *l1_temperature_drop_number_{nullptr};
  OumanNumber *l1_large_temperature_drop_number_{nullptr};
  OumanNumber *l1_supply_min_number_{nullptr};
  OumanNumber *l1_supply_max_number_{nullptr};
  OumanNumber *l1_curve_minus_20_number_{nullptr};
  OumanNumber *l1_curve_minus_10_number_{nullptr};
  OumanNumber *l1_curve_0_number_{nullptr};
  OumanNumber *l1_curve_plus_10_number_{nullptr};
  OumanNumber *l1_curve_plus_20_number_{nullptr};
  OumanNumber *l1_manual_valve_number_{nullptr};

  OumanSelect *l1_control_mode_select_{nullptr};
  OumanSelect *home_away_mode_select_{nullptr};
};

}  // namespace ouman_eh800
}  // namespace esphome
