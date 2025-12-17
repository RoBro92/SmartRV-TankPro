#pragma once

#include "esphome.h"
#include <ArduinoJson.h>
#include <algorithm>

// Parse MAC address string "AA:BB:CC:DD:EE:FF" into 6-byte array.
// Returns true on success.
inline bool tp_parse_mac_str(const std::string &str, uint8_t out[6]) {
  if (str.length() != 17) return false;
  return sscanf(str.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                &out[0], &out[1], &out[2], &out[3], &out[4], &out[5]) == 6;
}

// Format MAC bytes into buffer of at least 18 chars (including null).
inline void tp_format_mac(const uint8_t mac[6], char buf[18]) {
  snprintf(buf, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Fill a JSON params object with current configuration fields (mirrors existing lambdas).
inline void tp_pack_config_params(JsonObject params) {
  params["fill_stop_percent"] = id(fill_stop_level);
  params["drain_stop_percent"] = id(drain_stop_level);
  params["drain_timeout_ms"] = id(drain_timeout_ms);
  params["freeze_enabled"] = id(freeze_protection_enabled);
  params["freeze_threshold_c"] = id(freeze_protection_threshold_c);
  params["level_empty_volts"] = id(level_empty_volts);
  params["level_full_volts"] = id(level_full_volts);
  params["safety_override"] = id(safety_override);
  params["valve_override"] = id(valve_override);
}
