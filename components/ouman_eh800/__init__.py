import esphome.codegen as cg
from esphome.components import number, select, sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_HOST,
    CONF_ID,
    CONF_MAX_VALUE,
    CONF_MIN_VALUE,
    CONF_PASSWORD,
    CONF_PORT,
    CONF_STEP,
    CONF_USERNAME,
    DEVICE_CLASS_TEMPERATURE,
    ENTITY_CATEGORY_CONFIG,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)

CODEOWNERS = []
DEPENDENCIES = ["http_request"]
AUTO_LOAD = ["sensor", "number", "select"]

CONF_HTTP_REQUEST_ID = "http_request_id"
CONF_MAX_RESPONSE_SIZE = "max_response_size"
CONF_SLOW_UPDATE_INTERVAL = "slow_update_interval"

# Read-only measurements
CONF_OUTDOOR_TEMPERATURE = "outdoor_temperature"
CONF_L1_SUPPLY_TEMPERATURE = "l1_supply_temperature"
CONF_L1_ROOM_TEMPERATURE = "l1_room_temperature"
CONF_L1_ROOM_REMOTE = "l1_room_remote"
CONF_L1_VALVE_POSITION = "l1_valve_position"
CONF_L1_CURVE_SUPPLY_TARGET = "l1_curve_supply_target"
CONF_L1_ROOM_COMPENSATION = "l1_room_compensation"
CONF_L1_ROOM_COMPENSATION_TIME_CORRECTION = "l1_room_compensation_time_correction"
CONF_L1_SUPPLY_TARGET = "l1_supply_target"
CONF_L1_SMOOTHED_ROOM_TEMPERATURE = "l1_smoothed_room_temperature"
CONF_L1_CALCULATED_ROOM_TARGET = "l1_calculated_room_target"

# Writable numeric settings
CONF_L1_ROOM_SETPOINT = "l1_room_setpoint"
CONF_L1_TEMPERATURE_DROP = "l1_temperature_drop"
CONF_L1_LARGE_TEMPERATURE_DROP = "l1_large_temperature_drop"
CONF_L1_SUPPLY_MIN = "l1_supply_min"
CONF_L1_SUPPLY_MAX = "l1_supply_max"
CONF_L1_CURVE_MINUS_20 = "l1_curve_minus_20"
CONF_L1_CURVE_MINUS_10 = "l1_curve_minus_10"
CONF_L1_CURVE_0 = "l1_curve_0"
CONF_L1_CURVE_PLUS_10 = "l1_curve_plus_10"
CONF_L1_CURVE_PLUS_20 = "l1_curve_plus_20"
CONF_L1_MANUAL_VALVE = "l1_manual_valve"

# Writable selects
CONF_L1_CONTROL_MODE = "l1_control_mode"
CONF_HOME_AWAY_MODE = "home_away_mode"

ouman_ns = cg.esphome_ns.namespace("ouman_eh800")
OumanEH800 = ouman_ns.class_("OumanEH800", cg.PollingComponent)
OumanNumber = ouman_ns.class_(
    "OumanNumber", number.Number, cg.Parented.template(OumanEH800)
)
OumanSelect = ouman_ns.class_(
    "OumanSelect", select.Select, cg.Parented.template(OumanEH800)
)
OumanNumberType = ouman_ns.enum("OumanNumberType", is_class=True)
OumanSelectType = ouman_ns.enum("OumanSelectType", is_class=True)

http_request_ns = cg.esphome_ns.namespace("http_request")
HttpRequestComponent = http_request_ns.class_("HttpRequestComponent", cg.Component)


def _temperature_sensor_schema():
    return sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    )


def _writable_temperature_schema():
    # The EH-800 web UI tells us these are writable, but the downloaded
    # settingsl1 response does not contain min/max limits. Require the user to
    # provide those limits explicitly instead of guessing them.
    return number.number_schema(
        OumanNumber,
        unit_of_measurement=UNIT_CELSIUS,
        device_class=DEVICE_CLASS_TEMPERATURE,
        entity_category=ENTITY_CATEGORY_CONFIG,
    ).extend(
        {
            cv.Required(CONF_MIN_VALUE): cv.float_,
            cv.Required(CONF_MAX_VALUE): cv.float_,
            cv.Optional(CONF_STEP, default=0.1): cv.positive_float,
        }
    )


