# Esphome

## Ouman EH-800 ESPHome external component

This repository contains an ESPHome external component for Ouman EH-800.

The register map in this branch is based on the EH-800 web UI JavaScript and
the controller's own `measures?`, `settingsl1?`, `waterinfol1?` and
`roomtempl1?` responses.

### Architecture

The ESP32 polls EH-800 directly and publishes the latest values as normal
ESPHome entities. Home Assistant does not trigger one HTTP request per sensor.

Fast cycle (default 10 s):

1. Ensure the EH-800 login/session exists.
2. Build one batched `/request?...` query containing configured fast registers.
3. Parse the response.
4. Publish measurement sensors.

Slow cycle (default 60 s):

1. Read writable setting values and mode registers in one batched request.
2. Publish number/select states.

Writes use:

`/update?S_xxx_85=value;`

After a successful write the component immediately reads the slow group back
from EH-800. Home Assistant therefore shows the value confirmed by the
controller instead of an optimistic local value.

### Verified read-only L1 registers

| ESPHome key | EH-800 register | Web UI meaning |
|---|---|---|
| `outdoor_temperature` | `S_227_85` | Ulkolämpötila |
| `l1_supply_temperature` | `S_259_85` | L1 Menoveden lämpötila |
| `l1_room_temperature` | `S_284_85` | L1 Huonelämpötila |
| `l1_room_remote` | `S_274_85` | Huonelämpökaukoasetus / vaikutus |
| `l1_valve_position` | `S_272_85` | L1 Venttiilin asento |
| `l1_curve_supply_target` | `S_260_85` | Menovesi säätökäyrän mukaan |
| `l1_room_compensation` | `S_263_85` | Huonekompensoinnin vaikutus |
| `l1_room_compensation_time_correction` | `S_264_85` | Huonekompensoinnin aikakorjaus |
| `l1_supply_target` | `S_275_85` | Laskennallinen menoveden asetusarvo |
| `l1_smoothed_room_temperature` | `S_262_85` | Hidastettu huonelämpötilamittaus |
| `l1_calculated_room_target` | `S_278_85` | Laskennallinen huoneasetusarvo |

### Verified writable numeric registers

The EH-800's `settingsl1?` response identifies these as settings:

| ESPHome key | EH-800 register |
|---|---|
| `l1_room_setpoint` | `S_81_85` |
| `l1_temperature_drop` | `S_87_85` |
| `l1_large_temperature_drop` | `S_88_85` |
| `l1_supply_min` | `S_54_85` |
| `l1_supply_max` | `S_55_85` |
| `l1_curve_minus_20` | `S_67_85` |
| `l1_curve_minus_10` | `S_69_85` |
| `l1_curve_0` | `S_71_85` |
| `l1_curve_plus_10` | `S_73_85` |
| `l1_curve_plus_20` | `S_75_85` |
| `l1_manual_valve` | `S_92_85` |

The web responses do not expose numeric min/max limits, so the ESPHome
configuration deliberately requires an explicit `min_value` and `max_value`
for each writable number. This prevents the integration from inventing
controller limits.

### Verified mode mappings

L1 control mode uses `S_59_85`:

| Web UI option | value |
|---|---:|
| Automaatti | 0 |
| Pakko-ohjaus, normaalilämpötaso | 3 |
| Pakko-ohjaus, lämmönpudotus | 1 |
| Pakko-ohjaus, suuri lämmönpudotus | 2 |
| Käsiajo, sähköinen | 6 |
| Alasajo | 5 |

Home/away uses `S_135_85`:

| Web UI option | value |
|---|---:|
| Kotona | 0 |
| Ei K/P-ohjausta | 2 |
| Poissa | 1 |

The original web UI writes the same home/away value to `S_222_85` at the same
time. The ESPHome component mirrors that behavior.

### Usage

See `examples/ouman_eh800.yaml`.

### Status

This is still a test branch. The next important step is to compile it with the
user's ESPHome 2026.8.x installation and test the raw EH-800 responses before
merging the pull request.
