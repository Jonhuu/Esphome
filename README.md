# Esphome

## Ouman EH-800 ESPHome external component

This repository now contains an early read-only ESPHome component for Ouman EH-800.

### Architecture

The ESP32 polls the EH-800 directly and publishes the latest values as normal
ESPHome sensors. Home Assistant does not trigger one HTTP request per sensor.

One update cycle:

1. GET `/login?uid=...;pwd=...;`
2. Build one batched `/request?...` query containing only configured registers.
3. Parse the response.
4. Publish sensor states over the ESPHome native API.

The default polling interval is 15 seconds.

### Currently mapped registers

| ESPHome sensor | EH-800 register |
|---|---|
| `outdoor_temperature` | `S_227_85` |
| `l1_supply_temperature` | `S_259_85` |
| `l1_supply_target` | `S_275_85` |
| `l1_room_temperature` | `S_261_85` |
| `l1_valve_position` | `S_272_85` |

See `examples/ouman_eh800.yaml` for configuration.

### Safety

The current component is intentionally read-only. It does not call the EH-800
`/update` endpoint.

The next write-capable version should use a queued write followed by a read-back
verification instead of exposing arbitrary raw HTTP writes.
