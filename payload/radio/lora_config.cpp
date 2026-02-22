#include "lora_config.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <cstdlib>

using json = nlohmann::json;

namespace lora_config {

std::string default_config_path() {
    const char* env = std::getenv("LORA_CONFIG_PATH");
    if (env && env[0] != '\0') {
        return std::string(env);
    }
    std::ifstream pi_path("/home/pi/code/lora_config.json");
    if (pi_path.good()) {
        return "/home/pi/code/lora_config.json";
    }
    return "./lora_config.json";
}

static SX128x::RadioLoRaCodingRates_t coding_rate_from_string(const std::string& s) {
    if (s == "4_5") return SX128x::LORA_CR_4_5;
    if (s == "4_6") return SX128x::LORA_CR_4_6;
    if (s == "4_7") return SX128x::LORA_CR_4_7;
    if (s == "4_8") return SX128x::LORA_CR_4_8;
    if (s == "LI_4_5") return SX128x::LORA_CR_LI_4_5;
    if (s == "LI_4_6") return SX128x::LORA_CR_LI_4_6;
    if (s == "LI_4_7") return SX128x::LORA_CR_LI_4_7;
    return SX128x::LORA_CR_4_8;
}

static SX128x::RadioLoRaBandwidths_t bandwidth_from_string(const std::string& s) {
    if (s == "200") return SX128x::LORA_BW_0200;
    if (s == "400") return SX128x::LORA_BW_0400;
    if (s == "800") return SX128x::LORA_BW_0800;
    if (s == "1600") return SX128x::LORA_BW_1600;
    return SX128x::LORA_BW_1600;
}

static SX128x::RadioLoRaSpreadingFactors_t spreading_factor_from_int(int sf) {
    switch (sf) {
        case 5: return SX128x::LORA_SF5;
        case 6: return SX128x::LORA_SF6;
        case 7: return SX128x::LORA_SF7;
        case 8: return SX128x::LORA_SF8;
        case 9: return SX128x::LORA_SF9;
        case 10: return SX128x::LORA_SF10;
        case 11: return SX128x::LORA_SF11;
        case 12: return SX128x::LORA_SF12;
        default: return SX128x::LORA_SF7;
    }
}

static SX128x::RadioLoRaPacketLengthsMode_t header_type_from_string(const std::string& s) {
    if (s == "variable" || s == "explicit") return SX128x::LORA_PACKET_VARIABLE_LENGTH;
    return SX128x::LORA_PACKET_FIXED_LENGTH;
}

LoRaConfig load(const std::string& path) {
    LoRaConfig cfg;
    std::ifstream f(path);
    if (!f.good()) {
        std::cerr << "lora_config: could not open " << path << ", using defaults\n";
        return cfg;
    }
    try {
        json j = json::parse(f);
        if (j.contains("coding_rate")) cfg.coding_rate = j["coding_rate"].get<std::string>();
        if (j.contains("bandwidth")) cfg.bandwidth = j["bandwidth"].get<std::string>();
        if (j.contains("spreading_factor")) cfg.spreading_factor = j["spreading_factor"].get<int>();
        if (j.contains("packet_size")) cfg.packet_size = j["packet_size"].get<int>();
        if (j.contains("preamble_length")) cfg.preamble_length = j["preamble_length"].get<int>();
        if (j.contains("crc")) cfg.crc = j["crc"].get<bool>();
        if (j.contains("header_type")) cfg.header_type = j["header_type"].get<std::string>();
    } catch (const std::exception& e) {
        std::cerr << "lora_config: parse error: " << e.what() << ", using defaults\n";
    }
    return cfg;
}

void apply(LoRaConfig& cfg, SX128x::ModulationParams_t& modulation,
           SX128x::PacketParams_t& packet) {
    modulation.PacketType = SX128x::PACKET_TYPE_LORA;
    modulation.Params.LoRa.CodingRate = coding_rate_from_string(cfg.coding_rate);
    modulation.Params.LoRa.Bandwidth = bandwidth_from_string(cfg.bandwidth);
    modulation.Params.LoRa.SpreadingFactor = spreading_factor_from_int(cfg.spreading_factor);

    packet.PacketType = SX128x::PACKET_TYPE_LORA;
    auto& l = packet.Params.LoRa;
    l.PayloadLength = static_cast<uint8_t>(std::min(255, std::max(1, cfg.packet_size)));
    l.HeaderType = header_type_from_string(cfg.header_type);
    l.PreambleLength = static_cast<uint16_t>(std::max(0, cfg.preamble_length));
    l.Crc = cfg.crc ? SX128x::LORA_CRC_ON : SX128x::LORA_CRC_OFF;
    l.InvertIQ = SX128x::LORA_IQ_NORMAL;
}

}  // namespace lora_config
