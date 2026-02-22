#pragma once

#include <SX128x.hpp>
#include <cstdint>
#include <string>

namespace lora_config {

struct LoRaConfig {
    std::string coding_rate = "4_8";
    std::string bandwidth = "1600";
    int spreading_factor = 7;
    int packet_size = 253;
    int preamble_length = 12;
    bool crc = true;
    std::string header_type = "fixed";
};

// Load config from JSON file. Returns default config if file missing or invalid.
LoRaConfig load(const std::string& path);

// Apply config to SX128x ModulationParams and PacketParams structs.
void apply(LoRaConfig& cfg, SX128x::ModulationParams_t& modulation,
           SX128x::PacketParams_t& packet);

// Default config path: /home/pi/code/lora_config.json or ./lora_config.json
std::string default_config_path();

}  // namespace lora_config
