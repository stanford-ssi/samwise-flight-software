/*
 * Lora_beacon.cpp
 *
 * One-shot RPi beacon transmitter. Reads up to 253 bytes from stdin,
 * pads to PACKET_SIZE with zeros, and sends a single LoRa packet.
 *
 * Usage: sudo Lora_beacon <freq_MHz>
 *
 * Unlike Lora_tx (which sends a file using a multi-packet file-transfer
 * protocol), this program sends exactly one packet with no size header,
 * making it suitable for periodic telemetry beacons.
 */

#include "SX128x_Linux.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

constexpr size_t PACKET_SIZE = 253;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: Lora_beacon <freq in MHz>\n");
        return 1;
    }

    // Read beacon payload from stdin (up to PACKET_SIZE bytes)
    std::vector<uint8_t> payload(PACKET_SIZE, 0);
    size_t n = fread(payload.data(), 1, PACKET_SIZE, stdin);
    if (n == 0)
    {
        printf("Error: no beacon data on stdin\n");
        return 1;
    }

    // Pins based on hardware configuration (same as Lora_tx)
    SX128x_Linux Radio("/dev/spidev0.0", 0, {26, 22, 5, 19, -1, -1, 23, 24});
    Radio.SetSpiSpeed(8000000);

    Radio.Init();
    Radio.SetStandby(SX128x::STDBY_XOSC);
    Radio.SetRegulatorMode(static_cast<SX128x::RadioRegulatorModes_t>(0));
    Radio.SetLNAGainSetting(SX128x::LNA_HIGH_SENSITIVITY_MODE);
    Radio.SetTxParams(0, SX128x::RADIO_RAMP_20_US);
    Radio.SetBufferBaseAddresses(0x00, 0x00);

    SX128x::ModulationParams_t ModulationParams;
    ModulationParams.PacketType = SX128x::PACKET_TYPE_LORA;
    ModulationParams.Params.LoRa.CodingRate = SX128x::LORA_CR_4_8;
    ModulationParams.Params.LoRa.Bandwidth = SX128x::LORA_BW_1600;
    ModulationParams.Params.LoRa.SpreadingFactor = SX128x::LORA_SF7;

    SX128x::PacketParams_t PacketParams;
    PacketParams.PacketType = SX128x::PACKET_TYPE_LORA;
    auto &l = PacketParams.Params.LoRa;
    l.PayloadLength = PACKET_SIZE;
    l.HeaderType = SX128x::LORA_PACKET_FIXED_LENGTH;
    l.PreambleLength = 12;
    l.Crc = SX128x::LORA_CRC_ON;
    l.InvertIQ = SX128x::LORA_IQ_NORMAL;

    Radio.SetPacketType(SX128x::PACKET_TYPE_LORA);
    Radio.SetModulationParams(ModulationParams);
    Radio.SetPacketParams(PacketParams);

    auto freq = strtol(argv[1], nullptr, 10);
    Radio.SetRfFrequency(freq * 1000000UL);

    Radio.callbacks.txDone = [] { puts("Beacon sent!"); };

    auto IrqMask = SX128x::IRQ_TX_DONE | SX128x::IRQ_RX_TX_TIMEOUT;
    Radio.SetDioIrqParams(IrqMask, IrqMask, SX128x::IRQ_RADIO_NONE,
                          SX128x::IRQ_RADIO_NONE);
    Radio.StartIrqHandler();

    auto pkt_ToA = Radio.GetTimeOnAir();

    printf("Sending beacon (%zu payload bytes, padded to %zu)...\n", n,
           PACKET_SIZE);
    Radio.SendPayload(payload.data(), PACKET_SIZE,
                      {SX128x::RADIO_TICK_SIZE_1000_US, 1000});
    usleep((pkt_ToA + 20) * 1000);

    Radio.StopIrqHandler();
    return EXIT_SUCCESS;
}