def _manual_valve_schema():
    return number.number_schema(
        OumanNumber,
        unit_of_measurement=UNIT_PERCENT,
        entity_category=ENTITY_CATEGORY_CONFIG,
    ).extend(
        {
            cv.Required(CONF_MIN_VALUE): cv.float_,
            cv.Required(CONF_MAX_VALUE): cv.float_,
            cv.Optional(CONF_STEP, default=1.0): cv.positive_float,
        }
    )


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
        cv.Optional(CONF_SLOW_UPDATE_INTERVAL, default="60s"): cv.update_interval,

        cv.Optional(CONF_OUTDOOR_TEMPERATURE): _temperature_sensor_schema(),
        cv.Optional(CONF_L1_SUPPLY_TEMPERATURE): _temperature_sensor_schema(),
        cv.Optional(CONF_L1_ROOM_TEMPERATURE): _temperature_sensor_schema(),
        cv.Optional(CONF_L1_ROOM_REMOTE): _temperature_sensor_schema(),
        cv.Optional(CONF_L1_VALVE_POSITION): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_L1_CURVE_SUPPLY_TARGET): _temperature_sensor_schema(),
        cv.Optional(CONF_L1_ROOM_COMPENSATION): _temperature_sensor_schema(),
        cv.Optional(
            CONF_L1_ROOM_COMPENSATION_TIME_CORRECTION
        ): _temperature_sensor_schema(),
        cv.Optional(CONF_L1_SUPPLY_TARGET): _temperature_sensor_schema(),
        cv.Optional(CONF_L1_SMOOTHED_ROOM_TEMPERATURE): _temperature_sensor_schema(),
        cv.Optional(CONF_L1_CALCULATED_ROOM_TARGET): _temperature_sensor_schema(),

        cv.Optional(CONF_L1_ROOM_SETPOINT): _writable_temperature_schema(),
        cv.Optional(CONF_L1_TEMPERATURE_DROP): _writable_temperature_schema(),
        cv.Optional(CONF_L1_LARGE_TEMPERATURE_DROP): _writable_temperature_schema(),
        cv.Optional(CONF_L1_SUPPLY_MIN): _writable_temperature_schema(),
        cv.Optional(CONF_L1_SUPPLY_MAX): _writable_temperature_schema(),
        cv.Optional(CONF_L1_CURVE_MINUS_20): _writable_temperature_schema(),
        cv.Optional(CONF_L1_CURVE_MINUS_10): _writable_temperature_schema(),
        cv.Optional(CONF_L1_CURVE_0): _writable_temperature_schema(),
        cv.Optional(CONF_L1_CURVE_PLUS_10): _writable_temperature_schema(),
        cv.Optional(CONF_L1_CURVE_PLUS_20): _writable_temperature_schema(),
        cv.Optional(CONF_L1_MANUAL_VALVE): _manual_valve_schema(),

        cv.Optional(CONF_L1_CONTROL_MODE): select.select_schema(
            OumanSelect, entity_category=ENTITY_CATEGORY_CONFIG
        ),
        cv.Optional(CONF_HOME_AWAY_MODE): select.select_schema(
            OumanSelect, entity_category=ENTITY_CATEGORY_CONFIG
        ),
    }
).extend(cv.polling_component_schema("10s"))


SENSOR_MAP = (
    (CONF_OUTDOOR_TEMPERATURE, "set_outdoor_temperature_sensor"),
    (CONF_L1_SUPPLY_TEMPERATURE, "set_l1_supply_temperature_sensor"),
    (CONF_L1_ROOM_TEMPERATURE, "set_l1_room_temperature_sensor"),
    (CONF_L1_ROOM_REMOTE, "set_l1_room_remote_sensor"),
    (CONF_L1_VALVE_POSITION, "set_l1_valve_position_sensor"),
    (CONF_L1_CURVE_SUPPLY_TARGET, "set_l1_curve_supply_target_sensor"),
    (CONF_L1_ROOM_COMPENSATION, "set_l1_room_compensation_sensor"),
    (
        CONF_L1_ROOM_COMPENSATION_TIME_CORRECTION,
        "set_l1_room_compensation_time_correction_sensor",
    ),
    (CONF_L1_SUPPLY_TARGET, "set_l1_supply_target_sensor"),
    (
        CONF_L1_SMOOTHED_ROOM_TEMPERATURE,
        "set_l1_smoothed_room_temperature_sensor",
    ),
    (
        CONF_L1_CALCULATED_ROOM_TARGET,
        "set_l1_calculated_room_target_sensor",
    ),
)

