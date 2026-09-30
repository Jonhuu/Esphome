#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "esphome/components/http_request/http_request.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace ouman_eh800 {

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

  void set_outdoor_temperature_sensor(sensor::Sensor *sensor) { this->outdoor_temperature_sensor_ = sensor; }
  void set_l1_supply_temperature_sensor(sensor::Sensor *sensor) { this->l1_supply_temperature_sensor_ = sensor; }
  void set_l1_supply_target_sensor(sensor::Sensor *sensor) { this->l1_supply_target_sensor_ = sensor; }
  void set_l1_room_temperature_sensor(sensor::Sensor *sensor) { this->l1_room_temperature_sensor_ = sensor; }
  void set_l1_valve_position_sensor(sensor::Sensor *sensor) { this->l1_valve_position_sensor_ = sensor; }

 protected:
  std::string base_url_() const;
  bool login_();
  bool read_values_(std::string &body);
  bool http_get_body_(const std::string &url, std::string &body);
  bool parse_float_(const std::string &body, const char *register_name, float &value) const;
  void publish_if_present_(const std::string &body, const char *register_name, sensor::Sensor *sensor) const;
  std::vector<const char *> requested_registers_() const;

  http_request::HttpRequestComponent *http_{nullptr};

  std::string host_;
  uint16_t port_{80};
  std::string username_;
  std::string password_;
  size_t max_response_size_{4096};

  sensor::Sensor *outdoor_temperature_sensor_{nullptr};
  sensor::Sensor *l1_supply_temperature_sensor_{nullptr};
  sensor::Sensor *l1_supply_target_sensor_{nullptr};
  sensor::Sensor *l1_room_temperature_sensor_{nullptr};
  sensor::Sensor *l1_valve_position_sensor_{nullptr};
};

}  // namespace ouman_eh800
}  // namespace esphome
