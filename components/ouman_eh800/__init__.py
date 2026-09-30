import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_HOST,
    CONF_ID,
    CONF_PASSWORD,
    CONF_PORT,
    CONF_USERNAME,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)

CODEOWNERS = []
DEPENDENCIES = ["http_request"]
AUTO_LOAD = ["sensor"]

CONF_HTTP_REQUEST_ID = "http_request_id"
CONF_MAX_RESPONSE_SIZE = "max_response_size"

CONF_OUTDOOR_TEMPERATURE = "outdoor_temperature"
CONF_L1_SUPPLY_TEMPERATURE = "l1_supply_temperature"
CONF_L1_SUPPLY_TARGET = "l1_supply_target"
CONF_L1_ROOM_TEMPERATURE = "l1_room_temperature"
CONF_L1_VALVE_POSITION = "l1_valve_position"

ouman_ns = cg.esphome_ns.namespace("ouman_eh800")
OumanEH800 = ouman_ns.class_("OumanEH800", cg.PollingComponent)

http_request_ns = cg.esphome_ns.namespace("http_request")
HttpRequestComponent = http_request_ns.class_("HttpRequestComponent", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(OumanEH800),
        cv.GenerateID(CONF_HTTP_REQUEST_ID): cv.use_id(HttpRequestComponent),
        cv.Required(CONF_HOST): cv.string_strict,
        cv.Optional(CONF_PORT, default=80): cv.port,
        cv.Required(CONF_USERNAME): cv.string_strict,
        cv.Required(CONF_PASSWORD): cv.string_strict,
        cv.Optional(CONF_MAX_RESPONSE_SIZE, default=4096): cv.int_range(
            min=512, max=16384
        ),
        cv.Optional(CONF_OUTDOOR_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_L1_SUPPLY_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_L1_SUPPLY_TARGET): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_L1_ROOM_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_L1_VALVE_POSITION): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
).extend(cv.polling_component_schema("15s"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    http = await cg.get_variable(config[CONF_HTTP_REQUEST_ID])
    cg.add(var.set_http_request(http))
    cg.add(var.set_host(config[CONF_HOST]))
    cg.add(var.set_port(config[CONF_PORT]))
    cg.add(var.set_username(config[CONF_USERNAME]))
    cg.add(var.set_password(config[CONF_PASSWORD]))
    cg.add(var.set_max_response_size(config[CONF_MAX_RESPONSE_SIZE]))

    if CONF_OUTDOOR_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_OUTDOOR_TEMPERATURE])
        cg.add(var.set_outdoor_temperature_sensor(sens))

    if CONF_L1_SUPPLY_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_L1_SUPPLY_TEMPERATURE])
        cg.add(var.set_l1_supply_temperature_sensor(sens))

    if CONF_L1_SUPPLY_TARGET in config:
        sens = await sensor.new_sensor(config[CONF_L1_SUPPLY_TARGET])
        cg.add(var.set_l1_supply_target_sensor(sens))

    if CONF_L1_ROOM_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_L1_ROOM_TEMPERATURE])
        cg.add(var.set_l1_room_temperature_sensor(sens))

    if CONF_L1_VALVE_POSITION in config:
        sens = await sensor.new_sensor(config[CONF_L1_VALVE_POSITION])
        cg.add(var.set_l1_valve_position_sensor(sens))