NUMBER_MAP = (
    (CONF_L1_ROOM_SETPOINT, OumanNumberType.ROOM_SETPOINT, "set_l1_room_setpoint_number"),
    (
        CONF_L1_TEMPERATURE_DROP,
        OumanNumberType.TEMPERATURE_DROP,
        "set_l1_temperature_drop_number",
    ),
    (
        CONF_L1_LARGE_TEMPERATURE_DROP,
        OumanNumberType.LARGE_TEMPERATURE_DROP,
        "set_l1_large_temperature_drop_number",
    ),
    (CONF_L1_SUPPLY_MIN, OumanNumberType.SUPPLY_MIN, "set_l1_supply_min_number"),
    (CONF_L1_SUPPLY_MAX, OumanNumberType.SUPPLY_MAX, "set_l1_supply_max_number"),
    (
        CONF_L1_CURVE_MINUS_20,
        OumanNumberType.CURVE_MINUS_20,
        "set_l1_curve_minus_20_number",
    ),
    (
        CONF_L1_CURVE_MINUS_10,
        OumanNumberType.CURVE_MINUS_10,
        "set_l1_curve_minus_10_number",
    ),
    (CONF_L1_CURVE_0, OumanNumberType.CURVE_0, "set_l1_curve_0_number"),
    (
        CONF_L1_CURVE_PLUS_10,
        OumanNumberType.CURVE_PLUS_10,
        "set_l1_curve_plus_10_number",
    ),
    (
        CONF_L1_CURVE_PLUS_20,
        OumanNumberType.CURVE_PLUS_20,
        "set_l1_curve_plus_20_number",
    ),
    (
        CONF_L1_MANUAL_VALVE,
        OumanNumberType.MANUAL_VALVE,
        "set_l1_manual_valve_number",
    ),
)

CONTROL_MODE_OPTIONS = [
    "Automaatti",
    "Pakko-ohjaus, normaalilämpötaso",
    "Pakko-ohjaus, lämmönpudotus",
    "Pakko-ohjaus, suuri lämmönpudotus",
    "Käsiajo, sähköinen",
    "Alasajo",
]

HOME_AWAY_OPTIONS = [
    "Kotona",
    "Ei K/P-ohjausta",
    "Poissa",
]


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
    cg.add(var.set_slow_update_interval(config[CONF_SLOW_UPDATE_INTERVAL].total_milliseconds))

    for key, setter in SENSOR_MAP:
        if conf := config.get(key):
            sens = await sensor.new_sensor(conf)
            cg.add(getattr(var, setter)(sens))

    for key, number_type, setter in NUMBER_MAP:
        if conf := config.get(key):
            num = await number.new_number(
                conf,
                number_type,
                min_value=conf[CONF_MIN_VALUE],
                max_value=conf[CONF_MAX_VALUE],
                step=conf[CONF_STEP],
            )
            await cg.register_parented(num, config[CONF_ID])
            cg.add(getattr(var, setter)(num))

    if conf := config.get(CONF_L1_CONTROL_MODE):
        sel = await select.new_select(
            conf,
            OumanSelectType.CONTROL_MODE,
            options=CONTROL_MODE_OPTIONS,
        )
        await cg.register_parented(sel, config[CONF_ID])
        cg.add(var.set_l1_control_mode_select(sel))

    if conf := config.get(CONF_HOME_AWAY_MODE):
        sel = await select.new_select(
            conf,
            OumanSelectType.HOME_AWAY_MODE,
            options=HOME_AWAY_OPTIONS,
        )
        await cg.register_parented(sel, config[CONF_ID])
        cg.add(var.set_home_away_mode_select(sel))
